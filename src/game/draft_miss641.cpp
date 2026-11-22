// Catch minigame (digging) screen update functions in 0x8002xxxx.
#include "types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
void  CatchMinigame_UpdateBase(void* g, f32 dt);                                       // 0x800224C4
void  CatchMinigame_UpdatePointer(void* g);                                            // 0x800223E8
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);                         // 0x80209E70
u32   RandomU32(void);                                                                 // 0x80143460
void* ScreenSetFindElement(void* set, u32 hash);                                       // 0x80198F18
void* ScreenSetsCurrent(void* sets);                                                   // 0x8018D1E4
u32   StringHash(const char* s);                                                       // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);                                     // 0x80142EF8
void  UIPlayAnim(void* el, u32 anim);                                                  // 0x8019BE40
void  fn_80024958(u8* p);                                                              // 0x80024958
void  fn_8002C080(u8* p, u8* a, u8* b);                                                // 0x8002C080
s32   fn_8002C73C(u8* p);                                                              // 0x8002C73C
f32   fn_8002D6E4(u8* p);                                                              // 0x8002D6E4
void  fn_8002D83C(u8* p, f32 dt);                                                      // 0x8002D83C
void  fn_8002DA94(u8* p, f32 dt);                                                      // 0x8002DA94
void  fn_8002DC04(u8* p);                                                              // 0x8002DC04
s32   fn_80031114(void* list, f32* v);                                                 // 0x80031114
u8*    fn_8019552C(u8* list, s32 index);                                                // 0x8019552C
s32   fn_8019555C(u8* list);                                                           // 0x8019555C
void* fn_80198F1C(void* set, u32 hash);                                                // 0x80198F1C
void  fn_801A8654(u8* obj, s32 a1);                                                    // 0x801A8654
s32   fn_801AB80C(u8* obj, f32 rate);                                                  // 0x801AB80C
void  fn_801AA760(u8* out, u8* key);                                                   // 0x801AA760
void* fn_801D40E0(s32 n);                                                              // 0x801D40E0
void  fn_801F916C(f32* out, u8* p, s32 a);                                             // 0x801F916C
void* fn_80209814(void* v);                                                            // 0x80209814
void  fn_8020A398(s32 handle);                                                         // 0x8020A398
s32   fn_8020A9A8(s32 handle);                                                         // 0x8020A9A8
s32   fn_80336BF8(u8* file, const char* fmt, ...);                                     // 0x80336BF8
void  fn_8033B09C(void);                                                               // 0x8033B09C
void  operator_delete(void* p);                                                        // 0x801D426C

extern "C++" void* gEventMgr;       // 0x805FB6C8
extern "C++" void* gScreenSets;     // 0x805FB654
extern u8   __files[];              // 0x803B4978
extern u8*  lbl_805FAFD8;           // 0x805FAFD8 (current catch pointer object)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern f32  lbl_80357748[];         // 0x80357748 (per-slot meter values)
extern const char lbl_80355A6C[];   // 0x80355A6C (format string)

extern f32 lbl_805FC928;            // 0.0f
extern f32 lbl_805FC92C;            // -1.0f
extern f32 lbl_805FC93C;            // 1.0f
extern f32 lbl_805FC944;            // 0.5f
extern f32 lbl_805FC948;            // -0.0001f
extern f32 lbl_805FC94C;            // 0.0001f
extern f32 lbl_805FC950;            // 15.0f
extern f32 lbl_805FC954;            // 2.0f
extern f32 lbl_805FCAA8;            // 0.0f
extern f32 lbl_805FCAD8;            // 2.3283064e-10f (1 / 2^32)
extern f32 lbl_805FCAE8;            // 3.1f
extern f32 lbl_805FCAEC;            // 20.0f
extern f32 lbl_805FCB80;            // 0.0f
extern f32 lbl_805FCB98;            // 0.5f
extern f32 lbl_805FCB9C;            // 0.01f
extern f32 lbl_805FCBA0;            // 1.0f
extern f32 lbl_805FCBA4;            // 2.0f
extern f32 lbl_805FCBB0;            // 0.0f
extern f32 lbl_805FCBC0;            // 1.0f
extern f32 lbl_805FCBC4;            // 0.04f
extern f32 lbl_805FCBC8;            // -0.0001f
extern f32 lbl_805FCBCC;            // 0.0001f
extern f32 lbl_805FCBE4;            // 0.5f

