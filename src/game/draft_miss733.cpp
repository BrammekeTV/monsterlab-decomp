// Castle minigame score/combo update, creature bump, spin-bob effect and sphere-ray hit tests
// (0x8008xxxx-0x800Axxxx).
#include "types.h"
#include "engine/math.h"

extern "C++" void* gEventMgr;  // 0x805FB6C8

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // event dispatch
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);
void QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);                              // 0x80145EE8
u32 RandomU32(void);
s32 RaySceneTest(void* ray);                                                                  // 0x80070458 (difficulty index)
int SysSprintf(char* buf, const char* fmt, ...);
u8* fn_8006FBDC(void);                                                                        // 0x8006FBDC
void fn_800712B8(void);                                                                       // 0x800712B8
f32 fn_8008AC08(s32 unused, s32 n, s32 mode);                                                 // 0x8008AC08
void fn_8008CAFC(u8* a, s32 b);                                                               // 0x8008CAFC
void fn_800A7D88(void* obj, u32 hash);                                                        // 0x800A7D88
void fn_800B179C(u8* a);                                                                      // 0x800B179C
f32 fn_8033F200(f32 a, f32 b);                                                                // 0x8033F200 (fmod)
f32 sinf(f32 x);

extern char lbl_8035B7B8[];  // 0x8035B7B8 (printf formats: +0x00, +0x20, +0x44)
extern char lbl_805F89FC[];  // 0x805F89FC (small string)
extern f32 lbl_803469A8[];   // 0x803469A8 (score cap per difficulty)
extern f32 lbl_803469C8[];   // 0x803469C8 (stage 0 limit per difficulty)
extern f32 lbl_803469D8[];   // 0x803469D8 (stage 1 limit per difficulty)
extern f32 lbl_803469E8[];   // 0x803469E8 (stage 2 limit per difficulty)
extern f32 lbl_803469F8[];   // 0x803469F8 (stage bonus per difficulty)
extern f32 lbl_80346A08[];   // 0x80346A08 (combo base bonus)
extern f32 lbl_80346A18[];   // 0x80346A18 (combo scale per difficulty)
}

static const u32 kHashCombo = 0x9A547EC8u;
static const u32 kHashStage = 0x15DCC5AEu;
static const u32 kEventScore = 0xA97843C1u;
static const u32 kHashSphereHitA = 0xE01EAA97u;

// Difficulty index (RaySceneTest result, original reads no argument) clamped to 0..3.
static inline s32 DiffIdx() {
    s32 i = RaySceneTest(0);
    if (i < 0) return 0;
    if (i > 3) return 3;
    return i;
}

// Clamps v between 0 and hi (hi may be negative, then the range is [hi, 0]).
static inline f32 ClampToLimit(f32 v, f32 hi) {
    if (0.0f < hi) {
        if (v < 0.0f) return 0.0f;
        if (v > hi) return hi;
        return v;
    }
    if (v < hi) return hi;
    if (v > 0.0f) return 0.0f;
    return v;
}

// Adds amt to the stage meter at offVal; on reaching the limit it is capped and, if it was below the limit
// before this step, the stage counter at offCnt ticks, the score gets the stage bonus and the stage message
// is printed. Returns 1 if the stage completed.
static s32 StageStep(u8* p, s32 amt, s32 offVal, const f32* tbl, s32 offCnt) {
    s32 wasBelow = 0;
    if (*(f32*)(p + offVal) < tbl[DiffIdx()]) wasBelow = 1;
    *(f32*)(p + offVal) = *(f32*)(p + offVal) + (f32)amt;
    if (!(*(f32*)(p + offVal) >= tbl[DiffIdx()])) return 0;
    *(f32*)(p + offVal) = tbl[DiffIdx()];
    if (!wasBelow) return 0;
    *(s32*)(p + offCnt) = *(s32*)(p + offCnt) + 1;
    *(s32*)(p + 0x4A4) = *(s32*)(*(u8**)(p + 0x354) + 0x370);
    *(f32*)(p + 0x484) = *(f32*)(p + 0x484) + lbl_803469F8[DiffIdx()];
    *(f32*)(p + 0x484) = ClampToLimit(*(f32*)(p + 0x484), lbl_803469A8[DiffIdx()]);
    *(f32*)(p + 0x4AC) = 3.0f;
    SysSprintf((char*)(p + 0x530), lbl_8035B7B8 + 0x20, (double)lbl_803469F8[DiffIdx()]);
    return 1;
}

