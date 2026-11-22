// Render-state property setter, 0x8017D854.
//
// fn_8017D854(state, id, value): "set property `id` of the render state block to `value`".
// This is not a GX drawing routine (no FIFO writes): it is one big switch over property ids that
// validates `value` and stores it, as an enum number / flag / float / colour byte, into an array
// of sub-records inside the state block. Property ids are grouped in ranges; each range holds
// `count` records of a fixed stride, with the record index and the field derived from the id:
//   index = (id - base) / fields, field = (id - base) % fields.
// A value that is out of range for its property is ignored (the store is skipped).
// Float values arrive as raw 32-bit patterns in `value`.
#include "types.h"

static inline f32 BitsToF32(s32 v) {
    union { s32 i; f32 f; } u;
    u.i = v;
    return u.f;
}

// fctiwz: truncate toward zero, saturating (NaN gives the minimum).
static inline s32 FloatToInt(f32 x) {
    if (x != x) return (s32)0x80000000;
    if (x >= 2147483648.0f) return 0x7FFFFFFF;
    if (x < -2147483648.0f) return (s32)0x80000000;
    return (s32)x;
}

static inline s32 ClampS32(s32 x, s32 lo, s32 hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline s32 NonZero(s32 v) { return v != 0; }

// Store `v` when 0 <= v <= max (signed compares, as the original does).
static inline void StoreIfBelow(s32* dst, s32 v, s32 max) {
    if (v >= 0 && v <= max) *dst = v;
}

// Colour word channel k (1 = top byte .. 4 = low byte) from a float in 0..1, scaled by 255 and clamped.
static void SetColorChannel(u32* color, s32 k, s32 v) {
    s32 b = ClampS32(FloatToInt(255.0f * BitsToF32(v)), 0, 255);
    u32 shift = 24 - 8 * (k - 1);
    *color = (*color & ~(0xFFu << shift)) | ((u32)b << shift);
}

// 0/1/2 -> 0/1/2, anything else ignored.
static inline void Store012(s32* dst, s32 v) { StoreIfBelow(dst, v, 2); }
// 0..3 -> 0..3, anything else ignored.
static inline void Store0123(s32* dst, s32 v) { StoreIfBelow(dst, v, 3); }

// Value remap tables that the original keeps as jump tables (index = incoming value).
static const s32 kMap_0_1_2_3_6_7_10_11_14_15[10] = {0, 1, 2, 3, 6, 7, 10, 11, 14, 15};
static const s32 kMap_0_1_2_3_5_6_7_9_10_11[10] = {0, 1, 2, 3, 5, 6, 7, 9, 10, 11};

extern "C" {

// 0x8017D854
void fn_8017D854(u8* st, s32 id, s32 v) {
    // ---- single properties 0x3a..0x3d ----
    if (id < 0x3f) {
        switch (id) {
        case 0x3a:
            if ((u32)v <= 2) *(s32*)(st + 0x4) = v;
            break;
        case 0x3b:
            if ((u32)v <= 8) *(s32*)(st + 0x8) = v;
            break;
        case 0x3c:
            if ((u32)v <= 0x10) *(s32*)(st + 0xc) = v;
            break;
        case 0x3d:
            if ((u32)v <= 4) *(s32*)(st + 0x10) = v;
            break;
        default:
            break;
        }
        return;
    }

    // ---- 0x3e..0x51: 4 colour slots (enable byte + RGBA word), 5 ids each ----
    if (id >= 0x3e && id < 0x52) {
        s32 q = (id - 0x3e) / 5;
        u8* flag;
        u32* color;
        switch (q) {
        case 1:  flag = st + 0x1c; color = (u32*)(st + 0x20); break;
        case 2:  flag = st + 0x5c; color = (u32*)(st + 0x60); break;
        case 3:  flag = st + 0x64; color = (u32*)(st + 0x68); break;
        default: flag = st + 0x14; color = (u32*)(st + 0x18); break;
        }
        s32 k = id - 5 * q - 0x3e;
        switch (k) {
        case 0: *flag = (u8)NonZero(v); break;
        case 1:
        case 2:
        case 3:
        case 4: SetColorChannel(color, k, v); break;  // R, G, B, A
        }
        return;
    }

    // ---- 0x52..0x69: 4 records at +0x24/+0x40/+0x6c/+0x88, 6 ids each ----
    if (id >= 0x52 && id < 0x6a) {
        s32 q = (id - 0x52) / 6;
        u8* r;
        switch (q) {
        case 1:  r = st + 0x40; break;
        case 2:  r = st + 0x6c; break;
        case 3:  r = st + 0x88; break;
        default: r = st + 0x24; break;
        }
        s32 k = id - 6 * q - 0x52;
        switch (k) {
        case 0: *(u8*)(r + 0x0) = (u8)NonZero(v); break;
        case 1:
            if (v == 0) *(s32*)(r + 0x4) = 0;
            else if (v == 1) *(s32*)(r + 0x4) = 1;
            break;
        case 2:
            if (v == 0) *(s32*)(r + 0x8) = 0;
            else if (v == 1) *(s32*)(r + 0x8) = 1;
            break;
        case 3:
            if (v == 0) *(s32*)(r + 0xc) = 0;
            else if (v == 1) *(s32*)(r + 0xc) = 2;
            else if (v == 2) *(s32*)(r + 0xc) = 1;
            break;
        case 4:
            if (v == 0) *(s32*)(r + 0x10) = 2;
            else if (v == 1) *(s32*)(r + 0x10) = 1;
            else if (v == 2) *(s32*)(r + 0x10) = 3;
            break;
        case 5: *(u8*)(r + 0x1) = (u8)v; break;
        }
        return;
    }

    // ---- 0x6a..0xb1: 8 records of 0x24 bytes starting at +0xa4, 9 ids each ----
    if (id >= 0x6a && id < 0xb2) {
        s32 q = (id - 0x6a) / 9;
        u8* r = st + q * 0x24;
        s32 k = id - 9 * q - 0x6a;
        switch (k) {
        case 0:
            if (v == 0) *(s32*)(r + 0xa4) = 0;
            else if (v == 1) *(s32*)(r + 0xa4) = 2;
            else if (v == 2) *(s32*)(r + 0xa4) = 1;
            break;
        case 1:
            if (v == 0) *(s32*)(r + 0xa8) = 0;
            else if (v == 1) *(s32*)(r + 0xa8) = 2;
            else if (v == 2) *(s32*)(r + 0xa8) = 1;
            break;
        case 2: *(s32*)(r + 0xac) = NonZero(v); break;
        case 3: *(s32*)(r + 0xb0) = NonZero(v); break;
        case 4:
            if (v == 0) *(s32*)(r + 0xb4) = 0;
            else if (v == 1) *(s32*)(r + 0xb4) = 1;
            break;
        case 5:  // signed byte from a float scaled by 32
            *(s8*)(r + 0xb8) = (s8)ClampS32(FloatToInt(32.0f * BitsToF32(v)), -0x80, 0x7f);
            break;
        case 6: Store012((s32*)(r + 0xbc), v); break;
        case 7: *(u8*)(r + 0xc0) = (u8)NonZero(v); break;
        case 8:
            if ((u32)v < 8) *(s32*)(r + 0xc4) = v;
            break;
        }
        return;
    }

    // ---- 0xb5..0x124: 8 records of 0x34 bytes starting at +0x1c4, 14 ids each ----
    if (id >= 0xb5 && id < 0x125) {
        s32 q = (id - 0xb5) / 14;
        u8* r = st + q * 0x34;
        s32 k = id - 14 * q - 0xb5;
        switch (k) {
        case 0:
            if (v == 0) {
                *(u8*)(r + 0x1c4) = 1;
                *(s32*)(r + 0x1cc) = 0;
            } else {
                *(u8*)(r + 0x1c4) = 0;
                *(s32*)(r + 0x1cc) = v - 1;
            }
            break;
        case 1: *(u16*)(r + 0x1c6) = (u16)v; break;
        case 2: *(u16*)(r + 0x1c8) = (u16)v; break;
        case 3:  // two flag bytes from a 0..3 bit pair
            if (v >= 0 && v <= 3) {
                *(u8*)(r + 0x1d0) = (u8)(v & 1);
                *(u8*)(r + 0x1d1) = (u8)(v >> 1);
            }
            break;
        case 4:
            if (v >= 0 && v <= 3) {
                *(u8*)(r + 0x1d2) = (u8)(v & 1);
                *(u8*)(r + 0x1d3) = (u8)(v >> 1);
            }
            break;
        case 5: Store0123((s32*)(r + 0x1d4), v); break;
        case 6:
            if ((u32)v <= 0xc) *(s32*)(r + 0x1d8) = v;
            break;
        case 7:
            if (v == 0) *(s32*)(r + 0x1dc) = 0;
            else if (v == 1) *(s32*)(r + 0x1dc) = 1;
            break;
        case 8:
            if (v == 0) *(s32*)(r + 0x1e0) = 0;
            else if (v == 1) *(s32*)(r + 0x1e0) = 1;
            break;
        case 9:
            if ((u32)v < 8) *(s32*)(r + 0x1e4) = v;
            break;
        case 10:
            if ((u32)v < 7) *(s32*)(r + 0x1e8) = v;
            break;
        case 11:
            // 0 -> 0, 1 / 10 / 11 -> 30, 2 -> 60; 3..9 and everything else ignored
            if (v == 0) *(s32*)(r + 0x1ec) = 0;
            else if (v == 2) *(s32*)(r + 0x1ec) = 0x3c;
            else if (v == 1 || v == 10 || v == 11) *(s32*)(r + 0x1ec) = 0x1e;
            break;
        case 12: *(u8*)(r + 0x1f0) = (u8)NonZero(v); break;
        case 13:
            if ((u32)v <= 0x14) *(s32*)(r + 0x1f4) = (v == 0x14) ? 0x3d : v * 3;
            break;
        }
        return;
    }

    // ---- 0x125..0x133: 3 records of 0x14 bytes at +0x364 (int + 4 floats), 5 ids each ----
    if (id >= 0x125 && id < 0x134) {
        s32 q = (id - 0x125) / 5;
        u8* r = st + q * 0x14;
        s32 k = id - 5 * q - 0x125;
        switch (k) {
        case 0: *(s32*)(r + 0x364) = v; break;
        case 1: *(f32*)(r + 0x368) = BitsToF32(v); break;
        case 2: *(f32*)(r + 0x36c) = BitsToF32(v); break;
        case 3: *(f32*)(r + 0x370) = BitsToF32(v); break;
        case 4: *(f32*)(r + 0x374) = BitsToF32(v); break;
        }
        return;
    }

    // ---- 0x134..0x147: 4 records of 0x14 bytes at +0x3a0 (int + 4 floats), 5 ids each ----
    if (id >= 0x134 && id < 0x148) {
        s32 q = (id - 0x134) / 5;
        u8* r = st + q * 0x14;
        s32 k = id - 5 * q - 0x134;
        switch (k) {
        case 0: *(s32*)(r + 0x3a0) = v; break;
        case 1: *(f32*)(r + 0x3a4) = BitsToF32(v); break;
        case 2: *(f32*)(r + 0x3a8) = BitsToF32(v); break;
        case 3: *(f32*)(r + 0x3ac) = BitsToF32(v); break;
        case 4: *(f32*)(r + 0x3b0) = BitsToF32(v); break;
        }
        return;
    }

    // ---- 0x148..0x157: 4 records of 0x10 bytes at +0x3f0, 4 ids each ----
    if (id >= 0x148 && id < 0x158) {
        s32 q = (id - 0x148) / 4;
        s32 k = id - 4 * q - 0x148;
        // value 0..3 is kept, anything else becomes 0
        s32 m = (v >= 0 && v <= 3) ? v : 0;
        *(s32*)(st + q * 0x10 + 0x3f0 + 4 * k) = m;
        return;
    }

    // ---- 0x158..0x2f7: 16 records of 0x68 bytes at +0x430, 26 ids each ----
    if (id >= 0x158 && id < 0x2f8) {
        s32 q = (id - 0x158) / 26;
        u8* r = st + q * 0x68;
        s32 k = id - 26 * q - 0x158;
        switch (k) {
        case 0: *(u8*)(r + 0x430) = (u8)NonZero(v); break;
        case 1:
            if ((u32)v < 8) *(s32*)(r + 0x434) = v;
            break;
        case 2:
            if ((u32)v < 8) *(s32*)(r + 0x438) = v;
            break;
        case 3:
            // 0 -> 0, 1 -> 1, 2 -> 5, 3 -> 6, 4 -> 7
            if (v == 0) *(s32*)(r + 0x43c) = 0;
            else if (v == 1) *(s32*)(r + 0x43c) = 1;
            else if (v == 2) *(s32*)(r + 0x43c) = 5;
            else if (v == 3) *(s32*)(r + 0x43c) = 6;
            else if (v == 4) *(s32*)(r + 0x43c) = 7;
            break;
        case 4:
            // 0..7 -> 0..7, 8..23 -> 16..31
            if ((u32)v <= 0x17) *(s32*)(r + 0x440) = (v < 8) ? v : v + 8;
            break;
        case 5:
            // 0..7 -> 0..7, 8..27 -> 12..31
            if ((u32)v <= 0x1b) *(s32*)(r + 0x444) = (v < 8) ? v : v + 4;
            break;
        case 6: Store0123((s32*)(r + 0x448), v); break;
        case 7: Store0123((s32*)(r + 0x44c), v); break;
        case 8:
            if ((u32)v <= 9) *(s32*)(r + 0x450) = kMap_0_1_2_3_6_7_10_11_14_15[v];
            break;
        case 9: Store0123((s32*)(r + 0x454), v); break;
        case 10:
        case 11:
        case 12:
        case 13: {
            // 4 ids share one 0..15 value (out of range reads as 0)
            s32 m = ((u32)v <= 0xf) ? v : 0;
            *(s32*)(r + 0x458 + 4 * (k - 10)) = m;
            break;
        }
        case 14: *(u8*)(r + 0x468) = (u8)NonZero(v); break;
        case 15: Store012((s32*)(r + 0x46c), v); break;
        case 16: Store0123((s32*)(r + 0x470), v); break;
        case 17:
            if ((u32)v <= 9) *(s32*)(r + 0x474) = kMap_0_1_2_3_6_7_10_11_14_15[v];
            break;
        case 18: Store0123((s32*)(r + 0x478), v); break;
        case 19:
        case 20:
        case 21:
        case 22: {
            // 4 ids share one 0..7 value (out of range reads as 0)
            s32 m = ((u32)v <= 7) ? v : 0;
            *(s32*)(r + 0x47c + 4 * (k - 19)) = m;
            break;
        }
        case 23: *(u8*)(r + 0x48c) = (u8)NonZero(v); break;
        case 24: Store012((s32*)(r + 0x490), v); break;
        case 25: Store0123((s32*)(r + 0x494), v); break;
        }
        return;
    }

    // ---- 0x2f8..0x307: 4 records of 0x10 bytes at +0xab0, 4 ids each ----
    if (id >= 0x2f8 && id < 0x308) {
        s32 q = (id - 0x2f8) / 4;
        u8* r = st + q * 0x10;
        s32 k = id - 4 * q - 0x2f8;
        switch (k) {
        case 0:
            if (v >= 0 && v < 8) *(s32*)(r + 0xab0) = v;
            break;
        case 1:
            if (v >= 0 && v < 8) *(s32*)(r + 0xab4) = v;
            break;
        case 2:
            if ((u32)v <= 8) *(s32*)(r + 0xab8) = v;
            break;
        case 3:
            if ((u32)v <= 8) *(s32*)(r + 0xabc) = v;
            break;
        }
        return;
    }

    // ---- 0x308..0x397: 16 records of 0x20 bytes at +0xaf0, 9 ids each ----
    if (id >= 0x308 && id < 0x398) {
        s32 q = (id - 0x308) / 9;
        u8* r = st + q * 0x20;
        s32 k = id - 9 * q - 0x308;
        switch (k) {
        case 0: Store0123((s32*)(r + 0xaf0), v); break;
        case 1: Store0123((s32*)(r + 0xaf4), v); break;
        case 2:
            if ((u32)v <= 7) *(s32*)(r + 0xaf8) = v;
            break;
        case 3:
            if ((u32)v <= 9) *(s32*)(r + 0xafc) = kMap_0_1_2_3_5_6_7_9_10_11[v];
            break;
        case 4:
            if ((u32)v <= 6) *(s32*)(r + 0xb00) = v;
            break;
        case 5:
            if ((u32)v <= 6) *(s32*)(r + 0xb04) = v;
            break;
        case 6: *(u8*)(r + 0xb08) = (u8)NonZero(v); break;
        case 7: *(u8*)(r + 0xb09) = (u8)NonZero(v); break;
        case 8: Store0123((s32*)(r + 0xb0c), v); break;
        }
        return;
    }

    // ---- the remaining single properties ----
    switch (id) {
    case 0xb2: {  // flag word bit 0 = (v == 1)
        u32 w = *(u32*)(st + 0xcf4);
        *(u32*)(st + 0xcf4) = (w & ~1u) | (u32)(v == 1);
        break;
    }
    case 0xb3: {  // flag word bit 1 = (v == 0)
        u32 w = *(u32*)(st + 0xcf4);
        *(u32*)(st + 0xcf4) = (w & ~2u) | ((v == 0) ? 2u : 0u);
        break;
    }
    case 0xb4: {  // flag word bit 2 = (v != 0)
        u32 w = *(u32*)(st + 0xcf4);
        *(u32*)(st + 0xcf4) = (w & ~4u) | ((v != 0) ? 4u : 0u);
        break;
    }
    case 0x398:
        *(f32*)(st + 0xcf0) = BitsToF32(v);
        break;
    case 0x399: {
        // stores the raw value, then sets mode byte 0xd of the block pointed to by *st
        // (2 -> 2, 3 -> 3, anything else -> 1) and raises its dirty byte 0xe
        *(s32*)(st + 0xcf8) = v;
        u8* blk = *(u8**)st;
        if (v == 2) blk[0xd] = 2;
        else if (v == 3) blk[0xd] = 3;
        else blk[0xd] = 1;
        (*(u8**)st)[0xe] = 1;
        break;
    }
    case 0x39a:
        if (v == 0) {
            *(u8*)(st + 0xcfc) = 0;
            *(u8*)(st + 0xcfd) = 0;
            *(s32*)(st + 0xd00) = 0;
        } else if (v == 1) {
            *(u8*)(st + 0xcfc) = 1;
            *(u8*)(st + 0xcfd) = 0;
            *(s32*)(st + 0xd00) = 0;
        } else if (v == 2) {
            *(u8*)(st + 0xcfc) = 0;
            *(u8*)(st + 0xcfd) = 1;
            *(s32*)(st + 0xd00) = 0;
        } else if (v == 3) {
            *(u8*)(st + 0xcfc) = 1;
            *(u8*)(st + 0xcfd) = 0;
            *(s32*)(st + 0xd00) = 1;
        }
        break;
    case 0x39b:
        if ((u32)v <= 7) *(s32*)(st + 0xd08) = v;
        break;
    case 0x39c:
        if ((u32)v <= 7) *(s32*)(st + 0xd0c) = v;
        break;
    case 0x39d:
        if ((u32)v <= 0xf) *(s32*)(st + 0xd04) = v;
        break;
    case 0x39e: *(u8*)(st + 0xd10) = (u8)NonZero(v); break;
    case 0x39f: *(u8*)(st + 0xd11) = (u8)NonZero(v); break;
    case 0x3a0:
        if ((u32)v <= 7) {
            *(u8*)(st + 0xd12) = (u8)(v != 7);  // 7 clears the enable byte
            *(s32*)(st + 0xd14) = v;
        }
        break;
    case 0x3a1: Store012((s32*)(st + 0xd18), v); break;
    case 0x3a2: *(u8*)(st + 0xd1c) = (u8)NonZero(v); break;
    default:
        break;
    }
}

}  // extern "C"
