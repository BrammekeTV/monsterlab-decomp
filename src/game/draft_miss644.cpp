// Mixed 0x8009xxxx-0x800Bxxxx functions: spawn helper, aim/guide updates, HUD update, spawn-at.
#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

// Globals defined in C++ files.
extern "C++" void* gEventMgr;                // 0x805FB6C8
extern u8* lbl_805FB120;                     // 0x805FB120 (tracked actor)

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  MtxFromLocatorRotation(f32 out[12], const void* rot);   // 0x80143CA8
u32   RandomU32(void);                                         // 0x80143460
s32   RaySceneTest(void* ray);                                 // 0x80070458 (ignores its argument)
int   SysSprintf(char* buf, const char* fmt, ...);             // 0x80142EF8
s32   fn_80099AD8(void);                                       // 0x80099AD8
void  fn_800AA7A8(u8* p);                                      // 0x800AA7A8
void  fn_800AA8C0(u8* p);                                      // 0x800AA8C0
void  fn_800AC0F0(u8* p, f32 dt);                              // 0x800AC0F0
void  fn_800AE3A8(f32 dt);                                     // 0x800AE3A8
void  fn_800AFBEC(u8* p, f32 dt);                              // 0x800AFBEC
void  fn_800AFF8C(u8* arg0, u8* arg1);                         // 0x800AFF8C
void  fn_800B0118(u8* p, f32 angle);                           // 0x800B0118
void  fn_800B0D08(u8* p);                              // 0x800B0D08
void  fn_800B10BC(u8* p, f32 angle);                           // 0x800B10BC
void  fn_800B1674(u8* p, f32 dt);                              // 0x800B1674
void  fn_800B4664(u8* p, f32 dt);                              // 0x800B4664
void  fn_800B63F4(u8* p, f32 dt);                              // 0x800B63F4
s32   fn_800B6848(u8* p);                                      // 0x800B6848
s32   fn_800B6850(u8* p);                                      // 0x800B6850
s32 fn_800B66F0(u8* p, s32 mode);                            // 0x800B66F0
f32   fn_800B7198(u8* p);                                      // 0x800B7198
f32   fn_800B71A0(u8* p);                                      // 0x800B71A0
f32   fn_800B71A8(u8* p);                                      // 0x800B71A8
f32   fn_800B7380(u8* p);                                      // 0x800B7380
f32   fn_800B7388(u8* p);                                      // 0x800B7388
f32   fn_800B7390(u8* p);                                      // 0x800B7390
f32   fn_800B7398(u8* p);                                      // 0x800B7398
s32   fn_800B73A0(u8* p);                                      // 0x800B73A0
s32   fn_800B73D0(u8* p);                                      // 0x800B73D0
void  fn_800B79D8(u8* p, f32 dt);                              // 0x800B79D8
void  fn_800B7C64(u8* p, f32 dt);                              // 0x800B7C64
void  fn_800B82E0(u8* child, f32 delta);                       // 0x800B82E0
void  fn_800B8A68(u8* arg0, s32 arg1, f32 farg0, f32 farg1);   // 0x800B8A68
u8*   fn_800C2028(u8* a0);                                     // 0x800C2028
u32   fn_800C2618(u8* a3, s32 a4);                             // 0x800C2618 (really returns a pointer)
void  fn_801458B0(f32* quat, f32* a, f32* b);                  // 0x801458B0
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);    // 0x80146180 (rotate quat about axis)
void  fn_801B6F34(u8* p);                                      // 0x801B6F34
void  fn_801B7020(u8* p);                                      // 0x801B7020
f64   fn_8033F1F4(f64 x);                                      // 0x8033F1F4 (MSL acos)
f32   fn_8033F200(f32 a, f32 b);                               // 0x8033F200 (fmod)

