// PC port runtime: maps the Wii address space at its original addresses, loads the data sections
// of the user's own main.dol into it, routes calls made through original code addresses to the
// decompiled functions, and reports the first thing that is still missing.
// Design: docs/port_runtime.md. No original code here.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"

// This file's start-up object must be built before every other static object: game code may
// touch globals that live in the mapped address space from its own static initialisers.
#pragma comment(lib, "winmm.lib")
#pragma warning(disable : 4074)
#pragma init_seg(compiler)

extern "C" u32 _ctors[];      // 0x80345AE0, resolved by port_gen.obj like every other Wii global

PortOptions gPortOptions;

// ------------------------------------------------------------------ log and boot summary
struct Milestone { const char* what; u32 count; u32 ms; BOOL stub; };
static Milestone sMilestones[512];
static s32       sMilestoneCount;
static DWORD     sStartTick;
static s32       sFrames;
static LONG      sStopping;
static s32       sSkippedCtors, sRunCtors;
static HANDLE    sMainThread;

void PortLog(const char* fmt, ...) {
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    fprintf(stderr, "[port] %s\n", line);
    fflush(stderr);
}

static Milestone* FindMilestone(const char* what, BOOL stub) {
    for (s32 i = 0; i < sMilestoneCount; i++) {
        if (sMilestones[i].what == what || strcmp(sMilestones[i].what, what) == 0) return &sMilestones[i];
    }
    if (sMilestoneCount == 512) return &sMilestones[511];
    Milestone* m = &sMilestones[sMilestoneCount++];
    m->what = what;
    m->count = 0;
    m->ms = GetTickCount() - sStartTick;
    m->stub = stub;
    return m;
}

BOOL PortMilestone(const char* what) {
    Milestone* m = FindMilestone(what, FALSE);
    if (m->count++ == 0) {
        PortLog("%s", what);
        return TRUE;
    }
    return FALSE;
}

void PortStubCall(const char* name) {
    Milestone* m = FindMilestone(name, TRUE);
    if (m->count++ == 0 || gPortOptions.verbose) PortLog("SDK stub: %s (does nothing on PC yet)", name);
}

static void PrintMissing(void);
static void PrintSummary(void) {
    char buf[256];
    fprintf(stderr, "\n==== boot summary ====\n");
    fprintf(stderr, "ran for %.1f s, %d frame(s) presented\n", (GetTickCount() - sStartTick) / 1000.0, sFrames);
    if (sRunCtors || sSkippedCtors)
        fprintf(stderr, "static initialisers: %d run, %d skipped (not decompiled)\n", sRunCtors, sSkippedCtors);
    fprintf(stderr, "disc: %s\n", PortDvdStats(buf, sizeof(buf)));
    fprintf(stderr, "reached, in order:\n");
    for (s32 i = 0; i < sMilestoneCount; i++) {
        if (!sMilestones[i].stub) fprintf(stderr, "  %6u ms  %s\n", sMilestones[i].ms, sMilestones[i].what);
    }
    PrintMissing();
    s32 stubs = 0;
    for (s32 i = 0; i < sMilestoneCount; i++) stubs += sMilestones[i].stub;
    if (stubs) {
        fprintf(stderr, "SDK calls that do nothing yet (%d functions):\n ", stubs);
        s32 col = 1;
        for (s32 i = 0; i < sMilestoneCount; i++) {
            if (!sMilestones[i].stub) continue;
            col += fprintf(stderr, " %s x%u", sMilestones[i].what, sMilestones[i].count);
            if (col > 100) { fprintf(stderr, "\n "); col = 1; }
        }
        fprintf(stderr, "\n");
    }
    fflush(stderr);
}

void PortStop(int exitCode, const char* fmt, ...) {
    if (InterlockedExchange(&sStopping, 1)) {       // another thread is already stopping
        Sleep(INFINITE);
    }
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    fflush(stdout);
    if (!gPortOptions.selfTest || exitCode) PrintSummary();
    fprintf(stderr, "[port] STOP: %s\n", line);
    fflush(stderr);
    TerminateProcess(GetCurrentProcess(), (UINT)exitCode);
}

void PortFrameDone(void) {
    sFrames++;
    if (gPortOptions.frameLimit && sFrames >= gPortOptions.frameLimit) {
        PortStop(0, "frame limit reached (%d frames)", sFrames);
    }
}

// ------------------------------------------------------------------ function table
struct PcFunc { u32 pc; const PortFunc* f; };
static PcFunc* sByPc;
static s32     sByPcCount;
static u32     sImageLo, sImageHi;     // this exe

