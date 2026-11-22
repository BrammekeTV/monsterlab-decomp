// Drafts miss374: ground/ray collision helpers (0x8016BB04, 0x8016C2AC), the effects overlay pass
// (0x801620BC) and a view-volume test (0x8016EC2C).
#include <math.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"

typedef f32 Scalar;

extern "C++" u8 sRendDLs[];   // 0x80401108 (display-list records, 0x94 bytes each)

extern "C" {
void fn_80161398(u32 kind, u8* p);                              // 0x80161398 (per-kind update thunk)
void fn_80161810(u8* p);                                        // 0x80161810
void fn_80161BAC(u8* p);                                        // 0x80161BAC
void fn_8015ECA8(u8* p);                                        // 0x8015ECA8
void fn_8015F4CC(u8* p);                                        // 0x8015F4CC
void fn_80156CD8(void);                                         // 0x80156CD8 (empty)
void fn_80156F40(s32 i);                                        // 0x80156F40
void fn_80156F58(s32 i);                                        // 0x80156F58
void fn_80166FC4(u8* a, u8* b);                                 // 0x80166FC4
void fn_801649B4(u8* a, u8* b);                                 // 0x801649B4
void fn_80163C60(u8* a, u8* b);                                 // 0x80163C60
void fn_80165EF0(u8* a, u8* b);                                 // 0x80165EF0
u8*  fn_80156AC0(s32 a);                                        // 0x80156AC0
void fn_80150014(void);                                         // 0x80150014
void fn_80155D70(void);                                         // 0x80155D70
void fn_80154974(void);                                         // 0x80154974
void fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);     // 0x801566E8: orthographic projection
void fn_801542DC(void);                                         // 0x801542DC
void fn_8014DE68(void);                                         // 0x8014DE68
s32  fn_8015BA30(void* p);                                      // 0x8015BA30
s32  fn_8016B898(f32* origin, f32* dir, f32* boxMin, f32* boxMax);   // 0x8016B898 (ray vs box)

void Vec3Copy(Vec3* out, const Vec3* v);                        // 0x80052640
void Vec3Assign(Vec3* out, const Vec3* v);                      // 0x80053354
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);          // 0x80053370
void Vec3Dot(Scalar* out, const Vec3* a, const Vec3* b);        // 0x800549C0
void Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s);  // 0x80053224
void ScalarMul(Scalar* out, const Scalar* a, const Scalar* b);  // 0x80058214
void ScalarZero(Scalar* out);                                   // 0x80055E2C
void ScalarSet(Scalar* out, f32 v);                             // 0x80053250
s32  ScalarLess(const Scalar* a, const Scalar* b);              // 0x800533B0
s32  fn_80058BE8(const f32* a, const f32* b);                   // 0x80058BE8 (a > b)
void fn_80052638(void);                                      // 0x80052638 (empty leaf)
u8*  fn_800526D4(u8* mtx);                                      // 0x800526D4 (matrix translation)
void fn_8005C158(Vec3* out, const Vec3* v);                     // 0x8005C158
void fn_8005A244(Vec3* out, const Vec3* a, const Vec3* b);      // 0x8005A244 (normalised cross product)
void fn_800636A0(void);                                         // 0x800636A0 (no-op scalar ctor)
f32  fn_80082BE0(u8* out, u8* v);                               // 0x80082BE0 (plane {v, constant} from a vector)
f32  fn_80082F20(u8* out, u8* v);                               // 0x80082F20 (copies 3 floats)
void fn_80082F68(f32* out, f32* in);                            // 0x80082F68 (negated 4-float vector)
f32* fn_80148D70(f32* dst, const f32* src);                     // 0x80148D70 (copies 4 floats, returns dst)
f32  fn_8015EC78(u8* out, u8* in);                              // 0x8015EC78 (copies 4 floats)
s32  fn_8016F648(u8* view, f32* sphere);                        // 0x8016F648 (sphere vs the six planes: 1 outside, 2 straddling)
void fn_8016EB34(u8* out, u8* v);                               // 0x8016EB34 (*out = v[0])
void fn_8016EB40(u8* out, u8* v);                               // 0x8016EB40 (*out = v[1])
void fn_8016EB60(u8* v, u8* w);                                 // 0x8016EB60 (v[3] = *w)
void fn_8016EB6C(const f32* m, f32* out, const f32* in, s32 count);   // 0x8016EB6C
void fn_8016F6F0(f32* out, f32* v);                             // 0x8016F6F0 (out = v[3])
f32  fn_8016F718(u8* out, u8* v);                               // 0x8016F718 (dot product with itself)
s32  fn_8016F6FC(const Scalar* a, const Scalar* b);             // 0x8016F6FC (a <= b)
s32  fn_8016F740(f32* a, f32* b);                               // 0x8016F740 (a >= b)
void fn_8016F778(u8* out, u8* v);                               // 0x8016F778 (*out = v[2])
f32  fn_8016F75C(u8* out, u8* x, u8* y, u8* z);                 // 0x8016F75C (out = {*x, *y, *z})
f32  fn_8016F784(u8* out, u8* a, u8* b);                        // 0x8016F784 (*out = *a / *b)
void fn_80054840(u8* out, u8* in);                              // 0x80054840 (copy float)
void fn_8016F798(Vec3* out, const Vec3* a, const Vec3* b);      // 0x8016F798 (component-wise min)
void fn_8016F7F0(Vec3* out, const Vec3* a, const Vec3* b);      // 0x8016F7F0 (component-wise max)
void fn_8016F848(Vec3* out, const Scalar* s);                   // 0x8016F848 (out.y = *s)