typedef void (*VFnInt)(u8* obj, s32 a);   // second-vtable (+0x138) slots 0x2C / 0x30
typedef void (*VFnSelf)(u8* obj);
typedef s32  (*VFnSelfRet)(u8* obj);

static const u32 kHashNewDigging = 0x9A5E553Bu;  // "UI_MSA_NewDigging"
static const u32 kHashTreasure   = 0x07F6F633u;  // "Treasure"
static const u32 kHashImpact     = 0x34F10781u;  // "impact"

// Creates the voice SFX handle lbl_805FBBAC once (inlined in the asm at every use).
static inline void EnsureVoiceSfxHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// Uniform random float in [0, 1) from the engine RNG.
#define RAND01() (lbl_805FCAD8 * (f32)RandomU32())

// 0x800240E8
// Per-frame catch-pointer logic: resets the pointer object's four colour floats to 1.0, then
// sends the hit/miss events depending on whether the pointer is over the target box (mode 1) or
// idle; also steers the bounce direction at +0x104 and counts bounces at +0xAF0.
void fn_800240E8(u8* arg0) {
    f32 v1[3];
    f32 v2[3];
    u8* ptr = *(u8**)(arg0 + 0x90);
    f32 one = lbl_805FC93C;
    *(f32*)(ptr + 0x80) = one;
    *(f32*)(ptr + 0x84) = one;
    *(f32*)(ptr + 0x88) = one;
    *(f32*)(ptr + 0x8C) = one;
    ptr = *(u8**)(arg0 + 0x90);
    if (*(s32*)(ptr + 0x10) != 0) {
        if (*(u8*)(arg0 + 0xBC) == 1) {
            u8* box = *(u8**)(arg0 + 0x94);
            f32 hw = *(f32*)(box + 0x74) * lbl_805FC944;
            f32 cx = *(f32*)(box + 0x68);
            f32 hh = *(f32*)(box + 0x78) * lbl_805FC944;
            f32 cy = *(f32*)(box + 0x6C);
            f32 px = *(f32*)(ptr + 0x68);
            s32 inside;
            if (px < cx - hw || px > cx + hw) {
                inside = 0;
            } else {
                f32 py = *(f32*)(ptr + 0x6C);
                if (py < cy - hh || py > cy + hh) inside = 0;
                else inside = 1;
            }
            if (inside != 0) {
                EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0x1232869Bu, 0, 0, 1, 0);
                if (*(u8*)(arg0 + 0xAEC) != 0) *(u8*)(arg0 + 0xAEC) = 0;
            }
        }
    } else {
        if (*(u8*)(arg0 + 0xAEC) != 0) {
            EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0x3EADFCE2u, 0, 0, 1, 0);
            *(u8*)(arg0 + 0xAEC) = 0;
        }
        if (*(u8*)(arg0 + 0xFC) == 0) {
            u8* cur = *(u8**)(arg0 + 0x90);
            u8* el = *(u8**)(arg0 + 0x100);
            f32 x = *(f32*)(cur + 0x68);
            f32 y = *(f32*)(cur + 0x6C);
            f32 z = *(f32*)(cur + 0x70);
            *(f32*)(el + 0x68) = x;
            *(f32*)(el + 0x6C) = y;
            *(f32*)(el + 0x70) = z - lbl_805FC93C;
            el = *(u8**)(arg0 + 0x100);
            // the asm leaves the position in f1..f3 / r5 as extra (unused) call arguments
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
            el = *(u8**)(arg0 + 0x100);
            if (*(u8*)(arg0 + 0xFD) != 0) {
                (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
            } else {
                (*(VFnInt*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
            }
            s32 flags = 1;
            if (*(u8*)(arg0 + 0x1A4) != 0) {
                f32 t = *(f32*)(arg0 + 0x1A8);
                if (t > lbl_805FC928) *(f32*)(arg0 + 0x11C) = t;
            }
            if (*(u8*)(arg0 + 0x1A4) != 0) {
                fn_801F916C(v1, arg0 + 0x114, 0);
                if (v1[1] < lbl_805FC928) flags = 1 | 2;
            }
            if (*(u8*)(arg0 + 0x628) != 0) {
                f32 t = *(f32*)(arg0 + 0x62C);
                if (t > lbl_805FC928) *(f32*)(arg0 + 0x5A0) = t;
            }
            if (*(u8*)(arg0 + 0x628) != 0) {
                fn_801F916C(v2, arg0 + 0x598, 0);
                if (v2[0] < lbl_805FC928) flags |= 4;
                else flags |= 8;
            }
            if (flags & 2) {
                EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0x23E85CACu, 0, 0, 1, 0);
                *(u8*)(arg0 + 0xAEC) = 1;
            }
            u8 active = *(u8*)(arg0 + 0xAED);
            if (active == 0 && (flags & 0xC)) {
                if (*(u8*)(arg0 + 0xBC) == 0) {
                    f32 neg = lbl_805FC92C;
                    *(f32*)(arg0 + 0x104) = *(f32*)(arg0 + 0x104) * neg;
                    *(f32*)(arg0 + 0x108) = *(f32*)(arg0 + 0x108) * neg;
                    *(f32*)(arg0 + 0x10C) = *(f32*)(arg0 + 0x10C) * neg;
                }
                f32 dir = *(f32*)(arg0 + 0x104);
                s32 nearZero = 0;
                if (dir > lbl_805FC948 && dir < lbl_805FC94C) nearZero = 1;
                if (nearZero == 0) {
                    dir = *(f32*)(arg0 + 0x104);
                    u8* p = *(u8**)(arg0 + 0x90);
                    f32 w = *(f32*)(p + 0x74);
                    if (dir < lbl_805FC928) {
                        if (w > lbl_805FC928) *(f32*)(p + 0x74) = lbl_805FC92C * w;
                    } else if (dir > lbl_805FC928 && w < lbl_805FC928) {
                        *(f32*)(p + 0x74) = lbl_805FC92C * w;
                    }
                    p = *(u8**)(arg0 + 0x90);
                    (*(VFnInt*)(*(u8**)(p + 0x138) + 0x2C))(p, 0x18);
                    EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0x77CDDFC7u, 0, 0, 1, 0);
                    s32 n = *(s32*)(arg0 + 0xAF0);
                    *(u8*)(arg0 + 0xAED) = 1;
                    *(s32*)(arg0 + 0xAF0) = n + 1;
                }
            } else if (active != 0) {
                EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0x4642C5DAu, 0, 0, 1, 0);
                *(u8*)(arg0 + 0xAED) = 0;
            }
        }
    }
}

