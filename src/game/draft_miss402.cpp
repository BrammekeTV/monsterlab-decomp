// Misc 0x8014xxxx / 0x8015xxxx / 0x8016xxxx functions: random generator seeding, frame effects.
#include "types.h"
#include <math.h>

extern "C" {
u32  RandomU32(void);                                           // 0x80143460
f64  fn_8033F20C(f64 x, f64 y);                                 // 0x8033F20C (pow)
u8*  fn_80156AAC(s32* width);                                   // 0x80156AAC
void fn_80156ABC(void);                                         // 0x80156ABC
u8*  fn_80156AC0(s32 a);                                        // 0x80156AC0
void fn_80156CD8(void);                                         // 0x80156CD8 (empty)
void fn_80156CDC(s32 idx, s32 flag, s32 c, u32** out);          // 0x80156CDC
void fn_80156F3C(s32 idx);                                      // 0x80156F3C (empty)
s32  fn_8016195C(void* p, s32 a, s32 b, void* c);               // 0x8016195C

extern u32 lbl_80347E68[];          // 0x80347E68 (CRC32 table)
extern s32 lbl_80376008[];          // 0x80376008 (random generator index table)
extern s32 lbl_80376084[];          // 0x80376084 (random generator index table)
extern u8  lbl_8040E5F0[];          // 0x8040E5F0 (frame effects object)

typedef s32 (*TrajectoryFn)(f32* in, f32* outPos, f32* outScale);

// 0x80146328
// Seeds a 31-word lagged-Fibonacci generator state (index at +0x7C) from a 32-bit seed.
void fn_80146328(u32* arg0, u32 arg1) {
    s32 i;
    if (lbl_80376008[0] == -1) {
        for (i = 0; i < 31; i++) {
            lbl_80376008[i] = (i + 1) % 31;
            lbl_80376084[i] = (i + 0x18) % 31;
        }
    }
    u32 seed = arg1 ^ 0xBA530000u ^ 0xBA11u;
    u8 k = (u8)(seed ^ (seed >> 8) ^ ((seed >> 24) ^ (seed >> 16)));
    for (i = 0; i < 31; i++) {
        arg0[i] = lbl_80347E68[k];
        k++;
    }
    arg0[0] ^= seed;
    for (i = 1; i < 31; i++) {
        arg0[i] ^= arg0[i - 1];
    }
    *(s32*)((u8*)arg0 + 0x7C) = 0;
    for (i = 0; i < 31; i++) {
        s32 cur = *(s32*)((u8*)arg0 + 0x7C);
        s32 a = lbl_80376084[cur];
        s32 b = lbl_80376008[cur];
        *(s32*)((u8*)arg0 + 0x7C) = b;
        arg0[b] += arg0[a];
    }
}

// 0x8015EF64
// Samples the 48x36 frame buffer (scaled by +0xAC/+0xB0), computes average luma and
// stores pow(luma, +0xC4) + +0xC8 at +0x154.
void fn_8015EF64(u8* arg0) {
    s32 width = 0;
    s32 w = (s32)(0.5f + 48.0f * *(f32*)(arg0 + 0xAC));
    s32 h = (s32)(0.5f + 36.0f * *(f32*)(arg0 + 0xB0));
    s32 sumB = 0;
    s32 sumG = 0;
    s32 sumR = 0;
    s32 y;
    u8* row = fn_80156AAC(&width) + ((((0x30 - w) >> 1) + ((0x24 - h) >> 1) * width) * 4);
    for (y = 0; y < h; y++) {
        s32 x;
        u8* px = row;
        for (x = 0; x < w; x++) {
            sumG += px[1];
            sumR += px[2];
            sumB += px[3];
            px += 4;
        }
        row += width * 4;
    }
    fn_80156ABC();
    f32 lum = (0.11f * (f32)sumB + (0.3f * (f32)sumG + 0.59f * (f32)sumR)) / (255.0f * (f32)(w * h));
    *(f32*)(arg0 + 0x154) = (f32)fn_8033F20C((f64)lum, (f64) * (f32*)(arg0 + 0xC4)) + *(f32*)(arg0 + 0xC8);
}

// 0x80160054
// Rebuilds three 1024-entry gamma tables (+0x15C, +0x55C, +0x95C) when the gamma at +0xA4 changes.
void fn_80160054(u8* arg0) {
    f32 gamma = *(f32*)(arg0 + 0xA4);
    if (*(f32*)(arg0 + 0x158) != gamma) {
        s32 i;
        *(f32*)(arg0 + 0x158) = gamma;
        for (i = 0; i < 0x400; i++) {
            f32 e = 0.8f * *(f32*)(arg0 + 0xA4);
            f32 t = (f32)i / 1023.0f;
            u8* p = arg0 + i;
            *(u8*)(p + 0x15C) = (u8)(s32)(0.5f + 255.0f * (f32)fn_8033F20C((f64)t, (f64)e));
            *(u8*)(p + 0x55C) = (u8)(s32)(0.5f + 255.0f * (f32)fn_8033F20C((f64)t, (f64)e));
            *(u8*)(p + 0x95C) = (u8)(s32)(0.5f + 255.0f * (f32)fn_8033F20C((f64)t, (f64)e));
        }
    }
}

// 0x801606A4
// Runs the trajectory callback at +0x388 for the event and spawns a frame effect (kind 9).
void fn_801606A4(u8* arg0, u8* arg1) {
    TrajectoryFn fn = *(TrajectoryFn*)(arg0 + 0x388);
    if (fn != NULL
        && (*(s16*)(arg1 + 0x4) != 1 || *(s32*)(arg1 + 0xC) != 0
            || *(s32*)(arg0 + 0x384) % 8 == *(s32*)(arg1 + 0x0) % 8)) {
        f32 in[3];
        f32 out[3];
        f32 eff[3];
        f32 scale;
        *(s32*)(arg1 + 0xC) = 0;
        in[0] = *(f32*)(arg1 + 0x10);
        in[1] = *(f32*)(arg1 + 0x14);
        in[2] = *(f32*)(arg1 + 0x18);
        if (fn(in, out, &scale) != 0) {
            f32 v;
            eff[0] = out[0];
            eff[1] = out[1];
            v = 0.001f * (scale * *(f32*)(arg1 + 0x8));
            if (*(s16*)(arg1 + 0x4) == 1) {
                v *= 2.0f * (2.3283064e-10f * (f32)RandomU32() - 0.5f);
            }
            eff[2] = v;
            fn_8016195C(lbl_8040E5F0, 9, 0, eff);
            *(s32*)(arg1 + 0xC) = 1;
        }
    }
}

// 0x80162E4C
// Fills a 576-word buffer with a white color whose alpha is fade * level, then marks it used.
void fn_80162E4C(u8* arg0, u8* arg1) {
    f32 fade = *(f32*)(arg1 + 0xC);
    if (fade > 0.0f) {
        f32 a = fade * *(f32*)(arg1 + 0x4);
        s32 idx = *(s32*)(arg1 + 0x0) - 1;
        u32* buf = NULL;
        u8* slot = arg0 + idx * 4;
        u32 color = 0xFFFFFFu | ((u32)(s32)(255.0f * a) << 24);
        s32 i;
        fn_80156CDC(idx, *(s32*)(slot + 0x3608) == 0, 0, &buf);
        for (i = 0; i < 0x24 * 16; i++) {
            buf[i] = color;
        }
        fn_80156F3C(idx);
        *(s32*)(slot + 0x3608) = 1;
    }
}

// 0x80163778
// Fade state machine (1 -> 2 ramp up, 2 -> 3 on flag, 3 ramp down -> 4), consuming the delta.
void fn_80163778(u8* arg0, u8* arg1, s32* arg2, f32* arg3, s32 arg4) {
    switch (*arg2) {
    case 1:
        *arg2 = 2;
        return;
    case 2: {
        f32 v;
        f32 t;
        if (arg4 != 0) {
            *arg2 = 3;
            return;
        }
        v = *(f32*)(arg1 + 0x10) + *arg3;
        *(f32*)(arg1 + 0x10) = v;
        if (v >= 1.0f) v = 1.0f;
        *(f32*)(arg1 + 0x10) = v;
        t = *(f32*)(arg1 + 0xC) + *arg3 * *(f32*)(arg1 + 0x0);
        *(f32*)(arg1 + 0xC) = t;
        *(f32*)(arg1 + 0xC) = t - (f32)(s32)t;
        *arg3 = 0.0f;
        return;
    }
    case 3:
        *(f32*)(arg1 + 0x10) = *(f32*)(arg1 + 0x10) - *arg3;
        *(f32*)(arg1 + 0xC) = *(f32*)(arg1 + 0xC) + *arg3 * *(f32*)(arg1 + 0x0);
        *arg3 = 0.0f;
        if (*(f32*)(arg1 + 0x10) <= 0.0f) {
            *arg2 = 4;
        }
        return;
    }
}

// 0x80163890
// Animates the 18x32 grid of points (x, y, w): each cell's w grows by a wave offset and
// x/y are rescaled to keep their ratio to w; alternates sign in a checkerboard.
void fn_80163890(u8* arg0, u8* arg1) {
    f32* p = (f32*)fn_80156AC0(*(s32*)(arg0 + 0x3604) == 0);
    f32 ang;
    f32 c;
    f32 s;
    f32 even;
    f32 odd;
    s32 i;
    *(s32*)(arg0 + 0x3604) = 1;
    ang = 1.5707964f * (4.0f * *(f32*)(arg1 + 0xC));
    c = cosf(ang);
    s = sinf(ang);
    {
        f32 amp = *(f32*)(arg1 + 0x8);
        f32 base = *(f32*)(arg1 + 0x4);
        f32 k = *(f32*)(arg1 + 0x10);
        even = (base + s * amp) * k;
        odd = (-base - c * amp) * k;
    }
    for (i = 0; i < 0x12; i++) {
        s32 j = 0;
        s32 n;
        for (n = 0; n < 0x10; n++) {
            f32 x = p[0] / p[2];
            f32 y = p[1] / p[2];
            f32 w = p[2] + (((j ^ i) & 1) ? odd : even);
            j++;
            p[0] = x * w;
            p[1] = y * w;
            p[2] = w;
            x = p[3] / p[5];
            y = p[4] / p[5];
            w = p[5] + (((j ^ i) & 1) ? odd : even);
            j++;
            p[3] = x * w;
            p[4] = y * w;
            p[5] = w;
            p += 6;
        }
    }
    fn_80156CD8();
}

}  // extern "C"
