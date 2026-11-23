// Leaf functions that only copy or store values and return nothing (or their first argument).
// tools/gen_leaf.py gave them an f32 return because f1 was a scratch register; the callers
// declare the real return type, so the definitions live here and the generator skips them.
#include <stdint.h>

#include "types.h"
#include "engine/math.h"

extern f32 lbl_805FB05C;
extern f32 lbl_805FB060;
extern f32 lbl_805FB064;
extern f32 lbl_805FD118;
extern f32 lbl_805FE348;
extern f32 lbl_805FE34C;
extern f32 lbl_805FF0EC;
extern f32 lbl_805FF0E4;
extern f32 lbl_805FF20C;
extern f32 lbl_805FF210;
extern u8 lbl_805FF380[];
extern u8 lbl_805FF3B0[];
extern u8 lbl_805FF4C8[];

// 0x800E2F88
// Copies a position (3 floats) from src to dst+0x0C.
extern "C" void CIWaypointPath_StaticSetPosition(const f32* src, u8* dst) {
    f32 t0 = src[0];
    f32 t1 = src[1];
    f32 t2 = src[2];
    *(f32*)(dst + 0x0C) = t0;
    *(f32*)(dst + 0x10) = t1;
    *(f32*)(dst + 0x14) = t2;
}

// 0x800E2FA4
// Copies an orientation (4 floats) from src to dst+0x18.
extern "C" void CIWaypointPath_StaticSetOrientation(const f32* src, u8* dst) {
    f32 t0 = src[0];
    f32 t1 = src[1];
    f32 t2 = src[2];
    f32 t3 = src[3];
    *(f32*)(dst + 0x18) = t0;
    *(f32*)(dst + 0x1C) = t1;
    *(f32*)(dst + 0x20) = t2;
    *(f32*)(dst + 0x24) = t3;
}

// 0x80055D80
// Stores the ray direction (3 floats) at query+0x48.
extern "C" void RayQuerySetDir(u8* q, const Vec3* d) {
    f32 t0 = *(const f32*)((const u8*)d + 0);
    f32 t1 = *(const f32*)((const u8*)d + 4);
    f32 t2 = *(const f32*)((const u8*)d + 8);
    *(f32*)(q + 0x48) = t0;
    *(f32*)(q + 0x4C) = t1;
    *(f32*)(q + 0x50) = t2;
}

// 0x8004183C
// Moves three float globals: 805FB064 = 805FB05C, then 805FB060 and 805FB05C = 805FD118.
extern "C" void fn_8004183C(void) {
    f32 t0 = lbl_805FB05C;
    f32 t1 = lbl_805FD118;
    lbl_805FB064 = t0;
    lbl_805FB060 = t1;
    lbl_805FB05C = t1;
}

// 0x800651E4
// Copies 3 floats from src to dst.
extern "C" void fn_800651E4(const f32* src, f32* dst) {
    f32 t0 = src[0];
    f32 t1 = src[1];
    f32 t2 = src[2];
    dst[0] = t0;
    dst[1] = t1;
    dst[2] = t2;
}

// 0x800C09C0
// Sets a vector to (g0, g1, g0) from two float constants.
extern "C" void fn_800C09C0(Vec3* out) {
    f32 t0 = lbl_805FE348;
    f32 t1 = lbl_805FE34C;
    u8* p = (u8*)out;
    *(f32*)(p + 8) = t0;
    *(f32*)(p + 0) = t0;
    *(f32*)(p + 4) = t1;
}

// 0x80148D70
// Copies 4 floats from src to dst; r3 (dst) is left untouched, so it is also the result.
extern "C" f32* fn_80148D70(f32* dst, const f32* src) {
    f32 t0 = src[0];
    f32 t1 = src[1];
    f32 t2 = src[2];
    f32 t3 = src[3];
    dst[0] = t0;
    dst[1] = t1;
    dst[2] = t2;
    dst[3] = t3;
    return dst;
}

// 0x80149094
// Sets a vector to (g1, g0, g0) from two float constants.
extern "C" void fn_80149094(Vec3* out) {
    f32 t0 = lbl_805FF0EC;
    f32 t1 = lbl_805FF0E4;
    u8* p = (u8*)out;
    *(f32*)(p + 8) = t0;
    *(f32*)(p + 4) = t0;
    *(f32*)(p + 0) = t1;
}

// 0x8015DE58
// Writes four float constants to a, b, c, d (r3 unused).
extern "C" void fn_8015DE58(void* video, f32* a, f32* b, f32* c, f32* d) {
    f32 t0 = lbl_805FF20C;
    f32 t1 = lbl_805FF210;
    *a = t0;
    *b = t1;
    *c = t1;
    *d = t0;
}

// 0x80162D4C
// Copies a word and two floats from src to dst, then a constant to dst+0x0C (r3 unused).
extern "C" void fn_80162D4C(u8* mgr, u8* dst, u8* src, u8* link) {
    s32 t0 = *(s32*)(src + 0);
    f32 t1 = *(f32*)((u8*)&lbl_805FF380);
    *(u32*)(dst + 0) = (u32)t0;
    f32 t2 = *(f32*)(src + 4);
    *(f32*)(dst + 4) = t2;
    f32 t3 = *(f32*)(src + 8);
    *(f32*)(dst + 8) = t3;
    *(f32*)(dst + 12) = t1;
}

// 0x80163750
// Copies 3 floats from src to dst, then a constant to dst+0x0C and dst+0x10 (r3 unused).
extern "C" void fn_80163750(u8* mgr, u8* dst, u8* src, u8* link) {
    f32 t0 = *(f32*)(src + 0);
    f32 t1 = *(f32*)((u8*)&lbl_805FF3B0);
    *(f32*)(dst + 0) = t0;
    f32 t2 = *(f32*)(src + 4);
    *(f32*)(dst + 4) = t2;
    f32 t3 = *(f32*)(src + 8);
    *(f32*)(dst + 8) = t3;
    *(f32*)(dst + 12) = t1;
    *(f32*)(dst + 16) = t1;
}

// 0x80166314
// Copies 3 floats from src to dst, then a constant to dst+0x0C and dst+0x10 (r3 unused).
extern "C" void fn_80166314(u8* mgr, u8* dst, u8* src, u8* link) {
    f32 t0 = *(f32*)(src + 0);
    f32 t1 = *(f32*)((u8*)&lbl_805FF4C8);
    *(f32*)(dst + 0) = t0;
    f32 t2 = *(f32*)(src + 4);
    *(f32*)(dst + 4) = t2;
    f32 t3 = *(f32*)(src + 8);
    *(f32*)(dst + 8) = t3;
    *(f32*)(dst + 12) = t1;
    *(f32*)(dst + 16) = t1;
}
