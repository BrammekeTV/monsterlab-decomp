// Miscellaneous functions from the seventh "missing" draft batch: combat HUD action refresh,
// animation slot handles, UI element colors, button polling, camera/GX setup, shop/experiment
// chatter, drop-shadow style tuning curve, UI list unlink and a deleting destructor.
#include "types.h"
#include "revolution.h"
#include "engine/rend.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

struct View;
extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" void UpdateAllZoneEntities(void);   // 0x80010228

typedef void (*VFnI)(void*, s32);
typedef void (*VFnP)(void*, void*);
typedef void (*VFnIdx)(void*, u32);                               // vtable slot with a u32 argument
typedef u8   (*SlotFn)(void*, s32, f32);                          // animation slot callback (entry +0x6C)

extern "C" {
void   CameraSetMatrix(void* camera, const f32 mtx[3][4]);       // 0x801D6814
void*  EventPost(s32 id, void* sender);                          // 0x801F3918
void   PadConsumeButtons(s32 chan, u32 mask);                    // 0x801B9BC4
u32    PadGetButtons(s32 chan);                                  // 0x801B9B40
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
u32    RandomU32(void);                                          // 0x80143460
void*  ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void*  ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
u32    StringHash(const char* s);                                // 0x80143A5C
s32    fn_8000C30C(u8* ctx);                                     // 0x8000C30C
s32    fn_8003F8D4(void);                                        // 0x8003F8D4
void   fn_8004C718(u8* p, s32 idx, void* elem);                  // 0x8004C718
s32 fn_8004EED0(u8* arg0, u32 arg1, s32 arg2);              // 0x8004EED0
u8* fn_8005CAE8(u8* arg0, s32 arg1);                              // 0x8005CAE8
u8     fn_8006C65C(u8* obj);                                     // 0x8006C65C
s32    fn_8006C664(u8* obj);                                     // 0x8006C664
void   fn_8006C66C(u8* obj, s32 state);                          // 0x8006C66C
void   fn_8006C6AC(u8* obj);                                     // 0x8006C6AC
void*  fn_800E6228(void);                                        // 0x800E6228
u32    fn_80122E88(u32 button);                                  // 0x80122E88
void   fn_801514D4(void* view);                                  // 0x801514D4
void   fn_80154294(void);                                        // 0x80154294
void   fn_80154318(void);                                        // 0x80154318
u8*    fn_8015EC9C(u8* tbl, f32 a, f32 b);                       // 0x8015EC9C
void   fn_8018D2B8(const char* fmt, ...);                        // 0x8018D2B8
void   fn_8018D370(const char* fmt, ...);                        // 0x8018D370
void*  fn_80198F1C(void* set, u32 hash);                         // 0x80198F1C
s32 fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);             // 0x8019BBA4
s32 fn_8019BC80(u8* arg0, u32 arg1);                          // 0x8019BC80
u32    fn_801B94F8(void* obj, u32 flags);                        // 0x801B94F8
void   fn_801D6780(void);                                        // 0x801D6780
void   fn_801E93EC(void* list, void* entry);                     // 0x801E93EC
const char* fn_801EB708(u32 hash);                               // 0x801EB708
u32 fn_801F2C10(u32 key);                                    // 0x801F2C10
void   fn_801F357C(void);                                        // 0x801F357C
void   fn_801F3BB8(void);                                        // 0x801F3BB8
BOOL   fn_801F5220(void);                                        // 0x801F5220
s32    fn_801F525C(void);                                        // 0x801F525C
u32    fn_8020A9A8(s32 handle);                                  // 0x8020A9A8
void   fn_8020B024(f32 v);                                       // 0x8020B024
void   fn_8020B078(f32 v);                                       // 0x8020B078
void   fn_8020E59C(u8* p, s32 del);                              // 0x8020E59C
u8*    fn_8020EA44(u8* p, s32 del);                              // 0x8020EA44
void   fn_8020EB44(u8* p, s32 del);                              // 0x8020EB44
void   operator_delete(void* p);                                 // 0x801D426C

