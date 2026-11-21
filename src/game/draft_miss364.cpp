// Segment-vs-triangle sweep, voice fade setup, texture buffer sizing, and a 2D GX state setup.
#include "types.h"
#include <math.h>

extern "C" {
void   DCFlushRange(void* addr, u32 size);
s32    OSDisableInterrupts(void);                                  // 0x802393C4
s32    OSRestoreInterrupts(s32 level);                             // 0x802393EC
u8     SCGetAspectRatio(void);                                     // 0x8026A9D8: 1 = 16:9
void   fn_802050B0(void* tmp, s32 n, s32 size);                    // 0x802050B0
void   fn_80205134(u8* buf, void* tmp, s32 size, s32 a, s32 b);    // 0x80205134
void   fn_80248050(f32* mtx);                                      // 0x80248050
void   fn_8024849C(f32* mtx, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);  // 0x8024849C
void   fn_8024A3A4(s32 a, s32 b);                                  // 0x8024A3A4
void   fn_8024A7E4(void);                                          // 0x8024A7E4
void   fn_8024A818(s32 a, s32 b, s32 c, s32 d, s32 e);            // 0x8024A818
void   fn_8024AE74(s32 a);                                         // 0x8024AE74
void   fn_8024BB64(s32 a);                                         // 0x8024BB64
void   fn_8024C2F0(s32 a);                                         // 0x8024C2F0
void   fn_8024C3B8(u8* obj, void* image, s32 w, s32 h, s32 fmt, s32 wrapS, s32 wrapT, s32 mip);  // 0x8024C3B8
void   fn_8024C60C(u8* obj, s32 a, s32 b, s32 c, s32 d, s32 e, f32 f, f32 g, f32 h);  // 0x8024C60C
void   fn_8024D508(s32 a, s32 b);                                  // 0x8024D508
void   fn_8024DA08(s32 a, s32 b, s32 c, s32 d);                    // 0x8024DA08
void   fn_8024DB64(s32 a);                                         // 0x8024DB64
void   fn_8024DED8(s32 a, s32 b, s32 c, s32 d);                    // 0x8024DED8
void   fn_8024DF80(s32 a, s32 b, s32 c);                           // 0x8024DF80
void   fn_8024E1D0(f32* mtx, s32 a);                               // 0x8024E1D0
void   fn_8024E280(f32* mtx, s32 a);                               // 0x8024E280
void   fn_8024E328(s32 a);                                         // 0x8024E328
extern u8 lbl_8051E6A8[];          // 0x8051E6A8 (sound voices, 0x114 each)
}

// Smallest power of two above v (v is not a power of two here): 1 << (32 - cntlzw(v)).
static u32 NextPow2(u32 v) {
    u32 bits = 0;
    while (v) {
        v >>= 1;
        bits++;
    }
    return 1u << bits;
}

// Reciprocal square root, refined like the frsqrte + two Newton steps in the original.
static f32 RSqrtRefined(f32 x) { return (f32)(1.0 / sqrt((f64)x)); }

