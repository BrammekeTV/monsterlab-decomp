// Top-level game mode state machine, run once per frame via GameFrameCallback.
// Original: GameUpdate 0x8000D9EC (auto_03_8000CF8C_text.s)
//
// gGameMode selects the current top-level mode; each mode has a sub-state in sModeTable[mode].sub
// that steps through odd values (1 -> 3 -> 5 -> 7 -> 9 -> 0) as it starts up, runs and tears down.
// Modes 3 and 7 play queued in-game Bink movies.
#include <string.h>

#include "engine/engine.h"
#include "engine/view.h"
#include "engine/rend.h"
#include "game/game.h"

extern "C" {
void  RendClear(int flags, int a, int b, f32 depth);   // 0x801534F0
void  fn_8016FB20(void*);  void fn_80170044(void*);
void  fn_801B42B4(void*);  void fn_801B420C(void*);
void  fn_8010A374(void);
void  EngineRequestSuspend(int on);                     // 0x801D1830
s32   LoadingScreenTick(int, f32);                       // 0x8000E254
void  fn_80121254(void*);
s32   fn_801F525C(void); void fn_801F357C(void); void fn_801F3BB8(void); void fn_801F401C(void);
void  fn_8020A680(void); void fn_8020A69C(int); void MusicUpdate(void); void fn_8020AF2C(void);
s32 AcquireZoneSlot(u8* world);                            // 0x80131A20 ("lock_available_zone_slot waiting...")
s32   ReleaseZoneSlot(u8* world, s32 slot);                           // 0x80131C28
void  GameLog(int level, int category, const char* fmt, ...);   // 0x8000B2F0
s32   VideoIsPal50(RendVideo* video);                    // 0x8015DE24: video height == 520
void  AudioSetPaused(void* audio, int paused);           // 0x801FF708 (provisional)
void  MoviePlay(void* player, const char* path, int a, u32 b, s32 moreQueued, u32 duration, u32 c, int d);   // 0x802049E0
void  fn_8000B6A0(void*, int);
}


s32               gMovieZoneSlot = -1;  ORIG(0x805F81C8)
extern u32        gGameFlags;           ORIG(0x805FAED4)
extern u8         gIsReturningPlayer;   ORIG(0x805FAEA8)
extern u8         gWorldState[];        ORIG(0x803E6D00)
extern "C++" View*      gMainView;            ORIG(0x805FB75C)
extern u8         lbl_8047C7D8[], lbl_8047C7F0[], lbl_805FAEB0[], lbl_803C4298[];
extern u8         gMoviePlayer[];       ORIG(0x805FB7E8)
extern s32        gAudioOverride;       ORIG(0x805FBA30)
extern u32        lbl_805FAEB8;

enum { GAMEFLAG_IN_MOVIE = 0x400, GAMEFLAG_100 = 0x100, GAMEFLAG_200 = 0x200 };

static inline BOOL MovieQueueIsEmpty(const MovieQueue* q) {
    return q->head < 0 || q->tail < 0;
}

static inline MovieEntry* MovieQueueFront(const MovieQueue* q) {
    if (q->head < 0 || q->head >= q->capacity) return NULL;
    return &q->entries[q->head];
}

static inline void MovieQueuePop(MovieQueue* q) {
    if (q->tail == q->head) {
        q->tail = q->head = -1;
    } else {
        q->head = (q->head + 1) % q->capacity;
    }
}

static inline void ClearScreen(void) {
    RendClear(7, 0, 0, 0.0f);
}

static void DrawWorld(void) {
    ClearScreen();
    fn_8016FB20(gMainView);
    fn_801B42B4(lbl_8047C7D8);
    fn_801B420C(lbl_8047C7D8);
    fn_80170044(gMainView);
}

static void UpdateMode1(ModeState* st) {
    switch (st->sub) {
    case 1:
        if (LoadingScreenTick(0, -1.0f) == 0) st->sub = 3;
        break;
    case 3:
        st->sub = 5;
        fn_80121254(lbl_805FAEB0);
        EngineRequestSuspend(FALSE);
        break;
    case 5:
        ClearScreen();
        if (!fn_801F525C()) fn_801F357C();
        fn_801F3BB8();
        fn_801F401C();
        break;
    case 7:
        ClearScreen();
        st->sub = 9;
        break;
    case 9:
        ClearScreen();
        st->sub = 0;
        break;
    }
}

