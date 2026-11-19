// Missing functions batch 27: operating table (lightning manager) part/option callbacks, inventory
// part text/value queries, LoadDefaultLevel, a case-folding CRC, and the waypoint interpolation tasks.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "game/part.h"

struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)
struct StringTable;

// Callback stored in gScriptFuncs[i].fn used by the interpolation tasks: (value, user data, task).
typedef void (*TaskValueFn)(void* value, void* arg, void* task);
// Easing callback: called with its own table entry and an f32, returns the eased factor.
typedef f32 (*EaseFn)(void* self, f32 x);
// Virtual call on the manager's first object.
typedef s32 (*VLoadFn)(void* self, void* slot, s32 a);

extern "C" {
#include "game/script_callbacks.h"

void   MonsterGetName(u8* m, char* out, s32 size);                       // 0x8021C968
s32    MonsterGetEnergy(u8* m);                                          // 0x8021CB78
s32    PartActionStat0(PartInstance* p, u32 actionKey);                  // 0x8021DBE8
s32    PartActionStat1(PartInstance* p, u32 actionKey);                  // 0x8021DE10
s32    PartComputeActionCost(PartInstance* p, u32 actionKey, void* monster); // 0x8021E6EC
void*  ScreenSetFindWidget(void* set, u32 hash);                         // 0x80198E34
void*  ScreenSetsCurrent(void* sets);                                    // 0x8018D1E4
u32    StringHash(const char* s);                                        // 0x801EB704
int    SysSprintf(char* buf, const char* fmt, ...);                      // 0x80142EFC
void   UISetText(void* el, const char* text, s32 a);                     // 0x8019EFC8
void*  UIWidgetFindChild(void* widget, u32 hash);                        // 0x80199AEC
void   fn_8000B7A8(u8* p, s32 a, s32 b);                                 // 0x8000B7A8
s32    fn_8001D104(u8* mgr);                                             // 0x8001D104
void   fn_8001D718(u8* mgr, s32 index, u8* slot);                        // 0x8001D718
void   fn_8004D760(u8* c, u8* monster);                                  // 0x8004D760
s32    fn_8004DAF8(void* c, s32 a, s32 b);                               // 0x8004DAF8
void   fn_80145F9C(f32* out, f32* a, f32* b, f32 t);                     // 0x80145F9C (quaternion slerp)
BOOL   fn_80198C34(void* set, void* w);                                  // 0x80198C34
void   fn_80198A8C(void* set, void* widget);                             // 0x80198A8C
void   fn_801B8DDC(void* a0, f32* from, u8* a2, s32 a3, u8* color, s32 a5, f32 f); // 0x801B8DDC
u8*    fn_801C6BCC(u8* key);                                             // 0x801C6BCC
void   fn_801C6C80(u8* out, u8* rec, u8* color, s32 a);                  // 0x801C6C80
u8*    fn_801E7424(u8* base, u16* id);                                   // 0x801E7424
BOOL   fn_801E7460(u8* base, u16* id);                                   // 0x801E7460
s32    fn_801EAFA4(u8* task, f32 t);                                     // 0x801EAFA4
u32    fn_801ED2FC(u8* a3);                                              // 0x801ED2FC
void   fn_801ED554(u8* out, u8* color);                                  // 0x801ED554
void   fn_801ED788(f32* out, u8* a1);                                    // 0x801ED788
void   fn_801EDA4C(u8* out, u8* a1);                                     // 0x801EDA4C
s32    fn_80204210(u8* e);                                               // 0x80204210
void   fn_8020ECB0(u8* p, u8* b);                                        // 0x8020ECB0

extern u8   gLevelFlags[];                 // 0x80xxxxxx (0x20-byte records, hash at +4)
extern u8   gWorldState[];                 // 0x803E6D00 (first word: pointer to the world block)
extern ScriptFunc gScriptFuncs[];          // 0x80484230
extern u32  lbl_80347E68[];                // 0x80347E68 (CRC32 table)
extern u8   lbl_803B4DD0[];                // 0x803B4DD0 (+0x38: locale block, +0xC: case-fold table)
extern u8   lbl_803C4298[];                // 0x803C4298
extern s32  lbl_803C4E28[];                // 0x803C4E28 (combat part zone-loaded flags)
extern u8   lbl_80413400[];                // 0x80413400
extern u8   lbl_8048A8B4[];                // 0x8048A8B4 (ring buffer of 0x28-byte records)
extern const char lbl_80354800[];          // 0x80354800 ("Not found")
extern const char lbl_8035480C[];          // 0x8035480C ("NOT FOUND")
extern const char lbl_8036045C[];          // 0x8036045C
extern const char lbl_805F8200[];          // 0x805F8200 (empty string)
extern u8*  lbl_805FAF78;                  // 0x805FAF78 (operating table / lightning manager)
extern s32  lbl_805FB034;                  // 0x805FB034
extern s32  lbl_805FB038;                  // 0x805FB038 (number of combat part zones)
extern u32  lbl_805FB1EC;                  // 0x805FB1EC (cached string hash)
extern u8   lbl_805FB1F0;                  // 0x805FB1F0 (hash cached flag)
extern u8   lbl_805FB710[];                // 0x805FB710 (default id record)
extern u8*  lbl_805FB8D8;                  // 0x805FB8D8 (handle table)
extern u8*  lbl_805FBBE0;                  // 0x805FBBE0 (save block pointer)
extern f32  lbl_80600028;                  // 0x80600028 (0.0)
extern f32  lbl_8060002C;                  // 0x8060002C (0.0001)
extern f32  lbl_80600030;                  // 0x80600030 (-0.0001)
extern f32  lbl_80600034;                  // 0x80600034 (1.0)
}

