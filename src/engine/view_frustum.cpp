// Scene-graph view: the eight frustum corners (+0x120, near then far) and six planes (+0xC0:
// near, far, and the four sides; 4 floats each) from the view's orientation (+0x00), position
// (+0x24), half extents (+0x180, +0x184) and near/far distances (+0x194, +0x198).
// Original: 0x8016E304, 0x8016EB6C (auto_03_8016E304_text.s)
#include <math.h>
#include "types.h"
#include "engine/math.h"

typedef f32 Scalar;

extern "C" {
void Vec3Copy(Vec3* out, const Vec3* v);
void Vec3Assign(Vec3* out, const Vec3* v);
void Vec3Negate(Vec3* out, const Vec3* v);
void Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s);
void Vec3ScaleBy(Vec3* v, const Scalar* s);
void Vec3AddTo(Vec3* v, const Vec3* d);
void Vec3SubFrom(Vec3* v, const Vec3* d);
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);
void Vec3Dot(Scalar* out, const Vec3* a, const Vec3* b);
void Vec3SetX(Vec3* v, f32 x);
void Vec3SetY(Vec3* v, f32 y);
void Vec3SetZ(Vec3* v, f32 z);
void ScalarSet(Scalar* out, f32 v);
void fn_80054484(Vec3* out, const Vec3* a, const Vec3* b);   // 0x80054484
void fn_8005C158(Vec3* out, const Vec3* v);                  // 0x8005C158
void fn_8005A244(Vec3* out, const Vec3* a, const Vec3* b);   // 0x8005A244: normalised cross product
void fn_80082F68(f32* out, f32* in);                         // 0x80082F68: negated 4-float vector
f32  fn_80082BE0(u8* a3, u8* a4);                            // 0x80082BE0: plane {v, constant} from a vector
f32  fn_8015EC78(u8* a3, u8* a4);                            // 0x8015EC78: copies 4 floats
f32  fn_80082F20(u8* a3, u8* a4);                            // 0x80082F20: copies 3 floats
void fn_8016EB0C(u8* a3, f32 f1, f32 f2);                    // 0x8016EB0C: {f1, f2, constant}
void fn_8016EB34(u8* a3, u8* a4);                            // 0x8016EB34: *a3 = a4[0]
void fn_8016EB24(u8* a3, u8* a4);                            // 0x8016EB24: *a3 = -a4[0]
void fn_8016EB40(u8* a3, u8* a4);                            // 0x8016EB40: *a3 = a4[1]
void fn_8016EB54(u8* a3);                                    // 0x8016EB54: *a3 = constant
void fn_8016EB60(u8* a3, u8* a4);                            // 0x8016EB60: a3[3] = *a4
}

extern "C" {
s32  fn_8015BA30(void* dl);                                  // 0x8015BA30: bytes written to a display list
void fn_8014DE68(void);                                      // 0x8014DE68: close the display list, open the next
void fn_801567CC(f32 halfX, f32 halfY, f32 n, f32 f);        // 0x801567CC: perspective projection
void fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);  // 0x801566E8: orthographic projection
void fn_80154E54(u8* viewMtx);                               // 0x80154E54: loads the view matrix
void fn_80155D70(void);                                      // 0x80155D70
void fn_8016DDAC(u8* p);                                     // 0x8016DDAC

extern u8   sRendDLs[];                 // 0x80401108 (display-list buffers, 0x94 bytes each)
extern s32  lbl_805FB4BC;               // 0x805FB4BC (current display list)
extern s32  lbl_805FB4C8;               // 0x805FB4C8
extern u8*  lbl_80412AF0[];             // 0x80412AF0 (camera stack)
extern void* lbl_805FB5F8;              // 0x805FB5F8 (current camera)
extern s32  lbl_805FB5FC;               // 0x805FB5FC (camera set counter)
extern s32  lbl_805FB600;               // 0x805FB600 (camera stack depth)
}

