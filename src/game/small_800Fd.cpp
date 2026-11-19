// Small script-callback functions in 0x800F7668-0x800F8EFC (exploration / NIS helpers):
// NIS camera speed, coffin landing, conversation entity and FX, castle access checks,
// entity and character particle effects, quest-giver flag, aux effect start/stop,
// hazard collision toggles and player-monster damage.
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8    gGame[];              // 0x803B53A0
extern void* gObjDb;               // 0x805FB180
extern void* lbl_805FBBAC;         // 0x805FBBAC (aux effect handle)
extern void* lbl_805FBBE0;         // 0x805FBBE0 (save block pointer)
extern f32   lbl_805FEB0C;         // 0x805FEB0C (float constant, zero)
extern f32   lbl_805FEB84;         // 0x805FEB84 (float constant)
extern u8    lbl_8040E5F0[];       // 0x8040E5F0 (frame effects object)

// World pointer used by the phase checks (gGame + 0x10000 - 0x2A10).
#define WORLD_PTR (gGame + 0xD5F0)

extern "C" {
void*  CharacterFind(s32 id);                    // 0x800E876C
s32    WorldGetPhase(void* world);               // 0x800D83A8
u32    SaveCheckFlag2(u8* s, u8 flag);           // 0x80225DF8
u8*    fn_800D4588(void* db, void* data);        // 0x800D4588
void   fn_800D75D0(void* obj);                   // 0x800D75D0
void   fn_80050F08(void* ch);                    // 0x80050F08
void   fn_801629D0(void* obj, s32 n);            // 0x801629D0
void   fn_80059810(void* obj, u32 a1, f32* vec); // 0x80059810
void   fn_800598E8(void* obj, s32 on);           // 0x800598E8
void   fn_8005979C(void* obj, s32 on);           // 0x8005979C
void   fn_800596F0(void* obj, s32 on);           // 0x800596F0
void   fn_8006CB78(void* obj, u32 a1);           // 0x8006CB78
void   fn_8006CC14(u8* obj, s32 a1);             // 0x8006CC14
void   fn_8004DE74(void* mon, s32 kind);         // 0x8004DE74
void*  fn_801D40E0(s32 n);                       // 0x801D40E0
void*  fn_80209814(void* v);                     // 0x80209814
void   fn_8020B1C8(void* h, u32 a1, s32 a2);     // 0x8020B1C8
void   fn_8020B1D4(void* h, s32 on);             // 0x8020B1D4
}

typedef void* (*VirtGetFn)(void*);

// Calls the vtable slot 0x10 of obj and returns its pointer result.
static u8* VirtGet10(void* obj) {
    return (u8*)(*(VirtGetFn*)(*(u8**)obj + 0x10))(obj);
}

