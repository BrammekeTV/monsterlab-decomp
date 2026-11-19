// More small functions in 0x800Fxxxx (inventory, quest timer, camera, network script callbacks).
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8   gGame[];                  // 0x803B53A0
extern u8*  gBattle;                  // 0x805FBBB8
extern u8   lbl_803C4298[];           // 0x803C4298 (level info block)
extern u8   lbl_805FBBE0[];           // 0x805FBBE0 (inventory block pointer)
extern u8   lbl_805FB014[];           // 0x805FB014 (global object pointer)
extern void* lbl_805FB964;            // 0x805FB964 (network object)
extern s32  lbl_805FB974;             // 0x805FB974
extern u8   lbl_804DD180[];           // 0x804DD180 (0xA0-byte entries)
extern u8   lbl_8047C640[];           // 0x8047C640 (camera)
extern f32  lbl_805FEB0C;             // 0x805FEB0C (float constant)
extern f32  lbl_805FEB78;             // 0x805FEB78 (float constant)

extern "C" {
s32   PartGetCurrentHealth(const PartInstance* p);    // 0x8021EF68
u8    fn_8021EE94(u8* p);                             // 0x8021EE94
s32   WorldGetPhase(void* world);                     // 0x800D83A8
void  SessionAddTime(u8* s, f32 dt);                  // 0x80012F50
f32   SessionGetTimer(u8* s);                         // 0x80012F70
u32   fn_801EC880(u8* p, u32 hash);                   // 0x801EC880
void  fn_801ECD1C(u8* p, s32 idx, s32 immediate);     // 0x801ECD1C
void  fn_801ECFA0(u8* p, s32 idx);                    // 0x801ECFA0
void* CharacterFind(s32 id);                          // 0x800E876C
void  ControlsUnlock(void* player);                   // 0x8004CE44
s32   fn_80214F50(u8* p);                             // 0x80214F50
void  fn_8004504C(void);                              // 0x8004504C
f32   fn_801D8AD4(u8* p);                             // 0x801D8AD4
void  fn_8010A374(void);                              // 0x8010A374
void  fn_8010A28C(void);                              // 0x8010A28C
void* fn_8010A578(s32 arg0);                              // 0x8010A578
void  fn_802209D0(void (*cb)(void));                  // 0x802209D0
void  fn_80220A1C(s32 a);                             // 0x80220A1C
void  fn_80220A0C(s32 a);                             // 0x80220A0C
}

// Returns the inventory part at index idx, or null if the index is out of range or the slot is empty.
static u8* InventoryPartAt(s32 idx) {
    if (idx < 0) return 0;
    u8* inv = *(u8**)lbl_805FBBE0;
    if (idx >= *(s32*)(inv + 0x10EC)) return 0;
    return *(u8**)(inv + idx * 4 + 0xA58);
}

