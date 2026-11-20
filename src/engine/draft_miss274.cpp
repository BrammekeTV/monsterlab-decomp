// Engine functions in 0x801Dxxxx/0x801Exxxx: fader overlay draw, timed colour/intensity
// interpolator update, and the pool-handle list reset.
#include "types.h"
#include "revolution.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
u32   RandomU32(void);                                           // 0x80143460
f32   cosf(f32 x);                                               // 0x8033EC98 (MSL)
void  fn_801542DC(void);                                         // 0x801542DC
void  fn_80172E84(u8* obj);                                      // 0x80172E84
void  fn_80195A58(void* gx, void* a, u8* b, u8* c);              // 0x80195A58
s32   fn_8019600C(void* gx);                                     // 0x8019600C
s32   fn_801960E4(void* gx);                                     // 0x801960E4
}

extern u8    lbl_8047D248[];     // 0x8047D248 (fader colour: 4 floats R,G,B,A in 0..1, then GX state blocks)
extern void* lbl_805FB658;       // 0x805FB658 (global render object)
extern u8    lbl_805FB7F0;       // 0x805FB7F0 (fader initialized)
extern u8    lbl_805FB7F8[];     // 0x805FB7F8 (holds a pointer: fader texture/target)
extern u8    lbl_805FB7FC;       // 0x805FB7FC

// Float 0..1 colour channel -> 0..255, truncated and clamped.
static s32 FaderChannel(f32 v) {
    s32 i = (s32)(255.0f * v);
    if (i < 0) return 0;
    if (i > 0xFF) return 0xFF;
    return i;
}

// Fader colour packed as R<<24 | G<<16 | B<<8 | A (the channels are byte-swapped through the stack in the original).
static u32 FaderPackedColor(void) {
    s32 r = FaderChannel(*(f32*)(lbl_8047D248 + 0x0));
    s32 g = FaderChannel(*(f32*)(lbl_8047D248 + 0x4));
    s32 b = FaderChannel(*(f32*)(lbl_8047D248 + 0x8));
    s32 a = FaderChannel(*(f32*)(lbl_8047D248 + 0xC));
    return ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
}

