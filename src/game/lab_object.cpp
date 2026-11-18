// The lab object (gLab): the current experiment (ingredients, result row, grade) and the list of
// rewards collected during a visit. Original: 0x80218060-0x80218588, 0x802188CC, 0x80218998,
// 0x802189E0, 0x80218AFC, plus the reward record at 0x802168DC.
//   +0x08 reward count, +0x0C.. reward entries (0xC bytes: kind, material, count byte at +0x14)
//   +0x48 experiment ingredient, +0x50 experiment id, +0x54 experiment row, +0x58 result row (-1: none)
#include "types.h"
#include "game/part.h"

extern u8* gSaveSystem;     // 0x805FBBE0
extern void* gLab;          // 0x805FBBC8

extern "C" {
void LabTickReset(u8* lab);   // 0x802188E4
void operator_delete(void* p);
}

static inline const u8* Db() { return gGameDb->data; }

extern "C" {

// 0x80218060
// Strength of the match between two things (symmetric table at db+0xA648, 0xC-byte rows); 0 if
// either is -1 or there is no row.
f32 fn_80218060(void* unused, s32 a, s32 b) {
    (void)unused;
    if (a == -1 || b == -1) return 0.0f;
    s32 n = *(const s32*)(Db() + 0xA64C);
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(Db() + 0xA648) + i * 0xC;
        s32 x = *(const s32*)row, y = *(const s32*)(row + 4);
        if ((x == a && y == b) || (x == b && y == a)) return *(const f32*)(row + 8);
    }
    return 0.0f;
}

// 0x802180EC
// Find the row (0x18 bytes, db+0x9DF8) matching four keys whose s16 at +0x14 is set; copy it to
// `out` as { +0, +4, +8, +0xC, s16 +0x14 } (note the order of the words) and return 1.
BOOL fn_802180EC(void* unused, s32* out, s32 k0, s32 k1, s32 k2, s32 k3) {
    (void)unused;
    s32 n = *(const s32*)(Db() + 0x9DFC);
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(Db() + 0x9DF8) + i * 0x18;
        if (*(const s16*)(row + 0x14) == -1) continue;
        if (*(const s32*)row != k0 || *(const s32*)(row + 4) != k1 ||
            *(const s32*)(row + 8) != k2 || *(const s32*)(row + 0xC) != k3) continue;
        out[0] = *(const s32*)row;
        out[1] = *(const s32*)(row + 4);
        out[3] = *(const s32*)(row + 0xC);
        out[2] = *(const s32*)(row + 8);
        out[4] = *(const s16*)(row + 0x14);
        return TRUE;
    }
    return FALSE;
}

// 0x8021818C
// Lab constructor: the same clean state as a reset.
u8* fn_8021818C(u8* lab) {
    LabTickReset(lab);
    return lab;
}

// 0x8021824C
u8* fn_8021824C(u8* lab, s32 del) {
    if (lab != NULL && del > 0) operator_delete(lab);
    return lab;
}

// 0x8021828C
// Grade (0..3) of the current experiment result: the result part's quality value (a per-tier
// s16 of its definition, 100 if the tier is odd, times its multiplier byte / 100) is compared
// against three thresholds that grow with the player's rank. 3 is the best.
s32 LabExperimentGrade(u8* lab) {
    if (*(s32*)(lab + 0x58) == -1) return 0;
    const u8* row = *(const u8**)(Db() + 0x6C18) + *(s32*)(lab + 0x58) * 0x24;
    s32 partDef = *(const s32*)(row + 0x20);
    if (partDef == -1) return 0;
    PartDef def;
    PartDbGetDef(&def, gGameDb, partDef);
    const u8* exp = *(const u8**)(Db() + 0x5328) + *(s32*)(lab + 0x54) * 0x18;
    s32 tier = (s8)exp[0x13];
    s32 base = 100;
    switch (tier) {
    case 1: base = (s16)def.unk44; break;
    case 2: base = (s16)def.unk42; break;
    case 3: base = (s16)def.unk40; break;
    case 4: base = *(const s16*)&def.unk48[0]; break;
    default: break;
    }
    s32 scaled = (s32)(s8)def.unk4C[0] * base;
    f32 value = (f32)(scaled / 100);
    s32 rank = *(s32*)(gSaveSystem + 0x128);
    f32 rankVal = (f32)*(const s16*)(*(const u8**)(Db() + 0x63C8) + rank * 0x10 + 0xC);
    const u8* t = *(const u8**)(Db() + 0x99D0);
    if (value >= rankVal * *(const f32*)(t + 0x67C) + *(const f32*)(t + 0x684)) return 3;
    if (value >= rankVal * *(const f32*)(t + 0x68C) + *(const f32*)(t + 0x694)) return 2;
    if (value >= rankVal * *(const f32*)(t + 0x69C) + *(const f32*)(t + 0x6A4)) return 1;
    return 0;
}

