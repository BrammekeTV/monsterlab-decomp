// Miscellaneous functions from the fourth "missing" draft batch: conversation start, UI screen
// events, prop update, locator lookup, small pool allocator, audio voice release, fade timer.
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "revolution.h"

typedef void (*VFnI)(void*, s32);

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void   ExplorationPromptClear(BOOL animate);                     // 0x80016EA0
void   HudShowConversation(void* hud, s32 conversation);         // 0x8001B0B8
u32    OSGetTick(void);                                          // 0x8023D494
void   ScreenSetsActivate(void* sets, u32 hash);                 // 0x8018D70C
void*  ScreenSetFindWidget(void* set, u32 hash);                 // 0x80198E34
void*  ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
void   UISetText(void* el, const char* text, s32 a);             // 0x8019EFC8
void*  UIWidgetFindChild(void* widget, u32 hash);                // 0x80199AEC
void   UIWidgetPlayAnim(void* w, u32 anim);                      // 0x80199B38
s32    WorldGetPhase(void* world);                               // 0x800D83A8
BOOL   ZoneIsLoaded(void* world, s32 zone);                      // 0x8012F408
BOOL   PropIncrementState(void* prop);                           // 0x80069FDC
void   fn_8006A764(u8* prop, f32 t);                             // 0x8006A764
void   fn_8006B244(u8* prop);                                    // 0x8006B244
void   fn_8006C068(u8* prop, f32 t);                             // 0x8006C068
void   fn_8006F67C(u8* p, f32 t, s32 flag);                      // 0x8006F67C
s32    fn_800D5C48(u8* ref, s32 a);                              // 0x800D5C48
void   fn_80126808(void);                                        // 0x80126808
s32    fn_801478C8(u8* table, s32 idx);                          // 0x801478C8
void   fn_8016A3F8(u8* p);                                       // 0x8016A3F8
void   fn_8016A43C(u8* p);                                       // 0x8016A43C
void   fn_8016A788(u8* p);                                       // 0x8016A788
void   fn_8018D2B8(const char* fmt, ...);                        // 0x8018D2B8 (debug log)
void   fn_8018D370(const char* fmt, ...);                        // 0x8018D370 (debug log)
void   fn_8018DEF4(void* sets);                                  // 0x8018DEF4
s32 fn_8019BD18(u8* arg0, u32* arg1);                       // 0x8019BD18 (routes event to widget)
BOOL   fn_80198C34(void* set, void* w);                          // 0x80198C34
void   fn_801BBB68(void* mgr, s32 id, int a, int b);             // 0x801BBB68
void   fn_801C83AC(u8* mgr, u8* item);                           // 0x801C83AC
const char* fn_801EB708(u32 hash);                               // 0x801EB708 (hash -> name for logs)
void   fn_801EBAB8(u8* pool);                                    // 0x801EBAB8 (pool: build the free list)
void   fn_801EC080(u8* a, u8* b);                                // 0x801EC080
s32    fn_801FF21C(u8* audio, u8* sound);                        // 0x801FF21C
void   fn_801FF2A8(u8* audio, s32 handle);                       // 0x801FF2A8 (audio voice release)
}

extern u8   gAudio[];                    // audio manager
extern u8   gGame[];                     // 0x803B53A0
extern "C++" void* gScreenSets;          // 0x805FB654
extern "C++" void* gEventMgr;            // 0x805FB6C8
extern u8   gWorldState[];               // 0x803E6D00
extern "C++" s32   gZoneResourceSize;    // 0x805FB94C
extern const char lbl_803818A8[];        // 0x803818A8 (UI script: no active screen set)
extern const char lbl_803818D0[];        // 0x803818D0 (UI script: widget not found)
extern const char lbl_80381FC8[];        // 0x80381FC8 (UI script: null screen entry)
extern u8   lbl_8047C760[];              // 0x8047C760 (effect manager)
extern u8   lbl_80559D78[];              // 0x80559D78 (0x28-byte audio voice records)
extern const char lbl_805F9558[];        // 0x805F9558 ("%s: %s")
extern u8   lbl_805FB0E0[];              // 0x805FB0E0 (byte flag: set when a conversation starts)
extern u8   lbl_805FB0E4;                // 0x805FB0E4
extern u32  lbl_805FB0EC;                // 0x805FB0EC (pending conversation id)
extern u8   lbl_805FB0F8[];              // 0x805FB0F8 (pointer slot: conversation text widget)
extern u8*  lbl_805FB490;                // 0x805FB490 (current register table)
extern u8*  lbl_805FB494;                // 0x805FB494 (spare register table)
extern s32  lbl_805FB6B8;                // 0x805FB6B8
extern u32  lbl_805FB9DC;                // 0x805FB9DC (flag word)
extern s32  lbl_805FBA70;                // 0x805FBA70 (fade state 0..3)
extern u32  lbl_805FBA74;                // 0x805FBA74 (fade state start tick)
extern u8   lbl_805FBA7C;                // 0x805FBA7C (fade alpha)
extern u8*  lbl_805FBBE0;                // 0x805FBBE0 (save block pointer)
extern f32  lbl_805FF084;                // 0x805FF084 (float constant)
extern f32  lbl_805FF830;                // 0x805FF830 (float constant)

