// Drafted functions (round 444): sphere sweep test vs. objects, follow-the-target movement,
// closest point on a triangle, free-look camera stick control.
#include "engine/math.h"

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
Vec3  QuatGetAxisY(const Quat* q);
void  QuatGetForward(Vec3* out, const Quat* q);              // 0x801454CC
s32   RaySceneTest(void* ray);                               // 0x80070458
void  StickRead(s32 channel, f32* x, f32* y);                // 0x80147704
void  fn_800715B8(u8* p, s32 v);                             // 0x800715B8
void  fn_800716D0(u8* p, u32 v);                             // 0x800716D0
u8*   fn_80073454(void);                                     // 0x80073454 (camera/frustum block; planes at +0xC0)
void  fn_800BD440(u8* p);                                    // 0x800BD440
void  fn_800BD468(u8* p);                                    // 0x800BD468
u8*   fn_800BD644(u8* p);                                    // 0x800BD644
void  fn_800BDFD8(u8* p);                                    // 0x800BDFD8
u8*   fn_800BE0B8(u8* p);                                    // 0x800BE0B8
u8*   fn_800BF2AC(u8* p);                                    // 0x800BF2AC
void  fn_800BF710(u8* target, f32 v);                        // 0x800BF710
u8*   fn_800BF7F0(u8* p);                                    // 0x800BF7F0
void  fn_800BF878(u8* p, f32 v);               // 0x800BF878
void  fn_800C09D8(u8* p, s32 mode, f32 v);                   // 0x800C09D8
f32   fn_800DB844(f32 a, f32 b);                             // 0x800DB844
void  fn_800E03F0(u8* p, s32 mode);                          // 0x800E03F0
void  fn_800E084C(u8* p);                                    // 0x800E084C
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);  // 0x80146180
f32   fn_801F5E94(f32 cur, f32 target, f32 lo, f32 hi, f32 range);   // 0x801F5E94
void  fn_801F5F28(f32* out, const f32* cur, const f32* target, f32 lo, f32 hi, f32 range);   // 0x801F5F28
f64   fn_8033F1F4(f64 x);                                    // 0x8033F1F4 (MSL acos)
f64   fn_8033F1F8(f64 x);                                    // 0x8033F1F8 (MSL asin)

extern f32 lbl_80346F70[];   // 0x80346F70
extern f32 lbl_80346F80[];   // 0x80346F80
extern f32 lbl_80346F90[];   // 0x80346F90
extern u8  gGame[];          // 0x803B53A0
extern u8  lbl_803C4298[];   // 0x803C4298 (+0x38: pad channel)
extern f32 lbl_805FB194;     // 0x805FB194
}
extern "C++" f32 GetSign10(u8* p);       // 0x80013F70
extern "C++" f32 GetSignFlip11(u8* p);   // 0x80013F54
extern "C++" void* gEventMgr;   // 0x805FB6C8

// Moving-sphere test between two objects (centre +0x20, radius +0x3C, previous centre +0x14).
// Returns 1 and the hit time *t (0..) if `other` and `self` touch.
static s32 SweepTest(const u8* other, const u8* self, f32* t) {
    f32 dx = *(f32*)(other + 0x20) - *(f32*)(self + 0x20);
    f32 dy = *(f32*)(other + 0x24) - *(f32*)(self + 0x24);
    f32 dz = *(f32*)(other + 0x28) - *(f32*)(self + 0x28);
    f32 r  = *(f32*)(other + 0x3c) + *(f32*)(self + 0x3c);
    f32 sx = *(f32*)(self + 0x14) - *(f32*)(self + 0x20);
    f32 sy = *(f32*)(self + 0x18) - *(f32*)(self + 0x24);
    f32 sz = *(f32*)(self + 0x1c) - *(f32*)(self + 0x28);
    f32 ox = *(f32*)(other + 0x14) - *(f32*)(other + 0x20);
    f32 oy = *(f32*)(other + 0x18) - *(f32*)(other + 0x24);
    f32 oz = *(f32*)(other + 0x1c) - *(f32*)(other + 0x28);
    f32 vx = ox - sx;
    f32 vy = oy - sy;
    f32 vz = oz - sz;
    f32 c = (dz * dz + (dx * dx + dy * dy)) - r * r;
    if (c < 0.0f) {
        *t = 0.0f;      // already overlapping
        return 1;
    }
    f32 a = vz * vz + (vx * vx + vy * vy);
    if (a < 0.0001f) return 0;
    f32 b = vz * dz + (vx * dx + vy * dy);
    if (b >= 0.0f) return 0;
    f32 disc = b * b - a * c;
    if (disc < 0.0f) return 0;
    f32 root = (disc <= 0.0f) ? 0.0f : sqrtf(disc);
    *t = (-b - root) / a;
    return 1;
}

