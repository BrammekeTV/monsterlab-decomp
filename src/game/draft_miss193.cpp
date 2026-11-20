// Batch miss193: brain-chunk toss/collision helpers, enemy slot shuffle and misc 0x8006xxxx-0x8008xxxx functions.
#include <math.h>
#include <string.h>
#include "types.h"
#include "engine/math.h"

extern "C" {
void* EventPost(s32 id, void* sender);                                // 0x801F3918
s32   RaySceneTest(void* ray);                                        // 0x80070458
u32   StringHash(const char* s);                                      // 0x801EB704
struct Locator;
Locator* WorldFindLocator(void* world, u32 hash);                     // 0x8012AA50
void* fn_80070400(u8* obj);                                           // 0x80070400
void  fn_80070FAC(u8* p, f32 v);                                      // 0x80070FAC
void  fn_8006FB68(u8* p, f32 v);                                      // 0x8006FB68
s32   fn_80079440(u8* p);                                             // 0x80079440 (returns a count; old decls take 3 extra floats)
void  fn_80081CB0(u8* obj, u8* owner);                                // 0x80081CB0 (second arg is stored at +0x2C0, a pointer slot)
s32   fn_80138C40(void* world, f32* a, f32* b, f32* hit, void* p, s32 c, s32 d);  // 0x80138C40
void  fn_801458B0(f32* quat, f32* a, f32* b);                         // 0x801458B0
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);                   // 0x80145F9C
u8*   fn_801B161C(u8* p, u32 key);                                    // 0x801B161C
void* fn_801B18E4(u8* list);                                          // 0x801B18E4
u8*   fn_801B3308(void);                                              // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                                      // 0x801B3310
void* fn_801B50FC(u8* p);                                             // 0x801B50FC
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                         // 0x801B55F4
void  fn_801C5C74(u8* obj, f32* a, f32* b);                           // 0x801C5C74
void  fn_801C5CC0(u8* obj, f32* a, f32* b);                           // 0x801C5CC0
void  fn_801CA004(u8* obj, const f32* v);                             // 0x801CA004
void  fn_801CA30C(u8* obj, const f32* v);                             // 0x801CA30C
void* fn_801D40E0(s32 n);                                             // 0x801D40E0
void* fn_80209814(void* v);                                           // 0x80209814
void  fn_8020B1C8(void* handle, u32 hash, s32 a);                     // 0x8020B1C8
f64   fn_8033F1F4(f64 x);                                             // 0x8033F1F4 (acos)
s32   rand(void);                                                     // 0x80337150

extern u8 gWorldState[];                  // 0x803E6D00
extern u8 lbl_803C4298[];                 // 0x803C4298
extern f32 lbl_803C6990[];                // 0x803C6990 (gravity vector)
extern s32 lbl_803467AC[];                // 0x803467AC
extern s32 lbl_803467B8[];                // 0x803467B8
extern f32 lbl_803467A0[];                // 0x803467A0
extern f32 lbl_803467C4[];                // 0x803467C4
extern s32 lbl_803474F8[];                // 0x803474F8
extern s32 lbl_80347504[];                // 0x80347504
extern s32 lbl_80347510[];                // 0x80347510
extern s32 lbl_8034751C[];                // 0x8034751C
extern u8 lbl_80347D78[];                 // 0x80347D78 (locator name template)
extern f32 lbl_80346570[];                // 0x80346570
extern f32 lbl_80346590[];                // 0x80346590
extern s32 lbl_803465D0[];                // 0x803465D0
extern f32 lbl_803465B0[];                // 0x803465B0
extern f32 lbl_80346620[];                // 0x80346620
extern f32 lbl_80346630[];                // 0x80346630
extern s32 lbl_805FB124;                  // 0x805FB124 (difficulty index)
extern u8 lbl_805FB1D8[];                 // 0x805FB1D8 (2 entries of 4 bytes: u16 flags, u8 locator index)
extern u8* lbl_805FB1E0[];                // 0x805FB1E0 (2 effect objects)
extern void* lbl_805FBBAC;                // 0x805FBBAC (voice SFX handle)

extern f32 lbl_805FD4E8;                  // 0x805FD4E8 (0.0f)
extern f32 lbl_805FD508;                  // 0x805FD508 (1.0f)
extern f32 lbl_805FD520;                  // 0x805FD520
extern f32 lbl_805FD540;                  // 0x805FD540
extern f32 lbl_805FD544;                  // 0x805FD544
extern f32 lbl_805FD548;                  // 0x805FD548
extern f32 lbl_805FD54C;                  // 0x805FD54C
extern f32 lbl_805FD550;                  // 0x805FD550
extern f32 lbl_805FD554;                  // 0x805FD554
extern f32 lbl_805FD558;                  // 0x805FD558
extern f32 lbl_805FD55C;                  // 0x805FD55C
extern f32 lbl_805FD560;                  // 0x805FD560
extern f32 lbl_805FD52C;                  // 0x805FD52C
extern f32 lbl_805FD800;                  // 0x805FD800 (0.0f)
extern f32 lbl_805FD838;                  // 0x805FD838 (+epsilon)
extern f32 lbl_805FD83C;                  // 0x805FD83C (-epsilon)
}

