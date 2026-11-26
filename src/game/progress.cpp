// Player progress in the save: achievements, experience and rank, and the notification queue
// the HUD pops messages from.
// Original: fn_8022954C / fn_80229554 (auto_03_80221E1C_text.s area)
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <intrin.h>

#include "types.h"
#include "game/part.h"
#include "port/port.h"

#define MAX_RANK        49
#define MAX_MSP         99999999u
#define MAX_NOTICES     15

enum { NOTICE_RANK_UP = 1, NOTICE_ACHIEVEMENT = 4 };

enum {
    ACH_FIELD_REPAIR_1 = 0x46,
    ACH_FIELD_REPAIR_2 = 0x47,
    ACH_FIELD_REPAIR_3 = 0x48,
};

struct Notice {             // 0x1C
    s32 type;
    s32 value;              // achievement id / new rank
    s32 unk08, unk0C, unk10;
    s32 unk14;              // -1
    s32 seq;
};

// Achievement record in the game database (0x18 bytes); +0x10 is the XP reward.
struct AchievementDef { u32 unk00[4]; f32 xp; u32 unk14; };
// Rank thresholds (0x10 bytes).
struct RankDef { u32 unk00[2]; f32 xpRequired; u32 unk0C; };

struct SaveProgress {
    u8     pad000[0x30];
    u64    achievements;            // 0x30: one bit per achievement id - 0x40
    u8     pad038[0xE8];
    s32    achievementCount;        // 0x120
    u8     pad124[4];
    u32    rank;                    // 0x128
    u32    msp;                     // 0x12C: Mad Scientist Points (the game's experience)
    u8     pad130[0x614];
    Notice notices[MAX_NOTICES];    // 0x744
    s32    noticeCount;             // 0x8E8
    u8     pad8EC[0x24];
    s32    noticeSeq;               // 0x910
    u8     pad914[0xC8];
    u32    fieldRepairRecord;       // 0x9DC
};

extern "C" void NoticePosted(u8* p, s32 idx);    // 0x8022DF3C (idx: the entry just queued)
extern SaveProgress* gSaveSystem;                // 0x805FBBE0

static inline u8* Db(void) { return gGameDb->data; }

// ---------------------------------------------------------------- badge award log (port only)
// Every badge award logs "badge award: id 0xNN "name" frame F why=... caller=+0xRVA". `why` is
// the counter or condition the award site set with BadgeWhy() just before (empty for script
// awards); `caller` is the return address relative to the exe base (look it up in monsterlab.map).

struct BadgeStringTable { const char** strings; u8 pad[8]; u32 count; };
extern "C++" BadgeStringTable* gStringTable;      // 0x805FB910
extern "C++" BadgeStringTable* gStringTable2;     // 0x805FB90C
extern "C" char __ImageBase;

static char sBadgeWhy[128];

extern "C" void BadgeWhy(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sBadgeWhy, sizeof(sBadgeWhy), fmt, ap);
    va_end(ap);
}

static const char* BadgeName(s32 id) {
    const u8* db = Db();
    if (id < 0 || id >= *(const s32*)(db + 0x10AC)) return "(out of range)";
    s32 text = TextIdFromHash(*(const u32*)(*(u8**)(db + 0x10A8) + id * 0x18 + 4));
    if (text == -1) return "?";
    const BadgeStringTable* t = gStringTable;
    if ((text & 0x80000000) && gStringTable2 != NULL) { t = gStringTable2; text &= 0x7FFFFFFF; }
    if (t == NULL || t->strings == NULL || (u32)text >= t->count) return "?";
    return t->strings[text];
}

static void LogBadgeAward(s32 id, void* caller);

// For the award paths that set the badge bit themselves instead of calling SaveAddBadge.
extern "C" void BadgeLogAward(s32 id) { LogBadgeAward(id, _ReturnAddress()); }

static void LogBadgeAward(s32 id, void* caller) {
    PortLog("badge award: id 0x%02X \"%s\" frame %d why=%s caller=+0x%X", id, BadgeName(id),
            PortFrameCount(), sBadgeWhy[0] ? sBadgeWhy : "-", (u32)((char*)caller - &__ImageBase));
    sBadgeWhy[0] = 0;
}

static void PushNotice(SaveProgress* s, s32 type, s32 value) {
    if (s->noticeCount >= MAX_NOTICES) return;
    Notice* n = &s->notices[s->noticeCount];
    n->type = type;
    n->value = value;
    n->unk08 = n->unk0C = n->unk10 = 0;
    n->unk14 = -1;
    n->seq = ++s->noticeSeq;
    s->noticeCount++;
    NoticePosted((u8*)s, s->noticeCount - 1);
}

// Add MSP (experience) to a save and post a notice for every rank gained.
static void AddMSPTo(SaveProgress* s, u32 amount) {
    s->msp += amount;
    if (s->msp > MAX_MSP) s->msp = MAX_MSP;
    if ((s32)s->rank >= MAX_RANK) return;

    const RankDef* ranks = *(RankDef**)(Db() + 0x63C8);
    BOOL gained = FALSE;
    for (u32 r = s->rank + 1; r <= MAX_RANK && ranks[r].xpRequired < (f32)s->msp; r++) {
        s->rank = r;
        gained = TRUE;
    }
    if (gained) PushNotice(s, NOTICE_RANK_UP, s->rank);
}

static void AddMSP(u32 amount) { AddMSPTo(gSaveSystem, amount); }

