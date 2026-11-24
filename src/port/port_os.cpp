// PC platform layer: Revolution OS (arenas, heaps, time, threads, semaphores, alarms, reports)
// and the MEM library's expanded heap and allocators. Windows implementations of the SDK
// functions the game calls; no original code. Design: docs/port_runtime.md.
//
// A function is defined under the name the game code calls it by (config/RM7P4F/symbols.txt),
// with the parameter list the callers use: that is the ABI on PC.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <stdlib.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "revolution.h"

extern "C" void PortStartInterrupts(void);

// ------------------------------------------------------------------ heaps
// One block list per heap, kept inside the heap's own memory (as on the Wii), so every pointer
// the game gets is a Wii address. Blocks are in address order and cover the heap.
struct Block {
    u32    size;          // whole block, header included
    u32    used;
    Block* prev;
    Block* next;
};
struct Heap {
    u32    magic;         // 'PHEP'
    u8*    start;
    u8*    end;
    Block* first;
    u32    usedBytes;
    u32    peakBytes;
    u32    allocCount;
    u32    reserved;
};
extern "C" void PortWatchArmThread(void* thread);    // port_runtime.cpp
static const u32 kHeapMagic = 0x50484550;
static const u32 kHeadSize = sizeof(Block);

static Heap* HeapCreate(void* start, void* end) {
    u8* lo = (u8*)(((uintptr_t)start + 31) & ~(uintptr_t)31);
    u8* hi = (u8*)((uintptr_t)end & ~(uintptr_t)31);
    if (hi < lo + sizeof(Heap) + 2 * kHeadSize) return NULL;
    Heap* h = (Heap*)lo;
    memset(h, 0, sizeof(Heap));
    h->magic = kHeapMagic;
    h->start = lo + sizeof(Heap);
    h->end = hi;
    h->first = (Block*)h->start;
    h->first->size = (u32)(hi - h->start);
    h->first->used = 0;
    h->first->prev = h->first->next = NULL;
    return h;
}

// Debugging aid (PORT_HEAP_CHECK=1): walks the block list before every allocation and free and
// stops at the first block whose header was overwritten, naming the block in front of it.
static int sHeapCheck = -1;
static void HeapCheck(Heap* h) {
    if (sHeapCheck < 0) sHeapCheck = getenv("PORT_HEAP_CHECK") != NULL;
    if (!sHeapCheck) return;
    Block* prev = NULL;
    for (Block* b = h->first; b; prev = b, b = b->next) {
        u8* end = (u8*)b + b->size;
        BOOL ok = b->used <= 1 && b->prev == prev && end <= h->end && (b->next ? (u8*)b->next == end : TRUE);
        if (ok) continue;
        PortLog("heap check: block %p is damaged (size %08X used %08X prev %p next %p)", b, b->size, b->used, b->prev, b->next);
        if (prev) {
            u32* w = (u32*)(prev + 1);
            PortLog("heap check: the block in front is %p, size 0x%X, used %u: %08X %08X %08X %08X %08X %08X %08X %08X", prev + 1,
                    prev->size - kHeadSize, prev->used, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]);
        }
        *(volatile int*)1 = 0;
    }
}

