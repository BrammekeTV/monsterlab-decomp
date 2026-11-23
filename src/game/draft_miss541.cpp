#include "types.h"

extern "C" u8* fn_800C52F0(u8* p, s32 del);                    // 0x800C52F0 (C++ linkage in small_800C.cpp)

extern "C" {
void fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8); // 0x8006FA44
void operator_delete(void* p);                                   // 0x801D426C

extern u8  lbl_803C69E8[];   // 0x803C69E8 (parameter block for fn_8006FA44)
extern f32 lbl_803C69FC[];   // 0x803C69FC (three floats, written before the tail call)
extern f32 lbl_805FD928;     // 0x805FD928 (float constant)
extern f32 lbl_805FD934;     // 0x805FD934 (float constant)
extern u8  lbl_803C6C78[];   // 0x803C6C78 (parameter block for fn_8006FA44)
extern f32 lbl_803C6C8C[];   // 0x803C6C8C (three floats, written before the tail call)
extern f32 lbl_805FDF18;     // 0x805FDF18 (float constant)
extern f32 lbl_805FDF1C;     // 0x805FDF1C (float constant)
extern u8  lbl_803C6D98[];   // 0x803C6D98 (parameter block for fn_8006FA44)
extern f32 lbl_803C6DAC[];   // 0x803C6DAC (three floats, written before the tail call)
extern f32 lbl_805FE230;     // 0x805FE230 (float constant)
extern f32 lbl_805FE238;     // 0x805FE238 (float constant)
}

extern "C" {

// 0x801A8478
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801A8478(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x801A84B8
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801A84B8(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x801A9C14
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801A9C14(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x801AA1D8
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801AA1D8(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x801ACB3C
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801ACB3C(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x801BBEE4
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801BBEE4(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x801BC5B0
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801BC5B0(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x801BC7A8
// Free the block if it exists and the delete flag asks for it; returns the pointer.
u8* fn_801BC7A8(u8* arg0, s32 arg1) {
    if (arg0 != 0 && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x8008B3E4
// Stores three float constants into lbl_803C69FC, then tail-calls fn_8006FA44 with its parameter block.
void fn_8008B3E4(void) {
    f32 c0 = lbl_805FD928;
    f32 c1 = lbl_805FD934;
    lbl_803C69FC[0] = c0;
    lbl_803C69FC[1] = c1;
    lbl_803C69FC[2] = c0;
    fn_8006FA44(lbl_803C69E8, 0x282573C0, 3, (s32)0xB7AEEBD1u, (s32)0x85AB5092u, (s32)0x82C91C02u);
}

// 0x800AAEA0
// Stores three float constants into lbl_803C6C8C, then tail-calls fn_8006FA44 with its parameter block.
void fn_800AAEA0(void) {
    f32 c0 = lbl_805FDF18;
    f32 c1 = lbl_805FDF1C;
    lbl_803C6C8C[0] = c0;
    lbl_803C6C8C[1] = c1;
    lbl_803C6C8C[2] = c0;
    fn_8006FA44(lbl_803C6C78, (s32)0xDB2D58C6u, 0x4003, (s32)0x053BCBA4u, (s32)0x85AB5092u, 0);
}

// 0x800BC950
// Stores three float constants into lbl_803C6DAC, then tail-calls fn_8006FA44 with its parameter block.
void fn_800BC950(void) {
    f32 c0 = lbl_805FE230;
    f32 c1 = lbl_805FE238;
    lbl_803C6DAC[0] = c0;
    lbl_803C6DAC[1] = c1;
    lbl_803C6DAC[2] = c0;
    fn_8006FA44(lbl_803C6D98, (s32)0x28B1DA29u, 0x4003, (s32)0x8BD3CCCEu, (s32)0x85AB5092u, (s32)0x05F60DC3u);
}

}  // extern "C"