const char* PortFuncName(const PortFunc* f) { return gPortNames + f->name; }

const PortFunc* PortFindFunc(u32 addr) {
    s32 lo = 0, hi = (s32)gPortFuncCount - 1;
    while (lo <= hi) {
        s32 mid = (lo + hi) / 2;
        if (gPortFuncs[mid].addr == addr) return &gPortFuncs[mid];
        if (gPortFuncs[mid].addr < addr) lo = mid + 1; else hi = mid - 1;
    }
    return NULL;
}

static const PortFunc* FindFuncContaining(u32 addr) {
    s32 lo = 0, hi = (s32)gPortFuncCount - 1, best = -1;
    while (lo <= hi) {
        s32 mid = (lo + hi) / 2;
        if (gPortFuncs[mid].addr <= addr) { best = mid; lo = mid + 1; } else hi = mid - 1;
    }
    return best < 0 ? NULL : &gPortFuncs[best];
}

BOOL PortIsWiiAddr(u32 a) {
    return (a >= PORT_MEM1_BASE && a < PORT_MEM1_BASE + PORT_MEM1_SIZE) ||
           (a >= PORT_MEM2_BASE && a < PORT_MEM2_BASE + PORT_MEM2_SIZE);
}

// Names of every function in this exe, statics included, from the linker's map file
// (monsterlab.map beside the exe); loaded when the first report needs them.
struct MapSym { u32 pc; char name[60]; };
static MapSym* sMap;
static s32     sMapCount = -1;

static int CompareMap(const void* a, const void* b) {
    u32 x = ((const MapSym*)a)->pc, y = ((const MapSym*)b)->pc;
    return x < y ? -1 : x > y;
}

static void LoadMap(void) {
    sMapCount = 0;
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* dot = strrchr(path, '.');
    if (!dot) return;
    strcpy(dot, ".map");
    FILE* f = fopen(path, "r");
    if (!f) return;
    s32 cap = 0;
    char line[1024], name[512];
    while (fgets(line, sizeof(line), f)) {
        unsigned sec, off, addr;
        if (sscanf(line, " %x:%x %511s %x", &sec, &off, name, &addr) != 4) continue;
        if (addr < sImageLo + 0x1000 || addr >= sImageHi || sec != 1) continue;
        if (sMapCount == cap) { cap = cap ? cap * 2 : 4096; sMap = (MapSym*)realloc(sMap, cap * sizeof(MapSym)); }
        const char* n = name[0] == '_' ? name + 1 : name;
        if (n[0] == '?') {                           // C++ name: keep the identifier
            n++;
            char* at = (char*)strchr(n, '@');
            if (at) *at = 0;
        }
        sMap[sMapCount].pc = addr;
        snprintf(sMap[sMapCount].name, sizeof(sMap[sMapCount].name), "%s", n);
        sMapCount++;
    }
    fclose(f);
    qsort(sMap, sMapCount, sizeof(MapSym), CompareMap);
}

static int ComparePc(const void* a, const void* b) {
    u32 x = ((const PcFunc*)a)->pc, y = ((const PcFunc*)b)->pc;
    return x < y ? -1 : x > y;
}

const char* PortDescribePc(const void* pcPtr, char* buf, u32 n) {
    u32 pc = (u32)(uintptr_t)pcPtr;
    if (pc >= PORT_TEXT_LO && pc < PORT_TEXT_HI) {
        const PortFunc* f = FindFuncContaining(pc);
        if (f) snprintf(buf, n, "original code %s+0x%X (0x%08X)", PortFuncName(f), pc - f->addr, pc);
        else snprintf(buf, n, "original code 0x%08X", pc);
        return buf;
    }
    if (pc < sImageLo || pc >= sImageHi) {
        snprintf(buf, n, "0x%08X (%s)", pc, PortIsWiiAddr(pc) ? "Wii data, not code" : pc < 0x10000 ? "null" : "system library");
        return buf;
    }
    if (sMapCount < 0) LoadMap();
    s32 lo = 0, hi = sMapCount - 1, best = -1;
    while (lo <= hi) {
        s32 mid = (lo + hi) / 2;
        if (sMap[mid].pc <= pc) { best = mid; lo = mid + 1; } else hi = mid - 1;
    }
    if (best >= 0) {
        snprintf(buf, n, "%s+0x%X", sMap[best].name, pc - sMap[best].pc);
        return buf;
    }
    lo = 0, hi = sByPcCount - 1, best = -1;
    while (lo <= hi) {
        s32 mid = (lo + hi) / 2;
        if (sByPc[mid].pc <= pc) { best = mid; lo = mid + 1; } else hi = mid - 1;
    }
    if (best < 0 || pc - sByPc[best].pc > 0x4000) snprintf(buf, n, "exe+0x%X (port or static code)", pc - sImageLo);
    else snprintf(buf, n, "%s+0x%X", PortFuncName(sByPc[best].f), pc - sByPc[best].pc);
    return buf;
}

