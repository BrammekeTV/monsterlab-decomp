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
s32  RandomUpTo(s32 max);
s32  RandomRoll(void);        // raw 32-bit random (used unsigned)
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
// Lab constructor: the same clean state as a reset, plus +0x64 and +0x68 set to -1.
u8* fn_8021818C(u8* lab) {
    LabTickReset(lab);
    *(s32*)(lab + 0x64) = -1;
    *(s32*)(lab + 0x68) = -1;
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

// 0x80218BB0
// Work out the MSP the current experiment will give (stored at +0x78): the result part's quality
// value (as in the grade) times a factor from the ingredient's tuning row (db+0x99D0 + 8 * (0xB0 +
// 2 * ingredient)) times the lab's count at +0x74, divided by 100. 0 if there is no result.
void LabCalcExperimentMSP(u8* lab) {
    const u8* cur = (const u8*)gLab;
    s32 q = 0;
    if (*(const s32*)(cur + 0x58) != -1) {
        const u8* row = *(const u8**)(Db() + 0x6C18) + *(const s32*)(cur + 0x58) * 0x24;
        PartDef def;
        PartDbGetDef(&def, gGameDb, *(const s32*)(row + 0x20));
        const u8* exp = *(const u8**)(Db() + 0x5328) + *(const s32*)(lab + 0x54) * 0x18;
        s32 base = 100;
        switch ((s8)exp[0x13]) {
        case 1: base = (s16)def.unk44; break;
        case 2: base = (s16)def.unk42; break;
        case 3: base = (s16)def.unk40; break;
        case 4: base = *(const s16*)&def.unk48[0]; break;
        default: break;
        }
        q = ((s32)(s8)def.unk4C[0] * base) / 100;
    }
    u32 ing = *(const u32*)(cur + 0x48);
    s32 idx = (ing <= 11) ? (s32)(0xB0 + ing * 2) : -1;
    const u8* t = *(const u8**)(Db() + 0x99D0) + idx * 8;
    f32 f = ((f32)*(const s32*)(lab + 0x74)) * *(const f32*)(t + 4);
    *(s32*)(lab + 0x78) = (s32)((f32)q * f) / 100;
}

// 0x80218E6C
// Reward id of challenge row `idx` (0x4C bytes at db+0x80E0) for the lab's current choice:
// category at +0x5C (0..3, else 0) picks a group of four words starting at word 2 + 4 * category,
// and +0x60 picks inside it (1 -> +1, 0 -> +2, 2 -> +3, anything else -> +0).
s32 LabChallengeRewardTable(u8* lab, s32 idx) {
    const s32* row = (const s32*)(*(const u8**)(Db() + 0x80E0) + idx * 0x4C);
    s32 cat = *(s32*)(lab + 0x5C);
    if (cat < 0 || cat > 3) cat = 0;
    s32 sub = *(s32*)(lab + 0x60);
    s32 off = (sub == 1) ? 1 : (sub == 0) ? 2 : (sub == 2) ? 3 : 0;
    return row[2 + cat * 4 + off];
}

}  // extern "C"

// ---------------------------------------------------------------- experiment generator
//
// The lab object also holds the tuning for the current difficulty (+0x80..+0xA8, copied from the
// 0x28-byte rows at db+0xAA70) and the experiment being built: a current record at +0x60 and
// three candidate slots (+0x0C + 0x1C * k, occupied flag at +0x08 + k).
// A record is 0x1C bytes: four key words (kind, bucket, option, item), a value, a factor, a word.

struct ExpRec { s32 key[4]; s32 value; f32 factor; s32 word; };

static inline s16 LabS16(const u8* lab, u32 ofs) { return *(const s16*)(lab + ofs); }

// First random pick: three weights at +0xA2, +0x8C, +0x8E give 0, 1, 2 (-1 if the roll overshoots).
static s32 PickKind(u8* lab) {
    s32 r = RandomUpTo(LabS16(lab, 0x8C) + (LabS16(lab, 0x8E) + LabS16(lab, 0xA2)));
    if (r <= LabS16(lab, 0xA2)) return 0;
    r -= LabS16(lab, 0xA2);
    if (r <= LabS16(lab, 0x8C)) return 1;
    r -= LabS16(lab, 0x8C);
    return (r <= LabS16(lab, 0x8E)) ? 2 : -1;
}

