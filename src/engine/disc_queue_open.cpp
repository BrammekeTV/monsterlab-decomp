// Disc request queue: the "open" request.
// Original: 0x801E4BE4 (auto_03_801E47D8_text.s)
#include <string.h>
#include "types.h"

extern "C" {

extern u8   lbl_8048A8B4[];                     // 0x8048A8B4 (disc request queue)
extern u32  lbl_805F9690;                       // 0x805F9690 (-1: the invalid request handle)
}

// The three helpers below are inlined in every queue function of the original (the same code is in
// draft_pz2e.cpp for fn_801E4E20 and its neighbours).
// Queue slot allocation shared by fn_801E4E20 / fn_801E4FE0: pops the previous slot of the ring
// (head +0x10, tail +0x14, capacity +0x18, slots of 0x28 bytes at the pointer +0x1C), or NULL
// when the ring is full.
static u8* QueueTakeSlot(void) {
    u8* t = lbl_8048A8B4;
    s32 head = *(s32*)(t + 0x10);
    s32 tail = *(s32*)(t + 0x14);
    s32 cap = *(s32*)(t + 0x18);
    if (head == (tail + 1) % cap) {
        return NULL;
    }
    head -= 1;
    if (head < 0) {
        head = cap - 1;
    }
    tail = *(s32*)(t + 0x14);
    *(s32*)(t + 0x10) = head;
    if (tail < 0) {
        *(s32*)(t + 0x14) = head;
        *(s32*)(t + 0x8) = 0;
    }
    s32 ok = 0;
    if (head >= 0 && head < *(s32*)(t + 0x18)) {
        cap = *(s32*)(t + 0x18);
        s32 mid = head;
        s32 end = *(s32*)(t + 0x14);
        if (end < head) {
            if (head <= end) {
                mid = head + cap;
            }
            end += cap;
        }
        if (head <= mid && mid <= end) {
            ok = 1;
        }
    }
    if (ok == 0) {
        return NULL;
    }
    return *(u8**)(t + 0x1C) + head * 0x28;
}

// Initializes a taken slot (state 1, fresh generation) and returns its handle
// (generation << 7 | slot index); with no slot returns the invalid-handle sentinel.
static u32 QueueMakeHandle(u8* slot) {
    u8* t = lbl_8048A8B4;
    u32 handle = lbl_805F9690;
    if (slot != NULL) {
        *(s32*)(slot + 0x4) = 1;
        *(s32*)(slot + 0x20) = 0;
        *(s32*)(slot + 0x8) = 0;
        u32 gen = *(u32*)(t + 0x20);
        *(u32*)(slot + 0x24) = gen;
        *(u32*)(t + 0x20) = gen + 1;
        if (gen + 1 >= 0x01FFFFFFu) {
            *(u32*)(t + 0x20) = 0;
        }
        handle = (*(u32*)(slot + 0x24) << 7) | (u32)((s32)(slot - *(u8**)(t + 0x1C)) / 40);
    }
    return handle;
}

// Finds the slot of a handle in owner's table (+4), NULL when invalid or the generation differs.
static u8* QueueFindSlot(u8* owner, u32 handle) {
    if (handle + 0x10000 == 0xFFFF) {
        return NULL;
    }
    u8* slot = *(u8**)(owner + 0x4) + (handle & 0x7F) * 0x28;
    if ((handle >> 7) != *(u32*)(slot + 0x24)) {
        return NULL;
    }
    return slot;
}

// 0x801E4BE4
// Queues an "open" request (kind 0) for a file slot: stores the name at slot + 1 with '\' turned
// into '/', and the mode at slot + 0x101. Returns the request handle.
extern "C" s32 fn_801E4BE4(u8* queue, u8* file, const char* name, const char* mode) {
    u8* taken = QueueTakeSlot();
    u32 handle = QueueMakeHandle(taken);
    u8* slot = QueueFindSlot(queue, handle);
    if (slot != NULL) {
        *(u8**)(slot + 0xC) = file;
        *(s32*)(slot + 0x0) = 0;
        strcpy((char*)(file + 1), name);
        for (char* c = (char*)(file + 1); *c != 0; c++) {
            if (*c == 0x5C) {
                *c = 0x2F;
            }
        }
        strcpy((char*)(file + 0x101), mode);
    }
    return (s32)handle;
}
