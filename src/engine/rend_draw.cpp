// Renderer: drawing from a vertex array of {f32 u, v; u32 colour; f32 x, y, z} (0x18 bytes),
// and the GX state for drawing a full-screen layer.
// Original: 0x80151644, 0x80152620, 0x80152D84, 0x801534F0 (auto_03_801467F8_text.s), 0x80159258 (auto_03_80156AAC_text.s)
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

extern "C" {
void fn_80151264(void);                 // 0x80151264: re-sends the fog registers
void fn_8014E3EC(s32 bytes);            // 0x8014E3EC: reserves vertex space at the end of the display list
extern u8  lbl_80400FF0[];              // 0x80400FF0: the renderer's matrix block (+0x10 GX position matrix, +0x40 projection)
extern s32 lbl_805FB4B8;                // 0x805FB4B8: projection type (1 = orthographic)
extern u8  lbl_805FB50E;                // 0x805FB50E
extern u8  lbl_805FB524;                // 0x805FB524: fog enabled
extern u8  lbl_805F9211;                // 0x805F9211
extern u8  lbl_805F9212;                // 0x805F9212
extern u8  lbl_805F9231;                // 0x805F9231: depth test on (shadow)
extern u8  lbl_805F9232;                // 0x805F9232: depth write on (shadow)
}

// 0x80152620
// Draws `count` vertices in order from an array of {u32 colour; f32 x, y, z} (0x10 bytes).
extern "C" void fn_80152620(s32 prim, s32 count, u8* verts) {
    fn_80142ED0(verts, count * 0x10);
    Cp(0x60, 0);
    XfHead(0, 0x1008); GXWGFifo._u32 = 1;           // 1 colour, no texture coordinate
    Cp(0xA0, (u32)(uintptr_t)(verts + 4) & 0x3FFFFFFF);   // lint:ok
    Cp(0xA2, (u32)(uintptr_t)verts & 0x3FFFFFFF);         // lint:ok
    Cp(0xB0, 0x10);
    Cp(0xB2, 0x10);
    if (count < 0xFF) {
        Cp(0x50, 0x4400);
        GXWGFifo._u8 = 0x48;
        EnsureDLRoom(count * 2 + 0x10C0);
        GXWGFifo._u8 = (u8)prim;
        GXWGFifo._u16 = (u16)count;
        for (s32 i = 0; i < count; i++) {
            GXWGFifo._u8 = (u8)i;
            GXWGFifo._u8 = (u8)i;
        }
    } else {
        Cp(0x50, 0x6600);
        GXWGFifo._u8 = 0x48;
        EnsureDLRoom(count * 4 + 0x10C0);
        GXWGFifo._u8 = (u8)prim;
        GXWGFifo._u16 = (u16)count;
        for (s32 i = 0; i < count; i++) {
            GXWGFifo._u16 = (u16)i;
            GXWGFifo._u16 = (u16)i;
        }
    }
}

static inline u32 Swap32(u32 v) {
    return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24);
}

