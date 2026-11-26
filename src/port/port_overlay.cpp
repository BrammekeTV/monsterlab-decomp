// PC platform layer: the stats overlay (F3, --stats), drawn top-right over the picture with Dear ImGui
// (third_party/imgui). The renderer measures each shown frame (PortFrameStats) and hands it over at
// the display copy; the overlay goes onto the window's back buffer, so --screenshot pictures stay
// clean unless --screenshot-stats asks for it.
#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#include <tlhelp32.h>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "port/port.h"
#include "port/port_overlay.h"
#include "port/port_stateslots.h"

HWND PortGfxWindow(void);

// MONSTERLAB_SAMPLE=file: a sampling profiler. A thread reads the game thread's instruction pointer every ~0.5 ms
// once the frame count passes MONSTERLAB_SAMPLE_FROM (default 3300) and writes module-relative addresses to the file
// (tools/resolve_samples.py turns them into a function histogram with the linker's map file).
static volatile LONG sSampleOn;
// Samples every thread of the process (the game's OS threads take turns: only one is running, the rest wait inside the kernel).
// An address inside the exe is written module-relative; one outside (Direct3D, the driver, Windows) as "ext <module>".
static DWORD WINAPI SampleThread(void* arg) {
    const char* path = (const char*)arg;
    UINT_PTR base = (UINT_PTR)GetModuleHandleA(NULL);
    UINT_PTR size = ((PIMAGE_NT_HEADERS)(base + ((PIMAGE_DOS_HEADER)base)->e_lfanew))->OptionalHeader.SizeOfImage;
    FILE* f = fopen(path, "w");
    if (!f) return 0;
    while (!sSampleOn) Sleep(5);
    std::vector<HANDLE> threads;
    for (int i = 0; i < 30000; i++) {
        if (i % 500 == 0) {
            for (HANDLE h : threads) CloseHandle(h);
            threads.clear();
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            THREADENTRY32 te = {sizeof(te)};
            if (snap != INVALID_HANDLE_VALUE && Thread32First(snap, &te)) do {
                if (te.th32OwnerProcessID != GetCurrentProcessId() || te.th32ThreadID == GetCurrentThreadId()) continue;
                HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                if (h) threads.push_back(h);
            } while (Thread32Next(snap, &te));
            if (snap != INVALID_HANDLE_VALUE) CloseHandle(snap);
        }
        for (HANDLE h : threads) {
            if (SuspendThread(h) == (DWORD)-1) continue;
            CONTEXT c = {};
            c.ContextFlags = CONTEXT_CONTROL;
            if (GetThreadContext(h, &c)) {
                if (c.Eip >= base && c.Eip < base + size) fprintf(f, "%llx\n", (unsigned long long)(c.Eip - base));
                else {
                    HMODULE m = NULL; char name[64] = "?";
                    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)c.Eip, &m) && m)
                        GetModuleFileNameA(m, name, sizeof(name));
                    const char* slash = strrchr(name, 0x5C);
                    fprintf(f, "ext %s\n", slash ? slash + 1 : name);
                }
            }
            ResumeThread(h);
        }
        Sleep(1);
    }
    fclose(f);
    return 0;
}
static void SampleFrame() {
    static int state, frames, from = 3300;
    if (state == 2) return;
    if (state == 0) {
        const char* path = getenv("MONSTERLAB_SAMPLE");
        if (!path) { state = 2; return; }
        if (getenv("MONSTERLAB_SAMPLE_FROM")) from = atoi(getenv("MONSTERLAB_SAMPLE_FROM"));
        CreateThread(NULL, 0, SampleThread, _strdup(path), 0, NULL);
        state = 1;
    }
    if (++frames >= from) sSampleOn = 1;
}

BOOL gPortOverlayOn;

#define HISTORY 120
static ID3D11DeviceContext* sCtx;
static BOOL sReady;
static PortFrameStats sLast;
static f32  sMs[HISTORY];              // frame times, a ring
static u32  sMsCount, sMsNext;
static f32  sFps;                      // smoothed over about ten frames

