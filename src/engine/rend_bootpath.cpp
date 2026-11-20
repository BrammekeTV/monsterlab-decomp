// Renderer functions the boot path needs before the first frame: the orthographic projection
// and the image-header set-up used for render targets.
// Original: 0x801499F8, 0x801566E8 (auto_03_801467F8_text.s, auto_03_80156AAC_text.s)
#include "types.h"
#include "revolution.h"

extern "C" {
void fn_80151264(void);                 // 0x80151264: re-sends the fog registers
void fn_8015A5A4(u8* p, s32 a);         // 0x8015A5A4: derives sizes of an image header and allocates its data

extern f32 lbl_80401030[6];             // 0x80401030: projection as GX takes it (6 values)
extern f32 lbl_805F9220;                // 0x805F9220: projection near
extern f32 lbl_805F9224;                // 0x805F9224: projection far
extern u8  lbl_805F9230;                // 0x805F9230: cleared when the projection changes
extern s32 lbl_805FB4B8;                // 0x805FB4B8: projection type (1 = orthographic)
extern u8  lbl_805FB524;                // 0x805FB524: fog enabled
}

// 0x801499F8
// Fills in an image header: size, pixel format, bits per pixel, and for palette formats the
// palette size and its entry width, then lets fn_8015A5A4 allocate the pixels.
extern "C" s32 fn_801499F8(u8* target, s32 w, s32 h, s32 fmt, s32 a, s32 b, const char* name) {
    target[0xB] = 0;
    target[8] = (u8)b;
    target[9] = (u8)fmt;

    u8 bitsPerPixel;
    switch ((u32)fmt) {
    case 4: case 8: case 11: case 14: case 15:
        bitsPerPixel = 4;
        break;
    case 5: case 9: case 10: case 12: case 13: case 16: case 17:
        bitsPerPixel = 8;
        break;
    case 0: case 1: case 2: case 18: case 19: case 20: case 21: case 23:
        bitsPerPixel = 16;
        break;
    case 3: case 22: case 24:
        bitsPerPixel = 32;
        break;
    default:   // 6, 7 and anything above 24
        bitsPerPixel = 0;
        break;
    }
    target[0xA] = bitsPerPixel;
    target[0xC] = (u8)a;

    s32 paletteEntries;
    switch (fmt) {
    case 4:   paletteEntries = 0x10;  break;
    case 5:   paletteEntries = 0x100; break;
    case 11:  paletteEntries = 0x10;  break;
    case 12:  paletteEntries = 0x20;  break;
    case 13:  paletteEntries = 8;     break;
    default:  paletteEntries = 0;     break;
    }
    target[0xD] = (u8)(paletteEntries >> 3);

    u8 paletteBits = 0;
    if (((paletteEntries >> 3) & 0xFF) != 0) {
        if (a == 3)                paletteBits = 0x20;
        else if (a >= 0 && a < 9)  paletteBits = 0x10;
        else                       paletteBits = 0;
    }
    *(u16*)(target + 0) = (u16)w;
    target[0xE] = paletteBits;
    *(u16*)(target + 2) = (u16)h;

    fn_8015A5A4(target, (s32)(uintptr_t)name);   // lint:ok 32-bit build
    target[0xB] |= 1;
    return 0;   // the original returns nothing useful (r3 is left over from the call)
}

// 0x801566E8
// Orthographic projection: stores it and sends it to GX (XF registers 0x1020-0x1026).
extern "C" void fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    f32 farZ = f * 1.0000153f;
    lbl_805F9230 = 0;
    lbl_805F9220 = n;
    lbl_805F9224 = farZ;
    lbl_805FB4B8 = 1;
    lbl_80401030[0] = 2.0f / (r - l);
    lbl_80401030[2] = 2.0f / (t - b);
    lbl_80401030[1] = -(r + l) / (r - l);
    lbl_80401030[3] = -(t + b) / (t - b);
    lbl_80401030[4] = 1.0f / (n - farZ);
    lbl_80401030[5] = farZ / (n - farZ);

    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 6;
    GXWGFifo._u16 = 0x1020;
    for (int i = 0; i < 6; i++) GXWGFifo._f32 = lbl_80401030[i];
    GXWGFifo._u32 = 1;

    if (lbl_805FB524) fn_80151264();
}
