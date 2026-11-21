// Engine functions 0x801C: ray/box test, sphere-vs-frustum flag, entry lookup, hook jump.
#include "types.h"

extern "C" {
s32  fn_801C1960(f32* tmin, f32* tmax, f32 dir, f32 dist);  // 0x801C1960
void* fn_801C3F6C(u8* table, u8* entry);                     // 0x801C3F6C
}

extern "C++" {
extern void* lbl_805FB5F8;  // 0x805FB5F8 (current camera)
extern void* lbl_805FB898;  // 0x805FB898 (hook function pointer, may be null)
extern f32   lbl_805FFC70;  // 0x805FFC70 (0.0f)
extern f32   lbl_805FFC74;  // 0x805FFC74 (1.0f)
}

extern "C" {

// 0x801CBCF8
// Returns bit `bit` of the flags word at p.
s32 fn_801CBCF8(u8* p, s32 bit) {
    return (s32)((*(u32*)p & (1u << bit)) >> bit);
}

// 0x801CB988
// Tests the entity's sphere (pos at +0x10, radius at +0x1C) against two sets of three camera
// planes; sets/clears the 0x20 flag in the entity's flags word.
void fn_801CB988(u8* arg0) {
    u8* cam = (u8*)lbl_805FB5F8;
    s32 cnt = 0;
    s32 res;
    f32 x = *(f32*)(arg0 + 0x10);
    f32 y = *(f32*)(arg0 + 0x14);
    f32 z = *(f32*)(arg0 + 0x18);
    f32 r = *(f32*)(arg0 + 0x1C);

    f32 a = (z * *(f32*)(cam + 0xE8) + (x * *(f32*)(cam + 0xE0) + y * *(f32*)(cam + 0xE4))) - *(f32*)(cam + 0xEC);
    f32 b = (z * *(f32*)(cam + 0xF8) + (x * *(f32*)(cam + 0xF0) + y * *(f32*)(cam + 0xF4))) - *(f32*)(cam + 0xFC);
    f32 c = (z * *(f32*)(cam + 0x108) + (x * *(f32*)(cam + 0x100) + y * *(f32*)(cam + 0x104))) - *(f32*)(cam + 0x10C);
    f32 m = (a > b) ? a : b;
    m = (m > c) ? m : c;
    if (m > r) {
        res = 0;
    } else {
        if (m > -r) cnt = 1;
        a = (z * *(f32*)(cam + 0xC8) + (x * *(f32*)(cam + 0xC0) + y * *(f32*)(cam + 0xC4))) - *(f32*)(cam + 0xCC);
        b = (z * *(f32*)(cam + 0xD8) + (x * *(f32*)(cam + 0xD0) + y * *(f32*)(cam + 0xD4))) - *(f32*)(cam + 0xDC);
        c = (z * *(f32*)(cam + 0x118) + (x * *(f32*)(cam + 0x110) + y * *(f32*)(cam + 0x114))) - *(f32*)(cam + 0x11C);
        m = (a > b) ? a : b;
        m = (m > c) ? m : c;
        if (m > r) {
            res = 0;
        } else {
            if (m > -r) cnt++;
            res = cnt + 1;
        }
    }
    if (res != 0) {
        *(u32*)arg0 &= ~0x20u;
    } else {
        *(u32*)arg0 |= 0x20u;
    }
}

// 0x801C217C
// Slab test of a swept segment (arg3 + t*arg4, t in [0,1]) against an oriented box
// (center arg0, half extents arg1, axes arg2). Writes hit count (0..2) to *arg5, entry/exit
// points to arg6[0..5] and entry/exit t to arg7[0..1]. Returns nonzero if the segment intersects.
s32 fn_801C217C(u8* arg0, u8* arg1, u8* arg2, u8* arg3, u8* arg4, s32* arg5, f32* arg6, f32* arg7) {
    f32 d0 = *(f32*)(arg3 + 0x0) - *(f32*)(arg0 + 0x0);
    f32 d1 = *(f32*)(arg3 + 0x4) - *(f32*)(arg0 + 0x4);
    f32 d2 = *(f32*)(arg3 + 0x8) - *(f32*)(arg0 + 0x8);
    f32 v0 = *(f32*)(arg4 + 0x0);
    f32 v1 = *(f32*)(arg4 + 0x4);
    f32 v2 = *(f32*)(arg4 + 0x8);
    f32 tmin = lbl_805FFC70;
    f32 tmax = lbl_805FFC74;
    f32 p0 = d2 * *(f32*)(arg2 + 0x8) + (d0 * *(f32*)(arg2 + 0x0) + d1 * *(f32*)(arg2 + 0x4));
    f32 p1 = d2 * *(f32*)(arg2 + 0x14) + (d0 * *(f32*)(arg2 + 0xC) + d1 * *(f32*)(arg2 + 0x10));
    f32 p2 = d2 * *(f32*)(arg2 + 0x20) + (d0 * *(f32*)(arg2 + 0x18) + d1 * *(f32*)(arg2 + 0x1C));
    f32 q0 = v2 * *(f32*)(arg2 + 0x8) + (v0 * *(f32*)(arg2 + 0x0) + v1 * *(f32*)(arg2 + 0x4));
    f32 q1 = v2 * *(f32*)(arg2 + 0x14) + (v0 * *(f32*)(arg2 + 0xC) + v1 * *(f32*)(arg2 + 0x10));
    f32 q2 = v2 * *(f32*)(arg2 + 0x20) + (v0 * *(f32*)(arg2 + 0x18) + v1 * *(f32*)(arg2 + 0x1C));
    s32 ok0 = 0, ok1 = 0, ok2 = 0, ok3 = 0, ok4 = 0;
    s32 hit;

    if (fn_801C1960(&tmin, &tmax, q0, -p0 - *(f32*)(arg1 + 0x0)) != 0 &&
        fn_801C1960(&tmin, &tmax, -q0, p0 - *(f32*)(arg1 + 0x0)) != 0) {
        ok0 = 1;
    }
    if (ok0 != 0 && fn_801C1960(&tmin, &tmax, q1, -p1 - *(f32*)(arg1 + 0x4)) != 0) ok1 = 1;
    if (ok1 != 0 && fn_801C1960(&tmin, &tmax, -q1, p1 - *(f32*)(arg1 + 0x4)) != 0) ok2 = 1;
    if (ok2 != 0 && fn_801C1960(&tmin, &tmax, q2, -p2 - *(f32*)(arg1 + 0x8)) != 0) ok3 = 1;
    if (ok3 != 0 && fn_801C1960(&tmin, &tmax, -q2, p2 - *(f32*)(arg1 + 0x8)) != 0) ok4 = 1;

    hit = 0;
    if (ok4 != 0 && (tmin != lbl_805FFC70 || tmax != lbl_805FFC74)) hit = 1;

    if (hit != 0) {
        if (tmin > lbl_805FFC70) {
            if (tmax < lbl_805FFC74) {
                *arg5 = 2;
                arg6[0] = *(f32*)(arg3 + 0x0) + *(f32*)(arg4 + 0x0) * tmin;
                arg6[1] = *(f32*)(arg3 + 0x4) + *(f32*)(arg4 + 0x4) * tmin;
                arg6[2] = *(f32*)(arg3 + 0x8) + *(f32*)(arg4 + 0x8) * tmin;
                arg6[3] = *(f32*)(arg3 + 0x0) + *(f32*)(arg4 + 0x0) * tmax;
                arg6[4] = *(f32*)(arg3 + 0x4) + *(f32*)(arg4 + 0x4) * tmax;
                arg6[5] = *(f32*)(arg3 + 0x8) + *(f32*)(arg4 + 0x8) * tmax;
                arg7[0] = tmin;
                arg7[1] = tmax;
                return hit;
            }
            *arg5 = 1;
            arg6[0] = *(f32*)(arg3 + 0x0) + *(f32*)(arg4 + 0x0) * tmin;
            arg6[1] = *(f32*)(arg3 + 0x4) + *(f32*)(arg4 + 0x4) * tmin;
            arg6[2] = *(f32*)(arg3 + 0x8) + *(f32*)(arg4 + 0x8) * tmin;
            arg7[0] = tmin;
            return hit;
        }
        if (tmax < lbl_805FFC74) {
            *arg5 = 1;
            arg6[0] = *(f32*)(arg3 + 0x0) + *(f32*)(arg4 + 0x0) * tmax;
            arg6[1] = *(f32*)(arg3 + 0x4) + *(f32*)(arg4 + 0x4) * tmax;
            arg6[2] = *(f32*)(arg3 + 0x8) + *(f32*)(arg4 + 0x8) * tmax;
            arg7[0] = tmax;
            return hit;
        }
        *arg5 = 0;
        return hit;
    }
    *arg5 = 0;
    return hit;
}

// 0x801C3EA4
// Looks up the entry whose first word equals `key` in a table of 0x80-byte entries
// (count at +0x18, base at +0x1C; binary search if the byte at +0x20 is set, else linear),
// then tail-calls fn_801C3F6C(table, entry) (entry is NULL when not found).
u8* fn_801C3EA4(u8* table, s32 key) {
    u32 k = (u32)key;
    u8* entry;
    if (*(u8*)(table + 0x20) != 0) {
        s32 hi = *(s32*)(table + 0x18);
        s32 lo = -1;
        while (hi - lo > 1) {
            s32 mid = (hi + lo) / 2;
            u8* base = *(u8**)(table + 0x1C);
            if (k <= *(u32*)(base + (mid << 7))) hi = mid;
            if (k > *(u32*)(*(u8**)(table + 0x1C) + (mid << 7))) lo = mid;
        }
        entry = *(u8**)(table + 0x1C) + (hi << 7);
        if (k != *(u32*)entry) entry = NULL;
    } else {
        s32 n = *(s32*)(table + 0x18);
        s32 i;
        entry = NULL;
        for (i = 0; i < n; i++) {
            u8* base = *(u8**)(table + 0x1C);
            if (k == *(u32*)(base + i * 0x80)) {
                entry = base + (i << 7);
                break;
            }
        }
    }
    return (u8*)fn_801C3F6C(table, entry);
}

// 0x801DFE7C
// Calls the hook stored in lbl_805FB898 (if any), forwarding the caller's arguments.
void fn_801DFE7C(s32 a) {
    if (lbl_805FB898 != NULL) {
        ((void (*)(s32))lbl_805FB898)(a);
    }
}

}  // extern "C"
