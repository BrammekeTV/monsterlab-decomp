// Input-binding helpers in 0x8002xxxx (remote input sequencing, axis/button combiners).
#include "types.h"

extern "C" {

void fn_8001FCA8(u8* p);           // 0x8001FCA8 (input source update, result stored at +4)
f32  fn_8002030C(u8* p, f32 v);    // 0x8002030C (range filter: 0.0 if outside [+0x40, +0x44])

extern u8 lbl_805FC8BC[];          // 0x805FC8BC (0.0f)
extern u8 lbl_805FC8C8[];          // 0x805FC8C8 (0.0f)
extern u8 lbl_805FC8CC[];          // 0x805FC8CC (1.0f)
extern u8 lbl_805FC8D0[];          // 0x805FC8D0 (0.15f)

typedef void (*InputStepFn)(u8* step);

// 0x80020070
// Steps a sequence of input prompts (array of step objects at +0x10, count at +0). Advances to
// the next step when the current one finishes, wraps/resets when past the end.
void RemoteInputUpdate(u8* p, f32 dt) {
    f32 zero = *(f32*)lbl_805FC8BC;
    s32 count = *(s32*)(p + 0x0);
    if (count == 1) {
        *(f32*)(p + 0x38) = *(f32*)(p + 0x38) + dt;
        u8* step = *(u8**)(p + 0x10);
        (*(InputStepFn*)(*(u8**)(step + 0x4C) + 0xC))(step);
        step = *(u8**)(p + 0x10);
        f32 t = *(f32*)(p + 0x38);
        f32 tmin = *(f32*)(p + 0x3C);
        *(f32*)(p + 0xC) = *(f32*)(step + 0x4);
        if (t < tmin) {
            *(f32*)(p + 0xC) = zero;
        } else {
            f32 tmax = *(f32*)(p + 0x40);
            if (t > tmax && tmax > zero) {
                *(f32*)(p + 0x38) = zero;
                *(s32*)(p + 0x4) = 0;
                *(f32*)(p + 0xC) = zero;
                *(f32*)(p + 0x8) = zero;
                (*(InputStepFn*)(*(u8**)(step + 0x4C) + 0x10))(step);
            }
        }
        *(f32*)(p + 0x8) = *(f32*)(p + 0xC);
        return;
    }
    s32 idx = *(s32*)(p + 0x4);
    if (idx < count) {
        u8* step = *(u8**)(p + 0x10 + idx * 4);
        (*(InputStepFn*)(*(u8**)(step + 0x4C) + 0xC))(step);
        if (*(s32*)(p + 0x44) == 1) {
            *(f32*)(p + 0x38) = *(f32*)(p + 0x38) + dt;
        }
        if (*(s32*)(p + 0x44) == 0 && *(s32*)(p + 0x4) == 0 &&
            zero != *(f32*)(*(u8**)(p + 0x10) + 0x4) && zero == *(f32*)(p + 0x8)) {
            *(f32*)(p + 0x38) = zero;
            *(s32*)(p + 0x44) = 1;
            *(f32*)(p + 0x8) = *(f32*)(*(u8**)(p + 0x10) + 0x4);
        } else {
            idx = *(s32*)(p + 0x4);
            f32 cur = *(f32*)(*(u8**)(p + 0x10 + idx * 4) + 0x4);
            if (zero != cur) {
                *(f32*)(p + 0x8) = cur;
            } else if (zero == cur) {
                f32 prev = *(f32*)(p + 0x8);
                if (zero != prev) {
                    s32 n = *(s32*)(p + 0x0);
                    *(s32*)(p + 0x4) = idx + 1;
                    if (idx + 1 >= n) {
                        if (*(f32*)(p + 0x38) >= *(f32*)(p + 0x3C)) {
                            *(f32*)(p + 0xC) = prev;
                        }
                    }
                    idx = *(s32*)(p + 0x4);
                    if (idx < *(s32*)(p + 0x0)) {
                        step = *(u8**)(p + 0x10 + idx * 4);
                        (*(InputStepFn*)(*(u8**)(step + 0x4C) + 0x10))(step);
                    }
                    *(f32*)(p + 0x8) = zero;
                }
            }
        }
        f32 tmax = *(f32*)(p + 0x40);
        if (tmax > zero) {
            if (*(f32*)(p + 0x38) >= tmax) {
                *(f32*)(p + 0x38) = zero;
                *(s32*)(p + 0x44) = 0;
                *(s32*)(p + 0x4) = 0;
                *(f32*)(p + 0xC) = zero;
                *(f32*)(p + 0x8) = zero;
            }
        }
    } else {
        u8* step = *(u8**)(p + 0x10);
        (*(InputStepFn*)(*(u8**)(step + 0x4C) + 0x10))(step);
        *(s32*)(p + 0x44) = 0;
        *(s32*)(p + 0x4) = 0;
        *(f32*)(p + 0xC) = zero;
        RemoteInputUpdate(p, dt);
    }
}

// 0x800204D0
// Product of all sources (count at +8, 12-byte entries from +0xC); +4 = 1.0 on the frame the
// product becomes zero after being non-zero.
void fn_800204D0(u8* p) {
    *(f32*)(p + 0x0) = *(f32*)lbl_805FC8CC;
    for (s32 i = 0; i < *(s32*)(p + 0x8); i++) {
        fn_8001FCA8(p + 0xC + i * 0xC);
        *(f32*)(p + 0x0) = *(f32*)(p + 0x0) * *(f32*)(p + 0x10 + i * 0xC);
    }
    f32 zero = *(f32*)lbl_805FC8C8;
    if (zero == *(f32*)(p + 0x0) && zero != *(f32*)(p + 0x50)) {
        *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
    } else {
        *(f32*)(p + 0x4) = *(f32*)lbl_805FC8C8;
    }
    *(f32*)(p + 0x50) = *(f32*)(p + 0x0);
}

// 0x800205CC
// Product of all sources, then clamped/dead-zoned to the [+0x40, +0x44] magnitude range
// (+0x3C selects zeroing vs clamping); result copied to +4.
void fn_800205CC(u8* p) {
    *(f32*)(p + 0x0) = *(f32*)lbl_805FC8CC;
    for (s32 i = 0; i < *(s32*)(p + 0x8); i++) {
        fn_8001FCA8(p + 0xC + i * 0xC);
        *(f32*)(p + 0x0) = *(f32*)(p + 0x0) * *(f32*)(p + 0x10 + i * 0xC);
    }
    f32 zero = *(f32*)lbl_805FC8C8;
    if (zero != *(f32*)(p + 0x40) || zero != *(f32*)(p + 0x44)) {
        if (*(s32*)(p + 0x3C) != 0) {
            f32 v = *(f32*)(p + 0x0);
            if (v < zero) {
                if (v > -*(f32*)(p + 0x40)) *(f32*)(p + 0x0) = zero;
                if (*(f32*)(p + 0x0) < -*(f32*)(p + 0x44)) *(f32*)(p + 0x0) = *(f32*)lbl_805FC8C8;
            } else {
                if (v < *(f32*)(p + 0x40)) *(f32*)(p + 0x0) = zero;
                if (*(f32*)(p + 0x0) > *(f32*)(p + 0x44)) *(f32*)(p + 0x0) = *(f32*)lbl_805FC8C8;
            }
        } else {
            f32 v = *(f32*)(p + 0x0);
            if (v < zero) {
                f32 lo = -*(f32*)(p + 0x40);
                if (v > lo) *(f32*)(p + 0x0) = lo;
                lo = -*(f32*)(p + 0x44);
                if (*(f32*)(p + 0x0) < lo) *(f32*)(p + 0x0) = lo;
            } else {
                f32 hi = *(f32*)(p + 0x40);
                if (v < hi) *(f32*)(p + 0x0) = hi;
                hi = *(f32*)(p + 0x44);
                if (*(f32*)(p + 0x0) > hi) *(f32*)(p + 0x0) = hi;
            }
        }
    }
    *(f32*)(p + 0x4) = *(f32*)(p + 0x0);
}

// 0x800207A8
// Product of all sources; +4 = product on the frame it becomes non-zero (was zero before), else 0.
void fn_800207A8(u8* p) {
    *(f32*)(p + 0x0) = *(f32*)lbl_805FC8CC;
    for (s32 i = 0; i < *(s32*)(p + 0x8); i++) {
        fn_8001FCA8(p + 0xC + i * 0xC);
        *(f32*)(p + 0x0) = *(f32*)(p + 0x0) * *(f32*)(p + 0x10 + i * 0xC);
    }
    f32 zero = *(f32*)lbl_805FC8C8;
    f32 v = *(f32*)(p + 0x0);
    if (zero != v && zero == *(f32*)(p + 0x50)) {
        *(f32*)(p + 0x4) = v;
    } else {
        *(f32*)(p + 0x4) = *(f32*)lbl_805FC8C8;
    }
    *(f32*)(p + 0x50) = *(f32*)(p + 0x0);
}

// 0x800208C0
// Two-button toggle: tracks which of two sources (+0xC, +0x18) was pressed first (+0x50, -1 = none);
// +4 reports the first one's value once the second is pressed while the first is released.
void fn_800208C0(u8* p) {
    u8 pressed[2] = {0, 0};
    fn_8001FCA8(p + 0xC);
    fn_8001FCA8(p + 0x18);
    f32 zero = *(f32*)lbl_805FC8C8;
    if (zero != *(f32*)(p + 0x10) && zero == *(f32*)(p + 0x54)) pressed[0] = 1;
    zero = *(f32*)lbl_805FC8C8;
    if (zero != *(f32*)(p + 0x1C) && zero == *(f32*)(p + 0x58)) pressed[1] = 1;
    if (*(s32*)(p + 0x50) == -1) {
        if (*(f32*)(p + 0x10) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 0;
        } else if (*(f32*)(p + 0x1C) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 1;
        }
    } else {
        zero = *(f32*)lbl_805FC8C8;
        *(f32*)(p + 0x4) = zero;
        if (pressed[0] && pressed[1]) {
            *(f32*)(p + 0x54) = zero;
            *(s32*)(p + 0x50) = -1;
            *(f32*)(p + 0x58) = zero;
        } else {
            s32 cur = *(s32*)(p + 0x50);
            if (pressed[cur]) {
                s32 other = cur ^ 1;
                if (!pressed[other]) {
                    *(f32*)(p + 0x4) = *(f32*)(p + cur * 0xC + 0x10);
                    *(s32*)(p + 0x50) = other;
                }
            }
        }
    }
    *(f32*)(p + 0x54) = *(f32*)(p + 0x10);
    *(f32*)(p + 0x58) = *(f32*)(p + 0x1C);
}

// 0x80020A64
// Four-direction sequence input (sources at +0xC, +0x18, +0x24, +0x30 with values +0x10, +0x1C,
// +0x28, +0x34; current direction in +0x50, -1 = none). A press of the next direction in the
// rotation (1->3, 3->0, 0->2, 2->1) emits +4 = 1.0 and arms the +0x54 timer; any other press
// resets. +0x5C counts steps.
void fn_80020A64(u8* p, f32 dt) {
    *(f32*)(p + 0x0) = *(f32*)lbl_805FC8CC;
    f32 t = *(f32*)(p + 0x54) - dt;
    *(f32*)(p + 0x54) = t;
    if (t <= *(f32*)lbl_805FC8C8) {
        *(f32*)(p + 0x4) = *(f32*)lbl_805FC8C8;
        *(f32*)(p + 0x54) = *(f32*)lbl_805FC8C8;
    }
    *(f32*)(p + 0x58) = *(f32*)(p + 0x58) + dt;
    fn_8001FCA8(p + 0xC);
    fn_8001FCA8(p + 0x18);
    fn_8001FCA8(p + 0x24);
    fn_8001FCA8(p + 0x30);
    if (*(s32*)(p + 0x50) == -1) {
        f32 zero = *(f32*)lbl_805FC8C8;
        *(f32*)(p + 0x58) = zero;
        *(f32*)(p + 0x5C) = zero;
        if (fn_8002030C(p, *(f32*)(p + 0x10)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 0;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x1C)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 1;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x28)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 2;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x34)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 3;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
        }
    } else {
        if (fn_8002030C(p, *(f32*)(p + 0x34)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 1) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 3;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x10)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 3) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 0;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x28)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 0) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 2;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x1C)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 2) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 1;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
        }
    }
}

