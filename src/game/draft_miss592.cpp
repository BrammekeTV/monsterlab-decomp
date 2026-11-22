// Small functions: vector constant writers (globals at 0x803E6BE0..0x803F22B8) and one list append.
#include "types.h"

extern "C" {
extern u8 lbl_803E6BE0[];  // 0x803E6BE0 (vec3 global)
extern u8 lbl_803E6BF0[];  // 0x803E6BF0 (vec3 global)
extern u8 lbl_803E6C00[];  // 0x803E6C00 (vec3 global)
extern u8 lbl_803E6C10[];  // 0x803E6C10 (vec3 global)
extern u8 lbl_803E6CA0[];  // 0x803E6CA0 (vec3 global)
extern u8 lbl_803E6CB0[];  // 0x803E6CB0 (vec3 global)
extern u8 lbl_803E6CC0[];  // 0x803E6CC0 (vec3 global)
extern u8 lbl_803E6CD0[];  // 0x803E6CD0 (vec3 global)
extern u8 lbl_803E9FA8[];  // 0x803E9FA8 (vec3 global)
extern u8 lbl_803E9FB8[];  // 0x803E9FB8 (vec3 global)
extern u8 lbl_803EA0C0[];  // 0x803EA0C0 (vec3 global)
extern u8 lbl_803F0D00[];  // 0x803F0D00 (vec3 global)
extern u8 lbl_803F2078[];  // 0x803F2078 (vec3 global)
extern u8 lbl_803F2220[];  // 0x803F2220 (vec3 global)
extern u8 lbl_803F2240[];  // 0x803F2240 (vec3 global)
extern u8 lbl_803F22A8[];  // 0x803F22A8 (vec3 global)
extern u8 lbl_803F22B8[];  // 0x803F22B8 (vec3 global)
}

extern "C" {

// 0x801210C4
// Resets the global vec3 to (0, 1, 0).
void fn_801210C4(void) {
    *(f32*)(lbl_803E6BE0 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6BE0 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6BE0 + 0x8) = 0.0f;
}

// 0x8012146C
// Resets the global vec3 to (0, 1, 0).
void fn_8012146C(void) {
    *(f32*)(lbl_803E6BF0 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6BF0 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6BF0 + 0x8) = 0.0f;
}

// 0x801214E8
// Resets the global vec3 to (0, 1, 0).
void fn_801214E8(void) {
    *(f32*)(lbl_803E6C00 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6C00 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6C00 + 0x8) = 0.0f;
}

// 0x80121C70
// Resets the global vec3 to (0, 1, 0).
void fn_80121C70(void) {
    *(f32*)(lbl_803E6C10 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6C10 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6C10 + 0x8) = 0.0f;
}

// 0x80122B3C
// Resets the global vec3 to (0, 1, 0).
void fn_80122B3C(void) {
    *(f32*)(lbl_803E6CA0 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6CA0 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6CA0 + 0x8) = 0.0f;
}

// 0x801230BC
// Resets the global vec3 to (0, 1, 0).
void fn_801230BC(void) {
    *(f32*)(lbl_803E6CB0 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6CB0 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6CB0 + 0x8) = 0.0f;
}

// 0x80124398
// Resets the global vec3 to (0, 1, 0).
void fn_80124398(void) {
    *(f32*)(lbl_803E6CC0 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6CC0 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6CC0 + 0x8) = 0.0f;
}

// 0x801256EC
// Resets the global vec3 to (0, 1, 0).
void fn_801256EC(void) {
    *(f32*)(lbl_803E6CD0 + 0x0) = 0.0f;
    *(f32*)(lbl_803E6CD0 + 0x4) = 1.0f;
    *(f32*)(lbl_803E6CD0 + 0x8) = 0.0f;
}

// 0x8013020C
// Resets the global vec3 to (0, 1, 0).
void fn_8013020C(void) {
    *(f32*)(lbl_803E9FA8 + 0x0) = 0.0f;
    *(f32*)(lbl_803E9FA8 + 0x4) = 1.0f;
    *(f32*)(lbl_803E9FA8 + 0x8) = 0.0f;
}

// 0x80133A64
// Resets the global vec3 to (0, 1, 0).
void fn_80133A64(void) {
    *(f32*)(lbl_803E9FB8 + 0x0) = 0.0f;
    *(f32*)(lbl_803E9FB8 + 0x4) = 1.0f;
    *(f32*)(lbl_803E9FB8 + 0x8) = 0.0f;
}

// 0x80138A68
// Resets the global vec3 to (0, 1, 0).
void fn_80138A68(void) {
    *(f32*)(lbl_803EA0C0 + 0x0) = 0.0f;
    *(f32*)(lbl_803EA0C0 + 0x4) = 1.0f;
    *(f32*)(lbl_803EA0C0 + 0x8) = 0.0f;
}

// 0x8013A01C
// Resets the global vec3 to (0, 1, 0).
void fn_8013A01C(void) {
    *(f32*)(lbl_803F0D00 + 0x0) = 0.0f;
    *(f32*)(lbl_803F0D00 + 0x4) = 1.0f;
    *(f32*)(lbl_803F0D00 + 0x8) = 0.0f;
}

// 0x8013CA54
// Resets the global vec3 to (0, 1, 0).
void fn_8013CA54(void) {
    *(f32*)(lbl_803F2078 + 0x0) = 0.0f;
    *(f32*)(lbl_803F2078 + 0x4) = 1.0f;
    *(f32*)(lbl_803F2078 + 0x8) = 0.0f;
}

// 0x8013EAFC
// Resets the global vec3 to (0, 1, 0).
void fn_8013EAFC(void) {
    *(f32*)(lbl_803F2220 + 0x0) = 0.0f;
    *(f32*)(lbl_803F2220 + 0x4) = 1.0f;
    *(f32*)(lbl_803F2220 + 0x8) = 0.0f;
}

// 0x8013F014
// Resets the global vec3 to (0, 1, 0).
void fn_8013F014(void) {
    *(f32*)(lbl_803F2240 + 0x0) = 0.0f;
    *(f32*)(lbl_803F2240 + 0x4) = 1.0f;
    *(f32*)(lbl_803F2240 + 0x8) = 0.0f;
}

// 0x801420CC
// Resets the global vec3 to (0, 1, 0).
void fn_801420CC(void) {
    *(f32*)(lbl_803F22A8 + 0x0) = 0.0f;
    *(f32*)(lbl_803F22A8 + 0x4) = 1.0f;
    *(f32*)(lbl_803F22A8 + 0x8) = 0.0f;
}

// 0x80142498
// Resets the global vec3 to (0, 1, 0).
void fn_80142498(void) {
    *(f32*)(lbl_803F22B8 + 0x0) = 0.0f;
    *(f32*)(lbl_803F22B8 + 0x4) = 1.0f;
    *(f32*)(lbl_803F22B8 + 0x8) = 0.0f;
}

// 0x8019B860
// Appends a 32-bit value to a growable list: count at +4, data pointer at +8.
s32 fn_8019B860(s32 value, u8* list) {
    s32 count = *(s32*)(list + 0x4);
    u8* data = *(u8**)(list + 0x8);
    *(s32*)(data + count * 4) = value;
    *(s32*)(list + 0x4) = count + 1;
    return 0;
}

}  // extern "C"
