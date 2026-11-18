// Lab books UI: badges page callbacks and page helpers (0x8011xxxx).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                   // 0x805FB248 (lab books section table pointer)
extern u8  lbl_805FB250[];                   // 0x805FB250 (texture pointer, released by PartInfo_UnloadModel)
extern u8  lbl_805FB254[];                   // 0x805FB254 (texture pointer, released by PartInfo_UnloadModel)
extern u8  lbl_805FB290[];                   // 0x805FB290 (byte flag)
extern u8  lbl_805FB291[];                   // 0x805FB291 (byte flag)
extern u8  lbl_805F8F20[];                   // 0x805F8F20 (pointer to the selected entry)
extern u8  lbl_805FBBE0[];                   // 0x805FBBE0 (save block pointer)
extern u8  lbl_803E6B28[];                   // 0x803E6B28 (list entry record: +0 kind, +4 index)
extern u8  lbl_803E6B34[];                   // 0x803E6B34 (list entry record: +0 kind, +4 index)
extern const char lbl_805F8200[];            // 0x805F8200 (empty string, fallback text)
struct StringTable;
extern "C++" StringTable* gStringTable;                   // 0x805FB910
extern void* gLab;                           // 0x805FBBC8

void     operator_delete(void* p);           // 0x801D426C
void     TextureRelease(void* tex);          // 0x80184028
BOOL     LabTakeMaterial(u8* inv, s32 index, s32 kind, u32 amount); // 0x80219ED4
void     fn_80218588(u8* lab, s32 id);       // 0x80218588
void     fn_802186D0(u8* lab, s32 id);       // 0x802186D0
void     fn_802188A8(u8* lab, u8* entry);    // 0x802188A8
void     fn_8011A04C(void);                  // 0x8011A04C
u8*      fn_80116D00(u8* p, s32 del);        // 0x80116D00
u8*      fn_80116C04(u8* p, s32 del);        // 0x80116C04
void     fn_8011CD18(u8* p);                 // 0x8011CD18 (badge list rebuild)
s32      fn_8011CF64(u8* p, s32 idx);        // 0x8011CF64 (1 if idx is inside the badge list)
void     fn_8011CDD4(u8* p, s32 row);        // 0x8011CDD4 (row step, then tail call into 0x80116EF0)
u8       fn_8011CD10(u8* p);                 // 0x8011CD10 (ignores p, returns the byte flag at lbl_805FB2A8)
s32      fn_8020E4E8(s32 a);                 // 0x8020E4E8 (maps a hash to 0..2, or -1)
}

extern "C" {

// StringHash of "badges" (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashBadges = 0x880E941Cu;  // "badges"

// Creatures, Ingredients and Parts hashes (same values as lab_books_a.cpp).
static const u32 kHashCreatures   = 0xE5368B8Bu;
static const u32 kHashIngredients = 0xC66383A7u;
static const u32 kHashParts       = 0x9E14654Eu;

typedef u8* (*HashMethod)(u8* self, u32 hash);
typedef void (*VoidMethod)(u8* self);

// Calls the virtual slot at byte offset `off` of obj with a hash argument; returns r3.
static u8* CallVirt(u8* obj, s32 off, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + off);
    return fn(obj, hash);
}

// Calls the virtual slot at byte offset `off` of obj with no argument.
static void CallVoidVirt(u8* obj, s32 off) {
    VoidMethod fn = *(VoidMethod*)(*(u8**)obj + off);
    fn(obj);
}

// Badges section: lbl_805FB248 -> table pointer -> +8 sub-object (NULL if the table is missing).
// Its vfunc[0x28] with the "badges" hash returns the badge page object (NULL if none).
static u8* BadgesObj(void) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    u8* sec = *(u8**)(top + 8);
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashBadges);
}

// Shared lookup for fn_80118F44: strings[index] from gStringTable when the table holds more
// than `index` entries, otherwise the empty fallback string.
static const char* StringTableEntry(u32 index, s32 byteOff) {
    u8* st = (u8*)gStringTable;
    u8* strings = *(u8**)st;
    if (strings == 0 || *(u32*)(st + 0xc) <= index) return lbl_805F8200;
    return *(const char**)(strings + byteOff);
}

// 0x80115A80
void Badges_BadgeList_Generate(void) {
    u8* p = BadgesObj();
    if (p != 0) fn_8011CD18(p);
}

// 0x80115AE8
s32 Badges_DoesBadgeExist(s32 a0) {
    u8* p = BadgesObj();
    if (p == 0) return 0;
    return fn_8011CF64(p, a0);
}

// 0x80115CE8
// Steps the badge row by a0 (through the list's row helper) when the page exists.
void Badges_IncRow(s32 a0) {
    u8* p = BadgesObj();
    if (p == 0) return;
    fn_8011CDD4(p, a0);
}

// 0x80115EB8
s32 Badges_IsTextureAsyncLoaded(void) {
    u8* p = BadgesObj();
    if (p == 0) return 0;
    return fn_8011CD10(p);
}

// 0x80117320
// Selects entry idx (reset to 0 when past the count at +0x34), loads its value from the save
// block table (lbl_805FBBE0 + 0xA30 + idx*4) into +0x20, sets +0x4 to idx + 1 and, when the
// lightning tower is on the operating table, moves the monster inventory to that entry.
void fn_80117320(u8* p, s32 idx) {
    *(s32*)(p + 0x2c) = idx;
    if (idx >= *(s32*)(p + 0x34)) *(s32*)(p + 0x2c) = 0;
    s32 cur = *(s32*)(p + 0x2c);
    u8* save = *(u8**)lbl_805FBBE0;
    *(s32*)(p + 0x20) = *(s32*)(save + 0xa30 + cur * 4);
    *(s32*)(p + 0x4) = cur + 1;
    if (LightningTowerGetOnOperatingTable() > 0) SetMonsterInventoryPos(*(s32*)(p + 0x2c));
    *(s32*)(p + 0x38) = 0;
    *(s32*)(p + 0x3c) = 0;
}