static void Unlock(SaveProgress* s, s32 id) {
    u64 bit = (u64)1 << (id - 0x40);
    if (s->achievements & bit) return;
    s->achievements |= bit;
    s->achievementCount++;
    LogBadgeAward(id, _ReturnAddress());
    PushNotice(s, NOTICE_ACHIEVEMENT, id);
    const AchievementDef* defs = *(AchievementDef**)(Db() + 0x10A8);
    AddMSP((u32)defs[id].xp);
}

// 0x8022954C
u32 SaveGetFieldRepairRecord(SaveProgress* s) {
    return s->fieldRepairRecord;
}

// 0x80229554
// New best for repair points left after a battle; unlocks the three field-repair achievements.
void SaveSetFieldRepairRecord(SaveProgress* s, u32 points) {
    s->fieldRepairRecord = points;
    BadgeWhy("field repair record %u", points);
    const f32* t = (const f32*)(*(u8**)(Db() + 0x99D0) + 0x328);
    if (points >= (u32)t[5]) {
        Unlock(s, ACH_FIELD_REPAIR_1);
        Unlock(s, ACH_FIELD_REPAIR_2);
        Unlock(s, ACH_FIELD_REPAIR_3);
    } else if (points >= (u32)t[3]) {
        Unlock(s, ACH_FIELD_REPAIR_1);
        Unlock(s, ACH_FIELD_REPAIR_2);
    } else if (points >= (u32)t[1]) {
        Unlock(s, ACH_FIELD_REPAIR_1);
    }
}

// ---------------------------------------------------------------- save bit sets

// Bit sets in the save are arrays of u64 stored highest bits first: bit n lives in the word
// at `base - (n / 64) * 8`.
static inline u64* BitWord(u8* s, u32 base, u32 n) { return (u64*)(s + base - (n / 64) * 8); }
static inline BOOL BitGet(u8* s, u32 base, u32 n) { return (BOOL)((*BitWord(s, base, n) >> (n % 64)) & 1); }
static inline void BitSet(u8* s, u32 base, u32 n)   { *BitWord(s, base, n) |= (u64)1 << (n % 64); }
static inline void BitClear(u8* s, u32 base, u32 n) { *BitWord(s, base, n) &= ~((u64)1 << (n % 64)); }

#define BADGES_BASE     0x38   // 256 badges: +0x38, +0x30, +0x28, +0x20
#define REGIONS_BASE    0x18   // 128 regions: +0x18, +0x10
#define PROGRESS_BASE   0xF8   // 128 progress flags: +0xF8, +0xF0
#define FLAGS2_BASE     0xE8   // 128 more flags: +0xE8, +0xE0

extern "C" {

// 0x80223134
u32 SaveIsRegionReached(u8* s, u8 region) { return BitGet(s, REGIONS_BASE, region); }

// 0x802231A0
// Regions can only be marked once; an already reached region is left alone.
void SaveSetRegionReached(u8* s, u8 region, BOOL reached) {
    if (BitGet(s, REGIONS_BASE, region)) return;
    if (reached) BitSet(s, REGIONS_BASE, region);
    else         BitClear(s, REGIONS_BASE, region);
}

// 0x80223308
u32 SaveHasBadge(u8* s, u32 badge) { return BitGet(s, BADGES_BASE, (u8)badge); }

// 0x802233D4
// Award a badge once: count it, post a notice, and grant its XP (which can raise the rank).
void SaveAddBadge(SaveProgress* s, s32 badge) {
    if (badge == -1) return;
    u8* raw = (u8*)s;
    if (BitGet(raw, BADGES_BASE, (u8)badge)) return;
    s->achievementCount++;
    BitSet(raw, BADGES_BASE, (u8)badge);
    LogBadgeAward(badge, _ReturnAddress());
    PushNotice(s, NOTICE_ACHIEVEMENT, badge);
    const AchievementDef* defs = *(AchievementDef**)(Db() + 0x10A8);
    AddMSP((u32)defs[badge].xp);
}

// 0x802237AC
void SaveRemoveBadge(SaveProgress* s, u32 badge) {
    u8* raw = (u8*)s;
    if (!BitGet(raw, BADGES_BASE, (u8)badge)) return;
    s->achievementCount--;
    BitClear(raw, BADGES_BASE, (u8)badge);
}

// 0x8022397C
// Whether a part type (156 of them, one byte at +0x40 each) has been seen at a tier.
BOOL SavePartTierSeen(u8* s, u32 part, s32 tier) {
    if (part >= 0x9C || tier == 0 || (u32)tier > 4) return FALSE;
    u8 bit = (u8)(1 << (tier - 1));
    return bit == (bit & s[0x40 + part]);
}

// 0x80225C90
u32 SaveCheckProgressFlag(u8* s, u32 flag) { return BitGet(s, PROGRESS_BASE, (u8)flag); }

// 0x80225CFC
void SaveSetProgressFlag(u8* s, u32 flag, BOOL on) {
    if (on) BitSet(s, PROGRESS_BASE, (u8)flag);
    else    BitClear(s, PROGRESS_BASE, (u8)flag);
}

// 0x80225DF8
u32 SaveCheckFlag2(u8* s, u8 flag) { return BitGet(s, FLAGS2_BASE, flag); }

// 0x80225E64
void SaveSetFlag2(u8* s, u8 flag, BOOL on) {
    if (on) BitSet(s, FLAGS2_BASE, flag);
    else    BitClear(s, FLAGS2_BASE, flag);
}

// 0x80223004
// The pending quest rewards list (32 ids at +0x6BC, count u8 at +0x124).
void SaveCopyQuestRewards(const u8* s, u8* out) { memcpy(out, s + 0x6BC, 0x80); }

// 0x8022309C
void SaveSetQuestRewards(u8* s, const u8* in) { memcpy(s + 0x6BC, in, 0x80); }

}  // extern "C"

