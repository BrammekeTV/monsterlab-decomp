// Part script callbacks (PartAction1/2 name and attack/defense queries, enhancement and secondary
// ingredient lookups), verified against the asm (0x800F255C - 0x800F434C).
#include "types.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct StringTable {
    const char** strings;
    u8           pad[8];
    u32          count;     // 0x0C
};

extern StringTable* gStringTable;    ORIG(0x805FB910)
extern StringTable* gStringTable2;   ORIG(0x805FB90C)
extern u8*          gSaveSystem;     ORIG(0x805FBBE0)

extern "C" {
BOOL  SavePartTierOwned(u8* s, u32 part, s32 tier);   // 0x802241F0
u8    fn_80112FFC(void);                              // 0x80112FFC
u32   fn_80114E14(void);                              // 0x80114E14
void* PartFormatSecondaryIconName(s32 index);                         // 0x80118A6C: texture of ingredient record `index`

extern const char lbl_80354800[];   // 0x80354800 ("Not found")
extern const char lbl_8035480C[];   // 0x8035480C ("NOT FOUND")
extern const char lbl_805F8200[];   // 0x805F8200 (empty string)
}

#define TEXT_UNKNOWN_PART 0x475

// Resolve a text id from TextIdFromHash; bit 31 selects the secondary table.
static inline ScriptValue LookupText(s32 id) {
    if ((u32)(id + 0x10000) == 0xFFFFu) return (ScriptValue)lbl_80354800;
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        u32 index = (u32)id & 0x7FFFFFFFu;
        if (gStringTable2->strings != NULL && index < gStringTable2->count)
            return (ScriptValue)gStringTable2->strings[index];
        return (ScriptValue)lbl_805F8200;
    }
    if (gStringTable->strings != NULL && (u32)id < gStringTable->count)
        return (ScriptValue)gStringTable->strings[(u32)id];
    if ((u32)(id + 0x10000) == 0xFFFFu) return (ScriptValue)lbl_8035480C;
    return (ScriptValue)lbl_805F8200;
}

// Action record `index` and its type record, from the game database.
static inline const PartAction* DbAction(s32 index) {
    return &(*(PartAction**)(gGameDb->data + 0x430))[index];
}
static inline const PartActionType* DbActionType(s32 index) {
    return &(*(PartActionType**)(gGameDb->data + 0x8))[index];
}

extern "C" {

// 0x800F255C
// Display name of the part's first action; "unknown part" text for an invalid part index.
ScriptValue PartGetPartAction1(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        PartAction act = *DbAction(def.action1);
        PartActionType type = *DbActionType(act.type);
        return LookupText(TextIdFromHash(type.nameHash));
    }
    if (gStringTable->strings != NULL && gStringTable->count > TEXT_UNKNOWN_PART)
        return (ScriptValue)gStringTable->strings[TEXT_UNKNOWN_PART];
    return (ScriptValue)lbl_805F8200;
}

// 0x800F2734
// Is the part's first action an attack (action type kind 0)?
s32 PartIsAction1Attack(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        PartAction act = *DbAction(def.action1);
        PartActionType type = *DbActionType(act.type);
        if ((s32)type.unk00[2] == 0) return 1;
    }
    return 0;
}

// 0x800F2854
// Is the part's first action a defense (action type kind 1)?
s32 PartIsAction1Defense(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        PartAction act = *DbAction(def.action1);
        PartActionType type = *DbActionType(act.type);
        if ((s32)type.unk00[2] == 1) return 1;
    }
    return 0;
}

// 0x800F31AC
// Display name of the part's second action; "unknown part" text for an invalid part index.
ScriptValue PartGetPartAction2(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        PartAction act = *DbAction(def.action2);
        PartActionType type = *DbActionType(act.type);
        return LookupText(TextIdFromHash(type.nameHash));
    }
    if (gStringTable->strings != NULL && gStringTable->count > TEXT_UNKNOWN_PART)
        return (ScriptValue)gStringTable->strings[TEXT_UNKNOWN_PART];
    return (ScriptValue)lbl_805F8200;
}

