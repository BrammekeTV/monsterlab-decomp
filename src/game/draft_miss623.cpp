// Drafts miss623: sphere sweep test (0x800B1AD0), spawn-slot pickers (0x800BC050, 0x800BC388),
// roaming-orb step (0x800BD064) and orb launch (0x800BD984).
#include <math.h>

#include "types.h"

extern "C" {
u32 RandomU32(void);                                                                          // 0x80143460
s32 RaySceneTest(void* ray);                                                                  // 0x80070458
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
s32 fn_800BC704(u8* p, f32* pos);                                                             // 0x800BC704
void fn_800BCC24(u8* obj, f32* pos, u8* ref);                                                 // 0x800BCC24
void fn_800BD6D4(f32* out, f32* ref);                                                         // 0x800BD6D4
void fn_800BE148(f32* out, f32* ref);                                                         // 0x800BE148
void fn_800BD984(u8* obj, f32* pos, f32 radius);                                              // 0x800BD984
void fn_801B6F34(u8* p);                                                                      // 0x801B6F34
void fn_801B7020(u8* p);                                                                      // 0x801B7020

extern s32 lbl_80346E70[];  // 0x80346E70
extern s32 lbl_80346E80[];  // 0x80346E80
extern s32 lbl_80346E90[];  // 0x80346E90
extern f32 lbl_80346EC0[];  // 0x80346EC0
extern f32 lbl_80346EE0[];  // 0x80346EE0
extern f32 lbl_80346EF0[];  // 0x80346EF0
extern f32 lbl_80346F20[];  // 0x80346F20
extern f32 lbl_80346F30[];  // 0x80346F30
extern s32 lbl_80346F40[];  // 0x80346F40
extern s32 lbl_80346F50[];  // 0x80346F50
extern s32 lbl_80346F60[];  // 0x80346F60

extern f32 lbl_805FE0B0;  // 0x805FE0B0 (0.0)
extern f32 lbl_805FE0BC;  // 0x805FE0BC (1.0)
extern f32 lbl_805FE0C4;  // 0x805FE0C4 (small epsilon)
extern f32 lbl_805FE230;  // 0x805FE230 (0.0)
extern f32 lbl_805FE280;  // 0x805FE280 (0.0)
extern f32 lbl_805FE288;  // 0x805FE288 (RandomU32 scale)
extern f32 lbl_805FE28C;  // 0x805FE28C (-1.0)
extern f32 lbl_805FE290;  // 0x805FE290 (2.0)
extern f32 lbl_805FE2C8;  // 0x805FE2C8 (0.0)
extern f32 lbl_805FE2CC;  // 0x805FE2CC (RandomU32 scale)
extern f32 lbl_805FE2D0;  // 0x805FE2D0 (-1.0)
extern f32 lbl_805FE2D4;  // 0x805FE2D4 (2.0)
}

extern "C++" void* gEventMgr;  // 0x805FB6C8

static const u32 kEvtOrbLaunched = 0x3ABE6EA2u;

// Reciprocal square root, refined like the frsqrte + two Newton steps in the original.
static f32 RSqrtRefined(f32 x) { return (f32)(1.0 / sqrt((f64)x)); }

