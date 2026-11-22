// Small functions from the 0x8008-0x800C range (ray scene pick, UI widget setup, plane/ring
// buffer queries, event dispatch, quaternion helpers). Written from the asm; functional, not
// byte-matching.
#include "types.h"
#include "engine/math.h"
#include <math.h>

extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };

// Globals defined in C++ files.
extern "C++" void* gEventMgr;
extern "C++" u8 gWorldState[];
extern "C++" void* lbl_805FBBAC;

// Data labels (float/int constants and tables).
extern u8* lbl_805FB120;
extern s32 lbl_805F8A00;
extern u8 lbl_803C4298[];
extern u8 lbl_8047C760[];
extern f32 lbl_805FD8C8;
extern f32 lbl_805FD928;
extern f32 lbl_805FDB28;
extern f32 lbl_805FDB3C;
extern f32 lbl_805FDB64;
extern f32 lbl_805FDC28;
extern f32 lbl_805FDC50;
extern f32 lbl_805FDC68;
extern f32 lbl_805FDC6C;
extern f32 lbl_805FDC70;
extern f32 lbl_805FDC74;
extern f32 lbl_803462C8;
extern f32 lbl_805FE108;
extern f32 lbl_805FE118;
extern f32 lbl_805FE178;
extern f32 lbl_805FE194;
extern f32 lbl_805FE198;
extern f32 lbl_805FE19C;
extern f32 lbl_805FE1A0;
extern f32 lbl_805FE1C8;
extern f32 lbl_805FE200;
extern f32 lbl_805FE23C;
extern f32 lbl_805FE240;
extern f32 lbl_805FE244;
extern f32 lbl_805FE32C;
extern f32 lbl_805FE390;
extern f32 lbl_805FE3B0;
extern f32 lbl_805FE3B4;
extern f32 lbl_805FE3D8;
extern f32 lbl_80346068[];
extern f32 lbl_80346388[];
extern f32 lbl_80346478[];
extern f32 lbl_80346488[];
extern f32 lbl_80346730[];
extern f32 lbl_80346740[];
extern f32 lbl_80346FC0[];
extern f32 lbl_803460A8[];
extern f32 lbl_803470F0[];
extern f32 lbl_80347100[];
extern f32 lbl_80347110[];
extern f32 lbl_80347120[];
extern f32 lbl_80347130[];
extern f32 lbl_80347140[];
extern s32 lbl_803470A0[];
extern u32 lbl_80347DA8[];

// Virtual / slot calls: `(*(Fn*)(*(u8**)obj + OFF))(obj, ...)`, or the object-field form used
// where the asm loads the slot base straight from a field (no extra dereference).
typedef f32  (*VFnF32)(u8*);
typedef f32  (*VFnF32Arg)(u8*, f32);
typedef u8*  (*VFnPtr)(u8*);
typedef void (*VFnVoid)(u8*);
typedef void (*VFnVoidArg)(u8*, u8*);
typedef void (*VFnVoidInt)(u8*, s32);
typedef void (*VFnVoidF)(u8*, f32);

static u8* CallSlot0C(u8* obj) {
    return (*(VFnPtr*)(*(u8**)obj + 0xC))(obj);
}

// Calls slot `off` of the widget's +0x138 table with (widget, 2).
static void WidgetCall2(void* w, s32 off) {
    (*(VFnVoidInt*)(*(u8**)((u8*)w + 0x138) + off))((u8*)w, 2);
}

// fn_80097750 is defined with C++ linkage in small_8009c.cpp, so it is declared outside extern "C".
void fn_80097750(u8* p);  // 0x80097750

