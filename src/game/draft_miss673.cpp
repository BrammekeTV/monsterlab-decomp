// Experiment-station helpers in 0x8008xxxx-0x8009xxxx (series sum, nearest-target search, meter / sound updates).
#include "types.h"
#include <math.h>
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32   RaySceneTest(void* ray);                                   // 0x80070458 (ignores its argument)
void  StickRead(s32 channel, f32* x, f32* y);                    // 0x80147704
void  UISetProgress(void* obj, f32 t);
void* UIWidgetFindChild(void* widget, u32 hash);                 // 0x80199AEC
void  fn_80070E30(void* owner, u32 id, f32 v);                   // 0x80070E30
void* fn_80070F4C(void);                                         // 0x80070F4C
void  fn_80097BCC(f32* out, u8* obj);                            // 0x80097BCC
s32   fn_8016195C(void* p, s32 a, s32 b, void* c);              // 0x8016195C
void  fn_80162990(u8* p, s32 h);                                 // 0x80162990
f64   fn_8033F1F4(f64 x);                                        // 0x8033F1F4 (MSL acos)

extern f32 lbl_805FD928;   // 0x805FD928
extern f32 lbl_805FD948;   // 0x805FD948
extern f32 lbl_805FD950;   // 0x805FD950
extern f32 lbl_805FD9A8;   // 0x805FD9A8
extern f32 lbl_805FD9AC;   // 0x805FD9AC
extern f32 lbl_805FDB28;   // 0x805FDB28
extern f32 lbl_805FDB38;   // 0x805FDB38
extern f32 lbl_805FDB64;   // 0x805FDB64
extern f32 lbl_805FDB78;   // 0x805FDB78
extern f32 lbl_805FDB80;   // 0x805FDB80
extern f32 lbl_805FDB84;   // 0x805FDB84
extern f32 lbl_805FDBA8;   // 0x805FDBA8
extern f32 lbl_805FDBAC;   // 0x805FDBAC
extern f32 lbl_805FDBB0;   // 0x805FDBB0
extern f32 lbl_805FDBB4;   // 0x805FDBB4
extern f32 lbl_805FDBB8;   // 0x805FDBB8
extern f32 lbl_805FDBBC;   // 0x805FDBBC
extern f32 lbl_805FDD18;   // 0x805FDD18
extern f32 lbl_805FDD1C;   // 0x805FDD1C
extern f32 lbl_805FDD24;   // 0x805FDD24
extern f32 lbl_805FDD50;   // 0x805FDD50
extern f32 lbl_805FDD54;   // 0x805FDD54
extern f32 lbl_805FC5E0[]; // 0x805FC5E0 (two floats)

extern f32 lbl_80347080[];  // 0x80347080
extern u32 lbl_80347DA8[];  // 0x80347DA8 (3 sound ids)
extern f32 lbl_80346338[];  // 0x80346338
extern f32 lbl_80346358[];  // 0x80346358
extern f32 lbl_80346438[];  // 0x80346438
extern f32 lbl_80346448[];  // 0x80346448
extern f32 lbl_80346458[];  // 0x80346458
extern f32 lbl_80346468[];  // 0x80346468
extern s32 lbl_803463B8[];  // 0x803463B8
extern f32 lbl_803462D8[];  // 0x803462D8
extern f32 lbl_803462E8[];  // 0x803462E8
extern f32 lbl_80346308[];  // 0x80346308
extern u8  lbl_803C4298[];  // 0x803C4298
extern u8  lbl_8040E5F0[];  // 0x8040E5F0 (frame effects object)
}

static const u32 kHashAstralRiftMeter = 0xEB3D5086u;  // "astral_rift_meter"

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtGuard(f32 v) {
    return v <= 0.0f ? 0.0f : sqrtf(v);
}

