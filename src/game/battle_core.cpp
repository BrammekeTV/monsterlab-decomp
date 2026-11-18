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

// ---------------------------------------------------------------- limb and fighter status

extern "C" {
s32  PartGetMaxHealth(PartInstance* p);                   // 0x8021F168
s32  PartGetCurrentHealth(const PartInstance* p);         // 0x8021EF68
s8   PartGetQuality(const PartInstance* p);               // 0x8021EF60
s8   PartGetMadness(const PartInstance* p);               // 0x8021EF80
s8   fn_8021EFB0(const PartInstance* p);
void EffectDestroy(u8* e, s32 del);                       // 0x80216318
void LimbEffectDestroy(u8* e, s32 del);                   // 0x802165A0
void EffectTick(u8* e);                                   // 0x80216360
void EffectExpire(u8* e);                                 // 0x802164A8
void EffectApply(u8* e);                                  // 0x8021640C
void LimbTickEffects(u8** status);                        // 0x80214C98
}

// Limb status (BattlePart + 0x28 points at it, 0x2C bytes): +0 hp, +4 max hp, +8 madness,
// +0x10 quality, +0x14 scaled unk1D, +0x18/+0x1C, +0x20 enhancement, +0x24 defect.
// The status field is followed by three effect slots (+8) and one more effect (+0x14).

extern "C" {

// 0x80214A58
// Rebuild a limb's battle status from its part (or from the definition alone if the part
// has no health), dropping all effects.
void LimbStatusInit(u8** field, PartInstance** part) {
    PartInstance* p = *part;
    s32 def = p != NULL ? p->defIndex : -1;
    for (s32 i = 0; i < 3; i++) {
        u8** e = (u8**)((u8*)field + 8) + i;
        if (*e != NULL) {
            EffectDestroy(*e, 1);
            *e = NULL;
        }
    }
    u8** extra = (u8**)((u8*)field + 0x14);
    if (*extra != NULL) {
        LimbEffectDestroy(*extra, 1);
        *extra = NULL;
    }
    s32* st = *(s32**)field;
    for (s32 i = 0; i < 11; i++) st[i] = 0;
    if (def == -1) return;
    if (PartGetMaxHealth(p) != 0) {
        st[1] = PartGetMaxHealth(p);
        st[4] = PartGetQuality(p);
        st[8] = p->enhancement;
        st[9] = p->defect;
        st[0] = PartGetCurrentHealth(p);
        st[2] = PartGetMadness(p);
        st[5] = fn_8021EFB0(p);
        return;
    }
    PartDef d;
    PartDbGetDef(&d, gGameDb, def);
    const u8* raw = (const u8*)&d;
    st[0] = *(const s16*)(raw + 0x46);
    st[1] = *(const s16*)(raw + 0x46);
    st[2] = (s8)raw[0x4C];
    st[4] = -1;
    st[6] = -1;
    st[7] = -1;
    st[8] = -1;
    st[9] = -1;
}

// 0x80214E28
void LimbApplyEffects(u8** field) {
    for (s32 i = 0; i < 3; i++) {
        u8* e = ((u8**)((u8*)field + 8))[i];
        if (e != NULL) EffectApply(e);
    }
}

// 0x80213E74
// Fill a fighter's battle stats from its limbs: totals of hp, madness, +0xC and +0x14, and
// energy from the torso.
void FighterInitStats(u8* stats, u8** limbs) {
    s32 hp = 0, madness = 0, c = 0, d = 0;
    for (s32 i = 0; i < 5; i++) {
        u8* bp = limbs[i];
        if (bp == NULL) continue;
        u8** field = (u8**)(bp + 0x28);
        LimbStatusInit(field, (PartInstance**)bp);
        s32* st = *(s32**)field;
        hp += st[0];
        madness += st[2];
        c += st[3];
        d += st[5];
    }
    s32* out = *(s32**)(stats + 4);
    out[0] = hp;
    out[1] = madness;
    out[2] = c;
    out[6] = d;
    PartInstance* torso = *(PartInstance**)limbs[4];
    if (torso != NULL) {
        s32 e = PartGetBattleEnergy(torso, limbs);
        out[4] = e;
        out[3] = e;
    }
    for (s32 i = 0; i < 5; i++)
        if (limbs[i] != NULL) LimbApplyEffects((u8**)(limbs[i] + 0x28));
}

// 0x80213090
// Start a battle: fresh stats for both fighters, round 1.
void BattleReset(u8* b) {
    for (s32 side = 0; side < 2; side++) {
        u8* f = ((u8**)(b + 0xA00))[side];
        FighterInitStats(f + 0x20, (u8**)f);
        b[0x9FC + side] = 0;
    }
    *(s32*)(b + 0xA10) = 1;
    *(u32*)(b + 0xA4C) = 0;
    b[0xA5C] = 0;
    b[0xACD] = 0;
    *(s32*)(b + 0xA18) = 0;
    for (s32 i = 0; i < 5; i++) *(u32*)(b + 0xAF0 + i * 4) = 0;
}

// 0x80214674
// End-of-turn: tick the fighter's eight effects (removing expired ones), then each limb's.
void FighterTickEffects(u8* stats) {
    for (s32 i = 0; i < 8; i++) {
        u8** slot = (u8**)(stats + 0x28) + i;
        if (*slot == NULL) continue;
        EffectTick(*slot);
        if (*(s32*)(*slot + 4) <= 0) {
            EffectExpire(*slot);
            EffectDestroy(*slot, 1);
            *slot = NULL;
        }
    }
    for (s32 i = 0; i < 5; i++) {
        u8* f = FighterFromStats(stats);
        u8* bp = ((u8**)f)[i];
        if (bp != NULL) LimbTickEffects((u8**)(bp + 0x28));
    }
}

}  // extern "C"

