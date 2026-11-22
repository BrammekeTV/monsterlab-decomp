// Castle minigame / effect object teardown, reset and update functions (0x8008xxxx-0x800Axxxx).
#include "types.h"
#include "engine/math.h"

extern "C++" void* gEventMgr;  // 0x805FB6C8

extern "C" {
void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);  // 0x801BA2FC
void QuatGetForward(Vec3* out, const Quat* q);                              // 0x801454CC
s32 RaySceneTest(void* ray);                                                // 0x80070458
f32 cosf(f32 x);
f32 sinf(f32 x);
void fn_8008B9EC(u8* p);                      // 0x8008B9EC
void fn_8008CAB8(s32 p);                      // 0x8008CAB8
void fn_800AB0D4(u8* p);                      // 0x800AB0D4
void fn_800AC06C(u8* p);                      // 0x800AC06C
void fn_800ACE48(u8* p);                      // 0x800ACE48
void fn_800ADBE0(u8* p);                      // 0x800ADBE0
void fn_800AF960(u8* p);                      // 0x800AF960
void fn_800B0B88(u8* p);                      // 0x800B0B88
void fn_800B1564(u8* p);                      // 0x800B1564
void fn_800B1F88(u8* p);                      // 0x800B1F88
void fn_800C597C(u8* outQuat, u8* pos, f32 t);  // 0x800C597C
void fn_801455E0(f32* out, u8* quat);         // 0x801455E0
void fn_80146180(u8* quat, const f32* axis, f32 angle);  // 0x80146180
u8* fn_801B3308(void);                        // 0x801B3308
void fn_801B3378(u8* p, u8* key);             // 0x801B3378
u8* fn_801C3DB8(u8* table, u32 key);          // 0x801C3DB8
void* fn_801C3F6C(u8* table, u8* entry);      // 0x801C3F6C
void fn_801C42A8(u8* obj);                    // 0x801C42A8
void fn_801C4B90(u8* obj, f32* a, f32* b, s32 c, s32 d, u8* e);  // 0x801C4B90
s32 fn_801C5C60(u8* a);                       // 0x801C5C60
void fn_801C5C74(u8* obj, f32* a, f32* b);    // 0x801C5C74
void* fn_801D40E0(s32 n);                     // 0x801D40E0
void* fn_80209814(void* v);                   // 0x80209814
void fn_8020B1D4(void* handle, s32 on);       // 0x8020B1D4

extern void* lbl_805FBBAC;    // 0x805FBBAC (voice SFX handle)
extern f32 lbl_80346CF0[];    // 0x80346CF0
extern u8 lbl_8047C7AC[];     // 0x8047C7AC

extern f32 lbl_805FD928;  // 0.0
extern f32 lbl_805FD93C;  // 70.0
extern f32 lbl_805FDC50;  // 0.0
extern f32 lbl_805FDC68;  // 10.0
extern f32 lbl_805FDC90;  // 45.0
extern f32 lbl_805FDC94;  // 1/90
extern f32 lbl_805FDC98;  // pi/2
extern f32 lbl_805FDC9C;  // 2.0
extern f32 lbl_805FDCA0;  // 120.0
extern f32 lbl_805FDF18;  // 0.0
extern f32 lbl_805FDF1C;  // 1.0
extern f32 lbl_805FDF28;  // 70.0
extern f32 lbl_805FDF98;  // 0.0
extern f32 lbl_805FDF9C;  // 1.0
extern f32 lbl_805FDFB0;  // 0.5
extern f32 lbl_805FDFB8;  // 2.0
extern f32 lbl_805FDFC0;  // 0.0
extern f32 lbl_805FDFC4;  // 1.0
extern f32 lbl_805FDFCC;
extern f32 lbl_805FDFD0;
extern f32 lbl_805FDFD4;  // 10.0
extern f32 lbl_805FDFD8;  // 35.0

typedef f32 (*FnF32Obj)(u8*);
typedef void (*FnVecOut)(f32*, u8*, f32);

static const u32 kHashCastleWaterBallA = 0x8F25BB8Au;      // "castle_water_ballA"
static const u32 kHashCastleFireBallA = 0xBAA6441Fu;       // "castle_fire_ballA"
static const u32 kHashCastleGoodBallA = 0x67B779E5u;       // "castle_good_ballA"
static const u32 kHashCastleBadSymbolA = 0xF3B2D25Eu;      // "castle_bad_symbolA"
static const u32 kHashCastleAirSymbolA = 0xA65DCF50u;      // "castle_air_symbolA"
static const u32 kHashCastleEarthSymbolA = 0xB57069FEu;    // "castle_earth_symbolA"
static const u32 kHashCastleBrazierBowlA = 0x7279AE1Eu;    // "castle_brazier_bowlA"
static const u32 kHashCastleBrazierWandA = 0xE188D328u;    // "castle_brazier_wandA"
static const u32 kEventArg = 0x0C561960u;