// 0x80024528
// Update of the digging screen: shows/hides the Treasure and impact elements, handles the
// round state machine (+0xE8 round, +0xEC timer, +0xF4 wait), starts the next dig after a
// random delay and advances to the next round (fn_80024958) when the timer runs out.
void fn_80024528(u8* arg0, f32 dt) {
    char buf[2000];
    fn_800240E8(arg0);
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (*(u32*)set != kHashNewDigging) return;
    u8* treasure = (u8*)ScreenSetFindElement(set, kHashTreasure);
    ScreenSetFindElement(set, kHashImpact);
    (*(VFnInt*)(*(u8**)(treasure + 0x138) + 0x30))(treasure, 2);
    set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (*(u32*)set == kHashNewDigging) {
        u8* impact = (u8*)ScreenSetFindElement(set, kHashImpact);
        if (*(s32*)(impact + 0x10) == 0) {
            (*(VFnInt*)(*(u8**)(impact + 0x138) + 0x30))(impact, 2);
        }
    }
    if (*(s32*)(arg0 + 0xE8) <= 0) {
        (*(VFnInt*)(*(u8**)(treasure + 0x138) + 0x2C))(treasure, 2);
        if (*(u8*)(arg0 + 0xFC) == 0) {
            *(u8*)(arg0 + 0xFC) = 1;
            for (s32 i = 0; i < 3; i++) {
                u8* el = *(u8**)(arg0 + 0xAC + i * 4);
                (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x1A);
                fn_801A8654(*(u8**)(arg0 + 0xAC + i * 4), 1);
            }
            EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0xC608C842u, 0, 0, 1, 0);
        }
    }
    if (*(f32*)(arg0 + 0x6C) < lbl_805FC928 && *(u8*)(arg0 + 0xFC) == 0) {
        EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0x117741C3u, 0, 0, 1, 0);
    }
    if (*(u8*)(arg0 + 0xFC) != 0) return;
    CatchMinigame_UpdateBase(arg0, dt);
    (*(VFnSelf*)(*(u8**)(arg0 + 4) + 0x6C))(arg0);
    if (*(u8*)(arg0 + 0xBC) == 1) {
        EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0xD404827Eu, 0, 0, 1, 0);
    }
    if (*(s32*)(arg0 + 0xF4) <= 0 && *(u8*)(arg0 + 0xBD) == 1 && *(u8*)(arg0 + 0xBC) == 0) {
        s32 round = *(s32*)(arg0 + 0xE8);
        switch (round) {
        case 4:
            *(s32*)(arg0 + 0xC0) = 0;
            break;
        case 3: {
            u8* el = *(u8**)(arg0 + 0x98);
            *(u8*)(arg0 + 0xBC) = 1;
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
            *(u8**)(arg0 + 0x94) = *(u8**)(arg0 + 0x98);
            break;
        }
        case 2: {
            u8* el = *(u8**)(arg0 + 0x9C);
            *(u8*)(arg0 + 0xBC) = 1;
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
            *(u8**)(arg0 + 0x94) = *(u8**)(arg0 + 0x9C);
            break;
        }
        case 1: {
            u8* el = *(u8**)(arg0 + 0x98);
            *(u8*)(arg0 + 0xBC) = 1;
            *(f32*)(el + 0x6C) = *(f32*)(el + 0x6C) + lbl_805FC950;
            el = *(u8**)(arg0 + 0x98);
            // the asm leaves 15.0f in f1 as an extra (unused) call argument
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x1A);
            *(u8**)(arg0 + 0x94) = *(u8**)(arg0 + 0x98);
            break;
        }
        default:
            // r5 still holds the round number here; passed as the variadic argument
            SysSprintf(buf, lbl_80355A6C, round);
            break;
        }
        *(s32*)(arg0 + 0xAF0) = 0;
    }
    if (*(s32*)(arg0 + 0xC0) <= 0) {
        void* el = fn_80198F1C(ScreenSetsCurrent(gScreenSets), 0xBC4F008Fu);
        if (el != NULL) fn_801AB80C((u8*)el, lbl_805FC954);
        if (*(s32*)(*(u8**)(arg0 + 0x90) + 0x10) == 0) {
            *(u8*)(arg0 + 0xBC) = 0;
            *(u8*)(arg0 + 0xBD) = 1;
            s32 r = rand() % 8;
            *(s32*)(arg0 + 0xC0) = 6;
            *(s32*)(arg0 + 0xF4) = *(s32*)(arg0 + 0xF0) - r;
            EventSend(gEventMgr, *(void**)(arg0 + 0xB8), NULL, 0x1449C953u, 0, 0, 1, 0);
            *(s32*)(arg0 + 0xC4) = *(s32*)(arg0 + 0xC4) + 1;
        }
    }
    if (*(s32*)(arg0 + 0xEC) <= 0 && *(u8*)(arg0 + 0xBC) == 0) {
        s32 wait = *(s32*)(arg0 + 0xF8);
        *(s32*)(arg0 + 0xE8) = *(s32*)(arg0 + 0xE8) - 1;
        *(s32*)(arg0 + 0xEC) = wait;
        if (*(s32*)(arg0 + 0xF4) == wait) *(s32*)(arg0 + 0xEC) = wait + 1;
        fn_80024958(arg0);
    }
}

