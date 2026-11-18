// Quest state in the save: 47 quest records (0x18 bytes at save + 0x150) with 128 flags, a
// level and an integer each; sub-quests (21 and up) also own a 15-word block at save + 0x5B8.
// Original: 0x802227A4 - 0x80223000 (auto_03_8020F8E0_text.s)
#include <string.h>

#include "types.h"
#include "game/part.h"

static inline u8* gGameDb_data(void) { return gGameDb->data; }

#define QUESTS          47
#define FIRST_SUBQUEST  21

struct QuestRecord {
    u64 flagsHigh;      // 0x00: flags 64..127
    u64 flagsLow;       // 0x08: flags 0..63
    s32 level;          // 0x10: 0 = not started
    s32 value;          // 0x14: quest integer (also used as 2-bit fields)
};

static inline QuestRecord* Quest(u8* s, u32 q) { return (QuestRecord*)(s + 0x150) + q; }
static inline u64* FlagWord(QuestRecord* r, u8 flag) { return flag < 0x40 ? &r->flagsLow : &r->flagsHigh; }
static inline u64  FlagBit(u8 flag) { return (u64)1 << (flag < 0x40 ? flag : flag - 0x40); }

static inline void ClearSubQuest(u8* s, s32 q) {
    QuestRecord* r = Quest(s, (u32)q);
    r->level = 0;
    r->flagsLow = 0;
    r->flagsHigh = 0;
    r->value = 0;
    memset(s + 0x5B8 + (q - FIRST_SUBQUEST) * 0x3C, 0, 0x3C);
}

extern "C" {

// 0x802227A4
void SaveSetQuestFlag(u8* s, u32 q, u32 flag, s32 on) {
    if (q >= QUESTS) return;
    QuestRecord* r = Quest(s, q);
    if (on) *FlagWord(r, (u8)flag) |= FlagBit((u8)flag);
    else    *FlagWord(r, (u8)flag) &= ~FlagBit((u8)flag);
}

// 0x802228D4
void SaveSetQuestLevel(u8* s, u32 q, s32 level) {
    if (q < QUESTS) Quest(s, q)->level = level;
}

// 0x802228EC
s32 SaveGetQuestFlag(u8* s, u32 q, u32 flag) {
    if (q >= QUESTS) return 0;
    u8 f = (u8)flag;
    return (s32)((*FlagWord(Quest(s, q), f) >> (f < 0x40 ? f : f - 0x40)) & 1);
}

// 0x80222974
s32 SaveGetQuestLevel(u8* s, u32 q) { return q < QUESTS ? Quest(s, q)->level : 0; }

// 0x802229A8
// Make `q` the active quest (+0x6AC), remember `x` (+0x6A8), and start it.
void SaveSetActiveQuest(u8* s, s32 x, s32 q) {
    if (q < 0 || q >= QUESTS) return;
    *(s32*)(s + 0x6AC) = q;
    *(s32*)(s + 0x6A8) = x;
    Quest(s, (u32)q)->level = 1;
}

// 0x80222D5C
// The active quest, or one of quests 22..24 while in progress (level 1..254).
BOOL SaveIsQuestActive(u8* s, s32 q) {
    if (*(s32*)(s + 0x6AC) == q) return TRUE;
    if ((u32)(q - 0x16) > 2) return FALSE;
    s32 level = Quest(s, (u32)q)->level;
    return level > 0 && level < 0xFF;
}

// 0x80222DAC
void SaveStartSubQuest(u8* s, s32 q) {
    if (q < 0 || q >= QUESTS) return;
    ClearSubQuest(s, q);
    Quest(s, (u32)q)->level = 1;
}

// 0x80222E48
void SaveResetSubQuest(u8* s, s32 q) {
    if (q < 0 || q >= QUESTS) return;
    ClearSubQuest(s, q);
}

// 0x80222ECC
void SaveSetQuestInt(u8* s, s32 q, s32 v) {
    if (q >= 0 && q < QUESTS) Quest(s, (u32)q)->value = v;
}

// 0x80222EF0
s32 SaveGetQuestInt(u8* s, s32 q) { return (q < 0 || q >= QUESTS) ? 0 : Quest(s, (u32)q)->value; }

// 0x80222F18
void SaveSetQuestIntBits(u8* s, s32 q, s32 which, s32 v) {
    if (q < 0 || q >= QUESTS) return;
    s32 sh = which * 2;
    QuestRecord* r = Quest(s, (u32)q);
    r->value = (r->value & ~(3 << sh)) | (v << sh);
}

// 0x80222F58
s32 SaveGetQuestIntBits(u8* s, s32 q, s32 which) {
    if (q < 0 || q >= QUESTS) return 0;
    return (Quest(s, (u32)q)->value >> (which * 2)) & 3;
}

// 0x80222F8C
QuestRecord* SaveGetQuestRecord(u8* s, s32 q) { return (q < 0 || q >= QUESTS) ? NULL : Quest(s, (u32)q); }

// 0x80222FB4
void SaveSetQuestRecord(u8* s, s32 q, const QuestRecord* r) {
    if (q < 0 || q >= QUESTS) return;
    *Quest(s, (u32)q) = *r;
}

}  // extern "C"