extern u8*  gBattle;                  // 0x805FBBB8
extern u8   gGame[];                  // 0x803B53A0
extern "C++" s32  gPartSlotState[];         // 0x803C52E0 (2 sides x 5 slots of 0x14 bytes' worth: last state per slot)
extern u8   gWorldState[];            // 0x803E6D00 (first word: pointer to the world tuning record)
extern const char lbl_80381868[];     // 0x80381868 (UI script log strings)
extern const char lbl_80382AB4[];     // 0x80382AB4 (log string)
extern u8   lbl_80355940[];           // 0x80355940 (4 chatter sound handles)
extern u8   lbl_80359038[];           // 0x80359038 (vtable)
extern u8   lbl_803C4298[];           // 0x803C4298 (+4: camera object with vtable)
extern u8   lbl_8040D500[];           // 0x8040D500 (shadow tuning curve)
extern u8   lbl_8047C640[];           // 0x8047C640 (camera)
extern u8   lbl_8047C7D8[];           // 0x8047C7D8 (input lock byte)
extern u8   lbl_805F9212;             // 0x805F9212 (copy filter value)
extern u32  lbl_805F9614;             // 0x805F9614
extern u8   lbl_805FB014[];           // 0x805FB014 (global object pointer)
extern u8*  lbl_805FB134;             // 0x805FB134 (experiment object)
extern u8   lbl_805FB50C;             // 0x805FB50C (GX shadow flags byte)
extern void* lbl_805FB538[];          // 0x805FB538 (frame buffer pointers)
extern u8   lbl_805FB89C[];           // 0x805FB89C (float result register)
extern u8*  lbl_805FB8D8;             // 0x805FB8D8 (animation slot table, 0x80 bytes per entry)
extern void* lbl_805FB964;            // 0x805FB964 (movie player)
extern s32  lbl_805FB970;             // 0x805FB970
extern s32  lbl_805FB974;             // 0x805FB974
extern "C++" View* gMainView;         // 0x805FB75C
}

// Animation slots are addressed by a 16-bit handle: low byte = table index (0xFF = invalid),
// high byte = generation (0 = invalid). Returns the 0x80-byte entry or NULL.
static inline u8* SlotFromHandle(u16 h) {
    u32 idx = h & 0xFF;
    if (idx < 0xFF && ((h >> 8) & 0xFF) != 0) return lbl_805FB8D8 + (idx << 7);
    return NULL;
}

static const u32 kHashChatterC = 0x0FE1E172u;   // chatter sound C (no name known)

