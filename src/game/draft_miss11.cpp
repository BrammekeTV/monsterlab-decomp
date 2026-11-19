// Missing functions batch 11: junction/score-trigger/move-task script callbacks, zone slot
// acquisition, alert popup, UI child lookup, render-queue wait, small helpers.
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

struct StringTable;

extern u8           gGame[];                     // 0x803B53A0
extern u8           gWorldState[];               // 0x803E6D00 (first word: pointer to the world block)
extern u8           gIdPool[];                   // 0x8048A8F0
extern s32          gZoneResourceSize;           // 0x805FB94C
extern void*        gEventMgr;                   // 0x805FB6C8
extern void*        gScreenSets;                 // 0x805FB654
extern StringTable* gStringTable;                // 0x805FB910
extern StringTable* gStringTable2;               // 0x805FB90C
extern u8*          lbl_805FBBE0;                // 0x805FBBE0 (save block pointer)
extern void*        lbl_805FBBAC;                // 0x805FBBAC (voice SFX handle)
extern u8*          lbl_805FB2E8;                // 0x805FB2E8 (list of 0x40 event targets, count at +0x104)
extern u8           lbl_805FB09C[];              // 0x805FB09C (per-action "selectable" flags)
extern u8           lbl_803C5DB0[];              // 0x803C5DB0 (score trigger table, 0x8C bytes each, count at +0xAF0)
extern u8           lbl_8040D4C0[];              // 0x8040D4C0 (OSSemaphore)
extern u8           lbl_8040D4D8[];              // 0x8040D4D8 (OSSemaphore)
extern u8           lbl_8048A8B4[];              // 0x8048A8B4 (object checked by fn_801E5E24)
extern s32          lbl_805F90D8;                // 0x805F90D8
extern s32          lbl_805FBAF8;                // 0x805FBAF8
extern f32          lbl_805FBAF0;                // 0x805FBAF0
extern const u32    lbl_80375D80[];              // 0x80375D80 (6 widget hashes)
extern const char   lbl_805F8200[];              // 0x805F8200 (empty string)
extern const char   lbl_80354800[];              // 0x80354800 (string)
extern const char   lbl_8035480C[];              // 0x8035480C (string)

extern "C" {
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
u8*    GetGlobalSettings(void);                                  // 0x801D2BF8
u8*    LabAllocMonster(u8* inv);                                 // 0x80212D28
void   MonsterBuildPreset(u8* m, s32 preset);                    // 0x8021E4B0
void   MonsterSetName(void* monster, const char* name);          // 0x8021C8E0
void   PartSetHealth(PartInstance* p, s32 hp);                   // 0x8021F1F4
void   PartSetQuality(PartInstance* p, s32 q);                   // 0x8021EF68
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
void*  ScreenSetFindWidget(void* set, u32 hash);                 // 0x80198E34
void*  ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
void   UISetText(void* el, const char* text, s32 a);             // 0x8019EFC8
void*  UIWidgetFindChild(void* widget, u32 hash);                // 0x80199AEC
void   UIWidgetPlayAnim(void* w, u32 anim);                      // 0x80199B38
void   ZoneLog(void* loader, const char* fmt, ...);              // 0x801D7340
void   fn_801384B0(u8* world, s32 slot, s32 a);                  // 0x801384B0
void   fn_80138250(u8* world, s32 slot);                         // 0x80138250
void   fn_8013F45C(void* widget, s32 a);                         // 0x8013F45C
BOOL   fn_80198C34(void* set, void* w);                          // 0x80198C34
u8*    fn_8019BF64(u8* widget, u32 hash);                        // 0x8019BF64
u8*    fn_801B8040(u8* out, f32* from, f32* to, u32 a, u32 b, f32 duration);   // 0x801B8040
void*  fn_801D40E0(s32 n);                                       // 0x801D40E0
void   fn_801E4918(u8* obj, s32 a, f32 t);                       // 0x801E4918
void   fn_801E5C3C(u8* obj, s32 a, f32 t);                       // 0x801E5C3C
BOOL   fn_801E5E24(u8* p);                                       // 0x801E5E24
void   fn_801E8D74(void* out, void* list, void* owner, void* anim);   // 0x801E8D74
void   fn_801ED788(f32* out, u32 a1);                            // 0x801ED788
s32    fn_80338A2C(const char* a, const char* b, u32 n);          // 0x80338A2C (strncmp)
void   fn_801FF910(f32 v);                                       // 0x801FF910
void*  fn_80209814(void* v);                                     // 0x80209814
void   operator_delete(void* p);                                 // 0x801D426C
BOOL   PathIsAtJunction(void* path);                             // 0x800D5924
s32    PathGetNodeType(void* path);                              // 0x800D5944
}

