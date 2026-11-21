// Camera object: per-frame update.
// Original: 0x800DC6C4 (auto_03_800DB9A4_text.s)
#include "types.h"
#include "engine/math.h"

extern "C" {
void QuatGetAxisXY(Quat* q, Vec3* out);                              // 0x801453BC
void QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);      // 0x80145EE8
void fn_801458B0(f32* quat, f32* a, f32* b);                         // 0x801458B0
f32  fn_801F5E94(f32 cur, f32 target, f32 lo, f32 hi, f32 range);    // 0x801F5E94
f64  fn_8033F1F4(f64 x);                                             // 0x8033F1F4 (MSL acos)
void fn_800DC070(u8* cam, f32* pos, f32* dir, f32* out, f32 dt);     // 0x800DC070
void fn_800DC514(u8* cam, f32 dt);                                   // 0x800DC514
void fn_800DD1EC(u8* cam, f32 dt);                                   // 0x800DD1EC
void fn_800DD8B4(u8* cam, f32 dt);                                   // 0x800DD8B4
f32  fn_800DE2AC(u8* zone);                                          // 0x800DE2AC
extern u8  lbl_803C4298[];         // 0x803C4298 (+0x3C: flags)
extern f32 lbl_805FB80C;           // 0x805FB80C (camera shake amount)
}

typedef void (*CamModeFn)(u8* sub, f32* pos, f32* dir, f32 dt);

