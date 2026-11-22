// Renderer/engine leftovers: matrix-block reset, texel fetch, particle-record init, small accessors.
#include "types.h"

extern "C" {
extern u8  lbl_80400FF0[];        // 0x80400FF0 (renderer matrix block)
extern u8* lbl_805FB608;          // 0x805FB608 (heap object: slot +0 = alloc, slot +4 = free)
s32 fn_80332EE0(s32 x);           // 0x80332EE0 (abs)
}
extern "C++" f32 lbl_805FE668;    // 0x805FE668 (float constant used as zero)

extern "C" {

// 0x800CA934
// Tail call to abs().
s32 fn_800CA934(s32 x) {
    return fn_80332EE0(x);
}

// 0x800D8FA0
// Resets a state at +0x10/+0x14/+0x1C, then tail-calls virtual slot +0x98.
void fn_800D8FA0(u8* p) {
    f32 z = lbl_805FE668;
    *(s32*)(p + 0x10) = 0;
    *(f32*)(p + 0x14) = z;
    *(f32*)(p + 0x1c) = z;
    (*(void (**)(u8*))(*(u8**)p + 0x98))(p);
}

// 0x80156A90
// Zeroes the first three floats of the renderer matrix block.
void fn_80156A90(void) {
    *(f32*)(lbl_80400FF0 + 0x8) = 0.0f;
    *(f32*)(lbl_80400FF0 + 0x4) = 0.0f;
    *(f32*)(lbl_80400FF0 + 0x0) = 0.0f;
}

// 0x8015B82C
// Reads one texel from a GX-tiled image (u16 w @0, u16 h @2, u8 bpp @0xA, data ptr @0x18).
// Handles 32bpp (two 16-bit planes per 4x4 tile), 16bpp and 8bpp; out of range or 4bpp returns 0.
u32 fn_8015B82C(u8* img, s32 x, s32 y) {
    s32 w = *(u16*)(img + 0x0);
    s32 h = *(u16*)(img + 0x2);
    if (x < 0 || x >= w || y < 0 || y >= h) return 0;
    s32 bpp = *(u8*)(img + 0xA);
    s32 rowBytes = (w * bpp) / 8;
    if (bpp == 4) return 0;
    u8* data = *(u8**)(img + 0x18);
    if (bpp == 0x20) {
        u8* a = data + (((x / 4) + (y / 4) * (rowBytes / 16)) << 6) + (y % 4) * 8 + (x % 4) * 2;
        return ((u32)*(u16*)a << 16) + *(u16*)(a + 0x20);
    }
    s32 ppb = 8 / ((bpp >> 3) & 0x1F);
    s32 idx = (y / 4) * (((rowBytes / 8) * ppb) * 4) + (x / ppb) * (ppb * 4) + ((x % ppb) + (y % 4) * 8);
    if (bpp == 0x10) return *(u16*)(data + idx * 2);
    if (bpp == 8) return *(u8*)(data + idx);
    return 0;
}

// 0x80176330
// Allocates and initialises a 0x68-byte record owned by arg0 (heap alloc via slot +0 of lbl_805FB608).
// (The asm also stores 1.0/0.0 to four dead stack Vec4 locals; they are not reproduced.)
void fn_80176330(u8* arg0) {
    u8* r = (*(u8* (**)(s32, s32, s32, u8*, s32))lbl_805FB608)(0x68, 0x10, 0x12, arg0 + 0x80, 0);
    *(u8**)(r + 0x0) = arg0;
    *(u8*)(r + 0x4) = 1;
    *(u8*)(r + 0x5) = 0;
    *(f32*)(r + 0x8) = 1.0f;
    *(f32*)(r + 0xC) = 1.0f;
    *(f32*)(r + 0x10) = 1.0f;
    *(f32*)(r + 0x14) = 1.0f;
    *(f32*)(r + 0x18) = 1.0f;
    *(f32*)(r + 0x1C) = 1.0f;
    *(f32*)(r + 0x20) = 1.0f;
    *(f32*)(r + 0x24) = 1.0f;
    *(f32*)(r + 0x28) = 0.0f;
    *(f32*)(r + 0x2C) = 0.0f;
    *(f32*)(r + 0x30) = 0.0f;
    *(f32*)(r + 0x34) = 0.0f;
    *(f32*)(r + 0x38) = 1.0f;
    *(f32*)(r + 0x3C) = 1.0f;
    *(f32*)(r + 0x40) = 1.0f;
    *(f32*)(r + 0x44) = 1.0f;
    *(f32*)(r + 0x48) = 10.0f;
    *(f32*)(r + 0x4C) = 1.0f;
    *(f32*)(r + 0x50) = 0.0f;
    *(s32*)(r + 0x54) = 0;
    *(u8*)(r + 0x58) = 1;
    *(u8*)(r + 0x59) = 1;
    *(u8*)(r + 0x5A) = 1;
    *(s32*)(r + 0x5C) = 5;
    *(s32*)(r + 0x60) = 2;
    *(u8*)(r + 0x64) = 1;
    *(u8*)(r + 0x65) = 1;
    *(u8*)(r + 0x66) = 1;
    *(u8*)(r + 0x67) = 1;
}

// 0x801767AC
// Tail call: heap free slot (+4) of lbl_805FB608, called as (p, heap).
void fn_801767AC(void* p) {
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(p, lbl_805FB608);
}

// 0x80176B98
// Field getter by index (jump table of 27): returns the raw 32-bit value (floats as bit patterns).
u32 fn_80176B98(u8* p, u32 idx) {
    switch (idx) {
    case 0:  return *(u8*)(p + 0x4);
    case 1:  return *(u8*)(p + 0x5);
    case 2:  return *(u32*)(p + 0x8);
    case 3:  return *(u32*)(p + 0xC);
    case 4:  return *(u32*)(p + 0x10);
    case 5:  return *(u32*)(p + 0x4C);
    case 6:  return *(u32*)(p + 0x18);
    case 7:  return *(u32*)(p + 0x1C);
    case 8:  return *(u32*)(p + 0x20);
    case 9:  return *(u32*)(p + 0x28);
    case 10: return *(u32*)(p + 0x2C);
    case 11: return *(u32*)(p + 0x30);
    case 12: return *(u32*)(p + 0x38);
    case 13: return *(u32*)(p + 0x3C);
    case 14: return *(u32*)(p + 0x40);
    case 15: return *(u32*)(p + 0x48);
    case 16: return *(u32*)(p + 0x50);
    case 17: return *(u32*)(p + 0x54);
    case 18: return *(u8*)(p + 0x58);
    case 19: return *(u8*)(p + 0x59);
    case 20: return *(u8*)(p + 0x5A);
    case 21: return *(u32*)(p + 0x5C);
    case 22: return *(u32*)(p + 0x60);
    case 23: return *(u8*)(p + 0x64);
    case 24: return *(u8*)(p + 0x65);
    case 25: return *(u8*)(p + 0x66);
    case 26: return *(u8*)(p + 0x67);
    default: return 0;
    }
}

}  // extern "C"
