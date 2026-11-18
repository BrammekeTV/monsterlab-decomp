// Small functions in 0x800C8E54-0x800CD67C: vector helpers, object ctors/dtors,
// ring-buffer count, reticle quality tier, tagged-slot checks.
#include "types.h"
#include <math.h>

// The header declares the script callbacks with C linkage so the definition below matches.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                   // 0x801D426C
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);             // 0x800690D4 (base ctor)
void  fn_800C20FC(u8* p);                                         // 0x800C20FC (base ctor, sets vtable at +0xC)
void  fn_800C2388(u8* p);                                         // 0x800C2388 (base cleanup)
u8*   fn_80062CE0(u8* p, s32 del);                                // 0x80062CE0 (deleting dtor)
u8*   fn_801F94EC(u8* p, s32 del);                                // 0x801F94EC (deleting dtor)
u8*   fn_801F6238(u8* p, s32 del);                                // 0x801F6238 (deleting dtor)
u8*   fn_801B3308(void);                                          // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                // 0x801B3378
void  fn_800C954C(u8* p);                                         // 0x800C954C
void  fn_800C72B4(u8* item, u8* base);                            // 0x800C72B4
void  fn_800C72C4(u8* item, u8* other);                           // 0x800C72C4
void  fn_800CC29C(u8* p);                                         // 0x800CC29C
void  fn_8027D270(s32 a, f32 b, f32 c);                           // 0x8027D270
}

// Vtables and globals.
extern u8  lbl_8035F55C[];       // 0x8035F55C (vtable)
extern u8  lbl_8035F46C[];       // 0x8035F46C (vtable)
extern u8  lbl_8035F76C[];       // 0x8035F76C (vtable)
extern u32 lbl_8035F640[];       // 0x8035F640 (7 pairs of u32 tags)
extern u32 lbl_805F8C70;         // 0x805F8C70 (tag)
extern u32 lbl_805F8C80[];       // 0x805F8C80 (two tags: [0] at 805F8C80, [1] at 805F8C84)
extern f32 lbl_805FE4B8;         // 0x805FE4B8 (float constant)
extern f32 lbl_805FE510;         // 0x805FE510 (float constant)
extern f32 lbl_805FE514;         // 0x805FE514 (float constant)
extern f32 lbl_805FE51C;         // 0x805FE51C (quality threshold)
extern f32 lbl_805FE520;         // 0x805FE520 (quality threshold)
extern f32 lbl_805FE524;         // 0x805FE524 (quality threshold)