// ---------------------------------------------------------------- rewards

extern "C" {
PartInstance* PartInventoryAlloc(u8* inv);                        // 0x80219AE8
void PartSetDef(PartInstance* p, s32 def);                        // 0x8021D43C
void PartSetTier(PartInstance* p, u8 tier);                       // 0x8021DA68
void PartSetQuality(PartInstance* p, s32 q);                      // 0x8021DA40
void PartSetHealth(PartInstance* p, s32 hp);                      // 0x8021D8EC
s32  PartGetMaxHealth(PartInstance* p);                           // 0x8021F168
BOOL LabAddMaterial(u8* inv, s32 index, s32 kind, s32 amount, BOOL notify);   // 0x80219C44
void SaveDiscoverPart(u8* s, s32 type, u8 tier);                  // 0x802239D0
void LabOnPartReward(void* lab, PartInstance* p);                 // 0x80218AF4
void LabOnMaterialReward(void* lab, s32 index, s32 kind, s32 n);  // 0x80218AFC
void LabOnDiscoveryReward(void* lab, s32 type, s32 tier);         // 0x80218BA4
void LabOnMSPReward(void* lab, u32 msp);                          // 0x80218B9C
}
extern void* gLab;     // 0x805FBBC8

enum { NOTICE_MSP = 2, NOTICE_QUEST_REWARD = 3, NOTICE_KEY_INGREDIENT = 5, NOTICE_SECONDARY = 6,
       NOTICE_DISCOVERY = 8, NOTICE_PART = 9 };

static void PostNotice6(SaveProgress* s, s32 type, s32 a, s32 b) {
    if (s->noticeCount >= MAX_NOTICES) return;
    Notice* n = &s->notices[s->noticeCount];
    n->type = type;
    n->value = a;
    n->unk08 = b;
    n->unk0C = n->unk10 = 0;
    n->unk14 = -1;
    n->seq = ++s->noticeSeq;
    s->noticeCount++;
    NoticePosted((u8*)s, s->noticeCount - 1);
}

// Reward record (0x3C bytes): +0x18 MSP, +0x1C key ingredient, +0x24 secondary ingredient,
// +0x28 part, +0x2C part type to discover, +0x30 part tier, +0x32 discovery tier,
// +0x34 part quality, +0x36 secondary count, +0x37 key ingredient count.
static inline s32 AtLeastOne(s32 v) { return v > 1 ? v : 1; }

