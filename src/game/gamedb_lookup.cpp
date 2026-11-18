// Linear lookups in game database tables (global_binary.sir), plus small battle record
// initialisers. Original: 0x80214960, 0x8021605C, 0x80216658-0x80216834, 0x80216B94, 0x80216BF0
#include "types.h"
#include "game/part.h"

extern "C" {
void* ValueBoxCreate(void* box, s32 a, s32 b);   // 0x80213C68
void* operator_new(u32 size);                    // 0x801D40E0
void  operator_delete(void* p);                  // 0x801D426C
}

static inline u8* DbData(GameDb* db) { return db->data; }

extern "C" {

// 0x80214960
// A battle value box record: a value box (kind 0xB) followed by four cleared words.
u8* fn_80214960(u8* r) {
    ValueBoxCreate(r, 0xB, 0);
    *(s32*)(r + 0x8) = 0;
    *(s32*)(r + 0xC) = 0;
    *(s32*)(r + 0x10) = 0;
    *(s32*)(r + 0x14) = 0;
    return r;
}

// 0x8021605C
// Per-limb damage record: flag, a value box allocated on the heap, and a cleared word.
u8* DamageRecordInit(u8* r) {
    r[0] = 0;
    *(s32*)(r + 4) = 0;
    *(s32*)(r + 8) = 0;
    void* box = operator_new(8);
    if (box != NULL) box = ValueBoxCreate(box, 0xB, 0);
    *(void**)(r + 4) = box;
    return r;
}

// 0x80216658
// Row (0x34 bytes) of the table at db+0x2998 whose first word is `key`, or NULL.
u8* fn_80216658(GameDb* db, s32 key) {
    u8* d = DbData(db);
    s32 n = *(s32*)(d + 0x299C);
    for (s32 i = 0; i < n; i++) {
        u8* row = *(u8**)(DbData(db) + 0x2998) + i * 0x34;
        if (*(s32*)row == key) return *(u8**)(d + 0x2998) + i * 0x34;
    }
    return NULL;
}

// 0x802166AC
// Table at db+0x4AD8 (0xC-byte rows): the third word of the row whose first two words are a, b;
// -1 if none.
s32 fn_802166AC(GameDb* db, s32 a, s32 b) {
    s32 n = *(s32*)(DbData(db) + 0x4ADC);
    for (s32 i = 0; i < n; i++) {
        u8* row = *(u8**)(DbData(db) + 0x4AD8) + i * 0xC;
        if (*(s32*)row == a && *(s32*)(row + 4) == b) return *(s32*)(row + 8);
    }
    return -1;
}

// 0x80216700
// Same lookup in the table at db+0x4288.
s32 fn_80216700(GameDb* db, s32 a, s32 b) {
    s32 n = *(s32*)(DbData(db) + 0x428C);
    for (s32 i = 0; i < n; i++) {
        u8* row = *(u8**)(DbData(db) + 0x4288) + i * 0xC;
        if (*(s32*)row == a && *(s32*)(row + 4) == b) return *(s32*)(row + 8);
    }
    return -1;
}

// 0x80216754
// Table at db+0x46B0 (0xC-byte rows): the first word of the row whose second word is `key`;
// -1 if none.
s32 fn_80216754(GameDb* db, u32 key) {
    s32 n = *(s32*)(DbData(db) + 0x46B4);
    for (s32 i = 0; i < n; i++) {
        u8* row = *(u8**)(DbData(db) + 0x46B0) + i * 0xC;
        if (*(u32*)(row + 4) == key) return *(s32*)row;
    }
    return -1;
}

// 0x8021679C
// Ability effect table (db+0x3A38, 0x14-byte rows): the row for an ability, or NULL.
u8* AbilityEffect(GameDb* db, s32 ability) {
    s32 n = *(s32*)(DbData(db) + 0x3A3C);
    for (s32 i = 0; i < n; i++) {
        u8* row = *(u8**)(DbData(db) + 0x3A38) + i * 0x14;
        if (*(s32*)(row + 4) == ability) return row;
    }
    return NULL;
}

// 0x802167E4
// Same, for an ability on a particular limb (+0x08).
u8* AbilityEffectForLimb(GameDb* db, s32 ability, s32 limb) {
    s32 n = *(s32*)(DbData(db) + 0x3A3C);
    for (s32 i = 0; i < n; i++) {
        u8* row = *(u8**)(DbData(db) + 0x3A38) + i * 0x14;
        if (*(s32*)(row + 4) == ability && *(s32*)(row + 8) == limb) return row;
    }
    return NULL;
}

// 0x80216B94
// Reset a record: ids -1 (+0x00, +0xA8), flags cleared; freed if `del` > 0.
u8* fn_80216B94(u8* r, s32 del) {
    if (r != NULL) {
        *(s32*)r = -1;
        *(s32*)(r + 0xA8) = -1;
        r[8] = r[9] = r[0xA] = 0;
        if (del > 0) operator_delete(r);
    }
    return r;
}

// 0x80216BF0
// Clear the flags and set +0x00 to the largest s16 at +0x1C among rows (0x28 bytes, table at
// db+0xAA70) whose first word equals the record's +0x04 (0 if none). Returns 1.
BOOL fn_80216BF0(u8* r) {
    r[8] = r[9] = r[0xA] = 0;
    u8* d = gGameDb->data;
    s32 n = *(s32*)(d + 0xAA74);
    s32 best = 0;
    for (s32 i = 0; i < n; i++) {
        u8* row = *(u8**)(gGameDb->data + 0xAA70) + i * 0x28;
        if (*(s32*)row == *(s32*)(r + 4) && *(s16*)(row + 0x1C) > best) best = *(s16*)(row + 0x1C);
    }
    *(s32*)r = best;
    return TRUE;
}

}  // extern "C"
