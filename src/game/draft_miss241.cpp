// Missing functions batch 241: projectile/camera-follow object setup (fn_800AB028 ... fn_800B07F0),
// slot registration, controller lookup (fn_800A6F00) and the path database helpers (fn_800D42F4, fn_800D4DE8).
#include <math.h>
#include "types.h"
#include "engine/math.h"

typedef u8* (*GetFn)(u8* obj);   // vtable +0x10 of a world object

extern "C" {
void  EulerToQuat(Quat* out, const Vec3* euler);                 // 0x80145B00
void  fn_800ABA44(u8* obj);                                      // 0x800ABA44
void  fn_800ADA70(u8* obj);                                      // 0x800ADA70
void  fn_801458B0(f32* quat, f32* a, f32* b);                    // 0x801458B0
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);      // 0x80146180 (rotate quat about axis)
u8*   fn_801483BC(s32 idx);                                      // 0x801483BC
s32   fn_8018BDE0(u8* p, s32 key);                               // 0x8018BDE0
u8*   fn_8018C39C(u8* p, s32 idx);                               // 0x8018C39C
s32   fn_801B5338(u8* obj, u8* target, u32 hash, f32* vec, u8 flag);  // 0x801B5338
u32   fn_801EC880(u8* p, u32 hash);                              // 0x801EC880
extern u8 gWorldState[];       // 0x803E6D00
extern f32 lbl_80346B68[];     // 0x80346B68 (thresholds)
extern f32 lbl_80346B8C[];     // 0x80346B8C (values matching the thresholds)
extern u8 lbl_803C4298[];      // 0x803C4298 (+0x38 = controller index)
}

// Sets the four "position/orientation changed" flag bits on the object's scene record.
#define MARK_DIRTY(a)                                           \
    do {                                                        \
        *(u32*)(*(u8**)((a) + 0x4) + 0x20) |= 0x8000;           \
        *(u32*)(*(u8**)((a) + 0x4) + 0x20) |= 0x10000000;       \
        *(u32*)(*(u8**)((a) + 0x4) + 0x20) |= 0x4000;           \
        *(u32*)(*(u8**)((a) + 0x4) + 0x20) |= 0x10000000;       \
    } while (0)

// Path database entry accessor (0x88-byte entries at +0x2DCDC, count at +0x36454); an index outside
// [0, count] falls back to entry 0.
static u8* DbEntry(u8* db, s32 i) {
    s32 n = *(s32*)(db + 0x36454);
    s32 c;
    if (i < 0) {
        c = 0;
    } else {
        c = i;
        if (i > n) c = n;
    }
    if (i != c) return db + 0x2DCDC;
    return db + 0x2DCDC + i * 0x88;
}

