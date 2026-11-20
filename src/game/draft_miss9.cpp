// Miscellaneous functions from the ninth "missing" draft batch: UI element animation player
// attach/reset/update, UI state-machine stepper (HudUpdateScreens), render display-list flush,
// prop animation start, input-button masks, small light/anim helpers and the UI debug log/alloc.
#include <stdarg.h>
#include <stdio.h>

#include "types.h"
#include "engine/rend.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

typedef u8* (*GetFn)(void*);                                // vtable +0x10 / +0xCC: returns a sub-object
typedef void (*UIVisFn)(void*);                             // UI element (+0x138) vtable +0x10
typedef s32 (*UIFlagFn)(void*, s32);                        // UI element (+0x138) vtable +0x2C / +0x30
typedef void (*UIColorFn)(void*, s32, f32, f32, f32, f32);  // UI element (+0x138) vtable +0x2C with colour
typedef void (*StateFn)(void*, void*);                      // UI state vtable +0x10 / +0x14: (state, machine)
typedef u8* (*StateSubFn)(void*);                           // UI state machine sub-object vtable +0x18

extern "C" {
s32    AnimSetFindClip(void* set, s32 anim);                              // 0x801AC2FC
void   ExplorationPromptClear(BOOL animate);                              // 0x80016EA0
void   GameLog(int level, int category, const char* fmt, ...);            // 0x8000B2F0
void   PropPlayAnim(void* prop, int a, int b, BOOL once);                 // 0x80069350
void*  ScreenSetFindWidget(void* set, u32 hash);                          // 0x80198E34
int    SysSprintf(char* buf, const char* fmt, ...);                       // 0x80142EFC
s32 TriggerDoAction(void* action, void* a1, void* a2);                 // 0x801E0054
s32    WorldGetPhase(void* world);                                        // 0x800D83A8
u8*    fn_80062CE0(u8* p, s32 del);                                       // 0x80062CE0
void   fn_80126808(void);                                                 // 0x80126808
void   fn_80142ED0(u8* base, s32 size);                                   // 0x80142ED0 (DCFlushRange)
void   fn_8015EA40(void* p);                                              // 0x8015EA40
void   fn_8016A3F8(u8* p);                                                // 0x8016A3F8
void   fn_8016A43C(u8* p);                                                // 0x8016A43C
void   fn_8016A788(u8* p);                                                // 0x8016A788
u8*    fn_801842C8(u8* a);                                                // 0x801842C8
void   fn_80184590(u8* a, u8* b);                                         // 0x80184590
void   fn_8018BEA4(u8* p, f32 t);                                         // 0x8018BEA4
void   fn_8018D370(const char* fmt, ...);                                 // 0x8018D370 (debug log)
u8*    fn_8018E808(u8* table, u32 hash);                                  // 0x8018E808
s32    fn_8019BD18(u8* arg0, u32* arg1);                                  // 0x8019BD18
u8*    fn_8019BF64(u8* container, u32 hash);                              // 0x8019BF64
void   fn_8019C348(void* obj);                                            // 0x8019C348
s32 fn_801AB748(u8* p, f32 v);                                       // 0x801AB748
s32    fn_801AB56C(void* h, f32 t);                                       // 0x801AB56C
s32    fn_801AB7D4(void* h);                                              // 0x801AB7D4
f32    fn_801AB7F0(void* h);                                              // 0x801AB7F0
f32    fn_801AB818(void* h);                                              // 0x801AB818
void*  fn_801ABAF4(void* anim);                                           // 0x801ABAF4
void   fn_801ABC50(void* h);                                              // 0x801ABC50
void   fn_801ABCEC(void* h, void* owner);                                 // 0x801ABCEC
void   fn_801B9FA4(void* mgr, void* obj, void* cb, void* ctx);            // 0x801B9FA4
s32 fn_801BB3D4(u8* arg0, s32 arg1, s32 arg2, s32 arg3, u8* arg4);                                                 // 0x801BB3D4 (event callback)
s32 fn_801BB434(void* mgr, s32 id, s32 a, s32 b);                                                 // 0x801BB434 (event callback)
void   fn_801BB8A0(u8* hud);                                              // 0x801BB8A0
void   fn_801BBC9C(u8* hud, u8* sub, u8* state);                          // 0x801BBC9C
void   fn_801BC098(u8* state, u8* hud);                                   // 0x801BC098
void*  fn_801D5584(u32 size, const char* file, s32 line, void* tag, const char* zone, const char* name);   // 0x801D5584
void*  fn_801EEB20(u8* heap, u32 size);                                   // 0x801EEB20
const char* fn_8020E3D8(s32 idx);                                         // 0x8020E3D8
s32    fn_80336BF8(u8* file, const char* fmt, ...);                       // 0x80336BF8
void   operator_delete(void* p);                                          // 0x801D426C

extern u8   __files[];               // 0x803B4978
extern u8   gGame[];                 // 0x803B53A0
extern u8   lbl_80412BF8[];          // 0x80412BF8 (heap)
extern u8   lbl_80412F90[];          // 0x80412F90 (log text buffer)
extern u8*  lbl_805FB608;            // 0x805FB608 (global object pointer)
extern u8   sRendDLs[];              // 0x80401108 (display-list records, 0x94 bytes each)
extern u8   sRendDLRecords[];        // 0x80401ADC (submit records, 0x14 bytes each)
extern s32  gViewportIndex;          // 0x805FB4BC (display list being recorded)
extern s32  gViewportCount;          // 0x805FB4C0 (1 while recording)
extern s32  lbl_805FB4C4;            // 0x805FB4C4 (display-list capacity)
extern s32  gFrameBudget;            // 0x805FB4C8 (bytes still needing a cache flush start here)
extern s32  lbl_805FB4CC;            // 0x805FB4CC
extern u8*  lbl_805FB4D0;            // 0x805FB4D0 (display-list write cursor)
extern s32  lbl_805FB4D4;            // 0x805FB4D4 (defer-submit flag)
extern s32  lbl_805FB4D8;            // 0x805FB4D8 (defer queue head)
extern s32  lbl_805FB4DC;            // 0x805FB4DC (defer queue count)
extern s32  lbl_805FB4E0;            // 0x805FB4E0 (keep-display-lists flag)
extern s32  lbl_805FB4E8;            // 0x805FB4E8 (kept display-list count)
extern s32  lbl_805FB4EC;            // 0x805FB4EC (display-list balance)
extern s32  lbl_805FB594;            // 0x805FB594 (render-thread object)
extern "C++" s32  gRendTexelsInMEM2;       // 0x805F95F8
extern f32  lbl_80376BD8[];          // 0x80376BD8 (vec3)
}
extern "C++" void* gEventMgr;        // 0x805FB6C8
extern "C++" void* gScreenSets;      // 0x805FB654

