// Small out-of-line vector/scalar helpers used by the character movement code (the original
// C++ math classes, not inlined by the compiler). Vectors are 3 floats; "scalars" are a single
// float wrapped in a struct.
// Original: 0x80052640 - 0x80058240 (auto_03_80052504_text.s)
#include <math.h>

#include "types.h"
#include "engine/math.h"

typedef f32 Scalar;

static inline f32 Rsqrt(f32 v) { return 1.0f / sqrtf(v); }

extern "C" {

// 0x80052640
void Vec3Copy(Vec3* out, const Vec3* v) { out->x = v->x; out->y = v->y; out->z = v->z; }

// 0x800531A8
void Vec3SetZero(Vec3* out) { out->x = out->y = out->z = 0.0f; }

// 0x80053224
void Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s) {
    out->x = v->x * *s; out->y = v->y * *s; out->z = v->z * *s;
}

// 0x80053250
void ScalarSet(Scalar* out, f32 v) { *out = v; }

// 0x80053324
void Vec3Negate(Vec3* out, const Vec3* v) { out->x = -v->x; out->y = -v->y; out->z = -v->z; }

// 0x80053354
void Vec3Assign(Vec3* out, const Vec3* v) { out->x = v->x; out->y = v->y; out->z = v->z; }

// 0x80053370
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b) {
    out->x = a->x - b->x; out->y = a->y - b->y; out->z = a->z - b->z;
}

// 0x800533B0
BOOL ScalarLess(const Scalar* a, const Scalar* b) { return *a < *b; }

// 0x800544FC
void Vec3AddTo(Vec3* v, const Vec3* d) { v->x += d->x; v->y += d->y; v->z += d->z; }

// 0x8005461C
void Vec3SubFrom(Vec3* v, const Vec3* d) { v->x -= d->x; v->y -= d->y; v->z -= d->z; }

// 0x80054828
f32 Vec3GetX(const Vec3* v) { return v->x; }
// 0x80054830
f32 Vec3GetY(const Vec3* v) { return v->y; }
// 0x80054838
f32 Vec3GetZ(const Vec3* v) { return v->z; }

// 0x800549A0
void Vec3SetZ(Vec3* v, f32 z) { v->z = z; }

// 0x800549A8
void Vec3SetUnitZ(Vec3* v) { v->x = v->y = 0.0f; v->z = 1.0f; }

// 0x800549C0
void Vec3Dot(Scalar* out, const Vec3* a, const Vec3* b) { *out = a->x * b->x + a->y * b->y + a->z * b->z; }

// 0x80054A3C
void Vec3ScaleBy(Vec3* v, const Scalar* s) { v->x *= *s; v->y *= *s; v->z *= *s; }

// 0x80054A90
f32 Vec3LengthSqOf(const Vec3* v) { return v->x * v->x + v->y * v->y + v->z * v->z; }

// 0x80055D9C
void Vec3NormalizeTo(Vec3* out, const Vec3* v) {
    f32 sq = v->z * v->z + (v->x * v->x + v->y * v->y);
    if (sq == 0.0f) {
        out->x = out->y = out->z = 0.0f;
        return;
    }
    f32 r = Rsqrt(sq);
    out->x = v->x * r; out->y = v->y * r; out->z = v->z * r;
}

// 0x80055E2C
void ScalarZero(Scalar* out) { *out = 0.0f; }

// 0x80057128
void Vec3Length(Scalar* out, const Vec3* v) {
    f32 sq = v->z * v->z + (v->x * v->x + v->y * v->y);
    *out = sq <= 0.0f ? 0.0f : Rsqrt(sq) * sq;
}

// 0x80058214
void ScalarMul(Scalar* out, const Scalar* a, const Scalar* b) { *out = *a * *b; }

// 0x80058230
void Vec3SetX(Vec3* v, f32 x) { v->x = x; }
// 0x80058238
void Vec3SetY(Vec3* v, f32 y) { v->y = y; }

// 0x80058240
f32 FloatAbs(f32 v) { return fabsf(v); }

// 0x8005824C
// (point - origin) in the frame of the 3x3 rotation `m` (rows at +0, +0xC, +0x18).
void Vec3ToLocal(Vec3* out, const Vec3* point, const Vec3* origin, const f32 m[9]) {
    f32 dx = point->x - origin->x, dy = point->y - origin->y, dz = point->z - origin->z;
    out->x = m[2] * dz + (m[0] * dx + m[1] * dy);
    out->y = m[5] * dz + (m[3] * dx + m[4] * dy);
    out->z = m[8] * dz + (m[6] * dx + m[7] * dy);
}

// 0x80058324
// Rotate `v` by the transpose of `m` (local -> world) and normalise.
void Vec3RotateNormalized(Vec3* out, const Vec3* v, const f32 m[9]) {
    f32 x = m[6] * v->z + (m[0] * v->x + m[3] * v->y);
    f32 y = m[7] * v->z + (m[1] * v->x + m[4] * v->y);
    f32 z = m[8] * v->z + (m[2] * v->x + m[5] * v->y);
    out->x = x; out->y = y; out->z = z;
    f32 sq = z * z + (x * x + y * y);
    Vec3 n = { 0.0f, 0.0f, 0.0f };
    if (sq != 0.0f) {
        f32 r = Rsqrt(sq);
        n.x = x * r; n.y = y * r; n.z = z * r;
    }
    *out = n;
}

// 0x8005844C
void Vec3Rotate(Vec3* out, const f32 m[9], const Vec3* v) {
    f32 x = v->x, y = v->y, z = v->z;
    out->x = m[6] * z + (m[0] * x + m[3] * y);
    out->y = m[7] * z + (m[1] * x + m[4] * y);
    out->z = m[8] * z + (m[2] * x + m[5] * y);
}

}  // extern "C"