// ---------------------------------------------------------------- effects and records

extern "C" {
void  operator_delete(void* p);                                   // 0x801D4980
void* operator_new(u32 size);                                     // 0x801D40E0
void* ValueBoxCreate(void* mem, s32 kind, s32 v);                 // 0x80213C68
void  ValueBoxDestroy(void* box, s32 del);                        // 0x80213CE0
void  RoundNoteEffectEnd(void* round, s32 side, s32 limb, s32 id);   // 0x802152CC
}
extern s32 gDefaultActionDisplay;   // 0x805FA098

// Effect: +0 owner stats, +4 turns left, +8 id. Ids 13 and 14 have tuned durations.
static inline s32 EffectDuration(s32 id) {
    const u8* t = *(u8**)(gGameDb->data + 0x99D0);
    return (s32)*(const f32*)(t + (id == 0xE ? 0x214 : 0x204)) + 1;
}

extern "C" {

// 0x802160B8
void* DamageRecordDestroy(u8* r, s32 del) {
    if (r != NULL) {
        ValueBoxDestroy(*(void**)(r + 4), 1);
        if (del > 0) operator_delete(r);
    }
    return r;
}

// 0x80216114
// A 0x18-byte record: flag, four ids (-1) and a value box.
u8* EffectRecordInit(u8* r) {
    r[0] = 0;
    for (s32 i = 0; i < 4; i++) *(s32*)(r + 4 + i * 4) = -1;
    *(void**)(r + 0x14) = NULL;
    void* box = operator_new(8);
    if (box != NULL) box = ValueBoxCreate(box, 0xB, 0);
    *(void**)(r + 0x14) = box;
    return r;
}

// 0x80216180
void* EffectRecordDestroy(u8* r, s32 del) {
    if (r != NULL) {
        ValueBoxDestroy(*(void**)(r + 0x14), 1);
        if (del > 0) operator_delete(r);
    }
    return r;
}

// 0x80216318
void EffectDestroy(u8* e, s32 del) {
    if (e != NULL) {
        *(u32*)e = 0;
        if (del > 0) operator_delete(e);
    }
}

// 0x802165A0
void LimbEffectDestroy(u8* e, s32 del) {
    if (e != NULL) {
        *(u32*)e = 0;
        if (del > 0) operator_delete(e);
    }
}

// 0x80216370
void EffectResetDuration(u8* e) {
    s32 id = *(s32*)(e + 8);
    if (id == 0xD || id == 0xE) *(s32*)(e + 4) = EffectDuration(id);
}

// 0x8021640C
void EffectApply(u8* e) {
    s32 id = *(s32*)(e + 8);
    if (id == 0xD || id == 0xE) *(s32*)(e + 4) = EffectDuration(id);
}

// 0x802164A8
// Tell the round which limb (and side) lost the effect.
void EffectExpire(u8* e) {
    u8* stats = *(u8**)e;
    s32 side = ((u8**)(gBattle + 0xA00))[0] != FighterFromStats(stats) ? 1 : 0;
    s32 limb = -1;
    for (s32 i = 0; i < 5 && limb == -1; i++) {
        u8** field = FighterStatsGetPart(stats, i);
        if (field == NULL) continue;
        u8** slots = (u8**)((u8*)field + 8);
        if (slots == NULL) continue;
        if (slots[0] == e || slots[1] == e || slots[2] == e) limb = i;
    }
    if (limb != -1) RoundNoteEffectEnd(*(void**)(gBattle + 0xA0C), side, limb, *(s32*)(e + 8));
}

}  // extern "C"