extern "C" {

// Pad register entry query used by fn_80147554: the top three bits select the form (1: bit 0,
// 2: the entry shifted left 3 read as a float is greater than lbl_805FF084, else 0).
static BOOL PadEntryHeld(u8* table, s32 idx) {
    u32 v = (u32)fn_801478C8(table, idx);
    u32 kind = v >> 29;
    if (kind == 1) {
        return (BOOL)(v & 1);
    }
    if (kind == 2) {
        u32 bits = v << 3;
        f32 f;
        memcpy(&f, &bits, sizeof(f));
        return f > lbl_805FF084;
    }
    return 0;
}

// 0x8005100C
// Starts a conversation: shows it on the HUD when the conversation widget exists and nothing
// blocks it, otherwise remembers the id for later; then resets the exploration prompt / HUD state.
void StartConversationID(u32 a0) {
    fn_8018DEF4(gScreenSets);
    lbl_805FB0E0[0] = 1;
    u8* w = (u8*)ScreenSetFindWidget(ScreenSetsCurrent(gScreenSets), 0xBEF77BAEu);
    s32 show = 0;
    if (ScreenSetFindWidget(ScreenSetsCurrent(gScreenSets), 0xBEF77BAEu) != NULL
        && *(s32*)(lbl_805FBBE0 + 0xA1C) == 0
        && lbl_805FB0E4 != 0
        && WorldGetPhase(gGame + 0xD5F0) != 4) {
        show = 1;
    }
    if (show != 0) {
        *(void**)lbl_805FB0F8 = UIWidgetFindChild(w, 0xF8F01FECu);
        void* second = UIWidgetFindChild(w, 0x9D8A77u);
        UISetText(*(void**)lbl_805FB0F8, NULL, 1);
        UISetText(second, NULL, 1);
        if (WorldGetPhase(gGame + 0xD5F0) == 2) {
            ScreenSetsActivate(gScreenSets, 0x7A1060A7u);
        }
        HudShowConversation(*(void**)(gGame + 0xD634), (s32)a0);
    } else {
        lbl_805FB0EC = a0;
    }
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        fn_800D5C48(*(u8**)(*(u8**)(gGame + 0xD614) + 0xA00), 0);
        if (WorldGetPhase(gGame + 0xD5F0) == 2) {
            ExplorationPromptClear(1);
        } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
            fn_80126808();
            u8* set = *(u8**)((u8*)gScreenSets + 0x88);
            if (set != NULL) {
                u8* el = (u8*)ScreenSetFindWidget(set, 0xE7667FBEu);
                if (el != NULL) {
                    (*(VFnI*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
                }
            }
        }
    } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
        if (WorldGetPhase(gGame + 0xD5F0) == 2) {
            ExplorationPromptClear(1);
        } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
            fn_80126808();
            u8* set = *(u8**)((u8*)gScreenSets + 0x88);
            if (set != NULL) {
                u8* el = (u8*)ScreenSetFindWidget(set, 0xE7667FBEu);
                if (el != NULL) {
                    (*(VFnI*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
                }
            }
        }
    }
    WasButtonPressed(1);
}

// 0x80195104
// Plays the animation on every screen of the current screen set (count at +0x60, screens from +0x40).
void UIEvent_SendToAllScreens(u32 a0) {
    if (gScreenSets == NULL) {
        return;
    }
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_803818A8);
        return;
    }
    s32 count = *(s32*)(set + 0x60);
    u8* p = set;
    for (s32 i = 0; i < count; i++) {
        void* w = *(void**)(p + 0x40);
        if (w == NULL) {
            fn_8018D370(lbl_80381FC8);
        } else {
            UIWidgetPlayAnim(w, a0);
        }
        p += 4;
    }
}

