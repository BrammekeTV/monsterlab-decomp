// UI entity vertex colour setup, UIEntity bone-offset placement, and a ribbon/quad builder.
#include "types.h"
#include <math.h>
#include "engine/math.h"

extern "C" {
void fn_801455E0(f32* out, u8* quat);                          // 0x801455E0
void fn_80146180(void* quat, const Vec3* axis, f32 angle);     // 0x80146180 (rotate quat about axis)
void QuatGetForward(Vec3* out, const Quat* q);                 // 0x801454CC
void fn_800DB944(f32* out, u8* obj);                           // 0x800DB944
s32  fn_8018BDE0(u8* p, s32 key);                              // 0x8018BDE0
void fn_8018D2B8(const char* fmt, ...);                        // 0x8018D2B8 (debug log)
void* fn_801B7198(void* anim, s32 a);                          // 0x801B7198
extern const char lbl_80383050[];   // 0x80383050 ("UIEntity : unable to locate root bone position ...")
extern const char lbl_80383084[];   // 0x80383084 ("UIEntity : unable to locate offset bone ...")
}
extern "C++" void* gScreenSets;     // 0x805FB654

typedef u8* (*GetModelFn)(u8* self);

static inline s32 ClampByte(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

// Quantise four floats (0..1) to a clamped RGBA word, byte-reversed like the game stores it.
static inline u32 PackColorSwapped(f32 r, f32 g, f32 b, f32 a) {
    u32 R = (u32)ClampByte((s32)(255.0f * r));
    u32 G = (u32)ClampByte((s32)(255.0f * g));
    u32 B = (u32)ClampByte((s32)(255.0f * b));
    u32 A = (u32)ClampByte((s32)(255.0f * a));
    return (R << 24) | (G << 16) | (B << 8) | A;
}

extern "C" {

// 0x801A1238
// Copies four colours (16 floats) from arg1 into the entity at +0x1A0..+0x1DC, then fills the
// four vertices (stride 0x18 from +0x1E8): packed colour, u/v and the screen xy copied from
// +0x180..+0x19C. Returns 0.
s32 fn_801A1238(u8* arg0, f32* arg1) {
    *(f32*)(arg0 + 0x1A0) = arg1[0];
    *(f32*)(arg0 + 0x1A4) = arg1[1];
    *(f32*)(arg0 + 0x1A8) = arg1[2];
    *(f32*)(arg0 + 0x1AC) = arg1[3];
    *(f32*)(arg0 + 0x1B0) = arg1[4];
    *(f32*)(arg0 + 0x1B4) = arg1[5];
    *(f32*)(arg0 + 0x1B8) = arg1[6];
    *(f32*)(arg0 + 0x1BC) = arg1[7];
    *(f32*)(arg0 + 0x1C0) = arg1[8];
    *(f32*)(arg0 + 0x1C4) = arg1[9];
    *(f32*)(arg0 + 0x1C8) = arg1[10];
    *(f32*)(arg0 + 0x1CC) = arg1[11];
    *(f32*)(arg0 + 0x1D0) = arg1[12];
    *(f32*)(arg0 + 0x1D4) = arg1[13];
    *(f32*)(arg0 + 0x1D8) = arg1[14];
    *(f32*)(arg0 + 0x1DC) = arg1[15];
    *(f32*)(arg0 + 0x1E0) = *(f32*)(arg0 + 0x180);
    *(f32*)(arg0 + 0x1E4) = *(f32*)(arg0 + 0x184);

    *(u32*)(arg0 + 0x1E8) = PackColorSwapped(arg1[0], arg1[1], arg1[2], arg1[3]);
    *(f32*)(arg0 + 0x1EC) = 0.0f;
    *(f32*)(arg0 + 0x1F0) = 0.0f;
    *(f32*)(arg0 + 0x1F4) = 1.0f;
    *(f32*)(arg0 + 0x1F8) = *(f32*)(arg0 + 0x188);
    *(f32*)(arg0 + 0x1FC) = *(f32*)(arg0 + 0x18C);

    *(u32*)(arg0 + 0x200) = PackColorSwapped(*(f32*)(arg0 + 0x1B0), *(f32*)(arg0 + 0x1B4),
                                             *(f32*)(arg0 + 0x1B8), *(f32*)(arg0 + 0x1BC));
    *(f32*)(arg0 + 0x204) = 1.0f;
    *(f32*)(arg0 + 0x208) = 0.0f;
    *(f32*)(arg0 + 0x20C) = 1.0f;
    *(f32*)(arg0 + 0x210) = *(f32*)(arg0 + 0x190);
    *(f32*)(arg0 + 0x214) = *(f32*)(arg0 + 0x194);

    *(u32*)(arg0 + 0x218) = PackColorSwapped(*(f32*)(arg0 + 0x1C0), *(f32*)(arg0 + 0x1C4),
                                             *(f32*)(arg0 + 0x1C8), *(f32*)(arg0 + 0x1CC));
    *(f32*)(arg0 + 0x21C) = 0.0f;
    *(f32*)(arg0 + 0x220) = 1.0f;
    *(f32*)(arg0 + 0x224) = 1.0f;
    *(f32*)(arg0 + 0x228) = *(f32*)(arg0 + 0x198);
    *(f32*)(arg0 + 0x22C) = *(f32*)(arg0 + 0x19C);

    *(u32*)(arg0 + 0x230) = PackColorSwapped(*(f32*)(arg0 + 0x1D0), *(f32*)(arg0 + 0x1D4),
                                             *(f32*)(arg0 + 0x1D8), *(f32*)(arg0 + 0x1DC));
    *(f32*)(arg0 + 0x234) = 1.0f;
    *(f32*)(arg0 + 0x238) = 1.0f;
    *(f32*)(arg0 + 0x23C) = 1.0f;
    return 0;
}

// 0x801A31BC
// UIEntity: places the anim's root node. Reads the base orientation/origin/matrix from the
// screen-set data (gScreenSets +0x20..+0x68), optionally subtracts the offset-bone displacement
// (bone index at +0x144), rotates the quat by the entity's three angles (+0x90..+0x98), and writes
// the resulting position and quat into the anim's node, flagging both as dirty. Returns 0.
s32 fn_801A31BC(u8* arg0) {
    f32 off[3];      // sp50: offset-bone displacement
    f32 scl[3];      // sp5C: entity scale (+0x68..+0x70)
    f32 quat[4];     // sp68
    f32 origin[3];   // sp78
    f32 m[12];       // spD0: 3x3 matrix rows (+ 3 unused floats)
    u8* base = *(u8**)&gScreenSets;
    u8* h;
    u8* node;

    off[0] = 0.0f;
    off[1] = 0.0f;
    off[2] = 0.0f;
    quat[0] = *(f32*)(base + 0x2C);
    quat[1] = *(f32*)(base + 0x30);
    quat[2] = *(f32*)(base + 0x34);
    quat[3] = *(f32*)(base + 0x38);
    origin[0] = *(f32*)(base + 0x20);
    origin[1] = *(f32*)(base + 0x24);
    origin[2] = *(f32*)(base + 0x28);
    for (s32 i = 0; i < 12; i++) m[i] = *(f32*)(base + 0x3C + i * 4);

    if (*(s32*)(arg0 + 0x144) != 0 &&
        (h = (u8*)fn_801B7198(arg0 + 0x140, 0),
         (*(GetModelFn*)(*(u8**)h + 0x10))(h) != NULL)) {
        h = (u8*)fn_801B7198(arg0 + 0x140, 0);
        u8* rootBones = *(u8**)(*(u8**)(*(GetModelFn*)(*(u8**)h + 0x10))(h) + 4);
        u8* r29 = rootBones + 0x24;
        if (r29 == NULL) {
            fn_8018D2B8(lbl_80383050);
            *(s32*)(arg0 + 0x144) = 0;
        } else {
            h = (u8*)fn_801B7198(arg0 + 0x140, 0);
            s32 idx = fn_8018BDE0((*(GetModelFn*)(*(u8**)h + 0x10))(h), *(s32*)(arg0 + 0x144));
            if (idx <= -1) {
                fn_8018D2B8(lbl_80383084);
                *(s32*)(arg0 + 0x144) = 0;
            } else {
                h = (u8*)fn_801B7198(arg0 + 0x140, 0);
                u8* bones = *(u8**)(*(u8**)(*(GetModelFn*)(*(u8**)h + 0x10))(h) + 4);
                u8* r5 = bones + idx * 0x30 + 0x24;
                if (r5 == NULL) {
                    fn_8018D2B8(lbl_80383050);
                    *(s32*)(arg0 + 0x144) = 0;
                } else {
                    off[0] = *(f32*)(r5 + 0x0);
                    off[1] = *(f32*)(r5 + 0x4);
                    off[2] = *(f32*)(r5 + 0x8);
                    u8* t = (u8*)fn_801B7198(arg0 + 0x140, 0);
                    off[0] = off[0] * *(f32*)(t + 0x8) - *(f32*)(r29 + 0x0);
                    off[1] = off[1] * *(f32*)(t + 0xC) - *(f32*)(r29 + 0x4);
                    off[2] = off[2] * *(f32*)(t + 0x10) - *(f32*)(r29 + 0x8);
                }
            }
        }
    } else {
        *(s32*)(arg0 + 0x144) = 0;
    }

    f32 k = 0.63661975f;
    f32 angX = k * *(f32*)(arg0 + 0x90);
    f32 angY = 2.0f + k * *(f32*)(arg0 + 0x94);
    f32 angZ = k * (-1.0f * *(f32*)(arg0 + 0x98));
    scl[0] = *(f32*)(arg0 + 0x68);
    scl[1] = *(f32*)(arg0 + 0x6C);
    scl[2] = *(f32*)(arg0 + 0x70);

    Vec3 axC;
    Vec3 fwd;
    fn_801455E0((f32*)&axC, (u8*)quat);
    QuatGetForward(&fwd, (const Quat*)quat);
    Vec3 axY = QuatGetAxisY((const Quat*)quat);
    fn_80146180(quat, &axY, angZ);
    fn_80146180(quat, &axC, angY);
    fn_80146180(quat, &fwd, angX);

    f32 a0 = m[0] * scl[0], a1 = m[1] * scl[0], a2 = m[2] * scl[0];
    f32 b0 = m[3] * scl[1], b1 = m[4] * scl[1], b2 = m[5] * scl[1];
    f32 c0 = m[6] * scl[2], c1 = m[7] * scl[2], c2 = m[8] * scl[2];
    f32 px = (b0 + a0 + c0 + origin[0]) - off[0];
    f32 py = (b1 + a1 + c1 + origin[1]) - off[1];
    f32 pz = (b2 + a2 + c2 + origin[2]) - off[2];

    h = (u8*)fn_801B7198(arg0 + 0x140, 0);
    node = *(u8**)(h + 4);
    *(f32*)(node + 0x0) = px;
    *(f32*)(node + 0x4) = py;
    *(f32*)(node + 0x8) = pz;
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x10000000;

    h = (u8*)fn_801B7198(arg0 + 0x140, 0);
    node = *(u8**)(h + 4);
    *(f32*)(node + 0xC) = quat[0];
    *(f32*)(node + 0x10) = quat[1];
    *(f32*)(node + 0x14) = quat[2];
    *(f32*)(node + 0x18) = quat[3];
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(h + 4) + 0x20) |= 0x10000000;
    return 0;
}

// 0x801BF4B8
// Builds a two-sided ribbon along a polyline. For each of the *(u32*)(arg0+0x38) points (stored
// from +0x4C, stride 0xC) computes the normal as cross(point - origin, segment direction),
// normalises it and writes point +/- normal * halfWidth (halfWidth = *(f32*)(owner+0x54)) into the
// vertex array at *(arg0+0x48), stride 0x30: +0xC..+0x14 and +0x24..+0x2C.
void fn_801BF4B8(u8* arg0) {
    u8* owner = *(u8**)arg0;
    f32 half = *(f32*)(owner + 0x54);
    f32 origin[3];   // sp68
    fn_800DB944(origin, owner);

    u8* p = arg0;
    s32 off = 0;
    for (u32 i = 0; i < *(u32*)(arg0 + 0x38); i++) {
        f32 d0, d1, d2;
        if (i != 0) {
            d0 = *(f32*)(p + 0x4C) - *(f32*)(p + 0x40);
            d1 = *(f32*)(p + 0x50) - *(f32*)(p + 0x44);
            d2 = *(f32*)(p + 0x54) - *(f32*)(p + 0x48);
        } else {
            d0 = *(f32*)(p + 0x58) - *(f32*)(p + 0x4C);
            d1 = *(f32*)(p + 0x5C) - *(f32*)(p + 0x50);
            d2 = *(f32*)(p + 0x60) - *(f32*)(p + 0x54);
        }
        f32 e0 = *(f32*)(p + 0x4C) - origin[0];
        f32 e1 = *(f32*)(p + 0x50) - origin[1];
        f32 e2 = *(f32*)(p + 0x54) - origin[2];
        f32 c2 = e0 * d1 - e1 * d0;
        f32 c1 = e2 * d0 - e0 * d2;
        f32 c0 = e1 * d2 - e2 * d1;
        f32 len2 = c2 * c2 + (c0 * c0 + c1 * c1);
        f32 n0, n1, n2;
        if (len2 == 0.0f) {
            n0 = 0.0f;
            n1 = 0.0f;
            n2 = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
            n0 = c0 * inv;
            n1 = c1 * inv;
            n2 = c2 * inv;
        }
        f32 t0 = n0 * half;
        f32 t1 = n1 * half;
        f32 t2 = n2 * half;
        f32 px = *(f32*)(p + 0x4C);
        f32 py = *(f32*)(p + 0x50);
        f32 pz = *(f32*)(p + 0x54);
        u8* v = *(u8**)(arg0 + 0x48) + off;
        *(f32*)(v + 0xC) = px + t0;
        *(f32*)(v + 0x10) = py + t1;
        *(f32*)(v + 0x14) = pz + t2;
        p += 0xC;
        v = *(u8**)(arg0 + 0x48) + off;
        off += 0x30;
        *(f32*)(v + 0x24) = px - t0;
        *(f32*)(v + 0x28) = py - t1;
        *(f32*)(v + 0x2C) = pz - t2;
    }
}

}  // extern "C"
