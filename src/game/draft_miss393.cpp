// Drafts miss393: scene-object submit loop (0x80183BD0), light-slot selection for point and spot
// lights (0x80185200, 0x8018542C) and the billboard/matrix basis builder (0x801868D4).
#include <math.h>

#include "types.h"

extern "C++" u8 sRendDLs[];   // 0x80401108 (display-list records, 0x94 bytes each)

extern "C" {
void fn_8014DE68(void);                  // 0x8014DE68
void fn_80155724(u8* m);                 // 0x80155724
void fn_801561C0(f32 scale);             // 0x801561C0
s32  fn_8015BA30(void* p);               // 0x8015BA30
void fn_801873C0(void* a, s32 b, s32 c); // 0x801873C0

extern u8  lbl_805F9338;                 // 0x805F9338 (draw enable flag)
extern u8  lbl_80400FF0[];               // 0x80400FF0 (renderer matrix block; floats at +0..+8 = camera vector)
extern f32 lbl_804010D8[12];             // 0x804010D8 (3x4 matrix)
extern s32 lbl_805F920C;                 // 0x805F920C (matrix build flags)
extern s32 lbl_805FB4BC;                 // 0x805FB4BC (current viewport index)
extern s32 lbl_805FB4C0;                 // 0x805FB4C0 (display-list recording flag)
extern s32 lbl_805FB4C8;                 // 0x805FB4C8 (render budget)
}

// Light slot update shared by both light selectors: a slot (0x20 bytes) keeps the three lowest
// attenuation values (+0x10/+0x14/+0x18) and the matching light pointers through +0x1C.
static void InsertLight(u8* slot, f32 v, u8* light) {
    if (v < *(f32*)(slot + 0x18)) {
        f32 second = *(f32*)(slot + 0x14);
        if (v < second) {
            if (v < *(f32*)(slot + 0x10)) {
                *(f32*)(slot + 0x18) = second;
                u8* list = *(u8**)(slot + 0x1C);
                *(u8**)(list + 0x8) = *(u8**)(list + 0x4);
                *(f32*)(slot + 0x14) = *(f32*)(slot + 0x10);
                list = *(u8**)(slot + 0x1C);
                *(u8**)(list + 0x4) = *(u8**)(list + 0x0);
                *(f32*)(slot + 0x10) = v;
                *(u8**)(*(u8**)(slot + 0x1C) + 0x0) = light;
            } else {
                *(f32*)(slot + 0x18) = second;
                u8* list = *(u8**)(slot + 0x1C);
                *(u8**)(list + 0x8) = *(u8**)(list + 0x4);
                *(f32*)(slot + 0x14) = v;
                *(u8**)(*(u8**)(slot + 0x1C) + 0x4) = light;
            }
        } else {
            *(f32*)(slot + 0x18) = v;
            *(u8**)(*(u8**)(slot + 0x1C) + 0x8) = light;
        }
    }
}

