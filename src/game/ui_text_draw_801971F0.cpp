// UI font text drawing: the line-by-line layout and draw loop behind the text widget (reached from
// fn_8019578C / fn_8019D5E8).
#include <string.h>
#include "types.h"
#include "revolution.h"

extern "C" {
void  fn_8018979C(u8* font, const s8* str, s32* w, s32* h);                      // 0x8018979C (measures text)
void  fn_80189EFC(u8* font, const s8* str, u32* colors, u32 count,
                  f32 x, f32 y, f32 sx, f32 sy);                                 // 0x80189EFC (draws glyphs)
void  fn_8018D2B8(const char* fmt, ...);                                         // 0x8018D2B8 (debug log)
void  fn_8018D370(const char* fmt, ...);                                         // 0x8018D370 (debug log)
void* fn_8019AEAC(u8* arg0, u32 arg1, s32 arg2);                                 // 0x8019AEAC
}

extern "C++" void* gScreenSets;     // 0x805FB654
extern "C" {
extern u8         lbl_805FB50C;     // 0x805FB50C (GX shadow flags byte)
extern u8         lbl_805FB668;     // 0x805FB668 (set once lbl_80413390 has been zeroed)
extern f32        lbl_80413390[12]; // 0x80413390 (four xyz corners of the last measured text box)
extern const char lbl_80382208[];   // 0x80382208 (string block: +0x0 unknown alignment, +0x2C font hash miss, +0x68 no screen set)
extern const char lbl_805F93B8[];   // 0x805F93B8 (" ")
}

static void BP(u32 v) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = v;
}

static void Cmd10(u16 a, u16 b, u32 v) {
    GXWGFifo._u8  = 0x10;
    GXWGFifo._u16 = a;
    GXWGFifo._u16 = b;
    GXWGFifo._u32 = v;
}

