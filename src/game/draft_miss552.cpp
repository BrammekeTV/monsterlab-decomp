// Tiny functions from the 0x80006xxx, 0x8000Cxxx, 0x80014xxx, 0x800163xx, 0x80018xxx,
// 0x80019xxx, 0x8001Axxx, 0x8001Bxxx, 0x8001Exxx, 0x8001Fxxx and 0x801A2xxx/0x801A4xxx ranges.
#include "types.h"

extern "C" {
void fn_8018D4CC(void* p);                                      // 0x8018D4CC (free)
s32  fn_801A3798(u8* arg0, f32 farg0);                          // 0x801A3798
}

// Vec3 globals written by the functions below (x = 0, y = 1, z = 0).
extern "C" {
extern f32 lbl_803B5380[3];   // 0x803B5380
extern f32 lbl_803C4268[3];   // 0x803C4268
extern f32 lbl_803C4278[3];   // 0x803C4278
extern f32 lbl_803C4B88[3];   // 0x803C4B88
extern f32 lbl_803C4B98[3];   // 0x803C4B98
extern f32 lbl_803C4BA8[3];   // 0x803C4BA8
extern f32 lbl_803C4BB8[3];   // 0x803C4BB8
extern f32 lbl_803C4BC8[3];   // 0x803C4BC8
extern f32 lbl_803C4BD8[3];   // 0x803C4BD8
extern f32 lbl_803C4BE8[3];   // 0x803C4BE8
extern f32 lbl_803C4BF8[3];   // 0x803C4BF8
extern f32 lbl_803C4C08[3];   // 0x803C4C08
extern f32 lbl_803C4C18[3];   // 0x803C4C18
extern f32 lbl_803C4C28[3];   // 0x803C4C28
extern f32 lbl_803C4C38[3];   // 0x803C4C38
extern f32 lbl_803C4C48[3];   // 0x803C4C48
extern f32 lbl_803C4C58[3];   // 0x803C4C58
extern f32 lbl_803C4C68[3];   // 0x803C4C68
}