extern "C++" s32 gZoneResourceSize;       // 0x805FB94C
extern "C++" u8* fn_801B55A8(u8* ent);    // 0x801B55A8 (C++ linkage; returns the locator count as an integer value)

typedef u8*  (*GetSubFn)(u8* self);
typedef s32  (*IsDoneFn)(u8* self);
typedef void (*SetFlagFn)(u8* self, s32 v);
typedef f32* (*GetTargetFn)(u8* self, u8* a, u8* b);
typedef void* (*AttachFn)(u8* self, u8* owner);

static const u32 kHashBucket = 0x17C7F120u;  // "Bucket"

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtPos(f32 x) { return x <= 0.0f ? 0.0f : sqrtf(x); }

// Normalizes a quaternion stored as 4 floats (no zero guard in the original).
static inline void QuatNormalize4(f32* q) {
    f32 s = (q[3] * q[3]) + ((q[2] * q[2]) + ((q[0] * q[0]) + (q[1] * q[1])));
    f32 inv = 1.0f / sqrtf(s);
    q[0] *= inv;
    q[1] *= inv;
    q[2] *= inv;
    q[3] *= inv;
}

// Loads the 12-float locator entry selected by the 4-byte slot (u16 flags at +0, index at +2):
// from the object's locator list (flag 1) or from its local table at +0x98 (flag 2).
static inline bool LoadLocatorEntry(u8* self, const u8* slot, f32* e) {
    u16 h = *(u16*)(slot + 0);
    s32 idx = (h & 1) ? (s32) * (u8*)(slot + 2) : -1;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(self)) {
            fn_801B55F4(self, (u8*)e, idx);
            return true;
        }
        return false;
    }
    s32 j = (h & 2) ? (s32) * (u8*)(slot + 2) : -1;
    if (j != -1) {
        u8* src = *(u8**)(self + 0x98) + j * 0xC8;
        for (s32 k = 0; k < 12; k++) e[k] = *(f32*)(src + 0x38 + k * 4);
        return true;
    }
    return false;
}

// Finds the first entry of the world list (count +0x24, base +0x28, 0x20 bytes each) whose first
// word equals hash; NULL if none.
static inline u8* FindWorldEntry(u32 hash) {
    u8* list = *(u8**)gWorldState;
    u32 count = *(u32*)(list + 0x24);
    u8* base = *(u8**)(list + 0x28);
    s32 idx = -1;
    for (u32 i = 0; i < count; i++) {
        if (*(u32*)(base + i * 0x20) == hash) {
            idx = (s32)i;
            break;
        }
    }
    if (idx >= 0 && idx < *(s32*)(list + 0x24)) return base + idx * 0x20;
    return NULL;
}

// Angle (scaled by 2/pi) between v and w, in the XY plane; 0 if either is zero length.
static inline f32 AngleScaled(f32 vx, f32 vy, f32 wx, f32 wy) {
    f32 vz = 0.0f;
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
    return angle;
}

// Sets (on) or clears bit 1 of the node flags (+0x20) and of the sub-object flags (+8) for a chunk.
static inline void SetChunkBit(u8* o, bool on) {
    if (o) {
        if ((*(GetSubFn*)(*(u8**)o + 0xC))(o) != NULL) {
            u8* sub = (*(GetSubFn*)(*(u8**)o + 0xC))(o);
            if (on) *(u32*)(sub + 0x8) |= 2;
            else *(u32*)(sub + 0x8) &= ~2u;
        }
        u8* node = *(u8**)(o + 0x4);
        if (on) *(u32*)(node + 0x20) |= 2;
        else *(u32*)(node + 0x20) &= ~2u;
    }
}

static inline void SetChunkBitNoNull(u8* o, bool on) {
    if ((*(GetSubFn*)(*(u8**)o + 0xC))(o) != NULL) {
        u8* sub = (*(GetSubFn*)(*(u8**)o + 0xC))(o);
        if (on) *(u32*)(sub + 0x8) |= 2;
        else *(u32*)(sub + 0x8) &= ~2u;
    }
    u8* node = *(u8**)(o + 0x4);
    if (on) *(u32*)(node + 0x20) |= 2;
    else *(u32*)(node + 0x20) &= ~2u;
}

