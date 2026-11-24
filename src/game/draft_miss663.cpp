// Camera / HUD / collision helpers in 0x8007xxxx - 0x800Bxxxx.
#include "types.h"
#include "engine/math.h"

void fn_8009803C(u8* p, f32 dt);   // 0x8009803C (C++ linkage, defined in small_8009c.cpp)

extern "C" {
void  operator_delete(void* p);                                      // 0x801D426C
void  CameraSetMatrix(void* camera, const f32 mtx[3][4]);            // 0x801D6814
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);     // 0x80145EE8
void  QuatGetForward(Vec3* out, const Quat* q);                      // 0x801454CC
u32   RandomU32(void);                                               // 0x80143460
s32   RaySceneTest(void* ray);                                       // 0x80070458 (takes no argument: returns a global index)
void  GameFlowSignal(s32 id, s32 a, s32 b);                          // 0x8000C7F4
void  fn_80009840(u8* p);                                            // 0x80009840
void  fn_80070FAC(u8* p, f32 v);                                     // 0x80070FAC
void  fn_800712B4(void);                                             // 0x800712B4
void  fn_800712B8(void);                                             // 0x800712B8
void  fn_800712BC(u8* p, s32 a, s32 b);                              // 0x800712BC
void  fn_8016F854(f32* out, u8* camera, f32* pos);                   // 0x8016F854
void  fn_800977BC(u8* p, f32 dt);                                    // 0x800977BC
void  fn_800980C8(u8* p, s32 mode);                                  // 0x800980C8
void  fn_80095BEC(u8* p, f32 dt);                                    // 0x80095BEC
void  fn_80096038(u8* p, f32 dt);                                    // 0x80096038
void  fn_8009677C(u8* p, f32 dt);                                    // 0x8009677C
void  fn_80096BDC(u8* p);                                            // 0x80096BDC
u8*   fn_800964AC(u8* p, s32 mode);                                 // 0x800964AC
void  fn_80096D60(u8* p, f32 dt);                                    // 0x80096D60
s32   fn_80097064(f32* pos);                                         // 0x80097064
void  fn_800998E4(u8* obj, f32 angle);                               // 0x800998E4
void  fn_80099874(u8* p, u8* effect);                                // 0x80099874
void  fn_800995AC(u8* p, s32 b);                                     // 0x800995AC
f32   fn_800999B4(u8* obj, f32 v);                                   // 0x800999B4
void  fn_801EC080(u8* a, u8* b);                                     // 0x801EC080
void  fn_800A2E70(u8* self, u8* payload);                            // 0x800A2E70
void  fn_800A3188(u8* self, u8* node);                               // 0x800A3188
f32   fn_800B9914(u8* obj);                                          // 0x800B9914
void  fn_8009C1E8(f32* out, u8* p);                                  // 0x8009C1E8
void* fn_8009A47C(u8* p);                                            // 0x8009A47C
void  fn_8009CE40(u8* p, f32 dt);                                    // 0x8009CE40
void  fn_8009CBE8(u8* p, f32 dt);                                    // 0x8009CBE8
BOOL  fn_80162B30(u8* p, s32 h);                                     // 0x80162B30
void  fn_80162990(u8* p, s32 h);                                     // 0x80162990
void  fn_800C597C(f32* out, u8* p, f32 t);                           // 0x800C597C
void  fn_801455E0(f32* out, u8* quat);                               // 0x801455E0
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);          // 0x80146180
void  fn_801B7020(u8* p);                                            // 0x801B7020
u8*   fn_801ACFC4(u8* obj);                                          // 0x801ACFC4 (returns obj; only r3 is read)
void* fn_801ACCA0(u8* obj, s32 mode);                                // 0x801ACCA0
void  fn_800A23FC(u8* self, u8* node);                               // 0x800A23FC