extern "C" {

f32   fn_800C3E84(u8* arg0);                                                  // 0x800C3E84
void* CharacterFind(s32 id);                                                // 0x800E876C
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);   // 0x801BA2FC
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);                // 0x80209E70
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);              // 0x80145EE8
u32   RandomU32(void);                                                        // 0x80143460
s32   RaySceneTest(void* ray);                                                // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                                          // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                               // 0x801F53B0
s32   SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);   // 0x801C80E0
void  UISetProgress(void* widget, f32 t);                                     // 0x801A3FA0
void* UIWidgetFindChild(void* widget, u32 hash);                              // 0x80199AEC
void* WorldFindLocator(void* world, u32 hash);                                // 0x8012AA50
void  fn_80070460(u8* p);                                                     // 0x80070460
void* fn_80070F4C(void);                                                      // 0x80070F4C
void  fn_80070FAC(u8* p, f32 v);                                              // 0x80070FAC
void  fn_800712BC(u8* p, s32 a, s32 b);                                       // 0x800712BC
void  fn_800715B8(u8* p, s32 a);                                              // 0x800715B8
void  fn_800716D0(u8* p, u32 a);                                              // 0x800716D0
u8*   fn_80073454(void);                                                      // 0x80073454
void  fn_80097B78(u8* p);                                                     // 0x80097B78
void  fn_80099078(u8* p);                                                     // 0x80099078
void  fn_800980C8(u8* obj, s32 mode);                                         // 0x800980C8
void* fn_8009A47C(u8* p);                                                     // 0x8009A47C
void  fn_8009AC44(u8* p, f32 dt);                                             // 0x8009AC44
void  fn_8009AEA0(u8* p, f32 dt);                                             // 0x8009AEA0
void  fn_8009C618(void* h, f32 t);                                            // 0x8009C618
void  fn_800C176C(f32* out, u8* obj);                                         // 0x800C176C
u8*   fn_800E4D7C(u32 a0);                                                    // 0x800E4D7C
void  fn_8014474C(u8* src, f32* out);                                         // 0x8014474C
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);                           // 0x80145F9C
s32 fn_801A403C(u8* w, f32 t);                                              // 0x801A403C
s32 fn_801A4050(u8* w, f32 t);                                              // 0x801A4050
u8*   fn_801B3308(void);                                                      // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                            // 0x801B3378
void* fn_801D40E0(s32 n);                                                     // 0x801D40E0
void* fn_80209814(void* v);                                                   // 0x80209814
void  fn_8020B1D4(void* h, s32 on);                                           // 0x8020B1D4
s32 fn_800B66F0(u8* p, s32 mode);                                           // 0x800B66F0

// Sets the three progress-bar fields of a UI widget to their start values.
static void SetupProgressWidget(void* w) {
    fn_801A403C((u8*)w, lbl_805FE108);
    fn_801A4050((u8*)w, lbl_805FE118);
    UISetProgress(w, lbl_805FE108);
}

// 0x800854D4
void fn_800854D4(u8* arg0, f32 farg0) {
    f32 t = *(f32*)(arg0 + 0xC) - farg0;
    u8* ctl = *(u8**)(*(u8**)(arg0 + 0x4) + 0x29C);
    *(f32*)(arg0 + 0xC) = t;
    if (t < lbl_805FD8C8) {
        if (CallSlot0C(ctl) != NULL) {
            u8* r = CallSlot0C(ctl);
            *(s32*)(r + 0x8) |= 2;
        }
        u8* sub = *(u8**)(ctl + 0x4);
        *(s32*)(sub + 0x20) |= 2;
        return;
    }
    f32 t2 = *(f32*)(arg0 + 0x10) - farg0;
    *(f32*)(arg0 + 0x10) = t2;
    if (t2 < lbl_805FD8C8) {
        // RaySceneTest ignores its argument; chained calls pass NULL.
        s32 i = RaySceneTest(arg0);
        s32 j = RaySceneTest(NULL);
        f32 f30 = lbl_80346730[i];
        f32 f31 = lbl_80346740[j];
        f32 f1 = -f31;
        f32 rnd = 2.3283064e-10f * (f32)RandomU32();
        *(f32*)(arg0 + 0x10) = f30 + (f1 + ((f31 - f1) * rnd));
        u8* sub = *(u8**)(ctl + 0x4);
        if (*(s32*)(sub + 0x20) & 2) {
            if (CallSlot0C(ctl) != NULL) {
                u8* r = CallSlot0C(ctl);
                *(s32*)(r + 0x8) &= ~2;
            }
            sub = *(u8**)(ctl + 0x4);
            *(s32*)(sub + 0x20) &= ~2;
        } else {
            if (CallSlot0C(ctl) != NULL) {
                u8* r = CallSlot0C(ctl);
                *(s32*)(r + 0x8) |= 2;
            }
            sub = *(u8**)(ctl + 0x4);
            *(s32*)(sub + 0x20) |= 2;
        }
    }
}

