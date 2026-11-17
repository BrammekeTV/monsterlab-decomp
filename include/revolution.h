#pragma once
// Subset of the Revolution (Wii) SDK used by the game.
// On PC these are implemented by the platform layer (src/platform/), not decompiled.
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- OS ----
typedef s64 OSTime;
typedef struct OSContext OSContext;
typedef u16 OSError;
typedef void (*OSErrorHandler)(OSError error, OSContext* context, u32 dsisr, u32 dar);
typedef void (*OSResetCallback)(void);
typedef void (*OSPowerCallback)(void);
typedef struct { u8 r, g, b, a; } GXColor;

enum {
    OS_ERROR_SYSTEM_RESET = 0, OS_ERROR_MACHINE_CHECK = 1, OS_ERROR_DSI = 2,
    OS_ERROR_ISI = 3, OS_ERROR_EXTERNAL_INTERRUPT = 4, OS_ERROR_ALIGNMENT = 5,
    OS_ERROR_PROGRAM = 6, OS_ERROR_FLOATING_POINT = 7, OS_ERROR_DECREMENTER = 8,
    OS_ERROR_SYSTEM_CALL = 9, OS_ERROR_TRACE = 10, OS_ERROR_PERFORMANCE_MONITOR = 11,
    OS_ERROR_BREAKPOINT = 12, OS_ERROR_SYSTEM_INTERRUPT = 13, OS_ERROR_THERMAL_INTERRUPT = 14,
    OS_ERROR_PROTECTION = 15, OS_ERROR_FPE = 16,
};

void   OSInit(void);
OSTime OSGetTime(void);
u32    OSGetBusClock(void);  // *(u32*)0x800000F8 on hardware
void   OSReport(const char* fmt, ...);
void   OSFatal(GXColor fg, GXColor bg, const char* msg);
void   OSDumpContext(OSContext* context);
u32*   OSGetStackPointer(void);
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler);
OSResetCallback OSSetResetCallback(OSResetCallback cb);
OSPowerCallback OSSetPowerCallback(OSPowerCallback cb);
void   OSRestart(u32 resetCode);
void   OSReturnToMenu(void);
void   OSShutdownSystem(void);

void*  OSGetMEM1ArenaLo(void);
void*  OSGetMEM1ArenaHi(void);
void   OSSetMEM1ArenaLo(void* lo);
void   OSSetMEM1ArenaHi(void* hi);
void*  OSGetMEM2ArenaLo(void);
void*  OSGetMEM2ArenaHi(void);
void   OSSetMEM2ArenaLo(void* lo);
void*  OSInitAlloc(void* lo, void* hi, int maxHeaps);
int    OSCreateHeap(void* lo, void* hi);
int    OSSetCurrentHeap(int heap);

#define OS_TIMER_CLOCK (OSGetBusClock() / 4)
#define OSTicksToMicroseconds(ticks) (((ticks) * 8) / (OS_TIMER_CLOCK / 125000))

// ---- MEM ----
typedef struct MEMiHeapHead* MEMHeapHandle;
typedef struct MEMAllocator {
    const void* func;
    void* heap;
    u32 heapParam1;
    u32 heapParam2;
} MEMAllocator;

MEMHeapHandle MEMCreateExpHeapEx(void* start, u32 size, u16 opt);
void  MEMInitAllocatorForExpHeap(MEMAllocator* allocator, MEMHeapHandle heap, s32 align);
void  MEMInitAllocatorForOSHeap(MEMAllocator* allocator, int heapID);
void* MEMAllocFromAllocator(MEMAllocator* allocator, u32 size);
void  MEMFreeToAllocator(MEMAllocator* allocator, void* block);

// ---- SC / DVD / AI / AX / WPAD / KPAD ----
void SCInit(void);
s32  SCCheckStatus(void);
BOOL DVDInit(void);
void AIInit(u8* stack);
void AXInit(void);
void KPADInit(void);
typedef void* (*WPADAlloc)(u32 size);
typedef BOOL (*WPADFree)(void* ptr);
void WPADRegisterAllocator(WPADAlloc alloc, WPADFree free);

#ifdef __cplusplus
}
#endif