extern "C" {

// 0x801A2E60
// Frees the pointer passed in (r3) and returns 0.
s32 fn_801A2E60(u8* arg0) {
    fn_8018D4CC(arg0);
    return 0;
}

// 0x801A46B4
// Calls fn_801A3798 with r3/f1 never set in this function (stale), so both are passed as 0.
s32 fn_801A46B4(void) {
    fn_801A3798((u8*)0, 0.0f);
    return 0;
}

// 0x80006B40
// Resets the Vec3 global at 0x803B5380 to (0, 1, 0).
void fn_80006B40(void) {
    lbl_803B5380[0] = 0.0f;
    lbl_803B5380[1] = 1.0f;
    lbl_803B5380[2] = 0.0f;
}

// 0x8000C7A8
// Resets the Vec3 global at 0x803C4268 to (0, 1, 0).
void fn_8000C7A8(void) {
    lbl_803C4268[0] = 0.0f;
    lbl_803C4268[1] = 1.0f;
    lbl_803C4268[2] = 0.0f;
}

// 0x8000CF6C
// Resets the Vec3 global at 0x803C4278 to (0, 1, 0).
void fn_8000CF6C(void) {
    lbl_803C4278[0] = 0.0f;
    lbl_803C4278[1] = 1.0f;
    lbl_803C4278[2] = 0.0f;
}

// 0x80014210
// Resets the Vec3 global at 0x803C4B88 to (0, 1, 0).
void fn_80014210(void) {
    lbl_803C4B88[0] = 0.0f;
    lbl_803C4B88[1] = 1.0f;
    lbl_803C4B88[2] = 0.0f;
}

// 0x8001423C
// Resets the Vec3 global at 0x803C4B98 to (0, 1, 0).
void fn_8001423C(void) {
    lbl_803C4B98[0] = 0.0f;
    lbl_803C4B98[1] = 1.0f;
    lbl_803C4B98[2] = 0.0f;
}

// 0x80014308
// Resets the Vec3 global at 0x803C4BA8 to (0, 1, 0).
void fn_80014308(void) {
    lbl_803C4BA8[0] = 0.0f;
    lbl_803C4BA8[1] = 1.0f;
    lbl_803C4BA8[2] = 0.0f;
}

// 0x80014330
// Resets the Vec3 global at 0x803C4BB8 to (0, 1, 0).
void fn_80014330(void) {
    lbl_803C4BB8[0] = 0.0f;
    lbl_803C4BB8[1] = 1.0f;
    lbl_803C4BB8[2] = 0.0f;
}

// 0x800163E0
// Resets the Vec3 global at 0x803C4BC8 to (0, 1, 0).
void fn_800163E0(void) {
    lbl_803C4BC8[0] = 0.0f;
    lbl_803C4BC8[1] = 1.0f;
    lbl_803C4BC8[2] = 0.0f;
}

// 0x80018BE4
// Resets the Vec3 global at 0x803C4BD8 to (0, 1, 0).
void fn_80018BE4(void) {
    lbl_803C4BD8[0] = 0.0f;
    lbl_803C4BD8[1] = 1.0f;
    lbl_803C4BD8[2] = 0.0f;
}

// 0x80018DE0
// Resets the Vec3 global at 0x803C4BE8 to (0, 1, 0).
void fn_80018DE0(void) {
    lbl_803C4BE8[0] = 0.0f;
    lbl_803C4BE8[1] = 1.0f;
    lbl_803C4BE8[2] = 0.0f;
}

// 0x80019788
// Resets the Vec3 global at 0x803C4BF8 to (0, 1, 0).
void fn_80019788(void) {
    lbl_803C4BF8[0] = 0.0f;
    lbl_803C4BF8[1] = 1.0f;
    lbl_803C4BF8[2] = 0.0f;
}

// 0x80019CA8
// Resets the Vec3 global at 0x803C4C08 to (0, 1, 0).
void fn_80019CA8(void) {
    lbl_803C4C08[0] = 0.0f;
    lbl_803C4C08[1] = 1.0f;
    lbl_803C4C08[2] = 0.0f;
}

// 0x80019F90
// Resets the Vec3 global at 0x803C4C18 to (0, 1, 0).
void fn_80019F90(void) {
    lbl_803C4C18[0] = 0.0f;
    lbl_803C4C18[1] = 1.0f;
    lbl_803C4C18[2] = 0.0f;
}

// 0x8001A15C
// Resets the Vec3 global at 0x803C4C28 to (0, 1, 0).
void fn_8001A15C(void) {
    lbl_803C4C28[0] = 0.0f;
    lbl_803C4C28[1] = 1.0f;
    lbl_803C4C28[2] = 0.0f;
}

// 0x8001AF94
// Resets the Vec3 global at 0x803C4C38 to (0, 1, 0).
void fn_8001AF94(void) {
    lbl_803C4C38[0] = 0.0f;
    lbl_803C4C38[1] = 1.0f;
    lbl_803C4C38[2] = 0.0f;
}

// 0x8001B1EC
// Resets the Vec3 global at 0x803C4C48 to (0, 1, 0).
void fn_8001B1EC(void) {
    lbl_803C4C48[0] = 0.0f;
    lbl_803C4C48[1] = 1.0f;
    lbl_803C4C48[2] = 0.0f;
}

// 0x8001EE28
// Resets the Vec3 global at 0x803C4C58 to (0, 1, 0).
void fn_8001EE28(void) {
    lbl_803C4C58[0] = 0.0f;
    lbl_803C4C58[1] = 1.0f;
    lbl_803C4C58[2] = 0.0f;
}

// 0x8001FC88
// Resets the Vec3 global at 0x803C4C68 to (0, 1, 0).
void fn_8001FC88(void) {
    lbl_803C4C68[0] = 0.0f;
    lbl_803C4C68[1] = 1.0f;
    lbl_803C4C68[2] = 0.0f;
}

}  // extern "C"
