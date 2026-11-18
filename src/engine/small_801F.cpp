// Small engine functions in 0x801Fxxxx.
#include "types.h"

typedef void (*VFn)(void*);
typedef void (*VFnP)(void*, void*);
#define VT(obj, off) (*(void**)(*(u8**)(obj) + (off)))

extern "C" {
void    operator_delete(void* p);                                    // 0x801D426C
u8*     fn_801B3604(u8* p, s32 del);                                 // 0x801B3604 (deleting dtor)
void    fn_80146F50(u8* p, s32 a);                                   // 0x80146F50 (base dtor)
void    fn_80146FD8(u8* p, s32 a);                                   // 0x80146FD8 (base dtor)
void    fn_80147150(u8* p, s32 a);                                   // 0x80147150 (base dtor)
void    fn_801FA82C(u8* p, s32 a);                                   // 0x801FA82C (base dtor)
s32     fn_801F5294(s32 idx);                                        // 0x801F5294
void    fn_801F5E3C(void* a, const void* b, s32 c);                  // 0x801F5E3C (leaf in src/leaf, takes args here)
}

extern const char lbl_80385FD8[];   // 0x80385FD8 (string)
extern u8  lbl_804DD168[];          // 0x804DD168 (bss object, 0x18 bytes)
extern u8  gWorldState[];           // 0x803E6D00
extern void* lbl_805FB964;          // 0x805FB964 movie player
extern s32 lbl_805FB950;            // 0x805FB950
extern s32 lbl_805FB954;            // 0x805FB954
extern s32 lbl_805FB958;            // 0x805FB958
extern s32 lbl_805FB970;            // 0x805FB970 (movie player active flag)
extern s32 lbl_805FB97C;            // 0x805FB97C
extern f32 lbl_805F9820;            // 0x805F9820 (countdown float)
extern f32 lbl_806000C0;            // 0x806000C0 (float constant)