static void UpdateMode2(ModeState* st) {
    switch (st->sub) {
    case 0:
        DrawWorld();
        break;
    case 1:
        fn_8010A374();
        st->sub = 3;
        break;
    case 3:
        st->sub = 4;
        break;
    case 4:
        st->sub = 5;
        ClearScreen();
        EngineRequestSuspend(FALSE);
        break;
    case 5:
        ClearScreen();
        if (MovieQueueIsEmpty(&gMovieQueue) && gNextGameMode != NEXT_MODE_LOCKED) {
            gNextGameMode = 7;   // both settings branches pick 7
        }
        break;
    case 7:
        ClearScreen();
        st->sub = 9;
        break;
    case 9:
        ClearScreen();
        st->sub = 0;
        break;
    }
}

static void PlayQueuedMovies(void) {
    while (!MovieQueueIsEmpty(&gMovieQueue)) {
        MovieEntry* entry = MovieQueueFront(&gMovieQueue);
        u32 duration = entry ? entry->duration : 0;
        MovieQueuePop(&gMovieQueue);
        if (entry == NULL) continue;

        s32 savedAudio = gAudioOverride;
        gAudioOverride = 1;
        AudioSetPaused(lbl_8047C7F0, TRUE);

        if (entry->name[0] != '\0') {
            char path[0xC0];
            strcpy(path, VideoIsPal50(&gVideo) ? "./fmv_PAL/" : "./fmv/");
            strcat(path, entry->name);
            strcat(path, ".bik");
            BOOL moreQueued = !MovieQueueIsEmpty(&gMovieQueue);
            MoviePlay(gMoviePlayer, path, 0, ((u32*)sSirHeader)[0x824 / 4] /* HOME-unavailable icon */, moreQueued, duration, lbl_805FAEB8, 0);
        }

        AudioSetPaused(lbl_8047C7F0, FALSE);
        gAudioOverride = savedAudio;
        fn_8000B6A0(lbl_803C4298, 0);
    }
}

static void UpdateMovieMode(ModeState* st) {
    if (st->sub == 1) {
        fn_8020A680();
        fn_8020A69C(1);
        MusicUpdate();
        gGameFlags |= GAMEFLAG_IN_MOVIE;
        if (gGameFlags & GAMEFLAG_100) gGameFlags = (gGameFlags | 1) & ~(GAMEFLAG_100 | GAMEFLAG_200);
        fn_8020AF2C();
        ClearScreen();

        if (gGameMode == 7) {
            gMovieZoneSlot = 3;
        } else if (gGameMode == 3) {
            gMovieZoneSlot = (*(s32*)(gWorldState + 0xB4) > 0) ? AcquireZoneSlot(gWorldState) : -1;
        }

        if (gMovieZoneSlot != -1) {
            st->sub = 5;
        } else {
            GameLog(4, 0, "Failed to acquire zone slot for in-game movie\n");
            while (!MovieQueueIsEmpty(&gMovieQueue)) MovieQueuePop(&gMovieQueue);
            st->sub = 9;
        }
    }

    if (st->sub == 5) {
        PlayQueuedMovies();
        ClearScreen();
        st->sub = 9;
    }

    if (st->sub == 7 || st->sub == 9) {
        st->sub = 0;
        if (gMovieZoneSlot != -1) {
            if (gGameMode == 3 && gWorldState[4]) ReleaseZoneSlot((u8*)gWorldState, gMovieZoneSlot);
            gMovieZoneSlot = -1;
        }
        ClearScreen();

        if (gGameMode == 7) {
            if (gNextGameMode != NEXT_MODE_LOCKED) {
                gNextGameMode = (EngineGetGameSettings()->field_0x38 == 1 && gIsReturningPlayer) ? 6 : 2;
            }
        } else if (gGameMode == 3) {
            if (gNextGameMode != NEXT_MODE_LOCKED) gNextGameMode = 0;
        }

        gGameFlags &= ~GAMEFLAG_IN_MOVIE;
        MusicUpdate();
        fn_8020A69C(0);
    }
}

static void UpdateMode6(ModeState* st) {
    switch (st->sub) {
    case 1:
    case 5:
        EngineRequestSuspend(FALSE);
        break;
    case 7:
        st->sub = 0;
        break;
    }
}

// 0x8000D9EC
extern "C" void GameUpdate(void) {
    if ((u32)gGameMode <= 9) {
        ModeState* st = &sModeTable[gGameMode];
        switch (gGameMode) {
        case 1: UpdateMode1(st); break;
        case 2: UpdateMode2(st); break;
        case 3:
        case 7: UpdateMovieMode(st); break;
        case 6: UpdateMode6(st); break;
        case 9: DrawWorld(); break;
        default: break;
        }
    }
    MusicUpdate();
}
