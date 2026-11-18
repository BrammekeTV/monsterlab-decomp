// Small functions in 0x8016xxxx (handle pools, record copies and the SG module init).
#include "types.h"
#include "engine/engine.h"

// Handle pool layout used by the 0x80160xxx helpers below: 32 entries of 0x1C bytes at p + 0.
// Entry: +0 handle (generation in the upper bits, index in the low 5 bits), +4 state (s16),
// +6 next free index (s16, valid while on the free list), +8 f32, +0xC s32, +0x10..+0x18 f32.
// The free-list head index (-1 when empty) is stored at p + 0x380.
static u8* PoolFind(u8* p, s32 h) {
    if (h == 0) return 0;
    u8* e = p + (h & 0x1f) * 0x1c;
    if (*(s32*)(e + 0x0) != h) return 0;
    return e;
}

// Takes the head entry off the free list; returns 0 when the pool is empty.
static u8* PoolPop(u8* p) {
    s32 head = *(s32*)(p + 0x380);
    if (head < 0) return 0;
    u8* e = p + head * 0x1c;
    *(s32*)(p + 0x380) = *(s16*)(e + 0x6);
    return e;
}

// Table of 64 records of 0xD8 bytes at p; the handle sits at +0xC0 of each record.
static u8* BigFind(u8* p, s32 h) {
    if (h == 0) return 0;
    u8* e = p + (h & 0x3f) * 0xd8;
    if (*(s32*)(e + 0xc0) != h) return 0;
    return e;
}

extern "C" {
u8*  fn_8015ECA0(u8* a3);                                           // 0x8015ECA0
void fn_8016B7AC(void);                                             // 0x8016B7AC
void fn_80174A1C(void);                                             // 0x80174A1C
void fn_801731C0(void);                                             // 0x801731C0
void fn_80173874(void);                                             // 0x80173874
void fn_80184260(void);                                             // 0x80184260
void fn_801704E8(void);                                             // 0x801704E8
void fn_8016A14C(void);                                             // 0x8016A14C
void fn_801762E8(void);                                             // 0x801762E8
void fn_801736E4(void);                                             // 0x801736E4
void fn_80183F20(void);                                             // 0x80183F20
void fn_8016DFF0(void);                                             // 0x8016DFF0
}

extern u8  lbl_8040D500[];          // 0x8040D500 (record passed to fn_8015ECA0)
extern u8  lbl_80376BE8[];          // 0x80376BE8 (default SG memory callback table)
extern u8  lbl_805FF4C8[];          // 0x805FF4C8 (float threshold)
extern f32 lbl_805FF288;            // 0x805FF288 (float constant, pool default)
extern f32 lbl_805FF390;            // 0x805FF390 (float constant)
extern f32 lbl_805FF3A0;            // 0x805FF3A0 (float constant)
extern f32 lbl_805FF3C8;            // 0x805FF3C8 (float constant)
extern f32 lbl_805FF400;            // 0x805FF400 (float constant)
extern f32 lbl_805FF430;            // 0x805FF430 (float constant)
extern f32 lbl_805FF47C;            // 0x805FF47C (float constant)
extern f32 lbl_805FF488;            // 0x805FF488 (float constant)
extern f32 lbl_805FF4D0;            // 0x805FF4D0 (float constant)
extern f32 lbl_805FF4E8;            // 0x805FF4E8 (float constant)
extern f32 lbl_805FF500;            // 0x805FF500 (float constant)
extern f32 lbl_805FF558;            // 0x805FF558 (float constant)
extern f32 lbl_805FF560;            // 0x805FF560 (float constant)
extern f32 lbl_805FF590;            // 0x805FF590 (float constant)
extern EngineMemCallbacks* gSgMem;  // 0x805FB608 (set by SgInit)

