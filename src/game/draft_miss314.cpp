// Mixed functions: entity hit callback (0x80067C30), graph search (0x800D5DE4),
// UI keyframe interpolation (0x801AA218) and a debug arrow/cone draw (0x801D79D4).
#include "types.h"
#include "engine/math.h"
#include <math.h>

extern "C" {
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);  // 0x801C80E0 (really returns BOOL: tail call to SpawnEffectAtPos)
u32   RandomU32(void);                                // 0x80143460
void  fn_8006784C(u8* self);                          // 0x8006784C
void  fn_8018D370(const char* fmt, ...);              // 0x8018D370 (debug log)

f32   FloatAbs(f32 v);                                // 0x80058240
void  Vec3Assign(Vec3* out, const Vec3* v);           // 0x80053354
void  Vec3Copy(Vec3* out, const Vec3* v);             // 0x80052640
void  Vec3NormalizeTo(Vec3* out, const Vec3* v);      // 0x80055D9C
void  Vec3SetUnitZ(Vec3* v);                          // 0x800549A8
void  fn_800359F4(u8* a3, f32 f1, f32 f2, f32 f3);    // 0x800359F4
u8*   fn_80044634(s32 n);                             // 0x80044634
void  fn_800446BC(void*, int);                        // 0x800446BC
void  fn_80052638(Vec3* v);                           // 0x80052638 (empty leaf)
void  fn_800526D0(u8* out);                           // 0x800526D0
void  fn_8005535C(u8* dst, const u8* src);            // 0x8005535C
u8*   fn_800554D4(u8* mtx);                           // 0x800554D4
void  fn_800557B4(u8* out);                           // 0x800557B4
void  fn_8007FD98(u8* m, u8* v);                      // 0x8007FD98
void  fn_800C09C0(Vec3* out);                         // 0x800C09C0
void  fn_800C11A4(Vec3* out, const Vec3* a, const Vec3* b);  // 0x800C11A4 (cross product)
f32   fn_800C11F0(f32 x);                             // 0x800C11F0
void  fn_8010AB3C(void);                              // 0x8010AB3C
void  fn_8010AB70(void);                              // 0x8010AB70
void  fn_80149094(Vec3* out);                         // 0x80149094
s32   fn_80152620(s32 a, s32 b, u8* quad);            // 0x80152620
void  fn_80154318(void);                              // 0x80154318
void  fn_80155724(u8* m);                             // 0x80155724
u8*   fn_8016EB20(u8* m);                             // 0x8016EB20 (returns its argument)
u8*   fn_8016EB4C(u8* m);                             // 0x8016EB4C (returns m + 0x18)
void  fn_80174A24(u8* p, u32 color);                  // 0x80174A24
void  fn_801D77C0(void);                              // 0x801D77C0
void  fn_801D7E9C(f32* m, f32* v);                    // 0x801D7E9C
s32   fn_801CB938(void);                              // 0x801CB938
void* memcpy(void* d, const void* s, unsigned long n);
extern u8 lbl_8047C760[];     // 0x8047C760 (effect manager)
extern u8 lbl_8047D378[];     // 0x8047D378 (0x120-byte debug draw vertex buffer)
extern u8 lbl_805FB824;       // 0x805FB824 (debug draw buffer initialised flag)
}

extern "C++" void fn_80154358(void);   // 0x80154358

typedef BOOL (*SpawnFn)(void*, u32, const f32*, u8**);
typedef s32 (*SweepFn)(u8* obj, f32* outHit, u8* shape, f32* outInfo, s32 a, s32 b, s32 c);