// 0x80117ECC
// Deleting destructor: destroys the sub-object at +0x14AC, then frees p when del > 0.
u8* fn_80117ECC(u8* p, s32 del) {
    if (p != 0) {
        fn_80116D00(p + 0x14ac, -1);
        fn_80116C04(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80118ED4
// Stores the mode for hash-or-index value (via fn_8020E4E8; invalid values become 3). Changing
// the mode clears +0x30.
void fn_80118ED4(u8* p, s32 val) {
    s32 idx = fn_8020E4E8(val);
    s32 old = *(s32*)(p + 0x20);
    if (idx < 0 || idx >= 3) *(s32*)(p + 0x20) = 3;
    else *(s32*)(p + 0x20) = idx;
    if (*(s32*)(p + 0x20) != old) *(s32*)(p + 0x30) = 0;
}

// 0x80118F44
// Lab books type name for the current kind at +0x28: kind 0 uses string 0x466, kind 1 uses
// string 0x483, any other kind gives NULL. Falls back to the empty string when missing.
const char* fn_80118F44(u8* p) {
    s32 kind = *(s32*)(p + 0x28);
    if (kind == 0) return StringTableEntry(0x466, 0x466 * 4);
    if (kind == 1) return StringTableEntry(0x483, 0x483 * 4);
    return 0;
}

// 0x80119798
// Selects the entry p: records it in lbl_805F8F20, resets both list records (index -1, kind 0 /
// 1), sets the two flag bytes, refreshes the list and tells the lab about the new selection.
void fn_80119798(u8* p) {
    *(void**)lbl_805F8F20 = p;
    *(s32*)(lbl_803E6B28 + 0x4) = -1;
    *(s32*)(lbl_803E6B28 + 0x0) = 0;
    *(s32*)(lbl_803E6B34 + 0x4) = -1;
    *(s32*)(lbl_803E6B34 + 0x0) = 1;
    *(u8*)lbl_805FB290 = 1;
    *(u8*)lbl_805FB291 = 0;
    fn_8011A04C();
    if (gLab != 0) fn_802188A8((u8*)gLab, p);
}

// 0x80119BA8
// Clears the selected-entry pointer and both list indices, then notifies the lab (-1 index).
void fn_80119BA8(void) {
    *(void**)lbl_805F8F20 = (void*)(intptr_t)-1;
    *(s32*)(lbl_803E6B28 + 0x4) = -1;
    *(s32*)(lbl_803E6B34 + 0x4) = -1;
    fn_80218588((u8*)gLab, -1);
    fn_802186D0((u8*)gLab, -1);
}

// 0x80119BFC
// Takes one of each list record's material from the save block inventory (lbl_805FBBE0 + 0xA30).
void fn_80119BFC(void) {
    LabTakeMaterial(*(u8**)lbl_805FBBE0 + 0xa30, *(s32*)(lbl_803E6B28 + 0x4), *(s32*)(lbl_803E6B28 + 0x0), 1);
    LabTakeMaterial(*(u8**)lbl_805FBBE0 + 0xa30, *(s32*)(lbl_803E6B34 + 0x4), *(s32*)(lbl_803E6B34 + 0x0), 1);
}

// 0x8011A524
// Releases the child currently selected by index +0x18 (its vfunc 0x10), then clears the index.
void fn_8011A524(u8* p) {
    s32 idx = *(s32*)(p + 0x18);
    if (idx != -1) {
        u8* child = *(u8**)(p + 0x8 + idx * 4);
        CallVoidVirt(child, 0x10);
    }
    *(s32*)(p + 0x18) = -1;
}

// 0x8011A584
// Maps a list hash to its index: Creatures 0, Ingredients 1, the 0xEBD0DA39 hash 2, Parts 3;
// anything else is -1. The first argument is unused (r3 is ignored by the original).
s32 fn_8011A584(u8* unused, s32 hash) {
    (void)unused;
    if (hash == (s32)kHashCreatures) return 0;
    if (hash >= (s32)kHashCreatures) {
        return (hash == (s32)0xEBD0DA39u) ? 2 : -1;
    }
    if (hash == (s32)kHashIngredients) return 1;
    if (hash >= (s32)kHashIngredients) return -1;
    if (hash == (s32)kHashParts) return 3;
    return -1;
}

// 0x801167D4
// Drops one reference from each of the two cached model textures (lbl_805FB250 / lbl_805FB254);
// a texture is released when its count reaches zero, and the cache slot is cleared.
void PartInfo_UnloadModel(void) {
    u8* a = *(u8**)lbl_805FB250;
    if (a != 0) {
        s32 refs = *(s32*)(a + 0x24) - 1;
        *(s32*)(a + 0x24) = refs;
        if (refs == 0) TextureRelease(a);
    }
    *(u8**)lbl_805FB250 = 0;

    u8* b = *(u8**)lbl_805FB254;
    if (b != 0) {
        s32 refs = *(s32*)(b + 0x24) - 1;
        *(s32*)(b + 0x24) = refs;
        if (refs == 0) TextureRelease(b);
    }
    *(u8**)lbl_805FB254 = 0;
}

}  // extern "C"
