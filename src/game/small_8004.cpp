// Small functions in 0x8004xxxx.
#include "types.h"
#include "game/script_callbacks.h"

// Copied from their definitions (C++ linkage, so declared outside extern "C").
extern "C" int  SysSprintf(char* buf, const char* fmt, ...);               // src/engine/engine.cpp
extern "C" s32  PartDefCycleSameSlot(s32 def, BOOL backwards);             // 0x80213A6C

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
void  fn_801B40E4(u8* p);                                       // 0x801B40E4
u8* fn_8020F3D4(u8* arg0, s32 arg1);                               // 0x8020F3D4
u8* fn_8020F42C(u8* arg0, s32 arg1);                              // 0x8020F42C
void  fn_8020F23C(u8* p, s32 del);                              // 0x8020F23C
void  fn_8020F2B4(u8* p, s32 del);                              // 0x8020F2B4
void  fn_8020F228(u8* p, s32 a2);                               // 0x8020F228
void  fn_8020F498(u8* p, s32 a2);                               // 0x8020F498
u8* fn_8020F4F0(u8* p, s32 del);                              // 0x8020F4F0
void  fn_8014E3EC(s32 n);                                       // 0x8014E3EC
void  fn_801B3310(u8* p, u8* q);                                // 0x801B3310
u8*   fn_801B3308(void);                                        // 0x801B3308
void  fn_801B4308(u8*, s32);                                    // 0x801B4308

extern u8  lbl_8047C7D8[];   // 0x8047C7D8
extern u8  lbl_803585A0[];   // 0x803585A0 (vtable)
extern u8  lbl_803586D8[];   // 0x803586D8 (vtable)
extern u8  lbl_80358978[];   // 0x80358978 (vtable)
extern u8  lbl_805FB014[];   // 0x805FB014 (global object pointer)
extern u8* lbl_805FB078[];   // 0x805FB078 (pointer table)
extern u8* lbl_805FB080[];   // 0x805FB080 (pointer table)
extern u8* lbl_805FB088;     // 0x805FB088
extern const char lbl_805F85B4[];  // 0x805F85B4 (format string)
extern f32 lbl_805FD0F8;     // 0x805FD0F8
extern s32 lbl_805FB4C4;     // 0x805FB4C4
extern s32 lbl_805FB4CC;     // 0x805FB4CC
extern u8* lbl_805FB4D0;     // 0x805FB4D0

// 0x80040028
// Clears the flag byte at +0x88, calls a helper on a global, then clears the byte at +0x98.
void fn_80040028(u8* p) {
    *(u8*)(p + 0x88) = 0;
    fn_801B40E4(lbl_8047C7D8);
    *(u8*)(p + 0x98) = 0;
}

// 0x80040090
// Constructor: base init (args pass through), vtable at +0, float default at +0x4C.
u8* fn_80040090(u8* p, s32 a2) {
    fn_8020F3D4(p, a2);
    *(u8**)p = lbl_803585A0;
    *(f32*)(p + 0x4c) = lbl_805FD0F8;
    return p;
}