static void* HeapAlloc_(Heap* h, u32 size, s32 align) {
    HeapCheck(h);
    BOOL fromEnd = align < 0;                    // MEM: a negative alignment allocates from the top
    u32 a = (u32)(align < 0 ? -align : align);
    if (a < 4) a = 4;
    if (size == 0) size = 1;
    size = (size + 3) & ~3u;
    Block* found = NULL;
    u8* payload = NULL;
    for (Block* b = h->first; b; b = b->next) {
        if (b->used) continue;
        u8* blockEnd = (u8*)b + b->size;
        u8* p;
        if (fromEnd) {
            p = (u8*)(((uintptr_t)(blockEnd - size)) & ~(uintptr_t)(a - 1));
            // The header goes right before the payload; what is left in front must hold a block or nothing.
            while (p >= (u8*)b + kHeadSize && p - kHeadSize != (u8*)b && p - kHeadSize < (u8*)b + kHeadSize) p -= a;
            if (p < (u8*)b + kHeadSize) continue;
        } else {
            p = (u8*)(((uintptr_t)((u8*)b + kHeadSize) + a - 1) & ~(uintptr_t)(a - 1));
            while (p - kHeadSize != (u8*)b && p - kHeadSize < (u8*)b + kHeadSize) p += a;
            if (p + size > blockEnd) continue;
        }
        found = b;
        payload = p;
        if (!fromEnd) break;                     // first fit from the bottom, last fit from the top
    }
    if (!found) return NULL;
    Block* b = found;
    Block* nb = (Block*)(payload - kHeadSize);
    if (nb != b) {                               // split off the free space in front
        nb->size = (u32)((u8*)b + b->size - (u8*)nb);
        nb->prev = b;
        nb->next = b->next;
        if (b->next) b->next->prev = nb;
        b->next = nb;
        b->size = (u32)((u8*)nb - (u8*)b);
        b = nb;
    }
    u32 need = kHeadSize + size;
    if (b->size >= need + kHeadSize + 16) {      // split off the free space behind
        Block* rest = (Block*)((u8*)b + need);
        rest->size = b->size - need;
        rest->used = 0;
        rest->prev = b;
        rest->next = b->next;
        if (b->next) b->next->prev = rest;
        b->next = rest;
        b->size = need;
    }
    b->used = 1;
    h->usedBytes += b->size;
    if (h->usedBytes > h->peakBytes) h->peakBytes = h->usedBytes;
    h->allocCount++;
    return payload;
}

