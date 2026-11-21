// Brain toss minigame functions in 0x8008xxxx (brain chunk spawning, aim/swing helpers, ctor/dtor).
#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);                                // 0x80209E70
u32   RandomU32(void);                                                                        // 0x80143460
s32   RaySceneTest(void* ray);                                                                   // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                                                          // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                                               // 0x801F53B0
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);                // 0x801C80E0
u8*   fn_80062CE0(u8* p, s32 del);                                                            // 0x80062CE0 (base dtor)
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);                                         // 0x800690D4 (base ctor)
u8*   fn_8006FBDC(void);                                                                      // 0x8006FBDC
void  fn_800712B4(void);                                                                      // 0x800712B4
void  fn_800712B8(void);                                                                      // 0x800712B8
u8*   fn_8007F900(u8* mgr, s32 type);                                          // 0x8007F900
void  fn_80085C34(u8* p, s32 n);                                                              // 0x80085C34
void  fn_801C849C(u8* mgr, u8* item);                                                         // 0x801C849C
void* fn_801D40E0(s32 n);                                                                     // 0x801D40E0
void* fn_80209814(void* v);                                                                   // 0x80209814
f32   fn_8033F1FC(f32 y, f32 x);                                                              // 0x8033F1FC
void  operator_delete(void* p);                                                               // 0x801D426C

extern "C++" void* gEventMgr;            // 0x805FB6C8
extern u8    gWorldState[];        // 0x803E6D00
extern u8    lbl_8035B068[];       // 0x8035B068 (vtable)
extern s32   lbl_80346520[];       // 0x80346520
extern s32   lbl_80346530[];       // 0x80346530
extern s32   lbl_80346540[];       // 0x80346540
extern s32   lbl_80346550[];       // 0x80346550
extern f32   lbl_80346560[];       // 0x80346560
extern f32   lbl_803464E4[];       // 0x803464E4
extern f32   lbl_803464F0;         // 0x803464F0
extern s32   lbl_80346750[];       // 0x80346750
extern s32   lbl_80346760[];       // 0x80346760
extern f32   lbl_80346770[];       // 0x80346770
extern f32   lbl_80346780[];       // 0x80346780
extern f32   lbl_80346790[];       // 0x80346790
extern f32   lbl_803C6990[];       // 0x803C6990 (gravity vector)
extern u8    lbl_8047C760[];       // 0x8047C760 (effect manager)
extern void* lbl_805FBBAC;         // 0x805FBBAC (voice SFX handle)
}

typedef u8*  (*GetSubFn)(u8*);
typedef void (*SetFlagFn)(u8*, s32);
typedef void* (*SpawnFn)(void*, u32, const f32*, void*);

static const f32 kHalfPi = 1.5707964f;
static const f32 kTwoOverPi = 0.63661975f;
static const f32 kU32ToUnit = 2.3283064e-10f;  // 1 / 2^32

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtGuard(f32 v) {
    return v <= 0.0f ? 0.0f : sqrtf(v);
}

// Finds the first entry of the gWorldState list (count +0x24, base +0x28, 0x20 bytes each) whose
// first word is 0xAAE1A370; NULL if none.
static inline u8* FindTurntableLocator(void) {
    u8* list = *(u8**)gWorldState;
    s32 count = *(s32*)(list + 0x24);
    u8* base = *(u8**)(list + 0x28);
    s32 idx = -1;
    for (s32 i = 0; i < count; i++) {
        if (*(u32*)(base + i * 0x20) == 0xAAE1A370u) {
            idx = i;
            break;
        }
    }
    if (idx >= 0 && idx < *(s32*)(list + 0x24)) return base + idx * 0x20;
    return NULL;
}

// Pose setup shared by the brain chunk ctor/dtor/reset: position (0, 0, 100) and flag bits.
static inline void BrainChunkResetPose(u8* obj) {
    u8* p = *(u8**)(obj + 0x4);
    *(f32*)(p + 0x0) = 0.0f;
    *(f32*)(p + 0x4) = 0.0f;
    *(f32*)(p + 0x8) = 100.0f;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x8000;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x10000000;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x4000;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x10000000;
}

