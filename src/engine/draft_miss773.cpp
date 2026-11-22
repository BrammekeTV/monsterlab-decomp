// Script property getter: maps a property id to the value stored in the object's records.
#include "types.h"
#include <string.h>

// Helpers (not in the original: the asm has one big switch ladder with jump tables).
// Raw f32 bits as returned in r3 (original does lfs/stfs/lwz through the stack).
static inline u32 FBits(f32 f) {
    u32 u;
    memcpy(&u, &f, sizeof(u));
    return u;
}
// Identity map for 0..n-1, anything else yields 0.
static inline u32 IdN(s32 v, s32 n) { return (v >= 0 && v < n) ? (u32)v : 0u; }
// 0 -> 0, 1 -> 2, 2 -> 1, anything else 0.
static inline u32 Swap12(s32 v) { return v == 1 ? 2u : (v == 2 ? 1u : 0u); }
static inline u32 IsOne(s32 v) { return v == 1 ? 1u : 0u; }
// Sparse 16-entry map: 0..3 identity, 6->4, 7->5, 10->6, 11->7, 14->8, 15->9, rest 0.
static inline u32 Map16(u32 v) {
    switch (v) {
        case 0: case 1: case 2: case 3: return v;
        case 6: return 4;
        case 7: return 5;
        case 10: return 6;
        case 11: return 7;
        case 14: return 8;
        case 15: return 9;
        default: return 0;
    }
}

// Byte (as f32 colour channel) scaled by 1/255.
static inline u32 ColorChan(u32 w, s32 shift) {
    f32 k = 0.003921569f;
    f32 b = (f32)(s32)((w >> shift) & 0xff);
    return FBits(k * b);
}

