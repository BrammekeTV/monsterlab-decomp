#include "types.h"
#include "game/part.h"
#include "game/script_callbacks.h"

struct StringTable;
extern void* gEventMgr;             // C++ linkage
extern void* gScreenSets;           // C++ linkage
extern StringTable* gStringTable;   // C++ linkage

extern "C" {
void    EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
int     SysSprintf(char* buf, const char* fmt, ...);
s32     RaySceneTest(void* ray);
void*   ScreenSetsCurrent(void* sets);
void*   ScreenSetFindWidget(void* set, u32 hash);
void*   UIWidgetFindChild(void* widget, u32 hash);
void    UIWidgetPlayAnim(void* w, u32 anim);
BOOL    fn_80198C34(void* set, void* w);
void*   fn_801D40E0(s32 n);
void*   fn_80209814(void* v);
void    fn_8020B024(f32 v);
void    fn_8020B078(f32 v);
void    fn_8020B160(f32 v);                              // 0x8020B160 (asm reads f1 only)
u32     fn_801EC880(u8* p, u32 hash);
void    fn_801ECDE8(u8* p, s32 idx, u32 a, s32 immediate);
void*   fn_8011AEE8(u8* out, s32 idx, u8* end);
u8* fn_8011A3BC(u8* arg0);
u8* fn_8011D1C0(u8* arg0);
void    fn_801DFEA8(u8* p, s32 a, s32 b);                // 0x801DFEA8 (script FSM set bit)
void    fn_801DFF74(u8* p, u32 idx, f32 v);              // 0x801DFF74 (asm: idx < 0x20, stores f1)
void    fn_801DFFCC(u8* p, s32 a, s32 b);                // 0x801DFFCC (script FSM add int)
void    fn_8005F204(u8* obj);
void*   fn_80069090(u8* obj);
void    fn_80074D60(s32 a);
void    fn_8000B7A8(u8* p, s32 a, s32 b);                // 0x8000B7A8 (args from asm: lbl_803C4298, 0x43, 0)
s32     fn_80119B90(void);                               // 0x80119B90 (return type guessed from tail call)

extern u8*        lbl_805FAF78;
extern u8*        lbl_805FB170;
extern f32        lbl_805FB174;
extern f32        lbl_80346C70[];
extern u8*        lbl_805FB248;
extern u8         lbl_805FB24C;
extern void*      lbl_805FBBAC;
extern u8*        lbl_805FBBE0;
extern u8         lbl_803C4298[];
extern u8         lbl_803C7270[];
extern const char lbl_805F8200[];
}

extern "C" {

// 0x8000C7C8
void GameFlowSetControlFlowActive(void) {
    fn_80074D60(1);
}

// 0x8000CE0C
void GameFlowQuitGame(void) {
    fn_8000B7A8(lbl_803C4298, 0x43, 0);
}

// 0x800A7D88
// Sends event `hash` to obj once, guarded by bit 0 of obj->sub+0x20 (bit set while the event runs).
void fn_800A7D88(void* obj, u32 hash) {
    u8* o = (u8*)obj;
    u8* sub = *(u8**)(o + 0x4);
    s32 flags = *(s32*)(sub + 0x20);
    if (!(flags & 1)) {
        *(s32*)(sub + 0x20) = flags | 1;
        EventSend(gEventMgr, o, NULL, hash, 0, 0, 1, 0);
        sub = *(u8**)(o + 0x4);
        *(s32*)(sub + 0x20) = *(s32*)(sub + 0x20) & ~1;
        return;
    }
    EventSend(gEventMgr, o, NULL, hash, 0, 0, 1, 0);
}

// 0x801731C8
// Unlinks node from its intrusive list (prev at +0, next at +4, owner list head at +8) and clears it.
void fn_801731C8(void* p) {
    u8* a = (u8*)p;
    u8** prev = (u8**)(a + 0x0);   // the fields are read again after each store, as the original does
    u8** next = (u8**)(a + 0x4);
    u8** list = (u8**)(a + 0x8);
    u8* pc = *list;
    if (pc == NULL) return;
    if (*next != NULL) {
        *(u8**)*next = *prev;
    } else {
        *(u8**)pc = *prev;
    }
    if (*prev != NULL) {
        u8* n = *next;
        *(u8**)(*prev + 0x4) = n;
    } else {
        u8* n = *next;
        *(u8**)(*list + 0x4) = n;
    }
    u8* owner = *list;
    *(s32*)(owner + 0x8) = *(s32*)(owner + 0x8) - 1;
    *prev = NULL;
    *next = NULL;
    *list = NULL;
}

// 0x8001E854
// Battery status text: formats the tier of the selected part slot into the manager's text buffer.
ScriptValue GetBatteryString(void) {
    PartDef def;
    PartDef copy;
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL) return 0;
    u8* st = (u8*)gStringTable;
    const char* fmt = lbl_805F8200;
    if (*(u8**)st != NULL && *(u32*)(st + 0xC) > 0x4BFu) {
        fmt = *(const char**)(*(u8**)st + 0x12FC);
    }
    s32 idx = *(s32*)(mgr + 0xA88);
    if (idx < 5 && idx != -1) {
        u8* slot = *(u8**)(mgr + idx * 4 + 0x944);
        if (slot != NULL) {
            PartDbGetDef(&def, gGameDb, *(s32*)slot);
            copy = def;
            // r5 is left over from the part-def copy in the original; the callee ignores it for this call.
            void* ret = fn_8011AEE8((u8*)&copy, PartGetTier((const PartInstance*)slot), (u8*)&copy + 0x4C);
            SysSprintf((char*)(mgr + 0xAF8), fmt, ret);
            return (ScriptValue)(intptr_t)(mgr + 0xAF8);
        }
    }
    return 0;
}