// 0x801952AC
// Plays the animation on the named widget of the current screen set (logs when missing).
void UIEvent_SendToScreen(u32 a0, u32 a1) {
    void* w;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_803818A8);
        w = NULL;
    } else {
        w = ScreenSetFindWidget(set, a1);
        if (w == NULL) {
            fn_8018D370(lbl_803818D0, fn_801EB708(a1));
            w = NULL;
        }
    }
    if (w != NULL) {
        UIWidgetPlayAnim(w, a0);
    }
}

// 0x8019BE40
// Sends a play-animation event (the animation hash) to the widget.
void UIPlayAnim(void* w, u32 anim) {
    u32 ev = anim;
    fn_8019BD18((u8*)w, &ev);
}

// 0x8018EE3C
// Whether the named screen is pushed in the current screen set.
u32 /*0x06*/ UIScreen_Is_Pushed(u32 a0) {
    void* w = NULL;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        w = ScreenSetFindWidget(set, a0);
    }
    if (w == NULL) {
        return 0;
    }
    return (u32)fn_80198C34(set, w);
}

// 0x8012AA50
// Finds the locator with the given hash in the world's locator table (0x20-byte entries; count at
// +0x24, table at +0x28 of the block pointed to by world[0]).
void* WorldFindLocator(u8* world, u32 hash) {
    u8* found = NULL;
    if (hash != 0) {
        u32 i = 0;
        s32 off = 0;
        while (i < *(u32*)(*(u8**)world + 0x24)) {
            u8* table = *(u8**)(*(u8**)world + 0x28);
            if (hash == *(u32*)(table + off)) {
                found = table + off;
                break;
            }
            i++;
            off += 0x20;
        }
    }
    return found;
}

// 0x800D87EC
// Sends an event to the world's event target: through fn_801BBB68 when flag is set, else through
// EventSend.
void WorldSendEvent(void* world, s32 id, int a, int b, int flag) {
    s32 active = *(s32*)((u8*)world + 0x30);
    if (active == 0) {
        active = *(s32*)world;
    }
    if (active == 0) {
        return;
    }
    u8* mgr = *(u8**)(*(u8**)((u8*)world + 0x44) + 0x434);
    if (mgr == NULL) {
        return;
    }
    if (flag != 0) {
        fn_801BBB68(mgr, id, a, b);
    } else {
        EventSend(gEventMgr, mgr, NULL, (u32)id, a, b, 0, 0);
    }
}

