// More small functions in 0x800Exxxx (prop/FSM/entity script callbacks).
#include "types.h"

extern u8  gWorldState[];          // 0x803E6D00 (holds a pointer to the world block)
extern u8  gGame[];                // 0x803B53A0
extern "C++" void* gEventMgr;            // 0x805FB6C8
extern u32 lbl_805F81C0;           // movie state word
extern f32 lbl_805FEAE4;           // float constant (lower bound)
extern f32 lbl_805FEAE8;           // float constant (upper bound)
extern f32 lbl_805FEB18;           // float constant (scale)

BOOL MovieQueueIsEmpty(void);      // 0x8000F840

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
void  fn_800DE5E4(u8* p, s32 del);                              // 0x800DE5E4
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void* CharacterFind(s32 id);                                    // 0x800E876C
u8*   fn_80021750(void);                                        // 0x80021750
s32   fn_801ED538(u8* a3);
u8*   fn_801ED6E4(u32 a0);
void* fn_801B7198(void* anim, s32 a);

// 0x800E435C
// Deleting destructor: base dtor on the subobject at +0xC, then free if del > 0.
u8* fn_800E435C(u8* p, s32 del) {
    if (p) {
        fn_800DE5E4(p + 0xc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E4BFC
// Looks up the handle at +0x58 and resolves it (returns fn_801B7198's result).
u8* fn_800E4BFC(u8* p) {
    s32 h = fn_801ED538(p + 0x58);
    return (u8*)fn_801B7198(&h, 0);
}

// 0x800E4C30
// Same chain as fn_800E4BFC, then sets bit 0x04000000 in the returned block's +0x15C word.
void fn_800E4C30(u8* p) {
    s32 h = fn_801ED538(p + 0x58);
    u8* r = (u8*)fn_801B7198(&h, 0);
    *(u32*)(r + 0x15c) |= 0x04000000;
}

// 0x800E4C70
// Same chain as fn_800E4BFC, then clears bit 0x04000000 in the returned block's +0x15C word.
void fn_800E4C70(u8* p) {
    s32 h = fn_801ED538(p + 0x58);
    u8* r = (u8*)fn_801B7198(&h, 0);
    *(u32*)(r + 0x15c) &= ~0x04000000u;
}

// 0x800E4E48
// Returns the handle reached through obj->+0x58 chain, as the prop lookup.
u32 CIAnimatedPropGetProp(u32 a0) {
    u8* p = fn_801ED6E4(a0);
    s32 h = fn_801ED538(p + 0x58);
    return (u32)fn_801B7198(&h, 0);
}

// 0x800E61B0
// Returns 1 when (value at +0x98 - value at +0xA0) lies strictly between two constants.
u32 CIPannerAtZero(u32 a0) {
    u8* p = (u8*)fn_801ED6E4(a0);
    f32 d = *(f32*)(p + 0x98) - *(f32*)(p + 0xa0);
    if (d > lbl_805FEAE4 && d < lbl_805FEAE8) return 1;
    return 0;
}

// 0x800E6228
// Returns gWorldState -> +8 -> +0xA4 -> +4 chain, or 0 when any link is missing.
void* fn_800E6228(void) {
    u8* q = *(u8**)(*(u8**)gWorldState + 0x8);
    if (q == 0) return 0;
    u8* r = *(u8**)(q + 0xa4);
    if (r == 0) return 0;
    return *(void**)(r + 0x4);
}

// 0x800E75F0
// Calls the virtual slot 0x58 of the object returned by fn_80021750 (if any).
void ChallengeGame_EnterPauseScreen(void) {
    u8* o = fn_80021750();
    if (o) {
        u8* vt = *(u8**)(o + 0x4);
        (*(void (**)(void*))(vt + 0x58))(o);
    }
}

// 0x800E7628
// Calls the virtual slot 0x5C of the object returned by fn_80021750 (if any).
void ChallengeGame_ExitPauseScreen(void) {
    u8* o = fn_80021750();
    if (o) {
        u8* vt = *(u8**)(o + 0x4);
        (*(void (**)(void*))(vt + 0x5c))(o);
    }
}

// 0x800E7668
// True when the stored difficulty (gGame+0xC45C, clamped to 1 when outside 0..3) equals a0.
u32 IsDifficultyLevel(s32 a0) {
    s32 v = *(s32*)(gGame + 0xC45C);
    if (v < 0 || v > 3) v = 1;
    return (u32)(v == a0);
}

// 0x800E7810
// True when the movie queue is empty and the movie state word is 0 or 2.
u32 IsMovieDone(void) {
    u32 r = 0;
    if (MovieQueueIsEmpty()) {
        u32 s = lbl_805F81C0;
        if (s == 2 || s == 0) r = 1;
    }
    return r;
}

// Returns the register table at obj->+0xA4 -> *ptr, or 0 if any link is missing.
static u8* FsmRegTableBase(void* a0) {
    u8* p = (u8*)a0;
    if (p == 0) return 0;
    u8* t = *(u8**)(p + 0xa4);
    if (t == 0) return 0;
    return *(u8**)t;
}

// 0x800E7CA0
// Stores a2 into register a1 (0..3) of the object's register table.
void FSMSetRegInt(void* a0, BOOL a1, s32 a2) {
    if ((u32)a1 >= 4) return;
    u8* base = FsmRegTableBase(a0);
    if (base == 0) return;
    *(s32*)(base + ((u32)a1 << 2) + 0x2c) = a2;
}

// 0x800E7CE0
// Returns register a1 (0..3) of the object's register table, or 0.
s32 FSMGetRegInt(void* a0, BOOL a1) {
    if ((u32)a1 >= 4) return 0;
    u8* base = FsmRegTableBase(a0);
    if (base == 0) return 0;
    return *(s32*)(base + ((u32)a1 << 2) + 0x2c);
}

// 0x800E7D30
// Adds a2 to register a1 (0..3) of the object's register table.
void FSMAddRegInt(void* a0, BOOL a1, s32 a2) {
    if ((u32)a1 >= 4) return;
    u8* base = FsmRegTableBase(a0);
    if (base == 0) return;
    *(s32*)(base + ((u32)a1 << 2) + 0x2c) += a2;
}

// 0x800E8240
// Sends event (obj->+0xEC) to the object itself when the flag bit 0x100 is set in its +4 block.
void SpawnEntity(void* a0) {
    u8* p = (u8*)a0;
    u8* ent = *(u8**)(p + 0x4);
    if ((*(u32*)(ent + 0x20) & 0x100) == 0) return;
    EventSend(gEventMgr, p, NULL, *(u32*)(p + 0xec), 0, 0, 1, 0);
}

// 0x800E866C
// Sets the entity's health value at +0x24 as a percent of the max stored at +0x5F4 (flag 0x10)
// or +0x158 (flag 0x40).
void SetEntityHealthPercent(void* a0, f32 a1) {
    u8* p = (u8*)a0;
    u8* ent = *(u8**)(p + 0x4);
    u32 flags = *(u32*)(ent + 0x1c);
    if (flags & 0x10) {
        f32 max = *(f32*)(p + 0x5f4);
        *(f32*)(ent + 0x24) = a1 * max / lbl_805FEB18;
    } else if (flags & 0x40) {
        f32 max = *(f32*)(p + 0x158);
        *(f32*)(ent + 0x24) = a1 * max / lbl_805FEB18;
    }
}

// 0x800E86BC
// Returns the entity's health at +0x24 as a percent (scale constant when neither flag is set).
f32 GetEntityHealthPercent(void* a0) {
    u8* p = (u8*)a0;
    u8* ent = *(u8**)(p + 0x4);
    u32 flags = *(u32*)(ent + 0x1c);
    if (flags & 0x10) {
        return lbl_805FEB18 * *(f32*)(ent + 0x24) / *(f32*)(p + 0x5f4);
    }
    if (flags & 0x40) {
        return lbl_805FEB18 * *(f32*)(ent + 0x24) / *(f32*)(p + 0x158);
    }
    return lbl_805FEB18;
}

// 0x800E88FC
// Looks up character a0 and calls its virtual slot 0x80 with the animation slot a1.
void Wii_PlayEntityAnimationSlot(u32 a0, s32 a1) {
    u8* o = (u8*)CharacterFind((s32)a0);
    if (o) {
        (*(void (**)(void*, s32))(*(u8**)o + 0x80))(o, a1);
    }
}

// 0x800E9324
// Looks up character a0; returns 1 when the status word at +0x20 has none of bits 0x200000, 0x800, 0x400, 0x200.
s32 CheckMonsterDead(u32 a0) {
    u8* c = (u8*)CharacterFind((s32)a0);
    if (c == 0) return 0;
    u32 f = *(u32*)(*(u8**)(c + 0x4) + 0x20);
    u32 bits = (f & 0x200000) | (f & 0xe00);
    return bits == 0;
}

// 0x800E966C
// True when the local player's (world -> +8 -> +0x9C) id at +8 equals a0.
s32 IsLocalPlayerOfSubtype(u32 a0) {
    u8* w = *(u8**)gWorldState;
    u8* q = *(u8**)(w + 0x8);
    u8* r = *(u8**)(q + 0x9c);
    u32 v = (r != 0) ? *(u32*)(r + 0x8) : 0;
    return v == a0;
}
}
