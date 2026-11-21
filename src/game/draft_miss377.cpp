// Skinned-matrix palette builders and GX sprite-batch drawing, plus the animation-track
// start routine (0x80185F1C..0x8018B78C).
#include "types.h"
#include "revolution.h"
#include <math.h>

extern "C" {
void fn_8014E3EC(s32 n);                                           // 0x8014E3EC
void fn_80151E38(s32 quads, void* verts);                          // 0x80151E38
void fn_8016A2B8(u8* p);                                           // 0x8016A2B8
void fn_8016A3F8(u8* p);                                           // 0x8016A3F8
void fn_8016A43C(u8* p);                                           // 0x8016A43C
void fn_8016A570(u8* p);                                           // 0x8016A570
void fn_8016A788(u8* p);                                           // 0x8016A788
void fn_8016B024(u8* track, s32 zero, u8* table, s32 idx, s32 id); // 0x8016B024
void fn_8016B478(u8* player, u8* track);                           // 0x8016B478
u8*  fn_8016B50C(void* a, s32 b);                                  // 0x8016B50C
u8*  fn_80174788(u8* res, s32 idx);                                // 0x80174788
void fn_80185C98(f32* out, u8* src, s32 n, s32 idx);               // 0x80185C98
void fn_80186E1C(f32* out, u8* src, s32 n, s32 idx);               // 0x80186E1C
void fn_8018A948(u8* obj, f32 t);                                  // 0x8018A948
f32  fn_8018AF28(u8* obj, f32* a, f32* b);                         // 0x8018AF28

extern u8  lbl_80348950[];      // 0x80348950 (0x20-byte records, u32 at +0)
extern u8  lbl_80348A10[];      // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32 lbl_80348AD0;        // 0x80348AD0
extern u8  lbl_80400FF0[];      // 0x80400FF0 (the renderer's matrix block)
extern f32 lbl_804010D8[12];    // 0x804010D8 (3x4 matrix)
extern s32 lbl_8040ADF0[8];     // 0x8040ADF0
extern s32 lbl_8040AE10[8];     // 0x8040AE10
extern u8  lbl_804129E8[];      // 0x804129E8 (draw statistics, +0x30 = vertex count)
extern s32 lbl_805F920C;        // 0x805F920C (matrix build flags)
extern s32 lbl_805FB4B0;        // 0x805FB4B0
extern s32 lbl_805FB4C4;        // 0x805FB4C4 (display list write offset)
extern s32 lbl_805FB4CC;        // 0x805FB4CC (display list bytes left)
extern u8* lbl_805FB4D0;        // 0x805FB4D0 (base of the open display list)
extern u8  lbl_805FB50C;        // 0x805FB50C (GX shadow flags byte)
extern u8* lbl_805FB608;        // 0x805FB608 (heap object: slot +4 = free, called as (p, heap))
extern u8* lbl_805FB648;        // 0x805FB648 (pointer to a flag byte)
}

static inline void Bp(u32 v) { GXWGFifo._u8 = 0x61; GXWGFifo._u32 = v; }
static inline void XfHead(u16 countMinus1, u16 reg) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = countMinus1;
    GXWGFifo._u16 = reg;
}

// Normalizes (x, y, z) into out[0..2]; a zero-length vector gives zeros (original: frsqrte plus
// two Newton steps).
static void Norm3(f32 x, f32 y, f32 z, f32* out) {
    f32 len2 = (z * z) + ((x * x) + (y * y));
    if (len2 == 0.0f) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        out[0] = x * inv;
        out[1] = y * inv;
        out[2] = z * inv;
    }
}

static inline u32 Bswap32(u32 v) {
    return (v >> 24) | ((v >> 8) & 0xFF00u) | ((v << 8) & 0xFF0000u) | (v << 24);
}

