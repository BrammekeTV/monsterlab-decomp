// Per-frame update of the game session (gScriptCtx): the level load / run / unload state
// machine, followed by drawing the frame for the current state.
// Original: GameSessionUpdate 0x80008544 (auto_01_80006B40_text.s)
//
// States (session +0x18):
//   1 start loading the next level        5 running
//   2 wait for the loader / save restore   6 unloading, then back to 1
//   3 wait for the level, set up camera    7 paused (world still updates)
//   4 wait for the world to be ready       8 hand off to the queued movie
#include <stddef.h>

#include "types.h"
#include "revolution.h"
#include "engine/view.h"
#include "game/script_callbacks.h"

typedef void (*VFn)(void*);
typedef void (*VFnI)(void*, s32);
typedef void (*VFnF)(void*, f32);
typedef void (*VFnPP)(void*, const void*, const void*);
typedef BOOL (*VFnB)(void*);
#define VT(obj, off) (*(void**)(*(u8**)(obj) + (off)))

struct Locator { u8 pad[8]; f32 pos[3]; };

extern "C" {
void   GameStateReset(void* g);                         // 0x80006BB4
void   fn_80006F44(void);
void   fn_80009748(void* s, u8 level);                  // level teardown
void   fn_800098F4(void* s, f32 a, f32 dt);             // world render
void   fn_8000A6AC(void* s);                            // world draw
void   fn_8000B340(void* s);                            // movie hand-off state
void   fn_8000B9CC(void* s, f32 dt);                    // session input / HUD update
void   fn_80013F8C(void* p);
void   fn_8003D4B0(void);  void* fn_8003E39C(void);
void   fn_8003D580(void* p);  void fn_8003E354(void);
void   fn_800512D0(void);
void   fn_8006FD18(void);  void fn_8007015C(void);
void   fn_8010A3B0(void);  void fn_8010A3B4(void);  BOOL fn_8010A3B8(void);
void   fn_801214E4(void* p);
void   fn_80125F68(void);  void fn_801260C8(void);
BOOL   fn_80129334(void* world, u8 level);              // level loaded?
void   fn_80129438(void* world);
void   fn_8012AAD8(void* world);
void   fn_8012CD90(void* world);
BOOL   fn_8012CDF0(void* world);
void   fn_8013244C(void* world);
BOOL   fn_80132C50(void* world);                        // still unloading?
BOOL   fn_80147554(s32 button);                         // button pressed
void   fn_80150148(void);  void fn_801514F0(void);
void   fn_801542DC(void);  void fn_80154318(void);
s32   fn_8016195C(void* p, s32 a, s32 b, s32 c);
void   fn_8016FB20(void* view);  void fn_80170044(void* view);
void   fn_8018CE64(void* sets, void* a, void* b, void* c);
void   fn_8018DEF4(void* sets);
void   fn_801B42B4(void* p);
void   fn_801D5F98(f32 dt);
void   fn_801D66C4(f32 a, f32 b);
void   CameraSetMatrix(void* camera, const f32 mtx[3][4]);   // 0x801D6814
void   fn_801E7E28(void* p);
BOOL   fn_801F5220(void);  BOOL fn_801F525C(void);
void   fn_801F357C(void);  void fn_801F3BB8(void);  void fn_801F401C(void);  void fn_801F487C(void);
s32 fn_801FCA38(f32 x, f32 y);
void   AudioSetPaused(void* audio, int paused);          // 0x801FF708
void   SaveAddPlayTime(void* save, s64 seconds);         // 0x80225674
void   fn_80225C10(void* save, s64 v);
void   fn_8022624C(void* save);
s32    fn_802728A4(s32 chan, u32* out);
s32    fn_80272B64(s32 chan, u8* out);
void   fn_800D81AC(void* w);  s32 fn_800D83B0(void* w);  void fn_800D8280(void* w);
s32    WorldGetPhase(void* world);                       // 0x800D83A8
Locator* WorldFindLocator(void* world, u32 hash);        // 0x8012AA50
void   WorldPreUpdate(void* world);                      // 0x80130418
void*  CharacterFind(s32 id);                            // 0x800E876C
s32 TriggerDoAction(void* action, void* a1, void* a2);        // 0x801E0054
void   SessionBegin(void* session);                      // 0x80013690
void   GameSetNextMode(s32 mode);                        // 0x8000CFD8
void*  ScreenSetsCurrent(void* sets);                    // 0x8018D1E4
u32    StringHash(const char* s);
}
void GameRequestModeState(s32 mode, s32 sub);            // 0x8000D048

