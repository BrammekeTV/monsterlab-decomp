// Ctors and state setters, list walkers and small helpers in 0x800ADxxx-0x800CDxxx.
#include "types.h"

extern "C" {
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);   // 0x800690D4 (base ctor)
void  fn_800A7D88(void* obj, u32 hash);                 // 0x800A7D88
void  fn_800B1D98(u8* p);                               // 0x800B1D98
BOOL  fn_801B50B0(void* unused, u8* obj);               // 0x801B50B0
void fn_801B6F34(u8* p);                           // 0x801B6F34
void  fn_801F61AC(u8* p, s32 a);                        // 0x801F61AC
void  fn_801F94B0(u8* p, s32 v);                        // 0x801F94B0
void  fn_800C9500(u8* p, u8* item);                     // 0x800C9500
void  fn_800C69DC(u8* obj);                             // 0x800C69DC
s32   RaySceneTest(void* ray);                          // 0x80070458
void* memset(void* p, int c, unsigned long n);
}

extern u8  lbl_8035DC48[];          // 0x8035DC48 (vtable)
extern u8  lbl_8035DD48[];          // 0x8035DD48 (vtable)
extern u8  lbl_8035E378[];          // 0x8035E378 (vtable)
extern u8  lbl_8035E678[];          // 0x8035E678 (vtable)
extern u8  lbl_8035E778[];          // 0x8035E778 (vtable)
extern u8  lbl_8035E910[];          // 0x8035E910 (vtable)
extern u8  lbl_8035EA08[];          // 0x8035EA08 (vtable)
extern u8  lbl_8035EB08[];          // 0x8035EB08 (vtable)
extern u8  lbl_8035EC08[];          // 0x8035EC08 (vtable)
extern u8  lbl_8035EE00[];          // 0x8035EE00 (vtable)
extern u8  lbl_8035F378[];          // 0x8035F378 (vtable)
extern u8  lbl_8035F678[];          // 0x8035F678 (vtable)
extern u8  lbl_803C4298[];          // 0x803C4298 (level info block)
extern f32 lbl_80346BC0[];          // 0x80346BC0 (f32 table indexed by RaySceneTest)
extern f32 lbl_80346BD0[];          // 0x80346BD0 (f32 table indexed by RaySceneTest)

