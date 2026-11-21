// Small functions (batch miss582): 3-float record writers. Each stores float x (f1) at +0 and +8
// and float y (f0) at +4 into a global 3-float record.
#include "types.h"

extern "C" {
extern u8 lbl_803C7030[];  // 0x803C7030 (3-float record)
extern u8 lbl_803C7040[];  // 0x803C7040 (3-float record)
extern u8 lbl_803C7050[];  // 0x803C7050 (3-float record)
extern u8 lbl_803C7060[];  // 0x803C7060 (3-float record)
extern u8 lbl_803C7070[];  // 0x803C7070 (3-float record)
extern u8 lbl_803C7080[];  // 0x803C7080 (3-float record)
extern u8 lbl_803C7090[];  // 0x803C7090 (3-float record)
extern u8 lbl_803C70A0[];  // 0x803C70A0 (3-float record)
extern u8 lbl_803C70B0[];  // 0x803C70B0 (3-float record)
extern u8 lbl_803C70C0[];  // 0x803C70C0 (3-float record)
extern u8 lbl_803C70D0[];  // 0x803C70D0 (3-float record)
extern u8 lbl_803C70E0[];  // 0x803C70E0 (3-float record)
extern u8 lbl_803C70F0[];  // 0x803C70F0 (3-float record)
extern u8 lbl_803C7100[];  // 0x803C7100 (3-float record)
extern u8 lbl_803C7230[];  // 0x803C7230 (3-float record)
extern u8 lbl_803C7240[];  // 0x803C7240 (3-float record)
extern u8 lbl_803C7250[];  // 0x803C7250 (3-float record)
extern u8 lbl_803C7260[];  // 0x803C7260 (3-float record)
extern u8 lbl_803C7370[];  // 0x803C7370 (3-float record)
extern u8 lbl_803C75B8[];  // 0x803C75B8 (3-float record)

extern f32 lbl_805FE824;  // 0x805FE824 (float constant)
extern f32 lbl_805FE828;  // 0x805FE828 (float constant)
extern f32 lbl_805FE830;  // 0x805FE830 (float constant)
extern f32 lbl_805FE854;  // 0x805FE854 (float constant)
extern f32 lbl_805FE890;  // 0x805FE890 (float constant)
extern f32 lbl_805FE894;  // 0x805FE894 (float constant)
extern f32 lbl_805FE8B0;  // 0x805FE8B0 (float constant)
extern f32 lbl_805FE8B8;  // 0x805FE8B8 (float constant)
extern f32 lbl_805FE970;  // 0x805FE970 (float constant)
extern f32 lbl_805FE974;  // 0x805FE974 (float constant)
extern f32 lbl_805FE978;  // 0x805FE978 (float constant)
extern f32 lbl_805FE988;  // 0x805FE988 (float constant)
extern f32 lbl_805FE9A0;  // 0x805FE9A0 (float constant)
extern f32 lbl_805FE9CC;  // 0x805FE9CC (float constant)
extern f32 lbl_805FE9D0;  // 0x805FE9D0 (float constant)
extern f32 lbl_805FE9D4;  // 0x805FE9D4 (float constant)
extern f32 lbl_805FE9D8;  // 0x805FE9D8 (float constant)
extern f32 lbl_805FE9DC;  // 0x805FE9DC (float constant)
extern f32 lbl_805FE9E0;  // 0x805FE9E0 (float constant)
extern f32 lbl_805FEA1C;  // 0x805FEA1C (float constant)
extern f32 lbl_805FEA28;  // 0x805FEA28 (float constant)
extern f32 lbl_805FEA2C;  // 0x805FEA2C (float constant)
extern f32 lbl_805FEA50;  // 0x805FEA50 (float constant)
extern f32 lbl_805FEA54;  // 0x805FEA54 (float constant)
extern f32 lbl_805FEA88;  // 0x805FEA88 (float constant)
extern f32 lbl_805FEA8C;  // 0x805FEA8C (float constant)
extern f32 lbl_805FEAF0;  // 0x805FEAF0 (float constant)
extern f32 lbl_805FEAF4;  // 0x805FEAF4 (float constant)
extern f32 lbl_805FEB04;  // 0x805FEB04 (float constant)
extern f32 lbl_805FEB0C;  // 0x805FEB0C (float constant)
extern f32 lbl_805FEBB8;  // 0x805FEBB8 (float constant)
extern f32 lbl_805FEBBC;  // 0x805FEBBC (float constant)
extern f32 lbl_805FEBC0;  // 0x805FEBC0 (float constant)
extern f32 lbl_805FEBC4;  // 0x805FEBC4 (float constant)
extern f32 lbl_805FEBC8;  // 0x805FEBC8 (float constant)
extern f32 lbl_805FEBCC;  // 0x805FEBCC (float constant)
extern f32 lbl_805FEBD0;  // 0x805FEBD0 (float constant)
extern f32 lbl_805FEBD4;  // 0x805FEBD4 (float constant)
extern f32 lbl_805FEBF0;  // 0x805FEBF0 (float constant)
extern f32 lbl_805FEBF4;  // 0x805FEBF4 (float constant)
}

