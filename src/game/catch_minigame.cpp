// Catch mini-game: two lanes of falling objects and a grabber the player moves with the
// pointer and closes with a button. Ten catches win; running out of time loses.
// Original: CatchMinigame_Update 0x8002E388 (auto_03_8002DBC8_text.s area)
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "game/script_callbacks.h"

extern "C" {
void  CatchMinigame_UpdatePointer(void* g);                          // 0x800223E8
void  CatchMinigame_UpdateBase(void* g, f32 dt);                     // 0x800224C4
void  CatchMinigame_SpawnInLane(void* g, void* lane, f32 delay);     // 0x8002F208
void  CatchMinigame_UpdateObjects(void* g, f32 dt);                  // 0x8002FFBC
void  CatchMinigame_UpdateLane(void* g, void* lane, BOOL active, f32 dt);   // 0x8002EF4C
void  CatchObject_SetLane(void* obj, void* lane);                    // 0x80030348
void  CatchObject_Launch(void* obj, const f32 target[3], f32 speed); // 0x80030414
void  CatchObject_Release(void* obj, s32 a);                         // 0x80030A98
BOOL  PadButtonHeld(s32 button);                                     // 0x801474EC
void  UIWidgetPlayAnimLoop(void* w, u32 anim, s32 n);                // 0x8019BB1C
void  PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);        // 0x80209E70
u32   RandomU32(void);
}

extern u8  gScriptCtx[];    // 0x803C4298 (+0x38 = controller)
extern s32 gScreenWidth;    ORIG(0x805F9388)
extern s32 gScreenHeight;   ORIG(0x805F938C)

enum { GRAB_OPEN = 0, GRAB_CLOSED = 1, GRAB_HOLDING = 2 };
#define CATCHES_TO_WIN  10
#define OBJECTS         10

// Spawn intervals: random in [min, max], divided by the lane's rate.
static const f32 kLeftInterval[2]  = { 2.0f, 2.5f };   // 0x805FC5E8
static const f32 kRightInterval[2] = { 2.0f, 2.5f };   // 0x805FC5F0

// UI widget fields used here: +0x00 image, +0x10 locked, +0x68 position.
struct Widget;
#define W_IMAGE(w)  (*(void**)((u8*)(w) + 0x00))
#define W_LOCKED(w) (*(s32*)((u8*)(w) + 0x10))
#define W_POS(w)    ((f32*)((u8*)(w) + 0x68))

typedef void (*ShowFn)(void*, s32);
static inline void WidgetShow(void* w, s32 mode) {
    (*(ShowFn*)(*(u8**)((u8*)w + 0x138) + 0x2C))(w, mode);
}

#define G(o, T) (*(T*)((u8*)g + (o)))

static inline void SetGrabTexture(u8* g, s32 state) {
    Widget* grab = G(0x27C, Widget*);
    if (W_LOCKED(grab) != 0) return;
    if (state == GRAB_CLOSED)
        UIImage_SetTexture((u32)(uintptr_t)W_IMAGE(grab), UITextureList_GetTexture(0xEC1A6D6E, 3));
    else if (state == GRAB_OPEN || state == GRAB_HOLDING)
        UIImage_SetTexture((u32)(uintptr_t)W_IMAGE(grab), UITextureList_GetTexture(0x7AD719E2, 3));
}

// Send the first free object in a lane towards `target`. Returns its index (OBJECTS if none).
static s32 LaunchFree(u8* g, BOOL left, const f32 target[3], f32 speed) {
    for (s32 i = 0; i < OBJECTS; i++) {
        u8* obj = G(0xEE4 + i * 4, u8*);
        if (obj[0x20] != 0 || *(void**)(obj + 4) == NULL) continue;
        CatchObject_SetLane(obj, g + (left ? 0x90 : 0xC8));
        f32 t[3] = { target[0], target[1], target[2] };
        CatchObject_Launch(obj, t, speed);
        return i;
    }
    return OBJECTS;
}

static void LaunchAtGrabber(u8* g, f32 leftSpeed, f32 rightSpeed) {
    Widget* grab = G(0x27C, Widget*);
    if (G(0xF74, u8)) {
        f32 t[3] = { W_POS(grab)[0], 140.0f, 0.0f };
        LaunchFree(g, TRUE, t, leftSpeed);
    } else {
        f32 t[3] = { W_POS(grab)[0], 336.0f, 0.0f };
        LaunchFree(g, FALSE, t, rightSpeed);
    }
}

static void PlaceMarker(Widget* marker, Widget* at) {
    if (marker == NULL) return;
    W_POS(marker)[0] = W_POS(at)[0];
    W_POS(marker)[1] = W_POS(at)[1];
    WidgetShow(marker, 0x1A);
}

