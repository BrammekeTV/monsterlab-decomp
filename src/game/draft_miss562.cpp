// Small functions (batch miss562): float-pair/vector constant writers and tiny getters/setters.
#include "types.h"

extern "C" {
extern u8 lbl_803C51C8[];  // 0x803C51C8 (3-float record)
extern u8 lbl_803C51F0[];  // 0x803C51F0 (3-float record)
extern u8 lbl_803C5200[];  // 0x803C5200 (3-float record)
extern u8 lbl_803C5210[];  // 0x803C5210 (3-float record)
extern u8 lbl_803C5220[];  // 0x803C5220 (3-float record)
extern u8 lbl_803C5230[];  // 0x803C5230 (3-float record)
extern u8 lbl_803C5328[];  // 0x803C5328 (3-float record)
extern u8 lbl_803C5338[];  // 0x803C5338 (3-float record)
extern u8 lbl_803C5348[];  // 0x803C5348 (3-float record)
extern u8 lbl_803C5398[];  // 0x803C5398 (3-float record)
extern u8 lbl_803C53A8[];  // 0x803C53A8 (3-float record)
extern u8 lbl_803C53B8[];  // 0x803C53B8 (3-float record)
extern u8 lbl_803C53C8[];  // 0x803C53C8 (3-float record)
extern u8 lbl_803C53D8[];  // 0x803C53D8 (3-float record)
extern u8 lbl_803C5468[];  // 0x803C5468 (3-float record)
extern u8 lbl_803C5478[];  // 0x803C5478 (3-float record)
extern u8 lbl_803C5488[];  // 0x803C5488 (3-float record)
extern u8 lbl_803C54E0[];  // 0x803C54E0 (3-float record)

extern f32 lbl_805FD180;  // 0x805FD180
extern f32 lbl_805FD184;  // 0x805FD184
extern f32 lbl_805FD188;  // 0x805FD188
extern f32 lbl_805FD18C;  // 0x805FD18C
extern f32 lbl_805FD194;  // 0x805FD194
extern f32 lbl_805FD1A4;  // 0x805FD1A4
extern f32 lbl_805FD1A8;  // 0x805FD1A8
extern f32 lbl_805FD1AC;  // 0x805FD1AC
extern f32 lbl_805FD1B0;  // 0x805FD1B0
extern f32 lbl_805FD1B4;  // 0x805FD1B4
extern f32 lbl_805FD1B8;  // 0x805FD1B8
extern f32 lbl_805FD1BC;  // 0x805FD1BC
extern f32 lbl_805FD1F0;  // 0x805FD1F0
extern f32 lbl_805FD1F4;  // 0x805FD1F4
extern f32 lbl_805FD1F8;  // 0x805FD1F8
extern f32 lbl_805FD1FC;  // 0x805FD1FC
extern f32 lbl_805FD200;  // 0x805FD200
extern f32 lbl_805FD204;  // 0x805FD204
extern f32 lbl_805FD228;  // 0x805FD228
extern f32 lbl_805FD234;  // 0x805FD234
extern f32 lbl_805FD238;  // 0x805FD238
extern f32 lbl_805FD23C;  // 0x805FD23C
extern f32 lbl_805FD240;  // 0x805FD240
extern f32 lbl_805FD244;  // 0x805FD244
extern f32 lbl_805FD270;  // 0x805FD270
extern f32 lbl_805FD274;  // 0x805FD274
extern f32 lbl_805FD2B8;  // 0x805FD2B8
extern f32 lbl_805FD2DC;  // 0x805FD2DC
extern f32 lbl_805FD2E8;  // 0x805FD2E8
extern f32 lbl_805FD2EC;  // 0x805FD2EC
extern f32 lbl_805FD2F0;  // 0x805FD2F0
extern f32 lbl_805FD2F4;  // 0x805FD2F4
extern f32 lbl_805FD2F8;  // 0x805FD2F8
extern f32 lbl_805FD2FC;  // 0x805FD2FC
extern f32 lbl_805FD3E8;  // 0x805FD3E8
extern f32 lbl_805FD3EC;  // 0x805FD3EC
}

