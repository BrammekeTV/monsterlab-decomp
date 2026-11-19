// Lab books: current science, recipe selection and recipe ingredient checks (0x8011xxxx).
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)
extern u8  lbl_805FBBE0[];                // 0x805FBBE0 (save block pointer)

void fn_80118ED4(u8* p, s32 val);         // 0x80118ED4
void fn_8011BAB8(u8* p, s32 a);           // 0x8011BAB8
void fn_801208F8(u8* obj, s32 hash);      // 0x801208F8
s32  fn_80119FF4(void);                   // 0x80119FF4 (selected entry index)
void fn_80119AE0(void);                   // 0x80119AE0 (tail calls fn_8011A04C)
s32  fn_8011BB20(u8* p);                  // 0x8011BB20 (reads +0x2CAC)
s32  fn_8012096C(u8* p);                  // 0x8012096C (reads +0x1A650)
s32  fn_801206A0(u8* p, s32 a);           // 0x801206A0
void fn_801207DC(u8* p, s32 a);           // 0x801207DC
s32  fn_80120504(u8* p);                  // 0x80120504
u32  fn_801203C0(u8* p);                  // 0x801203C0
BOOL SavePartTierOwned(u8* s, u32 part, s32 tier);  // 0x802241F0
}

extern "C" {

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashIngredients = 0xC66383A7u;  // "ingredients"
static const u32 kHashParts       = 0x9E14654Eu;  // "parts"
static const u32 kHashRecipes     = 0x9C2C1CAFu;  // "recipes"
static const u32 kHashBadges      = 0x880E941Cu;  // "badges"
static const u32 kHashStatsExperiment = 0xE94C961Du;  // queried on the +8 section (name unknown)

typedef u8* (*HashMethod)(u8* self, u32 hash);
typedef void (*FloatMethod)(u8* self, f32 v);

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

// Calls vfunc[0x14](0.125f) on the object when it exists.
static void TickSub(u8* obj) {
    if (obj != 0) {
        FloatMethod fn = *(FloatMethod*)(*(u8**)obj + 0x14);
        fn(obj, 0.125f);
    }
}

// Shared body of CurrentScience_*: true when the active screen (parts or recipes) is showing
// the given science (0 mechanical, 1 biological, 2 alchemy).
static s32 CurrentScienceIs(s32 science) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    if (*(s32*)top == 0 && CallVirt(*(u8**)(top + 4), 0x24, kHashParts) != 0) {
        u8* parts = SubQuery(4, kHashParts);
        if (parts != 0 && fn_8011BB20(parts) == science) return 1;
        return 0;
    }
    top = *(u8**)lbl_805FB248;
    if (top != 0 && *(s32*)top == 1 && CallVirt(*(u8**)(top + 4 + 1 * 4), 0x24, kHashRecipes) != 0) {
        u8* recipes = SubQuery(8, kHashRecipes);
        if (recipes != 0 && fn_8012096C(recipes) == science) return 1;
    }
    return 0;
}

// 0x801102D8
// True when the current science is alchemy.
s32 CurrentScience_Alchemy(void) {
    return CurrentScienceIs(2);
}

// 0x80110470
// True when the current science is biological.
s32 CurrentScience_Biological(void) {
    return CurrentScienceIs(1);
}

// 0x80110608
// True when the current science is mechanical.
s32 CurrentScience_Mechanical(void) {
    return CurrentScienceIs(0);
}

// 0x801107A0
// Pushes the chosen science to the ingredients, parts and recipes pages.
void LabBooks_SetCurrentScience(u32 a0) {
    u8* ingredients = SubQuery(4, kHashIngredients);
    u8* parts = SubQuery(4, kHashParts);
    u8* recipes = SubQuery(8, kHashRecipes);
    if (ingredients != 0) fn_80118ED4(ingredients, (s32)a0);
    if (parts != 0) fn_8011BAB8(parts, (s32)a0);
    if (recipes != 0) fn_801208F8(recipes, (s32)a0);
}

