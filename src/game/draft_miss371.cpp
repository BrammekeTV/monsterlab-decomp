#include "types.h"

extern "C" {
void  fn_80142EE8(void);               // 0x80142EE8 (auto_03_80142E94_text.cpp)
void* fn_80142EEC(s32 a3);             // 0x80142EEC (auto_03_80142E94_text.cpp)
void  fn_80142EF0(u8* a3);             // 0x80142EF0 (auto_03_80142E94_text.cpp)
void  fn_80142EF4(void);               // 0x80142EF4 (auto_03_80142E94_text.cpp)
void* fn_801EEB20(u8* heap, u32 size); // 0x801EEB20 (heap_alloc.cpp)
s32   HeapInit(u8* arg0, u8* arg1, u32 arg2);  // 0x801EF8DC (engine.cpp)
void  fn_80185200(u8* obj, u8* rec, s32 pos, s32 count);  // 0x80185200
void  fn_8018542C(u8* obj, u8* rec, s32 pos, s32 count);  // 0x8018542C

extern u8*  lbl_805FB5D8;   // 0x805FB5D8 (head of the pending-record list)
extern u8   lbl_805FB7A8[]; // 0x805FB7A8 (heap selector record, +4 is the flag)
extern s32  lbl_805FB7E4;   // 0x805FB7E4 (running byte total)
extern u8   lbl_8047CAC0[]; // 0x8047CAC0 (main heap object, second heap at +0x394)
}