extern "C" {

// 0x800AD844
// Ctor: base ctor, then vtable, sentinel state and unit scale fields.
void* fn_800AD844(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035DC48;
    *(s32*)(p + 0x264) = -1;
    *(u32*)(p + 0x268) = 0xC2FDDCDCu;
    *(s32*)(p + 0x26C) = 0;
    *(f32*)(p + 0x270) = 1.0f;
    *(f32*)(p + 0x274) = 1.0f;
    *(f32*)(p + 0x278) = 1.0f;
    *(s32*)(p + 0x330) = 0;
    *(f32*)(p + 0x360) = 0.0f;
    *(u8*)(p + 0x364) = 0;
    *(f32*)(p + 0x368) = 0.0f;
    *(f32*)(p + 0x36C) = 0.0f;
    return arg0;
}

// 0x800AEE74
// Calls fn_800A7D88 on the object with the caller's hash, then sets the flag at +0x264.
void fn_800AEE74(u8* arg0, u32 arg1) {
    fn_800A7D88(arg0, arg1);
    *(s32*)(arg0 + 0x264) = 1;
}

// 0x800AF568
// Ctor: base ctor, then vtable and zeroed state (scale 1.0 at +0x27C).
void* fn_800AF568(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(f32*)(p + 0x264) = 0.0f;
    *(u8**)(p + 0x0) = lbl_8035DD48;
    *(f32*)(p + 0x268) = 0.0f;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 0.0f;
    *(f32*)(p + 0x27C) = 1.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(s32*)(p + 0x288) = 0;
    *(s32*)(p + 0x28C) = 0;
    *(s32*)(p + 0x290) = 0;
    *(s32*)(p + 0x294) = 0;
    *(s32*)(p + 0x298) = 0;
    *(s32*)(p + 0x2AC) = 0;
    *(s32*)(p + 0x29C) = 0;
    *(s32*)(p + 0x2B0) = 0;
    *(s32*)(p + 0x2A0) = 0;
    *(s32*)(p + 0x2B4) = 0;
    *(s32*)(p + 0x2A4) = 0;
    *(s32*)(p + 0x2B8) = 0;
    *(s32*)(p + 0x2A8) = 0;
    *(s32*)(p + 0x2BC) = 0;
    return arg0;
}

// 0x800B170C
// Adds table[RaySceneTest(obj)] / 100 to the value at +0x268, clamps it to [0, 1], then
// sends the event hash 0x3C15A048 and runs fn_800B1D98.
void fn_800B170C(u8* arg0) {
    s32 idx = RaySceneTest(arg0);
    f32 t = *(f32*)(arg0 + 0x268) + lbl_80346BC0[idx] / 100.0f;
    *(f32*)(arg0 + 0x268) = t;
    f32 v;
    if (t < 0.0f) {
        v = 0.0f;
    } else if (t > 1.0f) {
        v = 1.0f;
    } else {
        v = t;
    }
    *(f32*)(arg0 + 0x268) = v;
    fn_800A7D88(arg0, 0x3C15A048u);
    fn_800B1D98(arg0);
}

// 0x800B179C
// Same as fn_800B170C with table lbl_80346BD0 and event hash 0xC8F251D4.
void fn_800B179C(u8* arg0) {
    s32 idx = RaySceneTest(arg0);
    f32 t = *(f32*)(arg0 + 0x268) + lbl_80346BD0[idx] / 100.0f;
    *(f32*)(arg0 + 0x268) = t;
    f32 v;
    if (t < 0.0f) {
        v = 0.0f;
    } else if (t > 1.0f) {
        v = 1.0f;
    } else {
        v = t;
    }
    *(f32*)(arg0 + 0x268) = v;
    fn_800A7D88(arg0, 0xC8F251D4u);
    fn_800B1D98(arg0);
}

// 0x800B5E64
// Ctor: base ctor, then vtable and default state (timers at -1, scale 1.0 at +0x288).
void* fn_800B5E64(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035E378;
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(f32*)(p + 0x288) = 1.0f;
    *(f32*)(p + 0x29C) = 0.0f;
    *(f32*)(p + 0x2A0) = 0.0f;
    *(f32*)(p + 0x2A4) = 0.0f;
    *(f32*)(p + 0x2A8) = 0.0f;
    *(f32*)(p + 0x2AC) = 0.0f;
    *(s32*)(p + 0x2B0) = 0;
    *(s32*)(p + 0x2B4) = 0;
    *(s32*)(p + 0x2B8) = -1;
    *(s32*)(p + 0x2C0) = -1;
    *(f32*)(p + 0x2C4) = 0.0f;
    *(f32*)(p + 0x2C8) = 0.0f;
    *(f32*)(p + 0x2CC) = 0.0f;
    *(f32*)(p + 0x2D0) = 0.0f;
    *(f32*)(p + 0x2D4) = 0.0f;
    *(f32*)(p + 0x2D8) = 0.0f;
    *(f32*)(p + 0x2F0) = 0.0f;
    *(f32*)(p + 0x2F4) = 0.0f;
    *(s32*)(p + 0x2F8) = 0;
    *(s32*)(p + 0x2FC) = 0;
    *(s32*)(p + 0x300) = 0;
    return arg0;
}

// 0x800B7E34
// Ctor: base ctor, then vtable, flag bytes at +0x264, +0x2C4, +0x2E8 and default state.
void* fn_800B7E34(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8*)(p + 0x264) = 1;
    *(u8**)(p + 0x0) = lbl_8035E678;
    *(f32*)(p + 0x268) = 0.0f;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 1.0f;
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(f32*)(p + 0x288) = 1.0f;
    *(s32*)(p + 0x28C) = 0;
    *(s32*)(p + 0x290) = -1;
    *(s32*)(p + 0x294) = -1;
    *(s32*)(p + 0x298) = -1;
    *(f32*)(p + 0x29C) = 0.0f;
    *(f32*)(p + 0x2A0) = 0.0f;
    *(f32*)(p + 0x2A8) = 0.0f;
    *(f32*)(p + 0x2AC) = 0.0f;
    *(f32*)(p + 0x2B0) = 0.0f;
    *(f32*)(p + 0x2B4) = 0.0f;
    *(f32*)(p + 0x2B8) = 0.0f;
    *(s32*)(p + 0x2BC) = -1;
    *(s32*)(p + 0x2C0) = -1;
    *(u8*)(p + 0x2C4) = 0;
    *(s32*)(p + 0x2E0) = 0;
    *(s32*)(p + 0x2E4) = 1;
    *(u8*)(p + 0x2E8) = 0;
    *(f32*)(p + 0x2EC) = 0.0f;
    return arg0;
}

