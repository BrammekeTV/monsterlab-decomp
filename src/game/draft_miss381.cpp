#include "types.h"
#include "revolution.h"

extern "C" {
void fn_80152620(s32 prim, s32 count, u8* verts);  // 0x80152620
void fn_8014E3EC(s32 n);                           // 0x8014E3EC
void fn_801606A4(u8* arg0, u8* arg1);              // 0x801606A4 (return value unused by callers)

extern s32 lbl_80404040[];   // 0x80404040 (per-index flag words)
extern u8  lbl_805FB50C;     // 0x805FB50C (GX shadow flags byte)
extern s32 lbl_805FB514;     // 0x805FB514 (GX shadow word)
extern s32 lbl_805FB4C4;     // 0x805FB4C4
extern s32 lbl_805FB4CC;     // 0x805FB4CC
extern u8* lbl_805FB4D0;     // 0x805FB4D0 (base of the open display list)
extern f32 lbl_805FF47C;     // 0x805FF47C (float constant)
extern f32 lbl_805FF488;     // 0x805FF488 (float constant)
extern f32 lbl_805FF458;     // 0x805FF458 (float constant)
extern f32 lbl_805FF48C;     // 0x805FF48C (float constant)
extern f32 lbl_805FF6F0;     // 0x805FF6F0 (float constant, colour scale)
}

// Clamps a colour channel to 0..255.
static s32 ClampColorByte(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

extern "C" {

// 0x80156F40
// Sets the flag word lbl_80404040[index] to 1.
void fn_80156F40(s32 index) {
    lbl_80404040[index] = 1;
}

// 0x80156F58
// Clears the flag word lbl_80404040[index].
void fn_80156F58(s32 index) {
    lbl_80404040[index] = 0;
}

// 0x8016F648
// Classifies a bounding sphere (xyz + radius at arg1) against the six frustum planes at
// arg0 + 0xC0 (nx, ny, nz, d per plane). Returns 1 when the sphere lies past a plane, 2 when it
// straddles one, otherwise 0.
s32 fn_8016F648(u8* arg0, f32* arg1) {
    u8* plane = arg0 + 0xC0;
    s32 result = 0;
    s32 i = 6;
    do {
        f32 dot = (*(f32*)(plane + 0x0) * arg1[0] + *(f32*)(plane + 0x4) * arg1[1]) +
                  *(f32*)(plane + 0x8) * arg1[2];
        f32 dist = dot - *(f32*)(plane + 0xC);
        f32 radius = arg1[3];
        if (dist > radius) {
            return 1;
        }
        if (dist > -radius) {
            result = 2;
        }
        plane += 0x10;
    } while (--i);
    return result;
}

// 0x8016F6FC
// Returns 1 when *arg0 <= *arg1 (ordered compare).
s32 fn_8016F6FC(f32* arg0, f32* arg1) {
    return *arg0 <= *arg1;
}

// 0x8016F798
// Componentwise minimum of arg1 and arg2 written to arg0.
void fn_8016F798(f32* arg0, f32* arg1, f32* arg2) {
    arg0[0] = (arg1[0] < arg2[0]) ? arg1[0] : arg2[0];
    arg0[1] = (arg1[1] < arg2[1]) ? arg1[1] : arg2[1];
    arg0[2] = (arg1[2] < arg2[2]) ? arg1[2] : arg2[2];
}

// 0x8016F7F0
// Componentwise maximum of arg1 and arg2 written to arg0.
void fn_8016F7F0(f32* arg0, f32* arg1, f32* arg2) {
    arg0[0] = (arg1[0] > arg2[0]) ? arg1[0] : arg2[0];
    arg0[1] = (arg1[1] > arg2[1]) ? arg1[1] : arg2[1];
    arg0[2] = (arg1[2] > arg2[2]) ? arg1[2] : arg2[2];
}

// 0x80174A80
// If the GX shadow flag bit 0x4 is set, clears it and emits the two BP register writes
// (mode word with bit 0x40 set, then 0xF33F0000) to the write-gather pipe.
void fn_80174A80(void) {
    if (lbl_805FB50C & 0x4) {
        lbl_805FB50C = (u8)(lbl_805FB50C & ~0x4);
        s32 v = lbl_805FB514 | 0x40;
        lbl_805FB514 = v;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)v | 0x43000000u;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0xF33F0000u;
    }
}

// 0x801759D0
// Emits two fixed XF/CP register-write groups to the write-gather pipe.
void fn_801759D0(void) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x100E;
    GXWGFifo._u32 = 0x400;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x1010;
    GXWGFifo._u32 = 0x400;
}