extern u8     gScriptCtx[];        // 0x803C4298 (the session)
extern u8     gGame[];             // 0x803B53A0
extern u8     gWorldState[];       ORIG(0x803E6D00)
extern u8     gAudio[];
extern "C++" void*  gScreenSets;         // 0x805FB654
extern u8*    gSaveSystem;         ORIG(0x805FBBE0)
extern "C++" View*  gMainView;           ORIG(0x805FB75C)
extern s32    gAudioOverride;      ORIG(0x805FBA30)
extern s32    gGameMode;           // 0x805F81C0
extern u8     sWorldReady;         // 0x805FB2F0
extern u8     lbl_8047C7D8[], lbl_8047C790[], lbl_803C2A20[], lbl_8040E5F0[], lbl_803C4400[];
extern u8     lbl_8047C640[];      // camera
extern u8     lbl_805F81B0, lbl_805FB308, lbl_805FB1A1;
extern u8     lbl_805FB50C, lbl_805F9210, lbl_805F9212;
extern s32    lbl_805FB1D4, lbl_805FB778, lbl_805F81A0;
extern s32    lbl_805FB970, lbl_805FB974;
extern void*  lbl_805FB964;        // movie player
extern f32    lbl_805FBA80, lbl_806002D8;

static u8 sLoadingScreenUp;        ORIG(0x805FAE60)

#define WORLD           (gGame + 0xD5F0)
#define WORLD_DATA()    (*(u8**)gWorldState)
#define LEVEL_LAB       0x3F
#define LEVEL_FRONTEND  0x43

enum {
    SFLAG_PAUSED     = 0x02,
    SFLAG_RESET_GAME = 0x08,
    SFLAG_LOAD_A     = 0x20,
    SFLAG_LOAD_B     = 0x40,   // restore from save
    SFLAG_ENTERED    = 0x80,
};

struct GameSession {
    u8    pad00[4];
    void* camera;       // 0x04
    void* camTarget;    // 0x08
    u8    pad0C[0xC];
    s32   state;        // 0x18
    u8    level;        // 0x1C
    u8    nextLevel;    // 0x1D
    u8    prevLevel;    // 0x1E
    u8    restored;     // 0x1F
    u8    pad20[0x1C];
    u32   flags;        // 0x3C
    u8    unk40[0xC];
    u8    unk4C;        // 0x4C
    u8    pad4D[0xF];
    s32   movieHead;    // 0x5C
    s32   movieTail;    // 0x60
    u8    pad64[0xD4];
    s32   unk138;       // 0x138
    s32   unk13C;       // 0x13C
    f32   playTimer;    // 0x140
    u8    pad144[0x10];
    s32   unk154;       // 0x154
};

// Pixel-engine control register writes (BP 0x00 with a 0xFE00C000 mask).
static inline void GXWriteBP(u32 v) {
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = v;
}
static inline void SetCopyFilter(u32 v) {
    GXWriteBP(0xFE00C000);
    GXWriteBP(v);
}

static inline void CamCall(void* cam, u32 off) { ((VFn)VT(cam, off))(cam); }
static inline void CamCallI(void* cam, u32 off, s32 a) { ((VFnI)VT(cam, off))(cam, a); }

static inline void CamFollowPlayer(GameSession* s) {
    CamCallI(s->camera, 0x14, 0);
    CamCall(s->camera, 0x94);
    ((void (*)(void*, void*))VT(s->camera, 0x24))(s->camera, *(void**)(*(u8**)(WORLD_DATA() + 8) + 4));
}

