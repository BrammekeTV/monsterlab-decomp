// PC platform layer: the stats overlay (F3, --stats), drawn top-right over the picture with Dear ImGui
// (third_party/imgui). The renderer measures each shown frame (PortFrameStats) and hands it over at
// the display copy; the overlay goes onto the window's back buffer, so --screenshot pictures stay
// clean unless --screenshot-stats asks for it.
#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "port/port.h"
#include "port/port_overlay.h"

HWND PortGfxWindow(void);

BOOL gPortOverlayOn;

#define HISTORY 120
static ID3D11DeviceContext* sCtx;
static BOOL sReady;
static PortFrameStats sLast;
static f32  sMs[HISTORY];              // frame times, a ring
static u32  sMsCount, sMsNext;
static f32  sFps;                      // smoothed over about ten frames

int PortOverlayText(const PortFrameStats* s, f32 fps, f32 avgFps, char* buf, u32 n) {
    return snprintf(buf, n,
        "%.1f fps (average %.1f)  %.2f ms\n"
        "game %.2f ms  render %.2f ms  wait %.2f ms\n"
        "%u draw calls  %u vertices\n"
        "%u frame-buffer copies\n"
        "%u textures\n"
        "%u of %u voices\n"
        "%ux%u %s  picture %ux%u",
        fps, avgFps, s->frameMs, s->gameMs, s->renderMs, s->waitMs, s->draws, s->verts, s->efbCopies, s->textures,
        s->voices, s->voiceCount, s->outW, s->outH, s->wide ? "16:9" : "4:3", s->picW, s->picH);
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
    sReady = TRUE;
}

void PortOverlayFrame(const PortFrameStats* s) {
    sLast = *s;
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
}

void PortOverlayDraw(ID3D11RenderTargetView* target, u32 w, u32 h) {
    if (!gPortOverlayOn || !sReady || !target) return;
    f32 total = 0.0f, worst = 0.0f;
    for (u32 i = 0; i < sMsCount; i++) { total += sMs[i]; if (sMs[i] > worst) worst = sMs[i]; }
    char text[512];
    PortOverlayText(&sLast, sFps, total > 0.0f ? sMsCount * 1000.0f / total : 0.0f, text, sizeof(text));

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((f32)w, (f32)h);
    io.DeltaTime = sLast.frameMs > 0.0f ? sLast.frameMs / 1000.0f : 1.0f / 50.0f;
    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();
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
    PortOverlayText(&s, 50.0f, 48.2f, text, sizeof(text));
    static const char* const kWant[] = {"50.0 fps (average 48.2)  20.00 ms", "game 12.50 ms  render 5.00 ms  wait 2.50 ms",
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
