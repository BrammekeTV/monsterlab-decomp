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
#include "port/port_input.h"
#include "port/port_gx.h"
#include "port/port_mods.h"
#include "port/port_overlay.h"
extern "C" { s32 SaveManager_SelectFirstEmptySlot(void); void SaveManager_ResetCurrentData(void); void SaveManager_SaveData(void); u32 SaveManager_CanLoadGame(void); }
extern u8* lbl_805FBBE0;
extern "C++" void* gScreenSets;
extern "C" void fn_8022E3D8(void* save, const unsigned char* name);   // src/game/draft_miss37.cpp, draft_miss46.cpp

// This file's start-up object must be built before every other static object: game code may
// touch globals that live in the mapped address space from its own static initialisers.
#pragma comment(lib, "winmm.lib")
#pragma warning(disable : 4074)
#pragma init_seg(compiler)

extern "C" u32 _ctors[];      // 0x80345AE0, resolved by port_gen.obj like every other Wii global

PortOptions gPortOptions;
PortSettings gPortSettings = {1920, 1080, TRUE, FALSE, FALSE, FALSE, 100, FALSE, "", 100, PORT_FILTER_GAME, 0, 0, TRUE};

// ------------------------------------------------------------------ log and boot summary
struct Milestone { const char* what; u32 count; u32 ms; BOOL stub; };
static Milestone sMilestones[512];
static s32       sMilestoneCount;
static DWORD     sStartTick;
static s32       sFrames;
static LONG      sStopping;
static s32       sSkippedCtors, sRunCtors;
static HANDLE    sMainThread;

void PortSamplerStart();
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
static u32 sPeek[8];
static int sPeekCount;
static void PrintSummary(void) {
    char buf[256];
    fprintf(stderr, "\n==== boot summary ====\n");
    fprintf(stderr, "ran for %.1f s, %d frame(s) presented\n", (GetTickCount() - sStartTick) / 1000.0, sFrames);
    if (sRunCtors || sSkippedCtors)
        fprintf(stderr, "static initialisers: %d run, %d skipped (not decompiled)\n", sRunCtors, sSkippedCtors);
    fprintf(stderr, "disc: %s\n", PortDvdStats(buf, sizeof(buf)));
    fprintf(stderr, "data: %s\n", PortFileStats(buf, sizeof(buf)));
    fprintf(stderr, "audio: %s\n", PortAudioStats(buf, sizeof(buf)));
    { char gx[512]; fprintf(stderr, "GX: %s\n", PortGxStatsText(gx, sizeof(gx))); }
    { char gfx[256]; fprintf(stderr, "drawn: %s\n", PortGfxStatsText(gfx, sizeof(gfx))); }
    for (int i = 0; i < sPeekCount; i++) {       // --peek ADDR: four words of Wii memory at the stop
        const u32* p = (const u32*)(uintptr_t)(sPeek[i] & ~3u);
        if (PortIsWiiAddr(sPeek[i])) fprintf(stderr, "peek 0x%08X: %08X %08X %08X %08X\n", sPeek[i] & ~3u, p[0], p[1], p[2], p[3]);
    }
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
    PortPerfFinish();
    if (gPortOptions.screenshot) {
        if (PortGfxSaveScreenshot(gPortOptions.screenshot)) fprintf(stderr, "[port] screenshot: %s\n", gPortOptions.screenshot);
        else fprintf(stderr, "[port] no screenshot: nothing was shown\n");
    }
    if (!gPortOptions.selfTest || exitCode) PrintSummary();
    fprintf(stderr, "[port] STOP: %s\n", line);
    fflush(stderr);
    TerminateProcess(GetCurrentProcess(), (UINT)exitCode);
}

// PORT_SCRIPT_TRACE=1: logs every call the script engine makes to a registered script function.
// The function table (0x80484230: 0x880 entries of {signature, function, name hash}) gets a small
// code thunk in front of each function: push the index, call the logger, jump to the function.
// The x87 register stack. A float result comes back in an x87 register; when the caller's
// declaration says the function returns nothing, the value is never popped. Eight such calls
// fill the stack, and from then on every float result on this thread is the "indefinite" NaN
// (0xFFC00000). PortFpuDepth counts the registers in use; between two statements of the game
// that must be 0.
static u32 sFpuLastIp;                   // address of the last x87 instruction before the check
static u32 PortFpuDepth(void) {
    u32 env[7];
    __asm { fnstenv env }
    __asm { fldenv env }                     // fnstenv masks the exceptions: put the state back
    u32 tags = env[2] & 0xFFFF, n = 0; sFpuLastIp = env[3];
    for (u32 i = 0; i < 8; i++) if (((tags >> (i * 2)) & 3) != 3) n++;
    return n;
}
static void PortFpuClear(void) { __asm { emms } }