// 0x800AEB10
// Ends the berserk state of the grow-juice tentacle: resets the scale of the tentacle and its
// child, retargets the child, then clears the berserk pointer and stores the ray-test value.
void GrowJuiceTentacle_RecoverFromBerserk(void* a0) {
    if (lbl_805FB170 != NULL) {
        u8* t = *(u8**)(lbl_805FB170 + 0x90);
        *(f32*)(t + 0x38) = 1.0f;
        *(f32*)(t + 0x3C) = 1.0f;
        *(f32*)(t + 0x40) = 1.0f;
        *(f32*)(t + 0x44) = 1.0f;
        u8* c = *(u8**)(lbl_805FB170 + 0x26C);
        if (c != NULL) {
            u8* t2 = *(u8**)(c + 0x90);
            *(f32*)(t2 + 0x38) = 1.0f;
            *(f32*)(t2 + 0x3C) = 1.0f;
            *(f32*)(t2 + 0x40) = 1.0f;
            *(f32*)(t2 + 0x44) = 1.0f;
            u32 h = fn_801EC880(c + 0x94, 0xE2E205E8u);
            fn_801ECDE8(c + 0x94, (s32)h, 0xBA5145F9u, 0);
        }
    }
    lbl_805FB170 = NULL;
    // UNCERTAIN: r3 (ray argument) is not set on every path in the asm; NULL used here.
    lbl_805FB174 = lbl_80346C70[RaySceneTest(NULL)];
}

// 0x8013FB5C
void HideKeyboard(void) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        u8* w = (u8*)ScreenSetFindWidget(set, 0x68B693B2u);
        if (w != NULL && UIWidgetFindChild(w, 0x01F44EB8u) != NULL) {
            UIWidgetPlayAnim(w, 0x8040F3FDu);
        }
    }
}

// 0x8014029C
// Plays the hide animation on the progress screen widget if its state check passes, then turns
// the hit boxes on.
void HideProgressScreen(void) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        u8* w = (u8*)ScreenSetFindWidget(set, 0x42AFE7BCu);
        if (w != NULL && fn_80198C34(set, w) != 0) {
            UIWidgetPlayAnim(w, 0x39440BA2u);
            SetHitBoxes(1);
        }
    }
}

// 0x8013FAE0
// Plays the hide animation on the widget with hash a0 in the current screen set when its state check passes.
void HideScreen(u32 a0) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        u8* w = (u8*)ScreenSetFindWidget(set, a0);
        if (w != NULL && fn_80198C34(set, w) != 0) {
            UIWidgetPlayAnim(w, 0x39440BA2u);
        }
    }
}

// 0x8001CE10
// Moves the monster inventory cursor by a0 and wraps it into [0, count) (count from the save block).
void IncMonsterInventoryPos(s32 a0) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL) return;
    s32 pos = *(s32*)(mgr + 0xAD8);
    if (pos < 0) return;
    s32 count = *(s32*)(lbl_805FBBE0 + 0x10E8);
    if (count <= 0) return;
    *(s32*)(mgr + 0xAD8) = pos + a0;
    mgr = lbl_805FAF78;
    pos = *(s32*)(mgr + 0xAD8);
    if (pos >= count) {
        *(s32*)(mgr + 0xAD8) = pos % count;
    }
    while (*(s32*)(lbl_805FAF78 + 0xAD8) < 0) {
        *(s32*)(lbl_805FAF78 + 0xAD8) = *(s32*)(lbl_805FAF78 + 0xAD8) + count;
    }
}

