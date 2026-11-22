// Drafts miss376: lit-scene debug set-up (0x8017514C), fullscreen GX state reset (0x80175C48),
// sphere-vs-frustum cull + draw list build (0x80184C18) and column-normalised 3x3 matrix copy
// (0x80185C98).
#include <math.h>

#include "types.h"
#include "revolution.h"
#include "engine/engine.h"
#include "engine/math.h"

extern "C++" u8 sRendDLs[];                // 0x80401108 (display-list records, 0x94 bytes each)
extern EngineMemCallbacks* gSgMem;         // 0x805FB608 (scene-graph memory callbacks)

extern "C" {
BOOL PadButtonHeld(s32 button);                                   // 0x801474EC
Vec3 QuatGetAxisY(const Quat* q);
void ScalarSet(f32* out, f32 v);                                  // 0x80053250
void Vec3Copy(Vec3* out, const Vec3* v);                          // 0x80052640
f32  Vec3GetX(const Vec3* v);
f32  Vec3GetY(const Vec3* v);
f32  Vec3GetZ(const Vec3* v);
void Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);       // 0x80053224
void Vec3NormalizeTo(Vec3* out, const Vec3* v);                   // 0x80055D9C
void fn_800359F4(Vec3* out, f32 x, f32 y, f32 z);                 // 0x800359F4
s32  fn_80044694(void);                                           // 0x80044694
u32  fn_80044C0C(void);                                           // 0x80044C0C
void fn_80052638(void);                                        // 0x80052638
void fn_8005263C(Quat* q);                                        // 0x8005263C
void fn_8010AAE0(void);                                           // 0x8010AAE0
void fn_8010AB0C(s32 a, s32 b);                                   // 0x8010AB0C
void fn_8010AB70(void);                                           // 0x8010AB70
void fn_801458B0(Quat* quat, const Vec3* a, const Vec3* b);       // 0x801458B0
void fn_80146180(void* quat, const Vec3* axis, f32 angle);        // 0x80146180
void fn_80148770(s32 n);                                          // 0x80148770
void fn_8014879C(f32* out);                                       // 0x8014879C
void fn_801487B4(f32* out);                                       // 0x801487B4
void fn_801487CC(const f32* a, const f32* b, const f32* c, const f32* d);  // 0x801487CC
void fn_80148D94(const f32* v);                                   // 0x80148D94
void fn_801494B4(void);                                           // 0x801494B4
void fn_8014950C(s32 a, s32 b);                                   // 0x8014950C
void fn_80149540(void);                                           // 0x80149540
void fn_80149594(void);                                           // 0x80149594
void fn_8014970C(s32 a, s32 b);                                   // 0x8014970C
void fn_80149744(s32 a);                                          // 0x80149744
void fn_80149780(void);                                           // 0x80149780
void fn_8014DE68(void);                                           // 0x8014DE68
void fn_8015428C(s32 a);                                          // 0x8015428C
void fn_80154318(void);                                           // 0x80154318
void fn_80158BC4(s32 extra);                                      // 0x80158BC4
void fn_801590A0(void);                                           // 0x801590A0
s32  fn_8015BA30(void* p);                                        // 0x8015BA30
void fn_80171ABC(u8* p);                                          // 0x80171ABC
void fn_80174A80(void);                                           // 0x80174A80
void fn_801759A8(void);                                           // 0x801759A8
void fn_801759D0(void);                                           // 0x801759D0
void fn_80175A0C(s32 a, const Vec3* v);                           // 0x80175A0C
s32  fn_801758B0(const f32* color);                               // 0x801758B0 (packs an RGBA colour)
void fn_80175B60(void* obj, s32 slot, f32 x, f32 y, f32 z);       // 0x80175B60
void fn_80175B8C(void* obj, s32 slot, f32 x, f32 y, f32 z);       // 0x80175B8C
void fn_80175BB8(void* obj, s32 slot, f32 x, f32 y, f32 z);       // 0x80175BB8
void fn_80175BE4(void* obj, s32 slot, f32 x, f32 y, f32 z);       // 0x80175BE4
void fn_80175C10(f32* out, f32 a, f32 b, f32 c, f32 d);           // 0x80175C10
void fn_80175C24(void* obj, s32 slot, s32 color);                 // 0x80175C24
void* fn_80183F64(u8* p);                                         // 0x80183F64