extern u8  lbl_8040D500[];   // 0x8040D500
extern u8  lbl_8040E5F0[];   // 0x8040E5F0
extern u8  lbl_805F9212;     // 0x805F9212 (copy filter value)
extern s32 lbl_805FB4BC;     // 0x805FB4BC (current display list)
extern s32 lbl_805FB4C8;     // 0x805FB4C8 (render budget)
extern u8  lbl_805FB50C;     // 0x805FB50C (GX shadow flags byte)
extern s32 lbl_805FB514;     // 0x805FB514 (GX shadow word)
}

static inline void Bp(u32 v) { GXWGFifo._u8 = 0x61; GXWGFifo._u32 = v; }
static inline void XfHead(u16 countMinus1, u16 reg) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = countMinus1;
    GXWGFifo._u16 = reg;
}

// Colour channel f * 255 converted like fctiwz and clamped to 0..255.
static inline s32 ClampChannel(f32 f) {
    s32 v = (s32)(255.0f * f);
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

extern "C" {

// 0x801620BC
// Effects overlay pass on the effect manager: clears the four state words at +0x3604, updates the
// 0x40 effect records (stride 0xD8, state at +0xD0, kind at +0xC4), sets up an orthographic 1x1 view
// and a few GX registers, then draws the kind 0x10..0x12 records. Raw GX writes are kept exactly.
void fn_801620BC(u8* mgr) {
    s32 hadFlag = *(s32*)(mgr + 0x3604);
    *(s32*)(mgr + 0x3608) = 0;
    *(s32*)(mgr + 0x3604) = 0;
    *(s32*)(mgr + 0x360C) = 0;
    *(s32*)(mgr + 0x3610) = 0;
    fn_8015ECA8(lbl_8040D500);

    u8* rec = mgr;
    for (s32 i = 0; i < 0x40; i++, rec += 0xD8) {
        s32 state = *(s32*)(rec + 0xD0);
        if (state != 0 && state != 4) {
            fn_80161398(*(u32*)(rec + 0xC4), rec);
        }
    }
    if (*(s32*)(mgr + 0x40BC) == 0) {
        fn_80161810(mgr);
    }
    if (hadFlag != 0 && *(s32*)(mgr + 0x3604) == 0) {
        fn_80156AC0(1);
        fn_80156CD8();
    }
    for (s32 i = 0; i < 3; i++) {
        if (*(s32*)(mgr + 0x3608 + i * 4) != 0) {
            fn_80156F40(i);
        } else {
            fn_80156F58(i);
        }
    }
    fn_8015F4CC(lbl_8040D500);

    rec = mgr;
    for (s32 i = 0; i < 0x40; i++, rec += 0xD8) {
        s32 state = *(s32*)(rec + 0xD0);
        if (state != 0 && state != 4) {
            if (*(s32*)(rec + 0xC4) == 0xF) {
                fn_80166FC4(lbl_8040E5F0, rec);
            }
        }
    }
    fn_80150014();
    fn_80155D70();
    fn_80154974();
    fn_801566E8(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);

    // XF matrix registers 0x500..: 12 words (3x4 with 1.0 on the diagonal), then BP state writes.
    XfHead(0xB, 0x500);
    GXWGFifo._u32 = 0x3F800000;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0x3F800000;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0x3F800000;
    GXWGFifo._u32 = 0;
    Bp(0xFE000090);
    Bp(0x80000090);
    Bp(0xFE00000F);
    Bp(0x80000005);

    u8 flags = lbl_805FB50C;
    if (flags & 4) {
        lbl_805FB50C = (u8)(flags & ~4);
        s32 w = lbl_805FB514 | 0x40;
        lbl_805FB514 = w;
        Bp((u32)w | 0x43000000);
        Bp(0xF33F0000);
    }
    fn_801542DC();

    flags = lbl_805FB50C;
    if (flags & 2) {
        lbl_805FB50C = (u8)(flags & ~2);
        if (flags & 1) {
            Bp(0xFE00000F);
            Bp(0);
            XfHead(0, 0x103F);
            GXWGFifo._u32 = 0;
            Bp(0xFE003C00);
            Bp(0);
            XfHead(0, 0x1010);
            GXWGFifo._u32 = 0x441;
            Bp(0x28380000);
            Bp(0xC008F2A4);
            Bp(0xC108E6F0);
        } else {
            Bp(0xFE00000F);
            Bp(0);
            XfHead(0, 0x103F);
            GXWGFifo._u32 = 0;
            Bp(0xFE003C00);
            Bp(0);
            XfHead(0, 0x100E);
            GXWGFifo._u32 = 0x442;
            XfHead(0, 0x1010);
            GXWGFifo._u32 = 0x442;
            Bp(0x28380000);
            Bp(0xC008FFFA);
            Bp(0xC108FFD0);
        }
    }

    f32 col[4];
    col[0] = 1.0f;
    col[1] = 1.0f;
    col[2] = 1.0f;
    col[3] = 1.0f;
    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x1107 > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }

    flags = lbl_805FB50C;
    if (flags & 1) {
        lbl_805FB50C = (u8)(flags & 0xFE);
        if (flags & 2) {
            Bp(0xFE00000F);
            Bp(1);
            XfHead(0, 0x103F);
            GXWGFifo._u32 = 1;
            Bp(0xFE003C00);
            Bp(0);
            XfHead(0, 0x100E);
            GXWGFifo._u32 = 0x442;
            XfHead(0, 0x1010);
            GXWGFifo._u32 = 0x442;
            Bp(0x28380040);
            Bp(0xC008F8AF);
            Bp(0xC108F2F0);
        } else {
            Bp(0xFE00000F);
            Bp(0);
            XfHead(0, 0x103F);
            GXWGFifo._u32 = 0;
            Bp(0xFE003C00);
            Bp(0);
            XfHead(0, 0x100E);
            GXWGFifo._u32 = 0x442;
            XfHead(0, 0x1010);
            GXWGFifo._u32 = 0x442;
            Bp(0x28380000);
            Bp(0xC008FFFA);
            Bp(0xC108FFD0);
        }
    }

    s32 cr = ClampChannel(col[0]);
    s32 cg = ClampChannel(col[1]);
    s32 cb = ClampChannel(col[2]);
    s32 ca = ClampChannel(col[3]);
    // XF register 0x100C: the four clamped channels packed r,g,b,a.
    XfHead(0, 0x100C);
    GXWGFifo._u32 = (u32)(ca | (cb << 8) | ((cr << 24) | (cg << 16)));
    Bp(0xFE000FE0);
    Bp(0x410004A0);
    Bp(0xFE000001);
    Bp(0x41000001);
    lbl_805FB50C = (u8)(lbl_805FB50C | 8);
    Bp(0xFE00C000);
    Bp((u32)lbl_805F9212 << 14);

    rec = mgr;
    for (s32 i = 0; i < 0x40; i++, rec += 0xD8) {
        s32 state = *(s32*)(rec + 0xD0);
        if (state != 0 && state != 4) {
            switch (*(s32*)(rec + 0xC4)) {
            case 0x10: fn_801649B4(lbl_8040E5F0, rec); break;
            case 0x11: fn_80163C60(lbl_8040E5F0, rec); break;
            case 0x12: fn_80165EF0(lbl_8040E5F0, rec); break;
            default: break;
            }
        }
    }
    if (*(s32*)(mgr + 0x3604) != 0) {
        fn_80161BAC(mgr);
    }
}

}  // extern "C"