// 0x801534F0
// Clears the frame buffer by drawing a quad over it: `flags` bit 0 = write colour and alpha,
// bit 2 = write depth; `rgb` is 0xBBGGRR, `alpha` the alpha byte, `depth` 0 (near) to 1 (far).
// Leaves the engine's usual state behind (projection and position matrix restored).
extern "C" void RendClear(s32 flags, s32 rgb, s32 alpha, f32 depth) {
    s32 w = VideoGetWidth(&gVideo);
    s32 h = VideoGetHeight(&gVideo);
    Bp(0xFE000018);
    Bp((flags & 1) ? 0x41000018 : 0x41000000);
    if (flags & 4) {
        if (lbl_805F9231 == 0) {
            lbl_805F9231 = 1;
            Bp(0xFE000011);
            Bp(((u32)lbl_805F9232 << 4) | 0x40000000 | 1);
        }
        lbl_805F9232 = 1;
        if (lbl_805F9231 != 0) {
            Bp(0xFE000011);
            Bp(0x40000011);
        }
    } else {
        if (lbl_805F9231 != 0) {
            lbl_805F9231 = 0;
            Bp(0xFE000011);
            Bp(0x40000000);
        }
        lbl_805F9232 = 0;
        if (lbl_805F9231 != 0) {
            Bp(0xFE000011);
            Bp(0x40000001);
        }
    }
    Bp(0xFE00000E);
    Bp(0x4000000E);
    if (lbl_805FB50C & 4) {
        lbl_805FB50C &= ~4;
        lbl_805FB514 |= 0x40;
        Bp((u32)lbl_805FB514 | 0x43000000);
        Bp(0xF33F0000);
    }
    Bp(0xFE000001);
    Bp(0x41000000);
    lbl_805FB50C &= ~8;
    Bp(0xFE00C000);
    Bp(0);
    if (lbl_805FB524 != 0) {
        lbl_805FB524 = 0;
        fn_80151264();
    }
    if (lbl_805FB50C & 2) {
        u32 two = lbl_805FB50C & 1;
        lbl_805FB50C &= ~2;
        if (two) {
            Bp(0xFE00000F); Bp(0);
            XfHead(0, 0x103F); GXWGFifo._u32 = 0;
            Bp(0xFE003C00); Bp(0);
            XfHead(0, 0x1010); GXWGFifo._u32 = 0x441;
            Bp(0x28380000);
            Bp(0xC008F2A4);
            Bp(0xC108E6F0);
        } else {
            StateOneChannelNoTex();
        }
    }

    f32 colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    EnsureDLRoom(0x1107);
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

    XfHead(6, 0x1020);                  // projection: x, y, z straight through
    GXWGFifo._f32 = 1.0f; GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 1.0f; GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 1.0f; GXWGFifo._f32 = 0.0f;
    GXWGFifo._u32 = 1;
    XfHead(0xB, 0);                     // position matrix 0: identity
    GXWGFifo._f32 = 1.0f; GXWGFifo._f32 = 0.0f; GXWGFifo._f32 = 0.0f; GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f; GXWGFifo._f32 = 1.0f; GXWGFifo._f32 = 0.0f; GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f; GXWGFifo._f32 = 0.0f; GXWGFifo._f32 = 1.0f; GXWGFifo._f32 = 0.0f;

    s32 fullW = VideoGetWidth(&gVideo);
    s32 fullH = VideoGetHeight(&gVideo);
    u32 c = (u32)rgb | ((u32)alpha << 24);
    f32 x0 = 2.0f * ((f32)0 / (f32)fullW) - 1.0f;
    f32 x1 = 2.0f * ((f32)w / (f32)fullW) - 1.0f;
    f32 y0 = 1.0f + -2.0f * ((f32)0 / (f32)fullH);
    f32 y1 = 1.0f + -2.0f * ((f32)h / (f32)fullH);
    f32 z = 0.99998474f * depth - 1.0f;
    if (lbl_805FB4CC < 0x40) {
        fn_8014E3EC(0x40);
    }
    u8* v = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB4C4 += 0x40;
    lbl_805FB4CC -= 0x40;
    // The original reverses the colour's bytes in memory; as a value that is the same swap.
    const f32 xs[4] = {x0, x1, x0, x1};
    const f32 ys[4] = {y0, y0, y1, y1};
    for (s32 i = 0; i < 4; i++) {
        *(u32*)(v + i * 0x10) = Swap32(c);
        *(f32*)(v + i * 0x10 + 4) = xs[i];
        *(f32*)(v + i * 0x10 + 8) = ys[i];
        *(f32*)(v + i * 0x10 + 0xC) = z;
    }
    fn_80152620(0x98, 4, v);

    const f32* proj = (const f32*)(lbl_80400FF0 + 0x40);
    XfHead(6, 0x1020);
    for (s32 i = 0; i < 6; i++) GXWGFifo._f32 = proj[i];
    GXWGFifo._u32 = (u32)lbl_805FB4B8;
    const f32* pos = (const f32*)(lbl_80400FF0 + 0x10);
    XfHead(0xB, 0);
    for (s32 i = 0; i < 12; i++) GXWGFifo._f32 = pos[i];

    Bp(0xFE00000E);
    Bp(0x40000006);
    if (!(lbl_805FB50C & 4)) {
        lbl_805FB50C |= 4;
        lbl_805FB514 &= ~0x40;
        Bp((u32)lbl_805FB514 | 0x43000000);
        Bp((u32)lbl_805FB50E | 0xF3380000 | ((u32)lbl_805F9211 << 16));
    }
    if (lbl_805F9231 == 0) {
        lbl_805F9231 = 1;
        Bp(0xFE000011);
        Bp(((u32)lbl_805F9232 << 4) | 0x40000000 | 1);
    }
    Bp(0xFE000001);
    Bp(0x41000001);
    lbl_805FB50C |= 8;
    Bp(0xFE00C000);
    Bp((u32)lbl_805F9212 << 14);
    Bp(0xFE000018);
    Bp(0x41000018);
}
