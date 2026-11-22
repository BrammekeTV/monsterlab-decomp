// Drafts miss654: camera collision sweep, path nearest-segment search, aim camera update,
// flicker timer, UI text element draw, and a post-order tree walk.
#include "types.h"
#include "revolution.h"
#include <math.h>
#include <string.h>

extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFnObjS32)(u8*, s32);
typedef u8* (*VFnObjRetPtr)(u8*);
typedef void (*TreeCallback)(void*, void*);

extern "C" {
void   EntitySyncTransform(void* entity, int a);                  // 0x801B4F4C
void   EulerToQuat(Quat* out, const Vec3* euler);                 // 0x80145B00
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u32    RandomU32(void);                                           // 0x80143460
s32    RaySceneTest(void* ray);                                   // 0x80070458 (ignores its argument)
void   ScalarSet(f32* out, f32 v);                                // 0x80053250
void   Vec3Assign(Vec3* out, const Vec3* v);                      // 0x80053354
void   Vec3Copy(Vec3* out, const Vec3* v);                        // 0x80052640
void   Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);     // 0x80053224
void   Vec3Negate(Vec3* out, const Vec3* v);                      // 0x80053324
void   Vec3NormalizeTo(Vec3* out, const Vec3* v);                 // 0x80055D9C
void   Vec3SetY(Vec3* v, f32 y);
void   Vec3SetZero(Vec3* out);                                    // 0x800531A8
void   Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);          // 0x80053370
void   fn_80054484(Vec3* out, const Vec3* a, const Vec3* b);      // 0x80054484 (a + b)
f32    fn_8005B4A4(const Vec3* a, const Vec3* b);                 // 0x8005B4A4 (dot)
u8*    fn_80073454(void);                                         // 0x80073454 (camera/frustum block)
f32    fn_800734B4(u8* camera, f32* vec);                         // 0x800734B4
void   fn_80073B68(f32* out, f64 t, f32 x, f32 y, u8* cam, f32* nearOut, f32* farOut);  // 0x80073B68
f32    fn_8007DD7C(Vec3* v);                                      // 0x8007DD7C
void   fn_800C09C0(Vec3* out);                                    // 0x800C09C0
s32    fn_800C10D0(u8* arg0, u8* arg1, u8* arg2, u8* arg3, f32 farg0, f32 farg1, f32 farg2);  // 0x800C10D0
void   fn_800C11A4(Vec3* out, const Vec3* a, const Vec3* b);      // 0x800C11A4 (cross product)
f32    fn_800C11F0(f32 x);                                        // 0x800C11F0
f32    fn_800C121C(u8* a3, u8* a4);                               // 0x800C121C
s32    fn_800C124C(u8* arg0, u8* arg1, u8* arg2, u8* arg3, f32* arg4, f32 farg0, f32 farg1);  // 0x800C124C
u8*    fn_800C73EC(u8* h);                                        // 0x800C73EC
u8*    fn_8010C608(u8* path, s32 index);                          // 0x8010C608 (path point by ring index)
void   fn_80146180(void* quat, const Vec3* axis, f32 angle);      // 0x80146180 (rotate quat about axis)
BOOL   fn_80147554(s32 button);                                   // 0x80147554
s32    fn_80156A80(void);                                         // 0x80156A80 (screen width)
s32    fn_80156A88(void);                                         // 0x80156A88 (screen height)
void   fn_8018BEA4(u8* p, f32 t);                                 // 0x8018BEA4
u8*    fn_8018C9DC(void* sets, s32 idx);                          // 0x8018C9DC
void   fn_8018D370(const char* fmt, ...);                         // 0x8018D370 (debug log)
u8*    fn_8019578C(void* obj, const char* str, const f32* mat, s32 a, s32 b, u32 len, s32 c, s32 d, s32 e);  // 0x8019578C
u8*    fn_80196B24(u8* obj);                                      // 0x80196B24
s32    fn_80196E7C(u8* list, u8* font);                           // 0x80196E7C
s32    fn_8019B7D8(void* obj);                                    // 0x8019B7D8
void   fn_801EC080(u8* a, u8* b);                                 // 0x801EC080