extern "C" {

// 0x801F2BCC
// Starts the setup call with the 0x18-byte object and the string, then clears three counters.
void fn_801F2BCC(void) {
    fn_801F5E3C(lbl_804DD168, lbl_80385FD8, 1);
    lbl_805FB950 = 0;
    lbl_805FB954 = 0;
    lbl_805FB958 = 0;
}

// 0x801F3388
// Deleting destructor with an empty base: frees the object if del > 0.
u8* fn_801F3388(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801F51DC
// Movie player step: vtable slot 0x20 when flag is set, slot 0x28 otherwise. Does nothing if no movie is active.
void fn_801F51DC(s32 flag) {
    if (lbl_805FB970 == 0) return;
    if (flag != 0) ((VFn)VT(lbl_805FB964, 0x20))(lbl_805FB964);
    else           ((VFn)VT(lbl_805FB964, 0x28))(lbl_805FB964);
}

// 0x801F5734
// Forwards arg to movie player vtable slot 0x30; clears lbl_805FB97C when arg is null.
void fn_801F5734(void* arg) {
    if (lbl_805FB964 != 0) ((VFnP)VT(lbl_805FB964, 0x30))(lbl_805FB964, arg);
    if (arg == 0) lbl_805FB97C = 0;
}

// 0x801F57E0
// Countdown: returns 1 while the timer is still at or above the constant. Otherwise subtracts dt and
// returns 1 once it has reached the constant, 0 while it is still above it.
s32 fn_801F57E0(f32 dt) {
    if (lbl_805F9820 >= lbl_806000C0) return 1;
    lbl_805F9820 = lbl_805F9820 - dt;
    return lbl_805F9820 <= lbl_806000C0;
}

// 0x801F59B0
// Deleting destructor with an empty base: frees the object if del > 0.
u8* fn_801F59B0(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801F5B44
// Computes a 0/1 flag from fn_801F5294(x) plus the word at +0x10 (sign bit of a bit-trick difference).
s32 fn_801F5B44(u8* p, s32 x) {
    u32 a = (u32)fn_801F5294(x) + (u32)*(s32*)(p + 0x10);
    u32 t = (0x400u - a) >> 1;
    return (s32)(((0x400u | ~a) - t) >> 31);
}

// 0x801F5D34
// Deleting destructor: base dtor fn_80146F50 with 0, then frees if del > 0.
u8* fn_801F5D34(u8* p, s32 del) {
    if (p) {
        fn_80146F50(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F5D8C
// Deleting destructor: base dtor fn_80146FD8 with 0, then frees if del > 0.
u8* fn_801F5D8C(u8* p, s32 del) {
    if (p) {
        fn_80146FD8(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F5DE4
// Deleting destructor: base dtor fn_80147150 with 0, then frees if del > 0.
u8* fn_801F5DE4(u8* p, s32 del) {
    if (p) {
        fn_80147150(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F5E50
// Copies the position from the pointer chain at gWorldState into out, adding dz to the third component.
void fn_801F5E50(u8* out, f32 dz) {
    u8* a = *(u8**)gWorldState;
    u8* b = *(u8**)(a + 0x8);
    u8* c = *(u8**)(b + 0x4);
    *(f32*)(out + 0x0) = *(f32*)(c + 0x0);
    *(f32*)(out + 0x4) = *(f32*)(c + 0x4);
    *(f32*)(out + 0x8) = dz + *(f32*)(c + 0x8);
}

// 0x801F6238
// Deleting destructor: base dtor fn_801B3604 with 0, then frees if del > 0.
u8* fn_801F6238(u8* p, s32 del) {
    if (p) {
        fn_801B3604(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F71E8
// Deleting destructor: base dtor fn_801B3604 with 0, then frees if del > 0.
u8* fn_801F71E8(u8* p, s32 del) {
    if (p) {
        fn_801B3604(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F69B8
// Deleting destructor: base dtor fn_801F71E8 with 0, then frees if del > 0.
u8* fn_801F69B8(u8* p, s32 del) {
    if (p) {
        fn_801F71E8(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F858C
// Deleting destructor: base dtor fn_801FA82C with 0, then frees if del > 0.
u8* fn_801F858C(u8* p, s32 del) {
    if (p) {
        fn_801FA82C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F81E0
// Deleting destructor: base dtor fn_801F858C with 0, then frees if del > 0.
u8* fn_801F81E0(u8* p, s32 del) {
    if (p) {
        fn_801F858C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F85E4
// Stores the count at +0x1E0, then copies count 3-float records from src into the table at +0xA8 (stride 0xC).
// The loop re-reads the count each pass, as the original does (the table can overlap the count).
void fn_801F85E4(u8* p, const u8* src, s32 count) {
    *(s32*)(p + 0x1e0) = count;
    u8* dst = p;
    s32 i = 0;
    while (i < *(s32*)(p + 0x1e0)) {
        f32 x = *(const f32*)(src + 0x0);
        f32 y = *(const f32*)(src + 0x4);
        i++;
        f32 z = *(const f32*)(src + 0x8);
        *(f32*)(dst + 0xa8) = x;
        *(f32*)(dst + 0xac) = y;
        *(f32*)(dst + 0xb0) = z;
        src += 0xc;
        dst += 0xc;
    }
}

// 0x801F900C
// Copies count floats (count at +0x1E0) from src into the float table at +0x2B4.
void fn_801F900C(u8* p, const u8* src) {
    u8* dst = p + 0x2b4;
    s32 i = 0;
    while (i < *(s32*)(p + 0x1e0)) {
        f32 v = *(const f32*)src;
        src += 4;
        i++;
        *(f32*)dst = v;
        dst += 4;
    }
}

// 0x801F94EC
// Deleting destructor: base dtor fn_801F858C with 0, then frees if del > 0.
u8* fn_801F94EC(u8* p, s32 del) {
    if (p) {
        fn_801F858C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

}  // extern "C"