// 0x800B8180
// Clears the pending item at +0x28C (sets its object flag), then walks the list at +0xB0
// and sets the flag bit 0x1 on each item's object.
void fn_800B8180(u8* arg0) {
    if (*(void**)(arg0 + 0x28C) != NULL) {
        fn_801B50B0(*(void**)(arg0 + 0x28C), arg0);
        u8* sub = *(u8**)(arg0 + 0x4);
        *(u32*)(sub + 0x20) |= 0x1u;
        *(void**)(arg0 + 0x28C) = NULL;
    }
    u8* node = *(u8**)(arg0 + 0xB0);
    while (node != NULL) {
        u8* item = *(u8**)(node + 0x4);
        u8* next = *(u8**)(node + 0x8);
        if (item != NULL) {
            fn_801B50B0(arg0, item);
            u8* sub = *(u8**)(item + 0x4);
            *(u32*)(sub + 0x20) |= 0x1u;
        }
        node = next;
    }
}

// 0x800B8C54
// Ctor: base ctor, then vtable and default state (flags at +0x2A8, +0x2A9 set to 1).
void* fn_800B8C54(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035E778;
    *(s32*)(p + 0x278) = 0;
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(f32*)(p + 0x288) = 1.0f;
    *(f32*)(p + 0x28C) = 0.0f;
    *(f32*)(p + 0x290) = 0.0f;
    *(f32*)(p + 0x294) = 0.0f;
    *(f32*)(p + 0x298) = 1.0f;
    *(f32*)(p + 0x29C) = 0.0f;
    *(f32*)(p + 0x2A0) = 0.0f;
    *(f32*)(p + 0x2A4) = 0.0f;
    *(u8*)(p + 0x2A8) = 1;
    *(u8*)(p + 0x2A9) = 1;
    *(f32*)(p + 0x2AC) = 1.0f;
    return arg0;
}

// 0x800BC99C
// Ctor: base ctor, then vtable and two zeroed slots.
void* fn_800BC99C(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(s32*)(p + 0x29C) = 0;
    *(u8**)(p + 0x0) = lbl_8035E910;
    *(s32*)(p + 0x2A0) = 0;
    return arg0;
}

// 0x800BD894
// Ctor: base ctor, then vtable, flag byte 0 at +0x264, index -1 at +0x268, 0.0 at +0x26C.
void* fn_800BD894(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035EA08;
    *(u8*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = -1;
    *(f32*)(p + 0x26C) = 0.0f;
    return arg0;
}

// 0x800BE308
// Ctor: base ctor, then vtable.
void* fn_800BE308(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035EB08;
    return arg0;
}

// 0x800BF18C
// Sets the flag at +0x290, places the sub-object at (0, 0, -100) and sets its flag bits
// (0x1000C000), then returns fn_801B6F34(obj).
s32 fn_800BF18C(u8* arg0) {
    *(s32*)(arg0 + 0x290) = 1;
    u8* sub = *(u8**)(arg0 + 0x4);
    *(f32*)(sub + 0x0) = 0.0f;
    *(f32*)(sub + 0x4) = 0.0f;
    *(f32*)(sub + 0x8) = -100.0f;
    sub = *(u8**)(arg0 + 0x4);
    *(u32*)(sub + 0x20) |= 0x1000C000u;
    fn_801B6F34(arg0); return 0;  // r3 is left over from the callee
}

// 0x800BF654
// Ctor: base ctor, then vtable and two 50.0 slots at +0x264 and +0x268.
void* fn_800BF654(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035EC08;
    *(f32*)(p + 0x264) = 50.0f;
    *(f32*)(p + 0x268) = 50.0f;
    return arg0;
}

// 0x800C2670
// Sets the vtable at +0xD4 and clears the 20-entry table at +0x80 (4 bytes each), then
// points +0x78 at that table and sets the count and index fields.
void fn_800C2670(u8* arg0) {
    *(s32*)(arg0 + 0x0) = 0;
    *(u8**)(arg0 + 0xD4) = lbl_8035EE00;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0x60) = 0;
    u8* q = arg0 + 0x80;
    for (s32 i = 0; i < 20; i++) {
        memset(q, 0, 4);
        q += 4;
    }
    *(u8**)(arg0 + 0x78) = arg0 + 0x80;
    *(s32*)(arg0 + 0x74) = 20;
    *(s32*)(arg0 + 0x70) = -1;
    *(s32*)(arg0 + 0x6C) = -1;
    *(s32*)(arg0 + 0x68) = 0;
    *(s32*)(arg0 + 0xD0) = 8;
}

