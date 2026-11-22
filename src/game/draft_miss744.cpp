// Mixed 0x800B..0x801A functions: gauge/steer state machine, camera quaternion step, follow-camera
// yaw easing, trail ring-buffer fill, badge inventory list build and a 2D hit-test on an image.
#include <math.h>
#include "types.h"
#include "game/part.h"

struct StringTable {
    const char** strings;   // 0x00
    u8           pad[8];
    u32          count;     // 0x0C
};

extern "C++" StringTable* gStringTable;       // 0x805FB910
extern "C++" StringTable* gStringTable2;      // 0x805FB90C
extern "C++" void* gEventMgr;                 // 0x805FB6C8

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
u32  RandomU32(void);                                                // 0x80143460
s32  RaySceneTest(void* ray);                                        // 0x80070458
u32  SaveHasBadge(u8* s, u32 badge);                                 // 0x80223308
int  SysSprintf(char* buf, const char* fmt, ...);
f32  cosf(f32 x);                                                    // 0x8033EC98 (MSL)
f32  sinf(f32 x);                                                    // 0x8033F0A4 (MSL)
s32  fn_800B66F0(u8* p, s32 mode);                                   // 0x800B66F0
void fn_800DF7C4(u8* p, f32 dt);                                     // 0x800DF7C4
s32  fn_800DECD8(u8* p, f32 dt);                                     // 0x800DECD8
void fn_800E2E48(u8* p, f32 a, f32 b);                               // 0x800E2E48
u8*  fn_801094F4(char* name, s32 flags);                             // 0x801094F4
s32  fn_80109A0C(s32 id);                                            // 0x80109A0C
void fn_801458B0(f32* quat, f32* a, f32* b);                         // 0x801458B0
void fn_80149828(u8* arg0, u32* arg1, u8* arg2, u8* arg3, u8* arg4, u8* arg5);   // 0x80149828
u32  fn_8015B82C(u8* hdr, s32 x, s32 y);                             // 0x8015B82C
void fn_801ED788(f32* out, u8* a1);                                  // 0x801ED788
f32  fn_801F60F8(f32 farg0, f32 farg1);                              // 0x801F60F8

extern f32 lbl_80346038[];                  // 0x80346038 (per-difficulty table)
extern f32 lbl_80346048[];                  // 0x80346048 (per-difficulty table)
extern f32 lbl_80346208[];                  // 0x80346208 (per-difficulty table)
extern u8 lbl_803C4298[];                   // 0x803C4298 (+8: camera object)
extern const char lbl_80354800[];           // 0x80354800 "Not found"
extern const char lbl_8035480C[];           // 0x8035480C "NOT FOUND"
extern const char lbl_805F8200[];           // 0x805F8200 (empty string)
extern const char* lbl_805F8F50;            // 0x805F8F50 (icon path prefix)
extern u8* lbl_805FBBE0;                    // 0x805FBBE0 (save block pointer)
extern u8 lbl_805FB2A8;                     // 0x805FB2A8
}

// Resolve a string id; bit 31 selects the secondary table.
static const char* BadgeText(s32 id) {
    if (id == -1) return lbl_80354800;
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        u32 index = (u32)id & 0x7FFFFFFF;
        if (gStringTable2->strings != NULL && index < gStringTable2->count) return gStringTable2->strings[index];
        return lbl_805F8200;
    }
    if (gStringTable->strings != NULL && (u32)id < gStringTable->count) return gStringTable->strings[(u32)id];
    return id == -1 ? lbl_8035480C : lbl_805F8200;
}

// Clamp to [-2pi, 2pi], wrap into [-pi, pi], clamp again.
static f32 NormalizeYaw(f32 a) {
    f32 v;
    if (a < -6.2831855f)      v = -6.2831855f;
    else if (a > 6.2831855f)  v = 6.2831855f;
    else                      v = a;
    if (v < -3.1415927f)      v += 6.2831855f;
    else if (v > 3.1415927f)  v -= 6.2831855f;
    if (v < -3.1415927f)      return -3.1415927f;
    if (v > 3.1415927f)       return 3.1415927f;
    return v;
}

// Which side the steer value points to: 0 = dead zone, 1 = positive, 2 = negative.
static s32 SteerSide(u8* p) {
    f32 a = fabsf(*(f32*)(p + 0x2C4));
    if (a < 0.011111111f * lbl_80346208[RaySceneTest(NULL)]) return 0;
    return *(f32*)(p + 0x2C4) > 0.0f ? 1 : 2;
}

