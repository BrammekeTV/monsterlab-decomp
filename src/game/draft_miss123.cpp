// Misc decompiled functions: character aim checks, lob launch, experiment actor tracking,
// light prop motion, rect conversion, turntable slot setup, experiment ring/orbit update, ...
#include <math.h>
#include <string.h>

#include "types.h"
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C++" f32 ComputeLobSpeed(const Vec3* from, const Vec3* to, f32 angle, f32 gravity);   // 0x80019FB0 (projectile_math.cpp)

extern "C" {
void CharacterResetGroundState(u8* c, s32 a, s32 b);                 // 0x8005BAD4
void EulerToQuat(Quat* out, const Vec3* euler);
u32  RandomU32(void);                                                 // 0x80143460
s32  RaySceneTest(void* ray);                                         // 0x80070458 (really returns lbl_805FB124)
void ScopedLockCtor(ScopedLock* l);                                   // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                        // 0x801F53B0
void Vec3Assign(Vec3* out, const Vec3* v);
void Vec3Copy(Vec3* out, const Vec3* v);
void Vec3NormalizeTo(Vec3* out, const Vec3* v);                       // 0x80055D9C
void Vec3SetUnitZ(Vec3* v);
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);
void PlayExperimentSound(void* owner, u32 id, s32 vol);
void StopExperimentSound(void* owner, u32 id);

void fn_8005263C(f32* q);                                             // 0x8005263C (quat identity)
void fn_800526D0(u8* out);                                            // 0x800526D0 (matrix identity)
u8*  fn_800526D4(u8* mtx);                                            // 0x800526D4 (matrix translation)
f32  fn_800531E4(u8* dst, u8* src);                                   // 0x800531E4 (copy quat)
f32  fn_80053208(u8* q);                                              // 0x80053208 (identity quat)
f32  fn_800549F4(u8* obj);                                            // 0x800549F4
void fn_8005535C(u8* dst, const u8* src);
f32  fn_800553C0(u8* mtx, const Vec3* v);
void fn_800553DC(f32* q, u8* mtx);                                    // 0x800553DC (quat to matrix)
u8*  fn_800554D4(u8* mtx);                                            // 0x800554D4
void fn_800554DC(u8* out, const u8* a, const u8* b);                  // 0x800554DC (matrix multiply)
f32  fn_800570F8(u8* q);                                              // 0x800570F8 (quat length squared)
void fn_800557B4(u8* out);
void fn_80055828(f32* out, u8* mtx, const Vec3* v);                   // 0x80055828
s32  fn_80058C00(f32 x);                                              // 0x80058C00
f32  fn_8005B4A4(const Vec3* a, const Vec3* b);                       // 0x8005B4A4 (dot)
void fn_8005A244(Vec3* out, const Vec3* a, const Vec3* b);            // 0x8005A244 (cross)
void fn_8005C020(u8* skel, s32 idx, u8* outMtx);                      // 0x8005C020
u8*  fn_8005C148(u8* skel, s32 idx);                                  // 0x8005C148
void fn_8005C158(Vec3* out, const Vec3* v);                           // 0x8005C158
void fn_8005C1CC(f32* out, f32* q, f32 s);                            // 0x8005C1CC
void fn_8005C204(u8* out, u8* in);                                    // 0x8005C204 (matrix inverse)
s32  fn_8005F444(u8* a3);                                             // 0x8005F444
u8*  fn_8005E128(u8* a3);                                             // 0x8005E128
void fn_80144624(u8* mtx, const Vec3* axis, f32 angle);               // 0x80144624
void fn_80173EB0(u8* skel, s32 idx, u8* mtx);                         // 0x80173EB0
void fn_8014474C(u8* src, f32* out);
void fn_801455E0(f32* out, u8* quat);
void fn_801458B0(f32* quat, f32* a, f32* b);
void fn_80145F9C(f32* out, f32* a, f32* b, f32 t);
void fn_80146180(void* quat, const Vec3* axis, f32 angle);
void fn_8018A948(u8* obj, f32 t);
f32  fn_8018BE30(void* obj, f32 a);
void fn_8018C3D0(void* p);
void* fn_801B50FC(u8* p);                                             // 0x801B50FC (returns the object's transform matrix)
u8*  fn_801C668C(u8* key);
void* fn_801D40E0(s32 n);                                             // 0x801D40E0 (operator new)
s32  fn_801DA780(u8* a3);
u8*  fn_801DA788(u8* a3, s32 i);
void* fn_801DA798(void* p);
f64  floor(f64 x);                                              // 0x8033ED6C (floor)
f64  fn_8033F1F4(f64 x);                                              // 0x8033F1F4 (acos)
f64  fn_8033F1F8(f64 x);                                              // 0x8033F1F8 (asin)

f32 fn_8007C5B4(u8* obj, u8* a, u8* b);
f32 fn_8007A3F4(u8* obj, u8* a, u8* b);
void fn_8007C5E4(u8* obj, s32* w, s32* h);
void fn_8007A418(u8* obj, u8* a, u8* b);
void fn_800690D4(u8* p, s32 a, void* params, s32 b);                  // 0x800690D4 (base ctor)
void fn_800980F8(u8* p, s32 b);                                       // 0x800980F8
void fn_800712B4(void);                                               // 0x800712B4
void fn_800712B8(void);                                               // 0x800712B8
void fn_80070FD8(u8* obj);                                            // 0x80070FD8
void fn_8008C104(Quat* out, u8* obj);                                 // 0x8008C104 (copies the quat at +0x26C to out)
void fn_8008C128(Vec3* out, u8* obj);                                 // 0x8008C128 (copies the node position to out)
s32  LabExperimentGrade(u8* lab);

u8* fn_80077D1C(u8* p);   // 0x80077D1C
u8* fn_800756E0(u8* p);   // 0x800756E0
u8* fn_8009F8E8(u8* p);   // 0x8009F8E8
u8* fn_8007D4DC(u8* p);   // 0x8007D4DC
u8* fn_8008ED38(u8* p);   // 0x8008ED38
u8* fn_800940BC(u8* p);   // 0x800940BC
u8* fn_80099AF4(u8* p);   // 0x80099AF4
u8* fn_800A79A0(u8* p);   // 0x800A79A0
u8* fn_80086BE8(u8* p);   // 0x80086BE8
u8* fn_800B2384(u8* p);   // 0x800B2384
u8* fn_800B9AA0(u8* p);   // 0x800B9AA0
u8* fn_800CE480(u8* p);   // 0x800CE480
u8* fn_800D02C0(u8* p);   // 0x800D02C0

