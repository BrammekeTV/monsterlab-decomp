// Matrix of an object's transform: position at +0x0 and orientation quaternion (x, y, z, w) at
// +0xC of the transform the object points to at +0x4.
// Original: 0x800DBD10 (auto_03_800DB9A4_text.s)
#include "types.h"

// 0x800DBD10
// Writes the three axes (columns) and the position as 12 floats. The quaternion is divided by
// its squared length (as in the original; a unit quaternion is unchanged); a zero quaternion
// gives the identity.
extern "C" void fn_800DBD10(u8* obj, f32* out) {
    const f32* t = *(const f32**)(obj + 4);
    f32 x = t[3], y = t[4], z = t[5], w = t[6];
    f32 len2 = w * w + (z * z + (x * x + y * y));
    if (len2 > -0.0001f && len2 < 0.0001f) {
        x = 0.0f; y = 0.0f; z = 0.0f; w = 1.0f;
    } else {
        f32 inv = 1.0f / len2;
        x = t[3] * inv;
        y = t[4] * inv;
        z = t[5] * inv;
        w = t[6] * inv;
    }
    f32 x2 = 2.0f * x, y2 = 2.0f * y, z2 = 2.0f * z;
    f32 xx = x2 * x, yy = y2 * y, zz = z2 * z;
    f32 xw = x2 * w, yw = y2 * w, zw = z2 * w;
    f32 zy = z2 * y, zx = z2 * x, yx = y2 * x;
    out[0] = 1.0f - (yy + zz);
    out[1] = yx + zw;
    out[2] = zx - yw;
    out[3] = yx - zw;
    out[4] = 1.0f - (xx + zz);
    out[5] = zy + xw;
    out[6] = zx + yw;
    out[7] = zy - xw;
    out[8] = 1.0f - (xx + yy);
    const f32* pos = *(const f32**)(obj + 4);
    out[9] = pos[0];
    out[10] = pos[1];
    out[11] = pos[2];
}