extern u8  lbl_80412B20[];     // 0x80412B20 (static record)
extern s32 lbl_805F9330;       // 0x805F9330
extern u8  lbl_805F9210;       // 0x805F9210
extern u8  lbl_805F9211;       // 0x805F9211
extern u8  lbl_805F9212;       // 0x805F9212
extern u8  lbl_805F9340;       // 0x805F9340
extern u8  lbl_805F9341;       // 0x805F9341
extern f32 lbl_805F9208;       // 0x805F9208
extern s32 lbl_805FB4BC;       // 0x805FB4BC (current viewport index)
extern s32 lbl_805FB4C8;       // 0x805FB4C8 (render budget)
extern u8  lbl_805FB50C;       // 0x805FB50C (GX shadow flags byte)
extern u8  lbl_805FB50E;       // 0x805FB50E
extern u32 lbl_805FB514;       // 0x805FB514
extern u8  lbl_805FB598[];     // 0x805FB598
extern void* lbl_805FB5F8;     // 0x805FB5F8 (current camera)
extern s32 lbl_805FB614;       // 0x805FB614
}

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

// Sets the position, direction and attenuation words of light slot `slot` of the light block
// at lbl_805FB598 (the colour is set separately).
static void SetLightSlot(s32 slot, const Vec3* pos) {
    f32 z = Vec3GetZ(pos);
    f32 y = Vec3GetY(pos);
    f32 x = Vec3GetX(pos);
    fn_80175B60(lbl_805FB598, slot, x, y, z);
    fn_80175B8C(lbl_805FB598, slot, 0.0f, 0.0f, 1.0f);
    fn_80175BB8(lbl_805FB598, slot, 1.0f, 0.0f, 0.0f);
    fn_80175BE4(lbl_805FB598, slot, 1.0f, 0.0f, 0.0f);
}

// Light slot from a fixed direction: normalise it, push it far away (1e19) and set the slot.
static void SetDirLight(s32 slot, const Vec3* dir, Vec3* cur, s32 color) {
    f32 scale;
    Vec3 n, v;
    ScalarSet(&scale, 1e19f);
    Vec3NormalizeTo(&n, dir);
    Vec3MulScalar(&v, &n, &scale);
    Vec3Copy(cur, &v);
    SetLightSlot(slot, cur);
    fn_80175C24(lbl_805FB598, slot, color);
}

// Reciprocal square root, refined like the frsqrte + Newton steps in the original.
static f32 RSqrtRefined(f32 x) { return (f32)(1.0 / sqrt((f64)x)); }

