// Display-list buffers and the GX FIFO switching used to record them.
// Original: RendDL* 0x8015B998 - 0x8015BBC0, Video FIFO helpers 0x8015D2EC - 0x8015D530
// (auto_03_80156AAC_text.s)
//
// Recording works by pointing the CPU write-gather FIFO at the display list's buffer, so every
// GX call made in between lands in that buffer. Playback points the GPU's read FIFO at it.
#include <string.h>

#include "engine/rend.h"
#include "system/sys.h"

// ---- video FIFO state (shared with the rest of video.cpp, not yet decompiled) ----
extern GXFifoObj*  gVideoMainFifo;      ORIG(0x805FB530) // the normal immediate-mode FIFO
extern u8          gVideoCapturing;     ORIG(0x805FB540)
extern OSSemaphore gVideoDrawDone;      ORIG(0x8040AE30) // signalled by the draw-done callback
extern OSSemaphore gVideoGXLock;        ORIG(0x8040AE3C) // guards FIFO switching

// GX "PE done" token written at the end of every display list (BP register 0x45).
#define GX_LOAD_BP_REG   0x61
#define BP_PE_DONE_TOKEN 0x45000002

// 0x8015D2EC
// Executed on the render thread: run one recorded display list and wait for the GPU to finish it.
extern "C" void VideoRunDL(RendVideo* video, RendDLRecord* rec) {
    (void)video;
    OSWaitSemaphore(&gVideoGXLock);
    GXSaveGPFifo(gVideoMainFifo);
    GXInitFifoPtrs(rec->fifo, rec->base, rec->base + rec->size);
    GXSetGPFifo(rec->fifo);
    OSSignalSemaphore(&gVideoGXLock);

    OSWaitSemaphore(&gVideoDrawDone);

    OSWaitSemaphore(&gVideoGXLock);
    GXSetGPFifo(gVideoMainFifo);
    u8 overhi, underlow, readIdle, cmdIdle, brkpt;
    do {
        GXGetGPStatus(&overhi, &underlow, &readIdle, &cmdIdle, &brkpt);
    } while (!readIdle || !cmdIdle);
    OSSignalSemaphore(&gVideoGXLock);
}

// 0x8015D3AC
extern "C" void VideoInitDLFifo(RendVideo* video, GXFifoObj* fifo, void* base, u32 size) {
    (void)video;
    OSWaitSemaphore(&gVideoGXLock);
    DCInvalidateRange(base, size);
    GXSaveCPUFifo(gVideoMainFifo);
    GXSaveGPFifo(gVideoMainFifo);
    GXInitFifoBase(fifo, base, size);
    GXSetCPUFifo(gVideoMainFifo);
    GXSetGPFifo(gVideoMainFifo);
    OSSignalSemaphore(&gVideoGXLock);
}

// 0x8015D444
extern "C" void VideoBeginCapture(RendVideo* video, GXFifoObj* fifo, void* base) {
    (void)video;
    OSWaitSemaphore(&gVideoGXLock);
    GXInitFifoPtrs(fifo, base, base);
    GXSaveCPUFifo(gVideoMainFifo);
    GXSetCPUFifo(fifo);
    OSSignalSemaphore(&gVideoGXLock);
    gVideoCapturing = TRUE;
}

// 0x8015D4BC
// Returns the write pointer, i.e. the end of the recorded commands.
extern "C" u8* VideoEndCapture(RendVideo* video, GXFifoObj* fifo) {
    (void)video;
    OSWaitSemaphore(&gVideoGXLock);
    GXFlush();
    GXSaveCPUFifo(fifo);
    GXSetCPUFifo(gVideoMainFifo);
    void* readPtr;
    void* writePtr;
    GXGetFifoPtrs(fifo, &readPtr, &writePtr);
    OSSignalSemaphore(&gVideoGXLock);
    gVideoCapturing = FALSE;
    return (u8*)writePtr;
}

// 0x8015B998
extern "C" void RendDLCreate(RendDL* dl, u32 size) {
    dl->size   = size;
    dl->base   = (u8*)MEMAllocFromAllocator(&gSysAllocator, size);
    dl->used   = 0;
    dl->record = NULL;
    dl->writer = NULL;
    VideoInitDLFifo(&gVideo, &dl->fifo, dl->base, dl->size);
}

// 0x8015B9F8
extern "C" void RendDLBegin(RendDL* dl, RendDLRecord* rec) {
    dl->record    = rec;
    rec->fifo     = &dl->fifo;
    rec->capacity = dl->size;
    rec->base     = dl->base;
    VideoBeginCapture(&gVideo, &dl->fifo, dl->base);
}

// 0x8015BA4C
extern "C" void RendDLEnd(RendDL* dl) {
    RendMemWriter* w = dl->writer;
    if (w != NULL) {
        // Software writer: append the PE-done token by hand and pad to 32 bytes with GX NOPs.
        *w->cur++ = GX_LOAD_BP_REG;
        w->cur[0] = (u8)(BP_PE_DONE_TOKEN >> 24);
        w->cur[1] = (u8)(BP_PE_DONE_TOKEN >> 16);
        w->cur[2] = (u8)(BP_PE_DONE_TOKEN >> 8);
        w->cur[3] = (u8)(BP_PE_DONE_TOKEN);
        w->cur += 4;

        RendMemWriter* writer = dl->writer;
        u32 misalign = (u32)(writer->cur - writer->start) & 0x1F;
        if (misalign != 0) {
            u32 pad = 0x20 - misalign;
            memset(writer->cur, 0, pad);
            writer->cur += pad;
        }

        dl->record->size = (u32)(dl->writer->cur - dl->writer->start);
        if (dl->record->capacity == 0) {
            dl->record->capacity = dl->record->size;
            dl->used += dl->record->capacity;
        }
        DCFlushRange(dl->record->base, dl->record->size);
    } else {
        // Hardware path: the token goes straight through the write-gather pipe.
        GXWGFifo._u8  = GX_LOAD_BP_REG;
        GXWGFifo._u32 = BP_PE_DONE_TOKEN;
        u8* end = VideoEndCapture(&gVideo, &dl->fifo);
        dl->record->size = (u32)(end - dl->record->base);
        dl->used = dl->record->size;
    }
    dl->writer = NULL;
    dl->record = NULL;
}

// 0x8015BBB8
extern "C" u8* RendDLGetBase(RendDL* dl) {
    return dl->base;
}
