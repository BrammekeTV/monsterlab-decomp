// Decompiled leftovers: battle action choice, pickup/object helpers, UI and zone helpers.
#include "types.h"
#include <string.h>

#include "game/part.h"
#include "game/character.h"

extern "C++" s32 gSelectedPart;        // 0x805F8650
extern "C++" s32 gHighlightedAction;   // 0x805F8654
extern u8* gBattle;                    // 0x805FBBB8
extern "C++" void* gScreenSets;        // 0x805FB654
extern void* gObjDb;                   // 0x805FB180
extern u8 gWorldState[];               // 0x803E6D00
extern u8 lbl_8036D1BC[];              // 0x8036D1BC (UI page vtable)
extern u8 lbl_8036D230[];              // 0x8036D230 (UI page vtable)
extern u8 lbl_8036D348[];              // 0x8036D348 (UI page vtable)
extern const char lbl_80354800[];      // 0x80354800 (string)
extern const char lbl_8035480C[];      // 0x8035480C (string)
extern const char lbl_805F8200[];      // 0x805F8200 (empty string)
struct StringTable {
    const char** strings;
    u8           pad[8];
    u32          count;     // 0x0C
};
extern "C++" StringTable* gStringTable;    // 0x805FB910
extern "C++" StringTable* gStringTable2;   // 0x805FB90C
extern u8 lbl_8047C640[];              // 0x8047C640 (camera)
extern u8 lbl_805FB2C0;                // 0x805FB2C0
extern u8* lbl_805FB75C;               // 0x805FB75C (camera)
extern u8 lbl_803C4298[];              // 0x803C4298 (+4: camera object with vtable, +8: object with vtable)
extern u8 lbl_8047C7D8[];              // 0x8047C7D8
extern u8 lbl_805FB1A1;                // 0x805FB1A1
extern u8* lbl_805FB2E8;               // 0x805FB2E8 (marker state record)
extern "C++" void* gEventMgr;          // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;    // 0x805FB94C
extern u8 gGame[];                     // 0x80xxxxxx
extern u8 gLevelFlags[];               // 0x80369D60: 0x20 bytes per level, name pointer at +0
extern char lbl_80384390[];            // 0x80384390 (asset path buffer, initially "RW_ASSETS_nolevel")
extern const char lbl_805F8FE8[];      // 0x805F8FE8
extern "C++" void* lbl_805FB5A8;       // 0x805FB5A8 (handle passed to fn_80169C6C)
extern u8* lbl_805FB188;               // 0x805FB188 (spawn record table)
extern u8* lbl_805FBBE0;               // 0x805FBBE0 (save block pointer)
extern void* lbl_805FB1C8;             // 0x805FB1C8 (state record pointer)

extern "C++" void WorldStateInit(u8* w, s32 level);   // 0x80127680 (world_alloc.cpp)
extern "C++" void* SpawnHelperAttached(void* a, void* owner, void* b);   // 0x8000C508 (game_spawn.cpp)

