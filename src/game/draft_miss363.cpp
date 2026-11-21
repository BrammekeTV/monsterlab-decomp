// Particle / effect helpers: random emitter vectors, lightning bolt subdivision, quat spin axis.
#include <math.h>
#include "types.h"
#include "engine/math.h"

extern "C" {
void QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);   // 0x80145EE8
u32  RandomU32(void);                                              // 0x80143460
void fn_801458B0(f32* quat, f32* a, f32* b);                       // 0x801458B0
void fn_801BCD88(u8* node, s32 a, s32 b, f32 scale);               // 0x801BCD88
}

// Uniform random in [0, 1].
static inline f32 Rand01(void) {
    return 2.3283064e-10f * (f32)RandomU32();
}

// Uniform random in [-amp, amp].
static inline f32 RandSpread(f32 amp) {
    return amp * (2.0f * Rand01()) - amp;
}

// Reciprocal square root, refined like the frsqrte + two Newton steps in the original.
static inline f32 RSqrtRefined(f32 x) {
    return (f32)(1.0 / sqrt((f64)x));
}

// Clamp x to the range between a and b (either order).
static inline f32 ClampAny(f32 x, f32 a, f32 b) {
    if (a < b) {
        if (x < a) return a;
        if (x > b) return b;
        return x;
    }
    if (x < b) return b;
    if (x > a) return a;
    return x;
}

// Displaces the midpoint of nodes a and b by a random offset of +-amp and stores it in dst (xyz at +0x4C).
static inline void MidpointJitter(u8* dst, const u8* a, const u8* b, f32 amp) {
    f32 sx = *(f32*)(a + 0x4C) + *(f32*)(b + 0x4C);
    f32 sy = *(f32*)(a + 0x50) + *(f32*)(b + 0x50);
    f32 sz = *(f32*)(a + 0x54) + *(f32*)(b + 0x54);
    f32 mx = sx * 0.5f;
    f32 my = sy * 0.5f;
    f32 mz = sz * 0.5f;
    f32 rx = RandSpread(amp);
    f32 ry = RandSpread(amp);
    f32 rz = RandSpread(amp);
    *(f32*)(dst + 0x4C) = mx + rx;
    *(f32*)(dst + 0x50) = my + ry;
    *(f32*)(dst + 0x54) = mz + rz;
}

extern "C" {

// 0x801BD278
// Lightning bolt setup: stores start/end points in the first/last node (12 bytes each from +0x4C),
// the length at +0x34, then displaces the midpoint with random jitter and recurses on both halves.
void fn_801BD278(u8* arg0, f32* arg1, f32* arg2) {
    s32 last = *(s32*)(arg0 + 0x38) - 1;
    *(f32*)(arg0 + 0x4C) = arg1[0];
    *(f32*)(arg0 + 0x50) = arg1[1];
    *(f32*)(arg0 + 0x54) = arg1[2];
    u8* lastNode = arg0 + last * 0xC;
    *(f32*)(lastNode + 0x4C) = arg2[0];
    *(f32*)(lastNode + 0x50) = arg2[1];
    *(f32*)(lastNode + 0x54) = arg2[2];

    f32 dx = arg2[0] - arg1[0];
    f32 dy = arg2[1] - arg1[1];
    f32 dz = arg2[2] - arg1[2];
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 len = 0.0f;
    if (len2 > 0.0f) {
        len = (f32)((f64)RSqrtRefined(len2) * (f64)len2);
    }
    *(f32*)(arg0 + 0x34) = len;

    s32 n = *(s32*)(arg0 + 0x38) - 1;
    u8* params = *(u8**)arg0;
    f32 scale = *(f32*)(params + 0x48);
    s32 mid = n / 2;
    if (n > 1) {
        u8* endNode = arg0 + n * 0xC;
        f32 amp = scale * len * *(f32*)(params + 0x4C);
        u8* midNode = arg0 + mid * 0xC;
        MidpointJitter(midNode, arg0, endNode, amp);
        if (n > 2) {
            s32 q1 = mid / 2;
            if (mid > 1) {
                f32 amp1 = 0.5f * scale * *(f32*)(arg0 + 0x34) * *(f32*)(*(u8**)arg0 + 0x4C);
                MidpointJitter(arg0 + q1 * 0xC, arg0, midNode, amp1);
                if (mid > 2) {
                    fn_801BCD88(arg0, 0, q1, 0.5f * (0.5f * scale));
                    fn_801BCD88(arg0, q1, mid, 0.5f * (0.5f * scale));
                }
            }
            s32 rest = n - mid;
            s32 q3 = mid + rest / 2;
            if (rest > 1) {
                f32 amp2 = 0.5f * scale * *(f32*)(arg0 + 0x34) * *(f32*)(*(u8**)arg0 + 0x4C);
                MidpointJitter(arg0 + q3 * 0xC, midNode, endNode, amp2);
                if (rest > 2) {
                    fn_801BCD88(arg0, mid, q3, 0.5f * (0.5f * scale));
                    fn_801BCD88(arg0, q3, n, 0.5f * (0.5f * scale));
                }
            }
        }
    }
}

// 0x801CFEF4
// Random vec4 uniformly between the vec4 at arg1 (min) and arg2 (max): out = min + (max - min) * rand.
void fn_801CFEF4(u8* arg0, u8* arg1, u8* arg2) {
    f32 hi0 = *(f32*)(arg2 + 0x0);
    f32 lo0 = *(f32*)(arg1 + 0x0);
    f32 hi1 = *(f32*)(arg2 + 0x4);
    f32 lo1 = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x0) = lo0 + (hi0 - lo0) * Rand01();
    f32 hi2 = *(f32*)(arg2 + 0x8);
    f32 lo2 = *(f32*)(arg1 + 0x8);
    *(f32*)(arg0 + 0x4) = lo1 + (hi1 - lo1) * Rand01();
    f32 hi3 = *(f32*)(arg2 + 0xC);
    f32 lo3 = *(f32*)(arg1 + 0xC);
    *(f32*)(arg0 + 0x8) = lo2 + (hi2 - lo2) * Rand01();
    *(f32*)(arg0 + 0xC) = lo3 + (hi3 - lo3) * Rand01();
}