// 0x8006AADC
// Per-frame prop update: snaps the prop's anchor point and linked entries to its transform, runs
// the state machine forward, and refreshes its bounding sphere.
void fn_8006AADC(u8* arg0, f32 dt) {
    u8* t = *(u8**)(arg0 + 0x4);
    if (!(*(u32*)(t + 0x20) & 1)) {
        return;
    }
    if (*(u32*)(*(u8**)(arg0 + 0x9C) + 0x8) == 0x1AC481C5u) {
        fn_8006B244(arg0);
    }
    t = *(u8**)(arg0 + 0x4);
    if (*(u32*)(t + 0x20) & 0x04000000) {
        fn_8006A764(arg0, dt);
        f32 half = 0.5f * *(f32*)(arg0 + 0x14);   // lbl_805FD520
        t = *(u8**)(arg0 + 0x4);
        f32 dx = 0.0f * half;                      // lbl_805FD4E8
        f32 dz = 1.0f * half;                      // lbl_805FD508
        f32 x = *(f32*)(t + 0x0) + dx;
        f32 y = *(f32*)(t + 0x4) + dx;
        f32 z = *(f32*)(t + 0x8) + dz;
        *(f32*)(arg0 + 0x18) = x;
        *(f32*)(arg0 + 0x1C) = y;
        *(f32*)(arg0 + 0x20) = z;
        fn_8006F67C(arg0, dt, 0);
        fn_801EC080(arg0 + 0x94, arg0);
        return;
    }
    u8* list = *(u8**)(arg0 + 0xA8);
    if (list != NULL) {
        f32 x = *(f32*)(t + 0x0);
        f32 y = *(f32*)(t + 0x4);
        f32 z = *(f32*)(t + 0x8);
        for (s32 i = 0; i < *(s32*)(list + 0xC); i++) {
            u8* e = *(u8**)(list + 0x14 + i * 4);
            if (e != NULL) {
                *(f32*)(e + 0x10) = x;
                *(f32*)(e + 0x14) = y;
                *(f32*)(e + 0x18) = z;
                *(f32*)(e + 0x1C) = 0.0f;          // lbl_805FD4E8
            }
        }
    }
    if (*(u8*)(arg0 + 0x220) != 0) {
        fn_8006C068(arg0, dt);
    }
    t = *(u8**)(arg0 + 0x4);
    if (*(f32*)(t + 0x24) <= 0.0f && !(*(u32*)(t + 0x20) & 0x20)) {
        s32 n = 0;
        do {
            if (!PropIncrementState(arg0)) {
                break;
            }
            t = *(u8**)(arg0 + 0x4);
            if (*(u32*)(t + 0x20) & 0x20) {
                break;
            }
            if (*(f32*)(t + 0x24) > 0.0f) {
                break;
            }
            n++;
        } while (n < 0xA);
    }
    fn_8006A764(arg0, dt);
    f32 half = 0.5f * *(f32*)(arg0 + 0x14);       // lbl_805FD520
    f32 dz = 0.0f * half;                          // lbl_805FD4E8
    f32 dx = 1.0f * half;                          // lbl_805FD508
    t = *(u8**)(arg0 + 0x4);
    f32 x = *(f32*)(t + 0x0) + dz;
    f32 y = *(f32*)(t + 0x4) + dz;
    f32 z = *(f32*)(t + 0x8) + dx;
    *(f32*)(arg0 + 0x18) = x;
    *(f32*)(arg0 + 0x1C) = y;
    *(f32*)(arg0 + 0x20) = z;
    fn_801EC080(arg0 + 0x94, arg0);
    fn_8006F67C(arg0, dt, 0);
    *(s32*)(arg0 + 0x1A8) = 0;
}

// 0x80147554
// Pad edge query: the button is held in the current register table but not in the spare one.
BOOL fn_80147554(s32 button) {
    if (PadEntryHeld(lbl_805FB494, button)) {
        return 0;
    }
    return PadEntryHeld(lbl_805FB490, button) != 0;
}

// 0x801629D0
// For each of the 16 records (0x360 bytes, four 0xD8-byte sub-entries from +0xC4): sub-entries
// that match the id and are in state 1 or 2 get their flag byte (+0x10 of the sub-entry) set.
void fn_801629D0(u8* arg0, s32 arg1) {
    u8* p = arg0;
    for (s32 i = 0; i < 0x10; i++) {
        if (arg1 == *(s32*)(p + 0xC4) && (u32)(*(s32*)(p + 0xD0) - 1) <= 1u) {
            *(u8*)(p + 0xD4) = 1;
        }
        if (arg1 == *(s32*)(p + 0x19C) && (u32)(*(s32*)(p + 0x1A8) - 1) <= 1u) {
            *(u8*)(p + 0x1AC) = 1;
        }
        if (arg1 == *(s32*)(p + 0x274) && (u32)(*(s32*)(p + 0x280) - 1) <= 1u) {
            *(u8*)(p + 0x284) = 1;
        }
        if (arg1 == *(s32*)(p + 0x34C) && (u32)(*(s32*)(p + 0x358) - 1) <= 1u) {
            *(u8*)(p + 0x35C) = 1;
        }
        p += 0x360;
    }
}

// 0x8018BEA4
// Advances an animation clock: scales the time by the anim length (wrapped into 0..length), stores
// it at +0x24 of the playback state and runs the three update steps (the first only on wrap).
void fn_8018BEA4(u8* arg0, f32 farg0) {
    u8* obj = *(u8**)arg0;
    if (obj == NULL) {
        return;
    }
    u8* st = *(u8**)(obj + 0x14);
    u8* anim = *(u8**)(st + 0x14);
    if (anim == NULL) {
        return;
    }
    f32 t = *(f32*)(anim + 0x20) * farg0;
    f32 prev = *(f32*)(st + 0x24);
    while (t < lbl_805FF830) {
        t += *(f32*)(*(u8**)(st + 0x14) + 0x20);
    }
    u8* a = *(u8**)(st + 0x14);
    while (t > *(f32*)(a + 0x20)) {
        t -= *(f32*)(a + 0x20);
    }
    *(f32*)(st + 0x24) = t;
    *(s32*)(st + 0x1C) = 0;
    if (t < prev) {
        fn_8016A3F8(st);
    }
    fn_8016A43C(st);
    fn_8016A788(st);
}