extern "C" {

// 0x80224308
// Hand out one reward record: a part, ingredients, a part discovery and MSP, each with its
// notice when `notify` is set. Returns the MSP granted.
u32 SaveUnlockReward(SaveProgress* s, const u8* r, BOOL notify) {
    u8* raw = (u8*)s;
    s32 part = *(const s32*)(r + 0x28);
    if (part != -1) {
        PartInstance* p = PartInventoryAlloc(raw + 0xA30);
        if (p != NULL) PartSetDef(p, part);
        if (p != NULL) {
            PartSetTier(p, (u8) * (const s16*)(r + 0x30));
            PartSetQuality(p, (s8) * (const s16*)(r + 0x34));
            PartSetHealth(p, PartGetMaxHealth(p));
            LabOnPartReward(gLab, p);
            if (notify) PostNotice6(s, NOTICE_PART, *(const s32*)(r + 0x28), *(const s16*)(r + 0x30));
        }
    }
    s32 key = *(const s32*)(r + 0x1C);
    if (key != -1) {
        s32 n = AtLeastOne((s8)r[0x37]);
        LabAddMaterial(raw + 0xA30, key, 0, n, TRUE);
        LabOnMaterialReward(gLab, *(const s32*)(r + 0x1C), 0, n);
        if (notify) PostNotice6(s, NOTICE_KEY_INGREDIENT, *(const s32*)(r + 0x1C), 0);
    }
    s32 sec = *(const s32*)(r + 0x24);
    if (sec != -1) {
        s32 n = AtLeastOne((s8)r[0x36]);
        LabAddMaterial(raw + 0xA30, sec, 1, n, TRUE);
        LabOnMaterialReward(gLab, *(const s32*)(r + 0x24), 1, n);
        if (notify) PostNotice6(s, NOTICE_SECONDARY, *(const s32*)(r + 0x24), 0);
    }
    if (*(const s32*)(r + 0x2C) != -1) {
        s32 tier = AtLeastOne(*(const s16*)(r + 0x32));
        SaveDiscoverPart(raw, *(const s32*)(r + 0x2C), (u8)tier);
        u32 type = *(const u32*)(r + 0x2C);
        if (type < 0x9C && (u8)tier != 0 && (u8)tier <= 4) {
            u8 seen = (u8)(1 << ((u8)tier - 1));
            if (seen == (seen & raw[0x40 + type])) raw[0x40 + type] |= (u8)(1 << ((u8)tier + 3));
        }
        LabOnDiscoveryReward(gLab, *(const s32*)(r + 0x2C), tier);
        if (notify) PostNotice6(s, NOTICE_DISCOVERY, *(const s32*)(r + 0x2C), tier);
    }
    // MSP goes to the save that owns the record, and the rank-up is checked on it.
    u32 msp = (u32)*(const f32*)(r + 0x18);
    s->msp += msp;
    if (s->msp > MAX_MSP) s->msp = MAX_MSP;
    if ((s32)s->rank < MAX_RANK) {
        const RankDef* ranks = *(RankDef**)(Db() + 0x63C8);
        BOOL gained = FALSE;
        for (u32 k = s->rank + 1; k <= MAX_RANK && ranks[k].xpRequired < (f32)s->msp; k++) {
            s->rank = k;
            gained = TRUE;
        }
        if (gained) PushNotice(s, NOTICE_RANK_UP, s->rank);
    }
    LabOnMSPReward(gLab, (u32)*(const f32*)(r + 0x18));
    return (u32)*(const f32*)(r + 0x18);
}

// 0x80225F70
void SaveAddMSP(SaveProgress* s, s32 amount) {
    s->msp += amount;
    if (s->msp > MAX_MSP) s->msp = MAX_MSP;
    if ((s32)s->rank >= MAX_RANK) return;
    const RankDef* ranks = *(RankDef**)(Db() + 0x63C8);
    BOOL gained = FALSE;
    for (u32 k = s->rank + 1; k <= MAX_RANK && ranks[k].xpRequired < (f32)s->msp; k++) {
        s->rank = k;
        gained = TRUE;
    }
    if (gained) PushNotice(s, NOTICE_RANK_UP, s->rank);
}

// 0x8022624C
// Back at the castle: clear +0xA1C and heal every part of every lab monster.
void SaveReturnToCastle(u8* s) {
    *(s32*)(s + 0xA1C) = 0;
    for (s32 i = 0; i < *(s32*)(s + 0x10E8); i++) {
        u8* m = *(u8**)(s + 0xA30 + i * 4);
        if (m == NULL) continue;
        for (s32 k = 0; k < 5; k++) {
            PartInstance* p = ((PartInstance**)(m + 4))[k];
            if (p != NULL) PartSetHealth(p, PartGetMaxHealth(p));
        }
    }
}

// 0x802241D0
s32 TuningGetPair(GameDb* db, s32 i) { return *(s32*)(*(u8**)(db->data + 0x99D0) + i * 8); }

// 0x802241F0
// Part type seen and rewarded at this tier.
BOOL SavePartTierOwned(u8* s, u32 part, s32 tier) {
    if (part >= 0x9C || tier == 0 || (u32)tier > 4) return FALSE;
    u8 v = s[0x40 + part];
    u8 seen = (u8)(1 << (tier - 1)), owned = (u8)(1 << (u8)(tier + 3));
    return seen == (seen & v) && owned == (owned & v);
}

// 0x80224258
void SaveAddQuestReward(u8* s, u32 id) {
    if ((s32)id == -1) return;
    u8 n = s[0x124];
    if (n < 0x20) {
        ((u32*)(s + 0x6BC))[n] = id;
        s[0x124] = (u8)(n + 1);
    }
}

// 0x80224288
// Remove by swapping in the last entry.
void SaveRemoveQuestReward(u8* s, u32 id) {
    if ((s32)id == -1) return;
    u8 n = s[0x124];
    u32* list = (u32*)(s + 0x6BC);
    for (s32 i = 0; i < n; i++) {
        if (list[i] != id) continue;
        list[i] = list[n - 1];
        list[s[0x124] - 1] = (u32)-1;
        s[0x124]--;
        return;
    }
}

}  // extern "C"

extern "C" {
void LabQuestFinished(void* lab, s32 id);                         // 0x80218998
s32  LabChallengeRewardTable(void* lab, u32 challenge);           // 0x80218E6C
s32 SaveRollChallengeReward(u8* s, s32 table, s32 a);            // 0x802262E0
u8*  LabAllocMonster(u8* inv);                                    // 0x80219FC4
void MonsterBuildPreset(u8* m, s32 preset);                       // 0x8021A164
}

static inline const u8* RewardRec(s32 i) { return *(u8**)(Db() + 0x8D58) + i * 0x3C; }

static void GainMSP(SaveProgress* s, u32 amount) {
    s->msp += amount;
    if (s->msp > MAX_MSP) s->msp = MAX_MSP;
    if ((s32)s->rank >= MAX_RANK) return;
    const RankDef* ranks = *(RankDef**)(Db() + 0x63C8);
    BOOL gained = FALSE;
    for (u32 k = s->rank + 1; k <= MAX_RANK && ranks[k].xpRequired < (f32)s->msp; k++) {
        s->rank = k;
        gained = TRUE;
    }
    if (gained) PushNotice(s, NOTICE_RANK_UP, s->rank);
}

