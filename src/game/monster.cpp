// Monsters: building a monster from a generated spec (opponents and rewards), and the part
// inventory its parts come from.
// Original: MonsterBuildFromSpec 0x8021BE28, PartInventoryAlloc 0x80219AE8
// (auto_03_8020F8E0_text.s)
#include "types.h"
#include "game/part.h"

extern "C" {
void PartSetDefect(PartInstance* p, s32 d);          // 0x8021DA80
void PartSetEnhancement(PartInstance* p, s32 e);     // 0x8021DA88
void PartClear(PartInstance* p);              // 0x8021D408
s32  PartGetMaxHealth(PartInstance* p);       // 0x8021F168
void PartSetHealth(PartInstance* p, s32 hp);  // 0x8021D8EC
void PartSetQuality(PartInstance* p, s32 q);  // 0x8021DA40
void PartSetTier(PartInstance* p, u8 tier);   // 0x8021DA68
void PartSetDef(PartInstance* p, s32 def);    // 0x8021D43C
}
extern u8* gSaveSystem;      // 0x805FBBE0 (+0xA30 = part inventory)

#define LIMBS            5
#define INVENTORY_PARTS  100

// Monster (vtable at +0): parts[5] at +4 (index 4 = torso), +0x18 total power, +0x44 name,
// +0x48 / +0x49 flags.
#define MONSTER_PART(m, i) (((PartInstance**)((m) + 4))[i])

// Spec fields per monster limb: definition, tier, quality, defect, enhancement flag.
struct SpecLimb { u8 def, tier, quality, defect, enhance; };
static const SpecLimb kSpecLimbs[LIMBS] = {
    { 0x14, 0x3F, 0x35, 0x20, 0x44 },
    { 0x2C, 0x41, 0x42, 0x24, 0x43 },
    { 0x30, 0x37, 0x38, 0x10, 0x39 },
    { 0x1C, 0x36, 0x34, 0x18, 0x40 },
    { 0x0C, 0x3C, 0x3D, 0x28, 0x3E },
};

extern "C" {

// 0x80219AE8
// Take the next unused part from the inventory (100 slots, pointers at +0x28) and clear it.
PartInstance* PartInventoryAlloc(u8* inv) {
    PartInstance* p = NULL;
    s32 used = *(s32*)(inv + 0x6BC);
    if (used < INVENTORY_PARTS) {
        p = ((PartInstance**)(inv + 0x28))[used];
        PartClear(p);                         // virtual call, vtable slot 2
        *(s32*)(inv + 0x6BC) = used + 1;
    }
    return p;
}

static s32 SpecPartPower(s32 defIndex, s8 tier) {
    if (defIndex == -1) return 0;
    PartDef d;
    PartDbGetDef(&d, gGameDb, defIndex);
    const u8* raw = (const u8*)&d;
    s32 p = (s8)raw[0x4C];
    switch (tier) {
    case 1: p *= *(const s16*)(raw + 0x44); break;
    case 2: p *= *(const s16*)(raw + 0x42); break;
    case 3: p *= *(const s16*)(raw + 0x40); break;
    case 4: p *= *(const s16*)(raw + 0x48); break;
    }
    return p / 100;
}

// 0x8021BE28
// Monster vtable slot 0x10. Fill the monster's parts from the spec (allocating parts from the
// player's inventory where a slot is empty), heal everything, and total up its power.
void MonsterBuildFromSpec(u8* m, const u8* spec) {
    (*(void (**)(u8*))(*(u8**)m + 0xC))(m);   // reset

    for (s32 i = 0; i < LIMBS; i++) {
        const SpecLimb& l = kSpecLimbs[i];
        s32 def = *(const s32*)(spec + l.def);
        if (def == -1) continue;
        if (MONSTER_PART(m, i) == NULL) MONSTER_PART(m, i) = PartInventoryAlloc(gSaveSystem + 0xA30);
        if (MONSTER_PART(m, i) == NULL) continue;
        PartSetDef(MONSTER_PART(m, i), def);
        PartSetQuality(MONSTER_PART(m, i), (s8)spec[l.quality]);
        PartSetTier(MONSTER_PART(m, i), spec[l.tier]);
        PartSetDefect(MONSTER_PART(m, i), *(const s32*)(spec + l.defect));
        if (spec[l.enhance] != 0) {
            // The enhancement a part type can have is stored on its definition.
            PartDef d;
            PartDbGetDef(&d, gGameDb, def);
            PartSetEnhancement(MONSTER_PART(m, i), d.unk20);
        }
    }

    for (s32 i = 0; i < LIMBS; i++) {
        PartInstance* p = MONSTER_PART(m, i);
        if (p == NULL) continue;
        PartSetHealth(MONSTER_PART(m, i), PartGetMaxHealth(p));
        MONSTER_PART(m, i)->unk2C = 1;
    }

    *(u32*)(m + 0x44) = *(const u32*)(spec + 8);
    m[0x48] = spec[0x3A] == 0;
    m[0x49] = spec[0x3B] == 0;

    s32 power = 0;
    for (s32 i = 0; i < LIMBS; i++) power += SpecPartPower(*(const s32*)(spec + kSpecLimbs[i].def), (s8)spec[kSpecLimbs[i].tier]);
    *(s32*)(m + 0x18) = power;
}

}  // extern "C"