// 0x802165E8
// Which stat rows the combat HUD shows for an action kind (table at db + 0x2148).
struct ActionDisplay { u32 key; u8 show[4]; };
extern "C" ActionDisplay PartDbGetActionDisplay(GameDb* db, u32 key) {
    const ActionDisplay* t = *(const ActionDisplay**)(db->data + 0x2148);
    s32 n = *(s32*)(db->data + 0x214C);
    for (s32 i = 0; i < n; i++)
        if (t[i].key == key) return t[i];
    return t[gDefaultActionDisplay];
}

// ---------------------------------------------------------------- turns

extern "C" {
u32  BattleIsFinalRound(u8* b);                           // 0x80212BE4
void BattleAwardWin(u8* b);                               // 0x80213428
}

static inline s32 ActionKindOf(u32 action) {
    const u8* a = (const u8*)*(PartAction**)(gGameDb->data + 0x430) + action * 0x14;
    s32 type = *(const s32*)(a + 4);
    return *(const s32*)(*(u8**)(gGameDb->data + 0x08) + type * 0x24 + 8);
}

extern "C" {

// 0x80213818
// Can the side use its chosen action: not an "all energy" action under effect 14, and the limb
// is not disabled (effect 13).
BOOL BattleActionUsable(u8* b, s32 side) {
    u8* stats = ((u8**)(b + 0xA00))[side] + 0x20;
    if (stats == NULL) return TRUE;
    u32 action = *(u32*)(stats + 0x18);
    u8** field = FighterStatsGetPart(stats, *(s32*)(stats + 0x20));
    if (FighterStatsHasEffect(stats, 0xE) && ActionKindOf(action) == 3) return FALSE;
    if (field != NULL && FighterStatsHasStatus(field, 0xD)) return FALSE;
    return TRUE;
}

// 0x8021223C
// Hand the turn to the next side that can still act. When neither can, the round ends:
// round counter up, the winner is paid if this was the final round, energy regenerates
// (capped by the torso), effects tick, and limbs with the regeneration enhancement (7) heal.
BOOL BattleNextTurn(u8* b) {
    s32 tries = 0;
    for (;;) {
        s32 side = *(s32*)(b + 0xA08);
        s32 left = --*(s32*)(b + 0x9E8 + side * 4);
        if (left >= 0 && BattleCommitAction(b, side) && BattleActionUsable(b, side)) return TRUE;
        side = *(s32*)(b + 0xA08);
        if (b[0x9FC + side] != 0) break;
        tries++;
        *(s32*)(b + 0xA08) = 1 - side;
        if (tries >= 2) break;
    }

    for (s32 s = 0; s < 2; s++) *(u32*)(*(u8**)(((u8**)(b + 0xA00))[s] + 0x20) + 4) = 0;
    *(s32*)(b + 0xA18) = 7;
    (*(s32*)(b + 0xA10))++;
    if (BattleIsFinalRound(b) == 1) {
        BattleAwardWin(b);
        u8* ui = *(u8**)(b + 0xA34);
        *(s32*)(b + 0xA18) = 8;
        (*(void (**)(u8*))(*(u8**)ui + 0x14))(ui);
    }

    for (s32 s = 0; s < 2; s++) {
        u8* f = ((u8**)(b + 0xA00))[s];
        u8* stats = f + 0x20;
        s32 regen = FighterStatsGet(stats, 5);
        s32 e = FighterStatsGet(stats, 3) + regen;
        s32 max = PartGetBattleEnergy(*(PartInstance**)((u8**)f)[4], (u8**)f);
        if (e > max) e = max;
        if (e < 0)   e = 0;
        (*(s32**)(((u8**)(b + 0xA00))[s] + 0x24))[3] = e;
        FighterTickEffects(((u8**)(b + 0xA00))[s] + 0x20);
    }

    s32 healed[2][5];
    for (s32 s = 0; s < 2; s++) {
        for (s32 i = 0; i < 5; i++) {
            healed[s][i] = 0;
            u8* bp = ((u8**)((u8**)(b + 0xA00))[s])[i];
            if (bp == NULL || **(s32**)(bp + 0x28) <= 0) continue;
            u8* gbp = ((u8**)((u8**)(gBattle + 0xA00))[s])[i];
            if (gbp == NULL) continue;
            s32* st = *(s32**)(gbp + 0x28);
            if (st[8] != 7) continue;
            s32 hp = st[0], max = st[1];
            s32 now = hp + (s32)(*(f32*)(b + 0xAA8) * (f32)max);
            s32 gain = now - hp;
            if (now > max) {
                now = max;
                gain = max - hp;
            }
            st[0] = now;
            healed[s][i] = gain;
        }
    }
    u8* hud = *(u8**)(b + 0xA38);
    (*(void (**)(u8*, s32*))(*(u8**)hud + 0x10))(hud, &healed[0][0]);
    return FALSE;
}

}  // extern "C"