extern s32 gScreenHeight;           // 0x805F938C
extern s32 gScreenWidth;            // 0x805F9388
extern void* gLab;                  // 0x805FBBC8
extern u8 gWorldState[];            // 0x803E6D00
extern u8 lbl_80348F38[];           // 0x80348F38 (overlay dispatch table)
extern f32 lbl_80346870[];          // 0x80346870
extern f32 lbl_80346884[];          // 0x80346884
extern f32 lbl_80346FA0[];          // 0x80346FA0
extern f32 lbl_80347040[];          // 0x80347040
extern f32 lbl_80347050[];          // 0x80347050
extern f32 lbl_80347060[];          // 0x80347060
extern u8 lbl_803C4298[];           // 0x803C4298
extern f32 lbl_803C5670[];          // 0x803C5670 (quat, identity once initialised)
extern u8 lbl_805FB11C;             // 0x805FB11C
extern u8* lbl_805FB120;            // 0x805FB120 (tracked actor)
extern u8* lbl_805FB134;            // 0x805FB134 (experiment object)
extern s32 lbl_805FB124;            // 0x805FB124
extern u8 lbl_8035B8F0[];            // 0x8035B8F0 (vtable)
}

typedef u8*  (*GetObjFn)(u8*);
typedef void (*OverlayFn)(void*, s32, u32);
typedef void (*VFn)(u8*, s32);
typedef void (*VFn0)(u8*);

static const u32 kHashRattle = 0x1228CC56u;    // "reactor_leak_plate_rattle_01"
static const u32 kHashTestObj = 0x7330664Au;   // "test_object_01A"

