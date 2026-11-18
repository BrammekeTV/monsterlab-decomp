// Minigame game-flow state callbacks (intro, gameplay, success/failure outros, pause) and HUD helpers (0x8001xxxx).
#include <stdint.h>

#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8  gWorldState[];          // 0x803E6D00
extern u8  lbl_803C4298[];         // level info block (state id at +0x18)
extern u8  lbl_803C4B7C[];         // 12-byte float block written by fn_800124DC
extern u8  lbl_805FAEF8[];         // global object constructed by fn_800124DC
extern u8  lbl_803C49B8[];         // __register_global_object handle
extern f32 lbl_805FC6D8;           // float constant
extern f32 lbl_805FC6D0;           // float constant

extern "C" {
s32 fn_80074D48(void);                                                     // 0x80074D48 (current minigame object, may be 0)
u8  fn_80074D50(void);                                                     // 0x80074D50 (minigame state query, 1 = special case)
void fn_80074D74(void);                                                    // 0x80074D74
u8* fn_80021750(void);                                                     // 0x80021750
u8* fn_8020637C(u8* p);                                                    // 0x8020637C
u8* fn_80206434(u8* p, s32 del);                                           // 0x80206434
void fn_801BC574(void* p);                                                 // 0x801BC574
void __register_global_object(void* obj, void (*dtor)(void*), void* handle);  // 0x8032EFC8
}

// Virtual slot call on obj: the vtable pointer sits at obj+0x4 (the callee gets obj as this).
typedef void (*VSlot)(void* obj);
typedef void (*VSlotF)(void* obj, f32 t);

static void VCall(u8* obj, u32 off) {
    u8* vt = *(u8**)(obj + 0x4);
    (*(VSlot*)(vt + off))(obj);
}

static void VCallF(u8* obj, u32 off, f32 t) {
    u8* vt = *(u8**)(obj + 0x4);
    (*(VSlotF*)(vt + off))(obj, t);
}

// Current minigame object from fn_80074D48 (0 if none).
static u8* MinigameObj(void) {
    return (u8*)(intptr_t)fn_80074D48();
}

// Current world time, passed as the float argument of the update slots.
static f32 WorldTimeF(void) {
    return *(f32*)(gWorldState + 0xC);
}

// Level info state id at lbl_803C4298+0x18 (5 = the minigame is running).
static BOOL LevelStateIsRunning(void) {
    return *(s32*)(lbl_803C4298 + 0x18) == 5;
}

extern "C" {

// 0x800124DC
// Static initializer: sets the float block at lbl_803C4B7C, constructs lbl_805FAEF8 and registers its destructor.
void fn_800124DC(void) {
    *(f32*)(lbl_803C4B7C + 0x0) = lbl_805FC6D8;
    *(f32*)(lbl_803C4B7C + 0x4) = lbl_805FC6D0;
    *(f32*)(lbl_803C4B7C + 0x8) = lbl_805FC6D8;
    fn_8020637C(lbl_805FAEF8);
    __register_global_object(lbl_805FAEF8, (void (*)(void*))fn_80206434, lbl_803C49B8);
}

// 0x8001A17C
// Intro state entry: runs the minigame's intro-start hook (slot 0x0C), or the fallback start hook if the query returns 1.
void minigame_gameflow_control_intro_entry(void) {
    if (fn_80074D50() == 1) {
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0xC);
    }
}

// 0x8001A1C8
// Intro state update: while the minigame state is running, ticks the object with the world time, then calls its slot 0x44.
void minigame_gameflow_control_intro_update(void) {
    u8* obj = MinigameObj();
    if (!obj) return;
    if (!LevelStateIsRunning()) return;
    VCallF(obj, 0x14, WorldTimeF());
    VCall(obj, 0x44);
}

// 0x8001A244
// Intro state exit: runs the intro-exit hook (slot 0x10).
void minigame_gameflow_control_intro_exit(void) {
    if (fn_80074D50() == 1) {
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0x10);
    }
}

// 0x8001A290
// Gameplay entry: if the special query returns 1, starts gameplay via slot 0x28 (when the object exists) and fn_80074D74; otherwise runs slot 0x18.
void minigame_gameflow_control_gameplay_entry(void) {
    if (fn_80074D50() == 1) {
        if (MinigameObj()) {
            VCall(MinigameObj(), 0x28);
        }
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0x18);
    }
}

// 0x8001A300
// Gameplay update: while the minigame state is running, ticks the object with the world time, then calls its slot 0x44.
void minigame_gameflow_control_gameplay_update(void) {
    u8* obj = MinigameObj();
    if (!obj) return;
    if (!LevelStateIsRunning()) return;
    VCallF(obj, 0x20, WorldTimeF());
    VCall(obj, 0x44);
}

// 0x8001A378
// Gameplay exit: if the special query returns 1, runs slot 0x24 (when the object exists) and fn_80074D74; otherwise runs slot 0x1C.
void minigame_gameflow_control_gameplay_exit(void) {
    if (fn_80074D50() == 1) {
        if (MinigameObj()) {
            VCall(MinigameObj(), 0x24);
        }
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0x1C);
    }
}

