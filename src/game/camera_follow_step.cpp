// Camera follow state: the per-frame step (fn_800E15AC) and the smoothing of the output
// position (fn_800E1FE0). Original: 0x800E15AC, 0x800E1FE0 (auto_03_800E02A8_text.s)
#include "types.h"
#include <math.h>

extern "C" {
f64  fn_8033F1F8(f64 x);                                                      // 0x8033F1F8 (MSL asin)
f32  fn_800DB844(f32 a, f32 b);                                               // 0x800DB844 (angle difference b - a, wrapped)
f32  fn_801F5E94(f32 cur, f32 target, f32 lo, f32 hi, f32 range);             // 0x801F5E94
void fn_801F5F28(f32* out, const f32* cur, const f32* target, f32 lo, f32 hi, f32 range);  // 0x801F5F28
void fn_800E0688(u8* self, f32 dt);                                           // 0x800E0688 (not decompiled)
void fn_800E0C70(u8* self, f32 dt);                                           // 0x800E0C70 (not decompiled)
void fn_800E084C(u8* self);                                                   // 0x800E084C (camera_follow.cpp)
}

typedef void (*FollowVFn)(u8* self, s32 arg);

static const f32 kTwoPi = 6.2831855f;
static const f32 kPi = 3.1415927f;

// Brings an angle back into -pi..pi: clamped to +-2pi, one turn added or removed when it lies
// outside +-pi, then clamped to +-pi again (the original's three compare ladders).
static f32 WrapAngle(f32 v) {
    f32 a;
    if (!(v >= -kTwoPi)) a = -kTwoPi;
    else if (v > kTwoPi) a = kTwoPi;
    else a = v;
    if (a < -kPi) a = a + kTwoPi;
    else if (a > kPi) a = a - kTwoPi;
    f32 b;
    if (!(a >= -kPi)) b = -kPi;
    else if (a > kPi) b = kPi;
    else b = a;
    return b;
}

// Places the camera from its yaw (+0x4C) and pitch (+0x68): the unit vector (cos yaw, sin yaw, 0)
// is dotted with the up vector (+0xB8), the pitch is raised by 0.6 * asin of that, and the
// vector is tilted by the resulting angle and scaled by the distance (+0x70). The position
// (+0xC) is the target point (+0x24) minus that vector; the direction (+0x98) is the vector
// normalised (left as it is when the vector has length 0 and `guardZero` is set).
static void FollowPlace(u8* self, bool guardZero) {
    f32 yaw = *(f32*)(self + 0x4C);
    f32 s = sinf(yaw);
    f32 c = cosf(yaw);
    f32 v[3] = { c, s, 0.0f };
    f32 up = 0.0f * *(f32*)(self + 0xC0) + (c * *(f32*)(self + 0xB8) + s * *(f32*)(self + 0xBC));
    f32 pitch = *(f32*)(self + 0x68) + 0.6f * (f32)fn_8033F1F8((f64)up);
    f32 cp = cosf(pitch);
    v[0] *= cp; v[1] *= cp; v[2] *= cp;
    f32 sp = -sinf(pitch);
    f32 dist = *(f32*)(self + 0x70);
    f32 x = v[0] * dist;
    f32 y = v[1] * dist;
    f32 z = sp * dist;
    *(f32*)(self + 0xC) = *(f32*)(self + 0x24) - x;
    *(f32*)(self + 0x10) = *(f32*)(self + 0x28) - y;
    *(f32*)(self + 0x14) = *(f32*)(self + 0x2C) - z;
    f32 len2 = z * z + (x * x + y * y);
    if (guardZero && len2 == 0.0f) {
        *(f32*)(self + 0x98) = 0.0f;
        *(f32*)(self + 0x9C) = 0.0f;
        *(f32*)(self + 0xA0) = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        *(f32*)(self + 0x98) = x * inv;
        *(f32*)(self + 0x9C) = y * inv;
        *(f32*)(self + 0xA0) = z * inv;
    }
}

// Turns `angle` towards `want` by the shortest way, at most `rate * dt` per frame, and returns
// the new wrapped angle (the two stores of the original, the raw sum and the wrapped one, are
// left to the caller).
static f32 TurnStep(f32 angle, f32 want, f32 rate, f32 dt) {
    f32 d = fn_800DB844(angle, want);
    f32 maxStep = rate * dt;
    if ((f32)fabs((f64)d) > maxStep) {
        d = d * (maxStep / (f32)fabs((f64)d));
    }
    return angle + d;
}

