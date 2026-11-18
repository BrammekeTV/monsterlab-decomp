// Small functions in 0x8003xxxx.
#include "types.h"

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
void  fn_80022E10(void);                                        // 0x80022E10
void  fn_80022F28(void);                                        // 0x80022F28
void  fn_80021ECC(void);                                       // 0x80021ECC
void  fn_800223F0(u8* p, f32 v);                                // 0x800223F0
void* fn_80030368(u8* p);                                       // 0x80030368 (tail-called, returns through)
void  fn_80030554(u8* p, f32 f1);                               // 0x80030554
s32   fn_800304DC(u8* p, const f32* v);                         // 0x800304DC

extern u8  lbl_80357A98[];   // 0x80357A98 (vtable)
extern u8  lbl_80357BA8[];   // 0x80357BA8 (vtable)
extern u8  lbl_80357B70[];   // 0x80357B70 (vtable)
extern u8  lbl_80357B38[];   // 0x80357B38 (vtable)
extern u8  lbl_80357B00[];   // 0x80357B00 (vtable)
extern u8  lbl_80357AC8[];   // 0x80357AC8 (vtable)
extern f32 lbl_805FCCA0;     // 0x805FCCA0
extern f32 lbl_805FCCA4;     // 0x805FCCA4
extern f32 lbl_805FCCD0;     // 0x805FCCD0
extern f32 lbl_805FCCD4;     // 0x805FCCD4
extern f32 lbl_805FCCF8;     // 0x805FCCF8

// 0x800300E0
// Advances the counter at +0xF84; when it reaches 3 it resets to 0 and calls fn_80022E10.
void fn_800300E0(u8* p) {
    s32 c = *(s32*)(p + 0xf84);
    if (c >= 3) {
        fn_80022E10();
        *(s32*)(p + 0xf84) = 0;
    } else {
        *(s32*)(p + 0xf84) = c + 1;
    }
}

// 0x8003012C
// Clears +0xF84; advances the counter at +0xF80, and when it is 1 or more calls fn_80022F28 and resets it.
void fn_8003012C(u8* p) {
    s32 c = *(s32*)(p + 0xf80);
    *(s32*)(p + 0xf84) = 0;
    if (c >= 1) {
        fn_80022F28();
        *(s32*)(p + 0xf80) = 0;
    } else {
        *(s32*)(p + 0xf80) = c + 1;
    }
}

// 0x80030228
// Base init: sets vtable lbl_80357A98, clears fields, sets +0x10 = -1, and the floats at +0x18 and +0x28.
void fn_80030228(u8* p) {
    *(u8**)(p + 0x0)   = lbl_80357A98;
    *(s32*)(p + 0x10)  = -1;
    *(s32*)(p + 0x24)  = 0;
    *(s32*)(p + 0x4)   = 0;
    *(u8*)(p + 0x20)   = 0;
    *(s32*)(p + 0xc)   = 0;
    *(s32*)(p + 0x8)   = 0;
    *(s32*)(p + 0x1c)  = 0;
    *(s32*)(p + 0x14)  = 0;
    *(f32*)(p + 0x18)  = lbl_805FCCA0;
    *(f32*)(p + 0x28)  = lbl_805FCCA4;
}

