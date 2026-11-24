// F12 capture: a diagnostic snapshot for tracking down picture problems that show only on someone's machine
// (seams, lines, wrong colours). It writes, into captures/ next to the exe, with a timestamped name:
//   capture_<stamp>_movie.png        the decoded frame of the movie on show (when one plays)
//   capture_<stamp>_efb.png          the game's frame buffer at the render resolution
//   capture_<stamp>_picture.png      the output picture (what the display copy produced, at the output size)
//   capture_<stamp>_backbuffer.png   the window's back buffer as presented (with the overlay), windowed runs only
//   capture_<stamp>_settings.txt     every setting, the options of the run and the window's size and DPI
// Captures are never committed (the folder is in .gitignore). --capture-at N takes one at frame N (tests).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"

HWND PortGfxWindow(void);

static void ExeDir(char* out, size_t n) {
    DWORD len = GetModuleFileNameA(NULL, out, (DWORD)n);
    while (len > 0 && out[len - 1] != 92 && out[len - 1] != '/') len--;   // 92: the backslash
    out[len] = 0;
}

static void WriteSettings(const char* path, const char* stamp) {
    FILE* f = fopen(path, "w");
    if (!f) return;
    const PortSettings* s = &gPortSettings;
    fprintf(f, "capture %s, game frame %d\n", stamp, (int)PortFrameCount());
    fprintf(f, "output size: %d x %d, %s%s\n", (int)s->width, (int)s->height, s->wide ? "16:9" : "4:3", s->stretchUi ? ", stretch UI" : "");
    fprintf(f, "render scale: %d %%\nMSAA: %d\ntexture filter: %s (%d)\n", (int)s->renderScale, (int)s->msaa,
            (s->texFilter >= 0 && s->texFilter < PORT_FILTER_COUNT) ? kPortFilterNames[s->texFilter] : "?", (int)s->texFilter);
    fprintf(f, "vsync: %d\nfullscreen: %d\nfps cap: %d (-1 uncapped, -2 display)\n", (int)s->vsync, (int)s->fullscreen, (int)s->fpsCap);
    fprintf(f, "volume: %d, mute: %d, remote speaker: %d\n", (int)s->volume, (int)s->mute, (int)s->remoteSpeaker);
    HWND w = PortGfxWindow();
    if (w) {
        RECT c = {}, o = {};
        GetClientRect(w, &c); GetWindowRect(w, &o);
        fprintf(f, "window client: %d x %d, window: %d x %d, DPI: %u\n", (int)(c.right - c.left), (int)(c.bottom - c.top),
                (int)(o.right - o.left), (int)(o.bottom - o.top), (unsigned)GetDpiForWindow(w));
        HMONITOR m = MonitorFromWindow(w, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {sizeof(mi)};
        if (GetMonitorInfoA(m, &mi)) fprintf(f, "monitor: %d x %d\n", (int)(mi.rcMonitor.right - mi.rcMonitor.left), (int)(mi.rcMonitor.bottom - mi.rcMonitor.top));
    } else fprintf(f, "no window (headless)\n");
    fprintf(f, "options: noMovies %d, fastAll %d, window %d, noMods %d\n", (int)gPortOptions.noMovies, (int)gPortOptions.fastAll, (int)gPortOptions.window, (int)gPortOptions.noMods);
    fclose(f);
}

void PortCaptureFrame(void) {
    static BOOL wasDown, doneAt;
    HWND wnd = PortGfxWindow();
    BOOL down = wnd && GetForegroundWindow() == wnd && (GetAsyncKeyState(VK_F12) & 0x8000);
    BOOL take = (down && !wasDown) || (gPortOptions.captureAt > 0 && !doneAt && PortFrameCount() >= gPortOptions.captureAt);
    wasDown = down;
    if (!take) return;
    doneAt = TRUE;
    char dir[MAX_PATH + 16], prefix[MAX_PATH + 80], path[MAX_PATH + 100], stamp[64];
    ExeDir(dir, sizeof(dir));
    strcat(dir, "captures");
    CreateDirectoryA(dir, NULL);
    SYSTEMTIME t;
    GetLocalTime(&t);
    snprintf(stamp, sizeof(stamp), "%04d%02d%02d_%02d%02d%02d_f%d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, (int)PortFrameCount());
    snprintf(prefix, sizeof(prefix), "%s/capture_%s", dir, stamp);
    snprintf(path, sizeof(path), "%s_settings.txt", prefix);
    WriteSettings(path, stamp);
    snprintf(path, sizeof(path), "%s_movie.png", prefix);
    BOOL movie = PortBinkSaveFrame(path);
    BOOL pic = PortGfxCapture(prefix);
    PortLog("capture: %s_*.png/.txt (%s%s)", prefix, movie ? "movie frame, " : "", pic ? "frame buffer and picture" : "no picture yet");
}
#endif