extern "C++" void* gScreenSets;            // 0x805FB654
extern "C++" StringTable* gStringTable;    // 0x805FB910
extern "C++" void* gStringTable2;          // 0x805FB90C

// Selected item of a lightning manager slot entry (0x1A4 bytes): the list lives at +8, the selected
// index at +0x198 and the item count at +0x1A0. Returns the item id, or -1 when the index is invalid.
static s32 EntrySelectedItem(u8* entry) {
    u8* base = entry + 8;
    s32 count = *(s32*)(base + 0x198);
    s32 idx = *(s32*)(base + 0x190);
    if (count > 0 && idx < count) return *(s32*)(base + idx * 4);
    return -1;
}

// Wraps the selected index of a manager slot entry into [0, count) after adding delta.
static void EntryStepIndex(u8* entry, s32 delta) {
    if (*(s32*)(entry + 0x1A0) > 1) {
        s32 v = *(s32*)(entry + 0x198) + delta;
        *(s32*)(entry + 0x198) = v;
        s32 count = *(s32*)(entry + 0x1A0);
        if (v >= count) {
            *(s32*)(entry + 0x198) = *(s32*)(entry + 0x198) % count;
        }
        while (*(s32*)(entry + 0x198) < 0) {
            *(s32*)(entry + 0x198) = *(s32*)(entry + 0x198) + *(s32*)(entry + 0x1A0);
        }
    }
}

// Localized string for a text hash (as inlined in the inventory text callbacks): "Not found" when
// the hash is unknown, otherwise the entry of the matching string table (empty string when out of range).
static const char* TextFromHash(u32 hash) {
    s32 id = TextIdFromHash(hash);
    if (id == -1) return lbl_80354800;
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        const char** arr = *(const char***)((u8*)gStringTable2 + 0x0);
        if (arr != NULL && ((u32)id & 0x7FFFFFFFu) < *(u32*)((u8*)gStringTable2 + 0xC)) {
            return arr[(u32)id & 0x7FFFFFFFu];
        }
        return lbl_805F8200;
    }
    const char** arr = *(const char***)((u8*)gStringTable + 0x0);
    if (arr != NULL && (u32)id < *(u32*)((u8*)gStringTable + 0xC)) return arr[id];
    if (id == -1) return lbl_8035480C;
    return lbl_805F8200;
}