extern s32 lbl_803463B8[];   // 0x803463B8
extern f32 lbl_803463C8[];   // 0x803463C8
extern f32 lbl_803463D8[];   // 0x803463D8
extern f32 lbl_803463E8[];   // 0x803463E8
extern f32 lbl_80346098[];   // 0x80346098
extern f32 lbl_80346D00[];   // 0x80346D00
extern f32 lbl_80346D10[];   // 0x80346D10
extern f32 lbl_80346D20[];   // 0x80346D20
extern f32 lbl_80346D30[];   // 0x80346D30
extern u8  lbl_8035E138[];   // 0x8035E138 (format strings)
extern u8  lbl_803C4298[];   // 0x803C4298
extern f32 lbl_805FDCCC;     // 0.0f
extern f64 lbl_805FDCD8;     // 2*pi as a double
extern f32 lbl_805FDCE0;     // 1.0f
extern f32 lbl_805FDCE4;     // 2^-32
extern f32 lbl_805FDCE8;     // 2*pi
extern f32 lbl_805FDCEC;     // 2/pi
extern s32 lbl_805FDD04;     // 0x805FDD04
extern f32 lbl_805FDE40;     // 0.0f
extern f32 lbl_805FDE48;     // -11.0f
extern f32 lbl_805FDE4C;     // 8.0f
extern f32 lbl_805FDE50;     // 1.0f
extern f32 lbl_805FDE68;     // -1.0f
extern f32 lbl_805FDE6C;     // 2/pi
extern f32 lbl_805FDE70;     // pi/2
extern f32 lbl_805FDF18;     // 0.0f
extern f32 lbl_805FDF1C;     // 1.0f
extern f32 lbl_805FDF50;     // 6.5f
extern f32 lbl_805FDF80;     // 2^-32
extern f32 lbl_805FDF90;     // 0.01f
extern f32 lbl_805FE280;     // 0.0f
extern f32 lbl_805FE288;     // 2^-32
extern f32 lbl_805FE28C;     // -1.0f
extern f32 lbl_805FE290;     // 2.0f
extern f32 lbl_805FE2A8;     // 1.0f
}

typedef u8* (*SpawnFn)(u8*);
typedef void (*SlotFn_i)(u8*, s32);
typedef void (*SlotFn_v)(u8*);
typedef void (*SlotFn_vv)(u8*, f32*, f32*);
typedef void (*SlotFn_vp)(u8*, f32*);
typedef void (*SlotFn_f)(u8*, f32);
typedef void (*QueryFn)(f32*, u8*, f32, f32);
typedef void (*QueryFn_p)(u8*);
typedef void (*QueryFn_pp)(f32*, u8*);

// Marks the render object's transform dirty (same four flag ors the original does after each write).
static inline void MarkXformDirty(u8* t) {
    *(u32*)(t + 0x20) |= 0x8000;
    *(u32*)(t + 0x20) |= 0x10000000;
    *(u32*)(t + 0x20) |= 0x4000;
    *(u32*)(t + 0x20) |= 0x10000000;
}