extern "C" {

// 0x800BB7D0
// Tests the object arg1 against the hit objects of arg0 (two lists of 18 and 8, then the
// single one at +0x238); on the first hit within this step applies its effect and returns 1.
s32 fn_800BB7D0(u8* arg0, u8* arg1) {
    if (arg1 == NULL) return 0;
    u8* self = fn_800BF2AC(arg1);
    if (self == NULL) return 0;
    f32 t = 0.0f;
    if ((u32)(*(s32*)(arg1 + 0x290) - 4) <= 1u) {
        for (s32 i = 0; i < 0x12; i++) {
            u8* obj = *(u8**)(arg0 + 0x94 + i * 4);
            u8* other = fn_800BD644(obj);
            if (*(u8*)(obj + 0x298) != 0 && other != NULL) {
                if (SweepTest(other, self, &t) && t <= 1.0f) {
                    obj = *(u8**)(arg0 + 0x94 + i * 4);
                    fn_800BF878(*(u8**)(arg0 + 0x238), *(f32*)(obj + 0x27c));
                    if (*(s32*)(arg1 + 0x290) == 5) fn_800BD440(*(u8**)(arg0 + 0x94 + i * 4));
                    else fn_800BD468(*(u8**)(arg0 + 0x94 + i * 4));
                    return 1;
                }
            }
        }
        for (s32 i = 0; i < 8; i++) {
            u8* obj = *(u8**)(arg0 + 0x130 + i * 4);
            u8* other = fn_800BE0B8(obj);
            if (*(u8*)(obj + 0x264) != 0 && other != NULL) {
                if (SweepTest(other, self, &t) && t <= 1.0f) {
                    switch (*(s32*)(obj + 0x268)) {
                    case 1:
                        fn_800C09D8(arg0 + 0x240, 2, lbl_80346F90[RaySceneTest(obj)]);
                        break;
                    case 2:
                        fn_800C09D8(arg0 + 0x240, 3, lbl_80346F70[RaySceneTest(obj)]);
                        break;
                    case 0:
                        fn_800C09D8(arg0 + 0x240, 1, lbl_80346F80[RaySceneTest(obj)]);
                        break;
                    case 3: {
                        s32 v = *(s32*)(arg0 + 0x488) + 1;
                        if (v < 0) v = 0;
                        else if (v > 5) v = 5;
                        *(s32*)(arg0 + 0x488) = v;
                        fn_800715B8(arg0, v);
                        fn_800716D0(arg0, v > 0);
                        u8* o = *(u8**)(arg0 + 0x478);
                        if (o != NULL) {
                            f32 x = *(f32*)(o + 0x80);
                            f32 y = *(f32*)(o + 0x84);
                            f32 z = *(f32*)(o + 0x88);
                            f32 w = (*(s32*)(arg0 + 0x488) > 0) ? 1.0f : 0.0f;
                            *(f32*)(o + 0x80) = x;
                            *(f32*)(o + 0x84) = y;
                            *(f32*)(o + 0x88) = z;
                            *(f32*)(o + 0x8c) = w;
                            o = *(u8**)(arg0 + 0x478);
                            typedef void (*Fn)(u8*, s32, f32, f32, f32);
                            (*(Fn*)(*(u8**)(o + 0x138) + 0x2c))(o, 0x38, z, y, x);
                        }
                        break;
                    }
                    }
                    fn_800BDFD8(*(u8**)(arg0 + 0x130 + i * 4));
                    return 1;
                }
            }
        }
    }
    u8* other = fn_800BF7F0(*(u8**)(arg0 + 0x238));
    if (other != NULL) {
        if (SweepTest(other, self, &t) && t <= 1.0f) {
            fn_800BF710(*(u8**)(arg0 + 0x238), *(f32*)(arg1 + 0x288));
            return 1;
        }
    }
    return 0;
}

}   // extern "C"

// Marks the object's transform changed (four flag ORs in the original).
static void MarkMoved(u8* t) {
    *(u32*)(t + 0x20) |= 0x8000;
    *(u32*)(t + 0x20) |= 0x10000000;
    *(u32*)(t + 0x20) |= 0x4000;
    *(u32*)(t + 0x20) |= 0x10000000;
}

