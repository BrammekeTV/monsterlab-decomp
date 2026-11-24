// Queries and small helpers from the miss25 batch (0x8000CDF4-0x801B2DFC): game flow flags,
// inventory and monster checks, opponent inspect setup, UI animation state, rumble channels.
#include "types.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

struct StringTable;

extern "C++" void* gScreenSets;          // 0x805FB654
extern "C++" StringTable* gStringTable;  // 0x805FB910

extern "C" {
u32      AlertGetLastError(void);                                        // 0x80140B58
s32 MonsterGetName(u8* m, char* out, s32 size);                     // 0x8021C968
BOOL     MonsterHasPart(u8* m, const PartInstance* p);
u8*      PartFindOwner(PartInstance* p);
s32      PartComputeActionCost(PartInstance* p, u32 actionKey, void* monster);
u8       fn_8021EE94(u8* p);                                             // 0x8021EE94
void*    fn_8011AEE8(u8* out, s32 idx, u8* end);                         // 0x8011AEE8
s32      fn_800F4564(void);                                              // 0x800F4564
s32      fn_80074D48(void);                                              // 0x80074D48 (current minigame object, may be 0)
s32      fn_80075298(u8* p);                                             // 0x80075298
void     fn_800752F4(u8* p);                                             // 0x800752F4
void*    ScreenSetsCurrent(void* sets);                                  // 0x8018D1E4
void*    ScreenSetFindWidget(void* set, u32 hash);                       // 0x80198E34
void*    ScreenSetFindElement(void* set, u32 hash);                      // 0x80198F18
void*    UIWidgetFindChild(void* widget, u32 hash);                      // 0x80199AEC

extern u8*   lbl_805FBBE0;     // 0x805FBBE0 (save/inventory block pointer)
extern u8*   lbl_805FAF78;     // 0x805FAF78 (monster block pointer)
extern s32   lbl_805FAE8C;     // 0x805FAE8C (global word, game flow flag)
extern void* lbl_805FB208[];   // 0x805FB208 (splash screen slots)
extern f32   lbl_805F9070;     // 0x805F9070 (inspect opponent values)
extern f32   lbl_805F9074;     // 0x805F9074
extern f32   lbl_805F9078;     // 0x805F9078
extern f32   lbl_805FEFA0;     // 0x805FEFA0 (sentinel constant compared against the inspect values)
extern s8    lbl_805FB390;     // 0x805FB390
extern s32   lbl_805FB394;     // 0x805FB394
extern char  lbl_803C7120[];   // 0x803C7120 (monster name buffer)
extern const char lbl_805F8200[];  // 0x805F8200 (empty string, fallback text)
extern u8    lbl_804133D8[];   // 0x804133D8 (rumble channel slots, 4 entries)
}

// Copies the 0x50-byte PartDef image, as the original does with its word loop.
static void CopyPartDef(u8* dst, const PartDef* src) {
    s32 i;
    for (i = 0; i < 0x50; i++) {
        dst[i] = ((const u8*)src)[i];
    }
}

// Shared body of InventoryPartAction1GetCost / InventoryPartAction2GetCost: absolute action cost of
// the inventory part at index a0, with the action key read from the copied def at keyOff.
static ScriptValue InventoryPartCostAbs(s32 a0, s32 keyOff) {
    PartDef def;
    u8 copy[0x50];
    PartInstance* part;
    s32 cost;
    s32 sign;

    if (a0 >= 0 && a0 < *(s32*)((u8*)lbl_805FBBE0 + 0x10EC)) {
        part = *(PartInstance**)((u8*)lbl_805FBBE0 + a0 * 4 + 0xA58);
        if (part != NULL) {
            PartDbGetDef(&def, gGameDb, part->defIndex);
            CopyPartDef(copy, &def);
            cost = PartComputeActionCost(part, *(u32*)(copy + keyOff), PartFindOwner(part));
            sign = cost >> 31;
            return (cost ^ sign) - sign;
        }
    }
    return 0;
}