static inline f32 Clamp01(f32 v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtPos(f32 x) {
    return x > 0.0f ? sqrtf(x) : 0.0f;  // original: frsqrte with two Newton steps
}

static inline u32 F32Bits(f32 f) {
    u32 r;
    memcpy(&r, &f, 4);
    return r;
}

// Clamp p into the range spanned by a and b (either order).
static inline f32 ClampBetween(f32 p, f32 a, f32 b) {
    f32 lo = b;
    f32 hi = a;
    if (!(b < a)) {
        lo = a;
        hi = b;
    }
    if (p < lo) return lo;
    if (p > hi) return hi;
    return p;
}

// floor(n * frac) as an int.
static inline s32 FloorMul(s32 n, f32 frac) {
    return (s32)(f32)floor((f64)((f32)n * frac));
}

// Rotates the actor velocity (actor + 0xC0..) by the node quaternion (node + 0xC..) and moves the node
// by it, scaled per axis by arg0 + 0x8/0xC/0x10.
static void MoveNodeByActorVelocity(u8* arg0, u8* node, u8* actor) {
    f32 qx = *(f32*)(node + 0xC);
    f32 qy = *(f32*)(node + 0x10);
    f32 qz = *(f32*)(node + 0x14);
    f32 qw = *(f32*)(node + 0x18);
    f32 vx = *(f32*)(actor + 0xC0);
    f32 vy = *(f32*)(actor + 0xC4);
    f32 vz = *(f32*)(actor + 0xC8);
    f32 zero = 0.0f;
    f32 nqx = -qx;
    f32 nqy = -qy;
    f32 nqz = -qz;
    // t = q * (0, v)
    f32 t0 = ((qy * vz) + ((qw * vx) + (qx * zero))) - (qz * vy);
    f32 t3 = (((qw * zero) - (qx * vx)) - (qy * vy)) - (qz * vz);
    f32 t1 = (qz * vx) + ((qy * zero) + ((qw * vy) - (qx * vz)));
    f32 t2 = (qz * zero) + (((qw * vz) + (qx * vy)) - (qy * vx));
    // r = t * conj(q)
    f32 rx = ((t1 * nqz) + ((t3 * nqx) + (t0 * qw))) - (t2 * nqy);
    f32 ry = (t2 * nqx) + ((t1 * qw) + ((t3 * nqy) - (t0 * nqz)));
    f32 rz = (t2 * qw) + (((t3 * nqz) + (t0 * nqy)) - (t1 * nqx));
    *(f32*)(node + 0x0) = *(f32*)(node + 0x0) + (rx * *(f32*)(arg0 + 0x8));
    *(f32*)(node + 0x4) = *(f32*)(node + 0x4) + (ry * *(f32*)(arg0 + 0xC));
    *(f32*)(node + 0x8) = *(f32*)(node + 0x8) + (rz * *(f32*)(arg0 + 0x10));
}

// Shared by the two light-prop updates: alpha byte at node + 0x38 from the actor's fade (0..1 -> 0..255).
static void SetNodeAlpha(u8* arg0) {
    f32 a = 255.0f * fn_8018BE30(*(void**)(arg0 + 0xA0), 0.0f);
    if (a >= 255.0f) a = 254.999f;
    *(u8*)(*(u8**)(arg0 + 0x4) + 0x38) = (u8)(s32)a;
}

// Number of occupied slots (slot + 0x26C > 0) among the *(p + 0x1B4) pointers at p + 0x9C.
static s32 CountActiveSlots(u8* p) {
    s32 n = 0;
    s32 cnt = *(s32*)(p + 0x1B4);
    for (s32 i = 0; i < cnt; i++) {
        u8* slot = *(u8**)(p + 0x9C + i * 4);
        if (slot != NULL && *(u8*)(slot + 0x26C) > 0) n++;
    }
    return n;
}

// Type-1 check on the experiment object: returns NULL when the tag at +0 is not 1.
static inline u8* AsExperiment(u8* p) {
    if (p != NULL && *(s32*)p != 1) return NULL;
    return p;
}

extern "C" {

// 0x80060C90
// True when the target is within r, roughly in front (within ~10 degrees in the ground plane) and flag 1 is set.
s32 fn_80060C90(u8* arg0, f32 r) {
    u8* tgt = *(u8**)(arg0 + 0xA14);
    s32 near;
    if (tgt == NULL) {
        near = 0;
    } else {
        u8* tp = *(u8**)(tgt + 0x4);
        u8* sp = *(u8**)(arg0 + 0x4);
        f32 dx = *(f32*)(tp + 0x0) - *(f32*)(sp + 0x0);
        f32 dy = *(f32*)(tp + 0x4) - *(f32*)(sp + 0x4);
        f32 dz = *(f32*)(tp + 0x8) - *(f32*)(sp + 0x8);
        near = ((dz * dz) + ((dx * dx) + (dy * dy))) < (r * r);
    }
    if (near == 0) return 0;

    f32 vx, vy;
    if (*(s32*)(arg0 + 0xA08) & 8) {
        u8* sp = *(u8**)(arg0 + 0x4);
        vx = *(f32*)(arg0 + 0xB88) - *(f32*)(sp + 0x0);
        vy = *(f32*)(arg0 + 0xB8C) - *(f32*)(sp + 0x4);
    } else if (tgt != NULL) {
        u8* tp = *(u8**)(tgt + 0x4);
        u8* sp = *(u8**)(arg0 + 0x4);
        vx = *(f32*)(tp + 0x0) - *(f32*)(sp + 0x0);
        vy = *(f32*)(tp + 0x4) - *(f32*)(sp + 0x4);
    } else {
        return 0;
    }
    f32 vz = 0.0f;
    u8* m = (u8*)fn_801B50FC(arg0);
    f32 wx = *(f32*)(m + 0xC);
    f32 wy = *(f32*)(m + 0x10);
    f32 wz = 0.0f;
    f32 dot = (vz * wz) + ((vx * wx) + (vy * wy));
    f32 lenV = SqrtPos((vz * vz) + ((vx * vx) + (vy * vy)));
    f32 lenW = SqrtPos((wz * wz) + ((wx * wx) + (wy * wy)));
    f32 den = lenV * lenW;
    f32 angle = 0.0f;
    if (!(den <= 0.0f)) {
        f32 c = dot / den;
        if (c < -1.0f) c = -1.0f;
        else if (c > 1.0f) c = 1.0f;
        angle = 0.63661975f * (f32)fn_8033F1F4((f64)c);
    }
    if (angle < 0.17453292f && (*(s32*)(arg0 + 0xA08) & 1)) return 1;
    return 0;
}

// 0x80061CC0
// Lobs a shot at the looked-up target point (+0.5 up): picks a launch direction (at least 45 degrees up),
// computes the speed with ComputeLobSpeed and, if reachable, sets the velocity and resets ground state.
void fn_80061CC0(u8* arg0) {
    u8* t = fn_801C668C(arg0 + 0xBA8);
    if (t == NULL) return;

    Vec3 to;
    to.x = *(f32*)(t + 0x4);
    to.y = *(f32*)(t + 0x8);
    to.z = *(f32*)(t + 0xC) + 0.5f;
    u8* sp = *(u8**)(arg0 + 0x4);
    Vec3 d;
    d.x = to.x - *(f32*)(sp + 0x0);
    d.y = to.y - *(f32*)(sp + 0x4);
    d.z = (to.z - *(f32*)(sp + 0x8)) * 2.0f;
    Vec3 dir = Vec3Normalized(d);
    f32 z = dir.z;
    if (z < -1.0f) z = -1.0f;
    else if (z > 1.0f) z = 1.0f;
    f32 elev = 0.63661975f * (f32)fn_8033F1F8((f64)z);
    if (elev < 0.7853982f) {
        dir.z = 0.0f;
        dir = Vec3Normalized(dir);
        dir.z = 1.0f;
        dir = Vec3Normalized(dir);
        elev = 0.7853982f;
    }
    f32 speed = ComputeLobSpeed((const Vec3*)sp, &to, elev, 6.0f);
    if (speed > 0.0f) {
        *(f32*)(arg0 + 0x844) = dir.x * speed;
        *(f32*)(arg0 + 0x848) = dir.y * speed;
        *(f32*)(arg0 + 0x84C) = dir.z * speed;
        CharacterResetGroundState(arg0, 1, 0);
        *(u8*)(arg0 + 0xBDD) = 1;
        *(u8*)(arg0 + 0xB85) = 0;
    }
}

// 0x800625A4
// Steers one skeleton bone toward the tracked target: builds the aim rotation from the bone to the
// target (dir at +0xA14), slerps the stored quat lbl_803C5670 toward it by dt * fn_800549F4 and writes
// the resulting bone matrix back with fn_80173EB0.
void fn_800625A4(u8* arg0, u8* arg1, f32 dt) {
    s32 idx = fn_8005F444(arg1);
    if (idx == -1) return;
    if (*(void**)(arg0 + 0xA14) == NULL) return;
    if ((s8)lbl_805FB11C == 0) {
        fn_80053208((u8*)lbl_803C5670);
        lbl_805FB11C = 1;
    }
    u8* skel = **(u8***)(arg0 + 0xA0);
    u8 mRest[0x30];
    u8 mBone[0x30];
    u8 mWorld[0x30];
    u8 mAim[0x30];
    u8 mTmp1[0x30];
    u8 mTmp2[0x30];
    u8 mInv1[0x30];
    u8 mInv2[0x30];
    u8 mNew[0x30];
    u8 mRot[0x30];
    fn_800526D0(mRest);
    fn_8005C020(skel, idx, mRest);
    u8* bone = fn_8005C148(skel, idx);
    fn_800554DC(mBone, bone, (u8*)fn_801DA798(*(void**)(arg0 + 0x90)));
    u8* bpos = fn_800526D4(mBone);
    Vec3 dir;
    Vec3 tmp;
    Vec3 a;
    Vec3 b;
    Vec3Sub(&dir, (const Vec3*)fn_8005E128(*(u8**)(arg0 + 0xA14)), (const Vec3*)bpos);
    Vec3NormalizeTo(&tmp, &dir);
    Vec3Copy(&dir, &tmp);
    Vec3Assign(&a, (const Vec3*)fn_800554D4(mBone));
    Vec3Assign(&b, (const Vec3*)fn_800554D4((u8*)fn_801DA798(*(void**)(arg0 + 0x90))));
    fn_8005C158(&tmp, &a);
    Vec3Copy(&a, &tmp);
    fn_8005C158(&tmp, &b);
    Vec3Copy(&b, &tmp);
    f32 dot = fn_8005B4A4(&a, &b);
    fn_800557B4(mAim);
    if (dot > 0.001f) {
        Vec3 axis;
        fn_8005A244(&axis, &b, &a);
        fn_80144624(mAim, &axis, dot);
    }
    f32 q1[4];
    f32 q2[4];
    f32 q3[4];
    f32 q4[4];
    f32 q5[4];
    Vec3 up;
    fn_80055828(q1, mAim, &dir);
    fn_8005263C(q2);
    Vec3SetUnitZ(&up);
    fn_801458B0(q2, q1, &up.x);
    fn_800526D0(mWorld);
    f32 len2 = fn_800570F8((u8*)q2);
    fn_8005263C(q3);
    if (fn_80058C00(len2) != 0) {
        fn_80053208((u8*)q4);
        fn_800531E4((u8*)q3, (u8*)q4);
    } else {
        fn_8005C1CC(q4, q2, 1.0f / len2);
        fn_800531E4((u8*)q3, (u8*)q4);
    }
    fn_800553DC(q3, mWorld);
    fn_800553C0(mWorld, (const Vec3*)fn_800526D4(mBone));
    fn_8005C204(mInv1, mRest);
    fn_800554DC(mTmp1, mInv1, bone);
    fn_800554DC(mTmp2, mTmp1, (u8*)fn_801DA798(*(void**)(arg0 + 0x90)));
    fn_800526D0(mNew);
    fn_8005C204(mInv2, mTmp2);
    fn_800554DC(mRot, mWorld, mInv2);
    fn_8005535C(mNew, mRot);
    f32 q6[4];
    fn_8005263C(q6);
    fn_8014474C(mNew, q6);
    f32 step = dt * fn_800549F4(arg0);
    fn_80145F9C(q5, lbl_803C5670, q6, step);
    fn_800531E4((u8*)lbl_803C5670, (u8*)q5);
    Vec3 npos;
    Vec3Assign(&npos, (const Vec3*)fn_800526D4(mNew));
    f32 len2b = fn_800570F8((u8*)lbl_803C5670);
    if (fn_80058C00(len2b) != 0) {
        fn_80053208((u8*)q4);
        fn_800531E4((u8*)q3, (u8*)q4);
    } else {
        fn_8005C1CC(q4, lbl_803C5670, 1.0f / len2b);
        fn_800531E4((u8*)q3, (u8*)q4);
    }
    fn_800553DC(q3, mNew);
    fn_800553C0(mNew, &npos);
    fn_80173EB0(skel, idx, mNew);
}

// 0x800655D8
// Updates a light prop's scene node: alpha from the actor's fade, draw flags, and (unless pinned
// by flag 4) the node position follows the actor velocity rotated by the node orientation. Ends by
// placing the attach point (+0x18) half a size above the node.
void fn_800655D8(u8* arg0, f32 dt) {
    u8* actor = *(u8**)(arg0 + 0xA0);
    if (actor == NULL || (*(s32*)(arg0 + 0x244) & 2)) return;
    fn_8018A948(actor, dt);
    SetNodeAlpha(arg0);
    u8* node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x8000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x10000000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x4000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x10000000;
    if (*(s32*)(arg0 + 0x244) & 4) return;

    actor = *(u8**)(arg0 + 0xA0);
    s32 af = *(s32*)(actor + 0xF0);
    if ((af & 0x10) && !(af & 8)) {
        u8* n = *(u8**)(arg0 + 0x4);
        *(f32*)(n + 0x0) = *(f32*)(arg0 + 0xC8);
        *(f32*)(n + 0x4) = *(f32*)(arg0 + 0xCC);
        *(f32*)(n + 0x8) = *(f32*)(arg0 + 0xD0);
        fn_8018C3D0(*(void**)(arg0 + 0xA0));
    } else {
        MoveNodeByActorVelocity(arg0, *(u8**)(arg0 + 0x4), actor);
    }
    f32 h = 0.5f * *(f32*)(arg0 + 0x14);
    u8* n = *(u8**)(arg0 + 0x4);
    f32 dxy = 0.0f * h;
    f32 dz = 1.0f * h;
    *(f32*)(arg0 + 0x18) = *(f32*)(n + 0x0) + dxy;
    *(f32*)(arg0 + 0x1C) = *(f32*)(n + 0x4) + dxy;
    *(f32*)(arg0 + 0x20) = *(f32*)(n + 0x8) + dz;
}

// 0x8006A764
// Same node update as fn_800655D8 for the other prop type: gated by flags at +0x15C (1, 0x04000000,
// 0x00200000, 0x00400000) instead of +0x244.
void fn_8006A764(u8* arg0, f32 dt) {
    u8* actor = *(u8**)(arg0 + 0xA0);
    if (actor == NULL) return;
    s32 f = *(s32*)(arg0 + 0x15C);
    if ((f & 1) != 0 || (f & 0x04000000)) return;
    fn_8018A948(actor, dt);
    SetNodeAlpha(arg0);
    u8* node;
    if (*(s32*)(*(u8**)(arg0 + 0xA0) + 0xF0) & 0x10) {
        node = *(u8**)(arg0 + 0x4);
        *(s32*)(node + 0x20) |= 0x8000;
        node = *(u8**)(arg0 + 0x4);
        *(s32*)(node + 0x20) |= 0x10000000;
        node = *(u8**)(arg0 + 0x4);
        *(s32*)(node + 0x20) |= 0x4000;
        node = *(u8**)(arg0 + 0x4);
        *(s32*)(node + 0x20) |= 0x10000000;
    } else {
        node = *(u8**)(arg0 + 0x4);
        *(s32*)(node + 0x20) |= 0x8000;
        node = *(u8**)(arg0 + 0x4);
        *(s32*)(node + 0x20) |= 0x4000;
    }
    s32 f2 = *(s32*)(arg0 + 0x15C);
    if (f2 & 0x00200000) return;
    actor = *(u8**)(arg0 + 0xA0);
    s32 af = *(s32*)(actor + 0xF0);
    if ((af & 0x10) && !(af & 8)) {
        if (!(f2 & 0x00400000)) {
            u8* n = *(u8**)(arg0 + 0x4);
            *(f32*)(n + 0x0) = *(f32*)(arg0 + 0xC8);
            *(f32*)(n + 0x4) = *(f32*)(arg0 + 0xCC);
            *(f32*)(n + 0x8) = *(f32*)(arg0 + 0xD0);
        }
        fn_8018C3D0(*(void**)(arg0 + 0xA0));
    } else {
        MoveNodeByActorVelocity(arg0, *(u8**)(arg0 + 0x4), actor);
    }
}

// 0x8006C068
// Bounces the position at +0x210 (velocity * dt-like step from +0x228/+0x234 ...) off the box
// limits at +0x244/+0x254 (x), +0x248/+0x258 (y), +0x24C/+0x25C (z), then pushes the new position into
// every attached overlay entry (ids 0x135..0x137).
void fn_8006C068(u8* arg0, f32 t) {
    f32 x = *(f32*)(arg0 + 0x210) + (*(f32*)(arg0 + 0x234) * *(f32*)(arg0 + 0x228));
    f32 y = *(f32*)(arg0 + 0x214) + (*(f32*)(arg0 + 0x238) * *(f32*)(arg0 + 0x22C));
    f32 z = *(f32*)(arg0 + 0x218) + (*(f32*)(arg0 + 0x23C) * *(f32*)(arg0 + 0x230));
    f32 w = *(f32*)(arg0 + 0x21C) + 0.0f;
    if ((x > *(f32*)(arg0 + 0x244)) || (x < *(f32*)(arg0 + 0x254))) {
        x = ClampBetween(x, *(f32*)(arg0 + 0x244), *(f32*)(arg0 + 0x254));
    }
    if ((y > *(f32*)(arg0 + 0x248)) || (y < *(f32*)(arg0 + 0x258))) {
        y = ClampBetween(y, *(f32*)(arg0 + 0x248), *(f32*)(arg0 + 0x258));
    }
    if ((z > *(f32*)(arg0 + 0x24C)) || (z < *(f32*)(arg0 + 0x25C))) {
        z = ClampBetween(z, *(f32*)(arg0 + 0x24C), *(f32*)(arg0 + 0x25C));
    }
    *(f32*)(arg0 + 0x210) = x;
    *(f32*)(arg0 + 0x214) = y;
    *(f32*)(arg0 + 0x218) = z;
    *(f32*)(arg0 + 0x21C) = w;
    // (the original also kept dead locals with the clamped velocity flipped by -1)

    for (s32 i = 0; i < fn_801DA780((*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0)); i++) {
        s32 off = 0;
        for (s32 j = 0; j < *(s32*)(*(u8**)(fn_801DA788((*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0), i) + 0x10) + 0x6C); j++) {
            u8* e = *(u8**)(*(u8**)(fn_801DA788((*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0), i) + 0x10) + off + 0x70);
            if (e != NULL) {
                u8 type = *(u8*)(e + 0xC);
                OverlayFn fn = (OverlayFn)*(void**)(lbl_80348F38 + type * 4 + 0x24);
                fn(*(void**)(e + 0x8), 0x135, F32Bits(*(f32*)(arg0 + 0x210)));
                fn = (OverlayFn)*(void**)(lbl_80348F38 + *(u8*)(e + 0xC) * 4 + 0x24);
                fn(*(void**)(e + 0x8), 0x136, F32Bits(*(f32*)(arg0 + 0x214)));
                fn = (OverlayFn)*(void**)(lbl_80348F38 + *(u8*)(e + 0xC) * 4 + 0x24);
                fn(*(void**)(e + 0x8), 0x137, F32Bits(*(f32*)(arg0 + 0x218)));
            }
            off += 8;
        }
    }
}

// 0x8006FD18
// Experiment setup: caches the lab grade, creates the experiment object for the current experiment type
// (lbl_803C4298 + 0x1C) into lbl_805FB134, remembers and notifies the "test_object_01A" actor, then
// resets the experiment object's fields.
void fn_8006FD18(void) {
    lbl_805FB124 = LabExperimentGrade((u8*)gLab);
    u8* obj;
    switch (lbl_803C4298[0x1C]) {
    case 8:  obj = (u8*)fn_801D40E0(0xA48);  if (obj != NULL) obj = fn_800B2384(obj); lbl_805FB134 = obj; break;
    case 9:  obj = (u8*)fn_801D40E0(0x1C4);  if (obj != NULL) obj = fn_80099AF4(obj); lbl_805FB134 = obj; break;
    case 12: obj = (u8*)fn_801D40E0(0x3B0);  if (obj != NULL) obj = fn_8007D4DC(obj); lbl_805FB134 = obj; break;
    case 13: obj = (u8*)fn_801D40E0(0x83C);  if (obj != NULL) obj = fn_80086BE8(obj); lbl_805FB134 = obj; break;
    case 15: obj = (u8*)fn_801D40E0(0x3D14); if (obj != NULL) obj = fn_800D02C0(obj); lbl_805FB134 = obj; break;
    case 16: obj = (u8*)fn_801D40E0(0x738);  if (obj != NULL) obj = fn_800A79A0(obj); lbl_805FB134 = obj; break;
    case 19: obj = (u8*)fn_801D40E0(0x48C);  if (obj != NULL) obj = fn_800B9AA0(obj); lbl_805FB134 = obj; break;
    case 21: obj = (u8*)fn_801D40E0(0x6E4);  if (obj != NULL) obj = fn_800940BC(obj); lbl_805FB134 = obj; break;
    case 22: obj = (u8*)fn_801D40E0(0x828);  if (obj != NULL) obj = fn_8008ED38(obj); lbl_805FB134 = obj; break;
    case 23: obj = (u8*)fn_801D40E0(0xD04);  if (obj != NULL) obj = fn_8009F8E8(obj); lbl_805FB134 = obj; break;
    case 25: obj = (u8*)fn_801D40E0(0x6E0);  if (obj != NULL) obj = fn_800756E0(obj); lbl_805FB134 = obj; break;
    case 26: obj = (u8*)fn_801D40E0(0x3C10); if (obj != NULL) obj = fn_800CE480(obj); lbl_805FB134 = obj; break;
    case 27: obj = (u8*)fn_801D40E0(0x111C); if (obj != NULL) obj = fn_80077D1C(obj); lbl_805FB134 = obj; break;
    default: break;
    }

    u8* list = *(u8**)(*(u8**)gWorldState + 0x1A4);
    s32 total = *(s32*)(list + 0x94) + *(s32*)(list + 0x98);
    for (s32 i = 0; i < total; i++) {
        u8* l = *(u8**)(*(u8**)gWorldState + 0x1A4);
        s32 cnt = *(s32*)(l + 0x94);
        u8* actor;
        u8* rec;
        if (i < cnt) {
            actor = *(u8**)(*(u8**)(l + 0x120) + i * 4);
            rec = *(u8**)(l + 0x8C) + i * 0x40;
        } else {
            s32 j = i - cnt;
            actor = *(u8**)(*(u8**)(l + 0x12C) + j * 4);
            rec = *(u8**)(l + 0x90) + j * 0x40;
        }
        if ((*(s32*)(rec + 0x1C) & 0x40) && (*(s32*)(rec + 0x20) & 1) && actor != NULL) {
            u8* sub = *(u8**)(actor + 0x9C);
            u32 hash = sub != NULL ? *(u32*)(sub + 0x8) : 0;
            if (hash == kHashTestObj) {
                lbl_805FB120 = actor;
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(VFn*)(*(u8**)actor + 0x18))(actor, 0);
                ScopedLockDtor(&lock, -1);
            }
        }
    }

    if (lbl_805FB134 != NULL) {
        u8* p = AsExperiment(lbl_805FB134);
        (*(VFn0*)(*(u8**)(p + 0x4) + 0x54))(p);
    }
    *(f32*)(AsExperiment(lbl_805FB134) + 0x74) = 0.0f;
    *(f32*)(AsExperiment(lbl_805FB134) + 0x78) = 0.0f;
    *(f32*)(AsExperiment(lbl_805FB134) + 0x7C) = 0.5f;
    *(f32*)(AsExperiment(lbl_805FB134) + 0x80) = 0.5f;
    *(f32*)(AsExperiment(lbl_805FB134) + 0x84) = 0.5f;
    fn_80070FD8(AsExperiment(lbl_805FB134));
    *(f32*)(AsExperiment(lbl_805FB134) + 0x88) = 0.0f;
}

// 0x8007A8D0
// Converts two world-space rects (a2..a5 are the corner points) into the pixel rects stored in out:
// out[0..5] from the first viewport pair, out[6]/[7] the viewport size.
void fn_8007A8D0(u8* arg0, u8* out, f32* a2, f32* a3, f32* a4, f32* a5) {
    f32 A[2];
    f32 B[2];
    f32 C[2];
    f32 D[2];
    s32 w;
    s32 h;
    f32 fw;
    f32 fh;
    fn_8007C5B4(*(u8**)arg0, (u8*)A, (u8*)B);
    fn_8007A3F4(*(u8**)(arg0 + 4), (u8*)C, (u8*)D);
    fn_8007C5E4(*(u8**)arg0, &w, &h);
    fn_8007A418(*(u8**)(arg0 + 4), (u8*)&fw, (u8*)&fh);
    s32 W = (s32)fw;
    s32 H = (s32)fh;
    *(s32*)(out + 0x18) = W;
    *(s32*)(out + 0x1C) = H;

    f32 dy = D[1] - C[1];
    f32 dx = D[0] - C[0];
    s32 v = FloorMul(W, (a2[1] - C[1]) / dy);
    *(s32*)(out + 0x0) = v;
    if (v < 0) *(s32*)(out + 0x0) = 0;
    *(s32*)(out + 0x10) = FloorMul(W, (a3[1] - C[1]) / dy) - *(s32*)(out + 0x0);
    v = FloorMul(H, (a2[0] - C[0]) / dx);
    *(s32*)(out + 0x4) = v;
    if (v < 0) *(s32*)(out + 0x4) = 0;
    *(s32*)(out + 0x14) = FloorMul(H, (a3[0] - C[0]) / dx) - *(s32*)(out + 0x4);

    f32 dy2 = B[1] - A[1];
    f32 dx2 = B[0] - A[0];
    *(s32*)(out + 0x8) = FloorMul(w, (a4[1] - A[1]) / dy2);
    s32 r5 = FloorMul(w, (a5[1] - A[1]) / dy2) - *(s32*)(out + 0x8);
    if (r5 + *(s32*)(out + 0x8) > w) r5 = w - *(s32*)(out + 0x8);
    if (r5 + *(s32*)(out + 0x0) > W) r5 = W - *(s32*)(out + 0x0);
    *(s32*)(out + 0x10) = r5;
    *(s32*)(out + 0xC) = h - FloorMul(h, (a5[0] - A[0]) / dx2);
    s32 r6 = (h - FloorMul(h, (a4[0] - A[0]) / dx2)) - *(s32*)(out + 0xC);
    if (r6 + *(s32*)(out + 0x4) > H) r6 = H - *(s32*)(out + 0x4);
    if (r6 + *(s32*)(out + 0xC) > h) r6 = h - *(s32*)(out + 0x8);
    *(s32*)(out + 0x14) = r6;
}

// 0x8008B430
// Constructor: base ctor, vtable, then the 3x3 grid of slot positions (screen layout with 5% side
// margins, 5% top and 20% bottom) and their orientation quats from nine Euler triples.
u8* fn_8008B430(u8* arg0, s32 a, void* params, s32 b) {
    fn_800690D4(arg0, a, params, b);
    *(void**)arg0 = lbl_8035B8F0;
    *(s32*)(arg0 + 0x264) = -1;
    *(u8*)(arg0 + 0x268) = 1;
    *(f32*)(arg0 + 0x26C) = 0.0f;
    *(f32*)(arg0 + 0x270) = 0.0f;
    *(f32*)(arg0 + 0x274) = 0.0f;
    *(f32*)(arg0 + 0x278) = 1.0f;
    *(f32*)(arg0 + 0x27C) = 0.0f;
    *(f32*)(arg0 + 0x280) = 0.0f;
    *(f32*)(arg0 + 0x284) = 0.0f;
    *(f32*)(arg0 + 0x288) = 0.0f;
    *(f32*)(arg0 + 0x28C) = 1.0f;
    *(u8*)(arg0 + 0x290) = 0;
    *(s32*)(arg0 + 0x35C) = 0;
    *(s32*)(arg0 + 0x360) = -1;
    *(s32*)(arg0 + 0x364) = 0;
    *(s32*)(arg0 + 0x368) = 0;
    *(s32*)(arg0 + 0x36C) = -1;
    *(s32*)(arg0 + 0x370) = 0;
    *(s32*)(arg0 + 0x374) = 0;
    *(s32*)(arg0 + 0x378) = 0;
    *(s32*)(arg0 + 0x4CC) = 0;

    f32 W = (f32)gScreenWidth;
    f32 H = (f32)gScreenHeight;
    f32 L = (5.0f * W) / 100.0f;
    f32 R = (5.0f * W) / 100.0f;
    f32 T = (5.0f * H) / 100.0f;
    f32 B = (20.0f * H) / 100.0f;
    f32 cw = ((W - L) - R) / 3.0f;
    f32 ch = ((H - T) - B) / 3.0f;
    f32 halfw = cw * 0.5f;
    f32 halfh = ch * 0.5f;
    f32 col0 = L + halfw;
    f32 col1 = halfw + (L + cw);
    f32 col2 = halfw + (L + (2.0f * cw));
    f32 row0 = T + halfh;
    f32 row1 = halfh + (T + ch);
    f32 row2 = halfh + (T + (2.0f * ch));
    // slot i: quat at +0x37C + i * 0x18, screen position (x, y) at +0x38C + i * 0x18
    *(f32*)(arg0 + 0x38C) = col0;  *(f32*)(arg0 + 0x390) = row0;
    *(f32*)(arg0 + 0x3A4) = col1;  *(f32*)(arg0 + 0x3A8) = row0;
    *(f32*)(arg0 + 0x3BC) = col2;  *(f32*)(arg0 + 0x3C0) = row0;
    *(f32*)(arg0 + 0x3D4) = col0;  *(f32*)(arg0 + 0x3D8) = row1;
    *(f32*)(arg0 + 0x3EC) = col1;  *(f32*)(arg0 + 0x3F0) = row1;
    *(f32*)(arg0 + 0x404) = col2;  *(f32*)(arg0 + 0x408) = row1;
    *(f32*)(arg0 + 0x41C) = col0;  *(f32*)(arg0 + 0x420) = row2;
    *(f32*)(arg0 + 0x434) = col1;  *(f32*)(arg0 + 0x438) = row2;
    *(f32*)(arg0 + 0x44C) = col2;  *(f32*)(arg0 + 0x450) = row2;

    // Euler angle triples (x, y, z) at +0x454 + i * 0xC
    static const f32 kEuler[9][3] = {
        { -0.33333334f, 0.0f, 0.7777778f },
        { -0.22222222f, 0.0f, 0.0f },
        { -0.33333334f, 0.0f, -0.7777778f },
        { -0.8888889f, 0.0f, 0.8888889f },
        { -0.8888889f, 0.0f, 0.0f },
        { -0.8888889f, 0.0f, -0.8888889f },
        { -0.6666667f, 0.0f, 0.5f },
        { -0.6666667f, 0.0f, 0.0f },
        { -0.6666667f, 0.0f, -0.5f },
    };
    for (s32 i = 0; i < 9; i++) {
        *(f32*)(arg0 + 0x454 + i * 0xC + 0x0) = kEuler[i][0];
        *(f32*)(arg0 + 0x454 + i * 0xC + 0x4) = kEuler[i][1];
        *(f32*)(arg0 + 0x454 + i * 0xC + 0x8) = kEuler[i][2];
    }
    for (s32 i = 0; i < 9; i++) {
        EulerToQuat((Quat*)(arg0 + 0x37C + i * 0x18), (const Vec3*)(arg0 + 0x454 + i * 0xC));
    }
    return arg0;
}

// 0x8008CE44
// Starts the experiment actor in state 3: stores kind, start scale (f1, grown by a per-kind table
// entry times the int at (+0x284)->0x35C percent), the start position, a random side sign and the
// 0.85 fade, then runs the virtual update (slot 0x14) under a scoped lock.
s32 fn_8008CE44(u8* arg0, const f32* pos, s32 kind, f32 a, f32 b) {
    *(s32*)(arg0 + 0x264) = 3;
    *(s32*)(arg0 + 0x268) = kind;
    *(f32*)(arg0 + 0x29C) = a;
    *(f32*)(arg0 + 0x2A0) = 0.0f;
    *(f32*)(arg0 + 0x2A4) = a;
    s32 idx = 4;
    if (*(s32*)(arg0 + 0x270) == 0) {
        switch (kind) {
        case 2: idx = 0; break;
        case 3: idx = 1; break;
        case 4: idx = 2; break;
        case 0: idx = 3; break;
        default: idx = 0; break;
        }
    }
    f32 s = *(f32*)(arg0 + 0x2A4);
    *(f32*)(arg0 + 0x2A4) =
        s + ((f32)*(s32*)(*(u8**)(arg0 + 0x284) + 0x35C) * ((s * lbl_80346884[idx]) / 100.0f));
    *(f32*)(arg0 + 0x2B8) = pos[0];
    *(f32*)(arg0 + 0x2BC) = pos[1];
    *(f32*)(arg0 + 0x2C0) = pos[2];
    *(f32*)(arg0 + 0x2C4) = pos[0];
    *(f32*)(arg0 + 0x2C8) = pos[1];
    *(f32*)(arg0 + 0x2CC) = pos[2];
    if (*(s32*)(arg0 + 0x26C) == 1) {
        *(f32*)(arg0 + 0x2A8) = b;
        if (RandomU32() & 1) {
            *(f32*)(arg0 + 0x2AC) = 1.0f;
        } else {
            *(f32*)(arg0 + 0x2AC) = -1.0f;
        }
        RandomU32();  // result unused in the original
        *(f32*)(arg0 + 0x2B0) = 0.0f;
    } else {
        *(f32*)(arg0 + 0x2A8) = 0.0f;
        *(f32*)(arg0 + 0x2AC) = 1.0f;
        *(f32*)(arg0 + 0x2B0) = 0.0f;
    }
    *(f32*)(arg0 + 0x2B4) = *(f32*)(arg0 + 0x2A8);
    idx = 4;
    if (*(s32*)(arg0 + 0x270) == 0) {
        switch (*(s32*)(arg0 + 0x268)) {
        case 2: idx = 0; break;
        case 3: idx = 1; break;
        case 4: idx = 2; break;
        case 0: idx = 3; break;
        default: idx = 0; break;
        }
    }
    f32 s2 = *(f32*)(arg0 + 0x2B4);
    *(f32*)(arg0 + 0x2B4) =
        s2 + ((f32)*(s32*)(*(u8**)(arg0 + 0x284) + 0x35C) * ((s2 * lbl_80346870[idx]) / 100.0f));
    u8* node = *(u8**)(arg0 + 0x4);
    *(f32*)(node + 0x0) = *(f32*)(arg0 + 0x2B8);
    *(f32*)(node + 0x4) = *(f32*)(arg0 + 0x2BC);
    *(f32*)(node + 0x8) = *(f32*)(arg0 + 0x2C0);
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x8000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x10000000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x4000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x10000000;
    f32 fade = Clamp01(0.85f);
    *(f32*)(arg0 + 0x2D4) = fade;
    *(f32*)(arg0 + 0x8) = fade;
    *(f32*)(arg0 + 0xC) = fade;
    *(f32*)(arg0 + 0x10) = fade;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(VFn*)(*(u8**)arg0 + 0x14))(arg0, 1);
    ScopedLockDtor(&lock, -1);
    return 1;
}

