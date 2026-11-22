// Drafts miss664: clip-node update, closest points between two clip lines, input-driven vector
// cycling, two camera-follow helpers (0x800E region) and the sound/effect string spawner.
#include <math.h>
#include <string.h>
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef f32  (*FnF0)(u8* self);
typedef s32  (*FnI0)(u8* self);
typedef void (*FnV0)(u8* self);
typedef void (*FnUpdate)(u8* self, f32 dt);
typedef void (*FnVec)(u8* self, f32* v);
typedef u8*  (*FnSpawn)(u8* self, const char* str, s32 a, s32 b, u32 len, s32 c, s32 d, f32 scale);

extern "C" {
void  fn_800C148C(u8* p);                                   // 0x800C148C
void  fn_800C15E8(u8* p);                                   // 0x800C15E8
void  fn_800C2C80(u8* p);                                   // 0x800C2C80 (return value unused by the caller)
void  fn_800D1D3C(u8* p, f32 dt);                           // 0x800D1D3C
void  fn_800D28B8(u8* p);                                   // 0x800D28B8
void  fn_800DA8F0(u8* query, u8* result);                   // 0x800DA8F0
s32   fn_800DECD8(u8* p, f32 dt);                           // 0x800DECD8
BOOL  fn_80147554(s32 button);                              // 0x80147554
void  fn_80155724(u8* m);                                   // 0x80155724
void  fn_801B55F4(u8* obj, u8* out, s32 idx);               // 0x801B55F4
void* fn_801B7198(void* anim, s32 a);                       // 0x801B7198
void  fn_801D679C(void* unused, f32* src);                  // 0x801D679C
f32   fn_801F60F8(f32 farg0, f32 farg1);                    // 0x801F60F8
void  fn_801F6B78(u8* p, f32* v);                           // 0x801F6B78
f64   fn_8033F1F4(f64 x);                                   // 0x8033F1F4 (MSL acos)

extern u8  lbl_803C4298[];          // 0x803C4298 (+0x8: camera)
extern u32 lbl_805F9614;            // 0x805F9614 (view matrix mode)
extern f32 lbl_805FE3B0;            // 0x805FE3B0 (0.0)
extern f32 lbl_805FE3B4;            // 0x805FE3B4 (1.0)
extern f32 lbl_805FE3B8;            // 0x805FE3B8 (-1.0)
extern f32 lbl_805FE3BC;            // 0x805FE3BC (0.0001)
extern f32 lbl_805FE3C0;            // 0x805FE3C0 (-0.0001)
extern f32 lbl_805FE3D8;            // 0x805FE3D8 (2/pi)
extern f32 lbl_805FE3DC;            // 0x805FE3DC (pi/2)
extern f32 lbl_805FE5DC;            // 0x805FE5DC (-0.0001)
extern f32 lbl_805FE5E0;            // 0x805FE5E0 (0.0001)
extern f32 lbl_805FE9A0;            // 0x805FE9A0 (0.0)
extern f32 lbl_805FE9B8;            // 0x805FE9B8 (-2 pi)
extern f32 lbl_805FE9BC;            // 0x805FE9BC (2 pi)
extern f32 lbl_805FE9C0;            // 0x805FE9C0 (-pi)
extern f32 lbl_805FE9C4;            // 0x805FE9C4 (pi)
extern f32 lbl_805FE9C8;            // 0x805FE9C8 (300.0)
extern f32 lbl_805FEA20;            // 0x805FEA20 (15.0)
extern f32 lbl_805FEA24;            // 0x805FEA24 (5.0)
extern f32 lbl_805FEA28;            // 0x805FEA28 (0.0)
extern f32 lbl_805FEA2C;            // 0x805FEA2C (1.0)
extern f32 lbl_805FEA30;            // 0x805FEA30 (3.0)
extern f32 lbl_805FEA34;            // 0x805FEA34 (2/pi)
extern f32 lbl_805FEA38;            // 0x805FEA38 (pi/2)
extern f32 lbl_805FF8C0;            // 0x805FF8C0 (1.0)
extern f32 lbl_805FF8C4;            // 0x805FF8C4 (0.0)
}
// 0x801B55A8: defined elsewhere with C++ linkage; returns the locator count as an integer value.
extern "C++" u8* fn_801B55A8(u8* ent);
// 0x80197FE0: defined elsewhere with C++ linkage.
extern "C++" f32 fn_80197FE0(void* obj);

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

