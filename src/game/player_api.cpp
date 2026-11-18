// Script callbacks for the player's save: quests, badges, monsters and parts, ingredients,
// quest items, rewards and regions. Each forwards to the save object (gSaveSystem) and returns
// a default when no save is loaded.
// Original: 0x802212C8 - 0x80221C90 (auto_03_8020F8E0_text.s)
#include "types.h"
#include "game/part.h"
#include "game/script_callbacks.h"

extern "C" {
void SaveSetQuestFlag(u8* s, u32 quest, BOOL flag, s32 value);   // 0x802227A4
void SaveSetQuestLevel(u8* s, u32 quest, s32 level);              // 0x802228D4
s32  SaveGetQuestFlag(u8* s, u32 quest, BOOL flag);               // 0x802228EC
s32  SaveGetQuestLevel(u8* s, u32 quest);                         // 0x80222974
BOOL SaveIsQuestActive(u8* s, u32 quest);                         // 0x80222D5C
void SaveQueueEvent(u8* s, u32 a, u32 b, u32 c, s32 d, s32 e, s32 f);   // 0x8022DEE8
void SaveAddBadge(u8* s, u32 badge);                              // 0x802233D4
void SaveRemoveBadge(u8* s, u32 badge);                           // 0x802237AC
u32  SaveHasBadge(u8* s, u32 badge);                              // 0x80223308
s32  SaveCountBadges(u8* s);                                      // 0x802233CC
void SaveAddQuestReward(u8* s, u32 reward);                       // 0x80224258
void SaveGiveQuestReward(u8* s, u32 reward);                      // 0x8022497C
void SaveRewardQuest(u8* s, u32 quest);                           // 0x80224818
void SaveRemoveQuestReward(u8* s, u32 reward);                    // 0x80224288
void SaveRewardChallenge(u8* s, u32 challenge);                   // 0x802252D4
void SaveRewardExperiment(u8* s, u32 experiment, BOOL a);         // 0x80224A34
void SaveRewardCombat(u8* s);                                     // 0x802255F0
u32  SaveIsRegionReached(u8* s, u8 region);                       // 0x80223134
void SaveSetRegionReached(u8* s, u8 region, BOOL reached);        // 0x802231A0
s32  SaveGetNextRegion(u8* s);                                    // 0x80225654
PartInstance* PartInventoryAlloc(u8* inv);                        // 0x80219AE8
void LabReleaseMonster(u8* inv, u8* m);                           // 0x80219908
void LabReleasePart(u8* inv, PartInstance* p);                    // 0x80219B94
BOOL LabAddMaterial(u8* inv, s32 index, s32 kind, s32 amount, BOOL notify);   // 0x80219C44
BOOL LabTakeMaterial(u8* inv, s32 index, s32 kind, u32 amount);   // 0x80219ED4
BOOL LabHasMaterial(u8* inv, s32 index, s32 kind, u32 amount);    // 0x80219F14
void PartSetDef(PartInstance* p, s32 def);                        // 0x8021D43C
void PartSetTier(PartInstance* p, u8 tier);                       // 0x8021DA68
void PartSetQuality(PartInstance* p, s32 q);                      // 0x8021DA40
void PartSetHealth(PartInstance* p, s32 hp);                      // 0x8021D8EC
s32  PartGetMaxHealth(PartInstance* p);                           // 0x8021F168
}

extern u8* gSaveSystem;     // 0x805FBBE0

#define S         gSaveSystem
#define INV       (gSaveSystem + 0xA30)
#define INV_PARTS ((PartInstance**)(gSaveSystem + 0xA58))
#define MATERIAL_KEY        0
#define MATERIAL_SECONDARY  1

// Quest items: a 128-bit set, items 0..63 at +0x10E0 and 64..127 at +0x10D8.
static inline u64* QuestItemWord(u8 item) { return (u64*)(S + (item < 0x40 ? 0x10E0 : 0x10D8)); }
static inline u64  QuestItemBit(u8 item) { return (u64)1 << (item < 0x40 ? item : item - 0x40); }

// 0x802212C8
BOOL GetNumBattlesLostConsecutively(void) { return S == NULL ? 0 : *(s32*)(S + 0x8F8); }

// 0x802212E4
void SetQuestFlag(u32 quest, BOOL flag, s32 value) { if (S != NULL) SaveSetQuestFlag(S, quest, flag, value); }

