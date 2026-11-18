// Small functions in 0x800Edxxx-0x800Fxxxx: turret, NIS/conversation signals, event broadcast,
// projectile spawns, random rolls, camera influence, challenge start, instruction-page callbacks.
//
// This file does not include include/game/script_callbacks.h: the three instruction-string
// callbacks return ScriptValue (the header declares them void), which would clash in one TU.
#include <stdlib.h>

#include "types.h"
#include "engine/math.h"

typedef s32       (*VirtS32Self)(u8* self);
typedef void      (*VirtFire)(u8* self, void* p, u32 b, f32 c);
typedef ScriptValue (*VirtStrSelf)(u8* self);

extern "C" {
extern u8   gGame[];                // 0x803B53A0
extern u8   gWorldState[];          // 0x803E6D00 (holds a pointer to the world state)
extern "C++" s32  gZoneResourceSize;      // 0x805FB94C
extern "C++" void* gEventMgr;             // 0x805FB6C8
extern s32  lbl_805FB1E8;           // 0x805FB1E8 (last random roll)
extern f32  lbl_805FEB04;           // 0x805FEB04 (float constant, projectile up component)
extern f32  lbl_805FEB0C;           // 0x805FEB0C (float constant, zero)

void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);                 // 0x801BA2FC
void* EventPost(s32 id, void* sender);                                                      // 0x801F3918
void  TriggerSendToZone(void* trigger, u32 event);                                          // 0x8006EBC4
void* SpawnHelper(void* owner, s32 arg);                                                    // 0x8000C478
void  fn_80065200(void* spawned, u8* pos, Vec3* dir, u8* owner, s32 a7, s32 a8, s32 a9);    // 0x80065200
void  fn_800ECCCC(void* a0, u32 a1, void* a2, void* a3, s32 a4, s32 a5, f32 a6);            // 0x800ECCCC
u8*   CharacterGetGround(u8* c);                                                            // 0x8005A520
s32   fn_80074D48(void);                                                                    // 0x80074D48
u32   fn_8007526C(u8* a3);                                                                  // 0x8007526C
u32   fn_8007527C(u8* a3);                                                                  // 0x8007527C
u32   RandomU32(void);                                                                      // 0x80143460
u32   fn_801ED2FC(u8* a3);                                                                  // 0x801ED2FC
u8*   fn_801ED6E4(u32 a0);                                                                  // 0x801ED6E4
void  fn_800D83B8(u8* a3, s32 a4);                                                          // 0x800D83B8
void  fn_800217B0(u32 a0);                                                                  // 0x800217B0
}

// 0x800EC3B8
// True when the turret at a0 has a live target (its entity flag bit 0x40 is set, a target
// handle is stored at +0x18C) and the target's own check (vtable slot 0x4C) reports it valid.
extern "C" s32 TurretHasTarget(void* a0) {
    u8* p = (u8*)a0;
    if (p) {
        u8* ent = *(u8**)(p + 0x4);
        if (*(u32*)(ent + 0x1c) & 0x40) {
            if (*(s32*)(p + 0x18c) != 0) {
                if ((*(VirtS32Self*)(*(u8**)p + 0x4c))(p) == 0) return 1;
            }
        }
    }
    return 0;
}

// 0x800EC41C
// Turret reload: when the turret's ammo counter at +0x190 has reached the required amount,
// clears it and reports success.
extern "C" s32 TurretReload(void* a0, s32 a1) {
    u8* p = (u8*)a0;
    if (p) {
        u8* ent = *(u8**)(p + 0x4);
        if (*(u32*)(ent + 0x1c) & 0x40) {
            s32 count = *(u8*)(p + 0x190);
            if (count >= a1) {
                *(u8*)(p + 0x190) = 0;
                return 1;
            }
        }
    }
    return 0;
}

// 0x800EC6B0
// Sends the NIS signal id to the current game state object's target through the event manager.
extern "C" void SendNISSignal(u32 a0) {
    void* target = *(void**)(*(u8**)(gGame + 0xD634) + 0x434);
    EventSend(gEventMgr, target, NULL, a0, 0, 0, 0, 0);
}

// 0x800EC6E4
// Sends the conversation signal id the same way as SendNISSignal (same target and event manager).
extern "C" void SendConversationSignal(u32 a0) {
    void* target = *(void**)(*(u8**)(gGame + 0xD634) + 0x434);
    EventSend(gEventMgr, target, NULL, a0, 0, 0, 0, 0);
}

