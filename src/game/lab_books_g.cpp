// Lab books UI: string/texture/count getters for the quest log, recipes, badges and
// creatures pages (0x8011xxxx) and LightSetColor (0x801DB6E0). Their results are pointers
// (or counts) handed to the script engine, so they return ScriptValue.
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8 lbl_805FB248[];   // 0x805FB248 (lab books section table pointer)
extern f32 lbl_805FFF90;    // 0x805FFF90 (light colour alpha constant)

const char* fn_8011EDE0(u8* page);   // 0x8011EDE0 (current quest name)
const char* fn_8011EEF4(u8* page);   // 0x8011EEF4 (current quest description)
const char* fn_8011F32C(u8* page);   // 0x8011F32C (next quest location string)
const char* fn_80120098(u8* page);   // 0x80120098 (body part slot name)
const char* fn_801201B4(u8* page);   // 0x801201B4 (selected recipe name)
u32         fn_80120054(u8* page);   // 0x80120054 (recipe texture list address)
s32         fn_8011CF8C(u8* p, s32 a);   // 0x8011CF8C (badge texture)
s32         fn_8011CFC0(u8* p, s32 a);   // 0x8011CFC0 (badge name)
s32         fn_8011CFF4(u8* p, s32 a);   // 0x8011CFF4 (badge description)
const char* fn_8011D038(u8* p);          // 0x8011D038 (current badge type name)
char*       fn_8011D150(u8* p);          // 0x8011D150 (formats earned/total, returns the buffer)
const char* fn_80117970(u8* p);          // 0x80117970 (monster name)
void        fn_80172E84(u8* obj);        // 0x80172E84 (light colour update)
}

extern "C" {

static const u32 kHashQuestLog = 0x0FD3AF4Du;  // "QuestLog"
static const u32 kHashRecipes  = 0x9C2C1CAFu;  // "recipes"
static const u32 kHashBadges   = 0x880E941Cu;  // "badges"
static const u32 kHashCreatures = 0xE5368B8Bu; // "creatures"

typedef u8* (*HashMethod)(u8* self, u32 hash);

// Section table (lbl_805FB248) -> sub-object at `off` -> vfunc[0x28](hash): the page object,
// or NULL if any step is missing.
static u8* PageObj(s32 off, u32 hash) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    u8* sec = *(u8**)(top + off);
    if (sec == 0) return 0;
    HashMethod fn = *(HashMethod*)(*(u8**)sec + 0x28);
    return fn(sec, hash);
}

// 0x80114568
ScriptValue QuestLog_GetQuestName(void) {
    u8* p = PageObj(8, kHashQuestLog);
    return p ? (ScriptValue)fn_8011EDE0(p) : 0;
}

// 0x801145E4
ScriptValue QuestLog_GetQuestDescription(void) {
    u8* p = PageObj(8, kHashQuestLog);
    return p ? (ScriptValue)fn_8011EEF4(p) : 0;
}

// 0x80114A90
ScriptValue QuestLog_GetNextQuestLocationString(void) {
    u8* p = PageObj(8, kHashQuestLog);
    return p ? (ScriptValue)fn_8011F32C(p) : 0;
}

// 0x80114B88
ScriptValue Recipes_GetBodyPartSlotName(void) {
    u8* p = PageObj(8, kHashRecipes);
    return p ? (ScriptValue)fn_80120098(p) : 0;
}

// 0x80114C04
ScriptValue Recipes_GetSelectedName(void) {
    u8* p = PageObj(8, kHashRecipes);
    return p ? (ScriptValue)fn_801201B4(p) : 0;
}

// 0x80114F04
ScriptValue Recipes_GetTextureList(void) {
    u8* p = PageObj(8, kHashRecipes);
    return p ? (ScriptValue)fn_80120054(p) : 0;
}

// 0x80115B68
ScriptValue Badges_GetBadgeTexture(s32 a0) {
    u8* p = PageObj(8, kHashBadges);
    return p ? (ScriptValue)fn_8011CF8C(p, a0) : 0;
}

// 0x80115BE8
ScriptValue Badges_GetBadgeName(s32 a0) {
    u8* p = PageObj(8, kHashBadges);
    return p ? (ScriptValue)fn_8011CFC0(p, a0) : 0;
}

// 0x80115C68
ScriptValue Badges_GetBadgeDescription(s32 a0) {
    u8* p = PageObj(8, kHashBadges);
    return p ? (ScriptValue)fn_8011CFF4(p, a0) : 0;
}

// 0x80115DD8
ScriptValue Badges_GetCurrentTypeName(void) {
    u8* p = PageObj(8, kHashBadges);
    return p ? (ScriptValue)fn_8011D038(p) : 0;
}

// 0x80115E48
// Returns the page's formatted "earned/total" text buffer.
ScriptValue Badges_GetBadgesEarned(void) {
    u8* p = PageObj(8, kHashBadges);
    return p ? (ScriptValue)fn_8011D150(p) : 0;
}

// 0x80113710
ScriptValue Creatures_GetMonsterName(void) {
    u8* p = PageObj(4, kHashCreatures);
    return p ? (ScriptValue)fn_80117970(p) : 0;
}

// 0x801DB6E0
// Sets the RGB of a light (alpha constant) and refreshes it.
void LightSetColor(u8* light, f32 r, f32 g, f32 b) {
    if (light == 0) return;
    *(f32*)(light + 0x44) = r;
    *(f32*)(light + 0x48) = g;
    *(f32*)(light + 0x4C) = b;
    *(f32*)(light + 0x50) = lbl_805FFF90;
    fn_80172E84(light);
}

}  // extern "C"