// Second vtable of the player object (+0xD4).
static u8* PlayerVt(u8* p) {
    return *(u8**)(p + 0xD4);
}

// True when |v| is within the small epsilon around zero.
static BOOL NearZero(f32 v) {
    return v <= lbl_805FE3BC && v >= lbl_805FE3C0;
}

// Cycle-vector helpers of fn_800D163C: six vec3 at +0x188, current index +0x180, chosen index +0x184.
static void CycleStep(u8* p, s32 off) {
    s32 i = *(s32*)(p + off) + 1;
    *(s32*)(p + off) = i;
    if (i >= 6) {
        *(s32*)(p + off) = 0;
    }
}

// Dot product of the chosen vector (+0x184) with the current one (+0x180) is near zero.
static BOOL CycleOrthogonal(u8* p) {
    f32* a = (f32*)(p + 0x188 + *(s32*)(p + 0x180) * 0xC);
    f32* b = (f32*)(p + 0x188 + *(s32*)(p + 0x184) * 0xC);
    f32 d = (b[2] * a[2]) + ((b[0] * a[0]) + (b[1] * a[1]));
    return d > lbl_805FE5DC && d < lbl_805FE5E0;
}

// Feeds the current vector to the members at +0x150C and +0x2908.
static void CycleFeed(u8* p) {
    f32 v[3];
    f32* a = (f32*)(p + 0x188 + *(s32*)(p + 0x180) * 0xC);
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
    fn_801F6B78(p + 0x150C, v);
    fn_801F6B78(p + 0x2908, v);
}

// Steps the current index up to three times until its vector is near-orthogonal to the chosen
// one and feeds it; falls back to fn_800D28B8 when none is.
static void CycleFindOrthogonal(u8* p) {
    for (s32 n = 0; n < 3; n++) {
        CycleStep(p, 0x180);
        if (CycleOrthogonal(p)) {
            CycleFeed(p);
            return;
        }
    }
    fn_800D28B8(p);
}

// Clamps an angle to +-2pi, wraps it into +-pi and clamps again.
static f32 NormalizeAngle(f32 a) {
    f32 c = lbl_805FE9B8;
    if (!(a < c)) {
        c = lbl_805FE9BC;
        if (!(a > c)) {
            c = a;
        }
    }
    if (c < lbl_805FE9C0) {
        c += lbl_805FE9BC;
    } else if (c > lbl_805FE9C4) {
        c -= lbl_805FE9BC;
    }
    f32 r = lbl_805FE9C0;
    if (!(c < r)) {
        r = lbl_805FE9C4;
        if (!(c > r)) {
            r = c;
        }
    }
    return r;
}

// Query and result blocks of the camera collision (fn_800DA8F0), built on the stack.
struct CamQuery {
    f32 from[3];    // 0x00
    f32 to[3];      // 0x0C
    f32 f18;        // 0x18
    f32 f1C;        // 0x1C
    u8  b20;        // 0x20
    s32 w24;        // 0x24
    s32 w28;        // 0x28
    s32 w2C;        // 0x2C
};
struct CamResult {
    u8  hit;        // 0x00
    f32 v04[3];     // 0x04
    f32 v10[3];     // 0x10
    f32 pos[3];     // 0x1C
};