// 0x80111714
// Per-frame tick (0.125) for the ingredients, parts, recipes, stats and badges pages.
void LabBooks_UpdateLabBooks(void) {
    TickSub(SubQuery(4, kHashIngredients));
    TickSub(SubQuery(4, kHashParts));
    TickSub(SubQuery(8, kHashRecipes));
    TickSub(SubQuery(8, kHashStatsExperiment));
    TickSub(SubQuery(8, kHashBadges));
}

// 0x80111A74
// Recipe validity check for the selected entry on the recipes page.
s32 HasValidRecipe(void) {
    u8* recipes = SubQuery(8, kHashRecipes);
    if (recipes != 0) return fn_801206A0(recipes, fn_80119FF4());
    return 0;
}

// 0x80111B08
// Selects the highlighted recipe and resets the list entry records.
void SelectRecipe(void) {
    u8* recipes = SubQuery(8, kHashRecipes);
    if (recipes != 0) fn_801207DC(recipes, fn_80119FF4());
    fn_80119AE0();
}

// 0x80111B98
// True when the save block marks the recipe's secondary part as seen and owned at its tier.
s32 KnowsSecondaryForRecipe(void) {
    u8* recipes = SubQuery(8, kHashRecipes);
    if (recipes != 0) {
        u32 part = (u32)fn_80120504(recipes);
        u8 tier = (u8)fn_801203C0(recipes);
        if (SavePartTierOwned(*(u8**)lbl_805FBBE0, part, tier) != 0) return 1;
    }
    return 0;
}

// 0x80111C54
// True when the save block has a primary-ingredient slot (+0 == 0) with a nonzero count for
// the recipe's primary id (recipe record +0x10).
s32 HasPrimaryForRecipe(void) {
    u8* recipes = SubQuery(8, kHashRecipes);
    if (recipes == 0) return 0;
    s32 idx = fn_80120504(recipes);
    u8* rec = *(u8**)(gGameDb->data + 0x6C18) + idx * 0x24;
    s32 want = *(s32*)(rec + 0x10);
    u8* slot = *(u8**)lbl_805FBBE0 + 0xBE8;
    for (s32 i = 0; i < 0x69; i++, slot += 0xC) {
        if (*(s32*)(slot + 4) == want && *(s32*)(slot + 0) == 0 && *(u8*)(slot + 8) != 0) return 1;
    }
    return 0;
}

// 0x80111E28
// True when the recipe's secondary part is owned and the save block has a secondary-ingredient
// slot (+0 == 1) with a nonzero count for the matching ingredient index.
s32 HasSecondaryForRecipe(void) {
    u8* recipes = SubQuery(8, kHashRecipes);
    if (recipes == 0) return 0;
    u32 part = (u32)fn_80120504(recipes);
    u8 tier = (u8)fn_801203C0(recipes);
    if (SavePartTierOwned(*(u8**)lbl_805FBBE0, part, tier) == 0) return 0;
    s32 idx = fn_80120504(recipes);
    u8* rec = *(u8**)(gGameDb->data + 0x6C18) + idx * 0x24;
    s32 recId = *(s32*)(rec + 0x8);
    s32 recKey = *(s32*)(rec + 0x14);
    s32 found = 0x3C;
    for (s32 j = 0; j < 0x3C; j++) {
        u8* e = *(u8**)(gGameDb->data + 0x5328) + j * 0x18;
        if (*(s8*)(e + 0x13) == (s32)tier && *(s32*)(e + 0xC) == recId && *(s32*)(e + 0x8) == recKey) {
            found = j;
            break;
        }
    }
    u8* slot = *(u8**)lbl_805FBBE0 + 0xBE8;
    for (s32 i = 0; i < 0x69; i++, slot += 0xC) {
        if (*(s32*)(slot + 4) == found && *(s32*)(slot + 0) == 1 && *(u8*)(slot + 8) != 0) return 1;
    }
    return 0;
}

}  // extern "C"
