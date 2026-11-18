// Collision math helpers (box / segment tests) and a scene-list buffer reset in 0x801Cxxxx.
#include "types.h"
#include "engine/math.h"

extern "C" {
void  BufferFree(void* p);                                             // buffer free
void* MemAllocDefault(u32 size);                                       // 0x801D4650
void  fn_801EB748(void* p);                                            // 0x801EB748
u8*  fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);            // 0x801EB7A4
void  fn_801EBA20(void* list, void* item);                             // 0x801EBA20
void* memset(void* p, int c, unsigned long n);

extern f32 lbl_805FFC30;  // 0x805FFC30 (float constant, zero)
extern f32 lbl_805FFC70;  // 0x805FFC70 (float constant, zero)
extern f32 lbl_805FFC80;  // 0x805FFC80 (float constant, zero)
extern f32 lbl_805FFCA0;  // 0x805FFCA0 (float constant, +epsilon)
extern f32 lbl_805FFCA4;  // 0x805FFCA4 (float constant, -epsilon)
extern f32 lbl_805FFCA8;  // 0x805FFCA8 (float constant, 0.0)
extern f32 lbl_805FFCAC;  // 0x805FFCAC (float constant, 1.0)

// Distance from point arg2 to the box centred at arg0 with half extents arg1:
// negative (largest axis penetration) when inside, else the distance to the box.
// 0x801C065C
f32 fn_801C065C(u8* arg0, u8* arg1, u8* arg2) {
    f32 dx = *(f32*)(arg2 + 0x0) - *(f32*)(arg0 + 0x0);
    f32 dy = *(f32*)(arg2 + 0x4) - *(f32*)(arg0 + 0x4);
    f32 dz = *(f32*)(arg2 + 0x8) - *(f32*)(arg0 + 0x8);
    f32 ex = *(f32*)(arg1 + 0x0);
    f32 ey = *(f32*)(arg1 + 0x4);
    f32 ez = *(f32*)(arg1 + 0x8);
    f32 ax = fabsf(dx);
    f32 ay = fabsf(dy);
    f32 az = fabsf(dz);
    f32 ox = ax - ex;
    f32 oy = ay - ey;
    f32 oz = az - ez;
    if (ex >= ax && ey >= ay && ez >= az) {
        f32 m = ox > oy ? ox : oy;
        m = m > oz ? m : oz;
        return m;
    }
    f32 cx = ox > lbl_805FFC30 ? ox : lbl_805FFC30;
    f32 cy = oy > lbl_805FFC30 ? oy : lbl_805FFC30;
    f32 cz = oz > lbl_805FFC30 ? oz : lbl_805FFC30;
    f32 len2 = cz * cz + (cx * cx + cy * cy);
    f32 len = 0.0f;
    if (!(len2 <= lbl_805FFC30)) len = sqrtf(len2);
    return len;
}

// Returns 1 if point arg2 lies inside the box at arg0 with half extents arg1.
// 0x801C0830
s32 fn_801C0830(u8* arg0, u8* arg1, u8* arg2) {
    f32 ax = fabsf(*(f32*)(arg2 + 0x0) - *(f32*)(arg0 + 0x0));
    f32 ay = fabsf(*(f32*)(arg2 + 0x4) - *(f32*)(arg0 + 0x4));
    f32 az = fabsf(*(f32*)(arg2 + 0x8) - *(f32*)(arg0 + 0x8));
    s32 r = 0;
    if (*(f32*)(arg1 + 0x0) >= ax && *(f32*)(arg1 + 0x4) >= ay && *(f32*)(arg1 + 0x8) >= az) r = 1;
    return r;
}

// Returns 1 if two boxes (centres arg0/arg2, half extents arg1/arg3) overlap.
// 0x801C08CC
s32 fn_801C08CC(u8* arg0, u8* arg1, u8* arg2, u8* arg3) {
    f32 ax = fabsf(*(f32*)(arg0 + 0x0) - *(f32*)(arg2 + 0x0));
    f32 ay = fabsf(*(f32*)(arg0 + 0x4) - *(f32*)(arg2 + 0x4));
    f32 az = fabsf(*(f32*)(arg0 + 0x8) - *(f32*)(arg2 + 0x8));
    f32 sx = *(f32*)(arg1 + 0x0) + *(f32*)(arg3 + 0x0);
    f32 sy = *(f32*)(arg1 + 0x4) + *(f32*)(arg3 + 0x4);
    f32 sz = *(f32*)(arg1 + 0x8) + *(f32*)(arg3 + 0x8);
    s32 r = 0;
    if (sx >= ax && sy >= ay && sz >= az) r = 1;
    return r;
}

