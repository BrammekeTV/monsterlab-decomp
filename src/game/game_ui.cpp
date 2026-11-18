// Game-side UI glue: screen manager, screen sets per world zone, HUD fade timer.
// Original object: .text 0x8000C7C8 - 0x8000CF6C (auto_03_8000C7C8_text.s)
#include <stdint.h>
#include <string.h>

#include "engine/engine.h"
#include "game/game.h"

extern "C" {
void  fn_80074D60(int);
void  EventSend(void* mgr, void* screens, int a, s32 id, s32 b, s32 c, int d, int e);   // 0x801BA190
void  ScreenMgrUpdate(void* screens, f32 dt);           // 0x801BB5BC
void  EventMgrCtor(void* mgr);                         // 0x801B9C0C
void  ScreenMgrLoad(void* screens);                     // 0x801BC574
void  HudFadeOut(void* hud);                            // 0x8001B128
void  HudFadeIn(void* hud);                             // 0x8001B164
void  HudShowConversation(void* hud, s32 conversation);  // 0x8001B0B8
void  HudInit(void* hud, void* screens);                // 0x8001AFB4
void  HudReset(void* hud);                              // 0x8001AFCC
BOOL  InputIsBusy(void* input);                         // 0x801432FC
s32   InputGetPlayer(void* input);                      // 0x80143310
s32   InputGetPlayer2(void* input);                     // 0x80143330
s32   PlayerIndexOf(s32 player);                        // 0x801EB704
s32   WorldGetPhase(void* world);                       // 0x800D83A8
BOOL  fn_8003CF40(void);
void  GameStateInit(void* game);                        // 0x80006CA8
void  fn_8019EA7C(void* table);
void  ScreenSetsInit(void* sets);                       // 0x8018C8F4
void  fn_801ABA2C(void);
void  ScreenSetsLoadCsv(const char* path);              // 0x801091B4
void  GameSetNextMode(s32 mode);                        // 0x8000CFD8
void* ScreenSetsCurrent(void* sets);                    // 0x8018D1E4
void  ScreenSetsSelect(void* sets, s32 id);             // 0x8018D4EC
void  ScreenSetRelease(void* set);                      // 0x80198C78
void  SetLoadingIcon(BOOL on);                          // 0x800E6A84
void  ScreenSetsBind(void* sets, s32 a, s32 b);         // 0x8018D524
void  ZoneLoadScreens(void* world, s32 res, void* arg); // 0x801330D4
void* ResourceFind(void* list, s32 id, int flags);      // 0x801EBE74
void  ScreenSetsFlush(void* sets);                      // 0x8018DCA4
void* ScreenSetsFind(void* sets, s32 zone);             // 0x8018D1EC
s32   ResourceGetCurrentId(void);                       // 0x801D3A60
void  ZoneUnload(void* world, s32 id, BOOL now);        // 0x80131A18
void  ZoneSetCurrent(void* world, s32 zone);            // 0x80133840
void  ZoneLoadAsync(void* world, s32 zone, int a, void (*done)(void*, void*), s32 arg);   // 0x80132098
BOOL  ZoneIsLoaded(void* world, s32 zone);              // 0x8012F408
void  ScriptCall(void* ctx, s32 id, s32 arg);           // 0x8000B7A8
void  ScriptSetVar(void* ctx, s32 var, s32 value);      // 0x8000B924
s32   ScriptGetVar(void* ctx, s32 var);                 // 0x8000B964
void  ScreenSetsPush(void* sets, s32 id);               // 0x8018D70C
void  ScreenSetsPop(void* sets, s32 id);                // 0x8018DBB8
s32   ScreenSetsCount(void* sets);                      // 0x8018D6C8
}

// Pointers patched into the global_binary.sir header at load time.
#define SIR_PTR(off) (*(void**)(sSirHeader + (off)))
#define SIR_SCREENS  SIR_PTR(0xD98)   // screen list
#define SIR_HUD      SIR_PTR(0xD9C)

struct ScreenList {          // SIR_SCREENS
    struct { u8* entries; }* table;   // 0x00: entries are 0x30 bytes
    s32 current;                      // 0x04
    u8  pad08[0x20];
    u32 flags;                        // 0x28
};

