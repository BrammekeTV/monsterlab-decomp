// PC platform layer: the settings screen (F1 or Esc), built with Dear ImGui inside the overlay's
// frame (port_overlay.cpp). Pages: display, audio, controls. While it is open the keyboard and the
// mouse go to it and not to the game (port_input.cpp checks gPortGuiOpen). Closing it, or Save,
// writes monsterlab.ini (port_settings.cpp).
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "port/port.h"
#include "port/port_input.h"
#include "port/port_overlay.h"

HWND PortGfxWindow(void);

BOOL gPortGuiOpen;
int  gPortGuiPage = -1;

static PortSettings sEdit;             // what Save writes: size and aspect apply at the next start
static BOOL sEditValid;
static int  sCapture = -1, sCaptureSlot;   // the binding and key slot waiting for a key press
static u8   sCaptureHeld[256];         // keys that were down when the capture began: ignored until released
static char sNote[MAX_PATH + 64];

static BOOL Focused(void) {
    HWND wnd = PortGfxWindow();
    return wnd && GetForegroundWindow() == wnd;
}
static inline BOOL Down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

static void Save(void);
// The game disc: what is running, and another choice for the next start (checked like the first-run screen does).
static void PageGame(void) {
    static char verdict[320];
    static BOOL good;
    ImGui::PushTextWrapPos(0.0f);
    ImGui::Text("Playing from: %s", gPortOptions.dataDir ? gPortOptions.dataDir : "-");
    if (sEdit.gamePath[0] && (!gPortOptions.dataDir || _stricmp(sEdit.gamePath, gPortOptions.dataDir))) ImGui::Text("Next start: %s", sEdit.gamePath);
    ImGui::PopTextWrapPos();
    ImGui::Spacing();
    int pick = 0;
    if (ImGui::Button("Change game disc (.iso)...")) pick = 1;
    ImGui::SameLine();
    if (ImGui::Button("Choose extracted folder...")) pick = 2;
    if (pick) {
        char path[MAX_PATH];
        HWND wnd = PortGfxWindow();
        if (pick == 1 ? PortPickFile(wnd, path, sizeof(path)) : PortPickFolder(wnd, path, sizeof(path))) {
            good = PortGameCheck(path, verdict, sizeof(verdict)) == PORT_DISC_OK;
            if (good) {
                snprintf(sEdit.gamePath, sizeof(sEdit.gamePath), "%s", path);
                snprintf(gPortSettings.gamePath, sizeof(gPortSettings.gamePath), "%s", path);
                Save();
            }
        }
    }
    ImGui::PushTextWrapPos(0.0f);
    if (verdict[0]) ImGui::TextColored(good ? ImVec4(0.45f, 0.9f, 0.45f, 1.0f) : ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%s", verdict);
    if (verdict[0] && good) ImGui::TextDisabled("It is used when the game is started again.");
    ImGui::PopTextWrapPos();
}

static void Save(void) {
    if (PortSettingsSave(PortSettingsPath(), &sEdit)) snprintf(sNote, sizeof(sNote), "Saved to %s", PortSettingsPath());
    else snprintf(sNote, sizeof(sNote), "Could not write %s", PortSettingsPath());
}

void PortGuiSetOpen(BOOL open) {
    if (!open && gPortGuiOpen) { sCapture = -1; if (sEditValid) Save(); }
    gPortGuiOpen = open;
}

// F1 and Esc open and close the screen (Esc only ends a key capture while one is waiting).
void PortGuiKeys(void) {
    static BOOL wasDown;
    BOOL down = Focused() && (Down(VK_F1) || Down(VK_ESCAPE));
    if (down && !wasDown && sCapture < 0) PortGuiSetOpen(!gPortGuiOpen);
    wasDown = down;
}

// The mouse and the navigation keys, read on the game's thread and scaled from the window to the picture.
void PortGuiInput(u32 w, u32 h) {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    HWND wnd = PortGfxWindow();
    if (!gPortGuiOpen || !Focused()) { io.AddMousePosEvent(-FLT_MAX, -FLT_MAX); return; }
    POINT pt; RECT rc;
    if (GetCursorPos(&pt) && ScreenToClient(wnd, &pt) && GetClientRect(wnd, &rc) && rc.right > 0 && rc.bottom > 0)
        io.AddMousePosEvent((f32)pt.x * w / rc.right, (f32)pt.y * h / rc.bottom);
    io.AddMouseButtonEvent(0, Down(VK_LBUTTON));
    io.AddMouseButtonEvent(1, Down(VK_RBUTTON));
    f32 wheel = PortGfxTakeWheel();
    if (wheel != 0.0f) io.AddMouseWheelEvent(0.0f, wheel);
    static const struct { int vk; ImGuiKey key; } kKeys[] = {
        {VK_TAB, ImGuiKey_Tab}, {VK_LEFT, ImGuiKey_LeftArrow}, {VK_RIGHT, ImGuiKey_RightArrow}, {VK_UP, ImGuiKey_UpArrow},
        {VK_DOWN, ImGuiKey_DownArrow}, {VK_PRIOR, ImGuiKey_PageUp}, {VK_NEXT, ImGuiKey_PageDown}, {VK_HOME, ImGuiKey_Home},
        {VK_END, ImGuiKey_End}, {VK_RETURN, ImGuiKey_Enter}, {VK_SPACE, ImGuiKey_Space}, {VK_SHIFT, ImGuiMod_Shift},
    };
    if (sCapture < 0)
        for (const auto& k : kKeys) io.AddKeyEvent(k.key, Down(k.vk));
}

static const char* KeyName(int vk, char* buf, int n) {
    if (!vk) return "-";
    switch (vk) {
    case VK_LSHIFT: return "Left Shift";   case VK_RSHIFT: return "Right Shift";
    case VK_LCONTROL: return "Left Ctrl";  case VK_RCONTROL: return "Right Ctrl";
    case VK_LMENU: return "Left Alt";      case VK_RMENU: return "Right Alt";
    }
    LONG code = (LONG)MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC) << 16;
    switch (vk) {                          // keys of the cursor block share scan codes with the number pad
    case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT: case VK_HOME: case VK_END:
    case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK: code |= 1 << 24; break;
    }
    if (code && GetKeyNameTextA(code, buf, n) > 0) return buf;
    snprintf(buf, n, "Key %d", vk);
    return buf;
}

