// Matchup screen helpers, game-flow quit/cleanup, and small lookups in 0x8001xxxx-0x801Exxxx.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

// Virtual slot calls: the vtable pointer is read from the object, the callee gets the object as this.
typedef void (*VFnThis)(void*);
typedef void (*VFnThisS32)(void*, s32);

// Globals (C++ linkage, as in the other src/ files).
extern u8   gGame[];                    // 0x803B53A0
extern void* gScreenSets;               // 0x805FB654
extern void* gLab;                      // 0x805FBBC8
extern void* lbl_805FB964;              // 0x805FB964 movie player
extern u8*  lbl_805FB5A8;               // 0x805FB5A8 texture manager (list head at +4)
extern void* lbl_805FB5AC;              // 0x805FB5AC second list head
extern s32  lbl_805FB378;               // 0x805FB378
extern s32  lbl_805FB388;               // 0x805FB388 (invited code, high word)
extern s32  lbl_805FB38C;               // 0x805FB38C (invited code, low word)
extern f32  lbl_805FB410;               // 0x805FB410 matchup countdown
extern u8   lbl_805FB414;               // 0x805FB414 matchup ready flag
extern f32  lbl_805FEFE8;               // 0x805FEFE8 float constant (countdown start)
extern u32  gAlertPlayerWord;           // 0x805F9030
extern s32  lbl_805FAE8C;               // 0x805FAE8C (quit-to-menu flag)
extern u8   lbl_80383F58[];             // 0x80383F58
extern u8   lbl_8036CA20[];             // 0x8036CA20 (word table, 0x50 entries)

extern "C" {
#include "game/script_callbacks.h"

void*  ScreenSetsCurrent(void* sets);                                // 0x8018D1E4
void*  ScreenSetFindWidget(void* set, u32 hash);                     // 0x80198E34
void*  UIWidgetFindChild(void* widget, u32 hash);                    // 0x80199AEC
s32    fn_8019CC7C(void* el, u32 flag);                              // 0x8019CC7C
void   BufferFree(void* p);                                          // 0x801D4980
void   TextureRelease(void* tex);                                    // 0x80184028
void   fn_801731C8(void* p);                                         // 0x801731C8
void   fn_8017323C(u8* list, u8* node);                              // 0x8017323C
void fn_801BC5F0(void* obj);                         // 0x801BC5F0
void   fn_801ED788(f32* out, u8* ref);                               // 0x801ED788
s32    fn_8033B1C0(const char* name, u8* entry);                     // 0x8033B1C0
s32    fn_801F57C4(void);                                            // 0x801F57C4
BOOL   fn_801F56FC(s32 flags);                                       // 0x801F56FC
void   fn_801F5714(s32 flags);                                       // 0x801F5714
void   fn_801F5734(void* arg);                                       // 0x801F5734
void   fn_801F5794(s32 a);                                           // 0x801F5794
void   fn_801F51DC(s32 flag);                                        // 0x801F51DC
void   fn_801F57B8(void);                                            // 0x801F57B8
}

extern "C" {

// 0x8013BFA8
// Matchup teardown: when no error is up and the movie player is idle, stops it, clears the
// matchup state words and resets the alert word, then signals the matchup-exit event.
void MatchupCleanup(void) {
    if ((fn_801F57C4() == 0) && ((fn_801F56FC(0x800) == 0) || (IsShowingError() != 0))) {
        if ((lbl_805FB964 != NULL) && (*(u8*)((u8*)lbl_805FB964 + 0xB0) == 0)) {
            (*(VFnThis*)(*(u8**)lbl_805FB964 + 0x54))(lbl_805FB964);
        }
        lbl_805FB378 = 0;
        fn_801F5734(NULL);
        fn_801F5794(0);
        fn_801F51DC(0);
        fn_801F57B8();
        lbl_805FB38C = 0;
        gAlertPlayerWord = 0x40;
        lbl_805FB388 = 0;
        fn_801F5714(-1);
    }
    GameFlowSignal(0x32984033, 0, 0);
}

// 0x8013FBCC
// Sets the highlight on a matchup panel: finds the child widget by hash and applies the flag.
void MatchupSetHighlight(void* panel, u32 hash, BOOL on) {
    if (panel != NULL) {
        void* child = UIWidgetFindChild(panel, hash);
        if (child != NULL) {
            fn_8019CC7C(child, (u32)on);
        }
    }
}

// 0x8013F2A4
// Shows the matchup "ready" state on the countdown widget. Going ready starts the 60 s-style
// countdown and calls slot 0x2C of the widget; un-ready calls slot 0x30 and clears the flag.
void MatchupSetReady(BOOL ready) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    u8* panel;
    u8* btn;

    if (set == NULL) return;
    panel = (u8*)ScreenSetFindWidget(set, 0x43538242u);
    if (panel == NULL) return;
    if (UIWidgetFindChild(panel, 0x29DD4A4Eu) == NULL) return;
    btn = (u8*)UIWidgetFindChild(panel, 0x9D841CAAu);
    if (btn == NULL) return;

    if ((u32)ready == 1u) {
        if (lbl_805FB414 == 0) {
            lbl_805FB410 = lbl_805FEFE8;
            (*(VFnThisS32*)(*(u8**)(btn + 0x138) + 0x2C))(btn, 2);
            lbl_805FB414 = 1;
        }
    } else if (lbl_805FB414 == 1) {
        (*(VFnThisS32*)(*(u8**)(btn + 0x138) + 0x30))(btn, 2);
        lbl_805FB414 = 0;
    }
}

