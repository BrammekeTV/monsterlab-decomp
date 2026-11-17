// Projectile lob math.
// Original object: .text 0x80019FB0 - 0x8001A15C (auto_03_80019FB0_text.s)
#include "engine/math.h"

extern "C" {
f32 cosf(f32 x);   // 0x8033EC98 (MSL)
f32 sinf(f32 x);   // 0x8033F0A4 (MSL)
}

// 0x80019FB0
// Launch speed for a projectile to fly from `from` to `to` when fired at `angle` (fraction of
// 90 degrees above the horizontal) under `gravity` (negative = down). Returns 0 if the target
// can't be reached at that angle.
//   v^2 = -g * d^2 / (2 cos^2(a) * (dz - d tan(a)))
f32 ComputeLobSpeed(const Vec3* from, const Vec3* to, f32 angle, f32 gravity) {
    Vec3 flat = { to->x - from->x, to->y - from->y, 0.0f };
    f32 len2 = Vec3LengthSq(flat);
    f32 d = 0.0f;
    if (len2 > 0.0f) d = sqrtf(len2);

    f32 dz = to->z - from->z;
    f32 s = sinf(1.5707964f * angle);
    f32 c = cosf(1.5707964f * angle);

    f32 v2 = (d * (-gravity * d) / (2.0f * c * c)) / (dz - s * d / c);
    if (v2 < 0.0f) return 0.0f;
    if (v2 <= 0.0f) return 0.0f;
    return sqrtf(v2);
}