typedef void (*UiVFn)(u8*, s32);

// Sum of the 13 consecutive global-settings counters starting at byte offset `first`
// (the original reads them one by one through GetGlobalSettings()).
static s32 SettingsSum13(u32 first) {
    s32 sum = 0;
    for (u32 i = 0; i < 13; i++) sum += *(s32*)(GetGlobalSettings() + first + i * 4);
    return sum;
}

extern "C" {

// 0x800477D0
// Script callback ActionSelectable: per-action flag byte table lookup.
u32 /*0x06*/ ActionSelectable(s32 a0) {
    return lbl_805FB09C[a0];
}

// 0x800754D0
// Script callback AddTrackableScoreTrigger: appends a (threshold, comparison, id) entry to the
// score trigger entry whose key (+4) matches a0. a2 is the comparison text ("<", "<=", ">", ">=", "=").
void AddTrackableScoreTrigger(u32 a0, f32 a1, u32 /*0x0A*/ a2, u32 a3) {
    const u8* op = (const u8*)a2;
    u32 cmp = 0;
    if ((s8)op[0] == '<') {
        cmp = (op[1] == '=') ? 2 : 3;
    } else if ((s8)op[0] == '>') {
        cmp = ((s8)op[1] != '=') ? 1 : 0;
    }
    u8* entry = NULL;
    u32 count = *(u32*)(lbl_803C5DB0 + 0xAF0);
    for (u32 i = 0; i < count; i++) {
        if (a0 == *(u32*)(lbl_803C5DB0 + i * 0x8C + 4)) {
            entry = lbl_803C5DB0 + i * 0x8C;
            break;
        }
    }
    // No match leaves entry NULL and the original writes through it (crash); kept as is.
    s32 n = *(s32*)(entry + 0x88);
    *(s32*)(entry + 0x88) = n + 1;
    u8* slot = entry + n * 0xC;
    *(f32*)(slot + 0x10) = a1;
    *(u32*)(slot + 0x14) = cmp;
    *(u32*)(slot + 0x18) = a3;
}

// 0x800ED4F0
// Script callback AddMoveEntityTask: queues a move of entity a0 from position a1 to a2.
void AddMoveEntityTask(void* a0, u32 /*0x17*/ a1, u32 /*0x17*/ a2, f32 a3, u32 /*0x06*/ a4, u32 /*0x06*/ a5) {
    u8  tween[8];
    u8  anim[0x80];
    f32 from[3];
    f32 to[3];
    fn_801ED788(from, a1);
    fn_801ED788(to, a2);
    fn_801B8040(anim, from, to, a4, a5, a3);
    fn_801E8D74(tween, (u8*)a0 + 0xFC, a0, anim);
}

// 0x800F0B48
// Script callback ActivateCurrentJunction: when the player's path follower stands on a junction
// (or a0 forces it), sends the event for the node type (8, 2, 4 or 16) to the player.
void ActivateCurrentJunction(u32 /*0x06*/ a0) {
    u8* player = *(u8**)(gGame + 0xD614);
    void* path = *(void**)(player + 0xA00);
    if (path == NULL) return;
    if (!PathIsAtJunction(path) && a0 == 0) return;
    u32 ev;
    switch (PathGetNodeType(*(void**)(player + 0xA00))) {
    case 8:  ev = 0xD0354B85; break;
    case 2:  ev = 0x74027504; break;
    case 4:  ev = 0x8AD54581; break;
    case 16: ev = 0x06B0A31C; break;
    default: return;
    }
    EventSend(gEventMgr, player, NULL, ev, 0, 0, 0, 0);
}

// 0x801035B4
// Script callback AltitudeGreaterThan: true when the entity altitude (+0x7D4) is above a1.
s32 AltitudeGreaterThan(u32 /*0x2F*/ a0, f32 a1) {
    return *(f32*)((u8*)a0 + 0x7D4) > a1;
}

// 0x800F8F40
// Script callback AddPlayerMonsterFromDatabase: allocates a lab monster from preset a0, sets its
// parts to quality 100 / full health, flags it (+0x4A) and names it from the preset name text.
void AddPlayerMonsterFromDatabase(u32 /*0x44*/ a0) {
    u8* m = LabAllocMonster(lbl_805FBBE0 + 0xA30);
    if (m != NULL) {
        MonsterBuildPreset(m, (s32)a0);
        *(u8*)(m + 0x4A) = 1;
        for (s32 i = 0; i < 5; i++) {
            PartInstance* part = *(PartInstance**)(m + 4 + i * 4);
            if (part != NULL) {
                PartSetQuality(part, 100);
                PartSetHealth(part, PartGetMaxHealth(part));
            }
        }
    }
    if (m == NULL) return;

    // The preset record (0x48 bytes) holds the name text hash at +8.
    u8* spec = *(u8**)(gGameDb->data + 0x9180) + a0 * 0x48;
    s32 id = TextIdFromHash(*(u32*)(spec + 8));
    const char* name;
    if (id == -1) {
        name = lbl_80354800;
    } else if (((u32)id & 0x80000000u) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** arr = *(const char***)(t + 0x0);
        u32 idx = (u32)id & 0x7FFFFFFFu;
        if (arr != NULL && idx < *(u32*)(t + 0xC)) name = arr[idx];
        else name = lbl_805F8200;
    } else {
        u8* t = (u8*)gStringTable;
        const char** arr = *(const char***)(t + 0x0);
        if (arr != NULL && (u32)id < *(u32*)(t + 0xC)) {
            name = arr[(u32)id];
        } else {
            name = lbl_805F8200;
            if (id == -1) name = lbl_8035480C;   // unreachable: id == -1 handled above
        }
    }
    if (name != NULL) MonsterSetName(m, name);
}

// 0x80126808
// Fires event 0x7E57AD29 at the next pending target of the list at lbl_805FB2E8 (index at
// +0x104, 0x40 = done) and marks the list as finished.
void fn_80126808(void) {
    u8* list = lbl_805FB2E8;
    if (list != NULL) {
        s32 idx = *(s32*)(list + 0x104);
        if (idx < 0x40) {
            EventSend(gEventMgr, *(void**)(list + idx * 4), NULL, 0x7E57AD29u, 0, 0, 1, 1);
            *(s32*)(list + 0x104) = 0x40;
        }
    }
}

// 0x80131A20
// Picks a zone slot to load into: waits on the id pool lock, then scans the world slot table
// (0x30 bytes each, state at +4) preferring state 0, then 2, then 3, and starts it. Returns the
// slot index, or -1. `world` is gWorldState (callers leave it in r3).
s32 AcquireZoneSlot(u8* world) {
    s32 free0 = -1;
    s32 free2 = -1;
    s32 free3 = -1;
    BOOL first = TRUE;
    while (fn_801E5E24(gIdPool)) {
        if (first) {
            ZoneLog(gIdPool, "lock_available_zone_slot waiting...");
            first = FALSE;
        }
        fn_801E5C3C(gIdPool, 1, 0.01f);
        fn_801E4918(lbl_8048A8B4, 1, 0.01f);
    }
    if (!first) ZoneLog(gIdPool, "lock_available_zone_slot wait finished");

    for (s32 i = 0; i < SettingsSum13(0x34); i++) {
        switch (*(s32*)(*(u8**)(gWorldState + 0xB0) + i * 0x30 + 4)) {
        case 0:
            free0 = i;
            // fallthrough
        case 2:
            free2 = i;
            // fallthrough
        case 3:
            free3 = i;
            break;
        default:
            break;
        }
    }
    s32 slot = -1;
    if (free0 != -1) slot = free0;
    else if (free2 != -1) slot = free2;
    else if (free3 != -1) slot = free3;
    if (slot != -1) {
        fn_801384B0(world, slot, 0);
        fn_80138250(world, slot);
    }
    return slot;
}

// 0x80141CA4
// Shows the alert popup: pops three overlay screens, pushes screen 0x28DCC9E9, sets its title and
// message texts, plays the alert sound, then shows or hides the "backing" child (depending on
// whether one of six watched screens is open) and animates the popup in.
void AlertShowScreen(const char* title, const char* text, s32 a2) {
    UIScreen_PopScreen(0x42AFE7BCu);   // "connecting"
    UIScreen_PopScreen(0x21D18DCBu);
    UIScreen_PopScreen(0x4E1A118Du);
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* screen = (u8*)ScreenSetFindWidget(set, 0x28DCC9E9u);
    if (screen == NULL) return;
    UIScreen_Push(0x28DCC9E9u);
    void* titleEl = UIWidgetFindChild(screen, 0x4CE15F62u);   // "attention"
    void* textEl = UIWidgetFindChild(screen, 0x7E654778u);
    if (titleEl == NULL || textEl == NULL) return;
    UISetText(titleEl, title, 1);
    UISetText(textEl, text, 1);
    fn_8013F45C(screen, a2);
    SetHitBoxes(0);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(0xB74304F3u, 0, 0, 0, 0, 0);   // "ui_general_action_alert"
    u8* group = (u8*)UIWidgetFindChild(screen, 0x38569AA8u);
    if (group == NULL) return;
    u8* backing = fn_8019BF64(group, 0x20B39FB5u);   // "backing"
    if (backing != NULL) {
        BOOL watched = FALSE;
        void* cur = ScreenSetsCurrent(gScreenSets);
        if (cur != NULL) {
            for (u32 i = 0; i < 6; i++) {
                void* w = ScreenSetFindWidget(cur, lbl_80375D80[i]);
                if (w != NULL && fn_80198C34(cur, w)) {
                    watched = TRUE;
                    break;
                }
            }
        }
        if (watched || (u32)lbl_805F90D8 == 0xE38B521Fu) {
            (*(UiVFn*)(*(u8**)(backing + 0x138) + 0x2C))(backing, 2);   // show
        } else {
            (*(UiVFn*)(*(u8**)(backing + 0x138) + 0x30))(backing, 2);   // hide
        }
    }
    (*(UiVFn*)(*(u8**)(group + 0x138) + 0x2C))(group, 0x1A);
    UIWidgetPlayAnim(screen, 0x197F3A14u);
}

// 0x8015EA40
// Blocks until the render command queue semaphore is full again (0x40 free slots).
void fn_8015EA40(void* unused) {
    (void)unused;
    while (OSGetSemaphoreCount((OSSemaphore*)lbl_8040D4C0) < 0x40) OSWaitSemaphore((OSSemaphore*)lbl_8040D4D8);
}

// 0x8016A3F8
// Resolves the entry offsets of table a0 (count at +0x10, data block at +0x14) into pointers
// stored from +0x28 on: entry i = block + *(block + *(block + 0x24) + i * 8).
void fn_8016A3F8(u8* arg0) {
    u8** out = (u8**)(arg0 + 0x28);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x10); i++) {
        u8* block = *(u8**)(arg0 + 0x14);
        s32 off = *(s32*)(block + *(s32*)(block + 0x24) + i * 8);
        *out = block + off;
        out++;
    }
}

