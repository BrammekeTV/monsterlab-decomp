// Drafts miss443: aim-ray target/spark spawn (0x800893C4), camera-follow tracker (0x8009EC30),
// and the larger object routines 0x8009FE24, 0x800B387C, 0x800BAA84.
#include <math.h>
#include <string.h>

#include "types.h"
#include "engine/math.h"

extern "C" {
u32   RandomU32(void);                                            // 0x80143460
s32   RaySceneTest(void* ray);                                    // 0x80070458
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32   fn_8008A58C(u8* p);                                         // 0x8008A58C
void  fn_8008A768(u8* p, s32* a, s32* b, s32* c);                 // 0x8008A768
s32   fn_8008B264(u8* p, s32 a);                                  // 0x8008B264
void  fn_8008CE38(u8* p);                                         // 0x8008CE38 (leaf: sets +0x264 = 2, r3 unchanged)
void  fn_8008D25C(u8* p, u8* a);                                  // 0x8008D25C
void  fn_8008D294(u8* p, s32 a);                                  // 0x8008D294
void  fn_8008D6A0(u8* p, f32* a);                                 // 0x8008D6A0
void  fn_8008D7B0(u8* p, s32 a);                                  // 0x8008D7B0
void  fn_8008D7B8(u8* p, f32 a);                                  // 0x8008D7B8
void  fn_8008D7C0(u8* p, f32 a);                                  // 0x8008D7C0
void  fn_8006F794(u8* self, u8* node);                            // 0x8006F794
void  fn_8006FAF4(u8* p, s32 a);                                  // 0x8006FAF4
void  fn_8009C1E8(f32* out, u8* p);                               // 0x8009C1E8
void  fn_800A23FC(u8* self, u8* node);                            // 0x800A23FC
void  fn_800A2E70(void* unused, u8* tree);                        // 0x800A2E70
void  fn_800A3188(u8* self, u8* node);                            // 0x800A3188
void  fn_800A6630(u8* p);                                         // 0x800A6630
u8*   fn_800C210C(u8* p, s32 del);                                // 0x800C210C
u8*   fn_800C42FC(u8* p, s32 del);                                // 0x800C42FC
s32   fn_800DECD8(u8* p, f32 dt);                                 // 0x800DECD8
void  operator_delete(void* p);                                   // 0x801D426C
void  fn_8020A398(s32 handle);                                    // 0x8020A398
u8    fn_800B9A78(u8* p);                                         // 0x800B9A78
s32   fn_800B964C(u8* obj);                                       // 0x800B964C
void  fn_800B999C(u8* obj, s32 a, s32 b);                         // 0x800B999C
void  fn_800B990C(u8* obj, f32 a);                                // 0x800B990C
f32   fn_800B68B8(u8* p);                                         // 0x800B68B8
s32   fn_800B4070(u8* p, s32* out, f32 t);                               // 0x800B4070
f64   floor(f64 x);                                         // 0x8033ED6C (MSL floor)
void  GameFlowSignal(s32 id, s32 arg1, s32 arg2);                 // 0x8000C7F4
void  StickRead(s32 channel, f32* x, f32* y);                     // 0x80147704
BOOL  fn_80147554(s32 button);                                    // 0x80147554
u8*   fn_80073454(void);                                          // 0x80073454 (camera/frustum block)
void  fn_80073F58(f32* out, u8* cam, f32* pt, f32* a, f32* b, f32 x, f32 y);   // 0x80073F58 (only f1, f2 are read)
void  fn_800715B8(u8* p, s32 a);                                  // 0x800715B8
void  fn_800716D0(u8* p, u32 a);                                  // 0x800716D0
void  fn_800712BC(u8* p, s32 a, s32 b);                           // 0x800712BC
void  fn_80070FAC(u8* p, f32 v);                                  // 0x80070FAC
void  fn_800BB658(u8* p, f32 dt);                                 // 0x800BB658
s32   fn_800BC050(u8* p);                                         // 0x800BC050
void  fn_800BC388(u8* p);                                         // 0x800BC388
s32   fn_800BC704(u8* p, f32* out);                               // 0x800BC704
void  fn_800BD3D4(u8* self, u8* other);                           // 0x800BD3D4
void  fn_800BD984(u8* obj, f32* pos, f32 v);                      // 0x800BD984
void  fn_800BE148(f32* out, u8* a);                               // 0x800BE148
void  fn_800BE5EC(u8* obj, f32* a, f32* b);                       // 0x800BE5EC
void  fn_800BF878(u8* p, f32 a);                                  // 0x800BF878
void  fn_800C0660(u8* p, f32* pos);                               // 0x800C0660
void  fn_801455E0(f32* out, u8* quat);                         // 0x801455E0
void  fn_801458B0(f32* quat, f32* a, f32* b);                     // 0x801458B0
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);               // 0x80145F9C
f64   fn_8033F1F4(f64 x);                                         // 0x8033F1F4 (MSL acos)

