// Missing functions batch 635: debug-line ring buffer fill, world ray sweep, scene-tree walk,
// bone-attached transform helper, sphere/point approach solver, memory-card style init wait loop.
// Every body is checked against the asm (sh tools/dump.sh).
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"

extern "C" {
u8*  GetGlobalSettings(void);                                       // 0x801D2BB8
void Vec3AddTo(Vec3* v, const Vec3* d);                             // 0x800544FC
void Vec3Copy(Vec3* out, const Vec3* v);                            // 0x80052640
void Vec3Rotate(Vec3* out, const f32 m[9], const Vec3* v);          // 0x8005844C
void fn_800526D0(u8* out);                                          // 0x800526D0 (matrix identity)
u8*  fn_800526D4(u8* mtx);                                          // 0x800526D4 (matrix translation)
void fn_8005535C(u8* dst, const u8* src);                           // 0x8005535C
f32  fn_800553C0(u8* mtx, const Vec3* v);                           // 0x800553C0
void fn_800553DC(f32* q, u8* mtx);                                  // 0x800553DC (quat to matrix)
void fn_800554DC(u8* out, const u8* a, const u8* b);                // 0x800554DC
void fn_800557B4(u8* out);                                          // 0x800557B4
void fn_80055828(f32* out, u8* mtx, const Vec3* v);                 // 0x80055828
void fn_8016BB04(u8* shape, f32* a, f32* b, void* cb, u8* q);       // 0x8016BB04
void fn_8016C2AC(u8* shape, f32* a, f32* b, void* cb, u8* q);       // 0x8016C2AC
s32  fn_80138FB8(u8* arg0, u8* arg1);                               // 0x80138FB8
s32  fn_8018BDE0(u8* p, s32 key);                                   // 0x8018BDE0
u8*  fn_8018C39C(u8* p, s32 idx);                                   // 0x8018C39C
void* fn_801DA798(void* p);                                         // 0x801DA798
u8*  fn_801ACF80(u8* obj, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7);                                          // 0x801ACF80 (vtable thunk, slot 0x10; returns a pointer)
void fn_801ACF90(u8* arg0, u8* arg1);                               // 0x801ACF90

u8*  fn_80142500(void* p);                                          // 0x80142500
f32  TimerReset(u8* timer);                                         // 0x80142744
f32  TimerGetSeconds(void* timer);                                  // 0x80142538
void OSSleepTicks(s32 hi, u32 lo);                                   // 0x8023D3C8 (sleep ticks)
void fn_80207BE4(s32 flag);                                         // 0x80207BE4
s32  fn_802FAC0C(u8* p);                                            // 0x802FAC0C
void fn_802FABD4(u8* p);                                            // 0x802FABD4
void fn_802E8FD4(s32 a, u8* buf, s32 b, const void* str, void* c, s32 d, s32 e, u8* buf2, s32 f);  // 0x802E8FD4
s32  fn_802E9668(s16* name, s32 a, void* cb, s32 b);                // 0x802E9668
void fn_802E93CC(void);                                             // 0x802E93CC
s32  fn_802EA048(void);                                             // 0x802EA048
s32  fn_802E48B8(s32* a, s32* b);                                   // 0x802E48B8
void DWC_ClearError(void);                                             // 0x802E4970

extern u8  gWorldState[];           // 0x803E6D00
extern s32 lbl_805FB30C;            // 0x805FB30C (world object count)
extern s32 lbl_805FB36C;            // 0x805FB36C
extern s32 lbl_805FB374;            // 0x805FB374
extern s32 lbl_805FB380;            // 0x805FB380
extern u8  lbl_80556E80[];          // player data block
extern u8  lbl_80386ED0[];          // 0x80386ED0 (debug string block)
extern u8  lbl_805F9950;            // 0x805F9950
extern f32 lbl_80600478;            // 0x80600478 (timeout, seconds)
extern f32 lbl_805FEC70;            // 0x805FEC70 (0.0f)
extern f32 lbl_805FEF6C;            // 0x805FEF6C (0.0f)
extern f32 lbl_805FEF70;            // 0x805FEF70 (+0.0001f)
extern f32 lbl_805FEF74;            // 0x805FEF74 (-0.0001f)
extern f32 lbl_805FEF78;            // 0x805FEF78 (1.1f, initial hit fraction)
extern f32 lbl_805FEF7C;            // 0x805FEF7C (200.0f, ray length)
extern f32 lbl_805FFAB8;            // 0x805FFAB8 (0.0f)
extern f32 lbl_805FFAC8;            // 0x805FFAC8 (+epsilon)
extern f32 lbl_805FFAE0;            // 0x805FFAE0 (-epsilon)

// 0x8010B5A0
// Fills the 100-entry ring of 0x1C-byte segment records at arg0+8 from the segment list in arg1:
// each entry gets the start point, the start-to-end delta and its length. Returns arg0.
// (The original square root is a frsqrte + two Newton steps; sqrtf matches to float precision.)
u8* fn_8010B5A0(u8* arg0, u8* arg1) {
    s32 n = *(s32*)(arg1 + 0x24) - 1;
    s32 off = 0;
    *(s32*)(arg0 + 0xB08) = 100;
    *(u8**)(arg0 + 0xB0C) = arg0 + 8;
    *(s32*)(arg0 + 0xB04) = -1;
    *(s32*)(arg0 + 0xB00) = -1;
    *(s32*)(arg0 + 0xAFC) = 0;
    if (n > 0) {
        do {
            // Ring push (inlined): advance the tail unless the ring is full.
            u8* slot;
            s32 head = *(s32*)(arg0 + 0xB00);
            s32 cap = *(s32*)(arg0 + 0xB08);
            s32 next = (*(s32*)(arg0 + 0xB04) + 1) % cap;
            if (head == next) {
                slot = NULL;
            } else {
                *(s32*)(arg0 + 0xB04) = next;
                if (head < 0) {
                    *(s32*)(arg0 + 0xB00) = next;
                    *(s32*)(arg0 + 0xAF8) = 1;
                }
                s32 idx = *(s32*)(arg0 + 0xB04);
                s32 ok = 0;
                if (idx >= 0 && idx < *(s32*)(arg0 + 0xB08)) {
                    s32 h = *(s32*)(arg0 + 0xB00);
                    s32 lo = idx;
                    s32 hi = idx;
                    if (idx < h) {
                        lo = idx + *(s32*)(arg0 + 0xB08);
                        hi = idx + *(s32*)(arg0 + 0xB08);
                    }
                    if (!(h > lo) && !(lo > hi)) ok = 1;
                }
                if (ok) {
                    slot = *(u8**)(arg0 + 0xB0C) + idx * 0x1C;
                } else {
                    slot = NULL;
                }
            }
            u8* seg = *(u8**)arg1 + off;
            f32 px = *(f32*)(seg + 0xC);
            f32 py = *(f32*)(seg + 0x10);
            f32 pz = *(f32*)(seg + 0x14);
            *(f32*)(slot + 0x0) = px;
            *(f32*)(slot + 0x4) = py;
            *(f32*)(slot + 0x8) = pz;
            f32 dx = *(f32*)(seg + 0x34) - *(f32*)(seg + 0xC);
            f32 dy = *(f32*)(seg + 0x38) - *(f32*)(seg + 0x10);
            f32 dz = *(f32*)(seg + 0x3C) - *(f32*)(seg + 0x14);
            *(f32*)(slot + 0xC) = dx;
            *(f32*)(slot + 0x10) = dy;
            *(f32*)(slot + 0x14) = dz;
            f32 len2 = dz * dz + (dx * dx + dy * dy);
            f32 len;
            if (len2 <= lbl_805FEC70) {
                len = lbl_805FEC70;
            } else {
                len = (f32)sqrt((f64)len2);
            }
            off += 0x28;
            *(f32*)(slot + 0x18) = len;
            n -= 1;
        } while (n != 0);
    }
    *(s32*)(arg0 + 0x4) = 0;
    *(f32*)(arg0 + 0x0) = lbl_805FEC70;
    return arg0;
}

// 0x80139140
// Sweeps a 200-unit ray (start arg1, direction arg2) against the shape of every live world object
// whose slot is inside the settings object-count range and whose record state is 6. Keeps the
// nearest hit (fraction below 1.1) in arg3 and returns 1 when something was hit. arg0 and arg4
// are not used by the asm; arg5 selects the second shape query.
s32 fn_80139140(void* arg0, f32* arg1, f32* arg2, u8* arg3, s32 arg4, s32 arg5) {
    u8 hit[0x50];
    s32 found = 0;
    f32 z = lbl_805FEF6C;
    f32 dx = arg2[0];

    *(f32*)(hit + 0x0) = z;
    *(f32*)(hit + 0x4) = z;
    *(f32*)(hit + 0x8) = z;
    *(f32*)(hit + 0xC) = z;
    *(f32*)(hit + 0x10) = z;
    *(f32*)(hit + 0x14) = z;
    *(f32*)(hit + 0x18) = lbl_805FEF78;

    // Zero-length direction: nothing to sweep.
    s32 zero = 0;
    if (dx <= lbl_805FEF70 && dx >= lbl_805FEF74 &&
        arg2[1] <= lbl_805FEF70 && arg2[1] >= lbl_805FEF74 &&
        arg2[2] <= lbl_805FEF70 && arg2[2] >= lbl_805FEF74) {
        zero = 1;
    }
    if (zero != 0) {
        return 0;
    }

    for (s32 i = 0; i < lbl_805FB30C; i++) {
        if (*(s32*)(*(u8**)(gWorldState + 0xD4) + i * 4) == 0) continue;
        s32 inRange = 0;
        if (i >= 0) {
            s32 sum = *(s32*)(GetGlobalSettings() + 0x4C);
            sum = *(s32*)(GetGlobalSettings() + 0x34) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x50) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x54) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x58) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x5C) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x60) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x64) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x38) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x3C) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x40) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x44) + sum;
            sum = *(s32*)(GetGlobalSettings() + 0x48) + sum;
            if (i < sum) inRange = 1;
        }
        s32 isSix = 0;
        if (inRange != 0) {
            isSix = *(s32*)(*(u8**)(gWorldState + 0xB0) + i * 0x30 + 0x4) == 6;
        }
        if (isSix == 0) continue;

        u8* obj = *(u8**)(*(u8**)(gWorldState + 0xD4) + i * 4);
        u8* shape = *(u8**)(*(u8**)(obj + 0x4C) + 0x1C);
        if (shape == NULL) continue;

        f32 ex = arg2[0] * lbl_805FEF7C;
        f32 ey = arg2[1] * lbl_805FEF7C;
        f32 ez = arg2[2] * lbl_805FEF7C;
        f32 end[3];
        end[0] = arg1[0] + ex;
        end[1] = arg1[1] + ey;
        end[2] = arg1[2] + ez;
        if (arg5 != 0) {
            fn_8016C2AC(shape, arg1, end, (void*)fn_80138FB8, hit);
        } else {
            fn_8016BB04(shape, arg1, end, (void*)fn_80138FB8, hit);
        }
        f32 frac = *(f32*)(hit + 0x18);
        if (frac < lbl_805FEF78) {
            found = 1;
            if (arg3 != NULL) {
                *(f32*)(arg3 + 0x0) = *(f32*)(hit + 0x0);
                *(f32*)(arg3 + 0x4) = *(f32*)(hit + 0x4);
                *(f32*)(arg3 + 0x8) = *(f32*)(hit + 0x8);
                *(f32*)(arg3 + 0xC) = *(f32*)(hit + 0xC);
                *(f32*)(arg3 + 0x10) = *(f32*)(hit + 0x10);
                *(f32*)(arg3 + 0x14) = *(f32*)(hit + 0x14);
                *(f32*)(arg3 + 0x18) = *(f32*)(hit + 0x18);
                *(f32*)(arg3 + 0x18) = lbl_805FEF7C * *(f32*)(hit + 0x18);  // fraction -> distance
                *(s32*)(arg3 + 0x48) = *(s32*)(hit + 0x48);
            }
        }
    }
    return found;
}

