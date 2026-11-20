// PC platform layer: DVD (the extracted disc's files), VI (the 50 Hz retrace), and the SDK
// libraries that do nothing on PC yet (GX, AX, AI, SC, WPAD, KPAD, NAND): those only count their
// calls and log the first one. No original code. Design: docs/port_runtime.md.
//
// Functions are defined under the names the game code calls them by (symbols.txt), with the
// callers' parameter lists. A name like fn_8024F1E8 is an SDK function nobody has renamed yet;
// the comment says which one it is.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "revolution.h"

int PortSelfTestOs(void);

#define STUB(name) PortStubCall(#name)

extern "C" {

// ------------------------------------------------------------------ DVD
// DVDFileInfo, as the SDK lays it out (0x3C bytes); the game reads the length and the state.
struct DVDFileInfo {
    u8    cbPad0[0x0C];
    s32   state;          // 0x0C: 0 = done, 1 = busy
    u8    cbPad1[0x20];
    u32   startAddr;      // 0x30: on PC, index + 1 into sDvdFiles
    u32   length;         // 0x34
    void* callback;       // 0x38
};
static_assert(sizeof(DVDFileInfo) == 0x3C, "DVDFileInfo layout");
typedef void (*DVDCallback)(s32 result, DVDFileInfo* info);

struct DvdFile { FILE* f; u32 size; char name[120]; };
static DvdFile sDvdFiles[64];
static u32 sDvdOpens, sDvdReads, sDvdMisses;
static u64 sDvdBytes;

BOOL DVDInit(void) {
    PortMilestone("DVDInit");
    return TRUE;
}

s32 DVDGetDriveStatus(void) { return 0; }        // idle, no error

// DVDOpen
s32 fn_8024F1E8(char* path, u8* infoBytes) {
    DVDFileInfo* info = (DVDFileInfo*)infoBytes;
    char host[MAX_PATH + 160];
    const char* rel = path;
    while (*rel == '/' || *rel == '\\') rel++;
    snprintf(host, sizeof(host), "%s\\files\\%s", gPortOptions.dataDir ? gPortOptions.dataDir : ".", rel);
    for (char* c = host; *c; c++) if (*c == '/') *c = '\\';
    FILE* f = fopen(host, "rb");
    if (!f) {
        sDvdMisses++;
        PortLog("disc: %s not found", path);
        return 0;
    }
    for (s32 i = 0; i < 64; i++) {
        DvdFile* d = &sDvdFiles[i];
        if (d->f) continue;
        d->f = f;
        fseek(f, 0, SEEK_END);
        d->size = (u32)ftell(f);
        snprintf(d->name, sizeof(d->name), "disc: opened %s (%u bytes)", path, d->size);
        memset(info, 0, sizeof(DVDFileInfo));
        info->startAddr = (u32)i + 1;
        info->length = d->size;
        sDvdOpens++;
        PortMilestone(d->name);
        return 1;
    }
    fclose(f);
    PortStop(7, "more than 64 disc files open");
    return 0;
}

// DVDClose
void fn_8024F494(u8* infoBytes) {
    DVDFileInfo* info = (DVDFileInfo*)infoBytes;
    if (info->startAddr == 0 || info->startAddr > 64) return;
    DvdFile* d = &sDvdFiles[info->startAddr - 1];
    if (d->f) fclose(d->f);
    d->f = NULL;
    info->startAddr = 0;
}

struct DvdDone { DVDFileInfo* info; s32 result; DVDCallback cb; };
static DvdDone sDvdDone[64];
static u32 sDvdDoneNext;

static void DvdDeliver(void* arg) {
    DvdDone* d = (DvdDone*)arg;
    d->info->state = 0;
    if (d->cb) d->cb(d->result, d->info);
    PortSignalChange();
}

// DVDReadAsyncPrio. The data is read at once; the callback arrives later from the interrupt
// thread, as the drive's interrupt does on the console.
s32 DVDReadAsync(u8* infoBytes, void* addr, s32 len, s32 offset, void* cb, s32 prio) {
    DVDFileInfo* info = (DVDFileInfo*)infoBytes;
    if (info->startAddr == 0 || info->startAddr > 64 || !sDvdFiles[info->startAddr - 1].f) return 0;
    DvdFile* d = &sDvdFiles[info->startAddr - 1];
    if (offset < 0 || (u32)offset > d->size) return 0;
    fseek(d->f, offset, SEEK_SET);
    s32 got = (s32)fread(addr, 1, (size_t)len, d->f);
    sDvdReads++;
    sDvdBytes += (u32)got;
    info->state = 1;
    info->callback = cb;
    DvdDone* done = &sDvdDone[sDvdDoneNext++ % 64];
    done->info = info;
    done->result = got;
    done->cb = (DVDCallback)cb;
    PortQueueInterrupt(DvdDeliver, done);
    return 1;
}

}  // extern "C"

