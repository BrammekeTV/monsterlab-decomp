// UI anim track stepping, list membership filter, bezier smoothing, text measuring, effect spawn.
#include "types.h"
#include "engine/engine.h"
#include "engine/math.h"

extern "C" {
u32   RandomU32(void);                                   // 0x80143460
void  fn_8018D370(const char* fmt, ...);                 // 0x8018D370 (debug log)
u32   fn_8019552C(u8* list, s32 index);                  // 0x8019552C (leaf)
s32   fn_8019555C(u8* list);                             // 0x8019555C (leaf)
void  fn_801AA218(u8* out, u8* key, s32 a, s32 b);       // 0x801AA218
f32   fn_801AA728(u8* entry);                            // 0x801AA728 (leaf)
void  fn_801AA760(u8* out, u8* key);                     // 0x801AA760 (leaf)
void* fn_801EB94C(void* p);                              // 0x801EB94C
void  fn_801C5AAC(u8* node, u8* src);                    // 0x801C5AAC

extern u8 gWorldState[];                                 // 0x803E6D00
extern u8 lbl_805FB6F8;                                  // 0x805FB6F8
extern const char lbl_80383850[];                        // 0x80383850 (string)
extern f32 lbl_805FFB68;                                 // 0x805FFB68
extern f32 lbl_805FFB6C;                                 // 0x805FFB6C
extern f32 lbl_805FFB70;                                 // 0x805FFB70
extern f32 lbl_805FFBB0;                                 // 0x805FFBB0
extern f32 lbl_805FFBB4;                                 // 0x805FFBB4
extern f32 lbl_805FFBD0;                                 // 0x805FFBD0
extern f32 lbl_805FFC08;                                 // 0x805FFC08
extern f32 lbl_805FFCB4;                                 // 0x805FFCB4
extern f32 lbl_805FFCB8;                                 // 0x805FFCB8
extern f32 lbl_805FFCD0;                                 // 0x805FFCD0
extern f32 lbl_805FFCD4;                                 // 0x805FFCD4
extern f32 lbl_805FFCD8;                                 // 0x805FFCD8

typedef u8* (*VFn)(u8*);

// 0x801AA974
// Steps a keyframe cursor (list at +4, index +8, current key +0xC, next key +0x10) toward time t.
// dir == 0 walks forward, otherwise backward. Always returns 0.
s32 fn_801AA974(u8* p, s32 unused, s32 dir, f32 t) {
    s32 done = 0;
    if (*(s32*)(p + 0xC) == 0 && *(s32*)(p + 0x10) == 0) {
        if (dir == 0) {
            *(s32*)(p + 0x8) = 0;
        } else {
            *(s32*)(p + 0x8) = fn_8019555C(*(u8**)(p + 0x4)) - 1;
        }
        do {
            s32 idx = *(s32*)(p + 0x8);
            if (fn_8019555C(*(u8**)(p + 0x4)) > idx) {
                *(u8**)(p + 0xC) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), idx);
            } else {
                *(u8**)(p + 0xC) = 0;
            }
            if (dir == 0) {
                s32 n = *(s32*)(p + 0x8) + 1;
                if (fn_8019555C(*(u8**)(p + 0x4)) > n) {
                    *(u8**)(p + 0x10) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), n);
                } else {
                    *(u8**)(p + 0x10) = 0;
                }
                u8* e = *(u8**)(p + 0x10);
                if (e != 0 && fn_801AA728(e) <= t) {
                    *(s32*)(p + 0x8) = *(s32*)(p + 0x8) + 1;
                } else {
                    done = 1;
                }
            } else {
                u8* e = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), *(s32*)(p + 0x8) - 1);
                *(u8**)(p + 0x10) = e;
                if (e != 0 && fn_801AA728(e) >= t) {
                    *(s32*)(p + 0x8) = *(s32*)(p + 0x8) - 1;
                } else {
                    done = 1;
                }
            }
        } while (*(s32*)(p + 0xC) != 0 && *(s32*)(p + 0x10) != 0 && done == 0);
        return 0;
    }
    if (dir == 0) {
        u8* e = *(u8**)(p + 0x10);
        if (e != 0 && t >= fn_801AA728(e)) {
            s32 n = *(s32*)(p + 0x8) + 1;
            *(s32*)(p + 0x8) = n;
            if (n + 1 >= fn_8019555C(*(u8**)(p + 0x4))) {
                s32 last = fn_8019555C(*(u8**)(p + 0x4)) - 1;
                *(s32*)(p + 0x8) = last;
                *(u8**)(p + 0xC) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), last);
                *(u8**)(p + 0x10) = 0;
                return 0;
            }
            *(u8**)(p + 0xC) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), n);
            *(u8**)(p + 0x10) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), n + 1);
            return 0;
        }
    }
    if (dir != 0) {
        u8* e = *(u8**)(p + 0x10);
        if (e != 0 && t <= fn_801AA728(e)) {
            s32 n = *(s32*)(p + 0x8) - 1;
            *(s32*)(p + 0x8) = n;
            if (n - 1 < 0) {
                *(s32*)(p + 0x8) = 0;
                *(u8**)(p + 0xC) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), 0);
                *(u8**)(p + 0x10) = 0;
                return 0;
            }
            *(u8**)(p + 0xC) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), n);
            *(u8**)(p + 0x10) = (u8*)(uintptr_t)fn_8019552C(*(u8**)(p + 0x4), n - 1);
        }
    }
    return 0;
}

