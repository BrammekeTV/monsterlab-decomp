// Missing functions batch 31: lightning-manager part selection (fn_8001E508), combat part set-up
// (fn_8004DAF8), string table lookup (fn_8011AEE8), screen widget removal (fn_80198A8C), the engine
// memory allocator (MemAlloc / MemFree), effect slot removal (fn_801ECDE8), the matchup screen
// callbacks (LoadLevel, LoginPoller, LostOpponent, MPNameEntryUpdateCursor, MatchupPreInit,
// MonsterSelectInit / Update) and the overlay-mesh alpha callbacks.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "game/part.h"

struct StringTable;

typedef void (*WidgetVFn)(void* self, s32 arg);

extern "C" {
#include "game/script_callbacks.h"

u32   AlertGetLastError(void);                                   // 0x80140B58
s32   fn_801138F8(void);                                         // 0x801138F8
void  fn_8000B7A8(u8* p, s32 a, s32 b);                          // 0x8000B7A8
void  fn_8001D4FC(u8* mgr);                                      // 0x8001D4FC (lightning manager refresh)
s32   fn_8003DDE8(void* part, s32 slot, s32 def, s32 flag, s32 a);  // 0x8003DDE8 (returns the part's entity id)
void  fn_8004C718(u8* p, s32 idx, void* elem);                   // 0x8004C718
void* fn_8004FFB4(s32 id, void* part);                           // 0x8004FFB4
s32   fn_8004E300(void* a, s32 b);                               // 0x8004E300
void  fn_8013A0A4(s32 a, s32 b);                                 // 0x8013A0A4 (callback)
void  fn_8013A654(s32 a);                                        // 0x8013A654 (callback)
void  fn_8013A6F4(s32 a, s32 b);                                 // 0x8013A6F4 (callback)
void  fn_8013F394(s32 a);                                        // 0x8013F394
void  fn_8014210C(void);                                         // 0x8014210C
BOOL  fn_80142358(void);                                         // 0x80142358
void  fn_8018DEF4(void* sets);                                   // 0x8018DEF4
BOOL  fn_80198C34(void* set, void* w);                           // 0x80198C34
void  fn_80199C30(void* obj);                                    // 0x80199C30
void  fn_8019BFF8(void* obj, void* cb, void* arg);               // 0x8019BFF8
s32   fn_80199094(void* obj);                                    // 0x80199094
s32   fn_80199100(void* obj);                                    // 0x80199100
s32   fn_801A3010(void* obj, void* data);                        // 0x801A3010
void* fn_801B7198(void* anim, s32 a);                            // 0x801B7198
void  fn_801C83AC(u8* mgr, u8* item);                            // 0x801C83AC
void  fn_801C849C(u8* mgr, u8* item);                            // 0x801C849C
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
s32   fn_801D4790(void* p);                                      // 0x801D4790
void  fn_801D65C0(void);                                         // 0x801D65C0
void  fn_801E8D74(void* out, void* list, void* owner, void* anim);  // 0x801E8D74
s32   fn_801EDF58(u8* ref);                                        // 0x801EDF58 (current zone index)
s32   fn_801F1D64(u8* obj);                                       // 0x801F1D64 (bounce task callback)
void  fn_801F2018(void);                                         // 0x801F2018 (fade task callback)
void  fn_801EF070(u8* heap, void* p);                            // 0x801EF070
void* fn_801EF6E8(u8* obj, s32 a, s32 v);                   // 0x801EF6E8
BOOL  fn_801F56FC(s32 flags);                                    // 0x801F56FC
void  fn_801F5794(s32 a);                                        // 0x801F5794
void  fn_801F5814(s32 a);                                        // 0x801F5814
void* fn_80209814(void* v);                                      // 0x80209814
u8*   fn_8020E530(u8* p, s32 a);                                 // 0x8020E530 (a is really an owner pointer)
u8    fn_8021D310(u8* p);                                        // 0x8021D310
u8    fn_8021D318(u8* p);                                        // 0x8021D318
void  fn_8023A5B0(u8* p);                                        // 0x8023A5B0
void  fn_8023A68C(u8* p);                                        // 0x8023A68C
s32   fn_802E99C4(s32 a, void* cb1, s32 b, void* cb2, s32 c, void* cb3, s32 d);  // 0x802E99C4

void* CharacterFind(s32 id);                                     // 0x800E876C
BOOL  MatchupIsActive(void);                                     // 0x801409FC
void  MatchupSetReady(BOOL ready);                               // 0x8013F2A4
void* MemAllocDefault(u32 size);                                 // 0x801D4F18 (default allocator)
s32   PartGetCurrentHealth(const PartInstance* p);               // 0x8021EF68
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
void* ScreenSetFindWidget(void* set, u32 hash);                  // 0x80198E34
void* ScreenSetsCurrent(void* sets);                             // 0x8018D1E4
u32   StringHash(const char* s);                                 // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);               // 0x80142EF8
void  UISetProgress(void* obj, f32 t);                           // 0x801A3FA0
void  UISetText(void* el, const char* text, s32 a);              // 0x8019EFC8
void* UIWidgetFindChild(void* widget, u32 hash);                 // 0x80199AEC
void  UIWidgetPlayAnim(void* w, u32 anim);                       // 0x80199B38
extern "C++" void UpdateAllZoneEntities(void);                   // 0x80010228
s32   WorldGetPhase(void* world);                                // 0x800D83A8
BOOL  ZoneIsLoaded(void* world, s32 zone);                       // 0x8012F408

extern u8 gGame[];                      // 0x803B53A0
extern u8 gLevelFlags[];                // 0x80369D60: 0x20 bytes per level, hash at +4
extern u8 gWorldState[];                // 0x803E6D00 (first word: world pointer)
extern u8 lbl_803C4298[];               // 0x803C4298
extern u8* lbl_803C7404;                // 0x803C7404 (memory pool records, 0x20 bytes each)
extern char lbl_803F2088[];             // 0x803F2088 (local monster name)
extern char lbl_803F2210[];             // 0x803F2210 (monster name text buffer)
extern u8 lbl_803F2250[];               // 0x803F2250 (matchup monster preset copy, 0x45 bytes)
extern char lbl_803F2230[];             // 0x803F2230 (name edit buffer)
extern u8 lbl_8047C760[];               // 0x8047C760 (effect manager)
extern u8 lbl_8047CAC0[];               // 0x8047CAC0 (engine heaps, second one at +0x394)
extern u8 lbl_80556AB0[];               // 0x80556AB0 (connection lock object)
extern s32 lbl_805F90A0;                // 0x805F90A0
extern f32 lbl_805F90A4;                // 0x805F90A4
extern f32 lbl_805F90A8;                // 0x805F90A8
extern f32 lbl_805F90AC;                // 0x805F90AC
extern f32 lbl_805F90C0;                // 0x805F90C0
extern f32 lbl_805F90C4;                // 0x805F90C4
extern f32 lbl_805F90C8;                // 0x805F90C8
extern s32 lbl_805F95F8;                // 0x805F95F8 (current memory pool index, -1 = none)
extern u8* lbl_805FAF78;                // 0x805FAF78 (lightning manager)
extern s32 lbl_805FB038;                // 0x805FB038 (number of combat part zones)
extern s32 lbl_805FB368;                // 0x805FB368
extern s32 lbl_805FB36C;                // 0x805FB36C
extern s32 lbl_805FB37C;                // 0x805FB37C
extern s32 lbl_805FB380;                // 0x805FB380
extern s32 lbl_805FB384;                // 0x805FB384
extern u8 lbl_805FB3B3;                 // 0x805FB3B3
extern u8 lbl_805FB3B4;                 // 0x805FB3B4
extern u8 lbl_805FB3B5;                 // 0x805FB3B5
extern s32 lbl_805FB3B8;                // 0x805FB3B8
extern s32 lbl_805FB3CC;                // 0x805FB3CC
extern u8 lbl_805FB3DC;                 // 0x805FB3DC
extern u8 lbl_805FB3DD;                 // 0x805FB3DD
extern u8 lbl_805FB3DE;                 // 0x805FB3DE
extern s32 lbl_805FB3E0;                // 0x805FB3E0
extern f32 lbl_805FB3F0;                // 0x805FB3F0 (selected monster health)
extern f32 lbl_805FB3F4;                // 0x805FB3F4 (energy)
extern f32 lbl_805FB3F8;                // 0x805FB3F8 (attack)
extern f32 lbl_805FB3FC;                // 0x805FB3FC (defense)
extern s32 lbl_805FB400;                // 0x805FB400 (monster select hide countdown)
extern u8 lbl_805FB414;                 // 0x805FB414 (matchup ready flag)
extern s32 lbl_805FB430;                // 0x805FB430 (hash of the pushed matchup screen)
extern s32* lbl_805FB798;               // 0x805FB798 (per-pool allocated byte counters)
extern s32* lbl_805FB79C;               // 0x805FB79C (per-pool allocation counters)
extern u8 lbl_805FB7A8[];               // 0x805FB7A8 (+4: second heap enabled)
extern u32 lbl_805FB7D0;                // 0x805FB7D0 (alloc call counter, high word)
extern u32 lbl_805FB7D4;                // 0x805FB7D4 (alloc call counter, low word)
extern u32 lbl_805FB7D8;                // 0x805FB7D8 (free call counter, high word)
extern u32 lbl_805FB7DC;                // 0x805FB7DC (free call counter, low word)
extern u8 lbl_805FB7E1;                 // 0x805FB7E1 (pool allocation disabled flag)
extern s32 lbl_805FBB48;                // 0x805FBB48 (connection nesting counter)
extern s32 lbl_805FBB64;                // 0x805FBB64
extern void* lbl_805FBBAC;              // 0x805FBBAC (voice SFX handle)
extern u8* lbl_805FBBE0;                // 0x805FBBE0 (save block pointer)
extern const char lbl_80354800[];       // 0x80354800 (string)
extern const char lbl_805F8200[];       // 0x805F8200 (string)
extern const f32 lbl_80346008[];        // 0x80346008 (3 floats: 0, 0, -0.2)
}

