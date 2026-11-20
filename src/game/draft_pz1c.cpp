// Small functions in 0x801Dxxxx-0x801Exxxx (table lookup, table copy, bump-pool allocator helpers).
#include <string.h>

#include "types.h"

extern u8 lbl_804DD100[];   // bump pool block (see fn_801EE334 / fn_801EE38C)
extern u8 lbl_805FB868[];   // holds a function pointer (called with the code, see fn_801DC430)
extern u8 lbl_805FB900[];   // holds a pointer to an object whose first word is a function pointer

extern "C" {
void* MemAllocDefault(u32 size);                 // 0x801D4650
void  BufferFree(void* p);                       // 0x801D4980
s32   fn_801EE38C(u8* a3);                       // 0x801EE38C
s32   fn_801EE334(u8* pool, s32 size);           // 0x801EE334

// 0x801DC430
// Maps a code to a category: -2 for 10, 0 for codes not in the table, 1 or 4 for the ranges
// below, and for codes >= 0x2B forwards the code to the handler stored at lbl_805FB868 (if set).
static s32 CodeTailCall(s32 code) {
    if (code < 0x2B) return 0;
    void* fn = *(void**)lbl_805FB868;
    if (fn == NULL) return 0;
    return ((s32(*)(s32))fn)(code);
}

s32 fn_801DC430(s32 code) {
    if (code == 10) return -2;
    if (code >= 10) {
        if (code == 0x15) return 0;
        if (code > 0x15) {
            if (code >= 0x2B) return CodeTailCall(code);
            return 1;
        }
        if (code >= 0x14) return 1;
        if (code >= 0x12) return 4;
        return 1;
    }
    if (code == 7) return 0;
    if (code >= 7) return 1;
    if (code >= 2) return 1;
    if (code >= 0) return 0;
    return CodeTailCall(code);
}

// 0x801DFE94
// Copies a 0x100-byte table from `table` to `base`; does nothing when `table` is NULL.
void fn_801DFE94(u8* base, u8* table) {
    if (table == NULL) return;
    memcpy(base, table, 0x100);
}

// 0x801EBBE0
// Walks a linked list of nodes (head at obj+0x14; each node's next-node pointer is at node+4 and
// the node payload starts at node+8). Calls fn(node, arg) on each node in turn; the walk stops
// as soon as fn returns 0. Always returns 0.
s32 fn_801EBBE0(u8* obj, void* fn, s32 arg) {
    u8* head = *(u8**)(obj + 0x14);
    u8* cur = (head != NULL) ? head + 8 : NULL;
    while (cur != NULL) {
        u8* nextBase = *(u8**)(cur - 4);
        u8* next = (nextBase != NULL) ? nextBase + 8 : NULL;
        s32 r = ((s32(*)(u8*, s32))fn)(cur, arg);
        if (r == 0) return 0;
        cur = next;
    }
    return 0;
}

// 0x801EE1F8
// Resets the buffer record at `a` (flags at +0 and +0x14, buffer at +0x4, cursor at +0x8,
// used count at +0xC, capacity at +0x10). Frees any owned buffer, then allocates `n` bytes.
// Returns 1 on success (or when n is 0), 0 when the allocation failed.
s32 fn_801EE1F8(u8* a, s32 n) {
    if (*(u8*)(a + 0x0) != 0) {
        u8* buf = *(u8**)(a + 0x4);
        if (buf != NULL && *(u8*)(a + 0x14) != 0) {
            BufferFree(buf);
        }
    }
    *(u8**)(a + 0x4) = NULL;
    *(u8*)(a + 0x14) = 0;
    *(s32*)(a + 0x10) = 0;
    *(u8*)(a + 0x0) = 0;
    *(u8**)(a + 0x8) = NULL;
    *(s32*)(a + 0xC) = 0;
    if (n == 0) return 1;
    u8* p = (u8*)MemAllocDefault((u32)n);
    *(u8**)(a + 0x4) = p;
    *(u8*)(a + 0x14) = 1;
    if (p == NULL) return 0;
    *(u8**)(a + 0x8) = p;
    *(s32*)(a + 0x10) = n;
    *(u8*)(a + 0x0) = 1;
    *(s32*)(a + 0xC) = 0;
    return 1;
}

// Shared helper for the pool allocators below: reserves `size` bytes from lbl_804DD100 so the
// returned address is a multiple of `align` (a power of two). Returns 0 if the pool is full.
static s32 PoolAllocAligned(s32 size, s32 align) {
    s32 cur = fn_801EE38C(lbl_804DD100);
    s32 pad = 0;
    s32 low = (align - 1) & cur;
    if (low != 0) pad = align - low;
    s32 base = fn_801EE334(lbl_804DD100, size + pad);
    if (base == 0) return 0;
    return base + pad;
}

// 0x801EFA24
// Pool allocation of a0 bytes aligned to a1 (see PoolAllocAligned).
s32 fn_801EFA24(s32 a0, s32 a1) {
    return PoolAllocAligned(a0, a1);
}

// 0x801EFAA4
// Kinds 6 and 7 allocate a0 bytes aligned to a1 from the pool. Any other kind forwards all three
// arguments to the function pointer stored at the object held by lbl_805FB900.
s32 fn_801EFAA4(s32 a0, s32 a1, s32 a2) {
    if ((u32)(a2 - 6) <= 1U) {
        return PoolAllocAligned(a0, a1);
    }
    u8* obj = *(u8**)lbl_805FB900;
    void* fn = *(void**)obj;
    return ((s32(*)(s32, s32, s32))fn)(a0, a1, a2);
}

// 0x801EFB48
// Pool allocation of a0 bytes, 16-byte aligned.
s32 fn_801EFB48(s32 a0) {
    return PoolAllocAligned(a0, 16);
}

// 0x801EFBB8
// Pool allocation of a1 bytes, 16-byte aligned (a0 is not used).
s32 fn_801EFBB8(s32 a0, s32 a1) {
    (void)a0;
    return PoolAllocAligned(a1, 16);
}

}  // extern "C"
