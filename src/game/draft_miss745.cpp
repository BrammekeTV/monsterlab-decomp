// UI element input/update functions (slider, grid, model update), a matrix builder and the
// camera-path follower, from the 0x801Axxxx / 0x801Dxxxx / 0x801Fxxxx ranges.
// Skipped (see report): none.
#include "types.h"

extern "C++" void* lbl_805FB670;    // 0x805FB670 (pad state)

BOOL fn_801982A8(void* obj, u32 mask);   // 0x801982A8 (C++ linkage)
BOOL fn_801982E4(void* obj, u32 mask);   // 0x801982E4 (C++ linkage)

typedef void (*VFnI)(void*, s32);
typedef void (*VFnF)(void*, f32);

extern "C" {
void  UIPlayAnim(void* el, u32 anim);                     // 0x8019BE40
u8*   fn_801483BC(s32 idx);                               // 0x801483BC
f32   fn_801F6610(u8* arg0, s32 arg1);                    // 0x801F6610
void  fn_8018D2B8(const char* fmt, ...);                  // 0x8018D2B8 (debug log)
u32   fn_8019552C(u8* list, s32 index);                   // 0x8019552C (leaf; returns a slot pointer)
s32   fn_80196678(void* gx, void* mat, f32 t);            // 0x80196678
s32   fn_80196B10(u8* a3);                                // 0x80196B10 (leaf)
s32   fn_8019B6CC(u8* arg0, f32 farg0);                   // 0x8019B6CC (UI element base update)
s32   fn_801A5608(u8* arg0);                              // 0x801A5608
s32   fn_801A5830(u8* arg0);                              // 0x801A5830
s32   fn_801A5A6C(u8* arg0);                              // 0x801A5A6C
s32   fn_801A5C94(u8* arg0);                              // 0x801A5C94
s32   fn_801A72EC(u8* grid);                              // 0x801A72EC

extern f32 lbl_805FF9F0;    // 0x805FF9F0
extern f32 lbl_805FFA0C;    // 0x805FFA0C (0.0)
extern void* lbl_805FB658;  // 0x805FB658 (global render object)
}

// Pad-direction flags polled by the slider and grid: bit 0 = up/left-ish pair, etc.
// Reads the four direction booleans (0x1/0x4000, 0x2/0x8000, 0x4/0x10000, 0x8/0x20000).
// useRelease selects fn_801982E4 (element flag 0x100) instead of fn_801982A8.
static inline void ReadDirs(bool useRelease, s32* d0, s32* d1, s32* d2, s32* d3) {
    if (useRelease) {
        if (fn_801982E4(lbl_805FB670, 1) != 0 || fn_801982E4(lbl_805FB670, 0x4000) != 0) {
            *d0 = 1;
        } else if (fn_801982E4(lbl_805FB670, 2) != 0 || fn_801982E4(lbl_805FB670, 0x8000) != 0) {
            *d1 = 1;
        }
        if (fn_801982E4(lbl_805FB670, 4) != 0 || fn_801982E4(lbl_805FB670, 0x10000) != 0) {
            *d2 = 1;
        } else if (fn_801982E4(lbl_805FB670, 8) != 0 || fn_801982E4(lbl_805FB670, 0x20000) != 0) {
            *d3 = 1;
        }
    } else {
        if (fn_801982A8(lbl_805FB670, 1) != 0 || fn_801982A8(lbl_805FB670, 0x4000) != 0) {
            *d0 = 1;
        } else if (fn_801982A8(lbl_805FB670, 2) != 0 || fn_801982A8(lbl_805FB670, 0x8000) != 0) {
            *d1 = 1;
        }
        if (fn_801982A8(lbl_805FB670, 4) != 0 || fn_801982A8(lbl_805FB670, 0x10000) != 0) {
            *d2 = 1;
        } else if (fn_801982A8(lbl_805FB670, 8) != 0 || fn_801982A8(lbl_805FB670, 0x20000) != 0) {
            *d3 = 1;
        }
    }
}

// Clamp x between the two bounds a and b; if a >= b the roles are swapped (as in the asm).
static inline f32 ClampSwap(f32 x, f32 a, f32 b) {
    if (a < b) {
        if (x < a) return a;
        if (x > b) return b;
        return x;
    }
    if (x < b) return b;
    if (x > a) return a;
    return x;
}

