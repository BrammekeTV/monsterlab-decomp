// Camera follow state (the object at camera +0x68, vtable lbl_8035FF78): reset and the
// per-frame position / direction.
// Original: 0x800DBB64 (auto_03_800DB9A4_text.s), 0x800E054C, 0x800E084C, 0x800E2728,
// 0x800E2B8C, 0x800E2C50 (auto_03_800E02A8_text.s)
//
// Fields used here: +0xC position, +0x18 position of the last frame, +0x24 target point,
// +0x30 target point of the last frame, +0x48 height above the followed object, +0x4C / +0x5C
// yaw, +0x64 / +0x68 pitch, +0x6C wanted distance, +0x70 distance, +0x98 unit vector from the
// position to the target, +0xB8 up vector, +0xC4..+0xD0 four floats cleared every frame
// (+0xC4 a distance and +0xC8 its weight), +0xD4 mode.
#include "types.h"
#include "engine/math.h"

typedef f32 Scalar;

extern "C" {
void operator_delete(void* p);                                       // 0x801D426C
void fn_800DE28C(u8* out, u8* cam, f32 height);                      // 0x800DE28C
f32  fn_800DE2AC(u8* obj);                                           // 0x800DE2AC
void fn_800DA8F0(u8* query, u8* result);                             // 0x800DA8F0
void fn_800E15AC(u8* self, f32 dt);                                  // 0x800E15AC
void fn_800E1FE0(u8* self, f32* pos, f32 dt);                        // 0x800E1FE0
f32  fn_801F5E94(f32 cur, f32 target, f32 lo, f32 hi, f32 range);    // 0x801F5E94
f64  fn_8033F1F8(f64 x);                                             // 0x8033F1F8 (MSL asin)
f32  fn_8033F1FC(f32 y, f32 x);                                      // 0x8033F1FC (MSL atan2)
void QuatGetAxisXY(Quat* q, Vec3* out);                              // 0x801453BC
void Vec3Copy(Vec3* out, const Vec3* v);                             // 0x80052640
void Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s);       // 0x80053224
void ScalarSet(Scalar* out, f32 v);                                  // 0x80053250
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);               // 0x80053370
void Vec3SubFrom(Vec3* v, const Vec3* d);                            // 0x8005461C
void Vec3NormalizeTo(Vec3* out, const Vec3* v);                      // 0x80055D9C
extern u8 lbl_803C4298[];          // 0x803C4298 (+0x4: object, +0x8: camera)
}

typedef void (*ObjSetFn)(u8* obj, s32 v);
typedef u32  (*ObjFlagsFn)(u8* obj);
typedef BOOL (*ObjBoolFn)(u8* obj);

// The query and result blocks of the camera collision (fn_800DA8F0), built on the stack.
struct CamQuery {
    Vec3 from;      // 0x00
    Vec3 to;        // 0x0C
    f32  f18;       // 0x18
    f32  f1C;       // 0x1C
    u8   b20;       // 0x20
    s32  w24;       // 0x24
    s32  w28;       // 0x28
    s32  w2C;       // 0x2C
};
struct CamResult {
    u8   hit;       // 0x00
    Vec3 v04;       // 0x04
    Vec3 v10;       // 0x10
    Vec3 pos;       // 0x1C
};

static inline void CamResultClear(CamResult* r) {
    r->hit = 0;
    r->pos.x = 0.0f; r->pos.y = 0.0f; r->pos.z = 0.0f;
    r->v10.x = 0.0f; r->v10.y = 0.0f; r->v10.z = 0.0f;
    r->v04.x = 0.0f; r->v04.y = 0.0f; r->v04.z = 0.0f;
}

// 0x800DBB64
// Destructor (slot 0x8): frees the object when asked to.
extern "C" u8* fn_800DBB64(u8* self, s32 del) {
    if (self != NULL && del > 0) {
        operator_delete(self);
    }
    return self;
}

