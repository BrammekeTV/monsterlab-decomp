// Engine lifecycle: memory setup, renderer / scene-graph init, per-frame update, shutdown.
// Original object: .text 0x801D1504 - 0x801D1BCC (auto_03_801D0C04_text.s)
#include <string.h>

#include "engine/engine.h"
#include "engine/view.h"
#include "system/sys.h"

extern "C" int SysSprintf(char* buf, const char* fmt, ...);

// ---- not yet decompiled ----
extern "C" {
void* SysAllocMEM2Block(u32 size);          // 0x801FAEB0
void  SysFreeMEM2Block(void* ptr);           // 0x801FAEC0
s32 HeapInit(u8* arg0, u8* arg1, u32 arg2);   // 0x801EF8DC
void  fn_80146A74(void);  void fn_8014826C(void);  void fn_80148308(void);  void fn_80146A78(void);
void  CameraInit(void* camera);              // 0x801D8B50
void  fn_801B402C(void*); void fn_801FD1F0(void*);
void  fn_80151014(void);  void fn_80151244(void);
void  RendClear(int flags, int a, int b, f32 depth);   // 0x801534F0
void  RendInit(EngineMemCallbacks* mem);     // 0x8014E664
void  RendSetFlagA(int on);                  // 0x8015124C
void  SetRenderEnabled(int on);              // 0x80151258
void  SgInit(EngineMemCallbacks* mem);       // 0x801695A4
void  SgShutdown(void);                      // 0x80169604
void  CameraSetFov(void* camera, f32 fov);   // 0x801D8ACC
void  CameraUpdate(void* camera);            // 0x801D8C04
void  CameraEndFrame(void* camera);          // 0x801D8D48
void  GameStart(int argc, char** argv);      // 0x8000EF04 (src/game/game_init.cpp)
void  DebugDrawStats(void);                  // 0x8016DAF4: "FPS=%.1f/%d (%.2fmsec)"

void  fn_80162C1C(void);   void  fn_80162C28(void);
void  RendBeginFrame(void); // 0x8014F0A0
void  RendEndFrame(void);   // 0x801508AC: present
void  fn_801FBE04(void);
void  fn_80150014(void);   void  fn_8014855C(void);
void  fn_801FBE68(void);
void  fn_800446BC(void*, int);
void  fn_80170044(void*);  void  fn_801D1AFC(void);
void  fn_8000F298(void);   void  fn_8016FB20(void*);
void  fn_801D1BCC(void* out, void* view);
void  fn_801D2388(void* view, void* state);
void* fn_8013F38C(void);   void  fn_8018CD70(void*, int);
void  fn_801CB414(void*, int);
void  fn_801BE3A4(void*);  void  fn_801C3A64(void*);
void  fn_801D603C(void);   void  fn_801FC0A0(void);
void  fn_801FC1EC(void);   void  fn_80142970(void);
void  fn_801B95EC(void);
s32   fn_8015BA30(void*);   void  fn_8014DE68(void);
s32   fn_801CB938(void);    // returns 0x805FB5F8
}

// ---- data ----
static u8                  sEngineCtx[8];        ORIG(0x805FB760)
static void*               sMemPool;             ORIG(0x805FB7A8)
static EngineMemCallbacks  sSgMemCallbacks;      ORIG(0x8047C600)
static EngineMemCallbacks  sRendMemCallbacks;    ORIG(0x8047C614)
extern u8                  gCamera[];            ORIG(0x8047C640)
extern u8                  gWorldState[];        ORIG(0x803E6D00) // byte at 0xF5: split-update mode
extern u8                  lbl_8040D500[];
extern u8                  lbl_8047C760[], lbl_8047C790[], lbl_8047C7AC[];
extern u8                  lbl_80401108[];
s32                        gRendInitialized;     ORIG(0x805F9610)
s32                        gRendTexelsInMEM2;    ORIG(0x805F95F8) // >= 0: texels/CLUTs get flag 0x80
s32                        gUseAltRenderSetup;   ORIG(0x805F9214)
View*                      gMainView;            ORIG(0x805FB75C)
static EngineFrameCallback sFrameCallback;       ORIG(0x805FB784)
static s32                 sEngineSuspended;     ORIG(0x805FB788)
static s32                 sEngineSuspendRequest; ORIG(0x805FB78C)
u8                         gMainHeap[0x40];      ORIG(0x8047CAC0) // size unknown
extern u8                  lbl_8047C7D8[], lbl_8047C7F0[];
static s32                 sPendingWorldReset;   ORIG(0x805FB790)
static GameSettings*       sGameSettings;        ORIG(0x805FB794)
extern s32                 gHomeMenuOpen;        ORIG(0x805FB51C)
extern s32                 gViewportCount;       ORIG(0x805FB4C0)
extern s32                 gViewportIndex;       ORIG(0x805FB4BC)
extern s32                 gFrameBudget;         ORIG(0x805FB4C8)
extern s32                 gArgc;                ORIG(0x805FB77C)
extern char**              gArgv;                ORIG(0x805FB780)

