// Small functions in 0x8020xxxx (0x8020E530 - 0x8020F3D0): object constructors and deleting
// destructors for fighter stats, limb and part records, a slot-table link helper, and the
// generic "free if del > 0" destructor stubs used by several classes.
// Skipped (unclear): 0x80201A94, 0x80201B94 (audio table selectors; 0x80201AD8/0x80201BD4 return
// types unknown), 0x80204978 (sda21 constant argument and unknown 0x80146808/0x80146930/0x801469FC),
// 0x8020EAD4 (0x8032F1E8 is declared with a different signature in small_8006.cpp).
#include "types.h"
#include "game/part.h"

extern "C" {
void    operator_delete(void* p);                                   // 0x801D426C
u8*     FighterStatsInit(u8* s, u32 owner, s32 side);               // battle_core.cpp
u8*     FighterStatsDestroy(u8* stats, s32 del);                    // battle_ai.cpp
u8*     fn_80214960(u8* r);                                         // gamedb_lookup.cpp
void    LimbStatusInit(u8** field, PartInstance** part);            // 0x80214A58
void    PartSetQuality(PartInstance* p, s32 q);                     // 0x8021DA40
void    PartSetHealth(PartInstance* p, s32 hp);                     // 0x8021D8EC
void    PartCopy(const PartInstance* src, PartInstance* dst);       // part_instance.cpp
void    PartDestroy(PartInstance* p, s32 del);                      // 0x8021D374
void    __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count); // 0x8032F2E0
void    MonsterReset(u8* m);                                        // lab_inventory.cpp
u8*     MonsterDtor(u8* m, s32 del);                                // lab_inventory.cpp
}

extern "C" {

// 0x8020E530
// Constructor: stores the owner id at +0x1C, sets the -1 id at +0x14 and two flag bytes, builds the
// embedded fighter stats block at +0x20 and clears the five words at +0x0..+0x10.
u8* fn_8020E530(u8* p, s32 a) {
    *(s32*)(p + 0x1C) = a;
    *(s32*)(p + 0x14) = -1;
    *(u8*)(p + 0x18)  = 1;
    *(u8*)(p + 0x19)  = 1;
    FighterStatsInit(p + 0x20, 0, -1);
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x4) = 0;
    *(s32*)(p + 0x8) = 0;
    *(s32*)(p + 0xC) = 0;
    *(s32*)(p + 0x10) = 0;
    return p;
}

// 0x8020E59C
// Deleting destructor: clears the five words at +0x0..+0x10, destroys the embedded fighter stats
// block at +0x20 (FighterStatsDestroy with -1), then frees the object if del > 0.
void fn_8020E59C(u8* p, s32 del) {
    if (p == NULL) return;
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x4) = 0;
    *(s32*)(p + 0x8) = 0;
    *(s32*)(p + 0xC) = 0;
    *(s32*)(p + 0x10) = 0;
    FighterStatsDestroy(p + 0x20, -1);
    if (del > 0) operator_delete(p);
}

// 0x8020E610
// Puts elem into slot idx of a pointer table (4-byte slots) owned by table. Whatever was in the
// slot before is unlinked first (its index set to -1 and its back-pointer cleared). A non-null elem
// gets its index and a back-pointer to the table.
void fn_8020E610(u8* table, s32 idx, u8* elem) {
    u8* old = *(u8**)(table + idx * 4);
    if (old != NULL) {
        *(s32*)(old + 0x4) = -1;
        *(u8**)(old + 0x24) = NULL;
    }
    *(u8**)(table + idx * 4) = elem;
    if (elem != NULL) {
        *(s32*)(elem + 0x4) = idx;
        *(u8**)(elem + 0x24) = table;
    }
}

// 0x8020E7EC
// Constructor: stores a at +0x0 and b at +0x20, sets the -1 id at +0x4, clears the flag and counter
// words, runs the base constructor on the sub-record at +0x28, and initialises the limb status
// for the object.
u8* fn_8020E7EC(u8* p, u8* a, s32 b) {
    *(u8**)(p + 0x0)  = a;
    *(s32*)(p + 0x4)  = -1;
    *(s32*)(p + 0x8)  = 0;
    *(s32*)(p + 0x20) = b;
    *(s32*)(p + 0x24) = 0;
    fn_80214960(p + 0x28);
    *(s32*)(p + 0xC)  = 0;
    LimbStatusInit((u8**)(p + 0x28), (PartInstance**)p);
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = 0;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1C) = 0;
    return p;
}