int PortOverlayText(const PortFrameStats* s, f32 fps, f32 avgFps, f32 displayFps, char* buf, u32 n) {
    return snprintf(buf, n,
        "game %.1f fps (average %.1f)  %.2f ms per game frame\n"
        "display %.1f pictures/s (repeats included)\n"
        "game %.2f ms  render %.2f ms  wait %.2f ms\n"
        "%u draw calls  %u vertices\n"
        "%u frame-buffer copies\n"
        "%u textures\n"
        "%u of %u voices\n"
        "%ux%u %s  picture %ux%u",
        fps, avgFps, s->frameMs, displayFps, s->gameMs, s->renderMs, s->waitMs, s->draws, s->verts, s->efbCopies, s->textures,
        s->voices, s->voiceCount, s->outW, s->outH, s->wide ? "16:9" : "4:3", s->picW, s->picH);
}

static ID3D11Device* sDev;

// The savestate slots' pictures as textures for ImGui; made again when a slot's file changes.
void* PortOverlayThumb(int slot, unsigned long long stamp) {
    static struct { unsigned long long stamp; ID3D11ShaderResourceView* view; } thumbs[PORT_STATE_SLOTS];
    auto& t = thumbs[slot - 1];
    if (t.stamp == stamp) return t.view;
    if (t.view) { t.view->Release(); t.view = NULL; }
    t.stamp = stamp;
    static u8 rgba[PORT_THUMB_W * PORT_THUMB_H * 4];
    if (!sDev || !PortStateSlotThumb(slot, rgba)) return NULL;
    D3D11_TEXTURE2D_DESC d = {};
    d.Width = PORT_THUMB_W; d.Height = PORT_THUMB_H; d.MipLevels = d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count = 1; d.Usage = D3D11_USAGE_IMMUTABLE; d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA init = {rgba, PORT_THUMB_W * 4, 0};
    ID3D11Texture2D* tex = NULL;
    if (FAILED(sDev->CreateTexture2D(&d, &init, &tex))) return NULL;
    sDev->CreateShaderResourceView(tex, NULL, &t.view);
    tex->Release();
    return t.view;
}

void PortOverlayStart(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    if (sReady) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;             // the port keeps its own settings file
    io.LogFilename = NULL;
    // Segoe UI when Windows has it (the built-in font is tiny at 1080p), sized with the output.
    f32 size = gPortSettings.height * 18.0f / 1080.0f;
    if (size < 13.0f) size = 13.0f;
    char font[MAX_PATH + 32];
    UINT len = GetWindowsDirectoryA(font, MAX_PATH);
    if (len && len < MAX_PATH) {
        strcat(font, "\\Fonts\\segoeui.ttf");
        if (GetFileAttributesA(font) != INVALID_FILE_ATTRIBUTES) io.Fonts->AddFontFromFileTTF(font, size);
    }
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 4.0f;
    ImGui_ImplDX11_Init(dev, ctx);
    sCtx = ctx;
    sDev = dev;
    sReady = TRUE;
}

void PortOverlayFrame(const PortFrameStats* s) {
    sLast = *s;
    SampleFrame();
    // MONSTERLAB_STATSLOG=N: print the average of every N shown frames to stderr (for headless measuring).
    static s32 logN = -1;
    static u32 logCount;
    static f64 sum[7];
    if (logN < 0) { const char* e = getenv("MONSTERLAB_STATSLOG"); logN = e ? atoi(e) : 0; }
    if (logN > 0) {
        f64 v[7] = { s->frameMs, s->gameMs, s->renderMs, s->waitMs, (f64)s->draws, (f64)s->verts, (f64)s->efbCopies };
        for (s32 i = 0; i < 7; i++) sum[i] += v[i];
        if (++logCount == (u32)logN) {
            fprintf(stderr, "[stats] %.1f fps  frame %.2f  game %.2f  render %.2f  wait %.2f ms  draws %.0f  verts %.0f  copies %.0f\n",
                    1000.0 * logN / sum[0], sum[0] / logN, sum[1] / logN, sum[2] / logN, sum[3] / logN, sum[4] / logN, sum[5] / logN, sum[6] / logN);
            logCount = 0;
            for (s32 i = 0; i < 7; i++) sum[i] = 0;
        }
    }
    sMs[sMsNext] = s->frameMs;
    sMsNext = (sMsNext + 1) % HISTORY;
    if (sMsCount < HISTORY) sMsCount++;
    f32 fps = s->frameMs > 0.0f ? 1000.0f / s->frameMs : 0.0f;
    sFps = sFps == 0.0f ? fps : sFps + (fps - sFps) * 0.1f;

    // F3 turns it on and off while the game's window has the keyboard.
    static BOOL wasDown;
    HWND wnd = PortGfxWindow();
    BOOL down = wnd && GetForegroundWindow() == wnd && (GetAsyncKeyState(VK_F3) & 0x8000);
    if (down && !wasDown) gPortOverlayOn = !gPortOverlayOn;
    wasDown = down;
    PortGuiKeys();
}

