// Small functions (batch miss571): 3-float record writers that store a float pair
// (f1 at +0 and +8, f0 at +4) into a global record.
#include "types.h"

extern "C" {
extern u8 lbl_803C6950[];  // 0x803C6950 (3-float record)
extern u8 lbl_803C6960[];  // 0x803C6960 (3-float record)
extern u8 lbl_803C69A8[];  // 0x803C69A8 (3-float record)
extern u8 lbl_803C69B8[];  // 0x803C69B8 (3-float record)
extern u8 lbl_803C69C8[];  // 0x803C69C8 (3-float record)
extern u8 lbl_803C69D8[];  // 0x803C69D8 (3-float record)
extern u8 lbl_803C6A08[];  // 0x803C6A08 (3-float record)
extern u8 lbl_803C6A18[];  // 0x803C6A18 (3-float record)
extern u8 lbl_803C6A28[];  // 0x803C6A28 (3-float record)
extern u8 lbl_803C6A64[];  // 0x803C6A64 (3-float record)
extern u8 lbl_803C6A7C[];  // 0x803C6A7C (3-float record)
extern u8 lbl_803C6A94[];  // 0x803C6A94 (3-float record)
extern u8 lbl_803C6AE0[];  // 0x803C6AE0 (3-float record)
extern u8 lbl_803C6B00[];  // 0x803C6B00 (3-float record)
extern u8 lbl_803C6B10[];  // 0x803C6B10 (3-float record)
extern u8 lbl_803C6B20[];  // 0x803C6B20 (3-float record)

extern f32 lbl_805FD740;  // 0x805FD740 (float constant)
extern f32 lbl_805FD744;  // 0x805FD744 (float constant)
extern f32 lbl_805FD748;  // 0x805FD748 (float constant)
extern f32 lbl_805FD760;  // 0x805FD760 (float constant)
extern f32 lbl_805FD848;  // 0x805FD848 (float constant)
extern f32 lbl_805FD870;  // 0x805FD870 (float constant)
extern f32 lbl_805FD87C;  // 0x805FD87C (float constant)
extern f32 lbl_805FD878;  // 0x805FD878 (float constant)
extern f32 lbl_805FD8C8;  // 0x805FD8C8 (float constant)
extern f32 lbl_805FD8F0;  // 0x805FD8F0 (float constant)
extern f32 lbl_805FD920;  // 0x805FD920 (float constant)
extern f32 lbl_805FD924;  // 0x805FD924 (float constant)
extern f32 lbl_805FD9B0;  // 0x805FD9B0 (float constant)
extern f32 lbl_805FD9B4;  // 0x805FD9B4 (float constant)
extern f32 lbl_805FDA08;  // 0x805FDA08 (float constant)
extern f32 lbl_805FDA14;  // 0x805FDA14 (float constant)
extern f32 lbl_805FDA18;  // 0x805FDA18 (float constant)
extern f32 lbl_805FDA1C;  // 0x805FDA1C (float constant)
extern f32 lbl_805FDB00;  // 0x805FDB00 (float constant)
extern f32 lbl_805FDB04;  // 0x805FDB04 (float constant)
extern f32 lbl_805FDB08;  // 0x805FDB08 (float constant)
extern f32 lbl_805FDB0C;  // 0x805FDB0C (float constant)
extern f32 lbl_805FDB20;  // 0x805FDB20 (float constant)
extern f32 lbl_805FDB24;  // 0x805FDB24 (float constant)
extern f32 lbl_805FDBA0;  // 0x805FDBA0 (float constant)
extern f32 lbl_805FDBD8;  // 0x805FDBD8 (float constant)
extern f32 lbl_805FDC04;  // 0x805FDC04 (float constant)
extern f32 lbl_805FDC08;  // 0x805FDC08 (float constant)
extern f32 lbl_805FDC1C;  // 0x805FDC1C (float constant)
extern f32 lbl_805FDC20;  // 0x805FDC20 (float constant)
extern f32 lbl_805FDC28;  // 0x805FDC28 (float constant)
extern f32 lbl_805FDC2C;  // 0x805FDC2C (float constant)
}

