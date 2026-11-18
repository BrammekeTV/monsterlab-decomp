// Part instances ("XInventoryPart"): construction, reset, copying, setters, and the stat
// modifiers that enhancements and defects apply.
// Original: 0x8021D180 - 0x8021EFF0 (auto_03_8020F8E0_text.s)
#include <stdint.h>

#include "types.h"
#include "game/part.h"

extern "C" void operator_delete(void* p);         // 0x801D426C
extern u8  gPartVtable[];                         // 0x80389120: { rtti, 0, PartClear }
static s32 sNextPartUid;                          ORIG(0x805FA0A0)

// Enhancement (ability) and defect records: five s16 stat modifiers each.
#define ENHANCEMENT_MODS 0x14     // within the 0x24-byte ability record (db + 0x3610)
#define DEFECT_MODS      0x10     // within the 0x20-byte defect record (db + 0x31E8)
#define STAT_COUNT       5

extern "C" {

// 0x8021D3B4
void PartReset(PartInstance* p) {
    p->defIndex = -1;
    p->madnessBase = 0;
    p->unk1D = 0;
    p->defect = -1;
    p->enhancement = -1;
    p->unk1E = 0;
    p->unk1F = 0;
    p->quality = 100;
    p->tier = 1;
    p->unk24 = 0;
    p->health = 0;
    p->unk20 = 0;
    for (s32 i = 0; i < PART_TIERS; i++) p->tierScale[i] = 0;
}

// 0x8021D408
void PartClear(PartInstance* p) {
    PartReset(p);
    p->unk2C = 0;
}

// 0x8021D320
void PartInit(PartInstance* p) {
    p->vtable = (u32)(uintptr_t)gPartVtable;
    PartClear(p);                 // virtual call through vtable slot 2
    p->uid = sNextPartUid++;
}

// 0x8021D374
void PartDestroy(PartInstance* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
}

// 0x8021DA40
void PartSetQuality(PartInstance* p, s32 q) {
    s8 v = (s8)q;
    if (v > 100)    v = 100;
    else if (v < 0) v = 0;
    p->quality = v;
}

// 0x8021DA68
void PartSetTier(PartInstance* p, u8 tier) {
    if (tier >= 1 && tier <= PART_TIERS) p->tier = tier;
}

// 0x8021EE9C
// Copy everything except the uid's owner fields; the copy's +0x2C is cleared.
void PartCopy(const PartInstance* src, PartInstance* dst) {
    dst->tierScale[3] = src->tierScale[3];
    dst->defIndex = src->defIndex;
    dst->defect = src->defect;
    dst->enhancement = src->enhancement;
    dst->uid = src->uid;
    dst->quality = src->quality;
    dst->tier = src->tier;
    dst->tierScale[0] = src->tierScale[0];
    dst->tierScale[1] = src->tierScale[1];
    dst->tierScale[2] = src->tierScale[2];
    dst->madnessBase = src->madnessBase;
    dst->unk1D = src->unk1D;
    dst->unk1E = src->unk1E;
    dst->unk1F = src->unk1F;
    dst->unk20 = src->unk20;
    dst->unk24 = src->unk24;
    dst->health = src->health;
    dst->unk2C = 0;
}

// 0x8021EFB0
// unk1D scaled by the madness percentage of the current tier.
s8 fn_8021EFB0(const PartInstance* p) {
    return (s8)(((s32)p->unk1D * (s32)p->tierScale[p->tier - 1]) / 100);
}

// 0x8021EFF0
// Percentage modifier for one of five stats: from the enhancement if the part has one,
// otherwise from its defect, otherwise 0.
s16 PartGetStatModifier(const PartInstance* p, s32 stat) {
    const u8* rec;
    u32 base;
    if (p->enhancement != -1) {
        rec = *(u8**)(gGameDb->data + 0x3610) + p->enhancement * 0x24;
        base = ENHANCEMENT_MODS;
    } else if (p->defect != -1) {
        rec = *(u8**)(gGameDb->data + 0x31E8) + p->defect * 0x20;
        base = DEFECT_MODS;
    } else {
        return 0;
    }
    if ((u32)stat >= STAT_COUNT) return 0;
    return *(const s16*)(rec + base + stat * 2);
}

// 0x8021D180
// Map a 0..1 value onto a 0..1 rating through five tuned thresholds (piecewise linear,
// 0.2 per segment). Above the last threshold the rating is 1.
f32 PartRatingFromFraction(f32 v) {
    const f32* t = (const f32*)(*(u8**)(gGameDb->data + 0x99D0) + 0x648);
    f32 x = v * 100.0f;
    f32 t1 = t[1], t2 = t[3], t3 = t[5], t4 = t[7], t5 = t[9];
    if (x <= t1) return (x / t1) / 5.0f;
    if (x > t1 && x <= t2) return 0.2f + (x - t1) * ((1.0f / (t2 - t1)) / 5.0f);
    if (x > t2 && x <= t3) return 0.4f + (x - t2) * ((1.0f / (t3 - t2)) / 5.0f);
    if (x > t3 && x <= t4) return 0.6f + (x - t3) * ((1.0f / (t4 - t3)) / 5.0f);
    f32 r = 1.0f;
    if (x > t4 && x <= t5) r = 0.8f + (x - t4) * ((1.0f / (t5 - t4)) / 5.0f);
    return r;
}

}  // extern "C"

