// Small functions in 0x800Exxxx.
#include "types.h"

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
u8*  fn_800DE5E4(u8* p, s32 del);                              // 0x800DE5E4
void  fn_800DEC28(u8* p, void* arg);                            // 0x800DEC28
void* fn_800DECD8(u8* p);                                       // 0x800DECD8
s32   RegisterScriptFunc(const char* name, s32 type, void* fn); // 0x801DC91C
void  CIWaypointPath_StaticSetPosition(void);                   // 0x800E2F88
void  CIWaypointPath_StaticSetOrientation(void);                // 0x800E2FA4
void  fn_800E2DF4(u8* p, f32 a, f32 b);
void  fn_800E2E48(u8* p, f32 a, f32 b);

extern const char lbl_8035FFB0[];   // 0x8035FFB0 (script name string)
extern const char lbl_8035FFD4[];   // 0x8035FFD4 (script name string)
extern u8 lbl_80360010[];           // 0x80360010 (vtable)
extern u8 lbl_803C4298[];           // 0x803C4298
extern s32 lbl_805FB1B0;            // 0x805FB1B0
extern s32 lbl_805FB1B4;            // 0x805FB1B4
extern f32 lbl_805FE8B0;            // 0x805FE8B0
extern f32 lbl_805FE958;            // 0x805FE958
extern f32 lbl_805FE95C;            // 0x805FE95C
extern f32 lbl_805FE978;            // 0x805FE978

// 0x800E0228
// Deleting destructor: base dtor on the subobject at +0xC, then free if del > 0.
u8* fn_800E0228(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E0BCC
// Resets the record: 8 floats to the default constant, flag byte, and three ints.
void fn_800E0BCC(u8* p) {
    f32 v = lbl_805FE8B0;
    *(f32*)(p + 0x0)  = v;
    *(f32*)(p + 0x4)  = v;
    *(f32*)(p + 0x8)  = v;
    *(f32*)(p + 0xc)  = v;
    *(f32*)(p + 0x10) = v;
    *(f32*)(p + 0x14) = v;
    *(f32*)(p + 0x18) = v;
    *(f32*)(p + 0x1c) = v;
    *(u8*)(p + 0x20)  = 1;
    *(s32*)(p + 0x24) = 4;
    *(s32*)(p + 0x28) = 0;
    *(s32*)(p + 0x2c) = 2;
}

// 0x800E0C28
// Clears the record: flag byte at +0 and nine floats from +4 to +0x24 to the default constant.
void fn_800E0C28(u8* p) {
    f32 v = lbl_805FE8B0;
    *(u8*)(p + 0x0)  = 0;
    *(f32*)(p + 0x4)  = v;
    *(f32*)(p + 0x8)  = v;
    *(f32*)(p + 0xc)  = v;
    *(f32*)(p + 0x10) = v;
    *(f32*)(p + 0x14) = v;
    *(f32*)(p + 0x18) = v;
    *(f32*)(p + 0x1c) = v;
    *(f32*)(p + 0x20) = v;
    *(f32*)(p + 0x24) = v;
}

// 0x800E2DF4
// Running weighted average at +0xC4 with weight at +0xC8; skipped when the new weight is in (A, B).
void fn_800E2DF4(u8* p, f32 f1, f32 f2) {
    f32 w = *(f32*)(p + 0xc8) + f2;
    if (w > lbl_805FE958 && w < lbl_805FE95C) return;
    f32 oldW = *(f32*)(p + 0xc8);
    *(f32*)(p + 0xc8) = w;
    *(f32*)(p + 0xc4) = (*(f32*)(p + 0xc4) * oldW + f1 * f2) / w;
}

// 0x800E2E48
// Same running weighted average as fn_800E2DF4, on +0xCC / +0xD0.
void fn_800E2E48(u8* p, f32 f1, f32 f2) {
    f32 w = *(f32*)(p + 0xd0) + f2;
    if (w > lbl_805FE958 && w < lbl_805FE95C) return;
    f32 oldW = *(f32*)(p + 0xd0);
    *(f32*)(p + 0xd0) = w;
    *(f32*)(p + 0xcc) = (*(f32*)(p + 0xcc) * oldW + f1 * f2) / w;
}

// 0x800E2EBC
// Registers the two CIWaypointPath script functions.
void fn_800E2EBC(void) {
    lbl_805FB1B0 = RegisterScriptFunc(lbl_8035FFB0, 0x90, (void*)CIWaypointPath_StaticSetPosition);
    lbl_805FB1B4 = RegisterScriptFunc(lbl_8035FFD4, 0x8a, (void*)CIWaypointPath_StaticSetOrientation);
}

// 0x800E2FE8
// Constructor: base init, then sets vtable, the arg at +8 and a default float at +0x44.
u8* fn_800E2FE8(u8* p, void* arg) {
    fn_800DEC28(p, arg);
    *(void**)(p + 0x8) = arg;
    *(u8**)(p + 0x0)   = lbl_80360010;
    *(f32*)(p + 0x44)  = lbl_805FE978;
    return p;
}

// 0x800E3280
// Deleting destructor: base dtor at +0xC, then free if del > 0.
u8* fn_800E3280(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E36CC
// Deleting destructor: base dtor at +0xC, then free if del > 0.
u8* fn_800E36CC(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E3750
// If the global's sub-object is present, feeds the average at +0x68 from this record's +0x58 / +0x44.
s32 fn_800E3750(u8* p) {
    if (fn_800DECD8(p)) {
        u8* avg = *(u8**)(lbl_803C4298 + 8) + 0x68;
        fn_800E2DF4(avg, *(f32*)(p + 0x58), *(f32*)(p + 0x44));
    }
    return 0;
}

// 0x800E37B4
// Deleting destructor: base dtor at +0xC, then free if del > 0.
u8* fn_800E37B4(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E3838
// Same as fn_800E3750, but feeds fn_800E2E48.
s32 fn_800E3838(u8* p) {
    if (fn_800DECD8(p)) {
        u8* avg = *(u8**)(lbl_803C4298 + 8) + 0x68;
        fn_800E2E48(avg, *(f32*)(p + 0x58), *(f32*)(p + 0x44));
    }
    return 0;
}

// 0x800E389C
// Deleting destructor: base dtor at +0xC, then free if del > 0.
u8* fn_800E389C(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E3C74
// Deleting destructor: base dtor at +0xC, then free if del > 0.
u8* fn_800E3C74(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E4268
// Deleting destructor: base dtor at +0xC, then free if del > 0.
u8* fn_800E4268(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}
}
