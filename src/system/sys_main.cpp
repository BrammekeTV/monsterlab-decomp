// System bootstrap: entry point, SDK init, heaps, reset/power handling, crash screen.
// Original object: .text 0x801429CC - 0x80142E90 (auto_03_80142500_text.s)
#include <stdio.h>
#include <string.h>

#include "revolution.h"
#include "system/sys.h"

// Unidentified SDK init calls made from SysInit, probably audio mixer / synth setup.
extern "C" void fn_80264EE8(void);
extern "C" void fn_80264F70(void);

// ---- data ----
static BOOL        sInitialized;               ORIG(0x805FB460)
MEMAllocator       gSysAllocator;              ORIG(0x803F2870) // MEM1 OS heap
MEMAllocator       gSysMEM2Allocator;          ORIG(0x803F2880) // MEM2 expanded heap
static SysExitMode sExitMode;                  ORIG(0x805FB45C) // requested by reset/power
u8                 gSysExitEnabled = 1;        ORIG(0x805F90F0)
void SysSetExitMode(SysExitMode mode);
SysEventCallback   gSysEventCallback = (SysEventCallback)SysSetExitMode;   ORIG(0x805F90F4) // lint:ok default handler
char               gSysBuildInfo[64];          ORIG(0x803C5358) // header line on the crash screen
GXColor            gSysFatalBg = {0x00, 0x00, 0xFF, 0x00};   ORIG(0x805FF008)
GXColor            gSysFatalFg = {0xFF, 0xFF, 0xFF, 0x00};   ORIG(0x805FF00C)

static const char* const sErrorNames[] = {     ORIG(0x80375D98)
    "reset", "machine", "dsi", "isiexti", "align", "program", "nofp", "dec",
    "sys", "trace", "perf", "brk", "sysi", "thermi", "protect", "fpe",
};

// 0x801429CC
void* SysAllocMEM2(u32 size) {
    return MEMAllocFromAllocator(&gSysMEM2Allocator, size);
}

// 0x801429DC
BOOL SysFreeMEM2(void* ptr) {
    MEMFreeToAllocator(&gSysMEM2Allocator, ptr);
    return TRUE;
}

// 0x80142A0C
void SysSetExitMode(SysExitMode mode) {
    sExitMode = mode;
}

// 0x80142A14
static void SysResetCallback(void) {
    if (gSysEventCallback) gSysEventCallback(SYS_EVENT_RESET);
}

// 0x80142A30
static void SysPowerCallback(void) {
    if (gSysEventCallback) gSysEventCallback(SYS_EVENT_POWER);
}

// 0x80142EF8
extern "C" int SysSprintf(char* buf, const char* fmt, ...);

// 0x80142A4C
// Fatal exception handler: dumps the context and shows a red/blue OSFatal screen
// with the exception, DSISR/DAR and a backtrace of up to 16 stack frames.
static void SysErrorHandler(OSError error, OSContext* context, u32 dsisr, u32 dar) {
    char msg[0x1018];
    OSDumpContext(context);

    char* p = msg;
    SysSprintf(p, "%s", gSysBuildInfo);
    p += strlen(p);

    const char* name = error < 17 ? sErrorNames[error] : "?";
    SysSprintf(p, "%d (%s) %08x %08x\n", error, name, dsisr, dar);
    p += strlen(p);

    u32* sp = OSGetStackPointer();
    for (u32 depth = 0; sp != NULL && (u32)(uintptr_t)sp != 0xFFFFFFFF && depth++ < 16; sp = (u32*)(uintptr_t)sp[0]) {
        SysSprintf(p, "0x%08x: 0x%08x 0x%08x\n", (u32)(uintptr_t)sp, sp[0], sp[1]);
        p += strlen(p);
    }

    OSFatal(gSysFatalFg, gSysFatalBg, msg);
}


// 0x80142BBC
extern "C" void SysInit(void) {
    if (sInitialized) return;
    sInitialized = TRUE;

    OSInit();
    SCInit();
    while (SCCheckStatus() != 0) {
    }
    DVDInit();

#ifdef PLATFORM_WII
    // Paired-single quantization registers for u8/u16/s8/s16 and the s16 >> 8 format.
    __mtspr(GQR2, 0x00040004);
    __mtspr(GQR3, 0x00050005);
    __mtspr(GQR4, 0x00060006);
    __mtspr(GQR5, 0x00070007);
    __mtspr(GQR6, 0x08040804);
#endif

    // MEM1: a single OS heap covering the whole arena.
    u8* lo = (u8*)OSGetMEM1ArenaLo();
    u8* hi = (u8*)(((uintptr_t)OSGetMEM1ArenaHi()) & ~31u);
    lo = (u8*)OSInitAlloc((void*)(((uintptr_t)lo + 31) & ~31u), hi, 1);
    OSSetMEM1ArenaLo(lo);
    OSSetMEM1ArenaHi(hi);
    int heap = OSCreateHeap(lo, hi);
    OSSetCurrentHeap(heap);
    MEMInitAllocatorForOSHeap(&gSysAllocator, heap);

    // MEM2: one expanded heap; also used by WPAD.
    u8* lo2 = (u8*)OSGetMEM2ArenaLo();
    u8* hi2 = (u8*)OSGetMEM2ArenaHi();
    MEMHeapHandle heap2 = MEMCreateExpHeapEx(lo2, (u32)(hi2 - lo2), 0);
    OSSetMEM2ArenaLo(hi2);
    MEMInitAllocatorForExpHeap(&gSysMEM2Allocator, heap2, 32);

    WPADRegisterAllocator(SysAllocMEM2, SysFreeMEM2);
    KPADInit();
    AIInit(NULL);
    AXInit();
    fn_80264EE8();
    fn_80264F70();

    OSSetResetCallback(SysResetCallback);
    OSSetPowerCallback(SysPowerCallback);

    static const OSError kHandled[] = {
        OS_ERROR_SYSTEM_RESET, OS_ERROR_MACHINE_CHECK, OS_ERROR_DSI, OS_ERROR_ISI,
        OS_ERROR_ALIGNMENT, OS_ERROR_PERFORMANCE_MONITOR, OS_ERROR_SYSTEM_INTERRUPT,
        OS_ERROR_THERMAL_INTERRUPT, OS_ERROR_PROTECTION, OS_ERROR_PROGRAM,
        OS_ERROR_TRACE, OS_ERROR_BREAKPOINT,
    };
    for (OSError e : kHandled) OSSetErrorHandler(e, SysErrorHandler);
}

// 0x80142DC4
SysExitMode SysGetExitMode(void) {
    return gSysExitEnabled ? sExitMode : SYS_EXIT_NONE;
}

// 0x80142DE0
SysExitMode SysGetRequestedExitMode(void) {
    return sExitMode;
}

// 0x80142DE8
// Same as SysSetExitMode; a second copy the linker kept from another translation unit.
extern "C" void SysRequestExit(int mode) {
    sExitMode = (SysExitMode)mode;
}

// 0x80142DF0
int main(int argc, char** argv) {
    SysInit();
    int ret = GameMain(argc, argv);

    switch (SysGetExitMode()) {
    case SYS_EXIT_RESTART:  OSRestart(0);       break;
    case SYS_EXIT_MENU:     OSReturnToMenu();   break;
    case SYS_EXIT_SHUTDOWN: OSShutdownSystem(); break;
    default: break;
    }
    return ret;
}