// 0x800EC718
// Sets the camera-influence byte (+0x54) to (a1 != 0) on the entry matching a0 in the
// zone slot list of the world state.
extern "C" void Wii_SetCameraInfluenceState(u32 a0, u32 a1) {
    u8 flag = (a1 != 0) ? 1 : 0;
    u8* world = *(u8**)gWorldState;
    u8* zone = *(u8**)(world + 0x1a4) + gZoneResourceSize;
    u8* list = *(u8**)(zone + 0xa8);
    s32 count = *(u8*)(zone + 0xac);
    for (s32 i = 0; i < count; i++) {
        // List entries are 32-bit handles on the Wii; each is 4 bytes apart.
        u8* ent = (u8*)(uintptr_t)*(u32*)(list + i * 4);
        if (*(u32*)(ent + 0x4) == a0) {
            *(u8*)(ent + 0x54) = flag;
        }
    }
}

// 0x800EC794
// Returns 1 when the entity at a0 is not in the special state 1 and CharacterGetGround finds
// no ground object for it; 0 otherwise.
extern "C" s32 IsOnRealGround(u32 a0) {
    u8* c = (u8*)(uintptr_t)a0;
    if (*(s32*)(c + 0x78c) == 1) return 0;
    if (CharacterGetGround(c) == NULL) return 1;
    return 0;
}

// 0x800ECBE0
// Broadcasts event a1 from a0. With event 0, a trigger object (flag bit 0x100) sends its own
// event a2 to its zone; otherwise a0's default event at +0xF8 is used. Then the event goes
// to the event manager with a0 as sender.
extern "C" void BroadcastEvent(void* a0, u32 a1, u32 a2, BOOL a3, BOOL a4) {
    u8* self = (u8*)a0;
    if (a1 == 0) {
        u8* ent = *(u8**)(self + 0x4);
        if (*(u32*)(ent + 0x1c) & 0x100) {
            TriggerSendToZone(a0, a2);
            return;
        }
    }
    if (a1 == 0) {
        a1 = *(u32*)(self + 0xf8);
    }
    EventSendTo(gEventMgr, a1, a0, a2, (int)a3, (int)a4);
}

// 0x800ECC58
// Spawns a projectile owned by a1 and launches it from a0's position (+0x18) straight up
// (direction (0, 0, up constant)).
extern "C" void FireProjectileUp(void* a0, u32 a1) {
    u8* self = (u8*)a0;
    void* spawned = SpawnHelper((void*)(uintptr_t)a1, 0);
    if (spawned) {
        Vec3 dir;
        dir.x = lbl_805FEB0C;
        dir.y = lbl_805FEB0C;
        dir.z = lbl_805FEB04;
        fn_80065200(spawned, self + 0x18, &dir, self, 0, 0, 0);
    }
}

// 0x800ED198
// Fires a projectile from a0 at entity a2. The word at a3 is copied into a temp and its
// address is passed on with the float a4.
extern "C" void FireProjectileAtEntity(void* a0, u32 a1, void* a2, u32 a3, f32 a4) {
    u32 tmp = *(u32*)(uintptr_t)a3;
    fn_800ECCCC(a0, a1, a2, &tmp, 0, 0, a4);
}

// 0x800ED310
// Virtual call on a0 (vtable slot 0xD8) to fire at its active target: passes a copy of the
// word at a2 plus a1 and the float a3.
extern "C" void EnemyFireProjectileAtActiveTarget(u32 a0, u32 a1, u32 a2, f32 a3) {
    u8* obj = (u8*)(uintptr_t)a0;
    u32 tmp = *(u32*)(uintptr_t)a2;
    VirtFire fn = (VirtFire)*(void**)(*(u8**)obj + 0xd8);
    fn(obj, &tmp, a1, a3);
}

// 0x800ED358
// Rolls a percent (rand() % 100) and returns 1 when the roll is below a0 (a percent chance),
// computed with the original branchless compare.
extern "C" s32 RandomPercent(s32 a0) {
    s32 roll = rand() % 100;
    s32 x = a0 ^ roll;
    s32 t = x >> 1;
    s32 u = x & a0;
    s32 d = t - u;
    return (s32)((u32)d >> 31);
}