static void HeapFree_(Heap* h, void* ptr) {
    if (!ptr) return;
    HeapCheck(h);
    Block* b = (Block*)((u8*)ptr - kHeadSize);
    if ((u8*)b < h->start || (u8*)b >= h->end || b->used != 1) {
        PortLog("free of a block that is not allocated: %p (ignored)", ptr);
        return;
    }
    b->used = 0;
    h->usedBytes -= b->size;
    h->allocCount--;
    if (b->next && !b->next->used) {
        b->size += b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }
    if (b->prev && !b->prev->used) {
        b->prev->size += b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}

static u32 HeapFreeBytes(Heap* h, BOOL largest) {
    u32 total = 0, best = 0;
    for (Block* b = h->first; b; b = b->next) {
        if (b->used) continue;
        u32 n = b->size - kHeadSize;
        total += n;
        if (n > best) best = n;
    }
    return largest ? best : total;
}

static Heap* HeapOf(void* handle) {
    Heap* h = (Heap*)handle;
    return h && PortIsWiiAddr((u32)(uintptr_t)h) && h->magic == kHeapMagic ? h : NULL;
}

// ---- arenas ----
static u8 *sArena1Lo, *sArena1Hi, *sArena2Lo, *sArena2Hi;

extern "C" {

void* OSGetMEM1ArenaLo(void) { return sArena1Lo; }
void* OSGetMEM1ArenaHi(void) { return sArena1Hi; }
void* OSGetMEM2ArenaLo(void) { return sArena2Lo; }
void* OSGetMEM2ArenaHi(void) { return sArena2Hi; }
void  OSSetMEM1ArenaLo(void* p) { sArena1Lo = (u8*)p; }
void  OSSetMEM1ArenaHi(void* p) { sArena1Hi = (u8*)p; }
void  OSSetMEM2ArenaLo(void* p) { sArena2Lo = (u8*)p; }
void  OSSetMEM2ArenaHi(void* p) { sArena2Hi = (u8*)p; }

// ---- OS heaps (OSAlloc) ----
static Heap* sOsHeaps[8];
static int   sOsHeapMax, sOsCurrentHeap = -1;

void* OSInitAlloc(void* lo, void* hi, int maxHeaps) {
    PortMilestone("OSInitAlloc: MEM1 heap table");
    sOsHeapMax = maxHeaps > 8 ? 8 : maxHeaps;
    memset(sOsHeaps, 0, sizeof(sOsHeaps));
    // The original puts its heap descriptors (12 bytes each) at the start of the arena.
    return (void*)(((uintptr_t)lo + (u32)maxHeaps * 12 + 31) & ~(uintptr_t)31);
}

int OSCreateHeap(void* lo, void* hi) {
    for (int i = 0; i < sOsHeapMax; i++) {
        if (sOsHeaps[i]) continue;
        sOsHeaps[i] = HeapCreate(lo, hi);
        if (!sOsHeaps[i]) return -1;
        PortLog("OSCreateHeap %d: %p - %p (%.1f MB)", i, lo, hi, ((u8*)hi - (u8*)lo) / 1048576.0);
        return i;
    }
    return -1;
}

int OSSetCurrentHeap(int heap) {
    int prev = sOsCurrentHeap;
    sOsCurrentHeap = heap;
    return prev;
}

// MSL malloc and free: the original takes its memory from the current OS heap.
void* fn_80330D80(s32 size) {
    Heap* h = sOsCurrentHeap >= 0 ? sOsHeaps[sOsCurrentHeap] : NULL;
    return h ? HeapAlloc_(h, (u32)size, 8) : NULL;
}
void fn_80330E20(void* p) {
    Heap* h = sOsCurrentHeap >= 0 ? sOsHeaps[sOsCurrentHeap] : NULL;
    if (h) HeapFree_(h, p);
}

// ---- MEM expanded heap and allocators ----
MEMHeapHandle MEMCreateExpHeapEx(void* start, u32 size, u16 opt) {
    Heap* h = HeapCreate(start, (u8*)start + size);
    PortLog("MEMCreateExpHeapEx: %p, %.1f MB", start, size / 1048576.0);
    return (MEMHeapHandle)h;
}

static void* AllocOs(MEMAllocator* a, u32 size) {
    Heap* h = sOsHeaps[(int)(intptr_t)a->heap];
    return h ? HeapAlloc_(h, size, 32) : NULL;
}
static void FreeOs(MEMAllocator* a, void* p) {
    Heap* h = sOsHeaps[(int)(intptr_t)a->heap];
    if (h) HeapFree_(h, p);
}
static void* AllocExp(MEMAllocator* a, u32 size) {
    Heap* h = HeapOf(a->heap);
    return h ? HeapAlloc_(h, size, (s32)a->heapParam1) : NULL;
}
static void FreeExp(MEMAllocator* a, void* p) {
    Heap* h = HeapOf(a->heap);
    if (h) HeapFree_(h, p);
}
struct AllocatorFuncs { void* (*alloc)(MEMAllocator*, u32); void (*free)(MEMAllocator*, void*); };
static const AllocatorFuncs kOsFuncs = {AllocOs, FreeOs};
static const AllocatorFuncs kExpFuncs = {AllocExp, FreeExp};

void MEMInitAllocatorForOSHeap(MEMAllocator* a, int heapID) {
    a->func = &kOsFuncs;
    a->heap = (void*)(intptr_t)heapID;
    a->heapParam1 = a->heapParam2 = 0;
}

void MEMInitAllocatorForExpHeap(MEMAllocator* a, MEMHeapHandle heap, s32 align) {
    a->func = &kExpFuncs;
    a->heap = heap;
    a->heapParam1 = (u32)align;
    a->heapParam2 = 0;
}

void* MEMAllocFromAllocator(MEMAllocator* a, u32 size) {
    void* p = ((const AllocatorFuncs*)a->func)->alloc(a, size);
    if (!p) PortLog("MEMAllocFromAllocator: out of memory for %u bytes", size);
    return p;
}

void MEMFreeToAllocator(MEMAllocator* a, void* block) {
    ((const AllocatorFuncs*)a->func)->free(a, block);
}

// ------------------------------------------------------------------ time
static LARGE_INTEGER sQpcFreq, sQpcStart;

u32 OSGetBusClock(void) { return *(u32*)(uintptr_t)0x800000F8; }

}  // extern "C"

// The game clock (see port.h). Paced: the real clock plus sOffset. Fast-forwarding: sVirt, which
// moves only when the interrupt thread finds every game thread waiting (or a game thread polls
// OSGetTime, which then costs a few microseconds of game time so that a busy-wait ends).
static volatile BOOL sFast;
static s64 sVirt, sOffset, sNextWake;
static volatile LONG sGameThreads = 1;           // the main thread
volatile LONG gPortIdleWaiters;              // game threads waiting in PortWaitChange (port_runtime.cpp)
static const s64 kPollTicks = 100;               // game time per OSGetTime call while fast-forwarding

static s64 RealTicks(void) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    s64 d = now.QuadPart - sQpcStart.QuadPart;
    s64 rate = OSGetBusClock() / 4;
    return d / sQpcFreq.QuadPart * rate + d % sQpcFreq.QuadPart * rate / sQpcFreq.QuadPart;
}

