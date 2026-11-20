// Engine helpers in 0x801Cxxxx: oriented-box overlap test, trail/ribbon emitter update and
// the "pick next entry" selector for a list of 4-byte zone references.
#include "types.h"
#include "engine/math.h"

extern "C++" s32 gZoneResourceSize;   // 0x805FB94C

extern "C" {
u32  RandomU32(void);                                   // 0x80143460
s32  rand(void);                                        // 0x80337150
s32  fn_801B7A4C(u8* p, s32 mode);                      // 0x801B7A4C
s32  fn_801C71EC(u8* key);                              // 0x801C71EC
extern u8  gWorldState[];                               // 0x803E6D00 (first word: world block pointer)
extern f32 lbl_805F95A4;                                // 0x805F95A4 (float, 1.01)
extern u8  lbl_805FB6F9;                                // 0x805FB6F9 (byte flag)

// Separating-axis overlap test of two oriented boxes. arg0/arg3 are the box centres, arg1/arg4
// their half extents, arg2 the 3x3 rotation of box B relative to A (rows at +0/+0xC/+0x18).
// Returns 1 when they overlap; arg6 receives the normalised centre delta and arg5 its negation
// (both optional).
// 0x801C098C
s32 fn_801C098C(u8* arg0, u8* arg1, u8* arg2, u8* arg3, u8* arg4, f32* arg5, f32* arg6) {
    f32 dx = *(f32*)(arg3 + 0x0) - *(f32*)(arg0 + 0x0);
    f32 dy = *(f32*)(arg3 + 0x4) - *(f32*)(arg0 + 0x4);
    f32 dz = *(f32*)(arg3 + 0x8) - *(f32*)(arg0 + 0x8);

    // Rotation of box B relative to A (rows) and the extents of A (arg1) / B (arg4).
    f32 a0 = *(f32*)(arg2 + 0x0), a1 = *(f32*)(arg2 + 0x4), a2 = *(f32*)(arg2 + 0x8);
    f32 b0 = *(f32*)(arg2 + 0xC), b1 = *(f32*)(arg2 + 0x10), b2 = *(f32*)(arg2 + 0x14);
    f32 c0 = *(f32*)(arg2 + 0x18), c1 = *(f32*)(arg2 + 0x1C), c2 = *(f32*)(arg2 + 0x20);
    f32 ex = *(f32*)(arg4 + 0x0), ey = *(f32*)(arg4 + 0x4), ez = *(f32*)(arg4 + 0x8);
    f32 hx = *(f32*)(arg1 + 0x0), hy = *(f32*)(arg1 + 0x4), hz = *(f32*)(arg1 + 0x8);

    f32 fa0 = fabsf(a0), fa1 = fabsf(a1), fa2 = fabsf(a2);
    f32 fb0 = fabsf(b0), fb1 = fabsf(b1), fb2 = fabsf(b2);
    f32 fc0 = fabsf(c0), fc1 = fabsf(c1), fc2 = fabsf(c2);

    // Delta projected on the three axes of B.
    f32 pa = a2 * dz + (a0 * dx + a1 * dy);
    if (fabsf(pa) > (fa2 * ez + (fa0 * ex + fa1 * ey)) + hx) return 0;
    f32 pb = b2 * dz + (b0 * dx + b1 * dy);
    if (fabsf(pb) > (fb2 * ez + (fb0 * ex + fb1 * ey)) + hy) return 0;
    f32 cz = c2 * dz;
    f32 pc = cz + (c0 * dx + c1 * dy);
    if (fabsf(pc) > (fc2 * ez + (fc0 * ex + fc1 * ey)) + hz) return 0;

    // Axes of A.
    if (fabsf(dx) > (fc0 * hz + (fa0 * hx + fb0 * hy)) + ex) return 0;
    if (fabsf(dy) > (fc1 * hz + (fa1 * hx + fb1 * hy)) + ey) return 0;
    if (fabsf(dz) > (fc2 * hz + (fa2 * hx + fb2 * hy)) + ez) return 0;

    // Cross-product axes.
    if (fabsf(pc * b0 - pb * c0) > (fc0 * hy + fb0 * hz) + (fa2 * ey + fa1 * ez)) return 0;
    if (fabsf(pc * b1 - pb * c1) > (fc1 * hy + fb1 * hz) + (fa2 * ex + fa0 * ez)) return 0;
    if (fabsf(pc * b2 - pb * c2) > (fc2 * hy + fb2 * hz) + (fa1 * ex + fa0 * ey)) return 0;
    if (fabsf(pa * c0 - pc * a0) > (fc0 * hx + fa0 * hz) + (fb2 * ey + fb1 * ez)) return 0;
    if (fabsf(pa * c1 - pc * a1) > (fc1 * hx + fa1 * hz) + (fb2 * ex + fb0 * ez)) return 0;
    if (fabsf(pa * c2 - pc * a2) > (fc2 * hx + fa2 * hz) + (fb1 * ex + fb0 * ey)) return 0;
    if (fabsf(pb * a0 - pa * b0) > (fb0 * hx + fa0 * hy) + (fc2 * ey + fc1 * ez)) return 0;
    if (fabsf(pb * a1 - pa * b1) > (fb1 * hx + fa1 * hy) + (fc2 * ex + fc0 * ez)) return 0;
    if (fabsf(pb * a2 - pa * b2) > (fb2 * hx + fa2 * hy) + (fc1 * ex + fc0 * ey)) return 0;

    if (arg5 != NULL || arg6 != NULL) {
        f32 nx, ny, nz;
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        if (len2 == 0.0f) {
            nx = 0.0f; ny = 0.0f; nz = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            nx = dx * inv; ny = dy * inv; nz = dz * inv;
        }
        if (arg6 != NULL) {
            arg6[0] = nx; arg6[1] = ny; arg6[2] = nz;
        }
        if (arg5 != NULL) {
            arg5[0] = -nx; arg5[1] = -ny; arg5[2] = -nz;
        }
    }
    return 1;
}

// Per-frame update of a trail/ribbon effect (arg0) by dt (farg0): ages the 32 per-segment
// timers, fades the vertex alpha bytes, scrolls the texture coordinates and applies drift.
// 0x801C5444
void fn_801C5444(u8* arg0, f32 farg0) {
    u8* def = *(u8**)arg0;
    if (*(s32*)(def + 0x54) != 0) {
        f32 v = *(f32*)(arg0 + 0x394) - farg0 * *(f32*)(def + 0x5C);
        *(f32*)(arg0 + 0x394) = v;
        if (v <= 0.0f) {
            *(f32*)(arg0 + 0x394) = v + 1.0f;
        }
    }
    def = *(u8**)arg0;
    if (*(u8*)(*(u8**)(def + 0x6C) + 0x5) != 0) return;

    f32 age = *(f32*)(arg0 + 0x390) - farg0;
    *(f32*)(arg0 + 0x390) = age;
    f32 lim = -*(f32*)(def + 0x50);
    if (age <= lim) {
        *(f32*)(arg0 + 0x390) = lim;
    }

    def = *(u8**)arg0;
    f32 decay;
    if (*(u8*)(*(u8**)(def + 0x6C) + 0x1) != 0) {
        decay = 0.0625f * (farg0 * (f32) * (u32*)(def + 0x2C));
    } else {
        decay = farg0 * (f32) * (u32*)(def + 0x2C);
    }

    // Count the live segments (timers at +0x30C, up to 32) while draining them.
    u32 n = 0;
    u8* t = arg0;
    while (n < 0x20 && *(f32*)(t + 0x30C) > 0.0f) {
        f32 w = *(f32*)(t + 0x30C) - decay;
        *(f32*)(t + 0x30C) = w;
        if (!(w >= 0.0f)) {
            *(f32*)(t + 0x30C) = 0.0f;
        }
        t += 4;
        n++;
    }
    *(u32*)(arg0 + 0x8) = n;

    if (n > 2) {
        if (lbl_805FB6F9 == 0) {
            // Alpha byte of each of the 32 vertices (stride 0x18, word at +0x14) from the timers.
            for (s32 i = 0; i < 32; i++) {
                u32* w = (u32*)(arg0 + 0x14 + i * 0x18);
                s32 a = (s32) * (f32*)(arg0 + 0x30C + i * 4);
                *w = ((u32)a << 24) | (*w & 0xFFFFFFu);
            }
        } else {
            for (s32 i = 0; i < 2; i++) {
                u32* w = (u32*)(arg0 + 0x14 + i * 0x18);
                *w = (RandomU32() << 24) | (*w & 0xFFFFFFu);
            }
            f32 keep = 1.0f - lbl_805F95A4;
            u8* seg = arg0;
            u32 k = 0;
            for (s32 iter = 0; iter < 16; iter++) {
                f32 fade = (1.0f - ((f32)k / (f32)(*(u32*)(arg0 + 0x8) - 4))) * 255.0f;
                if (fade < 0.0f) fade = 0.0f;
                s32 a = (s32)fade;
                u32* w0 = (u32*)(arg0 + iter * 0x30 + 0x14);
                *w0 = (*w0 & 0xFFFFFFu) | ((u32)a << 24);
                u32* w1 = (u32*)(arg0 + (k + 1) * 0x18 + 0x14);
                *w1 = ((u32)a << 24) | (*w1 & 0xFFFFFFu);
                k += 2;

                // Blend the two positions at +0x18 and +0x30 of this segment pair.
                f32 p0x = *(f32*)(seg + 0x18), p0y = *(f32*)(seg + 0x1C), p0z = *(f32*)(seg + 0x20);
                f32 p1x = *(f32*)(seg + 0x30), p1y = *(f32*)(seg + 0x34), p1z = *(f32*)(seg + 0x38);
                *(f32*)(seg + 0x18) = p0x * lbl_805F95A4 + p1x * keep;
                *(f32*)(seg + 0x1C) = p0y * lbl_805F95A4 + p1y * keep;
                *(f32*)(seg + 0x20) = p0z * lbl_805F95A4 + p1z * keep;
                *(f32*)(seg + 0x30) = p0x * keep + p1x * lbl_805F95A4;
                *(f32*)(seg + 0x34) = p0y * keep + p1y * lbl_805F95A4;
                *(f32*)(seg + 0x38) = p0z * keep + p1z * lbl_805F95A4;
                seg += 0x30;
            }
        }
    }

    // Scroll the texture coordinates of every segment pair after the first.
    def = *(u8**)arg0;
    f32 sv = *(f32*)(def + 0x34) * farg0;
    f32 su = *(f32*)(def + 0x30) * farg0;
    u32 j = 2;
    s32 off = 0x30;
    while (j < *(u32*)(arg0 + 0x8)) {
        u8* q = arg0 + off;
        j += 2;
        off += 0x30;
        *(f32*)(q + 0xC) = su + *(f32*)(q + 0xC);
        *(f32*)(q + 0x10) = sv + *(f32*)(q + 0x10);
        *(f32*)(q + 0x24) = su + *(f32*)(q + 0x24);
        *(f32*)(q + 0x28) = sv + *(f32*)(q + 0x28);
    }

    // Optional drift velocity (flag 0x10).
    if (*(u32*)(arg0 + 0x4) & 0x10) {
        def = *(u8**)arg0;
        f32 vx = *(f32*)(def + 0x40) * farg0;
        f32 vy = *(f32*)(def + 0x44) * farg0;
        f32 vz = *(f32*)(def + 0x48) * farg0;
        u8* q = arg0 + 0x30;
        u32 m = 2;
        while (m < *(u32*)(arg0 + 0x8)) {
            m += 2;
            *(f32*)(q + 0x18) = *(f32*)(q + 0x18) + vx;
            *(f32*)(q + 0x1C) = *(f32*)(q + 0x1C) + vy;
            *(f32*)(q + 0x20) = *(f32*)(q + 0x20) + vz;
            *(f32*)(q + 0x30) = *(f32*)(q + 0x30) + vx;
            *(f32*)(q + 0x34) = *(f32*)(q + 0x34) + vy;
            *(f32*)(q + 0x38) = *(f32*)(q + 0x38) + vz;
            q += 0x30;
        }
    }
}

// Picks the next entry of a list of 4-byte zone references (count at arg1+0x48, array at
// arg1+0x4C) into arg0, honouring the list's selection mode (+0x34: 0/1 sequential, 2 ping-pong,
// 3 random without repeat, 5 fixed, else random). arg2 is the currently used entry (to avoid
// repeating it with two candidates), arg3 a filter passed to fn_801B7A4C (0 = take all).
// 0x801C6C80
void fn_801C6C80(u8* arg0, u8* arg1, u8* arg2, s32 arg3) {
    u8  resA = 0;
    u8  resB = 0;
    u16 resC = 0xFFFF;
    u32 idx[32];  // the original has room for 8 on its stack
    u32 cnt = 0;

    if (*(s32*)(arg1 + 0x48) == 0) {
        *(u16*)(arg0 + 2) = 0xFFFF;
        *(u8*)(arg0 + 1) = 0;
        *(u8*)(arg0 + 0) = 0;
        return;
    }

    if (arg3 == 0) {
        u32 i = 0;
        for (;;) {
            cnt = *(u32*)(arg1 + 0x48);
            if (!(i < cnt)) break;
            idx[i] = i;
            i++;
        }
    } else {
        for (u32 i = 0; i < *(u32*)(arg1 + 0x48); i++) {
            u8* e = *(u8**)(arg1 + 0x4C) + i * 4;
            u8 key[4];
            key[0] = *(u8*)(e + 0);
            key[1] = *(u8*)(e + 1);
            *(u16*)(key + 2) = *(u16*)(e + 2);
            u8* zone = *(u8**)gWorldState;
            u8* p;
            if ((s32)key[1] > *(s32*)(zone + 0x4)) {
                p = NULL;
            } else {
                u8* res = *(u8**)(zone + 0x1A4) + key[1] * gZoneResourceSize;
                if (*(u16*)(key + 2) >= *(u32*)(res + 0xCC)) {
                    p = NULL;
                } else if (fn_801C71EC(key) == 0) {
                    p = NULL;
                } else {
                    u8* res2 = *(u8**)(*(u8**)gWorldState + 0x1A4) + key[1] * gZoneResourceSize;
                    p = *(u8**)(res2 + 0xD0) + *(u16*)(key + 2) * 0x50;
                }
            }
            if (p != NULL && fn_801B7A4C(p + 4, arg3) != 0) {
                idx[cnt] = i;
                cnt++;
            }
        }
    }

    if (cnt == 0) {
        *(u16*)(arg0 + 2) = resC;
        *(u8*)(arg0 + 1) = resB;
        *(u8*)(arg0 + 0) = resA;
        return;
    }
    if (cnt == 1) {
        u8* e = *(u8**)(arg1 + 0x4C) + idx[0] * 4;
        *(u16*)(arg0 + 2) = *(u16*)(e + 2);
        *(u8*)(arg0 + 1) = *(u8*)(e + 1);
        *(u8*)(arg0 + 0) = *(u8*)(e + 0);
        return;
    }

    // Locate the current entry (arg2) in the list: same zone slot and id, byte 0 ignored.
    u32 total = *(u32*)(arg1 + 0x48);
    u8* arr = *(u8**)(arg1 + 0x4C);
    u8 cur[4];
    cur[0] = *(u8*)(arg2 + 0);
    cur[1] = *(u8*)(arg2 + 1);
    *(u16*)(cur + 2) = *(u16*)(arg2 + 2);
    s32 pos;
    if (fn_801C71EC(cur) == 0) {
        pos = -1;
    } else {
        pos = -1;
        for (u32 i = 0; i < total; i++) {
            u8* e = arr + i * 4;
            if (*(u16*)(cur + 2) == *(u16*)(e + 2) && cur[1] == *(u8*)(e + 1)) {
                pos = (s32)i;
                break;
            }
        }
    }
    if (pos != -1 && *(u32*)(arg1 + 0x48) != cnt) {
        // Translate to the position within the filtered list.
        s32 found = -1;
        for (u32 k = 0; k < cnt; k++) {
            if ((u32)pos == idx[k]) {
                found = (s32)k;
                break;
            }
        }
        pos = found;
    }

    s32 mode = *(s32*)(arg1 + 0x34);
    s32 sel;
    if (cnt == 2 && mode == 0) {
        // Two candidates: take the one that is not current (random when unknown).
        if (pos == -1) {
            sel = (s32)((u32)rand() % cnt);
        } else {
            sel = (s32)(cnt - 1) - pos;
        }
        u8* e = *(u8**)(arg1 + 0x4C) + idx[sel] * 4;
        resC = *(u16*)(e + 2);
        resB = *(u8*)(e + 1);
        resA = *(u8*)(e + 0);
    } else {
        if (mode == 3) {
            // Random without repeat until every entry was used (bit mask at +0x38).
            sel = (s32)((u32)rand() % cnt);
            u32 used = *(u32*)(arg1 + 0x38);
            u32 n = *(u32*)(arg1 + 0x48);
            u32 full = (((n & 0x20) ? 0u : (1u << (n & 31))) - 1u);
            if (used == full) {
                *(u32*)(arg1 + 0x38) = 1u << (idx[sel] & 31);
            } else {
                u32 bit = 1u << (idx[sel] & 31);
                if (!(used & bit)) {
                    *(u32*)(arg1 + 0x38) = used | bit;
                } else {
                    s32 s = sel + 1;
                    if (s >= (s32)cnt) s = 0;
                    while (s != sel && (*(u32*)(arg1 + 0x38) & (1u << (idx[s] & 31)))) {
                        s++;
                        if (s >= (s32)cnt) s = 0;
                    }
                    if (!(*(u32*)(arg1 + 0x38) & (1u << (idx[s] & 31)))) {
                        sel = s;
                    }
                    *(u32*)(arg1 + 0x38) = *(u32*)(arg1 + 0x38) | (1u << (idx[sel] & 31));
                }
            }
        } else if (mode > 3) {
            if (mode == 5) {
                sel = *(s32*)(arg1 + 0x38);
            } else {
                sel = (s32)((u32)rand() % cnt);
            }
        } else if (mode >= 2) {
            // Ping-pong through the list (direction flag at +0x3A, position at +0x38).
            if (*(u16*)(arg1 + 0x3A) == 0) {
                u16 curPos = *(u16*)(arg1 + 0x38);
                sel = curPos + 1;
                if (sel >= (s32)cnt) {
                    sel = curPos - 1;
                    *(u16*)(arg1 + 0x3A) = 1;
                }
            } else {
                sel = *(u16*)(arg1 + 0x38) - 1;
                if (sel < 0) {
                    sel = 1;
                    *(u16*)(arg1 + 0x3A) = 0;
                }
            }
            *(u16*)(arg1 + 0x38) = (u16)sel;
        } else if (mode >= 0) {
            // Sequential, forwards or backwards by the flag at +0x3A.
            if (*(u16*)(arg1 + 0x3A) == 0) {
                sel = *(u16*)(arg1 + 0x38) + 1;
                if (sel >= (s32)cnt) sel = 0;
            } else {
                sel = *(u16*)(arg1 + 0x38) - 1;
                if (sel < 0) sel = (s32)cnt - 1;
            }
            *(u16*)(arg1 + 0x38) = (u16)idx[sel];
        } else {
            sel = (s32)((u32)rand() % cnt);
        }
        u8* e = *(u8**)(arg1 + 0x4C) + idx[sel] * 4;
        resC = *(u16*)(e + 2);
        resB = *(u8*)(e + 1);
        resA = *(u8*)(e + 0);
    }
    *(u16*)(arg0 + 2) = resC;
    *(u8*)(arg0 + 1) = resB;
    *(u8*)(arg0 + 0) = resA;
}
}
