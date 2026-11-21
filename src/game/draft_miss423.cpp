// Effect-slot handlers: radial vignette / flash overlays. Both build a 3x4 transform
// (aspect scale, rotation about Z, un-aspect) and apply it to the 576 vertices of the slot's
// mesh buffer, then fill the per-vertex colour buffer.
#include <math.h>

#include "types.h"

extern "C" {
void fn_801451A0(u8* m, f32 angle);                              // 0x801451A0
void fn_80156CDC(s32 idx, s32 flag, void* c, u32** out);         // 0x80156CDC
void fn_80156F3C(s32 idx);                                       // 0x80156F3C (empty)

// Apply the 3x3 part of m to 0x240 packed (x,y,z) vertices in place.
static void ApplyMtx(const f32* m, u8* v) {
    for (s32 i = 0; i < 0x240; i++, v += 0xC) {
        f32 x = *(f32*)(v + 0), y = *(f32*)(v + 4), z = *(f32*)(v + 8);
        *(f32*)(v + 0) = m[6] * z + (m[0] * x + m[3] * y);
        *(f32*)(v + 4) = m[7] * z + (m[1] * x + m[4] * y);
        *(f32*)(v + 8) = m[8] * z + (m[2] * x + m[5] * y);
    }
}

// 0x80168E6C
// Radial alpha vignette: alpha ramps with the vertex's aspect-corrected distance from centre
// between p[0] (alpha 0) and p[4] (full).
void fn_80168E6C(u8* mgr, u8* p) {
    if (!(*(f32*)(p + 0x1C) > 0.0f)) return;
    s32 idx = *(s32*)(p + 0x18) - 1;
    u8* slot = mgr + idx * 4;
    u32* col = NULL;
    u8* verts = NULL;
    fn_80156CDC(idx, *(s32*)(slot + 0x3608) == 0, &verts, &col);

    f32 a = 255.0f * *(f32*)(p + 0x8);
    f32 scale = a / (*(f32*)(p + 0x4) - *(f32*)(p + 0x0));
    s32 full = (s32)(a * *(f32*)(p + 0x1C));
    f32 ang = 0.63661975f * (*(f32*)(p + 0x14) * (f32)sinf(1.5707964f * (0.63661975f * *(f32*)(p + 0x20))));

    f32 m[12];
    f32 s = *(f32*)(p + 0xC);
    m[0] = 1.7777778f * s;
    m[1] = 0.0f; m[2] = 0.0f; m[3] = 0.0f;
    m[4] = 1.0f * s;
    m[5] = 0.0f; m[6] = 0.0f; m[7] = 0.0f;
    m[8] = 1.0f * s;
    m[9] = 0.0f; m[10] = 0.0f; m[11] = 0.0f;
    fn_801451A0((u8*)m, ang);
    m[0] *= 0.5625f;
    m[4] *= 1.0f;
    m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;
    ApplyMtx(m, verts);

    const u8* v = verts;
    u32* o = col;
    for (s32 row = 0; row < 0x12; row++) {
        for (s32 i = 0; i < 0x20; i++, v += 0xC) {
            f32 x = *(f32*)(v + 0) * 1.7777778f;
            f32 y = *(f32*)(v + 4);
            f32 d2 = x * x + y * y;
            f32 d = (d2 <= 0.0f) ? 0.0f : sqrtf(d2);
            s32 al;
            if (d < *(f32*)(p + 0x0)) al = 0;
            else if (d >= *(f32*)(p + 0x4)) al = full;
            else al = (s32)(scale * (d - *(f32*)(p + 0x0)));
            *o++ = ((u32)al << 24) | 0xFFFFFFu;
        }
    }
    fn_80156F3C(idx);
    *(s32*)(slot + 0x3608) = 1;
}

// 0x801692C0
// Uniform full-screen tint: every vertex colour = 0xFFFFFF with alpha 255 * p[0x18] * p[0x10].
void fn_801692C0(u8* mgr, u8* p) {
    f32 amt = *(f32*)(p + 0x18);
    if (!(amt > 0.0f)) return;
    s32 idx = *(s32*)(p + 0x14) - 1;
    u32 color = 0xFFFFFFu | ((u32)(s32)(255.0f * (amt * *(f32*)(p + 0x10))) << 24);

    f32 m[12];
    f32 s = *(f32*)(p + 0xC);
    m[0] = 1.7777778f * s;
    m[1] = 0.0f; m[2] = 0.0f; m[3] = 0.0f;
    m[4] = 1.0f * s;
    m[5] = 0.0f; m[6] = 0.0f; m[7] = 0.0f;
    m[8] = 1.0f * s;
    m[9] = 0.0f; m[10] = 0.0f; m[11] = 0.0f;
    fn_801451A0((u8*)m, 0.63661975f * *(f32*)(p + 0x8));
    m[0] *= 0.5625f;
    m[4] *= 1.0f;
    m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;

    u8* slot = mgr + idx * 4;
    u32* col = NULL;
    u8* verts = NULL;
    fn_80156CDC(idx, *(s32*)(slot + 0x3608) == 0, &verts, &col);
    ApplyMtx(m, verts);
    for (s32 i = 0; i < 0x240; i++) col[i] = color;
    fn_80156F3C(idx);
    *(s32*)(slot + 0x3608) = 1;
}

}  // extern "C"