// ---- Ray (segment) vs triangle-tree query shared by fn_8016BB04 / fn_8016C2AC ----
//
// shape: +0x08 node count, +0x18 vertex array (12-byte vec3), +0x1C u32 triangle-info table,
// +0x20 node array. A node holds its box (min xyz at +0, max xyz at +0x10), a word at +0xC (top bit
// set: leaf, the low 31 bits are the byte offset of the triangle list in the node array; clear:
// inner, +0xC is the offset of the first child) and a word at +0x1C (leaf: triangle count; inner:
// offset of the second child, pushed on a stack). Triangles are 8 bytes: three u16 vertex indices
// and a u16 index into the info table. The query callback gets the hit record (segment start/end,
// t at +0x18, info pointer at +0x1C, normal pointer at +0x20, vertex pointers at +0x24..+0x2C)
// and the caller's query record; returning 0 stops the search.
typedef s32 (*SegQueryCb)(u8* hit, u8* query);

// Slab test of the segment (origin + t*dir, t in 0..1) against a node box; dir components all
// non-zero (inv = 1/dir).
static s32 SegHitsBox(const f32* o, const f32* inv, const u8* node) {
    f32 b0x = (*(f32*)(node + 0x0) - o[0]) * inv[0];
    f32 b1x = (*(f32*)(node + 0x10) - o[0]) * inv[0];
    f32 b0y = (*(f32*)(node + 0x4) - o[1]) * inv[1];
    f32 b1y = (*(f32*)(node + 0x14) - o[1]) * inv[1];
    f32 b0z = (*(f32*)(node + 0x8) - o[2]) * inv[2];
    f32 b1z = (*(f32*)(node + 0x18) - o[2]) * inv[2];
    f32 minx = (b0x >= b1x) ? b1x : b0x;
    f32 miny = (b0y >= b1y) ? b1y : b0y;
    f32 minz = (b0z >= b1z) ? b1z : b0z;
    f32 maxx = (b0x <= b1x) ? b1x : b0x;
    f32 maxy = (b0y <= b1y) ? b1y : b0y;
    f32 maxz = (b0z <= b1z) ? b1z : b0z;
    if (minx <= 1.0f && miny <= 1.0f && minz <= 1.0f) {
        if (maxx >= 0.0f && maxy >= 0.0f && maxz >= 0.0f) return 1;
    }
    return 0;
}