// 0x8008A58C
// Picks one of the ten flagged slots at +0x380 at random (clearing it), or runs the 0x3B8 counter
// when +0x90 == 1. Returns the picked index, or -1.
s32 fn_8008A58C(u8* arg0) {
    if (*(s32*)(arg0 + 0x90) == 1) {
        if (lbl_805F8A00 == 1) {
            s32 v = *(s32*)(arg0 + 0x3B8) + 1;
            *(s32*)(arg0 + 0x3B8) = v;
            if (v >= 9) lbl_805F8A00 = -1;
        } else {
            s32 v = *(s32*)(arg0 + 0x3B8) - 1;
            *(s32*)(arg0 + 0x3B8) = v;
            if (v <= 0) lbl_805F8A00 = 1;
        }
        return *(s32*)(arg0 + 0x3B8);
    }
    s32 count = 0;
    for (s32 i = 0; i < 10; i++) {
        if (*(f32*)(arg0 + 0x380 + i * 4) >= lbl_805FD928) count++;
    }
    if (count == 0) return -1;
    s32 pick = (s32)(RandomU32() % (u32)count);
    for (s32 i = 0; i < 10; i++) {
        f32* slot = (f32*)(arg0 + 0x380 + i * 4);
        if (*slot >= lbl_805FD928) {
            if (pick == 0) {
                *slot = lbl_805FD928;
                return i;
            }
            pick--;
        }
    }
    return -1;
}

// 0x80094C0C
// Resets the playing-state record: clears the 27 entries at +0x384, releases the +0x9C slots,
// resets the sound and random-timer fields, stops the related sounds and gets the voice handle.
void fn_80094C0C(u8* arg0) {
    for (s32 k = 0; k < 27; k++) fn_80097750(arg0 + 0x384 + k * 0x20);
    for (s32 k = 0; k < *(s32*)(arg0 + 0x1B4); k++) {
        fn_80097B78((u8*)*(void**)(arg0 + 0x9C + k * 4));
        *(void**)(arg0 + 0x9C + k * 4) = NULL;
    }
    *(s32*)(arg0 + 0x1B4) = 0;
    fn_80099078((u8*)*(void**)(arg0 + 0x90));
    *(void**)(arg0 + 0x90) = NULL;
    u8* o = *(u8**)(arg0 + 0x94);
    *(s32*)(arg0 + 0x98) = 0;
    (*(VFnVoid*)(*(u8**)o + 0xC0))(o);
    *(void**)(arg0 + 0x94) = NULL;
    *(f32*)(arg0 + 0x35C) = lbl_805FDB28;
    *(f32*)(arg0 + 0x360) = lbl_805FDB28;
    *(f32*)(arg0 + 0x364) = lbl_805FDB28;
    f32 rnd = 2.3283064e-10f * (f32)RandomU32();
    *(f32*)(arg0 + 0x8C) = lbl_805FDB28;
    u8* q = *(u8**)(arg0 + 0x1D8);
    *(f32*)(arg0 + 0x368) = rnd;
    if (q != NULL) {
        fn_800980C8(q, 0);
        *(void**)(arg0 + 0x1DC) = *(void**)(arg0 + 0x1D8);
    }
    *(void**)(arg0 + 0x1D8) = NULL;
    *(void**)(arg0 + 0x1DC) = NULL;
    *(f32*)(arg0 + 0x36C) = lbl_805FDB28;
    *(u8*)(arg0 + 0x370) = 0;
    u8* a = fn_801B3308();
    fn_801B3378(a, arg0 + 0x294);
    u8* b = fn_801B3308();
    fn_801B3378(b, arg0 + 0x1E0);
    StopExperimentSound(NULL, 0x10461444U);
    StopExperimentSound(NULL, 0x29CB2881U);
    StopExperimentSound(NULL, 0x3EB03CC2U);
    for (s32 k = 0; k < 3; k++) StopExperimentSound(NULL, lbl_80347DA8[k]);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    fn_8020B1D4(lbl_805FBBAC, 0);
}

