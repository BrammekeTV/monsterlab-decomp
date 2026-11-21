// Camera collision: keeps a segment (from the looked-at point to the camera) out of the
// scenery and of the large objects of the zones.
// Original: 0x800D91A8, 0x800DA8F0, 0x800DAA68 (auto_03_800D91A8_text.s)
//
// The query block (built by the caller, 0x30 bytes): +0x00 start point, +0x0C end point,
// +0x18 radius, +0x1C an angle, +0x20 a flag byte, +0x24..+0x2C three words (+0x28 and +0x2C
// are set to 0 and 2 here). The result block is filled by fn_800D9C24.
#include "types.h"
#include <math.h>

extern "C" {
void fn_80139684(u8* world, f32* centre, u8* result, f32 radius);             // 0x80139684
u32  fn_80143974(s32 seed, void* data, u32 len);                              // 0x80143974 (CRC-32)
void fn_801D7270(u8* view, f32* a, f32* b, f32* c, u32 colour);               // 0x801D7270
f32  fn_800DB708(f32* a, f32* b, u8* e);                                      // 0x800DB708
void fn_800D9C24(u8* query, u8* result);                                      // 0x800D9C24
u8*  GetGlobalSettings(void);                                                 // 0x801D2BB8
extern u8 gWorldState[];           // 0x803E6D00 (first word: world block pointer; +0xB0: zone records)
}
extern "C++" s32 gZoneResourceSize;            // 0x805FB94C
extern u8  lbl_803EA0D0[];         // 0x803EA0D0 (collected triangles, 0x48 bytes each: three corners first)
extern s32 lbl_805FB330;           // 0x805FB330 (their count)
extern u8  lbl_805FB198[];         // 0x805FB198 ([0]: draw the triangles when there are more than 0x40)
extern u8  lbl_805FB1A4[];         // 0x805FB1A4 ([3]: draw the triangles, each in its own colour)
extern u8  lbl_8047C640[];         // 0x8047C640

typedef u8*  (*ObjGetFn)(u8* obj);
typedef BOOL (*ObjBoolFn)(u8* obj);

// 0x800D91A8
// Collects the scenery triangles around a sphere into lbl_803EA0D0 (fn_80139684, with a record
// of 0x6C bytes on the stack: floats cleared, `a` at +0x50, `b` at +0x54, five zero words from
// +0x58). The rest only draws them when one of two debug bytes is set.
extern "C" void fn_800D91A8(f32* centre, s32 a, s32 b, f32 radius) {
    u8 rec[0x6C];
    for (s32 off = 0x00; off < 0x40; off += 4) *(f32*)(rec + off) = 0.0f;
    *(s32*)(rec + 0x50) = a;
    *(s32*)(rec + 0x54) = b;
    for (s32 off = 0x58; off < 0x6C; off += 4) *(s32*)(rec + off) = 0;
    fn_80139684(gWorldState, centre, rec, radius);

    if (lbl_805FB330 > 0x40 && lbl_805FB198[0] != 0) {
        f32* tri = (f32*)lbl_803EA0D0;
        for (s32 i = 0; i < lbl_805FB330; i++, tri += 0x12) {
            f32 p0[3] = { tri[0], tri[1], tri[2] };
            f32 p1[3] = { tri[3], tri[4], tri[5] };
            f32 p2[3] = { tri[6], tri[7], tri[8] };
            fn_801D7270(lbl_8047C640, p0, p1, p2, 0x8000FFFF);
        }
    }
    if (lbl_805FB1A4[3] != 0) {
        f32* tri = (f32*)lbl_803EA0D0;
        for (s32 i = 0; i < lbl_805FB330; i++, tri += 0x12) {
            f32 sum = tri[8] + (tri[7] + (tri[6] + (tri[5] + (tri[4] + (tri[3] + (tri[2] + (tri[0] + tri[1])))))));
            u32 colour = (fn_80143974(0, &sum, 4) & 0x00FFFFFF) | 0x80000000;
            f32 p0[3] = { tri[0], tri[1], tri[2] };
            f32 p1[3] = { tri[3], tri[4], tri[5] };
            f32 p2[3] = { tri[6], tri[7], tri[8] };
            fn_801D7270(lbl_8047C640, p0, p1, p2, colour);
        }
    }
}