extern "C" {

// 0x80183BD0
// Submits `count` scene objects (pointer array): runs the callback or sets up the model/scale
// and draws via fn_801873C0, then flushes the display list if the budget is exceeded.
void fn_80183BD0(u8* arg0, u8** objs, s32 count) {
    if (lbl_805F9338 != 0) {
        *(u8**)(arg0 + 0x3C) = NULL;
        s32 i = 0;
        for (; i < count; i++) {
            u8* o = objs[i];
            if (*(void**)(o + 0x4) == NULL) {
                (*(void (**)(void*))(o + 0x8))(*(void**)(o + 0xC));
            } else {
                u8* m = *(u8**)(o + 0x10);
                if (m != NULL) {
                    if (*(u8**)(arg0 + 0x3C) != m) {
                        *(u8**)(arg0 + 0x3C) = m;
                        fn_80155724(m);
                    }
                } else {
                    *(u8**)(arg0 + 0x3C) = NULL;
                    fn_801561C0(*(f32*)(o + 0x18));
                }
                u8* t = *(u8**)(o + 0xC);
                *(f32*)(lbl_80400FF0 + 0x0) = *(f32*)(t + 0x24);
                *(f32*)(lbl_80400FF0 + 0x4) = *(f32*)(t + 0x28);
                *(f32*)(lbl_80400FF0 + 0x8) = *(f32*)(t + 0x2C);
                fn_801873C0(*(void**)(o + 0x4), *(s32*)(o + 0x14), *(s32*)(*(u8**)(o + 0xC) + 0xC) & 0x80);
            }
        }
        if (lbl_805FB4C0 != 0 && fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0 > lbl_805FB4C8) {
            fn_8014DE68();
        }
    }
}

// 0x80185200
// Point-light selection: for `n` slots starting at `first`, computes the attenuation of light
// `light` (position +0x20, coefficients +0x2C: const/linear/quadratic) at the slot position
// and inserts it into the slot's three-best list. Flags (s16 at +0x12): bit0/bit1 mask the
// slot kind, bit3 forces zero attenuation.
void fn_80185200(u8* arg0, u8* light, s32 first, s32 n) {
    f32 cx = *(f32*)(light + 0x20);
    f32 cy = *(f32*)(light + 0x24);
    f32 cz = *(f32*)(light + 0x28);
    f32 ka = *(f32*)(light + 0x2C);
    f32 kb = *(f32*)(light + 0x30);
    f32 kc = *(f32*)(light + 0x34);
    s16 flags = *(s16*)(light + 0x12);
    if (ka == 0.0f && kb == 0.0f && kc == 0.0f) {
        return;
    }
    u8* slot = *(u8**)(arg0 + 0x60) + (first << 5);
    s32 i = 0;
    for (; i < n; i++, slot += 0x20) {
        s32 mask = (*(s32*)(slot + 0xC) != 0) ? (flags & 1) : (flags & 2);
        if (mask != 0) {
            continue;
        }
        f32 dx = *(f32*)(slot + 0x0) - cx;
        f32 dy = *(f32*)(slot + 0x4) - cy;
        f32 dz = *(f32*)(slot + 0x8) - cz;
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 d;
        if (len2 <= 0.0f) {
            d = 0.0f;
        } else {
            d = sqrtf(len2);   // original: frsqrte with two Newton steps
        }
        f32 v = 0.0f;
        if ((flags & 8) == 0) {
            v = ka + d * kb + kc * (d * d);
        }
        InsertLight(slot, v, light);
    }
}

// 0x8018542C
// Spot-light selection: like fn_80185200 but with a cone test. The cone axis is the negated
// direction at +0x38, the half-angle is (pi/2 * +0x14); a slot is tested through its bounding
// sphere (x, y, z, radius at arg0[+0x64] + 0x10 * index) against the cone, and lit slots use the
// attenuation at the slot position.
void fn_8018542C(u8* arg0, u8* light, s32 first, s32 n) {
    f32 cx = *(f32*)(light + 0x20);
    f32 cy = *(f32*)(light + 0x24);
    f32 cz = *(f32*)(light + 0x28);
    f32 ka = *(f32*)(light + 0x2C);
    f32 kb = *(f32*)(light + 0x30);
    f32 kc = *(f32*)(light + 0x34);
    s16 flags = *(s16*)(light + 0x12);
    if (ka == 0.0f && kb == 0.0f && kc == 0.0f) {
        return;
    }
    f32 ang = 1.5707964f * *(f32*)(light + 0x14);
    f32 cosA = cosf(ang);
    f32 sinA = sinf(ang);
    f32 cos2 = cosA * cosA;
    f32 invSin = 1.0f / sinA;
    f32 sin2 = sinA * sinA;
    f32 d0 = -*(f32*)(light + 0x38);
    f32 d1 = -*(f32*)(light + 0x3C);
    f32 d2 = -*(f32*)(light + 0x40);
    u8* slot = *(u8**)(arg0 + 0x60) + (first << 5);
    u8* sph = *(u8**)(arg0 + 0x64) + (first << 4);
    s32 i = 0;
    for (; i < n; i++, slot += 0x20, sph += 0x10) {
        s32 mask = (*(s32*)(slot + 0xC) != 0) ? (flags & 1) : (flags & 2);
        if (mask != 0) {
            continue;
        }
        f32 r = *(f32*)(sph + 0xC);
        f32 t = r * invSin;
        f32 ox = *(f32*)(sph + 0x0) - cx;
        f32 oy = *(f32*)(sph + 0x4) - cy;
        f32 oz = *(f32*)(sph + 0x8) - cz;
        f32 vx = ox + d0 * t;
        f32 vy = oy + d1 * t;
        f32 vz = oz + d2 * t;
        f32 dot = vz * d2 + (vx * d0 + vy * d1);
        f32 len2 = vz * vz + (vx * vx + vy * vy);
        bool lit;
        if (dot > 0.0f && dot * dot >= len2 * cos2) {
            f32 dot2 = oz * d2 + (ox * d0 + oy * d1);
            f32 olen2 = oz * oz + (ox * ox + oy * oy);
            if (dot2 < 0.0f && dot2 * dot2 >= olen2 * sin2) {
                lit = olen2 <= r * r;
            } else {
                lit = true;
            }
        } else {
            lit = false;
        }
        if (!lit) {
            continue;
        }
        f32 dx = *(f32*)(slot + 0x0) - cx;
        f32 dy = *(f32*)(slot + 0x4) - cy;
        f32 dz = *(f32*)(slot + 0x8) - cz;
        f32 slen2 = dz * dz + (dx * dx + dy * dy);
        f32 d;
        if (slen2 <= 0.0f) {
            d = 0.0f;
        } else {
            d = sqrtf(slen2);   // original: frsqrte with two Newton steps
        }
        f32 v = ka + d * kb + kc * (d * d);
        InsertLight(slot, v, light);
    }
}

// 0x801868D4
// Builds a 3x4 basis into arg0 from lbl_805F920C: bit 0 = camera-facing basis from the 3x4
// matrix and the camera vector (else identity), bit 1 = +1 in z, bit 2 = scale (3,3,4) with a
// -3.14 z offset.
void fn_801868D4(u8* arg0) {
    f32* m = lbl_804010D8;
    f32* o = (f32*)arg0;
    if (lbl_805F920C & 1) {
        f32 c0 = *(f32*)(lbl_80400FF0 + 0x0);
        f32 c1 = *(f32*)(lbl_80400FF0 + 0x4);
        f32 c2 = *(f32*)(lbl_80400FF0 + 0x8);
        f32 x = m[9] + (m[6] * c2 + (m[0] * c0 + m[3] * c1));
        f32 y = m[10] + (m[7] * c2 + (m[1] * c0 + m[4] * c1));
        f32 z = -(m[11] + (m[8] * c2 + (m[2] * c0 + m[5] * c1)));
        f32 nx, ny, nz;
        f32 len2 = z * z + (x * x + y * y);
        if (len2 == 0.0f) {
            nx = 0.0f;
            ny = 0.0f;
            nz = 0.0f;
        } else {
            f32 r = (f32)(1.0 / sqrt((f64)len2));   // original: frsqrte with two Newton steps
            nx = x * r;
            ny = y * r;
            nz = z * r;
        }
        o[6] = nx;
        o[7] = ny;
        o[8] = nz;
        // w = n x up (up = +Z), normalised
        f32 ax = 0.0f * nx - 0.0f * nz;
        f32 ay = 1.0f * nz - 0.0f * ny;
        f32 az = 0.0f * ny - 1.0f * nx;
        f32 wx, wy, wz;
        len2 = az * az + (ay * ay + ax * ax);
        if (len2 == 0.0f) {
            wx = 0.0f;
            wy = 0.0f;
            wz = 0.0f;
        } else {
            f32 r = (f32)(1.0 / sqrt((f64)len2));
            wx = ay * r;
            wy = ax * r;
            wz = az * r;
        }
        o[0] = wx;
        o[1] = wy;
        o[2] = wz;
        // u = n x w, normalised
        f32 bx = ny * wz - nz * wy;
        f32 by = nz * wx - nx * wz;
        f32 bz = nx * wy - ny * wx;
        f32 ux, uy, uz;
        len2 = bz * bz + (bx * bx + by * by);
        if (len2 == 0.0f) {
            ux = 0.0f;
            uy = 0.0f;
            uz = 0.0f;
        } else {
            f32 r = (f32)(1.0 / sqrt((f64)len2));
            ux = bx * r;
            uy = by * r;
            uz = bz * r;
        }
        o[3] = ux;
        o[4] = uy;
        o[5] = uz;
        // first row = u x n
        o[0] = uy * nz - uz * ny;
        o[1] = uz * nx - ux * nz;
        o[2] = ux * ny - uy * nx;
        o[9] = 0.0f;
        o[10] = 0.0f;
        o[11] = 0.0f;
    } else {
        o[0] = 1.0f;
        o[1] = 0.0f;
        o[2] = 0.0f;
        o[3] = 0.0f;
        o[4] = 1.0f;
        o[5] = 0.0f;
        o[6] = 0.0f;
        o[7] = 0.0f;
        o[8] = 1.0f;
        o[9] = 0.0f;
        o[10] = 0.0f;
        o[11] = 0.0f;
    }
    if (lbl_805F920C & 2) {
        o[9] = o[9] + 0.0f;
        o[10] = o[10] + 0.0f;
        o[11] = o[11] + 1.0f;
    }
    if (lbl_805F920C & 4) {
        f32 tx = o[9] * 3.0f;
        f32 ty = o[10] * 3.0f;
        f32 tz = o[11] * 4.0f;
        o[9] = tx;
        o[10] = ty;
        o[11] = tz;
        o[0] = o[0] * 3.0f;
        o[1] = o[1] * 3.0f;
        o[2] = o[2] * 4.0f;
        o[3] = o[3] * 3.0f;
        o[4] = o[4] * 3.0f;
        o[5] = o[5] * 4.0f;
        o[6] = o[6] * 3.0f;
        o[7] = o[7] * 3.0f;
        o[8] = o[8] * 4.0f;
        o[9] = tx + 0.0f;
        o[10] = ty + 0.0f;
        o[11] = tz + -3.14f;
    }
}

}  // extern "C"
