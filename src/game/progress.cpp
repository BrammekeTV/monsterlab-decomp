// Player progress in the save: achievements, experience and rank, and the notification queue
// the HUD pops messages from.
// Original: fn_8022954C / fn_80229554 (auto_03_80221E1C_text.s area)
#include <string.h>

#include "types.h"
#include "game/part.h"

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

extern "C" void NoticePosted(SaveProgress* s);   // 0x8022DF3C
extern SaveProgress* gSaveSystem;                // 0x805FBBE0

static inline u8* Db(void) { return gGameDb->data; }

static void PushNotice(SaveProgress* s, s32 type, s32 value) {
    if (s->noticeCount >= MAX_NOTICES) return;
    Notice* n = &s->notices[s->noticeCount];
    n->type = type;
    n->value = value;
    n->unk08 = n->unk0C = n->unk10 = 0;
    n->unk14 = -1;
    n->seq = ++s->noticeSeq;
    s->noticeCount++;
    NoticePosted(s);
}

// Add MSP (experience) to the player and post a notice for every rank gained.
static void AddMSP(u32 amount) {
    SaveProgress* s = gSaveSystem;
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

static void Unlock(SaveProgress* s, s32 id) {
    u64 bit = (u64)1 << (id - 0x40);
    if (s->achievements & bit) return;
    s->achievements |= bit;
    s->achievementCount++;
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
    NoticePosted(s);
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
            PartSetTier(p, r[0x30]);
            PartSetQuality(p, (s8)r[0x34]);
            PartSetHealth(p, PartGetMaxHealth(p));
            LabOnPartReward(gLab, p);
            if (notify) PostNotice6(s, NOTICE_PART, *(const s32*)(r + 0x28), (u8)r[0x30]);
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
        LabOnDiscoveryReward(gLab, *(const s32*)(r + 0x2C), (u8)tier);
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
