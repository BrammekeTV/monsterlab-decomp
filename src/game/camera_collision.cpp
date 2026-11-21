// Camera collision: keeps a segment (from the looked-at point to the camera) out of the
// scenery and of the large objects of the zones.
// Original: 0x800D91A8, 0x800D9410, 0x800D9C24, 0x800DA8F0, 0x800DAA68 (auto_03_800D91A8_text.s)
//
// The query block (built by the caller, 0x30 bytes): +0x00 start point, +0x0C end point,
// +0x18 radius, +0x1C an angle, +0x20 a flag byte, +0x24..+0x2C three words (+0x28 and +0x2C
// are set to 0 and 2 here). The result block (0x28 bytes): +0x00 a byte, set when a triangle
// was touched, +0x04 the unit sum of the push directions, +0x10 the total push, +0x1C the end
// point plus that push. The caller clears it; fn_800D9C24 adds to +0x04 and +0x10.
#include "types.h"
#include <math.h>

extern "C" {
void fn_80139684(u8* world, f32* centre, u8* result, f32 radius);             // 0x80139684
u32  fn_80143974(s32 seed, void* data, u32 len);                              // 0x80143974 (CRC-32)
void fn_801D7270(u8* view, f32* a, f32* b, f32* c, u32 colour);               // 0x801D7270
f32  fn_800DB708(f32* a, f32* b, u8* e);                                      // 0x800DB708
void fn_801D87B4(u8* view, f32* from, f32* dir, u32 colour, s32 a, f32 len, f32 radius);  // 0x801D87B4
f64  fn_8033F1F4(f64 x);                                                      // 0x8033F1F4 (MSL acos)
u8*  GetGlobalSettings(void);                                                 // 0x801D2BB8
extern u8 gWorldState[];           // 0x803E6D00 (first word: world block pointer; +0xB0: zone records)
}
extern "C++" s32 gZoneResourceSize;            // 0x805FB94C
extern u8  lbl_803EA0D0[];         // 0x803EA0D0 (collected triangles, 0x48 bytes each: three corners first)
extern s32 lbl_805FB330;           // 0x805FB330 (their count)
extern u8  lbl_805FB198[];         // 0x805FB198 ([0]: draw the triangles when there are more than 0x40)
extern u8  lbl_805FB1A4[];         // 0x805FB1A4 ([3]: draw the triangles, each in its own colour)
extern u8  lbl_8047C640[];         // 0x8047C640
extern f32 lbl_805FB190;           // 0x805FB190 (radius of the running query)
extern f32 lbl_805FB194;           // 0x805FB194 (the same: the solver works in units of it)
extern f32 lbl_8035FCF8[3];        // 0x8035FCF8 (0, 0, 0.1)

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

// A collected triangle (0x48 bytes), as 18 floats: corners at [0], [3], [6]; written here: unit
// normal at [9] (+0x24), closest point at [12] (+0x30), its distance at [15] (+0x3C, by the
// solver), half the length of the normal before it is made a unit vector at [16] (+0x40).
#define TRI_FLOATS 0x12

static inline f32 Clamp01(f32 t) {
    if (t < 0.0f) return 0.0f;
    if (t <= 1.0f) return t;
    return 1.0f;
}

