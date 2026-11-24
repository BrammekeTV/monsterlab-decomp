// PC platform layer: the renderer behind the GX command stream, on Direct3D 11. No original
// code. Design: docs/port_runtime.md, "GX: the renderer".
//
// The interpreter (port_gx.cpp) calls in here with the register files as the stream left them:
//   draw      vertices are decoded, transformed, lit and given their texture coordinates on the
//             CPU from the XF registers (what the console's transform unit does), then drawn with
//             a pixel shader generated from the TEV registers;
//   copy      BP 0x52: the frame buffer goes to the window (display copy) or into a texture;
//   loadTlut  a palette goes into the emulated texture memory.
// The frame buffer ("EFB", 640x528 on the console) is a render target scaled to the output size (sFx, sFy).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>

#include "port/port.h"
#include "port/port_gx.h"
#include "port/port_mods.h"
#include "port/port_overlay.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define EFB_W 640
#define EFB_H 528

static ID3D11Device*           sDev;
static ID3D11DeviceContext*    sCtx;
static IDXGISwapChain*         sSwap;
static ID3D11RenderTargetView* sBackRtv;
static ID3D11Texture2D*        sEfbTex;
static ID3D11Texture2D*        sEfbMs;          // multisampled frame buffer when MSAA is on (drawn into; sEfbTex gets the resolved picture)
static u32                     sSamples = 1;
static ID3D11RenderTargetView* sEfbRtv;
static ID3D11ShaderResourceView* sEfbSrv;
static ID3D11DepthStencilView* sEfbDsv;
static ID3D11Texture2D*        sOutTex;         // the output picture: what the last display copy showed, at the output size
static ID3D11RenderTargetView* sOutRtv;
static ID3D11ShaderResourceView* sOutSrv;
static BOOL                    sShown;          // a display copy has happened
static ID3D11VertexShader*     sVs[9];                       // by the number of texture coordinates a draw has
static ID3D11InputLayout*      sLayouts[9];
static ID3D11VertexShader*     sBlitVs;
static ID3D11PixelShader*      sBlitPs;
static ID3D11Buffer*           sVb;
static ID3D11Buffer*           sIb;
static ID3D11Buffer*           sPsConst;
static ID3D11Buffer*           sBlitConst;
static ID3D11SamplerState*     sLinearClamp;
static ID3D11ShaderResourceView* sWhite;
static HWND                    sWnd;
// Output size and the part of it the picture covers (gPortSettings: --res, --aspect). The frame
// buffer is scaled so that the console's 640 x DISP_H picture is exactly that part, pixel for pixel.
#define DISP_H 480
static u32                     sOutW = 1920, sOutH = 1080;
static u32                     sPicX, sPicY, sPicW = 1920, sPicH = 1080;
static f32                     sFx = 3.0f, sFy = 2.25f;     // frame-buffer pixels per console pixel
static u32                     sEfbPw, sEfbPh;              // frame buffer size in pixels
// Console screen position -> pixel. Whole numbers, so that bands and copies meet without a seam.
static inline s32 PxX(s32 x) { return (s32)lroundf((f32)x * sFx); }
static inline s32 PxY(s32 y) { return (s32)lroundf((f32)y * sFy); }
// The scissor offset (BP 0x59: (x + 342) / 2, (y + 342) / 2 in 10 bits each): the frame-buffer
// position of a pixel is its screen position minus this. Used to draw a frame in bands.
static inline void ScissorOffset(const PortGx* g, s32* offX, s32* offY) {
    *offX = *offY = 0;
    if (g->bp[0x59] != 0) { *offX = (s32)((g->bp[0x59] & 0x3FF) << 1) - 342; *offY = (s32)(((g->bp[0x59] >> 10) & 0x3FF) << 1) - 342; }
}
static BOOL                    sStarted;
static BOOL                    sDrawBound;                  // the pipeline state is still what FlushDraws left (Blit and copies clear it)
struct OutKey { u32 bm, zm, alpha, cull, vp[6], sc0, sc1; f32 squeeze; s32 offX, offY; };

static struct { u32 draws, tris, presents, texCopies, textures, shaders, skipped; } sGfxStats;
// This frame so far (reset at each display copy): time inside this backend, draws, GX vertices, copies.
static s64 sTotalDraw, sTotalCopy, sTotalFlush, sTRes, sTBlit, sTMk, sTDisp, sTPres, sTNew, sTRtv; static u32 sNewTex; static std::vector<ID3D11Texture2D*> sTexPool;     // time inside the backend over the whole run (the boot summary)
static struct { s64 renderTicks; u32 draws, verts, copies; } sFrame;
static inline s64 Ticks_freq(void) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f.QuadPart; }
static inline s64 Ticks(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
static u8 sTlutMem[0x80000];           // the palette half of the console's texture memory

struct Vtx { f32 pos[4]; f32 col[2][4]; f32 tc[8][3]; };
static u32  sTraceFrom;
static BOOL sTrace;                    // PORT_GFX_TRACE: log every draw and copy, save the frame buffer at each copy
static BOOL SavePng(ID3D11Texture2D* tex, u32 w, u32 h, const char* path);
#define VB_VERTS 65536
#define IB_IDXS (1u << 20)

// ------------------------------------------------------------------ window
static HANDLE sWndReady;
static volatile LONG sWheel;           // mouse wheel movement the settings screen has not taken yet
f32 PortGfxTakeWheel(void) { return (f32)InterlockedExchange(&sWheel, 0) / WHEEL_DELTA; }
#define WM_PORT_TITLE (WM_APP + 1)     // the game thread posts it; the title is set here, on the window's own thread
static char sTitleText[96];
static void PostTitle(const char* t) {
    if (!sWnd) return;
    snprintf(sTitleText, sizeof(sTitleText), "%s", t);
    PostMessageA(sWnd, WM_PORT_TITLE, 0, 0);
}
static LRESULT CALLBACK WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_PORT_TITLE) { SetWindowTextA(w, sTitleText); return 0; }
    if (m == WM_CLOSE) { PortStop(0, "window closed"); return 0; }
    if (m == WM_MOUSEWHEEL) { InterlockedExchangeAdd(&sWheel, (LONG)GET_WHEEL_DELTA_WPARAM(wp)); return 0; }
    return DefWindowProcA(w, m, wp, lp);
}
static DWORD WINAPI WindowThread(void*) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "MonsterLabPort";
    RegisterClassA(&wc);
    RECT r = {0, 0, (LONG)sOutW, (LONG)sOutH};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    // PORT_HIDDEN=1: not visible. PORT_OFFSCREEN=1: visible (the real code path) but far outside every monitor. For tests.
    BOOL offscreen = getenv("PORT_OFFSCREEN") != NULL;
    sWnd = CreateWindowA(wc.lpszClassName, "Monster Lab (PC port)", WS_OVERLAPPEDWINDOW | (getenv("PORT_HIDDEN") ? 0 : WS_VISIBLE), offscreen ? -32000 : CW_USEDEFAULT, offscreen ? -32000 : CW_USEDEFAULT,
                         r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
    SetEvent(sWndReady);
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    return 0;
}

HWND PortGfxWindow(void) { return sWnd; }