// 0x800E15AC
// One frame of the camera follow state `self` (mode word at +0xD4, frame time dt).
// First, when +0x74 is above 1.0, calls vtable slot 0x10 of the state with 0.
// Mode 0: the camera is put straight at the place given by the yaw (+0x4C) and pitch (+0x68)
// (byte +0x94 cleared; writes the position +0xC and direction +0x98).
// Mode 1: the wanted yaw (+0x5C, advanced by the rate +0x50 * dt and wrapped) is approached by
// the yaw (+0x4C) at 4 rad/s at most; the rate (+0x50) is approached towards 0 (word +0x54
// 0, or 1 with a negative rate, or 2 with a positive one) or just limited to +-2;
// fn_800E0688 is run, the camera is placed as in mode 0, and fn_800E084C keeps it out of the
// scenery.
// Mode 2: only fn_800E0C70.
// Mode 3: as mode 1 at 5 rad/s, no fn_800E0688, and the pitch (+0x68) also approaches +0x64.
// Other modes do nothing.
extern "C" void fn_800E15AC(u8* self, f32 dt) {
    if (*(f32*)(self + 0x74) > 1.0f) {
        (*(FollowVFn*)(*(u8**)self + 0x10))(self, 0);
    }
    s32 mode = *(s32*)(self + 0xD4);
    if (mode == 0) {
        *(u8*)(self + 0x94) = 0;
        FollowPlace(self, false);
    } else if (mode == 1 || mode == 3) {
        f32 turnRate = (mode == 1) ? 4.0f : 5.0f;
        f32 want = *(f32*)(self + 0x5C) + *(f32*)(self + 0x50) * dt;
        *(f32*)(self + 0x5C) = want;
        want = WrapAngle(want);
        *(f32*)(self + 0x5C) = want;
        f32 yaw = TurnStep(*(f32*)(self + 0x4C), want, turnRate, dt);
        *(f32*)(self + 0x4C) = yaw;
        *(f32*)(self + 0x4C) = WrapAngle(yaw);
        s32 kind = *(s32*)(self + 0x54);
        f32 rate = *(f32*)(self + 0x50);
        bool slow = false;
        if (kind == 0) {
            slow = true;
        } else if (kind == 1 && rate < 0.0f) {
            slow = true;
        } else if (kind == 2 && rate > 0.0f) {
            slow = true;
        }
        if (slow) {
            f32 hi = 30.0f * dt;
            *(f32*)(self + 0x50) = fn_801F5E94(rate, 0.0f, 0.5f * hi, hi, 2.0f);
        } else {
            f32 r;
            if (!(rate >= -2.0f)) r = -2.0f;
            else if (rate > 2.0f) r = 2.0f;
            else r = rate;
            *(f32*)(self + 0x50) = r;
        }
        if (mode == 1) {
            fn_800E0688(self, dt);
        } else {
            f32 pitch = TurnStep(*(f32*)(self + 0x68), *(f32*)(self + 0x64), 5.0f, dt);
            *(f32*)(self + 0x68) = pitch;
            *(f32*)(self + 0x68) = WrapAngle(pitch);
        }
        FollowPlace(self, true);
        fn_800E084C(self);
    } else if (mode == 2) {
        fn_800E0C70(self, dt);
    }
}

// 0x800E1FE0
// Smooths the camera's output position. `self` is the follow state (mode word +0xD4), `pos` the
// output position, dt the frame time.
// Modes 0 and 1: the previous position (+0x18) is approached towards the position (+0xC) with a
// step of up to 12 * dt (at least 0.05 of that, scaled by the distance as in fn_801F5E94).
// Modes 2 and 3: first the previous position (+0x18, written) is moved by the part of
// (position - previous position) that is perpendicular to the direction from the previous
// position to the previous target (+0x30), then it is approached towards the position as above.
// In every case the result goes to `pos` and, when dt > 0.001, the velocity (+0x3C, three
// floats) is (result - previous position) / dt, else 0. Other modes do nothing.
extern "C" void fn_800E1FE0(u8* self, f32* pos, f32 dt) {
    s32 mode = *(s32*)(self + 0xD4);
    if (mode < 0 || mode > 3) {
        return;
    }
    if (mode >= 2) {
        f32 aim[3];
        aim[0] = *(f32*)(self + 0x30) - *(f32*)(self + 0x18);
        aim[1] = *(f32*)(self + 0x34) - *(f32*)(self + 0x1C);
        aim[2] = *(f32*)(self + 0x38) - *(f32*)(self + 0x20);
        f32 off[3];
        off[0] = *(f32*)(self + 0xC) - *(f32*)(self + 0x18);
        off[1] = *(f32*)(self + 0x10) - *(f32*)(self + 0x1C);
        off[2] = *(f32*)(self + 0x14) - *(f32*)(self + 0x20);
        f32 len2 = aim[2] * aim[2] + (aim[0] * aim[0] + aim[1] * aim[1]);
        f32 n[3] = { 0.0f, 0.0f, 0.0f };
        if (len2 != 0.0f) {
            f32 inv = 1.0f / sqrtf(len2);
            n[0] = aim[0] * inv;
            n[1] = aim[1] * inv;
            n[2] = aim[2] * inv;
        }
        f32 dot = off[2] * n[2] + (off[0] * n[0] + off[1] * n[1]);
        *(f32*)(self + 0x18) = *(f32*)(self + 0x18) + (off[0] - n[0] * dot);
        *(f32*)(self + 0x1C) = *(f32*)(self + 0x1C) + (off[1] - n[1] * dot);
        *(f32*)(self + 0x20) = *(f32*)(self + 0x20) + (off[2] - n[2] * dot);
    }
    f32 out[3];
    f32 a = 12.0f + 0.0f;
    f32 hi = a * dt;
    fn_801F5F28(out, (const f32*)(self + 0x18), (const f32*)(self + 0xC), 0.05f * hi, hi, a / 12.0f);
    pos[0] = out[0];
    pos[1] = out[1];
    pos[2] = out[2];
    if (dt > 0.001f) {
        f32 inv = 1.0f / dt;
        *(f32*)(self + 0x3C) = (out[0] - *(f32*)(self + 0x18)) * inv;
        *(f32*)(self + 0x40) = (out[1] - *(f32*)(self + 0x1C)) * inv;
        *(f32*)(self + 0x44) = (out[2] - *(f32*)(self + 0x20)) * inv;
    } else {
        *(f32*)(self + 0x3C) = 0.0f;
        *(f32*)(self + 0x40) = 0.0f;
        *(f32*)(self + 0x44) = 0.0f;
    }
}
