// Small functions in 0x802200EC-0x8022E3E0 (battle object records, notice queue, debug hooks).
#include "types.h"
#include "game/part.h"

// Debug hook globals (C++ linkage, like the other lbl_ globals). Null in retail builds.
extern void* lbl_805FBBD0;   // 0x805FBBD0 (allocator hook, called with r3 passed through)
extern void* lbl_805FBBD4;   // 0x805FBBD4 (finish hook, called with r3/r4 passed through)

extern "C" {
void  fn_8021F35C(u8* p);                     // 0x8021F35C
void  NoticePosted(u8* p, s32 idx);          // 0x8022DF3C

// 0x802200EC
// Clears the four 0x1004-byte slots starting at +0x5C, then the block at +0x406C.
void fn_802200EC(u8* arg0) {
    u8* slot = arg0 + 0x5C;
    for (s32 i = 0; i < 4; i++) {
        fn_8021F35C(slot);
        slot += 0x1004;
    }
    fn_8021F35C(arg0 + 0x406C);
}

// 0x802209D8
// Debug record allocator: forwards to the hook in lbl_805FBBD0, or returns NULL when none is set.
void* fn_802209D8(s32 kind) {
    if (lbl_805FBBD0 == nullptr) {
        return nullptr;
    }
    return ((void* (*)(s32))lbl_805FBBD0)(kind);
}

// 0x802209F4
// Debug record finish: forwards (data, len) to the hook in lbl_805FBBD4 when one is set.
void fn_802209F4(u8* data, s32 len) {
    if (lbl_805FBBD4 != nullptr) {
        ((void (*)(u8*, s32))lbl_805FBBD4)(data, len);
    }
}

// 0x8022DE8C
// Appends a 0x1C-byte record to the queue at +0x744 (max 15 entries) and posts it.
// Record: a, b, three zeros, -1, then the running sequence number (+0x910) at +0x75C.
void fn_8022DE8C(u8* p, s32 a, s32 b) {
    s32 count = *(s32*)(p + 0x8E8);
    if (count >= 0xF) {
        return;
    }
    u8* rec = p + count * 0x1C;
    *(s32*)(rec + 0x744) = a;
    *(s32*)(rec + 0x748) = b;
    *(s32*)(rec + 0x74C) = 0;
    *(s32*)(rec + 0x750) = 0;
    *(s32*)(rec + 0x754) = 0;
    *(s32*)(rec + 0x758) = -1;
    s32 seq = *(s32*)(p + 0x910) + 1;
    *(s32*)(p + 0x910) = seq;
    *(s32*)(rec + 0x75C) = seq;
    *(s32*)(p + 0x8E8) = count + 1;
    return NoticePosted(p, count);
}

// 0x8022E2A4
// Sets (or, when accumulate is nonzero, adds to) the value at +0x5C4 of entry `idx` (stride 0x3C,
// 4 entries) and marks the record dirty at +0xA20. Out-of-range indices are ignored.
void fn_8022E2A4(u8* p, s32 idx, s32 value, s32 accumulate) {
    if (idx < 0 || idx >= 4) {
        return;
    }
    u8* entry = p + idx * 0x3C;
    if (accumulate != 0) {
        *(s32*)(entry + 0x5C4) = *(s32*)(entry + 0x5C4) + value;
    } else {
        *(s32*)(entry + 0x5C4) = value;
    }
    *(s32*)(p + 0xA20) = 1;
}

// 0x8022E3E0
// Stores the level index at +0x128 and the rank limit (f32 at +8 of 0x10-byte entry `index` of the
// game-DB table at +0x63C8) truncated to u32 plus one at +0x12C.
void fn_8022E3E0(u8* p, s32 index) {
    *(s32*)(p + 0x128) = index;
    u8* table = *(u8**)(gGameDb->data + 0x63C8);
    f32 limit = *(f32*)(table + index * 0x10 + 8);
    *(s32*)(p + 0x12C) = (s32)((u32)limit + 1);
}

}  // extern "C"
