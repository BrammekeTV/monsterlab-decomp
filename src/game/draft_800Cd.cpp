// Small functions in 0x800C09D8 - 0x800C2028 (camera target pointer table, ray/sphere tests,
// frustum visibility flag, zone object list builders).
#include <math.h>
#include "types.h"

extern "C" {
void*  MemAllocDefault(u32 size);                                     // 0x801D4650
void*  MemAllocSmall(u32 size);                                       // 0x801D41CC
u32    RandomU32(void);                                               // 0x80143460
void   operator_delete(void* p);                                      // 0x801D426C
u8*    fn_80073454(void);                                             // 0x80073454 (returns the camera/frustum block; planes at +0xC0)
u32    fn_801EC880(u8* p, u32 hash);                                  // 0x801EC880
// 0x801BFE38: ray/ray closest-approach style test; r3/r5/r6 are pointers, r4 a float vector,
// r7/r8 receive the two parameters, returns the squared distance in f1.
f32    fn_801BFE38(u8* a, f32* b, u8* c, u8* d, f32* outT0, f32* outT1);
extern u8  gWorldState[];       // 0x803E6D00
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C
}

// "Start" / "end" node hashes.
static const u32 kHashStart = 0x682D973Fu;  // "Start"
static const u32 kHashEnd   = 0x965B713Bu;  // "end"

// Returns 1 if the point lies in front of any of the six planes (normal xyz, distance w; 0x10 bytes each).
static s32 PointOutsideAnyPlane(u8* planes, const f32* p) {
    f32 x = p[0];
    f32 y = p[1];
    f32 z = p[2];
    s32 n = 6;
    do {
        if (x * *(f32*)(planes + 0x0) + y * *(f32*)(planes + 0x4) + z * *(f32*)(planes + 0x8) > *(f32*)(planes + 0xC)) {
            return 1;
        }
        planes += 0x10;
        n--;
    } while (n != 0);
    return 0;
}