extern "C" {

// 0x800B6E48
// Steering/lean state machine (states 0 idle, 1 roll new target, 2 ease toward it) plus the
// direction latch at +0x2B0.
void fn_800B6E48(u8* arg0, f32 farg0) {
    s32 state = *(s32*)(arg0 + 0x2B4);
    *(f32*)(arg0 + 0x2A8) = *(f32*)(arg0 + 0x2A8) + farg0;
    *(f32*)(arg0 + 0x2AC) = *(f32*)(arg0 + 0x2AC) + farg0;
    switch (state) {
    case 2: {
        if (*(f32*)(arg0 + 0x2A8) >= lbl_80346038[RaySceneTest(NULL)]) {
            f32 v = *(f32*)(arg0 + 0x29C);
            v = v - (1.5f * v * farg0);
            *(f32*)(arg0 + 0x29C) = v;
            if (v <= 10.0f) {
                *(s32*)(arg0 + 0x2B4) = 0;
                *(f32*)(arg0 + 0x29C) = 0.0f;
                *(f32*)(arg0 + 0x2AC) = 0.0f;
            }
        }
        if (fn_800B66F0(arg0, 1) == 1) {
            f32 v = *(f32*)(arg0 + 0x29C);
            if (v > 0.0f) *(f32*)(arg0 + 0x29C) = v * -1.0f;
        } else if (fn_800B66F0(arg0, 1) == 2) {
            f32 v = *(f32*)(arg0 + 0x29C);
            if (v < 0.0f) *(f32*)(arg0 + 0x29C) = v * -1.0f;
        }
        f32 v = *(f32*)(arg0 + 0x29C);
        if (v > 0.0f) {
            EventSend(gEventMgr, arg0, NULL, 0x82D4D48Fu, 0, 0, 1, 0);
        } else if (v < 0.0f) {
            EventSend(gEventMgr, arg0, NULL, 0x678C8AE2u, 0, 0, 1, 0);
        }
        f32 w = *(f32*)(arg0 + 0x29C);
        if (w > -0.0001f && w < 0.0001f) {
            EventSend(gEventMgr, arg0, NULL, 0xC1CFD89Fu, 0, 0, 1, 0);
        }
        break;
    }
    case 0:
        EventSend(gEventMgr, arg0, NULL, 0xC1CFD89Fu, 0, 0, 1, 0);
        if (*(f32*)(arg0 + 0x2AC) >= lbl_80346048[RaySceneTest(NULL)]) {
            *(s32*)(arg0 + 0x2B4) = 1;
        }
        break;
    case 1: {
        u32 r = RandomU32();
        *(f32*)(arg0 + 0x2A8) = 0.0f;
        *(s32*)(arg0 + 0x2B4) = 2;
        *(f32*)(arg0 + 0x29C) = 20.0f + 20.0f * (2.3283064e-10f * (f32)r);
        break;
    }
    }
    if (*(s32*)(arg0 + 0x2B0) != SteerSide(arg0)) {
        *(s32*)(arg0 + 0x2B0) = SteerSide(arg0);
    }
}

// 0x800DFC44
// Moves the object along its velocity rotated by the orientation quaternion (q * v * q^-1) and
// writes the facing vector (arg2) and the point 0.8 units behind the new position (arg1).
s32 fn_800DFC44(u8* arg0, f32* arg1, f32* arg2, f32 farg0) {
    f32 q[4];
    f32 up[3];
    f32 v[3];

    fn_800DF7C4(arg0, farg0);
    f32 sy = sinf(*(f32*)(arg0 + 0xC));
    f32 cy = cosf(*(f32*)(arg0 + 0xC));
    f32 sp = sinf(*(f32*)(arg0 + 0x10));
    f32 cp = cosf(*(f32*)(arg0 + 0x10));
    arg2[2] = sp;
    arg2[0] = cy * cp;
    arg2[1] = sy * cp;
    v[0] = *(f32*)(arg0 + 0x20) * farg0;
    v[1] = *(f32*)(arg0 + 0x24) * farg0;
    v[2] = *(f32*)(arg0 + 0x28) * farg0;
    up[0] = 0.0f;
    up[1] = 0.0f;
    up[2] = 1.0f;
    fn_801458B0(q, arg2, up);

    f32 qx = q[0], qy = q[1], qz = q[2], qw = q[3];
    f32 vx = v[0], vy = v[1], vz = v[2];
    f32 nx = -qx, ny = -qy, nz = -qz;
    // t = q * (v, 0)
    f32 A = ((qy * vz) + ((qw * vx) + (qx * 0.0f))) - (qz * vy);
    f32 B = (qz * vx) + ((qy * 0.0f) + ((qw * vy) - (qx * vz)));
    f32 C = (qz * 0.0f) + (((qw * vz) + (qx * vy)) - (qy * vx));
    f32 D = (((qw * 0.0f) - (qx * vx)) - (qy * vy)) - (qz * vz);
    // r = t * conj(q)
    f32 X = ((B * nz) + ((D * nx) + (A * qw))) - (C * ny);
    f32 Y = (C * nx) + ((B * qw) + ((D * ny) - (A * nz)));
    f32 Z = (C * qw) + (((D * nz) + (A * ny)) - (B * nx));

    f32 px = *(f32*)(arg0 + 0x14) + X;
    f32 py = *(f32*)(arg0 + 0x18) + Y;
    f32 pz = *(f32*)(arg0 + 0x1C) + Z;
    *(f32*)(arg0 + 0x18) = py;
    *(f32*)(arg0 + 0x14) = px;
    *(f32*)(arg0 + 0x1C) = pz;
    arg1[0] = px - arg2[0] * 0.8f;
    arg1[1] = py - arg2[1] * 0.8f;
    arg1[2] = pz - arg2[2] * 0.8f;
    return 1;
}

// 0x800E3920
// Eases the follow camera's yaw (+0xB4) toward the bearing to this object, clamped per step.
s32 fn_800E3920(u8* arg0, f32 farg0) {
    if (fn_800DECD8(arg0, farg0) == 0) return 0;
    if (*(u8*)(arg0 + 0x55) != 0) {
        u8* cam = *(u8**)(lbl_803C4298 + 8);
        f32 pos[3];
        f32 d[3];
        f32 n[3];
        fn_801ED788(pos, arg0 + 0x68);
        d[0] = pos[0] - *(f32*)(cam + 0x98);
        d[1] = pos[1] - *(f32*)(cam + 0x9C);
        d[2] = pos[2] - *(f32*)(cam + 0xA0);
        f32 len2 = (d[2] * d[2]) + ((d[0] * d[0]) + (d[1] * d[1]));
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            n[0] = d[0] * inv;
            n[1] = d[1] * inv;
            n[2] = d[2] * inv;
        }
        f32 yaw = NormalizeYaw(fn_801F60F8(n[1], n[0]));
        cam = *(u8**)(lbl_803C4298 + 8);
        f32 diff = NormalizeYaw(yaw - *(f32*)(cam + 0xB4));
        f32 dead = *(f32*)(arg0 + 0x58);
        f32 a = fabsf(diff);
        if (dead > 0.0f && a > 0.0001f) {
            if (a < dead)        diff = 0.0f;
            else if (diff < 0.0f) diff += dead;
            else                 diff -= dead;
        }
        f32 rate = (100.0f * *(f32*)(arg0 + 0x5C)) * *(f32*)(arg0 + 0x44);
        f32 step = diff * rate * farg0;
        if (step < -10.0f)      step = -10.0f;
        else if (step > 10.0f)  step = 10.0f;
        cam = *(u8**)(lbl_803C4298 + 8);
        f32* yawAcc = (f32*)(cam + 0xB8);
        s32 apply = 0;
        if (step > 0.0f) {
            if (*yawAcc < 3.0f * step) apply = 1;
        } else {
            if (*yawAcc > 3.0f * step) apply = 1;
        }
        if (apply != 0) *yawAcc = *yawAcc + step;
        fn_800E2E48(*(u8**)(lbl_803C4298 + 8) + 0x68, *(f32*)(arg0 + 0x60), *(f32*)(arg0 + 0x44));
    }
    return 0;
}

