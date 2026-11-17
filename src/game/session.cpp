// Play session block (gGame + 0x5C8): saved zone object states, value slots, timers.
// Original object: .text 0x80012534 - 0x80014230 (auto_03_80012534_text.s)
#include <string.h>

#include "game/game.h"

extern "C" {
void* ScreenSetsCurrent(void* sets);                    // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);         // 0x80198E34
BOOL  ScreenSetWidgetVisible(void* set, void* widget);  // 0x80198C34
void  UiHideByHash(u32 hash, int a);                    // 0x8018F618
BOOL  fn_801F5220(void);
void  fn_801F5484(void);
void* EventPost(s32 id, s32 flags);                     // 0x801F3918: returns the payload to fill
void  SessionBegin(void* session);                      // 0x80013690
}

extern void* gScreenSets;                // 0x805FB654
extern u8    gWorldState[];              // 0x803E6D00
extern s32   gZoneResourceSize;          // 0x805FB94C
extern u8    gFlag805FB948;              // 0x805FB948

// Per-object saved state copied back into a zone (0x40 bytes).
struct ZoneObjState {
    u8  pad00[0x20];
    u32 flags;        // 0x20
    u8  pad24[0x1C];
};

struct ValueSlot { s32 id; f32 value; };   // 256 slots at +0x20C


static inline u8* ZoneInfoAt(s32 zone) {
    return *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
}

// 0x80012534
// Restore zone 1's object states from the session, then flag persistent objects as changed.
void SessionRestoreZoneObjects(u8* s) {
    if (*(s32*)(s + 0x8) == 0) return;

    s32 first = *(s32*)(s + 0x250);
    s32 count = *(s32*)(s + 0x254) - first;
    s32 statics = *(s32*)(s + 0x2A4);
    ZoneObjState* saved = (ZoneObjState*)(s + 0x2F0);
    u8* zone1 = ZoneInfoAt(1);
    memcpy(*(void**)(zone1 + 0x90), &saved[first], statics * sizeof(ZoneObjState));
    memcpy(*(void**)(zone1 + 0x8C), &saved[first + statics], (count - statics) * sizeof(ZoneObjState));

    for (s32 zone = 0; zone <= *(s32*)(*(u8**)gWorldState + 4); zone++) {
        u8* z = ZoneInfoAt(zone);
        for (s32 i = 0; i < *(s32*)(z + 0x94) + *(s32*)(z + 0x98); i++) {
            s32 n = *(s32*)(z + 0x94);
            ZoneObjState* o = (i < n) ? (ZoneObjState*)(*(u8**)(z + 0x8C)) + i
                                      : (ZoneObjState*)(*(u8**)(z + 0x90)) + (i - n);
            u32 f = o->flags;
            if ((f & 0x403) && !(f & 0x80)) o->flags = (f & ~0x20703u) | 0x800;
        }
    }
}

// 0x8001292C
void SessionResetSlots(u8* s) {
    ValueSlot* slots = (ValueSlot*)(s + 0x20C);
    for (int i = 0; i < 0x100; i++) {
        slots[i].id = 0;
        slots[i].value = 0.0f;
    }
    *(s32*)(s + 0xA0C) = 0;
}

// 0x80012C2C
void SessionSetTimer(u8* s, f32 time, u8 flag) {
    *(f32*)(s + 0x200) = time;
    s[0x208] = flag;
}

// 0x80012C38
void SessionStopTimer(u8* s) {
    s[0x208] = 0;
    *(f32*)(s + 0x200) = -1.0f;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL && ScreenSetWidgetVisible(set, ScreenSetFindWidget(set, 0xDB1E4D0C))) {
        UiHideByHash(0x9ACB282D, 0);
    }
}

// 0x80012F50
void SessionAddTime(u8* s, f32 dt) {
    f32 t = *(f32*)(s + 0x200) + dt;
    *(f32*)(s + 0x200) = t;
    if (t < 0.0f) *(f32*)(s + 0x200) = 0.0f;
}

// 0x80012F70
f32 SessionGetTimer(u8* s) {
    return *(f32*)(s + 0x200);
}

// 0x8001360C
void SessionStart(u8* s) {
    if (fn_801F5220()) {
        if (gFlag805FB948) {
            fn_801F5484();
            EventPost(0x11, 0);
        }
        gWorldState[0xF4] = 1;
        SessionBegin(s);
    } else {
        EventPost(0x20, 0);
    }
}

// 0x80013684
void SessionResetUnk204(u8* s) {
    *(f32*)(s + 0x204) = 0.5f;
}

// 0x80013F54
f32 GetSignFlip11(u8* p) {
    return p[0x11] ? -1.0f : 1.0f;
}

// 0x80013F70
f32 GetSign10(u8* p) {
    return p[0x10] ? 1.0f : -1.0f;
}

// 0x80014188
// Set the 0..3 option stored at gGame + 0xBE94 and broadcast it in two-player mode.
void GameSetOption3(u8* game, s32 value) {
    if (value < 0 || value > 3) return;
    if (value == *(s32*)(game + 0xBE94)) return;
    *(s32*)(game + 0xBE94) = value;
    if (fn_801F5220() && gFlag805FB948) {
        u8* e = (u8*)EventPost(0x2A, 0);
        if (e != NULL) e[4] = (u8)value;
    }
}

// 0x80014208
f32 GetOne(void) {
    return 1.0f;
}

// ---------------------------------------------------------------- timed object disables