// 0x801AAC08
// UIAnimTrack apply: reads the current key value (4 floats) and writes 1..4 components
// (or an int for type 0) to the target at +0x18, optionally adding the base values at +0x20.
s32 fn_801AAC08(u8* p) {
    f32 v[4];
    if (*(s32*)(p + 0x10) != 0) {
        fn_801AA218((u8*)v, *(u8**)(p + 0xC), *(s32*)(p + 0x10), 0);
    } else {
        fn_801AA760((u8*)v, *(u8**)(p + 0xC));
    }
    u8* dst = *(u8**)(p + 0x18);
    switch (*(s32*)(p + 0x14)) {
    case 0:
        if (*(u8*)(p + 0x1C) != 0) {
            *(s32*)dst = *(s32*)(p + 0x20) + (s32)v[0];
        } else {
            *(s32*)dst = (s32)v[0];
        }
        break;
    case 1:
        if (*(u8*)(p + 0x1C) != 0) {
            *(f32*)dst = *(f32*)(p + 0x20) + v[0];
        } else {
            *(f32*)dst = v[0];
        }
        break;
    case 2:
        if (*(u8*)(p + 0x1C) != 0) {
            *(f32*)(dst + 0x0) = *(f32*)(p + 0x20) + v[0];
            *(f32*)(dst + 0x4) = *(f32*)(p + 0x24) + v[1];
        } else {
            *(f32*)(dst + 0x0) = v[0];
            *(f32*)(dst + 0x4) = v[1];
        }
        break;
    case 3:
        if (*(u8*)(p + 0x1C) != 0) {
            *(f32*)(dst + 0x0) = *(f32*)(p + 0x20) + v[0];
            *(f32*)(dst + 0x4) = *(f32*)(p + 0x24) + v[1];
            *(f32*)(dst + 0x8) = *(f32*)(p + 0x28) + v[2];
        } else {
            *(f32*)(dst + 0x0) = v[0];
            *(f32*)(dst + 0x4) = v[1];
            *(f32*)(dst + 0x8) = v[2];
        }
        break;
    case 4:
        if (*(u8*)(p + 0x1C) != 0) {
            *(f32*)(dst + 0x0) = *(f32*)(p + 0x20) + v[0];
            *(f32*)(dst + 0x4) = *(f32*)(p + 0x24) + v[1];
            *(f32*)(dst + 0x8) = *(f32*)(p + 0x28) + v[2];
            *(f32*)(dst + 0xC) = *(f32*)(p + 0x2C) + v[3];
        } else {
            *(f32*)(dst + 0x0) = v[0];
            *(f32*)(dst + 0x4) = v[1];
            *(f32*)(dst + 0x8) = v[2];
            *(f32*)(dst + 0xC) = v[3];
        }
        break;
    default:
        fn_8018D370(lbl_80383850);
        break;
    }
    return 0;
}

