// Game flow helpers: mode transitions, language selection, loading screen, movie queue,
// background music requests, audio listener and per-zone entity updates.
// Original object: .text 0x8000CF8C - 0x80010AC0 (auto_03_8000CF8C_text.s)
#include <string.h>

#include "engine/engine.h"
#include "engine/rend.h"
#include "engine/math.h"
#include "game/game.h"

extern "C" {
void  GameLog(int level, int category, const char* fmt, ...);   // 0x8000B2F0
u8    SCGetLanguage(void);
s32   GetRegion(void);                         // 0x8001425C: 0 = NA, 1 = EU
BOOL  fn_80147554(s32);
void  fn_801FCA38(f32, f32);
s64   TimerElapsed(void* timer);               // 0x801425DC
void  TimerReset(void* timer);                 // 0x80142744
void  RendClear(int flags, int a, int b, f32 depth);   // 0x801534F0
void  RendBeginFrame(void);                    // 0x8014F0A0
void  RendEndFrame(void);                      // 0x801508AC
void  RendFlush(void);                         // 0x80150148
void  ScreenSetsDraw(void* sets, int pass);    // 0x8018CD70
void  fn_801FC0A0(void); void fn_801FC1EC(void);
void* ScreenSetsCurrent(void* sets);           // 0x8018D1E4
void* ScreenSetGetStrings(void* set);          // 0x80198E20
void* StringsLookup(void* strings, s32 id);    // 0x80199AEC
struct LoadingAnim* LoadingAnimCreate(void* lines[6]);   // 0x80120FD8
u32   StringHash(const char* s);               // 0x801EB704
void  MusicPlay(const char* name, u8 loop, f32 volume);  // 0x8020AA94
void  StopMusic(void);                         // 0x8020AF2C
void  PauseMusic(BOOL paused);             // 0x8020AFB0
s32   WorldGetPhase(void* world);              // 0x800D83A8
void  Vec3Zero(Vec3* v);                       // 0x800DB944
void  Vec3Forward(Vec3* v);                    // 0x800DB888
void  Vec3Up(Vec3* v);                         // 0x800DB8D4
void  MtxGetForward(const f32* rot, Vec3* out);    // 0x801453BC
Vec3  MtxGetUp(const f32* rot);                // 0x801455E0
void  AudioSetListenerPos(int id, const Vec3* v);  // 0x8020AA04
void  AudioSetListenerFwd(int id, const Vec3* v);  // 0x8020AA3C
void  AudioSetListenerUp(int id, const Vec3* v);   // 0x8020AA68
BOOL  ZoneIsLoaded(void* world, s32 zone);     // 0x8012F408
BOOL  fn_801DA758(void* v);
void  EntityUpdate(void* entity);              // 0x80010298
}

// Loading animation object (vtable at +0; CodeWarrior layout: slot 0 at vtable+8).
class LoadingAnim {
public:
    virtual ~LoadingAnim();
    virtual void SetProgress(f32 percent);   // vtable+0x0C
    virtual void Draw();                     // vtable+0x10
};

// ---- data ----
extern s32   gLastSysEvent;                  // 0x805FAEAC
extern u8    gIsReturningPlayer;             // 0x805FAEA8
extern u8    gScriptCtx[];                   // 0x803C4298 (+0x18 = state)
s32          gTextLanguage;                  ORIG(0x805F81B4)
s32          gLanguage;                      ORIG(0x805F81B8)
u8           lbl_805F81BC;                   ORIG(0x805F81BC)
u8           gLoadingActive;                 ORIG(0x805FAEC8)
u8           lbl_805FAECA;                   ORIG(0x805FAECA)
LoadingAnim* gLoadingAnim;                   ORIG(0x805FAECC)
f32          gLoadingProgress;               ORIG(0x805FAED0)
u8           gLoadingTimer[8];               ORIG(0x805FAEC0)
extern s32   gAudioOverride;                 // 0x805FBA30
extern f32   lbl_805FBA80, lbl_806002D8;
extern void* gScreenSets;                    // 0x805FB654
u32          gGameFlags;                     ORIG(0x805FAED4)
static u32   sMusicHash;                     ORIG(0x805FAED8)
static u8    sMusicLoop;                     ORIG(0x805FAEDC)
static char  sMusicName[0x80];               ORIG(0x803C4918)
extern u8    gGame[];                        // 0x803B53A0
extern u8    lbl_805FB0E1;
extern u8    gWorldState[];                  // 0x803E6D00
extern s32   gZoneResourceSize;              // 0x805FB94C

