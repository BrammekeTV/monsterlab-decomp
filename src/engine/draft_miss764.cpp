// Font blob helpers and misc 0x8017xxxx-0x8019xxxx engine code: font endian swap, glyph quad and
// string drawing, record parsing.
#include "types.h"
#include "revolution.h"
#include <math.h>

extern "C" {
void fn_8014E3EC(s32 n);                    // 0x8014E3EC
void fn_80151E38(s32 quads, void* verts);   // 0x80151E38

extern u8  lbl_80348950[];     // 0x80348950 (0x20-byte records, u32 at +0)
extern u8  lbl_80348A10[];     // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32 lbl_80348AD0;       // 0x80348AD0
extern s32 lbl_8040ADF0[8];    // 0x8040ADF0
extern s32 lbl_8040AE10[8];    // 0x8040AE10
extern s32 lbl_805FB4B0;       // 0x805FB4B0
extern s32 lbl_805FB4C4;       // 0x805FB4C4 (vertex buffer write offset)
extern s32 lbl_805FB4CC;       // 0x805FB4CC (vertex buffer bytes left)
extern u8* lbl_805FB4D0;       // 0x805FB4D0 (vertex buffer base)

void fn_80189988(u8* font, u8* glyph, u32 color, f32 x, f32 y, f32 sx, f32 sy);   // 0x80189988
void fn_8017A618(void* p);                  // 0x8017A618
void fn_80142E94(u8* stream, s32 v);        // 0x80142E94 (writes a u16 at the stream cursor)
void fn_80142EA8(u8* stream, s32 v);        // 0x80142EA8 (writes a u32 at the stream cursor)
void fn_80142EBC(u8* stream, f32 v);        // 0x80142EBC (writes an f32 at the stream cursor)

extern u8* lbl_805FB608;       // 0x805FB608 (heap object: slot +0 = alloc, slot +4 = free, called as (args, heap))
}

typedef void* (*HeapAllocFn)(u32, u32, u32, void*, u32, void*);

// Record stream reader: *s is the cursor.
static inline u32 RdU32(u8** s) { u32 v = *(u32*)*s; *s += 4; return v; }
static inline f32 RdF32(u8** s) { f32 v = *(f32*)*s; *s += 4; return v; }
static inline u16 RdU16(u8** s) { u16 v = *(u16*)*s; *s += 2; return v; }
static inline u8  RdU8(u8** s)  { u8 v = **s; *s += 1; return v; }
static inline u8  RdBool(u8** s) { return RdU32(s) != 0; }

// Record stream writer (the original calls these through the table at 0x80375E08).
static inline void WrU16(u8** s, u32 v) { fn_80142E94((u8*)s, (s32)v); }
static inline void WrU32(u8** s, u32 v) { fn_80142EA8((u8*)s, (s32)v); }
static inline void WrF32(u8** s, f32 v) { fn_80142EBC((u8*)s, v); }

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

static inline u32 Swap32(u32 v) {
    return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24);
}

static inline void Swap16At(u8* p) {
    u16 v = *(u16*)p;
    *(u16*)p = (u16)((v << 8) | (v >> 8));
}