extern "C" {

// 0x800612B8
// True if the target (locked point at +0xB88 or +0xB94, or the object at +0xA14) lies within
// farg0 degrees of the facing direction.
s32 fn_800612B8(u8* obj, f32 farg0) {
    u32 flags = *(u32*)(obj + 0xA08);
    u8* pos = *(u8**)(obj + 0x4);
    bool partA = false;
    if (flags & 8) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + *(u16*)(pos + 0x30) * gZoneResourceSize;
        partA = !(*(u8*)(zone + 0xC0) != 0 && (flags & 7) == 0);
    }
    if (partA) {
        f32 vx, vy;
        if (flags & 8) {
            vx = *(f32*)(obj + 0xB88) - *(f32*)(pos + 0x0);
            vy = *(f32*)(obj + 0xB8C) - *(f32*)(pos + 0x4);
        } else {
            u8* t = *(u8**)(obj + 0xA14);
            if (t == NULL) return 0;
            u8* tp = *(u8**)(t + 0x4);
            vx = *(f32*)(tp + 0x0) - *(f32*)(pos + 0x0);
            vy = *(f32*)(tp + 0x4) - *(f32*)(pos + 0x4);
        }
        u8* m = (u8*)fn_801B50FC(obj);
        f32 angle = AngleScaled(vx, vy, *(f32*)(m + 0xC), *(f32*)(m + 0x10));
        return angle < (0.017453292f * farg0);
    }
    if (flags & 0x10) {
        f32 vx = *(f32*)(obj + 0xB94) - *(f32*)(pos + 0x0);
        f32 vy = *(f32*)(obj + 0xB98) - *(f32*)(pos + 0x4);
        u8* m = (u8*)fn_801B50FC(obj);
        f32 angle = AngleScaled(vx, vy, *(f32*)(m + 0xC), *(f32*)(m + 0x10));
        return angle < (0.017453292f * farg0);
    }
    return 0;
}

// 0x8006AD84
// Steers the chunk's facing quaternion (+0x17C) toward the locator target over dt, drops the
// steering object when done, mirrors the facing into the node and posts event 0x29 for the camera.
void fn_8006AD84(u8* self, f32 dt) {
    if (*(u8**)(self + 0x18C) == NULL) return;
    if ((*(IsDoneFn*)(*(u8**)self + 0x4C))(self) != 0) return;

    f32 e[12];
    if (!LoadLocatorEntry(self, self + 0x178, e)) return;

    u8* steer = *(u8**)(self + 0x18C);
    f32* tgt = (*(GetTargetFn*)(*(u8**)steer + 0x44))(steer, self + 0x18, self);
    Vec3 d = { tgt[0] - e[9], tgt[1] - e[10], tgt[2] - e[11] };
    Vec3 n = Vec3Normalized(d);
    f32 dir[3] = { n.x, n.y, n.z };
    f32 up[3] = { 0.0f, 0.0f, 1.0f };
    f32 q[4];
    fn_801458B0(q, dir, up);
    QuatNormalize4(q);
    f32 out[4];
    fn_80145F9C(out, (f32*)(self + 0x17C), q, *(f32*)(self + 0x24) * dt);
    *(f32*)(self + 0x17C) = out[0];
    *(f32*)(self + 0x180) = out[1];
    *(f32*)(self + 0x184) = out[2];
    *(f32*)(self + 0x188) = out[3];
    steer = *(u8**)(self + 0x18C);
    if ((*(IsDoneFn*)(*(u8**)steer + 0x4C))(steer) != 0) *(u8**)(self + 0x18C) = NULL;

    if (*(u32*)(self + 0x15C) & 0x800000) {
        u8* p = *(u8**)(self + 0x4);
        *(f32*)(p + 0xC) = *(f32*)(self + 0x17C);
        *(f32*)(p + 0x10) = *(f32*)(self + 0x180);
        *(f32*)(p + 0x14) = *(f32*)(self + 0x184);
        *(f32*)(p + 0x18) = *(f32*)(self + 0x188);
        *(f32*)(*(u8**)(self + 0x4) + 0xC) = 0.0f;
        *(f32*)(*(u8**)(self + 0x4) + 0x10) = 0.0f;
        p = *(u8**)(self + 0x4);
        QuatNormalize4((f32*)(p + 0xC));
        p = *(u8**)(self + 0x4);
        *(u32*)(p + 0x20) |= 0x8000;
        p = *(u8**)(self + 0x4);
        *(u32*)(p + 0x20) |= 0x10000000;
        p = *(u8**)(self + 0x4);
        *(u32*)(p + 0x20) |= 0x4000;
        p = *(u8**)(self + 0x4);
        *(u32*)(p + 0x20) |= 0x10000000;
    }
    if (*(u32*)(*(u8**)(self + 0x9C) + 0x8) == 0x1AC481C5u && !(*(u32*)(*(u8**)(self + 0x4) + 0x20) & 0x04000000)) {
        u8* ev = (u8*)EventPost(0x29, self);
        if (ev != NULL) {
            *(f32*)(ev + 0x4) = *(f32*)(self + 0x17C);
            *(f32*)(ev + 0x8) = *(f32*)(self + 0x180);
            *(f32*)(ev + 0xC) = *(f32*)(self + 0x184);
            *(f32*)(ev + 0x10) = *(f32*)(self + 0x188);
        }
    }
}

