// Mixed batch: Brain head movement callbacks, catch mini-game lane spawner, FireShot / FireWarningShot /
// FrameFxRippleAtChild script callbacks, UI anim player helpers, archive stream pump and a GX state setup.
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"
#include "engine/rend.h"
extern "C" {
#include "game/script_callbacks.h"
}

struct Seeker;
struct Mover;

extern "C++" {
void  PredictTargetPos(u8* self, Seeker* seeker, Mover* target, Vec3* out, f32 dt);   // 0x8000C604 (game_spawn.cpp)
u8*   fn_801B55A8(u8* ent);                                           // 0x801B55A8 (locator count, returned as an integer value)
}

extern "C" {
void  CatchObject_Launch(void* obj, const f32 target[3], f32 speed);  // 0x80030414
void  CatchObject_Release(void* obj, s32 a);                          // 0x80030A98
void  CatchObject_SetLane(void* obj, void* lane);                     // 0x80030348
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);      // 0x80145EE8
u32   RandomU32(void);                                                // 0x80143460
s32   RaySceneTest(void* ray);                                        // 0x80070458 (returns the difficulty index, ignores its argument)
void* SpawnHelper(void* owner, s32 arg);                              // 0x8000C478
s32   fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6);   // 0x80065200
void  fn_8008139C(u8* self, void* val);                               // 0x8008139C
void  fn_8014DE68(void);                                              // 0x8014DE68
void  fn_8014E3EC(s32 n);                                             // 0x8014E3EC
s32   fn_80151644(s32 a, s32 b, u8* quad);                            // 0x80151644
void  fn_80155724(u8* m);                                             // 0x80155724
s32   fn_8015BA30(void* p);                                           // 0x8015BA30
void  fn_8018D370(const char* fmt, ...);                              // 0x8018D370 (debug log)
u32   fn_8019552C(u8* list, s32 index);                               // 0x8019552C
s32   fn_8019555C(u8* list);                                          // 0x8019555C
s32   fn_801AA730(u8* dst, u8* src);                                  // 0x801AA730
void  fn_801AA760(u8* dst, u8* src);                                  // 0x801AA760
void  fn_801AA7D8(void* track, s32 loop, s32 rev, f32 pos);           // 0x801AA7D8
void  fn_801AAF14(void* a, f32 oldPos, f32 newPos, s32 loop, s32 rev);   // 0x801AAF14
void  fn_801AB4C4(void* dst, void* src);                              // 0x801AB4C4
s32   fn_801AB748(u8* p, f32 v);                                      // 0x801AB748
f32   fn_801AB7F0(u8* a);                                             // 0x801AB7F0
f32   fn_801AB818(u8* a);                                             // 0x801AB818
void  fn_801ABC50(void* h);                                           // 0x801ABC50
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                         // 0x801B55F4
u8*   fn_801D6D78(void);                                              // 0x801D6D78 (camera record: 3x3 matrix at +0, translation at +0x24)
s32   fn_801E4FE0(u8* obj, s32 a, u8* buf, u32 size, s32 b);          // 0x801E4FE0
BOOL  fn_801E51AC(u8* obj, u32* handle);                              // 0x801E51AC
s32   fn_801E6EB8(u8* p);                                             // 0x801E6EB8
s32   fn_801E6F08(u8* p);                                             // 0x801E6F08
s32   fn_801E6F78(u8* p);                                             // 0x801E6F78
void  fn_801E93EC(u8* list, u8* entry);                               // 0x801E93EC
void* fn_801D40E0(s32 n);                                             // 0x801D40E0 (alloc)
void* fn_801EB94C(void* p);                                           // 0x801EB94C
s32   fn_8023C168(OSThread* t);                                       // 0x8023C168
void  fn_8023D3C8(s32 hi, u32 lo);                                    // 0x8023D3C8
f64   fn_8033F1F4(f64 x);                                             // 0x8033F1F4 (acos)

extern u8  gGame[];                 // 0x803B53A0
extern u8  gWorldState[];           // 0x803E6D00
extern u8  lbl_80348950[];          // 0x80348950 (0x20-byte records, u32 at +0)
extern u8  lbl_80348A10[];          // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32 lbl_80348AD0;            // 0x80348AD0
extern f32 lbl_80346670[];          // 0x80346670
extern f32 lbl_80346710[];          // 0x80346710
extern f32 lbl_80346720[];          // 0x80346720
extern f32 lbl_80346730[];          // 0x80346730
extern f32 lbl_80346740[];          // 0x80346740
extern f32 lbl_80346680[];          // 0x80346680
extern s32 lbl_80346690[];          // 0x80346690
extern f32 lbl_803466C0[];          // 0x803466C0
extern f32 lbl_803466D0[];          // 0x803466D0
extern const char lbl_803838A0[];   // 0x803838A0 (string)
extern const char lbl_803838D0[];   // 0x803838D0 (string)
extern s32 lbl_8040ADF0[8];         // 0x8040ADF0
extern s32 lbl_8040AE10[8];         // 0x8040AE10
extern u8  lbl_8035B4B8[];          // 0x8035B4B8 (power-up effect vtables, one per type)
extern u8  lbl_8035B4E8[];          // 0x8035B4E8
extern u8  lbl_8035B518[];          // 0x8035B518
extern u8  lbl_8035B548[];          // 0x8035B548
extern u8  lbl_8035B570[];          // 0x8035B570
extern u8  lbl_8035B5A0[];          // 0x8035B5A0
extern u8  lbl_8035B5D0[];          // 0x8035B5D0
extern u8  lbl_8035B600[];          // 0x8035B600
extern u8  lbl_8035B638[];          // 0x8035B638
extern u8  lbl_8035B668[];          // 0x8035B668
extern u8  lbl_8035B698[];          // 0x8035B698 (base effect vtable)
extern u8  lbl_8048A8B4[];          // 0x8048A8B4 (archive handle table)
extern u8  lbl_8048A8E0[];          // 0x8048A8E0 (thread block: thread at +0x4A0, stack at +0x7C8, buffer at +0x27E0)
extern s32 lbl_805FB4B0;            // 0x805FB4B0
extern s32 lbl_805FB4C4;            // 0x805FB4C4 (display-list capacity)
extern s32 lbl_805FB4CC;            // 0x805FB4CC
extern u8* lbl_805FB4D0;            // 0x805FB4D0 (display-list write cursor)
extern u8  lbl_805FB50C;            // 0x805FB50C (GX shadow flags byte)
extern u8* lbl_805FB68C;            // 0x805FB68C (UI pool A)
extern u8* lbl_805FB690;            // 0x805FB690 (UI pool B)
extern u8* lbl_805FB694;            // 0x805FB694 (UI pool C)
extern u8  lbl_805FB8C8[];          // 0x805FB8C8 (zlib stream-active flag byte)
extern u8* lbl_805FB8D8;            // 0x805FB8D8 (effect pool: 0xFF entries of 0x80 bytes)
extern f32 lbl_80600020;            // 0x80600020 (0.0f)
}