extern "C" {

// 0x80217E90
// Pick one of six buckets from a roll, with weights s16 at +0x90, +0x92, +0x94, +0x96, +0x98,
// +0x9A; -1 if the roll is past the last.
s32 fn_80217E90(u8* lab, s32 r) {
    static const u32 w[6] = { 0x90, 0x92, 0x94, 0x96, 0x98, 0x9A };
    for (s32 i = 0; i < 6; i++) {
        if (r <= LabS16(lab, w[i])) return i;
        r -= LabS16(lab, w[i]);
    }
    return -1;
}

}  // extern "C"

static s32 PickBucket(u8* lab) {
    s32 total = LabS16(lab, 0x9A) + LabS16(lab, 0x98) +
                (LabS16(lab, 0x96) + LabS16(lab, 0x94) + (LabS16(lab, 0x90) + LabS16(lab, 0x92)));
    return fn_80217E90(lab, RandomUpTo(total));
}

static inline s32 Rand3() { return (s32)((u32)RandomRoll() % 3u); }

extern "C" {

// 0x802171A0
// Load the tuning for `level` into the lab: the last row (db+0xAA70, 0x28 bytes) whose id
// matches +0x04 and whose s16 at +0x1C is <= level. 0 if level <= 0 or nothing fits.
BOOL fn_802171A0(u8* lab, s32 level) {
    if (level <= 0) return FALSE;
    if (level == *(s32*)(lab + 0xA8)) return TRUE;
    s32 n = *(const s32*)(Db() + 0xAA74);
    s32 found = -1;
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(Db() + 0xAA70) + i * 0x28;
        if (*(const s32*)row == *(s32*)(lab + 4) && *(const s16*)(row + 0x1C) <= level) found = i;
    }
    if (found == -1) return FALSE;
    const u8* row = *(const u8**)(Db() + 0xAA70) + found * 0x28;
    *(s32*)(lab + 0xA8) = level;
    *(s32*)(lab + 0x80) = *(const s32*)row;
    *(f32*)(lab + 0x84) = *(const f32*)(row + 4);
    *(f32*)(lab + 0x88) = *(const f32*)(row + 8);
    for (s32 k = 0; k < 12; k++) *(s16*)(lab + 0x8C + k * 2) = *(const s16*)(row + 0xC + k * 2);
    lab[0xA4] = row[0x24];
    return TRUE;
}

// 0x80217F20
// Pick a random item (row id at +0 of the 0x18-byte rows at db+0xA220) among the rows with a
// level (+0x14) not below the lab's +0x04 whose match with `x` (fn_80218060) is within the lab's
// limit (+0x84). -1 if none.
s32 fn_80217F20(u8* lab, s32 x) {
    const u8* db = Db();
    s32 n = *(const s32*)(db + 0xA224);
    s32 count = 0;
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(db + 0xA220) + i * 0x18;
        s32 lvl = *(const s32*)(row + 0x14);
        if (lvl == -1 || lvl < *(s32*)(lab + 4)) continue;
        if (fn_80218060(lab, x, *(const s32*)row) <= *(const f32*)(lab + 0x84)) count++;
    }
    if (count == 0) return -1;
    u32 k = (u32)RandomRoll() % (u32)count;
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(db + 0xA220) + i * 0x18;
        s32 lvl = *(const s32*)(row + 0x14);
        if (lvl == -1 || lvl < *(s32*)(lab + 4)) continue;
        if (fn_80218060(lab, x, *(const s32*)row) <= *(const f32*)(lab + 0x84)) {
            if (k == 0) return *(const s32*)row;
            k--;
        }
    }
    return -1;
}

}  // extern "C"

