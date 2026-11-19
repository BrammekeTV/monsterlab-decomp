// Small functions from the 0x800D-0x8005xxxx and 0x801Bxxxx-0x8020xxxx ranges (world/zone
// lookup, pad-driven list stepping, entity flag resets, part/action name lookup, UI part state).
#include "types.h"
#include "game/part.h"

// Globals (C++ linkage, defined in their owning files).
extern u8 gWorldState[];          // 0x803E6D00 (first word is a pointer to the world block)
extern "C++" s32 gZoneResourceSize;  // 0x805FB94C
extern "C++" void* gScreenSets;   // 0x805FB654
extern u8 gGame[];                // 0x803B53A0
extern u32 lbl_805FAEB8;          // pad channel
extern u8 lbl_805FB0E4;           // flag
extern "C++" u8 lbl_805FB098;           // part selection lock flag
extern u8 lbl_805FB099;
extern u8 lbl_805FBBE0[];         // save block pointer (holds a pointer)
extern u8 lbl_8047C760[];         // effect manager
extern s32 lbl_805F95F8;          // word, compared with -1
extern f32 lbl_805FD240;          // float constant
extern f32 lbl_805FFBC0;          // float constant (zeroed value for +0x4C)
extern void* lbl_805FB01C;        // stored pointer (result of fn_8004DAF8)
extern void* lbl_805FB020;        // stored pointer (result of fn_8004DAF8)
extern u8 lbl_80359038[];         // vtable / data block
extern u8 lbl_80359180[];         // vtable / data block
extern u8 lbl_803C529C[];         // UI state block
extern s32 gPartSlotState[];      // 10 part slots

// Helper: when the status word at p+statusOff is 1 or 2, set the byte flag at p+flagOff to 1.
static void SetFlagIfStatusOne(u8* p, u32 statusOff, u32 flagOff) {
    if ((u32)(*(s32*)(p + statusOff) - 1) <= 1u) {
        *(u8*)(p + flagOff) = 1;
    }
}

extern "C" {
// Callees (addresses from config/RM7P4F/symbols.txt).
s32   fn_8001AFB4(u8* p, s32 del);                  // 0x8001AFB4
BOOL  ZoneIsLoaded(void* world, s32 zone);          // 0x8012F408
void* CharacterFind(s32 id);                        // 0x800E876C
s32   WorldGetPhase(void* world);                   // 0x800D83A8
u32   PadGetButtons(s32 chan);
void  UIEvent_SendToAllScreens(u32 a0);             // src/game/draft_miss4.cpp
BOOL  SetPartSelected(s32 slot);                    // src/game/combat_hud.cpp
void* ScreenSetsCurrent(void* sets);                // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);     // 0x80198E34
u32   StringHash(const char* s);
const char* AnimSetName(s32 id);
char* strchr(char* s, s32 c);                       // SDK strchr
void  operator_delete(void* p);                     // 0x801D426C
void  fn_801B3C20(u8* a, u8* b);                    // 0x801B3C20
void  fn_801EBA20(void* list, void* item);          // 0x801EBA20
void  fn_801C83AC(u8* mgr, u8* item);               // 0x801C83AC
void  fn_801C849C(u8* mgr, u8* item);               // 0x801C849C
s32   fn_801D329C(s32 arg0);                        // 0x801D329C
void  fn_8004D760(u8* c, u8* monster);              // 0x8004D760
void* fn_8004DAF8(void* c, s32 a, s32 b);           // 0x8004DAF8
void  fn_8005CC30(void);                            // 0x8005CC30
void  fn_8005D3C4(u8* c, s32 a, s32 b);             // 0x8005D3C4
void  fn_8020EA34(u8* a3);                          // 0x8020EA34
u8*   fn_802149AC(u8* obj, s32 del);                // 0x802149AC
u8*   fn_8020EA94(u8* p, s32 del);                  // 0x8020EA94
u8*   fn_80062CE0(u8* p, s32 del);                  // 0x80062CE0
BOOL  fn_801B50B0(void* unused, u8* obj);           // 0x801B50B0
void  fn_8004E8E8(u8* obj, u8* other);              // 0x8004E8E8
s32   fn_801ED050(u8* p, s32 a, u8* b);             // 0x801ED050
s32   fn_8004EED0(u8* arg0, u32 arg1, s32 arg2);    // 0x8004EED0
void  fn_8021570C(char* out, s32 limb, void* fighter, s32 action);  // 0x8021570C
}