static const char* const sSgMemoryNames[] = {    ORIG(0x80384240)
    "SG_MEMORY_ALBUM", "SG_MEMORY_ANIM", "SG_MEMORY_ANIMPLAYER", "SG_MEMORY_ANIMSET",
    "SG_MEMORY_MODEL", "SG_MEMORY_COLLIDE", "SG_MEMORY_COLLIDE_VERTICES",
    "SG_MEMORY_COLLIDE_SURFACES", "SG_MEMORY_COLLIDE_TREE", "SG_MEMORY_COMPILED_BONES",
    "SG_MEMORY_FRUSTUM", "SG_MEMORY_GROUP", "SG_MEMORY_GROUP_MODEL_LIST", "SG_MEMORY_LIGHT",
    "SG_MEMORY_MESH", "SG_MEMORY_PIVOT", "SG_MEMORY_RIG", "SG_MEMORY_RIG_NODES",
    "SG_MEMORY_SHADER", "SG_MEMORY_SHADER_ANIM_MATRIX", // list continues in the original table
};

static const char* const sRendMemoryNames[] = {  ORIG(0x80384370)
    "REND_MEMORY_BLOCK_HEAP", "REND_MEMORY_DISPLAY_LIST", "REND_MEMORY_THUMBNAIL",
    "REND_MEMORY_COLOR_TABLE", "REND_MEMORY_TRANSFER_GRID",
    "REND_MEMORY_PRECOMPILED_COMMAND_LIST", "REND_MEMORY_TEXELS", "REND_MEMORY_CLUT",
};

enum { REND_MEMORY_TEXELS = 6, REND_MEMORY_CLUT = 7 };
enum { MEM_FLAG_0x80 = 0x80 };   // allocation flag, purpose not yet known (likely "MEM2 / GPU visible")

#define MEM2_POOL_END 0x90400000u

// 0x801D4094
void MemInit(void) {
    if (sMemPool == NULL) {
        sMemPool = SysAllocMEM2Block(0x3380000);
        // The heap runs up to the fixed address global_binary.sir is loaded at.
        HeapInit((u8*)gMainHeap, (u8*)sMemPool, MEM2_POOL_END - (u32)(uintptr_t)sMemPool);
    }
}

// 0x801D5638
void CoreInit(void* ctx) {
    (void)ctx;
    fn_80146A74();
    fn_8014826C();
    CameraInit(gCamera);
    fn_801B402C(lbl_8047C7D8);
}

// 0x801D5674
void CoreShutdown(void* ctx) {
    (void)ctx;
    fn_801FD1F0(lbl_8047C7F0);
    fn_80148308();
    fn_80146A78();
}

// 0x801D1504
extern "C" void EngineStartup(void* ctx) {
    if (sMemPool == NULL) MemInit();
    CoreInit(ctx);
}

// 0x801D1538
extern "C" void EngineCleanup(void* ctx) {
    CoreShutdown(ctx);
    SysFreeMEM2Block(sMemPool);
}

// 0x801D1564
static void* SgMemAlloc(u32 size, u32 align, int type, const char* name, u32 extra) {
    char tag[0x108];
    SysSprintf(tag, "%s: %s", sSgMemoryNames[type], name);
    return MemAlloc(size, align, MEM_FLAG_0x80, tag, extra);
}

// 0x801D15DC
static void SgMemFree(void* ptr) {
    if (ptr) MemFree(ptr);
}

// 0x801D15EC
static void* EngineMemAllocTemp(u32 size) {
    return MemAlloc(size, 16, 0, "T*E*M*P", 0);
}

// 0x801D1600
static void* EngineMemRealloc(void* ptr, u32 size) {
    return MemRealloc(ptr, size, 0);
}

// 0x801D1608
static void EngineMemFree(void* ptr) {
    if (ptr) MemFree(ptr);
}

// 0x801D1618
static void* RendMemAlloc(u32 size, u32 align, int type, const char* name, u32 extra) {
    char tag[0x108];
    SysSprintf(tag, "%s: %s", sRendMemoryNames[type], name);
    u32 flags = 0;
    if (gRendTexelsInMEM2 >= 0 && (u32)(type - REND_MEMORY_TEXELS) <= 1) flags = MEM_FLAG_0x80;
    return MemAlloc(size, align, flags, tag, extra);
}

// 0x801D16D0
static void RendMemFree(void* ptr) {
    if (ptr) MemFree(ptr);
}