// 0x800D9410
// Closest point of a triangle to `point`, in units of the query radius (the corners are divided
// by lbl_805FB194; `point` and `out` are in those units). The point is projected onto the
// triangle's plane; when the projection lies outside one of the three edges (tested in the
// order corner 2 -> 1, 1 -> 0, 0 -> 2), the result is the nearest point of that edge to the
// unprojected point instead. Also writes the triangle's unit normal (+0x24) and half the
// length of the edge cross product (+0x40).
extern "C" void fn_800D9410(u8* triRec, f32* point, f32* out) {
    f32* tri = (f32*)triRec;
    f32 s = 1.0f / lbl_805FB194;
    f32 p0x = tri[0] * s, p0y = tri[1] * s, p0z = tri[2] * s;
    f32 p1x = tri[3] * s, p1y = tri[4] * s, p1z = tri[5] * s;
    f32 p2x = tri[6] * s, p2y = tri[7] * s, p2z = tri[8] * s;
    f32 e0x = p1x - p2x, e0y = p1y - p2y, e0z = p1z - p2z;
    f32 e1x = p0x - p1x, e1y = p0y - p1y, e1z = p0z - p1z;
    f32 e2x = p2x - p0x, e2y = p2y - p0y, e2z = p2z - p0z;
    f32 cx = e0y * e1z - e0z * e1y;
    f32 cy = e0z * e1x - e0x * e1z;
    f32 cz = e0x * e1y - e0y * e1x;
    f32 len2 = cz * cz + (cx * cx + cy * cy);
    f32 len = 0.0f;
    f32 nx = 0.0f, ny = 0.0f, nz = 0.0f;
    if (len2 != 0.0f) {
        if (!(len2 <= 0.0f)) len = sqrtf(len2);
        f32 inv = 1.0f / len;
        nx = cx * inv;
        ny = cy * inv;
        nz = cz * inv;
    }
    tri[16] = len * 0.5f;
    tri[9]  = nx;
    tri[10] = ny;
    tri[11] = nz;

    // Inward directions of the three edges, in the plane.
    f32 m0x = ny * e0z - nz * e0y, m0y = nz * e0x - nx * e0z, m0z = nx * e0y - ny * e0x;
    f32 m1x = ny * e1z - nz * e1y, m1y = nz * e1x - nx * e1z, m1z = nx * e1y - ny * e1x;
    f32 m2x = ny * e2z - nz * e2y, m2y = nz * e2x - nx * e2z, m2z = nx * e2y - ny * e2x;

    f32 px = point[0], py = point[1], pz = point[2];
    f32 dist = (nz * pz + (nx * px + ny * py)) + -(nz * p2z + (nx * p2x + ny * p2y));
    f32 qx = px - nx * dist;
    f32 qy = py - ny * dist;
    f32 qz = pz - nz * dist;
    out[0] = qx;
    out[1] = qy;
    out[2] = qz;
    f32 d0 = (m0z * qz + (m0x * qx + m0y * qy)) + -(m0z * p2z + (m0x * p2x + m0y * p2y));
    f32 d1 = (m1z * qz + (m1x * qx + m1y * qy)) + -(m1z * p1z + (m1x * p1x + m1y * p1y));
    f32 d2 = (m2z * qz + (m2x * qx + m2y * qy)) + -(m2z * p0z + (m2x * p0x + m2y * p0y));
    if (d0 < 0.0f) {
        f32 vx = point[0] - p2x, vy = point[1] - p2y, vz = point[2] - p2z;
        f32 t = Clamp01((e0z * vz + (e0x * vx + e0y * vy)) / (e0z * e0z + (e0x * e0x + e0y * e0y)));
        out[0] = e0x * t + p2x;
        out[1] = e0y * t + p2y;
        out[2] = e0z * t + p2z;
    } else if (d1 < 0.0f) {
        f32 vx = point[0] - p1x, vy = point[1] - p1y, vz = point[2] - p1z;
        f32 t = Clamp01((e1z * vz + (e1x * vx + e1y * vy)) / (e1z * e1z + (e1x * e1x + e1y * e1y)));
        out[0] = e1x * t + p1x;
        out[1] = e1y * t + p1y;
        out[2] = e1z * t + p1z;
    } else if (d2 < 0.0f) {
        f32 vx = point[0] - p0x, vy = point[1] - p0y, vz = point[2] - p0z;
        f32 t = Clamp01((e2z * vz + (e2x * vx + e2y * vy)) / (e2z * e2z + (e2x * e2x + e2y * e2y)));
        out[0] = e2x * t + p0x;
        out[1] = e2y * t + p0y;
        out[2] = e2z * t + p0z;
    }
}

// Closest point and distance of every collected triangle to `p` (solver units).
static inline void TrisMeasure(f32 px, f32 py, f32 pz) {
    f32* tri = (f32*)lbl_803EA0D0;
    for (s32 i = 0; i < lbl_805FB330; i++, tri += TRI_FLOATS) {
        f32 p[3] = { px, py, pz };
        fn_800D9410((u8*)tri, p, tri + 12);
        f32 dx = px - tri[12], dy = py - tri[13], dz = pz - tri[14];
        f32 d2 = dz * dz + (dx * dx + dy * dy);
        f32 d = 0.0f;
        if (!(d2 <= 0.0f)) d = sqrtf(d2);
        tri[15] = d;
    }
}

// The nearest triangle (index, or -1); with `facing`, triangles whose normal points along the
// step by 0.1 or more are left out.
static inline s32 TrisNearest(BOOL facing, f32 stepX, f32 stepY, f32 stepZ) {
    f32* tri = (f32*)lbl_803EA0D0;
    f32 best = 1000000000.0f;
    s32 idx = -1;
    for (s32 i = 0; i < lbl_805FB330; i++, tri += TRI_FLOATS) {
        f32 d = tri[15];
        if (d < best) {
            if (facing && stepZ * tri[11] + (stepX * tri[9] + stepY * tri[10]) >= 0.1f) {
                continue;
            }
            best = d;
            idx = i;
        }
    }
    return idx;
}

