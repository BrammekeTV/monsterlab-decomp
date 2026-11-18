// Exploration script callbacks in 0x800F0FBC - 0x800F1B28 (castle goto, entity movement AI,
// player teleport / junction distance, move direction, activate entity).
#include <string.h>
#include "game/character.h"
#include "game/game.h"

extern "C" {
// script_callbacks.h is not included: its PlayCharacterAnimation takes u32 for the Character*.
void  PlayCharacterAnimation(Character* c, int a, u32 anim, int loop);
void  GameFlowSignal(s32 id, s32 arg1, s32 arg2);
void  CharacterResetRace(Character* c);                                   // 0x800534C0
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  ExplorationPromptClear(BOOL animate);                              // 0x80016EA0
u8*   ObjDbLookup(void* db, s32 id);
u8*   ObjRefDtor(u8* ref, s32 flags);
struct ScopedLock { u8 pad[4]; };
void  ScopedLockCtor(ScopedLock* l);                                  // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                          // 0x801F53B0
s32   WorldGetPhase(void* world);                                        // 0x800D83A8
s32   fn_80053418(u8* ent);                                              // 0x80053418
void  fn_8005353C(u8* ent, s32 on);                                      // 0x8005353C
s32   fn_80053550(u8* ent, s32 a);                                       // 0x80053550
s32   fn_800D3C60(void* db, void* path, f32 a, f32 b);                  // 0x800D3C60
u8*   fn_800E4D7C(u32 a0);                                               // 0x800E4D7C
void* fn_800E60BC(void);                                                 // 0x800E60BC
BOOL  fn_800E6BD4(void);
s32   fn_801D5F6C(void);                                                 // 0x801D5F6C

extern void* gEventMgr;     // 0x805FB6C8
extern u8    gGame[];       // 0x803B53A0
extern void* gObjDb;        // 0x805FB180
extern u8    lbl_805FB0E1;  // 0x805FB0E1
extern u8    lbl_805FB1A1;  // 0x805FB1A1
extern u8    lbl_805FBBE0[];  // 0x805FBBE0 (save block pointer)

// Player entity pointer lives at gGame + 0xD614.
#define PLAYER_ENT() (*(u8**)(gGame + 0xD614))

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtGuard(f32 v) {
    return v <= 0.0f ? 0.0f : sqrtf(v);
}

// Copies the 0x404-byte object reference record for a junction into a stack buffer.
static inline void CopyObjRef(u8* dst, const u8* src) {
    *(s32*)dst = *(const s32*)src;
    memcpy(dst + 4, src + 4, 0x3F8);
    *(s32*)(dst + 0x3FC) = *(const s32*)(src + 0x3FC);
    *(s32*)(dst + 0x400) = *(const s32*)(src + 0x400);
}

// Sets the player's entity flags for a teleport (0x8000|0x10000000|0x4000|0x10000000).
static inline void MarkTeleported(u8* player) {
    *(u32*)(*(u8**)(player + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(player + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(player + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(player + 0x4) + 0x20) |= 0x10000000;
}

// 0x800F0FBC
// Stores the castle location in the save block and signals the game flow by world phase.
void GotoCastleLocation(u32 /*0x4B*/ a0) {
    *(u32*)(*(u8**)lbl_805FBBE0 + 0x104) = a0;
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        GameFlowSignal(0x06B0A31C, 0, 0);
        return;
    }
    if (WorldGetPhase(gGame + 0xD5F0) == 1) {
        GameFlowSignal(0x1D45B14E, 0, 0);
    }
}

// 0x800F10DC
// Switches an entity's NIS move mode on/off and sends the matching event.
void SetEntityMoveInNIS(void* a0, u32 /*0x06*/ a1) {
    u8* ent = (u8*)a0;
    if (ent != NULL && (*(s32*)(*(u8**)(ent + 0x4) + 0x1C) & 0x10)) {
        if ((s32)a1 != 0) {
            fn_8005353C(ent, 1);
            u8* path = *(u8**)(ent + 0xA00);
            if (lbl_805FB0E1 != 0) {
                if (path != NULL && PathAtWaypoint((PathFollower*)path) != 0) {
                    EventSend(gEventMgr, ent, NULL, 0x33EE55C6u, 0, 0, 0, 0);
                    return;
                }
                EventSend(gEventMgr, ent, NULL, 0xF4D7ABu, 0, 0, 0, 0);
            }
        } else {
            fn_8005353C(ent, 0);
            if (lbl_805FB0E1 != 0) {
                EventSend(gEventMgr, ent, NULL, 0x4F9213C1u, 0, 0, 0, 0);
            }
        }
    }
}

// 0x800F1204
// Sets an entity's AI mode (+0x87C), sub-mode (+0x880) and parameter (+0x888) from a command hash.
void SetEntityMovementAI(void* a0, u32 a1, u32 a2) {
    u8* ent = (u8*)a0;
    if (ent != NULL && (*(s32*)(*(u8**)(ent + 0x4) + 0x1C) & 0x10)) {
        switch ((s32)a1) {
        case 0x1C765CF4:
            *(s32*)(ent + 0x87C) = 0;
            *(s32*)(ent + 0x888) = 0;
            return;
        case (s32)0xE6BE4FBE:
            *(s32*)(ent + 0x87C) = 1;
            *(s32*)(ent + 0x880) = 3;
            *(s32*)(ent + 0x888) = 0;
            return;
        case (s32)0xB7DEA2AC:
            *(s32*)(ent + 0x87C) = 3;
            *(s32*)(ent + 0x888) = 0;
            return;
        case 0x655CDA89:
            *(u32*)(ent + 0x888) = a2;
            *(s32*)(ent + 0x87C) = 4;
            return;
        case (s32)0x90E9F6B6:
            *(u32*)(ent + 0x888) = a2;
            *(s32*)(ent + 0x87C) = 3;
            return;
        case (s32)0xF2DBEE2B:
            CharacterResetRace((Character*)ent);
            *(s32*)(ent + 0x880) = 0;
            *(s32*)(ent + 0x87C) = 1;
            return;
        case 0x10A62FEB:
            CharacterResetRace((Character*)ent);
            *(s32*)(ent + 0x880) = 2;
            *(s32*)(ent + 0x87C) = 1;
            return;
        case 0x662A3DA3:
            CharacterResetRace((Character*)ent);
            *(s32*)(ent + 0x880) = 1;
            *(s32*)(ent + 0x87C) = 1;
            break;
        }
    }
}

// 0x800F139C
// Distance (truncated to int) from the player to the position of the given junction.
s32 GetPlayerDistanceToJunction(u32 a0) {
    s32 result = 0;
    u8* player = PLAYER_ENT();
    u8 ref[0x404];
    u8* src = ObjDbLookup(gObjDb, (s32)a0);
    CopyObjRef(ref, src);
    if (player != NULL && WorldGetPhase(gGame + 0xD5F0) == 2) {
        Vec3* pos = PathGetPosition((PathFollower*)ref);
        f32* pp = *(f32**)(player + 0x4);
        f32 dx = pp[0] - pos->x;
        f32 dy = pp[1] - pos->y;
        f32 dz = pp[2] - pos->z;
        result = (s32)SqrtGuard(dz * dz + (dx * dx + dy * dy));
    }
    ObjRefDtor(ref, -1);
    return result;
}

// 0x800F1504
// Teleports the player to (x, y, z) while exploring and flags it as teleported.
void TeleportPlayerTo(f32 a0, f32 a1, f32 a2) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* player = PLAYER_ENT();
        f32* pp = *(f32**)(player + 0x4);
        pp[0] = a0;
        pp[1] = a1;
        pp[2] = a2;
        MarkTeleported(player);
        fn_800D3C60(gObjDb, *(void**)(player + 0xA00), a1, a0);
        fn_80053550(player, 0);
        fn_80053418(player);
    }
}

// 0x800F1610
// Teleports the player to a junction's position and plays the idle animation.
void TeleportPlayerToJunction(u32 a0) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* player = PLAYER_ENT();
        if (player != NULL && WorldGetPhase(gGame + 0xD5F0) == 2) {
            u8 ref[0x404];
            u8* src = ObjDbLookup(gObjDb, (s32)a0);
            CopyObjRef(ref, src);
            Vec3* pos = PathGetPosition((PathFollower*)ref);
            f32 x = pos->x, y = pos->y, z = pos->z;
            f32* pp = *(f32**)(player + 0x4);
            pp[0] = x;
            pp[1] = y;
            pp[2] = z;
            MarkTeleported(player);
            fn_800D3C60(gObjDb, *(void**)(player + 0xA00), y, x);
            fn_80053550(player, 0);
            fn_80053418(player);
            PlayCharacterAnimation((Character*)player, 1u, 0xE3712F44u, 0u);   // "idlereg"
            ObjRefDtor(ref, -1);
        }
    }
}