// 0x8016FB20
// Makes `view` the current camera (the previous one goes on the stack): derives its "billboard"
// axes (+0x30: the horizontal axis at right angles to the view direction, +0x3C, and +0x48 = up),
// keeps a copy of the orientation at +0x60, sets the projection, and builds the world-to-view
// matrix at +0x90 (the orientation's rows divided by their squared length, and the position
// carried through it), which goes to the renderer.
extern "C" void fn_8016FB20(u8* view) {
    f32* v = (f32*)view;
    lbl_80412AF0[lbl_805FB600] = (u8*)lbl_805FB5F8;
    lbl_805FB600++;
    lbl_805FB5F8 = view;

    f32 fx = v[6], fy = v[7], fz = v[8];        // +0x18: forward axis
    f32 nx = -fx, ny = -fy, nz = -fz;
    // (-forward) x (0, 0, 1)
    f32 cx = 0.0f * nz - 1.0f * ny;
    f32 cy = 1.0f * nx - 0.0f * nz;
    f32 cz = 0.0f * ny - 0.0f * nx;
    f32 len2 = cz * cz + (cx * cx + cy * cy);
    for (s32 i = 0; i < 6; i++) {
        v[0x18 + i] = v[i];                     // +0x60: first two axes
    }
    v[0x1E] = fx;
    v[0x1F] = fy;
    v[0x20] = fz;
    v[0x21] = 0.0f;                             // +0x84: no translation in the copy
    v[0x22] = 0.0f;
    v[0x23] = 0.0f;

    f32 sx, sy, sz;
    if (len2 == 0.0f) {
        sx = sy = sz = 0.0f;
    } else {
        // The original refines frsqrte twice; the result is 1 / sqrt to float precision.
        f32 r = (f32)(1.0 / sqrt((f64)len2));
        sx = cx * r;
        sy = cy * r;
        sz = cz * r;
    }
    v[0xC] = sx;
    v[0xD] = sy;
    v[0xE] = sz;
    // side x (0, 0, 1), negated
    f32 tx = sy * 1.0f - sz * 0.0f;
    f32 ty = sz * 0.0f - sx * 1.0f;
    f32 tz = sx * 0.0f - sy * 0.0f;
    v[0x12] = 0.0f;
    v[0x13] = 0.0f;
    v[0xF] = -tx;
    v[0x10] = -ty;
    v[0x11] = -tz;
    v[0x14] = 1.0f;
    v[0x15] = 0.0f;
    v[0x16] = 0.0f;
    v[0x17] = 0.0f;

    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x12A0 > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    if (*(s32*)(view + 0x188) != 0) {
        fn_801567CC(v[0x60], v[0x61], v[0x65], v[0x66]);
    } else {
        f32 hx = v[0x60];
        f32 hy = v[0x61];
        fn_801566E8(-hx, hx, -hy, hy, v[0x65], v[0x66]);
    }
    lbl_805FB5FC++;

    f32 ia = 1.0f / (v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]));
    f32 ib = 1.0f / (v[5] * v[5] + (v[3] * v[3] + v[4] * v[4]));
    f32 ic = 1.0f / (v[8] * v[8] + (v[6] * v[6] + v[7] * v[7]));
    f32 m00 = v[0] * ia, m01 = v[3] * ia, m02 = v[6] * ia;
    f32 m10 = v[1] * ib, m11 = v[4] * ib, m12 = v[7] * ib;
    f32 m20 = v[2] * ic, m21 = v[5] * ic, m22 = v[8] * ic;
    f32 px = -v[9], py = -v[10], pz = -v[11];
    v[0x24] = m00;
    v[0x25] = m01;
    v[0x26] = m02;
    v[0x27] = m10;
    v[0x28] = m11;
    v[0x29] = m12;
    v[0x2A] = m20;
    v[0x2B] = m21;
    v[0x2C] = m22;
    v[0x2D] = m20 * pz + (m00 * px + m10 * py);
    v[0x2E] = m21 * pz + (m01 * px + m11 * py);
    v[0x2F] = m22 * pz + (m02 * px + m12 * py);
    fn_80154E54(view + 0x90);
    fn_80155D70();
    fn_8016DDAC(view);
}

// 0x8016EB6C
// out[i] = M * in[i] + T for `count` vectors; M is the 3x3 at m (column vectors at +0, +0xC, +0x18)
// and T the vector at +0x24. `out` may be `in`.
extern "C" void fn_8016EB6C(const f32* m, f32* out, const f32* in, s32 count) {
    for (; count > 0; count--) {
        f32 x = in[0], y = in[1], z = in[2];
        in += 3;
        f32 ox = m[9] + (m[6] * z + (m[0] * x + m[3] * y));
        f32 oy = m[10] + (m[7] * z + (m[1] * x + m[4] * y));
        f32 oz = m[11] + (m[8] * z + (m[2] * x + m[5] * y));
        out[0] = ox;
        out[1] = oy;
        out[2] = oz;
        out += 3;
    }
}

