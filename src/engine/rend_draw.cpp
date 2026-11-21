// Renderer: drawing from a vertex array of {f32 u, v; u32 colour; f32 x, y, z} (0x18 bytes),
// and the GX state for drawing a full-screen layer.
// Original: 0x80151644, 0x80152D84 (auto_03_801467F8_text.s), 0x80159258 (auto_03_80156AAC_text.s)
#include "rend_dl_inline.h"

extern "C" {
void fn_8014DE68(void);                 // 0x8014DE68: close the display list and open the next
void fn_801542DC(void);                 // 0x801542DC
void fn_80155D70(void);                 // 0x80155D70
void fn_80154974(void);                 // 0x80154974
void fn_801514F0(void);                 // 0x801514F0
void fn_80154290(void);                 // 0x80154290
}

// Points the three GX arrays (position, colour 0, texture coordinate 0) at the vertex array and
// selects 8-bit indices when every index fits (16-bit otherwise); then invalidates the vertex cache.
static inline void SetVertexArrays(u8* verts, s32 numVerts) {
    XfHead(0, 0x1008); GXWGFifo._u32 = 0x11;        // 1 colour, 1 texture coordinate
    Cp(0xA0, (u32)(uintptr_t)(verts + 0xC) & 0x3FFFFFFF);   // lint:ok
    Cp(0xA2, (u32)(uintptr_t)(verts + 8) & 0x3FFFFFFF);   // lint:ok
    Cp(0xA4, (u32)(uintptr_t)verts & 0x3FFFFFFF);   // lint:ok
    Cp(0xB0, 0x18);
    Cp(0xB2, 0x18);
    Cp(0xB4, 0x18);
    if (numVerts < 0xFF) {
        Cp(0x50, 0x4400);
        Cp(0x60, 2);
    } else {
        Cp(0x50, 0x6600);
        Cp(0x60, 3);
    }
    GXWGFifo._u8 = 0x48;
}

// 0x80151644
// Draws `count` vertices of the array in order (index i for position, colour and texture
// coordinate). `prim` is the GX draw command byte.
extern "C" void fn_80151644(s32 prim, s32 count, u8* verts) {
    fn_80142ED0(verts, count * 0x18);
    SetVertexArrays(verts, count);
    if (count < 0xFF) {
        EnsureDLRoom(count * 3 + 0x10C0);
        GXWGFifo._u8 = (u8)prim;
        GXWGFifo._u16 = (u16)count;
        for (s32 i = 0; i < count; i++) {
            GXWGFifo._u8 = (u8)i;
            GXWGFifo._u8 = (u8)i;
            GXWGFifo._u8 = (u8)i;
        }
    } else {
        EnsureDLRoom(count * 6 + 0x10C0);
        GXWGFifo._u8 = (u8)prim;
        GXWGFifo._u16 = (u16)count;
        for (s32 i = 0; i < count; i++) {
            GXWGFifo._u16 = (u16)i;
            GXWGFifo._u16 = (u16)i;
            GXWGFifo._u16 = (u16)i;
        }
    }
}

// 0x80152D84
// Draws `indexCount` vertices picked from the array by `indices`.
extern "C" void fn_80152D84(s32 prim, s32 indexCount, const u16* indices, s32 numVerts, u8* verts) {
    fn_80142ED0(verts, numVerts * 0x18);
    SetVertexArrays(verts, numVerts);
    if (numVerts < 0xFF) {
        EnsureDLRoom(indexCount * 3 + 0x10C0);
        GXWGFifo._u8 = (u8)prim;
        GXWGFifo._u16 = (u16)indexCount;
        for (s32 i = 0; i < indexCount; i++) {
            u16 v = indices[i];
            GXWGFifo._u8 = (u8)v;
            GXWGFifo._u8 = (u8)v;
            GXWGFifo._u8 = (u8)v;
        }
    } else {
        EnsureDLRoom(indexCount * 6 + 0x10C0);
        GXWGFifo._u8 = (u8)prim;
        GXWGFifo._u16 = (u16)indexCount;
        for (s32 i = 0; i < indexCount; i++) {
            u16 v = indices[i];
            GXWGFifo._u16 = v;
            GXWGFifo._u16 = v;
            GXWGFifo._u16 = v;
        }
    }
}