extern "C++" u8  gIgcMode;          // 0x805FAF64
extern "C++" s32 gViewportIndex;    // 0x805FB4BC
extern "C++" s32 gViewportCount;    // 0x805FB4C0
extern "C++" s32 gFrameBudget;      // 0x805FB4C8
extern "C++" u8  sRendDLs[];        // 0x80401108 (display-list records, 0x94 bytes each)

// Locator hash of the head's walk-path locator (name not resolved by tools/hashes.py).
static const u32 kLocatorHeadPath = 0xAAE1A370u;

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// One XF/CP load (opcode 0x10, two halfwords, one data word).
static void GxXfWrite(u16 count, u16 addr, u32 value) {
    GXWGFifo._u8  = 0x10;
    GXWGFifo._u16 = count;
    GXWGFifo._u16 = addr;
    GXWGFifo._u32 = value;
}

// Finds the world locator with the given hash (records of 0x20 bytes at +0x28), or NULL.
static u8* FindLocator(u32 hash) {
    u8* data = *(u8**)gWorldState;
    u32 n = *(u32*)(data + 0x24);
    s32 found = -1;
    for (u32 i = 0; i < n; i++) {
        if (*(u32*)(*(u8**)(data + 0x28) + i * 0x20) == hash) {
            found = (s32)i;
            break;
        }
    }
    if (found >= 0 && found < *(s32*)(data + 0x24)) return *(u8**)(data + 0x28) + found * 0x20;
    return NULL;
}

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtPos(f32 x) {
    return x > 0.0f ? sqrtf(x) : 0.0f;  // original: frsqrte with two Newton steps
}

static inline f32 Clamp1(f32 v) {
    if (v < -1.0f) return -1.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline void MarkMoved(u8* def) {
    *(u32*)(def + 0x20) |= 0x8000;
    *(u32*)(def + 0x20) |= 0x10000000;
    *(u32*)(def + 0x20) |= 0x4000;
    *(u32*)(def + 0x20) |= 0x10000000;
}

// Turn angle (in half turns, 2 +/- acos of the heading) the head faces when moving from `pos` along the
// path locator's vertical axis; `pos` is the new position, `loc` the locator record.
static f32 HeadFacingAngle(const f32* pos, const u8* loc) {
    f32 ax = pos[0] - *(const f32*)(loc + 0x8);
    f32 ay = pos[1] - *(const f32*)(loc + 0xC);
    f32 az = 0.0f;
    f32 bx = 0.0f, by = 1.0f, bz = 0.0f;   // reference direction: +Y
    f32 dot = (bz * az) + ((bx * ax) + (by * ay));
    f32 lenB = SqrtPos((bz * bz) + ((bx * bx) + (by * by)));
    f32 lenA = SqrtPos((az * az) + ((ax * ax) + (ay * ay)));
    f32 prod = lenB * lenA;
    f32 angle = 0.0f;
    if (!(prod <= 0.0f)) {
        f32 c = dot / prod;
        c = Clamp1(Clamp1(c));
        angle = 0.63661975f * (f32)fn_8033F1F4((f64)c);
    }
    if (pos[0] < 0.0f) return 2.0f + angle;
    return 2.0f - angle;
}

// Writes the new position and facing quaternion into the object's transform and flags it as moved.
static void ApplyHeadTransform(u8* obj, const f32* pos, f32 turn) {
    Vec3 axis = { 0.0f, 0.0f, 1.0f };
    Quat q;
    QuatFromAxisAngle(&q, &axis, turn);
    u8* def = *(u8**)(obj + 4);
    *(f32*)(def + 0x0) = pos[0];
    *(f32*)(def + 0x4) = pos[1];
    *(f32*)(def + 0x8) = pos[2];
    MarkMoved(*(u8**)(obj + 4));
    def = *(u8**)(obj + 4);
    *(Quat*)(def + 0xC) = q;
    MarkMoved(*(u8**)(obj + 4));
}

// Resolves the locator that a script reference selects: by locator index (bit 0, via fn_801B55F4) or by
// the slot table at obj+0x98 (bit 1, 0xC8-byte slots whose 12 floats at +0x38 are the locator).
// Returns 0 when the reference selects nothing.
static s32 ResolveLocator(u8* obj, u8* ref, f32 loc[12]) {
    u16 flags = *(u16*)ref;
    s32 idx;
    if (flags & 1) idx = *(u8*)(ref + 2);
    else idx = -1;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(obj)) {
            fn_801B55F4(obj, (u8*)loc, idx);
            return 1;
        }
        return 0;
    }
    s32 idx2;
    if (flags & 2) idx2 = *(u8*)(ref + 2);
    else idx2 = -1;
    if (idx2 != -1) {
        u8* src = *(u8**)(obj + 0x98) + idx2 * 0xC8;
        for (s32 i = 0; i < 12; i++) loc[i] = *(f32*)(src + 0x38 + i * 4);
        return 1;
    }
    return 0;
}

// Picks the shot template id for the shooter and spawns the projectile entity from the target's pool.
static u8* SpawnShot(u8* obj, u8* ref, u8* target) {
    s32 spawnArg = 0;
    s32 refId = *(s32*)ref;
    s32 objFlags = *(s32*)(*(u8**)(obj + 4) + 0x1C);
    if (objFlags & 0x20) {
        spawnArg = *(s32*)(obj + 0xA14);
    } else if (objFlags & 0x40) {
        *(s32*)(obj + 0x178) = refId;
        spawnArg = *(s32*)(obj + 0x18C);
        *(u8*)(obj + 0x190) = (u8)(*(u8*)(obj + 0x190) + 1);
    }
    if (spawnArg == 0) return NULL;
    return (u8*)SpawnHelper(target, spawnArg);
}

// Handle (low byte = index, high byte = generation) to its effect-pool record, or NULL.
static u8* PoolRecord(u16 h) {
    u8 idx = (u8)h;
    if (idx < 0xFF && ((h >> 8) & 0xFF) != 0) return lbl_805FB8D8 + ((u32)idx << 7);
    return NULL;
}

// Runs the per-record callback (slot +0x6C) once with a zero time step, as the effect pool does when it
// finishes a chain: marks the record running (0x40), calls it, then marks it done (0x80).
static void RunPoolRecord(u8* cur, f32 z) {
    typedef s32 (*StepFn)(u8*, s32, f32);
    cur[0x7F] = 0x40;
    if (*(void**)(cur + 0x6C) != NULL) {
        *(f32*)(cur + 0x70) = *(f32*)(cur + 0x70) + z;
        cur[0x7E] = (u8)(*(StepFn*)(cur + 0x6C))(cur, *(s32*)(cur + 0x68), z);
    }
    cur[0x7F] = 0x80;
}