extern "C" {

// 0x8017514C
// Debug lighting set-up: resets the render state, builds the camera for the given mode (0-3),
// then programs four lights (slots 0x40, 0x50, 0x60, 0x70) from fixed directions; pad buttons
// 0x1087/0x1088 spin the angle, 0x1086 picks the rotating key light.
void fn_8017514C(s32 mode) {
    Vec3 dir0, dir1, dir2, dir3;     // 0x210, 0x204, 0x1F8, 0x1EC
    Vec3 cur;                        // 0x1E0
    Quat q;                          // 0x1D0
    f32 camA[4], camB[4], camC[4], camD[4];

    fn_80158BC4(0x294);
    lbl_805F9330 = mode;
    fn_80148770(3);
    fn_801494B4();
    fn_8014950C(1, 1);
    fn_8010AB0C(1, 1);
    fn_8010AB70();
    fn_80154318();
    fn_80149744(2);
    fn_8014970C(6, 8);
    fn_8010AAE0();
    switch (mode) {
    case 0:
        fn_80149594();
        fn_801487B4(camA);
        fn_80148D94(camA);
        fn_8015428C(0);
        fn_801759A8();
        fn_80174A80();
        fn_80044694();
        fn_801759D0();
        break;
    case 1:
        fn_801590A0();
        fn_801487B4(camD);
        fn_8014879C(camC);
        fn_8014879C(camB);
        fn_8014879C(camA);
        fn_801487CC(camA, camB, camC, camD);
        fn_8015428C(1);
        fn_80149780();
        fn_80174A80();
        fn_80044694();
        break;
    case 2:
        fn_801590A0();
        fn_801487B4(camD);
        fn_8014879C(camC);
        fn_8014879C(camB);
        fn_8014879C(camA);
        fn_801487CC(camA, camB, camC, camD);
        fn_8015428C(1);
        fn_80149780();
        fn_80149540();
        fn_80044694();
        break;
    case 3:
        fn_801590A0();
        fn_801487B4(camD);
        fn_8014879C(camC);
        fn_8014879C(camB);
        fn_8014879C(camA);
        fn_801487CC(camA, camB, camC, camD);
        fn_8015428C(1);
        fn_80149780();
        fn_80149540();
        fn_80044C0C();
        break;
    }
    fn_800359F4(&dir0, 1.0f, -1.0f, 6.0f);
    fn_800359F4(&dir1, 3.0f, 2.0f, 7.0f);
    fn_800359F4(&dir2, 6.0f, 6.0f, -7.0f);
    fn_800359F4(&dir3, -6.0f, 6.0f, -7.0f);
    fn_80052638();
    if (PadButtonHeld(0x1087) != 0) {
        lbl_805FB614 = (lbl_805FB614 + 1) % 360;
    }
    if (PadButtonHeld(0x1088) != 0) {
        lbl_805FB614 = (lbl_805FB614 - 1) % 360;
    }
    fn_8005263C(&q);
    if (PadButtonHeld(0x1086) != 0) {
        Vec3 fwd, right, axis, dirOut;
        fn_800359F4(&fwd, 0.0f, 0.0f, 1.0f);
        fn_800359F4(&right, 1.0f, 0.0f, 0.0f);
        fn_801458B0(&q, &right, &fwd);
        fn_800359F4(&axis, 0.0f, 0.0f, 1.0f);
        fn_80146180(&q, &axis, 0.011111111f * (f32)lbl_805FB614);
        dirOut = QuatGetAxisY(&q);
        fn_80175A0C(4, &dirOut);
    } else {
        Vec3 up, right, axis, a88, a94, aA0;
        f32 scale;
        fn_800359F4(&up, 0.0f, 1.0f, 0.0f);
        fn_800359F4(&right, 1.0f, 0.0f, 0.0f);
        fn_801458B0(&q, &right, &up);
        fn_800359F4(&axis, 0.0f, 1.0f, 0.0f);
        fn_80146180(&q, &axis, 0.011111111f * (f32)lbl_805FB614);
        ScalarSet(&scale, 1e19f);
        a88 = QuatGetAxisY(&q);
        Vec3NormalizeTo(&a94, &a88);
        Vec3MulScalar(&aA0, &a94, &scale);
        Vec3Copy(&cur, &aA0);
        SetLightSlot(0x40, &cur);
    }
    PadButtonHeld(0x1085);
    f32 col[4];
    fn_80175C10(col, 1.0f, 1.0f, 1.0f, 0.0f);
    fn_80175C24(lbl_805FB598, 0x40, fn_801758B0(col));

    fn_80175C10(col, 0.25f, 0.25f, 0.25f, 0.0f);
    s32 color1 = fn_801758B0(col);
    SetDirLight(0x50, &dir1, &cur, color1);

    fn_80175C10(col, 1.0f, 1.0f, 1.0f, 0.0f);
    s32 color2 = fn_801758B0(col);
    SetDirLight(0x60, &dir2, &cur, color2);

    fn_80175C10(col, 1.0f, 1.0f, 1.0f, 1.0f);
    s32 color3 = fn_801758B0(col);
    fn_80175B60(lbl_805FB598, 0x70, 0.0f, 0.0f, 0.0f);
    fn_80175B8C(lbl_805FB598, 0x70, 0.0f, 0.0f, 1.0f);
    fn_80175BB8(lbl_805FB598, 0x70, 1.0f, 0.0f, 0.0f);
    fn_80175BE4(lbl_805FB598, 0x70, 1.0f, 0.0f, 0.0f);
    fn_80175C24(lbl_805FB598, 0x70, color3);
}

// 0x80175C48
// Resets the GX pipeline to the full-screen default state: identity position matrix, texture and
// line-draw flags, then sets the colour register to opaque white (clamped 0..255 per channel).
void fn_80175C48(void) {
    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x11DA > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    lbl_805F9330 = -1;
    GxBpWrite(0xFE00000E);
    GxBpWrite(0x40000006);
    GXWGFifo._u8  = 0x10;
    GXWGFifo._u16 = 0xB;
    GXWGFifo._u16 = 0x500;
    GXWGFifo._u32 = 0x3F800000;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0x3F800000;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0x3F800000;
    GXWGFifo._u32 = 0;
    GxBpWrite(0xFE000090);
    GxBpWrite(0x80000090);
    GxBpWrite(0xFE00000F);
    GxBpWrite(0x80000005);
    if (!(lbl_805FB50C & 4)) {
        lbl_805FB50C |= 4;
        u32 r = lbl_805FB514 & 0xFFFFFFBFu;
        lbl_805FB514 = r;
        GxBpWrite(r | 0x43000000u);
        GxBpWrite((u32)lbl_805FB50E | 0xF3380000u | ((u32)lbl_805F9211 << 16));
    }
    GxBpWrite(0xFE000001);
    GxBpWrite(0x41000001);
    lbl_805FB50C |= 8;
    GxBpWrite(0xFE00C000);
    GxBpWrite((u32)lbl_805F9212 << 14);
    if (lbl_805FB50C & 2) {
        u8 old = lbl_805FB50C;
        lbl_805FB50C = old & 0xFD;
        if (old & 1) {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0x10, 0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0x10, 0, 0x1010, 0x441);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008F2A4);
            GxBpWrite(0xC108E6F0);
        } else {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0x10, 0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0x10, 0, 0x100E, 0x442);
            GxXfWrite(0x10, 0, 0x1010, 0x442);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008FFFA);
            GxBpWrite(0xC108FFD0);
        }
    }
    lbl_805F9211 = 6;
    lbl_805FB50E = 8;
    if (lbl_805FB50C & 4) {
        GxBpWrite(0xF33E0008);
    }
    GxBpWrite(0xFE000FE0);
    GxBpWrite(0x410004A0);
    fn_80154318();
    lbl_805F9212 = 2;
    if (lbl_805FB50C & 8) {
        GxBpWrite(0xFE00C000);
        GxBpWrite(0x8000);
    }
    fn_8015428C(1);
    GxBpWrite(0xFE000018);
    GxBpWrite(0x41000018);
    f32 color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x1107 > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    if (lbl_805FB50C & 1) {
        u8 old = lbl_805FB50C;
        lbl_805FB50C = old & 0xFE;
        if (old & 2) {
            GxBpWrite(0xFE00000F);
            GxBpWrite(1);
            GxXfWrite(0x10, 0, 0x103F, 1);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0x10, 0, 0x100E, 0x442);
            GxXfWrite(0x10, 0, 0x1010, 0x442);
            GxBpWrite(0x28380040);
            GxBpWrite(0xC008F8AF);
            GxBpWrite(0xC108F2F0);
        } else {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0x10, 0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0x10, 0, 0x100E, 0x442);
            GxXfWrite(0x10, 0, 0x1010, 0x442);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008FFFA);
            GxBpWrite(0xC108FFD0);
        }
    }
    s32 c[4];
    for (s32 i = 0; i < 4; i++) {
        s32 v = (s32)(255.0f * color[i]);
        c[i] = v < 0 ? 0 : (v > 0xFF ? 0xFF : v);
    }
    GxXfWrite(0x10, 0, 0x100C,
              (u32)c[3] | ((u32)c[2] << 8) | (((u32)c[0] << 24) | ((u32)c[1] << 16)));
}

