// Camera ray/orbit helpers, a quaternion rotate and a static initializer (drafts verified against asm).
#include <math.h>

#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void QuatGetAxisXY(Quat* q, Vec3* out);                       // 0x801453BC
s32  RaySceneTest(void* unused);                                      // 0x80070458 (takes no arguments, returns a global)
void __register_global_object(void* obj, void (*dtor)(void*), void* handle);  // 0x8032EFC8
f32  cosf(f32 x);
void fn_80070FAC(u8* p, f32 v);                               // 0x80070FAC
f32  fn_800C3E84(u8* a);                                      // 0x800C3E84
f32  fn_800C8A64(u8* a);                                      // 0x800C8A64
void* fn_800CF71C(void);                                    // 0x800CF71C
void fn_800CFCFC(u8* p, f32 a, f32 b);               // 0x800CFCFC
void fn_800CFDCC(u8* p, f32 x);                               // 0x800CFDCC
void fn_800DA8F0(u8* query, u8* result);                      // 0x800DA8F0
void fn_800DE28C(u8* out, u8* obj, f32 up);                   // 0x800DE28C
s32  fn_800DECD8(u8* p, f32 dt);                              // 0x800DECD8
void fn_801458B0(f32* quat, f32* a, f32* b);                  // 0x801458B0
void fn_80146180(f32* quat, f32* axis, f32 angle);            // 0x80146180
s32  fn_801C3170(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist);  // 0x801C3170
void* fn_80108FB4(void* p, s32 del);                          // 0x80108FB4
void* fn_80108FF4(void* p, s32 del);                          // 0x80108FF4
void* fn_80109034(void* p, s32 del);                          // 0x80109034
void* fn_80109074(void* p, s32 del);                          // 0x80109074
void* fn_801090B4(void* p, s32 del);                          // 0x801090B4
void* fn_801090F4(void* p, s32 del);                          // 0x801090F4
void* fn_80109134(void* p, s32 del);                          // 0x80109134
void* fn_80109174(void* p, s32 del);                          // 0x80109174
extern f32 lbl_80347290[];                                    // 0x80347290
extern u8  lbl_803C4298[];                                    // 0x803C4298
extern u8  lbl_803C7390[];                                    // 0x803C7390
extern f32 lbl_803C75A8[];                                    // 0x803C75A8
}

typedef void (*VFn)();

static inline f32 Rsqrt(f32 v) { return 1.0f / sqrtf(v); }

