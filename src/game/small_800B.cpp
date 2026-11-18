// Small functions in 0x800Bxxxx (grow juice experiment).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                         // 0x801D426C
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);                   // 0x800690D4 (base ctor)
u8*  fn_80062CE0(u8* p, s32 del);                                      // 0x80062CE0 (base dtor)
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8);      // 0x8006FA44
void  fn_8020A398(s32 handle);                                          // 0x8020A398
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);           // 0x80209E70
u32   fn_801EC880(u8* p, u32 hash);                                     // 0x801EC880
void  fn_801EC8C4(u8* p, u32 a, u32 b);                                 // 0x801EC8C4
s32   fn_800B20A4(u8* p, s32 a, s32 b);                                 // 0x800B20A4
void  fn_800B179C(s32 a);                                               // 0x800B179C
void  fn_800B3180(u8* p);                                               // 0x800B3180
void  fn_800B2740(u8* p);                                               // 0x800B2740
}

extern s32 lbl_805F8BB8;            // 0x805F8BB8 (heartbeat sound handle, -1 when stopped)
extern s32 lbl_805F8BBC;            // 0x805F8BBC (goo bubble sound handle, -1 when stopped)
extern u8  lbl_8035DF40[];          // 0x8035DF40 (vtable)
extern u8  lbl_8035E038[];          // 0x8035E038 (vtable)
extern f32 lbl_805FE0B0;            // 0x805FE0B0 (float default)
extern f32 lbl_805FE0B4;            // 0x805FE0B4 (float default)
extern f32 lbl_805FE108;            // 0x805FE108 (float constant)
extern f32 lbl_805FE110;            // 0x805FE110 (threshold)
extern f32 lbl_805FE114;            // 0x805FE114 (threshold)
extern f32 lbl_805FE11C;            // 0x805FE11C (float constant)
extern f32 lbl_805FE17C;            // 0x805FE17C (threshold)
extern f32 lbl_803C6D2C[];          // 0x803C6D2C (three floats)
extern u8  lbl_803C6D18[];          // 0x803C6D18 (block passed to fn_8006FA44)

extern "C" {

// 0x800B026C
// Plays the weapon-struck effect: two calls on the object's sub-block at +0x94 with fixed hashes.
void GrowJuiceWeapon_PlayStruckEffect(void* a0) {
    u8* p = (u8*)a0;
    if (p) {
        u32 r = fn_801EC880(p + 0x94, 0xCD7726D5u);
        fn_801EC8C4(p + 0x94, r, 0x29249E87u);
    }
}

// 0x800B0530
// Counts how many of the five slots at +0x298..+0x2A8 are non-zero.
s32 fn_800B0530(u8* p) {
    s32 n = 0;
    for (s32 off = 0x298; off <= 0x2a8; off += 4) {
        if (*(s32*)(p + off) != 0) n++;
    }
    return n;
}

// 0x800B0CB4
// Stores value into the first free slot of the five-entry table at +0x2C8. Returns 1 if stored, else 0.
s32 fn_800B0CB4(u8* p, s32 value) {
    for (s32 i = 0; i < 5; i++) {
        if (*(s32*)(p + 0x2c8 + i * 4) == 0) {
            *(s32*)(p + 0x2c8 + i * 4) = value;
            return 1;
        }
    }
    return 0;
}

// 0x800B1498
// Constructor: base ctor fn_800690D4, then sets vtable lbl_8035DF40, clears +0x264, +0x268 and +0x26C.
void* fn_800B1498(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0)   = lbl_8035DF40;
    *(s32*)(p + 0x264) = 0;
    *(f32*)(p + 0x268) = lbl_805FE0B0;
    *(s32*)(p + 0x26c) = -1;
    return p;
}

