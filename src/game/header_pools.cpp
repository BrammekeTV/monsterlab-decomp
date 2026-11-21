// The two ready-made object pools of global_binary.sir (header slots 0x10 and 0x418): giving
// every item in use back. fn_8012189C does the same after cleaning each item up.
// Original: 0x801214E4 and 0x801219F4 - 0x80121B08 (parts of auto_03_8012148C_text.s)
#include "types.h"

extern "C" {
void fn_801EBA20(void* list, void* item);   // 0x801EBA20: give an item back to its pool
}
extern u8 lbl_803C2A20[];                   // 0x803C2A20: the header block of global_binary.sir (sSirHeader)

// Gives every item of the pool's used list back and clears the counter that goes with the pool.
static inline void ReleaseAll(u32 poolSlot, u32 countSlot) {
    u8* pool = *(u8**)(lbl_803C2A20 + poolSlot);
    if (pool == NULL) return;
    u8* node = *(u8**)(pool + 0x14);
    u8* item = node != NULL ? node + 8 : NULL;
    while (item != NULL) {
        u8* nextNode = *(u8**)(item - 4);
        u8* next = nextNode != NULL ? nextNode + 8 : NULL;
        fn_801EBA20(*(void**)(lbl_803C2A20 + poolSlot), item);
        item = next;
    }
    *(s32*)(lbl_803C2A20 + countSlot) = 0;
}

// 0x801219F4
extern "C" void fn_801219F4(void) {
    ReleaseAll(0x10, 0x414);
    ReleaseAll(0x418, 0x81C);
}

// 0x801214E4
// A jump to fn_801219F4; the argument its callers pass is not used.
extern "C" void fn_801214E4(void* p) {
    (void)p;
    fn_801219F4();
}