s64  PortTicks(void) { return sFast ? sVirt : RealTicks() + sOffset; }
BOOL PortFastActive(void) { return sFast; }
void PortFastEnd(void) {
    if (!sFast) return;
    sOffset = sVirt - RealTicks();               // the same game time, now moving with the real clock
    sFast = FALSE;
}
void PortGameThreads(s32 delta) { InterlockedAdd(&sGameThreads, delta); }
void PortClockWakeAt(s64 t) { if (!sNextWake || t < sNextWake) sNextWake = t; }

// Interrupt thread, lock held. Every game thread waits and nothing is queued: nothing can happen
// before the next event, so jump to it.
BOOL PortClockAdvanceIfIdle(void) {
    if (!sFast || gPortIdleWaiters < sGameThreads) return FALSE;
    s64 next = 0;
    s64 cand[4] = {PortViNextTicks(), PortAudioNextTicks(), PortOsNextAlarm(), sNextWake};
    for (s64 c : cand) if (c && (!next || c < next)) next = c;
    if (!next || next <= sVirt) return FALSE;
    sVirt = next;
    return TRUE;
}

static void WakeSleepers(void) {
    if (sNextWake && PortTicks() >= sNextWake) { sNextWake = 0; PortSignalChange(); }
}

extern "C" {

OSTime OSGetTime(void) {
    PortYield();                                 // busy-wait loops poll the clock: let interrupts in
    if (sFast) sVirt += kPollTicks;
    return PortTicks();
}

u32 OSGetTick(void) { return (u32)OSGetTime(); }

// WAIT_FOR_INTERRUPT (types.h): the body of a game loop that waits for an interrupt handler to
// change memory. Like a loop that polls the clock: interrupts get in, and game time moves on
// while fast-forwarding.
void PortInterruptPoint(void) { OSGetTime(); }

// OSSleepTicks (64-bit tick count in two words): the SDK sets an alarm and sleeps the thread
// until it fires. Here the thread waits for the clock to reach the deadline.
void OSSleepTicks(s32 hi, u32 lo) {
    OSTime end = PortTicks() + (((OSTime)hi << 32) | lo);
    while (PortTicks() < end) {
        PortClockWakeAt(end);
        PortWaitChange();
    }
}

// ------------------------------------------------------------------ reports and system
void OSReport(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
}

void OSFatal(GXColor fg, GXColor bg, const char* msg) {
    fprintf(stderr, "\n---- OSFatal ----\n%s\n-----------------\n", msg);
    PortStop(6, "the game called OSFatal");
}

void OSDumpContext(OSContext* context) {}
u32* OSGetStackPointer(void) { return NULL; }    // no PowerPC stack frames to walk on PC

static OSErrorHandler  sErrorHandlers[17];
static OSResetCallback sResetCallback;
static OSPowerCallback sPowerCallback;

OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler) {
    if (error > 16) return NULL;
    OSErrorHandler prev = sErrorHandlers[error];
    sErrorHandlers[error] = handler;
    return prev;
}
OSResetCallback OSSetResetCallback(OSResetCallback cb) { OSResetCallback p = sResetCallback; sResetCallback = cb; return p; }
OSPowerCallback OSSetPowerCallback(OSPowerCallback cb) { OSPowerCallback p = sPowerCallback; sPowerCallback = cb; return p; }