static inline Vec3* V(u8* view, u32 off) { return (Vec3*)(view + off); }

// Plane through `point` with the normal already stored at `plane`.
static inline void PlaneThrough(u8* plane, const Vec3* point) {
    Vec3 n;
    Scalar d;
    fn_80082F20((u8*)&n, plane);
    Vec3Dot(&d, point, &n);
    fn_8016EB60(plane, (u8*)&d);
}

// Side plane: normal = normalised a x b, through `point`.
static inline void SidePlane(u8* plane, const Vec3* a, const Vec3* b, const Vec3* point) {
    Vec3 n;
    f32 p[4];
    fn_8005A244(&n, a, b);
    fn_80082BE0((u8*)p, (u8*)&n);
    fn_8015EC78(plane, (u8*)p);
    PlaneThrough(plane, point);
}

// 0x8016E304
extern "C" void fn_8016E304(u8* view) {
    f32 p[4];
    Scalar d;
    Vec3 e1, e2, t;

    if (*(s32*)(view + 0x188) != 0) {
        // Perspective: corners from the axes scaled by the half extents, then pushed out to the
        // near and far distances from the position.
        Vec3 half, right, up, fwd, corner;
        Scalar s;
        fn_8016EB0C((u8*)&half, *(f32*)(view + 0x180), *(f32*)(view + 0x184));
        fn_8005C158(&right, V(view, 0));        // fn_8016EB20(view) is view itself
        fn_8016EB34((u8*)&d, (u8*)&half);
        fn_8016EB24((u8*)&s, (u8*)&d);
        Vec3ScaleBy(&right, &s);
        fn_8005C158(&up, V(view, 0xC));         // fn_800554D4(view)
        fn_8016EB40((u8*)&s, (u8*)&half);
        Vec3ScaleBy(&up, &s);
        fn_8005C158(&fwd, V(view, 0x18));       // fn_8016EB4C(view)
        Vec3Negate(&t, &fwd);
        Vec3Copy(&fwd, &t);
        fn_80054484(&t, &fwd, &right);
        Vec3Copy(&corner, &t);
        Vec3AddTo(&corner, &up);
        fn_8016EB54((u8*)&s);
        Vec3ScaleBy(&right, &s);
        fn_8016EB54((u8*)&s);
        Vec3ScaleBy(&up, &s);
        Vec3Copy(V(view, 0x120), &corner);
        Vec3SubFrom(&corner, &right);
        Vec3Copy(V(view, 0x12C), &corner);
        Vec3SubFrom(&corner, &up);
        Vec3Copy(V(view, 0x138), &corner);
        Vec3AddTo(&corner, &right);
        Vec3Copy(V(view, 0x144), &corner);

        Scalar nearD, farD;
        ScalarSet(&nearD, *(f32*)(view + 0x194));
        ScalarSet(&farD, *(f32*)(view + 0x198));
        for (s32 i = 0; i < 4; i++) {
            Vec3 dir, scaled, at;
            Vec3* c = V(view, 0x120 + i * 0xC);
            Vec3Assign(&dir, c);
            Vec3MulScalar(&scaled, &dir, &nearD);
            fn_80054484(&at, V(view, 0x24), &scaled);       // fn_800526D4(view): the position
            Vec3Copy(c, &at);
            Vec3MulScalar(&scaled, &dir, &farD);
            fn_80054484(&at, V(view, 0x24), &scaled);
            Vec3Copy(V(view, 0x120 + (i + 4) * 0xC), &at);
        }

        fn_80082BE0((u8*)p, (u8*)&fwd);
        fn_8015EC78(view + 0xC0, (u8*)p);
        Vec3Dot(&d, V(view, 0x150), &fwd);
        fn_8016EB60(view + 0xC0, (u8*)&d);
        fn_80082F68(p, (f32*)(view + 0xC0));
        fn_8015EC78(view + 0xD0, (u8*)p);
        PlaneThrough(view + 0xD0, V(view, 0x120));

        Vec3Sub(&t, V(view, 0x12C), V(view, 0x15C));
        Vec3Copy(&e1, &t);
        Vec3Sub(&t, V(view, 0x168), V(view, 0x15C));
        Vec3Copy(&e2, &t);
        SidePlane(view + 0xE0, &e1, &e2, V(view, 0x12C));
        Vec3Sub(&t, V(view, 0x150), V(view, 0x15C));
        Vec3Copy(&e2, &t);
        SidePlane(view + 0xF0, &e2, &e1, V(view, 0x12C));
        Vec3Sub(&t, V(view, 0x144), V(view, 0x174));
        Vec3Copy(&e1, &t);
        Vec3Sub(&t, V(view, 0x150), V(view, 0x174));
        Vec3Copy(&e2, &t);
        SidePlane(view + 0x100, &e1, &e2, V(view, 0x144));
        Vec3Sub(&t, V(view, 0x168), V(view, 0x174));
        Vec3Copy(&e2, &t);
        SidePlane(view + 0x110, &e2, &e1, V(view, 0x144));
        return;
    }

    // Orthographic: a box in view space, transformed by the view's matrix.
    f32 nearZ = *(f32*)(view + 0x194);
    f32 farZ = *(f32*)(view + 0x198);
    Vec3SetZ(V(view, 0x120), nearZ);
    Vec3SetZ(V(view, 0x12C), nearZ);
    Vec3SetZ(V(view, 0x138), nearZ);
    Vec3SetZ(V(view, 0x144), nearZ);
    Vec3SetZ(V(view, 0x150), farZ);
    Vec3SetZ(V(view, 0x15C), farZ);
    Vec3SetZ(V(view, 0x168), farZ);
    Vec3SetZ(V(view, 0x174), farZ);
    f32 hx = *(f32*)(view + 0x180);
    Vec3SetX(V(view, 0x120), hx);
    Vec3SetX(V(view, 0x144), hx);
    Vec3SetX(V(view, 0x12C), -hx);
    Vec3SetX(V(view, 0x138), -hx);
    Vec3SetX(V(view, 0x150), hx);
    Vec3SetX(V(view, 0x174), hx);
    Vec3SetX(V(view, 0x15C), -hx);
    Vec3SetX(V(view, 0x168), -hx);
    f32 hy = *(f32*)(view + 0x184);
    Vec3SetY(V(view, 0x120), hy);
    Vec3SetY(V(view, 0x12C), hy);
    Vec3SetY(V(view, 0x138), -hy);
    Vec3SetY(V(view, 0x144), -hy);
    Vec3SetY(V(view, 0x150), hy);
    Vec3SetY(V(view, 0x15C), hy);
    Vec3SetY(V(view, 0x168), -hy);
    Vec3SetY(V(view, 0x174), -hy);
    fn_8016EB6C((const f32*)view, (f32*)(view + 0x120), (const f32*)(view + 0x120), 8);

    fn_80082BE0((u8*)p, view + 0x18);           // fn_8016EB4C(view): the forward axis
    fn_8015EC78(view + 0xC0, (u8*)p);
    PlaneThrough(view + 0xC0, V(view, 0x150));
    fn_80082F68(p, (f32*)(view + 0xC0));
    fn_8015EC78(view + 0xD0, (u8*)p);
    PlaneThrough(view + 0xD0, V(view, 0x120));

    Vec3Sub(&t, V(view, 0x12C), V(view, 0x15C));
    Vec3Copy(&e1, &t);
    Vec3Sub(&t, V(view, 0x168), V(view, 0x15C));
    Vec3Copy(&e2, &t);
    SidePlane(view + 0xE0, &e1, &e2, V(view, 0x12C));
    Vec3Sub(&t, V(view, 0x150), V(view, 0x15C));
    Vec3Copy(&e2, &t);
    SidePlane(view + 0xF0, &e2, &e1, V(view, 0x12C));
    fn_80082F68(p, (f32*)(view + 0xE0));
    fn_8015EC78(view + 0x100, (u8*)p);
    PlaneThrough(view + 0x100, V(view, 0x144));
    fn_80082F68(p, (f32*)(view + 0xF0));
    fn_8015EC78(view + 0x110, (u8*)p);
    PlaneThrough(view + 0x110, V(view, 0x144));
}
