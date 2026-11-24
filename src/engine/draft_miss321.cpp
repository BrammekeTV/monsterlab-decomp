// Drafts miss321: OBB-vs-OBB separating-axis test (0x801C10B4), ribbon/trail draw (0x801C4338) and
// particle emitter step (0x801C9260).
#include <math.h>

#include "types.h"
#include "revolution.h"

extern "C++" u8 sRendDLs[];           // 0x80401108 (display-list records, 0x94 bytes each)
extern "C++" void fn_80154358(void);  // 0x80154358 (C++ linkage, defined in small_8015.cpp)

extern "C" {
void fn_8014DB90(u8* verts, s32 count);                         // 0x8014DB90 (submits the vertex strip)
void fn_8014DE68(void);                                         // 0x8014DE68
void fn_8014E3EC(s32 n);                                        // 0x8014E3EC
void fn_80155D70(void);                                         // 0x80155D70
s32  fn_8015BA30(void* p);                                      // 0x8015BA30
u8*  fn_8016056C(u8* table, s32 handle, f32* v);                // 0x8016056C
void fn_801C85F0(u8* obj, s32 count, void* fxMgr);                          // 0x801C85F0 (spawns count particles)
void fn_801CA7B0(u32 mode);                                     // 0x801CA7B0
f32  sinf(f32 x);                                               // 0x8033F0A4 (MSL)

extern u8  lbl_80348950[];     // 0x80348950 (0x20-byte records, u32 at +0)
extern u8  lbl_80348A10[];     // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32 lbl_80348AD0;       // 0x80348AD0
extern s32 lbl_8040ADF0[8];    // 0x8040ADF0
extern s32 lbl_8040AE10[8];    // 0x8040AE10
extern u8  lbl_8040E260[];     // 0x8040E260 (effect handle pool)
extern u8  lbl_805F9212;       // 0x805F9212
extern s32 lbl_805FB4B0;       // 0x805FB4B0
extern s32 lbl_805FB4BC;       // 0x805FB4BC (current viewport index)
extern s32 lbl_805FB4C0;       // 0x805FB4C0 (display-list recording flag)
extern s32 lbl_805FB4C4;       // 0x805FB4C4 (vertex buffer write offset)
extern s32 lbl_805FB4C8;       // 0x805FB4C8 (render budget)
extern s32 lbl_805FB4CC;       // 0x805FB4CC (vertex buffer bytes left)
extern u8* lbl_805FB4D0;       // 0x805FB4D0 (vertex buffer base)
extern u8  lbl_805FB50C;       // 0x805FB50C (GX shadow flags byte)
}

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// One XF-style write: opcode byte, two halfwords, one word.
static void GxXfWrite(u8 op, u16 a, u16 b, u32 value) {
    GXWGFifo._u8  = op;
    GXWGFifo._u16 = a;
    GXWGFifo._u16 = b;
    GXWGFifo._u32 = value;
}

// Switches the line-strip draw state on (flag bit 0 selects one of two register sets).
static void GxLineStateOn(bool alt) {
    if (alt) {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000001);
        GxXfWrite(0x10, 0, 0x103F, 1);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000400);
        GxXfWrite(0x10, 0, 0x1010, 0x441);
        GxBpWrite(0x283C0000);
        GxBpWrite(0xC008F2A4);
        GxBpWrite(0xC108E6F0);
        GxBpWrite(0xC208F80F);
        GxBpWrite(0xC308F070);
    } else {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000001);
        GxXfWrite(0x10, 0, 0x103F, 1);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x100E, 0x442);
        GxXfWrite(0x10, 0, 0x1010, 0x442);
        GxBpWrite(0x28380040);
        GxBpWrite(0xC008F8AF);
        GxBpWrite(0xC108F2F0);
    }
}

// Switches the line-strip draw state back off.
static void GxLineStateOff(bool alt) {
    if (alt) {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x103F, 0);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x1010, 0x441);
        GxBpWrite(0x28380000);
        GxBpWrite(0xC008F2A4);
        GxBpWrite(0xC108E6F0);
    } else {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x103F, 0);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x100E, 0x442);
        GxXfWrite(0x10, 0, 0x1010, 0x442);
        GxBpWrite(0x28380000);
        GxBpWrite(0xC008FFFA);
        GxBpWrite(0xC108FFD0);
    }
}