// ---- data ----
extern s32   gUiActive;                         ORIG(0x805F81A8)
s32          gUiQuitRequested;                  ORIG(0x805FAE88)
s32          gUiFlag8C;                         ORIG(0x805FAE8C)
void*        gPendingScreens;                   ORIG(0x805FAE90)
s32          gPendingScreenId;                  ORIG(0x805FAE94)
s32          gPendingScreenArg1;                ORIG(0x805FAE98)
s32          gPendingScreenArg2;                ORIG(0x805FAE9C)
f32          gPendingScreenTimer;               ORIG(0x805FAEA0)
void*        gEventMgr;                        ORIG(0x805FB6C8)
void*        gScreenSets;                       ORIG(0x805FB654)
extern void* lbl_805FB658; extern void* lbl_805FB660; extern void* lbl_805FB670;
extern u32   gGlobalBinaryField844;             // 0x805FB888
extern void* gStringTable;                      // 0x805FB910
extern u8    gGame[];                           // 0x803B53A0
extern u8    gCamera[];                         // 0x8047C640
extern u8    gWorldState[];                     // 0x803E6D00
extern u8    gSirLoader[];                      // 0x8048A8F0
extern u8    gInput[];                          ORIG(0x803F22D8)
extern u8    gScriptCtx[];                      ORIG(0x803C4298)
extern u8    lbl_80359420[];
extern s32   gZoneResourceSize;                 ORIG(0x805FB94C)
extern void* gResourceDb;                       ORIG(0x805FB218)

static const f32 kZero = 0.0f;                  ORIG(0x805FC690)

// 0x8000C7C8
void fn_8000C7C8(void) {
    fn_80074D60(1);
}

// 0x8000C7D0
// Queue a screen to show after `delay` seconds.
void GameFlowDelayedSignal(s32 id, s32 arg1, s32 arg2, f32 delay) {
    gPendingScreenId = id;
    gPendingScreenArg1 = arg1;
    gPendingScreens = SIR_SCREENS;
    gPendingScreenArg2 = arg2;
    gPendingScreenTimer = delay;
}

// 0x8000C7F4
extern "C" void GameFlowSignal(s32 id, s32 arg1, s32 arg2) {
    EventSend(gEventMgr, SIR_SCREENS, 0, id, arg1, arg2, 0, 0);
}

// 0x8000C820
void SetWorldRender(s32 quit) {
    gUiQuitRequested = quit;
}

// 0x8000C828
BOOL GameFlowStopped(void) {
    return gUiActive == 0;
}

// 0x8000C838
void UiUpdate(f32 dt) {
    if (gUiActive != 0) {
        if (gPendingScreenTimer > kZero) {
            gPendingScreenTimer -= dt;
            if (gPendingScreenTimer <= kZero) {
                EventSend(gEventMgr, SIR_SCREENS, 0, gPendingScreenId, 0, 0, 0, 0);
                gPendingScreenTimer = kZero;
            }
        }

        u8* hud = (u8*)SIR_HUD;
        if (*(s32*)(hud + 0x42C) != 0) HudFadeOut(hud);
        else if (hud[0x430])           HudFadeIn(hud);

        ScreenMgrUpdate(SIR_SCREENS, dt);
    }

    if (gUiActive == 0 && !InputIsBusy(gInput)) gUiQuitRequested = 1;
}

// 0x8000C940
void GameFlowSleep(void) {
    ((ScreenList*)SIR_SCREENS)->flags |= 0x200;
}

// 0x8000C95C
void GameFlowPush(s32 mode) {
    HudShowConversation(SIR_HUD, mode);
}

// 0x8000C970
void GameFlowPop(void) {
    ((u8*)SIR_HUD)[0x430] = 1;
}

// 0x8000C988
BOOL IsPlayerLoaded(void) {
    if (*(s32*)(gScriptCtx + 0x18) != 5) return FALSE;
    if (WorldGetPhase(gGame + 0x10000 - 0x2A10) == 6) return fn_8003CF40();
    return TRUE;
}

// 0x8000C9E8
void UiInit(void) {
    gEventMgr = MemAllocDefault(0xC2C);
    if (gEventMgr != NULL) EventMgrCtor(gEventMgr);

    GameStateInit(gGame);
    fn_8019EA7C(lbl_80359420);

    u8* sets = (u8*)SIR_PTR(0xDB0);
    gScreenSets = sets;
    *(u32*)(sets + 0x84) = 0;
    *(u32*)(sets + 0x88) = 0;
    *(u32*)(sets + 0x8C) = 0;
    *(u32*)(sets + 0x90) = 0;
    ScreenSetsInit(gScreenSets);

    lbl_805FB660 = SIR_PTR(0xDB4);
    lbl_805FB658 = SIR_PTR(0xDB8);
    ((void**)lbl_805FB658)[1] = gCamera;
    lbl_805FB670 = SIR_PTR(0xDBC);
    gGlobalBinaryField844 = *(u32*)(sSirHeader + 0x844);
    gStringTable = SIR_PTR(0xDA8);

    void** screens = (void**)SIR_SCREENS;
    if (screens != NULL && screens[0] != NULL) ScreenMgrLoad(screens);

    HudInit(SIR_HUD, &SIR_SCREENS);
    HudReset(SIR_HUD);
    fn_801ABA2C();
    ScreenSetsLoadCsv("ui\\ScreenSets\\ScreenSets.csv");
    memset((u8*)gScreenSets + 0x88, 0, 0xC);

    if (gUiActive != 0) GameSetNextMode(2);
}