// 0x801B7A4C
// Filter test: is the object allowed by the id lists in p? Returns 1 when it passes.
s32 fn_801B7A4C(u8* p, u8* obj) {
    if (obj == 0) return 0;
    u32 flags = *(u32*)(p + 0xC);
    if (flags != 0 && (flags & 1) != 0) {
        if (obj != *(u8**)(*(u8**)gWorldState + 0x8)) return 0;
    }
    if (*(u32*)(p + 0x4) != 0) {
        u32 key = *(u32*)(obj + 0x2C);
        s32 found = 0;
        u32 i = 0;
        while (i < *(u32*)(p + 0x4)) {
            if (key == (*(u32**)p)[i]) { found = 1; break; }
            i++;
        }
        if (found == 0) return 0;
    } else if (*(u32*)(p + 0x8) != 0) {
        u8* sub = *(u8**)(obj + 0x9C);
        if (sub == 0) return 0;
        u32 key = *(u32*)(sub + 0x8);
        s32 found = 0;
        u32 i = 0;
        while (i < *(u32*)(p + 0x8)) {
            if (key == (*(u32**)p)[i]) { found = 1; break; }
            i++;
        }
        if (found == 0) return 0;
    }
    return 1;
}

// 0x801B7C5C
// Ping-pong a value (float at +0x30 of obj's virtual slot 0xC result) between 0 and 1 by dt * speed (p+0).
// Returns 2.
s32 fn_801B7C5C(u8* p, u8* obj, f32 dt) {
    if (*(u8*)(p + 0x7F) != 0x20) return 2;
    f32 speed = *(f32*)(p + 0x0);
    f32 cur = *(f32*)((*(VFn*)(*(u8**)obj + 0xC))(obj) + 0x30);
    f32 step = dt * speed;
    f32 v;
    if (*(s32*)(p + 0x10) != 0) {
        v = cur + step;
        if (v >= lbl_805FFB70) {
            *(s32*)(p + 0x10) = 0;
            v = lbl_805FFB6C - v;
        }
    } else {
        v = cur - step;
        if (!(v >= lbl_805FFB68)) {
            v = -v;
            *(s32*)(p + 0x10) = 1;
        }
    }
    *(f32*)((*(VFn*)(*(u8**)obj + 0xC))(obj) + 0x30) = v;
    return 2;
}

// 0x801BA74C
// Measures text (u16 chars start..end, stops at 0) with the font glyph table (+8 map, +0xC glyphs of 0xC
// bytes, width at +8). Outputs width to *outW and height to *outH; with the setting on it handles
// multi-line text (widest line).
void fn_801BA74C(u8* font, u16* text, s32 start, u32 end, f32* outW, f32* outH) {
    if (EngineGetGameSettings()->skipDebugOverlay != 0) {
        f32 maxW = lbl_805FFBB4;
        u16* c = text + start;
        *outW = lbl_805FFBB4;
        while ((u32)start <= end && *c != 0) {
            u16 g = (*(u16**)(font + 0x8))[*c];
            *outW = *outW + *(f32*)(*(u8**)(font + 0xC) + g * 0xC + 0x8);
            if (*c == 0xA || *c == 0xD) {
                if (*outW > maxW) maxW = *outW;
                *outW = lbl_805FFBB4;
            }
            c++;
            start++;
        }
        if (maxW > *outW) *outW = maxW;
        *outW = *outW * (*(f32*)(font + 0x18) / *(f32*)(font + 0x10));
        *outH = lbl_805FFBB0;
    } else {
        u16* c = text + start;
        *outW = lbl_805FFBB4;
        while ((u32)start <= end && *c != 0) {
            u16 g = (*(u16**)(font + 0x8))[*c];
            c++;
            start++;
            *outW = *outW + *(f32*)(*(u8**)(font + 0xC) + g * 0xC + 0x8);
        }
        *outW = *outW * *(f32*)(font + 0x18);
        *outH = *(f32*)(font + 0x10);
    }
}