static inline BOOL Key2Eq(const s32* a, const s32* b) { return a[0] == b[0] && a[1] == b[1]; }
static inline BOOL Key3Eq(const s32* a, const s32* b) { return Key2Eq(a, b) && a[2] == b[2]; }
static inline BOOL Key4Eq(const s32* a, const s32* b) { return Key3Eq(a, b) && a[3] == b[3]; }
// kind, bucket and item (not the option) equal
static inline BOOL KeyEqNo2(const s32* a, const s32* b) { return Key2Eq(a, b) && a[3] == b[3]; }

// Find the row (db+0x9DF8, 0x18 bytes) with the record's four keys and a value (s16 +0x14)
// not -1; store the value in the record.
static BOOL FindRecordValue(ExpRec* r) {
    s32 n = *(const s32*)(Db() + 0x9DFC);
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(Db() + 0x9DF8) + i * 0x18;
        s32 v = *(const s16*)(row + 0x14);
        if (v == -1) continue;
        if (*(const s32*)row == r->key[0] && *(const s32*)(row + 4) == r->key[1] &&
            *(const s32*)(row + 8) == r->key[2] && *(const s32*)(row + 0xC) == r->key[3]) {
            r->value = v;
            return TRUE;
        }
    }
    return FALSE;
}

extern "C" {

// 0x802172C4
// Make a new record `out`: with the lab's +0x9E percent chance it keeps the current record's
// kind and bucket (and then half the time its option, with a fresh item), otherwise it rolls
// kind and bucket until they differ from the current record's. Then looks up its value.
BOOL fn_802172C4(u8* lab, ExpRec* out) {
    const ExpRec* cur = (const ExpRec*)(lab + 0x60);
    s32 kind, bucket;
    if (RandomUpTo(99) < LabS16(lab, 0x9E)) {
        kind = cur->key[0];
        bucket = cur->key[1];
    } else {
        do {
            kind = PickKind(lab);
            bucket = PickBucket(lab);
        } while (kind == cur->key[0] && bucket == cur->key[1]);
    }
    s32 option, item;
    if (kind == cur->key[0] && bucket == cur->key[1]) {
        if (RandomUpTo(1) == 0) {
            option = cur->key[2];
            item = fn_80217F20(lab, cur->key[3]);
        } else {
            option = Rand3();
            item = cur->key[3];
        }
    } else {
        option = Rand3();
        item = cur->key[3];
    }
    return fn_802180EC(lab, (s32*)out, kind, bucket, option, item);
}

// 0x80217468
// Make two new records A and B for the current experiment. With the lab's +0x9E percent chance
// both copy the current record's kind and bucket, otherwise each rolls its own until it differs
// from the current. Option and item are then rolled so that A differs from the current record
// and B differs from both A and the current. Returns whether both have a value in the table.
BOOL fn_80217468(u8* lab, ExpRec* a, ExpRec* b) {
    const ExpRec* cur = (const ExpRec*)(lab + 0x60);
    if (RandomUpTo(99) < LabS16(lab, 0x9E)) {
        a->key[0] = cur->key[0]; a->key[1] = cur->key[1];
        b->key[0] = cur->key[0]; b->key[1] = cur->key[1];
    } else {
        do {
            a->key[0] = PickKind(lab);
            a->key[1] = PickBucket(lab);
        } while (Key2Eq(a->key, cur->key));
        do {
            b->key[0] = PickKind(lab);
            b->key[1] = PickBucket(lab);
        } while (Key2Eq(b->key, cur->key));
    }
    do {
        a->key[2] = Rand3();
        a->key[3] = fn_80217F20(lab, cur->key[3]);
    } while (Key4Eq(a->key, cur->key));
    do {
        b->key[2] = Rand3();
        b->key[3] = fn_80217F20(lab, cur->key[3]);
    } while (Key4Eq(b->key, cur->key) || Key4Eq(b->key, a->key));
    if (!FindRecordValue(a)) return FALSE;
    return FindRecordValue(b);
}

// 0x80217908
// Like the above, for the case where a first new record `x` already exists: A and B must also
// differ from `x` (and B from A), and from the current record in the ways the checks below spell
// out (first three keys equal, or kind/bucket/item equal).
BOOL fn_80217908(u8* lab, ExpRec* x, ExpRec* a, ExpRec* b) {
    const ExpRec* cur = (const ExpRec*)(lab + 0x60);
    do {
        a->key[0] = PickKind(lab);
        a->key[1] = PickBucket(lab);
    } while (Key2Eq(a->key, cur->key));
    do {
        b->key[0] = PickKind(lab);
        b->key[1] = PickBucket(lab);
    } while (Key2Eq(b->key, cur->key));
    for (;;) {
        a->key[2] = Rand3();
        a->key[3] = fn_80217F20(lab, cur->key[3]);
        if (Key4Eq(a->key, x->key)) continue;
        if (Key3Eq(a->key, cur->key) || KeyEqNo2(a->key, cur->key)) continue;
        break;
    }
    for (;;) {
        b->key[2] = Rand3();
        b->key[3] = fn_80217F20(lab, cur->key[3]);
        if (Key4Eq(b->key, x->key)) continue;
        if (Key4Eq(b->key, a->key)) continue;
        if (Key3Eq(b->key, cur->key) || KeyEqNo2(b->key, cur->key)) continue;
        break;
    }
    if (!FindRecordValue(a)) return FALSE;
    return FindRecordValue(b);
}

}  // extern "C"