// 0x80030270
// Frees p when del > 0; returns p.
u8* fn_80030270(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x80030604
// Base init, then sets vtable lbl_80357BA8 and type field +0x1C = 4.
u8* fn_80030604(u8* p) {
    fn_80030228(p);
    *(s32*)(p + 0x1c) = 4;
    *(u8**)(p + 0x0)  = lbl_80357BA8;
    return p;
}

// 0x80030648
// Deleting destructor: base dtor (fn_80030270 with 0), then free if del > 0.
u8* fn_80030648(u8* p, s32 del) {
    if (p) {
        fn_80030270(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800306A0
// Copies the 3-float vector and forwards it to fn_800304DC.
s32 fn_800306A0(u8* p, f32* v) {
    f32 tmp[3] = { v[0], v[1], v[2] };
    return fn_800304DC(p, tmp);
}

// 0x80030754
// Base init, then sets vtable lbl_80357B70 and type field +0x1C = 2.
u8* fn_80030754(u8* p) {
    fn_80030228(p);
    *(s32*)(p + 0x1c) = 2;
    *(u8**)(p + 0x0)  = lbl_80357B70;
    return p;
}

// 0x80030798
// Deleting destructor: base dtor (fn_80030270 with 0), then free if del > 0.
u8* fn_80030798(u8* p, s32 del) {
    if (p) {
        fn_80030270(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800307F0
// Copies the 3-float vector and forwards it to fn_800304DC.
s32 fn_800307F0(u8* p, f32* v) {
    f32 tmp[3] = { v[0], v[1], v[2] };
    return fn_800304DC(p, tmp);
}

// 0x800308A4
// Base init, then sets vtable lbl_80357B38 and type field +0x1C = 3.
u8* fn_800308A4(u8* p) {
    fn_80030228(p);
    *(s32*)(p + 0x1c) = 3;
    *(u8**)(p + 0x0)  = lbl_80357B38;
    return p;
}

// 0x800308E8
// Deleting destructor: base dtor (fn_80030270 with 0), then free if del > 0.
u8* fn_800308E8(u8* p, s32 del) {
    if (p) {
        fn_80030270(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80030940
// Copies the 3-float vector and forwards it to fn_800304DC.
s32 fn_80030940(u8* p, f32* v) {
    f32 tmp[3] = { v[0], v[1], v[2] };
    return fn_800304DC(p, tmp);
}

// 0x800309F4
// Base init, then sets vtable lbl_80357B00, type field +0x1C = 5, and +0x38 = 0.
u8* fn_800309F4(u8* p) {
    fn_80030228(p);
    *(u8**)(p + 0x0)  = lbl_80357B00;
    *(s32*)(p + 0x1c) = 5;
    *(s32*)(p + 0x38) = 0;
    return p;
}

// 0x80030A40
// Deleting destructor: base dtor (fn_80030270 with 0), then free if del > 0.
u8* fn_80030A40(u8* p, s32 del) {
    if (p) {
        fn_80030270(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80030B6C
// Base init, then sets vtable lbl_80357AC8, +0x3C = 1, +0x40 = 0, +0x1C = 1, +0x38 = 0, and float +0x28.
u8* fn_80030B6C(u8* p) {
    fn_80030228(p);
    *(s32*)(p + 0x3c) = 1;
    *(u8**)(p + 0x0)  = lbl_80357AC8;
    *(s32*)(p + 0x40) = 0;
    *(s32*)(p + 0x1c) = 1;
    *(s32*)(p + 0x38) = 0;
    *(f32*)(p + 0x28) = lbl_805FCCD0;
    return p;
}

// 0x80030BC8
// Deleting destructor: base dtor (fn_80030270 with 0), then free if del > 0.
u8* fn_80030BC8(u8* p, s32 del) {
    if (p) {
        fn_80030270(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80030EF4
// State machine on +0x38: 3 becomes 4; 0 or 5 tail-calls fn_80030368; otherwise returns the pointer at +0xC.
void* fn_80030EF4(u8* p) {
    s32 st = *(s32*)(p + 0x38);
    if (st == 3) {
        *(s32*)(p + 0x38) = 4;
    } else if (st == 0 || st == 5) {
        return fn_80030368(p);
    }
    return *(void**)(p + 0xc);
}

// 0x80031004
// Resets +0x38, calls fn_80030554 on the object, then sets three floats in the sub-object at +0x4.
void fn_80031004(u8* p, f32 f1) {
    fn_80030554(p, f1);
    *(s32*)(p + 0x38) = 0;
    u8* q = *(u8**)(p + 0x4);
    *(f32*)(q + 0x74) = lbl_805FCCF8;
    *(f32*)(q + 0x78) = lbl_805FCCF8;
    *(f32*)(q + 0x7c) = lbl_805FCCD4;
}

// 0x8003174C
// Calls fn_80021ECC, then fn_800223F0 with the signed integer at +0x98 converted to float.
void fn_8003174C(u8* p) {
    fn_80021ECC();
    f32 v = (f32)*(s32*)(p + 0x98);
    fn_800223F0(p, v);
}
}