// Game languages (gTextLanguage / gLanguage).
enum { LANG_EN = 0, LANG_FR = 1, LANG_DE = 2, LANG_IT = 3, LANG_ES = 4, LANG_NL = 5 };

// gGameFlags: music requests and state.
enum {
    MUSIC_REQ_PLAY   = 0x01,
    MUSIC_REQ_STOP   = 0x02,
    MUSIC_REQ_PAUSE  = 0x04,
    MUSIC_REQ_RESUME = 0x08,
    MUSIC_REQ_REPLAY = 0x10,   // re-issue a play request next update
    MUSIC_PLAYING    = 0x100,
    MUSIC_PAUSED     = 0x200,
    GAME_IN_MOVIE    = 0x400,  // music is left alone during movies
    MUSIC_STATE_MASK = 0x700,
};

// ---------------------------------------------------------------- system event / mode queries

// 0x8000CF8C
BOOL GameHasSysEvent(void) {
    return gLastSysEvent != 0;
}

// 0x8000CFA0
s32 GameGetSysEvent(void) {
    return gLastSysEvent;
}

// 0x8000CFA8
// Installed as renderer callback A: true while the game is in an interactive script state.
extern "C" BOOL fn_8000CFA8(void) {
    s32 state = *(s32*)(gScriptCtx + 0x18);
    return state == 7 || state == 5;
}

// 0x8000CFD0
extern "C" u8 fn_8000CFD0(void) {
    return lbl_805F81BC;
}

// 0x8000CFD8
// Request the next top-level mode. Returning players skip mode 2 and go to mode 6.
extern "C" void GameSetNextMode(s32 mode) {
    if (gNextGameMode == NEXT_MODE_LOCKED) return;
    if (EngineGetGameSettings()->field_0x38 == 1 && mode == 2 && gIsReturningPlayer) gNextGameMode = 6;
    else                                                                              gNextGameMode = mode;
}

// 0x8000D048
void GameRequestModeState(s32 mode, s32 sub) {
    sModeTable[mode].request = sub;
}

// Mode 1 can be cancelled half way through its start-up (request == 7).
static inline BOOL Mode1Cancel(void) {
    if (gGameMode != 1 || sModeTable[1].request != 7) return FALSE;
    BOOL changed = FALSE;
    switch (sModeTable[1].sub) {
    case 1:          sModeTable[1].sub = 0; changed = TRUE; break;
    case 3: case 5:  sModeTable[1].sub = 7; changed = TRUE; break;
    case 7: case 9:  changed = FALSE; break;
    }
    sModeTable[1].request = 0;
    return changed;
}

static inline BOOL ApplyModeRequest(void) {
    ModeState* st = &sModeTable[gGameMode];
    if (st->request == 0) return FALSE;
    if (gGameMode == 1 && st->request == 7) return Mode1Cancel();
    st->sub = st->request;
    st->request = 0;
    return TRUE;
}

// 0x8000D060
BOOL GameApplyModeRequest(void) {
    return ApplyModeRequest();
}