// 0x801D0580
// Random vec4 in +-arg2[i], plus the offset vec4 at arg1+0x10, each component clamped to [farg0, farg1].
void fn_801D0580(u8* arg0, u8* arg1, u8* arg2, f32 farg0, f32 farg1) {
    f32 x = RandSpread(*(f32*)(arg2 + 0x0));
    *(f32*)(arg0 + 0x0) = x;
    f32 y = RandSpread(*(f32*)(arg2 + 0x4));
    *(f32*)(arg0 + 0x4) = y;
    f32 z = RandSpread(*(f32*)(arg2 + 0x8));
    *(f32*)(arg0 + 0x8) = z;
    f32 w = RandSpread(*(f32*)(arg2 + 0xC));
    *(f32*)(arg0 + 0xC) = w;
    x += *(f32*)(arg1 + 0x10);
    *(f32*)(arg0 + 0x0) = x;
    y += *(f32*)(arg1 + 0x14);
    *(f32*)(arg0 + 0x4) = y;
    z += *(f32*)(arg1 + 0x18);
    *(f32*)(arg0 + 0x8) = z;
    w += *(f32*)(arg1 + 0x1C);
    *(f32*)(arg0 + 0xC) = w;
    f32 cx = ClampAny(x, farg0, farg1);
    f32 cy = ClampAny(y, farg0, farg1);
    f32 cz = ClampAny(z, farg0, farg1);
    f32 cw = ClampAny(w, farg0, farg1);
    *(f32*)(arg0 + 0x0) = cx;
    *(f32*)(arg0 + 0x4) = cy;
    *(f32*)(arg0 + 0x8) = cz;
    *(f32*)(arg0 + 0xC) = cw;
}

// 0x801D09B0
// Random vec4 in [-1, 1]; the xyz part is then normalised (zero stays zero).
void fn_801D09B0(u8* arg0, u8* arg1) {
    f32 rx = -1.0f + 2.0f * Rand01();
    *(f32*)(arg0 + 0x0) = rx;
    f32 ry = -1.0f + 2.0f * Rand01();
    *(f32*)(arg0 + 0x4) = ry;
    f32 rz = -1.0f + 2.0f * Rand01();
    *(f32*)(arg0 + 0x8) = rz;
    f32 x = *(f32*)(arg0 + 0x0);
    f32 y = *(f32*)(arg0 + 0x4);
    f32 z = *(f32*)(arg0 + 0x8);
    f32 len2 = z * z + (x * x + y * y);
    *(f32*)(arg0 + 0xC) = -1.0f + 2.0f * Rand01();
    f32 nx = 0.0f, ny = 0.0f, nz = 0.0f;
    if (len2 != 0.0f) {
        f32 inv = RSqrtRefined(len2);
        nx = x * inv;
        ny = y * inv;
        nz = z * inv;
    }
    *(f32*)(arg0 + 0x0) = nx;
    *(f32*)(arg0 + 0x4) = ny;
    *(f32*)(arg0 + 0x8) = nz;
}

// 0x801E3A30
// Builds a quaternion from the axis/angle in arg0 (axis = -dir x side, angle at +0x44) into arg1, then
// conjugates the cross vector by it and re-derives the final rotation with fn_801458B0.
void fn_801E3A30(u8* arg0, f32* arg1) {
    f32 ax[3];
    f32 v[3];
    ax[0] = -*(f32*)(arg0 + 0x18);
    ax[1] = -*(f32*)(arg0 + 0x1C);
    ax[2] = -*(f32*)(arg0 + 0x20);
    v[0] = ax[1] * *(f32*)(arg0 + 0x2C) - ax[2] * *(f32*)(arg0 + 0x28);
    v[1] = ax[2] * *(f32*)(arg0 + 0x24) - ax[0] * *(f32*)(arg0 + 0x2C);
    v[2] = ax[0] * *(f32*)(arg0 + 0x28) - ax[1] * *(f32*)(arg0 + 0x24);
    QuatFromAxisAngle((Quat*)arg1, (Vec3*)ax, *(f32*)(arg0 + 0x44));

    f32 qx = arg1[0];
    f32 qw = arg1[3];
    f32 qy = arg1[1];
    f32 qz = arg1[2];
    f32 nx = -qx;
    f32 nz = -qz;
    f32 ny = -qy;
    f32 a = ((qy * v[2]) + ((qw * v[0]) + (qx * 0.0f))) - (qz * v[1]);
    f32 b = (((qw * 0.0f) - (qx * v[0])) - (qy * v[1])) - (qz * v[2]);
    f32 c = (qz * v[0]) + ((qy * 0.0f) + ((qw * v[1]) - (qx * v[2])));
    f32 d = (qz * 0.0f) + (((qw * v[2]) + (qx * v[1])) - (qy * v[0]));
    f32 e = ((c * nz) + ((b * nx) + (a * qw))) - (d * ny);
    f32 f = (d * qw) + (((b * nz) + (a * ny)) - (c * nx));
    f32 g = (d * nx) + ((c * qw) + ((b * ny) - (a * nz)));
    v[0] = e;
    v[1] = g;
    v[2] = f;
    fn_801458B0(arg1, v, ax);
}

}  // extern "C"
