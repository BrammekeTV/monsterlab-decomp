// Draft miss384: effect renderers in 0x8016xxxx: the film-dust overlay (0x80163C60) and two
// large effect draw/update state machines (0x801649B4, 0x80166FC4).
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "revolution.h"
#include "engine/rend.h"
#include "../engine/rend_dl_inline.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void fn_80146328(u32* state, u32 seed);                                       // 0x80146328: seeds the 32-word random generator state
void fn_80149B4C(void* a, void* b, void* c, s32 w, s32 h, const char* name);  // 0x80149B4C
void fn_8014E3EC(s32 n);                                                      // 0x8014E3EC
void fn_80151E38(s32 quads, void* verts);                                     // 0x80151E38
s32  fn_801499F8(u8* target, s32 w, s32 h, s32 fmt, s32 a, s32 b, const char* name);   // 0x801499F8
void fn_801646E8(u8* arg0, u8* arg1);                                         // 0x801646E8
u32* fn_80156F70(s32 table, s32 enable);                                      // 0x80156F70 (returns the colour table)
void fn_80157320(void);                                                       // 0x80157320 (empty; the original calls pass 0..2 in r3, ignored)
f32  fn_8015EF5C(u8* obj);                                                    // 0x8015EF5C (returns *(f32*)(obj + 0x14C))
u32  RandomU32(void);                                                         // 0x80143460
s32  fn_80156A80(void);                                                       // 0x80156A80
s32  fn_80156A88(void);                                                       // 0x80156A88

extern u8         lbl_8040D500[];   // 0x8040D500 (object with a float at +0x14C)
extern u8         lbl_80348950[];   // 0x80348950 (0x20-byte records, u32 at +0)
extern u8         lbl_80348A10[];   // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32        lbl_80348AD0;     // 0x80348AD0
extern u8         lbl_80348AF0[];   // 0x80348AF0 (hex text of the 32x32 film dust texture)
extern u8         lbl_80348EF8[];   // 0x80348EF8
extern s32        lbl_80376008[];   // 0x80376008 (random generator index table)
extern s32        lbl_80376084[];   // 0x80376084 (random generator index table)
extern const char lbl_80376810[];   // 0x80376810 ("FilmDust")
extern s32        lbl_8040ADF0[8];  // 0x8040ADF0
extern s32        lbl_8040AE10[8];  // 0x8040AE10
extern u8         lbl_804126B8[];   // 0x804126B8 (film dust texture object)
extern s32        lbl_805FB4B0;     // 0x805FB4B0
extern const char lbl_80376820[];   // 0x80376820 ("FilmGrain0")
extern const char lbl_8037682C[];   // 0x8037682C ("FilmGrain1")
extern s32        lbl_805FB5A0;     // 0x805FB5A0 (film dust texture built)
}

static inline u8 HexNibble(u8 c) {
    if (c >= '0' && c <= '9') return (u8)(c - '0');
    if (c >= 'a' && c <= 'f') return (u8)(c - 'a' + 10);
    return 0;
}

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