// 0x8019C174
// Walks the child list (first child at +0x120, next sibling at +0x124) of obj. pre(node, ctx)
// returns 0 to descend into the node's children, -7 to skip them, anything else (-6, ...) to
// stop walking this list; post(node, ctx) runs after each visited node (not after a stop).
// The original is the same recursion unrolled four levels deep.
void fn_8019C174(u8* obj, s32 (*pre)(u8*, void*), s32 (*post)(u8*, void*), void* ctx) {
    for (u8* node = *(u8**)(obj + 0x120); node != NULL; node = *(u8**)(node + 0x124)) {
        s32 r = pre(node, ctx);
        if (r == 0) {
            fn_8019C174(node, pre, post, ctx);
        } else if (r != -7) {
            return;
        }
        post(node, ctx);
    }
}

// 0x801ACD08
// Builds the world transform arg2 for the attachment object arg0 at point arg1: when the owner
// (+0x10) has a bone cache the bone matrix (looked up once by the hash at +0x30 and cached at +0x34)
// is applied to arg1 and composed with the owner's matrix; otherwise a transform from the owner's
// position plus the rotated offset is built. The two float arguments are not used by the asm.
void fn_801ACD08(u8* arg0, u8* arg1, u8* arg2, f32 farg0, f32 farg1) {
    u8 m1E0[0x30];
    u8 m1B0[0x30];
    u8 m180[0x30];
    u8 m150[0x30];
    u8 m120[0x30];
    u8 mF0[0x30];
    u8 mC0[0x30];
    u8 m90[0x30];
    u8 m60[0x30];
    u8 m30[0x30];
    Vec3 v20;
    Vec3 v14;
    Vec3 v8;
    u8* bone;

    if (*(s32*)(arg0 + 0x34) == -1) {
        s32 key = *(s32*)(arg0 + 0x30);
        if (key != 0) {
            *(s32*)(arg0 + 0x34) = fn_8018BDE0(fn_801ACF80(*(u8**)(arg0 + 0x10), 0, 0, 0, 0, 0, 0, 0), key);
        }
    }
    bone = NULL;
    if (*(s32*)(arg0 + 0x34) != -1 && fn_801ACF80(*(u8**)(arg0 + 0x10), 0, 0, 0, 0, 0, 0, 0) != NULL) {
        bone = fn_8018C39C(fn_801ACF80(*(u8**)(arg0 + 0x10), 0, 0, 0, 0, 0, 0, 0), *(s32*)(arg0 + 0x34));
    }
    if (bone != NULL) {
        fn_800526D0(m1E0);
        if (bone != NULL) {
            const u8* owner;
            fn_8005535C(m1E0, bone);
            fn_80055828((f32*)&v20, m1E0, (const Vec3*)arg1);
            fn_800553C0(m1E0, &v20);
            u8* skin = *(u8**)(*(u8**)(arg0 + 0x10) + 0x90);
            if (skin != NULL) {
                owner = (const u8*)fn_801DA798(skin);
            } else {
                owner = arg2;
                fn_800526D0(m1B0);
                fn_800526D0(m180);
                fn_800557B4(mF0);
                fn_8005535C(m180, mF0);
                fn_801ACF90(m180, *(u8**)(arg0 + 0x10) + 8);
                fn_800553DC((f32*)(*(u8**)(*(u8**)(arg0 + 0x10) + 0x4) + 0xC), m1B0);
                fn_800554DC(mC0, m180, m1B0);
                fn_8005535C(arg2, mC0);
                fn_800553C0(arg2, *(Vec3**)(*(u8**)(arg0 + 0x10) + 0x4));
            }
            fn_800554DC(m90, m1E0, owner);
            fn_8005535C(arg2, m90);
        }
    } else {
        u8* skin = *(u8**)(*(u8**)(arg0 + 0x10) + 0x90);
        if (skin != NULL) {
            fn_8005535C(arg2, (const u8*)fn_801DA798(skin));
            Vec3Rotate(&v14, (const f32*)arg2, (const Vec3*)arg1);
            Vec3AddTo((Vec3*)fn_800526D4(arg2), &v14);
            return;
        }
        fn_800526D0(m150);
        fn_800526D0(m120);
        fn_800557B4(m60);
        fn_8005535C(m120, m60);
        fn_801ACF90(m120, *(u8**)(arg0 + 0x10) + 8);
        fn_800553DC((f32*)(*(u8**)(*(u8**)(arg0 + 0x10) + 0x4) + 0xC), m150);
        fn_800554DC(m30, m120, m150);
        fn_8005535C(arg2, m30);
        Vec3Copy((Vec3*)fn_800526D4(arg2), *(Vec3**)(*(u8**)(arg0 + 0x10) + 0x4));
        Vec3Rotate(&v8, (const f32*)arg2, (const Vec3*)arg1);
        Vec3AddTo((Vec3*)fn_800526D4(arg2), &v8);
    }
}

