// Small functions around 0x8002C6xx, 0x8004ECxx, 0x8008Dxxx, 0x8009Axxx and 0x8011-0x8012xxxx:
// record start/state helpers, flag setters on the sub-object, float range tests and the
// reset helpers for four 0x10-byte float triples.
#include "types.h"
#include "game/part.h"

typedef u8* (*VGetFn)(u8* self);                      // virtual getter at vtable+0xC

extern "C" {
void UIPlayAnim(void* el, u32 anim);                // 0x8019BE40
void fn_80069638(u8* p, s32 a1);                      // 0x80069638
void fn_8004E8E8(u8* p);                              // 0x8004E8E8
void fn_801ED050(u8* p, s32 flag);                    // 0x801ED050
s32  fn_8002C73C(u8* p);                              // 0x8002C73C

extern f32 lbl_805FCBB4;                              // 0x805FCBB4 (float)
extern f64 lbl_805FCBB8;                              // 0x805FCBB8 (double)
extern f32 lbl_805FCBC0;                              // 0x805FCBC0 (float)
extern f32 lbl_805FCBC8;                              // 0x805FCBC8 (float, range low)
extern f32 lbl_805FCBCC;                              // 0x805FCBCC (float, range high)
extern f32 lbl_805FD670;                              // 0x805FD670 (float)
extern f32 lbl_805FDA18;                              // 0x805FDA18 (float)
extern f32 lbl_803462C8;                              // 0x803462C8 (float)
extern f32 lbl_805FDC50;                              // 0x805FDC50 (float, 0.0)
extern f32 lbl_803E6B90[3];                           // 0x803E6B90
extern f32 lbl_803E6BA0[3];                           // 0x803E6BA0
extern f32 lbl_803E6BB0[3];                           // 0x803E6BB0
extern f32 lbl_803E6BC0[3];                           // 0x803E6BC0
extern f32 lbl_803E6BD0[3];                           // 0x803E6BD0
extern f32 lbl_805FED20;                              // 0x805FED20 (float)
extern f32 lbl_805FED24;                              // 0x805FED24 (float)
extern f32 lbl_805FED28;                              // 0x805FED28 (float)
extern f32 lbl_805FED2C;                              // 0x805FED2C (float)
extern f32 lbl_805FED30;                              // 0x805FED30 (float)
extern f32 lbl_805FED40;                              // 0x805FED40 (float)
extern f32 lbl_805FED48;                              // 0x805FED48 (float)
extern f32 lbl_805FED4C;                              // 0x805FED4C (float)
extern f32 lbl_805FED50;                              // 0x805FED50 (float)
extern f32 lbl_805FED54;                              // 0x805FED54 (float)
}

