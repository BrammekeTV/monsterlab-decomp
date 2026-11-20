// Small functions in 0x8015xxxx-0x8019xxxx (copy/cursor helpers, list and tree lookups, heap-backed record create/free, GX setup).
#include <string.h>
#include <stdint.h>
#include "types.h"
#include "revolution.h"

extern "C" {
void     fn_801731C8(void* p);                                    // 0x801731C8
void     fn_801732B0(u8* p);                                      // 0x801732B0
void     fn_8017385C(void* p);                                    // 0x8017385C
void     fn_8017493C(void* p);                                    // 0x8017493C
s32      fn_80174C2C(u8* p);                                      // 0x80174C2C
void     fn_801785A8(void* p);                                    // 0x801785A8
void     fn_8017A618(void* p);                                    // 0x8017A618
void     fn_8017ABB8(void* p, s32 idx);                           // 0x8017ABB8
u8*      fn_801842B0(u8* p);                                      // 0x801842B0
void     fn_80184414(u8* owner, u8* node);                        // 0x80184414
s32 fn_80198C78(u8* set);                                  // 0x80198C78
s32      fn_8033B1C0(const char* name, u8* entry);                // 0x8033B1C0
void     fn_8018D370(const char* fmt, ...);                       // 0x8018D370
}

extern u8*        lbl_805FB608;      // 0x805FB608 (heap object: slot +0 = alloc, slot +4 = free, called as (p, heap))
extern s32        lbl_805FB514;      // 0x805FB514 (GX shadow word)
extern u8         lbl_805FB50C;      // 0x805FB50C (GX shadow flags byte)
extern u8         lbl_805FB50E;      // 0x805FB50E (GX shadow byte)
extern u8         lbl_805F9211;      // 0x805F9211 (GX value byte)
extern u8         lbl_805F9212;      // 0x805F9212 (GX filter value)
extern u8         lbl_80412B00[];    // 0x80412B00 (4 record pointers)
extern u8         lbl_80412B10[];    // 0x80412B10 (4 record pointers)
extern const char lbl_80381450[];    // 0x80381450 (log string)