// 0x801D16E0
extern "C" void EngineInit(void) {
    sRendMemCallbacks.alloc     = RendMemAlloc;
    sRendMemCallbacks.free      = RendMemFree;
    sRendMemCallbacks.allocTemp = EngineMemAllocTemp;
    sRendMemCallbacks.realloc   = EngineMemRealloc;
    sRendMemCallbacks.freeAny   = EngineMemFree;
    RendInit(&sRendMemCallbacks);
    gRendInitialized = 1;
    RendSetFlagA(TRUE);

    sSgMemCallbacks.alloc     = SgMemAlloc;
    sSgMemCallbacks.free      = SgMemFree;
    sSgMemCallbacks.allocTemp = EngineMemAllocTemp;
    sSgMemCallbacks.realloc   = EngineMemRealloc;
    sSgMemCallbacks.freeAny   = EngineMemFree;
    fn_80162C1C();
    SgInit(&sSgMemCallbacks);

    gMainView = ViewCreate();
    CameraSetFov(gCamera, 0.9272953f);   // atan(4/3): 53.13 degrees
    lbl_8040D500[0] = 1;                  // via an inlined identity getter (0x8015EC9C)
    SetRenderEnabled(TRUE);
    RendBeginFrame();
    fn_801FBE04();
    if (gUseAltRenderSetup) fn_80150014();
    else                    fn_8014855C();

    GameStart(gArgc, gArgv);
    RendEndFrame();
}

// 0x801D17FC
extern "C" void EngineShutdown(void) {
    ViewDestroy(gMainView);
    SgShutdown();
    fn_80162C28();
}

// 0x801D1828
void EngineSetFrameCallback(EngineFrameCallback cb) {
    sFrameCallback = cb;
}

// 0x801D19E8
// Applies a pending suspend/resume request. On resume, renders four cleared frames
// so stale framebuffer contents never reach the screen.
static void EngineApplySuspend(void) {
    if (sEngineSuspended == sEngineSuspendRequest) return;

    if (sEngineSuspendRequest == 1) {
        fn_80151014();
        fn_80151244();
        RendBeginFrame();
    } else {
        for (int i = 0; i < 4; i++) {
            if (i > 0) RendBeginFrame();
            RendClear(7, 0, 0, 1.0f);
            fn_80150014();
            fn_801D603C();
            fn_801FC0A0();
            fn_801FC1EC();
            RendEndFrame();
        }
    }
    sEngineSuspended = sEngineSuspendRequest;
}

// 0x801D1AA0
static s32 EngineIsSuspended(void) {
    return sEngineSuspended;
}

// 0x801D1AA8
static void EngineSuspendedTick(void) {
    if (gViewportCount) {
        s32 used = fn_8015BA30(lbl_80401108 + gViewportIndex * 0x94);
        if (used + 0x10C0 > gFrameBudget) fn_8014DE68();
    }
}

// 0x801D1BAC
static f32 WorldGetField0C(const u8* world) {
    return *(const f32*)(world + 0xC);
}

// 0x801D1BB4
static void EngineRunFrameCallback(void) {
    if (sFrameCallback) sFrameCallback();
}

// 0x801D1838
extern "C" void EngineFrame(void) {
    if (!gHomeMenuOpen) fn_801FBE68();
    EngineApplySuspend();

    if (!EngineIsSuspended()) {
        RendBeginFrame();
        CameraUpdate(gCamera);

        if (gWorldState[0xF5]) {
            if (fn_801CB938()) {
                fn_800446BC(gCamera, 0);
                fn_80170044(gMainView);
            }
            fn_801D1AFC();
            fn_8000F298();
            fn_8016FB20(gMainView);
            fn_80170044(gMainView);
        } else {
            if (fn_801CB938()) {
                fn_800446BC(gCamera, 0);
                fn_80170044(gMainView);
            }
            fn_801D1AFC();
            fn_8000F298();
        }

        CameraEndFrame(gCamera);

        u8 frameState[0x1B0];
        fn_801D1BCC(frameState, gMainView);
        if (!EngineGetGameSettings()->skipDebugOverlay) {
            fn_8018CD70(fn_8013F38C(), 1);
            fn_8018CD70(fn_8013F38C(), 2);
        }
        if (fn_801CB938()) fn_80170044(gMainView);
        fn_801D2388(gMainView, frameState);
        fn_8016FB20(gMainView);

        if (sPendingWorldReset) {
            fn_801CB414(lbl_8047C760, 0);
            WorldGetField0C(gWorldState);   // result unused
            fn_801BE3A4(lbl_8047C790);
            fn_801C3A64(lbl_8047C7AC);
            sPendingWorldReset = 0;
        }

        EngineRunFrameCallback();
        DebugDrawStats();
        fn_80150014();
        fn_801D603C();
        fn_801FC0A0();
        fn_801FC1EC();
        RendEndFrame();
    } else {
        fn_8000F298();
        EngineRunFrameCallback();
        EngineSuspendedTick();
    }

    fn_80142970();
    fn_801B95EC();
}

// 0x801D2B44
GameSettings* EngineGetGameSettings(void) {
    return sGameSettings;
}

// 0x801D2B4C
void EngineSetGameSettings(GameSettings* settings) {
    sGameSettings = settings;
}