// ---------------------------------------------------------------- health and energy

// Tuning block (gGameDb->data + 0x99D0): +0x3C quality offset, +0x44 quality floor.
static inline f32 Tuning(u32 off) { return *(f32*)(*(u8**)(gGameDb->data + 0x99D0) + off); }

// Base health scaled by quality (into 50..+offset percent), the tier's scale, and the
// health modifier (stat 2) of the part's enhancement or defect.
static inline s32 MaxHealthOf(const PartInstance* p) {
    f32 floor = Tuning(0x44), ofs = Tuning(0x3C);
    f32 q = ((((f32)p->quality - floor) / (100.0f - floor)) * 50.0f + ofs) / 100.0f;
    s32 h = (s32)((f32)p->unk24 * q) * p->tierScale[p->tier - 1];
    return ((h / 100) * (PartGetStatModifier(p, 2) + 100)) / 100;
}

// Fill the definition-derived fields: base power, base health, energy (torsos only) and the
// four tier scales; health starts full.
static void ApplyDef(PartInstance* p, const PartDef& d) {
    const u8* raw = (const u8*)&d;
    p->defIndex = d.index;
    p->madnessBase = (s8)raw[0x4C];
    p->unk24 = *(const s16*)(raw + 0x46);
    p->unk20 = 0;
    if (d.slot == PART_SLOT_TORSO) {
        const u8* energy = *(u8**)(gGameDb->data + 0x7CB8) + d.unk38 * 8;
        p->unk20 = *(const s16*)(energy + 4);
    }
    p->tierScale[0] = (u8)*(const s16*)(raw + 0x44);
    p->tierScale[1] = (u8)*(const s16*)(raw + 0x42);
    p->tierScale[2] = (u8)*(const s16*)(raw + 0x40);
    p->tierScale[3] = (u8)*(const s16*)(raw + 0x48);
    s32 max = MaxHealthOf(p);
    if (p->health > max) p->health = max;
    p->health = max;
}

extern "C" {

// 0x8021D43C
void PartSetDef(PartInstance* p, s32 defIndex) {
    PartClear(p);
    if (defIndex == -1) return;
    PartDef d;
    PartDbGetDef(&d, gGameDb, defIndex);
    PartClear(p);
    ApplyDef(p, d);
}

// 0x8021D708
void PartSetFromDef(PartInstance* p, const PartDef* d) {
    PartClear(p);
    ApplyDef(p, *d);
}

// 0x8021D8EC
void PartSetHealth(PartInstance* p, s32 hp) {
    p->health = hp;
    s32 max = MaxHealthOf(p);
    if (p->health > max) p->health = max;
    if (p->health < 0)        p->health = 0;
    else if (p->health > max) p->health = max;
}

// 0x8021F168
s32 PartGetMaxHealth(PartInstance* p) {
    s32 max = MaxHealthOf(p);
    if (p->health > max) p->health = max;
    return max;
}

// 0x8021ECA8
// Energy a torso gives, raised by the energy modifiers (stat 4) of the monster's working
// parts (inventory layout: five part pointers at +4). Clamped to 1..99.
s32 PartGetEnergy(const PartInstance* torso, u8* monster) {
    s32 pct = 100;
    s32 base = (torso->unk20 * torso->tierScale[torso->tier - 1]) / 100;
    if (monster != NULL) {
        for (s32 i = 0; i < 5; i++) {
            PartInstance* q = ((PartInstance**)(monster + 4))[i];
            if (q != NULL && q->health > 0) pct += PartGetStatModifier(q, 4);
        }
    }
    s32 e = (base * pct) / 100;
    if (e > 99) return 99;
    if (e < 1)  e = 1;
    return e;
}

// 0x8021ED90
// Same in battle, where limbs are battle parts { inst, ..., +0x28 status { hp } }.
s32 PartGetBattleEnergy(const PartInstance* torso, u8** limbs) {
    s32 pct = 100;
    s32 base = (torso->unk20 * torso->tierScale[torso->tier - 1]) / 100;
    if (limbs != NULL) {
        for (s32 i = 0; i < 5; i++) {
            u8* bp = limbs[i];
            if (bp == NULL || *(PartInstance**)bp == NULL) continue;
            s32* hp = *(s32**)(bp + 0x28);
            if (*hp > 0) pct += PartGetStatModifier(*(PartInstance**)bp, 4);
        }
    }
    s32 e = (base * pct) / 100;
    if (e > 99) return 99;
    if (e < 1)  e = 1;
    return e;
}

}  // extern "C"