// ---------------------------------------------------------------- battle start / end checks

extern "C" {
void SaveCountBattleResult(u8* save, s32 a);              // 0x8022B8A8
void SaveRecordBattleLoss(u8* save, s32 a);               // 0x802271BC
BOOL IsOnlineBattle(void);                                // 0x80220A24
}
extern u8* gSaveSystem;     // 0x805FBBE0

enum { MODE_TUTORIAL = 1, MODE_TIMED = 3 };

static inline s32 LimbHp(u8* f, s32 i) {
    u8* bp = ((u8**)f)[i];
    return bp != NULL ? **(s32**)(bp + 0x28) : 0;
}

extern "C" {

// 0x8020E794
// The status field of a fighter's limb (NULL if the limb is missing).
u8** FighterGetSlot(u8* f, s32 i) {
    u8* bp = ((u8**)f)[i];
    return bp != NULL ? (u8**)(bp + 0x28) : NULL;
}

// 0x80213F84
void FighterStatsSetOwner(u8* stats, u32 v) { *(u32*)stats = v; }

// 0x80212BE4
// Is the battle decided? A side loses when its torso breaks or all four other limbs are
// broken; fleeing on working legs hands the win to the other side; a timed battle (mode 3)
// ends after its round limit in the player's favour.
u32 BattleIsFinalRound(u8* b) {
    if (*(u8**)(b + 0xA4C) != NULL) return 1;
    if (*(s32*)(b + 0xAD0) == MODE_TIMED && *(s32*)(b + 0xA10) == *(s32*)(b + 0xABC)) {
        b[0xA5C] = 1;
        *(u8**)(b + 0xA4C) = ((u8**)(b + 0xA00))[0];
        return 1;
    }
    s32 side = *(s32*)(b + 0xA08);
    if (b[0x9FC + side] != 0) {
        u8** legs = FighterGetSlot(((u8**)(b + 0xA00))[side], 1);
        if (legs != NULL && **(s32**)legs > 0) {
            u8* winner = ((u8**)(b + 0xA00))[1 - *(s32*)(b + 0xA08)];
            *(u8**)(b + 0xA4C) = winner;
            b[0xA5C] = 1;
            if (winner == ((u8**)(b + 0xA00))[1]) {
                SaveCountBattleResult(gSaveSystem, 1);
                if (!IsOnlineBattle()) SaveRecordBattleLoss(gSaveSystem, 1);
                else                   (*(s32*)(gSaveSystem + 0xA2C))++;
            }
            return 1;
        }
    }
    for (s32 v = 0; v < 2; v++) {
        u8* f = ((u8**)(gBattle + 0xA00))[v];
        BOOL beaten = TRUE;
        for (s32 i = 0; i < 4; i++)
            if (((u8**)f)[i] != NULL && LimbHp(f, i) > 0) beaten = FALSE;
        if (((u8**)f)[4] == NULL || LimbHp(f, 4) <= 0) beaten = TRUE;
        if (!beaten) continue;
        *(u8**)(b + 0xA4C) = ((u8**)(b + 0xA00))[1 - v];
        if (*(s32*)(b + 0xAD0) == MODE_TIMED) {
            b[0xA5C] = 1;
            *(u8**)(b + 0xA4C) = ((u8**)(b + 0xA00))[0];
        }
        return 1;
    }
    return 0;
}

// 0x80212E84
// Put a fighter into the battle. For the opponent (side 1) the MSP reward is worked out from
// its madness and average quality; tutorial opponents get 9999 HP limbs, timed-battle
// opponents a scaled HP.
void BattleSetFighter(u8* b, s32 side, u8* f, u32 owner) {
    FighterStatsSetOwner(f + 0x20, owner);
    *(s32*)(f + 0x44) = side;
    s32 quality = 0;
    ((u8**)(b + 0xA00))[side] = f;
    for (s32 i = 0; i < 5; i++) {
        u8* bp = ((u8**)f)[i];
        if (bp != NULL) quality += (*(s32**)(bp + 0x28))[4];
    }
    FighterInitStats(f + 0x20, (u8**)f);
    if (side != 1) return;
    u32 factor = *(u32*)(*(u8**)(gGameDb->data + 0x99D0) + 0x54C);
    *(u32*)(b + 0xAB8) = (u32)FighterStatsGet(((u8**)(b + 0xA00))[1] + 0x20, 1);
    *(u32*)(b + 0xAB8) = (*(u32*)(b + 0xAB8) * factor * (u32)(quality / 5)) / 100;
    switch (*(s32*)(b + 0xAD0)) {
    case MODE_TUTORIAL:
        for (s32 i = 0; i < 5; i++) {
            **(s32**)FighterGetSlot(f, i) = 9999;
            (*(s32**)FighterGetSlot(f, i))[1] = 9999;
        }
        return;
    case MODE_TIMED:
        for (s32 i = 0; i < 5; i++) {
            s32 hp = *(s32*)(b + 0xAC0) * (*(s32**)FighterGetSlot(f, i))[1];
            **(s32**)FighterGetSlot(f, i) = hp;
            (*(s32**)FighterGetSlot(f, i))[1] = hp;
        }
        break;
    }
}

}  // extern "C"

