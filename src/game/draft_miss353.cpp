// Bolt/rope particle effect builders and the effect-spawner init (0x801Bxxxx, 0x801Cxxxx, 0x801Exxxx).
#include <string.h>

#include "types.h"

extern "C" {
#include "game/script_callbacks.h"

u32   RandomU32(void);                                              // 0x80143460
void  MtxFromLocatorRotation(f32 out[12], const void* rot);         // 0x80143CA8
void  ScalarMul(f32* out, const f32* a, const f32* b);              // 0x80058214
void  ScalarSet(f32* out, f32 v);                                   // 0x80053250
void  ScalarZero(f32* out);                                         // 0x80055E2C
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);  // 0x801C80E0
void  Vec3AddTo(Vec3* v, const Vec3* d);                            // 0x800544FC
void  Vec3Copy(Vec3* out, const Vec3* v);                           // 0x80052640
f32   Vec3GetX(const Vec3* v);
f32   Vec3GetY(const Vec3* v);
f32   Vec3GetZ(const Vec3* v);
void  Vec3Length(f32* out, const Vec3* v);                          // 0x80057128
void  Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);        // 0x80053224
void  Vec3Rotate(Vec3* out, const f32 m[9], const Vec3* v);         // 0x8005844C
void  Vec3ScaleBy(Vec3* v, const f32* s);                           // 0x80054A3C
void  Vec3SetUnitZ(Vec3* v);                                        // 0x800549A8
void  fn_80052638(Vec3* v);                                         // 0x80052638 (empty leaf)
void  fn_8005263C(f32* q);                                          // 0x8005263C (quat identity)
void  fn_800526D0(u8* out);                                         // 0x800526D0 (matrix identity)
void  fn_80054484(Vec3* out, const Vec3* a, const Vec3* b);         // 0x80054484
f32   fn_800544B8(f32* out, f32* a, f32* b);                        // 0x800544B8 (out = a - b)
void  fn_800544CC(f32* out);                                        // 0x800544CC (out = 1.0f)
void  fn_80054650(u8* out, u8* q, u8* v);                           // 0x80054650
void  fn_80054840(u8* out, u8* in);                                 // 0x80054840 (copy float)
f32   fn_800548EC(u8* p);                                           // 0x800548EC
s32   fn_80058BE8(const f32* a, const f32* b);                      // 0x80058BE8
s32   fn_80058C00(f32 x);                                           // 0x80058C00
void  fn_800636A0(void);                                            // 0x800636A0 (no-op scalar ctor)
f32   fn_800677D0(f32 lo, f32 hi);                                  // 0x800677D0 (random in range)
u8*   fn_80067844(u8* e);                                           // 0x80067844 (leaf: pointer at +0x5C)
f32   fn_80082BE0(u8* out, u8* v);                                  // 0x80082BE0
void  fn_80082F0C(u8* a3, f32 f1);                                  // 0x80082F0C
f32   fn_80082F20(u8* out, u8* v);                                  // 0x80082F20 (copy Vec3)
void  fn_80082F68(f32* out, f32* in);                               // 0x80082F68
void  fn_8008CE28(u8* vec, f32 v);                                  // 0x8008CE28
void  fn_80139684(u8* world, f32* pos, f32* out, f32 t);            // 0x80139684
s32   fn_801449A4(f32* mat, f32* out);                              // 0x801449A4
void  fn_801458B0(f32* quat, f32* a, f32* b);                       // 0x801458B0
void  fn_801467D4(void);                                            // 0x801467D4 (empty leaf)
void  fn_8014879C(u8* out);                                         // 0x8014879C
f32   fn_8015EC78(u8* out, u8* in);                                 // 0x8015EC78 (copy vec4)
void  fn_8015FD30(u8* out, u8* in);                                 // 0x8015FD30
void  fn_8015FDA4(u8* out, u8* a, u8* b);                           // 0x8015FDA4 (vec4 add)
void  fn_8015FDE8(u8* out, u8* v, f32* s);                          // 0x8015FDE8 (vec4 * scalar)
void  fn_8015FED8(u8* out, u8* a, u8* b);                           // 0x8015FED8 (vec4 sub)
void  fn_8015FFEC(u8* a, u8* b);                                    // 0x8015FFEC (a += b, vec4)
void  fn_8016EB0C(u8* a3, f32 f1, f32 f2);                          // 0x8016EB0C
void  fn_8016EB60(u8* v, u8* w);                                    // 0x8016EB60 (v.w = *w)
void  fn_8016F6F0(u8* out, u8* v);                                  // 0x8016F6F0 (out = v.w)
f32   fn_8016F784(u8* a3, u8* a4, u8* a5);                          // 0x8016F784
u8* fn_801C3EA4(u8* table, s32 key);                              // 0x801C3EA4
void  fn_801C42A8(u8* obj);                                         // 0x801C42A8
void  fn_801C9000(u8* a, u8* b, f32 t);                             // 0x801C9000
f32   fn_801C911C(u8* a, u8* b, f32 t);                             // 0x801C911C
void  fn_801C9DF0(u8* obj, const f32* v, f32* out, f32 s);          // 0x801C9DF0
void  fn_801BD278(u8* bolt, f32* a, f32* b);                        // 0x801BD278
u8*   fn_801CBD28(u8* e);                                           // 0x801CBD28
s32   fn_801CBD38(u8* obj);                                         // 0x801CBD38
u8*   fn_801CC8E0(u8* node);                                        // 0x801CC8E0 (identity leaf)
u8*   fn_801CC8E4(u8* node);                                        // 0x801CC8E4
u8*   fn_801CC8EC(u8* node);                                        // 0x801CC8EC
s32   fn_801CC8F4(u8* obj, s32 bit);                                // 0x801CC8F4
f32   fn_801CD680(f32* a, f32* b);                                  // 0x801CD680 (a += b)
void  fn_801CEC30(u8* v, u8* s);                                    // 0x801CEC30 (vec4 *= *s)
f32   fn_801CE318(u8* proto);                                       // 0x801CE318
f32   fn_801CE320(u8* node);                                        // 0x801CE320
u8*   fn_801CE390(u8* a);                                           // 0x801CE390
u8*   fn_801CE3A0(u8* proto);                                       // 0x801CE3A0
f32   fn_801CE3A8(u8* node);                                        // 0x801CE3A8
u8*   fn_801CE3C8(u8* a);                                           // 0x801CE3C8
void  fn_801CE3D0(u8* a, u8* b);                                    // 0x801CE3D0
u8*   fn_801CE414(u8* proto);                                       // 0x801CE414
u8*   fn_801CE41C(u8* a);                                           // 0x801CE41C
f32   fn_801CE424(u8* obj);                                         // 0x801CE424
u8*   fn_801CE480(u8* proto);                                       // 0x801CE480
u8*   fn_801CE49C(u8* node);                                        // 0x801CE49C
u8*   fn_801CE53C(u8* a);                                           // 0x801CE53C
u8*   fn_801CE544(u8* a);                                           // 0x801CE544
void  fn_801CE764(u8* node, f32 v);                                 // 0x801CE764
u8*   fn_801CFEE4(u8* a);                                           // 0x801CFEE4
u8*   fn_801CFEEC(u8* a);                                           // 0x801CFEEC
void  fn_801CFEF4(u8* out, u8* a, u8* b);                           // 0x801CFEF4 (random vec between a and b)
s32   fn_801D0040(u8* a);                                           // 0x801D0040
u8*   fn_801D0048(u8* a);                                           // 0x801D0048
f32   fn_801D0050(u8* a);                                           // 0x801D0050
f32   fn_801D0058(u8* a, u8* b);                                    // 0x801D0058
u8*   fn_801D007C(u8* a);                                           // 0x801D007C
s32   fn_801D0084(u8* v);                                           // 0x801D0084
u8*   fn_801D010C(u8* a);                                           // 0x801D010C
void  fn_801D0114(u8* v, f32 lo, f32 hi);                           // 0x801D0114
f32   fn_801D0200(u8* a, u8* b);                                    // 0x801D0200
u8*   fn_801D0224(u8* a);                                           // 0x801D0224
void  fn_801D022C(u8* v, f32 lo, f32 hi);                           // 0x801D022C
f32   fn_801D0338(u8* a);                                           // 0x801D0338
u32   fn_801D0340(f32 v);                                           // 0x801D0340 (f32 -> u32 bits)
f32   fn_801D0354(u32 bits);                                        // 0x801D0354 (u32 bits -> f32)
void  fn_801D0368(u8* a, f32 v);                                    // 0x801D0368
f32   fn_801D0370(u8* a);                                           // 0x801D0370
void  fn_801D0378(u8* a, f32 v);                                    // 0x801D0378
f32   fn_801D0380(u8* a);                                           // 0x801D0380
void  fn_801D0388(u8* a, f32 v);                                    // 0x801D0388
s32   fn_801D0390(u8* a);                                           // 0x801D0390
void  fn_801D0398(u8* out, u8* in);                                 // 0x801D0398
u8*   fn_801D04A8(u8* a);                                           // 0x801D04A8
u8*   fn_801D04B0(u8* a);                                           // 0x801D04B0
void  fn_801D04B8(u8* a, f32 v);                                    // 0x801D04B8
s32   fn_801D04C0(u8* p);                                           // 0x801D04C0
u8*   fn_801D0504(u8* a);                                           // 0x801D0504
void  fn_801D050C(u8* a, u8* b);                                    // 0x801D050C
void  fn_801D0518(u8* a, u8* b);                                    // 0x801D0518
void  fn_801D0524(u8* a, u8* b);                                    // 0x801D0524
f32   fn_801D0530(u8* a);                                           // 0x801D0530
f32   fn_801D0538(u8* a, u8* b);                                    // 0x801D0538
f32   fn_801D055C(u8* a, u8* b);                                    // 0x801D055C
void  fn_801D0580(u8* out, u8* a, u8* b, f32 lo, f32 hi);           // 0x801D0580
u8*   fn_801D089C(u8* a);                                           // 0x801D089C
f32   fn_801D08A4(u8* a, u8* b);                                    // 0x801D08A4
u8*   fn_801D08C8(u8* a);                                           // 0x801D08C8
void  fn_801D08D0(u8* a, u8* b);                                    // 0x801D08D0
f32   fn_801D08DC(u8* p, f32 r);                                    // 0x801D08DC
f32   fn_801D0954(u8* a);                                           // 0x801D0954
f32   fn_801D095C(u8* a);                                           // 0x801D095C
void  fn_801D0964(u8* a, f32 v);                                    // 0x801D0964
void  fn_801D096C(u8* a, s32 v);                                    // 0x801D096C
f32   fn_801D0974(u8* a, u8* b);                                    // 0x801D0974
s32   fn_801D0998(u8* a);                                           // 0x801D0998
void  fn_801D09A0(u8* a, s32 v);                                    // 0x801D09A0
s32   fn_801D09A8(u8* a);                                           // 0x801D09A8
void  fn_801D09B0(u8* a, u8* b);                                    // 0x801D09B0
u8*   fn_801D6D78(void);                                            // 0x801D6D78 (camera record)
u8*   fn_801EB94C(void* p);                                         // 0x801EB94C (allocates from the pool in p)
u32   fn_801E3CE0(u32 color, s32 a);                                // 0x801E3CE0 (scales color alpha by (a+1)/256)
void  fn_801E3A30(u8* obj, f32* m);                                 // 0x801E3A30
void  fn_801E3CCC(u8* out, f32 v);                                  // 0x801E3CCC
s32   fn_801E41E4(f32* boxMin, f32* boxMax, u8* shapes, s32 count, f32* out);  // 0x801E41E4
f64   fn_8033F20C(f64 x, f64 y);                                    // 0x8033F20C (pow)