extern "C" {

// 0x80224818
// Grant every reward (of the 53) that belongs to this quest, each with a quest-reward notice.
void SaveRewardQuest(SaveProgress* s, u32 quest) {
    LabQuestFinished(gLab, (s32)quest);
    for (s32 i = 0; i < 0x35; i++) {
        u8 rec[0x3C];
        memcpy(rec, RewardRec(i), sizeof(rec));
        if (*(s32*)(rec + 0xC) != (s32)quest) continue;
        SaveUnlockReward(s, rec, TRUE);
        PostNotice6(s, NOTICE_QUEST_REWARD, (s32)quest, (s32)*(const f32*)(rec + 0x18));
    }
}

// 0x8022497C
void SaveGiveQuestReward(SaveProgress* s, u32 reward) {
    u8 rec[0x3C];
    memcpy(rec, RewardRec((s32)reward), sizeof(rec));
    SaveUnlockReward(s, rec, TRUE);
}

// 0x802252D4
// A challenge was won: pay out the matching pending reward (for this monster or any), or
// else the minigame score as MSP and a random reward from the challenge's table.
void SaveRewardChallenge(SaveProgress* s, u32 challenge) {
    u8* raw = (u8*)s;
    LabQuestFinished(gLab, (s32)challenge);
    BOOL paid = FALSE;
    for (s32 i = 0; i < raw[0x124]; i++) {
        u8 rec[0x3C];
        memcpy(rec, RewardRec(((s32*)(raw + 0x6BC))[i]), sizeof(rec));
        u32 who = *(u32*)(rec + 0x14);
        if (*(s32*)(rec + 4) != (s32)challenge || (who != *(u32*)(raw + 0x130) && who != 0)) continue;
        SaveUnlockReward(s, rec, FALSE);
        u8 n = raw[0x124];
        if ((u32)i < n) {
            ((s32*)(raw + 0x6BC))[i] = ((s32*)(raw + 0x6BC))[n - 1];
            ((s32*)(raw + 0x6BC))[raw[0x124] - 1] = -1;
            raw[0x124]--;
        }
        paid = TRUE;
        break;
    }
    if (paid) return;
    GainMSP(s, *(u32*)((u8*)gLab + 0x78));
    SaveRollChallengeReward(raw, LabChallengeRewardTable(gLab, challenge), 0);
}

// 0x802260FC
// Jump to the rank and MSP a quest sets, and give its preset monster (full quality and health),
// which becomes the current monster.
void SaveSetRankFromQuest(SaveProgress* s, u32 quest) {
    u8* raw = (u8*)s;
    const u8* q = *(u8**)(Db() + 0x8508) + quest * 0x1C;
    s32 rank = *(const s32*)(q + 0x10);
    s->rank = (u32)rank;
    const RankDef* ranks = *(RankDef**)(Db() + 0x63C8);
    s->msp = (u32)ranks[rank].xpRequired;
    s32 preset = *(const s32*)(q + 0x14);
    if (preset == -1) return;
    u8* m = LabAllocMonster(raw + 0xA30);
    if (m != NULL) {
        MonsterBuildPreset(m, preset);
        m[0x4A] = 1;
        for (s32 i = 0; i < 5; i++) {
            PartInstance* p = ((PartInstance**)(m + 4))[i];
            if (p == NULL) continue;
            PartSetQuality(p, 100);
            PartSetHealth(p, PartGetMaxHealth(p));
        }
    }
    if (m != NULL) *(u8**)(raw + 0x114) = m;
}

// 0x80225C1C
// Best (lowest non-zero) time for one of 10 records.
void SaveRecordBestTime(u8* s, s32 i, u32 t) {
    if (i < 0 || i >= 10) return;
    u32* slot = (u32*)(s + 0x9A8) + i;
    if (t != 0 && (t < *slot || *slot == 0)) *slot = t;
}

// 0x80225C5C
// Best (highest) score for one of 6 records.
void SaveRecordBestScore(u8* s, s32 i, u32 score) {
    if (score == 0) return;
    u32* slot = (u32*)(s + 0xA04) + i;
    if (score > *slot && i != -1 && i < 6) *slot = score;
}

}  // extern "C"

// ---------------------------------------------------------------- loot rolls

extern "C" {
s32 RandomUpTo(s32 max);   // 0x80220C8C-based helper, see opponent_gen.cpp

// 0x8022515C
// Pick a random row of the 4-row table at db+0x31E8 that suits the part (`part` is a part
// instance whose first word is its definition). Torso parts take rows whose byte +0x1A is set,
// every other slot takes rows whose byte +0x1B is set. Returns the row index, or -1 if none fit.
s32 fn_8022515C(void* unused, u8* part) {
    (void)unused;
    PartDef def;
    PartDbGetDef(&def, gGameDb, *(s32*)part);
    const u8* row = *(u8**)(Db() + 0x31E8);
    s32 fit[4];
    s32 n = 0;
    for (s32 k = 0; k < 4; k++, row += 0x20) {
        bool ok = (def.slot == 4) ? row[0x1A] != 0 : row[0x1B] != 0;
        if (ok) fit[n++] = k;
    }
    return n > 0 ? fit[RandomUpTo(n - 1)] : -1;
}

}  // extern "C"