// 0x8016964C
// Reference-counted release of a slot: moves the owner's entry at arg1+0x28 out of the list
// when the owner changes, frees the owner once its count reaches zero, then re-registers.
void fn_8016964C(u8* arg0, u8* arg1) {
    u8* cur = *(u8**)(arg1 + 0x30);
    u8* owner = (cur != NULL) ? cur - 4 : NULL;

    if (owner != arg0) {
        *(s32*)(arg1 + 0x24) = *(s32*)(arg1 + 0x24) + 1;
        if (owner != NULL) {
            u8* cur2 = *(u8**)(arg1 + 0x30);
            u8* owner2 = (cur2 != NULL) ? cur2 - 4 : NULL;
            if (owner2 == owner) {
                fn_801731C8(arg1 + 0x28);
                s32 count = *(s32*)(arg1 + 0x24) - 1;
                *(s32*)(arg1 + 0x24) = count;
                if (count == 0) {
                    TextureRelease(arg1);
                }
            }
        }
        fn_8017323C(arg0 + 4, arg1 + 0x28);
    }
}

// Walks one texture list (node link at +0, name at node+0x35 with node = link - 0x28) and returns
// the first node whose name matches, or NULL.
static u8* SearchTextureList(u8* cur, const char* name) {
    while (cur != NULL) {
        u8* node = cur - 0x28;
        if (fn_8033B1C0(name, node + 0x35) == 0) {
            return node;
        }
        cur = *(u8**)cur;
    }
    return NULL;
}

// 0x8016A08C
// Looks a texture up by name in the main list, then in the second list if it is a different list.
u8* fn_8016A08C(const char* name) {
    u8* found = SearchTextureList(*(u8**)(lbl_805FB5A8 + 0x4), name);
    if ((found == NULL) && ((u8*)lbl_805FB5A8 != (u8*)lbl_805FB5AC)) {
        found = SearchTextureList(*(u8**)((u8*)lbl_805FB5AC + 0x4), name);
    }
    return found;
}

// 0x801BC574
// Points the object's target slot at the static block and runs the setup on it. Returns the object.
void* fn_801BC574(void* arg0) {
    *(void**)((u8*)arg0 + 0x34) = lbl_80383F58;
    fn_801BC5F0(arg0);
    return arg0;
}

// 0x801EC03C
// Clears the object's first word and frees its buffer at +4.
void fn_801EC03C(u8* arg0) {
    *(s32*)(arg0 + 0x0) = 0;
    BufferFree(*(void**)(arg0 + 0x4));
    *(void**)(arg0 + 0x4) = NULL;
}

// 0x8000CDE8
// Flags that the game should quit to the main menu.
void GameFlowQuitToMainMenu(void) {
    lbl_805FAE8C = 1;
}

// 0x8010A634
// Script lookup into a word table of 0x50 entries; out-of-range index gives 0.
ScriptValue GameNetGetChunkSizeCallback(BOOL a0) {
    if ((u32)a0 <= 0x4FU) {
        return (ScriptValue)*(s32*)(lbl_8036CA20 + (u32)a0 * 4);
    }
    return 0;
}

// 0x801DDD64
// Returns the state ID stored at +4 of the state entry for the current index (0 if none).
// The entry table is reached through two pointer loads from the script context.
u32 GetCurrentStateID(ScriptCtx* a0) {
    u8* ctx = (u8*)a0;
    u8* entry = NULL;

    if (ctx != NULL) {
        s32 idx = *(s32*)(ctx + 0x4);
        if (idx > -1) {
            u8* table = *(u8**)(*(u8**)ctx);
            entry = table + idx * 0x30;
        }
        if (entry != NULL) {
            return *(u32*)(entry + 0x4);
        }
        return 0U;
    }
    return 0U;
}

// 0x800ECADC
// Distance from the entity's position (at +4 of a0) to the point given by handle a1.
f32 GetDistanceTo(void* a0, u32 a1) {
    f32 ref[3];
    f32 dist = 0.0f;
    u8* pos;
    f32 dx, dy, dz, d2;

    fn_801ED788(ref, (u8*)(uintptr_t)a1);
    pos = *(u8**)((u8*)a0 + 0x4);
    dx = *(f32*)(pos + 0x0) - ref[0];
    dy = *(f32*)(pos + 0x4) - ref[1];
    dz = *(f32*)(pos + 0x8) - ref[2];
    d2 = (dz * dz) + ((dx * dx) + (dy * dy));
    if (d2 > 0.0f) {
        dist = sqrtf(d2);
    }
    return dist;
}

// 0x800F92B0
// Distance from the entity's position (at +4 of a0) to the player's position (player at gGame+0xD614).
f32 GetDistanceToPlayer(void* a0) {
    f32 dist = 0.0f;
    u8* pos = *(u8**)((u8*)a0 + 0x4);
    u8* player = *(u8**)(gGame + 0xD614);
    u8* ppos = *(u8**)(player + 0x4);
    f32 dx, dy, dz, d2;

    dx = *(f32*)(pos + 0x0) - *(f32*)(ppos + 0x0);
    dy = *(f32*)(pos + 0x4) - *(f32*)(ppos + 0x4);
    dz = *(f32*)(pos + 0x8) - *(f32*)(ppos + 0x8);
    d2 = (dz * dz) + ((dx * dx) + (dy * dy));
    if (d2 > 0.0f) {
        dist = sqrtf(d2);
    }
    return dist;
}

// 0x800FA52C
// Returns the experiment word at +0x78 of the lab object.
s32 GetExperimentMSP(void) {
    return *(s32*)((u8*)gLab + 0x78);
}

}  // extern "C"