// Reciprocal square root, refined like the frsqrte + two Newton steps in the original.
static f32 RSqrtRefined(f32 x) { return (f32)(1.0 / sqrt((f64)x)); }

// Builds the orthonormal frame (w, n, m) around the direction v (n = v normalised, w = n x up with
// up = +Z, or +Y when n is parallel to it, m = w x n) and applies it to d:
// out = w * d[0] + n * d[1] + m * d[2].
static void FrameTransform(const f32* v, const f32* d, f32* out) {
    f32 len2 = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
    f32 nx, ny, nz;
    if (len2 == 0.0f) {
        nx = 0.0f;
        ny = 0.0f;
        nz = 0.0f;
    } else {
        f32 r = RSqrtRefined(len2);
        nx = v[0] * r;
        ny = v[1] * r;
        nz = v[2] * r;
    }
    f32 ux = 0.0f, uy = 0.0f, uz = 1.0f;
    f32 dot = fabsf(uz * nz + (ux * nx + uy * ny)) - 1.0f;
    if (dot > -0.0001f && dot < 0.0001f) {
        ux = 0.0f;
        uy = 1.0f;
        uz = 0.0f;
    }
    f32 wx = ny * uz - nz * uy;
    f32 wy = nz * ux - nx * uz;
    f32 wz = nx * uy - ny * ux;
    f32 mx = wy * nz - wz * ny;
    f32 my = wz * nx - wx * nz;
    f32 mz = wx * ny - wy * nx;
    out[0] = 0.0f + (mx * d[2] + (wx * d[0] + nx * d[1]));
    out[1] = 0.0f + (my * d[2] + (wy * d[0] + ny * d[1]));
    out[2] = 0.0f + (mz * d[2] + (wz * d[0] + nz * d[1]));
}