extern "C" {

// 0x8015FDA4
// out = a + b, component-wise over a 4-float vector.
void fn_8015FDA4(u8* out, u8* a, u8* b) {
    *(f32*)(out + 0x0) = *(f32*)(a + 0x0) + *(f32*)(b + 0x0);
    *(f32*)(out + 0x4) = *(f32*)(a + 0x4) + *(f32*)(b + 0x4);
    *(f32*)(out + 0x8) = *(f32*)(a + 0x8) + *(f32*)(b + 0x8);
    *(f32*)(out + 0xC) = *(f32*)(a + 0xC) + *(f32*)(b + 0xC);
}

// 0x8015FDE8
// out = a * s, scaling a 4-float vector by a float scalar.
void fn_8015FDE8(u8* out, u8* a, f32* s) {
    f32 k = *s;
    *(f32*)(out + 0x0) = *(f32*)(a + 0x0) * k;
    *(f32*)(out + 0x4) = *(f32*)(a + 0x4) * k;
    *(f32*)(out + 0x8) = *(f32*)(a + 0x8) * k;
    *(f32*)(out + 0xC) = *(f32*)(a + 0xC) * k;
}

// 0x8015FED8
// out = a - b, component-wise over a 4-float vector.
void fn_8015FED8(u8* out, u8* a, u8* b) {
    *(f32*)(out + 0x0) = *(f32*)(a + 0x0) - *(f32*)(b + 0x0);
    *(f32*)(out + 0x4) = *(f32*)(a + 0x4) - *(f32*)(b + 0x4);
    *(f32*)(out + 0x8) = *(f32*)(a + 0x8) - *(f32*)(b + 0x8);
    *(f32*)(out + 0xC) = *(f32*)(a + 0xC) - *(f32*)(b + 0xC);
}

// 0x8015FFEC
// a += b, component-wise over a 4-float vector.
void fn_8015FFEC(u8* a, u8* b) {
    *(f32*)(a + 0x0) = *(f32*)(a + 0x0) + *(f32*)(b + 0x0);
    *(f32*)(a + 0x4) = *(f32*)(a + 0x4) + *(f32*)(b + 0x4);
    *(f32*)(a + 0x8) = *(f32*)(a + 0x8) + *(f32*)(b + 0x8);
    *(f32*)(a + 0xC) = *(f32*)(a + 0xC) + *(f32*)(b + 0xC);
}

// 0x8016F740
// Returns 1 when *a >= *b (fcmpo, then cror eq,gt,eq), 0 otherwise.
s32 fn_8016F740(f32* a, f32* b) {
    return (s32)(*a >= *b);
}

// 0x80182ABC
// Resets the record at obj: the active flag at +0x30 is set, the memory pools are allocated
// (+0x0, +0x2C, +0x28 and four slots at +0x8), and the matching counters are cleared.
void fn_80182ABC(u8* obj) {
    *(s32*)(obj + 0x30) = 1;
    fn_80142EE8();
    *(void**)(obj + 0x0) = fn_80142EEC(0x90000);
    *(s32*)(obj + 0x4) = 0;
    *(void**)(obj + 0x2C) = fn_80142EEC(0x1C00);
    *(s32*)(obj + 0x38) = 0;
    *(void**)(obj + 0x28) = fn_80142EEC(0xC000);
    *(s32*)(obj + 0x34) = 0;
    for (s32 i = 0; i < 4; i++) {
        *(void**)(obj + 0x8 + i * 4) = fn_80142EEC(0x8000);
        *(s32*)(obj + 0x18 + i * 4) = 0;
    }
}

// 0x80183E88
// Frees the four slots at +0x8 and the pools at +0x28, +0x2C and +0x0, clears them and
// the active flag at +0x30.
void fn_80183E88(u8* obj) {
    for (s32 i = 0; i < 4; i++) {
        fn_80142EF0(*(u8**)(obj + 0x8 + i * 4));
        *(void**)(obj + 0x8 + i * 4) = NULL;
    }
    fn_80142EF0(*(u8**)(obj + 0x28));
    *(void**)(obj + 0x28) = NULL;
    fn_80142EF0(*(u8**)(obj + 0x2C));
    *(void**)(obj + 0x2C) = NULL;
    fn_80142EF0(*(u8**)(obj + 0x0));
    *(void**)(obj + 0x0) = NULL;
    fn_80142EF4();
    *(s32*)(obj + 0x30) = 0;
}

// 0x80185988
// Walks the record list at +0x10 in chunks of at most 0x80 units up to the total at +0x68,
// dispatching each chunk to fn_80185200 (kind 1) or fn_8018542C (kind 2). A non-null
// lbl_805FB5D8 is handled as the first record and ends the walk for that chunk.
void fn_80185988(u8* arg0) {
    s32 pos = 0;
    while (pos < *(s32*)(arg0 + 0x68)) {
        s32 n = *(s32*)(arg0 + 0x68) - pos;
        if (n > 0x80) {
            n = 0x80;
        }
        u8* node = *(u8**)(arg0 + 0x10);
        for (;;) {
            u8* cur = lbl_805FB5D8;
            if (cur == NULL) {
                if (node == NULL) {
                    break;
                }
                cur = node - 4;
            }
            s16 kind = *(s16*)(cur + 0x10);
            if (kind == 1) {
                fn_80185200(arg0, cur, pos, n);
            } else if (kind == 2) {
                fn_8018542C(arg0, cur, pos, n);
            }
            if (lbl_805FB5D8 != NULL) {
                break;
            }
            node = *(u8**)node;
        }
        pos += n;
    }
}

// 0x802050B0
// Writes a 4-row block of packed byte pairs (row and column index, biased by 0x80) into dst,
// row pitch in units of stride, w >> 3 entries per row.
void fn_802050B0(u8* dst, s32 w, s32 stride) {
    u32 count = (u32)w >> 3;
    s32 r10 = 0;
    s32 r11 = 0;
    for (s32 row = 0; row < 4; row++) {
        for (s32 k = 0; k < (s32)count; k++) {
            s32 a = r10 - k;
            s32 b = r11 - row;
            u8* p = dst + ((k + row * stride) * 2);
            r11 += 1;
            p[0] = (u8)(a + 0x80);
            p[1] = (u8)(b + 0x80);
            if (r11 == 4) {
                r11 = 0;
                r10 += 1;
            }
        }
    }
}

// 0x80205134
// Copies 8-byte columns of 4 rows at a time: each group of 4 source rows (stride arg2*arg4
// apart) is written as 32 bytes of 8 words into dst, advancing dst by 0x20 per column.
void fn_80205134(u8* dst, u8* src, s32 arg2, s32 arg3, s32 arg4) {
    u32 stride = (u32)arg2 * (u32)arg4;
    u32 colsPerRow = stride >> 3;
    u32 groups = (u32)arg3 >> 2;
    u32 dstOff = 0;
    u32 srcRow = 0;
    for (u32 g = 0; g < groups; g++) {
        u32 base = srcRow * stride;
        for (u32 j = 0; j < colsPerRow; j++) {
            u8* p = src + base + j * 8;
            u8* q = dst + dstOff;
            *(u32*)(q + 0x0) = *(u32*)(p + 0x0);
            *(u32*)(q + 0x4) = *(u32*)(p + 0x4);
            *(u32*)(q + 0x8) = *(u32*)(p + stride);
            *(u32*)(q + 0xC) = *(u32*)(p + stride + 0x4);
            *(u32*)(q + 0x10) = *(u32*)(p + stride * 2);
            *(u32*)(q + 0x14) = *(u32*)(p + stride * 2 + 0x4);
            *(u32*)(q + 0x18) = *(u32*)(p + stride * 3);
            *(u32*)(q + 0x1C) = *(u32*)(p + stride * 3 + 0x4);
            dstOff += 0x20;
        }
        srcRow += 4;
    }
}

// 0x801D54B0
// Adds size to lbl_805FB7E4, takes a block of that size from the main heap (or the second
// heap at +0x394 when lbl_805FB7A8+4 is set), zeroes obj+0x390 and initialises obj on the
// block with HeapInit. Returns 1 on success, 0 when no heap could supply the block.
s32 fn_801D54B0(u8* obj, s32 size) {
    s32 pick = 0;
    u8* block;
    lbl_805FB7E4 += size;
    if (*(s32*)(lbl_805FB7A8 + 4) == 0) {
        pick = 1;
        block = (u8*)fn_801EEB20(lbl_8047CAC0, (u32)size);
    } else {
        block = (u8*)fn_801EEB20(lbl_8047CAC0 + 0x394, (u32)size);
    }
    if (block == NULL) {
        block = (u8*)fn_801EEB20(lbl_8047CAC0 + pick * 0x394, (u32)size);
        if (block == NULL) {
            return 0;
        }
    }
    *(s32*)(obj + 0x390) = 0;
    HeapInit(obj, block, (u32)size);
    return 1;
}

}  // extern "C"
