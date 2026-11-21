// Start of a frame on the game thread: waits for the render thread, opens the next display
// list and writes the frame's initial GX state into it.
// Original: 0x8014F0A0, 0x80150148 (auto_03_801467F8_text.s)
#include "rend_dl_inline.h"

// The copy-filter registers and the full-screen viewport and scissor, clamped to the frame
// buffer. (Inlined in RendBeginFrame and fn_80150148.)
static inline void SetFilterAndViewport(void) {
    const u8* f = lbl_803488F0;
    Bp(((u32)lbl_805FB504 << 2) | 0x41000000 | 0x4B9);
    Bp(0x11);
    Bp(f[0] | (f[1] << 4) | (f[2] << 8) | (f[3] << 12) | (f[4] << 16) | (f[5] << 20) | 0x01000000);
    Bp(f[6] | (f[7] << 4) | (f[8] << 8) | (f[9] << 12) | (f[10] << 16) | (f[11] << 20) | 0x02000000);
    Bp(f[12] | (f[13] << 4) | (f[14] << 8) | (f[15] << 12) | (f[16] << 16) | (f[17] << 20) | 0x03000000);
    Bp(f[18] | (f[19] << 4) | (f[20] << 8) | (f[21] << 12) | (f[22] << 16) | (f[23] << 20) | 0x04000000);
    Bp(0xFEFFFF00);
    Bp(0x0F000000);
    Bp(0xFEFFFF00);
    Bp(0x0F000000);
    lbl_805FB514 = lbl_805FB500 | 0x40;
    Bp((u32)lbl_805FB514 | 0x43000000);
    lbl_805FB50C &= ~4;

    // Full-screen viewport and scissor, clamped to the frame buffer.
    s32 h = VideoGetHeight(&gVideo);
    s32 w = VideoGetWidth(&gVideo);
    s32 maxW = VideoGetWidth(&gVideo);
    s32 maxH = VideoGetHeight(&gVideo);
    s32 vw = 1;
    if (w >= 1) vw = w;
    s32 vh = 1;
    if (h >= 1) vh = h;
    if (vw > maxW) vw = maxW;
    if (vh > maxH) vh = maxH;
    s32 x = (vw - 1) & ((vw - 1) >> 31);    // min(vw - 1, 0): 0 here
    s32 y = (vh - 1) & ((vh - 1) >> 31);
    lbl_805F9234 = vw - x;
    lbl_805F9238 = vh - y;
    f32 sx = 0.5f * (f32)(vw - x);
    f32 sy = -0.5f * (f32)(vh - y);
    f32 ox = 342.0f + 0.5f * (f32)(vw + x);
    f32 oy = 342.0f + 0.5f * (f32)(y + vh);
    XfHead(5, 0x101A);
    GXWGFifo._f32 = sx;
    GXWGFifo._f32 = sy;
    GXWGFifo._f32 = 16777216.0f;
    GXWGFifo._f32 = ox;
    GXWGFifo._f32 = oy;
    GXWGFifo._f32 = 16777216.0f;
    Bp((((u32)(x + 0x156) & 0xFFFF) << 12) | ((u32)(y + 0x156) & 0xFFFF) | 0x20000000);
    Bp((((u32)(vw + 0x155) & 0xFFFF) << 12) | ((u32)(vh + 0x155) & 0xFFFF) | 0x21000000);
    Bp(0x5902ACAB);
}

// The engine's default GX state and the shadow flags that go with it.
static inline void DefaultStateTail(void) {
    RendSetDefaultState();
    lbl_805FB50C = 0;
    lbl_805FB50D = 0;
    Bp(0xFE00000F);
    Bp(0);
    XfHead(0, 0x103F); GXWGFifo._u32 = 0;
    Bp(0xFE003C00);
    Bp(0);
    XfHead(0, 0x100E); GXWGFifo._u32 = 0x442;
    XfHead(0, 0x1010); GXWGFifo._u32 = 0x442;
    Bp(0x28380000);
    Bp(0xC008FFFA);
    Bp(0xC108FFD0);
    Bp(0xFE00C000);
    Bp(0);
    lbl_805FB514 |= 0x40;
    Bp((u32)lbl_805FB514 | 0x43000000);
    Bp(0xF33F0000);
    fn_8015B7D4();
}

// 0x8014F0A0
extern "C" void RendBeginFrame(void) {
    lbl_805FB51C = 1;
    fn_8015E9D8(lbl_805FB594, lbl_805FB518, 2);
    // The original leaves r4-r7 as they are for this call; the queued command has no arguments.
    fn_8015E7C8(lbl_805FB594, 0, 0, 0, 0);

    OpenNextDL();

    GXWGFifo._u8 = 0x48;                // invalidate the vertex cache
    Bp(0xFEFFFF00);
    Bp(0x0F000000);
    Bp(0x66001000);
    Bp(0x66001100);
    Bp(0xFEFFFF00);
    Bp(0x0F000000);
    Cp(0x70, 0x41377009);               // vertex format 0, groups A-C
    Cp(0x80, 0xC8241209);
    Cp(0x90, 0x04824120);
    XfHead(0, 0x1040); GXWGFifo._u32 = 0x5280;
    XfHead(0, 0x1050); GXWGFifo._u32 = 0;
    Cp(0x30, 0x3CF3CF00);               // matrix indices A, B
    Cp(0x40, 0x00F3CF3C);
    XfHead(1, 0x1018); GXWGFifo._u32 = 0x3CF3CF00; GXWGFifo._u32 = 0x00F3CF3C;
    XfHead(0, 0x1009); GXWGFifo._u32 = 1;
    XfHead(0, 0x103F); GXWGFifo._u32 = 1;

    s32 wanted = 0;
    if (lbl_805FB4F0 != NULL) {
        wanted = lbl_805FB4F0();
    }

    if (lbl_805F9214 != 0 && wanted != 0) {
        lbl_805FB520 = 3;
        fn_8014ED64(1);
        CloseDL();
        s32 next = lbl_805FB4BC + 1;
        lbl_805FB4BC = next;
        if (next >= 0x11) {
            next = 0;
            lbl_805FB4BC = 0;
        }
        lbl_805FB4E0 = 1;
        lbl_805FB4E4 = next;
        lbl_805FB4E8 = 0;
        lbl_805FB4EC = 0x11;
        OpenNextDL();
    } else {
        lbl_805FB520 = 1;
        SetFilterAndViewport();
    }

    EnsureDLRoom(0x15BA);

    DefaultStateTail();
}

// 0x80150148
// Applies a pending render-mode change (lbl_805FB520 != 0): the same registers RendBeginFrame
// sets at the start of a normal frame. Mode 3 also ends the "kept display lists" mode.
extern "C" void fn_80150148(void) {
    if (lbl_805FB520 != 0) {
        if (lbl_805FB520 == 3) {
            lbl_805FB4E0 = 0;
        }
        SetFilterAndViewport();
        EnsureDLRoom(0x15BA);
        DefaultStateTail();
        lbl_805FB520 = 0;
    }
}