void OSRestart(u32 resetCode) { PortStop(0, "the game asked to restart (OSRestart)"); }
void OSReturnToMenu(void) { PortStop(0, "the game asked to return to the Wii Menu"); }
void OSShutdownSystem(void) { PortStop(0, "the game asked to shut down"); }
void OSReturnToDataManager(void) { PortStop(0, "the game asked to open the Wii data manager"); }

void DCFlushRange(void* addr, u32 size) {}
void DCStoreRange(void* addr, u32 size) {}
void DCInvalidateRange(void* addr, u32 size) {}

void OSInit(void) {
    static BOOL done;
    if (done) return;
    done = TRUE;
    PortMilestone("OSInit");                     // the port did the work before main (PortPreMain)
}

// ------------------------------------------------------------------ threads and semaphores
struct PortThread {
    OSThread*    key;
    OSThreadFunc func;
    void*        arg;
    HANDLE       handle;
    BOOL         done;
};
static PortThread sThreads[32];

// For the watchdog: the handle of game thread i, or NULL.
void* PortThreadHandle(int i, void** entry) {
    if (i < 0 || i >= 32 || !sThreads[i].key || sThreads[i].done) return NULL;
    *entry = (void*)sThreads[i].func;
    return sThreads[i].handle;
}

static PortThread* FindThread(OSThread* t, BOOL create) {
    for (PortThread& pt : sThreads) if (pt.key == t) return &pt;
    if (!create) return NULL;
    for (PortThread& pt : sThreads) {
        if (!pt.key) { pt = PortThread(); pt.key = t; return &pt; }
    }
    PortStop(7, "more than 32 OS threads");
    return NULL;
}

static DWORD WINAPI ThreadEntry(void* p) {
    PortThread* pt = (PortThread*)p;
    PortLockEnter();
    pt->func(pt->arg);
    pt->done = TRUE;
    PortGameThreads(-1);
    PortSignalChange();
    PortLockLeave();
    return 0;
}

// OSIsThreadTerminated: 1 when the thread has ended or was never created (state 8 or 0 in the SDK).
s32 OSIsThreadTerminated(OSThread* thread) {
    PortThread* pt = FindThread(thread, FALSE);
    return (!pt || pt->done) ? 1 : 0;
}

BOOL OSCreateThread(OSThread* thread, OSThreadFunc func, void* param, void* stackTop, u32 stackSize, s32 priority, u16 attr) {
    PortThread* pt = FindThread(thread, TRUE);
    pt->func = func;
    pt->arg = param;
    pt->handle = NULL;
    pt->done = FALSE;
    char who[128];
    PortLog("OSCreateThread: %s, priority %d", PortDescribePc((void*)func, who, sizeof(who)), priority);
    return TRUE;
}

s32 OSResumeThread(OSThread* thread) {
    PortThread* pt = FindThread(thread, FALSE);
    if (!pt) return 0;
    if (!pt->handle) {
        // The game's own stack block is not used: PC frames are larger than PowerPC ones.
        PortGameThreads(1);
        pt->handle = CreateThread(NULL, 0x100000, ThreadEntry, pt, 0, NULL);
        PortWatchArmThread(pt->handle);
        return 1;
    }
    return 0;
}

void OSInitSemaphore(OSSemaphore* sem, s32 count) {
    sem->count = count;
    sem->queue.head = sem->queue.tail = NULL;
}

s32 OSWaitSemaphore(OSSemaphore* sem) {
    for (s32 waits = 0; sem->count <= 0; waits++) {
        if (gPortOptions.keepGoing && waits > 25) {
            // Survey mode: whoever should signal this was probably skipped. Do not hang on it.
            static s32 logged;
            if (logged++ < 8) PortLog("semaphore %p was not signalled for 0.5 s: continuing (--keep-going)", (void*)sem);
            return 0;
        }
        PortWaitChange();
    }
    return sem->count--;
}

s32 OSSignalSemaphore(OSSemaphore* sem) {
    s32 count = sem->count++;
    PortSignalChange();
    PortYield();                                 // a waiting thread of higher priority would run now
    return count;
}