// 0x800E054C
// Moves the height above the followed object (+0x48) towards its wanted value: between 1.3
// and 0.6 (0.3 in mode 1) by the object's ground value (fn_800DE2AC) over 3, or 1.3 while
// bit 8 of slot 0x7C of the object at lbl_803C4298+4 is set.
extern "C" void fn_800E054C(u8* self, f32 dt) {
    f32 a = 1.3f;
    f32 b = 0.6f;
    if (*(s32*)(self + 0xD4) == 1) {
        b = 0.3f;
    }
    u8* o = *(u8**)(lbl_803C4298 + 0x4);
    if ((*(ObjFlagsFn*)(*(u8**)o + 0x7C))(o) & 0x8) {
        a = 1.3f;
        b = a;
    }
    f32 g = fn_800DE2AC(*(u8**)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0));
    f32 v = a + (g / 3.0f) * (b - a);
    f32 want;
    if (b < a) {
        if (v < b) want = b;
        else if (v <= a) want = v;
        else want = a;
    } else {
        if (v < a) want = a;
        else if (v <= b) want = v;
        else want = b;
    }
    f32 hi = 2.0f * dt;
    *(f32*)(self + 0x48) = fn_801F5E94(*(f32*)(self + 0x48), want, 0.05f * hi, hi, 0.1f);
}

// 0x800E084C
// Keeps the camera out of the scenery. A first collision query at the target point (+0x24)
// gives a free point near it; in mode 0, when that point lies ahead (along +0x98), the
// position (+0xC) is pushed that way by twice the offset times the speed of the followed
// object (vector at +0x770). A second query from the free point to the position gives the
// final position; the target point becomes that position plus 5 along the direction.
extern "C" void fn_800E084C(u8* self) {
    CamQuery q;
    CamResult r;
    Vec3 dir = { *(f32*)(self + 0x98), *(f32*)(self + 0x9C), *(f32*)(self + 0xA0) };
    q.to.x = 0.0f; q.to.y = 0.0f; q.to.z = 0.0f;
    q.from.x = 0.0f; q.from.y = 0.0f; q.from.z = 0.0f;
    q.b20 = 1;
    q.w24 = 4;
    q.w28 = 0;
    q.w2C = 2;
    q.f18 = 1.0f;
    q.f1C = 1.5707964f;
    CamResultClear(&r);
    q.from.x = *(f32*)(self + 0x24);
    q.from.y = *(f32*)(self + 0x28);
    q.from.z = *(f32*)(self + 0x2C);
    q.to.x = *(f32*)(self + 0x24);
    q.to.y = *(f32*)(self + 0x28);
    q.to.z = *(f32*)(self + 0x2C);
    fn_800DA8F0((u8*)&q, (u8*)&r);
    Vec3 freePoint = r.pos;
    if (*(s32*)(self + 0xD4) == 0) {
        f32 dx = freePoint.x - *(f32*)(self + 0x24);
        f32 dy = freePoint.y - *(f32*)(self + 0x28);
        f32 dz = freePoint.z - *(f32*)(self + 0x2C);
        if (dz * dir.z + (dx * dir.x + dy * dir.y) > 0.0f) {
            u8* obj = *(u8**)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0);
            f32 vx = *(f32*)(obj + 0x770), vy = *(f32*)(obj + 0x774), vz = *(f32*)(obj + 0x778);
            f32 len2 = vz * vz + (vx * vx + vy * vy);
            f32 speed = 0.0f;
            if (!(len2 <= 0.0f)) speed = sqrtf(len2);
            f32 px = dx * speed, py = dy * speed, pz = dz * speed;
            *(f32*)(self + 0xC)  = *(f32*)(self + 0xC)  + px * 2.0f;
            *(f32*)(self + 0x10) = *(f32*)(self + 0x10) + py * 2.0f;
            *(f32*)(self + 0x14) = *(f32*)(self + 0x14) + pz * 2.0f;
        }
    }
    CamResultClear(&r);
    q.from = freePoint;
    q.to.x = *(f32*)(self + 0xC);
    q.to.y = *(f32*)(self + 0x10);
    q.to.z = *(f32*)(self + 0x14);
    q.b20 = 1;
    q.f1C = 1.3962634f;
    fn_800DA8F0((u8*)&q, (u8*)&r);
    *(f32*)(self + 0xC)  = r.pos.x;
    *(f32*)(self + 0x10) = r.pos.y;
    *(f32*)(self + 0x14) = r.pos.z;
    *(f32*)(self + 0x24) = r.pos.x + dir.x * 5.0f;
    *(f32*)(self + 0x28) = r.pos.y + dir.y * 5.0f;
    *(f32*)(self + 0x2C) = r.pos.z + dir.z * 5.0f;
    *(f32*)(self + 0x98) = dir.x;
    *(f32*)(self + 0x9C) = dir.y;
    *(f32*)(self + 0xA0) = dir.z;
}