// 0x800250EC
// Rebuilds the float vector at +0xAF4 (data, +0xAF8 size, +0xAFC capacity) when the "dirt_fling_anim"
// list changed size: for each list entry, stores the distance of its point from the first
// entry's point (std::vector<float>::push_back, including the grow path).
void fn_800250EC(u8* arg0) {
    void* sets = gScreenSets;
    u32 hash = StringHash("dirt_fling_anim");
    u8* list = *(u8**)(*(u8**)((u8*)fn_80198F1C(ScreenSetsCurrent(sets), hash) + 4) + 4);
    s32 count = fn_8019555C(list);
    if (count == *(s32*)(arg0 + 0xAF8)) return;
    *(s32*)(arg0 + 0xAF8) = 0;
    f32 first[4];
    f32 cur[4];
    first[0] = lbl_805FC928;
    first[1] = lbl_805FC928;
    first[2] = lbl_805FC928;
    first[3] = lbl_805FC928;
    for (s32 i = 0; i < count; i++) {
        u8* key = fn_8019552C(list, i);
        if (i == 0) {
            fn_801AA760((u8*)first, key);
        }
        fn_801AA760((u8*)cur, key);
        f32 dx = cur[0] - first[0];
        f32 dy = cur[1] - first[1];
        f32 dz = cur[2] - first[2];
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 len = (len2 <= lbl_805FC928) ? lbl_805FC928 : sqrtf(len2);
        u32 size = *(u32*)(arg0 + 0xAF8);
        u32 cap = *(u32*)(arg0 + 0xAFC);
        if (size < cap) {
            *(s32*)(arg0 + 0xAF8) = (s32)(size + 1);
            *(f32*)(*(u8**)(arg0 + 0xAF4) + size * 4) = len;
            continue;
        }
        // grow path: new capacity about 1.6x / 1.5x the old one, with the MSL checks
        if (0x3FFFFFFFu - cap < 1u) {
            fn_80336BF8(__files + 0xA0, "vector length error");
            fn_8033B09C();
        }
        u32 need = (*(u32*)(arg0 + 0xAF8) + 1) - *(u32*)(arg0 + 0xAFC);
        u32 oldcap = *(u32*)(arg0 + 0xAFC);
        if (need > 0x3FFFFFFFu - oldcap) {
            fn_80336BF8(__files + 0xA0, "vector length error");
            fn_8033B09C();
        }
        u32 newcap;
        if (oldcap < 0x15555555u) {
            u32 g = (((oldcap + 1) * 4) - (oldcap + 1)) / 5;
            newcap = oldcap + (g < need ? need : g);
        } else if (oldcap < 0x2AAAAAAAu) {
            u32 g = (oldcap + 1) >> 1;
            newcap = oldcap + (g < need ? need : g);
        } else {
            newcap = 0x3FFFFFFFu;
        }
        if (newcap > 0x3FFFFFFFu) {
            fn_80336BF8(__files + 0xA0, "vector length error");
            fn_8033B09C();
        }
        u8* nd = (u8*)fn_801D40E0((s32)(newcap * 4));
        if (nd == NULL) {
            fn_80336BF8(__files + 0xA0, "Memory allocation failure");
            fn_8033B09C();
        }
        u32 sz = *(u32*)(arg0 + 0xAF8);
        *(f32*)(nd + sz * 4) = len;
        u8* old = *(u8**)(arg0 + 0xAF4);
        memcpy(nd, old, sz * 4);
        memset(old, 0, sz * 4);
        *(u32*)(arg0 + 0xAF8) = 0;
        *(u32*)(arg0 + 0xAFC) = newcap;
        *(u8**)(arg0 + 0xAF4) = nd;
        *(u32*)(arg0 + 0xAF8) = sz + 1;
        if (old != NULL) operator_delete(old);
    }
}

