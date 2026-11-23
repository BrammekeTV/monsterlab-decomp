// More small functions in 0x800Exxxx (prop, player input, entity script callbacks).
#include "types.h"
#include "engine/math.h"
#include <stdint.h>

extern u8  gGame[];                 // 0x803B53A0
extern u8  gWorldState[];           // 0x803E6D00 (holds a pointer to the world block)
extern u8  gFlag805FB948;           // 0x805FB948 (two-player mode flag)
extern "C++" void* gEventMgr;             // 0x805FB6C8
extern u8  lbl_803C4298[];          // 0x803C4298
extern u8  lbl_803C7110[];          // 0x803C7110 (three floats)
extern u8  lbl_803601B8[];          // 0x803601B8 (vtable)
extern u8  lbl_8047C760[];          // 0x8047C760 (effect manager)
extern u8  lbl_805FBBE0[];          // 0x805FBBE0 (save block pointer)
extern void* lbl_805FB1C8;          // 0x805FB1C8 (texture/state record pointer)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern void* lbl_805FB0F4;          // 0x805FB0F4 (pending voice SFX handle)
extern u8  lbl_805FB1C0;            // 0x805FB1C0
extern u8  lbl_805FB1C1;            // 0x805FB1C1
extern const char lbl_805F8D90[];   // 0x805F8D90 (format string)
extern f32 lbl_805FEA50;            // 0x805FEA50 (float constant)
extern f32 lbl_805FEAF8;            // 0x805FEAF8 (float constant)
extern f32 lbl_805FEAFC;            // 0x805FEAFC (float constant)
extern f32 lbl_805FEB08;            // 0x805FEB08 (float constant, threshold)
extern f32 lbl_805FEB0C;            // 0x805FEB0C (float constant, zero)

typedef void* (*GetShotFn)(void* self);

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
u8*  fn_800DE5E4(u8* p, s32 del);                              // 0x800DE5E4
u8* fn_800DEC78(u8* arg0, s32 arg1);                                // 0x800DEC78
void  fn_800E03F0(void* p, int a);                              // 0x800E03F0
void  fn_800E4E80(u8* p, s32 clip);                             // 0x800E4E80
u8*   fn_800E4D7C(u32 a0);                                      // 0x800E4D7C
s32   fn_801ED538(u8* a3);                                      // 0x801ED538
u8*   fn_801ED6E4(u32 a0);                                      // 0x801ED6E4
void* fn_801B7198(void* anim, s32 a);                           // 0x801B7198
u32   fn_801B94F8(void* obj, u32 flags);                        // 0x801B94F8
u8*   fn_8022E2F0(u8* save);                                    // 0x8022E2F0
void  fn_801E7E28(void* p);                                     // 0x801E7E28
void  fn_801E839C(u8* p);                                       // 0x801E839C
void  ZoneEffectsUpdate(void* effects, f32 dt);                 // 0x801E97FC
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
void* fn_80209814(void* v);                                     // 0x80209814
s32   PlaySoundAt(void* sound, int a, const Vec3* pos, int b);  // 0x80209ABC
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                   // 0x801B55F4
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
void* CharacterFind(s32 id);                                    // 0x800E876C
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
const char* FormatHashString(u32 hash);                              // 0x801EB708
f32   fn_8018BE30(void* obj, f32 a);                            // 0x8018BE30
void  TextureRelease(void* tex);                                // 0x80184028
f32   fn_8033F1FC(f32 y, f32 x);                                // 0x8033F1FC
void  UIText_SetString(u32 hash, const char* text);             // 0x80190E48
s32   AnimSetFindClip(void* set, s32 anim);                     // 0x801AC2FC
}

extern "C" int SysSprintf(char* buf, const char* fmt, ...);                // 0x80142EFC (C++ linkage, as in engine.cpp)