extern "C" {

// 0x800809C8
// Picks a brain chunk from the brain toss manager (random roll against the +0xBC threshold),
// activates it above the player's position (+5 up) and sends it event 0x274858A5.
u32 /*0x06*/ BrainTossReloadBrainChunk(void) {
    u8* mgr = fn_8006FBDC();
    f32 roll = kU32ToUnit * (f32)RandomU32();
    u8* obj = fn_8007F900(mgr, roll < *(f32*)(mgr + 0xBC));
    if (obj == NULL) return 0;
    if (*(u8*)(obj + 0x268) != 1) {
        *(u8*)(obj + 0x268) = 1;
        if ((*(GetSubFn*)(*(u8**)obj + 0xC))(obj) != NULL) {
            u8* sub = (*(GetSubFn*)(*(u8**)obj + 0xC))(obj);
            *(u32*)(sub + 0x8) |= 2;
        }
        u8* node = *(u8**)(obj + 0x4);
        *(u32*)(node + 0x20) |= 2;
        *(s32*)(obj + 0x2B4) = 0;
    }
    u8* target = *(u8**)(*(u8**)(mgr + 0x1F4) + 0x4);
    f32 x = *(f32*)(target + 0x0) + 0.0f;
    f32 y = *(f32*)(target + 0x4) + 0.0f;
    f32 z = *(f32*)(target + 0x8) + 5.0f;
    *(f32*)(obj + 0x274) = 0.0f;
    u8* p = *(u8**)(obj + 0x4);
    *(f32*)(p + 0x0) = x;
    *(f32*)(p + 0x4) = y;
    *(f32*)(p + 0x8) = z;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x8000;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x10000000;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x4000;
    p = *(u8**)(obj + 0x4);
    *(u32*)(p + 0x20) |= 0x10000000;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(SetFlagFn*)(*(u8**)obj + 0x14))(obj, 1);
    ScopedLockDtor(&lock, -1);
    EventSend(gEventMgr, obj, NULL, 0x274858A5u, 0, 0, 1, 0);
    return 1;
}

// 0x80080C5C
// Returns the active chunk (type 1, flag +0x268 set, model hash 0xB4C6463E) closest to the
// position stored in the object at +0x29C, or NULL. Chunks are listed at +0x1F8, count at +0x298.
u8* fn_80080C5C(u8* self) {
    u8* best = NULL;
    s32 count = *(s32*)(self + 0x298);
    u8* ref = *(u8**)(*(u8**)(self + 0x29C) + 0x4);
    f32 rx = *(f32*)(ref + 0x0);
    f32 ry = *(f32*)(ref + 0x4);
    f32 rz = *(f32*)(ref + 0x8);
    f32 bestDist = 100000.0f;
    for (s32 i = 0; i < count; i++) {
        u8* chunk = *(u8**)(self + 0x1F8 + i * 4);
        s32 type = *(s32*)(chunk + 0x264);
        u8* a = *(u8**)(*(u8**)(chunk + 0xA4) + 0x0);
        if (type != 1 || *(u8*)(chunk + 0x268) == 0) continue;
        s32 idx = *(s32*)(a + 0x4);
        u8* ent = NULL;
        if (idx > -1) ent = *(u8**)(*(u8**)(a + 0x0) + 0x0) + idx * 0x30;
        u32 hash = ent != NULL ? *(u32*)(ent + 0x4) : 0;
        if (hash != 0xB4C6463Eu) continue;
        u8* pos = *(u8**)(chunk + 0x4);
        f32 dx = *(f32*)(pos + 0x0) - rx;
        f32 dy = *(f32*)(pos + 0x4) - ry;
        f32 dz = *(f32*)(pos + 0x8) - rz;
        f32 dist = SqrtGuard(dz * dz + (dx * dx + dy * dy));
        if (dist < bestDist) {
            bestDist = dist;
            best = chunk;
        }
    }
    return best;
}