// 0x800ED3B8
// Returns a random value in 0..a0 (inclusive) and stores it in the last-roll global.
extern "C" s32 RandomZeroToMax(s32 a0) {
    u32 r = RandomU32() % (u32)(a0 + 1);
    lbl_805FB1E8 = (s32)r;
    return (s32)r;
}

// 0x800ED400
// Turns camera influence on for the object a0. When the owner's state from fn_801ED2FC is 5,
// posts event 0x16 carrying the object's word and sets the influence flags.
extern "C" void EnableCameraInfluence(u32 a0) {
    u8* obj = (u8*)(uintptr_t)a0;
    if (fn_801ED2FC(obj) == 5) {
        u8* ev = (u8*)EventPost(0x16, NULL);
        if (ev) {
            *(u32*)(ev + 0x4) = *(u32*)obj;
            *(u8*)(ev + 0x8) = 1;
        }
        u8* p = fn_801ED6E4(a0);
        if (p) {
            *(u8*)(p + 0x54) = 1;
        }
    }
}

// 0x800ED478
// Turns camera influence off for the object a0. Same as EnableCameraInfluence with the flags
// cleared.
extern "C" void DisableCameraInfluence(u32 a0) {
    u8* obj = (u8*)(uintptr_t)a0;
    if (fn_801ED2FC(obj) == 5) {
        u8* ev = (u8*)EventPost(0x16, NULL);
        if (ev) {
            *(u32*)(ev + 0x4) = *(u32*)obj;
            *(u8*)(ev + 0x8) = 0;
        }
        u8* p = fn_801ED6E4(a0);
        if (p) {
            *(u8*)(p + 0x54) = 0;
        }
    }
}

// 0x800EF99C
// Starts challenge a0: resets the challenge state in the game object (gGame + 0xD5F0, the
// world block) with mode 4, then starts the challenge with id a0.
extern "C" void StartChallenge(u32 a0) {
    fn_800D83B8(gGame + 0xD5F0, 4);
    fn_800217B0(a0);
}

// 0x800EFC94
// Instructions page callback: when a minigame object exists, calls its "next page" slot
// (vtable offset 0x48 on the object at +4). Returns the callee's result, or 0.
extern "C" ScriptValue GetNextInstructionsString(void) {
    if (fn_80074D48() != 0) {
        u8* obj = (u8*)(intptr_t)fn_80074D48();
        VirtStrSelf fn = (VirtStrSelf)*(void**)(*(u8**)(obj + 0x4) + 0x48);
        return fn(obj);
    }
    return 0;
}

// 0x800EFCD4
// Instructions page callback: same as GetNextInstructionsString but calls the "previous page"
// slot (vtable offset 0x4C).
extern "C" ScriptValue GetPreviousInstructionsString(void) {
    if (fn_80074D48() != 0) {
        u8* obj = (u8*)(intptr_t)fn_80074D48();
        VirtStrSelf fn = (VirtStrSelf)*(void**)(*(u8**)(obj + 0x4) + 0x4c);
        return fn(obj);
    }
    return 0;
}

// 0x800EFD10
// Instructions page callback: calls the header-string slot (vtable offset 0x50) on the current
// minigame object and returns its result, or 0 when there is none.
extern "C" ScriptValue SetInstructionsHeaderString(void) {
    if (fn_80074D48() != 0) {
        u8* obj = (u8*)(intptr_t)fn_80074D48();
        VirtStrSelf fn = (VirtStrSelf)*(void**)(*(u8**)(obj + 0x4) + 0x50);
        return fn(obj);
    }
    return 0;
}

// 0x800EFD4C
// True when the instructions are on the first page: no minigame object means first page;
// otherwise the object's page check (fn_8007526C) decides.
extern "C" s32 IsFirstPage(void) {
    if (fn_80074D48() != 0) {
        return (s32)fn_8007526C((u8*)(intptr_t)fn_80074D48());
    }
    return 1;
}

// 0x800EFD84
// True when the instructions are on the last page: no minigame object means last page;
// otherwise the object's page check (fn_8007527C) decides.
extern "C" s32 IsLastPage(void) {
    if (fn_80074D48() != 0) {
        return (s32)fn_8007527C((u8*)(intptr_t)fn_80074D48());
    }
    return 1;
}
