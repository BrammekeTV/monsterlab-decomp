// Battle basics: fighter lookup, part status access, discipline matchups, and committing the
// chosen action (paying its energy).
// Original: 0x80211F98, 0x80213988 - 0x80214EE0, 0x80215F68 (auto_03_8020F8E0_text.s)
#include <string.h>

#include "types.h"
#include "game/part.h"

extern "C" {
const char* DisciplineLetter(s32 discipline);                     // 0x8020E498
s32  PartActionCost(PartInstance* p, u32 action, u8** limbs);     // 0x8021E280
s32  PartGetBattleEnergy(const PartInstance* torso, u8** limbs);  // 0x8021ED90
}
extern u8* gBattle;     // 0x805FBBB8 (+0xA00 fighters[2], +0x9E4 committed[2])

#define PART_DEFS 0xBA

// Fighter (+0x00 battle parts[5], +0x20 stats). Stats +0x04 points at the s32 stat array
// (3 = energy), +0x18 chosen action, +0x1C charged, +0x20 limb, +0x28..+0x44 eight effects.
// Battle part: +0x00 part, +0x28 status { hp, ... }, +0x30..+0x38 three effects.
// An effect record has its id at +0x08.
static inline u8* Fighter(s32 side) { return ((u8**)(gBattle + 0xA00))[side]; }
static inline BOOL EffectIs(const u8* e, s32 id) { return e != NULL && *(const s32*)(e + 8) == id; }

extern "C" {

// 0x80213F8C
u8* FighterFromStats(u8* stats) {
    if (gBattle == NULL) return NULL;
    u8* f = Fighter(0);
    if (f != NULL && f + 0x20 == stats) return f;
    f = Fighter(1);
    if (f != NULL && f + 0x20 == stats) return f;
    return NULL;
}

// 0x80213FE0
// The status field of the battle part in `slot` (NULL if there is no part).
u8** FighterStatsGetPart(u8* stats, s32 slot) {
    u8* f = FighterFromStats(stats);
    if (f == NULL) return NULL;
    u8* bp = ((u8**)f)[slot];
    return bp != NULL ? (u8**)(bp + 0x28) : NULL;
}

// 0x80214950
s32 FighterStatsGet(u8* stats, s32 i) { return (*(s32**)(stats + 4))[i]; }

// 0x80214050
BOOL FighterStatsHasEffect(u8* stats, s32 id) {
    for (s32 i = 0; i < 8; i++)
        if (EffectIs(((u8**)(stats + 0x28))[i], id)) return TRUE;
    return FALSE;
}

// 0x80214EE0
// Effects on one battle part, given its status field.
BOOL FighterStatsHasStatus(u8** status, s32 id) {
    for (s32 i = 0; i < 3; i++)
        if (EffectIs(((u8**)((u8*)status + 8))[i], id)) return TRUE;
    return FALSE;
}

// 0x80213988
// Discipline matchup of part a against part b: 2 same, 0 strong (alchemy > mech > bio >
// alchemy), 1 weak.
s32 PartMatchup(s32 defA, s32 defB) {
    PartDef a, b;
    PartDbGetDef(&a, gGameDb, defA);
    PartDbGetDef(&b, gGameDb, defB);
    s32 da = a.discipline, db = b.discipline;
    if (da == db) return 2;
    if ((da == PART_ALCHEMY && db == PART_MECH) || (da == PART_MECH && db == PART_BIO) ||
        (da == PART_BIO && db == PART_ALCHEMY))
        return 0;
    return 1;
}

// 0x80213A6C
// The next (or previous) part definition that fits the same slot, wrapping around.
s32 PartDefCycleSameSlot(s32 def, BOOL backwards) {
    PartDef cur, next;
    PartDbGetDef(&cur, gGameDb, def);
    do {
        s32 n = backwards ? def - 1 : def + 1;
        def = n;
        if (n >= PART_DEFS) def = 0;
        else if (n == 0)    def = PART_DEFS - 1;
        PartDbGetDef(&next, gGameDb, def);
    } while (next.slot != cur.slot);
    return def;
}

// 0x80215F68
// "(X vs Y)" with each part's discipline letter, for the battle log.
void PartMatchupText(char* out, s32 defA, s32 defB) {
    PartDef a, b;
    PartDbGetDef(&a, gGameDb, defA);
    PartDbGetDef(&b, gGameDb, defB);
    strcpy(out, "(");
    strncpy(out + 1, DisciplineLetter(a.discipline), 1);
    strcpy(out + 2, " vs ");
    strncpy(out + 6, DisciplineLetter(b.discipline), 1);
    strcpy(out + 7, ")");
}

// 0x80211F98
// Lock in a side's chosen action if it can be paid for: the limb and torso must work, an
// "all energy" action is blocked by effect 14, and the remaining energy is capped by what the
// torso can hold.
BOOL BattleCommitAction(u8* battle, s32 side) {
    if (battle[0x9E4 + side] != 0) return TRUE;
    u8* f = ((u8**)(gBattle + 0xA00))[side];
    u8* stats = f + 0x20;
    if (stats == NULL) return FALSE;
    s32 slot = *(s32*)(stats + 0x20);
    u8* bp = ((u8**)f)[slot];
    if (bp == NULL) return FALSE;
    u8** st = FighterStatsGetPart(stats, slot);
    if (st == NULL || **(s32**)st <= 0) return FALSE;
    u8** torso = FighterStatsGetPart(stats, 4);
    if (torso == NULL || **(s32**)torso <= 0) return FALSE;
    PartInstance* inst = *(PartInstance**)bp;
    if (inst == NULL) return FALSE;
    if (FighterStatsHasEffect(stats, 0xE)) {
        u32 action = *(u32*)(stats + 0x18);
        const u8* a = (const u8*)*(PartAction**)(gGameDb->data + 0x430) + action * 0x14;
        s32 type = *(const s32*)(a + 4);
        if (*(const s32*)(*(u8**)(gGameDb->data + 0x08) + type * 0x24 + 8) == 3) return FALSE;
    }
    u8* fb = ((u8**)(battle + 0xA00))[side];
    s32 cost = PartActionCost(inst, *(u32*)(stats + 0x18), (u8**)fb);
    s32 left = FighterStatsGet(fb + 0x20, 3) - cost;
    if (left < 0) return FALSE;
    PartInstance* torsoPart = *(PartInstance**)(((u8**)fb)[4]);
    if (torsoPart != NULL) {
        s32 max = PartGetBattleEnergy(torsoPart, (u8**)fb);
        if (left > max) left = max;
    }
    (*(s32**)(fb + 0x24))[3] = left;
    battle[0x9E4 + side] = 1;
    return TRUE;
}

}  // extern "C"
