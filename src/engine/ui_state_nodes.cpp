// UI state machines: the methods of the state and node classes that run an action word.
// A state machine (FSM) definition holds states and nodes; fn_801BC5F0 installs their vtables
// by the type word at +8 (1 state, 3 action node, 4 choice node). Layouts: docs/loader.md.
// Original: 0x801BBF24 - 0x801BC34C (parts of auto_03_801B9278_text.s)
#include "types.h"

extern "C" {
s32 TriggerDoAction(void* action, void* a1, void* a2);   // 0x801E0054
}
extern void* gStringTable2;   // 0x805FB90C

// An action word has a type in bits 27-30 (0 = none) and a value in the low 18 bits. Type 3 takes
// the word to run from the machine's value table at +0x54 ({count, words}); an index outside the
// table gives the empty word 0x3FFFF. The action runs on the machine's owner (+0x10).
static inline BOOL ActionWordSet(u32 word) {
    return ((word >> 27) & 0xF) != 0;
}

static inline s32 RunActionWord(u32 word, u8* machine) {
    gStringTable2 = *(void**)(*(u8**)machine + 0xC);
    if (((word >> 27) & 0xF) == 3) {
        u32* values = *(u32**)(machine + 0x54);
        u32 index = word & 0x3FFFF;
        if (values != NULL && index < values[0]) word = values[index + 1];
        else                                     word = 0x3FFFF;
    }
    u32 action = word;
    return TriggerDoAction(&action, *(void**)(machine + 0x10), NULL);
}

// 0x801BBF24
// Action node (type 3), entered: hands its two parameters to the machine, then runs its action.
extern "C" void fn_801BBF24(u8* node, u8* machine) {
    s32 second = *(s32*)(node + 0x18);
    *(s32*)(machine + 0x1C) = *(s32*)(node + 0x14);
    *(s32*)(machine + 0x20) = second;
    if (ActionWordSet(*(u32*)(node + 0xC))) RunActionWord(*(u32*)(node + 0xC), machine);
}

// 0x801BC12C
// State, left: runs its exit action.
extern "C" void fn_801BC12C(u8* state, u8* machine) {
    if (ActionWordSet(*(u32*)(state + 0x14))) RunActionWord(*(u32*)(state + 0x14), machine);
}

// 0x801BC20C
// Choice node (type 4), entered: runs its action and keeps the result, which picks the way out.
extern "C" void fn_801BC20C(u8* node, u8* machine) {
    s32 result = 0;
    if (ActionWordSet(*(u32*)(node + 0xC))) result = RunActionWord(*(u32*)(node + 0xC), machine);
    *(s32*)(node + 0x1C) = result;
}

// 0x801BC2B8
// Choice node: the transition for the kept result (0 when the result is out of range), or the
// first transition there is when that one is missing. NULL for a node without transitions.
extern "C" u8* fn_801BC2B8(u8* node) {
    s32 count = *(s32*)(node + 0x14);
    if (count == 0) return NULL;

    s32 choice = *(s32*)(node + 0x1C);
    if (choice < 0 || choice >= count) *(s32*)(node + 0x1C) = 0;
    choice = *(s32*)(node + 0x1C);
    u8** transitions = *(u8***)(node + 0x10);
    u8* next = transitions[choice];
    if (next == NULL) {
        for (s32 i = 0; i < count; i++) {
            if (transitions[i] != NULL) {
                next = transitions[i];
                break;
            }
        }
    }
    *(s32*)(node + 0x20) = choice;
    *(s32*)(node + 0x1C) = -1;
    return next;
}
