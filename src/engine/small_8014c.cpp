// Small functions in 0x8014xxxx: log timestamp/auto-delete flags, the pad button and
// register-table helpers, the 0x80146xxx base destructor and GX write-gather command helpers.
#include <string.h>
#include "types.h"
#include "revolution.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
s32   fn_803334E8(void* a, s32 flag);                            // 0x803334E8
s32   fn_80146E44(u8* p, u8* dst, s32 count);                                        // 0x80146E44
void  fn_80146D98(u8* p);                                        // 0x80146D98
s32   fn_80146DD8(u8* p, s32 mode, s32 val);                     // 0x80146DD8
void  fn_80334458(void* obj, s32 x, s32 y);                      // 0x80334458
s32   fn_80334290(void* obj);                                    // 0x80334290
void  fn_80333B60(void* obj);                                    // 0x80333B60
void  DVDClose(u8* p);                                        // 0x8024F494
u8*   fn_80142500(void* p);                                      // 0x80142500
void  fn_801472A8(void);                                         // 0x801472A8
void  fn_801479D0(void);                                    // 0x801479D0
void  fn_802727E0(s32 a, s32 b);                                 // 0x802727E0
s32   fn_801478C8(u8* table, s32 idx);                           // 0x801478C8
}

extern u8    lbl_803F22D8[];     // 0x803F22D8 (global object; flags word at +0x308)
extern void* lbl_805FB478;       // 0x805FB478 (pointer; cleared when freed)
extern void* lbl_805FB488;       // 0x805FB488 (pointer; set by fn_80146EEC)
extern u8*   lbl_805FB490;       // 0x805FB490 (current register table)
extern u8*   lbl_805FB494;       // 0x805FB494 (spare register table)
extern void* lbl_805FB498;       // 0x805FB498 (callback pointer)
extern u8    lbl_805FB50C;       // 0x805FB50C (GX shadow flags byte)
extern u8    lbl_805FB50D;       // 0x805FB50D (GX shadow mask byte)
extern f32   lbl_805FF084;       // 0x805FF084 (float constant)
extern s32   lbl_80400FE0;       // 0x80400FE0