extern "C++" void* gScreenSets;         // 0x805FB654
extern "C++" StringTable* gStringTable; // 0x805FB910
extern "C++" StringTable* gStringTable2;  // 0x805FB90C
extern "C++" s32 gZoneResourceSize;     // 0x805FB94C

extern "C" {

// 0x8001E508
// Lightning manager: selects part index arg0 in the part picker. Deselects the old entry's widget,
// selects the new one, writes the "n/m" counter text and plays the select sound. Returns 1 if the
// selection changed.
s32 fn_8001E508(s32 arg0) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL || *(s32*)(mgr + 0xA80) == arg0) return 0;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return 0;
    void* root = ScreenSetFindWidget(set, 0xAD57FC00u);
    if (root == NULL) return 0;

    s32 prev = *(s32*)(mgr + 0xA80);
    if (prev != -1) {
        u8* w = (u8*)UIWidgetFindChild(root, *(u32*)(mgr + prev * 4 + 0xC1C));
        if (w != NULL) (*(WidgetVFn*)(*(u8**)(w + 0x138) + 0x30))(w, 2);
    }
    *(s32*)(lbl_805FAF78 + 0xA80) = arg0;
    mgr = lbl_805FAF78;
    s32 cur = *(s32*)(mgr + 0xA80);
    if (cur != -1) {
        u8* w = (u8*)UIWidgetFindChild(root, *(u32*)(mgr + cur * 4 + 0xC1C));
        if (w != NULL) (*(WidgetVFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 2);

        void* text = UIWidgetFindChild(root, *(u32*)(mgr + cur * 4 + 0xC30));
        if (text != NULL) {
            char buf[16] = { 0 };
            u8* entry = mgr + *(s32*)(mgr + 0xA80) * 0x1A4;
            s32 sel = *(s32*)(entry + 0x198);
            s32 last = *(s32*)(entry + 0x1A0) - 1;
            SysSprintf(buf, "%d/%d", (sel + 1) % (last + 1), last, sel + 1);
            UISetText(text, buf, 1);
        }
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0x175183F7u, 0, 0, 0, 0, 0);
    }
    fn_8001D4FC(lbl_805FAF78);
    return 1;
}