// Returns the aux effect handle, creating it on first use.
static void* EnsureAuxHandle(void) {
    if (lbl_805FBBAC == 0) {
        void* h = fn_801D40E0(1);
        if (h != 0) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
    return lbl_805FBBAC;
}

// 0x800F7668
// Scales the NIS camera animation speed of the character and flags the animation (bit 3).
void SpeedUpNISCameraAnimation(void) {
    void* ch = CharacterFind((s32)0xAE906EF7);
    u8* cam = VirtGet10(ch);
    *(f32*)(cam + 0xB0) = *(f32*)(cam + 0xB0) * lbl_805FEB84;
    u8* state = VirtGet10(ch);
    *(u32*)(state + 0xF0) |= 0x8;
}

// 0x800F78B0
// Lands the coffin: runs the object-DB update for the entity, then the landing step.
void SetCoffinLanded(void* a0) {
    fn_800D4588(gObjDb, *(void**)((u8*)a0 + 0x4));
    fn_800D75D0(a0);
}

// 0x800F7900
// Sets the conversation entity to the character with this id (0 clears it).
void Wii_SetConversationEntity(u32 a0) {
    void* ch = 0;
    if (a0 != 0) ch = CharacterFind((s32)a0);
    fn_80050F08(ch);
}

// 0x800F7E70
// Stops the conversation effects on the frame effects object (both slots).
void Wii_StopConversationFX(BOOL a0) {
    (void)a0;
    fn_801629D0((void*)lbl_8040E5F0, 0x10);
    fn_801629D0((void*)lbl_8040E5F0, 4);
}

// 0x800F81E4
// Returns 1 if the castle access flag is set in the save block, else 0.
u32 CheckCastleAccess(u32 a0) {
    u32 r = 0;
    if (SaveCheckFlag2((u8*)lbl_805FBBE0, (u8)a0) != 0) r = 1;
    return r;
}

// 0x800F8228
// Returns 1 if the castle access flag is NOT set in the save block, else 0.
u32 IsCastleAccessBlocked(u32 a0) {
    u32 r = 0;
    if (SaveCheckFlag2((u8*)lbl_805FBBE0, (u8)a0) == 0) r = 1;
    return r;
}

// 0x800F85C0
// Plays a particle effect on a character at the origin (zero offset).
void AddCharacterParticleEffect(u32 a0, u32 a1) {
    f32 zero = lbl_805FEB0C;
    f32 vec[3] = {zero, zero, zero};
    fn_80059810((void*)(uintptr_t)a0, a1, vec);
}

// 0x800F860C
// Adds a particle effect to an entity: flag 0x10 uses the origin offset, flag 0x40 the other path.
void AddEntityParticleEffect(void* a0, u32 a1) {
    u8* ent = *(u8**)((u8*)a0 + 0x4);
    u32 flags = *(u32*)(ent + 0x1C);
    if (flags & 0x10) {
        f32 zero = lbl_805FEB0C;
        f32 vec[3] = {zero, zero, zero};
        fn_80059810(a0, a1, vec);
    } else if (flags & 0x40) {
        fn_8006CB78(a0, a1);
    }
}

// 0x800F8660
// Removes the particle effect from an entity; a1 != 0 selects the second (on) variant.
void RemoveEntityParticleEffect(void* a0, u32 a1) {
    u8* ent = *(u8**)((u8*)a0 + 0x4);
    u32 flags = *(u32*)(ent + 0x1C);
    if (flags & 0x10) {
        fn_800598E8(a0, (s32)(a1 != 0));
    } else if (flags & 0x40) {
        fn_8006CC14((u8*)a0, (s32)(a1 != 0));
    }
}

// 0x800F86A4
// Marks an entity as a quest giver. With a1 == 0 it is cleared; otherwise it is set only when
// the entity has one of the quest-giver flag bits (0x200000 or 0xE00) in its state word.
void SetEntityAsQuestGiver(u32 a0, u32 a1) {
    u8* obj = (u8*)(uintptr_t)a0;
    if (a1 == 0) {
        fn_8005979C(obj, 1);
        return;
    }
    u32 f = *(u32*)(*(u8**)(obj + 0x4) + 0x20);
    if (((f & 0x200000) | (f & 0xE00)) == 0) return;
    fn_800596F0(obj, 1);
}

// 0x800F86D4
// Starts the primary aux effect on the given entity id.
void StartPrimaryAuxEffect(u32 a0) {
    void* h = EnsureAuxHandle();
    fn_8020B1C8(h, a0, 0);
}

// 0x800F8730
// Stops the primary aux effect.
void StopPrimaryAuxEffect(void) {
    void* h = EnsureAuxHandle();
    fn_8020B1D4(h, 0);
}

// 0x800F877C
// Starts the secondary aux effect on the given entity id.
void StartSecondaryAuxEffect(u32 a0) {
    void* h = EnsureAuxHandle();
    fn_8020B1C8(h, a0, 1);
}

// 0x800F87D8
// Stops the secondary aux effect.
void StopSecondaryAuxEffect(void) {
    void* h = EnsureAuxHandle();
    fn_8020B1D4(h, 1);
}

// 0x800F89BC
// Turns hazard collision off for an entity while the world is in phase 2.
void DisableHazardCollision(void* a0) {
    if (a0 == 0) return;
    u8* ent = *(u8**)((u8*)a0 + 0x4);
    if ((*(u32*)(ent + 0x1C) & 0x8000) == 0) return;
    if (WorldGetPhase(WORLD_PTR) != 2) return;
    *(s32*)((u8*)a0 + 0x270) = 0;
    *(u32*)(ent + 0x20) |= 0x40000000;
}

// 0x800F8A30
// Turns hazard collision back on for an entity while the world is in phase 2.
void EnableHazardCollision(void* a0) {
    if (a0 == 0) return;
    u8* ent = *(u8**)((u8*)a0 + 0x4);
    if ((*(u32*)(ent + 0x1C) & 0x8000) == 0) return;
    if (WorldGetPhase(WORLD_PTR) != 2) return;
    *(s32*)((u8*)a0 + 0x270) = 1;
    *(u32*)(ent + 0x20) &= ~0x40000000u;
}

// 0x800F8E9C
// Damages the player's monster: if it is alive and the world is in phase 2, the monster
// takes a hit (kind 5) and its HP at +0xC88 is set to the constant at 0x805FEB84.
void DamagePlayerMonster(s32 a0) {
    (void)a0;
    u8* mon = *(u8**)(gGame + 0xD614);
    if (mon == 0) return;
    if (WorldGetPhase(WORLD_PTR) != 2) return;
    f32 hp = *(f32*)(mon + 0xC88);
    if (hp <= lbl_805FEB0C) return;
    fn_8004DE74(mon, 5);
    *(f32*)(mon + 0xC88) = lbl_805FEB84;
}
