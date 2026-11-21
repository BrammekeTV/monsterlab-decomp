#include "types.h"

extern "C" {
u8* fn_800C52F0(u8* p, s32 del);
void operator_delete(void* p);

// Float constants (sdata2).
extern f32 lbl_805FCD88;
extern f32 lbl_805FCD8C;
extern f32 lbl_805FCDBC;
extern f32 lbl_805FCE14;
extern f32 lbl_805FCEBC;
extern f32 lbl_805FCEC0;
extern f32 lbl_805FCF00;
extern f32 lbl_805FCF5C;
extern f32 lbl_805FCFAC;
extern f32 lbl_805FCFEC;
extern f32 lbl_805FD070;
extern f32 lbl_805FD074;
extern f32 lbl_805FD098;
extern f32 lbl_805FD0B4;
extern f32 lbl_805FD0D8;
extern f32 lbl_805FD0DC;
extern f32 lbl_805FD0F0;
extern f32 lbl_805FD0F4;
extern f32 lbl_805FD0F8;
extern f32 lbl_805FD108;
extern f32 lbl_805FD110;
extern f32 lbl_805FD114;
extern f32 lbl_805FD118;
extern f32 lbl_805FD11C;
extern f32 lbl_805FD140;
extern f32 lbl_805FD144;
extern f32 lbl_805FD148;
extern f32 lbl_805FD14C;
extern f32 lbl_805FD150;
extern f32 lbl_805FD160;

// Float vectors (bss).
extern f32 lbl_803C4DCC[3];
extern f32 lbl_803C4DD8[4];
extern f32 lbl_803C4DE8[4];
extern f32 lbl_803C4DF8[4];
extern f32 lbl_803C4E08[4];
extern f32 lbl_803C4E18[4];
extern f32 lbl_803C4EE0[4];
extern f32 lbl_803C4EF0[4];
extern f32 lbl_803C4F00[4];
extern f32 lbl_803C4F10[4];
extern f32 lbl_803C4F20[4];
extern f32 lbl_803C4F30[4];
extern f32 lbl_803C4F40[4];
extern f32 lbl_803C4F50[4];
extern f32 lbl_803C51B8[4];

// 0x80034064
// Sets the vector at lbl_803C4DCC to (c0, c1, c0).
void fn_80034064(void) {
    lbl_803C4DCC[0] = lbl_805FCD88;
    lbl_803C4DCC[1] = lbl_805FCD8C;
    lbl_803C4DCC[2] = lbl_805FCD88;
}

// 0x800367AC
// Sets the vector at lbl_803C4DD8 to (c0, c1, c0).
void fn_800367AC(void) {
    lbl_803C4DD8[0] = lbl_805FCDBC;
    lbl_803C4DD8[1] = lbl_805FCE14;
    lbl_803C4DD8[2] = lbl_805FCDBC;
}

// 0x80038558
// Sets the vector at lbl_803C4DE8 to (c0, c1, c0).
void fn_80038558(void) {
    lbl_803C4DE8[0] = lbl_805FCEBC;
    lbl_803C4DE8[1] = lbl_805FCEC0;
    lbl_803C4DE8[2] = lbl_805FCEBC;
}

// 0x80039CD8
// Sets the vector at lbl_803C4DF8 to (c0, c1, c0).
void fn_80039CD8(void) {
    lbl_803C4DF8[0] = lbl_805FCF00;
    lbl_803C4DF8[1] = lbl_805FCF5C;
    lbl_803C4DF8[2] = lbl_805FCF00;
}

// 0x8003CA58
// Sets the vector at lbl_803C4E08 to (c0, c1, c0).
void fn_8003CA58(void) {
    lbl_803C4E08[0] = lbl_805FCFAC;
    lbl_803C4E08[1] = lbl_805FCFEC;
    lbl_803C4E08[2] = lbl_805FCFAC;
}

// 0x8003CF20
// Sets the vector at lbl_803C4E18 to (c0, c1, c0).
void fn_8003CF20(void) {
    lbl_803C4E18[0] = lbl_805FD070;
    lbl_803C4E18[1] = lbl_805FD074;
    lbl_803C4E18[2] = lbl_805FD070;
}

// 0x8003F8FC
// Sets the vector at lbl_803C4EE0 to (c0, c1, c0).
void fn_8003F8FC(void) {
    lbl_803C4EE0[0] = lbl_805FD098;
    lbl_803C4EE0[1] = lbl_805FD0B4;
    lbl_803C4EE0[2] = lbl_805FD098;
}

// 0x8003FB04
// Sets the vector at lbl_803C4EF0 to (c0, c1, c0).
void fn_8003FB04(void) {
    lbl_803C4EF0[0] = lbl_805FD0D8;
    lbl_803C4EF0[1] = lbl_805FD0DC;
    lbl_803C4EF0[2] = lbl_805FD0D8;
}

// 0x80040070
// Sets the vector at lbl_803C4F00 to (c0, c1, c0).
void fn_80040070(void) {
    lbl_803C4F00[0] = lbl_805FD0F0;
    lbl_803C4F00[1] = lbl_805FD0F4;
    lbl_803C4F00[2] = lbl_805FD0F0;
}

// 0x80040618
// Sets the vector at lbl_803C4F10 to (c0, c1, c0).
void fn_80040618(void) {
    lbl_803C4F10[0] = lbl_805FD0F8;
    lbl_803C4F10[1] = lbl_805FD108;
    lbl_803C4F10[2] = lbl_805FD0F8;
}

// 0x80040804
// Sets the vector at lbl_803C4F20 to (c0, c1, c0).
void fn_80040804(void) {
    lbl_803C4F20[0] = lbl_805FD110;
    lbl_803C4F20[1] = lbl_805FD114;
    lbl_803C4F20[2] = lbl_805FD110;
}

// 0x80041854
// Sets the vector at lbl_803C4F30 to (c0, c1, c0).
void fn_80041854(void) {
    lbl_803C4F30[0] = lbl_805FD118;
    lbl_803C4F30[1] = lbl_805FD11C;
    lbl_803C4F30[2] = lbl_805FD118;
}

// 0x800418C0
// Sets the vector at lbl_803C4F40 to (c0, c1, c0).
void fn_800418C0(void) {
    lbl_803C4F40[0] = lbl_805FD140;
    lbl_803C4F40[1] = lbl_805FD144;
    lbl_803C4F40[2] = lbl_805FD140;
}

// 0x80041A94
// Sets the vector at lbl_803C4F50 to (c0, c1, c0).
void fn_80041A94(void) {
    lbl_803C4F50[0] = lbl_805FD148;
    lbl_803C4F50[1] = lbl_805FD14C;
    lbl_803C4F50[2] = lbl_805FD148;
}

// 0x80042BF8
// Sets the vector at lbl_803C51B8 to (c0, c1, c0).
void fn_80042BF8(void) {
    lbl_803C51B8[0] = lbl_805FD150;
    lbl_803C51B8[1] = lbl_805FD160;
    lbl_803C51B8[2] = lbl_805FD150;
}

}  // extern "C"