static const u32 kHashUpdate = 0x68DDF2FEu;   // "update"

extern "C" {

// 0x8019BBA4
// Attaches a new animation player (from anim) to a UI element at priority `level`. If one is
// already attached with a higher priority the call is refused (returns 0); otherwise the current
// player's vtable +0x10 is run first. Returns -2 when the player cannot be allocated.
s32 fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2) {
    u8* cur = *(u8**)(arg0 + 0x10);
    if (cur != NULL) {
        if ((s8)*(u8*)(cur + 0x30) < (s8)arg2) {
            return 0;
        }
        (*(UIVisFn*)(*(u8**)(arg0 + 0x138) + 0x10))(arg0);
    }
    u8* player = (u8*)fn_801ABAF4(arg1);
    if (player == NULL) {
        fn_8018D370("UIElement : anim player failed alloc ...\n");
        return -2;
    }
    fn_801ABCEC(player, arg0);
    *(u8**)(arg0 + 0x10) = player;
    *(u8*)(player + 0x30) = (u8)arg2;
    s32 r = fn_801AB7D4(*(u8**)(arg0 + 0x10));
    if (r == 0) {
        fn_801AB56C(*(u8**)(arg0 + 0x10), 0.0f);
    }
    return r;
}

// 0x8019BC80
// Resets the animation player of a UI element: with flag set it rewinds the player (to its end
// time when the current time is >= 0, otherwise to 0) and re-evaluates it; always runs the
// element's vtable +0x10 afterwards. Returns 0.
s32 fn_8019BC80(u8* arg0, u32 arg1) {
    u8* player = *(u8**)(arg0 + 0x10);
    if (player != NULL) {
        if (arg1 != 0) {
            if (fn_801AB818(player) >= 0.0f) {
                f32 end = fn_801AB7F0(*(u8**)(arg0 + 0x10));
                fn_801AB748(*(u8**)(arg0 + 0x10), end);
            } else {
                fn_801AB748(*(u8**)(arg0 + 0x10), 0.0f);
            }
            fn_801AB56C(*(u8**)(arg0 + 0x10), 0.0f);
        }
        (*(UIVisFn*)(*(u8**)(arg0 + 0x138) + 0x10))(arg0);
    }
    return 0;
}