extern "C" {

// 0x80067C30
// Shape callback for a thrown / bouncing entity: sweeps the owner's collider against `arg0`.
// When the owner is a flagged projectile it is pushed out of the surface, reflected (with a
// little random scatter), plays its impact effect and sound; otherwise the closest hit
// point along the direction at arg1+0x0 is kept in arg1+0x18 / +0x30. Always returns 1.
s32 fn_80067C30(u8* arg0, u8* arg1) {
    // Stack block 0xEC..0x120 (floats), zeroed; the sweep fills the first part.
    f32 blk[14];
    for (int i = 0; i < 14; i++) blk[i] = 0.0f;
    f32* spEC = &blk[0];
    f32* sp108 = &blk[7];   // hit position (x, y, z)
    f32 sp120 = blk[13];

    u8* obj = *(u8**)(arg1 + 0x24);
    u8* sweep = *(u8**)(obj + 0x22C);
    if ((*(SweepFn*)(*(u8**)sweep + 0x18))(sweep, sp108, arg0, spEC, 0, 0, 0) == 0) return 1;

    u8* ctx = *(u8**)(arg1 + 0x2C);
    if ((*(s32*)(*(u8**)(ctx + 0x4) + 0x1C) & 0x10) && (*(s32*)(ctx + 0x838) & 0x20000)) {
        u8* o = *(u8**)(arg1 + 0x24);
        s32 flags = *(s32*)(o + 0x244);
        if (flags & 0x10000000) return 1;
        *(s32*)(o + 0x244) = flags | 0x10000000;

        // Push the owner out along the sweep segment (start +0x14 .. end +0x20).
        u8* seg = *(u8**)(*(u8**)(arg1 + 0x24) + 0x22C);
        f32 dx = *(f32*)(seg + 0x20) - *(f32*)(seg + 0x14);
        f32 dy = *(f32*)(seg + 0x24) - *(f32*)(seg + 0x18);
        f32 dz = *(f32*)(seg + 0x28) - *(f32*)(seg + 0x1C);
        f32 lenSq = dz * dz + (dx * dx + dy * dy);
        f32 len = 0.0f;
        if (!(lenSq <= 0.0f)) len = sqrtf(lenSq);
        f32 depth = 0.05f + (((1.0f - sp120) * len) + *(f32*)(seg + 0x3C));
        if (!(len <= 0.0001f && len >= -0.0001f)) {
            f32 inv = 1.0f / len;
            dx = dx * inv * depth;
            dy = dy * inv * depth;
            dz = dz * inv * depth;
            u8* pos = *(u8**)(o + 0x4);
            *(f32*)(pos + 0x0) = *(f32*)(pos + 0x0) + dx;
            *(f32*)(pos + 0x4) = *(f32*)(pos + 0x4) + dy;
            *(f32*)(pos + 0x8) = *(f32*)(pos + 0x8) + dz;
            u8* o2 = *(u8**)(arg1 + 0x24);
            *(f32*)(o2 + 0x18) = *(f32*)(o2 + 0x18) + dx;
            *(f32*)(o2 + 0x1C) = *(f32*)(o2 + 0x1C) + dy;
            *(f32*)(o2 + 0x20) = *(f32*)(o2 + 0x20) + dz;
            u8* seg2 = *(u8**)(*(u8**)(arg1 + 0x24) + 0x22C);
            *(f32*)(seg2 + 0x14) = *(f32*)(seg2 + 0x14) + dx;
            *(f32*)(seg2 + 0x18) = *(f32*)(seg2 + 0x18) + dy;
            *(f32*)(seg2 + 0x1C) = *(f32*)(seg2 + 0x1C) + dz;
        }

        // Reflect the velocity (+0x1AC) about the contact normal and normalise it.
        u8* o3 = *(u8**)(arg1 + 0x24);
        u8* c3 = *(u8**)(arg1 + 0x2C);
        u8* p3 = *(u8**)(o3 + 0x4);
        f32 nx = *(f32*)(p3 + 0x0) - *(f32*)(c3 + 0x18);
        f32 ny = *(f32*)(p3 + 0x4) - *(f32*)(c3 + 0x1C);
        f32 nz = *(f32*)(p3 + 0x8) - *(f32*)(c3 + 0x20);
        f32 vx = *(f32*)(o3 + 0x1AC);
        f32 vy = *(f32*)(o3 + 0x1B0);
        f32 vz = *(f32*)(o3 + 0x1B4);
        f32 d = vz * nz + (vx * nx + vy * ny);
        vx = vx - (nx * 2.0f) * d;
        vy = vy - (ny * 2.0f) * d;
        vz = vz - (nz * 2.0f) * d;
        *(f32*)(o3 + 0x1AC) = vx;
        *(f32*)(o3 + 0x1B0) = vy;
        *(f32*)(o3 + 0x1B4) = vz;

        u8* o4 = *(u8**)(arg1 + 0x24);
        vx = *(f32*)(o4 + 0x1AC);
        vy = *(f32*)(o4 + 0x1B0);
        vz = *(f32*)(o4 + 0x1B4);
        f32 vlenSq = vz * vz + (vx * vx + vy * vy);
        f32 ox, oy, oz;
        if (vlenSq == 0.0f) {
            ox = oy = oz = 0.0f;
        } else {
            f32 r = 1.0f / sqrtf(vlenSq);
            ox = vx * r;
            oy = vy * r;
            oz = vz * r;
        }
        *(f32*)(o4 + 0x1AC) = ox;
        *(f32*)(o4 + 0x1B0) = oy;
        *(f32*)(o4 + 0x1B4) = oz;

        // Random scatter in [0, 0.2) per axis, then damp by 0.3 * collider factor.
        f32 rx = 2.3283064e-10f * (f32)RandomU32();
        f32 ry = 2.3283064e-10f * (f32)RandomU32();
        f32 rz = 2.3283064e-10f * (f32)RandomU32();
        u8* o5 = *(u8**)(arg1 + 0x24);
        *(f32*)(o5 + 0x1AC) = *(f32*)(o5 + 0x1AC) + rx * 0.2f;
        *(f32*)(o5 + 0x1B0) = *(f32*)(o5 + 0x1B0) + ry * 0.2f;
        *(f32*)(o5 + 0x1B4) = *(f32*)(o5 + 0x1B4) + rz * 0.2f;
        u8* o6 = *(u8**)(arg1 + 0x24);
        f32 damp = 0.3f * *(f32*)(*(u8**)(o6 + 0x228) + 0xB8);
        *(f32*)(o6 + 0x1AC) = *(f32*)(o6 + 0x1AC) * damp;
        *(f32*)(o6 + 0x1B0) = *(f32*)(o6 + 0x1B0) * damp;
        *(f32*)(o6 + 0x1B4) = *(f32*)(o6 + 0x1B4) * damp;

        u8* o7 = *(u8**)(arg1 + 0x24);
        if (*(s32*)(o7 + 0x244) & 0x800) fn_8006784C(o7);

        // Impact effect at the owner's position, oriented along the push direction (+0xD4 frame:
        // the offset from the contact context to the owner position).
        u8* o8 = *(u8**)(arg1 + 0x24);
        u32 fx = *(u32*)(*(u8**)(o8 + 0x228) + 0xF4);
        if (fx != 0) {
            u8* handle = NULL;
            f32 fpos[4];
            fpos[0] = *(f32*)(o8 + 0x1A0);
            fpos[1] = *(f32*)(o8 + 0x1A4);
            fpos[2] = *(f32*)(o8 + 0x1A8);
            fpos[3] = 0.0f;
            if (((SpawnFn)SpawnEffectAt)(lbl_8047C760, fx, fpos, &handle)) {
                f32 ex = nx, ey = ny, ez = nz, ew = 0.0f;
                BOOL zero = FALSE;
                if (ex <= 0.0001f && ex >= -0.0001f && ey <= 0.0001f && ey >= -0.0001f &&
                    ez <= 0.0001f && ez >= -0.0001f && ew <= 0.0001f && ew >= -0.0001f) {
                    zero = TRUE;
                }
                s32 n = *(s32*)(handle + 0xC);
                if (zero) {
                    // Degenerate direction: point the emitters along +Z.
                    for (s32 i = 0; i < n; i++) {
                        u8* e = *(u8**)(handle + i * 4 + 0x14);
                        if (e != NULL) {
                            *(f32*)(e + 0x30) = 0.0f;
                            *(f32*)(e + 0x34) = 0.0f;
                            *(f32*)(e + 0x38) = 1.0f;
                            *(f32*)(e + 0x3C) = 0.0f;
                        }
                    }
                } else {
                    for (s32 i = 0; i < n; i++) {
                        u8* e = *(u8**)(handle + i * 4 + 0x14);
                        if (e != NULL) {
                            *(f32*)(e + 0x30) = ex;
                            *(f32*)(e + 0x34) = ey;
                            *(f32*)(e + 0x38) = ez;
                            *(f32*)(e + 0x3C) = ew;
                        }
                    }
                }
                u8** slot = *(u8***)handle;
                if (slot != NULL) {
                    *(s32*)slot = 0;
                    *(u8**)handle = NULL;
                }
            }
        }

        // Impact sound / animation through the context's virtual slot 0x50.
        u8* o9 = *(u8**)(arg1 + 0x24);
        u8* def = *(u8**)(o9 + 0x228);
        u8* cx = *(u8**)(arg1 + 0x2C);
        u8* snd = *(u8**)(def + 0x14C);
        if (*(f32*)(snd + 0x48) > 0.001f) {
            s32 id = (def == NULL) ? 0xC : *(s32*)(snd + 0x4C);
            s32 cf = *(s32*)(*(u8**)(cx + 0x4) + 0x1C);
            if (cf & 0x10) {
                *(s32*)(cx + 0x60C) = *(s32*)(def + 0x8);
            } else if (cf & 0x40) {
                *(s32*)(cx + 0x1A8) = *(s32*)(def + 0x8);
            }
            (*(s32(**)(u8*, u8*, s32, s32, s32))(*(u8**)cx + 0x50))(cx, o9, id, 1, 0);
        }
    } else if (*(u8**)(arg1 + 0x30) != NULL) {
        // Keep the hit that lies closest along the direction arg1[0..2].
        u8* o = *(u8**)(arg1 + 0x24);
        f32 ax = *(f32*)(arg1 + 0x18) - *(f32*)(o + 0x1A0);
        f32 ay = *(f32*)(arg1 + 0x1C) - *(f32*)(o + 0x1A4);
        f32 az = *(f32*)(arg1 + 0x20) - *(f32*)(o + 0x1A8);
        f32 bx = sp108[0] - *(f32*)(o + 0x1A0);
        f32 by = sp108[1] - *(f32*)(o + 0x1A4);
        f32 bz = sp108[2] - *(f32*)(o + 0x1A8);
        f32 dx = *(f32*)(arg1 + 0x0);
        f32 dy = *(f32*)(arg1 + 0x4);
        f32 dz = *(f32*)(arg1 + 0x8);
        f32 dotA = az * dz + (ax * dx + ay * dy);
        f32 dotB = bz * dz + (bx * dx + by * dy);
        if (dotB < dotA) {
            *(u8**)(arg1 + 0x30) = ctx;
            *(f32*)(arg1 + 0x18) = sp108[0];
            *(f32*)(arg1 + 0x1C) = sp108[1];
            *(f32*)(arg1 + 0x20) = sp108[2];
        }
    } else {
        *(u8**)(arg1 + 0x30) = ctx;
        *(f32*)(arg1 + 0x18) = sp108[0];
        *(f32*)(arg1 + 0x1C) = sp108[1];
        *(f32*)(arg1 + 0x20) = sp108[2];
    }
    return 1;
}

// 0x800D5DE4
// Depth-first search through a node graph: each node has up to 4 edges (+0x4..+0x10, with
// edge-blocked flags at +0x24..+0x30). `arg0` is the visited list (count at +0x400, nodes from
// +0x4). Returns the first reachable node id (+0x0) whose flag byte +0x64 is set or whose
// type (+0x40) is 0x10, or 0.
s32 fn_800D5DE4(u8* arg0, u8* arg1, s32 arg2) {
    if (arg1 == NULL) return 0;
    if (*(u8*)(arg1 + 0x64) != 0) return 0;

    s32 result = 0;
    u32 mask = 0;
    for (s32 i = 0; i < 4; i++) {
        if (*(s32*)(arg1 + 0x4 + i * 4) != 0 &&
            (*(s32*)(arg1 + 0x24 + i * 4) == 0 || arg2 != 0)) {
            mask |= 1u << i;
        }
    }

    u8* edge = arg1;
    for (s32 i = 0; i < 4; i++, edge += 4) {
        if (!(mask & (1u << i))) continue;
        u8* next = NULL;
        u8* e = *(u8**)(edge + 0x4);
        if (e != NULL) {
            next = *(u8**)(e + 0x4);
            if (arg1 == next) next = *(u8**)(e + 0x8);
        }
        if (next == NULL) continue;
        if (*(u8*)(next + 0x64) != 0 || *(s32*)(next + 0x40) == 0x10) {
            return *(s32*)(next + 0x0);
        }
        s32 count = *(s32*)(arg0 + 0x400);
        BOOL seen = FALSE;
        for (s32 k = 0; k < count; k++) {
            if (*(u8**)(arg0 + 0x4 + k * 4) == next) {
                seen = TRUE;
                break;
            }
        }
        if (seen) continue;
        *(u8**)(arg0 + 0x4 + count * 4) = next;
        *(s32*)(arg0 + 0x400) = *(s32*)(arg0 + 0x400) + 1;
        result = fn_800D5DE4(arg0, next, arg2);
        if (result != 0) return result;
    }
    return result;
}

// 0x801AA218
// UI key frame interpolation: evaluates the 4-float value between key frames `k0` and `k1` at
// time `pos` into `out` (via `buf`, or a local when null). The mode at k0+0x20 picks the curve:
// 0 linear, 1 cosine ease, 2 hold, 3 quadratic, 4 ease-out; others log an error.
void fn_801AA218(u8* out, u8* k0, u8* k1, f32* buf, f32 pos) {
    f32 local[4];
    if (buf == NULL) buf = local;

    f32 t0 = *(f32*)(k0 + 0x0);
    f32 span = *(f32*)(k1 + 0x0) - t0;
    f32 t;
    if (span != 0.0f) {
        t = (pos - t0) / span;
    } else {
        t = 1.0f;
    }

    s32 mode = *(s32*)(k0 + 0x20);
    switch (mode) {
    case 0:
        for (int i = 0; i < 4; i++) {
            f32 a = *(f32*)(k0 + 0x10 + i * 4);
            f32 b = *(f32*)(k1 + 0x10 + i * 4);
            buf[i] = (b - a) * t + a;
        }
        break;
    case 1: {
        f32 w = 0.5f * (1.0f + cosf(1.5707964f * (0.011111111f * (180.0f * t))));
        f32 iw = 1.0f - w;
        for (int i = 0; i < 4; i++) {
            f32 a = *(f32*)(k0 + 0x10 + i * 4);
            f32 b = *(f32*)(k1 + 0x10 + i * 4);
            buf[i] = a * w + b * iw;
        }
        break;
    }
    case 2:
        for (int i = 0; i < 4; i++) buf[i] = *(f32*)(k0 + 0x10 + i * 4);
        break;
    case 3: {
        f32 u = t * t;
        f32 iu = 1.0f - u;
        for (int i = 0; i < 4; i++) {
            f32 a = *(f32*)(k0 + 0x10 + i * 4);
            f32 b = *(f32*)(k1 + 0x10 + i * 4);
            buf[i] = a * iu + b * u;
        }
        break;
    }
    case 4: {
        f32 u = 2.0f * t + -(t * t);
        f32 iu = 1.0f - u;
        for (int i = 0; i < 4; i++) {
            f32 a = *(f32*)(k0 + 0x10 + i * 4);
            f32 b = *(f32*)(k1 + 0x10 + i * 4);
            buf[i] = a * iu + b * u;
        }
        break;
    }
    default:
        fn_8018D370("UIKeyFrame : invalid iterp. type %d ...\n", mode);
        break;
    }
    *(f32*)(out + 0x0) = buf[0];
    *(f32*)(out + 0x4) = buf[1];
    *(f32*)(out + 0x8) = buf[2];
    *(f32*)(out + 0xC) = buf[3];
}

// 0x801D79D4
// Debug draw of an arrow / cone: builds a frame whose axis is `dir` (arg2) with a scale from
// (arg5, arg6), moves it to `arg1`, fills the 0x120-byte vertex buffer with `color` (arg3; the
// later entries use a half-brightness variant) and queues it (arg4 picks the draw mode).
void fn_801D79D4(u8* arg0, u8* arg1, u8* arg2, u32 color, s32 mode, f32 scale, f32 angle) {
    if (fn_801CB938() == 0) return;

    u8 m[0x40];
    u8 mT[0x40];
    Vec3 vScale, vTmp, vSide, vUp, vAxis;
    Vec3 tmp;

    fn_800526D0(m);
    f32 s = scale * fn_800C11F0(0.63661975f * angle);
    fn_80052638(&vScale);
    fn_800359F4((u8*)&tmp, s, s, scale);
    Vec3Copy(&vScale, &tmp);
    fn_800557B4(mT);
    fn_8005535C(m, mT);
    fn_80052638(&vSide);
    fn_80052638(&vUp);
    Vec3Assign(&vAxis, (const Vec3*)arg2);

    // Pick a helper vector from the axis' smallest component.
    f32 ax = FloatAbs(vAxis.x);
    f32 ay = FloatAbs(vAxis.y);
    f32 az = FloatAbs(vAxis.z);
    if (ax < ay && ax < az) {
        fn_80149094(&tmp);
    } else if (ay < ax && ay < az) {
        fn_800C09C0(&tmp);
    } else if (az < ax && az < ay) {
        Vec3SetUnitZ(&tmp);
    } else if (ax > ay && ax > az) {
        fn_800C09C0(&tmp);
    } else if (ax > ay && ay > az) {
        Vec3SetUnitZ(&tmp);
    } else if (az > ax && az > ay) {
        fn_80149094(&tmp);
    } else {
        Vec3SetUnitZ(&tmp);
    }
    Vec3Copy(&vUp, &tmp);

    Vec3NormalizeTo(&tmp, &vAxis);
    Vec3Copy(&vAxis, &tmp);
    fn_800C11A4(&tmp, &vAxis, &vUp);
    Vec3Copy(&vSide, &tmp);
    Vec3NormalizeTo(&tmp, &vSide);
    Vec3Copy(&vSide, &tmp);
    fn_800C11A4(&tmp, &vSide, &vAxis);
    Vec3Copy(&vUp, &tmp);
    Vec3NormalizeTo(&tmp, &vUp);
    Vec3Copy(&vUp, &tmp);

    Vec3Copy((Vec3*)fn_8016EB20(m), &vSide);
    Vec3Copy((Vec3*)fn_8016EB4C(m), (const Vec3*)arg2);
    Vec3Copy((Vec3*)fn_800554D4(m), &vUp);
    fn_801D7E9C((f32*)m, (f32*)&vScale);
    fn_8007FD98(m, arg1);

    if (lbl_805FB824 == 0) {
        lbl_805FB824 = 1;
        fn_801D77C0();
    }
    fn_80174A24(lbl_8047D378, color);
    u32 dim = (color & 0xFFFFFF) | ((color >> 1) & 0x7F000000);
    for (s32 i = 1; i < 0x12; i++) fn_80174A24(lbl_8047D378 + i * 0x10, dim);

    fn_800446BC(arg0, 0);
    fn_8010AB3C();
    fn_80154358();
    u8* buf = fn_80044634(0x120);
    memcpy(buf, lbl_8047D378, 0x120u);
    fn_80155724(m);
    if (mode != 0) {
        fn_80152620(0xA0, 0x12, buf);
    } else {
        fn_80152620(0xB0, 0x12, buf);
    }
    fn_8010AB70();
    fn_80154318();
}

}  // extern "C"