// 0x800B14EC
// Deleting destructor: base dtor fn_80062CE0 (with del 0) on the object, then free if del > 0.
u8* fn_800B14EC(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800B1564
// Resets the per-object sound state (+0x264..+0x26C), then stops the shared handle lbl_805F8BBC if one is playing.
void fn_800B1564(u8* p) {
    *(s32*)(p + 0x264) = 0;
    *(f32*)(p + 0x268) = lbl_805FE0B4;
    *(s32*)(p + 0x26c) = -1;
    if (lbl_805F8BBC != -1) {
        fn_8020A398(lbl_805F8BBC);
        lbl_805F8BBC = -1;
    }
}

// 0x800B1964
// Starts the heartbeat sound unless one is already playing (handle in lbl_805F8BB8).
void GrowJuiceHeart_StartHeartbeat(u32 a0) {
    if (lbl_805F8BB8 == -1) {
        lbl_805F8BB8 = PlaySound(a0, 0, 0, 0, 0, 1);
    }
}

// 0x800B19A8
// Stops the heartbeat sound if one is playing.
void GrowJuiceHeart_StopHeartbeat(void) {
    if (lbl_805F8BB8 != -1) {
        fn_8020A398(lbl_805F8BB8);
        lbl_805F8BB8 = -1;
    }
}

// 0x800B19DC
// Starts the goo bubble sound unless one is already playing (handle in lbl_805F8BBC).
void GrowJuiceHeart_StartGooBubble(u32 a0) {
    if (lbl_805F8BBC == -1) {
        lbl_805F8BBC = PlaySound(a0, 0, 0, 0, 0, 1);
    }
}

// 0x800B1EDC
// Constructor: base ctor fn_800690D4, then sets vtable lbl_8035E038 and clears +0x264. Returns obj.
void* fn_800B1EDC(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0)   = lbl_8035E038;
    *(s32*)(p + 0x264) = 0;
    return p;
}

// 0x800B1F20
// Deleting destructor: base dtor fn_80062CE0 (with del 0) on the object, then free if del > 0.
u8* fn_800B1F20(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800B2054
// Returns the hash 0xDC2ED9B5 when fn_800B20A4 succeeds (after releasing +0x264 via fn_800B179C), else -1.
s32 fn_800B2054(u8* p, s32 a) {
    if (fn_800B20A4(p, a, 0) != 0) {
        fn_800B179C(*(s32*)(p + 0x264));
        return (s32)0xDC2ED9B5u;
    }
    return -1;
}

// 0x800B4908
// Adds delta to the float at +0x8C; when the total reaches lbl_805FE110 it raises game-flow signal 0x88C6E941.
void fn_800B4908(u8* p, f32 delta) {
    f32 v = *(f32*)(p + 0x8c) + delta;
    *(f32*)(p + 0x8c) = v;
    if (!(v >= lbl_805FE110)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x800B494C
// Same as fn_800B4908 but with the threshold lbl_805FE114.
void fn_800B494C(u8* p, f32 delta) {
    f32 v = *(f32*)(p + 0x8c) + delta;
    *(f32*)(p + 0x8c) = v;
    if (!(v >= lbl_805FE114)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x800B4984
// Runs the two update helpers fn_800B3180 and fn_800B2740 on the object.
void fn_800B4984(u8* p) {
    fn_800B3180(p);
    fn_800B2740(p);
}

// 0x800B5944
// Stores three floats in lbl_803C6D2C ({a, b, a}) and tail-calls fn_8006FA44 with fixed hashes.
void fn_800B5944(void) {
    lbl_803C6D2C[0] = lbl_805FE108;
    lbl_803C6D2C[1] = lbl_805FE11C;
    lbl_803C6D2C[2] = lbl_805FE108;
    fn_8006FA44(lbl_803C6D18, (s32)0x1CD5D8C3u, 3, (s32)0x5B291F8Cu,
                (s32)0x85AB5092u, (s32)0x714859F7u);
}

// 0x800B59A8
// Deleting destructor with no base call: frees the object if del > 0. Returns obj.
u8* fn_800B59A8(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x800B73A0
// Returns 1 when the state at +0x2B4 is 3 and the float at +0x2D0 is >= lbl_805FE17C, else 0.
s32 fn_800B73A0(u8* p) {
    if (*(s32*)(p + 0x2b4) == 3) {
        if (*(f32*)(p + 0x2d0) >= lbl_805FE17C) return 1;
    }
    return 0;
}

// 0x800B73D0
// Returns 1 when the state at +0x2B4 is 3 and the float at +0x2CC is >= lbl_805FE17C, else 0.
s32 fn_800B73D0(u8* p) {
    if (*(s32*)(p + 0x2b4) == 3) {
        if (*(f32*)(p + 0x2cc) >= lbl_805FE17C) return 1;
    }
    return 0;
}

// 0x800B7428
// Stores value at +0x2F8, +0x2FC or +0x300 for index 0, 1 or 2. Other indices do nothing.
void fn_800B7428(u8* p, s32 idx, s32 value) {
    if (idx == 0) {
        *(s32*)(p + 0x2f8) = value;
    } else if (idx == 1) {
        *(s32*)(p + 0x2fc) = value;
    } else if (idx == 2) {
        *(s32*)(p + 0x300) = value;
    }
}

}  // extern "C"
