// Misc decompiled functions: RNG (Mersenne Twister), matrix stack push, fade quad draw,
// character facing, ... Verified against the asm (tools/dump.sh).
#include <math.h>
#include <string.h>

#include "revolution.h"
#include "types.h"
#include "engine/math.h"
#include "engine/rend.h"
#include "game/character.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gEventMgr;   // 0x805FB6C8

extern "C" {
void  EntitySyncTransform(void* entity, int a);                // 0x801B4F4C
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);   // 0x801BA2FC
u32   RandomU32(void);
u32   StringHash(const char* s);
int   SysSprintf(char* buf, const char* fmt, ...);
void  TriggerDoAction(void* action, void* a4, void* a5);       // 0x801E0054
void  fn_8001D4FC(u8* mgr);                                    // 0x8001D4FC
void  fn_8003DCE0(u8* p);                                      // 0x8003DCE0
void* fn_8003DDE8(void* part, s32 slot, s32 def, s32 flag, s32 a);   // 0x8003DDE8
void  fn_8004C718(u8* p, s32 idx, void* elem);                 // 0x8004C718
s32   fn_8004D3F8(void* c, const char* name, s32 a, s32 b);    // 0x8004D3F8
u8*   fn_8004EDD0(u8* obj, s32 flag, s32 a2);                  // 0x8004EDD0
void  fn_8006A764(u8* prop, f32 t);                            // 0x8006A764
void  fn_80142ED0(u8* base, s32 size);                         // 0x80142ED0
void  fn_801443B0(f32* m);                                     // 0x801443B0 (matrix op, result unused)
void  fn_8014474C(u8* src, f32* out);                          // 0x8014474C
void  fn_801458B0(f32* quat, f32* a, f32* b);                  // 0x801458B0
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);            // 0x80145F9C
void  fn_801542DC(void);                                       // 0x801542DC
s32   fn_8015BA30(void* p);                                    // 0x8015BA30
void  fn_8016A2B8(u8* p);                                      // 0x8016A2B8
void  fn_8016A3F8(u8* p);                                      // 0x8016A3F8
void  fn_8016A43C(u8* p);                                      // 0x8016A43C
void  fn_8016A570(u8* p);                                      // 0x8016A570
void  fn_8016B478(u8* player, u8* track);                      // 0x8016B478
u8*   fn_8016B50C(void* a, s32 b);                             // 0x8016B50C
void  fn_8018B2CC(u8* obj, u8* entry, s32 a, s32 b, f32 c, f32 d);   // 0x8018B2CC
void  fn_8018A948(u8* obj, f32 t);                             // 0x8018A948
f32   fn_8018AF28(u8* obj, f32* a, f32* b);                    // 0x8018AF28
f32   fn_8018BE30(u8* p, f32 t);                               // 0x8018BE30
void  fn_8018C3D0(void* p);                                    // 0x8018C3D0
u8*   fn_8018C47C(u8* obj);                                    // 0x8018C47C
void  fn_80195A58(void* gx, void* a, u8* b, u8* c);            // 0x80195A58
s32   fn_8019600C(void* gx);                                   // 0x8019600C
s32   fn_801960E4(void* gx);                                   // 0x801960E4
u8*   fn_801AC184(u8* defs, s32 idx);                          // 0x801AC184
void* fn_801AC460(u8* anim, s32 a);                            // 0x801AC460
void  fn_801B1778(void* p, s32 a);                             // 0x801B1778
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                  // 0x801B55F4
void  fn_801DA344(u8* obj, u8* prop);                          // 0x801DA344
void  fn_801DA750(u8* obj, s32 state);                         // 0x801DA750
void  fn_801E93EC(u8* list, u8* entry);                        // 0x801E93EC
void  fn_801ED130(void* p);                                    // 0x801ED130

extern const char lbl_80355530[];   // 0x80355530
extern const char lbl_805F8270[];   // 0x805F8270 ("hanging")
extern s32 lbl_803C4E28[];          // 0x803C4E28
extern u8  lbl_80400FF0[];          // 0x80400FF0 (GX state block)
extern u8  lbl_804FD9E0[];          // 0x804FD9E0 (fade quad vertices, 0x18 bytes each)
extern f32 lbl_804FDA60[];          // 0x804FDA60 (3x4 transform)
extern u32 lbl_805FA0F0[];          // 0x805FA0F0 (MT19937 mag01 table)
extern u8* lbl_805FAF78;            // 0x805FAF78
extern s32 lbl_805FB034;            // 0x805FB034
extern s32 lbl_805FB4BC;            // 0x805FB4BC
extern s32 lbl_805FB4C0;            // 0x805FB4C0
extern s32 lbl_805FB4C4;            // 0x805FB4C4
extern s32 lbl_805FB4C8;            // 0x805FB4C8
extern s32 lbl_805FB4CC;            // 0x805FB4CC
extern u8* lbl_805FB4D0;            // 0x805FB4D0
extern s32 lbl_805FB4D4;            // 0x805FB4D4
extern s32 lbl_805FB4D8;            // 0x805FB4D8
extern s32 lbl_805FB4DC;            // 0x805FB4DC
extern s32 lbl_805FB4E0;            // 0x805FB4E0
extern s32 lbl_805FB4E8;            // 0x805FB4E8
extern s32 lbl_805FB4EC;            // 0x805FB4EC
extern f32 lbl_805FB510;            // 0x805FB510
extern void* lbl_805FB658;          // 0x805FB658
extern u8* lbl_805FB8D8;            // 0x805FB8D8 (effect pool)
extern u8* lbl_805FBA78;            // 0x805FBA78
extern u8  lbl_805FBA7C;            // 0x805FBA7C
extern u8* lbl_805FBBE0;            // 0x805FBBE0
extern f32 lbl_805FD300;            // 0x805FD300 (0.0)
extern f32 lbl_805FD304;            // 0x805FD304 (1.0)
extern f32 lbl_805FD368;            // 0x805FD368 (0.5)
extern f32 lbl_805FD3D8;            // 0x805FD3D8 (0.01)
extern f32 lbl_805FD4E8;            // 0x805FD4E8 (0.0)
extern f32 lbl_805FD500;            // 0x805FD500 (0.075)
extern f32 lbl_805FD504;            // 0x805FD504 (0.99)
extern f32 lbl_805FD508;            // 0x805FD508 (1.0)
extern f32 lbl_805FF5A8;            // 0x805FF5A8 (time scale)
}