extern "C" {

// 0x800C8E54
// Stores a and b into the object's two fields (+0x5AC, +0x398) after calling its virtual slot 0xBC, then runs fn_800C954C.
void fn_800C8E54(u8* p, s32 a, s32 b) {
    typedef void (*VFn)(u8*);
    (*(VFn*)(*(u8**)p + 0xbc))(p);
    *(s32*)(p + 0x5ac) = a;
    *(s32*)(p + 0x398) = b;
    fn_800C954C(p);
}

// 0x800C94B0
// Adds item to the first free slot of the 50-entry table at +0x39C, then calls fn_800C72B4(item, p).
void fn_800C94B0(u8* p, u8* item) {
    if (item == 0) return;
    void** slots = (void**)(p + 0x39c);
    for (s32 i = 0; i < 50; ++i) {
        if (slots[i] == 0) {
            slots[i] = item;
            return fn_800C72B4(item, p);
        }
    }
}

// 0x800C9500
// Removes item from the 50-entry table at +0x39C (clears its slot), then calls fn_800C72C4(item, item).
void fn_800C9500(u8* p, u8* item) {
    if (item == 0) return;
    void** slots = (void**)(p + 0x39c);
    for (s32 i = 0; i < 50; ++i) {
        if (slots[i] == item) {
            slots[i] = 0;
            return fn_800C72C4(item, item);
        }
    }
}

// 0x800C98A8
// Ring-buffer count: with head at +0xB00 and tail at +0xB04 (size at +0xB08), returns the number of
// entries from tail to head inclusive, or 0 if either index is negative.
s32 fn_800C98A8(u8* p) {
    s32 a = *(s32*)(p + 0xb00);
    if (a < 0) return 0;
    s32 b = *(s32*)(p + 0xb04);
    if (b < 0) return 0;
    if (b < a) b += *(s32*)(p + 0xb08);
    return b - a + 1;
}

// 0x800CA938
// Vec3 add: out = a + b. Also sets the int at +0xC to 0 and the flag byte at +0x10 to 0.
void fn_800CA938(u8* out, u8* a, u8* b) {
    f32 x = *(f32*)(a + 0x0) + *(f32*)(b + 0x0);
    f32 y = *(f32*)(a + 0x4) + *(f32*)(b + 0x4);
    f32 z = *(f32*)(a + 0x8) + *(f32*)(b + 0x8);
    *(s32*)(out + 0xc) = 0;
    *(u8*)(out + 0x10) = 0;
    *(f32*)(out + 0x0) = x;
    *(f32*)(out + 0x4) = y;
    *(f32*)(out + 0x8) = z;
}

// 0x800CA9B0
// Vec3 divide: out = v / *div. Sets the int at +0xC to 3 and the flag byte at +0x10 to 0.
void fn_800CA9B0(u8* out, u8* v, u8* div) {
    f32 d = *(f32*)div;
    f32 x = *(f32*)(v + 0x0) / d;
    f32 y = *(f32*)(v + 0x4) / d;
    f32 z = *(f32*)(v + 0x8) / d;
    *(s32*)(out + 0xc) = 3;
    *(u8*)(out + 0x10) = 0;
    *(f32*)(out + 0x0) = x;
    *(f32*)(out + 0x4) = y;
    *(f32*)(out + 0x8) = z;
}

// 0x800CAA00
// Vec3 length. Returns lbl_805FE4B8 when the squared length is <= that constant, else sqrt(x*x+y*y+z*z).
f32 fn_800CAA00(u8* v) {
    f32 x = *(f32*)(v + 0x0);
    f32 y = *(f32*)(v + 0x4);
    f32 z = *(f32*)(v + 0x8);
    f32 sq = (x * x + y * y) + z * z;
    if (sq <= lbl_805FE4B8) return lbl_805FE4B8;
    return sqrtf(sq);
}

// 0x800CAA70
// Vec3 subtract: out = a - b. Also sets the int at +0xC to 0 and the flag byte at +0x10 to 0.
void fn_800CAA70(u8* out, u8* a, u8* b) {
    f32 x = *(f32*)(a + 0x0) - *(f32*)(b + 0x0);
    f32 y = *(f32*)(a + 0x4) - *(f32*)(b + 0x4);
    f32 z = *(f32*)(a + 0x8) - *(f32*)(b + 0x8);
    *(s32*)(out + 0xc) = 0;
    *(u8*)(out + 0x10) = 0;
    *(f32*)(out + 0x0) = x;
    *(f32*)(out + 0x4) = y;
    *(f32*)(out + 0x8) = z;
}

// 0x800CC108
// Constructor: base init (fn_800690D4), vtable at +0x0, clears the flag byte at +0x264 and three floats at +0x268.
void* fn_800CC108(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    f32 zero = lbl_805FE510;
    *(void**)(p + 0x0) = (void*)lbl_8035F55C;
    *(u8*)(p + 0x264) = 0;
    *(f32*)(p + 0x268) = zero;
    *(f32*)(p + 0x26c) = zero;
    *(f32*)(p + 0x270) = zero;
    return p;
}

// 0x800CC168
// Deleting destructor: on a non-null p, runs fn_80062CE0(p, 0), then frees p when del > 0. Returns p.
u8* fn_800CC168(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800CC1C8
// Constructor: base init (fn_800690D4), vtable at +0x0, clears +0x264..+0x26C, sets the floats at +0x278 and +0x280..+0x288 to the zero constant, clears two flag bytes at +0x27C/+0x27D.
void* fn_800CC1C8(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    f32 zero = lbl_805FE510;
    *(s32*)(p + 0x264) = 0;
    *(void**)(p + 0x0) = (void*)lbl_8035F46C;
    *(s32*)(p + 0x268) = 0;
    *(s32*)(p + 0x26c) = 0;
    *(f32*)(p + 0x278) = zero;
    *(u8*)(p + 0x27c) = 0;
    *(u8*)(p + 0x27d) = 0;
    *(f32*)(p + 0x280) = zero;
    *(f32*)(p + 0x284) = zero;
    *(f32*)(p + 0x288) = zero;
    return p;
}

// 0x800CC23C
// Deleting destructor: on a non-null p, runs fn_80062CE0(p, 0), then frees p when del > 0. Returns p.
u8* fn_800CC23C(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800CC680
// Maps the reticle's last staple value (float at +0x278) to a quality tier: 0 if it is >= the first threshold,
// 1 and 2 for the next two thresholds, 3 above the zero constant, otherwise 4. Returns 0 for a null object.
s32 Stitching_Reticle_GetLastStapleQuality(void* a0) {
    u8* p = (u8*)a0;
    if (p == 0) return 0;
    f32 q = *(f32*)(p + 0x278);
    if (q >= lbl_805FE51C) return 0;
    if (q >= lbl_805FE520) return 1;
    if (q >= lbl_805FE524) return 2;
    if (!(q <= lbl_805FE510)) return 3;
    return 4;
}

// 0x800CC7D4
// Deleting destructor: on a non-null p, destroys the members at +0x334 (fn_801F94EC) and +0x278 (fn_801F6238),
// runs the base dtor fn_80062CE0(p, 0), then frees p when del > 0. Returns p.
u8* fn_800CC7D4(u8* p, s32 del) {
    if (p) {
        fn_801F94EC(p + 0x334, -1);
        fn_801F6238(p + 0x278, -1);
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800CCA88
// Cleanup: frees the object at +0x274 (fn_800CC29C) and clears the pointer, then resets the two
// lists at +0x278 and +0x334 through fn_801B3378 and runs fn_8027D270(0, lbl_805FE510, lbl_805FE514).
void fn_800CCA88(u8* p) {
    if (*(u8**)(p + 0x274) != 0) {
        fn_800CC29C(*(u8**)(p + 0x274));
        *(s32*)(p + 0x274) = 0;
    }
    u8* h = fn_801B3308();
    fn_801B3378(h, p + 0x278);
    h = fn_801B3308();
    fn_801B3378(h, p + 0x334);
    fn_8027D270(0, lbl_805FE510, lbl_805FE514);
}

// 0x800CD43C
// Constructor: base init (fn_800C20FC), vtable at +0xC, clears +0x10, +0x18, +0x1C and sets +0x20 to -1.
u8* fn_800CD43C(u8* p) {
    fn_800C20FC(p);
    *(void**)(p + 0xc) = (void*)lbl_8035F76C;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1c) = 0;
    *(s32*)(p + 0x20) = -1;
    return p;
}

// 0x800CD4B4
// Cleanup: base cleanup fn_800C2388, then clears the int at +0x10.
void fn_800CD4B4(u8* p) {
    fn_800C2388(p);
    *(s32*)(p + 0x10) = 0;
}

// 0x800CD4E8
// Returns 1 if the global tag lbl_805F8C70 equals the tag at +8 of the object at +0x9C (0 if that pointer is null), else 0.
s32 fn_800CD4E8(u8* p) {
    u8* sub = *(u8**)(p + 0x9c);
    u32 tag = 0;
    if (sub != 0) tag = *(u32*)(sub + 0x8);
    if (lbl_805F8C70 == tag) return 1;
    return 0;
}

// 0x800CD51C
// Returns 1 if the object's tag at +0x9C -> +8 matches either entry of any of the 7 pairs in lbl_8035F640, else 0.
s32 fn_800CD51C(u8* p) {
    u32* tbl = lbl_8035F640;
    for (s32 i = 0; i < 7; ++i) {
        u8* sub = *(u8**)(p + 0x9c);
        u32 tag = 0;
        if (sub != 0) tag = *(u32*)(sub + 0x8);
        if (tbl[0] == tag) return 1;
        sub = *(u8**)(p + 0x9c);
        tag = 0;
        if (sub != 0) tag = *(u32*)(sub + 0x8);
        if (tbl[1] == tag) return 1;
        tbl += 2;
    }
    return 0;
}

// 0x800CD67C
// Returns 1 if the object's tag at +0x9C -> +8 matches lbl_805F8C80 or lbl_805F8C84, else 0.
s32 fn_800CD67C(u8* p) {
    u8* sub = *(u8**)(p + 0x9c);
    u32 tag = 0;
    if (sub != 0) tag = *(u32*)(sub + 0x8);
    if (lbl_805F8C80[0] == tag) return 1;
    sub = *(u8**)(p + 0x9c);
    tag = 0;
    if (sub != 0) tag = *(u32*)(sub + 0x8);
    if (lbl_805F8C80[1] == tag) return 1;
    return 0;
}

}  // extern "C"