// Fullscreen is the same window without a border over its whole monitor; the picture is stretched to it.
void PortGfxSetFullscreen(BOOL on) {
    static RECT saved;
    static BOOL full;
    if (!sWnd || on == full) return;
    full = on;
    if (on) {
        GetWindowRect(sWnd, &saved);
        MONITORINFO mi = {sizeof(mi)};
        if (!GetMonitorInfoA(MonitorFromWindow(sWnd, MONITOR_DEFAULTTONEAREST), &mi)) { full = FALSE; return; }
        SetWindowLongA(sWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(sWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    } else {
        SetWindowLongA(sWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(sWnd, NULL, saved.left, saved.top, saved.right - saved.left, saved.bottom - saved.top,
                     SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
}

// Window position (0..1 across and down) -> the console pointer's -1..1, through the picture's
// place in the window and the narrowed 2D layer of widescreen.
static const f32 PORT_POINTER_Y_SCALE = 1.25f;
void PortGfxPointer(f32 wx, f32 wy, f32* px, f32* py) {
    f32 x = (wx * sOutW - sPicX) / sPicW * 2.0f - 1.0f, y = (wy * sOutH - sPicY) / sPicH * 2.0f - 1.0f;
    if (gPortSettings.wide && !gPortSettings.stretchUi) x /= 0.75f;
    // The 2D layer scales the pointer's y by 1.25 (fn_8018CA38 reads it from the pointer block, +0xB4): the
    // remote's y runs -0.8..0.8 over the picture's height, x -1..1 over its width (the cursor sits at
    // 240 + 240 * 1.25 * y of 480 lines). Without this the cursor drifts away from the mouse towards the top and bottom.
    y /= PORT_POINTER_Y_SCALE;
    *px = x; *py = y;
}

// ------------------------------------------------------------------ shaders
static ID3DBlob* Compile(const char* src, const char* entry, const char* target) {
    ID3DBlob* code = NULL; ID3DBlob* err = NULL;
    HRESULT hr = D3DCompile(src, strlen(src), NULL, NULL, NULL, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &code, &err);
    if (FAILED(hr)) {
        PortLog("gfx: shader compile failed: %s", err ? (const char*)err->GetBufferPointer() : "?");
        if (err) err->Release();
        return NULL;
    }
    if (err) err->Release();
    return code;
}

// The vertex shader takes the first NGEN texture coordinates (a draw only uploads the ones it makes); the rest read as (0, 0, 1).
static std::string VsSource(u32 ngen) {
    std::string in = "struct VI { float4 pos : POSITION; float4 c0 : COLOR0; float4 c1 : COLOR1;";
    std::string body = "VO main(VI i) { VO o; o.pos = i.pos; o.c0 = i.c0; o.c1 = i.c1;";
    for (u32 t = 0; t < 8; t++) {
        char b[96];
        if (t < ngen) { snprintf(b, sizeof(b), " float3 t%u : TEXCOORD%u;", t, t); in += b; snprintf(b, sizeof(b), " o.t%u = i.t%u;", t, t); }
        else snprintf(b, sizeof(b), " o.t%u = float3(0, 0, 1);", t);
        body += b;
    }
    return in + " };\n"
        "struct VO { float4 pos : SV_Position; float4 c0 : COLOR0; float4 c1 : COLOR1; float3 t0 : TEXCOORD0; float3 t1 : TEXCOORD1;"
        " float3 t2 : TEXCOORD2; float3 t3 : TEXCOORD3; float3 t4 : TEXCOORD4; float3 t5 : TEXCOORD5; float3 t6 : TEXCOORD6; float3 t7 : TEXCOORD7; };\n"
        + body + " return o; }\n";
}

static const char kBlitSrc[] =
    "cbuffer C : register(b0) { float4 rect; float4 pick; };\n"   // u0, v0, u size, v size; channel weights for an intensity copy
    "Texture2D tex : register(t0); SamplerState smp : register(s0);\n"
    "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
    "VO vs(uint id : SV_VertexID) { VO o; float2 p = float2((id << 1) & 2, id & 2); o.uv = rect.xy + p * rect.zw;"
    " o.pos = float4(p * float2(2, -2) + float2(-1, 1), 0, 1); return o; }\n"
    "float4 ps(VO i) : SV_Target { float4 c = tex.Sample(smp, i.uv); if (pick.x + pick.y + pick.z + pick.w > 0.0) { float v = dot(c, pick); return float4(v, v, v, v); }"
    " return float4(c.rgb, pick.w < 0.0 ? c.a : 1.0); }\n";

// The TEV stages as HLSL. Everything that changes the code is in the key; colours and reference
// values come from the constant buffer.
struct PsConst {
    f32 reg[4][4]; f32 konst[4][4]; f32 alphaRef[4];
    f32 texSize[8][4];      // width, height of each texture map (texels)
    f32 indRow[3][2][4];    // the three indirect matrices, two rows each, already scaled to texels per unit
    f32 indScale[4][4];     // per indirect stage: 1 / 2^scale for s and t of its coordinate
    f32 fogP[4];            // fog: a, b, c of the hardware equation
    f32 fogC[4];            // fog colour
};

// Draws queued by DrawPrims that share all their state, uploaded and drawn together by FlushDraws.
struct PendingState { u32 ngen; PsConst pc; OutKey out; ID3D11PixelShader* ps; ID3D11ShaderResourceView* srv[8]; ID3D11SamplerState* smp[8]; };
static PendingState sPend;
static u32 sPendCount;                                      // GX draws queued
static std::vector<u8> sBV;                                 // the queued vertices: position, two colours and ngen texture coordinates each
static u32 sBVCount;
static std::vector<u16> sBI;
static void FlushDraws();

static const char* const kColorIn[16] = {"prev.rgb", "prev.aaa", "c0.rgb", "c0.aaa", "c1.rgb", "c1.aaa", "c2.rgb", "c2.aaa",
                                         "tex.rgb", "tex.aaa", "ras.rgb", "ras.aaa", "float3(1,1,1)", "float3(0.5,0.5,0.5)", "kc", "float3(0,0,0)"};
static const char* const kAlphaIn[8] = {"prev.a", "c0.a", "c1.a", "c2.a", "tex.a", "ras.a", "ka", "0.0"};
static const char* const kRegName[4] = {"prev", "c0", "c1", "c2"};
static const char* const kBias[3] = {"", " + 0.5", " - 0.5"};
static const char* const kScale[4] = {"1.0", "2.0", "4.0", "0.5"};
static const char* const kCompare[8] = {"false", "(a < r)", "(a == r)", "(a <= r)", "(a > r)", "(a != r)", "(a >= r)", "true"};

static std::string KonstColor(u32 sel) {
    char b[48];
    static const char* const frac[8] = {"1.0", "0.875", "0.75", "0.625", "0.5", "0.375", "0.25", "0.125"};
    if (sel < 8) snprintf(b, sizeof(b), "float3(%s,%s,%s)", frac[sel], frac[sel], frac[sel]);
    else if (sel < 12) snprintf(b, sizeof(b), "float3(0,0,0)");
    else if (sel < 16) snprintf(b, sizeof(b), "konst[%u].rgb", sel - 12);
    else snprintf(b, sizeof(b), "konst[%u].%s", (sel - 16) & 3, sel < 20 ? "rrr" : sel < 24 ? "ggg" : sel < 28 ? "bbb" : "aaa");
    return b;
}
static std::string KonstAlpha(u32 sel) {
    char b[32];
    static const char* const frac[8] = {"1.0", "0.875", "0.75", "0.625", "0.5", "0.375", "0.25", "0.125"};
    if (sel < 8) return frac[sel];
    if (sel < 16) return "0.0";
    snprintf(b, sizeof(b), "konst[%u].%s", (sel - 16) & 3, sel < 20 ? "r" : sel < 24 ? "g" : sel < 28 ? "b" : "a");
    return b;
}

static u32 sUnsupported;               // bit 0: an indirect matrix kind that is not implemented
static const std::string& TevKey(const PortGx* g, u32* texUsed) {
    u32 stages = ((g->bp[0x00] >> 10) & 15) + 1;
    static std::string key;                                 // reused: no allocation per draw
    key.assign((const char*)&stages, 4);
    *texUsed = 0;
    for (u32 n = 0; n < stages; n++) {
        u32 order = g->bp[0x28 + n / 2] >> ((n & 1) ? 12 : 0) & 0xFFF;
        u32 ksel = g->bp[0xF6 + n / 2] >> ((n & 1) ? 14 : 4) & 0x3FF;
        u32 words[5] = {g->bp[0xC0 + n * 2], g->bp[0xC1 + n * 2], order & 0x3FF, ksel, g->bp[0x10 + n] & 0x1FFFFF};
        key.append((const char*)words, 20);
        if (order & 0x40) *texUsed |= 1u << (order & 7);
    }
    u32 tail[12];
    for (u32 i = 0; i < 8; i++) tail[i] = g->bp[0xF6 + i] & 0xF;       // swap tables
    tail[8] = g->bp[0xF3] & 0xFF0000;                                    // alpha compare functions
    tail[9] = (g->bp[0x00] >> 16) & 7;                                   // indirect stages
    tail[10] = g->bp[0x27] & 0xFFFFFF;                                   // their texture maps and coordinates
    tail[11] = (g->bp[0xF1] >> 20) & 15;                                 // fog: projection (bit 3) and kind
    for (u32 i = 0; i < tail[9]; i++) *texUsed |= 1u << ((g->bp[0x27] >> (i * 6)) & 7);
    key.append((const char*)tail, sizeof(tail));
    return key;
}

static std::string TevSource(const PortGx* g) {
    u32 stages = ((g->bp[0x00] >> 10) & 15) + 1;
    std::string s =
        "cbuffer C : register(b0) { float4 reg[4]; float4 konst[4]; float4 alphaRef; float4 texSize[8]; float4 indRow[6]; float4 indScale[4]; float4 fogP; float4 fogC; };\n"
        "Texture2D t0 : register(t0); Texture2D t1 : register(t1); Texture2D t2 : register(t2); Texture2D t3 : register(t3);\n"
        "Texture2D t4 : register(t4); Texture2D t5 : register(t5); Texture2D t6 : register(t6); Texture2D t7 : register(t7);\n"
        "SamplerState s0 : register(s0); SamplerState s1 : register(s1); SamplerState s2 : register(s2); SamplerState s3 : register(s3);\n"
        "SamplerState s4 : register(s4); SamplerState s5 : register(s5); SamplerState s6 : register(s6); SamplerState s7 : register(s7);\n"
        "struct VO { float4 pos : SV_Position; float4 c0 : COLOR0; float4 c1 : COLOR1; float3 tc[8] : TEXCOORD0; };\n"
        "float4 main(VO i) : SV_Target {\n"
        "  float4 prev = reg[0], c0 = reg[1], c1 = reg[2], c2 = reg[3], tex, ras; float3 kc; float ka;\n"
        "  float2 fix = float2(0, 0), coord;\n";
    char b[768];
    // Indirect stages: each samples its texture once; the stages below offset their coordinates
    // with the result (alpha, blue, green as s, t, u; 0 to 255).
    u32 numInd = (g->bp[0x00] >> 16) & 7;
    for (u32 i = 0; i < numInd && i < 4; i++) {
        u32 map = (g->bp[0x27] >> (i * 6)) & 7, coord = (g->bp[0x27] >> (i * 6 + 3)) & 7;
        snprintf(b, sizeof(b), "  float3 ind%u = floor(t%u.Sample(s%u, i.tc[%u].xy / i.tc[%u].z * indScale[%u].xy).abg * 255.0 + 0.5);\n", i, map, map, coord, coord, i);
        s += b;
    }
    static const char sw[5] = "rgba";
    u32 lastC = 0, lastA = 0;
    for (u32 n = 0; n < stages; n++) {
        u32 order = g->bp[0x28 + n / 2] >> ((n & 1) ? 12 : 0) & 0xFFF;
        u32 ksel = g->bp[0xF6 + n / 2] >> ((n & 1) ? 14 : 4) & 0x3FF;
        u32 cc = g->bp[0xC0 + n * 2], ac = g->bp[0xC1 + n * 2];
        u32 map = order & 7, coord = (order >> 3) & 7, chan = (order >> 7) & 7;
        // swap tables: registers 0xF6+2t (r, g) and 0xF7+2t (b, a)
        u32 rs = ac & 3, ts = (ac >> 2) & 3;
        char rsw[5], tsw[5];
        for (u32 k = 0; k < 2; k++) {
            rsw[k] = sw[(g->bp[0xF6 + rs * 2] >> (k * 2)) & 3]; rsw[k + 2] = sw[(g->bp[0xF7 + rs * 2] >> (k * 2)) & 3];
            tsw[k] = sw[(g->bp[0xF6 + ts * 2] >> (k * 2)) & 3]; tsw[k + 2] = sw[(g->bp[0xF7 + ts * 2] >> (k * 2)) & 3];
        }
        rsw[4] = tsw[4] = 0;
        if (order & 0x40) {
            // BP 0x10+n: how this stage's coordinate is changed by an indirect stage
            u32 ic = g->bp[0x10 + n];
            u32 bt = ic & 3, fmt = (ic >> 2) & 3, bias = (ic >> 4) & 7, mid = (ic >> 9) & 15, sw = (ic >> 13) & 7, tw = (ic >> 16) & 7, addPrev = (ic >> 20) & 1;
            static const char* const kWrap[8] = {"", "256.0", "128.0", "64.0", "32.0", "16.0", "0", "0"};
            snprintf(b, sizeof(b), "  coord = i.tc[%u].xy / i.tc[%u].z * texSize[%u].xy;\n", coord, coord, map);
            s += b;
            if (sw == 6) s += "  coord.x = 0.0;\n";
            else if (sw) { snprintf(b, sizeof(b), "  coord.x = coord.x - %s * floor(coord.x / %s);\n", kWrap[sw], kWrap[sw]); s += b; }
            if (tw == 6) s += "  coord.y = 0.0;\n";
            else if (tw) { snprintf(b, sizeof(b), "  coord.y = coord.y - %s * floor(coord.y / %s);\n", kWrap[tw], kWrap[tw]); s += b; }
            if (addPrev) s += "  coord += fix;\n";
            if (bt < numInd && mid >= 1 && mid <= 3) {
                static const char* const kMask[4] = {"", " - 32.0 * floor(v / 32.0)", " - 16.0 * floor(v / 16.0)", " - 8.0 * floor(v / 8.0)"};
                snprintf(b, sizeof(b), "  { float3 v = ind%u; v = v%s; v += float3(%s, %s, %s); coord += float2(dot(indRow[%u].xyz, v), dot(indRow[%u].xyz, v)); }\n",
                         bt, kMask[fmt], (bias & 1) ? (fmt ? "1.0" : "-128.0") : "0.0", (bias & 2) ? (fmt ? "1.0" : "-128.0") : "0.0",
                         (bias & 4) ? (fmt ? "1.0" : "-128.0") : "0.0", (mid - 1) * 2, (mid - 1) * 2 + 1);
                s += b;
            } else if (bt < numInd && mid != 0) {
                sUnsupported |= 1;                          // the two "dynamic" matrix kinds
            }
            s += "  fix = coord;\n";
            snprintf(b, sizeof(b), "  tex = t%u.Sample(s%u, coord / texSize[%u].xy).%s;\n", map, map, map, tsw);
            s += b;
        } else {
            s += "  tex = float4(1,1,1,1);\n";
        }
        snprintf(b, sizeof(b), "  ras = %s.%s;\n", chan == 0 ? "i.c0" : chan == 1 ? "i.c1" : "float4(0,0,0,0)", rsw);
        s += b;
        s += "  kc = " + KonstColor(ksel & 31) + "; ka = " + KonstAlpha((ksel >> 5) & 31) + ";\n";
        // colour
        u32 cd = cc & 15, ccn = (cc >> 4) & 15, cb = (cc >> 8) & 15, ca = (cc >> 12) & 15;
        u32 bias = (cc >> 16) & 3, sub = (cc >> 18) & 1, clamp = (cc >> 19) & 1, scale = (cc >> 20) & 3, dest = (cc >> 22) & 3;
        if (bias != 3) {
            snprintf(b, sizeof(b), "  { float3 r = (%s %s lerp(%s, %s, %s)%s) * %s; %s.rgb = %s; }\n", kColorIn[cd], sub ? "-" : "+",
                     kColorIn[ca], kColorIn[cb], kColorIn[ccn], kBias[bias], kScale[scale], kRegName[dest],
                     clamp ? "saturate(r)" : "clamp(r, -4.0, 4.0)");
        } else {                                            // compare: d + (a op b ? c : 0); the red channel, or per channel
            const char* cmp = sub ? "abs(a.r - b.r) < 0.002" : "a.r > b.r";
            if (scale == 3) cmp = sub ? "abs(a - b) < 0.002" : "a > b";
            snprintf(b, sizeof(b), "  { float3 a = %s, b = %s; %s.rgb = saturate(%s + ((%s) ? %s : float3(0,0,0))); }\n", kColorIn[ca], kColorIn[cb],
                     kRegName[dest], kColorIn[cd], scale == 3 ? "true" : cmp, kColorIn[ccn]);
            if (scale == 3) snprintf(b, sizeof(b), "  { float3 a = %s, b = %s; %s.rgb = saturate(%s + (%s) * %s); }\n", kColorIn[ca], kColorIn[cb],
                                     kRegName[dest], kColorIn[cd], sub ? "(abs(a - b) < 0.002)" : "(a > b)", kColorIn[ccn]);
        }
        s += b;
        lastC = dest;
        // alpha
        u32 ad = (ac >> 4) & 7, acn = (ac >> 7) & 7, ab = (ac >> 10) & 7, aa = (ac >> 13) & 7;
        bias = (ac >> 16) & 3; sub = (ac >> 18) & 1; clamp = (ac >> 19) & 1; scale = (ac >> 20) & 3; dest = (ac >> 22) & 3;
        if (bias != 3) {
            snprintf(b, sizeof(b), "  { float r = (%s %s lerp(%s, %s, %s)%s) * %s; %s.a = %s; }\n", kAlphaIn[ad], sub ? "-" : "+",
                     kAlphaIn[aa], kAlphaIn[ab], kAlphaIn[acn], kBias[bias], kScale[scale], kRegName[dest],
                     clamp ? "saturate(r)" : "clamp(r, -4.0, 4.0)");
        } else {
            snprintf(b, sizeof(b), "  %s.a = saturate(%s + ((%s) ? %s : 0.0));\n", kRegName[dest], kAlphaIn[ad],
                     sub ? (std::string("abs(") + kAlphaIn[aa] + " - " + kAlphaIn[ab] + ") < 0.002").c_str()
                         : (std::string(kAlphaIn[aa]) + " > " + kAlphaIn[ab]).c_str(), kAlphaIn[acn]);
        }
        s += b;
        lastA = dest;
    }
    snprintf(b, sizeof(b), "  float4 o = saturate(float4(%s.rgb, %s.a));\n", kRegName[lastC], kRegName[lastA]);
    s += b;
    u32 at = g->bp[0xF3];
    u32 f0 = (at >> 16) & 7, f1 = (at >> 19) & 7, logic = (at >> 22) & 3;
    if (!(f0 == 7 && f1 == 7) && !(logic == 1 && (f0 == 7 || f1 == 7))) {
        static const char* const kLogic[4] = {"&&", "||", "!=", "=="};
        std::string t0 = kCompare[f0], t1 = kCompare[f1];
        for (size_t p; (p = t0.find('r')) != std::string::npos && t0 != "true";) t0.replace(p, 1, "alphaRef.x");
        for (size_t p; (p = t1.find('r')) != std::string::npos && t1 != "true";) t1.replace(p, 1, "alphaRef.y");
        // quantise like the 8-bit hardware compare
        s += "  { float a = floor(o.a * 255.0 + 0.5) / 255.0; if (!(" + t0 + " " + kLogic[logic] + " " + t1 + ")) discard; }\n";
    }
    {   // fog (BP 0xEE-0xF2): the hardware's fogz = a / (b - z) - c (perspective) or a z - c (orthographic), then the kind's curve
        u32 fk = (g->bp[0xF1] >> 21) & 7, ortho = (g->bp[0xF1] >> 20) & 1;
        if (fk != 0) {
            s += ortho ? "  { float fz = fogP.x * i.pos.z - fogP.z;" : "  { float fz = fogP.x / (fogP.y - i.pos.z) - fogP.z;";
            s += " fz = saturate(fz); float f = fz;";
            if (fk == 4) s += " f = 1.0 - exp2(-8.0 * fz);";
            else if (fk == 5) s += " f = 1.0 - exp2(-8.0 * fz * fz);";
            else if (fk == 6) s += " f = exp2(-8.0 * (1.0 - fz));";
            else if (fk == 7) s += " f = exp2(-8.0 * (1.0 - fz) * (1.0 - fz));";
            s += " o.rgb = lerp(o.rgb, fogC.rgb, f); }\n";
        }
    }
    s += "  return o;\n}\n";
    return s;
}

static std::unordered_map<u64, ID3D11PixelShader*> sPsCache;
static ID3D11PixelShader* GetPixelShader(const PortGx* g, u32* texUsed) {
    const std::string& key = TevKey(g, texUsed);
    static std::string lastKey; static ID3D11PixelShader* lastPs; static u32 lastTex;
    if (lastPs && key == lastKey) { *texUsed = lastTex; return lastPs; }   // the draw before had the same TEV set-up
    u64 h = 1469598103934665603ull;
    const u32* w = (const u32*)key.data();
    for (size_t i = 0; i < key.size() / 4; i++) h = (h ^ w[i]) * 1099511628211ull;
    auto it = sPsCache.find(h);
    if (it != sPsCache.end()) { lastKey = key; lastPs = it->second; lastTex = *texUsed; return lastPs; }
    ID3D11PixelShader* ps = NULL;
    std::string src = TevSource(g);
    ID3DBlob* code = Compile(src.c_str(), "main", "ps_4_0");
    if (code) {
        sDev->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &ps);
        code->Release();
    } else {
        PortLog("gfx: the shader source was:\n%s", src.c_str());
    }
    sPsCache[h] = ps;
    sGfxStats.shaders++;
    return ps;
}

// ------------------------------------------------------------------ textures
static inline u32 Rgba(u32 r, u32 g, u32 b, u32 a) { return r | g << 8 | b << 16 | a << 24; }
static inline u32 E5(u32 v) { return (v << 3) | (v >> 2); }
static inline u32 E6(u32 v) { return (v << 2) | (v >> 4); }
static inline u32 E4(u32 v) { return (v << 4) | v; }
static inline u32 E3(u32 v) { return (v << 5) | (v << 2) | (v >> 1); }
static inline u32 Be16(const u8* p) { return (u32)p[0] << 8 | p[1]; }
static u32 From565(u32 c) { return Rgba(E5(c >> 11), E6((c >> 5) & 63), E5(c & 31), 255); }
static u32 From5A3(u32 c) {
    if (c & 0x8000) return Rgba(E5((c >> 10) & 31), E5((c >> 5) & 31), E5(c & 31), 255);
    return Rgba(E4((c >> 8) & 15), E4((c >> 4) & 15), E4(c & 15), E3((c >> 12) & 7));
}
static u32 Palette(const u8* tlut, u32 fmt, u32 index) {
    u32 c = Be16(tlut + index * 2);
    if (fmt == 0) return Rgba(c & 255, c & 255, c & 255, c >> 8);       // IA8: alpha in the high byte
    return fmt == 1 ? From565(c) : From5A3(c);
}

// Decodes a GX texture (console byte order, tiled) to RGBA8. FALSE for a format not handled.
static BOOL DecodeTexture(const u8* src, u32 w, u32 h, u32 fmt, const u8* tlut, u32 tlutFmt, u32* out) {
    u32 bw = (fmt == 0 || fmt == 8 || fmt == 14) ? 8 : (fmt == 1 || fmt == 2 || fmt == 9) ? 8 : 4;
    u32 bh = (fmt == 0 || fmt == 8 || fmt == 14) ? 8 : 4;
    for (u32 ty = 0; ty < h; ty += bh) {
        for (u32 tx = 0; tx < w; tx += bw) {
            if (fmt == 14) {                                // CMPR: four DXT1 blocks per tile
                for (u32 sb = 0; sb < 4; sb++, src += 8) {
                    u32 c0 = Be16(src), c1 = Be16(src + 2);
                    u32 col[4] = {From565(c0), From565(c1), 0, 0};
                    for (u32 k = 0; k < 3; k++) {
                        u32 a = (col[0] >> (k * 8)) & 255, b = (col[1] >> (k * 8)) & 255;
                        col[2] |= (c0 > c1 ? (2 * a + b) / 3 : (a + b) / 2) << (k * 8);
                        col[3] |= (c0 > c1 ? (a + 2 * b) / 3 : 0) << (k * 8);
                    }
                    col[2] |= 0xFF000000;
                    if (c0 > c1) col[3] |= 0xFF000000;
                    for (u32 y = 0; y < 4; y++) for (u32 x = 0; x < 4; x++) {
                        u32 px = tx + (sb & 1) * 4 + x, py = ty + (sb >> 1) * 4 + y;
                        if (px < w && py < h) out[py * w + px] = col[(src[4 + y] >> (6 - x * 2)) & 3];
                    }
                }
                continue;
            }
            for (u32 y = 0; y < bh; y++) {
                for (u32 x = 0; x < bw; x++) {
                    u32 c = 0, px = tx + x, py = ty + y;
                    switch (fmt) {
                    case 0: { u32 v = (src[(y * 8 + x) / 2] >> ((x & 1) ? 0 : 4)) & 15; v = E4(v); c = Rgba(v, v, v, v); break; }
                    case 1: { u32 v = src[y * 8 + x]; c = Rgba(v, v, v, v); break; }
                    case 2: { u32 v = src[y * 8 + x]; c = Rgba(E4(v & 15), E4(v & 15), E4(v & 15), E4(v >> 4)); break; }
                    case 3: { const u8* p = src + (y * 4 + x) * 2; c = Rgba(p[1], p[1], p[1], p[0]); break; }
                    case 4: c = From565(Be16(src + (y * 4 + x) * 2)); break;
                    case 5: c = From5A3(Be16(src + (y * 4 + x) * 2)); break;
                    case 6: { const u8* p = src + (y * 4 + x) * 2; c = Rgba(p[1], p[32], p[33], p[0]); break; }
                    case 8: c = Palette(tlut, tlutFmt, (src[(y * 8 + x) / 2] >> ((x & 1) ? 0 : 4)) & 15); break;
                    case 9: c = Palette(tlut, tlutFmt, src[y * 8 + x]); break;
                    case 10: c = Palette(tlut, tlutFmt, Be16(src + (y * 4 + x) * 2) & 0x3FFF); break;
                    default: return FALSE;
                    }
                    if (px < w && py < h) out[py * w + px] = c;
                }
            }
            src += fmt == 6 ? 64 : (fmt == 0 || fmt == 8) ? 32 : (fmt == 1 || fmt == 2 || fmt == 9) ? 32 : 32;
        }
    }
    return TRUE;
}

static u32 TextureBytes(u32 w, u32 h, u32 fmt) {
    u32 bw = (fmt == 3 || fmt == 4 || fmt == 5 || fmt == 6 || fmt == 10) ? 4 : 8;
    u32 bh = (fmt == 0 || fmt == 8 || fmt == 14) ? 8 : 4;
    u32 tiles = ((w + bw - 1) / bw) * ((h + bh - 1) / bh);
    return tiles * (fmt == 6 ? 64 : 32);
}

static u32 Fnv(const u8* p, u32 n, u32 h) {
    u32 step = n > 4096 ? n / 2048 : 1;
    for (u32 i = 0; i < n; i += step) h = (h ^ p[i]) * 16777619u;
    return h;
}

struct TexEntry { ID3D11ShaderResourceView* srv; u32 hash, w, h, fmt, gen; };
static u32 sTexGen = 1;                                    // bumped each frame and on a palette load: textures are re-hashed once per generation
static std::map<u64, TexEntry> sTextures;                   // by address and palette slot
static std::map<u32, ID3D11ShaderResourceView*> sEfbCopies; // frame-buffer copies, by destination address
static std::map<u32, u32> sEfbCopyRowBytes;                 // console bytes per row of each copy, by destination address

// Texture filtering (Settings > Display): the game's own sampler modes, or everything nearest, bilinear
// (trilinear over a mip chain made here) or anisotropic.
static BOOL sFilterDirty;
static BOOL WantMips(void) { return gPortSettings.texFilter >= PORT_FILTER_BILINEAR; }
void PortGfxApplySettings(void) { sFilterDirty = TRUE; }

static ID3D11ShaderResourceView* MakeTexture(const u32* rgba, u32 w, u32 h) {
    D3D11_TEXTURE2D_DESC d = {};
    std::vector<std::vector<u32>> chain;            // the smaller levels (box filter)
    D3D11_SUBRESOURCE_DATA init[16] = {{rgba, w * 4, 0}};
    u32 levels = 1;
    if (WantMips() && w > 1 && h > 1) {
        const u32* src = rgba; u32 sw = w, sh = h;
        while ((sw > 1 || sh > 1) && levels < 16) {
            u32 nw = sw > 1 ? sw / 2 : 1, nh = sh > 1 ? sh / 2 : 1;
            std::vector<u32> lv(nw * nh);
            for (u32 y = 0; y < nh; y++)
                for (u32 x = 0; x < nw; x++) {
                    u32 sum[4] = {0, 0, 0, 0};
                    for (u32 k = 0; k < 4; k++) {
                        u32 px = src[std::min(y * 2 + (k >> 1), sh - 1) * sw + std::min(x * 2 + (k & 1), sw - 1)];
                        for (u32 c = 0; c < 4; c++) sum[c] += (px >> (c * 8)) & 255;
                    }
                    lv[y * nw + x] = (sum[0] + 2) / 4 | ((sum[1] + 2) / 4) << 8 | ((sum[2] + 2) / 4) << 16 | ((sum[3] + 2) / 4) << 24;
                }
            chain.push_back(std::move(lv));
            src = chain.back().data(); sw = nw; sh = nh;
            init[levels++] = {src, nw * 4, 0};
        }
    }
    d.Width = w; d.Height = h; d.MipLevels = levels; d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count = 1; d.Usage = D3D11_USAGE_IMMUTABLE; d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* t = NULL; ID3D11ShaderResourceView* srv = NULL;
    if (FAILED(sDev->CreateTexture2D(&d, init, &t))) return NULL;
    sDev->CreateShaderResourceView(t, NULL, &srv);
    t->Release();
    return srv;
}

static ID3D11ShaderResourceView* GetTexture(const PortGx* g, u32 map) {
    u32 base = (map < 4 ? 0x80 : 0xA0) + (map & 3);
    u32 img0 = g->bp[base + 8], addr = (g->bp[base + 0x14] & 0xFFFFFF) << 5, tl = g->bp[base + 0x18];
    u32 w = (img0 & 0x3FF) + 1, h = ((img0 >> 10) & 0x3FF) + 1, fmt = (img0 >> 20) & 15;
    auto copy = sEfbCopies.find(addr);
    if (copy != sEfbCopies.end()) return copy->second;
    const u8* data = PortGxHostPtr(addr);
    if (!data) return sWhite;
    const u8* tlut = sTlutMem + ((tl & 0x3FF) << 9);
    BOOL pal = fmt == 8 || fmt == 9 || fmt == 10;
    u64 key = (u64)addr << 16 | (pal ? (tl & 0xFFF) : 0xFFFF);
    auto it = sTextures.find(key);
    if (it != sTextures.end() && it->second.gen == sTexGen && it->second.w == w && it->second.h == h && it->second.fmt == fmt && it->second.srv) return it->second.srv;
    u32 bytes = TextureBytes(w, h, fmt);
    u32 hash = Fnv(data, bytes, 2166136261u ^ w ^ h << 12 ^ fmt << 24);
    if (pal) hash = Fnv(tlut, fmt == 8 ? 32 : fmt == 9 ? 512 : 0x8000, hash ^ tl);
    if (it != sTextures.end() && it->second.hash == hash) { it->second.gen = sTexGen; it->second.w = w; it->second.h = h; it->second.fmt = fmt; return it->second.srv; }
    std::vector<u32> rgba(w * h);
    if (!DecodeTexture(data, w, h, fmt, tlut, (tl >> 10) & 3, rgba.data())) {
        static u32 warned;
        if (!(warned & (1u << fmt))) { warned |= 1u << fmt; PortLog("gfx: texture format %u is not decoded (drawn white)", fmt); }
        return sWhite;
    }
    if (it != sTextures.end() && it->second.srv) { FlushDraws(); it->second.srv->Release(); }
    // Mods: a PNG with this texture's content hash replaces it, at the PNG's own size (--dump-textures writes the originals).
    ID3D11ShaderResourceView* made = NULL;
    if (PortModsTexturesWanted()) {
        u64 content = PortModsTextureHash(data, bytes, tlut, pal ? (fmt == 8 ? 32 : fmt == 9 ? 512 : 0x8000) : 0, w, h, fmt);
        const char* tname = PortModsTextureName(data);
        PortModsDumpTexture(content, tname, rgba.data(), w, h, fmt);
        u32 rw = 0, rh = 0;
        u32* png = PortModsTexture(content, tname, &rw, &rh);
        if (png) { made = MakeTexture(png, rw, rh); free(png); }
    }
    if (!made) made = MakeTexture(rgba.data(), w, h);
    TexEntry e = {made, hash, w, h, fmt, sTexGen};
    sTextures[key] = e;
    sGfxStats.textures++;
    return e.srv ? e.srv : sWhite;
}

static ID3D11SamplerState* sSamplers[256];
static ID3D11SamplerState* GetSampler(const PortGx* g, u32 map) {
    u32 mode = g->bp[(map < 4 ? 0x80 : 0xA0) + (map & 3)] & 0xFF;
    if (sSamplers[mode]) return sSamplers[mode];
    static const D3D11_TEXTURE_ADDRESS_MODE kWrap[4] = {D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_MIRROR,
                                                        D3D11_TEXTURE_ADDRESS_WRAP};
    D3D11_SAMPLER_DESC d = {};
    BOOL mag = (mode >> 4) & 1, min = ((mode >> 5) & 7) & 4 ? TRUE : ((mode >> 5) & 7) == 1;
    d.Filter = mag && min ? D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT : mag ? D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
             : min ? D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;
    u32 tf = (u32)gPortSettings.texFilter;
    if (tf == PORT_FILTER_NEAREST) d.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    else if (tf == PORT_FILTER_BILINEAR) d.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    else if (tf >= PORT_FILTER_AF2) { d.Filter = D3D11_FILTER_ANISOTROPIC; d.MaxAnisotropy = 2u << (tf - PORT_FILTER_AF2); }
    d.MaxAnisotropy = d.MaxAnisotropy ? d.MaxAnisotropy : 1;
    d.AddressU = kWrap[mode & 3]; d.AddressV = kWrap[(mode >> 2) & 3]; d.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    d.MaxLOD = D3D11_FLOAT32_MAX; d.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ID3D11SamplerState* s = NULL;
    sDev->CreateSamplerState(&d, &s);
    sSamplers[mode] = s;
    return s;
}

static void LoadTlut(const PortGx* g, u32 source, u32 dest) {
    (void)g;
    const u8* src = PortGxHostPtr((source & 0xFFFFFF) << 5);
    u32 off = (dest & 0x3FF) << 9, bytes = ((dest >> 10) & 0x7FF) * 32;
    if (!src || off + bytes > sizeof(sTlutMem)) return;
    memcpy(sTlutMem + off, src, bytes);
    sTexGen++;
}

// ------------------------------------------------------------------ fixed pipeline state
static std::map<u32, ID3D11BlendState*> sBlends;
static std::map<u32, ID3D11DepthStencilState*> sDepths;
static ID3D11RasterizerState* sRaster[8];

// The fixed-function state of a draw, made from the GX registers when the draw is queued.
static OutKey MakeOutKey(const PortGx* g, f32 squeeze) {
    OutKey k = {};
    k.bm = g->bp[0x41] & 0xFFFF; k.zm = g->bp[0x40] & 0x1F; k.alpha = g->bp[0x43] & 7;
    k.cull = ((g->bp[0x00] >> 14) & 3) | (g->xf[0x05] & 1) << 2;
    memcpy(k.vp, &g->xf[0x1A], sizeof(k.vp));
    k.sc0 = g->bp[0x20]; k.sc1 = g->bp[0x21]; k.squeeze = squeeze;
    ScissorOffset(g, &k.offX, &k.offY);
    return k;
}

static void SetOutputState(const OutKey& k) {
    f32 squeeze = k.squeeze;
    u32 bm = k.bm, zm = k.zm, hasAlpha = k.alpha == 1;
    u32 bkey = bm | hasAlpha << 16;
    ID3D11BlendState*& bs = sBlends[bkey];
    if (!bs) {
        static const D3D11_BLEND kSrc[8] = {D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_INV_DEST_COLOR,
                                            D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA};
        static const D3D11_BLEND kDst[8] = {D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_INV_SRC_COLOR,
                                            D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA};
        static const D3D11_BLEND kAlpha[8] = {D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA,
                                              D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA};
        D3D11_BLEND_DESC d = {};
        D3D11_RENDER_TARGET_BLEND_DESC& rt = d.RenderTarget[0];
        u32 sf = (bm >> 8) & 7, df = (bm >> 5) & 7;
        if (!hasAlpha) {                                    // no alpha in the frame buffer: it reads as 1
            if (sf == 6) sf = 1; else if (sf == 7) sf = 0;
            if (df == 6) df = 1; else if (df == 7) df = 0;
        }
        rt.BlendEnable = (bm & 1) || (bm & 0x800);
        rt.SrcBlend = kSrc[sf]; rt.DestBlend = kDst[df]; rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = sf == 2 ? D3D11_BLEND_DEST_ALPHA : sf == 3 ? D3D11_BLEND_INV_DEST_ALPHA : kAlpha[sf];
        rt.DestBlendAlpha = kAlpha[df]; rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        if (bm & 0x800) {                                   // subtract: destination - source
            rt.SrcBlend = rt.DestBlend = rt.SrcBlendAlpha = rt.DestBlendAlpha = D3D11_BLEND_ONE;
            rt.BlendOp = rt.BlendOpAlpha = D3D11_BLEND_OP_REV_SUBTRACT;
        }
        rt.RenderTargetWriteMask = (UINT8)(((bm & 8) ? 7 : 0) | ((bm & 0x10) ? 8 : 0));
        sDev->CreateBlendState(&d, &bs);
    }
    sCtx->OMSetBlendState(bs, NULL, 0xFFFFFFFF);
    ID3D11DepthStencilState*& ds = sDepths[zm];
    if (!ds) {
        D3D11_DEPTH_STENCIL_DESC d = {};
        d.DepthEnable = zm & 1;
        d.DepthFunc = (D3D11_COMPARISON_FUNC)(((zm >> 1) & 7) + 1);
        d.DepthWriteMask = (zm & 1) && (zm & 0x10) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        sDev->CreateDepthStencilState(&d, &ds);
    }
    sCtx->OMSetDepthStencilState(ds, 0);
    // XF 0x1005 bit 0: clipping off. Depth is then clamped instead of clipped, as on the console.
    u32 cull = k.cull;
    if (!sRaster[cull]) {
        D3D11_RASTERIZER_DESC d = {};
        d.FillMode = D3D11_FILL_SOLID;
        d.CullMode = (cull & 3) == 1 ? D3D11_CULL_BACK : (cull & 3) == 2 ? D3D11_CULL_FRONT : D3D11_CULL_NONE;
        d.FrontCounterClockwise = FALSE;                    // GX front faces are clockwise
        d.DepthClipEnable = !(cull & 4); d.ScissorEnable = TRUE;
        sDev->CreateRasterizerState(&d, &sRaster[cull]);
    }
    sCtx->RSSetState(sRaster[cull]);

    // Viewport (XF 0x101A: half width, -half height, z range, centre x + 342, centre y + 342, far z)
    f32 vp[6];
    memcpy(vp, k.vp, sizeof(vp));
    D3D11_VIEWPORT v;
    f32 w = 2.0f * vp[0], h = -2.0f * vp[1];
    // The offset is taken off in whole pixels (the copy of a band does the same).
    s32 offX = k.offX, offY = k.offY;
    v.TopLeftX = (vp[3] - 342.0f - vp[0]) * sFx - (f32)PxX(offX); v.TopLeftY = (vp[4] - 342.0f + vp[1]) * sFy - (f32)PxY(offY);
    v.Width = w * sFx; v.Height = h * sFy; v.MinDepth = 0; v.MaxDepth = 1;
    if (v.Width <= 0 || v.Height <= 0) { v.TopLeftX = v.TopLeftY = 0; v.Width = (f32)sEfbPw; v.Height = (f32)sEfbPh; }
    sCtx->RSSetViewports(1, &v);
    // Scissor (BP 0x20 top-left, 0x21 bottom-right, both + 342, y in the low 12 bits)
    s32 x0 = (s32)((k.sc0 >> 12) & 0xFFF) - 342, y0 = (s32)(k.sc0 & 0xFFF) - 342;
    s32 x1 = (s32)((k.sc1 >> 12) & 0xFFF) - 342 + 1, y1 = (s32)(k.sc1 & 0xFFF) - 342 + 1;
    D3D11_RECT sc = {0, 0, (LONG)sEfbPw, (LONG)sEfbPh};
    if (x1 > x0 && y1 > y0) {
        // A clip window of a narrowed 2D draw is narrowed with it (not the whole-screen one).
        if (squeeze != 1.0f && (x0 > offX || x1 < offX + EFB_W)) {
            f32 mid = (f32)offX + EFB_W / 2.0f;
            sc.left = std::max((s32)lroundf((mid + ((f32)x0 - mid) * squeeze) * sFx) - PxX(offX), 0);
            sc.right = std::min((s32)lroundf((mid + ((f32)x1 - mid) * squeeze) * sFx) - PxX(offX), (s32)sEfbPw);
        } else {
            sc.left = std::max(PxX(x0) - PxX(offX), 0);
            sc.right = std::min(PxX(x1) - PxX(offX), (s32)sEfbPw);
        }
        sc.top = std::max(PxY(y0) - PxY(offY), 0);
        sc.bottom = std::min(PxY(y1) - PxY(offY), (s32)sEfbPh);
    }
    sCtx->RSSetScissorRects(1, &sc);
}

// ------------------------------------------------------------------ vertices: decode, transform, light
static inline f64 Pow2(s32 n) { u64 b = (u64)(n + 1023) << 52; f64 d; memcpy(&d, &b, 8); return d; }   // ldexp(1.0, n) for -1022 <= n <= 1023
static inline f32 BeF32(const u8* p) { u32 u = (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; f32 f; memcpy(&f, &u, 4); return f; }

// One component of a position, normal or texture coordinate. `be`: from the command stream
// (console byte order); otherwise from an array in memory (host order).
static f32 Component(const u8* p, u32 fmt, u32 shift, BOOL be) {
    static const f32 kScale[32] = {1.0f, 0.5f, 0.25f, 0.125f, 1.0f / 16, 1.0f / 32, 1.0f / 64, 1.0f / 128, 1.0f / 256, 1.0f / 512, 1.0f / 1024, 1.0f / 2048, 1.0f / 4096,
                                   1.0f / 8192, 1.0f / 16384, 1.0f / 32768, 1.0f / 65536};
    f32 scale = shift < 17 ? kScale[shift] : 1.0f / (f32)(1u << shift);
    switch (fmt) {
    case 0: return (f32)p[0] * scale;
    case 1: return (f32)(s8)p[0] * scale;
    case 2: return (f32)(be ? Be16(p) : *(const u16*)p) * scale;
    case 3: return (f32)(s16)(be ? Be16(p) : *(const u16*)p) * scale;
    default: if (be) return BeF32(p); { f32 f; memcpy(&f, p, 4); return f; }
    }
}

static void Colour(const u8* p, u32 fmt, BOOL be, f32* out) {
    u32 r, g, b, a = 255;
    if (fmt == 0 || fmt == 3) {
        u32 c = be ? Be16(p) : *(const u16*)p;
        if (fmt == 0) { r = E5(c >> 11); g = E6((c >> 5) & 63); b = E5(c & 31); }
        else { r = E4(c >> 12); g = E4((c >> 8) & 15); b = E4((c >> 4) & 15); a = E4(c & 15); }
    } else if (fmt == 1 || fmt == 4) {                      // three bytes
        if (fmt == 1) { r = p[0]; g = p[1]; b = p[2]; }
        else { u32 c = (u32)p[0] << 16 | (u32)p[1] << 8 | p[2]; r = E6(c >> 18); g = E6((c >> 12) & 63); b = E6((c >> 6) & 63); a = E6(c & 63); }
    } else {                                                // 32 bits: 0xRRGGBBAA as a word
        u32 c = be ? ((u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]) : *(const u32*)p;
        r = c >> 24; g = (c >> 16) & 255; b = (c >> 8) & 255; a = fmt == 2 ? 255 : (c & 255);
    }
    const f32 k = 1.0f / 255.0f;
    out[0] = r * k; out[1] = g * k; out[2] = b * k; out[3] = a * k;
}

static inline void Mul34(const f32* m, const f32* v, f32* out) {
    for (int r = 0; r < 3; r++) out[r] = m[r * 4] * v[0] + m[r * 4 + 1] * v[1] + m[r * 4 + 2] * v[2] + m[r * 4 + 3] * v[3];
}
static inline void RegColour(u32 c, f32* out) {
    const f32 k = 1.0f / 255.0f;
    out[0] = (c >> 24) * k; out[1] = ((c >> 16) & 255) * k; out[2] = ((c >> 8) & 255) * k; out[3] = (c & 255) * k;
}

// The direction and distance from the vertex to each light, worked out once per vertex however many colour or alpha
// channel evaluations use the light.
struct LightGeo { u32 done; f32 d[8][3], dist[8], dist2[8]; };

// Both halves of a colour channel at once (RGB from `cc`, alpha from `ac`): the lights' directions and attenuation are
// worked out once and the same arithmetic as two LightChannel calls gives the same colours.
static void LightChannel2(const PortGx* g, const f32 (*lightCol)[4], u32 cc, u32 ac, const f32* matC, const f32* ambC, const f32* matA, const f32* ambA,
                          const f32* pos, const f32* nrm, f32* out, LightGeo* geo) {
    const BOOL litC = (cc & 2) != 0, litA = (ac & 2) != 0;
    if (!litC && !litA) { out[0] = matC[0]; out[1] = matC[1]; out[2] = matC[2]; out[3] = matA[3]; return; }
    f32 sumC[3] = {ambC[0], ambC[1], ambC[2]}, sumA = ambA[3];
    u32 maskC = litC ? ((cc >> 2) & 15) | ((cc >> 11) & 15) << 4 : 0, maskA = litA ? ((ac >> 2) & 15) | ((ac >> 11) & 15) << 4 : 0;
    const u32 diffC = (cc >> 7) & 3, attnC = (cc >> 9) & 3, diffA = (ac >> 7) & 3, attnA = (ac >> 9) & 3;
    const BOOL same = diffC == diffA && attnC == attnA;
    for (u32 i = 0; i < 8; i++) {
        const u32 bit = 1u << i;
        if (!((maskC | maskA) & bit)) continue;
        const f32* l = &g->xfMem[0x600 + i * 0x10];
        const f32* col = lightCol[i];
        if (!(geo->done & bit)) {
            f32* gd = geo->d[i];
            gd[0] = l[10] - pos[0]; gd[1] = l[11] - pos[1]; gd[2] = l[12] - pos[2];
            f32 dd2 = gd[0] * gd[0] + gd[1] * gd[1] + gd[2] * gd[2], dd = sqrtf(dd2);
            if (dd > 0) { f32 inv = 1.0f / dd; gd[0] *= inv; gd[1] *= inv; gd[2] *= inv; }
            geo->dist2[i] = dd2; geo->dist[i] = dd;
            geo->done |= bit;
        }
        const f32* d = geo->d[i];
        const f32 dist2 = geo->dist2[i], dist = geo->dist[i];
        const f32 nd = nrm[0] * d[0] + nrm[1] * d[1] + nrm[2] * d[2];
        f32 aC = 1.0f, kC = 0;
        if (maskC & bit) {
            if ((attnC & 1) && (attnC & 2)) {               // spot light (specular: left as full strength)
                f32 c = -(d[0] * l[13] + d[1] * l[14] + d[2] * l[15]);
                if (c < 0) c = 0;
                f32 num = l[4] + c * (l[5] + c * l[6]), den = l[7] + dist * l[8] + dist2 * l[9];
                aC = den != 0 ? std::max(num, 0.0f) / den : 0;
            }
            kC = diffC == 0 ? 1.0f : diffC == 1 ? nd : std::max(nd, 0.0f);
            sumC[0] += col[0] * aC * kC; sumC[1] += col[1] * aC * kC; sumC[2] += col[2] * aC * kC;
        }
        if (maskA & bit) {
            f32 aA = aC, kA = kC;
            if (!((maskC & bit) && same)) {
                aA = 1.0f;
                if ((attnA & 1) && (attnA & 2)) {
                    f32 c = -(d[0] * l[13] + d[1] * l[14] + d[2] * l[15]);
                    if (c < 0) c = 0;
                    f32 num = l[4] + c * (l[5] + c * l[6]), den = l[7] + dist * l[8] + dist2 * l[9];
                    aA = den != 0 ? std::max(num, 0.0f) / den : 0;
                }
                kA = diffA == 0 ? 1.0f : diffA == 1 ? nd : std::max(nd, 0.0f);
            }
            sumA += col[3] * aA * kA;
        }
    }
    for (int k = 0; k < 3; k++) out[k] = litC ? matC[k] * std::min(std::max(sumC[k], 0.0f), 1.0f) : matC[k];
    out[3] = litA ? matA[3] * std::min(std::max(sumA, 0.0f), 1.0f) : matA[3];
}

static std::vector<Vtx> sVerts;

static void DrawPrims(const PortGx* g, u32 prim, const PortGxLayout* L, u32 count, const u8* data) {
    if (!sStarted || count == 0) return;
    if (prim >= 0xA8) { sGfxStats.skipped++; return; }     // lines and points: not drawn yet
    u32 texUsed;
    ID3D11PixelShader* ps = GetPixelShader(g, &texUsed);
    if (!ps) { sGfxStats.skipped++; return; }
    f32 proj[7]; memcpy(proj, &g->xf[0x20], 24);
    BOOL ortho = g->xf[0x26] & 1;
    u32 numChans = g->xf[0x09] & 3, numGens = g->xf[0x3F] & 15;
    f32 ambReg[2][4], matReg[2][4];
    for (int c = 0; c < 2; c++) { RegColour(g->xf[0x0A + c], ambReg[c]); RegColour(g->xf[0x0C + c], matReg[c]); }
    u32 midx[2] = {g->cp[0x30], g->cp[0x40]};
    f32 lightCol[8][4];
    BOOL lit = FALSE;                                       // some colour or alpha channel uses lights: otherwise their colours are not needed
    for (u32 c = 0; c < numChans && c < 2; c++) lit |= ((g->xf[0x0E + c] | g->xf[0x10 + c]) & 2) != 0;
    if (lit) for (u32 i = 0; i < 8; i++) { u32 lc; memcpy(&lc, &g->xfMem[0x600 + i * 0x10 + 3], 4); RegColour(lc, lightCol[i]); }

    sVerts.resize(count);
    // Per draw, not per vertex: where each array attribute's data is, and the attributes the vertex format has.
    const u8* arrBase[21] = {}; u32 arrStride[21] = {}; BOOL arrBe[21] = {};
    u32 active[12], nActive = 0;
    for (u32 a = 9; a < 21; a++) {
        if (!L->attr[a].mode) continue;
        if (L->attr[a].mode >= 2) {
            arrBase[a] = PortGxHostPtr(g->cp[0xA0 + (a - 9)]);
            if (!arrBase[a]) continue;
            arrStride[a] = g->cp[0xB0 + (a - 9)] & 0xFF;
            arrBe[a] = PortIsSectionData(arrBase[a]);       // a mesh's arrays are file data; arrays the game builds are in host order
        }
        active[nActive++] = a;
    }
    for (u32 n = 0; n < count; n++, data += L->size) {
        Vtx& o = sVerts[n];
        f32 in[21][4];                                      // position, normal, colours, texture coordinates as given (read only where has[])
        BOOL has[21];
        for (u32 a = 9; a < 21; a++) has[a] = FALSE;
        for (u32 ai = 0; ai < nActive; ai++) {
            const u32 a = active[ai];
            const PortGxAttr& at = L->attr[a];
            const u8* p = data + at.offset;
            BOOL be = TRUE;
            if (at.mode >= 2) {                             // through an array
                u32 index = at.mode == 2 ? p[0] : Be16(p);
                p = arrBase[a] + index * arrStride[a];
                be = arrBe[a];
            }
            has[a] = TRUE;
            if (a == 11 || a == 12) { Colour(p, at.fmt, be, in[a]); continue; }
            u32 comps = a == 10 ? 3 : at.comps;
            in[a][0] = in[a][1] = in[a][2] = 0; in[a][3] = 1;
            if (a >= 13) in[a][2] = 1;
            for (u32 k = 0; k < comps; k++) in[a][k] = Component(p + k * at.elem, at.fmt, a == 10 ? (at.fmt == 1 ? 6 : at.fmt == 3 ? 14 : 0) : at.shift, be);
        }
        if (!has[9]) { memset(&o, 0, sizeof(o)); continue; }
        // position and normal into view space
        u32 pm = L->attr[0].mode ? data[L->attr[0].offset] : (midx[0] & 63);
        f32 pos[3], nrm[3] = {0, 0, 1};
        Mul34(&g->xfMem[(pm & 63) * 4], in[9], pos);
        if (has[10]) {
            const f32* nm = &g->xfMem[0x400 + (pm & 63) % 32 * 3];
            for (int r = 0; r < 3; r++) nrm[r] = nm[r * 3] * in[10][0] + nm[r * 3 + 1] * in[10][1] + nm[r * 3 + 2] * in[10][2];
            f32 len = sqrtf(nrm[0] * nrm[0] + nrm[1] * nrm[1] + nrm[2] * nrm[2]);
            if (len > 0) { nrm[0] /= len; nrm[1] /= len; nrm[2] /= len; }
        }
        // projection; depth 0 (near) to 1 (far) for Direct3D: GX has z/w from -1 to 0
        f32 cz;
        if (ortho) { o.pos[0] = proj[0] * pos[0] + proj[1]; o.pos[1] = proj[2] * pos[1] + proj[3]; cz = proj[4] * pos[2] + proj[5]; o.pos[3] = 1.0f; }
        else { o.pos[0] = proj[0] * pos[0] + proj[1] * pos[2]; o.pos[1] = proj[2] * pos[1] + proj[3] * pos[2]; cz = proj[4] * pos[2] + proj[5]; o.pos[3] = -pos[2]; }
        o.pos[2] = cz + o.pos[3];
        // colour channels
        static const f32 kWhite[4] = {1, 1, 1, 1};
        LightGeo geo; geo.done = 0;
        for (u32 c = 0; c < 2; c++) {
            if (c >= numChans) { o.col[c][0] = o.col[c][1] = o.col[c][2] = 0; o.col[c][3] = 0; if (c == 0 && numChans == 0) memcpy(o.col[0], kWhite, 16); continue; }
            const f32* vc = has[11 + c] ? in[11 + c] : kWhite;
            u32 cc = g->xf[0x0E + c], ac = g->xf[0x10 + c];
            LightChannel2(g, lightCol, cc, ac, (cc & 1) ? vc : matReg[c], (cc & 0x40) ? vc : ambReg[c], (ac & 1) ? vc : matReg[c], (ac & 0x40) ? vc : ambReg[c], pos, nrm, o.col[c], &geo);
        }
        // texture coordinate generation
        for (u32 t = 0; t < numGens && t < 8; t++) {      // (a draw only uploads the coordinates it makes)
            f32* tc = o.tc[t];
            tc[0] = tc[1] = 0; tc[2] = 1;
            u32 gen = g->xf[0x40 + t];
            u32 type = (gen >> 4) & 7, row = (gen >> 7) & 31;
            if (type == 2 || type == 3) { tc[0] = o.col[type - 2][0]; tc[1] = o.col[type - 2][1]; continue; }
            f32 src[4] = {0, 0, 1, 1};
            if (row == 0) { src[0] = in[9][0]; src[1] = in[9][1]; src[2] = in[9][2]; }
            else if (row == 1) { if (has[10]) { src[0] = in[10][0]; src[1] = in[10][1]; src[2] = in[10][2]; } }
            else if (row >= 5 && row <= 12) { if (has[13 + row - 5]) { src[0] = in[13 + row - 5][0]; src[1] = in[13 + row - 5][1]; } }
            if (!((gen >> 2) & 1) && row >= 5) src[2] = 1;  // input form AB11
            u32 mi = L->attr[1 + t].mode ? data[L->attr[1 + t].offset] : (t < 4 ? (midx[0] >> (6 + t * 6)) & 63 : (midx[1] >> ((t - 4) * 6)) & 63);
            const f32* m = &g->xfMem[mi * 4];
            f32 r[3];
            Mul34(m, src, r);
            if (!(gen & 2)) r[2] = 1.0f;                    // two rows: s, t
            if (g->xf[0x12] & 1) {                          // post-transform matrices
                u32 post = g->xf[0x50 + t];
                const f32* pmx = &g->xfMem[0x500 + (post & 63) * 4];
                f32 v[4] = {r[0], r[1], r[2], 1}, q[3];
                if (post & 0x100) { f32 len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); if (len > 0) { v[0] /= len; v[1] /= len; v[2] /= len; } }
                Mul34(pmx, v, q);
                r[0] = q[0]; r[1] = q[1]; r[2] = q[2];
            }
            tc[0] = r[0]; tc[1] = r[1]; tc[2] = r[2] != 0 ? r[2] : 1.0f;
        }
    }

    // Widescreen. The game gives its 3D views a 16:9 projection, but lays out everything 2D
    // (orthographic) for 640x480 and lets the TV stretch it. Here 2D draws keep their shape and
    // stay centred instead: x is narrowed by 3/4. Layers that belong to the whole screen stay as
    // they are: those that show a copy of the frame buffer (glow, blur, the banded frame), and
    // those that reach from edge to edge without a picture on them (fades, bars, backdrops: no
    // texture, or a small one). A full-screen picture (320 texels wide or more) is narrowed.
    f32 squeeze = 1.0f;
    if (ortho && gPortSettings.wide && !gPortSettings.stretchUi && count) {
        BOOL whole = FALSE, picture = FALSE;
        for (u32 m = 0; m < 8; m++) {
            if (!(texUsed & (1u << m))) continue;
            u32 base = (m < 4 ? 0x80 : 0xA0) + (m & 3);
            if (sEfbCopies.count((g->bp[base + 0x14] & 0xFFFFFF) << 5)) whole = TRUE;
            else if ((g->bp[base + 8] & 0x3FF) + 1 >= 320) picture = TRUE;
        }
        if (!whole && !picture) {
            f32 lo = sVerts[0].pos[0], hi = lo;
            for (u32 n = 1; n < count; n++) { lo = std::min(lo, sVerts[n].pos[0]); hi = std::max(hi, sVerts[n].pos[0]); }
            if (lo <= -0.98f && hi >= 0.98f) whole = TRUE;
        }
        if (!whole) {
            squeeze = 0.75f;
            for (u32 n = 0; n < count; n++) sVerts[n].pos[0] *= squeeze;
        }
    }
    // to a triangle list (indices into the vertices, which are uploaded once; written into the batch below)
    if (!(prim == 0x80 ? count >= 4 : (prim == 0x90 || prim == 0x98 || prim == 0xA0) && count >= 3) || count > VB_VERTS) { sGfxStats.skipped++; return; }
    if (sTrace) {
        const Vtx& v = sVerts[0];
        f32 vp[6]; memcpy(vp, &g->xf[0x1A], sizeof(vp));
        PortLog("gfx trace: draw %02X x%u stages %u gens %u chans %u tex %02X bm %04X zm %02X cull %u vp %.0f,%.0f,%.0f,%.0f v0 pos %.2f %.2f %.2f %.2f col %.2f %.2f %.2f %.2f tc0 %.2f %.2f clipoff %u img0 %06X addr %08X",
                prim, count, ((g->bp[0] >> 10) & 15) + 1, numGens, numChans, texUsed, g->bp[0x41] & 0xFFFF, g->bp[0x40] & 0x1F, (g->bp[0] >> 14) & 3,
                vp[0], vp[1], vp[3], vp[4], v.pos[0], v.pos[1], v.pos[2], v.pos[3], v.col[0][0], v.col[0][1], v.col[0][2], v.col[0][3], v.tc[0][0], v.tc[0][1], g->xf[0x05] & 1, g->bp[0x88], (g->bp[0x94] & 0xFFFFFF) << 5);
        for (u32 m = 0; m < 8; m++) if (texUsed & (1u << m)) {
            u32 base = (m < 4 ? 0x80 : 0xA0) + (m & 3);
            PortLog("gfx trace:   map %u: %ux%u format %u at %08X%s; stage 0 colour %06X alpha %06X order %03X", m, (g->bp[base + 8] & 0x3FF) + 1,
                    ((g->bp[base + 8] >> 10) & 0x3FF) + 1, (g->bp[base + 8] >> 20) & 15, (g->bp[base + 0x14] & 0xFFFFFF) << 5,
                    sEfbCopies.count((g->bp[base + 0x14] & 0xFFFFFF) << 5) ? " (a frame-buffer copy)" : "", g->bp[0xC0], g->bp[0xC1], g->bp[0x28] & 0xFFF);
        }
    }
    if (sTrace && getenv("PORT_GFX_VERTS")) {
        for (u32 i = 0; i < count && i < 6; i++)
            PortLog("gfx trace:   v%u pos %.3f %.3f %.3f %.3f col %.2f %.2f %.2f %.2f tc0 %.3f %.3f %.3f tc1 %.3f %.3f", i, sVerts[i].pos[0], sVerts[i].pos[1], sVerts[i].pos[2],
                    sVerts[i].pos[3], sVerts[i].col[0][0], sVerts[i].col[0][1], sVerts[i].col[0][2], sVerts[i].col[0][3], sVerts[i].tc[0][0], sVerts[i].tc[0][1], sVerts[i].tc[0][2],
                    sVerts[i].tc[1][0], sVerts[i].tc[1][1]);
    }
    PsConst pc;
    memset(&pc, 0, sizeof(pc));
    // The constants only change now and then: rebuild them when one of the registers they come from did.
    struct PcSrc { s16 reg[16], konst[16]; u32 bp[8 + 9 + 5 + 2 + 1]; };
    static PcSrc lastSrc; static PsConst lastPcVal; static BOOL haveLast;
    PcSrc src;
    memcpy(src.reg, g->tevReg, sizeof(src.reg)); memcpy(src.konst, g->tevKonst, sizeof(src.konst));
    {
        u32 n = 0;
        for (u32 m = 0; m < 8; m++) src.bp[n++] = g->bp[(m < 4 ? 0x80 : 0xA0) + (m & 3) + 8];
        for (u32 i = 0x06; i < 0x0F; i++) src.bp[n++] = g->bp[i];
        for (u32 i = 0xEE; i < 0xF3; i++) src.bp[n++] = g->bp[i];
        src.bp[n++] = g->bp[0x25]; src.bp[n++] = g->bp[0x26];
        src.bp[n++] = g->bp[0xF3];
    }
    if (haveLast && !memcmp(&src, &lastSrc, sizeof(src))) pc = lastPcVal;
    else {
    for (int i = 0; i < 4; i++) for (int k = 0; k < 4; k++) { pc.reg[i][k] = g->tevReg[i][k] / 255.0f; pc.konst[i][k] = g->tevKonst[i][k] / 255.0f; }
        pc.alphaRef[0] = (g->bp[0xF3] & 255) / 255.0f; pc.alphaRef[1] = ((g->bp[0xF3] >> 8) & 255) / 255.0f; pc.alphaRef[2] = pc.alphaRef[3] = 0;
        for (u32 m = 0; m < 8; m++) {
            u32 img0 = g->bp[(m < 4 ? 0x80 : 0xA0) + (m & 3) + 8];
            pc.texSize[m][0] = (f32)((img0 & 0x3FF) + 1); pc.texSize[m][1] = (f32)(((img0 >> 10) & 0x3FF) + 1); pc.texSize[m][2] = pc.texSize[m][3] = 0;
        }
        for (u32 m = 0; m < 3; m++) {
            // BP 0x06+3m..: six 11-bit signed elements (a, b | c, d | e, f) and a 6-bit scale. The
            // offset in 1/128 texels is (M . v) >> 3, shifted by 17 - scale.
            u32 r0 = g->bp[0x06 + m * 3], r1 = g->bp[0x07 + m * 3], r2 = g->bp[0x08 + m * 3];
            s32 e[6] = {(s32)(r0 << 21) >> 21, (s32)(r0 << 10) >> 21, (s32)(r1 << 21) >> 21, (s32)(r1 << 10) >> 21, (s32)(r2 << 21) >> 21, (s32)(r2 << 10) >> 21};
            s32 scale = (s32)(((r0 >> 22) & 3) | ((r1 >> 22) & 3) << 2 | ((r2 >> 22) & 3) << 4);
            f32 k = (f32)Pow2(scale - 17) / 8.0f / 128.0f;
            pc.indRow[m][0][0] = e[0] * k; pc.indRow[m][0][1] = e[2] * k; pc.indRow[m][0][2] = e[4] * k; pc.indRow[m][0][3] = 0;
            pc.indRow[m][1][0] = e[1] * k; pc.indRow[m][1][1] = e[3] * k; pc.indRow[m][1][2] = e[5] * k; pc.indRow[m][1][3] = 0;
        }
        {
            u32 ab = (g->bp[0xEE] & 0xFFFFF) << 12, cb = (g->bp[0xF1] & 0xFFFFF) << 12;
            f32 a, c;
            memcpy(&a, &ab, 4); memcpy(&c, &cb, 4);
            s32 expReg = g->bp[0xF0] & 31;
            f32 b = (f32)(g->bp[0xEF] & 0xFFFFFF) / 8388638.0f * (f32)Pow2(expReg - 1);
            pc.fogP[0] = ((g->bp[0xF1] >> 20) & 1) ? a : a * (f32)Pow2(expReg);
            pc.fogP[1] = b; pc.fogP[2] = c; pc.fogP[3] = 0;
            pc.fogC[0] = ((g->bp[0xF2] >> 16) & 255) / 255.0f; pc.fogC[1] = ((g->bp[0xF2] >> 8) & 255) / 255.0f; pc.fogC[2] = (g->bp[0xF2] & 255) / 255.0f; pc.fogC[3] = 1.0f;
        }
        for (u32 i = 0; i < 4; i++) {
            u32 sc = g->bp[0x25 + i / 2] >> ((i & 1) * 8);
            pc.indScale[i][0] = 1.0f / (f32)(1u << (sc & 15)); pc.indScale[i][1] = 1.0f / (f32)(1u << ((sc >> 4) & 15)); pc.indScale[i][2] = pc.indScale[i][3] = 0;
        }
        lastSrc = src; lastPcVal = pc; haveLast = TRUE;
    }
    OutKey ok = MakeOutKey(g, squeeze);
    ID3D11ShaderResourceView* srv[8]; ID3D11SamplerState* smp[8];
    for (u32 i = 0; i < 8; i++) { srv[i] = (texUsed & (1u << i)) ? GetTexture(g, i) : sWhite; smp[i] = (texUsed & (1u << i)) ? GetSampler(g, i) : sLinearClamp; }

    // Draws that need exactly the same state as the ones queued before are added to them: one upload and one draw call for the lot.
    if (sPendCount && (sPend.ps != ps || sBVCount + count > VB_VERTS || sPend.ngen != numGens || memcmp(&sPend.out, &ok, sizeof(ok)) || memcmp(sPend.srv, srv, sizeof(srv)) ||
                       memcmp(sPend.smp, smp, sizeof(smp)) || memcmp(&sPend.pc, &pc, sizeof(pc))))
        FlushDraws();
    if (!sPendCount) { sPend.ngen = numGens > 8 ? 8 : numGens; sPend.ps = ps; sPend.out = ok; memcpy(sPend.srv, srv, sizeof(srv)); memcpy(sPend.smp, smp, sizeof(smp)); sPend.pc = pc; }
    u16 base = (u16)sBVCount;
    {
        u32 stride = 48 + 12 * sPend.ngen;
        size_t at = sBV.size();
        sBV.resize(at + (size_t)count * stride);
        for (u32 n = 0; n < count; n++) memcpy(&sBV[at + (size_t)n * stride], &sVerts[n], stride);
        sBVCount += count;
    }
    const size_t nIdxBefore = sBI.size();
    {
        size_t n = sBI.size();
        if (prim == 0x80) {
            u32 quads = count / 4;
            sBI.resize(n + (size_t)quads * 6);
            u16* o = &sBI[n];
            for (u32 i = 0; i < quads * 4; i += 4, o += 6) { u16 q = (u16)(base + i); o[0] = q; o[1] = (u16)(q + 1); o[2] = (u16)(q + 2); o[3] = q; o[4] = (u16)(q + 2); o[5] = (u16)(q + 3); }
        } else if (prim == 0x90) {
            u32 tris = count / 3;
            sBI.resize(n + (size_t)tris * 3);
            u16* o = &sBI[n];
            for (u32 i = 0; i < tris * 3; i++) o[i] = (u16)(base + i);
        } else if (prim == 0x98) {
            sBI.resize(n + (size_t)(count - 2) * 3);
            u16* o = &sBI[n];
            for (u32 i = 0; i + 2 < count; i++, o += 3) { o[0] = (u16)(base + i); o[1] = (u16)(base + i + 1 + (i & 1)); o[2] = (u16)(base + i + 2 - (i & 1)); }
        } else {
            sBI.resize(n + (size_t)(count - 2) * 3);
            u16* o = &sBI[n];
            for (u32 i = 1; i + 1 < count; i++, o += 3) { o[0] = base; o[1] = (u16)(base + i); o[2] = (u16)(base + i + 1); }
        }
    }
    sGfxStats.tris += (u32)(sBI.size() - nIdxBefore) / 3;
    sPendCount++;
    sGfxStats.draws++;
    sFrame.draws++; sFrame.verts += count;
}

// Uploads and draws what DrawPrims has queued.
static void FlushDraws() {
    if (!sPendCount) return;
    sPendCount = 0;
    // The vertices go behind those of the earlier flushes; the buffer (9 MB) is only replaced when
    // it is full. Replacing it for every draw made a scene of a few thousand small draws crawl.
    static u32 sVbUsed, sIbUsed;                            // in bytes and indices
    const u32 stride = 48 + 12 * sPend.ngen;
    u32 bytes = (u32)sBV.size(), ni = (u32)sBI.size();
    u32 first = sVbUsed, ifirst = sIbUsed;
    first = (first + stride - 1) / stride * stride;           // a whole number of vertices from the start of the buffer
    D3D11_MAPPED_SUBRESOURCE map;
    D3D11_MAP how = D3D11_MAP_WRITE_NO_OVERWRITE;
    if (first + bytes > VB_VERTS * sizeof(Vtx)) { first = 0; how = D3D11_MAP_WRITE_DISCARD; }
    if (FAILED(sCtx->Map(sVb, 0, how, 0, &map))) { sBV.clear(); sBI.clear(); sBVCount = 0; return; }
    memcpy((u8*)map.pData + first, sBV.data(), bytes);
    sCtx->Unmap(sVb, 0);
    sVbUsed = first + bytes;
    how = D3D11_MAP_WRITE_NO_OVERWRITE;
    if (ifirst + ni > IB_IDXS) { ifirst = 0; how = D3D11_MAP_WRITE_DISCARD; }
    if (FAILED(sCtx->Map(sIb, 0, how, 0, &map))) { sBV.clear(); sBI.clear(); sBVCount = 0; return; }
    memcpy((u16*)map.pData + ifirst, sBI.data(), ni * sizeof(u16));
    sCtx->Unmap(sIb, 0);
    sIbUsed = ifirst + ni;
    sBV.clear(); sBI.clear(); sBVCount = 0;

    static PsConst lastPc; static OutKey lastOut;
    if (!sDrawBound || memcmp(&sPend.pc, &lastPc, sizeof(lastPc))) { sCtx->UpdateSubresource(sPsConst, 0, NULL, &sPend.pc, 0, 0); lastPc = sPend.pc; }
    if (!sDrawBound) sCtx->OMSetRenderTargets(1, &sEfbRtv, sEfbDsv);
    if (!sDrawBound || memcmp(&sPend.out, &lastOut, sizeof(lastOut))) { SetOutputState(sPend.out); lastOut = sPend.out; }
    static ID3D11ShaderResourceView* lastSrv[8]; static ID3D11SamplerState* lastSmp[8]; static ID3D11PixelShader* lastPs;
    if (!sDrawBound || memcmp(sPend.srv, lastSrv, sizeof(lastSrv))) { sCtx->PSSetShaderResources(0, 8, sPend.srv); memcpy(lastSrv, sPend.srv, sizeof(lastSrv)); }
    if (!sDrawBound || memcmp(sPend.smp, lastSmp, sizeof(lastSmp))) { sCtx->PSSetSamplers(0, 8, sPend.smp); memcpy(lastSmp, sPend.smp, sizeof(lastSmp)); }
    static u32 lastNgen = ~0u;
    UINT vbStride = stride, vbOffset = first;
    sCtx->IASetVertexBuffers(0, 1, &sVb, &vbStride, &vbOffset);
    if (!sDrawBound || lastNgen != sPend.ngen) {
        sCtx->IASetInputLayout(sLayouts[sPend.ngen]);
        sCtx->VSSetShader(sVs[sPend.ngen], NULL, 0);
        lastNgen = sPend.ngen;
    }
    if (!sDrawBound) {
        sCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        sCtx->PSSetConstantBuffers(0, 1, &sPsConst);
        sCtx->IASetIndexBuffer(sIb, DXGI_FORMAT_R16_UINT, 0);
    }
    if (!sDrawBound || sPend.ps != lastPs) { sCtx->PSSetShader(sPend.ps, NULL, 0); lastPs = sPend.ps; }
    sDrawBound = TRUE;
    sCtx->DrawIndexed(ni, ifirst, 0);
}

// ------------------------------------------------------------------ frame-buffer copies
static void Blit(ID3D11ShaderResourceView* src, ID3D11RenderTargetView* dst, u32 dstX, u32 dstY, u32 dstW, u32 dstH, f32 u0, f32 v0, f32 du, f32 dv, const f32* pick = NULL) {
    sDrawBound = FALSE;
    ID3D11ShaderResourceView* none[8] = {};
    f32 rect[8] = {u0, v0, du, dv, 0, 0, 0, 0};
    if (pick) memcpy(rect + 4, pick, 16);
    sCtx->UpdateSubresource(sBlitConst, 0, NULL, rect, 0, 0);
    sCtx->OMSetRenderTargets(1, &dst, NULL);
    D3D11_VIEWPORT v = {(f32)dstX, (f32)dstY, (f32)dstW, (f32)dstH, 0, 1};
    D3D11_RECT sc = {(LONG)dstX, (LONG)dstY, (LONG)(dstX + dstW), (LONG)(dstY + dstH)};
    sCtx->RSSetViewports(1, &v);
    sCtx->RSSetScissorRects(1, &sc);
    sCtx->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
    sCtx->OMSetDepthStencilState(NULL, 0);
    sCtx->RSSetState(NULL);
    sCtx->IASetInputLayout(NULL);
    sCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    sCtx->VSSetShader(sBlitVs, NULL, 0);
    sCtx->PSSetShader(sBlitPs, NULL, 0);
    sCtx->VSSetConstantBuffers(0, 1, &sBlitConst);
    sCtx->PSSetConstantBuffers(0, 1, &sBlitConst);
    sCtx->PSSetShaderResources(0, 1, &src);
    sCtx->PSSetSamplers(0, 1, &sLinearClamp);
    sCtx->Draw(3, 0);
    sCtx->PSSetShaderResources(0, 8, none);
    sCtx->OMSetRenderTargets(0, NULL, NULL);
}

// The numbers of the frame that ends at this display copy, for the overlay.
static void FrameStats(PortFrameStats* s) {
    static s64 last, lastWait, freq;
    if (!freq) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); freq = f.QuadPart; }
    s64 now = Ticks(), wait = gPortWaitTicks;
    f64 ms = 1000.0 / (f64)freq;
    memset(s, 0, sizeof(*s));
    if (last) {
        s->frameMs = (f32)((now - last) * ms);
        s->renderMs = (f32)(sFrame.renderTicks * ms);
        s->waitMs = (f32)((wait - lastWait) * ms);
        s->gameMs = std::max(s->frameMs - s->renderMs - s->waitMs, 0.0f);
    }
    last = now; lastWait = wait;
    sTexGen++;
    s->draws = sFrame.draws; s->verts = sFrame.verts; s->efbCopies = sFrame.copies;
    s->textures = (u32)(sTextures.size() + sEfbCopies.size());
    s->voices = PortAudioVoices(&s->voiceCount);
    s->outW = sOutW; s->outH = sOutH; s->picW = sPicW; s->picH = sPicH; s->wide = gPortSettings.wide;
    // PORT_PERF_LOG=1: average split of every 300 shown frames on stderr (headless profiling without screenshots).
    static int perfOn = -1, perfN;
    static f64 perfSum[7];
    if (perfOn < 0) { const char* e = getenv("PORT_PERF_LOG"); perfOn = e && *e == '1'; }
    if (perfOn && last && s->frameMs > 0.0f) {
        f64 v[7] = { s->frameMs, s->gameMs, s->renderMs, s->waitMs, (f64)sFrame.draws, (f64)sFrame.verts, (f64)sFrame.copies };
        for (int i = 0; i < 7; i++) perfSum[i] += v[i];
        if (++perfN == 300) {
            PortLog("perf: frame %.2f ms  game %.2f  render %.2f  wait %.2f  draws %.0f  verts %.0f  copies %.1f\n",
                    perfSum[0] / perfN, perfSum[1] / perfN, perfSum[2] / perfN, perfSum[3] / perfN, perfSum[4] / perfN, perfSum[5] / perfN, perfSum[6] / perfN);
            memset(perfSum, 0, sizeof(perfSum)); perfN = 0;
        }
    }
    sFrame.renderTicks = 0; sFrame.draws = sFrame.verts = sFrame.copies = 0;
}

// Whether the window should get a picture now. Paced: always. Fast-forwarding: four times a second,
// and then the title shows "Fast-forwarding... frame N / M" (set from the window's thread).
static BOOL FastProgress(void) {
    static DWORD lastFast;
    static BOOL  titled;
    if (!sSwap) return FALSE;
    if (!PortFastActive()) {
        if (titled) { PostTitle("Monster Lab (PC port)"); titled = FALSE; }
        return TRUE;
    }
    DWORD now = GetTickCount();
    if (now - lastFast < 250) return FALSE;
    lastFast = now;
    char t[96];
    s32 total = gPortOptions.fastForward;
    if (total > 0) snprintf(t, sizeof(t), "Monster Lab (PC port) - Fast-forwarding... frame %d / %d", (int)PortFrameCount(), (int)total);
    else snprintf(t, sizeof(t), "Monster Lab (PC port) - Fast-forwarding... frame %d", (int)PortFrameCount());
    PostTitle(t);
    titled = TRUE;
    if (getenv("PORT_FAST_LOG")) PortLog("fast-forward progress: present + title \"%s\" at %lu ms", t, now);
    return TRUE;
}
// ---- frame rate (Settings > Display) ----
// The game logic runs at the console's 50/60 Hz retrace (PortViTick) whatever is set here: the setting only
// decides how often the picture is shown. With a cap (30, 60, 120, 144...), "display" or "uncapped" the game
// thread, while it waits for the next retrace, shows the last picture again (PortGfxIdle) at that rate, so the
// overlay and the settings screen stay smooth; a cap below the game's rate skips pictures. Off: one picture per
// game frame, with the Vertical sync setting as before.
static std::recursive_mutex    sGfxLock;        // the draw thread and the idle presents share the device context
static s64                     sNextPresent;    // QueryPerformanceCounter ticks: when the next picture may be shown
static DWORD                   sDrawThread;     // the thread that makes the game's pictures: the only one that shows them again
static u32                     sExtraPictures;  // pictures shown again while waiting for a retrace
static u32 DisplayHz(void) {
    DEVMODEA dm = {}; dm.dmSize = sizeof(dm);
    return EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 1 ? dm.dmDisplayFrequency : 60;
}
static s64 PresentInterval(void) {          // ticks between pictures; 0 = no limit
    s32 cap = gPortSettings.fpsCap;
    if (cap > 0) return Ticks_freq() / cap;
    if (cap == PORT_FPS_DISPLAY) return Ticks_freq() / DisplayHz();
    return 0;
}
// Pictures shown per second, new or repeated (the overlay's display rate), measured over about half a second.
static volatile LONG sShownCount;
f32 PortGfxDisplayFps(void) {
    static s64 t0; static LONG n0; static f32 rate;
    s64 now = Ticks();
    if (!t0) { t0 = now; n0 = sShownCount; }
    s64 dt = now - t0;
    if (dt * 2 >= Ticks_freq()) { rate = (f32)(sShownCount - n0) * (f32)Ticks_freq() / (f32)dt; t0 = now; n0 = sShownCount; }
    return rate;
}
static char sCapPrefix[400];
static BOOL sCapBack;
static void PresentOut(BOOL fast) {
    sShownCount++;
    Blit(sOutSrv, sBackRtv, 0, 0, sOutW, sOutH, 0, 0, 1, 1);
    if (!gPortOptions.screenshotStats) PortOverlayDraw(sBackRtv, sOutW, sOutH);
    if (sCapBack && sSwap) {                                // F12: the back buffer as it is about to be shown
        sCapBack = FALSE;
        ID3D11Texture2D* bb = NULL;
        if (SUCCEEDED(sSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) && bb) {
            char p[420];
            snprintf(p, sizeof(p), "%s_backbuffer.png", sCapPrefix);
            SavePng(bb, sOutW, sOutH, p);
            bb->Release();
        }
    }
    s32 cap = gPortSettings.fpsCap;
    BOOL sync = cap == 0 ? gPortSettings.vsync != 0 : cap == PORT_FPS_DISPLAY;
    if (sSwap) sSwap->Present(sync && !fast ? 1 : 0, 0);
}
// A picture of the game: with a cap below the game's rate some are skipped.
static BOOL GamePresentDue(void) {
    s64 iv = PresentInterval();
    if (!iv || gPortSettings.fpsCap == PORT_FPS_DISPLAY || PortFastActive()) return TRUE;
    s64 now = Ticks();
    if (now + iv / 4 < sNextPresent) return FALSE;
    sNextPresent = std::max(sNextPresent + iv, now - iv);
    return TRUE;
}
BOOL PortGfxIdle(void) {
    s32 cap = gPortSettings.fpsCap;
    if (!cap || !sStarted || !sShown || PortFastActive() || GetCurrentThreadId() != sDrawThread) return FALSE;
    s64 iv = PresentInterval();
    s64 retrace = PortViNextTicks();
    if (retrace && PortTicks() >= retrace) return FALSE;
    PortLockLeave();                                    // the interrupt and audio threads run meanwhile
    BOOL bail = FALSE;
    while (iv && Ticks() < sNextPresent) {
        s64 r = PortViNextTicks();
        if (r && PortTicks() >= r) { bail = TRUE; break; }
        if ((sNextPresent - Ticks()) * 1000 / Ticks_freq() > 2) Sleep(1); else YieldProcessor();
    }
    if (!bail) {
        std::lock_guard<std::recursive_mutex> lk(sGfxLock);
        s64 now = Ticks();
        if (iv) sNextPresent = std::max(sNextPresent + iv, now - iv);
        PresentOut(FALSE);
        sExtraPictures++;
    }
    PortLockEnter();
    return !bail;
}

static void CopyEfb(const PortGx* g, u32 control) {
    if (!sStarted) return;
    s64 tf = Ticks();
    FlushDraws();
    sTotalFlush += Ticks() - tf;
    sDrawBound = FALSE;
    s64 tr = Ticks();
    if (sEfbMs) sCtx->ResolveSubresource(sEfbTex, 0, sEfbMs, 0, DXGI_FORMAT_R8G8B8A8_UNORM);   // every copy reads the resolved picture
    sTRes += Ticks() - tr;
    if (sTrace) {
        static int n;
        char path[64];
        snprintf(path, sizeof(path), "build/pc/efb_%02d.png", n++);
        sCtx->OMSetRenderTargets(0, NULL, NULL);
        SavePng(sEfbTex, sEfbPw, sEfbPh, path);
        PortLog("gfx trace: copy %s control %06X src %u,%u %ux%u dest %08X -> %s", (control & 0x4000) ? "to display" : "to texture", control,
                g->bp[0x49] & 0x3FF, (g->bp[0x49] >> 10) & 0x3FF, (g->bp[0x4A] & 0x3FF) + 1, ((g->bp[0x4A] >> 10) & 0x3FF) + 1,
                (g->bp[0x4B] & 0xFFFFFF) << 5, path);
    }
    u32 x = g->bp[0x49] & 0x3FF, y = (g->bp[0x49] >> 10) & 0x3FF;
    u32 w = (g->bp[0x4A] & 0x3FF) + 1, h = ((g->bp[0x4A] >> 10) & 0x3FF) + 1;
    if (x + w > EFB_W) w = EFB_W - x;
    if (y + h > EFB_H) h = EFB_H - y;
    // The source in frame-buffer pixels: whole pixels, placed as the draws of this band were.
    s32 offX, offY;
    ScissorOffset(g, &offX, &offY);
    u32 px0 = (u32)std::max(PxX((s32)x + offX) - PxX(offX), 0), py0 = (u32)std::max(PxY((s32)y + offY) - PxY(offY), 0);
    u32 px1 = (u32)std::min(PxX((s32)(x + w) + offX) - PxX(offX), (s32)sEfbPw), py1 = (u32)std::min(PxY((s32)(y + h) + offY) - PxY(offY), (s32)sEfbPh);
    u32 pw = px1 - px0, ph = py1 - py0;
    f32 su0 = (f32)px0 / sEfbPw, sv0 = (f32)py0 / sEfbPh, sdu = (f32)pw / sEfbPw, sdv = (f32)ph / sEfbPh;
    if (sFilterDirty) {                                     // texture filtering changed: decode again, with or without mips
        sFilterDirty = FALSE;
        for (auto& e : sTextures) if (e.second.srv) e.second.srv->Release();
        sTextures.clear();
        for (auto& sm : sSamplers) if (sm) { sm->Release(); sm = NULL; }
        sDrawBound = FALSE;
    }
    s64 td0 = Ticks();
    if (control & 0x4000) {                                 // to the display: the picture's part of the output
        f32 black[4] = {0, 0, 0, 1};
        if (sPicW != sOutW || sPicH != sOutH) sCtx->ClearRenderTargetView(sOutRtv, black);
        Blit(sEfbSrv, sOutRtv, sPicX, sPicY, sPicW, sPicH, su0, sv0, sdu, sdv);
        sShown = TRUE;
        sDrawThread = GetCurrentThreadId();
        // The overlay goes over the window's picture only, or with --screenshot-stats into the output picture itself.
        PortFrameStats stats;
        FrameStats(&stats);
        PortOverlayFrame(&stats);
        PortCaptureFrame();
        if (gPortOptions.screenshotStats) PortOverlayDraw(sOutRtv, sOutW, sOutH);
        // Fast-forward: the window gets a picture only four times a second, with the progress in the title bar.
        BOOL fast = PortFastActive();
        s64 tp = Ticks();
        s64 tPres = Ticks();
        if (FastProgress() && GamePresentDue()) PresentOut(fast);
        if (PortPerfActive()) PortPerfFrame(&stats, (f32)((Ticks() - tPres) * 1000.0 / (f64)Ticks_freq()));
        sTPres += Ticks() - tp;
        sGfxStats.presents++;
        if (sTraceFrom > 1 && sGfxStats.presents >= sTraceFrom) sTrace = TRUE;
        sTDisp += Ticks() - td0;
    } else {                                                // into a texture, kept at the frame buffer's resolution
        // The copy's format (bits 3-6, low bit = the second half of the list) says which
        // channels it takes: 8 red, 9 green, 10 blue, 7 alpha as an intensity texture; the others
        // are kept as a colour picture with the frame buffer's alpha.
        u32 addr = (g->bp[0x4B] & 0xFFFFFF) << 5;
        u32 tpf = (control >> 3) & 15, fmt = tpf / 2 + (tpf & 1) * 8;
        static const f32 kPick[5][4] = {{0, 0, 0, 1}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, -1}};
        const f32* pick = fmt == 7 ? kPick[0] : fmt == 8 ? kPick[1] : fmt == 9 ? kPick[2] : fmt == 10 ? kPick[3] : kPick[4];
        if (fmt == 0 || fmt == 1) pick = kPick[1];          // R4, and the luminance formats: red stands in
        // A frame drawn in bands is copied band by band into one console texture: a copy whose
        // address is a whole number of rows into an earlier copy of the same width and format
        // (at or before its end) becomes the rows from there on of that copy.
        static const u8 kBits[16] = {4, 8, 8, 16, 16, 16, 32, 8, 8, 8, 8, 16, 16, 16, 16, 16};
        // Bit 9 (the mipmap flag of GXSetTexCopyDst): the copy is box-filtered to half the size in both
        // directions. The blur chain of the menu takes its source that way (640x240 -> 320x120 per band).
        bool half = (control & 0x200) != 0;
        u32 dw = half ? pw / 2 : pw, dh = half ? ph / 2 : ph;
        u32 rowBytes = (half ? w / 2 : w) * kBits[fmt & 15] / 8, row = 0;
        ID3D11Texture2D* under = NULL;
        D3D11_TEXTURE2D_DESC ud = {};
        for (auto& e : sEfbCopyRowBytes) {
            if (e.first >= addr || e.second != rowBytes || (addr - e.first) % rowBytes) continue;
            auto base = sEfbCopies.find(e.first);
            if (base == sEfbCopies.end() || !base->second) continue;
            ID3D11Resource* res = NULL;
            base->second->GetResource(&res);
            ID3D11Texture2D* bt = NULL;
            res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&bt);
            res->Release();
            if (!bt) continue;
            bt->GetDesc(&ud);
            if (ud.Width == dw && (u32)PxY((addr - e.first) / rowBytes) <= ud.Height) {
                under = bt; row = (addr - e.first) / rowBytes; addr = e.first;
                break;
            }
            bt->Release();
        }
        // The texture of the last copy to this address is used again when it has the right size
        // (a new one for every copy made a 1080p frame crawl); a band is drawn into its rows.
        ID3D11Texture2D* t = under;
        u32 needH = (u32)PxY(row) + dh;
        if (!t) {
            auto old = sEfbCopies.find(addr);
            if (old != sEfbCopies.end() && old->second) {
                ID3D11Resource* res = NULL;
                old->second->GetResource(&res);
                res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&t);
                res->Release();
                if (t) { t->GetDesc(&ud); if (ud.Width != dw || ud.Height != needH) { t->Release(); t = NULL; } }
            }
        } else if (ud.Height < needH) {
            t = NULL;                                       // the band reaches below the texture: a taller one, with the old rows
        }
        if (!t) {
            D3D11_TEXTURE2D_DESC d = {};
            d.Width = dw; d.Height = needH; d.MipLevels = 1; d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            d.SampleDesc.Count = 1; d.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            // A copy texture that was replaced goes into a small pool: the blur chain of the menu makes a taller texture
            // out of the same address every band and frame, and CreateTexture2D took about 0.8 ms each at 1080p. A pooled
            // texture of the same size is completely overwritten (blit, or copy of the rows below) before it is read.
            sNewTex++; s64 tn = Ticks();
            for (size_t i = 0; i < sTexPool.size(); i++) {
                D3D11_TEXTURE2D_DESC pd;
                sTexPool[i]->GetDesc(&pd);
                if (pd.Width == d.Width && pd.Height == d.Height) { t = sTexPool[i]; sTexPool.erase(sTexPool.begin() + i); break; }
            }
            if (t || SUCCEEDED(sDev->CreateTexture2D(&d, NULL, &t))) {
                if (under) { D3D11_BOX all = {0, 0, 0, ud.Width, ud.Height, 1}; sCtx->CopySubresourceRegion(t, 0, 0, 0, 0, under, 0, &all); }
                ID3D11ShaderResourceView*& slot = sEfbCopies[addr];
                if (slot) {
                    ID3D11Resource* res = NULL;
                    slot->GetResource(&res);
                    ID3D11Texture2D* ot = NULL;
                    if (res) { res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&ot); res->Release(); }
                    if (ot && sTexPool.size() < 16) sTexPool.push_back(ot);
                    else if (ot) ot->Release();
                    slot->Release();
                }
                slot = NULL;
                sDev->CreateShaderResourceView(t, NULL, &slot);
            }
            sTNew += Ticks() - tn;
            if (under) under->Release();
        }
        if (t) {
            ID3D11RenderTargetView* rtv = NULL;
            s64 tv = Ticks(); sDev->CreateRenderTargetView(t, NULL, &rtv); sTRtv += Ticks() - tv;
            if (rtv) { s64 tb = Ticks(); Blit(sEfbSrv, rtv, 0, (u32)PxY(row), dw, dh, su0, sv0, sdu, sdv, pick); sTBlit += Ticks() - tb; rtv->Release(); }
            if (!row || !sEfbCopyRowBytes.count(addr)) sEfbCopyRowBytes[addr] = rowBytes;
            t->Release();
        }
        sGfxStats.texCopies++;
        sFrame.copies++;
    }
    sTMk += Ticks() - td0;
    s64 tcl = Ticks();
    if (control & 0x800) {                                  // clear afterwards (BP 0x4F alpha/red, 0x50 green/blue, 0x51 depth)
        f32 c[4] = {(g->bp[0x4F] & 255) / 255.0f, ((g->bp[0x50] >> 8) & 255) / 255.0f, (g->bp[0x50] & 255) / 255.0f, ((g->bp[0x4F] >> 8) & 255) / 255.0f};
        if (g->bp[0x41] & 0x18) sCtx->ClearRenderTargetView(sEfbRtv, c);
        if (g->bp[0x40] & 0x10) sCtx->ClearDepthStencilView(sEfbDsv, D3D11_CLEAR_DEPTH, (g->bp[0x51] & 0xFFFFFF) / 16777215.0f, 0);
    }
    sTPres += Ticks() - tcl;
}

