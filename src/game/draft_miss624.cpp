// Small functions at 0x800BDC68, 0x800C0660, 0x800C6F18, 0x801A6880.
#include "types.h"
#include "engine/math.h"

extern "C" {
void  EulerToQuat(Quat* out, const Vec3* euler);                  
u32   RandomU32(void);                                              // 0x80143460
u8*   fn_80073454(void);                                            // 0x80073454 (camera block; frustum planes at +0xC0)
void  fn_80073A7C(void* camData, const f32* pos3, f32* outX, f32* outY);  // 0x80073A7C (world pos -> screen pixels)
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);         // 0x80146180 (rotate quat about axis)
s32   fn_80156A80(void);                                            // 0x80156A80 (screen width)
s32   fn_80156A88(void);                                            // 0x80156A88 (screen height)
u8*  fn_8019552C(u8* list, s32 index);                             // 0x8019552C (pointer to list slot)
u8*   fn_801A9EFC(u8* element, s32 type);                           // 0x801A9EFC
void  fn_801B6F34(u8* p);                                           // 0x801B6F34
void  fn_801B7020(u8* p);                                           // 0x801B7020
f64   fn_8033F1F4(f64 x);                                           // 0x8033F1F4 (MSL acos)

extern f32 lbl_805FE2CC;  // 0x805FE2CC (2^-32)
extern f32 lbl_805FE2D0;  // 0x805FE2D0 (-1.0)
extern f32 lbl_805FE2D4;  // 0x805FE2D4 (2.0)
extern f32 lbl_805FE348;  // 0x805FE348 (0.0)
extern f32 lbl_805FE34C;  // 0x805FE34C (1.0)
extern f32 lbl_805FE378;  // 0x805FE378 (-1.0)
extern f32 lbl_805FE37C;  // 0x805FE37C (2/pi)
extern f32 lbl_805FE448;  // 0x805FE448 (-1.0)
extern f32 lbl_805FE44C;  // 0x805FE44C (0.2)
extern f32 lbl_805FE450;  // 0x805FE450 (1.2)
extern f32 lbl_805FFA0C;  // 0x805FFA0C (0.0)
extern f32 lbl_805FFA24;  // 0x805FFA24 (5.0)

// Random float in [-1, 1) from a 32-bit random number.
static inline f32 RandSigned(void) {
    return lbl_805FE2D0 + lbl_805FE2D4 * (lbl_805FE2CC * (f32)RandomU32());
}

// 0x800BDC68
// Drifting point: advances the object's position by dir*speed*dt; if it ends up farther than
// the allowed radius from its anchor (+0x280), it is clamped onto the sphere and a new random
// unit direction is picked. Then writes the position into the scene node and flags it dirty.
void fn_800BDC68(u8* arg0, f32 dt) {
    if (*(u8*)(arg0 + 0x264) != 0 && *(s32*)(arg0 + 0x270) == 1) {
        f32 step = dt * *(f32*)(arg0 + 0x28C);
        u8* node = *(u8**)(arg0 + 0x4);
        f32 px = *(f32*)(node + 0x0) + *(f32*)(arg0 + 0x274) * step;
        f32 py = *(f32*)(node + 0x4) + *(f32*)(arg0 + 0x278) * step;
        f32 pz = *(f32*)(node + 0x8) + *(f32*)(arg0 + 0x27C) * step;
        Vec3 d;
        d.x = px - *(f32*)(arg0 + 0x280);
        d.y = py - *(f32*)(arg0 + 0x284);
        d.z = pz - *(f32*)(arg0 + 0x288);
        f32 radius = *(f32*)(arg0 + 0x290);
        if (d.z * d.z + (d.x * d.x + d.y * d.y) > radius * radius) {
            Vec3 n = Vec3Normalized(d);
            f32 r = *(f32*)(arg0 + 0x290);
            px = *(f32*)(arg0 + 0x280) + n.x * r;
            py = *(f32*)(arg0 + 0x284) + n.y * r;
            pz = *(f32*)(arg0 + 0x288) + n.z * r;
            // New random direction, stored normalised (x from the 3rd draw, y 2nd, z 1st).
            f32 rz = RandSigned();
            f32 ry = RandSigned();
            f32 rx = RandSigned();
            *(f32*)(arg0 + 0x278) = ry;
            *(f32*)(arg0 + 0x27C) = rz;
            *(f32*)(arg0 + 0x274) = rx;
            Vec3 dir;
            dir.x = rx;
            dir.y = ry;
            dir.z = rz;
            Vec3 nd = Vec3Normalized(dir);
            *(f32*)(arg0 + 0x274) = nd.x;
            *(f32*)(arg0 + 0x278) = nd.y;
            *(f32*)(arg0 + 0x27C) = nd.z;
        }
        node = *(u8**)(arg0 + 0x4);
        *(f32*)(node + 0x0) = px;
        *(f32*)(node + 0x4) = py;
        *(f32*)(node + 0x8) = pz;
        node = *(u8**)(arg0 + 0x4);
        *(u32*)(node + 0x20) |= 0x8000;
        node = *(u8**)(arg0 + 0x4);
        *(u32*)(node + 0x20) |= 0x10000000;
        node = *(u8**)(arg0 + 0x4);
        *(u32*)(node + 0x20) |= 0x4000;
        node = *(u8**)(arg0 + 0x4);
        *(u32*)(node + 0x20) |= 0x10000000;
    }
    fn_801B6F34(arg0);
}

