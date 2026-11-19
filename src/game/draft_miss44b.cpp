// Second half of the miss44 drafts: screen-set UI element helpers (UIActivationGrid_*,
// UIElement_*), Statistics display hook, NIS frame effects, the combat-leave check and the
// SFX voice table reset. Checked against the asm (sh tools/dump.sh <name>).
#include "types.h"

extern "C" {
void*       ScreenSetsCurrent(void* sets);                  // 0x8018D1E4
void*       ScreenSetFindElement(void* set, u32 hash);      // 0x80198F18
void        fn_8018D2B8(const char* fmt, ...);              // 0x8018D2B8 (debug log)
void        fn_8018D370(const char* fmt, ...);              // 0x8018D370 (debug log)
const char* fn_801EB708(u32 hash);                          // 0x801EB708 (hash -> name for logs)
u32         fn_8019A9D4(u8* a3);                            // 0x8019A9D4
void        fn_8019A9EC(u8* sub, s32 index);                // 0x8019A9EC
s32         fn_8016195C(void* p, s32 a, s32 b, void* c);    // 0x8016195C
void        fn_8011D69C(void* p, s32 a);                    // 0x8011D69C
BOOL        fn_801F56FC(s32 flags);                         // 0x801F56FC
BOOL        fn_80142358(void);                              // 0x80142358
u32         fn_80142468(void);                              // 0x80142468
u32         IsMultiPlayer(void);                            // src/game/small_800Fc.cpp
void        fn_801FF2A8(u8* audio, s32 handle);             // 0x801FF2A8 (audio voice release)
void        fn_801FF47C(u8* audio);                         // 0x801FF47C
}

extern "C++" void* gScreenSets;                             // 0x805FB654
extern const char lbl_80381868[];                           // 0x80381868 (string block)
extern const char lbl_803818A8[];                           // 0x803818A8 (string)
extern const char lbl_80381AA0[];                           // 0x80381AA0 (string)
extern u8   lbl_805FB248[];                                 // 0x805FB248 (table pointer word)
extern u8   lbl_8040E5F0[];                                 // 0x8040E5F0 (frame effects object)
extern u8   lbl_80559D78[];                                 // 0x80559D78 (0x28-byte audio voice records)
extern s32  lbl_805FB6B8;                                   // 0x805FB6B8
extern f32  lbl_806004F0;                                   // 0x806004F0 (zero constant)
extern f32  lbl_805FF8A4;                                   // 0x805FF8A4 (1.0f)
extern u8   gAudio[];

typedef void  (*ElemVoidFn)(void* self, s32 arg);           // vtable slot taking (element, arg)
typedef void* (*HashGetFn)(void* self, u32 hash);           // vtable slot taking (object, hash)

static const u32 kHashStatistics = 0xE94C961Du;             // "Statistics"

// Looks up the element `hash` in the current screen set. Logs like the originals and returns NULL on failure.
static u8* UIFindScreenElement(u32 hash) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    u8* el = NULL;
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
    } else {
        el = (u8*)ScreenSetFindElement(set, hash);
        if (el == NULL) {
            fn_8018D2B8(lbl_80381AA0, fn_801EB708(hash));
        }
    }
    return el;
}

extern "C" {

// 0x8018F3A8
s32 UIActivationGrid_GetCurrentIndex(void) {
    u8* set;
    u8* el;
    if (ScreenSetsCurrent(gScreenSets) == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        return -1;
    }
    set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x40);
        el = NULL;
    } else {
        s32 idx = *(s32*)(set + 0x60);
        if (idx <= 0) {
            fn_8018D2B8(lbl_80381868 + 0x14C);
            el = NULL;
        } else {
            el = *(u8**)(set + idx * 4 + 0x3C);
            if (el == NULL) {
                fn_8018D370(lbl_80381868 + 0x170);
            }
        }
    }
    if (el == NULL) {
        return -1;
    }
    {
        u8* sub = *(u8**)(el + 0x144);
        if (sub == NULL) {
            fn_8018D2B8(lbl_80381868 + 0x190);
            return -1;
        }
        return (s32)fn_8019A9D4(sub);
    }
}

// 0x8018F494
void UIActivationGrid_SetCurrentIndex(s32 a0) {
    u8* set;
    u8* el;
    if (ScreenSetsCurrent(gScreenSets) == NULL) {
        fn_8018D370(lbl_80381868 + 0x1B8);
        return;
    }
    set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x40);
        el = NULL;
    } else {
        s32 idx = *(s32*)(set + 0x60);
        if (idx <= 0) {
            fn_8018D2B8(lbl_80381868 + 0x14C);
            el = NULL;
        } else {
            el = *(u8**)(set + idx * 4 + 0x3C);
            if (el == NULL) {
                fn_8018D370(lbl_80381868 + 0x170);
            }
        }
    }
    if (el != NULL) {
        u8* sub = *(u8**)(el + 0x144);
        if (sub == NULL) {
            fn_8018D2B8(lbl_80381868 + 0x1F8);
            return;
        }
        fn_8019A9EC(sub, a0);
    }
}

