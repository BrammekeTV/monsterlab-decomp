// Monster part database lookups and the PartQuery_* interface used by every part screen
// (inventory, lab, lightning tower, combat HUD).
// Original: PartQuery_* 0x800F9790 - 0x800FA4B4, part accessors 0x8021EF58 - 0x8021EFAC,
// PartDbGetDef 0x80216838, TextIdFromHash 0x80120D34.
#include "game/part.h"

extern "C" {
void  PartPrepare(void* lab, s32 defIndex, s32 tierIndex);   // 0x8021947C
void* LightningTowerGetMonster(void);                        // 0x8001E020
void* DefectGetTexture(s32 defect);                          // 0x80116A20
void* EnhancementGetTexture(s32 enhancement);                // 0x80116870
}

extern u8*           lbl_805FBBC8;            // lab state; +0x3C = selected part
extern u8*           gSaveSystem;             // 0x805FBBE0: +0xA58 inventory, +0x10EC count
static PartInstance* sQueryPart;              ORIG(0x805FB1F8)
static void*         sQueryMonster;           ORIG(0x805FB1FC)
static void*         sQueryTexture;           ORIG(0x805FB200)

#define TEXT_UNKNOWN_PART 0x475
#define TEXT_NONE         0x557

// Sorted (hash, text id) pairs for every text key (0x10B2 entries).
struct TextKey { u32 hash; s32 id; };
extern const TextKey sTextKeys[0x10B2];       ORIG(0x8036D370)

// 0x80120D34
// Binary search of the text-key table.
extern "C" s32 TextIdFromHash(u32 hash) {
    s32 lo = 0, hi = 0x10B1;
    s32 mid = 0x858;
    while (lo <= hi) {
        u32 h = sTextKeys[mid].hash;
        if (h > hash)      hi = mid - 1;
        else if (h < hash) lo = mid + 1;
        else               return sTextKeys[mid].id;
        mid = lo + (hi - lo) / 2;
    }
    return -1;
}

// 0x80216838
// Copy part definition `index`. If only one of the two actions is set, it is used for both.
extern "C" void PartDbGetDef(PartDef* out, GameDb* db, s32 index) {
    PartDef def = (*(PartDef**)(db->data + 0x5B78))[index];
    if (def.action1 == -1) {
        if (def.action2 == -1) {
            def.action1 = 0;
            def.action2 = 0;
        } else {
            def.action1 = def.action2;
        }
    } else if (def.action2 == -1) {
        def.action2 = def.action1;
    }
    *out = def;
}

// 0x8021EF58
extern "C" u8 PartGetTier(const PartInstance* p) {
    return p->tier;
}

// 0x8021EF60
extern "C" s8 PartGetQuality(const PartInstance* p) {
    return p->quality;
}

// 0x8021EF80
// Madness the part adds to a monster: base madness scaled by the current tier's percentage.
extern "C" s8 PartGetMadness(const PartInstance* p) {
    u32 scaled = (u32)((s32)p->madnessBase * (s32)p->tierScale[p->tier - 1]);
    return (s8)(u8)(scaled / 100);
}

// 0x800F9790
// Query the part currently selected in the lab.
void PartQuery_SetPart(void) {
    PartInstance* p = lbl_805FBBC8 ? *(PartInstance**)(lbl_805FBBC8 + 0x3C) : NULL;
    sQueryPart = p;
    if (p != NULL) PartPrepare(lbl_805FBBC8, p->defIndex, p->tier - 1);
    sQueryMonster = NULL;
}

// 0x800F97E8
// Query inventory slot `index`.
void PartQuery_SetInventoryPartByIndex(s32 index) {
    PartInstance* p = NULL;
    if (index >= 0 && index < *(s32*)(gSaveSystem + 0x10EC)) p = ((PartInstance**)(gSaveSystem + 0xA58))[index];
    sQueryPart = p;
    if (p != NULL) PartPrepare(lbl_805FBBC8, p->defIndex, p->tier - 1);
    sQueryMonster = NULL;
}