extern "C" {

// 0x800C3884
// Per-frame update of a clip player: runs every attached clip (+0xC array, count +0x5C), marks
// the ones past the limit from vtable +0x20 as started, then lets the virtual hooks (+0x30,
// +0x2C of the +0xD4 table) consume the ring, tests the ring's first visible clip and updates
// every clip in the ring.
void fn_800C3884(u8* arg0, f32 dt) {
    f32 limit = (*(FnF0*)(PlayerVt(arg0) + 0x20))(arg0);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x5C); i++) {
        u8* o = ((u8**)(arg0 + 0xC))[i];
        (*(FnUpdate*)(*(u8**)o + 0xC4))(o, dt);
        o = ((u8**)(arg0 + 0xC))[i];
        if (limit > *(f32*)(o + 0x264) && *(u8*)(o + 0x268) == 0) {
            (*(FnV0*)(*(u8**)o + 0xC8))(o);
            *(u8*)(((u8**)(arg0 + 0xC))[i] + 0x268) = 1;
        }
    }
    BOOL done;
    do {
        done = TRUE;
        if (RingFirst(arg0) != NULL && (*(FnI0*)(PlayerVt(arg0) + 0x30))(arg0) != 0) {
            fn_800C2C80(arg0);
            done = FALSE;
        }
    } while (!done);
    do {
        done = TRUE;
        if (RingFirst(arg0) != NULL && (*(FnI0*)(PlayerVt(arg0) + 0x2C))(arg0) != 0) {
            (*(FnV0*)(PlayerVt(arg0) + 0x18))(arg0);
            done = FALSE;
        }
    } while (!done);
    u8* e = RingFirst(arg0);
    while (e != NULL) {
        if (*(u8*)(e + 0x26D) == 0) {
            fn_800C15E8(e);
            break;
        }
        e = RingNext(arg0);
    }
    e = RingFirst(arg0);
    while (e != NULL) {
        (*(FnUpdate*)(*(u8**)e + 0xC4))(e, dt);
        e = RingNext(arg0);
    }
}