// 0x8020E950
// Applies the stat block at +0x28 to the part at +0x0: quality, a copied word at +0x8, health,
// and two byte fields at +0x1C and +0x1D.
void fn_8020E950(u8* p) {
    u8* src = *(u8**)(p + 0x28);
    PartSetQuality(*(PartInstance**)p, (s8)*(s32*)(src + 0x10));
    src = *(u8**)(p + 0x28);
    *(s32*)(*(u8**)p + 0x8) = *(s32*)(src + 0x20);
    src = *(u8**)(p + 0x28);
    PartSetHealth(*(PartInstance**)p, *(s32*)(src + 0x0));
    src = *(u8**)(p + 0x28);
    *(u8*)(*(u8**)p + 0x1C) = (u8)*(s32*)(src + 0x8);
    src = *(u8**)(p + 0x28);
    *(u8*)(*(u8**)p + 0x1D) = (u8)*(s32*)(src + 0x14);
}

// 0x8020E9E0
// Deleting destructor with no base call: frees the object if del > 0.
void fn_8020E9E0(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
}

// 0x8020EA44
// Deleting destructor with no base call: frees the object if del > 0; returns the object.
u8* fn_8020EA44(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
    return p;
}

// 0x8020EA94
// Deleting destructor with no base call: frees the object if del > 0; returns the object.
u8* fn_8020EA94(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
    return p;
}

// 0x8020EB44
// Deleting destructor: destroys the array of five 0x30-byte parts at +0x4C (PartDestroy), runs the
// base destructor on the object, then frees it if del > 0.
void fn_8020EB44(u8* p, s32 del) {
    if (p == NULL) return;
    __destroy_arr(p + 0x4C, (void (*)(void*, s32))PartDestroy, 0x30, 5);
    MonsterDtor(p, 0);
    if (del > 0) operator_delete(p);
}

// 0x8020EDAC
// Resets the monster and marks the five part slots at +0x4C, +0x7C, +0xAC, +0xDC and +0x10C
// (0x30 apart) as empty by setting their id to -1.
void fn_8020EDAC(u8* p) {
    MonsterReset(p);
    *(s32*)(p + 0x4C)  = -1;
    *(s32*)(p + 0x7C)  = -1;
    *(s32*)(p + 0xAC)  = -1;
    *(s32*)(p + 0xDC)  = -1;
    *(s32*)(p + 0x10C) = -1;
}

// 0x8020F188
// Copies src into part slot idx (five 0x30-byte parts at +0x4C) and records the part in the
// pointer table at +0x4. Does nothing when idx is -1.
void fn_8020F188(u8* p, s32 idx, u8* src) {
    if (idx == -1) return;
    u8* part = p + idx * 0x30 + 0x4C;
    PartCopy((const PartInstance*)src, (PartInstance*)part);
    *(u8**)(p + 4 + idx * 4) = part;
}

// 0x8020F1F0
// Releases part slot idx: if the slot is in use, its id is set to -1 and the slot pointer is cleared.
void fn_8020F1F0(u8* p, s32 idx) {
    if (idx == -1) return;
    if (*(u8**)(p + 4 + idx * 4) == NULL) return;
    *(s32*)(p + idx * 0x30 + 0x4C) = -1;
    *(u8**)(p + 4 + idx * 4) = NULL;
}

// 0x8020F23C
// Deleting destructor with no base call: frees the object if del > 0.
void fn_8020F23C(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
}

// 0x8020F2B4
// Deleting destructor with no base call: frees the object if del > 0.
void fn_8020F2B4(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
}

// 0x8020F31C
// Deleting destructor with no base call: frees the object if del > 0.
void fn_8020F31C(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
}

// 0x8020F394
// Choice record destructor: frees the choice if del > 0; returns the choice.
void* ChoiceDestroy(void* c, s32 del) {
    if (c != NULL && del > 0) operator_delete(c);
    return c;
}

}  // extern "C"