static Vec3 NormalizeOrZero(f32 x, f32 y, f32 z) {
    f32 len2 = z * z + (x * x + y * y);
    if (len2 == 0.0f) return Vec3{ 0.0f, 0.0f, 0.0f };
    f32 inv = 1.0f / sqrtf(len2);   // original: frsqrte with two Newton steps
    return Vec3{ x * inv, y * inv, z * inv };
}

// Angle between two vectors as a fraction of a half turn (acos * 2/pi); 0 if either is zero.
static f32 AngleFrac(const Vec3& a, const Vec3& b) {
    f32 la2 = a.z * a.z + (a.x * a.x + a.y * a.y);
    f32 la = (la2 <= 0.0f) ? 0.0f : sqrtf(la2);
    f32 lb2 = b.z * b.z + (b.x * b.x + b.y * b.y);
    f32 lb = (lb2 <= 0.0f) ? 0.0f : sqrtf(lb2);
    f32 den = la * lb;
    if (den <= 0.0f) return 0.0f;
    f32 c = (a.z * b.z + (a.x * b.x + a.y * b.y)) / den;
    if (c < -1.0f) c = -1.0f; else if (c > 1.0f) c = 1.0f;
    return 0.63661975f * (f32)fn_8033F1F4((f64)c);
}

extern "C" {

// 0x800C60C0
// Moves the object's transform (arg0+4) toward a target point. Mode 0: sends an event once the
// position is inside all six camera planes' complement test. Mode 2: steps along the object's
// Y axis, turning its orientation toward the target; arrives (mode 3) within one step.
void fn_800C60C0(u8* arg0, f32 dt) {
    if (dt == 0.0f) return;
    s32 mode = *(s32*)(arg0 + 0x268);
    if (mode == 0) {
        u8* t = *(u8**)(arg0 + 4);
        f32 px = *(f32*)(t + 0);
        f32 py = *(f32*)(t + 4);
        f32 pz = *(f32*)(t + 8);
        const u8* plane = fn_80073454() + 0xc0;
        for (s32 i = 0; i < 6; i++, plane += 0x10) {
            f32 d = pz * *(f32*)(plane + 8) + (px * *(f32*)(plane + 0) + py * *(f32*)(plane + 4));
            if (d > *(f32*)(plane + 0xc)) return;
        }
        EventSend(gEventMgr, arg0, NULL, 0x2FA77194u, 0, 0, 0, 0);
    } else if (mode == 2) {
        u8* t = *(u8**)(arg0 + 4);
        f32 speed = *(f32*)(arg0 + 0x264);
        f32 step = speed * dt;
        f32 dx = *(f32*)(arg0 + 0x26c) - *(f32*)(t + 0);
        f32 dy = *(f32*)(arg0 + 0x270) - *(f32*)(t + 4);
        f32 dz = *(f32*)(arg0 + 0x274) - *(f32*)(t + 8);
        if (dz * dz + (dx * dx + dy * dy) < step * step) {
            // Within one step: snap to the target and report arrival.
            *(f32*)(t + 0) = *(f32*)(arg0 + 0x26c);
            *(f32*)(t + 4) = *(f32*)(arg0 + 0x270);
            *(f32*)(t + 8) = *(f32*)(arg0 + 0x274);
            MarkMoved(*(u8**)(arg0 + 4));
            EventSend(gEventMgr, arg0, NULL, 0x0453A5B1u, 0, 0, 0, 0);
            *(s32*)(arg0 + 0x268) = 3;
            return;
        }
        Quat q;
        q.x = *(f32*)(t + 0xc);
        q.y = *(f32*)(t + 0x10);
        q.z = *(f32*)(t + 0x14);
        q.w = *(f32*)(t + 0x18);
        Vec3 axis = QuatGetAxisY(&q);
        Vec3 n = NormalizeOrZero(axis.x, axis.y, axis.z);
        f32 mx = n.x * speed * dt;
        f32 my = n.y * speed * dt;
        f32 mz = n.z * speed * dt;
        (void)my;   // Y keeps the value from +0xCC
        u8* t2 = *(u8**)(arg0 + 4);
        *(f32*)(t2 + 0) = *(f32*)(t2 + 0) + mx;
        *(f32*)(t2 + 4) = *(f32*)(arg0 + 0xcc);
        *(f32*)(t2 + 8) = *(f32*)(t2 + 8) + mz;
        MarkMoved(*(u8**)(arg0 + 4));
        Vec3 dir = NormalizeOrZero(dx, dy, dz);
        // Angle between the facing axis and the direction to the target, limited per step.
        f32 ang = AngleFrac(n, dir);
        if (ang > -0.0001f && ang < 0.0001f) return;
        Vec3 fwd;
        QuatGetForward(&fwd, &q);
        f32 side = AngleFrac(fwd, dir);
        f32 lim = (f32)(4.000000096857548 * (f64)dt);
        f32 turn;
        if (lim > 0.0f) {
            turn = (ang < 0.0f) ? 0.0f : (ang > lim ? lim : ang);
        } else {
            turn = (ang < lim) ? lim : (ang > 0.0f ? 0.0f : ang);
        }
        if (side >= 1.0f) turn *= -1.0f;
        Vec3 up = { 0.0f, 1.0f, 0.0f };
        fn_80146180(&q, &up, turn);
        u8* t3 = *(u8**)(arg0 + 4);
        *(f32*)(t3 + 0xc) = q.x;
        *(f32*)(t3 + 0x10) = q.y;
        *(f32*)(t3 + 0x14) = q.z;
        *(f32*)(t3 + 0x18) = q.w;
        MarkMoved(*(u8**)(arg0 + 4));
    }
}

}   // extern "C"