// 0x800E2728
// Reset (slot 0x10). Calls slot 0x70 of the object at lbl_803C4298+4 with 1, puts the target
// point 1.3 above the followed object and clears the state. The yaw comes from the X/Y axis
// of the camera's transform when `fromCamera` is set, else from the vector (-0, 3, 0); the
// pitch is 0.5236 plus 0.6 times the tilt of the up vector along that yaw, the distance stays
// what it was (+0x70). The position is the target minus that offset, then corrected by the
// collision (fn_800E084C).
extern "C" void fn_800E2728(u8* self, s32 fromCamera) {
    u8* c = lbl_803C4298;
    u8* o = *(u8**)(c + 0x4);
    (*(ObjSetFn*)(*(u8**)o + 0x70))(o, 1);
    Vec3 p;
    fn_800DE28C((u8*)&p, *(u8**)(c + 0x8), 1.3f);
    *(f32*)(self + 0x24) = p.x;
    *(f32*)(self + 0x28) = p.y;
    *(f32*)(self + 0x2C) = p.z;
    *(f32*)(self + 0x30) = p.x;
    *(f32*)(self + 0x34) = p.y;
    *(f32*)(self + 0x38) = p.z;
    *(f32*)(self + 0x6C) = 12.5f;
    *(f32*)(self + 0x70) = 12.5f;
    *(f32*)(self + 0x74) = 0.0f;
    *(u8*)(self + 0x78)  = 0;
    *(s32*)(self + 0xB0) = 0;
    *(f32*)(self + 0xB4) = 0.0f;
    *(u8*)(self + 0x94)  = 0;
    *(u8*)(self + 0x60)  = 0;
    *(u8*)(self + 0x95)  = 0;
    *(f32*)(self + 0xB8) = 0.0f;
    *(f32*)(self + 0xBC) = 0.0f;
    *(f32*)(self + 0xC0) = 1.0f;
    *(f32*)(self + 0x50) = 0.0f;
    *(s32*)(self + 0x54) = 0;
    *(f32*)(self + 0x3C) = 0.0f;
    *(f32*)(self + 0x40) = 0.0f;
    *(f32*)(self + 0x44) = 0.0f;

    Vec3 dir;
    if (fromCamera != 0) {
        QuatGetAxisXY((Quat*)(*(u8**)(*(u8**)(c + 0x8) + 0x4) + 0xC), &dir);
    } else {
        dir.x = -0.0f;
        dir.y = 3.0f;
        dir.z = 0.0f;
    }
    f32 off = (dir.z * dir.z + (dir.x * dir.x + dir.y * dir.y)) - 1.0f;
    if (!(off > -0.0001f && off < 0.0001f)) {
        dir = Vec3Normalized(dir);
    }
    f32 yaw = fn_8033F1FC(dir.y, dir.x);
    *(f32*)(self + 0x68) = 0.5235988f;
    *(f32*)(self + 0x4C) = yaw;
    *(f32*)(self + 0x5C) = yaw;
    *(f32*)(self + 0x64) = 0.5235988f;
    *(f32*)(self + 0x6C) = 12.5f;
    f32 s = sinf(yaw);
    f32 co = cosf(*(f32*)(self + 0x4C));
    f32 tilt = (f32)fn_8033F1F8(0.0f * *(f32*)(self + 0xC0) + (co * *(f32*)(self + 0xB8) + s * *(f32*)(self + 0xBC)));
    f32 pitch = *(f32*)(self + 0x68) + 0.6f * tilt;
    f32 cp = cosf(pitch);
    f32 vx = co * cp;
    f32 vy = s * cp;
    f32 sp = sinf(pitch);
    f32 dist = *(f32*)(self + 0x70);
    Vec3 v = { vx * dist, vy * dist, -sp * dist };
    Vec3 n = Vec3Normalized(v);
    *(f32*)(self + 0xC)  = *(f32*)(self + 0x24) - v.x;
    *(f32*)(self + 0x10) = *(f32*)(self + 0x28) - v.y;
    *(f32*)(self + 0x14) = *(f32*)(self + 0x2C) - v.z;
    *(f32*)(self + 0x98) = n.x;
    *(f32*)(self + 0x9C) = n.y;
    *(f32*)(self + 0xA0) = n.z;
    fn_800E084C(self);
    *(f32*)(self + 0x18) = *(f32*)(self + 0xC);
    *(f32*)(self + 0x1C) = *(f32*)(self + 0x10);
    *(f32*)(self + 0x20) = *(f32*)(self + 0x14);
    *(f32*)(self + 0x30) = *(f32*)(self + 0x24);
    *(f32*)(self + 0x34) = *(f32*)(self + 0x28);
    *(f32*)(self + 0x38) = *(f32*)(self + 0x2C);
    *(u8*)(self + 0x95) = 1;
}