// Normalises (x, y, z) into out; the zero vector stays zero. (The original uses frsqrte plus two
// Newton steps.)
static void NormalizeInto(f32* out, f32 x, f32 y, f32 z) {
    f32 len2 = z * z + (x * x + y * y);
    if (len2 == 0.0f) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    } else {
        f32 r = (f32)(1.0 / sqrt((f64)len2));
        out[0] = x * r;
        out[1] = y * r;
        out[2] = z * r;
    }
}

// Fills the hit record for triangle tri (t = hit parameter, n = unit normal) and calls the query
// callback. Returns the callback result.
static s32 ReportHit(u8* shape, const u8* tri, f32* seg, f32 t, f32* n, SegQueryCb cb, u8* query) {
    u8 hit[0x40];
    f32* h = (f32*)hit;
    h[0] = seg[0];
    h[1] = seg[1];
    h[2] = seg[2];
    h[3] = seg[3];
    h[4] = seg[4];
    h[5] = seg[5];
    h[6] = t;
    u8* verts = *(u8**)(shape + 0x18);
    u8* info = *(u8**)(shape + 0x1C);
    // The original stores 32-bit pointers at these offsets; they overlap on x64.
    *(u8**)(hit + 0x1C) = info + *(u16*)(tri + 6) * 4;
    *(f32**)(hit + 0x20) = n;
    *(u8**)(hit + 0x24) = verts + *(u16*)(tri + 0) * 0xC;
    *(u8**)(hit + 0x28) = verts + *(u16*)(tri + 2) * 0xC;
    *(u8**)(hit + 0x2C) = verts + *(u16*)(tri + 4) * 0xC;
    return cb(hit, query);
}

