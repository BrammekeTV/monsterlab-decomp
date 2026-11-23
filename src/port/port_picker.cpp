// PC platform layer: the first-run screen that asks for the player's own Monster Lab disc. It runs
// before the game starts, when no game data was found: a small window of its own, drawn with Dear
// ImGui, with Windows' file and folder dialogs. The player picks a disc image (.iso) or a folder
// with the extracted disc; the header is checked (port_disc.cpp) and the verdict is shown in words.
// The choice is kept in monsterlab.ini ([game] path). The settings screen's "Game" page uses the
// same dialogs and the same check. Nothing of the disc is copied.
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <d3d11.h>
#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "port/port.h"

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL PortGfxSaveTexture(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, u32 w, u32 h, const char* path);

// Is this the player's Monster Lab: a disc image, or a folder with the extracted disc (sys\main.dol, files\)?
int PortGameCheck(const char* path, char* why, u32 n) {
    DWORD a = GetFileAttributesA(path);
    if (a == INVALID_FILE_ATTRIBUTES) { snprintf(why, n, "It is not there: %s", path); return PORT_DISC_NO_FILE; }
    if (!(a & FILE_ATTRIBUTE_DIRECTORY)) return PortDiscCheck(path, why, n);
    char probe[MAX_PATH + 32];
    snprintf(probe, sizeof(probe), "%s\\sys\\main.dol", path);
    DWORD dol = GetFileAttributesA(probe);
    snprintf(probe, sizeof(probe), "%s\\files", path);
    DWORD files = GetFileAttributesA(probe);
    if (dol == INVALID_FILE_ATTRIBUTES || (dol & FILE_ATTRIBUTE_DIRECTORY) || files == INVALID_FILE_ATTRIBUTES || !(files & FILE_ATTRIBUTE_DIRECTORY)) {
        snprintf(why, n, "This folder is not an extracted Wii disc: it needs sys\\main.dol and a files folder.");
        return PORT_DISC_NOT_WII;
    }
    // Dolphin writes the disc header next to main.dol: the same check as for an image, when it is there.
    snprintf(probe, sizeof(probe), "%s\\sys\\boot.bin", path);
    if (GetFileAttributesA(probe) != INVALID_FILE_ATTRIBUTES) {
        int r = PortDiscCheck(probe, why, n);
        if (r != PORT_DISC_OK) return r;
    }
    snprintf(why, n, "Monster Lab (extracted disc)");
    return PORT_DISC_OK;
}

BOOL PortPickFile(HWND owner, char* out, u32 n) {
    OPENFILENAMEA o = {};
    o.lStructSize = sizeof(o);
    o.hwndOwner = owner;
    o.lpstrFilter = "Wii disc images (*.iso)\0*.iso\0All files\0*.*\0";
    o.lpstrFile = out;
    o.nMaxFile = n;
    o.lpstrTitle = "Choose your Monster Lab disc image";
    o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;   // the game finds its files from the current folder
    out[0] = 0;
    return GetOpenFileNameA(&o) != 0;
}

BOOL PortPickFolder(HWND owner, char* out, u32 n) {
    static BOOL com;
    if (!com) { CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); com = TRUE; }
    BROWSEINFOA b = {};
    b.hwndOwner = owner;
    b.lpszTitle = "Choose the folder with your extracted Monster Lab disc (it holds sys and files)";
    b.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
    PIDLIST_ABSOLUTE id = SHBrowseForFolderA(&b);
    if (!id) return FALSE;
    char path[MAX_PATH];
    BOOL ok = SHGetPathFromIDListA(id, path) && strlen(path) < n;
    CoTaskMemFree(id);
    if (ok) strcpy(out, path);
    return ok;
}

// ------------------------------------------------------------------ the screen
struct Picker {
    char path[MAX_PATH];
    char why[320];
    int  verdict;              // PORT_DISC_*, -1 = nothing picked yet
    BOOL start, quit;
    HWND wnd;
};

static void Choose(Picker* p, BOOL folder) {
    char path[MAX_PATH];
    if (!(folder ? PortPickFolder(p->wnd, path, sizeof(path)) : PortPickFile(p->wnd, path, sizeof(path)))) return;
    snprintf(p->path, sizeof(p->path), "%s", path);
    p->verdict = PortGameCheck(p->path, p->why, sizeof(p->why));
}

