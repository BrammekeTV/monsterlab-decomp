// More robot evolution experiment functions (0x8009xxxx).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);   // 0x801DC91C
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32   RaySceneTest(void* ray);                                    // 0x80070458
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);             // 0x800690D4 (base ctor)
void  fn_80097BFC(u8* p);                                         // 0x80097BFC
void  fn_800999B4(u8* obj, f32 v);                                // 0x800999B4
}

// Door callbacks defined in small_8009.cpp (C++ linkage).
void RobotEvolutionExperiment_DoorOpenning(u8* p);
void RobotEvolutionExperiment_DoorOpen(u8* p);
void RobotEvolutionExperiment_DoorClosing(u8* p);
void RobotEvolutionExperiment_DoorClosed(u8* p);

extern void* gEventMgr;                                           // 0x805FB6C8
extern u8    lbl_8035BD58[];                                      // registration name block
extern u8    lbl_8035C3F0[];                                      // vtable
extern u8*   lbl_803C6A70[3];                                     // robot table (key at +4)
extern u8*   lbl_803C6A88[3];                                     // smasher table (key at +4)
extern s32   lbl_803471B0[];                                      // table indexed by RaySceneTest result
extern s32   lbl_805F8A38;                                        // last random value
extern f32   lbl_805FDB08;                                        // float threshold
extern f32   lbl_805FDBA0;                                        // float 0.0
extern f32   lbl_805FDBC8;                                        // float constant

// Finds the robot table entry whose key at +4 equals key, or NULL.
static u8* RobotEntryFind(u8* key) {
    for (s32 i = 0; i < 3; i++) {
        if (*(u8**)(lbl_803C6A70[i] + 0x4) == key) return lbl_803C6A70[i];
    }
    return NULL;
}

// Finds the smasher table entry whose key at +4 equals key, or NULL.
static u8* SmasherEntryFind(u8* key) {
    for (s32 i = 0; i < 3; i++) {
        if (*(u8**)(lbl_803C6A88[i] + 0x4) == key) return lbl_803C6A88[i];
    }
    return NULL;
}

// 0x80090674
// Scales the object's +0x354 value by a table entry picked from the RaySceneTest result.
f32 fn_80090674(u8* p) {
    s32 idx = RaySceneTest(p);
    f32 a = (f32)*(s32*)(p + 0x354);
    f32 b = (f32)(s32)(lbl_803471B0[idx] * 2);
    return b * a;
}

// 0x80091DAC
// Registers the four robot door script callbacks.
void fn_80091DAC(void) {
    RegisterScriptFunc((const char*)(lbl_8035BD58 + 0x0),  0x4E, (void*)RobotEvolutionExperiment_DoorOpenning);
    RegisterScriptFunc((const char*)(lbl_8035BD58 + 0x28), 0x4E, (void*)RobotEvolutionExperiment_DoorOpen);
    RegisterScriptFunc((const char*)(lbl_8035BD58 + 0x4C), 0x4E, (void*)RobotEvolutionExperiment_DoorClosing);
    RegisterScriptFunc((const char*)(lbl_8035BD58 + 0x74), 0x4E, (void*)RobotEvolutionExperiment_DoorClosed);
}

// 0x8009253C
// Counts the timer at +0x38 down by dt while enabled; when it reaches zero (and +0x08 is 0),
// clears the enable flag and sends event 0xDC155FBA to the object at +0x04.
void fn_8009253C(u8* p, f32 dt) {
    if (*(u8*)(p + 0x34) == 0) return;
    f32 t = *(f32*)(p + 0x38) - dt;
    *(f32*)(p + 0x38) = t;
    if (*(s32*)(p + 0x8) != 0) return;
    if (t > lbl_805FDB08) return;
    *(u8*)(p + 0x34) = 0;
    EventSend(gEventMgr, *(void**)(p + 0x4), NULL, 0xDC155FBAu, 0, 0, 1, 0);
}

// 0x80092CC4
// Returns the robot table entry whose key at +4 equals the argument, or NULL.
u8* fn_80092CC4(u8* key) {
    return RobotEntryFind(key);
}

// 0x80093278
// Stores the last random value into the matched robot entry at +0x44 and returns it; -1 if no match.
s32 RobotEvolutionExperiment_RobotGetLastRandom(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return -1;
    s32 v = lbl_805F8A38;
    *(s32*)(e + 0x44) = v;
    return v;
}

// 0x800932E0
// True if the matched robot's +0x00 state is 0 (left).
s32 RobotEvolutionExperiment_RobotIsLeft(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(s32*)e == 0;
}

// 0x80093350
// True if the matched robot's +0x00 state is 1 (center).
s32 RobotEvolutionExperiment_RobotIsCenter(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(s32*)e == 1;
}