// 0x8004DAF8
// Builds the part-record for a combat creature c (stored at c+0xC4C) and creates the entity for
// each of its five part slots (c+0x89C); slots without a living part decrement the part zone count.
// In phase 2/4 with a == 0 the entities are then resolved through the world resource table.
// Returns the new record.
void* fn_8004DAF8(void* c, s32 a, s32 b) {
    u8* chr = (u8*)c;
    u8* rec = (u8*)fn_801D40E0(0x68);
    if (rec != NULL) {
        u8* owner = chr;
        if (chr != NULL) owner = chr + 0xC48;
        rec = fn_8020E530(rec, (s32)(intptr_t)owner);  // lint:ok owner pointer passed through an s32 parameter
    }
    *(s32*)(rec + 0x14) = *(s32*)(chr + 0x8DC);
    *(u8*)(rec + 0x18) = fn_8021D318(chr + 0x898);
    *(u8*)(rec + 0x19) = fn_8021D310(chr + 0x898);
    *(u8**)(chr + 0xC4C) = rec;

    s32 resolve = 0;
    if ((WorldGetPhase(gGame + 0xD5F0) == 2 || WorldGetPhase(gGame + 0xD5F0) == 4) && a == 0) resolve = 1;

    for (s32 k = 0; k < 5; k++) {
        PartInstance* p = *(PartInstance**)(chr + 0x89C + k * 4);
        if (p != NULL) {
            s32 def = p->defIndex;
            if (def != -1 && (PartGetCurrentHealth(p) > 0 || k == 1 || k == 4)) {
                *(s32*)(chr + 0xC6C + k * 4) = fn_8003DDE8(p, k, def, resolve, b);
            } else {
                *(s32*)(chr + 0xC6C + k * 4) = 0;
                lbl_805FB038 -= 1;
            }
        } else {
            lbl_805FB038 -= 1;
        }
    }
    PartInstance* extra = *(PartInstance**)(chr + 0x8AC);
    if (extra != NULL) PartGetCurrentHealth(extra);

    if (resolve != 0) {
        for (s32 k = 0; k < 5; k++) {
            s32 id = *(s32*)(chr + 0xC6C + k * 4);
            if (id != 0) {
                if ((u32)id < 0x3E8u) {
                    u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + (id - 1) * gZoneResourceSize;
                    *(s32*)(chr + 0xC6C + k * 4) = *(s32*)(res + 0x10C);
                }
                void* e = fn_8004FFB4(*(s32*)(chr + 0xC6C + k * 4), *(void**)(chr + 0x89C + k * 4));
                if (e != NULL) fn_8004C718(chr, k, e);
            }
        }
        UpdateAllZoneEntities();
    }
    return rec;
}

