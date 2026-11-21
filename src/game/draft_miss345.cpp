// Zone slot loading/copying (0x80135854/0x80135BC0), draw-list sort, 3x4 matrix rotations and
// the line-strip vertex builder dispatch (0x8014DB90).
#include "types.h"
#include <string.h>

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C++" u32* gZoneBase;          // 0x803C7434 (world_alloc.cpp)
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C
extern "C++" void* fn_80199024(void* obj);   // 0x80199024 (small_8019.cpp)

extern "C" {
u8*   GetGlobalSettings(void);                                                              // 0x801D2BF8
void  ScopedLockCtor(ScopedLock* l);                                                        // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                                             // 0x801F53B0
void* SirLoad(void* loader, const char* path, void* dst, u32 size, s32 flags);              // 0x801E6138
f32   cosf(f32 x);
f32   sinf(f32 x);
void  fn_8012FD0C(u8* self, u8* zones);                                                     // 0x8012FD0C
void  fn_8012FF5C(u8* self, void* obj);                                                     // 0x8012FF5C
s32   fn_80133134(void* world, s32 zone);                                                   // 0x80133134
void  fn_8014B428(u8* dst, u8* src, s32 n);                                                 // 0x8014B428
void  fn_8014C8F0(u8* dst, u8* src);                                                        // 0x8014C8F0
void  fn_8014D190(u8* dst, u8* src);                                                        // 0x8014D190
void  fn_8014D834(u8* dst, u8* src);                                                        // 0x8014D834
void  fn_8014E3EC(s32 n);                                                                   // 0x8014E3EC
s32   fn_80151644(s32 a, s32 b, u8* quad);                                                  // 0x80151644
u32   fn_801D3A84(s32 n);                                                                   // 0x801D3A84
void  fn_801E7E28(void* p);                                                                 // 0x801E7E28
extern u8 gIdPool[];                  // 0x8048A8F0
extern u8 gWorldState[];              // 0x803E6D00 (first word: world block pointer)
extern s32 lbl_805FB4C4;              // 0x805FB4C4 (temp buffer offset of the free area)
extern s32 lbl_805FB4CC;              // 0x805FB4CC (temp buffer bytes left)
extern u8* lbl_805FB4D0;              // 0x805FB4D0 (temp buffer base)
extern f32 lbl_805FB924;              // 0x805FB924
extern f32 lbl_805F900C;              // 0x805F900C
}

// Sum of the per-category slot counts in the global settings (valid slot index < this).
static s32 SlotLimit() {
    s32 s = *(s32*)(GetGlobalSettings() + 0x4C);
    s += *(s32*)(GetGlobalSettings() + 0x34);
    s += *(s32*)(GetGlobalSettings() + 0x50);
    s += *(s32*)(GetGlobalSettings() + 0x54);
    s += *(s32*)(GetGlobalSettings() + 0x58);
    s += *(s32*)(GetGlobalSettings() + 0x5C);
    s += *(s32*)(GetGlobalSettings() + 0x60);
    s += *(s32*)(GetGlobalSettings() + 0x64);
    s += *(s32*)(GetGlobalSettings() + 0x38);
    s += *(s32*)(GetGlobalSettings() + 0x3C);
    s += *(s32*)(GetGlobalSettings() + 0x40);
    s += *(s32*)(GetGlobalSettings() + 0x44);
    s += *(s32*)(GetGlobalSettings() + 0x48);
    return s;
}

// Sum of the per-category zone counts in the global settings (valid zone id <= this).
static s32 ZoneLimit() {
    s32 s = *(s32*)(GetGlobalSettings() + 0x80);
    s += *(s32*)(GetGlobalSettings() + 0x68);
    s += *(s32*)(GetGlobalSettings() + 0x84);
    s += *(s32*)(GetGlobalSettings() + 0x88);
    s += *(s32*)(GetGlobalSettings() + 0x8C);
    s += *(s32*)(GetGlobalSettings() + 0x90);
    s += *(s32*)(GetGlobalSettings() + 0x94);
    s += *(s32*)(GetGlobalSettings() + 0x98);
    s += *(s32*)(GetGlobalSettings() + 0x6C);
    s += *(s32*)(GetGlobalSettings() + 0x70);
    s += *(s32*)(GetGlobalSettings() + 0x74);
    s += *(s32*)(GetGlobalSettings() + 0x78);
    s += *(s32*)(GetGlobalSettings() + 0x7C);
    return s;
}

// Sum of the "parts" zone category counts (0x68, 0x7C, 0x6C..0x78) used for the shared_parts range.
static s32 PartsZoneEnd() {
    s32 s = *(s32*)(GetGlobalSettings() + 0x68);
    s += *(s32*)(GetGlobalSettings() + 0x7C);
    s += *(s32*)(GetGlobalSettings() + 0x6C);
    s += *(s32*)(GetGlobalSettings() + 0x70);
    s += *(s32*)(GetGlobalSettings() + 0x74);
    s += *(s32*)(GetGlobalSettings() + 0x78);
    return s;
}