extern "C" {

// 0x8022667C
// Roll a kind-0 material reward. `parts` (5 part pointers, may be NULL) is scanned from a random
// start for the first part whose id is set; the material whose table row (0x24 bytes at
// db+0x6C18) carries that id is given. With no match, a random material (of 45) whose
// threshold (s16 at +8 of 0x10-byte rows at db+0x4F00) is within the save's value at +0x118 is
// drawn instead. Amount is uniform in the spec's s16 range at +0xE..+0x10.
void fn_8022667C(u8* s, const u8* spec, void** parts) {
    s32 id = -1, mat = -1;
    if (parts != NULL) {
        s32 start = RandomUpTo(4);
        for (s32 i = 0; i < 5; i++) {
            void* p = parts[(start + i) % 5];
            if (p == NULL || *(void**)p == NULL) continue;
            id = **(s32**)p;
            if (id != -1) break;
        }
    }
    if (id != -1) {
        // Two rows per iteration, 78 iterations (the original reads that far if nothing matches).
        const u8* tab = *(u8**)(Db() + 0x6C18);
        for (s32 k = 0; k < 156; k++) {
            const u8* row = tab + k * 0x24;
            if (*(const s32*)(row + 0x20) == id) { mat = *(const s32*)(row + 0x10); break; }
        }
    }
    while (mat == -1) {
        s32 r = RandomUpTo(0x2C);
        mat = r;
        const u8* row = *(u8**)(Db() + 0x4F00) + r * 0x10;
        if ((u32)(s32)*(const s16*)(row + 8) > *(u32*)(s + 0x118)) mat = -1;
    }
    s32 n = *(const s16*)(spec + 0xE) + RandomUpTo(*(const s16*)(spec + 0x10) - *(const s16*)(spec + 0xE));
    LabOnMaterialReward(gLab, mat, 0, (u8)n);
    LabAddMaterial(s + 0xA30, mat, 0, (u8)n, TRUE);
}

// 0x802268BC
// Roll a kind-1 material reward: first a category (weights at spec +0x16/+0x1A/+0x18 -> 1, 2, 0),
// then a level 1..4 (weights at +0x1E/+0x20/+0x22/+0x24; +0x1C is the fallback). Rows of the
// 0x18-byte table at db+0x5328 whose level (low byte of +0x10) equals the level and whose
// threshold (s16 at +0x10) is below gSaveSystem+0x118 are candidates; if none, the level drops
// by one and it retries. A candidate of the rolled category (next category if none) is picked.
void fn_802268BC(u8* s, const u8* spec) {
    s32 a = *(const s16*)(spec + 0x16), b = *(const s16*)(spec + 0x1A), c = *(const s16*)(spec + 0x18);
    s32 r = RandomUpTo(a + b + c - 1);
    s32 cat = (r < a) ? 1 : (r < a + b ? 2 : 0);

    s32 lvl = *(const s16*)(spec + 0x1C);
    s32 d = *(const s16*)(spec + 0x1E), e = *(const s16*)(spec + 0x20);
    s32 f = *(const s16*)(spec + 0x22), g = *(const s16*)(spec + 0x24);
    if (g + f + d + e != 0) {
        r = RandomUpTo(g + f + d + e - 1);
        if (r < d) lvl = 1;
        else if (r < d + e) lvl = 2;
        else lvl = (r < d + e + f) ? 3 : 4;
    }

    const u8* tab = *(u8**)(Db() + 0x5328);
    u32 limit = *(u32*)((u8*)gSaveSystem + 0x118);
    s32 cand[64];
    s32 cnt[3];
    s32 n;
    for (;;) {
        cnt[0] = cnt[1] = cnt[2] = 0;
        n = 0;
        for (s32 k = 0; k < 60; k++) {
            const u8* row = tab + k * 0x18;
            u32 w10 = *(const u32*)(row + 0x10);
            if ((s32)(s8)(w10 & 0xFF) == lvl && (u32)(s32)(s16)(w10 >> 16) < limit) {
                cand[n++] = *(const s32*)row;
                cnt[*(const s32*)(row + 0xC)]++;
            }
        }
        if (lvl <= 1) break;
        lvl--;
        if (n != 0) break;
    }
    if (n <= 0) return;
    while (cnt[cat] <= 0) cat = (cat + 1) % 3;
    s32 k = RandomUpTo(cnt[cat] - 1);
    s32 mat = -1;
    for (s32 j = 0; j < n; j++) {
        if (*(const s32*)(tab + cand[j] * 0x18 + 0xC) == cat) {
            if (k == 0) { mat = cand[j]; break; }
            k--;
        }
    }
    s32 q = *(const s16*)(spec + 0x12) + RandomUpTo(*(const s16*)(spec + 0x14) - *(const s16*)(spec + 0x12));
    LabOnMaterialReward(gLab, mat, 1, (u8)q);
    LabAddMaterial(s + 0xA30, mat, 1, (u8)q, TRUE);
}

}  // extern "C"

// ---------------------------------------------------------------- experiment / discovery

extern "C" {
void fn_802189E0(void* lab);              // 0x802189E0
s32  PartPower(s32 defIndex, s32 tier);   // 0x8021B650 (opponent_gen.cpp)
}

// Badges for discovering many part types: (row of the threshold table at db+0x99D0, badge id).
static const u32 kDiscoverBadgeRow[8] = { 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0xA3, 0x5F };
static const u32 kDiscoverBadge[8]    = { 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E };
enum { NOTICE_TYPE_7 = 7 };

