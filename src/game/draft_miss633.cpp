// Small functions in 0x800B9138-0x800C324C (camera-follow target helper, grow-juice style
// object bookkeeping, sorted-by-key object list).
#include "types.h"
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };                                   // as in props.cpp

extern "C" {
void  EulerToQuat(Quat* out, const Vec3* euler);                    // 0x80145B00
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32   RaySceneTest(void* ray);                                      // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                                // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);
void  fn_800BCC10(u8* p);                                           // 0x800BCC10
void  fn_800BD978(u8* p);                                           // 0x800BD978
void  fn_800BF700(u8* p);                                           // 0x800BF700
void  fn_801455E0(f32* out, u8* quat);                              // 0x801455E0
void  fn_801458B0(f32* quat, f32* a, f32* b);                       // 0x801458B0
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);         // 0x80146180
u8*   fn_801B3308(void);                                            // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                  // 0x801B3378
void  fn_801B6F34(u8* p);                                           // 0x801B6F34
}

extern "C++" void* gEventMgr;                                       // 0x805FB6C8
extern f32 lbl_80346D60[];          // 0x80346D60 (f32 table indexed by RaySceneTest)
extern f32 lbl_80346E30[];          // 0x80346E30 (f32 table indexed by RaySceneTest)
extern f32 lbl_805FE208;            // 0x805FE208 (0.0)
extern f32 lbl_805FE20C;            // 0x805FE20C (1.0)
extern f32 lbl_805FE220;            // 0x805FE220 (lower dead-zone bound)
extern f32 lbl_805FE224;            // 0x805FE224 (upper dead-zone bound)
extern f32 lbl_805FE228;            // 0x805FE228 (degrees-to-quarter-turn scale)
extern f32 lbl_805FE22C;            // 0x805FE22C (fade speed)
extern f32 lbl_805FE234;            // 0x805FE234 (default value for +0x90)
extern f32 lbl_805FE308;            // 0x805FE308 (0.0)
extern f32 lbl_805FE320;            // 0x805FE320 (1.0)
extern f32 lbl_805FE324;            // 0x805FE324 (scale)
extern f32 lbl_805FE348;            // 0x805FE348 (timer threshold)
extern f32 lbl_805FE350;            // 0x805FE350 (vertical drop)
extern f32 lbl_805FE354;            // 0x805FE354 (scale)

// Marks a scene object's transform dirty (four flag ORs at +0x20, as the original emits them).
static inline void XformTouch(u8* xf) {
    *(u32*)(xf + 0x20) |= 0x8000;
    *(u32*)(xf + 0x20) |= 0x10000000;
    *(u32*)(xf + 0x20) |= 0x4000;
    *(u32*)(xf + 0x20) |= 0x10000000;
}

// Sort key of an object in the list at +0xC of fn_800C324C's owner.
static inline f32 SortKey(u8* o) {
    return *(f32*)(o + 0x264);
}