extern "C" {

// 0x8015A144
// Copy the primary block (+0x14 bytes to +0x18) and the strided block (+0xD count x +0xE stride to +0x1C)
// from *arg1, advance *arg1 with 4-byte alignment, then flush both destinations.
void fn_8015A144(u8* arg0, u8** arg1) {
    u32 size;
    u32 len;
    u8 count;
    u8* next;

    size = *(u32*)(arg0 + 0x14);
    memcpy(*(void**)(arg0 + 0x18), *arg1, size);
    next = *arg1 + size;
    *arg1 = next;
    count = *(u8*)(arg0 + 0xD);
    if (count != 0) {
        len = (u32)*(u8*)(arg0 + 0xE) * count;
        memcpy(*(void**)(arg0 + 0x1C), next, len);
        *arg1 = *arg1 + len;
    }
    *arg1 = (u8*)(((uintptr_t)*arg1 + 3) & ~(uintptr_t)3);
    DCFlushRange(*(void**)(arg0 + 0x18), *(u32*)(arg0 + 0x14));
    if (count != 0) {
        DCFlushRange(*(void**)(arg0 + 0x1C), (u32)*(u8*)(arg0 + 0xE) * count);
    }
}

// 0x80169770
// Walk the list headed at arg0+4 (link at node+0, entry at node-0x28, name at entry+0x35) and return the entry matching name, or NULL.
u8* fn_80169770(u8* arg0, const char* name) {
    u8* node = *(u8**)(arg0 + 0x4);
    while (node != NULL) {
        u8* entry = node - 0x28;
        if (fn_8033B1C0(name, entry + 0x35) == 0) {
            return entry;
        }
        node = *(u8**)node;
    }
    return NULL;
}

// 0x801697E0
// If arg1 is owned by arg0 (owner = arg1+0x18 - 0x10), release its sub-object at +0x10 and clear it via fn_80174C2C.
void fn_801697E0(u8* arg0, u8* arg1) {
    u8* owner = NULL;
    if (*(u8**)(arg1 + 0x18) != NULL) {
        owner = *(u8**)(arg1 + 0x18) - 0x10;
    }
    if (owner == arg0) {
        fn_801731C8(arg1 + 0x10);
        fn_80174C2C(arg1);
    }
}

// 0x801698C0
// If arg1 is owned by arg0 (owner = arg1+0xC - 0x28), drop one reference on arg1 and free it at zero.
void fn_801698C0(u8* arg0, u8* arg1) {
    u8* owner = NULL;
    s32 cnt;
    if (*(u8**)(arg1 + 0xC) != NULL) {
        owner = *(u8**)(arg1 + 0xC) - 0x28;
    }
    if (owner == arg0) {
        fn_801731C8(arg1 + 0x4);
        cnt = *(s32*)(arg1 + 0x10) - 1;
        *(s32*)(arg1 + 0x10) = cnt;
        if (cnt == 0) {
            fn_801732B0(arg1);
        }
    }
}

// 0x8016D818
// Release the three owned blocks at +0x18, +0x1C, +0x20 through the heap, then free arg0 itself.
void fn_8016D818(u8* arg0) {
    if (*(u8**)(arg0 + 0x18) != NULL) {
        (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(*(void**)(arg0 + 0x18), lbl_805FB608);
        *(void**)(arg0 + 0x18) = NULL;
    }
    if (*(u8**)(arg0 + 0x1C) != NULL) {
        (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(*(void**)(arg0 + 0x1C), lbl_805FB608);
        *(void**)(arg0 + 0x1C) = NULL;
    }
    if (*(u8**)(arg0 + 0x20) != NULL) {
        (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(*(void**)(arg0 + 0x20), lbl_805FB608);
        *(void**)(arg0 + 0x20) = NULL;
    }
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(arg0, lbl_805FB608);
}

// 0x80172708
// Tear down a node: detach from its owner, release sub-objects at +0x4, +0x10, +0x38, +0xB0, clear counters, then free it.
void fn_80172708(u8* arg0) {
    u8* tmp;
    s32 rc;

    tmp = fn_801842B0(*(u8**)(arg0 + 0xC0));
    if (tmp != NULL) {
        fn_80184414(tmp, arg0);
    }
    if (*(void**)(arg0 + 0x4) != NULL) {
        fn_8017385C(*(void**)(arg0 + 0x4));
        *(void**)(arg0 + 0x4) = NULL;
    }
    tmp = *(u8**)(arg0 + 0x10);
    if (tmp != NULL) {
        rc = *(s32*)(tmp + 0x10) - 1;
        *(s32*)(tmp + 0x10) = rc;
        if (rc == 0) {
            fn_801732B0(tmp);
        }
        *(void**)(arg0 + 0x10) = NULL;
    }
    if (*(void**)(arg0 + 0x38) != NULL) {
        (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(*(void**)(arg0 + 0x38), lbl_805FB608);
        *(void**)(arg0 + 0x38) = NULL;
    }
    if (*(u8**)(arg0 + 0xB0) != NULL) {
        fn_8017493C(*(void**)(arg0 + 0xB0));
        *(void**)(arg0 + 0xB0) = NULL;
    }
    *(s32*)(arg0 + 0x30) = 0;
    *(s32*)(arg0 + 0x34) = 0;
    *(s32*)(arg0 + 0x0) = 0;
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(arg0, lbl_805FB608);
}

// 0x80173400
// Scan the count-prefixed table at +0x70 (8-byte stride from arg0) for an entry whose byte at +0xD equals arg1.
s32 fn_80173400(u8* arg0, s32 arg1) {
    s32 i;
    s32 count = *(s32*)(arg0 + 0x6C);
    u8* p = arg0;
    for (i = 0; i < count; i++) {
        if (arg1 == (s32)*(u8*)(*(u8**)(p + 0x70) + 0xD)) {
            return 1;
        }
        p += 8;
    }
    return 0;
}

// 0x8017A56C
// Create the four 0x13C-byte records stored in lbl_80412B00; record 0 also clears its byte at +0x139 and word at +0x12C.
void fn_8017A56C(void) {
    s32 i;
    u8* rec;

    for (i = 0; i < 4; i++) {
        rec = (u8*)(*(void* (**)(u32, u32, u32, u32, u32))lbl_805FB608)(0x13C, 0x10, 0x12, 0, 0);
        *(s32*)(rec + 0x50) = 0;
        fn_801785A8(rec);
        *(void**)(lbl_80412B00 + i * 4) = rec;
        if (i == 0) {
            *(u8*)(rec + 0x139) = 0;
            *(s32*)(rec + 0x12C) = 0;
        }
    }
}

// 0x80182A1C
// Create the four 0xD20-byte records stored in lbl_80412B10 and initialise each one.
void fn_80182A1C(void) {
    s32 i;
    u8* rec;

    for (i = 0; i < 4; i++) {
        rec = (u8*)(*(void* (**)(u32, u32, u32, u32, u32))lbl_805FB608)(0xD20, 0x10, 0x12, 0, 0);
        *(s32*)(rec + 0x0) = 0;
        fn_8017A618(rec);
        *(void**)(lbl_80412B10 + i * 4) = rec;
        fn_8017ABB8(rec, i);
    }
}

// 0x80189410
// GX write-gather setup: fixed command sequence, then one-time register init guarded by flags bit 4, then filter register.
void fn_80189410(void) {
    u32 val;

    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE000090u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x80000090u;
    if ((lbl_805FB50C & 4) == 0) {
        lbl_805FB50C |= 4;
        val = (u32)lbl_805FB514 & 0xFFFFFFBFu;
        lbl_805FB514 = (s32)val;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = val | 0x43000000u;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)lbl_805FB50E | 0xF3380000u | ((u32)lbl_805F9211 << 16);
    }
    lbl_805FB50C |= 8;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
}

// 0x8018D648
// Release the pointer at slot arg1 (4-byte stride from +0x88 of arg0) through fn_80198C78, then clear it.
void fn_8018D648(u8* arg0, s32 arg1) {
    u8* slot = arg0 + arg1 * 4;
    if (*(void**)(slot + 0x88) != NULL) {
        fn_80198C78(*(u8**)(slot + 0x88));
    }
    *(void**)(slot + 0x88) = NULL;
}

// 0x8018E808
// Binary-tree lookup of key arg1 in the tree at arg0 (root at +0, bounds records at +4 and +8, children at node+0xC/+0x10); NULL if absent.
u8* fn_8018E808(u8* arg0, u32 arg1) {
    u8* node;
    u32 key;

    if (arg0 == NULL || (node = *(u8**)arg0) == NULL) {
        fn_8018D370(lbl_80381450, arg1);
        return NULL;
    }
    if (arg1 < *(u32*)(*(u8**)(arg0 + 0x4))) {
        return NULL;
    }
    if (arg1 > *(u32*)(*(u8**)(arg0 + 0x8))) {
        return NULL;
    }
    for (;;) {
        key = *(u32*)node;
        if (arg1 == key) {
            return node;
        }
        node = (arg1 < key) ? *(u8**)(node + 0xC) : *(u8**)(node + 0x10);
        if (node == NULL) {
            return NULL;
        }
    }
}

// 0x80195570
// Remove entry arg1 from the array at arg0 (count +0, capacity +4, stride +8, base +0x10) by shifting the tail down.
void fn_80195570(u8* arg0, s32 arg1) {
    s32 count = *(s32*)(arg0 + 0x0);
    s32 stride;
    u8* dst;

    if (arg1 == count) {
        *(s32*)(arg0 + 0x0) = count - 1;
        return;
    }
    if (arg1 < 0 || arg1 >= *(s32*)(arg0 + 0x4)) {
        dst = NULL;
    } else {
        dst = *(u8**)(arg0 + 0x10) + arg1 * *(s32*)(arg0 + 0x8);
    }
    stride = *(s32*)(arg0 + 0x8);
    memcpy(dst, dst + stride, stride * (count - arg1));
    *(s32*)(arg0 + 0x0) = *(s32*)(arg0 + 0x0) - 1;
}

}  // extern "C"