extern "C" {

// 0x8017AD34
// Allocates a 0xD20-byte record from the heap and fills it from the record stream *s (fields are
// 32-bit words, flags are booleans stored as bytes, a few halfwords/bytes, then float groups);
// parent is stored at +0. Returns the new record.
void* fn_8017AD34(u8* parent, u8** s) {
    u8* p = (u8*)(*(HeapAllocFn*)lbl_805FB608)(0xD20, 0x10, 0x12, parent ? (void*)(parent + 0x80) : (void*)0, 0, lbl_805FB608);
    *(u8**)p = parent;
    fn_8017A618(p);
    *(u32*)(p + 0x4) = RdU32(s);
    *(u32*)(p + 0x8) = RdU32(s);
    *(u32*)(p + 0xC) = RdU32(s);
    u32 v10 = RdU32(s);
    s32 n4 = (s32)((v10 + 1) & ~1u);
    *(u32*)(p + 0x10) = v10;
    s32 c1 = *(s32*)(p + 0x4);
    s32 c3 = *(s32*)(p + 0x8);
    s32 n3 = (s32)((*(u32*)(p + 0xC) + 1) & ~1u);

    u8* q = p;
    for (s32 i = 0; i < c1; i++) {
        *(u32*)(q + 0x18) = RdU32(s);
        *(u32*)(q + 0x20) = RdU32(s);
        *(u8*)(q + 0x24) = RdBool(s);
        *(u8*)(q + 0x25) = (u8)RdU32(s);
        *(u32*)(q + 0x28) = RdU32(s);
        *(u32*)(q + 0x2C) = RdU32(s);
        *(u32*)(q + 0x30) = RdU32(s);
        *(u32*)(q + 0x34) = RdU32(s);
        *(u8*)(q + 0x40) = RdBool(s);
        *(u8*)(q + 0x41) = (u8)RdU32(s);
        *(u32*)(q + 0x44) = RdU32(s);
        *(u32*)(q + 0x48) = RdU32(s);
        *(u32*)(q + 0x4C) = RdU32(s);
        *(u32*)(q + 0x50) = RdU32(s);
        q += 0x48;
    }
    q = p;
    for (s32 i = 0; i < 8; i++) {
        *(u32*)(q + 0xA4) = RdU32(s);
        *(u32*)(q + 0xA8) = RdU32(s);
        *(u32*)(q + 0xAC) = RdU32(s);
        *(u32*)(q + 0xB0) = RdU32(s);
        *(u32*)(q + 0xB4) = RdU32(s);
        *(u32*)(q + 0xBC) = RdU32(s);
        *(u8*)(q + 0xB8) = RdU8(s);
        *(u8*)(q + 0xC0) = RdU8(s) != 0;
        *(u32*)(q + 0xC4) = RdU8(s);
        *s = (u8*)(((uintptr_t)*s + 3) & ~(uintptr_t)3);
        q += 0x24;
    }
    q = p;
    for (s32 i = 0; i < c3; i++) {
        *(u8*)(q + 0x1C4) = RdBool(s);
        *(u16*)(q + 0x1C6) = RdU16(s);
        *(u16*)(q + 0x1C8) = RdU16(s);
        *(u32*)(q + 0x1CC) = RdU32(s);
        *(u8*)(q + 0x1D0) = RdBool(s);
        *(u8*)(q + 0x1D1) = RdBool(s);
        *(u8*)(q + 0x1D2) = RdBool(s);
        *(u8*)(q + 0x1D3) = RdBool(s);
        *(u32*)(q + 0x1D4) = RdU32(s);
        *(u32*)(q + 0x1D8) = RdU32(s);
        *(u32*)(q + 0x1DC) = RdU32(s);
        *(u32*)(q + 0x1E0) = RdU32(s);
        *(u32*)(q + 0x1E4) = RdU32(s);
        *(u32*)(q + 0x1E8) = RdU32(s);
        *(u32*)(q + 0x1EC) = RdU32(s);
        *(u8*)(q + 0x1F0) = RdBool(s);
        *(u32*)(q + 0x1F4) = RdU32(s);
        q += 0x34;
    }
    q = p;
    for (s32 i = 0; i < 3; i++) {
        *(u32*)(q + 0x364) = RdU32(s);
        *(f32*)(q + 0x368) = RdF32(s);
        *(f32*)(q + 0x36C) = RdF32(s);
        *(f32*)(q + 0x370) = RdF32(s);
        *(f32*)(q + 0x374) = RdF32(s);
        q += 0x14;
    }
    q = p;
    for (s32 i = 0; i < 2; i++) {
        *(u32*)(q + 0x3A0) = RdU32(s);
        *(f32*)(q + 0x3A4) = RdF32(s);
        *(f32*)(q + 0x3A8) = RdF32(s);
        *(f32*)(q + 0x3AC) = RdF32(s);
        *(f32*)(q + 0x3B0) = RdF32(s);
        *(u32*)(q + 0x3B4) = RdU32(s);
        *(f32*)(q + 0x3B8) = RdF32(s);
        *(f32*)(q + 0x3BC) = RdF32(s);
        *(f32*)(q + 0x3C0) = RdF32(s);
        *(f32*)(q + 0x3C4) = RdF32(s);
        q += 0x28;
    }
    q = p;
    for (s32 i = 0; i < 2; i++) {
        for (s32 k = 0; k < 8; k++) *(u32*)(q + 0x3F0 + k * 4) = RdU32(s);
        q += 0x20;
    }
    q = p;
    for (s32 i = 0; i < n3; i++) {
        *(u8*)(q + 0x430) = RdBool(s);
        for (s32 k = 0x434; k <= 0x464; k += 4) *(u32*)(q + k) = RdU32(s);
        *(u8*)(q + 0x468) = RdBool(s);
        for (s32 k = 0x46C; k <= 0x488; k += 4) *(u32*)(q + k) = RdU32(s);
        *(u8*)(q + 0x48C) = RdBool(s);
        *(u32*)(q + 0x490) = RdU32(s);
        *(u32*)(q + 0x494) = RdU32(s);
        q += 0x68;
    }
    q = p;
    for (s32 i = 0; i < n4; i++) {
        *(u32*)(q + 0xAB0) = RdU32(s);
        *(u32*)(q + 0xAB4) = RdU32(s);
        *(u32*)(q + 0xAB8) = RdU32(s);
        *(u32*)(q + 0xABC) = RdU32(s);
        q += 0x10;
    }
    q = p;
    for (s32 i = 0; i < n3; i++) {
        *(u32*)(q + 0xAF0) = RdU32(s);
        *(u32*)(q + 0xAF4) = RdU32(s);
        *(u32*)(q + 0xAF8) = RdU32(s);
        *(u32*)(q + 0xAFC) = RdU32(s);
        *(u32*)(q + 0xB00) = RdU32(s);
        *(u32*)(q + 0xB04) = RdU32(s);
        *(u8*)(q + 0xB08) = RdBool(s);
        *(u8*)(q + 0xB09) = RdBool(s);
        *(u32*)(q + 0xB0C) = RdU32(s);
        q += 0x20;
    }
    *(f32*)(p + 0xCF0) = RdF32(s);
    *(u32*)(p + 0xCF4) = RdU32(s);
    *(u32*)(p + 0xCF8) = RdU32(s);
    *(u8*)(p + 0xCFC) = RdBool(s);
    *(u8*)(p + 0xCFD) = RdBool(s);
    *(u32*)(p + 0xD00) = RdU32(s);
    *(u32*)(p + 0xD04) = RdU32(s);
    *(u32*)(p + 0xD08) = RdU32(s);
    *(u32*)(p + 0xD0C) = RdU32(s);
    *(u8*)(p + 0xD10) = RdBool(s);
    *(u8*)(p + 0xD11) = RdBool(s);
    *(u8*)(p + 0xD12) = RdBool(s);
    *(u32*)(p + 0xD14) = RdU32(s);
    *(u32*)(p + 0xD18) = RdU32(s);
    *(u8*)(p + 0xD1C) = RdBool(s);
    return p;
}

// 0x80181FA8
// Writes the record built by fn_8017AD34 back to the record stream *s (same field order; flags
// and bytes widened to words, the 3 bytes at +0xB8/+0xC0/+0xC4 of each of the 8 entries written
// raw and zero-padded to a 4-byte boundary).
void fn_80181FA8(u8* p, u8** s) {
    WrU32(s, *(u32*)(p + 0x4));
    WrU32(s, *(u32*)(p + 0x8));
    WrU32(s, *(u32*)(p + 0xC));
    WrU32(s, *(u32*)(p + 0x10));
    s32 c1 = *(s32*)(p + 0x4);
    s32 c3 = *(s32*)(p + 0x8);
    s32 n3 = (s32)((*(u32*)(p + 0xC) + 1) & ~1u);
    s32 n4 = (s32)((*(u32*)(p + 0x10) + 1) & ~1u);

    u8* q = p;
    for (s32 i = 0; i < c1; i++) {
        WrU32(s, *(u32*)(q + 0x18));
        WrU32(s, *(u32*)(q + 0x20));
        WrU32(s, *(u8*)(q + 0x24));
        WrU32(s, *(u8*)(q + 0x25));
        WrU32(s, *(u32*)(q + 0x28));
        WrU32(s, *(u32*)(q + 0x2C));
        WrU32(s, *(u32*)(q + 0x30));
        WrU32(s, *(u32*)(q + 0x34));
        WrU32(s, *(u8*)(q + 0x40));
        WrU32(s, *(u8*)(q + 0x41));
        WrU32(s, *(u32*)(q + 0x44));
        WrU32(s, *(u32*)(q + 0x48));
        WrU32(s, *(u32*)(q + 0x4C));
        WrU32(s, *(u32*)(q + 0x50));
        q += 0x48;
    }
    q = p;
    for (s32 i = 0; i < 8; i++) {
        WrU32(s, *(u32*)(q + 0xA4));
        WrU32(s, *(u32*)(q + 0xA8));
        WrU32(s, *(u32*)(q + 0xAC));
        WrU32(s, *(u32*)(q + 0xB0));
        WrU32(s, *(u32*)(q + 0xB4));
        WrU32(s, *(u32*)(q + 0xBC));
        u8* cur = *s;
        cur[0] = *(u8*)(q + 0xB8);
        cur[1] = *(u8*)(q + 0xC0);
        cur[2] = (u8)*(u32*)(q + 0xC4);
        *s = cur + 3;
        while (((uintptr_t)*s & 3) != 0) {
            **s = 0;
            *s += 1;
        }
        q += 0x24;
    }
    q = p;
    for (s32 i = 0; i < c3; i++) {
        WrU32(s, *(u8*)(q + 0x1C4));
        WrU16(s, *(u16*)(q + 0x1C6));
        WrU16(s, *(u16*)(q + 0x1C8));
        WrU32(s, *(u32*)(q + 0x1CC));
        WrU32(s, *(u8*)(q + 0x1D0));
        WrU32(s, *(u8*)(q + 0x1D1));
        WrU32(s, *(u8*)(q + 0x1D2));
        WrU32(s, *(u8*)(q + 0x1D3));
        WrU32(s, *(u32*)(q + 0x1D4));
        WrU32(s, *(u32*)(q + 0x1D8));
        WrU32(s, *(u32*)(q + 0x1DC));
        WrU32(s, *(u32*)(q + 0x1E0));
        WrU32(s, *(u32*)(q + 0x1E4));
        WrU32(s, *(u32*)(q + 0x1E8));
        WrU32(s, *(u32*)(q + 0x1EC));
        WrU32(s, *(u8*)(q + 0x1F0));
        WrU32(s, *(u32*)(q + 0x1F4));
        q += 0x34;
    }
    q = p;
    for (s32 i = 0; i < 3; i++) {
        WrU32(s, *(u32*)(q + 0x364));
        WrF32(s, *(f32*)(q + 0x368));
        WrF32(s, *(f32*)(q + 0x36C));
        WrF32(s, *(f32*)(q + 0x370));
        WrF32(s, *(f32*)(q + 0x374));
        q += 0x14;
    }
    q = p;
    for (s32 i = 0; i < 4; i++) {
        WrU32(s, *(u32*)(q + 0x3A0));
        WrF32(s, *(f32*)(q + 0x3A4));
        WrF32(s, *(f32*)(q + 0x3A8));
        WrF32(s, *(f32*)(q + 0x3AC));
        WrF32(s, *(f32*)(q + 0x3B0));
        q += 0x14;
    }
    q = p;
    for (s32 i = 0; i < 4; i++) {
        WrU32(s, *(u32*)(q + 0x3F0));
        WrU32(s, *(u32*)(q + 0x3F4));
        WrU32(s, *(u32*)(q + 0x3F8));
        WrU32(s, *(u32*)(q + 0x3FC));
        q += 0x10;
    }
    q = p;
    for (s32 i = 0; i < n3; i++) {
        WrU32(s, *(u8*)(q + 0x430));
        for (s32 k = 0x434; k <= 0x464; k += 4) WrU32(s, *(u32*)(q + k));
        WrU32(s, *(u8*)(q + 0x468));
        for (s32 k = 0x46C; k <= 0x488; k += 4) WrU32(s, *(u32*)(q + k));
        WrU32(s, *(u8*)(q + 0x48C));
        WrU32(s, *(u32*)(q + 0x490));
        WrU32(s, *(u32*)(q + 0x494));
        q += 0x68;
    }
    q = p;
    for (s32 i = 0; i < n4; i++) {
        WrU32(s, *(u32*)(q + 0xAB0));
        WrU32(s, *(u32*)(q + 0xAB4));
        WrU32(s, *(u32*)(q + 0xAB8));
        WrU32(s, *(u32*)(q + 0xABC));
        q += 0x10;
    }
    q = p;
    for (s32 i = 0; i < n3; i++) {
        WrU32(s, *(u32*)(q + 0xAF0));
        WrU32(s, *(u32*)(q + 0xAF4));
        WrU32(s, *(u32*)(q + 0xAF8));
        WrU32(s, *(u32*)(q + 0xAFC));
        WrU32(s, *(u32*)(q + 0xB00));
        WrU32(s, *(u32*)(q + 0xB04));
        WrU32(s, *(u8*)(q + 0xB08));
        WrU32(s, *(u8*)(q + 0xB09));
        WrU32(s, *(u32*)(q + 0xB0C));
        q += 0x20;
    }
    WrF32(s, *(f32*)(p + 0xCF0));
    WrU32(s, *(u32*)(p + 0xCF4));
    WrU32(s, *(u32*)(p + 0xCF8));
    WrU32(s, *(u8*)(p + 0xCFC));
    WrU32(s, *(u8*)(p + 0xCFD));
    WrU32(s, *(u32*)(p + 0xD00));
    WrU32(s, *(u32*)(p + 0xD04));
    WrU32(s, *(u32*)(p + 0xD08));
    WrU32(s, *(u32*)(p + 0xD0C));
    WrU32(s, *(u8*)(p + 0xD10));
    WrU32(s, *(u8*)(p + 0xD11));
    WrU32(s, *(u8*)(p + 0xD12));
    WrU32(s, *(u32*)(p + 0xD14));
    WrU32(s, *(u32*)(p + 0xD18));
    WrU32(s, *(u8*)(p + 0xD1C));
}

// 0x801894F4
// Byte-swaps the 16-bit fields of a font blob in place: header counts at +8/+0xA, the 8 x 8
// halfword table at +0xC, then the glyph records (4 halfwords each) and the kerning records
// (3 halfwords each) that follow the page pointer table.
void fn_801894F4(u8* font) {
    Swap16At(font + 0x8);
    Swap16At(font + 0xA);
    u8* t = font;
    for (s32 i = 0; i < 8; i++) {
        for (s32 k = 0; k < 8; k++) Swap16At(t + 0xC + k * 2);
        t += 0x10;
    }
    u8* g = font + 0x8C + font[3] * 4;
    s32 i = 0;
    while (i < (s32)*(u16*)(font + 0x8) + 1) {
        i++;
        Swap16At(g + 0x0);
        Swap16At(g + 0x2);
        Swap16At(g + 0x4);
        Swap16At(g + 0x6);
        g += 0x10;
    }
    u8* k = font + 0x8C + font[3] * 4 + (u32)*(u16*)(font + 0x8) * 0x10;
    s32 j = 0;
    while (j < (s32)*(u16*)(font + 0xA) + 1) {
        j++;
        Swap16At(k + 0x0);
        Swap16At(k + 0x2);
        Swap16At(k + 0x4);
        k += 0x6;
    }
}

// 0x80189988
// Draws one glyph as a textured quad at (x, y): `glyph` is a glyph record of `font`, `color` the
// vertex colour, sx/sy divide the half-size border added around the glyph's texture rectangle.
// Sets up the texture state (BP writes) when the glyph's page changed, then appends 4 vertices
// (0x18 bytes each: u, v, colour, x, y, z) to the vertex buffer and submits them as one quad.
void fn_80189988(u8* font, u8* glyph, u32 color, f32 x, f32 y, f32 sx, f32 sy) {
    u8* tex = *(u8**)(font + glyph[0xE] * 4 + 0x8C);
    f32 h = 0.5f * (f32)font[4];
    s32 ox = (s8)glyph[0xA];
    s32 oy = (s8)glyph[0xB];
    s32 rx = (s8)glyph[0xC];
    s32 ry = (s8)glyph[0xD];
    s32 u = *(u16*)(glyph + 0x2);
    s32 v = *(u16*)(glyph + 0x4);
    f32 invW = 1.0f / (f32)(s32)*(u16*)(tex + 0x0);
    f32 invH = 1.0f / (f32)(s32)*(u16*)(tex + 0x2);
    f32 hx = h / sx;
    f32 hy = h / sy;
    f32 gx = x + (f32)ox;
    f32 gy = y + (f32)oy;
    f32 x0 = gx - h;
    f32 y0 = (y + (f32)oy) - h;
    f32 x1 = h + ((f32)rx + gx);
    f32 y1 = h + ((f32)ry + (y + (f32)oy));
    f32 u0 = invW * ((f32)u - hx);
    f32 v0 = invH * ((f32)v - hy);

    GxBpWrite(0xFE000060);
    GxBpWrite(((u32)tex[0x12] << 5) | 0x80000000u);

    f32 u1 = invW * (hx + (f32)(u + rx));
    f32 v1 = invH * (hy + (f32)(v + ry));

    if ((u32)lbl_8040ADF0[0] != *(u32*)(tex + 0x18) || lbl_8040AE10[0] != 0) {
        lbl_805FB4B0 = lbl_805FB4B0 + 1;
        lbl_8040ADF0[0] = *(s32*)(tex + 0x18);
        lbl_8040AE10[0] = 0;
        u32 cfg = lbl_80348AD0;
        u32 page = tex[0x13];
        GxBpWrite((((u32)tex[0xF] - 1) & 0xF) << 12 | 0x84000000u);
        s32 second = *(s32*)(lbl_80348A10 + page * 0x20);
        GxBpWrite(((u32)*(u16*)(tex + 0x4) - 1) | (((u32)*(u16*)(tex + 0x6) - 1) << 10) |
                  ((u32)tex[0x10] << 20) | 0x88000000u);
        GxBpWrite((*(u32*)(lbl_80348950 + page * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
        if (second >= 0) {
            GxBpWrite(((u32)second >> 5) | 0x900D0000u | 0x8000u);
        } else {
            GxBpWrite(0x90000000u);
        }
        GxBpWrite(((*(u32*)(tex + 0x18) >> 5) & 0xFFFFFF) | 0x94000000u);
        if (tex[0xD] != 0) {
            u32 g = (cfg >> 9) & 0x3FF;
            GxBpWrite(((g | 0x98000000u) & ~0x3FC00u) | (((u32)tex[0x11] << 10) & 0x3FC00u));
            GxBpWrite(0xFEFFFF00u);
            GxBpWrite(0x0F000000u);
            GxBpWrite(((*(u32*)(tex + 0x1C) >> 5) & 0xFFFFFF) | 0x64000000u);
            GxBpWrite(((g | 0x65000000u) & ~0x1FC00u) | ((((s32)tex[0xD] >> 1) << 10) & 0x1FC00u));
            GxBpWrite(0xFEFFFF00u);
            GxBpWrite(0x0F000000u);
        }
    }
    GxBpWrite(((u32)*(u16*)(tex + 0x4) - 1) | 0x30000000u);
    GxBpWrite(((u32)*(u16*)(tex + 0x6) - 1) | 0x31000000u);

    if (lbl_805FB4CC < 0x60) fn_8014E3EC(0x60);
    s32 pos = lbl_805FB4C4;
    s32 left = lbl_805FB4CC;
    u8* q = lbl_805FB4D0 + pos;
    lbl_805FB4CC = left - 0x60;
    lbl_805FB4C4 = pos + 0x60;
    // The original reverses the colour's bytes in memory; as a value that is the same swap.
    u32 c = Swap32(color);
    *(f32*)(q + 0x00) = u0;
    *(f32*)(q + 0x04) = v0;
    *(u32*)(q + 0x08) = c;
    *(f32*)(q + 0x0C) = x0;
    *(f32*)(q + 0x10) = y0;
    *(f32*)(q + 0x14) = 0.0f;
    *(f32*)(q + 0x18) = u1;
    *(f32*)(q + 0x1C) = v0;
    *(u32*)(q + 0x20) = c;
    *(f32*)(q + 0x24) = x1;
    *(f32*)(q + 0x28) = y0;
    *(f32*)(q + 0x2C) = 0.0f;
    *(f32*)(q + 0x30) = u0;
    *(f32*)(q + 0x34) = v1;
    *(u32*)(q + 0x38) = c;
    *(f32*)(q + 0x3C) = x0;
    *(f32*)(q + 0x40) = y1;
    *(f32*)(q + 0x44) = 0.0f;
    *(f32*)(q + 0x48) = u1;
    *(f32*)(q + 0x4C) = v1;
    *(u32*)(q + 0x50) = c;
    *(f32*)(q + 0x54) = x1;
    *(f32*)(q + 0x58) = y1;
    *(f32*)(q + 0x5C) = 0.0f;
    fn_80151E38(1, q);
}

}  // extern "C"

// Looks up the glyph record of byte c in the font's hash buckets (indexed by c & 0x3F, records
// chained by consecutive entries with the same low 6 bits); NULL when absent.
static u8* FontFindGlyph(u8* font, u8 c) {
    u8* e = font + 0x8C + font[3] * 4 + (u32)*(u16*)(font + ((c & 0x3F) << 1) + 0xC) * 0x10;
    for (;;) {
        u32 code = *(u16*)e;
        if (code == 0) return NULL;
        if (code == c) return e;
        if ((s32)(code & 0x3F) != (s32)(c & 0x3F)) return NULL;
        e += 0x10;
    }
}

extern "C" {

// 0x80189EFC
// Draws the NUL-terminated string `str` with `font` starting at (x, y), advancing x by each
// glyph's advance (s8 at +8) plus the kerning between consecutive glyphs. colors[i] is the colour
// of the i-th character (the index is clamped to count). Characters without a glyph are skipped.
void fn_80189EFC(u8* font, f32 x, f32 y, f32 sx, f32 sy, const u8* str, const u32* colors, u32 count) {
    u32 i = 0;
    u8* prev;
    for (;;) {
        u8 c = *str;
        if ((s8)c == 0) return;
        u32 col = colors[i];
        str++;
        u32 old = i;
        i++;
        if (!(old < count)) i = count;
        prev = FontFindGlyph(font, c);
        if (prev != NULL) {
            fn_80189988(font, prev, col, x, y, sx, sy);
            x += (f32)(s32)(s8)prev[8];
            break;
        }
    }
    for (;;) {
        u8 c = *str;
        if ((s8)c == 0) return;
        u32 col = colors[i];
        str++;
        u32 old = i;
        i++;
        if (!(old < count) && count > 1) i = count;
        u8* cur = FontFindGlyph(font, c);
        if (cur == NULL) continue;
        s32 kern = 0;
        if ((prev[0xF] & 2) && (cur[0xF] & 1)) {
            u32 want = *(u16*)cur;
            u8* k = font + 0x8C + font[3] * 4 + (u32)*(u16*)(font + 0x8) * 0x10 + (u32)*(u16*)(prev + 0x6) * 6;
            for (;;) {
                if (*(u16*)k != *(u16*)prev) { kern = 0; break; }
                u32 second = *(u16*)(k + 0x2);
                if (second > want) { kern = 0; break; }
                if (second == want) { kern = *(s16*)(k + 0x4); break; }
                k += 6;
            }
        }
        prev = cur;
        x = x + (f32)kern;
        fn_80189988(font, cur, col, x, y, sx, sy);
        x += (f32)(s32)(s8)cur[8];
    }
}

}  // extern "C"
