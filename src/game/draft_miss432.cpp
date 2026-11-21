// Flicker / noise effect handlers in 0x8016xxxx (fn_8016535C, fn_8016599C).
#include "types.h"
#include <math.h>

extern "C" {
u32 RandomU32(void);  // 0x80143460

// Random float in [0,1).
static inline f32 Rand01(void) { return 2.3283064e-10f * (f32)RandomU32(); }

// Advance the flicker timer by dt; each time it passes the period, re-roll the
// random offset (+0x20) from the two probability/amplitude pairs, then wrap the timer.
static void FlickerTick(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x1C) + dt;
    *(f32*)(p + 0x1C) = t;
    if (t >= *(f32*)(p + 0x4)) {
        *(f32*)(p + 0x20) = 0.0f;
        if (Rand01() <= *(f32*)(p + 0x8)) {
            *(f32*)(p + 0x20) = *(f32*)(p + 0xC) * Rand01();
        }
        if (Rand01() <= *(f32*)(p + 0x10)) {
            *(f32*)(p + 0x20) = *(f32*)(p + 0x20) + *(f32*)(p + 0x14) * Rand01();
        }
        while (*(f32*)(p + 0x1C) >= *(f32*)(p + 0x4)) {
            *(f32*)(p + 0x1C) = *(f32*)(p + 0x1C) - *(f32*)(p + 0x4);
        }
    }
}

// 0x8016535C
// Fade state machine (state 1 = fade in, 2 = hold, 3 = fade out, 4 = done) driving the flicker.
// arg1 = params/data, arg2 = state, arg3 = pending dt (consumed), arg4 = stop request.
void fn_8016535C(u8* arg0, u8* arg1, s32* arg2, f32* arg3, s32 arg4) {
    switch (*arg2) {
    case 1:
        if (arg4 != 0) {
            *arg2 = 3;
            return;
        }
        FlickerTick(arg1, *arg3);
        *(f32*)(arg1 + 0x18) = *(f32*)(arg1 + 0x18) + *arg3 * 1.0f;
        *arg3 = 0.0f;
        if (*(f32*)(arg1 + 0x18) >= *(f32*)(arg1 + 0x0)) {
            *(f32*)(arg1 + 0x18) = *(f32*)(arg1 + 0x0);
            *arg2 = 2;
        }
        return;
    case 2:
        if (arg4 != 0) {
            *arg2 = 3;
            return;
        }
        FlickerTick(arg1, *arg3);
        *arg3 = 0.0f;
        return;
    case 3:
        *(f32*)(arg1 + 0x18) = *(f32*)(arg1 + 0x18) - *arg3 * 1.0f;
        FlickerTick(arg1, *arg3);
        *arg3 = 0.0f;
        if (*(f32*)(arg1 + 0x18) <= 0.0f) {
            *arg2 = 4;
        }
        break;
    }
}

// 0x8016599C
// Random-walk wobble: steps a mode machine (+0x18: 0 pick, 1 ease, 2 random step, 3 decay)
// once per period (+0xC) of accumulated time (+0x14), then moves +0x24 toward +0x28.
void fn_8016599C(u8* arg0, u8* arg1, f32 dt) {
    *(f32*)(arg1 + 0x14) = *(f32*)(arg1 + 0x14) + dt;
    while (*(f32*)(arg1 + 0x14) >= *(f32*)(arg1 + 0xC)) {
        do {
            switch (*(s32*)(arg1 + 0x18)) {
            case 0:
                switch (RandomU32() & 2) {
                case 0:
                    *(s32*)(arg1 + 0x18) = 1;
                    *(f32*)(arg1 + 0x1C) = (2.0f * Rand01() - 1.0f) * 1.1f;
                    break;
                case 1: {
                    *(s32*)(arg1 + 0x18) = 2;
                    f32 v = *(f32*)(arg1 + 0x20) + Rand01() - 0.5f;
                    if (v < -1.1f) {
                        v = -1.1f;
                    } else if (v > 1.1f) {
                        v = 1.1f;
                    }
                    *(f32*)(arg1 + 0x1C) = v;
                    break;
                }
                case 2:
                    *(s32*)(arg1 + 0x18) = 3;
                    *(f32*)(arg1 + 0x1C) = 1.5f * Rand01();
                    break;
                }
                break;
            case 1: {
                f32 target = *(f32*)(arg1 + 0x1C);
                f32 v = 0.25f * target + 0.75f * *(f32*)(arg1 + 0x20);
                *(f32*)(arg1 + 0x20) = v;
                if (fabsf(v - target) < 0.01f) {
                    *(f32*)(arg1 + 0x20) = target;
                    *(s32*)(arg1 + 0x18) = 0;
                }
                break;
            }
            case 2: {
                f32 target = *(f32*)(arg1 + 0x1C);
                f32 cur = *(f32*)(arg1 + 0x20);
                f32 step = (0.15f + *(f32*)(arg1 + 0x8) * Rand01()) * *(f32*)(arg1 + 0xC);
                if (target > cur) {
                    cur = cur + step;
                    *(f32*)(arg1 + 0x20) = cur;
                    if (cur >= target) {
                        *(s32*)(arg1 + 0x18) = 0;
                    }
                } else {
                    cur = cur - step;
                    *(f32*)(arg1 + 0x20) = cur;
                    if (cur <= target) {
                        *(s32*)(arg1 + 0x18) = 0;
                    }
                }
                break;
            }
            case 3: {
                f32 period = *(f32*)(arg1 + 0xC);
                f32 v = *(f32*)(arg1 + 0x1C) - period;
                f32 r = Rand01();
                *(f32*)(arg1 + 0x1C) = v;
                *(f32*)(arg1 + 0x20) = *(f32*)(arg1 + 0x20) + *(f32*)(arg1 + 0x8) * (r - 0.5f) * period;
                if (v <= 0.0f) {
                    *(s32*)(arg1 + 0x18) = 0;
                }
                break;
            }
            }
        } while (*(s32*)(arg1 + 0x18) == 0);
        f32 cur = *(f32*)(arg1 + 0x24);
        f32 dest = *(f32*)(arg1 + 0x28);
        f32 period = *(f32*)(arg1 + 0xC);
        f32 q = 0.25f * period;
        f32 d = fabsf(cur - dest);
        *(f32*)(arg1 + 0x14) = *(f32*)(arg1 + 0x14) - period;
        if (d <= q) {
            *(f32*)(arg1 + 0x24) = dest;
            *(f32*)(arg1 + 0x28) = Rand01();
        } else if (cur > dest) {
            *(f32*)(arg1 + 0x24) = cur - q;
        } else {
            *(f32*)(arg1 + 0x24) = cur + q;
        }
    }
}

}  // extern "C"