// 0x80080DC0
// Adds a score-like delta to the meter at +0x328 (clamped 0..100): bumps the per-slot counter,
// rolls four table lookups, picks one by the chunk's slot (+0x2B8) and multiplies by +0x26C.
void fn_80080DC0(u8* self, u8* chunk) {
    u8* counters = *(s32*)(chunk + 0x264) == 1 ? self + 0x340 : self + 0x330;
    s32 slot = *(s32*)(chunk + 0x2B8);
    s32* counter = (s32*)(counters + slot * 4);
    *counter += 1;
    s32 v0 = lbl_80346520[RaySceneTest(NULL)];
    s32 v1 = lbl_80346530[RaySceneTest(NULL)];
    s32 v2 = lbl_80346540[RaySceneTest(NULL)];
    s32 v3 = lbl_80346550[RaySceneTest(NULL)];
    s32 amount = 0;
    s32 isBrainBody;
    u8* held = *(u8**)(chunk + 0x2B4);
    if (held == NULL) {
        isBrainBody = 0;
    } else {
        u8* sub = *(u8**)(held + 0x9C);
        u32 hash = sub != NULL ? *(u32*)(sub + 0x8) : 0;
        isBrainBody = hash == 0xB2E0CC76u;  // "brain_bodyA"
    }
    switch (*(s32*)(chunk + 0x2B8)) {
    case 1:
        if (isBrainBody) {
            if (*(s32*)(chunk + 0x264) == 1) {
                amount = v3;
            } else {
                fn_80085C34(*(u8**)(self + 0x2A0), 1);
                amount = v2;
            }
        }
        break;
    case 2:
        if (*(s32*)(chunk + 0x264) != 1) amount = v0;
        break;
    case 3:
        if (isBrainBody && *(s32*)(chunk + 0x264) != 1) amount = v1;
        break;
    default:
        break;
    }
    amount *= *(s32*)(chunk + 0x26C);
    f32 meter = *(f32*)(self + 0x328) + (f32)amount * lbl_80346560[RaySceneTest(NULL)];
    f32 out = 0.0f;
    if (meter >= 0.0f) {
        out = 100.0f;
        if (meter <= 100.0f) out = meter;
    }
    *(f32*)(self + 0x328) = out;
    if (amount > 0) {
        fn_800712B8();
    } else if (amount < 0) {
        fn_800712B4();
    }
}

// 0x80081004
// Computes a target position at the turntable locator, offset backwards along a direction that is
// blended between the vectors at +0x9C (cos) and +0x90 (sin) of obj by the angle t, scaled by
// table[idx] * obj[+0xB4]. Writes (x, y, 0) to out.
void fn_80081004(u8* out, u8* obj, s32 idx, f32 t) {
    *(f32*)(out + 0x8) = 0.0f;
    f32 dist = lbl_803464E4[idx] * *(f32*)(obj + 0xB4);
    *(f32*)(out + 0x4) = 0.0f;
    *(f32*)(out + 0x0) = 0.0f;
    f32 c = cosf(kHalfPi * (kTwoOverPi * t));
    f32 cx = *(f32*)(obj + 0x9C) * c;
    f32 cy = *(f32*)(obj + 0xA0) * c;
    f32 cz = *(f32*)(obj + 0xA4) * c;
    f32 s = sinf(kHalfPi * (kTwoOverPi * t));
    f32 dx = *(f32*)(obj + 0x90) * s + cx;
    f32 dy = *(f32*)(obj + 0x94) * s + cy;
    f32 dz = *(f32*)(obj + 0x98) * s + cz;
    u8* loc = FindTurntableLocator();
    f32 px = dx * dist;
    f32 py = dy * dist;
    f32 pz = dz * dist;
    f32 ox = *(f32*)(loc + 0x8) - px;
    f32 oy = *(f32*)(loc + 0xC) - py;
    f32 oz = *(f32*)(loc + 0x10) - pz;
    *(f32*)(out + 0x8) = oz;
    *(f32*)(out + 0x0) = ox;
    *(f32*)(out + 0x4) = oy;
    *(f32*)(out + 0x8) = 0.0f;
}

// 0x80081210
// Angle (scaled by 2/pi * pi/2) of the vector from p to the turntable locator, measured with the
// two axes at +0x90 and +0x9C of obj.
f32 fn_80081210(u8* obj, u8* p) {
    u8* loc = FindTurntableLocator();
    f32 dx = *(f32*)(loc + 0x8) - *(f32*)(p + 0x0);
    f32 ax = *(f32*)(obj + 0x90);
    f32 ay = *(f32*)(obj + 0x94);
    f32 dy = *(f32*)(loc + 0xC) - *(f32*)(p + 0x4);
    f32 bx = *(f32*)(obj + 0x9C);
    f32 by = *(f32*)(obj + 0xA0);
    f32 az = *(f32*)(obj + 0x98);
    f32 bz = *(f32*)(obj + 0xA4);
    f32 dz = *(f32*)(loc + 0x10) - *(f32*)(p + 0x8);
    f32 ang = fn_8033F1FC(dz * az + (dx * ax + dy * ay), dz * bz + (dx * bx + dy * by));
    return kHalfPi * (kTwoOverPi * ang);
}