// 0x800F3384
// Is the part's second action an attack (action type kind 0)?
s32 PartIsAction2Attack(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        PartAction act = *DbAction(def.action2);
        PartActionType type = *DbActionType(act.type);
        if ((s32)type.unk00[2] == 0) return 1;
    }
    return 0;
}

// 0x800F34A4
// Is the part's second action a defense (action type kind 1)?
s32 PartIsAction2Defense(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        PartAction act = *DbAction(def.action2);
        PartActionType type = *DbActionType(act.type);
        if ((s32)type.unk00[2] == 1) return 1;
    }
    return 0;
}

// 0x800F3E04
// Does the part have an enhancement? On the recipe/parts screens the matching enhancement
// entry only counts if the player owns the part at the screen's selected tier.
s32 PartHasEnhancement(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        s32 enhancement = def.unk20;
        for (s32 i = 0; i < 0x9C; i++) {
            u8* entry = *(u8**)(gGameDb->data + 0x6C18) + i * 0x24;
            if (*(s32*)(entry + 0x20) != a0) continue;
            if (CurrentScreen_Recipes() != 0) {
                if (SavePartTierOwned(gSaveSystem, i, (u8)fn_80114E14()) == 0) return 0;
            } else if (CurrentScreen_Parts() == 1) {
                if (SavePartTierOwned(gSaveSystem, i, fn_80112FFC()) == 0) return 0;
            }
            if (enhancement != -1) return 1;
        }
    }
    return 0;
}

// 0x800F3F80
// Display name of the part's enhancement, or NULL if it has none.
ScriptValue PartGetEnhancementName(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a0);
        s32 enhancement = def.unk20;
        if (enhancement != -1) {
            u8* entry = *(u8**)(gGameDb->data + 0x3610) + enhancement * 0x24;
            return LookupText(TextIdFromHash(*(u32*)(entry + 0x4)));
        }
    }
    return 0;
}

// 0x800F4168
// Texture of the secondary ingredient record matching the part's entry and the screen's tier.
ScriptValue PartGetSecondaryIngredientTexture(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        for (s32 i = 0; i < 0x9C; i++) {
            u8* entry = *(u8**)(gGameDb->data + 0x6C18) + i * 0x24;
            if (*(s32*)(entry + 0x20) != a0) continue;
            s32 tier = 0;
            if (CurrentScreen_Recipes() != 0) tier = (s32)fn_80114E14();
            else if (CurrentScreen_Parts() == 1) tier = (s32)fn_80112FFC();
            for (s32 j = 0; j < 0x3C; j++) {
                u8* rec = *(u8**)(gGameDb->data + 0x5328) + j * 0x18;
                if (*(s32*)(rec + 0x8) == *(s32*)(entry + 0x14) &&
                    *(s32*)(rec + 0xC) == *(s32*)(entry + 0x8) &&
                    *(s8*)(rec + 0x13) == tier)
                    return (ScriptValue)PartFormatSecondaryIconName(j);
            }
        }
    }
    return 0;
}

// 0x800F434C
// Name of the secondary ingredient record matching the part's entry and the screen's tier.
ScriptValue PartGetSecondaryIngredientName(s32 a0) {
    if (a0 >= 0 && a0 < 0xBA) {
        for (s32 i = 0; i < 0x9C; i++) {
            u8* entry = *(u8**)(gGameDb->data + 0x6C18) + i * 0x24;
            if (*(s32*)(entry + 0x20) != a0) continue;
            s32 tier = 0;
            if (CurrentScreen_Recipes() != 0) tier = (s32)fn_80114E14();
            else if (CurrentScreen_Parts() == 1) tier = (s32)fn_80112FFC();
            for (s32 j = 0; j < 0x3C; j++) {
                u8* rec = *(u8**)(gGameDb->data + 0x5328) + j * 0x18;
                if (*(s32*)(rec + 0x8) == *(s32*)(entry + 0x14) &&
                    *(s32*)(rec + 0xC) == *(s32*)(entry + 0x8) &&
                    *(s8*)(rec + 0x13) == tier)
                    return LookupText(TextIdFromHash(*(u32*)(rec + 0x4)));
            }
        }
    }
    return 0;
}

}  // extern "C"
