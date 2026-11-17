#pragma once
#include "types.h"

// Per-mode state; see src/game/game_update.cpp.
struct ModeState { s32 id; s32 sub; s32 unk8; s32 unkC; };

enum { GAME_MODE_COUNT = 10, NEXT_MODE_LOCKED = 8 };

extern ModeState sModeTable[GAME_MODE_COUNT];   // 0x803C4400
extern s32       gGameMode;                     // 0x805F81C0
extern s32       gNextGameMode;                 // 0x805F81C4
extern u8        sSirHeader[0x1000];            // 0x803C2A20: first 4 KB of global_binary.sir