// 0x8008DF44
// Per-frame orbit update: in state 5 the fade (+0x2D4) drops by 0.1 per call (to 0.5, then state 6) and
// the stored angle/radius (+0x28C/+0x294) take their end values (+0x290/+0x298); then the node eases
// 20% of the way toward the point at distance radius along the rotated axis around the target.
void fn_8008DF44(u8* arg0, f32 dt) {
    s32 state = *(s32*)(arg0 + 0x264);
    f32 angle = *(f32*)(arg0 + 0x294);
    f32 radius = *(f32*)(arg0 + 0x28C);
    if (state == 5) {
        angle = *(f32*)(arg0 + 0x298);
        f32 fade = *(f32*)(arg0 + 0x2D4) - 0.1f;
        radius = *(f32*)(arg0 + 0x290);
        *(f32*)(arg0 + 0x294) = angle;
        *(f32*)(arg0 + 0x28C) = radius;
        *(f32*)(arg0 + 0x2D4) = fade;
        if (fade <= 0.5f) {
            *(f32*)(arg0 + 0x2D4) = 0.5f;
            *(s32*)(arg0 + 0x264) = 6;
        }
        f32 c = Clamp01(*(f32*)(arg0 + 0x2D4));
        *(f32*)(arg0 + 0x2D4) = c;
        *(f32*)(arg0 + 0x8) = c;
        *(f32*)(arg0 + 0xC) = c;
        *(f32*)(arg0 + 0x10) = c;
    } else if (state == 6) {
        angle = *(f32*)(arg0 + 0x298);
        radius = *(f32*)(arg0 + 0x290);
        *(f32*)(arg0 + 0x294) = angle;
        *(f32*)(arg0 + 0x28C) = radius;
    }
    // (the original also normalised the node position into dead locals here)

    u8* target = *(u8**)(arg0 + 0x280);
    Quat q;
    fn_8008C104(&q, target);
    Vec3 axis0 = QuatGetAxisY(&q);
    Vec3 axis1;
    fn_801455E0(&axis1.x, (u8*)&q);
    f32 rot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    f32 newAngle = angle + (1.0f * dt);
    fn_801458B0(rot, &axis0.x, &axis1.x);
    fn_80146180(rot, &axis1, newAngle);
    Vec3 ax = QuatGetAxisY((const Quat*)rot);
    Vec3 off;
    off.x = ax.x * radius;
    off.y = ax.y * radius;
    off.z = ax.z * radius;
    Vec3 tp;
    fn_8008C128(&tp, *(u8**)(arg0 + 0x280));
    u8* node = *(u8**)(arg0 + 0x4);
    f32 nx = *(f32*)(node + 0x0);
    f32 ny = *(f32*)(node + 0x4);
    f32 nz = *(f32*)(node + 0x8);
    f32 gx = off.x + tp.x;
    f32 dx = gx - nx;
    f32 gy = off.y + tp.y;
    f32 gz = off.z + tp.z;
    f32 dy = gy - ny;
    f32 sx = dx * 0.2f;
    f32 nxn = nx + sx;
    f32 dz = gz - nz;
    f32 sy = dy * 0.2f;
    f32 sz = dz * 0.2f;
    *(f32*)(arg0 + 0x30C) = nxn - nx;
    *(f32*)(node + 0x0) = nxn;
    *(f32*)(node + 0x4) = ny + sy;
    *(f32*)(node + 0x8) = nz + sz;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x8000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x10000000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x4000;
    node = *(u8**)(arg0 + 0x4);
    *(s32*)(node + 0x20) |= 0x10000000;
}

