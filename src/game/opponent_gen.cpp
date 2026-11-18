// Opponent monsters: build a random monster from an opponent template whose total part power
// fits a window around the player's rank, then give it defects, enhancements and madness.
// Original: MonsterGenerateOpponent 0x8021A1D0 (auto_03_8020F8E0_text.s)
#include <string.h>

#include "types.h"
#include "game/part.h"

extern "C" {
void MonsterSpecRandomize(void* builder, u8* spec, s32 tmpl);                  // 0x8021AC84
BOOL MonsterSpecReroll(void* builder, u8* spec, s16 minPower, s16 maxPower);   // 0x8021AF54
void MonsterSpecAddModifier(void* builder, u8* spec, s32 kind, s32 roll);      // 0x8021BC44
s32  RandomRoll(void);                                                         // 0x80220800
s8   RandomRangeS8(s16 lo, s16 hi);                                            // 0x80220840
}
extern u8* gSaveSystem;     // 0x805FBBE0 (+0x128 = player rank)

#define LIMBS     5
#define TRIES     10
#define MIN_SPAN  125      // power window is at least this wide

// Opponent template (0x34 bytes, gGameDb->data + 0x95A8).
struct OpponentTemplate {
    u32 unk00;
    u32 name;               // 0x04
    u8  pad08[0x14];
    s16 maxPower;           // 0x1C
    s16 defectsMax;         // 0x1E
    s16 defectsMin;         // 0x20
    s16 madnessMin;         // 0x22
    s16 madnessMax;         // 0x24
    s16 enhanceMin;         // 0x26
    s16 enhanceMax;         // 0x28
    s16 rankLow;            // 0x2A: added to the rank power for the window
    s16 rankHigh;           // 0x2C
    s16 minPower;           // 0x2E
    u8  unk30, unk31;       // 0x30
    u8  pad32[2];
};

// Monster spec being built (the generator's record; offsets relative to its start).
// Limb order in the record: def +0x14/+0x2C/+0x0C/+0x30/+0x1C, tier +0x3F/+0x41/+0x3C/+0x37/+0x36.
static const u8 kLimbDef[LIMBS]  = { 0x14, 0x2C, 0x0C, 0x30, 0x1C };
static const u8 kLimbTier[LIMBS] = { 0x3F, 0x41, 0x3C, 0x37, 0x36 };

// A part's power: the definition's base power scaled by the multiplier for its tier.
static s32 PartPower(s32 defIndex, s8 tier) {
    if (defIndex == -1) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, defIndex);
    const u8* d = (const u8*)&def;
    s32 p = (s8)d[0x4C];
    switch (tier) {
    case 1: p *= *(const s16*)(d + 0x44); break;
    case 2: p *= *(const s16*)(d + 0x42); break;
    case 3: p *= *(const s16*)(d + 0x40); break;
    case 4: p *= *(const s16*)(d + 0x48); break;
    }
    return p / 100;
}

static s32 SpecPower(const u8* spec) {
    s32 total = 0;
    for (s32 i = 0; i < LIMBS; i++) total += PartPower(*(const s32*)(spec + kLimbDef[i]), (s8)spec[kLimbTier[i]]);
    return total;
}

// 0x8021A1D0
void MonsterGenerateOpponent(void* builder, s32 tmplIndex) {
    u8* data = gGameDb->data;
    const u8* rankRow = *(u8**)(data + 0x63C8) + *(s32*)(gSaveSystem + 0x128) * 0x10;
    f32 rankScale = *(f32*)(*(u8**)(data + 0x99D0) + 0x194);
    OpponentTemplate t;
    memcpy(&t, *(u8**)(data + 0x95A8) + tmplIndex * 0x34, sizeof(t));

    // Window: the template's limits, narrowed towards the player's rank.
    s32 rankPower = (s32)((f32)*(const s16*)(rankRow + 0xC) * rankScale);
    s32 lo = rankPower + t.rankLow;   if (lo < 0) lo = 0;
    s32 hi = rankPower + t.rankHigh;  if (hi < 0) hi = 0;
    s32 minP = t.minPower, maxP = t.maxPower;
    if (minP < lo && lo < maxP) minP = lo;
    if (maxP > hi && hi < minP) maxP = hi;     // (as in the original: only when hi is below the minimum)
    if (maxP - minP < MIN_SPAN) maxP = minP + MIN_SPAN;

    u8 spec[0x48];
    s32 power = -1;
    for (s32 tries = 0; power < 0 && tries < TRIES; ) {
        if (power < 0) {
            MonsterSpecRandomize(builder, spec, tmplIndex);
            power = SpecPower(spec);
            tries++;
        }
        for (s32 rerolls = 0; (power < minP || power > maxP) && rerolls < TRIES; ) {
            rerolls++;
            power = MonsterSpecReroll(builder, spec, (s16)minP, (s16)maxP) ? SpecPower(spec) : -1;
        }
    }

    spec[0x3A] = t.unk30;
    spec[0x3B] = t.unk31;
    *(u32*)(spec + 0x08) = t.name;
    s32 defects = RandomRangeS8(t.defectsMin, t.defectsMax);
    s32 enhancements = RandomRangeS8(t.enhanceMin, t.enhanceMax);

    // Clear every limb's modifier slot.
    spec[0x44] = 0; *(s32*)(spec + 0x20) = -1;
    spec[0x43] = 0; *(s32*)(spec + 0x24) = -1;
    spec[0x3E] = 0; *(s32*)(spec + 0x28) = -1;
    spec[0x39] = 0; *(s32*)(spec + 0x10) = -1;
    spec[0x40] = 0; *(s32*)(spec + 0x18) = -1;

    if (defects > 0) {
        if (defects > LIMBS) defects = LIMBS;
        for (s32 i = 0; i < defects; i++) MonsterSpecAddModifier(builder, spec, 0, RandomRoll());
    }
    if (enhancements > LIMBS - defects) enhancements = LIMBS - defects;
    if (enhancements > 0) {
        s32 n = enhancements < LIMBS ? enhancements : LIMBS;
        for (s32 i = 0; i < n; i++) MonsterSpecAddModifier(builder, spec, 1, RandomRoll());
    }

    // Madness per limb.
    static const u8 kLimbMadness[LIMBS] = { 0x35, 0x42, 0x3D, 0x38, 0x34 };
    for (s32 i = 0; i < LIMBS; i++) spec[kLimbMadness[i]] = (u8)RandomRangeS8(t.madnessMin, t.madnessMax);

    (*(void (**)(void*, u8*))(*(u8**)builder + 0x10))(builder, spec);
}