static const char* RegionOf(u32 addr) {
    static const struct { u32 lo, hi; const char* name; } kRegions[] = {
        {0x80004000, 0x80006B40, "start-up code"}, {0x80006B40, 0x8022FEC4, "game code"},
        {0x8022FEC4, 0x80232AF8, "zlib"}, {0x80232AF8, 0x80241000, "SDK: OS"},
        {0x80241000, 0x80244000, "SDK: EXI/SI/DB"}, {0x80244000, 0x80253000, "SDK: GX/VI"},
        {0x80253000, 0x80257700, "SDK: DVD"}, {0x80257700, 0x80264000, "SDK: AI/AX/MEM"},
        {0x80264000, 0x8026E000, "SDK: NAND/SC/IPC"}, {0x8026E000, 0x802A4000, "SDK: WPAD/KPAD/BT"},
        {0x802A4000, 0x80323600, "SDK: network"}, {0x80323600, 0x8033F400, "MSL C/C++ runtime"},
        {0x8033F400, PORT_TEXT_HI, "debugger stub"},
    };
    for (const auto& r : kRegions) if (addr >= r.lo && addr < r.hi) return r.name;
    return "?";
}

// ------------------------------------------------------------------ traps
// Test hook: when set, a call to a missing function calls the hook and returns to the caller.
static void (*sMissingHook)(u32 addr);
static u32  sHookAddr;
static void HookLanding(void) { sMissingHook(sHookAddr); }

// --keep-going: a missing function returns 0 and the run continues, to list everything the boot
// path still needs. What happens after the first one is not what the game does.
struct Missing { const PortFunc* f; u32 count; char from[96]; };
static Missing sMissing[256];
static s32     sMissingCount;
static u32 __cdecl ReturnZero(void) { return 0; }

static void PrintMissing(void) {
    if (!sMissingCount) return;
    fprintf(stderr, "missing functions hit, in order (each was skipped and returned 0):\n");
    for (s32 i = 0; i < sMissingCount; i++) {
        const PortFunc* f = sMissing[i].f;
        fprintf(stderr, "  %-28s 0x%08X %5u bytes  %-18s x%-5u from %s\n", PortFuncName(f), f->addr, f->size,
                RegionOf(f->addr), sMissing[i].count, sMissing[i].from);
    }
}

static BOOL NoteMissing(const PortFunc* f, u32 ret) {
    for (s32 i = 0; i < sMissingCount; i++) {
        if (sMissing[i].f == f) { sMissing[i].count++; return FALSE; }
    }
    if (sMissingCount == 256) return FALSE;
    Missing* m = &sMissing[sMissingCount++];
    m->f = f;
    m->count = 1;
    PortDescribePc((void*)(uintptr_t)ret, m->from, sizeof(m->from));
    return TRUE;
}

static void PrintStack(const CONTEXT* c) {
    char buf[128];
    const u32* sp = (const u32*)(uintptr_t)c->Esp;
    s32 shown = 0;
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(sp, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT) return;
    const u32* end = (const u32*)((u8*)mbi.BaseAddress + mbi.RegionSize);
    for (s32 i = 0; i < 1024 && sp + i < end && shown < 14; i++) {
        u32 v = sp[i];
        if (v < sImageLo + 0x1000 || v >= sImageHi) continue;
        const u8* p = (const u8*)(uintptr_t)v;
        if (!(p[-5] == 0xE8 || p[-2] == 0xFF || p[-3] == 0xFF || p[-6] == 0xFF)) continue;   // after a call
        fprintf(stderr, "[port]   %s %s\n", shown++ ? "          " : "stack:    ", PortDescribePc(p, buf, sizeof(buf)));
    }
}