// 0x8011AEE8
// Looks up the display text of a part-text record: picks the text hash at +4..+0x10 by idx
// (1..4, anything else uses +4), resolves it through the string tables and returns the string.
void* fn_8011AEE8(u8* out, s32 idx, u8* end) {
    u32 hash = *(u32*)(out + 0x4);
    switch (idx) {
        case 2: hash = *(u32*)(out + 0x8); break;
        case 3: hash = *(u32*)(out + 0xC); break;
        case 4: hash = *(u32*)(out + 0x10); break;
        default: break;
    }
    s32 id = TextIdFromHash(hash);
    if (id == -1) return (void*)lbl_80354800;
    if (((u32)id & 0x80000000u) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** arr = *(const char***)(t + 0x0);
        if (arr != NULL) {
            u32 i = (u32)id & 0x7FFFFFFFu;
            if (i < *(u32*)(t + 0xC)) return (void*)arr[i];
        }
        return (void*)lbl_805F8200;
    }
    u8* t = (u8*)gStringTable;
    const char** arr = *(const char***)(t + 0x0);
    if (arr != NULL && (u32)id < *(u32*)(t + 0xC)) return (void*)arr[(u32)id];
    return (void*)lbl_805F8200;
}

// 0x80198A8C
// Removes widget arg1 from screen set arg0: clears its callbacks, drops it from the set's widget
// list (list at +0x40, count at +0x60) and, if no remaining widget is a blocking type, calls
// fn_801D65C0. Returns 0, or -1 if the widget was not in the set.
s32 fn_80198A8C(u8* arg0, u8* arg1) {
    fn_8018DEF4(gScreenSets);
    *(s32*)((u8*)gScreenSets + 0x74) = 0;
    fn_8019BFF8(arg1, (void*)fn_80199094, NULL);
    fn_8019BFF8(arg1, (void*)fn_80199100, NULL);
    fn_80199C30(arg1);

    s32 count = *(s32*)(arg0 + 0x60);
    s32 i = 0;
    for (s32 n = count; n > 0; n--) {
        if (*(u8**)(arg0 + 0x40 + i * 4) == arg1) break;
        i++;
    }
    if (i >= count) return -1;

    while (i < *(s32*)(arg0 + 0x60) - 1) {
        *(u8**)(arg0 + 0x40 + i * 4) = *(u8**)(arg0 + 0x44 + i * 4);
        i++;
    }
    *(u8**)(arg0 + 0x40 + i * 4) = NULL;
    *(s32*)(arg0 + 0x60) = *(s32*)(arg0 + 0x60) - 1;

    s32 found = 0;
    for (s32 j = 0; j < *(s32*)(arg0 + 0x60); j++) {
        u8* w = *(u8**)(arg0 + 0x40 + j * 4);
        if (w == NULL) continue;
        u32 type = *(u32*)w;
        if (type == 0x6825C6AEu) {  // "loading"
            found = 1;
        } else if (type == 0x1B6732BBu) {
            u8* child = (u8*)UIWidgetFindChild(w, 0x8DC6BEC1u);
            if (*(u32*)(child + 0x64) & 2) found = 1;
        }
        if (found) break;
    }
    if (found == 0) fn_801D65C0();
    return 0;
}

// 0x801ECDE8
// Removes the effect in slot with id a from the record idx of the effect list p (count at +0,
// records of 0xC8 bytes at +4: slot pointers at +0x88, ids at +0x68, extras at +0xA8, 8 slots).
// A live slot is released through the effect manager (immediate if non-zero); an empty slot just
// clears its id.
void fn_801ECDE8(u8* p, s32 idx, u32 a, s32 immediate) {
    if (idx < 0 || idx >= *(s32*)(p + 0x0)) return;
    s32 off = idx * 0xC8;
    u8* rec = *(u8**)(p + 0x4) + off;
    s32 found = -1;
    for (s32 k = 0; k < 8; k++) {
        if (*(s32*)(rec + 0x88 + k * 4) != 0 && a == *(u32*)(rec + 0x68 + k * 4)) found = k;
    }
    if (found == -1) return;

    s32 so = found * 4;
    u8* slot = *(u8**)(p + 0x4) + off + so;
    u8* item = *(u8**)(slot + 0x88);
    if (item != NULL) {
        if (immediate != 0) fn_801C83AC(lbl_8047C760, item);
        else fn_801C849C(lbl_8047C760, item);
        *(s32*)(*(u8**)(p + 0x4) + off + so + 0x88) = 0;
        *(s32*)(*(u8**)(p + 0x4) + off + so + 0x68) = 0;
        *(s32*)(*(u8**)(p + 0x4) + off + so + 0xA8) = 0;
        return;
    }
    if (*(s32*)(slot + 0x68) != 0) {
        *(s32*)(slot + 0x68) = 0;
        *(s32*)(*(u8**)(p + 0x4) + off + so + 0xA8) = 0;
    }
}

