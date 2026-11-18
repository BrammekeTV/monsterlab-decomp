// More 0x800Fxxxx script callbacks (junctions, parts, quests, NIS camera).
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8   gGame[];                // 0x803B53A0
extern u8   gWorldState[];          // 0x803E6D00 (holds a pointer to the world state)
extern u8   lbl_803C4298[];         // 0x803C4298 (level info block)
extern u8   lbl_805FBBE0[];         // 0x805FBBE0 (holds the save/session pointer)
extern u8   lbl_805FAE8C[];         // 0x805FAE8C (global word, cleared in SetupGame)
extern u8   lbl_805FB0E1[];         // 0x805FB0E1 (global byte, cleared in StopNISFSM)
extern s32  lbl_805FB970;           // 0x805FB970
extern void* lbl_805FB964;          // 0x805FB964 (network object)
extern s32  lbl_805FB974;           // 0x805FB974
extern "C++" void* gEventMgr;             // 0x805FB6C8 (ai_states.cpp)
extern "C++" void* gScreenSets;           // 0x805FB654 (small_8019.cpp)

// ScopedLock is a 4-byte RAII guard (see props.cpp / world_update.cpp).
struct ScopedLock { u8 pad[4]; };

extern "C" {
u32   fn_8010720C(s32 a0);                                  // 0x8010720C
s32   WorldGetPhase(void* world);                           // 0x800D83A8
s32   PathGetNodeId(void* path);                            // 0x800D56F8
s32   PathIsAtJunction(PathFollower* p);                    // 0x800D5924
s32   fn_80126354(void);                                    // 0x80126354
void  fn_800D7FF0(u8* world, s32 a1);                       // 0x800D7FF0
void  fn_800E6C60(void);                                    // 0x800E6C60
void  SetConversationReady(s32 a0);                         // 0x80051EB8
void  SaveResetAlias(u8* s);                                // 0x802222FC
void  ScopedLockCtor(ScopedLock* l);                        // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);             // 0x801F53B0
void* EnhancementGetTexture(s32 enhancement);               // 0x80116870
void  SaveSetActiveQuest(u8* s, s32 x, s32 q);              // 0x802229A8
s32   fn_800517C0(void);                                    // 0x800517C0
void  fn_800D81AC(void* w);                                 // 0x800D81AC
s32   fn_80222994(u8* s);                                   // 0x80222994
void  SaveEndQuest(u8* s, BOOL abandon);                    // 0x802229D8
void  fn_800D8280(void* w);                                 // 0x800D8280
u32   fn_801EC880(u8* p, u32 hash);                         // 0x801EC880
void  fn_801ECDE8(u8* p, s32 idx, u32 a, s32 immediate);    // 0x801ECDE8
u8    fn_8004936C(void);                                    // 0x8004936C
void  fn_80228AFC(u8* s, s32 a1);                           // 0x80228AFC
void  fn_8022C2BC(u8* s, s32 a1);                           // 0x8022C2BC
void* ScreenSetsCurrent(void* sets);                        // 0x8018D1E4
void* ScreenSetFindElement(void* set, u32 hash);            // 0x80198F18
void  SessionResetUnk204(u8* s);                            // 0x80013684
void* SpawnHelper(void* owner, s32 arg);                    // 0x8000C478
s32 fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6); // 0x80065200
u8*   fn_800E4D7C(u32 a0);                                  // 0x800E4D7C
u8*   fn_800E4BFC(u8* p);                                   // 0x800E4BFC (returns a value here; small_800Eb.cpp declares void)
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190
}

// 0x800F0054
// True when the given level is the one loaded now and the level info block says it is a
// level of type 5 or 7 (the level-type check at +0x18).
BOOL IsCurrentLevelLoaded(u32 a0) {
    u32 cur = fn_8010720C(lbl_803C4298[0x1c]);
    if (a0 != cur) return 0;
    s32 type = *(s32*)(lbl_803C4298 + 0x18);
    return (type == 5 || type == 7) ? 1 : 0;
}

// 0x800F00EC
// Spawns a projectile from the given owner, aimed along the owner's facing axis (Y).
void SpawnProjectile(void* a0, u32 a1) {
    u8* p = (u8*)a0;
    Quat* q = (Quat*)(*(u8**)(p + 4) + 0xC);
    Vec3 dir = QuatGetAxisY(q);
    void* spawned = SpawnHelper((void*)(uintptr_t)a1, 0);
    if (spawned != 0) {
        fn_80065200((u8*)spawned, p + 0x18, &dir, p, 0, 0, 0);
    }
}

