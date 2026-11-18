// More small functions in 0x800Bxxxx (grow juice, arm wrestle).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                         // 0x801D426C
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);                   // 0x800690D4 (base ctor)
void  fn_80062CE0(u8* p, s32 del);                                      // 0x80062CE0 (base dtor)
void  fn_800B0B88(u8* p);                                               // 0x800B0B88
void  fn_800704AC(u8* p);                                               // 0x800704AC
void  fn_800B82E0(u8* child, f32 delta);                                // 0x800B82E0
void  fn_800B8180(u8* p);                                               // 0x800B8180
BOOL  fn_801B50B0(void* unused, u8* obj);                               // 0x801B50B0
u32   fn_801EC880(u8* p, u32 hash);                                     // 0x801EC880
s32   RaySceneTest(void* ray);                                          // 0x80070458
u32   RandomU32(void);                                                  // 0x80143460
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190
}

extern void* gEventMgr;             // 0x805FB6C8
extern u8  lbl_8035DE40[];          // 0x8035DE40 (vtable)
extern u8  lbl_8035E470[];          // 0x8035E470 (vtable)
extern u8  lbl_8035E568[];          // 0x8035E568 (vtable)
extern u8  lbl_8035E678[];          // 0x8035E678 (vtable)
extern f32 lbl_805FE178;            // 0x805FE178 (float constant, zero/reset value)
extern f32 lbl_805FE17C;            // 0x805FE17C (threshold)
extern f32 lbl_805FE180;            // 0x805FE180 (idle timer threshold)
extern f32 lbl_805FE1B8;            // 0x805FE1B8 (float default)
extern f32 lbl_805FE1C0;            // 0x805FE1C0 (float default)
extern f32 lbl_805FE208;            // 0x805FE208 (float constant)
extern f32 lbl_805FE20C;            // 0x805FE20C (float constant)
extern f32 lbl_805FE25C;            // 0x805FE25C (threshold)
extern f32 lbl_80346068[];          // 0x80346068 (f32 table indexed by RaySceneTest)
extern f32 lbl_80346078[];          // 0x80346078 (f32 table indexed by RaySceneTest)

extern "C" {

// 0x800B0778
// Deleting destructor: sets vtable lbl_8035DE40, runs fn_800B0B88 and base dtor fn_80062CE0, frees if del > 0.
u8* fn_800B0778(u8* p, s32 del) {
    if (p) {
        *(u8**)(p + 0x0) = lbl_8035DE40;
        fn_800B0B88(p);
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800B4284
// Runs fn_800704AC on the object, then applies delta to each non-null child pointer at +0x98 and +0x9C.
void fn_800B4284(u8* p, f32 delta) {
    fn_800704AC(p);
    for (s32 i = 0; i < 2; i++) {
        u8* child = *(u8**)(p + 0x98 + i * 4);
        if (child) fn_800B82E0(child, delta);
    }
}

// 0x800B665C
// Sets the current value at +0x2B8 to v, shifting the old one into +0x2BC. When v differs from the
// previous value at +0x2BC, sends event 0x6A07F7F3 to the event manager.
void fn_800B665C(u8* p, s32 v) {
    if (*(s32*)(p + 0x2bc) != v) {
        EventSend(gEventMgr, p, (void*)0, 0x6A07F7F3u, 0, 0, 1, 0);
    }
    *(s32*)(p + 0x2bc) = *(s32*)(p + 0x2b8);
    *(s32*)(p + 0x2b8) = v;
}

// 0x800B6850
// Classifies the two floats at +0x2CC and +0x2D0 against lbl_805FE17C: returns 0 if both reach it,
// 1 if only the first does, 2 if only the second does, 3 if neither does.
s32 fn_800B6850(u8* p) {
    if (*(f32*)(p + 0x2cc) >= lbl_805FE17C && *(f32*)(p + 0x2d0) >= lbl_805FE17C) return 0;
    if (*(f32*)(p + 0x2cc) >= lbl_805FE17C) return 1;
    if (*(f32*)(p + 0x2d0) >= lbl_805FE17C) return 2;
    return 3;
}

// 0x800B71B0
// Ray-test driven value at +0x2D0: adds scale * table[RaySceneTest hit] (table lbl_80346068), clamped above at lbl_805FE17C.
// The first float argument is forwarded to RaySceneTest in the original but is not used there.
void fn_800B71B0(u8* p, f32 unused, f32 scale) {
    s32 idx = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x2d0) + scale * lbl_80346068[idx];
    *(f32*)(p + 0x2d0) = v;
    if (v > lbl_805FE17C) *(f32*)(p + 0x2d0) = lbl_805FE17C;
}

// 0x800B7228
// Same as fn_800B71B0 but on the float at +0x2CC.
void fn_800B7228(u8* p, f32 unused, f32 scale) {
    s32 idx = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x2cc) + scale * lbl_80346068[idx];
    *(f32*)(p + 0x2cc) = v;
    if (v > lbl_805FE17C) *(f32*)(p + 0x2cc) = lbl_805FE17C;
}

// 0x800B72A0
// Ray-test driven value at +0x2D0: subtracts scale * table[RaySceneTest hit] (table lbl_80346078), floored at lbl_805FE178.
void fn_800B72A0(u8* p, f32 scale) {
    s32 idx = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x2d0) - scale * lbl_80346078[idx];
    *(f32*)(p + 0x2d0) = v;
    if (v < lbl_805FE178) *(f32*)(p + 0x2d0) = lbl_805FE178;
}