extern const char lbl_803816F8[];   // 0x803816F8 (log message)
extern s32 lbl_803472A0[];          // 0x803472A0
extern f32 lbl_803472B0[];          // 0x803472B0
extern f32 lbl_803472C0[];          // 0x803472C0
extern f32 lbl_803472D0[];          // 0x803472D0
extern f32 lbl_803472E0[];          // 0x803472E0
extern f32 lbl_803472F0[];          // 0x803472F0
extern f32 lbl_80347300[];          // 0x80347300
extern f32 lbl_80347310[];          // 0x80347310
extern u8  lbl_803C4298[];          // 0x803C4298
extern f32 lbl_805FE348;            // 0x805FE348 (0.0f)
extern f32 lbl_805FE4B8;            // 0x805FE4B8 (0.0f)
extern f32 lbl_805FE510;            // 0x805FE510 (0.0f)
extern f32 lbl_805FE514;            // 0x805FE514 (1.0f)
extern f32 lbl_805FE518;            // 0x805FE518 (2/pi)
extern f32 lbl_805FE520;            // 0x805FE520
extern f32 lbl_805FE538;            // 0x805FE538
extern f32 lbl_805FE558;            // 0x805FE558 (0.0f)
extern f32 lbl_805FE574;            // 0x805FE574 (2^-32 scale for RandomU32)
extern f32 lbl_805FE578;            // 0x805FE578
extern f32 lbl_805FE57C;            // 0x805FE57C (-1.0f)
extern f32 lbl_805FE580;            // 0x805FE580 (500.0f)
extern f32 lbl_805FE5AC;            // 0x805FE5AC (200.0f)
extern f32 lbl_805FF958;            // 0x805FF958 (0.0f)
extern u8  lbl_805F9212;            // 0x805F9212 (copy filter value)
extern u8  lbl_805FB50C;            // 0x805FB50C (GX shadow flags byte)
extern void* lbl_805FB658;          // 0x805FB658 (global render object)
extern void* lbl_805FB660;          // 0x805FB660 (font list object)
}
extern "C++" void* gEventMgr;       // 0x805FB6C8
extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" s32   gScreenWidth;    // 0x805F9388
extern "C++" s32   gScreenHeight;   // 0x805F938C