// 0x800F050C
// Returns 1 when the junction the player is on (or the current path node) equals a0.
s32 IsCurrentJunctionID(u32 a0) {
    s32 node = 0;
    s32 phase = *(s32*)(*(u8**)gWorldState + 0x1E0);
    if (phase == 1) {
        node = fn_80126354();
    } else if (phase == 2) {
        Character* player = *(Character**)(gGame + 0xD614);
        if (player->path != 0) {
            node = PathGetNodeId(player->path);
        }
    }
    return (u32)node == a0;
}

// 0x800F0ADC
// Returns whether the player's path is currently standing at a junction (only during phase 2).
u32 IsCurrentJunctionActive(void) {
    if (WorldGetPhase(gGame + 0xD5F0) != 2) return 0;
    Character* player = *(Character**)(gGame + 0xD614);
    if (player->path == 0) return 0;
    return (u32)PathIsAtJunction(player->path);
}

// 0x800F19A4
// Returns 1 when the player's path is at a waypoint (only during phase 2).
s32 IsPlayerAtJunction(void) {
    s32 at = 0;
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        Character* player = *(Character**)(gGame + 0xD614);
        if (PathAtWaypoint(player->path)) at = 1;
    }
    return at;
}

// 0x800F1A24
// Resets the game-side state at the start of a game session.
void SetupGame(void) {
    fn_800D7FF0(gGame + 0xD5F0, 0);
    fn_800E6C60();
    SetConversationReady(0);
    u8* save = *(u8**)lbl_805FBBE0;
    if (save != 0) {
        SaveResetAlias(save);
    }
    *(s32*)lbl_805FAE8C = 0;
    *(s32*)(gGame + 0xFE0) = 0;
}

// 0x800F1B2C
// Deactivates an entity: runs its vtable slot 0x18 under a scoped lock, then flags its
// owner block (+0x20 bit 0x80) as inactive.
void DeactivateEntity(void* a0) {
    if (a0 == 0) return;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    u8* obj = (u8*)a0;
    ((void(*)(void*, s32))(*(void**)(*(u8**)obj + 0x18)))(obj, 0);
    ScopedLockDtor(&lock, -1);
    u8* owner = *(u8**)(obj + 4);
    *(u32*)(owner + 0x20) |= 0x80;
}

// 0x800F1CD4
// Part definition has discipline 0 (Mech). False for indices outside the part table.
s32 PartIsMech(s32 a0) {
    if (a0 < 0 || a0 >= 0xBA) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, a0);
    return def.discipline == 0;
}

// 0x800F1D4C
// Part definition has discipline 1 (Bio). False for indices outside the part table.
s32 PartIsBio(s32 a0) {
    if (a0 < 0 || a0 >= 0xBA) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, a0);
    return def.discipline == 1;
}

// 0x800F1DC4
// Part definition has discipline 2 (Alchemy). False for indices outside the part table.
s32 PartIsAlchemy(s32 a0) {
    if (a0 < 0 || a0 >= 0xBA) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, a0);
    return def.discipline == 2;
}

// 0x800F40F0
// Returns the enhancement texture of a part definition (0 if the part has no enhancement).
// Note: the header lists this as BOOL, but the original returns the texture pointer.
ScriptValue PartGetEnhancementTexture(s32 a0) {
    if (a0 < 0 || a0 >= 0xBA) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, a0);
    if (def.unk20 == -1) return 0;
    return (ScriptValue)EnhancementGetTexture(def.unk20);
}

// 0x800F48D4
// Starts a quest: records it as the active quest and, if no quest is running and the
// flag at +0x430 is clear, runs the quest-start step on the world.
void StartQuest(u32 a0, u32 a1) {
    *(u32*)(gGame + 0xD5F0) = a0;
    SaveSetActiveQuest(*(u8**)lbl_805FBBE0, a0, a1);
    if (fn_800517C0() == 0) {
        u8* hud = *(u8**)(gGame + 0xD634);
        if (*(u8*)(hud + 0x430) == 0) {
            fn_800D81AC(gGame + 0xD5F0);
        }
    }
}