extern "C" {

// 0x80195A58
// Draws one textured quad record: reserves 0x60 bytes in the display list, copies the quad into it, and
// (when `rec` is given) programs the GX copy/texture BP registers for it; without `rec` it only resets
// the GX shadow flag. `m` is the matrix handed to fn_80155724. The unused first argument is the GX context.
void fn_80195A58(void* gx, void* rec_, u8* m, u8* quad) {
    u8* rec = (u8*)rec_;
    (void)gx;
    if (lbl_805FB4CC < 0x60) {
        fn_8014E3EC(0x60);
    }
    s32 cap = lbl_805FB4C4;
    s32 left = lbl_805FB4CC;
    u8* base = lbl_805FB4D0;
    lbl_805FB4C4 = cap + 0x60;
    u8* dst = base + cap;
    lbl_805FB4CC = left - 0x60;
    memcpy(dst, quad, 0x60);
    if (rec != NULL) {
        if (gViewportCount != 0 && fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x10C0 > gFrameBudget) {
            fn_8014DE68();
        }
        if (!(lbl_805FB50C & 2)) {
            u8 flags = lbl_805FB50C | 2;
            lbl_805FB50C = flags;
            if (flags & 1) {
                GxBpWrite(0xFE00000F);
                GxBpWrite(1);
                GxXfWrite(0, 0x103F, 1);
                GxBpWrite(0xFE003C00);
                GxBpWrite(0x400);
                GxXfWrite(0, 0x1010, 0x441);
                GxBpWrite(0x283C0000);
                GxBpWrite(0xC008F2A4);
                GxBpWrite(0xC108E6F0);
                GxBpWrite(0xC208F80F);
                GxBpWrite(0xC308F070);
            } else {
                GxBpWrite(0xFE00000F);
                GxBpWrite(1);
                GxXfWrite(0, 0x103F, 1);
                GxBpWrite(0xFE003C00);
                GxBpWrite(0);
                GxXfWrite(0, 0x100E, 0x442);
                GxXfWrite(0, 0x1010, 0x442);
                GxBpWrite(0x28380040);
                GxBpWrite(0xC008F8AF);
                GxBpWrite(0xC108F2F0);
            }
        }
        GxBpWrite(0xFE000060);
        GxBpWrite(((u32)*(u8*)(rec + 0x16) << 5) | 0x80000000u);
        if ((u32)lbl_8040ADF0[0] != *(u32*)(rec + 0x1C) || lbl_8040AE10[0] != 0) {
            u32 cfg = lbl_80348AD0;
            lbl_805FB4B0 = lbl_805FB4B0 + 1;
            lbl_8040ADF0[0] = (s32)*(u32*)(rec + 0x1C);
            lbl_8040AE10[0] = 0;
            u8 idx = *(u8*)(rec + 0x17);
            GxBpWrite((((u32)*(u8*)(rec + 0x13) - 1) & 0xF) << 12 | 0x84000000u);
            s32 second = *(s32*)(lbl_80348A10 + idx * 0x20);
            GxBpWrite(((u32)*(u16*)(rec + 0x8) - 1) |
                      ((((u32)*(u16*)(rec + 0xA) - 1) << 10) | (((u32)*(u8*)(rec + 0x14) << 20) | 0x88000000u)));
            GxBpWrite((*(u32*)(lbl_80348950 + idx * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
            if (second >= 0) {
                GxBpWrite(((u32)second >> 5) | 0x900D0000u | 0x8000u);
            } else {
                GxBpWrite(0x90000000u);
            }
            GxBpWrite(((*(u32*)(rec + 0x1C) >> 5) & 0xFFFFFF) | 0x94000000u);
            if (*(u8*)(rec + 0x11) != 0) {
                u32 f = (cfg >> 9) & 0x3FF;
                GxBpWrite(((f | 0x98000000u) & ~0x3FC00u) | (((u32)*(u8*)(rec + 0x15) << 10) & 0x3FC00u));
                GxBpWrite(0xFEFFFF00u);
                GxBpWrite(0x0F000000u);
                GxBpWrite(((*(u32*)(rec + 0x20) >> 5) & 0xFFFFFF) | 0x64000000u);
                GxBpWrite(((f | 0x65000000u) & ~0x1FC00u) |
                          ((((s32)*(u8*)(rec + 0x11) >> 1) << 10) & 0x1FC00u));
                GxBpWrite(0xFEFFFF00u);
                GxBpWrite(0x0F000000u);
            }
        }
        GxBpWrite(((u32)*(u16*)(rec + 0x8) - 1) | 0x30000000u);
        GxBpWrite(((u32)*(u16*)(rec + 0xA) - 1) | 0x31000000u);
    } else if (lbl_805FB50C & 2) {
        u8 old = lbl_805FB50C;
        lbl_805FB50C = old & ~2;
        if (old & 1) {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0, 0x1010, 0x441);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008F2A4);
            GxBpWrite(0xC108E6F0);
        } else {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0, 0x100E, 0x442);
            GxXfWrite(0, 0x1010, 0x442);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008FFFA);
            GxBpWrite(0xC108FFD0);
        }
    }
    fn_80155724(m);
    fn_80151644(0x98, 4, dst);
}

