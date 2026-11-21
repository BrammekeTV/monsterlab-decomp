// Move a value towards a target by a step that grows with the distance.
// Original: 0x801F5E94
#include "types.h"
#include <math.h>

// 0x801F5E94
// The step is lo + (|target - cur| / range) * (hi - lo), clamped between lo and hi (whichever
// is larger). Returns the target when the step would pass it.
extern "C" f32 fn_801F5E94(f32 cur, f32 target, f32 lo, f32 hi, f32 range) {
    f32 dist = fabsf(target - cur);
    f32 step = lo + (dist / range) * (hi - lo);
    if (lo < hi) {
        if (step < lo) step = lo;
        else if (step > hi) step = hi;
    } else {
        if (step < hi) step = hi;
        else if (step > lo) step = lo;
    }
    if (step > dist) {
        return target;
    }
    if (target < cur) {
        step = step * -1.0f;
    }
    return cur + step;
}

// 0x801F5F28
// The same for a point: `out` is `cur` moved towards `target` by lo + (distance / range) *
// (hi - lo), clamped between lo and hi, or `target` itself when the step is longer than the
// distance.
extern "C" void fn_801F5F28(f32* out, const f32* cur, const f32* target, f32 lo, f32 hi, f32 range) {
    f32 dx = target[0] - cur[0];
    f32 dy = target[1] - cur[1];
    f32 dz = target[2] - cur[2];
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 dist = 0.0f;
    f32 nx = 0.0f, ny = 0.0f, nz = 0.0f;
    if (len2 != 0.0f) {
        if (len2 > 0.0f) dist = sqrtf(len2);
        f32 inv = 1.0f / dist;
        nx = dx * inv;
        ny = dy * inv;
        nz = dz * inv;
    }
    f32 step = lo + (dist / range) * (hi - lo);
    if (lo < hi) {
        if (step < lo) step = lo;
        else if (step > hi) step = hi;
    } else {
        if (step < hi) step = hi;
        else if (step > lo) step = lo;
    }
    if (step > dist) {
        f32 x = target[0], y = target[1], z = target[2];
        out[0] = x;
        out[1] = y;
        out[2] = z;
        return;
    }
    f32 x = cur[0] + nx * step;
    f32 y = cur[1] + ny * step;
    f32 z = cur[2] + nz * step;
    out[0] = x;
    out[1] = y;
    out[2] = z;
}