void PortOverlayDraw(ID3D11RenderTargetView* target, u32 w, u32 h) {
    const char* message = PortStateMessage();
    if ((!gPortOverlayOn && !gPortGuiOpen && !message) || !sReady || !target) return;
    f32 total = 0.0f, worst = 0.0f;
    for (u32 i = 0; i < sMsCount; i++) { total += sMs[i]; if (sMs[i] > worst) worst = sMs[i]; }
    char text[512];
    PortOverlayText(&sLast, sFps, total > 0.0f ? sMsCount * 1000.0f / total : 0.0f, PortGfxDisplayFps(), text, sizeof(text));

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((f32)w, (f32)h);
    io.DeltaTime = sLast.frameMs > 0.0f ? sLast.frameMs / 1000.0f : 1.0f / 50.0f;
    PortGuiInput(w, h);
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();
    PortGuiBuild(w, h);
    if (message) {                       // a savestate was saved, loaded or refused: a line at the bottom
        ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h - h / 30.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::Begin("message", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);
        ImGui::TextUnformatted(message);
        ImGui::End();
    }
    if (gPortOverlayOn) {
    f32 pad = h / 90.0f;
    ImGui::SetNextWindowPos(ImVec2((f32)w - pad, pad), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.65f);
    ImGui::Begin("stats", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(text);
    // The last frames' times, oldest first; the top of the scale is the slowest of them (at least two PAL frames).
    f32 line[HISTORY];
    for (u32 i = 0; i < sMsCount; i++) line[i] = sMs[(sMsNext + HISTORY - sMsCount + i) % HISTORY];
    ImGui::PlotLines("##ms", line, (int)sMsCount, 0, NULL, 0.0f, worst > 40.0f ? worst : 40.0f,
                     ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 2.0f));
    ImGui::End();
    }
    ImGui::Render();
    sCtx->OMSetRenderTargets(1, &target, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    sCtx->OMSetRenderTargets(0, NULL, NULL);
}

// The overlay's text for known numbers, and one ImGui frame laid out without a device.
int PortSelfTestOverlay(void) {
    int fails = 0;
    PortFrameStats s = {};
    s.frameMs = 20.0f; s.gameMs = 12.5f; s.renderMs = 5.0f; s.waitMs = 2.5f;
    s.draws = 312; s.verts = 15234; s.efbCopies = 3; s.textures = 215; s.voices = 5; s.voiceCount = 64;
    s.outW = s.picW = 1920; s.outH = s.picH = 1080; s.wide = TRUE;
    char text[512];
    PortOverlayText(&s, 50.0f, 48.2f, 144.0f, text, sizeof(text));
    static const char* const kWant[] = {"game 50.0 fps (average 48.2)  20.00 ms per game frame", "display 144.0 pictures/s (repeats included)", "game 12.50 ms  render 5.00 ms  wait 2.50 ms",
                                        "312 draw calls  15234 vertices", "3 frame-buffer copies", "215 textures", "5 of 64 voices",
                                        "1920x1080 16:9  picture 1920x1080"};
    for (const char* want : kWant)
        if (!strstr(text, want)) { PortLog("selftest overlay: the text lacks \"%s\"", want); fails++; }

    ImGuiContext* before = ImGui::GetCurrentContext();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.DisplaySize = ImVec2(1920, 1080);
    io.DeltaTime = 0.02f;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;   // no renderer: the font atlas is built and dropped
    for (int frame = 0; frame < 3; frame++) {                   // a window that sizes itself is hidden on its first frame
        ImGui::NewFrame();
        ImGui::Begin("stats", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextUnformatted(text);
        ImGui::End();
        ImGui::Render();
    }
    ImDrawData* dd = ImGui::GetDrawData();
    if (!dd || dd->TotalVtxCount < 4 * 100) { PortLog("selftest overlay: ImGui laid out %d vertices", dd ? dd->TotalVtxCount : -1); fails++; }
    ImGui::DestroyContext(ctx);
    ImGui::SetCurrentContext(before);
    if (!fails) PortLog("selftest overlay: ok (Dear ImGui %s)", IMGUI_VERSION);
    return fails;
}
#endif