extern "C" {

// 0x8007CEB8
void fn_8007CEB8(void) {
    *(f32*)(lbl_803C6950 + 0x0) = lbl_805FD740;
    *(f32*)(lbl_803C6950 + 0x4) = lbl_805FD744;
    *(f32*)(lbl_803C6950 + 0x8) = lbl_805FD740;
}

// 0x8007D4BC
void fn_8007D4BC(void) {
    *(f32*)(lbl_803C6960 + 0x0) = lbl_805FD748;
    *(f32*)(lbl_803C6960 + 0x4) = lbl_805FD760;
    *(f32*)(lbl_803C6960 + 0x8) = lbl_805FD748;
}

// 0x80083A4C
void fn_80083A4C(void) {
    *(f32*)(lbl_803C69A8 + 0x0) = lbl_805FD848;
    *(f32*)(lbl_803C69A8 + 0x4) = lbl_805FD870;
    *(f32*)(lbl_803C69A8 + 0x8) = lbl_805FD848;
}

// 0x80084E74
void fn_80084E74(void) {
    *(f32*)(lbl_803C69B8 + 0x0) = lbl_805FD87C;
    *(f32*)(lbl_803C69B8 + 0x4) = lbl_805FD878;
    *(f32*)(lbl_803C69B8 + 0x8) = lbl_805FD87C;
}

// 0x80086760
void fn_80086760(void) {
    *(f32*)(lbl_803C69C8 + 0x0) = lbl_805FD8C8;
    *(f32*)(lbl_803C69C8 + 0x4) = lbl_805FD8F0;
    *(f32*)(lbl_803C69C8 + 0x8) = lbl_805FD8C8;
}

// 0x80086BC8
void fn_80086BC8(void) {
    *(f32*)(lbl_803C69D8 + 0x0) = lbl_805FD920;
    *(f32*)(lbl_803C69D8 + 0x4) = lbl_805FD924;
    *(f32*)(lbl_803C69D8 + 0x8) = lbl_805FD920;
}

// 0x8008C9D8
void fn_8008C9D8(void) {
    *(f32*)(lbl_803C6A08 + 0x0) = lbl_805FD9B0;
    *(f32*)(lbl_803C6A08 + 0x4) = lbl_805FD9B4;
    *(f32*)(lbl_803C6A08 + 0x8) = lbl_805FD9B0;
}

// 0x8008CB90
void fn_8008CB90(void) {
    *(f32*)(lbl_803C6A18 + 0x0) = lbl_805FDA08;
    *(f32*)(lbl_803C6A18 + 0x4) = lbl_805FDA14;
    *(f32*)(lbl_803C6A18 + 0x8) = lbl_805FDA08;
}

// 0x8008ED18
void fn_8008ED18(void) {
    *(f32*)(lbl_803C6A28 + 0x0) = lbl_805FDA18;
    *(f32*)(lbl_803C6A28 + 0x4) = lbl_805FDA1C;
    *(f32*)(lbl_803C6A28 + 0x8) = lbl_805FDA18;
}

// 0x800920EC
void fn_800920EC(void) {
    *(f32*)(lbl_803C6A64 + 0x0) = lbl_805FDB00;
    *(f32*)(lbl_803C6A64 + 0x4) = lbl_805FDB04;
    *(f32*)(lbl_803C6A64 + 0x8) = lbl_805FDB00;
}

// 0x80093884
void fn_80093884(void) {
    *(f32*)(lbl_803C6A7C + 0x0) = lbl_805FDB08;
    *(f32*)(lbl_803C6A7C + 0x4) = lbl_805FDB0C;
    *(f32*)(lbl_803C6A7C + 0x8) = lbl_805FDB08;
}

// 0x8009409C
void fn_8009409C(void) {
    *(f32*)(lbl_803C6A94 + 0x0) = lbl_805FDB20;
    *(f32*)(lbl_803C6A94 + 0x4) = lbl_805FDB24;
    *(f32*)(lbl_803C6A94 + 0x8) = lbl_805FDB20;
}

// 0x80098644
void fn_80098644(void) {
    *(f32*)(lbl_803C6AE0 + 0x0) = lbl_805FDBA0;
    *(f32*)(lbl_803C6AE0 + 0x4) = lbl_805FDBD8;
    *(f32*)(lbl_803C6AE0 + 0x8) = lbl_805FDBA0;
}

// 0x80098BA0
void fn_80098BA0(void) {
    *(f32*)(lbl_803C6B00 + 0x0) = lbl_805FDC08;
    *(f32*)(lbl_803C6B00 + 0x4) = lbl_805FDC04;
    *(f32*)(lbl_803C6B00 + 0x8) = lbl_805FDC08;
}

// 0x80098D88
void fn_80098D88(void) {
    *(f32*)(lbl_803C6B10 + 0x0) = lbl_805FDC1C;
    *(f32*)(lbl_803C6B10 + 0x4) = lbl_805FDC20;
    *(f32*)(lbl_803C6B10 + 0x8) = lbl_805FDC1C;
}

// 0x80099AB8
void fn_80099AB8(void) {
    *(f32*)(lbl_803C6B20 + 0x0) = lbl_805FDC28;
    *(f32*)(lbl_803C6B20 + 0x4) = lbl_805FDC2C;
    *(f32*)(lbl_803C6B20 + 0x8) = lbl_805FDC28;
}

}  // extern "C"