extern "C" {

// 0x80196678
// Pushes one 3x4 matrix onto the matrix stack at arg0 (count at +0x40, 0x60 bytes per entry:
// local copy at +0x44, accumulated world matrix at +0x74). The world matrix is the previous
// entry's world matrix times the new local one (the first entry is just a copy).
// Always returns 0; f1 is passed through by callers but unused.
s32 fn_80196678(void* gx, void* mat, f32 t) {
    u8* list = (u8*)gx;
    f32* b = (f32*)mat;
    s32 n = *(s32*)(list + 0x40);
    f32* local = (f32*)(list + n * 0x60 + 0x44);
    for (s32 i = 0; i < 12; i++) local[i] = b[i];
    n = *(s32*)(list + 0x40);
    if (n > 0) {
        f32* p = (f32*)(list + (n - 1) * 0x60 + 0x74);
        f32* d = (f32*)(list + n * 0x60 + 0x74);
        for (s32 j = 0; j < 4; j++) {
            f32 bx = b[j * 3 + 0];
            f32 by = b[j * 3 + 1];
            f32 bz = b[j * 3 + 2];
            for (s32 r = 0; r < 3; r++) {
                f32 v = p[6 + r] * bz + (p[r] * bx + p[3 + r] * by);
                if (j == 3) v = p[9 + r] + v;
                d[j * 3 + r] = v;
            }
        }
    } else {
        f32* d = (f32*)(list + n * 0x60 + 0x74);
        for (s32 i = 0; i < 12; i++) d[i] = b[i];
    }
    *(s32*)(list + 0x40) = *(s32*)(list + 0x40) + 1;
    return 0;
}

// 0x801FC1EC
// Draws the full-screen fade quad: with lbl_805FBA7C (alpha byte) nonzero, sets the four vertex
// colours of lbl_804FD9E0 (stride 0x18, colour at +8) to white with alpha = byte / 255 and draws
// the quad through fn_80195A58, then writes the two BP register commands.
void fn_801FC1EC(void) {
    if (lbl_805FBA7C == 0) return;
    fn_8019600C(lbl_805FB658);
    fn_801960E4(lbl_805FB658);
    fn_801542DC();
    f32 one = 1.0f;
    f32 alpha = (1.0f / 255.0f) * (f32)lbl_805FBA7C;
    for (s32 i = 0; i < 4; i++) {
        s32 r = (s32)(255.0f * one);
        s32 g = (s32)(255.0f * one);
        s32 b = (s32)(255.0f * one);
        s32 a = (s32)(255.0f * alpha);
        if (r < 0) r = 0; else if (r > 0xFF) r = 0xFF;
        if (g < 0) g = 0; else if (g > 0xFF) g = 0xFF;
        if (b < 0) b = 0; else if (b > 0xFF) b = 0xFF;
        if (a < 0) a = 0; else if (a > 0xFF) a = 0xFF;
        u8* col = lbl_804FD9E0 + 8 + i * 0x18;
        col[0] = (u8)r;
        col[1] = (u8)g;
        col[2] = (u8)b;
        col[3] = (u8)a;
    }
    fn_80195A58(lbl_805FB658, lbl_805FBA78, (u8*)lbl_804FDA60, lbl_804FD9E0);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00000Fu;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x80000005u;
}

// 0x80220C8C
// Mersenne Twister (MT19937) step. State: 624 words at +0, position at +0x9C0 (0x271 = not
// seeded yet, seed in +0x9C4, which is expanded with an LCG first), tempered output.
u32 RngNext(void* rng) {
    u32* mt = (u32*)rng;
    s32 pos = *(s32*)((u8*)rng + 0x9C0);
    if (pos >= 0x270) {
        if (pos == 0x271) {
            u32 seed = *(u32*)((u8*)rng + 0x9C4);
            for (s32 i = 0; i < 0x270; i++) {
                mt[i] = seed & 0xFFFF0000u;
                seed = seed * 0x10DCDu + 1;
                mt[i] |= seed >> 16;
                seed = seed * 0x10DCDu + 1;
            }
            *(s32*)((u8*)rng + 0x9C0) = 0x270;
            *(u32*)((u8*)rng + 0x9C4) = seed + 1;
        }
        for (s32 i = 0; i < 0x26F; i++) {
            u32 y = (mt[i] & 0x80000000u) | (mt[i + 1] & 0x7FFFFFFFu);
            u32 m = (i < 227) ? mt[i + 397] : mt[i - 227];
            mt[i] = m ^ (y >> 1) ^ lbl_805FA0F0[y & 1];
        }
        u32 y = (mt[0x26F] & 0x80000000u) | (mt[0] & 0x7FFFFFFFu);
        *(s32*)((u8*)rng + 0x9C0) = 0;
        mt[0x26F] = mt[396] ^ (y >> 1) ^ lbl_805FA0F0[y & 1];
    }
    pos = *(s32*)((u8*)rng + 0x9C0);
    *(s32*)((u8*)rng + 0x9C0) = pos + 1;
    u32 y = mt[pos];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9D2C5680u;
    y ^= (y << 15) & 0xEFC60000u;
    return y ^ (y >> 18);
}

// 0x8005B650
// Turns the character to face the (x, y) direction `dir` (z is zeroed and the vector
// normalized in place). A direction shorter than 0.1 resets the facing quaternion at +0x738 to
// identity. Otherwise the quaternion is built with fn_801458B0 from dir and an up/look vector
// (the camera look vector at +0x7A4 when flag bit 0x08000000 and the slot-0x4C virtual or
// def flag 0x20 say so, else +Z) and renormalized. With `now` the quaternion is copied to the
// def object (+4, +0xC..0x18) and its update flags set, else the 0x10000000 flag is raised.
void CharacterFaceDirection(Character* c, const Vec3* dirIn, BOOL now) {
    u8* p = (u8*)c;
    Vec3* dir = (Vec3*)dirIn;
    typedef s32 (*SlotFn)(void*);
    f32 zero = lbl_805FD300;
    f32 one = lbl_805FD304;
    f32 x = dir->x;
    f32 y = dir->y;
    dir->z = zero;
    f32 len2 = zero * zero + (x * x + y * y);
    if (len2 < lbl_805FD3D8) {
        *(f32*)(p + 0x738) = zero;
        *(f32*)(p + 0x73C) = zero;
        *(f32*)(p + 0x740) = zero;
        *(f32*)(p + 0x744) = one;
    } else {
        f32 nx, ny, nz;
        if (zero == len2) {
            nx = zero; ny = zero; nz = zero;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            nx = x * inv; ny = y * inv; nz = zero * inv;
        }
        dir->x = nx;
        dir->y = ny;
        dir->z = nz;
        s32 useLook = 0;
        if (*(u32*)(p + 0x838) & 0x08000000) {
            if ((*(SlotFn*)(*(u8**)p + 0x4C))(p) != 0 || (*(u32*)(*(u8**)(p + 4) + 0x1C) & 0x20)) useLook = 1;
        }
        if (useLook) {
            f32 half = lbl_805FD368;
            f32 vx = *(f32*)(p + 0x7A4);
            f32 vy = *(f32*)(p + 0x7A8);
            f32 vz = *(f32*)(p + 0x7AC);
            if (vz < half) {
                vz = half;
                f32 l2 = half * half + (vx * vx + vy * vy);
                if (lbl_805FD300 == l2) {
                    vx = lbl_805FD300; vy = lbl_805FD300; vz = lbl_805FD300;
                } else {
                    f32 inv = 1.0f / sqrtf(l2);
                    vx = vx * inv; vy = vy * inv; vz = half * inv;
                }
            }
            if ((*(SlotFn*)(*(u8**)p + 0x4C))(p) == 0) {
                f32 a = vx + lbl_805FD300;
                f32 b = vy + lbl_805FD300;
                f32 d = vz + lbl_805FD304;
                vx = a * lbl_805FD368;
                vy = b * lbl_805FD368;
                vz = d * lbl_805FD368;
            }
            f32 dx = dir->x;
            f32 dy = dir->y;
            f32 dz = dir->z;
            f32 t0 = dy * vz - dz * vy;
            f32 t1 = dz * vx - dx * vz;
            f32 t2 = dx * vy - dy * vx;
            dir->x = vy * t2 - vz * t1;
            dir->y = vz * t0 - vx * t2;
            dir->z = vx * t1 - vy * t0;
            f32 v[3] = { vx, vy, vz };
            fn_801458B0((f32*)(p + 0x738), &dir->x, v);
        } else {
            f32 v[3];
            v[0] = lbl_805FD300;
            v[1] = lbl_805FD300;
            v[2] = lbl_805FD304;
            fn_801458B0((f32*)(p + 0x738), &dir->x, v);
        }
        f32 qx = *(f32*)(p + 0x738);
        f32 qy = *(f32*)(p + 0x73C);
        f32 qz = *(f32*)(p + 0x740);
        f32 qw = *(f32*)(p + 0x744);
        f32 q2 = qw * qw + (qz * qz + (qx * qx + qy * qy));
        f32 inv = 1.0f / sqrtf(q2);
        *(f32*)(p + 0x738) = qx * inv;
        *(f32*)(p + 0x73C) = qy * inv;
        *(f32*)(p + 0x740) = qz * inv;
        *(f32*)(p + 0x744) = qw * inv;
    }
    if (now) {
        u8* def = *(u8**)(p + 4);
        *(u32*)(p + 0x838) &= 0xEFFFFFFFu;
        *(u32*)(def + 0x20) |= 0x8000;
        def = *(u8**)(p + 4);
        *(u32*)(def + 0x20) |= 0x10000000;
        def = *(u8**)(p + 4);
        *(u32*)(def + 0x20) |= 0x4000;
        def = *(u8**)(p + 4);
        *(u32*)(def + 0x20) |= 0x10000000;
        def = *(u8**)(p + 4);
        *(f32*)(def + 0xC) = *(f32*)(p + 0x738);
        *(f32*)(def + 0x10) = *(f32*)(p + 0x73C);
        *(f32*)(def + 0x14) = *(f32*)(p + 0x740);
        *(f32*)(def + 0x18) = *(f32*)(p + 0x744);
    } else {
        *(u32*)(p + 0x838) |= 0x10000000u;
    }
}

typedef s32 (*ZonePoolCb)(u8* self, void* arg, f32 dt);

// Resolves a pool handle (low byte = slot, high byte = generation) to its entry, or NULL.
static u8* ZonePoolEntry(u16 h) {
    u8* pool = lbl_805FB8D8;
    u32 idx = h & 0xFF;
    if (idx < 0xFF && (h & 0xFF00) != 0) return pool + (idx << 7);
    return NULL;
}

// Calls the entry's callback with dt (adds dt to its timer first, stores the result byte at
// +0x7E). Returns 3 (finished) when the entry has no callback.
static s32 ZonePoolCall(u8* e, f32 dt) {
    s32 r = 3;
    if (*(ZonePoolCb*)(e + 0x6C) != NULL) {
        *(f32*)(e + 0x70) = *(f32*)(e + 0x70) + dt;
        r = (*(ZonePoolCb*)(e + 0x6C))(e, *(void**)(e + 0x68), dt);
        e[0x7E] = (u8)r;
    }
    return r;
}

// Runs an entry's callback once with dt = 0 (state 0x40 while running, 0x80 when done).
static void ZonePoolRun(u8* e) {
    e[0x7F] = 0x40;
    ZonePoolCall(e, 0.0f);
    e[0x7F] = 0x80;
}

// Walks the entry chain starting at the list head: entries in state 0x40 are run, entries that
// were already finished (0x80) are handed to fn_801E93EC. Clears the dirty flag (bit 1).
static void ZoneFlush(u8* z) {
    u8* e = ZonePoolEntry(*(u16*)(z + 0));
    while (e != NULL) {
        u8* next = ZonePoolEntry(*(u16*)(e + 0x78));
        u8 st = e[0x7F];
        if (st == 0x40 && st != 0x80) ZonePoolRun(e);
        if (st == 0x80) fn_801E93EC(z, e);
        e = next;
    }
    *(u16*)(z + 6) = *(u16*)(z + 6) & 0xFFFD;
}

// 0x801E97FC
// Per-frame update of a zone effect list (head handle +0, tail handle +2, flags +6: bit0 =
// updating, bit1 = needs flush). Steps every entry in state 0x10 (start) / 0x20 (running) with dt;
// a callback returning 3 finishes the entry (state 0x80) and flags a flush, other results stop
// the walk. A pending flush runs ZoneFlush before and after.
void ZoneEffectsUpdate(void* effects, f32 dt) {
    u8* z = (u8*)effects;
    if (*(u16*)(z + 6) & 2) ZoneFlush(z);
    if (*(u16*)(z + 6) & 1) return;
    u16 head = *(u16*)(z + 0);
    if (!((head & 0xFF) < 0xFF && (head & 0xFF00) != 0)) return;
    u8* e = ZonePoolEntry(head);
    u8* tail = ZonePoolEntry(*(u16*)(z + 2));
    *(u16*)(z + 6) = *(u16*)(z + 6) | 1;
    while (e != NULL) {
        s32 r;
        if (e[0x7F] == 0x10) {
            if (*(u16*)(z + 6) & 1) {
                r = ZonePoolCall(e, 0.0f);
                if (r == 3) {
                    if (e[0x7F] != 0x80) ZonePoolRun(e);
                    *(u16*)(z + 6) = *(u16*)(z + 6) | 2;
                } else {
                    e[0x7F] = 0x20;
                }
            }
            if (!(*(u16*)(z + 6) & 1)) return;
        }
        if (e[0x7F] == 0x20) {
            r = ZonePoolCall(e, dt);
            if (!(*(u16*)(z + 6) & 1)) return;
            if (r != 2) {
                if (r < 2 || r >= 4) break;
                if (e[0x7F] != 0x80) ZonePoolRun(e);
                if (!(*(u16*)(z + 6) & 1)) return;
                *(u16*)(z + 6) = *(u16*)(z + 6) | 2;
            }
        }
        if (e == tail) break;
        e = ZonePoolEntry(*(u16*)(e + 0x78));
    }
    u16 fl = *(u16*)(z + 6);
    *(u16*)(z + 6) = fl & 0xFFFE;
    if (fl & 2) ZoneFlush(z);
}

typedef void* (*MgrVFn0)(void*);
typedef void* (*MgrVFn1)(void*, s32);
typedef void (*MgrVFn2)(void*, s32, void*);

// Unhooks the part at mgr slot idx: resets the part pointer, re-registers the part's template
// (virtual slot 0xCC), and un-hangs the figure ("hanging" anim) when present.
static void LightningSlotClear(u8* mgr, s32 idx) {
    s32 off = idx * 4;
    fn_8004C718(*(u8**)mgr, idx, NULL);
    u8* obj = *(u8**)(mgr + off + 0xC04);
    if (obj != NULL) {
        fn_8004C718(*(u8**)mgr, idx, (*(MgrVFn0*)(*(u8**)obj + 0xCC))(obj));
        u8* t = *(u8**)(*(u8**)mgr + off + 0xC50);
        if (t != NULL) fn_8004EDD0(t, 0, 1);
        if (lbl_805FAF78 != NULL) {
            u8* x = *(u8**)lbl_805FAF78;
            if (x != NULL) fn_8004D3F8(x, lbl_805F8270, 0, 1);
        }
    }
}

// 0x8001D718
// Updates the lightning/operating-table manager for slot idx: refreshes the hit-test flag of the
// three table buttons from lbl_803C4E28 (all parts loaded), and when the slot's selection index
// (+0x198) differs from the applied one (+0x19C) swaps the slot's part: the old one is released
// (unless it is one of the five preset parts), the new one is created with fn_8003DDE8 and the
// slot object's flags updated (bit 1 cleared/set, 0x8000 set). Slot stride 0x1A4, list at +8.
void fn_8001D718(u8* mgr, s32 idx, u8* slot) {
    s32 allLoaded = 1;
    for (s32 i = 0; i < lbl_805FB034; i++) {
        if (lbl_803C4E28[i] == 0) allLoaded = 0;
    }
    lbl_805FAF78[0xA8C] = (u8)allLoaded;
    u8 v = lbl_805FAF78[0xA8C];
    if (v != 0 && lbl_805FAF78[0xC18] == 0) {
        lbl_805FAF78[0xC18] = 1;
        UIElement_SetDoHitTest(StringHash(lbl_80355530 + 0x60), lbl_805FAF78[0xC18]);
        UIElement_SetDoHitTest(StringHash(lbl_80355530 + 0x6C), lbl_805FAF78[0xC18]);
        UIElement_SetDoHitTest(StringHash(lbl_80355530 + 0x78), lbl_805FAF78[0xC18]);
    } else if (v == 0 && lbl_805FAF78 != NULL) {
        lbl_805FAF78[0xC18] = 0;
        UIElement_SetDoHitTest(StringHash(lbl_80355530 + 0x60), lbl_805FAF78[0xC18]);
        UIElement_SetDoHitTest(StringHash(lbl_80355530 + 0x6C), lbl_805FAF78[0xC18]);
        UIElement_SetDoHitTest(StringHash(lbl_80355530 + 0x78), lbl_805FAF78[0xC18]);
    }

    u8* sl = mgr + idx * 0x1A4;
    s32 cnt = *(s32*)(sl + 0x1A0);
    s32 cur = (cnt > 0 && *(s32*)(sl + 0x198) < cnt) ? *(s32*)(sl + 8 + *(s32*)(sl + 0x198) * 4) : -1;
    s32 prev = (cnt > 0 && *(s32*)(sl + 0x19C) < cnt) ? *(s32*)(sl + 8 + *(s32*)(sl + 0x19C) * 4) : -1;
    if (cur == prev) return;

    s32 ok = 1;
    for (s32 i = 0; i < lbl_805FB034; i++) {
        if (lbl_803C4E28[i] == 0) ok = 0;
    }
    if (cnt <= 0) return;
    if (ok != 0) {
        uintptr_t w = (uintptr_t) * (void**)(*(u8**)mgr + idx * 4 + 0xC6C);
        if (w != 0 && w < 0x3E8) ok = 0;
    }
    if (ok == 0) return;

    s32 off = idx * 4;
    u8* old = *(u8**)(*(u8**)mgr + off + 0xC50);
    if (old != NULL) {
        *(s32*)(sl + 0x1A4) = 0;
        LightningSlotClear(mgr, idx);
        s32 release = 1;
        if (old == *(u8**)(mgr + 0xC04)) release = 0;
        if (old == *(u8**)(mgr + 0xC08)) release = 0;
        if (old == *(u8**)(mgr + 0xC0C)) release = 0;
        if (old == *(u8**)(mgr + 0xC10)) release = 0;
        if (old == *(u8**)(mgr + 0xC14)) release = 0;
        if (release != 0) {
            fn_8003DCE0(old);
            if (*(s32*)(sl + 0x1A4) != 0) *(s32*)(sl + 0x1A4) = 0;
        }
        (*(MgrVFn1*)(*(u8**)(mgr + 0x940) + 0x18))(mgr + 0x940, idx);
    }

    cnt = *(s32*)(sl + 0x1A0);
    cur = (cnt > 0 && *(s32*)(sl + 0x198) < cnt) ? *(s32*)(sl + 8 + *(s32*)(sl + 0x198) * 4) : -1;
    if (cur >= 0) {
        cur = (cnt > 0 && *(s32*)(sl + 0x198) < cnt) ? *(s32*)(sl + 8 + *(s32*)(sl + 0x198) * 4) : -1;
        u8* tbl = lbl_805FBBE0 + 0xA58;
        u8* part = *(u8**)(tbl + cur * 4);
        s32 def;
        if (cnt > 0 && *(s32*)(sl + 0x198) >= 0 && *(s32*)(sl + 8 + *(s32*)(sl + 0x198) * 4) >= 0) {
            def = **(s32**)(tbl + *(s32*)(sl + 8 + *(s32*)(sl + 0x198) * 4) * 4);
        } else {
            def = -1;
        }
        *(void**)(*(u8**)mgr + off + 0xC6C) = fn_8003DDE8(part, idx, def, 0, 0);
        (*(MgrVFn2*)(*(u8**)(mgr + 0x940) + 0x14))(mgr + 0x940, idx, part);
        *(s32*)(mgr + off + 0x92C) = *(s32*)(mgr + off + 0x944);
        u8* obj = *(u8**)(sl + 0x1A8);
        if ((*(MgrVFn0*)(*(u8**)obj + 0xC))(obj) != NULL) {
            u8* r = (u8*)(*(MgrVFn0*)(*(u8**)obj + 0xC))(obj);
            *(u32*)(r + 8) &= 0xFFFFFFFDu;
        }
        *(u32*)(*(u8**)(obj + 4) + 0x20) &= 0xFFFFFFFDu;
        *(u32*)(*(u8**)(*(u8**)(sl + 0x1A8) + 4) + 0x20) |= 0x8000;
    } else {
        *(s32*)(sl + 0x1A4) = 0;
        LightningSlotClear(mgr, idx);
        u8* obj = *(u8**)(sl + 0x1A8);
        if ((*(MgrVFn0*)(*(u8**)obj + 0xC))(obj) != NULL) {
            u8* r = (u8*)(*(MgrVFn0*)(*(u8**)obj + 0xC))(obj);
            *(u32*)(r + 8) |= 2;
        }
        *(u32*)(*(u8**)(obj + 4) + 0x20) |= 2;
        *(u32*)(*(u8**)(*(u8**)(sl + 0x1A8) + 4) + 0x20) |= 0x8000;
        if (lbl_805FAF78 != NULL) fn_8001D4FC(lbl_805FAF78);
    }
    lbl_805FAF78[0xA8C] = 0;
    *(s32*)(sl + 0x19C) = *(s32*)(sl + 0x198);
}

// 0x800699E4
// Switches prop `prop` to its animation/state number `state` (table at +0x1A0, count +0x1A4):
// moves the def flags over, binds the new state's anim (looked up in the anim def table, with the
// prop's render object at +0xA0 restarted on it), and when a flagged state is reached runs the
// enter actions / sends the finished event. Does nothing for an out-of-range state.
void fn_800699E4(u8* prop, u32 state, f32 t) {
    typedef u8* (*PropVFn)(void*);
    s32 idx = (s32)state;
    if (idx >= *(s32*)(prop + 0x1A4)) return;

    u8* tbl = *(u8**)(prop + 0x1A0);
    s32 off = idx * 4;
    u8* newState = *(u8**)(tbl + off);
    u8* firstState = *(u8**)tbl;
    u8* def = *(u8**)(prop + 4);
    def[0x36] = def[0x35];
    def = *(u8**)(prop + 4);
    def[0x35] = (u8)state;
    def = *(u8**)(prop + 4);
    *(u32*)(def + 0x20) &= 0xFFFFFFDFu;

    u32 carry = *(u32*)(*(u8**)(prop + 0x9C) + 0xC) & 0xFFFDFADCu;
    if ((carry & 2) && (*(PropVFn*)(*(u8**)prop + 0xC))(prop) != NULL) {
        u8* r = (*(PropVFn*)(*(u8**)prop + 0xC))(prop);
        *(u32*)(r + 8) |= 2;
    }
    def = *(u8**)(prop + 4);
    *(u32*)(def + 0x20) |= carry;
    *(u8**)(prop + 0x9C) = newState;
    // The next mask only keeps bit 0x20, so the follow-up "carry & 2" test of the asm is dead.
    carry = *(u32*)(newState + 0xC) & 0x20;
    def = *(u8**)(prop + 4);
    *(u32*)(def + 0x20) |= carry;
    def = *(u8**)(prop + 4);
    *(u32*)(def + 0x20) |= 0x8000;
    def = *(u8**)(prop + 4);
    *(u32*)(def + 0x20) |= 0x4000;

    u8* obj = *(u8**)(prop + 0x90);
    fn_801DA750(obj, idx);
    def = *(u8**)(prop + 4);
    (*(void (**)(void*, void*, void*, void*))(*(u8**)obj + 0x14))(obj, def, prop + 8, def + 0xC);

    u8* tbl2 = *(u8**)(prop + 0x194);
    if (tbl2 != NULL) *(u8**)(prop + 0x8C) = *(u8**)(tbl2 + off);
    if (*(u8**)(prop + 0x8C) != NULL) fn_801B1778(*(u8**)(prop + 0x8C), 1);

    u8* cur = *(u8**)(prop + 0x9C);
    u32 f15C = *(u32*)(cur + 0xBC);
    *(u32*)(prop + 0x15C) = f15C;
    if (cur[0x1C] != 0) {
        if (*(u8**)(prop + 0xA0) != NULL) {
            u8* defs = *(u8**)(newState + 0x48);
            if (defs == NULL && firstState != NULL) {
                defs = *(u8**)(firstState + 0x48);
                *(u8**)(newState + 0x48) = defs;
            }
            if (defs == NULL) {
                char buf[0x80];
                SysSprintf(buf, "ERROR - attempt to animate prop (%s) without anim def table ...\n", prop + 0x108);
                return;
            }
            u8* anim = fn_801AC184(defs, *(s32*)(newState + 0x10));
            if (anim == NULL) anim = *(u8**)(defs + 0xC) + def[0x39] * 0x44;
            if (anim == NULL) {
                char buf[0x58];
                SysSprintf(buf, "ERROR - unable to find anim [%d] for prop (%s) ...\n", (s32)state, prop + 0x108);
                return;
            }
            u8* clip = (u8*)fn_801AC460(anim, *(u16*)(*(u8**)(prop + 4) + 0x32));
            if (clip == NULL && *(s32*)*(u8**)(prop + 0xA0) != 0) clip = fn_8018C47C(*(u8**)(prop + 0xA0));
            if (clip != NULL) {
                u32 fl = 0;
                if (!(*(u32*)(prop + 0x15C) & 0x80)) {
                    fl |= 8;
                    *(u32*)(*(u8**)(prop + 4) + 0x2C) |= 2;
                }
                if (*(u32*)(*(u8**)(prop + 4) + 0x2C) & 2) fl |= 8;
                fn_8018B2CC(*(u8**)(prop + 0xA0), clip, (s32)(fl | 0x10000), 0, lbl_805FD500, lbl_805FD500);
                *(u32*)(*(u8**)(prop + 0xA0) + 0xF0) = fl;
                *(u32*)(*(u8**)(prop + 0xA0) + 0xF0) &= 0xFFFFFFEFu;
                fn_8018C3D0(*(u8**)(prop + 0xA0));
                u8* ro = *(u8**)(prop + 0xA0);
                f32 saved = *(f32*)(ro + 0xB0);
                *(f32*)(ro + 0xB0) = t * *(f32*)(clip + 0x20);
                u32 old15C = *(u32*)(prop + 0x15C);
                *(u32*)(prop + 0x15C) = old15C & ~1u;
                fn_8006A764(prop, lbl_805FD504);
                *(f32*)(*(u8**)(prop + 0xA0) + 0xB0) = saved;
                *(f32*)(*(u8**)(prop + 0xA0) + 0xB8) = fn_8018BE30(*(u8**)(prop + 0xA0), lbl_805FD4E8);
                if (old15C & 1) *(u32*)(prop + 0x15C) |= 1;
                *(u32*)(*(u8**)(prop + 4) + 0x20) |= 0x8000;
                *(u32*)(*(u8**)(prop + 4) + 0x20) |= 0x4000;
            }
        }
    } else {
        *(u32*)(prop + 0x15C) = f15C | 1;
    }

    u32 w = *(u32*)(*(u8**)(prop + 0x9C) + 0x14);
    if (w != 0) *(u32*)(prop + 0xF0) = w;
    w = *(u32*)(*(u8**)(prop + 0x9C) + 0x18);
    if (w != 0) *(u32*)(prop + 0xF4) = w;
    fn_801DA344(obj, prop);

    def = *(u8**)(prop + 4);
    if (*(u32*)(def + 0x20) & 0x20) {
        if (*(f32*)(def + 0x24) <= lbl_805FD4E8) *(f32*)(def + 0x24) = lbl_805FD508;
        if (idx != 0) {
            if (!(*(u32*)(*(u8**)(prop + 4) + 0x20) & 0x40000)) {
                s32 cnt = *(s32*)(prop + 0x1A4);
                s32 allDone = 1;
                s32 o = off;
                for (s32 i = idx; i < cnt; i++) {
                    u8* s = *(u8**)(*(u8**)(prop + 0x1A0) + o);
                    if (!(*(u32*)(s + 0xC) & 0x20)) allDone = 0;
                    o += 4;
                }
                if (allDone != 0) {
                    u32 ev = *(u32*)(prop + 0xF8);
                    if (ev != 0) EventSendTo(gEventMgr, ev, prop, 0xEDA76DD2u, 0, 0);
                    *(u32*)(*(u8**)(prop + 4) + 0x20) |= 0x40000;
                }
            }
            if (!(*(u32*)(*(u8**)(prop + 4) + 0x20) & 0x08000000)) {
                fn_801ED130(prop + 0x94);
                u8* s = *(u8**)(prop + 0x9C);
                if (s != NULL && ((*(u32*)(s + 0x78) >> 27) & 0xF)) TriggerDoAction(s + 0x78, prop, NULL);
                if ((*(u32*)(prop + 0x48) >> 27) & 0xF) TriggerDoAction(prop + 0x48, prop, NULL);
                *(u32*)(*(u8**)(prop + 4) + 0x20) |= 0x08000000;
            }
        }
    }
}

// Bounds-checked access to the entry table at db+0x2DCDC (0x88 bytes each, count at +0x36454);
// an out-of-range index yields entry 0.
static u8* DbGroup(u8* db, s32 i) {
    s32 c = *(s32*)(db + 0x36454);
    s32 r = i < 0 ? 0 : (i > c ? c : i);
    return (i == r) ? db + 0x2DCDC + i * 0x88 : db + 0x2DCDC;
}

// Same for the table at db+0x13284 (0x1AC bytes each, count at +0x2DCD8).
static u8* DbNode(u8* db, s32 i) {
    s32 c = *(s32*)(db + 0x2DCD8);
    s32 r = i < 0 ? 0 : (i > c ? c : i);
    return (i == r) ? db + 0x13284 + i * 0x1AC : db + 0x13284;
}

// Finds the last node whose id equals `id`; the result may be -1 (then DbNode yields node 0).
static s32 DbFindNode(u8* db, u32 id) {
    s32 j = *(s32*)(db + 0x2DCD8) - 1;
    for (s32 n = j + 1; j > -1 && n != 0; n--) {
        if (id == *(u32*)DbNode(db, j)) break;
        j--;
    }
    return j;
}

// 0x800D4730
// Picks a random node id from group `id` (group entry: id +0, count +0x84, node ids from +4)
// whose node is free (node word +0x54 == 0). Tries the random pick first, then up to `count`
// further picks stepping by 1 % count. Returns the node id, or 0 when none is free / the group
// does not exist.
s32 fn_800D4730(void* dbp, s32 id) {
    u8* db = (u8*)dbp;
    u32 val = 0;
    s32 gi = *(s32*)(db + 0x36454) - 1;
    for (s32 n = gi + 1; gi > -1 && n != 0; n--, gi--) {
        if (id != *(s32*)DbGroup(db, gi)) continue;
        u8* grp = DbGroup(db, gi);
        s32 k = (s32)((f32)RandomU32() * 2.3283064e-10f) % *(s32*)(grp + 0x84);
        val = *(u32*)(DbGroup(db, gi) + 4 + k * 4);
        s32 j = DbFindNode(db, val);
        if (*(s32*)(DbNode(db, j) + 0x54) != 0) {
            val = 0;
            for (s32 tries = 0; tries < *(s32*)(DbGroup(db, gi) + 0x84); tries++) {
                s32 m = *(s32*)(DbGroup(db, gi) + 0x84);
                k += 1 - ((1 / m) * m);
                val = *(u32*)(DbGroup(db, gi) + 4 + k * 4);
                j = DbFindNode(db, val);
                if (*(s32*)(DbNode(db, j) + 0x54) == 0) return (s32)val;
                val = 0;
            }
        }
        return (s32)val;
    }
    return (s32)val;
}

// 0x80155D70
// Loads the identity matrix into the GX state block lbl_80400FF0 (once; lbl_805FB510 = 1.0
// marks it done): resets the 3x4 matrix at +0x58 and snapshots +0xE8 into +0x88. Makes sure the
// current display list has room (ends and submits it, starts the next one) and writes the
// transposed matrix as a GX matrix load (cmd 0x10, reg 0x0B, 12 floats).
void fn_80155D70(void) {
    if (lbl_805FB510 == 1.0f) return;
    f32* m58 = (f32*)(lbl_80400FF0 + 0x58);
    f32* m88 = (f32*)(lbl_80400FF0 + 0x88);
    f32* mE8 = (f32*)(lbl_80400FF0 + 0xE8);
    u8* dls = lbl_80400FF0 + 0x118;
    u8* recs = lbl_80400FF0 + 0xAEC;
    lbl_805FB510 = 1.0f;
    m58[0] = 1.0f;
    m58[1] = 0.0f;
    m58[2] = 0.0f;
    m58[3] = 0.0f;
    m58[4] = 1.0f;
    m58[5] = 0.0f;
    m58[6] = 0.0f;
    m58[7] = 0.0f;
    m58[8] = 1.0f;
    m58[9] = 0.0f;
    m58[10] = 0.0f;
    m58[11] = 0.0f;
    for (s32 i = 0; i < 12; i++) m88[i] = mE8[i];
    if (fn_8015BA30(dls + lbl_805FB4BC * 0x94) + 0x10FC > lbl_805FB4C8) {
        if (lbl_805FB4C0 != 0) {
            RendDLEnd((RendDL*)(dls + lbl_805FB4BC * 0x94));
            s32 rem = 0x24000 - lbl_805FB4C8;
            if (rem != 0) {
                fn_80142ED0(RendDLGetBase((RendDL*)(dls + lbl_805FB4BC * 0x94)) + lbl_805FB4C8, rem);
            }
            if (lbl_805FB4D4 != 0) {
                lbl_805FB4DC = lbl_805FB4DC + 1;
            } else {
                RendThreadSubmitDL((RendDLRecord*)(recs + lbl_805FB4BC * 0x14), lbl_805FB4E0 == 0);
            }
            lbl_805FB4C0 = 0;
            lbl_805FB4D0 = NULL;
            lbl_805FB4C4 = 0;
            lbl_805FB4C8 = 0;
            lbl_805FB4CC = 0;
            if (lbl_805FB4E0 != 0) lbl_805FB4E8 = lbl_805FB4E8 + 1;
            lbl_805FB4BC = lbl_805FB4BC + 1;
            if (lbl_805FB4BC >= 0x11) lbl_805FB4BC = 0;
            if (lbl_805FB4D4 != 0 && !RendHasFreeDL() && lbl_805FB4D4 != 0) {
                RendThreadSync();
                s32 head = lbl_805FB4D8;
                s32 count = lbl_805FB4DC;
                while (count > 0) {
                    RendThreadSubmitDL((RendDLRecord*)(recs + head * 0x14), lbl_805FB4E0 == 0);
                    head = lbl_805FB4D8 + 1;
                    lbl_805FB4D8 = head;
                    if (head >= 0x11) {
                        lbl_805FB4D8 = 0;
                        head = 0;
                    }
                    count = lbl_805FB4DC - 1;
                    lbl_805FB4DC = count;
                }
                lbl_805FB4D4 = 0;
                lbl_805FB4D8 = 0;
                lbl_805FB4DC = 0;
            }
            RendWaitFreeDL();
            RendDLBegin((RendDL*)(dls + lbl_805FB4BC * 0x94), (RendDLRecord*)(recs + lbl_805FB4BC * 0x14));
            lbl_805FB4C0 = 1;
            lbl_805FB4C4 = 0x24000;
            lbl_805FB4C8 = 0x24000;
            lbl_805FB4CC = 0;
            lbl_805FB4D0 = RendDLGetBase((RendDL*)(dls + lbl_805FB4BC * 0x94));
            lbl_805FB4EC = lbl_805FB4EC - 1;
        }
        fn_8015BA30(dls + lbl_805FB4BC * 0x94);
    }
    f32* out = (f32*)(lbl_80400FF0 + 0x10);
    out[0] = m88[0];
    out[1] = m88[3];
    out[2] = m88[6];
    out[3] = m88[9];
    out[4] = m88[1];
    out[5] = m88[4];
    out[6] = m88[7];
    out[7] = m88[10];
    out[8] = m88[2];
    out[9] = m88[5];
    out[10] = m88[8];
    out[11] = m88[11];
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0xB;
    GXWGFifo._u16 = 0;
    for (s32 i = 0; i < 12; i++) GXWGFifo._f32 = out[i];
}

// 0x8016A788
// Samples a compressed key track at its current time. Mode 0 (keyframes): the time ratio
// (+0x24 / header +0x20) is scaled by lbl_805FF5A8 and clamped to 0..0xFFFF, then for each of
// the track's keys (pointer list at +0x28, count +0x10) the quantised position (3 x s16, scaled
// by the header's scale/offset floats at +hdr[0x2C]) and quaternion (4 x s16 / 32768) are
// decoded and, when the next sample is later (+0x16 > +0x6), blended with it through
// fn_80145F9C. The 0x1C-byte result (pos 3 floats + quat 4 floats) goes to the output array at
// +offset(+8). Mode 1: copies one word per key (key+4) to the output array (0xC stride, +8).
void fn_8016A788(u8* a) {
    s32 mode = *(s32*)(a + 4);
    if (mode == 1) {
        u8** keys = (u8**)(a + 0x28);
        u8* out = a + *(s32*)(a + 8);
        for (s32 i = 0; i < *(s32*)(a + 0x10); i++) {
            u8* key = *keys++;
            *(s32*)(out + 8) = *(s32*)(key + 4);
            out += 0xC;
        }
        return;
    }
    if (mode < 0 || mode > 1) return;

    u8* hdr = *(u8**)(a + 0x14);
    s32 frame = 0xFFFF;
    u8** keys = (u8**)(a + 0x28);
    u8* out = a + *(s32*)(a + 8);
    s32 ft = (s32)lrintf(lbl_805FF5A8 * (*(f32*)(a + 0x24) / *(f32*)(hdr + 0x20)));
    s32 ftc = ft & ~(ft >> 31);
    if (ftc <= 0xFFFF) frame = ftc;
    f32* sc = (f32*)(hdr + *(s32*)(hdr + 0x2C));
    f32 sx = sc[0], sy = sc[1], sz = sc[2];
    f32 ox = sc[3], oy = sc[4], oz = sc[5];
    const f32 kQ = 0.00003051851f;
    for (s32 i = 0; i < *(s32*)(a + 0x10); i++) {
        u8* k = *keys++;
        f32 x = (f32) * (s16*)(k + 0x0);
        f32 y = (f32) * (s16*)(k + 0x2);
        f32 z = (f32) * (s16*)(k + 0x4);
        f32 qa[4];
        qa[0] = (f32) * (s16*)(k + 0x8) * kQ;
        qa[1] = (f32) * (s16*)(k + 0xA) * kQ;
        qa[2] = (f32) * (s16*)(k + 0xC) * kQ;
        qa[3] = (f32) * (s16*)(k + 0xE) * kQ;
        f32 pax = x * sx + ox;
        f32 pay = y * sy + oy;
        f32 paz = z * sz + oz;
        s32 dt = (s32) * (u16*)(k + 0x16) - (s32) * (u16*)(k + 0x6);
        if (dt > 0) {
            f32 x2 = (f32) * (s16*)(k + 0x10);
            f32 y2 = (f32) * (s16*)(k + 0x12);
            f32 z2 = (f32) * (s16*)(k + 0x14);
            f32 qb[4];
            qb[0] = (f32) * (s16*)(k + 0x18) * kQ;
            qb[1] = (f32) * (s16*)(k + 0x1A) * kQ;
            qb[2] = (f32) * (s16*)(k + 0x1C) * kQ;
            qb[3] = (f32) * (s16*)(k + 0x1E) * kQ;
            f32 pbx = x2 * sx + ox;
            f32 pby = y2 * sy + oy;
            f32 pbz = z2 * sz + oz;
            f32 t = (f32)(frame - (s32) * (u16*)(k + 0x6)) / (f32)dt;
            f32 inv = 1.0f - t;
            *(f32*)(out + 0x0) = pax * inv + pbx * t;
            *(f32*)(out + 0x4) = pay * inv + pby * t;
            *(f32*)(out + 0x8) = paz * inv + pbz * t;
            f32 q[4];
            fn_80145F9C(q, qa, qb, t);
            *(f32*)(out + 0xC) = q[0];
            *(f32*)(out + 0x10) = q[1];
            *(f32*)(out + 0x14) = q[2];
            *(f32*)(out + 0x18) = q[3];
        } else {
            *(f32*)(out + 0x0) = pax;
            *(f32*)(out + 0x4) = pay;
            *(f32*)(out + 0x8) = paz;
            *(f32*)(out + 0xC) = qa[0];
            *(f32*)(out + 0x10) = qa[1];
            *(f32*)(out + 0x14) = qa[2];
            *(f32*)(out + 0x18) = qa[3];
        }
        out += 0x1C;
    }
}

// Binds track `src` to player `dst`: takes a reference on src, drops the old track (freeing it
// at zero), clamps the length to dst's limit (+0xC), rewinds and re-evaluates the pose.
static void AnimPlayerBind(u8* dst, u8* src) {
    *(s32*)(src + 0x10) = *(s32*)(src + 0x10) + 1;
    u8* old = *(u8**)(dst + 0x14);
    if (old != NULL) {
        s32 c = *(s32*)(old + 0x10) - 1;
        *(s32*)(old + 0x10) = c;
        if (c == 0) fn_8016A2B8(old);
    }
    *(u8**)(dst + 0x14) = src;
    s32 n = *(s32*)(src + 0x18);
    if (*(s32*)(dst + 0xC) < n) n = *(s32*)(dst + 0xC);
    *(s32*)(dst + 0x10) = n;
    *(f32*)(dst + 0x24) = 0.0f;
    *(s32*)(dst + 0x1C) = 0;
    fn_8016A570(dst);
    fn_8016A3F8(dst);
    fn_8016A788(dst);
}

// 0x8018B2CC
// Starts animation `clip` on the animated object `obj` (flags a, extra word b stored at +0xEC,
// blend speed d stored at +0xA8; c is the blend time added to the previous speed). Creates the
// two track players (+0x74/+0x78) on first use. With a current track (state +0x14 of +0x14)
// it either crossfades from it (flag bit 0x10000 clear and the time differs) or restarts it;
// flag bit 0 additionally aligns the new clip's phase to the pose pair at +0xA0/+0xA4 via
// fn_8018AF28. Clears flag 0x10 at the end.
void fn_8018B2CC(u8* obj, u8* clip, s32 a, s32 b, f32 c, f32 d) {
    if (clip == NULL) return;
    u8* state = *(u8**)obj;
    if (state == NULL) return;
    f32 oldSpeed = *(f32*)(obj + 0xA8);
    *(s32*)(obj + 0xEC) = b;
    *(f32*)(obj + 0xDC) = 0.0f;
    *(f32*)(obj + 0xE0) = 0.0f;
    *(f32*)(obj + 0xE4) = 0.0f;
    *(f32*)(obj + 0xC0) = 0.0f;
    *(f32*)(obj + 0xC4) = 0.0f;
    *(f32*)(obj + 0xC8) = 0.0f;
    *(f32*)(obj + 0xCC) = 0.0f;
    *(f32*)(obj + 0xD0) = 0.0f;
    *(f32*)(obj + 0xD4) = 0.0f;
    *(s32*)(obj + 0xF0) = a;
    *(f32*)(obj + 0xA8) = d;
    u8* cur = *(u8**)(*(u8**)(state + 0x14) + 0x14);
    if (*(u8**)(obj + 0x74) == NULL) {
        *(u8**)(obj + 0x74) = fn_8016B50C(NULL, *(s32*)(state + 8));
        *(u8**)(obj + 0x78) = fn_8016B50C(NULL, *(s32*)(*(u8**)obj + 8));
    }
    u8* pl = *(u8**)(obj + 0x74);
    if (pl == NULL || *(u8**)(obj + 0x78) == NULL) return;

    if (cur != NULL) {
        if (*(u32*)(obj + 0xF0) & 0x10000) {
            *(f32*)(obj + 0xAC) = 0.0f;
        } else {
            f32 diff = *(f32*)(obj + 0x9C) - *(f32*)(obj + 0xAC);
            s32 near = 0;
            if (diff > -0.0001f && diff < 0.0001f) near = 1;
            s32 reset = 1;
            if (near != 0) {
                f32 v = *(f32*)(obj + 0x9C);
                s32 tiny = 0;
                if (v > -0.0001f && v < 0.0001f) tiny = 1;
                reset = tiny;
            }
            if (reset) {
                AnimPlayerBind(pl, cur);
                fn_8016B478(*(u8**)(obj + 0x74), *(u8**)(*(u8**)obj + 0x14));
            }
            *(f32*)(obj + 0xAC) = c + oldSpeed;
        }
        *(f32*)(obj + 0x9C) = *(f32*)(obj + 0xAC);
        if (*(u32*)(obj + 0xF0) & 1) {
            f32 pa[3];
            f32 pb[3];
            u8* ea = *(u8**)(*(u8**)obj + 4) + *(s32*)(obj + 0xA0) * 0x30;
            pa[0] = *(f32*)(ea + 0x24);
            pa[1] = *(f32*)(ea + 0x28);
            pa[2] = *(f32*)(ea + 0x2C);
            u8* eb = *(u8**)(*(u8**)obj + 4) + *(s32*)(obj + 0xA4) * 0x30;
            pb[0] = *(f32*)(eb + 0x24);
            pb[1] = *(f32*)(eb + 0x28);
            pb[2] = *(f32*)(eb + 0x2C);
            u8* tp = *(u8**)(*(u8**)obj + 0x14);
            AnimPlayerBind(tp, clip);
            f32 ph = fn_8018AF28(obj, pa, pb);
            tp = *(u8**)(*(u8**)obj + 0x14);
            f32 oldT = *(f32*)(tp + 0x24);
            if (ph < 0.0f) {
                u8* trk = *(u8**)(tp + 0x14);
                while (ph < 0.0f) ph += *(f32*)(trk + 0x20);
            }
            u8* trk2 = *(u8**)(tp + 0x14);
            while (ph > *(f32*)(trk2 + 0x20)) ph -= *(f32*)(trk2 + 0x20);
            *(f32*)(tp + 0x24) = ph;
            *(s32*)(tp + 0x1C) = 0;
            if (ph < oldT) fn_8016A3F8(tp);
            fn_8016A43C(tp);
            fn_8016A788(tp);
            fn_8018A948(obj, 0.0f);
        } else {
            AnimPlayerBind(*(u8**)(*(u8**)obj + 0x14), clip);
        }
    } else {
        AnimPlayerBind(*(u8**)(*(u8**)obj + 0x14), clip);
    }
    *(u32*)(obj + 0xF0) &= 0xFFFFFFEFu;
}

// Sets the three entity update flag pairs used after moving a linked entity and syncs it.
static void LinkSyncEntity(u8* a) {
    u8* pa = *(u8**)(a + 4);
    *(u32*)(*(u8**)(pa + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(pa + 4) + 0x20) |= 0x10000000;
    pa = *(u8**)(a + 4);
    *(u32*)(*(u8**)(pa + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(pa + 4) + 0x20) |= 0x10000000;
}

// 0x801B75AC
// Makes the entity at +4 follow the source entity at +0 (attachment / bone index +0xC; -1 =
// the entity's own transform). Copies position (plus the offset vector +0x10 transformed by the
// bone matrix, and +0x20 added when mode byte +0x1D is 1) and, when +0x1D is 0, the rotation
// (optionally slerped towards the bone rotation when +0x1C is set), then marks the target's
// transform dirty and syncs it.
void fn_801B75AC(void* sp) {
    u8* a = (u8*)sp;
    u8* dstEnt = *(u8**)(a + 4);
    if (dstEnt == NULL) return;
    u8* srcEnt = *(u8**)a;
    if (srcEnt == NULL) return;
    s32 bone = *(s32*)(a + 0xC);
    if (bone > -1) {
        f32 m[12];
        f32 q[4];
        f32 qa[4];
        fn_801B55F4(srcEnt, (u8*)m, bone);
        fn_801443B0(m);
        fn_8014474C((u8*)m, q);
        u8* dq = *(u8**)(*(u8**)(a + 4) + 4);
        qa[0] = *(f32*)(dq + 0xC);
        qa[1] = *(f32*)(dq + 0x10);
        qa[2] = *(f32*)(dq + 0x14);
        qa[3] = *(f32*)(dq + 0x18);
        f32 dot = qa[3] * q[3] + (qa[2] * q[2] + (qa[0] * q[0] + qa[1] * q[1]));
        if (dot > 1.0f) dot = 1.0f;
        if (dot < 0.0f) {
            q[0] = q[0] * -1.0f;
            q[1] = q[1] * -1.0f;
            q[2] = q[2] * -1.0f;
            q[3] = q[3] * -1.0f;
        }
        if (*(u8*)(a + 0x1C) != 0) {
            f32 s[4];
            fn_80145F9C(s, qa, q, 0.05f);
            f32 len2 = s[3] * s[3] + (s[2] * s[2] + (s[0] * s[0] + s[1] * s[1]));
            f32 inv = 1.0f / sqrtf(len2);
            s[0] = s[0] * inv;
            s[1] = s[1] * inv;
            s[2] = s[2] * inv;
            s[3] = s[3] * inv;
            if (*(u8*)(a + 0x1D) == 0) {
                u8* t = *(u8**)(*(u8**)(a + 4) + 4);
                *(f32*)(t + 0xC) = s[0];
                *(f32*)(t + 0x10) = s[1];
                *(f32*)(t + 0x14) = s[2];
                *(f32*)(t + 0x18) = s[3];
            }
        } else if (*(u8*)(a + 0x1D) == 0) {
            u8* t = *(u8**)(*(u8**)(a + 4) + 4);
            *(f32*)(t + 0xC) = q[0];
            *(f32*)(t + 0x10) = q[1];
            *(f32*)(t + 0x14) = q[2];
            *(f32*)(t + 0x18) = q[3];
        }
        f32 ox = *(f32*)(a + 0x10);
        f32 oy = *(f32*)(a + 0x14);
        f32 oz = *(f32*)(a + 0x18);
        f32 px = m[9] + ((m[6] * oz) + ((m[0] * ox) + (m[3] * oy)));
        f32 py = m[10] + ((m[7] * oz) + ((m[1] * ox) + (m[4] * oy)));
        f32 pz = m[11] + ((m[8] * oz) + ((m[2] * ox) + (m[5] * oy)));
        if (*(u8*)(a + 0x1D) == 1) {
            px = px + *(f32*)(a + 0x20);
            py = py + *(f32*)(a + 0x24);
            pz = pz + *(f32*)(a + 0x28);
        }
        u8* t = *(u8**)(*(u8**)(a + 4) + 4);
        *(f32*)(t + 0x0) = px;
        *(f32*)(t + 0x4) = py;
        *(f32*)(t + 0x8) = pz;
        LinkSyncEntity(a);
        EntitySyncTransform(*(u8**)(a + 4), 0);
        u8* ent = *(u8**)(a + 4);
        u8* tr = *(u8**)(ent + 4);
        *(f32*)(ent + 0x18) = *(f32*)(tr + 0x0);
        *(f32*)(ent + 0x1C) = *(f32*)(tr + 0x4);
        *(f32*)(ent + 0x20) = *(f32*)(tr + 0x8);
        return;
    }
    if (*(u8*)(a + 0x1D) == 0) {
        u8* st = *(u8**)(srcEnt + 4);
        u8* dt = *(u8**)(dstEnt + 4);
        *(f32*)(dt + 0xC) = *(f32*)(st + 0xC);
        *(f32*)(dt + 0x10) = *(f32*)(st + 0x10);
        *(f32*)(dt + 0x14) = *(f32*)(st + 0x14);
        *(f32*)(dt + 0x18) = *(f32*)(st + 0x18);
    }
    u8* sp2 = *(u8**)(*(u8**)a + 4);
    u8* dp2 = *(u8**)(*(u8**)(a + 4) + 4);
    *(f32*)(dp2 + 0x0) = *(f32*)(sp2 + 0x0);
    *(f32*)(dp2 + 0x4) = *(f32*)(sp2 + 0x4);
    *(f32*)(dp2 + 0x8) = *(f32*)(sp2 + 0x8);
    LinkSyncEntity(a);
    EntitySyncTransform(*(u8**)(a + 4), 0);
}


}  // extern "C"
