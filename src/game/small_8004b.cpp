// More small functions in 0x8004xxxx (combat HUD, combat results).
#include <string.h>

#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

// UI screen hash for the combat HUD (UI_MSA_Combat_HUD).
#define HUD_COMBAT_SCREEN 0x1B66AC94u

// Callee prototypes (C linkage, matching their definitions in other files).
extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
u8* fn_8020F3D4(u8* arg0, s32 arg1);                               // 0x8020F3D4
void  fn_8020F2A4(u8* p);                                       // 0x8020F2A4
u8* fn_8020F42C(u8* arg0, s32 arg1);                              // 0x8020F42C
u8* fn_8020F4F0(u8* p, s32 del);                              // 0x8020F4F0
void  fn_8020F46C(u8* p, u8* choice);                           // 0x8020F46C: commits a choice into p (+0x38)
void  fn_8020E9CC(u8* p, s32 a2);                               // 0x8020E9CC
void  fn_8020E9E0(u8* p, s32 del);                              // 0x8020E9E0
void  ChoiceInit(u8* c);                                        // 0x8020F36C
void* ChoiceDestroy(void* c, s32 del);                          // 0x8020F394
void  fn_801B4308(u8* p, const char* text);                     // 0x801B4308
u8*   fn_801B3308(void);                                        // 0x801B3308: returns the global list at lbl_805FB6A0
u8*   fn_801F858C(u8* p, s32 del);                              // 0x801F858C
void  fn_8000B8D4(u8* p, s32 a2);                               // 0x8000B8D4
void  fn_8000B8FC(u8* p, s32 a2);                               // 0x8000B8FC
void  fn_8000B964(u8* p, s32 a2);                               // 0x8000B964
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
void* fn_80044CDC(void* p);                                     // 0x80044CDC
s32 fn_8019CC7C(void* el, u32 flag);                            // 0x8019CC7C
void* ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
void* ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void  ExplorationHudSetFighter(void* set, void* fighter, int a); // 0x80045138
}

// Copied from their definition (C++ linkage).
extern "C" void fn_801B3378(u8* p, u8* key);                               // 0x801B3378: removes key from the list p

// Globals used here (defined elsewhere, or below).
extern "C++" void*  gScreenSets;                                      // 0x805FB654
extern u8*    gBattle;                                          // 0x805FBBB8
extern f32    lbl_805FD150;                                     // 0x805FD150 (float constant)
extern u8     lbl_803C4298[];                                   // 0x803C4298 (level info block)
extern u8     lbl_803585E8[];                                   // 0x803585E8 (vtable)
extern u8     lbl_80358784[];                                   // 0x80358784 (vtable)
extern u8     lbl_80358920[];                                   // 0x80358920 (vtable)
extern u8     lbl_80358B78[];                                   // 0x80358B78 (vtable)
extern u8     lbl_80358BC8[];                                   // 0x80358BC8 (vtable)
extern const u32 lbl_803589D8[];                                // 0x803589D8 (HUD element hashes per limb)
extern const char lbl_805F85A8[];                               // 0x805F85A8 (text)
extern const char lbl_805F85B8[];                               // 0x805F85B8 (text)
extern const char lbl_805F85C0[];                               // 0x805F85C0 (text)

// Globals owned by this file (first referenced here).
s32   lbl_803C5268[13] ORIG(0x803C5268);     // part-slot entries (10 used), reset to -1
s32   lbl_805FB090     ORIG(0x805FB090);     // call counter for fn_80044F54
void* lbl_805FB094     ORIG(0x805FB094);     // cached pointer for fn_80044F54
u8    lbl_805FB098     ORIG(0x805FB098); // set once the part selection is locked
u8    lbl_8047C730[0x30] ORIG(0x8047C730);  // combat results state; +0x20 is set by CombatResultsPauseGame