extern "C" {
void ChoiceInit(u8* c);   // 0x8020F36C
void fn_800C69DC(u8* obj);                 // 0x800C69DC
void fn_800C6A28(u8* obj);                 // 0x800C6A28
void fn_800C9500(u8* p, u8* item);         // 0x800C9500
s32 fn_800D4730(void* db, s32 id);         // 0x800D4730
u8* ObjDbLookup(void* db, s32 id);         // 0x800D39BC
void fn_80116BE8(u8* p);                   // 0x80116BE8 (base ctor, sets vtable)
void fn_80116CA8(u8* p);                   // 0x80116CA8
void fn_80116E54(u8* p, s32 a, s32 b);     // 0x80116E54
void fn_80116EF0(u8* p, s32 a);            // 0x80116EF0
void PartDbGetDef(PartDef* out, GameDb* db, s32 index);   // 0x80216838
void RendClear(int flags, int a, int b, f32 depth);       // 0x801534F0
void fn_8015B7D4(void);                    // 0x8015B7D4
void fn_8016FB20(void* cam);               // 0x8016FB20
void fn_80170044(void* cam);               // 0x80170044
void fn_8012112C(void);                    // 0x8012112C
void fn_800526D0(u8* out);                 // 0x800526D0 (leaf, takes an out matrix)
void fn_800557B4(u8* out);                 // 0x800557B4
void fn_8005535C(u8* dst, const u8* src);  // 0x8005535C
void fn_80121490(u8* m, f32 a, f32 b, f32 c);   // 0x80121490 (blr)
void fn_80155724(u8* m);                   // 0x80155724
u8* fn_80044634(s32 n);                    // 0x80044634
void fn_800446BC(void* cam, s32 v);        // 0x800446BC
void fn_80121434(u8* v, f32 a, f32 b);     // 0x80121434: texture coords
void fn_80121440(u8* v, s32 r, s32 g, s32 b, s32 a);   // 0x80121440: packs the vertex colour
void fn_8012145C(u8* v, f32 x, f32 y, f32 z);          // 0x8012145C: position
s32 fn_80151644(s32 a, s32 b, u8* quad);   // 0x80151644
s32 WorldGetPhase(void* world);            // 0x800D83A8
s32 fn_8000B888(void);                     // 0x8000B888
BOOL fn_800E6BD4(void);                    // 0x800E6BD4
u8* fn_8019BF64(u8* container, u32 hash);  // 0x8019BF64
void fn_80125970(u8* self, f32 dt);        // 0x80125970
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void* WorldFindLocator(void* world, u32 hash);   // 0x8012AA50
void* fn_800E60BC(void);                   // 0x800E60BC
void ScopedLockCtor(u8* l);                // 0x801F5384
void ScopedLockDtor(u8* l, s32 flags);     // 0x801F53B0
void fn_80169C6C(void* a0, s32 a1);        // 0x80169C6C
void fn_801294CC(u8* w, s32 level, const char* dir, const char* name);   // 0x801294CC
u8* GetGlobalSettings(void);               // 0x801D2BF8
s32 fn_801D3A60(s32 zone);                 // 0x801D3A60
void fn_801384B0(u8* world, s32 slot, s32 a);   // 0x801384B0
u8* fn_800E5684(u8* p);                     // 0x800E5684
u8* fn_800E4D7C(u32 a0);                   // 0x800E4D7C
u8* ObjRefDtor(u8* ref, s32 flags);        // 0x800D50D0
void* ScreenSetsCurrent(void* sets);       // 0x8018D1E4
void* fn_80198E68(void* set, u32 hash);    // 0x80198E68
u8* fn_8022E2F0(u8* save);                 // 0x8022E2F0
u8* fn_8011ED38(s32 a);                    // 0x8011ED38
s32 fn_80109A0C(s32 id);                   // 0x80109A0C
s32 TextIdFromHash(u32 hash);              // 0x80120D34: -1 if unknown
s32 fn_80222994(u8* s);                    // 0x80222994
u8* fn_80222704(u8* s, s32 a, s32 b);      // 0x80222704
void UIImage_SetTexture(u32 hash, BOOL tex);            // 0x80191B68
void UIEvent_SendToScreen(u32 event, u32 screen);
void UIText_SetString(u32 hash, const char* text);       // 0x80190E48
int SysSprintf(char* buf, const char* fmt, ...);         // 0x80142EF8
}