extern u8  gWorldState[];       // 0x803E6D00
extern u8  lbl_803EA0D0[];      // 0x803EA0D0 (array of 0x48-byte collision shapes)
extern f32 lbl_8047C5B8[12];    // 0x8047C5B8 (scratch 3x4 matrix)
extern u8  lbl_8047C7AC[];      // 0x8047C7AC (effect table)
extern s32 lbl_8048A830[];      // 0x8048A830 (16 corner masks, built by fn_801E40DC)
extern u8  lbl_8048A870[];      // 0x8048A870 (54 corner indices, built by fn_801E40DC)
extern s32 lbl_805FB330;        // 0x805FB330
extern s8  lbl_805FB8B0;        // 0x805FB8B0 (init guard)
extern f32 lbl_805FB8B4;        // 0x805FB8B4
extern s8  lbl_805FB8B8;        // 0x805FB8B8 (init guard)
extern f32 lbl_805FB8BC;        // 0x805FB8BC
extern s8  lbl_805FB8C0;        // 0x805FB8C0 (init guard)
extern f32 lbl_805FB8C4;        // 0x805FB8C4
}

// Float whose bits are rebuilt as ((b << 24) & 0x80000000) | b, as the original does inline when
// copying a component of a randomised vector (b = the float's bit pattern).
static f32 RebuildBits(f32 x) {
    u32 a = fn_801D0340(x);
    u32 b = fn_801D0340(x);
    return fn_801D0354(((a << 24) & 0x80000000u) | b);
}