// Binds a film texture object (u16 size at +4/+6, bytes at +0xD..+0x13, image pointers at
// +0x18/+0x1C) to texture unit 0: only re-sends the image setup when the image changed.
static inline void LoadFilmTexture(u8* t) {
    Bp(0xFE000060);
    Bp(((u32)t[0x12] << 5) | 0x80000000u);
    u32 imgAddr = *(u32*)(t + 0x18);
    if ((u32)lbl_8040ADF0[0] != imgAddr || lbl_8040AE10[0] != 0) {
        lbl_805FB4B0++;
        lbl_8040ADF0[0] = (s32)imgAddr;
        lbl_8040AE10[0] = 0;
        u32 cfg = lbl_80348AD0;
        u8 idx = t[0x13];
        s32 second = *(s32*)(lbl_80348A10 + idx * 0x20);
        Bp(((u32)(t[0xF] - 1) & 0xF) << 12 | 0x84000000u);
        Bp(((u32)*(u16*)(t + 4) - 1) | (((u32)*(u16*)(t + 6) - 1) << 10) |
           ((u32)t[0x10] << 20) | 0x88000000u);
        Bp((*(u32*)(lbl_80348950 + idx * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
        if (second >= 0) {
            Bp(((u32)second >> 5) | 0x900D0000u | 0x8000u);
        } else {
            Bp(0x90000000u);
        }
        Bp(((*(u32*)(t + 0x18) >> 5) & 0xFFFFFF) | 0x94000000u);
        if (t[0xD] != 0) {
            u32 g = (cfg >> 9) & 0x3FF;
            Bp(((g | 0x98000000u) & ~0x3FC00u) | (((u32)t[0x11] << 10) & 0x3FC00u));
            Bp(0xFEFFFF00u);
            Bp(0x0F000000u);
            Bp(((*(u32*)(t + 0x1C) >> 5) & 0xFFFFFF) | 0x64000000u);
            Bp(((g | 0x65000000u) & ~0x1FC00u) | ((((s32)t[0xD] >> 1) << 10) & 0x1FC00u));
            Bp(0xFEFFFF00u);
            Bp(0x0F000000u);
        }
    }
    Bp(((u32)*(u16*)(t + 4) - 1) | 0x30000000u);
    Bp(((u32)*(u16*)(t + 6) - 1) | 0x31000000u);
}

static inline s32 ClampByteS(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

// Colour table ramp: out[i] = tbl[i] * wOrig + tbl[idx] * wShift, per byte channel, where
// idx = clamp(round(offset + i * scale), 0, 255). The result is packed back to bytes (truncated).
static inline void RampColourTable(const u32* tbl, u32* out, f32 offset, f32 scale, f32 wShift,
                                   f32 wOrig) {
    for (s32 i = 0; i < 0x100; i++) {
        s32 idx = ClampByteS((s32)lrintf(offset + (f32)i * scale));
        u32 a = tbl[i];
        u32 b = tbl[idx];
        u32 packed = 0;
        for (s32 j = 0; j < 4; j++) {
            f32 ca = 0.003921569f * (f32)((a >> (8 * j)) & 0xFF);
            f32 cb = 0.003921569f * (f32)((b >> (8 * j)) & 0xFF);
            f32 m = ca * wOrig + cb * wShift;
            packed |= (u32)ClampByteS((s32)(255.0f * m)) << (8 * j);
        }
        out[i] = packed;
    }
}

// Two-stage state with everything switched off (the teardown of StateTwoStages).
static inline void StateTwoStagesOff(void) {
    Bp(0xFE00000F); Bp(0);
    XfHead(0, 0x103F); GXWGFifo._u32 = 0;
    Bp(0xFE003C00); Bp(0);
    XfHead(0, 0x1010); GXWGFifo._u32 = 0x441;
    Bp(0x28380000);
    Bp(0xC008F2A4);
    Bp(0xC108E6F0);
}

extern "C" {

// 0x80163C60
// Film dust overlay: builds the 32x32 dust texture on first use, then draws arg1[4] randomly
// placed, randomly tinted textured quads (alpha scaled by arg1[8]) when arg1[0x10] > 0.
void fn_80163C60(u8* arg0, u8* arg1) {
    u32 st[32];
    u8  tex[0x200];

    if (!(*(f32*)(arg1 + 0x10) > 0.0f)) return;

    if (lbl_805FB5A0 == 0) {
        lbl_805FB5A0 = 1;
        const u8* src = lbl_80348AF0;
        for (s32 i = 0; i < 0x200; i++) {
            u8 lo = HexNibble(src[0]);
            u8 hi = HexNibble(src[1]);
            src += 2;
            tex[i] = (u8)(lo | ((hi << 4) & 0xFF0));
        }
        fn_80149B4C(lbl_804126B8, tex, lbl_80348EF8, 0x20, 0x20, lbl_80376810);
    }

    Bp(0xFE000FE0);
    Bp(0x41000420);
    if (!(lbl_805FB50C & 2)) {
        lbl_805FB50C |= 2;
        if (lbl_805FB50C & 1) {
            StateTwoStages();
        } else {
            StateOneChannelOneTex();
        }
    }

    LoadFilmTexture(lbl_804126B8);

    s32 left = *(s32*)(arg1 + 0x4);
    s32 size = (left * 4 * 0x18 + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < size) fn_8014E3EC(size);
    s32 pos = lbl_805FB4C4;
    lbl_805FB4C4 = pos + size;
    u8* base = lbl_805FB4D0 + pos;
    lbl_805FB4CC -= size;
    u8* v = base;
    fn_80146328(st, *(u32*)(arg1 + 0x18));
    f32 invW = 16.0f / (f32)fn_80156A80();
    f32 invH = 16.0f / (f32)fn_80156A88();

    while (left > 0) {
        s32 chunk = 0x20;
        if (left <= 0x20) chunk = left;
        for (s32 n = chunk; n > 0; n--) {
            u32 r[5];
            for (s32 k = 0; k < 5; k++) {
                u32 i = st[31];
                st[31] = (u32)lbl_80376008[i];
                st[lbl_80376008[i]] += st[lbl_80376084[i]];
                r[k] = st[st[31]];
            }
            u32 a = r[0], b = r[1], c = r[2], d = r[3], e = r[4];

            f32 x0 = -1.0f + 0.000015259022f * (2.0f * (f32)((a >> 16) ^ (a & 0xFFFF)));
            f32 y0 = -1.0f + 0.000015259022f * (2.0f * (f32)((b >> 16) ^ (b & 0xFFFF)));
            f32 sz = 1.0f + 0.000015259022f * (1.0f * (f32)((c >> 16) ^ (c & 0xFFFF)));
            f32 y1 = y0 - invH * sz;
            f32 x1 = x0 + invW * sz;
            f32 al = 0.5f + 0.000015259022f * (0.5f * (f32)((d >> 16) ^ (d & 0xFFFF)));
            u32 ev = (e >> 16) ^ (e & 0xFFFF);
            u32 sel = (ev >> 8) ^ (ev & 0xFF);
            f32 tv = (f32)((sel >> 2) & 3) * 0.25f;
            f32 tu = (f32)(sel & 3) * 0.25f;
            u32 col = 0xFFFFFF00u | ((u32)(s32)(255.0f * *(f32*)(arg1 + 0x8) * al) & 0xFF);

            *(f32*)(v + 0xC) = x0;
            *(f32*)(v + 0x10) = y1;
            *(f32*)(v + 0x14) = -1.0f;
            *(u32*)(v + 0x8) = col;
            *(f32*)(v + 0x0) = tu;
            *(f32*)(v + 0x4) = tv + 0.25f;
            *(f32*)(v + 0x24) = x1;
            *(f32*)(v + 0x28) = y1;
            *(f32*)(v + 0x2C) = -1.0f;
            *(u32*)(v + 0x20) = col;
            *(f32*)(v + 0x18) = tu + 0.25f;
            *(f32*)(v + 0x1C) = tv + 0.25f;
            *(f32*)(v + 0x3C) = x0;
            *(f32*)(v + 0x40) = y0;
            *(f32*)(v + 0x44) = -1.0f;
            *(u32*)(v + 0x38) = col;
            *(f32*)(v + 0x30) = tu;
            *(f32*)(v + 0x34) = tv;
            *(f32*)(v + 0x54) = x1;
            *(f32*)(v + 0x58) = y0;
            *(f32*)(v + 0x5C) = -1.0f;
            *(u32*)(v + 0x50) = col;
            *(f32*)(v + 0x48) = tu + 0.25f;
            *(f32*)(v + 0x4C) = tv;
            v += 0x60;
        }
        fn_80151E38(chunk, base);
        left -= chunk;
    }

    Bp(0xFE000FE0);
    Bp(0x410004A0);
    if (lbl_805FB50C & 2) {
        u8 f = lbl_805FB50C;
        lbl_805FB50C = f & ~2;
        if (f & 1) {
            StateTwoStagesOff();
        } else {
            StateOneChannelNoTex();
        }
    }
}


// 0x801649B4
// Film grain overlay: builds two 32x32 grain textures on first use, flips between them (and
// regenerates the other through fn_801646E8) every arg1[4] seconds, then tiles the screen with
// 32x32-pixel quads of random texture offset tinted by arg1[8..0x14] and alpha arg1[0x18]/arg1[0].
void fn_801649B4(u8* arg0, u8* arg1) {
    if (!(*(f32*)(arg1 + 0x18) > 0.0f)) return;

    if (arg1[0x20] == 0) {
        arg1[0x20] = 1;
        *(s32*)(arg1 + 0x24) = 0;
        fn_801499F8(arg1 + 0x28, 0x20, 0x20, 0x11, 0, 1, lbl_80376820);
        fn_801499F8(arg1 + 0x48, 0x20, 0x20, 0x11, 0, 1, lbl_8037682C);
    }

    if (*(f32*)(arg1 + 0x1C) >= *(f32*)(arg1 + 0x4)) {
        *(s32*)(arg1 + 0x24) ^= 1;
        fn_801646E8(arg0, arg1);
        while (*(f32*)(arg1 + 0x1C) >= *(f32*)(arg1 + 0x4)) {
            *(f32*)(arg1 + 0x1C) -= *(f32*)(arg1 + 0x4);
        }
    }

    Bp(0xFE000FE0);
    Bp(0x41000420);
    if (!(lbl_805FB50C & 2)) {
        lbl_805FB50C |= 2;
        if (lbl_805FB50C & 1) {
            StateTwoStages();
        } else {
            StateOneChannelOneTex();
        }
    }

    LoadFilmTexture(arg1 + 0x28 + (*(s32*)(arg1 + 0x24) << 5));

    s32 r = (s32)(255.0f * *(f32*)(arg1 + 0x8));
    s32 g = (s32)(255.0f * *(f32*)(arg1 + 0xC));
    s32 b = (s32)(255.0f * *(f32*)(arg1 + 0x10));
    s32 a = (s32)(*(f32*)(arg1 + 0x18) * (255.0f * *(f32*)(arg1 + 0x14)) / *(f32*)(arg1 + 0x0));
    s32 cols = (fn_80156A80() + 0x1F) / 32;
    s32 rows = (fn_80156A88() + 0x1F) / 32;
    u32 col = ((u32)(a & 0xFF)) | (((u32)b & 0xFF) << 8) | (((u32)g & 0xFF) << 16) | ((u32)r << 24);
    s32 size = (cols * 4 * 0x18 + 0x1F) & ~0x1F;

    for (s32 row = 0; row < rows; row++) {
        if (lbl_805FB4CC < size) fn_8014E3EC(size);
        s32 pos = lbl_805FB4C4;
        lbl_805FB4C4 = pos + size;
        u8* base = lbl_805FB4D0 + pos;
        lbl_805FB4CC -= size;
        u8* v = base;
        f32 yTop = 1.0f - 2.0f * ((32.0f * (f32)row) / (f32)fn_80156A88());
        f32 yBot = 1.0f - 2.0f * ((32.0f * (f32)(row + 1)) / (f32)fn_80156A88());
        for (s32 i = 0; i < cols; i++) {
            f32 xl = 2.0f * ((32.0f * (f32)i) / (f32)fn_80156A80()) - 1.0f;
            f32 xr = 2.0f * ((32.0f * (f32)(i + 1)) / (f32)fn_80156A80()) - 1.0f;
            f32 u = 2.3283064e-10f * (f32)RandomU32();
            f32 w = 2.3283064e-10f * (f32)RandomU32();
            *(f32*)(v + 0xC) = xl;
            *(f32*)(v + 0x10) = yBot;
            *(f32*)(v + 0x14) = -1.0f;
            *(u32*)(v + 0x8) = col;
            *(f32*)(v + 0x0) = u;
            *(f32*)(v + 0x4) = 1.0f + w;
            *(f32*)(v + 0x24) = xr;
            *(f32*)(v + 0x28) = yBot;
            *(f32*)(v + 0x2C) = -1.0f;
            *(u32*)(v + 0x20) = col;
            *(f32*)(v + 0x18) = 1.0f + u;
            *(f32*)(v + 0x1C) = 1.0f + w;
            *(f32*)(v + 0x3C) = xl;
            *(f32*)(v + 0x40) = yTop;
            *(f32*)(v + 0x44) = -1.0f;
            *(u32*)(v + 0x38) = col;
            *(f32*)(v + 0x30) = u;
            *(f32*)(v + 0x34) = w;
            *(f32*)(v + 0x54) = xr;
            *(f32*)(v + 0x58) = yTop;
            *(f32*)(v + 0x5C) = -1.0f;
            *(u32*)(v + 0x50) = col;
            *(f32*)(v + 0x48) = 1.0f + u;
            *(f32*)(v + 0x4C) = w;
            v += 0x60;
        }
        fn_80151E38(cols, base);
    }

    Bp(0xFE000FE0);
    Bp(0x410004A0);
    if (lbl_805FB50C & 2) {
        u8 f = lbl_805FB50C;
        lbl_805FB50C = f & ~2;
        if (f & 1) {
            StateTwoStagesOff();
        } else {
            StateOneChannelNoTex();
        }
    }
}

// 0x80166FC4
// Colour grading: blends each of the three 256-entry colour tables (fn_80156F70 0..2) towards a
// shifted copy of itself (index shifted by offset + i * arg1[8/0xC/0x10]) by arg1[0x18] / arg1[0],
// writes the result back into the tables and re-applies them via fn_80157320.
void fn_80166FC4(u8* arg0, u8* arg1) {
    u32 out0[0x100];
    u32 out1[0x100];
    u32 out2[0x100];

    if (!(*(f32*)(arg1 + 0x18) > 0.0f)) return;

    u32* t0 = fn_80156F70(0, 0);
    u32* t1 = fn_80156F70(1, 0);
    u32* t2 = fn_80156F70(2, 0);
    f32 wShift = *(f32*)(arg1 + 0x18) / *(f32*)(arg1 + 0x0);
    f32 wOrig = 1.0f - wShift;
    f32 offset = *(f32*)(arg1 + 0x4) * (255.0f * fn_8015EF5C(lbl_8040D500));

    RampColourTable(t0, out0, offset, *(f32*)(arg1 + 0x8), wShift, wOrig);
    RampColourTable(t1, out1, offset, *(f32*)(arg1 + 0xC), wShift, wOrig);
    RampColourTable(t2, out2, offset, *(f32*)(arg1 + 0x10), wShift, wOrig);

    for (s32 i = 0; i < 0x100; i++) {
        t0[i] = out0[i];
        t1[i] = out1[i];
        t2[i] = out2[i];
    }
    fn_80157320();
    fn_80157320();
    fn_80157320();
}

}   // extern "C"