// 0x8005150C
// Hides the HUD info panel of the current conversation phase: phase 2 clears the exploration
// prompt; phase 1 hides the widget 0xE7667FBE of the HUD screen set.
void ConversationHideHUDInfo(void) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        ExplorationPromptClear(1);
        return;
    }
    if (WorldGetPhase(gGame + 0xD5F0) == 1) {
        fn_80126808();
        u8* set = *(u8**)((u8*)gScreenSets + 0x88);
        if (set != NULL) {
            u8* w = (u8*)ScreenSetFindWidget(set, 0xE7667FBEu);
            if (w != NULL) {
                (*(UIFlagFn*)(*(u8**)(w + 0x138) + 0x30))(w, 2);
            }
        }
    }
}

// 0x801BB5BC
// Per-frame step of a UI state machine (hud): counts down the timer at +0x4C, then (unless the
// pause bits are set) moves from the current state (+0x4) to the pending one (+0xC), running the
// old state's exit (vtable +0x14) and the new state's enter (+0x10), pumps the event queue and
// updates the current state. Flags are at +0x28.
void HudUpdateScreens(u8* arg0, f32 farg0) {
    if (!(*(u32*)(arg0 + 0x28) & 0x800)) {
        *(f32*)(arg0 + 0x4C) = *(f32*)(arg0 + 0x4C) - farg0;
    }
    if (*(f32*)(arg0 + 0x4C) < 0.0f) {
        *(f32*)(arg0 + 0x4C) = 0.0f;
    }
    u32 flags = *(u32*)(arg0 + 0x28);
    if (flags & 0x400) {
        return;
    }
    u32 cleared = flags & ~0x100u;
    *(u32*)(arg0 + 0x28) = cleared;
    if (!(flags & 1)) {
        if (cleared & 0x80) {
            u8* m = *(u8**)arg0;
            s32 next = *(s32*)(arg0 + 0xC);
            bool nextOk = (m != NULL) && (next > -1) && (next < *(s32*)(m + 4));
            if (nextOk) {
                s32 cur = *(s32*)(arg0 + 0x4);
                bool curOk = (m != NULL) && (cur > -1) && (cur < *(s32*)(m + 4));
                if (curOk) {
                    u8* st = *(u8**)m + cur * 0x30;
                    (*(StateFn*)(*(u8**)st + 0x14))(st, arg0);
                }
                s32 n = *(s32*)(arg0 + 0xC);
                *(s32*)(arg0 + 0x4) = n;
                *(s32*)(arg0 + 0xC) = -1;
                u8* ns = **(u8***)arg0 + n * 0x30;
                if (*(u32*)(ns + 0x1C) & 1) {
                    *(s32*)(arg0 + 0xC) = n;
                }
                u8* st = **(u8***)arg0 + *(s32*)(arg0 + 0x4) * 0x30;
                (*(StateFn*)(*(u8**)st + 0x10))(st, arg0);
                *(u32*)(arg0 + 0x28) = *(u32*)(arg0 + 0x28) | 0x30;
            } else {
                u8* sub = m + 0x34;
                u8* state = (*(StateSubFn*)(*(u8**)sub + 0x18))(sub);
                fn_801BBC9C(arg0, sub, state);
            }
            *(u32*)(arg0 + 0x28) = *(u32*)(arg0 + 0x28) & ~0x80u;
        } else {
            u8* sub = *(u8**)arg0 + 0x34;
            u8* state = (*(StateSubFn*)(*(u8**)sub + 0x18))(sub);
            fn_801BBC9C(arg0, sub, state);
        }
        *(u32*)(arg0 + 0x28) = *(u32*)(arg0 + 0x28) | 1;
    }
    *(u32*)(arg0 + 0x28) = (*(u32*)(arg0 + 0x28) | 2) & ~0x3Cu;
    void* queued = *(void**)(arg0 + 0x10);
    fn_801B9FA4(gEventMgr, queued, (void*)(queued != NULL ? (void*)fn_801BB3D4 : (void*)fn_801BB434), arg0);
    s32 count = 0;
    u32 f = *(u32*)(arg0 + 0x28);
    bool again = (f & 0x200) == 0;
    while (again && count < 0x20) {
        *(u32*)(arg0 + 0x28) = *(u32*)(arg0 + 0x28) & ~0x14u;
        fn_801BB8A0(arg0);
        f = *(u32*)(arg0 + 0x28);
        again = false;
        if ((f & 4) && !(f & 0x200)) {
            again = true;
        }
        count++;
    }
    if (!(f & 0x20)) {
        s32 idx = *(s32*)(arg0 + 0x4);
        u8* st = NULL;
        if (idx > -1) {
            st = **(u8***)arg0 + idx * 0x30;
        }
        fn_801BC098(st, arg0);
    }
    *(u32*)(arg0 + 0x28) = *(u32*)(arg0 + 0x28) & ~0x200u & ~0x2u;
}

