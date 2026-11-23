// Small functions from the missing-symbol batch 161: GX FIFO packing helpers, UI cursor and
// timer helpers, list comparators, a pad-button mask table lookup, a ring-buffer push and a
// few flag tests.
#include "types.h"
#include "revolution.h"

// fn_80198DE4 is defined in src/engine/small_8019.cpp with C++ linkage.
extern "C++" BOOL fn_80198DE4(void* p);

extern "C" {
u32     PadGetHeld(s32 chan);                                   // 0x801B9B98
u32     fn_801B9B6C(s32 chan);                                  // 0x801B9B6C
void    fn_8018D2B8(const char* fmt, ...);                      // 0x8018D2B8 (debug log)
void    fn_8018D370(const char* fmt, ...);                      // 0x8018D370 (debug log)
void*   UIEngineAlloc(s32 size, const char* file, void* tag, s32 line);  // 0x8018D428
s32 fn_8018D534(void* obj);                                      // 0x8018D534 (callback, address only)
s32 UIScreen_AddDeferredRender(u8* obj, u8* list);                                      // 0x80199CE8 (callback, address only)
s32 fn_80196678(void* gx, void* mat, f32 t);                  // 0x80196678
s32     fn_80196B10(u8* a3);                                    // 0x80196B10
void    fn_8019BFF8(void* obj, void* cb, void* arg);            // 0x8019BFF8
void    fn_8019C348(void* obj);                                 // 0x8019C348
s32     UIHashTree_InsertNode(u8* list, u8* node);                        // 0x8018E0CC
void    fn_80198350(s32 a, u32 held, u8* p);                    // 0x80198350
void    fn_801BBB68(void* mgr, s32 id, s32 a, s32 b);           // 0x801BBB68

extern void* lbl_805FB658;              // 0x805FB658 global object pointer
extern char  lbl_80381410[];            // 0x80381410 debug message block
extern char  lbl_803813A8[];            // 0x803813A8 debug message
extern s32   lbl_805F9370;              // 0x805F9370 allocation tag
extern f32   lbl_805FF894;              // 0x805FF894 lower clamp bound
extern f32   lbl_805FF898;              // 0x805FF898 upper clamp bound
extern f32   lbl_805FF918;              // 0x805FF918 zero constant
extern u32   lbl_80413430[];            // 0x80413430 pad button mask table
}

// Hashes of the 0x8008D6BC state-selector switch.
static const u32 kHashCastleGoodBallA   = 0x67B779E5u;  // "castle_good_ballA"
static const u32 kHashCastleFireBallA   = 0xBAA6441Fu;  // "castle_fire_ballA"
static const u32 kHashCastleWaterBallA  = 0x8F25BB8Au;  // "castle_water_ballA"
static const u32 kHashCastleEarthSymbol = 0xB57069FEu;  // "castle_earth_symbolA"
static const u32 kHashCastleAirSymbol   = 0xA65DCF50u;  // "castle_air_symbolA"
static const u32 kHashCastleBadSymbol   = 0xF3B2D25Eu;  // "castle_bad_symbolA"

// Vtable slot 0x2C of the object at +0x138: (self, mode, a, b, c) -> f32.
typedef f32 (*VFnPos)(u8* self, s32 mode, f32 a, f32 b, f32 c);