// 0x800FAB44
// Music volume option: adds a0 to the saved level (clamped 0..10), applies level/10 to the audio
// bus and stores the new level.
void IncOptionsMusicVolume(s32 a0) {
    u8* sb = lbl_805FBBE0;
    s32 v = (s32)*(u16*)(sb + 0x916) + a0;
    if (v < 0) {
        v = 0;
    } else if (v > 10) {
        v = 10;
    }
    s32 q = ((u32)v < 10u) ? v : 10;
    f32 f = (f32)q / 10.0f;
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) {
            h = fn_80209814(h);
        }
        lbl_805FBBAC = h;
    }
    fn_8020B024(f);
    *(u16*)(lbl_805FBBE0 + 0x916) = (u16)v;
}

// 0x800FACC0
// Sfx volume option: adds a0 to the saved level (clamped 0..10), applies level/10, stores the level.
void IncOptionsSfxVolume(s32 a0) {
    u8* sb = lbl_805FBBE0;
    s32 v = (s32)*(u16*)(sb + 0x914) + a0;
    if (v < 0) {
        v = 0;
    } else if (v > 10) {
        v = 10;
    }
    s32 q = ((u32)v < 10u) ? v : 10;
    f32 f = (f32)q / 10.0f;
    fn_8020B078(f);
    *(u16*)(lbl_805FBBE0 + 0x914) = (u16)v;
}

// 0x800FAE74
// Voice volume option: adds a0 to the saved level (clamped 0..10), applies level/10, stores the level.
void IncOptionsVoiceVolume(s32 a0) {
    u8* sb = lbl_805FBBE0;
    s32 v = (s32)*(u16*)(sb + 0x918) + a0;
    if (v < 0) {
        v = 0;
    } else if (v > 10) {
        v = 10;
    }
    s32 q = ((u32)v < 10u) ? v : 10;
    f32 f = (f32)q / 10.0f;
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) {
            h = fn_80209814(h);
        }
        lbl_805FBBAC = h;
    }
    fn_8020B160(f);
    *(u16*)(lbl_805FBBE0 + 0x918) = (u16)v;
}

// 0x8010FDA4
// Creates the lab book section table once (-1 counter, two sub-objects), then clears lbl_805FB24C.
void InitLabBooks(void) {
    if (lbl_805FB248 == NULL) {
        u8* t = (u8*)fn_801D40E0(0xC);
        if (t != NULL) {
            *(s32*)(t + 0x0) = -1;
            *(u8**)(t + 0x4) = NULL;
            *(u8**)(t + 0x8) = NULL;
            u8* a = (u8*)fn_801D40E0(0x1C);
            if (a != NULL) {
                a = (u8*)fn_8011A3BC(a);
            }
            *(u8**)(t + 0x4) = a;
            u8* b = (u8*)fn_801D40E0(0x20);
            if (b != NULL) {
                b = (u8*)fn_8011D1C0(b);
            }
            *(u8**)(t + 0x8) = b;
        }
        lbl_805FB248 = t;
    }
    lbl_805FB24C = 0;
}

// 0x800ED2EC
// Explode dispatch: flag bit 0x40 on obj->sub+0x1C selects one handler, else bit 0x20 selects another.
void Explode(void* a0) {
    u32 flags = *(u32*)(*(u8**)((u8*)a0 + 0x4) + 0x1C);
    if (flags & 0x40) {
        fn_80069090((u8*)a0);
        return;
    }
    if (flags & 0x20) {
        fn_8005F204((u8*)a0);
    }
}

// 0x80106E38
// Script FSM set-bit: target is ctx->+0x58 when set, else the global FSM block lbl_803C7270.
void FSM_SetBit(ScriptCtx* a0, u32 a1, s32 a2) {
    u8* t = (a0 != NULL) ? *(u8**)((u8*)a0 + 0x58) : NULL;
    if (t == NULL) t = lbl_803C7270;
    fn_801DFEA8(t, (s32)a1, a2);
}

// 0x80106EC8
// Script FSM set-float: same target selection as FSM_SetBit.
void FSM_SetFloat(ScriptCtx* a0, u32 a1, f32 a2) {
    u8* t = (a0 != NULL) ? *(u8**)((u8*)a0 + 0x58) : NULL;
    if (t == NULL) t = lbl_803C7270;
    fn_801DFF74(t, a1, a2);
}

// 0x80106F10
// Script FSM add-int: same target selection as FSM_SetBit.
void FSM_AddToInt(ScriptCtx* a0, u32 a1, s32 a2) {
    u8* t = (a0 != NULL) ? *(u8**)((u8*)a0 + 0x58) : NULL;
    if (t == NULL) t = lbl_803C7270;
    fn_801DFFCC(t, (s32)a1, a2);
}

// 0x801124B0
s32 IngredientsSelectionFinished(void) {
    return fn_80119B90();
}

}  // extern "C"