// 0x800C72C4
// If the item at +0x8 is set, clears it, passes it with this object to fn_800C9500, sets
// the flag at +0x10, places the sub-object at (sub+0xC8, sub+0xCC - 100, sub+0xD0) and sets
// its flag bits (0x1000C000). Then calls fn_800C69DC if the mode at +0xC is 2.
void fn_800C72C4(u8* arg0, u8* arg1) {  // arg1 is passed by callers but unused by the asm
    u8* item = *(u8**)(arg0 + 0x8);
    if (item != NULL) {
        *(u8**)(arg0 + 0x8) = NULL;
        fn_800C9500(item, arg0);
        *(s32*)(arg0 + 0x10) = 1;
        u8* sub = *(u8**)(arg0 + 0x4);
        f32 x = *(f32*)(sub + 0xC8) + 0.0f;
        f32 y = *(f32*)(sub + 0xCC) + -100.0f;
        f32 z = *(f32*)(sub + 0xD0) + 0.0f;
        u8* inner = *(u8**)(sub + 0x4);
        *(f32*)(inner + 0x0) = x;
        *(f32*)(inner + 0x4) = y;
        *(f32*)(inner + 0x8) = z;
        inner = *(u8**)(sub + 0x4);
        *(u32*)(inner + 0x20) |= 0x1000C000u;
        if (*(s32*)(arg0 + 0xC) == 2) {
            fn_800C69DC(*(u8**)(arg0 + 0x4));
        }
    }
}

// 0x800CC29C
// Clears the counters at +0x264..+0x26C, sets -1 at +0x270 and +0x274, then walks the list
// at +0xB0 and sets the flag bit 0x1 on each item's object (as fn_800B8180 does).
void fn_800CC29C(u8* arg0) {
    *(s32*)(arg0 + 0x264) = 0;
    u8* node = *(u8**)(arg0 + 0xB0);
    *(s32*)(arg0 + 0x268) = 0;
    *(s32*)(arg0 + 0x26C) = 0;
    *(s32*)(arg0 + 0x270) = -1;
    *(s32*)(arg0 + 0x274) = -1;
    while (node != NULL) {
        u8* item = *(u8**)(node + 0x4);
        u8* next = *(u8**)(node + 0x8);
        if (item != NULL) {
            fn_801B50B0(arg0, item);
            u8* sub = *(u8**)(item + 0x4);
            *(u32*)(sub + 0x20) |= 0x1u;
        }
        node = next;
    }
}

// 0x800CC71C
// Ctor: base ctor, then vtable, flag byte 0 at +0x264, zeroed floats, and the sub-objects
// set up from the level info block (lbl_803C4298 + 0x38) by fn_801F61AC and fn_801F94B0.
void* fn_800CC71C(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035F378;
    *(u8*)(p + 0x264) = 0;
    *(f32*)(p + 0x268) = 0.0f;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    fn_801F61AC(p + 0x278, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F94B0(p + 0x334, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0x7C8) = 0;
    *(f32*)(p + 0x7B8) = 0.0f;
    *(f32*)(p + 0x7BC) = 0.0f;
    *(f32*)(p + 0x7C0) = 0.0f;
    *(f32*)(p + 0x7CC) = 0.0f;
    *(f32*)(p + 0x7D0) = 0.0f;
    return arg0;
}

// 0x800CD8EC
// Ctor: base ctor, then vtable and three zeroed floats at +0x264..+0x26C.
void* fn_800CD8EC(void* arg0, s32 arg1, void* arg2, s32 arg3) {
    u8* p = (u8*)arg0;
    fn_800690D4(p, arg1, arg2, arg3);
    *(u8**)(p + 0x0) = lbl_8035F678;
    *(f32*)(p + 0x264) = 0.0f;
    *(f32*)(p + 0x268) = 0.0f;
    *(f32*)(p + 0x26C) = 0.0f;
    return arg0;
}

}