// 0x80184C18
// Builds the three per-frame draw arrays from the object list at +4 (count at +0xC): each entry is
// tested as a sphere against the camera's two groups of three planes (0 = outside, 1 = inside,
// 2/3 = straddling one/both groups) and the visible ones are appended to the arrays at +0x5C,
// +0x60 and +0x64; +0x68 gets the number of records written.
void fn_80184C18(u8* arg0, s32 arg1) {
    u8* cam;
    f32 size;
    u8* rec;      // 0x20-byte records at +0x5C
    u8* rec2;     // 0x20-byte records at +0x60
    u8* rec3;     // 0x10-byte records at +0x64

    u8* mem = (u8*)gSgMem->allocTemp((u32)(*(s32*)(arg0 + 0xC) * 0x50));
    *(u8**)(arg0 + 0x5C) = mem;
    rec = mem;
    s32 bytes = *(s32*)(arg0 + 0xC) << 5;
    u8* node = *(u8**)(arg0 + 0x4);
    rec2 = mem + bytes;
    rec3 = rec2 + bytes;
    *(u8**)(arg0 + 0x60) = rec2;
    *(u8**)(arg0 + 0x64) = rec3;
    cam = (u8*)lbl_805FB5F8;
    if (arg1 != 0) {
        size = 48.0f;
    } else {
        size = 16.0f;
    }
    while (node != NULL) {
        u8* e = node - 0xB8;
        u32 flags = *(u32*)(node - 0xAC);
        u32 keep = flags & ~0x80u;
        if ((flags & 0xC) != 0xC || (flags & 3) != 0) {
            fn_80171ABC(e);
        }
        f32 x = *(f32*)(e + 0x14);
        f32 y = *(f32*)(e + 0x18);
        f32 z = *(f32*)(e + 0x1C);
        f32 radius = *(f32*)(e + 0x20);
        if (lbl_805F9210 == 0) {
            radius = 10000.0f * radius;
        }
        // Group 1: planes at camera +0xE0, +0xF0, +0x100 (a, b, c, d each).
        f32 d1 = ((x * *(f32*)(cam + 0xE0) + y * *(f32*)(cam + 0xE4)) + z * *(f32*)(cam + 0xE8)) -
                 *(f32*)(cam + 0xEC);
        f32 d2 = ((x * *(f32*)(cam + 0xF0) + y * *(f32*)(cam + 0xF4)) + z * *(f32*)(cam + 0xF8)) -
                 *(f32*)(cam + 0xFC);
        f32 d3 = ((x * *(f32*)(cam + 0x100) + y * *(f32*)(cam + 0x104)) + z * *(f32*)(cam + 0x108)) -
                 *(f32*)(cam + 0x10C);
        f32 m = d1 > d2 ? d1 : d2;
        m = m > d3 ? m : d3;
        s32 vis;
        if (m > radius) {
            vis = 0;
        } else {
            vis = 0;
            if (m > -radius) vis = 1;
            // Group 2: planes at camera +0xC0, +0xD0, +0x110.
            f32 e1 = ((x * *(f32*)(cam + 0xC0) + y * *(f32*)(cam + 0xC4)) + z * *(f32*)(cam + 0xC8)) -
                     *(f32*)(cam + 0xCC);
            f32 e2 = ((x * *(f32*)(cam + 0xD0) + y * *(f32*)(cam + 0xD4)) + z * *(f32*)(cam + 0xD8)) -
                     *(f32*)(cam + 0xDC);
            f32 e3 = ((x * *(f32*)(cam + 0x110) + y * *(f32*)(cam + 0x114)) + z * *(f32*)(cam + 0x118)) -
                     *(f32*)(cam + 0x11C);
            f32 m2 = e1 > e2 ? e1 : e2;
            m2 = m2 > e3 ? m2 : e3;
            if (m2 > radius) {
                vis = 0;
            } else {
                if (m2 > -radius) vis += 1;
                vis += 1;
            }
        }
        if (vis != 0) {
            if (vis > 1) {
                keep |= 0x80;
                *(u32*)(e + 0xC) = keep;
            }
            if ((lbl_805F9341 != 0 && vis > 1) || (lbl_805F9340 != 0 && vis == 1)) {
                *(f32*)(e + 0x3C) = *(f32*)(arg0 + 0x48);
                *(f32*)(e + 0x40) = *(f32*)(arg0 + 0x4C);
                *(f32*)(e + 0x44) = *(f32*)(arg0 + 0x50);
                *(f32*)(e + 0x48) = *(f32*)(arg0 + 0x54);
                typedef s32 (*EntryCb)(u8*, s32);
                EntryCb cb = *(EntryCb*)(e + 0x30);
                s32 ok = 1;
                if (cb != NULL) {
                    ok = cb(e, *(s32*)(e + 0x34));
                }
                if (ok != 0) {
                    u32* slot = (u32*)fn_80183F64(lbl_80412B20);
                    if (slot != NULL) {
                        slot[0] = 0;
                        slot[1] = 0;
                        slot[2] = 0;
                        *(u8**)(rec + 0x10) = e;
                        f32 ex = *(f32*)(e + 0x14);
                        f32 ey = *(f32*)(e + 0x18);
                        f32 ez = *(f32*)(e + 0x1C);
                        f32 er = *(f32*)(e + 0x20);
                        *(f32*)(rec + 0x0) = ex;
                        *(f32*)(rec + 0x4) = ey;
                        *(f32*)(rec + 0x8) = ez;
                        *(f32*)(rec + 0xC) = er;
                        f32 px = *(f32*)(cam + 0xD0);
                        f32 py = *(f32*)(cam + 0xD4);
                        f32 pz = *(f32*)(cam + 0xD8);
                        *(f32*)(rec + 0x14) =
                            (*(f32*)(cam + 0xDC) - ((ez * pz) + ((ex * px) + (ey * py)))) + er;
                        *(u32**)(rec + 0x18) = slot;
                        *(f32*)(rec2 + 0x0) = *(f32*)(e + 0x24);
                        *(f32*)(rec2 + 0x4) = *(f32*)(e + 0x28);
                        *(f32*)(rec2 + 0x8) = *(f32*)(e + 0x2C);
                        *(u32*)(rec2 + 0xC) = keep & 4;
                        *(f32*)(rec2 + 0x10) = size;
                        *(f32*)(rec2 + 0x14) = size;
                        *(f32*)(rec2 + 0x18) = size;
                        *(u32**)(rec2 + 0x1C) = slot;
                        rec2 += 0x20;
                        *(f32*)(rec3 + 0x0) = *(f32*)(rec + 0x0);
                        *(f32*)(rec3 + 0x4) = *(f32*)(rec + 0x4);
                        *(f32*)(rec3 + 0x8) = *(f32*)(rec + 0x8);
                        f32 w = *(f32*)(rec + 0xC);
                        rec += 0x20;
                        *(f32*)(rec3 + 0xC) = w;
                        rec3 += 0x10;
                    }
                }
            }
        } else {
            *(u32*)(e + 0xC) = keep;
        }
        node = *(u8**)node;
    }
    *(s32*)(arg0 + 0x68) = (s32)((rec - *(u8**)(arg0 + 0x5C)) / 32);
}