// 0x80221310
void SetQuestLevel(u32 quest, s32 level) { if (S != NULL) SaveSetQuestLevel(S, quest, level); }

// 0x80221334
s32 GetQuestFlag(u32 quest, BOOL flag) { return S == NULL ? 1 : SaveGetQuestFlag(S, quest, flag); }

// 0x80221360
s32 GetQuestLevel(u32 quest) { return S == NULL ? 1 : SaveGetQuestLevel(S, quest); }

// 0x80221384
s32 IsQuestAtLevel(u32 quest, BOOL level) {
    if (S == NULL) return 1;
    return SaveGetQuestLevel(S, quest) == level;
}

// 0x802213F8
s32 IsQuestFlagTrue(u32 quest, BOOL flag) {
    if (S == NULL) return 1;
    return SaveGetQuestFlag(S, quest, flag) != 0;
}

// 0x80221458
s32 IsQuestFlagFalse(u32 quest, BOOL flag) {
    if (S == NULL) return 1;
    return SaveGetQuestFlag(S, quest, flag) == 0;
}

// 0x802214B8
s32 IsActiveQuest(u32 quest) {
    if (S == NULL) return 1;
    return SaveIsQuestActive(S, quest) != 0;
}

// 0x80221510
void QueueGameEvent(u32 a0, u32 a1, u32 a2, s32 a3, s32 a4) { SaveQueueEvent(S, a0, a1, a2, a3, a4, -1); }

// 0x80221540
void AddBadge(u32 badge) { if (S != NULL) SaveAddBadge(S, badge); }

// 0x8022155C
void RemoveBadge(u32 badge) { if (S != NULL) SaveRemoveBadge(S, badge); }

// 0x80221578
u32 HasBadge(u32 badge) { return S == NULL ? 1 : SaveHasBadge(S, badge); }

// 0x8022159C
s32 GetNumBadges(void) { return S == NULL ? 1 : SaveCountBadges(S); }

// 0x802215BC
void RemovePlayerMonster(u32 monster) { if (S != NULL) LabReleaseMonster(INV, (u8*)(uintptr_t)monster); }

// 0x802215D8
u32 GetPlayerMonster(BOOL index) { return S == NULL ? 0 : *(u32*)(S + 0xA30 + index * 4); }

// 0x802215FC
// Add a new part with the given definition, tier and quality, at full health.
void AddPartToPlayerInventory(u32 def, s32 tier, s32 quality) {
    if (S == NULL) return;
    PartInstance* p = PartInventoryAlloc(INV);
    if (p != NULL) PartSetDef(p, (s32)def);
    PartSetTier(p, (u8)tier);
    PartSetQuality(p, (s8)(u8)quality);
    PartSetHealth(p, PartGetMaxHealth(p));
}

// 0x8022169C
void AddPlayerPart(u32 def) {
    if (S == NULL) return;
    PartInstance* p = PartInventoryAlloc(INV);
    if (p != NULL) PartSetDef(p, (s32)def);
}

// 0x802216E8
void RemovePlayerPart(u32 part) { if (S != NULL) LabReleasePart(INV, (PartInstance*)(uintptr_t)part); }

// 0x80221704
u32 GetPlayerPart(BOOL index) { return S == NULL ? 0 : *(u32*)(S + 0xA58 + index * 4); }

// 0x80221728
s32 GetNumPlayerMonsters(void) { return S == NULL ? 1 : *(s32*)(S + 0x10E8); }

// 0x80221744
s32 GetNumPlayerParts(void) { return S == NULL ? 1 : *(s32*)(S + 0x10EC); }

// 0x80221760
u32 IsPlayerPartInventoryFull(void) { return S == NULL ? 1 : *(s32*)(S + 0x10EC) >= 100; }

// 0x80221790
void AddPlayerKeyIngredient(u32 i) { if (S != NULL) LabAddMaterial(INV, (s32)i, MATERIAL_KEY, 1, TRUE); }

// 0x802217B8
void RemovePlayerKeyIngredient(u32 i) { if (S != NULL) LabTakeMaterial(INV, (s32)i, MATERIAL_KEY, 1); }

// 0x802217DC
u32 PlayerHasKeyIngredient(u32 i) { return S == NULL ? 1 : LabHasMaterial(INV, (s32)i, MATERIAL_KEY, 1); }

