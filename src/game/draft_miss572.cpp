#include "types.h"

// Vec3 constant writers: each function stores a 3-float vector {x, y, x} into a global
// record (x and z come from the first sdata2 float, y from the second).

extern "C" {
extern u8 lbl_803C6B50[];
extern u8 lbl_803C6BA0[];
extern u8 lbl_803C6BB0[];
extern u8 lbl_803C6BC0[];
extern u8 lbl_803C6BD0[];
extern u8 lbl_803C6BE0[];
extern u8 lbl_803C6C38[];
extern u8 lbl_803C6C48[];
extern u8 lbl_803C6C58[];
extern u8 lbl_803C6C68[];
extern u8 lbl_803C6C98[];
extern u8 lbl_803C6CA8[];
extern u8 lbl_803C6CB8[];
extern u8 lbl_803C6CC8[];
extern u8 lbl_803C6CD8[];
extern u8 lbl_803C6CE8[];
extern u8 lbl_803C6CF8[];
extern u8 lbl_803C6D08[];
extern u8 lbl_803C6D38[];
extern u8 lbl_803C6D48[];

extern f32 lbl_805FDCA8;
extern f32 lbl_805FDCAC;
extern f32 lbl_805FDD0C;
extern f32 lbl_805FDD10;
extern f32 lbl_805FDD18;
extern f32 lbl_805FDD1C;
extern f32 lbl_805FDD78;
extern f32 lbl_805FDDBC;
extern f32 lbl_805FDDD0;
extern f32 lbl_805FDDF4;
extern f32 lbl_805FDE08;
extern f32 lbl_805FDE0C;
extern f32 lbl_805FDE90;
extern f32 lbl_805FDE94;
extern f32 lbl_805FDE9C;
extern f32 lbl_805FDEA0;
extern f32 lbl_805FDEA8;
extern f32 lbl_805FDEAC;
extern f32 lbl_805FDEB0;
extern f32 lbl_805FDEB4;
extern f32 lbl_805FDF98;
extern f32 lbl_805FDF9C;
extern f32 lbl_805FDFC0;
extern f32 lbl_805FDFC4;
extern f32 lbl_805FE018;
extern f32 lbl_805FE030;
extern f32 lbl_805FE038;
extern f32 lbl_805FE03C;
extern f32 lbl_805FE070;
extern f32 lbl_805FE074;
extern f32 lbl_805FE090;
extern f32 lbl_805FE094;
extern f32 lbl_805FE0B0;
extern f32 lbl_805FE0BC;
extern f32 lbl_805FE0E8;
extern f32 lbl_805FE100;
extern f32 lbl_805FE170;
extern f32 lbl_805FE174;
extern f32 lbl_805FE178;
extern f32 lbl_805FE17C;
}