// ---------------------------------------------------------------- monster ratings (lab screen)

extern "C" {
s32  PartActionStat1(PartInstance* p, u32 actionKey);        // 0x8021DE10
s32  PartActionStat2(PartInstance* p, u32 actionKey);        // 0x8021DA90
s8   PartGetMadness(const PartInstance* p);                  // 0x8021EF80
s32  PartGetCurrentHealth(const PartInstance* p);            // 0x8021EF68
u8   PartGetTier(const PartInstance* p);                     // 0x8021EF58
BOOL PartAllowedAtRank(void* coll, s32 defIndex, s32 tier);  // 0x802191CC
}
extern u8*  gSaveSystem;          // 0x805FBBE0 (+0x128 = rank)
extern void* gLab;     ORIG(0x805FBBC8)

#define MONSTER_LIMBS 5
#define MONSTER_PART(m, i) (((PartInstance**)((u8*)(m) + 4))[i])
#define MONSTER_TORSO(m)   MONSTER_PART(m, 4)

static inline f32 RankPower(s32 rank) {
    const u8* row = *(u8**)(gGameDb->data + 0x63C8) + rank * 0x10;
    return (f32)*(const s16*)(row + 0xC) * Tuning(0x194);
}

extern "C" {

// 0x8021CB98
f32 MonsterRateHealth(u8* m) {
    s32 total = 0;
    for (s32 i = 0; i < MONSTER_LIMBS; i++)
        if (MONSTER_PART(m, i) != NULL) total += PartGetMaxHealth(MONSTER_PART(m, i));
    return PartRatingFromFraction((f32)total / Tuning(0x104));
}

// 0x8021CC44
BOOL MonsterIsFullyHealed(u8* m) {
    for (u32 i = 0; i < MONSTER_LIMBS; i++) {
        PartInstance* p = MONSTER_PART(m, i);
        if (p == NULL) continue;
        s32 max = PartGetMaxHealth(p);
        if (PartGetCurrentHealth(MONSTER_PART(m, i)) < max) return FALSE;
    }
    return TRUE;
}

// 0x8021CCC0
f32 MonsterRateAttack(u8* m) {
    s32 total = 0;
    for (s32 i = 0; i < MONSTER_LIMBS; i++) {
        if (MONSTER_PART(m, i) == NULL) continue;
        PartDef d;
        PartDbGetDef(&d, gGameDb, MONSTER_PART(m, i)->defIndex);
        total += PartActionStat2(MONSTER_PART(m, i), (u32)d.action1);
        total += PartActionStat2(MONSTER_PART(m, i), (u32)d.action2);
    }
    return PartRatingFromFraction((f32)total / Tuning(0x114));
}

// 0x8021CDC0
f32 MonsterRateDefence(u8* m) {
    s32 total = 0;
    for (s32 i = 0; i < MONSTER_LIMBS; i++) {
        if (MONSTER_PART(m, i) == NULL) continue;
        PartDef d;
        PartDbGetDef(&d, gGameDb, MONSTER_PART(m, i)->defIndex);
        total += PartActionStat1(MONSTER_PART(m, i), (u32)d.action1);
        total += PartActionStat1(MONSTER_PART(m, i), (u32)d.action2);
    }
    return PartRatingFromFraction((f32)total / Tuning(0x11C));
}

// 0x8021CEC0
f32 MonsterRateEnergy(u8* m) {
    s32 e = 0;
    if (MONSTER_TORSO(m) != NULL) e = PartGetEnergy(MONSTER_TORSO(m), m);
    return PartRatingFromFraction((f32)e / Tuning(0x10C));
}

// 0x8021CF40
// Total madness relative to the top rank's power.
f32 MonsterRateMadness(u8* m) {
    s32 total = 0;
    for (s32 i = 0; i < MONSTER_LIMBS; i++)
        if (MONSTER_PART(m, i) != NULL) total += PartGetMadness(MONSTER_PART(m, i));
    return (f32)total / RankPower(49);
}

// 0x8021D02C
// Every part is within the power limit of the player's rank.
BOOL MonsterPartsAllowedAtRank(u8* m) {
    for (s32 i = 0; i < MONSTER_LIMBS; i++) {
        PartInstance* p = MONSTER_PART(m, i);
        if (p != NULL && gLab != NULL &&
            !PartAllowedAtRank(gLab, MONSTER_PART(m, i)->defIndex, PartGetTier(p) - 1))
            return FALSE;
    }
    return TRUE;
}

// 0x8021D0B8
// The player's rank progress: this rank's power over the top rank's.
f32 PlayerRankFraction(void) {
    return RankPower(*(s32*)(gSaveSystem + 0x128)) / RankPower(49);
}

}  // extern "C"