// 0x800C0660
// Aims the object's orientation (quat at +0x158) toward the target point arg1: the angle between
// +Z and (target - object position at +0x134), scaled by 2/pi, signed by which side of x it is
// on, is applied as a rotation about +Z on top of the base euler angles of the active child.
// The quat is then copied into the four child nodes and they are flagged dirty.
void fn_800C0660(u8* arg0, f32* arg1) {
    Vec3 d;
    d.x = arg1[0] - *(f32*)(arg0 + 0x134);
    d.y = arg1[1] - *(f32*)(arg0 + 0x138);
    d.z = arg1[2] - *(f32*)(arg0 + 0x13C);

    // |(0,0,1)| and the dot product with it (the reference axis).
    f32 dot = lbl_805FE348 * d.x + lbl_805FE348 * d.y;
    dot = lbl_805FE34C * d.z + dot;
    f32 lenA = lbl_805FE348;
    f32 lenSqA = lbl_805FE34C * lbl_805FE34C + (lbl_805FE348 * lbl_805FE348 + lbl_805FE348 * lbl_805FE348);
    if (!(lenSqA <= lbl_805FE348)) lenA = sqrtf(lenSqA);
    f32 lenD = lbl_805FE348;
    f32 lenSqD = d.z * d.z + (d.x * d.x + d.y * d.y);
    if (!(lenSqD <= lbl_805FE348)) lenD = sqrtf(lenSqD);

    f32 angle = lbl_805FE348;
    f32 lens = lenA * lenD;
    if (!(lens <= lbl_805FE348)) {
        f32 c = dot / lens;
        if (c < lbl_805FE378) c = lbl_805FE378;
        else if (c > lbl_805FE34C) c = lbl_805FE34C;
        if (c < lbl_805FE378) c = lbl_805FE378;
        else if (c > lbl_805FE34C) c = lbl_805FE34C;
        angle = lbl_805FE37C * (f32)fn_8033F1F4((f64)c);
    }
    if (arg1[0] < *(f32*)(arg0 + 0x134)) angle = -angle;

    u8* child = *(u8**)(arg0 + 4 + *(s32*)arg0 * 4);
    Vec3 euler;
    euler.x = *(f32*)(child + 0xD4) * lbl_805FE37C;
    euler.y = *(f32*)(child + 0xD8) * lbl_805FE37C;
    euler.z = *(f32*)(child + 0xDC) * lbl_805FE37C;
    EulerToQuat((Quat*)(arg0 + 0x158), &euler);

    Vec3 axis;
    axis.x = lbl_805FE348;
    axis.y = lbl_805FE348;
    axis.z = lbl_805FE34C;
    fn_80146180(arg0 + 0x158, &axis, angle);

    for (s32 i = 0; i < 4; i++) {
        u8* slot = *(u8**)(arg0 + 4 + i * 4);
        u8* node = *(u8**)(slot + 4);
        *(f32*)(node + 0xC) = *(f32*)(arg0 + 0x158);
        *(f32*)(node + 0x10) = *(f32*)(arg0 + 0x15C);
        *(f32*)(node + 0x14) = *(f32*)(arg0 + 0x160);
        *(f32*)(node + 0x18) = *(f32*)(arg0 + 0x164);
        node = *(u8**)(slot + 4);
        *(u32*)(node + 0x20) |= 0x8000;
        node = *(u8**)(slot + 4);
        *(u32*)(node + 0x20) |= 0x10000000;
        node = *(u8**)(slot + 4);
        *(u32*)(node + 0x20) |= 0x4000;
        node = *(u8**)(slot + 4);
        *(u32*)(node + 0x20) |= 0x10000000;
    }
    fn_801B7020(*(u8**)(arg0 + 4 + *(s32*)arg0 * 4));
}

