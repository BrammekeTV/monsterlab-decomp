// Small functions in 0x80020000-0x80032FFF: each one resets a 0x10-byte float
// triple in .bss (x = f1, y = f0, z = f1) from float constants in .sdata2.
#include "types.h"

extern "C" {
extern u8 lbl_803C4C78[];           // 0x803C4C78 (vec3 state)
extern u8 lbl_803C4C88[];           // 0x803C4C88
extern u8 lbl_803C4C98[];           // 0x803C4C98
extern u8 lbl_803C4CA8[];           // 0x803C4CA8
extern u8 lbl_803C4CB8[];           // 0x803C4CB8
extern u8 lbl_803C4CC8[];           // 0x803C4CC8
extern u8 lbl_803C4CD8[];           // 0x803C4CD8
extern u8 lbl_803C4CE8[];           // 0x803C4CE8
extern u8 lbl_803C4CF8[];           // 0x803C4CF8
extern u8 lbl_803C4D08[];           // 0x803C4D08
extern u8 lbl_803C4D18[];           // 0x803C4D18
extern u8 lbl_803C4D28[];           // 0x803C4D28
extern u8 lbl_803C4D38[];           // 0x803C4D38
extern u8 lbl_803C4D54[];           // 0x803C4D54
extern u8 lbl_803C4D60[];           // 0x803C4D60
extern u8 lbl_803C4D70[];           // 0x803C4D70
extern u8 lbl_803C4D80[];           // 0x803C4D80
extern u8 lbl_803C4D90[];           // 0x803C4D90
extern u8 lbl_803C4DA0[];           // 0x803C4DA0
extern u8 lbl_803C4DB0[];           // 0x803C4DB0

extern f32 lbl_805FC8B8;  // 0x805FC8B8 (float constant)
extern f32 lbl_805FC8BC;  // 0x805FC8BC (float constant)
extern f32 lbl_805FC8C8;  // 0x805FC8C8 (float constant)
extern f32 lbl_805FC8CC;  // 0x805FC8CC (float constant)
extern f32 lbl_805FC8D8;  // 0x805FC8D8 (float constant)
extern f32 lbl_805FC8DC;  // 0x805FC8DC (float constant)
extern f32 lbl_805FC8E0;  // 0x805FC8E0 (float constant)
extern f32 lbl_805FC8F4;  // 0x805FC8F4 (float constant)
extern f32 lbl_805FC920;  // 0x805FC920 (float constant)
extern f32 lbl_805FC924;  // 0x805FC924 (float constant)
extern f32 lbl_805FC928;  // 0x805FC928 (float constant)
extern f32 lbl_805FC93C;  // 0x805FC93C (float constant)
extern f32 lbl_805FC990;  // 0x805FC990 (float constant)
extern f32 lbl_805FCA34;  // 0x805FCA34 (float constant)
extern f32 lbl_805FCA38;  // 0x805FCA38 (float constant)
extern f32 lbl_805FCA60;  // 0x805FCA60 (float constant)
extern f32 lbl_805FCAA8;  // 0x805FCAA8 (float constant)
extern f32 lbl_805FCADC;  // 0x805FCADC (float constant)
extern f32 lbl_805FCB18;  // 0x805FCB18 (float constant)
extern f32 lbl_805FCB24;  // 0x805FCB24 (float constant)
extern f32 lbl_805FCB34;  // 0x805FCB34 (float constant)
extern f32 lbl_805FCB38;  // 0x805FCB38 (float constant)
extern f32 lbl_805FCB40;  // 0x805FCB40 (float constant)
extern f32 lbl_805FCB7C;  // 0x805FCB7C (float constant)
extern f32 lbl_805FCB80;  // 0x805FCB80 (float constant)
extern f32 lbl_805FCBA0;  // 0x805FCBA0 (float constant)
extern f32 lbl_805FCBB0;  // 0x805FCBB0 (float constant)
extern f32 lbl_805FCBC0;  // 0x805FCBC0 (float constant)
extern f32 lbl_805FCC20;  // 0x805FCC20 (float constant)
extern f32 lbl_805FCC24;  // 0x805FCC24 (float constant)
extern f32 lbl_805FCCA0;  // 0x805FCCA0 (float constant)
extern f32 lbl_805FCCC0;  // 0x805FCCC0 (float constant)
extern f32 lbl_805FCCD4;  // 0x805FCCD4 (float constant)
extern f32 lbl_805FCCE4;  // 0x805FCCE4 (float constant)
extern f32 lbl_805FCD00;  // 0x805FCD00 (float constant)
extern f32 lbl_805FCD04;  // 0x805FCD04 (float constant)
extern f32 lbl_805FCD08;  // 0x805FCD08 (float constant)
extern f32 lbl_805FCD24;  // 0x805FCD24 (float constant)
extern f32 lbl_805FCD38;  // 0x805FCD38 (float constant)
extern f32 lbl_805FCD80;  // 0x805FCD80 (float constant)
}