static void* sScriptReal[0x880];
static void __cdecl ScriptTraceLog(u32 index) {
    static u32 last = ~0u, repeats;
    static u32 before = ~0u;                // the script function called before this one
    u32 depth = PortFpuDepth();
    if (depth && before != ~0u) {
        char who[160], who2[160];
        fprintf(stderr, "[port] x87 stack: %u value(s) in use after script function %s (%u) and the code up to the next script call: a function that returns a float was called through a declaration that does not (frame %d; last x87 instruction in %s)%c",
                depth, PortDescribePc(sScriptReal[before], who, sizeof(who)), before, (int)sFrames, PortDescribePc((void*)(uintptr_t)sFpuLastIp, who2, sizeof(who2)), 10);
        PortFpuClear();
    }
    before = index;
    if (index == last) { repeats++; return; }
    if (repeats) fprintf(stderr, "[script]   (x%u more)\n", repeats);
    last = index; repeats = 0;
    char who[160];
    fprintf(stderr, "[script] %s (function %u, frame %d)\n", PortDescribePc(sScriptReal[index], who, sizeof(who)), index, (int)sFrames);
}
static void InstallScriptTrace(void) {
    struct Entry { u32 type; void* fn; u32 hash; };
    Entry* table = (Entry*)(uintptr_t)0x80484230;
    u8* code = (u8*)VirtualAlloc(NULL, 0x880 * 24, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!code) return;
    u32 n = 0;
    for (u32 i = 0; i < 0x880; i++) {
        if (!table[i].fn) continue;
        sScriptReal[i] = table[i].fn;
        u8* t = code + i * 24;
        t[0] = 0x68; memcpy(t + 1, &i, 4);                                   // push index
        t[5] = 0xE8; s32 rel = (s32)((u8*)ScriptTraceLog - (t + 10)); memcpy(t + 6, &rel, 4);   // call logger
        t[10] = 0x83; t[11] = 0xC4; t[12] = 0x04;                            // add esp, 4
        t[13] = 0xE9; rel = (s32)((u8*)table[i].fn - (t + 18)); memcpy(t + 14, &rel, 4);       // jmp function
        table[i].fn = t;
        n++;
    }
    PortLog("script trace: %u script functions wrapped", n);
}

s64 gPortWaitTicks;
s32 PortFrameCount(void) { return sFrames; }

// PORT_STATE_LOG=1: one line whenever the game mode, its sub-state and flags, the session state, the world's
// load fields or the ready flag change (what a "Loading" screen that stays is waiting for).
#include "game/game.h"
extern u8 gWorldState[];   // 0x803E6D00
extern u8 gScriptCtx[];    // 0x803C4298
extern u8 sWorldReady;     // 0x805FB2F0
extern "C" { extern u8 gGame[]; }   // 0x803B53A0
extern "C" { extern s32 lbl_803C4E28[]; extern s32 lbl_805FB038; extern s32 lbl_805FB034; }   // combat part zones: loaded flags, wanted, requested
static void PortStateLog(void) {
    static s32 on = -1;
    static s32 last[12];
    if (on < 0) on = getenv("PORT_STATE_LOG") != NULL;
    if (!on) return;
    s32 loaded = 0;
    for (s32 i = 0; i < 10; i++) if (lbl_803C4E28[i]) loaded |= 1 << i;
    s32 now[12] = { gGameMode, sModeTable[0].sub, sModeTable[0].unkC, *(s32*)(gScriptCtx + 0x18), *(s32*)(gScriptCtx + 0x3C),
                   *(s32*)(gWorldState + 0x114), *(s32*)(gWorldState + 0x1A8), sWorldReady,
                    *(s32*)(gWorldState + 0x290), lbl_805FB038, lbl_805FB034, loaded };
    // The HUD's current state machine (HudShowConversation: hud +0x24 -> slot -> machine; +0x00 the stack depth):
    // the machine's key (definition +0x30), its state (machine +0x04) and that state's key (state +0x04).
    static u32 lastFsm[4];
    u32 fsm[4] = { 0, 0, 0, 0 };
    u8* hud = *(u8**)(gGame + 0xD5F0 + 0x44);
    // Only while a level runs: the pointers are stale while one loads (this runs on the render thread).
    if (*(s32*)(gScriptCtx + 0x18) == 5 && sWorldReady && hud != NULL && *(u8***)(hud + 0x24) != NULL && **(u8***)(hud + 0x24) != NULL) {
        u8* m = **(u8***)(hud + 0x24);
        u8* def = *(u8**)m;
        s32 st = *(s32*)(m + 4);
        fsm[0] = (u32)*(s32*)hud;
        if (def != NULL) {
            fsm[1] = *(u32*)(def + 0x30);
            fsm[2] = (u32)st;
            if (st >= 0 && st < *(s32*)(def + 4)) fsm[3] = *(u32*)(*(u8**)def + st * 0x30 + 4);
        }
    }
    if (memcmp(fsm, lastFsm, sizeof(fsm))) {
        memcpy(lastFsm, fsm, sizeof(fsm));
        fprintf(stderr, "[state] frame %d: hud machine %08X (depth %u) state %d key %08X%c", (int)sFrames, fsm[1], fsm[0], (s32)fsm[2], fsm[3], 10);
    }
    if (!memcmp(now, last, sizeof(now))) return;
    memcpy(last, now, sizeof(now));
    fprintf(stderr, "[state] frame %d: mode %d sub %d flags %X, session state %d flags %X, world loading %d zone %d ready %d queue %d, part zones wanted %d asked %d loaded %X%c",
            (int)sFrames, now[0], now[1], now[2], now[3], now[4], now[5], now[6], now[7], now[8], now[9], now[10], now[11], 10);
}