static void ExpRecClear(ExpRec* r) {
    r->key[0] = r->key[1] = r->key[2] = r->key[3] = -1;
    r->value = 0;
    r->factor = -1.0f;
    r->word = 0;
}

// Put a record into a random free candidate slot (+0x0C + 0x1C * k) and mark it occupied.
static s32 PlaceRecord(u8* lab, const ExpRec* r) {
    s32 k = Rand3();
    while (lab[8 + k] == 1) k = Rand3();
    *(ExpRec*)(lab + 0xC + k * 0x1C) = *r;
    lab[8 + k] = 1;
    return k;
}

extern "C" {

// 0x80216C80
// Generate the three candidate records of an experiment at difficulty `level`. Returns 0 if
// the difficulty tuning or any record can't be found. The first record is X (a fresh one, or a
// copy of the current record), then A and B; they go into random distinct slots, and X's slot is
// remembered at +0x7C.
BOOL fn_80216C80(u8* lab, s32 level) {
    lab[8] = lab[9] = lab[0xA] = 0;
    if (!fn_802171A0(lab, level)) return FALSE;
    s32 kind = PickKind(lab);
    s32 bucket = PickBucket(lab);
    s32 option = Rand3();
    s32 item = fn_80217F20(lab, -1);
    if (!fn_802180EC(lab, (s32*)(lab + 0x60), kind, bucket, option, item)) return FALSE;
    ExpRec x, a, b;
    ExpRecClear(&x);
    ExpRecClear(&a);
    ExpRecClear(&b);
    if (RandomUpTo(99) < LabS16(lab, 0xA0)) {
        if (!fn_802172C4(lab, &x)) return FALSE;
        if (!fn_80217908(lab, &x, &a, &b)) return FALSE;
    } else {
        x = *(const ExpRec*)(lab + 0x60);
        if (!fn_80217468(lab, &a, &b)) return FALSE;
    }
    *(s32*)(lab + 0x7C) = PlaceRecord(lab, &x);
    PlaceRecord(lab, &a);
    PlaceRecord(lab, &b);
    return TRUE;
}

}  // extern "C"

extern "C" {

// 0x80217094
// Is the tuning for `level` loaded (loading it if needed) and its flag byte (+0xA4) clear?
BOOL fn_80217094(u8* lab, s32 level) {
    if (level != *(s32*)(lab + 0xA8) && !fn_802171A0(lab, level)) return FALSE;
    return lab[0xA4] == 0;
}

// 0x802170E8
// The factor (+0x88) of the tuning for `level`; 0 if it can't be loaded.
f32 fn_802170E8(u8* lab, s32 level) {
    if (level != *(s32*)(lab + 0xA8) && !fn_802171A0(lab, level)) return 0.0f;
    return *(f32*)(lab + 0x88);
}

// 0x80217134
// Four floats (+4, +8, +0xC, +0x10) of the item row (db+0xA220) with this id; FALSE if none.
BOOL fn_80217134(void* unused, s32 id, f32* a, f32* b, f32* c, f32* d) {
    (void)unused;
    s32 n = *(const s32*)(Db() + 0xA224);
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(Db() + 0xA220) + i * 0x18;
        if (*(const s32*)row == id) {
            *a = *(const f32*)(row + 4);
            *b = *(const f32*)(row + 8);
            *c = *(const f32*)(row + 0xC);
            *d = *(const f32*)(row + 0x10);
            return TRUE;
        }
    }
    return FALSE;
}

}  // extern "C"

