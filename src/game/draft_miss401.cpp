// Vector helpers (vec4 mul, min, max, to-int) and the per-slot float state handlers around
// 0x80162D70..0x801691E4 (accumulate / step / wait, used by the slot dispatcher).
#include "types.h"

extern "C++" f32 lbl_805FF380;      // 0x805FF380 (float constant)
extern "C++" f32 lbl_805FF384;      // 0x805FF384 (float constant)
extern "C++" f32 lbl_805FF390;      // 0x805FF390 (float constant)
extern "C++" f32 lbl_805FF394;      // 0x805FF394 (float constant)
extern "C++" f32 lbl_805FF3A0;      // 0x805FF3A0 (float constant)
extern "C++" f32 lbl_805FF3A4;      // 0x805FF3A4 (float constant)
extern "C++" f32 lbl_805FF490;      // 0x805FF490 (float constant)
extern "C++" f32 lbl_805FF494;      // 0x805FF494 (float constant)
extern "C++" f32 lbl_805FF4D0;      // 0x805FF4D0 (float constant)
extern "C++" f32 lbl_805FF4D4;      // 0x805FF4D4 (float constant)
extern "C++" f32 lbl_805FF4E8;      // 0x805FF4E8 (float constant)
extern "C++" f32 lbl_805FF4EC;      // 0x805FF4EC (float constant)
extern "C++" f32 lbl_805FF500;      // 0x805FF500 (float constant)
extern "C++" f32 lbl_805FF504;      // 0x805FF504 (float constant)
extern "C++" f32 lbl_805FF530;      // 0x805FF530 (float constant)
extern "C++" f32 lbl_805FF534;      // 0x805FF534 (float constant)
extern "C++" f32 lbl_805FF590;      // 0x805FF590 (float constant)
extern "C++" f32 lbl_805FF594;      // 0x805FF594 (float constant)

// min(a, b) as the PPC fcmpo/bge sequence gives it (b when a >= b; a when unordered).
static f32 VecMinF(f32 a, f32 b) {
    return (a >= b) ? b : a;
}

// max(a, b) as the PPC fcmpo/ble sequence gives it (b when a <= b; a when unordered).
static f32 VecMaxF(f32 a, f32 b) {
    return (a <= b) ? b : a;
}

extern "C" {

// 0x8015FD48
// out = (f32)v (signed int to float).
void fn_8015FD48(f32* out, s32 v) {
    *out = (f32)v;
}

// 0x8015FE20
// Component-wise vec4 multiply: out = a * b (all products are read before the stores).
void fn_8015FE20(u8* out, u8* a, u8* b) {
    f32 p0 = *(f32*)(a + 0x0) * *(f32*)(b + 0x0);
    f32 p1 = *(f32*)(a + 0x4) * *(f32*)(b + 0x4);
    f32 p2 = *(f32*)(a + 0x8) * *(f32*)(b + 0x8);
    f32 p3 = *(f32*)(a + 0xC) * *(f32*)(b + 0xC);
    *(f32*)(out + 0x0) = p0;
    *(f32*)(out + 0x4) = p1;
    *(f32*)(out + 0x8) = p2;
    *(f32*)(out + 0xC) = p3;
}

// 0x8015FE64
// Component-wise vec4 min: out = min(a, b).
void fn_8015FE64(u8* out, u8* a, u8* b) {
    *(f32*)(out + 0x0) = VecMinF(*(f32*)(a + 0x0), *(f32*)(b + 0x0));
    *(f32*)(out + 0x4) = VecMinF(*(f32*)(a + 0x4), *(f32*)(b + 0x4));
    *(f32*)(out + 0x8) = VecMinF(*(f32*)(a + 0x8), *(f32*)(b + 0x8));
    *(f32*)(out + 0xC) = VecMinF(*(f32*)(a + 0xC), *(f32*)(b + 0xC));
}

// 0x8015FF1C
// Component-wise vec4 max: out = max(a, b).
void fn_8015FF1C(u8* out, u8* a, u8* b) {
    *(f32*)(out + 0x0) = VecMaxF(*(f32*)(a + 0x0), *(f32*)(b + 0x0));
    *(f32*)(out + 0x4) = VecMaxF(*(f32*)(a + 0x4), *(f32*)(b + 0x4));
    *(f32*)(out + 0x8) = VecMaxF(*(f32*)(a + 0x8), *(f32*)(b + 0x8));
    *(f32*)(out + 0xC) = VecMaxF(*(f32*)(a + 0xC), *(f32*)(b + 0xC));
}

// 0x8015FF90
// Truncates the four floats of in to ints (fctiwz) and stores them to out.
void fn_8015FF90(u8* in, s32* out) {
    f32 f0 = *(f32*)(in + 0x0);
    f32 f1 = *(f32*)(in + 0x4);
    f32 f2 = *(f32*)(in + 0x8);
    f32 f3 = *(f32*)(in + 0xC);
    out[0] = (s32)f0;
    out[1] = (s32)f1;
    out[2] = (s32)f2;
    out[3] = (s32)f3;
}

// 0x80162D70
// Slot step handler: state 1 adds dt * slot[+8] into slot[+0xC] and ends at 1, state 2 holds,
// state 3 steps slot[+0xC] back to 0 and ends in state 4. The flag (nonzero) skips to state 3.
void fn_80162D70(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0xC) = *(f32*)(slot + 0xC) + *dt * *(f32*)(slot + 0x8);
        *dt = lbl_805FF380;
        if (*(f32*)(slot + 0xC) >= lbl_805FF384) {
            *(f32*)(slot + 0xC) = lbl_805FF384;
            *state = 2;
        }
        return;
    case 2:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *dt = lbl_805FF380;
        return;
    case 3:
        *(f32*)(slot + 0xC) = *(f32*)(slot + 0xC) - *dt * *(f32*)(slot + 0x8);
        *dt = lbl_805FF380;
        if (*(f32*)(slot + 0xC) <= lbl_805FF380) {
            *state = 4;
        }
        return;
    }
}

