// Small functions in 0x801Bxxxx.
#include "types.h"

extern "C" {
void  operator_delete(void* p);                                 // 0x801D4980
void  BufferFree(void* p);                                      // 0x801D4980
void  WPADControlMotor(s32 chan, u32 command);                  // 0x80272E3C
void  fn_80234418(void* p);                                     // 0x80234418
void  fn_801EB748(void* p);                                     // 0x801EB748
void  fn_801B3430(u8* a3, s32 a4);                              // 0x801B3430
void  fn_801B3940(void* p);                                     // 0x801B3940
void  fn_801B7524(void* p);                                     // 0x801B7524

extern u8  lbl_80383B60[];      // 0x80383B60 (vtable)
extern u8  lbl_80383C90[];      // 0x80383C90 (vtable)
extern u8  lbl_80383CC0[];      // 0x80383CC0 (vtable)
extern f32 lbl_805FFAC4;        // 0x805FFAC4
extern f32 lbl_805FFB10;        // 0x805FFB10
extern s32 lbl_805FB7B0;        // 0x805FB7B0
extern s32 lbl_805FB7B4;        // 0x805FB7B4
extern s32 lbl_805FB7B8;        // 0x805FB7B8
extern s32 lbl_805FB7BC;        // 0x805FB7BC
extern s32 lbl_805FB7C0;        // 0x805FB7C0
extern s32 lbl_805FB7C4;        // 0x805FB7C4
extern s32 lbl_805FB7C8;        // 0x805FB7C8
extern s32 lbl_805FB7CC;        // 0x805FB7CC


// 0x801B161C
// Linear search of the pointer array at +0x24 (count at +0x28) for an element whose key at +0x2C matches.
u8* fn_801B161C(u8* p, u32 key) {
    s32 count = *(s32*)(p + 0x28);
    for (s32 i = 0; i < count; i++) {
        u8* arr = *(u8**)(p + 0x24);
        u8* elem = *(u8**)(arr + i * 4);
        if (key == *(u32*)(elem + 0x2c)) return *(u8**)(arr + i * 4);
    }
    return NULL;
}

// 0x801B1724
// Linear search of the pointer array at +0xC (count at +0x10) for a non-null element whose u32 at +8 matches.
u8* fn_801B1724(u8* p, u32 key) {
    s32 count = *(s32*)(p + 0x10);
    for (s32 i = 0; i < count; i++) {
        u8* arr = *(u8**)(p + 0xc);
        u8* elem = *(u8**)(arr + i * 4);
        if (elem != NULL && key == *(u32*)(elem + 0x8)) return *(u8**)(arr + i * 4);
    }
    return NULL;
}

// 0x801B1988
// Sets the float at +0x18 of every non-null element of the pointer array at +0xC (count at +0x10) to a constant.
void fn_801B1988(u8* p) {
    f32 v = lbl_805FFAC4;
    s32 count = *(s32*)(p + 0x10);
    for (s32 i = 0; i < count; i++) {
        u8* elem = *(u8**)(*(u8**)(p + 0xc) + i * 4);
        if (elem != NULL) *(f32*)(elem + 0x18) = v;
    }
}

// 0x801B2488
// Deleting destructor: sets the vtable, then frees if del > 0.
u8* fn_801B2488(u8* p, s32 del) {
    if (p) {
        *(u8**)p = lbl_80383B60;
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B24DC
// Deleting destructor: sets the vtable, then frees if del > 0.
u8* fn_801B24DC(u8* p, s32 del) {
    if (p) {
        *(u8**)p = lbl_80383B60;
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B2530
// Deleting destructor: sets the vtable, then frees if del > 0.
u8* fn_801B2530(u8* p, s32 del) {
    if (p) {
        *(u8**)p = lbl_80383B60;
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B2584
// Deleting destructor with no vtable store: frees if del > 0.
u8* fn_801B2584(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B2ED0
// Deleting destructor with no vtable store: frees if del > 0.
u8* fn_801B2ED0(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B2FAC
// Stops the controller motor of the channel at +4, clears the flag byte at +0x48, and calls fn_80234418 on +0x18.
void fn_801B2FAC(u8* p) {
    WPADControlMotor(*(s32*)(p + 0x4), 0);
    *(u8*)(p + 0x48) = 0;
    fn_80234418(p + 0x18);
}

// 0x801B33F4
// Returns TRUE if the key is present in the u32 array at +4 (count at +0).
BOOL fn_801B33F4(u8* p, u32 key) {
    s32 count = *(s32*)(p + 0x0);
    for (s32 i = 0; i < count; i++) {
        if (*(u32*)(*(u8**)(p + 0x4) + i * 4) == key) return 1;
    }
    return 0;
}

// 0x801B3454
// Base destructor: frees if del > 0.
u8* fn_801B3454(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B35C8
// Calls the base constructor fn_801B3430, then sets the vtable.
u8* fn_801B35C8(u8* p, s32 a4) {
    fn_801B3430(p, a4);
    *(u8**)p = lbl_80383C90;
    return p;
}

// 0x801B3604
// Deleting destructor: calls base dtor fn_801B3454 with 0, then frees if del > 0.
u8* fn_801B3604(u8* p, s32 del) {
    if (p) {
        fn_801B3454(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B365C
// Calls the base constructor fn_801B3430, then sets the vtable, clears the flag at +0x90 and sets the float at +0x94.
u8* fn_801B365C(u8* p, s32 a4) {
    fn_801B3430(p, a4);
    f32 v = lbl_805FFB10;
    *(u8**)p = lbl_80383CC0;
    *(u8*)(p + 0x90) = 0;
    *(f32*)(p + 0x94) = v;
    return p;
}

// 0x801B36A8
// Deleting destructor: calls base dtor fn_801B3454 with 0, then frees if del > 0.
u8* fn_801B36A8(u8* p, s32 del) {
    if (p) {
        fn_801B3454(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B3700
// Clears the flag at +0x90. If the float at +8 is >= 0, subtracts f1 from it; otherwise tail-calls vtable slot 0x10.
void fn_801B3700(u8* p, f32 f1) {
    typedef void (*VFn)(u8*, f32);
    f32 v = *(f32*)(p + 0x8);
    *(u8*)(p + 0x90) = 0;
    if (!(v >= lbl_805FFB10)) {
        VFn fn = *(VFn*)(*(u8**)p + 0x10);
        fn(p, f1);
        return;
    }
    *(f32*)(p + 0x8) = v - f1;
}

// 0x801B37C4
// Deleting destructor: calls fn_801EB748 on the pointer at +0x60, then frees if del > 0.
u8* fn_801B37C4(u8* p, s32 del) {
    if (p) {
        fn_801EB748(*(void**)(p + 0x60));
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B42B4
// Calls fn_801B3940 on the first element of the object at +4 (if any), then clears eight global words.
void fn_801B42B4(void* p) {
    u8* q = *(u8**)((u8*)p + 0x4);
    if (q != NULL) fn_801B3940(*(void**)q);
    lbl_805FB7B4 = 0;
    lbl_805FB7B0 = 0;
    lbl_805FB7BC = 0;
    lbl_805FB7B8 = 0;
    lbl_805FB7C4 = 0;
    lbl_805FB7C0 = 0;
    lbl_805FB7CC = 0;
    lbl_805FB7C8 = 0;
}

// 0x801B50B0
// If the buffer pointer at +0xB4 of the second argument is set, frees it and returns TRUE.
BOOL fn_801B50B0(void* unused, u8* obj) {
    void* q = *(void**)(obj + 0xb4);
    if (q == NULL) return 0;
    fn_801B7524(q);
    BufferFree(q);
    return 1;
}
}