// Value of action 1 (second == 0) or action 2 (second != 0) of inventory part a0, as shown in the
// part list: stat type 0 uses PartActionStat0, type 1 PartActionStat1, anything else gives 0.
static ScriptValue InventoryPartActionValue(s32 a0, s32 second) {
    if (a0 >= 0 && a0 < *(s32*)(lbl_805FBBE0 + 0x10EC)) {
        PartInstance* p = *(PartInstance**)(lbl_805FBBE0 + a0 * 4 + 0xA58);
        PartDef tmp;
        PartDef def;
        PartDbGetDef(&tmp, gGameDb, p->defIndex);
        def = tmp;
        u8* data = gGameDb->data;
        u8* act = *(u8**)(data + 0x430) + (second ? def.action2 : def.action1) * 0x14;
        s32 type = *(s32*)(act + 0x4);
        s32 statKey = *(s32*)(*(u8**)(data + 0x8) + type * 0x24 + 0x8);
        if (p != NULL) {
            PartDbGetDef(&tmp, gGameDb, p->defIndex);
            def = tmp;
            u32 key = (u32)(second ? def.action2 : def.action1);
            // The original converts the float left in f1 by the callee; the integer result is the same value.
            if (statKey == 0) return PartActionStat0(p, key);
            if (statKey == 1) return PartActionStat1(p, key);
        }
    }
    return 0;
}

extern "C" {

// 0x8001C25C
// Script callback: true when the monster has enough energy for both part actions of every part
// slot; otherwise stores the first failing slot at +0xA88 and returns false.
s32 LT_MonsterHasEnoughEnergyForPartActions(void) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL) return 0;
    s32 energy = MonsterGetEnergy(mgr + 0x940);
    *(s32*)(mgr + 0xA88) = -1;
    for (s32 i = 0;;) {
        if (i != 4) {
            PartInstance* p = *(PartInstance**)(mgr + i * 4 + 0x944);
            if (p != NULL && p->defIndex != -1) {
                PartDef tmp;
                PartDef def;
                PartDbGetDef(&tmp, gGameDb, p->defIndex);
                def = tmp;
                s32 c1 = PartComputeActionCost(p, (u32)def.action1, mgr + 0x940);
                s32 c2 = PartComputeActionCost(p, (u32)def.action2, mgr + 0x940);
                if (c1 > energy || c2 > energy) {
                    *(s32*)(mgr + 0xA88) = i;
                    return 0;
                }
            }
        }
        i++;
        if (i >= 5) return 1;
    }
}

// 0x8001C808
// Script callback: false when the currently selected part's action 1 (a0 == 0) or action 2 costs
// more energy than the monster has.
s32 LT_GetMonsterHasEnergyForCurrentPartAction(s32 a0) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL) return 0;
    s32 cur = *(s32*)(mgr + 0xA80);
    if (cur == -1) return 0;
    s32 item = EntrySelectedItem(mgr + cur * 0x1A4);
    if (item >= 0 && item < *(s32*)(lbl_805FBBE0 + 0x10EC)) {
        PartInstance* p = *(PartInstance**)(lbl_805FBBE0 + item * 4 + 0xA58);
        PartDef tmp;
        PartDef def;
        PartDbGetDef(&tmp, gGameDb, p->defIndex);
        def = tmp;
        s32 ai = (a0 == 0) ? def.action1 : def.action2;
        u32 key = *(u32*)(*(u8**)(gGameDb->data + 0x430) + ai * 0x14);
        s32 cost = PartComputeActionCost(p, key, mgr + 0x940);
        if (cost > MonsterGetEnergy(mgr + 0x940)) return 0;
    }
    return 1;
}

