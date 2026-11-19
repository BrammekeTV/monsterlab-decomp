// Miscellaneous functions from the third "missing" draft batch: logging stub, zone lookup,
// effect list removal, exploration pause/unpause, path exits, heap alloc/free, save options.
#include <stdint.h>

#include "types.h"
#include "game/part.h"

typedef s32 (*VFnThisS32)(void*);
typedef void* (*VFnThisPtr)(void*);
typedef void (*VFnThisDel)(void*, s32);
typedef void (*VFnTransform)(void* self, void* entity, void* xform, void* pos);

extern "C" {
const char* AnimSetName(s32 id);                                  // 0x8020E370
void  BufferFree(void* p);                                        // 0x801D4980
void  CharacterStartReaction(void* ch);                           // 0x8004E3D0
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
u8*   GetGlobalSettings(void);                                    // 0x801D2BF8
void* MemAllocDefault(u32 size);                                  // 0x801D4650
BOOL  PathAtWaypoint(void* p);                                    // 0x800D56E0
char* StrCopy(char* dst, const char* src);
int   SysSprintf(char* buf, const char* fmt, ...);                // 0x80142EFC
void  UISetText(void* el, const char* text, s32 a);
s32   WorldGetPhase(void* world);                                 // 0x800D83A8
BOOL  ZoneIsLoaded(void* world, s32 zone);                        // 0x8012F408
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);   // 0x8032F2E0
s32   fn_8004E594(u8* p);                                         // 0x8004E594
s32   fn_8004F0DC(void* slot, const char* name, s32 loop);        // 0x8004F0DC
s32   fn_800517C0(void);                                          // 0x800517C0
s32   fn_80123954(s32 v, u8* save);                               // 0x80123954
s32   fn_80133134(void* world, s32 zone);                         // 0x80133134
s32   fn_801333B0(void* world, s32 zone);                         // 0x801333B0
s32   fn_80160BD4(u8* mgr, s32 a, u8* item, void* c, u8* link);   // 0x80160BD4
u8*   fn_801B3308(void);                                          // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                // 0x801B3378
void  fn_801B75AC(void* p);                                       // 0x801B75AC
void  fn_801C7E9C(u8* item, u8* mgr);                             // 0x801C7E9C
s32   fn_801D3A60(s32 zone);                                      // 0x801D3A60
void* fn_801EEB20(u8* heap, u32 size);                            // 0x801EEB20
void  fn_801EBA20(void* list, void* item);                        // 0x801EBA20
void  fn_801EF1A8(u8* heap, u8* arg);                             // 0x801EF1A8
void* fn_8022ED60(void* p);                                       // 0x8022ED60
u8* fn_8022ED98(u8* p, s32 del);                              // 0x8022ED98
void  fn_8032F1E8(void* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);   // 0x8032F1E8
void  operator_delete(void* p);                                   // 0x801D426C

// Defined in save.cpp with C++ linkage (r6 = 1 is also passed to the Read/Write calls, unused).
}
extern "C++" {
BOOL  SaveFileExists(const char* dir, const char* file);          // 0x80011960
s32   SaveGameRead(const char* name, void* data, u32 size);       // 0x800113CC
s32   SaveGameWrite(const char* name, const void* data, u32 size);   // 0x80011024
}
extern "C" {
extern "C++" void* gEventMgr;      // 0x805FB6C8
extern "C++" s32   gZoneResourceSize;   // 0x805FB94C
extern u8   gGame[];               // 0x803B53A0
extern u8   gWorldState[];         // 0x803E6D00 (first word points to the world block)
extern const char* const lbl_80389058[];   // 0x80389058 (reaction slot name table)
extern const char lbl_805F8FD4[];  // 0x805F8FD4 (save directory string)
extern u8   lbl_8035FDE0[];        // 0x8035FDE0 (vtable)
extern u8   lbl_8047C760[];        // 0x8047C760 (effect manager)
extern u8   lbl_8047CAC0[];        // 0x8047CAC0 (main heap; second heap at +0x394)
extern u8*  lbl_803C7404;          // 0x803C7404 (allocation pool table, 0x20 bytes each)
extern s32* lbl_805FB798;          // 0x805FB798 (per-pool requested byte counters)
extern s32  lbl_805F95F8;          // 0x805F95F8 (current allocation pool index, -1 = main heap)
extern u32  lbl_805F81C0;          // 0x805F81C0 (movie state word)
extern u8   lbl_805FB0E0[];        // 0x805FB0E0
extern void* lbl_805FB1C8;         // 0x805FB1C8 (state record pointer)
extern s32  lbl_805FB2CC;          // 0x805FB2CC (value saved as the first options field)
extern u8   lbl_805FB2D5;          // 0x805FB2D5 (save error flag)
extern u8   lbl_805FB7A8[];        // 0x805FB7A8 (+4: second heap enabled)
extern u32  lbl_805FB7B0;          // 0x805FB7B0 (alloc count, high word)
extern u32  lbl_805FB7B4;          // 0x805FB7B4 (alloc count, low word)
extern u32  lbl_805FB7C0;          // 0x805FB7C0 (allocated bytes, high word)
extern u32  lbl_805FB7C4;          // 0x805FB7C4 (allocated bytes, low word)
extern u32  lbl_805FB7C8;          // 0x805FB7C8 (freed bytes, high word)
extern u32  lbl_805FB7CC;          // 0x805FB7CC (freed bytes, low word)
extern u8   lbl_805FB7E1[];        // 0x805FB7E1 (nonzero: allocation disabled)
extern u8*  lbl_805FBBE0;          // 0x805FBBE0 (save block pointer)
}