// 0x80020D88
// Same as fn_80020A64 with the opposite rotation (0->3, 2->0, 1->2, 3->1).
void fn_80020D88(u8* p, f32 dt) {
    *(f32*)(p + 0x0) = *(f32*)lbl_805FC8CC;
    f32 t = *(f32*)(p + 0x54) - dt;
    *(f32*)(p + 0x54) = t;
    if (t <= *(f32*)lbl_805FC8C8) {
        *(f32*)(p + 0x4) = *(f32*)lbl_805FC8C8;
        *(f32*)(p + 0x54) = *(f32*)lbl_805FC8C8;
    }
    *(f32*)(p + 0x58) = *(f32*)(p + 0x58) + dt;
    fn_8001FCA8(p + 0xC);
    fn_8001FCA8(p + 0x18);
    fn_8001FCA8(p + 0x24);
    fn_8001FCA8(p + 0x30);
    if (*(s32*)(p + 0x50) == -1) {
        f32 zero = *(f32*)lbl_805FC8C8;
        *(f32*)(p + 0x58) = zero;
        *(f32*)(p + 0x5C) = zero;
        if (fn_8002030C(p, *(f32*)(p + 0x10)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 0;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x1C)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 1;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x28)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 2;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x34)) > *(f32*)lbl_805FC8C8) {
            *(s32*)(p + 0x50) = 3;
            *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
        }
    } else {
        if (fn_8002030C(p, *(f32*)(p + 0x34)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 0) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 3;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x10)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 2) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 0;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x28)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 1) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 2;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
            return;
        }
        if (fn_8002030C(p, *(f32*)(p + 0x1C)) > *(f32*)lbl_805FC8C8) {
            if (*(s32*)(p + 0x50) == 3) {
                *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
                *(f32*)(p + 0x54) = *(f32*)lbl_805FC8D0;
                *(s32*)(p + 0x50) = 1;
                *(f32*)(p + 0x5C) = *(f32*)(p + 0x5C) + *(f32*)lbl_805FC8CC;
                return;
            }
            *(s32*)(p + 0x50) = -1;
        }
    }
}

