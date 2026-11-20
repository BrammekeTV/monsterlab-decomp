// UIActivationGrid update (fn_80199E80) and the screen-position-to-world ray helper (fn_801A9174).
#include <math.h>
#include "types.h"
#include "engine/math.h"

extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" void* lbl_805FB670;    // 0x805FB670 (pad state)
extern "C++" void* lbl_805FB680;    // 0x805FB680 (active camera)
extern "C++" s32   gScreenWidth;    // viewport width
extern "C++" s32   gScreenHeight;   // viewport height

BOOL fn_801982A8(void* obj, u32 mask);   // 0x801982A8 (C++ linkage)
BOOL fn_801982E4(void* obj, u32 mask);   // 0x801982E4 (C++ linkage)

extern "C" {
u32   fn_8019552C(u8* list, s32 index);                       // 0x8019552C (leaf; returns a slot pointer)
s32   fn_8019555C(u8* list);                                  // 0x8019555C (leaf; element count)
void  fn_8018D2B8(const char* fmt, ...);                      // 0x8018D2B8
void  UIPlayAnim(void* el, u32 anim);                         // 0x8019BE40
void  fn_8016E304(void* cam);                                 // 0x8016E304
void  fn_801455E0(f32* out, u8* quat);                        // 0x801455E0
void  QuatGetForward(Vec3* out, const Quat* q);               // 0x801454CC
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);   // 0x80146180
s32   fn_801449A4(f32* mat, f32* out);                        // 0x801449A4 (3x3 matrix to euler angles)
}

// Slot pointer returned by fn_8019552C (declared u32 by the other files; pointer-sized here).
static inline u8** Slot(u8* list, s32 index) { return (u8**)(uintptr_t)fn_8019552C(list, index); }

typedef void (*VFnF32)(void*, f32);
typedef f32* (*VFnMat)(void*);

static const char kBadIndex[] = "UIActivationGrid : index (%d,%d) not valid ...\n";  // 0x803827D8

// Looks up grid cell (x,y): the grid is a list of rows, each row a list of cell pointers.
// Logs and returns NULL when the row or the cell is missing (the second log only passes x).
static inline u8* GridCell(u8* grid, s32 x, s32 y) {
    u8* list = *(u8**)(grid + 0x4);
    u8* row = NULL;
    u8** cellSlot = NULL;
    if (fn_8019555C(list) > x) {
        row = *Slot(list, x);
    }
    if (row == NULL) {
        fn_8018D2B8(kBadIndex, x, y);
        return NULL;
    }
    if (fn_8019555C(row) > y) {
        cellSlot = Slot(row, y);
    }
    if (cellSlot == NULL) {
        fn_8018D2B8(kBadIndex, x);
        return NULL;
    }
    return *cellSlot;
}