// 0x800972CC
// Weighted random pick over the four-stage table set; returns 0..2, or -1 for none.
s32 fn_800972CC(u8* arg0) {
    f32 f29 = lbl_805FDB64 - (lbl_805FDB64 - (*(f32*)(arg0 + 0x358) / lbl_805FDB3C));
    s32 i0 = RaySceneTest(arg0);
    if (lbl_803470A0[i0] == 2) f29 *= f29;
    s32 i1 = RaySceneTest(NULL);
    f32 f30 = lbl_803470F0[i1];
    s32 i2 = RaySceneTest(NULL);
    f30 = f29 * (lbl_80347120[i2] - f30);
    s32 i3 = RaySceneTest(NULL);
    f32 f31 = lbl_803470F0[i3] + f30;
    s32 i4 = RaySceneTest(NULL);
    f30 = lbl_80347100[i4];
    s32 i5 = RaySceneTest(NULL);
    f30 = f29 * (lbl_80347130[i5] - f30);
    s32 i6 = RaySceneTest(NULL);
    f32 f28 = lbl_80347100[i6] + f30;
    s32 i7 = RaySceneTest(NULL);
    f30 = lbl_80347110[i7];
    s32 i8 = RaySceneTest(NULL);
    f30 = f29 * (lbl_80347140[i8] - f30);
    s32 i9 = RaySceneTest(NULL);
    f29 = lbl_80347110[i9] + f30;
    f32 f1 = f31 + f28;
    f32 total = f29 + f1;
    f32 r = total * (2.3283064e-10f * (f32)RandomU32());
    if (r < f31) return 0;
    if (r < f1) return 1;
    if (r < total) return 2;
    return -1;
}

// 0x80099078
// Resets the per-object counters and picks the 0x298 target from the RaySceneTest index.
// The original also contains a block after its early branch that nothing jumps to; not reproduced.
void fn_80099078(u8* arg0) {
    s32 i = RaySceneTest(arg0);
    *(s32*)(arg0 + 0x268) = 0;
    *(f32*)(arg0 + 0x298) = lbl_80346FC0[i];
    *(f32*)(arg0 + 0x270) = lbl_805FDC28;
}

// 0x8009A7C4
// Sets up the spawn/locator state: three effect spawns at the locators, the picked
// timing values, and a locked call on the character found by id.
void fn_8009A7C4(u8* arg0) {
    (*(VFnVoidArg*)(*(u8**)(arg0 + 0x170) + 0xC))(arg0 + 0x9C, arg0 + 0x180);
    (*(VFnVoid*)(*(u8**)(arg0 + 0x170) + 0x18))(arg0 + 0x9C);
    (*(VFnVoid*)(*(u8**)(arg0 + 0x170) + 0x18))(arg0 + 0x9C);
    u8* r = (*(VFnPtr*)(*(u8**)(arg0 + 0x4) + 0x54))(arg0);
    s32 i = RaySceneTest(r);
    *(f32*)(arg0 + 0x1C0) = lbl_80346478[i];
    s32 j = RaySceneTest(NULL);
    *(s32*)(arg0 + 0x98) = -1;
    *(f32*)(arg0 + 0x94) = lbl_80346388[j];
    *(void**)(arg0 + 0x1A4) = WorldFindLocator(gWorldState, 0x3C49F42CU);
    *(void**)(arg0 + 0x1A8) = WorldFindLocator(gWorldState, 0x4B4EC4BAU);
    void* loc3 = WorldFindLocator(gWorldState, 0xD2479500U);
    *(void**)(arg0 + 0x1AC) = loc3;
    // Last r3 value of the locator lookups; passed through to fn_800E4D7C below.
    intptr_t lastRet = (intptr_t)loc3;
    f32 zero = lbl_805FDC50;
    for (s32 k = 0; k < 3; k++) {
        u8* loc = (u8*)*(void**)(arg0 + 0x1A4 + k * 4);
        if (loc != NULL) {
            *(s32*)(arg0 + 0x1B0 + k * 4) = 0;
            f32 pos[4];
            pos[0] = *(f32*)(loc + 0x8);
            pos[1] = *(f32*)(loc + 0xC);
            pos[2] = *(f32*)(loc + 0x10);
            pos[3] = zero;
            lastRet = (intptr_t)SpawnEffectAt(lbl_8047C760, 0xBE21A016U, pos, (u8**)(arg0 + 0x1B0 + k * 4));
        }
    }
    *(f32*)(arg0 + 0x1BC) = zero;
    *(u8*)(fn_800E4D7C((u32)lastRet) + 0x54) = 0;
    *(u8*)((u8*)fn_8009A47C(arg0) + 0x54) = 1;
    u8* ch = (u8*)CharacterFind(0x6D235EC6);
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(VFnVoidInt*)(*(u8**)ch + 0x18))(ch, 0);
    ScopedLockDtor(&lock, -1);
    fn_8009C618(*(void**)(arg0 + 0x198), zero);
}