// 0x8006B244
// For each of the two locator slots: casts a 60 unit ray along the facing axis from the locator
// position and updates the effect object's end caps (a +/-0.25 wide pair) at the start and the hit.
void fn_8006B244(u8* self) {
    for (s32 i = 0; i < 2; i++) {
        f32 e[12];
        if (!LoadLocatorEntry(self, lbl_805FB1D8 + i * 4, e)) continue;

        f32 p[3] = { e[9], e[10], e[11] };
        Vec3 axis = QuatGetAxisY((const Quat*)(self + 0x17C));
        f32 end[3] = { p[0] + axis.x * 60.0f, p[1] + axis.y * 60.0f, p[2] + axis.z * 60.0f };
        f32 hit[3];
        if (fn_80138C40(gWorldState, p, end, hit, NULL, 1, 0) != 0) {
            end[0] = hit[0];
            end[1] = hit[1];
            end[2] = hit[2];
        }
        if (lbl_805FB1E0[i] != NULL) {
            u8* tp = *(u8**)(*(u8**)(lbl_803C4298 + 4) + 4);
            f32 vx = *(f32*)(tp + 0x0) - p[0];
            f32 vy = *(f32*)(tp + 0x4) - p[1];
            f32 vz = *(f32*)(tp + 0x8) - p[2];
            f32 cx = (vy * e[5]) - (vz * e[4]);
            f32 cy = (vz * e[3]) - (vx * e[5]);
            f32 cz = (vx * e[4]) - (vy * e[3]);
            Vec3 n = Vec3Normalized(Vec3{ cx, cy, cz });
            f32 hx = n.x * 0.25f;
            f32 hy = n.y * 0.25f;
            f32 hz = n.z * 0.25f;
            f32 a[3] = { p[0] + hx, p[1] + hy, p[2] + hz };
            f32 b[3] = { p[0] - hx, p[1] - hy, p[2] - hz };
            fn_801C5C74(lbl_805FB1E0[i], a, b);
            f32 a2[3] = { end[0] + hx, end[1] + hy, end[2] + hz };
            f32 b2[3] = { end[0] - hx, end[1] - hy, end[2] - hz };
            fn_801C5CC0(lbl_805FB1E0[i], a2, b2);
        }
    }
}

// 0x8006C6C0
// Sets the three RGBA-style 4-float colour/param records at +0x234, +0x244 and +0x254 for mode 0..8
// (any other mode uses the default record set).
void fn_8006C6C0(u8* self, u32 mode) {
    f32 z = lbl_805FD4E8;
    f32 a[4], b[4], c[4];
    switch (mode) {
    case 0:
        a[0] = z; a[1] = z; a[2] = z; a[3] = z;
        b[0] = lbl_805FD540; b[1] = lbl_805FD540; b[2] = lbl_805FD540; b[3] = z;
        c[0] = z; c[1] = z; c[2] = z; c[3] = z;
        break;
    case 1:
        a[0] = lbl_805FD544; a[1] = lbl_805FD544; a[2] = lbl_805FD544; a[3] = z;
        b[0] = lbl_805FD548; b[1] = lbl_805FD548; b[2] = lbl_805FD548; b[3] = z;
        c[0] = lbl_805FD54C; c[1] = lbl_805FD54C; c[2] = lbl_805FD54C; c[3] = z;
        break;
    case 2:
        a[0] = z; a[1] = lbl_805FD550; a[2] = z; a[3] = z;
        b[0] = lbl_805FD508; b[1] = lbl_805FD52C; b[2] = z; b[3] = z;
        c[0] = lbl_805FD508; c[1] = lbl_805FD554; c[2] = z; c[3] = z;
        break;
    case 3:
        a[0] = z; a[1] = lbl_805FD550; a[2] = z; a[3] = z;
        b[0] = lbl_805FD520; b[1] = lbl_805FD558; b[2] = lbl_805FD540; b[3] = z;
        c[0] = lbl_805FD520; c[1] = z; c[2] = lbl_805FD540; c[3] = z;
        break;
    case 4:
        a[0] = lbl_805FD55C; a[1] = z; a[2] = z; a[3] = z;
        b[0] = lbl_805FD560; b[1] = lbl_805FD540; b[2] = lbl_805FD540; b[3] = z;
        c[0] = lbl_805FD554; c[1] = lbl_805FD540; c[2] = lbl_805FD540; c[3] = z;
        break;
    case 5:
        a[0] = lbl_805FD560; a[1] = lbl_805FD560; a[2] = lbl_805FD560; a[3] = z;
        b[0] = lbl_805FD560; b[1] = lbl_805FD560; b[2] = lbl_805FD560; b[3] = z;
        c[0] = lbl_805FD560; c[1] = lbl_805FD560; c[2] = lbl_805FD560; c[3] = z;
        break;
    case 6:
        a[0] = z; a[1] = z; a[2] = z; a[3] = z;
        b[0] = lbl_805FD508; b[1] = z; b[2] = z; b[3] = z;
        c[0] = lbl_805FD508; c[1] = z; c[2] = z; c[3] = z;
        break;
    case 7:
        a[0] = z; a[1] = z; a[2] = z; a[3] = z;
        b[0] = lbl_805FD508; b[1] = lbl_805FD560; b[2] = lbl_805FD560; b[3] = z;
        c[0] = lbl_805FD508; c[1] = lbl_805FD560; c[2] = lbl_805FD560; c[3] = z;
        break;
    case 8:
        a[0] = lbl_805FD560; a[1] = lbl_805FD560; a[2] = lbl_805FD560; a[3] = z;
        b[0] = lbl_805FD560; b[1] = lbl_805FD560; b[2] = lbl_805FD560; b[3] = z;
        c[0] = lbl_805FD560; c[1] = lbl_805FD560; c[2] = lbl_805FD560; c[3] = z;
        break;
    default:
        a[0] = z; a[1] = z; a[2] = lbl_805FD544; a[3] = z;
        b[0] = lbl_805FD560; b[1] = lbl_805FD560; b[2] = lbl_805FD560; b[3] = z;
        c[0] = z; c[1] = z; c[2] = z; c[3] = z;
        break;
    }
    for (s32 k = 0; k < 4; k++) {
        *(f32*)(self + 0x234 + k * 4) = a[k];
    }
    for (s32 k = 0; k < 4; k++) {
        *(f32*)(self + 0x244 + k * 4) = b[k];
    }
    for (s32 k = 0; k < 4; k++) {
        *(f32*)(self + 0x254 + k * 4) = c[k];
    }
}

