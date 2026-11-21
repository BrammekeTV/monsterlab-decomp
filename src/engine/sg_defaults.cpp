// Default values of the scene graph's material / render-state record (0xD20 bytes).
// Original: 0x8017A618 (auto_03_8016E304_text.s)
//
// Layout as far as this function shows it (names are descriptions, not original names):
//   0x004  two 0x48-byte blocks of pass settings (0x04, 0x4C... see the code)
//   0x0A4  8 x 0x24: {u32 x4 = 1, u32 0, u8 0, u32 0, u8 0, u32 index}
//   0x1C4  8 x 0x34: texture-coordinate slots {u8 0, u16 1, u16 1, u32 index, u8[4] 0, u32 0,
//                    u32 index + 5, u32 0, u32 0, u32 3, u32 5, u32 0x3C, u8 0, u32 0x3D}
//   0x364  4 x 0x14: {u32 enabled (first two only), f32[4] = 1.0}: colours
//   0x3F0  4 x {u32 x4}: (0,1,2,3) (0,0,0,3) (1,1,1,3) (2,2,2,3): the four swap tables
//   0x430  16 x 0x68: TEV stages
//   0xAB0  16 x u32 0
//   0xAF0  16 x 0x20
//   0xCF0  the tail: alpha/depth/blend settings
#include "types.h"

#define W8(off)  (*(u8*)(p + (off)))
#define W16(off) (*(u16*)(p + (off)))
#define W32(off) (*(u32*)(p + (off)))
#define WF(off)  (*(f32*)(p + (off)))