// 0x800C45C4
// Finds the closest points between the two lines of the clip entries named by +0x264 and +0x268
// (entries of 0xC8 bytes at +0x98: point +0x2C, direction +0x14). Stores the point on the first
// line at +0x2A0, the "no hit" marker -1.0 at +0x2AC and the angle between the directions
// (scaled to pi/2 per unit) at +0x2B0; stays at the defaults when the lines are parallel.
void fn_800C45C4(u8* arg0) {
    fn_800C148C(arg0);
    s32 idxA = *(s32*)(arg0 + 0x264);
    f32 ax = lbl_805FE3B0, ay = lbl_805FE3B0, az = lbl_805FE3B0;   // line A point
    f32 dax = lbl_805FE3B0, day = lbl_805FE3B4, daz = lbl_805FE3B0; // line A direction
    if (idxA >= 0) {
        u8* ea = *(u8**)(arg0 + 0x98) + idxA * 0xC8;
        ax = *(f32*)(ea + 0x2C);
        ay = *(f32*)(ea + 0x30);
        az = *(f32*)(ea + 0x34);
        dax = *(f32*)(ea + 0x14);
        day = *(f32*)(ea + 0x18);
        daz = *(f32*)(ea + 0x1C);
    }
    u8* eb = *(u8**)(arg0 + 0x98) + *(s32*)(arg0 + 0x268) * 0xC8;
    f32 dbx = *(f32*)(eb + 0x14);
    f32 dby = *(f32*)(eb + 0x18);
    f32 dbz = *(f32*)(eb + 0x1C);
    f32 bx = *(f32*)(eb + 0x2C);
    f32 by = *(f32*)(eb + 0x30);
    f32 bz = *(f32*)(eb + 0x34);
    // n = dA x dB
    f32 nx = (day * dbz) - (daz * dby);
    f32 ny = (daz * dbx) - (dax * dbz);
    f32 nz = (dax * dby) - (day * dbx);
    *(f32*)(arg0 + 0x2A0) = lbl_805FE3B0;
    *(f32*)(arg0 + 0x2A4) = lbl_805FE3B0;
    *(f32*)(arg0 + 0x2A8) = lbl_805FE3B0;
    *(f32*)(arg0 + 0x2AC) = lbl_805FE3B8;
    *(f32*)(arg0 + 0x2B0) = lbl_805FE3B0;
    if (NearZero(nx) && NearZero(ny) && NearZero(nz)) {
        return;   // parallel lines
    }
    // mA = dA x n, mB = dB x n
    f32 mAx = (day * nz) - (daz * ny);
    f32 mAy = (daz * nx) - (dax * nz);
    f32 mAz = (dax * ny) - (day * nx);
    f32 mBx = (dby * nz) - (dbz * ny);
    f32 mBy = (dbz * nx) - (dbx * nz);
    f32 mBz = (dbx * ny) - (dby * nx);
    f32 dx = ax - bx;
    f32 dy = ay - by;
    f32 dz = az - bz;
    f32 denX = mBx - mAx;
    f32 denY = mBy - mAy;
    f32 denZ = mBz - mAz;
    f32 t = lbl_805FE3B0;
    if (!NearZero(denX)) {
        t = dx / denX;
    } else if (!NearZero(denY)) {
        t = dy / denY;
    } else if (!NearZero(denZ)) {
        t = dz / denZ;
    }
    *(f32*)(arg0 + 0x2A0) = ax + mAx * t;
    *(f32*)(arg0 + 0x2A4) = ay + mAy * t;
    *(f32*)(arg0 + 0x2A8) = az + mAz * t;
    f32 la2 = (daz * daz) + ((dax * dax) + (day * day));
    f32 la = lbl_805FE3B0;
    if (la2 > lbl_805FE3B0) {
        la = (f32)sqrt((f64)la2);
    }
    f32 lb2 = (dbz * dbz) + ((dbx * dbx) + (dby * dby));
    f32 lb = lbl_805FE3B0;
    if (lb2 > lbl_805FE3B0) {
        lb = (f32)sqrt((f64)lb2);
    }
    f32 angle = lbl_805FE3B0;
    f32 prod = la * lb;
    if (!(prod <= lbl_805FE3B0)) {
        f32 c = ((daz * dbz) + ((dax * dbx) + (day * dby))) / prod;
        if (c < lbl_805FE3B8) {
            c = lbl_805FE3B8;
        } else if (c > lbl_805FE3B4) {
            c = lbl_805FE3B4;
        }
        angle = lbl_805FE3D8 * (f32)fn_8033F1F4((f64)c);
    }
    *(f32*)(arg0 + 0x2B0) = lbl_805FE3DC * angle;
}

// 0x800D163C
// Debug/menu input: buttons 4 and 5 fire two game-flow signals; button 6 steps the chosen index
// (+0x184) and button 7 the current index (+0x180) through the six vec3 at +0x188 until a
// near-orthogonal pair is found and feeds it to +0x150C and +0x2908 (button 6 then sends the
// chosen vector to the four members +0x1464/+0x150C/+0x2860/+0x2908 through vtable +0x10).
// Always ends with fn_800D1D3C(dt).
void fn_800D163C(u8* arg0, f32 dt) {
    if (fn_80147554(4)) {
        GameFlowSignal((s32)0xC608C842u, 0, 0);
    } else if (fn_80147554(5)) {
        GameFlowSignal((s32)0x117741C3u, 0, 0);
    } else if (fn_80147554(6)) {
        CycleStep(arg0, 0x184);
        if (!CycleOrthogonal(arg0)) {
            CycleFindOrthogonal(arg0);
        }
        f32 v[3];
        f32* sel = (f32*)(arg0 + 0x188 + *(s32*)(arg0 + 0x184) * 0xC);
        v[0] = sel[0];
        v[1] = sel[1];
        v[2] = sel[2];
        (*(FnVec*)(*(u8**)(arg0 + 0x1464) + 0x10))(arg0 + 0x1464, v);
        (*(FnVec*)(*(u8**)(arg0 + 0x150C) + 0x10))(arg0 + 0x150C, v);
        (*(FnVec*)(*(u8**)(arg0 + 0x2860) + 0x10))(arg0 + 0x2860, v);
        (*(FnVec*)(*(u8**)(arg0 + 0x2908) + 0x10))(arg0 + 0x2908, v);
    } else if (fn_80147554(7)) {
        CycleFindOrthogonal(arg0);
    }
    fn_800D1D3C(arg0, dt);
}

