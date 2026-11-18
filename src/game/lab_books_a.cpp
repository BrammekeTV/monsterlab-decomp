// Lab books UI script callbacks: current screen and ingredient lists (0x8011xxxx).
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)

s32  fn_80118F3C(u8* p);                  // 0x80118F3C
const char* fn_80118F44(u8* p);                 // 0x80118F44
void fn_80118FC0(u8* p, s32 a);           // 0x80118FC0
void fn_801190E8(u8* p, s32 a);           // 0x801190E8
u32  fn_801191AC(u8* p);                  // 0x801191AC
void fn_801191C8(u8* p);                  // 0x801191C8
void fn_8011954C(u8* p, s32 a);           // 0x8011954C
u32  fn_80119538(u8* p);                  // 0x80119538
s32  fn_80119600(u8* p, s32 a);           // 0x80119600
u32  fn_80119630(u8* p);                  // 0x80119630
u32  fn_80119644(u8* p);                  // 0x80119644
u32  fn_80119658(u8* p);                  // 0x80119658
s32  fn_801196A8(u8* p);                  // 0x801196A8
}

// Defined in small_8011.cpp with C++ linkage.
void fn_801194E0(u8* p, s32 a);           // 0x801194E0
u8*  fn_8011966C(u8* p);                  // 0x8011966C

extern "C" {

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashIngredients = 0xC66383A7u;  // "ingredients"
static const u32 kHashParts       = 0x9E14654Eu;  // "parts"
static const u32 kHashCreatures   = 0xE5368B8Bu;  // "creatures"
static const u32 kHashRecipes     = 0x9C2C1CAFu;  // "recipes"
static const u32 kHashBadges      = 0x880E941Cu;  // "badges"

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

// Shared lookup for the ingredient list callbacks: section->vfunc[0x28](ingredients), or NULL.
static u8* IngredientsObj(void) {
    u8* sec = LabBooksSection();
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashIngredients);
}

// Shared check for CurrentScreen_*: table[0] must equal index, then the screen at
// table + 4 + index*4 answers the hash query through vfunc 0x24.
static s32 CurrentScreenIs(s32 index, u32 hash) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    if (*(s32*)top != index) return 0;
    u8* cur = *(u8**)(top + 4 + index * 4);
    if (CallVirt(cur, 0x24, hash) != 0) return 1;
    return 0;
}

// 0x80110084
s32 CurrentScreen_Ingredients(void) {
    return CurrentScreenIs(0, kHashIngredients);
}

// 0x801100F0
s32 CurrentScreen_Parts(void) {
    return CurrentScreenIs(0, kHashParts);
}

// 0x8011015C
s32 CurrentScreen_Creatures(void) {
    return CurrentScreenIs(0, kHashCreatures);
}

// 0x801101C8
s32 CurrentScreen_Recipes(void) {
    return CurrentScreenIs(1, kHashRecipes);
}

// 0x80110234
s32 CurrentScreen_Badges(void) {
    return CurrentScreenIs(1, kHashBadges);
}

// 0x801108F4
BOOL LabBooks_Ingredients_GetCurrentType(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return fn_80118F3C(p);
}

// 0x80110964
BOOL LabBooks_Ingredients_GetTypeName(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return (BOOL)(uintptr_t)fn_80118F44(p);  // TODO: header return type cannot hold a pointer on x64
}

// 0x801109D4
void LabBooks_Ingredients_IncrementType(s32 a0) {
    u8* p = IngredientsObj();
    if (p == 0) return;
    fn_80118FC0(p, a0);
}

// 0x80110A4C
void LabBooks_Ingredients_IncrementRow(s32 a0) {
    u8* p = IngredientsObj();
    if (p == 0) return;
    fn_801190E8(p, a0);
}

// 0x80110AC4
s32 LabBooks_Ingredients_ShowIngredientFilters(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return (s32)fn_801191AC(p);
}

// 0x80110B34
void LabBooks_Ingredients_RenderList_Generate(void) {
    u8* p = IngredientsObj();
    if (p == 0) return;
    fn_801191C8(p);
}

// 0x80110B9C
void LabBooks_Ingredients_RenderList_SetSelectedItem(BOOL a0) {
    u8* p = IngredientsObj();
    if (p == 0) return;
    fn_801194E0(p, a0);
}

// 0x80110C14
void LabBooks_Ingredients_RenderList_IncSelectedItem(s32 a0) {
    u8* p = IngredientsObj();
    if (p == 0) return;
    fn_8011954C(p, a0);
}

// 0x80110C8C
BOOL LabBooks_Ingredients_RenderList_GetSelectedItem(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return (BOOL)fn_80119538(p);
}

// 0x80110CFC
BOOL LabBooks_Ingredients_RenderList_GetTextureList(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return (BOOL)fn_80119630(p);
}

// 0x80110D6C
s32 Ingredients_IsValidIngredient(s32 a0) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return fn_80119600(p, a0);
}

// 0x80110DEC
BOOL LabBooks_Ingredients_RenderList_GetNumberList(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return (BOOL)fn_80119644(p);
}

// 0x80110E5C
BOOL LabBooks_Ingredients_RenderList_GetIconList(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return (BOOL)fn_80119658(p);
}

// 0x80110ECC
// The header types this as BOOL, but the callee returns a pointer (value passed through as is).
BOOL LabBooks_Ingredients_RenderList_GetSelectedName(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return (BOOL)(uintptr_t)fn_8011966C(p);
}

// 0x80110F3C
BOOL Ingredients_RenderList_GetSelectedImage(void) {
    u8* p = IngredientsObj();
    if (p == 0) return 0;
    return fn_801196A8(p);
}

}  // extern "C"