// 0x8010B7C8
// Fills a ring buffer of 0x1C-byte trail segments (at +8, capacity +0xB08) from the source
// segment list: start point, direction transformed by the 3x3 at arg2, and length.
void fn_8010B7C8(u8* arg0, u8* arg1, u8* arg2) {
    s32 count = *(s32*)(arg1 + 0x24) - 1;
    *(u8**)(arg0 + 0xB0C) = arg0 + 8;
    *(s32*)(arg0 + 0xB08) = 0x64;
    *(s32*)(arg0 + 0xB04) = -1;
    *(s32*)(arg0 + 0xB00) = -1;
    *(s32*)(arg0 + 0xAFC) = 0;
    if (count > 0) {
        s32 off = 0;
        s32 i;
        for (i = 0; i < count; i++) {
            u8* e;
            s32 tail = *(s32*)(arg0 + 0xB00);
            s32 cap = *(s32*)(arg0 + 0xB08);
            s32 next = (*(s32*)(arg0 + 0xB04) + 1) % cap;
            if (tail == next) {
                e = NULL;   // ring full
            } else {
                *(s32*)(arg0 + 0xB04) = next;
                if (tail < 0) {
                    *(s32*)(arg0 + 0xB00) = next;
                    *(s32*)(arg0 + 0xAF8) = 1;
                }
                s32 head = *(s32*)(arg0 + 0xB04);
                s32 valid = 0;
                if (head >= 0 && head < *(s32*)(arg0 + 0xB08)) {
                    s32 t = *(s32*)(arg0 + 0xB00);
                    s32 lo = head;
                    s32 hi = head;
                    if (head < t) {
                        lo = head + *(s32*)(arg0 + 0xB08);
                        hi = head + *(s32*)(arg0 + 0xB08);
                    }
                    if (t <= lo && lo <= hi) valid = 1;
                }
                e = valid ? *(u8**)(arg0 + 0xB0C) + head * 0x1C : NULL;
            }
            u8* src = *(u8**)arg1 + off;
            f32 px = *(f32*)(src + 0xC);
            f32 py = *(f32*)(src + 0x10);
            f32 pz = *(f32*)(src + 0x14);
            *(f32*)(e + 0x0) = px + *(f32*)(arg2 + 0x24);
            *(f32*)(e + 0x4) = py + *(f32*)(arg2 + 0x28);
            *(f32*)(e + 0x8) = pz + *(f32*)(arg2 + 0x2C);
            f32 dx = *(f32*)(src + 0x34) - *(f32*)(src + 0xC);
            f32 dy = *(f32*)(src + 0x38) - *(f32*)(src + 0x10);
            f32 dz = *(f32*)(src + 0x3C) - *(f32*)(src + 0x14);
            f32 rx = (*(f32*)(arg2 + 0x18) * dz) + ((*(f32*)(arg2 + 0x0) * dx) + (*(f32*)(arg2 + 0xC) * dy));
            *(f32*)(e + 0xC) = rx;
            f32 ry = (*(f32*)(arg2 + 0x1C) * dz) + ((*(f32*)(arg2 + 0x4) * dx) + (*(f32*)(arg2 + 0x10) * dy));
            f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
            *(f32*)(e + 0x10) = ry;
            f32 rz = (*(f32*)(arg2 + 0x20) * dz) + ((*(f32*)(arg2 + 0x8) * dx) + (*(f32*)(arg2 + 0x14) * dy));
            *(f32*)(e + 0x14) = rz;
            f32 len = (len2 <= 0.0f) ? 0.0f : sqrtf(len2);
            off += 0x28;
            *(f32*)(e + 0x18) = len;
        }
    }
    *(s32*)(arg0 + 0x4) = 0;
    *(f32*)(arg0 + 0x0) = 0.0f;
}

