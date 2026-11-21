#include "types.h"

extern "C" {
u8* fn_8015ECA0(u8* a3);  // 0x8015ECA0
extern u8 lbl_8040D500[];
}

// Blend one 4-float colour slot of the effect record towards `src` by weight t.
static void BlendSlot(u32 off, const f32* src, f32 t) {
    f32 mul[4];
    for (int i = 0; i < 4; i++) mul[i] = src[i] * t;
    u8* rec = fn_8015ECA0(lbl_8040D500);
    f32 w = 1.0f - t;
    f32 res[4];
    for (int i = 0; i < 4; i++) res[i] = *(f32*)(rec + off + i * 4) * w + mul[i];
    rec = fn_8015ECA0(lbl_8040D500);
    for (int i = 0; i < 4; i++) *(f32*)(rec + off + i * 4) = res[i];
}

extern "C" {

// 0x80166C44
// Effect-slot handler: when the weight (p+0x20) is positive, overwrite the whole colour
// record with fixed parameters and the colours stored in p (arg0 unused).
void fn_80166C44(void* fx, u8* p) {
    if (*(f32*)(p + 0x20) > 0.0f) {
        static const f32 kQuarter3 = 0.75f;
        f32 c75[4] = {kQuarter3, kQuarter3, kQuarter3, kQuarter3};
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x04) = 1.35f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x08) = 0.0f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x0C) = 1.0f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x10) = 1.0f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x14) = 0.1f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x18) = 0.0f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x1C) = 0.01f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x20) = 1.0f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x24) = 0.75f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x28) = 0.0f;
        *(f32*)(fn_8015ECA0(lbl_8040D500) + 0x2C) = 0.0f;
        u8* r = fn_8015ECA0(lbl_8040D500);
        for (int i = 0; i < 4; i++) *(f32*)(r + 0x30 + i * 4) = *(f32*)(p + 0x10 + i * 4);
        r = fn_8015ECA0(lbl_8040D500);
        for (int i = 0; i < 4; i++) *(f32*)(r + 0x40 + i * 4) = *(f32*)(p + i * 4);
        r = fn_8015ECA0(lbl_8040D500);
        for (int i = 0; i < 4; i++) *(f32*)(r + 0x50 + i * 4) = *(f32*)(p + i * 4);
        r = fn_8015ECA0(lbl_8040D500);
        for (int i = 0; i < 4; i++) *(f32*)(r + 0x60 + i * 4) = c75[i];
        r = fn_8015ECA0(lbl_8040D500);
        for (int i = 0; i < 4; i++) *(f32*)(r + 0x70 + i * 4) = c75[i];
        r = fn_8015ECA0(lbl_8040D500);
        for (int i = 0; i < 4; i++) *(f32*)(r + 0x80 + i * 4) = c75[i];
        r = fn_8015ECA0(lbl_8040D500);
        for (int i = 0; i < 4; i++) *(f32*)(r + 0x90 + i * 4) = 0.0f;
    }
}

// 0x801665E4
// Effect-slot handler: when the weight t (p+0x20) is positive, blend each colour slot of
// the record towards its target by t (arg0 unused).
void fn_801665E4(void* fx, u8* p) {
    f32 t = *(f32*)(p + 0x20);
    if (t > 0.0f) {
        f32 a[4], b[4], c[4], d[4], e[4];
        for (int i = 0; i < 4; i++) {
            a[i] = *(f32*)(p + i * 4);
            b[i] = *(f32*)(p + 0x10 + i * 4);
            c[i] = 0.3019608f;
            d[i] = 0.5882353f;
            e[i] = 0.10980392f;
        }
        BlendSlot(0x40, a, t);
        BlendSlot(0x50, a, t);
        BlendSlot(0x30, b, t);
        BlendSlot(0x60, c, t);
        BlendSlot(0x70, d, t);
        BlendSlot(0x80, e, t);
    }
}

}  // extern "C"