extern "C" {
struct ObjRef { u8 data[0x404]; };                      // opaque object handle (copied by value)
ObjRef* ObjDbLookup(void* db, s32 id);                  // 0x800D39BC
u32   PathGetNodeType(ObjRef* ref);                      // 0x800D5944
void  ObjRefSetState(ObjRef* ref, u32 state);           // 0x800D59F4
u32   PathGetNodeId(void* ref);                           // 0x800D56F8
void  ObjRefDtor(ObjRef* ref, s32 flags);               // 0x800D50D0
void  ObjSetEnabled(s32 id, BOOL on);                   // 0x800F058C
void  CharacterSetTarget(void* chr, void* target);      // 0x80016FD0
BOOL  fn_800E6BD4(void);
}
extern void* gObjDb;                     ORIG(0x805FB180)
extern u8    gGame[];                    // 0x803B53A0
extern u8    gScriptCtx[];               // 0x803C4298

#define SLOT_COUNT(s) (*(u32*)((s) + 0xA0C))
#define SLOTS(s)      ((ValueSlot*)((s) + 0x20C))

// 0x80012820
// Disable object `id` (if it is in state 2) and schedule it to come back after `time`.
void SessionDisableFor(u8* s, s32 id, f32 time) {
    if (id == 0) return;
    BOOL disabled = FALSE;
    if (gObjDb != NULL) {
        ObjRef ref = *ObjDbLookup(gObjDb, id);
        if (PathGetNodeType(&ref) == 2) {
            disabled = TRUE;
            ObjRefSetState(&ref, 0);
            ObjSetEnabled(id, FALSE);
        }
        ObjRefDtor(&ref, -1);
    }
    if (disabled) {
        SLOTS(s)[SLOT_COUNT(s)].id = id;
        SLOTS(s)[SLOT_COUNT(s)].value = time;
        SLOT_COUNT(s)++;
    }
}

// 0x800129D4
// Re-enable object `id` and drop its slot (swap with the last one).
void SessionReenable(u8* s, s32 id) {
    u32 count = SLOT_COUNT(s);
    for (u32 i = 0; i < count; i++) {
        if (SLOTS(s)[i].id != id) continue;

        SLOTS(s)[i].id = SLOTS(s)[count - 1].id;
        SLOTS(s)[i].value = SLOTS(s)[SLOT_COUNT(s) - 1].value;
        SLOTS(s)[SLOT_COUNT(s) - 1].id = 0;
        SLOT_COUNT(s)--;

        if (gObjDb != NULL) {
            ObjRef ref = *ObjDbLookup(gObjDb, id);
            ObjRefSetState(&ref, 1);
            ObjSetEnabled(id, TRUE);

            // If the player was targeting this object, clear the target.
            u8* player = *(u8**)(gGame + 0xD614);
            if ((*(u32*)(*(u8**)(player + 4) + 0x1C) & 0x10) && player != NULL) {
                void* target = *(void**)(player + 0xA00);
                if (target != NULL && PathGetNodeId(&ref) == PathGetNodeId(target)) CharacterSetTarget(player, NULL);
            }
            ObjRefDtor(&ref, -1);
        }
        return;
    }
}

// 0x80012B5C
void SessionUpdateDisabled(u8* s, f32 dt) {
    if (fn_800E6BD4() || *(s32*)(gScriptCtx + 0x18) != 5) return;
    for (u32 i = 0; i < SLOT_COUNT(s); i++) {
        if (SLOTS(s)[i].id == 0) continue;
        SLOTS(s)[i].value -= dt;
        if (SLOTS(s)[i].value <= 0.0f) SessionReenable(s, SLOTS(s)[i].id);
    }
}

// ---------------------------------------------------------------- moving characters

extern "C" void fn_8020EBB8(void* dst, void* src);
extern "C" void ZoneLog(void* loader, const char* fmt, ...);   // 0x8000CC30
extern u8  gSirLoader[];                 // 0x8048A8F0
extern u8* gSaveSystem;                  // 0x805FBBE0

#define MAX_MOVING_CHARACTERS 20

// 0x80012F78
// Register a moving character in gGame's tables (max 20 per level).
BOOL RegisterMovingCharacter(u8* chr) {
    u32 n = *(u32*)(gGame + 0xFDC);
    if (n >= MAX_MOVING_CHARACTERS) {
        ZoneLog(gSirLoader, "Too Many Moving Characters in the level\n");
        return FALSE;
    }

    u32 defFlags = *(u32*)(*(u8**)(chr + 4) + 0x1C);
    if (!(defFlags & 0x10) || *(void**)(chr + 0xA00) == NULL) return TRUE;

    ((s32*)(gGame + 0xA8C8))[n] = *(s32*)(chr + 0x2C);
    ((void**)(gGame + 0xAB48))[n] = *(void**)(chr + 0xA00);
    memcpy(gGame + 0xA918 + n * 0x1C, chr + 0x87C, 0x1C);

    if (*(u32*)(*(u8**)(chr + 4) + 0x1C) & 0x2000) {
        fn_8020EBB8(gGame + 0xAB98 + n * 0x13C, chr + 0x898);
        if (chr == *(u8**)(gGame + 0xD614)) {
            *(u32*)(gGame + 0xD62C) = *(u32*)(gGame + 0xFDC);
            ZoneLog(gSirLoader, "Player Index %d\n", *(u32*)(gGame + 0xFDC));
        } else if (*(s32*)(chr + 0x2C) == *(s32*)(gSaveSystem + 0x130)) {
            *(u32*)(gGame + 0xD630) = *(u32*)(gGame + 0xFDC);
            ZoneLog(gSirLoader, "Enemy Index %d\n", *(u32*)(gGame + 0xFDC));
        }
    }
    (*(u32*)(gGame + 0xFDC))++;
    return TRUE;
}
