#pragma once
// REND: the renderer module. Game code records GX commands into display lists ("RendDL"),
// which are handed to a dedicated render thread that kicks them to the GPU and flips
// frame buffers on vertical retrace.
//
// PC port note: GX display lists are captured by temporarily redirecting the CPU write-gather
// FIFO into the display list's memory. The platform layer replaces this whole file's backend
// (src/platform/gx) with a command recorder that replays into OpenGL/Vulkan at any resolution.
#include "types.h"
#include "revolution.h"
#include "engine/engine.h"

#define REND_DL_COUNT      17          // ring of display-list buffers
#define REND_DL_SIZE       0x24000     // 144 KB each
#define REND_CMD_QUEUE_LEN 64          // render-thread command ring

// One recorded submission of a display list (0x14 bytes).
struct RendDLRecord {
    GXFifoObj* fifo;      // 0x00: the display list's FIFO object
    u8*        base;      // 0x04: command data
    u32        unk8;      // 0x08
    u32        size;      // 0x0C: bytes recorded
    u32        capacity;  // 0x10
};

// Simple memory writer used instead of the hardware FIFO when non-NULL.
struct RendMemWriter {
    u8* start;
    u8* cur;
};

// A display-list buffer (0x94 bytes).
struct RendDL {
    u8*            base;     // 0x00
    u32            size;     // 0x04
    u32            used;     // 0x08
    GXFifoObj      fifo;     // 0x0C
    RendDLRecord*  record;   // 0x8C: record being written, NULL when idle
    RendMemWriter* writer;   // 0x90
};

// Video / GX context (gVideo). Only the fields used so far are named.
struct RendVideo;
extern RendVideo gVideo;   // 0x805FB558 (small-data label; real object spans 0x805FB530..)

// Render-thread commands (RendThreadCmd::type).
enum RendCmdType {
    REND_CMD_RUN_DL       = 0,   // execute a display list; arg = signal "DL free" when done
    REND_CMD_WAIT_VSYNC   = 1,   // wait until N retraces passed since last flip
    REND_CMD_FLIP         = 2,   // show frame buffer arg
    REND_CMD_FRAME_DONE   = 3,   // mark frame arg finished, update GPU-time stats
    REND_CMD_TIMER_START  = 4,
    REND_CMD_SET_INTERVAL = 5,   // set retraces per frame (1 = 60 Hz, 2 = 30 Hz)
    REND_CMD_SET_BLACK    = 6,   // arg = 0 to blank the screen
    REND_CMD_SYNC         = 7,   // no-op; used to drain the queue
    REND_CMD_NOP          = 8,
    REND_CMD_QUIT         = 9,
};

extern "C" {
// rend_init.cpp
void RendInit(EngineMemCallbacks* mem);                   // 0x8014E664
// rend_dl.cpp
void RendDLCreate(RendDL* dl, u32 size);                  // 0x8015B998
void RendDLBegin(RendDL* dl, RendDLRecord* rec);          // 0x8015B9F8
void RendDLEnd(RendDL* dl);                               // 0x8015BA4C
u8*  RendDLGetBase(RendDL* dl);                           // 0x8015BBB8
// rend_thread.cpp
void RendThreadInit(RendVideo* video, s32 freeDLs);       // 0x8015E4CC
void RendThreadSubmitDL(RendDLRecord* rec, BOOL signalFree);   // 0x8015E5D0
void RendThreadSync(void);                                // 0x8015E930
void RendWaitFreeDL(void);                                // 0x8015EA34
BOOL RendHasFreeDL(void);                                 // 0x8015EAF4
// video.cpp
void* VideoGetFrameBuffer(RendVideo* video, int index);   // 0x8015DE38
s32   VideoGetWidth(RendVideo* video);                    // 0x8015DE48
s32   VideoGetHeight(RendVideo* video);                   // 0x8015DE50
void  VideoInitDLFifo(RendVideo* video, GXFifoObj* fifo, void* base, u32 size);   // 0x8015D3AC
void  VideoBeginCapture(RendVideo* video, GXFifoObj* fifo, void* base);          // 0x8015D444
u8*   VideoEndCapture(RendVideo* video, GXFifoObj* fifo);                         // 0x8015D4BC
void  VideoRunDL(RendVideo* video, RendDLRecord* rec);                            // 0x8015D2EC
}

extern EngineMemCallbacks* gRendMem;   // 0x805FB4A8
