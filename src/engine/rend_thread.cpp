// Render thread: consumes a 64-entry command ring filled by the game thread, kicks display
// lists to the GPU, flips frame buffers and paces frames on vertical retrace.
// Original: 0x8015E1E0 - 0x8015EB28 (auto_03_80156AAC_text.s)
#include "engine/rend.h"

extern "C" f32 __cvt_ull_flt(u64 v);   // 0x8032FA98 (compiler runtime: u64 -> float)

struct RendThreadCmd {
    s32   type;   // RendCmdType
    void* ptr;    // 0x04
    u32   arg;    // 0x08
};

// Render thread state block (0x8040AEA8).
struct RendThreadState {
    OSThread      thread;                              // 0x0000
    u8            stack[0x2000];                       // 0x0318
    RendThreadCmd queue[REND_CMD_QUEUE_LEN];           // 0x2318
    OSSemaphore   freeSlots;                           // 0x2618: free queue entries
    OSSemaphore   pending;                             // 0x2624: queued commands
    OSSemaphore   slotFreed;                           // 0x2630: pulsed whenever a command finishes
    OSSemaphore   vsync;                               // 0x263C: pulsed on every retrace
    OSSemaphore   freeDLs;                             // 0x2648: display-list buffers available
};
static RendThreadState sRend;                          ORIG(0x8040AEA8)

static s32    sQueueWrite;                             ORIG(0x805FB560)
static s32    sQueueRead;                              ORIG(0x805FB564)
static s32    sRetracesSinceFlip;                      ORIG(0x805FB568)
static s32    sRetracesPerFrame;                       ORIG(0x805FB56C)
static u32    sFrameTag;                               ORIG(0x805FB570) // last finished frame
static u32    sShownFrameTag;                          ORIG(0x805FB574)
static OSTime sLastFrameTime;                          ORIG(0x805FB578)
static f32    sDisplayFps;                             ORIG(0x805FB580) // measured on retrace
static OSTime sGpuBusyTime;                            ORIG(0x805FB588)
static f32    sGpuFps;                                 ORIG(0x805FB590) // 1 / GPU time per frame

static inline f32 TicksToHz(OSTime ticks) {
    return (f32)OS_TIMER_CLOCK / __cvt_ull_flt((u64)ticks);
}

static inline void QueuePush(s32 type, void* ptr, u32 arg) {
    RendThreadCmd* cmd = &sRend.queue[sQueueWrite];
    if (++sQueueWrite >= REND_CMD_QUEUE_LEN) sQueueWrite = 0;
    cmd->type = type;
    cmd->ptr  = ptr;
    cmd->arg  = arg;
}

// 0x8015E1E0
static void RendThreadMain(RendVideo* video) {
    BOOL quit = FALSE;
    while (!quit) {
        OSWaitSemaphore(&sRend.pending);
        RendThreadCmd* cmd = &sRend.queue[sQueueRead];
        if (++sQueueRead >= REND_CMD_QUEUE_LEN) sQueueRead = 0;

        switch (cmd->type) {
        case REND_CMD_RUN_DL:
            VideoRunDL(&gVideo, (RendDLRecord*)cmd->ptr);
            if (cmd->arg) OSSignalSemaphore(&sRend.freeDLs);
            break;
        case REND_CMD_WAIT_VSYNC:
            if (sRetracesSinceFlip < sRetracesPerFrame) {
                // Time spent waiting for retrace doesn't count as GPU time.
                sGpuBusyTime += OSGetTime();
                while (sRetracesSinceFlip < sRetracesPerFrame) OSWaitSemaphore(&sRend.vsync);
                sGpuBusyTime -= OSGetTime();
            }
            break;
        case REND_CMD_FLIP:
            VISetNextFrameBuffer(VideoGetFrameBuffer(&gVideo, cmd->arg));
            VIFlush();
            sRetracesSinceFlip = 0;
            break;
        case REND_CMD_FRAME_DONE:
            sFrameTag = cmd->arg;
            sGpuBusyTime += OSGetTime();
            sGpuFps = TicksToHz(sGpuBusyTime);
            break;
        case REND_CMD_TIMER_START:
            sGpuBusyTime = -OSGetTime();
            break;
        case REND_CMD_SET_INTERVAL:
            sRetracesPerFrame = cmd->arg;
            break;
        case REND_CMD_SET_BLACK:
            VISetBlack(cmd->arg == 0);
            VIFlush();
            break;
        case REND_CMD_QUIT:
            quit = TRUE;
            break;
        default:
            break;
        }

        OSSignalSemaphore(&sRend.freeSlots);
        if (OSGetSemaphoreCount(&sRend.slotFreed) <= 0) OSSignalSemaphore(&sRend.slotFreed);
    }
    (void)video;
}