// 0x800C6F18
// Visibility state machine for an entity. State 2: if its position is inside all six frustum
// planes, go to state 3. States 3/4: project the position to the screen and, when it falls
// outside the screen rectangle (with a 20% margin), go to state 5.
void fn_800C6F18(u8* arg0) {
    u8* target = *(u8**)(arg0 + 0x4);
    if (target == NULL) return;
    u8* other = *(u8**)(arg0 + 0x8);
    if (other != NULL && *(u8*)(other + 0x26D) == 0) return;

    s32 state = *(s32*)(arg0 + 0x10);
    if (state == 2) {
        const f32* pos = *(f32**)(target + 0x4);
        f32 p[3];
        p[0] = pos[0];
        p[1] = pos[1];
        p[2] = pos[2];
        const f32* plane = (const f32*)(fn_80073454() + 0xC0);
        for (s32 i = 6; i != 0; i--, plane += 4) {
            f32 dist = p[2] * plane[2] + (p[0] * plane[0] + p[1] * plane[1]);
            if (dist > plane[3]) return;
        }
        *(s32*)(arg0 + 0x10) = 3;
        return;
    }
    if ((u32)(state - 3) <= 1u) {
        f32 sx = lbl_805FE448;
        f32 sy = lbl_805FE448;
        const f32* pos = *(f32**)(*(u8**)(arg0 + 0x4) + 0x4);
        f32 p[3];
        p[0] = pos[0];
        p[1] = pos[1];
        p[2] = pos[2];
        fn_80073A7C(NULL, p, &sx, &sy);
        if ((f32)(s32)sx < -(lbl_805FE44C * (f32)fn_80156A80()) ||
            (f32)(s32)sy < -(lbl_805FE44C * (f32)fn_80156A88()) ||
            (f32)(s32)sx > lbl_805FE450 * (f32)fn_80156A80() ||
            (f32)(s32)sy > lbl_805FE450 * (f32)fn_80156A88()) {
            *(s32*)(arg0 + 0x10) = 5;
        }
    }
}