static inline u32 ClampByte(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return (u32)v;
}

// The three register sets the shadow flags (lbl_805FB50C bit 0 and bit 1) choose between.
static inline void StateOneChannelOneTex(void) {
    Bp(0xFE00000F); Bp(1);
    XfHead(0, 0x103F); GXWGFifo._u32 = 1;
    Bp(0xFE003C00); Bp(0);
    XfHead(0, 0x100E); GXWGFifo._u32 = 0x442;
    XfHead(0, 0x1010); GXWGFifo._u32 = 0x442;
    Bp(0x28380040);
    Bp(0xC008F8AF);
    Bp(0xC108F2F0);
}
static inline void StateOneChannelNoTex(void) {
    Bp(0xFE00000F); Bp(0);
    XfHead(0, 0x103F); GXWGFifo._u32 = 0;
    Bp(0xFE003C00); Bp(0);
    XfHead(0, 0x100E); GXWGFifo._u32 = 0x442;
    XfHead(0, 0x1010); GXWGFifo._u32 = 0x442;
    Bp(0x28380000);
    Bp(0xC008FFFA);
    Bp(0xC108FFD0);
}
static inline void StateTwoStages(void) {
    Bp(0xFE00000F); Bp(1);
    XfHead(0, 0x103F); GXWGFifo._u32 = 1;
    Bp(0xFE003C00); Bp(0x400);
    XfHead(0, 0x1010); GXWGFifo._u32 = 0x441;
    Bp(0x283C0000);
    Bp(0xC008F2A4);
    Bp(0xC108E6F0);
    Bp(0xC208F80F);
    Bp(0xC308F070);
}

// 0x80159258
// GX state for a textured full-screen layer: orthographic projection, white material colour,
// identity position matrix, texturing on.
extern "C" void fn_80159258(void) {
    if (fn_8015BA30(CurDL()) + 0x134B > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(CurDL());
    }
    fn_801542DC();
    Bp(0xFE00000E);
    Bp(0x4000000E);
    fn_80155D70();
    fn_80154974();

    XfHead(6, 0x1020);
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = -0.5f;
    GXWGFifo._u32 = 0;

    f32 colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    if (fn_8015BA30(CurDL()) + 0x1107 > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(CurDL());
    }
    if (lbl_805FB50C & 1) {
        u32 tex = lbl_805FB50C & 2;
        lbl_805FB50C &= 0xFE;
        if (tex) {
            StateOneChannelOneTex();
        } else {
            StateOneChannelNoTex();
        }
    }
    u32 r = ClampByte((s32)(255.0f * colour[0]));
    u32 g = ClampByte((s32)(255.0f * colour[1]));
    u32 b = ClampByte((s32)(255.0f * colour[2]));
    u32 a = ClampByte((s32)(255.0f * colour[3]));
    XfHead(0, 0x100C); GXWGFifo._u32 = a | (b << 8) | (r << 24) | (g << 16);
    if (lbl_805FB50C & 4) {
        lbl_805FB50C &= ~4;
        lbl_805FB514 |= 0x40;
        Bp((u32)lbl_805FB514 | 0x43000000);
        Bp(0xF33F0000);
    }

    fn_801514F0();
    Bp(0xFE000001);
    Bp(0x41000000);
    lbl_805FB50C &= ~8;
    Bp(0xFE00C000);
    Bp(0);
    if (!(lbl_805FB50C & 2)) {
        lbl_805FB50C |= 2;
        if (lbl_805FB50C & 1) {
            StateTwoStages();
        } else {
            StateOneChannelOneTex();
        }
    }

    XfHead(0xB, 0x500);                 // position matrix 0: identity
    GXWGFifo._u32 = 0x3F800000; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0; GXWGFifo._u32 = 0x3F800000; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0x3F800000; GXWGFifo._u32 = 0;
    fn_80154290();
    Bp(0xFE000090);
    Bp(0x80000090);
    Bp(0xFE00000F);
    Bp(0x80000000);
}