// 0x80107244
// Script callback: switches to the level whose name hash is a0 ("QUIT" selects the quit level 0x43).
// Unless it is already the current level, stops the rumble, closes the loading widget if shown and
// starts the level change.
void LoadLevel(u32 a0) {
    u8 level = 0;
    s32 found = 0;
    if (a0 == StringHash("QUIT")) {
        level = 0x43;
        found = 1;
    } else if (a0 != 0) {
        u8 i = 0;
        for (s32 n = 0; n < 0x11; n++, i++) {
            if (a0 == *(u32*)(gLevelFlags + (u32)i * 0x20 + 4)) {
                level = i;
                found = 1;
                break;
            }
        }
    }
    if (found != 0 && level != lbl_803C4298[0x1C]) {
        for (s32 ch = 0; ch < 4; ch++) StopRumble(ch);
        void* set = ScreenSetsCurrent(gScreenSets);
        if (set != NULL) {
            void* w = ScreenSetFindWidget(set, 0xBEF77BAEu);
            if (fn_80198C34(set, w) != 0) fn_80198A8C((u8*)set, (u8*)w);
        }
        fn_8000B7A8(lbl_803C4298, level, 0);
    }
}

// 0x8013B9E8
// Script callback: online login poll. Once per connect attempt, signals the game flow depending on
// the connection state (disconnect / error screens, or runs the login wait with the lock held).
void LoginPoller(void) {
    if (fn_80142358() == 0) return;
    s32 ready = 0;
    if (lbl_805FB380 != 0) {
        fn_8014210C();
        ready = 1;
    } else if (lbl_805FB384 != 0 || lbl_805FBB64 == 0) {
        ready = 1;
    }
    if (ready == 0 || lbl_805FB37C != 0) return;

    lbl_805FB37C = 1;
    if (lbl_805FB368 == 0 || lbl_805FB36C == 0) {
        if (fn_801F56FC(2) != 0) {
            GameFlowSignal(0x0A22EBA3, 0, 0);
            return;
        }
        if (IsShowingError() == 0) GameFlowSignal((s32)0xF82C50DEu, 0, 0);
    } else {
        if (fn_801F56FC(2) != 0) {
            GameFlowSignal(0x0A22EBA3, 0, 0);
            return;
        }
        fn_8023A5B0(lbl_80556AB0);
        lbl_805FBB48 += 1;
        SaveManager_SaveOptions();
        GameFlowSignal((s32)0xB90F7B7Du, 0, 0);
        fn_802E99C4(0, (void*)fn_8013A6F4, 0, (void*)fn_8013A0A4, 0, (void*)fn_8013A654, 0);
        fn_8023A68C(lbl_80556AB0);
        lbl_805FBB48 -= 1;
    }
}

// 0x8013F8E4
// Script callback: the opponent left. Pops the previously pushed matchup screen, then pushes the
// popup (0x9DE0B3FE) over whichever matchup screen is showing and remembers it in lbl_805FB430;
// signals the game flow when none is up.
void LostOpponent(void) {
    if (lbl_805FB430 != 0) {
        UIScreen_PopScreen((u32)lbl_805FB430);
        lbl_805FB430 = 0;
    }
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;

    void* w = ScreenSetFindWidget(set, 0x9DE0B3FEu);
    if (w != NULL && fn_80198C34(set, w) != 0) {
        GameFlowSignal(0x0CAA6159, 0, 0);
        return;
    }
    static const u32 kScreens[3] = { 0x2C434515u, 0x9631294Eu, 0xFEE5B157u };
    for (s32 i = 0; i < 3; i++) {
        w = ScreenSetFindWidget(set, kScreens[i]);
        if (w != NULL && fn_80198C34(set, w) != 0) {
            UIWidgetPlayAnim(w, 0x39440BA2u);  // "HideScreen"
            UIScreen_Push(0x9DE0B3FEu);
            lbl_805FB430 = (s32)kScreens[i];
            return;
        }
    }
    UIScreen_Push(0x9DE0B3FEu);
    GameFlowSignal(0x0CAA6159, 0, 0);
}

// 0x8013EDFC
// Script callback: moves the name-entry cursor widget. With an empty name it sits at the default
// position; otherwise it is placed at the world position of the "NAME" child transformed by that
// widget's matrix (3x4 at +0x17C), offset 2.0 in x.
void MPNameEntryUpdateCursor(void) {
    u8* set = *(u8**)((u8*)gScreenSets + 0x88);
    if (set == NULL) return;
    void* root = ScreenSetFindWidget(set, 0x68B693B2u);  // "NAME"
    if (root == NULL) return;
    u8* name = (u8*)UIWidgetFindChild(root, 0x68B693B2u);
    u8* cursor = (u8*)UIWidgetFindChild(root, 0xC0E2E2ABu);
    if (cursor == NULL || name == NULL) return;

    if (lbl_803F2230[0] == 0) {
        *(f32*)(cursor + 0x68) = lbl_805F90C0;
        *(f32*)(cursor + 0x6C) = lbl_805F90C4;
        *(f32*)(cursor + 0x70) = lbl_805F90C8;
        return;
    }
    f32 x = *(f32*)(name + 0x1D4);
    f32 y = *(f32*)(name + 0x1D8);
    f32 z = *(f32*)(name + 0x1DC);
    f32 px = *(f32*)(name + 0x1A0) + (*(f32*)(name + 0x194) * z + (*(f32*)(name + 0x17C) * x + *(f32*)(name + 0x188) * y));
    *(f32*)(cursor + 0x68) = 2.0f + px;
    *(f32*)(cursor + 0x6C) = lbl_805F90C4;
    *(f32*)(cursor + 0x70) = lbl_805F90C8;
}

