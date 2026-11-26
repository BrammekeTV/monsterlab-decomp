// PC platform layer: the settings screen (F1 or Esc), built with Dear ImGui inside the overlay's
// frame (port_overlay.cpp). Pages: display, audio, controls, game, mods. While it is open the keyboard and the
// mouse go to it and not to the game (port_input.cpp checks gPortGuiOpen). Closing it, or Save,
// writes monsterlab.ini (port_settings.cpp).
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "port/port.h"
#include "port/port_input.h"
#include "port/port_mods.h"
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
    if (ImGui::Button("Change game disc (.iso, .wbfs, .rvz)...")) pick = 1;
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

// Dump textures: on or off, the folder, and how many were written. Saved at once.
static void PageDump(void) {
    bool on = sEdit.dumpTextures || gPortOptions.dumpTextures;
    if (ImGui::Checkbox("Dump textures", &on)) {
        gPortOptions.dumpTextures = NULL;                    // from here on the settings decide, not --dump-textures
        sEdit.dumpTextures = gPortSettings.dumpTextures = on;
        PortGfxApplySettings();                              // what is on screen is decoded again, and so saved
        Save();
    }
    ImGui::SameLine();
    u32 there = PortModsDumpPresent(), failed = PortModsDumpFailed();
    ImGui::Text("%u written", PortModsDumpCount());
    if (there) { ImGui::SameLine(); ImGui::TextDisabled("(%u were there already)", there); }
    if (failed) { ImGui::SameLine(); ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%u could not be written", failed); }
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    ImGui::InputTextWithHint("##dumpdir", "dump\\textures next to the game", sEdit.dumpDir, sizeof(sEdit.dumpDir));
    if (ImGui::IsItemDeactivatedAfterEdit()) { memcpy(gPortSettings.dumpDir, sEdit.dumpDir, sizeof(sEdit.dumpDir)); PortGfxApplySettings(); Save(); }
    ImGui::SameLine();
    if (ImGui::Button("Choose folder...")) {
        char path[MAX_PATH];
        if (PortPickFolder(PortGfxWindow(), path, sizeof(path))) {
            snprintf(sEdit.dumpDir, sizeof(sEdit.dumpDir), "%s", path);
            memcpy(gPortSettings.dumpDir, sEdit.dumpDir, sizeof(sEdit.dumpDir));
            PortGfxApplySettings();
            Save();
        }
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("While on, every texture the game draws with is saved as a PNG in %s. These are the game's own art: do not share them.",
                        gPortOptions.dumpTextures ? gPortOptions.dumpTextures : PortModsDumpFolder());
    ImGui::PopTextWrapPos();
}

// The mods folder's mods in load order: switch one on or off, move it up or down. Saved at once; a
// file the game opens from now on comes from the new list (what is loaded already stays until it is loaded again).
static void PageMods(void) {
    static BOOL scanned;
    if (!scanned) { scanned = TRUE; if (!gPortOptions.noMods) PortModsScan(); }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::Text("Mods folder: %s", PortModsDir());
    if (gPortOptions.noMods) ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Started with --no-mods: no mod is used in this run.");
    ImGui::PopTextWrapPos();
    PageDump();
    ImGui::Separator();
    if (ImGui::Button("Look again")) { PortModsScan(); PortGfxApplySettings(); }
    ImGui::SameLine();
    ImGui::TextDisabled("A mod lower in the list wins over the ones above it.");
    if (!gPortModCount) { ImGui::Spacing(); ImGui::TextDisabled("No mods. Put a mod's folder into the mods folder (docs/modding.md)."); return; }
    BOOL changed = FALSE;
    int move = -1, dir = 0;
    if (ImGui::BeginTable("mods", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Mod", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Replaces", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (int i = 0; i < gPortModCount; i++) {
            PortMod* m = &gPortMods[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool on = m->enabled != 0;
            if (ImGui::Checkbox("##on", &on)) { m->enabled = on; changed = TRUE; }
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            if (!m->present) ImGui::TextDisabled("%s (folder not found)", m->name);
            else {
                // mod.ini's name, version and author, the description under them; the folder in the tooltip.
                ImGui::TextUnformatted(m->title[0] ? m->title : m->name);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Folder: %s", m->name);
                if (m->version[0] || m->author[0]) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s%s%s%s", m->version, m->version[0] && m->author[0] ? " " : "", m->author[0] ? "by " : "", m->author);
                }
                if (m->about[0]) {
                    ImGui::PushTextWrapPos(0.0f);
                    ImGui::TextDisabled("%s", m->about);
                    ImGui::PopTextWrapPos();
                }
            }
            ImGui::TableNextColumn();
            if (m->present) ImGui::Text("%u file%s, %u texture%s", m->fileCount, m->fileCount == 1 ? "" : "s", m->textureCount, m->textureCount == 1 ? "" : "s");
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(i == 0);
            if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { move = i; dir = -1; }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(i == gPortModCount - 1);
            if (ImGui::ArrowButton("##down", ImGuiDir_Down)) { move = i; dir = 1; }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (move >= 0 && PortModsMove(move, dir)) changed = TRUE;
    if (changed) {
        PortModsRefreshTextures();
        PortGfxApplySettings();                // textures are decoded again, with the new list's replacements
        Save();
    }
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
static u32 sPadHeld;                   // pad buttons that were down when a gamepad capture began: ignored until released
static BOOL sEscHeld;

static const char* PadName(u32 bits, char* buf, int n) {
    static const struct { u32 bit; const char* name; } kPad[] = {
        {0x1000, "A"}, {0x2000, "B"}, {0x4000, "X"}, {0x8000, "Y"}, {0x100, "LB"}, {0x200, "RB"}, {0x10000, "LT"}, {0x20000, "RT"},
        {0x10, "Start"}, {0x20, "Back"}, {0x40, "Left stick click"}, {0x80, "Right stick click"},
        {0x1, "D-pad up"}, {0x2, "D-pad down"}, {0x4, "D-pad left"}, {0x8, "D-pad right"}};
    buf[0] = 0;
    for (const auto& p : kPad)
        if (bits & p.bit) snprintf(buf + strlen(buf), n - (int)strlen(buf), "%s%s", buf[0] ? " / " : "", p.name);
    if (!buf[0]) snprintf(buf, n, "-");
    return buf;
}

// Press-a-button: the first pad button that goes down after the capture began becomes the binding; Esc cancels.
static void CapturePadStep(void) {
    BOOL esc = Down(VK_ESCAPE);
    if (esc && !sEscHeld) { sCapture = -1; return; }
    sEscHeld = esc;
    u32 now = PortInputPadBits(), fresh = now & ~sPadHeld;
    sPadHeld &= now;
    if (!fresh) return;
    gPortBindings[sCapture].pad = fresh & (~fresh + 1);        // the lowest of the buttons that went down together
    sCapture = -1;
}

static void CaptureStep(void) {
    if (sCapture < 0 || !Focused()) return;
    if (sCaptureSlot == 2) { CapturePadStep(); return; }
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
    b = sEdit.windowsCursor != 0;
    if (ImGui::Checkbox("Show the Windows mouse cursor over the game", &b)) sEdit.windowsCursor = gPortSettings.windowsCursor = b;
    b = sEdit.vsync != 0;
    if (ImGui::Checkbox("Vertical sync", &b)) sEdit.vsync = gPortSettings.vsync = b;
    static const struct { int cap; const char* name; const char* what; } kRates[] = {
        {0, "Off (game rate)", "One picture per game frame (50 or 60 a second); Vertical sync decides whether it waits for the monitor."},
        {30, "30", "Shows every second picture: half the game's smoothness, for slow machines."},
        {60, "60", "At most 60 pictures a second."},
        {120, "120", "Shows the last picture again between game frames; the game itself keeps its own speed. Menus and overlays update at 120."},
        {144, "144", "As 120, for a 144 Hz monitor."},
        {PORT_FPS_DISPLAY, "Display's refresh rate", "Shows pictures in step with the monitor, whatever its rate (like Vertical sync, but between game frames too)."},
        {PORT_FPS_UNCAPPED, "Uncapped", "Shows pictures as fast as the graphics card allows, without waiting for the monitor (may tear, uses a lot of power). The game itself still runs at 60 and does not run faster: only the last picture is shown again between game frames."},
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
    ImGui::TextDisabled("Size of the internal frame buffer relative to the picture: below 100%% is faster and softer, above is supersampled. Applies when the game is started again.");
    static const int kMsaa[4] = {0, 2, 4, 8};
    int mi = sEdit.msaa >= 8 ? 3 : sEdit.msaa >= 4 ? 2 : sEdit.msaa >= 2 ? 1 : 0;
    char mlabel[16];
    snprintf(mlabel, sizeof(mlabel), kMsaa[mi] ? "%dx" : "Off", kMsaa[mi]);
    if (ImGui::BeginCombo("Anti-aliasing (MSAA)", mlabel)) {
        for (int k = 0; k < 4; k++) {
            snprintf(mlabel, sizeof(mlabel), kMsaa[k] ? "%dx" : "Off", kMsaa[k]);
            if (ImGui::Selectable(mlabel, k == mi)) sEdit.msaa = kMsaa[k];
        }
        ImGui::EndCombo();
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("Smooths the edges of 3D shapes (not cut-out textures such as grass). Costs speed; the graphics card may offer fewer samples than chosen. Applies when the game is started again.");
    ImGui::PopTextWrapPos();
    static const char* const kFilter[PORT_FILTER_COUNT] = {"Game's own", "Nearest (sharp pixels)", "Bilinear (smooth, mip-mapped)",
                                                           "Anisotropic 2x", "Anisotropic 4x", "Anisotropic 8x", "Anisotropic 16x"};
    if (ImGui::BeginCombo("Texture filtering", kFilter[sEdit.texFilter])) {
        for (int k = 0; k < PORT_FILTER_COUNT; k++)
            if (ImGui::Selectable(kFilter[k], k == sEdit.texFilter)) { sEdit.texFilter = gPortSettings.texFilter = k; PortGfxApplySettings(); }
        ImGui::EndCombo();
    }
    ImGui::TextDisabled("Anisotropic keeps textures sharp on surfaces seen at a slant; the others change how close-up and distant textures are smoothed.");
    static const char* const kLighting[2] = {"Original (as on the Wii)", "Improved (no clipped highlights)"};
    int li = sEdit.lighting == PORT_LIGHTING_ORIGINAL ? 0 : 1;
    if (ImGui::BeginCombo("Lighting", kLighting[li])) {
        for (int k = 0; k < 2; k++)
            if (ImGui::Selectable(kLighting[k], k == li)) sEdit.lighting = gPortSettings.lighting = k ? PORT_LIGHTING_IMPROVED : PORT_LIGHTING_ORIGINAL;
        ImGui::EndCombo();
    }
    if (sEdit.lighting == PORT_LIGHTING_IMPROVED) {
        int ex = sEdit.exposure;
        if (ImGui::SliderInt("Exposure", &ex, 50, 150, "%d%%")) sEdit.exposure = gPortSettings.exposure = ex;
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("Improved keeps the colour of bright, lit 3D surfaces (the Wii clips them to white) and rolls highlights off smoothly; exposure brightens or darkens them. The 2D layer is never changed.");
    ImGui::PopTextWrapPos();
    static const char* const kShadows[2] = {"Original (hard, black)", "Soft (blurred, see-through)"};
    int si = sEdit.shadows == PORT_SHADOWS_ORIGINAL ? 0 : 1;
    if (ImGui::BeginCombo("Shadows", kShadows[si])) {
        for (int k = 0; k < 2; k++)
            if (ImGui::Selectable(kShadows[k], k == si)) sEdit.shadows = gPortSettings.shadows = k ? PORT_SHADOWS_SOFT : PORT_SHADOWS_ORIGINAL;
        ImGui::EndCombo();
    }
    if (sEdit.shadows == PORT_SHADOWS_SOFT) {
        int sd = sEdit.shadowDark;
        if (ImGui::SliderInt("Shadow darkness", &sd, 20, 100, "%d%%")) sEdit.shadowDark = gPortSettings.shadowDark = sd;
    }
    b = gPortOverlayOn != 0;
    if (ImGui::Checkbox("Stats overlay (F3)", &b)) gPortOverlayOn = b;
}

static void PageAudio(void) {
    int v = sEdit.volume;
    if (ImGui::SliderInt("Volume", &v, 0, 100, "%d%%")) sEdit.volume = gPortSettings.volume = v;
    bool b = sEdit.mute != 0;
    if (ImGui::Checkbox("Mute", &b)) sEdit.mute = gPortSettings.mute = b;
    b = sEdit.remoteSpeaker != 0;
    if (ImGui::Checkbox("Wii Remote speaker sounds", &b)) sEdit.remoteSpeaker = gPortSettings.remoteSpeaker = b;
    ImGui::TextDisabled("The console plays some sounds from the Wii Remote's speaker; this plays them through your speakers too.");
}

static void PageControls(void) {
    ImGui::TextDisabled(sCapture >= 0 ? (sCaptureSlot == 2 ? "Press a gamepad button (Esc cancels)..." : "Press a key (Esc cancels)...") : "Click a key or gamepad button to change it. Right-click clears it.");
    if (!ImGui::BeginTable("bindings", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY)) return;
    ImGui::TableSetupColumn("Wii Remote / Nunchuk");
    ImGui::TableSetupColumn("Key 1");
    ImGui::TableSetupColumn("Key 2");
    ImGui::TableSetupColumn("Mouse");
    ImGui::TableSetupColumn("Gamepad");
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
        ImGui::TableNextColumn();
        {
            char name[96], label[128];
            BOOL waiting = sCapture == i && sCaptureSlot == 2;
            snprintf(label, sizeof(label), "%s###pad", waiting ? "press a button" : PadName(bind->pad, name, sizeof(name)));
            if (ImGui::Button(label, ImVec2(-FLT_MIN, 0)) && sCapture < 0) {
                sCapture = i; sCaptureSlot = 2;
                sPadHeld = PortInputPadBits(); sEscHeld = Down(VK_ESCAPE);
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && sCapture < 0) bind->pad = 0;
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
                if (ImGui::BeginTabItem("Mods", NULL, page == 4 ? ImGuiTabItemFlags_SetSelected : 0)) { PageMods(); ImGui::EndTabItem(); }
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
