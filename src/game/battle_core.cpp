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