// ------------------------------------------------------------------ PNG (stored, no compression)
static u32 Crc(const u8* p, size_t n, u32 crc) {
    static u32 table[256];
    if (!table[1]) for (u32 i = 0; i < 256; i++) { u32 c = i; for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1; table[i] = c; }
    for (size_t i = 0; i < n; i++) crc = table[(crc ^ p[i]) & 255] ^ (crc >> 8);
    return crc;
}
static void Chunk(FILE* f, const char* type, const u8* data, u32 n) {
    u8 len[4] = {(u8)(n >> 24), (u8)(n >> 16), (u8)(n >> 8), (u8)n};
    fwrite(len, 1, 4, f); fwrite(type, 1, 4, f);
    if (n) fwrite(data, 1, n, f);
    u32 crc = Crc((const u8*)type, 4, 0xFFFFFFFFu);
    if (n) crc = Crc(data, n, crc);
    crc ^= 0xFFFFFFFFu;
    u8 c[4] = {(u8)(crc >> 24), (u8)(crc >> 16), (u8)(crc >> 8), (u8)crc};
    fwrite(c, 1, 4, f);
}

static BOOL SavePng(ID3D11Texture2D* tex, u32 sLastW, u32 sLastH, const char* path) {
    D3D11_TEXTURE2D_DESC d;
    tex->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING; d.BindFlags = 0; d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* st = NULL;
    if (FAILED(sDev->CreateTexture2D(&d, NULL, &st))) return FALSE;
    sCtx->CopyResource(st, tex);
    D3D11_MAPPED_SUBRESOURCE map;
    if (FAILED(sCtx->Map(st, 0, D3D11_MAP_READ, 0, &map))) { st->Release(); return FALSE; }
    // rows of filter byte + RGB, in stored deflate blocks
    u32 row = 1 + sLastW * 3;
    std::vector<u8> raw((size_t)row * sLastH);
    for (u32 y = 0; y < sLastH; y++) {
        const u8* s = (const u8*)map.pData + (size_t)y * map.RowPitch;
        u8* o = &raw[(size_t)y * row];
        *o++ = 0;
        for (u32 x = 0; x < sLastW; x++) { *o++ = s[x * 4]; *o++ = s[x * 4 + 1]; *o++ = s[x * 4 + 2]; }
    }
    sCtx->Unmap(st, 0);
    st->Release();
    std::vector<u8> z;
    z.push_back(0x78); z.push_back(0x01);
    u32 a = 1, b = 0;
    for (size_t at = 0; at < raw.size();) {
        u32 n = (u32)std::min<size_t>(65535, raw.size() - at);
        z.push_back(at + n == raw.size() ? 1 : 0);
        z.push_back((u8)n); z.push_back((u8)(n >> 8)); z.push_back((u8)~n); z.push_back((u8)(~n >> 8));
        z.insert(z.end(), raw.begin() + at, raw.begin() + at + n);
        for (u32 i = 0; i < n; i++) { a = (a + raw[at + i]) % 65521; b = (b + a) % 65521; }
        at += n;
    }
    u32 adler = b << 16 | a;
    z.push_back((u8)(adler >> 24)); z.push_back((u8)(adler >> 16)); z.push_back((u8)(adler >> 8)); z.push_back((u8)adler);
    FILE* f = fopen(path, "wb");
    if (!f) return FALSE;
    static const u8 sig[8] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    u8 ihdr[13] = {(u8)(sLastW >> 24), (u8)(sLastW >> 16), (u8)(sLastW >> 8), (u8)sLastW, (u8)(sLastH >> 24), (u8)(sLastH >> 16), (u8)(sLastH >> 8), (u8)sLastH, 8, 2, 0, 0, 0};
    Chunk(f, "IHDR", ihdr, 13);
    Chunk(f, "IDAT", z.data(), (u32)z.size());
    Chunk(f, "IEND", NULL, 0);
    fclose(f);
    return TRUE;
}