extern "C" {

// 0x800B1AD0
// Sweeps the sphere of arg1 (moved from +0x20 to +0x14, radius +0x3C) against the enabled spheres
// in the lists hanging off arg0+0x8C. Returns 1 and the hit sphere centre in out, else 0 and zeros.
s32 fn_800B1AD0(u8* arg0, u8* arg1, f32* out) {
    f32 mx = *(f32*)(arg1 + 0x14) - *(f32*)(arg1 + 0x20);
    f32 my = *(f32*)(arg1 + 0x18) - *(f32*)(arg1 + 0x24);
    f32 mz = *(f32*)(arg1 + 0x1C) - *(f32*)(arg1 + 0x28);
    u8* grp = *(u8**)(arg0 + 0x8C);
    if (grp != NULL) {
        u32 nlists = *(u32*)(grp + 0x10);
        for (u32 i = 0; i < nlists; i++) {
            u8* list = (*(u8***)(grp + 0xC))[i];
            if (list == NULL) continue;
            u32 n = *(u32*)(list + 0x28);
            for (u32 j = 0; j < n; j++) {
                u8* e = (*(u8***)(list + 0x24))[j];
                if (*(s32*)(e + 0xC) != 0) continue;
                f32 dx = *(f32*)(e + 0x14) - *(f32*)(arg1 + 0x20);
                f32 dy = *(f32*)(e + 0x18) - *(f32*)(arg1 + 0x24);
                f32 dz = *(f32*)(e + 0x1C) - *(f32*)(arg1 + 0x28);
                f32 r = *(f32*)(e + 0x3C) + *(f32*)(arg1 + 0x3C);
                f32 nx = lbl_805FE0B0 - mx;
                f32 ny = lbl_805FE0B0 - my;
                f32 nz = lbl_805FE0B0 - mz;
                f32 c = ((dz * dz) + ((dx * dx) + (dy * dy))) - (r * r);
                f32 t = lbl_805FE0B0;
                s32 ok;
                if (c < lbl_805FE0B0) {
                    ok = 1;  // already overlapping
                } else {
                    f32 a = (nz * nz) + ((nx * nx) + (ny * ny));
                    if (a < lbl_805FE0C4) {
                        ok = 0;
                    } else {
                        f32 b = (nz * dz) + ((nx * dx) + (ny * dy));
                        if (b >= lbl_805FE0B0) {
                            ok = 0;
                        } else {
                            f32 disc = (b * b) - (a * c);
                            if (disc < lbl_805FE0B0) {
                                ok = 0;
                            } else {
                                if (disc != lbl_805FE0B0) t = (f32)sqrt((f64)disc);
                                ok = 1;
                                t = (-b - t) / a;
                            }
                        }
                    }
                }
                if (ok != 0 && t <= lbl_805FE0BC) {
                    if (out != NULL) {
                        out[0] = *(f32*)(e + 0x14);
                        out[1] = *(f32*)(e + 0x18);
                        out[2] = *(f32*)(e + 0x1C);
                    }
                    return 1;
                }
            }
        }
    }
    if (out != NULL) {
        out[0] = lbl_805FE0B0;
        out[1] = lbl_805FE0B0;
        out[2] = lbl_805FE0B0;
    }
    return 0;
}

// 0x800BC050
// Weighted pick among three groups of six slots (+0xDC/+0xF4/+0x10C) for a free one (flag +0x298
// clear); tries up to 10 random positions for it, and on success arms +0x3AC. Returns 1 on success.
s32 fn_800BC050(u8* p) {
    static const s32 kOffs[3] = {0xDC, 0xF4, 0x10C};
    s32 idx = RaySceneTest(p);
    s32 w[3];
    w[0] = lbl_80346E70[idx];
    w[1] = lbl_80346E80[idx];
    w[2] = lbl_80346E90[idx];
    for (;;) {
        s32 w01 = w[0] + w[1];
        u32 total = (u32)(w[2] + w01);
        if (total == 0) return 0;
        s32 r = (s32)(RandomU32() % total);
        s32 g;
        if (r < w[0]) g = 0;
        else if (r < w01) g = 1;
        else if (r < (s32)total) g = 2;
        else continue;
        u8* obj = NULL;
        for (s32 i = 0; i < 6; i++) {
            u8* cand = *(u8**)(p + kOffs[g] + i * 4);
            if (*(u8*)(cand + 0x298) == 0) {
                obj = cand;
                break;
            }
        }
        if (obj == NULL) {
            w[g] = 0;
            continue;
        }
        f32 pos[3];
        pos[2] = lbl_805FE230;
        pos[1] = lbl_805FE230;
        pos[0] = lbl_805FE230;
        for (s32 tries = 0; tries < 10; tries++) {
            f32 tmp[3];
            fn_800BD6D4(tmp, *(f32**)(*(u8**)(p + 0x238) + 4));
            pos[0] = tmp[0];
            pos[1] = tmp[1];
            pos[2] = tmp[2];
            if (fn_800BC704(p, pos) != 0) {
                fn_800BCC24(obj, pos, *(u8**)(p + 0x238));
                *(f32*)(p + 0x3AC) = lbl_80346EC0[idx];
                return 1;
            }
        }
        return 0;
    }
}

// 0x800BC388
// Same idea as fn_800BC050 for three groups of two slots (+0x150/+0x160/+0x158, flag +0x264):
// launches a free one at a random valid position with the radius from lbl_80346EF0, arms +0x3B4.
void fn_800BC388(u8* p) {
    static const s32 kOffs[3] = {0x150, 0x160, 0x158};
    s32 idx = RaySceneTest(p);
    s32 w0 = lbl_80346F40[idx];
    s32 w01 = w0 + lbl_80346F50[idx];
    s32 r = (s32)(RandomU32() % (u32)(lbl_80346F60[idx] + w01));
    s32 g;
    if (r < w0) g = 0;
    else if (r < w01) g = 1;
    else g = 2;
    u8* obj = NULL;
    for (s32 i = 0; i < 2; i++) {
        u8* cand = *(u8**)(p + kOffs[g] + i * 4);
        if (*(u8*)(cand + 0x264) == 0) {
            obj = cand;
            break;
        }
    }
    if (obj == NULL) return;
    f32 radius = lbl_80346EF0[idx];
    f32 pos[3];
    pos[2] = lbl_805FE230;
    pos[1] = lbl_805FE230;
    pos[0] = lbl_805FE230;
    for (s32 tries = 0; tries < 10; tries++) {
        f32 tmp[3];
        fn_800BE148(tmp, *(f32**)(*(u8**)(p + 0x238) + 4));
        pos[0] = tmp[0];
        pos[1] = tmp[1];
        pos[2] = tmp[2];
        if (fn_800BC704(p, pos) != 0) {
            fn_800BD984(obj, pos, radius);
            *(f32*)(p + 0x3B4) = lbl_80346EE0[idx];
            return;
        }
    }
}

// 0x800BD064
// Moves the orb along its velocity (+0x280..0x288, speed +0x270) for dt. If it leaves the sphere
// of radius +0x26C around +0x28C, it is pulled back to the surface and given a new random heading.
void fn_800BD064(u8* p, f32 dt) {
    if (*(u8*)(p + 0x298) == 0) return;
    f32 rad = *(f32*)(p + 0x26C);
    f32 step = dt * *(f32*)(p + 0x270);
    *(f32*)(p + 0x268) = *(f32*)(p + 0x268) + dt;
    f32 vx = *(f32*)(p + 0x280) * step;
    f32 vy = *(f32*)(p + 0x284) * step;
    f32 vz = *(f32*)(p + 0x288) * step;
    f32* pos = *(f32**)(p + 4);
    f32 nx = pos[0] + vx;
    f32 ny = pos[1] + vy;
    f32 nz = pos[2] + vz;
    f32 dx = nx - *(f32*)(p + 0x28C);
    f32 dy = ny - *(f32*)(p + 0x290);
    f32 dz = nz - *(f32*)(p + 0x294);
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    if (len2 > rad * rad) {
        f32 ux, uy, uz;
        if (len2 == lbl_805FE280) {
            ux = lbl_805FE280;
            uy = lbl_805FE280;
            uz = lbl_805FE280;
        } else {
            f32 inv = RSqrtRefined(len2);
            ux = dx * inv;
            uy = dy * inv;
            uz = dz * inv;
        }
        f32 rr = *(f32*)(p + 0x26C);
        nx = *(f32*)(p + 0x28C) + ux * rr;
        ny = *(f32*)(p + 0x290) + uy * rr;
        nz = *(f32*)(p + 0x294) + uz * rr;
        f32 hz = lbl_805FE28C + lbl_805FE290 * (lbl_805FE288 * (f32)RandomU32());
        f32 hy = lbl_805FE28C + lbl_805FE290 * (lbl_805FE288 * (f32)RandomU32());
        f32 hx = lbl_805FE28C + lbl_805FE290 * (lbl_805FE288 * (f32)RandomU32());
        *(f32*)(p + 0x284) = hy;
        *(f32*)(p + 0x288) = hz;
        *(f32*)(p + 0x280) = hx;
        f32 hl2 = (hz * hz) + ((hx * hx) + (hy * hy));
        f32 ox, oy, oz;
        if (hl2 == lbl_805FE280) {
            ox = lbl_805FE280;
            oy = lbl_805FE280;
            oz = lbl_805FE280;
        } else {
            f32 inv = RSqrtRefined(hl2);
            ox = hx * inv;
            oy = hy * inv;
            oz = hz * inv;
        }
        *(f32*)(p + 0x280) = ox;
        *(f32*)(p + 0x284) = oy;
        *(f32*)(p + 0x288) = oz;
    }
    f32* dst = *(f32**)(p + 4);
    dst[0] = nx;
    dst[1] = ny;
    dst[2] = nz;
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x10000000;
    fn_801B7020(p);
}

// 0x800BD984
// Launches the orb at pos with radius, flags it active (+0x264); in mode 1 (+0x270) it also picks
// a random heading (+0x274..0x27C), a velocity toward pos (+0x280..0x288) and the leash centre
// (+0x28C/+0x290). Then sends the launch event and runs the follow-up.
void fn_800BD984(u8* p, f32* pos, f32 radius) {
    f32* dst = *(f32**)(p + 4);
    dst[0] = pos[0];
    dst[1] = pos[1];
    dst[2] = pos[2];
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x10000000;
    *(u8*)(p + 0x264) = 1;
    *(f32*)(p + 0x26C) = radius;
    if (*(s32*)(p + 0x270) == 1) {
        s32 idx = RaySceneTest(p);
        f32 hz = lbl_805FE2D0 + lbl_805FE2D4 * (lbl_805FE2CC * (f32)RandomU32());
        f32 hy = lbl_805FE2D0 + lbl_805FE2D4 * (lbl_805FE2CC * (f32)RandomU32());
        *(f32*)(p + 0x278) = hy;
        *(f32*)(p + 0x27C) = hz;
        f32 hx = lbl_805FE2D0 + lbl_805FE2D4 * (lbl_805FE2CC * (f32)RandomU32());
        *(f32*)(p + 0x274) = hx;
        f32 hl2 = (hz * hz) + ((hx * hx) + (hy * hy));
        f32 ox, oy, oz;
        if (hl2 == lbl_805FE2C8) {
            ox = lbl_805FE2C8;
            oy = lbl_805FE2C8;
            oz = lbl_805FE2C8;
        } else {
            f32 inv = RSqrtRefined(hl2);
            ox = hx * inv;
            oy = hy * inv;
            oz = hz * inv;
        }
        *(f32*)(p + 0x274) = ox;
        *(f32*)(p + 0x278) = oy;
        *(f32*)(p + 0x27C) = oz;
        *(f32*)(p + 0x280) = pos[0];
        *(f32*)(p + 0x284) = pos[1];
        *(f32*)(p + 0x288) = pos[2];
        *(f32*)(p + 0x28C) = lbl_80346F30[idx];
        *(f32*)(p + 0x290) = lbl_80346F20[idx];
    }
    EventSend(gEventMgr, p, NULL, kEvtOrbLaunched, 0, 0, 1, 0);
    fn_801B6F34(p);
}

}  // extern "C"