static inline void CamLookAt(GameSession* s, const void* from, const void* to) {
    CamCallI(s->camera, 0x0C, 6);
    ((VFnPP)VT(s->camera, 0x2C))(s->camera, from, to);
    CamCall(s->camera, 0x94);
}

static inline BOOL IntroCharacterPresent(void) {
    return CharacterFind((s32)0xAE906EF7) != NULL && *(s32*)(gGame + 0xFE0) == 0;
}

// A queued movie is waiting and the player is ready to show it.
static inline BOOL MovieReady(GameSession* s) {
    if (s->movieHead < 0 || s->movieTail < 0) return FALSE;
    if (!(lbl_805FB970 && lbl_805FB974 && lbl_805FB964)) return FALSE;
    if (!((VFnB)VT(lbl_805FB964, 0x34))(lbl_805FB964)) return FALSE;
    return fn_801F5220();
}

// Draw an empty frame (loading / transition states).
static void DrawBlankFrame(void) {
    static const f32 kIdentity[3][4] = { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0} };
    fn_80150148();
    fn_8016FB20(gMainView);
    fn_801D66C4(0.5f, 1000.0f);
    *(u32*)((u8*)gMainView + 0x188) = 0;
    CameraSetMatrix(lbl_8047C640, kIdentity);
    fn_801514F0();
    lbl_805FB50C &= ~0x08;
    SetCopyFilter(0);
    fn_801542DC();
    fn_80154318();
    GXWriteBP(0xFE000FE0);
    GXWriteBP(0x410004A0);
    fn_801B42B4(lbl_8047C7D8);
    fn_80170044(gMainView);
}

static void Session_SetupLevelCamera(GameSession* s) {
    u8* data = WORLD_DATA();
    switch (*(s32*)(data + 0x1E0)) {
    case 2:
        fn_800D81AC(WORLD);
        if (IntroCharacterPresent()) {
            Locator* a = WorldFindLocator(gWorldState, 0x4A9DC9E2);
            Locator* b = WorldFindLocator(gWorldState, 0x56D1FD27);
            f32 from[3], to[3];
            if (a != NULL && b != NULL) {
                from[0] = a->pos[0]; from[1] = a->pos[1]; from[2] = a->pos[2];
                to[0] = b->pos[0];   to[1] = b->pos[1];   to[2] = b->pos[2];
            } else {
                from[0] = -11.4f; from[1] = 30.3f;  from[2] = 41.6f;
                to[0] = 119.5f;   to[1] = -14.4f;   to[2] = 0.4f;
            }
            CamLookAt(s, from, to);
            CamCallI(s->camera, 0x50, 0);
        } else {
            CamFollowPlayer(s);
            CamCallI(s->camera, 0x50, 0);
        }
        return;
    case 3: {
        CamCallI(s->camera, 0x0C, 6);
        f32 from[3] = { 0.0f, -2.0f, 0.5f };
        ((VFnPP)VT(s->camera, 0x2C))(s->camera, from, *(void**)(*(u8**)(gGame + 0xD614) + 4));
        CamCall(s->camera, 0x94);
        return;
    }
    case 1:
        fn_800D81AC(WORLD);
        if (fn_800D83B0(WORLD) == 2 || fn_800D83B0(WORLD) == 6) fn_8022624C(gSaveSystem);
        break;
    }

    if (WorldGetPhase(WORLD) == 1 && s->level != LEVEL_LAB) {
        if (IntroCharacterPresent()) {
            Locator* a = WorldFindLocator(gWorldState, 0x4A9DC9E2);
            Locator* b = WorldFindLocator(gWorldState, 0x56D1FD27);
            if (a == NULL || b == NULL) {
                a = WorldFindLocator(gWorldState, 0x834C7963);
                b = WorldFindLocator(gWorldState, 0xAAE1A370);
            }
            CamLookAt(s, a->pos, b->pos);
            CamCallI(s->camera, 0x50, 0);
            lbl_805FB1A1 = 0;
            ((VFnF)VT(s->camTarget, 0x30))(s->camTarget, 0.0f);
            return;
        }
    }
    CamFollowPlayer(s);
}