// ---------------------------------------------------------------- action stats

// Limbs an action hits, unpacked from the action's target mask (one nibble per limb, a nibble
// of 0xF or >= 5 ends the list).
struct TargetList {
    s32 limbs[5];
    s32 count;      // 0x14
};

extern "C" {

// 0x802161DC
void TargetListInit(TargetList* t) {
    t->count = 0;
    for (s32 i = 0; i < 5; i++) t->limbs[i] = -1;
}

// 0x80216200
TargetList* TargetListDestroy(TargetList* t, s32 del) {
    if (t != NULL && del > 0) operator_delete(t);
    return t;
}

// 0x80216240
// side == 3 mirrors the two arm limbs (2 <-> 3).
void TargetListDecode(TargetList* t, const u32* mask, s32 side) {
    u32 m = *mask;
    t->count = 0;
    for (s32 i = 0; i < 5; i++) t->limbs[i] = (s32)((m >> (i * 4)) & 0xF);
    for (s32 i = 0; i < 5; i++) {
        if (t->limbs[i] == -1 || t->limbs[i] >= 5) {
            t->limbs[i] = -1;
            return;
        }
        t->count = i + 1;
        if (side == 3) {
            if (t->limbs[i] == 3)      t->limbs[i] = 2;
            else if (t->limbs[i] == 2) t->limbs[i] = 3;
        }
    }
}

}  // extern "C"

static inline const u8* Action(u32 id) { return (const u8*)*(PartAction**)(gGameDb->data + 0x430) + id * 0x14; }

// Quality (blended between the tuned low/high factors) times the tier's scale.
static inline f32 ActionScale(const PartInstance* p) {
    f32 hi = Tuning(0x6AC), lo = Tuning(0x6B4);
    f32 q = ((100.0f * lo) + (f32)p->quality * (hi - lo)) / 10000.0f;
    return ((f32)p->tierScale[p->tier - 1] * q) / 100.0f;
}

extern "C" {

// 0x8021DBE8
// Damage of an action: base damage scaled by quality/tier and the part's attack modifier (stat 0),
// split over the targets when it hits more than one limb. Broken parts do base damage.
s32 PartActionStat0(PartInstance* p, u32 action) {
    s16 dmg = *(const s16*)(Action(action) + 0xE);
    if (p->health > 0) {
        s32 d = (s32)((f32)dmg * ActionScale(p));
        dmg = (s16)((d * (PartGetStatModifier(p, 0) + 100)) / 100);
    }
    TargetList t;
    TargetListInit(&t);
    u32 mask = *(const u32*)(Action(action) + 8);
    TargetListDecode(&t, &mask, -1);
    if (t.count > 1) dmg = (s16)(((f32)dmg * Tuning(0x8C)) / (f32)t.count);
    TargetListDestroy(&t, -1);
    return dmg;
}

// 0x8021DE10
// Defence of an action: base defence scaled by quality/tier.
s32 PartActionStat1(PartInstance* p, u32 action) {
    return (s32)((f32)*(const s16*)(Action(action) + 0xC) * ActionScale(p));
}

// 0x8021DA90
// Damage before it is split over targets (used for ratings and return damage).
s32 PartActionStat2(PartInstance* p, u32 action) {
    s16 dmg = *(const s16*)(Action(action) + 0xE);
    if (p->health > 0) {
        s32 d = (s32)((f32)dmg * ActionScale(p));
        dmg = (s16)((d * (PartGetStatModifier(p, 0) + 100)) / 100);
    }
    return dmg;
}

}  // extern "C"