// 0x80218588
// Set the experiment id (+0x50). For a real id, find the result row (0x24 bytes, db+0x6C18)
// whose +0x10 is the id and whose +0x0C is the lab's ingredient (+0x48) and remember it at
// +0x58 (left alone if there is no such row); -1 clears the id and the result row.
void fn_80218588(u8* lab, s32 id) {
    *(s32*)(lab + 0x50) = id;
    if (id == -1) {
        *(s32*)(lab + 0x58) = -1;
        return;
    }
    // The original unrolls this two rows per pass over 78 passes.
    for (s32 i = 0; i < 156; i++) {
        const u8* row = *(const u8**)(Db() + 0x6C18) + i * 0x24;
        if (*(const s32*)(row + 0x10) == id && *(const s32*)(row + 0xC) == *(s32*)(lab + 0x48)) {
            *(s32*)(lab + 0x58) = *(const s32*)row;
            return;
        }
    }
}

// 0x802188CC
// Has an ingredient been chosen for the experiment?
BOOL fn_802188CC(u8* lab) { return *(s32*)(lab + 0x48) != -1; }

// 0x80218998
// Forget the rewards collected so far (the quest id is not used).
void LabQuestFinished(u8* lab, s32 quest) {
    (void)quest;
    *(u32*)(lab + 0x3C) = 0;
    *(s32*)(lab + 0x00) = -1;
    *(u32*)(lab + 0x10) = 0;
    lab[0x14] = 0;
    *(s32*)(lab + 0x0C) = -1;
    *(u32*)(lab + 0x1C) = 0;
    lab[0x20] = 0;
    *(s32*)(lab + 0x18) = -1;
    *(u32*)(lab + 0x28) = 0;
    lab[0x2C] = 0;
    *(s32*)(lab + 0x24) = -1;
    *(u32*)(lab + 0x34) = 0;
    lab[0x38] = 0;
    *(s32*)(lab + 0x30) = -1;
    *(u32*)(lab + 0x08) = 0;
}

// 0x802189E0
// Clear the experiment choice: experiment id, row and result row all -1.
void fn_802189E0(u8* lab) {
    *(s32*)(lab + 0x50) = -1;
    *(s32*)(lab + 0x54) = -1;
    *(s32*)(lab + 0x58) = -1;
}

// 0x80218AFC
// Count a material reward: add to the entry with the same kind and material, or append one.
void LabOnMaterialReward(u8* lab, s32 material, s32 kind, s32 amount) {
    u32 n = *(u32*)(lab + 8);
    for (u32 i = 0; i < n; i++) {
        u8* e = lab + i * 0xC;
        if (*(s32*)(e + 0x10) == material && *(s32*)(e + 0xC) == kind) {
            e[0x14] = (u8)(e[0x14] + amount);
            return;
        }
    }
    u8* e = lab + n * 0xC;
    *(s32*)(e + 0x10) = material;
    e[0x14] = (u8)amount;
    *(s32*)(e + 0xC) = kind;
    *(u32*)(lab + 8) = n + 1;
}

// 0x802168DC
// The experiment result record: four cleared slots (-1 ids, 0 count, -1.0 factor) at +0x0C,
// +0x28, +0x44 and +0x60, ids -1 at +0x00 and +0xA8, the grade of the current experiment at
// +0x04 and the three flag bytes at +0x08..+0x0A cleared.
u8* fn_802168DC(u8* r) {
    *(s32*)r = -1;
    for (s32 k = 0; k < 4; k++) {
        u8* s = r + 0xC + k * 0x1C;
        *(s32*)s = -1; *(s32*)(s + 4) = -1; *(s32*)(s + 8) = -1; *(s32*)(s + 12) = -1;
        *(s32*)(s + 0x10) = 0;
        *(f32*)(s + 0x14) = -1.0f;
        *(s32*)(s + 0x18) = 0;
    }
    *(s32*)(r + 0xA8) = -1;
    *(s32*)(r + 4) = LabExperimentGrade((u8*)gLab);
    r[8] = r[9] = r[0xA] = 0;
    return r;
}

}  // extern "C"