extern s32 gScreenWidth;
extern s32 gScreenHeight;
extern u8  lbl_803C4298[];       // 0x803C4298
extern u8  lbl_8047C640[];       // 0x8047C640 (camera)
extern u8  lbl_8040E5F0[];       // 0x8040E5F0
extern void* lbl_805FB5F8;       // 0x805FB5F8 (current camera)
extern s32 lbl_80347150[];       // 0x80347150
extern s32 lbl_80347160[];       // 0x80347160
extern f32 lbl_80346FB0[];       // 0x80346FB0
extern f32 lbl_80347070[];       // 0x80347070
extern f32 lbl_80346FE0[];       // 0x80346FE0
extern f32 lbl_80346FF0[];       // 0x80346FF0
extern f32 lbl_80347000[];       // 0x80347000

extern f32 lbl_805FD5D8;         // 0.0
extern f32 lbl_805FD5F0;         // 1.0
extern f32 lbl_805FDB28;         // 0.0
extern f32 lbl_805FDB60;
extern f32 lbl_805FDB64;         // 1.0
extern f32 lbl_805FDB68;
extern f32 lbl_805FDB6C;
extern f32 lbl_805FDB70;
extern f32 lbl_805FDB74;
extern f32 lbl_805FDB78;
extern f32 lbl_805FDB7C;
extern f32 lbl_805FDD00;         // 6.0
extern f32 lbl_805FDD18;         // 0.0
extern f32 lbl_805FDD1C;         // 1.0
extern f32 lbl_805FDD28;         // 4.0
extern f32 lbl_805FDD40;
extern f32 lbl_805FDD58;         // 0.0001
extern f32 lbl_805FDD5C;         // 100000.0
extern f32 lbl_805FE178;         // 0.0
extern f32 lbl_805FE17C;         // 1.0
extern f32 lbl_805FE180;         // 10.0
extern f32 lbl_805FE184;
extern f32 lbl_805FE188;         // 5.0
extern f32 lbl_805FE18C;
extern f32 lbl_805FE190;

// Sets the four "dirty" flag bits in the transform block's flags word (+0x20), one OR at a time.
static void MarkTransformDirty(u8* obj) {
    *(u32*)(obj + 0x20) |= 0x8000;
    *(u32*)(obj + 0x20) |= 0x10000000;
    *(u32*)(obj + 0x20) |= 0x4000;
    *(u32*)(obj + 0x20) |= 0x10000000;
}

// Bilinear blend of the four vec3 corners at base: w0..w3 weight corners 0..3. Writes o[0..2].
static void BlendCorners(u8* base, f32 w0, f32 w1, f32 w2, f32 w3, f32* o) {
    for (s32 c = 0; c < 3; c++) {
        f32 c0 = *(f32*)(base + 0x00 + c * 4);
        f32 c1 = *(f32*)(base + 0x0C + c * 4);
        f32 c2 = *(f32*)(base + 0x18 + c * 4);
        f32 c3 = *(f32*)(base + 0x24 + c * 4);
        o[c] = ((c1 * w1 + c0 * w0) + c2 * w2) + c3 * w3;
    }
}

// Quaternion product r = a * b (x, y, z, w order), in the operation order the game uses.
static void QuatMulXyzw(const f32* a, const f32* b, f32* r) {
    f32 ax = a[0], ay = a[1], az = a[2], aw = a[3];
    f32 bx = b[0], by = b[1], bz = b[2], bw = b[3];
    f32 w = ((aw * bw - ax * bx) - ay * by) - az * bz;
    f32 x = (ay * bz + (aw * bx + ax * bw)) - az * by;
    f32 y = az * bx + (ay * bw + (aw * by - ax * bz));
    f32 z = az * bw + ((aw * bz + ax * by) - ay * bx);
    r[0] = x;
    r[1] = y;
    r[2] = z;
    r[3] = w;
}