extern "C" {

// 0x8016BB04
// Segment a->b against the triangle tree (back-face culled, barycentric tolerance 0): walks the box
// tree depth first; every triangle the segment crosses (front side only, det > 1e-8) is reported
// to cb(hit, q). Stops when cb returns 0.
void fn_8016BB04(u8* shape, f32* a, f32* b, void* cbp, u8* q) {
    SegQueryCb cb = (SegQueryCb)cbp;
    if (*(s32*)(shape + 0x8) <= 0) return;

    f32 o[3] = { a[0], a[1], a[2] };
    f32 d[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    f32 seg[6] = { a[0], a[1], a[2], b[0], b[1], b[2] };
    f32 inv[3];
    s32 hasDir = 0;
    if (d[0] != 0.0f && d[1] != 0.0f && d[2] != 0.0f) hasDir = 1;
    if (hasDir) {
        inv[0] = 1.0f / d[0];
        inv[1] = 1.0f / d[1];
        inv[2] = 1.0f / d[2];
    } else {
        inv[0] = 0.0f;
        inv[1] = 0.0f;
        inv[2] = 0.0f;
    }

    s32 stack[64];
    s32* top = stack;
    s32 off = 0;
    for (;;) {
        u8* nodes = *(u8**)(shape + 0x20);
        u8* node = nodes + off;
        f32 nmin[3] = { *(f32*)(node + 0x0), *(f32*)(node + 0x4), *(f32*)(node + 0x8) };
        f32 nmax[3] = { *(f32*)(node + 0x10), *(f32*)(node + 0x14), *(f32*)(node + 0x18) };
        s32 hit;
        if (!hasDir) {
            hit = fn_8016B898(o, d, nmin, nmax);
        } else {
            hit = SegHitsBox(o, inv, node);
        }
        if (hit != 0) {
            u32 w = *(u32*)(node + 0xC);
            if (!(w & 0x80000000u)) {
                *top++ = *(s32*)(node + 0x1C);
                off = (s32)w;
                continue;
            }
            s32 count = *(s32*)(node + 0x1C);
            u8* tri = nodes + (w & 0x7FFFFFFF);
            do {
                u8* verts = *(u8**)(shape + 0x18);
                f32* p0 = (f32*)(verts + *(u16*)(tri + 0) * 0xC);
                f32* p1 = (f32*)(verts + *(u16*)(tri + 4) * 0xC);
                f32* p2 = (f32*)(verts + *(u16*)(tri + 2) * 0xC);
                // e1 = p1 - p0, e2 = p2 - p0, qv = d x e1, det = e2 . qv
                f32 e1x = p1[0] - p0[0], e1y = p1[1] - p0[1], e1z = p1[2] - p0[2];
                f32 e2x = p2[0] - p0[0], e2y = p2[1] - p0[1], e2z = p2[2] - p0[2];
                f32 qx = d[1] * e1z - d[2] * e1y;
                f32 qy = d[2] * e1x - d[0] * e1z;
                f32 qz = d[0] * e1y - d[1] * e1x;
                f32 det = e2z * qz + (e2x * qx + e2y * qy);
                if (det > 1e-8f) {
                    f32 tol = det * 1e-5f;
                    f32 sx = o[0] - p0[0], sy = o[1] - p0[1], sz = o[2] - p0[2];
                    f32 u = sz * qz + (sx * qx + sy * qy);
                    if (u >= -tol && u <= det + tol) {
                        f32 rx = sy * e2z - sz * e2y;
                        f32 ry = sz * e2x - sx * e2z;
                        f32 rz = sx * e2y - sy * e2x;
                        f32 v = d[2] * rz + (d[0] * rx + d[1] * ry);
                        if (v >= -tol && u + v <= det + tol) {
                            f32 t = (e1z * rz + (e1x * rx + e1y * ry)) * (1.0f / det);
                            if (t >= 0.0f && t <= 1.0f) {
                                f32 nx = e2y * e1z - e2z * e1y;
                                f32 ny = e2z * e1x - e2x * e1z;
                                f32 nz = e2x * e1y - e2y * e1x;
                                f32 n[3];
                                NormalizeInto(n, nx, ny, nz);
                                if (ReportHit(shape, tri, seg, t, n, cb, q) == 0) return;
                            }
                        }
                    }
                }
                tri += 8;
            } while (--count > 0);
        }
        if (top <= stack) return;
        off = *--top;
    }
}

// 0x8016C2AC
// Same query as fn_8016BB04 but two-sided: uses 1/det so back faces are reported too (det only has
// to be non-zero) and a barycentric tolerance of 1e-5.
void fn_8016C2AC(u8* shape, f32* a, f32* b, void* cbp, u8* q) {
    SegQueryCb cb = (SegQueryCb)cbp;
    if (*(s32*)(shape + 0x8) <= 0) return;

    f32 o[3] = { a[0], a[1], a[2] };
    f32 d[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    f32 seg[6] = { a[0], a[1], a[2], b[0], b[1], b[2] };
    f32 inv[3];
    s32 hasDir = 0;
    if (d[0] != 0.0f && d[1] != 0.0f && d[2] != 0.0f) hasDir = 1;
    if (hasDir) {
        inv[0] = 1.0f / d[0];
        inv[1] = 1.0f / d[1];
        inv[2] = 1.0f / d[2];
    } else {
        inv[0] = 0.0f;
        inv[1] = 0.0f;
        inv[2] = 0.0f;
    }

    s32 stack[64];
    s32* top = stack;
    s32 off = 0;
    for (;;) {
        u8* nodes = *(u8**)(shape + 0x20);
        u8* node = nodes + off;
        f32 nmin[3] = { *(f32*)(node + 0x0), *(f32*)(node + 0x4), *(f32*)(node + 0x8) };
        f32 nmax[3] = { *(f32*)(node + 0x10), *(f32*)(node + 0x14), *(f32*)(node + 0x18) };
        s32 hit;
        if (!hasDir) {
            hit = fn_8016B898(o, d, nmin, nmax);
        } else {
            hit = SegHitsBox(o, inv, node);
        }
        if (hit != 0) {
            u32 w = *(u32*)(node + 0xC);
            if (!(w & 0x80000000u)) {
                *top++ = *(s32*)(node + 0x1C);
                off = (s32)w;
                continue;
            }
            s32 count = *(s32*)(node + 0x1C);
            u8* tri = nodes + (w & 0x7FFFFFFF);
            do {
                u8* verts = *(u8**)(shape + 0x18);
                f32* p0 = (f32*)(verts + *(u16*)(tri + 0) * 0xC);
                f32* p1 = (f32*)(verts + *(u16*)(tri + 4) * 0xC);
                f32* p2 = (f32*)(verts + *(u16*)(tri + 2) * 0xC);
                f32 e1x = p1[0] - p0[0], e1y = p1[1] - p0[1], e1z = p1[2] - p0[2];
                f32 e2x = p2[0] - p0[0], e2y = p2[1] - p0[1], e2z = p2[2] - p0[2];
                f32 qx = d[1] * e1z - d[2] * e1y;
                f32 qy = d[2] * e1x - d[0] * e1z;
                f32 qz = d[0] * e1y - d[1] * e1x;
                f32 det = e2z * qz + (e2x * qx + e2y * qy);
                f32 invDet;
                if (det == 0.0f) {
                    invDet = 0.0f;
                } else {
                    invDet = 1.0f / det;
                }
                if (invDet != 0.0f) {
                    f32 sx = o[0] - p0[0], sy = o[1] - p0[1], sz = o[2] - p0[2];
                    f32 u = (sz * qz + (sx * qx + sy * qy)) * invDet;
                    if (u >= -0.00001f && u <= 1.00001f) {
                        f32 rx = sy * e2z - sz * e2y;
                        f32 ry = sz * e2x - sx * e2z;
                        f32 rz = sx * e2y - sy * e2x;
                        f32 v = (d[2] * rz + (d[0] * rx + d[1] * ry)) * invDet;
                        if (v >= -0.00001f && u + v <= 1.00001f) {
                            f32 t = (e1z * rz + (e1x * rx + e1y * ry)) * invDet;
                            if (t >= 0.0f && t <= 1.0f) {
                                f32 nx = e2y * e1z - e2z * e1y;
                                f32 ny = e2z * e1x - e2x * e1z;
                                f32 nz = e2x * e1y - e2y * e1x;
                                f32 n[3];
                                NormalizeInto(n, nx, ny, nz);
                                if (ReportHit(shape, tri, seg, t, n, cb, q) == 0) return;
                            }
                        }
                    }
                }
                tri += 8;
            } while (--count > 0);
        }
        if (top <= stack) return;
        off = *--top;
    }
}

}  // extern "C"

// ---- View-volume test (fn_8016EC2C) ----

// Plane through `point` with the normal already stored at `plane`.
static inline void PlaneThrough(u8* plane, const Vec3* point) {
    Vec3 n;
    Scalar d;
    fn_80082F20((u8*)&n, plane);
    Vec3Dot(&d, point, &n);
    fn_8016EB60(plane, (u8*)&d);
}

// Side plane: normal = normalised a x b, through `point`.
static inline void SidePlane(u8* plane, const Vec3* a, const Vec3* b, const Vec3* point) {
    Vec3 n;
    f32 p[4];
    fn_8005A244(&n, a, b);
    fn_80082BE0((u8*)p, (u8*)&n);
    fn_8015EC78(plane, (u8*)p);
    PlaneThrough(plane, point);
}

// dst = a - b (through a temporary, as the original does).
static inline void SubInto(Vec3* dst, const Vec3* a, const Vec3* b) {
    Vec3 t;
    Vec3Sub(&t, a, b);
    Vec3Copy(dst, &t);
}

// One box corner: out = {xs.x, ys.y, zs.z}, picked through the component accessors.
static inline void BoxCorner(Vec3* out, const Vec3* xs, const Vec3* ys, const Vec3* zs) {
    Scalar x, y, z;
    Vec3 t;
    fn_8016F778((u8*)&z, (u8*)zs);
    fn_8016EB40((u8*)&y, (u8*)ys);
    fn_8016EB34((u8*)&x, (u8*)xs);
    fn_8016F75C((u8*)&t, (u8*)&x, (u8*)&y, (u8*)&z);
    Vec3Copy(out, &t);
}

// out = v scaled so that its z becomes `negDist` (projection onto the plane z = negDist).
static inline void ProjectToZ(Vec3* out, const Vec3* v, const Scalar* negDist) {
    Scalar z, ratio, s;
    fn_8016F778((u8*)&z, (u8*)v);
    fn_8016F784((u8*)&ratio, (u8*)negDist, (u8*)&z);
    fn_80054840((u8*)&s, (u8*)&ratio);
    Vec3MulScalar(out, v, &s);
}

extern "C" {

// 0x8016EC2C
// View-volume test of a sphere (sphere = centre xyz + radius) against the perspective view at
// `view`, with the box `box` (min xyz, max xyz) and the direction `dir`. Returns 1 when the sphere
// is outside, 2 when the camera is inside the sphere or on the front side of the plane (point
// `box`, normal `dir`), and otherwise (box corners taken into view space, projected onto the near
// plane and clipped against the view's own near rectangle; no overlap returns 1) rewrites the
// view's frustum corners (+0x120..+0x174) and its six planes (+0xC0..+0x110) around the clipped
// region and returns 2. A non-perspective view (+0x188 == 0) returns 1 at once.
s32 fn_8016EC2C(u8* view, u8* sphere, u8* dir, u8* box) {
    if (*(s32*)(view + 0x188) == 0) return 1;

    f32 sph[4];
    if (fn_8016F648(view, fn_80148D70(sph, (const f32*)sphere)) == 1) return 1;

    Vec3 e1, e2;
    Vec3 pos;
    fn_80052638();
    fn_80052638();
    Vec3Assign(&pos, (const Vec3*)fn_800526D4(view));

    // Camera inside the sphere: |pos - centre|^2 <= r^2.
    Vec3 centre, off;
    fn_80082F20((u8*)&centre, sphere);
    Vec3Sub(&off, &pos, &centre);
    Vec3Copy(&e1, &off);
    Scalar r1, r2, r2sq, lenSq;
    fn_8016F6F0(&r1, (f32*)sphere);
    fn_8016F6F0(&r2, (f32*)sphere);
    ScalarMul(&r2sq, &r2, &r1);
    fn_8016F718((u8*)&lenSq, (u8*)&e1);
    if (fn_8016F6FC(&lenSq, &r2sq) != 0) return 2;

    // Camera on the front side of the plane through `box` with normal `dir`.
    Vec3 toBox;
    Vec3Sub(&toBox, &pos, (const Vec3*)box);
    Vec3Copy(&e1, &toBox);
    Scalar zero, dotv;
    ScalarZero(&zero);
    Vec3Dot(&dotv, &e1, (const Vec3*)dir);
    if (fn_8016F740(&dotv, &zero) != 0) return 2;

    // The eight corners of the box, taken into view space and projected onto the near plane.
    fn_800636A0();
    Vec3 bmin, bmax;
    Vec3Assign(&bmin, (const Vec3*)box);
    Vec3Assign(&bmax, (const Vec3*)(box + 0xC));
    Vec3 corners[8];
    for (s32 i = 0; i < 8; i++) fn_80052638();
    BoxCorner(&corners[0], &bmax, &bmax, &bmin);   // (max.x, max.y, min.z)
    BoxCorner(&corners[1], &bmin, &bmax, &bmin);   // (min.x, max.y, min.z)
    Vec3Copy(&corners[2], &bmin);                  // (min.x, min.y, min.z)
    BoxCorner(&corners[3], &bmax, &bmin, &bmin);   // (max.x, min.y, min.z)
    Vec3Copy(&corners[4], &bmax);                  // (max.x, max.y, max.z)
    BoxCorner(&corners[5], &bmin, &bmax, &bmax);   // (min.x, max.y, max.z)
    BoxCorner(&corners[6], &bmin, &bmin, &bmax);   // (min.x, min.y, max.z)
    BoxCorner(&corners[7], &bmax, &bmin, &bmax);   // (max.x, min.y, max.z)
    fn_8016EB6C((const f32*)(view + 0x90), (f32*)corners, (const f32*)corners, 8);

    Scalar negNear;
    ScalarSet(&negNear, -*(f32*)(view + 0x194));
    for (s32 i = 0; i < 8; i++) {
        Vec3 c, t;
        fn_8005C158(&c, &corners[i]);
        ProjectToZ(&t, &c, &negNear);
        Vec3Copy(&corners[i], &t);
    }
    Vec3 lo, hi;                       // bounds of the projected corners
    Vec3Assign(&lo, &corners[0]);
    Vec3Assign(&hi, &corners[0]);
    for (s32 i = 1; i < 8; i++) {
        Vec3 t;
        fn_8016F798(&t, &lo, &corners[i]);
        Vec3Copy(&lo, &t);
        fn_8016F7F0(&t, &hi, &corners[i]);
        Vec3Copy(&hi, &t);
    }

    // Bounds of the view's own four near corners, in world space.
    Vec3 nearPts[4];
    for (s32 i = 0; i < 4; i++) fn_80052638();
    fn_8016EB6C((const f32*)(view + 0x90), (f32*)nearPts, (const f32*)(view + 0x120), 4);
    Vec3 nlo, nhi;
    Vec3Assign(&nlo, &nearPts[0]);
    Vec3Assign(&nhi, &nearPts[0]);
    for (s32 i = 1; i < 4; i++) {
        Vec3 t;
        fn_8016F798(&t, &nlo, &nearPts[i]);
        Vec3Copy(&nlo, &t);
        fn_8016F7F0(&t, &nhi, &nearPts[i]);
        Vec3Copy(&nhi, &t);
    }

    // The two rectangles must overlap in x and in y.
    {
        Scalar a, b;
        fn_8016EB34((u8*)&a, (u8*)&nlo);
        fn_8016EB34((u8*)&b, (u8*)&hi);
        if (ScalarLess(&b, &a) != 0) return 1;
        fn_8016EB34((u8*)&a, (u8*)&nhi);
        fn_8016EB34((u8*)&b, (u8*)&lo);
        if (fn_80058BE8(&b, &a) != 0) return 1;
        fn_8016EB40((u8*)&a, (u8*)&nlo);
        fn_8016EB40((u8*)&b, (u8*)&hi);
        if (ScalarLess(&b, &a) != 0) return 1;
        fn_8016EB40((u8*)&a, (u8*)&nhi);
        fn_8016EB40((u8*)&b, (u8*)&lo);
        if (fn_80058BE8(&b, &a) != 0) return 1;
    }

    // Intersection of the two rectangles, then pushed out to the far plane as well.
    Vec3 t;
    fn_8016F7F0(&t, &lo, &nlo);
    Vec3Copy(&lo, &t);
    fn_8016F798(&t, &hi, &nhi);
    Vec3Copy(&hi, &t);
    Vec3 nearLo, nearHi;
    fn_8005C158(&nearLo, &lo);
    fn_8005C158(&nearHi, &hi);
    Scalar negFar;
    ScalarSet(&negFar, -*(f32*)(view + 0x198));
    Vec3 farLo, farHi;
    ProjectToZ(&farLo, &nearLo, &negFar);
    ProjectToZ(&farHi, &nearHi, &negFar);

    // New frustum corners (view space, then moved by the view matrix): near four, far four.
    Scalar y;
    Vec3Copy((Vec3*)(view + 0x120), &hi);
    Vec3Copy((Vec3*)(view + 0x12C), &lo);
    fn_8016EB40((u8*)&y, (u8*)&hi);
    fn_8016F848((Vec3*)(view + 0x12C), &y);
    Vec3Copy((Vec3*)(view + 0x138), &lo);
    Vec3Copy((Vec3*)(view + 0x144), &hi);
    fn_8016EB40((u8*)&y, (u8*)&lo);
    fn_8016F848((Vec3*)(view + 0x144), &y);
    Vec3Copy((Vec3*)(view + 0x150), &farHi);
    Vec3Copy((Vec3*)(view + 0x15C), &farLo);
    fn_8016EB40((u8*)&y, (u8*)&farHi);
    fn_8016F848((Vec3*)(view + 0x15C), &y);
    Vec3Copy((Vec3*)(view + 0x168), &farLo);
    Vec3Copy((Vec3*)(view + 0x174), &farHi);
    fn_8016EB40((u8*)&y, (u8*)&farLo);
    fn_8016F848((Vec3*)(view + 0x174), &y);
    fn_8016EB6C((const f32*)view, (f32*)(view + 0x120), (const f32*)(view + 0x120), 8);

    // The six planes, as in the perspective branch of fn_8016E304.
    f32 p[4];
    SubInto(&e1, (Vec3*)(view + 0x174), (Vec3*)(view + 0x150));
    SubInto(&e2, (Vec3*)(view + 0x15C), (Vec3*)(view + 0x150));
    SidePlane(view + 0xC0, &e1, &e2, (Vec3*)(view + 0x150));
    fn_80082F68(p, (f32*)(view + 0xC0));
    fn_8015EC78(view + 0xD0, (u8*)p);
    PlaneThrough(view + 0xD0, (Vec3*)(view + 0x120));
    SubInto(&e1, (Vec3*)(view + 0x168), (Vec3*)(view + 0x15C));
    SubInto(&e2, (Vec3*)(view + 0x12C), (Vec3*)(view + 0x15C));
    SidePlane(view + 0xE0, &e1, &e2, (Vec3*)(view + 0x12C));
    SubInto(&e1, (Vec3*)(view + 0x150), (Vec3*)(view + 0x15C));
    SidePlane(view + 0xF0, &e2, &e1, (Vec3*)(view + 0x12C));
    SubInto(&e1, (Vec3*)(view + 0x150), (Vec3*)(view + 0x174));
    SubInto(&e2, (Vec3*)(view + 0x144), (Vec3*)(view + 0x174));
    SidePlane(view + 0x100, &e1, &e2, (Vec3*)(view + 0x144));
    SubInto(&e1, (Vec3*)(view + 0x168), (Vec3*)(view + 0x174));
    SidePlane(view + 0x110, &e2, &e1, (Vec3*)(view + 0x144));
    return 2;
}

}  // extern "C"
