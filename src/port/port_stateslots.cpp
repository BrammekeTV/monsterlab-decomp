// PC port: savestate slots for the player. F5 saves to the current slot, F8 loads it, F6 and F7 pick the slot,
// the settings screen (F1, Savestates page) does the same with buttons. The files are the savestates of
// port_state.cpp, in states\ beside the exe. No original code.
//
// Both happen in PortFrameDone, between two game frames. A load does not rewrite the running game: it starts
// the exe again with --load-state, and this process keeps its window up until the new one has shown a frame of
// the state, then leaves. So the audio voices, a playing movie and everything else the renderer or Windows
// holds start fresh, and a state that cannot be loaded leaves the running game as it was.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "port/port.h"
#include "port/port_overlay.h"
#include "port/port_stateslots.h"

HWND PortGfxWindow(void);

int gPortStateKeys[PORT_STATEKEY_COUNT] = {VK_F5, VK_F8, VK_F6, VK_F7};
const char* const kPortStateKeyNames[PORT_STATEKEY_COUNT] = {"save_key", "load_key", "prev_key", "next_key"};

static const char* const kActionNames[PORT_STATEKEY_COUNT] = {"save", "load", "prev", "next"};
struct Hook { int action; s32 frame; BOOL fired; };
static Hook sHooks[64];
static int  sHookCount;

static int  sSlot = 1;
static volatile LONG sWant;            // from the settings screen: (action << 8 | slot), or 0
static int  sSaveSlot, sLoadSlot;      // a save or load waiting for a good moment
static int  sSaveTries, sLoadWait;
static char sMessage[160];
static DWORD sMessageUntil;
static HANDLE sReady;                  // this process was started by a load: the old one waits for this event

static void Message(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sMessage, sizeof(sMessage), fmt, ap);
    va_end(ap);
    sMessageUntil = GetTickCount() + 3000;
    PortLog("state: %s (frame %d)", sMessage, PortFrameCount());
}
const char* PortStateMessage(void) { return sMessage[0] && (LONG)(sMessageUntil - GetTickCount()) > 0 ? sMessage : NULL; }

int PortStateSlot(void) { return sSlot; }

static const char* Folder(void) {
    static char dir[MAX_PATH + 16];
    if (!dir[0]) {
        const char* e = getenv("MONSTERLAB_STATES");       // tests keep their slots elsewhere
        if (e && *e) snprintf(dir, sizeof(dir), "%s", e);
        else {
            DWORD n = GetModuleFileNameA(NULL, dir, MAX_PATH);
            char* slash = n && n < MAX_PATH ? strrchr(dir, '\\') : NULL;
            if (slash) strcpy(slash + 1, "states");
            else strcpy(dir, "states");
        }
    }
    return dir;
}
static void SlotPath(int slot, const char* ext, char* path, u32 n) { snprintf(path, n, "%s\\slot%d.%s", Folder(), slot, ext); }

BOOL PortStateSlotInfo(int slot, PortStateSlotFile* out) {
    char path[MAX_PATH + 40];
    SlotPath(slot, "mlst", path, sizeof(path));
    memset(out, 0, sizeof(*out));
    WIN32_FILE_ATTRIBUTE_DATA d;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &d)) return FALSE;
    FILETIME local;
    SYSTEMTIME t;
    FileTimeToLocalFileTime(&d.ftLastWriteTime, &local);
    FileTimeToSystemTime(&local, &t);
    snprintf(out->when, sizeof(out->when), "%04u-%02u-%02u %02u:%02u:%02u", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    out->stamp = (unsigned long long)d.ftLastWriteTime.dwHighDateTime << 32 | d.ftLastWriteTime.dwLowDateTime;
    out->problem = PortStateCheck(path);
    return TRUE;
}
BOOL PortStateSlotThumb(int slot, u8* rgba) {
    char path[MAX_PATH + 40];
    SlotPath(slot, "thumb", path, sizeof(path));
    FILE* f = fopen(path, "rb");
    if (!f) return FALSE;
    BOOL ok = fread(rgba, 1, PORT_THUMB_W * PORT_THUMB_H * 4, f) == PORT_THUMB_W * PORT_THUMB_H * 4;
    fclose(f);
    return ok;
}

void PortStateRequest(int action, int slot) { InterlockedExchange(&sWant, (LONG)(action << 8 | slot) + 1); }