// Camera offset behind the orientation q: -p[0x2F8] along the forward vector plus -p[0x2FC]
// along the second axis, times 4, clamped to a length of 6. Writes v[0..2].
static void CameraOffset(u8* p, f32* q, f32* v) {
    f32 axis[3];
    Vec3 fwd;
    fn_801455E0(axis, (u8*)q);
    QuatGetForward(&fwd, (const Quat*)q);
    f32 n1 = -*(f32*)(p + 0x2F8);
    f32 n2 = -*(f32*)(p + 0x2FC);
    f32 x = (fwd.x * n1 + axis[0] * n2) * lbl_805FDD28;
    f32 y = (fwd.y * n1 + axis[1] * n2) * lbl_805FDD28;
    f32 z = (fwd.z * n1 + axis[2] * n2) * lbl_805FDD28;
    f32 len2 = z * z + (x * x + y * y);
    f32 len = lbl_805FDD18;
    if (len2 > lbl_805FDD18) len = sqrtf(len2);
    if (len > lbl_805FDD00) {
        f32 l2 = z * z + (x * x + y * y);
        f32 nx = lbl_805FDD18, ny = lbl_805FDD18, nz = lbl_805FDD18;
        if (l2 != lbl_805FDD18) {
            f32 inv = 1.0f / sqrtf(l2);
            nx = x * inv;
            ny = y * inv;
            nz = z * inv;
        }
        x = nx * lbl_805FDD00;
        y = ny * lbl_805FDD00;
        z = nz * lbl_805FDD00;
    }
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

// 0x800734B4
// Where a world point lies between the two bilinear frustum planes of a camera: the point's
// screen position picks the blend, and the result is |pos - near| / |far - near| clamped to
// 0..1 (0 when the far point equals the near one). cam == NULL loads the current camera first.
f32 fn_800734B4(u8* cam, f32* pos) {
    f32 mtx[3][4];
    f32 pt[3];
    f32 uv[3];                                       // fn_8016F854 writes three ratios
    f32 a[3];
    f32 b[3];

    if (cam == NULL) {
        u8* obj = *(u8**)(lbl_803C4298 + 4);
        (*(void (**)(u8*, f32 (*)[4]))(*(u8**)obj + 0x38))(obj, mtx);
        CameraSetMatrix(lbl_8047C640, mtx);
        fn_80009840(lbl_803C4298);
        cam = (u8*)lbl_805FB5F8;
    }
    pt[0] = pos[0];
    pt[1] = pos[1];
    pt[2] = pos[2];
    if (cam == NULL) {
        u8* obj = *(u8**)(lbl_803C4298 + 4);
        (*(void (**)(u8*, f32 (*)[4]))(*(u8**)obj + 0x38))(obj, mtx);
        CameraSetMatrix(lbl_8047C640, mtx);
        fn_80009840(lbl_803C4298);
        cam = (u8*)lbl_805FB5F8;
    }
    fn_8016F854(uv, cam, pt);

    f32 w = (f32)gScreenWidth;
    f32 u = (w * uv[0]) / (f32)gScreenWidth;
    if (u < lbl_805FD5D8) u = lbl_805FD5D8;
    else if (u > lbl_805FD5F0) u = lbl_805FD5F0;

    f32 h = (f32)gScreenHeight;
    f32 v = lbl_805FD5F0 - (h * uv[1]) / (f32)gScreenHeight;
    if (v < lbl_805FD5D8) v = lbl_805FD5D8;
    else if (v > lbl_805FD5F0) v = lbl_805FD5F0;

    f32 w1 = u * v;
    f32 w0 = (lbl_805FD5F0 - u) * v;
    f32 w2 = u * (lbl_805FD5F0 - v);
    f32 w3 = (lbl_805FD5F0 - u) * (lbl_805FD5F0 - v);
    BlendCorners(cam + 0x120, w0, w1, w2, w3, a);   // near corners
    BlendCorners(cam + 0x150, w0, w1, w2, w3, b);   // far corners

    f32 dx = b[0] - a[0];
    f32 dy = b[1] - a[1];
    f32 dz = b[2] - a[2];
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 farLen = lbl_805FD5D8;
    if (len2 > lbl_805FD5D8) farLen = sqrtf(len2);

    f32 ex = pos[0] - a[0];
    f32 ey = pos[1] - a[1];
    f32 ez = pos[2] - a[2];
    f32 dlen2 = ez * ez + (ex * ex + ey * ey);
    f32 nearLen = lbl_805FD5D8;
    if (dlen2 > lbl_805FD5D8) nearLen = sqrtf(dlen2);

    if (farLen != farLen || lbl_805FD5D8 == farLen) return lbl_805FD5D8;
    f32 t = nearLen / farLen;
    if (t < lbl_805FD5D8) return lbl_805FD5D8;
    if (t > lbl_805FD5F0) return lbl_805FD5F0;
    return t;
}

// 0x800952D0
// Per-frame update of the experiment HUD object p: reads the vtable slot 0x64 value (as a
// percentage), raises the game-flow signals when it runs out or the timer at +0x358 expires,
// ticks the 27 sub-records at +0x384 and the objects in the list at +0x9C, and drives the
// effect state at +0x36C / +0x374 and the per-effect-level counters.
void fn_800952D0(u8* p, f32 dt) {
    s32 counts[4];

    f32 v = (*(f32 (**)(u8*))(*(u8**)(p + 4) + 0x64))(p);
    fn_80070FAC(p, v / lbl_805FDB60);
    if (v <= lbl_805FDB28) {
        GameFlowSignal(0x117741C3, 0, 0);   // hash, unresolved
        return;
    }
    f32 t = *(f32*)(p + 0x358) - dt;
    *(f32*)(p + 0x358) = t;
    if (t <= lbl_805FDB28) {
        if (v > lbl_805FDB28) GameFlowSignal((s32)0xC608C842, 0, 0);   // hash, unresolved
        else GameFlowSignal(0x117741C3, 0, 0);
    }
    f32 timer = *(f32*)(p + 0x358);
    fn_800712BC(p, (s32)timer, timer <= lbl_805FDB68);

    u8* rec = p + 0x384;
    for (s32 i = 0; i < 0x1B; i++) {
        fn_800977BC(rec, dt);
        rec += 0x20;
    }
    u8* blk = *(u8**)(p + 0x90);
    fn_801EC080(blk + 0x94, blk);
    fn_80095BEC(p, dt);
    for (s32 i = 0; i < *(s32*)(p + 0x1B4); i++) {
        fn_8009803C(*(u8**)(p + 0x9C + i * 4), dt);
    }
    fn_80096038(p, dt);
    fn_8009677C(p, dt);

    f32 fx = *(f32*)(p + 0x36C);
    if (fx > lbl_805FDB28) {
        fx -= dt;
        *(f32*)(p + 0x36C) = fx;
        if (*(u8*)(p + 0x370) != 0) {
            if (fx < lbl_805FDB28) {
                u8* cur = *(u8**)(p + 0x1D8);
                *(u8*)(p + 0x370) = 0;
                *(f32*)(p + 0x36C) = lbl_805FDB28;
                if (cur != NULL) {
                    fn_800980C8(cur, 0);
                    *(u8**)(p + 0x1DC) = *(u8**)(p + 0x1D8);
                }
                *(u8**)(p + 0x1D8) = NULL;
            }
        } else if (*(u8**)(p + 0x1D8) != NULL) {
            f32 lvl = *(f32*)(*(u8**)(p + 0x90) + 0x270);
            if (lvl >= lbl_80346FB0[RaySceneTest(NULL)]) {   // r3 left over, callee takes no argument
                fn_80096BDC(p);
            }
        }
    }

    f32 cd = *(f32*)(p + 0x374);
    if (cd > lbl_805FDB28) *(f32*)(p + 0x374) = cd - dt;
    if (*(f32*)(p + 0x374) < lbl_805FDB28) {
        u8* cur = *(u8**)(p + 0x1D8);
        if (cur != NULL && (s32)*(u8*)(cur + 0x26C) > 0) {
            f32 step = lbl_80347070[RaySceneTest(NULL)];   // r3 left over, callee takes no argument
            if (step > lbl_805FDB28) fn_800712B8();
            else if (step < lbl_805FDB28) fn_800712B4();
            f32 g = *(f32*)(p + 0x35C) + step;
            f32 clamped = lbl_805FDB28;
            if (g >= lbl_805FDB28) {
                clamped = lbl_805FDB64;
                if (g <= lbl_805FDB64) clamped = g;
            }
            *(f32*)(p + 0x35C) = clamped;
        }
        s32 k = (s32)(RandomU32() & 7);
        u8* found = fn_800964AC(p, 1);
        if (found != NULL) {
            f32 pos[3];
            u8* src = *(u8**)(found + 4);
            pos[0] = *(f32*)(src + 0);
            pos[1] = *(f32*)(src + 4);
            pos[2] = *(f32*)(src + 8);
            k = fn_80097064(pos);
        }
        s32 slot = *(s32*)(p + 0x1B8 + k * 4);
        f32 fk = (f32)k;
        f32 ang = lbl_805FDB6C + (lbl_805FDB70 * fk) * lbl_805FDB74;
        fn_800998E4(*(u8**)(p + 0x90), ang);
        s32 nk = (k + 4) % 8;
        fn_80099874(*(u8**)(p + 0x90), *(u8**)(p + 0x194 + nk * 4));
        fn_800995AC(*(u8**)(p + 0x90), slot);
        if (*(u8**)(p + 0x1D8) != NULL) {
            fn_800980C8(*(u8**)(p + 0x1D8), 0);
            *(u8**)(p + 0x1DC) = *(u8**)(p + 0x1D8);
        }
        *(u8**)(p + 0x1D8) = NULL;
        *(f32*)(p + 0x374) = lbl_805FDB78;
    }

    fn_800999B4(*(u8**)(p + 0x90), lbl_805FDB7C * dt);

    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    for (s32 i = 0; i < *(s32*)(p + 0x1B4); i++) {
        u8* o = *(u8**)(p + 0x9C + i * 4);
        s32 lvl = *(u8*)(o + 0x26C);
        s32 idx;
        if (lvl > lbl_80347160[RaySceneTest(NULL)]) idx = 3;   // r3 left over, callee takes no argument
        else if (lvl > lbl_80347150[RaySceneTest(NULL)]) idx = 2;
        else idx = (lvl != 0);
        counts[idx]++;
    }
    fn_800999B4(*(u8**)(p + 0x90), dt * ((f32)counts[1] * lbl_80346FE0[RaySceneTest(NULL)]));
    fn_800999B4(*(u8**)(p + 0x90), dt * ((f32)counts[2] * lbl_80346FF0[RaySceneTest(NULL)]));
    fn_800999B4(*(u8**)(p + 0x90), dt * ((f32)counts[3] * lbl_80347000[RaySceneTest(NULL)]));
    fn_80096D60(p, dt);
}

// 0x8009C618
// Per-frame camera follow for p: while the +0x3F0 flag is set it only waits for the handle at
// +0x3F8 to finish (then releases it). Otherwise it runs the sub-updates, takes the target's
// orientation (turned by -p[0x304] about +Y when the target object has its flag set), places the
// camera object at p+4 (rotation = target * p[0x3D8..], position = target + CameraOffset).
void fn_8009C618(u8* p, f32 dt) {
    f32 cur[4];     // x, y, z, w
    f32 rot[4];
    f32 axis[3];
    f32 pos[3];
    f32 off[3];

    if (*(u8*)(p + 0x3F0) != 0) {
        if (*(s32*)(p + 0x3F8) != 0) {
            if (fn_80162B30(lbl_8040E5F0, *(s32*)(p + 0x3F8)) == 0) {
                *(u8*)(p + 0x3F0) = 0;
                *(s32*)(p + 0x3F8) = 0;
                fn_80162990(lbl_8040E5F0, *(s32*)(p + 0x3FC));
                *(s32*)(p + 0x3FC) = 0;
            }
        }
        return;
    }

    fn_8009CE40(p, dt);
    if (*(u8*)(p + 0x3F0) == 0) fn_8009CBE8(p, dt);
    fn_8009C1E8(cur, *(u8**)(p + 0x264) + 0x9C);
    u8* tgt = (u8*)fn_8009A47C(*(u8**)(p + 0x264));
    if (tgt != NULL && *(u8*)(tgt + 0x5D) != 0) {
        axis[0] = lbl_805FDD18;
        axis[1] = lbl_805FDD1C;
        axis[2] = lbl_805FDD18;
        QuatFromAxisAngle((Quat*)rot, (const Vec3*)axis, lbl_805FDD40 * -*(f32*)(p + 0x304));
        QuatMulXyzw(cur, rot, cur);
    }
    f32 base[4];
    base[0] = *(f32*)(p + 0x3D8);
    base[1] = *(f32*)(p + 0x3DC);
    base[2] = *(f32*)(p + 0x3E0);
    base[3] = *(f32*)(p + 0x3E4);
    u8* obj = *(u8**)(p + 4);
    f32 q[4];
    QuatMulXyzw(cur, base, q);
    *(f32*)(obj + 0xC) = q[0];
    *(f32*)(obj + 0x10) = q[1];
    *(f32*)(obj + 0x14) = q[2];
    *(f32*)(obj + 0x18) = q[3];
    MarkTransformDirty(*(u8**)(p + 4));

    u8* tobj = *(u8**)(p + 0x264) + 0x9C;
    (*(void (**)(u8*))(*(u8**)(tobj + 0xD4) + 0x20))(tobj);
    (*(void (**)(f32*, u8*))(*(u8**)(tobj + 0xD4) + 0x24))(pos, tobj);
    CameraOffset(p, cur, off);

    obj = *(u8**)(p + 4);
    *(f32*)(obj + 0) = pos[0] + off[0];
    *(f32*)(obj + 4) = pos[1] + off[1];
    *(f32*)(obj + 8) = pos[2] + off[2];
    MarkTransformDirty(*(u8**)(p + 4));
}

// 0x8009D130
// Tests whether the camera for the viewer p stays clear of the geometry of q: builds a query
// object, places it at the target position plus CameraOffset, and over every sub-shape of q's
// object list takes the smallest clearance (radius - (distance - 0.0001)). Stores it to *out
// (100000.0 when nothing was hit) and returns 1 when no shape was hit.
s32 fn_8009D130(u8* p, u8* q, f32* out) {
    alignas(16) u8 query[0x40];
    u8 params[0x40];
    f32 tpos[3];
    f32 rotq[4];
    f32 off[3];
    f32 hitA[7];
    f32 hitB[7];

    for (s32 i = 0; i < 0x40; i++) params[i] = 0;   // asm zeroes the used fields only
    fn_801ACFC4(query);
    (*(void (**)(u8*, u8*, s32, s32))(*(u8**)query + 0xC))(query, params, 0, 0);

    f32 t = *(f32*)(q + 0x264);
    u8* tobj = *(u8**)(p + 0x264) + 0x9C;
    (*(void (**)(f32*, u8*, f32))(*(u8**)(tobj + 0xD4) + 0x24))(tpos, tobj, t);
    fn_800C597C(rotq, *(u8**)(p + 0x264) + 0x9C, t);
    CameraOffset(p, rotq, off);

    *(f32*)(query + 0x14) = tpos[0] + off[0];
    *(f32*)(query + 0x18) = tpos[1] + off[1];
    *(f32*)(query + 0x1C) = tpos[2] + off[2];
    *(f32*)(query + 0x3C) = lbl_805FDD58;
    fn_801B7020(q);

    s32 result = 1;
    u8* list = *(u8**)(q + 0x8C);
    f32 best = lbl_805FDD5C;
    for (u32 i = 0; i < *(u32*)(list + 0x10); i++) {
        u8* group = (i >= *(u32*)(list + 0x10)) ? NULL : *(u8**)(*(u8**)(list + 0xC) + i * 4);
        if (*(s32*)(group + 0x28) == 0) continue;
        for (u32 j = 0; j < *(u32*)(group + 0x28); j++) {
            for (s32 k = 0; k < 7; k++) {
                hitA[k] = lbl_805FDD18;
                hitB[k] = lbl_805FDD18;
            }
            u8* shape = (j >= *(u32*)(group + 0x28)) ? NULL : *(u8**)(*(u8**)(group + 0x24) + j * 4);
            s32 hit = (*(s32 (**)(u8*, f32*, u8*, f32*, s32, s32, s32))(*(u8**)shape + 0x14))(
                shape, hitA, query, hitB, 0, 0, 0);
            if (hit != 0) {
                result = 0;
                f32 dx = *(f32*)(shape + 0x14) - *(f32*)(query + 0x14);
                f32 dy = *(f32*)(shape + 0x18) - *(f32*)(query + 0x18);
                f32 dz = *(f32*)(shape + 0x1C) - *(f32*)(query + 0x1C);
                f32 rad = *(f32*)(shape + 0x3C);
                f32 len2 = dz * dz + (dx * dx + dy * dy);
                f32 len = lbl_805FDD18;
                if (len2 > lbl_805FDD18) len = sqrtf(len2);
                f32 clear = rad - (len - lbl_805FDD58);
                if (clear < best) best = clear;
            }
        }
    }
    *out = best;
    fn_801ACCA0(query, 0);
    return result;
}

// Frees a leaf-level tree node of the camera-shape tree: recurses into its children, then the
// payload and the node (levels 1 to 3 below the root were inlined by the compiler).
static void FreeShapeNodeL3(u8* self, u8* n) {
    if (n == NULL) return;
    u8* c = *(u8**)(n + 0);
    if (c != NULL) {
        fn_800A23FC(self, c);
    }
    c = *(u8**)(n + 4);
    if (c != NULL) {
        fn_800A23FC(self, c);
    }
    fn_800A2E70(self, n + 0xC);
    operator_delete(n);
}

static void FreeShapeNodeL2(u8* self, u8* n) {
    if (n == NULL) return;
    FreeShapeNodeL3(self, *(u8**)(n + 0));
    FreeShapeNodeL3(self, *(u8**)(n + 4));
    fn_800A2E70(self, n + 0xC);
    operator_delete(n);
}

static void FreeShapeNodeL1(u8* self, u8* n) {
    if (n == NULL) return;
    FreeShapeNodeL2(self, *(u8**)(n + 0));
    FreeShapeNodeL2(self, *(u8**)(n + 4));
    fn_800A2E70(self, n + 0xC);
    operator_delete(n);
}

// Second tree (rooted at +0x14): the deepest nodes pass their children to fn_800A3188.
static void FreeIndexNodeD3(u8* self2, u8* n) {
    if (n == NULL) return;
    u8* c = *(u8**)(n + 0);
    if (c != NULL) fn_800A3188(self2, c);
    c = *(u8**)(n + 4);
    if (c != NULL) fn_800A3188(self2, c);
    operator_delete(n);
}

static void FreeIndexNodeD2(u8* self2, u8* n) {
    if (n == NULL) return;
    FreeIndexNodeD3(self2, *(u8**)(n + 0));
    FreeIndexNodeD3(self2, *(u8**)(n + 4));
    operator_delete(n);
}

static void FreeIndexNodeD1(u8* self2, u8* n) {
    if (n == NULL) return;
    FreeIndexNodeD2(self2, *(u8**)(n + 0));
    FreeIndexNodeD2(self2, *(u8**)(n + 4));
    operator_delete(n);
}

// 0x800A23FC
// Destroys a node of the two-tree structure: frees the subtrees at +0 and +4 (each node gives
// its payload at +0xC to fn_800A2E70 before it is deleted), then the second tree rooted at
// +0x14 (its leaves go to fn_800A3188 with self2 = node+0x10), then the node itself.
void fn_800A23FC(u8* self, u8* node) {
    FreeShapeNodeL1(self, *(u8**)(node + 0));
    FreeShapeNodeL1(self, *(u8**)(node + 4));

    // The asm tests (node + 0xC) and (node + 0x10) against NULL; both are always true.
    u8* root = *(u8**)(node + 0x14);
    if (root != NULL) {
        u8* self2 = node + 0x10;
        FreeIndexNodeD1(self2, *(u8**)(root + 0));
        FreeIndexNodeD1(self2, *(u8**)(root + 4));
        operator_delete(root);
    }
    operator_delete(node);
}

// 0x800B6A88
// Eases the tilt angle p[0x2C4] toward the target given by p[0x29C] + p[0x2A0] (limited to
// +-0.5555556, step capped at 10), rebuilds the quaternion at p+0x27C from it about the axis
// of p+0x28C, writes it to the object at p+4 and places that object p[0x2F4] behind the pivot
// p[0x264..0x26C] along the quaternion's axis. Skipped while the mode at +0x2B4 is 3.
void fn_800B6A88(u8* p, f32 dt) {
    if (*(s32*)(p + 0x2B4) != 3) {
        f32 a = *(f32*)(p + 0x29C);
        f32 b = *(f32*)(p + 0x2A0);
        s32 bigA = 0;
        if (fabsf(a) > fabsf(b)) bigA = 1;
        f32 speed = fabsf(*(f32*)(p + 0x29C) + *(f32*)(p + 0x2A0)) / lbl_805FE188;
        if (bigA != 0 && *(s32*)(p + 0x2FC) != -1) {
            u8* e = *(u8**)(p + 0x2DC + *(s32*)(p + 0x2FC) * 4);
            f32 diff = fabsf(*(f32*)(p + 0x2C4) - fn_800B9914(*(u8**)e));
            if (diff < (lbl_805FE184 * (lbl_805FE188 * speed)) * dt) speed = diff;
        }
        if (*(f32*)(p + 0x29C) + *(f32*)(p + 0x2A0) < lbl_805FE178) {
            if (speed > lbl_805FE180) speed = lbl_805FE180;
            f32 ang = *(f32*)(p + 0x2C4) + (lbl_805FE184 * (lbl_805FE188 * speed)) * dt;
            *(f32*)(p + 0x27C) = lbl_805FE178;
            *(f32*)(p + 0x280) = lbl_805FE178;
            *(f32*)(p + 0x284) = lbl_805FE178;
            *(f32*)(p + 0x288) = lbl_805FE17C;
            *(f32*)(p + 0x2C4) = ang;
            if (ang > lbl_805FE190) *(f32*)(p + 0x2C4) = lbl_805FE190;
            Vec3 axis = QuatGetAxisY((const Quat*)(p + 0x28C));
            fn_80146180(p + 0x27C, &axis, *(f32*)(p + 0x2C4));
            u8* obj = *(u8**)(p + 4);
            *(f32*)(obj + 0xC) = *(f32*)(p + 0x27C);
            *(f32*)(obj + 0x10) = *(f32*)(p + 0x280);
            *(f32*)(obj + 0x14) = *(f32*)(p + 0x284);
            *(f32*)(obj + 0x18) = *(f32*)(p + 0x288);
            MarkTransformDirty(*(u8**)(p + 4));
        } else {
            if (speed > lbl_805FE180) speed = lbl_805FE180;
            f32 ang = *(f32*)(p + 0x2C4) - (lbl_805FE184 * (lbl_805FE188 * speed)) * dt;
            *(f32*)(p + 0x27C) = lbl_805FE178;
            *(f32*)(p + 0x280) = lbl_805FE178;
            *(f32*)(p + 0x284) = lbl_805FE178;
            *(f32*)(p + 0x288) = lbl_805FE17C;
            *(f32*)(p + 0x2C4) = ang;
            if (ang < lbl_805FE18C) *(f32*)(p + 0x2C4) = lbl_805FE18C;
            Vec3 axis = QuatGetAxisY((const Quat*)(p + 0x28C));
            fn_80146180(p + 0x27C, &axis, *(f32*)(p + 0x2C4));
            u8* obj = *(u8**)(p + 4);
            *(f32*)(obj + 0xC) = *(f32*)(p + 0x27C);
            *(f32*)(obj + 0x10) = *(f32*)(p + 0x280);
            *(f32*)(obj + 0x14) = *(f32*)(p + 0x284);
            *(f32*)(obj + 0x18) = *(f32*)(p + 0x288);
            MarkTransformDirty(*(u8**)(p + 4));
        }
    }
    f32 dist = *(f32*)(p + 0x2F4);
    f32 dir[3];
    fn_801455E0(dir, p + 0x27C);
    u8* obj = *(u8**)(p + 4);
    *(f32*)(obj + 0) = *(f32*)(p + 0x264) - dir[0] * dist;
    *(f32*)(obj + 4) = *(f32*)(p + 0x268) - dir[1] * dist;
    *(f32*)(obj + 8) = *(f32*)(p + 0x26C) - dir[2] * dist;
    MarkTransformDirty(*(u8**)(p + 4));
}

}  // extern "C"
