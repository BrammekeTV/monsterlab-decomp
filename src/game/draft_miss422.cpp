#include "types.h"

extern "C" {
u32  fn_8014B1A8(f32 x, f32 s, f32 v);  // 0x8014B1A8 (hue -> packed RGBA, x = hue fraction)
u8*  fn_8015ECA0(u8* a3);               // 0x8015ECA0
extern f32 lbl_80376840[4];             // 0x80376840 (per-channel weights)
extern u8  lbl_8040D500[];              // 0x8040D500
}

extern "C" {

// 0x80168440
// Effect-slot handler: builds three colours from a hue (*p, +1/3, +2/3 offsets), weights each
// channel by lbl_80376840, normalises by the total and stores the three RGBA vectors into the
// table at +0x60/+0x70/+0x80.
void fn_80168440(void* fx, u8* p) {
    f32 t = *(f32*)p;
    f32 h1 = 0.33333334f + t;
    f32 h2 = 0.6666667f + t;
    u32 c0 = fn_8014B1A8(t - (f32)(s32)t, 1.0f, 0.5f);
    u32 c1 = fn_8014B1A8(h1 - (f32)(s32)h1, 1.0f, 0.5f);
    u32 c2 = fn_8014B1A8(h2 - (f32)(s32)h2, 1.0f, 0.5f);

    f32 a[4], b[4], c[4];
    a[0] = 0.003921569f * (f32)(s32)(c0 & 0xFF);
    a[1] = 0.003921569f * (f32)(s32)((c0 >> 8) & 0xFF);
    a[2] = 0.003921569f * (f32)(s32)((c0 >> 16) & 0xFF);
    a[3] = 0.003921569f * (f32)(s32)(c0 >> 24);
    b[0] = 0.003921569f * (f32)(s32)(c1 & 0xFF);
    b[1] = 0.003921569f * (f32)(s32)((c1 >> 8) & 0xFF);
    b[2] = 0.003921569f * (f32)(s32)((c1 >> 16) & 0xFF);
    b[3] = 0.003921569f * (f32)(s32)(c1 >> 24);
    c[0] = 0.003921569f * (f32)(s32)(c2 & 0xFF);
    c[1] = 0.003921569f * (f32)(s32)((c2 >> 8) & 0xFF);
    c[2] = 0.003921569f * (f32)(s32)((c2 >> 16) & 0xFF);
    c[3] = 0.003921569f * (f32)(s32)(c2 >> 24);

    f32 w0 = lbl_80376840[0];
    f32 w1 = lbl_80376840[1];
    f32 w2 = lbl_80376840[2];
    f32 w3 = lbl_80376840[3];
    f32 sum = (w3 * a[3] + (w2 * a[2] + (w0 * a[0] + w1 * a[1])))
            + (w3 * b[3] + (w2 * b[2] + (w0 * b[0] + w1 * b[1])))
            + (w3 * c[3] + (w2 * c[2] + (w0 * c[0] + w1 * c[1])));
    f32 inv = 1.0f / sum;

    u8* r = fn_8015ECA0(lbl_8040D500);
    *(f32*)(r + 0x60) = a[0] * inv;
    *(f32*)(r + 0x64) = a[1] * inv;
    *(f32*)(r + 0x68) = a[2] * inv;
    *(f32*)(r + 0x6C) = a[3] * inv;
    r = fn_8015ECA0(lbl_8040D500);
    *(f32*)(r + 0x70) = b[0] * inv;
    *(f32*)(r + 0x74) = b[1] * inv;
    *(f32*)(r + 0x78) = b[2] * inv;
    *(f32*)(r + 0x7C) = b[3] * inv;
    r = fn_8015ECA0(lbl_8040D500);
    *(f32*)(r + 0x80) = c[0] * inv;
    *(f32*)(r + 0x84) = c[1] * inv;
    *(f32*)(r + 0x88) = c[2] * inv;
    *(f32*)(r + 0x8C) = c[3] * inv;
}

// 0x80168A58
// Effect-slot handler: when p+0x20 > 0, blends the vector p[0..3] into each of the three
// colour vectors at +0x60/+0x70/+0x80 of the table, with weight k = p[4] * p[8].
void fn_80168A58(void* fx, u8* p) {
    f32 amt = *(f32*)(p + 0x20);
    if (amt > 0.0f) {
        f32 k = *(f32*)(p + 0x10) * amt;
        for (s32 off = 0x60; off <= 0x80; off += 0x10) {
            f32 add0 = *(f32*)(p + 0x0) * k;
            f32 add1 = *(f32*)(p + 0x4) * k;
            f32 add2 = *(f32*)(p + 0x8) * k;
            f32 add3 = *(f32*)(p + 0xC) * k;
            f32 keep = 1.0f - k;
            u8* r = fn_8015ECA0(lbl_8040D500);
            f32 v0 = *(f32*)(r + off + 0x0) * keep;
            f32 v1 = *(f32*)(r + off + 0x4) * keep;
            f32 v2 = *(f32*)(r + off + 0x8) * keep;
            f32 v3 = *(f32*)(r + off + 0xC) * keep;
            r = fn_8015ECA0(lbl_8040D500);
            *(f32*)(r + off + 0x0) = v0 + add0;
            *(f32*)(r + off + 0x4) = v1 + add1;
            *(f32*)(r + off + 0x8) = v2 + add2;
            *(f32*)(r + off + 0xC) = v3 + add3;
        }
    }
}

}  // extern "C"