void PortFrameDone(void) {
    if (sFrames == 1 && getenv("PORT_SCRIPT_TRACE")) InstallScriptTrace();
    sFrames++;
    PortStateLog();
    // PORT_MAKE_PROFILE=<frame>: creates a profile in the first free slot and saves it (a test aid until
    // the game saves one itself: the save point in the first level is not reachable yet).
    static s32 sMakeProfile = getenv("PORT_MAKE_PROFILE") ? atoi(getenv("PORT_MAKE_PROFILE")) : 0;
    if (sMakeProfile && sFrames == sMakeProfile) {
        PortLog("profile: slot %d selected", (int)SaveManager_SelectFirstEmptySlot());
        SaveManager_ResetCurrentData();
        fn_8022E3D8(lbl_805FBBE0, (const unsigned char*)"Player");   // the name is what marks the slot as used
        SaveManager_SaveData();
        SaveManager_SaveData();   // the first save of a new file leaves the continue slot at -1 (asm 0x80123B78); the second one sets it
        PortLog("profile: can load %u", (unsigned)SaveManager_CanLoadGame());
    }
    if (getenv("PORT_CURSOR_LOG") && sFrames == 3500) {   // where the game put its cursor widget (screen units, 640 x 480)
        u8* cur = *(u8**)((u8*)gScreenSets + 0x7C);
        if (cur) PortLog("cursor: %.1f %.1f", *(f32*)(cur + 0x68), *(f32*)(cur + 0x6C));
    }
    if (!gPortOptions.fastAll && sFrames >= gPortOptions.fastForward) PortFastEnd();
    if (gPortOptions.bench && !PortFastActive()) {      // the benchmark's 20 s of real time
        static DWORD benchStart;
        if (!benchStart) benchStart = GetTickCount();
        else if (GetTickCount() - benchStart >= 20000) PortStop(0, "benchmark finished (20 s of real time)");
    }
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

// --show-texture: the first time the main thread would stop, it goes to the demo instead.
static DWORD sMainThreadId;
static void __cdecl DemoLanding(void) {
    PortLockLeave();                             // the demo takes the lock itself
    PortDemoMain();
}
static BOOL DemoRedirect(CONTEXT* c, BOOL onMainThread);
static BOOL DemoTakesOver(CONTEXT* c) { return DemoRedirect(c, TRUE); }
static BOOL DemoRedirect(CONTEXT* c, BOOL onMainThread) {
    static LONG taken;
    if (gPortOptions.showTexture < 0 || (onMainThread && GetCurrentThreadId() != sMainThreadId)) return FALSE;
    if (InterlockedExchange(&taken, 1)) return FALSE;
    char who[160];
    PortLog("the boot stopped at %s; --show-texture takes over", PortDescribePc((void*)(uintptr_t)c->Eip, who, sizeof(who)));
    c->Esp = (c->Esp & ~15u) - 64;
    c->Eip = (DWORD)(uintptr_t)DemoLanding;
    return TRUE;
}

// --watch ADDR: a hardware write breakpoint on four bytes of Wii memory, on the game's threads.
// Each write is reported with the function that made it, the new value and the stack.
static u32 sWatch;
static DWORD WINAPI WatchArm(void* thread) {
    CONTEXT c;
    SuspendThread((HANDLE)thread);
    c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext((HANDLE)thread, &c)) {
        c.Dr0 = sWatch & ~3u;
        c.Dr7 = (c.Dr7 & ~0x000F0003u) | 0x000D0001u;    // slot 0: local enable, write, 4 bytes
        SetThreadContext((HANDLE)thread, &c);
    }
    ResumeThread((HANDLE)thread);
    return 0;
}

// The watch covers the game's other threads too: OSCreateThread arms each one it starts.
extern "C" void PortWatchArmThread(void* thread) {
    if (!sWatch) return;
    HANDLE h = CreateThread(NULL, 0, WatchArm, thread, 0, NULL);
    if (h) { WaitForSingleObject(h, INFINITE); CloseHandle(h); }
}