// 0x80081404
// Counts down the timer at +0x388; when it expires re-rolls which of the 6 slots (mask byte at
// +0x384) are lit, resets the timer, then spawns an effect and sound for lit slots and frees the
// effect handle (+0x36C) of unlit ones.
void fn_80081404(u8* obj, f32 dt) {
    f32 timer = *(f32*)(obj + 0x388) - dt;
    *(f32*)(obj + 0x388) = timer;
    if (timer >= 0.0f) return;
    f32 f31 = lbl_80346780[RaySceneTest(NULL)];
    f32 f30 = lbl_80346770[RaySceneTest(NULL)];
    f32 f29 = lbl_80346790[RaySceneTest(NULL)];
    s32 minCount = lbl_80346750[RaySceneTest(NULL)];
    s32 maxCount = lbl_80346760[RaySceneTest(NULL)];
    s32 mask = 0;
    if (*(u8*)(obj + 0x384) == 0) {
        s32 count = 0;
        s32 want = minCount + (s32)(RandomU32() % (u32)((maxCount - minCount) + 1));
        while (count < want) {
            s32 r = (s32)(RandomU32() % 6u);
            if (!((mask >> r) & 1)) {
                count++;
                mask = (u8)(mask | (1 << r));
            }
        }
        *(f32*)(obj + 0x388) = f30;
    } else {
        f32 neg = -f29;
        *(f32*)(obj + 0x388) = f31 + (neg + (f29 - neg) * (kU32ToUnit * (f32)RandomU32()));
    }
    *(u8*)(obj + 0x384) = (u8)mask;
    for (s32 i = 0; i < 6; i++) {
        if ((*(u8*)(obj + 0x384) >> i) & 1) {
            u8* node = *(u8**)(obj + 0x354 + i * 4);
            f32 pos[4];
            pos[0] = *(f32*)(node + 0x8);
            pos[1] = *(f32*)(node + 0xC);
            pos[2] = *(f32*)(node + 0x10);
            pos[3] = 0.0f;
            ((SpawnFn)SpawnEffectAt)(lbl_8047C760, 0xFDB35BFEu, pos, obj + 0x36C + i * 4);  // 4th arg: handle slot
            if (lbl_805FBBAC == NULL) {
                void* h = fn_801D40E0(1);
                if (h != NULL) h = fn_80209814(h);
                lbl_805FBBAC = h;
            }
            PlaySound(0x6158D3FDu, 0, 0, 0, 0, 1);
        } else {
            u8* fx = *(u8**)(obj + 0x36C + i * 4);
            if (fx != NULL) {
                fn_801C849C(lbl_8047C760, fx);
                *(u8**)(obj + 0x36C + i * 4) = NULL;
            }
        }
    }
}

// 0x80081AF4
// Brain chunk constructor: base ctor, then zero the state block (+0x264..+0x2AC), default speed
// from lbl_803464F0 at +0x2B0, vtable lbl_8035B068.
u8* fn_80081AF4(u8* obj, s32 a, void* params, s32 b) {
    fn_800690D4(obj, a, params, b);
    f32 zero = 0.0f;
    *(s32*)(obj + 0x264) = 0;
    *(u8**)(obj + 0x0) = lbl_8035B068;
    *(u8*)(obj + 0x268) = 0;
    *(s32*)(obj + 0x26C) = 1;
    *(f32*)(obj + 0x270) = zero;
    *(f32*)(obj + 0x274) = zero;
    *(f32*)(obj + 0x278) = zero;
    *(f32*)(obj + 0x27C) = zero;
    *(f32*)(obj + 0x280) = zero;
    *(f32*)(obj + 0x284) = zero;
    *(f32*)(obj + 0x288) = zero;
    *(f32*)(obj + 0x28C) = zero;
    *(f32*)(obj + 0x290) = zero;
    *(f32*)(obj + 0x294) = zero;
    *(f32*)(obj + 0x298) = zero;
    *(f32*)(obj + 0x29C) = zero;
    *(f32*)(obj + 0x2A0) = zero;
    *(f32*)(obj + 0x2A4) = zero;
    *(f32*)(obj + 0x2A8) = zero;
    *(f32*)(obj + 0x2AC) = zero;
    *(f32*)(obj + 0x2B0) = lbl_803464F0;
    *(s32*)(obj + 0x2B4) = 0;
    *(s32*)(obj + 0x2B8) = 0;
    *(s32*)(obj + 0x2C0) = 0;
    return obj;
}