extern "C" {

// 0x80199E80
// Per-frame update of a UIActivationGrid: reads the pad (edge or held, flag bit 0), steps the
// selected cell down/up (index +0xC) and right/left (index +0x10), skipping cells without bit 0
// of +0x64 and wrapping when flag bit 1 (rows) / 2 (columns) is set, plays the unselect/select
// animations on change, plays the direction animations on the current cell and forwards dt to its
// vtable slot 0x28. Returns 0.
s32 fn_80199E80(u8* grid, f32 dt) {
    s32 up = 0;      // r30
    s32 down = 0;    // r29
    s32 left = 0;    // r28
    s32 right = 0;   // r27
    u32 flags = *(u32*)(grid + 0x0);
    if (flags & 0x10) {
        return 0;
    }
    if (!(flags & 0x8)) {
        // First update: widest row length (the row index read is always +0xC, as in the original).
        *(s32*)(grid + 0x8) = 0;
        s32 rows = fn_8019555C(*(u8**)(grid + 0x4));
        for (s32 i = 0; i < rows; i++) {
            u8* row = *Slot(*(u8**)(grid + 0x4), *(s32*)(grid + 0xC));
            s32 n = row ? fn_8019555C(row) : 0;
            if (n > *(s32*)(grid + 0x8)) {
                *(s32*)(grid + 0x8) = n;
            }
        }
        *(u32*)(grid + 0x0) |= 0x8;
    }
    if (*(u32*)(grid + 0x0) & 1) {
        if (fn_801982E4(lbl_805FB670, 0x1) || fn_801982E4(lbl_805FB670, 0x4000)) {
            up = 1;
        } else if (fn_801982E4(lbl_805FB670, 0x2) || fn_801982E4(lbl_805FB670, 0x8000)) {
            down = 1;
        }
        if (fn_801982E4(lbl_805FB670, 0x4) || fn_801982E4(lbl_805FB670, 0x10000)) {
            left = 1;
        } else if (fn_801982E4(lbl_805FB670, 0x8) || fn_801982E4(lbl_805FB670, 0x20000)) {
            right = 1;
        }
    } else {
        if (fn_801982A8(lbl_805FB670, 0x1) || fn_801982A8(lbl_805FB670, 0x4000)) {
            up = 1;
        } else if (fn_801982A8(lbl_805FB670, 0x2) || fn_801982A8(lbl_805FB670, 0x8000)) {
            down = 1;
        }
        if (fn_801982A8(lbl_805FB670, 0x4) || fn_801982A8(lbl_805FB670, 0x10000)) {
            left = 1;
        } else if (fn_801982A8(lbl_805FB670, 0x8) || fn_801982A8(lbl_805FB670, 0x20000)) {
            right = 1;
        }
    }

    // Step the row index (+0xC) by one, repeating until a selectable cell is found.
    if (down) {
        u8* list = *(u8**)(grid + 0x4);
        s32 count = list ? fn_8019555C(list) : 0;
        u8* cell;
        do {
            s32 v = *(s32*)(grid + 0xC) + 1;
            *(s32*)(grid + 0xC) = v;
            if (v >= count) {
                if (*(u32*)(grid + 0x0) & 2) {
                    *(s32*)(grid + 0xC) = 0;
                } else {
                    *(s32*)(grid + 0xC) = count - 1;
                }
            }
            cell = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
        } while (cell == NULL || !(*(u32*)(cell + 0x64) & 1));
    } else if (up) {
        u8* list = *(u8**)(grid + 0x4);
        s32 count = list ? fn_8019555C(list) : 0;
        u8* cell;
        do {
            s32 v = *(s32*)(grid + 0xC) - 1;
            *(s32*)(grid + 0xC) = v;
            if (v < 0) {
                if (*(u32*)(grid + 0x0) & 2) {
                    *(s32*)(grid + 0xC) = count - 1;
                } else {
                    *(s32*)(grid + 0xC) = 0;
                }
            }
            cell = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
        } while (cell == NULL || !(*(u32*)(cell + 0x64) & 1));
    }

    // Step the column index (+0x10) within the current row.
    if (left) {
        u8* list = *(u8**)(grid + 0x4);
        s32 count = 0;
        if (list != NULL) {
            u8* row = *Slot(list, *(s32*)(grid + 0xC));
            if (row != NULL) {
                count = fn_8019555C(row);
            }
        }
        u8* cell;
        do {
            s32 v = *(s32*)(grid + 0x10) - 1;
            *(s32*)(grid + 0x10) = v;
            if (v < 0) {
                if (*(u32*)(grid + 0x0) & 4) {
                    *(s32*)(grid + 0x10) = count - 1;
                } else {
                    *(s32*)(grid + 0x10) = 0;
                }
            }
            cell = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
        } while (cell == NULL || !(*(u32*)(cell + 0x64) & 1));
    } else if (right) {
        u8* list = *(u8**)(grid + 0x4);
        s32 count = 0;
        if (list != NULL) {
            u8* row = *Slot(list, *(s32*)(grid + 0xC));
            if (row != NULL) {
                count = fn_8019555C(row);
            }
        }
        u8* cell;
        do {
            s32 v = *(s32*)(grid + 0x10) + 1;
            *(s32*)(grid + 0x10) = v;
            if (v >= count) {
                if (*(u32*)(grid + 0x0) & 4) {
                    *(s32*)(grid + 0x10) = 0;
                } else {
                    *(s32*)(grid + 0x10) = count - 1;
                }
            }
            cell = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
        } while (cell == NULL || !(*(u32*)(cell + 0x64) & 1));
    }

    // Selection changed: unselect the previous cell, select the new one.
    s32 prevX = *(s32*)(grid + 0x14);
    if (prevX != *(s32*)(grid + 0xC) || *(s32*)(grid + 0x18) != *(s32*)(grid + 0x10)) {
        if (prevX > -1) {
            s32 prevY = *(s32*)(grid + 0x18);
            if (prevY > -1) {
                u8* prev = GridCell(grid, prevX, prevY);
                if (prev != NULL) {
                    UIPlayAnim(prev, 0xF5A9CE6Cu);
                }
            }
        }
        u8* sel = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
        if (sel != NULL) {
            UIPlayAnim(sel, 0x705598E1u);
        }
        *(s32*)(grid + 0x14) = *(s32*)(grid + 0xC);
        *(s32*)(grid + 0x18) = *(s32*)(grid + 0x10);
    }

    // Direction animations on the current cell.
    u8* cur = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
    if (cur != NULL && *(s32*)(cur + 0x60) != 4) {
        if (left) {
            UIPlayAnim(cur, 0x3AB57627u);
        } else if (right) {
            UIPlayAnim(cur, 0xD1993CBFu);
        }
        if (up) {
            UIPlayAnim(cur, 0x82091142u);
        } else if (down) {
            UIPlayAnim(cur, 0x69255BDAu);
        }
    }

    // Tick the current cell.
    u8* tick = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
    if (tick != NULL) {
        (*(VFnF32*)(*(u8**)(tick + 0x138) + 0x28))(tick, dt);
    }

    // Only the owning screen reacts to the confirm button.
    u8* active = *(u8**)((u8*)gScreenSets + 0x74);
    if (active != NULL && *(u8**)(active + 0xBC) != grid) {
        return 0;
    }
    if (fn_801982A8(lbl_805FB670, 0x40)) {
        u8* confirm = GridCell(grid, *(s32*)(grid + 0xC), *(s32*)(grid + 0x10));
        if (confirm != NULL) {
            UIPlayAnim(confirm, 0x43D82AB2u);
        }
    }
    return 0;
}

// 0x801A9174
// Unprojects the widget's screen position through the active camera. Writes the ray origin
// (near-plane point plus the normalized near-to-far direction scaled by e+0x70) to pos and the
// widget's world rotation (euler angles from the camera basis, widget +0x17C) to out.
// With widget flag 0x20000 the screen point is mapped through the camera's four frustum corner
// rows (offset by the parent chain, flag 0x40000 uses the parent's matrix); otherwise the
// position comes from the screen-set basis vectors. Returns 0.
s32 fn_801A9174(u8* e, f32* pos, f32* out) {
    u8* gs = (u8*)gScreenSets;
    f32 rotYaw = *(f32*)(e + 0x90);     // f31
    f32 rotPitch = *(f32*)(e + 0x94);   // f30
    if (lbl_805FB680 == NULL) {
        return 0;
    }
    u32 flags = *(u32*)(e + 0x64);
    if (flags & 0x20000) {
        *(s32*)((u8*)lbl_805FB680 + 0x188) = 1;
        fn_8016E304(lbl_805FB680);
        u8* cam = (u8*)lbl_805FB680;   // r28
        f32 x = *(f32*)(e + 0x68);
        f32 y = *(f32*)(e + 0x6C);
        f32 z = *(f32*)(e + 0x70);
        if (flags & 0x40000) {
            u8* parent = *(u8**)(e + 0x128);
            if (parent != NULL) {
                x = x + *(f32*)(parent + 0x9C);
                y = y + *(f32*)(parent + 0xA0);
            }
            // Parent's 3x4 world matrix (virtual slot 0x34).
            f32* m = (*(VFnMat*)(*(u8**)(parent + 0x138) + 0x34))(parent);
            f32 nx = m[9]  + ((m[6] * z) + ((m[0] * x) + (m[3] * y)));
            f32 ny = m[10] + ((m[7] * z) + ((m[1] * x) + (m[4] * y)));
            x = nx;
            y = ny;
        } else {
            // Rotate the offset by each ancestor's angle and add its position.
            for (u8* node = *(u8**)(e + 0x128); node != NULL; node = *(u8**)(node + 0x128)) {
                f32 a = 1.5707964f * (0.63661975f * *(f32*)(node + 0x98));
                f32 c = (f32)cosf(a);
                f32 s = (f32)sinf(a);
                f32 nx = ((x * c) + (y * -s)) + *(f32*)(node + 0x68);
                f32 ny = ((y * c) + (x * s)) + *(f32*)(node + 0x6C);
                y = ny;
                x = nx;
            }
        }
        // Normalised screen position -> bilinear blend of the camera frustum corners.
        f32 u = x / (f32)gScreenWidth;
        f32 v = y / (f32)gScreenHeight;
        f32 omu = (f32)(1.0 - (f64)u);
        f32 omv = (f32)(1.0 - (f64)v);
        f32 omu2 = 1.0f - u;
        f32 omv2 = 1.0f - v;
        f32 ez = *(f32*)(e + 0x70);
        // Near plane edge points A and B, blended by v into Q1.
        f32 ax = (*(f32*)(cam + 0x12C) * omu) + (*(f32*)(cam + 0x120) * u);
        f32 ay = (*(f32*)(cam + 0x130) * omu) + (*(f32*)(cam + 0x124) * u);
        f32 az = (*(f32*)(cam + 0x134) * omu) + (*(f32*)(cam + 0x128) * u);
        f32 bx = (*(f32*)(cam + 0x138) * omu) + (*(f32*)(cam + 0x144) * u);
        f32 by = (*(f32*)(cam + 0x13C) * omu) + (*(f32*)(cam + 0x148) * u);
        f32 bz = (*(f32*)(cam + 0x140) * omu) + (*(f32*)(cam + 0x14C) * u);
        f32 q1x = (ax * omv) + (bx * v);
        f32 q1y = (ay * omv) + (by * v);
        f32 q1z = (az * omv) + (bz * v);
        // Far plane edge points C and D, blended by v into Q2.
        f32 cx = (*(f32*)(cam + 0x15C) * omu2) + (*(f32*)(cam + 0x150) * u);
        f32 cy = (*(f32*)(cam + 0x160) * omu2) + (*(f32*)(cam + 0x154) * u);
        f32 cz = (*(f32*)(cam + 0x164) * omu2) + (*(f32*)(cam + 0x158) * u);
        f32 dx = (*(f32*)(cam + 0x168) * omu2) + (*(f32*)(cam + 0x174) * u);
        f32 dy = (*(f32*)(cam + 0x16C) * omu2) + (*(f32*)(cam + 0x178) * u);
        f32 dz = (*(f32*)(cam + 0x170) * omu2) + (*(f32*)(cam + 0x17C) * u);
        f32 q2x = (cx * omv2) + (dx * v);
        f32 q2y = (cy * omv2) + (dy * v);
        f32 q2z = (cz * omv2) + (dz * v);
        // Direction Q1 -> Q2, normalized (original: frsqrte with two Newton steps); zero stays zero.
        f32 rx = q2x - q1x;
        f32 ry = q2y - q1y;
        f32 rz = q2z - q1z;
        f32 len2 = (rz * rz) + ((rx * rx) + (ry * ry));
        f32 nxd = 0.0f;
        f32 nyd = 0.0f;
        f32 nzd = 0.0f;
        if (len2 != 0.0f) {
            f32 inv = 1.0f / sqrtf(len2);
            nxd = rx * inv;
            nyd = ry * inv;
            nzd = rz * inv;
        }
        pos[0] = (nxd * ez) + q1x;
        pos[1] = (nyd * ez) + q1y;
        pos[2] = (nzd * ez) + q1z;
        *(s32*)((u8*)lbl_805FB680 + 0x188) = 0;
    } else {
        f32 ex = *(f32*)(e + 0x68);
        f32 ey = *(f32*)(e + 0x6C);
        f32 ez = *(f32*)(e + 0x70);
        // Screen-set basis: origin at +0x20, three direction vectors at +0x3C/+0x48/+0x54.
        f32 px = (*(f32*)(gs + 0x48) * ey) + (*(f32*)(gs + 0x3C) * ex) + (*(f32*)(gs + 0x54) * ez);
        f32 py = (*(f32*)(gs + 0x4C) * ey) + (*(f32*)(gs + 0x40) * ex) + (*(f32*)(gs + 0x58) * ez);
        f32 pz = (*(f32*)(gs + 0x50) * ey) + (*(f32*)(gs + 0x44) * ex) + (*(f32*)(gs + 0x5C) * ez);
        pos[0] = *(f32*)(gs + 0x20) + px;
        pos[1] = *(f32*)(gs + 0x24) + py;
        pos[2] = *(f32*)(gs + 0x28) + pz;
        // Screen-set orientation quaternion at +0x2C rotated by the widget's yaw/pitch.
        Quat q;
        q.x = *(f32*)(gs + 0x2C);
        q.y = *(f32*)(gs + 0x30);
        q.z = *(f32*)(gs + 0x34);
        q.w = *(f32*)(gs + 0x38);
        Vec3 axisA;
        Vec3 axisB;
        f32 tmp[3];
        fn_801455E0(tmp, (u8*)&q);
        axisA.x = tmp[0];
        axisA.y = tmp[1];
        axisA.z = tmp[2];
        Vec3 fwd;
        QuatGetForward(&fwd, &q);
        axisB.x = fwd.x;
        axisB.y = fwd.y;
        axisB.z = fwd.z;
        fn_80146180(&q, &axisA, 0.63661975f * rotPitch);
        fn_80146180(&q, &axisB, 0.63661975f * rotYaw);
    }
    // Camera basis (right/up/forward negated as in the original) -> euler angles at e+0x17C.
    u8* cam = (u8*)lbl_805FB680;
    f32 mat[9];
    mat[0] = -*(f32*)(cam + 0x18);
    mat[1] = -*(f32*)(cam + 0x1C);
    mat[2] = -*(f32*)(cam + 0x20);
    mat[3] = *(f32*)(cam + 0x0);
    mat[4] = *(f32*)(cam + 0x4);
    mat[5] = *(f32*)(cam + 0x8);
    mat[6] = *(f32*)(cam + 0xC);
    mat[7] = *(f32*)(cam + 0x10);
    mat[8] = *(f32*)(cam + 0x14);
    fn_801449A4(mat, (f32*)(e + 0x17C));
    out[0] = *(f32*)(e + 0x17C);
    out[1] = *(f32*)(e + 0x180);
    out[2] = *(f32*)(e + 0x184);
    out[0] = *(f32*)(e + 0x17C);
    return 0;
}

}  // extern "C"