extern "C" {

// Writes an entity id (u32 script argument) back as a pointer. The PC build is 64-bit, so this
// is only correct for the 32-bit Wii addresses the script system still hands us.
static inline u8* EntityPtr(u32 v) { return (u8*)(uintptr_t)v; }

// Finds the player input word owner: world -> +8 -> +0xA4 -> +4. Returns 0 if any link is null.
static u8* PlayerInputObj(void) {
    u8* world = *(u8**)gWorldState;
    u8* a = world ? *(u8**)(world + 0x8) : NULL;
    if (!a) return NULL;
    u8* b = *(u8**)(a + 0xa4);
    if (!b) return NULL;
    return *(u8**)(b + 0x4);
}

// Calls the virtual method in slot 0x10 (returns a shot/attack object) and tests whether
// fn_8018BE30 of that object is at or past the threshold.
static BOOL AttackAnimationAtEnd(u8* obj) {
    void* shot = (*(GetShotFn*)(*(u8**)obj + 0x10))(obj);
    f32 t = fn_8018BE30(shot, lbl_805FEB0C);
    return t >= lbl_805FEB08;
}

// 0x800E04CC
// If the record's +0xD4 flag is clear, initialises the sub-object (fn_800E03F0 with 2) and
// stores the facing angle of the owner's rotation axis Y at +0x5C, enabling it with flag +0x60.
void fn_800E04CC(u8* p) {
    if (*(s32*)(p + 0xd4) != 0) return;
    fn_800E03F0(p, 2);
    u8* a = *(u8**)(lbl_803C4298 + 0x8);
    u8* b = *(u8**)(a + 0x2c0);
    u8* c = *(u8**)(b + 0x4);
    Vec3 axis = QuatGetAxisY((const Quat*)(c + 0xc));
    *(f32*)(p + 0x58) = *(f32*)(p + 0x4c);
    *(f32*)(p + 0x5c) = fn_8033F1FC(axis.y, axis.x);
    *(u8*)(p + 0x60) = 1;
}

// 0x800E2F10
// Per-zone state dispatch on the mode byte at +0x7F: 0x20 updates zone effects (state 2),
// 0x40 runs fn_801E839C (state 3), 0x08 runs fn_801E7E28 and 0x10 both yield state 2.
// Any other mode yields 0.
s32 fn_800E2F10(u8* p, f32 dt) {
    switch (*(u8*)(p + 0x7f)) {
    case 0x20:
        ZoneEffectsUpdate(p, dt);
        return 2;
    case 0x40:
        fn_801E839C(p);
        return 3;
    case 0x08:
        fn_801E7E28(p);
        return 2;
    case 0x10:
        return 2;
    default:
        return 0;
    }
}

// 0x800E42E8
// Initialises a record: calls fn_800DEC78 with 0xF, sets the vtable, clears the flags and
// the global flags 0x805FB1C0/1, and resets five ints to -1 and a float to lbl_805FEA50.
void fn_800E42E8(u8* p) {
    fn_800DEC78(p, 0xf);
    *(u8*)(p + 0x74) = 0;
    *(void**)p = lbl_803601B8;
    *(f32*)(p + 0x44) = lbl_805FEA50;
    *(s32*)(p + 0x78) = 0;
    lbl_805FB1C0 = 0;
    lbl_805FB1C1 = 0;
    *(s32*)(p + 0x60) = -1;
    *(s32*)(p + 0x64) = -1;
    *(s32*)(p + 0x68) = -1;
    *(s32*)(p + 0x6c) = -1;
    *(s32*)(p + 0x70) = -1;
}

// 0x800E507C
// Looks up the animation set of the animated prop with handle a0 and enables clip a1 on it.
void CIAnimatedPropEnable(u32 a0, u32 a1) {
    u8* p = fn_801ED6E4(a0);
    s32 h = fn_801ED538(p + 0x58);
    void* anim = fn_801B7198(&h, 0);
    u8* set = *(u8**)(*(u8**)((u8*)anim + 0x9c) + 0x48);
    s32 clip = AnimSetFindClip(set, (s32)a1);
    fn_800E4E80(p, clip);
}

// 0x800E50E8
// Same as CIAnimatedPropEnable, but the prop is found through fn_800E4D7C.
void CIAnimatedPropEnableByID(u32 a0, u32 a1) {
    u8* p = fn_800E4D7C(a0);
    s32 h = fn_801ED538(p + 0x58);
    void* anim = fn_801B7198(&h, 0);
    u8* set = *(u8**)(*(u8**)((u8*)anim + 0x9c) + 0x48);
    s32 clip = AnimSetFindClip(set, (s32)a1);
    fn_800E4E80(p, clip);
}

// 0x800E5608
// Deleting destructor: clears four ints, runs the base dtor on the subobject at +0xC with -1,
// then frees the object if del > 0. Returns p.
u8* fn_800E5608(u8* p, s32 del) {
    if (p) {
        *(s32*)(p + 0x64) = 0;
        *(s32*)(p + 0x60) = 0;
        *(s32*)(p + 0x5c) = 0;
        *(s32*)(p + 0x58) = 0;
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E6260
// Turns on the player's input bits (OR with the mask from fn_801B94F8).
void EnablePlayerInput(u32 a0) {
    u8* obj = PlayerInputObj();
    u32 mask = fn_801B94F8(obj, a0);
    *(u32*)obj |= mask;
}

// 0x800E62D0
// Turns off the player's input bits (AND NOT the mask from fn_801B94F8).
void DisablePlayerInput(u32 a0) {
    u8* obj = PlayerInputObj();
    u32 mask = fn_801B94F8(obj, a0);
    *(u32*)obj &= ~mask;
}

// 0x800E68BC
// Formats the quest counter number into the HUD text: from the quest record when one is
// active, otherwise from the state record at +0x20. Sends the text to UI widget 0xC216F8BC.
void QuestCounterUpdateNumber(void) {
    char buf[0x88];
    u8* save = *(u8**)lbl_805FBBE0;
    u8* rec = fn_8022E2F0(save);
    if (rec) {
        SysSprintf(buf, lbl_805F8D90, *(s32*)(rec + 0xc), *(s32*)(rec + 0x10));
    } else {
        s32 v = *(s32*)((u8*)lbl_805FB1C8 + 0x20);
        SysSprintf(buf, lbl_805F8D90, v, v);
    }
    UIText_SetString(0xC216F8BCu, buf);
}

// 0x800E6C60
// Resets the current quest counter state record: clears its count and timers, and releases
// its texture when the last reference goes away. Then clears the record pointer.
void fn_800E6C60(void) {
    u8* s = (u8*)lbl_805FB1C8;
    *(s32*)(s + 0x18) = 0;
    *(f32*)(s + 0x14) = lbl_805FEAF8;
    *(u8*)(s + 0x11) = 0;
    u8* tex = *(u8**)s;
    if (tex) {
        s32 refs = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = refs;
        if (refs == 0) TextureRelease(tex);
    }
    *(void**)s = NULL;
}

// 0x800E7530
// Creates the quest counter state record (0x28 bytes) and stores it in lbl_805FB1C8.
// Also sets the three floats at lbl_803C7110 to {lbl_805FEAF8, lbl_805FEAFC, lbl_805FEAF8}.
void fn_800E7530(void) {
    *(f32*)(lbl_803C7110 + 0x0) = lbl_805FEAF8;
    *(f32*)(lbl_803C7110 + 0x4) = lbl_805FEAFC;
    *(f32*)(lbl_803C7110 + 0x8) = lbl_805FEAF8;
    u8* s = (u8*)fn_801D40E0(0x28);
    if (s) {
        *(s32*)(s + 0x18) = 0;
        *(f32*)(s + 0x14) = lbl_805FEAF8;
        *(u8*)(s + 0x11) = 0;
        *(u8*)(s + 0x10) = 0;
        *(s32*)(s + 0x0) = 0;
        *(s32*)(s + 0x4) = 0;
        *(s32*)(s + 0x8) = 0;
        *(s32*)(s + 0xc) = 0;
        *(u8*)(s + 0x1c) = 0;
        *(u8*)(s + 0x1d) = 0;
        *(s32*)(s + 0x20) = 0;
    }
    lbl_805FB1C8 = s;
}

// 0x800E83B4
// Adds a (positive) amount to the health of the object's owner at +4 (field +0x24), capped
// by the max at +0x5F4 when flag 0x10 is set in the owner, and by the value at +0x158 when
// flag 0x40 is set.
void AddHealth(void* a0, void* a1, f32 a2) {
    (void)a1;
    if (a2 <= lbl_805FEB0C) return;
    u8* owner = *(u8**)((u8*)a0 + 0x4);
    *(f32*)(owner + 0x24) += a2;

    owner = *(u8**)((u8*)a0 + 0x4);
    if (*(u32*)(owner + 0x1c) & 0x10) {
        f32 cap = *(f32*)((u8*)a0 + 0x5f4);
        if (cap < *(f32*)(owner + 0x24)) *(f32*)(owner + 0x24) = cap;
    }

    owner = *(u8**)((u8*)a0 + 0x4);
    if (*(u32*)(owner + 0x1c) & 0x40) {
        f32 cap = *(f32*)((u8*)a0 + 0x158);
        if (cap < *(f32*)(owner + 0x24)) *(f32*)(owner + 0x24) = cap;
    }
}

// 0x800E8880
// Sends event a1 with arguments a2, a3 (and 1, 0 as the last two) from the global event
// manager to the character with id a0.
void SendSignalToEntity(u32 a0, u32 a1, s32 a2, s32 a3) {
    FormatHashString(a0);
    void* target = CharacterFind((s32)a0);
    EventSend(gEventMgr, target, NULL, a1, a2, a3, 1, 0);
}

// 0x800E9420
// Returns 1 when the player's current attack animation has reached its end (see
// AttackAnimationAtEnd), 0 otherwise or when there is no player.
u32 IsPlayerAttackAnimationDone(void) {
    u32 done = 0;
    u8* player = *(u8**)(gGame + 0xD614);
    if (player) {
        u8* obj = *(u8**)(player + 0xc60);
        if (AttackAnimationAtEnd(obj)) done = 1;
    }
    return done;
}

// 0x800E9538
// Returns 1 when the monster entity a0's current attack animation has reached its end, 0
// otherwise or when a0 is null.
u32 IsMonsterAttackAnimationDone(u32 a0) {
    u32 done = 0;
    u8* ent = EntityPtr(a0);
    if (ent) {
        u8* obj = *(u8**)(ent + 0xc60);
        if (AttackAnimationAtEnd(obj)) done = 1;
    }
    return done;
}

// 0x800E95C0
// Plays voice SFX object a0 (unless it is null or its flag bit 0 at +5 is set), at the
// voice slot lbl_805FBBAC, and remembers it in lbl_805FB0F4.
void PlayVoiceSFX(u32 a0) {
    u8* obj = EntityPtr(a0);
    if (!obj) return;
    if (*(u8*)(obj + 0x5) & 1) return;
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySoundAt(obj, 0, NULL, 2);
    lbl_805FB0F4 = obj;
}

// 0x800E96D0
// Returns 1 when the given id is the player's id: always when gFlag805FB948 is set,
// otherwise when it matches the id at +8 of the player object (world +8 -> +0x9C).
s32 IsPlayerActive(u32 a0) {
    if (gFlag805FB948) return 1;
    u8* world = *(u8**)gWorldState;
    u8* a = *(u8**)(world + 0x8);
    u8* b = *(u8**)(a + 0x9c);
    u32 id = b ? *(u32*)(b + 0x8) : 0;
    return a0 == id ? 1 : 0;
}

// 0x800E9DD4
// Spawns effect a1 at the position of the bone with index a2 on entity a0 (if the entity
// has a bone table, +0xA0 non-null). The effect uses a zero fourth component.
void SpawnParticleOnBone(void* a0, u32 a1, s32 a2) {
    if (*(s32*)((u8*)a0 + 0xa0) == 0) return;
    u8 out[0x30];
    fn_801B55F4((u8*)a0, out, a2);
    f32 pos[4];
    pos[0] = *(f32*)(out + 0x24);
    pos[1] = *(f32*)(out + 0x28);
    pos[2] = *(f32*)(out + 0x2c);
    pos[3] = lbl_805FEB0C;
    SpawnEffectAt(lbl_8047C760, a1, pos, 0);
}

// 0x800EBE3C
// Returns 1 when the object's flag bit 0x10 at +4 -> +0x1C is set and its value at +0x84C is
// below zero (falling), otherwise 0.
s32 IsFalling(void* a0) {
    u8* owner = *(u8**)((u8*)a0 + 0x4);
    if (*(u32*)(owner + 0x1c) & 0x10) {
        if (*(f32*)((u8*)a0 + 0x84c) < lbl_805FEB0C) return 1;
    }
    return 0;
}

// 0x800EC30C
// Sets (a1 != 0) or clears the rotate mode bit 0x800000 at +0x15C, but only when the owner's
// flag bit 0x40 at +4 -> +0x1C is set.
void TurretSetRotateMode(void* a0, s32 a1) {
    u8* owner = *(u8**)((u8*)a0 + 0x4);
    if (!(*(u32*)(owner + 0x1c) & 0x40)) return;
    u32 v = *(u32*)((u8*)a0 + 0x15c);
    if (a1 != 0) v |= 0x00800000u;
    else         v &= ~0x00800000u;
    *(u32*)((u8*)a0 + 0x15c) = v;
}

}
