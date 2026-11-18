// Small functions in 0x8007xxxx (experiments: scoring, loading, comments).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

// memset is declared the same way as in battle_object.cpp.
extern "C" void* memset(void* p, int c, unsigned long n);

extern "C" {
void* ScreenSetsCurrent(void* sets);                              // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);                   // 0x80198E34
void* ScreenSetFindElement(void* set, u32 hash);                  // 0x80198F18
u32   SaveCheckProgressFlag(u8* s, u32 flag);                     // 0x80225C90
void  SaveSetProgressFlag(u8* s, u32 flag, BOOL on);              // 0x80225CFC
void  SaveRewardExperiment(u8* s, u32 experiment, BOOL a);       // 0x80224A34
void  LabShowBattleResult(void* lab, const void* result);         // 0x802186D8
void  CameraSetMatrix(void* camera, const f32 mtx[3][4]);         // 0x801D6814
void* fn_801DA758(void* v);                                       // 0x801DA758
void* fn_800701A4(void);                                          // 0x800701A4
void  fn_80070FD8(u8* p);                                         // 0x80070FD8
void  fn_80009840(u8* p);                                         // 0x80009840
s32   fn_8007532C(u8* p);                                         // 0x8007532C
void  fn_8007043C(void);                                          // 0x8007043C
u8*   fn_8006FBDC(void);                                          // 0x8006FBDC
extern void* gScreenSets;                                         // 0x805FB654
extern void* gLab;                                                // 0x805FBBC8
extern u8  lbl_805FB134[];                                        // experiment object pointer
extern u8  lbl_805FBBE0[];                                        // save block pointer
extern u8  lbl_803C4298[];                                        // level info block
extern u8  lbl_8047C640[];                                        // camera
extern u8  lbl_803C5DB0[];                                        // score tracker block
extern u8  lbl_803C68A4[];                                        // score tracker float triple
extern u8  lbl_805FB5F8[];                                        // global read by fn_80073454
extern f32 lbl_805FD5DC;                                          // float
extern f32 lbl_805FD5D8;                                          // float
extern f32 lbl_805FD638;                                          // float (music volume)
extern f32 lbl_805FD648;                                          // float
extern f32 lbl_805FD64C;                                          // float
}

// Returns the experiment object if it is valid (state != 1), else NULL.
static u8* ExperimentObj(void) {
    u8* p = *(u8**)lbl_805FB134;
    if (p != NULL && *(s32*)p != 1) p = NULL;
    return p;
}

extern "C" {

// 0x8007015C
// Calls the object's slot 2 with arg 1 (if present), then clears the global.
void fn_8007015C(void) {
    u8* p = *(u8**)lbl_805FB134;
    if (p == NULL) return;
    (*(void (**)(u8*, s32))(*(u8**)(p + 4) + 8))(p, 1);
    *(u8**)lbl_805FB134 = NULL;
}

// 0x80070400
// Virtual call slot 3 on obj, passes the result through fn_801DA758, then reads a pointer chain.
void* fn_80070400(u8* obj) {
    void* r = (*(void* (**)(u8*))(*(u8**)obj + 0xC))(obj);
    u8* q = (u8*)fn_801DA758(r);
    u8* a = *(u8**)(q + 0x10);
    a = *(u8**)(a + 0x70);
    return *(void**)(a + 0x1C);
}

// 0x80070460
// Resets the float fields at +0x6C..+0x88 of the object to their defaults.
void fn_80070460(u8* p) {
    *(f32*)(p + 0x7C) = lbl_805FD5DC;
    *(f32*)(p + 0x6C) = lbl_805FD5D8;
    *(f32*)(p + 0x80) = lbl_805FD5DC;
    *(f32*)(p + 0x84) = lbl_805FD5DC;
    fn_80070FD8(p);
    *(f32*)(p + 0x88) = lbl_805FD5D8;
}

// 0x80070F4C
// Finds the widget named by the loading-screen block of the current screen set.
void* fn_80070F4C(void) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return NULL;
    u8* blk = (u8*)fn_800701A4();
    if (blk == NULL) return NULL;
    return ScreenSetFindWidget(set, *(u32*)blk);
}

// 0x80071AA0
// Shows a full-score battle result (score 100) and saves the reward experiment for the current quest.
void SetSuccessExperimentScore(void) {
    u8 res[0x18] = {0};
    res[0] = 1;
    *(u32*)(res + 4) = 0x64;
    *(u32*)(res + 8) = 0;
    *(u32*)(res + 0x10) = 0;
    LabShowBattleResult(gLab, res);
    SaveRewardExperiment(*(u8**)lbl_805FBBE0, *(u32*)((u8*)gLab + 0x48), 0x64);
}

// 0x80071AF8
// Virtual getter at vtable slot 0x64 of the experiment object, truncated to s32.
s32 GetExperimentQuality(void) {
    u8* p = ExperimentObj();
    f32 v = (*(f32 (**)(u8*))(*(u8**)(p + 4) + 0x64))(p);
    return (s32)v;
}

// 0x80071B4C
// Virtual getter at vtable slot 0x68 of the experiment object, truncated to s32.
s32 GetExperimentScore(void) {
    u8* p = ExperimentObj();
    f32 v = (*(f32 (**)(u8*))(*(u8**)(p + 4) + 0x68))(p);
    return (s32)v;
}