// 0x800E3304
// Camera-follow helper: writes the camera's eye point (+0x80) to arg1 and the unit direction to
// the look-at point (+0x98) to arg2, then turns the camera yaw (+0xB8) towards the heading of the
// object's vector at +0x28 (flipped to face away from the direction), scaled by 300 * (+0x58) *
// (+0x44) * speed (length of the vec3 at (+0x2C0)+0x770) * dt. Only runs when +0x55 is set.
// Always returns 0.
s32 fn_800E3304(u8* arg0, f32* arg1, f32* arg2, f32 dt) {
    if (fn_800DECD8(arg0, dt) == 0) {
        return 0;
    }
    if (*(u8*)(arg0 + 0x55) != 0) {
        u8* cam = *(u8**)(lbl_803C4298 + 0x8);
        f32 ex = *(f32*)(cam + 0x80);
        f32 ey = *(f32*)(cam + 0x84);
        f32 ez = *(f32*)(cam + 0x88);
        f32 dx = *(f32*)(cam + 0x98) - ex;
        f32 dy = *(f32*)(cam + 0x9C) - ey;
        f32 dz = *(f32*)(cam + 0xA0) - ez;
        arg1[0] = ex;
        arg1[1] = ey;
        arg1[2] = ez;
        f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
        arg2[0] = dx;
        arg2[1] = dy;
        arg2[2] = dz;
        f32 ux, uy, uz;
        if (lbl_805FE9A0 == len2) {
            ux = lbl_805FE9A0;
            uy = lbl_805FE9A0;
            uz = lbl_805FE9A0;
        } else {
            f32 inv = (f32)(1.0 / sqrt((f64)len2));
            ux = dx * inv;
            uy = dy * inv;
            uz = dz * inv;
        }
        arg2[0] = ux;
        arg2[1] = uy;
        arg2[2] = uz;
        f32 vx = *(f32*)(arg0 + 0x28);
        f32 vy = *(f32*)(arg0 + 0x2C);
        f32 vz = *(f32*)(arg0 + 0x30);
        f32 dotFwd = (vz * uz) + ((vx * ux) + (vy * uy));
        f32 dotBack = (vz * -uz) + ((vx * -ux) + (vy * -uy));
        if (dotBack > dotFwd) {
            vx = -vx;
            vy = -vy;
            vz = -vz;
        }
        f32 yaw = NormalizeAngle(fn_801F60F8(vy, vx));
        cam = *(u8**)(lbl_803C4298 + 0x8);
        f32 turn = NormalizeAngle(yaw - *(f32*)(cam + 0xB4));
        u8* sp = *(u8**)(cam + 0x2C0);
        f32 sx = *(f32*)(sp + 0x770);
        f32 sy = *(f32*)(sp + 0x774);
        f32 sz = *(f32*)(sp + 0x778);
        f32 s2 = (sz * sz) + ((sx * sx) + (sy * sy));
        f32 speed = lbl_805FE9A0;
        if (s2 > lbl_805FE9A0) {
            speed = (f32)sqrt((f64)s2);
        }
        f32 k = lbl_805FE9C8 * *(f32*)(arg0 + 0x58);
        k = k * *(f32*)(arg0 + 0x44);
        k = k * speed;
        k = turn * k;
        k = k * dt;
        *(f32*)(cam + 0xB8) = *(f32*)(cam + 0xB8) + k;
    }
    return 0;
}