extern "C" {

// 0x8008AC08
// Arithmetic series sum of (b + a*i) for i in [0,n) with (a,b) picked by mode (2,3: 2.5/0.3; 4: 2.5/0.1).
// r3 is unused by the function. n == 1 returns a fixed constant. The asm's type-2/3 series branches are
// unreachable (type is only ever 0 or 1).
f32 fn_8008AC08(s32 unused, s32 n, s32 mode) {
    f32 a = lbl_805FD928;
    f32 b = lbl_805FD928;
    f32 sum = lbl_805FD928;
    s32 type = 0;
    if (mode == 2) {
        a = lbl_805FD9A8;
        type = 1;
        b = lbl_805FD9AC;
    } else if (mode == 3) {
        a = lbl_805FD9A8;
        type = 1;
        b = lbl_805FD9AC;
    } else if (mode == 4) {
        a = lbl_805FD9A8;
        type = 1;
        b = lbl_805FD950;
    }
    if (type == 1) {
        sum = b;
        for (s32 i = 1; i < n; i++) {
            sum += b + a * (f32)i;
        }
    }
    if (n == 1) sum = lbl_805FD948;
    return sum;
}

// 0x800964AC
// Returns the live candidate (slot list at +0x9C, count at +0x1B4) whose direction from fn_80097BCC is
// closest to straight up (smallest angle); skips +0x1D8 and, when mode != 0, +0x1DC.
u8* fn_800964AC(u8* p, s32 mode) {
    f32 zero = lbl_805FDB28;
    f32 one = lbl_805FDB64;
    f32 upX = zero;
    f32 upY = one;
    f32 best = lbl_805FDB78;
    u8* found = NULL;
    for (s32 i = 0; i < *(s32*)(p + 0x1B4); i++) {
        u8* cand = *(u8**)(p + i * 4 + 0x9C);
        s32 ok = 0;
        if (*(u8*)(cand + 0x26E) != 0 && *(u8*)(cand + 0x26C) > 0 && cand != *(u8**)(p + 0x1D8)) {
            ok = 1;
        }
        if (mode != 0 && cand == *(u8**)(p + 0x1DC)) ok = 0;
        if (ok != 0) {
            f32 v[3];
            fn_80097BCC(v, cand);
            f32 dot = v[2] * upX + (v[0] * upX + v[1] * upY);
            f32 lenA = SqrtGuard(v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]));
            f32 sq = upX * upX;
            f32 lenB = SqrtGuard(sq + (sq + upY * upY));
            f32 prod = lenA * lenB;
            f32 ang;
            if (prod <= zero) {
                ang = zero;
            } else {
                f32 c = dot / prod;
                if (c < lbl_805FDB38) c = lbl_805FDB38;
                else if (c > one) c = one;
                ang = lbl_805FDB84 * (f32)fn_8033F1F4((f64)c);
            }
            if (ang < best) {
                found = cand;
                best = ang;
            }
        }
    }
    return found;
}

// 0x80096D60
// Drains the meter at +0x35C (clamped 0..1), then plays/updates/stops three zone sounds depending on which
// of three ranges the meter value is in; finally remembers the value at +0x360.
void fn_80096D60(u8* p, f32 dt) {
    f32 r[6];
    f32 zero;
    f32 one;
    f32 half;
    f32 v;
    f32 lo;
    f32 hi;
    f32 mid;
    f32 d;
    s32 i;

    v = *(f32*)(p + 0x35C) - dt * lbl_80347080[RaySceneTest(p)];
    *(f32*)(p + 0x35C) = v;
    if (v < lbl_805FDB28) {
        v = lbl_805FDB28;
    } else if (v > lbl_805FDB64) {
        v = lbl_805FDB64;
    }
    zero = lbl_805FDB28;
    one = lbl_805FDB64;
    half = lbl_805FDB80;
    *(f32*)(p + 0x35C) = v;
    lo = lbl_805FC5E0[0];
    hi = lbl_805FC5E0[1];
    f32 h = half * (hi - lo);
    r[0] = zero;
    r[5] = one;
    r[1] = lo + h;
    r[2] = lo - h;
    r[3] = hi + h;
    r[4] = hi - h;
    for (i = 0; i < 3; i++) {
        f32 cur = *(f32*)(p + 0x35C);
        lo = r[i * 2];
        hi = r[i * 2 + 1];
        if (cur >= lo && cur <= hi) {
            f32 prev = *(f32*)(p + 0x360);
            if (prev < lo || prev > hi) {
                PlayExperimentSound(NULL, lbl_80347DA8[i], 1);
            }
            hi = r[i * 2 + 1];
            lo = r[i * 2];
            f32 hw = half * (hi - lo);
            mid = hw + lo;
            d = *(f32*)(p + 0x35C) - mid;
            if ((i == 0 && d < zero) || (i == 2 && d > zero)) d = lbl_805FDB28;
            fn_80070E30(NULL, lbl_80347DA8[i], one - (f32)fabs(d) / hw);
        } else {
            f32 prev = *(f32*)(p + 0x360);
            if (prev >= r[i * 2] && prev <= r[i * 2 + 1]) {
                StopExperimentSound(NULL, lbl_80347DA8[i]);
            }
        }
    }
    *(f32*)(p + 0x360) = *(f32*)(p + 0x35C);
}