// 0x80073454
// Pulls the camera matrix from the level object and applies it to the camera; returns a global.
u8* fn_80073454(void) {
    f32 mtx[3][4];
    u8* obj = *(u8**)(lbl_803C4298 + 4);
    (*(void (**)(u8*, f32 (*)[4]))(*(u8**)obj + 0x38))(obj, mtx);
    CameraSetMatrix(lbl_8047C640, mtx);
    fn_80009840(lbl_803C4298);
    return *(u8**)lbl_805FB5F8;
}

// 0x80074BD8
// Pushes the experiment loading screen if the loading block has its flag set.
void PushExperimentLoadingScreen(void) {
    u8* blk = (u8*)fn_800701A4();
    if (*(s32*)(blk + 8) != 0) UIScreen_Push(0x6825C6AEu);
}

// 0x80074C38
// Starts the experiment loading music if the loading block has a track set.
void StartExperimentLoadingMusic(void) {
    u8* blk = (u8*)fn_800701A4();
    u32 track = *(u32*)(blk + 0xC);
    if (track != 0) app_PlayMusic(track, lbl_805FD638);
}

// 0x80074C6C
// Resets the experiment state and restarts the active object's slot 0x5C handler.
void RestartExperiment(void) {
    fn_8007043C();
    u8* o = fn_8006FBDC();
    (*(void (**)(u8*))(*(u8**)(o + 4) + 0x5C))(o);
}

// 0x80074DA4
// Sets the text of the experiment-screen element to the string id stored at +0x58.
void fn_80074DA4(u8* p) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    void* el = ScreenSetFindElement(set, 0xA0139595u);
    if (el == NULL) return;
    UIText_SetString_TextID(*(u32*)el, *(u32*)(p + 0x58));
}

// 0x80075298
// Returns 1 if the object's flag at +0x64 is set or the save progress flag for its id is set, else 0.
s32 fn_80075298(u8* p) {
    if (*(s32*)(p + 0x64) != 0) return 1;
    s32 flag = fn_8007532C(p);
    if (flag == -1) return 0;
    if (SaveCheckProgressFlag(*(u8**)lbl_805FBBE0, (u32)flag) != 0) return 1;
    return 0;
}

// 0x800752F4
// Sets the save progress flag for the id from fn_8007532C, if it has one.
void fn_800752F4(u8* p) {
    s32 flag = fn_8007532C(p);
    if (flag == -1) return;
    SaveSetProgressFlag(*(u8**)lbl_805FBBE0, (u32)flag, 1);
}

// 0x80075434
// Clears the score tracker block and its entry counter.
void ClearScoreTracker(void) {
    memset(lbl_803C5DB0, 0, 0xAF0);
    *(s32*)(lbl_803C5DB0 + 0xAF0) = 0;
}

// 0x80075478
// Appends an 0x8C-byte record to the score tracker (id, hi and lo bounds, clamped score).
void AddTrackableScore(u32 id, f32 a1, f32 a2, f32 a3) {
    s32 n = *(s32*)(lbl_803C5DB0 + 0xAF0);
    *(s32*)(lbl_803C5DB0 + 0xAF0) = n + 1;
    u8* e = lbl_803C5DB0 + n * 0x8C;
    *(u32*)(e + 4) = id;
    *(f32*)(e + 8) = a3;
    *(f32*)(e + 0xC) = a2;
    f32 v = a1;
    if (a1 < a2) v = *(f32*)(e + 0xC);
    if (v > *(f32*)(e + 8)) v = *(f32*)(e + 8);
    *(f32*)(e + 0) = v;
    *(s32*)(e + 0x88) = 0;
}

// 0x80075680
// Resets the score tracker and sets the three float defaults in the float triple.
void fn_80075680(void) {
    *(f32*)(lbl_803C68A4 + 0) = lbl_805FD648;
    *(f32*)(lbl_803C68A4 + 4) = lbl_805FD64C;
    *(f32*)(lbl_803C68A4 + 8) = lbl_805FD648;
    memset(lbl_803C5DB0, 0, 0xAF0);
    *(s32*)(lbl_803C5DB0 + 0xAF0) = 0;
}

// 0x80076584
// Increments the counter at +0x6C4 and writes a record (flag 0, value) into slot idx of obj's 0x38-byte entry.
void fn_80076584(u8* obj, s32 idx, s32 value) {
    *(s32*)(obj + 0x6C4) += 1;
    u8* e = obj + idx * 0x38;
    u8* r = e + (*(s32*)(e + 0x608) << 3);
    *(u8*)(r + 0x5E4) = 0;
    r = e + (*(s32*)(e + 0x608) << 3);
    *(s32*)(r + 0x5E8) = value;
    *(s32*)(e + 0x608) = (*(s32*)(e + 0x608) + 1) & 3;
}

// 0x80076864
// Clears the dance-animation pointer chain if the animation is active.
void ReleaseDanceAnimation(void* a0) {
    u8* o = (u8*)a0;
    u8* sub = *(u8**)(o + 0x9C);
    if (sub == NULL || *(s32*)(sub + 0x48) == 0) return;
    u8* r = (*(u8* (**)(u8*))(*(u8**)o + 0x10))(o);
    u8* x = *(u8**)(*(u8**)r + 0x14);
    *(s32*)(x + 0x20) = 0;
}

// 0x80070104
// True if the experiment object's float at +0x88 is above a0.
u32 CommentTimerPastThreshold(f32 a0) {
    u8* p = ExperimentObj();
    return (u32)(*(f32*)(p + 0x88) > a0);
}

} // extern "C"
