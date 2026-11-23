// Small functions in 0x800Cxxxx.
#include "types.h"

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);                                       // 0x800690D4 (base ctor)
u8*  fn_80062CE0(u8* p, s32 del);                              // 0x80062CE0 (base dtor)
void  fn_800C2670(u8* p);                                       // 0x800C2670
void  fn_800C2760(u8* p, u8* owner);                            // 0x800C2760
void  fn_800C28C4(u8* p);                                       // 0x800C28C4
void  fn_800C1798(void* out, void* arg);                        // 0x800C1798
void  fn_8010BB64(u8* obj, s32 v, void* tmp);                   // 0x8010BB64
void  fn_8010B4AC(u8* a3);                                      // 0x8010B4AC
void  fn_8010BB54(u8* a3);                                      // 0x8010BB54
}

extern u8 lbl_8035ED2C[];   // 0x8035ED2C (vtable)
extern u8 lbl_8035EE40[];   // 0x8035EE40 (vtable)
extern u8 lbl_8035EF18[];   // 0x8035EF18 (vtable)
extern u8 lbl_8035EF58[];   // 0x8035EF58 (vtable)
extern u8 lbl_8035F038[];   // 0x8035F038 (vtable)
extern u8 lbl_8035F078[];   // 0x8035F078 (vtable)
extern u8 lbl_8035F148[];   // 0x8035F148 (vtable)
extern f32 lbl_805FE3E0;    // 0x805FE3E0
extern f32 lbl_805FE3F0;    // 0x805FE3F0

// 0x800C0B54
// Zeroes 18 words at offsets 0x00..0x44 of the record.
void fn_800C0B54(u8* p) {
    *(s32*)(p + 0x00) = 0;
    *(s32*)(p + 0x04) = 0;
    *(s32*)(p + 0x08) = 0;
    *(s32*)(p + 0x0c) = 0;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = 0;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1c) = 0;
    *(s32*)(p + 0x20) = 0;
    *(s32*)(p + 0x24) = 0;
    *(s32*)(p + 0x28) = 0;
    *(s32*)(p + 0x2c) = 0;
    *(s32*)(p + 0x30) = 0;
    *(s32*)(p + 0x34) = 0;
    *(s32*)(p + 0x38) = 0;
    *(s32*)(p + 0x3c) = 0;
    *(s32*)(p + 0x40) = 0;
    *(s32*)(p + 0x44) = 0;
}

// 0x800C11A4
// Cross product: out = a x b (3-component f32 vectors).
extern "C" void fn_800C11A4(f32* out, const f32* a, const f32* b) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

// 0x800C13D8
// Constructor: base init, then sets vtable and clears the trailing fields.
u8* fn_800C13D8(u8* p, s32 a, void* params, s32 b) {
    fn_800690D4(p, a, params, b);
    *(s32*)(p + 0x264) = -1;
    *(u8**)(p + 0x0)   = lbl_8035ED2C;
    *(s32*)(p + 0x268) = -1;
    *(u8*)(p + 0x26c)  = 0;
    *(u8*)(p + 0x26d)  = 0;
    return p;
}