extern "C" {

// 0x800C09D8
// Switches the active entry of a pointer table (index at +0): the old entry's node is parked 100 units
// below its target and flagged, the new entry's node is moved onto its target; stores the new blend value.
void fn_800C09D8(u8* arg0, s32 arg1, f32 farg0) {
    u8* old = *(u8**)(arg0 + *(s32*)(arg0 + 0x0) * 4 + 0x4);
    if (old != NULL) {
        f32 tx = *(f32*)(old + 0xC8);
        f32 ty = *(f32*)(old + 0xCC);
        f32 tz = *(f32*)(old + 0xD0);
        u8* node = *(u8**)(old + 0x4);
        *(f32*)(node + 0x0) = tx;
        *(f32*)(node + 0x4) = ty - 100.0f;
        *(f32*)(node + 0x8) = tz;
        node = *(u8**)(old + 0x4);
        *(u32*)(node + 0x20) |= 0x8000;
        node = *(u8**)(old + 0x4);
        *(u32*)(node + 0x20) |= 0x10000000;
        node = *(u8**)(old + 0x4);
        *(u32*)(node + 0x20) |= 0x4000;
        node = *(u8**)(old + 0x4);
        *(u32*)(node + 0x20) |= 0x10000000;
    }
    *(s32*)(arg0 + 0x0) = arg1;
    u8* cur = *(u8**)(arg0 + arg1 * 4 + 0x4);
    if (cur != NULL) {
        u8* node = *(u8**)(cur + 0x4);
        *(f32*)(node + 0x0) = *(f32*)(cur + 0xC8);
        *(f32*)(node + 0x4) = *(f32*)(cur + 0xCC);
        *(f32*)(node + 0x8) = *(f32*)(cur + 0xD0);
        node = *(u8**)(cur + 0x4);
        *(u32*)(node + 0x20) |= 0x8000;
        node = *(u8**)(cur + 0x4);
        *(u32*)(node + 0x20) |= 0x10000000;
        node = *(u8**)(cur + 0x4);
        *(u32*)(node + 0x20) |= 0x4000;
        node = *(u8**)(cur + 0x4);
        *(u32*)(node + 0x20) |= 0x10000000;
    }
    *(f32*)(arg0 + 0x168) = farg0;
}

// 0x800C0BA4
// Deleting destructor: clears the 9 pairs of words, frees the block if del > 0.
u8* fn_800C0BA4(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(s32*)(arg0 + 0x0) = 0;
        *(s32*)(arg0 + 0x8) = 0;
        *(s32*)(arg0 + 0x10) = 0;
        *(s32*)(arg0 + 0x18) = 0;
        *(s32*)(arg0 + 0x20) = 0;
        *(s32*)(arg0 + 0x28) = 0;
        *(s32*)(arg0 + 0x30) = 0;
        *(s32*)(arg0 + 0x38) = 0;
        *(s32*)(arg0 + 0x40) = 0;
        *(s32*)(arg0 + 0x4) = 0;
        *(s32*)(arg0 + 0xC) = 0;
        *(s32*)(arg0 + 0x14) = 0;
        *(s32*)(arg0 + 0x1C) = 0;
        *(s32*)(arg0 + 0x24) = 0;
        *(s32*)(arg0 + 0x2C) = 0;
        *(s32*)(arg0 + 0x34) = 0;
        *(s32*)(arg0 + 0x3C) = 0;
        *(s32*)(arg0 + 0x44) = 0;
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800C10D0
// Ray/sphere style test between two points: true if the closest-approach squared distance is within
// (farg2 + lerp(farg0, farg1, t))^2. arg2/arg3 are the caller's leftover r5/r6, forwarded to fn_801BFE38.
s32 fn_800C10D0(u8* arg0, u8* arg1, u8* arg2, u8* arg3, f32 farg0, f32 farg1, f32 farg2) {
    f32 t1 = 0.0f;
    f32 t0 = 0.0f;
    f32 d[3];
    d[0] = *(f32*)(arg1 + 0x0) - *(f32*)(arg0 + 0x0);
    d[1] = *(f32*)(arg1 + 0x4) - *(f32*)(arg0 + 0x4);
    d[2] = *(f32*)(arg1 + 0x8) - *(f32*)(arg0 + 0x8);
    f32 dist2 = fn_801BFE38(arg0, d, arg2, arg3, &t1, &t0);
    f32 r = farg2 + ((1.0f - t1) * farg0 + t1 * farg1);
    return dist2 <= r * r;
}

// 0x800C124C
// Ray vs sphere: points arg0/arg1 give the sphere offset, arg2/arg3 the ray direction, radius
// farg0 + farg1. Writes the first hit parameter to *arg4; returns 1 on hit (or already inside), else 0.
s32 fn_800C124C(u8* arg0, u8* arg1, u8* arg2, u8* arg3, f32* arg4, f32 farg0, f32 farg1) {
    f32 rad = farg1 + farg0;
    f32 cx = *(f32*)(arg1 + 0x0) - *(f32*)(arg0 + 0x0);
    f32 cy = *(f32*)(arg1 + 0x4) - *(f32*)(arg0 + 0x4);
    f32 cz = *(f32*)(arg1 + 0x8) - *(f32*)(arg0 + 0x8);
    f32 dx = *(f32*)(arg3 + 0x0) - *(f32*)(arg2 + 0x0);
    f32 dy = *(f32*)(arg3 + 0x4) - *(f32*)(arg2 + 0x4);
    f32 dz = *(f32*)(arg3 + 0x8) - *(f32*)(arg2 + 0x8);
    f32 c = (cz * cz + (cx * cx + cy * cy)) - rad * rad;
    if (c < 0.0f) {
        *arg4 = 0.0f;
        return 1;
    }
    f32 a = dz * dz + (dx * dx + dy * dy);
    if (a < 0.0001f) {
        return 0;
    }
    f32 b = dz * cz + (dx * cx + dy * cy);
    if (b >= 0.0f) {
        return 0;
    }
    f32 disc = b * b - a * c;
    if (disc < 0.0f) {
        return 0;
    }
    f32 root = 0.0f;
    if (disc > 0.0f) {
        root = sqrtf(disc);  // original: frsqrte with two Newton steps
    }
    *arg4 = (-b - root) / a;
    return 1;
}

// 0x800C148C
// Resets the node state: clears two flag bytes, looks up the "Start" and "end" hashes, and
// sets the 3x4 matrix at +0x270 to identity.
void fn_800C148C(u8* arg0) {
    *(u8*)(arg0 + 0x26C) = 0;
    *(u8*)(arg0 + 0x26D) = 0;
    *(u32*)(arg0 + 0x264) = fn_801EC880(arg0 + 0x94, kHashStart);
    *(u32*)(arg0 + 0x268) = fn_801EC880(arg0 + 0x94, kHashEnd);
    *(u32*)(arg0 + 0x15C) |= 0x400000;
    typedef u8* (*GetFn)(u8*);
    if ((*(GetFn*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* o = (*(GetFn*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(o + 0x8) &= 0xFFFFFFFDu;
    }
    u8* node = *(u8**)(arg0 + 0x4);
    *(u32*)(node + 0x20) &= 0xFFFFFFFDu;
    *(f32*)(arg0 + 0x270) = 1.0f;
    *(f32*)(arg0 + 0x274) = 0.0f;
    *(f32*)(arg0 + 0x278) = 0.0f;
    *(f32*)(arg0 + 0x27C) = 0.0f;
    *(f32*)(arg0 + 0x280) = 1.0f;
    *(f32*)(arg0 + 0x284) = 0.0f;
    *(f32*)(arg0 + 0x288) = 0.0f;
    *(f32*)(arg0 + 0x28C) = 0.0f;
    *(f32*)(arg0 + 0x290) = 1.0f;
    *(f32*)(arg0 + 0x294) = 0.0f;
    *(f32*)(arg0 + 0x298) = 0.0f;
    *(f32*)(arg0 + 0x29C) = 0.0f;
}

// 0x800C15E8
// Per-frame visibility: if not yet hidden (+0x26D == 0), tests the node position and the target
// "end" position against the frustum planes; +0x26D = node inside, or already set, or end inside.
void fn_800C15E8(u8* arg0) {
    if (*(u8*)(arg0 + 0x26D) == 0) {
        u8* planes = fn_80073454() + 0xC0;
        s32 nodeOut = PointOutsideAnyPlane(planes, *(f32**)(arg0 + 0x4));
        *(u8*)(arg0 + 0x26D) = (nodeOut == 0);
        u8* ent = *(u8**)(arg0 + 0x98) + *(s32*)(arg0 + 0x268) * 0xC8;
        s32 endOut = PointOutsideAnyPlane(planes, (f32*)(ent + 0x5C));
        u8 v = 0;
        if (*(u8*)(arg0 + 0x26D) != 0 || endOut == 0) {
            v = 1;
        }
        *(u8*)(arg0 + 0x26D) = v;
    }
}

// 0x800C1B78
// Like fn_800C15E8 but first sets the visible bit (2) on the object and node, sets +0x26C = 1 and
// clears +0x26D, then runs the same frustum test.
void fn_800C1B78(u8* arg0) {
    typedef u8* (*GetFn)(u8*);
    if ((*(GetFn*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* o = (*(GetFn*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(o + 0x8) |= 2;
    }
    u8* node = *(u8**)(arg0 + 0x4);
    *(u32*)(node + 0x20) |= 2;
    *(u8*)(arg0 + 0x26C) = 1;
    *(u8*)(arg0 + 0x26D) = 0;
    u8* planes = fn_80073454() + 0xC0;
    s32 nodeOut = PointOutsideAnyPlane(planes, *(f32**)(arg0 + 0x4));
    *(u8*)(arg0 + 0x26D) = (nodeOut == 0);
    u8* ent = *(u8**)(arg0 + 0x98) + *(s32*)(arg0 + 0x268) * 0xC8;
    s32 endOut = PointOutsideAnyPlane(planes, (f32*)(ent + 0x5C));
    u8 v = 0;
    if (*(u8*)(arg0 + 0x26D) != 0 || endOut == 0) {
        v = 1;
    }
    *(u8*)(arg0 + 0x26D) = v;
}

// 0x800C1DC0
// Counts the zone objects (static + dynamic lists) whose flag 0x20000 is set and whose +0x9C->+8 hash
// is in the list arg0 (ptr +0x0, count +0x4); stores the count at +0x10, mode 1 at +0x14, and
// allocates the result array at +0xC. (The other allocator branch is dead: mode is constant 1.)
void fn_800C1DC0(u8* arg0) {
    s32 found = 0;
    s32 i = 0;
    u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 total = *(s32*)(res + 0x94) + *(s32*)(res + 0x98);
    s32 off64 = 0;
    s32 off4 = 0;
    while (i < total) {
        u8* obj;
        u8* rec;
        if (i < *(s32*)(res + 0x94)) {
            u8* r2 = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
            obj = *(u8**)(*(u8**)(r2 + 0x120) + off4);
            rec = *(u8**)(r2 + 0x8C) + off64;
        } else {
            u8* r2 = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
            s32 j = i - *(s32*)(r2 + 0x94);
            obj = *(u8**)(*(u8**)(r2 + 0x12C) + j * 4);
            rec = *(u8**)(r2 + 0x90) + (j << 6);
        }
        if ((*(u32*)(rec + 0x1C) & 0x20000) && obj != NULL) {
            s32 n = *(s32*)(arg0 + 0x4);
            if (n > 0) {
                s32 k = 0;
                do {
                    if (*(u32*)(*(u8**)(obj + 0x9C) + 0x8) == *(u32*)(*(u8**)arg0 + k)) {
                        found++;
                    }
                    k += 4;
                    n--;
                } while (n != 0);
            }
        }
        off64 += 0x40;
        off4 += 4;
        i++;
    }
    *(s32*)(arg0 + 0x10) = found;
    *(s32*)(arg0 + 0x14) = 1;
    *(void**)(arg0 + 0xC) = MemAllocDefault((u32)found * 4);
}

// 0x800C1F18
// Second pass of fn_800C1DC0: stores each matching zone object pointer into the array at +0xC.
void fn_800C1F18(u8* arg0) {
    s32 out = 0;
    s32 i = 0;
    s32 off64 = 0;
    s32 off4 = 0;
    u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 total = *(s32*)(res + 0x94) + *(s32*)(res + 0x98);
    if (total > 0) {
        s32 ctr = total;
        do {
            u8* obj;
            u8* rec;
            u8* r2 = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
            s32 n94 = *(s32*)(r2 + 0x94);
            if (i < n94) {
                obj = *(u8**)(*(u8**)(r2 + 0x120) + off4);
                rec = *(u8**)(r2 + 0x8C) + off64;
            } else {
                s32 j = i - n94;
                obj = *(u8**)(*(u8**)(r2 + 0x12C) + j * 4);
                rec = *(u8**)(r2 + 0x90) + (j << 6);
            }
            if ((*(u32*)(rec + 0x1C) & 0x20000) && obj != NULL) {
                s32 k = 0;
                s32 off = 0;
                while (k < *(s32*)(arg0 + 0x4)) {
                    if (*(u32*)(*(u8**)(obj + 0x9C) + 0x8) == *(u32*)(*(u8**)arg0 + off)) {
                        *(u8**)(*(u8**)(arg0 + 0xC) + out * 4) = obj;
                        out++;
                    }
                    off += 4;
                    k++;
                }
            }
            off64 += 0x40;
            off4 += 4;
            i++;
            ctr--;
        } while (ctr != 0);
    }
}

// 0x800C2028
// Picks a random entry of the array at +0xC (count +0x10) whose byte +0x26C is 0; NULL if none.
u8* fn_800C2028(u8* arg0) {
    s32* idx = (s32*)MemAllocSmall((u32)*(s32*)(arg0 + 0x10) * 4);
    s32 count = 0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x10); i++) {
        u8* e = *(u8**)(*(u8**)(arg0 + 0xC) + i * 4);
        if (*(u8*)(e + 0x26C) == 0) {
            idx[count] = i;
            count++;
        }
    }
    u8* result = NULL;
    if (count > 0) {
        u32 r = RandomU32() % (u32)count;
        result = *(u8**)(*(u8**)(arg0 + 0xC) + idx[r] * 4);
    }
    operator_delete(idx);
    return result;
}

}  // extern "C"