extern "C" {

// 0x802239D0
// Record that a part type has been seen at a tier. The first time gives MSP (part power times a
// tuning factor) and the matching rank-ups, tells the lab, and then (while fewer than 0x270 part
// tiers are known) awards any "discovered N parts" badge whose threshold has been reached.
// (The original also fetched the part definition here but never used it.)
void SaveDiscoverPart(u8* s, s32 type, u8 tier) {
    SaveProgress* sp = (SaveProgress*)s;
    if ((u32)type >= 0x9C || tier == 0 || tier > 4) return;
    u8 bit = (u8)(1 << (tier - 1));
    if (bit != (bit & s[0x40 + type])) {
        *(s32*)(s + 0x11C) += 1;
        s[0x40 + type] |= bit;
        s32 part = *(const s32*)(*(u8**)(Db() + 0x6C18) + type * 0x24 + 0x20);
        s32 power = PartPower(part, tier);
        f32 factor = *(const f32*)(*(u8**)(Db() + 0x99D0) + 0x674);
        AddMSPTo(sp, (u32)(power * (u32)factor));
        LabOnDiscoveryReward(gLab, type, tier);
    }
    s32 count = *(s32*)(s + 0x11C);
    if (count >= 0x270) return;
    for (s32 i = 7; i >= 0; i--) {
        u32 badge = kDiscoverBadge[i];
        if (SaveHasBadge(s, badge)) continue;
        const u8* row = *(u8**)(Db() + 0x99D0) + kDiscoverBadgeRow[i] * 8;
        if (count < (s32)*(const f32*)(row + 4)) continue;
        BadgeWhy("discovered part tiers %d >= %d", count, (s32)*(const f32*)(row + 4));
        SaveAddBadge(sp, (s32)badge);
    }
}

// 0x80224A34
// Finish an experiment: `quest` is the quest/reward id it completes (-1 for none), `quality`
// (capped at 100) is the quality of the part produced. If the quest has a pending unlock reward
// it is handed out (and removed from the pending list) instead of the experiment's own part.
// Otherwise (when the lab has an experiment result selected) the result row gives MSP (rank-ups
// are checked), a part discovery with notice 8 (new tier of a known part) or 7 (new part), and a
// new part for the inventory, which gets an enhancement or a random defect depending on the kind.
void SaveRewardExperiment(SaveProgress* s, s32 quest, s32 quality) {
    u8* raw = (u8*)s;
    if ((u32)quality > 100) quality = 100;
    LabQuestFinished(gLab, quest);
    BOOL unlocked = FALSE;
    if (quest != -1) {
        for (s32 i = 0; i < (s32)raw[0x124]; i++) {
            const u8* rec = *(u8**)(Db() + 0x8D58) + *(s32*)(raw + 0x6BC + i * 4) * 0x3C;
            u8 copy[0x3C];
            memcpy(copy, rec, 0x3C);
            if (*(const s32*)(copy + 8) != quest) continue;
            SaveUnlockReward(s, copy, FALSE);
            if ((u32)i < raw[0x124]) {
                // Remove from the pending list by moving the last entry into the hole.
                *(s32*)(raw + 0x6BC + i * 4) = *(s32*)(raw + 0x6BC + (raw[0x124] - 1) * 4);
                *(s32*)(raw + 0x6BC + (raw[0x124] - 1) * 4) = -1;
                raw[0x124]--;
            }
            unlocked = TRUE;
            break;
        }
    }

    u8* lab = (u8*)gLab;
    if (*(s32*)(lab + 0x58) == -1) return;
    AddMSPTo(s, *(u32*)(lab + 0x78));

    const u8* exp = *(u8**)(Db() + 0x5328) + *(s32*)(lab + 0x54) * 0x18;
    const u8* row = *(u8**)(Db() + 0x6C18) + *(s32*)(lab + 0x58) * 0x24;
    s32 kind = *(const s32*)(exp + 8);
    u8  tier = *(const u8*)(exp + 0x13);       // low byte of the word at +0x10
    s32 partType = *(const s32*)row;
    BOOL isKind = *(const s32*)(row + 0x14) == kind;

    BOOL full = SavePartTierSeen(raw, partType, tier) && (raw[0x40 + partType] & (1 << (tier + 3)));
    BOOL seen = SavePartTierSeen(raw, partType, tier);
    LabOnDiscoveryReward(gLab, -1, 0);
    if (isKind && !full) {
        SaveDiscoverPart(raw, partType, tier);
        if (SavePartTierSeen(raw, partType, tier)) raw[0x40 + partType] |= (u8)(1 << (tier + 3));
        PostNotice6(s, NOTICE_DISCOVERY, partType, (s8)tier);
    } else if (!full && !seen) {
        SaveDiscoverPart(raw, partType, tier);
        PostNotice6(s, NOTICE_TYPE_7, partType, (s8)tier);
    }

    if (unlocked) return;
    s32 def = *(const s32*)(row + 0x20);
    PartInstance* p = PartInventoryAlloc(raw + 0xA30);
    if (p != NULL) PartSetDef(p, def);
    if (p != NULL) {
        PartSetTier(p, tier);
        PartSetQuality(p, (s8)quality);
        if (*(const s32*)(row + 0x14) == kind) {
            PartDef d;
            PartDbGetDef(&d, gGameDb, def);
            p->enhancement = d.unk20;
        } else if (*(const s32*)(row + 0x18) == kind || *(const s32*)(row + 0x1C) == kind) {
            p->defect = fn_8022515C(raw, (u8*)p);
        }
        PartSetHealth(p, PartGetMaxHealth(p));
        LabOnPartReward(gLab, p);
    }
    fn_802189E0(gLab);
}

}  // extern "C"

// ---------------------------------------------------------------- statistic counters and badges
//
// Several save statistics are counters (or per-index counters) that unlock badges at thresholds
// from the tuning table at db+0x99D0 (8-byte rows, float at +4).

static inline u32 TuningThreshold(u32 row) {
    return (u32)*(const f32*)(*(u8**)(Db() + 0x99D0) + row * 8 + 4);
}

// Add `amount` to the counter at `ofs`, then award the badge `badges[i]` for every i (last
// first) whose threshold (tuning row `rows[i]`) the counter has reached.
static void CountAndAward(u8* s, u32 ofs, s32 amount, const u32* rows, const u32* badges, s32 n) {
    s32 total = *(s32*)(s + ofs) + amount;
    *(s32*)(s + ofs) = total;
    for (s32 i = n - 1; i >= 0; i--) {
        if (SaveHasBadge(s, badges[i])) continue;
        if (total < (s32)*(const f32*)(*(u8**)(Db() + 0x99D0) + rows[i] * 8 + 4)) continue;
        BadgeWhy("counter +0x%X = %d (+%d) >= %d", ofs, total, amount,
                 (s32)*(const f32*)(*(u8**)(Db() + 0x99D0) + rows[i] * 8 + 4));
        SaveAddBadge((SaveProgress*)s, (s32)badges[i]);
    }
}

