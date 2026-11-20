// Lab books: ingredient / recipe selection and active-part queries (0x8011xxxx).
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)
extern u8  lbl_805FBBE0[];                // 0x805FBBE0 (save block pointer)

void fn_8011726C(u8* arg0, s32 arg1);   // 0x8011726C
u8*  fn_80118C24(u8* ingredients);        // 0x80118C24 (selected ingredient entry or NULL)
s32  fn_80118C54(u8* p, s32 a, s32 b);    // 0x80118C54
void fn_80118E88(u8* p, s32 a, s32 b);    // 0x80118E88
void fn_80119810(u8* entry);              // 0x80119810
void fn_80119858(u8* entry);              // 0x80119858
void fn_801198A0(s32 a, u32 b);           // 0x801198A0
s32  fn_80119FF4(void);                   // 0x80119FF4 (selected entry index)
BOOL fn_8011C214(u8* p);                  // 0x8011C214
s32  fn_80120390(u8* recipes);            // 0x80120390
u32  fn_801203C0(u8* p);                  // 0x801203C0
s32  fn_80120504(u8* p);                  // 0x80120504
}

extern "C" {

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashIngredients = 0xC66383A7u;  // "ingredients"
static const u32 kHashParts       = 0x9E14654Eu;  // "parts"
static const u32 kHashRecipes     = 0x9C2C1CAFu;  // "recipes"
static const u32 kHashCreatures   = 0xE5368B8Bu;  // "creatures"

typedef u8* (*HashMethod)(u8* self, u32 hash);

// Calls the virtual slot at byte offset `off` of obj with a hash argument; returns r3.
static u8* CallVirt(u8* obj, s32 off, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + off);
    return fn(obj, hash);
}

// lbl_805FB248 -> table pointer -> +off sub-object (NULL if the table is missing).
static u8* LabBooksSub(s32 off) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    return *(u8**)(top + off);
}

// sub-object (+off)->vfunc[0x28](hash), or NULL when the sub-object is missing.
static u8* SubQuery(s32 off, u32 hash) {
    u8* sub = LabBooksSub(off);
    if (sub == 0) return 0;
    return CallVirt(sub, 0x28, hash);
}

// True when the active screen is the one named by `hash` (vfunc[0x24] on the current section).
static bool ScreenIs(s32 screen, u32 hash) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return false;
    if (*(s32*)top != screen) return false;
    return CallVirt(*(u8**)(top + screen * 4 + 4), 0x24, hash) != 0;
}

// 0x801120EC
// True when the ingredients list has a valid key ingredient entry.
s32 HasValidKeyIngredient(void) {
    u8* ing = SubQuery(4, kHashIngredients);
    if (ing != 0) return fn_80118C54(ing, fn_80119FF4(), 0);
    return 0;
}

// 0x80112178
// Selects the current entry as the key ingredient.
void SelectKeyIngredient(void) {
    u8* ing = SubQuery(4, kHashIngredients);
    if (ing != 0) fn_80118E88(ing, fn_80119FF4(), 0);
}

// 0x801121FC
// True when the ingredients list has a valid secondary ingredient entry.
s32 HasValidSecondaryIngredient(void) {
    u8* ing = SubQuery(4, kHashIngredients);
    if (ing != 0) return fn_80118C54(ing, fn_80119FF4(), 1);
    return 0;
}

// 0x80112288
// Selects the current entry as the secondary ingredient.
void SelectSecondaryIngredient(void) {
    u8* ing = SubQuery(4, kHashIngredients);
    if (ing != 0) fn_80118E88(ing, fn_80119FF4(), 1);
}

// 0x8011230C
// Applies the selected ingredient entry to list 0 or list 1 depending on its kind (+4).
void UpdateSelectedIngredients(void) {
    u8* ing = SubQuery(4, kHashIngredients);
    if (ing != 0) {
        u8* entry = fn_80118C24(ing);
        if (entry != 0) {
            s32 kind = *(s32*)(entry + 4);
            if (kind == 0) {
                fn_80119810(entry);
            } else if (kind == 1) {
                fn_80119858(entry);
            }
        }
    }
}

// 0x8011239C
// Pushes the selected recipe's tier into the lab (fn_801198A0).
void UpdateSelectedRecipe(void) {
    u8* recipes = SubQuery(8, kHashRecipes);
    if (recipes != 0) {
        u32 tier = fn_801203C0(recipes);
        s32 part = fn_80120504(recipes);
        fn_801198A0(part, tier);
    }
}

// 0x80112A3C
// Id of the part currently active: from the parts screen, the recipes screen, or the
// lightning tower selection.
ScriptValue GetActivePartID(void) {
    if (ScreenIs(0, kHashParts)) {
        u8* parts = SubQuery(4, kHashParts);
        if (parts == 0) return 0;
        u8* save = *(u8**)lbl_805FBBE0;
        s32 idx = fn_8011C214(parts);
        return **(s32**)(save + idx * 4 + 0xA58);
    }
    if (ScreenIs(1, kHashRecipes)) {
        u8* recipes = SubQuery(8, kHashRecipes);
        if (recipes == 0) return 0;
        return (ScriptValue)fn_80120390(recipes);
    }
    if (LighthingTowerGetSelectingPart() == 1) {
        return LightningGetInventoryPartID();
    }
    return 0;
}

// 0x80112BE0
// Index of the current inventory part on the parts screen (or the lightning tower position).
ScriptValue GetCurrentInventoryPart(void) {
    if (LighthingTowerGetSelectingPart() == 1) {
        return LightningGetInventoryPos();
    }
    if (ScreenIs(0, kHashParts)) {
        u8* parts = SubQuery(4, kHashParts);
        if (parts != 0) return fn_8011C214(parts);
        return 0;
    }
    return 0;
}

// 0x80112FFC
// Byte at +0x11 of the selected part's record (0 when there is no parts screen).
u8 fn_80112FFC(void) {
    u8* parts = SubQuery(4, kHashParts);
    if (parts != 0) {
        u8* save = *(u8**)lbl_805FBBE0;
        s32 idx = fn_8011C214(parts);
        return *(u8*)(*(u8**)(save + idx * 4 + 0xA58) + 0x11);
    }
    return 0;
}

// 0x80113238
// Forwards the argument to the creatures list when that section exists.
void Creatures_IncCreature(s32 a0) {
    u8* creatures = SubQuery(4, kHashCreatures);
    if (creatures != 0) fn_8011726C(creatures, a0);
}

}  // extern "C"