// Creates the voice SFX handle once, then switches it off.
static void ResetVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    fn_8020B1D4(lbl_805FBBAC, 0);
}

// 0x80087720
// Resets the castle object: frees the two handles, sends the eight castle events, clears the
// slot tables and counters, and registers the two sub-records with the list at fn_801B3308.
void fn_80087720(u8* arg0) {
    s32 h = *(s32*)(arg0 + 0x350);
    if (h != 0) {
        fn_8008CAB8(h);
        *(s32*)(arg0 + 0x350) = 0;
    }
    if (*(u8**)(arg0 + 0x354) != NULL) {
        fn_8008B9EC(*(u8**)(arg0 + 0x354));
        *(s32*)(arg0 + 0x354) = 0;
    }
    EventSendTo(gEventMgr, kHashCastleWaterBallA, NULL, kEventArg, 0, 0);
    EventSendTo(gEventMgr, kHashCastleFireBallA, NULL, kEventArg, 0, 0);
    EventSendTo(gEventMgr, kHashCastleGoodBallA, NULL, kEventArg, 0, 0);
    EventSendTo(gEventMgr, kHashCastleBadSymbolA, NULL, kEventArg, 0, 0);
    EventSendTo(gEventMgr, kHashCastleAirSymbolA, NULL, kEventArg, 0, 0);
    EventSendTo(gEventMgr, kHashCastleEarthSymbolA, NULL, kEventArg, 0, 0);
    EventSendTo(gEventMgr, kHashCastleBrazierBowlA, NULL, kEventArg, 0, 0);
    EventSendTo(gEventMgr, kHashCastleBrazierWandA, NULL, kEventArg, 0, 0);

    // Clears 0x94..0xF4, then three blocks of 0xF8..0x1B4 (stride 0xC0), then 0x338..0x34C.
    for (s32 off = 0x94; off <= 0xF4; off += 4) *(s32*)(arg0 + off) = 0;
    u8* blk = arg0;
    for (s32 i = 0; i < 3; i++) {
        for (s32 off = 0xF8; off <= 0x1B4; off += 4) *(s32*)(blk + off) = 0;
        blk += 0xC0;
    }
    for (s32 off = 0x338; off <= 0x34C; off += 4) *(s32*)(arg0 + off) = 0;

    for (s32 off = 0x358; off <= 0x37C; off += 4) *(s32*)(arg0 + off) = 0;
    for (s32 off = 0x380; off <= 0x3A8; off += 4) *(f32*)(arg0 + off) = lbl_805FD928;
    *(s32*)(arg0 + 0x3AC) = 0;
    *(s32*)(arg0 + 0x3B0) = 0;
    *(f32*)(arg0 + 0x3B4) = lbl_805FD928;
    *(s32*)(arg0 + 0x3B8) = 0;
    *(f32*)(arg0 + 0x3BC) = lbl_805FD928;
    *(f32*)(arg0 + 0x47C) = lbl_805FD93C;
    for (s32 off = 0x480; off <= 0x490; off += 4) *(f32*)(arg0 + off) = lbl_805FD928;
    for (s32 off = 0x494; off <= 0x4A4; off += 4) *(s32*)(arg0 + off) = 0;
    *(f32*)(arg0 + 0x4AC) = lbl_805FD928;
    fn_801B3378(fn_801B3308(), arg0 + 0x730);
    fn_801B3378(fn_801B3308(), arg0 + 0x3C0);
    ResetVoiceHandle();
}

