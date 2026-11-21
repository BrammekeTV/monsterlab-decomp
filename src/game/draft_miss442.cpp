// Character/physics helpers in 0x8005xxxx-0x8006xxxx: collision-shape corner transform, character
// velocity integration, ground (platform) tracking, the per-frame movement step and the
// character teardown.
#include <math.h>
#include <stddef.h>
#include <string.h>

#include "types.h"
#include "engine/math.h"

typedef f32 Scalar;
struct ScopedLock { u8 pad[4]; };   // as in props.cpp

extern "C" {
void   CharacterResetGroundState(u8* c, s32 a, s32 b);           // 0x8005BAD4
void*  EventPost(s32 id, void* sender);                          // 0x801F3918
s32    PlaySoundAt(void* sound, int a, const void* pos, int b);  // 0x80209ABC
void   QuatFromMtx(Quat* out, const f32 m[12]);                  // 0x801456E4
BOOL   RayQueryCast(u8* q, void* world, u32 filter, u8* hit, u8* hit2);   // 0x801B203C
void   RayQueryInit(u8* q);                                      // 0x801B11DC
void   RayHitInit(u8* hit);                                      // 0x80055CD8
void   RayQuerySetStart(u8* q, const Vec3* p);                   // 0x80055D64
void   RayQuerySetDir(u8* q, const Vec3* d);                     // 0x80055D80
void   ScalarSet(Scalar* out, f32 v);
void   ScalarZero(Scalar* out);                                  // 0x80055E2C
void   ScopedLockCtor(ScopedLock* l);                            // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                 // 0x801F53B0
s32    TriggerDoAction(void* action, void* a1, void* a2);        // 0x801E0054
void   Vec3Assign(Vec3* out, const Vec3* v);                     // 0x80053354
void   Vec3Copy(Vec3* out, const Vec3* v);                       // 0x80052640
f32    Vec3LengthSqOf(const Vec3* v);                            // 0x80054A90
void   Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s); // 0x80053224
void   Vec3NormalizeTo(Vec3* out, const Vec3* v);                // 0x80055D9C
void   Vec3SetUnitZ(Vec3* v);                                    // 0x800549A8
void   Vec3SetZ(Vec3* v, f32 z);
void   Vec3SetZero(Vec3* out);                                   // 0x800531A8
void   Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);         // 0x80053370
void   fn_80007CD4(u8* arg0);                                      // 0x80007CD4
void   fn_8005263C(f32* q);                                      // 0x8005263C (quat identity)
u8*    fn_800526DC(u8* c);                                       // 0x800526DC: returns the position pointer (+4)
s32    fn_800544D8(u8* obj, s32 bit);                            // 0x800544D8: bit of the flag word at +4->+0x1C
s32    fn_80053ACC(u8* obj, s32 mask);                           // 0x80053ACC: mask on the flag word at +4->+0x20
void   fn_8005379C(u8* obj, s32 slot, s32 arg);                  // 0x8005379C
void   fn_80054650(u8* out, u8* q, u8* v);                       // 0x80054650
s32    fn_80054988(u8* word, s32 mask);                          // 0x80054988: *word & mask
void   fn_8005B0B8(u8* c, f32 dt);                               // 0x8005B0B8
u8*    fn_8005E128(u8* a3);                                      // 0x8005E128
void   fn_800636A0(void);                                        // 0x800636A0
s32    fn_80064568(void* fxMgr, u32 effect, const f32* v, u8** out);   // 0x80064568
void   fn_800651E4(const f32* src, f32* dst);                    // 0x800651E4
s32    fn_800651C8(u8* obj, s32 mask);                           // 0x800651C8
s32    fn_800653E8(void* sound);                                 // 0x800653E8
u8**   fn_800655D0(u8* obj);                                     // 0x800655D0: address of the field at +0x9C
s32    fn_80066800(u8* c);                                       // 0x80066800
s32    fn_800676AC(f32 a, f32 b);                                // 0x800676AC
void   fn_800676D4(Vec3* out, const Scalar* s);                  // 0x800676D4
void   fn_800676E0(f32* out, const f32* in, f32* outLength);     // 0x800676E0
s32    fn_800677A0(const f32* v);                                // 0x800677A0
f32    fn_800677D0(f32 lo, f32 hi);                              // 0x800677D0
u8*    fn_80067844(u8* e);                                       // 0x80067844 (leaf: pointer at +0x5C)
void   fn_80067BB0(u8* obj, u8* item, s32 a2, void* a3, f32 t);  // 0x80067BB0
void   fn_8006680C(u8* list, const f32* vec);                    // 0x8006680C
void   fn_80066960(u8* list);                                    // 0x80066960
s32    fn_80068450(u8* c);                                       // 0x80068450
s32    fn_80138C40(void* world, f32* a, f32* b, f32* hit, void* p, s32 c, s32 d);   // 0x80138C40
s32    fn_80139140(void* world, f32* p0, f32* p1, u8* hit, s32 a, s32 b);           // 0x80139140
void   fn_801458B0(f32* quat, f32* a, f32* b);                   // 0x801458B0
void   fn_80145F9C(f32* out, f32* a, f32* b, f32 t);             // 0x80145F9C
void*  fn_801B7198(void* anim, s32 a);                           // 0x801B7198
void   fn_801C849C(u8* mgr, u8* item);                           // 0x801C849C
void   fn_801C85E0(u8* mgr, u8* obj);                            // 0x801C85E0
void*  fn_801ACCA0(u8* q, s32 arg1);                             // 0x801ACCA0
void   fn_8020A398(s32 handle);                                  // 0x8020A398

extern u8  gWorldState[];           // 0x803E6D00
extern u8  jumptable_803597C0[];    // 0x803597C0 (+0x40: four Vec3 corner points)
extern f32 lbl_803C5498[12];        // 0x803C5498 (four Vec3 points, transformed in place)
extern f32 lbl_803C54C8[3];         // 0x803C54C8
extern u8  lbl_8047C760[];          // 0x8047C760 (effect manager)
extern f32 lbl_805FB108;            // 0x805FB108
extern f32 lbl_805FB10C;            // 0x805FB10C
extern u8  lbl_805FB110;            // 0x805FB110
extern s32 lbl_805FB6B8;            // 0x805FB6B8
}