// 0x8004EED0
// Starts animation (hash) on a prop through its animation set (+0x9C -> +0x48) with the given
// play flag. Returns 1 when the animation started, 0 otherwise (log "Animation for %s not
// present" and a fallback pose when the clip is missing); returns 1 right away when the object
// returned by vtable +0xCC has a non-null pointer at +0x28 whose +0x28 field is set (busy).
s32 fn_8004EED0(u8* arg0, u32 arg1, s32 arg2) {
    s32 result;
    u8* sub = (*(GetFn*)(*(u8**)arg0 + 0xCC))(arg0);
    if (sub != NULL) {
        sub = (*(GetFn*)(*(u8**)arg0 + 0xCC))(arg0);
        if ((sub + 0x28) != NULL) {
            sub = (*(GetFn*)(*(u8**)arg0 + 0xCC))(arg0);
            u8* p = *(u8**)(sub + 0x28);
            if (*(s32*)(p + 0x28) != 0) {
                return 1;
            }
        }
    }
    result = 0;
    u8* anim = *(u8**)(arg0 + 0x9C);
    if (anim != NULL) {
        u8* set = *(u8**)(anim + 0x48);
        if (set != NULL) {
            s32 clip = AnimSetFindClip(set, (s32)arg1);
            PropPlayAnim(arg0, clip, arg2, 0);
            if (clip >= 0) {
                result = 1;
            } else {
                fn_8018BEA4((*(GetFn*)(*(u8**)arg0 + 0x10))(arg0), 1.0f);
                u8* info = (*(GetFn*)(*(u8**)arg0 + 0xCC))(arg0);
                fn_80336BF8(__files + 0xA0, "WARNING - Animation for %s not present\n",
                            fn_8020E3D8(*(s32*)(info + 0x4)));
            }
        }
    }
    return result;
}