// 0x800E2B8C
// Wanted distance (+0x6C): 3.2 in mode 1, else 12.5 (the original adds -0.3 times 0 / 5),
// mixed with the distance at +0xC4 by its weight at +0xC8 (1 or more: only +0xC4). The
// distance (+0x70) moves towards it.
extern "C" void fn_800E2B8C(u8* self, f32 dt) {
    *(f32*)(self + 0x6C) = 12.5f;
    if (*(s32*)(self + 0xD4) == 1) {
        *(f32*)(self + 0x6C) = 3.2f;
    } else {
        *(f32*)(self + 0x6C) = 12.5f + -0.3000002f * (0.0f / 5.0f);
    }
    f32 w = *(f32*)(self + 0xC8);
    f32 k;
    if (w >= 1.0f) k = 0.0f;
    else k = 1.0f - w;
    f32 want = k * *(f32*)(self + 0x6C) + (1.0f - k) * *(f32*)(self + 0xC4);
    f32 hi = 5.0f * dt;
    *(f32*)(self + 0x6C) = want;
    *(f32*)(self + 0x70) = fn_801F5E94(*(f32*)(self + 0x70), want, 0.05f * hi, hi, 0.1f);
}

// 0x800E2C50
// Slot 0xC: the camera position and view direction of this frame. Updates the height and the
// distance, puts the target point above the followed object, steps the angles (fn_800E15AC)
// and, unless the followed object's slot 0x4C is set with the value 1 at its +0x78C or the
// frame time is (almost) zero, the position (fn_800E1FE0); else the position of the last frame
// is kept. `dir` is the unit vector from the position to the target; `pos` is given 0.8 back
// along it. Always returns 1.
//
// The original also builds an unused collision query and result on its stack here
// (fn_800E0BCC, fn_800E0C28); they are left out.
extern "C" s32 fn_800E2C50(u8* self, f32* pos, f32* dir, f32 dt) {
    fn_800E054C(self, dt);
    fn_800E2B8C(self, dt);
    u8* c = lbl_803C4298;
    Vec3 p;
    fn_800DE28C((u8*)&p, *(u8**)(c + 0x8), *(f32*)(self + 0x48));
    Vec3Copy((Vec3*)(self + 0x24), &p);
    fn_800E15AC(self, dt);
    u8* obj = *(u8**)(*(u8**)(c + 0x8) + 0x2C0);
    BOOL move = FALSE;
    if (obj != NULL) {
        if (!(*(ObjBoolFn*)(*(u8**)obj + 0x4C))(obj) || *(s32*)(obj + 0x78C) != 1) {
            move = TRUE;
        }
    }
    if (move && dt > 0.0001f) {
        fn_800E1FE0(self, pos, dt);
    } else {
        Vec3Copy((Vec3*)pos, (Vec3*)(self + 0x18));
    }
    Vec3Copy((Vec3*)(self + 0x30), (Vec3*)(self + 0x24));
    Vec3 d;
    Vec3Sub(&d, (Vec3*)(self + 0x30), (Vec3*)pos);
    Vec3Copy((Vec3*)dir, &d);
    Vec3 n;
    Vec3NormalizeTo(&n, (Vec3*)dir);
    Vec3Copy((Vec3*)dir, &n);
    Vec3Copy((Vec3*)(self + 0x18), (Vec3*)pos);
    Scalar back;
    ScalarSet(&back, 0.8f);
    Vec3 t;
    Vec3MulScalar(&t, (Vec3*)dir, &back);
    Vec3SubFrom((Vec3*)pos, &t);
    *(f32*)(self + 0xC4) = 0.0f;
    *(f32*)(self + 0xC8) = 0.0f;
    *(f32*)(self + 0xCC) = 0.0f;
    *(f32*)(self + 0xD0) = 0.0f;
    return 1;
}