// Cast a ray (arg1: velocity = +0x14 - +0x20) against the sphere target tgt (radius +0x3C summed with
// ray radius +0x3C). Returns 1 and the entry time in *t on a hit (t = 0 if already inside).
static s32 SphereRayTime(u8* ray, u8* tgt, f32* t) {
    f32 vx = *(f32*)(ray + 0x14) - *(f32*)(ray + 0x20);
    f32 vy = *(f32*)(ray + 0x18) - *(f32*)(ray + 0x24);
    f32 vz = *(f32*)(ray + 0x1C) - *(f32*)(ray + 0x28);
    f32 dx = *(f32*)(tgt + 0x14) - *(f32*)(ray + 0x20);
    f32 dy = *(f32*)(tgt + 0x18) - *(f32*)(ray + 0x24);
    f32 dz = *(f32*)(tgt + 0x1C) - *(f32*)(ray + 0x28);
    f32 r = *(f32*)(tgt + 0x3C) + *(f32*)(ray + 0x3C);
    f32 nx = 0.0f - vx;
    f32 ny = 0.0f - vy;
    f32 nz = 0.0f - vz;
    f32 c = ((dz * dz) + ((dx * dx) + (dy * dy))) - (r * r);
    if (c < 0.0f) {
        *t = 0.0f;
        return 1;
    }
    f32 a = (nz * nz) + ((nx * nx) + (ny * ny));
    if (a < 0.0001f) return 0;
    f32 b = (nz * dz) + ((nx * dx) + (ny * dy));
    if (b >= 0.0f) return 0;
    f32 disc = (b * b) - (a * c);
    if (disc < 0.0f) return 0;
    f32 s = 0.0f;
    if (!(disc <= 0.0f)) s = (1.0f / sqrtf(disc)) * disc;  // sqrt(disc)
    *t = (-b - s) / a;
    return 1;
}

// Walks the +0x8C reference chain (two nested single-slot links) to the target object; null if broken.
static u8* SphereTarget(u8* self) {
    u8* a = *(u8**)(self + 0x8C);
    if (a && *(u32*)(a + 0x10) == 1) {
        u8* b = **(u8***)(a + 0xC);
        if (b && *(u32*)(b + 0x28) == 1) {
            u8* c = **(u8***)(b + 0x24);
            if (*(s32*)(c + 0xC) == 0) return c;
        }
    }
    return 0;
}