// ---------------------------------------------------------------- action energy cost

extern "C" f64 ceil_(f64 x);    // 0x8033EB2C (MSL ceil)

extern "C" {

// 0x80220998
s32 CeilToInt(f32 v) { return (s32)(f32)ceil_(v); }

}  // extern "C"

enum { ACTION_ENERGY_SCALED = 2, ACTION_ENERGY_ALL = 3 };   // action type +0x08

static inline s32 ActionKind(const u8* action) {
    s32 type = *(const s32*)(action + 4);
    return *(const s32*)(*(u8**)(gGameDb->data + 0x08) + type * 0x24 + 8);
}

// Base cost: the action's cost byte scaled by the tier, and by the part's cost modifier (stat 3)
// while the part works.
static inline s32 BaseCost(const PartInstance* p, const u8* action) {
    s32 c = ((s8)action[0x10] * p->tierScale[p->tier - 1]) / 100;
    if (p->health > 0) c = (c * (PartGetStatModifier(p, 3) + 100)) / 100;
    return c;
}

extern "C" {

// 0x8021DF18
// Cost of an action record. "Scaled" actions instead cost a tuned fraction of the part's first
// action (recursively, if that one is scaled too).
s32 PartActionCostOf(PartInstance* p, const u8* action) {
    s32 c = BaseCost(p, action);
    if (ActionKind(action) != ACTION_ENERGY_SCALED) return c;
    PartDef d;
    PartDbGetDef(&d, gGameDb, p->defIndex);
    const u8* first = Action((u32)d.action1);
    s32 c1 = BaseCost(p, first);
    if (ActionKind(first) == ACTION_ENERGY_SCALED) {
        PartDbGetDef(&d, gGameDb, p->defIndex);
        u8 rec[0x14];
        for (s32 i = 0; i < 0x14; i++) rec[i] = Action((u32)d.action1)[i];
        c1 = -CeilToInt((f32)-PartActionCostOf(p, rec) * Tuning(0x1BC));
    }
    return -CeilToInt((f32)-c1 * Tuning(0x1BC));
}

// 0x8021E280
// Energy an action costs in battle. Actions of kind 3 use all the monster's energy, kind 2 a
// tuned share of it; others cost their own amount.
s32 PartActionCost(PartInstance* p, u32 action, u8** limbs) {
    const u8* a = Action(action);
    if (limbs != NULL) {
        s32 kind = ActionKind(a);
        if (kind == ACTION_ENERGY_ALL) return -PartGetBattleEnergy(p, limbs);
        if (kind == ACTION_ENERGY_SCALED) {
            s32 r = CeilToInt((f32)PartGetBattleEnergy(p, limbs) * Tuning(0x1BC));
            return r - r * 2;
        }
    }
    u8 rec[0x14];
    for (s32 i = 0; i < 0x14; i++) rec[i] = a[i];
    return PartActionCostOf(p, rec);
}

// 0x8021E6EC
// Same outside battle, with the monster's inventory parts.
s32 PartComputeActionCost(PartInstance* p, u32 action, u8* monster) {
    const u8* a = Action(action);
    if (monster != NULL) {
        s32 kind = ActionKind(a);
        if (kind == ACTION_ENERGY_ALL) return -PartGetEnergy(p, monster);
        if (kind == ACTION_ENERGY_SCALED) {
            s32 r = CeilToInt((f32)PartGetEnergy(p, monster) * Tuning(0x1BC));
            return r - r * 2;
        }
    }
    u8 rec[0x14];
    for (s32 i = 0; i < 0x14; i++) rec[i] = a[i];
    return PartActionCostOf(p, rec);
}

}  // extern "C"