static void Build(Picker* p, f32 w, f32 h) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("picker", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushFont(NULL, ImGui::GetStyle().FontSizeBase * 1.6f);
    ImGui::TextUnformatted("Monster Lab");
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted("To play, this port needs your own copy of Monster Lab for Wii (the European release, RM7P4F). "
                           "Choose your disc image (.iso), or the folder of a disc you extracted. "
                           "Nothing is copied or changed; the choice is remembered in monsterlab.ini.");
    ImGui::PopTextWrapPos();
    ImGui::Spacing(); ImGui::Spacing();
    if (ImGui::Button("Choose disc image (.iso)...")) Choose(p, FALSE);
    ImGui::SameLine();
    if (ImGui::Button("Choose extracted folder...")) Choose(p, TRUE);
    ImGui::Spacing(); ImGui::Spacing();
    ImGui::PushTextWrapPos(0.0f);
    if (p->path[0]) ImGui::Text("Chosen: %s", p->path);
    if (p->why[0]) ImGui::TextColored(p->verdict == PORT_DISC_OK ? ImVec4(0.45f, 0.9f, 0.45f, 1.0f) : ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%s", p->why);
    ImGui::PopTextWrapPos();
    ImGui::SetCursorPosY(h - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().WindowPadding.y);
    ImGui::BeginDisabled(p->verdict != PORT_DISC_OK);
    if (ImGui::Button("Start the game")) p->start = TRUE;
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Quit")) p->quit = TRUE;
    ImGui::End();
}

static Picker* sPicker;
static LRESULT CALLBACK PickerProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(w, m, wp, lp)) return 1;
    if (m == WM_CLOSE && sPicker) { sPicker->quit = TRUE; return 0; }
    return DefWindowProcA(w, m, wp, lp);
}