// 0x800782AC
// Picks, per row (3 rows of 20 candidates at +0xFD4, stride 0x50, modulus at +0x1110+4*row), random
// distinct-neighbour entries, then interleaves the rows randomly into out1 (value) / out2 (row index).
void fn_800782AC(u8* self, s32* out1, s32* out2) {
    s32 rows[3][20];
    s32 cnt[3];
    s32 a0[4] = { lbl_803474F8[0], lbl_80347504[0], lbl_80347510[0], lbl_8034751C[0] };
    s32 a1[4] = { lbl_803474F8[1], lbl_80347504[1], lbl_80347510[1], lbl_8034751C[1] };
    s32 a2[4] = { lbl_803474F8[2], lbl_80347504[2], lbl_80347510[2], lbl_8034751C[2] };
    cnt[0] = a0[lbl_805FB124];
    cnt[1] = a1[lbl_805FB124];
    cnt[2] = a2[lbl_805FB124];

    for (s32 i = 0; i < 3; i++) {
        s32 n = cnt[i];
        s32* row = rows[i];
        for (s32 j = 0; j < n; j++) {
            s32 ok;
            do {
                s32 mod = *(s32*)(self + 0x1110 + i * 4);
                s32 r = rand() % mod;
                row[j] = *(s32*)(self + i * 0x50 + r * 4 + 0xFD4);
                s32 lo = (j - 3) & ~((j - 3) >> 31);
                s32 hi = j + 3;
                ok = 1;
                if (hi > n) hi = n;
                for (s32 k = lo; k < hi; k++) {
                    if (k != j && row[j] == row[k]) {
                        ok = 0;
                        break;
                    }
                }
            } while (ok == 0);
        }
    }

    s32 left = *(s32*)(self + 0x8C);
    while (left > 0) {
        s32 r;
        s32 v;
        do {
            r = rand() % 3;
            v = cnt[r];
        } while (v == 0);
        cnt[r] = v - 1;
        left--;
        *out1 = rows[r][v - 1];
        out1++;
        *out2 = r;
        out2++;
    }
}

// 0x8007C5FC
// Initializes a pair record: owner objects (a2 at +0, a1 at +4), mode a3 (+8) and two 3-float vectors
// (+0xC, +0x18); builds the sub-object at +0x24 and flips the visibility bit 2 on the pair
// depending on whether a2 is NULL. Returns the result of fn_80079440 on the new sub-object.
s32 fn_8007C5FC(u8* self, u8* a1, u8* a2, s32 a3, f32* v1, f32* v2) {
    *(s32*)(self + 0x8) = a3;
    *(u8**)(self + 0x4) = a1;
    *(u8**)(self + 0x0) = a2;
    *(f32*)(self + 0xC) = v1[0];
    *(f32*)(self + 0x10) = v1[1];
    *(f32*)(self + 0x14) = v1[2];
    *(f32*)(self + 0x18) = v2[0];
    *(f32*)(self + 0x1C) = v2[1];
    *(f32*)(self + 0x20) = v2[2];
    s32 mode = (a2 == NULL) ? 1 : 0;
    u8* sub = (u8*)fn_80070400(*(u8**)(self + 0x4));
    *(u8**)(self + 0x24) = sub;
    s32 ret = fn_80079440(sub + 4);
    switch (mode) {
    case 0: {
        u8* first = *(u8**)(self + 0x0);
        SetChunkBit(first, true);
        u8* second = *(u8**)(self + 0x4);
        SetChunkBitNoNull(second, false);
        break;
    }
    case 1: {
        u8* first = *(u8**)(self + 0x0);
        SetChunkBit(first, false);
        u8* second = *(u8**)(self + 0x4);
        SetChunkBitNoNull(second, true);
        break;
    }
    case 2: {
        u8* first = *(u8**)(self + 0x0);
        SetChunkBit(first, false);
        u8* second = *(u8**)(self + 0x4);
        SetChunkBitNoNull(second, false);
        break;
    }
    }
    return ret;
}