extern "C" {

// 0x800C0C30
// Sweeps the moving sphere arg1 (position +0x20, previous +0x14, radius +0x3C) against the two
// point lists hanging off arg0 (list k at +k*4, entry count at +0x40+k*4, entries 8 bytes apart,
// radius at +0x3C). Writes the contact point to arg2 and the contact normal to arg3, sets *arg4
// to 1 for list 0 and 0 for list 1; returns 1 on contact.
s32 fn_800C0C30(u8* arg0, u8* arg1, Vec3* arg2, Vec3* arg3, u8* arg4) {
    Vec3 dir;
    Vec3 edge, edgeDir, perp, perpDir, n0, n1, tmp0, tmp1, rel, relA, relB, diff, pt;
    Vec3 cross, crossN, back, hit, hitN, scaled, scaledPt, offA, offB, towardsA, towardsB;
    Vec3 sweepN, sweepOff, sweepPos, zeroVec, hitRel, hitRelN;
    f32 sc0, sc1, sc2, sc3, sc4, sc5;
    u8 flags[8];
    f32 zero;
    s32 k;
    s32 off;
    u8* list;

    *arg4 = 1;
    Vec3Sub(&dir, (Vec3*)(arg1 + 0x14), (Vec3*)(arg1 + 0x20));
    zero = lbl_805FE348;
    list = arg0;
    off = 0;
    for (k = 0; k < 2; k++, off += 4, list += 4) {
        if (k != 0) {
            *arg4 = 0;
        }
        if (*(s32*)(list + 0x40) >= 2) {
            u8* pairs = arg0 + off;
            u8* ent = pairs;
            s32 i;
            s32 best;
            // flags[i]: segment i passes the sphere test
            for (i = 0; i < 7; i++) {
                flags[i] = 0;
            }
            for (i = 0; i < *(s32*)(list + 0x40) - 1; i++) {
                u8* p0 = *(u8**)(ent + 0x0);
                u8* p1;
                Vec3SetY((Vec3*)(p0 + 0x14), zero);
                Vec3SetY((Vec3*)(*(u8**)(ent + 0x8) + 0x14), zero);
                p1 = *(u8**)(ent + 0x8);
                p0 = *(u8**)(ent + 0x0);
                // r7 (p0) is also still live in the original call, the callee only takes four pointers
                flags[i] = (u8)fn_800C10D0(p0 + 0x14, p1 + 0x14, arg1 + 0x20, (u8*)&dir,
                                           *(f32*)(p0 + 0x3C), *(f32*)(p1 + 0x3C),
                                           *(f32*)(arg1 + 0x3C));
                ent += 8;
            }
            ent = pairs;
            best = -1;
            for (i = 0; i < *(s32*)(list + 0x40) - 1; i++, ent += 8) {
                if (flags[i] == 0) {
                    continue;
                }
                Vec3Sub(&edge, (Vec3*)(*(u8**)(ent + 0x8) + 0x14), (Vec3*)(*(u8**)(ent + 0x0) + 0x14));
                Vec3NormalizeTo(&edgeDir, &edge);
                ScalarSet(&sc0, *(f32*)(*(u8**)(ent + 0x0) + 0x3C));
                fn_800C09C0(&perp);
                fn_800C11A4(&perpDir, &edgeDir, &perp);
                Vec3MulScalar(&offA, &perpDir, &sc0);
                ScalarSet(&sc1, *(f32*)(*(u8**)(ent + 0x8) + 0x3C));
                Vec3Negate(&n0, &edgeDir);
                fn_800C09C0(&n1);
                fn_800C11A4(&tmp0, &n1, &n0);
                Vec3MulScalar(&offB, &tmp0, &sc1);
                fn_80054484(&towardsA, (Vec3*)(*(u8**)(ent + 0x0) + 0x14), &offA);
                fn_80054484(&towardsB, (Vec3*)(*(u8**)(ent + 0x8) + 0x14), &offB);
                Vec3Sub(&cross, &towardsB, &towardsA);
                fn_800C09C0(&tmp1);
                fn_800C11A4(&crossN, &cross, &tmp1);
                Vec3NormalizeTo(&back, &crossN);
                Vec3Copy(arg3, &back);
                Vec3Assign(&relA, (Vec3*)(arg1 + 0x20));
                Vec3Assign(&relB, &towardsA);
                Vec3Sub(&rel, &relA, &relB);
                Vec3Sub(&diff, &relA, &relB);
                {
                    f32 dist = fn_8007DD7C(&diff);
                    f32 d1 = fn_800C11F0(fn_8005B4A4(&cross, &dir));
                    f32 d2 = fn_800C11F0(fn_8005B4A4(&rel, &cross));
                    ScalarSet(&sc2, (dist * d2) / d1);
                    Vec3NormalizeTo(&sweepN, &dir);
                    Vec3MulScalar(&sweepOff, &sweepN, &sc2);
                    fn_80054484(&hit, &relA, &sweepOff);
                    ScalarSet(&sc3, *(f32*)(arg1 + 0x3C) / d1);
                }
                Vec3NormalizeTo(&hitN, &dir);
                Vec3MulScalar(&scaled, &hitN, &sc3);
                Vec3Sub(&pt, &hit, &scaled);
                Vec3Copy(arg2, &pt);
                Vec3Sub(&hitRel, arg2, &towardsA);
                if (fn_800C121C((u8*)&cross, (u8*)&hitRel) < zero) {
                    best = i;
                    continue;
                }
                Vec3Sub(&hitRelN, arg2, &towardsB);
                Vec3Negate(&back, &cross);
                if (fn_800C121C((u8*)&back, (u8*)&hitRelN) < zero) {
                    best = i + 1;
                    continue;
                }
                return 1;
            }
            if (best != -1) {
                u8* hitEnt = *(u8**)(arg0 + best * 8 + off);
                sc4 = zero;
                Vec3SetZero(&zeroVec);
                fn_800C124C(arg1 + 0x20, hitEnt + 0x14, (u8*)&dir, (u8*)&zeroVec, &sc4,
                            *(f32*)(arg1 + 0x3C), *(f32*)(hitEnt + 0x3C));
                ScalarSet(&sc5, sc4);
                Vec3MulScalar(&scaledPt, &dir, &sc5);
                fn_80054484(&sweepPos, (Vec3*)(arg1 + 0x20), &scaledPt);
                Vec3Copy(arg2, &sweepPos);
                Vec3Sub(&tmp0, arg2, (Vec3*)(*(u8**)(arg0 + best * 8 + off) + 0x14));
                Vec3NormalizeTo(&sweepN, &tmp0);
                Vec3Copy(arg3, &sweepN);
                return 1;
            }
        }
    }
    return 0;
}

// 0x800C965C
// Walks the ring-buffer path of arg2 (start +0xB00, end +0xB04, capacity +0xB08) and returns the
// first segment start point whose flattened length is longer than the distance from arg1 to it;
// if none matches, the last point visited (NULL for fewer than two points). Y of the path is ignored.
u8* fn_800C965C(u8* arg0, f32* arg1, u8* arg2) {
    s32 start = *(s32*)(arg2 + 0xB00);
    s32 end = *(s32*)(arg2 + 0xB04);
    s32 count;
    s32 i;
    u8* result;
    f32 zero;
    f32 yd2;

    if (start < 0 || end < 0) {
        count = 0;
    } else {
        s32 e = *(s32*)(arg2 + 0xB04);
        if (e < start) {
            e += *(s32*)(arg2 + 0xB08);
        }
        count = (e - start) + 1;
    }
    result = NULL;
    zero = lbl_805FE4B8;
    yd2 = (zero - zero) * (zero - zero);
    for (i = 0; i < count - 1; i++) {
        u8* p0 = fn_8010C608(arg2, i);
        f32 x0, z0, dx, dz, d2, segLen, ex, ey, ez, e2, dist;
        result = fn_8010C608(arg2, i + 1);
        x0 = *(f32*)(p0 + 0x0);
        z0 = *(f32*)(p0 + 0x8);
        dx = *(f32*)(result + 0x0) - x0;
        dz = *(f32*)(result + 0x8) - z0;
        d2 = dz * dz + (dx * dx + yd2);
        if (d2 <= zero) {
            segLen = zero;
        } else {
            segLen = sqrtf(d2);
        }
        ex = arg1[0] - x0;
        ey = arg1[1] - zero;
        ez = arg1[2] - z0;
        e2 = ez * ez + (ex * ex + ey * ey);
        if (e2 <= zero) {
            dist = zero;
        } else {
            dist = sqrtf(e2);
        }
        if (dist < segLen) {
            return p0;
        }
    }
    return result;
}

// 0x800CCAF4
// Aim camera update: projects the target to the screen, clamps the cursor to the screen range,
// converts it back to a world point and moves the entity at +0x274 there, then re-orients it and
// its linked entity (+0x268) from the Euler angles with the aim angle (+0x7D0), and finally calls
// virtual slot 0x10 and passes its result to fn_8018BEA4 with the cursor Y fraction.
void fn_800CCAF4(u8* arg0, f32 farg0) {
    f32 world[3];
    f32 vec[3];
    f32 w;
    f32 h;
    f32 x;
    f32 y;
    f32 zero;
    f32 extra;
    f32 aim;
    f32 sx;
    f32 sy;
    f32 frac;
    u8* ent;
    u8* xf;
    u8* link;
    Vec3 euler;
    Vec3 axis;
    Quat q;

    if (*(s32*)(arg0 + 0x7C8) == 0) {
        u8* v3;
        *(u8**)(arg0 + 0x7C8) = fn_80073454();
        v3 = *(u8**)(*(u8**)(arg0 + 0x274) + 0x4);
        vec[0] = *(f32*)(v3 + 0x0);
        vec[1] = *(f32*)(v3 + 0x4);
        vec[2] = *(f32*)(v3 + 0x8);
        *(f32*)(arg0 + 0x7CC) = (f32)fn_800734B4(*(u8**)(arg0 + 0x7C8), vec);
    }
    zero = lbl_805FE510;
    {
        f32 sw = lbl_805FE520 * (f32)gScreenWidth;
        f32 sh = lbl_805FE520 * (f32)gScreenHeight;
        f32 c308 = *(f32*)(arg0 + 0x308);
        f32 c30C = *(f32*)(arg0 + 0x30C);
        extra = lbl_805FE538 * (lbl_805FE518 * *(f32*)(arg0 + 0x314));
        sx = sw + sw * (*(f32*)(arg0 + 0x328) * c308);
        sy = sh + sh * (*(f32*)(arg0 + 0x32C) * c30C);
    }
    *(f32*)(arg0 + 0x7B8) = sx;
    *(f32*)(arg0 + 0x7BC) = sy;
    *(f32*)(arg0 + 0x7C0) = zero;

    // clamp cursor X to [0, width] (or [width, 0] when the width is not positive)
    x = *(f32*)(arg0 + 0x7B8);
    w = (f32)fn_80156A80();
    if (zero < w) {
        if (x >= zero) {
            y = (x > w) ? w : x;
        } else {
            y = zero;
        }
    } else {
        if (x >= w) {
            y = (x > zero) ? zero : x;
        } else {
            y = w;
        }
    }
    *(f32*)(arg0 + 0x7B8) = y;

    // clamp cursor Y likewise
    x = *(f32*)(arg0 + 0x7BC);
    h = (f32)fn_80156A88();
    if (zero < h) {
        if (x >= zero) {
            y = (x > h) ? h : x;
        } else {
            y = zero;
        }
    } else {
        if (x >= h) {
            y = (x > zero) ? zero : x;
        } else {
            y = h;
        }
    }
    *(f32*)(arg0 + 0x7BC) = y;

    fn_80073B68(world, (f64)*(f32*)(arg0 + 0x7CC), *(f32*)(arg0 + 0x7B8), y,
                *(u8**)(arg0 + 0x7C8), NULL, NULL);

    if (*(u8*)(arg0 + 0x3C4) != 0) {
        f32 v = *(f32*)(arg0 + 0x3C8);
        if (v > zero) {
            *(f32*)(arg0 + 0x33C) = v;
        }
    }
    if (*(u8*)(arg0 + 0x3C4) != 0) {
        u8* tgt = *(u8**)(arg0 + 0x274);
        if (*(s32*)(tgt + 0x268) != 0) {
            EventSend(gEventMgr, tgt, NULL, 0x617478B3u, 0, 0, 0, 0);
        }
    } else {
        fn_80147554(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0x109B);
    }

    // move the entity's transform to the world point
    ent = *(u8**)(arg0 + 0x274);
    xf = *(u8**)(ent + 0x4);
    *(f32*)(xf + 0x0) = world[0];
    *(f32*)(xf + 0x4) = world[1];
    *(f32*)(xf + 0x8) = world[2];
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x10000000;

    ent = *(u8**)(arg0 + 0x274);
    aim = *(f32*)(arg0 + 0x7D0);
    euler.x = *(f32*)(ent + 0xD4) * lbl_805FE518;
    euler.y = *(f32*)(ent + 0xD8) * lbl_805FE518;
    euler.z = *(f32*)(ent + 0xDC) * lbl_805FE518;
    EulerToQuat(&q, &euler);
    axis.x = zero;
    axis.y = lbl_805FE514;
    axis.z = zero;
    fn_80146180(&q, &axis, aim);
    xf = *(u8**)(ent + 0x4);
    *(f32*)(xf + 0xC) = q.x;
    *(f32*)(xf + 0x10) = q.y;
    *(f32*)(xf + 0x14) = q.z;
    *(f32*)(xf + 0x18) = q.w;
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x10000000;

    if (*(u8**)(ent + 0x268) != NULL) {
        link = fn_800C73EC(*(u8**)(ent + 0x268));
        if (link != NULL) {
            euler.x = *(f32*)(link + 0xD4) * lbl_805FE518;
            euler.y = *(f32*)(link + 0xD8) * lbl_805FE518;
            euler.z = *(f32*)(link + 0xDC) * lbl_805FE518;
            EulerToQuat(&q, &euler);
            axis.x = zero;
            axis.y = lbl_805FE514;
            axis.z = zero;
            fn_80146180(&q, &axis, aim + extra);
            xf = *(u8**)(link + 0x4);
            *(f32*)(xf + 0xC) = q.x;
            *(f32*)(xf + 0x10) = q.y;
            *(f32*)(xf + 0x14) = q.z;
            *(f32*)(xf + 0x18) = q.w;
            *(u32*)(*(u8**)(link + 0x4) + 0x20) |= 0x8000;
            *(u32*)(*(u8**)(link + 0x4) + 0x20) |= 0x10000000;
            *(u32*)(*(u8**)(link + 0x4) + 0x20) |= 0x4000;
            *(u32*)(*(u8**)(link + 0x4) + 0x20) |= 0x10000000;
        }
    }
    EntitySyncTransform(ent, 0);
    fn_801EC080(ent + 0x94, ent);
    frac = *(f32*)(arg0 + 0x7BC);
    frac = frac / (f32)fn_80156A88();
    fn_8018BEA4((*(VFnObjRetPtr*)(*(u8**)arg0 + 0x10))(arg0), frac);
}

// 0x800CFDCC
// Flicker timer: arg0+0 active flag, +4 speed, +8 duration, +0xC elapsed, +0x10 accumulated value.
// While active the value ramps with a fade in/out envelope (scaled by the RaySceneTest-indexed
// table); when the duration is reached it goes idle with a random pause. While idle it waits out
// the pause, then (if the table allows) restarts with a random signed speed and duration.
void fn_800CFDCC(u8* arg0, f32 farg0) {
    f32 dur = *(f32*)(arg0 + 0x8);
    f32 t = *(f32*)(arg0 + 0xC) + farg0;
    *(f32*)(arg0 + 0xC) = t;
    if (t > dur) {
        *(f32*)(arg0 + 0xC) = dur;
    }
    if (*(u8*)(arg0 + 0x0) != 0) {
        f32 speed = *(f32*)(arg0 + 0x4);
        f32 total;
        f32 elapsed;
        f32 edge;
        s32 idx = RaySceneTest(NULL);
        total = *(f32*)(arg0 + 0x8);
        elapsed = *(f32*)(arg0 + 0xC);
        edge = (total * lbl_80347310[idx]) / lbl_805FE5AC;
        if (elapsed < edge) {
            speed = *(f32*)(arg0 + 0x4) * (elapsed / edge);
        } else if (elapsed > (total - edge)) {
            speed = *(f32*)(arg0 + 0x4) * ((total - elapsed) / edge);
        }
        *(f32*)(arg0 + 0x10) = *(f32*)(arg0 + 0x10) + farg0 * speed;
        if (*(f32*)(arg0 + 0xC) >= *(f32*)(arg0 + 0x8)) {
            f32 hi;
            f32 lo;
            *(u8*)(arg0 + 0x0) = 0;
            *(f32*)(arg0 + 0xC) = lbl_805FE558;
            *(f32*)(arg0 + 0x4) = lbl_805FE558;
            hi = lbl_80347300[RaySceneTest(NULL)];
            lo = lbl_803472F0[RaySceneTest(NULL)];
            *(f32*)(arg0 + 0x8) = lo + (hi - lo) * (lbl_805FE574 * (f32)RandomU32());
        }
    } else {
        if (*(f32*)(arg0 + 0xC) >= *(f32*)(arg0 + 0x8)) {
            *(f32*)(arg0 + 0xC) = lbl_805FE558;
            if (lbl_803472A0[RaySceneTest(NULL)] != 0) {
                f32 hi;
                f32 lo;
                *(u8*)(arg0 + 0x0) = 1;
                hi = lbl_803472C0[RaySceneTest(NULL)];
                lo = lbl_803472B0[RaySceneTest(NULL)];
                *(f32*)(arg0 + 0x4) = lbl_805FE578 * (lo + (hi - lo) * (lbl_805FE574 * (f32)RandomU32()));
                if (RandomU32() & 1) {
                    *(f32*)(arg0 + 0x4) = *(f32*)(arg0 + 0x4) * lbl_805FE57C;
                }
                hi = lbl_803472E0[RaySceneTest(NULL)];
                lo = lbl_803472D0[RaySceneTest(NULL)];
                *(f32*)(arg0 + 0x8) = lo + (hi - lo) * (lbl_805FE574 * (f32)RandomU32());
                return;
            }
            *(u8*)(arg0 + 0x0) = 0;
            *(f32*)(arg0 + 0x8) = lbl_805FE580;
        }
    }
}

// 0x8018E928
// Post-order walk of a binary tree (children at +0xC and +0x10): calls cb(node[0], node[4]) for
// each node after its children. Returns -2 (after logging) if node or cb is NULL, else 0.
s32 fn_8018E928(u8* node, TreeCallback cb) {
    u8* left;
    u8* right;

    if (cb == NULL || node == NULL) {
        fn_8018D370(lbl_803816F8);
        return -2;
    }
    left = *(u8**)(node + 0xC);
    if (left != NULL) {
        fn_8018E928(left, cb);
    }
    right = *(u8**)(node + 0x10);
    if (right != NULL) {
        fn_8018E928(right, cb);
    }
    cb(*(void**)(node + 0x0), *(void**)(node + 0x4));
    return 0;
}

// 0x8019D5E8
// Text element draw: composes the element's 4x3 matrix (+0xF0) with the render object's current
// transform into +0x17C when flag 0x10 is set, then draws the string through fn_8019578C and
// stores the returned 0x30-byte record at +0x1B0 (zeros if none). GX FIFO bracketed by BP writes.
s32 fn_8019D5E8(u8* arg0) {
    const char* str;
    u32 len;
    s32 extra;
    u8* rec;

    lbl_805FB50C = (u8)(lbl_805FB50C & ~8);
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0;
    fn_8019B7D8(arg0);
    fn_80196E7C((u8*)lbl_805FB660, fn_8018C9DC(gScreenSets, *(s32*)(arg0 + 0x15C)));
    if (*(s32*)(arg0 + 0x64) & 0x10) {
        u8* a = fn_80196B24((u8*)lbl_805FB658);
#define A(r, c) (*(f32*)(a + (r) * 0xC + (c) * 4))
#define B(r, c) (*(f32*)(arg0 + 0xF0 + (r) * 0xC + (c) * 4))
#define OUT(r, c) (*(f32*)(arg0 + 0x17C + (r) * 0xC + (c) * 4))
        if (a != NULL) {
            // out = B * A (rotation rows), translation row also gets A's translation
            OUT(0, 0) = A(2, 0) * B(0, 2) + (A(0, 0) * B(0, 0) + A(1, 0) * B(0, 1));
            OUT(0, 1) = A(2, 1) * B(0, 2) + (A(0, 1) * B(0, 0) + A(1, 1) * B(0, 1));
            OUT(0, 2) = A(2, 2) * B(0, 2) + (A(0, 2) * B(0, 0) + A(1, 2) * B(0, 1));
            OUT(1, 0) = A(2, 0) * B(1, 2) + (A(0, 0) * B(1, 0) + A(1, 0) * B(1, 1));
            OUT(1, 1) = A(2, 1) * B(1, 2) + (A(0, 1) * B(1, 0) + A(1, 1) * B(1, 1));
            OUT(1, 2) = A(2, 2) * B(1, 2) + (A(0, 2) * B(1, 0) + A(1, 2) * B(1, 1));
            OUT(2, 0) = A(2, 0) * B(2, 2) + (A(0, 0) * B(2, 0) + A(1, 0) * B(2, 1));
            OUT(2, 1) = A(2, 1) * B(2, 2) + (A(0, 1) * B(2, 0) + A(1, 1) * B(2, 1));
            OUT(2, 2) = A(2, 2) * B(2, 2) + (A(0, 2) * B(2, 0) + A(1, 2) * B(2, 1));
            OUT(3, 0) = A(3, 0) + (A(2, 0) * B(3, 2) + (A(0, 0) * B(3, 0) + A(1, 0) * B(3, 1)));
            OUT(3, 1) = A(3, 1) + (A(2, 1) * B(3, 2) + (A(0, 1) * B(3, 0) + A(1, 1) * B(3, 1)));
            OUT(3, 2) = A(3, 2) + (A(2, 2) * B(3, 2) + (A(0, 2) * B(3, 0) + A(1, 2) * B(3, 1)));
        } else {
            s32 i;
            for (i = 0; i < 12; i++) {
                *(f32*)(arg0 + 0x17C + i * 4) = *(f32*)(arg0 + 0xF0 + i * 4);
            }
        }
#undef A
#undef B
#undef OUT
        *(f32*)(arg0 + 0x5C) = *(f32*)(arg0 + 0x1A8);
        (*(VFnObjS32*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x10);
    }
    str = *(const char**)(arg0 + 0x170);
    len = *(u32*)(arg0 + 0x150);
    extra = *(s32*)(arg0 + 0x178);
    if (*(u32*)(arg0 + 0x1EC) > 1U) {
        const char* alt = *(const char**)(arg0 + 0x1E0);
        if (alt != NULL) {
            str = alt;
            len = (u32)strlen(alt);
            extra = *(s32*)(arg0 + 0x1E4);
        }
    }
    rec = fn_8019578C(lbl_805FB658, str, (const f32*)(arg0 + 0x17C), 100000, extra, len,
                      *(s32*)(arg0 + 0x154), *(s32*)(arg0 + 0x158), 0);
    if (rec != NULL) {
        memcpy(arg0 + 0x1B0, rec, 0x30);
    } else {
        s32 i;
        for (i = 0; i < 12; i++) {
            *(f32*)(arg0 + 0x1B0 + i * 4) = lbl_805FF958;
        }
    }
    lbl_805FB50C |= 8;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
    return 0;
}

}  // extern "C"