// 0x8000D134
// Advance mode transitions: pending sub-state requests first, then gNextGameMode.
// Modes 1, 3 and 7 are overlays (loading / movies) that interrupt the current mode
// immediately; leaving an overlay resumes the previous mode where it was.
void GameUpdateModeTransition(void) {
    if (ApplyModeRequest()) return;

    s32 next = gNextGameMode;
    if (next == 9) return;

    if (next == 1 || next == 3 || next == 7) {
        gGameMode = next;
        sModeTable[next].sub = 1;
        gNextGameMode = 9;
        return;
    }

    s32 mode = gGameMode;
    switch (sModeTable[mode].sub) {
    case 1: case 3: case 5:
        sModeTable[mode].sub = 7;   // start tearing the current mode down
        break;
    case 0: {
        BOOL fromOverlay = (mode == 1 || mode == 3 || mode == 7);
        gGameMode = next;
        gNextGameMode = 9;
        if (!fromOverlay || next == NEXT_MODE_LOCKED) sModeTable[next].sub = 1;
        break;
    }
    }

    ApplyModeRequest();
}

// 0x8000D964
BOOL GameCancelMode1(void) {
    return Mode1Cancel();
}

// ---------------------------------------------------------------- loading screen

// 0x8000E248
void fn_8000E248(void) {
    lbl_805FAECA = 1;
}

// 0x8000E254
// Draws one frame of the loading screen, at most every 20 ms. dt < 0 skips the progress bar.
extern "C" s32 LoadingScreenTick(int unused, f32 dt) {
    (void)unused;
    if (!gLoadingActive) return 0;

    if (dt >= 0.0f && gLoadingAnim != NULL) {
        gLoadingProgress += dt;
        if (gLoadingProgress >= 100.0f) gLoadingProgress = 100.0f;
        gLoadingAnim->SetProgress(gLoadingProgress);
    }

    if (gAudioOverride == 1 && fn_80147554(0x108B)) fn_801FCA38(lbl_805FBA80, lbl_806002D8);

    if ((s32)TimerElapsed(gLoadingTimer) < 20000) return 0;
    TimerReset(gLoadingTimer);

    RendClear(5, 0, 0, 1.0f);
    ScreenSetsDraw(gScreenSets, 0);
    RendFlush();
    ScreenSetsDraw(gScreenSets, 1);
    ScreenSetsDraw(gScreenSets, 2);
    if (gLoadingAnim != NULL) gLoadingAnim->Draw();
    fn_801FC0A0();
    fn_801FC1EC();
    RendEndFrame();
    RendBeginFrame();
    RendFlush();
    return 0;
}

// 0x8000E390
extern "C" s32 LoadingScreenTick2(int unused, f32 dt) {
    return LoadingScreenTick(unused, dt);
}

// 0x8000E394
// Set up the loading animation with six lines of text from the current screen set.
void InitLoadProgressProjectile(s32 s0, s32 s1, s32 s2, s32 s3, s32 s4, s32 s5) {
    s32 ids[6] = { s0, s1, s2, s3, s4, s5 };
    void* lines[6];
    for (int i = 0; i < 6; i++) {
        lines[i] = StringsLookup(ScreenSetGetStrings(ScreenSetsCurrent(gScreenSets)), ids[i]);
    }
    gLoadingProgress = 0.0f;
    if (gLoadingAnim == NULL) gLoadingAnim = LoadingAnimCreate(lines);
}

// ---------------------------------------------------------------- language

// 0x8000ED5C
void GameInitLanguage(void) {
    gIsReturningPlayer = 0;
    switch (SCGetLanguage()) {
    case 1: gLanguage = gTextLanguage = LANG_EN; break;   // SC_LANG_ENGLISH
    case 2: gLanguage = gTextLanguage = LANG_DE; break;   // SC_LANG_GERMAN
    case 3: gLanguage = gTextLanguage = LANG_FR; break;   // SC_LANG_FRENCH
    case 4: gLanguage = gTextLanguage = LANG_ES; break;   // SC_LANG_SPANISH
    case 5: gLanguage = gTextLanguage = LANG_IT; break;   // SC_LANG_ITALIAN
    case 6:                                               // SC_LANG_DUTCH
        // NA builds have no Dutch text, so it falls back to English.
        if (GetRegion() == 0) { gTextLanguage = LANG_EN; gLanguage = LANG_NL; }
        else                  { gTextLanguage = LANG_NL; gLanguage = LANG_NL; }
        break;
    default: gLanguage = gTextLanguage = LANG_EN; break;
    }
}

