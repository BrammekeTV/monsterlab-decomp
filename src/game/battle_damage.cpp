// Battle: resolve one attack. Works out the damage every targeted limb of the defender takes
// (part strengths and weaknesses, abilities, charge, block and return damage), writes it into
// the round's damage records and the battle log.
// Original: BattleResolveAttack 0x80211180 (auto_03_801F7C88_text.s area)
#include <stddef.h>

#include "types.h"
#include "game/part.h"

#define SIDES       2
#define LIMBS       5
#define EPSILON     0.000001f

enum { DMG_APPLIED = 0x1, DMG_MISSED = 0x2, DMG_INCAPACITATED = 0x10 };

// Ability ids (part status +0x20) that change damage against a discipline, or hit every limb.
enum { ABILITY_VS_BIO = 0, ABILITY_VS_ALCHEMY = 1, ABILITY_HIT_ALL = 3, ABILITY_VS_MECH = 6 };

// Round kinds that never hit.
static inline BOOL RoundMisses(s32 kind) {
    return kind == 4 || kind == 3 || (u32)(kind - 6) <= 2;
}

struct DamageRecord {       // 0x0C
    u8   hit;
    s32** value;            // 0x04: **value = HP change
    u32  flags;             // 0x08
};

struct PartStatus {
    s32 hp;                 // 0x00
    s32 maxHp;              // 0x04
    u8  pad08[0x18];
    s32 ability;            // 0x20: index into the ability table, -1 = none
    s32 resistance;         // 0x24: index into the resistance table, -1 = none
};

struct BattlePart {
    PartInstance* inst;     // 0x00
    s32           slot;     // 0x04
    u8            pad08[0x20];
    PartStatus*   status;   // 0x28
};

// Per-fighter combat state (Fighter + 0x20).
struct FighterStats {
    u8  pad00[0x18];
    u32 actionKey;          // 0x18: chosen action
    u8  charged;            // 0x1C
    u8  pad1D[3];
    s32 targetSlot;         // 0x20: limb the action is used from
    s32 side;               // 0x24
};

struct Fighter {
    BattlePart*  slots[8];  // 0x00
    FighterStats stats;     // 0x20
};

struct Round {
    s32 kind;                                   // 0x00
    struct { DamageRecord* limbs[LIMBS]; u8 pad[0x24]; } side[SIDES];   // 0x04, 0x38 each
};

struct Battle {
    u8       pad000[0x9F2];
    u8       blocking[SIDES];       // 0x9F2
    s32      blockAmount[SIDES];    // 0x9F4
    u8       pad09FC[4];            // 0x9FC: two per-side flag bytes (battle_core.cpp)
    Fighter* fighters[SIDES];       // 0xA00
    s32      attacker;              // 0xA08
    Round*   round;                 // 0xA0C
    u8       pad0A10[0x34];
    void*    log[SIDES];            // 0xA44
    u8       pad0A4C[0x11];
    u8       torsoBonus;            // 0xA5D
    u8       pad0A5E[0xA];
    f32      abilityMul;            // 0xA68
    u8       pad0A6C[4];
    f32      returnMul;             // 0xA70
    f32      chargeMul;             // 0xA74
    f32      matchupBonus;          // 0xA78
    u8       pad0A7C[0xC];
    f32      disciplineBonus;       // 0xA88
    f32      hitAllMul;             // 0xA8C
    f32      hitAllExtraMul;        // 0xA90
    u8       pad0A94[0x20];
    f32      torsoMul;              // 0xAB4
    u8       pad0AB8[0xC];
    s32      aiMulMode3;            // 0xAC4
    s32      aiMulMode1;            // 0xAC8
    u8       pad0ACC[4];
    s32      mode;                  // 0xAD0
    u8       pad0AD4[8];
    f32      fixedDamage[SIDES];    // 0xADC: > 0 overrides the damage (fraction of max HP left)
    s32      protect[SIDES];        // 0xAE4: limbs can't drop below 1 HP this attack
};

static_assert(offsetof(Battle, fighters) == 0xA00 && offsetof(Battle, log) == 0xA44 && offsetof(Battle, torsoBonus) == 0xA5D, "Battle layout");
static_assert(offsetof(Battle, abilityMul) == 0xA68 && offsetof(Battle, torsoMul) == 0xAB4 && offsetof(Battle, mode) == 0xAD0 && offsetof(Battle, protect) == 0xAE4, "Battle layout");
static_assert(offsetof(Fighter, stats) == 0x20 && offsetof(FighterStats, side) == 0x24 && offsetof(BattlePart, status) == 0x28, "fighter layout");

struct AbilityDef    { u8 pad[0x14]; s16 percent; u8 pad16[0xE]; };   // 0x24, db +0x3610
struct ResistanceDef { u8 pad[0x10]; s16 percent; u8 pad12[0xE]; };   // 0x20, db +0x31E8