// 0x801ADB44
// Moves from point a toward point b until the distance equals farg0 + farg1 (R). Returns 0 when b
// is farther than R, 1 otherwise (also 1 for coincident points, writing nothing). out+0x18 gets
// R - distance. Without an axis (arg3 == 0) out gets the point on the a->b line at distance R
// from b and out+0xC the unit direction; with an axis arg4 the direction is the component of
// a->b perpendicular to it and the point is placed from b along that direction.
// (sqrt is frsqrte + two Newton steps in the original; sqrtf matches to float precision.)
s32 fn_801ADB44(u8* arg0, u8* arg1, u8* arg2, s32 arg3, u8* arg4, f32 farg0, f32 farg1) {
    f32 R = farg0 + farg1;
    f32 d[3];
    f32 n[3];
    f32 len;
    s32 tiny = 0;

    d[0] = *(f32*)(arg1 + 0x0) - *(f32*)(arg0 + 0x0);
    d[1] = *(f32*)(arg1 + 0x4) - *(f32*)(arg0 + 0x4);
    d[2] = *(f32*)(arg1 + 0x8) - *(f32*)(arg0 + 0x8);
    f32 d2 = d[2] * d[2] + (d[0] * d[0] + d[1] * d[1]);
    f32 diff = d2 - R * R;
    if (d2 <= lbl_805FFAC8 && d2 >= lbl_805FFAE0) {
        tiny = 1;
    }
    if (tiny != 0) {
        return 1;
    }
    if (!(diff <= lbl_805FFAB8)) {
        return 0;
    }

    f32 l2 = d[2] * d[2] + (d[0] * d[0] + d[1] * d[1]);
    if (lbl_805FFAB8 == l2) {
        n[0] = lbl_805FFAB8;
        n[1] = lbl_805FFAB8;
        n[2] = lbl_805FFAB8;
    } else {
        f32 inv = (f32)(1.0 / sqrt((f64)l2));
        n[0] = d[0] * inv;
        n[1] = d[1] * inv;
        n[2] = d[2] * inv;
    }
    len = lbl_805FFAB8;
    f32 l2b = d[2] * d[2] + (d[0] * d[0] + d[1] * d[1]);
    if (!(l2b <= lbl_805FFAB8)) {
        len = (f32)sqrt((f64)l2b);
    }
    f32 gap = R - len;
    *(f32*)(arg2 + 0x18) = gap;
    if (arg3 != 0) {
        f32 ax = *(f32*)(arg4 + 0x0);
        f32 ay = *(f32*)(arg4 + 0x4);
        f32 az = *(f32*)(arg4 + 0x8);
        f32 dot = d[2] * az + (d[0] * ax + d[1] * ay);
        f32 px = ax * dot;
        f32 py = ay * dot;
        f32 pz = az * dot;
        f32 qx = d[0] - px;
        f32 qy = d[1] - py;
        f32 qz = d[2] - pz;
        f32 q2 = qz * qz + (qx * qx + qy * qy);
        f32 plen = lbl_805FFAB8;
        if (!(q2 <= lbl_805FFAB8)) {
            plen = (f32)sqrt((f64)q2);
        }
        if (lbl_805FFAB8 != plen) {
            qx /= plen;
            qy /= plen;
            qz /= plen;
        }
        f32 h2 = R * R - dot * dot;
        f32 h = lbl_805FFAB8;
        if (!(h2 <= lbl_805FFAB8)) {
            h = (f32)sqrt((f64)h2);
        }
        *(f32*)(arg2 + 0xC) = qx;
        *(f32*)(arg2 + 0x10) = qy;
        *(f32*)(arg2 + 0x14) = qz;
        f32 t = -plen + h;
        f32 ox = qx * t;
        f32 oy = qy * t;
        f32 oz = qz * t;
        *(f32*)(arg2 + 0x0) = *(f32*)(arg1 + 0x0) + ox;
        *(f32*)(arg2 + 0x4) = *(f32*)(arg1 + 0x4) + oy;
        *(f32*)(arg2 + 0x8) = *(f32*)(arg1 + 0x8) + oz;
    } else {
        f32 ox = n[0] * gap;
        f32 oy = n[1] * gap;
        f32 oz = n[2] * gap;
        *(f32*)(arg2 + 0x0) = *(f32*)(arg0 + 0x0) + ox;
        *(f32*)(arg2 + 0x4) = *(f32*)(arg0 + 0x4) + oy;
        *(f32*)(arg2 + 0x8) = *(f32*)(arg0 + 0x8) + oz;
        *(f32*)(arg2 + 0xC) = n[0];
        *(f32*)(arg2 + 0x10) = n[1];
        *(f32*)(arg2 + 0x14) = n[2];
    }
    return 1;
}