// 0x801AB56C
// Advances a UI animation player by dt: moves the playhead (+0x24) by speed (+0x28), wraps it when the
// loop flag (+0x1C bit 0) is set or clamps it at the ends and reports the end. Pushes the new time to
// each of the five track slots (+0x4..+0x14); on a loop wrap restarts the tracks, and notifies the
// listener (+0x18) when the time changed. Returns 1 when a non-looping animation reached its end.
s32 fn_801AB56C(void* h, f32 dt) {
    u8* obj = (u8*)h;
    s32 wrapped = 0;
    s32 ended = 0;
    u32 flags = *(u32*)(obj + 0x1C);
    if (flags & 2) return 0;
    f32 speed = *(f32*)(obj + 0x28);
    s32 rev = speed < 0.0f;
    f32 oldPos = *(f32*)(obj + 0x24);
    s32 loop = flags & 1;
    f32 pos = oldPos + dt * speed;
    *(f32*)(obj + 0x24) = pos;
    if (pos < 0.0f || pos > *(f32*)(obj + 0x20)) {
        if (loop == 0) {
            ended = 1;
            if (rev == 0) {
                *(f32*)(obj + 0x24) = *(f32*)(obj + 0x20);
            } else {
                *(f32*)(obj + 0x24) = 0.0f;
            }
        } else {
            if (rev == 0) {
                wrapped = 1;
                *(f32*)(obj + 0x24) = *(f32*)(obj + 0x24) - *(f32*)(obj + 0x20);
            } else {
                wrapped = 1;
                *(f32*)(obj + 0x24) = *(f32*)(obj + 0x24) + *(f32*)(obj + 0x20);
            }
            if (*(u32*)(obj + 0x1C) & 4) {
                for (s32 i = 0; i < 5; i++) {
                    u8* slot = *(u8**)(obj + 4 + i * 4);
                    if (slot != NULL) {
                        u8* list = *(u8**)(slot + 4);
                        u8* last = (u8*)(uintptr_t)fn_8019552C(list, fn_8019555C(list) - 1);
                        u8* first = (u8*)(uintptr_t)fn_8019552C(*(u8**)(slot + 4), 0);
                        f32 v[4];
                        fn_801AA760((u8*)v, last);
                        fn_801AA730(first, (u8*)v);
                    }
                }
            }
        }
    }
    for (s32 i = 0; i < 5; i++) {
        u8* slot = *(u8**)(obj + 4 + i * 4);
        if (slot != NULL) {
            if (wrapped != 0) {
                *(s32*)(slot + 0xC) = 0;
                *(s32*)(slot + 0x10) = 0;
            }
            fn_801AA7D8(slot, loop, rev, *(f32*)(obj + 0x24));
        }
    }
    if (oldPos != *(f32*)(obj + 0x24) && *(s32*)(obj + 0x18) != 0) {
        fn_801AAF14(*(void**)(obj + 0x18), oldPos, *(f32*)(obj + 0x24), loop, rev);
    }
    return ended;
}

// 0x801ABAF4
// Creates a UI anim player as a copy of the description: allocates the player from pool A, a track from
// pool B for each used slot (five at +0x4) and one from pool C for the optional listener (+0x18),
// releasing the player again if a pool is exhausted. Starts it at the end when its length is
// non-negative, otherwise at the description's start time.
void* fn_801ABAF4(void* anim) {
    u8* src = (u8*)anim;
    u8* dst = (u8*)fn_801EB94C(lbl_805FB68C);
    if (dst == NULL) {
        fn_8018D370(lbl_803838A0);
        return NULL;
    }
    for (s32 i = 0; i < 5; i++) {
        if (*(s32*)(src + 4 + i * 4) != 0) {
            u8* track = (u8*)fn_801EB94C(lbl_805FB690);
            if (track == NULL) {
                fn_8018D370(lbl_803838D0);
                fn_801ABC50(dst);
                return NULL;
            }
            *(u8**)(dst + 4 + i * 4) = track;
        } else {
            *(u8**)(dst + 4 + i * 4) = NULL;
        }
    }
    if (*(s32*)(src + 0x18) != 0) {
        u8* listener = (u8*)fn_801EB94C(lbl_805FB694);
        if (listener == NULL) {
            fn_8018D370(lbl_803838D0);
            fn_801ABC50(dst);
            return NULL;
        }
        *(u8**)(dst + 0x18) = listener;
    } else {
        *(u8**)(dst + 0x18) = NULL;
    }
    fn_801AB4C4(dst, src);
    if (fn_801AB818(src) >= 0.0f) {
        fn_801AB748(dst, 0.0f);
    } else {
        fn_801AB748(dst, fn_801AB7F0(src));
    }
    return dst;
}

// 0x801E6FAC
// Pumps one streamed archive read. Types 1 and 2 run a worker thread (copy / inflate); otherwise
// the request is fed through the handle table: when the handle (+0x50) is ready for the first time
// the buffer is set up and a streamed-inflate thread started, when the thread finishes the next chunk
// is queued. With `wait` set the call blocks (sleeping 100 ms steps) until the worker is done.
// Returns 1 when the request is complete, 0 while it is still running.
s32 fn_801E6FAC(u8* p, s32 wait, f32 t) {
    (void)t;
    u8* blk = lbl_8048A8E0;
    if (*(s32*)(p + 4) == 0) return 1;
    s32 mode = *(s32*)p;
    if (mode == 2 || mode == 1) {
        if (*(s32*)p == 1 && *(void**)(p + 0x84) == NULL) {
            OSThread* th = (OSThread*)(blk + 0x10 + 0x490);
            *(OSThread**)(p + 0x84) = th;
            OSCreateThread(th, (OSThreadFunc)fn_801E6F78, p, blk + 0x7C8 + 0x2000, 0x2000, 0x1F, 1);
            OSResumeThread(th);
        }
        if (*(s32*)p == 2 && *(void**)(p + 0x84) == NULL) {
            OSThread* th = (OSThread*)(blk + 0x10 + 0x490);
            *(OSThread**)(p + 0x84) = th;
            OSCreateThread(th, (OSThreadFunc)fn_801E6EB8, p, blk + 0x7C8 + 0x2000, 0x2000, 0x1F, 1);
            OSResumeThread(th);
        }
        OSThread* th = *(OSThread**)(p + 0x84);
        if (th != NULL) {
            if (wait != 0) {
                while (fn_8023C168(th) == 0) {
                    u64 ticks = (u64)(OSGetBusClock() / 4 / 1000) * 100;
                    fn_8023D3C8((s32)(ticks >> 32), (u32)ticks);
                }
            }
            if (fn_8023C168(*(OSThread**)(p + 0x84)) != 0) {
                *(void**)(p + 0x84) = NULL;
                return 1;
            }
            return 0;
        }
    }
    if (*(s32*)(p + 0x44) == 0) {
        u32 h = *(u32*)(p + 0x50);
        if (fn_801E51AC(lbl_8048A8B4, &h) != 0 && *(s32*)p == 0) return 1;
        return 0;
    }
    OSThread* th = *(OSThread**)(p + 0x84);
    if (th == NULL) {
        if (*(void**)(p + 0x78) == NULL && *(s32*)(p + 0x14) == 0) {
            u32 limit = *(u32*)(p + 0x60);
            u32 off = *(u32*)(p + 0x7C);
            u32 len = off < limit ? off : limit;
            u8* buf = blk + 0x27E0;
            buf[off] = 0xA5;
            buf[off + 1] = 0xA6;
            buf[off + 2] = 0xA7;
            buf[off + 3] = 0xA8;
            lbl_805FB8C8[0] = 1;
            buf[off + 4] = 0xA9;
            *(u8**)(p + 0x78) = buf;
            *(s32*)(p + 0x50) = fn_801E4FE0(lbl_8048A8B4, *(s32*)(p + 0x54), buf, (len + 0x1F) & 0xFFFFFFE0u,
                                             *(s32*)(p + 0x68));
        }
        u32 h = *(u32*)(p + 0x50);
        if (fn_801E51AC(lbl_8048A8B4, &h) != 0) {
            u32 n = *(u32*)(p + 0x7C);
            u32 rem = *(u32*)(p + 0x60) - *(u32*)(p + 0x14);
            *(u8**)(p + 0xC) = *(u8**)(p + 0x78);
            if (!(n < rem)) n = rem;
            OSThread* nt = (OSThread*)(blk + 0x10 + 0x490);
            *(u32*)(p + 0x10) = n;
            *(s32*)(p + 0x1C) = *(s32*)(p + 0x64) - *(s32*)(p + 0x20);
            *(OSThread**)(p + 0x84) = nt;
            OSCreateThread(nt, (OSThreadFunc)fn_801E6F08, p, blk + 0x7C8 + 0x2000, 0x2000, 0x1F, 1);
            OSResumeThread(nt);
        }
        return 0;
    }
    if (wait != 0) {
        while (fn_8023C168(th) == 0) {
            u64 ticks = (u64)(OSGetBusClock() / 4 / 1000) * 100;
            fn_8023D3C8((s32)(ticks >> 32), (u32)ticks);
        }
    }
    if (fn_8023C168(*(OSThread**)(p + 0x84)) != 0) {
        *(void**)(p + 0x84) = NULL;
        if (*(void**)(p + 0x78) == NULL) return 1;
        u32 n = *(u32*)(p + 0x7C);
        u32 rem = *(u32*)(p + 0x60) - *(u32*)(p + 0x14);
        if (!(n < rem)) n = rem;
        *(s32*)(p + 0x50) = fn_801E4FE0(lbl_8048A8B4, *(s32*)(p + 0x54), *(u8**)(p + 0x78), (n + 0x1F) & 0xFFFFFFE0u,
                                         *(s32*)(p + 0x68) + (s32)(*(u32*)(p + 0x14) >> 11));
    }
    return 0;
}