extern "C" {

// 0x800CEF88
// Per-frame update of the camera orbit state: positions the follow target from the camera
// object's vtable, records the pose in the camera record, and fires the game-flow signals when
// the view has settled or the timer ran out.
void fn_800CEF88(u8* arg0, f32 farg0) {
    f32 a[3];       // sp8
    f32 b[3];       // sp14
    f32 quat[4];    // sp20
    f32 axis[3];    // sp30

    u8* vt = *(u8**)(arg0 + 0x30F4);
    (*(void (**)(u8*, f32))(vt + 0x1C))(arg0 + 0x3020, farg0 * lbl_80347290[RaySceneTest(0)]);
    a[0] = 0.0f;
    a[1] = -70.0f;
    a[2] = 0.0f;
    (*(void (**)(u8*))(*(u8**)(arg0 + 0x30F4) + 0x20))(arg0 + 0x3020);
    (*(void (**)(f32*, u8*))(*(u8**)(arg0 + 0x30F4) + 0x24))(b, arg0 + 0x3020);
    f32 ax = a[0] + b[0];
    f32 ay = a[1] + b[1];
    f32 az = a[2] + b[2];
    a[0] = ax;
    a[1] = ay;
    a[2] = az;
    u8* g = lbl_803C4298;
    u8* o4 = *(u8**)(g + 0x4);
    (*(void (**)(u8*, f32*, f32*))(*(u8**)o4 + 0x2C))(o4, a, b);
    (*(void (**)(u8*, f32*))(*(u8**)o4 + 0x1C))(o4, a);
    u8* o8 = *(u8**)(g + 0x8);
    (*(void (**)(u8*, f32))(*(u8**)o8 + 0x98))(o8, 0.0f);
    fn_800CFDCC(arg0 + 0x90, farg0);
    u8* rec = (u8*)fn_800CF71C();
    quat[0] = 0.0f;
    quat[1] = 0.0f;
    quat[2] = 0.0f;
    quat[3] = 1.0f;
    axis[0] = 0.0f;
    axis[1] = 1.0f;
    axis[2] = 0.0f;
    fn_80146180(quat, axis, *(f32*)(arg0 + 0xA0));
    *(f32*)(rec + 0x58) = a[0];
    *(f32*)(rec + 0x5C) = a[1];
    *(f32*)(rec + 0x60) = a[2];
    *(f32*)(rec + 0x64) = quat[0];
    *(f32*)(rec + 0x68) = quat[1];
    *(f32*)(rec + 0x6C) = quat[2];
    *(f32*)(rec + 0x70) = quat[3];
    f32 angle = *(f32*)(arg0 + 0xA0);
    // Note: the asm passes the leftover r3 (return of fn_80146180) to RaySceneTest, which ignores it.
    fn_800CFCFC(arg0, farg0, angle);
    f32 t = fn_800C8A64(arg0 + 0xF4);
    fn_80070FAC(arg0, t);
    f32 h = fn_800C3E84(arg0 + 0x3020);
    f32 d = (*(f32 (**)(u8*))(*(u8**)(arg0 + 0x30F4) + 0x20))(arg0 + 0x3020) - h;
    bool settled = false;
    if (d > -0.0001f && d < 0.0001f) settled = true;
    if (settled) GameFlowSignal((s32)0xC608C842, 0, 0);  // won
    if (fn_800C8A64(arg0 + 0xF4) <= 0.0f) GameFlowSignal(0x117741C3, 0, 0);  // out of time
}

// 0x800DF208
// Blends the camera start/end positions toward the target pose with a cosine ease, writes the
// resulting eye/target to arg0+0x60/+0x6C, copies the eye to arg1 and the unit view direction to arg2.
s32 fn_800DF208(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    if (fn_800DECD8(arg0, farg0) == 0) return 0;

    u8* tgt = *(u8**)(lbl_803C4298 + 0x8);
    f32 p0x = *(f32*)(tgt + 0x80), p0y = *(f32*)(tgt + 0x84), p0z = *(f32*)(tgt + 0x88);
    f32 p1x = *(f32*)(tgt + 0x98), p1y = *(f32*)(tgt + 0x9C), p1z = *(f32*)(tgt + 0xA0);

    u8* sa = *(u8**)(arg0 + 0x58);
    f32 e = 0.5f * (1.0f + (f32)cosf(1.5707964f * (2.0f * *(f32*)(arg0 + 0x44))));
    f32 w = 1.0f * (1.0f - e) + 0.0f * e;

    f32 dx = *(f32*)(sa + 0x8) - p0x;
    f32 dy = *(f32*)(sa + 0xC) - p0y;
    f32 dz = *(f32*)(sa + 0x10) - p0z;
    u8* sb = *(u8**)(arg0 + 0x5C);
    f32 ex = dx * w + p0x;
    f32 ey = dy * w + p0y;
    f32 ez = dz * w + p0z;
    *(f32*)(arg0 + 0x60) = ex;
    *(f32*)(arg0 + 0x64) = ey;
    *(f32*)(arg0 + 0x68) = ez;

    f32 fx = *(f32*)(sb + 0x8) - p1x;
    f32 fy = *(f32*)(sb + 0xC) - p1y;
    f32 fz = *(f32*)(sb + 0x10) - p1z;
    *(f32*)(arg0 + 0x6C) = fx * w + p1x;
    *(f32*)(arg0 + 0x70) = fy * w + p1y;
    *(f32*)(arg0 + 0x74) = fz * w + p1z;

    *(f32*)(arg1 + 0x0) = ex;
    *(f32*)(arg1 + 0x4) = *(f32*)(arg0 + 0x64);
    *(f32*)(arg1 + 0x8) = *(f32*)(arg0 + 0x68);

    f32 vx = *(f32*)(arg0 + 0x6C) - *(f32*)(arg0 + 0x60);
    f32 vy = *(f32*)(arg0 + 0x70) - *(f32*)(arg0 + 0x64);
    f32 vz = *(f32*)(arg0 + 0x74) - *(f32*)(arg0 + 0x68);
    *(f32*)(arg2 + 0x0) = vx;
    *(f32*)(arg2 + 0x4) = vy;
    *(f32*)(arg2 + 0x8) = vz;
    f32 len2 = vz * vz + (vx * vx + vy * vy);
    f32 n[3];
    if (len2 == 0.0f) {
        n[2] = 0.0f;
        n[1] = 0.0f;
        n[0] = 0.0f;
    } else {
        f32 r = Rsqrt(len2);
        n[0] = vx * r;
        n[1] = vy * r;
        n[2] = vz * r;
    }
    *(f32*)(arg2 + 0x0) = n[0];
    *(f32*)(arg2 + 0x4) = n[1];
    *(f32*)(arg2 + 0x8) = n[2];
    return 1;
}

// 0x800DFFA8
// Camera pose from the target object's offset (arg0+0x58 + offset), optionally corrected by a
// scene query when arg0+0x56 is set; writes eye to arg1 and the unit view direction to arg2.
s32 fn_800DFFA8(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    f32 off[3];     // sp44
    f32 q[12];      // sp78 query record (0x30 bytes)
    f32 r[10];      // sp50 result record (0x28 bytes)

    if (fn_800DECD8(arg0, farg0) == 0) return 0;

    u8* tgt = *(u8**)(lbl_803C4298 + 0x8);
    fn_800DE28C((u8*)off, tgt, *(f32*)(tgt + 0xB0));
    f32 x = off[0] + *(f32*)(arg0 + 0x58);
    f32 y = off[1] + *(f32*)(arg0 + 0x5C);
    f32 z = off[2] + *(f32*)(arg0 + 0x60);
    *(f32*)(arg0 + 0x64) = x;
    *(f32*)(arg0 + 0x68) = y;
    *(f32*)(arg0 + 0x6C) = z;

    if (*(u8*)(arg0 + 0x56) != 0) {
        q[0] = off[0];
        q[1] = off[1];
        q[2] = off[2];
        q[3] = *(f32*)(arg0 + 0x64);
        q[4] = *(f32*)(arg0 + 0x68);
        q[5] = *(f32*)(arg0 + 0x6C);
        q[6] = 1.0f;
        q[7] = 0.0f;
        *(u8*)((u8*)q + 0x20) = 1;
        *(s32*)((u8*)q + 0x24) = 4;
        *(s32*)((u8*)q + 0x28) = 0;
        *(s32*)((u8*)q + 0x2C) = 2;
        *(u8*)(r + 0) = 0;
        for (int i = 1; i < 10; i++) r[i] = 0.0f;
        fn_800DA8F0((u8*)q, (u8*)r);
        *(f32*)(arg0 + 0x64) = r[7];
        *(f32*)(arg0 + 0x68) = r[8];
        *(f32*)(arg0 + 0x6C) = r[9];
    }
    *(f32*)(arg1 + 0x0) = *(f32*)(arg0 + 0x64);
    *(f32*)(arg1 + 0x4) = *(f32*)(arg0 + 0x68);
    *(f32*)(arg1 + 0x8) = *(f32*)(arg0 + 0x6C);

    f32 vx = off[0] - *(f32*)(arg0 + 0x64);
    f32 vy = off[1] - *(f32*)(arg0 + 0x68);
    f32 vz = off[2] - *(f32*)(arg0 + 0x6C);
    *(f32*)(arg2 + 0x0) = vx;
    *(f32*)(arg2 + 0x4) = vy;
    *(f32*)(arg2 + 0x8) = vz;
    f32 len2 = vz * vz + (vx * vx + vy * vy);
    f32 n[3];
    if (len2 == 0.0f) {
        n[2] = 0.0f;
        n[1] = 0.0f;
        n[0] = 0.0f;
    } else {
        f32 rs = Rsqrt(len2);
        n[0] = vx * rs;
        n[1] = vy * rs;
        n[2] = vz * rs;
    }
    *(f32*)(arg2 + 0x0) = n[0];
    *(f32*)(arg2 + 0x4) = n[1];
    *(f32*)(arg2 + 0x8) = n[2];
    return 1;
}

// 0x80108CB4
// Static initializer: sets the global vector (0,1,0), registers the destructors of the static
// objects in lbl_803C7390 and builds the "PNP0" table at +0xF8 (2 entries of 0x18 bytes, 4 of 0x10).
void fn_80108CB4(void) {
    u8* base = lbl_803C7390;
    lbl_803C75A8[0] = 0.0f;
    lbl_803C75A8[1] = 1.0f;
    lbl_803C75A8[2] = 0.0f;
    __register_global_object(base + 0x5C, (void (*)(void*))fn_80108FF4, base + 0x50);
    __register_global_object(base + 0x74, (void (*)(void*))fn_80109134, base + 0x68);
    __register_global_object(base + 0x8C, (void (*)(void*))fn_801090F4, base + 0x80);
    __register_global_object(base + 0xA4, (void (*)(void*))fn_801090B4, base + 0x98);
    __register_global_object(base + 0xBC, (void (*)(void*))fn_80109074, base + 0xB0);
    __register_global_object(base + 0xD4, (void (*)(void*))fn_80109034, base + 0xC8);
    __register_global_object(base + 0xEC, (void (*)(void*))fn_80109174, base + 0xE0);

    u8* t = base + 0xF8;
    *(s32*)(t + 0x40) = 0;
    *(s32*)(t + 0x44) = 0;
    *(s32*)(t + 0x48) = 0;
    *(s32*)(t + 0x4C) = 0;
    // Inlined array-constructor zero fill of t+0x50 .. t+0x80 (unrolled by the compiler).
    for (u8* p = t + 0x50; p < t + 0x80; p += 4) *(s32*)p = 0;

    *(s32*)(t + 0x0) = 0x504E5030;  // "PNP0"
    *(s32*)(t + 0x4) = 2;
    *(s32*)(t + 0x8) = 4;
    *(s32*)(t + 0xC) = 0;
    u8* e = t;
    for (int i = 0; i < 2; i++) {
        *(s32*)(e + 0x10) = 0;
        *(s32*)(e + 0x14) = 0;
        *(s32*)(e + 0x18) = 0;
        *(s32*)(e + 0x1C) = 0;
        *(s32*)(e + 0x20) = 0;
        *(s32*)(e + 0x24) = 0;
        e += 0x18;
    }
    u8* c = t;
    for (u32 i = 0; i < (u32)*(s32*)(t + 0x8); i++) {
        *(s32*)(c + 0x40) = 0;
        *(s32*)(c + 0x44) = 0;
        *(s32*)(c + 0x48) = 0;
        *(s32*)(c + 0x4C) = 0;
        c += 0x10;
    }
    __register_global_object(base + 0x20C, (void (*)(void*))fn_80108FB4, base + 0x200);
}

// 0x80121EC0
// Rotates the vector arg2 by the orientation that maps the camera's forward direction
// (horizontal component of the direction from arg0's position to arg1's) onto +Z (q * v * conj(q)).
void fn_80121EC0(u8* arg0, u8* arg1, f32* arg2) {
    Vec3 dir;       // sp98
    f32 q[4];       // sp88
    f32 n[3];       // sp6C
    f32 up[3];      // sp60

    QuatGetAxisXY((Quat*)(*(u8**)(arg0 + 0x4) + 0xC), &dir);
    u8* pb = *(u8**)(arg1 + 0x4);
    u8* pa = *(u8**)(arg0 + 0x4);
    f32 d0 = *(f32*)(pb + 0x0) - *(f32*)(pa + 0x0);
    f32 d1 = *(f32*)(pb + 0x4) - *(f32*)(pa + 0x4);
    f32 d2 = *(f32*)(pb + 0x8) - *(f32*)(pa + 0x8);
    if (((dir.z * d2) + ((dir.x * d0) + (dir.y * d1))) < 0.0f) {
        dir.x = d0;
        dir.y = d1;
        dir.z = d2;
    }
    f32 dx = dir.x;
    dir.z = 0.0f;
    f32 len2 = (0.0f * 0.0f) + ((dx * dx) + (dir.y * dir.y));
    if (len2 == 0.0f) {
        n[2] = 0.0f;
        n[1] = 0.0f;
        n[0] = 0.0f;
    } else {
        f32 r = Rsqrt(len2);
        n[0] = dx * r;
        n[1] = dir.y * r;
        n[2] = 0.0f * r;
    }
    dir.x = n[0];
    dir.y = n[1];
    dir.z = n[2];
    up[1] = 0.0f;
    up[0] = 0.0f;
    up[2] = 1.0f;
    fn_801458B0(q, &dir.x, up);

    f32 x = q[0], y = q[1], z = q[2], w = q[3];
    f32 vx = arg2[0], vz = arg2[2], vy = arg2[1];
    f32 cx = -x, cz = -z, cy = -y;
    // t = q * v (v as a pure quaternion)
    f32 X1 = ((y * vz) + ((w * vx) + (x * 0.0f))) - (z * vy);
    f32 W1 = (((w * 0.0f) - (x * vx)) - (y * vy)) - (z * vz);
    f32 Y1 = (z * vx) + ((y * 0.0f) + ((w * vy) - (x * vz)));
    f32 Z1 = (z * 0.0f) + (((w * vz) + (x * vy)) - (y * vx));
    // result = t * conj(q)
    f32 Rx = ((Y1 * cz) + ((W1 * cx) + (X1 * w))) - (Z1 * cy);
    f32 Ry = (Z1 * cx) + ((Y1 * w) + ((W1 * cy) - (X1 * cz)));
    f32 Rz = (Z1 * w) + (((W1 * cz) + (X1 * cy)) - (Y1 * cx));
    arg2[0] = Rx;
    arg2[1] = Ry;
    arg2[2] = Rz;
}

// 0x801AF770
// Sweeps a sphere (radius farg0 + farg1) from arg1 toward arg2 against arg0; on a hit writes the
// contact point (arg3+0), the unit normal (arg3+0xC) and the fraction (arg3+0x18). Returns 0 when
// both candidate fractions lie outside [0,1].
s32 fn_801AF770(u8* arg0, u8* arg1, u8* arg2, u8* arg3, f32 farg0, f32 farg1) {
    f32 dist[2];    // sp10
    s32 count;      // sp8
    f32 pos[6];     // sp58 (two candidate contact points)
    f32 delta[3];   // sp48

    delta[0] = *(f32*)(arg2 + 0x0) - *(f32*)(arg1 + 0x0);
    delta[1] = *(f32*)(arg2 + 0x4) - *(f32*)(arg1 + 0x4);
    delta[2] = *(f32*)(arg2 + 0x8) - *(f32*)(arg1 + 0x8);
    s32 hit = fn_801C3170(farg0 + farg1, arg0, arg1, (u8*)delta, &count, pos, dist);
    if (hit != 0) {
        f32* pp;
        f32 dd;
        if (count > 1) {
            if (dist[0] < dist[1]) {
                pp = pos;
                dd = dist[0];
            } else {
                pp = pos + 3;
                dd = dist[1];
            }
        } else {
            pp = pos;
            dd = dist[0];
        }
        *(f32*)(arg3 + 0x0) = pp[0];
        *(f32*)(arg3 + 0x4) = pp[1];
        *(f32*)(arg3 + 0x8) = pp[2];
        f32 nx = pp[0] - *(f32*)(arg0 + 0x0);
        f32 ny = pp[1] - *(f32*)(arg0 + 0x4);
        f32 nz = pp[2] - *(f32*)(arg0 + 0x8);
        *(f32*)(arg3 + 0xC) = nx;
        *(f32*)(arg3 + 0x10) = ny;
        *(f32*)(arg3 + 0x14) = nz;
        *(f32*)(arg3 + 0x18) = dd;

        f32 ax = *(f32*)(arg3 + 0xC);
        f32 ay = *(f32*)(arg3 + 0x10);
        f32 az = *(f32*)(arg3 + 0x14);
        f32 len2 = (az * az) + ((ax * ax) + (ay * ay));
        f32 n[3];
        if (len2 == 0.0f) {
            n[2] = 0.0f;
            n[1] = 0.0f;
            n[0] = 0.0f;
        } else {
            f32 r = Rsqrt(len2);
            n[0] = ax * r;
            n[1] = ay * r;
            n[2] = az * r;
        }
        *(f32*)(arg3 + 0xC) = n[0];
        *(f32*)(arg3 + 0x10) = n[1];
        *(f32*)(arg3 + 0x14) = n[2];
        if ((dist[0] < 0.0f || dist[0] > 1.0f) && (dist[1] < 0.0f || dist[1] > 1.0f)) hit = 0;
    }
    return hit;
}

}  // extern "C"