// 0x8017A618
extern "C" void fn_8017A618(void* rec) {
    u8* p = (u8*)rec;
    s32 i;

    // Two identical halves at 0x04 and 0x4C, apart from the first three words.
    W32(0x04) = 0;
    W32(0x08) = 1;
    W32(0x0C) = 1;
    W32(0x10) = 0;
    for (i = 0; i < 2; i++) {
        u8* q = p + i * 0x48;
        *(u8*)(q + 0x14) = 1;
        *(u32*)(q + 0x18) = 0;
        *(u8*)(q + 0x1C) = 1;
        *(u32*)(q + 0x20) = 0xFFFFFFFF;
        *(u8*)(q + 0x24) = 0;
        *(u8*)(q + 0x25) = 0xF;
        *(u32*)(q + 0x28) = 0;
        *(u32*)(q + 0x2C) = 1;
        *(u32*)(q + 0x30) = 0;
        *(u32*)(q + 0x34) = 2;
        *(u8*)(q + 0x40) = 0;
        *(u8*)(q + 0x41) = 0xF;
        *(u32*)(q + 0x44) = 0;
        *(u32*)(q + 0x48) = 1;
        *(u32*)(q + 0x4C) = 0;
        *(u32*)(q + 0x50) = 2;
    }

    for (i = 0; i < 8; i++) {
        u8* q = p + 0xA4 + i * 0x24;
        *(u32*)(q + 0x00) = 1;
        *(u32*)(q + 0x04) = 1;
        *(u32*)(q + 0x08) = 1;
        *(u32*)(q + 0x0C) = 1;
        *(u32*)(q + 0x10) = 0;
        *(u8*)(q + 0x14) = 0;
        *(u32*)(q + 0x18) = 0;
        *(u8*)(q + 0x1C) = 0;
        *(u32*)(q + 0x20) = (u32)i;
    }

    for (i = 0; i < 8; i++) {
        u8* q = p + 0x1C4 + i * 0x34;
        *(u8*)(q + 0x00) = 0;
        *(u16*)(q + 0x02) = 1;
        *(u16*)(q + 0x04) = 1;
        *(u32*)(q + 0x08) = (u32)i;
        *(u8*)(q + 0x0C) = 0;
        *(u8*)(q + 0x0D) = 0;
        *(u8*)(q + 0x0E) = 0;
        *(u8*)(q + 0x0F) = 0;
        *(u32*)(q + 0x10) = 0;
        *(u32*)(q + 0x14) = (u32)(i + 5);
        *(u32*)(q + 0x18) = 0;
        *(u32*)(q + 0x1C) = 0;
        *(u32*)(q + 0x20) = 3;
        *(u32*)(q + 0x24) = 5;
        *(u32*)(q + 0x28) = 0x3C;
        *(u8*)(q + 0x2C) = 0;
        *(u32*)(q + 0x30) = 0x3D;
    }

    // Four colours, all white; the first two are switched on.
    for (i = 0; i < 4; i++) {
        u8* q = p + 0x364 + i * 0x14;
        *(u32*)(q + 0x00) = (i < 2) ? 1 : 0;
        *(f32*)(q + 0x04) = 1.0f;
        *(f32*)(q + 0x08) = 1.0f;
        *(f32*)(q + 0x0C) = 1.0f;
        *(f32*)(q + 0x10) = 1.0f;
    }

    W32(0x3F0) = 0; W32(0x3F4) = 1; W32(0x3F8) = 2; W32(0x3FC) = 3;
    W32(0x400) = 0; W32(0x404) = 0; W32(0x408) = 0; W32(0x40C) = 3;
    W32(0x410) = 1; W32(0x414) = 1; W32(0x418) = 1; W32(0x41C) = 3;
    W32(0x420) = 2; W32(0x424) = 2; W32(0x428) = 2; W32(0x42C) = 3;

    // 16 stages. The first eight use texture coordinate / map i; stage 0 passes its inputs
    // through, the others combine with the previous stage.
    for (i = 0; i < 16; i++) {
        u8* q = p + 0x430 + i * 0x68;
        if (i < 8) {
            *(u8*)(q + 0x00) = 1;
            *(u32*)(q + 0x04) = (u32)i;
            *(u32*)(q + 0x08) = (u32)i;
            *(u32*)(q + 0x0C) = 0;
        } else {
            *(u8*)(q + 0x00) = 0;
            *(u32*)(q + 0x04) = 0;
            *(u32*)(q + 0x08) = 0;
            *(u32*)(q + 0x0C) = 7;
        }
        if (i <= 0) {
            *(u32*)(q + 0x28) = 0xF;
            *(u32*)(q + 0x2C) = 0xF;
            *(u32*)(q + 0x30) = 0xF;
            *(u32*)(q + 0x34) = 8;
            *(u32*)(q + 0x4C) = 7;
            *(u32*)(q + 0x50) = 7;
            *(u32*)(q + 0x54) = 7;
            *(u32*)(q + 0x58) = 4;
        } else {
            *(u32*)(q + 0x28) = 0;
            *(u32*)(q + 0x2C) = 0xC;
            *(u32*)(q + 0x30) = 8;
            *(u32*)(q + 0x34) = 0xF;
            *(u32*)(q + 0x4C) = 7;
            *(u32*)(q + 0x50) = 4;
            *(u32*)(q + 0x54) = 0;
            *(u32*)(q + 0x58) = 7;
        }
        *(u32*)(q + 0x10) = 0;
        *(u32*)(q + 0x14) = 6;
        *(u32*)(q + 0x18) = 0;
        *(u32*)(q + 0x1C) = 0;
        *(u32*)(q + 0x20) = 0;
        *(u32*)(q + 0x24) = 0;
        *(u8*)(q + 0x38) = 1;
        *(u32*)(q + 0x3C) = 0;
        *(u32*)(q + 0x40) = 0;
        *(u32*)(q + 0x44) = 0;
        *(u32*)(q + 0x48) = 0;
        *(u8*)(q + 0x5C) = 1;
        *(u32*)(q + 0x60) = 0;
        *(u32*)(q + 0x64) = 0;
    }

    for (i = 0; i < 16; i++) {
        W32(0xAB0 + i * 4) = 0;
    }

    for (i = 0; i < 16; i++) {
        u8* q = p + 0xAF0 + i * 0x20;
        *(u32*)(q + 0x00) = 0;
        *(u32*)(q + 0x04) = 0;
        *(u32*)(q + 0x08) = 0;
        *(u32*)(q + 0x0C) = 0;
        *(u32*)(q + 0x10) = 0;
        *(u32*)(q + 0x14) = 0;
        *(u8*)(q + 0x18) = 0;
        *(u8*)(q + 0x19) = 0;
        *(u32*)(q + 0x1C) = 0;
    }

    WF(0xCF0) = 1.0f;
    W32(0xCF4) = 7;
    W32(0xCF8) = 0;
    W8(0xCFC) = 1;
    W8(0xCFD) = 0;
    W32(0xD00) = 0;
    W32(0xD04) = 0;
    W32(0xD08) = 4;
    W32(0xD0C) = 5;
    W8(0xD10) = 1;
    W8(0xD11) = 1;
    W8(0xD12) = 1;
    W32(0xD14) = 3;
    W32(0xD18) = 2;
    W8(0xD1C) = 0;
}