// Packs four 0..1 floats (+0x80..+0x8C) into RGBA bytes, each clamped to 0..255.
static inline u32 PackColor(const u8* p) {
    s32 r = (s32)(255.0f * *(f32*)(p + 0x80));
    s32 g = (s32)(255.0f * *(f32*)(p + 0x84));
    s32 b = (s32)(255.0f * *(f32*)(p + 0x88));
    s32 a = (s32)(255.0f * *(f32*)(p + 0x8C));
    r = r < 0 ? 0 : (r > 0xFF ? 0xFF : r);
    g = g < 0 ? 0 : (g > 0xFF ? 0xFF : g);
    b = b < 0 ? 0 : (b > 0xFF ? 0xFF : b);
    a = a < 0 ? 0 : (a > 0xFF ? 0xFF : a);
    return (u32)(r | (g << 8) | (a << 24) | (b << 16));
}

extern "C" {

// 0x801A3A28
// UISlider input: polls pad directions and nudges the slider value (+0x140, 0..1 between the
// min/max at +0x14C/+0x150) by the step at +0x154, then plays the matching arrow animations.
// Only active when flag 0x40000 is set; the +0x98 test restricts it to one axis.
s32 fn_801A3A28(u8* arg0) {
    s32 var_r31 = 0;
    s32 var_r30 = 0;
    s32 var_r29 = 0;
    s32 var_r28 = 0;
    if (!(*(s32*)(arg0 + 0x64) & 0x40000)) {
        return 0;
    }
    ReadDirs((*(s32*)(arg0 + 0x64) & 0x100) != 0, &var_r31, &var_r30, &var_r29, &var_r28);
    if (*(f32*)(arg0 + 0x98) != lbl_805FF9F0) {
        var_r31 = 0;
        var_r30 = 0;
    } else {
        var_r29 = 0;
        var_r28 = 0;
    }
    f32 minV = *(f32*)(arg0 + 0x14C);
    f32 cur = minV + *(f32*)(arg0 + 0x140) * (*(f32*)(arg0 + 0x150) - minV);
    if (var_r29 != 0 || var_r30 != 0) {
        cur -= *(f32*)(arg0 + 0x154);
        if (cur < *(f32*)(arg0 + 0x14C)) {
            cur = *(f32*)(arg0 + 0x14C);
        }
        f32 lo = *(f32*)(arg0 + 0x14C);
        if (cur < lo) {
            cur = lo;
        } else {
            f32 hi = *(f32*)(arg0 + 0x150);
            if (cur > hi) {
                cur = hi;
            }
        }
        f32 hi2 = *(f32*)(arg0 + 0x150);
        f32 lo2 = *(f32*)(arg0 + 0x14C);
        if (hi2 != lo2) {
            *(f32*)(arg0 + 0x140) = (cur - lo2) / (hi2 - lo2);
        } else {
            fn_8018D2B8("UISlider : min/max range = 0.0f ...\n");
        }
        UIPlayAnim(arg0, 0x3AB57627U);  // "xDEc"
        UIPlayAnim(arg0, 0x82091142U);  // "yDEc"
    } else if (var_r28 != 0 || var_r31 != 0) {
        f32 hi = *(f32*)(arg0 + 0x150);
        cur += *(f32*)(arg0 + 0x154);
        if (cur > hi) {
            cur = hi;
        }
        f32 lo = *(f32*)(arg0 + 0x14C);
        if (cur < lo) {
            cur = lo;
        } else {
            f32 hi3 = *(f32*)(arg0 + 0x150);
            if (cur > hi3) {
                cur = hi3;
            }
        }
        f32 hi2 = *(f32*)(arg0 + 0x150);
        f32 lo2 = *(f32*)(arg0 + 0x14C);
        if (hi2 != lo2) {
            *(f32*)(arg0 + 0x140) = (cur - lo2) / (hi2 - lo2);
        } else {
            fn_8018D2B8("UISlider : min/max range = 0.0f ...\n");
        }
        UIPlayAnim(arg0, 0xD1993CBFU);  // "XINC"
        UIPlayAnim(arg0, 0x69255BDAU);
    }
    return 0;
}

// 0x801A52A0
// UIGrid input: polls pad directions and scrolls the grid, redraws it, and plays the matching
// animations on the grid and on the cell at the (wrapped) cursor when the accept bit (0x40) is down.
// Does nothing when the value at +0x174 is >= 0.0.
s32 fn_801A52A0(u8* arg0) {
    s32 var_r31 = 0;
    s32 var_r30 = 0;
    s32 var_r29 = 0;
    s32 var_r28 = 0;
    if (*(f32*)(arg0 + 0x174) >= lbl_805FFA0C) {
        return 0;
    }
    ReadDirs((*(s32*)(arg0 + 0x64) & 0x100) != 0, &var_r31, &var_r30, &var_r29, &var_r28);
    if (var_r31 != 0) {
        fn_801A5830(arg0);
    } else if (var_r30 != 0) {
        fn_801A5608(arg0);
    }
    if (var_r29 != 0) {
        fn_801A5C94(arg0);
    } else if (var_r28 != 0) {
        fn_801A5A6C(arg0);
    }
    if (var_r31 != 0 || var_r30 != 0 || var_r29 != 0 || var_r28 != 0) {
        fn_801A72EC(arg0);
    }
    if (fn_801982A8(lbl_805FB670, 0x40) != 0) {
        s32 row = *(s32*)(arg0 + 0x14C) + *(s32*)(arg0 + 0x15C);
        s32 col = *(s32*)(arg0 + 0x150) + *(s32*)(arg0 + 0x160);
        if (row < 0) {
            row += *(s32*)(arg0 + 0x13C);
        } else if (row >= *(s32*)(arg0 + 0x13C)) {
            row -= *(s32*)(arg0 + 0x13C);
        }
        if (col < 0) {
            col += *(s32*)(arg0 + 0x140);
        } else if (col >= *(s32*)(arg0 + 0x140)) {
            col -= *(s32*)(arg0 + 0x140);
        }
        u8** slot = (u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC), col + row * *(s32*)(arg0 + 0x140));
        u8* cell = (slot != NULL) ? *slot : NULL;
        if (cell != NULL) {
            UIPlayAnim(cell, 0x43D82AB2U);  // "accept"
        }
    }
    if (var_r31 != 0) {
        UIPlayAnim(arg0, 0x69255BDAU);
    } else if (var_r30 != 0) {
        UIPlayAnim(arg0, 0x82091142U);  // "yDEc"
    }
    if (var_r29 != 0) {
        UIPlayAnim(arg0, 0x3AB57627U);  // "xDEc"
    } else if (var_r28 != 0) {
        UIPlayAnim(arg0, 0xD1993CBFU);  // "XINC"
    }
    return 0;
}