// 0x801DA758
// Entry pointer at index [+0x50] of the table pointer at +0x58, or NULL.
void* fn_801DA758(void* v) {
    u8* arg0 = (u8*)v;
    u8* table = *(u8**)(arg0 + 0x58);
    if (table == NULL) {
        return NULL;
    }
    void* e = *(void**)(table + *(s32*)(arg0 + 0x50) * 4);
    if (e == NULL) {
        return NULL;
    }
    return e;
}

// 0x801EB7A4
// Pool constructor: clamps the parameters, derives the block size from the item size/alignment and
// builds the free list. Returns the pool.
u8* fn_801EB7A4(u8* arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    *(u8*)(arg0 + 0x20) = (u8)arg4;
    *(s32*)(arg0 + 0x24) = 0;
    if (arg2 < 0x10) {
        arg2 = 0x10;
    }
    if (arg1 < 1) {
        arg1 = 1;
    }
    if (arg1 < arg2) {
        arg1 = arg2;
    }
    if (arg3 < 4) {
        arg3 = 4;
    }
    s32 mask = ~(arg2 - 1);
    s32 total = mask & (arg2 + arg1 + 7);
    *(s32*)(arg0 + 0x8) = arg2;
    *(s32*)(arg0 + 0x4) = total - 8;
    *(s32*)(arg0 + 0x0) = arg3;
    *(s32*)(arg0 + 0x1C) = 0;
    *(s32*)(arg0 + 0x18) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(s32*)(arg0 + 0x14) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x28) = -1;
    fn_801EBAB8(arg0);
    return arg0;
}

// 0x801EB94C
// Pool allocate: takes the head of the free list (of the parent pool at +0x18 when set), links it at
// the tail of the used list and returns its payload (node + 8), or NULL when exhausted.
void* fn_801EB94C(void* p) {
    u8* arg0 = (u8*)p;
    u8* src = *(u8**)(arg0 + 0x18);
    if (src == NULL) {
        src = arg0;
    }
    if (*(u8**)(src + 0x10) == NULL) {
        if (*(u8*)(arg0 + 0x20) != 0) {
            fn_801EBAB8(src);
        } else {
            return NULL;
        }
    }
    u8* tail = *(u8**)(arg0 + 0x14);
    if (tail != NULL) {
        *(u8**)tail = *(u8**)(src + 0x10);
    }
    u8* node = *(u8**)(src + 0x10);
    u8* next = *(u8**)(node + 0x4);
    *(u8**)(node + 0x4) = *(u8**)(arg0 + 0x14);
    *(u8**)(arg0 + 0x14) = *(u8**)(src + 0x10);
    *(u8**)(src + 0x10) = next;
    if (next != NULL) {
        *(u8**)next = NULL;
    }
    *(s32*)(arg0 + 0xC) = *(s32*)(arg0 + 0xC) + 1;
    u8* used = *(u8**)(arg0 + 0x14);
    if (used != NULL) {
        return used + 8;
    }
    return NULL;
}

// 0x801ECFA0
// Releases the 8 effect handles of the given entry (0xC8-byte entries, base at +4, count at +0):
// each non-null handle at +0x88 is returned to the effect manager and the slots cleared.
void fn_801ECFA0(u8* arg0, s32 arg1) {
    if (arg1 >= 0 && arg1 < *(s32*)arg0) {
        s32 base = arg1 * 0xC8;
        s32 off = 0;
        for (s32 i = 0; i < 8; i++) {
            u8* h = *(u8**)(*(u8**)(arg0 + 0x4) + (off + base) + 0x88);
            if (h != NULL) {
                fn_801C83AC(lbl_8047C760, h);
                *(void**)(*(u8**)(arg0 + 0x4) + (off + base) + 0x88) = NULL;
                *(void**)(*(u8**)(arg0 + 0x4) + (off + base) + 0x68) = NULL;
                *(void**)(*(u8**)(arg0 + 0x4) + (off + base) + 0xA8) = NULL;
            }
            off += 4;
        }
    }
}