extern "C" {

// 0x80089B30
// Per-tick castle score update: adds the points for the current kill to the score, advances the
// three stage meters, fires the combo bonus when all stages are full and reports the score event.
void fn_80089B30(u8* p) {
    u8* t = *(u8**)(p + 0x354);
    if (*(s32*)(t + 0x264) == 1) return;
    s32 amt = *(s32*)(t + 0x35C);
    if (amt == 0) return;

    f32 d = fn_8008AC08(0, amt, *(s32*)(t + 0x370));
    *(f32*)(p + 0x484) = *(f32*)(p + 0x484) + d;
    *(f32*)(p + 0x484) = ClampToLimit(*(f32*)(p + 0x484), lbl_803469A8[DiffIdx()]);
    *(f32*)(p + 0x4AC) = 2.0f;
    SysSprintf((char*)(p + 0x4B0), lbl_8035B7B8, amt, (double)d);
    SysSprintf((char*)(p + 0x530), lbl_805F89FC);
    fn_800712B8();
    fn_8008CAFC(*(u8**)(p + 0x350), amt);

    s32 stageDone = 0;
    switch (*(s32*)(*(u8**)(p + 0x354) + 0x370)) {
    case 2:
        stageDone = StageStep(p, amt, 0x488, lbl_803469C8, 0x494);
        break;
    case 3:
        stageDone = StageStep(p, amt, 0x48C, lbl_803469D8, 0x498);
        break;
    case 4:
        stageDone = StageStep(p, amt, 0x490, lbl_803469E8, 0x49C);
        break;
    default:
        break;
    }

    if (*(u8*)(p + 0x7E4) == 0) {
        if (*(f32*)(p + 0x488) >= lbl_803469C8[DiffIdx()] &&
            *(f32*)(p + 0x48C) >= lbl_803469D8[DiffIdx()] &&
            *(f32*)(p + 0x490) >= lbl_803469E8[DiffIdx()]) {
            // all three stages full: combo bonus
            *(u8*)(p + 0x7E4) = 1;
            s32 base = DiffIdx();
            f32 scaled = *(f32*)(p + 0x47C) * lbl_80346A18[DiffIdx()];
            *(f32*)(p + 0x4AC) = 5.0f;
            f32 bonus = lbl_80346A08[base] + scaled;
            SysSprintf((char*)(p + 0x4B0), lbl_8035B7B8 + 0x44, (double)bonus);
            *(f32*)(p + 0x484) = *(f32*)(p + 0x484) + bonus;
            *(f32*)(p + 0x484) = ClampToLimit(*(f32*)(p + 0x484), lbl_803469A8[DiffIdx()]);
            *(f32*)(p + 0x488) = 0.0f;
            *(f32*)(p + 0x48C) = 0.0f;
            *(f32*)(p + 0x490) = 0.0f;
            *(u8*)(p + 0x7E4) = 0;
            *(f32*)(p + 0x7E8) = 0.12f;
            *(s32*)(p + 0x7EC) = 5;
            *(s32*)(p + 0x4A0) = *(s32*)(p + 0x4A0) + 1;
            PlaySound(kHashCombo, 0, 0, 0, 0, 0);
        } else if (stageDone != 0) {
            PlaySound(kHashStage, 0, 0, 0, 0, 0);
        }
    }

    EventSend(gEventMgr, *(void**)(p + 0x354), 0, kEventScore, 0, 0, 1, 0);
}

// 0x8008E5E8
// Pushes two nearby living creatures (state 3) apart: when within range in the XZ plane, adds a
// normalized separation vector (scaled by 1/distance) to a's velocity at +0x2E0..+0x2E8, floor-clamped.
void fn_8008E5E8(u8* a, u8* b) {
    if (*(s32*)(a + 0x264) != 3) return;
    if (*(s32*)(b + 0x264) != 3) return;
    if (a == b) return;
    if (fabsf(*(f32*)(a + 0x2F8) - *(f32*)(b + 0x2F8)) > 1.0f) return;
    if (fabsf(*(f32*)(a + 0x300) - *(f32*)(b + 0x300)) > 1.0f) return;

    u8* pa = *(u8**)(a + 4);
    u8* pb = *(u8**)(b + 4);
    f32 dx = *(f32*)(pa + 0) - *(f32*)(pb + 0);
    f32 dz = *(f32*)(pa + 8) - *(f32*)(pb + 8);
    f32 vx = dx;
    f32 vy = 0.0f;
    f32 vz = dz;
    f32 d2 = (dz * dz) + ((dx * dx) + (0.0f * 0.0f));
    if (d2 <= 0.05f) {
        // practically on top of each other: pick a random direction
        vx = -1.0f + (2.0f * (2.3283064e-10f * (f32)RandomU32()));
        vz = 0.0f + (1.0f * (2.3283064e-10f * (f32)RandomU32()));
        d2 = (vz * vz) + ((vx * vx) + (vy * vy));
    }
    if (*(s32*)(a + 0x26C) != 3) return;
    if (!(d2 < 3.5f)) return;

    f32 n2 = (vz * vz) + ((vx * vx) + (vy * vy));
    f32 nx, ny, nz;
    if (n2 == 0.0f) {
        nx = ny = nz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(n2);
        nx = vx * inv;
        ny = vy * inv;
        nz = vz * inv;
    }
    vz = nz / d2;
    vx = nx / d2;
    vy = ny / d2;
    f32 z = *(f32*)(a + 0x2E8) + vz;
    *(f32*)(a + 0x2E8) = z;
    *(f32*)(a + 0x2E0) = *(f32*)(a + 0x2E0) + vx;
    *(f32*)(a + 0x2E4) = *(f32*)(a + 0x2E4) + vy;
    if (z < 0.0f) *(f32*)(a + 0x2E8) = 0.0f;
}

// 0x8009E308
// Bobbing/spinning motion of the attached transform while the manager's value at +0x3EC is negative;
// otherwise clears the active flag (bit 0 of +0x15C).
void fn_8009E308(u8* p, f32 dt) {
    u8* mgr = fn_8006FBDC();
    f32 gate = *(f32*)(*(u8**)(mgr + 0x198) + 0x3EC);
    if (!(gate >= 0.0f)) {
        f32 offX = 0.0f;
        f32 offY = 0.0f;
        f32 offZ = 0.0f;
        *(u32*)(p + 0x15C) = *(u32*)(p + 0x15C) | 1;
        *(f32*)(p + 0x290) = *(f32*)(p + 0x290) + dt;
        f32 per = *(f32*)(p + 0x294);
        f32 ph = fn_8033F200(*(f32*)(p + 0x290), per) / *(f32*)(p + 0x294);
        offX = sinf(1.5707964f * (0.63661975f * (6.2831855f * ph))) * (0.75f * 6.0f);
        f32 half = 0.5f * *(f32*)(p + 0x294);
        f32 ph2 = fn_8033F200(*(f32*)(p + 0x290), half) / (0.5f * *(f32*)(p + 0x294));
        offZ = sinf(1.5707964f * (0.63661975f * (6.2831855f * ph2))) * (0.2f * 6.0f);
        f32 px = *(f32*)(p + 0x274) + offX;
        f32 py = *(f32*)(p + 0x278) + offY;
        f32 pz = *(f32*)(p + 0x27C) + offZ;
        u8* xf = *(u8**)(p + 4);
        *(f32*)(xf + 0) = px;
        *(f32*)(xf + 4) = py;
        *(f32*)(xf + 8) = pz;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x8000;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x10000000;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x4000;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x10000000;

        *(f32*)(p + 0x298) = *(f32*)(p + 0x298) + 15.0f * dt;
        Vec3 axis;
        axis.x = 0.0f;
        axis.y = 0.0f;
        axis.z = 1.0f;
        Quat spin;
        QuatFromAxisAngle(&spin, &axis, 0.63661975f * *(f32*)(p + 0x298));

        // rot = base * spin, base quaternion at +0x280 (x, y, z, w)
        f32 bw = *(f32*)(p + 0x28C);
        f32 bx = *(f32*)(p + 0x280);
        f32 by = *(f32*)(p + 0x284);
        f32 bz = *(f32*)(p + 0x288);
        xf = *(u8**)(p + 4);
        f32 qx = (by * spin.z) + ((bw * spin.x) + (bx * spin.w)) - (bz * spin.y);
        f32 qy = (bz * spin.x) + ((by * spin.w) + ((bw * spin.y) - (bx * spin.z)));
        f32 qz = (bz * spin.w) + (((bw * spin.z) + (bx * spin.y)) - (by * spin.x));
        f32 qw = (((bw * spin.w) - (bx * spin.x)) - (by * spin.y)) - (bz * spin.z);
        *(f32*)(xf + 0xC) = qx;
        *(f32*)(xf + 0x10) = qy;
        *(f32*)(xf + 0x14) = qz;
        *(f32*)(xf + 0x18) = qw;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x8000;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x10000000;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x4000;
        xf = *(u8**)(p + 4);
        *(u32*)(xf + 0x20) = *(u32*)(xf + 0x20) | 0x10000000;
        return;
    }
    *(u32*)(p + 0x15C) = *(u32*)(p + 0x15C) & ~1u;
}

// 0x800AC850
// Sphere test of the ray arg1 against the target object of arg0. On a hit within the ray length
// (t <= 1) notifies arg0 and returns the hit hash, else -1.
s32 fn_800AC850(u8* arg0, u8* arg1) {
    f32 t = 0.0f;
    u8* tgt = SphereTarget(arg0);
    if (SphereRayTime(arg1, tgt, &t) != 0 && t <= 1.0f) {
        fn_800A7D88(arg0, kHashSphereHitA);
        return (s32)kHashSphereHitA;
    }
    return -1;
}

// 0x800AD340
// Same sphere test as fn_800AC850; on a hit (t <= 1) releases the object at +0x274 via fn_800B179C
// and returns the hit hash 0xDC2ED9B5, else -1.
s32 fn_800AD340(u8* arg0, u8* arg1) {
    f32 t = 0.0f;
    u8* tgt = SphereTarget(arg0);
    s32 hit = 0;
    if (SphereRayTime(arg1, tgt, &t) != 0 && t <= 1.0f) hit = 1;
    if (hit != 0) {
        fn_800B179C(*(u8**)(arg0 + 0x274));
        return (s32)0xDC2ED9B5u;
    }
    return -1;
}

// 0x800AD5A8
// Sphere test as above; returns 1 on a hit (t <= 1) and 0 otherwise. If out is given it receives the
// target position (+0x14) on a hit and zeros on a miss.
s32 fn_800AD5A8(u8* arg0, u8* arg1, f32* out) {
    f32 t = 0.0f;
    u8* tgt = SphereTarget(arg0);
    if (SphereRayTime(arg1, tgt, &t) != 0 && t <= 1.0f) {
        if (out != 0) {
            out[0] = *(f32*)(tgt + 0x14);
            out[1] = *(f32*)(tgt + 0x18);
            out[2] = *(f32*)(tgt + 0x1C);
        }
        return 1;
    }
    if (out != 0) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    }
    return 0;
}

}  // extern "C"
