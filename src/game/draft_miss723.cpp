// Small functions in 0x800C924C-0x800CBD4C: queue drain and nearest-item search over the
// ring buffers at +0x39C (50-slot table) and +0x464..+0x47C.
#include "types.h"

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
void  fn_800C1D44(u8* p);                                        // 0x800C1D44
void  fn_800C72C4(u8* item, u8* other);                          // 0x800C72C4
u8**  fn_800CBFC8(u8* p, s32 idx);                               // 0x800CBFC8 (ring slot lookup, NULL if none)
}

extern "C" {
extern f32 lbl_805FE4B8;  // 0x805FE4B8 (float constant, "no item" distance and position)

// Returns the item stored in ring slot idx of the queue at p+0x464, or NULL.
static u8* RingItem(u8* p, s32 idx) {
    u8** slot = fn_800CBFC8(p + 0x464, idx);
    return slot ? *slot : NULL;
}

// Pops the head of the ring queue (tail +0x46C, head +0x470, capacity +0x474, table +0x478, last slot +0x47C).
// Returns the popped item, or NULL when the queue is empty or the head is invalid.
static u8* RingPop(u8* p) {
    s32 tail = *(s32*)(p + 0x46c);
    s32 head = *(s32*)(p + 0x470);
    s32 cap = *(s32*)(p + 0x474);
    void** slot = NULL;
    u8* item;
    if (tail < 0 || head < 0) {
        return NULL;
    }
    if (head >= 0 && head < cap) {
        s32 eff = (head < tail) ? head + cap : head;
        if (tail <= eff) {
            slot = (void**)*(void***)(p + 0x478) + head;
        }
    }
    *(void***)(p + 0x47c) = slot;
    if (slot == NULL) {
        return NULL;
    }
    item = (u8*)*slot;
    *slot = NULL;
    if (head == tail) {
        *(s32*)(p + 0x470) = -1;
        *(s32*)(p + 0x46c) = -1;
    } else {
        s32 next = head - 1;
        if (next < 0) {
            next = cap - 1;
        }
        *(s32*)(p + 0x470) = next;
    }
    return item;
}

// 0x800C924C
// Clears the 50-slot table at +0x39C (calling fn_800C72C4 on each item still in it), then
// pops and frees every item left in the ring queue. arg1 is forwarded unchanged (leftover r4).
void fn_800C924C(u8* arg0, u8* arg1) {
    void** slots = (void**)(arg0 + 0x39c);
    s32 i;
    u8* item;

    fn_800C1D44(arg0);
    for (i = 0; i < 50; ++i) {
        if (slots[i] != NULL) {
            fn_800C72C4((u8*)slots[i], arg1);
            slots[i] = NULL;
        }
    }
    item = RingPop(arg0);
    while (item != NULL) {
        operator_delete(item);
        item = RingPop(arg0);
    }
}

// 0x800CBD4C
// Finds the unflagged item (flag byte at +0x8) in the ring queue closest to the point arg1,
// writes its position (+0xC..+0x14) to arg2 and returns 1. Returns 0 if the queue is empty.
// With no unflagged item, arg2 receives the default constant and the result is still 1.
s32 fn_800CBD4C(u8* arg0, f32* arg1, f32* arg2) {
    s32 head = *(s32*)(arg0 + 0x470);
    s32 tail = *(s32*)(arg0 + 0x46c);
    s32 count = 0;
    s32 first = -1;
    f32 bestDist = lbl_805FE4B8;
    f32 bestPos[3];
    s32 i;

    if (tail >= 0 && head >= 0) {
        s32 eff = head;
        if (head < tail) {
            eff += *(s32*)(arg0 + 0x474);
        }
        count = eff - tail + 1;
    }
    if (count <= 0) {
        return 0;
    }

    bestPos[0] = lbl_805FE4B8;
    bestPos[1] = lbl_805FE4B8;
    bestPos[2] = lbl_805FE4B8;

    for (i = 0; i < count; ++i) {
        u8* item = RingItem(arg0, i);
        f32 dx;
        f32 dy;
        f32 dz;
        f32 d;
        if (*(u8*)(item + 0x8) != 0) {
            continue;
        }
        dx = arg1[0] - *(f32*)(item + 0xc);
        dy = arg1[1] - *(f32*)(item + 0x10);
        dz = arg1[2] - *(f32*)(item + 0x14);
        d = (dz * dz) + ((dx * dx) + (dy * dy));
        if (first == -1) {
            first = i;
            bestPos[0] = *(f32*)(item + 0xc);
            bestPos[1] = *(f32*)(item + 0x10);
            bestPos[2] = *(f32*)(item + 0x14);
            bestDist = d;
        } else if (d < bestDist) {
            first = i;
            bestDist = d;
            bestPos[0] = *(f32*)(item + 0xc);
            bestPos[1] = *(f32*)(item + 0x10);
            bestPos[2] = *(f32*)(item + 0x14);
        }
    }

    arg2[0] = bestPos[0];
    arg2[1] = bestPos[1];
    arg2[2] = bestPos[2];
    return 1;
}

}  // extern "C"