// 0x800977BC
// Shake effect: advances the timer at +0x1C, finds the last active entry of the owner's track list, takes the
// direction between that point and the end point, scales it by a sine mix and the highest level among the
// attached slots (+0x14 list of +8 entries, +0xC list of +9 entries), and nudges the end point by it.
void fn_800977BC(u8* p, f32 dt) {
    f32 dir[3];
    s32 n;
    s32 idx;
    s32 off;
    s32 maxLvl;
    u8* owner;
    u8* track;
    u8* base;
    u8* a;
    u8* b;
    u8* tgt;
    s32 cnt;
    s32 k;

    *(f32*)(p + 0x1C) = *(f32*)(p + 0x1C) + dt;
    owner = *(u8**)p;
    track = *(u8**)((*(u8*(**)(u8*))(*(u8**)owner + 0x10))(owner));
    n = *(s32*)(p + 0x4);
    idx = n - 1;
    off = idx * 8;
    if (idx > 0) {
        s32 c = idx;
        s32 hit = 0;
        for (; c > 0; c--) {
            if (*(u32*)(*(u8**)(track + 0xC) + off + 4) & 1) {
                hit = 1;
                break;
            }
            off -= 8;
            idx--;
        }
        if (!hit) idx = 0;
    } else {
        idx = 0;
    }
    base = *(u8**)(track + 0x4);
    cnt = *(u8*)(p + 0x8);
    maxLvl = 0;
    a = base + idx * 0x30;
    b = base + n * 0x30;
    dir[0] = *(f32*)(b + 0x24) - *(f32*)(a + 0x24);
    dir[1] = *(f32*)(b + 0x28) - *(f32*)(a + 0x28);
    dir[2] = *(f32*)(b + 0x2C) - *(f32*)(a + 0x2C);
    for (k = 0; k < cnt; k++) {
        s32 lvl = *(u8*)(*(u8**)(p + 0x14 + k * 4) + 0x26C);
        if (lvl > maxLvl) maxLvl = lvl;
    }
    cnt = *(u8*)(p + 0x9);
    for (k = 0; k < cnt; k++) {
        s32 lvl = *(u8*)(*(u8**)(p + 0xC + k * 4) + 0x26C);
        if (lvl > maxLvl) maxLvl = lvl;
    }
    if (maxLvl > 0) {
        f32 t;
        f32 s1;
        f32 s2;
        f32 s3;
        f32 scale;
        tgt = *(u8**)(track + 0x4) + n * 0x30;
        t = *(f32*)(p + 0x1C) * lbl_805FDBA8;
        s1 = sinf(lbl_805FDBAC * t);
        s2 = sinf(lbl_805FDBAC * (lbl_805FDBB0 * (*(f32*)(p + 0x1C) * lbl_805FDBA8)));
        s3 = sinf(lbl_805FDBAC * (lbl_805FDBB4 * (*(f32*)(p + 0x1C) * lbl_805FDBA8)));
        scale = ((f32)maxLvl * (lbl_805FDBB8 * (s3 + (s1 + s2)))) * lbl_805FDBBC;
        *(f32*)(tgt + 0x24) = *(f32*)(tgt + 0x24) + dir[0] * scale;
        *(f32*)(tgt + 0x28) = *(f32*)(tgt + 0x28) + dir[1] * scale;
        *(f32*)(tgt + 0x2C) = *(f32*)(tgt + 0x2C) + dir[2] * scale;
    }
}

// 0x8009CBE8
// Eases the level at +0x3F4 toward 1 (down with a stop/play sound pair when it reaches 1, up otherwise) and
// pushes a stick-driven value to the owner's (+0x264) callback at slot +0x1C of the table at (+0x9C)+0xD4.
void fn_8009CBE8(u8* p, f32 dt) {
    f32 stick[2];  // stick[0] = x (sp 0xC), stick[1] = y (sp 0x8)
    f32 cur = *(f32*)(p + 0x3F4);
    f32 one = lbl_805FDD1C;
    if (cur > one) {
        f32 v = *(f32*)(p + 0x3F4) - dt * lbl_80346338[RaySceneTest(p)];
        f32 out = lbl_805FDD1C;
        if (!(out > v)) out = v;
        *(f32*)(p + 0x3F4) = out;
        if (lbl_805FDD1C == (f32)out) {
            StopExperimentSound(NULL, 0x27B8449Du);
            PlayExperimentSound(NULL, 0xC265C05Fu, 0);
        }
    } else if (cur < one) {
        f32 v = *(f32*)(p + 0x3F4) + dt * lbl_80346358[RaySceneTest(p)];
        f32 out = lbl_805FDD1C;
        if (out >= v) out = v;
        *(f32*)(p + 0x3F4) = out;
    }
    stick[1] = lbl_805FDD18;
    stick[0] = lbl_805FDD18;
    StickRead(*(s32*)(lbl_803C4298 + 0x38) * 0x10 + 0xE, &stick[0], &stick[1]);
    f32 a = lbl_80346438[RaySceneTest(NULL)];  // r3 leftover from StickRead
    f32 b = lbl_80346438[RaySceneTest(NULL)];  // r3 leftover
    f32 c = lbl_80346448[RaySceneTest(NULL)];  // r3 leftover
    s32 lvl = *(s32*)(*(u8**)(p + 0x264) + 0x17C);
    b = c - b;
    s32 div = lbl_803463B8[RaySceneTest(NULL)];  // r3 leftover
    f32 y = stick[1];
    f32 total = a + b * ((f32)lvl / (f32)div);
    if (y > lbl_805FDD18) {
        y = y * lbl_80346458[RaySceneTest(NULL)];  // r3 leftover
    } else {
        y = y * lbl_80346468[RaySceneTest(NULL)];  // r3 leftover
    }
    u8* obj = *(u8**)(p + 0x264) + 0x9C;
    total = (total + y) * *(f32*)(p + 0x3F4);
    (*(void (**)(u8*, f32))(*(u8**)(obj + 0xD4) + 0x1C))(obj, total * dt);
}