// 0x800DC6C4
// Slot 0x98 of the camera vtable, with the frame time. Steps the blend state (fn_800DD1EC, not
// for a zero frame time) and the zone tracking (fn_800DC514 and the ground value at 0x5C / 0x60),
// gets the wanted position and view direction from fn_800DC070 and from the sub-object of the
// active mode (mode at +0x8: 2 -> +0x140, 3 -> +0x17C, 4 -> +0x1E0; slot 0xC), then moves the
// transform (+0x4: position, quaternion at +0xC) towards them: at once, or, while the byte at
// 0x64 is clear and fn_800DC070 gave a positive blend value, by a part of the way per frame
// (position) and a limited turn per frame (direction). The up vector is (0,0,1) unless the byte
// at 0xFC selects the one at 0xF0. Ends with fn_800DD8B4 and the camera shake amount (0xC,
// reduced by 0x10; zero while bit 1 of lbl_803C4298+0x3C is set).
//
// The original also measures the distance from the camera to the two objects at gWorldState+0x10
// and +0x14 (raised by 0.4) and clamps it to 1.5..2.5 without using the result; that is left out.
extern "C" void fn_800DC6C4(u8* cam, f32 dt) {
    *(u8*)(cam + 0x64) = 0;
    if (!(dt > -0.0001f && dt < 0.0001f)) {
        fn_800DD1EC(cam, dt);
    }
    if (*(u8**)(cam + 0x2C0) != NULL) {
        fn_800DC514(cam, dt);
        u8* zone = *(u8**)(cam + 0x2C0);
        if (*(u8*)(*(u8**)(zone + 0x4) + 0x34) == 4) {
            f32 ground = fn_800DE2AC(zone);
            *(f32*)(cam + 0x5C) = ground;
            *(f32*)(cam + 0x60) = fn_801F5E94(*(f32*)(cam + 0x60), ground, 0.25f * dt, dt, 0.2f);
        }
    }
    f32 blend = 0.0f;
    Vec3 pos = { 0.0f, 0.0f, 0.0f };
    Vec3 dir = { 0.0f, 0.0f, 0.0f };
    fn_800DC070(cam, &pos.x, &dir.x, &blend, dt);
    switch (*(s32*)(cam + 0x8)) {
    case 2: (*(CamModeFn*)(*(u8**)(cam + 0x140) + 0xC))(cam + 0x140, &pos.x, &dir.x, dt); break;
    case 3: (*(CamModeFn*)(*(u8**)(cam + 0x17C) + 0xC))(cam + 0x17C, &pos.x, &dir.x, dt); break;
    case 4: (*(CamModeFn*)(*(u8**)(cam + 0x1E0) + 0xC))(cam + 0x1E0, &pos.x, &dir.x, dt); break;
    }
    dir = Vec3Normalized(dir);
    Vec3 up = { 0.0f, 0.0f, 1.0f };
    if (*(u8*)(cam + 0xFC) != 0) {
        up.x = *(f32*)(cam + 0xF0);
        up.y = *(f32*)(cam + 0xF4);
        up.z = *(f32*)(cam + 0xF8);
    }
    if (*(u8*)(cam + 0x64) == 0 && blend > 0.0f) {
        f32 speed = 2.0f + -1.5f * (blend * 0.5f);
        if (speed < 0.5f) speed = 0.5f;
        else if (speed > 2.0f) speed = 2.0f;
        f32 t = 30.0f * (0.8f * speed) * dt;
        if (t > 1.0f) t = 1.0f;
        else if (!(t > 0.0f)) t = 0.0f;
        f32 keep = 1.0f - t;
        f32* p = *(f32**)(cam + 0x4);
        f32 nx = p[0] * keep + pos.x * t;
        f32 ny = p[1] * keep + pos.y * t;
        f32 nz = p[2] * keep + pos.z * t;
        p[0] = nx;
        p[1] = ny;
        p[2] = nz;
        Vec3 cur;
        QuatGetAxisXY((Quat*)(*(u8**)(cam + 0x4) + 0xC), &cur);
        cur = Vec3Normalized(cur);
        f32 d = cur.z * dir.z + (cur.x * dir.x + cur.y * dir.y);
        if (d < -1.0f) d = -1.0f;
        else if (d > 1.0f) d = 1.0f;
        f32 ang = (f32)fn_8033F1F4((f64)d);
        Vec3 axis;
        axis.x = cur.y * dir.z - cur.z * dir.y;
        axis.y = cur.z * dir.x - cur.x * dir.z;
        axis.z = cur.x * dir.y - cur.y * dir.x;
        f32 a = 0.63661975f * ang;
        axis = Vec3Normalized(axis);
        if (a > 0.0001f) {
            if (a < 0.01f) {
                fn_801458B0((f32*)(*(u8**)(cam + 0x4) + 0xC), &dir.x, &up.x);
            } else {
                f32 rate = 0.17453292f + 2.9670599f * (a / 0.17453292f);
                if (rate < 0.17453292f) rate = 0.17453292f;
                else if (rate > 3.1415927f) rate = 3.1415927f;
                rate = rate * speed;
                if (rate * dt > a) rate = a / dt;
                Quat q;
                QuatFromAxisAngle(&q, &axis, 0.63661975f * (rate * dt));
                // cur = q * cur * conj(q)
                f32 tx = (q.y * cur.z + (q.w * cur.x + q.x * 0.0f)) - q.z * cur.y;
                f32 tw = ((q.w * 0.0f - q.x * cur.x) - q.y * cur.y) - q.z * cur.z;
                f32 ty = q.z * cur.x + (q.y * 0.0f + (q.w * cur.y - q.x * cur.z));
                f32 tz = q.z * 0.0f + ((q.w * cur.z + q.x * cur.y) - q.y * cur.x);
                f32 cx = -q.x, cy = -q.y, cz = -q.z;
                Vec3 r;
                r.x = (ty * cz + (tw * cx + tx * q.w)) - tz * cy;
                r.y = tz * cx + (ty * q.w + (tw * cy - tx * cz));
                r.z = tz * q.w + ((tw * cz + tx * cy) - ty * cx);
                r = Vec3Normalized(r);
                fn_801458B0((f32*)(*(u8**)(cam + 0x4) + 0xC), &r.x, &up.x);
            }
        }
    } else {
        fn_801458B0((f32*)(*(u8**)(cam + 0x4) + 0xC), &dir.x, &up.x);
        f32* p = *(f32**)(cam + 0x4);
        p[0] = pos.x;
        p[1] = pos.y;
        p[2] = pos.z;
    }
    if (*(u8*)(cam + 0xFC) == 0) {
        Vec3 y = QuatGetAxisY((const Quat*)(*(u8**)(cam + 0x4) + 0xC));
        y = Vec3Normalized(y);
        Vec3 z = { 0.0f, 0.0f, 1.0f };
        fn_801458B0((f32*)(*(u8**)(cam + 0x4) + 0xC), &y.x, &z.x);
    }
    fn_800DD8B4(cam, dt);
    if ((*(u32*)(lbl_803C4298 + 0x3C) >> 1) & 1) {
        lbl_805FB80C = 0.0f;
        return;
    }
    f32 shake = *(f32*)(cam + 0xC) - *(f32*)(cam + 0x10);
    *(f32*)(cam + 0xC) = shake;
    if (shake < 0.0f) {
        *(f32*)(cam + 0xC) = 0.0f;
    }
    lbl_805FB80C = *(f32*)(cam + 0xC);
}