extern "C" {

// 0x801E41E4
// Sweeps the segment org -> end against `count` triangles (0x48 bytes each: three vertices at
// +0x18, +0x0, +0xC). Keeps the nearest hit with t < 1: writes hit point (+0), normal (+0xC),
// t (+0x18) and the triangle's tag (+0x44 -> +0x48) to out. Returns 1 on a hit.
s32 fn_801E41E4(f32* org, f32* end, u8* tri, s32 count, f32* out) {
    f32 dx = end[0] - org[0];
    f32 dy = end[1] - org[1];
    f32 dz = end[2] - org[2];
    f32 hitPosX = 0.0f, hitPosY = 0.0f, hitPosZ = 0.0f;
    f32 hitNX = 0.0f, hitNY = 0.0f, hitNZ = 0.0f;
    f32 bestT = 1.1f;
    s32 hitTag = 0;

    do {
        f32 v0x = *(f32*)(tri + 0x18);
        f32 v0y = *(f32*)(tri + 0x1C);
        f32 v0z = *(f32*)(tri + 0x20);
        f32 ax = *(f32*)(tri + 0x0) - v0x;
        f32 ay = *(f32*)(tri + 0x4) - v0y;
        f32 az = *(f32*)(tri + 0x8) - v0z;
        f32 bx = *(f32*)(tri + 0xC) - v0x;
        f32 by = *(f32*)(tri + 0x10) - v0y;
        f32 bz = *(f32*)(tri + 0x14) - v0z;

        // h = d x a
        f32 hx = dy * az - dz * ay;
        f32 hy = dz * ax - dx * az;
        f32 hz = dx * ay - dy * ax;
        f32 det = (bz * hz) + ((bx * hx) + (by * hy));

        if (det > 0.00000001f) {
            f32 sx = org[0] - v0x;
            f32 sy = org[1] - v0y;
            f32 sz = org[2] - v0z;
            f32 eps = det * 0.00001f;
            f32 negEps = -eps;
            f32 hiLim = det + eps;
            f32 u = (sz * hz) + ((sx * hx) + (sy * hy));
            if (u >= negEps && u <= hiLim) {
                // q = s x b
                f32 qx = sy * bz - sz * by;
                f32 qy = sz * bx - sx * bz;
                f32 qz = sx * by - sy * bx;
                f32 v = (dz * qz) + ((dx * qx) + (dy * qy));
                if (v >= negEps && (u + v) <= hiLim) {
                    f32 t = ((az * qz) + ((ax * qx) + (ay * qy))) * (1.0f / det);
                    if (t >= 0.0f && t <= 1.0f) {
                        // Triangle normal (b x a), normalised.
                        f32 nx = by * az - bz * ay;
                        f32 ny = bz * ax - bx * az;
                        f32 nz = bx * ay - by * ax;
                        f32 len2 = (nz * nz) + ((nx * nx) + (ny * ny));
                        f32 n0, n1, n2;
                        if (len2 == 0.0f) {
                            n0 = 0.0f;
                            n1 = 0.0f;
                            n2 = 0.0f;
                        } else {
                            f32 inv = RSqrtRefined(len2);
                            n0 = nx * inv;
                            n1 = ny * inv;
                            n2 = nz * inv;
                        }
                        if (t < bestT) {
                            // Degenerate normal falls back to +Z.
                            if (n0 <= 0.0001f && n0 >= -0.0001f && n1 <= 0.0001f && n1 >= -0.0001f &&
                                n2 <= 0.0001f && n2 >= -0.0001f) {
                                n0 = 0.0f;
                                n1 = 0.0f;
                                n2 = 1.0f;
                            }
                            hitPosX = org[0] + dx * t;
                            hitPosY = org[1] + dy * t;
                            hitPosZ = org[2] + dz * t;
                            hitNX = n0;
                            hitNY = n1;
                            hitNZ = n2;
                            bestT = t;
                            hitTag = *(s32*)(tri + 0x44);
                        }
                    }
                }
            }
        }
        count -= 1;
        tri += 0x48;
    } while (count > 0);

    if (bestT <= 1.0f) {
        out[0] = hitPosX;
        out[1] = hitPosY;
        out[2] = hitPosZ;
        out[3] = hitNX;
        out[4] = hitNY;
        out[5] = hitNZ;
        out[6] = bestT;
        *(s32*)((u8*)out + 0x48) = hitTag;
        return 1;
    }
    return 0;
}

// 0x80200494
// Sets a sound voice's volume ramp (voice slot index at +8): stores the target params, the
// delta from the old value, and a 0..23 step derived from where `pos` falls in the range at
// +0x3C..+0x40 (only in its upper half).
void fn_80200494(u8* obj, f32 a, f32 b, f32 c, f32 d) {
    u8* voice = lbl_8051E6A8 + (*(u32*)(obj + 0x8) * 0x114);
    f32 old = *(f32*)(voice + 0x34);
    s32 level = OSDisableInterrupts();
    *(f32*)(voice + 0x2C) = a;
    *(f32*)(voice + 0x30) = b;
    u32 step = 0;
    *(f32*)(voice + 0x34) = d;
    *(f32*)(voice + 0x38) = old - d;
    *(s32*)(voice + 0x3C) = 1;
    f32 lo = *(f32*)(obj + 0x3C);
    f32 hi = *(f32*)(obj + 0x40);
    f32 rel = c - lo;
    f32 range = hi - lo;
    f32 mid = (f32)((f64)lo + 0.5 * (f64)range);
    if (range > 0.0f && rel > mid) {
        step = (u32)(23.0f * ((rel - mid) / range));
        if (step > 0x17u) step = 0x17;
    }
    *(u32*)(voice + 0x80) = step;
    *(s32*)(voice + 0x84) = 1;
    OSRestoreInterrupts(level);
}

// 0x80205208
// Builds a 4-row texture in buf from n bits: width = n/8 rounded to 4 and then up to a power of
// two. Returns the flushed byte count (width * 8 rounded to 0x80).
s32 fn_80205208(u8* buf, u8* texObj, s32* outWidth, s32* outHeight, s32 n) {
    u8 tmp[0x800];
    u32 width = (((u32)n >> 3) + 3) & 0xFFFFFFFCu;
    if (width & (width - 1)) {
        width = NextPow2(width);
    }
    *outWidth = (s32)width;
    *outHeight = 4;
    u32 bytes = ((width * 8) + 0x7F) & 0xFFFFFF80u;
    fn_802050B0(tmp, n, (s32)width);
    fn_80205134(buf, tmp, (s32)width, 4, 2);
    DCFlushRange(buf, bytes);
    fn_8024C3B8(texObj, buf, (s32)(u16)width, 4, 3, 1, 1, 0);
    fn_8024C60C(texObj, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f);
    return (s32)bytes;
}

// 0x80205310
// Computes plane sizes/offsets (all rounded to 0x80) of a video frame descriptor: luma and two
// chroma planes, and the total size of the two power-of-two row buffers.
void fn_80205310(u8* p, s32* arg1, s32* arg2, s32* arg3, s32* arg4, s32* arg5, u32* arg6, s32* arg7) {
    u32 a = (*(s32*)(p + 0x18) != 0) ? *(u32*)(p + 0x4) : 0;
    u32 b = (*(s32*)(p + 0x3C) != 0) ? *(u32*)(p + 0x4) : 0;
    u32 c = (*(s32*)(p + 0x24) != 0) ? *(u32*)(p + 0xC) : 0;
    s32 height = *(s32*)(p + 0x8);
    u32 w0 = (((u32)*(s32*)(p + 0x4) >> 3) + 3) & 0xFFFFFFFCu;
    s32 stride = *(s32*)(p + 0x10);
    u32 planeA = ((a * height) + 0x7F) & 0xFFFFFF80u;
    u32 planeC = ((c * stride) + 0x7F) & 0xFFFFFF80u;
    u32 planeB = ((b * height) + 0x7F) & 0xFFFFFF80u;
    if (w0 & (w0 - 1)) {
        w0 = NextPow2(w0);
    }
    u32 w1 = (((u32)*(s32*)(p + 0xC) >> 3) + 3) & 0xFFFFFFFCu;
    if (w1 & (w1 - 1)) {
        w1 = NextPow2(w1);
    }
    u32 ab = planeA + planeB;
    *arg5 = (s32)((((w0 * 8) + 0x7F) & 0xFFFFFF80u) + (((w1 * 8) + 0x7F) & 0xFFFFFF80u));
    *arg3 = (s32)planeA;
    *arg1 = (s32)ab;
    *arg2 = (s32)(planeC + ab);
    *arg4 = (s32)(ab + planeC * 2);
    *arg6 = a | b;
    *arg7 = (s32)c;
}

// 0x8020574C
// Sets up GX for 2D drawing: one texgen/channel, replace TEV, no culling, identity modelview,
// and a 640x480 orthographic projection (same for 4:3 and 16:9).
void fn_8020574C(void) {
    f32 mv[12];
    f32 proj[16];

    fn_8024A7E4();
    fn_8024A818(4, 9, 0, 4, 0);
    fn_8024A818(4, 0xB, 0, 1, 0);
    fn_8024A3A4(9, 1);
    fn_8024A3A4(0xB, 1);
    fn_8024C2F0(1);
    fn_8024AE74(0);
    fn_8024DB64(1);
    fn_8024DA08(0, 0xFF, 0xFF, 4);
    fn_8024D508(0, 4);
    fn_8024DED8(0, 0, 0, 0);
    fn_8024DF80(1, 3, 1);
    fn_8024E328(3);
    fn_80248050(mv);
    fn_8024E280(mv, 3);
    fn_8024BB64(0);
    if (SCGetAspectRatio() == 1) {
        fn_8024849C(proj, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 500.0f);
    } else {
        fn_8024849C(proj, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 500.0f);
    }
    fn_8024E1D0(proj, 1);
}

}  // extern "C"