// 0x8009CE40
// Timer/meter update for the experiment station: +0x3EC is a countdown (negative = idle), +0x3E8 the meter.
// While the countdown is running the meter drains and the effect handle (+0x400) is released at the end;
// otherwise a held button (+0x3B4) with a non-empty meter starts the countdown, sound and screen effect.
// The meter is shown on the astral_rift_meter widget.
void fn_8009CE40(u8* p, f32 dt) {
    f32 zero = lbl_805FDD18;
    if (*(f32*)(p + 0x3EC) >= zero) {
        s32 held = 0;
        if (*(u8*)(p + 0x3B4) != 0) {
            f32 t = *(f32*)(p + 0x3B8);
            if (t > zero) *(f32*)(p + 0x32C) = t;
        }
        if (*(u8*)(p + 0x3B4) != 0) held = 1;
        if (held != 0 || *(f32*)(p + 0x3E8) <= lbl_805FDD18) {
            *(f32*)(p + 0x3EC) = lbl_805FDD24;
        } else {
            f32 lim;
            f32 v;
            f32 out;
            *(f32*)(p + 0x3EC) = *(f32*)(p + 0x3EC) - dt;
            lim = lbl_803462E8[RaySceneTest(p)];
            out = lbl_805FDD18;
            v = *(f32*)(p + 0x3E8) + -dt;
            if (out < lim) {
                if (v >= out) {
                    if (v > lim) v = lim;
                    out = v;
                }
            } else {
                if (v >= lim) {
                    if (v > out) v = out;
                    lim = v;
                }
                out = lim;
            }
            *(f32*)(p + 0x3E8) = out;
        }
        if (*(f32*)(p + 0x3EC) < lbl_805FDD18) {
            PlayExperimentSound(NULL, 0x3925C988u, 0);
            s32 h = *(s32*)(p + 0x400);
            if (h != 0) {
                fn_80162990(lbl_8040E5F0, h);
                *(s32*)(p + 0x400) = 0;
            }
        }
    } else {
        s32 held = 0;
        if (*(u8*)(p + 0x3B4) != 0) {
            f32 t = *(f32*)(p + 0x3B8);
            if (t > zero) *(f32*)(p + 0x32C) = t;
        }
        if (*(u8*)(p + 0x3B4) != 0) held = 1;
        if (held != 0 && *(f32*)(p + 0x3E8) > lbl_805FDD18) {
            f32 fx[10];
            *(f32*)(p + 0x3EC) = lbl_803462D8[RaySceneTest(p)];
            PlayExperimentSound(NULL, 0x3925C988u, 0);
            fx[4] = lbl_805FDD50;
            fx[5] = lbl_805FDD54;
            fx[0] = lbl_805FDD24;
            fx[1] = lbl_805FDD24;
            fx[2] = lbl_805FDD24;
            fx[3] = lbl_805FDD24;
            fx[6] = lbl_805FDD24;
            fx[7] = lbl_805FDD24;
            fx[8] = lbl_805FDD24;
            fx[9] = lbl_805FDD24;
            *(s32*)(p + 0x400) = fn_8016195C(lbl_8040E5F0, 0xF, 0, &fx[4]);
        } else {
            f32 lim;
            f32 v;
            f32 out;
            f32 step = dt * lbl_80346308[RaySceneTest(p)];
            out = lbl_805FDD18;
            v = *(f32*)(p + 0x3E8) + step;
            lim = lbl_803462E8[RaySceneTest(NULL)];  // r3 leftover
            if (out < lim) {
                if (v >= out) {
                    if (v > lim) v = lim;
                    out = v;
                }
            } else {
                if (v >= lim) {
                    if (v > out) v = out;
                    lim = v;
                }
                out = lim;
            }
            *(f32*)(p + 0x3E8) = out;
        }
    }
    void* w = fn_80070F4C();
    if (w != NULL) {
        UISetProgress(UIWidgetFindChild(w, kHashAstralRiftMeter), *(f32*)(p + 0x3E8));
    }
}

}  // extern "C"