// Slot record (0x30 bytes) in the world's slot table, or NULL when the index is out of range.
static u8* ZoneSlot(u8* world, s32 index) {
    if (index >= 0 && index < SlotLimit()) {
        return *(u8**)(world + 0xB0) + index * 0x30;
    }
    return NULL;
}

// Draw-list ordering: entries whose owner flags (+0x20 of the object at +4) have 0x80000 or
// 0x04000000 set sort last; otherwise ascending by the float at +0x14.
static s32 DrawLess(u8* a, u8* b) {
    u32 fa = *(u32*)(*(u8**)(a + 0x4) + 0x20);
    if ((fa & 0x80000) || (fa & 0x04000000)) {
        return 0;
    }
    u32 fb = *(u32*)(*(u8**)(b + 0x4) + 0x20);
    if ((fb & 0x80000) || (fb & 0x04000000)) {
        return 1;
    }
    return *(f32*)(a + 0x14) < *(f32*)(b + 0x14);
}

extern "C" {

// 0x8012D468
// Sorts n draw-list entry pointers with DrawLess: median-of-three quicksort with an explicit
// stack down to partitions of <= 4, then a minimum swap and insertion sort to finish.
void fn_8012D468(u8** arr, u32 n) {
    if (n > 4) {
        u8** stk[8][2] = {};   // [i][0] = hi, [i][1] = lo; entry 0 is the terminator
        s32 sp = 1;
        u8** lo = arr;
        u8** hi = arr + n - 1;
        while (sp > 0) {
            u8** mid = lo + (s32)(hi - lo) / 2;
            u8* p = *mid;
            if (DrawLess(p, *lo)) {
                *mid = *lo;
                *lo = p;
            }
            p = *hi;
            if (DrawLess(p, *mid)) {
                u8* t = *mid;
                *mid = p;
                *hi = t;
                p = *mid;
                if (DrawLess(p, *lo)) {
                    *mid = *lo;
                    *lo = p;
                }
            }
            u8** i = lo + 1;
            u8** j = hi - 1;
            u8** piv = mid;
            for (;;) {
                u8* vi;
                for (;;) {
                    vi = *i;
                    if (!DrawLess(vi, *piv)) break;
                    i++;
                }
                u8* pv = *piv;
                while (DrawLess(pv, *j)) {
                    j--;
                }
                if (i < j) {
                    *i = *j;
                    *j = vi;
                    if (piv == i) {
                        piv = j;
                    } else if (piv == j) {
                        piv = i;
                    }
                    i++;
                    j--;
                    if (i <= j) continue;
                } else if (i == j) {
                    i++;
                    j--;
                }
                break;
            }
            s32 leftN = (s32)(j - lo);
            s32 rightN = (s32)(hi - i);
            if (leftN <= 4) {
                if (rightN <= 4) {
                    sp--;
                    lo = stk[sp][1];
                    hi = stk[sp][0];
                } else {
                    lo = i;
                }
            } else if (rightN <= 4) {
                hi = j;
            } else if (leftN > rightN) {
                stk[sp][1] = lo;
                stk[sp][0] = j;
                sp++;
                lo = i;
            } else {
                stk[sp][1] = i;
                stk[sp][0] = hi;
                sp++;
                hi = j;
            }
        }
    }
    u8** end = arr + n - 1;
    u8** limit = arr + 4;
    if (limit > end) limit = end;
    u8** minp = arr;
    for (u8** q = arr + 1; q <= limit; q++) {
        if (DrawLess(*q, *minp)) minp = q;
    }
    if (minp != arr) {
        u8* t = *minp;
        *minp = *arr;
        *arr = t;
    }
    for (u8** q = arr + 1; q <= end; q++) {
        u8* v = *q;
        u8** k = q - 1;
        while (DrawLess(v, *k)) {
            k--;
        }
        u8** ins = k + 1;
        if (ins != q) {
            u8** dst = q;
            for (u8** s = q - 1; s >= ins; s--) {
                *dst = *s;
                dst = s;
            }
            *dst = v;
        }
    }
}

// 0x80135854
// Starts loading the zone assigned to slot `index`: marks it pending (state 1, timestamp) and
// queues the SIR load into the zone's memory block, from shared_ui/shared_parts for those ranges.
void fn_80135854(u8* world, s32 index) {
    u8* slot = ZoneSlot(world, index);
    s32 zone = ZoneSlot(world, index) ? *(s32*)(ZoneSlot(world, index) + 0x8) : -2;
    if (zone >= 0 && zone < ZoneLimit() + 1) {
        *(s32*)(slot + 0x4) = 1;
        *(f32*)(slot + 0x14) = lbl_805FB924 + lbl_805F900C;
        void* dst = (void*)(uintptr_t)gZoneBase[zone];
        u32 size = fn_801D3A84(zone);
        const char* path = (const char*)(world + 0x124);
        if (fn_80133134(world, zone) != 0) {
            path = "shared_ui.sir";
        } else if (*(s32*)(GetGlobalSettings() + 0x68) + 1 <= zone) {
            if (zone <= PartsZoneEnd()) {
                path = "shared_parts.sir";
            }
        }
        *(void**)(slot + 0x20) = SirLoad(gIdPool, path, dst, size, zone);
    }
}

// 0x80135BC0
// Activates slot `index` with its zone: copies the zone image into the zone resource block
// (shared zones / non-shared range), sets up the zone objects under a scoped lock and marks the
// slot loaded (state 3).
void fn_80135BC0(u8* world, s32 index) {
    ScopedLock lock;
    u8* slot = ZoneSlot(world, index);
    s32 zone = ZoneSlot(world, index) ? *(s32*)(ZoneSlot(world, index) + 0x8) : -2;
    s32 zone2 = ZoneSlot(world, index) ? *(s32*)(ZoneSlot(world, index) + 0x8) : -2;
    u8* res = NULL;
    if (zone2 >= 0 && zone2 < ZoneLimit() + 1) {
        res = *(u8**)(*(u8**)world + 0x1A4) + zone2 * gZoneResourceSize;
    }
    if ((*(s32*)(GetGlobalSettings() + 0x68) + 1 <= zone && zone <= PartsZoneEnd()) ||
        fn_80133134(world, zone) != 0) {
        memcpy(res, (void*)(uintptr_t)gZoneBase[zone], *(s32*)(GetGlobalSettings() + 0x34) * 0x54 + 0x148);
    }
    if (res != NULL && zone > 0) {
        ScopedLockCtor(&lock);
        fn_8012FD0C(world, res);
        fn_801E7E28(*(u8**)(*(u8**)world + 0x1C8) + zone * 0xC);
        u8* ws = *(u8**)gWorldState;
        fn_8012FF5C(world, ((void**)*(u8**)(ws + 0x1B0))[zone]);
        fn_8012FF5C(world, ((void**)*(u8**)(ws + 0x1BC))[zone]);
        fn_801D3A84(zone);
        u8* wb = *(u8**)world;
        ((void**)*(u8**)(world + 0xC8))[index] = ((void**)*(u8**)(wb + 0x1B0))[zone];
        ((void**)*(u8**)(world + 0xD4))[index] = ((void**)*(u8**)(wb + 0x1BC))[zone];
        *(u32*)(slot + 0xC) &= ~1u;
        *(s32*)(slot + 0x4) = 3;
        *(s32*)(slot + 0x8) = zone;
        *(u8**)(slot + 0x28) = res;
        *(void**)(slot + 0x2C) = ((void**)*(u8**)(world + 0xC8))[index];
        void* extra = *(void**)(*(u8**)(slot + 0x28) + 0x100);
        if (extra != NULL) {
            fn_80199024(extra);
        }
        ScopedLockDtor(&lock, -1);
    }
}

// 0x80144F8C
// Rotates the matrix rows' column pairs (1,2) of a 3x4 matrix by angle*(pi/2) (rotation about X).
void fn_80144F8C(u8* m, f32 angle) {
    f32 a = 1.5707964f * angle;
    f32 c = cosf(a);
    f32 s = sinf(a);
    f32 m4 = *(f32*)(m + 0x4), m10 = *(f32*)(m + 0x10), m1c = *(f32*)(m + 0x1C);
    f32 m8 = *(f32*)(m + 0x8), m14 = *(f32*)(m + 0x14), m20 = *(f32*)(m + 0x20);
    f32 m28 = *(f32*)(m + 0x28), m2c = *(f32*)(m + 0x2C);
    *(f32*)(m + 0x4)  = c * m4 - s * m8;
    *(f32*)(m + 0x10) = c * m10 - s * m14;
    *(f32*)(m + 0x1C) = c * m1c - s * m20;
    *(f32*)(m + 0x28) = c * m28 - s * m2c;
    *(f32*)(m + 0x8)  = s * m4 + c * m8;
    *(f32*)(m + 0x14) = s * m10 + c * m14;
    *(f32*)(m + 0x20) = s * m1c + c * m20;
    *(f32*)(m + 0x2C) = s * m28 + c * m2c;
}

// 0x80145094
// Rotates a 3x4 matrix by angle*(pi/2) about Y (mixes columns 0 and 2 of rows 0..2 / column 3).
void fn_80145094(u8* m, f32 angle) {
    f32 a = 1.5707964f * angle;
    f32 c = cosf(a);
    f32 s = sinf(a);
    f32 m8 = *(f32*)(m + 0x8), m0 = *(f32*)(m + 0x0), mc = *(f32*)(m + 0xC);
    f32 ns = -s;
    f32 m14 = *(f32*)(m + 0x14), m20 = *(f32*)(m + 0x20), m18 = *(f32*)(m + 0x18);
    f32 m24 = *(f32*)(m + 0x24), m2c = *(f32*)(m + 0x2C);
    *(f32*)(m + 0xC)  = c * mc + s * m14;
    *(f32*)(m + 0x0)  = c * m0 + s * m8;
    *(f32*)(m + 0x8)  = ns * m0 + c * m8;
    *(f32*)(m + 0x18) = c * m18 + s * m20;
    *(f32*)(m + 0x24) = c * m24 + s * m2c;
    *(f32*)(m + 0x14) = ns * mc + c * m14;
    *(f32*)(m + 0x20) = ns * m18 + c * m20;
    *(f32*)(m + 0x2C) = ns * m24 + c * m2c;
}

// 0x801451A0
// Rotates a 3x4 matrix by angle*(pi/2) about Z (mixes columns 0 and 1).
void fn_801451A0(u8* m, f32 angle) {
    f32 a = 1.5707964f * angle;
    f32 c = cosf(a);
    f32 s = sinf(a);
    f32 m0 = *(f32*)(m + 0x0), mc = *(f32*)(m + 0xC), m18 = *(f32*)(m + 0x18);
    f32 m4 = *(f32*)(m + 0x4), m10 = *(f32*)(m + 0x10), m1c = *(f32*)(m + 0x1C);
    f32 m24 = *(f32*)(m + 0x24), m28 = *(f32*)(m + 0x28);
    *(f32*)(m + 0x0)  = c * m0 - s * m4;
    *(f32*)(m + 0xC)  = c * mc - s * m10;
    *(f32*)(m + 0x18) = c * m18 - s * m1c;
    *(f32*)(m + 0x24) = c * m24 - s * m28;
    *(f32*)(m + 0x4)  = s * m0 + c * m4;
    *(f32*)(m + 0x10) = s * mc + c * m10;
    *(f32*)(m + 0x1C) = s * m18 + c * m1c;
    *(f32*)(m + 0x28) = s * m24 + c * m28;
}

// 0x8014DB90
// Builds a strip of `count` (>= 2) points into the temp buffer and submits it (prim 0x98).
// Counts 2/3/4 use dedicated builders; larger counts (capped at 32) use the generic one.
// Each builder runs twice: once on the base data, once on the data at +0x18.
void fn_8014DB90(u8* verts, s32 count) {
    s32 pairs;
    u8* dst;
    if (count < 2) return;
    if (count == 2) {
        pairs = 2;
        if (lbl_805FB4CC < 0x60) fn_8014E3EC(0x60);
        s32 off = lbl_805FB4C4;
        lbl_805FB4C4 = off + 0x60;
        dst = lbl_805FB4D0 + off;
        lbl_805FB4CC -= 0x60;
        fn_8014D834(dst, verts);
        fn_8014D834(dst + 0x18, verts + 0x18);
    } else if (count == 3) {
        pairs = 0x11;
        if (lbl_805FB4CC < 0x340) fn_8014E3EC(0x340);
        s32 off = lbl_805FB4C4;
        lbl_805FB4C4 = off + 0x340;
        dst = lbl_805FB4D0 + off;
        lbl_805FB4CC -= 0x340;
        fn_8014D190(dst, verts);
        fn_8014D190(dst + 0x18, verts + 0x18);
    } else if (count == 4) {
        pairs = 0x19;
        if (lbl_805FB4CC < 0x4C0) fn_8014E3EC(0x4C0);
        s32 off = lbl_805FB4C4;
        lbl_805FB4C4 = off + 0x4C0;
        dst = lbl_805FB4D0 + off;
        lbl_805FB4CC -= 0x4C0;
        fn_8014C8F0(dst, verts);
        fn_8014C8F0(dst + 0x18, verts + 0x18);
    } else {
        if (count > 0x20) count = 0x20;
        s32 k = count - 5;
        pairs = k * 8 + 0x21;
        s32 size = (pairs * 2 * 0x18 + 0x1F) & ~0x1F;
        if (lbl_805FB4CC < size) fn_8014E3EC(size);
        s32 off = lbl_805FB4C4;
        lbl_805FB4C4 = off + size;
        dst = lbl_805FB4D0 + off;
        lbl_805FB4CC -= size;
        fn_8014B428(dst, verts, k);
        fn_8014B428(dst + 0x18, verts + 0x18, k);
    }
    fn_80151644(0x98, pairs * 2, dst);
}

}  // extern "C"