extern "C" {
s32  SaveRollChallengeReward(u8* s, s32 table, u8* monster);  // 0x802262E0: returns the MSP given
void SaveRecordBattleWin(u8* s, s32 a);                       // 0x80226C4C
void SaveAddBadge(u8* s, s32 badge);                          // 0x802233D4
void SaveAddMSP(u8* s, s32 amount);                           // 0x80225F70
void SaveRecordBestScore(u8* s, s32 i, u32 score);            // 0x80225C5C
s32  SaveCurrentScoreSlot(u8* s);                             // 0x802255FC
void OnlineRecordWin(s32 count, u8* s);                       // 0x80140320
void OnlineRecordLoss(s32 count, u8* s);                      // 0x80140324
void LabShowBattleResult(void* lab, const void* result);      // 0x802186D8
}
extern void* gLab;      // 0x805FBBC8

// Battle result shown on the results screen.
struct BattleResult {
    u8  won;            // 0x00
    u8  pad[3];
    s32 unk04;          // 0x04
    s32 unk08;          // 0x08
    u32 msp;            // 0x0C
    s32 unk10;          // 0x10
};

extern "C" {

// 0x80213428
// After the final round: on a win, roll the opponent's loot (or the online table), award the
// "only the torso left" / "only the torso broken" badges, add MSP and show the result; on a
// real loss (not fleeing or time-out), count it.
void BattleAwardWin(u8* b) {
    if (*(s32*)(b + 0xAD0) == MODE_TIMED) return;
    u8* p1 = ((u8**)(b + 0xA00))[0];
    if (*(u8**)(b + 0xA4C) == p1) {
        s32 msp = 0;
        if (!IsOnlineBattle()) {
            u8* opp = ((u8**)(b + 0xA00))[1];
            msp = SaveRollChallengeReward(gSaveSystem, *(s32*)(opp + 0x14), opp);
            SaveRecordBattleWin(gSaveSystem, 1);
            s32 hp[5];
            for (s32 i = 0; i < 5; i++) {
                u8** f = FighterGetSlot(((u8**)(b + 0xA00))[1], i);
                hp[i] = f != NULL ? **(s32**)f : 0;
            }
            if (hp[0] > 0 && hp[2] > 0 && hp[3] > 0 && hp[1] > 0 && hp[4] <= 0) SaveAddBadge(gSaveSystem, 0x59);
            if (hp[0] <= 0 && hp[2] <= 0 && hp[3] <= 0 && hp[1] <= 0 && hp[4] > 0) SaveAddBadge(gSaveSystem, 0x5A);
        } else {
            SaveRollChallengeReward(gSaveSystem, 0x39, NULL);
            OnlineRecordWin((*(s32*)(gSaveSystem + 0xA28))++, gSaveSystem);
        }
        BattleResult r;
        r.won = 1;
        r.unk04 = 0;
        r.unk10 = 0;
        if (msp != 0) {
            r.msp = (u32)msp;
        } else {
            r.msp = *(u32*)(b + 0xAB8);
            SaveAddMSP(gSaveSystem, *(s32*)(b + 0xAB8));
        }
        LabShowBattleResult(gLab, &r);
        if (!IsOnlineBattle()) SaveRecordBestScore(gSaveSystem, SaveCurrentScoreSlot(gSaveSystem), r.msp);
    } else if (b[0xA5C] == 0) {
        if (!IsOnlineBattle()) {
            if (*(s32*)(b + 0xAD0) != MODE_TUTORIAL) SaveRecordBattleLoss(gSaveSystem, 1);
        } else {
            OnlineRecordLoss((*(s32*)(gSaveSystem + 0xA2C))++, gSaveSystem);
        }
    }
}

}  // extern "C"