extern "C" {

// 0x801604FC
// Allocates a pool entry: marks it active (state 1), sets its floats to the default constant
// and returns its handle, or 0 when the pool is full.
s32 fn_801604FC(u8* p) {
    u8* e = PoolPop(p);
    if (!e) return 0;
    f32 c = lbl_805FF288;
    *(s16*)(e + 0x4)  = 1;
    *(f32*)(e + 0x8)  = c;
    *(s32*)(e + 0xc)  = 1;
    *(f32*)(e + 0x10) = c;
    *(f32*)(e + 0x14) = c;
    *(f32*)(e + 0x18) = c;
    return *(s32*)(e + 0x0);
}

// 0x8016060C
// Sets the float at +8 of an active entry to v. Returns the entry, or 0 for a stale handle.
u8* fn_8016060C(u8* p, s32 h, f32 v) {
    u8* e = PoolFind(p, h);
    if (!e) return 0;
    if (*(s16*)(e + 0x4) == 1) *(f32*)(e + 0x8) = v;
    return e;
}

// 0x80160650
// Allocates a pool entry marked state 2 with +8 = a and +0x10..+0x18 copied from the vec3 at v.
void fn_80160650(u8* p, f32* v, f32 a) {
    u8* e = PoolPop(p);
    if (!e) return;
    *(s16*)(e + 0x4)  = 2;
    *(f32*)(e + 0x8)  = a;
    *(f32*)(e + 0x10) = v[0];
    *(f32*)(e + 0x14) = v[1];
    *(f32*)(e + 0x18) = v[2];
}

// 0x801609CC
// Releases a pool entry: state 0, bumps the handle generation and pushes it on the free list.
void fn_801609CC(u8* p, s32 h) {
    u8* e = PoolFind(p, h);
    if (!e) return;
    s32 next = *(s32*)(e + 0x0) + 0x20;
    *(s16*)(e + 0x4) = 0;
    *(s32*)(e + 0x0) = next;
    *(s16*)(e + 0x6) = (s16)*(s32*)(p + 0x380);
    *(s32*)(p + 0x380) = next & 0x1f;
}

// 0x80162990
// Sets the byte flag at +0xD4 of the record with handle h.
void fn_80162990(u8* p, s32 h) {
    u8* e = BigFind(p, h);
    if (!e) return;
    *(u8*)(e + 0xd4) = 1;
}

// 0x80162B30
// Returns 1 when handle h names a live record in the 0xD8-byte table, else 0.
BOOL fn_80162B30(u8* p, s32 h) {
    return BigFind(p, h) != 0;
}

// 0x80162FD0
// Copies a record: five floats and one word from src, then two default floats at +0x18 and +0x1C.
void fn_80162FD0(u8* dst, u8* src) {
    f32 c = lbl_805FF390;
    *(f32*)(dst + 0x0)  = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4)  = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x8)  = *(f32*)(src + 0x8);
    *(f32*)(dst + 0xc)  = *(f32*)(src + 0xc);
    *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
    *(s32*)(dst + 0x14) = *(s32*)(src + 0x14);
    *(f32*)(dst + 0x18) = c;
    *(f32*)(dst + 0x1c) = c;
}

// 0x80163418
// Copies four floats and one word from src, then sets +0x14 to the default constant.
void fn_80163418(u8* dst, u8* src) {
    *(f32*)(dst + 0x0)  = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4)  = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x8)  = *(f32*)(src + 0x8);
    *(f32*)(dst + 0xc)  = *(f32*)(src + 0xc);
    *(s32*)(dst + 0x10) = *(s32*)(src + 0x10);
    *(f32*)(dst + 0x14) = lbl_805FF3A0;
}

// 0x80163A14
// Copies a float, a word, and two floats from src; sets +0x10 and +0x14 to the default
// constant and clears the word at +0x18.
void fn_80163A14(u8* dst, u8* src) {
    *(f32*)(dst + 0x0)  = *(f32*)(src + 0x0);
    *(s32*)(dst + 0x4)  = *(s32*)(src + 0x4);
    *(f32*)(dst + 0x8)  = *(f32*)(src + 0x8);
    *(f32*)(dst + 0xc)  = *(f32*)(src + 0xc);
    *(f32*)(dst + 0x10) = lbl_805FF3C8;
    *(f32*)(dst + 0x14) = lbl_805FF3C8;
    *(s32*)(dst + 0x18) = 0;
}

// 0x8016481C
// Copies six floats from src; sets +0x18 and +0x1C to the default constant and clears the byte at +0x20.
void fn_8016481C(u8* dst, u8* src) {
    f32 c = lbl_805FF400;
    *(f32*)(dst + 0x0)  = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4)  = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x8)  = *(f32*)(src + 0x8);
    *(f32*)(dst + 0xc)  = *(f32*)(src + 0xc);
    *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
    *(f32*)(dst + 0x14) = *(f32*)(src + 0x14);
    *(f32*)(dst + 0x18) = c;
    *(f32*)(dst + 0x1c) = c;
    *(u8*)(dst + 0x20)  = 0;
}

// 0x80165320
// Copies six floats from src and sets +0x20 to the default constant.
void fn_80165320(u8* dst, u8* src) {
    *(f32*)(dst + 0x0)  = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4)  = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x8)  = *(f32*)(src + 0x8);
    *(f32*)(dst + 0xc)  = *(f32*)(src + 0xc);
    *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
    *(f32*)(dst + 0x14) = *(f32*)(src + 0x14);
    *(f32*)(dst + 0x20) = lbl_805FF430;
}