// 0x800B7310
// Same as fn_800B72A0 but on the float at +0x2CC.
void fn_800B7310(u8* p, f32 scale) {
    s32 idx = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x2cc) - scale * lbl_80346078[idx];
    *(f32*)(p + 0x2cc) = v;
    if (v < lbl_805FE178) *(f32*)(p + 0x2cc) = lbl_805FE178;
}

// 0x800B7858
// Arm wrestle arm status: state 0 -> 1, 1 -> 2, 2 -> 3, anything else -> 0 (state at +0x2B8).
s32 ArmWrestleArmGetStatus(void* a0) {
    s32 state = *(s32*)((u8*)a0 + 0x2b8);
    if (state == 0) return 1;
    if (state == 1) return 2;
    if (state == 2) return 3;
    return 0;
}

// 0x800B789C
// Arm wrestle idle animation index (0..2). Re-rolls the index with RandomU32 when the timer at +0x304
// exceeds lbl_805FE180 or no index is set yet (+0x308 == -1), and resets the timer to lbl_805FE178.
s32 ArmWrestleArmGetIdleAnimation(void* a0) {
    u8* p = (u8*)a0;
    if (!(*(f32*)(p + 0x304) > lbl_805FE180) && *(s32*)(p + 0x308) != -1) {
        return *(s32*)(p + 0x308);
    }
    *(f32*)(p + 0x304) = lbl_805FE178;
    *(s32*)(p + 0x308) = (s32)(RandomU32() % 3u);
    return *(s32*)(p + 0x308);
}

// 0x800B7934
// Constructor: base ctor fn_800690D4, then sets vtable lbl_8035E470 and the float at +0x264 to lbl_805FE1B8. Returns obj.
void* fn_800B7934(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0)   = lbl_8035E470;
    *(f32*)(p + 0x264) = lbl_805FE1B8;
    return p;
}

// 0x800B7978
// Deleting destructor with base dtor fn_80062CE0 (del 0) on the object, then free if del > 0.
u8* fn_800B7978(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800B7A08
// Constructor: base ctor fn_800690D4, then sets vtable lbl_8035E568 and clears four floats at +0x264, +0x274, +0x278, +0x27C to lbl_805FE1C0. Returns obj.
void* fn_800B7A08(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0)   = lbl_8035E568;
    *(f32*)(p + 0x264) = lbl_805FE1C0;
    *(f32*)(p + 0x274) = lbl_805FE1C0;
    *(f32*)(p + 0x278) = lbl_805FE1C0;
    *(f32*)(p + 0x27c) = lbl_805FE1C0;
    return p;
}

// 0x800B7B08
// Looks up a handle for the sub-block at +0x94 with fn_801EC880 and stores it at +0x26C, with a at +0x270.
void fn_800B7B08(u8* p, s32 a, u32 hash) {
    *(s32*)(p + 0x26c) = (s32)fn_801EC880(p + 0x94, hash);
    *(s32*)(p + 0x270) = a;
}

// 0x800B7B50
// If a handle is stored at +0x268, releases it via fn_801B50B0, sets flag 1 on the linked object's +0x20 word, and clears the handle.
void fn_800B7B50(u8* p) {
    u8* h = *(u8**)(p + 0x268);
    if (h) {
        fn_801B50B0(h, p);
        u8* obj = *(u8**)(p + 0x4);
        *(s32*)(obj + 0x20) |= 1;
        *(void**)(p + 0x268) = 0;
    }
}

// 0x800B7F14
// Deleting destructor: vtable lbl_8035E678, calls fn_800B8180 and base dtor fn_80062CE0 (del 0), frees if del > 0.
u8* fn_800B7F14(u8* p, s32 del) {
    if (p) {
        *(u8**)(p + 0x0) = lbl_8035E678;
        fn_800B8180(p);
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800B8CFC
// Deleting destructor with base dtor fn_80062CE0 (del 0) on the object, then free if del > 0.
u8* fn_800B8CFC(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800B9008
// Walks the linked list at +0xB0 (node: +0x4 item, +0x8 next). For each non-null item, calls fn_801B50B0
// and sets flag 1 on the item's linked object +0x20 word.
void fn_800B9008(u8* p) {
    u8* node = *(u8**)(p + 0xb0);
    while (node) {
        u8* item = *(u8**)(node + 0x4);
        node = *(u8**)(node + 0x8);
        if (item) {
            fn_801B50B0(p, item);
            u8* obj = *(u8**)(item + 0x4);
            *(s32*)(obj + 0x20) |= 1;
        }
    }
}

// 0x800B991C
// Stores flag at +0x2A8. Sets the fourth float (+0x44) of the sub-block at +0x90 to lbl_805FE20C when flag is
// non-zero, else lbl_805FE208.
void fn_800B991C(u8* p, u8 flag) {
    *(u8*)(p + 0x2a8) = flag;
    u8* sub = *(u8**)(p + 0x90);
    *(f32*)(sub + 0x44) = (flag != 0) ? lbl_805FE20C : lbl_805FE208;
}

// 0x800BB4E8
// Adds delta to the float at +0x8C; when the total reaches lbl_805FE25C it raises game-flow signal 0x88C6E941.
void fn_800BB4E8(u8* p, f32 delta) {
    f32 v = *(f32*)(p + 0x8c) + delta;
    *(f32*)(p + 0x8c) = v;
    if (!(v >= lbl_805FE25C)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

}  // extern "C"