// 0x8005CAE8
// Destructor wrapper: runs the base destructor (fn_80062CE0, no delete), then frees p when
// del > 0. Returns p.
u8* fn_8005CAE8(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80122E88
// Maps a button id (0..16) to its input bit mask; other ids give 0.
u32 fn_80122E88(u32 button) {
    u32 mask = 0;
    switch (button) {
    case 1:  mask |= 0x40; break;
    case 2:  mask |= 0x80; break;
    case 3:  mask |= 0x100; break;
    case 4:  mask |= 0x200; break;
    case 5:  mask |= 0x1; break;
    case 6:  mask |= 0x2; break;
    case 7:  mask |= 0x4; break;
    case 8:  mask |= 0x8; break;
    case 9:  mask |= 0x20; break;
    case 10: mask |= 0x10; break;
    case 11: mask |= 0x800; break;
    case 12: mask |= 0x400; break;
    case 13: mask |= 0x3C0; break;
    case 14: mask |= 0xF; break;
    case 15: mask |= 0x30; break;
    case 16: mask |= 0xFFF; break;
    default: break;
    }
    return mask;
}

// 0x8014DE68
// Ends the display list being recorded (flushes its unflushed tail, submits or queues it), advances
// to the next of the 17 display lists (draining the deferred queue when no list is free) and
// begins recording into it. Does nothing when no list is being recorded.
// (The original passes the render-thread object &lbl_805FB594 as a hidden first argument to the
// Rend* thread calls; the repo's prototypes drop it.)
void fn_8014DE68(void) {
    if (gViewportCount == 0) {
        return;
    }
    RendDLEnd((RendDL*)(sRendDLs + gViewportIndex * 0x94));
    s32 unflushed = 0x24000 - gFrameBudget;
    if (unflushed != 0) {
        fn_80142ED0(RendDLGetBase((RendDL*)(sRendDLs + gViewportIndex * 0x94)) + gFrameBudget, unflushed);
    }
    if (lbl_805FB4D4 != 0) {
        lbl_805FB4DC += 1;
    } else {
        RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + gViewportIndex * 0x14), lbl_805FB4E0 == 0);
    }
    gViewportCount = 0;
    lbl_805FB4D0 = NULL;
    lbl_805FB4C4 = 0;
    gFrameBudget = 0;
    lbl_805FB4CC = 0;
    if (lbl_805FB4E0 != 0) {
        lbl_805FB4E8 += 1;
    }
    gViewportIndex = gViewportIndex + 1;
    if (gViewportIndex >= 0x11) {
        gViewportIndex = 0;
    }
    if (lbl_805FB4D4 != 0 && !RendHasFreeDL() && lbl_805FB4D4 != 0) {
        RendThreadSync();
        while (lbl_805FB4DC > 0) {
            RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + lbl_805FB4D8 * 0x14), lbl_805FB4E0 == 0);
            lbl_805FB4D8 = lbl_805FB4D8 + 1;
            if (lbl_805FB4D8 >= 0x11) {
                lbl_805FB4D8 = 0;
            }
            lbl_805FB4DC = lbl_805FB4DC - 1;
        }
        lbl_805FB4D4 = 0;
        lbl_805FB4D8 = 0;
        lbl_805FB4DC = 0;
    }
    RendWaitFreeDL();
    RendDLBegin((RendDL*)(sRendDLs + gViewportIndex * 0x94), (RendDLRecord*)(sRendDLRecords + gViewportIndex * 0x14));
    gViewportCount = 1;
    lbl_805FB4C4 = 0x24000;
    gFrameBudget = 0x24000;
    lbl_805FB4CC = 0;
    lbl_805FB4D0 = RendDLGetBase((RendDL*)(sRendDLs + gViewportIndex * 0x94));
    lbl_805FB4EC -= 1;
}

// 0x80151014
// Same display-list flush as fn_8014DE68 (skipped when nothing is being recorded), then always
// runs fn_8015EA40 on the render-thread object.
void fn_80151014(void) {
    if (gViewportCount != 0) {
        RendDLEnd((RendDL*)(sRendDLs + gViewportIndex * 0x94));
        s32 unflushed = 0x24000 - gFrameBudget;
        if (unflushed != 0) {
            fn_80142ED0(RendDLGetBase((RendDL*)(sRendDLs + gViewportIndex * 0x94)) + gFrameBudget, unflushed);
        }
        if (lbl_805FB4D4 != 0) {
            lbl_805FB4DC += 1;
        } else {
            RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + gViewportIndex * 0x14), lbl_805FB4E0 == 0);
        }
        gViewportCount = 0;
        lbl_805FB4D0 = NULL;
        lbl_805FB4C4 = 0;
        gFrameBudget = 0;
        lbl_805FB4CC = 0;
        if (lbl_805FB4E0 != 0) {
            lbl_805FB4E8 += 1;
        }
        gViewportIndex = gViewportIndex + 1;
        if (gViewportIndex >= 0x11) {
            gViewportIndex = 0;
        }
        if (lbl_805FB4D4 != 0 && !RendHasFreeDL() && lbl_805FB4D4 != 0) {
            RendThreadSync();
            while (lbl_805FB4DC > 0) {
                RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + lbl_805FB4D8 * 0x14), lbl_805FB4E0 == 0);
                lbl_805FB4D8 = lbl_805FB4D8 + 1;
                if (lbl_805FB4D8 >= 0x11) {
                    lbl_805FB4D8 = 0;
                }
                lbl_805FB4DC = lbl_805FB4DC - 1;
            }
            lbl_805FB4D4 = 0;
            lbl_805FB4D8 = 0;
            lbl_805FB4DC = 0;
        }
        RendWaitFreeDL();
        RendDLBegin((RendDL*)(sRendDLs + gViewportIndex * 0x94), (RendDLRecord*)(sRendDLRecords + gViewportIndex * 0x14));
        gViewportCount = 1;
        lbl_805FB4C4 = 0x24000;
        gFrameBudget = 0x24000;
        lbl_805FB4CC = 0;
        lbl_805FB4D0 = RendDLGetBase((RendDL*)(sRendDLs + gViewportIndex * 0x94));
        lbl_805FB4EC -= 1;
    }
    fn_8015EA40(&lbl_805FB594);
}