extern "C" {
void NoticePosted(u8* s, s32 idx);                            // 0x8022DF3C
void LabQuestFinished(void* lab, s32 a);               // 0x80218998
s32 SaveUnlockReward(u8* s, const u8* reward, BOOL notify);   // 0x80224308
}
extern void* gLab;     // 0x805FBBC8

// Notice queue (save + 0x744, 15 entries of 0x1C; count +0x8E8, sequence +0x910).
static void PostNotice(u8* s, s32 type, s32 a, s32 b, s32 c, s32 d, s32 e) {
    s32 n = *(s32*)(s + 0x8E8);
    if (n >= 15) return;
    s32* rec = (s32*)(s + 0x744 + n * 0x1C);
    rec[0] = type; rec[1] = a; rec[2] = b; rec[3] = c; rec[4] = d; rec[5] = e;
    rec[6] = ++*(s32*)(s + 0x910);
    *(s32*)(s + 0x8E8) = n + 1;
    NoticePosted(s, n);
}

extern "C" {

// 0x802229D8
// End the active quest. Abandoning clears it; finishing it hands out every reward tied to it
// (53 reward records at gGameDb->data + 0x8D58), posts the notices and marks it done (level
// 255). Either way the shared sub-quest scratch block is cleared.
void SaveEndQuest(u8* s, BOOL abandon) {
    s32 q = *(s32*)(s + 0x6AC);
    if (q > -1 && q < QUESTS) {
        if (abandon) {
            QuestRecord* r = Quest(s, (u32)*(s32*)(s + 0x6AC));
            r->level = 0;
            r->flagsLow = 0;
            r->flagsHigh = 0;
            r->value = 0;
        } else {
            const u8* def = *(u8**)(gGameDb_data() + 0x8508) + q * 0x1C;
            s32 msp = *(const s32*)(def + 8);
            *(s32*)(s + 0x6B8) = msp;
            LabQuestFinished(gLab, *(const s32*)(def + 0x14));
            for (s32 i = 0; i < 0x35; i++) {
                const u8* reward = *(u8**)(gGameDb_data() + 0x8D58) + i * 0x3C;
                if (*(const s32*)(reward + 0xC) != q) continue;
                SaveUnlockReward(s, reward, TRUE);
                PostNotice(s, 3, q, *(const s32*)(reward + 0x18), 0, 0, -1);
            }
            PostNotice(s, 2, 0, 0, msp, 0, 0);
            Quest(s, (u32)*(s32*)(s + 0x6AC))->level = 0xFF;
        }
        *(s32*)(s + 0x6AC) = -1;
        *(s32*)(s + 0x6A8) = 0;
        *(s32*)(s + 0x6B4) = 0;
    }
    memset(s + 0x5B8, 0, 0xF0);
}

}  // extern "C"