extern "C" {

// 0x8014308C
// Script callback: a0 == 1 sets bit 0x2 of the log flags, a0 == 0 clears it; other values do nothing.
void FilenameTimestamp(BOOL a0) {
    if (a0 == 1) {
        *(u32*)(lbl_803F22D8 + 0x308) |= 0x2;
    } else if (a0 == 0) {
        *(u32*)(lbl_803F22D8 + 0x308) &= ~0x2u;
    }
}

// 0x801430D0
// Script callback: a0 == 1 sets bit 0x4 of the log flags, a0 == 0 clears it; other values do nothing.
void LineTimestamp(BOOL a0) {
    if (a0 == 1) {
        *(u32*)(lbl_803F22D8 + 0x308) |= 0x4;
    } else if (a0 == 0) {
        *(u32*)(lbl_803F22D8 + 0x308) &= ~0x4u;
    }
}

// 0x80143114
// Script callback: a0 == 1 sets bit 0x1 of the log flags, a0 == 0 clears it; other values do nothing.
void LogAutoDelete(BOOL a0) {
    if (a0 == 1) {
        *(u32*)(lbl_803F22D8 + 0x308) |= 0x1;
    } else if (a0 == 0) {
        *(u32*)(lbl_803F22D8 + 0x308) &= ~0x1u;
    }
}

// 0x80146930
// Dispatches to the attached child when present; otherwise, for state 1, runs the inline sub-object.
s32 fn_80146930(u8* p, void* a, s32 c) {
    s32 r = 0;
    if (*(void**)(p + 0x4) != 0) {
        r = fn_803334E8(a, 1);
    } else if (*(s32*)(p + 0x0) == 1) {
        r = fn_80146E44(p + 0xc, (u8*)a, c);
    }
    return r;
}

// 0x8014698C
// Forwards the (a, b) pair to the attached child when present; otherwise, for state 1, updates the inline
// sub-object via fn_80146DD8 (mode a, value b). Returns 0 when neither applies.
s32 fn_8014698C(u8* p, s32 a, s32 b) {
    if (*(void**)(p + 0x4) != 0) {
        fn_80334458(*(void**)(p + 0x4), b, a);
        return fn_80334290(*(void**)(p + 0x4));
    }
    if (*(s32*)(p + 0x0) == 1) {
        return fn_80146DD8(p + 0xc, a, b);
    }
    return 0;
}

// 0x801469FC
// Releases the attached child (if any) and clears its pointer; otherwise, for state 1, runs the inline teardown.
void fn_801469FC(u8* p) {
    if (*(void**)(p + 0x4) != 0) {
        fn_80333B60(*(void**)(p + 0x4));
        *(void**)(p + 0x4) = 0;
    } else if (*(s32*)(p + 0x0) == 1) {
        fn_80146D98(p + 0xc);
    }
}

// 0x80146D98
// Teardown of the inline sub-object: notifies DVDClose and clears the shared pointer lbl_805FB478 if it names p.
void fn_80146D98(u8* p) {
    DVDClose(p);
    if (lbl_805FB478 == p) {
        lbl_805FB478 = 0;
    }
}

// 0x80146DD8
// Updates the 32-bit value at +0x40 (mode 0 sets, 1 adds, 2 sets to the value at +0x3C plus val);
// clears the status word at +0x50 and returns the value. Other modes flag -1 at +0x4C and return -1.
s32 fn_80146DD8(u8* p, s32 mode, s32 val) {
    if (mode == 0) {
        *(s32*)(p + 0x40) = val;
    } else if (mode == 1) {
        *(s32*)(p + 0x40) += val;
    } else if (mode == 2) {
        *(s32*)(p + 0x40) = *(s32*)(p + 0x3c) + val;
    } else {
        *(s32*)(p + 0x4c) = -1;
        return -1;
    }
    *(s32*)(p + 0x50) = 0;
    return *(s32*)(p + 0x40);
}

// 0x80146EEC
// Init: sets up the sub-block at +0xA8 and claims the shared slot lbl_805FB488 (error code if already held).
u8* fn_80146EEC(u8* p) {
    fn_80142500(p + 0xa8);
    if (lbl_805FB488 != 0) {
        *(s32*)(p + 0x8) = -0xe;
        *(s32*)(p + 0x4) = -2;
    } else {
        lbl_805FB488 = p;
        *(s32*)(p + 0x4) = 0;
    }
    return p;
}

// 0x80146F50
// Base destructor: when p is set, releases the child at +4 (marking the state at +8), clears lbl_805FB488,
// and frees p when del > 0.
void fn_80146F50(u8* p, s32 del) {
    if (p) {
        if (*(s32*)(p + 0x4) != 0) {
            *(s32*)(p + 0x4) = 0;
            *(s32*)(p + 0x8) = 1;
        }
        lbl_805FB488 = 0;
        if (del > 0) operator_delete(p);
    }
}

// 0x801474EC
// Pad button query: reads the entry for the button from the current register table. The top three bits
// of the entry select the form: 1 gives bit 0; 2 compares the entry (shifted left 3) read as a float with
// lbl_805FF084 and gives whether it is greater; anything else gives 0.
BOOL PadButtonHeld(s32 button) {
    u32 v = (u32)fn_801478C8(lbl_805FB490, button);
    u32 kind = v >> 29;
    if (kind == 1) {
        return (BOOL)(v & 1);
    }
    if (kind == 2) {
        u32 bits = v << 3;
        f32 f = *(f32*)&bits;
        return (f > lbl_805FF084) ? 1 : 0;
    }
    return 0;
}

// 0x801478C8
// Reads a register-table word. Indices 0..0xFF are direct. Indices 0x1085..0x5084 select one of four
// 0xB4-byte banks, each holding 0x2D words at +0x400, using the low 12 bits as the word index (0x85..0xB1).
// Anything else reads as 0.
s32 fn_801478C8(u8* table, s32 idx) {
    if (idx >= 0 && idx < 0x100) {
        return *(s32*)(table + idx * 4);
    }
    s32 bank = (idx - 0x1085) >> 12;
    if (bank < 0 || bank >= 4) return 0;
    s32 off = (idx & 0xFFF) - 0x85;
    if (off < 0 || off >= 0x2d) return 0;
    return *(s32*)(table + 0x400 + bank * 0xb4 + off * 4);
}

// 0x8014792C
// Writes a register-table word using the same index mapping as fn_801478C8. Out-of-range indices are ignored.
void fn_8014792C(u8* table, s32 idx, s32 val) {
    if (idx >= 0 && idx < 0x100) {
        *(s32*)(table + idx * 4) = val;
        return;
    }
    s32 bank = (idx - 0x1085) >> 12;
    if (bank < 0 || bank >= 4) return;
    s32 off = (idx & 0xFFF) - 0x85;
    if (off < 0 || off >= 0x2d) return;
    *(s32*)(table + 0x400 + bank * 0xb4 + off * 4) = val;
}

// 0x80147990
// Dispatch: with a == 0, clears lbl_80400FE0 when b < 0 and forwards to the callback in lbl_805FB498 if set;
// with a != 0 and b == 0, forwards to fn_802727E0.
void fn_80147990(s32 a, s32 b) {
    if (a == 0) {
        if (b < 0) lbl_80400FE0 = 0;
        void* cb = lbl_805FB498;
        if (cb != 0) ((void (*)(s32, s32))cb)(a, b);
        return;
    }
    if (b == 0) fn_802727E0(a, b);
}

// 0x80148228
// Swaps the current and spare register tables, runs fn_801479D0 on the old one, then clears the new
// current table (0x400 bytes) and calls fn_801472A8 on it.
void fn_80148228(void) {
    u8* old = lbl_805FB490;
    lbl_805FB490 = lbl_805FB494;
    lbl_805FB494 = old;
    fn_801479D0();
    memset(lbl_805FB490, 0, 0x400);
    fn_801472A8();
}

// 0x80149428
// Writes three floats to the GX command pipe as a load of the register at index idx (addressed as
// (idx << 4) + 0x607), preceded by a 0x10 header and a 0x0002 word.
void fn_80149428(s32 idx, f32* v) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)((idx << 4) + 0x607);
    GXWGFifo._f32 = v[0];
    GXWGFifo._f32 = v[1];
    GXWGFifo._f32 = v[2];
}

// 0x80149468
// Clears bit idx of the shadow mask lbl_805FB50D. When the flag in lbl_805FB50C is set, also sends the
// updated mask to the GX pipe.
void fn_80149468(s32 idx) {
    u8 flags = lbl_805FB50C;
    u32 sh = (u32)idx & 0x3f;
    u32 bit = (sh < 32) ? (1u << sh) : 0;
    u8 mask = (u8)(lbl_805FB50D & ~bit);
    lbl_805FB50D = mask;
    if ((flags & 1) == 0) return;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x100e;
    GXWGFifo._u32 = (((u32)mask << 2) & 0x3FC) | 0x742;
}

// 0x801494B4
// Sends a fixed block of GX command words: a 0x10 header, two halfwords, then three rows of
// 1.0f and four zero words, followed by a final 1.0f and a zero word.
void fn_801494B4(void) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0xb;
    GXWGFifo._u16 = 0x500;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._u32 = 0;
}

// 0x8014950C
// Writes a two-word BP-style register update (0xFE000090 header) with the fields a and b packed into the
// second word.
void fn_8014950C(s32 a, s32 b) {
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE000090;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x80000000u | ((u32)a << 7) | ((u32)b << 4);
}

}  // extern "C"