// 0x8009AEA0
// Spins three orbiting effect rings: each ring's transform point is moved around the axis of the
// base orientation and its position copied to every sub-entry.
void fn_8009AEA0(u8* arg0, f32 dt) {
    f32 pos[3];     // sp68
    f32 axis[3];    // sp48
    u8 quat[16];    // sp58
    Vec3 fwd;       // sp3C
    u8* base = arg0;

    f32 t = lbl_805FDC68 + (*(FnF32Obj*)(*(u8**)(arg0 + 0x170) + 0x20))(arg0 + 0x9C);
    (*(FnVecOut*)(*(u8**)(arg0 + 0x170) + 0x24))(pos, arg0 + 0x9C, t);
    fn_800C597C(quat, arg0 + 0x9C, t);
    fn_801455E0(axis, quat);
    QuatGetForward(&fwd, (const Quat*)quat);

    f32 angle = *(f32*)(arg0 + 0x1BC) + lbl_805FDC90 * dt;
    *(f32*)(arg0 + 0x1BC) = angle;
    for (s32 i = 0; i < 3; i++) {
        if (*(s32*)(base + 0x1B0) != 0) {
            u8* tr = *(u8**)(base + 0x1A4);
            f32 a = lbl_805FDC94 * angle;
            *(f32*)(tr + 0x8) = pos[0];
            *(f32*)(tr + 0xC) = pos[1];
            *(f32*)(tr + 0x10) = pos[2];
            f32 c = cosf(lbl_805FDC98 * a);
            f32 cx = axis[0] * c;
            f32 cy = axis[1] * c;
            f32 cz = axis[2] * c;
            f32 s = sinf(lbl_805FDC98 * a);
            f32 sx = fwd.x * s;
            f32 sy = fwd.y * s;
            f32 sz = fwd.z * s;
            tr = *(u8**)(base + 0x1A4);
            *(f32*)(tr + 0x8) = *(f32*)(tr + 0x8) + (sx + cx) * lbl_805FDC9C;
            *(f32*)(tr + 0xC) = *(f32*)(tr + 0xC) + (sy + cy) * lbl_805FDC9C;
            *(f32*)(tr + 0x10) = *(f32*)(tr + 0x10) + (sz + cz) * lbl_805FDC9C;
            tr = *(u8**)(base + 0x1A4);
            u8* list = *(u8**)(base + 0x1B0);
            f32 px = *(f32*)(tr + 0x8);
            f32 py = *(f32*)(tr + 0xC);
            f32 pz = *(f32*)(tr + 0x10);
            u8* slot = list;
            for (s32 k = 0; k < *(s32*)(list + 0xC); k++) {
                u8* e = *(u8**)(slot + 0x14);
                if (e != NULL) {
                    *(f32*)(e + 0x10) = px;
                    *(f32*)(e + 0x14) = py;
                    *(f32*)(e + 0x18) = pz;
                    *(f32*)(e + 0x1C) = lbl_805FDC50;
                }
                slot += 4;
            }
        }
        angle += lbl_805FDCA0;
        base += 4;
    }
}

// 0x800A8810
// Resets the effect manager: releases every slot table (6, 10, 14 and 7 entries plus single
// handles), restores defaults and re-registers its sub-records.
void fn_800A8810(u8* arg0) {
    u8* p = arg0;
    for (s32 i = 0; i < 6; i++, p += 4) {
        if (*(u8**)(p + 0x98) != NULL) {
            fn_800ACE48(*(u8**)(p + 0x98));
            *(s32*)(p + 0x98) = 0;
        }
    }
    if (*(u8**)(arg0 + 0x94) != NULL) {
        fn_800B1564(*(u8**)(arg0 + 0x94));
        *(s32*)(arg0 + 0x94) = 0;
    }
    if (*(u8**)(arg0 + 0x90) != NULL) {
        fn_800B1F88(*(u8**)(arg0 + 0x90));
        *(s32*)(arg0 + 0x90) = 0;
    }
    if (*(u8**)(arg0 + 0xB0) != NULL) {
        fn_800AF960(*(u8**)(arg0 + 0xB0));
        *(s32*)(arg0 + 0xB0) = 0;
    }
    if (*(u8**)(arg0 + 0xB4) != NULL) {
        fn_800B0B88(*(u8**)(arg0 + 0xB4));
        *(s32*)(arg0 + 0xB4) = 0;
    }
    p = arg0;
    for (s32 i = 0; i < 10; i++, p += 4) {
        if (*(u8**)(p + 0xB8) != NULL) {
            fn_800AB0D4(*(u8**)(p + 0xB8));
            *(s32*)(p + 0xB8) = 0;
        }
    }
    p = arg0;
    for (s32 i = 0; i < 14; i++, p += 4) {
        if (*(u8**)(p + 0xE0) != NULL) {
            fn_800AC06C(*(u8**)(p + 0xE0));
            *(s32*)(p + 0xE0) = 0;
        }
    }
    p = arg0;
    for (s32 i = 0; i < 7; i++, p += 4) {
        if (*(s32*)(p + 0x134) != 0) *(s32*)(p + 0x134) = 0;
        if (*(u8**)(p + 0x118) != NULL) {
            fn_800ADBE0(*(u8**)(p + 0x118));
            *(s32*)(p + 0x118) = 0;
        }
    }
    *(s32*)(arg0 + 0x6A4) = 0;
    *(f32*)(arg0 + 0x6A0) = lbl_805FDF28;
    // The asm passes the leftover r3 (the last fn_800ADBE0 slot or 0) as the ray argument.
    s32 idx = RaySceneTest(NULL);
    f32 zero = lbl_805FDF18;
    f32 one = lbl_805FDF1C;
    *(f32*)(arg0 + 0x154) = zero;
    *(f32*)(arg0 + 0x158) = zero;
    *(f32*)(arg0 + 0x150) = lbl_80346CF0[idx];
    *(f32*)(arg0 + 0x15C) = zero;
    *(s32*)(arg0 + 0x6A8) = 0;
    // 0x6AC..0x6FC cleared, then identity-like 1.0 entries at 0x700 and 0x710. (The original also
    // fills a dead 3x4 matrix on the stack with the same pattern; omitted.)
    for (s32 off = 0x6AC; off <= 0x6FC; off += 4) *(f32*)(arg0 + off) = zero;
    *(f32*)(arg0 + 0x700) = one;
    *(f32*)(arg0 + 0x704) = zero;
    *(f32*)(arg0 + 0x708) = zero;
    *(f32*)(arg0 + 0x70C) = zero;
    *(f32*)(arg0 + 0x710) = one;
    for (s32 off = 0x714; off <= 0x724; off += 4) *(f32*)(arg0 + off) = zero;
    fn_801B3378(fn_801B3308(), arg0 + 0x160);
    fn_801B3378(fn_801B3308(), arg0 + 0x21C);
    ResetVoiceHandle();
}