extern "C" {

// 0x80040638
// Constructor: two action-choice slots, A at +0x4C and B at +0x5C, each with a pending flag
// (+0x58 for A, +0x68 for B). Both start empty with the default value 0x4E.
void fn_80040638(u8* p, s32 a2) {
    fn_8020F3D4(p, a2);
    *(void**)p = lbl_803585E8;
    ChoiceInit(p + 0x4C);
    p[0x58] = 0;
    ChoiceInit(p + 0x5C);
    p[0x68] = 0;
    p[0x50] = 0;
    *(s32*)(p + 0x54) = 4;
    *(s32*)(p + 0x4C) = 0x4E;
    p[0x60] = 0;
    *(s32*)(p + 0x64) = 4;
    *(s32*)(p + 0x5C) = 0x4E;
}

// 0x800406B8
// Deleting destructor: destroys both choice slots, base destructor, frees if del > 0.
u8* fn_800406B8(u8* p, s32 del) {
    if (p != NULL) {
        ChoiceDestroy(p + 0x5C, -1);
        ChoiceDestroy(p + 0x4C, -1);
        fn_8020F42C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8004072C
// Per-frame update: if slot A is pending, commit it. Then, if slot B was queued, move B into A
// and mark A pending again; otherwise A is no longer pending.
void fn_8004072C(u8* p) {
    if (p[0x58] == 0) return;
    fn_8020F46C(p, p + 0x4C);
    if (p[0x68] != 0) {
        u8  b4 = p[0x60];
        s32 b8 = *(s32*)(p + 0x64);
        s32 b0 = *(s32*)(p + 0x5C);
        p[0x58] = 1;
        p[0x50] = b4;
        *(s32*)(p + 0x54) = b8;
        *(s32*)(p + 0x4C) = b0;
        p[0x68] = 0;
    } else {
        p[0x58] = 0;
    }
}

// 0x80041AB4
// Constructor: base constructor, then a float at +4 set from a constant, a vtable, and cleared
// flags and counters (+8..+0xD, +0x10, +0x20).
void fn_80041AB4(u8* p) {
    fn_8020F2A4(p);
    *(f32*)(p + 0x4) = lbl_805FD150;
    *(void**)p = lbl_80358784;
    p[0x8] = 0;
    p[0x9] = 0;
    p[0xA] = 0;
    p[0xB] = 0;
    p[0xC] = 0;
    p[0xD] = 0;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x20) = 0;
}

// 0x800440E0
// Shows a short text for the state at this+0xD4: a plain line when flag +4 is clear, otherwise
// one of two variants chosen by flag +5. The text is handed to fn_801B4308.
void fn_800440E0(u8* p) {
    char buf[0x40];
    u8* state = *(u8**)(p + 0xD4);
    const char* text;
    if (state[4] == 0) {
        text = lbl_805F85A8;
    } else if (state[5] == 0) {
        text = lbl_805F85C0;
    } else {
        text = lbl_805F85B8;
    }
    strcpy(buf, text);
    fn_801B4308(p, buf);
}

// 0x800442CC
// Deleting destructor: sets the vtable, removes this+0x48 from the global list at lbl_805FB6A0,
// destroys the member at +0x48, runs the base destructor, frees if del > 0.
u8* fn_800442CC(u8* p, s32 del) {
    if (p != NULL) {
        *(void**)p = lbl_80358920;
        fn_801B3378(fn_801B3308(), p + 0x48);
        fn_801F858C(p + 0x48, -1);
        fn_8020F4F0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80044F20
// Resets the 10 part-slot entries at lbl_803C5268 to -1.
void fn_80044F20(void) {
    for (s32 i = 0; i < 10; i++) lbl_803C5268[i] = -1;
}

// 0x80044F54
// Returns the cached pointer, building it on the first call (fn_801D40E0, then fn_80044CDC
// on a non-null result). Each call also bumps the counter in lbl_805FB090.
void* fn_80044F54(void) {
    if (lbl_805FB090 == 0) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80044CDC(v);
        lbl_805FB094 = v;
    }
    void* result = lbl_805FB094;
    lbl_805FB090 = lbl_805FB090 + 1;
    return result;
}

// 0x8004504C
// While the combat HUD is up, refresh the fighter shown in each of the two HUD slots.
void fn_8004504C(void) {
    if (UIScreen_Is_Pushed(0x3AB51700)) {
        for (s32 i = 0; i < 2; i++) {
            void* set = ScreenSetsCurrent(gScreenSets);
            ExplorationHudSetFighter(set, *(void**)(gBattle + 0xA00 + i * 4), i);
        }
    }
}

// 0x800450C0
// Clears the part-slot entries and, while the combat HUD is up, shows the player's first fighter.
void CombatHUD_UpdatePlayersPaperDoll(void) {
    fn_80044F20();
    if (UIScreen_Is_Pushed(0xCFBD5B9E)) {
        void* set = ScreenSetsCurrent(gScreenSets);
        ExplorationHudSetFighter(set, *(void**)(gBattle + 0xA00), 0);
    }
}

// 0x80046B48
// Locks the part selection on the first call: announces it to the screens, then sends the lock
// NIS signal. Later calls only send the signal.
void LockPartSelected(void) {
    if (lbl_805FB098 == 0) {
        UIEvent_SendToAllScreens(0x218664FD);
        UIEvent_SendToAllScreens(0xA60E7C96);
    }
    lbl_805FB098 = 1;
    SendNISSignal(0x3977C22B);
}

// 0x800477DC
// Limb lost on the combat HUD: when the combat HUD screen is the current one, run the element
// for this limb (table lbl_803589D8).
void BattleHudLimbLost(s32 limb) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    if (*(u32*)set != HUD_COMBAT_SCREEN) return;
    void* el = ScreenSetFindElement(set, lbl_803589D8[limb]);
    if (el != NULL) fn_8019CC7C(el, 0);
}

// 0x80049494
// Two calls on the level info block (lbl_803C4298) with arguments 0x10, then 4.
void fn_80049494(void) {
    fn_8000B964(lbl_803C4298, 0x10);
    fn_8000B964(lbl_803C4298, 4);
}

// 0x800494D4
// Combat results start: sets lbl_8047C730+0x20, then calls fn_8000B8D4 and fn_8000B8FC with 1.
void CombatResultsPauseGame(void) {
    lbl_8047C730[0x20] = 1;
    fn_8000B8D4(lbl_803C4298, 1);
    fn_8000B8FC(lbl_803C4298, 1);
}

// 0x80049524
// Combat results end: clears lbl_8047C730+0x20, then calls fn_8000B8D4 and fn_8000B8FC with 0.
void CombatResultsResumeGame(void) {
    lbl_8047C730[0x20] = 0;
    fn_8000B8D4(lbl_803C4298, 0);
    fn_8000B8FC(lbl_803C4298, 0);
}

// 0x80049628
// Constructor: base constructor, then a vtable.
u8* fn_80049628(u8* p, s32 a2) {
    fn_8020E9CC(p, a2);
    *(void**)p = lbl_80358B78;
    return p;
}

// 0x80049664
// Deleting destructor: base destructor, frees if del > 0.
u8* fn_80049664(u8* p, s32 del) {
    if (p != NULL) {
        fn_8020E9E0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800496DC
// Constructor: base constructor, a vtable, one action-choice slot at +0x4C, pending flag +0x58 clear.
u8* fn_800496DC(u8* p, s32 a2) {
    fn_8020F3D4(p, a2);
    *(void**)p = lbl_80358BC8;
    ChoiceInit(p + 0x4C);
    p[0x58] = 0;
    return p;
}

// 0x80049728
// Deleting destructor: destroys the choice slot at +0x4C, base destructor, frees if del > 0.
u8* fn_80049728(u8* p, s32 del) {
    if (p != NULL) {
        ChoiceDestroy(p + 0x4C, -1);
        fn_8020F42C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80049790
// If the choice slot at +0x4C is pending, commit it and clear the pending flag.
void fn_80049790(u8* p) {
    if (p[0x58] != 0) {
        fn_8020F46C(p, p + 0x4C);
        p[0x58] = 0;
    }
}

}  // extern "C"