extern "C" {
s32        SysSprintf(char* buf, const char* fmt, ...);        // 0x80142EFC
void*       operator_new(u32 size);                              // 0x801D40E0
u8*        DamageRecordInit(DamageRecord* r);                   // 0x8021605C
Fighter*    FighterFromStats(FighterStats* stats);               // 0x80213F8C
PartInstance** FighterStatsGetPart(FighterStats* stats, s32 slot);   // 0x80213FE0
const char* PartSlotName(s32 slot);                              // 0x8020E7B4
s32         PartMatchup(s32 defA, s32 defB);                     // 0x80213988: 0 strong, 1 weak
void        RoundNoteAbility(Round* r, s32 side, s32 slot, s32 ability);   // 0x802152B4
void        PartMatchupText(char* out, s32 defA, s32 defB);       // 0x80215F68
s32        BattleLogAdd(void* log, const char* text);           // 0x802207A0
void        BattleAfterAttack(struct Battle* b, FighterStats* att, FighterStats* def);   // 0x802126DC
s32         PartActionStat0(PartInstance* p, u32 actionKey);     // 0x8021DBE8: damage
s32         PartActionStat2(PartInstance* p, u32 actionKey);     // 0x8021DA90
}


static inline void Log(void* log, const char* text) { BattleLogAdd(log, text); }

static inline s32 Discipline(PartInstance* p) {
    PartDef d;
    PartDbGetDef(&d, gGameDb, p->defIndex);
    return d.discipline;
}

static inline f32 AbilityScale(s32 ability) {
    const AbilityDef* t = *(AbilityDef**)(gGameDb->data + 0x3610);
    return 1.0f + (f32)t[ability].percent / 100.0f;
}

static inline DamageRecord* NewDamageRecord(void) {
    DamageRecord* r = (DamageRecord*)operator_new(sizeof(DamageRecord));
    if (r != NULL) DamageRecordInit(r);
    return r;
}

// Bonus for the attacker's ability against the defender's discipline.
static inline BOOL AbilityMatchesDiscipline(s32 ability, PartInstance* target) {
    if (target == NULL) return FALSE;
    switch (ability) {
    case ABILITY_VS_BIO:     return Discipline(target) == PART_BIO;
    case ABILITY_VS_ALCHEMY: return Discipline(target) == PART_ALCHEMY;
    case ABILITY_VS_MECH:    return Discipline(target) == PART_MECH;
    }
    return FALSE;
}

static inline f32 MatchupScale(Battle* b, PartInstance* a, PartInstance* d) {
    f32 m = 1.0f;
    if (a != NULL && d != NULL) {
        s32 r = PartMatchup(a->defIndex, d->defIndex);
        if (r == 0)      m += b->matchupBonus;
        else if (r == 1) m -= b->matchupBonus;
    }
    return m;
}

static inline s32 CeilDamage(f32 v) {
    s32 whole = (s32)v;
    return (s32)(v + (f32)(v - (f32)whole > EPSILON));
}