extern "C" {

// 0x801C10B4
// Oriented-box overlap test (separating axis theorem, 15 axes). Arguments are the two boxes as
// (centre vec3, half extents vec3, 3x3 axis matrix of floats, rows = box axes). Returns 1 when the
// boxes overlap, 0 as soon as a separating axis is found.
s32 fn_801C10B4(u8* arg0, u8* arg1, u8* arg2, u8* arg3, u8* arg4, u8* arg5) {
    const f32* pa = (const f32*)arg0;  // centre A
    const f32* ea = (const f32*)arg1;  // half extents A
    const f32* ra = (const f32*)arg2;  // axes A
    const f32* pb = (const f32*)arg3;  // centre B
    const f32* eb = (const f32*)arg4;  // half extents B
    const f32* rb = (const f32*)arg5;  // axes B

    // cIJ = axisA_I . axisB_J, aIJ = |cIJ|, tAI / tBJ = centre offset projected on axis.
    f32 c00 = ra[2] * rb[2] + (ra[0] * rb[0] + ra[1] * rb[1]);
    f32 c01 = ra[2] * rb[5] + (ra[0] * rb[3] + ra[1] * rb[4]);
    f32 c02 = ra[2] * rb[8] + (ra[0] * rb[6] + ra[1] * rb[7]);
    f32 t0 = pb[0] - pa[0];
    f32 t1 = pb[1] - pa[1];
    f32 t2 = pb[2] - pa[2];
    f32 a00 = fabsf(c00);
    f32 a01 = fabsf(c01);
    f32 a02 = fabsf(c02);
    f32 ta0 = ra[2] * t2 + (ra[0] * t0 + ra[1] * t1);
    if (fabsf(ta0) > a02 * eb[2] + (a00 * eb[0] + a01 * eb[1]) + ea[0]) return 0;

    f32 c10 = ra[5] * rb[2] + (ra[3] * rb[0] + ra[4] * rb[1]);
    f32 c11 = ra[5] * rb[5] + (ra[3] * rb[3] + ra[4] * rb[4]);
    f32 c12 = ra[5] * rb[8] + (ra[3] * rb[6] + ra[4] * rb[7]);
    f32 a10 = fabsf(c10);
    f32 a11 = fabsf(c11);
    f32 a12 = fabsf(c12);
    f32 ta1 = ra[5] * t2 + (ra[3] * t0 + ra[4] * t1);
    if (fabsf(ta1) > a12 * eb[2] + (a10 * eb[0] + a11 * eb[1]) + ea[1]) return 0;

    f32 c20 = ra[8] * rb[2] + (ra[6] * rb[0] + ra[7] * rb[1]);
    f32 c21 = ra[8] * rb[5] + (ra[6] * rb[3] + ra[7] * rb[4]);
    f32 c22 = ra[8] * rb[8] + (ra[6] * rb[6] + ra[7] * rb[7]);
    f32 a20 = fabsf(c20);
    f32 a21 = fabsf(c21);
    f32 a22 = fabsf(c22);
    f32 ta2 = ra[8] * t2 + (ra[6] * t0 + ra[7] * t1);
    if (fabsf(ta2) > a22 * eb[2] + (a20 * eb[0] + a21 * eb[1]) + ea[2]) return 0;

    // Axes of B.
    f32 tb0 = rb[2] * t2 + (rb[0] * t0 + rb[1] * t1);
    if (fabsf(tb0) > a20 * ea[2] + (a00 * ea[0] + a10 * ea[1]) + eb[0]) return 0;
    f32 tb1 = rb[5] * t2 + (rb[3] * t0 + rb[4] * t1);
    if (fabsf(tb1) > a21 * ea[2] + (a01 * ea[0] + a11 * ea[1]) + eb[1]) return 0;
    f32 tb2 = rb[8] * t2 + (rb[6] * t0 + rb[7] * t1);
    if (fabsf(tb2) > a22 * ea[2] + (a02 * ea[0] + a12 * ea[1]) + eb[2]) return 0;

    // Cross products of axes (A_I x B_J).
    if (fabsf(ta2 * c10 - ta1 * c20) > (a20 * ea[1] + a10 * ea[2]) + (a02 * eb[1] + a01 * eb[2])) return 0;
    if (fabsf(ta2 * c11 - ta1 * c21) > (a21 * ea[1] + a11 * ea[2]) + (a02 * eb[0] + a00 * eb[2])) return 0;
    if (fabsf(ta2 * c12 - ta1 * c22) > (a22 * ea[1] + a12 * ea[2]) + (a01 * eb[0] + a00 * eb[1])) return 0;
    if (fabsf(ta0 * c20 - ta2 * c00) > (a20 * ea[0] + a00 * ea[2]) + (a12 * eb[1] + a11 * eb[2])) return 0;
    if (fabsf(ta0 * c21 - ta2 * c01) > (a21 * ea[0] + a01 * ea[2]) + (a12 * eb[0] + a10 * eb[2])) return 0;
    if (fabsf(ta0 * c22 - ta2 * c02) > (a22 * ea[0] + a02 * ea[2]) + (a11 * eb[0] + a10 * eb[1])) return 0;
    if (fabsf(ta1 * c00 - ta0 * c10) > (a10 * ea[0] + a00 * ea[1]) + (a22 * eb[1] + a21 * eb[2])) return 0;
    if (fabsf(ta1 * c01 - ta0 * c11) > (a11 * ea[0] + a01 * ea[1]) + (a22 * eb[0] + a20 * eb[2])) return 0;
    f32 lhs = fabsf(ta1 * c02 - ta0 * c12);
    f32 rhs = (a12 * ea[0] + a02 * ea[1]) + (a21 * eb[0] + a20 * eb[1]);
    return !(lhs > rhs);
}

// 0x801C4338
// Draws a trail/ribbon node as a vertex strip. For flag 0x4 first spreads the u coordinates
// along the strip; sets up the textured line state from the node def (texture record at
// def+0x6C, image at def+0x70) or switches it off when there is none, then copies pairs of
// 0x18-byte point records (position, optional sine wobble on z) into the vertex buffer and
// submits them.
void fn_801C4338(u8* arg0) {
    u32 flags = *(u32*)(arg0 + 0x4);
    if (!(flags & 1)) return;
    u32 count = *(u32*)(arg0 + 0x8);
    if (count < 4) return;

    if (flags & 4) {
        f32 step = *(f32*)(*(u8**)arg0 + 0x38) / (f32)count;
        u32 i = 0;
        u32 off = 0;
        while (i < *(u32*)(arg0 + 0x8)) {
            u8* e = arg0 + off;
            off += 0x30;
            *(f32*)(e + 0xC) = (f32)i * step;
            *(f32*)(e + 0x24) = (f32)i * step;
            i += 2;
        }
    }
    fn_801CA7B0(*(u32*)(*(u8**)arg0 + 0x74));
    GxBpWrite(0xFE00000F);
    GxBpWrite(0x80000001);

    u8* def = *(u8**)arg0;
    u8* tex = *(u8**)(def + 0x6C);
    u8* f = *(u8**)(def + 0x70);
    u8 hasTex = *(u8*)(tex + 0x6);
    if (hasTex != 0 && f != NULL) {
        if (lbl_805FB4C0 != 0 && fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0 > lbl_805FB4C8) {
            fn_8014DE68();
        }
        if (!(lbl_805FB50C & 2)) {
            lbl_805FB50C = lbl_805FB50C | 2;
            GxLineStateOn((lbl_805FB50C & 1) != 0);
        }
        GxBpWrite(0xFE000060);
        GxBpWrite(((u32)*(u8*)(f + 0x16) << 5) | 0x80000000u);
        if ((u32)lbl_8040ADF0[0] != *(u32*)(f + 0x1C) || lbl_8040AE10[0] != 0) {
            lbl_805FB4B0 = lbl_805FB4B0 + 1;
            lbl_8040ADF0[0] = *(s32*)(f + 0x1C);
            lbl_8040AE10[0] = 0;
            u32 cfg = lbl_80348AD0;
            u8 texIdx = *(u8*)(f + 0x17);
            s32 second = *(s32*)(lbl_80348A10 + texIdx * 0x20);
            GxBpWrite((((u32)*(u8*)(f + 0x13) - 1) & 0xF) << 12 | 0x84000000u);
            GxBpWrite(((u32)*(u16*)(f + 0x8) - 1) | (((u32)*(u16*)(f + 0xA) - 1) << 10) |
                      ((u32)*(u8*)(f + 0x14) << 20) | 0x88000000u);
            GxBpWrite((*(u32*)(lbl_80348950 + texIdx * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
            if (second >= 0) {
                GxBpWrite(((u32)second >> 5) | 0x900D0000u | 0x8000u);
            } else {
                GxBpWrite(0x90000000u);
            }
            GxBpWrite(((*(u32*)(f + 0x1C) >> 5) & 0xFFFFFF) | 0x94000000u);
            if (*(u8*)(f + 0x11) != 0) {
                u32 g = (cfg >> 9) & 0x3FF;
                GxBpWrite(((g | 0x98000000u) & ~0x3FC00u) | (((u32)*(u8*)(f + 0x15) << 10) & 0x3FC00u));
                GxBpWrite(0xFEFFFF00u);
                GxBpWrite(0x0F000000u);
                GxBpWrite(((*(u32*)(f + 0x20) >> 5) & 0xFFFFFF) | 0x64000000u);
                GxBpWrite(((g | 0x65000000u) & ~0x1FC00u) |
                          ((((s32)*(u8*)(f + 0x11) >> 1) << 10) & 0x1FC00u));
                GxBpWrite(0xFEFFFF00u);
                GxBpWrite(0x0F000000u);
            }
        }
        GxBpWrite(((u32)*(u16*)(f + 0x8) - 1) | 0x30000000u);
        GxBpWrite(((u32)*(u16*)(f + 0xA) - 1) | 0x31000000u);
    } else if (lbl_805FB50C & 2) {
        u8 old = lbl_805FB50C;
        lbl_805FB50C = old & ~2;
        GxLineStateOff((old & 1) != 0);
    }

    fn_80154358();
    fn_80155D70();
    lbl_805FB50C = lbl_805FB50C & ~8;
    GxBpWrite(0xFE00C000);
    GxBpWrite(0);

    count = *(u32*)(arg0 + 0x8);
    s32 pairs = (s32)(count >> 1);
    s32 size = (s32)((((count & ~1u) * 0x18) + 0x1F) & ~0x1Fu);
    if (lbl_805FB4CC < size) fn_8014E3EC(size);
    s32 pos = lbl_805FB4C4;
    u8* dst = lbl_805FB4D0 + pos;
    lbl_805FB4C4 = pos + size;
    lbl_805FB4CC = lbl_805FB4CC - size;

    u8* src = arg0 + 0xC;
    u8* d = dst;
    for (s32 k = 0; k < pairs; k++) {
        u8* p = src;
        for (u32 j = 0; j < 2; j++) {
            *(u32*)(d + 0xC) = *(u32*)(p + 0x8);
            *(f32*)(d + 0x10) = *(f32*)(p + 0x0);
            *(f32*)(d + 0x14) = *(f32*)(p + 0x4);
            u8* cdef = *(u8**)arg0;
            f32 z;
            if (*(s32*)(cdef + 0x54) != 0 && k < pairs - 1) {
                f32 phase = 0.011111111f * (360.0f * *(f32*)(arg0 + 0x394)) + (f32)k * *(f32*)(cdef + 0x60);
                f32 s = sinf(1.5707964f * phase);
                z = *(f32*)(p + 0x14) + *(f32*)(*(u8**)arg0 + 0x58) * s;
            } else {
                z = *(f32*)(p + 0x14);
            }
            *(f32*)(d + 0x0) = *(f32*)(p + 0xC);
            *(f32*)(d + 0x4) = *(f32*)(p + 0x10);
            *(f32*)(d + 0x8) = z;
            p += 0x18;
            d += 0x18;
        }
        src += 0x30;
    }
    fn_8014DB90(dst, pairs);
    lbl_805FB50C = lbl_805FB50C | 8;
    GxBpWrite(0xFE00C000);
    GxBpWrite((u32)lbl_805F9212 << 14);
    fn_80154358();
    GxBpWrite(0xFE00000F);
    GxBpWrite(0x80000005);
}

// 0x801C9260
// Emitter step for a particle emitter node (def at +0x5C). Does nothing when the node is disabled
// (flag 0x4), not active (flag 0x2 clear) or its timer at +0x4 is above zero. Otherwise works out
// how many spawn intervals (+0xC) fit in the elapsed negative time, rewinds the timer, derives the
// movement since the previous frame (corrected by frames built around the direction vectors at
// +0x40 and +0x30 unless the def offsets at +0x3C..0x48 are all near zero), scales the spawn count
// by how far the emitter moved against def+0x1E0, clamps it to 100 and the def cap (+0x1AC), spawns
// that many particles with fn_801C85F0 and latches the current position/direction as the previous.
// arg1 (owner / fxMgr) is forwarded to fn_801C85F0; farg0 (dt) is unused.
void fn_801C9260(u8* arg0, u8* arg1, f32 farg0) {
    u32 flags = *(u32*)arg0;
    if (flags & 4) return;
    f32 t = *(f32*)(arg0 + 0x4);
    if (!(t <= 0.0f)) return;
    if (!(flags & 2)) return;

    f32 negT = -t;
    f32 interval = *(f32*)(arg0 + 0xC);
    s32 n = (s32)(1.0f + negT / interval);
    u8* def = *(u8**)(arg0 + 0x5C);

    f32 delta[4];
    delta[0] = *(f32*)(arg0 + 0x10) - *(f32*)(arg0 + 0x20);
    delta[1] = *(f32*)(arg0 + 0x14) - *(f32*)(arg0 + 0x24);
    delta[2] = *(f32*)(arg0 + 0x18) - *(f32*)(arg0 + 0x28);
    delta[3] = *(f32*)(arg0 + 0x1C) - *(f32*)(arg0 + 0x2C);
    *(f32*)(arg0 + 0x4) = interval - (negT - (f32)(n - 1) * interval);

    f32 o0 = *(f32*)(def + 0x3C);
    f32 o1 = *(f32*)(def + 0x40);
    f32 o2 = *(f32*)(def + 0x44);
    f32 o3 = *(f32*)(def + 0x48);
    bool offsetZero = o0 <= 0.0001f && o0 >= -0.0001f && o1 <= 0.0001f && o1 >= -0.0001f &&
                      o2 <= 0.0001f && o2 >= -0.0001f && o3 <= 0.0001f && o3 >= -0.0001f;
    if (!offsetZero) {
        f32 d[3];
        d[0] = *(f32*)(def + 0x2C);
        d[1] = *(f32*)(def + 0x30);
        d[2] = *(f32*)(def + 0x34);
        f32 d3 = *(f32*)(def + 0x38);
        f32 prev[3];
        f32 cur[3];
        FrameTransform((f32*)(arg0 + 0x40), d, prev);
        FrameTransform((f32*)(arg0 + 0x30), d, cur);
        f32 p0 = prev[0] + *(f32*)(arg0 + 0x20);
        f32 p1 = prev[1] + *(f32*)(arg0 + 0x24);
        f32 p2 = prev[2] + *(f32*)(arg0 + 0x28);
        f32 p3 = d3 + *(f32*)(arg0 + 0x2C);
        f32 c0 = cur[0] + *(f32*)(arg0 + 0x10);
        f32 c1 = cur[1] + *(f32*)(arg0 + 0x14);
        f32 c2 = cur[2] + *(f32*)(arg0 + 0x18);
        f32 c3 = d3 + *(f32*)(arg0 + 0x1C);
        delta[0] = c0 - p0;
        delta[1] = c1 - p1;
        delta[2] = c2 - p2;
        delta[3] = c3 - p3;
    }

    f32 scale;
    f32 ref = *(f32*)(def + 0x1E0);
    if (ref == 0.0f) {
        scale = 1.0f;
    } else {
        f32 dist2 = delta[2] * delta[2] + (delta[0] * delta[0] + delta[1] * delta[1]);
        if (dist2 < ref * ref) {
            scale = 1.0f;
        } else {
            f32 dist = 0.0f;
            if (dist2 > 0.0f) dist = (f32)sqrt((f64)dist2);
            scale = dist / ref;
            if (!(scale > 1.0f)) scale = 1.0f;
        }
    }

    s32 cnt = (s32)((f32)n * scale);
    s32 cap = *(s32*)(def + 0x1AC);
    if (cap > 0) {
        s32 live = *(s32*)(arg0 + 0x64);
        if (cnt + live > cap) cnt = cap - live;
    }
    if (cnt > 0x64) cnt = 0x64;
    if (cnt < 0) cnt = 0;
    if (cnt > 0) fn_801C85F0(arg0, cnt, arg1);  // arg1 (owner / fxMgr) is passed through unchanged

    f32 px = *(f32*)(arg0 + 0x10);
    *(f32*)(arg0 + 0x20) = px;
    *(f32*)(arg0 + 0x24) = *(f32*)(arg0 + 0x14);
    *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x18);
    *(f32*)(arg0 + 0x2C) = *(f32*)(arg0 + 0x1C);
    *(f32*)(arg0 + 0x40) = *(f32*)(arg0 + 0x30);
    *(f32*)(arg0 + 0x44) = *(f32*)(arg0 + 0x34);
    *(f32*)(arg0 + 0x48) = *(f32*)(arg0 + 0x38);
    *(f32*)(arg0 + 0x4C) = *(f32*)(arg0 + 0x3C);
    if (*(s32*)(arg0 + 0x78) != 0) {
        f32 pos[3];
        pos[0] = px;
        pos[1] = *(f32*)(arg0 + 0x14);
        pos[2] = *(f32*)(arg0 + 0x18);
        fn_8016056C(lbl_8040E260, *(s32*)(arg0 + 0x78), pos);
    }
}

}  // extern "C"
