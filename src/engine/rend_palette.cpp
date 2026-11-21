// Palette conversion: any supported colour format -> 32-bit (A<<24 | B<<16 | G<<8 | R) -> any
// supported colour format. Used when a paletted image gets its palette.
// Original: 0x80149D88, 0x80149E1C, 0x80149E98 (auto_03_801467F8_text.s)
//
// Format numbers as the two switches of fn_80149E98 use them (16 bits unless noted):
//   0  B5 G6 R5         (blue in the top bits)
//   1  A1 B5 G5 R5
//   2  A4 B4 G4 R4
//   3  32-bit, copied
//   4  A8 L8            (to: luminance from R, G, B; from: L to all three)
//   5  R5 G6 B5         (red in the top bits: GX RGB565)
//   6  GX RGB5A3        (top bit set: R5 G5 B5 opaque, else A3 R4 G4 B4)
//   7  x R5 G5 B5 opaque (to: top bit set)
//   8  as 7; to: entry 0 is then made 0 (transparent)
// The original unrolls every loop 2, 4 or 8 times; the loops here are the plain form.
#include "types.h"
#include "engine/engine.h"
#include "engine/rend.h"

static inline u32 Exp5(u32 v) { return ((v << 3) | (v >> 2)) & 0xFF; }
static inline u32 Exp6(u32 v) { return ((v << 2) | (v >> 4)) & 0xFF; }
static inline u32 Exp4(u32 v) { return ((v << 4) | v) & 0xFF; }
static inline u32 Pack(u32 a, u32 hi, u32 mid, u32 lo) {
    return (a << 24) | (hi << 16) | (mid << 8) | lo;
}

// 0x80149D88
// GX RGB5A3 -> 32-bit. The third argument is not used.
extern "C" void fn_80149D88(u32* dst, const u16* src, s32 unused, s32 count) {
    (void)unused;
    for (; count > 0; count--) {
        u32 c = *src++;
        u32 a, r, g, b;
        if (c >> 15) {
            b = Exp5(c & 0x1F);
            g = Exp5((c >> 5) & 0x1F);
            r = Exp5((c >> 10) & 0x1F);
            a = 0xFF;
        } else {
            u32 a3 = (c >> 12) & 7;
            b = Exp4(c & 0xF);
            g = Exp4((c >> 4) & 0xF);
            r = Exp4((c >> 8) & 0xF);
            a = (a3 << 5) | (a3 << 2) | (a3 >> 1);
        }
        *dst++ = Pack(a, b, g, r);
    }
}

// 0x80149E1C
// 32-bit -> GX RGB5A3: opaque when the top three alpha bits are all set. The third argument is
// not used.
extern "C" void fn_80149E1C(u16* dst, const u32* src, s32 unused, s32 count) {
    (void)unused;
    for (; count > 0; count--) {
        u32 c = *src++;
        u32 a3 = c >> 29;
        u32 r = c & 0xFF;
        u32 g = (c >> 8) & 0xFF;
        u32 b = (c >> 16) & 0xFF;
        if (a3 == 7) {
            *dst++ = (u16)(0x8000 | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
        } else {
            *dst++ = (u16)((a3 << 12) | ((r >> 4) << 8) | (g & 0xF0) | (b >> 4));
        }
    }
}

// 0x80149E98
// Converts `count` palette entries from srcFmt to dstFmt through a 0x400-byte temporary
// (256 entries of 32 bits) from the renderer's temporary allocator.
extern "C" void fn_80149E98(void* dst, s32 dstFmt, const void* src, s32 srcFmt, s32 count) {
    u32* tmp = (u32*)gRendMem->allocTemp(0x400);
    const u16* s16 = (const u16*)src;
    s32 i;

    switch ((u32)srcFmt) {
    case 0:
        for (i = 0; i < count; i++) {
            u32 c = s16[i];
            tmp[i] = Pack(0xFF, Exp5((c >> 11) & 0x1F), Exp6((c >> 5) & 0x3F), Exp5(c & 0x1F));
        }
        break;
    case 1:
        for (i = 0; i < count; i++) {
            u32 c = s16[i];
            tmp[i] = Pack((c >> 15) ? 0xFF : 0, Exp5((c >> 10) & 0x1F), Exp5((c >> 5) & 0x1F),
                          Exp5(c & 0x1F));
        }
        break;
    case 2:
        for (i = 0; i < count; i++) {
            u32 c = s16[i];
            tmp[i] = Pack(Exp4(c >> 12), Exp4((c >> 8) & 0xF), Exp4((c >> 4) & 0xF), Exp4(c & 0xF));
        }
        break;
    case 3:
        for (i = 0; i < count; i++) {
            tmp[i] = ((const u32*)src)[i];
        }
        break;
    case 4:
        for (i = 0; i < count; i++) {
            u32 c = s16[i];
            u32 l = c & 0xFF;
            tmp[i] = Pack(c >> 8, l, l, l);
        }
        break;
    case 5:
        for (i = 0; i < count; i++) {
            u32 c = s16[i];
            tmp[i] = Pack(0xFF, Exp5(c & 0x1F), Exp6((c >> 5) & 0x3F), Exp5((c >> 11) & 0x1F));
        }
        break;
    case 6:
        fn_80149D88(tmp, s16, 0, count);
        break;
    case 7:
    case 8:
        for (i = 0; i < count; i++) {
            u32 c = s16[i];
            tmp[i] = Pack(0xFF, Exp5((c >> 10) & 0x1F), Exp5((c >> 5) & 0x1F), Exp5(c & 0x1F));
        }
        break;
    default:
        break;
    }

    u16* d16 = (u16*)dst;
    switch ((u32)dstFmt) {
    case 0:
        for (i = 0; i < count; i++) {
            u32 c = tmp[i];
            d16[i] = (u16)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x1F));
        }
        break;
    case 1:
        for (i = 0; i < count; i++) {
            u32 c = tmp[i];
            d16[i] = (u16)(((c >> 16) & 0x8000) | ((c >> 9) & 0x7C00) | ((c >> 6) & 0x03E0) |
                           ((c >> 3) & 0x1F));
        }
        break;
    case 2:
        for (i = 0; i < count; i++) {
            u32 c = tmp[i];
            d16[i] = (u16)(((c >> 16) & 0xF000) | ((c >> 12) & 0x0F00) | ((c >> 8) & 0xF0) |
                           ((c >> 4) & 0xF));
        }
        break;
    case 3:
        for (i = 0; i < count; i++) {
            ((u32*)dst)[i] = tmp[i];
        }
        break;
    case 4:
        for (i = 0; i < count; i++) {
            u32 c = tmp[i];
            u32 lum = (((c >> 16) & 0xFF) * 0x1C + (c & 0xFF) * 0x4D + ((c >> 8) & 0xFF) * 0x97) >> 8;
            d16[i] = (u16)(((c >> 16) & 0xFF00) | lum);
        }
        break;
    case 5:
        for (i = 0; i < count; i++) {
            u32 c = tmp[i];
            d16[i] = (u16)(((c << 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 19) & 0x1F));
        }
        break;
    case 6:
        fn_80149E1C(d16, tmp, 0, count);
        break;
    case 7:
    case 8:
        for (i = 0; i < count; i++) {
            u32 c = tmp[i];
            d16[i] = (u16)(0x8000 | ((c >> 9) & 0x7C00) | ((c >> 6) & 0x03E0) | ((c >> 3) & 0x1F));
        }
        if (dstFmt == 8) {
            d16[0] = 0;
        }
        break;
    default:
        break;
    }

    gRendMem->freeAny(tmp);
}
