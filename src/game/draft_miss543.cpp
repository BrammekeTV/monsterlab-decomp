#include "types.h"

extern "C" {
void fn_80086F84(u8* self);                    // 0x80086F84
void fn_80087720(u8* p, void* arg1);           // 0x80087720
void fn_800704AC(u8* arg0, f32 dt);            // 0x800704AC
extern "C++" void fn_800C537C(u8* p);          // 0x800C537C (small_800C.cpp defines it outside extern "C")
void fn_800C5F44(u8* p);                       // 0x800C5F44
void fn_800CD014(void);                        // 0x800CD014
s32 fn_800AEFEC(u8* p, s32 arg1, s32 arg2);    // 0x800AEFEC
u32 fn_80070270(s32 flags);                    // 0x80070270
void UIScreen_PopScreen(u32 hash);
extern f32 lbl_805FDDD0;                       // 0x805FDDD0
}

extern "C" {

// 0x80087F34
// Calls fn_80087720 with both arguments, then fn_80086F84 on the first.
void fn_80087F34(u8* arg0, u8* arg1) {
    fn_80087720(arg0, arg1);
    fn_80086F84(arg0);
}

// 0x8009BFB0
void fn_8009BFB0(u8* arg0) {
    fn_800C537C(arg0);
    *(s32*)(arg0 + 0xE0) = 0;
}

// 0x8009E22C
void fn_8009E22C(u8* arg0) {
    fn_800C5F44(arg0);
    *(f32*)(arg0 + 0x298) = lbl_805FDDD0;
}

// 0x800AEF04
// Returns 1 when fn_800AEFEC(arg0 + 0x27C, ...) is not -1, else 0.
u32 fn_800AEF04(u8* arg0, s32 arg1, s32 arg2) {
    s32 t = fn_800AEFEC(arg0 + 0x27C, arg1, arg2);
    return ((u32)((-1 - t) | (t + 1))) >> 31;
}

// 0x800CF294
// Calls fn_800704AC with dt, then fn_800CD014. The +0xA4 word is loaded into r3 in the
// asm, but fn_800CD014 is a bare blr and ignores its argument, so it is not passed.
void fn_800CF294(u8* arg0, f32 farg0) {
    fn_800704AC(arg0, farg0);
    fn_800CD014();
}

// 0x801ACF90
// Component-wise scale of the three floats at +0x0, +0x10, +0x20 by arg1[0], arg1[1], arg1[2].
void fn_801ACF90(u8* arg0, u8* arg1) {
    *(f32*)(arg0 + 0x0) = *(f32*)(arg0 + 0x0) * *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x10) = *(f32*)(arg0 + 0x10) * *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x20) = *(f32*)(arg0 + 0x20) * *(f32*)(arg1 + 0x8);
}

// 0x80076390
void fn_80076390(void) {
    UIScreen_PopScreen(fn_80070270(0x8000));
}

// 0x80090648
// Returns the signed word at +0x7F4 converted to float.
f32 fn_80090648(u8* arg0) {
    return (f32)*(s32*)(arg0 + 0x7F4);
}

// 0x801A81E0
// If the word at +0x190 is non-zero, appends it to the array at arg1[+8] (index arg1[+4])
// and bumps the count. Returns 0.
s32 fn_801A81E0(u8* arg0, u8* arg1) {
    s32 val = *(s32*)(arg0 + 0x190);
    if (val != 0) {
        s32 idx = *(s32*)(arg1 + 0x4);
        *(s32*)(*(u8**)(arg1 + 0x8) + idx * 4) = val;
        *(s32*)(arg1 + 0x4) = idx + 1;
    }
    return 0;
}

}  // extern "C"