// 0x800E3CF8
// Camera-follow helper for a tracked entity: looks up the entity by the handle at +0x6C, builds
// a point in front of the camera target (+0x60/+0x64 distance), turns the offset to the
// entity's anchor (locator or entity point) by the angle +0x34, and writes the direction to arg2
// and the point pulled back by (+0x58) along it to arg1 (through the camera collision query
// when +0x56 is set). Returns 1 on success, 0 when the entity is missing or the update is idle.
s32 fn_800E3CF8(u8* arg0, f32* arg1, f32* arg2, f32 dt) {
    if (fn_800DECD8(arg0, dt) == 0) {
        return 0;
    }
    u8* ent;
    if (*(s32*)(arg0 + 0x68) != 0) {
        u32 handle = *(u32*)(arg0 + 0x6C);
        ent = (u8*)fn_801B7198(&handle, 0);
    } else {
        ent = NULL;
    }
    if (ent == NULL) {
        return 0;
    }
    u8* entPos = *(u8**)(ent + 0x4);
    u8* tgt = *(u8**)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0);
    u8* tp = *(u8**)(tgt + 0x4);
    // (the original also evaluates a distance falloff ratio from the two points here; unused)
    f32 off = lbl_805FEA30 * lbl_805FEA28;
    f32 t = *(f32*)(arg0 + 0x64) + off;
    f32 zero = lbl_805FEA28;
    f32 px = *(f32*)(tp + 0x0) + zero * t;
    f32 py = *(f32*)(tp + 0x4) + zero * t;
    f32 pz = *(f32*)(tp + 0x8) + lbl_805FEA2C * t;
    f32 rx, ry, rz;
    if (*(s32*)(arg0 + 0x70) != 0) {
        u16 flags = *(u16*)(arg0 + 0x74);
        s32 idx = (flags & 1) ? *(u8*)(arg0 + 0x76) : -1;
        f32 rec[12] = { 0 };   // locator record (uninitialized on the stack in the original)
        if (idx != -1) {
            if (idx < (s32)(intptr_t)fn_801B55A8(ent)) {
                fn_801B55F4(ent, (u8*)rec, idx);
            }
        } else {
            s32 idx2 = (flags & 2) ? *(u8*)(arg0 + 0x76) : -1;
            if (idx2 != -1) {
                u8* e2 = *(u8**)(ent + 0x98) + idx2 * 0xC8;
                for (s32 i = 0; i < 12; i++) {
                    rec[i] = *(f32*)(e2 + 0x38 + i * 4);
                }
            }
        }
        rx = rec[9] - px;
        ry = rec[10] - py;
        rz = rec[11] - pz;
    } else {
        f32 t2 = *(f32*)(arg0 + 0x60) + lbl_805FEA28;
        f32 ox = *(f32*)(entPos + 0x0) + zero * t2;
        f32 oy = *(f32*)(entPos + 0x4) + zero * t2;
        f32 oz = *(f32*)(entPos + 0x8) + lbl_805FEA2C * t2;
        rx = ox - px;
        ry = oy - py;
        rz = oz - pz;
    }
    f32 ang = lbl_805FEA38 * (lbl_805FEA34 * *(f32*)(arg0 + 0x34));
    f32 c = (f32)cosf(ang);
    f32 s = (f32)sinf(ang);
    f32 nx = (rx * c) + (ry * -s);
    f32 ny = (ry * c) + (rx * s);
    f32 len2 = (rz * rz) + ((nx * nx) + (ny * ny));
    f32 ux, uy, uz;
    if (lbl_805FEA28 == len2) {
        ux = lbl_805FEA28;
        uy = lbl_805FEA28;
        uz = lbl_805FEA28;
    } else {
        f32 inv = (f32)(1.0 / sqrt((f64)len2));
        ux = nx * inv;
        uy = ny * inv;
        uz = rz * inv;
    }
    f32 back = *(f32*)(arg0 + 0x58) + off;
    f32 qx = px - ux * back;
    f32 qy = py - uy * back;
    f32 qz = pz - uz * back;
    if (*(u8*)(arg0 + 0x56) != 0) {
        CamQuery q;
        CamResult r;
        q.from[0] = px;
        q.from[1] = py;
        q.from[2] = pz;
        q.to[0] = qx;
        q.to[1] = qy;
        q.to[2] = qz;
        q.f18 = lbl_805FEA2C;
        q.f1C = lbl_805FEA28;
        q.b20 = 1;
        q.w24 = 4;
        q.w28 = 0;
        q.w2C = 2;
        r.hit = 0;
        r.v04[0] = lbl_805FEA28; r.v04[1] = lbl_805FEA28; r.v04[2] = lbl_805FEA28;
        r.v10[0] = lbl_805FEA28; r.v10[1] = lbl_805FEA28; r.v10[2] = lbl_805FEA28;
        r.pos[0] = lbl_805FEA28; r.pos[1] = lbl_805FEA28; r.pos[2] = lbl_805FEA28;
        fn_800DA8F0((u8*)&q, (u8*)&r);
        arg1[0] = r.pos[0];
        arg1[1] = r.pos[1];
        arg1[2] = r.pos[2];
    } else {
        arg1[0] = qx;
        arg1[1] = qy;
        arg1[2] = qz;
    }
    arg2[0] = ux;
    arg2[1] = uy;
    arg2[2] = uz;
    return 1;
}

