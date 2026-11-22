// Text layout and draw: lays out a string with inline control codes and draws it segment by segment.
#include <string.h>
#include "types.h"
#include "revolution.h"

extern "C" {
void  fn_8018979C(u8* font, s8* str, s32* w, s32* h);                                   // 0x8018979C
void  fn_80189EFC(u8* font, f32 x, f32 y, f32 sx, f32 sy, const u8* str, const u32* colors,
                  u32 count);                                                            // 0x80189EFC
void* fn_8019AEAC(u8* arg0, u32 arg1, s32 arg2);                                         // 0x8019AEAC
void  fn_8018D2B8(const char* fmt, ...);                                                 // 0x8018D2B8 (debug log)
void  fn_8018D370(const char* fmt, ...);                                                 // 0x8018D370 (debug log)

extern const char lbl_80382208[];    // 0x80382208 (log strings)
extern f32        lbl_80413390[12];  // 0x80413390 (last quad: 4 vertices xyz)
extern s8         lbl_805FB668;      // 0x805FB668 (quad zeroed)
extern u8         lbl_805FB50C;      // 0x805FB50C (GX shadow flags byte)
}

extern "C++" void* gScreenSets;  // 0x805FB654

static const u32 kHashLeft   = 0x4CF20ADCu;  // "left"
static const u32 kHashCenter = 0xB0082CA2u;  // "Center"
static const u32 kHashRight  = 0x439EB7A4u;  // "right"

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

// Switches the line draw state on (flag bit 0 selects one of two register sets).
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

