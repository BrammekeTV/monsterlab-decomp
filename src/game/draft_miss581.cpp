// Tiny functions 0x800CC0E8-0x800DF550: each one writes a default 3-float vector into a
// 0x10-byte global (x = f1 constant, y = f0 constant, z = f1 constant). The float constants
// are read from .sdata2 exactly as the asm loads them.
#include "types.h"

extern "C" {
extern f32 lbl_805FE4B8;  // 0x805FE4B8 (float constant)
extern f32 lbl_805FE4BC;  // 0x805FE4BC (float constant)
extern f32 lbl_805FE510;  // 0x805FE510 (float constant)
extern f32 lbl_805FE514;  // 0x805FE514 (float constant)
extern f32 lbl_805FE5B0;  // 0x805FE5B0 (float constant)
extern f32 lbl_805FE5B4;  // 0x805FE5B4 (float constant)
extern f32 lbl_805FE608;  // 0x805FE608 (float constant)
extern f32 lbl_805FE60C;  // 0x805FE60C (float constant)
extern f32 lbl_805FE610;  // 0x805FE610 (float constant)
extern f32 lbl_805FE614;  // 0x805FE614 (float constant)
extern f32 lbl_805FE618;  // 0x805FE618 (float constant)
extern f32 lbl_805FE638;  // 0x805FE638 (float constant)
extern f32 lbl_805FE640;  // 0x805FE640 (float constant)
extern f32 lbl_805FE654;  // 0x805FE654 (float constant)
extern f32 lbl_805FE658;  // 0x805FE658 (float constant)
extern f32 lbl_805FE65C;  // 0x805FE65C (float constant)
extern f32 lbl_805FE668;  // 0x805FE668 (float constant)
extern f32 lbl_805FE66C;  // 0x805FE66C (float constant)
extern f32 lbl_805FE698;  // 0x805FE698 (float constant)
extern f32 lbl_805FE69C;  // 0x805FE69C (float constant)
extern f32 lbl_805FE6F0;  // 0x805FE6F0 (float constant)
extern f32 lbl_805FE6F4;  // 0x805FE6F4 (float constant)
extern f32 lbl_805FE708;  // 0x805FE708 (float constant)
extern f32 lbl_805FE70C;  // 0x805FE70C (float constant)
extern f32 lbl_805FE798;  // 0x805FE798 (float constant)
extern f32 lbl_805FE79C;  // 0x805FE79C (float constant)
extern f32 lbl_805FE7B0;  // 0x805FE7B0 (float constant)
extern f32 lbl_805FE7C4;  // 0x805FE7C4 (float constant)
extern f32 lbl_805FE7D0;  // 0x805FE7D0 (float constant)
extern f32 lbl_805FE7E0;  // 0x805FE7E0 (float constant)
extern f32 lbl_805FE7F8;  // 0x805FE7F8 (float constant)
extern f32 lbl_805FE808;  // 0x805FE808 (float constant)

extern f32 lbl_803C6EB8[];  // 0x803C6EB8 (3-float vector)
extern f32 lbl_803C6EC8[];  // 0x803C6EC8 (3-float vector)
extern f32 lbl_803C6F30[];  // 0x803C6F30 (3-float vector)
extern f32 lbl_803C6F60[];  // 0x803C6F60 (3-float vector)
extern f32 lbl_803C6F70[];  // 0x803C6F70 (3-float vector)
extern f32 lbl_803C6F80[];  // 0x803C6F80 (3-float vector)
extern f32 lbl_803C6F90[];  // 0x803C6F90 (3-float vector)
extern f32 lbl_803C6FA0[];  // 0x803C6FA0 (3-float vector)
extern f32 lbl_803C6FB0[];  // 0x803C6FB0 (3-float vector)
extern f32 lbl_803C6FC0[];  // 0x803C6FC0 (3-float vector)
extern f32 lbl_803C6FD0[];  // 0x803C6FD0 (3-float vector)
extern f32 lbl_803C6FE0[];  // 0x803C6FE0 (3-float vector)
extern f32 lbl_803C6FF0[];  // 0x803C6FF0 (3-float vector)
extern f32 lbl_803C7000[];  // 0x803C7000 (3-float vector)
extern f32 lbl_803C7010[];  // 0x803C7010 (3-float vector)
extern f32 lbl_803C7020[];  // 0x803C7020 (3-float vector)
}