// A texture of another device as a PNG (the first-run screen's test picture).
BOOL PortGfxSaveTexture(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, u32 w, u32 h, const char* path) {
    ID3D11Device* keepDev = sDev;
    ID3D11DeviceContext* keepCtx = sCtx;
    sDev = dev; sCtx = ctx;
    BOOL ok = SavePng(tex, w, h, path);
    sDev = keepDev; sCtx = keepCtx;
    return ok;
}

// F12 capture (port_capture.cpp): the frame buffer and the output picture now, the back buffer at the next present.
BOOL PortGfxCapture(const char* prefix) {
    if (!sStarted || !sShown) return FALSE;
    char p[420];
    snprintf(p, sizeof(p), "%s_efb.png", prefix);
    SavePng(sEfbTex, sEfbPw, sEfbPh, p);
    snprintf(p, sizeof(p), "%s_picture.png", prefix);
    SavePng(sOutTex, sOutW, sOutH, p);
    if (sSwap) { snprintf(sCapPrefix, sizeof(sCapPrefix), "%s", prefix); sCapBack = TRUE; }
    return TRUE;
}

BOOL PortGfxSaveScreenshot(const char* path) {
    if (!sStarted || !sShown) return FALSE;
    return SavePng(sOutTex, sOutW, sOutH, path);
}

