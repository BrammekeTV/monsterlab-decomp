// Drafts miss634: animation blend position, clip ring walk, camera ray sweep, spawn timer, camera FOV.
#include "types.h"

typedef f32  (*FnF0)(u8* self);                 // virtual returning f32 (slot +0xD0: clip length)
typedef void (*FnClipSample)(u8* out, u8* clip, f32 t);  // virtual slot +0xD8
typedef s32  (*FnI0)(u8* self);
typedef s32  (*FnHit)(u8* self, f32* out, u8* shape, f32* hit, s32 a, s32 b, s32 c);  // slot +0x14
typedef void (*FnShapeInit)(u8* self, s32* kind, s32 a, s32 b);                       // slot +0xC

extern "C" {
u32   RandomU32(void);                                      // 0x80143460
f32   fn_800DE2AC(u8* obj);                                 // 0x800DE2AC
void* fn_801ACCA0(u8* arg0, s32 arg1);                      // 0x801ACCA0
// 0x801B0E34: defined elsewhere with a single argument (src/engine/draft_801Bb.cpp); the caller in
// fn_800DB490 passes four (this, &kind, 0, 0).
u8*   fn_801B0E34(u8* self);       // 0x801B0E34
f32   fn_801F5E94(f32 cur, f32 target, f32 lo, f32 hi, f32 range);  // 0x801F5E94
f32   sinf(f32 x);                                          // 0x8033F0A4 (MSL)

extern u8  lbl_803C4298[];          // 0x803C4298 (+0x4: object, +0x8: camera)
extern u8* lbl_805FB188;            // 0x805FB188 (spawn record table)

extern f32 lbl_805FE3B0;            // 0x805FE3B0 (0.0)
extern f32 lbl_805FE3B4;            // 0x805FE3B4 (1.0)
extern f32 lbl_805FE3D8;            // 0x805FE3D8 (2/pi)
extern f32 lbl_805FE3DC;            // 0x805FE3DC (pi/2)
extern f32 lbl_805FE3E4;            // 0x805FE3E4 (-epsilon)
extern f32 lbl_805FE3E8;            // 0x805FE3E8 (+epsilon)
extern f32 lbl_805FE644;            // 0x805FE644 (RandomU32 scale)
extern f32 lbl_805FE6F0;            // 0x805FE6F0 (0.0)
extern f32 lbl_805FE6F8;            // 0x805FE6F8 (1e20)
extern f32 lbl_805FE8AC;            // 0x805FE8AC
extern f32 lbl_805FE8B0;            // 0x805FE8B0 (0.0)
extern f32 lbl_805FE8B8;            // 0x805FE8B8 (1.0)
extern f32 lbl_805FE8C4;            // 0x805FE8C4
extern f32 lbl_805FE8F0;            // 0x805FE8F0
extern f32 lbl_805FE8F4;            // 0x805FE8F4
extern f32 lbl_805FE8F8;            // 0x805FE8F8
extern f32 lbl_805FE8FC;            // 0x805FE8FC
extern f32 lbl_805FE900;            // 0x805FE900
extern f32 lbl_805FE904;            // 0x805FE904
extern f32 lbl_805FE908;            // 0x805FE908
extern f32 lbl_805FE90C;            // 0x805FE90C
extern f32 lbl_805FE910;            // 0x805FE910
extern f32 lbl_805FE914;            // 0x805FE914
}

// Clip ring of an animation player: head (+0x6C), tail (+0x70), capacity (+0x74), slot array
// (+0x78), current slot (+0x7C). A slot is only valid when its index lies within head..tail
// (wrapping around the capacity).
static u8** RingSlot(u8* r, s32 idx) {
    s32 head = *(s32*)(r + 0x6C);
    s32 tail = *(s32*)(r + 0x70);
    s32 cap  = *(s32*)(r + 0x74);
    if (idx < 0 || idx >= cap) {
        return NULL;
    }
    s32 lo = idx;
    s32 hi = tail;
    if (tail < head) {
        if (idx <= tail) {
            lo = idx + cap;
        }
        hi = tail + cap;
    }
    if (head > lo || lo > hi) {
        return NULL;
    }
    return *(u8***)(r + 0x78) + idx;
}

// Points the current slot at the tail clip (when the ring is not empty) and returns that clip.
static u8* RingFirst(u8* r) {
    if (*(s32*)(r + 0x6C) >= 0 && *(s32*)(r + 0x70) >= 0) {
        u8** s = RingSlot(r, *(s32*)(r + 0x70));
        *(u8***)(r + 0x7C) = s;
        if (s != NULL) {
            return *s;
        }
    }
    return NULL;
}