extern "C" {

// 0x8002C63C
// Starts a record (only while its state at +0xC is 0): stores the inputs, saves the target's
// +0x68..+0x70 vector, applies a scale to the target's +0x78 value and plays the state anim.
void fn_8002C63C(u8* p, u8 flag, u8* target, s32 arg3, f32 scale) {
    if (*(s32*)(p + 0xc) != 0) {
        return;
    }
    *(s32*)(p + 0xc) = 1;
    *(u8*)(p + 0x19) = flag;
    *(u8**)(p + 0x1c) = target;
    *(s32*)(p + 0x20) = arg3;
    *(f32*)(p + 0x0) = *(f32*)(target + 0x68);
    *(f32*)(p + 0x4) = *(f32*)(target + 0x6c);
    *(f32*)(p + 0x8) = *(f32*)(target + 0x70);
    *(f32*)(target + 0x68) = lbl_805FCBB4;
    *(f32*)(p + 0x10) = scale;
    *(u8*)(p + 0x18) = (u8)(scale >= lbl_805FCBB8);
    *(f32*)(p + 0x14) = *(f32*)(*(u8**)(p + 0x1c) + 0x78);
    if (*(u8*)(p + 0x19) == 0) {
        *(u8*)(p + 0x18) = 1;
        *(f32*)(p + 0x10) = lbl_805FCBC0;
    }
    *(f32*)(*(u8**)(p + 0x1c) + 0x78) = *(f32*)(p + 0x10) * *(f32*)(p + 0x14);
    s32 anim = fn_8002C73C(p);
    if ((u32)anim != (u32)*(s32*)(p + 0x24) && anim != 0) {
        *(s32*)(p + 0x24) = anim;
        UIPlayAnim(*(u8**)(p + 0x1c), (u32)anim);
    }
    u8* obj = *(u8**)(p + 0x1c);
    u8* sub = *(u8**)(obj + 0x138);
    ((void (*)(u8*, s32))(*(void**)(sub + 0x2c)))(obj, 0x18);
}

// 0x8002C73C
// Returns a state code for the record at p: 0 is the "inactive" code, the range checks pick
// among the codes using the value at +0x10 and the game-db thresholds.
s32 fn_8002C73C(u8* p) {
    if (*(u8*)(p + 0x19) == 0) {
        return 0x35C15887;
    }
    f32 v = *(f32*)(p + 0x10);
    if (v > lbl_805FCBC8 && v < lbl_805FCBCC) {
        return 0x97D63107;
    }
    u8* rec = *(u8**)(gGameDb->data + 0x99D0);
    f32 lo = *(f32*)(rec + 0xe4);
    if (v < lo) {
        return 0x0B559F67;
    }
    f32 mid = *(f32*)(rec + 0xdc);
    if (v < mid) {
        return 0xC6DD51E0;
    }
    f32 d = v - lbl_805FCBC0;
    if (d > lbl_805FCBC8 && d < lbl_805FCBCC) {
        return 0x73B3A02B;
    }
    return 0x0B1C0C07;
}

// 0x8004EC00
// Sets (state byte at +0x38F != 0) or clears the 0x2 flag on the child objects of p, applies
// the 0x8000 flag on p's sub-record, and clears the sub-state when the flag is off.
void fn_8004EC00(u8* p, s32 arg1) {
    fn_80069638(p, arg1);
    if (*(u8*)(p + 0x38f) != 0) {
        if ((*(VGetFn*)(*(u8**)p + 0xc))(p) != NULL) {
            u8* q = (*(VGetFn*)(*(u8**)p + 0xc))(p);
            *(s32*)(q + 0x8) |= 2;
        }
        u8* sub = *(u8**)(p + 0x4);
        *(s32*)(sub + 0x20) |= 2;
        fn_8004E8E8(p);
    } else {
        if ((*(VGetFn*)(*(u8**)p + 0xc))(p) != NULL) {
            u8* q = (*(VGetFn*)(*(u8**)p + 0xc))(p);
            *(s32*)(q + 0x8) &= ~2;
        }
        u8* sub = *(u8**)(p + 0x4);
        *(s32*)(sub + 0x20) &= ~2;
        fn_801ED050(p + 0x94, 0);
        *(u8*)(p + 0x38e) = 0;
    }
    u8* s = *(u8**)(p + 0x4);
    *(s32*)(s + 0x20) |= 0x8000;
}

// 0x800771F8
// Returns 1 when the float at +0x10 is greater than the threshold constant.
s32 fn_800771F8(u8* p) {
    return *(f32*)(p + 0x10) > lbl_805FD670;
}

// 0x8008D67C
// Returns 1 when the float at +0x308 is less than or equal to the threshold constant.
s32 fn_8008D67C(u8* p) {
    return *(f32*)(p + 0x308) <= lbl_805FDA18;
}

// 0x80099AD8
// Returns 1 when the float global lbl_803462C8 is greater than 0.0.
s32 fn_80099AD8(void) {
    return lbl_803462C8 > lbl_805FDC50;
}

// 0x8011EA00
// Resets the float triple lbl_803E6B90: [0] = [2] = first constant, [1] = second constant.
void fn_8011EA00(void) {
    lbl_803E6B90[0] = lbl_805FED20;
    lbl_803E6B90[1] = lbl_805FED24;
    lbl_803E6B90[2] = lbl_805FED20;
}

// 0x8011F3F0
// Resets the float triple lbl_803E6BA0 (same pattern as fn_8011EA00).
void fn_8011F3F0(void) {
    lbl_803E6BA0[0] = lbl_805FED28;
    lbl_803E6BA0[1] = lbl_805FED2C;
    lbl_803E6BA0[2] = lbl_805FED28;
}

// 0x80120A8C
// Resets the float triple lbl_803E6BB0 (same pattern as fn_8011EA00).
void fn_80120A8C(void) {
    lbl_803E6BB0[0] = lbl_805FED30;
    lbl_803E6BB0[1] = lbl_805FED40;
    lbl_803E6BB0[2] = lbl_805FED30;
}

// 0x80120D14
// Resets the float triple lbl_803E6BC0 (same pattern as fn_8011EA00).
void fn_80120D14(void) {
    lbl_803E6BC0[0] = lbl_805FED48;
    lbl_803E6BC0[1] = lbl_805FED4C;
    lbl_803E6BC0[2] = lbl_805FED48;
}

// 0x80120DA0
// Resets the float triple lbl_803E6BD0 (same pattern as fn_8011EA00).
void fn_80120DA0(void) {
    lbl_803E6BD0[0] = lbl_805FED50;
    lbl_803E6BD0[1] = lbl_805FED54;
    lbl_803E6BD0[2] = lbl_805FED50;
}

}  // extern "C"
