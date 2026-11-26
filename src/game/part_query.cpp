// Monster part database lookups and the PartQuery_* interface used by every part screen
// (inventory, lab, lightning tower, combat HUD).
// Original: PartQuery_* 0x800F9790 - 0x800FA4B4, part accessors 0x8021EF58 - 0x8021EFAC,
// PartDbGetDef 0x80216838, TextIdFromHash 0x80120D34.
#include "game/part.h"

extern "C" {
void  PartComputeRequiredRank(void* lab, s32 defIndex, s32 tierIndex);   // 0x8021947C
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

extern "C" s32 PortModPartsTextId(u32 hash);   // src/port/port_mod_parts.cpp

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
    return PortModPartsTextId(hash);   // PC port: -1 as in the original, or the name of a part a mod added
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

extern "C" void PartQuery_SetPart(void);
// 0x800F9790
// Query the part currently selected in the lab.
void PartQuery_SetPart(void) {
    PartInstance* p = lbl_805FBBC8 ? *(PartInstance**)(lbl_805FBBC8 + 0x3C) : NULL;
    sQueryPart = p;
    if (p != NULL) PartComputeRequiredRank(lbl_805FBBC8, p->defIndex, p->tier - 1);
    sQueryMonster = NULL;
}

extern "C" void PartQuery_SetInventoryPartByIndex(s32 index);
// 0x800F97E8
// Query inventory slot `index`.
void PartQuery_SetInventoryPartByIndex(s32 index) {
    PartInstance* p = NULL;
    if (index >= 0 && index < *(s32*)(gSaveSystem + 0x10EC)) p = ((PartInstance**)(gSaveSystem + 0xA58))[index];
    sQueryPart = p;
    if (p != NULL) PartComputeRequiredRank(lbl_805FBBC8, p->defIndex, p->tier - 1);
    sQueryMonster = NULL;
}

extern "C" void PartQuery_SetLightningTowerMonster(void);
// 0x800F985C
void PartQuery_SetLightningTowerMonster(void) {
    sQueryMonster = LightningTowerGetMonster();
}

extern "C" s32 PartQuery_GetMadness(void);
// 0x800F9880
s32 PartQuery_GetMadness(void) {
    return sQueryPart ? PartGetMadness(sQueryPart) : 0;
}

extern "C" s32 PartQuery_GetQuality(void);
// 0x800F98B8
s32 PartQuery_GetQuality(void) {
    return sQueryPart ? PartGetQuality(sQueryPart) : 0;
}

extern "C" s32 PartQuery_GetName(void);
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

extern "C" void* PartQuery_GetDefectTexture(void);
// 0x800F9AE4
void* PartQuery_GetDefectTexture(void) {
    if (sQueryPart == NULL || sQueryPart->defect == -1) return NULL;
    return DefectGetTexture(sQueryPart->defect);
}

extern "C" void* PartQuery_GetEnhancementTexture(void);
// 0x800F9B08
void* PartQuery_GetEnhancementTexture(void) {
    if (sQueryPart == NULL || sQueryPart->enhancement == -1) return NULL;
    return EnhancementGetTexture(sQueryPart->enhancement);
}

extern "C" s32 PartQuery_GetHealth(void);
// 0x800F9B2C
s32 PartQuery_GetHealth(void) {
    if (sQueryPart == NULL) return 0;
    return PartGetMaxHealth(sQueryPart);
}

extern "C" void* PartQuery_GetTexture(void);
// 0x800FA350
void* PartQuery_GetTexture(void) {
    return sQueryTexture;
}

extern "C" {
s32 PartComputeActionCost(PartInstance* p, u32 actionKey, void* monster);   // 0x8021E6EC
s32 __cvt_fp2signed(s32 v);                                                   // 0x80332ED0 (integer abs)
}

static inline PartAction* ActionTable(void) {
    return *(PartAction**)(gGameDb->data + 0x430);
}

static inline PartActionType* ActionTypeTable(void) {
    return *(PartActionType**)(gGameDb->data + 0x08);
}

extern "C" s32 PartQuery_GetActionName(u32 which);
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

extern "C" s32 PartQuery_GetActionCost(u32 which);
// 0x800F9E0C
// Energy cost of action `which` for the queried part (on sQueryMonster, if set).
s32 PartQuery_GetActionCost(u32 which) {
    if (which >= 2) return 0;
    if (sQueryPart == NULL) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    s32 action = which == 0 ? def.action1 : def.action2;
    PartAction a = ActionTable()[action];
    return PartComputeActionCost(sQueryPart, a.key, sQueryMonster);
}

static inline BOOL QueryDisciplineIs(s32 discipline) {
    if (sQueryPart == NULL) return FALSE;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    return def.discipline == discipline;
}

extern "C" BOOL PartQuery_PartIsMech(void);
// 0x800FA358
BOOL PartQuery_PartIsMech(void) {
    return QueryDisciplineIs(PART_MECH);
}

extern "C" BOOL PartQuery_PartIsBio(void);
// 0x800FA3CC
BOOL PartQuery_PartIsBio(void) {
    return QueryDisciplineIs(PART_BIO);
}

extern "C" BOOL PartQuery_PartIsAlchemy(void);
// 0x800FA440
BOOL PartQuery_PartIsAlchemy(void) {
    return QueryDisciplineIs(PART_ALCHEMY);
}

extern "C" {
s32   PartActionStat0(PartInstance* p, u32 actionKey);               // 0x8021DBE8
s32   PartActionStat1(void* p, u32 actionKey);                       // 0x8021DE10
BOOL  PartAllowedAtRank(void* coll, s32 defIndex, s32 tier);         // 0x802191CC
void  TargetListInit(void* t);                                       // 0x802161DC
void  TargetListDecode(void* t, const u32* mask, s32 side);          // 0x80216240
void* TargetListDestroy(void* t, s32 del);                           // 0x80216200
void* fn_8011AD88(void* def, s32 flag);                              // 0x8011AD88
void  TextureRelease(void* tex);                                     // 0x80184028
}
extern void* gLab;                                                   // 0x805FBBC8

extern "C" s32 PartQuery_GetDefectName(void);
// 0x800F99B4
// Text id of the queried part's defect name (0 if none).
s32 PartQuery_GetDefectName(void) {
    if (sQueryPart == NULL || sQueryPart->defect == -1) return 0;
    u8* e = *(u8**)(gGameDb->data + 0x31E8) + (sQueryPart->defect << 5);
    return TextIdFromHash(*(u32*)(e + 4));
}

extern "C" s32 PartQuery_GetEnhancementName(void);
// 0x800F9A48
// Text id of the queried part's enhancement name (0 if none).
s32 PartQuery_GetEnhancementName(void) {
    if (sQueryPart == NULL || sQueryPart->enhancement == -1) return 0;
    u8* e = *(u8**)(gGameDb->data + 0x3610) + sQueryPart->enhancement * 0x24;
    return TextIdFromHash(*(u32*)(e + 4));
}

// Action `which` (0/1) of the queried part and its action type (copied like the original).
extern "C" s32 PartQuery_GetActionDamage(BOOL which);
// 0x800F9C7C
// Damage stat of action `which`: abs of stat 0 or stat 1 depending on the action type.
s32 PartQuery_GetActionDamage(BOOL which) {
    if ((u32)which >= 2) return 0;
    if (sQueryPart == NULL) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    PartDef def2 = def;
    s32 action = which == 0 ? def2.action1 : def2.action2;
    PartAction a = ActionTable()[action];
    PartActionType t = ActionTypeTable()[a.type];
    if (t.unk00[2] == 0) {
        return __cvt_fp2signed(PartActionStat0(sQueryPart, (u32)(which == 0 ? def2.action1 : def2.action2)));
    }
    if (t.unk00[2] == 1) {
        return __cvt_fp2signed(PartActionStat1(sQueryPart, (u32)(which == 0 ? def2.action1 : def2.action2)));
    }
    return 0;
}

extern "C" s32 PartQuery_IsActionAttack(BOOL which);
// 0x800F9EEC
// 1 if action `which` has action-type stat 0 (an attack).
s32 PartQuery_IsActionAttack(BOOL which) {
    if ((u32)which >= 2) return 0;
    if (sQueryPart == NULL) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    PartDef def2 = def;
    s32 action = which == 0 ? def2.action1 : def2.action2;
    PartAction a = ActionTable()[action];
    PartActionType t = ActionTypeTable()[a.type];
    if (t.unk00[2] == 0) return 1;
    return 0;
}

extern "C" s32 PartQuery_IsActionDefense(BOOL which);
// 0x800FA02C
// 1 if action `which` has a non-zero action-type stat (a defensive action).
s32 PartQuery_IsActionDefense(BOOL which) {
    if ((u32)which >= 2) return 0;
    if (sQueryPart == NULL) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    PartDef def2 = def;
    s32 action = which == 0 ? def2.action1 : def2.action2;
    PartAction a = ActionTable()[action];
    PartActionType t = ActionTypeTable()[a.type];
    if (t.unk00[2] == 0) return 0;
    return 1;
}

extern "C" s32 PartQuery_AttackTargetsLimb(BOOL which, BOOL limb);
// 0x800FA174
// 1 if action `which` of the queried part can target limb `limb`.
s32 PartQuery_AttackTargetsLimb(BOOL which, BOOL limb) {
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    PartDef def2 = def;
    s32 action = which == 0 ? def2.action1 : def2.action2;
    PartAction a = ActionTable()[action];
    s32 list[6];                      // target list: entries inline, count at +0x14
    TargetListInit(list);
    u32 mask = a.unk08;
    TargetListDecode(list, &mask, def2.slot);
    for (s32 i = 0; i < list[5]; i++) {
        if (limb == list[i]) {
            TargetListDestroy(list, -1);
            return 1;
        }
    }
    TargetListDestroy(list, -1);
    return 0;
}

extern "C" void PartQuery_LoadTexture(void);
// 0x800FA29C
// Load the icon texture of the queried part's definition.
void PartQuery_LoadTexture(void) {
    if (sQueryPart == NULL) return;
    PartDef def;
    PartDbGetDef(&def, gGameDb, sQueryPart->defIndex);
    PartDef def2 = def;
    sQueryTexture = fn_8011AD88(&def2, 1);
}

extern "C" void PartQuery_ReleaseTexture(void);
// 0x800FA308
// Drop the icon texture reference (frees it at zero).
void PartQuery_ReleaseTexture(void) {
    if (sQueryTexture == NULL) return;
    s32 refs = *(s32*)((u8*)sQueryTexture + 0x24) - 1;
    *(s32*)((u8*)sQueryTexture + 0x24) = refs;
    if (refs <= 0 && refs == 0) TextureRelease(sQueryTexture);
    sQueryTexture = NULL;
}

extern "C" s32 PartQuery_PlayerNeedsMoreRanks(void);
// 0x800FA4B4
// 1 if the lab rank does not yet allow the queried part's tier.
s32 PartQuery_PlayerNeedsMoreRanks(void) {
    if (sQueryPart != NULL && PartAllowedAtRank(gLab, sQueryPart->defIndex, sQueryPart->tier - 1) == 0) return 1;
    return 0;
}