// 0x8013CA74
// Script callback: resets the matchup state, opens the "inventory"/"creatures" lab book section
// and copies the 0x45-byte matchup monster preset (record at gGameDb data +0x9180, +0x35C) into
// lbl_803F2250.
void MatchupPreInit(void) {
    lbl_803F2088[0] = 0;
    lbl_805FB3B8 = 0;
    lbl_805FB3B3 = 0;
    lbl_805FB414 = 0;
    lbl_805FB3B4 = 0;
    lbl_805FB3B5 = 0;
    lbl_805FB3DC = 0;
    lbl_805FB3DD = 0;
    lbl_805FB3DE = 0;
    lbl_805FB3CC = 0;
    lbl_805FB3E0 = 0;
    InitLabBooks();
    SetBook(0xF5EF54D9u);     // "inventory"
    SetSection(0xE5368B8Bu);  // "creatures"

    u8* src = *(u8**)(gGameDb->data + 0x9180) + 0x35C + 4;
    for (s32 k = 0; k <= 0x30; k += 4) *(u32*)(lbl_803F2250 + k) = *(u32*)(src + k);
    for (s32 k = 0x34; k <= 0x44; k++) lbl_803F2250[k] = src[k];

    fn_8013F394(0);
    MatchupSetReady(0);
    fn_801F5794(1);
    fn_801F5814(0);
}

// 0x801D4A90
// Engine allocator. In pool mode (pool index lbl_805F95F8 >= 0 and flag 0x80 / 0x100 set) bumps
// the pool record's cursor (record: cursor +0xC, end +0x10, stats +0x14/+0x18/+0x1C), falling back
// to the default allocator when the pool is exhausted. Otherwise allocates from the main heap,
// then from the second heap if it is enabled.
void* MemAlloc(u32 size, u32 align, u32 flags, const char* tag, u32 extra) {
    s32 pool = lbl_805F95F8;
    u32 f = 0;
    if (pool >= 0) f = flags;
    s32 idx = (pool == 0x1FF) ? 0 : pool;

    if (pool >= 0 && (f & 0x180) != 0) {
        lbl_805FB798[idx] += size;
        u8* rec = lbl_803C7404 + idx * 0x20;
        if (lbl_805FB7E1 != 0) return (void*)1;
        u8* end = *(u8**)(rec + 0x10);
        uintptr_t mask = align - 1;
        u8* cur = *(u8**)(rec + 0xC);
        uintptr_t start = ((uintptr_t)cur + mask) & ~mask;
        uintptr_t next = start + size;
        uintptr_t result;
        if (cur < end - 1 && next < (uintptr_t)end) {
            result = start;
        } else {
            s32 need = (s32)(size + (s32)(start - (uintptr_t)cur));
            s32 want = (s32)((size + align) & ~mask);
            *(s32*)(rec + 0x1C) = *(s32*)(rec + 0x1C) + need;
            if (want > need) need = want;
            result = ((uintptr_t)MemAllocDefault((u32)need) + mask) & ~mask;
        }
        *(s32*)(rec + 0x14) = *(s32*)(rec + 0x14) + (s32)(start - (uintptr_t)*(u8**)(rec + 0xC));
        *(u8**)(rec + 0xC) = (u8*)next;
        *(s32*)(rec + 0x18) = *(s32*)(rec + 0x18) + 1;
        return (void*)result;
    }

    if (pool >= 0 && pool != 0x1FF) lbl_805FB79C[pool] += size;
    lbl_805FB7D4 += 1;
    if (lbl_805FB7D4 == 0) lbl_805FB7D0 += 1;
    void* r = fn_801EF6E8(lbl_8047CAC0, align, size);
    if (r == NULL && *(s32*)(lbl_805FB7A8 + 4) != 0) r = fn_801EF6E8(lbl_8047CAC0 + 0x394, align, size);
    return r;
}

// Returns the engine heap (lbl_8047CAC0 or the second heap at +0x394) whose address range
// (+0x37C - 0x10 .. +0x384, resp. +0x710 - 0x10 .. +0x718) contains p, or NULL.
static u8* HeapFor(void* p) {
    if (p == NULL) return NULL;
    u8* b = (u8*)p;
    if (b >= *(u8**)(lbl_8047CAC0 + 0x37C) - 0x10 && b < *(u8**)(lbl_8047CAC0 + 0x384)) return lbl_8047CAC0;
    if (*(s32*)(lbl_805FB7A8 + 4) != 0 && b >= *(u8**)(lbl_8047CAC0 + 0x710) - 0x10 &&
        b < *(u8**)(lbl_8047CAC0 + 0x718))
        return lbl_8047CAC0 + 0x394;
    return NULL;
}