// 0x80185C98
// Copies `count` 3x3 parts out of 4x4 matrices (src and dst advance `stride` floats per item):
// each source column is normalised and scaled by lbl_805F9208, then stored as a row-major 3x3.
void fn_80185C98(f32* dst, const u8* src, s32 count, s32 stride) {
    for (s32 i = 0; i < count; i++) {
        f32 c0x = *(const f32*)(src + 0x0), c0y = *(const f32*)(src + 0x10), c0z = *(const f32*)(src + 0x20);
        f32 c1x = *(const f32*)(src + 0x4), c1y = *(const f32*)(src + 0x14), c1z = *(const f32*)(src + 0x24);
        f32 c2x = *(const f32*)(src + 0x8), c2y = *(const f32*)(src + 0x18), c2z = *(const f32*)(src + 0x28);
        f32 r0 = RSqrtRefined(c0z * c0z + (c0x * c0x + c0y * c0y));
        f32 r1 = RSqrtRefined(c1z * c1z + (c1x * c1x + c1y * c1y));
        f32 r2 = RSqrtRefined(c2z * c2z + (c2x * c2x + c2y * c2y));
        src += stride * 4;
        f32 s = lbl_805F9208;
        dst[0] = (c0x * r0) * s;
        dst[1] = (c1x * r1) * s;
        dst[2] = (c2x * r2) * s;
        dst[3] = (c0y * r0) * s;
        dst[4] = (c1y * r1) * s;
        dst[5] = (c2y * r2) * s;
        dst[6] = (c0z * r0) * s;
        dst[7] = (c1z * r1) * s;
        dst[8] = (c2z * r2) * s;
        dst += stride;
    }
}

}  // extern "C"