extern f32 lbl_80346208[];                // 0x80346208
extern f32 lbl_80346218[];                // 0x80346218
extern f32 lbl_803460E8[];                // 0x803460E8
extern f32 lbl_80346148[];                // 0x80346148
extern f32 lbl_80346158[];                // 0x80346158
extern f32 lbl_803461A8[];                // 0x803461A8
extern f32 lbl_803461B8[];                // 0x803461B8
extern f32 lbl_803460F8[];                // 0x803460F8
extern f32 lbl_80346168[];                // 0x80346168
extern f32 lbl_80346178[];                // 0x80346178
extern f32 lbl_803461C8[];                // 0x803461C8
extern f32 lbl_803461D8[];                // 0x803461D8
extern f32 lbl_80346108[];                // 0x80346108
extern f32 lbl_80346188[];                // 0x80346188
extern f32 lbl_80346198[];                // 0x80346198
extern f32 lbl_803461E8[];                // 0x803461E8
extern f32 lbl_803461F8[];                // 0x803461F8
extern f32 lbl_80346D50[];          // 0x80346D50
extern f32 lbl_80346ED0[];          // 0x80346ED0
extern f32 lbl_80346F00[];          // 0x80346F00
extern f32 lbl_80346F10[];          // 0x80346F10
extern s32 gScreenWidth;            // 0x805F9388
extern s32 gScreenHeight;           // 0x805F938C
extern s32 lbl_805F8A04;           // 0x805F8A04
extern f32 lbl_803467D0[];          // 0x803467D0
extern f32 lbl_803467E4[];          // 0x803467E4
extern f32 lbl_8034680C[];          // 0x8034680C
extern f32 lbl_80346820[];          // 0x80346820
extern u8  lbl_803C4298[];          // 0x803C4298
extern u8  lbl_8035CF88[];          // 0x8035CF88 (vtable)
extern u8* lbl_805FB158;            // 0x805FB158
extern u8  gWorldState[];           // 0x803E6D00 (first word: world block pointer)
extern "C++" s32 gZoneResourceSize;       // 0x805FB94C
}
extern "C++" void* gEventMgr;       // 0x805FB6C8

typedef void (*FreeFieldFn)(u8*, u8*);

// Frees a fixed-depth binary tree (children at +0 and +4): at depth 0 the two fields are
// handed to freeField, then the node is deleted (children first).
static void FreeTreeNode(u8* cont, u8* n, s32 depth, FreeFieldFn freeField) {
    if (depth == 0) {
        u8* a = *(u8**)(n + 0x0);
        if (a != NULL) freeField(cont, a);
        u8* b = *(u8**)(n + 0x4);
        if (b != NULL) freeField(cont, b);
    } else {
        u8* l = *(u8**)(n + 0x0);
        if (l != NULL) FreeTreeNode(cont, l, depth - 1, freeField);
        u8* r = *(u8**)(n + 0x4);
        if (r != NULL) FreeTreeNode(cont, r, depth - 1, freeField);
    }
    operator_delete(n);
}

// Same shape, but every node (leaf too) also runs fn_800A2E70 on its sub-tree at +0xC first.
static void FreeTreeNodeSub(u8* cont, u8* n, s32 depth) {
    if (depth == 0) {
        u8* a = *(u8**)(n + 0x0);
        if (a != NULL) fn_800A23FC(cont, a);
        u8* b = *(u8**)(n + 0x4);
        if (b != NULL) fn_800A23FC(cont, b);
    } else {
        u8* l = *(u8**)(n + 0x0);
        if (l != NULL) FreeTreeNodeSub(cont, l, depth - 1);
        u8* r = *(u8**)(n + 0x4);
        if (r != NULL) FreeTreeNodeSub(cont, r, depth - 1);
    }
    fn_800A2E70(cont, n + 0xc);
    operator_delete(n);
}

// Table index for an aim-ray state: 2/3/4 -> 0/1/2, 0 -> 3, other -> 0; 4 when the flag is set.
static s32 AimTableIndex(s32 flag, s32 state) {
    if (flag != 0) return 4;
    switch (state) {
    case 2: return 0;
    case 3: return 1;
    case 4: return 2;
    case 0: return 3;
    default: return 0;
    }
}

