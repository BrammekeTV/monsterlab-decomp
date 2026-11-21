// Misc functions: quaternion product, centered text print, ray test against zone entities,
// recursive 5-level tree update, random vector fills.
#include "types.h"

extern "C" {
u8*  GetGlobalSettings(void);                                    // 0x801D2BF8
u32  RandomU32(void);                                            // 0x80143460
void fn_80188EDC(const char* text, s32* w, s32* h);              // 0x80188EDC
void fn_80188F00(const char* fmt, ...);                          // 0x80188F00
s32  fn_80139514(void* a, void* b);                              // 0x80139514
void fn_8016CA24(void* a, void* query, void* cb, void* result);  // 0x8016CA24

extern u8  gWorldState[];       // 0x803E6D00
extern u8  lbl_80412BE0[];      // 0x80412BE0 (screen size: s32 w, s32 h)
extern s32 lbl_805FB30C;        // 0x805FB30C (entity slot count)
extern s32 lbl_805FB330;        // 0x805FB330
extern s32 lbl_805FB628;        // 0x805FB628
extern s32 lbl_805FB62C;        // 0x805FB62C
extern s32 lbl_805FB630;        // 0x805FB630

typedef void (*VUpdateFn)(u8* obj, f32 dt);

// 0x80054650
// Quaternion-style product: out = q * (v, 0) * conj(q) (rotates v by q), writes xyz of the result.
void fn_80054650(u8* out, u8* q, u8* v) {
    f32 qx = *(f32*)(q + 0x0);
    f32 qy = *(f32*)(q + 0x4);
    f32 qz = *(f32*)(q + 0x8);
    f32 qw = *(f32*)(q + 0xC);
    f32 vx = *(f32*)(v + 0x0);
    f32 vy = *(f32*)(v + 0x4);
    f32 vz = *(f32*)(v + 0x8);
    f32 nx = -qx;
    f32 ny = -qy;
    f32 nz = -qz;

    f32 tx = ((qy * vz) + ((qw * vx) + (qx * 0.0f))) - (qz * vy);
    f32 tw = (((qw * 0.0f) - (qx * vx)) - (qy * vy)) - (qz * vz);
    f32 ty = (qz * vx) + ((qy * 0.0f) + ((qw * vy) - (qx * vz)));
    f32 tz = (qz * 0.0f) + (((qw * vz) + (qx * vy)) - (qy * vx));

    *(f32*)(out + 0x0) = ((ty * nz) + ((tw * nx) + (tx * qw))) - (tz * ny);
    *(f32*)(out + 0x4) = (tz * nx) + ((ty * qw) + ((tw * ny) - (tx * nz)));
    *(f32*)(out + 0x8) = (tz * qw) + (((tw * nz) + (tx * ny)) - (ty * nx));
}

// 0x80121B08
// Prints the text at p+0 on screen at a position relative to the screen centre (p+4/p+8 scaled by
// 1.6 * half the screen width), aligned by p+0x18 (1 = centre, 2 = right), colour/param p+0x14.
void fn_80121B08(u8* p) {
    s32 h;
    s32 w;
    f32 half = (f32)*(s32*)(lbl_80412BE0 + 0x0) * 0.5f;
    f32 scale = 1.6f * half;
    f32 x = half + (*(f32*)(p + 0x4) * scale);
    f32 y = ((f32)*(s32*)(lbl_80412BE0 + 0x4) * 0.5f) + (*(f32*)(p + 0x8) * -scale);
    fn_80188EDC(*(const char**)(p + 0x0), &w, &h);
    switch (*(s32*)(p + 0x18)) {
    case 1:
        x -= (f32)((w + (s32)((u32)w >> 31)) >> 1);
        break;
    case 2:
        x -= (f32)w;
        break;
    }
    lbl_805FB628 = (s32)x;
    lbl_805FB62C = (s32)y;
    lbl_805FB630 = *(s32*)(p + 0x14);
    fn_80188F00("%s", *(const char**)(p + 0x0));
}

// 0x80139684
// Casts a segment from arg1 (length scale dt) against every live entity of kind 6 and writes the
// nearest hit into the result record arg2 (hit data, entity, scaled distance at +0x18).
void fn_80139684(u8* unused, f32* from, u8* result, f32 dt) {
    f32 query[4];
    struct {
        f32 hit[7];     // 0x00..0x18: hit data, 0x18 = fraction (starts at 1.1)
        u8  pad[0x2C];
        s32 field48;
        u8* entity;     // 0x4C
        s32 in50;
        s32 in54;
    } res;
    s32 i;

    for (i = 0; i < (s32)sizeof(res); i += 4) *(s32*)((u8*)&res + i) = 0;
    res.in50 = *(s32*)(result + 0x50);
    res.in54 = *(s32*)(result + 0x54);
    lbl_805FB330 = 0;
    res.hit[6] = 1.1f;

    if (dt > -0.0001f && dt < 0.0001f) return;

    query[0] = from[0];
    query[1] = from[1];
    query[2] = from[2];
    query[3] = dt;

    for (i = 0; i < lbl_805FB30C; i++) {
        u8* ent = (*(u8***)(gWorldState + 0xD4))[i];
        if (ent != NULL) {
            s32 ok = 0;
            if (i >= 0) {
                s32 n = 0;
                u32 off;
                for (off = 0x34; off <= 0x64; off += 4) n += *(s32*)(GetGlobalSettings() + off);
                if (i < n) ok = 1;
            }
            if (ok && *(s32*)(*(u8**)(gWorldState + 0xB0) + i * 0x30 + 4) == 6) {
                u8* e = (*(u8***)(gWorldState + 0xD4))[i];
                u8* sub = *(u8**)(*(u8**)(e + 0x4C) + 0x1C);
                if (sub != NULL) {
                    res.entity = e;
                    fn_8016CA24(sub, query, (void*)fn_80139514, &res);
                    if (res.hit[6] < 1.1f && result != NULL) {
                        *(f32*)(result + 0x0)  = res.hit[0];
                        *(f32*)(result + 0x4)  = res.hit[1];
                        *(f32*)(result + 0x8)  = res.hit[2];
                        *(f32*)(result + 0xC)  = res.hit[3];
                        *(f32*)(result + 0x10) = res.hit[4];
                        *(f32*)(result + 0x14) = res.hit[5];
                        *(f32*)(result + 0x18) = res.hit[6];
                        *(s32*)(result + 0x48) = res.field48;
                        *(f32*)(result + 0x18) = res.hit[6] * dt;
                    }
                }
            }
        }
    }
}

// 0x801B3494
// Updates a 5-level object tree: children first (array at +0xC, count at +0x8C), then the object's
// own virtual update (vtable slot +0xC) with dt.
void fn_801B3494(u8* o0, f32 dt) {
    s32 a, b, c, d;
    for (a = 0; a < *(s32*)(o0 + 0x8C); a++) {
        u8* o1 = *(u8**)(o0 + 0xC + a * 4);
        for (b = 0; b < *(s32*)(o1 + 0x8C); b++) {
            u8* o2 = *(u8**)(o1 + 0xC + b * 4);
            for (c = 0; c < *(s32*)(o2 + 0x8C); c++) {
                u8* o3 = *(u8**)(o2 + 0xC + c * 4);
                for (d = 0; d < *(s32*)(o3 + 0x8C); d++) {
                    fn_801B3494(*(u8**)(o3 + 0xC + d * 4), dt);
                }
                (*(VUpdateFn*)(*(u8**)o3 + 0xC))(o3, dt);
            }
            (*(VUpdateFn*)(*(u8**)o2 + 0xC))(o2, dt);
        }
        (*(VUpdateFn*)(*(u8**)o1 + 0xC))(o1, dt);
    }
    (*(VUpdateFn*)(*(u8**)o0 + 0xC))(o0, dt);
}

// 0x801D022C
// Fills a 4-float vector with uniform randoms in [lo, hi].
void fn_801D022C(u8* out, f32 lo, f32 hi) {
    *(f32*)(out + 0x0) = lo + ((hi - lo) * (2.3283064e-10f * (f32)RandomU32()));
    *(f32*)(out + 0x4) = lo + ((hi - lo) * (2.3283064e-10f * (f32)RandomU32()));
    *(f32*)(out + 0x8) = lo + ((hi - lo) * (2.3283064e-10f * (f32)RandomU32()));
    *(f32*)(out + 0xC) = lo + ((hi - lo) * (2.3283064e-10f * (f32)RandomU32()));
}

// 0x801D0398
// Fills a 4-float vector with uniform randoms in [-r, +r] where r is the matching float of arg1.
void fn_801D0398(u8* out, u8* range) {
    f32 r;
    r = *(f32*)(range + 0x0);
    *(f32*)(out + 0x0) = (r * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - r;
    r = *(f32*)(range + 0x4);
    *(f32*)(out + 0x4) = (r * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - r;
    r = *(f32*)(range + 0x8);
    *(f32*)(out + 0x8) = (r * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - r;
    r = *(f32*)(range + 0xC);
    *(f32*)(out + 0xC) = (r * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - r;
}

}  // extern "C"