// 0x8011C7A0
// Builds the badge inventory list: one 0x1C-byte row per owned badge (name, description,
// category, icon), plus per-category owned/total counters.
void fn_8011C7A0(u8* arg0) {
    s32 i = 0;
    s32 off = 0;
    *(s32*)(arg0 + 0x4) = 3;
    *(s32*)(arg0 + 0x8) = 4;
    *(s32*)(arg0 + 0x12F0) = 0;
    *(s32*)(arg0 + 0x12F4) = 3;
    *(s32*)(arg0 + 0x4) = 1;
    *(s32*)(arg0 + 0x12F8) = 0;
    *(s32*)(arg0 + 0x12FC) = 0;
    *(s32*)(arg0 + 0x1300) = 0;
    *(s32*)(arg0 + 0x1304) = 0;
    *(s32*)(arg0 + 0x1308) = 0;
    *(s32*)(arg0 + 0x130C) = 0;
    *(s32*)(arg0 + 0x1310) = 0;
    *(s32*)(arg0 + 0x1314) = 0;
    while (i < *(s32*)(gGameDb->data + 0x10AC)) {
        u8* ent = *(u8**)(gGameDb->data + 0x10A8) + off;
        s32 id    = *(s32*)(ent + 0x0);
        u32 hash1 = *(u32*)(ent + 0x4);
        u32 hash2 = *(u32*)(ent + 0x8);
        s32 cat   = *(s32*)(ent + 0xC);
        if (SaveHasBadge(lbl_805FBBE0, (u32)i) != 0) {
            u8* row = arg0 + *(s32*)(arg0 + 0x12F0) * 0x1C;
            char path[0x58];
            *(u32*)(row + 0x1C) = hash1;
            *(const char**)(row + 0x20) = BadgeText(TextIdFromHash(hash1));
            *(u32*)(row + 0x24) = hash2;
            *(const char**)(row + 0x28) = BadgeText(TextIdFromHash(hash2));
            *(s32*)(row + 0x2C) = cat;
            u8* rec = *(u8**)(gGameDb->data + 0x10A8) + id * 0x18;
            SysSprintf(path, "%sinventory_badge_%03d", lbl_805F8F50, (int)*(s16*)(rec + 0x14));
            u8* icon = fn_801094F4(path, 0);
            if (icon == NULL) {
                SysSprintf(path, "%sinventory_badge_placeholder", lbl_805F8F50);
                icon = fn_801094F4(path, 0);
            }
            *(u8**)(row + 0x34) = icon;
            if (fn_80109A0C((s32)(intptr_t)icon) != 0) {
                *(u8**)(row + 0x30) = *(u8**)(row + 0x34);
                *(u8**)(row + 0x34) = NULL;
            }
            *(s32*)(arg0 + 0x12F0) = *(s32*)(arg0 + 0x12F0) + 1;
            *(s32*)(arg0 + cat * 8 + 0x12FC) = *(s32*)(arg0 + cat * 8 + 0x12FC) + 1;
            *(s32*)(arg0 + 0x1314) = *(s32*)(arg0 + 0x1314) + 1;
        }
        off += 0x18;
        i++;
        *(s32*)(arg0 + cat * 8 + 0x12F8) = *(s32*)(arg0 + cat * 8 + 0x12F8) + 1;
        *(s32*)(arg0 + 0x1310) = *(s32*)(arg0 + 0x1310) + 1;
    }
    lbl_805FB2A8 = 0;
}

}  // extern "C"