// Slab clip helper: narrows the [*arg0, *arg1] parameter range by the constraint
// farg0 * t >= farg1; returns 0 if the range becomes empty.
// 0x801C1960
s32 fn_801C1960(f32* arg0, f32* arg1, f32 farg0, f32 farg1) {
    if (farg0 > lbl_805FFC70) {
        if (farg1 > farg0 * *arg1) return 0;
        if (farg1 > farg0 * *arg0) *arg0 = farg1 / farg0;
        return 1;
    }
    if (farg0 < lbl_805FFC70) {
        if (farg1 > farg0 * *arg0) return 0;
        if (farg1 > farg0 * *arg1) *arg1 = farg1 / farg0;
        return 1;
    }
    return farg1 <= lbl_805FFC70;
}

// Returns 1 if point arg0 is within radius farg0 of the segment arg1 -> arg1+arg2.
// 0x801C2B40
s32 fn_801C2B40(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    f32 dx = *(f32*)(arg0 + 0x0) - *(f32*)(arg1 + 0x0);
    f32 dy = *(f32*)(arg0 + 0x4) - *(f32*)(arg1 + 0x4);
    f32 dz = *(f32*)(arg0 + 0x8) - *(f32*)(arg1 + 0x8);
    f32 vx = *(f32*)(arg2 + 0x0);
    f32 vy = *(f32*)(arg2 + 0x4);
    f32 vz = *(f32*)(arg2 + 0x8);
    f32 dot = dz * vz + (dx * vx + dy * vy);
    if (!(dot <= lbl_805FFC80)) {
        f32 len2 = vz * vz + (vx * vx + vy * vy);
        if (dot >= len2) {
            dx = dx - vx;
            dy = dy - vy;
            dz = dz - vz;
        } else {
            f32 t = dot / len2;
            dx = dx - vx * t;
            dy = dy - vy * t;
            dz = dz - vz * t;
        }
    }
    return (dz * dz + (dx * dx + dy * dy)) <= farg0 * farg0;
}

// Like fn_801C2B40 but for a ray (no end clamp): 1 if point arg0 is within farg0 of arg1 + t*arg2, t >= 0.
// 0x801C3074
s32 fn_801C3074(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    f32 dx = *(f32*)(arg0 + 0x0) - *(f32*)(arg1 + 0x0);
    f32 dy = *(f32*)(arg0 + 0x4) - *(f32*)(arg1 + 0x4);
    f32 dz = *(f32*)(arg0 + 0x8) - *(f32*)(arg1 + 0x8);
    f32 vx = *(f32*)(arg2 + 0x0);
    f32 vy = *(f32*)(arg2 + 0x4);
    f32 vz = *(f32*)(arg2 + 0x8);
    f32 dot = dz * vz + (dx * vx + dy * vy);
    if (!(dot <= lbl_805FFC80)) {
        f32 len2 = vz * vz + (vx * vx + vy * vy);
        f32 t = dot / len2;
        dx = dx - vx * t;
        dy = dy - vy * t;
        dz = dz - vz * t;
    }
    return (dz * dz + (dx * dx + dy * dy)) <= farg0 * farg0;
}

// Projects point arg0 onto the infinite line arg1 -> arg2. Writes t to *arg3 (if non-null) and the
// closest point to arg4; returns -1 if the line is degenerate, else 0.
// 0x801C34CC
s32 fn_801C34CC(u8* arg0, u8* arg1, u8* arg2, f32* arg3, u8* arg4) {
    f32 abx = *(f32*)(arg2 + 0x0) - *(f32*)(arg1 + 0x0);
    f32 aby = *(f32*)(arg2 + 0x4) - *(f32*)(arg1 + 0x4);
    f32 abz = *(f32*)(arg2 + 0x8) - *(f32*)(arg1 + 0x8);
    f32 apx = *(f32*)(arg0 + 0x0) - *(f32*)(arg1 + 0x0);
    f32 apy = *(f32*)(arg0 + 0x4) - *(f32*)(arg1 + 0x4);
    f32 apz = *(f32*)(arg0 + 0x8) - *(f32*)(arg1 + 0x8);
    f32 len2 = abz * abz + (abx * abx + aby * aby);
    if (len2 <= lbl_805FFCA0 && len2 >= lbl_805FFCA4) return -1;
    f32 t = (apz * abz + (apx * abx + apy * aby)) / len2;
    f32 cx = *(f32*)(arg1 + 0x0) + abx * t;
    f32 cy = *(f32*)(arg1 + 0x4) + aby * t;
    f32 cz = *(f32*)(arg1 + 0x8) + abz * t;
    if (arg3) *arg3 = t;
    *(f32*)(arg4 + 0x0) = cx;
    *(f32*)(arg4 + 0x4) = cy;
    *(f32*)(arg4 + 0x8) = cz;
    return 0;
}

