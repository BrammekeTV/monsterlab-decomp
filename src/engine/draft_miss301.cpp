// Small helpers from 0x801B-0x801E: 16-bit glyph lookup and string decode for the text
// layout, temp vertex buffer alloc/commit (shared with the quad batcher), flag bit tests,
// float/bit-pattern reinterpretation, bit-field extract and the script table type lookup.
#include "types.h"

// Script table entry (same layout as src/engine/small_801Dc.cpp and script_dispatch.cpp).
struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes

extern "C++" ScriptFunc gScriptFuncs[];   // 0x80484230

extern "C" {
void fn_8014E3EC(s32 n);                  // 0x8014E3EC
f32  cosf(f32 x);                         // 0x8033EC98 (MSL)
f32  sinf(f32 x);                         // 0x8033F0A4 (MSL)
extern s32 lbl_805FB4C4;                  // 0x805FB4C4 (temp buffer offset of the free area)
extern s32 lbl_805FB4CC;                  // 0x805FB4CC (temp buffer bytes left)
extern u8* lbl_805FB4D0;                  // 0x805FB4D0 (temp buffer base)
extern u8* lbl_805FB728;                  // 0x805FB728 (vertex batch start)
extern u8* lbl_805FB72C;                  // 0x805FB72C (vertex batch write cursor)
extern f32 lbl_805FFDB8;                  // 0x805FFDB8 (lower bound of the float compare)
extern f32 lbl_805FFDBC;                  // 0x805FFDBC (upper bound of the float compare)
extern f32 lbl_805FFDE8;                  // 0x805FFDE8 (angle scale constant)
}

extern "C" {

// Glyph index of a character: table at font+8, indexed by the 16-bit char code times 2.
// 0x801BA6E4
u16 fn_801BA6E4(u8* font, u16 ch) {
    u8* table = *(u8**)(font + 0x8);
    return *(u16*)(table + (((u32)ch << 1) & 0x1FFFE));
}

// Decodes a byte string into 16-bit characters: 0xFF starts a two-byte char whose
// second byte is the low byte, the rest are single bytes. Terminated with a 0 char.
// 0x801BA6F4
void fn_801BA6F4(void* font, u16* out, const u8* str) {
    u32 hi = 0;
    s32 pending = 0;

    (void)font;
    while (*str != 0) {
        u8 c = *str;
        if (pending != 0) {
            hi = ((u32)c << 8) & 0xFF00;
            pending = 0;
        } else if (c == 0xFF) {
            pending = 1;
        } else {
            *out = (u16)(hi | c);
            out++;
        }
        str++;
    }
    *out = 0;
}

// Reserves a 32-byte aligned block of the temp buffer (growing it if needed) and returns it.
// 0x801BB2F8
u8* fn_801BB2F8(s32 size) {
    s32 n = (size + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < n) {
        fn_8014E3EC(n);
    }
    return lbl_805FB4D0 + lbl_805FB4C4;
}

// Commits a block reserved with fn_801BB2F8: it is taken off the free area.
// 0x801BB374
void fn_801BB374(s32 size) {
    s32 n = (size + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < n) {
        fn_8014E3EC(n);
    }
    lbl_805FB4CC -= n;
    lbl_805FB4C4 += n;
}

// Bit test on the flags word at +0x98 of a node: returns the bit as 0 or 1.
// 0x801CC8C8
s32 fn_801CC8C8(u8* node, s32 bit) {
    u32 flags = *(u32*)(node + 0x98);
    return (s32)PpcSrw(flags & PpcSlw(1u, bit), bit);
}

// Bit test on the flags word at +0x1A8 of an object: returns the bit as 0 or 1.
// 0x801CC8F4
s32 fn_801CC8F4(u8* obj, s32 bit) {
    u32 flags = *(u32*)(obj + 0x1A8);
    return (s32)PpcSrw(flags & PpcSlw(1u, bit), bit);
}

// Starts a new vertex batch at the current end of the temp buffer.
// 0x801CCDC4
void fn_801CCDC4(void) {
    u8* start;

    if (lbl_805FB4CC < 0x400) {
        fn_8014E3EC(0x400);
    }
    start = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB728 = start;
    lbl_805FB72C = start;
}

// Commits the vertices written since the batch started (the batch size rounded to 32 bytes).
// 0x801CCFE0
void fn_801CCFE0(void) {
    s32 n = (s32)(lbl_805FB72C - lbl_805FB728);
    n = (n + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < n) {
        fn_8014E3EC(n);
    }
    lbl_805FB4CC -= n;
    lbl_805FB4C4 += n;
}

// Returns 1 when a - b lies strictly between the two float bounds, 0 otherwise.
// 0x801CD648
s32 fn_801CD648(f32* a, f32* b) {
    f32 d = *a - *b;

    // 1 when the difference is in [lbl_805FFDBC, lbl_805FFDB8]: "<= lbl_805FFDB8", then ">= lbl_805FFDBC".
    if (!(d <= lbl_805FFDB8)) {
        return 0;
    }
    if (!(d >= lbl_805FFDBC)) {
        return 0;
    }
    return 1;
}

// Cosine and sine of angle * scale (scale constant at lbl_805FFDE8).
// 0x801CD6A8
void fn_801CD6A8(f32* angle, f32* cosOut, f32* sinOut) {
    f32 a = lbl_805FFDE8 * *angle;

    *cosOut = cosf(a);
    *sinOut = sinf(a);
}

// Float to its 32-bit pattern (no conversion).
// 0x801D0340
u32 fn_801D0340(f32 v) {
    return *(u32*)&v;
}

// 32-bit pattern to float (no conversion).
// 0x801D0354
f32 fn_801D0354(u32 bits) {
    return *(f32*)&bits;
}

// Extracts a bit field: width bits starting at bit shift, moved down to bit 0.
// Shift amounts use the PowerPC rules (low 6 bits, 32..63 gives 0).
// 0x801E2380
u32 fn_801E2380(u32 value, u32 width, u32 shift) {
    u32 mask = (PpcSlw(1u, width) - 1u);
    mask = PpcSlw(mask, shift);
    return PpcSrw(value & mask, shift);
}

// Type of a script table entry; 0 when the index is -1 (no entry).
// 0x801E23B0
u32 fn_801E23B0(u32 idx) {
    if (idx == 0xFFFFFFFFu) {
        return 0;
    }
    return gScriptFuncs[idx].type;
}

}  // extern "C"