extern "C" {

// 0x800CC0E8
// Resets the default vector lbl_803C6EB8.
void fn_800CC0E8(void) {
    lbl_803C6EB8[0] = lbl_805FE4B8;
    lbl_803C6EB8[1] = lbl_805FE4BC;
    lbl_803C6EB8[2] = lbl_805FE4B8;
}

// 0x800CD41C
// Resets the default vector lbl_803C6EC8.
void fn_800CD41C(void) {
    lbl_803C6EC8[0] = lbl_805FE510;
    lbl_803C6EC8[1] = lbl_805FE514;
    lbl_803C6EC8[2] = lbl_805FE510;
}

// 0x800D02A0
// Resets the default vector lbl_803C6F30.
void fn_800D02A0(void) {
    lbl_803C6F30[0] = lbl_805FE5B0;
    lbl_803C6F30[1] = lbl_805FE5B4;
    lbl_803C6F30[2] = lbl_805FE5B0;
}

// 0x800D4C44
// Resets the default vector lbl_803C6F60.
void fn_800D4C44(void) {
    lbl_803C6F60[0] = lbl_805FE608;
    lbl_803C6F60[1] = lbl_805FE60C;
    lbl_803C6F60[2] = lbl_805FE608;
}

// 0x800D50A4
// Resets the default vector lbl_803C6F70.
void fn_800D50A4(void) {
    lbl_803C6F70[0] = lbl_805FE610;
    lbl_803C6F70[1] = lbl_805FE614;
    lbl_803C6F70[2] = lbl_805FE610;
}

// 0x800D6AEC
// Resets the default vector lbl_803C6F80.
void fn_800D6AEC(void) {
    lbl_803C6F80[0] = lbl_805FE618;
    lbl_803C6F80[1] = lbl_805FE638;
    lbl_803C6F80[2] = lbl_805FE618;
}

// 0x800D7F04
// Resets the default vector lbl_803C6F90.
void fn_800D7F04(void) {
    lbl_803C6F90[0] = lbl_805FE640;
    lbl_803C6F90[1] = lbl_805FE654;
    lbl_803C6F90[2] = lbl_805FE640;
}

// 0x800D885C
// Resets the default vector lbl_803C6FA0.
void fn_800D885C(void) {
    lbl_803C6FA0[0] = lbl_805FE65C;
    lbl_803C6FA0[1] = lbl_805FE658;
    lbl_803C6FA0[2] = lbl_805FE65C;
}

// 0x800D9188
// Resets the default vector lbl_803C6FB0.
void fn_800D9188(void) {
    lbl_803C6FB0[0] = lbl_805FE668;
    lbl_803C6FB0[1] = lbl_805FE66C;
    lbl_803C6FB0[2] = lbl_805FE668;
}

// 0x800DB350
// Resets the default vector lbl_803C6FC0.
void fn_800DB350(void) {
    lbl_803C6FC0[0] = lbl_805FE69C;
    lbl_803C6FC0[1] = lbl_805FE698;
    lbl_803C6FC0[2] = lbl_805FE69C;
}

// 0x800DB984
// Resets the default vector lbl_803C6FD0.
void fn_800DB984(void) {
    lbl_803C6FD0[0] = lbl_805FE6F0;
    lbl_803C6FD0[1] = lbl_805FE6F4;
    lbl_803C6FD0[2] = lbl_805FE6F0;
}

// 0x800DE594
// Resets the default vector lbl_803C6FE0.
void fn_800DE594(void) {
    lbl_803C6FE0[0] = lbl_805FE708;
    lbl_803C6FE0[1] = lbl_805FE70C;
    lbl_803C6FE0[2] = lbl_805FE708;
}

// 0x800DEC08
// Resets the default vector lbl_803C6FF0.
void fn_800DEC08(void) {
    lbl_803C6FF0[0] = lbl_805FE798;
    lbl_803C6FF0[1] = lbl_805FE79C;
    lbl_803C6FF0[2] = lbl_805FE798;
}

// 0x800DEF98
// Resets the default vector lbl_803C7000.
void fn_800DEF98(void) {
    lbl_803C7000[0] = lbl_805FE7B0;
    lbl_803C7000[1] = lbl_805FE7C4;
    lbl_803C7000[2] = lbl_805FE7B0;
}

// 0x800DF1E4
// Resets the default vector lbl_803C7010.
void fn_800DF1E4(void) {
    lbl_803C7010[0] = lbl_805FE7D0;
    lbl_803C7010[1] = lbl_805FE7E0;
    lbl_803C7010[2] = lbl_805FE7D0;
}

// 0x800DF550
// Resets the default vector lbl_803C7020.
void fn_800DF550(void) {
    lbl_803C7020[0] = lbl_805FE7F8;
    lbl_803C7020[1] = lbl_805FE808;
    lbl_803C7020[2] = lbl_805FE7F8;
}

}  // extern "C"