// Closest point on the segment arg1 -> arg2 to arg0: projection as in fn_801C34CC, then clamped
// to the end points by t. Returns -1 if arg3 is null or the segment is degenerate.
// 0x801C3624
s32 fn_801C3624(u8* arg0, u8* arg1, u8* arg2, f32* arg3, u8* arg4) {
    s32 r = -1;
    if (arg3) {
        f32 abx = *(f32*)(arg2 + 0x0) - *(f32*)(arg1 + 0x0);
        f32 aby = *(f32*)(arg2 + 0x4) - *(f32*)(arg1 + 0x4);
        f32 abz = *(f32*)(arg2 + 0x8) - *(f32*)(arg1 + 0x8);
        f32 apx = *(f32*)(arg0 + 0x0) - *(f32*)(arg1 + 0x0);
        f32 apy = *(f32*)(arg0 + 0x4) - *(f32*)(arg1 + 0x4);
        f32 apz = *(f32*)(arg0 + 0x8) - *(f32*)(arg1 + 0x8);
        f32 len2 = abz * abz + (abx * abx + aby * aby);
        if (len2 <= lbl_805FFCA0 && len2 >= lbl_805FFCA4) return -1;
        f32 t = (apz * abz + (apx * abx + apy * aby)) / len2;
        f32 cx = *(f32*)(arg1 + 0x0) + abx * t;
        f32 cy = *(f32*)(arg1 + 0x4) + aby * t;
        f32 cz = *(f32*)(arg1 + 0x8) + abz * t;
        *arg3 = t;
        *(f32*)(arg4 + 0x0) = cx;
        *(f32*)(arg4 + 0x4) = cy;
        *(f32*)(arg4 + 0x8) = cz;
        r = 0;
        if (*arg3 < lbl_805FFCA8) {
            *(f32*)(arg4 + 0x0) = *(f32*)(arg1 + 0x0);
            *(f32*)(arg4 + 0x4) = *(f32*)(arg1 + 0x4);
            *(f32*)(arg4 + 0x8) = *(f32*)(arg1 + 0x8);
        } else if (*arg3 > lbl_805FFCAC) {
            *(f32*)(arg4 + 0x0) = *(f32*)(arg2 + 0x0);
            *(f32*)(arg4 + 0x4) = *(f32*)(arg2 + 0x4);
            *(f32*)(arg4 + 0x8) = *(f32*)(arg2 + 0x8);
        }
    }
    return r;
}

// Clears the 12 bytes at +0x18, frees the two list buffers at +0x10 / +0x14 and allocates
// fresh ones (0x2C bytes each, initialised with sizes 0x3A0 and 0x68).
// 0x801C383C
void fn_801C383C(u8* arg0) {
    memset(arg0 + 0x18, 0, 0xC);
    if (*(u8**)(arg0 + 0x10)) {
        fn_801EB748(*(u8**)(arg0 + 0x10));
        BufferFree(*(u8**)(arg0 + 0x10));
        *(u8**)(arg0 + 0x10) = 0;
    }
    if (*(u8**)(arg0 + 0x14)) {
        fn_801EB748(*(u8**)(arg0 + 0x14));
        BufferFree(*(u8**)(arg0 + 0x14));
        *(u8**)(arg0 + 0x14) = 0;
    }
    if (*(u8**)(arg0 + 0x10) == 0) {
        u8* buf = (u8*)MemAllocDefault(0x2C);
        *(u8**)(arg0 + 0x10) = buf;
        if (buf) fn_801EB7A4(buf, 0x3A0, 1, 0x20, 0);
    }
    if (*(u8**)(arg0 + 0x14) == 0) {
        u8* buf = (u8*)MemAllocDefault(0x2C);
        *(u8**)(arg0 + 0x14) = buf;
        if (buf) fn_801EB7A4(buf, 0x68, 1, 0x20, 0);
    }
}

// For each of the arg0[0x18] entries (0x80 bytes each, array at +0x1C): drains the node list of
// the object at +0x7C (if entry flag +0x4C is set) or +0x78, calling fn_801EBA20 per node.
// Nodes link through the pointer at node-4; a node pointer is that link + 8.
// 0x801C3924
void fn_801C3924(u8* arg0) {
    s32 i = 0;
    u8* e = *(u8**)(arg0 + 0x1C);
    while (i < *(s32*)(arg0 + 0x18)) {
        u8* list = (*(s32*)(e + 0x4C) != 0) ? *(u8**)(e + 0x7C) : *(u8**)(e + 0x78);
        u8* head = *(u8**)(list + 0x14);
        u8* cur = head ? head + 8 : 0;
        while (cur) {
            u8* nl = *(u8**)(cur - 4);
            u8* next = nl ? nl + 8 : 0;
            fn_801EBA20(list, cur);
            if (!next) break;
            u8* nl2 = *(u8**)(next - 4);
            cur = nl2 ? nl2 + 8 : 0;
        }
        i++;
        e += 0x80;
    }
}

}  // extern "C"