// 0x80008544
void GameSessionUpdate(GameSession* s, f32 a, f32 dt) {
    if (!fn_801F525C()) fn_801F357C();

    s32 entryState = s->state;
    switch (entryState) {
    case 1:
        fn_800512D0();
        fn_8018DEF4(gScreenSets);
        if (s->level == LEVEL_FRONTEND) {
            lbl_805F81B0 = 0;
            GameSetNextMode(2);
            GameRequestModeState(gGameMode, 9);
            s->state = 0;
            break;
        }
        if (s->flags & SFLAG_LOAD_A)      fn_8010A3B0();
        else if (s->flags & SFLAG_LOAD_B) fn_8010A3B4();
        s->state = 2;
        break;

    case 2:
        if (fn_8010A3B8()) break;
        if (s->flags & SFLAG_LOAD_B) {
            fn_80013F8C(gGame + 0xC448);
            if (gGame[0xC460] == 0 && fn_801F5220()) {
                u32 v = *(u32*)(gGame + 0xC464);
                *(u32*)(gGame + 0xC464) = (v == 0);
                *(u32*)(gGame + 0xC468) = v;
            }
            lbl_805FB308 = 0;
            s->restored = 1;
        }
        sLoadingScreenUp = 1;
        s->flags &= ~(SFLAG_LOAD_A | SFLAG_LOAD_B);
        SuspendSFX(1);
        fn_8012CD90(gWorldState);
        s->state = 3;
        break;

    case 3:
        if (fn_80129334(gWorldState, s->level) && fn_8012CDF0(gWorldState)) {
            fn_801E7E28(s->unk40);
            s->unk138 = 0;
            s->unk13C = 0;
            s->flags &= ~SFLAG_PAUSED;
            AudioSetPaused(gAudio, 0);
            ((u8*)gScreenSets)[0x94] = 0;
            lbl_8047C790[2] = 0;
            s->unk4C = 0;
            if (s->flags & SFLAG_RESET_GAME) {
                fn_80006F44();
                s->flags &= ~SFLAG_RESET_GAME;
            }
            GameStateReset(gGame);
            fn_801214E4(lbl_803C2A20 + 0x10);
            CamCall(s->camera, 0x10);
            Session_SetupLevelCamera(s);
            StoreFOV();
            s->state = 4;
        } else {
            fn_8012CDF0(gWorldState);
        }
        break;

    case 4:
        if (!sWorldReady || *(s32*)(gWorldState + 0x114) != 0) break;
        switch (WorldGetPhase(WORLD)) {
        case 3:
            fn_8006FD18();
            break;
        case 6:
            lbl_805F9210 = 0;
            lbl_805FB50C &= ~0x08;
            SetCopyFilter(0);
            fn_8003D4B0();
            if (fn_8003E39C()) fn_8003D580(fn_8003E39C());
            break;
        case 1:
            *(s32*)(gGame + 0xFE0) = 0;
            fn_801260C8();
            break;
        }
        if (MovieReady(s)) {
            s->state = 8;
            break;
        }
        fn_8012AAD8(gWorldState);
        s->state = 5;
        break;

    case 5: {
        if (sLoadingScreenUp && ScreenSetsCurrent(gScreenSets) != NULL) {
            u32 loading = StringHash("UI_MSA_Loading");
            if (*(u32*)ScreenSetsCurrent(gScreenSets) != loading) {
                sLoadingScreenUp = 0;
                SuspendSFX(0);
            }
        }
        if (sWorldReady && !(s->flags & SFLAG_PAUSED)) {
            void* ui = *(void**)(WORLD_DATA() + 8);
            if (!((VFnB)VT(ui, 0x4C))(ui)) {
                if (fn_80147554(8)) {
                    GameFlowPauseGame(0x019CE64E);
                } else if (fn_80147554(9) && !CheckGameProgressBitFlag(0x34)) {
                    GameFlowPauseGame((s32)0xB2740182);
                }
            }
        }
        fn_8000B9CC(s, dt);
        if (gAudioOverride == 1 && fn_80147554(0x108B)) fn_801FCA38(lbl_805FBA80, lbl_806002D8);

        if (s->flags & SFLAG_ENTERED) {
            u32 status = 0;
            u8 info[0x20];
            if (!fn_802728A4(0, &status) && !fn_80272B64(0, info) && *(s32*)(info + 0xC) != 0)
                s->unk154 = 1;
            if (WorldGetPhase(WORLD) == 2) SessionBegin(gGame + 0x5C8);
            TriggerDoAction(WORLD_DATA() + 0x94, 0, 0);
            TriggerDoAction(WORLD_DATA() + 0x98, 0, 0);
            TriggerDoAction(WORLD_DATA() + 0x9C, 0, 0);
            s->flags &= ~SFLAG_ENTERED;
            fn_800D8280(WORLD);
            if (WorldGetPhase(WORLD) == 2 || WorldGetPhase(WORLD) == 1) *(s32*)(gGame + 0xFE0) = 0;
        }

        // Play time is not counted in the two front-end levels.
        if (s->level != 0x18 && s->level != 0x2A) {
            s->playTimer += dt;
            if (s->playTimer > (f32)lbl_805F81A0) {
                SaveAddPlayTime(gSaveSystem, lbl_805F81A0);
                fn_80225C10(gSaveSystem, 0);
                s->playTimer = 0.0f;
            }
        }

        if (*(s32*)(WORLD_DATA() + 0x1F0) != 0) {
            fn_8016195C(lbl_8040E5F0, 2, 0, 0);
            *(s32*)(WORLD_DATA() + 0x1F0) = 0;
        }
        fn_801D5F98(*(f32*)(gWorldState + 0xC));
        if (sWorldReady && *(s32*)(gWorldState + 0x114) == 0 && MovieReady(s)) s->state = 8;
        break;
    }

    case 6:
        if (fn_80132C50(gWorldState)) break;
        RestoreFOV();
        lbl_805FB778 = 0;
        fn_8007015C();
        fn_8013244C(gWorldState);
        fn_80009748(s, s->level);
        fn_80129438(gWorldState);
        lbl_805F9210 = 1;
        lbl_805FB50C |= 0x08;
        SetCopyFilter((u32)lbl_805F9212 << 14);
        s->prevLevel = s->level;
        s->level = s->nextLevel;
        s->nextLevel = 0;
        s->state = 1;
        if (fn_8003E39C()) fn_8003E354();
        fn_80125F68();
        break;

    case 7:
        fn_8000B9CC(s, dt);
        if (gAudioOverride == 1 && fn_80147554(0x108B)) fn_801FCA38(lbl_805FBA80, lbl_806002D8);
        WorldPreUpdate(gWorldState);
        break;

    case 8:
        fn_8000B340(s);
        break;
    }

    fn_8016FB20(gMainView);
    fn_801F487C();
    fn_80170044(gMainView);

    if (lbl_805FB1D4 > 0) {
        lbl_805FB1D4--;
    } else {
        switch (s->state) {
        case 5: case 7: case 9:
            if (gWorldState[4] && entryState != 3 && entryState != 4 && sWorldReady) {
                fn_8000A6AC(s);
                break;
            }
            DrawBlankFrame();
            break;
        case 1: case 2: case 3: case 6:
            if (*(u32*)(lbl_803C4400 + 0xC) & 0x4) DrawBlankFrame();
            break;
        case 4: case 8:
            DrawBlankFrame();
            break;
        }
    }

    if (gWorldState[4] && entryState != 3) {
        fn_8016FB20(gMainView);
        fn_800098F4(s, a, dt);
        fn_80170044(gMainView);
    }
    fn_801F3BB8();
    fn_801F401C();

    void* cam = s->camera;
    if (cam != NULL) {
        u8 camData[0x40];
        ((void (*)(void*, void*))VT(cam, 0x38))(cam, camData);
        fn_8018CE64(gScreenSets, *(u8**)((u8*)cam + 4), *(u8**)((u8*)cam + 4) + 0xC, camData);
    }
}

// Clears the requested bits in the object's flag word at +0x64.
// 0x8000BD78
extern "C" void fn_8000BD78(u8* object, u32 mask) {
    *(u32*)(object + 0x64) &= ~mask;
}