// 0x8002E388
void CatchMinigame_Update(u8* g, f32 dt) {
    CatchMinigame_UpdatePointer(g);
    Widget* cursor = G(0x280, Widget*);
    W_POS(cursor)[0] = G(0xF4C, f32);
    W_POS(cursor)[1] = G(0xF50, f32);
    W_POS(cursor)[2] = G(0xF54, f32);

    if (G(0x6C, f32) <= 0.0f) {
        GameFlowSignal(0x117741C3, 0, 0);                   // out of time
    } else if (G(0xF60, s32) >= CATCHES_TO_WIN) {
        UIElement_SetVisibleFlag(0x9D841CAA, 0);
        UIElement_SetVisibleFlag(0x0CAF9C6F, 0);
        GameFlowSignal((s32)0xC608C842, 0, 0);              // won
        G(0x2A4, s32) = GRAB_OPEN;
        return;
    }

    const f32 halfW = 0.5f * (f32)gScreenWidth;
    const f32 halfH = 0.5f * (f32)gScreenHeight;
    Widget* grab = G(0x27C, Widget*);
    s32 state = G(0x2A4, s32);
    if (state == GRAB_OPEN) {
        // Follow the pointer; the screen half picks the lane.
        G(0xF3C, f32) = 0.0f;
        f32 y = halfH + halfH * (G(0x1B4, f32) * G(0x194, f32));
        if (y > 240.0f) {
            G(0xF74, u8) = 0;
            W_POS(grab)[1] = 326.0f;
        } else {
            G(0xF74, u8) = 1;
            W_POS(grab)[1] = 130.0f;
        }
        W_POS(grab)[0] = halfW + halfW * (G(0x1B0, f32) * G(0x190, f32));
        WidgetShow(grab, 8);
    } else if (state == GRAB_CLOSED) {
        W_POS(grab)[0] = halfW + halfW * (G(0x1B0, f32) * G(0x190, f32));
        f32 target[3] = { W_POS(grab)[0] - 170.0f, W_POS(grab)[1], W_POS(grab)[2] };
        // While the grabber is closed, a new object is thrown at it every 0xF38 seconds.
        if (G(0xF3C, f32) >= G(0xF38, f32)) {
            s32 i = G(0xF74, u8) ? LaunchFree(g, TRUE, target, 2.0f) : LaunchFree(g, FALSE, target, 3.5f);
            u8* obj = G(0xEE4 + i * 4, u8*);    // i == OBJECTS reads the held object (+0xF0C)
            if (obj != NULL) CatchObject_Release(obj, 0);
            G(0xF3C, f32) = 0.0f;
        } else {
            G(0xF3C, f32) += dt;
        }
        W_POS(grab)[1] = G(0xF74, u8) ? 130.0f : 326.0f;
        WidgetShow(grab, 8);
    }

    state = G(0x2A4, s32);
    if (state != GRAB_HOLDING) {
        s32 button = *(s32*)(gScriptCtx + 0x38) * 0x10 + 0x108A;
        if (state != GRAB_CLOSED && PadButtonHeld(button)) {
            TimedRumble(*(s32*)(gScriptCtx + 0x38), 0.2f, 4);
            PlaySound(0x554CB2E7, 0, 0, 0, 0, 0);
            G(0x2A4, s32) = GRAB_CLOSED;
            SetGrabTexture(g, GRAB_CLOSED);
            LaunchAtGrabber(g, 2.5f, 4.0f);
            PlaceMarker(G(0x29C, Widget*), grab);
        } else if (G(0x2A4, s32) != GRAB_OPEN && !PadButtonHeld(button)) {
            G(0x2A4, s32) = GRAB_OPEN;
            SetGrabTexture(g, GRAB_OPEN);
            PlaySound(0x263BDBBA, 0, 0, 0, 0, 0);
            LaunchAtGrabber(g, 2.5f, 4.0f);
            PlaceMarker(G(0x2A0, Widget*), grab);
        }
    } else if (G(0xF70, s32) > 0) {
        // Holding: shake the remote to shake the object loose.
        if (G(0x24C, u8) && G(0x250, f32) > 0.0f) G(0x1C4, f32) = G(0x250, f32);
        if (G(0x24C, u8)) {
            if (W_LOCKED(grab) == 0) UIWidgetPlayAnimLoop(grab, 0x95F48F6E, 10);
            G(0xF70, s32)--;
            PlaySound(0x47F894AB, 0, 0, 0, 0, 0);
        }
    } else {
        void* held = G(0xF0C, void*);
        (*(void (**)(void*))(*(u8**)held + 0x14))(held);
        G(0xF0C, void*) = NULL;
        G(0x2A4, s32) = GRAB_OPEN;
        G(0xF70, s32) = 24;
    }

    CatchMinigame_UpdateBase(g, dt);

    // Spawn timers for both lanes.
    G(0xF40, f32) += dt;
    G(0xF44, f32) += dt;
    if (G(0xF40, f32) > G(0xF58, f32)) {
        G(0xF40, f32) -= G(0xF58, f32);
        f32 r = 2.3283064e-10f * (f32)RandomU32();
        f32 v = kLeftInterval[0] + (kLeftInterval[1] - kLeftInterval[0]) * r;
        G(0xF58, f32) = v / 3.0f;
        CatchMinigame_SpawnInLane(g, g + 0x90, v);
    }
    if (G(0xF44, f32) > G(0xF5C, f32)) {
        G(0xF44, f32) -= G(0xF5C, f32);
        f32 r = 2.3283064e-10f * (f32)RandomU32();
        f32 v = kRightInterval[0] + (kRightInterval[1] - kRightInterval[0]) * r;
        G(0xF5C, f32) = v / 4.5f;
        CatchMinigame_SpawnInLane(g, g + 0xC8, v);
    }

    CatchMinigame_UpdateObjects(g, dt);
    CatchMinigame_UpdateLane(g, g + 0x90, G(0xF74, u8), dt);
    CatchMinigame_UpdateLane(g, g + 0xC8, G(0xF74, u8) == 0, dt);

    Widget* heldMarker = G(0x294, Widget*);
    u8* held = G(0xF0C, u8*);
    if (heldMarker != NULL && held != NULL) PlaceMarker(heldMarker, *(Widget**)(held + 4));

    (*(void (**)(void*))(*(u8**)g + 0x6C))(g);
}