// 0x8007D860
// Initializes the chunk: copies three parameter triples, sets up the voice SFX handle, difficulty
// scaled values, looks up the six "locator N" entries and the distance between the two world markers,
// then parks each owned chunk via fn_80081CB0.
void fn_8007D860(u8* self) {
    *(s32*)(self + 0x190) = lbl_803467B8[0];
    *(f32*)(self + 0x19C) = lbl_803467A0[0];
    *(s32*)(self + 0x1A8) = lbl_803467AC[0];
    *(f32*)(self + 0x1B4) = lbl_803467C4[0];
    *(s32*)(self + 0x190) = lbl_803467B8[1];
    *(f32*)(self + 0x19C) = lbl_803467A0[1];
    *(s32*)(self + 0x1A8) = lbl_803467AC[1];
    *(f32*)(self + 0x1B4) = lbl_803467C4[1];
    *(s32*)(self + 0x190) = lbl_803467B8[2];
    *(f32*)(self + 0x19C) = lbl_803467A0[2];
    *(s32*)(self + 0x1A8) = lbl_803467AC[2];
    *(f32*)(self + 0x1B4) = lbl_803467C4[2];
    *(f32*)(self + 0x180) = 0.2f;
    *(f32*)(self + 0x184) = 0.4f;
    *(f32*)(self + 0x188) = 0.25f;
    *(f32*)(self + 0x18C) = 0.05f;
    *(f32*)(self + 0x174) = 0.2f;
    *(f32*)(self + 0x178) = 1.0f;
    *(u8*)(self + 0x17C) = 0;
    fn_801B3310(fn_801B3308(), self + 0xC8);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    fn_8020B1C8(lbl_805FBBAC, 0xA5237C5Fu, 0);
    u8* o1 = *(u8**)(self + 0x29C);
    (*(AttachFn*)(*(u8**)o1 + 0xBC))(o1, self);
    u8* o2 = *(u8**)(self + 0x2A0);
    (*(AttachFn*)(*(u8**)o2 + 0xBC))(o2, self);
    *(f32*)(self + 0x324) = lbl_80346620[RaySceneTest(NULL)];
    *(s32*)(self + 0x2AC) = lbl_803465D0[RaySceneTest(NULL)];
    f32 v0 = lbl_80346630[RaySceneTest(NULL)];
    *(u8*)(self + 0xC0) = 0;
    *(f32*)(self + 0xC4) = 1.0f;
    *(f32*)(self + 0xBC) = v0;
    f32 meter = lbl_80346570[RaySceneTest(NULL)];
    *(f32*)(self + 0x32C) = 0.0f;
    *(f32*)(self + 0x328) = meter;
    *(s32*)(self + 0x330) = 0;
    *(s32*)(self + 0x340) = 0;
    *(s32*)(self + 0x334) = 0;
    *(s32*)(self + 0x344) = 0;
    *(s32*)(self + 0x338) = 0;
    *(s32*)(self + 0x348) = 0;
    *(s32*)(self + 0x33C) = 0;
    *(s32*)(self + 0x34C) = 0;
    fn_8006FB68(self, 35.0f);
    *(f32*)(self + 0xA8) = 0.0f;
    fn_80070FAC(self, 0.5f);
    *(f32*)(self + 0x350) = 0.0f;
    char name[11];
    memcpy(name, lbl_80347D78, 11);
    for (s32 i = 0; i < 6; i++) {
        name[9] = (char)(0x31 + i);
        *(void**)(self + 0x354 + i * 4) = WorldFindLocator(gWorldState, StringHash(name));
    }
    *(s32*)(self + 0x36C) = 0;
    *(s32*)(self + 0x370) = 0;
    *(s32*)(self + 0x374) = 0;
    *(s32*)(self + 0x378) = 0;
    *(s32*)(self + 0x37C) = 0;
    *(s32*)(self + 0x380) = 0;
    *(f32*)(self + 0x388) = 5.0f;
    u8* m1 = FindWorldEntry(0xAAE1A370u);
    u8* m2 = FindWorldEntry(0x4E2DA8F0u);
    f32 dx = *(f32*)(m2 + 0x8) - *(f32*)(m1 + 0x8);
    f32 dy = *(f32*)(m2 + 0xC) - *(f32*)(m1 + 0xC);
    f32 dz = 0.0f;
    *(f32*)(self + 0x390) = SqrtPos((dz * dz) + ((dx * dx) + (dy * dy)));
    for (s32 i = 0; i < *(s32*)(self + 0x298); i++) {
        fn_80081CB0(*(u8**)(self + 0x1F8 + i * 4), self);
    }
    f32 d = lbl_80346590[RaySceneTest(NULL)];
    *(f32*)(self + 0x38C) = lbl_803465B0[RaySceneTest(NULL)] / d;
}