// Clamp to 0..1.
static f32 Saturate(f32 v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

extern "C" {

// 0x801D603C
// Draws the screen fader: when the fader is initialized and its alpha is not ~0, sets up the render
// state, writes the fader colour into the four vertex colour slots and draws it.
void fn_801D603C(void) {
    if (lbl_805FB7F0 == 0 || lbl_805FB7FC == 1) return;
    if (*(void**)lbl_805FB7F8 == NULL) return;
    f32 alpha = *(f32*)(lbl_8047D248 + 0xC);
    if (alpha > -0.0001f && alpha < 0.0001f) return;     // fully transparent: nothing to draw
    fn_8019600C(lbl_805FB658);
    fn_801960E4(lbl_805FB658);
    fn_801542DC();
    u32 col = FaderPackedColor();
    *(u32*)(lbl_8047D248 + 0x18) = col;
    *(u32*)(lbl_8047D248 + 0x30) = col;
    *(u32*)(lbl_8047D248 + 0x48) = col;
    *(u32*)(lbl_8047D248 + 0x60) = col;
    fn_80195A58(lbl_805FB658, *(void**)lbl_805FB7F8, lbl_8047D248 + 0x70, lbl_8047D248 + 0x10);
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00000Fu;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0x80000005u;
}

// 0x801D9B34
// Per-frame update of a timed interpolator (duration at +0xBC, remaining time at +0x4C) that drives
// a target object (+0x48): an RGBA colour (+0x44..+0x50 of the target), an intensity (+0x54..+0x5C) and
// an angle (+0x14/+0x18 = angle, cos(angle)). The current values are at +0x78 (colour), +0xA4 (intensity)
// and +0xB4 (angle); the previous ones at +0x68, +0xA0, +0xB0. When the timer runs out, new random targets
// are picked from the base value +/- the spread; until then the values follow a cosine ease between them.
void fn_801D9B34(u8* arg0, f32 farg0) {
    f32 duration = *(f32*)(arg0 + 0xBC);
    if (!(duration > 0.0f)) return;

    f32 t = *(f32*)(arg0 + 0x4C) - farg0;
    *(f32*)(arg0 + 0x4C) = t;
    u8* tgt;
    f32 f31, f30;

    if (t < 0.0f) {
        // Timer ran out: restart it and pick new random targets.
        f32 sx = *(f32*)(arg0 + 0x88);
        f32 sy = *(f32*)(arg0 + 0x8C);
        f32 sz = *(f32*)(arg0 + 0x90);
        *(f32*)(arg0 + 0x4C) = duration;
        if (!(sx <= 0.0f && sy <= 0.0f && sz <= 0.0f)) {
            f32 x = sx * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - sx;
            f32 y = sy * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - sy;
            f32 z = sz * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - sz;
            f32 sw = *(f32*)(arg0 + 0x94);
            f32 w = sw * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - sw;
            x += *(f32*)(arg0 + 0x58);
            y += *(f32*)(arg0 + 0x5C);
            z += *(f32*)(arg0 + 0x60);
            w += *(f32*)(arg0 + 0x64);
            f32 nx = Saturate(x), ny = Saturate(y), nz = Saturate(z), nw = Saturate(w);
            // shift current colour to previous, store the new one
            f32 ox = *(f32*)(arg0 + 0x78), oy = *(f32*)(arg0 + 0x7C);
            f32 oz = *(f32*)(arg0 + 0x80), ow = *(f32*)(arg0 + 0x84);
            tgt = *(u8**)(arg0 + 0x48);
            *(f32*)(arg0 + 0x68) = ox;
            *(f32*)(arg0 + 0x6C) = oy;
            *(f32*)(arg0 + 0x70) = oz;
            *(f32*)(arg0 + 0x74) = ow;
            *(f32*)(arg0 + 0x78) = nx;
            *(f32*)(arg0 + 0x7C) = ny;
            *(f32*)(arg0 + 0x80) = nz;
            *(f32*)(arg0 + 0x84) = nw;
            *(f32*)(tgt + 0x44) = ox;
            *(f32*)(tgt + 0x48) = oy;
            *(f32*)(tgt + 0x4C) = oz;
            *(f32*)(tgt + 0x50) = ow;
            fn_80172E84(tgt);
        }

        f31 = *(f32*)(arg0 + 0xA8);
        if (f31 > 0.0f) {
            f30 = -f31;
            f32 r = (f32)RandomU32();
            tgt = *(u8**)(arg0 + 0x48);
            f32 prev = *(f32*)(arg0 + 0xA4);
            *(f32*)(arg0 + 0xA0) = prev;
            *(f32*)(arg0 + 0xA4) = *(f32*)(arg0 + 0x9C) + (f30 + ((f31 - f30) * (2.3283064e-10f * r)));
            if (tgt != NULL && prev > 0.0f) {
                f32 rate = 5.0f / prev;
                *(f32*)(tgt + 0x54) = 1.0f;
                *(f32*)(tgt + 0x58) = 0.0f;
                *(f32*)(tgt + 0x5C) = rate;
                fn_80172E84(tgt);
            }
        }

        f31 = *(f32*)(arg0 + 0xB8);
        if (f31 > 0.0f) {
            f30 = -f31;
            f32 r = (f32)RandomU32();
            tgt = *(u8**)(arg0 + 0x48);
            f32 prev = *(f32*)(arg0 + 0xB4);
            *(f32*)(arg0 + 0xB0) = prev;
            *(f32*)(arg0 + 0xB4) = *(f32*)(arg0 + 0xAC) + (f30 + ((f31 - f30) * (2.3283064e-10f * r)));
            if (tgt != NULL) {
                f32 ang = 0.63661975f * prev;
                *(f32*)(tgt + 0x14) = ang;
                *(f32*)(tgt + 0x18) = cosf(1.5707964f * ang);
            }
        }
    } else {
        // Ease from the previous to the current values.
        f32 sx = *(f32*)(arg0 + 0x88);
        f32 sy = *(f32*)(arg0 + 0x8C);
        f32 sz = *(f32*)(arg0 + 0x90);
        if (!(sx <= 0.0f && sy <= 0.0f && sz <= 0.0f)) {
            f32 b = (cosf(1.5707964f * (2.0f * (*(f32*)(arg0 + 0x4C) / *(f32*)(arg0 + 0xBC)))) + 1.0f) * 0.5f;
            f32 ib = 1.0f - b;
            tgt = *(u8**)(arg0 + 0x48);
            *(f32*)(tgt + 0x44) = *(f32*)(arg0 + 0x78) * b + *(f32*)(arg0 + 0x68) * ib;
            *(f32*)(tgt + 0x48) = *(f32*)(arg0 + 0x7C) * b + *(f32*)(arg0 + 0x6C) * ib;
            *(f32*)(tgt + 0x4C) = *(f32*)(arg0 + 0x80) * b + *(f32*)(arg0 + 0x70) * ib;
            *(f32*)(tgt + 0x50) = 0.0f;
            fn_80172E84(tgt);
        }

        if (*(f32*)(arg0 + 0xA8) > 0.0f) {
            f32 a = *(f32*)(arg0 + 0xA0);
            f32 c = *(f32*)(arg0 + 0xA4);
            f32 b = 0.5f * (1.0f + cosf(1.5707964f * (2.0f * (*(f32*)(arg0 + 0x4C) / *(f32*)(arg0 + 0xBC)))));
            tgt = *(u8**)(arg0 + 0x48);
            f32 v = a * (1.0f - b) + c * b;
            if (tgt != NULL && v > 0.0f) {
                *(f32*)(tgt + 0x54) = 1.0f;
                f32 rate = 5.0f / v;
                *(f32*)(tgt + 0x58) = 0.0f;
                *(f32*)(tgt + 0x5C) = rate;
                fn_80172E84(tgt);
            }
        }

        if (*(f32*)(arg0 + 0xB8) > 0.0f) {
            f32 a = *(f32*)(arg0 + 0xB0);
            f32 c = *(f32*)(arg0 + 0xB4);
            f32 b = 0.5f * (1.0f + cosf(1.5707964f * (2.0f * (*(f32*)(arg0 + 0x4C) / *(f32*)(arg0 + 0xBC)))));
            tgt = *(u8**)(arg0 + 0x48);
            f32 v = a * (1.0f - b) + c * b;
            if (tgt != NULL) {
                f32 ang = 0.63661975f * v;
                *(f32*)(tgt + 0x14) = ang;
                *(f32*)(tgt + 0x18) = cosf(1.5707964f * ang);
            }
        }
    }
}

// 0x801E763C
// Resets a pool-handle list header: first/last handles (+0/+2) = 0xFFFF (none), count (+4), flags (+6)
// and +8 = 0; returns the list. The original also inlined walks over the effect pool (lbl_805FB8D8) that
// start from the just-cleared 0xFFFF handle (always invalid) and only read memory, so they have no effect
// and are omitted.
u8* fn_801E763C(u8* arg0) {
    *(u16*)(arg0 + 0x0) = 0xFFFF;
    *(u16*)(arg0 + 0x2) = 0xFFFF;
    *(u16*)(arg0 + 0x6) = 0;
    *(u16*)(arg0 + 0x4) = 0;
    *(u16*)(arg0 + 0x0) = 0xFFFF;
    *(u16*)(arg0 + 0x2) = 0xFFFF;
    *(s32*)(arg0 + 0x8) = 0;
    return arg0;
}

}  // extern "C"