// SDK functions the game calls that have no PC implementation yet and whose effect the boot does
// not depend on: a call logs once, returns 0 and is counted in the boot summary. Listed by
// original address, so a rename in symbols.txt does not matter. Everything not listed here stops
// the run when called. Add an entry only after checking in the asm what the function is.
const char* PortSoftSdk(u32 addr) {
    static const struct { u32 addr; const char* what; } kSoft[] = {
        {0x80264EE8, "audio: mixer init (fn_80264EE8)"},
        {0x80264F70, "audio: sequencer init (fn_80264F70)"},
        {0x8027FF24, "KPADReset (fn_8027FF24)"},
    };
    for (const auto& e : kSoft) if (e.addr == addr) return e.what;
    return NULL;
}

const char* PortDvdStats(char* buf, u32 n) {
    snprintf(buf, n, "%u file(s) opened, %u read(s), %.1f MB read, %u file(s) not found",
             sDvdOpens, sDvdReads, sDvdBytes / 1048576.0, sDvdMisses);
    return buf;
}

extern "C" {

// ------------------------------------------------------------------ VI
static VIRetraceCallback sPreRetrace, sPostRetrace;
static u32   sRetraceCount;
static DWORD sNextRetraceMs;
static BOOL  sFramePending;

void VIInit(void) { PortMilestone("VIInit"); }
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb) { VIRetraceCallback p = sPreRetrace; sPreRetrace = cb; return p; }
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb) { VIRetraceCallback p = sPostRetrace; sPostRetrace = cb; return p; }
void VISetNextFrameBuffer(void* fb) { sFramePending = TRUE; }
void VISetBlack(BOOL black) { STUB(VISetBlack); }
u32  VIGetTvFormat(void) { return 1; }           // PAL

void VIFlush(void) {
    if (!sFramePending) return;
    sFramePending = FALSE;
    PortMilestone("first frame presented (VISetNextFrameBuffer + VIFlush)");
    PortFrameDone();
}

// ------------------------------------------------------------------ GX (nothing is drawn yet)
volatile PPCWGPipe GXWGFifo;
static GXFifoObj sGxFifo;
struct PortFifo { void* base; u32 size; void* rd; void* wr; };

GXFifoObj* GXInit(void* base, u32 size) {
    PortMilestone("GXInit");
    GXInitFifoBase(&sGxFifo, base, size);
    return &sGxFifo;
}
void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size) {
    PortFifo* f = (PortFifo*)fifo;
    f->base = f->rd = f->wr = base;
    f->size = size;
}
void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr) {
    PortFifo* f = (PortFifo*)fifo;
    f->rd = readPtr;
    f->wr = writePtr;
}
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr, void** writePtr) {
    PortFifo* f = (PortFifo*)fifo;
    *readPtr = f->rd;
    *writePtr = f->wr;
}
void GXSetCPUFifo(GXFifoObj* fifo) { STUB(GXSetCPUFifo); }
void GXSetGPFifo(GXFifoObj* fifo) { STUB(GXSetGPFifo); }
void GXSaveCPUFifo(GXFifoObj* fifo) { STUB(GXSaveCPUFifo); }
void GXSaveGPFifo(GXFifoObj* fifo) { STUB(GXSaveGPFifo); }
void GXGetGPStatus(u8* overhi, u8* underlow, u8* readIdle, u8* cmdIdle, u8* brkpt) {
    *overhi = *underlow = *brkpt = 0;
    *readIdle = *cmdIdle = 1;                    // the "graphics processor" has always caught up
}
void GXFlush(void) { STUB(GXFlush); }
void GXLoadTexMtxImm(const Mtx m, u32 id, int type) { STUB(GXLoadTexMtxImm); }