// 0x801D52D0
// Engine free: pool memory (fn_801D4790) is ignored; otherwise counts the call and releases p
// into whichever heap owns it.
void MemFree(void* arg0) {
    if (fn_801D4790(arg0) != 0) return;
    lbl_805FB7DC += 1;
    if (lbl_805FB7DC == 0) lbl_805FB7D8 += 1;
    u8* heap = HeapFor(arg0);
    if (heap != NULL) fn_801EF070(heap, arg0);
}

// 0x801F2984
// Script callback: fades an overlay mesh (a1) in the zone of entity a0 to alpha a2 over time a3 by
// starting a fade task (fn_801F2018) on the zone's task list.
void MeshFadeToOverlayAlpha(void* a0, void* a1, f32 a2, f32 a3) {
    if (a1 == NULL) return;
    u8 zone = *(u8*)((u8*)a0 + 0x1);
    if (ZoneIsLoaded(gWorldState, (s32)zone) == 0) return;
    u8* list = *(u8**)(*(u8**)gWorldState + 0x1C8) + zone * 0xC;

    u8 anim[0x80];
    memset(anim, 0, 0x64);
    // Pointer slots first: they are 8 bytes wide on x64 and overlap the following float.
    *(void**)(anim + 0x4) = a1;
    *(void**)(anim + 0x6C) = (void*)fn_801F2018;
    *(u16*)(anim + 0x78) = 0xFFFF;
    *(u16*)(anim + 0x7A) = 0xFFFF;
    *(u16*)(anim + 0x7C) = 0xFFFF;
    *(u32*)(anim + 0x64) = 0;
    *(s32*)(anim + 0x68) = 0;
    *(f32*)(anim + 0x70) = 0.0f;
    *(s32*)(anim + 0x74) = 0;
    *(u8*)(anim + 0x7E) = 0;
    *(u8*)(anim + 0x7F) = 0;
    *(s32*)(anim + 0x0) = 0;
    *(f32*)(anim + 0x8) = a2;
    f32 dur = (0.0f <= a3) ? a3 : 0.0f;
    *(f32*)(anim + 0xC) = dur;
    f32 rate = 0.0f;
    if (dur > 0.0f) rate = 1.0f / a3;
    *(f32*)(anim + 0x10) = rate;

    u8* rec = *(u8**)((u8*)a1 + 0x10);
    const char* name = (rec != NULL) ? *(const char**)(rec + 0x5C) : NULL;
    *(u32*)(anim + 0x64) = (name != NULL) ? StringHash(name) : 0;
    u8 handle[8];
    fn_801E8D74(handle, list, NULL, anim);
}

// 0x801F26A0
// Script callback: starts a bouncing alpha on overlay mesh a1 in the current zone: period from a2
// (min 0.01), alpha oscillating between a3 and a4 (both clamped to 0..1) via fn_801F1D64.
void MeshStartBouncingOverlayAlpha(void* a0, void* a1, f32 a2, f32 a3, f32 a4) {
    if (a1 == NULL) return;
    s32 zone = fn_801EDF58((u8*)(uintptr_t)a0);
    if (ZoneIsLoaded(gWorldState, zone) == 0) return;
    u8* list = *(u8**)(*(u8**)gWorldState + 0x1C8) + zone * 0xC;

    f32 period = (a2 > 0.01f) ? a2 : 0.01f;
    f32 inv = 1.0f / period;

    u8 anim[0x80];
    memset(anim, 0, 0x64);
    // Pointer slots first: they are 8 bytes wide on x64 and overlap the following float.
    *(void**)(anim + 0x4) = a1;
    *(void**)(anim + 0x6C) = (void*)fn_801F1D64;
    *(u32*)(anim + 0x64) = 0;
    *(s32*)(anim + 0x68) = 0;
    *(f32*)(anim + 0x70) = 0.0f;
    *(u16*)(anim + 0x78) = 0xFFFF;
    *(u16*)(anim + 0x7A) = 0xFFFF;
    *(u16*)(anim + 0x7C) = 0xFFFF;
    *(s32*)(anim + 0x74) = 0;
    *(u8*)(anim + 0x7E) = 0;
    *(u8*)(anim + 0x7F) = 0;
    *(f32*)(anim + 0x8) = 2.0f * inv;

    if (a3 >= 1.0f) a3 = 1.0f;
    f32 lo = (0.0f <= a3) ? a3 : 0.0f;
    if (a4 >= 1.0f) a4 = 1.0f;
    f32 hi = (0.0f <= a4) ? a4 : 0.0f;

    *(s32*)(anim + 0x0) = 0;
    f32 mn = (lo < hi) ? lo : hi;
    *(f32*)(anim + 0xC) = mn;
    f32 mx = (lo <= hi) ? hi : lo;
    *(f32*)(anim + 0x10) = mx;
    *(u8*)(anim + 0x18) = 1;
    *(f32*)(anim + 0x14) = mx - mn;

    u8* rec = *(u8**)((u8*)a1 + 0x10);
    const char* name = (rec != NULL) ? *(const char**)(rec + 0x5C) : NULL;
    *(u32*)(anim + 0x64) = (name != NULL) ? StringHash(name) : 0;
    u8 handle[8];
    fn_801E8D74(handle, list, NULL, anim);
}