// ------------------------------------------------------------------ start
// Fast-forward with a frame target: draws only matter through the copies of the frame buffer that later
// frames use (the engine redraws and recopies everything each frame) and the picture at the target, so the
// draws of all but the last kFastDrawn frames are dropped. The GX interpreter still decodes them: state and
// copies go on. (--fast has no target and always draws.)
static const s32 kFastDrawn = 30;
static u32 sFastSkipped;
static BOOL FastDropping(void) { return PortFastActive() && !gPortOptions.fastAll && gPortOptions.fastForward > 0 && PortFrameCount() < gPortOptions.fastForward - kFastDrawn; }
static void Draw(const PortGx* g, u32 prim, const PortGxLayout* L, u32 count, const u8* data) {
    if (FastDropping()) { sFastSkipped++; return; }
    std::lock_guard<std::recursive_mutex> lk(sGfxLock);
    s64 t = Ticks();
    DrawPrims(g, prim, L, count, data);
    sFrame.renderTicks += Ticks() - t;
    sTotalDraw += Ticks() - t;
}
static void Copy(const PortGx* g, u32 control) {
    std::lock_guard<std::recursive_mutex> lk(sGfxLock);
    if (FastDropping()) {                        // nothing was drawn: a copy would only move an old picture
        if ((control & 0x4000) && FastProgress()) PresentOut(TRUE);      // but the window still gets its progress
        return;
    }
    s64 t = Ticks();
    CopyEfb(g, control);
    sFrame.renderTicks += Ticks() - t;
    sTotalCopy += Ticks() - t;
}
static const PortGxBackend kBackend = {Draw, Copy, LoadTlut};

