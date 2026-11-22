// Draft fixes (miss665): UI element update, collision shape overlap tests, hit-plane sweeps, error text lookup.
#include <math.h>

#include "types.h"

extern "C" {
s32   fn_8019B6CC(u8* obj, f32 dt);                      // 0x8019B6CC (UI element base update)
u8*   fn_80196B24(u8* obj);                              // 0x80196B24 (top of the transform stack)
u8*   fn_80196C9C(void* p);                              // 0x80196C9C (current tint record)

extern void* lbl_805FB658;  // 0x805FB658 (global render object)
extern f32 lbl_805FFAB8;    // 0x805FFAB8 (0.0)
extern f32 lbl_805FFABC;    // 0x805FFABC (1.0)
extern f32 lbl_805FFAC4;    // 0x805FFAC4 (-1.0)
extern f32 lbl_805FFAC8;    // 0x805FFAC8 (+epsilon)
extern f32 lbl_806001DC;    // 0x806001DC (0.0)
extern f32 lbl_80600258;    // 0x80600258 (0.0)
extern u8 lbl_80386ED0[];   // 0x80386ED0 (error text block)
extern const char lbl_805F9920[];  // 0x805F9920 ("?" fallback text)
extern const char lbl_805F9928[];  // 0x805F9928
extern const char lbl_805F9930[];  // 0x805F9930
}