extern "C" {

// 0x800B9138
// Moves +0x29C toward the target at +0x2A0 (speed +0x2A4); while they differ, rebuilds the
// quaternion at +0x27C from the owner's Euler angles and the axis, positions the object's
// transform from it, and aligns the orientation. Then fades +0x2AC up or down by +0x2A9 and
// writes it into the +0x44 float of the sub-block at +0x90 of the object in slot +0x278.
void fn_800B9138(u8* arg0, f32 dt) {
    s32 settled = 0;
    f32 diff = *(f32*)(arg0 + 0x29C) - *(f32*)(arg0 + 0x2A0);
    if (diff > lbl_805FE220 && diff < lbl_805FE224) {
        settled = 1;
    }
    if (settled == 0) {
        f32 target = *(f32*)(arg0 + 0x2A0);
        f32 cur = *(f32*)(arg0 + 0x29C);
        f32 step = *(f32*)(arg0 + 0x2A4) * dt;
        if (fabsf(target - cur) < step) {
            *(f32*)(arg0 + 0x29C) = target;
        } else if (target > cur) {
            *(f32*)(arg0 + 0x29C) = cur + step;
        } else {
            *(f32*)(arg0 + 0x29C) = cur - step;
        }
        f32 zero = lbl_805FE208;
        f32 one = lbl_805FE20C;
        *(f32*)(arg0 + 0x27C) = zero;
        u8* owner = *(u8**)(arg0 + 0x270);
        *(f32*)(arg0 + 0x280) = zero;
        *(f32*)(arg0 + 0x284) = zero;
        *(f32*)(arg0 + 0x288) = one;
        Vec3 euler;
        euler.x = *(f32*)(owner + 0x14) * lbl_805FE228;
        euler.y = *(f32*)(owner + 0x18) * lbl_805FE228;
        euler.z = *(f32*)(owner + 0x1C) * lbl_805FE228;
        EulerToQuat((Quat*)(arg0 + 0x27C), &euler);
        Vec3 axis = QuatGetAxisY((const Quat*)(arg0 + 0x28C));
        fn_80146180(arg0 + 0x27C, &axis, *(f32*)(arg0 + 0x29C));
        u8* owner2 = *(u8**)(arg0 + 0x270);
        Vec3 fwd = QuatGetAxisY((const Quat*)(arg0 + 0x27C));
        f32 dist = *(f32*)(arg0 + 0x274);
        f32 px = fwd.x * dist;
        f32 py = fwd.y * dist;
        f32 pz = fwd.z * dist;
        u8* xf = *(u8**)(arg0 + 0x4);
        *(f32*)(xf + 0x0) = px + *(f32*)(owner2 + 0x8);
        *(f32*)(xf + 0x4) = py + *(f32*)(owner2 + 0xC);
        *(f32*)(xf + 0x8) = pz + *(f32*)(owner2 + 0x10);
        XformTouch(*(u8**)(arg0 + 0x4));
        Vec3 dir = Vec3Normalized(QuatGetAxisY((const Quat*)(arg0 + 0x27C)));
        // side = dir x (1, 0, 0)
        f32 side[3];
        side[0] = dir.y * zero - dir.z * zero;
        side[1] = dir.z * one - dir.x * zero;
        side[2] = dir.x * zero - dir.y * one;
        fn_801458B0((f32*)(arg0 + 0x27C), side, &dir.x);
        u8* xf2 = *(u8**)(arg0 + 0x4);
        *(f32*)(xf2 + 0xC) = *(f32*)(arg0 + 0x27C);
        *(f32*)(xf2 + 0x10) = *(f32*)(arg0 + 0x280);
        *(f32*)(xf2 + 0x14) = *(f32*)(arg0 + 0x284);
        *(f32*)(xf2 + 0x18) = *(f32*)(arg0 + 0x288);
        XformTouch(*(u8**)(arg0 + 0x4));
    }
    if (*(s32*)(arg0 + 0x278) != -1) {
        if (*(u8*)(arg0 + 0x2A9) != 0) {
            f32 v = *(f32*)(arg0 + 0x2AC);
            if (v < lbl_805FE20C) {
                *(f32*)(arg0 + 0x2AC) = v + lbl_805FE22C * dt;
            }
            if (*(f32*)(arg0 + 0x2AC) > lbl_805FE20C) {
                *(f32*)(arg0 + 0x2AC) = lbl_805FE20C;
            }
        } else {
            f32 v = *(f32*)(arg0 + 0x2AC);
            if (v > lbl_805FE208) {
                *(f32*)(arg0 + 0x2AC) = v - lbl_805FE22C * dt;
            }
            if (*(f32*)(arg0 + 0x2AC) < lbl_805FE208) {
                *(f32*)(arg0 + 0x2AC) = lbl_805FE208;
            }
        }
        f32 fade = *(f32*)(arg0 + 0x2AC);
        u8* sub = ((u8**)(arg0 + 0x264))[*(s32*)(arg0 + 0x278)];
        sub = *(u8**)(sub + 0x90);
        f32 a = *(f32*)(sub + 0x38);
        f32 b = *(f32*)(sub + 0x3C);
        f32 c = *(f32*)(sub + 0x40);
        *(f32*)(sub + 0x38) = a;
        *(f32*)(sub + 0x3C) = b;
        *(f32*)(sub + 0x40) = c;
        *(f32*)(sub + 0x44) = fade;
    }
}

// 0x800BA2E8
// Releases and clears every slot of the object table: three groups of six, two groups of two,
// the plain pointer ranges, the single block at +0x238, and the key at +0x3BC in the global list.
void fn_800BA2E8(u8* arg0) {
    s32 i;
    for (i = 0; i < 6; i++) {
        fn_800BCC10(((u8**)(arg0 + 0xDC))[i]);
        ((u8**)(arg0 + 0xDC))[i] = NULL;
        fn_800BCC10(((u8**)(arg0 + 0x10C))[i]);
        ((u8**)(arg0 + 0x10C))[i] = NULL;
        fn_800BCC10(((u8**)(arg0 + 0xF4))[i]);
        ((u8**)(arg0 + 0xF4))[i] = NULL;
    }
    for (i = 0; i < 18; i++) {
        ((u8**)(arg0 + 0x94))[i] = NULL;
    }
    for (i = 0; i < 2; i++) {
        fn_800BD978(((u8**)(arg0 + 0x160))[i]);
        ((u8**)(arg0 + 0x160))[i] = NULL;
        fn_800BD978(((u8**)(arg0 + 0x158))[i]);
        ((u8**)(arg0 + 0x158))[i] = NULL;
        fn_800BD978(((u8**)(arg0 + 0x150))[i]);
        ((u8**)(arg0 + 0x150))[i] = NULL;
        fn_800BD978(((u8**)(arg0 + 0x168))[i]);
        ((u8**)(arg0 + 0x168))[i] = NULL;
    }
    for (i = 0; i < 8; i++) {
        ((u8**)(arg0 + 0x130))[i] = NULL;
    }
    for (i = 0; i < 50; i++) {
        ((u8**)(arg0 + 0x170))[i] = NULL;
    }
    if (*(u8**)(arg0 + 0x238) != NULL) {
        fn_800BF700(*(u8**)(arg0 + 0x238));
    }
    *(u8**)(arg0 + 0x238) = NULL;
    *(u8**)(arg0 + 0x23C) = NULL;
    *(f32*)(arg0 + 0x90) = lbl_805FE234;
    fn_801B3378(fn_801B3308(), arg0 + 0x3BC);
    *(u8**)(arg0 + 0x488) = NULL;
}

// 0x800BC704
// Returns 0 when any live object in the 18-slot list (+0x94) or any of the eight slots at
// +0x130 lies within the RaySceneTest-scaled radius of point pos, else 1.
s32 fn_800BC704(u8* arg0, f32* pos) {
    f32 r = lbl_80346D60[RaySceneTest(arg0)];
    f32 r2 = r * r;
    u8** list = (u8**)(arg0 + 0x94);
    s32 i;
    for (i = 0; i < 18; i++) {
        u8* o = list[i];
        if (*(u8*)(o + 0x298) == 1) {
            f32 dx = *(f32*)(o + 0x28C) - pos[0];
            f32 dy = *(f32*)(o + 0x290) - pos[1];
            f32 dz = *(f32*)(o + 0x294) - pos[2];
            if (dz * dz + (dx * dx + dy * dy) < r2) {
                return 0;
            }
        }
    }
    u8** objs = (u8**)(arg0 + 0x130);
    for (i = 0; i < 8; i++) {
        u8* o = objs[i];
        if (*(u8*)(o + 0x264) != 0) {
            u8* xf = *(u8**)(o + 0x4);
            f32 dx = *(f32*)(xf + 0x0) - pos[0];
            f32 dy = *(f32*)(xf + 0x4) - pos[1];
            f32 dz = *(f32*)(xf + 0x8) - pos[2];
            if (dz * dz + (dx * dx + dy * dy) < r2) {
                return 0;
            }
        }
    }
    return 1;
}

// Rotates the position trail (+0x14 into +0x20) of the object reached through the one-element
// containers at +0x8C; the new position is the owner's transform position.
static inline void RotateTrail(u8* arg0) {
    u8* h = *(u8**)(arg0 + 0x8C);
    if (h != NULL) {
        if (*(u32*)(h + 0x10) == 1) {
            u8* p = **(u8***)(h + 0xC);
            if (p != NULL) {
                if (*(u32*)(p + 0x28) == 1) {
                    u8* q = **(u8***)(p + 0x24);
                    if (*(s32*)(q + 0xC) == 0) {
                        *(f32*)(q + 0x20) = *(f32*)(q + 0x14);
                        *(f32*)(q + 0x24) = *(f32*)(q + 0x18);
                        *(f32*)(q + 0x28) = *(f32*)(q + 0x1C);
                        u8* xf = *(u8**)(arg0 + 0x4);
                        *(f32*)(q + 0x14) = *(f32*)(xf + 0x0);
                        *(f32*)(q + 0x18) = *(f32*)(xf + 0x4);
                        *(f32*)(q + 0x1C) = *(f32*)(xf + 0x8);
                        if (*(s32*)(arg0 + 0x290) != 5) {
                            *(f32*)(q + 0x24) = lbl_805FE308;
                            *(f32*)(q + 0x18) = lbl_805FE308;
                        }
                    }
                }
            }
        }
    }
}

// 0x800BE5EC
// Sets mode 5, stores from/to at +0x270/+0x27C and a table value at +0x288, places the
// transform at from, aims the orientation along to - from (scaled by 18, kept at +0x264),
// sends event 0x221E3DE2 to itself, and rotates the position trail of the linked object (twice).
void fn_800BE5EC(u8* arg0, f32* from, f32* to) {
    *(s32*)(arg0 + 0x290) = 5;
    f32 tbl = lbl_80346E30[RaySceneTest(arg0)];
    *(f32*)(arg0 + 0x270) = from[0];
    u8* xf = *(u8**)(arg0 + 0x4);
    *(f32*)(arg0 + 0x274) = from[1];
    *(f32*)(arg0 + 0x278) = from[2];
    *(f32*)(arg0 + 0x27C) = to[0];
    *(f32*)(arg0 + 0x288) = tbl;
    *(f32*)(arg0 + 0x280) = to[1];
    *(f32*)(arg0 + 0x284) = to[2];
    Vec3 d;
    d.x = to[0] - from[0];
    d.y = to[1] - from[1];
    *(f32*)(xf + 0x0) = from[0];
    d.z = to[2] - from[2];
    *(f32*)(xf + 0x4) = from[1];
    *(f32*)(xf + 0x8) = from[2];
    XformTouch(*(u8**)(arg0 + 0x4));
    Vec3 n = Vec3Normalized(d);
    f32 sx = n.x * lbl_805FE324;
    f32 sy = n.y * lbl_805FE324;
    f32 sz = n.z * lbl_805FE324;
    *(f32*)(arg0 + 0x264) = sx;
    *(f32*)(arg0 + 0x268) = sy;
    *(f32*)(arg0 + 0x26C) = sz;
    f32 q[4];
    q[0] = lbl_805FE308;
    q[1] = lbl_805FE308;
    q[2] = lbl_805FE308;
    q[3] = lbl_805FE320;
    f32 up[3];
    up[0] = lbl_805FE308;
    up[1] = lbl_805FE320;
    up[2] = lbl_805FE308;
    fn_801458B0(q, (f32*)(arg0 + 0x264), up);
    u8* xf2 = *(u8**)(arg0 + 0x4);
    *(f32*)(xf2 + 0xC) = q[0];
    *(f32*)(xf2 + 0x10) = q[1];
    *(f32*)(xf2 + 0x14) = q[2];
    *(f32*)(xf2 + 0x18) = q[3];
    XformTouch(*(u8**)(arg0 + 0x4));
    EventSend(gEventMgr, arg0, NULL, 0x221E3DE2u, 0, 0, 1, 0);
    fn_801B6F34(arg0);
    RotateTrail(arg0);
    RotateTrail(arg0);
}

// 0x800BFF64
// Counts down the timer at +0x168 by dt. When it expires and a target is selected (index at
// +0x0 into the list at +0x4), drops the target's transform to its saved position minus a
// vertical offset, snaps the first entry's transform to its saved position, clears the
// selection and sends event 0x5520DBE9. Then advances the follow position at +0x14C along the
// axis of the quaternion at +0x158.
void fn_800BFF64(u8* arg0, f32 dt) {
    f32 t = *(f32*)(arg0 + 0x168) - dt;
    *(f32*)(arg0 + 0x168) = t;
    if (t < lbl_805FE348) {
        s32 idx = *(s32*)(arg0 + 0x0);
        if (idx != 0) {
            u8** list = (u8**)(arg0 + 0x4);
            u8* target = list[idx];
            if (target != NULL) {
                f32 sy = *(f32*)(target + 0xCC);
                f32 sx = *(f32*)(target + 0xC8);
                f32 sz = *(f32*)(target + 0xD0);
                u8* xf = *(u8**)(target + 0x4);
                *(f32*)(xf + 0x0) = sx;
                *(f32*)(xf + 0x4) = sy - lbl_805FE350;
                *(f32*)(xf + 0x8) = sz;
                XformTouch(*(u8**)(target + 0x4));
            }
            *(s32*)(arg0 + 0x0) = 0;
            u8* first = list[0];
            if (first != NULL) {
                u8* xf = *(u8**)(first + 0x4);
                *(f32*)(xf + 0x0) = *(f32*)(first + 0xC8);
                *(f32*)(xf + 0x4) = *(f32*)(first + 0xCC);
                *(f32*)(xf + 0x8) = *(f32*)(first + 0xD0);
                XformTouch(*(u8**)(first + 0x4));
            }
            EventSend(gEventMgr, list[*(s32*)(arg0 + 0x0)], NULL, 0x5520DBE9u, 0, 0, 1, 0);
        }
    }
    f32 ax = *(f32*)(arg0 + 0x14C);
    f32 ay = *(f32*)(arg0 + 0x150);
    f32 az = *(f32*)(arg0 + 0x154);
    *(f32*)(arg0 + 0x140) = ax;
    *(f32*)(arg0 + 0x144) = ay;
    *(f32*)(arg0 + 0x148) = az;
    f32 k = lbl_805FE354;
    f32 axis[3];
    fn_801455E0(axis, arg0 + 0x158);
    f32 mx = axis[0] * k;
    f32 my = axis[1] * k;
    f32 mz = axis[2] * k;
    *(f32*)(arg0 + 0x14C) = *(f32*)(arg0 + 0x134) + mx;
    *(f32*)(arg0 + 0x150) = *(f32*)(arg0 + 0x138) + my;
    *(f32*)(arg0 + 0x154) = *(f32*)(arg0 + 0x13C) + mz;
}

// 0x800C324C
// Registers object arg1 (with sort key dt at +0x264) in the owner arg0's list (count at +0x5C,
// entries from +0xC), then keeps that list sorted ascending by the key at +0x264 (an inlined
// quicksort for large lists followed by an insertion sort).
void fn_800C324C(u8* arg0, u8* arg1, f32 dt) {
    typedef void (*VFn1)(u8*);
    typedef void (*VFn2)(u8*, s32);
    typedef Vec3 (*PosFn)(u8*, f32);
    *(f32*)(arg1 + 0x264) = dt;
    *(u8*)(arg1 + 0x270) = 1;
    (*(VFn1*)(*(u8**)arg1 + 0xBC))(arg1);
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(VFn2*)(*(u8**)arg1 + 0x14))(arg1, 1);
    ScopedLockDtor(&lock, -1);
    *(u8**)(arg1 + 0x26C) = arg0;
    Vec3 pos = (*(PosFn*)(*(u8**)(arg0 + 0xD4) + 0x24))(arg0, dt);
    u8* xf = *(u8**)(arg1 + 0x4);
    *(f32*)(xf + 0x0) = pos.x;
    *(f32*)(xf + 0x4) = pos.y;
    *(f32*)(xf + 0x8) = pos.z;
    XformTouch(*(u8**)(arg1 + 0x4));

    u8** arr = (u8**)(arg0 + 0xC);
    arr[*(u32*)(arg0 + 0x5C)] = arg1;
    u32 count = *(u32*)(arg0 + 0x5C) + 1;
    *(u32*)(arg0 + 0x5C) = count;

    if (count > 4) {
        struct Range { u8** hi; u8** lo; };
        Range stk[64];
        Range* sp = &stk[1];     // stk[0] is the terminating sentinel
        u8** lo = arr;
        u8** hi = arr + count - 1;
        while (&stk[0] < sp) {
            s32 n = (s32)(hi - lo);
            u8** mid = lo + ((u32)n >> 1);
            u8** pv = mid;
            // median of three: order lo, mid, hi
            u8* a = *lo;
            u8* b = *mid;
            if (SortKey(b) < SortKey(a)) {
                *mid = a;
                *lo = b;
            }
            b = *mid;
            a = *hi;
            if (SortKey(a) < SortKey(b)) {
                *mid = a;
                *hi = b;
                a = *lo;
                b = *mid;
                if (SortKey(b) < SortKey(a)) {
                    *mid = a;
                    *lo = b;
                }
            }
            u8** i = lo + 1;
            u8** j = hi - 1;
            for (;;) {
                u8* pivot = *pv;
                u8* vi = *i;
                if (SortKey(vi) < SortKey(pivot)) {
                    i++;
                    continue;
                }
                u8* vj;
                for (;;) {
                    vj = *j;
                    if (SortKey(pivot) < SortKey(vj)) {
                        j--;
                    } else {
                        break;
                    }
                }
                if (i < j) {
                    *i = vj;
                    *j = vi;
                    if (pv == i) {
                        pv = j;
                    } else if (pv == j) {
                        pv = i;
                    }
                    i++;
                    j--;
                    if (i <= j) {
                        continue;
                    }
                } else if (i == j) {
                    i++;
                    j--;
                }
                break;
            }
            s32 nLeft = (s32)(j - lo);
            s32 nRight = (s32)(hi - i);
            if (nLeft <= 4) {
                if (nRight <= 4) {
                    sp--;
                    lo = sp->lo;
                    hi = sp->hi;
                } else {
                    lo = i;
                }
            } else if (nRight <= 4) {
                hi = j;
            } else if (nLeft > nRight) {
                sp->lo = lo;
                sp->hi = j;
                sp++;
                lo = i;
            } else {
                sp->lo = i;
                sp->hi = hi;
                sp++;
                hi = j;
            }
        }
    }

    // Insertion sort; the smallest of the first five goes to the front as a sentinel.
    u8** last = arr + count - 1;
    u8** end = arr + 4;
    if (end > last) {
        end = last;
    }
    u8** minp = arr;
    for (u8** p = arr + 1; p <= end; p++) {
        if (SortKey(*p) < SortKey(*minp)) {
            minp = p;
        }
    }
    if (minp != arr) {
        u8* t = *minp;
        *minp = *arr;
        *arr = t;
    }
    for (u8** p = arr + 2; p <= last; p++) {
        u8* v = *p;
        u8** j = p - 1;
        while (SortKey(v) < SortKey(*j)) {
            j--;
        }
        u8** dst = j + 1;
        if (dst != p) {
            for (u8** q = p; q > dst; q--) {
                *q = *(q - 1);
            }
            *dst = v;
        }
    }
}

}  // extern "C"
