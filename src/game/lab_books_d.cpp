// Lab books UI script callbacks: experiment slider, quest log and recipes pages (0x8011xxxx).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)

void fn_8011D6F4(u8* p);                  // 0x8011D6F4
void fn_8011D708(u8* p);                  // 0x8011D708
void fn_8011E9B8(u8* p);                  // 0x8011E9B8
s32  fn_8011EDAC(u8* p);                  // 0x8011EDAC (0/1 flag; the argument is not read)
BOOL fn_8011EFB8(u8* p);                  // 0x8011EFB8 (always returns 0; the argument is not read)
u32  fn_8011F1C4(u8* p);                  // 0x8011F1C4 (0/1 flag from the object's field)
s32  fn_8011F1D8(u8* p);                  // 0x8011F1D8 (field value from the object)
s32  fn_8011F2FC(u8* p);                  // 0x8011F2FC (0/1 flag)
void fn_8011FF28(u8* p);                  // 0x8011FF28
u32  fn_801203C0(u8* p);                  // 0x801203C0 (float field converted to unsigned; return type assumed)
u32  fn_80120084(u8* p);                  // 0x80120084 (leaf defines an s32 argument; pointer passed)
s32  fn_80120418(u8* p);                  // 0x80120418 (leaf defines an s32 argument; pointer passed)
u8   fn_80120A84(void);                   // 0x80120A84 (leaf takes no arguments)
}

extern "C" {

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
// The names behind the statistics and quest log hashes are not identified yet.
static const u32 kHashStatsExperiment = 0xE94C961Du;  // queried on the statistics section (name unknown)
static const u32 kHashQuestLog        = 0x0FD3AF4Du;  // queried on the quest log section (name unknown)
static const u32 kHashRecipes         = 0x9C2C1CAFu;  // "recipes"

typedef u8* (*HashMethod)(u8* self, u32 hash);

// Calls the virtual slot at byte offset 0x28 of obj with a hash argument; returns r3.
static u8* CallHashVirt(u8* obj, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + 0x28);
    return fn(obj, hash);
}

// lbl_805FB248 -> table pointer -> +8 sub-object (NULL if the table is missing).
static u8* LabBooksSub(void) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    return *(u8**)(top + 8);
}

// Shared lookup: sub-object->vfunc[0x28](hash), or NULL when the sub-object is missing.
static u8* SubQuery(u32 hash) {
    u8* sub = LabBooksSub();
    if (sub == 0) return 0;
    return CallHashVirt(sub, hash);
}

// 0x80114390
void Statistics_ExperimentPageUp(void) {
    u8* obj = SubQuery(kHashStatsExperiment);
    if (obj != 0) fn_8011D6F4(obj);
}

// 0x80114404
void Statistics_ExperimentPageDown(void) {
    u8* obj = SubQuery(kHashStatsExperiment);
    if (obj != 0) fn_8011D708(obj);
}

// 0x80114478
void Statistics_SetupExperimentSlider(void) {
    u8* obj = SubQuery(kHashStatsExperiment);
    if (obj != 0) fn_8011E9B8(obj);
}

// 0x801144EC
s32 QuestLog_HasQuest(void) {
    u8* obj = SubQuery(kHashQuestLog);
    if (obj == 0) return 0;
    return fn_8011EDAC(obj);
}

// 0x80114660
BOOL QuestLog_GetQuestJournalEntry(void) {
    u8* obj = SubQuery(kHashQuestLog);
    if (obj == 0) return 0;
    return fn_8011EFB8(obj);
}

// 0x8011490C
s32 QuestLog_HasMainIcon(s32 a0) {
    (void)a0;  // the original overwrites r3 and never reads the argument
    u8* obj = SubQuery(kHashQuestLog);
    if (obj == 0) return 0;
    return fn_8011F1C4(obj);
}

// 0x80114988
BOOL QuestLog_GetMainIcon(s32 a0) {
    (void)a0;  // the original overwrites r3 and never reads the argument
    u8* obj = SubQuery(kHashQuestLog);
    if (obj == 0) return 0;
    return fn_8011F1D8(obj);
}

// 0x80114B0C
s32 QuestLog_HasNextQuestLocationString(void) {
    u8* obj = SubQuery(kHashQuestLog);
    if (obj == 0) return 0;
    return fn_8011F2FC(obj);
}

// 0x80114E14

// (not in the script table; the return type is assumed from fn_801203C0)
u32 fn_80114E14(void) {
    u8* obj = SubQuery(kHashRecipes);
    if (obj == 0) return 0;
    return fn_801203C0(obj);
}

// 0x80114E90
void Recipes_SetupRenderList(void) {
    u8* obj = SubQuery(kHashRecipes);
    if (obj != 0) fn_8011FF28(obj);
}

// 0x80114F80
BOOL Recipes_GetImage(void) {
    u8* obj = SubQuery(kHashRecipes);
    if (obj == 0) return 0;
    return fn_80120418(obj);
}

// 0x80114FFC
s32 Recipes_RenderList_GetSelectedItem(void) {
    u8* obj = SubQuery(kHashRecipes);
    if (obj == 0) return 0;
    return fn_80120084(obj);
}

// 0x80115A04
s32 Recipes_IsTextureAsyncLoaded(void) {
    u8* obj = SubQuery(kHashRecipes);
    if (obj == 0) return 0;
    return fn_80120A84();
}

}