// ---------------------------------------------------------------- battle result

extern "C" {
void fn_8022CF24(u8* s, s32 amount);          // 0x8022CF24
void fn_8022D94C(u8* s, s32 amount);          // 0x8022D94C
u32  fn_8022CD34(u8* s, s32 idx);             // 0x8022CD34: record at +0x928
void fn_8022CD54(u8* s, s32 idx, u32 value);  // 0x8022CD54
void fn_802276F4(u8* s, s32 amount);          // 0x802276F4
void fn_8022DE78(u8* s);                      // 0x8022DE78: clears +0x9D0
void fn_80227C20(u8* s, s32 idx, s32 amount); // 0x80227C20
u32  fn_8022D93C(u8* s, s32 idx);             // 0x8022D93C: record at +0x980
void fn_8022D460(u8* s, s32 idx, s32 value);  // 0x8022D460
void fn_802280D0(u8* s, s32 amount);          // 0x802280D0
void LabCalcExperimentMSP(u8* lab);           // 0x80218BB0

// 0x802186D8
// Record the outcome of a battle (`r`: +0 won flag, +4..+0x10 stats) in the lab and update the save's
// statistics. After an experiment fight (+0x48 an ingredient): a win bumps two counters and
// raises the ingredient's best MSP; a loss bumps a counter and clears another. After a challenge
// fight (+0x4C a challenge, not 8; 9 only counts while +0x64 is -1 or 0): a win bumps the
// challenge's counters/records and takes the MSP from its table row; a loss bumps a counter.
void LabShowBattleResult(u8* lab, const u8* r) {
    u8 won = r[0];
    lab[0x6C] = won;
    *(u32*)(lab + 0x70) = *(const u32*)(r + 4);
    *(u32*)(lab + 0x74) = *(const u32*)(r + 8);
    *(u32*)(lab + 0x78) = *(const u32*)(r + 0xC);
    *(u32*)(lab + 0x7C) = *(const u32*)(r + 0x10);
    s32 ing = *(s32*)(lab + 0x48);
    if (ing != -1 && won) {
        fn_8022CF24(gSaveSystem, 1);
        fn_8022D94C(gSaveSystem, 1);
        LabCalcExperimentMSP(lab);
        u32 msp = *(u32*)(lab + 0x78);
        if (fn_8022CD34(gSaveSystem, *(s32*)(lab + 0x48)) < msp)
            fn_8022CD54(gSaveSystem, *(s32*)(lab + 0x48), msp);
    } else if (ing != -1) {
        fn_802276F4(gSaveSystem, 1);
        fn_8022DE78(gSaveSystem);
    } else {
        s32 ch = *(s32*)(lab + 0x4C);
        if (ch == -1) return;
        BOOL counts = (ch != 9) || ((u32)(*(s32*)(lab + 0x64) + 1) <= 1);
        if (won) {
            if (counts) {
                fn_80227C20(gSaveSystem, ch, 1);
                u32 rec = *(u32*)(lab + 0x7C);
                if (fn_8022D93C(gSaveSystem, ch) < rec) fn_8022D460(gSaveSystem, ch, (s32)rec);
            }
            const u8* row = *(const u8**)(Db() + 0x80E0) + *(s32*)((u8*)gLab + 0x4C) * 0x4C;
            *(s32*)(lab + 0x78) = (s32)*(const f32*)(row + 0x48);
        } else if (ch != 8 && counts) {
            fn_802280D0(gSaveSystem, 1);
        }
    }
}

}  // extern "C"