// 0x80081BD0
// Brain chunk deleting destructor: reset vtable and pose, run the base dtor, free if del > 0.
u8* fn_80081BD0(u8* obj, s32 del) {
    if (obj != NULL) {
        *(u8**)(obj + 0x0) = lbl_8035B068;
        *(s32*)(obj + 0x2C0) = 0;
        BrainChunkResetPose(obj);
        *(f32*)(obj + 0x274) = 0.0f;
        *(u8*)(obj + 0x268) = 0;
        fn_80062CE0(obj, 0);
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x80081CB0
// Parks the chunk: stores a at +0x2C0, resets the pose, sets flag 0x400000 at +0x15C, calls
// virtual slot 0x18 (arg 1) under a lock, and if the chunk was active clears its active state.
void fn_80081CB0(u8* obj, s32 a) {
    *(s32*)(obj + 0x2C0) = a;
    BrainChunkResetPose(obj);
    u32 flags = *(u32*)(obj + 0x15C);
    *(f32*)(obj + 0x274) = 0.0f;
    *(u32*)(obj + 0x15C) = flags | 0x400000;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(SetFlagFn*)(*(u8**)obj + 0x18))(obj, 1);
    ScopedLockDtor(&lock, -1);
    if (*(u8*)(obj + 0x268) != 0) {
        *(u8*)(obj + 0x268) = 0;
        if ((*(GetSubFn*)(*(u8**)obj + 0xC))(obj) != NULL) {
            u8* sub = (*(GetSubFn*)(*(u8**)obj + 0xC))(obj);
            *(u32*)(sub + 0x8) &= ~2u;
        }
        u8* node = *(u8**)(obj + 0x4);
        *(u32*)(node + 0x20) &= ~2u;
        *(s32*)(obj + 0x2B4) = 0;
    }
}

// 0x80081F04
// Sets up a toss: precomputes the launch velocity (+0x298..) from start (+0x280) to end (+0x28C)
// over time +0x278 under gravity, solves the quadratic for the flight time stored at +0x27C.
void fn_80081F04(u8* obj) {
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(SetFlagFn*)(*(u8**)obj + 0x14))(obj, 1);
    ScopedLockDtor(&lock, -1);
    f32 half = 0.5f;
    f32 zero = 0.0f;
    f32 speed = *(f32*)(obj + 0x2B0);
    f32 dur = *(f32*)(obj + 0x278);
    f32 half_speed = half * speed;
    *(f32*)(obj + 0x274) = zero;
    *(f32*)(obj + 0x270) = *(f32*)(gWorldState + 0x8);
    *(s32*)(obj + 0x2B8) = 0;
    f32 invDur = 1.0f / dur;
    f32 sy0 = *(f32*)(obj + 0x288);
    half_speed = half_speed * dur;
    f32 k = dur * half_speed;
    f32 sx0 = *(f32*)(obj + 0x280);
    f32 sy1 = *(f32*)(obj + 0x284);
    f32 gx = lbl_803C6990[0] * k;
    f32 gy = lbl_803C6990[1] * k;
    f32 gz = lbl_803C6990[2] * k;
    f32 z0 = sy0 - zero;
    f32 ex = *(f32*)(obj + 0x28C);
    f32 ey = *(f32*)(obj + 0x290);
    f32 ez = *(f32*)(obj + 0x294);
    f32 py = gy + sy1;
    f32 px = gx + sx0;
    f32 s2 = *(f32*)(obj + 0x2B0);
    f32 pz = gz + sy0;
    f32 ddx = ex - px;
    f32 ddy = ey - py;
    f32 vx = ddx * invDur;
    f32 ddz = ez - pz;
    f32 vy = ddy * invDur;
    f32 vz = ddz * invDur;
    *(f32*)(obj + 0x298) = vx;
    *(f32*)(obj + 0x29C) = vy;
    *(f32*)(obj + 0x2A0) = vz;
    f32 a = s2 * (half * lbl_803C6990[2]);
    f32 disc = vz * vz - 4.0f * a * z0;
    f32 root = SqrtGuard(disc);
    f32 nb = -vz;
    f32 den = 2.0f * a;
    f32 t1 = (nb + root) / den;
    f32 t2 = (nb - root) / den;
    if (t1 <= t2) t1 = t2;
    *(f32*)(obj + 0x27C) = t1;
    *(f32*)(obj + 0x2A4) = zero;
    *(f32*)(obj + 0x2A8) = zero;
    *(f32*)(obj + 0x2AC) = 20.0f;
}

}  // extern "C"