extern "C" {

// 0x800D8100
// Copies the world's value at +0x1F4 into the record at +0x44.
void fn_800D8100(u8* rec) {
    *(s32*)(rec + 0x44) = *(s32*)(*(u8**)gWorldState + 0x1F4);
}

// 0x800ACDE0
// Stores the key looked up in the table at +0x94 plus three parameters.
void fn_800ACDE0(u8* obj, u32 arg1, u32 arg2, u32 arg3, s32 arg4) {
    *(s32*)(obj + 0x264) = fn_801EC880(obj + 0x94, arg3);
    *(u32*)(obj + 0x268) = arg1;
    *(u32*)(obj + 0x26C) = arg2;
    *(s32*)(obj + 0x270) = arg4;
}

// 0x800AB028
// Hides the object: runs fn_800ABA44, parks its scene record at (-0, -0, -1) and flags it dirty.
void fn_800AB028(u8* obj) {
    fn_800ABA44(obj);
    u8* rec = *(u8**)(obj + 0x4);
    *(f32*)(rec + 0x0) = -0.0f;
    *(f32*)(rec + 0x4) = -0.0f;
    *(f32*)(rec + 0x8) = -1.0f;
    MARK_DIRTY(obj);
}

// 0x800AD9C4
// Initialises the follower: target at +0x26C, unit scale at +0x270..0x278, and raycasts to the target;
// on a hit (result 1) clears bit 0 of the target's scene flags. Then runs fn_800ADA70.
void fn_800AD9C4(u8* obj, u8* target) {
    f32 vec[6];
    *(s32*)(obj + 0x264) = 0;
    *(u32*)(obj + 0x268) = 0xC2FDDCDCu;
    *(s32*)(obj + 0x330) = 0;
    *(u8**)(obj + 0x26C) = target;
    *(f32*)(obj + 0x270) = 1.0f;
    *(f32*)(obj + 0x274) = 1.0f;
    *(f32*)(obj + 0x278) = 1.0f;
    vec[0] = 0.0f;
    vec[1] = 0.0f;
    vec[2] = 0.0f;
    vec[3] = 1.0f;
    vec[4] = 1.0f;
    vec[5] = 1.0f;
    if (fn_801B5338(obj, *(u8**)(obj + 0x26C), 0x5B34ECA4u, vec, 0) == 1) {
        *(u32*)(*(u8**)(*(u8**)(obj + 0x26C) + 0x4) + 0x20) &= 0xFFFFFFFEu;
    }
    fn_800ADA70(obj);
}

// 0x800ABFA8
// Sets the owner at +0x2A0, resets state, parks the scene record at (-0, -0, -1) and flags it dirty,
// then sets the four floats at +0x38..0x44 of the sub-record at +0x90 to 1.0.
void fn_800ABFA8(u8* obj, u8* owner) {
    *(u8**)(obj + 0x2A0) = owner;
    u8* rec = *(u8**)(obj + 0x4);
    *(s32*)(obj + 0x264) = 0;
    *(s32*)(obj + 0x268) = -1;
    *(f32*)(rec + 0x0) = -0.0f;
    *(f32*)(rec + 0x4) = -0.0f;
    *(f32*)(rec + 0x8) = -1.0f;
    MARK_DIRTY(obj);
    u8* sub = *(u8**)(obj + 0x90);
    *(f32*)(sub + 0x38) = 1.0f;
    *(f32*)(sub + 0x3C) = 1.0f;
    *(f32*)(sub + 0x40) = 1.0f;
    *(f32*)(sub + 0x44) = 1.0f;
}

// 0x800AFA54
// Copies the source's scene position and rotation into the destination's record, flags it dirty, and
// registers dst in the first free slot of the 5-entry list at +0x2AC. Returns 1 if a slot was free.
s32 fn_800AFA54(u8* src, u8* dst) {
    u8* s = *(u8**)(src + 0x4);
    u8* d = *(u8**)(dst + 0x4);
    *(f32*)(d + 0x0) = *(f32*)(s + 0x0);
    *(f32*)(d + 0x4) = *(f32*)(s + 0x4);
    *(f32*)(d + 0x8) = *(f32*)(s + 0x8);
    MARK_DIRTY(dst);
    s = *(u8**)(src + 0x4);
    d = *(u8**)(dst + 0x4);
    *(f32*)(d + 0xC) = *(f32*)(s + 0xC);
    *(f32*)(d + 0x10) = *(f32*)(s + 0x10);
    *(f32*)(d + 0x14) = *(f32*)(s + 0x14);
    *(f32*)(d + 0x18) = *(f32*)(s + 0x18);
    MARK_DIRTY(dst);
    for (s32 i = 0; i < 5; i++) {
        if (*(u8**)(src + 0x2AC + i * 4) == NULL) {
            *(u8**)(src + 0x2AC + i * 4) = dst;
            return 1;
        }
    }
    return 0;
}

// 0x800A6F00
// Reads the current history entry (0x84-byte ring at +4) of the controller selected by lbl_803C4298+0x38,
// takes -value at +0x14 clamped to [-1, 1], and returns the signed table value of the highest threshold
// in lbl_80346B68 that its magnitude exceeds (0 if none). Returns 0 with no history.
f32 fn_800A6F00(void) {
    f32 sp[6];
    u8* ctl = fn_801483BC(*(s32*)(lbl_803C4298 + 0x38));
    if (*(s32*)(ctl + 0x14) == 0) {
        return 0.0f;
    }
    s32 head = *(s32*)(ctl + 0x10);
    s32 cap = *(s32*)(ctl + 0xC);
    u8* e = *(u8**)(ctl + 0x4) + (head % cap) * 0x84;
    f32 y = *(f32*)(e + 0x14);
    f32 x = *(f32*)(e + 0x10);
    f32 neg = -y;
    f32 z = *(f32*)(e + 0xC);
    sp[0] = z;
    sp[1] = x;
    sp[2] = y;
    sp[3] = z;
    sp[4] = x;
    sp[5] = y;
    f32 c = -1.0f;
    if (!(neg < -1.0f)) c = neg;
    f32 v = 1.0f;
    if (!(c > 1.0f)) v = c;
    f32 r = 0.0f;
    for (s32 i = 8; i >= 0; i--) {
        if ((f32)fabs(v) > lbl_80346B68[i]) {
            r = lbl_80346B8C[i];
            break;
        }
    }
    if (v < 0.0f) r = -r;
    return r;
}

// 0x800AD130
// Stores the target at +0x274, transforms the local origin through the matrix of the object's
// model (vtable +0x10 lookup) into a +/- position pair, raycasts from the target; on a hit (1) clears
// bit 0 of the scene flags and returns 1, else 0.
s32 fn_800AD130(u8* obj, u8* target) {
    f32 in[3];
    f32 vec[6];
    *(u8**)(obj + 0x274) = target;
    in[2] = 0.0f;
    in[1] = 0.0f;
    in[0] = 0.0f;
    u8* a = (*(GetFn*)(*(u8**)obj + 0x10))(obj);
    s32 key = fn_8018BDE0(a, *(s32*)(obj + 0x26C));
    u8* b = (*(GetFn*)(*(u8**)obj + 0x10))(obj);
    u8* m = fn_8018C39C(b, key);
    f32 tx = *(f32*)(m + 0x24) + ((*(f32*)(m + 0x18) * in[2]) + ((*(f32*)(m + 0x0) * in[0]) + (*(f32*)(m + 0xC) * in[1])));
    f32 ty = *(f32*)(m + 0x28) + ((*(f32*)(m + 0x1C) * in[2]) + ((*(f32*)(m + 0x4) * in[0]) + (*(f32*)(m + 0x10) * in[1])));
    f32 tz = *(f32*)(m + 0x2C) + ((*(f32*)(m + 0x20) * in[2]) + ((*(f32*)(m + 0x8) * in[0]) + (*(f32*)(m + 0x14) * in[1])));
    vec[0] = -tx;
    vec[1] = -ty;
    vec[2] = -tz;
    vec[3] = tx;
    vec[4] = ty;
    vec[5] = tz;
    if (fn_801B5338(*(u8**)(obj + 0x274), obj, *(u32*)(obj + 0x268), vec, 0) == 1) {
        *(u32*)(*(u8**)(obj + 0x4) + 0x20) &= 0xFFFFFFFEu;
        return 1;
    }
    return 0;
}

// 0x800D42F4
// Registers id under hash in the path database: appends id to the entry with that key (count at +0x84),
// or creates a new entry (up to 0xFF) holding it.
void fn_800D42F4(u8* db, u32 hash, u32 id) {
    s32 found = -1;
    s32 n = *(s32*)(db + 0x36454);
    for (s32 i = n - 1; i > -1; i--) {
        if ((s32)hash == *(s32*)DbEntry(db, i)) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        u8* base = DbEntry(db, found);
        u8* ent = DbEntry(db, found);
        *(u32*)(base + *(u32*)(ent + 0x84) * 4 + 0x4) = id;
        u8* cnt = DbEntry(db, found);
        *(s32*)(cnt + 0x84) = *(s32*)(cnt + 0x84) + 1;
        return;
    }
    s32 tmp[0x22];
    for (s32 k = 0; k < 0x22; k++) tmp[k] = 0;
    tmp[0] = (s32)hash;
    tmp[1] = (s32)id;
    tmp[0x21] = 1;
    s32 cur = *(s32*)(db + 0x36454);
    if (cur < 0xFF) {
        u8* e = db + 0x2DCDC + cur * 0x88;
        *(s32*)(e + 0x0) = (s32)hash;
        for (s32 k = 1; k < 0x21; k++) *(s32*)(e + k * 4) = tmp[k];
        *(s32*)(e + 0x84) = tmp[0x21];
        *(s32*)(db + 0x36454) = *(s32*)(db + 0x36454) + 1;
    }
}

// 0x800D4DE8
// Copies a 0x1AC-byte path record from src to dst (field by field; the bytes at +0x65..0x67 and
// +0x9D..0x9F are not copied).
void fn_800D4DE8(u8* dst, u8* src) {
    *(s32*)(dst + 0x0) = *(s32*)(src + 0x0);
    *(s32*)(dst + 0x4) = *(s32*)(src + 0x4);
    *(s32*)(dst + 0x14) = *(s32*)(src + 0x14);
    *(s32*)(dst + 0x24) = *(s32*)(src + 0x24);
    *(s32*)(dst + 0x8) = *(s32*)(src + 0x8);
    *(s32*)(dst + 0x18) = *(s32*)(src + 0x18);
    *(s32*)(dst + 0x28) = *(s32*)(src + 0x28);
    *(s32*)(dst + 0xC) = *(s32*)(src + 0xC);
    *(s32*)(dst + 0x1C) = *(s32*)(src + 0x1C);
    *(s32*)(dst + 0x2C) = *(s32*)(src + 0x2C);
    *(s32*)(dst + 0x10) = *(s32*)(src + 0x10);
    *(s32*)(dst + 0x20) = *(s32*)(src + 0x20);
    *(s32*)(dst + 0x30) = *(s32*)(src + 0x30);
    *(f32*)(dst + 0x34) = *(f32*)(src + 0x34);
    *(f32*)(dst + 0x38) = *(f32*)(src + 0x38);
    *(f32*)(dst + 0x3C) = *(f32*)(src + 0x3C);
    for (s32 o = 0x40; o <= 0x60; o += 4) *(s32*)(dst + o) = *(s32*)(src + o);
    *(u8*)(dst + 0x64) = *(u8*)(src + 0x64);
    for (s32 o = 0x68; o <= 0x98; o += 4) *(s32*)(dst + o) = *(s32*)(src + o);
    *(u8*)(dst + 0x9C) = *(u8*)(src + 0x9C);
    *(s32*)(dst + 0xA0) = *(s32*)(src + 0xA0);
    for (s32 o = 0xA4; o <= 0x1A4 - 4; o += 4) *(s32*)(dst + o) = *(s32*)(src + o);
    *(s32*)(dst + 0x1A4) = *(s32*)(src + 0x1A4);
    *(s32*)(dst + 0x1A8) = *(s32*)(src + 0x1A8);
}

// 0x800AF6A0
// Aims the follower at pos: records pos at +0x264, builds a look-at quaternion at +0x270 from the
// direction (obj +0xC8/+0xCC minus pos), stores the distance at +0x280 and places the scene record at
// base + (quat Y axis * distance).
void fn_800AF6A0(u8* obj, f32* pos) {
    f32 dir[3];
    f32 up[3];
    Vec3 base;
    f32 dx = *(f32*)(obj + 0xC8) - pos[0];
    f32 dy = *(f32*)(obj + 0xCC) - pos[1];
    *(f32*)(obj + 0x268) = pos[1];
    *(f32*)(obj + 0x264) = pos[0];
    *(f32*)(obj + 0x26C) = pos[2];
    f32 dz = 0.0f;
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    *(f32*)(obj + 0x270) = 0.0f;
    *(f32*)(obj + 0x274) = 0.0f;
    *(f32*)(obj + 0x278) = 0.0f;
    *(f32*)(obj + 0x27C) = 1.0f;
    if (len2 == 0.0f) {
        dir[2] = 0.0f;
        dir[1] = 0.0f;
        dir[0] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        dir[0] = dx * inv;
        dir[1] = dy * inv;
        dir[2] = dz * inv;
    }
    up[1] = 0.0f;
    up[0] = 0.0f;
    up[2] = 1.0f;
    fn_801458B0((f32*)(obj + 0x270), dir, up);
    f32 dist = 0.0f;
    f32 l2 = (dx * dx) + (dy * dy) + (dz * dz);
    if (l2 > 0.0f) {
        dist = (1.0f / sqrtf(l2)) * l2;
    }
    f32 h = *(f32*)(obj + 0xD0);
    f32 angle = 0.0f - 0.0f;
    *(f32*)(obj + 0x280) = dist;
    *(f32*)(obj + 0x284) = 0.0f;
    base.x = *(f32*)(obj + 0x264) + 0.0f;
    base.y = *(f32*)(obj + 0x268) + 0.0f;
    base.z = *(f32*)(obj + 0x26C) + h;
    if ((f32)fabs(angle) > 0.0f) {
        Vec3 axis;
        axis.y = 0.0f;
        axis.x = 0.0f;
        axis.z = 1.0f;
        fn_80146180(obj + 0x270, &axis, angle);
    }
    *(f32*)(obj + 0x284) = 0.0f;
    Vec3 ax = QuatGetAxisY((const Quat*)(obj + 0x270));
    f32 len = *(f32*)(obj + 0x280);
    u8* rec = *(u8**)(obj + 0x4);
    *(f32*)(rec + 0x0) = base.x + ax.x * len;
    *(f32*)(rec + 0x4) = base.y + ax.y * len;
    *(f32*)(rec + 0x8) = base.z + ax.z * len;
    MARK_DIRTY(obj);
}

// 0x800B07F0
// Variant of fn_800AF6A0 with two quaternions (+0x270 from Euler angles at +0xD4..0xDC scaled by 2/pi,
// +0x280 look-at), distance at +0x290; after placing the scene record it also copies the quaternion at
// +0x270 into the record (+0xC..0x18).
void fn_800B07F0(u8* obj, f32* pos) {
    f32 dir[3];
    f32 up[3];
    Vec3 euler;
    Vec3 base;
    f32 dx = *(f32*)(obj + 0xC8) - pos[0];
    f32 dy = *(f32*)(obj + 0xCC) - pos[1];
    *(f32*)(obj + 0x268) = pos[1];
    *(f32*)(obj + 0x264) = pos[0];
    *(f32*)(obj + 0x26C) = pos[2];
    f32 dz = 0.0f;
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    *(s32*)(obj + 0x298) = 0;
    *(f32*)(obj + 0x280) = 0.0f;
    *(f32*)(obj + 0x284) = 0.0f;
    *(f32*)(obj + 0x288) = 0.0f;
    *(f32*)(obj + 0x28C) = 1.0f;
    if (len2 == 0.0f) {
        dir[2] = 0.0f;
        dir[1] = 0.0f;
        dir[0] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        dir[0] = dx * inv;
        dir[1] = dy * inv;
        dir[2] = dz * inv;
    }
    up[1] = 0.0f;
    up[0] = 0.0f;
    up[2] = 1.0f;
    fn_801458B0((f32*)(obj + 0x280), dir, up);
    f32 dist = 0.0f;
    f32 l2 = (dx * dx) + (dy * dy) + (dz * dz);
    if (l2 > 0.0f) {
        dist = (1.0f / sqrtf(l2)) * l2;
    }
    *(f32*)(obj + 0x290) = dist;
    euler.x = *(f32*)(obj + 0xD4) * 0.63661975f;
    euler.y = *(f32*)(obj + 0xD8) * 0.63661975f;
    euler.z = *(f32*)(obj + 0xDC) * 0.63661975f;
    EulerToQuat((Quat*)(obj + 0x270), &euler);
    f32 h = *(f32*)(obj + 0xD0);
    f32 angle = 0.0f - 0.0f;
    *(f32*)(obj + 0x294) = 0.0f;
    base.x = *(f32*)(obj + 0x264) + 0.0f;
    base.y = *(f32*)(obj + 0x268) + 0.0f;
    base.z = *(f32*)(obj + 0x26C) + h;
    if ((f32)fabs(angle) > 0.0f) {
        Vec3 axis;
        axis.y = 0.0f;
        axis.x = 0.0f;
        axis.z = 1.0f;
        fn_80146180(obj + 0x280, &axis, angle);
        Vec3 axis2;
        axis2.y = 0.0f;
        axis2.x = 0.0f;
        axis2.z = 1.0f;
        fn_80146180(obj + 0x270, &axis2, angle);
    }
    *(f32*)(obj + 0x294) = 0.0f;
    Vec3 ax = QuatGetAxisY((const Quat*)(obj + 0x280));
    f32 len = *(f32*)(obj + 0x290);
    u8* rec = *(u8**)(obj + 0x4);
    *(f32*)(rec + 0x0) = base.x + ax.x * len;
    *(f32*)(rec + 0x4) = base.y + ax.y * len;
    *(f32*)(rec + 0x8) = base.z + ax.z * len;
    MARK_DIRTY(obj);
    rec = *(u8**)(obj + 0x4);
    *(f32*)(rec + 0xC) = *(f32*)(obj + 0x270);
    *(f32*)(rec + 0x10) = *(f32*)(obj + 0x274);
    *(f32*)(rec + 0x14) = *(f32*)(obj + 0x278);
    *(f32*)(rec + 0x18) = *(f32*)(obj + 0x27C);
    MARK_DIRTY(obj);
}

}  // extern "C"