static LONG CALLBACK TrapHandler(EXCEPTION_POINTERS* e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    CONTEXT* c = e->ContextRecord;
    if (code == EXCEPTION_SINGLE_STEP && sWatch && (c->Dr6 & 1)) {
        char buf[160];
        fprintf(stderr, "[port] watch 0x%08X = %08X, written before %s\n", sWatch & ~3u,
                *(const u32*)(uintptr_t)(sWatch & ~3u), PortDescribePc((void*)(uintptr_t)c->Eip, buf, sizeof(buf)));
        PrintStack(c);
        c->Dr6 = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    u32 eip = c->Eip;
    char who[160], from[160];
    if (getenv("PORT_RAW")) {
        char line[96];
        int n = _snprintf(line, sizeof(line), "\n[raw] exc %08lX at 0x%08X, esp 0x%08lX tid %lu\n", code, eip, c->Esp, GetCurrentThreadId());
        DWORD done;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, (DWORD)n, &done, NULL);
    }

    if (eip >= PORT_TEXT_LO && eip < PORT_TEXT_HI) {
        const PortFunc* f = PortFindFunc(eip);
        if (f && f->impl) {                      // too short for a jump: redirect here
            c->Eip = (DWORD)(uintptr_t)f->impl;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (f && !PortSoftSdk(eip) && !sMissingHook && DemoTakesOver(c)) return EXCEPTION_CONTINUE_EXECUTION;
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

    if (code == EXCEPTION_STACK_OVERFLOW) {      // no room for the full report: one raw line
        char line[96];
        int n = _snprintf(line, sizeof(line), "\n[port] stack overflow at 0x%08X, esp 0x%08lX\n", eip, c->Esp);
        DWORD done;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, (DWORD)n, &done, NULL);
        TerminateProcess(GetCurrentProcess(), 4);
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
    if (DemoTakesOver(c)) return EXCEPTION_CONTINUE_EXECUTION;
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

// Bytes of main.dol: from the file, or with no file from the open disc image.
static BOOL DolRead(FILE* f, u32 off, void* dst, u32 size) {
    if (!f) return PortDiscRead(PortDiscDolOffset() + off, dst, size);
    return fseek(f, (long)off, SEEK_SET) == 0 && fread(dst, 1, size, f) == size;
}

static char sDolPath[MAX_PATH + 32];     // the main.dol that was loaded ("" = from the disc image)

static BOOL LoadDol(const char* path) {
    FILE* f = path ? fopen(path, "rb") : NULL;
    if (path && !f) return FALSE;
    snprintf(sDolPath, sizeof(sDolPath), "%s", path ? path : "");
    u8 hdr[0x100];
    if (!DolRead(f, 0, hdr, sizeof(hdr))) { if (f) fclose(f); return FALSE; }
    sDataEnd = Be32(hdr + 0xD8) + Be32(hdr + 0xDC);          // bss
    for (s32 i = 7; i < 18; i++) {                           // data sections only; code is not used
        u32 off = Be32(hdr + i * 4), addr = Be32(hdr + 0x48 + i * 4), size = Be32(hdr + 0x90 + i * 4);
        if (!size) continue;
        if (addr < PORT_MEM1_BASE || addr + size > PORT_MEM1_BASE + PORT_MEM1_SIZE) { if (f) fclose(f); return FALSE; }
        if (addr >= PORT_TEXT_LO && addr < PORT_TEXT_HI) continue;       // exception tables inside the code range
        if (!DolRead(f, off, (void*)(uintptr_t)addr, size)) { if (f) fclose(f); return FALSE; }
        if (addr + size > sDataEnd) sDataEnd = addr + size;
        if (addr == (u32)(uintptr_t)_ctors) Swap((u8*)(uintptr_t)addr, size, 4);     // .ctors has no symbols
        SwapData((u8*)(uintptr_t)addr, addr, gPortData, gPortDataCount, addr, addr + size);
    }
    if (f) fclose(f);
    return TRUE;
}

// Every section of main.dol as it is on the disc (big-endian, code included), at dst + (address - lo):
// the differential tests run the original code from it (port_difftest.cpp). Nothing is converted.
BOOL PortDolImage(u8* dst, u32 lo, u32 hi, u32* secAddr, u32* secSize) {
    if (!gPortOptions.haveData) return FALSE;
    FILE* f = sDolPath[0] ? fopen(sDolPath, "rb") : NULL;
    if (sDolPath[0] && !f) return FALSE;
    u8 hdr[0x100];
    BOOL ok = DolRead(f, 0, hdr, sizeof(hdr));
    for (s32 i = 0; i < 18 && ok; i++) {
        u32 off = Be32(hdr + i * 4), addr = Be32(hdr + 0x48 + i * 4), size = Be32(hdr + 0x90 + i * 4);
        secAddr[i] = addr; secSize[i] = size;
        if (!size) continue;
        ok = addr >= lo && addr + size <= hi && DolRead(f, off, dst + (addr - lo), size);
    }
    if (f) fclose(f);
    return ok;
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

extern volatile LONG gPortIdleWaiters;  // port_os.cpp: waiters registered at the current generation
static HANDLE sIdleEvent;                                    // fast-forward: wakes the interrupt thread when all threads wait

void PortWaitChange(void) {
    if (PortGfxIdle()) return;                   // the drawing thread waits: it showed the last picture again (the caller checks its condition again)
    u32 gen = sChangeGen;
    AcquireSRWLockExclusive(&sChangeLock);
    if (sChangeGen == gen) {                     // a signal clears the count: every waiter wakes and registers again
        gPortIdleWaiters++;
        if (sIdleEvent && PortFastActive()) SetEvent(sIdleEvent);
    }
    ReleaseSRWLockExclusive(&sChangeLock);
    PortLockLeave();
    AcquireSRWLockExclusive(&sChangeLock);
    while (sChangeGen == gen) SleepConditionVariableSRW(&sChangeCv, &sChangeLock, 20, 0);
    ReleaseSRWLockExclusive(&sChangeLock);
    PortLockEnter();
}

void PortSignalChange(void) {
    AcquireSRWLockExclusive(&sChangeLock);
    sChangeGen++;
    gPortIdleWaiters = 0;
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
        if (PortFastActive()) WaitForSingleObject(sIdleEvent, 1);   // fast-forward: woken as soon as every game thread waits
        else Sleep(1);
        PortLockEnter();
        if (sIntrEnabled) {
            while (sQueueHead != sQueueTail) {
                Queued q = sQueue[sQueueHead++ % 256];
                q.fn(q.arg);
            }
            // Fast-forward: nothing is queued and every game thread waits, so game time jumps to the next event.
            if (sQueueHead == sQueueTail) PortClockAdvanceIfIdle();
            PortOsTick();
            PortViTick();
            PortAudioTick();
        }
        PortLockLeave();
    }
}

extern "C" void* PortThreadHandle(int i, void** entry);   // port_os.cpp
static DWORD WINAPI WatchdogThread(void*) {
    Sleep((DWORD)gPortOptions.secondsLimit * 1000);
    char where[160] = "?";
    SuspendThread(sMainThread);
    CONTEXT c;
    c.ContextFlags = CONTEXT_CONTROL;
    // --show-texture: a main thread that loops in game code (it then holds the lock) goes to
    // the demo, and gets the same time again.
    if (gPortOptions.showTexture >= 0 && GetThreadContext(sMainThread, &c) && c.Eip >= sImageLo && c.Eip < sImageHi && DemoRedirect(&c, FALSE)) {
        c.ContextFlags = CONTEXT_CONTROL;
        SetThreadContext(sMainThread, &c);
        ResumeThread(sMainThread);
        Sleep((DWORD)gPortOptions.secondsLimit * 1000);
        SuspendThread(sMainThread);
        c.ContextFlags = CONTEXT_CONTROL;
    }
    if (GetThreadContext(sMainThread, &c)) {
        PortDescribePc((void*)(uintptr_t)c.Eip, where, sizeof(where));
        fprintf(stderr, "\n[port] time limit: the main thread is at %s\n", where);
        PrintStack(&c);
    }
    for (int i = 0; i < 32; i++) {               // and where every other game thread waits
        void* entry = NULL;
        HANDLE h = (HANDLE)PortThreadHandle(i, &entry);
        if (!h) continue;
        SuspendThread(h);
        c.ContextFlags = CONTEXT_CONTROL;
        if (GetThreadContext(h, &c)) {
            char name[160], at[160];
            fprintf(stderr, "[port] thread %s is at %s\n", PortDescribePc(entry, name, sizeof(name)), PortDescribePc((void*)(uintptr_t)c.Eip, at, sizeof(at)));
            PrintStack(&c);
        }
    }
    PortStop(5, "time limit of %d s reached before the frame limit; main thread at %s", gPortOptions.secondsLimit, where);
    return 0;
}

extern "C" void PortStartInterrupts(void) {
    static BOOL started;
    if (started) return;
    started = TRUE;
    timeBeginPeriod(1);
    sIdleEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
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
    fails += PortSelfTestGx();
    fails += PortSelfTestAudio();
    fails += PortSelfTestZlib();
    fails += PortSelfTestOverlay();
    fails += PortSelfTestDisc();
    fails += PortSelfTestPicker();
    fails += PortSelfTestMods();
    fails += PortSelfTestSettings();
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

static const s32 kMenuFrame = 3200;           // --to-menu: fast-forward this many frames

struct PortBoot {
    PortBoot() {
        sStartTick = GetTickCount();
        gPortOptions.frameLimit = 300;
        gPortOptions.showTexture = -1;
        gPortOptions.secondsLimit = 60;
        const char* asked = getenv("MONSTERLAB_DATA");
        BOOL sawFrames = FALSE, sawSeconds = FALSE, noMods = FALSE;
        // monsterlab.ini first, so the command line wins; the self-test runs without it.
        BOOL ini = TRUE;
        for (int i = 1; i < __argc && __argv; i++) if (!strcmp(__argv[i], "--selftest") || !strcmp(__argv[i], "--no-ini")) ini = FALSE;
        if (ini && PortSettingsLoad(PortSettingsPath(), &gPortSettings)) fprintf(stderr, "[port] settings: %s\n", PortSettingsPath());
        for (int i = 1; i < __argc && __argv; i++) {
            const char* a = __argv[i];
            if (!strcmp(a, "--selftest")) gPortOptions.selfTest = TRUE;
            else if (!strcmp(a, "--verbose")) gPortOptions.verbose = TRUE;
            else if (!strcmp(a, "--keep-going")) gPortOptions.keepGoing = TRUE;
            else if (!strcmp(a, "--frames") && i + 1 < __argc) { gPortOptions.frameLimit = atoi(__argv[++i]); sawFrames = TRUE; }
            else if (!strcmp(a, "--fast-forward") && i + 1 < __argc) gPortOptions.fastForward = atoi(__argv[++i]);
            else if (!strcmp(a, "--fast")) gPortOptions.fastAll = TRUE;
            else if (!strcmp(a, "--to-menu")) gPortOptions.toMenu = TRUE;
            else if (!strcmp(a, "--perf-log") && i + 1 < __argc) gPortOptions.perfLog = __argv[++i];
            else if (!strcmp(a, "--bench") && i + 1 < __argc) {
                const char* v = __argv[++i];
                gPortOptions.bench = !strcmp(v, "menu") ? 1 : !strcmp(v, "village") ? 2 : 0;
                if (!gPortOptions.bench) fprintf(stderr, "[port] --bench %s: expected menu or village\n", v);
            }
            else if (!strcmp(a, "--seconds") && i + 1 < __argc) { gPortOptions.secondsLimit = atoi(__argv[++i]); sawSeconds = TRUE; }
            else if ((!strcmp(a, "--data") || !strcmp(a, "--iso")) && i + 1 < __argc) asked = __argv[++i];
            else if (!strcmp(a, "--window")) gPortOptions.window = TRUE;
            else if (!strcmp(a, "--wav") && i + 1 < __argc) gPortOptions.wav = __argv[++i];
            else if (!strcmp(a, "--no-audio")) gPortOptions.noAudio = TRUE;
            else if (!strcmp(a, "--no-movies")) gPortOptions.noMovies = TRUE;
            else if (!strcmp(a, "--peek") && i + 1 < __argc && sPeekCount < 8) sPeek[sPeekCount++] = strtoul(__argv[++i], NULL, 16);
            else if (!strcmp(a, "--watch") && i + 1 < __argc) sWatch = strtoul(__argv[++i], NULL, 16);
            else if (!strcmp(a, "--show-texture") && i + 1 < __argc) gPortOptions.showTexture = atoi(__argv[++i]);
            else if (!strcmp(a, "--screenshot") && i + 1 < __argc) gPortOptions.screenshot = __argv[++i];
            else if (!strcmp(a, "--capture-at") && i + 1 < __argc) gPortOptions.captureAt = atoi(__argv[++i]);
            else if (!strcmp(a, "--stats")) gPortOverlayOn = TRUE;
            else if (!strcmp(a, "--mods") && i + 1 < __argc) PortModsSetDir(__argv[++i]);
            else if (!strcmp(a, "--no-mods")) noMods = TRUE;
            else if (!strcmp(a, "--difftest") && i + 1 < __argc) gPortOptions.diffTest = __argv[++i];
            else if (!strcmp(a, "--dump-data") && i + 1 < __argc) gPortOptions.dumpData = __argv[++i];
            else if (!strcmp(a, "--dump-textures") && i + 1 < __argc) gPortOptions.dumpTextures = __argv[++i];
            else if (!strcmp(a, "--picker-screenshot") && i + 1 < __argc) gPortOptions.pickerShot = __argv[++i];
            else if (!strcmp(a, "--no-ini")) {}
            else if (!strcmp(a, "--settings")) {                // open the settings screen at the start, optionally on a page
                PortGuiSetOpen(TRUE);
                if (i + 1 < __argc && __argv[i + 1][0] != '-') {
                    const char* page = __argv[++i];
                    gPortGuiPage = !strcmp(page, "audio") ? 1 : !strcmp(page, "controls") ? 2 : !strcmp(page, "game") ? 3 : !strcmp(page, "mods") ? 4 : 0;
                }
            }
            else if (!strcmp(a, "--vsync")) gPortSettings.vsync = TRUE;
            else if (!strcmp(a, "--fullscreen")) gPortSettings.fullscreen = TRUE;
            else if (!strcmp(a, "--screenshot-stats")) gPortOptions.screenshotStats = gPortOverlayOn = TRUE;
            else if (!strcmp(a, "--res") && i + 1 < __argc) {
                int w = 0, h = 0;
                if (sscanf(__argv[++i], "%dx%d", &w, &h) == 2 && w >= 320 && h >= 240 && w <= 8192 && h <= 8192) { gPortSettings.width = w; gPortSettings.height = h; }
                else fprintf(stderr, "[port] --res %s: expected WxH, e.g. 1920x1080\n", __argv[i]);
            }
            else if (!strcmp(a, "--render-scale") && i + 1 < __argc) {
                int pct = atoi(__argv[++i]);                    // 50..200 percent; 0.5..2 is taken as a factor
                if (pct > 0 && pct < 10) pct = (int)(atof(__argv[i]) * 100 + 0.5);
                if (pct >= 50 && pct <= 200) gPortSettings.renderScale = pct;
                else fprintf(stderr, "[port] --render-scale %s: expected 50-200 (percent) or 0.5-2\n", __argv[i]);
            }
            else if (!strcmp(a, "--msaa") && i + 1 < __argc) {
                const char* v = __argv[++i];
                int n = !_stricmp(v, "off") ? 0 : atoi(v);
                if (n == 0 || n == 2 || n == 4 || n == 8) gPortSettings.msaa = n;
                else fprintf(stderr, "[port] --msaa %s: expected off, 2, 4 or 8\n", v);
            }
            else if (!strcmp(a, "--fps-cap") && i + 1 < __argc) {
                const char* v = __argv[++i];
                if (!_stricmp(v, "off")) gPortSettings.fpsCap = 0;
                else if (!_stricmp(v, "uncapped")) gPortSettings.fpsCap = PORT_FPS_UNCAPPED;
                else if (!_stricmp(v, "display")) gPortSettings.fpsCap = PORT_FPS_DISPLAY;
                else if (atoi(v) >= 10 && atoi(v) <= 1000) gPortSettings.fpsCap = atoi(v);
                else fprintf(stderr, "[port] --fps-cap %s: expected off, uncapped, display or a rate (30, 60, 120, 144...)\n", v);
            }
            else if (!strcmp(a, "--filter") && i + 1 < __argc) {
                int f = -1;
                for (int k = 0; k < PORT_FILTER_COUNT; k++) if (!_stricmp(__argv[i + 1], kPortFilterNames[k])) f = k;
                if (f >= 0) gPortSettings.texFilter = f;
                else fprintf(stderr, "[port] --filter %s: expected game, nearest, bilinear, 2x, 4x, 8x or 16x\n", __argv[i + 1]);
                i++;
            }
            else if (!strcmp(a, "--stretch-ui")) gPortSettings.stretchUi = TRUE;
            else if (!strcmp(a, "--aspect") && i + 1 < __argc) {
                const char* v = __argv[++i];
                if (!strcmp(v, "16:9")) gPortSettings.wide = TRUE;
                else if (!strcmp(v, "4:3")) gPortSettings.wide = FALSE;
                else fprintf(stderr, "[port] --aspect %s: expected 4:3 or 16:9\n", v);
            }
            else if (!strcmp(a, "--pointer") && i + 1 < __argc) {
                if (!PortInputAddPointer(__argv[++i])) fprintf(stderr, "[port] --pointer %s: expected X,Y@FRAME\n", __argv[i]);
            }
            else if (!strcmp(a, "--press") && i + 1 < __argc) {
                if (!PortInputAddPress(__argv[++i])) fprintf(stderr, "[port] --press %s: expected BUTTON@FRAME[-FRAME]\n", __argv[i]);
            }
        }

        if (gPortOptions.bench) {                 // --bench: the scripted run of docs/port_runtime.md; 20 s of real time after the fast-forward
            gPortOptions.toMenu = TRUE;
            gPortOptions.noMovies = TRUE;
            if (gPortOptions.bench == 2) {        // village: title, new game conversation, the village map (idle from about frame 4710)
                static const char* const kPresses[] = {"A@3000-3006", "A@3600-3610", "A@4000-4006", "A@4200-4206"};
                for (const char* p : kPresses) PortInputAddPress(p);
                PortInputAddPointer("663,200@3500");
                if (gPortOptions.fastForward < 4800) gPortOptions.fastForward = 4800;
            }
            if (!gPortOptions.perfLog) gPortOptions.perfLog = "build/pc/bench.csv";
        }
        if (gPortOptions.perfLog) PortPerfOpen(gPortOptions.perfLog);
        if (gPortOptions.toMenu) {                // --to-menu: unpaced to the title, A, on to the main menu, then real time
            // Frames as counted by PortFrameDone (the title is up from about frame 2400; the menu from about 3100).
            if (gPortOptions.fastForward < kMenuFrame) gPortOptions.fastForward = kMenuFrame;
            PortInputAddPress("A@2600-2610");
            if (!sawFrames) gPortOptions.frameLimit = 0;
            if (!sawSeconds) gPortOptions.secondsLimit = 0;
        }
        // The Wii's two memory blocks at their original addresses. The code range must be executable.
        void* m1 = VirtualAlloc((void*)(uintptr_t)PORT_MEM1_BASE, PORT_MEM1_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        void* m2 = VirtualAlloc((void*)(uintptr_t)PORT_MEM2_BASE, PORT_MEM2_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        // The hardware registers (0xCC000000: VI, PI, the GX write-gather pipe at 0xCC008000...).
        // Generated code reads and writes them by address; on PC they are plain memory: writes
        // are lost and reads give what was last written.
        void* hw = VirtualAlloc((void*)(uintptr_t)PORT_HW_BASE, PORT_HW_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        DWORD old;
        if (!m1 || !m2 || !hw || !VirtualProtect((void*)(uintptr_t)PORT_TEXT_LO, PORT_TEXT_HI - PORT_TEXT_LO, PAGE_EXECUTE_READWRITE, &old)) {
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
        sMainThreadId = GetCurrentThreadId();
        if (sWatch) {
            HANDLE h = CreateThread(NULL, 0, WatchArm, sMainThread, 0, NULL);
            if (h) { WaitForSingleObject(h, INFINITE); CloseHandle(h); }
        }
        // The mods folder: the mods of monsterlab.ini in their order, then folders it does not list yet.
        gPortOptions.noMods = noMods;
        if (!noMods && !gPortOptions.selfTest) {
            PortModsScan();
            int on = 0;
            for (int i = 0; i < gPortModCount; i++) if (gPortMods[i].enabled && gPortMods[i].present) on++;
            if (on) fprintf(stderr, "[port] mods: %d on, from %s\n", on, PortModsDir());
        } else PortModsRefreshTextures();        // also hooks the texture names (--dump-textures with --no-mods)
        PortLockEnter();                     // the main thread is the first game thread

        // A file is the player's disc image, read as it is (--iso FILE; --data and MONSTERLAB_DATA take one too).
        // Asked for on the command line or in MONSTERLAB_DATA, else the choice kept in monsterlab.ini, else
        // orig/RM7P4F. With none of them a player (started in a window, or with no options at all) gets the
        // first-run screen to choose the disc; that choice goes into monsterlab.ini.
        const char* dir = NULL;
        static char picked[MAX_PATH];
        char why[320] = "";
        BOOL fromIni = FALSE;
        if (gPortOptions.pickerShot) {
            PortPickerRun(NULL, asked, picked, sizeof(picked), gPortOptions.pickerShot);
            ExitProcess(0);
        }
        if (!asked && gPortSettings.gamePath[0] && !gPortOptions.selfTest) {
            if (PortGameCheck(gPortSettings.gamePath, why, sizeof(why)) == PORT_DISC_OK) { asked = gPortSettings.gamePath; fromIni = TRUE; }
            else fprintf(stderr, "[port] the game disc of monsterlab.ini is not used: %s\n       %s\n", gPortSettings.gamePath, why);
        }
        if (!asked && !gPortOptions.selfTest && !FindDataDir(NULL) && (gPortOptions.window || __argc <= 1)) {
            char note[400] = "";
            if (why[0]) snprintf(note, sizeof(note), "The disc chosen before cannot be used. %s", why);
            if (!PortPickerRun(note[0] ? note : NULL, NULL, picked, sizeof(picked), NULL)) ExitProcess(0);
            asked = picked;
            snprintf(gPortSettings.gamePath, sizeof(gPortSettings.gamePath), "%s", picked);
            if (!PortSettingsSave(PortSettingsPath(), &gPortSettings)) fprintf(stderr, "[port] cannot write %s\n", PortSettingsPath());
            if (__argc <= 1) {                    // started with no options: a player, so play in a window without limits
                gPortOptions.window = TRUE;
                gPortOptions.frameLimit = 0;
                gPortOptions.secondsLimit = 0;
            }
        }
        if (asked && FileExists(asked) && !gPortOptions.selfTest) {
            if (PortDiscOpen(asked, why, sizeof(why)) != PORT_DISC_OK) {
                fprintf(stderr, "[port] %s\n       %s\n", asked, why);
                ExitProcess(12);
            }
            if (!LoadDol(NULL)) {
                fprintf(stderr, "[port] the main.dol of %s is not usable\n", asked);
                ExitProcess(11);
            }
            gPortOptions.dataDir = dir = asked;
            gPortOptions.haveData = TRUE;
        } else if ((dir = FindDataDir(asked)) != NULL) {
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
            fprintf(stderr, "[port] no game data found. Pass your own Monster Lab disc image with --iso <file>, or put the\n"
                            "       extracted disc in orig/RM7P4F (sys/main.dol, files/), pass --data <dir> or set MONSTERLAB_DATA. Run with --selftest to test without data.\n");
            ExitProcess(2);
        }
        PortLog("game data: %s", dir);
        if (gPortOptions.verbose) PortModsLogOverrides();
        PortMilestone("main.dol data loaded at the original addresses");
    }
};
static PortBoot sBoot;

// What the original's start-up code does between loading the executable and calling main: start
// the OS, then run the static initialisers. Some of those allocate memory, and the game's
// allocator calls SysInit itself when the heaps do not exist yet, so this must not wait for
// main. It runs after every C++ static initialiser of the PC build (.CRT$XCY).
extern "C" void PortRunStaticInit(void);
void PortDiffTestRun(const char* jobsPath);   // port_difftest.cpp
static void __cdecl PortPreMain(void) {
    PortOsStart();
    if (gPortOptions.diffTest && gPortOptions.haveData && !gPortOptions.selfTest) {   // tools/difftest.py: no interrupts, no window
        PortRunStaticInit();
        PortDiffTestRun(gPortOptions.diffTest);
    }
    PortStartInterrupts();
    if (gPortOptions.haveData && !gPortOptions.selfTest && (gPortOptions.window || gPortOptions.screenshot || gPortOptions.showTexture >= 0))
        PortSamplerStart();                                 // PORT_SAMPLE=1: instruction-pointer sampler (src/port/port_sampler.cpp)
        PortGfxStart(gPortOptions.window);
    PortRunStaticInit();
}
#pragma section(".CRT$XCY", read)
__declspec(allocate(".CRT$XCY")) static void (__cdecl* const sPreMain)(void) = PortPreMain;
#pragma comment(linker, "/include:_PortPreMainAnchor")
extern "C" const void* const PortPreMainAnchor = &sPreMain;

#endif  // _WIN32