// 0x800C142C
// Deleting destructor: base dtor on p, then free if del > 0.
u8* fn_800C142C(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C2720
// Deleting wrapper: frees p if del > 0 (no base dtor call).
u8* fn_800C2720(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x800C407C
// Constructor: base init, sets vtable and clears the field at +0x2A0.
extern "C" u8* fn_800C407C(u8* p, s32 a, void* params, s32 b) {
    fn_800C13D8(p, a, params, b);
    *(u8**)(p + 0x0)   = lbl_8035EE40;
    *(s32*)(p + 0x2a0) = 0;
    return p;
}

// 0x800C40C0
// Deleting destructor: base dtor (fn_800C142C) on p, then free if del > 0.
extern "C" u8* fn_800C40C0(u8* p, s32 del) {
    if (p) {
        fn_800C142C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C42B8
// Constructor: base init, sets vtable at +0 and then calls the sub-object init at +0xD8.
u8* fn_800C42B8(u8* p) {
    fn_800C2670(p);
    *(u8**)(p + 0xd4) = lbl_8035EF18;
    fn_8010B4AC(p + 0xd8);
    return p;
}

// 0x800C444C
// Calls base cleanup, then the sub-object cleanup at +0xD8.
void fn_800C444C(u8* p) {
    fn_800C28C4(p);
    fn_8010BB54(p + 0xd8);
}

// 0x800C44C0
// Reads the field at +0x2A0 of src, builds a temp from arg, and forwards both to the sub-object at +0xD8.
void fn_800C44C0(u8* p, u8* src, void* arg) {
    u8 tmp[8];
    s32 v = *(s32*)(src + 0x2a0);
    fn_800C1798(tmp, arg);
    fn_8010BB64(p + 0xd8, v, tmp);
}

// 0x800C4530
// Constructor: base init, then sets vtable.
extern "C" u8* fn_800C4530(u8* p, s32 a, void* params, s32 b) {
    fn_800C13D8(p, a, params, b);
    *(u8**)(p + 0x0) = lbl_8035EF58;
    return p;
}

// 0x800C456C
// Deleting destructor: base dtor (fn_800C142C) on p, then free if del > 0.
extern "C" u8* fn_800C456C(u8* p, s32 del) {
    if (p) {
        fn_800C142C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C52B4
// Constructor: base init (fn_800C2670), then sets vtable at +0xD4.
extern "C" u8* fn_800C52B4(u8* p) {
    fn_800C2670(p);
    *(u8**)(p + 0xd4) = lbl_8035F038;
    return p;
}

// 0x800C52F0
// Deleting destructor: base dtor (fn_800C2720) on p, then free if del > 0.
extern "C" u8* fn_800C52F0(u8* p, s32 del) {
    if (p) {
        fn_800C2720(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C5348
// Init: base init (fn_800C2760), then sets float at +0xD8 to the default constant.
void fn_800C5348(u8* p, u8* owner) {
    fn_800C2760(p, owner);
    *(f32*)(p + 0xd8) = lbl_805FE3E0;
}

// 0x800C537C
// Init: base init (fn_800C28C4), then sets float at +0xD8 to the default constant.
void fn_800C537C(u8* p) {
    fn_800C28C4(p);
    *(f32*)(p + 0xd8) = lbl_805FE3E0;
}

// 0x800C5458
// Calls virtual slot 0xD0 on obj with v, then subtracts v from the float at +0xD8.
void fn_800C5458(u8* p, u8* obj, f32 v) {
    u8* vt = *(u8**)obj;
    void (*vfn)(u8*, f32) = *(void (**)(u8*, f32))(vt + 0xd0);
    vfn(obj, v);
    *(f32*)(p + 0xd8) = *(f32*)(p + 0xd8) - v;
}

// 0x800C5E90
// Constructor: base init, sets float at +0x264, vtable, and clears flags at +0x268/+0x26C/+0x270.
u8* fn_800C5E90(u8* p, s32 a, void* params, s32 b) {
    fn_800690D4(p, a, params, b);
    *(f32*)(p + 0x264) = lbl_805FE3F0;
    *(u8**)(p + 0x0)   = lbl_8035F078;
    *(u8*)(p + 0x268)  = 0;
    *(s32*)(p + 0x26c) = 0;
    *(u8*)(p + 0x270)  = 0;
    return p;
}

// 0x800C5EE4
// Deleting destructor: base dtor on p, then free if del > 0.
u8* fn_800C5EE4(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C5F84
// Constructor: base init, sets vtable and the int at +0x268 to -1.
extern "C" u8* fn_800C5F84(u8* p, s32 a, void* params, s32 b) {
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0)   = lbl_8035F148;
    *(s32*)(p + 0x268) = -1;
    return p;
}