// 0x8016A43C
// Advances each entry pointer (+0x28 on, count at +0x10) to the first key >= the current position:
// position = clamp(arg+0x24 / block+0x20 * 65535, 0, 65535). Mode (+4) 0 = 0x10-byte entries with
// the u16 key at +0x16, mode 1 = 8-byte entries with the key at +8.
void fn_8016A43C(u8* arg0) {
    s32 mode = *(s32*)(arg0 + 0x4);
    if (mode != 0 && mode != 1) return;
    f32 ratio = *(f32*)(arg0 + 0x24) / *(f32*)(*(u8**)(arg0 + 0x14) + 0x20);
    s32 pos = (s32)lrintf(65535.0f * ratio);
    s32 limit = 0xFFFF;
    s32 clamped = pos & ~(pos >> 31);
    if (clamped <= 0xFFFF) limit = clamped;
    u32 stride = (mode == 0) ? 0x10 : 8;
    u32 keyOff = (mode == 0) ? 0x16 : 8;
    u8** slot = (u8**)(arg0 + 0x28);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x10); i++) {
        while ((s32)(*(u16*)(*slot + keyOff)) < limit) *slot += stride;
        slot++;
    }
}

// 0x8019BF64
// Search of the widget tree (first child +0x120, next sibling +0x124) for the widget whose id
// (+0) equals hash: checks the direct children first, then recurses into each child.
u8* fn_8019BF64(u8* widget, u32 hash) {
    u8* first = *(u8**)(widget + 0x120);
    if (first == NULL) return NULL;
    for (u8* c = first; c != NULL; c = *(u8**)(c + 0x124)) {
        if (hash == *(u32*)c) return c;
    }
    for (u8* c = first; c != NULL; c = *(u8**)(c + 0x124)) {
        u8* r = fn_8019BF64(c, hash);
        if (r != NULL) return r;
    }
    return NULL;
}

