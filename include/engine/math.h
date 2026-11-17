#pragma once
#include <float.h>
#include <math.h>

#include "types.h"

struct Vec3 {
    f32 x, y, z;
};

struct Quat {
    f32 x, y, z, w;
};

// Position + orientation, as stored in scene objects.
struct Transform {
    Vec3 pos;   // 0x00
    Quat rot;   // 0x0C
};

static inline f32 Vec3LengthSq(const Vec3& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

// Normalize with the engine's rsqrt (two Newton steps on frsqrte); zero stays zero.
static inline Vec3 Vec3Normalized(const Vec3& v) {
    f32 len2 = Vec3LengthSq(v);
    if (len2 == 0.0f) return Vec3{ 0.0f, 0.0f, 0.0f };
    f32 inv = 1.0f / sqrtf(len2);
    return Vec3{ v.x * inv, v.y * inv, v.z * inv };
}

extern "C" {
// 0x801452A8: Y axis (second matrix column) of the rotation.
Vec3 QuatGetAxisY(const Quat* q);
}
