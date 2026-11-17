#pragma once
#include "types.h"

// Per-mode state; see src/game/game_update.cpp.
struct ModeState { s32 id; s32 sub; s32 request; s32 unkC; };

enum { GAME_MODE_COUNT = 10, NEXT_MODE_LOCKED = 8 };

extern ModeState sModeTable[GAME_MODE_COUNT];   // 0x803C4400
extern s32       gGameMode;                     // 0x805F81C0
extern s32       gNextGameMode;                 // 0x805F81C4
extern u8        sSirHeader[0x1000];            // 0x803C2A20: first 4 KB of global_binary.sir

// Loaded data buffers released at shutdown (0x18 bytes each, only the first 3 fields used here).
struct LoadedBuffer {
    void* data;
    u32   size;
    s32   kind;     // 1 or 2 = owned by us, must be freed
    u8    pad[0xC];
};
extern LoadedBuffer gBuffer803C7434;   // per-zone resource ids

// In-game movie queue (ring buffer; head/tail == -1 when empty).
#define MOVIE_NAME_LEN 0x80
struct MovieEntry {
    char name[MOVIE_NAME_LEN];   // file name without directory/extension
    u32  duration;               // 0x80
};
struct MovieQueue {
    s32         active;          // 0x00
    u8          unk4[4];
    s32         head;            // 0x08
    s32         tail;            // 0x0C
    s32         capacity;        // 0x10
    MovieEntry* entries;         // 0x14
};
extern MovieQueue gMovieQueue;   // 0x803C44E0