// 0x801B5A60
// Index of the zone resource of the object (pointer at +4) inside its zone resource block:
// (ptr - block) >> 6 with the +0x8C block if flag 8 is set, else the +0x90 block if flag 4.
// 0 if there is no object or the zone is out of range, -1 if neither flag is set.
s32 fn_801B5A60(u8* arg0) {
    u8* obj = *(u8**)(arg0 + 0x4);
    if (obj == NULL) return 0;
    s32 zone = *(u16*)(obj + 0x32);
    if (zone >= SettingsSum13(0x68) + 1) return 0;
    u32 flags = *(u32*)(*(u8**)(arg0 + 0x4) + 0x1C);
    u32 blockOff;
    if (flags & 8) blockOff = 0x8C;
    else if (flags & 4) blockOff = 0x90;
    else return -1;
    u8* zones = *(u8**)(*(u8**)gWorldState + 0x1A4);
    u8* block = *(u8**)(zones + zone * gZoneResourceSize + blockOff);
    return (s32)(((uintptr_t)(*(u8**)(arg0 + 0x4)) - (uintptr_t)block) >> 6);
}

// 0x8020F4F0
// Deleting-destructor stub: frees p if del > 0; returns p.
u8* fn_8020F4F0(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
    return p;
}

// 0x8022ED98
// Deleting-destructor stub: frees p if del > 0; returns p.
u8* fn_8022ED98(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
    return p;
}

