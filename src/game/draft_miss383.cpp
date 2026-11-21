// Effect/trail manager helpers around 0x8016xxxx: slot dispatcher, wave height smoothing,
// wave normal scaling, wave mesh draw, ray vs box and sphere vs box tests.
#include <math.h>

#include "types.h"
#include "revolution.h"

extern "C" {
void fn_8014E3EC(s32 n);                                       // 0x8014E3EC
void  fn_80152620(s32 a, s32 b, u8* quad);                      // 0x80152620
u8*  fn_80156AC0(s32 a);                                       // 0x80156AC0
void fn_80156CD8(void);                                        // 0x80156CD8 (empty)

// Per-slot state handlers called by the dispatcher (all tail calls with the arguments shifted
// down one register: slot, state, dt, flag).
void fn_80162D70(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x80162D70
void fn_8016633C(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x8016633C
void fn_8016891C(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x8016891C
void fn_80166510(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x80166510
void fn_8016835C(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x8016835C
void fn_80163778(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x80163778
void fn_8016811C(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x8016811C
void fn_80166040(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x80166040
void fn_80163010(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x80163010
void fn_8016344C(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x8016344C
void fn_801691E4(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x801691E4
void fn_80168D64(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x80168D64
void fn_80166EEC(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x80166EEC
void fn_80164864(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x80164864
void fn_80163A4C(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x80163A4C
void fn_80165DB4(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x80165DB4
void fn_8016535C(u8* mgr, u8* slot, u8* state, f32* dt, u32 b);   // 0x8016535C
void fn_80166BB4(u8* mgr, u8* slot, s32* state, f32* dt, u32 b);   // 0x80166BB4

extern s32 lbl_805FB4C4;    // 0x805FB4C4 (temp buffer offset of the free area)
extern s32 lbl_805FB4CC;    // 0x805FB4CC (temp buffer bytes left)
extern u8* lbl_805FB4D0;    // 0x805FB4D0 (temp buffer base)
extern f32 lbl_805FF2B4;    // 0x805FF2B4 (0.75)
extern f32 lbl_805FF2C4;    // 0x805FF2C4 (1.0)
extern f32 lbl_805FF328;    // 0x805FF328 (-1.0)
extern f32 lbl_805FF350;    // 0x805FF350 (fixed step, 1/30)
extern f32 lbl_805FF360;    // 0x805FF360 (0.015)
extern f32 lbl_805FF5C0;    // 0x805FF5C0 (0.0)
extern f32 lbl_805FF5C4;    // 0x805FF5C4 (1.0)

// 0x8016120C
// Dispatches slot type 0..0x14 to its handler (types 0-2 share one, 5-6 share one);
// anything above 0x14 does nothing.
void fn_8016120C(u8* mgr, s32 type, u8* slot, u8* state, f32* dt, u32 b) {
    switch ((u32)type) {
    case 0:
    case 1:
    case 2:  return fn_80162D70(mgr, slot, (s32*)state, dt, b);
    case 3:  return fn_8016633C(mgr, slot, state, dt, b);
    case 4:  return fn_8016891C(mgr, slot, state, dt, b);
    case 5:
    case 6:  return fn_80166510(mgr, slot, (s32*)state, dt, b);
    case 7:  return fn_8016835C(mgr, slot, (s32*)state, dt, b);
    case 8:  return fn_80163778(mgr, slot, (s32*)state, dt, b);
    case 9:  return fn_8016811C(mgr, slot, state, dt, b);
    case 10: return fn_80166040(mgr, slot, (s32*)state, dt, b);
    case 11: return fn_80163010(mgr, slot, (s32*)state, dt, b);
    case 12: return fn_8016344C(mgr, slot, (s32*)state, dt, b);
    case 13: return fn_801691E4(mgr, slot, (s32*)state, dt, b);
    case 14: return fn_80168D64(mgr, slot, state, dt, b);
    case 15: return fn_80166EEC(mgr, slot, (s32*)state, dt, b);
    case 16: return fn_80164864(mgr, slot, state, dt, b);
    case 17: return fn_80163A4C(mgr, slot, state, dt, b);
    case 18: return fn_80165DB4(mgr, slot, state, dt, b);
    case 19: return fn_8016535C(mgr, slot, state, dt, b);
    case 20: return fn_80166BB4(mgr, slot, (s32*)state, dt, b);
    default: return;
    }
}

// 0x80161464
// One step of the water height-field simulation: each cell's front-buffer neighbours (and
// itself) are summed into the back buffer (>>2 minus old value), then the buffers swap and
// everything is damped by 0.75. Sets the "settled" flag at +0x40BC when all heights are zero
// and decreases the timer at +0x40C0 by one fixed step.
void fn_80161464(u8* arg0) {
    s32 i, j, k;
    s32 acc = 0;
    s32 cur;
    f32 damp;

    for (i = 1; i < 0x13; i++) {
        u8* row = arg0 + i * 0x44;
        s32 off = 2;
        for (j = 0; j < 0x10; j++) {
            u8* p;
            u8* q;
            s32 sum;

            cur = *(s32*)(arg0 + 0x40B8);
            p = row + off + cur * 0x550;
            q = row + off + (cur ^ 1) * 0x550;
            off += 2;
            sum = *(s16*)(p + 0x3616) + *(s16*)(p + 0x361A) + *(s16*)(p + 0x35D4) + *(s16*)(p + 0x365C);
            *(s16*)(q + 0x3618) = (s16)((sum >> 2) - *(s16*)(q + 0x3618));
            acc |= sum;

            cur = *(s32*)(arg0 + 0x40B8);
            p = row + off + cur * 0x550;
            q = row + off + (cur ^ 1) * 0x550;
            sum = *(s16*)(p + 0x3616) + *(s16*)(p + 0x361A) + *(s16*)(p + 0x35D4) + *(s16*)(p + 0x365C);
            *(s16*)(q + 0x3618) = (s16)((sum >> 2) - *(s16*)(q + 0x3618));
            acc |= sum;
            off += 2;
        }
    }

    damp = lbl_805FF2B4;
    *(s32*)(arg0 + 0x40B8) ^= 1;

    for (i = 0; i < 0x14; i++) {
        u8* row = arg0 + i * 0x44;
        for (j = 0; j < 4; j++) {
            u8* base = row + j * 0x10;
            for (k = 0; k < 8; k++) {
                u8* p = base + *(s32*)(arg0 + 0x40B8) * 0x550;
                s32 v = (s32)((f32)*(s16*)(p + 0x3618 + k * 2) * damp);
                *(s16*)(p + 0x3618 + k * 2) = (s16)v;
                acc |= v;
            }
        }
        {
            u8* tail = row + 0x40 + *(s32*)(arg0 + 0x40B8) * 0x550;
            s32 v;

            v = (s32)((f32)*(s16*)(tail + 0x3618) * damp);
            *(s16*)(tail + 0x3618) = (s16)v;
            acc |= v;
            v = (s32)((f32)*(s16*)(tail + 0x361A) * damp);
            *(s16*)(tail + 0x361A) = (s16)v;
            acc |= v;
        }
    }

    *(s32*)(arg0 + 0x40BC) = (acc == 0);
    *(f32*)(arg0 + 0x40C0) = *(f32*)(arg0 + 0x40C0) - lbl_805FF350;
}

// 0x80161810
// Raises the wave mesh vertices (0x12 rows of 0x20 points, xyz+scale from the temp buffer
// returned by fn_80156AC0) by sqrt(height * 0.015) for every cell with positive height.
void fn_80161810(u8* arg0) {
    f32* v;
    s32 i, j;

    v = (f32*)fn_80156AC0(*(s32*)(arg0 + 0x3604) == 0);
    *(s32*)(arg0 + 0x3604) = 1;
    for (i = 0; i < 0x12; i++) {
        for (j = 0; j < 0x20; j++) {
            s16 h = *(s16*)(arg0 + i * 0x44 + j * 2 + *(s32*)(arg0 + 0x40B8) * 0x550 + 0x365E);
            if (h > 0) {
                f32 w = v[2];
                f32 x = v[0] / w;
                f32 y = v[1] / w;
                f32 add = (f32)h * lbl_805FF360;
                f32 s;

                if (add <= 0.0f) {
                    add = 0.0f;
                } else {
                    add = sqrtf(add);
                }
                s = w + add;
                v[0] = x * s;
                v[1] = y * s;
                v[2] = s;
            }
            v += 3;
        }
    }
    fn_80156CD8();
}

// Writes one 0x20-byte pair of vertices: color word (0xFF000000 as stored, then the byte-reversed
// 0x000000FF) at +0 and +0x10.
static inline void WaveColors(u8* v) {
    *(u32*)(v + 0x0) = 0xFF000000u;
    *(u32*)(v + 0x0) = 0x000000FFu;
    *(u32*)(v + 0x10) = 0xFF000000u;
    *(u32*)(v + 0x10) = 0x000000FFu;
}

// 0x80161BAC
// Draws the four edge strips of the wave mesh border into the temp buffer (each a list of 0x20
// or 0x12 vertex pairs from the xyz+w points of fn_80156AC0), submitting each with
// fn_80152620(0x98, n, buf), then closes the GX state bracket (0x61 register writes).
void fn_80161BAC(u8* arg0) {
    u8* pts;
    u8* out;
    u8* src;
    s32 i;

    (void)arg0;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE000001;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x41000000;

    pts = fn_80156AC0(0);

    // Strip 1: 0x20 points from the start.
    if (lbl_805FB4CC < 0x400) {
        fn_8014E3EC(0x400);
    }
    out = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB4C4 += 0x400;
    lbl_805FB4CC -= 0x400;
    src = pts;
    for (i = 0; i < 0x20; i++) {
        f32 x = *(f32*)(src + 0) / *(f32*)(src + 8);
        f32 y = *(f32*)(src + 4) / *(f32*)(src + 8);
        f32 e = (i == 0) ? lbl_805FF328 : (i == 0x1F) ? lbl_805FF2C4 : x;
        u8* v = out + i * 0x20;

        src += 0xC;
        *(f32*)(v + 0x4) = e;
        *(f32*)(v + 0x8) = lbl_805FF2C4;
        *(f32*)(v + 0xC) = lbl_805FF328;
        *(f32*)(v + 0x14) = x;
        *(f32*)(v + 0x18) = y;
        *(f32*)(v + 0x1C) = lbl_805FF328;
        WaveColors(v);
    }
    fn_80152620(0x98, 0x40, out);

    // Strip 2: 0x12 points, stride 0x180, from the start.
    if (lbl_805FB4CC < 0x240) {
        fn_8014E3EC(0x240);
    }
    out = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB4C4 += 0x240;
    lbl_805FB4CC -= 0x240;
    src = pts;
    for (i = 0; i < 0x12; i++) {
        f32 x = *(f32*)(src + 0) / *(f32*)(src + 8);
        f32 y = *(f32*)(src + 4) / *(f32*)(src + 8);
        f32 e = (i == 0) ? lbl_805FF2C4 : (i == 0x11) ? lbl_805FF328 : y;
        u8* v = out + i * 0x20;

        src += 0x180;
        *(f32*)(v + 0x14) = lbl_805FF328;
        *(f32*)(v + 0x18) = e;
        *(f32*)(v + 0x1C) = lbl_805FF328;
        *(f32*)(v + 0x4) = x;
        *(f32*)(v + 0x8) = y;
        *(f32*)(v + 0xC) = lbl_805FF328;
        WaveColors(v);
    }
    fn_80152620(0x98, 0x24, out);

    // Strip 3: 0x12 points, stride 0x180, starting at +0x174.
    if (lbl_805FB4CC < 0x240) {
        fn_8014E3EC(0x240);
    }
    out = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB4C4 += 0x240;
    lbl_805FB4CC -= 0x240;
    src = pts + 0x174;
    for (i = 0; i < 0x12; i++) {
        f32 x = *(f32*)(src + 0) / *(f32*)(src + 8);
        f32 y = *(f32*)(src + 4) / *(f32*)(src + 8);
        f32 e = (i == 0) ? lbl_805FF2C4 : (i == 0x11) ? lbl_805FF328 : y;
        u8* v = out + i * 0x20;

        src += 0x180;
        *(f32*)(v + 0x4) = lbl_805FF2C4;
        *(f32*)(v + 0x8) = e;
        *(f32*)(v + 0xC) = lbl_805FF328;
        *(f32*)(v + 0x14) = x;
        *(f32*)(v + 0x18) = y;
        *(f32*)(v + 0x1C) = lbl_805FF328;
        WaveColors(v);
    }
    fn_80152620(0x98, 0x24, out);

    // Strip 4: 0x20 points starting at +0x1980.
    if (lbl_805FB4CC < 0x400) {
        fn_8014E3EC(0x400);
    }
    out = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB4C4 += 0x400;
    lbl_805FB4CC -= 0x400;
    src = pts + 0x1980;
    for (i = 0; i < 0x20; i++) {
        f32 x = *(f32*)(src + 0) / *(f32*)(src + 8);
        f32 y = *(f32*)(src + 4) / *(f32*)(src + 8);
        f32 e = (i == 0) ? lbl_805FF328 : (i == 0x1F) ? lbl_805FF2C4 : x;
        u8* v = out + i * 0x20;

        src += 0xC;
        *(f32*)(v + 0x14) = e;
        *(f32*)(v + 0x18) = lbl_805FF328;
        *(f32*)(v + 0x1C) = lbl_805FF328;
        *(f32*)(v + 0x4) = x;
        *(f32*)(v + 0x8) = y;
        *(f32*)(v + 0xC) = lbl_805FF328;
        WaveColors(v);
    }
    fn_80152620(0x98, 0x40, out);

    fn_80156CD8();
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE000001;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x41000001;
}

// 0x8016B898
// Slab test of a ray (origin, direction) against a box (min, max): returns 1 when the ray
// overlaps the box within t in [0, 1].
s32 fn_8016B898(f32* arg0, f32* arg1, f32* arg2, f32* arg3) {
    s32 i;

    for (i = 0; i < 3; i++) {
        f32 d = arg1[i];
        f32 o = arg0[i];

        if (lbl_805FF5C0 == d) {
            if (o < arg2[i]) {
                return 0;
            }
            if (o > arg3[i]) {
                return 0;
            }
        } else {
            f32 inv = lbl_805FF5C4 / d;
            f32 a = (arg3[i] - o) * inv;
            f32 b = (arg2[i] - o) * inv;
            f32 tmin = (b < a) ? b : a;
            f32 tmax = (b > a) ? b : a;

            if (tmin > lbl_805FF5C4) {
                return 0;
            }
            if (tmax < lbl_805FF5C0) {
                return 0;
            }
        }
    }
    return 1;
}

// 0x8016CBC4
// Sphere (centre at +0, radius at +0xC) vs box (arg1 = min corner side, arg2 = max corner side):
// 1 when the squared distance from the centre to the box does not exceed radius squared.
s32 fn_8016CBC4(u8* arg0, u8* arg1, u8* arg2) {
    f32 zero = lbl_805FF5C0;
    f32 cx = *(f32*)(arg0 + 0x0);
    f32 cy = *(f32*)(arg0 + 0x4);
    f32 cz = *(f32*)(arg0 + 0x8);
    f32 ax = *(f32*)(arg1 + 0x0) - cx;
    f32 ay = *(f32*)(arg1 + 0x4) - cy;
    f32 az = *(f32*)(arg1 + 0x8) - cz;
    f32 bx = cx - *(f32*)(arg2 + 0x0);
    f32 by = cy - *(f32*)(arg2 + 0x4);
    f32 bz = cz - *(f32*)(arg2 + 0x8);
    f32 r;
    f32 d1;
    f32 d2;

    if (!(ax > zero)) ax = zero;
    if (!(ay > zero)) ay = zero;
    if (!(az > zero)) az = zero;
    if (!(bx > zero)) bx = zero;
    if (!(by > zero)) by = zero;
    if (!(bz > zero)) bz = zero;

    r = *(f32*)(arg0 + 0xC);
    d1 = (az * az) + ((ax * ax) + (ay * ay));
    d2 = (bz * bz) + ((bx * bx) + (by * by));
    return (r * r) >= (d1 + d2);
}

}  // extern "C"