// 0x8001CAC0
// Script callback: loads the selected inventory monster into the operating table: builds its
// character, copies five values, points each slot's selection at the matching part and caches the name length.
void LoadInventoryMonster(void) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL || *(u8**)mgr == NULL) return;
    s32 sel = *(s32*)(mgr + 0xAD8);
    if (sel < 0) return;
    for (s32 k = 0; k < 10; k++) lbl_803C4E28[k] = 0;
    s32 loaded = 0;
    lbl_805FB038 = 10;
    lbl_805FB034 = 0;
    u8* obj = *(u8**)mgr;
    u8* slot = lbl_805FBBE0 + sel * 4;
    u8* m = *(u8**)(slot + 0xA30);
    if ((*(VLoadFn*)(*(u8**)obj + 0xF0))(obj, slot, 0) == 0) {
        loaded = 1;
        fn_8004D760(*(u8**)mgr, m);
        fn_8004DAF8(*(void**)mgr, 1, 0);
        *(s32*)(mgr + 0x92C) = *(s32*)(*(u8**)mgr + 0x89C);
        *(s32*)(mgr + 0x930) = *(s32*)(*(u8**)mgr + 0x8A0);
        *(s32*)(mgr + 0x934) = *(s32*)(*(u8**)mgr + 0x8A4);
        *(s32*)(mgr + 0x938) = *(s32*)(*(u8**)mgr + 0x8A8);
        *(s32*)(mgr + 0x93C) = *(s32*)(*(u8**)mgr + 0x8AC);
    }
    fn_8020ECB0(mgr + 0x940, m);
    u8* mp = m;
    s32 i = 0;
    s32 off = 0;
    do {
        void* want = *(void**)(mp + 4);
        if (want != NULL) {
            s32 found = 0;
            s32 j = 0;
            while (j < *(s32*)(mgr + off + 0x1A0) && found == 0) {
                *(s32*)(mgr + off + 0x198) = j;
                *(s32*)(mgr + off + 0x19C) = j;
                s32 item = EntrySelectedItem(mgr + off);
                if (item >= 0 && *(void**)(lbl_805FBBE0 + item * 4 + 0xA58) == want) found = 1;
                j++;
            }
        } else {
            *(s32*)(mgr + off + 0x198) = *(s32*)(mgr + off + 0x1A0) - 1;
        }
        if (loaded == 0) {
            *(s32*)(mgr + off + 0x19C) = *(s32*)(mgr + off + 0x1A0);
            fn_8001D718(mgr, i, NULL);
        }
        i++;
        off += 0x1A4;
        mp += 4;
    } while (i < 5);
    MonsterGetName(m, (char*)(mgr + 0xAE8), 0x10);
    *(s32*)(mgr + 0xBF8) = 0;
    while (*(s8*)(mgr + *(s32*)(mgr + 0xBF8) + 0xAE8) != 0) {
        *(s32*)(mgr + 0xBF8) = *(s32*)(mgr + 0xBF8) + 1;
    }
}

// 0x8001CE84
// Script callback: steps the selected option of the current manager slot by a0 (skipping entries
// fn_8001D104 rejects) and updates the "n/count" label on the screen.
void IncrementOption(s32 a0) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL || *(u8**)mgr == NULL) return;
    s32 cur = *(s32*)(mgr + 0xA80);
    u8* entry = mgr + cur * 0x1A4;
    if (*(s32*)(entry + 0x1A0) > 1 && a0 != 0) {
        if (cur == 4) {
            s32 item = EntrySelectedItem(mgr + 4 * 0x1A4);
            if (item >= 0) {
                u8* part = *(u8**)(lbl_805FBBE0 + item * 4 + 0xA58);
                *(u8*)(part + 0x1F) = 0;
            }
        }
        EntryStepIndex(lbl_805FAF78 + *(s32*)(lbl_805FAF78 + 0xA80) * 0x1A4, a0);
        s32 dir = (a0 < 0) ? -1 : 1;
        while (fn_8001D104(lbl_805FAF78) == 0) {
            EntryStepIndex(lbl_805FAF78 + *(s32*)(lbl_805FAF78 + 0xA80) * 0x1A4, dir);
        }
    }
    void* sets = ScreenSetsCurrent(gScreenSets);
    if (sets == NULL) return;
    void* widget = ScreenSetFindWidget(sets, 0xAD57FC00u);
    if (widget == NULL) return;
    void* label = UIWidgetFindChild(widget, *(u32*)(lbl_805FAF78 + *(s32*)(lbl_805FAF78 + 0xA80) * 4 + 0xC30));
    if (label == NULL) return;
    s32 text[4] = {0, 0, 0, 0};
    u8* e = lbl_805FAF78 + *(s32*)(lbl_805FAF78 + 0xA80) * 0x1A4;
    s32 sel = *(s32*)(e + 0x198);
    s32 last = *(s32*)(e + 0x1A0) - 1;
    SysSprintf((char*)text, "%d/%d", (sel + 1) % (last + 1), last);
    UISetText(label, (const char*)text, 1);
}