// 64-bit counter kept as two big-endian words (hi, lo): *hi:*lo += v.
static inline void Add64(u32* hi, u32* lo, u32 v) {
    u32 old = *lo;
    u32 nw = old + v;
    *lo = nw;
    *hi += (nw < old);
}

// Bump-allocates size bytes (16-byte aligned) from an allocation pool; falls back to a fresh
// block from the default allocator when the pool is full. Pool: +0xC cursor, +0x10 end,
// +0x14 bytes used, +0x18 allocation count, +0x1C overflow bytes.
static inline void* PoolAlloc(u8* pool, u32 size) {
    u32 end = *(u32*)(pool + 0x10);
    u32 cur = *(u32*)(pool + 0xC);
    u32 aligned = (cur + 0xF) & ~0xFu;
    u32 newCur = aligned + size;
    uintptr_t result;
    if (cur < end - 1 && newCur < end) {
        result = aligned;
    } else {
        s32 n = (s32)(size + (aligned - cur));
        s32 m = (s32)((size + 0x10) & ~0xFu);
        *(s32*)(pool + 0x1C) += n;
        if (m > n) n = m;
        result = ((uintptr_t)MemAllocDefault((u32)n) + 0xF) & ~(uintptr_t)0xF;
    }
    *(s32*)(pool + 0x14) += (s32)(aligned - cur);
    *(u32*)(pool + 0xC) = newCur;
    *(s32*)(pool + 0x18) += 1;
    return (void*)result;
}