// 0x800F4940
// Ends the active quest if fn_80222994 reports one (not -1). Marks the quest-end flag at +0x430.
void EndQuest(void) {
    if (fn_80222994(*(u8**)lbl_805FBBE0) == -1) return;
    SaveEndQuest(*(u8**)lbl_805FBBE0, 0);
    *(s32*)(gGame + 0xD620) = 0;
    fn_800D8280(gGame + 0xD5F0);
    u8* hud = *(u8**)(gGame + 0xD634);
    *(u8*)(hud + 0x430) = 1;
}

// 0x800F4E00
// Removes one specific attached particle effect from the entity at +0x94, if it is found.
void RemoveSpecificAttachedParticleEffectAtEmitter(void* a0, u32 a1, u32 a2) {
    if (a0 == 0) return;
    u8* p = (u8*)a0 + 0x94;
    s32 idx = (s32)fn_801EC880(p, a2);
    if (idx >= 0) {
        fn_801ECDE8(p, idx, a1, 0);
    }
}

// 0x800F5A78
// Sets the field-repair flag on the save (+0xA1C). When it is set, starts the repair
// presentation; otherwise, if the repair is allowed in phase 6, runs the follow-up step.
void SetPerformFieldRepair(s32 a0) {
    s32 flag = (a0 != 0) ? 1 : 0;
    u8* save = *(u8**)lbl_805FBBE0;
    *(s32*)(save + 0xA1C) = flag;
    if (flag != 0) {
        fn_80228AFC(save, 1);
        return;
    }
    if (fn_8004936C() != 0) {
        if (WorldGetPhase(gGame + 0xD5F0) == 6) {
            fn_8022C2BC(*(u8**)lbl_805FBBE0, 1);
        }
    }
}

// 0x800F5E70
// Shows the fight text element when the current screen set has the matching id (0x7BD22420).
void VSHACKSetFightTextVisible(void) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == 0) return;
    if (*(u32*)set != 0x7BD22420u) return;
    void* el = ScreenSetFindElement(set, 0x57A5DDB3u);
    if (el == 0) return;
    u8* vt = *(u8**)((u8*)el + 0x138);
    ((void(*)(void*, s32))(*(void**)(vt + 0x2C)))(el, 2);
}

// 0x800F5FDC
// Returns 1 when a network session is active and its object reports multiplayer (slot 0x34).
u32 IsMultiPlayer(void) {
    u32 result = 0;
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != 0) {
        void* obj = lbl_805FB964;
        s32 r = ((s32(*)(void*))(*(void**)(*(u8**)obj + 0x34)))(obj);
        if (r != 0) result = 1;
    }
    return result;
}

// 0x800F6574
// Sends an event to the NIS camera. The event id is a0; a1 is not read by the original.
void SendEventToNISCamera(u32 a0, u32 a1) {
    (void)a1;
    u8* p1 = fn_800E4D7C(a0);
    u8* x = fn_800E4BFC(p1);
    u8* y = fn_800E4BFC(p1);
    EventSend(gEventMgr, y, x, a0, 0, 0, 1, 0);
}

// 0x800F6A8C
// Freezes or unfreezes the player. Only acts when the player's flags have bit 0x2000 set;
// then pauses the player's path (a0 != 0) and plays the freeze animation.
void SetFreezePlayer(u32 a0) {
    Character* player = *(Character**)(gGame + 0xD614);
    u8* def = *(u8**)((u8*)player + 4);
    if ((*(u32*)(def + 0x1C) & 0x2000) != 0) {
        PathSetPaused(player->path, (a0 != 0) ? 1 : 0);
        PlayCharacterAnimation((u32)(uintptr_t)player, 1, 0xE3712F44u, 1);
    }
}

// 0x800F6CE0
// Stops the NIS state machine: clears the NIS flags, resets the session, shows the HUD
// and, in phase 2, unpauses the exploration characters.
void StopNISFSM(void) {
    u8* hud = *(u8**)(gGame + 0xD634);
    *(u8*)(hud + 0x430) = 1;
    *(u8*)lbl_805FB0E1 = 0;
    SessionResetUnk204(gGame + 0x5C8);
    ConversationShowHUDInfo();
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        UnPauseAllExplorationCharacters();
    }
}