s32 OSGetSemaphoreCount(OSSemaphore* sem) { return sem->count; }

// ------------------------------------------------------------------ alarms
// OSAlarm is opaque to the game (it only passes the pointer); the state is kept beside it.
typedef void (*OSAlarmHandler)(u8* alarm, OSContext* context);
struct PortAlarm { u8* key; OSTime fire, period; OSAlarmHandler handler; };
static PortAlarm sAlarms[32];

static PortAlarm* FindAlarm(u8* alarm, BOOL create) {
    for (PortAlarm& a : sAlarms) if (a.key == alarm) return &a;
    if (!create) return NULL;
    for (PortAlarm& a : sAlarms) if (!a.key) { a.key = alarm; return &a; }
    return NULL;
}

// OSSetAlarmUserData / the SDK keeps it at +0x28 of the alarm, where the game's handlers read it.
void fn_80234840(u8* alarm, u8* data) { *(u8**)(alarm + 0x28) = data; }
// OSGetAlarmUserData (0x80234848: lwz r3,0x28(r3)).
u8* fn_80234848(u8* alarm) { return *(u8**)(alarm + 0x28); }

void OSCreateAlarm(u8* alarm) {
    PortAlarm* a = FindAlarm(alarm, TRUE);
    if (a) { a->handler = NULL; a->fire = a->period = 0; }
}

// OSSetAlarm: the handler runs once, `tick` ticks from now (PortOsTick delivers it). The SDK keeps the
// handler in the alarm's first word, which OSCancelAlarm clears.
void OSSetAlarm(void* alarm, s64 tick, void (*handler)(void)) {
    PortAlarm* a = FindAlarm((u8*)alarm, TRUE);
    if (!a) return;
    a->fire = PortTicks() + (tick > 0 ? tick : 1);
    a->period = 0;
    a->handler = (OSAlarmHandler)handler;
    *(u32*)alarm = (u32)(uintptr_t)handler;
}

// OSCancelAlarm: the alarm no longer fires. Its first word (the handler) is cleared, as the SDK does.
void fn_80234418(u8* alarm) {
    PortAlarm* a = FindAlarm(alarm, FALSE);
    if (a) { a->handler = NULL; a->fire = a->period = 0; }
    *(u32*)alarm = 0;
}

// OSDisableInterrupts / OSEnableInterrupts / OSRestoreInterrupts: the interrupt thread delivers
// nothing while they are off. Each returns the state before the call.
s32 OSDisableInterrupts(void) { return (s32)PortDisableInterrupts(); }
s32 OSEnableInterrupts(void) { s32 was = (s32)PortDisableInterrupts(); PortRestoreInterrupts(1); return was; }
s32 OSRestoreInterrupts(s32 level) { s32 was = (s32)PortDisableInterrupts(); PortRestoreInterrupts((u32)(level != 0)); return was; }

}  // extern "C"

s64 PortOsNextAlarm(void) {
    s64 next = 0;
    for (PortAlarm& a : sAlarms) if (a.key && a.handler && (!next || a.fire < next)) next = a.fire ? a.fire : 1;
    return next;
}

void PortOsTick(void) {
    WakeSleepers();
    OSTime now = 0;
    for (PortAlarm& a : sAlarms) {
        if (!a.key || !a.handler) continue;
        if (!now) now = PortTicks();
        if (now < a.fire) continue;
        OSAlarmHandler h = a.handler;
        if (a.period > 0) a.fire += a.period; else a.handler = NULL;
        h(a.key, NULL);
    }
}

void PortOsStart(void) {
    QueryPerformanceFrequency(&sQpcFreq);
    QueryPerformanceCounter(&sQpcStart);
    sFast = gPortOptions.fastAll || gPortOptions.fastForward > 0 || gPortOptions.toMenu;
    // MEM1: from the end of the executable's data to just under the top; MEM2: as on a retail console.
    sArena1Lo = (u8*)(uintptr_t)PortArenaLoMEM1();
    sArena1Hi = (u8*)(uintptr_t)(PORT_MEM1_BASE + PORT_MEM1_SIZE - 0x20000);
    sArena2Lo = (u8*)(uintptr_t)(PORT_MEM2_BASE + 0x800);
    sArena2Hi = (u8*)(uintptr_t)(PORT_MEM2_BASE + 0x033E0000);
}