// 0x80211180
extern "C" void BattleResolveAttack(Battle* b) {
    FighterStats* att = &b->fighters[b->attacker]->stats;
    FighterStats* def = &b->fighters[1 - b->attacker]->stats;
    char text[0x100];

    SysSprintf(text, "Damage Taken:");
    Log(b->log[def->side], text);

    BattlePart* attPart = FighterFromStats(att)->slots[att->targetSlot];
    if (attPart == NULL) return;

    // With the "hit all" ability, every other limb of the defender is hit as well.
    s32 ability = attPart->status->ability;
    s32 addedLimb[LIMBS] = { 0, 0, 0, 0, 0 };
    if (ability != -1 && ability == ABILITY_HIT_ALL && !RoundMisses(b->round->kind)) {
        for (s32 s = 0; s < LIMBS; s++) {
            PartInstance** p = FighterStatsGetPart(def, s);
            BOOL empty = (p == NULL) || ((*(BattlePart**)p)->inst == NULL);
            if (empty) continue;
            DamageRecord** slot = &b->round->side[def->side].limbs[s];
            if (*slot != NULL) continue;
            DamageRecord* r = NewDamageRecord();
            addedLimb[s] = 1;
            b->round->side[def->side].limbs[s] = r;
        }
    }

    PartInstance* attInst = attPart->inst;
    f32 baseDamage = 0.0f, baseReturn = 0.0f;
    if (attInst != NULL) {
        baseDamage = (f32)PartActionStat0(attInst, att->actionKey);
        baseReturn = (f32)PartActionStat2(attInst, att->actionKey);
    }
    if (b->attacker == 1) {         // computer opponent: difficulty scale
        if (b->mode == 1) {
            baseDamage *= (f32)b->aiMulMode1;
            baseReturn *= (f32)b->aiMulMode1;
        } else if (b->mode == 3) {
            baseDamage *= (f32)b->aiMulMode3;
            baseReturn *= (f32)b->aiMulMode3;
        }
    }

    for (s32 s = 0; s < LIMBS; s++) {
        DamageRecord* rec = b->round->side[def->side].limbs[s];
        if (rec == NULL) continue;
        if (RoundMisses(b->round->kind)) {
            rec->hit = 0;
            rec->flags |= DMG_MISSED;
            SysSprintf(text, "MISSED");
            Log(b->log[def->side], text);
            continue;
        }
        if (s == 4 && b->torsoBonus) baseDamage *= b->torsoMul;

        BattlePart* defPart = FighterFromStats(def)->slots[s];
        if (defPart == NULL) continue;
        PartStatus** status = &defPart->status;
        PartInstance* defInst = defPart->inst;
        s32 defAbility = defPart->status->ability;
        s32 defResist = defPart->status->resistance;

        rec->hit = 1;
        f32 scale = MatchupScale(b, attInst, defInst);

        b->abilityMul = 1.0f;
        if (ability != -1) {
            b->abilityMul = 1.0f * AbilityScale(ability);
            if (ability == ABILITY_HIT_ALL) {
                RoundNoteAbility(b->round, b->attacker, att->targetSlot, ability);
                b->abilityMul *= addedLimb[s] == 1 ? b->hitAllExtraMul : b->hitAllMul;
            } else if (defInst != NULL && AbilityMatchesDiscipline(ability, defInst)) {
                scale += b->disciplineBonus;
                RoundNoteAbility(b->round, b->attacker, att->targetSlot, ability);
            }
        }

        f32 damage = b->abilityMul * (baseDamage * scale);
        f32 retBase = b->abilityMul * (baseReturn * scale);
        f32 charge = 0.0f;
        if (att->charged) charge = b->chargeMul * (baseDamage * scale);

        // A blocking defender absorbs some damage and may return some to the attacker.
        if (b->blocking[def->side]) {
            f32 ret = retBase * b->returnMul;
            damage = retBase - (f32)b->blockAmount[def->side];
            if (damage < 0.0f) damage = 0.0f;

            f32 rscale = MatchupScale(b, defInst, attInst);
            if (defAbility != -1) {
                ret *= AbilityScale(defAbility);
                if (attInst != NULL && AbilityMatchesDiscipline(defAbility, attInst)) {
                    RoundNoteAbility(b->round, 1 - b->attacker, defPart->slot, defAbility);
                    rscale += b->disciplineBonus;
                }
            }
            if (defResist != -1) {
                const ResistanceDef* t = *(ResistanceDef**)(gGameDb->data + 0x31E8);
                ret *= 1.0f - (f32)t[defResist].percent / 100.0f;
            }
            ret *= rscale;

            if (ret > 0.0f) {
                f32 attHp = (f32)attPart->status->hp;
                DamageRecord** slot = &b->round->side[b->attacker].limbs[att->targetSlot];
                DamageRecord* back = *slot;
                if (back == NULL) {
                    back = NewDamageRecord();
                    b->round->side[b->attacker].limbs[att->targetSlot] = back;
                }
                // Tutorial battles never let return damage knock out the player's legs.
                BOOL tutorial = (b->mode == 4 || b->mode == 2) && att->targetSlot == 1;
                if (tutorial && attHp <= ret && b->attacker == 0) ret = attHp - 1.0f;
                if (attHp <= ret) {
                    back->flags |= DMG_INCAPACITATED;
                    ret = attHp;
                    SysSprintf(text, "Return damage! %s: Incapacitated!", PartSlotName(s));
                    Log(b->log[att->side], text);
                }
                **back->value = -CeilDamage(ret);
                back->flags |= DMG_APPLIED;
                back->hit = 1;
            }
        }

        s32 hp = (*status)->hp;
        s32 dmg = (s32)(damage + charge);
        if (dmg < 1) dmg = 1;

        char matchup[0x10] = { 0 };
        if (attInst != NULL && defInst != NULL) PartMatchupText(matchup, attInst->defIndex, defInst->defIndex);
        SysSprintf(text, "%s: %d HP %+d Base %s = %d HP", PartSlotName(s), hp, dmg, matchup, hp - dmg);
        Log(b->log[def->side], text);

        f32 fixed = b->fixedDamage[b->attacker];
        if (fixed > EPSILON) dmg = (s32)(1.0f + (f32)(*status)->maxHp * (1.0f - fixed));
        if (dmg == 0) {
            rec->hit = 0;
            rec->flags |= DMG_MISSED;
            continue;
        }

        BOOL protect = b->protect[1 - b->attacker] != 0 ||
                       ((b->mode == 4 || b->mode == 2) && b->attacker == 1 && (s == 1 || s == 4));
        if (protect && hp <= dmg) dmg = hp - 1;
        if (hp <= dmg) {
            rec->flags |= DMG_INCAPACITATED;
            dmg = hp;
            SysSprintf(text, "%s: Incapacitated!", PartSlotName(s));
            Log(b->log[def->side], text);
        }
        **rec->value = -dmg;
        rec->flags |= DMG_APPLIED;
    }

    b->fixedDamage[b->attacker] = -1.0f;
    b->protect[1 - b->attacker] = 0;
    BattleAfterAttack(b, att, def);
}