BOOL PortGfxStart(BOOL window) {
    if (sStarted) return TRUE;
    // The picture is the largest 16:9 or 4:3 part of the output, centred (black bars beside it).
    sOutW = (u32)std::min(std::max(gPortSettings.width, 320), 8192); sOutH = (u32)std::min(std::max(gPortSettings.height, 240), 8192);
    u32 an = gPortSettings.wide ? 16 : 4, ad = gPortSettings.wide ? 9 : 3;
    sPicW = sOutW; sPicH = sOutW * ad / an;
    if (sPicH > sOutH) { sPicH = sOutH; sPicW = sOutH * an / ad; }
    sPicX = (sOutW - sPicW) / 2; sPicY = (sOutH - sPicH) / 2;
    // The frame buffer is drawn at renderScale percent of the picture (0.5x..2x), then scaled to the picture.
    f32 rs = (f32)std::min(std::max(gPortSettings.renderScale, 50), 200) / 100.0f;
    sFx = (f32)sPicW * rs / EFB_W; sFy = (f32)sPicH * rs / DISP_H;
    if (EFB_W * sFx > 8192.0f) { sFx = 8192.0f / EFB_W; sFy = sFx * ((f32)sPicH / DISP_H) / ((f32)sPicW / EFB_W); }
    if (EFB_H * sFy > 8192.0f) { sFy = 8192.0f / EFB_H; sFx = sFy * ((f32)sPicW / EFB_W) / ((f32)sPicH / DISP_H); }
    sEfbPw = (u32)lroundf(EFB_W * sFx); sEfbPh = (u32)ceilf(EFB_H * sFy);
    if (window) {
        sWndReady = CreateEventA(NULL, TRUE, FALSE, NULL);
        CloseHandle(CreateThread(NULL, 0, WindowThread, NULL, 0, NULL));
        WaitForSingleObject(sWndReady, 5000);
        if (!sWnd) window = FALSE;
        else if (gPortSettings.fullscreen) PortGfxSetFullscreen(TRUE);
    }
    static const D3D_DRIVER_TYPE kDrivers[2] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
    static const D3D_FEATURE_LEVEL kLevels[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = E_FAIL;
    for (int i = 0; i < 2 && FAILED(hr); i++) {
        if (window) {
            DXGI_SWAP_CHAIN_DESC sd = {};
            sd.BufferCount = 2; sd.BufferDesc.Width = sOutW; sd.BufferDesc.Height = sOutH; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = sWnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
            hr = D3D11CreateDeviceAndSwapChain(NULL, kDrivers[i], NULL, 0, kLevels, 2, D3D11_SDK_VERSION, &sd, &sSwap, &sDev, NULL, &sCtx);
        } else {
            hr = D3D11CreateDevice(NULL, kDrivers[i], NULL, 0, kLevels, 2, D3D11_SDK_VERSION, &sDev, NULL, &sCtx);
        }
        if (SUCCEEDED(hr)) PortLog("gfx: Direct3D 11 on %s, output %ux%u, picture %ux%u (%s), frame buffer %ux%u (render scale %d%%)%s", i == 0 ? "the graphics card" : "the software rasteriser (WARP)",
                                   sOutW, sOutH, sPicW, sPicH, gPortSettings.wide ? "16:9" : "4:3", sEfbPw, sEfbPh, gPortSettings.renderScale, window ? "" : ", no window");
    }
    if (FAILED(hr)) { PortLog("gfx: no Direct3D 11 device (0x%08lX); nothing will be drawn", hr); return FALSE; }
    if (sSwap) {
        ID3D11Texture2D* back = NULL;
        sSwap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back);
        sDev->CreateRenderTargetView(back, NULL, &sBackRtv);
        back->Release();
    }
    D3D11_TEXTURE2D_DESC d = {};
    d.Width = sOutW; d.Height = sOutH; d.MipLevels = 1; d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count = 1; d.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    sDev->CreateTexture2D(&d, NULL, &sOutTex);
    if (!sSwap) {                                           // no window: a stand-in back buffer, so that the idle pictures cost what they would
        ID3D11Texture2D* back = NULL;
        if (SUCCEEDED(sDev->CreateTexture2D(&d, NULL, &back))) { sDev->CreateRenderTargetView(back, NULL, &sBackRtv); back->Release(); }
    }
    sDev->CreateRenderTargetView(sOutTex, NULL, &sOutRtv);
    sDev->CreateShaderResourceView(sOutTex, NULL, &sOutSrv);
    d.Width = sEfbPw; d.Height = sEfbPh;
    sDev->CreateTexture2D(&d, NULL, &sEfbTex);
    sDev->CreateShaderResourceView(sEfbTex, NULL, &sEfbSrv);
    // MSAA: draw into a multisampled copy of the frame buffer (and depth) and resolve into sEfbTex at each
    // frame-buffer copy: the copies, the display and every read of the picture only ever see sEfbTex.
    sSamples = (u32)std::max(gPortSettings.msaa, 0) >= 2 ? (u32)gPortSettings.msaa : 1;
    while (sSamples > 1) {
        UINT qc = 0, qd = 0;
        sDev->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, sSamples, &qc);
        sDev->CheckMultisampleQualityLevels(DXGI_FORMAT_D24_UNORM_S8_UINT, sSamples, &qd);
        if (qc && qd) break;
        sSamples /= 2;
    }
    if (sSamples != (u32)gPortSettings.msaa && gPortSettings.msaa) PortLog("gfx: MSAA %dx is not available on this card, using %ux", gPortSettings.msaa, sSamples);
    d.SampleDesc.Count = sSamples;
    if (sSamples > 1 && SUCCEEDED(sDev->CreateTexture2D(&d, NULL, &sEfbMs))) {
        D3D11_RENDER_TARGET_VIEW_DESC rd = {}; rd.Format = d.Format; rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        sDev->CreateRenderTargetView(sEfbMs, &rd, &sEfbRtv);
    } else {
        sSamples = 1; d.SampleDesc.Count = 1;
        sDev->CreateRenderTargetView(sEfbTex, NULL, &sEfbRtv);
    }
    d.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; d.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* depth = NULL;
    sDev->CreateTexture2D(&d, NULL, &depth);
    sDev->CreateDepthStencilView(depth, NULL, &sEfbDsv);
    depth->Release();
    PortLog("gfx: frame buffer %ux%u, %ux multisampled", sEfbPw, sEfbPh, sSamples);
    f32 black[4] = {0, 0, 0, 1};
    sCtx->ClearRenderTargetView(sOutRtv, black);
    sCtx->ClearRenderTargetView(sEfbRtv, black);
    sCtx->ClearDepthStencilView(sEfbDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    ID3DBlob* bvs = Compile(kBlitSrc, "vs", "vs_4_0");
    ID3DBlob* bps = Compile(kBlitSrc, "ps", "ps_4_0");
    if (!bvs || !bps) return FALSE;
    sDev->CreateVertexShader(bvs->GetBufferPointer(), bvs->GetBufferSize(), NULL, &sBlitVs);
    sDev->CreatePixelShader(bps->GetBufferPointer(), bps->GetBufferSize(), NULL, &sBlitPs);
    D3D11_INPUT_ELEMENT_DESC el[11] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    for (UINT i = 0; i < 8; i++) el[3 + i] = {"TEXCOORD", i, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48 + i * 12, D3D11_INPUT_PER_VERTEX_DATA, 0};
    for (u32 n = 0; n <= 8; n++) {
        std::string src = VsSource(n);
        ID3DBlob* vs = Compile(src.c_str(), "main", "vs_4_0");
        if (!vs) return FALSE;
        sDev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), NULL, &sVs[n]);
        sDev->CreateInputLayout(el, 3 + n, vs->GetBufferPointer(), vs->GetBufferSize(), &sLayouts[n]);
        vs->Release();
    }
    bvs->Release(); bps->Release();

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = VB_VERTS * sizeof(Vtx); bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    sDev->CreateBuffer(&bd, NULL, &sVb);
    bd.ByteWidth = IB_IDXS * 2; bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    sDev->CreateBuffer(&bd, NULL, &sIb);
    bd = {};
    bd.ByteWidth = sizeof(PsConst); bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    sDev->CreateBuffer(&bd, NULL, &sPsConst);
    bd.ByteWidth = 32;
    sDev->CreateBuffer(&bd, NULL, &sBlitConst);
    D3D11_SAMPLER_DESC smp = {};
    smp.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    smp.AddressU = smp.AddressV = smp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; smp.MaxLOD = D3D11_FLOAT32_MAX; smp.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sDev->CreateSamplerState(&smp, &sLinearClamp);
    u32 white = 0xFFFFFFFF;
    sWhite = MakeTexture(&white, 1, 1);

    // PORT_GFX_TRACE=N (N > 1) starts the trace when N pictures have been shown; any other value at once.
    sTraceFrom = getenv("PORT_GFX_TRACE") ? (u32)atoi(getenv("PORT_GFX_TRACE")) : 0;
    sTrace = getenv("PORT_GFX_TRACE") != NULL && sTraceFrom <= 1;
    PortOverlayStart(sDev, sCtx);
    sStarted = TRUE;
    PortGxSetBackend(&kBackend);
    return TRUE;
}

