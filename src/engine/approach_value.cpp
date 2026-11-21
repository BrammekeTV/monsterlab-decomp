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