// Steps the current slot one clip back towards the head and returns that clip (NULL at the head).
static u8* RingNext(u8* r) {
    u8** it = *(u8***)(r + 0x7C);
    if (it == NULL) {
        return NULL;
    }
    u8** base = *(u8***)(r + 0x78);
    s32 head = *(s32*)(r + 0x6C);
    s32 cur = (s32)(it - base);
    u8** ns;
    if (cur == head) {
        ns = NULL;
    } else {
        s32 idx = cur - 1;
        if (idx < 0) {
            idx = *(s32*)(r + 0x74) - 1;
        }
        ns = RingSlot(r, idx);
    }
    *(u8***)(r + 0x7C) = ns;
    if (ns != NULL) {
        return *ns;
    }
    return NULL;
}

// Element of the 0x28-byte spawn record table; an index outside 0..count gives the table base.
static u8* SpawnRecAt(u8* tbl, s32 count, s32 i) {
    s32 c = (i < 0) ? 0 : ((i > count) ? count : i);
    return (i == c) ? tbl + i * 0x28 : tbl;
}

extern "C" {

// 0x800C4C84
// Blends two stored points (slots at +0x264 and +0x268 of the 0xC8-byte entries at +0x98) around
// the base point at +0x2A0, weighted by dt over the clip length (optionally eased by the sine
// warp at +0x2B0), rotates the result by the quaternion at (+0x4)+0xC and adds that
// transform's translation. Result vec3 goes to arg0.
void fn_800C4C84(u8* arg0, u8* arg1, f32 dt) {
    f32 w1 = dt / (*(FnF0*)(*(u8**)arg1 + 0xD0))(arg1);
    f32 warp = *(f32*)(arg1 + 0x2B0);
    f32 w0 = lbl_805FE3B4 - w1;
    if (warp > lbl_805FE3B0) {
        f32 k = lbl_805FE3D8 * warp;
        f32 norm = lbl_805FE3B4 / (f32)sinf(lbl_805FE3DC * k);
        w0 = norm * (f32)sinf(lbl_805FE3DC * ((lbl_805FE3B4 - w1) * k));
        w1 = norm * (f32)sinf(lbl_805FE3DC * (w1 * k));
    }
    f32 ax = lbl_805FE3B0;
    f32 ay = lbl_805FE3B0;
    f32 az = lbl_805FE3B0;
    s32 idxA = *(s32*)(arg1 + 0x264);
    if (idxA >= 0) {
        u8* e = *(u8**)(arg1 + 0x98) + idxA * 0xC8;
        ax = *(f32*)(e + 0x2C);
        ay = *(f32*)(e + 0x30);
        az = *(f32*)(e + 0x34);
    }
    f32 cx = *(f32*)(arg1 + 0x2A0);
    f32 cy = *(f32*)(arg1 + 0x2A4);
    f32 cz = *(f32*)(arg1 + 0x2A8);
    u8* eB = *(u8**)(arg1 + 0x98) + *(s32*)(arg1 + 0x268) * 0xC8;
    f32 bx = *(f32*)(eB + 0x2C);
    f32 by = *(f32*)(eB + 0x30);
    f32 bz = *(f32*)(eB + 0x34);
    f32 dAx = ax - cx;
    f32 dAy = ay - cy;
    f32 dAz = az - cz;
    f32 dBx = bx - cx;
    f32 dBy = by - cy;
    f32 dBz = bz - cz;
    f32 vx = ((dAx * w0) + (dBx * w1)) + cx;
    f32 vy = ((dAy * w0) + (dBy * w1)) + cy;
    f32 vz = ((dAz * w0) + (dBz * w1)) + cz;
    *(f32*)(arg0 + 0x0) = vx;
    *(f32*)(arg0 + 0x4) = vy;
    *(f32*)(arg0 + 0x8) = vz;
    u8* xf = *(u8**)(arg1 + 0x4);
    f32 qx = *(f32*)(xf + 0xC);
    f32 qy = *(f32*)(xf + 0x10);
    f32 qz = *(f32*)(xf + 0x14);
    f32 qw = *(f32*)(xf + 0x18);
    f32 nqx = -qx;
    f32 nqy = -qy;
    f32 nqz = -qz;
    f32 zero = lbl_805FE3B0;
    // t = q * (v, 0)
    f32 tx = ((qy * vz) + ((qw * vx) + (qx * zero))) - (qz * vy);
    f32 tw = (((qw * zero) - (qx * vx)) - (qy * vy)) - (qz * vz);
    f32 ty = (qz * vx) + ((qy * zero) + ((qw * vy) - (qx * vz)));
    f32 tz = (qz * zero) + (((qw * vz) + (qx * vy)) - (qy * vx));
    // r = t * conj(q)
    f32 rx = ((ty * nqz) + ((tw * nqx) + (tx * qw))) - (tz * nqy);
    f32 ry = (tz * nqx) + ((ty * qw) + ((tw * nqy) - (tx * nqz)));
    f32 rz = (tz * qw) + (((tw * nqz) + (tx * nqy)) - (ty * nqx));
    *(f32*)(arg0 + 0x0) = rx + *(f32*)(xf + 0x0);
    *(f32*)(arg0 + 0x4) = ry + *(f32*)(xf + 0x4);
    *(f32*)(arg0 + 0x8) = rz + *(f32*)(xf + 0x8);
}

// 0x800C54A0
// Time left inside the clip that contains time t (from the virtual at (+0xD4)+0x20): walks the
// clip ring subtracting each clip's length (slot +0xD0) until t fits; returns the remainder (or
// the clip length when the remainder is within epsilon of zero).
f32 fn_800C54A0(u8* arg0) {
    f32 t = (*(FnF0*)(*(u8**)(arg0 + 0xD4) + 0x20))(arg0);
    u8* cur = RingFirst(arg0);
    while (t > (*(FnF0*)(*(u8**)cur + 0xD0))(cur)) {
        t -= (*(FnF0*)(*(u8**)cur + 0xD0))(cur);
        if (t > lbl_805FE3E4 && t < lbl_805FE3E8) {
            return (*(FnF0*)(*(u8**)cur + 0xD0))(cur);
        }
        cur = RingNext(arg0);
    }
    return t;
}

// 0x800C597C
// Same walk as fn_800C54A0 over the ring of arg1 with the time dt given: once dt fits inside a
// clip, samples that clip (virtual +0xD8) at the remaining time into arg0.
void fn_800C597C(u8* arg0, u8* arg1, f32 dt) {
    u8* cur = RingFirst(arg1);
    while (dt > (*(FnF0*)(*(u8**)cur + 0xD0))(cur)) {
        dt -= (*(FnF0*)(*(u8**)cur + 0xD0))(cur);
        cur = RingNext(arg1);
    }
    (*(FnClipSample*)(*(u8**)cur + 0xD8))(arg0, cur, dt);
}

// 0x800D7394
// Finds the spawn record whose handle (+0) is arg0 and gives it a random timer between its
// floats at +0xC and +0x10 (stored at +0x14), sets its flag byte (+0x20) and clears +0x24.
void fn_800D7394(u8* arg0) {
    u8* tbl = lbl_805FB188;
    if (tbl == NULL) {
        return;
    }
    s32 found = -1;
    s32 n = *(s32*)(tbl + 0x320);
    for (s32 i = 0; i < n; i++) {
        u8* r = SpawnRecAt(tbl, *(s32*)(tbl + 0x320), i);
        if (arg0 == *(u8**)r) {
            found = i;
            break;
        }
    }
    if (found < 0) {
        return;
    }
    u8* t0 = lbl_805FB188;
    s32 c0 = *(s32*)(t0 + 0x320);
    f32 lo = *(f32*)(SpawnRecAt(t0, c0, found) + 0xC);
    f32 hi = *(f32*)(SpawnRecAt(tbl, c0, found) + 0x10);
    u8* rec = SpawnRecAt(tbl, c0, found);
    *(f32*)(rec + 0x14) = lo + ((hi - lo) * (lbl_805FE644 * (f32)RandomU32()));
    u8* t1 = lbl_805FB188;
    *(u8*)(SpawnRecAt(t1, *(s32*)(t1 + 0x320), found) + 0x20) = 1;
    u8* t2 = lbl_805FB188;
    *(s32*)(SpawnRecAt(t2, *(s32*)(t2 + 0x320), found) + 0x24) = 0;
}

// 0x800DB490
// Sweeps the segment a..b (a shape-query object of kind 5) against every sub-shape of every
// shape in arg2 (shape array at +0xC, count at +0x10; sub-shapes at +0x24, count at +0x28) and
// returns the smallest hit distance, 1e20 when nothing is hit.
f32 fn_800DB490(f32* a, f32* b, u8* arg2) {
    u8 query[0x54];
    s32 kind = 5;
    f32 zero = lbl_805FE6F0;
    f32 best = lbl_805FE6F8;
    u8* shape;
    u32 i;
    u32 j;
    fn_801B0E34(query);
    (*(FnShapeInit*)(*(u8**)query + 0xC))(query, &kind, 0, 0);
    f32 ax = a[0];
    f32 ay = a[1];
    f32 az = a[2];
    *(f32*)(query + 0x3C) = ax;
    *(f32*)(query + 0x40) = ay;
    *(f32*)(query + 0x44) = az;
    *(f32*)(query + 0x48) = b[0] - ax;
    *(f32*)(query + 0x4C) = b[1] - ay;
    *(f32*)(query + 0x50) = b[2] - az;
    u32 count = *(u32*)(arg2 + 0x10);
    for (i = 0; i < count; i++) {
        if (i >= *(u32*)(arg2 + 0x10)) {
            shape = NULL;
        } else {
            shape = ((u8**)*(u8**)(arg2 + 0xC))[i];
        }
        if (shape == NULL) {
            continue;
        }
        f32 nearest = lbl_805FE6F8;
        for (j = 0; j < *(u32*)(shape + 0x28); j++) {
            f32 hit[7];
            f32 out[7];
            for (s32 k = 0; k < 7; k++) {
                hit[k] = zero;
                out[k] = zero;
            }
            u8* sub;
            if (j >= *(u32*)(shape + 0x28)) {
                sub = NULL;
            } else {
                sub = ((u8**)*(u8**)(shape + 0x24))[j];
            }
            if ((*(FnHit*)(*(u8**)sub + 0x14))(sub, out, query, hit, 0, 0, 0) != 0 && hit[6] < nearest) {
                nearest = hit[6];
            }
        }
        if (!(best < nearest)) {
            best = nearest;
        }
    }
    fn_801ACCA0(query, 0);
    return best;
}

// 0x800E0688
// Camera field of view: picks a min/max range by the camera mode (+0xD4), scales it by the
// camera target's value (fn_800DE2AC, over 5), clamps it, blends it with the previous value
// (+0xCC) by the factor at +0xD0, converts the result to radians (+0x64) and eases the angle at
// +0x68 towards it.
void fn_800E0688(u8* arg0, f32 dt) {
    u8* cam = *(u8**)(lbl_803C4298 + 0x8);
    if (*(u8**)(cam + 0x2C0) == NULL) {
        return;
    }
    s32 mode = *(s32*)(arg0 + 0xD4);
    f32 lo = lbl_805FE8C4;
    f32 hi = lbl_805FE8F0;
    if (mode == 0) {
        *(f32*)(arg0 + 0x64) = lbl_805FE8AC;
    } else if (mode == 1) {
        *(f32*)(arg0 + 0x64) = lbl_805FE8F4;
        lo = lbl_805FE8F8;
        hi = lbl_805FE8FC;
    }
    u8* obj = *(u8**)(lbl_803C4298 + 0x4);
    if ((*(FnI0*)(*(u8**)obj + 0x7C))(obj) & 8) {
        lo = lbl_805FE900;
        hi = lo;
        *(f32*)(arg0 + 0x64) = lo;
    }
    f32 v = lo + ((fn_800DE2AC(*(u8**)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0)) / lbl_805FE904) * (hi - lo));
    *(f32*)(arg0 + 0x64) = v;
    if (lo < hi) {
        if (v < lo) {
        } else {
            if (v > hi) {
            } else {
                hi = v;
            }
            lo = hi;
        }
    } else {
        if (v < hi) {
        } else {
            if (v > lo) {
            } else {
                lo = v;
            }
            hi = lo;
        }
        lo = hi;
    }
    f32 t = *(f32*)(arg0 + 0xD0);
    *(f32*)(arg0 + 0x64) = lo;
    f32 blend;
    if (t >= lbl_805FE8B8) {
        blend = lbl_805FE8B0;
    } else {
        blend = lbl_805FE8B8 - t;
    }
    f32 ang = lbl_805FE908 * ((blend * *(f32*)(arg0 + 0x64)) + ((lbl_805FE8B8 - blend) * *(f32*)(arg0 + 0xCC)));
    f32 step = lbl_805FE90C * dt;
    *(f32*)(arg0 + 0x64) = ang;
    *(f32*)(arg0 + 0x68) = fn_801F5E94(*(f32*)(arg0 + 0x68), ang, lbl_805FE910 * step, step, lbl_805FE914);
}

}  // extern "C"
