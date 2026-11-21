// Renderer matrix load and the colour-ramp (fade) object: 0x801561C0, 0x8015ECA8, 0x8015F224,
// 0x8015F4CC.
#include <math.h>
#include "../engine/rend_dl_inline.h"

extern "C" {
void fn_80054840(u8* out, u8* in);                       // 0x80054840 (copy float)
void fn_800544CC(u8* out);                               // 0x800544CC (out = 1.0f)
f32  fn_800544B8(f32* out, f32* a, f32* b);              // 0x800544B8 (out = a - b)
void fn_800636A0(void);                                  // 0x800636A0 (no-op scalar ctor)
void ScalarMul(f32* out, const f32* a, const f32* b);    // 0x80058214
void ScalarSet(f32* out, f32 v);                         // 0x80053250
void fn_801467D4(void);                                  // 0x801467D4 (no-op vec4 ctor)
u32* fn_80156F70(s32 table, s32 enable);                 // 0x80156F70 (returns the colour table)
void fn_80157320(void);                                  // 0x80157320 (empty)
f32  fn_8015EC78(u8* out, u8* in);                       // 0x8015EC78 (copy vec4)
void fn_8015EF64(u8* p);                                 // 0x8015EF64
f32  fn_8015FD0C(u8* out, u8* in);                       // 0x8015FD0C (copy vec4)
void fn_8015FD30(u8* out, u8* in);                       // 0x8015FD30 (splat float)
void fn_8015FD48(f32* out, s32 v);                       // 0x8015FD48 (int to float)
f64  fn_8015FD74(u8* out, u8* in);                       // 0x8015FD74 (clamp to 0..1)
void fn_8015FDA4(u8* out, u8* a, u8* b);                 // 0x8015FDA4 (vec4 add)
void fn_8015FDE8(u8* out, u8* v, f32* s);                // 0x8015FDE8 (vec4 * scalar)
void fn_8015FE20(u8* out, u8* a, u8* b);                 // 0x8015FE20 (vec4 multiply)
void fn_8015FE64(u8* out, u8* a, u8* b);                 // 0x8015FE64 (vec4 min)
void fn_8015FED8(u8* out, u8* a, u8* b);                 // 0x8015FED8 (vec4 sub)
void fn_8015FF1C(u8* out, u8* a, u8* b);                 // 0x8015FF1C (vec4 max)
void fn_8015FF90(u8* in, s32* out);                      // 0x8015FF90 (vec4 to 4 ints, truncating)
void fn_8015FFEC(u8* a, u8* b);                          // 0x8015FFEC (a += b, vec4)
f32  fn_80160030(u8* out, u8* in);                       // 0x80160030 (out = sum of the 4 lanes)
void fn_80160054(u8* p);                                 // 0x80160054
f64  fn_8033F20C(f64 x, f64 y);                          // 0x8033F20C (pow)

extern u8  lbl_80376700[];       // 0x80376700 (vec4 constant)
extern u8  lbl_80400FF0[];       // 0x80400FF0 (renderer matrix block)
extern f32 lbl_805FB510;         // 0x805FB510 (current model scale)

// 0x801561C0
// Sets the renderer's model matrix to a uniform scale (once per distinct value), combines it with
// the current view matrix into the position matrix and loads that into GX.
void fn_801561C0(f32 scale) {
    if (lbl_805FB510 == scale) return;
    f32* model = (f32*)(lbl_80400FF0 + 0x58);
    f32* comb = (f32*)(lbl_80400FF0 + 0x88);
    f32* gx = (f32*)(lbl_80400FF0 + 0x10);
    const f32* in = (const f32*)(lbl_80400FF0 + 0xE8);

    lbl_805FB510 = scale;
    model[0] = scale;
    model[1] = 0.0f;
    model[2] = 0.0f;
    model[3] = 0.0f;
    model[4] = scale;
    model[5] = 0.0f;
    model[6] = 0.0f;
    model[7] = 0.0f;
    model[8] = scale;
    model[9] = 0.0f;
    model[10] = 0.0f;
    model[11] = 0.0f;

    f32 c[12];
    for (s32 j = 0; j < 4; j++) {
        for (s32 i = 0; i < 3; i++) {
            f32 v = in[6 + i] * model[3 * j + 2] + (in[i] * model[3 * j] + in[3 + i] * model[3 * j + 1]);
            if (j == 3) {
                v = in[9 + i] + v;
            }
            c[3 * j + i] = v;
        }
    }
    for (s32 i = 0; i < 12; i++) {
        comb[i] = c[i];
    }

    EnsureDLRoom(0x10FC);

    for (s32 row = 0; row < 3; row++) {
        for (s32 col = 0; col < 4; col++) {
            gx[row * 4 + col] = comb[col * 3 + row];
        }
    }
    XfHead(0xB, 0);
    for (s32 i = 0; i < 12; i++) {
        GXWGFifo._f32 = gx[i];
    }
}

// 0x8015ECA8
// Copies the fade object's saved settings block (0x00..0x78, with its two flag bytes) into the
// working copy at +0xA0.., and 9 floats from +0x7C into +0x11C; +0x9C also goes to +0x13C.
void fn_8015ECA8(u8* p) {
    for (s32 i = 0; i < 8; i++) {
        *(f32*)(p + 0x11C + i * 4) = *(f32*)(p + 0x7C + i * 4);
    }
    f32 last = *(f32*)(p + 0x9C);
    *(u8*)(p + 0xA0) = *(u8*)(p + 0x0);
    *(u8*)(p + 0xA1) = *(u8*)(p + 0x1);
    for (s32 i = 0; i < 30; i++) {
        *(f32*)(p + 0xA4 + i * 4) = *(f32*)(p + 0x4 + i * 4);
    }
    *(f32*)(p + 0x13C) = last;
}

// 0x8015F224
// Per-frame update of the colour-ramp fade: clamps its parameters, derives the target level
// (+0x150) from the ramp state and moves the current level (+0x14C) towards it by at most
// dt * rate per call.
void fn_8015F224(u8* p, f32 dt) {
    f32 v;

    v = *(f32*)(p + 0xA4);
    if (v < 0.1f) v = 0.1f;
    else if (v > 10.0f) v = 10.0f;
    *(f32*)(p + 0xA4) = v;

    v = *(f32*)(p + 0xC4);
    if (v < 0.1f) v = 0.1f;
    else if (v > 10.0f) v = 10.0f;
    *(f32*)(p + 0xC4) = v;

    v = *(f32*)(p + 0xAC);
    if (v < 1.0f / 48.0f) v = 1.0f / 48.0f;
    else if (v > 1.0f) v = 1.0f;
    *(f32*)(p + 0xAC) = v;

    v = *(f32*)(p + 0xB0);
    if (v < 1.0f / 36.0f) v = 1.0f / 36.0f;
    else if (v > 1.0f) v = 1.0f;
    *(f32*)(p + 0xB0) = v;

    if (*(u8*)(p + 0xA1) == 0) {
        *(f32*)(p + 0x150) = 1.0f;
        *(f32*)(p + 0x14C) = 1.0f;
        return;
    }
    fn_8015EF64(p);

    f32 t = 2.0f * *(f32*)(p + 0x154);
    *(f32*)(p + 0x150) = t;
    f32 a = *(f32*)(p + 0xBC);
    f32 b = *(f32*)(p + 0xC0);
    f32 lo = a;
    f32 hi = b;
    if (!(a < b)) {
        lo = b;
        hi = a;
    }
    if (t < lo) t = lo;
    else if (t > hi) t = hi;
    *(f32*)(p + 0x150) = t;

    t = t * (f32)fn_8033F20C(2.0, (f64)-*(f32*)(p + 0xA8));
    *(f32*)(p + 0x150) = t;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    *(f32*)(p + 0x150) = t;

    if (*(s32*)(p + 0x140) != 0) {
        *(s32*)(p + 0x140) = 0;
        *(f32*)(p + 0x14C) = t;
        return;
    }
    s32 moving = *(s32*)(p + 0x144);
    f32 d = (f32)fabs(t - *(f32*)(p + 0x14C));
    if (moving != 0 || d > *(f32*)(p + 0xB8)) {
        f32 lim = dt * *(f32*)(p + 0xB4);
        if (d >= lim) d = lim;
        if (*(f32*)(p + 0x14C) > *(f32*)(p + 0x150)) d = -d;
        if (moving != 0) {
            if (*(f32*)(p + 0x148) * d <= 0.0f) {
                *(s32*)(p + 0x144) = 0;
                *(f32*)(p + 0x14C) = *(f32*)(p + 0x150);
                return;
            }
            *(f32*)(p + 0x148) = d;
            *(f32*)(p + 0x14C) = *(f32*)(p + 0x14C) + d;
            return;
        }
        if (d != 0.0f) {
            *(f32*)(p + 0x148) = d;
            *(s32*)(p + 0x144) = 1;
            *(f32*)(p + 0x14C) = *(f32*)(p + 0x14C) + d;
        }
    }
}

// Packs three colour-table bytes (tables at +0x15C, +0x55C, +0x95C) into one 0xFFBBGGRR word.
static inline u32 PackRamp(u8* p, const s32* idx) {
    return (u32)p[0x15C + idx[0]] | ((u32)p[0x55C + idx[1]] << 8) | ((u32)p[0x95C + idx[2]] << 16) | 0xFF000000u;
}

// 0x8015F4CC
// Builds the three 256-entry colour tables of the fade: for each of the 256 steps the colour is
// ramped from the current colour towards the target and quantised through the object's lookup
// tables. When the object is disabled the tables are reset to their plain ramps instead.
void fn_8015F4CC(u8* p) {
    f32 v3F0[4], v3E0[4], v3D0[4], v3C0[4], v3B0[4], v3A0[4];
    f32 v390[4], v380[4], v370[4], v360[4], v350[4], v340[4];
    f32 v330[4], v320[4], v310[4], v300[4], v2F0[4], v2E0[4];
    s32 i2D0[4], i2C0[4], i2B0[4];
    f32 tmp[4], tmp2[4], tmp3[4];
    f32 s38, s1C, s14, s18, s34, s10, sC, s30, s2C, s28, s24, s8;

    if (*(u8*)(p + 0xA0) == 0) {
        fn_80156F70(0, 1);
        fn_80157320();
        fn_80156F70(1, 1);
        fn_80157320();
        fn_80156F70(2, 1);
        fn_80157320();
        return;
    }
    fn_80160054(p);
    fn_8015FD0C((u8*)v3F0, lbl_80376700);
    fn_8015FD48(&s1C, 0x3FF);
    fn_8015FD30((u8*)v3E0, (u8*)&s1C);
    fn_801467D4();

    f32 lo = *(f32*)(p + 0xBC);
    f32 range = *(f32*)(p + 0xC0) - lo;
    if (range > 0.0f) {
        ScalarSet(&s38, (*(f32*)(p + 0x14C) - lo) / range);
        fn_8015FD74((u8*)&s1C, (u8*)&s38);
        fn_80054840((u8*)&s38, (u8*)&s1C);
        fn_8015FDE8((u8*)tmp, p + 0xF0, &s38);
        fn_800544CC((u8*)&s14);
        fn_800544B8(&s18, &s14, &s38);
        fn_8015FDE8((u8*)tmp2, p + 0xE0, &s18);
        fn_8015FDA4((u8*)tmp3, (u8*)tmp2, (u8*)tmp);
        fn_8015EC78((u8*)v3D0, (u8*)tmp3);
    } else {
        fn_8015EC78((u8*)v3D0, p + 0xF0);
    }
    u32* t0 = fn_80156F70(0, 0);
    u32* t1 = fn_80156F70(1, 0);
    u32* t2 = fn_80156F70(2, 0);

    fn_8015FE20((u8*)tmp, p + 0x100, (u8*)v3D0);
    fn_8015FE20((u8*)v3C0, (u8*)tmp, (u8*)v3E0);
    fn_8015FE20((u8*)tmp, p + 0x110, (u8*)v3D0);
    fn_8015FE20((u8*)v3B0, (u8*)tmp, (u8*)v3E0);
    fn_8015FE20((u8*)tmp, p + 0x120, (u8*)v3D0);
    fn_8015FE20((u8*)v3A0, (u8*)tmp, (u8*)v3E0);
    fn_801467D4();
    fn_801467D4();
    fn_801467D4();
    fn_801467D4();
    fn_801467D4();
    fn_801467D4();

    fn_8015FE20((u8*)v330, (u8*)v3C0, p + 0x130);
    fn_8015FE64((u8*)tmp, (u8*)v330, (u8*)v3E0);
    fn_8015EC78((u8*)v330, (u8*)tmp);
    fn_8015FE20((u8*)v320, (u8*)v3B0, p + 0x130);
    fn_8015FE64((u8*)tmp, (u8*)v320, (u8*)v3E0);
    fn_8015EC78((u8*)v320, (u8*)tmp);
    fn_8015FE20((u8*)v310, (u8*)v3A0, p + 0x130);
    fn_8015FE64((u8*)tmp, (u8*)v310, (u8*)v3E0);
    fn_8015EC78((u8*)v310, (u8*)tmp);
    fn_800636A0();

    s32 i = 0;
    f32 scaled = 255.0f * *(f32*)(p + 0x14C);
    s32 n = (s32)scaled;
    if (n > 0) {
        ScalarSet(&s10, 1.0f / scaled);
        fn_80054840((u8*)&s34, (u8*)&s10);
        fn_8015FE20((u8*)tmp, p + 0xD0, (u8*)v3E0);
        fn_8015EC78((u8*)v390, (u8*)tmp);
        fn_8015FE20((u8*)tmp, p + 0xD0, (u8*)v3E0);
        fn_8015EC78((u8*)v380, (u8*)tmp);
        fn_8015FE20((u8*)tmp, p + 0xD0, (u8*)v3E0);
        fn_8015EC78((u8*)v370, (u8*)tmp);
        fn_8015FED8((u8*)tmp, (u8*)v3C0, (u8*)v390);
        fn_8015FDE8((u8*)tmp2, (u8*)tmp, &s34);
        fn_8015EC78((u8*)v360, (u8*)tmp2);
        fn_8015FED8((u8*)tmp, (u8*)v3B0, (u8*)v380);
        fn_8015FDE8((u8*)tmp2, (u8*)tmp, &s34);
        fn_8015EC78((u8*)v350, (u8*)tmp2);
        fn_8015FED8((u8*)tmp, (u8*)v3A0, (u8*)v370);
        fn_8015FDE8((u8*)tmp2, (u8*)tmp, &s34);
        fn_8015EC78((u8*)v340, (u8*)tmp2);
        u32* d0 = t0;
        u32* d1 = t1;
        u32* d2 = t2;
        while (i < n) {
            fn_8015FE64((u8*)v300, (u8*)v390, (u8*)v3E0);
            fn_8015FE64((u8*)v2F0, (u8*)v380, (u8*)v3E0);
            fn_8015FE64((u8*)v2E0, (u8*)v370, (u8*)v3E0);
            fn_8015FF1C((u8*)tmp, (u8*)v300, (u8*)v330);
            fn_8015EC78((u8*)v300, (u8*)tmp);
            fn_8015FF1C((u8*)tmp, (u8*)v2F0, (u8*)v320);
            fn_8015EC78((u8*)v2F0, (u8*)tmp);
            fn_8015FF1C((u8*)tmp, (u8*)v2E0, (u8*)v310);
            fn_8015EC78((u8*)v2E0, (u8*)tmp);
            fn_8015FF90((u8*)v300, i2D0);
            fn_8015FF90((u8*)v2F0, i2C0);
            fn_8015FF90((u8*)v2E0, i2B0);
            *d0 = PackRamp(p, i2D0);
            *d1 = PackRamp(p, i2C0);
            *d2 = PackRamp(p, i2B0);
            fn_8015FFEC((u8*)v390, (u8*)v360);
            fn_8015FFEC((u8*)v380, (u8*)v350);
            fn_8015FFEC((u8*)v370, (u8*)v340);
            d0++;
            d1++;
            d2++;
            i++;
        }
    } else {
        fn_800544CC((u8*)&sC);
        fn_80054840((u8*)&s34, (u8*)&sC);
    }

    fn_8015EC78((u8*)v390, (u8*)v3C0);
    fn_8015EC78((u8*)v380, (u8*)v3B0);
    fn_8015EC78((u8*)v370, (u8*)v3A0);
    fn_8015FE20((u8*)tmp, (u8*)v3C0, (u8*)v3F0);
    fn_80160030((u8*)&s30, (u8*)tmp);
    fn_8015FE20((u8*)tmp, (u8*)v3B0, (u8*)v3F0);
    fn_80160030((u8*)&s2C, (u8*)tmp);
    fn_8015FE20((u8*)tmp, (u8*)v3A0, (u8*)v3F0);
    fn_80160030((u8*)&s28, (u8*)tmp);
    ScalarSet(&s8, *(f32*)(p + 0xCC));
    ScalarMul(&s24, &s34, &s8);
    fn_8015FDE8((u8*)tmp, (u8*)v3D0, &s30);
    fn_8015FDE8((u8*)tmp2, (u8*)tmp, &s24);
    fn_8015EC78((u8*)v360, (u8*)tmp2);
    fn_8015FDE8((u8*)tmp, (u8*)v3D0, &s2C);
    fn_8015FDE8((u8*)tmp2, (u8*)tmp, &s24);
    fn_8015EC78((u8*)v350, (u8*)tmp2);
    fn_8015FDE8((u8*)tmp, (u8*)v3D0, &s28);
    fn_8015FDE8((u8*)tmp2, (u8*)tmp, &s24);
    fn_8015EC78((u8*)v340, (u8*)tmp2);

    u32* e0 = t0 + i;
    u32* e1 = t1 + i;
    u32* e2 = t2 + i;
    while (i < 0x100) {
        fn_8015FE64((u8*)v300, (u8*)v390, (u8*)v3E0);
        fn_8015FE64((u8*)v2F0, (u8*)v380, (u8*)v3E0);
        fn_8015FE64((u8*)v2E0, (u8*)v370, (u8*)v3E0);
        fn_8015FF1C((u8*)tmp, (u8*)v300, (u8*)v330);
        fn_8015EC78((u8*)v300, (u8*)tmp);
        fn_8015FF1C((u8*)tmp, (u8*)v2F0, (u8*)v320);
        fn_8015EC78((u8*)v2F0, (u8*)tmp);
        fn_8015FF1C((u8*)tmp, (u8*)v2E0, (u8*)v310);
        fn_8015EC78((u8*)v2E0, (u8*)tmp);
        fn_8015FF90((u8*)v300, i2D0);
        fn_8015FF90((u8*)v2F0, i2C0);
        fn_8015FF90((u8*)v2E0, i2B0);
        *e0 = PackRamp(p, i2D0);
        *e1 = PackRamp(p, i2C0);
        *e2 = PackRamp(p, i2B0);
        fn_8015FFEC((u8*)v390, (u8*)v360);
        fn_8015FFEC((u8*)v380, (u8*)v350);
        fn_8015FFEC((u8*)v370, (u8*)v340);
        e0++;
        e1++;
        e2++;
        i++;
    }
    fn_80157320();
    fn_80157320();
    fn_80157320();
}

}  // extern "C"