// 0x8018F6E0
s32 UIElement_GetVisibleFlag(u32 a0) {
    u8* el = UIFindScreenElement(a0);
    if (el == NULL) {
        return 0;
    }
    return (s32)((*(u32*)(el + 0x64) >> 1) & 1);
}

// 0x8018FB84
f32 UIElement_GetXAtPercent(u32 a0, f32 a1) {
    u8* el = UIFindScreenElement(a0);
    f32 t;
    f32 base;
    f32 scale;
    f32 span;
    if (el == NULL) {
        return 0.0f;
    }
    base = *(f32*)(el + 0x68);
    scale = *(f32*)(el + 0x74);
    span = *(f32*)(el + 0x9C);
    // Clamp the 0..1 fraction.
    if (a1 < 0.0f) {
        t = 0.0f;
    } else if (a1 > lbl_805FF8A4) {
        t = lbl_805FF8A4;
    } else {
        t = a1;
    }
    return (base - scale * span) + scale * t;
}

// 0x8018FA34
void UIElement_RestoreZ(u32 a0) {
    u8* el = UIFindScreenElement(a0);
    if (el != NULL) {
        *(f32*)(el + 0x70) = *(f32*)(el + 0x1C);
        ((ElemVoidFn)(*(void**)(*(u8**)(el + 0x138) + 0x2C)))(el, 0x18);
    }
}

// 0x8018F778
void UIElement_SetEnableFlag(u32 a0, u32 a1) {
    u8* el = UIFindScreenElement(a0);
    if (el != NULL) {
        if (a1 != 0) {
            ((ElemVoidFn)(*(void**)(*(u8**)(el + 0x138) + 0x2C)))(el, 1);
            return;
        }
        ((ElemVoidFn)(*(void**)(*(u8**)(el + 0x138) + 0x30)))(el, 1);
    }
}

// 0x801139F0
void Statistics_IncDisplay(s32 a0) {
    u8* table = *(u8**)lbl_805FB248;
    void* found = NULL;
    if (table != NULL) {
        u8* sub = *(u8**)(table + 0x8);
        if (sub != NULL) {
            found = ((HashGetFn)(*(void**)(*(u8**)sub + 0x28)))(sub, kHashStatistics);
        }
    }
    if (found != NULL) {
        fn_8011D69C(found, a0);
    }
}

// 0x801DBBC0
void StartNISFrameFx(void) {
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x12, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x12, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x12, 0, NULL);
}

// 0x801DBC98
void StartNISTVStaticFrameFx(void) {
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x10, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x12, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x12, 0, NULL);
    fn_8016195C(lbl_8040E5F0, 0x12, 0, NULL);
}

// 0x801408F8
ScriptValue ShouldWeLeaveCombat(void) {
    s32 ok = 0;
    if (fn_801F56FC(0x10) != 0 && fn_80142358() != 0) {
        ok = 1;
    }
    if (ok != 1) {
        if ((fn_801F56FC(4) != 0 || fn_801F56FC(8) != 0) && fn_80142358() != 0) {
            ok = 1;
        }
    }
    if (ok != 1) {
        if (fn_80142468() != 1) {
            return 0;
        }
    }
    return IsMultiPlayer() != 0 ? 1 : 0;
}

// 0x8020A59C
void StopAllSFX(void) {
    f32 zero = lbl_806004F0;
    s32 i;
    for (i = 0; i < 0x100; i++) {
        u8* rec = lbl_80559D78 + i * 0x28;
        if (*(s32*)rec == 0) {
            continue;
        }
        if (*(s32*)(rec + 0x14) != -1) {
            fn_801FF2A8(gAudio, *(s32*)(rec + 0x14));
            *(s32*)(rec + 0x14) = -1;
        }
        *(s32*)(rec + 0x0) = 0;
        *(s32*)(rec + 0x4) = lbl_805FB6B8;
        *(f32*)(rec + 0x8) = zero;
        *(f32*)(rec + 0xC) = zero;
        *(f32*)(rec + 0x10) = zero;
        *(s32*)(rec + 0x14) = -1;
        *(f32*)(rec + 0x18) = zero;
        *(s32*)(rec + 0x20) = 0;
        *(s32*)(rec + 0x24) = 0;
        *(s32*)(rec + 0x1C) = -1;
    }
    fn_801FF47C(gAudio);
}

}  // extern "C"
