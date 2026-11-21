// Scene node world-transform update: combines the node's local matrix with the parent
// and (optionally) the camera matrix, producing position, radius and a second position.
#include "types.h"
#include <math.h>

extern "C++" {
extern void* lbl_805FB5F8;  // 0x805FB5F8 (current camera)
extern f32   lbl_805FF648;  // 0x805FF648 (0.0f)
}

extern "C" {

// 0x80171ABC
// p+0x4 = local transform (3x3 rows at 0x0..0x20, translation at 0x24), p+0x10 = parent.
// Writes position at +0x14, scaled radius at +0x20 and a second position at +0x24.
void fn_80171ABC(u8* p) {
    u32 flags = *(u32*)(p + 0xC) | 8;
    u8* par = *(u8**)(p + 0x10);
    *(u32*)(p + 0xC) = flags;
    if (par == NULL) {
        f32 z = lbl_805FF648;
        *(f32*)(p + 0x14) = z;
        *(f32*)(p + 0x18) = z;
        *(f32*)(p + 0x1C) = z;
        *(f32*)(p + 0x20) = z;
        *(f32*)(p + 0x24) = z;
        *(f32*)(p + 0x28) = z;
        *(f32*)(p + 0x2C) = z;
        return;
    }
    u8* xf = *(u8**)(p + 0x4);
    if (xf == NULL) {
        *(f32*)(p + 0x14) = *(f32*)(par + 0x20);
        *(f32*)(p + 0x18) = *(f32*)(par + 0x24);
        *(f32*)(p + 0x1C) = *(f32*)(par + 0x28);
        *(f32*)(p + 0x20) = *(f32*)(par + 0x2C);
        *(f32*)(p + 0x24) = *(f32*)(par + 0x48);
        *(f32*)(p + 0x28) = *(f32*)(par + 0x4C);
        *(f32*)(p + 0x2C) = *(f32*)(par + 0x50);
        return;
    }

    f32 m[9];
    if (flags & 3) {
        f32 r0a = *(f32*)(xf + 0x0), r0b = *(f32*)(xf + 0x4), r0c = *(f32*)(xf + 0x8);
        f32 r1a = *(f32*)(xf + 0xC), r1b = *(f32*)(xf + 0x10), r1c = *(f32*)(xf + 0x14);
        f32 r2a = *(f32*)(xf + 0x18), r2b = *(f32*)(xf + 0x1C), r2c = *(f32*)(xf + 0x20);
        // Camera basis: flag bit 0 selects the set at +0x30, otherwise the set at +0x60.
        u8* cam = (u8*)lbl_805FB5F8;
        s32 o = (flags & 1) ? 0x30 : 0x60;
        f32 x0 = *(f32*)(cam + o + 0x00), y0 = *(f32*)(cam + o + 0x04), z0 = *(f32*)(cam + o + 0x08);
        f32 x1 = *(f32*)(cam + o + 0x0C), y1 = *(f32*)(cam + o + 0x10), z1 = *(f32*)(cam + o + 0x14);
        f32 x2 = *(f32*)(cam + o + 0x18), y2 = *(f32*)(cam + o + 0x1C), z2 = *(f32*)(cam + o + 0x20);
        m[0] = x2 * r0c + (x0 * r0a + x1 * r0b);
        m[1] = y2 * r0c + (y0 * r0a + y1 * r0b);
        m[2] = z2 * r0c + (z0 * r0a + z1 * r0b);
        m[3] = x2 * r1c + (x0 * r1a + x1 * r1b);
        m[4] = y2 * r1c + (y0 * r1a + y1 * r1b);
        m[5] = z2 * r1c + (z0 * r1a + z1 * r1b);
        m[6] = x2 * r2c + (x0 * r2a + x1 * r2b);
        m[7] = y2 * r2c + (y0 * r2a + y1 * r2b);
        m[8] = z2 * r2c + (z0 * r2a + z1 * r2b);

        f32 t0 = *(f32*)(xf + 0x24), t1 = *(f32*)(xf + 0x28), t2 = *(f32*)(xf + 0x2C);
        f32 a = *(f32*)(par + 0x20), b = *(f32*)(par + 0x24), c = *(f32*)(par + 0x28);
        f32 px = t0 + (m[6] * c + (m[0] * a + m[3] * b));
        f32 py = t1 + (m[7] * c + (m[1] * a + m[4] * b));
        f32 pz = t2 + (m[8] * c + (m[2] * a + m[5] * b));

        f32 n = (m[2] * m[2]) + ((m[0] * m[0]) + (m[1] * m[1]));
        f32 n1 = (m[5] * m[5]) + ((m[3] * m[3]) + (m[4] * m[4]));
        if (!(n > n1)) n = n1;
        f32 n2 = (m[8] * m[8]) + ((m[6] * m[6]) + (m[7] * m[7]));
        if (!(n > n2)) n = n2;
        f32 root = (n <= lbl_805FF648) ? lbl_805FF648 : sqrtf(n);

        *(f32*)(p + 0x14) = px;
        f32 rad = *(f32*)(par + 0x2C) * *(f32*)(p + 0x8) * root;
        *(f32*)(p + 0x18) = py;
        *(f32*)(p + 0x1C) = pz;
        *(f32*)(p + 0x20) = rad;

        f32 a2 = *(f32*)(par + 0x48), b2 = *(f32*)(par + 0x4C), c2 = *(f32*)(par + 0x50);
        *(f32*)(p + 0x24) = t0 + (m[6] * c2 + (m[0] * a2 + m[3] * b2));
        *(f32*)(p + 0x28) = t1 + (m[7] * c2 + (m[1] * a2 + m[4] * b2));
        *(f32*)(p + 0x2C) = t2 + (m[8] * c2 + (m[2] * a2 + m[5] * b2));
        return;
    }

    // No camera flags: transform the parent vectors by the local matrix directly.
    f32 a = *(f32*)(par + 0x20), b = *(f32*)(par + 0x24), c = *(f32*)(par + 0x28);
    f32 px = *(f32*)(xf + 0x24) + (*(f32*)(xf + 0x18) * c + (*(f32*)(xf + 0x0) * a + *(f32*)(xf + 0xC) * b));
    f32 py = *(f32*)(xf + 0x28) + (*(f32*)(xf + 0x1C) * c + (*(f32*)(xf + 0x4) * a + *(f32*)(xf + 0x10) * b));
    f32 pz = *(f32*)(xf + 0x2C) + (*(f32*)(xf + 0x20) * c + (*(f32*)(xf + 0x8) * a + *(f32*)(xf + 0x14) * b));

    f32 r0 = *(f32*)(xf + 0x0), r1 = *(f32*)(xf + 0x4), r2 = *(f32*)(xf + 0x8);
    f32 r3 = *(f32*)(xf + 0xC), r4 = *(f32*)(xf + 0x10), r5 = *(f32*)(xf + 0x14);
    f32 n = (r2 * r2) + ((r0 * r0) + (r1 * r1));
    f32 n1 = (r5 * r5) + ((r3 * r3) + (r4 * r4));
    if (!(n > n1)) n = n1;
    f32 s0 = *(f32*)(xf + 0x18), s1 = *(f32*)(xf + 0x1C), s2 = *(f32*)(xf + 0x20);
    f32 n2 = (s2 * s2) + ((s0 * s0) + (s1 * s1));
    if (!(n > n2)) n = n2;
    f32 root = (n <= lbl_805FF648) ? lbl_805FF648 : sqrtf(n);

    *(f32*)(p + 0x14) = px;
    f32 rad = *(f32*)(par + 0x2C) * *(f32*)(p + 0x8) * root;
    *(f32*)(p + 0x18) = py;
    *(f32*)(p + 0x1C) = pz;
    *(f32*)(p + 0x20) = rad;

    f32 a2 = *(f32*)(par + 0x48), b2 = *(f32*)(par + 0x4C), c2 = *(f32*)(par + 0x50);
    *(f32*)(p + 0x24) = *(f32*)(xf + 0x24) + (*(f32*)(xf + 0x18) * c2 + (*(f32*)(xf + 0x0) * a2 + *(f32*)(xf + 0xC) * b2));
    *(f32*)(p + 0x28) = *(f32*)(xf + 0x28) + (*(f32*)(xf + 0x1C) * c2 + (*(f32*)(xf + 0x4) * a2 + *(f32*)(xf + 0x10) * b2));
    *(f32*)(p + 0x2C) = *(f32*)(xf + 0x2C) + (*(f32*)(xf + 0x20) * c2 + (*(f32*)(xf + 0x8) * a2 + *(f32*)(xf + 0x14) * b2));
}

}  // extern "C"