// ---------------------------------------------------------------- rank limits

// Each rank (table at gGameDb->data + 0x63C8, 0x10 bytes, 50 ranks) allows parts whose power
// (base power times tier scale) is at most its +0xC value times 100.
#define RANKS 50
static inline s32 RankLimit(s32 rank) {
    return *(const s16*)(*(u8**)(gGameDb->data + 0x63C8) + rank * 0x10 + 0xC) * 100;
}
static inline s32 PartTierPower(s32 defIndex, s32 tierIndex) {
    PartDef d;
    PartDbGetDef(&d, gGameDb, defIndex);
    const u8* raw = (const u8*)&d;
    const s32 scale[4] = { *(const s16*)(raw + 0x44), *(const s16*)(raw + 0x42),
                           *(const s16*)(raw + 0x40), *(const s16*)(raw + 0x48) };
    return (s8)raw[0x4C] * scale[tierIndex];
}

// Shop/reward entry: +0x54 index into the table at db+0x5328 (0x18 bytes; +0x13 = tier),
// +0x58 index into db+0x6C18 (0x24 bytes; +0x20 = part definition), +0x68 required rank.
static inline s32 EntryDef(const u8* e)  { return *(const s32*)(*(u8**)(gGameDb->data + 0x6C18) + *(const s32*)(e + 0x58) * 0x24 + 0x20); }
static inline s32 EntryTier(const u8* e) { return (s8)*(*(u8**)(gGameDb->data + 0x5328) + *(const s32*)(e + 0x54) * 0x18 + 0x13); }

extern "C" {

// 0x80219004
void fn_80219004(u8* e, s32 v) {
    *(s32*)(e + 0x5C) = v;
    if (v < 0)      *(s32*)(e + 0x5C) = 0;
    else if (v > 3) *(s32*)(e + 0x5C) = 3;
}

// 0x802191CC
BOOL PartAllowedAtRank(void* lab, s32 defIndex, s32 tierIndex) {
    return PartTierPower(defIndex, tierIndex) <= RankLimit(*(s32*)(gSaveSystem + 0x128));
}

// 0x80219030
BOOL EntryAllowedAtRank(u8* e) {
    if (*(s32*)(e + 0x58) == -1 || *(s32*)(e + 0x54) == -1) return TRUE;
    return PartTierPower(EntryDef(e), EntryTier(e) - 1) <= RankLimit(*(s32*)(gSaveSystem + 0x128));
}

// 0x8021947C
// Lowest rank whose limit allows this part at this tier (49 if none).
void PartComputeRequiredRank(u8* lab, s32 defIndex, s32 tierIndex) {
    s32 rank = 0;
    while (RankLimit(rank) < PartTierPower(defIndex, tierIndex)) {
        if (++rank >= RANKS) {
            rank = RANKS - 1;
            break;
        }
    }
    *(s32*)(lab + 0x68) = rank;
}

// 0x802192B8
void EntryComputeRequiredRank(u8* e) {
    if (*(s32*)(e + 0x58) == -1 || *(s32*)(e + 0x54) == -1) {
        *(s32*)(e + 0x68) = -1;
        return;
    }
    s32 def = EntryDef(e), tier = EntryTier(e);
    s32 rank = 0;
    while (RankLimit(rank) < PartTierPower(def, tier - 1)) {
        if (++rank >= RANKS) {
            rank = RANKS - 1;
            break;
        }
    }
    *(s32*)(e + 0x68) = rank;
}

}  // extern "C"