// 0x801A7EA0
// UI model element update: base update, then (flag 0x20) copies its colour to the child at +0x190
// when the packed RGBA differs and notifies via the vtable (+0x2C/+0x30 with 0x20); (flag 0x10)
// renders the child with the element's matrix at +0xF0, position at +0x15C and updates it.
s32 fn_801A7EA0(u8* arg0, f32 farg0) {
    fn_8019B6CC(arg0, farg0);
    if (*(s32*)(arg0 + 0x64) & 0x20) {
        u8* child = *(u8**)(arg0 + 0x190);
        if (child != NULL) {
            if (PackColor(arg0) != PackColor(child)) {
                f32 r = *(f32*)(arg0 + 0x80);
                f32 g = *(f32*)(arg0 + 0x84);
                f32 b = *(f32*)(arg0 + 0x88);
                f32 a = *(f32*)(arg0 + 0x8C);
                *(f32*)(child + 0x80) = r;
                *(f32*)(child + 0x84) = g;
                *(f32*)(child + 0x88) = b;
                *(f32*)(child + 0x8C) = a;
                u8* c2 = *(u8**)(arg0 + 0x190);
                (*(VFnI*)(*(u8**)(c2 + 0x138) + 0x2C))(c2, 0x20);
            }
        }
        (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x20);
    }
    if (*(s32*)(arg0 + 0x64) & 0x10) {
        if (*(u8**)(arg0 + 0x190) != NULL) {
            fn_80196678(lbl_805FB658, arg0 + 0xF0, farg0);
            f32 px = *(f32*)(arg0 + 0x15C);
            f32 py = *(f32*)(arg0 + 0x160);
            f32 pz = *(f32*)(arg0 + 0x164);
            u8* c = *(u8**)(arg0 + 0x190);
            *(f32*)(c + 0x68) = px;
            *(f32*)(c + 0x6C) = py;
            *(f32*)(c + 0x70) = pz;
            u8* c2 = *(u8**)(arg0 + 0x190);
            (*(VFnI*)(*(u8**)(c2 + 0x138) + 0x2C))(c2, 8);
            u8* c3 = *(u8**)(arg0 + 0x190);
            (*(VFnF*)(*(u8**)(c3 + 0x138) + 0x18))(c3, farg0);
            fn_80196B10((u8*)lbl_805FB658);
        }
        (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x10);
    }
    return 0;
}