// 0x800968C4
// Experiment slot meter update: counts occupied slots, then drives the meter at +0x35C from the table
// entry for the current grade (rising/falling plays the matching tick), advances the selected slot
// (+0x1D8) and starts/stops the plate rattle sound when the occupied count changes between zero/non-zero.
void fn_800968C4(u8* arg0) {
    s32 before = CountActiveSlots(arg0);
    if (*(u8**)(arg0 + 0x1D8) == NULL) return;

    *(f32*)(arg0 + 0x374) = -1.0f;
    *(f32*)(*(u8**)(arg0 + 0x90) + 0x274) = 0.08f;
    RaySceneTest(*(void**)(arg0 + 0x90));
    f32 f30 = lbl_80346FA0[RaySceneTest(NULL)];
    u8* sel = *(u8**)(arg0 + 0x1D8);
    if (*(u8*)(sel + 0x26C) > 0) {
        f32 d = lbl_80347040[RaySceneTest(NULL)];
        if (d > 0.0f) {
            fn_800712B8();
        } else if (d < 0.0f) {
            fn_800712B4();
        }
        *(f32*)(arg0 + 0x35C) = Clamp01(*(f32*)(arg0 + 0x35C) + d);
        sel = *(u8**)(arg0 + 0x1D8);
        fn_800980F8(sel, *(u8*)(sel + 0x26C) - 1);
        sel = *(u8**)(arg0 + 0x1D8);
        if (*(u8*)(sel + 0x26C) == 0) {
            f32 d2 = lbl_80347060[RaySceneTest(NULL)];
            if (d2 > 0.0f) {
                fn_800712B8();
            } else if (d2 < 0.0f) {
                fn_800712B4();
            }
            *(f32*)(arg0 + 0x35C) = Clamp01(*(f32*)(arg0 + 0x35C) + d2);
            if (f30 >= 0.0f && *(u8*)(arg0 + 0x370) == 0) {
                *(f32*)(arg0 + 0x36C) = f30;
                *(u8*)(arg0 + 0x370) = 1;
            }
        }
    } else {
        f32 d = lbl_80347050[RaySceneTest(NULL)];
        if (d > 0.0f) {
            fn_800712B8();
        } else if (d < 0.0f) {
            fn_800712B4();
        }
        *(f32*)(arg0 + 0x35C) = Clamp01(*(f32*)(arg0 + 0x35C) + d);
        sel = *(u8**)(arg0 + 0x1D8);
        if (*(u8*)(sel + 0x26C) == 0) fn_800980F8(sel, 1);
    }

    if (before == 0) {
        if (CountActiveSlots(arg0) > 0) {
            PlayExperimentSound(NULL, kHashRattle, 1);
            return;
        }
    }
    if (before > 0) {
        if (CountActiveSlots(arg0) == 0) StopExperimentSound(NULL, kHashRattle);
    }
}

}  // extern "C"