extern "C" {

// 0x80020460
void fn_80020460(void) {
    u8* g = lbl_803C4C78;
    *(f32*)(g + 0x0) = lbl_805FC8BC;
    *(f32*)(g + 0x4) = lbl_805FC8B8;
    *(f32*)(g + 0x8) = lbl_805FC8BC;
}

// 0x800214B4
void fn_800214B4(void) {
    u8* g = lbl_803C4C88;
    *(f32*)(g + 0x0) = lbl_805FC8C8;
    *(f32*)(g + 0x4) = lbl_805FC8CC;
    *(f32*)(g + 0x8) = lbl_805FC8C8;
}

// 0x80021574
void fn_80021574(void) {
    u8* g = lbl_803C4C98;
    *(f32*)(g + 0x0) = lbl_805FC8D8;
    *(f32*)(g + 0x4) = lbl_805FC8DC;
    *(f32*)(g + 0x8) = lbl_805FC8D8;
}

// 0x80023284
void fn_80023284(void) {
    u8* g = lbl_803C4CA8;
    *(f32*)(g + 0x0) = lbl_805FC8E0;
    *(f32*)(g + 0x4) = lbl_805FC8F4;
    *(f32*)(g + 0x8) = lbl_805FC8E0;
}

// 0x80023864
void fn_80023864(void) {
    u8* g = lbl_803C4CB8;
    *(f32*)(g + 0x0) = lbl_805FC920;
    *(f32*)(g + 0x4) = lbl_805FC924;
    *(f32*)(g + 0x8) = lbl_805FC920;
}

// 0x800259B8
void fn_800259B8(void) {
    u8* g = lbl_803C4CC8;
    *(f32*)(g + 0x0) = lbl_805FC928;
    *(f32*)(g + 0x4) = lbl_805FC93C;
    *(f32*)(g + 0x8) = lbl_805FC928;
}

// 0x80028018
void fn_80028018(void) {
    u8* g = lbl_803C4CD8;
    *(f32*)(g + 0x0) = lbl_805FC990;
    *(f32*)(g + 0x4) = lbl_805FCA34;
    *(f32*)(g + 0x8) = lbl_805FC990;
}

// 0x800293A8
void fn_800293A8(void) {
    u8* g = lbl_803C4CE8;
    *(f32*)(g + 0x0) = lbl_805FCA38;
    *(f32*)(g + 0x4) = lbl_805FCA60;
    *(f32*)(g + 0x8) = lbl_805FCA38;
}

// 0x8002AAE0
void fn_8002AAE0(void) {
    u8* g = lbl_803C4CF8;
    *(f32*)(g + 0x0) = lbl_805FCAA8;
    *(f32*)(g + 0x4) = lbl_805FCADC;
    *(f32*)(g + 0x8) = lbl_805FCAA8;
}

// 0x8002B4B0
void fn_8002B4B0(void) {
    u8* g = lbl_803C4D08;
    *(f32*)(g + 0x0) = lbl_805FCB18;
    *(f32*)(g + 0x4) = lbl_805FCB24;
    *(f32*)(g + 0x8) = lbl_805FCB18;
}

// 0x8002B56C
void fn_8002B56C(void) {
    u8* g = lbl_803C4D18;
    *(f32*)(g + 0x0) = lbl_805FCB34;
    *(f32*)(g + 0x4) = lbl_805FCB38;
    *(f32*)(g + 0x8) = lbl_805FCB34;
}

// 0x8002BFBC
void fn_8002BFBC(void) {
    u8* g = lbl_803C4D28;
    *(f32*)(g + 0x0) = lbl_805FCB40;
    *(f32*)(g + 0x4) = lbl_805FCB7C;
    *(f32*)(g + 0x8) = lbl_805FCB40;
}

// 0x8002C61C
void fn_8002C61C(void) {
    u8* g = lbl_803C4D38;
    *(f32*)(g + 0x0) = lbl_805FCB80;
    *(f32*)(g + 0x4) = lbl_805FCBA0;
    *(f32*)(g + 0x8) = lbl_805FCB80;
}

// 0x8002DE7C
void fn_8002DE7C(void) {
    u8* g = lbl_803C4D54;
    *(f32*)(g + 0x0) = lbl_805FCBB0;
    *(f32*)(g + 0x4) = lbl_805FCBC0;
    *(f32*)(g + 0x8) = lbl_805FCBB0;
}

// 0x80030208
void fn_80030208(void) {
    u8* g = lbl_803C4D60;
    *(f32*)(g + 0x0) = lbl_805FCC20;
    *(f32*)(g + 0x4) = lbl_805FCC24;
    *(f32*)(g + 0x8) = lbl_805FCC20;
}

// 0x800305E4
void fn_800305E4(void) {
    u8* g = lbl_803C4D70;
    *(f32*)(g + 0x0) = lbl_805FCCA0;
    *(f32*)(g + 0x4) = lbl_805FCCC0;
    *(f32*)(g + 0x8) = lbl_805FCCA0;
}

// 0x8003105C
void fn_8003105C(void) {
    u8* g = lbl_803C4D80;
    *(f32*)(g + 0x0) = lbl_805FCCD4;
    *(f32*)(g + 0x4) = lbl_805FCCE4;
    *(f32*)(g + 0x8) = lbl_805FCCD4;
}

// 0x800314D8
void fn_800314D8(void) {
    u8* g = lbl_803C4D90;
    *(f32*)(g + 0x0) = lbl_805FCD00;
    *(f32*)(g + 0x4) = lbl_805FCD04;
    *(f32*)(g + 0x8) = lbl_805FCD00;
}

// 0x800321E8
void fn_800321E8(void) {
    u8* g = lbl_803C4DA0;
    *(f32*)(g + 0x0) = lbl_805FCD08;
    *(f32*)(g + 0x4) = lbl_805FCD24;
    *(f32*)(g + 0x8) = lbl_805FCD08;
}

// 0x80032EBC
void fn_80032EBC(void) {
    u8* g = lbl_803C4DB0;
    *(f32*)(g + 0x0) = lbl_805FCD38;
    *(f32*)(g + 0x4) = lbl_805FCD80;
    *(f32*)(g + 0x8) = lbl_805FCD38;
}

}  // extern "C"