extern "C" {

// 0x801971F0
// Lays out and draws the text str (with obj+8 as the current font) line by line. The string is cut
// at the control bytes 1..5 and 10: 1 skips itself, 2 + hash switches font (found through
// gScreenSets), 3 + hash picks the horizontal alignment, 4 counts a leading space, 5 moves down half
// a font height and 10 starts a new line. Each segment is measured with fn_8018979C, positioned
// by the alignment (0..9; boxW is the box width for styles 4..9, mode picks the default style,
// vmode the vertical offset) and drawn with fn_80189EFC. colors/count are advanced by the number
// of bytes consumed. obj+8 is restored at the end. Returns lbl_80413390, the last box corners.
f32* fn_801971F0(u8* obj, s8* str, u32 boxW, u32* colors, u32 count, s32 mode, s32 vmode, f32 scale) {
    const f32 zero = 0.0f;  // lbl_805FF8F8
    u8* font;
    u8* savedFont;
    s32 style;
    s32 styleOrig;
    s32 spaceW = 0;
    s32 w;
    s32 h;
    s32 spaces = 0;
    s32 lineStart = 1;
    s32 cellH;
    f32 maxW = zero, minX = zero, lineX = zero, spaceX = zero, boxY = zero, vOff = zero;
    s8 delims[8];
    s8* seg;
    s8* hit;
    s32 delim;
    s32 segLen = 0;
    s32 i;

    if ((s8)lbl_805FB668 == 0) {
        for (i = 0; i < 12; i++) {
            lbl_80413390[i] = zero;
        }
        lbl_805FB668 = 1;
    }

    savedFont = *(u8**)(obj + 0x8);
    font = savedFont;
    if (mode == 1) {
        style = 2;
    } else if (mode > 1 && mode < 3) {
        style = 0;
    } else {
        style = 1;
    }
    styleOrig = style;

    if ((lbl_805FB50C & 2) == 0) {
        lbl_805FB50C |= 2;
        if (lbl_805FB50C & 1) {
            BP(0xFE00000Fu);
            BP(1);
            Cmd10(0, 0x103F, 1);
            BP(0xFE003C00u);
            BP(0x400);
            Cmd10(0, 0x1010, 0x441);
            BP(0x283C0000u);
            BP(0xC008F2A4u);
            BP(0xC108E6F0u);
            BP(0xC208F80Fu);
            BP(0xC308F070u);
        } else {
            BP(0xFE00000Fu);
            BP(1);
            Cmd10(0, 0x103F, 1);
            BP(0xFE003C00u);
            BP(0);
            Cmd10(0, 0x100E, 0x442);
            Cmd10(0, 0x1010, 0x442);
            BP(0x28380040u);
            BP(0xC008F8AFu);
            BP(0xC108F2F0u);
        }
    }

    delims[0] = 2;
    delims[1] = 3;
    delims[2] = 1;
    delims[3] = 4;
    delims[4] = 5;
    delims[5] = 10;
    delims[6] = 0;
    delims[7] = 0;

    cellH = (s8)font[2];
    seg = str;
    while (seg != NULL) {
        // Find the first control byte of the segment.
        delim = 0;
        segLen = 0;
        hit = NULL;
        for (i = 0; seg[i] != 0 && hit == NULL; i++) {
            const s8* d;
            for (d = delims; *d != 0; d++) {
                if (seg[i] == *d) {
                    hit = seg + i;
                    break;
                }
            }
        }
        if (hit != NULL) {
            delim = *hit;
        }

        if (hit == NULL || hit > seg) {
            f32 w25;

            if (hit != NULL) {
                *hit = 0;
            }
            if (spaces != 0) {
                fn_8018979C(font, (const s8*)lbl_805F93B8, &spaceW, &h);
            }
            fn_8018979C(font, seg, &w, &h);
            if (spaces != 0) {
                spaceX = (f32)(spaces * (spaceW << 3));
            }
            w25 = (f32)w;
            if (vmode == 1) {
                vOff = 0.5f * (f32)(-cellH);
            } else if (vmode > 1 && vmode < 3) {
                vOff = (f32)(-cellH);
            } else {
                vOff = zero;
            }

            if (lineStart != 0) {
                lineStart = 0;
                if (spaces != 0) {
                    spaceX = zero;
                    w25 = w25 + (f32)(spaces * (spaceW << 3));
                }
                switch ((u32)style) {
                case 0: lineX = -w25; break;
                case 1: lineX = (f32)(spaces * (spaceW << 3)); break;
                case 2: lineX = 0.5f * -w25; break;
                case 3: lineX = (f32)(spaces * (spaceW << 3)); break;
                case 4: lineX = 0.25f * (f32)boxW - 0.5f * w25; break;
                case 5: lineX = 0.5f * (f32)boxW - w25; break;
                case 6: lineX = 0.5f * (f32)boxW - 0.5f * w25; break;
                case 7: lineX = 0.5f * (f32)boxW; break;
                case 8: lineX = 0.75f * (f32)boxW - 0.5f * w25; break;
                case 9: lineX = (f32)boxW - w25; break;
                default:
                    fn_8018D2B8(lbl_80382208, styleOrig);
                    break;
                }
            }

            {
                f32 fh = (f32)(s32)(s8)font[2];
                f32 wN = (f32)w * scale / fh;
                f32 xN = lineX / fh;
                f32 hN = (f32)h * scale / fh;
                f32 yN = boxY / fh;
                f32 sN = spaceX / fh;
                f32 vN = vOff / fh;
                f32 c0, c1, c2;

                if (maxW < wN) {
                    maxW = wN;
                }
                if (minX > xN) {
                    minX = xN;
                }
                c0 = minX + sN;
                c1 = maxW + c0;
                c2 = hN + (yN + vN);
                lbl_80413390[0] = c0;  lbl_80413390[1] = vN;  lbl_80413390[2] = zero;
                lbl_80413390[3] = c1;  lbl_80413390[4] = vN;  lbl_80413390[5] = zero;
                lbl_80413390[6] = c0;  lbl_80413390[7] = c2;  lbl_80413390[8] = zero;
                lbl_80413390[9] = c1;  lbl_80413390[10] = c2; lbl_80413390[11] = zero;
                fn_80189EFC(font, seg, colors, count, lineX + spaceX, boxY + vOff, scale, scale);
            }

            if ((u32)style > 9) {
                fn_8018D2B8(lbl_80382208, styleOrig);
            } else if (style == 0) {
                lineX = lineX - (w25 + spaceX);
            } else {
                lineX = lineX + (w25 + spaceX);
            }
            spaceX = zero;
            spaces = 0;
            segLen = (s32)strlen((const char*)seg);
            if (hit != NULL) {
                *hit = (s8)delim;
            }
        }

        if (hit == NULL) {
            seg = NULL;
        } else {
            switch ((u32)delim) {
            case 10:
                seg = hit + 1;
                count -= segLen + 1;
                colors += segLen + 1;
                lineStart = 1;
                boxY += (f32)cellH;
                break;
            case 5:
                seg = hit + 1;
                count -= segLen + 1;
                colors += segLen + 1;
                lineStart = 1;
                boxY += (f32)(cellH / 2);
                break;
            case 4:
                seg = hit + 1;
                colors += segLen + 1;
                count -= segLen + 1;
                spaces++;
                break;
            case 2: {
                u8* p = *(u8**)((u8*)gScreenSets + 0x84);
                if (p != NULL && *(u8**)(p + 0x2C) != NULL) {
                    u32 hash = *(u32*)(hit + 1);
                    u8* r = (u8*)fn_8019AEAC(*(u8**)(p + 0x2C), hash, 1);
                    if (r != NULL && *(u8**)r != NULL) {
                        font = *(u8**)(*(u8**)r + 0x8);
                    } else {
                        fn_8018D2B8(lbl_80382208 + 0x2C, hash);
                        font = *(u8**)(obj + 0x8);
                    }
                    cellH = (s8)font[2];
                } else {
                    fn_8018D370(lbl_80382208 + 0x68);
                }
                seg = hit + 5;
                colors += segLen + 5;
                count -= segLen + 5;
                break;
            }
            case 3: {
                u32 hash = *(u32*)(hit + 1);
                switch (hash) {
                case 0x4CF20ADCu: style = 3; break;
                case 0xCAAB42DFu: style = 4; break;
                case 0x8AAA622Fu: style = 5; break;
                case 0xB0082CA2u: style = 6; break;
                case 0x4EFDDBA5u: style = 7; break;
                case 0xBF210E50u: style = 8; break;
                case 0x439EB7A4u: style = 9; break;
                default: break;
                }
                seg = hit + 5;
                lineStart = 1;
                colors += segLen + 5;
                count -= segLen + 5;
                break;
            }
            default:
                seg = hit + 1;
                colors += segLen + 1;
                count -= segLen + 1;
                break;
            }
        }
    }

    *(u8**)(obj + 0x8) = savedFont;
    return lbl_80413390;
}

}  // extern "C"