// ------------------------------------------------------------------ SC, AI, AX, WPAD, KPAD
void SCInit(void) { PortMilestone("SCInit"); }
s32  SCCheckStatus(void) { return 0; }           // settings are loaded
u8   SCGetAspectRatio(void) { return 1; }        // 16:9
u8   SCGetLanguage(void) { return 1; }           // English

void AIInit(u8* stack) { STUB(AIInit); }
void AXInit(void) { STUB(AXInit); }

static WPADAlloc sWpadAlloc;
static WPADFree  sWpadFree;
void WPADRegisterAllocator(WPADAlloc alloc, WPADFree free) { sWpadAlloc = alloc; sWpadFree = free; }
void WPADSetConnectCallback(s32 chan, void (*cb)(s32, s32)) { STUB(WPADSetConnectCallback); }
void WPADControlMotor(s32 chan, u32 command) { STUB(WPADControlMotor); }
void KPADInit(void) { STUB(KPADInit); }

}  // extern "C"

void PortViTick(void) {
    DWORD now = GetTickCount();
    if (sNextRetraceMs == 0) sNextRetraceMs = now + 20;
    if ((s32)(now - sNextRetraceMs) < 0) return;
    sNextRetraceMs += 20;                        // PAL: 50 fields per second
    if ((s32)(now - sNextRetraceMs) > 200) sNextRetraceMs = now + 20;
    sRetraceCount++;
    if (sPreRetrace) sPreRetrace(sRetraceCount);
    if (sPostRetrace) sPostRetrace(sRetraceCount);
    PortSignalChange();
}

// ------------------------------------------------------------------ self-test (no game data)
static s32 sTestResult = -1;
static void TestReadDone(s32 result, DVDFileInfo* info) { sTestResult = result; }
static u32 sTestRetraces;
static void TestRetrace(u32 count) { sTestRetraces++; }

int PortSelfTestSdk(void) {
    int fails = PortSelfTestOs();
#define CHECK(cond) do { if (!(cond)) { fails++; PortLog("self-test FAILED: %s (line %d)", #cond, __LINE__); } } while (0)
    // A disc file: open by its disc path, read part of it into Wii memory, completion by callback.
    char dir[MAX_PATH], sub[MAX_PATH + 32], file[MAX_PATH + 64];
    GetTempPathA(MAX_PATH, dir);
    snprintf(sub, sizeof(sub), "%smonsterlab_selftest_%lu", dir, GetCurrentProcessId());
    CreateDirectoryA(sub, NULL);
    snprintf(file, sizeof(file), "%s\\files", sub);
    CreateDirectoryA(file, NULL);
    snprintf(file, sizeof(file), "%s\\files\\test.bin", sub);
    FILE* f = fopen(file, "wb");
    CHECK(f != NULL);
    if (f) {
        for (int i = 0; i < 4096; i++) fputc(i & 0xFF, f);
        fclose(f);
        const char* savedDir = gPortOptions.dataDir;
        gPortOptions.dataDir = sub;
        u8 info[0x3C];
        u8* dst = (u8*)(uintptr_t)(PORT_MEM2_BASE + 0x02000000);
        CHECK(fn_8024F1E8((char*)"/test.bin", info) == 1);
        CHECK(*(u32*)(info + 0x34) == 4096);
        CHECK(DVDReadAsync(info, dst, 0x100, 0x800, (void*)TestReadDone, 2) == 1);
        for (int i = 0; i < 200 && sTestResult < 0; i++) PortWaitChange();
        CHECK(sTestResult == 0x100 && dst[0] == 0 && dst[0xFF] == 0xFF && *(s32*)(info + 0x0C) == 0);
        fn_8024F494(info);
        CHECK(fn_8024F1E8((char*)"/missing.bin", info) == 0);
        gPortOptions.dataDir = savedDir;
        DeleteFileA(file);
        snprintf(file, sizeof(file), "%s\\files", sub);
        RemoveDirectoryA(file);
        RemoveDirectoryA(sub);
    }
    // The retrace interrupt arrives while the game thread polls the clock.
    VISetPostRetraceCallback(TestRetrace);
    OSTime until = OSGetTime() + OS_TIMER_CLOCK / 5;
    while (OSGetTime() < until && sTestRetraces < 3) {
    }
    CHECK(sTestRetraces >= 3);
    VISetPostRetraceCallback(NULL);
#undef CHECK
    return fails;
}

#endif  // _WIN32