// 0x8009A9C4
// Per-frame update: ticks the timer at +0x8C, then either fires the end-of-state signal or
// runs the movement/tick helpers.
void fn_8009A9C4(u8* arg0, f32 farg0) {
    *(f32*)(arg0 + 0x8C) = *(f32*)(arg0 + 0x8C) - farg0;
    f32 pct = (*(VFnF32*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
    fn_80070FAC(arg0, pct / lbl_805FDC6C);
    if (pct <= lbl_805FDC50) {
        GameFlowSignal(0x117741C3, 0, 0);
        return;
    }
    bool cond;
    if (lbl_803462C8 > lbl_805FDC50) {
        cond = *(f32*)(arg0 + 0x8C) <= lbl_805FDC50;
    } else {
        f32 pos = fn_800C3E84(arg0 + 0x9C);
        f32 d = (*(VFnF32*)(*(u8**)(arg0 + 0x170) + 0x20))(arg0 + 0x9C) - pos;
        cond = d > lbl_805FDC70 && d < lbl_805FDC74;
    }
    if (cond) {
        s32 idx = RaySceneTest(NULL);
        if (*(f32*)(arg0 + 0x1C0) >= lbl_80346488[idx]) {
            GameFlowSignal(-0x39F737BE, 0, 0);
        } else {
            GameFlowSignal(0x117741C3, 0, 0);
        }
        return;
    }
    f32 v = *(f32*)(arg0 + 0x8C);
    fn_800712BC(arg0, (s32)v, (s32)(v <= lbl_805FDC68));
    (*(VFnVoidF*)(*(u8**)(arg0 + 0x170) + 0x14))(arg0 + 0x9C, farg0);
    fn_8009AC44(arg0, farg0);
    fn_8009C618(*(void**)(arg0 + 0x198), farg0);
    fn_8009AEA0(arg0, farg0);
}

// 0x800B2F98
// Clears the UI widget slots at +0xA2C..+0xA44 and, when the HUD root exists, looks up the
// widgets, sets the progress bars and the per-widget states.
void fn_800B2F98(u8* arg0) {
    *(void**)(arg0 + 0xA2C) = NULL;
    *(void**)(arg0 + 0xA30) = NULL;
    *(void**)(arg0 + 0xA34) = NULL;
    *(void**)(arg0 + 0xA38) = NULL;
    *(void**)(arg0 + 0xA3C) = NULL;
    *(void**)(arg0 + 0xA40) = NULL;
    *(void**)(arg0 + 0xA44) = NULL;
    void* p = fn_80070F4C();
    if (p == NULL) return;
    void* w = UIWidgetFindChild(p, 0x57EFBC28U);
    *(void**)(arg0 + 0xA2C) = w;
    if (w != NULL) SetupProgressWidget(w);
    w = UIWidgetFindChild(p, 0xCEE6ED92U);
    *(void**)(arg0 + 0xA30) = w;
    if (w != NULL) SetupProgressWidget(w);
    w = UIWidgetFindChild(p, 0x552FC6C7U);
    *(void**)(arg0 + 0xA34) = w;
    if (w != NULL) {
        s32 idx = RaySceneTest(w);
        if (lbl_80346068[idx] <= lbl_805FE108) {
            WidgetCall2(w, 0x30);
        } else {
            WidgetCall2(w, 0x2C);
        }
    }
    w = UIWidgetFindChild(p, 0xD40638A9U);
    *(void**)(arg0 + 0xA38) = w;
    WidgetCall2(w, 0x30);
    w = UIWidgetFindChild(p, 0x218007DCU);
    *(void**)(arg0 + 0xA3C) = w;
    WidgetCall2(w, 0x30);
    w = UIWidgetFindChild(p, 0x58F351CCU);
    *(void**)(arg0 + 0xA40) = w;
    WidgetCall2(w, 0x30);
    w = UIWidgetFindChild(p, 0xAD756EB9U);
    *(void**)(arg0 + 0xA44) = w;
    WidgetCall2(w, 0x30);
}

// 0x800B68C0
// Adjusts the value at +0x2A0 by the event amount (arg1 2 = add, 1 = subtract), decays it,
// fires a sound on a large jump, and syncs +0x2A4 to it.
void fn_800B68C0(u8* arg0, s32 arg1, f32 farg0, f32 farg1) {
    if (arg1 == 2) {
        *(f32*)(arg0 + 0x2A0) = *(f32*)(arg0 + 0x2A0) + farg0 * farg1;
    } else if (arg1 == 1) {
        *(f32*)(arg0 + 0x2A0) = *(f32*)(arg0 + 0x2A0) - farg0 * farg1;
    }
    f32 v = *(f32*)(arg0 + 0x2A0);
    v = v - (lbl_805FE194 * v * farg1);
    *(f32*)(arg0 + 0x2A0) = v;
    s32 r0 = 0;
    if (v > lbl_805FE198 && v < lbl_805FE19C) r0 = 1;
    if (r0 != 0) *(f32*)(arg0 + 0x2A0) = lbl_805FE178;

    s32 r3 = fn_800B66F0(arg0, 0);
    if (r3 == 0 && *(f32*)(arg0 + 0x2A0) != lbl_805FE178) {
        f32 v2 = *(f32*)(arg0 + 0x2A0);
        s32 idx = RaySceneTest(0);
        v2 = v2 - (farg1 * (v2 * lbl_803460A8[idx]));
        *(f32*)(arg0 + 0x2A0) = v2;
        s32 r0b = 0;
        if (v2 > lbl_805FE198 && v2 < lbl_805FE19C) r0b = 1;
        if (r0b != 0) *(f32*)(arg0 + 0x2A0) = lbl_805FE178;
    }

    f32 a4 = *(f32*)(arg0 + 0x2A4);
    f32 a0 = *(f32*)(arg0 + 0x2A0);
    if ((a4 * a0) < lbl_805FE178 && fabsf(a0 - a4) > lbl_805FE1A0) {
        if (lbl_805FBBAC == NULL) {
            void* h = fn_801D40E0(1);
            if (h != NULL) h = fn_80209814(h);
            lbl_805FBBAC = h;
        }
        PlaySound(0x42A93043U, 0, 0, 0, 0, 1);
    }
    f32 cur = *(f32*)(arg0 + 0x2A0);
    if (fabsf(cur - *(f32*)(arg0 + 0x2A4)) > lbl_805FE1A0) {
        *(f32*)(arg0 + 0x2A4) = cur;
    }
}

// 0x800B8A68
// Sets the state byte at +0x2E8 and picks an event hash from the arguments, then sends that event
// to the object list at +0xB0 and to the object at +0x28C.
void fn_800B8A68(u8* arg0, s32 arg1, f32 farg0, f32 farg1) {
    u32 ev;
    if (arg1 != 0) {
        *(u8*)(arg0 + 0x2E8) = (u8)arg1;
        ev = 0xB79D93B4U;
    } else {
        if (farg0 >= lbl_805FE200) {
            ev = 0x1E7FD221U;
        } else if (farg0 > lbl_805FE1C8) {
            ev = 0xC2689D6BU;
        } else {
            ev = 0x8F4CEA5EU;
        }
    }
    if (arg1 != 0 && *(f32*)(arg0 + 0x2EC) != farg1) {
        *(f32*)(arg0 + 0x2EC) = farg1;
        ev = 0xABB71F82U;
    }
    if (*(u8*)(arg0 + 0x2E8) == 1 && arg1 == 0) {
        *(u8*)(arg0 + 0x2E8) = 0;
        ev = 0x9C12D5FDU;
        *(f32*)(arg0 + 0x2EC) = lbl_805FE1C8;
    }
    if (ev != 0) {
        u8* node = *(u8**)(arg0 + 0xB0);
        while (node != NULL) {
            u8* obj = *(u8**)(node + 0x4);
            if (obj != NULL) {
                u8* sub = *(u8**)(obj + 0x4);
                if (!(*(s32*)(sub + 0x20) & 1)) {
                    *(s32*)(sub + 0x20) |= 1;
                    EventSend(gEventMgr, obj, NULL, ev, 0, 0, 1, 0);
                    sub = *(u8**)(obj + 0x4);
                    *(s32*)(sub + 0x20) &= ~1;
                } else {
                    EventSend(gEventMgr, obj, NULL, ev, 0, 0, 1, 0);
                }
            }
            node = *(u8**)(node + 0x8);
        }
        u8* owner = *(u8**)(arg0 + 0x28C);
        u8* sub2 = *(u8**)(owner + 0x4);
        *(s32*)(sub2 + 0x20) |= 1;
        EventSend(gEventMgr, owner, NULL, ev, 0, 0, 1, 0);
        sub2 = *(u8**)(*(u8**)(arg0 + 0x28C) + 0x4);
        *(s32*)(sub2 + 0x20) &= ~1;
    }
}

// 0x800BA690
// Re-seeds the transform at the object's +0x4 slot from the global pose record, offsetting it
// along the pose's Y axis, and resets the scale/flags of the object.
void fn_800BA690(u8* arg0) {
    fn_80070460(arg0);
    EventSendTo(gEventMgr, 0x83B4950AU, NULL, 0xCA90298EU, 0, 0);
    if (lbl_805FB120 != NULL) {
        u8* q = *(u8**)(*(u8**)(lbl_803C4298 + 0x4) + 0x4);
        Quat in = { *(f32*)(q + 0xC), *(f32*)(q + 0x10), *(f32*)(q + 0x14), *(f32*)(q + 0x18) };
        Vec3 axis = QuatGetAxisY(&in);
        f32 s = lbl_805FE23C;
        f32 nx = *(f32*)(q + 0x0) + axis.x * s;
        f32 ny = *(f32*)(q + 0x4) + axis.y * s;
        f32 nz = *(f32*)(q + 0x8) + axis.z * s;
        u8* U = *(u8**)(lbl_805FB120 + 0x4);
        *(f32*)(U + 0x0) = nx;
        *(f32*)(U + 0x4) = ny;
        *(f32*)(U + 0x8) = nz;
        U = *(u8**)(lbl_805FB120 + 0x4);
        *(s32*)(U + 0x20) |= 0x8000;
        *(s32*)(U + 0x20) |= 0x10000000;
        *(s32*)(U + 0x20) |= 0x4000;
        *(s32*)(U + 0x20) |= 0x10000000;
    }
    f32 hv = *(f32*)(arg0 + 0x90);
    fn_800712BC(arg0, (s32)lrintf(hv), (s32)(hv < lbl_805FE240));
    f32 rate = (*(VFnF32*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
    fn_80070FAC(arg0, rate / lbl_805FE244);
    fn_800715B8(arg0, 0);
    fn_800716D0(arg0, 0U);
}

// 0x800BF044
// Returns 1 when the target at +0x4 is within the range for the current +0x290 type: type 4 =
// sphere (squared distance to arg1 >= 255), type 5 = box (point beyond the +0x270..+0x284 range).
s32 fn_800BF044(u8* arg0, f32* arg1) {
    s32 type = *(s32*)(arg0 + 0x290);
    if (type == 4) {
        u8* p = *(u8**)(arg0 + 0x4);
        f32 dx = *(f32*)(p + 0x0) - arg1[0];
        f32 dy = *(f32*)(p + 0x4) - arg1[1];
        f32 dz = *(f32*)(p + 0x8) - arg1[2];
        f32 d2 = (dx * dx + dy * dy) + dz * dz;
        if (d2 >= lbl_805FE32C) return 1;
        return 0;
    }
    if (type == 5) {
        u8* p = *(u8**)(arg0 + 0x4);
        f32 lo0 = *(f32*)(arg0 + 0x270);
        f32 lo1 = *(f32*)(arg0 + 0x274);
        f32 lo2 = *(f32*)(arg0 + 0x278);
        f32 rx = *(f32*)(arg0 + 0x27C) - lo0;
        f32 ry = *(f32*)(arg0 + 0x280) - lo1;
        f32 rz = *(f32*)(arg0 + 0x284) - lo2;
        f32 px = *(f32*)(p + 0x0) - lo0;
        f32 py = *(f32*)(p + 0x4) - lo1;
        f32 pz = *(f32*)(p + 0x8) - lo2;
        f32 pos2 = (px * px + py * py) + pz * pz;
        f32 rng2 = (rx * rx + ry * ry) + rz * rz;
        if (pos2 > rng2) return 1;
        return 0;
    }
    return 0;
}

// 0x800C2AB0
// Returns 1 when the arg1 point lies beyond the plane table at +0xC0 in both the current view
// vector and the target position, after the ring buffer at +0x6C..+0x74 has enough entries.
s32 fn_800C2AB0(u8* arg0, u8* arg1) {
    s32 r6 = *(s32*)(arg0 + 0x6C);
    s32 count = 0;
    if (r6 >= 0 && *(s32*)(arg0 + 0x70) >= 0) {
        s32 x = *(s32*)(arg0 + 0x70);
        if (x < r6) x += *(s32*)(arg0 + 0x74);
        count = x - r6 + 1;
    }
    if (count <= *(s32*)(arg0 + 0xD0)) return 0;
    if (*(u8*)(arg1 + 0x26D) == 0) return 0;

    u8* base = fn_80073454();
    f32 v[3];
    fn_800C176C(v, arg1);
    s32 found1 = 0;
    for (s32 i = 0; i < 6; i++) {
        f32* pl = (f32*)(base + 0xC0 + i * 0x10);
        if (v[0] * pl[0] + v[1] * pl[1] + v[2] * pl[2] > pl[3]) {
            found1 = 1;
            break;
        }
    }
    if (!found1) return 0;

    f32* pos = *(f32**)(arg1 + 0x4);
    for (s32 i = 0; i < 6; i++) {
        f32* pl = (f32*)(base + 0xC0 + i * 0x10);
        if (pos[0] * pl[0] + pos[1] * pl[1] + pos[2] * pl[2] > pl[3]) return 1;
    }
    return 0;
}

// 0x800C3E84
// Walks the ring of objects at +0x78 (start +0x70, count +0x74, cursor +0x7C, window +0x6C),
// summing the value each one returns from its slot 0xD0.
f32 fn_800C3E84(u8* arg0) {
    f32 sum = lbl_805FE390;
    s32 s = *(s32*)(arg0 + 0x6C);
    s32 st = *(s32*)(arg0 + 0x70);
    if (s < 0 || st < 0) return sum;

    s32 n = *(s32*)(arg0 + 0x74);
    u8** base = *(u8***)(arg0 + 0x78);
    u8** cur = NULL;
    if (st >= 0 && st < n) {
        s32 lo = st;
        s32 hi = st;
        if (st < s) {
            lo = st + n;
            hi = st + n;
        }
        if (s <= lo && lo <= hi) cur = base + st;
    }
    *(u8***)(arg0 + 0x7C) = cur;
    u8* e = (cur != NULL) ? *cur : NULL;
    while (e != NULL) {
        sum += (*(VFnF32*)(*(u8**)e + 0xD0))(e);
        u8** c = *(u8***)(arg0 + 0x7C);
        if (c == NULL) break;
        u8** next = NULL;
        s32 ic = (s32)(c - base);
        if (ic != s) {
            s32 j = ic - 1;
            if (j < 0) j = n - 1;
            if (j >= 0 && j < n) {
                s32 stv = *(s32*)(arg0 + 0x70);
                s32 lo = j;
                s32 hi = stv;
                if (stv < s) {
                    if (j <= stv) lo = j + n;
                    hi = stv + n;
                }
                if (s <= lo && lo <= hi) next = base + j;
            }
        }
        *(u8***)(arg0 + 0x7C) = next;
        e = (next != NULL) ? *next : NULL;
    }
    return sum;
}

// 0x800C509C
// Slerps the pose quaternions at the +0x264 and +0x268 indices (scaled by farg0 / the object's
// duration), then multiplies arg0 by the conjugate of an axis-angle rotation about +Y.
void fn_800C509C(f32* arg0, u8* arg1, f32 farg0) {
    f32 scale = farg0 / (*(VFnF32*)(*(u8**)arg1 + 0xD0))(arg1);
    f32 q1[4] = { lbl_805FE3B0, lbl_805FE3B0, lbl_805FE3B0, lbl_805FE3B4 };
    f32 q2[4];
    s32 i264 = *(s32*)(arg1 + 0x264);
    if (i264 >= 0) fn_8014474C(*(u8**)(arg1 + 0x98) + i264 * 0xC8 + 0x38, q1);
    s32 i268 = *(s32*)(arg1 + 0x268);
    fn_8014474C(*(u8**)(arg1 + 0x98) + i268 * 0xC8 + 0x38, q2);
    fn_80145F9C(arg0, q1, q2, scale);

    Vec3 axis = { lbl_805FE3B0, lbl_805FE3B4, lbl_805FE3B0 };
    Quat qa;
    QuatFromAxisAngle(&qa, &axis, lbl_805FE3D8 * *(f32*)(arg1 + 0x2B4));
    f32 qx = -qa.x;
    f32 qy = -qa.y;
    f32 qz = -qa.z;
    f32 qw = qa.w;

    f32 ax = arg0[0];
    f32 ay = arg0[1];
    f32 az = arg0[2];
    f32 aw = arg0[3];
    arg0[0] = ay * qz + aw * qx + ax * qw - az * qy;
    arg0[1] = az * qx + ay * qw + aw * qy - ax * qz;
    arg0[2] = az * qw + aw * qz + ax * qy - ay * qx;
    arg0[3] = aw * qw - ax * qx - ay * qy - az * qz;
}

}  // extern "C"