// 0x801E7E28
// Releases an effect-pool list (head handle +0, tail +2, count +4, flags +6, owner word +8): when its
// first record belongs to it, runs the pending records to completion and unlinks the finished ones,
// then clears the list fields.
void fn_801E7E28(void* p) {
    u8* obj = (u8*)p;
    if (lbl_805FB8D8 != NULL) {
        u8* head = PoolRecord(*(u16*)obj);
        if (head != NULL && *(u8**)(head + 0x74) == obj && *(u16*)(obj + 4) != 0) {
            f32 z = lbl_80600020;
            for (u8* cur = PoolRecord(*(u16*)obj); cur != NULL; cur = PoolRecord(*(u16*)(cur + 0x78))) {
                if (cur[0x7F] != 0x80) RunPoolRecord(cur, z);
            }
            u16 fl = *(u16*)(obj + 6) | 2;
            *(u16*)(obj + 6) = fl;
            if (!(fl & 1) && (fl & 2)) {
                z = lbl_80600020;
                u8* cur = PoolRecord(*(u16*)obj);
                while (cur != NULL) {
                    u8* next = PoolRecord(*(u16*)(cur + 0x78));
                    u8 st = cur[0x7F];
                    if (st == 0x40 && st != 0x80) RunPoolRecord(cur, z);
                    if (st == 0x80) fn_801E93EC(obj, cur);
                    cur = next;
                }
                *(u16*)(obj + 6) = (u16)(*(u16*)(obj + 6) & 0xFFFD);
            }
        }
        // Look up this list's own handle among the 0xFF pool records.
        u8* pool = lbl_805FB8D8;
        u16 mine = 0xFFFF;
        for (s32 i = 0; i < 0xFF; i++) {
            u8* r = pool + i * 0x80;
            if (r[0x7F] != 0 && (u8)*(u16*)(r + 0x7C) == i && *(u8**)(r + 0x74) == obj) {
                mine = *(u16*)(pool + (i << 7) + 0x7C);
                break;
            }
        }
        // Walks the chain from the owner's head toward its own record (the walk has no side effects).
        u8* rec = PoolRecord(mine);
        if (rec != NULL && rec[0x7F] != 0) {
            u16* owner = *(u16**)(rec + 0x74);
            if (owner != NULL) {
                u8* self = PoolRecord(*(u16*)(rec + 0x7C));
                u8* cur = NULL;
                if (self != NULL) cur = PoolRecord(*owner);
                if (self != NULL) {
                    while (cur != NULL && cur != self) {
                        u16 hh = *(u16*)(cur + 0x78);
                        u8 ii = (u8)hh;
                        s32 ok = 0;
                        if (ii < 0xFF && ((hh >> 8) & 0xFF) != 0 && *(u16*)(pool + ((u32)ii << 7) + 0x7C) == hh) ok = 1;
                        cur = ok ? pool + ((hh << 7) & 0x7F80) : NULL;
                    }
                }
            }
        }
    }
    *(u16*)(obj + 6) = 0;
    *(u16*)(obj + 4) = 0;
    *(u16*)(obj + 0) = 0xFFFF;
    *(u16*)(obj + 2) = 0xFFFF;
    *(s32*)(obj + 8) = 0;
}

// 0x80084768
// Script callback BrainHeadCrazyWalk: moves the head along a sine-shaped offset (amplitude from the
// difficulty table, period lbl_803466D0) for one frame step, turns it to face away from the path
// locator and returns 1 while the walk is running; at the end of the period resets the phase and returns 0.
u32 /*0x06*/ BrainHeadCrazyWalk(u32 /*0x58*/ a0) {
    u8* obj = (u8*)(uintptr_t)a0;
    f32 dt = *(f32*)(gWorldState + 0xC);
    f32 period = lbl_803466D0[RaySceneTest(NULL)];
    f32 amp[3];
    amp[0] = 0.0f;
    amp[1] = 0.0f;
    amp[2] = 0.0f;
    amp[0] = lbl_803466C0[RaySceneTest(NULL)];
    f32 s1 = (f32)sinf(1.5707964f * (0.63661975f * (6.2831855f * (*(f32*)(obj + 0x268) / period))));
    f32 phase = *(f32*)(obj + 0x268) + dt;
    f32 d1[3];
    d1[0] = amp[0] * s1;
    d1[1] = amp[1] * s1;
    d1[2] = amp[2] * s1;
    *(f32*)(obj + 0x268) = phase;
    f32 s2 = (f32)sinf(1.5707964f * (0.63661975f * (6.2831855f * (phase / period))));
    f32 d2[3];
    d2[0] = amp[0] * s2;
    d2[1] = amp[1] * s2;
    d2[2] = amp[2] * s2;
    f32 step[3];
    step[0] = d2[0] - d1[0];
    step[1] = d2[1] - d1[1];
    step[2] = d2[2] - d1[2];
    u8* def = *(u8**)(obj + 4);
    f32 pos[3];
    pos[0] = *(f32*)(def + 0x0) + step[0];
    pos[1] = *(f32*)(def + 0x4) + step[1];
    pos[2] = *(f32*)(def + 0x8) + step[2];
    u8* loc = FindLocator(kLocatorHeadPath);
    f32 turn = HeadFacingAngle(pos, loc);
    ApplyHeadTransform(obj, pos, turn);
    if (*(f32*)(obj + 0x268) >= period) {
        *(f32*)(obj + 0x268) = 0.0f;
        return 0;
    }
    return 1;
}