// 0x80163010
// Slot step handler on slot[+0x18] (mirrored to slot[+0x1C]): same state machine as 0x80162D70.
void fn_80163010(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) + *dt;
        *dt = lbl_805FF390;
        if (*(f32*)(slot + 0x18) >= lbl_805FF394) {
            *(f32*)(slot + 0x18) = lbl_805FF394;
            *state = 2;
        }
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x18);
        return;
    case 2:
        if (flag != 0) {
            *(f32*)(slot + 0x18) = lbl_805FF394;
            *(f32*)(slot + 0x1C) = lbl_805FF394;
            *state = 3;
        }
        *dt = lbl_805FF390;
        return;
    case 3:
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) - *dt;
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x18);
        *dt = lbl_805FF390;
        if (*(f32*)(slot + 0x18) <= lbl_805FF390) {
            *state = 4;
        }
        return;
    }
}

// 0x8016344C
// Slot step handler on slot[+0x14], dt divided by slot[+0] (up) or slot[+4] (down).
void fn_8016344C(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) + (*dt / *(f32*)(slot + 0x0));
        *dt = lbl_805FF3A0;
        if (*(f32*)(slot + 0x14) >= lbl_805FF3A4) {
            *(f32*)(slot + 0x14) = lbl_805FF3A4;
            *state = 2;
        }
        return;
    case 2:
        *state = 3;
        return;
    case 3:
        *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) - (*dt / *(f32*)(slot + 0x4));
        *dt = lbl_805FF3A0;
        if (*(f32*)(slot + 0x14) <= lbl_805FF3A0) {
            *state = 4;
        }
        return;
    }
}

// 0x80166040
// Slot step handler on the float at slot[+0], step dt * lbl_805FF494, ends at lbl_805FF494.
void fn_80166040(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x0) = *(f32*)(slot + 0x0) + *dt * lbl_805FF494;
        *dt = lbl_805FF490;
        if (*(f32*)(slot + 0x0) >= lbl_805FF494) {
            *(f32*)(slot + 0x0) = lbl_805FF494;
            *state = 2;
        }
        return;
    case 2:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *dt = lbl_805FF490;
        return;
    case 3:
        *(f32*)(slot + 0x0) = *(f32*)(slot + 0x0) - *dt * lbl_805FF494;
        *dt = lbl_805FF490;
        if (*(f32*)(slot + 0x0) <= lbl_805FF490) {
            *state = 4;
        }
        return;
    }
}