// Sum of consecutive words of the global settings, from offset `from` to `to` inclusive.
static inline s32 SettingsSum(s32 from, s32 to) {
    s32 sum = 0;
    for (s32 off = from; off <= to; off += 4) sum += *(s32*)(GetGlobalSettings() + off);
    return sum;
}

// The zone resource with the number `zi`: one of the zones of the level (1..world +4), or one
// in the ranges the global settings give (counts at +0x68..+0x88), else NULL.
static inline u8* ZoneResource(s32 zi) {
    u8* world = *(u8**)gWorldState;
    if (zi > 0 && zi <= *(s32*)(world + 0x4)) {
        return *(u8**)(world + 0x1A4) + zi * gZoneResourceSize;
    }
    if ((*(s32*)(GetGlobalSettings() + 0x68) + 1 <= zi && zi <= SettingsSum(0x68, 0x7C)) ||
        (SettingsSum(0x68, 0x80) + 1 <= zi && zi <= SettingsSum(0x68, 0x84)) ||
        (SettingsSum(0x68, 0x84) + 1 <= zi && zi <= SettingsSum(0x68, 0x88)) ||
        (SettingsSum(0x68, 0x7C) + 1 <= zi && zi <= SettingsSum(0x68, 0x80))) {
        return *(u8**)(*(u8**)gWorldState + 0x1A4) + zi * gZoneResourceSize;
    }
    return NULL;
}