// 0x800F985C
void PartQuery_SetLightningTowerMonster(void) {
    sQueryMonster = LightningTowerGetMonster();
}

// 0x800F9880
s32 PartQuery_GetMadness(void) {
    return sQueryPart ? PartGetMadness(sQueryPart) : 0;
}

// 0x800F98B8
s32 PartQuery_GetQuality(void) {
    return sQueryPart ? PartGetQuality(sQueryPart) : 0;
}

// 0x800F98F0
// Text id of the part's name for its tier (e.g. "Gorilla Arm" for tier 3 of part 0).
s32 PartQuery_GetName(void) {
    if (sQueryPart == NULL) return TEXT_UNKNOWN_PART;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    switch (PartGetTier(sQueryPart)) {
    case 1: return TextIdFromHash(def.tierName[0]);
    case 2: return TextIdFromHash(def.tierName[1]);
    case 3: return TextIdFromHash(def.tierName[2]);
    case 4: return TextIdFromHash(def.tierName[3]);
    }
    return TEXT_UNKNOWN_PART;
}

// 0x800F9AE4
void* PartQuery_GetDefectTexture(void) {
    if (sQueryPart == NULL || sQueryPart->defect == -1) return NULL;
    return DefectGetTexture(sQueryPart->defect);
}

// 0x800F9B08
void* PartQuery_GetEnhancementTexture(void) {
    if (sQueryPart == NULL || sQueryPart->enhancement == -1) return NULL;
    return EnhancementGetTexture(sQueryPart->enhancement);
}

// 0x800F9B2C
s32 PartQuery_GetHealth(void) {
    if (sQueryPart == NULL) return 0;
    return PartGetMaxHealth(sQueryPart);
}

// 0x800FA350
void* PartQuery_GetTexture(void) {
    return sQueryTexture;
}

extern "C" {
f32 PartComputeActionCost(PartInstance* p, u32 actionKey, void* monster);   // 0x8021E6EC
s32 __cvt_fp2signed(f32 v);                                                   // 0x80332ED0
}

static inline PartAction* ActionTable(void) {
    return *(PartAction**)(gGameDb->data + 0x430);
}

static inline PartActionType* ActionTypeTable(void) {
    return *(PartActionType**)(gGameDb->data + 0x08);
}

// 0x800F9B44
// Text id of action `which` (0 or 1) of the queried part.
s32 PartQuery_GetActionName(u32 which) {
    if (which >= 2) return 0;
    if (sQueryPart == NULL) return TEXT_NONE;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    s32 action = which == 0 ? def.action1 : def.action2;
    PartAction a = ActionTable()[action];
    PartActionType t = ActionTypeTable()[a.type];
    return TextIdFromHash(t.nameHash);
}

// 0x800F9E0C
// Energy cost of action `which` for the queried part (on sQueryMonster, if set).
s32 PartQuery_GetActionCost(u32 which) {
    if (which >= 2) return 0;
    if (sQueryPart == NULL) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    s32 action = which == 0 ? def.action1 : def.action2;
    PartAction a = ActionTable()[action];
    return __cvt_fp2signed(PartComputeActionCost(sQueryPart, a.key, sQueryMonster));
}

static inline BOOL QueryDisciplineIs(s32 discipline) {
    if (sQueryPart == NULL) return FALSE;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    return def.discipline == discipline;
}

// 0x800FA358
BOOL PartQuery_PartIsMech(void) {
    return QueryDisciplineIs(PART_MECH);
}

// 0x800FA3CC
BOOL PartQuery_PartIsBio(void) {
    return QueryDisciplineIs(PART_BIO);
}

// 0x800FA440
BOOL PartQuery_PartIsAlchemy(void) {
    return QueryDisciplineIs(PART_ALCHEMY);
}