// Shows the screen until the player starts with a good choice (TRUE, the path in out) or quits.
// note: a line to show first (why the saved disc is not used), path: a choice to show checked already.
// With shot, nothing is shown: the screen is drawn once into that PNG (tests) and FALSE comes back.
BOOL PortPickerRun(const char* note, const char* path, char* out, u32 n, const char* shot) {
    enum { W = 860, H = 440 };
    Picker p = {};
    p.verdict = -1;
    if (note) snprintf(p.why, sizeof(p.why), "%s", note);
    if (path && path[0]) {
        snprintf(p.path, sizeof(p.path), "%s", path);
        p.verdict = PortGameCheck(p.path, p.why, sizeof(p.why));
    }
    sPicker = &p;
    if (!shot) {
        WNDCLASSA wc = {};
        wc.lpfnWndProc = PickerProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = "MonsterLabPicker";
        RegisterClassA(&wc);
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT r = {0, 0, W, H};
        AdjustWindowRect(&r, style, FALSE);
        p.wnd = CreateWindowA(wc.lpszClassName, "Monster Lab (PC port)", style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                              r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
        if (!p.wnd) { sPicker = NULL; return FALSE; }
    }
    ID3D11Device* dev = NULL;
    ID3D11DeviceContext* ctx = NULL;
    IDXGISwapChain* swap = NULL;
    ID3D11Texture2D* target = NULL;
    ID3D11RenderTargetView* rtv = NULL;
    static const D3D_DRIVER_TYPE kDrivers[2] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
    HRESULT hr = E_FAIL;
    for (int i = 0; i < 2 && FAILED(hr); i++) {
        if (p.wnd) {
            DXGI_SWAP_CHAIN_DESC sd = {};
            sd.BufferCount = 2; sd.BufferDesc.Width = W; sd.BufferDesc.Height = H; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = p.wnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
            hr = D3D11CreateDeviceAndSwapChain(NULL, kDrivers[i], NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &swap, &dev, NULL, &ctx);
        } else {
            hr = D3D11CreateDevice(NULL, kDrivers[i], NULL, 0, NULL, 0, D3D11_SDK_VERSION, &dev, NULL, &ctx);
        }
    }
    if (SUCCEEDED(hr)) {
        if (swap) {
            swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&target);
        } else {
            D3D11_TEXTURE2D_DESC d = {};
            d.Width = W; d.Height = H; d.MipLevels = 1; d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            d.SampleDesc.Count = 1; d.BindFlags = D3D11_BIND_RENDER_TARGET;
            dev->CreateTexture2D(&d, NULL, &target);
        }
        if (target) dev->CreateRenderTargetView(target, NULL, &rtv);
    }
    BOOL result = FALSE;
    if (rtv) {
        ImGuiContext* before = ImGui::GetCurrentContext();
        ImGuiContext* gui = ImGui::CreateContext();
        ImGui::SetCurrentContext(gui);
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
        char font[MAX_PATH + 32];
        UINT len = GetWindowsDirectoryA(font, MAX_PATH);
        if (len && len < MAX_PATH) {
            strcat(font, "\\Fonts\\segoeui.ttf");
            if (GetFileAttributesA(font) != INVALID_FILE_ATTRIBUTES) io.Fonts->AddFontFromFileTTF(font, 19.0f);
        }
        ImGui::StyleColorsDark();
        ImGui::GetStyle().WindowPadding = ImVec2(24, 20);
        if (p.wnd) ImGui_ImplWin32_Init(p.wnd);
        ImGui_ImplDX11_Init(dev, ctx);
        f32 clear[4] = {0.06f, 0.06f, 0.07f, 1.0f};
        for (int frame = 0; !p.start && !p.quit; frame++) {
            if (p.wnd) {
                MSG msg;
                while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
                ImGui_ImplWin32_NewFrame();
            } else {
                io.DisplaySize = ImVec2((f32)W, (f32)H);
                io.DeltaTime = 1.0f / 60.0f;
            }
            ImGui_ImplDX11_NewFrame();
            ImGui::NewFrame();
            Build(&p, io.DisplaySize.x, io.DisplaySize.y);
            ImGui::Render();
            ctx->OMSetRenderTargets(1, &rtv, NULL);
            ctx->ClearRenderTargetView(rtv, clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            if (swap) swap->Present(1, 0);
            else if (frame == 2) {                           // the layout has settled by the third frame
                ctx->OMSetRenderTargets(0, NULL, NULL);
                if (!PortGfxSaveTexture(dev, ctx, target, W, H, shot)) fprintf(stderr, "[port] cannot write %s\n", shot);
                break;
            }
        }
        result = p.start && p.verdict == PORT_DISC_OK && strlen(p.path) < n;
        if (result) strcpy(out, p.path);
        ImGui_ImplDX11_Shutdown();
        if (p.wnd) ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(gui);
        ImGui::SetCurrentContext(before);
    }
    if (rtv) rtv->Release();
    if (target) target->Release();
    if (swap) swap->Release();
    if (ctx) ctx->Release();
    if (dev) dev->Release();
    if (p.wnd) { DestroyWindow(p.wnd); UnregisterClassA("MonsterLabPicker", GetModuleHandleA(NULL)); }
    sPicker = NULL;
    return result;
}

// Folder verdicts on folders made here (no game data).
int PortSelfTestPicker(void) {
    int fails = 0;
    char base[MAX_PATH + 64], path[MAX_PATH + 96], why[320];
    DWORD n = GetTempPathA(MAX_PATH, base);
    snprintf(base + n, sizeof(base) - n, "monsterlab_selftest_%lu", GetCurrentProcessId());
    CreateDirectoryA(base, NULL);
    if (PortGameCheck(base, why, sizeof(why)) != PORT_DISC_NOT_WII) { PortLog("selftest picker: an empty folder was accepted: %s", why); fails++; }
    snprintf(path, sizeof(path), "%s\\sys", base); CreateDirectoryA(path, NULL);
    snprintf(path, sizeof(path), "%s\\files", base); CreateDirectoryA(path, NULL);
    snprintf(path, sizeof(path), "%s\\sys\\main.dol", base);
    FILE* f = fopen(path, "wb");
    if (f) { fputs("not a real executable", f); fclose(f); }
    if (PortGameCheck(base, why, sizeof(why)) != PORT_DISC_OK) { PortLog("selftest picker: a folder with sys\\main.dol and files was refused: %s", why); fails++; }
    // A header beside main.dol that names another game.
    char boot[MAX_PATH + 96];
    snprintf(boot, sizeof(boot), "%s\\sys\\boot.bin", base);
    f = fopen(boot, "wb");
    if (f) {
        u8 hdr[0x60] = {};
        memcpy(hdr, "RSPE01", 6); hdr[0x18] = 0x5D; hdr[0x19] = 0x1C; hdr[0x1A] = 0x9E; hdr[0x1B] = 0xA3;
        fwrite(hdr, 1, sizeof(hdr), f); fclose(f);
    }
    if (PortGameCheck(base, why, sizeof(why)) != PORT_DISC_WRONG_GAME) { PortLog("selftest picker: a folder of another game was accepted: %s", why); fails++; }
    DeleteFileA(boot); DeleteFileA(path);
    snprintf(path, sizeof(path), "%s\\sys", base); RemoveDirectoryA(path);
    snprintf(path, sizeof(path), "%s\\files", base); RemoveDirectoryA(path);
    RemoveDirectoryA(base);
    if (PortGameCheck(base, why, sizeof(why)) != PORT_DISC_NO_FILE) { PortLog("selftest picker: a missing folder was accepted"); fails++; }
    if (!fails) PortLog("selftest picker: ok (folder check)");
    return fails;
}
#endif