// 0x80166510
// Slot step handler on the float at slot[+0x20], same shape as 0x80166040 with lbl_805FF4D4.
void fn_80166510(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x20) = *(f32*)(slot + 0x20) + *dt * lbl_805FF4D4;
        *dt = lbl_805FF4D0;
        if (*(f32*)(slot + 0x20) >= lbl_805FF4D4) {
            *(f32*)(slot + 0x20) = lbl_805FF4D4;
            *state = 2;
        }
        return;
    case 2:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *dt = lbl_805FF4D0;
        return;
    case 3:
        *(f32*)(slot + 0x20) = *(f32*)(slot + 0x20) - *dt * lbl_805FF4D4;
        *dt = lbl_805FF4D0;
        if (*(f32*)(slot + 0x20) <= lbl_805FF4D0) {
            *state = 4;
        }
        return;
    }
}

// 0x80166BB4
// Slot handler that sets slot[+0x20] to lbl_805FF4EC (state 1) or lbl_805FF4E8 (state 3) and ends.
void fn_80166BB4(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x20) = lbl_805FF4EC;
        *dt = lbl_805FF4E8;
        *state = 2;
        return;
    case 2:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *dt = lbl_805FF4E8;
        return;
    case 3:
        *dt = lbl_805FF4E8;
        *(f32*)(slot + 0x20) = lbl_805FF4E8;
        *state = 4;
        return;
    }
}

// 0x80166EEC
// Slot step handler on slot[+0x18], capped at slot[+0] (up) and stepping down to lbl_805FF500.
void fn_80166EEC(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) + *dt * lbl_805FF504;
        *dt = lbl_805FF500;
        if (*(f32*)(slot + 0x18) >= *(f32*)(slot + 0x0)) {
            *(f32*)(slot + 0x18) = *(f32*)(slot + 0x0);
            *state = 2;
        }
        return;
    case 2:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *dt = lbl_805FF500;
        return;
    case 3:
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) - *dt * lbl_805FF504;
        *dt = lbl_805FF500;
        if (*(f32*)(slot + 0x18) <= lbl_805FF500) {
            *state = 4;
        }
        return;
    }
}

// 0x801691E4
// Slot step handler on slot[+0x18], dt divided by slot[+0] (up) or slot[+4] (down).
void fn_801691E4(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    switch (*state) {
    case 1:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) + (*dt / *(f32*)(slot + 0x0));
        *dt = lbl_805FF590;
        if (*(f32*)(slot + 0x18) >= lbl_805FF594) {
            *(f32*)(slot + 0x18) = lbl_805FF594;
            *state = 2;
        }
        return;
    case 2:
        if (flag != 0) {
            *state = 3;
            return;
        }
        *dt = lbl_805FF590;
        return;
    case 3:
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) - (*dt / *(f32*)(slot + 0x4));
        *dt = lbl_805FF590;
        if (*(f32*)(slot + 0x18) <= lbl_805FF590) {
            *state = 4;
        }
        return;
    }
}

// 0x8016835C
// Wrapping accumulator on the float at slot[+0]: state 2 adds dt * lbl_805FF534 and keeps the
// fractional part; state 3 adds the same and moves to state 4 once the value is positive.
void fn_8016835C(u8* mgr, u8* slot, s32* state, f32* dt, u32 flag) {
    f32 v;
    switch (*state) {
    case 1:
        *state = 2;
        return;
    case 2:
        if (flag != 0) {
            *state = 3;
            return;
        }
        v = *(f32*)(slot + 0x0) + *dt * lbl_805FF534;
        *(f32*)(slot + 0x0) = v;
        *(f32*)(slot + 0x0) = v - (f32)(s32)v;
        *dt = lbl_805FF530;
        return;
    case 3:
        *(f32*)(slot + 0x0) = *(f32*)(slot + 0x0) + *dt * lbl_805FF534;
        *dt = lbl_805FF530;
        if ((s32)*(f32*)(slot + 0x0) > 0) {
            *state = 4;
        }
        return;
    }
}

}  // extern "C"