static inline BOOL TriTouches(s32 idx) {
    return idx >= 0 && ((f32*)lbl_803EA0D0)[idx * TRI_FLOATS + 15] <= 1.0f;
}

// 0x800D9C24
// The solver: moves a sphere of the query radius from the start point to the end point in
// ceil(distance / radius) + 1 positions (at most 0x19) and pushes it out of the collected
// triangles at each one. All of it is done in units of the radius (a unit sphere).
//
// At a position, while the nearest triangle is nearer than 1 and for at most query +0x24
// rounds: the push direction is the unit sum of the normals of all triangles nearer than 1,
// weighted by (1 - distance) * triangle +0x40; the push length is 1.01 * (1 - distance of the
// nearest). When the angle between that direction and the way back to the start, times 2 / pi,
// is not above query +0x1C (and the segment is not a point), the push is made along the way
// back instead, lengthened by 1 / cosine (cosines below 0 or within 0.0001 of it count as 1).
// The push is added to the position, to result +0x10 and its direction to result +0x04.
// With the flag byte (query +0x20) set, triangles that face along the step are not candidates
// for the nearest one; after a push at the last position the flag is not applied.
//
// At the end result +0x04 is made a unit vector, result +0x10 is scaled back to world units,
// result +0x1C is the end point plus that push and result +0 says whether a triangle was
// touched at all. The first part only draws the segment when a debug byte is set.
extern "C" void fn_800D9C24(u8* query, u8* result) {
    lbl_805FB190 = *(f32*)(query + 0x18);
    lbl_805FB194 = *(f32*)(query + 0x18);
    if (lbl_805FB1A4[3] != 0) {
        f32 d[3] = { *(f32*)(query + 0xC) - *(f32*)(query + 0x0), *(f32*)(query + 0x10) - *(f32*)(query + 0x4),
                     *(f32*)(query + 0x14) - *(f32*)(query + 0x8) };
        f32 l2 = d[2] * d[2] + (d[0] * d[0] + d[1] * d[1]);
        f32 l = 0.0f;
        if (l2 == 0.0f) {
            d[0] = 0.0f; d[1] = 0.0f; d[2] = 0.0f;
        } else {
            if (!(l2 <= 0.0f)) l = sqrtf(l2);
            f32 inv = 1.0f / l;
            d[0] *= inv; d[1] *= inv; d[2] *= inv;
        }
        fn_801D87B4(lbl_8047C640, (f32*)query, d, 0x10FF00FF, 1, l, *(f32*)(query + 0x18));
    }

    f32 s = 1.0f / lbl_805FB194;
    f32 curX = *(f32*)(query + 0x0) * s, curY = *(f32*)(query + 0x4) * s, curZ = *(f32*)(query + 0x8) * s;
    f32 stepX = *(f32*)(query + 0xC) * s - curX;
    f32 stepY = *(f32*)(query + 0x10) * s - curY;
    f32 stepZ = *(f32*)(query + 0x14) * s - curZ;
    s32 touched = 0;
    f32 len2 = stepZ * stepZ + (stepX * stepX + stepY * stepY);
    f32 len = 0.0f;
    if (!(len2 <= 0.0f)) len = sqrtf(len2);
    f32 dirX = 0.0f, dirY = 0.0f, dirZ = 0.0f;
    if (len2 != 0.0f) {
        f32 inv = 1.0f / sqrtf(len2);
        dirX = stepX * inv;
        dirY = stepY * inv;
        dirZ = stepZ * inv;
    }
    s32 n = (s32)(f32)ceil((f64)len) + 1;
    if (n > 1) {
        f32 inv = 1.0f / (f32)(n - 1);
        stepX *= inv;
        stepY *= inv;
        stepZ *= inv;
    }
    if (n > 0x19) {
        n = 0x19;
    } else if (n < 0) {
        f32 x = lbl_8035FCF8[0], y = lbl_8035FCF8[1], z = lbl_8035FCF8[2];
        *(f32*)(result + 0x10) = x;
        *(f32*)(result + 0x14) = y;
        *(f32*)(result + 0x18) = z;
        *(f32*)(result + 0x4) = 0.0f;
        *(f32*)(result + 0x8) = 0.0f;
        *(f32*)(result + 0xC) = 1.0f;
        return;
    }
    f32 backX = -dirX, backY = -dirY, backZ = -dirZ;

    while (n-- != 0) {
        TrisMeasure(curX, curY, curZ);
        s32 idx = TrisNearest(*(u8*)(query + 0x20) != 0, stepX, stepY, stepZ);
        BOOL found = TriTouches(idx);
        if (found) touched++;
        s32 rounds = *(s32*)(query + 0x24);
        while (found && rounds-- != 0) {
            f32 ax = 0.0f, ay = 0.0f, az = 0.0f;
            f32* tri = (f32*)lbl_803EA0D0;
            for (s32 i = 0; i < lbl_805FB330; i++, tri += TRI_FLOATS) {
                f32 d = tri[15];
                if (d < 1.0f) {
                    f32 w = (1.0f - d) * tri[16];
                    ax = ax + tri[9] * w;
                    ay = ay + tri[10] * w;
                    az = az + tri[11] * w;
                }
            }
            f32 a2 = az * az + (ax * ax + ay * ay);
            f32 pushX = 0.0f, pushY = 0.0f, pushZ = 0.0f;
            if (a2 != 0.0f) {
                f32 inv = 1.0f / sqrtf(a2);
                pushX = ax * inv;
                pushY = ay * inv;
                pushZ = az * inv;
            }
            f32 cosBack = pushZ * backZ + (pushX * backX + pushY * backY);
            f32 push = 1.01f * (1.0f - ((f32*)lbl_803EA0D0)[idx * TRI_FLOATS + 15]);
            f32 c = cosBack;
            if (cosBack < -1.0f) c = -1.0f;
            else if (cosBack > 1.0f) c = 1.0f;
            f32 angle = (f32)fn_8033F1F4(c);
            if (0.63661975f * angle <= *(f32*)(query + 0x1C)) {
                BOOL point = FALSE;
                if (len > -0.0001f && len < 0.0001f) point = TRUE;
                if (!point) {
                    if (cosBack < 0.0f || (cosBack <= 0.0001f && cosBack >= -0.0001f)) {
                        cosBack = 1.0f;
                    }
                    push = push / cosBack;
                    pushX = -dirX;
                    pushY = -dirY;
                    pushZ = -dirZ;
                }
            }
            f32 mx = pushX * push, my = pushY * push, mz = pushZ * push;
            curX = curX + mx;
            curY = curY + my;
            curZ = curZ + mz;
            *(f32*)(result + 0x10) = *(f32*)(result + 0x10) + mx;
            *(f32*)(result + 0x14) = *(f32*)(result + 0x14) + my;
            *(f32*)(result + 0x18) = *(f32*)(result + 0x18) + mz;
            *(f32*)(result + 0x4) = *(f32*)(result + 0x4) + pushX;
            *(f32*)(result + 0x8) = *(f32*)(result + 0x8) + pushY;
            *(f32*)(result + 0xC) = *(f32*)(result + 0xC) + pushZ;
            TrisMeasure(curX, curY, curZ);
            BOOL facing = FALSE;
            if (n != 0) facing = *(u8*)(query + 0x20) != 0;
            idx = TrisNearest(facing, stepX, stepY, stepZ);
            found = TriTouches(idx);
            if (found) touched++;
        }
        curX = curX + stepX;
        curY = curY + stepY;
        curZ = curZ + stepZ;
    }

    f32 unit = lbl_805FB194;
    f32 sx = *(f32*)(result + 0x4) * unit;
    f32 sy = *(f32*)(result + 0x8) * unit;
    f32 sz = *(f32*)(result + 0xC) * unit;
    *(f32*)(result + 0x4) = sx;
    *(f32*)(result + 0x8) = sy;
    *(f32*)(result + 0xC) = sz;
    f32 s2 = sz * sz + (sx * sx + sy * sy);
    f32 ux = 0.0f, uy = 0.0f, uz = 0.0f;
    if (s2 != 0.0f) {
        f32 inv = 1.0f / sqrtf(s2);
        ux = sx * inv;
        uy = sy * inv;
        uz = sz * inv;
    }
    *(f32*)(result + 0x4) = ux;
    *(f32*)(result + 0x8) = uy;
    *(f32*)(result + 0xC) = uz;
    unit = lbl_805FB194;
    f32 tx = *(f32*)(result + 0x10) * unit;
    f32 ty = *(f32*)(result + 0x14) * unit;
    f32 tz = *(f32*)(result + 0x18) * unit;
    *(f32*)(result + 0x10) = tx;
    *(f32*)(result + 0x14) = ty;
    *(f32*)(result + 0x18) = tz;
    *(u8*)(result + 0x0) = touched > 0;
    *(f32*)(result + 0x1C) = *(f32*)(query + 0xC) + tx;
    *(f32*)(result + 0x20) = *(f32*)(query + 0x10) + ty;
    *(f32*)(result + 0x24) = *(f32*)(query + 0x14) + tz;
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