// 0x8021F338
// Appends a pointer to the list at a0 (pointers from +0, count at +0x1000).
void fn_8021F338(u8* arg0, u8* arg1) {
    s32 n = *(s32*)(arg0 + 0x1000);
    *(u8**)(arg0 + n * 4) = arg1;
    *(s32*)(arg0 + 0x1000) = n + 1;
}

// 0x801FFC04
// Clamps v to [0, 1]; if the lbl_805FBAF8 flag is set stores it in lbl_805FBAF0, otherwise
// passes it on to fn_801FF910.
void fn_801FFC04(u8* audio, f32 v) {
    (void)audio;   // r3 is not used by the original
    f32 c;
    if (v < 0.0f) c = 0.0f;
    else if (v <= 1.0f) c = v;
    else c = 1.0f;
    if (lbl_805FBAF8 != 0) {
        lbl_805FBAF0 = c;
        return;
    }
    fn_801FF910(c);
}

// 0x802209CC
// Tail call to memset.
void* fn_802209CC(void* dst, s32 val, u32 n) {
    return memset(dst, val, n);
}

// 0x80221288
// Tail call to the MSL strncmp (fn_80338A2C).
s32 fn_80221288(const char* a, const char* b, u32 n) {
    return fn_80338A2C(a, b, n);
}

}  // extern "C"