// 0x80021088
// Sum of all sources; +4 = 1.0 if the sum is positive, else 0.0.
void fn_80021088(u8* p) {
    f32 sum = *(f32*)lbl_805FC8C8;
    *(f32*)(p + 0x4) = sum;
    for (s32 i = 0; i < *(s32*)(p + 0x8); i++) {
        fn_8001FCA8(p + 0xC + i * 0xC);
        sum += *(f32*)(p + 0x10 + i * 0xC);
    }
    if (sum > *(f32*)lbl_805FC8C8) {
        *(f32*)(p + 0x4) = *(f32*)lbl_805FC8CC;
    }
}

// 0x80021188
// Edge detect over all sources: +4 = 1.0 if any source went from positive (previous values at
// +0x50 + i*4) to exactly zero this frame; previous values are updated.
void fn_80021188(u8* p) {
    f32 zero = *(f32*)lbl_805FC8C8;
    f32 one = *(f32*)lbl_805FC8CC;
    *(f32*)(p + 0x4) = zero;
    for (s32 i = 0; i < *(s32*)(p + 0x8); i++) {
        fn_8001FCA8(p + 0xC + i * 0xC);
        if (*(f32*)(p + 0x50 + i * 4) > zero && zero == *(f32*)(p + 0x10 + i * 0xC)) {
            *(f32*)(p + 0x4) = one;
        }
        *(f32*)(p + 0x50 + i * 4) = *(f32*)(p + 0x10 + i * 0xC);
    }
}

}  // extern "C"