extern "C" {

// 0x800D8114
void fn_800D8114(u8* arg0) {
    u8* temp = *(u8**)(arg0 + 0x44);
    if (temp != NULL) {
        fn_8001AFB4(temp, 0);
        temp = *(u8**)(arg0 + 0x44);
        if (*(s32*)(temp + 0x434) != 0) {
            *(s32*)(temp + 0x434) = 0;
        }
    }
}

// 0x800E60BC
// Scans the loaded zones for the first zone entity whose type field is 0x10 and returns it (NULL if none).
void* fn_800E60BC(void) {
    u8* found = NULL;
    s32 zone = 0;
    while (zone <= *(s32*)(*(u8**)gWorldState + 0x4) && found == NULL) {
        if (ZoneIsLoaded(gWorldState, zone) != 0) {
            u8* world = *(u8**)gWorldState;
            u8* rec = *(u8**)(world + 0x1A4) + zone * gZoneResourceSize;
            s32 i = 0;
            while (i < (s32)*(u8*)(rec + 0xAC) && found == NULL) {
                u8* entry = *(u8**)(*(u8**)(rec + 0xA8) + i * 4);
                if (*(s32*)(entry + 0x8) == 0x10) {
                    found = entry;
                }
                i += 1;
            }
        }
        zone += 1;
    }
    return found;
}

// 0x80162A70
// Eight 0x6C0-byte records: where a status word is 1 or 2, sets the matching byte flag to 1.
void fn_80162A70(u8* arg0) {
    u8* p = arg0;
    for (s32 i = 0; i < 8; i++) {
        SetFlagIfStatusOne(p, 0xD0, 0xD4);
        SetFlagIfStatusOne(p, 0x1A8, 0x1AC);
        SetFlagIfStatusOne(p, 0x280, 0x284);
        SetFlagIfStatusOne(p, 0x358, 0x35C);
        SetFlagIfStatusOne(p, 0x430, 0x434);
        SetFlagIfStatusOne(p, 0x508, 0x50C);
        SetFlagIfStatusOne(p, 0x5E0, 0x5E4);
        SetFlagIfStatusOne(p, 0x6B8, 0x6BC);
        p += 0x6C0;
    }
}

// 0x801B420C
// Pad-driven stepping through the linked list at +0x4: while button 0x200 is held, advances the current node.
void fn_801B420C(u8* arg0) {
    u32 buttons = PadGetButtons(lbl_805FAEB8);
    u8* cur = *(u8**)(arg0 + 0x4);
    if (cur != NULL) {
        fn_801B3C20(*(u8**)cur, arg0);
        if ((buttons & 0x200) != 0) {
            cur = *(u8**)(arg0 + 0x4);
            if (cur != NULL) {
                u8* next = *(u8**)(cur - 0x4);
                u8* var = (next != NULL) ? next + 0x8 : NULL;
                fn_801EBA20(*(void**)(arg0 + 0x8), cur);
                *(u8**)(arg0 + 0x4) = var;
                if (var == NULL) {
                    *(u8*)arg0 = 0;
                }
            } else {
                *(u8*)arg0 = 0;
            }
        }
    }
}

// 0x801BB478
// Resets the record's state; when arg1 is set and the +0xC index is valid, keeps the 0x80 state bit.
void fn_801BB478(u8* arg0, s32 arg1) {
    s32 flags = *(s32*)(arg0 + 0x28) & ~0x81;
    *(s32*)(arg0 + 0x28) = flags;
    if (arg1 != 0 && *(s32*)(arg0 + 0xC) > -1) {
        *(s32*)(arg0 + 0x28) = flags | 0x80;
    } else {
        *(s32*)(arg0 + 0x4) = -1;
        *(s32*)(arg0 + 0xC) = -1;
    }
    f32 sp14 = *(f32*)(arg0 + 0x3C);
    f32 sp8 = *(f32*)(arg0 + 0x40);
    f32 spC = *(f32*)(arg0 + 0x44);
    f32 sp10 = *(f32*)(arg0 + 0x48);
    *(s32*)(arg0 + 0x8) = -1;
    *(s32*)(arg0 + 0x28) = *(s32*)(arg0 + 0x28) & 0x80;
    *(s32*)(arg0 + 0x14) = 0;
    *(s32*)(arg0 + 0x18) = 0;
    *(s32*)(arg0 + 0x1C) = 0;
    *(s32*)(arg0 + 0x20) = 0;
    *(f32*)(arg0 + 0x4C) = lbl_805FFBC0;
    *(f32*)(arg0 + 0x2C) = sp14;
    *(f32*)(arg0 + 0x30) = sp8;
    *(f32*)(arg0 + 0x34) = spC;
    *(f32*)(arg0 + 0x38) = sp10;
}

// 0x801D3A60
// Returns the count from fn_801D329C minus one.
s32 fn_801D3A60(s32 arg0) {
    return fn_801D329C(arg0) - 1;
}

// 0x801ECD1C
// Entry arg1 (0xC8-byte records) gets its item at +0x88 released via the effect manager (arg2 != 0 selects
// fn_801C83AC, else fn_801C849C) and its slots cleared; if no item is held, clears the +0x68 flag.
void fn_801ECD1C(u8* arg0, s32 arg1, s32 arg2) {
    if (arg1 >= 0 && arg1 < *(s32*)arg0) {
        s32 off = arg1 * 0xC8;
        u8* rec = *(u8**)(arg0 + 0x4) + off;
        u8* item = *(u8**)(rec + 0x88);
        if (item != NULL) {
            if (arg2 != 0) {
                fn_801C83AC(lbl_8047C760, item);
            } else {
                fn_801C849C(lbl_8047C760, item);
            }
            rec = *(u8**)(arg0 + 0x4) + off;
            *(void**)(rec + 0x88) = NULL;
            *(s32*)(rec + 0x68) = 0;
            *(s32*)(rec + 0xA8) = 0;
            return;
        }
        if (*(s32*)(rec + 0x68) != 0) {
            *(s32*)(rec + 0x68) = 0;
            *(s32*)(*(u8**)(arg0 + 0x4) + off + 0xA8) = 0;
        }
    }
}

// 0x8020E864
// Releases the object at +0x8 (vtable slot 0x8 with arg 1, then clears it), releases the five objects in
// the +0xC slot array the same way, calls fn_802149AC on the sub-block at +0x28 and deletes the object when arg1 > 0.
void fn_8020E864(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* p = *(u8**)(arg0 + 0x8);
        if (p != NULL) {
            (*(void (**)(u8*, s32))(*(u8**)p + 0x8))(p, 1);
            *(void**)(arg0 + 0x8) = NULL;
        }
        p = *(u8**)(arg0 + 0x8);
        if (p != NULL) {
            (*(void (**)(u8*, s32))(*(u8**)p + 0x8))(p, 1);
        }
        for (s32 i = 0; i < 5; i++) {
            u8* obj = *(u8**)(arg0 + 0xC + i * 4);
            if (obj != NULL) {
                (*(void (**)(u8*, s32))(*(u8**)obj + 0x8))(obj, 1);
                *(void**)(arg0 + 0xC + i * 4) = NULL;
            }
        }
        fn_802149AC(arg0 + 0x28, -1);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
}

// 0x8003E000
// Looks up two fixed characters, binds each to its arg with fn_8004D760 and stores the results in lbl_805FB01C/20.
void fn_8003E000(u8* arg0, u8* arg1, u8* arg2) {
    u8* c1 = (u8*)CharacterFind((s32)0x9C2CDBD4u);
    u8* c2 = (u8*)CharacterFind((s32)0xE9A6975Bu);
    fn_8004D760(c1, arg1);
    lbl_805FB01C = fn_8004DAF8(c1, 0, 0);
    fn_8004D760(c2, arg2);
    lbl_805FB020 = fn_8004DAF8(c2, 0, 1);
}

// 0x80046F5C
// Resets the part-selection UI state: sends the UI reset event, clears the lock flags and all 10 part slots, selects slot 4.
void fn_80046F5C(void) {
    UIEvent_SendToAllScreens(0xD00B7E35u);
    lbl_805FB098 = 0;
    *(s32*)(lbl_803C529C + 0x0) = 0x4E;
    *(u8*)(lbl_803C529C + 0x4) = 0;
    *(s32*)(lbl_803C529C + 0x8) = 4;
    lbl_805FB099 = 0;
    for (s32 i = 0; i < 10; i++) {
        gPartSlotState[i] = -1;
    }
    SetPartSelected(4);
}

// 0x8004B604
// Initialises the entity block: vtable, sub-object at +0xC48, zeroed fields and, for a valid +0x4 link, flags it.
void fn_8004B604(u8* arg0) {
    fn_8005CC30();
    fn_8020EA34(arg0 + 0xC48);
    u8* temp6 = *(u8**)(arg0 + 0x4);
    *(u8**)(arg0 + 0x0) = (u8*)lbl_80359038;
    *(u8**)(arg0 + 0xC48) = (u8*)lbl_80359038 + 0xDC;
    *(s32*)(arg0 + 0xC4C) = 0;
    *(u8*)(arg0 + 0xC82) = 1;
    *(u8*)(arg0 + 0xC83) = 1;
    *(u8*)(arg0 + 0xC85) = 0;
    *(u8*)(arg0 + 0xC86) = 0;
    if (temp6 != NULL) {
        *(s32*)(temp6 + 0x1C) = *(s32*)(temp6 + 0x1C) | 0x2000;
        *(u8*)(temp6 + 0x34) = 0xD;
    }
    if (lbl_805F95F8 != -1) {
        *(s32*)(arg0 + 0xC50) = 0;
        *(s32*)(arg0 + 0xC6C) = 0;
        *(s32*)(arg0 + 0xC54) = 0;
        *(s32*)(arg0 + 0xC70) = 0;
        *(s32*)(arg0 + 0xC58) = 0;
        *(s32*)(arg0 + 0xC74) = 0;
        *(s32*)(arg0 + 0xC5C) = 0;
        *(s32*)(arg0 + 0xC78) = 0;
        *(s32*)(arg0 + 0xC60) = 0;
        *(s32*)(arg0 + 0xC7C) = 0;
        *(s32*)(arg0 + 0xC64) = 0;
        *(s32*)(arg0 + 0xC68) = 0;
        *(u8*)(arg0 + 0xC84) = 0;
        *(u8*)(arg0 + 0xC80) = 0;
        *(u8*)(arg0 + 0xC81) = 0;
        *(f32*)(arg0 + 0xC88) = lbl_805FD240;
        *(u8*)(arg0 + 0xC87) = 0;
    }
}

// 0x8004B87C
// Per-tick update of an entity: sets a flag on its sub-object when the state word matches, runs the
// virtual check at slot 0xF0 (re-initialising through fn_8004DAF8 when it fails), clears the timer fields.
void fn_8004B87C(u8* arg0, s32 arg1, s32 arg2) {
    if ((u32)(*(u32*)(arg0 + 0x2C) + 0x569D0000u) == 0x8202u) {
        u8* temp4 = *(u8**)(arg0 + 0x4);
        *(s32*)(temp4 + 0x20) = *(s32*)(temp4 + 0x20) | 0x40000000;
    }
    if (*(u8*)(arg0 + 0xC84) != 0 && (*(s32 (**)(u8*))(*(u8**)arg0 + 0xF0))(arg0) == 0) {
        fn_8004DAF8(arg0, 0, 0);
    }
    *(u8*)(arg0 + 0xC80) = 0;
    *(f32*)(arg0 + 0xC88) = lbl_805FD240;
    *(u8*)(arg0 + 0xC81) = 0;
    fn_8005D3C4(arg0, arg1, arg2);
}

// 0x8004BA50
// For each of the five slots at +0xC50, walks its list (vtable slot 0xCC) and calls fn_801B50B0 for every
// other slot's entry, passing the entry's owner object (offset 0x20 of its +0x4 block, rebased by -0x264).
void fn_8004BA50(u8* arg0) {
    for (s32 i = 0; i < 5; i++) {
        u8* obj = *(u8**)(arg0 + 0xC50 + i * 4);
        if (obj != NULL) {
            u8* list = (*(u8* (**)(u8*))(*(u8**)obj + 0xCC))(obj);
            for (s32 j = 0; j < 5; j++) {
                if (j != i) {
                    u8* node = *(u8**)(list + 0xC + j * 4);
                    if (node != NULL) {
                        u8* owner = *(u8**)(*(u8**)(node + 0x4) + 0x20);
                        if (owner != NULL) {
                            owner -= 0x264;
                        }
                        fn_801B50B0(obj, owner);
                    }
                }
            }
        }
    }
}

// 0x8004E594
// Finds the two underscores in a name like "x_NAME_y" and returns the index of the animation set whose
// three-letter name matches the characters after the first one (-1 when not found).
s32 fn_8004E594(u8* arg0) {
    char* s = (char*)arg0;
    if (strchr(s, 0x5F) == NULL || strchr(strchr(s, 0x5F) + 1, 0x5F) == NULL) {
        return -1;
    }
    char* tail = strchr(strchr(s, 0x5F) + 1, 0x5F);
    for (s32 i = 0; i < 0x15; i++) {
        const char* name = AnimSetName(i);
        if ((s8)tail[1] == (s8)name[0] && (s8)tail[2] == (s8)name[1] && (s8)tail[3] == (s8)name[2]) {
            return i;
        }
    }
    return -1;
}

// 0x8004E7D4
// Resets the object's vtable and sub-object at +0x264, releases the sub-object at +0x268 through fn_8020E864,
// clears its links and frees the object when arg1 > 0. Returns the object.
u8* fn_8004E7D4(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* temp5 = *(u8**)(arg0 + 0x268);
        *(u8**)(arg0 + 0x0) = (u8*)lbl_80359180;
        *(u8**)(arg0 + 0x264) = (u8*)lbl_80359180 + 0xBC;
        if (temp5 != NULL) {
            fn_8020E864(temp5, 1);
        }
        fn_8020EA94(arg0 + 0x264, 0);
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8004EDD0
// Sets or clears the active flag (byte +0x38F): when set, marks the two linked objects; when clear, unmarks
// them, calls fn_801ED050 with the sub-block at +0x94 and zeroes +0x38E. Returns the +0x4 object after setting bit 0x8000.
u8* fn_8004EDD0(u8* arg0, s32 arg1, s32 arg2) {
    *(u8*)(arg0 + 0x38F) = (u8)arg1;
    if ((u8)arg1 != 0) {
        if ((*(u8* (**)(u8*))(*(u8**)arg0 + 0xC))(arg0) != NULL) {
            u8* temp3 = (*(u8* (**)(u8*))(*(u8**)arg0 + 0xC))(arg0);
            *(s32*)(temp3 + 0x8) = *(s32*)(temp3 + 0x8) | 2;
        }
        u8* temp4 = *(u8**)(arg0 + 0x4);
        *(s32*)(temp4 + 0x20) = *(s32*)(temp4 + 0x20) | 2;
        fn_8004E8E8(arg0, temp4);
    } else {
        if ((*(u8* (**)(u8*))(*(u8**)arg0 + 0xC))(arg0) != NULL) {
            u8* temp3 = (*(u8* (**)(u8*))(*(u8**)arg0 + 0xC))(arg0);
            *(s32*)(temp3 + 0x8) = *(s32*)(temp3 + 0x8) & ~2;
        }
        u8* temp5 = *(u8**)(arg0 + 0x4);
        *(s32*)(temp5 + 0x20) = *(s32*)(temp5 + 0x20) & ~2;
        fn_801ED050(arg0 + 0x94, arg2, temp5);
        *(u8*)(arg0 + 0x38E) = 0;
    }
    u8* temp = *(u8**)(arg0 + 0x4);
    *(s32*)(temp + 0x20) = *(s32*)(temp + 0x20) | 0x8000;
    return temp;
}

// 0x8004F020
// Looks up the action at index *arg1 (20-byte entries of the game database), asks the owner (vtable 0xCC)
// for the fighter, formats the action's name with fn_8021570C, hashes it and forwards to fn_8004EED0.
s32 fn_8004F020(u8* arg0, u8* arg1, s32 arg2) {
    u8* actions = *(u8**)(*(u8**)((u8*)gGameDb + 0xC) + 0x430);
    u8* entry = actions + *(s32*)arg1 * 0x14;
    s32 w0 = *(s32*)(entry + 0x0);
    s32 w1 = *(s32*)(entry + 0x4);
    s32 w2 = *(s32*)(entry + 0x8);
    s32 w3 = *(s32*)(entry + 0xC);
    void* fighter = (*(void* (**)(u8*, s32, s32, s32, s32, u8*))(*(u8**)arg0 + 0xCC))(arg0, w3, w2, w1, w0, entry);
    char name[0x40];
    fn_8021570C(name, *(s32*)(arg1 + 0x8), fighter, w0);
    u32 hash = StringHash(name);
    return fn_8004EED0(arg0, hash, arg2);
}

// 0x80050F88
// True when the screen set has the widget 0xBEF77BAE, the save block's +0xA1C field is zero, the
// lbl_805FB0E4 flag is set and the world phase is not 4.
BOOL fn_80050F88(void) {
    BOOL result = FALSE;
    if (ScreenSetFindWidget(ScreenSetsCurrent(gScreenSets), 0xBEF77BAEu) != NULL &&
        *(s32*)(*(u8**)lbl_805FBBE0 + 0xA1C) == 0 &&
        lbl_805FB0E4 != 0 &&
        WorldGetPhase(gGame + 0xD5F0) != 4) {
        result = TRUE;
    }
    return result;
}

}  // extern "C"