// 0x8000CB20
void UIEngine_ActivateUIScreenSet(s32 id) {
    void* prev = ScreenSetsCurrent(gScreenSets);
    ScreenSetsSelect(gScreenSets, id);
    if (prev != ScreenSetsCurrent(gScreenSets)) {
        if (prev != NULL) ScreenSetRelease(prev);
        SetLoadingIcon(FALSE);
    }
}

// 0x8000CC30
// Debug log; the body was compiled out of the release build.
void ZoneLog(void* loader, const char* fmt, ...) {
    (void)loader; (void)fmt;
}

// 0x8000CB90
// Completion callback for ZoneLoadAsync.
static void OnZoneScreensLoaded(void* zoneInfo, void* arg) {
    s32 zone = *(s32*)((u8*)zoneInfo + 0x18);
    u8* zoneTable = *(u8**)(*(u8**)gWorldState + 0x1A4);
    ScreenSetsBind(gScreenSets, (s32)(intptr_t)arg, *(s32*)(zoneTable + zone * gZoneResourceSize + 0x100));
    ZoneLoadScreens(gWorldState, ((s32*)gBuffer803C7434.data)[zone], arg);
    ZoneLog(gSirLoader, "ScreenSet load end: zone=%d", zone);
}

// 0x8000CC80
void UIEngine_LoadUIScreenSet(s32 id) {
    u8* res = (u8*)ResourceFind(*(void**)((u8*)gResourceDb + 0x20), id, 0);
    if (res == NULL) return;

    s32 zone = *(s32*)(res + 0x44);
    s32 state = *(s32*)(gScriptCtx + 0x18);
    if (state != 5 && state != 7) return;

    ScreenSetsFlush(gScreenSets);
    u8* loaded = (u8*)ScreenSetsFind(gScreenSets, id);
    if (loaded != NULL) {
        if (*(s32*)(loaded + 4) == zone) return;
        ZoneUnload(gWorldState, ResourceGetCurrentId(), TRUE);
    }
    SetLoadingIcon(FALSE);
    ZoneSetCurrent(gWorldState, zone);
    ZoneLog(gSirLoader, "ScreenSet load start: zone=%d", *(s32*)(res + 0x44));
    ZoneLoadAsync(gWorldState, zone, 1, OnZoneScreensLoaded, id);
}

// 0x8000CD8C
void fn_8000CD8C(void) {
}

// 0x8000CD90
BOOL IsUISlotActive(s32 id) {
    BOOL loaded = FALSE;
    u8* set = (u8*)ScreenSetsFind(gScreenSets, id);
    if (set != NULL) loaded = ZoneIsLoaded(gWorldState, *(s32*)(set + 4));
    return loaded;
}

// 0x8000CDE8
void fn_8000CDE8(void) {
    gUiFlag8C = 1;
}

// 0x8000CDF4
BOOL fn_8000CDF4(void) {
    return gUiFlag8C != 0;
}

// 0x8000CE0C
void fn_8000CE0C(void) {
    ScriptCall(gScriptCtx, 0x43, 0);
}

// 0x8000CE20
BOOL GameFlowQuitGameDone(void) {
    return gGameMode == 2;
}

// 0x8000CE34
void GameFlowPauseGame(s32 value) {
    ScriptSetVar(gScriptCtx, 4, value);
}

// 0x8000CE48
s32 GameFlowResumeGame(void) {
    return ScriptGetVar(gScriptCtx, 4);
}

// 0x8000CE58
void SetCursor(s32 id) {
    ScreenSetsPush(gScreenSets, id);
}

// 0x8000CE64
void ShowCursor(s32 id) {
    ScreenSetsPop(gScreenSets, id);
}

// 0x8000CE70
void DeleteCursor(void) {
    ScreenSetsFlush(gScreenSets);
}

// 0x8000CE78
s32 GetCursorPos(void) {
    if (gScreenSets == NULL) return 0;
    return ScreenSetsCount(gScreenSets);
}

// 0x8000CE90
BOOL IfCurrentGameFlowStateIs(s32 id) {
    ScreenList* list = (ScreenList*)SIR_SCREENS;
    u8* entry = (list->current > -1) ? list->table->entries + list->current * 0x30 : NULL;
    s32 cur = entry ? *(s32*)(entry + 4) : 0;
    return cur == id;
}

// 0x8000CEE4
BOOL IfTestTypeIs(s32 player) {
    return PlayerIndexOf(InputGetPlayer(gInput)) == player;
}

// 0x8000CF28
BOOL IfTestNameIs(s32 player) {
    return PlayerIndexOf(InputGetPlayer2(gInput)) == player;
}