extern "C" {

// 0x8000CDF4
s32 IsGameFlowQuiting(void) {
    if (lbl_805FAE8C != 0) {
        return 1;
    }
    return 0;
}

// 0x8001DF90
s32 IsInventoryMonster(void) {
    s32 idx;

    if (lbl_805FAF78 != NULL) {
        idx = *(s32*)(lbl_805FAF78 + 0xAD8);
        if (idx >= 0 && idx < *(s32*)((u8*)lbl_805FBBE0 + 0x10E8)) {
            return 1;
        }
    }
    return 0;
}

// 0x8001DDA0
// Checks the five monster slots at lbl_805FAF78+8 (stride 0x1A4): each slot must reference a
// valid entry in the save block list (+0xA58) or the check fails.
s32 IsValidMonster(void) {
    u8* m = lbl_805FAF78;
    u8* p;
    s32 n;
    s32 var;
    s32 t0;
    s32 t2;

    if (m == NULL || *(s32*)m == 0) {
        return 0;
    }
    p = m + 8;
    for (n = 5; n > 0; n--) {
        var = -1;
        if (*(s32*)(p + 0x198) > 0 && (t0 = *(s32*)(p + 0x190)) >= 0 && (t2 = *(s32*)(p + t0 * 4)) >= 0) {
            var = *(s32*)*(u8**)((u8*)lbl_805FBBE0 + t2 * 4 + 0xA58);
        }
        if (var == -1) {
            return 0;
        }
        p += 0x1A4;
    }
    return 1;
}

// 0x8001E740
// Trims trailing spaces (0x20) from the name at lbl_805FAF78+0xAE8, shrinking the length at +0xBF8.
s32 IsValidName(void) {
    u8* m = lbl_805FAF78;
    s32 n;

    if (m == NULL || *(s32*)(m + 0xBF8) <= 0) {
        return 0;
    }
    for (;;) {
        n = *(s32*)(m + 0xBF8);
        if (n <= 1 || m[n + 0xAE7] != 0x20) {
            return 1;
        }
        *(s32*)(m + 0xBF8) = n - 1;
        m[n - 1 + 0xAE8] = 0;
    }
}

// 0x800E8720
s32 IsEntityOfType(void* a0, u32 a1) {
    return a1 < 32 && (*(u32*)(*(u8**)((u8*)a0 + 0x4) + 0x1C) & (1U << a1)) != 0;   // slw by 32+ gives 0
}

// 0x800EFDBC
// Asks the current minigame whether it can exit; without a minigame the answer is yes.
s32 InstructionsCanExit(void) {
    u8* obj;

    if (fn_80074D48() != 0) {
        obj = (u8*)(intptr_t)fn_80074D48();
        return fn_80075298(obj);
    }
    return 1;
}

// 0x800EFDF4
// Tells the current minigame it has been exited.
void InstructionsExitted(void) {
    u8* obj;

    if (fn_80074D48() != 0) {
        obj = (u8*)(intptr_t)fn_80074D48();
        fn_800752F4(obj);
    }
}

// 0x800EFEBC
s32 IsUIAnimationFinished(u32 a0) {
    s32 result = 1;
    void* set;
    void* el;

    set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        el = ScreenSetFindElement(set, a0);
        if (el != NULL && *(s32*)((u8*)el + 0x10) != 0) {
            result = 0;
        }
    }
    return result;
}

// 0x800F1F04
// Name of the inventory part at index a0 (fallback text when the index or string table is unusable).
ScriptValue InventoryGetPartName(s32 a0) {
    PartDef def;
    u8 copy[0x50];
    PartInstance* part;
    u8* strs;

    if (a0 >= 0 && a0 < *(s32*)((u8*)lbl_805FBBE0 + 0x10EC)) {
        part = *(PartInstance**)((u8*)lbl_805FBBE0 + a0 * 4 + 0xA58);
        if (part != NULL) {
            PartDbGetDef(&def, gGameDb, part->defIndex);
            CopyPartDef(copy, &def);
            return (ScriptValue)fn_8011AEE8(copy, part->tier, copy + 0x4C);
        }
    }
    strs = *(u8**)gStringTable;
    if (strs != NULL && (u32)*(s32*)((u8*)gStringTable + 0xC) > 0x476U) {
        return *(s32*)(strs + 0x11D8);
    }
    return (ScriptValue)lbl_805F8200;
}

