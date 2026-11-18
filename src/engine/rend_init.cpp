// Renderer bring-up: display-list pool, render thread and the first (state-reset) display list.
// Original: 0x8014DD84 - 0x8014EACC (auto_03_801467F8_text.s), default allocators 0x80148530
#include <stdlib.h>

#include "engine/rend.h"
#include "system/sys.h"

extern "C" {
void* MemAllocSmall(u32 size);          // 0x801D41CC: small-block pool allocator (provisional)
void  VideoInit(RendVideo* video);      // 0x8015BF40: VI/GX init, frame buffers, TV mode tables
void  RendSetDefaultState(void);        // 0x8014855C: resets all GX state to engine defaults
void  RendInitTargets(void);            // 0x80157668: render targets ("hdrRed"/"hdrGreen" etc.)
}

// ---- data ----
EngineMemCallbacks* gRendMem;                           ORIG(0x805FB4A8)
static RendDL       sDLs[REND_DL_COUNT];                ORIG(0x80401108)
static RendDLRecord sDLRecords[REND_DL_COUNT];          ORIG(0x80401ADC)
static void**       sScratchA;                          ORIG(0x805FB4F8) // 255 blocks of 0x3FC bytes
static void**       sScratchB;                          ORIG(0x805FB4FC)

static s32  sCurDL;            ORIG(0x805FB4BC) // DL currently being recorded
static s32  sRecording;        ORIG(0x805FB4C0)
static u32  sDLCapacity;       ORIG(0x805FB4C4)
static u32  sDLFlushedEnd;     ORIG(0x805FB4C8) // bytes [x, capacity) still need a cache flush
static u32  sDLUnkCC;          ORIG(0x805FB4CC)
static u8*  sDLCursor;         ORIG(0x805FB4D0)
static s32  sDeferSubmit;      ORIG(0x805FB4D4) // when set, finished DLs queue up instead of running
static s32  sDeferHead;        ORIG(0x805FB4D8)
static s32  sDeferCount;       ORIG(0x805FB4DC)
static s32  sKeepDLs;          ORIG(0x805FB4E0) // when set, finished DLs are not returned to the pool
static s32  sUnk4E4;           ORIG(0x805FB4E4)
static s32  sKeptDLCount;      ORIG(0x805FB4E8)
static s32  sDLBalance;        ORIG(0x805FB4EC)
static s32  sUnk518;           ORIG(0x805FB518)
static s32  sUnk51C;           ORIG(0x805FB51C)

// 0x80148530

// - 0x80148558: fallbacks when the engine passes no callbacks.
static void* DefaultAlloc(u32 size, u32, int, const char*, u32) {
    return MEMAllocFromAllocator(&gSysMEM2Allocator, size);
}
static void  DefaultFree(void* ptr)                 { MEMFreeToAllocator(&gSysMEM2Allocator, ptr); }
static void* DefaultAllocTemp(u32 size)             { return malloc(size); }         // 0x80330D80
static void* DefaultRealloc(void* ptr, u32 size)    { return realloc(ptr, size); }   // 0x80330F50
static void  DefaultFreeAny(void* ptr)              { free(ptr); }                   // 0x80330E20

static EngineMemCallbacks sDefaultRendMem = {       ORIG(0x80376188)
    DefaultAlloc, DefaultFree, DefaultAllocTemp, DefaultRealloc, DefaultFreeAny,
};

static inline void NextDL(s32& index) {
    if (++index >= REND_DL_COUNT) index = 0;
}

// Submit every display list that was held back while sDeferSubmit was set.
static inline void RendFlushDeferred(void) {
    if (sDeferSubmit && !RendHasFreeDL() && sDeferSubmit) {
        RendThreadSync();
        while (sDeferCount > 0) {
            RendThreadSubmitDL(&sDLRecords[sDeferHead], sKeepDLs == 0);
            NextDL(sDeferHead);
            sDeferCount--;
        }
        sDeferSubmit = 0;
        sDeferHead = 0;
        sDeferCount = 0;
    }
}

static inline void RendBeginDL(void) {
    RendWaitFreeDL();
    RendDLBegin(&sDLs[sCurDL], &sDLRecords[sCurDL]);
    sRecording = 1;
    sDLCapacity = sDLFlushedEnd = REND_DL_SIZE;
    sDLUnkCC = 0;
    sDLCursor = RendDLGetBase(&sDLs[sCurDL]);
    sDLBalance--;
}

static inline void RendEndDL(void) {
    RendDLEnd(&sDLs[sCurDL]);
    u32 unflushed = REND_DL_SIZE - sDLFlushedEnd;
    if (unflushed != 0) DCFlushRange(RendDLGetBase(&sDLs[sCurDL]) + sDLFlushedEnd, unflushed);

    if (sDeferSubmit) sDeferCount++;
    else              RendThreadSubmitDL(&sDLRecords[sCurDL], sKeepDLs == 0);

    sRecording = 0;
    sDLCursor = NULL;
    sDLCapacity = sDLFlushedEnd = sDLUnkCC = 0;
    if (sKeepDLs) sKeptDLCount++;
    NextDL(sCurDL);
}

// 0x8014DD84
static void RendDLPoolInit(void) {
    sScratchA = (void**)MemAllocSmall(0xFF * sizeof(void*));
    sScratchB = (void**)MemAllocSmall(0xFF * sizeof(void*));
    for (int i = 0; i < 0xFF; i++) {
        sScratchA[i] = MemAllocSmall(0x3FC);
        sScratchB[i] = MemAllocSmall(0x3FC);
    }
    for (int i = 0; i < REND_DL_COUNT; i++) RendDLCreate(&sDLs[i], REND_DL_SIZE);

    sCurDL = sRecording = 0;
    sDLCapacity = sDLFlushedEnd = sDLUnkCC = 0;
    sDeferSubmit = sDeferHead = sDeferCount = 0;
    sKeepDLs = sUnk4E4 = sKeptDLCount = sDLBalance = 0;
}

// 0x8014E664
extern "C" void RendInit(EngineMemCallbacks* mem) {
    gRendMem = mem ? mem : &sDefaultRendMem;
    VideoInit(&gVideo);
    RendThreadInit(&gVideo, 16);
    RendDLPoolInit();
    sUnk518 = 0;
    sUnk51C = 0;

    // First display list: reset GX to known defaults and clear the 8 post-transform
    // texture matrices to identity.
    RendFlushDeferred();
    RendBeginDL();
    RendSetDefaultState();
    static const Mtx kIdentity = { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0} };
    for (int i = 0; i < 8; i++) GXLoadTexMtxImm(kIdentity, GX_PTTEXMTX0 + i * 3, GX_MTX3x4);
    RendEndDL();

    RendInitTargets();
}
