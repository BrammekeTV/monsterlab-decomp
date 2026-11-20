// Particle constraint solver and two copies of a 0x1A4-byte view/state block (0x801D region).
#include <math.h>
#include "types.h"

extern "C" {

// 0x801D0DDC
// Constraint relaxation for the particle system stepped by fn_801D0C04. Particles are 0x28 bytes
// (pos +0xC, previous pos +0x0, accel +0x18, pinned flag +0x24); constraints are 0x24 bytes
// (type +8: 1 = distance spring, 2 = point anchor, 4 = accel spring, 5 = sphere collider,
// anything else is skipped). Runs [+0x10] passes over [+0xC] constraints. dt is the caller's
// leftover f1 (fn_801D0C04 passes its own f64 dt through). Returns arg0.
u8* fn_801D0DDC(u8* arg0, f64 dt) {
    if (*(s32*)(arg0 + 0x4) > 0) {
        for (s32 pass = 0; pass < *(s32*)(arg0 + 0x10); pass++) {
            s32 off = 0;
            for (s32 i = 0; i < *(s32*)(arg0 + 0xC); i++, off += 0x24) {
                u8* c = *(u8**)(arg0 + 0x8) + off;
                switch (*(s32*)(c + 0x8)) {
                case 1: {
                    f32 k = *(f32*)(c + 0x10);
                    u8* a = *(u8**)arg0 + *(s32*)(c + 0x0) * 0x28;
                    u8* b = *(u8**)arg0 + *(s32*)(c + 0x4) * 0x28;
                    f32 dx = *(f32*)(b + 0xC) - *(f32*)(a + 0xC);
                    f32 dy = *(f32*)(b + 0x10) - *(f32*)(a + 0x10);
                    f32 dz = *(f32*)(b + 0x14) - *(f32*)(a + 0x14);
                    f32 s = (k / (dz * dz + (dx * dx + dy * dy) + k)) - 0.5f;
                    f32 cx = dx * s;
                    f32 cy = dy * s;
                    f32 cz = dz * s;
                    if (*(u8*)(a + 0x24) != 0) {
                        // a is pinned: b takes the whole correction
                        *(f32*)(b + 0xC) = *(f32*)(b + 0xC) + cx * 2.0f;
                        *(f32*)(b + 0x10) = *(f32*)(b + 0x10) + cy * 2.0f;
                        *(f32*)(b + 0x14) = *(f32*)(b + 0x14) + cz * 2.0f;
                    } else if (*(u8*)(b + 0x24) != 0) {
                        *(f32*)(a + 0xC) = *(f32*)(a + 0xC) - cx * 2.0f;
                        *(f32*)(a + 0x10) = *(f32*)(a + 0x10) - cy * 2.0f;
                        *(f32*)(a + 0x14) = *(f32*)(a + 0x14) - cz * 2.0f;
                    } else {
                        *(f32*)(a + 0xC) = *(f32*)(a + 0xC) - cx;
                        *(f32*)(a + 0x10) = *(f32*)(a + 0x10) - cy;
                        *(f32*)(a + 0x14) = *(f32*)(a + 0x14) - cz;
                        *(f32*)(b + 0xC) = *(f32*)(b + 0xC) + cx;
                        *(f32*)(b + 0x10) = *(f32*)(b + 0x10) + cy;
                        *(f32*)(b + 0x14) = *(f32*)(b + 0x14) + cz;
                    }
                    break;
                }
                case 2: {
                    // pull one particle toward the fixed point at c+0x14
                    f32 tx = *(f32*)(c + 0x14);
                    f32 ty = *(f32*)(c + 0x18);
                    f32 tz = *(f32*)(c + 0x1C);
                    u8* p = *(u8**)arg0 + *(s32*)(c + 0x0) * 0x28;
                    f32 dx = tx - *(f32*)(p + 0xC);
                    f32 dy = ty - *(f32*)(p + 0x10);
                    f32 k = *(f32*)(c + 0x10);
                    f32 dz = tz - *(f32*)(p + 0x14);
                    f32 s = (k / (dz * dz + (dx * dx + dy * dy) + k)) - 0.5f;
                    f32 cx = dx * s;
                    f32 cy = dy * s;
                    f32 cz = dz * s;
                    *(f32*)(p + 0xC) = *(f32*)(p + 0xC) - cx * 2.0f;
                    *(f32*)(p + 0x10) = *(f32*)(p + 0x10) - cy * 2.0f;
                    *(f32*)(p + 0x14) = *(f32*)(p + 0x14) - cz * 2.0f;
                    break;
                }
                case 4: {
                    // spring on the accumulated accel (+0x18) instead of the position
                    u8* a = *(u8**)arg0 + *(s32*)(c + 0x0) * 0x28;
                    u8* b = *(u8**)arg0 + *(s32*)(c + 0x4) * 0x28;
                    f32 dx = *(f32*)(b + 0xC) - *(f32*)(a + 0xC);
                    f32 dy = *(f32*)(b + 0x10) - *(f32*)(a + 0x10);
                    f32 dz = *(f32*)(b + 0x14) - *(f32*)(a + 0x14);
                    if (*(u8*)(a + 0x24) != 0) {
                        *(f32*)(b + 0x18) = *(f32*)(b + 0x18) + dx * 2.0f;
                        *(f32*)(b + 0x1C) = *(f32*)(b + 0x1C) + dy * 2.0f;
                        *(f32*)(b + 0x20) = *(f32*)(b + 0x20) + dz * 2.0f;
                    } else if (*(u8*)(b + 0x24) != 0) {
                        *(f32*)(a + 0x18) = *(f32*)(a + 0x18) - dx * 2.0f;
                        *(f32*)(a + 0x1C) = *(f32*)(a + 0x1C) - dy * 2.0f;
                        *(f32*)(a + 0x20) = *(f32*)(a + 0x20) - dz * 2.0f;
                    } else {
                        *(f32*)(a + 0x18) = *(f32*)(a + 0x18) - dx;
                        *(f32*)(a + 0x1C) = *(f32*)(a + 0x1C) - dy;
                        *(f32*)(a + 0x20) = *(f32*)(a + 0x20) - dz;
                        *(f32*)(b + 0x18) = *(f32*)(b + 0x18) + dx;
                        *(f32*)(b + 0x1C) = *(f32*)(b + 0x1C) + dy;
                        *(f32*)(b + 0x20) = *(f32*)(b + 0x20) + dz;
                    }
                    break;
                }
                case 5: {
                    // sphere collider (center c+0x18, radius c+0xC, strength c+0x10, friction
                    // c+0x14): add an accel pushing the particle out, damped against its velocity
                    u8* p = *(u8**)arg0 + *(s32*)(c + 0x0) * 0x28;
                    f32 dx = *(f32*)(c + 0x18) - *(f32*)(p + 0xC);
                    f32 dy = *(f32*)(c + 0x1C) - *(f32*)(p + 0x10);
                    f32 dz = *(f32*)(c + 0x20) - *(f32*)(p + 0x14);
                    f32 l2 = dz * dz + (dx * dx + dy * dy);
                    f32 len = 0.0f;
                    if (l2 > 0.0f) {
                        len = (f32)sqrt((f64)l2);  // original: frsqrte with two Newton steps
                    }
                    f32 pen = len - *(f32*)(c + 0xC);
                    if (len > 0.01f) {
                        f32 strength = *(f32*)(c + 0x10);
                        f32 qx = (dx / len) * strength;
                        f32 qy = (dy / len) * strength;
                        f32 qz = (dz / len) * strength;
                        f32 mx = qx * pen;
                        f32 my = qy * pen;
                        f32 mz = qz * pen;
                        f32 vx = *(f32*)(p + 0xC) - *(f32*)(p + 0x0);
                        f32 vy = *(f32*)(p + 0x10) - *(f32*)(p + 0x4);
                        f32 vz = *(f32*)(p + 0x14) - *(f32*)(p + 0x8);
                        if (dt > 0.0001f) {
                            f32 inv = 1.0f / (f32)dt;
                            vx = vx * inv;
                            vy = vy * inv;
                            vz = vz * inv;
                            if (vz * vz + (vx * vx + vy * vy) > 0.01f) {
                                f32 fr = *(f32*)(c + 0x14);
                                f32 ax = mx - vx * fr;
                                f32 ay = my - vy * fr;
                                f32 az = mz - vz * fr;
                                f32 dot = az * mz + (ax * mx + ay * my);
                                if (dot > 0.0f) {
                                    f32 mm = mz * mz + (mx * mx + my * my);
                                    f32 aa = az * az + (ax * ax + ay * ay);
                                    if (aa < mm) {
                                        mx = ax;
                                        my = ay;
                                        mz = az;
                                    }
                                } else if (dot < 0.0f) {
                                    mx = 0.0f;
                                    my = 0.0f;
                                    mz = 0.0f;
                                }
                            }
                        }
                        *(f32*)(p + 0x18) = *(f32*)(p + 0x18) + mx;
                        *(f32*)(p + 0x1C) = *(f32*)(p + 0x1C) + my;
                        *(f32*)(p + 0x20) = *(f32*)(p + 0x20) + mz;
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
    return arg0;
}

// 0x801D1BCC
// Copies the 0x1A4-byte block at arg1 into arg0 (field by field in the original).
void fn_801D1BCC(u8* arg0, u8* arg1) {
    for (s32 i = 0; i < 0x1A4; i += 4) {
        *(u32*)(arg0 + i) = *(u32*)(arg1 + i);
    }
}

// 0x801D2388
// Same 0x1A4-byte block copy as fn_801D1BCC.
void fn_801D2388(u8* arg0, u8* arg1) {
    for (s32 i = 0; i < 0x1A4; i += 4) {
        *(u32*)(arg0 + i) = *(u32*)(arg1 + i);
    }
}

}  // extern "C"