// ---------------------------------------------------------------- round flow

extern "C" {
void BattleAdvance(u8* b);                                // 0x80212A00 (battle state machine)
s32  BattleDecideRound(u8* b, u8* att, u8* def);          // 0x80210AB8: returns the round kind
u8*  EffectRecordInit(u8* r);                             // 0x80216114
u32  RngNext(void* rng);                                  // 0x80220C8C
}

static inline void CopyChoice(u8* dst, const u8* stats) {
    *(u32*)(dst + 0) = *(const u32*)(stats + 0x18);   // action
    dst[4] = stats[0x1C];                              // charged
    *(s32*)(dst + 8) = *(const s32*)(stats + 0x20);   // limb
}

extern "C" {

// 0x80221240
u32 RngBelow(void* rng, u32 max) { return RngNext(rng) % (max + 1); }

// 0x802124F4
// Start resolving a round once a side has acted (or a round is forced): store both choices
// in the round record, decide what kind of round it is, show it and advance the battle.
void BattleStartRound(u8* b) {
    if (!(b[0xA50] != 0 || b[0xACC] != 0 || BattleNextTurn(b))) return;
    for (s32 i = 0; i < 4; i++) *(u32*)(*(u8**)(b + 0xA3C + i * 4) + 8) = 0;
    u8* round = *(u8**)(b + 0xA0C);
    *(s32*)(round + 0x7C) = *(s32*)(b + 0xA08);
    for (s32 s = 0; s < 2; s++) CopyChoice(round + s * 0x38 + 0x30, ((u8**)(b + 0xA00))[s] + 0x20);

    if (b[0xA50] != 0) {
        *(s32*)round = 7;
        b[0xA50] = 0;
    } else {
        s32 att = *(s32*)(b + 0xA08);
        u8* attStats = ((u8**)(gBattle + 0xA00))[att] + 0x20;
        u8* defFighter = ((u8**)(gBattle + 0xA00))[1 - att];
        u8* rec = (u8*)operator_new(0x18);
        if (rec != NULL) rec = EffectRecordInit(rec);
        rec[0] = 0;
        s32 limb = *(s32*)(attStats + 0x20);
        *(u8**)(round + *(s32*)(attStats + 0x24) * 0x38 + limb * 4 + 0x18) = rec;
        round[0x74] = b[0x9FE];
        round[0x75] = b[0x9FF];
        *(s32*)round = BattleDecideRound(b, attStats, defFighter + 0x20);
    }
    u8* hud = *(u8**)(b + 0xA38);
    (*(void (**)(u8*, u8*))(*(u8**)hud + 0xC))(hud, *(u8**)(b + 0xA0C));
    BattleAdvance(b);
}

// 0x80213308
// Take the acting side's choice: from its AI controller once it has decided (the result is
// the charge flag), or straight away (no charge) for a side without one.
void BattleTakeChoice(u8* b) {
    s32 att = *(s32*)(b + 0xA08);
    u8* ai = *(u8**)(b + 0xA54 + att * 4);
    u8* stats = ((u8**)(gBattle + 0xA00))[*(s32*)(b + 0xA08)] + 0x20;
    if (ai != NULL) {
        u8 charged = (u8)(*(s32 (**)(u8*))(*(u8**)ai + 0xC))(ai);
        if ((*(u8**)(b + 0xA54 + *(s32*)(b + 0xA08) * 4))[0x44] == 0) return;
        stats = ((u8**)(gBattle + 0xA00))[*(s32*)(b + 0xA08)] + 0x20;
        stats[0x1C] = charged;
    } else {
        stats[0x1C] = 0;
    }
    BattleAdvance(b);
}

// 0x80213158
// An animation listener finished. When all are done: the attacking limb's "second strike"
// enhancement (5) may grant another attack (tuned chance), which cancels the defender's block
// after a block round; then the battle advances.
void BattleOnListenerDone(u8* b, u32 who) {
    BOOL all = TRUE;
    for (s32 i = 0; i < *(s32*)(b + 0xA30); i++) {
        if (*(u32*)(b + 0xA1C + i * 4) == who) b[0xA2C + i] = 1;
        all = all && b[0xA2C + i] != 0;
    }
    if (!all) return;
    if (b[0xACC] == 0) {
        b[0xACC] = 1;
        u8* f = ((u8**)(b + 0xA00))[*(s32*)(b + 0xA08)];
        u8** st = FighterGetSlot(f, *(s32*)(f + 0x20 + 0x20));
        if ((*(s32**)st)[8] == 5 && BattleIsFinalRound(b) == 0) {
            BOOL broken = st == NULL || **(s32**)st <= 0;
            if (!broken && (f32)(RngBelow(b + 4, 99) + 1) < 100.0f * *(f32*)(b + 0xAA4)) {
                *(s32*)(b + 0xA18) = 3;
                *(s32*)(b + 0xAEC) = 1;
                if (**(s32**)(b + 0xA0C) == 5) b[0x9F2 + (1 - *(s32*)(b + 0xA08))] = 0;
            }
        }
    }
    BattleAdvance(b);
}

}  // extern "C"