// 0x801D964C
// Builds a 3x4 transform from a quaternion (arg3), scale (arg2) and position (arg1) and stores it
// at the matrix at +0x34 of arg0, plus the X scale at +0x68. The 4x4 temporaries on the stack come
// from the original matrix multiplies (identity/zero terms kept so the arithmetic matches).
void fn_801D964C(u8* arg0, u8* arg1, u8* arg2, u8* arg3) {
    f32 sp124, sp120, sp11C, sp118, sp114, sp110, sp10C, sp108, sp104, sp100, spFC, spF8;
    f32 spF4, spF0, spEC, spE8, spE4, spE0, spDC, spD8, spD4, spD0, spCC, spC8;
    f32 spC4, spC0, spBC, spB8, spB4, spB0, spAC, spA8, spA4, spA0, sp9C, sp98;
    f32 sp94, sp90, sp8C, sp88, sp84, sp80, sp7C, sp78, sp74, sp70, sp6C, sp68;
    f32 sp64, sp60, sp5C, sp58, sp54, sp50, sp4C, sp48, sp44, sp40, sp3C, sp38;
    f32 sp34, sp30, sp2C, sp28, sp24, sp20, sp1C, sp18, sp14, sp10, spC, sp8;
    f32 temp_f0_2, temp_f0_3, temp_f10, temp_f10_2, temp_f10_3, temp_f11, temp_f11_2, temp_f11_3, temp_f11_4;
    f32 temp_f12, temp_f12_2, temp_f13, temp_f1, temp_f1_2, temp_f1_3, temp_f1_4, temp_f21;
    f32 temp_f22, temp_f22_2, temp_f23, temp_f23_2, temp_f23_3, temp_f24, temp_f24_2, temp_f25;
    f32 temp_f29, temp_f0, temp_f2, temp_f2_2, temp_f2_3, temp_f2_4, temp_f2_5, temp_f30, temp_f31;
    f32 temp_f3, temp_f3_2, temp_f3_3, temp_f4, temp_f4_2, temp_f4_3, temp_f5, temp_f5_2;
    f32 temp_f6, temp_f6_2, temp_f6_3, temp_f7, temp_f7_2, temp_f7_3, temp_f8, temp_f8_2, temp_f8_3;
    f32 temp_f9, temp_f9_2, temp_f9_3;
    u8* temp_r4;
    u8* temp_r5;

    temp_f2 = 1.0f;
    temp_f29 = *(f32*)(arg2 + 0x0);
    temp_f30 = temp_f2 * temp_f29;
    sp40 = 0.0f;
    temp_f31 = temp_f2 * *(f32*)(arg2 + 0x4);
    temp_f13 = temp_f2 * *(f32*)(arg2 + 0x8);
    sp3C = 0.0f;
    sp38 = 1.0f;
    sp98 = 1.0f;
    sp9C = 0.0f;
    spA0 = 0.0f;
    sp4C = 0.0f;
    sp44 = 0.0f;
    sp48 = 1.0f;
    spA4 = 0.0f;
    spA8 = 1.0f;
    spAC = 0.0f;
    sp54 = 0.0f;
    sp50 = 0.0f;
    sp58 = 1.0f;
    spB0 = 0.0f;
    spB4 = 0.0f;
    spB8 = 1.0f;
    sp64 = 0.0f;
    sp60 = 0.0f;
    sp5C = 0.0f;
    spBC = 0.0f;
    spC0 = 0.0f;
    spC4 = 0.0f;
    spCC = 0.0f;
    spD0 = 0.0f;
    spD4 = 0.0f;
    spDC = 0.0f;
    spE0 = 0.0f;
    spE4 = 0.0f;
    spEC = 0.0f;
    spF0 = 0.0f;
    spF4 = 0.0f;
    spC8 = temp_f30;
    spD8 = temp_f31;
    spE8 = temp_f13;
    temp_f3 = *(f32*)(arg3 + 0x4);
    temp_f2_2 = *(f32*)(arg3 + 0x8);
    temp_f4 = 2.0f * temp_f3;
    temp_f8 = *(f32*)(arg3 + 0x0);
    temp_f0 = 2.0f * temp_f2_2;
    temp_f7 = *(f32*)(arg3 + 0xC);
    temp_f1 = 2.0f * temp_f8;
    sp34 = 0.0f;
    temp_f6 = temp_f4 * temp_f3;
    sp30 = 0.0f;
    temp_f5 = temp_f0 * temp_f2_2;
    temp_f11 = temp_f0 * temp_f7;
    sp2C = 0.0f;
    temp_f10 = temp_f4 * temp_f8;
    sp11C = 0.0f;
    temp_f25 = temp_f4 * temp_f7;
    temp_f22 = temp_f10 - temp_f11;
    sp120 = 0.0f;
    temp_f23 = 1.0f - (temp_f6 + temp_f5);
    temp_f12 = temp_f10 + temp_f11;
    sp124 = 0.0f;
    temp_f9 = temp_f0 * temp_f8;
    temp_f24 = temp_f1 * temp_f8;
    sp8 = temp_f23;
    temp_f8_2 = temp_f9 + temp_f25;
    spF8 = temp_f23;
    temp_f11_2 = temp_f9 - temp_f25;
    sp14 = temp_f22;
    temp_f7_2 = temp_f1 * temp_f7;
    spC = temp_f12;
    temp_f10_2 = 1.0f - (temp_f24 + temp_f5);
    temp_f6_2 = 1.0f - (temp_f24 + temp_f6);
    sp10 = temp_f11_2;
    temp_f4_2 = temp_f22 * 0.0f;
    spFC = temp_f12;
    temp_f5_2 = temp_f8_2 * 0.0f;
    sp100 = temp_f11_2;
    temp_f2_3 = temp_f23 * 0.0f;
    temp_f1_2 = temp_f0 * temp_f3;
    sp18 = temp_f10_2;
    temp_f23_2 = temp_f5_2 + ((temp_f23 * temp_f30) + temp_f4_2);
    sp104 = temp_f22;
    temp_f9_2 = temp_f1_2 + temp_f7_2;
    temp_f7_3 = temp_f1_2 - temp_f7_2;
    sp108 = temp_f10_2;
    temp_f3_2 = temp_f5_2 + (temp_f2_3 + (temp_f22 * temp_f31));
    sp20 = temp_f8_2;
    sp1C = temp_f9_2;
    sp10C = temp_f9_2;
    sp24 = temp_f7_3;
    sp28 = temp_f6_2;
    sp110 = temp_f8_2;
    sp114 = temp_f7_3;
    sp118 = temp_f6_2;
    sp68 = temp_f23_2;
    sp74 = temp_f3_2;
    temp_r5 = *(u8**)(arg0 + 0x34);
    temp_f21 = temp_f10_2 * 0.0f;
    temp_f24_2 = temp_f9_2 * 0.0f;
    *(f32*)(temp_r5 + 0x0) = temp_f23_2;
    temp_f23_3 = temp_f12 * 0.0f;
    temp_f1_3 = temp_f7_3 * 0.0f;
    temp_f0_2 = temp_f6_2 * 0.0f;
    temp_f22_2 = temp_f1_3 + ((temp_f12 * temp_f30) + temp_f21);
    temp_f10_3 = temp_f0_2 + ((temp_f11_2 * temp_f30) + temp_f24_2);
    temp_f2_4 = temp_f2_3 + temp_f4_2;
    *(f32*)(temp_r5 + 0x4) = temp_f22_2;
    temp_f11_3 = temp_f11_2 * 0.0f;
    *(f32*)(temp_r5 + 0x8) = temp_f10_3;
    temp_f12_2 = temp_f23_3 + temp_f21;
    temp_f4_3 = temp_f11_3 + temp_f24_2;
    *(f32*)(temp_r5 + 0xC) = temp_f3_2;
    sp6C = temp_f22_2;
    temp_f11_4 = temp_f1_3 + (temp_f23_3 + (temp_f10_2 * temp_f31));
    sp70 = temp_f10_3;
    temp_f8_3 = temp_f0_2 + (temp_f11_3 + (temp_f9_2 * temp_f31));
    temp_f9_3 = (temp_f8_2 * temp_f13) + temp_f2_4;
    *(f32*)(temp_r5 + 0x10) = temp_f11_4;
    *(f32*)(temp_r5 + 0x14) = temp_f8_3;
    *(f32*)(temp_r5 + 0x18) = temp_f9_3;
    temp_f6_3 = (temp_f7_3 * temp_f13) + temp_f12_2;
    temp_f2_5 = (temp_f6_2 * temp_f13) + temp_f4_3;
    sp80 = temp_f9_3;
    temp_f3_3 = 0.0f + (temp_f5_2 + temp_f2_4);
    *(f32*)(temp_r5 + 0x1C) = temp_f6_3;
    temp_f1_4 = 0.0f + (temp_f1_3 + temp_f12_2);
    temp_f0_3 = 0.0f + (temp_f0_2 + temp_f4_3);
    *(f32*)(temp_r5 + 0x20) = temp_f2_5;
    *(f32*)(temp_r5 + 0x24) = temp_f3_3;
    *(f32*)(temp_r5 + 0x28) = temp_f1_4;
    *(f32*)(temp_r5 + 0x2C) = temp_f0_3;
    temp_r4 = *(u8**)(arg0 + 0x34);
    sp8C = temp_f3_3;
    *(f32*)(temp_r4 + 0x24) = *(f32*)(arg1 + 0x0);
    *(f32*)(temp_r4 + 0x28) = *(f32*)(arg1 + 0x4);
    *(f32*)(temp_r4 + 0x2C) = *(f32*)(arg1 + 0x8);
    *(f32*)(arg0 + 0x68) = temp_f29;
    sp78 = temp_f11_4;
    sp84 = temp_f6_3;
    sp90 = temp_f1_4;
    sp7C = temp_f8_3;
    sp88 = temp_f2_5;
    sp94 = temp_f0_3;
}

