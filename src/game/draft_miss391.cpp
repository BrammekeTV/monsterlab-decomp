// Frame-effect dispatcher, skeleton/bone buffer copy, sphere-vs-triangle test, bone matrix
// builder and the per-pass draw hook runner.
#include "types.h"
#include <math.h>

extern "C" {
void fn_80162E4C(void* fx, u8* p);                      // 0x80162E4C
void fn_80166468(void* fx, u8* p);                      // 0x80166468
void fn_80168A58(void* fx, u8* p);                      // 0x80168A58
void fn_801665E4(void* fx, u8* p);                      // 0x801665E4
void fn_80168440(void* fx, u8* p);                      // 0x80168440
void fn_80163890(void* fx, u8* p);                      // 0x80163890
void fn_8016834C(void* fx, u8* p);                      // 0x8016834C
void fn_80166114(void* fx, u8* p);                      // 0x80166114
void fn_801630F0(void* fx, u8* p);                      // 0x801630F0
void fn_80163514(void* fx, u8* p);                      // 0x80163514
void fn_801692C0(void* fx, u8* p);                      // 0x801692C0
void fn_80168E6C(void* fx, u8* p);                      // 0x80168E6C
void fn_8016578C(void* fx, u8* p);                      // 0x8016578C
void fn_80166C44(void* fx, u8* p);                      // 0x80166C44
void fn_801873C0(u8* a, s32 b, s32 c);                  // 0x801873C0

extern u8 lbl_8040E5F0[];     // 0x8040E5F0 (frame effects object)
extern u8 lbl_80348F38[];     // 0x80348F38 (shader/object dispatch table)
extern u8 lbl_804129E8[];     // 0x804129E8 (counters)
extern u8 lbl_805F9328;       // 0x805F9328 (draw enable byte)
extern void* lbl_805FB5F8;    // 0x805FB5F8 (current camera)
extern s32 lbl_805FB5FC;      // 0x805FB5FC (camera set counter)
extern u8* lbl_805FB610;      // 0x805FB610 (object being drawn)
}

typedef void (*DispatchFn)(void* obj, u8* buf, u8* entry);

// Triangle edge test for fn_8016CD60: p, q, other are sphere-relative vertices. Takes the
// foot of the perpendicular from the sphere centre onto line p-q (as the offset perp from p);
// true when that point is outside the sphere and the third vertex lies beyond it.
static bool EdgeRejects(const f32* p, const f32* q, const f32* other, f32 r2) {
    f32 ex = q[0] - p[0];
    f32 ey = q[1] - p[1];
    f32 ez = q[2] - p[2];
    f32 len2 = ez * ez + (ex * ex + ey * ey);
    f32 t = (ez * p[2] + (ex * p[0] + ey * p[1])) / len2;
    f32 px = p[0] - ex * t;
    f32 py = p[1] - ey * t;
    f32 pz = p[2] - ez * t;
    f32 d = pz * pz + (px * px + py * py);
    if (d > r2) {
        f32 o = pz * other[2] + (px * other[0] + py * other[1]);
        if (d < o) return true;
    }
    return false;
}