extern "C" {

// 0x80047230
// Builds the choice for the selected part: its first action (or the second when one is highlighted).
void fn_80047230(u8* arg0) {
    PartDef def;
    u32 key;
    u8* fighter = *(u8**)(gBattle + 0xA00);
    u8* part = ((u8**)fighter)[gSelectedPart];
    PartDbGetDef(&def, gGameDb, **(s32**)part);
    const PartAction* acts = *(const PartAction**)(gGameDb->data + 0x430);
    if (gHighlightedAction == 0) {
        key = acts[def.action1].key;
    } else {
        key = acts[def.action2].key;
    }
    ChoiceInit(arg0);
    arg0[4] = 0;
    *(s32*)(arg0 + 8) = gSelectedPart;
    *(u32*)arg0 = key;
}

// 0x800C719C
// Mode 2: set state 4 and call fn_800C6A28 on the sub-object. Mode 1: if the item at +0x8 is
// set, hand it over (as fn_800C72C4 does, with the sub-object placed 100 below its anchor).
void fn_800C719C(u8* arg0) {
    s32 mode = *(s32*)(arg0 + 0xC);
    if (mode == 2) {
        *(s32*)(arg0 + 0x10) = 4;
        fn_800C6A28(*(u8**)(arg0 + 0x4));
    } else if (mode == 1) {
        u8* item = *(u8**)(arg0 + 0x8);
        if (item != NULL) {
            *(u8**)(arg0 + 0x8) = NULL;
            fn_800C9500(item, arg0);
            *(s32*)(arg0 + 0x10) = 1;
            u8* sub = *(u8**)(arg0 + 0x4);
            f32 x = *(f32*)(sub + 0xC8) + 0.0f;
            f32 y = *(f32*)(sub + 0xCC) + -100.0f;
            f32 z = *(f32*)(sub + 0xD0) + 0.0f;
            u8* inner = *(u8**)(sub + 0x4);
            *(f32*)(inner + 0x0) = x;
            *(f32*)(inner + 0x4) = y;
            *(f32*)(inner + 0x8) = z;
            inner = *(u8**)(sub + 0x4);
            *(u32*)(inner + 0x20) |= 0x1000C000u;
            if (*(s32*)(arg0 + 0xC) == 2) {
                fn_800C69DC(*(u8**)(arg0 + 0x4));
            }
        }
    }
}

// Timed-entry table row: 0x28 bytes each, count at +0x320; out-of-range indexes fall back to row 0.
static inline u8* TimedEntry(u8* tbl, s32 i) {
    s32 count = *(s32*)(tbl + 0x320);
    s32 j = (i < 0) ? 0 : (i > count ? count : i);
    return (i != j) ? tbl : tbl + i * 0x28;
}

// 0x800D76FC
// Spawns the attached helper for a timed entry: resolves its object id (+0x4) from the source id
// (+0x8) when set, then spawns at a point 25 units above the object's path position and stores
// the result at +0x24.
void fn_800D76FC(u8* tbl, s32 idx) {
    if (lbl_805FB188 == NULL) return;
    if (*(s32*)(TimedEntry(tbl, idx) + 0x8) != 0) {
        u8* dst = TimedEntry(tbl, idx);
        *(s32*)(dst + 0x4) = fn_800D4730(gObjDb, *(s32*)(TimedEntry(tbl, idx) + 0x8));
    }
    void* owner = *(void**)(*(u8**)gWorldState + 0x74);
    u8* obj = ObjDbLookup(gObjDb, *(s32*)(TimedEntry(tbl, idx) + 0x4));
    u8 ref[0x404];
    memcpy(ref, obj, 0x404);
    Vec3* pos = PathGetPosition((PathFollower*)ref);
    f32 x = pos->x, y = pos->y, z = pos->z;
    f32 spawn[3];
    f32 off[3];
    f32 zTop = 25.0f + z;
    spawn[0] = x;
    spawn[1] = y;
    spawn[2] = zTop;
    off[0] = x - x;
    off[1] = y - y;
    off[2] = z - zTop;
    u8* e = TimedEntry(tbl, idx);
    *(void**)(e + 0x24) = SpawnHelperAttached(spawn, owner, off);
    ObjRefDtor(ref, -1);
}

// 0x800E6684
// Per-frame update of the exploration HUD element: refreshes the icon and "%d/%d" counter, plays
// the show/hide events and picks the next icon from the game database.
void fn_800E6684(u8* arg0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* el = (u8*)fn_80198E68(set, 0xC8E7B245u);
    if (el == NULL) return;
    u8* sv = fn_8022E2F0(lbl_805FBBE0);
    if (arg0[0x1C] != 0 && *(s32*)(arg0 + 0x8) != 0) {
        UIImage_SetTexture(0xC796E203u, *(s32*)(arg0 + 0x8));
        UIEvent_SendToScreen(0x61E5B532u, 0xDB1E4D0Cu);   // "exploration_screen"
        arg0[0x1C] = 0;
        (*(void (**)(void*, s32))(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        char buf[0x80];
        u8* sv2 = fn_8022E2F0(lbl_805FBBE0);
        if (sv2 != NULL) {
            SysSprintf(buf, "%d/%d", *(s32*)(sv2 + 0xC), *(s32*)(sv2 + 0x10));
        } else {
            s32 v = *(s32*)((u8*)lbl_805FB1C8 + 0x20);
            SysSprintf(buf, "%d/%d", v, v);
        }
        UIText_SetString(0xC216F8BCu, buf);
        if (sv != NULL) *(s32*)(arg0 + 0x20) = *(s32*)(sv + 0x10);
    }
    if (sv != NULL) {
        if (*(s32*)(sv + 0x10) > 0) {
            if (*(u32*)(el + 0x64) & 2) {
                if (*(s32*)(lbl_805FBBE0 + 0xA20) != 0) {
                    *(s32*)(lbl_805FBBE0 + 0xA20) = 0;
                    UIEvent_SendToScreen(0x8159F179u, 0xDB1E4D0Cu);
                }
            } else {
                arg0[0x1C] = 1;
            }
            s32 k = *(s32*)(sv + 0x4);
            if (k != 0 && arg0[0x1C] != 0 && *(s32*)(arg0 + 0xC) == 0) {
                u8* rec = *(u8**)(gGameDb->data + 0x8930) + k * 8;
                u8 b = (u8)(*(u32*)(rec + 4) >> 24);
                *(u8**)(arg0 + 0xC) = fn_8011ED38((s8)b);
                if (fn_80109A0C(*(s32*)(arg0 + 0xC)) != 0) {
                    *(s32*)(arg0 + 0x8) = *(s32*)(arg0 + 0xC);
                    *(s32*)(arg0 + 0xC) = 0;
                }
            }
        }
    } else if ((*(u32*)(el + 0x64) & 2) && arg0[0x1D] == 0) {
        UIEvent_SendToScreen(0x8159F179u, 0xDB1E4D0Cu);
        arg0[0x1D] = 1;
    }
}

// 0x8011A778
// Constructor of a UI page: 100 rows of 0x60 bytes at +0x1C (all zero except -1 at +0x3C, +0x40
// and +0x58), then counters, a sub-object at +0x2CB4 and four 7x16-word tables cleared.
u8* fn_8011A778(u8* p) {
    fn_80116BE8(p);
    *(void**)p = (void*)lbl_8036D1BC;
    for (u8* row = p + 0x1C; row < p + 0x259C; row += 0x60) {
        for (s32 i = 0; i < 0x60; i += 4) *(s32*)(row + i) = 0;
        *(s32*)(row + 0x3C) = -1;
        *(s32*)(row + 0x40) = -1;
        *(s32*)(row + 0x58) = -1;
    }
    *(s32*)(p + 0x259C) = 0;
    *(s32*)(p + 0x25A0) = 0;
    *(s32*)(p + 0x25A4) = 0;
    *(s32*)(p + 0x2CAC) = 3;
    *(s32*)(p + 0x2CB0) = 5;
    fn_80116CA8(p + 0x2CB4);
    *(s32*)(p + 0x2DE0) = 0;
    for (s32 k = 0; k < 7; k++) {
        u8* q = p + k * 0x40;
        for (s32 i = 0; i < 0x40; i += 4) {
            *(s32*)(q + 0x25A8 + i) = 0;
            *(s32*)(q + 0x2768 + i) = 0;
            *(s32*)(q + 0x2928 + i) = 0;
            *(s32*)(q + 0x2AE8 + i) = 0;
        }
    }
    return p;
}

// 0x8011C460
// Constructor of a UI page: 150 rows of 0x1C bytes at +0x1C (all zero except -1 at +0x10), then
// a word table at +0x1084..+0x12F0 cleared and a sub-object at +0x1318.
u8* fn_8011C460(u8* p) {
    fn_80116BE8(p);
    *(void**)p = (void*)lbl_8036D230;
    for (u8* row = p + 0x1C; row < p + 0x1084; row += 0x1C) {
        for (s32 i = 0; i < 0x1C; i += 4) *(s32*)(row + i) = 0;
        *(s32*)(row + 0x10) = -1;
    }
    *(s32*)(p + 0x1084) = 0;
    *(s32*)(p + 0x12EC) = 0;
    *(s32*)(p + 0x12F0) = 0;
    fn_80116CA8(p + 0x1318);
    for (u8* w = p + 0x1088; w <= p + 0x12E8; w += 4) *(s32*)w = 0;
    return p;
}

// Looks a text id up in the string tables (ids with the top bit set use the second table),
// as the inlined text lookup in the callers below does.
static const char* TextLookup(s32 id) {
    if (id == -1) return lbl_80354800;
    if ((u32)id & 0x80000000u) {
        StringTable* t2 = gStringTable2;
        if (t2 != NULL) {
            if (t2->strings != NULL && ((u32)id & 0x7FFFFFFFu) < t2->count) {
                return t2->strings[(u32)id & 0x7FFFFFFFu];
            }
            return lbl_805F8200;
        }
    }
    StringTable* t = gStringTable;
    if (t->strings != NULL && (u32)id < t->count) return t->strings[id];
    if (id == -1) return lbl_8035480C;
    return lbl_805F8200;
}

// 0x8011EDE0
// Text of the entry (0x1C bytes each, table at data + 0x8508) selected by fn_80222994(save).
const char* fn_8011EDE0(u8* arg0) {
    u8* rec = *(u8**)(gGameDb->data + 0x8508) + fn_80222994(lbl_805FBBE0) * 0x1C;
    return TextLookup(TextIdFromHash(*(u32*)(rec + 0x4)));
}

// 0x8011F1E0
// Text of the entry found by fn_80222704: counted from the front (+1) when the flag byte at
// +0x2B is set, otherwise counted from the back of the +0x2C entries. NULL when out of range.
const char* fn_8011F1E0(u8* arg0, s32 arg1) {
    u8* e = NULL;
    if (arg0[0x2B] != 0) {
        e = fn_80222704(lbl_805FBBE0, arg1 + 1, 0);
    } else {
        s32 n = *(s32*)(arg0 + 0x2C);
        if (arg1 < n) e = fn_80222704(lbl_805FBBE0, 0, n - arg1 - 1);
    }
    if (e == NULL) return NULL;
    return TextLookup(TextIdFromHash(*(u32*)e));
}

// 0x8011F410
// Constructor of a UI page: 624 rows of 0x3C bytes at +0x1C, counters, a word table at
// +0x9264 cleared and a sub-object at +0xA658.
u8* fn_8011F410(u8* p) {
    fn_80116BE8(p);
    *(void**)p = (void*)lbl_8036D348;
    for (u8* row = p + 0x1C; row < p + 0x925C; row += 0x3C) {
        for (s32 i = 0; i < 0x38; i += 4) *(s32*)(row + i) = 0;
        *(s32*)(row + 0xC) = -1;
        *(s32*)(row + 0x10) = -1;
        *(s32*)(row + 0x18) = -1;
        *(f32*)(row + 0x38) = 0.0f;
    }
    *(s32*)(p + 0x925C) = 0;
    *(s32*)(p + 0x9260) = 0;
    *(s32*)(p + 0xA64C) = -1;
    *(s32*)(p + 0xA650) = 3;
    *(s32*)(p + 0xA654) = 5;
    fn_80116CA8(p + 0xA658);
    for (u8* w = p + 0x9264; w < p + 0xA644; w += 4) *(s32*)w = 0;
    return p;
}

// 0x8011FF28
// Rebuilds the visible-row list of a UI page: rows (0x3C bytes at +0x1C, count at +0x9260) whose
// +0xC / +0x10 match the filters at +0xA650 / +0xA654 (3 and 5 mean "any") are collected into
// the pointer table at +0x9264 and their first word into the table at +0x9C54, then the sub-object
// at +0xA658 is told the page count (4 per page).
void fn_8011FF28(u8* arg0) {
    memset(arg0 + 0x9C54, 0, 0x9F0);
    *(s32*)(arg0 + 0xA644) = 0;
    u8* row = arg0 + 0x1C;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x9260); i++, row += 0x3C) {
        s32 f1 = *(s32*)(arg0 + 0xA650);
        if (*(s32*)(row + 0xC) != f1 && f1 != 3) continue;
        s32 f2 = *(s32*)(arg0 + 0xA654);
        if (*(s32*)(row + 0x10) != f2 && f2 != 5) continue;
        s32 n = *(s32*)(arg0 + 0xA644);
        ((u8**)(arg0 + 0x9264))[n] = row;
        ((s32*)(arg0 + 0x9C54))[*(s32*)(arg0 + 0xA644)] = *(s32*)row;
        *(s32*)(arg0 + 0xA644) = *(s32*)(arg0 + 0xA644) + 1;
    }
    s32 n = *(s32*)(arg0 + 0xA644);
    s32 pages = 0;
    if (n > 0) pages = (n + 3) / 4;
    fn_80116E54(arg0 + 0xA658, 3, pages);
    fn_80116EF0(arg0 + 0xA658, *(s32*)(arg0 + 0xA648));
}

// 0x80120098
// Name of the current filter at +0xA654: entry of the table at data + 0xC80 (8 bytes each,
// text hash at +4), or string 0x43F when the filter is 5 ("all").
const char* fn_80120098(u8* arg0) {
    s32 f = *(s32*)(arg0 + 0xA654);
    if (f != 5) {
        u8* rec = *(u8**)(gGameDb->data + 0xC80) + f * 8;
        return TextLookup(TextIdFromHash(*(u32*)(rec + 0x4)));
    }
    StringTable* t = gStringTable;
    if (t->strings != NULL && t->count > 0x43F) return t->strings[0x43F];
    return lbl_805F8200;
}

// 0x801207DC
// Selects entry a1 of the table at data + 0x3E60 (12 bytes each): stores it at +0xA64C and its
// word +8 at +0xA650, then finds the row of the table at data + 0x6C18 (0x24 bytes, 156 rows)
// whose +0xC equals a1 and stores that part's slot at +0xA654.
void fn_801207DC(u8* arg0, s32 a1) {
    *(s32*)(arg0 + 0xA64C) = a1;
    *(s32*)(arg0 + 0xA650) = *(s32*)(*(u8**)(gGameDb->data + 0x3E60) + a1 * 0xC + 0x8);
    for (s32 i = 0; i < 0x9C; i++) {
        u8* row = *(u8**)(gGameDb->data + 0x6C18) + i * 0x24;
        if (*(s32*)(row + 0xC) == a1) {
            PartDef def;
            PartDbGetDef(&def, gGameDb, *(s32*)(row + 0x20));
            *(s32*)(arg0 + 0xA654) = def.slot;
        }
    }
}

// 0x80121254
// Draws the fullscreen quad of the page when its flag word is set: clears, sets up the camera
// and a textured 4-vertex quad (0x18 bytes per vertex, 5 units deep) and renders it.
void fn_80121254(u8* arg0) {
    if (*(s32*)arg0 == 0) return;
    if (lbl_805FB2C0 != 0) {
        fn_8015B7D4();
        lbl_805FB2C0 = 0;
    }
    RendClear(5, 0, 0, 0.0f);
    fn_8016FB20(lbl_805FB75C);
    fn_8012112C();
    u8 m1[0x30];
    u8 m2[0x30];
    fn_800526D0(m1);
    fn_800557B4(m2);
    fn_8005535C(m1, m2);
    fn_80121490(m1, 0.0f, 0.0f, 0.0f);
    fn_80155724(m1);
    u8* q = fn_80044634(0x60);
    fn_80121434(q, 0.0f, 1.0f);
    fn_80121440(q, 0xFF, 0xFF, 0xFF, 0xFF);
    fn_8012145C(q, -0.6666667f, -0.5f, 5.0f);
    fn_80121434(q + 0x18, 1.0f, 1.0f);
    fn_80121440(q + 0x18, 0xFF, 0xFF, 0xFF, 0xFF);
    fn_8012145C(q + 0x18, 0.6666667f, -0.5f, 5.0f);
    fn_80121434(q + 0x30, 0.0f, 0.0f);
    fn_80121440(q + 0x30, 0xFF, 0xFF, 0xFF, 0xFF);
    fn_8012145C(q + 0x30, -0.6666667f, 0.5f, 5.0f);
    fn_80121434(q + 0x48, 1.0f, 0.0f);
    fn_80121440(q + 0x48, 0xFF, 0xFF, 0xFF, 0xFF);
    fn_8012145C(q + 0x48, 0.6666667f, 0.5f, 5.0f);
    fn_800446BC(lbl_8047C640, *(s32*)arg0);
    fn_80151644(0x98, 4, q);
    fn_800446BC(lbl_8047C640, 0);
    fn_80170044(lbl_805FB75C);
}

// True when the four floats at +0x80..+0x8C of a UI element are all within +-0.0001 of zero.
static BOOL FourFloatsNearZero(const u8* el) {
    for (s32 i = 0; i < 4; i++) {
        f32 v = *(const f32*)(el + 0x80 + i * 4);
        if (!(v <= 0.0001f)) return 0;
        if (!(v >= -0.0001f)) return 0;
    }
    return 1;
}

// 0x8012570C
// Per-frame update of the exploration markers: while exploring (phase 1, state 5, nothing
// blocking) and both marker widgets have settled to zero, fires event 0x7E57AD29 at the pending
// marker (slot at +0x104) and marks it handled (0x40); then runs fn_80125970.
void fn_8012570C(f32 dt) {
    if (WorldGetPhase(gGame + 0xD5F0) == 1 && *(s32*)(lbl_803C4298 + 0x18) == 5 &&
        fn_8000B888() == 0 && *(s32*)(lbl_805FB2E8 + 0x14C) == 0 &&
        *(s32*)(lbl_805FB2E8 + 0x148) != 0 && fn_800E6BD4() == 0 &&
        lbl_8047C7D8[0] == 0 && lbl_805FB1A1 == 0) {
        u8* scr = *(u8**)((u8*)gScreenSets + 0x7C);
        if (scr != NULL) {
            u8* a = fn_8019BF64(scr, 0x0517EA02u);
            u8* b = fn_8019BF64(*(u8**)((u8*)gScreenSets + 0x7C), 0x9C1EBBB8u);
            if (a != NULL && b != NULL && FourFloatsNearZero(a) && FourFloatsNearZero(b)) {
                u8* st = lbl_805FB2E8;
                if (st != NULL) {
                    s32 slot = *(s32*)(st + 0x104);
                    if (slot < 0x40) {
                        EventSend(gEventMgr, ((u8**)st)[slot], NULL, 0x7E57AD29u, 0, 0, 1, 1);
                        *(s32*)(st + 0x104) = 0x40;
                    }
                }
            }
        }
        fn_80125970(lbl_805FB2E8, dt);
    }
}

// 0x80125F68
// Resets the marker state record: counters, the 8 x 0x20-byte slot area at +0 and the locator
// pairs at +0x118..+0x13C.
void fn_80125F68(void) {
    *(s32*)(lbl_805FB2E8 + 0x148) = 0;
    *(s32*)(lbl_805FB2E8 + 0x110) = 0;
    *(s32*)(lbl_805FB2E8 + 0x114) = 0;
    *(s32*)(lbl_805FB2E8 + 0x108) = 0;
    *(s32*)(lbl_805FB2E8 + 0x100) = 0;
    *(s32*)(lbl_805FB2E8 + 0x104) = 0x40;
    for (s32 i = 0; i < 0x100; i += 4) *(s32*)(lbl_805FB2E8 + i) = 0;
    for (s32 i = 0x118; i <= 0x13C; i += 4) *(s32*)(lbl_805FB2E8 + i) = 0;
}

// 0x801260C8
// Rebuilds the marker state on entering exploration: finds the two locators (hashes 0x834C7963,
// 0xAAE1A370), repositions the camera/player objects on the first, and collects the active
// world objects of both zones (flag 0x10, or 0x40 with a nonzero field at +0x80) into the list at +0.
void fn_801260C8(void) {
    fn_80125F68();
    if (WorldGetPhase(gGame + 0xD5F0) != 1) return;
    BOOL found = 0;
    u8* st = lbl_805FB2E8;
    *(void**)(st + *(s32*)(st + 0x114) * 8 + 0x118) = WorldFindLocator(gWorldState, 0x834C7963u);
    st = lbl_805FB2E8;
    *(void**)(st + *(s32*)(st + 0x114) * 8 + 0x11C) = WorldFindLocator(gWorldState, 0xAAE1A370u);
    s32 n = *(s32*)(lbl_805FB2E8 + 0x114);
    u8* rec = lbl_805FB2E8 + n * 8;
    if (*(void**)(rec + 0x118) != NULL && *(void**)(rec + 0x11C) != NULL) {
        found = 1;
        *(s32*)(lbl_805FB2E8 + 0x114) = n + 1;
    }
    if (found) {
        u8* cam = *(u8**)(lbl_803C4298 + 0x4);
        (*(void (**)(void*, s32))(*(u8**)cam + 0x50))(cam, 0);
        u8* obj = (u8*)fn_800E60BC();
        if (obj != NULL) {
            obj[0x54] = 1;
            u8* other = fn_800E4D7C((u32)(uintptr_t)fn_800E5684(obj))  /* lint:ok r3 passthrough */;
            if (other != NULL) other[0x54] = 0;
            cam = *(u8**)(lbl_803C4298 + 0x4);
            (*(void (**)(void*))(*(u8**)cam + 0x10))(cam);
        }
        u8* loc = *(u8**)(lbl_805FB2E8 + *(s32*)(lbl_805FB2E8 + 0x110) * 8 + 0x118);
        cam = *(u8**)(lbl_803C4298 + 0x4);
        (*(void (**)(void*, void*))(*(u8**)cam + 0x1C))(cam, loc + 0x8);
        u8* o2 = *(u8**)(lbl_803C4298 + 0x8);
        (*(void (**)(void*, f32))(*(u8**)o2 + 0x98))(o2, 0.0f);
        lbl_805FB1A1 = 0;
        o2 = *(u8**)(lbl_803C4298 + 0x8);
        (*(void (**)(void*, f32))(*(u8**)o2 + 0x30))(o2, 0.0f);
    }
    for (s32 z = 0; z < 2; z++) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize;
        s32 count = *(s32*)(zone + 0x94) + *(s32*)(zone + 0x98);
        for (s32 k = 0; k < count; k++) {
            zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize;
            s32 first = *(s32*)(zone + 0x94);
            u8* v;
            if (k < first) {
                v = (*(u8***)(zone + 0x120))[k];
            } else {
                v = (*(u8***)(zone + 0x12C))[k - first];
            }
            if (v == NULL) continue;
            u32 fl = *(u32*)(*(u8**)(v + 0x4) + 0x1C);
            if ((fl & 0x10) || ((fl & 0x40) && ((*(u32*)(v + 0x80) >> 27) & 0xF))) {
                s32 c = *(s32*)(lbl_805FB2E8 + 0x100);
                ((u8**)lbl_805FB2E8)[c] = v;
                *(s32*)(lbl_805FB2E8 + 0x100) = c + 1;
            }
        }
    }
}