// 0x80029724
// Picks the spawn slots of the minigame targets: optionally one random point just outside one
// of the four edges of the play area (+0x1C..+0x28 bounds, chosen at random), then random points
// inside the bounds (kept only if fn_80031114 accepts them) until ten are stored at +0x3C (12
// bytes each, z from the pointer object's +0x70). Then fn_8002C080 on the path at +0xB8, and
// every child at +0x104 / +0x114 is placed at the first point (+0x3C/+0x40).
void fn_80029724(u8* arg0) {
    *(s32*)(arg0 + 0xB4) = 0;
    if (*(s32*)(arg0 + 0x18) != 0) {
        f32 x = lbl_805FCAA8;
        f32 y = lbl_805FCAA8;
        s32 side = (s32)(lbl_805FCAA8 + lbl_805FCAE8 * RAND01());
        switch (side % 4) {
        case 0: {
            f32 hi = *(f32*)(arg0 + 0x28);
            x = *(f32*)(arg0 + 0x1C) - lbl_805FCAEC;
            f32 lo = *(f32*)(arg0 + 0x20);
            y = lo + (hi - lo) * RAND01();
            break;
        }
        case 1: {
            f32 hi = *(f32*)(arg0 + 0x28);
            x = lbl_805FCAEC + *(f32*)(arg0 + 0x24);
            f32 lo = *(f32*)(arg0 + 0x20);
            y = lo + (hi - lo) * RAND01();
            break;
        }
        case 2: {
            f32 hi = *(f32*)(arg0 + 0x24);
            f32 lo = *(f32*)(arg0 + 0x1C);
            y = *(f32*)(arg0 + 0x20) - lbl_805FCAEC;
            x = lo + (hi - lo) * RAND01();
            break;
        }
        case 3: {
            f32 hi = *(f32*)(arg0 + 0x24);
            f32 lo = *(f32*)(arg0 + 0x1C);
            y = lbl_805FCAEC + *(f32*)(arg0 + 0x20);
            x = lo + (hi - lo) * RAND01();
            break;
        }
        }
        f32 z = *(f32*)(*(u8**)(arg0 + 0x104) + 0x70);
        u8* slot = arg0 + *(s32*)(arg0 + 0xB4) * 0xC;
        *(f32*)(slot + 0x3C) = x;
        *(f32*)(slot + 0x40) = y;
        *(f32*)(slot + 0x44) = z;
        *(s32*)(arg0 + 0xB4) = *(s32*)(arg0 + 0xB4) + 1;
    }
    while (*(s32*)(arg0 + 0xB4) < 10) {
        f32 z = *(f32*)(*(u8**)(arg0 + 0x104) + 0x70);
        f32 yhi = *(f32*)(arg0 + 0x28);
        f32 ylo = *(f32*)(arg0 + 0x20);
        f32 y = ylo + (yhi - ylo) * RAND01();
        f32 xhi = *(f32*)(arg0 + 0x24);
        f32 xlo = *(f32*)(arg0 + 0x1C);
        f32 x = xlo + (xhi - xlo) * RAND01();
        u8* slot = arg0 + *(s32*)(arg0 + 0xB4) * 0xC;
        *(f32*)(slot + 0x3C) = x;
        *(f32*)(slot + 0x40) = y;
        *(f32*)(slot + 0x44) = z;
        if (fn_80031114(*(void**)(arg0 + 0x124), (f32*)(arg0 + *(s32*)(arg0 + 0xB4) * 0xC + 0x3C)) != 0) {
            *(s32*)(arg0 + 0xB4) = *(s32*)(arg0 + 0xB4) + 1;
        }
    }
    fn_8002C080(arg0 + 0xB8, arg0 + 0x3C, arg0 + 0x48);
    *(s32*)(arg0 + 0xB4) = 2;
    *(f32*)(arg0 + 0x2C) = *(f32*)(arg0 + 0x3C);
    *(f32*)(arg0 + 0x30) = *(f32*)(arg0 + 0x40);
    for (s32 i = 0; i < (*(VFnSelfRet*)(*(u8**)arg0 + 0x14))(arg0); i++) {
        u8* el = *(u8**)(arg0 + 0x104 + i * 4);
        f32 ex = *(f32*)(arg0 + 0x2C);
        f32 ey = *(f32*)(arg0 + 0x30);
        f32 ez = *(f32*)(el + 0x70);
        *(f32*)(el + 0x68) = ex;
        *(f32*)(el + 0x6C) = ey;
        *(f32*)(el + 0x70) = ez;
        el = *(u8**)(arg0 + 0x104 + i * 4);
        (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
    }
    u8* last = *(u8**)(arg0 + 0x114);
    f32 lx = *(f32*)(arg0 + 0x2C);
    f32 ly = *(f32*)(arg0 + 0x30);
    f32 lz = *(f32*)(last + 0x70);
    *(f32*)(last + 0x68) = lx;
    *(f32*)(last + 0x6C) = ly;
    *(f32*)(last + 0x70) = lz;
    last = *(u8**)(arg0 + 0x114);
    (*(VFnInt*)(*(u8**)(last + 0x138) + 0x2C))(last, 0x18);
}

// 0x8002C15C
// Builds a quadratic Bezier path through the midpoints of a, b and c (control point b) into the
// record at arg0: stores the segment midpoints (+0x4, +0x10), the control point (+0x34), the
// start (+0x1C) and then sums the polyline length in 0.01 steps into +0x28; +0x0 is the
// running parameter (reset to 0 at the end).
void fn_8002C15C(u8* arg0, u8* arg1, u8* arg2, u8* arg3) {
    f32 half = lbl_805FCB98;
    f32 sx = *(f32*)(arg1 + 0x0) + *(f32*)(arg2 + 0x0);
    f32 sy = *(f32*)(arg1 + 0x4) + *(f32*)(arg2 + 0x4);
    f32 sz = *(f32*)(arg1 + 0x8) + *(f32*)(arg2 + 0x8);
    f32 m1x = sx * half;
    f32 m1y = sy * half;
    f32 m1z = sz * half;
    *(f32*)(arg0 + 0x4) = m1x;
    *(f32*)(arg0 + 0x8) = m1y;
    *(f32*)(arg0 + 0xC) = m1z;
    f32 tx = *(f32*)(arg2 + 0x0) + *(f32*)(arg3 + 0x0);
    f32 ty = *(f32*)(arg2 + 0x4) + *(f32*)(arg3 + 0x4);
    f32 tz = *(f32*)(arg2 + 0x8) + *(f32*)(arg3 + 0x8);
    *(f32*)(arg0 + 0x10) = tx * half;
    *(f32*)(arg0 + 0x14) = ty * half;
    *(f32*)(arg0 + 0x18) = tz * half;
    *(f32*)(arg0 + 0x34) = *(f32*)(arg2 + 0x0);
    *(f32*)(arg0 + 0x38) = *(f32*)(arg2 + 0x4);
    *(f32*)(arg0 + 0x0) = lbl_805FCB80;
    *(f32*)(arg0 + 0x3C) = *(f32*)(arg2 + 0x8);
    *(f32*)(arg0 + 0x2C) = lbl_805FCB80;
    *(f32*)(arg0 + 0x1C) = m1x;
    *(f32*)(arg0 + 0x20) = m1y;
    *(f32*)(arg0 + 0x24) = m1z;
    f32 px = m1x;
    f32 py = m1y;
    f32 pz = m1z;
    *(f32*)(arg0 + 0x28) = lbl_805FCB80;
    *(f32*)(arg0 + 0x0) = lbl_805FCB9C;
    while (*(f32*)(arg0 + 0x0) <= lbl_805FCBA0) {
        f32 t = *(f32*)(arg0 + 0x0);
        f32 z = *(f32*)(arg0 + 0x24);
        f32 u = lbl_805FCBA0 - t;
        f32 tt = t * t;
        f32 uu = u * u;
        f32 tu2 = (lbl_805FCBA4 * t) * u;
        f32 dz = z - pz;
        f32 x = tt * *(f32*)(arg0 + 0x10) + (uu * *(f32*)(arg0 + 0x4) + tu2 * *(f32*)(arg0 + 0x34));
        f32 y = tt * *(f32*)(arg0 + 0x14) + (uu * *(f32*)(arg0 + 0x8) + tu2 * *(f32*)(arg0 + 0x38));
        f32 dx = x - px;
        f32 dy = y - py;
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 len = (len2 <= lbl_805FCB80) ? lbl_805FCB80 : sqrtf(len2);
        px = x;
        py = y;
        pz = z;
        *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x28) + len;
        *(f32*)(arg0 + 0x0) = *(f32*)(arg0 + 0x0) + lbl_805FCB9C;
    }
    *(f32*)(arg0 + 0x0) = lbl_805FCB80;
}