extern "C" {

// 0x800F2070
// Returns the current health of the inventory part at index a0, or 0 if the slot is empty.
s32 InventoryGetPartHealth(s32 a0) {
    u8* p = InventoryPartAt(a0);
    if (p == 0) return 0;
    return PartGetCurrentHealth((const PartInstance*)p);
}

// 0x800F20A8
// Returns 1 if the inventory part at index a0 has its +0x8 field set (not -1), else 0.
s32 InventoryPartHasEnhancement(s32 a0) {
    u8* p = InventoryPartAt(a0);
    if (p == 0) return 0;
    return *(s32*)(p + 0x8) != -1;
}

// 0x800F20F0
// Returns 1 if the inventory part at index a0 has its +0x4 field set (not -1), else 0.
s32 InventoryPartHasDefect(s32 a0) {
    u8* p = InventoryPartAt(a0);
    if (p == 0) return 0;
    return *(s32*)(p + 0x4) != -1;
}

// 0x800F2428
// Returns 1 if the inventory part at index a0 is installed (fn_8021EE94 says so), else 0.
s32 InventoryPartGetInstalled(s32 a0) {
    u8* p = InventoryPartAt(a0);
    if (p == 0) return 0;
    return fn_8021EE94(p) != 0;
}

// 0x800F4888
// Returns 1 when the world is in phase 1, else 0.
s32 IsCastleGameContext(void) {
    return WorldGetPhase(gGame + 0xD5F0) == 1;
}

// 0x800F4A80
// Adds a0 seconds to the quest timer in the session block.
void AddTimetoQuestTimer(s32 a0) {
    SessionAddTime(gGame + 0x5C8, (f32)a0);
}

// 0x800F4AC8
// Returns 1 when the quest timer has run down to or below the threshold constant, else 0.
u32 IsQuestTimerDone(void) {
    f32 t = SessionGetTimer(gGame + 0x5C8);
    return t <= lbl_805FEB0C;
}

// 0x800F4D64
// Removes the particle effect attached to the emitter at a0 (record at +0x94), non-immediate.
void RemoveAttachedParticleEffectAtEmitter(void* a0, u32 a1) {
    if (a0 == 0) return;
    u8* rec = (u8*)a0 + 0x94;
    s32 idx = (s32)fn_801EC880(rec, a1);
    if (idx >= 0) fn_801ECD1C(rec, idx, 0);
}

// 0x800F4DB4
// Removes all particle effects attached to the emitter at a0 (record at +0x94).
void RemoveAllAttachedParticleEffectAtEmitter(void* a0, u32 a1) {
    if (a0 == 0) return;
    u8* rec = (u8*)a0 + 0x94;
    s32 idx = (s32)fn_801EC880(rec, a1);
    if (idx >= 0) fn_801ECFA0(rec, idx);
}

// 0x800F4E64
// Removes the particle effect attached to the emitter at a0 (record at +0x94), immediately.
void RemoveAttachedParticleEffectAtEmitterImmediate(void* a0, u32 a1) {
    if (a0 == 0) return;
    u8* rec = (u8*)a0 + 0x94;
    s32 idx = (s32)fn_801EC880(rec, a1);
    if (idx >= 0) fn_801ECD1C(rec, idx, 1);
}

// 0x800F4F4C
// Plays camera animation a0 on the boss camera, unless the game flag at +0x24 is set.
void PlayBossCameraAnimation(u32 a0) {
    if (*(u8*)(gGame + 0x24) != 0) return;
    u8* obj = *(u8**)lbl_805FB014;
    u8* cam = obj ? *(u8**)(obj + 0x4D4) : 0;
    if (cam == 0) return;
    (*(void (**)(void*, u32))(*(u8**)cam + 0x84))(cam, a0);
}

// 0x800F4FB0
// Rumbles the controller for one second when the battle state at +0xA0C is not ready; always
// runs the per-frame update fn_8004504C afterwards.
void fn_800F4FB0(void) {
    s32 r = fn_80214F50((u8*)*(void**)(gBattle + 0xA0C));
    if (r == 0) {
        TimedRumble(*(s32*)(lbl_803C4298 + 0x38), lbl_805FEB78, 1);
    }
    fn_8004504C();
}

// 0x800F58C0
// Stores the field of view returned by fn_801D8AD4 for the camera block into the game state at +0x20.
void StoreFOV(void) {
    f32 fov = fn_801D8AD4(lbl_8047C640);
    *(f32*)(gGame + 0x20) = fov;
}

// 0x800F5DF0
// Sends the combat exit location signal, chosen by the state of the object at lbl_805FBBE0.
void SendSignalCombatExitLocation(void) {
    u8* obj = *(u8**)lbl_805FBBE0;
    if (obj == 0) {
        GameFlowSignal(0x1F511018, 0, 0);
    } else if (*(s32*)(obj + 0x138) == 1) {
        GameFlowSignal((s32)0xE448BE94u, 0, 0);
    } else {
        GameFlowSignal((s32)0x69469B6Eu, 0, 0);
    }
}

// 0x800F5EF0
// Sets up the network: two init calls, registers the callback fn_8010A578, then enables it.
void InitNetwork(void) {
    fn_8010A374();
    fn_8010A28C();
    fn_802209D0((void (*)(void))fn_8010A578);  // lint:ok callback ignores the pointer return
    fn_80220A1C(1);
    lbl_805FB974 = 1;
}

// 0x800F5F30
// Hosts a game when the network object exists: calls its vtable slot 0x20, then fn_80220A0C(0).
void HostGame(void) {
    if (lbl_805FB964 != 0) {
        void* obj = lbl_805FB964;
        (*(void (**)(void*))(*(u8**)obj + 0x20))(obj);
        fn_80220A0C(0);
    }
}

// 0x800F5F74
// Joins the game at slot a0 (0xA0-byte entry) when the network object accepts it.
void JoinGame(s32 a0) {
    void* obj = lbl_805FB964;
    if (obj != 0) {
        u8* entry = lbl_804DD180 + a0 * 0xA0;
        s32 ok = (*(s32 (**)(void*, u8*))(*(u8**)obj + 0x24))(obj, entry);
        if (ok != 0) fn_80220A0C(1);
    }
}

// 0x800F6348
// Locks the camera onto the character with id a0 (set as the camera's target at +0x2C0).
void LockCameraToEntity(u32 a0) {
    void* e = CharacterFind((s32)a0);
    if (e != 0) {
        u8* cam = *(u8**)(lbl_803C4298 + 0x8);
        *(void**)(cam + 0x2C0) = e;
        u8* obj = *(u8**)(lbl_803C4298 + 0x4);
        (*(void (**)(void*))(*(u8**)obj + 0x10))(obj);
    }
}

// 0x800F6394
// Like LockCameraToEntity, and also sets the camera's instant-lock flag (+0x18) to 1.
void InstantLockCameraToEntity(u32 a0) {
    void* e = CharacterFind((s32)a0);
    if (e != 0) {
        u8* cam = *(u8**)(lbl_803C4298 + 0x8);
        *(void**)(cam + 0x2C0) = e;
        u8* obj = *(u8**)(lbl_803C4298 + 0x4);
        (*(void (**)(void*))(*(u8**)obj + 0x10))(obj);
    }
    *(u8*)(*(u8**)(lbl_803C4298 + 0x4) + 0x18) = 1;
}

// 0x800F6E28
// Stops Wii input play: unlocks the player controls, unpauses exploration characters and pops UI screen 0xAA7F5A69.
void StopWiiInputGame(void) {
    ControlsUnlock(*(void**)(gGame + 0xD614));
    UnPauseAllExplorationCharacters();
    UIScreen_PopScreen(0xAA7F5A69u);
}

}  // extern "C"