// 0x800933C0
// True if the matched robot's +0x00 state is 2 (right).
s32 RobotEvolutionExperiment_RobotIsRight(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(s32*)e == 2;
}

// 0x80093430
// True if the matched robot's taunt byte at +0x3C is set.
s32 RobotEvolutionExperiment_RobotTaunts(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(u8*)(e + 0x3C) != 0;
}

// 0x800934A0
// True if the matched robot's +0x40 flee mode is 0.
s32 RobotEvolutionExperiment_RobotFleesLeft(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(s32*)(e + 0x40) == 0;
}

// 0x80093510
// True if the matched robot's +0x40 flee mode is 2.
s32 RobotEvolutionExperiment_RobotFleesRight(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(s32*)(e + 0x40) == 2;
}

// 0x80093608
// True if the matched robot's +0x08 state is 4 (cheer after winning the game).
s32 RobotEvolutionExperiment_RobotGameWonCheer(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(s32*)(e + 0x8) == 4;
}

// 0x80093678
// True if the matched robot's +0x08 state is 5 (flee after losing the game).
s32 RobotEvolutionExperiment_RobotGameLostFlee(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(s32*)(e + 0x8) == 5;
}

// 0x800936E8
// True if the matched robot's +0x14 flag byte is set.
s32 RobotEvolutionExperiment_RobotIsCorrectRobot(u8* obj) {
    u8* e = RobotEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(u8*)(e + 0x14) != 0;
}

// 0x80093758
// Sets up a looping animation sound on the matched robot: +0x28 = -1, +0x2C = a2, +0x30 = a1.
void RobotEvolutionExperiment_RobotSetLoopingAnimSound(ScriptCtx* a0, u32 a1, u32 a2) {
    u8* e = RobotEntryFind(*(u8**)((u8*)a0 + 0x10));
    if (e == NULL) return;
    *(s32*)(e + 0x28) = -1;
    *(u32*)(e + 0x2C) = a2;
    *(u32*)(e + 0x30) = a1;
}

// 0x80093FBC
// True if the matched smasher's impact-frame byte at +0x1C is set.
s32 RobotEvolutionExperiment_SmasherCheckImpactFrame(u8* obj) {
    u8* e = SmasherEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(u8*)(e + 0x1C) != 0;
}

// 0x8009402C
// True if the matched smasher's hit byte at +0x1D is set.
s32 RobotEvolutionExperiment_SmasherHitSomething(u8* obj) {
    u8* e = SmasherEntryFind(*(u8**)(obj + 0x10));
    if (e == NULL) return 0;
    return *(u8*)(e + 0x1D) != 0;
}

// 0x80097750
// Clears the object's counters: +0x00 = 0, +0x04 = -1, zeroes the +0x14 array (count at +0x08)
// and the +0x0C array (count at +0x09), then resets both counts to 0.
void fn_80097750(u8* p) {
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x4) = -1;
    for (s32 i = 0; i < *(u8*)(p + 0x8); i++) *(s32*)(p + 0x14 + i * 4) = 0;
    for (s32 i = 0; i < *(u8*)(p + 0x9); i++) *(s32*)(p + 0xC + i * 4) = 0;
    *(u8*)(p + 0x9) = 0;
    *(u8*)(p + 0x8) = 0;
}

// 0x8009803C
// Counts the timer at +0x270 down by dt. When it reaches zero, calls fn_800999B4 on the object
// at +0x264 with the negated value at +0x274 and resets the timer to 0.
void fn_8009803C(u8* p, f32 dt) {
    fn_80097BFC(p);
    if (!(*(f32*)(p + 0x270) > lbl_805FDBA0)) return;
    f32 t = *(f32*)(p + 0x270) - dt;
    *(f32*)(p + 0x270) = t;
    if (!(t <= lbl_805FDBA0)) return;
    fn_800999B4(*(u8**)(p + 0x264), -*(f32*)(p + 0x274));
    *(f32*)(p + 0x270) = lbl_805FDBA0;
}

// 0x8009848C
// Constructor: runs the base constructor, sets the vtable and the initial state fields
// (+0x264 = 0, +0x268 = -1, flags at +0x26C/+0x26D/+0x26E = 0/0/1, timers +0x270/+0x274 = 0.0,
// +0x278 = lbl_805FDBC8), and returns the object.
void* fn_8009848C(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(void**)(p + 0x0) = (void*)lbl_8035C3F0;
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = -1;
    *(u8*)(p + 0x26C) = 0;
    *(u8*)(p + 0x26D) = 0;
    *(u8*)(p + 0x26E) = 1;
    *(f32*)(p + 0x270) = lbl_805FDBA0;
    *(f32*)(p + 0x274) = lbl_805FDBA0;
    *(f32*)(p + 0x278) = lbl_805FDBC8;
    return obj;
}
