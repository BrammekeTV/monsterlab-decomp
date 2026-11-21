// Effect/wave mesh helpers around 0x8016xxxx: noise texture fill and wave slot state handlers.
#include <math.h>

#include "types.h"

void fn_8015A2E8(u8* p, u32 unused4, u32 flush);             // 0x8015A2E8

extern "C" {
u32  RandomU32(void);                                        // 0x80143460
s32  fn_80156A88(void);                                      // 0x80156A88
u8*  fn_80156AC0(s32 a);                                     // 0x80156AC0
void fn_80156CD8(void);                                      // 0x80156CD8 (empty)
u8*  fn_8015A210(u8* p, s32 a, s32* out, s32 b);             // 0x8015A210

// 0x801646E8
// Fills a 32-row texture with random black/white bytes (0x00 or 0xFF, one per random bit).
void fn_801646E8(u8* arg0, u8* arg1) {
    s32 stride = 0;
    u8* tex = arg1 + (*(u32*)(arg1 + 0x24) << 5) + 0x28;
    u8* row = fn_8015A210(tex, 0, &stride, 6);
    s32 i;
    s32 k;

    for (i = 0; i < 0x20; i++) {
        u32 r = RandomU32();
        for (k = 0; k < 0x20; k++) {
            row[k] = (u8)-(s32)(r & 1);
            r >>= 1;
        }
        row += stride;
    }
    fn_8015A2E8(tex, 0, 1);
}

// 0x80166114
// Wave mesh (0x12 rows of 0x20 points) is scaled radially from the slot amplitude *slot.
void fn_80166114(u8* mgr, u8* slot) {
    f32* v;
    s32 i, j;

    if (*(f32*)slot > 0.0f) {
        v = (f32*)fn_80156AC0(*(s32*)(mgr + 0x3604) == 0);
        *(s32*)(mgr + 0x3604) = 1;
        for (i = 0; i < 0x12; i++) {
            f32 y = 1.0f + (f32)(-2 * i) / 17.0f;
            f32 y2 = y * y;
            for (j = 0; j < 0x20; j++) {
                f32 x = ((f32)(j * 2) / 31.0f - 1.0f) * 1.7777778f;
                f32 l2 = x * x + y2;
                f32 len;
                f32 a = *(f32*)slot;
                f32 w = v[2];
                f32 scale = 1.0f + 1.3f * a;

                if (l2 <= 0.0f) {
                    len = 0.0f;
                } else {
                    len = (f32)sqrt((f64)l2);  // original: frsqrte with two Newton steps
                }
                v[0] = v[0] / w * (1.0f / (1.0f + a * (len * 0.75f))) * scale * w;
                v[1] = v[1] / w * (1.0f / (1.0f + a * (len * 1.0f))) * scale * w;
                v += 3;
            }
        }
        fn_80156CD8();
    }
}

// 0x8016578C
// Shifts all wave mesh points in y by a per-frame step computed from the slot fields
// (-slot[0x20] * slot[0x18] / slot[0] / (0.5 * fn_80156A88())); does nothing for a zero step.
void fn_8016578C(u8* mgr, u8* slot) {
    f32 step;
    f32* v;
    s32 i, j;

    step = ((-*(f32*)(slot + 0x20) * *(f32*)(slot + 0x18)) / *(f32*)(slot + 0x0)) /
           (0.5f * (f32)fn_80156A88());
    if (step != 0.0f) {
        v = (f32*)fn_80156AC0(*(s32*)(mgr + 0x3604) == 0);
        *(s32*)(mgr + 0x3604) = 1;
        for (i = 0; i < 0x12; i++) {
            for (j = 0; j < 0x20; j++) {
                f32 w = v[2];
                f32 x = v[0] / w;
                f32 y = v[1] / w;
                v[0] = x * w;
                v[1] = (y + step) * w;
                v += 3;
            }
        }
        fn_80156CD8();
    }
}

// 0x8016811C
// Splats a bilinear height impulse (slot x,y in 0..1, strength slot[8] * 1000) into the four
// grid cells (s16 heights, 0x20 x 0x12) around the point, then ends the slot (state 4).
void fn_8016811C(u8* mgr, u8* slot, s32* state, f32* dt, u32 b) {
    f32 fx = 31.0f * *(f32*)(slot + 0x0);
    f32 fy = 17.0f * *(f32*)(slot + 0x4);
    s32 ix = (s32)fx;
    s32 iy = (s32)fy;

    if (ix >= -1 && iy >= -1 && ix < 0x20 && iy < 0x12) {
        s32 ix1 = ix + 1;
        s32 iy1 = iy + 1;
        f32 w = 1000.0f * *(f32*)(slot + 0x8);
        f32 a = (fx - (f32)ix) * w;
        f32 bb = fy - (f32)iy;
        f32 c = w - a;
        f32 d = bb * a;
        f32 e = bb * c;
        f32 f1 = a - d;
        f32 f0 = c - e;
        u8* bank = mgr + *(s32*)(mgr + 0x40B8) * 0x550;
        s16* p;

        if (ix >= 0 && ix < 0x20 && iy >= 0 && iy < 0x12) {
            p = (s16*)(bank + iy1 * 0x44 + ix1 * 2 + 0x3618);
            *p = (s16)(*p + (s32)f0);
        }
        if (ix1 >= 0 && ix1 < 0x20 && iy >= 0 && iy < 0x12) {
            p = (s16*)(bank + iy1 * 0x44 + (ix1 + 1) * 2 + 0x3618);
            *p = (s16)(*p + (s32)f1);
        }
        if (ix >= 0 && ix < 0x20 && iy1 >= 0 && iy1 < 0x12) {
            p = (s16*)(bank + (iy1 + 1) * 0x44 + (ix + 1) * 2 + 0x3618);
            *p = (s16)(*p + (s32)e);
        }
        if (ix1 >= 0 && ix1 < 0x20 && iy1 >= 0 && iy1 < 0x12) {
            p = (s16*)(bank + (iy1 + 1) * 0x44 + (ix1 + 1) * 2 + 0x3618);
            *p = (s16)(*p + (s32)d);
        }
        *(s32*)(mgr + 0x3614) = 1;
    }
    *state = 4;
}

// 0x80168D64
// Phase (slot[0x20], wraps at 2*pi) advances by dt * slot[0x10]; slot[0x1c] fades in (state 1,
// until 1.0 then state 2), holds in state 2, and fades out in state 3 (until 0.0 then state 4).
// A nonzero flag in states 1/2 starts the fade-out.
void fn_80168D64(u8* mgr, u8* slot, s32* state, f32* dt, u32 b) {
    f32 ph = *(f32*)(slot + 0x20) + *dt * *(f32*)(slot + 0x10);
    *(f32*)(slot + 0x20) = ph;
    if (ph >= 6.2831855f) {
        *(f32*)(slot + 0x20) = ph - 6.2831855f;
    }
    switch (*state) {
    case 1:
        if (b != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) + *dt * 1.0f;
        *dt = 0.0f;
        if (*(f32*)(slot + 0x1C) >= 1.0f) {
            *(f32*)(slot + 0x1C) = 1.0f;
            *state = 2;
        }
        return;
    case 2:
        if (b != 0) {
            *state = 3;
            return;
        }
        *dt = 0.0f;
        return;
    case 3:
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) - *dt * 1.0f;
        *dt = 0.0f;
        if (*(f32*)(slot + 0x1C) <= 0.0f) {
            *state = 4;
        }
        return;
    default:
        return;
    }
}

}  // extern "C"