// 0x800AB6A0
// Sets up a sweep box from the endpoints at +0x264/+0x270: length 2*field14, a perpendicular
// extent (normalized cross with the up vector scaled by half the length), then looks up the
// scene entry for arg1 and pushes the box corners into its collision shape.
// (The original has an unreachable first path, skipped by a branch; omitted.)
void fn_800AB6A0(u8* arg0, u32 arg1) {
    f32 zero = lbl_805FDF98;
    f32 one = lbl_805FDF9C;
    f32 dx = *(f32*)(arg0 + 0x264) - *(f32*)(arg0 + 0x270);
    f32 dy = *(f32*)(arg0 + 0x268) - *(f32*)(arg0 + 0x274);
    f32 dz = *(f32*)(arg0 + 0x26C) - *(f32*)(arg0 + 0x278);
    *(f32*)(arg0 + 0x288) = lbl_805FDFB8 * *(f32*)(arg0 + 0x14);
    *(u8*)(arg0 + 0x28C) = 0;

    f32 cx = dy * one - dz * zero;
    f32 cy = dz * zero - dx * one;
    f32 cz = dx * zero - dy * zero;
    *(f32*)(arg0 + 0x298) = cx;
    *(f32*)(arg0 + 0x29C) = cy;
    *(f32*)(arg0 + 0x2A0) = cz;
    f32 len2 = cz * cz + (cx * cx + cy * cy);
    f32 nx, ny, nz;
    if (zero == len2) {
        nx = ny = nz = zero;
    } else {
        // The original refines frsqrte twice; the result is 1 / sqrt to float precision.
        f32 r = (f32)(1.0 / sqrt((f64)len2));
        nx = cx * r;
        ny = cy * r;
        nz = cz * r;
    }
    f32 half = lbl_805FDFB0 * *(f32*)(arg0 + 0x288);
    *(f32*)(arg0 + 0x298) = nx * half;
    *(f32*)(arg0 + 0x29C) = ny * half;
    *(f32*)(arg0 + 0x2A0) = nz * half;

    u8* entry = fn_801C3DB8(lbl_8047C7AC, arg1);
    if (entry == NULL) return;
    if (*(s32*)(entry + 0x4C) != 0) {
        *(void**)(arg0 + 0x294) = fn_801C3F6C(lbl_8047C7AC, entry);
    } else {
        *(void**)(arg0 + 0x290) = fn_801C3F6C(lbl_8047C7AC, entry);
    }
    u8* shape = *(u8**)(arg0 + 0x290);
    if (shape != NULL) {
        f32 hi[3], lo[3];
        fn_801C42A8(shape);
        hi[0] = *(f32*)(arg0 + 0x27C) + *(f32*)(arg0 + 0x298);
        hi[1] = *(f32*)(arg0 + 0x280) + *(f32*)(arg0 + 0x29C);
        hi[2] = *(f32*)(arg0 + 0x284) + *(f32*)(arg0 + 0x2A0);
        lo[0] = *(f32*)(arg0 + 0x27C) - *(f32*)(arg0 + 0x298);
        lo[1] = *(f32*)(arg0 + 0x280) - *(f32*)(arg0 + 0x29C);
        lo[2] = *(f32*)(arg0 + 0x284) - *(f32*)(arg0 + 0x2A0);
        shape = *(u8**)(arg0 + 0x290);
        fn_801C4B90(shape, hi, lo, 0, 0, *(u8**)(*(u8**)shape + 4));
        shape = *(u8**)(arg0 + 0x290);
        fn_801C4B90(shape, hi, lo, 0, 0, *(u8**)(*(u8**)shape + 4));
        return;
    }
    u8* shape2 = *(u8**)(arg0 + 0x294);
    if (shape2 != NULL) {
        f32 hi[3], lo[3];
        fn_801C5C60(shape2);
        hi[0] = *(f32*)(arg0 + 0x27C) + *(f32*)(arg0 + 0x298);
        hi[1] = *(f32*)(arg0 + 0x280) + *(f32*)(arg0 + 0x29C);
        hi[2] = *(f32*)(arg0 + 0x284) + *(f32*)(arg0 + 0x2A0);
        lo[0] = *(f32*)(arg0 + 0x27C) - *(f32*)(arg0 + 0x298);
        lo[1] = *(f32*)(arg0 + 0x280) - *(f32*)(arg0 + 0x29C);
        lo[2] = *(f32*)(arg0 + 0x284) - *(f32*)(arg0 + 0x2A0);
        fn_801C5C74(*(u8**)(arg0 + 0x294), hi, lo);
    }
}