// 0x80082120
// Sets up a toss to the end point p: stores it at +0x28C, derives the launch velocity (+0x298..)
// from start (+0x280) over duration +0x278 under gravity, and solves for the flight time at +0x27C.
void fn_80082120(u8* obj, f32* p) {
    f32 half = 0.5f;
    f32 zero = lbl_805FD800;
    f32 speed = *(f32*)(obj + 0x2B0);
    f32 dur = *(f32*)(obj + 0x278);
    f32 invDur = 1.0f / dur;
    f32 ex = p[0];
    f32 ey = p[1];
    f32 ez = p[2];
    *(f32*)(obj + 0x28C) = ex;
    *(f32*)(obj + 0x290) = ey;
    *(f32*)(obj + 0x294) = ez;
    f32 k = dur * ((half * speed) * dur);
    f32 sx0 = *(f32*)(obj + 0x280);
    f32 sy1 = *(f32*)(obj + 0x284);
    f32 sz0 = *(f32*)(obj + 0x288);
    f32 gx = lbl_803C6990[0] * k;
    f32 gy = lbl_803C6990[1] * k;
    f32 gz = lbl_803C6990[2] * k;
    f32 vx = (ex - (gx + sx0)) * invDur;
    f32 vy = (ey - (gy + sy1)) * invDur;
    f32 vz = (ez - (gz + sz0)) * invDur;
    *(f32*)(obj + 0x298) = vx;
    *(f32*)(obj + 0x29C) = vy;
    *(f32*)(obj + 0x2A0) = vz;
    f32 z0 = sz0 - zero;
    f32 a = *(f32*)(obj + 0x2B0) * (half * lbl_803C6990[2]);
    f32 disc = (vz * vz) - ((4.0f * a) * z0);
    f32 root = zero;
    if (!(disc <= zero)) root = sqrtf(disc);
    f32 nb = -vz;
    f32 den = 2.0f * a;
    f32 t1 = (nb + root) / den;
    f32 t2 = (nb - root) / den;
    if (t1 <= t2) t1 = t2;
    *(f32*)(obj + 0x27C) = t1;
}

// 0x80082C04
// Sets the vector v (+ a 0 w) on every child at +0x14.. (count at +0xC): if v is within epsilon of
// zero the children get the fixed pair (0,0,1)/(0,0,1), otherwise v itself.
void fn_80082C04(u8* self, f32* v) {
    f32 vec[4];
    f32 zero = lbl_805FD800;
    f32 x = v[0], y = v[1], z = v[2];
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
    vec[3] = zero;
    s32 small = 0;
    if (x <= lbl_805FD838 && x >= lbl_805FD83C && y <= lbl_805FD838 && y >= lbl_805FD83C &&
        z <= lbl_805FD838 && z >= lbl_805FD83C && zero <= lbl_805FD838 && zero >= lbl_805FD83C) {
        small = 1;
    }
    if (small != 0) {
        f32 one = 1.0f;
        u8* p = self;
        for (s32 i = 0; i < *(s32*)(self + 0xC); i++) {
            if (*(u8**)(p + 0x14) != NULL) {
                f32 axis[7];
                axis[0] = zero;
                axis[1] = zero;
                axis[2] = one;
                axis[3] = zero;
                axis[5] = zero;
                axis[4] = zero;
                axis[6] = one;
                fn_801CA30C(*(u8**)(p + 0x14), axis);
            }
            p += 4;
        }
    } else {
        u8* p = self;
        for (s32 i = 0; i < *(s32*)(self + 0xC); i++) {
            u8* child = *(u8**)(p + 0x14);
            if (child != NULL) fn_801CA30C(child, vec);
            p += 4;
        }
    }
}

// 0x80082D88
// Same as fn_80082C04 but applies the vector with fn_801CA004.
void fn_80082D88(u8* self, f32* v) {
    f32 vec[4];
    f32 zero = lbl_805FD800;
    f32 x = v[0], y = v[1], z = v[2];
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
    vec[3] = zero;
    s32 small = 0;
    if (x <= lbl_805FD838 && x >= lbl_805FD83C && y <= lbl_805FD838 && y >= lbl_805FD83C &&
        z <= lbl_805FD838 && z >= lbl_805FD83C && zero <= lbl_805FD838 && zero >= lbl_805FD83C) {
        small = 1;
    }
    if (small != 0) {
        f32 one = 1.0f;
        u8* p = self;
        for (s32 i = 0; i < *(s32*)(self + 0xC); i++) {
            if (*(u8**)(p + 0x14) != NULL) {
                f32 axis[7];
                axis[0] = zero;
                axis[1] = zero;
                axis[2] = one;
                axis[3] = zero;
                axis[5] = zero;
                axis[4] = zero;
                axis[6] = one;
                fn_801CA004(*(u8**)(p + 0x14), axis);
            }
            p += 4;
        }
    } else {
        u8* p = self;
        for (s32 i = 0; i < *(s32*)(self + 0xC); i++) {
            u8* child = *(u8**)(p + 0x14);
            if (child != NULL) fn_801CA004(child, vec);
            p += 4;
        }
    }
}