// 0x80129334
// Loads the level: world reset with a "World::Reset waiting..." scoped lock, builds the asset
// path buffer ("RW_ASSETS_%s") and the "levels\%s\\" name, and hands over to fn_801294CC with the
// "Streams\\" directory. Always returns 1.
BOOL fn_80129334(void* world, u8 level) {
    u8* w = (u8*)world;
    u8 lock[0x8];   // scoped lock object (0x10-byte window on the stack in the original)
    char path[0x80];
    ScopedLockCtor(lock);
    const char* name = *(const char**)(gLevelFlags + (level & 0xFF) * 0x20);
    SysSprintf(lbl_80384390, "RW_ASSETS_%s", name);
    SysSprintf(path, "%s - %s", name, lbl_805F8FE8);
    if (w[0x120] == 0) WorldStateInit(w, level);
    if (name != NULL) SysSprintf(path, "levels\%s\\", name);
    if (w[0x120] == 0) fn_80169C6C(lbl_805FB5A8, 1);
    fn_801294CC(w, level, "Streams\\", name);
    ScopedLockDtor(lock, -1);
    return 1;
}

// Start of the cumulative zone ranges: sum of the first n 32-bit counts at global settings +0x68.
static s32 SettingPrefix(s32 n) {
    s32 sum = 0;
    for (s32 i = 0; i < n; i++) sum += *(s32*)(GetGlobalSettings() + 0x68 + i * 4);
    return sum;
}

