// Small functions in 0x800Fxxxx (exploration script callbacks).
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8 gGame[];                 // 0x803B53A0
extern u8 gWorldState[];           // 0x803E6D00
extern u8 lbl_803C2A20[];          // holds the register-table pointer at +0xD98
extern u8 lbl_803C4298[];          // level info block
extern u8 lbl_80360334[];          // level-name log format string
extern u8 lbl_805FBBE0[];          // inventory block pointer
extern u8 lbl_805FEB0C[];          // float constant

extern "C" {
void  GameLog(int level, int category, const char* fmt, ...);  // 0x8000B2F0
void  fn_800D8114(u8* world);                                  // 0x800D8114
void  fn_800D36A0(void);                                       // 0x800D36A0
void  fn_800D6DDC(void);                                       // 0x800D6DDC
void  SessionResetSlots(u8* s);
void* CharacterFind(s32 id);                                   // 0x800E876C
s32   WorldGetPhase(void* world);                              // 0x800D83A8
s32   PathGetNodeId(void* path);                               // 0x800D56F8
s32   fn_80126354(void);                                       // 0x80126354

// 0x800F00BC
// Logs the current level name (the level-name check is never taken as written).
void PrintCurrentLevelName(void) {
    u32 v = lbl_803C4298[0x1c];
    if (v <= 0xff) return;
    GameLog(4, 3, (const char*)lbl_80360334);
}

// Shared helper: obj->0xA4 -> ptr -> *ptr is the register table base (0 if missing).
static u8* RegBase(u8* obj) {
    if (obj == 0) return 0;
    u8* p = *(u8**)(obj + 0xA4);
    if (p == 0) return 0;
    return *(u8**)p;
}

// 0x800F0194
// Returns global register idx (0..3) from the table at lbl_803C2A20 + 0xD98, or 0.
s32 get_reg(BOOL a0) {
    if ((u32)a0 >= 4) return 0;
    u8* base = *(u8**)(lbl_803C2A20 + 0xD98);
    if (base == 0) return 0;
    return *(s32*)(base + a0 * 4 + 0x2C);
}

// 0x800F01D0
// Decrements object register idx (0..3).
void decr_reg(void* a0, BOOL a1) {
    if ((u32)a1 >= 4) return;
    u8* base = RegBase((u8*)a0);
    if (base == 0) return;
    *(s32*)(base + a1 * 4 + 0x2C) -= 1;
}

// 0x800F0248
// Returns object register 0.
s32 get_reg0(void* a0) {
    u8* base = RegBase((u8*)a0);
    if (base == 0) return 0;
    return *(s32*)(base + 0x2C);
}

// 0x800F0280
// Returns 1 if object register 0 is greater than 0, else 0.
s32 reg0_gt_0(void* a0) {
    u8* base = RegBase((u8*)a0);
    if (base == 0) return 0;
    s32 v = *(s32*)(base + 0x2C);
    return v > 0 ? 1 : 0;
}

// 0x800F02C4
// Decrements object register 0.
void decr_reg0(void* a0) {
    u8* base = RegBase((u8*)a0);
    if (base == 0) return;
    *(s32*)(base + 0x2C) -= 1;
}

// 0x800F04C0
// Returns the junction id: from fn_80126354 in state 1, or the player's path node in state 2.
u32 GetCurrentJunctionID(void) {
    u8* w = *(u8**)gWorldState;
    s32 state = *(s32*)(w + 0x1E0);
    if (state == 1) return (u32)fn_80126354();
    if (state != 2) return 0;
    u8* player = *(u8**)(gGame + 0xD614);
    u32 path = *(u32*)(player + 0xA00);
    if (path == 0) return 0;
    return (u32)PathGetNodeId((void*)path);
}

// 0x800F0F6C
// Tears down exploration: world, session slots and the float at 0x7CC.
void ExplorationShutdown(void) {
    fn_800D8114(gGame + 0xD5F0);
    fn_800D36A0();
    fn_800D6DDC();
    SessionResetSlots(gGame + 0x5C8);
    *(f32*)(gGame + 0x7CC) = *(f32*)lbl_805FEB0C;
}

// 0x800F1044
// Sends a character to junction a1 if its flag bit 0x10 is set.
void MoveEnemyToJunction(u32 a0, u32 a1) {
    u8* c = (u8*)CharacterFind((s32)a0);
    if (c == 0) return;
    u8* info = *(u8**)(c + 4);
    if (*(u32*)(info + 0x1C) & 0x10) {
        *(s32*)(c + 0x87C) = 3;
        *(u32*)(c + 0x888) = a1;
    }
}

// 0x800F1094
// Sends a character to the player if its flag bit 0x10 is set.
void MoveEnemyToPlayer(u32 a0) {
    u8* c = (u8*)CharacterFind((s32)a0);
    if (c == 0) return;
    u8* info = *(u8**)(c + 4);
    if (*(u32*)(info + 0x1C) & 0x10) {
        *(s32*)(c + 0x87C) = 3;
        *(u32*)(c + 0x888) = 0;
    }
}

// 0x800F1778
// Returns 1 if (a0, a1) matches the current junction entry pair at +0x14/+0x18.
s32 CheckJunctionEnterEntity(u32 a0, u32 a1) {
    u8* s = *(u8**)(*(u8**)(gGame + 0xD634) + 0x434);
    return (a0 == *(u32*)(s + 0x14) && a1 == *(u32*)(s + 0x18)) ? 1 : 0;
}

// 0x800F186C
// Reverses the player's path while the world is in phase 2.
void ReversePlayerDirection(void) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* player = *(u8**)(gGame + 0xD614);
        PathReverse((PathFollower*)*(u8**)(player + 0xA00));
    }
}

// 0x800F194C
// Stores the confusion value a0 on the player while the world is in phase 2.
void SetPlayerConfused(u32 a0) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* player = *(u8**)(gGame + 0xD614);
        *(u32*)(player + 0x9D4) = a0;
    }
}

// 0x800F1FC0
// Returns the madness of the inventory part at index a0, or 0 if out of range or empty.
s32 InventoryGetPartMadness(s32 a0) {
    u8* inv = *(u8**)lbl_805FBBE0;
    if (a0 < 0 || a0 >= *(s32*)(inv + 0x10EC)) return 0;
    u8* p = *(u8**)(inv + a0 * 4 + 0xA58);
    if (p == 0) return 0;
    return (s32)PartGetMadness((const PartInstance*)p);
}

// 0x800F2018
// Returns the quality of the inventory part at index a0, or 0 if out of range or empty.
s32 InventoryGetPartQuality(s32 a0) {
    u8* inv = *(u8**)lbl_805FBBE0;
    if (a0 < 0 || a0 >= *(s32*)(inv + 0x10EC)) return 0;
    u8* p = *(u8**)(inv + a0 * 4 + 0xA58);
    if (p == 0) return 0;
    return (s32)PartGetQuality((const PartInstance*)p);
}
}