// Clamps to +-2pi, wraps into +-pi, clamps to +-pi.
static f32 WrapAngle(f32 a) {
    if (a < -6.2831855f) a = -6.2831855f; else if (a > 6.2831855f) a = 6.2831855f;
    if (a < -3.1415927f) a += 6.2831855f;
    else if (a > 3.1415927f) a -= 6.2831855f;
    if (a < -3.1415927f) a = -3.1415927f; else if (a > 3.1415927f) a = 3.1415927f;
    return a;
}

// Limits the shortest-angle step toward `to` to maxStep.
static f32 LimitedStep(f32 from, f32 to, f32 maxStep) {
    f32 d = fn_800DB844(from, to);
    if (fabsf(d) > maxStep) d *= maxStep / fabsf(d);
    return d;
}

extern "C" {

// 0x800E0C70
// Free-look camera update: either eases the yaw (+0x4C/+0x58) to the target yaw, or reads the
// stick and steers yaw/pitch (+0x5C/+0x64/+0x68); then derives the look vector (+0xC, +0x98).
void fn_800E0C70(u8* arg0, f32 dt) {
    if (*(u8*)(arg0 + 0x60) != 0) {
        f32 step = fn_800DB844(*(f32*)(arg0 + 0x58), *(f32*)(arg0 + 0x5c));
        f32 lim = 5.0f * dt;
        if (fabsf(step) > lim) {
            step *= lim / fabsf(step);
        } else if (fabsf(fn_800DB844(*(f32*)(arg0 + 0x4c), *(f32*)(arg0 + 0x5c))) < 0.017453292f) {
            *(u8*)(arg0 + 0x60) = 0;
            fn_800E03F0(arg0, 0);
        }
        f32 v = *(f32*)(arg0 + 0x58) + step;
        *(f32*)(arg0 + 0x58) = v;
        v = WrapAngle(v);
        *(f32*)(arg0 + 0x58) = v;
        *(f32*)(arg0 + 0x4c) = v;
    } else {
        f32 sx, sy;
        StickRead(*(s32*)(lbl_803C4298 + 0x38) * 0x10 + 0xe, &sx, &sy);
        sx = (sx < 0.0f) ? -(sx * sx) : sx * sx;
        sy = (sy < 0.0f) ? -(sy * sy) : sy * sy;
        f32 ix = sx, iy = sy, iz = 0.0f;
        f32 l2 = (0.0f * 0.0f) + (ix * ix + iy * iy);
        if (l2 > 1.0f) {
            f32 inv = 1.0f / sqrtf(l2);   // original: frsqrte with two Newton steps
            ix *= inv; iy *= inv; iz *= inv;
        }
        f32 s10 = ix * GetSign10(gGame + 0xc448);
        f32 tx = 3.0f * s10;
        f32 ty = 3.0f * (-iy * GetSignFlip11(gGame + 0xc448));
        f32 target[3] = { tx, ty, 0.0f };
        f32 cx = *(f32*)(arg0 + 0xa4);
        f32 cy = *(f32*)(arg0 + 0xa8);
        f32 cz = *(f32*)(arg0 + 0xac);
        bool fast = false;
        if (cz * cz + (cx * cx + cy * cy) > tx * tx + ty * ty + 0.0f * 0.0f) fast = true;
        else if (cz * 0.0f + (cx * tx + cy * ty) < 0.0f) fast = true;
        f32 cur[3] = { cx, cy, cz };
        f32 out[3];
        if (fast) fn_801F5F28(out, cur, target, 8.0f * dt, 20.0f * dt, 0.5f);
        else      fn_801F5F28(out, cur, target, 2.5f * dt, 5.0f * dt, 0.5f);
        *(f32*)(arg0 + 0xa4) = out[0];
        *(f32*)(arg0 + 0xa8) = out[1];
        *(f32*)(arg0 + 0xac) = out[2];

        f32 v = *(f32*)(arg0 + 0x5c) + dt * *(f32*)(arg0 + 0xa4);
        *(f32*)(arg0 + 0x5c) = v;
        *(f32*)(arg0 + 0x5c) = WrapAngle(v);

        f32 pitch = *(f32*)(arg0 + 0x64) + dt * *(f32*)(arg0 + 0xa8);
        *(f32*)(arg0 + 0x64) = pitch;
        if (pitch < -1.0471976f) pitch = -1.0471976f; else if (pitch > 1.0471976f) pitch = 1.0471976f;
        *(f32*)(arg0 + 0x64) = pitch;

        v = *(f32*)(arg0 + 0x5c) + *(f32*)(arg0 + 0x50) * dt;
        *(f32*)(arg0 + 0x5c) = v;
        *(f32*)(arg0 + 0x5c) = WrapAngle(v);

        f32 yaw = *(f32*)(arg0 + 0x4c) + LimitedStep(*(f32*)(arg0 + 0x4c), *(f32*)(arg0 + 0x5c), 5.0f * dt);
        *(f32*)(arg0 + 0x4c) = yaw;
        *(f32*)(arg0 + 0x4c) = WrapAngle(yaw);

        s32 m = *(s32*)(arg0 + 0x54);
        bool decay = false;
        if (m == 0) decay = true;
        else if (m == 1 && *(f32*)(arg0 + 0x50) < 0.0f) decay = true;
        else if (m == 2 && *(f32*)(arg0 + 0x50) > 0.0f) decay = true;
        if (decay) {
            f32 lim = 30.0f * dt;
            *(f32*)(arg0 + 0x50) = fn_801F5E94(*(f32*)(arg0 + 0x50), 0.0f, 0.5f * lim, lim, 2.0f);
        } else {
            f32 r = *(f32*)(arg0 + 0x50);
            if (r < -2.0f) r = -2.0f; else if (r > 2.0f) r = 2.0f;
            *(f32*)(arg0 + 0x50) = r;
        }

        f32 p = *(f32*)(arg0 + 0x68) + LimitedStep(*(f32*)(arg0 + 0x68), *(f32*)(arg0 + 0x64), 5.0f * dt);
        *(f32*)(arg0 + 0x68) = p;
        *(f32*)(arg0 + 0x68) = WrapAngle(p);
    }

    f32 s = sinf(*(f32*)(arg0 + 0x4c));
    f32 c = cosf(*(f32*)(arg0 + 0x4c));
    f32 vx = c, vy = s, vz = 0.0f;
    f32 lookDot = 0.0f * *(f32*)(arg0 + 0xc0) + (c * *(f32*)(arg0 + 0xb8) + s * *(f32*)(arg0 + 0xbc));
    f32 elev = *(f32*)(arg0 + 0x68) + 0.6f * (f32)fn_8033F1F8((f64)lookDot);
    f32 ce = cosf(elev);
    vx *= ce; vy *= ce; vz *= ce;
    f32 r = *(f32*)(arg0 + 0x70);
    vx *= r;
    vy *= r;
    vz = -sinf(elev) * r;
    Vec3 n = NormalizeOrZero(vx, vy, vz);
    *(f32*)(arg0 + 0xc) = *(f32*)(arg0 + 0x24) - vx;
    *(f32*)(arg0 + 0x10) = *(f32*)(arg0 + 0x28) - vy;
    *(f32*)(arg0 + 0x14) = *(f32*)(arg0 + 0x2c) - vz;
    *(f32*)(arg0 + 0x98) = n.x;
    *(f32*)(arg0 + 0x9c) = n.y;
    *(f32*)(arg0 + 0xa0) = n.z;
    fn_800E084C(arg0);
}

}   // extern "C"