extern "C" {

// 0x800DF610
void fn_800DF610(void) {
    *(f32*)(lbl_803C7030 + 0x0) = lbl_805FE824;
    *(f32*)(lbl_803C7030 + 0x4) = lbl_805FE828;
    *(f32*)(lbl_803C7030 + 0x8) = lbl_805FE824;
}

// 0x800DFF84
void fn_800DFF84(void) {
    *(f32*)(lbl_803C7040 + 0x0) = lbl_805FE830;
    *(f32*)(lbl_803C7040 + 0x4) = lbl_805FE854;
    *(f32*)(lbl_803C7040 + 0x8) = lbl_805FE830;
}

// 0x800E0288
void fn_800E0288(void) {
    *(f32*)(lbl_803C7050 + 0x0) = lbl_805FE890;
    *(f32*)(lbl_803C7050 + 0x4) = lbl_805FE894;
    *(f32*)(lbl_803C7050 + 0x8) = lbl_805FE890;
}

// 0x800E2E9C
void fn_800E2E9C(void) {
    *(f32*)(lbl_803C7060 + 0x0) = lbl_805FE8B0;
    *(f32*)(lbl_803C7060 + 0x4) = lbl_805FE8B8;
    *(f32*)(lbl_803C7060 + 0x8) = lbl_805FE8B0;
}

// 0x800E2FC8
void fn_800E2FC8(void) {
    *(f32*)(lbl_803C7070 + 0x0) = lbl_805FE970;
    *(f32*)(lbl_803C7070 + 0x4) = lbl_805FE974;
    *(f32*)(lbl_803C7070 + 0x8) = lbl_805FE970;
}

// 0x800E32E0
void fn_800E32E0(void) {
    *(f32*)(lbl_803C7080 + 0x0) = lbl_805FE978;
    *(f32*)(lbl_803C7080 + 0x4) = lbl_805FE988;
    *(f32*)(lbl_803C7080 + 0x8) = lbl_805FE978;
}

// 0x800E372C
void fn_800E372C(void) {
    *(f32*)(lbl_803C7090 + 0x0) = lbl_805FE9A0;
    *(f32*)(lbl_803C7090 + 0x4) = lbl_805FE9CC;
    *(f32*)(lbl_803C7090 + 0x8) = lbl_805FE9A0;
}

// 0x800E3814
void fn_800E3814(void) {
    *(f32*)(lbl_803C70A0 + 0x0) = lbl_805FE9D0;
    *(f32*)(lbl_803C70A0 + 0x4) = lbl_805FE9D4;
    *(f32*)(lbl_803C70A0 + 0x8) = lbl_805FE9D0;
}

// 0x800E38FC
void fn_800E38FC(void) {
    *(f32*)(lbl_803C70B0 + 0x0) = lbl_805FE9D8;
    *(f32*)(lbl_803C70B0 + 0x4) = lbl_805FE9DC;
    *(f32*)(lbl_803C70B0 + 0x8) = lbl_805FE9D8;
}

// 0x800E3CD4
void fn_800E3CD4(void) {
    *(f32*)(lbl_803C70C0 + 0x0) = lbl_805FE9E0;
    *(f32*)(lbl_803C70C0 + 0x4) = lbl_805FEA1C;
    *(f32*)(lbl_803C70C0 + 0x8) = lbl_805FE9E0;
}

// 0x800E42C8
void fn_800E42C8(void) {
    *(f32*)(lbl_803C70D0 + 0x0) = lbl_805FEA28;
    *(f32*)(lbl_803C70D0 + 0x4) = lbl_805FEA2C;
    *(f32*)(lbl_803C70D0 + 0x8) = lbl_805FEA28;
}

// 0x800E5530
void fn_800E5530(void) {
    *(f32*)(lbl_803C70E0 + 0x0) = lbl_805FEA54;
    *(f32*)(lbl_803C70E0 + 0x4) = lbl_805FEA50;
    *(f32*)(lbl_803C70E0 + 0x8) = lbl_805FEA54;
}

// 0x800E6208
void fn_800E6208(void) {
    *(f32*)(lbl_803C70F0 + 0x0) = lbl_805FEA88;
    *(f32*)(lbl_803C70F0 + 0x4) = lbl_805FEA8C;
    *(f32*)(lbl_803C70F0 + 0x8) = lbl_805FEA88;
}

// 0x800E6340
void fn_800E6340(void) {
    *(f32*)(lbl_803C7100 + 0x0) = lbl_805FEAF0;
    *(f32*)(lbl_803C7100 + 0x4) = lbl_805FEAF4;
    *(f32*)(lbl_803C7100 + 0x8) = lbl_805FEAF0;
}

// 0x800FB938
void fn_800FB938(void) {
    *(f32*)(lbl_803C7230 + 0x0) = lbl_805FEB0C;
    *(f32*)(lbl_803C7230 + 0x4) = lbl_805FEB04;
    *(f32*)(lbl_803C7230 + 0x8) = lbl_805FEB0C;
}

// 0x80106864
void fn_80106864(void) {
    *(f32*)(lbl_803C7240 + 0x0) = lbl_805FEBB8;
    *(f32*)(lbl_803C7240 + 0x4) = lbl_805FEBBC;
    *(f32*)(lbl_803C7240 + 0x8) = lbl_805FEBB8;
}

// 0x80106888
void fn_80106888(void) {
    *(f32*)(lbl_803C7250 + 0x0) = lbl_805FEBC0;
    *(f32*)(lbl_803C7250 + 0x4) = lbl_805FEBC4;
    *(f32*)(lbl_803C7250 + 0x8) = lbl_805FEBC0;
}

// 0x80106A88
void fn_80106A88(void) {
    *(f32*)(lbl_803C7260 + 0x0) = lbl_805FEBC8;
    *(f32*)(lbl_803C7260 + 0x4) = lbl_805FEBCC;
    *(f32*)(lbl_803C7260 + 0x8) = lbl_805FEBC8;
}

// 0x801070FC
void fn_801070FC(void) {
    *(f32*)(lbl_803C7370 + 0x0) = lbl_805FEBD0;
    *(f32*)(lbl_803C7370 + 0x4) = lbl_805FEBD4;
    *(f32*)(lbl_803C7370 + 0x8) = lbl_805FEBD0;
}

// 0x801091CC
void fn_801091CC(void) {
    *(f32*)(lbl_803C75B8 + 0x0) = lbl_805FEBF0;
    *(f32*)(lbl_803C75B8 + 0x4) = lbl_805FEBF4;
    *(f32*)(lbl_803C75B8 + 0x8) = lbl_805FEBF0;
}

}  // extern "C"