// ------------------------------------------------------------------ self-test (no game data)
static OSSemaphore sTestSem;
static s32         sTestValue;
static void* TestThread(void* arg) {
    sTestValue = (s32)(intptr_t)arg;
    OSSignalSemaphore(&sTestSem);
    return NULL;
}

int PortSelfTestOs(void) {
    int fails = 0;
#define CHECK(cond) do { if (!(cond)) { fails++; PortLog("self-test FAILED: %s (line %d)", #cond, __LINE__); } } while (0)
    PortOsStart();
    PortStartInterrupts();
    // The same heap set-up SysInit does.
    u8* lo = (u8*)OSGetMEM1ArenaLo();
    u8* hi = (u8*)OSGetMEM1ArenaHi();
    lo = (u8*)OSInitAlloc(lo, hi, 1);
    int heap = OSCreateHeap(lo, hi);
    CHECK(heap == 0);
    MEMAllocator a1, a2;
    MEMInitAllocatorForOSHeap(&a1, heap);
    MEMHeapHandle h2 = MEMCreateExpHeapEx(OSGetMEM2ArenaLo(), (u32)((u8*)OSGetMEM2ArenaHi() - (u8*)OSGetMEM2ArenaLo()), 0);
    MEMInitAllocatorForExpHeap(&a2, h2, 32);
    u32 free0 = HeapFreeBytes(HeapOf(h2), FALSE);
    void* p[64];
    for (int i = 0; i < 64; i++) {
        p[i] = MEMAllocFromAllocator(i & 1 ? &a1 : &a2, 100 + i * 37);
        CHECK(p[i] && ((uintptr_t)p[i] & 31) == 0 && PortIsWiiAddr((u32)(uintptr_t)p[i]));
        memset(p[i], i, 100 + i * 37);
    }
    CHECK(((uintptr_t)p[1] >> 28) == 8 && ((uintptr_t)p[0] >> 28) == 9);   // MEM1 and MEM2 addresses
    for (int i = 0; i < 64; i++) CHECK(*(u8*)p[i] == i);
    for (int i = 0; i < 64; i += 3) MEMFreeToAllocator(i & 1 ? &a1 : &a2, p[i]);
    for (int i = 1; i < 64; i += 3) MEMFreeToAllocator(i & 1 ? &a1 : &a2, p[i]);
    for (int i = 2; i < 64; i += 3) MEMFreeToAllocator(i & 1 ? &a1 : &a2, p[i]);
    CHECK(HeapFreeBytes(HeapOf(h2), FALSE) == free0);                       // everything merged back
    CHECK(HeapFreeBytes(HeapOf(h2), TRUE) == free0);
    void* top = HeapAlloc_(HeapOf(h2), 0x1000, -32);                        // from the top of the heap
    void* bottom = HeapAlloc_(HeapOf(h2), 0x1000, 32);
    CHECK(top > bottom && ((uintptr_t)top & 31) == 0);
    HeapFree_(HeapOf(h2), top);
    HeapFree_(HeapOf(h2), bottom);
    CHECK(HeapFreeBytes(HeapOf(h2), TRUE) == free0);
    // Time moves forward at the console's timer rate.
    OSTime t0 = OSGetTime();
    Sleep(20);
    OSTime t1 = OSGetTime();
    CHECK(t1 - t0 > OS_TIMER_CLOCK / 100 && t1 - t0 < OS_TIMER_CLOCK);
    // A second game thread runs and wakes the first through a semaphore.
    static OSThread thread;
    OSInitSemaphore(&sTestSem, 0);
    OSCreateThread(&thread, TestThread, (void*)(intptr_t)42, NULL, 0, 16, 0);
    OSResumeThread(&thread);
    OSWaitSemaphore(&sTestSem);
    CHECK(sTestValue == 42);
#undef CHECK
    return fails;
}

#endif  // _WIN32