extern "C" {

// 0x8022CF24
void fn_8022CF24(u8* s, s32 amount) {
    static const u32 rows[4] = { 0x52, 0x53, 0x54, 0x55 }, badges[4] = { 0x30, 0x31, 0x32, 0x33 };
    CountAndAward(s, 0x908, amount, rows, badges, 4);
}

// 0x8022D94C
void fn_8022D94C(u8* s, s32 amount) {
    static const u32 rows[3] = { 0x56, 0x57, 0x58 }, badges[3] = { 0x34, 0x35, 0x36 };
    CountAndAward(s, 0x9D0, amount, rows, badges, 3);
}

// 0x802276F4
void fn_802276F4(u8* s, s32 amount) {
    static const u32 rows[3] = { 0x93, 0x94, 0x95 }, badges[3] = { 0x8A, 0x8B, 0x8C };
    CountAndAward(s, 0x90C, amount, rows, badges, 3);
}

// 0x802280D0
void fn_802280D0(u8* s, s32 amount) {
    static const u32 rows[3] = { 0x90, 0x91, 0x92 }, badges[3] = { 0x87, 0x88, 0x89 };
    CountAndAward(s, 0xA00, amount, rows, badges, 3);
}

// 0x80227C20
// Per-index counter at +0x958 (index `idx`, -1 = none); one badge per index.
void fn_80227C20(u8* s, s32 idx, s32 amount) {
    static const s32 badges[10] = { 0x6C, 0x6B, 0x6F, 0x6E, 0x70, 0x72, 0x71, 0x73, -1, 0x6D };
    if (idx == -1) return;
    u32* c = (u32*)(s + 0x958 + idx * 4);
    *c += amount;
    u32 thr = (u32)*(const f32*)(*(u8**)(Db() + 0x99D0) + 0x284);
    if (*c < thr) return;
    BadgeWhy("counter +0x%X = %u (+%d) >= %u", 0x958 + idx * 4, *c, amount, thr);
    SaveAddBadge((SaveProgress*)s, badges[idx]);
}

// 0x8022D460
// Per-index record at +0x980 (set, not added); one badge per index once it reaches the row's value.
void fn_8022D460(u8* s, s32 idx, s32 value) {
    static const u32 rows[10] = { 0x81, 0x80, 0x84, 0x83, 0x85, 0x87, 0x86, 0x88, 0x00, 0x82 };
    static const s32 badges[10] = { 0x75, 0x74, 0x78, 0x77, 0x79, 0x7B, 0x7A, 0x7C, -1, 0x76 };
    if (idx == -1) return;
    u32* c = (u32*)(s + 0x980 + idx * 4);
    *c = value;
    if (*c < TuningThreshold(rows[idx])) return;
    BadgeWhy("record +0x%X = %u >= %u", 0x980 + idx * 4, *c, TuningThreshold(rows[idx]));
    SaveAddBadge((SaveProgress*)s, badges[idx]);
}

// 0x8022CD54
// Per-index record at +0x928 (set); reaching the tuning thresholds 0x4D < 0x4E < 0x4F < 0xA2 awards
// 1, 2, 3 or 4 of the index's four badges (12 groups).
void fn_8022CD54(u8* s, s32 idx, u32 value) {
    static const s32 badges[48] = {
        0x00, 0x0C, 0x18, 0x24, 0x04, 0x10, 0x1C, 0x28, 0x05, 0x11, 0x1D, 0x29,
        0x06, 0x12, 0x1E, 0x2A, 0x07, 0x13, 0x1F, 0x2B, 0x08, 0x14, 0x20, 0x2C,
        0x09, 0x15, 0x21, 0x2D, 0x0A, 0x16, 0x22, 0x2E, 0x01, 0x0D, 0x19, 0x25,
        0x02, 0x0E, 0x1A, 0x26, 0x0B, 0x17, 0x23, 0x2F, 0x03, 0x0F, 0x1B, 0x27,
    };
    if (idx == -1) return;
    *(u32*)(s + 0x928 + idx * 4) = value;
    SaveProgress* sp = (SaveProgress*)s;
    const s32* b = badges + idx * 4;
#define WHY928() BadgeWhy("record +0x%X = %u (thresholds %u %u %u %u)", 0x928 + idx * 4, value,         TuningThreshold(0x4D), TuningThreshold(0x4E), TuningThreshold(0x4F), TuningThreshold(0xA2))
    if (value >= TuningThreshold(0xA2)) { for (s32 k = 0; k < 4; k++) { WHY928(); SaveAddBadge(sp, b[k]); } }
    if (value >= TuningThreshold(0x4F)) { for (s32 k = 0; k < 3; k++) { WHY928(); SaveAddBadge(sp, b[k]); } return; }
    if (value >= TuningThreshold(0x4E)) { for (s32 k = 0; k < 2; k++) { WHY928(); SaveAddBadge(sp, b[k]); } return; }
    if (value >= TuningThreshold(0x4D)) { WHY928(); SaveAddBadge(sp, b[0]); }
#undef WHY928
}

}  // extern "C"
