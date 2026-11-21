// Small functions: each stores a float triple (A, B, A) into a 0x10-byte global, with the
// values taken from float constants in .sdata2 (the asm loads two constants per function).
#include "types.h"

extern "C" {

extern f32 lbl_805FD450;  // float constant (triple element A)
extern f32 lbl_805FD454;  // float constant (triple element B)
extern f32 lbl_805FD458;
extern f32 lbl_805FD460;
extern f32 lbl_805FD468;  // float constant (declared in small_8006.cpp)
extern f32 lbl_805FD484;
extern f32 lbl_805FD4E8;  // float constant (0.0, declared in draft_miss163.cpp)
extern f32 lbl_805FD508;  // float constant (1.0, declared in draft_miss163.cpp)
extern f32 lbl_805FD568;  // float constant (declared in small_8006c.cpp)
extern f32 lbl_805FD56C;
extern f32 lbl_805FD570;
extern f32 lbl_805FD58C;
extern f32 lbl_805FD598;  // float constant (0.0f, declared in draft_miss48.cpp)
extern f32 lbl_805FD5A8;  // float constant (1.0f, declared in draft_miss48.cpp)
extern f32 lbl_805FD5B8;
extern f32 lbl_805FD5BC;
extern f32 lbl_805FD5C0;
extern f32 lbl_805FD5C4;
extern f32 lbl_805FD5C8;
extern f32 lbl_805FD5CC;
extern f32 lbl_805FD5D0;
extern f32 lbl_805FD5D4;
extern f32 lbl_805FD5D8;  // float constant (declared in draft_8007b.cpp)
extern f32 lbl_805FD5F0;  // float constant (declared in draft_miss502.cpp)
extern f32 lbl_805FD638;  // float constant (music volume, declared in small_8007.cpp)
extern f32 lbl_805FD63C;
extern f32 lbl_805FD640;
extern f32 lbl_805FD644;
extern f32 lbl_805FD668;  // float constant (declared in small_8007b.cpp)
extern f32 lbl_805FD66C;
extern f32 lbl_805FD670;
extern f32 lbl_805FD674;
extern f32 lbl_805FD680;  // float constant (declared in small_8007b.cpp)
extern f32 lbl_805FD684;
extern f32 lbl_805FD6C0;
extern f32 lbl_805FD6D8;
extern f32 lbl_805FD6E8;
extern f32 lbl_805FD6EC;
extern f32 lbl_805FD6F0;  // float constant (declared in small_8007c.cpp)
extern f32 lbl_805FD6F4;  // float constant (declared in small_8007c.cpp)

extern f32 lbl_803C5690[];  // 0x803C5690 (4 floats)
extern f32 lbl_803C56A0[];  // 0x803C56A0
extern f32 lbl_803C56B0[];  // 0x803C56B0
extern f32 lbl_803C56C0[];  // 0x803C56C0
extern f32 lbl_803C56D0[];  // 0x803C56D0
extern f32 lbl_803C56E0[];  // 0x803C56E0
extern f32 lbl_803C56F0[];  // 0x803C56F0
extern f32 lbl_803C5700[];  // 0x803C5700
extern f32 lbl_803C5710[];  // 0x803C5710
extern f32 lbl_803C5720[];  // 0x803C5720
extern f32 lbl_803C5730[];  // 0x803C5730
extern f32 lbl_803C5D80[];  // 0x803C5D80
extern f32 lbl_803C5D90[];  // 0x803C5D90
extern f32 lbl_803C5DA0[];  // 0x803C5DA0
extern f32 lbl_803C68D0[];  // 0x803C68D0
extern f32 lbl_803C68E0[];  // 0x803C68E0
extern f32 lbl_803C68F0[];  // 0x803C68F0
extern f32 lbl_803C6920[];  // 0x803C6920
extern f32 lbl_803C6930[];  // 0x803C6930
extern f32 lbl_803C6940[];  // 0x803C6940

// 0x8006305C
void fn_8006305C(void) {
    lbl_803C5690[0] = lbl_805FD450;
    lbl_803C5690[1] = lbl_805FD454;
    lbl_803C5690[2] = lbl_805FD450;
}

// 0x80063680
void fn_80063680(void) {
    lbl_803C56A0[0] = lbl_805FD458;
    lbl_803C56A0[1] = lbl_805FD460;
    lbl_803C56A0[2] = lbl_805FD458;
}

// 0x80069008
void fn_80069008(void) {
    lbl_803C56B0[0] = lbl_805FD468;
    lbl_803C56B0[1] = lbl_805FD484;
    lbl_803C56B0[2] = lbl_805FD468;
}

// 0x8006CCA4
void fn_8006CCA4(void) {
    lbl_803C56C0[0] = lbl_805FD4E8;
    lbl_803C56C0[1] = lbl_805FD508;
    lbl_803C56C0[2] = lbl_805FD4E8;
}

// 0x8006CF84
void fn_8006CF84(void) {
    lbl_803C56D0[0] = lbl_805FD568;
    lbl_803C56D0[1] = lbl_805FD56C;
    lbl_803C56D0[2] = lbl_805FD568;
}

// 0x8006D2B0
void fn_8006D2B0(void) {
    lbl_803C56E0[0] = lbl_805FD570;
    lbl_803C56E0[1] = lbl_805FD58C;
    lbl_803C56E0[2] = lbl_805FD570;
}

// 0x8006F580
void fn_8006F580(void) {
    lbl_803C56F0[0] = lbl_805FD598;
    lbl_803C56F0[1] = lbl_805FD5A8;
    lbl_803C56F0[2] = lbl_805FD598;
}

// 0x8006F5FC
void fn_8006F5FC(void) {
    lbl_803C5700[0] = lbl_805FD5B8;
    lbl_803C5700[1] = lbl_805FD5BC;
    lbl_803C5700[2] = lbl_805FD5B8;
}

// 0x8006F69C
void fn_8006F69C(void) {
    lbl_803C5710[0] = lbl_805FD5C0;
    lbl_803C5710[1] = lbl_805FD5C4;
    lbl_803C5710[2] = lbl_805FD5C0;
}

// 0x8006F774
void fn_8006F774(void) {
    lbl_803C5720[0] = lbl_805FD5C8;
    lbl_803C5720[1] = lbl_805FD5CC;
    lbl_803C5720[2] = lbl_805FD5C8;
}

// 0x8006FA24
void fn_8006FA24(void) {
    lbl_803C5730[0] = lbl_805FD5D0;
    lbl_803C5730[1] = lbl_805FD5D4;
    lbl_803C5730[2] = lbl_805FD5D0;
}

// 0x800747BC
void fn_800747BC(void) {
    lbl_803C5D80[0] = lbl_805FD5D8;
    lbl_803C5D80[1] = lbl_805FD5F0;
    lbl_803C5D80[2] = lbl_805FD5D8;
}

// 0x80074CA8
void fn_80074CA8(void) {
    lbl_803C5D90[0] = lbl_805FD63C;
    lbl_803C5D90[1] = lbl_805FD638;
    lbl_803C5D90[2] = lbl_805FD63C;
}

// 0x80075414
void fn_80075414(void) {
    lbl_803C5DA0[0] = lbl_805FD640;
    lbl_803C5DA0[1] = lbl_805FD644;
    lbl_803C5DA0[2] = lbl_805FD640;
}

// 0x80076D18
void fn_80076D18(void) {
    lbl_803C68D0[0] = lbl_805FD668;
    lbl_803C68D0[1] = lbl_805FD66C;
    lbl_803C68D0[2] = lbl_805FD668;
}

// 0x80077B30
void fn_80077B30(void) {
    lbl_803C68E0[0] = lbl_805FD670;
    lbl_803C68E0[1] = lbl_805FD674;
    lbl_803C68E0[2] = lbl_805FD670;
}

// 0x80077CFC
void fn_80077CFC(void) {
    lbl_803C68F0[0] = lbl_805FD684;
    lbl_803C68F0[1] = lbl_805FD680;
    lbl_803C68F0[2] = lbl_805FD684;
}

// 0x8007A660
void fn_8007A660(void) {
    lbl_803C6920[0] = lbl_805FD6C0;
    lbl_803C6920[1] = lbl_805FD6D8;
    lbl_803C6920[2] = lbl_805FD6C0;
}

// 0x8007AFA4
void fn_8007AFA4(void) {
    lbl_803C6930[0] = lbl_805FD6E8;
    lbl_803C6930[1] = lbl_805FD6EC;
    lbl_803C6930[2] = lbl_805FD6E8;
}

// 0x8007C48C
void fn_8007C48C(void) {
    lbl_803C6940[0] = lbl_805FD6F0;
    lbl_803C6940[1] = lbl_805FD6F4;
    lbl_803C6940[2] = lbl_805FD6F0;
}

}  // extern "C"