// 0x80207DCC
// One-time init of the 0x40-byte player data block and its record stream, then waits (5 ms
// sleeps, up to lbl_80600478 seconds) for the init callback fn_80207BE4 to set lbl_805FB374,
// servicing the pending event (1: DWC_ClearError, 2..7: mark the init as failed). Returns 0 when
// the stream could not be opened, else 1.
s32 fn_80207DCC(void) {
    s16 name[16];
    u8 timer[8];
    s32 ev;
    s32 aux;
    s32 got;
    s32 prev;

    if (lbl_805FB380 != 0) {
        return 1;
    }
    if (fn_802FAC0C(lbl_80556E80) == 0) {
        fn_802FABD4(lbl_80556E80);
        memset(lbl_80556E80 + 0x40, 0, 0x300);
    }
    fn_802E8FD4(0, lbl_80556E80, 0x2C61, (const void*)(lbl_80386ED0 + 0x56C), (void*)&lbl_805F9950,
                0x3E80, 0x3E80, lbl_80556E80 + 0x40, 0x40);
    lbl_805FB374 = 0;
    // Widen the NUL-terminated byte string at +0xB40 into name[] (at most 16 entries).
    for (s32 i = 0; i < 16; i++) {
        s32 c = (s8) * (lbl_80556E80 + 0xB40 + i);
        name[i] = (s16)c;
        if (c == 0) break;
    }
    if (fn_802E9668(name, 0, (void*)fn_80207BE4, 0) == 0) {
        return 0;
    }
    prev = -1;
    fn_80142500(timer);
    TimerReset(timer);
    while (lbl_805FB374 == 0 && TimerGetSeconds(timer) < lbl_80600478) {
        fn_802E93CC();
        u64 ticks = (u64)(OSGetBusClock() / 4 / 1000) * 5;
        OSSleepTicks((s32)(ticks >> 32), (u32)ticks);
        got = fn_802EA048();
        if (got != prev) {
            prev = got;  // the asm has a 7-entry jump table here; every entry goes to the next step
        }
        if (fn_802E48B8(&aux, &ev) != 0) {
            if (ev == 1) {
                DWC_ClearError();
            } else if (ev >= 2 && ev < 8) {
                lbl_805FB374 = 1;
                lbl_805FB36C = 0;
            }
        }
    }
    return 1;
}

}  // extern "C"