// 0x801ED6E4
// Resolves a packed zone/index reference (kind in the low nibble of byte 0, kind 5 carries zone in
// byte 1 and index in the low byte of the halfword at +2) to the pointer in the zone's table.
u8* fn_801ED6E4(u8* arg0) {
    u32 kind = 1;
    u8 index = 0;
    u8 zone = 0;
    u8 b = *(u8*)arg0;
    if (!((b >> 4) & 8)) {
        kind = b & 0xF;
    }
    if (kind == 5) {
        u16 h = *(u16*)(arg0 + 2);
        zone = *(u8*)(arg0 + 1);
        index = (u8)h;
    }
    if (ZoneIsLoaded(gWorldState, (s32)zone) != 0) {
        u8* z = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
        return *(u8**)(*(u8**)(z + 0xA8) + ((u32)index << 2));
    }
    return NULL;
}

// 0x801F5714
// Clears bits in the flag word.
void fn_801F5714(u32 mask) {
    lbl_805FB9DC &= ~mask;
}

// 0x801FC0A0
// Screen fade timer: state 1 fades the alpha in over 250 ms, state 2 holds for 1000 ms, state 3
// fades out over 250 ms (alpha is a byte; time in ms since the state started).
void fn_801FC0A0(void) {
    if (lbl_805FBA70 == 0) {
        return;
    }
    u32 ticks = OSGetTick() - lbl_805FBA74;
    f32 t = (f32)(ticks / (OS_TIMER_CLOCK / 1000));
    switch (lbl_805FBA70) {
    case 1:
        lbl_805FBA7C = (u8)(s32)(255.9f * (t / 250.0f));     // lbl_806002E4 / lbl_806002E8
        if (t >= 250.0f) {
            lbl_805FBA74 = OSGetTick();
            lbl_805FBA70 = 2;
            lbl_805FBA7C = 0xFF;
        }
        break;
    case 2:
        lbl_805FBA7C = 0xFF;
        if (t >= 1000.0f) {                                   // lbl_806002EC
            lbl_805FBA74 = OSGetTick();
            lbl_805FBA70 = 3;
        }
        break;
    case 3:
        lbl_805FBA7C = (u8)(s32)(255.9f * ((250.0f - t) / 250.0f));
        if (t >= 250.0f) {
            lbl_805FBA7C = 0;
            lbl_805FBA70 = 0;
        }
        break;
    }
}

// 0x801B4308
// Formats "%s: %s" (buffer, text) into the buffer's tail at +0x60 (0x5F bytes) and terminates it.
void fn_801B4308(u8* arg0, const char* arg1) {
    *(u8*)(arg0 + 0xBF) = 0;
    snprintf((char*)(arg0 + 0x60), 0x5F, lbl_805F9558, (char*)arg0, arg1);
}

// 0x8020A474
// Releases a sound: when the sound has a handle count, finds its voice record (0x28 bytes, 0x100 of
// them), stops the voice and resets the record; then tells the audio manager to release it.
void fn_8020A474(u8* arg0) {
    if (arg0 != NULL && *(u16*)(arg0 + 0xC) != 0) {
        u8* ent = lbl_80559D78;
        for (s32 i = 0; i < 0x100; i++) {
            u8* owner = *(u8**)ent;
            if (owner != NULL && *(u8**)owner == arg0) {
                s32 off = (((u32)i | 0x0AB00000u) & 0xFFFFFu) * 0x28;
                u8* rec = lbl_80559D78 + off;
                s32 handle = *(s32*)(rec + 0x14);
                if (handle != -1) {
                    fn_801FF2A8(gAudio, handle);
                    *(s32*)(rec + 0x14) = -1;
                }
                *(void**)(lbl_80559D78 + off) = NULL;
                *(s32*)(rec + 0x4) = lbl_805FB6B8;
                *(f32*)(rec + 0x8) = 0.0f;               // lbl_806004F0
                *(f32*)(rec + 0xC) = 0.0f;
                *(f32*)(rec + 0x10) = 0.0f;
                *(s32*)(rec + 0x14) = -1;
                *(f32*)(rec + 0x18) = 0.0f;
                *(s32*)(rec + 0x24) = 0;
                *(s32*)(rec + 0x20) = 0;
                *(s32*)(rec + 0x1C) = -1;
                break;
            }
            ent += 0x28;
        }
    }
    if (arg0 != NULL) {
        fn_801FF21C(gAudio, arg0);
    }
}

}  // extern "C"