// 0x800F17B4
// True when no game event / cutscene / conversation is active.
u32 /*0x06*/ CheckNoGameEventsActive(void) {
    u32 result = 1;
    u8* a = fn_800E4D7C(0);   // asm passes caller's leftover r3 (callback has no args)
    u8* b = (u8*)fn_800E60BC();
    if (fn_800E6BD4() != 0 || fn_801D5F6C() == 0 ||
        (a != NULL && *(u8*)(a + 0x54) != 0 && WorldGetPhase(gGame + 0xD5F0) != 6) ||
        (b != NULL && *(u8*)(b + 0xA4) != 0) || lbl_805FB1A1 != 0) {
        result = 0;
    }
    return result;
}

// 0x800F18BC
// Makes the player's path follower take an exit, restores speed and plays the explore anim.
void MovePlayerDirection(BOOL a0) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* player = PLAYER_ENT();
        PathTakeExit(*(PathFollower**)(player + 0xA00), a0, 1);
        PathSetSpeed(*(PathFollower**)(player + 0xA00), *(f32*)(player + 0x9F4));
        PlayCharacterAnimation((Character*)player, 1u, 0xF5770040u, 1u);   // "explore1"
        ExplorationPromptClear(0);
    }
}

// 0x800F1AA4
// Activates an entity: if flagged 0x200000 call its vtable slot 0x14 under a scoped lock,
// otherwise set flag 0x800; always clears flag 0x80.
void ActivateEntity(void* a0) {
    u8* ent = (u8*)a0;
    if (ent != NULL) {
        u8* data = *(u8**)(ent + 0x4);
        s32 flags = *(s32*)(data + 0x20);
        if (flags & 0x200000) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(void (**)(void*, s32))(*(u8**)ent + 0x14))(ent, 1);
            ScopedLockDtor(&lock, -1);
        } else {
            *(s32*)(data + 0x20) = flags | 0x800;
        }
        *(s32*)(*(u8**)(ent + 0x4) + 0x20) &= 0xFFFFFF7F;
    }
}

}  // extern "C"