// 0x801A6880
// Grid-cell update: positions the cell's widget (+0x1B4) at its grid cell's scroll offset
// (x,y,z), adds the cell image's offset when flag 0x01000000 is set, and copies a second offset
// (0x400000 flag) into the widget; then calls the widget's virtual slot 0x2C with 8. Returns 0.
s32 fn_801A6880(u8* arg0) {
    s32 col = *(s32*)(arg0 + 0x14C) + *(s32*)(arg0 + 0x15C);
    u8* widget = *(u8**)(arg0 + 0x1B4);
    u8* nodeA = *(u8**)(widget + 0x190);
    if (col >= *(s32*)(arg0 + 0x13C)) col -= *(s32*)(arg0 + 0x13C);
    f32* base = (f32*)fn_8019552C(*(u8**)(arg0 + 0x1B8), col);
    s32 row0 = *(s32*)(arg0 + 0x150);
    s32 col0 = *(s32*)(arg0 + 0x14C);
    f32 x = *base + (f32)row0 * *(f32*)(arg0 + 0x180);
    f32 z = *(f32*)(arg0 + 0x70) - lbl_805FFA24;
    f32 y = lbl_805FFA0C + (f32)col0 * *(f32*)(arg0 + 0x184);

    if (*(u32*)(*(u8**)(arg0 + 0x1B4) + 0x64) & 0x01000000) {
        s32 cx = col0 + *(s32*)(arg0 + 0x15C);
        s32 cy = row0 + *(s32*)(arg0 + 0x160);
        if (cx < 0) cx += *(s32*)(arg0 + 0x13C);
        else if (cx >= *(s32*)(arg0 + 0x13C)) cx -= *(s32*)(arg0 + 0x13C);
        if (cy < 0) cy += *(s32*)(arg0 + 0x140);
        else if (cy >= *(s32*)(arg0 + 0x140)) cy -= *(s32*)(arg0 + 0x140);
        u8** slot = (u8**)fn_8019552C(*(u8**)(arg0 + 0x1BC), cy + cx * *(s32*)(arg0 + 0x140));
        u8* cell = slot != NULL ? *slot : NULL;
        if (cell != NULL) {
            u8* img = fn_801A9EFC(cell, 2);
            if (img != NULL) {
                x += *(f32*)(img + 0x68);
                y += *(f32*)(img + 0x6C);
            }
        }
    }
    widget = *(u8**)(arg0 + 0x1B4);
    if (x != *(f32*)(widget + 0x68) || y != *(f32*)(widget + 0x6C) || z != *(f32*)(widget + 0x70)) {
        *(f32*)(widget + 0x68) = x;
        *(f32*)(widget + 0x6C) = y;
        *(f32*)(widget + 0x70) = z;
    }
    if (*(u32*)(*(u8**)(arg0 + 0x1B4) + 0x64) & 0x00400000) {
        s32 cx = *(s32*)(arg0 + 0x14C) + *(s32*)(arg0 + 0x15C);
        s32 cy = *(s32*)(arg0 + 0x150) + *(s32*)(arg0 + 0x160);
        if (cx < 0) cx += *(s32*)(arg0 + 0x13C);
        else if (cx >= *(s32*)(arg0 + 0x13C)) cx -= *(s32*)(arg0 + 0x13C);
        if (cy < 0) cy += *(s32*)(arg0 + 0x140);
        else if (cy >= *(s32*)(arg0 + 0x140)) cy -= *(s32*)(arg0 + 0x140);
        u8** slot = (u8**)fn_8019552C(*(u8**)(arg0 + 0x1BC), cy + cx * *(s32*)(arg0 + 0x140));
        u8* cell = slot != NULL ? *slot : NULL;
        u8* img = cell != NULL ? fn_801A9EFC(cell, 2) : NULL;
        if (img != NULL) {
            *(f32*)(nodeA + 0x74) = *(f32*)(img + 0x74);
            *(f32*)(nodeA + 0x78) = *(f32*)(img + 0x78);
            *(f32*)(nodeA + 0x7C) = *(f32*)(img + 0x7C);
        } else {
            *(f32*)(nodeA + 0x74) = *(f32*)(nodeA + 0x20);
            *(f32*)(nodeA + 0x78) = *(f32*)(nodeA + 0x24);
            *(f32*)(nodeA + 0x7C) = *(f32*)(nodeA + 0x28);
        }
    }
    widget = *(u8**)(arg0 + 0x1B4);
    typedef void (*Fn)(u8* self, s32 arg);
    (*(Fn*)(*(u8**)(widget + 0x138) + 0x2C))(widget, 8);
    return 0;
}

}  // extern "C"
