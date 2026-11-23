// Draft-fix batch miss431: gradient vertex buffers, a render pass, HSL -> RGBA.
#include "types.h"

extern "C" {
void fn_800446BC(void* cam, s32 v);                               // 0x800446BC
u32  fn_8010A9F4(u8* p, s32 mask);                                // 0x8010A9F4
void fn_8010AAE0(void);                                           // 0x8010AAE0
void fn_8010AB0C(s32 a, s32 b);                                   // 0x8010AB0C
void fn_8010AB3C(void);                                           // 0x8010AB3C
void fn_8010AB70(void);                                           // 0x8010AB70
void fn_8010ABA8(u8* p, s32 v);                                   // 0x8010ABA8
void fn_8010AED8(u8* p, s32 v);                                   // 0x8010AED8
void fn_80155D70(void);                                           // 0x80155D70
extern u8 lbl_80404050[];  // 0x80404050 (gradient vertex buffers, 0x1B00 per set)
extern u8 lbl_80409150[];  // 0x80409150 (gradient color buffers, 0x900 per set)
extern u8 lbl_8047C640[];  // 0x8047C640 (camera)
}
extern "C++" void fn_80154358(void);  // 0x80154358 (C++ linkage, defined in small_8015.cpp)

extern "C" {

// 0x80156CDC
// Optionally fills buffer set idx with an 18-row, 32-column vertical gradient grid (x in -1..1,
// y from 1 down to -1, z = 1, colors all 0xFFFFFFFF); returns the set's buffers via c/out.
void fn_80156CDC(s32 idx, s32 flag, void** c, u32** out) {
    u8* verts = lbl_80404050 + idx * 0x1B00;
    u8* cols = lbl_80409150 + idx * 0x900;
    if (flag != 0) {
        u8* v = verts;
        u8* col = cols;
        for (s32 row = 0; row < 0x12; row++) {
            f32 y = 1.0f + (-2.0f * (0.05882353f * (f32)row));
            // 32 columns, written eight at a time; the column number runs on across the groups.
            for (s32 first = 0; first < 0x20; first += 8) {
                for (s32 k = 0; k < 8; k++) {
                    *(f32*)(v + k * 0xC + 0x0) = (2.0f * (0.032258064f * (f32)(first + k))) - 1.0f;
                    *(f32*)(v + k * 0xC + 0x4) = y;
                    *(f32*)(v + k * 0xC + 0x8) = 1.0f;
                    *(s32*)(col + k * 4) = -1;
                }
                v += 0x60;
                col += 0x20;
            }
        }
    }
    if (c != NULL) {
        *c = verts;
    }
    if (out != NULL) {
        *out = (u32*)cols;
    }
}

// 0x8010AA0C
// Renders one object inside the pass bracket (fn_8010AB0C), using the camera value at
// (*(p+0x34))+0x70; flag byte +0x4A picks the draw routine.
void fn_8010AA0C(u8* p) {
    if (fn_8010A9F4(p + 0x30, 1) != 0) {
        s32 v = 0;
        fn_8010AAE0();
        fn_8010AB0C(1, 0);
        fn_80154358();
        fn_80155D70();
        fn_8010AB3C();
        u8* sub = *(u8**)(p + 0x34);
        if (sub != NULL) {
            v = *(s32*)(sub + 0x70);
            fn_800446BC(lbl_8047C640, v);
        } else {
            fn_800446BC(lbl_8047C640, 0);
        }
        if (*(u8*)(p + 0x4A) != 0) {
            fn_8010AED8(p, v);
        } else {
            fn_8010ABA8(p, v);
        }
        fn_8010AB70();
        fn_80154358();
        fn_8010AB0C(1, 1);
    }
}

// Hue-to-channel helper for fn_8014B1A8 (standard HSL conversion, t already offset).
static inline f32 HslChan(f32 t, f32 p, f32 q) {
    if (t < 0.0f) {
        t += 1.0f;
    }
    if (t > 1.0f) {
        t -= 1.0f;
    }
    if (6.0f * t < 1.0f) {
        return p + t * (6.0f * (q - p));
    }
    if (2.0f * t < 1.0f) {
        return q;
    }
    if (3.0f * t < 2.0f) {
        return p + 6.0f * ((0.6666667f - t) * (q - p));
    }
    return p;
}

// 0x8014B1A8
// HSL (h, s, l fractions) -> packed color 0xAARRGGBB-style word: alpha 0xFF in the top byte,
// then the bytes of the three channels (r from h+1/3, g from h, b from h-1/3).
u32 fn_8014B1A8(f32 h, f32 s, f32 l) {
    f32 r, g, b;
    if (s == 0.0f) {
        r = g = b = 255.0f * l;
    } else {
        f32 q;
        if (l < 0.5f) {
            q = l * (1.0f + s);
        } else {
            q = (l + s) - (s * l);
        }
        f32 p = (2.0f * l) - q;
        r = 255.0f * HslChan(0.33333334f + h, p, q);
        g = 255.0f * HslChan(h, p, q);
        b = 255.0f * HslChan(h - 0.33333334f, p, q);
    }
    return 0xFF000000u | ((u32)(u8)(s32)b << 16) | ((u32)(u8)(s32)g << 8) | (u32)(u8)(s32)r;
}

}  // extern "C"
