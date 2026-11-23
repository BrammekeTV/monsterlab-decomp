// Game mode 0/2/4/8 update, movie hand-off command queue and a UI element state machine.
#include <string.h>

#include "types.h"
#include "engine/engine.h"
#include "game/game.h"

extern "C" {
void  RendClear(int flags, int a, int b, f32 depth);               // 0x801534F0
void  fn_8016FB20(void*);                                          // 0x8016FB20
void  fn_80170044(void*);                                          // 0x80170044
void  fn_801B42B4(void*);                                          // 0x801B42B4
void  fn_801B420C(void*);                                          // 0x801B420C
f32   TimerReset(u8* timer);                                       // 0x80142744
s32   LoadingScreenTick(int, f32);                                 // 0x8000E254
s32   fn_80007D68(void* p);                                        // 0x80007D68
void  fn_80008298(void* p);                                        // 0x80008298
void  fn_80122B9C(u8* p, s32 v);                                   // 0x80122B9C
void* ScreenSetsCurrent(void* sets);                               // 0x8018D1E4
void* ScreenSetGetStrings(void* set);                              // 0x80198E20
void* ScreenSetFindWidget(void* set, u32 hash);                    // 0x80198E34
void* UIAnim_FindByName(void* set, u32 hash);                            // 0x80198F1C
s32   fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);                   // 0x8019BBA4
void  UIEngine_ShowCursor(void* sets, s32 a);                              // 0x8018DBB8
BOOL  ZoneIsLoaded(void* world, s32 zone);                         // 0x8012F408
void  UpdateZoneEntities(s32 zone);                                // 0x800108B8
void  EngineRequestSuspend(int on);                                // 0x801D1830
void  fn_80121254(void*);                                          // 0x80121254
void  SysRequestExit(int mode);                                    // 0x80142DE8
void  fn_801D5F98(f32 dt);                                         // 0x801D5F98
f64   fn_8033F45C(f64 x);                                          // 0x8033F45C (sqrt)
BOOL  fn_801F5220(void);                                           // 0x801F5220
s32   fn_8000AF80(u8* ctx, s32 a, s32 b, s32 c);                   // 0x8000AF80
void  fn_801DFB58(u8* cmd, s32 mode);                              // 0x801DFB58
void  fn_8012AAD8(void* world);                                    // 0x8012AAD8

extern u8  lbl_805FAE80;      // 0x805FAE80 (movie hand-off flag)
extern u8  lbl_805FAEC9;      // 0x805FAEC9
extern u8  lbl_805FAEB0[];    // 0x805FAEB0
extern u32 lbl_805FAEB8;      // 0x805FAEB8 (pad channel)
extern f32 lbl_805FB918;      // 0x805FB918 (frame time step)
extern f32 lbl_805FB91C;      // 0x805FB91C
extern f32 lbl_805FC698;      // 0x805FC698
extern f32 lbl_805FC69C;      // 0x805FC69C
extern f32 lbl_805FC6A0;      // 0x805FC6A0
extern f32 lbl_805FC6A4;      // 0x805FC6A4
extern f32 lbl_805FCAA8;      // 0x805FCAA8
extern f32 lbl_805FCADC;      // 0x805FCADC
}

struct GameSession;
void GameSessionUpdate(GameSession* s, f32 a, f32 dt);   // 0x80008544
void GameUpdateModeTransition(void);                      // 0x8000D148
extern u8  gScriptCtx[];                 // 0x803C4298
extern u8  lbl_8047C7D8[];               // 0x8047C7D8
extern u8  gWorldState[];                // 0x803E6D00
extern "C++" void* gScreenSets;          // 0x805FB654
struct View;
extern "C++" View* gMainView;        // 0x805FB75C
class LoadingAnim;
extern "C++" LoadingAnim* gLoadingAnim;         // 0x805FAECC
extern "C++" u8    gLoadingActive;       // 0x805FAEC8
extern "C++" u8    gLoadingTimer[8];     // 0x805FAEC0
extern "C++" u8    lbl_805F81BC;         // 0x805F81BC
extern "C++" s32   gLastSysEvent;        // 0x805FAEAC