// 0x80221808
void AddPlayerSecondaryIngredient(u32 i) { if (S != NULL) LabAddMaterial(INV, (s32)i, MATERIAL_SECONDARY, 1, TRUE); }

// 0x80221830
void RemovePlayerSecondaryIngredient(u32 i) { if (S != NULL) LabTakeMaterial(INV, (s32)i, MATERIAL_SECONDARY, 1); }

// 0x80221854
// Any owned part with this definition (and this tier, unless tier is 0).
s32 PlayerHasPart(u32 def, s32 tier) {
    if (S == NULL) return 1;
    BOOL found = FALSE;
    s32 n = *(s32*)(S + 0x10EC);
    for (s32 i = 0; i < n; i++) {
        PartInstance* p = INV_PARTS[i];
        if ((u32)p->defIndex == def && (tier == 0 || p->tier == tier)) found = TRUE;
    }
    return found;
}

// 0x802218B4
u32 PlayerHasSecondaryIngredient(u32 i) { return S == NULL ? 1 : LabHasMaterial(INV, (s32)i, MATERIAL_SECONDARY, 1); }

// 0x802218E0
void AddPlayerQuestItem(u32 item) {
    if (S == NULL) return;
    *QuestItemWord((u8)item) |= QuestItemBit((u8)item);
    (*(s32*)(S + 0x10F0))++;
}

// 0x80221978
void RemovePlayerQuestItem(u32 item) {
    if (S == NULL) return;
    *QuestItemWord((u8)item) &= ~QuestItemBit((u8)item);
    (*(s32*)(S + 0x10F0))--;
}

// 0x80221A20
s32 PlayerHasQuestItem(u32 item) {
    if (S == NULL) return 1;
    u8 i = (u8)item;
    return (s32)((*QuestItemWord(i) >> (i < 0x40 ? i : i - 0x40)) & 1);
}

// 0x80221A9C
s32 GetNumPlayerQuestItems(void) { return S == NULL ? 1 : *(s32*)(S + 0x10F0); }

// 0x80221AB8
void AddPlayerQuestReward(u32 r) { if (S != NULL) SaveAddQuestReward(S, r); }

// 0x80221AD4
void GivePlayerQuestReward(u32 r) { SaveGiveQuestReward(S, r); }

// 0x80221AE0
void RewardPlayerQuest(u32 q) { SaveRewardQuest(S, q); }

// 0x80221AEC
void RemovePlayerQuestReward(u32 r) { if (S != NULL) SaveRemoveQuestReward(S, r); }

// 0x80221B08
void RewardPlayerChallenge(u32 c) { if (S != NULL) SaveRewardChallenge(S, c); }

// 0x80221B24
void RewardPlayerExperiment(u32 e, BOOL a) { if (S != NULL) SaveRewardExperiment(S, e, a); }

// 0x80221B48
void RewardPlayerCombat(void) { if (S != NULL) SaveRewardCombat(S); }

// 0x80221B64
u32 IsRegionReached(u32 region) { return S == NULL ? 1 : SaveIsRegionReached(S, (u8)region); }

// 0x80221B8C
u32 IsRegionLocked(u32 region) {
    u32 reached = S == NULL ? 1 : SaveIsRegionReached(S, (u8)region);
    return reached == 0;
}

// 0x80221BD8
void SetRegionReached(u32 region, u32 reached) {
    if (S == NULL) return;
    SaveSetRegionReached(S, (u8)region, reached != 0);
}

// 0x80221C0C
void SetNextLocation(u32 loc) { if (S != NULL) *(u32*)(S + 0x104) = loc; }

// 0x80221C20
// The previous location is remembered; with no previous one, it is the new location.
void SetCurrentLocation(u32 loc) {
    if (S == NULL) return;
    s32 prev = *(s32*)(S + 0x108);
    *(s32*)(S + 0x10C) = prev;
    *(u32*)(S + 0x108) = loc;
    *(s32*)(S + 0x104) = -1;
    if (prev == -1) *(s32*)(S + 0x10C) = *(s32*)(S + 0x108);
}

// 0x80221C54
// Next location as an index among locations 7..12 (0 otherwise).
s32 GetNextLocation(void) {
    if (S == NULL) return 0;
    s32 loc = *(s32*)(S + 0x104);
    if (loc < 7 || loc > 12) return 0;
    return loc - 7;
}

// 0x80221C88
s32 GetNextRegion(void) { return S == NULL ? 0 : SaveGetNextRegion(S); }