// 0x80172E84
// Light/plane helper: dot = n . (obj +0x44 vec3) with n = lbl_80376BD8. When |dot| <= 0.0001 the
// vec3 at +0x2C is zeroed, otherwise it is set to (obj +0x54 vec3) / dot.
void fn_80172E84(u8* arg0) {
    f32 x = *(f32*)(arg0 + 0x44);
    f32 y = *(f32*)(arg0 + 0x48);
    f32 z = *(f32*)(arg0 + 0x4C);
    f32 px = x * lbl_80376BD8[0];
    f32 py = y * lbl_80376BD8[1];
    f32 pz = z * lbl_80376BD8[2];
    f32 dot = pz + (px + py);
    if (dot <= 0.0001f && dot >= -0.0001f) {
        *(f32*)(arg0 + 0x2C) = 0.0f;
        *(f32*)(arg0 + 0x30) = 0.0f;
        *(f32*)(arg0 + 0x34) = 0.0f;
        return;
    }
    f32 inv = 1.0f / dot;
    f32 a = *(f32*)(arg0 + 0x54) * inv;
    f32 b = *(f32*)(arg0 + 0x58) * inv;
    f32 c = *(f32*)(arg0 + 0x5C) * inv;
    *(f32*)(arg0 + 0x2C) = a;
    *(f32*)(arg0 + 0x30) = b;
    *(f32*)(arg0 + 0x34) = c;
}

// 0x80173164
// Releases an object: runs fn_80184590 on the owner found by fn_801842C8(+0xC) (if any), clears
// the first word and calls the callback at +4 of the global object lbl_805FB608 with (p, global).
void fn_80173164(void* p) {
    u8* arg0 = (u8*)p;
    u8* owner = fn_801842C8(*(u8**)(arg0 + 0xC));
    if (owner != NULL) {
        fn_80184590(owner, arg0);
    }
    *(s32*)arg0 = 0;
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(arg0, lbl_805FB608);
}

// 0x8018C3D0
// Restarts the animation referenced by *(p) +0x14: sets its time to 0 (wrapped into the clip
// length at clip +0x20), clears +0x1C and runs the three follow-up updates (the first one only
// when the new time went backwards).
void fn_8018C3D0(void* p) {
    u8* anim = *(u8**)(*(u8**)p + 0x14);
    if (anim != NULL) {
        f32 t = 0.0f;
        f32 oldT = *(f32*)(anim + 0x24);
        // The original first wraps up with `while (t < t)` (never runs), then down into the clip length.
        u8* clip = *(u8**)(anim + 0x14);
        while (t > *(f32*)(clip + 0x20)) {
            t -= *(f32*)(clip + 0x20);
        }
        *(f32*)(anim + 0x24) = t;
        *(s32*)(anim + 0x1C) = 0;
        if ((f32)t < oldT) {
            fn_8016A3F8(anim);
        }
        fn_8016A43C(anim);
        fn_8016A788(anim);
    }
}

// 0x8018D1FC
// Debug log (printf style): formats into the shared message buffer lbl_80412F90, then logs it
// with GameLog(1, 1, "DEBUG - %s"). The original builds its va_list from the register save area.
void fn_8018D1FC(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsprintf((char*)lbl_80412F90, fmt, ap);
    va_end(ap);
    GameLog(1, 1, "DEBUG - %s", (const char*)lbl_80412F90);
}

