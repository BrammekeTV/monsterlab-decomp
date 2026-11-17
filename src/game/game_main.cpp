// Top-level game loop.
// Original: GameMain 0x801FAED0 (auto_03_801F5D34_text.s)
#include "revolution.h"
#include "system/sys.h"

// ---- not yet decompiled (names are provisional) ----
extern "C" {
void fn_801467F8(int);            // sets a global flag (0x805F91E0)
void EngineStartup(void* ctx);    // 0x801D1504
void GameInit(void);              // 0x8000EE8C
void EngineInit(void);            // 0x801D16E0
void HomeButtonInit(void);        // 0x801FB87C (loads HomeButton2/homeBtn.arc)
void SetRenderEnabled(int on);    // 0x80151258
void EngineFrame(void);           // 0x801D1838 (update + draw one frame)
void StopRumble(int chan);   // 0x801B2B94
void GameShutdown(void);          // 0x8000F5C0
void EngineShutdown(void);        // 0x801D17FC
void EngineCleanup(void* ctx);    // 0x801D1538
}

static int    sArgc;              ORIG(0x805FB77C)
static char** sArgv;              ORIG(0x805FB780)
static u8     sEngineCtx[8];      ORIG(0x805FB770)
u8            gFrameLimitEnabled; ORIG(0x805FBA08)
s32           gTargetFps;         ORIG(0x805FBA0C)
u8            gQuitRequested;     ORIG(0x805FB758)

extern "C" {
void* MEMAllocFromAllocator(struct MEMAllocator* allocator, u32 size);
void  MEMFreeToAllocator(struct MEMAllocator* allocator, void* block);
}


// 0x801FAEB0
// Duplicate of SysAllocMEM2 compiled into this translation unit.
extern "C" void* SysAllocMEM2Block(u32 size) {
    return MEMAllocFromAllocator(&gSysMEM2Allocator, size);
}

// 0x801FAEC0
extern "C" void SysFreeMEM2Block(void* ptr) {
    MEMFreeToAllocator(&gSysMEM2Allocator, ptr);
}

// 0x801FAED0
int GameMain(int argc, char** argv) {
    sArgc = argc;
    sArgv = argv;

    fn_801467F8(1);
    EngineStartup(sEngineCtx);
    GameInit();
    EngineInit();
    HomeButtonInit();

    u64 lastFrameUs = 0;
    SetRenderEnabled(TRUE);

    while (!gQuitRequested) {
        u64 nowUs = OSTicksToMicroseconds((u64)OSGetTime());

        if (gFrameLimitEnabled) {
            // Busy-wait until the next frame slot.
            u64 nextUs = lastFrameUs + (s64)(1000000 / gTargetFps);
            while (nowUs < nextUs) {
                nowUs = OSTicksToMicroseconds((u64)OSGetTime());
            }
        }

        EngineFrame();

        if (SysGetRequestedExitMode() != SYS_EXIT_NONE) {
            SetRenderEnabled(FALSE);
        }
        lastFrameUs = nowUs;

        if (SysGetExitMode() != SYS_EXIT_NONE) {
            gQuitRequested = TRUE;
        }
    }

    for (int chan = 0; chan < 4; chan++) {
        StopRumble(chan);
    }
    GameShutdown();
    EngineShutdown();
    EngineCleanup(sEngineCtx);
    return 0;
}