// 0x800F2A98
ScriptValue InventoryPartAction1GetCost(s32 a0) {
    return InventoryPartCostAbs(a0, 0x14);
}

// 0x800F36E8
ScriptValue InventoryPartAction2GetCost(s32 a0) {
    return InventoryPartCostAbs(a0, 0x2C);
}

// 0x800F2488
// Name of the first monster (in the monster list) that has the inventory part at index a0.
ScriptValue InventoryPartGetMonsterName(s32 a0) {
    PartInstance* part;
    u8* m;
    s32 i;
    s32 off;
    s32 count;

    if (a0 >= 0 && a0 < *(s32*)((u8*)lbl_805FBBE0 + 0x10EC)) {
        part = *(PartInstance**)((u8*)lbl_805FBBE0 + a0 * 4 + 0xA58);
        if (part != NULL && fn_8021EE94((u8*)part) != 0) {
            count = *(s32*)((u8*)lbl_805FBBE0 + 0x10E8);
            off = 0;
            for (i = 0; i < count; i++) {
                m = *(u8**)((u8*)lbl_805FBBE0 + off + 0xA30);
                if (m != NULL && MonsterHasPart(m, part)) {
                    MonsterGetName(m, lbl_803C7120, 0x10);
                    return (ScriptValue)lbl_803C7120;
                }
                off += 4;
            }
        }
    }
    return 0;
}

// 0x800FA8F8
u32 IsSplashScreenReady(BOOL a0) {
    u32 ready = 0;

    if ((u32)a0 < 2U && lbl_805FB208[a0] != NULL) {
        ready = 1;
    }
    return ready;
}

// 0x800F47DC
u32 IsSelectedActionRPSGood(void) {
    return fn_800F4564() == 0;
}

// 0x800F4804
u32 IsSelectedActionRPSNeutral(void) {
    return (fn_800F4564() - 2) == 0;
}

// 0x800F4830
u32 IsSelectedActionRPSBad(void) {
    return (fn_800F4564() - 1) == 0;
}

// 0x800F485C
u32 IsSelectedActionRPSMixed(void) {
    return (fn_800F4564() - 3) == 0;
}

// 0x8013C600
// Once per opponent inspect: caches three float values from the inspect widget (only while they
// still hold the sentinel) and selects section 0x7B89ED91. Skipped when the last alert error is 0x7B89ED91.
void InspectOpponentInit(void) {
    void* set;
    void* w;
    void* c;

    if (AlertGetLastError() != 0x7B89ED91U) {
        lbl_805FB390 = 0;
        lbl_805FB394 = 0;
        set = ScreenSetsCurrent(gScreenSets);
        if (set != NULL) {
            w = ScreenSetFindWidget(set, 0xFEE5B157U);
            if (w != NULL) {
                c = UIWidgetFindChild(w, 0x60B15D91U);
                if (c != NULL && lbl_805F9070 == lbl_805FEFA0 && lbl_805F9074 == lbl_805FEFA0 &&
                    lbl_805F9078 == lbl_805FEFA0) {
                    lbl_805F9070 = *(f32*)((u8*)c + 0x68);
                    lbl_805F9074 = *(f32*)((u8*)c + 0x6C);
                    lbl_805F9078 = *(f32*)((u8*)c + 0x70);
                }
            }
        }
        SetMPSection(0x7B89ED91U);
    }
}

// 0x801B2DDC
// Rumble channel a0: the byte at +0x10 of its slot, 0 when the slot is empty.
u32 IsRumbling(s32 a0) {
    u8* p = *(u8**)(lbl_804133D8 + a0 * 4);

    if (p != NULL) {
        return *(u8*)(p + 0x10);
    }
    return 0U;
}

}  // extern "C"