// 0x8018D428
// UI engine allocator: when lbl_805F95F8 > 0 allocates through fn_801D5584 with a
// "ui engine: <tag>" name ("zone_malloc" zone), otherwise from the heap lbl_80412BF8.
void* fn_8018D428(s32 arg0, const char* arg1, const char* arg2, s32 arg3) {
    char name[0x400];
    if (gRendTexelsInMEM2 > 0) {
        SysSprintf(name, "ui engine: %s", arg2);
        return fn_801D5584((u32)arg0, arg1, arg3, (void*)arg2, "zone_malloc", name);
    }
    return fn_801EEB20(lbl_80412BF8, (u32)arg0);
}

// 0x8018DEF4
// Clears the tint of two child widgets (0x0517EA02, 0x9C1EBBB8) of the screen set at +0x7C to
// (0,0,0,0) and hides a third (0x382C6918).
void fn_8018DEF4(u8* arg0) {
    u8* set = *(u8**)(arg0 + 0x7C);
    if (set != NULL) {
        u8* w = fn_8019BF64(set, 0x0517EA02u);
        if (w != NULL) {
            fn_8019BC80(w, 0);
            *(f32*)(w + 0x80) = 0.0f;
            *(f32*)(w + 0x84) = 0.0f;
            *(f32*)(w + 0x88) = 0.0f;
            *(f32*)(w + 0x8C) = 0.0f;
            (*(UIColorFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 0x20, 0.0f, 0.0f, 0.0f, 0.003921569f);
        }
        w = fn_8019BF64(*(u8**)(arg0 + 0x7C), 0x9C1EBBB8u);
        if (w != NULL) {
            fn_8019BC80(w, 0);
            *(f32*)(w + 0x80) = 0.0f;
            *(f32*)(w + 0x84) = 0.0f;
            *(f32*)(w + 0x88) = 0.0f;
            *(f32*)(w + 0x8C) = 0.0f;
            (*(UIColorFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 0x20, 0.0f, 0.0f, 0.0f, 0.003921569f);
        }
        w = fn_8019BF64(*(u8**)(arg0 + 0x7C), 0x382C6918u);
        if (w != NULL) {
            (*(UIFlagFn*)(*(u8**)(w + 0x138) + 0x30))(w, 2);
        }
    }
}

// 0x8018E8D8
// Table lookup by hash: returns the +4 field of the entry found by fn_8018E808, or NULL (also
// when the table is empty or NULL).
void* fn_8018E8D8(void* table, u32 hash) {
    if (table == NULL || *(s32*)table == 0) {
        return NULL;
    }
    u8* entry = fn_8018E808((u8*)table, hash);
    if (entry != NULL) {
        return *(void**)(entry + 0x4);
    }
    return NULL;
}

// 0x80196B4C
// Pushes a 4-float value (v) onto a stack of 32-byte entries at obj +0x408 (count at +0x404): the
// value is stored at entry +0x0; the running product (previous entry's +0x10 vec4 times v, or v
// itself for the first entry) at entry +0x10; then the count is incremented. Returns 0.
s32 fn_80196B4C(u8* arg0, const f32* arg1) {
    u8* e = arg0 + (*(s32*)(arg0 + 0x404) << 5);
    *(f32*)(e + 0x408) = arg1[0];
    *(f32*)(e + 0x40C) = arg1[1];
    *(f32*)(e + 0x410) = arg1[2];
    *(f32*)(e + 0x414) = arg1[3];
    s32 n = *(s32*)(arg0 + 0x404);
    if (n > 0) {
        e = arg0 + (n << 5);
        *(f32*)(e + 0x408) = arg1[0];
        *(f32*)(e + 0x40C) = arg1[1];
        *(f32*)(e + 0x410) = arg1[2];
        *(f32*)(e + 0x414) = arg1[3];
        s32 cur = *(s32*)(arg0 + 0x404);
        u8* prev = arg0 + ((cur - 1) << 5);
        u8* dst = arg0 + (cur << 5);
        f32 m0 = *(f32*)(prev + 0x418) * arg1[0];
        f32 m1 = *(f32*)(prev + 0x41C) * arg1[1];
        f32 m2 = *(f32*)(prev + 0x420) * arg1[2];
        *(f32*)(dst + 0x418) = m0;
        f32 m3 = *(f32*)(prev + 0x424) * arg1[3];
        *(f32*)(dst + 0x41C) = m1;
        *(f32*)(dst + 0x420) = m2;
        *(f32*)(dst + 0x424) = m3;
    } else {
        e = arg0 + (n << 5);
        *(f32*)(e + 0x408) = arg1[0];
        *(f32*)(e + 0x40C) = arg1[1];
        *(f32*)(e + 0x410) = arg1[2];
        *(f32*)(e + 0x414) = arg1[3];
        e = arg0 + (*(s32*)(arg0 + 0x404) << 5);
        *(f32*)(e + 0x418) = arg1[0];
        *(f32*)(e + 0x41C) = arg1[1];
        *(f32*)(e + 0x420) = arg1[2];
        *(f32*)(e + 0x424) = arg1[3];
    }
    *(s32*)(arg0 + 0x404) = *(s32*)(arg0 + 0x404) + 1;
    return 0;
}

// 0x8019B6CC
// Base UI element update: fires the trigger action at +0xB0 when its type bits (31..28) are set,
// sends the "update" event, advances the animation player at +0x10 by t (when it finishes it is
// released and event 0x1BE90661 is sent; vtable +0x2C(0x28) runs either way) and handles the
// pending-hide flag 8 of +0x64. Returns 0.
s32 fn_8019B6CC(u8* arg0, f32 farg0) {
    if ((*(u32*)(arg0 + 0xB0) >> 27) & 0xF) {
        TriggerDoAction(arg0 + 0xB0, NULL, NULL);
    }
    u32 ev = kHashUpdate;
    fn_8019BD18(arg0, &ev);
    if (*(u8**)(arg0 + 0x10) != NULL) {
        if (fn_801AB56C(*(u8**)(arg0 + 0x10), farg0) != 0) {
            fn_801ABC50(*(u8**)(arg0 + 0x10));
            *(u8**)(arg0 + 0x10) = NULL;
            u32 ev2 = 0x1BE90661u;
            fn_8019BD18(arg0, &ev2);
        }
        (*(UIFlagFn*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x28);
    }
    if (*(u32*)(arg0 + 0x64) & 8) {
        fn_8019C348(arg0);
        (*(UIFlagFn*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 8);
        (*(UIFlagFn*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x10);
    }
    return 0;
}

// 0x8019CC7C
// Tail-calls the element's vtable +0x2C (flag set) or +0x30 (flag clear) with 0x200.
s32 fn_8019CC7C(void* el, u32 flag) {
    if (flag != 0) {
        return (*(UIFlagFn*)(*(u8**)((u8*)el + 0x138) + 0x2C))(el, 0x200);
    }
    return (*(UIFlagFn*)(*(u8**)((u8*)el + 0x138) + 0x30))(el, 0x200);
}

// 0x801B94F8
// Input-bit mask for button id (0..22): ids 1..16 give the same mask as fn_80122E88; ids 17..22
// pick a record index (0, 8, 4, 0xC, 0x14, 0x10 for ids 17..22; word at obj + idx * 12 + 0x1C) whose
// value is OR-ed in. Other ids give 0.
u32 fn_801B94F8(void* obj, u32 flags) {
    u32 mask = 0;
    s32 rec = 0x41;
    switch (flags) {
    case 1:  mask |= 0x40; break;
    case 2:  mask |= 0x80; break;
    case 3:  mask |= 0x100; break;
    case 4:  mask |= 0x200; break;
    case 5:  mask |= 0x1; break;
    case 6:  mask |= 0x2; break;
    case 7:  mask |= 0x4; break;
    case 8:  mask |= 0x8; break;
    case 9:  mask |= 0x20; break;
    case 10: mask |= 0x10; break;
    case 11: mask |= 0x800; break;
    case 12: mask |= 0x400; break;
    case 13: mask |= 0x3C0; break;
    case 14: mask |= 0xF; break;
    case 15: mask |= 0x30; break;
    case 16: mask |= 0xFFF; break;
    case 17: rec = 0; break;
    case 18: rec = 8; break;
    case 19: rec = 4; break;
    case 20: rec = 0xC; break;
    case 21: rec = 0x14; break;
    case 22: rec = 0x10; break;
    default: break;
    }
    if (rec != 0x41) {
        mask |= *(u32*)((u8*)obj + rec * 12 + 0x1C);
    }
    return mask;
}

}  // extern "C"