// 0x801BD828
// Smooths a point chain (stride 0xC from +0x4C, count at +0x38): each inner point is blended by t
// with the average of two neighbouring control values (+0x40 and +0x58 sets).
void fn_801BD828(u8* obj, f32 t) {
    f32 tmp[0x190 / 4];
    f32 inv = lbl_805FFC08 - t;
    f32 half = lbl_805FFBD0;
    u8* src = obj + 0xC;
    f32* out = tmp;
    for (s32 i = 1; i < *(s32*)(obj + 0x38) - 1; i++) {
        f32 mx = (*(f32*)(src + 0x40) + *(f32*)(src + 0x58)) * half;
        f32 my = (*(f32*)(src + 0x44) + *(f32*)(src + 0x5C)) * half;
        f32 mz = (*(f32*)(src + 0x48) + *(f32*)(src + 0x60)) * half;
        out[0] = *(f32*)(src + 0x4C) * inv + mx * t;
        out[1] = *(f32*)(src + 0x50) * inv + my * t;
        out[2] = *(f32*)(src + 0x54) * inv + mz * t;
        src += 0xC;
        out += 3;
    }
    f32* in = tmp;
    u8* dst = obj + 0xC;
    for (s32 i = 1; i < *(s32*)(obj + 0x38) - 1; i++) {
        f32 a = in[0];
        f32 b = in[1];
        f32 c = in[2];
        in += 3;
        *(f32*)(dst + 0x4C) = a;
        *(f32*)(dst + 0x50) = b;
        *(f32*)(dst + 0x54) = c;
        dst += 0xC;
    }
}

// 0x801C3F6C
// Spawns an effect node for def: optionally randomizes the def's colour (+4) with a random unit vector,
// then allocates from the pool at +0x7C (or +0x78 for plain nodes) and initialises flags.
void* fn_801C3F6C(u8* self, u8* def) {
    void* result = 0;
    if (def == 0) return 0;
    if (lbl_805FB6F8 != 0) {
        Vec3 r;
        r.x = lbl_805FFCB8 * (f32)RandomU32();
        r.y = lbl_805FFCB8 * (f32)RandomU32();
        r.z = lbl_805FFCB8 * (f32)RandomU32();
        Vec3 n = Vec3Normalized(r);
        s32 c[4];
        c[0] = (s32)(lbl_805FFCD0 * n.x);
        c[1] = (s32)(lbl_805FFCD0 * n.y);
        c[2] = (s32)(lbl_805FFCD0 * n.z);
        c[3] = (s32)(lbl_805FFCD0 * lbl_805FFCB4);
        for (s32 i = 0; i < 4; i++) {
            if (c[i] < 0) c[i] = 0;
            else if (c[i] > 0xFF) c[i] = 0xFF;
        }
        *(u32*)(def + 0x4) = (u32)c[0] | ((u32)c[1] << 8) | ((u32)c[2] << 16) | ((u32)c[3] << 24);
    }
    if (*(s32*)(def + 0x4C) != 0) {
        u8* node = (u8*)fn_801EB94C(*(void**)(def + 0x7C));
        result = node;
        if (node != 0) fn_801C5AAC(node, def);
    } else {
        u8* node = (u8*)fn_801EB94C(*(void**)(def + 0x78));
        result = node;
        if (node != 0) {
            *(u32*)(node + 0x4) = 0;
            *(u8**)(node + 0x0) = def;
            if (*(s32*)(def + 0x28) != 0) {
                *(u32*)(node + 0x4) |= 0x4;
            } else {
                *(u32*)(node + 0x4) &= ~0x4u;
            }
            u8* d = *(u8**)(node + 0x0);
            f32 a = *(f32*)(d + 0x40);
            f32 b = *(f32*)(d + 0x44);
            f32 e = *(f32*)(d + 0x48);
            if (e * e + (a * a + b * b) > lbl_805FFCD4) {
                *(u32*)(node + 0x4) |= 0x10;
            }
            if (fabsf(*(f32*)(*(u8**)(node + 0x0) + 0x3C)) > lbl_805FFCD8) {
                *(u32*)(node + 0x4) |= 0x20;
            }
        }
    }
    return result;
}

}  // extern "C"