extern "C" {

// 0x8009B244
void fn_8009B244(void) {
    f32 x = lbl_805FDCA8, y = lbl_805FDCAC;
    *(f32*)(lbl_803C6B50 + 0x0) = x;
    *(f32*)(lbl_803C6B50 + 0x4) = y;
    *(f32*)(lbl_803C6B50 + 0x8) = x;
}

// 0x8009C2A8
void fn_8009C2A8(void) {
    f32 x = lbl_805FDD0C, y = lbl_805FDD10;
    *(f32*)(lbl_803C6BA0 + 0x0) = x;
    *(f32*)(lbl_803C6BA0 + 0x4) = y;
    *(f32*)(lbl_803C6BA0 + 0x8) = x;
}

// 0x8009D814
void fn_8009D814(void) {
    f32 x = lbl_805FDD18, y = lbl_805FDD1C;
    *(f32*)(lbl_803C6BB0 + 0x0) = x;
    *(f32*)(lbl_803C6BB0 + 0x4) = y;
    *(f32*)(lbl_803C6BB0 + 0x8) = x;
}

// 0x8009E144
void fn_8009E144(void) {
    f32 x = lbl_805FDD78, y = lbl_805FDDBC;
    *(f32*)(lbl_803C6BC0 + 0x0) = x;
    *(f32*)(lbl_803C6BC0 + 0x4) = y;
    *(f32*)(lbl_803C6BC0 + 0x8) = x;
}

// 0x8009E7F8
void fn_8009E7F8(void) {
    f32 x = lbl_805FDDD0, y = lbl_805FDDF4;
    *(f32*)(lbl_803C6BD0 + 0x0) = x;
    *(f32*)(lbl_803C6BD0 + 0x4) = y;
    *(f32*)(lbl_803C6BD0 + 0x8) = x;
}

// 0x8009F64C
void fn_8009F64C(void) {
    f32 x = lbl_805FDE08, y = lbl_805FDE0C;
    *(f32*)(lbl_803C6BE0 + 0x0) = x;
    *(f32*)(lbl_803C6BE0 + 0x4) = y;
    *(f32*)(lbl_803C6BE0 + 0x8) = x;
}

// 0x800A38D4
void fn_800A38D4(void) {
    f32 x = lbl_805FDE90, y = lbl_805FDE94;
    *(f32*)(lbl_803C6C38 + 0x0) = x;
    *(f32*)(lbl_803C6C38 + 0x4) = y;
    *(f32*)(lbl_803C6C38 + 0x8) = x;
}

// 0x800A5BAC
void fn_800A5BAC(void) {
    f32 x = lbl_805FDE9C, y = lbl_805FDEA0;
    *(f32*)(lbl_803C6C48 + 0x0) = x;
    *(f32*)(lbl_803C6C48 + 0x4) = y;
    *(f32*)(lbl_803C6C48 + 0x8) = x;
}

// 0x800A6518
void fn_800A6518(void) {
    f32 x = lbl_805FDEAC, y = lbl_805FDEA8;
    *(f32*)(lbl_803C6C58 + 0x0) = x;
    *(f32*)(lbl_803C6C58 + 0x4) = y;
    *(f32*)(lbl_803C6C58 + 0x8) = x;
}

// 0x800A7980
void fn_800A7980(void) {
    f32 x = lbl_805FDEB0, y = lbl_805FDEB4;
    *(f32*)(lbl_803C6C68 + 0x0) = x;
    *(f32*)(lbl_803C6C68 + 0x4) = y;
    *(f32*)(lbl_803C6C68 + 0x8) = x;
}

// 0x800ABDF0
void fn_800ABDF0(void) {
    f32 x = lbl_805FDF98, y = lbl_805FDF9C;
    *(f32*)(lbl_803C6C98 + 0x0) = x;
    *(f32*)(lbl_803C6C98 + 0x4) = y;
    *(f32*)(lbl_803C6C98 + 0x8) = x;
}

// 0x800ACD04
void fn_800ACD04(void) {
    f32 x = lbl_805FDFC0, y = lbl_805FDFC4;
    *(f32*)(lbl_803C6CA8 + 0x0) = x;
    *(f32*)(lbl_803C6CA8 + 0x4) = y;
    *(f32*)(lbl_803C6CA8 + 0x8) = x;
}

// 0x800AD824
void fn_800AD824(void) {
    f32 x = lbl_805FE018, y = lbl_805FE030;
    *(f32*)(lbl_803C6CB8 + 0x0) = x;
    *(f32*)(lbl_803C6CB8 + 0x4) = y;
    *(f32*)(lbl_803C6CB8 + 0x8) = x;
}

// 0x800AF548
void fn_800AF548(void) {
    f32 x = lbl_805FE03C, y = lbl_805FE038;
    *(f32*)(lbl_803C6CC8 + 0x0) = x;
    *(f32*)(lbl_803C6CC8 + 0x4) = y;
    *(f32*)(lbl_803C6CC8 + 0x8) = x;
}

// 0x800B066C
void fn_800B066C(void) {
    f32 x = lbl_805FE070, y = lbl_805FE074;
    *(f32*)(lbl_803C6CD8 + 0x0) = x;
    *(f32*)(lbl_803C6CD8 + 0x4) = y;
    *(f32*)(lbl_803C6CD8 + 0x8) = x;
}

// 0x800B1478
void fn_800B1478(void) {
    f32 x = lbl_805FE090, y = lbl_805FE094;
    *(f32*)(lbl_803C6CE8 + 0x0) = x;
    *(f32*)(lbl_803C6CE8 + 0x4) = y;
    *(f32*)(lbl_803C6CE8 + 0x8) = x;
}

// 0x800B1EBC
void fn_800B1EBC(void) {
    f32 x = lbl_805FE0B0, y = lbl_805FE0BC;
    *(f32*)(lbl_803C6CF8 + 0x0) = x;
    *(f32*)(lbl_803C6CF8 + 0x4) = y;
    *(f32*)(lbl_803C6CF8 + 0x8) = x;
}

// 0x800B2364
void fn_800B2364(void) {
    f32 x = lbl_805FE0E8, y = lbl_805FE100;
    *(f32*)(lbl_803C6D08 + 0x0) = x;
    *(f32*)(lbl_803C6D08 + 0x4) = y;
    *(f32*)(lbl_803C6D08 + 0x8) = x;
}

// 0x800B5E44
void fn_800B5E44(void) {
    f32 x = lbl_805FE170, y = lbl_805FE174;
    *(f32*)(lbl_803C6D38 + 0x0) = x;
    *(f32*)(lbl_803C6D38 + 0x4) = y;
    *(f32*)(lbl_803C6D38 + 0x8) = x;
}

// 0x800B7914
void fn_800B7914(void) {
    f32 x = lbl_805FE178, y = lbl_805FE17C;
    *(f32*)(lbl_803C6D48 + 0x0) = x;
    *(f32*)(lbl_803C6D48 + 0x4) = y;
    *(f32*)(lbl_803C6D48 + 0x8) = x;
}

}  // extern "C"