// 0x8000F25C
BOOL fn_8000F25C(void** screens) {
    if (*(s32*)(gScriptCtx + 0x18) == 5) return TRUE;
    return *screens != *(void**)(sSirHeader + 0xD98);
}

// ---------------------------------------------------------------- movie queue


// 0x8000F840
BOOL MovieQueueIsEmpty(void) {
    return gMovieQueue.head < 0 || gMovieQueue.tail < 0;
}

static inline MovieEntry* MovieQueueAt(MovieQueue* q, s32 i) {
    if (i < 0 || i >= q->capacity) return NULL;
    s32 idx = i, last = i;
    if (i < q->head) {
        idx = i + q->capacity;
        last = i + q->capacity;
    }
    if (q->head > idx || idx > last) return NULL;
    return &q->entries[i];
}

// 0x8000F86C
// Queue an in-game movie (fmv/<name>.bik) and switch to the movie mode.
void QueueMovie(const char* name, u32 duration) {
    if (name != NULL) {
        MovieQueue* q = &gMovieQueue;
        s32 next = (q->tail + 1) % q->capacity;
        MovieEntry* entry;
        if (q->head == next) {
            entry = NULL;   // full
        } else {
            q->tail = next;
            if (q->head < 0) {
                q->head = next;
                q->active = 1;
            }
            entry = MovieQueueAt(q, q->tail);
        }

        if (entry != NULL) {
            strncpy(entry->name, name, MOVIE_NAME_LEN);
            entry->name[MOVIE_NAME_LEN - 1] = '\0';
            entry->duration = duration;
        } else {
            GameLog(4, 0, "QueueMovie failed: %s.\n", name);
        }
    }

    if (gGameMode == 2) {
        if (gNextGameMode != NEXT_MODE_LOCKED) { EngineGetGameSettings(); gNextGameMode = 7; }
    } else {
        if (gNextGameMode != NEXT_MODE_LOCKED) { EngineGetGameSettings(); gNextGameMode = 3; }
    }
    GameUpdateModeTransition();
}

// ---------------------------------------------------------------- music

// 0x8000FA04
// Request background music (e.g. "Sounds/Music/region_music_castle"). Applied by MusicUpdate.
void MusicRequest(const char* name, u8 loop) {
    if ((gGameFlags & MUSIC_STATE_MASK) == 0) {
        if (name == NULL) return;
        sMusicLoop = loop;
        sMusicHash = StringHash(name);
        strncpy(sMusicName, name, sizeof(sMusicName) - 1);
        gGameFlags = (gGameFlags & ~0x1F) | MUSIC_REQ_PLAY;
    } else {
        if (name == NULL || StringHash(name) == sMusicHash) return;
        sMusicLoop = loop;
        sMusicHash = StringHash(name);
        strncpy(sMusicName, name, sizeof(sMusicName) - 1);
        gGameFlags = (gGameFlags | MUSIC_REQ_REPLAY) & ~MUSIC_REQ_PLAY;
    }
}