extern "C" {

// 0x8000B2F0
// Debug log stub: the original only spills its argument registers to the stack and returns.
void GameLog(int level, int category, const char* fmt, ...) {
    (void)level;
    (void)category;
    (void)fmt;
}

// 0x8004CE44
// Releases the two control locks (slots at +0xC64): unregisters each with the input manager and
// destroys it through its vtable.
void ControlsUnlock(void* player) {
    u8* p = (u8*)player;
    for (s32 i = 0; i < 2; i++) {
        u8* lock = *(u8**)(p + 0xC64 + i * 4);
        if (lock != NULL) {
            fn_801B3378(fn_801B3308(), lock);
            u8* obj = *(u8**)(p + 0xC64 + i * 4);
            if (obj != NULL) (*(VFnThisDel*)(*(u8**)obj + 0x8))(obj, 1);
            *(u8**)(p + 0xC64 + i * 4) = NULL;
        }
    }
}

// 0x8004D3F8
// Starts a reaction animation named <slot>_<animset>_<name> on a character. Returns 0 when there
// is no reaction table, 1 when the slot is empty, else 1 unless some attached slot (+0xC50) does not
// know the name.
BOOL fn_8004D3F8(void* ch, const char* name, s32 slot, s32 loop) {
    u8* c = (u8*)ch;
    u8* table = *(u8**)(c + 0xC4C);
    if (table == NULL) return 0;
    u8* entry = ((u8**)table)[slot];
    if (entry == NULL || *(u8**)entry == NULL) return 1;

    u8* sub = *(u8**)(entry + 0x20);
    if (sub != NULL) sub -= 0x264;
    s32 defIndex = **(s32**)entry;
    s32 animSet;
    PartDef def;
    if (defIndex != -1) {
        PartDbGetDef(&def, gGameDb, defIndex);
        animSet = def.unk34;
    } else {
        animSet = fn_8004E594(sub + 0x108);
    }
    char buf[0x98];
    SysSprintf(buf, "%s_%s_%s", lbl_80389058[slot], AnimSetName(animSet), name);
    BOOL result = 1;
    for (s32 i = 0; i < 5; i++) {
        void* s = *(void**)(c + 0xC50 + i * 4);
        if (s != NULL && fn_8004F0DC(s, buf, loop) == 0) result = 0;
    }
    CharacterStartReaction(ch);
    return result;
}

// 0x800D675C
// Takes exit `exit` (clamped to 0..3) of the junction the follower stands at, if that exit exists and
// is not blocked (blocked exits are allowed when allowBlocked is set). Returns TRUE when taken.
BOOL PathTakeExit(void* path, s32 exit, s32 allowBlocked) {
    s32 e = exit < 0 ? 0 : (exit > 3 ? 3 : exit);
    u8* node = *(u8**)path;
    u8* junction = *(u8**)(node + 0x14);
    u32 mask = 0;
    if (junction != NULL) {
        for (s32 i = 0; i < 4; i++) {
            if (*(s32*)(junction + 0x4 + i * 4) != 0 &&
                (*(s32*)(junction + 0x24 + i * 4) == 0 || allowBlocked != 0)) {
                mask |= 1u << i;
            }
        }
    }
    if (((1u << e) & mask) == 0) return 0;

    u8* from = *(u8**)(node + 0x14);
    u8* target = *(u8**)(from + e * 4 + 0x4);
    *(u8**)(node + 0x10) = target;
    if (from == *(u8**)(target + 0x4)) {
        *(u8*)(*(u8**)path + 0x4) = 1;
        *(s32*)(*(u8**)path + 0xC) = 0;
    } else {
        *(u8*)(*(u8**)path + 0x4) = 0;
        *(s32*)(*(u8**)path + 0xC) = *(s32*)(target + 0xD8) - 1;
    }
    *(u8**)(*(u8**)path + 0x14) = NULL;
    return 1;
}

// 0x800DE5E4
// Buffer object destructor: installs the base vtable, frees the two buffers (+4, +0xC), clears the
// fields and deletes the object when del > 0. Returns the object.
u8* fn_800DE5E4(u8* p, s32 del) {
    if (p != NULL) {
        void* buf = *(void**)(p + 0x4);
        *(u8**)(p + 0x10) = lbl_8035FDE0;
        if (buf != NULL) BufferFree(buf);
        void* buf2 = *(void**)(p + 0xC);
        if (buf2 != NULL) BufferFree(buf2);
        *(s32*)(p + 0x0) = 0;
        *(s32*)(p + 0x8) = 0;
        *(void**)(p + 0x4) = NULL;
        *(void**)(p + 0xC) = NULL;
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800E4D7C
// Finds the first object of kind 0xF in any loaded zone (zones 0..count); NULL when there is none.
u8* fn_800E4D7C(u32 a0) {
    (void)a0;
    u8* found = NULL;
    for (s32 zone = 0; zone <= *(s32*)(*(u8**)gWorldState + 0x4) && found == NULL; zone++) {
        if (ZoneIsLoaded(gWorldState, zone) != 0) {
            u8* z = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
            u8** objs = *(u8***)(z + 0xA8);
            for (s32 i = 0; i < (s32)*(u8*)(z + 0xAC) && found == NULL; i++) {
                if (*(s32*)(objs[i] + 0x8) == 0xF) found = objs[i];
            }
        }
    }
    return found;
}

// 0x800E6BD4
// TRUE when the world is in phase 6 and (flag byte or the state record's byte +0x10 is set), or
// in another phase and (fn_800517C0 or the state record's byte +0x10 is set).
BOOL fn_800E6BD4(void) {
    BOOL r = 0;
    if (WorldGetPhase(gGame + 0xD5F0) == 6) {
        if (lbl_805FB0E0[0] != 0 || *(u8*)((u8*)lbl_805FB1C8 + 0x10) != 0) r = 1;
    } else if (fn_800517C0() != 0 || *(u8*)((u8*)lbl_805FB1C8 + 0x10) != 0) {
        r = 1;
    }
    return r;
}

// 0x800F6B00
// Sends the pause event to every exploration character (gGame + 0x140, 0x20 slots) that is an
// ordinary character, has flag 0x10 on its definition and is not already paused (+0x895).
void PauseAllExplorationCharacters(void) {
    u8** slot = (u8**)(gGame + 0x140);
    for (s32 i = 0; i < 0x20; i++, slot++) {
        u8* ch = *slot;
        if (ch != NULL && (*(VFnThisS32*)(*(u8**)ch + 0x4C))(ch) == 0) {
            u8* c = *slot;
            if ((*(u32*)(*(u8**)(c + 0x4) + 0x1C) & 0x10) && *(u8*)(c + 0x895) == 0) {
                EventSend(gEventMgr, c, NULL, 0x4F9213C1u, 0, 0, 0, 0);
            }
        }
    }
}

// 0x800F6BBC
// Sends the resume event to every exploration character with flag 0x10: the "move on" event when
// it has no path or is not at a waypoint, the "wait" event when it has a path but is not at one.
void UnPauseAllExplorationCharacters(void) {
    u8** slot = (u8**)(gGame + 0x140);
    for (s32 i = 0; i < 0x20; i++, slot++) {
        u8* ch = *slot;
        if (ch != NULL && (*(VFnThisS32*)(*(u8**)ch + 0x4C))(ch) == 0) {
            u8* c = *slot;
            if (*(u32*)(*(u8**)(c + 0x4) + 0x1C) & 0x10) {
                u8* path = *(u8**)(c + 0xA00);
                if (path != NULL && PathAtWaypoint(path) != 0) {
                    EventSend(gEventMgr, *slot, NULL, 0x33EE55C6u, 0, 0, 0, 0);
                } else if (path != NULL) {
                    EventSend(gEventMgr, *slot, NULL, 0xF4D7ABu, 0, 0, 0, 0);
                } else {
                    EventSend(gEventMgr, *slot, NULL, 0x33EE55C6u, 0, 0, 0, 0);
                }
            }
        }
    }
}

// 0x80123A88
// Writes the options to the save file: builds the save buffer (0x31C0 bytes, with the four
// 0x988-byte slots at +0xB98), loads the existing file if there is one, fills in the options and
// writes it; writes a second time when the save block has no completed flag (+0x924).
void SaveManager_SaveOptions(void) {
    alignas(32) u8 save[0x31C0];
    if (lbl_805FB2D5 == 0) {
        fn_8032F1E8(save + 0xB98, (void (*)(void))fn_8022ED60, (void (*)(void))fn_8022ED98, 0x988, 4);
        if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) != 0) {
            SaveGameRead("save", save, 0x31C0);
        }
        s32 complete = *(s32*)(lbl_805FBBE0 + 0x924);
        fn_80123954(lbl_805FB2CC, save);
        SaveGameWrite("save", save, 0x31C0);
        if (complete == 0) {
            fn_80123954(lbl_805FB2CC, save);
            SaveGameWrite("save", save, 0x31C0);
        }
        __destroy_arr(save + 0xB98, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
    }
}

// 0x8012F408
// TRUE when the zone's resources are loaded. Zone 0 needs the flag at +0x121; zones outside
// 1..0x1FE count as loaded; special zone ranges come from the global settings.
BOOL ZoneIsLoaded(void* world, s32 zone) {
    u8* w = (u8*)world;
    if (zone == 0 && *(u8*)(w + 0x121) == 0) return 0;
    if (zone < 1 || zone >= 0x1FF) return 1;
    if (fn_80133134(w, zone) != 0) return fn_801333B0(w, zone);

    if (*(s32*)(GetGlobalSettings() + 0x68) + 1 <= zone) {
        s32 a = *(s32*)(GetGlobalSettings() + 0x68);
        s32 b = *(s32*)(GetGlobalSettings() + 0x7C);
        s32 c = *(s32*)(GetGlobalSettings() + 0x6C);
        s32 d = *(s32*)(GetGlobalSettings() + 0x70);
        s32 e = *(s32*)(GetGlobalSettings() + 0x74);
        s32 f = *(s32*)(GetGlobalSettings() + 0x78);
        if (zone <= f + (e + (d + (c + (b + a))))) {
            u8* rec = *(u8**)(w + 0xB0) + fn_801D3A60(zone) * 0x30;
            if (*(s32*)(rec + 0x4) == 6 && zone == *(s32*)(rec + 0x8)) return 1;
            return 0;
        }
    }
    if (lbl_805F81C0 == 0 || lbl_805F81C0 == 3) {
        if (zone <= *(s32*)(*(u8**)w + 0x4)) {
            u8* rec = *(u8**)(w + 0xB0) + fn_801D3A60(zone) * 0x30;
            if (*(s32*)(rec + 0x4) == 6 && zone == *(s32*)(rec + 0x8)) return 1;
        }
        return 0;
    }
    if (lbl_805F81C0 == 2 && zone == 1) return 1;
    return 0;
}

// 0x8016195C
// Takes a free 0xD8-byte slot from the free list (head index at +0x3600, next link at +0xC8),
// fills it in and starts it via fn_80160BD4. On failure the slot goes back on the free list
// (its generation +0xC0 bumped by 0x40). Returns the slot's generation on success, else 0.
s32 fn_8016195C(u8* mgr, s32 a, s32 b, void* c) {
    s32 head = *(s32*)(mgr + 0x3600);
    u8* item;
    if (head < 0) {
        item = NULL;
    } else {
        item = mgr + head * 0xD8;
        *(s32*)(mgr + 0x3600) = *(s32*)(item + 0xC8);
    }
    if (item == NULL) return 0;
    *(s32*)(item + 0xC4) = a;
    *(s32*)(item + 0xC8) = 0;
    *(s32*)(item + 0xCC) = b;
    *(s32*)(item + 0xD0) = 1;
    *(u8*)(item + 0xD4) = 0;
    if (fn_80160BD4(mgr, a, item, c, item + 0xC8) == 0) {
        s32 gen = *(s32*)(item + 0xC0) + 0x40;
        *(s32*)(item + 0xC0) = gen;
        *(s32*)(item + 0xD0) = 0;
        *(s32*)(item + 0xC8) = *(s32*)(mgr + 0x3600);
        *(s32*)(mgr + 0x3600) = gen & 0x3F;
        return 0;
    }
    return *(s32*)(item + 0xC0);
}

// 0x8019F2A8
// Sets a UI element's text to the decimal number n.
void UISetNumber(void* el, s32 n) {
    char buf[0x28];
    SysSprintf(buf, "%d", n);
    UISetText(el, buf, 1);
}

// 0x801B4F4C
// If the entity's flag 0x8000 is set: clears it and pushes the transform (xform, or the entity's own
// at +8 when NULL) to the object returned by the vtable, then refreshes all children (+0xB0 list)
// whose entity has flag 1 or 2.
void EntitySyncTransform(void* entity, void* xform) {
    u8* e = (u8*)entity;
    if (*(u32*)(*(u8**)(e + 0x4) + 0x20) & 0x8000) {
        void* obj = (*(VFnThisPtr*)(*(u8**)e + 0xC))(e);
        u8* c = *(u8**)(e + 0x4);
        *(u32*)(c + 0x20) &= 0xFFFF7FFFu;
        if (xform == NULL) {
            if (obj != NULL) {
                u8* c2 = *(u8**)(e + 0x4);
                (*(VFnTransform*)(*(u8**)obj + 0x14))(obj, c2, e + 0x8, c2 + 0xC);
            }
        } else if (obj != NULL) {
            u8* c2 = *(u8**)(e + 0x4);
            (*(VFnTransform*)(*(u8**)obj + 0x14))(obj, c2, xform, c2 + 0xC);
        }
    }
    for (u8* n = *(u8**)(e + 0xB0); n != NULL; n = *(u8**)(n + 0x8)) {
        u8* child = *(u8**)(n + 0x4);
        if (child != NULL && (*(u32*)(*(u8**)(child + 0x4) + 0x20) & 3)) fn_801B75AC(n);
    }
}

// 0x801C83AC
// Removes an effect group (item) from the effect manager once none of its members are alive:
// notifies each member, unlinks it, clears the members' back references and drops the group.
void fn_801C83AC(u8* mgr, u8* item) {
    if (item == NULL) return;
    for (s32 i = 0; i < *(s32*)(item + 0xC); i++) {
        u8* m = *(u8**)(item + 0x14 + i * 4);
        if (m != NULL) fn_801C7E9C(m, mgr);
    }
    if (*(u8*)(item + 0x8) != 0) return;
    s32* link = *(s32**)(item + 0x0);
    if (link != NULL) {
        *link = 0;
        *(s32**)(item + 0x0) = NULL;
    }
    for (s32 i = 0; i < *(s32*)(item + 0xC); i++) {
        u8* m = *(u8**)(item + 0x14 + i * 4);
        if (m != NULL) *(s32*)(m + 0x68) = 0;
    }
    fn_801EBA20(*(void**)(mgr + 0xC), item);
    *(s32*)(mgr + 0x0) = *(s32*)(mgr + 0x0) - 1;
}

// 0x801C849C
// Like fn_801C83AC, but only notifies members whose definition flag (bit 8 of +0x1A8) is set, and
// instead of dropping the group it marks it finished (+8) and flags each member with 4.
void fn_801C849C(u8* mgr, u8* item) {
    if (*(u8*)(item + 0x8) != 0) return;
    for (s32 i = 0; i < *(s32*)(item + 0xC); i++) {
        u8* m = *(u8**)(item + 0x14 + i * 4);
        if (m != NULL && ((*(u32*)(*(u8**)(m + 0x5C) + 0x1A8) >> 8) & 1)) fn_801C7E9C(m, mgr);
    }
    if (*(u8*)(item + 0x8) != 0) return;
    s32* link = *(s32**)(item + 0x0);
    if (link != NULL) {
        *link = 0;
        *(s32**)(item + 0x0) = NULL;
    }
    for (s32 i = 0; i < *(s32*)(item + 0xC); i++) {
        u8* m = *(u8**)(item + 0x14 + i * 4);
        if (m != NULL) *(s32*)(m + 0x68) = 0;
    }
    *(u8*)(item + 0x8) = 1;
    for (s32 i = 0; i < *(s32*)(item + 0xC); i++) {
        u8* m = *(u8**)(item + 0x14 + i * 4);
        if (m != NULL) *(u32*)m |= 4;
    }
    fn_801EBA20(*(void**)(mgr + 0xC), item);
    *(s32*)(mgr + 0x0) = *(s32*)(mgr + 0x0) - 1;
}

// 0x801D4414
// Allocates size bytes from the current allocation pool (index lbl_805F95F8, 0x1FF means pool 0);
// a negative index uses the main heap, then the second heap. Counts the allocation. With
// lbl_805FB7E1 set it just returns 1 (allocation disabled).
void* ObjAlloc(u32 size) {
    s32 idx = lbl_805F95F8;
    if (idx == 0x1FF) idx = 0;
    void* r;
    if (idx >= 0) {
        if (lbl_805FB7E1[0] != 0) return (void*)1;
        lbl_805FB798[idx] += (s32)size;
        r = PoolAlloc(lbl_803C7404 + (idx << 5), size);
    } else {
        r = fn_801EEB20(lbl_8047CAC0, size);
        if (r == NULL && *(s32*)(lbl_805FB7A8 + 4) != 0) r = fn_801EEB20(lbl_8047CAC0 + 0x394, size);
    }
    Add64(&lbl_805FB7B0, &lbl_805FB7B4, 1);
    Add64(&lbl_805FB7C0, &lbl_805FB7C4, size);
    return r;
}

// 0x801EBA20
// Removes an item from the list (item - 8 is its link node: +0 prev, +4 next; list +0x14 head,
// +0x18 free-list owner or NULL for the list itself) and pushes the node onto the free list at
// +0x10; decrements the list's count (+0xC).
void fn_801EBA20(void* list, void* item) {
    u8* l = (u8*)list;
    u8* node = (u8*)item - 8;
    u8* owner = *(u8**)(l + 0x18);
    if (owner == NULL) owner = l;

    u8* prev = *(u8**)(node + 0x0);
    if (prev != NULL) *(u8**)(prev + 0x4) = *(u8**)(node + 0x4);
    u8* next = *(u8**)(node + 0x4);
    if (next != NULL) *(u8**)next = *(u8**)(node + 0x0);

    u8* head = *(u8**)(l + 0x14);
    if (node == head) {
        u8* nh = *(u8**)(head + 0x4);
        *(u8**)(l + 0x14) = nh;
        if (nh != NULL) *(u8**)nh = NULL;
    }
    *(u8**)(node + 0x4) = *(u8**)(owner + 0x10);
    *(u8**)(node + 0x0) = NULL;
    u8* freeHead = *(u8**)(owner + 0x10);
    if (freeHead != NULL) *(u8**)freeHead = node;
    *(u8**)(owner + 0x10) = node;
    *(s32*)(l + 0xC) = *(s32*)(l + 0xC) - 1;
}

// 0x801EF070
// Heap free (dlmalloc style, chunk header 8 bytes before p): counts the freed bytes, puts small
// chunks on their fast bin, otherwise coalesces with the previous and next free chunks (or the top
// chunk at +0x30), links the result into the unsorted list at +0x38 and trims the heap when the
// merged chunk is >= 0x20000 bytes.
void fn_801EF070(u8* heap, void* p) {
    if (p == NULL) return;
    u8* chunk = (u8*)p - 8;
    u32 size = *(u32*)(chunk + 4) & ~3u;
    Add64(&lbl_805FB7C8, &lbl_805FB7CC, size);

    u32 maxFast = *(u32*)heap;
    if (size <= maxFast) {
        *(u32*)heap = maxFast | 3;
        u8* bin = heap + (((size >> 3) - 2) << 2);
        *(u8**)(chunk + 8) = *(u8**)(bin + 4);
        *(u8**)(bin + 4) = chunk;
        return;
    }
    if (*(u32*)(chunk + 4) & 2) return;
    *(u32*)heap = maxFast | 1;

    u8* next = chunk + size;
    u32 nextSize = *(u32*)(next + 4) & ~3u;
    if ((*(u32*)(chunk + 4) & 1) == 0) {
        u32 prevSize = *(u32*)chunk;
        chunk -= prevSize;
        size += prevSize;
        u8* fd = *(u8**)(chunk + 8);
        u8* bk = *(u8**)(chunk + 0xC);
        *(u8**)(fd + 0xC) = bk;
        *(u8**)(bk + 8) = fd;
    }
    if (next == *(u8**)(heap + 0x30)) {
        size += nextSize;
        *(u32*)(chunk + 4) = size | 1;
        *(u8**)(heap + 0x30) = chunk;
    } else {
        u32 nextInUse = *(u32*)(next + nextSize + 4) & 1;
        *(u32*)(next + 4) = nextSize;
        if (!nextInUse) {
            u8* fd = *(u8**)(next + 8);
            size += nextSize;
            u8* bk = *(u8**)(next + 0xC);
            *(u8**)(fd + 0xC) = bk;
            *(u8**)(bk + 8) = fd;
        }
        u8* first = *(u8**)(heap + 0x40);
        *(u8**)(chunk + 0xC) = heap + 0x38;
        *(u8**)(chunk + 8) = first;
        *(u8**)(heap + 0x40) = chunk;
        *(u8**)(first + 0xC) = chunk;
        *(u32*)(chunk + 4) = size | 1;
        *(u32*)(chunk + size) = size;
    }
    if (size < 0x20000) return;
    if ((*(u32*)heap & 2) == 0) return;
    fn_801EF1A8(heap, heap);
}

// 0x802207A0
// Appends a copy of text to a fixed-capacity string list (capacity +4, count +8, strings +0xC).
// Returns FALSE when the list is full.
BOOL BattleLogAdd(void* log, const char* text) {
    u8* l = (u8*)log;
    s32 count = *(s32*)(l + 0x8);
    if (count == *(s32*)(l + 0x4)) return 0;
    StrCopy(((char**)*(u8**)(l + 0xC))[count], text);
    *(s32*)(l + 0x8) = *(s32*)(l + 0x8) + 1;
    return 1;
}

}  // extern "C"