static const u32 kHashLoading = 0x6825C6AEu;   // "Loading"

typedef void (*VFnI)(u8*, s32);
typedef void (*VFnF)(u8*, f32);
typedef s32  (*VFnR)(u8*);
typedef void (*VFnP)(u8*, f32*);

// Calls slot 0x2c / 0x30 of the secondary vtable at +0x138 of an element.
#define ELEM_CALL2C(el, arg) ((*(VFnI*)(*(u8**)((el) + 0x138) + 0x2c))((el), (arg)))
#define ELEM_CALL30(el, arg) ((*(VFnI*)(*(u8**)((el) + 0x138) + 0x30))((el), (arg)))

extern "C" {

// ---------------------------------------------------------------- 0x8000B340

// Pops one 0x18-byte command from the ring buffer at s+0x5c (head), +0x60 (tail), +0x64 (capacity),
// +0x68 (entries); returns FALSE when empty.
static BOOL HandoffQueuePop(u8* s, u8* out) {
    s32 head = *(s32*)(s + 0x5c);
    s32 tail = *(s32*)(s + 0x60);
    if (head < 0 || tail < 0) return FALSE;
    s32 cap = *(s32*)(s + 0x64);
    if (head >= 0 && head < cap) memcpy(out, *(u8**)(s + 0x68) + head * 0x18, 0x18);
    if (tail == head) {
        *(s32*)(s + 0x60) = -1;
        *(s32*)(s + 0x5c) = -1;
    } else {
        *(s32*)(s + 0x5c) = (head + 1) % cap;
    }
    return TRUE;
}

static inline BOOL HandoffQueueEmpty(u8* s) {
    return *(s32*)(s + 0x5c) < 0 || *(s32*)(s + 0x60) < 0;
}

// 0x8000B340
// Movie hand-off state machine (state at +0x50): drains the queued commands, waits for them to
// be consumed, and finally resets the session.
void fn_8000B340(u8* s) {
    if (!fn_801F5220()) return;
    switch (*(s32*)(s + 0x50)) {
    case 1:
        lbl_805FAE80 = 0;
        *(s32*)(s + 0x12c) = 0;
        *(s32*)(s + 0x130) = 0;
        *(s32*)(s + 0x134) = 0;
        if (!HandoffQueueEmpty(s)) {
            fn_8000AF80(s, 1, 1, 0);
            *(s32*)(s + 0x50) = 2;
        } else {
            *(s32*)(s + 0x50) = 0;
        }
        break;
    case 2: {
        u32 v = *(u32*)(s + 0x12c);
        if (v & 1) {
            *(u32*)(s + 0x12c) = v & ~1u;
            *(s32*)(s + 0x50) = 3;
        }
        break;
    }
    case 3: {
        u8 cmd[0x18];
        while (!HandoffQueueEmpty(s)) {
            if (HandoffQueuePop(s, cmd)) {
                fn_801DFB58(cmd, 1);
                *(s32*)(s + 0x130) += 1;
                *(s32*)(s + 0x134) += 1;
            } else {
                *(s32*)(s + 0x50) = 6;
            }
        }
        *(s32*)(s + 0x50) = 4;
        break;
    }
    case 4:
        if (*(s32*)(s + 0x130) <= 0) {
            *(s32*)(s + 0x130) = 0;
            *(s32*)(s + 0x50) = 5;
        }
        break;
    case 5:
        if (*(s32*)(s + 0x134) <= 0) {
            *(s32*)(s + 0x134) = 0;
            if (!HandoffQueueEmpty(s)) *(s32*)(s + 0x50) = 3;
            else                       *(s32*)(s + 0x50) = 6;
            u8 flag = lbl_805FAE80;
            *(s32*)(s + 0x12c) = 0;
            if (flag) lbl_805FAE80 = 0;
        } else if (lbl_805FAE80 == 0) {
            lbl_805FAE80 = 1;
        }
        break;
    case 6:
        fn_8000AF80(s, 0, 0, 0);
        *(s32*)(s + 0x50) = 0;
        *(s32*)(s + 0x12c) = 0;
        *(s32*)(s + 0x130) = 0;
        *(s32*)(s + 0x134) = 0;
        *(s32*)(s + 0x18) = 5;
        fn_8012AAD8(gWorldState);
        break;
    }
}

// ---------------------------------------------------------------- 0x8000D3AC

static void DrawWorldScene(void) {
    RendClear(7, 0, 0, lbl_805FC698);
    fn_8016FB20(gMainView);
    fn_801B42B4(lbl_8047C7D8);
    fn_801B420C(lbl_8047C7D8);
    fn_80170044(gMainView);
}

// Mode 0 sub-state 3 / 5 tail: when a movie is queued, request the next mode (3).
static void RequestModeAfterMovie(void) {
    BOOL empty = TRUE;
    if (gMovieQueue.head >= 0 && gMovieQueue.tail >= 0) empty = FALSE;
    if (empty) return;
    if (gNextGameMode == NEXT_MODE_LOCKED) return;
    if (EngineGetGameSettings()->field_0x38 == 1) gNextGameMode = 3;
    else                                          gNextGameMode = 3;
}

// 0x8000D3AC
// Per-frame body of the game-mode update for modes 0 (script/session run), 2 and 8 (loading
// screen), 4 (world draw).
void fn_8000D3AC(void) {
    lbl_805F81BC = 1;
    GameUpdateModeTransition();
    switch (gGameMode) {
    case 0: {
        ModeState* st = &sModeTable[gGameMode];
        switch (st->sub) {
        case 0:
            DrawWorldScene();
            break;
        case 1:
            lbl_805F81BC = 0;
            TimerReset(gLoadingTimer);
            gLoadingActive = 1;
            if (LoadingScreenTick(0, lbl_805FC698) == 0) {
                fn_80007D68(gScriptCtx);
                fn_80122B9C(gScriptCtx + 0x20, lbl_805FAEB8);
                st = &sModeTable[gGameMode];
                st->unkC &= ~2;
                st->sub = 3;
            }
            break;
        case 2:
            lbl_805F81BC = 0;
            TimerReset(gLoadingTimer);
            gLoadingActive = 1;
            if (LoadingScreenTick(0, lbl_805FC698) == 0) sModeTable[gGameMode].sub = 3;
            break;
        case 3: {
            lbl_805F81BC = 0;
            void* set = ScreenSetsCurrent(gScreenSets);
            BOOL show = TRUE;
            if (set) {
                void* strings = ScreenSetGetStrings(set);
                void* w = ScreenSetFindWidget(set, kHashLoading);
                if (w && strings != w) show = FALSE;
            }
            u8* sets = (u8*)gScreenSets;
            if (*(u8**)(sets + 0x7c) != NULL && show) {
                lbl_805FAEC9 = (*(u32*)(*(u8**)(sets + 0x7c) + 0x64) & 2) == 0;
                UIEngine_ShowCursor(sets, 0);
            }
            if (gLoadingActive == 0) {
                TimerReset(gLoadingTimer);
                gLoadingActive = 1;
                LoadingScreenTick(0, lbl_805FC698);
            } else {
                LoadingScreenTick(0, lbl_805FC69C);
            }
            GameSessionUpdate((GameSession*)gScriptCtx, lbl_805FB91C, lbl_805FB918);
            u32 flags = sModeTable[gGameMode].unkC;
            if (flags & 2) {
                sets = (u8*)gScreenSets;
                if (*(u8**)(sets + 0x7c) != NULL && lbl_805FAEC9 == 0) UIEngine_ShowCursor(sets, 1);
                LoadingScreenTick(0, lbl_805FC6A0);
                void* anim = gLoadingAnim;
                if (anim) (*(VFnI*)(*(u8**)anim + 8))((u8*)anim, 1);   // delete
                gLoadingAnim = NULL;
                EngineRequestSuspend(0);
                st = &sModeTable[gGameMode];
                st->sub = 5;
                st->unkC &= ~6;
                UpdateZoneEntities(0);
                for (s32 z = 1; z <= *(s32*)(*(u8**)gWorldState + 4); z++) {
                    if (ZoneIsLoaded(gWorldState, z)) UpdateZoneEntities(z);
                }
            } else if (flags & 4) {
                EngineRequestSuspend(0);
            } else {
                RequestModeAfterMovie();
            }
            break;
        }
        case 5:
            if (*(s32*)(gScriptCtx + 0x18) == 4) LoadingScreenTick(0, lbl_805FC698);
            else                                 gLoadingActive = 0;
            GameSessionUpdate((GameSession*)gScriptCtx, lbl_805FB91C, lbl_805FB918);
            RequestModeAfterMovie();
            break;
        case 6:
            if (LoadingScreenTick(0, lbl_805FC6A4) == 0) {
                st = &sModeTable[gGameMode];
                st->unkC &= ~2;
                st->sub = 3;
            }
            break;
        case 7:
            st->request = 8;
            RendClear(7, 0, 0, lbl_805FC698);
            break;
        case 8:
            fn_80121254(lbl_805FAEB0);
            GameSessionUpdate((GameSession*)gScriptCtx, lbl_805FB91C, lbl_805FB918);
            break;
        case 9:
            RendClear(7, 0, 0, lbl_805FC698);
            fn_80008298(gScriptCtx);
            sModeTable[gGameMode].sub = 0;
            break;
        }
        break;
    }
    case 8:
        if (sModeTable[gGameMode].sub == 1) {
            EngineRequestSuspend(1);
            RendClear(7, 0, 0, lbl_805FC698);
            sModeTable[8].request = 5;
            SysRequestExit(gLastSysEvent);
        }
        // fall through
    case 2:
        fn_801D5F98(lbl_805FB918);
        gLoadingActive = 1;
        LoadingScreenTick(0, lbl_805FC69C);
        break;
    case 4:
        DrawWorldScene();
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------- 0x8002A158 / 0x8002A31C

// 0x8002A158
// Stores v at +0x70 of the child element (+0x114) and sends it message 0x18.
void fn_8002A158(u8* self, f32 v) {
    *(f32*)(*(u8**)(self + 0x114) + 0x70) = v;
    u8* el = *(u8**)(self + 0x114);
    ELEM_CALL2C(el, 0x18);
}

// True when the circle at (+0x2c, +0x30) with radius +0x34 overlaps the rectangle [c, d].
static inline BOOL CircleInRect(u8* self, f32* c, f32* d) {
    f32 x = *(f32*)(self + 0x2c);
    f32 r = *(f32*)(self + 0x34);
    if (x - r > d[0]) return FALSE;
    if (x + r < c[0]) return FALSE;
    f32 y = *(f32*)(self + 0x30);
    if (y - r > d[1]) return FALSE;
    if (y + r < c[1]) return FALSE;
    return TRUE;
}

// Switch to state 5: tell every child element (message 0x1a), set the final element's
// parameter and message 0x3a, then run the element's update (vtable slot 0xc).
static void EnterState5(u8* self, f32 dt) {
    *(s32*)(self + 0x18) = 5;
    u8* p = self;
    for (s32 i = 0; i < (*(VFnR*)(*(u8**)self + 0x14))(self); i++) {
        u8* child = *(u8**)(p + 0x104);
        ELEM_CALL30(child, 0x1a);
        p += 4;
    }
    *(f32*)(*(u8**)(self + 0x114) + 0x8c) = lbl_805FCADC;
    u8* el = *(u8**)(self + 0x114);
    ELEM_CALL2C(el, 0x3a);
    (*(VFnF*)(*(u8**)self + 0xc))(self, dt);
}

// 0x8002A31C
// UI element state machine (state at +0x18). a points at the element's countdown amount, b is
// the target point, c/d the min/max corners of the capture rectangle.
void fn_8002A31C(u8* self, u8* a, u8* b, u8* c, u8* d, f32 dt) {
    switch (*(s32*)(self + 0x18)) {
    case 1: {
        s32 idx = *(s32*)(self + 0x110);
        *(s32*)(self + 0x18) = 2;
        u8* child = *(u8**)(self + 0x104 + idx * 4);
        ELEM_CALL2C(child, 0x18);
        break;
    }
    case 2: {
        s32 idx = *(s32*)(self + 0x110);
        *(s32*)(self + 0x18) = 3;
        u8* child = *(u8**)(self + 0x104 + idx * 4);
        ELEM_CALL2C(child, 2);
        break;
    }
    case 5:
        *(s32*)(self + 0x18) = 6;
        break;
    case 7: {
        f32 k = lbl_805FCAA8;
        f32 t = *(f32*)(self + 0x10) - dt;
        if (k > t) t = k;
        *(f32*)(self + 0x10) = t;
        if (t <= lbl_805FCAA8) {
            void* set = ScreenSetsCurrent(gScreenSets);
            void* w = UIAnim_FindByName(set, 0x01CEB043u);
            fn_8019BBA4(*(u8**)(self + 0x114), (u8*)w, 10);
            *(s32*)(self + 0x18) = 8;
        }
        u8* el = *(u8**)(self + 0x114);
        ELEM_CALL2C(el, 0x38);
        break;
    }
    case 3: {
        s32 type = *(s32*)(*(u8**)(self + 0x11c) + 0x4c);
        if (type == 4 && CircleInRect(self, (f32*)c, (f32*)d)) {
            EnterState5(self, dt);
            break;
        }
        if (type == 4) {
            f32 dx = *(f32*)(self + 0x2c) - ((f32*)b)[0];
            f32 dy = *(f32*)(self + 0x30) - ((f32*)b)[1];
            f32 dist = (f32)fn_8033F45C(dx * dx + dy * dy);
            if (dist < *(f32*)(self + 0xc)) *(s32*)(self + 0x18) = 4;
        }
        (*(VFnF*)(*(u8**)self + 0xc))(self, dt);
        break;
    }
    case 4: {
        s32 type = *(s32*)(*(u8**)(self + 0x11c) + 0x4c);
        if (type == 4 && CircleInRect(self, (f32*)c, (f32*)d)) {
            EnterState5(self, dt);
            break;
        }
        f32 x = *(f32*)(self + 0x2c);
        f32 y = *(f32*)(self + 0x30);
        f32 nx = x + (x - ((f32*)b)[0]);
        f32 ny = y + (y - ((f32*)b)[1]);
        f32 lo = *(f32*)(self + 0x1c);
        if (nx >= lo) lo = nx;
        f32 outX = *(f32*)(self + 0x24);
        if (lo <= outX) outX = lo;
        f32 lo2 = *(f32*)(self + 0x20);
        if (ny >= lo2) lo2 = ny;
        f32 outY = *(f32*)(self + 0x28);
        if (lo2 <= outY) outY = lo2;
        s32 idx = *(s32*)(self + 0x110);
        f32 pos[3];
        pos[0] = outX;
        pos[1] = outY;
        pos[2] = *(f32*)(*(u8**)(self + 0x104 + idx * 4) + 0x70);
        (*(VFnP*)(*(u8**)self + 0x20))(self, pos);
        *(s32*)(self + 0x18) = 3;
        (*(VFnF*)(*(u8**)self + 0xc))(self, dt);
        break;
    }
    case 8: {
        f32 t = *(f32*)(self + 0x14) - dt;
        *(f32*)(self + 0x14) = t;
        if (t > lbl_805FCAA8) break;
        if (*(f32*)a <= lbl_805FCADC) break;
        if ((*(VFnR*)(*(u8**)self + 0x10))(self) != 0) *(f32*)a = *(f32*)a - lbl_805FCADC;
        break;
    }
    default:
        break;
    }
}

}  // extern "C"