extern "C" {

// 0x80042C18
// Sets the record {a, b, a} (floats at +0, +4, +8) from two constants.
void fn_80042C18(void) {
    f32 a = lbl_805FD180;
    f32 b = lbl_805FD184;
    *(f32*)(lbl_803C51C8 + 0x0) = a;
    *(f32*)(lbl_803C51C8 + 0x4) = b;
    *(f32*)(lbl_803C51C8 + 0x8) = a;
}

// 0x800441FC
// Sets the record {a, b, a} from two constants.
void fn_800441FC(void) {
    f32 a = lbl_805FD188;
    f32 b = lbl_805FD18C;
    *(f32*)(lbl_803C51F0 + 0x0) = a;
    *(f32*)(lbl_803C51F0 + 0x4) = b;
    *(f32*)(lbl_803C51F0 + 0x8) = a;
}

// 0x800444DC
// Sets the record {a, b, a} from two constants.
void fn_800444DC(void) {
    f32 a = lbl_805FD194;
    f32 b = lbl_805FD1A4;
    *(f32*)(lbl_803C5200 + 0x0) = a;
    *(f32*)(lbl_803C5200 + 0x4) = b;
    *(f32*)(lbl_803C5200 + 0x8) = a;
}

// 0x800445C4
// Sets the record {a, b, a} from two constants.
void fn_800445C4(void) {
    f32 a = lbl_805FD1A8;
    f32 b = lbl_805FD1AC;
    *(f32*)(lbl_803C5210 + 0x0) = a;
    *(f32*)(lbl_803C5210 + 0x4) = b;
    *(f32*)(lbl_803C5210 + 0x8) = a;
}

// 0x80044C38
// Sets the record {a, b, a} from two constants.
void fn_80044C38(void) {
    f32 a = lbl_805FD1B0;
    f32 b = lbl_805FD1B4;
    *(f32*)(lbl_803C5220 + 0x0) = a;
    *(f32*)(lbl_803C5220 + 0x4) = b;
    *(f32*)(lbl_803C5220 + 0x8) = a;
}

// 0x80044CBC
// Sets the record {a, b, a} from two constants.
void fn_80044CBC(void) {
    f32 a = lbl_805FD1B8;
    f32 b = lbl_805FD1BC;
    *(f32*)(lbl_803C5230 + 0x0) = a;
    *(f32*)(lbl_803C5230 + 0x4) = b;
    *(f32*)(lbl_803C5230 + 0x8) = a;
}

// 0x800496BC
// Sets the record {a, b, a} from two constants.
void fn_800496BC(void) {
    f32 a = lbl_805FD1F0;
    f32 b = lbl_805FD1F4;
    *(f32*)(lbl_803C5328 + 0x0) = a;
    *(f32*)(lbl_803C5328 + 0x4) = b;
    *(f32*)(lbl_803C5328 + 0x8) = a;
}

// 0x800497D4
// Sets the record {a, b, a} from two constants.
void fn_800497D4(void) {
    f32 a = lbl_805FD1F8;
    f32 b = lbl_805FD1FC;
    *(f32*)(lbl_803C5338 + 0x0) = a;
    *(f32*)(lbl_803C5338 + 0x4) = b;
    *(f32*)(lbl_803C5338 + 0x8) = a;
}

// 0x8004AC48
// Sets the record {a, b, a} from two constants.
void fn_8004AC48(void) {
    f32 a = lbl_805FD200;
    f32 b = lbl_805FD204;
    *(f32*)(lbl_803C5348 + 0x0) = a;
    *(f32*)(lbl_803C5348 + 0x4) = b;
    *(f32*)(lbl_803C5348 + 0x8) = a;
}

// 0x8004B478
// Sets the record {a, b, a} from two constants.
void fn_8004B478(void) {
    f32 a = lbl_805FD228;
    f32 b = lbl_805FD234;
    *(f32*)(lbl_803C5398 + 0x0) = a;
    *(f32*)(lbl_803C5398 + 0x4) = b;
    *(f32*)(lbl_803C5398 + 0x8) = a;
}

// 0x8004B58C
// Clamps v to the limit stored at +0x5F4 of p (the asm keeps the limit when v >= limit,
// and also when v is NaN) and stores it at +0x24 of the sub-record at +4.
void fn_8004B58C(u8* p, f32 v) {
    f32 cap = *(f32*)(p + 0x5f4);
    if (!(v < cap)) v = cap;
    *(f32*)(*(u8**)(p + 0x4) + 0x24) = v;
}

// 0x8004B5AC
// Returns 1 when the float at +0x24 of the sub-record at +4 is <= lbl_805FD238, else 0.
s32 fn_8004B5AC(u8* p) {
    f32 v = *(f32*)(*(u8**)(p + 0x4) + 0x24);
    return v <= lbl_805FD238 ? 1 : 0;
}

// 0x8004B5CC
// Sets the record {a, b, a} from two constants.
void fn_8004B5CC(void) {
    f32 a = lbl_805FD238;
    f32 b = lbl_805FD23C;
    *(f32*)(lbl_803C53A8 + 0x0) = a;
    *(f32*)(lbl_803C53A8 + 0x4) = b;
    *(f32*)(lbl_803C53A8 + 0x8) = a;
}

// 0x8004E56C
// Sets the record {a, b, a} from two constants.
void fn_8004E56C(void) {
    f32 a = lbl_805FD240;
    f32 b = lbl_805FD244;
    *(f32*)(lbl_803C53B8 + 0x0) = a;
    *(f32*)(lbl_803C53B8 + 0x4) = b;
    *(f32*)(lbl_803C53B8 + 0x8) = a;
}

// 0x800505C0
// Sets the record {a, b, a} from two constants.
void fn_800505C0(void) {
    f32 a = lbl_805FD270;
    f32 b = lbl_805FD274;
    *(f32*)(lbl_803C53C8 + 0x0) = a;
    *(f32*)(lbl_803C53C8 + 0x4) = b;
    *(f32*)(lbl_803C53C8 + 0x8) = a;
}

// 0x80051ECC
// Sets the record {a, b, a} from two constants.
void fn_80051ECC(void) {
    f32 a = lbl_805FD2B8;
    f32 b = lbl_805FD2DC;
    *(f32*)(lbl_803C53D8 + 0x0) = a;
    *(f32*)(lbl_803C53D8 + 0x4) = b;
    *(f32*)(lbl_803C53D8 + 0x8) = a;
}

// 0x800522EC
// Sets the record {a, b, a} from two constants.
void fn_800522EC(void) {
    f32 a = lbl_805FD2E8;
    f32 b = lbl_805FD2EC;
    *(f32*)(lbl_803C5468 + 0x0) = a;
    *(f32*)(lbl_803C5468 + 0x4) = b;
    *(f32*)(lbl_803C5468 + 0x8) = a;
}

// 0x80052398
// Sets the record {a, b, a} from two constants.
void fn_80052398(void) {
    f32 a = lbl_805FD2F0;
    f32 b = lbl_805FD2F4;
    *(f32*)(lbl_803C5478 + 0x0) = a;
    *(f32*)(lbl_803C5478 + 0x4) = b;
    *(f32*)(lbl_803C5478 + 0x8) = a;
}

// 0x800524E4
// Sets the record {a, b, a} from two constants.
void fn_800524E4(void) {
    f32 a = lbl_805FD2F8;
    f32 b = lbl_805FD2FC;
    *(f32*)(lbl_803C5488 + 0x0) = a;
    *(f32*)(lbl_803C5488 + 0x4) = b;
    *(f32*)(lbl_803C5488 + 0x8) = a;
}

// 0x8005CBA4
// Sets the record {a, b, a} from two constants.
void fn_8005CBA4(void) {
    f32 a = lbl_805FD3E8;
    f32 b = lbl_805FD3EC;
    *(f32*)(lbl_803C54E0 + 0x0) = a;
    *(f32*)(lbl_803C54E0 + 0x4) = b;
    *(f32*)(lbl_803C54E0 + 0x8) = a;
}

}  // extern "C"