// 0x8019578C
// Spawns a named string/effect through the factory object at (obj+8)->+8. When a 3x4 matrix is
// given it is first made current (mode 0: copied straight through fn_801D679C; mode 1: the 3x3
// part is divided by the object's scale byte and loaded with fn_80155724). While spawning the
// view-matrix mode (lbl_805F9614) is forced to 1. With e == 0 it calls the factory's vtable
// slot 0x8 with scale 1.0, otherwise slot 0xC with e and the length of the matrix' first row
// as the scale. Returns the spawned record, or NULL for an empty name.
u8* fn_8019578C(void* obj, const char* str, const f32* mat, s32 a, s32 b, u32 len, s32 c, s32 d, s32 e) {
    f32 scale = lbl_805FF8C0;
    if (str == NULL || strlen(str) == 0) {
        return NULL;
    }
    u8* fac = *(u8**)(*(u8**)((u8*)obj + 0x8) + 0x8);
    if (mat != NULL) {
        s32 mode = *(s32*)(fac + 0x4);
        if (mode == 0) {
            fn_801D679C(*(void**)((u8*)obj + 0x4), (f32*)mat);
        } else if (mode == 1) {
            f32 m[12];
            for (s32 i = 0; i < 12; i++) {
                m[i] = mat[i];
            }
            f32 inv = lbl_805FF8C0 / fn_80197FE0(fac);
            for (s32 i = 0; i < 9; i++) {
                m[i] = m[i] * inv;
            }
            fn_80155724((u8*)m);
            // (the original then measures the first row of m; the result is unused)
            scale = lbl_805FF8C0;
        }
    }
    u32 savedMode = lbl_805F9614;
    lbl_805F9614 = 1;
    u8* ret;
    if (e == 0) {
        ret = (*(FnSpawn*)(*(u8**)fac + 0x8))(fac, str, a, b, len, c, d, scale);
    } else {
        if (mat != NULL) {
            f32 l2 = (mat[2] * mat[2]) + ((mat[0] * mat[0]) + (mat[1] * mat[1]));
            scale = lbl_805FF8C4;
            if (l2 > lbl_805FF8C4) {
                scale = (f32)sqrt((f64)l2);
            }
        }
        ret = (*(FnSpawn*)(*(u8**)fac + 0xC))(fac, str, e, b, len, c, d, scale);
    }
    lbl_805F9614 = savedMode;
    return ret;
}

}  // extern "C"