// 0x801F6290
// Camera-path follower: if the current path point (entry stride 0x84, flag byte at +0x5E) is
// active, stores its XY (clamped to +-1/scale when +0xB8 is set) with Z = 0 at +0x90..+0x98 and
// the path value at +0x9C. If the next point is also active, does the same for it and stores the
// differences at +0xA0..+0xA8 and the value delta at +0xAC.
// (The asm also converts gScreenWidth/gScreenHeight to float via the 0x4330 idiom, but never
// reads the result.)
void fn_801F6290(u8* arg0) {
    u8* T = fn_801483BC(*(s32*)(arg0 + 0x4));
    s32 count = *(s32*)(T + 0xC);
    s32 cur = *(s32*)(T + 0x10);
    u8* arr = *(u8**)(T + 0x4);
    if (*(s8*)(arr + (cur % count) * 0x84 + 0x5E) > 0) {
        s32 hasNext = *(s8*)(arr + ((cur + 1) % count) * 0x84 + 0x5E) > 0;

        T = fn_801483BC(*(s32*)(arg0 + 0x4));
        count = *(s32*)(T + 0xC);
        cur = *(s32*)(T + 0x10);
        u8* e = *(u8**)(T + 0x4) + (cur % count) * 0x84;
        f32 px = *(f32*)(e + 0x20);
        f32 py = *(f32*)(e + 0x24);
        f32 pz = 0.0f;
        if (*(u8*)(arg0 + 0xB8) != 0) {
            f32 hx = 1.0f / *(f32*)(arg0 + 0xB0);
            f32 hy = 1.0f / *(f32*)(arg0 + 0xB4);
            px = ClampSwap(px, -hx, hx);
            py = ClampSwap(py, -hy, hy);
        }
        *(f32*)(arg0 + 0x90) = px;
        *(f32*)(arg0 + 0x94) = py;
        *(f32*)(arg0 + 0x98) = pz;
        *(f32*)(arg0 + 0x9C) = fn_801F6610(arg0, 0);

        if (hasNext) {
            T = fn_801483BC(*(s32*)(arg0 + 0x4));
            count = *(s32*)(T + 0xC);
            s32 nxt = *(s32*)(T + 0x10) + 1;
            u8* e2 = *(u8**)(T + 0x4) + (nxt % count) * 0x84;
            f32 nx = *(f32*)(e2 + 0x20);
            f32 ny = *(f32*)(e2 + 0x24);
            f32 nz = 0.0f;
            if (*(u8*)(arg0 + 0xB8) != 0) {
                f32 hx = 1.0f / *(f32*)(arg0 + 0xB0);
                f32 hy = 1.0f / *(f32*)(arg0 + 0xB4);
                nx = ClampSwap(nx, -hx, hx);
                ny = ClampSwap(ny, -hy, hy);
            }
            f32 dx = *(f32*)(arg0 + 0x90) - nx;
            f32 dy = *(f32*)(arg0 + 0x94) - ny;
            f32 dz = *(f32*)(arg0 + 0x98) - nz;
            *(f32*)(arg0 + 0xA0) = dx;
            *(f32*)(arg0 + 0xA4) = dy;
            *(f32*)(arg0 + 0xA8) = dz;
            *(f32*)(arg0 + 0xAC) = *(f32*)(arg0 + 0x9C) - fn_801F6610(arg0, 1);
        }
    }
}