extern "C" {

// 0x800893C4
// Picks an aim-ray spark from the object's slot list (0x96 or 0x19 entries), sets its target,
// speed and direction, and sends it the spawn event. Returns 0 only when no ray is ready.
s32 fn_800893C4(u8* arg0, s32 arg1, s32 arg2) {
    s32 slot = fn_8008A58C(arg0);
    if (slot == -1) return 0;
    s32 sp10 = 0;
    s32 spC = 0;
    s32 sp8 = 1;
    f32 dir[3] = { 0.0f, 0.0f, 0.0f };   // 0x20..0x28
    if (arg1 == 0) fn_8008A768(arg0, &sp10, &spC, &sp8);
    if (arg1 == 1) {
        if (fn_8008B264(arg0, arg2) == 0) return 1;
        slot = 4;
        // Sweeping angle in degrees, bounces between 60 and 120.
        if (lbl_805F8A04 == 1) *(f32*)(arg0 + 0x3bc) = *(f32*)(arg0 + 0x3bc) + 20.0f;
        else                   *(f32*)(arg0 + 0x3bc) = *(f32*)(arg0 + 0x3bc) - 20.0f;
        f32 ang = *(f32*)(arg0 + 0x3bc);
        if (ang < 60.0f) {
            *(f32*)(arg0 + 0x3bc) = 60.0f;
            lbl_805F8A04 = 1;
        } else if (ang > 120.0f) {
            *(f32*)(arg0 + 0x3bc) = 120.0f;
            lbl_805F8A04 = -1;
        }
        f32 c = 4.0f * cosf(1.5707964f * (0.011111111f * *(f32*)(arg0 + 0x3bc)));
        f32 s = 4.0f * sinf(1.5707964f * (0.011111111f * *(f32*)(arg0 + 0x3bc)));
        dir[0] = c;
        dir[2] = s;
        // Poll until a matching event comes up.
        s32 done = 0;
        do {
            fn_8008A768(arg0, &sp10, &spC, &sp8);
            if (sp10 == 0) {
                switch (spC) {
                case 2: case 3: case 4:
                    if (spC != arg2) done = 1;
                    break;
                default: break;
                }
            } else {
                switch (sp8) {
                case 1: case 2: done = 1; break;
                default: break;
                }
            }
        } while (done == 0);
        sp8 = 3;
    }

    f32 speed = 0.0f;
    f32 spread = 0.0f;
    if (arg1 == 0) {
        s32 i = AimTableIndex(sp10, spC);
        speed = lbl_803467D0[i];
        spread = lbl_8034680C[i];
    } else if (arg1 == 1) {
        speed = 0.5f + (0.5f * (2.3283064e-10f * (f32)RandomU32()));
    }
    if (arg1 == 0) {
        f32 t = 1.0f - (*(f32*)(arg0 + 0x47c) / 70.0f);
        speed += t * (lbl_803467E4[AimTableIndex(sp10, spC)] - lbl_803467D0[AimTableIndex(sp10, spC)]);
        spread += t * (lbl_80346820[AimTableIndex(sp10, spC)] - lbl_8034680C[AimTableIndex(sp10, spC)]);
    }

    s32 base = (sp10 == 0) ? 0xf8 : 0x94;
    s32 count = (sp10 == 0) ? 0x96 : 0x19;
    for (s32 i = 0; i < count; i++) {
        u8* e = *(u8**)(arg0 + base + i * 4);
        if (e != NULL && *(s32*)(e + 0x264) == 1 && spC == *(s32*)(e + 0x268)) {
            fn_8008D25C(e, *(u8**)(arg0 + slot * 4 + 0x358) + 8);
            fn_8008D7B0(e, sp8);
            fn_8008D7B8(e, speed);
            fn_8008D7C0(e, spread);
            fn_8008D6A0(e, dir);
            fn_8008CE38(e);
            s32 r = RaySceneTest(e);
            if (r < 0) r = 0;
            else if (r > 3) r = 3;
            fn_8008D294(e, r);
            EventSend(gEventMgr, e, NULL, 0xA3B1D107u, 0, 0, 1, 0);
            return 1;
        }
    }
    return 1;
}

// 0x8009EC30
// Camera-follow tracker: samples the followed object's position, stores the heading in a ring
// buffer, and (per flag bytes 0x5C/0x5D/0x5E) derives eye position, look direction and the
// smoothed orientation quaternion. Returns 0 when inactive.
s32 fn_8009EC30(u8* arg0, f32* arg1, f32* arg2, f32 dt) {
    typedef void (*VFnA)(u8*);
    typedef void (*VFnB)(f32*, u8*);

    if (*(u8*)(arg0 + 0x54) == 0 || *(u8**)(arg0 + 0x58) == NULL) return 0;
    if (fn_800DECD8(arg0, dt) == 0) return 0;

    u8* r31 = *(u8**)(*(u8**)(arg0 + 0x58) + 0x198);
    u8* tgt = *(u8**)(r31 + 0x4);
    f32 pos[3];                      // 0x134..0x13c
    pos[0] = *(f32*)(tgt + 0x0);
    pos[1] = *(f32*)(tgt + 0x4);
    pos[2] = *(f32*)(tgt + 0x8);
    f32 dx = pos[0] - *(f32*)(arg0 + 0x60);
    f32 dy = pos[1] - *(f32*)(arg0 + 0x64);
    f32 dz = pos[2] - *(f32*)(arg0 + 0x68);
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 heading[3];                  // 0x128..0x130
    if (len2 == 0.0f) {
        heading[0] = heading[1] = heading[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        heading[0] = dx * inv;
        heading[1] = dy * inv;
        heading[2] = dz * inv;
    }

    // Push the heading into the ring buffer (newest at the lowest index).
    s32 cap = *(s32*)(arg0 + 0xa4);
    *(s32*)(arg0 + 0xac) = 1;
    s32 idx = ((cap + *(s32*)(arg0 + 0xa8)) - 1) % cap;
    *(s32*)(arg0 + 0xa8) = idx;
    memcpy(*(u8**)(arg0 + 0x9c) + idx * 0xc, heading, 0xc);
    (*(s32**)(arg0 + 0xa0))[*(s32*)(arg0 + 0xa8)] = *(s32*)(arg0 + 0xb0);
    s32 serial = *(s32*)(arg0 + 0xb0) + 1;
    *(s32*)(arg0 + 0xb0) = serial;
    if (serial == 0) *(s32*)(arg0 + 0xb0) = serial + 1;

    arg2[0] = 0.0f;
    arg2[1] = 0.0f;
    arg2[2] = 0.0f;
    f32 q[4];                        // 0x118..0x124
    fn_8009C1E8(q, *(u8**)(arg0 + 0x58) + 0x9c);
    f32 e[3];                        // 0x108..0x110
    fn_801455E0(e, (u8*)q);
    if (*(u8*)(arg0 + 0x5c) != 0) {
        arg1[0] = pos[0];
        arg1[1] = pos[1];
        arg1[2] = pos[2];
        if (*(u8*)(arg0 + 0x5d) != 0) {
            f32 f13 = *(f32*)(r31 + 0x3d8);
            f32 f12 = *(f32*)(r31 + 0x3dc);
            f32 f11 = *(f32*)(r31 + 0x3e0);
            f32 f3  = *(f32*)(r31 + 0x3e4);
            u8* t5 = *(u8**)(r31 + 0x4);
            f32 f1 = -f13;
            f32 f2 = -f11;
            f32 f0 = -f12;
            f32 a = *(f32*)(t5 + 0xc);
            f32 b = *(f32*)(t5 + 0x10);
            f32 c = *(f32*)(t5 + 0x14);
            f32 d = *(f32*)(t5 + 0x18);
            f32 q2[4];               // 0xf8..0x104
            q2[0] = ((b * f2) + ((d * f1) + (a * f3))) - (c * f0);
            q2[1] = (c * f1) + ((b * f3) + ((d * f0) - (a * f2)));
            q2[2] = (c * f3) + (((d * f2) + (a * f0)) - (b * f1));
            q2[3] = (((d * f3) - (a * f1)) - (b * f0)) - (c * f2);
            f32 out[3];              // 0x74..0x7c
            fn_801455E0(out, (u8*)q2);
            e[0] = out[0];
            e[1] = out[1];
            e[2] = out[2];
        }
        if (*(u8*)(arg0 + 0x5e) != 0) {
            // Average the buffered headings.
            for (s32 i = 0; i < *(s32*)(arg0 + 0xa4); i++) {
                s32 n = *(s32*)(arg0 + 0xa4);
                u8* h = *(u8**)(arg0 + 0x9c) + ((*(s32*)(arg0 + 0xa8) + i) % n) * 0xc;
                arg2[0] = arg2[0] + *(f32*)(h + 0x0);
                arg2[1] = arg2[1] + *(f32*)(h + 0x4);
                arg2[2] = arg2[2] + *(f32*)(h + 0x8);
            }
            f32 ax = arg2[0], ay = arg2[1], az = arg2[2];
            f32 l2 = (az * az) + ((ax * ax) + (ay * ay));
            f32 nrm[3];
            if (l2 == 0.0f) {
                nrm[0] = nrm[1] = nrm[2] = 0.0f;
            } else {
                f32 inv2 = 1.0f / sqrtf(l2);
                nrm[0] = ax * inv2;
                nrm[1] = ay * inv2;
                nrm[2] = az * inv2;
            }
            arg2[0] = nrm[0];
            arg2[1] = nrm[1];
            arg2[2] = nrm[2];
            f32 q3[4];               // 0xe8..0xf4
            fn_801458B0(q3, arg2, e);

            // w of (q * conj(stored 0x88)); acos of it (result unused).
            f32 g12 = -*(f32*)(arg0 + 0x88);
            f32 g9  = -*(f32*)(arg0 + 0x8c);
            f32 g30 = -*(f32*)(arg0 + 0x90);
            f32 g0  = *(f32*)(arg0 + 0x94);
            f32 w1 = (((q[3] * g0) - (q[0] * g12)) - (q[1] * g9)) - (q[2] * g30);
            f32 cl = -1.0f;
            if (!(w1 < -1.0f)) {
                cl = 1.0f;
                if (!(w1 > 1.0f)) cl = w1;
            }
            fn_8033F1F4((f64)cl);

            // Angle between the smoothed orientation (0x78) and the new one.
            f32 h12 = -*(f32*)(arg0 + 0x78);
            f32 h9  = -*(f32*)(arg0 + 0x7c);
            f32 h31 = -*(f32*)(arg0 + 0x80);
            f32 h0  = *(f32*)(arg0 + 0x84);
            f32 w2 = (((q3[3] * h0) - (q3[0] * h12)) - (q3[1] * h9)) - (q3[2] * h31);
            cl = -1.0f;
            if (!(w2 < -1.0f)) {
                cl = 1.0f;
                if (!(w2 > 1.0f)) cl = w2;
            }
            f32 angle = (f32)fabs((2.0f * (0.63661975f * (f32)fn_8033F1F4((f64)cl))) - 2.0f);
            if (angle > 0.055555556f) {
                // Limit the turn rate.
                f32 lim[4];          // 0xb8..0xc4
                fn_80145F9C(lim, (f32*)(arg0 + 0x78), q3, 0.055555556f / angle);
                q3[0] = lim[0];
                q3[1] = lim[1];
                q3[2] = lim[2];
                q3[3] = lim[3];
                Vec3 up = QuatGetAxisY((const Quat*)q3);
                arg2[0] = up.x;
                arg2[1] = up.y;
                arg2[2] = up.z;
            }
            *(f32*)(arg0 + 0x78) = q3[0];
            *(f32*)(arg0 + 0x7c) = q3[1];
            *(f32*)(arg0 + 0x80) = q3[2];
            *(f32*)(arg0 + 0x84) = q3[3];
        }
    } else {
        u8* sub = *(u8**)(arg0 + 0x58) + 0x9c;
        (*(VFnA*)(*(u8**)(sub + 0xd4) + 0x20))(sub);
        f32 p[3];                    // 0x2c..0x34
        (*(VFnB*)(*(u8**)(sub + 0xd4) + 0x24))(p, sub);
        arg1[0] = p[0];
        arg1[1] = p[1];
        arg1[2] = p[2];
        f32 o0 = arg2[0] * 20.0f;
        f32 o1 = arg2[1] * 20.0f;
        f32 o2 = arg2[2] * 20.0f;
        arg1[0] = p[0] - o0;
        arg1[1] = p[1] - o1;
        arg1[2] = p[2] - o2;
    }

    // If the direction is (near) zero, fall back to the sub-object's own direction (vtable +0x28).
    if (arg2[0] <= 0.0001f && arg2[0] >= -0.0001f &&
        arg2[1] <= 0.0001f && arg2[1] >= -0.0001f &&
        arg2[2] <= 0.0001f && arg2[2] >= -0.0001f) {
        u8* sub = *(u8**)(arg0 + 0x58) + 0x9c;
        f32 v[3];                    // 0x8..0x10
        (*(VFnB*)(*(u8**)(sub + 0xd4) + 0x28))(v, sub);
        f32 l2 = (v[2] * v[2]) + ((v[0] * v[0]) + (v[1] * v[1]));
        f32 nv[3];
        if (l2 == 0.0f) {
            nv[0] = nv[1] = nv[2] = 0.0f;
        } else {
            f32 inv3 = 1.0f / sqrtf(l2);
            nv[0] = v[0] * inv3;
            nv[1] = v[1] * inv3;
            nv[2] = v[2] * inv3;
        }
        arg2[0] = nv[0];
        arg2[1] = nv[1];
        arg2[2] = nv[2];
    }

    u8* cam = *(u8**)(lbl_803C4298 + 4);
    *(u8*)(cam + 0xfc) = 1;
    cam = *(u8**)(lbl_803C4298 + 4);
    *(f32*)(cam + 0xf0) = e[0];
    *(f32*)(cam + 0xf4) = e[1];
    *(f32*)(cam + 0xf8) = e[2];
    *(f32*)(arg0 + 0x60) = pos[0];
    *(f32*)(arg0 + 0x64) = pos[1];
    *(f32*)(arg0 + 0x68) = pos[2];
    *(f32*)(arg0 + 0x6c) = arg2[0];
    *(f32*)(arg0 + 0x70) = arg2[1];
    *(f32*)(arg0 + 0x74) = arg2[2];
    *(f32*)(arg0 + 0x88) = q[0];
    *(f32*)(arg0 + 0x8c) = q[1];
    *(f32*)(arg0 + 0x90) = q[2];
    *(f32*)(arg0 + 0x94) = q[3];
    return 1;
}

// 0x8009FE24
// Deleting destructor of the zone object: restores the vtable, releases the two audio
// handles, drops the zone-slot flags, frees the three owned tree containers (+0xCE8, +0xCD8
// with its inner tree at +0x10), destroys the members at +0xC74 and +0x8C, and frees the
// object when del > 0.
u8* fn_8009FE24(u8* arg0, s32 arg1) {
    typedef void (*VFn)(u8*);
    if (arg0 != NULL) {
        *(u8**)(arg0 + 0x4) = lbl_8035CF88;
        lbl_805FB158 = NULL;
        fn_8020A398(*(s32*)(arg0 + 0xcf8));
        fn_8020A398(*(s32*)(arg0 + 0xcfc));
        if (*(u8**)(arg0 + 0xc8c) != NULL) {
            fn_800A6630(*(u8**)(arg0 + 0xc8c));
            *(u8**)(arg0 + 0xc8c) = NULL;
        }
        (*(VFn*)(*(u8**)(arg0 + 0x160) + 0x10))(arg0 + 0x8c);

        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1a4) + gZoneResourceSize;
        for (s32 i = 0; i < *(s32*)(zone + 0x98); i++) {
            u8* obj = (*(u8***)(zone + 0x12c))[i];
            u8* p = *(u8**)(obj + 0x4);
            *(u32*)(p + 0x20) = *(u32*)(p + 0x20) | 1;
        }

        // Tree container at +0xCE8 (root at +0xCEC): 4 levels, leaf fields freed by fn_8006F794.
        u8* root1 = *(u8**)(arg0 + 0xcec);
        if (root1 != NULL) FreeTreeNode(arg0 + 0xce8, root1, 3, fn_8006F794);

        // Container at +0xCD8 (root at +0xCDC).
        u8* root2 = *(u8**)(arg0 + 0xcdc);
        if (root2 != NULL) {
            u8* l = *(u8**)(root2 + 0x0);
            if (l != NULL) FreeTreeNodeSub(arg0 + 0xcd8, l, 2);
            u8* r = *(u8**)(root2 + 0x4);
            if (r != NULL) FreeTreeNodeSub(arg0 + 0xcd8, r, 2);
            // Inner container at root2+0x10 (root at root2+0x14), leaf fields freed by fn_800A3188.
            u8* root3 = *(u8**)(root2 + 0x14);
            if (root3 != NULL) FreeTreeNode(root2 + 0x10, root3, 3, fn_800A3188);
            operator_delete(root2);
        }

        fn_800C210C(arg0 + 0xc74, 0);
        fn_800C42FC(arg0 + 0x8c, 0);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// Clamped scene level (0..3) from the RaySceneTest getter.
static s32 SceneLevel() {
    s32 v = RaySceneTest(NULL);
    if (v < 0) v = 0;
    else if (v > 3) v = 3;
    return v;
}

// Uniform random in [0, 1).
static f32 RandUnit() {
    return 2.3283064e-10f * (f32)RandomU32();
}

// 0x800B387C
// Picks a new slot for the animation record `arg1` (records are 0x14 bytes apart from arg0): by
// the current record kind (0/1/2) it re-rolls two random timer values at +0xB0/+0xB4 from
// difficulty tables, then chooses a free slot (flag +0x160 per slot) at random, avoiding the
// slot returned by fn_800B4070, and updates the slot index at +0xAC.
void fn_800B387C(u8* arg0, s32 arg1) {
    s32 chosen = 0;                  // r27
    f32 a = lbl_80346208[SceneLevel()];
    f32 sum = a + lbl_80346218[SceneLevel()];
    s32 half = (s32)(f32)floor((f64)(50.0f / sum));    // r29
    s32 slots = half * 2 + 1;                                // r26
    u8* rec = arg0 + arg1 * 0x14;                            // r30
    u8* obj = *(u8**)(rec + 0xa8);
    if (obj == NULL) return;

    s32 kind = fn_800B964C(obj);
    if (kind >= 0 && kind <= 2) {
        const f32* cmp;
        const f32* lo0; const f32* hi0;
        const f32* lo1; const f32* hi1;
        if (kind == 0) {
            cmp = lbl_803460E8; lo0 = lbl_80346148; hi0 = lbl_80346158; lo1 = lbl_803461A8; hi1 = lbl_803461B8;
        } else if (kind == 1) {
            cmp = lbl_803460F8; lo0 = lbl_80346168; hi0 = lbl_80346178; lo1 = lbl_803461C8; hi1 = lbl_803461D8;
        } else {
            cmp = lbl_80346108; lo0 = lbl_80346188; hi0 = lbl_80346198; lo1 = lbl_803461E8; hi1 = lbl_803461F8;
        }
        s32 i = SceneLevel();
        f32 chance = 0.0f + 0.99f * RandUnit();
        fn_800B999C(*(u8**)(rec + 0xa8), chance < cmp[i], 0);
        i = SceneLevel();
        s32 j = SceneLevel();
        f32 hi = hi0[j];
        f32 lo = lo0[i];
        *(f32*)(arg0 + arg1 * 0x14 + 0xb0) = lo + ((hi - lo) * RandUnit());
        i = SceneLevel();
        j = SceneLevel();
        hi = hi1[j];
        lo = lo1[i];
        *(f32*)(arg0 + arg1 * 0x14 + 0xb4) = lo + ((hi - lo) * RandUnit());
    }

    if (fn_800B9A78(*(u8**)(rec + 0xa8)) != 0 || fn_800B964C(*(u8**)(rec + 0xa8)) == 1) {
        s32 sp8 = 0;
        f32 t = fn_800B68B8(*(u8**)(arg0 + 0x94));   // f1 is passed straight through to fn_800B4070
        s32 have = fn_800B4070(arg0, &sp8, t);
        if (have != 0) sp8 += half;
        s32 freeCount = 0;
        for (s32 i = 0; i < slots; i++) {
            if (*(s32*)(arg0 + i * 4 + 0x160) == 0) {
                if (fn_800B964C(*(u8**)(rec + 0xa8)) == 0) {
                    if (have == 0 || sp8 != i) freeCount++;
                } else {
                    freeCount++;
                }
            }
        }
        s32 done = 0;
        if (freeCount <= 0) {
            done = 1;
            chosen = *(s32*)(rec + 0xac);
        }
        while (done == 0) {
            chosen = (s32)(RandomU32() % (u32)slots);
            if (*(s32*)(arg0 + chosen * 4 + 0x160) == 0) {
                done = 1;
                if (fn_800B964C(*(u8**)(rec + 0xa8)) == 0 && have != 0 && chosen == sp8) done = 0;
            }
        }
        *(s32*)(arg0 + *(s32*)(rec + 0xac) * 4 + 0x160) = 0;
        *(s32*)(rec + 0xac) = chosen;
        *(s32*)(arg0 + chosen * 4 + 0x160) = 1;
        s32 rel = *(s32*)(rec + 0xac) - half;
        f32 lv = lbl_80346208[SceneLevel()];
        lv += lbl_80346218[SceneLevel()];
        fn_800B990C(*(u8**)(rec + 0xa8), 0.011111111f * ((f32)rel * lv));
    }
}

// Finds the first of the 0x32 slot pointers at +0x170 whose state (+0x290) is not 2..5.
static u8* FindIdleSlot(u8* arg0) {
    for (s32 i = 0; i < 0x32; i++) {
        u8* e = *(u8**)(arg0 + 0x170 + i * 4);
        if ((u32)(*(s32*)(e + 0x290) - 2) > 3u) return e;
    }
    return NULL;
}

// 0x800BAA84
// Per-frame update of the aiming/targeting object: counts the timers (+0x90, +0x3AC..+0x3B8)
// down, signals win / out-of-time when the main timer runs out, projects the aim point to the
// screen, reads the stick to nudge the target, fires a shot on the buttons, places the crosshair
// widget at +0x478, and re-arms the three spawn timers when they expire.
void fn_800BAA84(u8* arg0, f32 dt) {
    typedef f32  (*VFnTimer)(u8*);
    typedef void (*VFnNotify)(u8*, s32);

    f32 t90 = *(f32*)(arg0 + 0x90) - dt;
    *(f32*)(arg0 + 0x90) = t90;
    *(f32*)(arg0 + 0x3b0) = *(f32*)(arg0 + 0x3b0) - dt;
    *(f32*)(arg0 + 0x3ac) = *(f32*)(arg0 + 0x3ac) - dt;
    *(f32*)(arg0 + 0x3b4) = *(f32*)(arg0 + 0x3b4) - dt;
    *(f32*)(arg0 + 0x3b8) = *(f32*)(arg0 + 0x3b8) - dt;
    if (t90 <= 0.0f) {
        if ((*(VFnTimer*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0) > 0.0f) GameFlowSignal((s32)0xC608C842, 0, 0);   // won
        else                                                          GameFlowSignal(0x117741C3, 0, 0);          // out of time
    }
    if ((*(VFnTimer*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0) <= 0.0f) {
        if ((*(VFnTimer*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0) > 0.0f) GameFlowSignal((s32)0xC608C842, 0, 0);
        else                                                          GameFlowSignal(0x117741C3, 0, 0);
    }
    fn_800BF878(*(u8**)(arg0 + 0x238), dt * lbl_80346D50[RaySceneTest(NULL)]);

    f32 ray[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };   // 0x184..0x198: ray points
    u8* cam = fn_80073454();
    f32 halfW = 0.5f * (f32)gScreenWidth;
    f32 halfH = 0.5f * (f32)gScreenHeight;
    u8* target = *(u8**)(*(u8**)(arg0 + 0x238) + 0x4);
    f32 sx = *(f32*)(arg0 + 0x46c) * *(f32*)(arg0 + 0x44c);
    f32 sy = *(f32*)(arg0 + 0x470) * *(f32*)(arg0 + 0x450);
    f32 pt[3];                       // 0x154..0x15c
    pt[0] = *(f32*)(target + 0x0);
    pt[1] = *(f32*)(target + 0x4);
    pt[2] = *(f32*)(target + 0x8);
    f32 hit[3];                      // 0x160..0x168
    fn_80073F58(hit, cam, pt, &ray[3], &ray[0], halfW + (halfW * sx), halfH + (halfH * sy));
    *(f32*)(arg0 + 0x47c) = hit[0];
    *(f32*)(arg0 + 0x480) = hit[1];
    *(f32*)(arg0 + 0x484) = hit[2];

    f32 stickX = 0.0f;               // 0x10
    f32 stickY = 0.0f;               // 0xc
    StickRead(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0xe, &stickX, &stickY);
    if (fabsf(stickY) >= 0.2f || fabsf(stickX) >= 0.2f) {
        u8* tp = *(u8**)(*(u8**)(arg0 + 0x238) + 0x4);
        f32 np[3];                   // 0x178..0x180
        np[0] = *(f32*)(tp + 0x0) + stickX;
        np[1] = *(f32*)(tp + 0x4) + 0.0f;
        np[2] = *(f32*)(tp + 0x8) + stickY;
        fn_800C0660(arg0 + 0x240, np);
    }

    s32 pad = *(s32*)(lbl_803C4298 + 0x38);
    if (fn_80147554(pad * 16 + 0x1039) != 0 || fn_80147554(pad * 16 + 0x108a) != 0) {
        if (*(s32*)(arg0 + 0x488) > 0) {
            u8* idle = FindIdleSlot(arg0);
            if (idle != NULL) {
                f32 b[3] = { ray[0], ray[1], ray[2] };   // 0xd4 (copy of 0x130..)
                f32 a[3] = { ray[3], ray[4], ray[5] };   // 0xc8
                fn_800BE5EC(idle, a, b);
                s32 left = *(s32*)(arg0 + 0x488) - 1;
                *(s32*)(arg0 + 0x488) = left;
                fn_800715B8(arg0, left);
                fn_800716D0(arg0, *(s32*)(arg0 + 0x488) > 0);
                u8* w = *(u8**)(arg0 + 0x478);
                if (w != NULL) {
                    // Rewrite the widget's 4-float colour (+0x80) with alpha 1 while shots remain.
                    f32 c3 = (*(s32*)(arg0 + 0x488) > 0) ? 1.0f : 0.0f;
                    *(f32*)(w + 0x8c) = c3;
                    w = *(u8**)(arg0 + 0x478);
                    (*(VFnNotify*)(*(u8**)(w + 0x138) + 0x2c))(w, 0x38);
                }
            }
        }
    }

    if (*(s32*)(arg0 + 0x488) > 0) {
        u8* w = *(u8**)(arg0 + 0x478);
        f32 hw = *(f32*)(w + 0x74) / 2.0f;
        f32 hh = *(f32*)(w + 0x78) / 2.0f;
        f32 px = halfW + (halfW * sx);
        f32 py = halfH + (halfH * sy);
        *(f32*)(w + 0x68) = px - hw;
        *(f32*)(w + 0x6c) = py - hh;
        *(f32*)(w + 0x70) = 1.0f;
        w = *(u8**)(arg0 + 0x478);
        (*(VFnNotify*)(*(u8**)(w + 0x138) + 0x2c))(w, 0x38);
    }

    f32 t = *(f32*)(arg0 + 0x90);
    fn_800712BC(arg0, (s32)lrintf(t), t < 10.0f);
    fn_80070FAC(arg0, (*(VFnTimer*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0) / 100.0f);
    fn_800BB658(arg0, dt);

    if (*(f32*)(arg0 + 0x3b0) <= 0.0f) {
        // Best (largest +0x268) of the 18 candidate objects at +0x94 whose state byte (+0x298) is 1.
        u8* best = NULL;
        f32 bestVal = 0.0f;
        for (s32 i = 0; i < 18; i++) {
            u8* e = *(u8**)(arg0 + 0x94 + i * 4);
            if (*(u8*)(e + 0x298) == 1) {
                f32 v = *(f32*)(e + 0x268);
                if (v > bestVal) {
                    bestVal = v;
                    best = e;
                }
            }
        }
        if (best != NULL) {
            u8* idle = FindIdleSlot(arg0);
            fn_800BD3D4(best, idle);
            *(f32*)(arg0 + 0x3b0) = 5.0f * lbl_80346ED0[RaySceneTest(NULL)];
        }
    }
    if (*(f32*)(arg0 + 0x3ac) <= 0.0f) fn_800BC050(arg0);
    if (*(f32*)(arg0 + 0x3b4) <= 0.0f) fn_800BC388(arg0);
    if (*(f32*)(arg0 + 0x3b8) <= 0.0f) {
        s32 lvl = RaySceneTest(NULL);
        for (s32 k = 0; k < 2; k++) {
            u8* obj = *(u8**)(arg0 + 0x168 + k * 4);
            if (*(u8*)(obj + 0x264) == 0) {
                f32 pos[3] = { 0.0f, 0.0f, 0.0f };   // 0x80..0x88
                f32 v = lbl_80346F10[lvl];
                s32 ok = 0;
                for (s32 tries = 0; tries < 10; tries++) {
                    f32 cand[3];
                    fn_800BE148(cand, *(u8**)(*(u8**)(arg0 + 0x238) + 0x4));
                    pos[0] = cand[0];
                    pos[1] = cand[1];
                    pos[2] = cand[2];
                    if (fn_800BC704(arg0, pos) != 0) {
                        fn_800BD984(obj, pos, v);
                        ok = 1;
                        break;
                    }
                }
                if (ok != 0) *(f32*)(arg0 + 0x3b8) = lbl_80346F00[lvl];
                break;
            }
        }
    }
}

}  // extern "C"