// 0x800EF538
// Script callback: true when the lightning object a0 has a queued entry with the given key a1 that
// is still pending (its state at +0x40 is 0). Walks the chain of records starting at a0's slot.
u32 /*0x06*/ IsLightningQueued(void* a0, u32 a1) {
    u8* world = *(u8**)gWorldState;
    u32 slot = 0;
    if (a0 != NULL) slot = *(u16*)(*(u8**)((u8*)a0 + 0x4) + 0x32);
    u8* entry = *(u8**)(world + 0x1C8) + slot * 0xC;
    u16 h = *(u16*)(entry + 0x2);
    u32 hash = lbl_805FB1EC;
    u8* rec = fn_801E7424(lbl_805FB8D8, &h);
    while (rec != NULL) {
        if ((s8)lbl_805FB1F0 == 0) {
            hash = StringHash(lbl_8036045C);
            lbl_805FB1EC = hash;
            lbl_805FB1F0 = 1;
        }
        if (*(u32*)(rec + 0x64) == hash && a1 == *(u32*)(rec + 0x4C)) break;
        u16 next = *(u16*)(rec + 0x7A);
        u8* table = lbl_805FB8D8;
        h = next;
        if (fn_801E7460(table, &h)) {
            rec = table + (((u32)next & 0xFF) << 7);
        } else {
            rec = NULL;
        }
    }
    u32 r = 0;
    if (rec != NULL && *(s32*)(rec + 0x40) == 0) r = 1;
    return r;
}

// 0x800F2138
// Script callback: localized name of the enhancement on inventory part a0 (default string when none).
ScriptValue InventoryGetPartEnhancement(s32 a0) {
    if (a0 >= 0 && a0 < *(s32*)(lbl_805FBBE0 + 0x10EC)) {
        PartInstance* p = *(PartInstance**)(lbl_805FBBE0 + a0 * 4 + 0xA58);
        if (p != NULL && p->enhancement != -1) {
            u8* e = *(u8**)(gGameDb->data + 0x3610) + p->enhancement * 0x24;
            return (ScriptValue)TextFromHash(*(u32*)(e + 0x4));
        }
    }
    u8* arr = *(u8**)gStringTable;
    if (arr != NULL && *(u32*)((u8*)gStringTable + 0xC) > 0x471u) return *(ScriptValue*)(arr + 0x11C4);
    return (ScriptValue)lbl_805F8200;
}

// 0x800F22B4
// Script callback: localized name of the defect on inventory part a0 (default string when none).
ScriptValue InventoryGetPartDefect(s32 a0) {
    if (a0 >= 0 && a0 < *(s32*)(lbl_805FBBE0 + 0x10EC)) {
        PartInstance* p = *(PartInstance**)(lbl_805FBBE0 + a0 * 4 + 0xA58);
        if (p != NULL && p->defect != -1) {
            u8* e = *(u8**)(gGameDb->data + 0x31E8) + (p->defect << 5);
            return (ScriptValue)TextFromHash(*(u32*)(e + 0x4));
        }
    }
    u8* arr = *(u8**)gStringTable;
    if (arr != NULL && *(u32*)((u8*)gStringTable + 0xC) > 0x470u) return *(ScriptValue*)(arr + 0x11C0);
    return (ScriptValue)lbl_805F8200;
}

// 0x800F2D14
// Script callback: value of part action 1 of inventory part a0.
ScriptValue InventoryPartAction1GetValue(s32 a0) {
    return InventoryPartActionValue(a0, 0);
}

// 0x800F3798
// Script callback: value of part action 2 of inventory part a0.
ScriptValue InventoryPartAction2GetValue(s32 a0) {
    return InventoryPartActionValue(a0, 1);
}