// 0x8002CF8C
// Per-frame update of the catch minigame: places the five slot elements (+0xF8, stride 0x28),
// ends the round (stops the loop sound, sends an event, hides the element groups) once every slot
// is full or the pointer is below zero; otherwise advances the active slot (fill meter +0x108,
// anim at +0x11C), the timer at +0xEC, and starts/stops the looping voice sound by the catch
// pointer object's speed.
void fn_8002CF8C(u8* arg0, f32 dt) {
    CatchMinigame_UpdatePointer(arg0);
    CatchMinigame_UpdateBase(arg0, dt);
    s32 allFull = 1;
    {
        u8* p = arg0;
        for (s32 i = 0; i < 5; i++) {
            f32 x = *(f32*)(p + 0xF8);
            f32 y = *(f32*)(p + 0xFC);
            f32 z = *(f32*)(p + 0x100);
            u8* el = *(u8**)(p + 0x114);
            *(f32*)(el + 0x68) = x;
            *(f32*)(el + 0x6C) = y;
            *(f32*)(el + 0x70) = z;
            el = *(u8**)(p + 0x114);
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
            if (*(f32*)(p + 0x108) < lbl_805FCBC0) allFull = 0;
            p += 0x28;
        }
    }
    if (allFull != 0 || *(f32*)(arg0 + 0x6C) < lbl_805FCBB0) {
        EnsureVoiceSfxHandle();
        if (fn_8020A9A8(*(s32*)(arg0 + 0x98)) != 0) fn_8020A398(*(s32*)(arg0 + 0x98));
        *(s32*)(arg0 + 0x98) = -1;
        EventSend(gEventMgr, *(void**)(arg0 + 0x9C), NULL, 0x7A104E9Du, 0, 0, 1, 0);
        for (s32 i = 0; i < 3; i++) {
            u8* el = *(u8**)(arg0 + 0x1C0 + i * 4);
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
            fn_801A8654(*(u8**)(arg0 + 0x1C0 + i * 4), 0);
        }
        for (s32 i = 0; i < 5; i++) {
            u8* el = *(u8**)(arg0 + 0x1CC + i * 4);
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x30))(el, 0x1A);
            fn_801A8654(*(u8**)(arg0 + 0x1CC + i * 4), 0);
        }
        return;
    }
    fn_8002DC04(arg0);
    *(f32*)(arg0 + 0xA4) = *(f32*)(arg0 + 0xA4) + dt;
    f32 speed = fn_8002D6E4(arg0);
    *(f32*)(arg0 + 0xAC) = speed;
    s32 nearZero = 0;
    if (speed > lbl_805FCBC8 && speed < lbl_805FCBCC) nearZero = 1;
    {
        u8* el = *(u8**)(arg0 + 0x3D4);
        if (nearZero == 0) {
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        } else {
            (*(VFnInt*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    }
    fn_8002D83C(arg0, dt);
    s32 idx = *(s32*)(arg0 + 0xA8);
    u8* slot = arg0 + idx * 0x28;
    f32 step = dt * (f32)fabs(*(f32*)(arg0 + 0xAC));
    if (*(u8*)(slot + 0x111) != 0) {
        f32 fill = *(f32*)(slot + 0x108) + lbl_805FCBC4 * step;
        *(f32*)(slot + 0x108) = fill;
        if (!(fill < lbl_805FCBC0)) fill = lbl_805FCBC0;
        u8* s2 = arg0 + idx * 0x28;
        *(f32*)(s2 + 0x108) = fill;
        *(f32*)(*(u8**)(s2 + 0x114) + 0x78) = fill * *(f32*)(s2 + 0x10C);
        u32 anim = (u32)fn_8002C73C(s2 + 0xF8);
        if (anim != *(u32*)(s2 + 0x11C) && anim != 0) {
            *(u32*)(s2 + 0x11C) = anim;
            UIPlayAnim(*(void**)(s2 + 0x114), anim);
        }
        u8* el = *(u8**)(s2 + 0x114);
        (*(VFnInt*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
    }
    lbl_80357748[idx] = *(f32*)(arg0 + *(s32*)(arg0 + 0xA8) * 0x28 + 0x108);
    fn_8002DA94(arg0, dt);
    f32 timer = *(f32*)(arg0 + 0xEC) - dt;
    *(f32*)(arg0 + 0xEC) = timer;
    if (!(timer > lbl_805FCBB0)) *(f32*)(arg0 + 0xEC) = lbl_805FCBE4;
    (*(VFnSelf*)(*(u8**)(arg0 + 4) + 0x6C))(arg0);
    if ((f32)fabs(*(f32*)(lbl_805FAFD8 + 0xAC)) > lbl_805FCBB0) {
        EnsureVoiceSfxHandle();
        if (fn_8020A9A8(*(s32*)(arg0 + 0x98)) == 0) {
            EnsureVoiceSfxHandle();
            *(s32*)(arg0 + 0x98) = PlaySound(0xC7D94A2Du, 0, 0, 0, 0, 0);
        }
    } else if (*(s32*)(arg0 + 0x90) == 0) {
        EnsureVoiceSfxHandle();
        if (fn_8020A9A8(*(s32*)(arg0 + 0x98)) != 0) fn_8020A398(*(s32*)(arg0 + 0x98));
        *(s32*)(arg0 + 0x98) = -1;
    }
}

}  // extern "C"