extern "C" {

// 0x8009B7A8
// Spawns (or reuses) a spawn-pool object for the current difficulty, orients it with a random
// yaw and stores the rotation matrix (+0x270) and yaw (+0x2B4, relative to the entry in arg1).
u8* fn_8009B7A8(u8* arg0, u8* arg1) {
    f32 yaw = lbl_805FDCCC;
    s32 reuse = 0;
    // Identity 3x4 matrix (the original also initialises several unused unit vectors).
    f32 m[12];
    m[0] = lbl_805FDCE0; m[1] = lbl_805FDCCC; m[2] = lbl_805FDCCC;
    m[3] = lbl_805FDCCC; m[4] = lbl_805FDCE0; m[5] = lbl_805FDCCC;
    m[6] = lbl_805FDCCC; m[7] = lbl_805FDCCC; m[8] = lbl_805FDCE0;
    m[9] = lbl_805FDCCC; m[10] = lbl_805FDCCC; m[11] = lbl_805FDCCC;

    if (fn_80099AD8() == 0) {
        s32 limit = *(s32*)(arg1 + 0x8);
        reuse = (lbl_803463B8[RaySceneTest(NULL)] - 1) <= limit;
    }

    u8* obj;
    if (reuse == 0) {
        s32 kind;
        f32 r = lbl_805FDCE4 * (f32)RandomU32();
        if (r > lbl_803463C8[RaySceneTest(NULL)]) {
            r = lbl_805FDCE4 * (f32)RandomU32();
            kind = (r < lbl_803463E8[RaySceneTest(NULL)]) ? 5 : 4;
            s32 steps = lbl_805FDD04;
            f32 step = lbl_805FDCE8 / (f32)steps;
            yaw = step * (f32)(s32)(RandomU32() % (u32)steps);
        } else {
            r = lbl_805FDCE4 * (f32)RandomU32();
            kind = (r < lbl_803463D8[RaySceneTest(NULL)]) ? 3 : 2;
        }
        obj = fn_800C2028((u8*)(uintptr_t)fn_800C2618(arg0, kind));
        f32 rot[3];
        rot[0] = lbl_805FDCCC;
        rot[1] = lbl_805FDCEC * yaw;
        rot[2] = lbl_805FDCCC;
        MtxFromLocatorRotation(m, rot);
        // Fall back to the pool entries from index 2 on until one yields an object.
        for (s32 i = 2; i < *(s32*)(arg0 + 0x4) && obj == NULL; i++) {
            obj = fn_800C2028((u8*)(uintptr_t)fn_800C2618(arg0, i));
        }
    } else {
        obj = (*(SpawnFn*)(*(u8**)(arg0 + 0xC) + 0x18))(arg0);
    }

    for (s32 i = 0; i < 12; i++) {
        *(f32*)(obj + 0x270 + i * 4) = m[i];
    }

    // Ring-buffer style lookup of the entry at index arg1+0x6C (inlined range check).
    s32 idx = *(s32*)(arg1 + 0x6C);
    u8* elem;
    s32 invalid = 1;
    if (idx >= 0 && *(s32*)(arg1 + 0x70) >= 0) invalid = 0;
    if (invalid == 0) {
        s32 lo = idx;
        s32 ok;
        s32 count;
        if (idx < 0 || !(idx < (count = *(s32*)(arg1 + 0x74)))) {
            ok = 0;
        } else {
            s32 hi = *(s32*)(arg1 + 0x70);
            if (hi < idx) {
                if (idx <= hi) lo = idx + count;
                hi += count;
            }
            ok = 0;
            if (!(idx > lo) && !(lo > hi)) ok = 1;
        }
        u8* slot = ok ? (*(u8**)(arg1 + 0x78) + idx * 4) : NULL;
        *(u8**)(arg1 + 0x7C) = slot;
        elem = slot ? *(u8**)slot : NULL;
    } else {
        elem = NULL;
    }
    *(f32*)(obj + 0x2B4) = fn_8033F200(yaw + *(f32*)(elem + 0x2B4), (f32)lbl_805FDCD8);
    return obj;
}

// 0x800A076C
// Places the camera/guide pair: queries the ground vector below the actor, turns the offset
// pair by the angle between the ground vector and the up axis, then publishes both points.
void fn_800A076C(u8* arg0) {
    u8* a = *(u8**)(lbl_803C4298 + 4);
    (*(SlotFn_i*)(*(u8**)a + 0x50))(a, 0);
    a = *(u8**)(lbl_803C4298 + 4);
    (*(SlotFn_i*)(*(u8**)a + 0xC))(a, 6);

    f32 p44[3];   // sp44
    f32 p38[3];   // sp38
    p44[0] = lbl_805FDE40; p44[1] = lbl_805FDE48; p44[2] = lbl_805FDE4C;
    p38[0] = lbl_805FDE40; p38[1] = lbl_805FDE4C; p38[2] = lbl_805FDE40;

    // Query slot at +0x28 of the table stored at +0x160: (out, pos, -11.0f, 0.0f); the two
    // floats are what the original still holds in f1/f2 at the call.
    f32 v[3];
    (*(QueryFn*)(*(u8**)(arg0 + 0x160) + 0x28))(v, arg0 + 0x8C, lbl_805FDE48, lbl_805FDE40);

    f32 up[3];
    f32 right[3];
    up[0] = lbl_805FDE40; up[1] = lbl_805FDE50; up[2] = lbl_805FDE40;
    right[0] = lbl_805FDE50; right[1] = lbl_805FDE40; right[2] = lbl_805FDE40;

    f32 len1 = lbl_805FDE40;
    f32 sq1 = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
    f32 dotUp = v[2] * up[2] + (v[0] * up[0] + v[1] * up[1]);
    if (!(sq1 <= lbl_805FDE40)) len1 = sqrtf(sq1);

    f32 len2 = lbl_805FDE40;
    f32 sq2 = up[2] * up[2] + (up[0] * up[0] + up[1] * up[1]);
    if (!(sq2 <= lbl_805FDE40)) len2 = sqrtf(sq2);

    f32 ang = lbl_805FDE40;
    f32 prod = len1 * len2;
    if (!(prod <= ang)) {
        f32 c = dotUp / prod;
        // clamp to [-1, 1]
        c = (c < lbl_805FDE68) ? lbl_805FDE68 : ((c > lbl_805FDE50) ? lbl_805FDE50 : c);
        ang = lbl_805FDE6C * (f32)fn_8033F1F4((f64)c);
    }
    f32 dotRight = v[2] * right[2] + (v[0] * right[0] + v[1] * right[1]);
    if (dotRight > lbl_805FDE40) ang = -ang;

    // Rotate the 2D offset pairs (+0x38: x/y at [0],[1]; +0x44: x/y at [0],[1]) by the angle.
    f32 a1 = lbl_805FDE70 * ang;
    f32 c1 = cosf(a1);
    f32 s1 = sinf(a1);
    f32 n38_0 = p38[0] * c1 + p38[1] * (-s1);
    f32 n38_1 = p38[1] * c1 + p38[0] * s1;
    p38[0] = n38_0;
    p38[1] = n38_1;
    f32 a2 = lbl_805FDE70 * ang;
    f32 c2 = cosf(a2);
    f32 s2 = sinf(a2);
    f32 n44_1 = p44[1] * c2 + p44[0] * s2;
    f32 n44_0 = p44[0] * c2 + p44[1] * (-s2);
    p44[1] = n44_1;
    p44[0] = n44_0;

    (*(QueryFn_p*)(*(u8**)(arg0 + 0x160) + 0x20))(arg0 + 0x8C);
    f32 w[3];
    (*(QueryFn_pp*)(*(u8**)(arg0 + 0x160) + 0x24))(w, arg0 + 0x8C);

    p38[0] = p38[0] + w[0];
    p38[1] = p38[1] + w[1];
    p38[2] = p38[2] + w[2];
    p44[0] = p44[0] + w[0];
    p44[1] = p44[1] + w[1];
    p44[2] = p44[2] + w[2];

    *(f32*)(arg0 + 0xC94) = p44[0];
    *(f32*)(arg0 + 0xC98) = p44[1];
    *(f32*)(arg0 + 0xC9C) = p44[2];
    *(f32*)(arg0 + 0xCA0) = p38[0];
    *(f32*)(arg0 + 0xCA4) = p38[1];
    *(f32*)(arg0 + 0xCA8) = p38[2];

    a = *(u8**)(lbl_803C4298 + 4);
    (*(SlotFn_vv*)(*(u8**)a + 0x2C))(a, p44, p38);
    a = *(u8**)(lbl_803C4298 + 4);
    (*(SlotFn_vp*)(*(u8**)a + 0x1C))(a, p44);
    a = *(u8**)(lbl_803C4298 + 8);
    (*(SlotFn_f*)(*(u8**)a + 0x98))(a, lbl_805FDE40);
}

// 0x800AA094
// Per-frame update of a guide object: ticks the 14 + 10 child parts, updates the two sub
// objects and counts down the two timers (+0x154 reroll timer, +0x158 blink timer).
void fn_800AA094(u8* arg0, f32 dt) {
    for (s32 i = 0; i < 0xE; i++) {
        fn_800AC0F0(*(u8**)(arg0 + 0xE0 + i * 4), dt);
        fn_801B7020(*(u8**)(arg0 + 0xE0 + i * 4));
    }
    fn_800B1674(*(u8**)(arg0 + 0x94), dt);
    fn_800B0118(*(u8**)(arg0 + 0xB0), *(f32*)(arg0 + 0x71C));
    f32 pos[3];
    pos[0] = *(f32*)(arg0 + 0x6AC);
    pos[1] = *(f32*)(arg0 + 0x6B0);
    pos[2] = *(f32*)(arg0 + 0x6B4);
    fn_800AFF8C(*(u8**)(arg0 + 0xB0), (u8*)pos);
    fn_800AFBEC(*(u8**)(arg0 + 0xB0), dt);
    fn_800B10BC(*(u8**)(arg0 + 0xB4), *(f32*)(arg0 + 0x71C));
    fn_800B0D08(*(u8**)(arg0 + 0xB4));
    for (s32 i = 0; i < 0xA; i++) {
        fn_801B7020(*(u8**)(arg0 + 0xB8 + i * 4));
    }
    fn_800AE3A8(dt);

    f32 t = *(f32*)(arg0 + 0x154) - dt;
    *(f32*)(arg0 + 0x154) = t;
    if (t <= lbl_805FDF18) {
        // Reroll the timer with a random value between two table entries.
        if (*(f32*)(arg0 + 0x150) <= lbl_805FDF18) {
            f32 hi = lbl_80346D30[RaySceneTest(NULL)];
            f32 lo = lbl_80346D20[RaySceneTest(NULL)];
            *(f32*)(arg0 + 0x154) = lo + (hi - lo) * (lbl_805FDF80 * (f32)RandomU32());
        } else {
            f32 hi = lbl_80346D10[RaySceneTest(NULL)];
            f32 lo = lbl_80346D00[RaySceneTest(NULL)];
            *(f32*)(arg0 + 0x154) = lo + (hi - lo) * (lbl_805FDF80 * (f32)RandomU32());
        }
        fn_800AA7A8(arg0);
    }

    f32 t2 = *(f32*)(arg0 + 0x158) - dt;
    *(f32*)(arg0 + 0x158) = t2;
    if (t2 <= lbl_805FDF18) {
        *(f32*)(arg0 + 0x158) = lbl_805FDF50;
        fn_800AA8C0(arg0);
    }
}

// 0x800AA2D0
// Rotates the two aim quats (+0x6F4, +0x704) about Z by the angle change (arg 3, f1 unused),
// recomputes both end points from their Y axes and moves the tracked actor towards them.
void fn_800AA2D0(u8* arg0, f32 dt, f32 angle) {
    f32 p98[3];
    f32 p8c[3];
    p98[0] = *(f32*)(arg0 + 0x6D0) + lbl_805FDF18;
    p98[1] = *(f32*)(arg0 + 0x6D4) + lbl_805FDF18;
    p98[2] = *(f32*)(arg0 + 0x6D8) + *(f32*)(arg0 + 0x6E4);
    p8c[0] = *(f32*)(arg0 + 0x6D0) + lbl_805FDF18;
    p8c[1] = *(f32*)(arg0 + 0x6D4) + lbl_805FDF18;
    p8c[2] = *(f32*)(arg0 + 0x6D8) + *(f32*)(arg0 + 0x6F0);

    f32 diff = angle - *(f32*)(arg0 + 0x720);
    if (fabsf(diff) > lbl_805FDF18) {
        Vec3 axisA = { lbl_805FDF18, lbl_805FDF18, lbl_805FDF1C };
        fn_80146180(arg0 + 0x6F4, &axisA, diff);
        Vec3 axisB = { lbl_805FDF18, lbl_805FDF18, lbl_805FDF1C };
        fn_80146180(arg0 + 0x704, &axisB, diff);
    }
    *(f32*)(arg0 + 0x720) = angle;

    Vec3 d1 = QuatGetAxisY((const Quat*)(arg0 + 0x6F4));
    f32 len1 = *(f32*)(arg0 + 0x714);
    p98[0] = p98[0] + d1.x * len1;
    p98[1] = p98[1] + d1.y * len1;
    p98[2] = p98[2] + d1.z * len1;

    Vec3 d2 = QuatGetAxisY((const Quat*)(arg0 + 0x704));
    f32 len2 = *(f32*)(arg0 + 0x718);
    f32 e0 = p8c[0] + d2.x * len2;
    f32 e1 = p8c[1] + d2.y * len2;
    f32 e2 = p8c[2] + d2.z * len2;
    p8c[0] = e0;
    p8c[1] = e1;
    p8c[2] = e2;

    if (lbl_805FB120 != NULL) {
        f32 dx = *(f32*)(arg0 + 0x6D0) - p98[0];
        f32 dy = *(f32*)(arg0 + 0x6D4) - p98[1];
        f32 dz = *(f32*)(arg0 + 0x6D8) - p98[2];
        u8* t = *(u8**)(lbl_805FB120 + 4);
        *(f32*)(t + 0x0) = e0 + dx * lbl_805FDF90;
        *(f32*)(t + 0x4) = e1 + dy * lbl_805FDF90;
        *(f32*)(t + 0x8) = e2 + dz * lbl_805FDF90;
        MarkXformDirty(*(u8**)(lbl_805FB120 + 4));
    }

    u8* a = *(u8**)(lbl_803C4298 + 4);
    (*(SlotFn_vv*)(*(u8**)a + 0x2C))(a, p8c, p98);
    a = *(u8**)(lbl_803C4298 + 8);
    (*(SlotFn_f*)(*(u8**)a + 0x98))(a, lbl_805FDF18);
}

// 0x800B5478
// Per-frame update of the status HUD: ticks the child widgets, picks the label text for the
// current mode into +0x894 and updates the two icon widgets and the stat line (+0x998).
void fn_800B5478(u8* arg0, f32 dt) {
#define WSLOT(w, off) (*(SlotFn_i*)(*(u8**)((w) + 0x138) + (off)))
    fn_800B63F4(*(u8**)(arg0 + 0x94), dt);
    for (s32 i = 0; i < 2; i++) {
        fn_800B7C64(*(u8**)(arg0 + 0xA0 + i * 4), dt);
        fn_800B82E0(*(u8**)(arg0 + 0x98 + i * 4), dt);
        for (s32 j = 0; j < 3; j++) {
            fn_800B79D8(*(u8**)(arg0 + 0x148 + i * 0xC + j * 4), dt);
        }
    }
    fn_800B4664(arg0, dt);

    u8* w;
    w = *(u8**)(arg0 + 0xA3C); WSLOT(w, 0x30)(w, 2);
    w = *(u8**)(arg0 + 0xA38); WSLOT(w, 0x30)(w, 2);
    w = *(u8**)(arg0 + 0xA44); WSLOT(w, 0x30)(w, 2);
    w = *(u8**)(arg0 + 0xA40); WSLOT(w, 0x30)(w, 2);

    if (fn_800B6848(*(u8**)(arg0 + 0x94)) == 3) {
        f32 val;
        if (fn_800B6850(*(u8**)(arg0 + 0x94)) == 1) {
            val = fn_800B7398(*(u8**)(arg0 + 0x94));
        } else {
            val = fn_800B7390(*(u8**)(arg0 + 0x94));
        }
        s32 sel = 0;
        if (fn_800B6850(*(u8**)(arg0 + 0x94)) == 1) {
            sel = 0;
        } else if (fn_800B6850(*(u8**)(arg0 + 0x94)) == 2) {
            sel = 1;
        }
        s32 idx = RaySceneTest(NULL);
        if (idx < 0) idx = 0;
        else if (idx > 3) idx = 3;
        if (sel == 0) {
            SysSprintf((char*)(arg0 + 0x894), (const char*)(lbl_8035E138 + 0xC0), val, lbl_80346098[idx]);
            w = *(u8**)(arg0 + 0xA44); WSLOT(w, 0x2C)(w, 2);
            w = *(u8**)(arg0 + 0xA40); WSLOT(w, 0x30)(w, 2);
        } else {
            SysSprintf((char*)(arg0 + 0x894), (const char*)(lbl_8035E138 + 0xE4), val, lbl_80346098[idx]);
            w = *(u8**)(arg0 + 0xA44); WSLOT(w, 0x30)(w, 2);
            w = *(u8**)(arg0 + 0xA40); WSLOT(w, 0x2C)(w, 2);
        }
    } else {
        if (fn_800B66F0(*(u8**)(arg0 + 0x94), 0) == 0) {
            SysSprintf((char*)(arg0 + 0x894), (const char*)(lbl_8035E138 + 0x10C));
            w = *(u8**)(arg0 + 0xA3C); WSLOT(w, 0x2C)(w, 2);
            w = *(u8**)(arg0 + 0xA38); WSLOT(w, 0x2C)(w, 2);
        } else {
            s32 sel = 0;
            if (fn_800B66F0(*(u8**)(arg0 + 0x94), 0) == 2) {
                sel = 1;
            } else if (fn_800B66F0(*(u8**)(arg0 + 0x94), 0) == 1) {
                sel = 0;
            }
            if (sel == 0) {
                SysSprintf((char*)(arg0 + 0x894), (const char*)(lbl_8035E138 + 0x124));
                w = *(u8**)(arg0 + 0xA3C); WSLOT(w, 0x2C)(w, 2);
                w = *(u8**)(arg0 + 0xA38); WSLOT(w, 0x30)(w, 2);
            } else {
                SysSprintf((char*)(arg0 + 0x894), (const char*)(lbl_8035E138 + 0x140));
                w = *(u8**)(arg0 + 0xA3C); WSLOT(w, 0x30)(w, 2);
                w = *(u8**)(arg0 + 0xA38); WSLOT(w, 0x2C)(w, 2);
            }
        }
    }

    s32 kindA = fn_800B73D0(*(u8**)(arg0 + 0x94));
    f32 fa = fn_800B7398(*(u8**)(arg0 + 0x94));
    f32 fb = fn_800B7388(*(u8**)(arg0 + 0x94));
    fn_800B8A68(*(u8**)(arg0 + 0x98), kindA, fb, fa);
    s32 kindB = fn_800B73A0(*(u8**)(arg0 + 0x94));
    f32 fc = fn_800B7390(*(u8**)(arg0 + 0x94));
    f32 fd = fn_800B7380(*(u8**)(arg0 + 0x94));
    fn_800B8A68(*(u8**)(arg0 + 0x9C), kindB, fd, fc);

    f32 s3 = fn_800B71A8(*(u8**)(arg0 + 0x94));
    f32 s2 = fn_800B7198(*(u8**)(arg0 + 0x94));
    f32 s1 = fn_800B71A0(*(u8**)(arg0 + 0x94));
    SysSprintf((char*)(arg0 + 0x998), (const char*)(lbl_8035E138 + 0x158), s1, s2, s3);
#undef WSLOT
}

// 0x800BCC24
// Spawns/places an effect object at pos with a random initial direction (+0x280) and an
// orientation quat facing away from ref's position, then notifies it (event 0x3ABE6EA2).
void fn_800BCC24(u8* arg0, f32* arg1, u8* arg2) {
    f32 px = arg1[0];
    f32 py = arg1[1];
    f32 pz = arg1[2];
    u8* t = *(u8**)(arg0 + 0x4);
    *(u8*)(arg0 + 0x298) = 1;
    *(f32*)(t + 0x0) = px;
    *(f32*)(t + 0x4) = py;
    *(f32*)(t + 0x8) = pz;
    MarkXformDirty(*(u8**)(arg0 + 0x4));
    *(u8**)(arg0 + 0x2A0) = arg2;
    *(f32*)(arg0 + 0x28C) = px;
    *(f32*)(arg0 + 0x290) = py;
    *(f32*)(arg0 + 0x294) = pz;

    // Random vector in [-1, 1]^3, then normalised (zero stays zero).
    f32 rz = lbl_805FE28C + lbl_805FE290 * (lbl_805FE288 * (f32)RandomU32());
    f32 ry = lbl_805FE28C + lbl_805FE290 * (lbl_805FE288 * (f32)RandomU32());
    f32 rx = lbl_805FE28C + lbl_805FE290 * (lbl_805FE288 * (f32)RandomU32());
    *(f32*)(arg0 + 0x284) = ry;
    *(f32*)(arg0 + 0x288) = rz;
    *(f32*)(arg0 + 0x280) = rx;
    f32 n[3];
    f32 sq = rz * rz + (rx * rx + ry * ry);
    if (lbl_805FE280 == sq) {
        n[2] = lbl_805FE280;
        n[1] = lbl_805FE280;
        n[0] = lbl_805FE280;
    } else {
        f32 inv = 1.0f / sqrtf(sq);   // original: frsqrte with two Newton steps
        n[0] = rx * inv;
        n[1] = ry * inv;
        n[2] = rz * inv;
    }
    *(f32*)(arg0 + 0x280) = n[0];
    *(f32*)(arg0 + 0x288) = n[2];
    *(f32*)(arg0 + 0x284) = n[1];

    // Unit vector from ref's position to pos.
    u8* rp = *(u8**)(arg2 + 0x4);
    f32 dx = arg1[0] - *(f32*)(rp + 0x0);
    f32 dy = arg1[1] - *(f32*)(rp + 0x4);
    f32 dz = arg1[2] - *(f32*)(rp + 0x8);
    f32 d[3];
    f32 sq2 = dz * dz + (dx * dx + dy * dy);
    if (lbl_805FE280 == sq2) {
        d[2] = lbl_805FE280;
        d[1] = lbl_805FE280;
        d[0] = lbl_805FE280;
    } else {
        f32 inv = 1.0f / sqrtf(sq2);
        d[0] = dx * inv;
        d[1] = dy * inv;
        d[2] = dz * inv;
    }

    f32 q[4];
    f32 side[3];
    if (arg1[0] < *(f32*)(*(u8**)(arg2 + 0x4) + 0x0)) {
        side[0] = lbl_805FE2A8 * d[2] - lbl_805FE280 * d[1];
        side[1] = lbl_805FE280 * d[0] - lbl_805FE280 * d[2];
        side[2] = lbl_805FE280 * d[1] - lbl_805FE2A8 * d[0];
    } else {
        side[0] = d[1] * lbl_805FE280 - d[2] * lbl_805FE2A8;
        side[1] = d[2] * lbl_805FE280 - d[0] * lbl_805FE280;
        side[2] = d[0] * lbl_805FE2A8 - d[1] * lbl_805FE280;
    }
    fn_801458B0(q, d, side);

    u8* o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0xC) = q[0];
    *(f32*)(o + 0x10) = q[1];
    *(f32*)(o + 0x14) = q[2];
    *(f32*)(o + 0x18) = q[3];
    MarkXformDirty(*(u8**)(arg0 + 0x4));
    EventSend(gEventMgr, arg0, NULL, 0x3ABE6EA2u, 0, 0, 1, 0);
    fn_801B6F34(arg0);
    *(f32*)(arg0 + 0x268) = lbl_805FE280;
}

}  // extern "C"
