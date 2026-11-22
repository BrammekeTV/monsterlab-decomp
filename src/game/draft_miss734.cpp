// Grow juice weapon / animation-track functions in 0x800Bxxxx-0x800Cxxxx (spawn, matrix concat, ring walk, sorts).
#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void EntitySyncTransform(void* entity, int a);                                                  // 0x801B4F4C
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);     // 0x801BA190
void QuatFromMtx(Quat* out, const f32 m[12]);                                                   // 0x801456E4
void fn_801455E0(f32* out, u8* quat);                                                           // 0x801455E0
void fn_80146180(void* quat, const Vec3* axis, f32 angle);                                      // 0x80146180 (rotate quat about axis)
u8*  fn_8006FBDC(void);                                                                         // 0x8006FBDC
u8*  fn_800BC6BC(u8* mgr);                                                                      // 0x800BC6BC
void fn_800C176C(f32* out, u8* obj);                                                            // 0x800C176C
u8*  fn_800C2FF0(u8* p);                                                                        // 0x800C2FF0
void fn_801B6F34(u8* p);                                                                        // 0x801B6F34
void fn_801EC080(u8* a, u8* b);                                                                 // 0x801EC080
}
extern "C++" void* gEventMgr;                                                                   // 0x805FB6C8

typedef f32 (*DurFn)(u8*);
typedef void (*EvalFn)(f32*, u8*, f32);
typedef f32 (*RootDurFn)(u8*);
typedef Quat (*QuatEvalFn)(u8*, f32);
typedef s32 (*Fn2)(u8*, u8*);

// Marks the transform dirty (same four flag ors the original does after each write).
static inline void MarkXformDirty(u8* t) {
    *(u32*)(t + 0x20) |= 0x8000;
    *(u32*)(t + 0x20) |= 0x10000000;
    *(u32*)(t + 0x20) |= 0x4000;
    *(u32*)(t + 0x20) |= 0x10000000;
}

// Ring of u8* slots: +0x6C head, +0x70 tail, +0x74 capacity, +0x78 array, +0x7C current slot.
// Positions the current slot on the tail element; returns that element or NULL.
static u8* RingSeekTail(u8* ring) {
    s32 head = *(s32*)(ring + 0x6C);
    if (head < 0 || *(s32*)(ring + 0x70) < 0) return NULL;
    s32 tail = *(s32*)(ring + 0x70);
    s32 cap = *(s32*)(ring + 0x74);
    bool ok = false;
    if (tail >= 0 && tail < cap) {
        s32 probe = tail;
        s32 end = tail;
        if (tail < head) {
            probe = tail + cap;
            end = tail + cap;
        }
        ok = head <= probe && probe <= end;
    }
    u8** cur = ok ? *(u8***)(ring + 0x78) + tail : NULL;
    *(u8***)(ring + 0x7C) = cur;
    return cur != NULL ? *cur : NULL;
}

// Steps the current slot back one (wrapping); NULL at the head. Returns the new element or NULL.
static u8* RingStepBack(u8* ring) {
    u8** cur = *(u8***)(ring + 0x7C);
    if (cur == NULL) return NULL;
    u8** base = *(u8***)(ring + 0x78);
    s32 head = *(s32*)(ring + 0x6C);
    s32 idx = (s32)(cur - base);
    u8** next;
    if (idx == head) {
        next = NULL;
    } else {
        s32 p = idx - 1;
        if (p < 0) p = *(s32*)(ring + 0x74) - 1;
        bool ok = false;
        if (p >= 0) {
            s32 cap = *(s32*)(ring + 0x74);
            if (p < cap) {
                s32 probe = p;
                s32 end = *(s32*)(ring + 0x70);
                if (end < head) {
                    if (p <= end) probe = p + cap;
                    end += cap;
                }
                ok = head <= probe && probe <= end;
            }
        }
        next = ok ? base + p : NULL;
    }
    *(u8***)(ring + 0x7C) = next;
    return next != NULL ? *next : NULL;
}