// True when zone id x lies in the ranges of the 7th, 8th or 9th zone counts (checked in the
// order 8th, 7th, 9th).
static BOOL InLateZoneRanges(s32 x) {
    return (SettingPrefix(7) + 1 <= x && x <= SettingPrefix(8)) ||
           (SettingPrefix(6) + 1 <= x && x <= SettingPrefix(7)) ||
           (SettingPrefix(8) + 1 <= x && x <= SettingPrefix(9));
}

// 0x80132C50
// True when any of the 0x10-byte entries (count +0x290, array pointer +0x284) has a zone id in
// the late zone ranges.
BOOL fn_80132C50(u8* arg0) {
    for (s32 i = 0; i < *(s32*)(arg0 + 0x290); i++) {
        s32 id = *(s32*)(*(u8**)(arg0 + 0x284) + i * 0x10);
        if (InLateZoneRanges(id)) return 1;
    }
    return 0;
}

// 0x80133134
// True when zone id arg1 lies in the late zone ranges (arg0 is unused).
BOOL fn_80133134(u8* arg0, s32 arg1) {
    return InLateZoneRanges(arg1);
}

// 0x801333B0
// True when arg1 is in the late zone ranges and equals the id (+0x18) of one of the three
// objects at arg0 +0x14/+0x18/+0x1C (the first non-null one that matches).
BOOL fn_801333B0(u8* arg0, s32 arg1) {
    if (!InLateZoneRanges(arg1)) return 0;
    u8* p = *(u8**)(arg0 + 0x14);
    if (p != NULL && arg1 == *(s32*)(p + 0x18)) return 1;
    p = *(u8**)(arg0 + 0x18);
    if (p != NULL && arg1 == *(s32*)(p + 0x18)) return 1;
    p = *(u8**)(arg0 + 0x1C);
    if (p != NULL && arg1 == *(s32*)(p + 0x18)) return 1;
    return 0;
}

// 0x80133698
// When zone arg1 maps (fn_801D3A60) to the same value as one of the first five zone-count
// boundaries (+1), runs fn_801384B0 for the mapped id of the end of the first eight counts (+1).
void fn_80133698(u8* arg0, s32 arg1) {
    s32 z = fn_801D3A60(arg1);
    if (z == fn_801D3A60(SettingPrefix(1) + 1) || z == fn_801D3A60(SettingPrefix(2) + 1) ||
        z == fn_801D3A60(SettingPrefix(3) + 1) || z == fn_801D3A60(SettingPrefix(4) + 1) ||
        z == fn_801D3A60(SettingPrefix(5) + 1)) {
        fn_801384B0(arg0, fn_801D3A60(SettingPrefix(8) + 1), 1);
    }
}

}  // extern "C"