// 0x800400D4
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800400D4(u8* p, s32 del) {
    if (p) {
        fn_8020F42C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800407A8
// Copies two fields from q into p; the slot used depends on the flag byte at +0x58.
void fn_800407A8(u8* p, u8* q) {
    if (!q) return;
    if (*(u8*)(p + 0x58) != 0) {
        *(u8*)(p + 0x68) = 1;
        *(u8*)(p + 0x60) = 0;
        *(s32*)(p + 0x64) = *(s32*)(q + 0x2c);
        *(s32*)(p + 0x5c) = *(s32*)(q + 0x24);
    } else {
        *(u8*)(p + 0x58) = 1;
        *(u8*)(p + 0x50) = 0;
        *(s32*)(p + 0x54) = *(s32*)(q + 0x2c);
        *(s32*)(p + 0x4c) = *(s32*)(q + 0x24);
    }
}

// 0x80041880
// Deleting destructor with no base call: frees the object if del > 0.
u8* fn_80041880(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800418E0
// Constructor: base init with kind 2, vtable lbl_803586D8, stores arg at +0xC and clears +0xD.
u8* fn_800418E0(u8* p, s32 a2) {
    fn_8020F228(p, 2);
    *(u8**)p = lbl_803586D8;
    *(u8*)(p + 0xc) = (u8)a2;
    *(u8*)(p + 0xd) = 0;
    return p;
}

// 0x80041938
// Deleting destructor: base dtor (kind 0) on the object, then free if del > 0.
u8* fn_80041938(u8* p, s32 del) {
    if (p) {
        fn_8020F23C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80041B1C
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_80041B1C(u8* p, s32 del) {
    if (p) {
        fn_8020F2B4(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8004287C
// Clears two words and a byte on the object, hides the selection HUD, and sets state 2 on the global object.
void fn_8004287C(u8* p) {
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = 0;
    *(u8*)(p + 0x9) = 0;
    HideSelectionHUD();
    u8* g = *(u8**)lbl_805FB014;
    *(s32*)(g + 0x4f4) = 2;
}

// 0x800434C8
// Cycles the part definition stored at +0xC of the record at +0xD4 forward.
void fn_800434C8(u8* p) {
    u8* o = *(u8**)(p + 0xd4);
    *(s32*)(o + 0xc) = PartDefCycleSameSlot(*(s32*)(o + 0xc), 1);
}

// 0x80043500
// Cycles the part definition stored at +0xC of the record at +0xD4 backward.
void fn_80043500(u8* p) {
    u8* o = *(u8**)(p + 0xd4);
    *(s32*)(o + 0xc) = PartDefCycleSameSlot(*(s32*)(o + 0xc), 0);
}

// 0x80043910
// Table lookup: kind 1 indexes lbl_805FB080, kind 0 indexes lbl_805FB078, kind 2 returns lbl_805FB088, else null.
void* fn_80043910(void* a0, s32 idx, s32 kind) {
    (void)a0;
    if (kind == 1) return lbl_805FB080[idx];
    if (kind == 0) return lbl_805FB078[idx];
    if (kind == 2) return lbl_805FB088;
    return 0;
}

// 0x80044010
// Formats a string from the field at +4 of the record at +0xD4 and passes it to fn_801B4308.
void fn_80044010(u8* p) {
    char buf[0x40];
    u8* o = *(u8**)(p + 0xd4);
    SysSprintf(buf, lbl_805F85B4, *(s32*)(o + 4));
    fn_801B4308(p, (s32)(uintptr_t)buf);
}

// 0x80044098
// Copies two bytes from the record at +0xD4 into the entries at lbl_805FB078[idx] and lbl_805FB080[idx].
void fn_80044098(u8* p) {
    u8* o = *(u8**)(p + 0xd4);
    s32 idx = *(s32*)o;
    u8 b4 = *(u8*)(o + 4);
    u8 b5 = *(u8*)(o + 5);
    u8* e = lbl_805FB078[idx];
    *(u8*)(e + 0x45) = b4;
    *(u8*)(e + 0x46) = b5;
    e = lbl_805FB080[idx];
    *(u8*)(e + 0x45) = b4;
    *(u8*)(e + 0x46) = b5;
}

// 0x80044498
// Clears the flag byte at +0x44, copies the float at +0x4D0 to +0x4CC, then calls two helpers.
void fn_80044498(u8* p) {
    f32 v = *(f32*)(p + 0x4d0);
    *(u8*)(p + 0x44) = 0;
    *(f32*)(p + 0x4cc) = v;
    fn_801B3308();
    fn_801B3310(p, p + 0x48);
}

// 0x800444FC
// Constructor: base init (args pass through), clears the byte at +0x47, vtable lbl_80358978.
u8* fn_800444FC(u8* p, s32 a2) {
    fn_8020F498(p, a2);
    *(u8*)(p + 0x47) = 0;
    *(u8**)p = lbl_80358978;
    return p;
}

// 0x80044540
// Deleting destructor: base dtor (kind 0) on the object, then free if del > 0.
u8* fn_80044540(u8* p, s32 del) {
    if (p) {
        fn_8020F4F0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800445F4
// Deleting destructor with no base call: frees the object if del > 0.
u8* fn_800445F4(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80044634
// Carves a 32-byte aligned block from the pool (lbl_805FB4C4 offset, lbl_805FB4CC remaining, base lbl_805FB4D0).
u8* fn_80044634(s32 n) {
    s32 sz = (n + 31) & ~31;
    if (lbl_805FB4CC < sz) fn_8014E3EC(sz);
    s32 old = lbl_805FB4C4;
    lbl_805FB4C4 = old + sz;
    lbl_805FB4CC = lbl_805FB4CC - sz;
    return lbl_805FB4D0 + old;
}

// 0x80044C7C
// Deleting destructor with no base call: frees the object if del > 0.
u8* fn_80044C7C(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

}  // extern "C"