extern "C" {

// 0x80185F1C
// Builds a 3x4 transform from the flags in lbl_805F920C (bit 0: camera-facing basis from the
// renderer matrix, bit 1: +1 in z, bit 2: scale (3,3,4) and offset -3.14 in z), then writes it,
// concatenated with each of `n` source 3x3 bases (rows normalised), to `out` (stride `stride`
// floats).
void fn_80185F1C(f32* out, u8* src, s32 n, s32 stride) {
    f32 m[12];   // m[0..8]: basis (row-major), m[9..11]: translation
    s32 flags = lbl_805F920C;
    if (flags & 1) {
        const f32* c = (const f32*)lbl_80400FF0;
        const f32* u = lbl_804010D8;
        f32 px = u[9] + ((u[6] * c[2]) + ((u[0] * c[0]) + (u[3] * c[1])));
        f32 py = u[10] + ((u[7] * c[2]) + ((u[1] * c[0]) + (u[4] * c[1])));
        f32 pz = (f32)-(u[11] + ((u[8] * c[2]) + ((u[2] * c[0]) + (u[5] * c[1]))));
        f32 a[3];   // forward axis
        f32 b[3];   // side axis
        f32 d[3];   // up axis
        Norm3(px, py, pz, a);
        m[6] = a[0];
        m[7] = a[1];
        m[8] = a[2];
        Norm3((1.0f * a[2]) - (0.0f * a[1]), (0.0f * a[0]) - (0.0f * a[2]),
              (0.0f * a[1]) - (1.0f * a[0]), b);
        Norm3((a[1] * b[2]) - (a[2] * b[1]), (a[2] * b[0]) - (a[0] * b[2]),
              (a[0] * b[1]) - (a[1] * b[0]), d);
        m[3] = d[0];
        m[4] = d[1];
        m[5] = d[2];
        m[0] = (d[1] * a[2]) - (d[2] * a[1]);
        m[1] = (d[2] * a[0]) - (d[0] * a[2]);
        m[2] = (d[0] * a[1]) - (d[1] * a[0]);
        m[9] = 0.0f;
        m[10] = 0.0f;
        m[11] = 0.0f;
    } else {
        m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
        m[3] = 0.0f; m[4] = 1.0f; m[5] = 0.0f;
        m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;
        m[9] = 0.0f;
        m[10] = 0.0f;
        m[11] = 0.0f;
    }
    if (flags & 2) {
        m[9] += 0.0f;
        m[10] += 0.0f;
        m[11] += 1.0f;
    }
    if (flags & 4) {
        m[0] *= 3.0f; m[1] *= 3.0f; m[2] *= 4.0f;
        m[3] *= 3.0f; m[4] *= 3.0f; m[5] *= 4.0f;
        m[6] *= 3.0f; m[7] *= 3.0f; m[8] *= 4.0f;
        m[9] = (m[9] * 3.0f) + 0.0f;
        m[10] = (m[10] * 3.0f) + 0.0f;
        m[11] = (m[11] * 4.0f) + -3.14f;
    }
    f32 tx = m[9] + ((m[6] * 0.0f) + ((m[0] * 0.0f) + (m[3] * 0.0f)));
    f32 ty = m[10] + ((m[7] * 0.0f) + ((m[1] * 0.0f) + (m[4] * 0.0f)));
    f32 tz = m[11] + ((m[8] * 0.0f) + ((m[2] * 0.0f) + (m[5] * 0.0f)));
    for (s32 i = 0; i < n; i++) {
        const f32* s = (const f32*)src;
        // Columns of the source basis, normalised.
        f32 inv0 = 1.0f / sqrtf((s[8] * s[8]) + ((s[0] * s[0]) + (s[4] * s[4])));
        f32 inv1 = 1.0f / sqrtf((s[9] * s[9]) + ((s[1] * s[1]) + (s[5] * s[5])));
        f32 inv2 = 1.0f / sqrtf((s[10] * s[10]) + ((s[2] * s[2]) + (s[6] * s[6])));
        f32 n0x = s[0] * inv0, n0y = s[4] * inv0, n0z = s[8] * inv0;
        f32 n1x = s[1] * inv1, n1y = s[5] * inv1, n1z = s[9] * inv1;
        f32 n2x = s[2] * inv2, n2y = s[6] * inv2, n2z = s[10] * inv2;
        out[0] = (m[6] * n0z) + ((m[0] * n0x) + (m[3] * n0y));
        out[1] = (m[6] * n1z) + ((m[0] * n1x) + (m[3] * n1y));
        out[2] = (m[6] * n2z) + ((m[0] * n2x) + (m[3] * n2y));
        out[3] = tx;
        out[4] = (m[7] * n0z) + ((m[1] * n0x) + (m[4] * n0y));
        out[5] = (m[7] * n1z) + ((m[1] * n1x) + (m[4] * n1y));
        out[6] = (m[7] * n2z) + ((m[1] * n2x) + (m[4] * n2y));
        out[7] = ty;
        out[8] = (m[8] * n0z) + ((m[2] * n0x) + (m[5] * n0y));
        out[9] = (m[8] * n1z) + ((m[2] * n1x) + (m[5] * n1y));
        out[10] = (m[8] * n2z) + ((m[2] * n2x) + (m[5] * n2y));
        out[11] = tz;
        src += stride * 4;
        out += stride;
    }
}

// 0x801870AC
// Skins `n` vertices into the open display list: each vertex is a weighted sum of 12-float
// matrices from `mats` (table: s32 count, then {s32 byteOffset, f32 weight} pairs), then the
// matrix-specific post-pass chosen by `flags` (0x20, 0x40, 0x60) and the "load matrix memory"
// command are issued.
void fn_801870AC(s32 n, u8* table, u8* mats, s32 flags) {
    s32 stride = 0xC;
    if (flags & 0x20) {
        stride = 0x15;
    }
    if (flags & 0x40) {
        stride += 0xC;
    }
    s32 size = ((n * (stride * 4)) + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < size) {
        fn_8014E3EC(size);
    }
    s32 off = lbl_805FB4C4;
    lbl_805FB4C4 = off + size;
    u8* buf = lbl_805FB4D0 + off;
    lbl_805FB4CC -= size;
    f32* dst = (f32*)buf;
    for (s32 i = 0; i < n; i++) {
        f32 w = *(f32*)(table + 0x8);
        const f32* m = (const f32*)(mats + *(s32*)(table + 0x4));
        for (s32 k = 0; k < 12; k++) {
            dst[k] = w * m[k];
        }
        s32 count = *(s32*)table;
        table += 0xC;
        for (s32 j = 1; j < count; j++) {
            f32 w2 = *(f32*)(table + 0x4);
            const f32* m2 = (const f32*)(mats + *(s32*)table);
            table += 8;
            for (s32 k = 0; k < 12; k++) {
                dst[k] = dst[k] + (w2 * m2[k]);
            }
        }
        dst += stride;
    }
    if (flags == 0x20) {
        fn_80185C98((f32*)(buf + 0x30), buf, n, stride);
    } else if (flags == 0x40) {
        fn_80185F1C((f32*)(buf + 0x30), buf, n, stride);
    } else if (flags == 0x60) {
        fn_80186E1C((f32*)(buf + 0x30), buf, n, stride);
    }
    GXWGFifo._u8 = 8;
    GXWGFifo._u8 = 0xAC;
    GXWGFifo._u32 = (u32)(uintptr_t)buf & 0x3FFFFFFF;   // lint:ok
    GXWGFifo._u8 = 8;
    GXWGFifo._u8 = 0xBC;
    GXWGFifo._u32 = 4;
    *(s32*)(lbl_804129E8 + 0x30) += n;
}

// Closes the vertices batched so far (`count` vertices at `base`) and hands them to the GX.
static inline void FlushQuads(s32 count, u8* base) {
    s32 size = ((count * 0x18) + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < size) {
        fn_8014E3EC(size);
    }
    lbl_805FB4CC -= size;
    lbl_805FB4C4 += size;
    fn_80151E38(count >> 2, base);
}

typedef void (*GlyphDrawFn)(u8* info, s32 word, f32 x0, f32 x1, f32 y0, f32 y1, f32 u0, f32 u1,
                            f32 v0, f32 v1);

// 0x80187E90
// Draws the glyph/sprite list of entry `idx` of the atlas set in `arg0` (entry stride 0xC:
// +0 = texture info, +4 = first node of a list of s16 {x,y,w,h,u,v} rects). `arg1` holds the
// scale (+8/+0xC) and origin (+0x10/+0x14). With a callback each rect is handed to it with
// screen and texture coordinates; without one the texture state is set up and the rects are
// batched as quads (0x18-byte vertices) into the display list.
void fn_80187E90(void* arg0, void* arg1, void* arg2, s32 idx) {
    u8* obj = (u8*)arg0;
    u8* params = (u8*)arg1;
    GlyphDrawFn cb = (GlyphDrawFn)arg2;
    s32 ent = idx * 0xC;
    u8* info = *(u8**)(*(u8**)(obj + 0x4) + ent);
    f32 sx = *(f32*)(params + 0x8);
    f32 sy = *(f32*)(params + 0xC);
    f32 ox = *(f32*)(params + 0x10);
    f32 oy = *(f32*)(params + 0x14);
    f32 invW = 1.0f / (f32)*(u16*)(info + 0x0);
    f32 invH = 1.0f / (f32)*(u16*)(info + 0x2);
    u8* node;
    if (cb != NULL) {
        for (node = *(u8**)(*(u8**)(obj + 0x4) + ent + 0x4); node != NULL; node = *(u8**)node) {
            s16 x = *(s16*)(node + 0xC);
            s16 y = *(s16*)(node + 0xE);
            s16 w = *(s16*)(node + 0x8);
            s16 h = *(s16*)(node + 0xA);
            s16 u = *(s16*)(node + 0x10);
            s16 v = *(s16*)(node + 0x12);
            cb(info, *(s32*)(node + 0x4),
               ox + ((f32)x * sx), ox + ((f32)(x + w) * sx),
               oy + ((f32)y * sy), oy + ((f32)(y + h) * sy),
               0.0f + ((f32)u * invW), invW + ((f32)((u + w) - 1) * invW),
               0.0f + ((f32)v * invH), invH + ((f32)((v + h) - 1) * invH));
        }
        return;
    }
    Bp(0xFE000060);
    Bp(((u32)*(u8*)(info + 0x12) << 5) | 0x80000000u);
    if ((u32)lbl_8040ADF0[0] != *(u32*)(info + 0x18) || lbl_8040AE10[0] != 0) {
        lbl_805FB4B0 += 1;
        lbl_8040ADF0[0] = *(s32*)(info + 0x18);
        u32 cfg = lbl_80348AD0;
        lbl_8040AE10[0] = 0;
        u32 recIdx = (u32)*(u8*)(info + 0x13) << 5;
        u32 texAddr = *(u32*)(lbl_80348950 + recIdx);
        s32 lutAddr = *(s32*)(lbl_80348A10 + recIdx);
        Bp((((u32)*(u8*)(info + 0xF) - 1) << 12 & 0xF000u) | 0x84000000u);
        Bp((((u32)*(u16*)(info + 0x4) - 1) |
            (((u32)*(u16*)(info + 0x6) - 1) << 10) |
            ((u32)*(u8*)(info + 0x10) << 20) | 0x88000000u));
        Bp((texAddr >> 5) | 0x8C0D8000u);
        if (lutAddr >= 0) {
            Bp(((u32)lutAddr >> 5) | 0x900D8000u);
        } else {
            Bp(0x90000000);
        }
        Bp(((*(u32*)(info + 0x18) >> 5) & 0xFFFFFFu) | 0x94000000u);
        if (*(u8*)(info + 0xD) != 0) {
            u32 cfgBits = (cfg >> 9) & 0x3FFu;
            Bp(((cfgBits | 0x98000000u) & ~0x3FC00u) |
               (((u32)*(u8*)(info + 0x11) << 10) & 0x3FC00u));
            Bp(0xFEFFFF00);
            Bp(0x0F000000);
            Bp(((*(u32*)(info + 0x1C) >> 5) & 0xFFFFFFu) | 0x64000000u);
            Bp(((cfgBits | 0x65000000u) & ~0x1FC00u) |
               ((((s32)*(u8*)(info + 0xD) >> 1) << 10) & 0x1FC00));
            Bp(0xFEFFFF00);
            Bp(0x0F000000);
        }
    }
    Bp((u32)(*(u16*)(info + 0x4) - 1) | 0x30000000u);
    Bp((u32)(*(u16*)(info + 0x6) - 1) | 0x31000000u);
    if (!(lbl_805FB50C & 2)) {
        lbl_805FB50C |= 2;
        if (lbl_805FB50C & 1) {
            Bp(0xFE00000F);
            Bp(1);
            XfHead(0, 0x103F);
            GXWGFifo._u32 = 1;
            Bp(0xFE003C00);
            Bp(0x400);
            XfHead(0, 0x1010);
            GXWGFifo._u32 = 0x441;
            Bp(0x283C0000);
            Bp(0xC008F2A4);
            Bp(0xC108E6F0);
            Bp(0xC208F80F);
            Bp(0xC308F070);
        } else {
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
        }
    }
    if (lbl_805FB4CC < 0xC00) {
        fn_8014E3EC(0xC00);
    }
    s32 nverts = 0;
    node = *(u8**)(*(u8**)(obj + 0x4) + ent + 0x4);
    u8* base = lbl_805FB4D0 + lbl_805FB4C4;
    for (; node != NULL; node = *(u8**)node) {
        s16 x = *(s16*)(node + 0xC);
        s16 y = *(s16*)(node + 0xE);
        s16 w = *(s16*)(node + 0x8);
        s16 h = *(s16*)(node + 0xA);
        s16 u = *(s16*)(node + 0x10);
        s16 v = *(s16*)(node + 0x12);
        u32 color = Bswap32(*(u32*)(node + 0x4));
        f32 x0 = ox + ((f32)x * sx);
        f32 y0 = oy + ((f32)y * sy);
        f32 u0 = 0.0f + ((f32)u * invW);
        f32 v0 = 0.0f + ((f32)v * invH);
        f32 x1 = ox + ((f32)(x + w) * sx);
        f32 y1 = oy + ((f32)(y + h) * sy);
        f32 u1 = invW + ((f32)((u + w) - 1) * invW);
        f32 v1 = invH + ((f32)((v + h) - 1) * invH);
        u8* q = base + (nverts * 0x18);
        // Vertex layout: {f32 u, v; u32 color; f32 x, y, z}.
        *(f32*)(q + 0x00) = u0;
        *(f32*)(q + 0x04) = v0;
        *(u32*)(q + 0x08) = color;
        *(f32*)(q + 0x0C) = x0;
        *(f32*)(q + 0x10) = y0;
        *(f32*)(q + 0x14) = 0.0f;
        *(f32*)(q + 0x18) = u1;
        *(f32*)(q + 0x1C) = v0;
        *(u32*)(q + 0x20) = color;
        *(f32*)(q + 0x24) = x1;
        *(f32*)(q + 0x28) = y0;
        *(f32*)(q + 0x2C) = 0.0f;
        *(f32*)(q + 0x30) = u0;
        *(f32*)(q + 0x34) = v1;
        *(u32*)(q + 0x38) = color;
        *(f32*)(q + 0x3C) = x0;
        *(f32*)(q + 0x40) = y1;
        *(f32*)(q + 0x44) = 0.0f;
        *(f32*)(q + 0x48) = u1;
        *(f32*)(q + 0x4C) = v1;
        *(u32*)(q + 0x50) = color;
        *(f32*)(q + 0x54) = x1;
        *(f32*)(q + 0x58) = y1;
        *(f32*)(q + 0x5C) = 0.0f;
        nverts += 4;
        if (nverts >= 0x80) {
            FlushQuads(nverts, base);
            if (lbl_805FB4CC < 0xC00) {
                fn_8014E3EC(0xC00);
            }
            nverts = 0;
            base = lbl_805FB4D0 + lbl_805FB4C4;
        }
    }
    if (nverts > 0) {
        FlushQuads(nverts, base);
    }
}

// Attaches `anim` to `track` (reference-counted), clamps the track length and restarts it.
static inline void BindTrack(u8* track, u8* anim) {
    *(s32*)(anim + 0x10) += 1;
    u8* old = *(u8**)(track + 0x14);
    if (old != NULL) {
        s32 refs = *(s32*)(old + 0x10) - 1;
        *(s32*)(old + 0x10) = refs;
        if (refs == 0) {
            fn_8016A2B8(old);
        }
    }
    *(u8**)(track + 0x14) = anim;
    s32 len = *(s32*)(anim + 0x18);
    s32 cap = *(s32*)(track + 0xC);
    if (cap < len) {
        len = cap;
    }
    *(s32*)(track + 0x10) = len;
    *(f32*)(track + 0x24) = 0.0f;
    *(s32*)(track + 0x1C) = 0;
    fn_8016A570(track);
    fn_8016A3F8(track);
    fn_8016A788(track);
}

// Index of `key` in the 8-byte entries of the table at res+0xC (count at res+8), or -1.
static inline s32 FindKey(u8* res, s32 key) {
    s32 count = *(s32*)(res + 0x8);
    u8* table = *(u8**)(res + 0xC);
    for (s32 i = 0; i < count; i++) {
        if (key == *(s32*)(table + i * 8)) {
            return i;
        }
    }
    return -1;
}

// 0x8018B78C
// Starts animation `anim` on the slot of `obj` that is playing resource `key` (three slots of
// 0x24 bytes at obj+4; slot +0 = key, +4 = resource, +8 = player, +0xC/+0x10 = time/speed,
// +0x14 = flags, +0x18 = track, +0x20 = user value), claiming a free slot if none is. Returns the
// slot (at obj+4+i*0x24) or NULL.
void* fn_8018B78C(u8* obj, u8* anim, s32 flags, f32 farg0, f32 farg1, s32 key, s32 userVal) {
    u8* res = NULL;       // r30: resource of the slot
    u8* slot = NULL;      // r29: slot (slot+0 = key)
    u8* clip = NULL;      // r28
    s32 i = 0;
    u8* p = obj;
    while (i < 3) {
        u32 k = *(u32*)(p + 0x4);
        if (k == 0xFFFFFFFFu) {
            break;
        }
        if ((u32)key == k) {
            u8* e = obj + (i * 0x24);
            res = *(u8**)(e + 0x8);
            slot = e + 4;
            break;
        }
        p += 0x24;
        i += 1;
    }
    if (res == NULL) {
        if (i >= 3) {
            return NULL;
        }
        u8* r = NULL;
        u8* holder = *(u8**)obj;
        if (FindKey(holder, key) != -1) {
            r = fn_80174788(holder, FindKey(holder, key));
        }
        res = r;
        u8* e = obj + (i * 0x24);
        *(u8**)(e + 0x8) = r;
        if (r == NULL) {
            return NULL;
        }
        *(s32*)(e + 0x4) = key;
        slot = e + 4;
    } else {
        clip = *(u8**)(*(u8**)(res + 0x14) + 0x14);
    }
    if (slot == NULL) {
        return slot;
    }
    u32 oldFlags = *(u32*)(slot + 0x14);
    *(u32*)(slot + 0x14) = (u32)flags & 0xFFFDFFFFu;
    *(f32*)(slot + 0x10) = farg0;
    *(f32*)(slot + 0xC) = farg1;
    s32 notShared = (oldFlags & 0x20000) == 0;
    *(s32*)(*(u8**)(res + 0x14) + 0x1C) = 0;
    if (*(u8**)(slot + 0x18) == NULL) {
        *(u8**)(slot + 0x18) = fn_8016B50C(NULL, *(s32*)(res + 0x8));
    }
    u8* track = *(u8**)(slot + 0x18);
    if (track != NULL &&
        *(s32*)(*(u8**)(res + 0x14) + 0x10) > *(s32*)(track + 0x10)) {
        u8* old = *(u8**)(track + 0x14);
        if (old != NULL) {
            s32 refs = *(s32*)(old + 0x10) - 1;
            *(s32*)(old + 0x10) = refs;
            if (refs == 0) {
                fn_8016A2B8(old);
            }
            *(u8**)(track + 0x14) = NULL;
        }
        *(s32*)track = 0;
        (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(track, lbl_805FB608);
        *(u8**)(slot + 0x18) = fn_8016B50C(NULL, *(s32*)(res + 0x8));
    }
    if (*(s32*)(obj + 0x74) == 0) {
        return NULL;
    }
    if (*(s32*)(obj + 0x78) == 0) {
        return NULL;
    }
    if (clip != NULL && notShared) {
        u8* t = *(u8**)(slot + 0x18);
        BindTrack(t, clip);
        fn_8016B478(*(u8**)(slot + 0x18), *(u8**)(res + 0x14));
        if (flags & 0x10000) {
            *(f32*)(slot + 0x8) = 0.0f;
        } else {
            *(f32*)(slot + 0x8) = farg0;
        }
        if (flags & 1) {
            f32 a[3];
            f32 b[3];
            u8* bones = *(u8**)(res + 0x4);
            u8* b0 = bones + (*(s32*)(obj + 0xA0) * 0x30);
            a[0] = *(f32*)(b0 + 0x24);
            a[1] = *(f32*)(b0 + 0x28);
            a[2] = *(f32*)(b0 + 0x2C);
            u8* b1 = *(u8**)(res + 0x4) + (*(s32*)(obj + 0xA4) * 0x30);
            b[0] = *(f32*)(b1 + 0x24);
            b[1] = *(f32*)(b1 + 0x28);
            b[2] = *(f32*)(b1 + 0x2C);
            BindTrack(*(u8**)(res + 0x14), anim);
            f32 t1 = fn_8018AF28(obj, a, b);
            u8* tr = *(u8**)(res + 0x14);
            f32 prev = *(f32*)(tr + 0x24);
            if (t1 < 0.0f) {
                u8* seq = *(u8**)(tr + 0x14);
                while (t1 < 0.0f) {
                    t1 += *(f32*)(seq + 0x20);
                }
            }
            u8* seq2 = *(u8**)(tr + 0x14);
            while (t1 > *(f32*)(seq2 + 0x20)) {
                t1 -= *(f32*)(seq2 + 0x20);
            }
            *(f32*)(tr + 0x24) = t1;
            *(s32*)(tr + 0x1C) = 0;
            if (t1 < prev) {
                fn_8016A3F8(tr);
            }
            fn_8016A43C(tr);
            fn_8016A788(tr);
            fn_8018A948(obj, 0.0f);
        } else {
            BindTrack(*(u8**)(res + 0x14), anim);
        }
    } else if (*lbl_805FB648 != 0) {
        u8* holder = *(u8**)obj;
        s32 idx = FindKey(holder, key);
        fn_8016B024(*(u8**)(slot + 0x18), 0, *(u8**)(holder + 0x14), idx, *(s32*)(res + 0x8));
        BindTrack(*(u8**)(res + 0x14), anim);
        *(f32*)(slot + 0x8) = farg0;
        *(f32*)(slot + 0x10) = farg0;
        *(f32*)(slot + 0xC) = farg1;
    } else {
        BindTrack(*(u8**)(res + 0x14), anim);
    }
    *(u32*)(slot + 0x14) &= 0xFFFFFFEFu;
    *(s32*)(slot + 0x20) = userVal;
    return slot;
}

}  // extern "C"