extern "C" {

// 0x80161398
// Per-kind update thunk: forwards the frame-effects object and record to the handler for kind.
void fn_80161398(u32 kind, u8* p) {
    switch (kind) {
    case 0:
    case 1:
    case 2:
        fn_80162E4C(lbl_8040E5F0, p);
        return;
    case 3:
        fn_80166468(lbl_8040E5F0, p);
        return;
    case 4:
        fn_80168A58(lbl_8040E5F0, p);
        return;
    case 5:
    case 6:
        fn_801665E4(lbl_8040E5F0, p);
        return;
    case 7:
        fn_80168440(lbl_8040E5F0, p);
        return;
    case 8:
        fn_80163890(lbl_8040E5F0, p);
        return;
    case 9:
        fn_8016834C(lbl_8040E5F0, p);
        return;
    case 10:
        fn_80166114(lbl_8040E5F0, p);
        return;
    case 11:
        fn_801630F0(lbl_8040E5F0, p);
        return;
    case 12:
        fn_80163514(lbl_8040E5F0, p);
        return;
    case 13:
        fn_801692C0(lbl_8040E5F0, p);
        return;
    case 14:
        fn_80168E6C(lbl_8040E5F0, p);
        return;
    case 19:
        fn_8016578C(lbl_8040E5F0, p);
        return;
    case 20:
        fn_80166C44(lbl_8040E5F0, p);
        return;
    default:  // 15..18 and anything above 20: nothing to do
        return;
    }
}

// 0x8016B024
// Copies count elements from src (starting at element srcIdx) into dst (starting at dstIdx) in a
// bone/vertex buffer; count is limited by what src holds. Raises dst's used count if needed.
// Buffer kind (+4): 0 = 0x1C-byte elements, 1 = 0xC-byte elements, other kinds copy nothing.
void fn_8016B024(u8* dst, s32 dstIdx, u8* src, s32 srcIdx, s32 maxCount) {
    s32 count = *(s32*)(src + 0x10) - srcIdx;
    if (maxCount < count) count = maxCount;
    s32 used = dstIdx + count;
    s32 oldUsed = *(s32*)(dst + 0x10);
    if (oldUsed > used) used = oldUsed;
    s32 kind = *(s32*)(dst + 0x4);
    *(s32*)(dst + 0x10) = used;
    if (kind == 1) {
        if (count <= 0) return;
        u8* d = dst + *(s32*)(dst + 0x8) + dstIdx * 0xC;
        u8* s = src + *(s32*)(src + 0x8) + srcIdx * 0xC;
        for (s32 i = 0; i < count; i++) {
            *(u32*)(d + 0x0) = *(u32*)(s + 0x0);
            *(u32*)(d + 0x4) = *(u32*)(s + 0x4);
            *(u32*)(d + 0x8) = *(u32*)(s + 0x8);
            s += 0xC;
            d += 0xC;
        }
    } else if (kind == 0) {
        if (count <= 0) return;
        u8* d = dst + *(s32*)(dst + 0x8) + dstIdx * 0x1C;
        u8* s = src + *(s32*)(src + 0x8) + srcIdx * 0x1C;
        for (s32 i = 0; i < count; i++) {
            *(f32*)(d + 0x00) = *(f32*)(s + 0x00);
            *(f32*)(d + 0x04) = *(f32*)(s + 0x04);
            *(f32*)(d + 0x08) = *(f32*)(s + 0x08);
            *(f32*)(d + 0x0C) = *(f32*)(s + 0x0C);
            *(f32*)(d + 0x10) = *(f32*)(s + 0x10);
            *(f32*)(d + 0x14) = *(f32*)(s + 0x14);
            *(f32*)(d + 0x18) = *(f32*)(s + 0x18);
            s += 0x1C;
            d += 0x1C;
        }
    }
}

// 0x8016CD60
// Sphere (centre arg0[0..2], radius arg0[3]) against triangle arg1/arg2/arg3. On a hit stores the
// travel fraction (-dist / radius) in *t, the triangle normal in normal, and returns 1.
s32 fn_8016CD60(u8* arg0, u8* arg1, u8* arg2, u8* arg3, f32* t, u8* normal) {
    f32 cx = *(f32*)(arg0 + 0x0);
    f32 cy = *(f32*)(arg0 + 0x4);
    f32 cz = *(f32*)(arg0 + 0x8);
    f32 r = *(f32*)(arg0 + 0xC);
    f32 v1x = *(f32*)(arg1 + 0x0), v1y = *(f32*)(arg1 + 0x4), v1z = *(f32*)(arg1 + 0x8);
    f32 v2x = *(f32*)(arg2 + 0x0), v2y = *(f32*)(arg2 + 0x4), v2z = *(f32*)(arg2 + 0x8);
    f32 v3x = *(f32*)(arg3 + 0x0), v3y = *(f32*)(arg3 + 0x4), v3z = *(f32*)(arg3 + 0x8);

    // Vertices relative to the sphere centre.
    f32 A[3] = {v1x - cx, v1y - cy, v1z - cz};
    f32 B[3] = {v2x - cx, v2y - cy, v2z - cz};
    f32 C[3] = {v3x - cx, v3y - cy, v3z - cz};
    // Triangle edges v2-v1 and v3-v1.
    f32 ux = v2x - v1x, uy = v2y - v1y, uz = v2z - v1z;
    f32 wx = v3x - v1x, wy = v3y - v1y, wz = v3z - v1z;

    // Bounding box of the triangle against the sphere's cube.
    f32 minx = (A[0] < B[0]) ? A[0] : B[0];
    f32 miny = (A[1] < B[1]) ? A[1] : B[1];
    f32 minz = (A[2] < B[2]) ? A[2] : B[2];
    f32 maxx = (A[0] > B[0]) ? A[0] : B[0];
    f32 maxy = (A[1] > B[1]) ? A[1] : B[1];
    f32 maxz = (A[2] > B[2]) ? A[2] : B[2];
    minx = (minx < C[0]) ? minx : C[0];
    miny = (miny < C[1]) ? miny : C[1];
    minz = (minz < C[2]) ? minz : C[2];
    maxx = (maxx > C[0]) ? maxx : C[0];
    maxy = (maxy > C[1]) ? maxy : C[1];
    maxz = (maxz > C[2]) ? maxz : C[2];
    f32 negr = -r;
    if (!(minx < r && miny < r && minz < r)) return 0;
    if (!(maxx >= negr && maxy >= negr && maxz >= negr)) return 0;

    // Triangle normal = u x w, normalised (zero if degenerate).
    f32 nx0 = uy * wz - uz * wy;
    f32 ny0 = uz * wx - ux * wz;
    f32 nz0 = ux * wy - uy * wx;
    f32 nlen2 = nz0 * nz0 + (nx0 * nx0 + ny0 * ny0);
    f32 nx, ny, nz;
    if (nlen2 == 0.0f) {
        nx = 0.0f;
        ny = 0.0f;
        nz = 0.0f;
    } else {
        f32 inv = (f32)(1.0 / sqrt((f64)nlen2));  // original: frsqrte with two Newton steps
        nx = nx0 * inv;
        ny = ny0 * inv;
        nz = nz0 * inv;
    }
    *(f32*)(normal + 0x0) = nx;
    *(f32*)(normal + 0x4) = ny;
    *(f32*)(normal + 0x8) = nz;
    if (nx == 0.0f && ny == 0.0f && nz == 0.0f) return 0;

    // Signed distance of the centre from the triangle plane.
    f32 dist = A[2] * nz + (A[0] * nx + A[1] * ny);
    if (!(dist <= r && dist >= negr)) return 0;

    f32 r2 = r * r;
    f32 dA = A[2] * A[2] + (A[0] * A[0] + A[1] * A[1]);
    f32 dB = B[2] * B[2] + (B[0] * B[0] + B[1] * B[1]);
    f32 dC = C[2] * C[2] + (C[0] * C[0] + C[1] * C[1]);
    if (dA > r2 && dB > r2 && dC > r2) {
        // All three vertices are outside the sphere: check the nearest vertex region, then edges.
        if (dA < dB) {
            if (dC < dA) {
                f32 ca = C[2] * A[2] + (C[0] * A[0] + C[1] * A[1]);
                if (ca > dC) {
                    f32 cb = C[2] * B[2] + (C[0] * B[0] + C[1] * B[1]);
                    if (cb > dC) return 0;
                }
            } else {
                f32 ab = A[2] * B[2] + (A[0] * B[0] + A[1] * B[1]);
                if (ab > dA) {
                    f32 ac = A[2] * C[2] + (A[0] * C[0] + A[1] * C[1]);
                    if (ac > dA) return 0;
                }
            }
        } else {
            if (dC < dB) {
                f32 ca = C[2] * A[2] + (C[0] * A[0] + C[1] * A[1]);
                if (ca > dC) {
                    f32 cb = C[2] * B[2] + (C[0] * B[0] + C[1] * B[1]);
                    if (cb > dC) return 0;
                }
            } else {
                f32 ba = B[2] * A[2] + (B[0] * A[0] + B[1] * A[1]);
                if (ba > dB) {
                    f32 bc = B[2] * C[2] + (B[0] * C[0] + B[1] * C[1]);
                    if (bc > dB) return 0;
                }
            }
        }
        if (EdgeRejects(A, B, C, r2)) return 0;
        if (EdgeRejects(B, C, A, r2)) return 0;
        if (EdgeRejects(C, A, B, r2)) return 0;
    }
    *t = (-dist) / r;
    return 1;
}

// 0x801704F8
// Rebuilds the object's per-bone matrices once per camera set: out[i] = (a[i] . b[i]) . (camera
// . object matrix), with the 3x3 part scaled by *scale. Returns the output array (obj + 0x38).
u8* fn_801704F8(u8* obj, f32* scale) {
    u8* out = *(u8**)(obj + 0x38);
    if (lbl_805FB5FC == *(s32*)(obj + 0xB4)) return out;
    *(s32*)(obj + 0xB4) = lbl_805FB5FC;

    const f32* m = *(const f32**)(obj + 0x4);          // object matrix (3x3 + translation)
    const f32* g = (const f32*)((u8*)lbl_805FB5F8 + 0x90);  // camera matrix

    // T = camera * object matrix.
    f32 T[12];
    T[0] = g[6] * m[2] + (g[0] * m[0] + g[3] * m[1]);
    T[1] = g[7] * m[2] + (g[1] * m[0] + g[4] * m[1]);
    T[2] = g[8] * m[2] + (g[2] * m[0] + g[5] * m[1]);
    T[3] = g[6] * m[5] + (g[0] * m[3] + g[3] * m[4]);
    T[4] = g[7] * m[5] + (g[1] * m[3] + g[4] * m[4]);
    T[5] = g[8] * m[5] + (g[2] * m[3] + g[5] * m[4]);
    T[6] = g[6] * m[8] + (g[0] * m[6] + g[3] * m[7]);
    T[7] = g[7] * m[8] + (g[1] * m[6] + g[4] * m[7]);
    T[8] = g[8] * m[8] + (g[2] * m[6] + g[5] * m[7]);
    T[9] = g[9] + (g[6] * m[11] + (g[0] * m[9] + g[3] * m[10]));
    T[10] = g[10] + (g[7] * m[11] + (g[1] * m[9] + g[4] * m[10]));
    T[11] = g[11] + (g[8] * m[11] + (g[2] * m[9] + g[5] * m[10]));

    u8* skel = *(u8**)(obj + 0xB0);
    const f32* a = *(const f32**)(skel + 0x0);
    const f32* b = *(const f32**)(skel + 0x4);
    f32* o = (f32*)out;
    for (s32 i = 0; i < *(s32*)(*(u8**)(obj + 0xB0) + 0x8); i++) {
        // M = b * a (bone-local chain).
        f32 M[12];
        M[0] = b[6] * a[2] + (b[0] * a[0] + b[3] * a[1]);
        M[1] = b[7] * a[2] + (b[1] * a[0] + b[4] * a[1]);
        M[2] = b[8] * a[2] + (b[2] * a[0] + b[5] * a[1]);
        M[3] = b[6] * a[5] + (b[0] * a[3] + b[3] * a[4]);
        M[4] = b[7] * a[5] + (b[1] * a[3] + b[4] * a[4]);
        M[5] = b[8] * a[5] + (b[2] * a[3] + b[5] * a[4]);
        M[6] = b[6] * a[8] + (b[0] * a[6] + b[3] * a[7]);
        M[7] = b[7] * a[8] + (b[1] * a[6] + b[4] * a[7]);
        M[8] = b[8] * a[8] + (b[2] * a[6] + b[5] * a[7]);
        M[9] = b[9] + (b[6] * a[11] + (b[0] * a[9] + b[3] * a[10]));
        M[10] = b[10] + (b[7] * a[11] + (b[1] * a[9] + b[4] * a[10]));
        M[11] = b[11] + (b[8] * a[11] + (b[2] * a[9] + b[5] * a[10]));

        // out = T * M, rotation part scaled, translation left unscaled.
        f32 sc = *scale;
        o[0] = (T[6] * M[2] + (T[0] * M[0] + T[3] * M[1])) * sc;
        o[1] = (T[6] * M[5] + (T[0] * M[3] + T[3] * M[4])) * sc;
        o[2] = (T[6] * M[8] + (T[0] * M[6] + T[3] * M[7])) * sc;
        o[3] = T[9] + (T[6] * M[11] + (T[0] * M[9] + T[3] * M[10]));
        o[4] = (T[7] * M[2] + (T[1] * M[0] + T[4] * M[1])) * sc;
        o[5] = (T[7] * M[5] + (T[1] * M[3] + T[4] * M[4])) * sc;
        o[6] = (T[7] * M[8] + (T[1] * M[6] + T[4] * M[7])) * sc;
        o[7] = T[10] + (T[7] * M[11] + (T[1] * M[9] + T[4] * M[10]));
        o[8] = (T[8] * M[2] + (T[2] * M[0] + T[5] * M[1])) * sc;
        o[9] = (T[8] * M[5] + (T[2] * M[3] + T[5] * M[4])) * sc;
        o[10] = (T[8] * M[8] + (T[2] * M[6] + T[5] * M[7])) * sc;
        o[11] = T[11] + (T[8] * M[11] + (T[2] * M[9] + T[5] * M[10]));
        a += 12;
        b += 12;
        o += 12;
    }
    return out;
}

// 0x80173438
// Runs the pre/post draw hooks around the draw of every object in list whose pass byte (+0xD)
// equals type, and counts the draws.
void fn_80173438(u8* list, s32 type, s32 flag, u8* buf) {
    if (lbl_805F9328 == 0) return;
    if (type == 1) {
        if (flag != 0) {
            *(s32*)(lbl_804129E8 + 0x10) += 1;
        } else {
            *(s32*)(lbl_804129E8 + 0x20) += 1;
        }
    }
    u8* node = list;
    for (s32 i = 0; i < *(s32*)(list + 0x6C); i++, node += 8) {
        u8* obj = *(u8**)(node + 0x70);
        if (type == *(u8*)(obj + 0xD)) {
            lbl_805FB610 = obj;
            *(s32*)(lbl_804129E8 + 0x34) += 1;
            u8* e = lbl_80348F38 + *(u8*)(obj + 0xC) * 4;
            (*(DispatchFn*)(e + 0x48))(*(void**)(obj + 0x8), buf, e);
            lbl_805FB610 = obj;
            fn_801873C0(*(u8**)(node + 0x74), flag, 1);
            lbl_805FB610 = NULL;
            e = lbl_80348F38 + *(u8*)(obj + 0xC) * 4;
            (*(DispatchFn*)(e + 0x54))(*(void**)(obj + 0x8), buf, e);
        }
    }
}

}  // extern "C"