// Width/height (u16 at +4/+6) of the image header: the direct header at +0x144, else the one
// the +0x140 pointer points at, else 0.
static u16 ImageDim(u8* p, s32 off) {
    u8* hdr = *(u8**)(p + 0x144);
    if (hdr != NULL) return *(u16*)(hdr + off);
    u8** ref = *(u8***)(p + 0x140);
    if (ref != NULL) return *(u16*)(*ref + off);
    return 0;
}

extern "C" {

// 0x801A17B4
// Hit-test: maps (farg0, farg1) through the 2x3 matrix at +0xC0..+0xEC to image UV in [0,1),
// converts to a pixel and, when flag 0x20000 is set, asks the image whether that pixel is opaque.
s32 fn_801A17B4(u8* arg0, s32 arg1, f32 farg0, f32 farg1) {
    if (arg1 == 0 && !(*(u32*)(arg0 + 0x64) & 1)) return 0;
    f32 u = *(f32*)(arg0 + 0xE4) + ((*(f32*)(arg0 + 0xD8) * 0.0f) + ((*(f32*)(arg0 + 0xC0) * farg0) + (*(f32*)(arg0 + 0xCC) * farg1)));
    f32 v = *(f32*)(arg0 + 0xE8) + ((*(f32*)(arg0 + 0xDC) * 0.0f) + ((*(f32*)(arg0 + 0xC4) * farg0) + (*(f32*)(arg0 + 0xD0) * farg1)));
    if (u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f) return 0;
    u8* direct = *(u8**)(arg0 + 0x144);
    s32 px = (s32)(u * (f32)(s32)ImageDim(arg0, 4));
    s32 py = (s32)(v * (f32)(s32)ImageDim(arg0, 6));
    if (px < 0 || px >= (s32)ImageDim(arg0, 4)) return 0;
    if (py < 0 || py >= (s32)ImageDim(arg0, 6)) return 0;
    if (!(*(u32*)(arg0 + 0x64) & 0x20000)) return 1;
    u32 pix;
    if (direct != NULL) {
        pix = fn_8015B82C(direct + 4, px, py);
    } else {
        u8** ref = *(u8***)(arg0 + 0x140);
        pix = ref != NULL ? fn_8015B82C(*ref + 4, px, py) : 0;
    }
    u8 opaque, b, c, d;
    u8** ref = *(u8***)(arg0 + 0x140);
    u8* hdr;
    if (ref != NULL && *ref != NULL) {
        hdr = *ref;
    } else {
        hdr = *(u8**)(arg0 + 0x144);
        if (hdr == NULL) hdr = ref != NULL ? *ref : NULL;
    }
    fn_80149828(hdr + 4, &pix, &opaque, &b, &c, &d);
    return opaque != 0;
}

}  // extern "C"