// 0x80165D70
// Copies the first four floats of src into dst (+0x4 is also written to +0x28), sets +0x10 and
// +0x14 to one constant, +0x18 to zero and +0x20 to another constant.
void fn_80165D70(u8* dst, u8* src) {
    f32 s4 = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x0)  = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4)  = s4;
    *(f32*)(dst + 0x8)  = *(f32*)(src + 0x8);
    *(f32*)(dst + 0xc)  = *(f32*)(src + 0xc);
    *(f32*)(dst + 0x10) = lbl_805FF47C;
    *(f32*)(dst + 0x14) = lbl_805FF47C;
    *(s32*)(dst + 0x18) = 0;
    *(f32*)(dst + 0x20) = lbl_805FF488;
    *(f32*)(dst + 0x28) = s4;
}

// 0x80166468
// Adds the record's +0x10 times +0x0 into the float at +0x8 of the global block returned by
// fn_8015ECA0, but only when +0x10 is above the threshold. The first argument is unused (r3 is overwritten).
void fn_80166468(void* unused, u8* p) {
    (void)unused;
    f32 threshold = *(f32*)lbl_805FF4C8;
    if (*(f32*)(p + 0x10) > threshold) {
        u8* g = fn_8015ECA0(lbl_8040D500);
        *(f32*)(g + 0x8) = *(f32*)(g + 0x8) + *(f32*)(p + 0x10) * *(f32*)(p + 0x0);
    }
}

// 0x801664C4
// Copies eight floats from src and sets +0x20 to the default constant.
void fn_801664C4(u8* dst, u8* src) {
    for (s32 i = 0; i < 8; i++) {
        *(f32*)(dst + i * 4) = *(f32*)(src + i * 4);
    }
    *(f32*)(dst + 0x20) = lbl_805FF4D0;
}

// 0x80166B68
// Copies eight floats from src and sets +0x20 to the default constant.
void fn_80166B68(u8* dst, u8* src) {
    for (s32 i = 0; i < 8; i++) {
        *(f32*)(dst + i * 4) = *(f32*)(src + i * 4);
    }
    *(f32*)(dst + 0x20) = lbl_805FF4E8;
}

// 0x80166EB0
// Copies six floats from src and sets +0x18 to the default constant.
void fn_80166EB0(u8* dst, u8* src) {
    for (s32 i = 0; i < 6; i++) {
        *(f32*)(dst + i * 4) = *(f32*)(src + i * 4);
    }
    *(f32*)(dst + 0x18) = lbl_805FF500;
}

// 0x801688D4
// Copies seven floats from src; sets +0x1C and +0x20 to the default constant.
void fn_801688D4(u8* dst, u8* src) {
    for (s32 i = 0; i < 7; i++) {
        *(f32*)(dst + i * 4) = *(f32*)(src + i * 4);
    }
    *(f32*)(dst + 0x1c) = lbl_805FF558;
    *(f32*)(dst + 0x20) = lbl_805FF558;
}

// 0x80168D1C
// Copies six floats and one word from src; sets +0x1C and +0x20 to the default constant.
void fn_80168D1C(u8* dst, u8* src) {
    for (s32 i = 0; i < 6; i++) {
        *(f32*)(dst + i * 4) = *(f32*)(src + i * 4);
    }
    *(s32*)(dst + 0x18) = *(s32*)(src + 0x18);
    *(f32*)(dst + 0x1c) = lbl_805FF560;
    *(f32*)(dst + 0x20) = lbl_805FF560;
}

// 0x801691A8
// Copies five floats and one word from src; sets +0x18 to the default constant.
void fn_801691A8(u8* dst, u8* src) {
    for (s32 i = 0; i < 5; i++) {
        *(f32*)(dst + i * 4) = *(f32*)(src + i * 4);
    }
    *(s32*)(dst + 0x14) = *(s32*)(src + 0x14);
    *(f32*)(dst + 0x18) = lbl_805FF590;
}

// 0x801695A4
// SG module init: records the memory callback table (the built-in one when mem is null) in
// gSgMem, then runs the subsystem init calls in order.
void SgInit(EngineMemCallbacks* mem) {
    if (mem == 0) mem = (EngineMemCallbacks*)lbl_80376BE8;
    gSgMem = mem;
    fn_8016B7AC();
    fn_80174A1C();
    fn_801731C0();
    fn_80173874();
    fn_80184260();
    fn_801704E8();
    fn_8016A14C();
    fn_801762E8();
    fn_801736E4();
    fn_80183F20();
    fn_8016DFF0();
}

}