// 0x80082F9C
// Collision of the chunk against other's bucket entry: inside the inner radius (+0x3C) it latches
// the contact (+0x2B4/+0x2B8 = 1); inside inner+outer radius it latches mode 3, records which side
// the hit came from (+0x2BC) and reflects the velocity (+0x298..) about the contact normal.
s32 fn_80082F9C(u8* self, u8* other) {
    u8* r5 = *(u8**)(other + 0x8C);
    u8* first = NULL;
    if (*(s32*)(r5 + 0x10) != 0) first = *(u8**)(*(u8**)(r5 + 0xC));
    u8* a = fn_801B161C(first, kHashBucket);
    u8* b = (u8*)fn_801B18E4(*(u8**)(self + 0x8C));
    f32 dx = *(f32*)(a + 0x14) - *(f32*)(b + 0x14);
    f32 dy = *(f32*)(a + 0x18) - *(f32*)(b + 0x18);
    f32 dz = *(f32*)(a + 0x1C) - *(f32*)(b + 0x1C);
    f32 dist = lbl_805FD800;
    f32 d2 = (dz * dz) + ((dx * dx) + (dy * dy));
    if (!(d2 <= dist)) dist = sqrtf(d2);
    f32 inner = *(f32*)(a + 0x3C);
    if (dist < inner) {
        *(u8**)(self + 0x2B4) = other;
        *(s32*)(self + 0x2B8) = 1;
        u8* owner = *(u8**)(self + 0x2C0);
        *(u8**)(*(u8**)(owner + 0x29C) + 0x280) = self;
        *(f32*)(self + 0x274) = lbl_805FD800;
        return 1;
    }
    if (dist < inner + *(f32*)(b + 0x3C)) {
        *(u8**)(self + 0x2B4) = other;
        *(s32*)(self + 0x2B8) = 3;
        u8* owner = *(u8**)(self + 0x2C0);
        *(u8**)(*(u8**)(owner + 0x29C) + 0x280) = self;
        *(u8*)(self + 0x2BC) = *(f32*)(a + 0x14) < *(f32*)(b + 0x14);
        *(f32*)(self + 0x274) = lbl_805FD800;
        Vec3 n = Vec3Normalized(Vec3{ dx, dy, dz });
        f32 t = *(f32*)(self + 0x2B0) * *(f32*)(self + 0x274);
        f32 gx = lbl_803C6990[0] * t;
        f32 gy = lbl_803C6990[1] * t;
        f32 gz = lbl_803C6990[2] * t;
        f32 px = *(f32*)(self + 0x298) + gx;
        f32 py = *(f32*)(self + 0x29C) + gy;
        f32 pz = *(f32*)(self + 0x2A0) + gz;
        f32 dot = (n.z * pz) + ((n.x * px) + (n.y * py));
        f32 qx = (-n.x * dot) * 2.0f;
        f32 qy = (-n.y * dot) * 2.0f;
        f32 qz = (-n.z * dot) * 2.0f;
        *(f32*)(self + 0x298) = px + qx;
        *(f32*)(self + 0x29C) = py + qy;
        *(f32*)(self + 0x2A0) = pz + qz;
        u8* node = *(u8**)(self + 0x4);
        *(f32*)(self + 0x280) = *(f32*)(node + 0x0);
        *(f32*)(self + 0x284) = *(f32*)(node + 0x4);
        *(f32*)(self + 0x288) = *(f32*)(node + 0x8);
        return 1;
    }
    return 0;
}

// 0x80083314
// Contact test against other (if its +0x270 flag is set): if the distance between the two bucket
// entries is below the sum of their radii (+0x3C), latches the contact on both objects.
s32 fn_80083314(u8* self, u8* other) {
    if (*(u8*)(other + 0x270) == 0) return 0;
    u8* a = (u8*)fn_801B18E4(*(u8**)(self + 0x8C));
    u8* b = (u8*)fn_801B18E4(*(u8**)(other + 0x8C));
    f32 dx = *(f32*)(b + 0x14) - *(f32*)(a + 0x14);
    f32 dy = *(f32*)(b + 0x18) - *(f32*)(a + 0x18);
    f32 dz = *(f32*)(b + 0x1C) - *(f32*)(a + 0x1C);
    f32 dist = lbl_805FD800;
    f32 d2 = (dz * dz) + ((dx * dx) + (dy * dy));
    if (!(d2 <= dist)) dist = sqrtf(d2);
    if (dist < *(f32*)(b + 0x3C) + *(f32*)(a + 0x3C)) {
        *(u8**)(self + 0x2B4) = other;
        *(s32*)(self + 0x2B8) = 1;
        *(u8**)(other + 0x280) = self;
        *(f32*)(self + 0x274) = lbl_805FD800;
        return 1;
    }
    return 0;
}

}  // extern "C"