extern "C" {

// 0x800BF370
// Spawns a projectile object: copies the owner's aim quat rotated about +Y by farg0, copies
// owner fields, sets its direction (owner vec at +0x264 length * aim axis) and position
// (arg1 + axis * farg1), then sends the spawn event. Returns 1 if spawned, 0 if no slot.
s32 fn_800BF370(u8* arg0, f32* arg1, f32 farg0, f32 farg1) {
    u8* mgr = fn_8006FBDC();
    u8* obj = fn_800BC6BC(mgr);
    if (obj == NULL) return 0;

    f32 q[4];
    u8* ownerXf = *(u8**)(arg0 + 0x4);
    q[0] = *(f32*)(ownerXf + 0xC);
    q[1] = *(f32*)(ownerXf + 0x10);
    q[2] = *(f32*)(ownerXf + 0x14);
    q[3] = *(f32*)(ownerXf + 0x18);
    Vec3 up = { 0.0f, 1.0f, 0.0f };
    fn_80146180(q, &up, farg0);

    u8* xf = *(u8**)(obj + 0x4);
    *(f32*)(xf + 0xC) = q[0];
    *(f32*)(xf + 0x10) = q[1];
    *(f32*)(xf + 0x14) = q[2];
    *(f32*)(xf + 0x18) = q[3];
    MarkXformDirty(*(u8**)(obj + 0x4));

    *(s32*)(obj + 0x290) = *(s32*)(arg0 + 0x290);
    *(f32*)(obj + 0x288) = *(f32*)(arg0 + 0x288);

    Vec3 axis = QuatGetAxisY((const Quat*)q);

    f32 vx = *(f32*)(arg0 + 0x264);
    f32 vy = *(f32*)(arg0 + 0x268);
    f32 vz = *(f32*)(arg0 + 0x26C);
    f32 lenSq = vz * vz + (vx * vx + vy * vy);
    f32 len = 0.0f;
    if (lenSq > 0.0f) len = sqrtf(lenSq);

    *(f32*)(obj + 0x264) = axis.x * len;
    *(f32*)(obj + 0x268) = axis.y * len;
    *(f32*)(obj + 0x26C) = axis.z * len;

    f32 ox = arg1[0] + axis.x * farg1;
    f32 oy = arg1[1] + axis.y * farg1;
    f32 oz = arg1[2] + axis.z * farg1;
    xf = *(u8**)(obj + 0x4);
    *(f32*)(xf + 0x0) = ox;
    *(f32*)(xf + 0x4) = oy;
    *(f32*)(xf + 0x8) = oz;
    MarkXformDirty(*(u8**)(obj + 0x4));

    EventSend(gEventMgr, obj, NULL, 0x96C877C4u, 0, 0, 1, 0);
    fn_801B6F34(obj);
    return 1;
}

// 0x800C180C
// Concatenates the 3x4 matrix at entry+0x38 (entry = table at +0x98 of arg1, index +0x268, stride
// 0xC8) with the matrix at arg0+0x270, writes translation + quaternion into arg0's transform,
// syncs it, then calls virtual slot +0xC8 on arg0.
s32 fn_800C180C(u8* arg0, u8* arg1) {
    u8* e = *(u8**)(arg1 + 0x98) + *(s32*)(arg1 + 0x268) * 0xC8;
    f32 a0 = *(f32*)(arg0 + 0x270), a1 = *(f32*)(arg0 + 0x274), a2 = *(f32*)(arg0 + 0x278);
    f32 f11 = *(f32*)(e + 0x38);
    f32 f8 = *(f32*)(e + 0x44);
    f32 f5 = *(f32*)(e + 0x50);
    f32 f10 = *(f32*)(e + 0x3C);
    f32 f9 = *(f32*)(e + 0x40);
    f32 f7 = *(f32*)(e + 0x48);
    f32 f6 = *(f32*)(e + 0x4C);
    f32 f4 = *(f32*)(e + 0x54);
    f32 f3 = *(f32*)(e + 0x58);
    f32 f1 = *(f32*)(e + 0x5C);
    f32 f0 = *(f32*)(e + 0x60);
    f32 f2 = *(f32*)(e + 0x64);

    f32 m[12];
    m[0] = f5 * a2 + (f11 * a0 + f8 * a1);
    m[3] = f5 * *(f32*)(arg0 + 0x284) + (f11 * *(f32*)(arg0 + 0x27C) + f8 * *(f32*)(arg0 + 0x280));
    m[6] = f5 * *(f32*)(arg0 + 0x290) + (f11 * *(f32*)(arg0 + 0x288) + f8 * *(f32*)(arg0 + 0x28C));
    m[9] = f1 + (f5 * *(f32*)(arg0 + 0x29C) + (f11 * *(f32*)(arg0 + 0x294) + f8 * *(f32*)(arg0 + 0x298)));
    m[1] = f4 * a2 + (f10 * a0 + f7 * a1);
    m[4] = f4 * *(f32*)(arg0 + 0x284) + (f10 * *(f32*)(arg0 + 0x27C) + f7 * *(f32*)(arg0 + 0x280));
    m[7] = f4 * *(f32*)(arg0 + 0x290) + (f10 * *(f32*)(arg0 + 0x288) + f7 * *(f32*)(arg0 + 0x28C));
    m[10] = f0 + (f4 * *(f32*)(arg0 + 0x29C) + (f10 * *(f32*)(arg0 + 0x294) + f7 * *(f32*)(arg0 + 0x298)));
    m[2] = f3 * a2 + (f9 * a0 + f6 * a1);
    m[5] = f3 * *(f32*)(arg0 + 0x284) + (f9 * *(f32*)(arg0 + 0x27C) + f6 * *(f32*)(arg0 + 0x280));
    m[8] = f3 * *(f32*)(arg0 + 0x290) + (f9 * *(f32*)(arg0 + 0x288) + f6 * *(f32*)(arg0 + 0x28C));
    m[11] = f2 + (f3 * *(f32*)(arg0 + 0x29C) + (f9 * *(f32*)(arg0 + 0x294) + f6 * *(f32*)(arg0 + 0x298)));

    u8* xf = *(u8**)(arg0 + 0x4);
    *(f32*)(xf + 0x0) = m[9];
    *(f32*)(xf + 0x4) = m[10];
    *(f32*)(xf + 0x8) = m[11];
    MarkXformDirty(*(u8**)(arg0 + 0x4));

    Quat q;
    QuatFromMtx(&q, m);
    xf = *(u8**)(arg0 + 0x4);
    *(f32*)(xf + 0xC) = q.x;
    *(f32*)(xf + 0x10) = q.y;
    *(f32*)(xf + 0x14) = q.z;
    *(f32*)(xf + 0x18) = q.w;
    MarkXformDirty(*(u8**)(arg0 + 0x4));

    EntitySyncTransform(arg0, 0);
    fn_801EC080(arg0 + 0x94, arg0);
    return (*(Fn2*)(*(u8**)arg0 + 0xC8))(arg0, arg1);
}

// 0x800C5708
// Walks the ring from the tail element backwards, subtracting each element's duration (slot +0xD0)
// from t until t fits; evaluates that element at the remaining time (slot +0xD4). If t lands within
// 0.0001 of an element boundary, hands off to fn_800C176C instead.
void fn_800C5708(f32* arg0, u8* arg1, f32 farg0) {
    f32 t = farg0;
    u8* obj = RingSeekTail(arg1);
    for (;;) {
        f32 d = (*(DurFn*)(*(u8**)obj + 0xD0))(obj);
        if (!(t > d)) {
            (*(EvalFn*)(*(u8**)obj + 0xD4))(arg0, obj, t);
            return;
        }
        t -= (*(DurFn*)(*(u8**)obj + 0xD0))(obj);
        if (t > -0.0001f && t < 0.0001f) {
            fn_800C176C(arg0, obj);
            return;
        }
        obj = RingStepBack(arg1);
    }
}

// 0x800C5B98
// Same ring walk as fn_800C5708 starting from t = total duration (slot +0x20 of arg1's +0xD4
// object); locates the element at t, evaluates its quaternion and writes the negated Y axis of it.
void fn_800C5B98(f32* arg0, u8* arg1) {
    f32 t = (*(RootDurFn*)(*(u8**)(arg1 + 0xD4) + 0x20))(arg1);
    u8* obj = RingSeekTail(arg1);
    for (;;) {
        f32 d = (*(DurFn*)(*(u8**)obj + 0xD0))(obj);
        if (!(t > d)) break;
        t -= (*(DurFn*)(*(u8**)obj + 0xD0))(obj);
        if (t > -0.0001f && t < 0.0001f) {
            t = (*(DurFn*)(*(u8**)obj + 0xD0))(obj);
            break;
        }
        obj = RingStepBack(arg1);
    }
    u8* cur = fn_800C2FF0(arg1);
    Quat q = (*(QuatEvalFn*)(*(u8**)cur + 0xD8))(cur, t);
    Vec3 axis = QuatGetAxisY(&q);
    f32 unused[3];
    fn_801455E0(unused, (u8*)&q);
    arg0[0] = -axis.x;
    arg0[1] = -axis.y;
    arg0[2] = -axis.z;
}

// 0x800C9CF0
// Sorts an array of pointers by the f32 key at +0x0 of what they point to (quicksort with an
// explicit stack, median-of-three pivot; ranges of 5 or fewer are left for the insertion pass).
void fn_800C9CF0(u8** arg0, u32 arg1) {
    u8** stk[34];
    s32 sp = 0;
    u8** lo;
    u8** hi;
    u8** last = arg0 + arg1 - 1;
    if (arg1 > 4) {
        lo = arg0;
        hi = last;
        for (;;) {
            u8** mid = lo + (s32)((hi - lo) / 2);
            u8* a = *lo;
            u8* b = *mid;
            if (*(f32*)b < *(f32*)a) {
                *mid = a;
                *lo = b;
            }
            b = *mid;
            a = *hi;
            if (*(f32*)a < *(f32*)b) {
                *mid = a;
                *hi = b;
                a = *lo;
                b = *mid;
                if (*(f32*)b < *(f32*)a) {
                    *mid = a;
                    *lo = b;
                }
            }
            u8** i = lo + 1;
            u8** j = hi - 1;
            for (;;) {
                u8* piv = *mid;
                u8* x = *i;
                while (*(f32*)x < *(f32*)piv) {
                    i++;
                    piv = *mid;
                    x = *i;
                }
                u8* y = *j;
                while (*(f32*)piv < *(f32*)y) {
                    j--;
                    y = *j;
                }
                if (i < j) {
                    *i = y;
                    *j = x;
                    if (mid == i) mid = j;
                    else if (mid == j) mid = i;
                    i++;
                    j--;
                    if (!(i <= j)) break;
                } else {
                    if (i == j) {
                        i++;
                        j--;
                    }
                    break;
                }
            }
            s32 nl = (s32)(j - lo);
            s32 nr = (s32)(hi - i);
            if (nl <= 4) {
                if (nr <= 4) {
                    if (sp == 0) break;
                    lo = stk[sp - 1];
                    hi = stk[sp - 2];
                    sp -= 2;
                } else {
                    lo = i;
                }
            } else if (nr <= 4) {
                hi = j;
            } else if (nl > nr) {
                stk[sp + 1] = lo;
                lo = i;
                stk[sp] = j;
                sp += 2;
            } else {
                stk[sp + 1] = i;
                stk[sp] = hi;
                hi = j;
                sp += 2;
            }
        }
    }
    // Pull the smallest of the first five to the front, then insertion sort the rest.
    u8** lim = arg0 + 4;
    if (lim > last) lim = last;
    u8** minp = arg0;
    for (u8** p = arg0 + 1; p <= lim; p++) {
        if (*(f32*)*p < *(f32*)*minp) minp = p;
    }
    if (minp != arg0) {
        u8* t = *minp;
        *minp = *arg0;
        *arg0 = t;
    }
    for (u8** cur = arg0 + 2; cur <= last; cur++) {
        u8* v = *cur;
        u8** pos = cur - 1;
        while (*(f32*)v < *(f32*)*pos) pos--;
        pos++;
        if (pos != cur) {
            for (u8** q = cur; q > pos; q--) *q = *(q - 1);
            *pos = v;
        }
    }
}

// 0x800C9FC0
// Same sort as fn_800C9CF0 but keyed on the f32 at +0x8 of each pointed-to record.
void fn_800C9FC0(u8** arg0, u32 arg1) {
    u8** stk[34];
    s32 sp = 0;
    u8** lo;
    u8** hi;
    u8** last = arg0 + arg1 - 1;
    if (arg1 > 4) {
        lo = arg0;
        hi = last;
        for (;;) {
            u8** mid = lo + (s32)((hi - lo) / 2);
            u8* a = *lo;
            u8* b = *mid;
            if (*(f32*)(b + 8) < *(f32*)(a + 8)) {
                *mid = a;
                *lo = b;
            }
            b = *mid;
            a = *hi;
            if (*(f32*)(a + 8) < *(f32*)(b + 8)) {
                *mid = a;
                *hi = b;
                a = *lo;
                b = *mid;
                if (*(f32*)(b + 8) < *(f32*)(a + 8)) {
                    *mid = a;
                    *lo = b;
                }
            }
            u8** i = lo + 1;
            u8** j = hi - 1;
            for (;;) {
                u8* piv = *mid;
                u8* x = *i;
                while (*(f32*)(x + 8) < *(f32*)(piv + 8)) {
                    i++;
                    piv = *mid;
                    x = *i;
                }
                u8* y = *j;
                while (*(f32*)(piv + 8) < *(f32*)(y + 8)) {
                    j--;
                    y = *j;
                }
                if (i < j) {
                    *i = y;
                    *j = x;
                    if (mid == i) mid = j;
                    else if (mid == j) mid = i;
                    i++;
                    j--;
                    if (!(i <= j)) break;
                } else {
                    if (i == j) {
                        i++;
                        j--;
                    }
                    break;
                }
            }
            s32 nl = (s32)(j - lo);
            s32 nr = (s32)(hi - i);
            if (nl <= 4) {
                if (nr <= 4) {
                    if (sp == 0) break;
                    lo = stk[sp - 1];
                    hi = stk[sp - 2];
                    sp -= 2;
                } else {
                    lo = i;
                }
            } else if (nr <= 4) {
                hi = j;
            } else if (nl > nr) {
                stk[sp + 1] = lo;
                lo = i;
                stk[sp] = j;
                sp += 2;
            } else {
                stk[sp + 1] = i;
                stk[sp] = hi;
                hi = j;
                sp += 2;
            }
        }
    }
    u8** lim = arg0 + 4;
    if (lim > last) lim = last;
    u8** minp = arg0;
    for (u8** p = arg0 + 1; p <= lim; p++) {
        if (*(f32*)(*p + 8) < *(f32*)(*minp + 8)) minp = p;
    }
    if (minp != arg0) {
        u8* t = *minp;
        *minp = *arg0;
        *arg0 = t;
    }
    for (u8** cur = arg0 + 2; cur <= last; cur++) {
        u8* v = *cur;
        u8** pos = cur - 1;
        while (*(f32*)(v + 8) < *(f32*)(*pos + 8)) pos--;
        pos++;
        if (pos != cur) {
            for (u8** q = cur; q > pos; q--) *q = *(q - 1);
            *pos = v;
        }
    }
}

}  // extern "C"