// 0x8015E3E8
static void* RendThreadEntry(void* param) {
    RendThreadMain((RendVideo*)param);
    return NULL;
}

// 0x8015E40C
static void RendRetraceCallback(u32 retraceCount) {
    (void)retraceCount;
    if (sShownFrameTag != sFrameTag) {
        OSTime now = OSGetTime();
        sDisplayFps = TicksToHz(now - sLastFrameTime);
        sLastFrameTime = now;
        sShownFrameTag = sFrameTag;
    }
    sRetracesSinceFlip++;
    if (OSGetSemaphoreCount(&sRend.vsync) <= 0) OSSignalSemaphore(&sRend.vsync);
}

// 0x8015E4CC
extern "C" void RendThreadInit(RendVideo* video, s32 freeDLs) {
    sFrameTag = 0;
    sRetracesSinceFlip = 1;
    sQueueWrite = 0;
    sQueueRead = 0;
    sRetracesPerFrame = 1;
    sShownFrameTag = 0;
    sLastFrameTime = OSGetTime();
    sDisplayFps = 0.0f;
    sGpuBusyTime = 0;
    sGpuFps = 0.0f;

    OSInitSemaphore(&sRend.freeSlots, REND_CMD_QUEUE_LEN);
    OSInitSemaphore(&sRend.pending, 0);
    OSInitSemaphore(&sRend.slotFreed, 1);
    OSInitSemaphore(&sRend.vsync, 0);
    OSInitSemaphore(&sRend.freeDLs, freeDLs);

    VISetPostRetraceCallback(RendRetraceCallback);
    OSCreateThread(&sRend.thread, RendThreadEntry, video, sRend.stack + sizeof(sRend.stack),
                   sizeof(sRend.stack), 14, OS_THREAD_ATTR_DETACH);
    OSResumeThread(&sRend.thread);
}

// 0x8015E5D0
extern "C" void RendThreadSubmitDL(RendDLRecord* rec, BOOL signalFree) {
    OSWaitSemaphore(&sRend.freeSlots);
    QueuePush(REND_CMD_RUN_DL, rec, signalFree);
    OSSignalSemaphore(&sRend.pending);
}

// 0x8015E930
// Wait until the render thread has drained every queued command.
extern "C" void RendThreadSync(void) {
    while (OSGetSemaphoreCount(&sRend.freeSlots) < REND_CMD_QUEUE_LEN) OSWaitSemaphore(&sRend.slotFreed);

    OSWaitSemaphore(&sRend.freeSlots);
    QueuePush(REND_CMD_SYNC, &sRend, 0);   // original also stores &sRend as arg (unused)
    OSSignalSemaphore(&sRend.pending);

    while (OSGetSemaphoreCount(&sRend.freeSlots) < REND_CMD_QUEUE_LEN) OSWaitSemaphore(&sRend.slotFreed);
}

// 0x8015EA34
extern "C" void RendWaitFreeDL(void) {
    OSWaitSemaphore(&sRend.freeDLs);
}

// 0x8015EAF4
extern "C" BOOL RendHasFreeDL(void) {
    return OSGetSemaphoreCount(&sRend.freeDLs) > 0;
}