// 0x8000FACC
extern "C" void MusicUpdate(void) {
    u32 flags = gGameFlags;
    if (flags & GAME_IN_MOVIE) return;

    u32 state = flags & MUSIC_STATE_MASK;
    if (flags & MUSIC_REQ_STOP) {
        StopMusic();
        state &= ~(MUSIC_PLAYING | MUSIC_PAUSED);
    } else if (flags & MUSIC_REQ_PLAY) {
        if (sMusicName[0] != '\0') {
            MusicPlay(sMusicName, sMusicLoop, 1.0f);
            state = (state | MUSIC_PLAYING) & ~MUSIC_PAUSED;
        }
    } else if (flags & MUSIC_REQ_PAUSE) {
        if (state & MUSIC_PLAYING) {
            PauseMusic(TRUE);
            state = (state | MUSIC_PAUSED) & ~MUSIC_PLAYING;
        }
    } else if (flags & MUSIC_REQ_RESUME) {
        if (state & MUSIC_PAUSED) {
            PauseMusic(FALSE);
            state = (state & ~MUSIC_PAUSED) | MUSIC_PLAYING;
        }
    }

    if (gGameFlags & MUSIC_REQ_REPLAY) state |= MUSIC_REQ_PLAY;
    gGameFlags = state;
}

// 0x80010100
void MusicRequestStop(void) {
    if ((gGameFlags & MUSIC_STATE_MASK) == 0) return;
    gGameFlags = (gGameFlags | MUSIC_REQ_STOP) & ~MUSIC_REQ_PLAY;
}

// ---------------------------------------------------------------- audio listener

struct PlayerObj { u8 pad[4]; Transform* xform; };

// 0x8001011C
// Place the 3D audio listener on the player while exploring (world phase 2).
void AudioUpdateListener(void) {
    Vec3 pos, fwd, up;
    Vec3Zero(&pos);
    Vec3Forward(&fwd);
    Vec3Up(&up);

    u8* world = gGame + 0xD5F0;
    PlayerObj* player = *(PlayerObj**)(gGame + 0xD614);
    if (world != NULL && player != NULL && player->xform != NULL &&
        WorldGetPhase(world) == 2 && !lbl_805FB0E1) {
        Transform* xf = player->xform;
        pos = xf->pos;
        MtxGetForward((const f32*)&xf->rot, &fwd);
        up = MtxGetUp((const f32*)&player->xform->rot);
    }

    AudioSetListenerPos(0, &pos);
    AudioSetListenerFwd(0, &fwd);
    AudioSetListenerUp(0, &up);
}

// ---------------------------------------------------------------- zone entities

struct ZoneInfo;   // gZoneResourceSize bytes each
static inline u8* GetZoneInfo(s32 zone) {
    u8* table = *(u8**)(*(u8**)gWorldState + 0x1A4);
    return table + zone * gZoneResourceSize;
}

struct Entity {
    void** vtbl;
    u8*    def;        // 0x04: definition; +0x1C flags
    u8     pad08[0x94];
    s32    unk9C;      // 0x9C
    u8     padA0[0x18];
    u32    flags;      // 0xB8
};

// 0x800108B8
static void UpdateZoneEntities(s32 zone) {
    if (zone < 0) return;
    u8* z = GetZoneInfo(zone);
    s32 count = *(s32*)(z + 0x94) + *(s32*)(z + 0x98);

    for (s32 i = 0; i < count; i++) {
        z = GetZoneInfo(zone);
        s32 statics = *(s32*)(z + 0x94);
        Entity* e = (i < statics) ? (*(Entity***)(z + 0x120))[i]
                                  : (*(Entity***)(z + 0x12C))[i - statics];
        if (e == NULL) continue;

        BOOL update;
        if (!(e->flags & 1)) {
            update = TRUE;
        } else {
            update = FALSE;
            if ((*(u32*)(e->def + 0x1C) & 0x1000) && e->unk9C != 0) {
                typedef void* (*GetFn)(Entity*);
                update = fn_801DA758(((GetFn)e->vtbl[3])(e));
            }
        }
        if (update) EntityUpdate(e);
    }
}

// 0x80010228
void UpdateAllZoneEntities(void) {
    UpdateZoneEntities(0);
    for (s32 zone = 1; zone <= *(s32*)(*(u8**)gWorldState + 4); zone++) {
        if (ZoneIsLoaded(gWorldState, zone)) UpdateZoneEntities(zone);
    }
}