const char* PortGfxStatsText(char* buf, u32 n) {
    if (sStarted) PortLog("gfx copy time: flush %.1f s, resolve %.1f s, blit %.1f s", sTotalFlush / (double)Ticks_freq(), sTRes / (double)Ticks_freq(), sTBlit / (double)Ticks_freq()); if (sStarted) PortLog("gfx new textures %u: %.1f s, rtv %.1f s", sNewTex, sTNew / (double)Ticks_freq(), sTRtv / (double)Ticks_freq()); PortLog("gfx tex/display part %.1f s, clear+present %.1f s; disp %.1f", sTMk / (double)Ticks_freq(), sTPres / (double)Ticks_freq(), sTDisp / (double)Ticks_freq(), sTPres / (double)Ticks_freq());
    if (!sStarted) { snprintf(buf, n, "no renderer (run with --window or --screenshot FILE)"); return buf; }
    snprintf(buf, n, "%u draw(s), %u triangle(s), %u picture(s) shown (+%u again while waiting for a retrace), %u copy(ies) to a texture, %u texture(s) decoded, %u shader(s), %u draw(s) skipped; backend time: draws %.1f s, copies %.1f s, %u draw(s) dropped by fast-forward",
             sGfxStats.draws, sGfxStats.tris, sGfxStats.presents, sExtraPictures, sGfxStats.texCopies, sGfxStats.textures, sGfxStats.shaders, sGfxStats.skipped,
             sTotalDraw / (double)Ticks_freq(), sTotalCopy / (double)Ticks_freq(), sFastSkipped);
    return buf;
}
#endif