// 0x8001A3E8
// Success outro entry: runs slot 0x2C, or fn_80074D74 if the special query returns 1.
void minigame_gameflow_control_success_outro_entry(void) {
    if (fn_80074D50() == 1) {
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0x2C);
    }
}

// 0x8001A434
// Failure outro entry: runs slot 0x38, or fn_80074D74 if the special query returns 1.
void minigame_gameflow_control_failure_outro_entry(void) {
    if (fn_80074D50() == 1) {
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0x38);
    }
}

// 0x8001A480
// Success outro update: while the minigame state is running, ticks slot 0x34 with the world time, then calls slot 0x44.
void minigame_gameflow_control_success_outro_update(void) {
    u8* obj = MinigameObj();
    if (!obj) return;
    if (!LevelStateIsRunning()) return;
    VCallF(obj, 0x34, WorldTimeF());
    VCall(obj, 0x44);
}

// 0x8001A4F8
// Failure outro update: while the minigame state is running, ticks slot 0x40 with the world time, then calls slot 0x44.
void minigame_gameflow_control_failure_outro_update(void) {
    u8* obj = MinigameObj();
    if (!obj) return;
    if (!LevelStateIsRunning()) return;
    VCallF(obj, 0x40, WorldTimeF());
    VCall(obj, 0x44);
}

// 0x8001A570
// Quit: calls slot 0x54 on the object from fn_80021750, if there is one.
void minigame_gameflow_control_quit(void) {
    u8* obj = fn_80021750();
    if (obj) VCall(obj, 0x54);
}

// 0x8001A5A8
// Success outro exit: runs slot 0x30, or fn_80074D74 if the special query returns 1.
void minigame_gameflow_control_success_outro_exit(void) {
    if (fn_80074D50() == 1) {
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0x30);
    }
}

// 0x8001A5F4
// Failure outro exit: runs slot 0x3C, or fn_80074D74 if the special query returns 1.
void minigame_gameflow_control_failure_outro_exit(void) {
    if (fn_80074D50() == 1) {
        fn_80074D74();
    } else {
        u8* obj = MinigameObj();
        if (obj) VCall(obj, 0x3C);
    }
}

// 0x8001A640
// Pause update: calls slot 0x44 on the minigame object, if there is one.
void minigame_gameflow_control_pause_update(void) {
    u8* obj = MinigameObj();
    if (obj) VCall(obj, 0x44);
}

// 0x8001A678
// Success/fail screen update: calls slot 0x44 on the minigame object, if there is one.
void minigame_gameflow_control_success_fail_screen_update(void) {
    u8* obj = MinigameObj();
    if (obj) VCall(obj, 0x44);
}

// 0x8001AFCC
// For each of the object's entries (count at +0x428, pointers from +0x28), calls fn_801BC574 on the first word of the entry's target when set.
void fn_8001AFCC(u8* obj) {
    u32 count = *(u32*)(obj + 0x428);
    for (u32 i = 0; i < count; ++i) {
        u8* entry = *(u8**)(obj + 0x28 + i * 4);
        if (entry) {
            void* target = *(void**)entry;
            if (target) fn_801BC574(target);
        }
    }
}

// 0x8001B044
// Finds the entry whose key (target's word at +0x30) matches key. Returns the entry pointer, or 0.
u8* fn_8001B044(u8* obj, u32 key) {
    u32 count = *(u32*)(obj + 0x428);
    for (u32 i = 0; i < count; ++i) {
        u8* entry = *(u8**)(obj + 0x28 + i * 4);
        u8* target = *(u8**)entry;
        if (key == *(u32*)(target + 0x30)) return entry;
    }
    return 0;
}

// 0x8001B0B8
// Shows the conversation with the given key: finds its entry, sets the 0x200 flag on the HUD root and remembers the entry at +0x42C.
void HudShowConversation(void* hud, s32 conversation) {
    u8* h = (u8*)hud;
    u8* entry = fn_8001B044(h, (u32)conversation);
    if (!entry) return;
    u8* root = *(u8**)(*(u8**)(h + 0x24));
    *(u32*)(root + 0x28) |= 0x200;
    *(u8**)(h + 0x42C) = entry;
}

// 0x8001B128
// Pushes the current conversation slot onto the list at hud+4 (indexed by count at +0), clears the current slot and bumps the count.
void fn_8001B128(void* hud) {
    u8* h = (u8*)hud;
    u8* x = *(u8**)(h + 0x24);
    s32 idx = *(s32*)h;
    *(u8**)(h + idx * 4 + 4) = *(u8**)x;
    u8* cur = *(u8**)(h + 0x42C);
    *(u8**)(*(u8**)(h + 0x24)) = cur;
    *(u8**)(h + 0x42C) = 0;
    *(s32*)h = idx + 1;
}

}  // extern "C"