// Sets the three "moved" flags on the owner's transform record (+4), as the asm does one by one.
static void MarkMoved(u8* arg0) {
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
}

// 0x800AC0F0
// Per-frame update of the flying object: state 1 lifts it with gravity-like drag and spins it,
// raising by the clamped height fraction; state 3 pushes it along its rotation axis.
void fn_800AC0F0(u8* arg0, f32 dt) {
    s32 state = *(s32*)(arg0 + 0x264);
    if (state == 1) {
        u8* pos = *(u8**)(arg0 + 0x4);
        s32 mode = *(s32*)(arg0 + 0x268);
        f32 px = *(f32*)(pos + 0x0);
        f32 py = *(f32*)(pos + 0x4);
        f32 pz = *(f32*)(pos + 0x8);
        f32 v = *(f32*)(arg0 + 0x26C) * dt;
        *(f32*)(arg0 + 0x294) = px;
        *(f32*)(arg0 + 0x298) = py;
        *(f32*)(arg0 + 0x29C) = pz;
        if (mode == 0) {
            v -= lbl_805FDFCC * *(f32*)(arg0 + 0x270) * dt;
        } else if (mode == 1) {
            v -= lbl_805FDFD0 * *(f32*)(arg0 + 0x270) * dt;
        }
        *(f32*)(arg0 + 0x270) = *(f32*)(arg0 + 0x270) + dt;
        f32 axis[3];
        axis[0] = lbl_805FDFC0;
        axis[1] = lbl_805FDFC0;
        axis[2] = lbl_805FDFC4;
        pz += v;
        fn_80146180(arg0 + 0x278, axis, *(f32*)(arg0 + 0x274) * dt);
        f32 frac = pz / lbl_805FDFD4;
        f32 clamped;
        if (frac < lbl_805FDFC0) {
            clamped = lbl_805FDFC0;
        } else if (frac > lbl_805FDFC4) {
            clamped = lbl_805FDFC4;
        } else {
            clamped = frac;
        }
        Vec3 up = QuatGetAxisY((const Quat*)(arg0 + 0x278));
        f32 ox = up.x * clamped;
        f32 oy = up.y * clamped;
        f32 oz = up.z * clamped;
        pos = *(u8**)(arg0 + 0x4);
        *(f32*)(pos + 0x0) = *(f32*)(arg0 + 0x288) + ox;
        *(f32*)(pos + 0x4) = *(f32*)(arg0 + 0x28C) + oy;
        *(f32*)(pos + 0x8) = pz + oz;
        MarkMoved(arg0);
    } else if (state == 3) {
        f32 step = lbl_805FDFD8 * dt;
        f32 dir[3];
        fn_801455E0(dir, *(u8**)(arg0 + 0x4) + 0xC);
        u8* pos = *(u8**)(arg0 + 0x4);
        f32 nx = *(f32*)(pos + 0x0) + dir[0] * step;
        f32 ny = *(f32*)(pos + 0x4) + dir[1] * step;
        f32 nz = *(f32*)(pos + 0x8) + dir[2] * step;
        *(f32*)(pos + 0x0) = nx;
        *(f32*)(pos + 0x4) = ny;
        *(f32*)(pos + 0x8) = nz;
        MarkMoved(arg0);
    }
}

}  // extern "C"