// 0x800840C4
// Script callback BrainHeadMove: steps the head toward its target (+0x274) at a speed that scales with the
// head's damage (difficulty tables lbl_80346670/80/90), faces it away from the path locator and updates
// the attached object's (virtual slot +0x10) scale field from the speed.
void BrainHeadMove(u32 /*0x58*/ a0) {
    u8* obj = (u8*)(uintptr_t)a0;
    u8* def = *(u8**)(obj + 4);
    f32 dx = *(f32*)(obj + 0x274) - *(f32*)(def + 0x0);
    f32 dy = *(f32*)(obj + 0x278) - *(f32*)(def + 0x4);
    f32 dz = *(f32*)(obj + 0x27C) - *(f32*)(def + 0x8);
    f32 dt = *(f32*)(gWorldState + 0xC);
    f32 len = SqrtPos((dz * dz) + ((dx * dx) + (dy * dy)));
    f32 dir[3];
    dir[0] = dx / len;
    dir[1] = dy / len;
    dir[2] = dz / len;
    f32 slow = lbl_80346670[RaySceneTest(NULL)];
    f32 speed = lbl_80346680[RaySceneTest(NULL)];
    s32 hp = *(s32*)(*(u8**)(obj + 0x264) + 0x334);
    s32 cnt = lbl_80346690[RaySceneTest(NULL)];
    s32 n = cnt;
    if (hp < cnt) n = hp;
    if (cnt > 0) {
        speed = slow + ((f32)n * ((speed - slow) / (f32)cnt));
    }
    f32 stepLen = speed * dt;
    if (!(stepLen < len)) stepLen = len;
    f32 mv[3];
    mv[0] = dir[0] * stepLen;
    mv[1] = dir[1] * stepLen;
    mv[2] = dir[2] * stepLen;
    def = *(u8**)(obj + 4);
    f32 pos[3];
    pos[0] = *(f32*)(def + 0x0) + mv[0];
    pos[1] = *(f32*)(def + 0x4) + mv[1];
    pos[2] = *(f32*)(def + 0x8) + mv[2];
    u8* loc = FindLocator(kLocatorHeadPath);
    f32 turn = HeadFacingAngle(pos, loc);
    ApplyHeadTransform(obj, pos, turn);
    f32 base = lbl_80346670[0];
    typedef u8* (*VFn)(u8*);
    u8* attached = (*(VFn*)(*(u8**)obj + 0x10))(obj);
    if (attached != NULL) {
        *(f32*)(attached + 0xB0) = *(f32*)(obj + 0x288) * (speed / base);
    }
}

// 0x8002F208
// Catch mini-game: spawns a new batch of falling objects for `lane` (left lane when lane == g+0x90).
// Rolls a random type (weights depend on the lane), takes the first free object of that type's array
// and throws it, together with the first free pool object, towards the lane; the pool object's index
// is stored in the first one (+0x10, -1 if none) and that pool object is then released.
void CatchMinigame_SpawnInLane(void* g_, void* lane, f32 delay) {
    u8* g = (u8*)g_;
    (void)delay;
    f32 hi = 40.0f;
    s32 left = lane == (void*)(g + 0x90);
    f32 hiLimit = 20.0f + 40.0f;
    if (left == 0) {
        hiLimit += 10.0f;
        hi = 40.0f - 10.0f;
    }
    u8** objs = NULL;
    u8* counters = g + left * 0x14;
    u8* state = counters + 0xF10;
    while (objs == NULL) {
        f32 roll = 0.0f + (100.0f * (2.3283064e-10f * (f32)RandomU32()));
        if (roll > hiLimit) break;
        if (roll > hi) {
            s32 n = *(s32*)(counters + 0xF14);
            if (n < 3) {
                objs = (u8**)(g + 0xE44);
                memset(state, 0, 0x14);
                *(s32*)(counters + 0xF14) = n + 1;
            }
        } else {
            switch (RandomU32() % 3) {
            case 0: {
                s32 n = *(s32*)(counters + 0xF18);
                if (n < 3) {
                    objs = (u8**)(g + 0xE6C);
                    memset(state, 0, 0x14);
                    *(s32*)(counters + 0xF18) = n + 1;
                }
                break;
            }
            case 1: {
                s32 n = *(s32*)(counters + 0xF1C);
                if (n < 3) {
                    objs = (u8**)(g + 0xE94);
                    memset(state, 0, 0x14);
                    *(s32*)(counters + 0xF1C) = n + 1;
                }
                break;
            }
            case 2: {
                s32 n = *(s32*)(counters + 0xF20);
                if (n < 3) {
                    objs = (u8**)(g + 0xEBC);
                    memset(state, 0, 0x14);
                    *(s32*)(counters + 0xF20) = n + 1;
                }
                break;
            }
            }
        }
    }
    if (objs == NULL) return;

    f32 poolX = 64.0f + 650.0f;
    if (objs == (u8**)(g + 0xE44)) {
        poolX += *(f32*)(*(u8**)(*(u8**)(g + 0xE44) + 4) + 0x74) * 0.5f;
    }
    f32 y = left != 0 ? 140.0f : 336.0f;
    f32 speed = left != 0 ? 2.5f : 4.0f;
    s32 k = 0;
    s32 poolIdx;
    {
        f32 target[3] = { 650.0f, y, 0.0f };
        for (s32 c = 0; c < 10; c++) {
            u8* o = objs[k];
            if (*(u8*)(o + 0x20) == 0 && *(s32*)(o + 4) != 0) {
                CatchObject_SetLane(objs[k], lane);
                f32 t[3] = { target[0], target[1], target[2] };
                CatchObject_Launch(objs[k], t, speed);
                break;
            }
            k++;
        }
    }
    poolIdx = 0;
    {
        f32 target[3] = { poolX, y, 0.0f };
        for (s32 c = 0; c < 10; c++) {
            u8* o = *(u8**)(g + 0xEE4 + poolIdx * 4);
            if (*(u8*)(o + 0x20) == 0 && *(s32*)(o + 4) != 0) {
                u8* pg = g + poolIdx * 4;
                CatchObject_SetLane(*(u8**)(pg + 0xEE4), lane);
                f32 t[3] = { target[0], target[1], target[2] };
                CatchObject_Launch(*(u8**)(pg + 0xEE4), t, speed);
                break;
            }
            poolIdx++;
        }
    }
    if (poolIdx > 9) poolIdx = -1;
    *(s32*)(objs[k] + 0x10) = poolIdx;
    if (poolIdx > -1 && poolIdx < 10) {
        u8* o = *(u8**)(g + poolIdx * 4 + 0xEE4);
        if (o != NULL) CatchObject_Release(o, 1);
    }
}