// 0x8013E4D0
// Script callback: monster select screen set-up. Resets the displayed stats, remembers the
// preview's position and fills the stat bars (health, energy, attack, defense), the monster name
// and the "n/m" counter, then enters the multiplayer section.
void MonsterSelectInit(void) {
    if ((u32)AlertGetLastError() == 0x13E20C39u) return;
    lbl_805FB400 = 0;
    lbl_805FB3F0 = 0.0f;
    lbl_805FB3F4 = 0.0f;
    lbl_805FB3F8 = 0.0f;
    lbl_805FB3FC = 0.0f;
    lbl_803F2210[0] = 0;
    if (WeArePostMatchupScreen() == 0) lbl_805F90A0 = -1;

    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        void* root = ScreenSetFindWidget(set, 0x2C434515u);
        if (root != NULL) {
            u8* preview = (u8*)UIWidgetFindChild(root, 0x60B15D91u);
            if (preview != NULL && lbl_805F90A4 == -1.0f && lbl_805F90A8 == -1.0f && lbl_805F90AC == -1.0f) {
                lbl_805F90A4 = *(f32*)(preview + 0x68);
                lbl_805F90A8 = *(f32*)(preview + 0x6C);
                lbl_805F90AC = *(f32*)(preview + 0x70);
            }
            void* bar = UIWidgetFindChild(root, 0x3E22E495u);  // "Health"
            if (bar != NULL) {
                lbl_805FB3F0 = Creatures_GetCurrentHP();
                UISetProgress(bar, lbl_805FB3F0);
            }
            bar = UIWidgetFindChild(root, 0x67E9BE17u);  // "energy"
            if (bar != NULL) {
                lbl_805FB3F4 = Creatures_GetCurrentEnergy();
                UISetProgress(bar, lbl_805FB3F4);
            }
            bar = UIWidgetFindChild(root, 0xB738EABDu);  // "attack"
            if (bar != NULL) {
                lbl_805FB3F8 = Creatures_GetCurrentAttack();
                UISetProgress(bar, lbl_805FB3F8);
            }
            bar = UIWidgetFindChild(root, 0xE4E00B6Fu);  // "Defense"
            if (bar != NULL) {
                lbl_805FB3FC = Creatures_GetCurrentDefense();
                UISetProgress(bar, lbl_805FB3FC);
            }
            void* nameText = UIWidgetFindChild(root, 0x2EADCD52u);
            if (nameText != NULL) {
                SysSprintf(lbl_803F2210, "%s", (const char*)Creatures_GetMonsterName());
                UISetText(nameText, lbl_803F2210, 1);
            }
            void* counter = UIWidgetFindChild(root, 0x524A7A53u);
            if (counter != NULL) {
                char buf[24];
                s32 n = fn_801138F8();
                SysSprintf(buf, "%d/%d", n + 1, *(s32*)(lbl_805FBBE0 + 0x10E8));
                UISetText(counter, buf, 1);
            }
        }
    }
    SetMPSection(0x13E20C39u);
    SetHitBoxes(1);
}

// 0x8013E898
// Script callback: per-frame update of the monster select preview. Keeps the preview monster at
// its saved position (lowered while it plays animation 13), refreshes its visibility and, after
// the hide countdown (lbl_805FB400) runs out, hides the stat overlay and the preview.
void MonsterSelectUpdate(void) {
    if (MatchupIsActive() == 0) return;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    void* root = ScreenSetFindWidget(set, 0x2C434515u);
    if (root == NULL) return;
    u8* preview = (u8*)UIWidgetFindChild(root, 0x60B15D91u);
    if (preview == NULL) return;

    if (lbl_805F90A4 == -1.0f) {
        lbl_805F90A4 = *(f32*)(preview + 0x68);
        lbl_805F90A8 = *(f32*)(preview + 0x6C);
        lbl_805F90AC = *(f32*)(preview + 0x70);
    }
    fn_801A3010(preview, CharacterFind((s32)0xA9638202u));
    f32 pos[3] = { lbl_805F90A4, lbl_805F90A8, lbl_805F90AC };
    void* anim = fn_801B7198(preview + 0x140, 0);
    if (anim != NULL && fn_8004E300(anim, 1) == 0xD) {
        pos[0] += lbl_80346008[0];
        pos[1] += lbl_80346008[1];
        pos[2] += lbl_80346008[2];
    }
    *(f32*)(preview + 0x68) = pos[0];
    *(f32*)(preview + 0x6C) = pos[1];
    *(f32*)(preview + 0x70) = pos[2];
    (*(WidgetVFn*)(*(u8**)(preview + 0x138) + 0x2C))(preview, 0x18);

    if (!(*(u32*)(preview + 0x64) & 2) && lbl_805FB400 == 0) lbl_805FB400 = 3;
    if (lbl_805FB400 > 0) {
        lbl_805FB400 -= 1;
        if (lbl_805FB400 == 0) {
            UIElement_SetVisibleFlag(0x379F79BDu, 0);
            (*(WidgetVFn*)(*(u8**)(preview + 0x138) + 0x2C))(preview, 2);
        }
    }
}

}  // extern "C"