// 0x80160808
// Walks the 32 records at arg0 (28 bytes each, type halfword at +4). Types 1 and 2 call
// fn_801606A4; type 2 also advances the record's word at +0 by 0x20 (wrapped to 5 bits) and
// swaps the value at arg0 + 0x380 into the record. Bumps the counter at arg0 + 0x384.
void fn_80160808(u8* arg0) {
    s32 i = 0;
    u8* p = arg0;
    do {
        s16 type = *(s16*)(p + 0x4);
        if (type == 1 || type == 2) {
            fn_801606A4(arg0, p);
        }
        if (type == 2) {
            s32 next = *(s32*)p + 0x20;
            *(s32*)p = next;
            *(s16*)(p + 0x4) = 0;
            *(s16*)(p + 0x6) = (s16)*(s32*)(arg0 + 0x380);
            *(s32*)(arg0 + 0x380) = next & 0x1F;
        }
        i += 1;
        p += 0x1C;
    } while (i < 0x20);
    *(s32*)(arg0 + 0x384) += 1;
}

// 0x80165EF0
// Emits a GX command pair and, when the record at arg1 is visible and its +0x20 value lies in
// (-1, 1), appends one 0xA8-vertex primitive to the open display list.
void fn_80165EF0(u8* arg0, u8* arg1) {
    if (*(f32*)(arg1 + 0x10) > lbl_805FF47C) {
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0xFE000FE0;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0x41000420;

        f32 a = *(f32*)(arg1 + 0x20);
        if (a > lbl_805FF488 && a < lbl_805FF458) {
            s32 alpha = (s32)(lbl_805FF48C * *(f32*)(arg1 + 0x4) * *(f32*)(arg1 + 0x24));
            if (lbl_805FB4CC < 0x20) {
                fn_8014E3EC(0x20);
            }
            s32 base = lbl_805FB4C4;
            s32 color = (s32)(0xFFFFFF00u | ((u32)alpha & 0xFFu));
            lbl_805FB4C4 = base + 0x20;
            u8* v = lbl_805FB4D0 + base;
            lbl_805FB4CC -= 0x20;
            *(f32*)(v + 0x4) = a;
            *(f32*)(v + 0x8) = lbl_805FF458;
            *(f32*)(v + 0xC) = lbl_805FF488;
            *(s32*)(v + 0x0) = color;
            *(f32*)(v + 0x14) = *(f32*)(arg1 + 0x20);
            *(f32*)(v + 0x18) = lbl_805FF488;
            *(f32*)(v + 0x1C) = lbl_805FF488;
            *(s32*)(v + 0x10) = color;
            fn_80152620(0xA8, 2, v);
        }

        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0xFE000FE0;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0x410004A0;
    }
}

// 0x801758B0
// Packs a float colour (r, g, b, a in 0..1 at arg0[0..3]) into 0xRRGGBBAA, each channel scaled
// by lbl_805FF6F0 (255), truncated and clamped to a byte.
s32 fn_801758B0(f32* arg0) {
    f32 scale = lbl_805FF6F0;
    u32 r = (u32)ClampColorByte((s32)(scale * arg0[0]));
    u32 g = (u32)ClampColorByte((s32)(scale * arg0[1]));
    u32 b = (u32)ClampColorByte((s32)(scale * arg0[2]));
    u32 a = (u32)ClampColorByte((s32)(scale * arg0[3]));
    return (s32)((r << 24) | (g << 16) | (b << 8) | a);
}

}  // extern "C"