// 0x800DAA68
// Pulls the end point `to` of the segment from..to in front of the large objects it passes
// through. Objects are those of the zone records of kind 6 (as in fn_800DD8B4) with bit
// 0x100000 of the transform flags at +0x20 and bit 0x40 of those at +0x1C set, slot 0x4C
// false, a non-NULL slot 0xC, a radius (+0x14) of 1.5 or more and bit 0x100000 of +0x15C
// clear. For one the segment hits (fn_800DB708 <= 1), `to` becomes the hit point plus 0.02
// along the direction from the object's position to the old `to`. If the world's current
// object (world block +8) then lies more than 0.5 behind that direction and the segment from
// it to the new `to` hits the same object again, `to` is moved to that hit, 1 short of it
// (only when the two are more than 0.001 apart). Returns whether any object was hit.
//
// The original reads the object count through a NULL zone resource when a record names a zone
// that does not exist; such a record is skipped here.
extern "C" BOOL fn_800DAA68(f32* to, f32* from) {
    BOOL hitAny = FALSE;
    for (s32 zr = 0; zr < SettingsSum(0x34, 0x64); zr++) {
        BOOL valid = FALSE;
        if (zr >= 0 && zr < SettingsSum(0x34, 0x64)) valid = TRUE;
        if (!(valid && *(s32*)(*(u8**)(gWorldState + 0xB0) + zr * 0x30 + 0x4) == 6)) {
            continue;
        }
        u8* res = ZoneResource(*(s32*)(*(u8**)(gWorldState + 0xB0) + zr * 0x30 + 0x8));
        if (res == NULL) {
            continue;
        }
        s32 count = *(s32*)(res + 0x98);
        for (s32 k = 0; k < count; k++) {
            u8* e = (*(u8***)(res + 0x12C))[k];
            if (e == NULL) continue;
            if (!(*(u32*)(*(u8**)(e + 0x4) + 0x20) & 0x100000)) continue;
            if (!(*(u32*)(*(u8**)(e + 0x4) + 0x1C) & 0x40)) continue;
            if ((*(ObjBoolFn*)(*(u8**)e + 0x4C))(e)) continue;
            if ((*(ObjGetFn*)(*(u8**)e + 0xC))(e) == NULL) continue;
            if (*(f32*)(e + 0x14) < 1.5f) continue;
            if (*(u32*)(e + 0x15C) & 0x100000) continue;
            f32 t = fn_800DB708(from, to, e);
            if (!(t <= 1.0f)) continue;

            const f32* ePos = *(const f32**)(e + 0x4);
            f32 ax = to[0] - ePos[0];
            f32 ay = to[1] - ePos[1];
            f32 az = to[2] - ePos[2];
            f32 hx = from[0] + (to[0] - from[0]) * t;
            f32 hy = from[1] + (to[1] - from[1]) * t;
            f32 hz = from[2] + (to[2] - from[2]) * t;
            f32 len2 = az * az + (ax * ax + ay * ay);
            f32 nx = 0.0f, ny = 0.0f, nz = 0.0f;
            if (len2 != 0.0f) {
                f32 inv = 1.0f / sqrtf(len2);
                nx = ax * inv;
                ny = ay * inv;
                nz = az * inv;
            }
            f32 tx = hx + nx * 0.02f;
            f32 ty = hy + ny * 0.02f;
            f32 tz = hz + nz * 0.02f;
            to[0] = tx;
            to[1] = ty;
            to[2] = tz;
            u8* cur = *(u8**)(*(u8**)gWorldState + 0x8);
            f32* curPos = *(f32**)(cur + 0x4);
            f32 dx = curPos[0] - tx;
            f32 dy = curPos[1] - ty;
            f32 dz = curPos[2] - tz;
            if (dz * nz + (dx * nx + dy * ny) < -0.5f) {
                f32 t2 = fn_800DB708(curPos, to, e);
                if (t2 <= 1.0f) {
                    const f32* p = *(const f32**)(cur + 0x4);
                    f32 vx = to[0] - p[0];
                    f32 vy = to[1] - p[1];
                    f32 vz = to[2] - p[2];
                    f32 l2 = vz * vz + (vx * vx + vy * vy);
                    f32 len = 0.0f;
                    if (!(l2 <= 0.0f)) len = sqrtf(l2);
                    if (len > 0.001f) {
                        f32 s = t2 - 1.0f / len;
                        p = *(const f32**)(cur + 0x4);
                        f32 x = p[0] + vx * s;
                        f32 y = p[1] + vy * s;
                        f32 z = p[2] + vz * s;
                        to[0] = x;
                        to[1] = y;
                        to[2] = z;
                    }
                }
            }
            hitAny = TRUE;
        }
    }
    return hitAny;
}

// 0x800DA8F0
// One collision query: first the end point is pulled in front of large objects
// (fn_800DAA68), then the triangles around the segment are collected (a sphere at its middle
// with the radius 2 * query radius + half its length) and fn_800D9C24 works out the result.
extern "C" void fn_800DA8F0(u8* query, u8* result) {
    fn_800DAA68((f32*)(query + 0xC), (f32*)query);
    f32 fx = *(f32*)(query + 0x0), fy = *(f32*)(query + 0x4), fz = *(f32*)(query + 0x8);
    f32 tx = *(f32*)(query + 0xC), ty = *(f32*)(query + 0x10), tz = *(f32*)(query + 0x14);
    f32 dx = tx - fx, dy = ty - fy, dz = tz - fz;
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 len = 0.0f;
    if (!(len2 <= 0.0f)) len = sqrtf(len2);
    *(s32*)(query + 0x28) = 0;
    *(s32*)(query + 0x2C) = 2;
    f32 mid[3] = { (fx + tx) * 0.5f, (fy + ty) * 0.5f, (fz + tz) * 0.5f };
    fn_800D91A8(mid, *(s32*)(query + 0x28), *(s32*)(query + 0x2C), 2.0f * *(f32*)(query + 0x18) + 0.5f * len);
    fn_800D9C24(query, result);
}
