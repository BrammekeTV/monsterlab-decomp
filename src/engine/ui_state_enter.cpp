// UI state machine: entering a state. Runs the state's action, if it has one, and starts its timer.
// Original: 0x801BBFD0 (auto_03_801B9278_text.s)
#include "types.h"

extern "C" {
s32 TriggerDoAction(void* action, void* a1, void* a2);   // 0x801E0054
extern u8* lbl_805FB90C;                // 0x805FB90C: gStringTable2
}

// 0x801BBFD0
// `state` +0xC is an action reference (kind in bits 27-30, index in the low 18 bits), +0x18 a
// duration. `machine` +0 points at its definition (string table at +0xC), +0x10 is the target,
// +0x54 an optional table {count, words...} that kind 3 looks its index up in.
extern "C" void fn_801BBFD0(u8* state, u8* machine) {
    u32 ref = *(u32*)(state + 0xC);
    if ((ref >> 27) & 0xF) {
        lbl_805FB90C = *(u8**)(*(u8**)machine + 0xC);
        ref = *(u32*)(state + 0xC);
        if (((ref >> 27) & 0xF) == 3) {
            u32* table = *(u32**)(machine + 0x54);
            u32 index = ref & 0x3FFFF;
            if (table != NULL && index < table[0]) {
                ref = table[index + 1];
            } else {
                ref = 0x3FFFF;
            }
        }
        u32 action = ref;
        TriggerDoAction(&action, *(void**)(machine + 0x10), NULL);
    }
    f32 t = *(f32*)(state + 0x18);
    if (t >= 0.0f) {
        *(f32*)(machine + 0x4C) = t;
        *(f32*)(machine + 0x50) = t;
    }
}