// 0x801FAB94
// Camera-path state machine: walks the path points from the last down to 0 and runs a 3-state
// machine at +0xA8 (0 = wait for the point's Y to exceed +0xAC, 1 = hold while it stays above
// +0xAC counting at +0xBC, 2 = cool down until Y drops below +0xB4). Sets the byte at +0x90 when
// state 2 ends because Y dropped below +0xB4.
void fn_801FAB94(u8* arg0) {
    u8* T = fn_801483BC(*(s32*)(arg0 + 0x4));
    *(u8*)(arg0 + 0x90) = 0;
    for (s32 i = *(s32*)(T + 0x14) - 1; i >= 0; i--) {
        s32 state = *(s32*)(arg0 + 0xA8);
        if (state == 0) {
            u8* t = fn_801483BC(*(s32*)(arg0 + 0x4));
            u8* e = *(u8**)(t + 0x4) + ((*(s32*)(t + 0x10) + i) % *(s32*)(t + 0xC)) * 0x84;
            f32 y = (*(s32*)(arg0 + 0x98) == 0) ? *(f32*)(e + 0x10) : *(f32*)(e + 0x6C);
            if (y > *(f32*)(arg0 + 0xAC)) {
                *(s32*)(arg0 + 0xA8) = 1;
                *(s32*)(arg0 + 0xBC) = 0;
            }
        } else if (state == 1) {
            u8* t = fn_801483BC(*(s32*)(arg0 + 0x4));
            u8* e = *(u8**)(t + 0x4) + ((*(s32*)(t + 0x10) + i) % *(s32*)(t + 0xC)) * 0x84;
            f32 y = (*(s32*)(arg0 + 0x98) == 0) ? *(f32*)(e + 0x10) : *(f32*)(e + 0x6C);
            if (y < *(f32*)(arg0 + 0xAC)) {
                if (*(s32*)(arg0 + 0xBC) > *(s32*)(arg0 + 0xB0)) {
                    *(s32*)(arg0 + 0xA8) = 2;
                    *(s32*)(arg0 + 0xBC) = 0;
                } else {
                    *(s32*)(arg0 + 0xA8) = 0;
                }
            } else {
                *(s32*)(arg0 + 0xBC) = *(s32*)(arg0 + 0xBC) + 1;
            }
        } else if (state == 2) {
            u8* t = fn_801483BC(*(s32*)(arg0 + 0x4));
            u8* e = *(u8**)(t + 0x4) + ((*(s32*)(t + 0x10) + i) % *(s32*)(t + 0xC)) * 0x84;
            f32 y = (*(s32*)(arg0 + 0x98) == 0) ? *(f32*)(e + 0x10) : *(f32*)(e + 0x6C);
            u8 flag;
            if (y < *(f32*)(arg0 + 0xB4)) {
                *(s32*)(arg0 + 0xA8) = 0;
                flag = 1;
                *(s32*)(arg0 + 0xBC) = 0;
            } else {
                s32 n = *(s32*)(arg0 + 0xBC) + 1;
                *(s32*)(arg0 + 0xBC) = n;
                if (n > *(s32*)(arg0 + 0xB8)) {
                    *(s32*)(arg0 + 0xA8) = 0;
                    *(s32*)(arg0 + 0xBC) = 0;
                }
                flag = 0;
            }
            *(u8*)(arg0 + 0x90) = flag;
        }
    }
}

}  // extern "C"