extern "C" {

// 0x8008D6BC
// Stores the state selector at +0x304 and sets the state pair at +0x268/+0x270 (and the byte
// flag at +0x274 for one selector). Unknown selectors only store the selector.
void fn_8008D6BC(u8* arg0, u32 arg1) {
    *(u32*)(arg0 + 0x304) = arg1;
    if (arg1 == kHashCastleGoodBallA) {
        *(s32*)(arg0 + 0x268) = 2;
        *(s32*)(arg0 + 0x270) = 0;
    } else if (arg1 == kHashCastleFireBallA) {
        *(s32*)(arg0 + 0x268) = 3;
        *(s32*)(arg0 + 0x270) = 0;
    } else if (arg1 == kHashCastleWaterBallA) {
        *(s32*)(arg0 + 0x268) = 4;
        *(s32*)(arg0 + 0x270) = 0;
    } else if (arg1 == kHashCastleEarthSymbol) {
        *(s32*)(arg0 + 0x268) = 0;
        *(s32*)(arg0 + 0x270) = 0;
    } else if (arg1 == kHashCastleAirSymbol) {
        *(s32*)(arg0 + 0x268) = 1;
        *(s32*)(arg0 + 0x270) = 0;
        *(u8*)(arg0 + 0x274) = 1;
    } else if (arg1 == kHashCastleBadSymbol) {
        *(s32*)(arg0 + 0x268) = 0;
        *(s32*)(arg0 + 0x270) = 1;
    }
}

// 0x80158C94
// Packs one 32-bit GX FIFO word from the eleven inputs and writes it with a 0x61 command byte.
// When bit 1 of arg7 is set, the packing uses the alternate field layout (bit 18 from arg7 bit 0,
// bits 20..31 from arg7 bits 2..13, and bit 19 of arg10 masked off bit 18).
void fn_80158C94(s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8, s32 arg9, s32 arg10, s32 arg11) {
    u32 val;

    (void)arg1;
    if (arg7 & 2) {
        val = (((u32)arg10 << 19) & ~0x40000u) | (((u32)arg7 & 1u) << 18) | 0x30000u;
        val |= (u32)arg3 << 12;
        val |= (u32)arg4 << 8;
        val |= (u32)arg5 << 4;
        val |= (u32)arg6;
        val |= (((u32)arg7 >> 2) & 0xFFFu) << 20;
        val |= (u32)arg11 << 22;
        val |= (((u32)arg2 << 1) + 0xC0u) << 24;
    } else {
        val = (u32)arg6;
        val |= (u32)arg5 << 4;
        val |= (u32)arg4 << 8;
        val |= (u32)arg3 << 12;
        val |= (u32)arg8 << 16;
        val |= (u32)arg7 << 18;
        val |= (u32)arg10 << 19;
        val |= (u32)arg9 << 20;
        val |= (u32)arg11 << 22;
        val |= (((u32)arg2 << 1) + 0xC0u) << 24;
    }
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = val;
}

// 0x80158F04
// Writes three GX FIFO words (each with a 0x61 command byte) built from the three 32-bit fields at
// arg2+0x0/0x4/0x8 and the matching shifted fields at +0xC/+0x10/+0x14, with the base index arg1+6..8
// in the top byte and two bits of arg3+0x11 in each word.
void fn_80158F04(u32 arg0, s32 arg1, u8* arg2, u32 arg3) {
    u32 t = arg3 + 0x11u;

    (void)arg0;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = *(u32*)(arg2 + 0x0) | (*(u32*)(arg2 + 0xC) << 0xB)
                  | ((t << 0x16) & 0xC00000u) | ((u32)(arg1 + 6) << 0x18);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = *(u32*)(arg2 + 0x4) | (*(u32*)(arg2 + 0x10) << 0xB)
                  | ((u32)(arg1 + 7) << 0x18) | (((t >> 2) & 3u) << 0x16);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = *(u32*)(arg2 + 0x8) | (*(u32*)(arg2 + 0x14) << 0xB)
                  | ((u32)(arg1 + 8) << 0x18) | (((t >> 4) & 3u) << 0x16);
}

// 0x8018CA04
// Calls vtable slot 0x18 of the object at +0x138 with self and the float *arg1.
s32 fn_8018CA04(u8* arg0, f32* arg1) {
    (*(void (**)(u8*, f32))(*(u8**)(arg0 + 0x138) + 0x18))(arg0, *arg1);
    return 0;
}

// 0x8018D558
// Sets the cursor position on the object at +0x7C (x, y, z stored at +0x68/+0x6C/+0x70), then
// refreshes it through the object's vtable slot 0x2C and the GX helpers. Positions outside the
// clamp bounds are ignored.
void UIEngine_SetCursorPos(u8* arg0, f32 x, f32 y, f32 z) {
    u8* obj;
    VFnPos vfn;
    f32 t;

    if (x < lbl_805FF894) return;
    if (y < lbl_805FF894) return;
    if (x > lbl_805FF898) return;
    if (y > lbl_805FF898) return;

    obj = *(u8**)(arg0 + 0x7C);
    if (obj == NULL) {
        fn_8018D2B8(lbl_803813A8);
        return;
    }
    *(f32*)(obj + 0x68) = x;
    *(f32*)(obj + 0x6C) = y;
    *(f32*)(obj + 0x70) = z;
    obj = *(u8**)(arg0 + 0x7C);
    vfn = *(VFnPos*)(*(u8**)(obj + 0x138) + 0x2C);
    t = vfn(obj, 0x30, y, y, z);
    obj = *(u8**)(arg0 + 0x7C);
    if (*(s32*)(obj + 0x60) == 0) {
        fn_80196678(lbl_805FB658, obj + 0xF0, t);
        fn_8019BFF8(*(void**)(arg0 + 0x7C), (void*)fn_8018D534, NULL);
        fn_80196B10((u8*)lbl_805FB658);
    }
    fn_8019C348(*(void**)(arg0 + 0x7C));
}

// 0x8018E208
// Allocates a 0x14-byte node, stores arg1 and arg2 in it and links it into the list at arg0.
// Returns -2 for a NULL list, -3 when the allocation fails, otherwise the link result.
s32 fn_8018E208(u8* arg0, u32 arg1, u8* arg2) {
    u8* node;

    if (arg0 == NULL) {
        fn_8018D370(lbl_80381410 + 0x90, arg1);
        return -2;
    }
    node = (u8*)UIEngineAlloc(0x14, lbl_80381410, &lbl_805F9370, 0x93);
    if (node == NULL) {
        fn_8018D370(lbl_80381410 + 0xC0);
    } else {
        *(u32*)(node + 0x0) = 0xDEADBEEFu;
        *(u32*)(node + 0x4) = 0;
        *(u32*)(node + 0x8) = 0;
        *(u32*)(node + 0xC) = 0;
        *(u32*)(node + 0x10) = 0;
    }
    if (node == NULL) {
        fn_8018D370(lbl_80381410 + 0xEC);
        return -3;
    }
    *(u32*)(node + 0x0) = arg1;
    *(void**)(node + 0x4) = arg2;
    return UIHashTree_InsertNode(arg0, node);
}

// 0x801981FC
// Advances the timer at +0x18 by arg1 while the limit at +0x1C is positive. When the timer reaches
// the limit, or +0xC is zero, the timer and +0x14 are reset. Then polls the pad (channel 0) into the
// input block at +0xC. Returns 0.
s32 fn_801981FC(u8* arg0, f32 arg1) {
    f32 limit;
    f32 t;

    *(s32*)(arg0 + 0x10) = *(s32*)(arg0 + 0xC);
    *(s32*)(arg0 + 0x14) = *(s32*)(arg0 + 0xC);
    limit = *(f32*)(arg0 + 0x1C);
    if (limit > lbl_805FF918) {
        t = *(f32*)(arg0 + 0x18) + arg1;
        *(f32*)(arg0 + 0x18) = t;
        if (t >= limit || *(s32*)(arg0 + 0xC) == 0) {
            *(f32*)(arg0 + 0x18) = lbl_805FF918;
            *(s32*)(arg0 + 0x14) = 0;
        }
    }
    fn_80198350(0, PadGetHeld(0), arg0 + 0xC);
    return 0;
}

// 0x80199540
// Qsort-style comparator on the float at +0x5C of the two objects: -1 when the first is larger,
// otherwise 1 when it is smaller and 0 when equal.
s32 fn_80199540(u8** arg0, u8** arg1) {
    f32 a = *(f32*)(*arg0 + 0x5C);
    f32 b = *(f32*)(*arg1 + 0x5C);

    if (a > b) {
        return -1;
    }
    return a < b;
}

// 0x801999D4
// Checks the flags at +0x64 against fn_80198DE4 and the event kind arg1. When the event matches
// and flag bit 1 is set with a positive count at +0x13C, queues a callback through fn_8019BFF8.
s32 fn_801999D4(u8* arg0, s32 arg1, u8* arg2) {
    BOOL r3;
    s32 flags;
    s32 a;
    s32 b;
    s32 ok;

    r3 = fn_80198DE4(arg0);
    flags = *(s32*)(arg0 + 0x64);
    a = 0;
    if ((flags & 0x10000) && r3 == 0) {
        a = 1;
    }
    b = 0;
    if (a == 0 && r3 == 0) {
        b = 1;
    }
    ok = ((a == 1 && arg1 == 0) || (b == 1 && arg1 == 1) || (r3 == 1 && arg1 == 2));
    if (ok && (flags & 2) && *(s32*)(arg0 + 0x13C) > 0) {
        fn_8019BFF8(arg0, (void*)UIScreen_AddDeferredRender, arg2);
    }
    return 0;
}

// 0x801B9B6C
// Returns the pad button mask for channel arg0 (0..3) from the table at lbl_80413430, else 0.
u32 fn_801B9B6C(s32 arg0) {
    if (arg0 < 0 || arg0 >= 4) {
        return 0;
    }
    return lbl_80413430[arg0];
}

// 0x801B9DD0
// Drains a 0x40-entry ring of 0x10-byte records at +0x81C: while the read index differs from the
// write index at +0x824, copies the record into the tail slot at +0x8, advances both indices and
// bumps the head at +0x4 when the tail catches it. Clears +0xC28 first.
void fn_801B9DD0(u8* arg0) {
    u32 idx;

    idx = *(u32*)(arg0 + 0x81C);
    *(s32*)(arg0 + 0xC28) = 0;
    while (idx != *(u32*)(arg0 + 0x824)) {
        u8* src = arg0 + 0x81C + idx * 0x10;
        u8* dst;
        u8* dst2;
        u32 tail;

        idx += 1;
        if (idx == 0x40) {
            idx = 0;
        }
        tail = *(u32*)(arg0 + 0x8);
        dst = arg0 + tail * 0x10;
        tail += 1;
        *(u32*)(arg0 + 0x8) = tail;
        if (tail == 0x40) {
            *(u32*)(arg0 + 0x8) = 0;
        }
        if (*(u32*)(arg0 + 0x8) == *(u32*)(arg0 + 0x4)) {
            u32 head = *(u32*)(arg0 + 0x4) + 1;
            *(u32*)(arg0 + 0x4) = head;
            if (head == 0x40) {
                *(u32*)(arg0 + 0x4) = 0;
            }
        }
        dst2 = dst + 0x10;
        if (dst2 != NULL) {
            *(u32*)(dst2 + 0x0) = *(u32*)(src + 0xC);
            *(u32*)(dst2 + 0x4) = *(u32*)(src + 0x10);
            *(u32*)(dst2 + 0x8) = *(u32*)(src + 0x14);
            *(u32*)(dst2 + 0xC) = *(u32*)(src + 0x18);
        }
    }
}

// 0x801B9F14
// Returns 1 when the object at +0x4 may be used for arg1. Sets flag 0x200 on the first match and
// returns 1. Otherwise, when the pointer at +0xA4 holds a non-zero word, returns the inverse of the
// active flag, or 1 when the flag 0x100 or 0x400 is set. Returns 1 otherwise.
s32 fn_801B9F14(u8* arg0, u32 arg1) {
    u8* obj;
    s32 flags;
    s32 r7;
    s32* p;

    if (arg0 == NULL) {
        return 1;
    }
    obj = *(u8**)(arg0 + 0x4);
    flags = *(s32*)(obj + 0x20);
    r7 = flags & 0x100;
    if (r7 != 0 && !(flags & 0x200) && arg1 == *(u32*)(arg0 + 0xEC)) {
        *(s32*)(obj + 0x20) = flags | 0x200;
        return 1;
    }
    p = *(s32**)(arg0 + 0xA4);
    if (p != NULL && *p != 0) {
        if (r7 != 0) {
            return 1;
        }
        if (flags & 0x400) {
            return 1;
        }
        return (flags & 1) ^ 1;
    }
    return 1;
}

// 0x801BB3D4
// When the object at +0x4 of arg0 has none of the flags 0x20500 set, and arg4 has bit 1 set at
// +0x28, forwards arg4 with the caller's arguments to fn_801BBB68. Returns 0 when flagged,
// otherwise 1.
s32 fn_801BB3D4(u8* arg0, s32 arg1, s32 arg2, s32 arg3, u8* arg4) {
    u8* obj;

    if (arg0 != NULL) {
        obj = *(u8**)(arg0 + 0x4);
        if (*(s32*)(obj + 0x20) & 0x20500) {
            return 0;
        }
        if (*(s32*)(arg4 + 0x28) & 2) {
            fn_801BBB68(arg4, arg1, arg2, arg3);
        }
    }
    return 1;
}

}  // extern "C"