extern "C" {

// 0x801BCD88
// Midpoint displacement of the bolt's points between indices lo and hi (points are 0xC bytes
// apart from +0x4C): the middle point is the average of the ends plus a random offset in
// +-(scale * obj[0x34] * proto[0x4C]), then both halves recurse with a quarter of the scale.
void fn_801BCD88(u8* obj, s32 lo, s32 hi, f32 scale) {
    s32 span = hi - lo;
    s32 mid = lo + ((s32)(((u32)span >> 31) + span) >> 1);
    if (span <= 1) return;

    u8* ph = obj + hi * 0xC;
    u8* pl = obj + lo * 0xC;
    f32 amp = scale * *(f32*)(obj + 0x34) * *(f32*)(*(u8**)obj + 0x4C);
    f32 sx = *(f32*)(pl + 0x4C) + *(f32*)(ph + 0x4C);
    f32 sy = *(f32*)(pl + 0x50) + *(f32*)(ph + 0x50);
    f32 sz = *(f32*)(pl + 0x54) + *(f32*)(ph + 0x54);
    f32 hx = sx * 0.5f;
    f32 hy = sy * 0.5f;
    f32 hz = sz * 0.5f;
    f32 ox = (amp * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - amp;
    f32 oy = (amp * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - amp;
    u8* pm = obj + mid * 0xC;
    f32 r = 2.0f * (2.3283064e-10f * (f32)RandomU32());
    *(f32*)(pm + 0x4C) = hx + ox;
    *(f32*)(pm + 0x50) = hy + oy;
    f32 oz = (amp * r) - amp;
    *(f32*)(pm + 0x54) = hz + oz;
    if (span <= 2) return;

    // Lower half (lo..mid).
    s32 span2 = mid - lo;
    s32 mid2 = lo + ((s32)(((u32)span2 >> 31) + span2) >> 1);
    if (span2 > 1) {
        f32 ax = *(f32*)(pl + 0x4C) + *(f32*)(pm + 0x4C);
        f32 ay = *(f32*)(pl + 0x50) + *(f32*)(pm + 0x50);
        f32 az = *(f32*)(pl + 0x54) + *(f32*)(pm + 0x54);
        f32 amp2 = 0.5f * scale * *(f32*)(obj + 0x34) * *(f32*)(*(u8**)obj + 0x4C);
        f32 bx = ax * 0.5f;
        f32 by = ay * 0.5f;
        f32 bz = az * 0.5f;
        f32 px = (amp2 * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - amp2;
        f32 py = (amp2 * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - amp2;
        u8* pq = obj + mid2 * 0xC;
        f32 r2 = 2.0f * (2.3283064e-10f * (f32)RandomU32());
        *(f32*)(pq + 0x4C) = bx + px;
        *(f32*)(pq + 0x50) = by + py;
        *(f32*)(pq + 0x54) = bz + ((amp2 * r2) - amp2);
        if (span2 > 2) {
            fn_801BCD88(obj, lo, mid2, 0.5f * (0.5f * scale));
            fn_801BCD88(obj, mid2, mid, 0.5f * (0.5f * scale));
        }
    }

    // Upper half (mid..hi).
    s32 span3 = hi - mid;
    s32 mid3 = mid + ((s32)(((u32)span3 >> 31) + span3) >> 1);
    if (span3 > 1) {
        u8* pm2 = obj + mid * 0xC;
        f32 ax = *(f32*)(pm + 0x4C) + *(f32*)(ph + 0x4C);
        f32 ay = *(f32*)(pm2 + 0x50) + *(f32*)(ph + 0x50);
        f32 az = *(f32*)(pm2 + 0x54) + *(f32*)(ph + 0x54);
        f32 amp3 = 0.5f * scale * *(f32*)(obj + 0x34) * *(f32*)(*(u8**)obj + 0x4C);
        f32 bx = ax * 0.5f;
        f32 by = ay * 0.5f;
        f32 bz = az * 0.5f;
        f32 px = (amp3 * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - amp3;
        f32 py = (amp3 * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - amp3;
        u8* pq = obj + mid3 * 0xC;
        f32 r3 = 2.0f * (2.3283064e-10f * (f32)RandomU32());
        *(f32*)(pq + 0x4C) = bx + px;
        *(f32*)(pq + 0x50) = by + py;
        *(f32*)(pq + 0x54) = bz + ((amp3 * r3) - amp3);
        if (span3 > 2) {
            fn_801BCD88(obj, mid, mid3, 0.5f * (0.5f * scale));
            fn_801BCD88(obj, mid3, hi, 0.5f * (0.5f * scale));
        }
    }
}

// 0x801BEDBC
// Constructor of a bolt: sizes the point list from the prototype (2 + sum of 2^i for i below
// proto[0x30], at most 0x21), sets the end points, a color gradient along the 0x30-byte vertices
// (faded ends, 0x7F alpha next to them), and builds up to 6 child bolts (each with a random life)
// while the depth is below proto[0x38]. `parent` is read only for its word at +0x28.
// Returns the bolt.
u8* fn_801BEDBC(u8* obj, u8* proto, f32* from, f32* to, u32 depth, u8* parent) {
    *(u8**)obj = proto;
    *(f32*)(obj + 0x1F4) = 0.0f;
    *(s32*)(obj + 0x38) = 2;
    *(f32*)(obj + 0x1F8) = 0.0f;
    *(s32*)(obj + 0x3C) = 0;
    *(u8*)(obj + 0x44) = 0;
    *(u8*)(obj + 0x45) = 0;
    if (parent != NULL) {
        *(s32*)(obj + 0x1FC) = *(s32*)(parent + 0x28);
    } else {
        *(s32*)(obj + 0x1FC) = 0;
    }

    for (u32 i = 0; i < *(u32*)(proto + 0x30); i++) {
        f32 p = (f32)fn_8033F20C(2.0, (f64)(f32)i);
        *(s32*)(obj + 0x38) = *(s32*)(obj + 0x38) + (s32)p;
    }
    if (*(u32*)(obj + 0x38) > 0x21) {
        *(s32*)(obj + 0x38) = 0x21;
    }

    // Clear the point list.
    {
        u8* q = obj;
        for (u32 i = 0; i < *(u32*)(obj + 0x38); i++) {
            *(f32*)(q + 0x4C) = 0.0f;
            *(f32*)(q + 0x50) = 0.0f;
            *(f32*)(q + 0x54) = 0.0f;
            q += 0xC;
        }
    }
    u32 count = *(u32*)(obj + 0x38);
    *(f32*)(obj + 0x4C) = from[0];
    u8* last = obj + (count - 1) * 0xC;
    *(f32*)(obj + 0x50) = from[1];
    *(f32*)(obj + 0x54) = from[2];
    *(f32*)(last + 0x4C) = to[0];
    *(f32*)(last + 0x50) = to[1];
    *(f32*)(last + 0x54) = to[2];

    // Copy the first camera vector, then the second record's vector, and negate both x parts.
    f32* cam = (f32*)fn_801D6D78();
    *(f32*)(obj + 0x1D8) = cam[0];
    *(f32*)(obj + 0x1DC) = cam[1];
    *(f32*)(obj + 0x1E0) = cam[2];
    cam = (f32*)fn_801D6D78();
    *(f32*)(obj + 0x1E4) = cam[3];
    *(f32*)(obj + 0x1E8) = cam[4];
    *(f32*)(obj + 0x1EC) = cam[5];
    *(f32*)(obj + 0x1D8) = -1.0f * *(f32*)(obj + 0x1D8);
    *(f32*)(obj + 0x1E4) = -1.0f * cam[3];

    // Vertex list (0x30 bytes each): position along the bolt in +0/+0x18, color in +8/+0x20.
    u32 n = *(u32*)(obj + 0x38);
    for (u32 i = 0; i < n; i++) {
        f32 t = (f32)i / (f32)(n - 1);
        u8* v = *(u8**)(obj + 0x48) + i * 0x30;
        u32 col = ((u32)proto[5] << 16) | ((u32)proto[4] << 24) | ((u32)proto[6] << 8) | (u32)proto[7];
        *(u32*)(v + 0x8) = col;
        *(u32*)(v + 0x20) = col;
        *(f32*)(v + 0x0) = t;
        *(f32*)(v + 0x4) = 0.0f;
        *(f32*)(v + 0x18) = t;
        *(f32*)(v + 0x1C) = 1.0f;
    }
    u32 faded = ((u32)proto[4] << 24) | ((u32)proto[5] << 16) | ((u32)proto[6] << 8);
    u32 half = faded | 0x7F;
    u8* verts = *(u8**)(obj + 0x48);
    *(u32*)(verts + 0x8) = faded;
    *(u32*)(verts + 0x20) = faded;
    verts = *(u8**)(obj + 0x48) + (*(s32*)(obj + 0x38) - 1) * 0x30;
    *(u32*)(verts + 0x8) = faded;
    *(u32*)(verts + 0x20) = faded;
    verts = *(u8**)(obj + 0x48);
    *(u32*)(verts + 0x38) = half;
    *(u32*)(verts + 0x50) = half;
    verts = *(u8**)(obj + 0x48) + (*(s32*)(obj + 0x38) - 2) * 0x30;
    *(u32*)(verts + 0x8) = half;
    *(u32*)(verts + 0x20) = half;

    *(u32*)(obj + 0x1F0) = depth;
    if (depth < *(u32*)(proto + 0x38) && *(s32*)(proto + 0x34) != 0) {
        u8* slot = obj;
        for (u32 i = 0; i < 6; i++) {
            if (i < (u32)(*(s32*)(proto + 0x34) - (*(s32*)(obj + 0x1F0) - 1))) {
                u8* child = NULL;
                if (proto != NULL) {
                    if (*(s32*)(proto + 0x60) != 0) {
                        child = NULL;
                    } else {
                        child = fn_801EB94C(*(void**)(proto + 0x68));
                        if (child != NULL) {
                            fn_801BEDBC(child, proto, from, to, depth + 1, NULL);
                            fn_801BD278(child, from, to);
                        }
                    }
                }
                *(u8**)(slot + 0x4) = child;
                if (child != NULL) {
                    u32 pts = *(u32*)(obj + 0x38);
                    f32 hi = 0.75f * (f32)pts;
                    f32 lo = 1.0f + (0.25f * (f32)pts);
                    *(s32*)(slot + 0x1C) = (s32)(lo + ((hi - lo) * (2.3283064e-10f * (f32)RandomU32())));
                } else {
                    *(s32*)(slot + 0x1C) = -1;
                }
            } else {
                *(u8**)(slot + 0x4) = NULL;
                *(s32*)(slot + 0x1C) = -1;
            }
            slot += 4;
        }
    } else {
        for (u32 i = 0; i < 6; i++) {
            *(u8**)(obj + 0x4 + i * 4) = NULL;
            *(s32*)(obj + 0x1C + i * 4) = -1;
        }
    }
    return obj;
}

// 0x801CEC68
// Initialises a freshly spawned particle `p` from its emitter `em` (proto = em's definition
// record via fn_80067844). Picks the start position/orientation by the proto's shape
// (kind 0/1: random direction around a cone/sphere, kind 2: box/point sources), derives
// velocity, color, size, life and spin from random ranges in the proto, and starts the linked
// effect (proto[0x1E8]) and sound-like object (proto[0x1EC]) if any. `fxMgr` is the effect manager.
void fn_801CEC68(u8* p, u8* em, f32* basePos, u8* fxMgr) {
    f32 sp370[16], sp340[16];
    f32 sp330[4], sp320[4], sp310[4], sp300[4];
    f32 sp2F0[4], sp2E0[4], sp2D0[4], sp2C0[4], sp2B0[4];
    f32 sp2A0[4], sp290[4], sp280[4];
    f32 sp270[4], sp260[4], sp250[4], sp240[4], sp230[4], sp220[4], sp210[4], sp200[4];
    f32 sp1F0[4], sp1E4[4], sp1D8[4], sp1CC[4], sp1C0[4], sp1B0[4], sp1A4[4], sp198[4];
    f32 sp188[4], sp17C[4], sp170[4], sp160[4], sp150[4], sp140[4], sp130[4], sp124[4];
    f32 sp118[4], sp108[4], spF8[4], spE8[4], spD8[4], spC8[4], spB8[4], spA8[4];
    f32 sp98[4], sp90, sp8C, sp88, sp84, sp80, sp7C, sp78, sp74, sp70, sp6C, sp68, sp64;
    f32 sp60, sp5C, sp58, sp54, sp50, sp4C, sp48, sp44, sp40, sp3C, sp38, sp34, sp30;
    f32 sp2C, sp28, sp24, sp20, sp1C, sp18, sp14, sp10, spC, sp8;

    u8* proto = fn_80067844(em);
    fn_800636A0();
    fn_801467D4();
    fn_801467D4();
    u8* defA = fn_801CFEE4(proto);
    u8* defB = fn_801CFEEC(proto);
    fn_801467D4();
    fn_800636A0();
    fn_80082F68(sp250, (f32*)defB);
    fn_801CFEF4((u8*)sp310, (u8*)sp250, defB);
    fn_8015FFEC((u8*)sp310, defA);

    switch (fn_801D0040(proto)) {
    case 1: {
        f32 f30 = fn_801D0050(proto);
        f30 = fn_800677D0(-fn_801D0050(proto), f30);
        ScalarSet(&sp70, *(f32*)fn_801D0048(proto) + f30);
        fn_80054840((u8*)&sp90, (u8*)&sp70);
        if (fn_801CC8F4(proto, 0) != 0) {
            fn_801D0058(p, fn_801D007C(*(u8**)(em + 0x5C)));
            if (fn_801D0084(fn_801D010C(*(u8**)(em + 0x5C))) == 0) {
                fn_8015FED8((u8*)sp240, (u8*)basePos, fn_801CBD28(em));
                fn_801D0058(p, (u8*)sp240);
            }
        } else {
            fn_801D0058(p, (u8*)basePos);
        }
        fn_80052638((Vec3*)sp300);
        fn_801D0114((u8*)sp300, -1.0f, 1.0f);
        fn_80082BE0((u8*)sp230, (u8*)sp300);
        fn_8015EC78((u8*)sp330, (u8*)sp230);
        fn_801D0200(p, (u8*)sp330);
        Vec3Length(&sp88, (Vec3*)fn_801CE390(p));
        ScalarSet(&sp6C, 0.001f);
        if (fn_80058BE8(&sp88, &sp6C) != 0) {
            fn_8016F784((u8*)&sp68, (u8*)&sp90, (u8*)&sp88);
            fn_801CEC30(fn_801CE390(p), (u8*)&sp68);
        } else {
            Vec3SetUnitZ((Vec3*)sp200);
            fn_80082BE0((u8*)sp210, (u8*)sp200);
            fn_8015FDE8((u8*)sp220, (u8*)sp210, &sp90);
            fn_8015EC78(fn_801CE390(p), (u8*)sp220);
        }
        break;
    }
    case 0: {
        f32 f30 = fn_801D0050(proto);
        f30 = fn_800677D0(-fn_801D0050(proto), f30);
        ScalarSet(&sp64, *(f32*)fn_801D0048(proto) + f30);
        fn_80054840((u8*)&sp90, (u8*)&sp64);
        if (fn_801CC8F4(proto, 0) != 0) {
            fn_801D0058(p, fn_801D007C(*(u8**)(em + 0x5C)));
            if (fn_801D0084(fn_801D010C(*(u8**)(em + 0x5C))) == 0) {
                fn_8015FED8((u8*)sp1F0, (u8*)basePos, fn_801CBD28(em));
                fn_801D0058(p, (u8*)sp1F0);
            }
        } else {
            fn_801D0058(p, (u8*)basePos);
        }
        Vec3Length(&sp60, (Vec3*)fn_801D0224(em));
        fn_80054840((u8*)&sp8C, (u8*)&sp60);
        fn_801D0200(p, fn_801D0224(em));
        fn_8016F784((u8*)&sp5C, (u8*)&sp90, (u8*)&sp8C);
        fn_801CEC30(fn_801CE390(p), (u8*)&sp5C);
        f32 lo = ((f32*)fn_801D0048(proto))[1];
        f32 hi = ((f32*)fn_801D0048(proto))[2];
        f32 range = (hi - lo) * 1.5707964f;
        fn_8008CE28((u8*)sp1E4, lo);
        fn_80082BE0((u8*)sp2F0, (u8*)sp1E4);
        if (range >= 0.00001f) {
            fn_801D022C((u8*)sp2F0, lo, hi);
            fn_801D0368((u8*)sp2F0, RebuildBits(fn_801D0338((u8*)sp2F0)));
            fn_801D0378((u8*)sp2F0, RebuildBits(fn_801D0370((u8*)sp2F0)));
            fn_801D0388((u8*)sp2F0, RebuildBits(fn_801D0380((u8*)sp2F0)));
        }
        ScalarSet(&sp58, 0.63661975f);
        fn_80082F20((u8*)sp1CC, (u8*)sp2F0);
        Vec3MulScalar((Vec3*)sp1D8, (Vec3*)sp1CC, &sp58);
        MtxFromLocatorRotation(lbl_8047C5B8, sp1D8);
        fn_8016F6F0((u8*)&sp84, fn_801CE390(p));
        Vec3Rotate((Vec3*)sp1C0, lbl_8047C5B8, (Vec3*)fn_801CE390(p));
        Vec3Copy((Vec3*)fn_801CE390(p), (Vec3*)sp1C0);
        fn_8016EB60(fn_801CE390(p), (u8*)&sp84);
        break;
    }
    case 2: {
        switch (fn_801D0390(proto)) {
        case 0: {
            fn_801D0398((u8*)sp330, fn_801D04A8(proto));
            switch ((s32)fn_800677D0(0.0f, 2.9f)) {
            case 0: {
                f32 a = fn_801D0338(fn_801D04A8(proto));
                f32 b = fn_801D0338(fn_801D04B0(proto));
                fn_801D0368((u8*)sp330, RebuildBits(fn_800677D0(b, a)));
                break;
            }
            case 1: {
                f32 a = fn_801D0370(fn_801D04A8(proto));
                f32 b = fn_801D0370(fn_801D04B0(proto));
                fn_801D0378((u8*)sp330, RebuildBits(fn_800677D0(b, a)));
                break;
            }
            case 2: {
                f32 a = fn_801D0380(fn_801D04A8(proto));
                f32 b = fn_801D0380(fn_801D04B0(proto));
                fn_801D0388((u8*)sp330, RebuildBits(fn_800677D0(b, a)));
                break;
            }
            }
            fn_801D04B8((u8*)sp330, 0.0f);
            fn_801D0058(p, (u8*)sp330);
            break;
        }
        case 1: {
            fn_801D09B0((u8*)sp330, (u8*)sp320);
            u8* a = fn_801D04A8(proto);
            fn_801CFEF4((u8*)sp320, fn_801D04B0(proto), a);
            fn_801D0368((u8*)sp320, RebuildBits(fn_801D0338((u8*)sp320)));
            fn_801D0378((u8*)sp320, RebuildBits(fn_801D0370((u8*)sp320)));
            fn_801D0388((u8*)sp320, RebuildBits(fn_801D0380((u8*)sp320)));
            fn_801CE3D0((u8*)sp330, (u8*)sp320);
            fn_801D04B8((u8*)sp330, 0.0f);
            fn_801D0058(p, (u8*)sp330);
            break;
        }
        default:
            fn_8014879C((u8*)sp1B0);
            fn_801D04B8((u8*)sp330, 0.0f);
            fn_801D0058(p, (u8*)sp330);
            break;
        }
        if (fn_801CC8F4(proto, 9) != 0) {
            if (fn_801D04C0(fn_801D0504(proto)) == 0) {
                fn_8005263C(sp2E0);
                Vec3SetUnitZ((Vec3*)sp198);
                fn_80082F20((u8*)sp1A4, fn_801D0224(em));
                fn_801458B0(sp2E0, sp1A4, sp198);
                fn_80082F20((u8*)sp170, fn_801CC8E0(p));
                fn_80054650((u8*)sp17C, (u8*)sp2E0, (u8*)sp170);
                fn_80082BE0((u8*)sp188, (u8*)sp17C);
                fn_801D0058(p, (u8*)sp188);
            }
        }
        fn_8014879C((u8*)sp160);
        fn_801D0200(p, (u8*)sp160);
        fn_8014879C((u8*)sp2D0);
        fn_8014879C((u8*)sp2C0);
        fn_8014879C((u8*)sp2B0);
        if (*(f32*)fn_801D0048(proto) != 0.0f) {
            f32 f30 = fn_801D0050(proto);
            f30 = fn_800677D0(-fn_801D0050(proto), f30);
            ScalarSet(&sp54, *(f32*)fn_801D0048(proto) + f30);
            fn_80054840((u8*)&sp90, (u8*)&sp54);
            Vec3Length(&sp80, (Vec3*)fn_801CC8E0(p));
            ScalarSet(&sp50, 0.001f);
            if (fn_80058BE8(&sp80, &sp50) != 0) {
                Vec3Length(&sp48, (Vec3*)fn_801CC8E0(p));
                fn_8016F784((u8*)&sp4C, (u8*)&sp90, (u8*)&sp48);
                fn_8015FDE8((u8*)sp150, fn_801CC8E0(p), &sp4C);
                fn_8015EC78((u8*)sp2D0, (u8*)sp150);
            } else {
                Vec3SetUnitZ((Vec3*)sp124);
                fn_80082BE0((u8*)sp130, (u8*)sp124);
                fn_8015FDE8((u8*)sp140, (u8*)sp130, &sp90);
                fn_8015EC78((u8*)sp2D0, (u8*)sp140);
            }
        }
        if (((f32*)fn_801D0048(proto))[1] != 0.0f) {
            f32 f30 = fn_801D0050(proto);
            f30 = fn_800677D0(-fn_801D0050(proto), f30);
            ScalarSet(&sp44, ((f32*)fn_801D0048(proto))[1] + f30);
            fn_80054840((u8*)&sp90, (u8*)&sp44);
            fn_801D022C((u8*)sp2C0, -1.0f, 1.0f);
            fn_801D04B8((u8*)sp2C0, 0.0f);
            fn_80082F20((u8*)sp118, (u8*)sp2C0);
            Vec3Length(&sp3C, (Vec3*)sp118);
            fn_8016F784((u8*)&sp40, (u8*)&sp90, (u8*)&sp3C);
            fn_801CEC30((u8*)sp2C0, (u8*)&sp40);
        }
        if (((f32*)fn_801D0048(proto))[2] != 0.0f) {
            f32 f30 = fn_801D0050(proto);
            f30 = fn_800677D0(-fn_801D0050(proto), f30);
            ScalarSet(&sp38, ((f32*)fn_801D0048(proto))[2] + f30);
            fn_80054840((u8*)&sp90, (u8*)&sp38);
            Vec3Length(&sp30, (Vec3*)fn_801D0224(em));
            fn_8016F784((u8*)&sp34, (u8*)&sp90, (u8*)&sp30);
            fn_8015FDE8((u8*)sp108, fn_801D0224(em), &sp34);
            fn_8015EC78((u8*)sp2B0, (u8*)sp108);
        }
        fn_8015FDA4((u8*)spF8, (u8*)sp2D0, (u8*)sp2C0);
        fn_8015EC78(fn_801CE390(p), (u8*)spF8);
        fn_8015FFEC(fn_801CE390(p), (u8*)sp2B0);
        fn_8016F6F0((u8*)&sp7C, fn_801CC8E0(p));
        ScalarSet(&sp2C, fn_801CE424(em));
        Vec3ScaleBy((Vec3*)fn_801CC8E0(p), &sp2C);
        fn_8016EB60(fn_801CC8E0(p), (u8*)&sp7C);
        if (fn_801CC8F4(proto, 0) == 0) {
            fn_8015FFEC(fn_801CC8E0(p), (u8*)basePos);
        }
        break;
    }
    }

    if (fn_801CBD38(proto) != 0) {
        f32 f30 = fn_801D0338((u8*)sp310);
        fn_801D04B8(fn_801CE390(p), f30);
        fn_801D04B8(fn_801CC8E0(p), 0.0f);
        if (fn_801D0338(defB) > 0.01f) {
            f32 ang = fn_800677D0(0.0f, 6.2831855f);
            fn_801D04B8(fn_801CC8E0(p), ang);
        }
    } else {
        fn_8015EC78(fn_801CE544(p), (u8*)sp310);
        fn_8014879C((u8*)spE8);
        fn_8015EC78(fn_801CE53C(p), (u8*)spE8);
        if (fn_801CC8F4(proto, 10) != 0) {
            fn_800526D0((u8*)sp370);
            fn_801C9DF0(em, (f32*)fn_801D0224(em), sp370, 0.0f);
            fn_80052638((Vec3*)sp2A0);
            fn_801449A4(sp370, sp2A0);
            fn_801D0368(fn_801CE53C(p), 1.5707964f * Vec3GetX((Vec3*)sp2A0));
            fn_801D0378(fn_801CE53C(p), 1.5707964f * Vec3GetY((Vec3*)sp2A0));
            fn_801D0388(fn_801CE53C(p), 1.5707964f * Vec3GetZ((Vec3*)sp2A0));
        }
        if (fn_801CC8F4(proto, 3) != 0) {
            fn_801467D4();
            fn_801D022C((u8*)sp290, 0.0f, 6.2831855f);
            if (fn_80058C00(fn_801D0338(defB)) != 0) {
                ScalarZero(&sp28);
                fn_801D050C((u8*)sp290, (u8*)&sp28);
            }
            if (fn_80058C00(fn_801D0370(defB)) != 0) {
                ScalarZero(&sp24);
                fn_801D0518((u8*)sp290, (u8*)&sp24);
            }
            if (fn_80058C00(fn_801D0380(defB)) != 0) {
                ScalarZero(&sp20);
                fn_801D0524((u8*)sp290, (u8*)&sp20);
            }
            if (fn_80058C00(fn_801D0530(defB)) != 0) {
                ScalarZero(&sp1C);
                fn_8016EB60((u8*)sp290, (u8*)&sp1C);
            }
            fn_8015FFEC(fn_801CE53C(p), (u8*)sp290);
        } else if (fn_801CC8F4(proto, 4) != 0) {
            fn_800526D0((u8*)sp340);
            fn_801C9DF0(em, (f32*)fn_801CE390(p), sp340, 0.0f);
            fn_80052638((Vec3*)sp280);
            fn_801449A4(sp340, sp280);
            fn_801D0368(fn_801CE53C(p), 1.5707964f * Vec3GetX((Vec3*)sp280));
            fn_801D0378(fn_801CE53C(p), 1.5707964f * Vec3GetY((Vec3*)sp280));
            fn_801D0388(fn_801CE53C(p), 1.5707964f * Vec3GetZ((Vec3*)sp280));
            fn_8014879C((u8*)spD8);
            fn_801D0538(p, (u8*)spD8);
        }
    }

    // Color, size, life, spin ranges.
    u8* rangeB = fn_801D089C(proto);
    fn_801D0580((u8*)spC8, fn_801CE3A0(proto), rangeB, 0.0f, 1.0f);
    fn_801D055C(p, (u8*)spC8);
    u8* rangeC = fn_801D08C8(proto);
    fn_801D0580((u8*)spB8, fn_801CE414(proto), rangeC, 0.0001f, 65535.0f);
    fn_801D08A4(p, (u8*)spB8);
    if (fn_80058C00(fn_801D0530(fn_801D08C8(proto))) == 0) {
        fn_8016F6F0((u8*)&sp18, fn_801D08C8(proto));
        fn_8015FD30((u8*)sp270, (u8*)&sp18);
        fn_801D0580((u8*)sp260, fn_801CE414(proto), (u8*)sp270, 0.0001f, 65535.0f);
        fn_8016F6F0((u8*)&sp14, (u8*)sp260);
        fn_8015FD30((u8*)spA8, (u8*)&sp14);
        fn_801D08A4(p, (u8*)spA8);
    }
    f32 f30 = fn_801D0954(proto);
    sp10 = fn_801D08DC(fn_801CE480(proto), f30);
    fn_801D08D0(p, (u8*)&sp10);
    f30 = fn_801D095C(proto);
    f30 = fn_801CE318(proto) + f30;
    f32 f31 = fn_801D095C(proto);
    fn_801CE764(p, fn_800677D0(fn_801CE318(proto) - f31, f30));
    fn_801D0964(p, fn_801CE320(p));

    f31 = fn_801CE3A8(p);
    u8* t = fn_801CE3C8(p);
    fn_801C9000(fn_801CE3A0(proto), t, f31);
    f31 = fn_801CE3A8(p);
    t = fn_801CE41C(p);
    fn_801C9000(fn_801CE414(proto), t, f31);
    f31 = fn_801CE3A8(p);
    t = fn_801CE49C(p);
    fn_801C911C(fn_801CE480(proto), t, f31);

    fn_801CE3D0(fn_801CE3C8(p), *(u8**)(em + 0x70) + 4);
    fn_8016F6F0((u8*)&sp78, fn_801CE390(p));
    ScalarSet(&sp74, fn_801CE424(em));
    Vec3ScaleBy((Vec3*)fn_801CE390(p), &sp74);
    fn_8016EB60(fn_801CE390(p), (u8*)&sp78);
    fn_8016F6F0((u8*)&spC, fn_801CE41C(p));
    fn_80054840((u8*)&sp78, (u8*)&spC);
    Vec3ScaleBy((Vec3*)fn_801CE41C(p), &sp74);
    fn_8016EB60(fn_801CE41C(p), (u8*)&sp78);
    fn_801CE3D0(fn_801CC8E4(p), *(u8**)(em + 0x70) + 4);
    fn_8016F6F0((u8*)&sp8, fn_801CC8EC(p));
    fn_80054840((u8*)&sp78, (u8*)&sp8);
    Vec3ScaleBy((Vec3*)fn_801CC8EC(p), &sp74);
    fn_8016EB60(fn_801CC8EC(p), (u8*)&sp78);
    fn_801D096C(p, 0);
    fn_801D0974(p, fn_801CC8E0(p));

    // Linked effect.
    if (fn_801D0998(proto) != 0) {
        if (fn_801CC8F4(proto, 0) != 0) {
            u8* pos = fn_801CC8E0(p);
            fn_8015FDA4((u8*)sp98, fn_801CBD28(em), pos);
            if (SpawnEffectAt(fxMgr, (u32)fn_801D0998(proto), sp98, (u8**)(p + 0x90)) == 0) {
                *(u8**)(p + 0x90) = NULL;
            }
        } else {
            u8* pos = fn_801CC8E0(p);
            if (SpawnEffectAt(fxMgr, (u32)fn_801D0998(proto), (f32*)pos, (u8**)(p + 0x90)) == 0) {
                *(u8**)(p + 0x90) = NULL;
            }
        }
    } else {
        fn_801D09A0(p, 0);
    }
    if (*(u8**)(p + 0x90) != NULL) {
        for (s32 i = 0; i < *(s32*)(*(u8**)(p + 0x90) + 0xC); i++) {
            u8* child = *(u8**)(*(u8**)(p + 0x90) + 0x14 + i * 4);
            if (child != NULL) {
                *(s32*)(child + 0x74) = *(s32*)(em + 0x74);
            }
        }
    }

    // Linked object from the effect table.
    if (fn_801D09A8(proto) != 0) {
        u8* obj = fn_801C3EA4(lbl_8047C7AC, fn_801D09A8(proto));
        *(u8**)(p + 0x94) = obj;
        if (obj != NULL) {
            fn_801C42A8(obj);
        }
    } else {
        *(u8**)(p + 0x94) = NULL;
    }
}

// 0x801E361C
// Builds the 16 vertices (0x18 bytes each from +0x50: position, color, u, v) of a 4x4 patch
// from the object's size (+0x38, +0x3C) and orientation: corner vectors are rotated and moved
// to the object position (+0xC), then interpolated across the grid with the shared scalars
// (lazily initialised: 1/3, 1/3, 1.0).
void fn_801E361C(u8* obj) {
    f32 sp140[4];
    Vec3 sp130, sp124, sp118, sp10C, sp100, spF4, spE8;
    Vec3 spDC, spD0, spC4;
    Vec3 spB8, spAC, spA0, sp94;
    Vec3 sp88, sp7C, sp70, sp64, sp58, sp4C, sp40, sp34, sp28;
    f32 sp24, sp10, spC, sp8;
    f32 sp14, sp1C, sp18, sp20;

    if (lbl_805FB8B0 == 0) {
        ScalarSet(&lbl_805FB8B4, 0.33333334f);
        lbl_805FB8B0 = 1;
    }
    if (lbl_805FB8B8 == 0) {
        ScalarSet(&lbl_805FB8BC, 0.33333334f);
        lbl_805FB8B8 = 1;
    }
    if (lbl_805FB8C0 == 0) {
        fn_800544CC(&lbl_805FB8C4);
        lbl_805FB8C0 = 1;
    }
    fn_8005263C(sp140);
    fn_801E3A30(obj, sp140);
    fn_8016EB0C((u8*)&sp130, -0.5f * *(f32*)(obj + 0x38), -0.5f * *(f32*)(obj + 0x3C));
    fn_80082F0C((u8*)&spDC, *(f32*)(obj + 0x38));
    fn_80054484(&sp124, &sp130, &spDC);
    fn_801E3CCC((u8*)&spD0, *(f32*)(obj + 0x3C));
    fn_80054484(&sp118, &sp130, &spD0);
    fn_801E3CCC((u8*)&spC4, *(f32*)(obj + 0x3C));
    fn_80054484(&sp10C, &sp124, &spC4);
    ScalarZero(&sp24);
    fn_80052638(&sp100);
    fn_80052638(&spF4);
    fn_80052638(&spE8);
    fn_800636A0();
    fn_800636A0();
    fn_800636A0();
    fn_800636A0();
    fn_80054650((u8*)&spB8, (u8*)sp140, (u8*)&sp130);
    Vec3Copy(&sp130, &spB8);
    Vec3AddTo(&sp130, (Vec3*)(obj + 0xC));
    fn_80054650((u8*)&spAC, (u8*)sp140, (u8*)&sp124);
    Vec3Copy(&sp124, &spAC);
    Vec3AddTo(&sp124, (Vec3*)(obj + 0xC));
    fn_80054650((u8*)&spA0, (u8*)sp140, (u8*)&sp118);
    Vec3Copy(&sp118, &spA0);
    Vec3AddTo(&sp118, (Vec3*)(obj + 0xC));
    fn_80054650((u8*)&sp94, (u8*)sp140, (u8*)&sp10C);
    Vec3Copy(&sp10C, &sp94);
    Vec3AddTo(&sp10C, (Vec3*)(obj + 0xC));
    u32 color = fn_801E3CE0(*(u32*)(obj + 0x48), 0x7F);

    u8* rowFirst = obj + 0x50;
    s32 row = 0;
    s32 rowBytes = 0;
    do {
        u8* rowLast = obj + (row + 3) * 0x18 + 0x50;
        ScalarMul(&sp10, &sp24, &lbl_805FB8B4);
        fn_80054840((u8*)&sp14, (u8*)&sp10);
        fn_800544B8(&spC, &lbl_805FB8C4, &sp14);
        fn_80054840((u8*)&sp1C, (u8*)&spC);
        Vec3MulScalar(&sp70, &sp118, &sp14);
        Vec3MulScalar(&sp7C, &sp130, &sp1C);
        fn_80054484(&sp88, &sp7C, &sp70);
        Vec3Copy(&sp100, &sp88);
        Vec3MulScalar(&sp4C, &sp10C, &sp14);
        Vec3MulScalar(&sp58, &sp124, &sp1C);
        fn_80054484(&sp64, &sp58, &sp4C);
        Vec3Copy(&spF4, &sp64);
        *(f32*)(rowFirst + 0x10) = 0.0f;
        *(f32*)(rowFirst + 0x14) = fn_800548EC((u8*)&sp14);
        *(u32*)(rowFirst + 0xC) = color;
        Vec3Copy((Vec3*)rowFirst, &sp100);
        *(f32*)(rowLast + 0x10) = 1.0f;
        *(f32*)(rowLast + 0x14) = fn_800548EC((u8*)&sp14);
        *(u32*)(rowLast + 0xC) = color;
        Vec3Copy((Vec3*)rowLast, &spF4);
        fn_80054840((u8*)&sp18, (u8*)&lbl_805FB8BC);
        s32 k = 1;
        u8* v = obj + rowBytes + 0x18;
        do {
            fn_801CD680(&sp18, &lbl_805FB8BC);
            fn_800544B8(&sp8, &lbl_805FB8C4, &sp18);
            fn_80054840((u8*)&sp20, (u8*)&sp8);
            Vec3MulScalar(&sp28, &spF4, &sp18);
            Vec3MulScalar(&sp34, &sp100, &sp20);
            fn_80054484(&sp40, &sp34, &sp28);
            Vec3Copy(&spE8, &sp40);
            *(f32*)(v + 0x60) = fn_800548EC((u8*)&sp18);
            *(f32*)(v + 0x64) = fn_800548EC((u8*)&sp14);
            *(u32*)(v + 0x5C) = color;
            Vec3Copy((Vec3*)(obj + (row + k) * 0x18 + 0x50), &spE8);
            k++;
            v += 0x18;
        } while (k < 3);
        rowFirst += 0x60;
        row += 4;
        rowBytes += 0x60;
        fn_801CD680(&sp24, &lbl_805FB8C4);
    } while (row < 0x10);
}

// 0x801E3CFC
// Moves the 16 corner points (0x18 bytes each from +0x50) of a box with half extents obj+0x18..
// against the world: first sweeps the box with fn_80139684, then for every corner still flagged
// in obj[0x4C] (bits 0x3FFFF; 0x400000 skips everything, 0x200000 skips the sweep once) tests
// the corner's small box against the collision shapes and either moves the corner by the
// resolved offset or clears that corner's mask bits.
void fn_801E3CFC(u8* obj) {
    f32 off[3];
    f32 ext[3];
    f32 boxMin[3];
    f32 boxMax[3];
    f32 out[3];

    *(u32*)(obj + 0x4C) = *(u32*)(obj + 0x4C) | 0x3FFFF;
    if ((*(u32*)(obj + 0x4C) & 0x400000) != 0) return;

    f32 ex = *(f32*)(obj + 0x18);
    f32 ey = *(f32*)(obj + 0x1C);
    f32 ez = *(f32*)(obj + 0x20);
    f32 s = -*(f32*)(obj + 0x30);
    off[0] = ex * s;
    off[1] = ey * s;
    off[2] = ez * s;
    ext[0] = ex * 2.0f;
    ext[1] = ey * 2.0f;
    ext[2] = ez * 2.0f;
    out[0] = out[1] = out[2] = 0.0f;
    if ((*(u32*)(obj + 0x4C) & 0x200000) == 0) {
        f32 pos[3];
        pos[0] = *(f32*)(obj + 0xC) - off[0];
        pos[1] = *(f32*)(obj + 0x10) - off[1];
        pos[2] = *(f32*)(obj + 0x14) - off[2];
        fn_80139684(gWorldState, pos, out, *(f32*)(obj + 0x40));
    } else {
        *(u32*)(obj + 0x4C) = *(u32*)(obj + 0x4C) & 0xFFDFFFFF;
    }

    u8* corner = obj;
    s32* mask = lbl_8048A830;
    for (s32 i = 0; i < 0x10 && (*(u32*)(obj + 0x4C) & 0x3FFFF) != 0; i++) {
        f32 cx = *(f32*)(corner + 0x50);
        f32 cy = *(f32*)(corner + 0x54);
        f32 cz = *(f32*)(corner + 0x58);
        boxMax[0] = cx + ext[0];
        boxMax[1] = cy + ext[1];
        boxMax[2] = cz + ext[2];
        boxMin[0] = cx - ext[0];
        boxMin[1] = cy - ext[1];
        boxMin[2] = cz - ext[2];
        if (fn_801E41E4(boxMin, boxMax, lbl_803EA0D0, lbl_805FB330, out) != 0) {
            *(f32*)(corner + 0x50) = out[0] + off[0];
            *(f32*)(corner + 0x54) = out[1] + off[1];
            *(f32*)(corner + 0x58) = out[2] + off[2];
        } else {
            *(u32*)(obj + 0x4C) = *(u32*)(obj + 0x4C) & ~(u32)*mask;
        }
        corner += 0x18;
        mask++;
    }
}

// 0x801E3FA8
// Packs the corner indices (3 bytes per set bit of obj[0x4C]) into obj+0x1D0 and stores the
// count at +0x208. All 18 bits set copies the whole 54-byte table (count 0x12); none leaves it empty.
void fn_801E3FA8(u8* obj) {
    u32 bits = *(u32*)(obj + 0x4C) & 0x3FFFF;
    if (bits == 0x3FFFF) {
        *(s32*)(obj + 0x208) = 0x12;
        memcpy(obj + 0x1D0, lbl_8048A870, 0x36);
        return;
    }
    *(s32*)(obj + 0x208) = 0;
    if (bits == 0) return;

    u8* src = lbl_8048A870;
    s32 bit = 0;
    for (s32 i = 0; i < 9; i++) {
        if ((*(u32*)(obj + 0x4C) & (1u << bit)) != 0) {
            s32 n = *(s32*)(obj + 0x208);
            *(u8*)(obj + 0x1D0 + n * 3) = src[0];
            *(u8*)(obj + 0x1D1 + n * 3) = src[1];
            *(u8*)(obj + 0x1D2 + n * 3) = src[2];
            *(s32*)(obj + 0x208) = n + 1;
        }
        bit++;
        if ((*(u32*)(obj + 0x4C) & (1u << bit)) != 0) {
            s32 n = *(s32*)(obj + 0x208);
            *(u8*)(obj + 0x1D0 + n * 3) = src[3];
            *(u8*)(obj + 0x1D1 + n * 3) = src[4];
            *(u8*)(obj + 0x1D2 + n * 3) = src[5];
            *(s32*)(obj + 0x208) = n + 1;
        }
        src += 6;
        bit++;
    }
}

// 0x801E40DC
// Builds the corner tables: for 3 groups of 3 steps it writes 6 corner indices per step into
// lbl_8048A870 (54 bytes) and ORs the step's two mask bits into the 16 corner masks lbl_8048A830.
void fn_801E40DC(void) {
    u8* out = lbl_8048A870;
    s32 bitBase = 0;
    s8 v0 = 0;
    for (s32 g = 0; g < 3; g++) {
        s8 v = v0;
        for (s32 k = 0; k < 3; k++) {
            u32 b0 = 1u << bitBase;
            u32 b1 = 1u << (bitBase + 1);
            u32 i0 = (u8)v;
            u32 i1 = (u8)(v + 1);
            u32 i4 = (u8)(v + 4);
            u32 i5 = (u8)(v + 5);
            out[0] = (u8)v;
            lbl_8048A830[i0] |= b0;
            out[1] = (u8)(v + 1);
            lbl_8048A830[i1] |= b0;
            out[2] = (u8)(v + 4);
            lbl_8048A830[i4] |= b0;
            lbl_8048A830[i4] |= b1;
            out[3] = (u8)(v + 4);
            lbl_8048A830[i1] |= b1;
            out[4] = (u8)(v + 1);
            out[5] = (u8)(v + 5);
            out += 6;
            lbl_8048A830[i5] |= b1;
            bitBase += 2;
            v++;
        }
        v0 += 4;
    }
}

}  // extern "C"