// Press-a-key: the first key that goes down after the capture began becomes the binding; Esc cancels.
static void CaptureStep(void) {
    if (sCapture < 0 || !Focused()) return;
    for (int vk = 8; vk < 255; vk++) {
        if (vk >= VK_SHIFT && vk <= VK_MENU) continue;     // the left/right codes are taken instead
        if (!Down(vk)) { sCaptureHeld[vk] = 0; continue; }
        if (sCaptureHeld[vk]) continue;
        if (vk != VK_ESCAPE) gPortBindings[sCapture].key[sCaptureSlot] = vk;
        sCapture = -1;
        return;
    }
}

static void PageDisplay(void) {
    static const struct { int w, h; } kSizes[] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}, {1024, 768}, {1440, 1080}};
    char now[32];
    snprintf(now, sizeof(now), "%dx%d", sEdit.width, sEdit.height);
    if (ImGui::BeginCombo("Resolution", now)) {
        for (const auto& s : kSizes) {
            char label[32];
            snprintf(label, sizeof(label), "%dx%d", s.w, s.h);
            if (ImGui::Selectable(label, s.w == sEdit.width && s.h == sEdit.height)) { sEdit.width = s.w; sEdit.height = s.h; }
        }
        ImGui::EndCombo();
    }
    int wide = sEdit.wide ? 1 : 0;
    ImGui::RadioButton("16:9", &wide, 1); ImGui::SameLine();
    ImGui::RadioButton("4:3", &wide, 0); ImGui::SameLine();
    ImGui::TextUnformatted("Aspect");
    sEdit.wide = wide;
    if (sEdit.width != gPortSettings.width || sEdit.height != gPortSettings.height || sEdit.wide != gPortSettings.wide)
        ImGui::TextDisabled("Resolution and aspect change when the game is started again.");
    bool b = sEdit.stretchUi != 0;
    if (ImGui::Checkbox("Stretch the 2D layer over the whole width (16:9)", &b)) sEdit.stretchUi = gPortSettings.stretchUi = b;
    b = sEdit.vsync != 0;
    if (ImGui::Checkbox("Vertical sync", &b)) sEdit.vsync = gPortSettings.vsync = b;
    static const struct { int cap; const char* name; const char* what; } kRates[] = {
        {0, "Off (game rate)", "One picture per game frame (50 or 60 a second); Vertical sync decides whether it waits for the monitor."},
        {30, "30", "Shows every second picture: half the game's smoothness, for slow machines."},
        {60, "60", "At most 60 pictures a second."},
        {120, "120", "Shows the last picture again between game frames; the game itself keeps its own speed. Menus and overlays update at 120."},
        {144, "144", "As 120, for a 144 Hz monitor."},
        {PORT_FPS_DISPLAY, "Display's refresh rate", "Shows pictures in step with the monitor, whatever its rate (like Vertical sync, but between game frames too)."},
        {PORT_FPS_UNCAPPED, "Uncapped", "Shows pictures as fast as the graphics card allows, without waiting for the monitor (may tear, uses a lot of power). The game keeps its own speed: it does not run faster."},
    };
    int cur = 0;
    for (int k = 0; k < 7; k++) if (kRates[k].cap == sEdit.fpsCap) cur = k;
    if (ImGui::BeginCombo("Frame rate", kRates[cur].name)) {
        for (int k = 0; k < 7; k++)
            if (ImGui::Selectable(kRates[k].name, k == cur)) sEdit.fpsCap = gPortSettings.fpsCap = kRates[k].cap;
        ImGui::EndCombo();
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("%s", kRates[cur].what);
    ImGui::PopTextWrapPos();
    b = sEdit.fullscreen != 0;
    if (ImGui::Checkbox("Fullscreen (borderless)", &b)) { sEdit.fullscreen = gPortSettings.fullscreen = b; PortGfxSetFullscreen(b); }
    int rs = sEdit.renderScale;
    if (ImGui::SliderInt("Render scale", &rs, 50, 200, "%d%%")) sEdit.renderScale = rs;
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("Size of the internal frame buffer relative to the picture: below 100%% is faster and softer, above is supersampled. Applies when the game is started again.");
    ImGui::PopTextWrapPos();
    static const char* const kFilter[PORT_FILTER_COUNT] = {"Game's own", "Nearest (sharp pixels)", "Bilinear (smooth, mip-mapped)",
                                                           "Anisotropic 2x", "Anisotropic 4x", "Anisotropic 8x", "Anisotropic 16x"};
    if (ImGui::BeginCombo("Texture filtering", kFilter[sEdit.texFilter])) {
        for (int k = 0; k < PORT_FILTER_COUNT; k++)
            if (ImGui::Selectable(kFilter[k], k == sEdit.texFilter)) { sEdit.texFilter = gPortSettings.texFilter = k; PortGfxApplySettings(); }
        ImGui::EndCombo();
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("Anisotropic keeps textures sharp on surfaces seen at a slant; the others change how close-up and distant textures are smoothed.");
    ImGui::PopTextWrapPos();
    b = gPortOverlayOn != 0;
    if (ImGui::Checkbox("Stats overlay (F3)", &b)) gPortOverlayOn = b;
}

static void PageAudio(void) {
    int v = sEdit.volume;
    if (ImGui::SliderInt("Volume", &v, 0, 100, "%d%%")) sEdit.volume = gPortSettings.volume = v;
    bool b = sEdit.mute != 0;
    if (ImGui::Checkbox("Mute", &b)) sEdit.mute = gPortSettings.mute = b;
}

static void PageControls(void) {
    ImGui::TextDisabled(sCapture >= 0 ? "Press a key (Esc cancels)..." : "Click a key to change it. Right-click clears it.");
    if (!ImGui::BeginTable("bindings", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY)) return;
    ImGui::TableSetupColumn("Wii Remote / Nunchuk");
    ImGui::TableSetupColumn("Key 1");
    ImGui::TableSetupColumn("Key 2");
    ImGui::TableSetupColumn("Mouse");
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();
    for (int i = 0; i < gPortBindingCount; i++) {
        PortBinding* bind = &gPortBindings[i];
        ImGui::PushID(i);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(bind->name);
        for (int slot = 0; slot < 2; slot++) {
            ImGui::TableNextColumn();
            ImGui::PushID(slot);
            char name[64], label[80];
            BOOL waiting = sCapture == i && sCaptureSlot == slot;
            snprintf(label, sizeof(label), "%s###key", waiting ? "press a key" : KeyName(bind->key[slot], name, sizeof(name)));
            if (ImGui::Button(label, ImVec2(-FLT_MIN, 0)) && sCapture < 0) {
                sCapture = i; sCaptureSlot = slot;
                for (int vk = 0; vk < 256; vk++) sCaptureHeld[vk] = Down(vk);
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && sCapture < 0) bind->key[slot] = 0;
            ImGui::PopID();
        }
        ImGui::TableNextColumn();
        static const char* const kMouse[3] = {"-", "Left button", "Right button"};
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##mouse", kMouse[bind->mouse >= 0 && bind->mouse <= 2 ? bind->mouse : 0])) {
            for (int m = 0; m < 3; m++)
                if (ImGui::Selectable(kMouse[m], bind->mouse == m)) bind->mouse = m;
            ImGui::EndCombo();
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

// Called between ImGui::NewFrame and ImGui::Render.
void PortGuiBuild(u32 w, u32 h) {
    if (!gPortGuiOpen) return;
    if (!sEditValid) { sEdit = gPortSettings; sEditValid = TRUE; }
    CaptureStep();
    ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(w * 0.5f, h * 0.62f), ImGuiCond_Appearing);
    bool open = true;
    if (ImGui::Begin("Monster Lab settings", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        f32 footer = ImGui::GetFrameHeightWithSpacing() * 2.0f;
        if (ImGui::BeginChild("pages", ImVec2(0, -footer))) {
            if (ImGui::BeginTabBar("tabs")) {
                int page = gPortGuiPage;
                gPortGuiPage = -1;
                if (ImGui::BeginTabItem("Display", NULL, page == 0 ? ImGuiTabItemFlags_SetSelected : 0)) { PageDisplay(); ImGui::EndTabItem(); }
                if (ImGui::BeginTabItem("Audio", NULL, page == 1 ? ImGuiTabItemFlags_SetSelected : 0)) { PageAudio(); ImGui::EndTabItem(); }
                if (ImGui::BeginTabItem("Controls", NULL, page == 2 ? ImGuiTabItemFlags_SetSelected : 0)) { PageControls(); ImGui::EndTabItem(); }
                if (ImGui::BeginTabItem("Game", NULL, page == 3 ? ImGuiTabItemFlags_SetSelected : 0)) { PageGame(); ImGui::EndTabItem(); }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Save")) Save();
        ImGui::SameLine();
        if (ImGui::Button("Close (F1 / Esc)")) open = false;
        ImGui::TextDisabled("%s", sNote[0] ? sNote : PortSettingsPath());
    }
    ImGui::End();
    if (!open) PortGuiSetOpen(FALSE);
}
#endif
