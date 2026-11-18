// Lab books UI script callbacks: ingredient actions and the parts page (0x8011xxxx).
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)

s32  fn_801191C0(u8* p);                  // 0x801191C0
s32  fn_80118BD4(u8* p);                  // 0x80118BD4
void fn_80119C6C(u8* p);                  // 0x80119C6C
void fn_8011B898(u8* p);                  // 0x8011B898
s32  fn_8011BA5C(u8* p);                  // 0x8011BA5C
void fn_8011BA48(u8* p, s32 a);           // 0x8011BA48
void fn_8011C318(u8* p);                  // 0x8011C318
BOOL fn_8011BA7C(u8* p);                  // 0x8011BA7C
BOOL fn_8011BA90(u8* p);                  // 0x8011BA90
BOOL fn_8011BAA4(u8* p);                  // 0x8011BAA4
void fn_8011BCF8(u8* p, s32 a);           // 0x8011BCF8
void fn_8011BDA8(u8* p, s32 a);           // 0x8011BDA8
BOOL fn_8011BB28(u8* p);                  // 0x8011BB28
BOOL fn_8011BF34(u8* p);                  // 0x8011BF34
s32  fn_8011C018(u8* p, s32 a);           // 0x8011C018
BOOL fn_8011C214(u8* p);                  // 0x8011C214
BOOL fn_8011C108(u8* p);                  // 0x8011C108
s32  fn_8011C130(u8* p);                  // 0x8011C130
s32  fn_8011C168(u8* p);                  // 0x8011C168
}

// Defined in small_8011.cpp with C++ linkage.
extern "C" void fn_80118BDC(u8* s);                  // 0x80118BDC

extern "C" {

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashIngredients         = 0xC66383A7u;  // "ingredients"
static const u32 kHashParts               = 0x9E14654Eu;  // "parts"
static const u32 kHashIngredientSelection = 0xEBD0DA39u;  // "IngredientSelection"

typedef u8* (*HashMethod)(u8* self, u32 hash);

// Calls the virtual slot at byte offset `off` of obj with a hash argument; returns r3.
static u8* CallVirt(u8* obj, s32 off, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + off);
    return fn(obj, hash);
}

// lbl_805FB248 -> pointer -> +4 sub-object (NULL if the table is missing).
static u8* LabBooksSection(void) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    return *(u8**)(top + 4);
}

// Section->vfunc[0x28](ingredients), or NULL.
static u8* IngredientsObj(void) {
    u8* sec = LabBooksSection();
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashIngredients);
}

// Section->vfunc[0x28](IngredientSelection), or NULL.
static u8* IngredientSelectionObj(void) {
    u8* sec = LabBooksSection();
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashIngredientSelection);
}

// Section->vfunc[0x28](parts), or NULL.
static u8* PartsObj(void) {
    u8* sec = LabBooksSection();
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashParts);
}

// 0x80111928
// Plays the science play-off animation on the ingredients page; returns the animation's result.
s32 Ingredients_SciencePlayOffAnim(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return fn_801191C0(p);
}

// 0x80111998
// Removes the currently selected ingredient.
void Ingredients_RemoveCurrentIngredient(void) {
    u8* p = IngredientsObj();
    if (p == 0) return;
    fn_80118BDC(p);
}

// 0x80111A00
// Reports whether the selected ingredient's texture has finished loading.
s32 Ingredients_IsTextureAsyncLoaded(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return fn_80118BD4(p);
}

// 0x8011243C
// Refreshes the recipe shown for the selected ingredients.
void UpdateSelectedIngredientsRecipe(void) {
    u8* p = IngredientSelectionObj();
    if (p == 0) return;
    fn_80119C6C(p);
}

// 0x801124E4
// Builds the parts render list.
void Parts_SetupRenderList(void) {
    u8* p = PartsObj();
    if (p == 0) return;
    fn_8011B898(p);
}

// 0x8011254C
// Returns the selected entry of the parts render list.
s32 Parts_RenderList_GetSelectedItem(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011BA5C(p);
}

// 0x801125BC
// Selects entry a0 in the parts render list.
void Parts_RenderList_SetSelectedItem(s32 a0) {
    u8* p = PartsObj();
    if (p == 0) return;
    fn_8011BA48(p, a0);
}

// 0x80112634
// Shows the info images for the selected part.
void Parts_GetInfoImages(void) {
    u8* p = PartsObj();
    if (p == 0) return;
    fn_8011C318(p);
}

// 0x8011269C
// Reports whether the parts texture list is available.
BOOL Parts_GetTextureList(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011BA7C(p);
}

// 0x8011270C
// Reports whether the parts modification list is available.
BOOL Parts_GetModificationList(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011BA90(p);
}

// 0x8011277C
// Reports whether the parts on the monster list is available.
BOOL Parts_GetOnMonsterList(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011BAA4(p);
}

// 0x801127EC
// Steps the selected body part by a0.
void Parts_IncrementBodyPart(s32 a0) {
    u8* p = PartsObj();
    if (p == 0) return;
    fn_8011BCF8(p, a0);
}

// 0x80112864
// Steps the parts list row by a0.
void Parts_IncrementRow(s32 a0) {
    u8* p = PartsObj();
    if (p == 0) return;
    fn_8011BDA8(p, a0);
}

// 0x801128DC
// Reports whether the body part slot name is available.
BOOL Parts_GetBodyPartSlotName(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011BB28(p);
}

// 0x8011294C
// Reports whether the selected part's name is available.
BOOL Parts_GetSelectedName(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011BF34(p);
}

// 0x801129BC
// Returns the name for the part at index a0.
s32 Parts_GetNameByIndex(s32 a0) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011C018(p, a0);
}

// 0x80112CC4
// Reports whether a current part is set.
BOOL Parts_GetCurrentPart(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011C214(p);
}

// 0x80112D34
// Reports whether the selected part's image is available.
BOOL Parts_GetImage(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011C108(p);
}

// 0x80112DA4
// Reports whether the selected part has a recipe.
s32 Parts_HasRecipe(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011C130(p);
}

// 0x80112E14
// Reports whether the selected part can be deleted.
s32 Parts_CanDelete(void) {
    u8* p = PartsObj();
    if (p == 0) return 0;
    return fn_8011C168(p);
}

}  // extern "C"