extern "C" {

// 0x800476D8
// Re-syncs every combat HUD slot (2 sides x 5): resets a slot whose cached state is -1 and that is
// flagged, otherwise pushes the cached state when it differs from the current one.
void CombatHudRefreshActions(void) {
    s32* state = gPartSlotState;
    for (s32 side = 0; side < 2; side++) {
        u8* const* row = *(u8* const**)(gBattle + 0xA00 + side * 4);
        for (s32 i = 0; i < 5; i++) {
            u8* ent = row[i];
            u8* obj = NULL;
            if (ent != NULL) obj = *(u8**)(ent + 0x20);
            if (obj != NULL) obj -= 0x264;
            if (obj != NULL && fn_8006C65C(obj) != 0 && state[i] == -1) {
                fn_8006C6AC(obj);
            } else if (obj != NULL && state[i] != fn_8006C664(obj)) {
                fn_8006C66C(obj, state[i]);
            }
        }
        state += 5;
    }
    UpdateAllZoneEntities();
}

// 0x801E9678
// Looks up the animation slot named by the handle *arg1, runs its pending callback once (marking it
// busy 0x40 then done 0x80) and either flags the list (+6 bit 1) or calls fn_801E93EC for it.
// Returns 1 if the handle was valid, else 0. The list's use count at +8 is held across the call.
s32 fn_801E9678(void* list, void* handle) {
    u8* l = (u8*)list;
    u16 h = *(u16*)handle;
    *(s32*)(l + 0x8) = *(s32*)(l + 0x8) + 1;
    BOOL valid = FALSE;
    u8* e = SlotFromHandle(h);
    if (e != NULL && *(u16*)(e + 0x7C) == h) valid = TRUE;
    if (!valid) {
        *(s32*)(l + 0x8) = *(s32*)(l + 0x8) - 1;
        return 0;
    }
    e = SlotFromHandle(h);
    if (e == NULL) {
        *(s32*)(l + 0x8) = *(s32*)(l + 0x8) - 1;
        return 0;
    }
    if (*(u8*)(e + 0x7F) != 0x80) {
        *(u8*)(e + 0x7F) = 0x40;
        if (*(void**)(e + 0x6C) != NULL) {
            *(f32*)(e + 0x70) = *(f32*)(e + 0x70) + 0.0f;
            *(u8*)(e + 0x7E) = (*(SlotFn*)(e + 0x6C))(e, *(s32*)(e + 0x68), 0.0f);
        }
        *(u8*)(e + 0x7F) = 0x80;
    }
    if (*(u16*)(l + 0x6) & 1) {
        *(u16*)(l + 0x6) = *(u16*)(l + 0x6) | 2;
    } else {
        fn_801E93EC(l, e);
    }
    *(s32*)(l + 0x8) = *(s32*)(l + 0x8) - 1;
    return 1;
}

// 0x801E9D18
// Walks the slot chain starting at the handle in *p (next handle at +0x78) and returns the first
// entry whose flag byte (+0x7F) has a bit of mask and whose key (+0x64) equals key, else NULL.
void* fn_801E9D18(void* p, u32 key, s32 mask) {
    u8* e = SlotFromHandle(*(u16*)p);
    while (e != NULL) {
        if ((mask & *(u8*)(e + 0x7F)) != 0 && key == *(u32*)(e + 0x64)) return e;
        e = SlotFromHandle(*(u16*)(e + 0x78));
    }
    return NULL;
}

// 0x8018C07C
// Length of the animation: the f32 at +0x20 of the clip two links down, or the default constant.
f32 AnimGetLength(void* anim) {
    u8* a = *(u8**)anim;
    if (a != NULL) {
        u8* clip = *(u8**)(*(u8**)(a + 0x14) + 0x14);
        if (clip != NULL) return *(f32*)(clip + 0x20);
    }
    return 0.0f;
}

// 0x8004F398
// Plays the named reaction on the limb.
void LimbPlayReaction(void* limb, const char* name, BOOL special) {
    fn_8004EED0((u8*)limb, StringHash(name), special);
}

// 0x800F4EB4
// During a battle (and outside the state that blocks it) asks the camera controller of the global
// object (+0x4D4, vtable slot 0x84) to play the camera animation a0.
void PlayCameraAnimation(u32 a0) {
    if (gBattle != NULL && *(u8*)(gGame + 0x24) == 0) {
        if (fn_8003F8D4() != 0 && *(s32*)(*(u8**)lbl_805FB014 + 0x4F4) == 1) return;
        u8* g = *(u8**)lbl_805FB014;
        u8* cam = NULL;
        if (g != NULL) cam = *(u8**)(g + 0x4D4);
        if (cam != NULL) (*(VFnIdx*)(*(u8**)cam + 0x84))(cam, a0);
    }
}

// 0x8018FE5C
// Sets the tint (4 floats at a1) of the UI element with hash a0 in the current screen set and
// notifies it through its vtable. (a1 is a pointer to the color; the header types it BOOL.)
void UIElement_SetColor(u32 a0, BOOL a1) {
    const f32* col = (const f32*)(intptr_t)a1;
    u8* el;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        el = NULL;
    } else {
        el = (u8*)ScreenSetFindElement(set, a0);
        if (el == NULL) {
            fn_8018D2B8(lbl_80381868 + 0x238, fn_801EB708(a0));
            el = NULL;
        }
    }
    if (el != NULL) {
        if (col == NULL) {
            fn_8018D370(lbl_80381868 + 0x2BC);
            return;
        }
        f32 r = col[0], g = col[1], b = col[2], a = col[3];
        *(f32*)(el + 0x80) = r;
        *(f32*)(el + 0x84) = g;
        *(f32*)(el + 0x88) = b;
        *(f32*)(el + 0x8C) = a;
        (*(VFnI*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x20);
    }
}

// 0x8018FF54
// Same as UIElement_SetColor with the four components passed directly.
void UIElement_SetColor_Floats(u32 a0, f32 a1, f32 a2, f32 a3, f32 a4) {
    u8* el;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        el = NULL;
    } else {
        el = (u8*)ScreenSetFindElement(set, a0);
        if (el == NULL) {
            fn_8018D2B8(lbl_80381868 + 0x238, fn_801EB708(a0));
            el = NULL;
        }
    }
    if (el != NULL) {
        *(f32*)(el + 0x80) = a1;
        *(f32*)(el + 0x84) = a2;
        *(f32*)(el + 0x88) = a3;
        *(f32*)(el + 0x8C) = a4;
        (*(VFnI*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x20);
    }
}

// 0x8019BB1C
// Starts the looping animation with hash arg1 (looked up in the current screen set) on widget arg0.
void UIWidgetPlayAnimLoop(void* arg0, u32 arg1, s32 arg2) {
    void* anim = fn_80198F1C(ScreenSetsCurrent(gScreenSets), arg1);
    if (anim == NULL) {
        fn_8018D370(lbl_80382AB4, arg1);
    }
    fn_8019BBA4((u8*)arg0, (u8*)anim, (s32)(s8)arg2);
}

// 0x8015DE38
// Returns frame buffer number index.
void* VideoGetFrameBuffer(RendVideo* video, int index) {
    return lbl_805FB538[index];
}

// 0x80122FA0
// Returns the subset of button mask a0 (mapped through the active pad profile) that was just
// pressed, consuming those buttons. 0 while input is locked.
u32 /*0x06*/ WasButtonPressed(u32 /*0x36*/ a0) {
    u32 mask;
    if (*lbl_8047C7D8 != 0) return 0;
    if (fn_8000C30C(lbl_803C4298) != 0) return 0;
    void* prof = fn_800E6228();
    if (prof != NULL) {
        mask = fn_80122E88(a0);
    } else {
        mask = fn_801B94F8(prof, a0);
    }
    u32 hit = mask & PadGetButtons(0);
    if (hit != 0) PadConsumeButtons(0, mask);
    return hit;
}

// 0x801E1BA4
// Returns the script float result register.
f32 fGetResultRegister(void) {
    return *(f32*)lbl_805FB89C;
}

// 0x80009840
// One-time (per frame) UI view setup: marks the view, sets the script camera matrix, clears the
// GX shadow flag bit 3 inside the BP register writes.
void fn_80009840(u8* arg0) {
    if (lbl_805F9614 != 1) {
        lbl_805F9614 = 1;
        fn_801D6780();
        *(s32*)((u8*)gMainView + 0x188) = 1;
        fn_801514D4(gMainView);
        f32 m[3][4];
        u8* cam = *(u8**)(lbl_803C4298 + 4);
        (*(VFnP*)(*(u8**)cam + 0x38))(cam, m);
        CameraSetMatrix(lbl_8047C640, m);
        fn_80154294();
        fn_80154318();
        lbl_805FB50C |= 8;
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = 0xFE00C000u;
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
    }
}

// 0x8000B6A0
// While the movie player is ready: when fn_801F5220 reports a pending skip, decrements the counter
// at +0x134 (arg1 != 0) or +0x130; otherwise (unless in state 6 / byte 0x43) pauses and posts event 0x65.
void fn_8000B6A0(u8* arg0, s32 arg1) {
    BOOL ready = FALSE;
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
        (*(s32 (**)(void*))(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) {
        ready = TRUE;
    }
    if (ready) {
        if (fn_801F5220() != 0) {
            if (arg1 != 0) {
                *(s32*)(arg0 + 0x134) = *(s32*)(arg0 + 0x134) - 1;
                return;
            }
            *(s32*)(arg0 + 0x130) = *(s32*)(arg0 + 0x130) - 1;
            return;
        }
        if (*(s32*)(arg0 + 0x18) != 6 || *(u8*)(arg0 + 0x1D) != 0x43) {
            if (fn_801F525C() == 0) fn_801F357C();
            if (EventPost(0x65, NULL) == NULL) fn_801F2C10(-0x5FC00);
            fn_801F3BB8();
        }
    }
}

// 0x8000B964
// Clears the bits of arg1 in the flags at +0x148, sets bit 1 and clears bit 0.
void fn_8000B964(u8* arg0, s32 arg1) {
    *(s32*)(arg0 + 0x148) = ((*(s32*)(arg0 + 0x148) & ~arg1) | 2) & ~1;
}

// 0x80013F8C
// Maps three 0..10 settings from the record onto the audio/shadow tuning: the first two go to
// fn_8020B024 / fn_8020B078 (as 0..1), the third (+0xC) drives a two-segment curve through the
// world tuning values (+0x1A84 clamped to 1..1.4, +0x1A88 clamped to -0.15..0.15) into table lbl_8040D500.
void fn_80013F8C(u8* arg0) {
    u32 s2 = *(u32*)(arg0 + 0xC);
    u32 c2 = 10;
    if (s2 < 10) c2 = s2;
    f32 t = (f32)c2 / 10.0f;
    u32 s0 = *(u32*)(arg0 + 0x0);
    u32 c0 = 10;
    if (s0 < 10) c0 = s0;
    fn_8020B024((f32)c0 / 10.0f);
    u32 s1 = *(u32*)(arg0 + 0x8);
    u32 c1 = 10;
    if (s1 < 10) c1 = s1;
    fn_8020B078((f32)c1 / 10.0f);

    u8* w = *(u8**)gWorldState;
    f32 a = *(f32*)(w + 0x1A84);
    f32 hi;
    if (a < 1.0f) {
        hi = 1.0f;
    } else if (a > 1.4f) {
        hi = 1.4f;
    } else {
        hi = a;
    }
    f32 b = *(f32*)(w + 0x1A88);
    f32 lo;
    if (b < -0.15f) {
        lo = -0.15f;
    } else if (b > 0.15f) {
        lo = 0.15f;
    } else {
        lo = b;
    }
    if (t < 0.5f) {
        f32 k = t / 0.5f;
        f32 v = 1.4f + k * (hi - 1.4f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, 1.4f, k) + 4) = v;
        k = t / 0.5f;
        v = -0.15f + k * (lo - -0.15f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, -0.15f, k) + 8) = v;
        return;
    }
    f32 k = (t - 0.5f) / 0.5f;
    f32 v = hi + k * (1.0f - hi);
    *(f32*)(fn_8015EC9C(lbl_8040D500, k, 0.5f) + 4) = v;
    k = (t - 0.5f) / 0.5f;
    v = lo + k * (0.15f - lo);
    *(f32*)(fn_8015EC9C(lbl_8040D500, k, 0.5f) + 8) = v;
}