static const u32 kHashBAse = 0xF62113D5u;     // "BAse"

// Virtual slots used below (self pointer first).
typedef s32   (*FnVirtQuery)(u8* self);                         // slot 0x4C
typedef void  (*FnVirtDestroy)(u8* self, s32 flag);             // slot 0x18
typedef Vec3* (*FnVirtGetPoint)(u8* self, u8* pos, u8* from);   // slot 0x44
typedef void  (*FnVirtApply)(u8* self, u8* other, Vec3* v);     // slot 0x54

// v scaled to unit length; a zero vector stays zero.
static void NormalizeInPlace3(f32* v) {
    f32 l2 = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
    if (l2 == 0.0f) {
        v[0] = v[1] = v[2] = 0.0f;
        return;
    }
    f32 inv = 1.0f / sqrtf(l2);
    v[0] = v[0] * inv;
    v[1] = v[1] * inv;
    v[2] = v[2] * inv;
}

extern "C" {

// 0x80054B48
// Places a collision shape's four reference points (lbl_803C5498) in the world: builds a 3x3
// matrix plus translation from the shape record arg1 (kind 4: axis-aligned box, kind 7: oriented
// box with rotation matrix at +0x3C and half extents at +0x6C, else identity), transforms the
// points by it, stores the shape position (arg0) and the two extra floats.
void fn_80054B48(u8* arg0, u8* arg1, f32 farg0, f32 farg1) {
    const f32* tbl = (const f32*)(jumptable_803597C0 + 0x40);
    for (s32 i = 0; i < 12; ++i) lbl_803C5498[i] = tbl[i];

    f32 m[12];   // 3x3 matrix (row-major, 9 floats) followed by the translation
    switch (*(s32*)(arg1 + 0xC)) {
    case 4: {
        f32 sx = *(f32*)(arg1 + 0x3C) * 2.0f;
        f32 sy = *(f32*)(arg1 + 0x40) * 2.0f;
        f32 sz = *(f32*)(arg1 + 0x44) * 2.0f;
        m[0] = sx;   m[1] = 0.0f; m[2] = 0.0f;
        m[3] = 0.0f; m[4] = sy;   m[5] = 0.0f;
        m[6] = 0.0f; m[7] = 0.0f; m[8] = sz;
        m[9] = *(f32*)(arg1 + 0x14);
        m[10] = *(f32*)(arg1 + 0x18);
        m[11] = *(f32*)(arg1 + 0x1C);
        break;
    }
    case 7: {
        f32 sx = *(f32*)(arg1 + 0x6C) * 2.0f;
        f32 sy = *(f32*)(arg1 + 0x70) * 2.0f;
        f32 sz = *(f32*)(arg1 + 0x74) * 2.0f;
        Quat q;
        QuatFromMtx(&q, (const f32*)(arg1 + 0x3C));
        f32 qx2 = 2.0f * q.x;
        f32 qy2 = 2.0f * q.y;
        f32 qz2 = 2.0f * q.z;
        f32 xx = qx2 * q.x, xw = qx2 * q.w;
        f32 yy = qy2 * q.y, yw = qy2 * q.w, yx = qy2 * q.x;
        f32 zz = qz2 * q.z, zy = qz2 * q.y, zx = qz2 * q.x, zw = qz2 * q.w;
        // rotation matrix rows, each row i then scaled by the extent s_i
        m[0] = (1.0f - (yy + zz)) * sx;
        m[1] = (yx + zw) * sx;
        m[2] = (zx - yw) * sx;
        m[3] = (yx - zw) * sy;
        m[4] = (1.0f - (xx + zz)) * sy;
        m[5] = (zy + xw) * sy;
        m[6] = (zx + yw) * sz;
        m[7] = (zy - xw) * sz;
        m[8] = (1.0f - (xx + yy)) * sz;
        m[9] = *(f32*)(arg1 + 0x14);
        m[10] = *(f32*)(arg1 + 0x18);
        m[11] = *(f32*)(arg1 + 0x1C);
        break;
    }
    default:
        m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
        m[3] = 0.0f; m[4] = 1.0f; m[5] = 0.0f;
        m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;
        m[9] = 0.0f; m[10] = 0.0f; m[11] = 0.0f;
        break;
    }

    for (s32 i = 0; i < 4; ++i) {
        f32* p = lbl_803C5498 + i * 3;
        f32 x = p[0], y = p[1], z = p[2];
        p[0] = m[9] + (m[6] * z + (m[0] * x + m[3] * y));
        p[1] = m[10] + (m[7] * z + (m[1] * x + m[4] * y));
        p[2] = m[11] + (m[8] * z + (m[2] * x + m[5] * y));
    }

    lbl_803C54C8[0] = *(f32*)(arg0 + 0x0);
    lbl_803C54C8[1] = *(f32*)(arg0 + 0x4);
    lbl_803C54C8[2] = *(f32*)(arg0 + 0x8);
    lbl_805FB110 = 1;
    lbl_805FB108 = farg0;
    lbl_805FB10C = farg1;
}

// 0x80056864
// Character velocity/orientation step. Clamps the velocity (max speed 50, fall speed -20), turns
// the character's rotation towards its ground normal while standing, integrates the velocity
// (plus acceleration), pulls it back towards its anchor point, moves the position and flags the
// collision state as moved when the step is not tiny.
void fn_80056864(u8* arg0, f32 farg0) {
    f32* vel = (f32*)(arg0 + 0x844);
    f32 speedSq = vel[2] * vel[2] + (vel[0] * vel[0] + vel[1] * vel[1]);
    if (speedSq > 2500.0f) {
        NormalizeInPlace3(vel);
        vel[0] = vel[0] * 50.0f;
        vel[1] = vel[1] * 50.0f;
        vel[2] = vel[2] * 50.0f;
    }
    if (vel[2] < -20.0f) vel[2] = -20.0f;

    u32 flags = *(u32*)(arg0 + 0x838);
    if (flags & 0x10000000) {
        fn_8005B0B8(arg0, *(f32*)(arg0 + 0x9DC) * farg0);
    } else {
        s32 turn = 0;
        if (flags & 0x08000000) {
            if ((*(FnVirtQuery*)(*(u8**)arg0 + 0x4C))(arg0) != 0 ||
                (*(u32*)(*(u8**)(arg0 + 0x4) + 0x1C) & 0x20)) {
                turn = 1;
            }
        }
        if (turn) {
            // Target up direction: the ground normal, pushed towards at least 0.5 (1.5 when the
            // virtual query at +0x4C is zero, 0.5 otherwise) in z.
            f32 up[3];
            up[0] = *(f32*)(arg0 + 0x7A4);
            up[1] = *(f32*)(arg0 + 0x7A8);
            up[2] = *(f32*)(arg0 + 0x7AC);
            if (up[2] < 0.5f) {
                if ((*(FnVirtQuery*)(*(u8**)arg0 + 0x4C))(arg0) == 0) {
                    up[2] = 1.5f;
                } else {
                    up[2] = 0.5f;
                }
                NormalizeInPlace3(up);
            }
            u8* obj = *(u8**)(arg0 + 0x4);
            Vec3 axis = QuatGetAxisY((const Quat*)(obj + 0xC));
            f32 target[4];
            fn_801458B0(target, (f32*)&axis, up);
            f32 qLenSq = target[3] * target[3] + (target[2] * target[2] + (target[0] * target[0] + target[1] * target[1]));
            f32 qInv = 1.0f / sqrtf(qLenSq);
            target[0] = target[0] * qInv;
            target[1] = target[1] * qInv;
            target[2] = target[2] * qInv;
            target[3] = target[3] * qInv;

            f32 d0 = *(f32*)(obj + 0xC) - target[0];
            f32 d1 = *(f32*)(obj + 0x10) - target[1];
            f32 d2 = *(f32*)(obj + 0x14) - target[2];
            f32 d3 = *(f32*)(obj + 0x18) - target[3];
            if (d3 * d3 + (d2 * d2 + (d0 * d0 + d1 * d1)) > 0.001f) {
                *(u32*)(obj + 0x20) |= 0x8000;
                *(u32*)(obj + 0x20) |= 0x10000000;
                *(u32*)(obj + 0x20) |= 0x4000;
                *(u32*)(obj + 0x20) |= 0x10000000;
            }
            f32 out[4];
            fn_80145F9C(out, (f32*)(obj + 0xC), target, 2.0f * farg0);
            *(f32*)(obj + 0xC) = out[0];
            *(f32*)(obj + 0x10) = out[1];
            *(f32*)(obj + 0x14) = out[2];
            *(f32*)(obj + 0x18) = out[3];
        }
    }

    // Step vector: base (+0x810, or the +0x780 direction scaled by the length of +0x810 when
    // bit 0x04000000 is set) plus (velocity + acceleration) * dt plus 0.5 * a * dt^2.
    f32 step[3];
    if (*(u32*)(arg0 + 0x838) & 0x04000000) {
        f32 g0 = *(f32*)(arg0 + 0x810);
        f32 g1 = *(f32*)(arg0 + 0x814);
        f32 g2 = *(f32*)(arg0 + 0x818);
        f32 gl2 = g2 * g2 + (g0 * g0 + g1 * g1);
        f32 len = 0.0f;
        if (gl2 > 0.0f) len = sqrtf(gl2);
        step[0] = *(f32*)(arg0 + 0x780) * len;
        step[1] = *(f32*)(arg0 + 0x784) * len;
        step[2] = *(f32*)(arg0 + 0x788) * len;
        *(u32*)(arg0 + 0x838) &= 0xFBFFFFFF;
    } else {
        step[0] = *(f32*)(arg0 + 0x810);
        step[1] = *(f32*)(arg0 + 0x814);
        step[2] = *(f32*)(arg0 + 0x818);
    }
    f32 vx = *(f32*)(arg0 + 0x850) + *(f32*)(arg0 + 0x844);
    f32 vy = *(f32*)(arg0 + 0x854) + *(f32*)(arg0 + 0x848);
    f32 vz = *(f32*)(arg0 + 0x858) + *(f32*)(arg0 + 0x84C);
    f32 half = 0.5f * (farg0 * farg0);
    f32 ax = *(f32*)(arg0 + 0x758) * half;
    f32 ay = *(f32*)(arg0 + 0x75C) * half;
    f32 az = *(f32*)(arg0 + 0x760) * half;
    step[0] += (vx * farg0) + ax;
    step[1] += (vy * farg0) + ay;
    step[2] += (vz * farg0) + az;

    if (*(u32*)(arg0 + 0x838) & 0x200) {
        // Pull towards the anchor point at +0x6C4: remove the part of the step that moves away
        // from it, then add a spring term (20 * dt * offset).
        u8* obj = *(u8**)(arg0 + 0x4);
        f32 off[3];
        off[0] = *(f32*)(arg0 + 0x6C4) - *(f32*)(obj + 0x0);
        off[1] = *(f32*)(arg0 + 0x6C8) - *(f32*)(obj + 0x4);
        off[2] = *(f32*)(arg0 + 0x6CC) - *(f32*)(obj + 0x8);
        f32 dir[3] = { off[0], off[1], off[2] };
        f32 ol2 = off[2] * off[2] + (off[0] * off[0] + off[1] * off[1]);
        if (ol2 == 0.0f) {
            dir[0] = dir[1] = dir[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(ol2);
            dir[0] = off[0] * inv;
            dir[1] = off[1] * inv;
            dir[2] = off[2] * inv;
        }
        f32 dot = dir[2] * step[2] + (dir[0] * step[0] + dir[1] * step[1]);
        if (dot < 0.0f) {
            step[0] -= dir[0] * dot;
            step[1] -= dir[1] * dot;
            step[2] -= dir[2] * dot;
        }
        f32 spring = 20.0f * farg0;
        step[0] += off[0] * spring;
        step[1] += off[1] * spring;
        step[2] += off[2] * spring;
        *(u32*)(arg0 + 0x838) &= 0xFFFFFDFF;
    }

    u8* pos = *(u8**)(arg0 + 0x4);
    *(f32*)(pos + 0x0) = *(f32*)(pos + 0x0) + step[0];
    *(f32*)(pos + 0x4) = *(f32*)(pos + 0x4) + step[1];
    *(f32*)(pos + 0x8) = *(f32*)(pos + 0x8) + step[2];
    s32 tiny = 0;
    if (step[0] <= 0.0001f && step[0] >= -0.0001f && step[1] <= 0.0001f && step[1] >= -0.0001f &&
        step[2] <= 0.0001f && step[2] >= -0.0001f) {
        tiny = 1;
    }
    if (tiny == 0) {
        u8* obj = *(u8**)(arg0 + 0x4);
        *(u32*)(obj + 0x20) |= 0x8000;
        *(u32*)(obj + 0x20) |= 0x10000000;
        *(u32*)(obj + 0x20) |= 0x4000;
        *(u32*)(obj + 0x20) |= 0x10000000;
    }

    // Integrate the acceleration into the velocity and clear it.
    f32 dvx = *(f32*)(arg0 + 0x758) * farg0;
    f32 dvy = *(f32*)(arg0 + 0x75C) * farg0;
    f32 dvz = *(f32*)(arg0 + 0x760) * farg0;
    *(f32*)(arg0 + 0x844) = *(f32*)(arg0 + 0x844) + dvx;
    *(f32*)(arg0 + 0x758) = 0.0f;
    *(f32*)(arg0 + 0x848) = *(f32*)(arg0 + 0x848) + dvy;
    *(f32*)(arg0 + 0x84C) = *(f32*)(arg0 + 0x84C) + dvz;
    *(f32*)(arg0 + 0x75C) = 0.0f;
    *(f32*)(arg0 + 0x760) = 0.0f;
}

}  // extern "C"

// Shared by fn_8005A65C: lets go of the platform the character stands on (runs the platform's
// leave actions) and resets the ground-contact fields.
static void ReleaseGround(u8* a) {
    s32 key = *(s32*)(a + 0x7B0);
    u8* plat = (u8*)fn_801B7198(&key, 0);
    if (plat != NULL) {
        if (*(void**)(plat + 0x8C) == NULL) {
            *(s32*)(a + 0x790) = 1;
            *(u32*)(a + 0x838) |= 0x02000000;
        }
        u8* h = *(u8**)(plat + 0x9C);
        if (h != NULL && ((*(u32*)(h + 0xA4) >> 27) & 0xF)) TriggerDoAction(h + 0xA4, a, plat);
        if ((*(u32*)(plat + 0x74) >> 27) & 0xF) TriggerDoAction(plat + 0x74, a, plat);
    }
    *(u8**)(a + 0x7B4) = NULL;
    *(s32*)(a + 0x7B0) = lbl_805FB6B8;
    *(f32*)(a + 0x7B8) = 0.0f;
    *(f32*)(a + 0x7BC) = 0.0f;
    *(f32*)(a + 0x7C0) = 0.0f;
    *(f32*)(a + 0x7C4) = 0.0f;
    *(f32*)(a + 0x7C8) = 0.0f;
    *(f32*)(a + 0x7CC) = 0.0f;
    *(f32*)(a + 0x7D0) = 1.0f;
}

extern "C" {

// 0x8005A65C
// Ground tracking: damps the velocity, then (if the character is bound to a platform) casts a
// ray straight down onto it. A miss or a platform move sends the character falling or re-binds
// it; without a platform the fall timer (+0x7D8) decides between airborne and grounded.
void fn_8005A65C(u8* arg0) {
    f32 damp = *(f32*)(arg0 + 0x860) * *(f32*)(arg0 + 0x24);
    f32 dx = *(f32*)(arg0 + 0x844) * damp;
    f32 dy = *(f32*)(arg0 + 0x848) * damp;
    f32 dz = *(f32*)(arg0 + 0x84C) * damp;
    *(f32*)(arg0 + 0x844) = *(f32*)(arg0 + 0x844) - dx;
    *(f32*)(arg0 + 0x848) = *(f32*)(arg0 + 0x848) - dy;
    *(f32*)(arg0 + 0x84C) = *(f32*)(arg0 + 0x84C) - dz;

    s32 key = *(s32*)(arg0 + 0x7B0);
    u8* ground = (u8*)fn_801B7198(&key, 0);
    if (ground != NULL && *(void**)(arg0 + 0x7B4) != NULL && *(void**)(ground + 0x8C) != NULL) {
        // keep the current platform
    } else {
        ReleaseGround(arg0);
        ground = NULL;
    }

    if (ground != NULL) {
        *(s32*)(arg0 + 0x748) = -1;
        u8 q[0x6C], hit[0x6C], hit2[0x6C], hitWorld[0x6C];
        RayQueryInit(q);
        RayHitInit(hit);
        RayHitInit(hit2);
        memset(hitWorld, 0, sizeof(hitWorld));
        *(s32*)(hitWorld + 0x54) = 1;

        // Ray from the character position straight down.
        RayQuerySetStart(q, *(const Vec3**)(arg0 + 0x4));
        Vec3 down = { -0.0f, -0.0f, -1.0f };
        RayQuerySetDir(q, &down);

        void* world = *(void**)(ground + 0x8C);
        if (!(*(u32*)(*(u8**)(ground + 0x4) + 0x20) & 2) || world == NULL ||
            RayQueryCast(q, world, kHashBAse, hit, hit2) == 0) {
            // No hit on the platform: fall, and look for ground in the world instead.
            *(s32*)(arg0 + 0x790) = 1;
            *(u32*)(arg0 + 0x838) |= 0x02000000;
            Vec3 dir = { -0.0f, -0.0f, -1.0f };
            Vec3 from;
            from.x = *(f32*)(arg0 + 0x74C);
            from.y = *(f32*)(arg0 + 0x750);
            from.z = *(f32*)(arg0 + 0x754);
            fn_80139140(gWorldState, (f32*)&from, (f32*)&dir, hitWorld, 0, 0);
            *(f32*)(arg0 + 0x7D8) = *(f32*)(hitWorld + 0x18);
        } else {
            u8* col = *(u8**)(arg0 + 0x7B4);
            f32 mz = *(f32*)(col + 0x1C) - *(f32*)(arg0 + 0x7C0);
            f32 mx = *(f32*)(col + 0x14) - *(f32*)(arg0 + 0x7B8);
            f32 my = *(f32*)(col + 0x18) - *(f32*)(arg0 + 0x7BC);
            f32 dist = *(f32*)(hit + 0x18);
            if (mz != 0.0f || mx != 0.0f || my != 0.0f) {
                // The platform moved since the last frame.
                if (dist <= 0.1f) {
                    *(f32*)(arg0 + 0x7D8) = 0.0f;
                    *(f32*)(arg0 + 0x7A4) = 0.0f;
                    *(f32*)(arg0 + 0x7A8) = 0.0f;
                    *(f32*)(arg0 + 0x7AC) = 1.0f;
                    CharacterResetGroundState(arg0, 0, 0);
                } else {
                    ReleaseGround(arg0);
                    CharacterResetGroundState(arg0, 1, 0);
                    *(f32*)(arg0 + 0x7A4) = 0.0f;
                    *(f32*)(arg0 + 0x7A8) = 0.0f;
                    *(f32*)(arg0 + 0x7AC) = 1.0f;
                    *(f32*)(arg0 + 0x7D8) = dist;
                }
            } else if (dist <= 0.1f) {
                *(f32*)(arg0 + 0x7D8) = 0.0f;
                CharacterResetGroundState(arg0, 0, 0);
            } else {
                ReleaseGround(arg0);
                *(s32*)(arg0 + 0x790) = 1;
                *(u32*)(arg0 + 0x838) |= 0x02000000;
                *(f32*)(arg0 + 0x7D8) = -1.0f;
            }
        }
        fn_801ACCA0(q, 0);
        return;
    }

    if (*(f32*)(arg0 + 0x7D8) < 0.0f) {
        CharacterResetGroundState(arg0, 1, 0);
        *(u32*)(arg0 + 0x838) &= 0xF7FFFFFF;
        return;
    }
    CharacterResetGroundState(arg0, 0, 0);
    *(u32*)(arg0 + 0x838) |= 0x08000000;
}

// Surface kind (hit +0x48 bits 12..15) to surface hash stored at +0x14C.
static const u32 kSurfaceHash[16] = {
    0xDC1BFEC5u,   // "default"
    0x1B164D9Cu,   // "gOo"
    0x5F595D43u,   // "grass"
    0x0C67D66Au,   // "water"
    0x0A08F4B9u,   // "wood"
    0xB5CE36F5u,
    0x37363679u,   // "metal"
    0x401A44B0u,   // "TILE"
    0xAEF20C02u,   // "dirt"
    0x1A811CBEu,
    0xDC1BFEC5u, 0xDC1BFEC5u, 0xDC1BFEC5u, 0xDC1BFEC5u, 0xDC1BFEC5u, 0xDC1BFEC5u,
};

// 0x80065FF0
// Per-frame character movement: applies gravity-like acceleration (+0x1B4), moves the position
// by the velocity (+0x1AC) (with an ease-in curve while +0x220/+0x224 are active), snaps to a
// stored point when fn_80068450 says so, then, unless moving is blocked, ray-tests the new
// position against the world. On a hit the character is pushed out along the surface normal,
// the surface hash is recorded, and (when bounce bit 0x100 is set) the velocity is reflected
// off the normal, scaled and a hit effect is spawned.
void fn_80065FF0(u8* arg0, f32 farg0) {
    u8* rec = *(u8**)(arg0 + 0x228);
    f32 gravity = *(f32*)(rec + 0xD4);
    if (gravity != 0.0f && !(*(u32*)(arg0 + 0x244) & 0x04000000)) {
        *(f32*)(arg0 + 0x1B4) = gravity * farg0 + *(f32*)(arg0 + 0x1B4);
    }

    u32 flags = *(u32*)(arg0 + 0x244);
    if (flags & 0x80) return;

    if (flags & 0x20) {
        // Eased move: a sine ramp of the +0x230 timer blends in the +0x210 vector.
        f32 t = *(f32*)(arg0 + 0x230);
        if (!(t > 0.0f)) t = 0.0f;
        f32 s = sinf(1.5707964f * (3.1415927f * (t * *(f32*)(arg0 + 0x224))));
        f32 rest = *(f32*)(arg0 + 0x220) - t;
        f32 w = *(f32*)(arg0 + 0x21C) * s;
        f32 vx = *(f32*)(arg0 + 0x1AC) * rest;
        f32 vy = *(f32*)(arg0 + 0x1B0) * rest;
        f32 vz = *(f32*)(arg0 + 0x1B4) * rest;
        f32 wx = *(f32*)(arg0 + 0x210) * w;
        f32 wy = *(f32*)(arg0 + 0x214) * w;
        f32 wz = *(f32*)(arg0 + 0x218) * w;
        f32* pos = *(f32**)(arg0 + 0x4);
        pos[0] = (*(f32*)(arg0 + 0x194) + vx) + wx;
        pos[1] = (*(f32*)(arg0 + 0x198) + vy) + wy;
        pos[2] = (*(f32*)(arg0 + 0x19C) + vz) + wz;
    } else {
        f32 t = *(f32*)(arg0 + 0x230);
        f32 step = (t >= 0.0f) ? farg0 : farg0 + t;
        f32* pos = *(f32**)(arg0 + 0x4);
        pos[0] = pos[0] + *(f32*)(arg0 + 0x1AC) * step;
        pos[1] = pos[1] + *(f32*)(arg0 + 0x1B0) * step;
        pos[2] = pos[2] + *(f32*)(arg0 + 0x1B4) * step;
    }

    if (!(*(u32*)(arg0 + 0x244) & 0x8) && fn_80068450(arg0) != 0) {
        *(f32*)(arg0 + 0x230) = 0.0f;
        f32* pos = *(f32**)(arg0 + 0x4);
        pos[0] = *(f32*)(arg0 + 0x1D0);
        pos[1] = *(f32*)(arg0 + 0x1D4);
        pos[2] = *(f32*)(arg0 + 0x1D8);
        *(u32*)(arg0 + 0x244) &= 0xFBFFFFFF;
    }

    flags = *(u32*)(arg0 + 0x244);
    if (flags & 0x800) return;
    if ((flags & 0x8000) && *(s32*)(arg0 + 0x248) != 0) return;
    if (flags & 0x10000) return;

    u8 hit[0x6C];
    memset(hit, 0, sizeof(hit));
    *(s32*)(hit + 0x54) = 4;
    f32 from[3];
    f32* cur = *(f32**)(arg0 + 0x4);
    from[0] = cur[0];
    from[1] = cur[1];
    from[2] = cur[2];
    if (fn_80138C40(gWorldState, (f32*)(arg0 + 0x1A0), from, (f32*)hit, NULL, 0, 0) == 0) return;

    // Surface the character is touching.
    *(u32*)(arg0 + 0x14C) = kSurfaceHash[(*(u32*)(hit + 0x48) >> 12) & 0xF];

    // Stand 0.1 above the hit point along the normal.
    const f32* hp = (const f32*)hit;
    f32 ox = hp[3] * 0.1f;
    f32 oy = hp[4] * 0.1f;
    f32 oz = hp[5] * 0.1f;
    f32* pos = *(f32**)(arg0 + 0x4);
    pos[0] = hp[0] + ox;
    pos[1] = hp[1] + oy;
    pos[2] = hp[2] + oz;

    pos = *(f32**)(arg0 + 0x4);
    u32 fl = *(u32*)(arg0 + 0x244) | 0x04000000;
    rec = *(u8**)(arg0 + 0x228);
    *(f32*)(arg0 + 0x1B8) = pos[0];
    *(f32*)(arg0 + 0x1BC) = pos[1];
    *(f32*)(arg0 + 0x1C0) = pos[2];
    *(f32*)(arg0 + 0x1C4) = hp[3];
    *(f32*)(arg0 + 0x1C8) = hp[4];
    *(f32*)(arg0 + 0x1CC) = hp[5];
    *(u32*)(arg0 + 0x244) = fl;

    void* snd = *(void**)(rec + 0x10C);
    if (snd != NULL && !(*((u8*)snd + 5) & 1)) PlaySoundAt(snd, 0, pos, 1);

    fl = *(u32*)(arg0 + 0x244);
    if (fl & 0x100) {
        // Bounce: reflect the velocity about the surface normal, keep its direction and give it
        // the record's rebound speed (+0xB8) times 0.75.
        f32 vx = *(f32*)(arg0 + 0x1AC);
        f32 vy = *(f32*)(arg0 + 0x1B0);
        f32 vz = *(f32*)(arg0 + 0x1B4);
        f32 nx = *(f32*)(arg0 + 0x1C4);
        f32 ny = *(f32*)(arg0 + 0x1C8);
        f32 nz = *(f32*)(arg0 + 0x1CC);
        f32 dot = vz * nz + (vx * nx + vy * ny);
        *(u32*)(arg0 + 0x244) = fl & 0xFBFFFFFF;
        f32 rx = vx - (nx * 2.0f) * dot;
        f32 ry = vy - (ny * 2.0f) * dot;
        f32 rz = vz - (nz * 2.0f) * dot;
        f32 r[3] = { rx, ry, rz };
        NormalizeInPlace3(r);
        rec = *(u8**)(arg0 + 0x228);
        f32 speed = 0.75f * *(f32*)(rec + 0xB8);
        *(f32*)(arg0 + 0x1AC) = r[0] * speed;
        *(f32*)(arg0 + 0x1B0) = r[1] * speed;
        *(f32*)(arg0 + 0x1B4) = r[2] * speed;

        u32 fx = *(u32*)(rec + 0xF4);
        if (fx == 0) return;
        u8* eff;
        if (fn_80064568(lbl_8047C760, fx, (const f32*)(arg0 + 0x1B8), &eff) == 0) return;
        fn_8006680C(eff, (const f32*)(arg0 + 0x1C4));
        fn_80066960(eff);
    } else if (fl & 0x10) {
        *(f32*)(arg0 + 0x1AC) = 0.0f;
        *(f32*)(arg0 + 0x1B0) = 0.0f;
        *(f32*)(arg0 + 0x1B4) = 0.0f;
    } else {
        *(f32*)(arg0 + 0x230) = 0.0f;
    }
}

// 0x80066E54
// Character teardown (the body of the destructor): releases the sound handle, posts the "died"
// event with the final position, plays the despawn effect and sound for the situation the
// character is in (hit by something, in its death state, attached, or plain), frees the effect
// list and handles, then calls virtual slot 0x18 (delete).
void fn_80066E54(u8* arg0) {
    u8* h = *(u8**)(arg0 + 0xA0);
    if (h != NULL) {
        fn_80007CD4(h);
        *(s32*)(arg0 + 0xA0) = 0;
    }
    if (fn_80053ACC(arg0, 0x04000000) == 0) {
        u8* ev = (u8*)EventPost(1, *(void**)(arg0 + 0x24C));
        if (ev != NULL) {
            if (*(u8**)(arg0 + 0x24C) != NULL) {
                fn_800651E4((const f32*)(arg0 + 0x1D0), (f32*)(ev + 4));
            } else if (fn_80066800(arg0) != 0) {
                fn_800651E4((const f32*)(arg0 + 0x1B8), (f32*)(ev + 4));
            } else {
                fn_800651E4(*(const f32**)(arg0 + 0x4), (f32*)(ev + 4));
            }
            *(s32*)(ev + 0x10) = *(s32*)(arg0 + 0x30);
        }
    }

    ScopedLock lock;
    ScopedLockCtor(&lock);
    fn_8005379C(arg0, 5, 0);

    if (fn_80054988(arg0 + 0x244, 0x10) != 0) {
        u8* rec = *(u8**)(arg0 + 0x228);
        fn_80064568(lbl_8047C760, *(u32*)(rec + 0xF8), *(const f32**)(arg0 + 0x4), NULL);
        rec = *(u8**)(arg0 + 0x228);
        void* snd = *(void**)(rec + 0x110);
        if (snd != NULL && fn_800653E8(snd) == 0) {
            PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x228) + 0x110), 0, *(void**)(arg0 + 0x4), 0);
        }
    } else if (*(u8**)(arg0 + 0x24C) != NULL) {
        Vec3 up;                         // 0x120
        Vec3SetUnitZ(&up);
        u8* other = *(u8**)(arg0 + 0x24C);
        if (fn_800651C8(other, 0x50) != 0) {
            s8 kind = (s8) * (s16*)(*(u8**)(*(u8**)(arg0 + 0x228) + 0x14C) + 0x54);
            Vec3Copy((Vec3*)fn_800526DC(arg0), (const Vec3*)(arg0 + 0x1D0));
            Vec3Copy((Vec3*)fn_8005E128(arg0), (const Vec3*)(arg0 + 0x1D0));

            Vec3 impulse;                // 0x114
            Vec3SetZero(&impulse);
            u8* rec = *(u8**)(arg0 + 0x228);
            if (fn_800676AC(*(f32*)(rec + 0xDC), 0.0f) == 0) {
                Vec3 v, vn, scaled;
                Scalar sc;
                Vec3Assign(&v, (const Vec3*)(arg0 + 0x1AC));
                Vec3NormalizeTo(&vn, &v);
                Vec3Copy(&v, &vn);
                ScalarSet(&sc, *(f32*)(*(u8**)(arg0 + 0x228) + 0xDC));
                Vec3MulScalar(&scaled, &v, &sc);
                Vec3Copy(&impulse, &scaled);
                Vec3SetZ(&impulse, 0.0f);
            }

            // Pushed by the player: a small (0.075 scaled) shove along the velocity.
            u8* player = *(u8**)(*(u8**)gWorldState + 8);
            if (*(u8**)(arg0 + 0x24C) == player && *(u8**)(arg0 + 0x23C) != NULL &&
                fn_800544D8(*(u8**)(arg0 + 0x23C), 4) != 0 && *(s32*)(*(u8**)(arg0 + 0x24C) + 0x78C) == 1) {
                Scalar k;
                Vec3 shove;
                ScalarSet(&k, 0.075f);
                Vec3MulScalar(&shove, (const Vec3*)(arg0 + 0x1AC), &k);
                Vec3Copy(&impulse, &shove);
                kind = 3;
            }

            u8* myField = *fn_800655D0(arg0);
            if (fn_800544D8(*(u8**)(arg0 + 0x24C), 4) != 0) {
                u8* otherField = *fn_800655D0(*(u8**)(arg0 + 0x24C));
                s32 idx = *(s32*)(otherField + 0x1EC);
                if (idx != -1) {
                    Vec3Copy(&impulse, (const Vec3*)(*(u8**)(myField + 0x14C) + idx * 0xC));
                    f32 rot[4];                  // 0xF8
                    fn_8005263C(rot);
                    if (Vec3LengthSqOf((const Vec3*)(arg0 + 0x1AC)) < 0.010000001f) {
                        Vec3 fwd, delta;
                        Vec3SetUnitZ(&fwd);
                        u8* selfAnchor = fn_8005E128(arg0);
                        u8* selfPos = fn_800526DC(arg0);
                        u8* o = *(u8**)(arg0 + 0x24C);
                        Vec3* otherPt = (*(FnVirtGetPoint*)(*(u8**)o + 0x44))(o, selfPos, arg0);
                        Vec3Sub(&delta, otherPt, (const Vec3*)selfAnchor);
                        fn_801458B0(rot, (f32*)&delta, (f32*)&fwd);
                    } else {
                        Vec3 dir, dirN, fwd;
                        Scalar zero;
                        Vec3Assign(&dir, (const Vec3*)(arg0 + 0x1AC));
                        ScalarZero(&zero);
                        fn_800676D4(&dir, &zero);
                        Vec3NormalizeTo(&dirN, &dir);
                        Vec3Copy(&dir, &dirN);
                        Vec3SetUnitZ(&fwd);
                        fn_801458B0(rot, (f32*)&dir, (f32*)&fwd);
                    }
                    Vec3 rotated;
                    fn_80054650((u8*)&rotated, (u8*)rot, (u8*)&impulse);
                    Vec3Copy(&impulse, &rotated);
                }
            }

            u8* item = *(u8**)(*(u8**)(arg0 + 0x228) + 0x14C);
            fn_80067BB0(arg0, *(u8**)(arg0 + 0x24C), kind, &impulse, *(f32*)(item + 0x48));

            // Let the other entity react at our position.
            Vec3 myPos;
            Vec3Assign(&myPos, (const Vec3*)(arg0 + 0x1D0));
            (*(FnVirtApply*)(*(u8**)arg0 + 0x54))(arg0, *(u8**)(arg0 + 0x24C), &myPos);

            // Effect direction: from the other entity's point towards us, flattened.
            u8* o = *(u8**)(arg0 + 0x24C);
            Vec3* otherPt = (*(FnVirtGetPoint*)(*(u8**)o + 0x44))(o, fn_800526DC(arg0), arg0);
            Vec3 otherPos, toUs, toUsN;
            Vec3Assign(&otherPos, otherPt);
            Vec3Sub(&toUs, (const Vec3*)(arg0 + 0x1D0), &otherPos);
            Vec3SetZ(&toUs, 0.0f);
            f32 len;
            fn_800636A0();
            fn_800676E0((f32*)&toUsN, (const f32*)&toUs, &len);
            Vec3Copy(&toUs, &toUsN);
            if (fn_800677A0(&len) != 0) {
                Vec3 unitZ;
                Vec3SetUnitZ(&unitZ);
                Vec3Copy(&toUs, &unitZ);
            } else {
                Vec3 flat;
                Vec3SetZ(&toUs, fn_800677D0(0.25f, 0.5f));
                Vec3NormalizeTo(&flat, &toUs);
                Vec3Copy(&toUs, &flat);
            }

            u8* oth = *(u8**)(arg0 + 0x24C);
            if (*(u8**)(oth + 0x9C) != NULL &&
                (fn_800544D8(oth, 5) == 0 || *(u8*)(*(u8**)(arg0 + 0x24C) + 0xBDE) == 0) &&
                (fn_800544D8(*(u8**)(arg0 + 0x24C), 4) == 0 || fn_80054988(oth + 0x838, 0x20000) == 0)) {
                u32 fx = *(u32*)(*(u8**)(*(u8**)(arg0 + 0x24C) + 0x9C) + 0x28);
                if (fx != 0) {
                    u8* eff;
                    if (fn_80064568(lbl_8047C760, fx, (const f32*)(arg0 + 0x1D0), &eff) != 0) {
                        fn_8006680C(eff, (const f32*)&toUs);
                        fn_80066960(eff);
                    }
                }
            }
        }

        // Sound and effect for a hit by the player or by something else.
        u8* player = *(u8**)(*(u8**)gWorldState + 8);
        u32 fx;
        if (*(u8**)(arg0 + 0x24C) == player) {
            void* snd = *(void**)(*(u8**)(arg0 + 0x228) + 0x114);
            if (snd != NULL && fn_800653E8(snd) == 0) {
                PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x228) + 0x114), 0, *(void**)(arg0 + 0x4), 0);
            }
            fx = *(u32*)(*(u8**)(arg0 + 0x228) + 0xFC);
        } else {
            void* snd = *(void**)(*(u8**)(arg0 + 0x228) + 0x118);
            if (snd != NULL && fn_800653E8(snd) == 0) {
                PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x228) + 0x118), 0, *(void**)(arg0 + 0x4), 0);
            }
            fx = *(u32*)(*(u8**)(arg0 + 0x228) + 0x100);
        }
        if (fx != 0) {
            u8* eff;
            if (fn_80064568(lbl_8047C760, fx, (const f32*)(arg0 + 0x1D0), &eff) != 0) {
                fn_8006680C(eff, (const f32*)&up);
                fn_80066960(eff);
            }
        }
    } else if (fn_80066800(arg0) != 0) {
        Vec3 at;
        Vec3Assign(&at, (const Vec3*)(arg0 + 0x1B8));
        u32 fx = *(u32*)(*(u8**)(arg0 + 0x228) + 0xF4);
        if (fx != 0) {
            u8* eff;
            if (fn_80064568(lbl_8047C760, fx, (const f32*)&at, &eff) != 0) {
                fn_8006680C(eff, (const f32*)(arg0 + 0x1C4));
                fn_80066960(eff);
            }
        }
        void* snd = *(void**)(*(u8**)(arg0 + 0x228) + 0x110);
        if (snd != NULL && fn_800653E8(snd) == 0) {
            PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x228) + 0x110), 0, *(void**)(arg0 + 0x4), 1);
        }
        fn_8005379C(arg0, 0x11, 0);
        Vec3 at2;
        Vec3Assign(&at2, (const Vec3*)(arg0 + 0x1B8));
        (*(FnVirtApply*)(*(u8**)arg0 + 0x54))(arg0, NULL, &at2);
    } else if (*(u8**)(arg0 + 0x24C) == NULL) {
        u32 fx = *(u32*)(*(u8**)(arg0 + 0x228) + 0xF8);
        if (fx != 0) {
            fn_80064568(lbl_8047C760, fx, *(const f32**)(arg0 + 0x4), NULL);
            void* snd = *(void**)(*(u8**)(arg0 + 0x228) + 0x110);
            if (snd != NULL && fn_800653E8(snd) == 0) {
                PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x228) + 0x110), 0, *(void**)(arg0 + 0x4), 0);
            }
        }
    }

    // Release the attached effect list and its handle.
    if (*(u8**)(arg0 + 0xA8) != NULL) {
        for (s32 i = 0; ; ++i) {
            u8* list = *(u8**)(arg0 + 0xA8);
            if (list == NULL || i >= *(s32*)(list + 0xC)) break;
            u8* item = *(u8**)(*(u8**)(arg0 + 0xA8) + i * 4 + 0x14);
            if (item != NULL && *(s32*)(fn_80067844(item) + 0x1D4) == 0) {
                fn_801C85E0(lbl_8047C760, *(u8**)(*(u8**)(arg0 + 0xA8) + i * 4 + 0x14));
            }
        }
        if (*(u8**)(arg0 + 0xA8) != NULL) fn_801C849C(lbl_8047C760, *(u8**)(arg0 + 0xA8));
        if (*(s32*)(arg0 + 0xAC) != -1) {
            fn_8020A398(*(s32*)(arg0 + 0xAC));
            *(s32*)(arg0 + 0xAC) = -1;
        }
    }

    (*(FnVirtDestroy*)(*(u8**)arg0 + 0x18))(arg0, 1);
    if (*(s32*)(arg0 + 0x250) != -1) {
        fn_8020A398(*(s32*)(arg0 + 0x250));
        *(s32*)(arg0 + 0x250) = -1;
    }
    ScopedLockDtor(&lock, -1);
}

}  // extern "C"