// 0x800EA6A8
// Script callback FireShot: fires a projectile from the shooter's locator at the target with a random
// spread (a3, scaled by the difficulty) and a lead time a4 (also scaled by the difficulty).
void FireShot(void* a0, u32 /*0x30*/ a1, u32 /*0x38*/ a2, f32 a3, f32 a4) {
    u8* obj = (u8*)a0;
    u8* ref = (u8*)(uintptr_t)a1;
    u8* target = (u8*)(uintptr_t)a2;
    f32 spread = a3;
    f32 lead = a4;
    f32 loc[12];
    if (gIgcMode != 0) return;
    if (ResolveLocator(obj, ref, loc) == 0) return;
    u8* shot = SpawnShot(obj, ref, target);
    if (shot == NULL) return;
    s32 diff = *(s32*)(gGame + 0xC45C);
    if (!(diff >= 0 && diff <= 3)) diff = 1;
    switch (diff) {
    case 0: lead = 1.0f + (1.25f * (lead - 1.0f)); break;
    case 1: lead = 1.0f + (lead - 1.0f); break;
    case 2: lead = 1.0f + (0.75f * (lead - 1.0f)); break;
    case 3: lead = 1.0f + (0.66f * (lead - 1.0f)); break;
    }
    f32 tgt[3];
    PredictTargetPos(target, (Seeker*)obj, *(Mover**)(shot + 0x248), (Vec3*)tgt, lead);
    diff = *(s32*)(gGame + 0xC45C);
    if (!(diff >= 0 && diff <= 3)) diff = 1;
    switch (diff) {
    case 0: spread *= 1.25f; break;
    case 2: spread *= 0.75f; break;
    case 3: spread *= 0.66f; break;
    }
    f32 ox = (spread * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - spread;
    f32 oy = (spread * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - spread;
    f32 oz = (spread * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - spread;
    if (oz < 0.0f) oz = oz * -1.0f;
    f32 tx = tgt[0] + ox;
    f32 ty = tgt[1] + oy;
    f32 tz = tgt[2] + oz;
    f32 dx = tx - loc[9];
    f32 dy = ty - loc[10];
    f32 dz = tz - loc[11];
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    f32 dir[3];
    if (len2 == 0.0f) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        dir[0] = dx * inv;
        dir[1] = dy * inv;
        dir[2] = dz * inv;
    }
    fn_80065200(shot, (u8*)&loc[9], dir, obj, 0, 0, 0);
}

// 0x800EAF14
// Script callback FireWarningShot: like FireShot but aims at a random point on a sphere-ish offset
// (radius a3, always from above) around the predicted target position, so the shot misses on purpose.
void FireWarningShot(void* a0, u32 /*0x30*/ a1, u32 /*0x38*/ a2, f32 a3, f32 a4) {
    u8* obj = (u8*)a0;
    u8* ref = (u8*)(uintptr_t)a1;
    u8* target = (u8*)(uintptr_t)a2;
    f32 loc[12];
    if (gIgcMode != 0) return;
    if (ResolveLocator(obj, ref, loc) == 0) return;
    u8* shot = SpawnShot(obj, ref, target);
    if (shot == NULL) return;
    f32 tgt[3];
    PredictTargetPos(target, (Seeker*)obj, *(Mover**)(shot + 0x248), (Vec3*)tgt, a4);
    f32 rx = (1.0f * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - 1.0f;
    f32 ry = (1.0f * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - 1.0f;
    (void)RandomU32();   // third roll is discarded
    f32 rz = -1.0f;
    f32 rlen2 = (rz * rz) + ((rx * rx) + (ry * ry));
    f32 off[3];
    if (rlen2 == 0.0f) {
        off[0] = 0.0f;
        off[1] = 0.0f;
        off[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(rlen2);  // original: frsqrte with two Newton steps
        off[0] = rx * inv;
        off[1] = ry * inv;
        off[2] = rz * inv;
    }
    f32 ox = off[0] * a3;
    f32 oy = off[1] * a3;
    f32 oz = off[2] * a3;
    f32 tx = tgt[0] + ox;
    f32 ty = tgt[1] + oy;
    f32 tz = tgt[2] + oz;
    f32 dx = tx - loc[9];
    f32 dy = ty - loc[10];
    f32 dz = tz - loc[11];
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    f32 dir[3];
    if (len2 == 0.0f) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        dir[0] = dx * inv;
        dir[1] = dy * inv;
        dir[2] = dz * inv;
    }
    fn_80065200(shot, (u8*)&loc[9], dir, obj, 0, 0, 0);
}


// 0x800861A0
// Script callback BrainPowerUpEnact: builds the effect node for the power-up type at +0x284 (type 10
// picks 1 or 2 at random; 0..9 each have their own node class and parameters from the difficulty
// tables), hands it to the owner (+0x264) through fn_8008139C, and advances the type to the next of 3.
void BrainPowerUpEnact(u32 /*0x59*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    s32 type = *(s32*)(self + 0x284);
    u8* owner = *(u8**)(self + 0x264);
    if (type == 10) type = (s32)(RandomU32() & 1) + 1;
    u8* node = NULL;
    if ((u32)type <= 9) {
        switch (type) {
        case 0:
            node = (u8*)fn_801D40E0(0x14);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)(node + 4) = owner;
                *(s32*)(node + 8) = -1;
                *(u8**)node = lbl_8035B638;
                *(f32*)(node + 0x10) = -1.0f;
                *(s32*)(node + 8) = 0;
                *(f32*)(node + 0xC) = 100000.0f;
            }
            break;
        case 1:
            node = (u8*)fn_801D40E0(0x10);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(f32*)(node + 0xC) = 0.0f;
                *(u8**)(node + 4) = owner;
                *(u8**)node = lbl_8035B4E8;
                *(s32*)(node + 8) = 1;
                *(f32*)(node + 0xC) = lbl_80346720[RaySceneTest(NULL)];
            }
            break;
        case 2:
            node = (u8*)fn_801D40E0(0x14);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(f32*)(node + 0xC) = 0.0f;
                *(u8**)(node + 4) = owner;
                *(u8**)node = lbl_8035B600;
                *(s32*)(node + 0x10) = 2;
                *(s32*)(node + 8) = 2;
                *(f32*)(node + 0xC) = lbl_80346710[RaySceneTest(NULL)];
            }
            break;
        case 3:
            node = (u8*)fn_801D40E0(0x14);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)(node + 4) = owner;
                *(u8**)node = lbl_8035B668;
                *(s32*)(node + 0x10) = 2;
                *(s32*)(node + 8) = 3;
                *(f32*)(node + 0xC) = 100000.0f;
            }
            break;
        case 4:
            node = (u8*)fn_801D40E0(0x24);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)(node + 4) = owner;
                *(s32*)(node + 8) = -1;
                *(u8**)node = lbl_8035B5D0;
                *(s32*)(node + 0x10) = 3;
                *(s32*)(node + 0x14) = 0;
                *(s32*)(node + 0x18) = 0;
                *(f32*)(node + 0x1C) = 0.1f;
                *(s32*)(node + 8) = 4;
                *(f32*)(node + 0xC) = 100000.0f;
                *(f32*)(node + 0x20) = 0.1f;
            }
            break;
        case 5:
            node = (u8*)fn_801D40E0(0x14);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)(node + 4) = owner;
                *(u8**)node = lbl_8035B5A0;
                *(u8*)(node + 0x10) = 0;
                *(s32*)(node + 8) = 5;
                *(f32*)(node + 0xC) = 3.0f;
                *(u8*)(node + 0x10) = *(u8*)(owner + 0xC0);
            }
            break;
        case 6:
            node = (u8*)fn_801D40E0(0x14);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)(node + 4) = owner;
                *(s32*)(node + 8) = -1;
                *(u8**)node = lbl_8035B570;
                *(f32*)(node + 0x10) = 1.5f;
                *(s32*)(node + 8) = 6;
                *(f32*)(node + 0xC) = 3.0f;
                *(f32*)(owner + 0xC4) = 1.5f * *(f32*)(owner + 0xC4);
            }
            break;
        case 7:
            node = (u8*)fn_801D40E0(0x14);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)(node + 4) = owner;
                *(s32*)(node + 8) = -1;
                *(u8**)node = lbl_8035B548;
                *(f32*)(node + 0x10) = 3.0f;
                *(s32*)(node + 8) = 7;
                *(f32*)(node + 0xC) = 100000.0f;
            }
            break;
        case 8:
            node = (u8*)fn_801D40E0(0x14);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)(node + 4) = owner;
                *(u8**)node = lbl_8035B518;
                *(f32*)(node + 0x10) = 0.0f;
                *(s32*)(node + 8) = 8;
                *(f32*)(node + 0xC) = 15.0f;
                f32 lo = lbl_80346730[RaySceneTest(NULL)];
                f32 hi = lbl_80346740[RaySceneTest(NULL)];
                *(f32*)(node + 0x10) = lo + ((-hi) + ((hi - (-hi)) * (2.3283064e-10f * (f32)RandomU32())));
            }
            break;
        case 9:
            node = (u8*)fn_801D40E0(0x18);
            if (node != NULL) {
                *(u8**)node = lbl_8035B698;
                *(u8**)node = lbl_8035B4B8;
                *(s32*)(node + 8) = 1;
                *(f32*)(node + 0xC) = 100000.0f;
                *(u8**)(node + 4) = owner;
                *(s32*)(node + 0x10) = 0;
                *(s32*)(node + 0x10) = *(s32*)(*(u8**)(owner + 0x1F4) + 0x264);
                *(s32*)(node + 0x14) = *(s32*)(*(u8**)(owner + 0x1F4) + 0x268);
            }
            break;
        }
    }
    fn_8008139C(*(u8**)(self + 0x264), node);
    *(s32*)(self + 0x284) = (*(s32*)(self + 0x284) + 1) % 3;
}
}  // extern "C"

extern "C" {

// 0x800E7D78
// Script callback FrameFxRippleAtChild: takes the locator a1 selects, transforms its position (loc[9..11])
// by the inverse of the camera matrix (fn_801D6D78: 3x3 at +0, translation at +0x24), maps it to
// screen coordinates (0..1, y flipped) and starts the ripple frame effect there with strength a2.
void FrameFxRippleAtChild(void* a0, u32 /*0x30*/ a1, f32 a2) {
    u8* obj = (u8*)a0;
    u8* ref = (u8*)(uintptr_t)a1;
    f32 loc[12];
    if (ResolveLocator(obj, ref, loc) == 0) return;
    u8* cam = fn_801D6D78();
    f32 m0 = *(f32*)(cam + 0x0), m1 = *(f32*)(cam + 0x4), m2 = *(f32*)(cam + 0x8);
    f32 m3 = *(f32*)(cam + 0xC), m4 = *(f32*)(cam + 0x10), m5 = *(f32*)(cam + 0x14);
    f32 m6 = *(f32*)(cam + 0x18), m7 = *(f32*)(cam + 0x1C), m8 = *(f32*)(cam + 0x20);
    f32 n0 = (m2 * m2) + ((m0 * m0) + (m1 * m1));
    f32 n1 = (m5 * m5) + ((m3 * m3) + (m4 * m4));
    f32 i0 = 1.0f / n0;
    f32 i1 = 1.0f / n1;
    f32 a0v = m0 * i0;   // inverse of the (row-orthogonal) matrix: each element scaled by 1 / |row|^2
    f32 a3v = m3 * i0;
    f32 a1v = m1 * i1;
    f32 a4v = m4 * i1;
    f32 a6v = m6 * i0;
    f32 a7v = m7 * i1;
    f32 n2 = (m8 * m8) + ((m6 * m6) + (m7 * m7));
    f32 i2 = 1.0f / n2;
    f32 a2v = m2 * i2;
    f32 a5v = m5 * i2;
    f32 a8v = m8 * i2;
    f32 ntx = -*(f32*)(cam + 0x24);
    f32 nty = -*(f32*)(cam + 0x28);
    f32 ntz = -*(f32*)(cam + 0x2C);
    f32 lx = loc[9], ly = loc[10], lz = loc[11];
    f32 tx = (a2v * ntz) + ((a0v * ntx) + (a1v * nty));
    f32 ty = (a5v * ntz) + ((a3v * ntx) + (a4v * nty));
    f32 tz = (a8v * ntz) + ((a6v * ntx) + (a7v * nty));
    f32 vx = tx + ((a2v * lz) + ((a0v * lx) + (a1v * ly)));
    f32 vy = ty + ((a5v * lz) + ((a3v * lx) + (a4v * ly)));
    (void)tz;
    f32 sy = 1.0f - (0.5f + (vy / ((f32)VideoGetHeight(&gVideo) - 1.0f)));
    f32 sx = 0.5f + (vx / ((f32)VideoGetWidth(&gVideo) - 1.0f));
    if (sx < 0.0f) {
        sx = 0.0f;
    } else if (sx > 1.0f) {
        sx = 1.0f;
    }
    if (sy < 0.0f) {
        sy = 0.0f;
    } else if (sy > 1.0f) {
        sy = 1.0f;
    }
    FrameFxRipple(sx, sy, a2);
}

}  // extern "C"