BOOL PortStateAddKeyArg(const char* value) {
    const char* at = strchr(value, '@');
    if (!at || sHookCount >= 64) return FALSE;
    for (int a = 0; a < PORT_STATEKEY_COUNT; a++) {
        if ((size_t)(at - value) != strlen(kActionNames[a]) || strncmp(value, kActionNames[a], at - value)) continue;
        sHooks[sHookCount++] = {a, atoi(at + 1), FALSE};
        return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------ saving
static void Save(void) {
    char path[MAX_PATH + 40];
    SlotPath(sSaveSlot, "mlst", path, sizeof(path));
    CreateDirectoryA(Folder(), NULL);
    s32 r = PortStateWrite(path, PortFrameCount());
    if (r == PORT_STATE_RESUMED) return;               // the process that loaded this state comes out of the save here
    if (r == PORT_STATE_LATER && ++sSaveTries < 180) return;
    int slot = sSaveSlot;
    sSaveSlot = 0;
    if (r == PORT_STATE_MOVIE) { Message("Cannot save while a movie plays"); return; }
    if (r != PORT_STATE_SAVED) { Message("State %d could not be saved", slot); return; }
    static u8 rgba[PORT_THUMB_W * PORT_THUMB_H * 4];
    SlotPath(slot, "thumb", path, sizeof(path));
    FILE* f = PortGfxThumbnail(PORT_THUMB_W, PORT_THUMB_H, rgba) ? fopen(path, "wb") : NULL;
    if (f) { fwrite(rgba, 1, sizeof(rgba), f); fclose(f); }
    else DeleteFileA(path);
    Message("State %d saved", slot);
}

// ------------------------------------------------------------------ loading
// One argument as the C runtime of the new process reads it back.
static void Quote(std::string& line, const char* a) {
    if (!line.empty()) line += ' ';
    line += '"';
    size_t slashes = 0;
    for (; *a; a++) {
        if (*a == '\\') { slashes++; continue; }
        line.append(*a == '"' ? slashes * 2 + 1 : slashes, '\\');
        slashes = 0;
        line += *a;
    }
    line.append(slashes * 2, '\\');
    line += '"';
}

// This run's command line, with the state to load instead of the old one and without the --state-key
// entries that already happened.
static std::string CommandLine(const char* state) {
    std::string line;
    int hook = 0;
    for (int i = 0; i < __argc; i++) {
        const char* a = __argv[i];
        if (i && !strcmp(a, "--load-state") && i + 1 < __argc) { i++; continue; }
        if (i && !strcmp(a, "--settings")) {
            if (i + 1 < __argc && __argv[i + 1][0] != '-') i++;
            continue;
        }
        if (i && !strcmp(a, "--state-key") && i + 1 < __argc) {
            const char* v = __argv[++i];
            if (hook < sHookCount && sHooks[hook++].fired) continue;
            Quote(line, a);
            Quote(line, v);
            continue;
        }
        Quote(line, a);
    }
    Quote(line, "--load-state");
    Quote(line, state);
    return line;
}

static void Load(void) {
    char path[MAX_PATH + 40];
    SlotPath(sLoadSlot, "mlst", path, sizeof(path));
    int slot = sLoadSlot;
    sLoadSlot = 0;
    const char* problem = PortStateCheck(path);        // looked at again: the file may have changed
    if (problem) { Message("%s", problem); return; }

    char event[64], num[16], rect[64] = "";
    snprintf(event, sizeof(event), "Local\\MonsterLabState_%lu_%lu", GetCurrentProcessId(), GetTickCount());
    HANDLE ready = CreateEventA(NULL, TRUE, FALSE, event);
    HWND wnd = PortGfxWindow();
    RECT r;
    if (wnd && PortGfxWindowPlace(&r))
        snprintf(rect, sizeof(rect), "%ld,%ld,%d", r.left, r.top, PortGfxIsFullscreen() ? 1 : 0);
    snprintf(num, sizeof(num), "%d", slot);
    SetEnvironmentVariableA("MONSTERLAB_STATE_READY", event);
    SetEnvironmentVariableA("MONSTERLAB_STATE_SLOT", num);
    SetEnvironmentVariableA("MONSTERLAB_STATE_RECT", rect[0] ? rect : NULL);

    char exe[MAX_PATH];
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    std::string line = CommandLine(path);
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    fflush(stdout);
    fflush(stderr);
    BOOL started = ready && CreateProcessA(exe, &line[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    SetEnvironmentVariableA("MONSTERLAB_STATE_READY", NULL);
    SetEnvironmentVariableA("MONSTERLAB_STATE_SLOT", NULL);
    SetEnvironmentVariableA("MONSTERLAB_STATE_RECT", NULL);
    BOOL up = FALSE;
    if (started) {
        if (wnd) AllowSetForegroundWindow(pi.dwProcessId);
        HANDLE both[2] = {ready, pi.hProcess};
        up = WaitForMultipleObjects(2, both, FALSE, 60000) == WAIT_OBJECT_0;
        if (!up) TerminateProcess(pi.hProcess, 1);
    }
    if (up) {
        // The new process is the game from here on. Without a window (tests) this one waits for it and
        // ends as it ends, so whoever started the run sees its result.
        DWORD code = 0;
        if (wnd) ShowWindow(wnd, SW_HIDE);
        else {
            WaitForSingleObject(pi.hProcess, INFINITE);
            GetExitCodeProcess(pi.hProcess, &code);
        }
        TerminateProcess(GetCurrentProcess(), code);
    }
    if (started) { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
    if (ready) CloseHandle(ready);
    Message("State %d could not be loaded", slot);
}

// ------------------------------------------------------------------ every frame
static void Act(int action, int slot) {
    if (action == PORT_STATEKEY_PREV || action == PORT_STATEKEY_NEXT) {
        sSlot = (sSlot - 1 + (action == PORT_STATEKEY_NEXT ? 1 : PORT_STATE_SLOTS - 1)) % PORT_STATE_SLOTS + 1;
        PortStateSlotFile f;
        BOOL there = PortStateSlotInfo(sSlot, &f);
        Message("Slot %d: %s", sSlot, there ? f.when : "empty");
        return;
    }
    sSlot = slot;
    if (action == PORT_STATEKEY_SAVE) { sSaveSlot = slot; sSaveTries = 0; return; }
    char path[MAX_PATH + 40];
    SlotPath(slot, "mlst", path, sizeof(path));
    const char* problem = PortStateCheck(path);
    if (problem) { Message("%s", problem); return; }
    Message("Loading state %d...", slot);
    sLoadSlot = slot;
    sLoadWait = 3;                                     // frames for the message to be shown
}

void PortStateSlotsFrame(s32 frame) {
    static BOOL started;
    if (!started) {
        started = TRUE;
        const char* event = getenv("MONSTERLAB_STATE_READY");
        const char* slot = getenv("MONSTERLAB_STATE_SLOT");
        if (slot && atoi(slot) >= 1 && atoi(slot) <= PORT_STATE_SLOTS) sSlot = atoi(slot);
        if (event) sReady = OpenEventA(EVENT_MODIFY_STATE, FALSE, event);
        if (sReady) {
            PortGfxShowWindow();
            Message("State %d loaded", sSlot);
            SetEvent(sReady);
            CloseHandle(sReady);
        }
    }

    // The keys count while the game's window has the keyboard; one held from before does nothing.
    static BOOL held[PORT_STATEKEY_COUNT] = {TRUE, TRUE, TRUE, TRUE};
    HWND wnd = PortGfxWindow();
    BOOL focus = wnd && GetForegroundWindow() == wnd;
    for (int a = 0; a < PORT_STATEKEY_COUNT; a++) {
        BOOL down = focus && gPortStateKeys[a] && (GetAsyncKeyState(gPortStateKeys[a]) & 0x8000) != 0;
        if (down && !held[a]) Act(a, sSlot);
        held[a] = down;
    }
    for (int i = 0; i < sHookCount; i++) {             // --state-key: the same path, for tests
        if (sHooks[i].fired || sHooks[i].frame != frame) continue;
        sHooks[i].fired = TRUE;
        Act(sHooks[i].action, sSlot);
    }
    LONG want = InterlockedExchange(&sWant, 0);
    if (want) Act((want - 1) >> 8, (want - 1) & 0xFF);

    if (sSaveSlot) Save();
    else if (sLoadSlot && --sLoadWait <= 0) {
        if (PortStateBusy() && sLoadWait > -180) return;   // the game is writing its save file: not now
        if (PortStateBusy()) { sLoadSlot = 0; Message("Cannot load while the game is saving"); return; }
        Load();
    }
}
#endif