static LONG CALLBACK TrapHandler(EXCEPTION_POINTERS* e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    CONTEXT* c = e->ContextRecord;
    u32 eip = c->Eip;
    char who[160], from[160];

    if (eip >= PORT_TEXT_LO && eip < PORT_TEXT_HI) {
        const PortFunc* f = PortFindFunc(eip);
        if (f && f->impl) {                      // too short for a jump: redirect here
            c->Eip = (DWORD)(uintptr_t)f->impl;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        u32 ret = *(u32*)(uintptr_t)c->Esp;
        const char* soft = f ? PortSoftSdk(eip) : NULL;
        if (soft && !sMissingHook) {                 // an SDK call that may do nothing for now
            PortStubCall(soft);
            c->Eip = (DWORD)(uintptr_t)ReturnZero;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (f && sMissingHook) {
            sHookAddr = eip;
            c->Eip = (DWORD)(uintptr_t)HookLanding;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (f && gPortOptions.keepGoing) {
            if (NoteMissing(f, ret))
                PortLog("missing #%d: %s (0x%08X, %u bytes, %s), called from %s: returns 0", sMissingCount,
                        PortFuncName(f), eip, f->size, RegionOf(eip), sMissing[sMissingCount - 1].from);
            c->Eip = (DWORD)(uintptr_t)ReturnZero;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (f) {
            BOOL game = eip >= 0x80006B40 && eip < 0x8022FEC4;
            fprintf(stderr, "\n[port] missing function: %s (0x%08X, %u bytes, %s)\n", PortFuncName(f), eip, f->size, RegionOf(eip));
            fprintf(stderr, "[port]   called from %s\n", PortDescribePc((void*)(uintptr_t)ret, from, sizeof(from)));
            PrintStack(c);
            PortStop(3, game ? "%s (0x%08X) is not decompiled yet" : "%s (0x%08X) needs a PC implementation in src/port/",
                     PortFuncName(f), eip);
        }
        fprintf(stderr, "\n[port] jump into the middle of %s\n", PortDescribePc((void*)(uintptr_t)eip, who, sizeof(who)));
        PrintStack(c);
        PortStop(4, "bad jump to 0x%08X", eip);
    }

    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: case EXCEPTION_ILLEGAL_INSTRUCTION: case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO: case EXCEPTION_STACK_OVERFLOW: case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: case EXCEPTION_DATATYPE_MISALIGNMENT:
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (InterlockedCompareExchange(&sStopping, 0, 0)) return EXCEPTION_CONTINUE_SEARCH;
    fprintf(stderr, "\n[port] crash: exception 0x%08lX at %s\n", code, PortDescribePc((void*)(uintptr_t)eip, who, sizeof(who)));
    if (code == EXCEPTION_ACCESS_VIOLATION && e->ExceptionRecord->NumberParameters >= 2) {
        ULONG_PTR kind = e->ExceptionRecord->ExceptionInformation[0];
        fprintf(stderr, "[port]   %s address 0x%08lX\n", kind == 0 ? "reading" : kind == 1 ? "writing" : "executing",
                (unsigned long)e->ExceptionRecord->ExceptionInformation[1]);
    }
    fprintf(stderr, "[port]   eax %08lX ebx %08lX ecx %08lX edx %08lX esi %08lX edi %08lX ebp %08lX esp %08lX\n",
            c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
    PrintStack(c);
    PortStop(4, "crash in %s", who);
    return EXCEPTION_CONTINUE_SEARCH;
}

// Fills the original code range: a jump to the PC function where one is linked, a trap elsewhere.
static void BuildCodeRange(void) {
    u8* p = (u8*)(uintptr_t)PORT_TEXT_LO;
    for (u32 i = 0; i < PORT_TEXT_HI - PORT_TEXT_LO; i += 2) { p[i] = 0x0F; p[i + 1] = 0x0B; }   // ud2
    sByPc = (PcFunc*)malloc(sizeof(PcFunc) * (gPortFuncCount + 1));
    for (u32 i = 0; i < gPortFuncCount; i++) {
        const PortFunc* f = &gPortFuncs[i];
        if (!f->impl) continue;
        sByPc[sByPcCount].pc = (u32)(uintptr_t)f->impl;
        sByPc[sByPcCount++].f = f;
        u32 room = (i + 1 < gPortFuncCount ? gPortFuncs[i + 1].addr : PORT_TEXT_HI) - f->addr;
        if (f->addr < PORT_TEXT_LO || f->addr >= PORT_TEXT_HI || room < 5) continue;
        u8* at = (u8*)(uintptr_t)f->addr;
        at[0] = 0xE9;                            // jmp rel32
        s32 rel = (s32)((u32)(uintptr_t)f->impl - (f->addr + 5));
        memcpy(at + 1, &rel, 4);
    }
    qsort(sByPc, sByPcCount, sizeof(PcFunc), ComparePc);
}

// ------------------------------------------------------------------ main.dol
static u32 sDataEnd;          // end of the executable's data and bss

static u32 Be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

static void Swap(u8* p, u32 size, u32 width) {
    for (u32 i = 0; i + width <= size; i += width) {
        for (u32 a = 0, b = width - 1; a < b; a++, b--) { u8 t = p[i + a]; p[i + a] = p[i + b]; p[i + b] = t; }
    }
}

// A run nobody has typed: text stays as it is, anything else word-aligned is taken as 32-bit words.
static u32 GuessKind(const u8* p, u32 addr, u32 size) {
    if (p[0] != 0) {
        BOOL text = TRUE;
        u32 printable = 0;
        for (u32 i = 0; i < size && text; i++) {
            u8 ch = p[i];
            if (ch >= 0x20 && ch < 0x7F) printable++;
            else if (ch != 0 && ch != '\n' && ch != '\r' && ch != '\t') text = FALSE;
        }
        if (text && printable * 4 >= size) return PORT_DATA_RAW;
    }
    if (size == 2 && addr % 2 == 0) return PORT_DATA_16;
    if (size % 4 == 0 && addr % 4 == 0) return PORT_DATA_32;
    return PORT_DATA_RAW;
}

static void SwapData(u8* base, u32 baseAddr, const PortDataRun* runs, u32 count, u32 lo, u32 hi) {
    for (u32 i = 0; i < count; i++) {
        const PortDataRun* r = &runs[i];
        if (r->addr < lo || r->addr + r->size > hi) continue;
        u8* p = base + (r->addr - baseAddr);
        u32 kind = r->kind == PORT_DATA_AUTO ? GuessKind(p, r->addr, r->size) : r->kind;
        if (kind != PORT_DATA_RAW) Swap(p, r->size, kind);
    }
}

static BOOL LoadDol(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return FALSE;
    u8 hdr[0x100];
    if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) { fclose(f); return FALSE; }
    sDataEnd = Be32(hdr + 0xD8) + Be32(hdr + 0xDC);          // bss
    for (s32 i = 7; i < 18; i++) {                           // data sections only; code is not used
        u32 off = Be32(hdr + i * 4), addr = Be32(hdr + 0x48 + i * 4), size = Be32(hdr + 0x90 + i * 4);
        if (!size) continue;
        if (addr < PORT_MEM1_BASE || addr + size > PORT_MEM1_BASE + PORT_MEM1_SIZE) { fclose(f); return FALSE; }
        if (addr >= PORT_TEXT_LO && addr < PORT_TEXT_HI) continue;       // exception tables inside the code range
        fseek(f, off, SEEK_SET);
        if (fread((void*)(uintptr_t)addr, 1, size, f) != size) { fclose(f); return FALSE; }
        if (addr + size > sDataEnd) sDataEnd = addr + size;
        if (addr == (u32)(uintptr_t)_ctors) Swap((u8*)(uintptr_t)addr, size, 4);     // .ctors has no symbols
        SwapData((u8*)(uintptr_t)addr, addr, gPortData, gPortDataCount, addr, addr + size);
    }
    fclose(f);
    return TRUE;
}

u32 PortArenaLoMEM1(void) {
    u32 end = sDataEnd ? sDataEnd : 0x80700000;
    return (end + 0x10000 + 0xFFF) & ~0xFFFu;                // the original keeps its stack here
}

// Static initialisers of the original executable (.ctors).
extern "C" void PortRunStaticInit(void) {
    if (!gPortOptions.haveData) return;
    for (const u32* p = _ctors; *p; p++) {
        const PortFunc* f = PortFindFunc(*p);
        if (f && f->impl) {
            sRunCtors++;
            ((void (*)(void))(uintptr_t)*p)();
        } else if (p != _ctors) {                            // the first entry is the C++ exception set-up
            sSkippedCtors++;
            if (gPortOptions.verbose)
                PortLog("static initialiser %s (0x%08X) is not decompiled: skipped", f ? PortFuncName(f) : "?", *p);
        }
    }
    PortLog("static initialisers: %d run, %d skipped", sRunCtors, sSkippedCtors);
}

// ------------------------------------------------------------------ the big lock
static SRWLOCK            sLock = SRWLOCK_INIT;
static CONDITION_VARIABLE sLockCv = CONDITION_VARIABLE_INIT;
static u32                sNextTicket, sServing;      // FIFO: the holder is the thread with ticket sServing
static SRWLOCK            sChangeLock = SRWLOCK_INIT;
static CONDITION_VARIABLE sChangeCv = CONDITION_VARIABLE_INIT;
static u32                sChangeGen;
static u32                sIntrEnabled = 1;

void PortLockEnter(void) {
    AcquireSRWLockExclusive(&sLock);
    u32 mine = sNextTicket++;
    while (mine != sServing) SleepConditionVariableSRW(&sLockCv, &sLock, INFINITE, 0);
    ReleaseSRWLockExclusive(&sLock);
}

void PortLockLeave(void) {
    AcquireSRWLockExclusive(&sLock);
    sServing++;
    ReleaseSRWLockExclusive(&sLock);
    WakeAllConditionVariable(&sLockCv);
}

void PortYield(void) {
    if (!sIntrEnabled) return;
    if (sNextTicket - sServing > 1) {            // someone is waiting
        PortLockLeave();
        PortLockEnter();
    }
}

void PortWaitChange(void) {
    u32 gen = sChangeGen;
    PortLockLeave();
    AcquireSRWLockExclusive(&sChangeLock);
    while (sChangeGen == gen) SleepConditionVariableSRW(&sChangeCv, &sChangeLock, 20, 0);
    ReleaseSRWLockExclusive(&sChangeLock);
    PortLockEnter();
}

void PortSignalChange(void) {
    AcquireSRWLockExclusive(&sChangeLock);
    sChangeGen++;
    ReleaseSRWLockExclusive(&sChangeLock);
    WakeAllConditionVariable(&sChangeCv);
}

u32 PortDisableInterrupts(void) {
    u32 was = sIntrEnabled;
    sIntrEnabled = 0;
    return was;
}

void PortRestoreInterrupts(u32 state) {
    sIntrEnabled = state;
    if (state) PortYield();
}

struct Queued { void (*fn)(void*); void* arg; };
static Queued sQueue[256];
static u32    sQueueHead, sQueueTail;      // touched only with the lock held

void PortQueueInterrupt(void (*fn)(void*), void* arg) {
    sQueue[sQueueTail++ % 256] = {fn, arg};
}

static DWORD WINAPI InterruptThread(void*) {
    for (;;) {
        Sleep(1);
        PortLockEnter();
        if (sIntrEnabled) {
            while (sQueueHead != sQueueTail) {
                Queued q = sQueue[sQueueHead++ % 256];
                q.fn(q.arg);
            }
            PortOsTick();
            PortViTick();
        }
        PortLockLeave();
    }
}

static DWORD WINAPI WatchdogThread(void*) {
    Sleep((DWORD)gPortOptions.secondsLimit * 1000);
    char where[160] = "?";
    SuspendThread(sMainThread);
    CONTEXT c;
    c.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(sMainThread, &c)) {
        PortDescribePc((void*)(uintptr_t)c.Eip, where, sizeof(where));
        fprintf(stderr, "\n[port] time limit: the main thread is at %s\n", where);
        PrintStack(&c);
    }
    PortStop(5, "time limit of %d s reached before the frame limit; main thread at %s", gPortOptions.secondsLimit, where);
    return 0;
}

extern "C" void PortStartInterrupts(void) {
    static BOOL started;
    if (started) return;
    started = TRUE;
    timeBeginPeriod(1);
    CloseHandle(CreateThread(NULL, 0x40000, InterruptThread, NULL, 0, NULL));
    if (gPortOptions.secondsLimit > 0) CloseHandle(CreateThread(NULL, 0x10000, WatchdogThread, NULL, 0, NULL));
}

// ------------------------------------------------------------------ self-test
static u32  sTestMissing;
static void TestMissingHook(u32 addr) { sTestMissing = addr; }

static int SelfTest(void) {
    int fails = 0;
#define CHECK(cond) do { if (!(cond)) { fails++; PortLog("self-test FAILED: %s (line %d)", #cond, __LINE__); } } while (0)
    // Address space.
    *(u32*)(uintptr_t)(PORT_MEM1_BASE + 0x100) = 0x12345678;
    *(u32*)(uintptr_t)(PORT_MEM2_BASE + PORT_MEM2_SIZE - 4) = 0x9ABCDEF0;
    CHECK(*(u8*)(uintptr_t)(PORT_MEM1_BASE + 0x100) == 0x78);
    CHECK(*(u32*)(uintptr_t)(PORT_MEM2_BASE + PORT_MEM2_SIZE - 4) == 0x9ABCDEF0);
    // A global the exe does not define resolves to its original address.
    CHECK((u32)(uintptr_t)_ctors == PORT_CTORS);
    // Function table: sorted, and most decompiled functions are linked.
    CHECK(gPortFuncCount > 1000);
    u32 linked = 0, tiny = 0, missing = 0;
    BOOL sorted = TRUE;
    for (u32 i = 0; i < gPortFuncCount; i++) {
        const PortFunc* f = &gPortFuncs[i];
        if (i && f->addr <= gPortFuncs[i - 1].addr) sorted = FALSE;
        if (f->impl) linked++;
        if (f->impl && f->size == 4 && !tiny) tiny = f->addr;
        if (!f->impl && !missing && f->addr >= 0x80006B40 && f->size) missing = f->addr;
    }
    CHECK(sorted);
    CHECK(linked > 1000);
    // A call through an original address reaches the decompiled function (jump, and trap for
    // functions too short to hold a jump).
    const PortFunc* mainFn = PortFindFunc(0x80142DE0);       // SysGetRequestedExitMode: reads one global
    CHECK(mainFn && mainFn->impl);
    if (mainFn && mainFn->impl) {
        CHECK(*(u8*)(uintptr_t)0x80142DE0 == 0xE9);
        CHECK(((int (*)(void))(uintptr_t)0x80142DE0)() == ((int (*)(void))mainFn->impl)());
    }
    if (tiny) ((void (*)(void))(uintptr_t)tiny)();           // a 4-byte function: returns at once
    // A call to a function that is not decompiled is caught and named.
    if (missing) {
        sMissingHook = TestMissingHook;
        ((void (*)(void))(uintptr_t)missing)();
        sMissingHook = NULL;
        CHECK(sTestMissing == missing);
    }
    // Byte order of initialised data, by type.
    u8 img[20] = {0, 0, 0, 5, 0x12, 0x34, 'a', 'b', 0x3F, 0x80, 0, 0, 'h', 'i', 0, 0, 0, 0, 1, 0};
    static const PortDataRun runs[] = {{0x1000, 4, PORT_DATA_32}, {0x1004, 2, PORT_DATA_16}, {0x1006, 2, PORT_DATA_RAW},
                                       {0x1008, 4, PORT_DATA_AUTO}, {0x100C, 4, PORT_DATA_AUTO}, {0x1010, 4, PORT_DATA_AUTO}};
    SwapData(img, 0x1000, runs, 6, 0x1000, 0x1014);
    u32 w; u16 h; f32 fl;
    memcpy(&w, img, 4); memcpy(&h, img + 4, 2); memcpy(&fl, img + 8, 4);
    CHECK(w == 5 && h == 0x1234 && img[6] == 'a' && fl == 1.0f);
    CHECK(img[12] == 'h' && img[13] == 'i');                 // text is left alone
    memcpy(&w, img + 16, 4);
    CHECK(w == 0x100);
    fails += PortSelfTestSdk();
#undef CHECK
    PortLog("self-test: %u functions, %u linked; %s", gPortFuncCount, linked, fails ? "FAILED" : "ok");
    return fails;
}

// ------------------------------------------------------------------ start-up
static BOOL FileExists(const char* path) {
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static const char* FindDataDir(const char* asked) {
    static char dir[MAX_PATH], probe[MAX_PATH + 32];
    if (asked) {
        snprintf(probe, sizeof(probe), "%s\\sys\\main.dol", asked);
        return FileExists(probe) ? asked : NULL;
    }
    char start[2][MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, start[0]);
    GetModuleFileNameA(NULL, start[1], MAX_PATH);
    for (s32 s = 0; s < 2; s++) {
        char* base = start[s];
        for (s32 up = 0; up < 8; up++) {
            snprintf(dir, sizeof(dir), "%s\\orig\\RM7P4F", base);
            snprintf(probe, sizeof(probe), "%s\\sys\\main.dol", dir);
            if (FileExists(probe)) return dir;
            char* slash = strrchr(base, '\\');
            if (!slash) break;
            *slash = 0;
        }
    }
    return NULL;
}

struct PortBoot {
    PortBoot() {
        sStartTick = GetTickCount();
        gPortOptions.frameLimit = 300;
        gPortOptions.secondsLimit = 60;
        const char* asked = getenv("MONSTERLAB_DATA");
        for (int i = 1; i < __argc && __argv; i++) {
            const char* a = __argv[i];
            if (!strcmp(a, "--selftest")) gPortOptions.selfTest = TRUE;
            else if (!strcmp(a, "--verbose")) gPortOptions.verbose = TRUE;
            else if (!strcmp(a, "--keep-going")) gPortOptions.keepGoing = TRUE;
            else if (!strcmp(a, "--frames") && i + 1 < __argc) gPortOptions.frameLimit = atoi(__argv[++i]);
            else if (!strcmp(a, "--seconds") && i + 1 < __argc) gPortOptions.secondsLimit = atoi(__argv[++i]);
            else if (!strcmp(a, "--data") && i + 1 < __argc) asked = __argv[++i];
        }

        // The Wii's two memory blocks at their original addresses. The code range must be executable.
        void* m1 = VirtualAlloc((void*)(uintptr_t)PORT_MEM1_BASE, PORT_MEM1_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        void* m2 = VirtualAlloc((void*)(uintptr_t)PORT_MEM2_BASE, PORT_MEM2_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        DWORD old;
        if (!m1 || !m2 || !VirtualProtect((void*)(uintptr_t)PORT_TEXT_LO, PORT_TEXT_HI - PORT_TEXT_LO, PAGE_EXECUTE_READWRITE, &old)) {
            fprintf(stderr, "[port] cannot map the Wii address space at 0x80000000 / 0x90000000 (error %lu).\n"
                            "       This needs 64-bit Windows; the exe must be linked /LARGEADDRESSAWARE.\n", GetLastError());
            ExitProcess(10);
        }
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery((void*)(uintptr_t)&PortLog, &mbi, sizeof(mbi));
        sImageLo = (u32)(uintptr_t)mbi.AllocationBase;
        const u8* pe = (const u8*)mbi.AllocationBase;
        sImageHi = sImageLo + *(const u32*)(pe + *(const u32*)(pe + 0x3C) + 0x50);    // SizeOfImage

        BuildCodeRange();
        AddVectoredExceptionHandler(1, TrapHandler);
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &sMainThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
        PortLockEnter();                         // the main thread is the first game thread

        const char* dir = FindDataDir(asked);
        if (dir) {
            char path[MAX_PATH + 32];
            snprintf(path, sizeof(path), "%s\\sys\\main.dol", dir);
            if (!LoadDol(path)) {
                fprintf(stderr, "[port] %s is not a usable main.dol\n", path);
                ExitProcess(11);
            }
            gPortOptions.dataDir = dir;
            gPortOptions.haveData = TRUE;
        }
        // Console globals the SDK keeps in low memory (host byte order).
        *(u32*)(uintptr_t)0x80000028 = PORT_MEM1_SIZE;       // physical memory size
        *(u32*)(uintptr_t)0x800000F8 = 243000000;            // bus clock
        *(u32*)(uintptr_t)0x800000FC = 729000000;            // CPU clock

        if (gPortOptions.selfTest) {
            int fails = SelfTest();
            fflush(stderr);
            TerminateProcess(GetCurrentProcess(), fails ? 1 : 0);
        }
        if (!gPortOptions.haveData) {
            fprintf(stderr, "[port] no game data found. Put your own extracted disc in orig/RM7P4F (sys/main.dol, files/),\n"
                            "       or pass --data <dir> / set MONSTERLAB_DATA. Run with --selftest to test without data.\n");
            ExitProcess(2);
        }
        PortLog("game data: %s", dir);
        PortMilestone("main.dol data loaded at the original addresses");
    }
};
static PortBoot sBoot;

// What the original's start-up code does between loading the executable and calling main: start
// the OS, then run the static initialisers. Some of those allocate memory, and the game's
// allocator calls SysInit itself when the heaps do not exist yet, so this must not wait for
// main. It runs after every C++ static initialiser of the PC build (.CRT$XCY).
extern "C" void PortRunStaticInit(void);
static void __cdecl PortPreMain(void) {
    PortOsStart();
    PortStartInterrupts();
    PortRunStaticInit();
}
#pragma section(".CRT$XCY", read)
__declspec(allocate(".CRT$XCY")) static void (__cdecl* const sPreMain)(void) = PortPreMain;
#pragma comment(linker, "/include:_PortPreMainAnchor")
extern "C" const void* const PortPreMainAnchor = &sPreMain;

#endif  // _WIN32
