#pragma once
#include "types.h"

// Memory callback table handed to the renderer (REND) and scene-graph (SG) modules.
// Both modules tag allocations with a type enum (REND_MEMORY_* / SG_MEMORY_*).
struct EngineMemCallbacks {
    void* (*alloc)(u32 size, u32 align, int type, const char* name, u32 extra);
    void  (*free)(void* ptr);
    void* (*allocTemp)(u32 size);
    void* (*realloc)(void* ptr, u32 size);
    void  (*freeAny)(void* ptr);
};

// Per-game settings block allocated in GameInit (0x5C bytes).
struct GameSettings {
    u8  skipDebugOverlay;   // 0x00: set to 1 at init; when 0, EngineFrame draws extra debug layers
    u8  pad01[0x37];
    s32 field_0x38;         // 0x38: start-mode selector read by GameStart
    u8  pad3C[0x20];
};

typedef void (*EngineFrameCallback)(void);

// engine.cpp
void  EngineStartup(void* ctx);
void  EngineInit(void);
void  EngineFrame(void);
void  EngineShutdown(void);
void  EngineCleanup(void* ctx);
void  EngineSetFrameCallback(EngineFrameCallback cb);
GameSettings* EngineGetGameSettings(void);
void  EngineSetGameSettings(GameSettings* settings);

// Memory manager (0x801D4xxx, not yet decompiled)
extern "C" {
void* MemAlloc(u32 size, u32 align, u32 flags, const char* tag, u32 extra);   // 0x801D4A90
void* MemAllocDefault(u32 size);                                              // 0x801D4650
void* MemRealloc(void* ptr, u32 size, u32 flags);                             // 0x801D4D30
void  MemFree(void* ptr);                                                     // 0x801D52D0
}