extern "C" {

// 0x801A2764
// UI element update: multiplies the 4-float colour at +0x80 by the current tint into +0x178 (copies it
// when there is no tint), then concatenates the 3x4 matrix at +0xF0 with the top of the transform stack
// into +0x148 and pushes both results to the child record behind +0x140.
s32 fn_801A2764(u8* obj, f32 dt) {
    typedef void (*Fn)(u8*, s32);
    fn_8019B6CC(obj, dt);
    u8* tint = fn_80196C9C(lbl_805FB658);
    if (tint != NULL) {
        *(f32*)(obj + 0x178) = *(f32*)(obj + 0x80) * *(f32*)(tint + 0x0);
        *(f32*)(obj + 0x17c) = *(f32*)(obj + 0x84) * *(f32*)(tint + 0x4);
        *(f32*)(obj + 0x180) = *(f32*)(obj + 0x88) * *(f32*)(tint + 0x8);
        *(f32*)(obj + 0x184) = *(f32*)(obj + 0x8c) * *(f32*)(tint + 0xc);
    } else {
        *(f32*)(obj + 0x178) = *(f32*)(obj + 0x80);
        *(f32*)(obj + 0x17c) = *(f32*)(obj + 0x84);
        *(f32*)(obj + 0x180) = *(f32*)(obj + 0x88);
        *(f32*)(obj + 0x184) = *(f32*)(obj + 0x8c);
    }
    (*(Fn*)(*(u8**)(obj + 0x138) + 0x30))(obj, 0x20);
    u8** child = *(u8***)(obj + 0x140);
    f32 c0 = *(f32*)(obj + 0x178);
    f32 c1 = *(f32*)(obj + 0x17c);
    f32 c2 = *(f32*)(obj + 0x180);
    f32 c3 = *(f32*)(obj + 0x184);
    if (child != NULL) {
        u8* rec = *child;
        if (rec != NULL) {
            *(f32*)(rec + 0x3c) = c0;
            *(f32*)(rec + 0x40) = c1;
            *(f32*)(rec + 0x44) = c2;
            *(f32*)(rec + 0x48) = c3;
        }
    }
    u8* a = fn_80196B24((u8*)lbl_805FB658);
    f32 m[12];
    if (a != NULL) {
        f32 A0 = *(f32*)(a + 0x0),  A1 = *(f32*)(a + 0x4),  A2 = *(f32*)(a + 0x8),  A3 = *(f32*)(a + 0xc);
        f32 A4 = *(f32*)(a + 0x10), A5 = *(f32*)(a + 0x14), A6 = *(f32*)(a + 0x18), A7 = *(f32*)(a + 0x1c);
        f32 A8 = *(f32*)(a + 0x20), A9 = *(f32*)(a + 0x24), A10 = *(f32*)(a + 0x28), A11 = *(f32*)(a + 0x2c);
        f32 B0 = *(f32*)(obj + 0xf0),  B1 = *(f32*)(obj + 0xf4),  B2 = *(f32*)(obj + 0xf8),  B3 = *(f32*)(obj + 0xfc);
        f32 B4 = *(f32*)(obj + 0x100), B5 = *(f32*)(obj + 0x104), B6 = *(f32*)(obj + 0x108), B7 = *(f32*)(obj + 0x10c);
        f32 B8 = *(f32*)(obj + 0x110), B9 = *(f32*)(obj + 0x114), B10 = *(f32*)(obj + 0x118), B11 = *(f32*)(obj + 0x11c);
        m[0]  = A6 * B2 + (A0 * B0 + A3 * B1);
        m[1]  = A7 * B2 + (A1 * B0 + A4 * B1);
        m[2]  = A8 * B2 + (A2 * B0 + A5 * B1);
        m[3]  = A6 * B5 + (A0 * B3 + A3 * B4);
        m[4]  = A7 * B5 + (A1 * B3 + A4 * B4);
        m[5]  = A8 * B5 + (A2 * B3 + A5 * B4);
        m[6]  = A6 * B8 + (A0 * B6 + A3 * B7);
        m[7]  = A7 * B8 + (A1 * B6 + A4 * B7);
        m[8]  = A8 * B8 + (A2 * B6 + A5 * B7);
        m[9]  = A9 + (A6 * B11 + (A0 * B9 + A3 * B10));
        m[10] = A10 + (A7 * B11 + (A1 * B9 + A4 * B10));
        m[11] = A11 + (A8 * B11 + (A2 * B9 + A5 * B10));
    } else {
        for (s32 i = 0; i < 12; i++) m[i] = *(f32*)(obj + 0xf0 + i * 4);
    }
    for (s32 i = 0; i < 12; i++) *(f32*)(obj + 0x148 + i * 4) = m[i];
    *(f32*)(obj + 0x5c) = m[11];
    (*(Fn*)(*(u8**)(obj + 0x138) + 0x30))(obj, 0x10);
    child = *(u8***)(obj + 0x140);
    f32 s[12];
    for (s32 i = 0; i < 12; i++) s[i] = *(f32*)(obj + 0x148 + i * 4);
    if (child != NULL) {
        u8* rec = *child;
        if (rec != NULL) {
            u8* dst = *(u8**)(rec + 0x4);
            for (s32 i = 0; i < 12; i++) *(f32*)(dst + i * 4) = s[i];
        }
    }
    return 0;
}

// 0x801AD6AC
// Box-vs-box penetration test: offset b-a is projected on the axes in rot (three rows of 3 floats at
// +0, +0xC, +0x18) and compared with the half-extents in ext plus margin. On overlap writes the
// hit point (+0), contact normal (+0xC) and depth (+0x18) to out and returns 1.
s32 fn_801AD6AC(u8* a, u8* ext, u8* rot, u8* b, u8* out, s32 flag, f32 margin) {
    f32 dx = *(f32*)(b + 0x0) - *(f32*)(a + 0x0);
    f32 dy = *(f32*)(b + 0x4) - *(f32*)(a + 0x4);
    f32 dz = *(f32*)(b + 0x8) - *(f32*)(a + 0x8);
    f32 pen[3];
    f32 proj[3];
    proj[0] = *(f32*)(rot + 0x8) * dz + (*(f32*)(rot + 0x0) * dx + *(f32*)(rot + 0x4) * dy);
    proj[1] = *(f32*)(rot + 0x14) * dz + (*(f32*)(rot + 0xc) * dx + *(f32*)(rot + 0x10) * dy);
    proj[2] = *(f32*)(rot + 0x20) * dz + (*(f32*)(rot + 0x18) * dx + *(f32*)(rot + 0x1c) * dy);
    pen[0] = (margin + *(f32*)(ext + 0x0)) - fabsf(proj[0]);
    if (pen[0] < lbl_805FFAC8) return 0;
    pen[1] = (margin + *(f32*)(ext + 0x4)) - fabsf(proj[1]);
    if (pen[1] < lbl_805FFAC8) return 0;
    pen[2] = (margin + *(f32*)(ext + 0x8)) - fabsf(proj[2]);
    if (pen[2] < lbl_805FFAC8) return 0;
    if (pen[2] > lbl_805FFAB8) flag = 0;
    // Pick the axis of least penetration; it becomes the unit axis vector in pen[].
    f32 depth;
    if (fabsf(pen[0]) < fabsf(pen[1])) {
        if (flag != 0 || fabsf(pen[0]) < fabsf(pen[2])) {
            depth = fabsf(pen[0]);
            pen[1] = lbl_805FFAB8;
            pen[2] = lbl_805FFAB8;
            pen[0] = (proj[0] < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
        } else {
            depth = fabsf(pen[2]);
            pen[0] = lbl_805FFAB8;
            pen[1] = lbl_805FFAB8;
            pen[2] = (proj[2] < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
        }
    } else if (flag != 0 || fabsf(pen[1]) < fabsf(pen[2])) {
        depth = fabsf(pen[1]);
        pen[0] = lbl_805FFAB8;
        pen[2] = lbl_805FFAB8;
        pen[1] = (proj[1] < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
    } else {
        depth = fabsf(pen[2]);
        pen[0] = lbl_805FFAB8;
        pen[1] = lbl_805FFAB8;
        pen[2] = (proj[2] < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
    }
    // World-space normal = rot^T * axis, then normalised (original: frsqrte with two Newton steps).
    f32 nx = *(f32*)(rot + 0x18) * pen[2] + (*(f32*)(rot + 0x0) * pen[0] + *(f32*)(rot + 0xc) * pen[1]);
    f32 ny = *(f32*)(rot + 0x1c) * pen[2] + (*(f32*)(rot + 0x4) * pen[0] + *(f32*)(rot + 0x10) * pen[1]);
    f32 nz = *(f32*)(rot + 0x20) * pen[2] + (*(f32*)(rot + 0x8) * pen[0] + *(f32*)(rot + 0x14) * pen[1]);
    f32 len2 = nz * nz + (nx * nx + ny * ny);
    f32 ux, uy, uz;
    if (lbl_805FFAB8 == len2) {
        ux = uy = uz = lbl_805FFAB8;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        ux = nx * inv;
        uy = ny * inv;
        uz = nz * inv;
    }
    *(f32*)(out + 0xc) = ux;
    *(f32*)(out + 0x10) = uy;
    *(f32*)(out + 0x14) = uz;
    *(f32*)(out + 0x18) = depth;
    *(f32*)(out + 0x0) = *(f32*)(b + 0x0) + ux * depth;
    *(f32*)(out + 0x4) = *(f32*)(b + 0x4) + uy * depth;
    *(f32*)(out + 0x8) = *(f32*)(b + 0x8) + uz * depth;
    return 1;
}

// 0x801ADF6C
// Collision-shape overlap test of shape a against shape b (both with centre at +0x14 and radius
// at +0x3C). When a is a capsule (segment +0x14..+0x20, longer than its radius) and b is a sphere
// (type 0 at +0xC), the closest point on the segment decides. Otherwise dispatches on b's type:
// 0 sphere, 3 plane, 4 axis-aligned box, 7 oriented box.
s32 fn_801ADF6C(u8* a, u8* b) {
    f32 dx = *(f32*)(a + 0x14) - *(f32*)(a + 0x20);
    f32 dy = *(f32*)(a + 0x18) - *(f32*)(a + 0x24);
    f32 dz = *(f32*)(a + 0x1c) - *(f32*)(a + 0x28);
    f32 ra = *(f32*)(a + 0x3c);
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    if (len2 > ra * ra && *(s32*)(b + 0xc) == 0) {
        f32 wx = *(f32*)(b + 0x14) - *(f32*)(a + 0x20);
        f32 wy = *(f32*)(b + 0x18) - *(f32*)(a + 0x24);
        f32 wz = *(f32*)(b + 0x1c) - *(f32*)(a + 0x28);
        f32 t = (wz * dz + (wx * dx + wy * dy)) / len2;
        f32 px = *(f32*)(a + 0x20) + dx * t;
        f32 py = *(f32*)(a + 0x24) + dy * t;
        f32 pz = *(f32*)(a + 0x28) + dz * t;
        if (t < lbl_805FFAB8) {
            px = *(f32*)(a + 0x20);
            py = *(f32*)(a + 0x24);
            pz = *(f32*)(a + 0x28);
        } else if (t > lbl_805FFABC) {
            px = *(f32*)(a + 0x14);
            py = *(f32*)(a + 0x18);
            pz = *(f32*)(a + 0x1c);
        }
        f32 qx = *(f32*)(b + 0x14) - px;
        f32 qy = *(f32*)(b + 0x18) - py;
        f32 qz = *(f32*)(b + 0x1c) - pz;
        f32 rs = *(f32*)(b + 0x3c) + ra;
        if (qz * qz + (qx * qx + qy * qy) - rs * rs <= lbl_805FFAB8) return 1;
    }
    switch (*(s32*)(b + 0xc)) {
    case 4: {
        f32 ex = *(f32*)(a + 0x14) - *(f32*)(b + 0x14);
        f32 ey = *(f32*)(a + 0x18) - *(f32*)(b + 0x18);
        f32 ez = *(f32*)(a + 0x1c) - *(f32*)(b + 0x1c);
        f32 r = *(f32*)(a + 0x3c);
        if ((r + *(f32*)(b + 0x3c)) - fabsf(ex) < lbl_805FFAB8) return 0;
        if ((r + *(f32*)(b + 0x40)) - fabsf(ey) < lbl_805FFAB8) return 0;
        return !((r + *(f32*)(b + 0x44)) - fabsf(ez) < lbl_805FFAB8);
    }
    case 7: {
        f32 ex = *(f32*)(a + 0x14) - *(f32*)(b + 0x14);
        f32 ey = *(f32*)(a + 0x18) - *(f32*)(b + 0x18);
        f32 ez = *(f32*)(a + 0x1c) - *(f32*)(b + 0x1c);
        f32 r = *(f32*)(a + 0x3c);
        f32 p0 = *(f32*)(b + 0x44) * ez + (*(f32*)(b + 0x3c) * ex + *(f32*)(b + 0x40) * ey);
        f32 p1 = *(f32*)(b + 0x50) * ez + (*(f32*)(b + 0x48) * ex + *(f32*)(b + 0x4c) * ey);
        f32 p2 = *(f32*)(b + 0x5c) * ez + (*(f32*)(b + 0x54) * ex + *(f32*)(b + 0x58) * ey);
        if ((r + *(f32*)(b + 0x6c)) - fabsf(p0) < lbl_805FFAB8) return 0;
        if ((r + *(f32*)(b + 0x70)) - fabsf(p1) < lbl_805FFAB8) return 0;
        return !((r + *(f32*)(b + 0x74)) - fabsf(p2) < lbl_805FFAB8);
    }
    case 0: {
        f32 ex = *(f32*)(a + 0x14) - *(f32*)(b + 0x14);
        f32 ey = *(f32*)(a + 0x18) - *(f32*)(b + 0x18);
        f32 ez = *(f32*)(a + 0x1c) - *(f32*)(b + 0x1c);
        f32 rs = *(f32*)(b + 0x3c) + *(f32*)(a + 0x3c);
        return ez * ez + (ex * ex + ey * ey) - rs * rs <= lbl_805FFAB8;
    }
    case 3: {
        f32 ex = *(f32*)(a + 0x14) - *(f32*)(b + 0x14);
        f32 ey = *(f32*)(a + 0x18) - *(f32*)(b + 0x18);
        f32 ez = *(f32*)(a + 0x1c) - *(f32*)(b + 0x1c);
        f32 ra2 = *(f32*)(a + 0x3c);
        f32 rs = ra2 + *(f32*)(b + 0x3c);
        if (ez * ez + (ex * ex + ey * ey) - rs * rs > lbl_805FFAB8) return 0;
        f32 dist = *(f32*)(b + 0x48) * ez + (ex * *(f32*)(b + 0x40) + ey * *(f32*)(b + 0x44));
        return dist <= ra2;
    }
    default:
        return 0;
    }
}

// 0x801F8764
// Walks the hit-plane ring buffer (count +0xC, head +0x10, 0x84-byte entries at +4, ids at +8) from
// index while the entry position (plus start) stays on the near side of the plane, then keeps
// walking while it is past threshold f1, summing |distance| into *outT. Returns the number of
// entries stepped, or -1 if none was found or the entry id equals *outU (otherwise stored there).
s32 fn_801F8764(u8* list, s32 mode, f32* normal, s32 index, f32* outT, u32* outU, f32* start, f32 f1) {
    f32 px, py, pz;
    s32 n;
    *outT = lbl_806001DC;
    n = *(s32*)(list + 0xc);
    while (index < n) {
        u8* e = *(u8**)(list + 0x4) + ((*(s32*)(list + 0x10) + index) % n) * 0x84;
        if (mode == 0) {
            px = *(f32*)(e + 0xc);
            py = *(f32*)(e + 0x10);
            pz = *(f32*)(e + 0x14);
        } else {
            px = *(f32*)(e + 0x68);
            py = *(f32*)(e + 0x6c);
            pz = *(f32*)(e + 0x70);
        }
        px += start[0];
        py += start[1];
        pz += start[2];
        if (normal[2] * pz + (normal[0] * px + normal[1] * py) > f1) break;
        index++;
    }
    n = *(s32*)(list + 0xc);
    if (index == n) return -1;
    u32 id = ((u32*)*(u8**)(list + 0x8))[(*(s32*)(list + 0x10) + index) % n];
    if (*outU == id) return -1;
    *outU = id;
    s32 first = index;
    while (index < *(s32*)(list + 0xc)) {
        s32 cnt = *(s32*)(list + 0xc);
        u8* e = *(u8**)(list + 0x4) + ((*(s32*)(list + 0x10) + index) % cnt) * 0x84;
        if (mode == 0) {
            px = *(f32*)(e + 0xc);
            py = *(f32*)(e + 0x10);
            pz = *(f32*)(e + 0x14);
        } else {
            px = *(f32*)(e + 0x68);
            py = *(f32*)(e + 0x6c);
            pz = *(f32*)(e + 0x70);
        }
        px += start[0];
        py += start[1];
        pz += start[2];
        f32 d = normal[2] * pz + (normal[0] * px + normal[1] * py);
        if (d < f1) break;
        index++;
        *outT += fabsf(d);
    }
    n = *(s32*)(list + 0xc);
    if (index >= n) index = n - 1;
    return index - first;
}

// 0x801FA0A8
// Same ring-buffer walk as fn_801F8764 but on |distance| to the plane and without the id check:
// skips entries closer than f1, then counts entries at least f1 away (summing |distance| into
// *outT). Returns that count, or -1 if the whole ring was skipped.
s32 fn_801FA0A8(u8* list, s32 mode, f32* plane, s32 index, f32* outT, f32* start, f32 f1) {
    f32 px, py, pz;
    s32 n;
    *outT = lbl_80600258;
    n = *(s32*)(list + 0xc);
    while (index < n) {
        u8* e = *(u8**)(list + 0x4) + ((*(s32*)(list + 0x10) + index) % n) * 0x84;
        if (mode == 0) {
            px = *(f32*)(e + 0xc);
            py = *(f32*)(e + 0x10);
            pz = *(f32*)(e + 0x14);
        } else {
            px = *(f32*)(e + 0x68);
            py = *(f32*)(e + 0x6c);
            pz = *(f32*)(e + 0x70);
        }
        px += start[0];
        py += start[1];
        pz += start[2];
        if (fabsf(plane[2] * pz + (plane[0] * px + plane[1] * py)) > f1) break;
        index++;
    }
    if (index == *(s32*)(list + 0xc)) return -1;
    s32 first = index;
    while (index < *(s32*)(list + 0xc)) {
        s32 cnt = *(s32*)(list + 0xc);
        u8* e = *(u8**)(list + 0x4) + ((*(s32*)(list + 0x10) + index) % cnt) * 0x84;
        if (mode == 0) {
            px = *(f32*)(e + 0xc);
            py = *(f32*)(e + 0x10);
            pz = *(f32*)(e + 0x14);
        } else {
            px = *(f32*)(e + 0x68);
            py = *(f32*)(e + 0x6c);
            pz = *(f32*)(e + 0x70);
        }
        px += start[0];
        py += start[1];
        pz += start[2];
        f32 d = plane[2] * pz + (plane[0] * px + plane[1] * py);
        if (fabsf(d) < f1) break;
        index++;
        *outT += fabsf(d);
    }
    return index - first;
}

// 0x8020716C
// Maps an error code (0 and negative values) to its message text; unknown codes give "?".
const char* fn_8020716C(s32 code) {
    switch (code) {
    case (-0x7FFFFFFF - 1): return (const char*)(lbl_80386ED0 + 0xc);
    case -0x79: return (const char*)(lbl_80386ED0 + 0x498);
    case -0x70: return (const char*)(lbl_80386ED0 + 0x484);
    case -0x6F: return (const char*)(lbl_80386ED0 + 0x46c);
    case -0x6D: return (const char*)(lbl_80386ED0 + 0x460);
    case -0x6C: return (const char*)(lbl_80386ED0 + 0x454);
    case -0x6B: return (const char*)(lbl_80386ED0 + 0x448);
    case -0x66: return (const char*)(lbl_80386ED0 + 0x438);
    case -0x65: return (const char*)(lbl_80386ED0 + 0x420);
    case -0x64: return (const char*)(lbl_80386ED0 + 0x40c);
    case -0x4E: return (const char*)(lbl_80386ED0 + 0x400);
    case -0x4D: return (const char*)(lbl_80386ED0 + 0x3f4);
    case -0x4C: return (const char*)(lbl_80386ED0 + 0x3e4);
    case -0x4B: return (const char*)(lbl_80386ED0 + 0x3d8);
    case -0x4A: return (const char*)(lbl_80386ED0 + 0x3cc);
    case -0x49: return (const char*)(lbl_80386ED0 + 0x3c0);
    case -0x48: return (const char*)(lbl_80386ED0 + 0x3b4);
    case -0x47: return (const char*)(lbl_80386ED0 + 0x3a8);
    case -0x46: return (const char*)(lbl_80386ED0 + 0x39c);
    case -0x45: return (const char*)(lbl_80386ED0 + 0x38c);
    case -0x44: return (const char*)(lbl_80386ED0 + 0x378);
    case -0x43: return (const char*)(lbl_80386ED0 + 0x36c);
    case -0x42: return (const char*)(lbl_80386ED0 + 0x360);
    case -0x41: return (const char*)(lbl_80386ED0 + 0x354);
    case -0x40: return (const char*)(lbl_80386ED0 + 0x344);
    case -0x3F: return (const char*)(lbl_80386ED0 + 0x334);
    case -0x3E: return (const char*)(lbl_80386ED0 + 0x328);
    case -0x3D: return (const char*)(lbl_80386ED0 + 0x31c);
    case -0x3C: return (const char*)(lbl_80386ED0 + 0x310);
    case -0x3B: return (const char*)(lbl_80386ED0 + 0x304);
    case -0x3A: return (const char*)(lbl_80386ED0 + 0x2f4);
    case -0x39: return (const char*)(lbl_80386ED0 + 0x2e8);
    case -0x38: return (const char*)(lbl_80386ED0 + 0x2dc);
    case -0x37: return (const char*)(lbl_80386ED0 + 0x2d0);
    case -0x36: return (const char*)(lbl_80386ED0 + 0x2c4);
    case -0x35: return (const char*)(lbl_80386ED0 + 0x2b8);
    case -0x34: return (const char*)(lbl_80386ED0 + 0x2ac);
    case -0x33: return (const char*)(lbl_80386ED0 + 0x29c);
    case -0x32: return (const char*)(lbl_80386ED0 + 0x290);
    case -0x31: return (const char*)(lbl_80386ED0 + 0x284);
    case -0x30: return (const char*)(lbl_80386ED0 + 0x278);
    case -0x2F: return (const char*)(lbl_80386ED0 + 0x26c);
    case -0x2E: return (const char*)(lbl_80386ED0 + 0x260);
    case -0x2D: return (const char*)(lbl_80386ED0 + 0x254);
    case -0x2C: return (const char*)(lbl_80386ED0 + 0x248);
    case -0x2B: return (const char*)(lbl_80386ED0 + 0x23c);
    case -0x2A: return (const char*)(lbl_80386ED0 + 0x230);
    case -0x29: return (const char*)(lbl_80386ED0 + 0x224);
    case -0x28: return (const char*)(lbl_80386ED0 + 0x214);
    case -0x27: return (const char*)(lbl_80386ED0 + 0x204);
    case -0x26: return (const char*)(lbl_80386ED0 + 0x1f8);
    case -0x25: return (const char*)(lbl_80386ED0 + 0x1e8);
    case -0x24: return (const char*)(lbl_80386ED0 + 0x1d8);
    case -0x23: return (const char*)(lbl_80386ED0 + 0x1cc);
    case -0x22: return (const char*)(lbl_80386ED0 + 0x1c0);
    case -0x21: return (const char*)(lbl_80386ED0 + 0x1b4);
    case -0x20: return (const char*)(lbl_80386ED0 + 0x1a8);
    case -0x1F: return (const char*)(lbl_80386ED0 + 0x19c);
    case -0x1E: return (const char*)(lbl_80386ED0 + 0x190);
    case -0x1D: return lbl_805F9930;
    case -0x1C: return (const char*)(lbl_80386ED0 + 0x184);
    case -0x1B: return (const char*)(lbl_80386ED0 + 0x178);
    case -0x1A: return (const char*)(lbl_80386ED0 + 0x168);
    case -0x19: return (const char*)(lbl_80386ED0 + 0x15c);
    case -0x18: return (const char*)(lbl_80386ED0 + 0x150);
    case -0x17: return (const char*)(lbl_80386ED0 + 0x140);
    case -0x16: return (const char*)(lbl_80386ED0 + 0x134);
    case -0x15: return (const char*)(lbl_80386ED0 + 0x128);
    case -0x14: return (const char*)(lbl_80386ED0 + 0x11c);
    case -0x13: return (const char*)(lbl_80386ED0 + 0x110);
    case -0x12: return lbl_805F9928;
    case -0x11: return (const char*)(lbl_80386ED0 + 0x100);
    case -0x10: return (const char*)(lbl_80386ED0 + 0xf0);
    case -0xF: return (const char*)(lbl_80386ED0 + 0xe0);
    case -0xE: return (const char*)(lbl_80386ED0 + 0xd0);
    case -0xD: return (const char*)(lbl_80386ED0 + 0xc0);
    case -0xC: return (const char*)(lbl_80386ED0 + 0xb4);
    case -0xB: return (const char*)(lbl_80386ED0 + 0xa4);
    case -0xA: return (const char*)(lbl_80386ED0 + 0x98);
    case -0x9: return (const char*)(lbl_80386ED0 + 0x8c);
    case -0x8: return (const char*)(lbl_80386ED0 + 0x80);
    case -0x7: return (const char*)(lbl_80386ED0 + 0x74);
    case -0x6: return (const char*)(lbl_80386ED0 + 0x68);
    case -0x5: return (const char*)(lbl_80386ED0 + 0x58);
    case -0x4: return (const char*)(lbl_80386ED0 + 0x40);
    case -0x3: return (const char*)(lbl_80386ED0 + 0x30);
    case -0x2: return (const char*)(lbl_80386ED0 + 0x24);
    case -0x1: return (const char*)(lbl_80386ED0 + 0x18);
    case 0: return (const char*)(lbl_80386ED0 + 0x0);
    default: return lbl_805F9920;
    }
}

}  // extern "C"