// 0x801073DC
// Script callback: switches to the default level (forest, or the quit entry) unless it is already
// loaded; stops rumble, hides the open screen widget first.
void LoadDefaultLevel(void) {
    u32 hash = StringHash("l00_forest01");
    u8 level = 0;
    s32 found = 0;
    if (hash == StringHash("QUIT")) {
        level = 0x43;
        found = 1;
    } else if (hash != 0) {
        u8 i = 0;
        for (s32 n = 0; n < 0x44; n++) {
            if (hash == *(u32*)(gLevelFlags + ((i << 5) & 0x1FE0) + 4)) {
                level = i;
                found = 1;
                break;
            }
            i++;
        }
    }
    if (found != 0 && level != *(u8*)(lbl_803C4298 + 0x1C)) {
        for (s32 c = 0; c < 4; c++) StopRumble(c);
        void* sets = ScreenSetsCurrent(gScreenSets);
        if (sets != NULL) {
            void* w = ScreenSetFindWidget(sets, 0xBEF77BAEu);
            if (fn_80198C34(sets, w) != 0) fn_80198A8C(sets, w);
        }
        fn_8000B7A8(lbl_803C4298, level, 0);
    }
}

// 0x80143A5C
// Case-folded CRC32 of a string (up to 0x3FF chars): maps bytes through the locale case table, then
// runs the standard table CRC. Returns 0 for NULL or too long strings.
u32 fn_80143A5C(const char* s) {
    char buf[0x400];
    if (s == NULL) return 0;
    u32 len = (u32)strlen(s);
    if (len >= 0x400) return 0;
    u8* locale = *(u8**)(lbl_803B4DD0 + 0x38);
    for (u32 i = 0; i < len; i++) {
        s8 c = (s8)s[i];
        if (c >= 0) c = (s8)(*(u8**)(locale + 0xC))[c];
        buf[i] = c;
    }
    buf[len] = 0;
    s32 n = (s32)strlen(buf);
    u32 crc = 0xFFFFFFFFu;
    for (s32 i = 0; i < n; i++) {
        crc = (crc >> 8) ^ lbl_80347E68[(crc ^ (u8)buf[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFFu;
}

// Last record of the ring buffer q (head at +0x10, tail at +0x14, capacity at +0x18, records of
// 0x28 bytes at *(q + 0x1C)), or NULL when the tail is not a valid slot.
static u8* RingTail(u8* q) {
    s32 tail = *(s32*)(q + 0x14);
    s32 cap;
    if (tail < 0 || tail >= (cap = *(s32*)(q + 0x18))) return NULL;
    s32 head = *(s32*)(q + 0x10);
    s32 x = tail;
    s32 hi = tail;
    if (tail < head) {
        x = tail + cap;
        hi = tail + cap;
    }
    if (head <= x && x <= hi) return *(u8**)(q + 0x1C) + tail * 0x28;
    return NULL;
}

// 0x801E4918
// Drops finished records from the tail of the ring buffer q, then, if the new tail record is
// done (fn_80204210 == 1), clears it and pops one more entry off the global ring buffer.
// Only q is read; callers pass two more arguments that the asm ignores.
void fn_801E4918(u8* q, s32 /*unused*/ a, f32 /*unused*/ t) {
    s32 head = *(s32*)(q + 0x10);
    if (head < 0 || *(s32*)(q + 0x14) < 0) return;
    u8* rec = RingTail(q);
    while (rec != NULL && *(s32*)(rec + 0x20) != 0) {
        s32 tail = *(s32*)(q + 0x14);
        if (tail == *(s32*)(q + 0x10)) {
            *(s32*)(q + 0x14) = -1;
            *(s32*)(q + 0x10) = -1;
        } else {
            s32 v = tail - 1;
            if (v < 0) v = *(s32*)(q + 0x18) - 1;
            *(s32*)(q + 0x14) = v;
        }
        rec = RingTail(q);
    }
    if (rec != NULL && fn_80204210(rec) == 1) {
        *(s32*)(rec + 0x4) = 0;
        if (*(s32*)(lbl_8048A8B4 + 0x14) == *(s32*)(lbl_8048A8B4 + 0x10)) {
            *(s32*)(lbl_8048A8B4 + 0x14) = -1;
            *(s32*)(lbl_8048A8B4 + 0x10) = -1;
            return;
        }
        s32 v = *(s32*)(lbl_8048A8B4 + 0x14) - 1;
        if (v < 0) v = *(s32*)(lbl_8048A8B4 + 0x18) - 1;
        *(s32*)(lbl_8048A8B4 + 0x14) = v;
    }
}

// 0x801B916C
// Script callback InterpolateTo: reads the target position a1, picks the color record (default,
// or the one of a1 when it is of kind 2) and starts the interpolation of a0 over a2; a3 == 0 also
// passes the extra record built by fn_801EDA4C.
void InterpolateTo(void* a0, u32 /*0x17*/ a1, f32 a2, u32 /*0x06*/ a3) {
    f32 pos[3];
    u8 extra[8];
    u8 tmp[4];
    u8 col[4];
    u8* v = (u8*)(uintptr_t)a1;
    fn_801ED788(pos, v);
    u8 c0 = lbl_805FB710[0];
    u8 c1 = lbl_805FB710[1];
    u16 c2 = *(u16*)(lbl_805FB710 + 2);
    s32 id = -1;
    if (fn_801ED2FC(v) == 2) {
        fn_801ED554(tmp, v);
        c2 = *(u16*)(tmp + 2);
        c1 = tmp[1];
        c0 = tmp[0];
        id = *(s32*)(lbl_80413400 + 8);
    }
    if (a3 == 0) {
        fn_801EDA4C(extra, v);
        *(u16*)(col + 2) = c2;
        col[1] = c1;
        col[0] = c0;
        fn_801B8DDC(a0, pos, extra, 1, col, id, a2);
        return;
    }
    *(u16*)(col + 2) = c2;
    col[1] = c1;
    col[0] = c0;
    fn_801B8DDC(a0, pos, NULL, 1, col, id, a2);
}

// 0x801EA134
// Vector3 interpolation task update: linear blend between the start (+4) and end (+0x10) vectors
// by the (optionally eased) progress; returns 3 when finished, 1 or 2 otherwise (flag +0x3C bit 1).
s32 InterpolateVector3Task_Update_LERP(u8* task, void* arg) {
    u8 mode = task[0x7F];
    f32 t = *(f32*)task * *(f32*)(task + 0x70);
    if (mode != 0x20) return 2;
    f32 end[3];
    end[0] = *(f32*)(task + 0x10);
    end[1] = *(f32*)(task + 0x14);
    end[2] = *(f32*)(task + 0x18);
    if (t >= lbl_80600034 || mode == 0x40) {
        f32 out[3];
        out[0] = end[0];
        out[1] = end[1];
        out[2] = end[2];
        ((TaskValueFn)gScriptFuncs[*(s32*)(task + 0x38)].fn)(out, arg, task);
        return 3;
    }
    s32 ease = *(s32*)(task + 0x40);
    if (ease != -1) {
        t = ((EaseFn)gScriptFuncs[ease].fn)(&gScriptFuncs[ease], lbl_80600028);
    }
    f32 s = t;
    f32 sx = *(f32*)(task + 0x4);
    f32 sy = *(f32*)(task + 0x8);
    f32 sz = *(f32*)(task + 0xC);
    f32 dx = end[0] - sx;
    f32 dy = end[1] - sy;
    f32 dz = end[2] - sz;
    f32 out[3];
    out[0] = sx + dx * s;
    out[1] = sy + dy * s;
    out[2] = sz + dz * s;
    ((TaskValueFn)gScriptFuncs[*(s32*)(task + 0x38)].fn)(out, arg, task);
    if (*(u32*)(task + 0x3C) & 2) return 1;
    return 2;
}

// 0x801EA7F8
// Quaternion interpolation task update: slerps from the start (+0) to the end (+0x10) quaternion
// by the (optionally eased) progress; returns 3 when finished, 1 or 2 otherwise (flag +0x4C bit 1).
s32 InterpolateQuaternionTask_Update_SLERP(u8* task, void* arg) {
    u8 mode = task[0x7F];
    f32 t = *(f32*)(task + 0x70) * *(f32*)(task + 0x40);
    if (mode != 0x20 && mode != 8) return 2;
    f32 end[4];
    end[0] = *(f32*)(task + 0x10);
    end[1] = *(f32*)(task + 0x14);
    end[2] = *(f32*)(task + 0x18);
    end[3] = *(f32*)(task + 0x1C);
    if (t >= lbl_80600034 || task[0x7F] == 0x40) {
        f32 out[4];
        out[0] = end[0];
        out[1] = end[1];
        out[2] = end[2];
        out[3] = end[3];
        ((TaskValueFn)gScriptFuncs[*(s32*)(task + 0x48)].fn)(out, arg, task);
        return 3;
    }
    s32 ease = *(s32*)(task + 0x50);
    if (ease != -1) {
        t = ((EaseFn)gScriptFuncs[ease].fn)(&gScriptFuncs[ease], lbl_80600028);
    }
    f32 start[4];
    start[0] = *(f32*)(task + 0x0);
    start[1] = *(f32*)(task + 0x4);
    start[2] = *(f32*)(task + 0x8);
    start[3] = *(f32*)(task + 0xC);
    f32 res[4];
    fn_80145F9C(res, start, end, t);
    f32 out[4];
    out[0] = res[0];
    out[1] = res[1];
    out[2] = res[2];
    out[3] = res[3];
    ((TaskValueFn)gScriptFuncs[*(s32*)(task + 0x48)].fn)(out, arg, task);
    if (*(u32*)(task + 0x4C) & 2) return 1;
    return 2;
}

// 0x801EB330
// Waypoint task callback: given the next waypoint position v, state (record being built) and task.
// When the task progress reaches 1/(first float of the task) the waypoint is reached: the three
// color records of the state shift down, the new one is resolved, the state's next-waypoint flag
// is raised if fn_801EAFA4 fails, and the state's callback is called; otherwise the callback
// of the task is called with the position.
void InterpolateToNextWaypointTask_SetVector3Callback(f32* v, u8* state, u8* task) {
    f32 x = *(f32*)task;
    f32 inv;
    if (x > lbl_80600030 && x < lbl_8060002C) {
        inv = lbl_80600028;
    } else {
        inv = lbl_80600034 / x;
    }
    f32 progress = *(f32*)(task + 0x70);
    if (progress >= inv) {
        u16 h1 = *(u16*)(state + 0xA);
        u8 b1 = state[0x9];
        u8 b0 = state[0x8];
        u16 h2 = *(u16*)(state + 0xE);
        u8 b3 = state[0xD];
        u8 b2 = state[0xC];
        u16 h3 = *(u16*)(state + 0x12);
        u8 b5 = state[0x11];
        u8 b4 = state[0x10];
        *(u16*)(state + 0x6) = h1;
        state[0x5] = b1;
        state[0x4] = b0;
        *(u16*)(state + 0xA) = h2;
        state[0x9] = b3;
        state[0x8] = b2;
        *(u16*)(state + 0xE) = h3;
        state[0xD] = b5;
        state[0xC] = b4;
        u8 key[4];
        *(u16*)(key + 2) = h3;
        key[1] = state[0xD];
        key[0] = state[0xC];
        u8* rec = fn_801C6BCC(key);
        if (rec != NULL) {
            u8 col[4];
            u8 out[4];
            *(u16*)(col + 2) = *(u16*)(state + 0xE);
            col[1] = state[0xD];
            col[0] = state[0xC];
            fn_801C6C80(out, rec, col, 0);
            *(u16*)(state + 0x12) = *(u16*)(out + 2);
            state[0x11] = out[1];
            state[0x10] = out[0];
        }
        if (fn_801EAFA4(state, progress - inv) == 0) {
            *(u32*)(state + 0x38) |= 4;
        }
        void* arg = *(void**)(state + 0x40);
        u8 cur[4];
        *(u16*)(cur + 2) = *(u16*)(state + 0x6);
        cur[1] = state[0x5];
        cur[0] = state[0x4];
        u8* r = fn_801C6BCC(cur);
        ((TaskValueFn)gScriptFuncs[*(s32*)(state + 0x48)].fn)(r, arg, state);
        return;
    }
    f32 pos[3];
    pos[0] = v[0];
    pos[1] = v[1];
    pos[2] = v[2];
    ((TaskValueFn)gScriptFuncs[*(s32*)(state + 0x1C)].fn)(pos, *(void**)(state + 0x40), task);
}

}  // extern "C"