extern "C" {

// 0x801971F0
// Draws `text` with the font at ctx+8, using scale, a box width, per-character colours and a
// remaining-character count. Control codes: 1 = skip, 2 = switch font by hash (4 bytes follow),
// 3 = set alignment by hash (left/Center/right, 4 bytes follow), 4 = add a space of padding,
// 5 = half line down, 10 = newline. `mode` picks the initial alignment, `vmode` the vertical
// offset. Writes the bounding quad of the last segment into lbl_80413390 and returns it.
// The scale is the last parameter here, as in the function-pointer type its caller uses
// (FnSpawn in draft_miss664.cpp: fn_8019578C calls this through slot 0x8 of the font object).
f32* fn_801971F0(u8* ctx, u8* text, u32 width, u32* colors, s32 count, s32 mode, s32 vmode,
                 f32 scale) {
    if (lbl_805FB668 == 0) {
        for (int i = 0; i < 12; i++) lbl_80413390[i] = 0.0f;
        lbl_805FB668 = 1;
    }

    u8* font = *(u8**)(ctx + 8);
    u8* origFont = font;
    s32 sp8 = 0;
    s32 sp10, spC;
    s32 first = 1;
    s32 spaces = 0;

    s32 align;
    if (mode == 1) align = 2;
    else if (mode >= 1 && mode < 3) align = 0;
    else align = 1;
    s32 origAlign = align;

    s8 ctrl[8] = {2, 3, 1, 4, 5, 10, 0, 0};

    if (!(lbl_805FB50C & 2)) {
        lbl_805FB50C = lbl_805FB50C | 2;
        GxLineStateOn((lbl_805FB50C & 1) != 0);
    }

    f32 lineY = 0.0f;  // f22: accumulated line offset
    f32 lead = 0.0f;   // f24: alignment offset
    f32 pad = 0.0f;    // f23: padding from spaces
    f32 minX = 0.0f;   // f20
    f32 maxW = 0.0f;   // f19
    f32 zero = 0.0f;   // f31
    f32 vOff = 0.0f;   // f21
    f32 segW = 0.0f;   // f25
    s32 lineH = (s8)font[2];
    u8* cur = text;

    while (cur != NULL) {
        s32 ch = 0;
        s32 segLen = 0;
        u8* hit;
        s32 i = 0;
        for (;;) {
            hit = cur + i;
            if ((s8)*hit == 0) {
                hit = NULL;
                break;
            }
            bool found = false;
            for (s8* p = ctrl; *p != 0; p++) {
                if (*p == (s8)*hit) {
                    found = true;
                    break;
                }
            }
            if (found) break;
            i++;
        }
        if (hit != NULL) ch = (s8)*hit;

        if (hit == NULL || hit - cur > 0) {
            if (hit != NULL) *hit = 0;
            if (spaces != 0) fn_8018979C(font, (s8*)" ", &sp8, &spC);
            fn_8018979C(font, (s8*)cur, &sp10, &spC);
            if (spaces != 0) pad = (f32)(spaces * (sp8 << 3));
            segW = (f32)sp10;
            if (vmode == 1) vOff = 0.5f * (f32)(-lineH);
            else if (vmode >= 1 && vmode < 3) vOff = (f32)(-lineH);
            else vOff = 0.0f;

            if (first != 0) {
                first = 0;
                if (spaces != 0) {
                    pad = 0.0f;
                    segW = segW + (f32)(spaces * (sp8 << 3));
                }
                switch (align) {
                    case 0: lead = -segW; break;
                    case 1:
                    case 3: lead = (f32)(spaces * (sp8 << 3)); break;
                    case 2: lead = 0.5f * -segW; break;
                    case 4: lead = 0.25f * (f32)width - 0.5f * segW; break;
                    case 5: lead = 0.5f * (f32)width - segW; break;
                    case 6: lead = 0.5f * (f32)width - 0.5f * segW; break;
                    case 7: lead = 0.5f * (f32)width; break;
                    case 8: lead = 0.75f * (f32)width - 0.5f * segW; break;
                    case 9: lead = (f32)width - segW; break;
                    default: fn_8018D2B8(lbl_80382208, origAlign); break;
                }
            }

            f32 fh = (f32)(s32)(s8)font[2];
            f32 w5 = (f32)sp10 * scale / fh;
            f32 h7 = (f32)spC * scale / fh;
            f32 l3 = lead / fh;
            f32 y2 = lineY / fh;
            f32 p1 = pad / fh;
            f32 v8 = vOff / fh;
            if (maxW < w5) maxW = w5;
            if (minX > l3) minX = l3;
            f32 a = minX + p1;
            f32 b = maxW + a;
            f32 c = h7 + (y2 + v8);
            lbl_80413390[0] = a;
            lbl_80413390[1] = v8;
            lbl_80413390[2] = zero;
            lbl_80413390[3] = b;
            lbl_80413390[4] = v8;
            lbl_80413390[5] = zero;
            lbl_80413390[6] = a;
            lbl_80413390[7] = c;
            lbl_80413390[8] = zero;
            lbl_80413390[9] = b;
            lbl_80413390[10] = c;
            lbl_80413390[11] = zero;

            fn_80189EFC(font, lead + pad, lineY + vOff, scale, scale, cur, colors, (u32)count);

            switch (align) {
                case 0: lead = lead - (segW + pad); break;
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9: lead = lead + (segW + pad); break;
                default: fn_8018D2B8(lbl_80382208, origAlign); break;
            }
            pad = 0.0f;
            spaces = 0;
            segLen = (s32)strlen((const char*)cur);
            if (hit != NULL) *hit = (u8)ch;
        }

        if (hit == NULL) {
            cur = NULL;
            continue;
        }

        switch ((u32)ch) {
            case 10:
                lineY = lineY + (f32)lineH;
                cur = hit + 1;
                count -= segLen + 1;
                colors += segLen + 1;
                first = 1;
                break;
            case 5:
                lineY = lineY + (f32)(lineH / 2);
                cur = hit + 1;
                count -= segLen + 1;
                colors += segLen + 1;
                first = 1;
                break;
            case 4:
                cur = hit + 1;
                spaces++;
                colors += segLen + 1;
                count -= segLen + 1;
                break;
            case 2: {
                u8* sets = *(u8**)((u8*)gScreenSets + 0x84);
                u8* tbl = sets != NULL ? *(u8**)(sets + 0x2C) : NULL;
                if (tbl != NULL) {
                    u32 hash = *(u32*)(hit + 1);
                    u8** slot = (u8**)fn_8019AEAC(tbl, hash, 1);
                    if (slot != NULL && *slot != NULL) {
                        font = *(u8**)(*slot + 8);
                    } else {
                        fn_8018D2B8(lbl_80382208 + 0x2C, hash);
                        font = *(u8**)(ctx + 8);
                    }
                    lineH = (s8)font[2];
                } else {
                    fn_8018D370(lbl_80382208 + 0x68);
                }
                cur = hit + 5;
                colors += segLen + 5;
                count -= segLen + 5;
                break;
            }
            case 3: {
                u32 hash = *(u32*)(hit + 1);
                if (hash == kHashLeft) align = 3;
                else if (hash == 0xCAAB42DFu) align = 4;
                else if (hash == 0x8AAA622Fu) align = 5;
                else if (hash == kHashCenter) align = 6;
                else if (hash == 0x4EFDDBA5u) align = 7;
                else if (hash == 0xBF210E50u) align = 8;
                else if (hash == kHashRight) align = 9;
                cur = hit + 5;
                colors += segLen + 5;
                count -= segLen + 5;
                first = 1;
                break;
            }
            default:
                cur = hit + 1;
                colors += segLen + 1;
                count -= segLen + 1;
                break;
        }
    }

    *(u8**)(ctx + 8) = origFont;
    return lbl_80413390;
}

}  // extern "C"
