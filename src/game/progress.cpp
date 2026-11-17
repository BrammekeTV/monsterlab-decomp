// Player progress in the save: achievements, experience and rank, and the notification queue
// the HUD pops messages from.
// Original: fn_8022954C / fn_80229554 (auto_03_80221E1C_text.s area)
#include "types.h"
#include "game/part.h"

#define MAX_RANK        49
#define MAX_XP          99999999u
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
    u32    xp;                      // 0x12C
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

// Add experience to the player and post a notice for every rank gained.
static void AddXp(u32 amount) {
    SaveProgress* s = gSaveSystem;
    s->xp += amount;
    if (s->xp > MAX_XP) s->xp = MAX_XP;
    if ((s32)s->rank >= MAX_RANK) return;

    const RankDef* ranks = *(RankDef**)(Db() + 0x63C8);
    BOOL gained = FALSE;
    for (u32 r = s->rank + 1; r <= MAX_RANK && ranks[r].xpRequired < (f32)s->xp; r++) {
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
    AddXp((u32)defs[id].xp);
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