// 0x80023160
// Ambient chatter: when the experiment object's float at +0x6C is >= 2.0 and none of the four
// chatter sounds is still playing, starts chatter sound C into handle slot 2 with 67% chance.
// (The original reads +0x6C even through a NULL object.)
void fn_80023160(void) {
    BOOL ready;
    u8* obj = lbl_805FB134;
    if (obj != NULL && *(s32*)obj != 0) obj = NULL;
    if (*(f32*)(obj + 0x6C) < 2.0f) {
        ready = FALSE;
    } else {
        ready = TRUE;
        for (s32 i = 0; i < 4; i++) {
            if (fn_8020A9A8(*(s32*)(lbl_80355940 + i * 4)) != 0) {
                ready = FALSE;
                break;
            }
        }
    }
    if (ready) {
        u32 r = RandomU32();
        if ((f32)(s32)(r % 100 + 1) < 67.0f) {   // lbl_805FC918
            *(s32*)(lbl_80355940 + 0x8) = PlaySound(kHashChatterC, 0, 0, 0, 0, 0);
        }
    }
}

// 0x80030368
// Unlinks the UI list entry from its doubly linked list (next at +8, prev at +0xC), then resets
// its element (+4): anim 0, y at 690.0, vtable slot 0x2C(0x18), hidden. Returns the old prev.
u8* fn_80030368(u8* arg0) {
    u8* prev = *(u8**)(arg0 + 0xC);
    if (prev != NULL) *(void**)(prev + 0x8) = *(void**)(arg0 + 0x8);
    u8* next = *(u8**)(arg0 + 0x8);
    if (next != NULL) *(void**)(next + 0xC) = *(void**)(arg0 + 0xC);
    *(void**)(arg0 + 0x8) = NULL;
    *(void**)(arg0 + 0xC) = NULL;
    *(u8*)(arg0 + 0x20) = 0;
    fn_8019BC80(*(u8**)(arg0 + 0x4), 0);
    *(f32*)(*(u8**)(arg0 + 0x4) + 0x68) = 690.0f;
    u8* el = *(u8**)(arg0 + 0x4);
    (*(VFnI*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
    UIElement_SetVisibleFlag(**(u32**)(arg0 + 0x4), 0);
    return prev;
}

// 0x8004B754
// Deleting destructor: restores the vtables, clears the 5 slot entries, frees the object at +0xC4C,
// runs the embedded destructors and frees the record if del > 0. Returns p.
u8* fn_8004B754(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(u8**)arg0 = lbl_80359038;
        *(u8**)(arg0 + 0xC48) = lbl_80359038 + 0xDC;
        for (s32 i = 0; i < 5; i++) fn_8004C718(arg0, i, NULL);
        u8* obj = *(u8**)(arg0 + 0xC4C);
        if (obj != NULL) {
            fn_8020E59C(obj, 1);
            *(void**)(arg0 + 0xC4C) = NULL;
        }
        fn_8020EA44(arg0 + 0xC48, 0);
        fn_8020EB44(arg0 + 0x898, -1);
        fn_8005CAE8(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

}  // extern "C"