extern "C" {

// 0x80180100
// Reads script property `id` from the object: record tables with fixed strides, with the stored
// engine values mapped back to script enum values. Float results come back as raw f32 bits.
u32 fn_80180100(u8* obj, s32 id) {
    if (id < 0x3f) {
        switch (id) {
            case 0x39: return 1;
            case 0x3a: return *(u32*)(obj + 0x4);
            case 0x3b: return *(u32*)(obj + 0x8);
            case 0x3c: return *(u32*)(obj + 0xc);
            case 0x3d: return *(u32*)(obj + 0x10);
            default: return 0;
        }
    }

    if (id >= 0x3e && id < 0x52) {
        // 4 records of 5 properties: a flag byte and four colour channels.
        s32 idx = (id - 0x3e) / 5;
        s32 sub = id - idx * 5 + 0x3e - 0x3e;  // sub = id - 5*idx (0x3e..0x42)
        u8* bytePtr;
        u8* colPtr;
        switch (idx) {
            case 1: bytePtr = obj + 0x1c; colPtr = obj + 0x20; break;
            case 2: bytePtr = obj + 0x5c; colPtr = obj + 0x60; break;
            case 3: bytePtr = obj + 0x64; colPtr = obj + 0x68; break;
            default: bytePtr = obj + 0x14; colPtr = obj + 0x18; break;
        }
        switch (sub) {
            case 0x3e: return *(u8*)bytePtr;
            case 0x3f: return ColorChan(*(u32*)colPtr, 24);
            case 0x40: return ColorChan(*(u32*)colPtr, 16);
            case 0x41: return ColorChan(*(u32*)colPtr, 8);
            case 0x42: return ColorChan(*(u32*)colPtr, 0);
            default: return 0;
        }
    }

    if (id >= 0x52 && id < 0x6a) {
        // 4 records of 6 properties.
        s32 idx = (id - 0x52) / 6;
        s32 sub = id - idx * 6;
        u8* r;
        switch (idx) {
            case 1: r = obj + 0x40; break;
            case 2: r = obj + 0x6c; break;
            case 3: r = obj + 0x88; break;
            default: r = obj + 0x24; break;
        }
        switch (sub) {
            case 0x52: return *(u8*)r;
            case 0x53: return IsOne(*(s32*)(r + 0x4));
            case 0x54: return IsOne(*(s32*)(r + 0x8));
            case 0x55: {
                s32 v = *(s32*)(r + 0xc);
                return v == 1 ? 2u : (v == 2 ? 1u : 0u);
            }
            case 0x56: {
                s32 v = *(s32*)(r + 0x10);
                return v == 1 ? 1u : (v == 3 ? 2u : 0u);
            }
            case 0x57: return *(u8*)(r + 1);
            default: return 0;
        }
    }

    if (id >= 0x6a && id < 0xb2) {
        // 8 records (stride 0x24) of 9 properties.
        s32 idx = (id - 0x6a) / 9;
        s32 sub = (id - 0x6a) % 9;
        u8* r = obj + idx * 0x24;
        switch (sub) {
            case 0: return Swap12(*(s32*)(r + 0xa4));
            case 1: return Swap12(*(s32*)(r + 0xa8));
            case 2: return IsOne(*(s32*)(r + 0xac));
            case 3: return IsOne(*(s32*)(r + 0xb0));
            case 4: return IsOne(*(s32*)(r + 0xb4));
            case 5: return FBits(0.03125f * (f32)(s32) * (s8*)(r + 0xb8));
            case 6: return IdN(*(s32*)(r + 0xbc), 3);
            case 7: return *(u8*)(r + 0xc0);
            case 8: return *(u32*)(r + 0xc4);
            default: return 0;
        }
    }

    if (id >= 0xb5 && id < 0x125) {
        // 8 records (stride 0x34) of 14 properties.
        s32 idx = (id - 0xb5) / 14;
        s32 sub = (id - 0xb5) % 14;
        u8* r = obj + idx * 0x34;
        switch (sub) {
            case 0:
                if (*(u8*)(r + 0x1c4) != 0) return 0;
                return (u32)(*(s32*)(r + 0x1cc) + 1);
            case 1: return *(u16*)(r + 0x1c6);
            case 2: return *(u16*)(r + 0x1c8);
            case 3: return (*(u8*)(r + 0x1d0) ? 1u : 0u) | (*(u8*)(r + 0x1d1) ? 2u : 0u);
            case 4: return (*(u8*)(r + 0x1d2) ? 1u : 0u) | (*(u8*)(r + 0x1d3) ? 2u : 0u);
            case 5: return IdN(*(s32*)(r + 0x1d4), 4);
            case 6: return IdN(*(s32*)(r + 0x1d8), 13);
            case 7: return IsOne(*(s32*)(r + 0x1dc));
            case 8: return IsOne(*(s32*)(r + 0x1e0));
            case 9: return *(u32*)(r + 0x1e4);
            case 10: return *(u32*)(r + 0x1e8);
            case 11: {
                s32 v = *(s32*)(r + 0x1ec);
                return v == 0x1e ? 1u : (v == 0x3c ? 2u : 0u);
            }
            case 12: return *(u8*)(r + 0x1f0);
            case 13: {
                u32 v = *(u32*)(r + 0x1f4);
                if (v > 0x3d) return 0;
                if (v == 0x3d) return 20;
                return (v % 3 == 0) ? v / 3 : 0;
            }
            default: return 0;
        }
    }

    if (id >= 0x125 && id < 0x134) {
        // 3 records (stride 0x14) of 5 properties.
        s32 idx = (id - 0x125) / 5;
        s32 sub = (id - 0x125) % 5;
        u8* r = obj + idx * 0x14;
        switch (sub) {
            case 0: return *(u32*)(r + 0x364);
            case 1: return FBits(*(f32*)(r + 0x368));
            case 2: return FBits(*(f32*)(r + 0x36c));
            case 3: return FBits(*(f32*)(r + 0x370));
            case 4: return FBits(*(f32*)(r + 0x374));
            default: return 0;
        }
    }

    if (id >= 0x134 && id < 0x148) {
        // 4 records (stride 0x14) of 5 properties.
        s32 idx = (id - 0x134) / 5;
        s32 sub = (id - 0x134) % 5;
        u8* r = obj + idx * 0x14;
        switch (sub) {
            case 0: return *(u32*)(r + 0x3a0);
            case 1: return FBits(*(f32*)(r + 0x3a4));
            case 2: return FBits(*(f32*)(r + 0x3a8));
            case 3: return FBits(*(f32*)(r + 0x3ac));
            case 4: return FBits(*(f32*)(r + 0x3b0));
            default: return 0;
        }
    }

    if (id >= 0x148 && id < 0x158) {
        // 4 records (stride 0x10) of 4 properties, all enum-mapped (identity 0..3).
        s32 idx = (id - 0x148) / 4;
        s32 sub = (id - 0x148) % 4;
        return IdN(*(s32*)(obj + idx * 0x10 + 0x3f0 + sub * 4), 4);
    }

    if (id >= 0x158 && id < 0x2f8) {
        // 16 records (stride 0x68) of 26 properties.
        s32 idx = (id - 0x158) / 26;
        s32 sub = (id - 0x158) % 26;
        u8* r = obj + idx * 0x68;
        switch (sub) {
            case 0: return *(u8*)(r + 0x430);
            case 1: return *(u32*)(r + 0x434);
            case 2: return *(u32*)(r + 0x438);
            case 3: {
                s32 v = *(s32*)(r + 0x43c);
                return v == 1 ? 1u : (v == 5 ? 2u : (v == 6 ? 3u : (v == 7 ? 4u : 0u)));
            }
            case 4: {
                u32 v = *(u32*)(r + 0x440);
                if (v < 8) return v;
                if (v >= 16 && v < 32) return v - 8;
                return 0;
            }
            case 5: {
                u32 v = *(u32*)(r + 0x444);
                if (v < 8) return v;
                if (v >= 12 && v < 32) return v - 4;
                return 0;
            }
            case 6: return IdN(*(s32*)(r + 0x448), 4);
            case 7: return IdN(*(s32*)(r + 0x44c), 4);
            case 8: return Map16(*(u32*)(r + 0x450));
            case 9: return IdN(*(s32*)(r + 0x454), 4);
            case 10: case 11: case 12: case 13: {
                // four consecutive words, one per property (0x162..0x165)
                u32 v = *(u32*)(r + 0x458 + (sub - 10) * 4);
                return v <= 0xf ? v : 0;
            }
            case 14: return *(u8*)(r + 0x468);
            case 15: return IdN(*(s32*)(r + 0x46c), 3);
            case 16: return IdN(*(s32*)(r + 0x470), 4);
            case 17: return Map16(*(u32*)(r + 0x474));
            case 18: return IdN(*(s32*)(r + 0x478), 4);
            case 19: case 20: case 21: case 22: {
                u32 v = *(u32*)(r + 0x47c + (sub - 19) * 4);
                return v <= 7 ? v : 0;
            }
            case 23: return *(u8*)(r + 0x48c);
            case 24: return IdN(*(s32*)(r + 0x490), 3);
            case 25: return IdN(*(s32*)(r + 0x494), 4);
            default: return 0;
        }
    }

    if (id >= 0x2f8 && id < 0x308) {
        // 4 records (stride 0x10) of 4 properties.
        s32 idx = (id - 0x2f8) / 4;
        s32 sub = (id - 0x2f8) % 4;
        u8* r = obj + idx * 0x10;
        switch (sub) {
            case 0: return *(u32*)(r + 0xab0);
            case 1: return *(u32*)(r + 0xab4);
            case 2: return IdN(*(s32*)(r + 0xab8), 9);
            case 3: return IdN(*(s32*)(r + 0xabc), 9);
            default: return 0;
        }
    }

    if (id >= 0x308 && id < 0x398) {
        // 16 records (stride 0x20) of 9 properties.
        s32 idx = (id - 0x308) / 9;
        s32 sub = (id - 0x308) % 9;
        u8* r = obj + idx * 0x20;
        switch (sub) {
            case 0: return IdN(*(s32*)(r + 0xaf0), 4);
            case 1: return IdN(*(s32*)(r + 0xaf4), 4);
            case 2: return IdN(*(s32*)(r + 0xaf8), 8);
            case 3: {
                u32 v = *(u32*)(r + 0xafc);
                switch (v) {
                    case 0: case 1: case 2: case 3: return v;
                    case 5: return 4;
                    case 6: return 5;
                    case 7: return 6;
                    case 9: return 7;
                    case 10: return 8;
                    case 11: return 9;
                    default: return 0;
                }
            }
            case 4: return IdN(*(s32*)(r + 0xb00), 7);
            case 5: return IdN(*(s32*)(r + 0xb04), 7);
            case 6: return *(u8*)(r + 0xb08);
            case 7: return *(u8*)(r + 0xb09);
            case 8: return IdN(*(s32*)(r + 0xb0c), 4);
            default: return 0;
        }
    }

    // Single properties.
    switch (id) {
        case 0xb2: return *(u32*)(obj + 0xcf4) & 1;
        case 0xb3: return (*(u32*)(obj + 0xcf4) & 2) == 0 ? 1u : 0u;
        case 0xb4: return (*(u32*)(obj + 0xcf4) >> 2) & 1;
        case 0x398: return FBits(*(f32*)(obj + 0xcf0));
        case 0x399: return *(u32*)(obj + 0xcf8);
        case 0x39a: {
            u8 b0 = *(u8*)(obj + 0xcfc);
            u8 b1 = *(u8*)(obj + 0xcfd);
            if (b0 == 0 && b1 == 0) return 0;
            if (b0 != 0 && b1 == 0 && *(s32*)(obj + 0xd00) == 0) return 1;
            if (b0 == 0 && b1 != 0) return 2;
            if (b0 != 0 && b1 == 0 && *(s32*)(obj + 0xd00) == 1) return 3;
            return 0;
        }
        case 0x39b: return IdN(*(s32*)(obj + 0xd08), 8);
        case 0x39c: return IdN(*(s32*)(obj + 0xd0c), 8);
        case 0x39d: return IdN(*(s32*)(obj + 0xd04), 16);
        case 0x39e: return *(u8*)(obj + 0xd10);
        case 0x39f: return *(u8*)(obj + 0xd11);
        case 0x3a0: return IdN(*(s32*)(obj + 0xd14), 8);
        case 0x3a1: return IdN(*(s32*)(obj + 0xd18), 3);
        case 0x3a2: return *(u8*)(obj + 0xd1c);
        default: return 0;
    }
}

}  // extern "C"
