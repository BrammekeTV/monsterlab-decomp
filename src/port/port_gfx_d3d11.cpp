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
#include <intrin.h>
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
#include "port/port_stateslots.h"

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
static ID3D11VertexShader*     sVsG[9];                      // the GPU vertex path, by the number of texture coordinate attributes uploaded
static ID3D11InputLayout*      sLayoutsG[9];
static ID3D11Buffer*           sRecBuf;                      // the GPU vertex path's per-draw records (see GpuVtx)
static ID3D11ShaderResourceView* sRecSrv;
static BOOL                    sGpuVerts;                    // the GPU vertex path is on (PORT_CPU_VERTS=1 turns it off)
#define REC_U4 (1u << 20)                                    // uint4s in sRecBuf (16 MB)
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
struct OutKey { u32 bm, zm, alpha, cull, vp[6], sc0, sc1; f32 squeeze; s32 offX, offY; u32 shadow; };   // shadow: SHADOW_* (soft shadows)

static struct { u32 draws, tris, presents, texCopies, textures, shaders, skipped, gpuDraws, flushes; u64 verts, gpuVerts; } sGfxStats;
// This frame so far (reset at each display copy): time inside this backend, draws, GX vertices, copies.
static s64 sTotalDraw, sTotalCopy, sTotalFlush, sTRes, sTBlit, sTMk, sTDisp, sTPres, sTNew, sTRtv; static u32 sNewTex; static std::vector<ID3D11Texture2D*> sTexPool;     // time inside the backend over the whole run (the boot summary)
static struct { s64 renderTicks; u32 draws, verts, copies; u64 drawTsc, drawFlushTsc, copyTsc, flushTsc, decodeTsc; } sFrame;
static s64 sShaderTicks, sTextureTicks;   // this frame: making new pixel shaders and new textures (level changes)
static u64 sCopyStart;                 // time-stamp counter after a copy flushed the queued draws
static BOOL sSplit;                    // --perf-log or PORT_PERF_LOG: time the parts of the backend (time-stamp counter)
static inline u64 Tsc(void) { return sSplit ? __rdtsc() : 0; }
void PortSamplerThread(DWORD id);      // port_sampler.cpp
static inline s64 Ticks_freq(void) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f.QuadPart; }
static inline s64 Ticks(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
static u8 sTlutMem[0x80000];           // the palette half of the console's texture memory
static struct TlutStateHook { TlutStateHook() { PortStateBlob("tlut", sTlutMem, sizeof(sTlutMem)); } } sTlutStateHook;   // part of a savestate

struct Vtx { f32 pos[4]; f32 col[2][4]; f32 tc[8][3]; };
static u32  sTraceFrom;
static inline u32 VStride(u32 vfmt) { return vfmt >= 16 ? 40 + 8 * (vfmt - 16) : 48 + 12 * vfmt; }   // vertex format: CPU 0-8 coordinates, GPU 16 + attributes
static BOOL sTrace;                    // PORT_GFX_TRACE: log every draw and copy, save the frame buffer at each copy
static BOOL SavePng(ID3D11Texture2D* tex, u32 w, u32 h, const char* path);
#define VB_VERTS 65536                                      // per batch (16-bit indices)
#define VB_BYTES (32u << 20)                                // the vertex buffer, for all queued batches
#define IB_IDXS (1u << 20)

// ------------------------------------------------------------------ window
static HANDLE sWndReady;
static volatile LONG sWheel;           // mouse wheel movement the settings screen has not taken yet
f32 PortGfxTakeWheel(void) { return (f32)InterlockedExchange(&sWheel, 0) / WHEEL_DELTA; }
#define WM_PORT_TITLE (WM_APP + 1)     // the game thread posts it; the title is set here, on the window's own thread
static char sTitleText[96];
static BOOL sHandoff, sShowFull;       // a window waiting for a loaded state (PortGfxShowWindow)
static void PostTitle(const char* t) {
    if (!sWnd) return;
    snprintf(sTitleText, sizeof(sTitleText), "%s", t);
    PostMessageA(sWnd, WM_PORT_TITLE, 0, 0);
}
static LRESULT CALLBACK WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_PORT_TITLE) { SetWindowTextA(w, sTitleText); return 0; }
    if (m == WM_CLOSE) { PortStop(0, "window closed"); return 0; }
    if (m == WM_MOUSEWHEEL) { InterlockedExchangeAdd(&sWheel, (LONG)GET_WHEEL_DELTA_WPARAM(wp)); return 0; }
    // The game draws its own pointer (the hand): no Windows cursor over the picture, except while the settings screen is open or windows_cursor is set.
    if (m == WM_SETCURSOR && LOWORD(lp) == HTCLIENT) { SetCursor(gPortGuiOpen || gPortSettings.windowsCursor ? LoadCursor(NULL, IDC_ARROW) : NULL); return TRUE; }
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
    // Started by a savestate load: the window takes the old one's place and stays hidden until the state runs.
    const char* place = getenv("MONSTERLAB_STATE_RECT");
    int px = 0, py = 0, full = 0;
    sHandoff = place && sscanf(place, "%d,%d,%d", &px, &py, &full) == 3;
    sShowFull = sHandoff && full;
    sWnd = CreateWindowA(wc.lpszClassName, "Monster Lab (PC port)", WS_OVERLAPPEDWINDOW | (getenv("PORT_HIDDEN") || sHandoff ? 0 : WS_VISIBLE), sHandoff ? px : offscreen ? -32000 : CW_USEDEFAULT, sHandoff ? py : offscreen ? -32000 : CW_USEDEFAULT,
                         r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
    SetEvent(sWndReady);
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    return 0;
}

HWND PortGfxWindow(void) { return sWnd; }
static RECT sBeforeFull;
static BOOL sFull;
BOOL PortGfxIsFullscreen(void) { return sFull; }
BOOL PortGfxWindowPlace(RECT* r) {
    if (!sWnd) return FALSE;
    if (sFull) { *r = sBeforeFull; return TRUE; }
    return GetWindowRect(sWnd, r);
}
void PortGfxShowWindow(void) {
    if (!sWnd || !sHandoff) return;
    sHandoff = FALSE;
    if (!getenv("PORT_HIDDEN")) ShowWindow(sWnd, SW_SHOW);
    if (sShowFull) PortGfxSetFullscreen(TRUE);
    SetForegroundWindow(sWnd);
}

// Fullscreen is the same window without a border over its whole monitor; the picture is stretched to it.
void PortGfxSetFullscreen(BOOL on) {
    RECT& saved = sBeforeFull;
    BOOL& full = sFull;
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
static u32 sShownLines = 480;          // the height of the last frame-buffer copy to the display, in console lines
f32 PortGfxPixelsPerLine(void) { return (f32)sPicH / (f32)(sShownLines ? sShownLines : 480); }   // output pixels per console line
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
// Compiled shaders are kept on disk, in `shadercache` beside the executable (build\pc\shadercache): D3DCompile takes
// milliseconds per TEV set-up, which showed as a hitch where a level brings new ones. The file name is a hash of
// everything the byte code is made from (source, entry point, target, flags, compiler version), so a changed
// generator or compiler simply misses; old files are never read again. A file is { magic, size, hash of the byte
// code, byte code }: a damaged one is compiled again. PORT_NO_SHADER_CACHE=1 turns the cache off.
static const UINT kCompileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL1;
static const u32 kCacheMagic = 0x31435348;                  // "HSC1"
static u32 sShaderHits, sShaderCompiles;
static u64 Fnv64(const void* p, size_t n, u64 h) {
    const u8* b = (const u8*)p;
    for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 1099511628211ull;
    return h;
}
static BOOL ShaderCachePath(const char* src, const char* entry, const char* target, char* path, size_t size) {
    static char dir[MAX_PATH];
    static int on = -1;
    if (on < 0) {
        const char* off = getenv("PORT_NO_SHADER_CACHE");
        on = !(off && *off && *off != '0');
        DWORD n = GetModuleFileNameA(NULL, dir, MAX_PATH);
        char* slash = n && n < MAX_PATH ? strrchr(dir, '\\') : NULL;
        if (!slash || (size_t)(slash - dir) + 16 >= MAX_PATH) on = 0;
        else {
            strcpy_s(slash + 1, MAX_PATH - (slash + 1 - dir), "shadercache");
            CreateDirectoryA(dir, NULL);
        }
    }
    if (!on) return FALSE;
    u64 h = Fnv64(src, strlen(src) + 1, 1469598103934665603ull);
    h = Fnv64(entry, strlen(entry) + 1, h);
    h = Fnv64(target, strlen(target) + 1, h);
    const u32 how[2] = {kCompileFlags, D3D_COMPILER_VERSION};
    h = Fnv64(how, sizeof(how), h);
    snprintf(path, size, "%s\\%016llX.cso", dir, (unsigned long long)h);
    return TRUE;
}
static ID3DBlob* ShaderCacheRead(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    u32 head[4];                                            // magic, size, hash (64 bits)
    ID3DBlob* code = NULL;
    if (fread(head, 1, sizeof(head), f) == sizeof(head) && head[0] == kCacheMagic && head[1] && head[1] < (1u << 22) &&
        SUCCEEDED(D3DCreateBlob(head[1], &code))) {
        u64 want; memcpy(&want, &head[2], 8);
        if (fread(code->GetBufferPointer(), 1, head[1], f) != head[1] || Fnv64(code->GetBufferPointer(), head[1], 1469598103934665603ull) != want) {
            code->Release();
            code = NULL;
        }
    }
    fclose(f);
    return code;
}
static void ShaderCacheWrite(const char* path, ID3DBlob* code) {
    char tmp[MAX_PATH + 32];                                // written aside, then renamed: runs at the same time never read half a file
    snprintf(tmp, sizeof(tmp), "%s.%lu.tmp", path, GetCurrentProcessId());
    FILE* f = fopen(tmp, "wb");
    if (!f) return;
    const u32 size = (u32)code->GetBufferSize(), head[2] = {kCacheMagic, size};
    const u64 hash = Fnv64(code->GetBufferPointer(), size, 1469598103934665603ull);
    BOOL ok = fwrite(head, 1, sizeof(head), f) == sizeof(head) && fwrite(&hash, 1, 8, f) == 8 && fwrite(code->GetBufferPointer(), 1, size, f) == size;
    ok = fclose(f) == 0 && ok;
    if (!ok || !MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING)) DeleteFileA(tmp);
}
static ID3DBlob* Compile(const char* src, const char* entry, const char* target) {
    ID3DBlob* code = NULL; ID3DBlob* err = NULL;
    char path[MAX_PATH + 32];
    const BOOL cache = ShaderCachePath(src, entry, target, path, sizeof(path));
    if (cache && (code = ShaderCacheRead(path)) != NULL) { sShaderHits++; return code; }
    sShaderCompiles++;
    HRESULT hr = D3DCompile(src, strlen(src), NULL, NULL, NULL, entry, target, kCompileFlags, 0, &code, &err);
    if (FAILED(hr)) {
        PortLog("gfx: shader compile failed: %s", err ? (const char*)err->GetBufferPointer() : "?");
        if (err) err->Release();
        return NULL;
    }
    if (err) err->Release();
    if (cache) ShaderCacheWrite(path, code);
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

// ---- GPU vertex path. Draws hand the vertex shader the decoded attributes; it does the transform, lighting and
// texture-coordinate generation that DrawPrims does on the CPU with PORT_CPU_VERTS=1, with the same arithmetic.
// What it needs from the XF registers is in a "record" per draw in a buffer (sRecBuf, uint4s; floats as their bits),
// found through the vertex's record index:
//   0-2 position matrix rows, 3-5 normal matrix rows, 6 projection 0-3, 7 projection 4-5, light block, counts,
//   8 colour/alpha channel controls (0x100E-0x1011), 9 material/ambient registers (0x100C-D, 0x100A-B),
//   10-11 texgen words (0x1040+), 12-13 post-matrix words (0x1050+), 14 + 6t texgen t: its matrix rows, post matrix rows.
//   Counts (row 7 w): channels, texgens << 4, post matrices on << 8, matrix per vertex << 9, orthographic << 10,
//   narrowed for widescreen (x * 3/4) << 11, texgen t (0-2) with a texture matrix per vertex << (12 + t). With a position
//   matrix per vertex, row 0 x is the matrix block (xfMem 0x000-0x0FF as 64 rows, then the 32 normal matrices as 3 rows
//   each) and the vertex flags hold the matrix index in bits 16-21; with a texture matrix per vertex, row 14 + 6t x is the
//   block and the index is in the vertex: bits 20-25 (texgen 0), 26-31 (1) of the record word, flags bits 22-27 (2).
// The light block (shared while the lights do not change): per light colour, position + a0, direction + a1, a2 k0 k1 k2.
struct GpuVtx { f32 pos[3]; u32 rec; f32 nrm[3]; u32 flags; u32 c0, c1; f32 tc[8][2]; };   // flags: 1 normal, 2/4 colours, 8 << t coordinate t, 16-21 matrix index, bit 31 no position
static const char kVsGpuBody[] =
    "Buffer<uint4> R : register(t0);\n"
    "float4 F(uint i) { return asfloat(R.Load(i)); }\n"
    "float3 M34(uint b, float4 v) { return float3(dot(F(b), v), dot(F(b + 1), v), dot(F(b + 2), v)); }\n"
    "float4 U8(uint c) { return float4(c >> 24, (c >> 16) & 255, (c >> 8) & 255, c & 255) * (1.0 / 255.0); }\n"
    "float4 Lit(uint ctrl, float4 mat, float4 amb, float3 pos, float3 nrm, uint lb) {\n"
    "  if (!(ctrl & 2)) return mat;\n"
    "  float4 sum = amb; uint mask = ((ctrl >> 2) & 15) | (((ctrl >> 11) & 15) << 4), diff = (ctrl >> 7) & 3, attn = (ctrl >> 9) & 3;\n"
    "  [loop] for (uint i = 0; i < 8; i++) { if (!(mask & (1u << i))) continue;\n"
    "    uint b = lb + i * 4; float4 P = F(b + 1);\n"
    "    float3 d = P.xyz - pos; float dist2 = dot(d, d), dist = sqrt(dist2); if (dist > 0) d *= 1.0 / dist;\n"
    "    float a = 1.0;\n"
    "    if (attn == 3) { float4 D = F(b + 2), K = F(b + 3); float c = max(-dot(d, D.xyz), 0.0);\n"
    "      float num = P.w + c * (D.w + c * K.x), den = K.y + dist * K.z + dist2 * K.w; a = den != 0 ? max(num, 0.0) / den : 0.0; }\n"
    "    float nd = dot(nrm, d); float k = diff == 0 ? 1.0 : diff == 1 ? nd : max(nd, 0.0);\n"
    "    sum += F(b) * a * k; }\n"
    "  return mat * saturate(sum);\n"
    "}\n"
    "VO main(VI i) {\n"
    "  VO o; o.pos = 0; o.c0 = 0; o.c1 = 0; float3 tc[8]; [unroll] for (uint z = 0; z < 8; z++) { tc[z] = float3(0, 0, 1); o.tc[z] = 0; }\n"
    "  float2 tin[8] = { TIN };\n"
    "  float3 ipos = i.pw.xyz, inrm = i.nw.xyz; uint irec = asuint(i.pw.w) & 0xFFFFF, ifl = asuint(i.nw.w);\n"
    "  if (ifl & 0x80000000u) return o;\n"   // a vertex without a position: all zero
    "  uint rb = irec; uint4 h7 = R.Load(rb + 7); uint cnt = h7.w, pb = rb, nb = rb + 3;\n"
    "  if (cnt & 0x200) { uint mb = R.Load(rb).x, pmi = (ifl >> 16) & 63; pb = mb + pmi; nb = mb + 64 + (pmi & 31) * 3; }\n"   // a matrix per vertex
    "  float3 pos = M34(pb, float4(ipos, 1)); float3 nrm = float3(0, 0, 1);\n"
    "  if (ifl & 1) { float3 n = float3(dot(F(nb).xyz, inrm), dot(F(nb + 1).xyz, inrm), dot(F(nb + 2).xyz, inrm));\n"
    "    float len = length(n); if (len > 0) n /= len; nrm = n; }\n"
    "  float4 p6 = F(rb + 6); float2 p7 = asfloat(h7.xy);\n"
    "  if (cnt & 0x400) o.pos = float4(p6.x * pos.x + p6.y, p6.z * pos.y + p6.w, (p7.x * pos.z + p7.y) + 1.0, 1.0);\n"   // orthographic
    "  else { float w = -pos.z; o.pos = float4(p6.x * pos.x + p6.y * pos.z, p6.z * pos.y + p6.w * pos.z, (p7.x * pos.z + p7.y) + w, w); }\n"
    "  if (cnt & 0x800) o.pos.x *= 0.75;\n"   // widescreen: a 2D draw keeps its shape
    "  uint lb = h7.z, nc = cnt & 15, ng = (cnt >> 4) & 15, postOn = (cnt >> 8) & 1;\n"
    "  uint4 cw = R.Load(rb + 8), rg = R.Load(rb + 9);\n"
    "  float4 col[2]; col[0] = nc == 0 ? float4(1, 1, 1, 1) : float4(0, 0, 0, 0); col[1] = float4(0, 0, 0, 0);\n"
    "  [unroll] for (uint c = 0; c < 2; c++) { if (c >= nc) continue;\n"
    "    float4 vc = (ifl & (2u << c)) ? (c == 0 ? i.c0 : i.c1) : float4(1, 1, 1, 1);\n"
    "    uint cc = c == 0 ? cw.x : cw.y, ac = c == 0 ? cw.z : cw.w; float4 mr = U8(c == 0 ? rg.x : rg.y), ar = U8(c == 0 ? rg.z : rg.w);\n"
    "    float3 rgb = Lit(cc, (cc & 1) ? vc : mr, (cc & 0x40) ? vc : ar, pos, nrm, lb).rgb;\n"
    "    float al = Lit(ac, (ac & 1) ? vc : mr, (ac & 0x40) ? vc : ar, pos, nrm, lb).a;\n"
    "    col[c] = float4(rgb, al); }\n"
    "  o.c0 = col[0]; o.c1 = col[1];\n"
    "  uint4 g0 = R.Load(rb + 10), g1 = R.Load(rb + 11), q0 = R.Load(rb + 12), q1 = R.Load(rb + 13);\n"
    "  uint gens[8] = {g0.x, g0.y, g0.z, g0.w, g1.x, g1.y, g1.z, g1.w}, posts[8] = {q0.x, q0.y, q0.z, q0.w, q1.x, q1.y, q1.z, q1.w};\n"
    "  [loop] for (uint t = 0; t < ng && t < 8; t++) {\n"
    "    uint gen = gens[t], type = (gen >> 4) & 7, row = (gen >> 7) & 31;\n"
    "    if (type == 2 || type == 3) { tc[t] = float3(col[type - 2].xy, 1); continue; }\n"
    "    float4 src = float4(0, 0, 1, 1);\n"
    "    if (row == 0) src.xyz = ipos; else if (row == 1) { if (ifl & 1) src.xyz = inrm; }\n"
    "    else if (row >= 5 && row <= 12) { if (ifl & (8u << (row - 5))) src.xy = tin[row - 5]; }\n"
    "    uint mb = rb + 14 + t * 6, mrow = mb;\n"
    "    if (cnt & (0x1000u << t)) { uint ti = t == 0 ? asuint(i.pw.w) >> 20 : t == 1 ? asuint(i.pw.w) >> 26 : ifl >> 22; mrow = R.Load(mb).x + (ti & 63); }\n"   // a texture matrix per vertex
    "    float3 r = M34(mrow, src); if (!(gen & 2)) r.z = 1.0;\n"
    "    if (postOn) { float3 v = r; if (posts[t] & 0x100) { float l = length(v); if (l > 0) v /= l; } r = M34(mb + 3, float4(v, 1)); }\n"
    "    tc[t] = float3(r.xy, r.z != 0 ? r.z : 1.0); }\n"
    "  [unroll] for (uint u = 0; u < 8; u++) o.tc[u] = tc[u];\n"
    "  return o;\n"
    "}\n";
static std::string VsGpuSource(u32 ntc) {
    std::string in = "struct VI { float4 pw : POSITION; float4 nw : NORMAL; float4 c0 : COLOR0; float4 c1 : COLOR1;";
    std::string tin;
    for (u32 t = 0; t < 8; t++) {
        char b[64];
        if (t < ntc) { snprintf(b, sizeof(b), " float4 t%u : TEXCOORD%u;", t, t); in += b; snprintf(b, sizeof(b), "%si.t%u.xy", t ? ", " : "", t); }
        else snprintf(b, sizeof(b), "%sfloat2(0, 0)", t ? ", " : "");
        tin += b;
    }
    std::string body = kVsGpuBody;
    body.replace(body.find("TIN"), 3, tin);
    return in + " };\nstruct VO { float4 pos : SV_Position; float4 c0 : COLOR0; float4 c1 : COLOR1; float3 tc[8] : TEXCOORD0; };\n" + body;
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

static ID3D11Texture2D* sMovTex; static ID3D11RenderTargetView* sMovRtv; static ID3D11ShaderResourceView* sMovSrv;   // the movie's own 640x480 target
static ID3D11BlendState* sBlitBlend;   // Blit: the blend state to composite with (NULL: opaque)
static bool sMovieOff;                 // PORT_NO_MOVIE_TARGET=1: draw movies straight into the frame buffer (to compare)
// Draws queued by DrawPrims that share all their state, uploaded and drawn together by FlushDraws.
struct PendingState { bool movie; u32 vfmt, pcSerial; PsConst pc; OutKey out; ID3D11PixelShader* ps; ID3D11ShaderResourceView* srv[8]; ID3D11SamplerState* smp[8]; };
static PendingState sPend;
static u32 sPendCount;                                      // GX draws queued
// A growing array whose resize leaves the new elements as they are (std::vector clears them: a few MB a frame).
template <class T> struct RawBuf {
    T* p = NULL; size_t n = 0, cap = 0;
    size_t size() const { return n; }
    void clear() { n = 0; }
    T* data() { return p; }
    T& operator[](size_t i) { return p[i]; }
    void resize(size_t m) {
        if (m > cap) { size_t c = std::max(m, cap * 2); T* q = (T*)realloc(p, c * sizeof(T)); if (!q) abort(); p = q; cap = c; }
        n = m;
    }
};
// The queued vertices (position, two colours and the texture coordinates each) are written straight into the vertex
// ring, which stays mapped from the first vertex of a queue until FlushDraws draws it (VbReserve, VbClose). The mapped
// memory is write-combined: it is only written, vertex after vertex, and never read.
static u8* sVbMap;                                          // the mapped buffer while a queue is open, else NULL
static u32 sVbFirst, sVbLen, sVbRoom;                       // the open queue: where it starts, its bytes, how many it may take
static u32 sBVCount;                                        // vertices in the open batch
static RawBuf<u16> sBI;                                      // the queued indices (each batch's from 0)
static u32 sBatchVb, sBatchIdx;                             // where the open batch starts in sBV and sBI
// Batches closed but not drawn yet: FlushDraws uploads all their vertices, indices and records with one Map each
// (a Map costs more than the draws of a small batch) and then draws them in order.
struct Batch { PendingState st; u32 vb, idx, ni; };
static std::vector<Batch> sBatches;
static void FlushDraws();
static BOOL sShadowBroken;             // soft shadows could not be set up: drawn as the game does
static BOOL sShadowDirty;              // soft shadows: shadow pixels are marked in the stencil and not in the picture yet
static BOOL IsShadowPass(const PortGx* g);
static void ShadowComposite();
static void DebugMessages(const char* where);
static void Blit(ID3D11ShaderResourceView* src, ID3D11RenderTargetView* dst, u32 dstX, u32 dstY, u32 dstW, u32 dstH, f32 u0, f32 v0, f32 du, f32 dv, const f32* pick);
static void CloseBatch() {
    if (!sPendCount) return;
    sPendCount = 0;
    sBatches.push_back({sPend, sBatchVb, sBatchIdx, (u32)sBI.size() - sBatchIdx});
}

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
static const std::string& TevKey(const PortGx* g, u32* texUsed, u32 hdr) {
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
    u32 tail[13];
    for (u32 i = 0; i < 8; i++) tail[i] = g->bp[0xF6 + i] & 0xF;       // swap tables
    tail[8] = g->bp[0xF3] & 0xFF0000;                                    // alpha compare functions
    tail[9] = (g->bp[0x00] >> 16) & 7;                                   // indirect stages
    tail[10] = g->bp[0x27] & 0xFFFFFF;                                   // their texture maps and coordinates
    tail[11] = (g->bp[0xF1] >> 20) & 15;                                 // fog: projection (bit 3) and kind
    tail[12] = hdr;                                                      // improved lighting (TevSource)
    for (u32 i = 0; i < tail[9]; i++) *texUsed |= 1u << ((g->bp[0x27] >> (i * 6)) & 7);
    key.append((const char*)tail, sizeof(tail));
    return key;
}

// hdr (the improved lighting, lit 3D draws only): colour results are not clipped to 1 between the stages (only kept
// at 0..16; a register used as a lerp weight still is), and the end colour is scaled by the exposure (alphaRef.z)
// and tone-mapped: below 0.8 unchanged, above it compressed smoothly towards 1, the brightest channel deciding so
// the hue stays. Without it the code is exactly the GX arithmetic.
static std::string TevSource(const PortGx* g, u32 hdr) {
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
            std::string w = hdr && ccn < 8 ? std::string("saturate(") + kColorIn[ccn] + ")" : kColorIn[ccn];
            snprintf(b, sizeof(b), "  { float3 r = (%s %s lerp(%s, %s, %s)%s) * %s; %s.rgb = %s; }\n", kColorIn[cd], sub ? "-" : "+",
                     kColorIn[ca], kColorIn[cb], w.c_str(), kBias[bias], kScale[scale], kRegName[dest],
                     clamp ? (hdr ? "clamp(r, 0.0, 16.0)" : "saturate(r)") : (hdr ? "clamp(r, -4.0, 16.0)" : "clamp(r, -4.0, 4.0)"));
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
    if (hdr) snprintf(b, sizeof(b), "  float4 o = float4(max(%s.rgb, 0.0) * alphaRef.z, saturate(%s.a));\n"
                                    "  { float m = max(o.r, max(o.g, o.b)); if (m > 0.8) o.rgb *= (0.8 + 0.2 * (1.0 - exp(-(m - 0.8) * 5.0))) / m; }\n",
                      kRegName[lastC], kRegName[lastA]);
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
static ID3D11PixelShader* GetPixelShader(const PortGx* g, u32* texUsed, u32 hdr) {
    const std::string& key = TevKey(g, texUsed, hdr);
    static std::string lastKey; static ID3D11PixelShader* lastPs; static u32 lastTex;
    if (lastPs && key == lastKey) { *texUsed = lastTex; return lastPs; }   // the draw before had the same TEV set-up
    u64 h = 1469598103934665603ull;
    const u32* w = (const u32*)key.data();
    for (size_t i = 0; i < key.size() / 4; i++) h = (h ^ w[i]) * 1099511628211ull;
    auto it = sPsCache.find(h);
    if (it != sPsCache.end()) { lastKey = key; lastPs = it->second; lastTex = *texUsed; return lastPs; }
    ID3D11PixelShader* ps = NULL;
    const s64 t0 = Ticks();
    std::string src = TevSource(g, hdr);
    ID3DBlob* code = Compile(src.c_str(), "main", "ps_4_0");
    if (code) {
        sDev->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &ps);
        code->Release();
    } else {
        PortLog("gfx: the shader source was:\n%s", src.c_str());
    }
    sPsCache[h] = ps;
    sShaderTicks += Ticks() - t0;
    sGfxStats.shaders++;
    return ps;
}

// ------------------------------------------------------------------ textures
static inline u32 Rgba(u32 r, u32 g, u32 b, u32 a) { return r | g << 8 | b << 16 | a << 24; }
static inline u32 E5(u32 v) { return (v << 3) | (v >> 2); }
static inline u32 E6(u32 v) { return (v << 2) | (v >> 4); }
static inline u32 E4(u32 v) { return (v << 4) | v; }
static inline u32 E3(u32 v) { return (v << 5) | (v << 2) | (v >> 1); }
static inline u32 Be16(const u8* p) { u16 v; memcpy(&v, p, 2); return _byteswap_ushort(v); }
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

struct TexEntry { ID3D11ShaderResourceView* srv; u32 hash, w, h, fmt, gen, levels; u64 content; BOOL waiting, wideArt; u32 flushSeen; };   // flushSeen: PortFlushCount when it was last read   // waiting: a mod's file is still being read; wideArt: its picture is wider than the game's
static u32 sModBytes;                                      // mod pictures handed to the graphics card in this frame
static u32 sTexGen = 1;                                    // bumped each frame and on a palette load: textures are re-hashed once per generation
static std::map<u64, TexEntry> sTextures;                   // by address and palette slot
static std::map<u32, ID3D11ShaderResourceView*> sEfbCopies; // frame-buffer copies, by destination address
static std::map<u32, u32> sEfbCopyRowBytes;                 // console bytes per row of each copy, by destination address

// Texture filtering (Settings > Display): the game's own sampler modes, or everything nearest, bilinear
// (trilinear over a mip chain made here) or anisotropic.
static BOOL sFilterDirty;
static BOOL WantMips(void) { return gPortSettings.texFilter >= PORT_FILTER_BILINEAR; }
void PortGfxApplySettings(void) { sFilterDirty = TRUE; }

// The mip levels the game samples (GXInitTexObjLOD): a mip min filter (mode 0 bits 5-7 >= 2) and a
// max LOD (mode 1 bits 8-15, 4.4 fixed point) above 0. The smaller levels follow level 0 in memory,
// each padded to whole tiles. The console picks the level from the screen-space size of the texels,
// so a texture on a thin quad (the menu's 8x32 raindrop on a 1-2 pixel wide strip) is drawn from a
// small, averaged level: fainter and smeared along its length.
static u32 GameMipLevels(const PortGx* g, u32 map, u32 w, u32 h) {
    u32 mode0 = g->bp[(map < 4 ? 0x80 : 0xA0) + (map & 3)], mode1 = g->bp[(map < 4 ? 0x84 : 0xA4) + (map & 3)];
    if (((mode0 >> 5) & 7) < 2) return 1;
    u32 levels = 1 + (((mode1 >> 8) & 0xFF) + 15) / 16;
    u32 full = 1;
    for (u32 m = w > h ? w : h; m > 1; m >>= 1) full++;
    return levels < full ? levels : full;
}

// game: the levels below level 0 from the game's data (GameMipLevels), or NULL.
static ID3D11ShaderResourceView* MakeTexture(const u32* rgba, u32 w, u32 h, const std::vector<std::vector<u32>>* game = NULL) {
    D3D11_TEXTURE2D_DESC d = {};
    std::vector<std::vector<u32>> chain;            // the smaller levels (box filter)
    D3D11_SUBRESOURCE_DATA init[16] = {{rgba, w * 4, 0}};
    u32 levels = 1;
    if (game && !game->empty()) {
        for (u32 i = 0; i < game->size() && levels < 16; i++) {
            u32 lw = w >> levels ? w >> levels : 1;
            init[levels++] = {(*game)[i].data(), lw * 4, 0};
        }
    } else if (WantMips() && w > 1 && h > 1) {
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

// A mod's replacement, with the levels the file has (a PNG's smaller ones only when the filter setting or the
// game's own sampler mode uses mip levels).
static ID3D11ShaderResourceView* MakeModTexture(const PortModImage& im, BOOL mips) {
    D3D11_SUBRESOURCE_DATA init[16] = {};
    const u8* at = im.data;
    u32 levels = !im.dxgi && !mips && !WantMips() ? 1 : im.levels < 16 ? im.levels : 16;
    for (u32 i = 0; i < levels; i++) {
        u32 pitch = 0, bytes = PortModLevelBytes(im.dxgi, im.w, im.h, i, &pitch);
        init[i].pSysMem = at;
        init[i].SysMemPitch = pitch;
        at += bytes;
    }
    sModBytes += (u32)(at - im.data);
    D3D11_TEXTURE2D_DESC d = {};
    d.Width = im.w; d.Height = im.h; d.MipLevels = levels; d.ArraySize = 1;
    d.Format = im.dxgi ? (DXGI_FORMAT)im.dxgi : DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count = 1; d.Usage = D3D11_USAGE_IMMUTABLE; d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* t = NULL; ID3D11ShaderResourceView* srv = NULL;
    if (FAILED(sDev->CreateTexture2D(&d, init, &t))) {         // BC7 needs a Direct3D 11 card; or out of video memory
        static BOOL told;
        if (!told) { told = TRUE; PortLog("mods: the graphics device cannot make a %ux%u texture; the game's texture is used", im.w, im.h); }
        return NULL;
    }
    sDev->CreateShaderResourceView(t, NULL, &srv);
    t->Release();
    return srv;
}

// The replacement of a texture if its file has been read; *waiting says to ask again at a later picture. At most
// some 24 MB go to the graphics card per frame, so a screen full of large pictures does not stall one.
// *wide: the replacement is at least a fifth wider for its height than the game's w x h texture (4:3 art redrawn
// at 16:9 is a third wider).
static ID3D11ShaderResourceView* ModTexture(u64 content, const char* name, BOOL mips, u32 w, u32 h, BOOL* waiting, BOOL* wide) {
    const PortModImage* im = NULL;
    *waiting = FALSE;
    if (sModBytes >= (24u << 20)) { *waiting = TRUE; return NULL; }
    int r = PortModsTexture(content, name, &im);
    *waiting = r == PORT_MODTEX_WAIT;
    ID3D11ShaderResourceView* srv = r == PORT_MODTEX_READY ? MakeModTexture(*im, mips) : NULL;
    if (srv) *wide = (u64)im->w * h * 5 >= (u64)w * im->h * 6;
    return srv;
}

// TRUE when texture map `map` was last drawn with wide replacement art (ModTexture), for the widescreen decision,
// which comes before the textures are bound: the first picture a new replacement is in still has the old shape.
static BOOL WideArt(const PortGx* g, u32 map) {
    u32 base = (map < 4 ? 0x80 : 0xA0) + (map & 3);
    u32 img0 = g->bp[base + 8], addr = (g->bp[base + 0x14] & 0xFFFFFF) << 5, tl = g->bp[base + 0x18];
    u32 w = (img0 & 0x3FF) + 1, h = ((img0 >> 10) & 0x3FF) + 1, fmt = (img0 >> 20) & 15;
    BOOL pal = fmt == 8 || fmt == 9 || fmt == 10;
    auto it = sTextures.find((u64)addr << 16 | (pal ? (tl & 0xFFF) : 0xFFFF));
    return it != sTextures.end() && it->second.wideArt && it->second.w == w && it->second.h == h && it->second.fmt == fmt;
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
    u32 levels = GameMipLevels(g, map, w, h);
    if (it != sTextures.end() && it->second.levels != levels) it->second.hash = 0;   // decode again with the other number of levels
    if (it != sTextures.end() && it->second.gen == sTexGen && it->second.w == w && it->second.h == h && it->second.fmt == fmt && it->second.levels == levels && it->second.srv) return it->second.srv;
    u32 bytes = TextureBytes(w, h, fmt);
    const u32 flushNow = PortFlushCount();
    if (it != sTextures.end()) {      // the game flushed part of it since it was read (a picture it draws into): read it again
        for (u32 at = addr & ~((1u << PORT_FLUSH_SHIFT) - 1); at < addr + bytes; at += 1u << PORT_FLUSH_SHIFT)
            if (PortFlushStamp(at) > it->second.flushSeen) { it->second.hash = 0; break; }
        it->second.flushSeen = flushNow;
    }
    u32 hash = Fnv(data, bytes, 2166136261u ^ w ^ h << 12 ^ fmt << 24);
    if (pal) hash = Fnv(tlut, fmt == 8 ? 32 : fmt == 9 ? 512 : 0x8000, hash ^ tl);
    if (it != sTextures.end() && it->second.hash == hash) {
        TexEntry& e = it->second;
        e.gen = sTexGen; e.w = w; e.h = h; e.fmt = fmt;
        if (e.waiting) {
            ID3D11ShaderResourceView* got = ModTexture(e.content, PortModsTextureName(data), e.levels > 1, w, h, &e.waiting, &e.wideArt);
            if (got) { FlushDraws(); if (e.srv) e.srv->Release(); e.srv = got; }
        }
        return e.srv ? e.srv : sWhite;
    }
    const s64 t0 = Ticks();
    std::vector<u32> rgba(w * h);
    if (!DecodeTexture(data, w, h, fmt, tlut, (tl >> 10) & 3, rgba.data())) {
        static u32 warned;
        if (!(warned & (1u << fmt))) { warned |= 1u << fmt; PortLog("gfx: texture format %u is not decoded (drawn white)", fmt); }
        return sWhite;
    }
    if (it != sTextures.end() && it->second.srv) { FlushDraws(); it->second.srv->Release(); }
    // Mods: a PNG or DDS with this texture's content hash replaces it, at the file's own size (--dump-textures writes the originals).
    ID3D11ShaderResourceView* made = NULL;
    u64 content = 0;
    BOOL waiting = FALSE, wideArt = FALSE;
    if (PortModsTexturesWanted()) {
        content = PortModsTextureHash(data, bytes, tlut, pal ? (fmt == 8 ? 32 : fmt == 9 ? 512 : 0x8000) : 0, w, h, fmt);
        PortModsDolphinName(content, data, bytes, tlut, w, h, fmt);
        const char* tname = PortModsTextureName(data);
        PortModsDumpTexture(content, tname, rgba.data(), w, h, fmt);
        made = ModTexture(content, tname, levels > 1, w, h, &waiting, &wideArt);
    }
    if (!made) {
        std::vector<std::vector<u32>> mips;
        const u8* at = data;
        for (u32 i = 1; i < levels; i++) {
            u32 pw = w >> (i - 1) ? w >> (i - 1) : 1, ph = h >> (i - 1) ? h >> (i - 1) : 1;
            at += TextureBytes(pw, ph, fmt);
            u32 lw = w >> i ? w >> i : 1, lh = h >> i ? h >> i : 1;
            mips.emplace_back(lw * lh);
            if (!DecodeTexture(at, lw, lh, fmt, tlut, (tl >> 10) & 3, mips.back().data())) { mips.clear(); break; }
        }
        made = MakeTexture(rgba.data(), w, h, &mips);
    }
    TexEntry e = {made, hash, w, h, fmt, sTexGen, levels, content, waiting, wideArt, flushNow};
    sTextures[key] = e;
    sTextureTicks += Ticks() - t0;
    sGfxStats.textures++;
    return e.srv ? e.srv : sWhite;
}

static std::map<u64, ID3D11SamplerState*> sSamplers;   // by mode 0 bits 0-16 (wrap, filters, LOD bias) and mode 1 (LOD range)
static ID3D11SamplerState* GetSampler(const PortGx* g, u32 map) {
    u32 mode0 = g->bp[(map < 4 ? 0x80 : 0xA0) + (map & 3)], mode1 = g->bp[(map < 4 ? 0x84 : 0xA4) + (map & 3)];
    u32 mode = mode0 & 0xFF; u64 key = (mode0 & 0x1FFFF) | (u64)(mode1 & 0xFFFF) << 17;
    auto found = sSamplers.find(key);
    if (found != sSamplers.end()) return found->second;
    static const D3D11_TEXTURE_ADDRESS_MODE kWrap[4] = {D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_MIRROR,
                                                        D3D11_TEXTURE_ADDRESS_WRAP};
    D3D11_SAMPLER_DESC d = {};
    // GX min filter: 0 near, 1 linear, 2 near/mip near, 3 linear/mip near, 4 near/mip linear, 5 linear/mip linear.
    u32 gmin = (mode >> 5) & 7;
    BOOL mag = (mode >> 4) & 1, min = gmin == 1 || gmin == 3 || gmin == 5, mip = gmin == 4 || gmin == 5;
    d.Filter = (D3D11_FILTER)((min ? 0x10 : 0) | (mag ? 0x4 : 0) | (mip ? 0x1 : 0));
    u32 tf = (u32)gPortSettings.texFilter;
    if (tf == PORT_FILTER_NEAREST) d.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    else if (tf == PORT_FILTER_BILINEAR) d.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    else if (tf >= PORT_FILTER_AF2) { d.Filter = D3D11_FILTER_ANISOTROPIC; d.MaxAnisotropy = 2u << (tf - PORT_FILTER_AF2); }
    d.MaxAnisotropy = d.MaxAnisotropy ? d.MaxAnisotropy : 1;
    d.AddressU = kWrap[mode & 3]; d.AddressV = kWrap[(mode >> 2) & 3]; d.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    d.MaxLOD = D3D11_FLOAT32_MAX; d.ComparisonFunc = D3D11_COMPARISON_NEVER;
    if (tf == PORT_FILTER_GAME && gmin >= 2) {          // the game's LOD range and bias (4.4 fixed point; bias s2.5)
        d.MinLOD = (f32)(mode1 & 0xFF) / 16.0f; d.MaxLOD = (f32)((mode1 >> 8) & 0xFF) / 16.0f;
        d.MipLODBias = (f32)(s8)((mode0 >> 9) & 0xFF) / 32.0f;
    }
    ID3D11SamplerState* s = NULL;
    sDev->CreateSamplerState(&d, &s);
    sSamplers[key] = s;
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
enum { SHADOW_OFF = 0, SHADOW_COVER = 1, SHADOW_DRAW = 2 };   // OutKey::shadow: soft shadows off; a draw that clears the mark; a shadow draw
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
    u32 bkey = bm | hasAlpha << 16 | (k.shadow == SHADOW_DRAW ? 1u << 17 : 0);
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
        if (k.shadow == SHADOW_DRAW) rt.RenderTargetWriteMask = 0;   // soft shadows: only the stencil mark (ShadowComposite darkens later)
        sDev->CreateBlendState(&d, &bs);
    }
    sCtx->OMSetBlendState(bs, NULL, 0xFFFFFFFF);
    ID3D11DepthStencilState*& ds = sDepths[zm | k.shadow << 5];
    if (!ds) {
        D3D11_DEPTH_STENCIL_DESC d = {};
        d.DepthEnable = zm & 1;
        d.DepthFunc = (D3D11_COMPARISON_FUNC)(((zm >> 1) & 7) + 1);
        d.DepthWriteMask = (zm & 1) && (zm & 0x10) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        if (k.shadow) {                                     // soft shadows: stencil 1 where a shadow is the surface seen, 0 where a draw covered it since
            d.StencilEnable = TRUE; d.StencilReadMask = d.StencilWriteMask = 1;
            d.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS; d.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
            d.FrontFace.StencilFailOp = d.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
            d.BackFace = d.FrontFace;
        }
        sDev->CreateDepthStencilState(&d, &ds);
    }
    sCtx->OMSetDepthStencilState(ds, k.shadow == SHADOW_DRAW ? 1 : 0);
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
static inline f32 BeF32(const u8* p) { u32 u; memcpy(&u, p, 4); u = _byteswap_ulong(u); f32 f; memcpy(&f, &u, 4); return f; }

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

static inline u32 ColourU32(const u8* p, u32 fmt, BOOL be) {    // r, g, b, a in the bytes from low to high
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
    return r | g << 8 | b << 16 | a << 24;
}
static void Colour(const u8* p, u32 fmt, BOOL be, f32* out) {
    u32 c = ColourU32(p, fmt, be);
    const f32 k = 1.0f / 255.0f;
    out[0] = (c & 255) * k; out[1] = ((c >> 8) & 255) * k; out[2] = ((c >> 16) & 255) * k; out[3] = (c >> 24) * k;
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

// GPU vertex path: the records the queued draws use (uint4s, 4 words each), uploaded with them by FlushDraws. Indices
// are absolute positions in sRecBuf; when it is full the queue is drawn and the buffer starts over.
static std::vector<u32> sRec;
static u32  sRecUsed;                                       // uint4s in sRecBuf before those in sRec
static BOOL sRecDiscard = TRUE;                             // the next upload starts the buffer over
static u32  sRecLights = ~0u, sRecLast = ~0u, sRecLastN;    // the last light block and record (reused while unchanged)
static u32  sRecLightGen;                                   // PortGx::xfLightGen when the light block was made
static u32  sRecMtx = ~0u, sRecMtxGen;                      // the last matrix block (draws with a matrix per vertex) and PortGx::xfMtxGen then
static u32  sRecLastW[4 * (14 + 6 * 8)];
// The big dynamic buffers (records, vertices, indices) are used as rings of four segments, mapped with NO_OVERWRITE:
// starting the buffer over with DISCARD made the driver find a new 16-32 MB buffer, a frame several times as long
// every few frames. Leaving a segment marks the point in the command stream (an event query); a segment is written
// again once the GPU has passed the mark made when it was left the time before (a lap earlier: no wait).
// The wait is limited: on an AMD card shared by several running games the mark was sometimes never reported, and the
// game spun here for good (docs/playthrough.md). After RING_WAIT_MS the buffer is mapped with DISCARD instead, which
// is safe whatever the GPU still reads, and the ring starts again with new queries.
#define RING_WAIT_MS 2000
struct Ring { ID3D11Query* q[4]; BOOL pend[4]; u32 seg; BOOL discard; };   // discard: the next Map of the buffer must be DISCARD
static Ring sRecRing, sVbRing, sIbRing;
static void RingWait(Ring& r) {
    const s64 start = Ticks(), ms = Ticks_freq() / 1000;
    HRESULT hr;
    while ((hr = sCtx->GetData(r.q[r.seg], NULL, 0, 0)) == S_FALSE) {
        const s64 waited = Ticks() - start;
        if (waited >= RING_WAIT_MS * ms) break;
        if (waited < 2 * ms) YieldProcessor(); else Sleep(1);
    }
    r.pend[r.seg] = FALSE;
    if (hr == S_OK) return;
    static u32 reports;
    if (reports < 8) {
        reports++;
        PortLog("gfx: no GPU mark for the %s buffer after %d ms (query 0x%08lX, device 0x%08lX); the buffer is replaced",
                &r == &sRecRing ? "record" : &r == &sVbRing ? "vertex" : "index", (int)((Ticks() - start) / ms), hr, sDev->GetDeviceRemovedReason());
    }
    for (u32 i = 0; i < 4; i++) { if (r.q[i]) { r.q[i]->Release(); r.q[i] = NULL; } r.pend[i] = FALSE; }
    r.discard = TRUE;
}
static u32 RingNext(Ring& r) {
    if (!r.q[r.seg]) { D3D11_QUERY_DESC qd = {D3D11_QUERY_EVENT, 0}; sDev->CreateQuery(&qd, &r.q[r.seg]); }
    if (r.q[r.seg]) { sCtx->End(r.q[r.seg]); r.pend[r.seg] = TRUE; }
    r.seg = (r.seg + 1) & 3;
    if (r.pend[r.seg]) RingWait(r);
    return r.seg;
}
// Records never cross a segment: the draws that use a segment's records are all drawn before it is left, and what is
// reused (lights, matrices, the last record) starts again in each segment.
// The test is against the ring's own segment: with pos / seg alone, a record that ended exactly on a boundary let the
// next one start in the following segment without RingNext (no wait for the GPU), and at the end of the buffer
// (pos == REC_U4) it was written past the end of the mapped buffer, into the driver's memory.
static u32 sRecExactFits, sRecWraps;
static void RecReserve(u32 n) {
    const u32 seg = REC_U4 / 4, pos = sRecUsed + (u32)sRec.size() / 4;
    if (pos == (sRecRing.seg + 1) * seg) sRecExactFits++;   // the case the old test got wrong (counted for the run summary)
    if (pos >= sRecRing.seg * seg && pos + n <= (sRecRing.seg + 1) * seg) return;
    FlushDraws();
    sRecWraps++;
    sRec.clear(); sRecUsed = RingNext(sRecRing) * seg; sRecLights = sRecLast = sRecMtx = ~0u;
    if (sRecRing.discard) { sRecDiscard = TRUE; sRecRing.discard = FALSE; }
}
static u32 RecAlloc(u32 n) { u32 at = sRecUsed + (u32)sRec.size() / 4; sRec.resize(sRec.size() + (size_t)n * 4); return at; }

// The record of a GPU draw (layout above GpuVtx): its matrices and the XF registers the vertex shader reads.
// `extra`: the count bits 9-11 (matrix per vertex, orthographic, narrowed).
static u32 GpuRecord(const PortGx* g, BOOL lit, u32 numChans, u32 numGens, const f32* proj, const u32* midx, u32 extra) {
    // Nothing the record is made of changed since the last one (no XF write, same matrix indices and kind): the same record.
    static u32 kXfGen, kMidx0, kMidx1, kKind;
    const u32 kind = extra | numChans | numGens << 4 | (lit ? 1u << 31 : 0);
    if (sRecLast != ~0u && kXfGen == g->xfGen && kMidx0 == midx[0] && kMidx1 == midx[1] && kKind == kind) return sRecLast;
    kXfGen = g->xfGen; kMidx0 = midx[0]; kMidx1 = midx[1]; kKind = kind;
    const u32 n = 14 + 6 * numGens;
    RecReserve(32 + 160 + n);
    u32 mb = 0;
    if (extra & 0x7200) {                                    // the matrix block: all position/texture matrices and the normal matrices
        if (sRecMtx == ~0u || sRecMtxGen != g->xfMtxGen) {
            sRecMtxGen = g->xfMtxGen;
            sRecMtx = RecAlloc(160);
            u32* w = &sRec[sRec.size() - 640];
            memcpy(w, g->xfMem, 256 * 4);
            for (u32 m = 0; m < 32; m++) for (u32 r = 0; r < 3; r++) {
                u32* row = w + 256 + (m * 3 + r) * 4;
                memcpy(row, &g->xfMem[0x400 + m * 3 + r * 3], 12); row[3] = 0;
            }
        }
        mb = sRecMtx;
    }
    u32 lb = 0;
    if (lit) {
        if (sRecLights == ~0u || sRecLightGen != g->xfLightGen) {
            sRecLightGen = g->xfLightGen;
            sRecLights = RecAlloc(32);
            u32* w = &sRec[sRec.size() - 128];
            for (u32 i = 0; i < 8; i++) {
                const f32* l = &g->xfMem[0x600 + i * 0x10];
                u32 lc; memcpy(&lc, &l[3], 4);
                f32 q[16];
                RegColour(lc, q);
                q[4] = l[10]; q[5] = l[11]; q[6] = l[12]; q[7] = l[4];
                q[8] = l[13]; q[9] = l[14]; q[10] = l[15]; q[11] = l[5];
                q[12] = l[6]; q[13] = l[7]; q[14] = l[8]; q[15] = l[9];
                memcpy(w + i * 16, q, sizeof(q));
            }
        }
        lb = sRecLights;
    }
    u32 r[4 * (14 + 6 * 8)];
    memset(r, 0, (size_t)n * 16);
    if (extra & 0x200) r[0] = mb;
    else {
        const u32 pm = midx[0] & 63;
        memcpy(r, &g->xfMem[pm * 4], 48);
        const f32* nm = &g->xfMem[0x400 + pm % 32 * 3];
        for (u32 k = 0; k < 3; k++) memcpy(r + 12 + k * 4, nm + k * 3, 12);
    }
    memcpy(r + 24, proj, 24);
    r[30] = lb; r[31] = numChans | numGens << 4 | (g->xf[0x12] & 1) << 8 | extra;
    r[32] = g->xf[0x0E]; r[33] = g->xf[0x0F]; r[34] = g->xf[0x10]; r[35] = g->xf[0x11];
    r[36] = g->xf[0x0C]; r[37] = g->xf[0x0D]; r[38] = g->xf[0x0A]; r[39] = g->xf[0x0B];
    for (u32 t = 0; t < 8; t++) { r[40 + t] = g->xf[0x40 + t]; r[48 + t] = g->xf[0x50 + t]; }
    for (u32 t = 0; t < numGens; t++) {
        u32 mi = t < 4 ? (midx[0] >> (6 + t * 6)) & 63 : (midx[1] >> ((t - 4) * 6)) & 63;
        if (extra & (0x1000u << t)) r[(14 + 6 * t) * 4] = mb;
        else memcpy(r + (14 + 6 * t) * 4, &g->xfMem[mi * 4], 48);
        memcpy(r + (17 + 6 * t) * 4, &g->xfMem[0x500 + (g->xf[0x50 + t] & 63) * 4], 48);
    }
    if (sRecLast != ~0u && sRecLastN == n && !memcmp(sRecLastW, r, (size_t)n * 16)) return sRecLast;
    sRecLast = RecAlloc(n);
    memcpy(&sRec[sRec.size() - (size_t)n * 4], r, (size_t)n * 16);
    memcpy(sRecLastW, r, (size_t)n * 16); sRecLastN = n;
    return sRecLast;
}
static inline f32 CompScaled(const u8* p, u32 fmt, f32 scale, BOOL be) {   // Component() with the scale worked out per draw
    switch (fmt) {
    case 0: return (f32)p[0] * scale;
    case 1: return (f32)(s8)p[0] * scale;
    case 2: return (f32)(be ? Be16(p) : *(const u16*)p) * scale;
    case 3: return (f32)(s16)(be ? Be16(p) : *(const u16*)p) * scale;
    default: if (be) return BeF32(p); { f32 f; memcpy(&f, p, 4); return f; }
    }
}

// The GPU path's attribute decoders, one per format, component count and byte order (positions, normals and texture
// coordinates: N floats; colours, N = 0: the packed RGBA word). `scale` is 1 / 2^shift.
typedef void (*DecFn)(const u8* p, f32 scale, u8* out);
template <u32 FMT, bool BE> static inline f32 DecOne(const u8* p, f32 scale) {
    if (FMT == 0) return (f32)p[0] * scale;
    if (FMT == 1) return (f32)(s8)p[0] * scale;
    if (FMT == 2) return (f32)(BE ? Be16(p) : *(const u16*)p) * scale;
    if (FMT == 3) return (f32)(s16)(BE ? Be16(p) : *(const u16*)p) * scale;
    if (BE) return BeF32(p);
    f32 f; memcpy(&f, p, 4); return f;
}
template <u32 FMT, u32 N, bool BE> static void Dec(const u8* p, f32 scale, u8* out) {
    const u32 e = FMT <= 1 ? 1 : FMT <= 3 ? 2 : 4;
    f32 d[3];
    for (u32 c = 0; c < N; c++) d[c] = DecOne<FMT, BE>(p + c * e, scale);
    memcpy(out, d, N * 4);
}
template <u32 FMT, bool BE> static void DecCol(const u8* p, f32, u8* out) { u32 c = ColourU32(p, FMT, BE); memcpy(out, &c, 4); }
static DecFn DecPick(u32 fmt, u32 comps, BOOL be) {
    static const DecFn kCol[2][6] = {{DecCol<0, false>, DecCol<1, false>, DecCol<2, false>, DecCol<3, false>, DecCol<4, false>, DecCol<5, false>},
                                     {DecCol<0, true>, DecCol<1, true>, DecCol<2, true>, DecCol<3, true>, DecCol<4, true>, DecCol<5, true>}};
#define DEC_ROW(F) {{Dec<F, 1, false>, Dec<F, 2, false>, Dec<F, 3, false>}, {Dec<F, 1, true>, Dec<F, 2, true>, Dec<F, 3, true>}}
    static const DecFn kTab[5][2][3] = {DEC_ROW(0), DEC_ROW(1), DEC_ROW(2), DEC_ROW(3), DEC_ROW(4)};
#undef DEC_ROW
    if (!comps) return kCol[be ? 1 : 0][fmt > 5 ? 5 : fmt];
    return kTab[fmt > 4 ? 4 : fmt][be ? 1 : 0][comps > 3 ? 2 : comps - 1];
}
// Per draw, not per vertex: where each array attribute's data is, and the attributes the vertex format has (their
// numbers go into `active`; returns how many).
static u32 ResolveArrays(const PortGx* g, const PortGxLayout* L, const u8** arrBase, u32* arrStride, BOOL* arrBe, u32* active) {
    u32 nActive = 0;
    static u32 cArrAddr[21], cArrEpoch[21]; static const u8* cArrPtr[21]; static BOOL cArrBe[21];   // the last lookup per array (once a frame)
    for (u32 a = 9; a < 21; a++) {
        if (!L->attr[a].mode) continue;
        if (L->attr[a].mode >= 2) {
            const u32 addr = g->cp[0xA0 + (a - 9)];
            if (cArrEpoch[a] != sTexGen || cArrAddr[a] != addr) {
                cArrEpoch[a] = sTexGen; cArrAddr[a] = addr;
                cArrPtr[a] = PortGxHostPtr(addr);
                cArrBe[a] = cArrPtr[a] && PortIsSectionData(cArrPtr[a]);   // a mesh's arrays are file data; arrays the game builds are in host order
            }
            arrBase[a] = cArrPtr[a];
            if (!arrBase[a]) continue;
            arrStride[a] = g->cp[0xB0 + (a - 9)] & 0xFF;
            arrBe[a] = cArrBe[a];
        }
        active[nActive++] = a;
    }
    return nActive;
}

static void MapOverrun(const char* what, size_t at, size_t bytes, size_t size);
// Bytes of the open queue that `bytes` more would not fit: the caller then draws it first (FlushDraws).
static BOOL VbFull(size_t bytes) { return sVbMap ? sVbLen + bytes > sVbRoom : bytes > VB_BYTES / 4 - 16; }
static BOOL sVbStarted, sVbLost;                            // the buffer has been mapped once; the open queue could not be mapped
static u32  sVbUsed;                                        // bytes of sVb behind the queues drawn so far
static RawBuf<u8> sVbSpare;                                 // where a queue goes when mapping fails (its draws are lost)
// Room for `bytes` more vertex data (VbFull has been asked): opens the queue, in the next ring segment when this one
// has no room for it, and returns where to write.
static u8* VbReserve(u32 bytes) {
    if (!sVbMap) {
        u32 first = (sVbUsed + 15) & ~15u;
        if (first + bytes > (sVbRing.seg + 1) * (VB_BYTES / 4)) first = RingNext(sVbRing) * (VB_BYTES / 4);
        const D3D11_MAP how = !sVbStarted || sVbRing.discard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
        sVbRing.discard = FALSE;
        D3D11_MAPPED_SUBRESOURCE map;
        sVbFirst = first; sVbLen = 0;
        sVbRoom = (sVbRing.seg + 1) * (VB_BYTES / 4) - first;
        sVbLost = !((size_t)first + bytes <= VB_BYTES && SUCCEEDED(sCtx->Map(sVb, 0, how, 0, &map)));
        if (sVbLost) {
            MapOverrun("vertex", first, bytes, VB_BYTES);
            sVbSpare.resize(VB_BYTES / 4);
            sVbMap = sVbSpare.data() - first;
        } else { sVbMap = (u8*)map.pData; sVbStarted = TRUE; }
    }
    u8* at = sVbMap + sVbFirst + sVbLen;
    sVbLen += bytes;
    return at;
}
// Ends the open queue before it is drawn; FALSE when its vertices are lost.
static BOOL VbClose(void) {
    if (!sVbMap) return FALSE;
    if (!sVbLost) sCtx->Unmap(sVb, 0);
    sVbMap = NULL;
    sVbUsed = sVbFirst + sVbLen;
    return !sVbLost;
}

// The attributes of a GPU draw's vertex, worked out once per vertex layout (it rarely changes from draw to draw).
struct DecAttr { const u8* base; u32 arrStride, off, mode, dst, arr; f32 scale; DecFn fn, fnBe, fnLe; };
struct DecPlan {
    PortGxLayout layout;
    DecAttr ap[12];
    u32 nap, flags, pmOff;
    u8 zero[16];                                            // places in the vertex no decoder fills that must read 0
    u32 nzero;
    BOOL made, hasPos;
};
static DecPlan sPlan;
static void MakePlan(const PortGxLayout* L, const u32* active, u32 nActive) {
    DecPlan& p = sPlan;
    p.layout = *L; p.made = TRUE; p.nap = 0; p.flags = 0; p.hasPos = FALSE; p.nzero = 0;
    p.pmOff = L->attr[0].mode ? L->attr[0].offset : ~0u;
    // (`active` leaves out array attributes with no array: those are in the layout but not in the vertex)
    for (u32 ai = 0; ai < nActive; ai++) {
        const u32 a = active[ai];
        const PortGxAttr& ga = L->attr[a];
        DecAttr& q = p.ap[p.nap++];
        q.arr = a; q.off = ga.offset; q.mode = ga.mode;
        u32 shift = ga.shift, comps;
        if (a == 9) { q.dst = 0; comps = ga.comps > 3 ? 3 : ga.comps; p.hasPos = TRUE; }
        else if (a == 10) { q.dst = 16; comps = 3; shift = ga.fmt == 1 ? 6 : ga.fmt == 3 ? 14 : 0; p.flags |= 1; }
        else if (a <= 12) { q.dst = 32 + (a - 11) * 4; comps = 0; p.flags |= 2u << (a - 11); }
        else { q.dst = 40 + (a - 13) * 8; comps = ga.comps > 2 ? 2 : ga.comps; p.flags |= 8u << (a - 13); }
        q.scale = 1.0f / (f32)(1u << (shift & 31));
        q.fnBe = DecPick(ga.fmt, comps, TRUE); q.fnLe = DecPick(ga.fmt, comps, FALSE);
        // a position with two components gets z = 0, a texture coordinate with one t = 0
        const u32 want = q.dst == 0 ? 3 : q.dst >= 40 ? 2 : comps;
        for (u32 c = comps; c < want && p.nzero < 16; c++) p.zero[p.nzero++] = (u8)(q.dst + c * 4);
    }
}

// The vertices and indices of one GX draw, added to the open batch (DrawPrims has set that up or found it unchanged).
struct DrawSrc { const PortGxLayout* L; const u8* data; const u8* const* arrBase; const u32* arrStride; const BOOL* arrBe; const u32* active; u32 nActive; };
static void AppendDraw(const DrawSrc& d, u32 prim, u32 count, BOOL gpu, u32 rec, u32 tmVtx, u32 stride) {
    const PortGxLayout* L = d.L;
    const u8* data = d.data;
    const u8* const* arrBase = d.arrBase; const u32* arrStride = d.arrStride; const BOOL* arrBe = d.arrBe; const u32* active = d.active;
    const u32 nActive = d.nActive;
    u16 base = (u16)sBVCount;
    {
        const u64 tscVerts = Tsc();
        u8* o = VbReserve(count * stride);
        if (gpu) {
            // The vertex shader does the rest: the attributes go into the queue as they are given (GpuVtx).
            u32 present = 0;                                    // (the same layout may lack an array one time and not the next)
            for (u32 ai = 0; ai < nActive; ai++) present |= 1u << active[ai];
            static u32 sPresent;
            if (!sPlan.made || present != sPresent || memcmp(&sPlan.layout, L, sizeof(*L))) { MakePlan(L, active, nActive); sPresent = present; }
            DecPlan& plan = sPlan;
            const u32 nap = plan.nap;
            for (u32 k = 0; k < nap; k++) {                     // where the arrays are this time, and their byte order
                DecAttr& q = plan.ap[k];
                q.base = arrBase[q.arr]; q.arrStride = arrStride[q.arr];
                q.fn = q.mode >= 2 && !arrBe[q.arr] ? q.fnLe : q.fnBe;
            }
            // The shader reads only what the flags say is there: the rest of a vertex is not written.
            if (!plan.hasPos) {                                 // no position: nothing is drawn (as on the CPU path)
                memset(o, 0, (size_t)count * stride);
                const u32 none = 0x80000000u;
                for (u32 n = 0; n < count; n++) memcpy(o + (size_t)n * stride + 28, &none, 4);
            } else {
                const u32 pmOff = plan.pmOff, fl = plan.flags, nzero = plan.nzero;
                for (u32 n = 0; n < count; n++, data += L->size, o += stride) {
                    u32 vfl = pmOff != ~0u ? fl | (data[pmOff] & 63u) << 16 : fl, vrec = rec;
                    if (tmVtx) {
                        if (tmVtx & 1) vrec |= (data[L->attr[1].offset] & 63u) << 20;
                        if (tmVtx & 2) vrec |= (data[L->attr[2].offset] & 63u) << 26;
                        if (tmVtx & 4) vfl |= (data[L->attr[3].offset] & 63u) << 22;
                    }
                    memcpy(o + 12, &vrec, 4); memcpy(o + 28, &vfl, 4);
                    for (u32 k = 0; k < nap; k++) {
                        const DecAttr& q = plan.ap[k];
                        const u8* p = data + q.off;
                        if (q.mode >= 2) { const u32 index = q.mode == 2 ? p[0] : Be16(p); p = q.base + index * q.arrStride; }
                        q.fn(p, q.scale, o + q.dst);
                    }
                    for (u32 z = 0; z < nzero; z++) memset(o + plan.zero[z], 0, 4);
                }
            }
        } else for (u32 n = 0; n < count; n++, o += stride) memcpy(o, &sVerts[n], stride);
        sFrame.decodeTsc += Tsc() - tscVerts;
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
    sGfxStats.draws++; sGfxStats.verts += count;
    if (gpu) { sGfxStats.gpuDraws++; sGfxStats.gpuVerts += count; }
    sFrame.draws++; sFrame.verts += count;
}

// The fast path: most GX draws (about 19 in 20 in the village and in battle) are the next piece of the same mesh, with no
// register written since the draw before. Such a draw needs the same shader, constants, textures, output state and
// record as that one, which is still the open batch, so only its vertices and indices are added. Kept for 3D draws
// of the GPU vertex path only (a 2D draw is placed by its own bounds). PORT_NO_FASTPATH=1 turns it off.
static struct { BOOL on; u32 bpGen, xfGen, texGen, midx0, midx1, rec, tmVtx, stride; PortGxLayout layout; } sFast;
static u32 sFastHits;
static BOOL FastDraw(const PortGx* g, u32 prim, const PortGxLayout* L, u32 count, const u8* data) {
    if (!sFast.on || !sPendCount || g->bpGen != sFast.bpGen || g->xfGen != sFast.xfGen || sTexGen != sFast.texGen ||
        g->cp[0x30] != sFast.midx0 || g->cp[0x40] != sFast.midx1) return FALSE;
    if (!(prim == 0x80 ? count >= 4 : (prim == 0x90 || prim == 0x98 || prim == 0xA0) && count >= 3) || sBVCount + count > VB_VERTS) return FALSE;
    const u32 stride = sFast.stride;                        // (the queue has to fit the buffers: otherwise the long way flushes it)
    if (VbFull((size_t)count * stride) || sBI.size() + (size_t)count * 3 > IB_IDXS / 4) return FALSE;
    if (memcmp(L, &sFast.layout, sizeof(*L))) return FALSE;
    const u8* arrBase[21] = {}; u32 arrStride[21] = {}; BOOL arrBe[21] = {};
    u32 active[12];
    const u32 nActive = ResolveArrays(g, L, arrBase, arrStride, arrBe, active);
    const DrawSrc src = {L, data, arrBase, arrStride, arrBe, active, nActive};
    AppendDraw(src, prim, count, TRUE, sFast.rec, sFast.tmVtx, stride);
    sFastHits++;
    return TRUE;
}

static void DrawPrims(const PortGx* g, u32 prim, const PortGxLayout* L, u32 count, const u8* data) {
    if (!sStarted || count == 0) return;
    if (prim >= 0xA8) { sGfxStats.skipped++; return; }     // lines and points: not drawn yet
    if (!sTrace && FastDraw(g, prim, L, count, data)) return;
    sFast.on = FALSE;
    // What only the BP registers (TEV, textures, blending) decide is kept from the draw before while none was written
    // and the frame is the same (textures are looked at again once a frame): the pixel shader, constants, textures.
    static u32 cBpGen = ~0u, cTexGen, cHdr; static ID3D11PixelShader* cPs; static u32 cTexUsed;
    static ID3D11ShaderResourceView* cSrv[8]; static ID3D11SamplerState* cSmp[8];
    u32 numChans = g->xf[0x09] & 3, numGens = g->xf[0x3F] & 15;
    BOOL lit = FALSE;                                       // some colour or alpha channel uses lights: otherwise their colours are not needed
    for (u32 c = 0; c < numChans && c < 2; c++) lit |= ((g->xf[0x0E + c] | g->xf[0x10 + c]) & 2) != 0;
    const u32 hdr = lit && gPortSettings.lighting == PORT_LIGHTING_IMPROVED;   // improved lighting: lit 3D draws only, never the 2D layer
    const BOOL sameBp = g->bpGen == cBpGen && sTexGen == cTexGen && hdr == cHdr;
    u32 texUsed;
    ID3D11PixelShader* ps = sameBp ? cPs : GetPixelShader(g, &texUsed, hdr);
    if (sameBp) texUsed = cTexUsed;
    if (!ps) { sGfxStats.skipped++; return; }
    f32 proj[7]; memcpy(proj, &g->xf[0x20], 24);
    BOOL ortho = g->xf[0x26] & 1;
    f32 ambReg[2][4], matReg[2][4];
    for (int c = 0; c < 2; c++) { RegColour(g->xf[0x0A + c], ambReg[c]); RegColour(g->xf[0x0C + c], matReg[c]); }
    u32 midx[2] = {g->cp[0x30], g->cp[0x40]};
    f32 lightCol[8][4];
    // to a triangle list (indices into the vertices, which are uploaded once; written into the batch below)
    if (!(prim == 0x80 ? count >= 4 : (prim == 0x90 || prim == 0x98 || prim == 0xA0) && count >= 3) || count > VB_VERTS) { sGfxStats.skipped++; return; }
    // Draws are transformed and lit by the vertex shader, also those with a position matrix per vertex (skinned meshes;
    // the record then has all the matrices), and those with a texture matrix per vertex for texgens 0-2.
    BOOL gpu = sGpuVerts && numGens <= 8;
    u32 tmVtx = 0;                                          // texgens with a texture matrix per vertex
    for (u32 t = 0; t < numGens && t < 8; t++) if (L->attr[1 + t].mode) tmVtx |= 1u << t;
    if (tmVtx & ~7u) gpu = FALSE;
    u32 rec = 0, ntc = 0;
    if (gpu) {
        for (u32 t = 0; t < 8; t++) if (L->attr[13 + t].mode) ntc = t + 1;
    } else {
        if (lit) for (u32 i = 0; i < 8; i++) { u32 lc; memcpy(&lc, &g->xfMem[0x600 + i * 0x10 + 3], 4); RegColour(lc, lightCol[i]); }
        sVerts.resize(count);
    }
    const u8* arrBase[21] = {}; u32 arrStride[21] = {}; BOOL arrBe[21] = {};
    u32 active[12];
    const u32 nActive = ResolveArrays(g, L, arrBase, arrStride, arrBe, active);
    const u64 tscDecode = Tsc();                            // perf: the vertex loops of both paths count as decode
    // Where a 2D draw lands on the screen (clip x, y), for the widescreen and movie decisions below: worked out from the
    // positions alone for a GPU draw.
    f32 bnd[4] = {0, 0, 0, 0};                              // lowest x, highest x, lowest y, highest y
    const BOOL movieShape = ortho && prim == 0x80 && count == 4 && ((g->bp[0x00] >> 16) & 7) >= 3;
    const BOOL wideCheck = ortho && gPortSettings.wide && !gPortSettings.stretchUi;
    if (gpu && (movieShape || wideCheck) && L->attr[9].mode) {
        const PortGxAttr& at = L->attr[9];
        const u8* d = data;
        for (u32 n = 0; n < count; n++, d += L->size) {
            const u8* p = d + at.offset;
            BOOL be = TRUE;
            if (at.mode >= 2) { if (!arrBase[9]) break; p = arrBase[9] + (at.mode == 2 ? p[0] : Be16(p)) * arrStride[9]; be = arrBe[9]; }
            f32 v[4] = {0, 0, 0, 1};
            for (u32 k = 0; k < at.comps && k < 3; k++) v[k] = Component(p + k * at.elem, at.fmt, at.shift, be);
            const u32 pm = L->attr[0].mode ? d[L->attr[0].offset] : (midx[0] & 63);
            f32 q[3]; Mul34(&g->xfMem[(pm & 63) * 4], v, q);
            const f32 x = proj[0] * q[0] + proj[1], y = proj[2] * q[1] + proj[3];
            if (!n) { bnd[0] = bnd[1] = x; bnd[2] = bnd[3] = y; }
            else { bnd[0] = std::min(bnd[0], x); bnd[1] = std::max(bnd[1], x); bnd[2] = std::min(bnd[2], y); bnd[3] = std::max(bnd[3], y); }
        }
    }
    if (!gpu)
    for (u32 n = 0; n < count; n++, data += L->size) {
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
        Vtx& o = sVerts[n];
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

    sFrame.decodeTsc += Tsc() - tscDecode;
    // Widescreen. The game gives its 3D views a 16:9 projection, but lays out everything 2D
    // (orthographic) for 640x480 and lets the TV stretch it. Here 2D draws keep their shape and
    // stay centred instead: x is narrowed by 3/4. Layers that belong to the whole screen stay as
    // they are: those that show a copy of the frame buffer (glow, blur, the banded frame), and
    // those that reach from edge to edge without a picture on them (fades, bars, backdrops: no
    // texture, or a small one). A full-screen picture (320 texels wide or more) is narrowed, unless a
    // mod replaced it with wider art (ModTexture) and the draw covers the whole screen: that art is
    // made for 16:9 and fills it (docs/modding.md).
    // A movie frame (MoviePlay, fn_80205894): one full-screen quad through three indirect stages that undo the 8x4 tiling of the
    // planes. That only works when a pixel is a texel (the offset table and the plane are read for the same console pixel), which
    // is lost at any other size: a pixel centre that lies on the edge of a block of 8 reads a texel off, a thin line. It is drawn
    // at the console's size, 640x480, and the picture is scaled into the frame buffer.
    if (!gpu && (movieShape || wideCheck)) {
        bnd[0] = bnd[1] = sVerts[0].pos[0]; bnd[2] = bnd[3] = sVerts[0].pos[1];
        for (u32 n = 1; n < count; n++) {
            bnd[0] = std::min(bnd[0], sVerts[n].pos[0]); bnd[1] = std::max(bnd[1], sVerts[n].pos[0]);
            bnd[2] = std::min(bnd[2], sVerts[n].pos[1]); bnd[3] = std::max(bnd[3], sVerts[n].pos[1]);
        }
    }
    bool movie = movieShape && bnd[0] <= -0.98f && bnd[1] >= 0.98f && bnd[2] <= -0.98f && bnd[3] >= 0.98f && !sMovieOff;
    f32 squeeze = 1.0f;
    if (wideCheck) {
        BOOL whole = FALSE, picture = FALSE, art = FALSE;
        for (u32 m = 0; m < 8; m++) {
            if (!(texUsed & (1u << m))) continue;
            u32 base = (m < 4 ? 0x80 : 0xA0) + (m & 3);
            if (sEfbCopies.count((g->bp[base + 0x14] & 0xFFFFFF) << 5)) whole = TRUE;
            else if ((g->bp[base + 8] & 0x3FF) + 1 >= 320) { picture = TRUE; art |= WideArt(g, m); }
        }
        if (!whole && !picture && bnd[0] <= -0.98f && bnd[1] >= 0.98f) whole = TRUE;
        if (!whole && art && bnd[0] <= -0.98f && bnd[1] >= 0.98f && bnd[2] <= -0.98f && bnd[3] >= 0.98f) whole = TRUE;
        if (!whole) {
            squeeze = 0.75f;
            if (!gpu) for (u32 n = 0; n < count; n++) sVerts[n].pos[0] *= squeeze;
        }
    }
    if (gpu) rec = GpuRecord(g, lit, numChans, numGens, proj, midx, (L->attr[0].mode ? 0x200 : 0) | (ortho ? 0x400 : 0) | (squeeze != 1.0f ? 0x800 : 0) | tmVtx << 12);
    if (sTrace) {
        static const Vtx kNone = {};
        const Vtx& v = gpu ? kNone : sVerts[0];             // (a GPU draw's vertices are made by the vertex shader: logged as zero)
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
    if (sTrace && !gpu && getenv("PORT_GFX_VERTS")) {
        for (u32 i = 0; i < count && i < 6; i++)
            PortLog("gfx trace:   v%u pos %.3f %.3f %.3f %.3f col %.2f %.2f %.2f %.2f tc0 %.3f %.3f %.3f tc1 %.3f %.3f", i, sVerts[i].pos[0], sVerts[i].pos[1], sVerts[i].pos[2],
                    sVerts[i].pos[3], sVerts[i].col[0][0], sVerts[i].col[0][1], sVerts[i].col[0][2], sVerts[i].col[0][3], sVerts[i].tc[0][0], sVerts[i].tc[0][1], sVerts[i].tc[0][2],
                    sVerts[i].tc[1][0], sVerts[i].tc[1][1]);
    }
    PsConst pc;
    memset(&pc, 0, sizeof(pc));
    // The constants only change now and then: rebuild them when one of the registers they come from did.
    struct PcSrc { s16 reg[16], konst[16]; u32 bp[8 + 9 + 5 + 2 + 1 + 1]; };
    static PcSrc lastSrc; static PsConst lastPcVal; static BOOL haveLast; static u32 pcSerial;
    PcSrc src;
    if (!sameBp) {
    memcpy(src.reg, g->tevReg, sizeof(src.reg)); memcpy(src.konst, g->tevKonst, sizeof(src.konst));
        u32 n = 0;
        for (u32 m = 0; m < 8; m++) src.bp[n++] = g->bp[(m < 4 ? 0x80 : 0xA0) + (m & 3) + 8];
        for (u32 i = 0x06; i < 0x0F; i++) src.bp[n++] = g->bp[i];
        for (u32 i = 0xEE; i < 0xF3; i++) src.bp[n++] = g->bp[i];
        src.bp[n++] = g->bp[0x25]; src.bp[n++] = g->bp[0x26];
        src.bp[n++] = g->bp[0xF3];
        src.bp[n++] = hdr ? (u32)gPortSettings.exposure : 0;        // alphaRef.z: the exposure of the improved lighting
    }
    if (haveLast && (sameBp || !memcmp(&src, &lastSrc, sizeof(src)))) pc = lastPcVal;
    else {
    for (int i = 0; i < 4; i++) for (int k = 0; k < 4; k++) { pc.reg[i][k] = g->tevReg[i][k] / 255.0f; pc.konst[i][k] = g->tevKonst[i][k] / 255.0f; }
        pc.alphaRef[0] = (g->bp[0xF3] & 255) / 255.0f; pc.alphaRef[1] = ((g->bp[0xF3] >> 8) & 255) / 255.0f; pc.alphaRef[2] = hdr ? gPortSettings.exposure / 100.0f : 1.0f; pc.alphaRef[3] = 0;
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
        lastSrc = src; lastPcVal = pc; haveLast = TRUE; pcSerial++;
    }
    OutKey ok = MakeOutKey(g, squeeze);
    if (gPortSettings.shadows == PORT_SHADOWS_SOFT && !movie && !sShadowBroken) {
        ok.shadow = IsShadowPass(g) ? SHADOW_DRAW : SHADOW_COVER;
        // A draw that does not write depth (see-through effects, the 2D layer) is blended over the picture as it is: the
        // shadows drawn so far are put in first, so it is neither cut out of them nor darkened by them.
        if (ok.shadow == SHADOW_COVER && sShadowDirty && !((ok.zm & 1) && (ok.zm & 0x10))) { FlushDraws(); ShadowComposite(); }
        if (ok.shadow == SHADOW_DRAW) sShadowDirty = TRUE;
    }
    ID3D11ShaderResourceView* srv[8]; ID3D11SamplerState* smp[8];
    if (sameBp) { memcpy(srv, cSrv, sizeof(srv)); memcpy(smp, cSmp, sizeof(smp)); }
    else {
        for (u32 i = 0; i < 8; i++) { srv[i] = (texUsed & (1u << i)) ? GetTexture(g, i) : sWhite; smp[i] = (texUsed & (1u << i)) ? GetSampler(g, i) : sLinearClamp; }
        memcpy(cSrv, srv, sizeof(srv)); memcpy(cSmp, smp, sizeof(smp)); cPs = ps; cTexUsed = texUsed; cBpGen = g->bpGen; cTexGen = sTexGen; cHdr = hdr;
    }

    // Draws that need exactly the same state as the ones queued before are added to them: one upload and one draw call for the lot.
    const u32 vfmt = gpu ? 16 + ntc : (numGens > 8 ? 8 : numGens);
    const u32 stride = VStride(vfmt);
    // The queue is uploaded at once (FlushDraws); it has to fit the buffers.
    if ((size_t)count * stride > VB_BYTES / 4 - 16) { sGfxStats.skipped++; return; }   // larger than a ring segment
    if (VbFull((size_t)count * stride) || sBI.size() + (size_t)count * 3 > IB_IDXS / 4) FlushDraws();
    if (sPendCount && (sPend.ps != ps || sBVCount + count > VB_VERTS || sPend.vfmt != vfmt || sPend.movie != movie || memcmp(&sPend.out, &ok, sizeof(ok)) || memcmp(sPend.srv, srv, sizeof(srv)) ||
                       memcmp(sPend.smp, smp, sizeof(smp)) || (sPend.pcSerial != pcSerial && memcmp(&sPend.pc, &pc, sizeof(pc)))))
        CloseBatch();
    if (!sPendCount) {
        sPend.movie = movie; sPend.vfmt = vfmt; sPend.pcSerial = pcSerial; sPend.ps = ps; sPend.out = ok; memcpy(sPend.srv, srv, sizeof(srv)); memcpy(sPend.smp, smp, sizeof(smp)); sPend.pc = pc;
        sBatchVb = sVbMap ? sVbLen : 0; sBatchIdx = (u32)sBI.size(); sBVCount = 0;
    }
    const DrawSrc src2 = {L, data, arrBase, arrStride, arrBe, active, nActive};
    AppendDraw(src2, prim, count, gpu, rec, tmVtx, stride);
    static const BOOL noFast = getenv("PORT_NO_FASTPATH") != NULL;
    if (gpu && !ortho && !noFast) {
        sFast.on = TRUE; sFast.bpGen = g->bpGen; sFast.xfGen = g->xfGen; sFast.texGen = sTexGen; sFast.midx0 = midx[0]; sFast.midx1 = midx[1];
        sFast.rec = rec; sFast.tmVtx = tmVtx; sFast.stride = stride; sFast.layout = *L;
    }
    if (movie) FlushDraws();                                // drawn at once: it has its own target
}

// A write into a mapped buffer that would not fit: skipped (the draws of that flush are lost) and logged, never written.
static void MapOverrun(const char* what, size_t at, size_t bytes, size_t size) {
    static u32 reports;
    if (reports++ < 8) PortLog("gfx: BUG: %s buffer write of %zu bytes at %zu would pass its end (%zu); skipped", what, bytes, at, size);
}

// Uploads and draws what DrawPrims has queued.
static void FlushQueued();
static void FlushDraws() {
    CloseBatch();
    if (sBatches.empty()) return;
    const u64 tsc = Tsc();
    FlushQueued();
    sFrame.flushTsc += Tsc() - tsc;
}
static void FlushQueued() {
    sGfxStats.flushes += (u32)sBatches.size();
    if (!sRec.empty()) {                                    // the GPU path's records, behind those already in the buffer
        D3D11_MAPPED_SUBRESOURCE rm;
        if ((size_t)sRecUsed * 16 + sRec.size() * 4 > (size_t)REC_U4 * 16) {   // never past the buffer (MapOverrun logs it)
            MapOverrun("record", (size_t)sRecUsed * 16, sRec.size() * 4, (size_t)REC_U4 * 16);
        } else if (SUCCEEDED(sCtx->Map(sRecBuf, 0, sRecDiscard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE, 0, &rm))) {
            memcpy((u8*)rm.pData + (size_t)sRecUsed * 16, sRec.data(), sRec.size() * 4);
            sCtx->Unmap(sRecBuf, 0);
        }
        sRecUsed += (u32)sRec.size() / 4; sRec.clear(); sRecDiscard = FALSE;
    }
    // The vertices are in the ring already (VbReserve). Indices go behind those of the earlier flushes; the buffers are
    // only replaced when they are full. Replacing them for every draw made a scene of a few thousand small draws crawl.
    static u32 sIbUsed;                                     // in indices
    static BOOL started;
    const u32 ni = (u32)sBI.size();
    u32 ifirst = sIbUsed;
    D3D11_MAPPED_SUBRESOURCE map;
    D3D11_MAP how;
    const u32 first = sVbFirst;
    BOOL ok = VbClose();
    if (ifirst + ni > (sIbRing.seg + 1) * (IB_IDXS / 4)) ifirst = RingNext(sIbRing) * (IB_IDXS / 4);
    how = !started || sIbRing.discard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    sIbRing.discard = FALSE;
    if (((size_t)ifirst + ni) * 2 > (size_t)IB_IDXS * 2) { MapOverrun("index", (size_t)ifirst * 2, (size_t)ni * 2, (size_t)IB_IDXS * 2); ok = FALSE; }
    else if (SUCCEEDED(sCtx->Map(sIb, 0, how, 0, &map))) { memcpy((u16*)map.pData + ifirst, sBI.data(), ni * sizeof(u16)); sCtx->Unmap(sIb, 0); } else ok = FALSE;
    started = TRUE;
    sIbUsed = ifirst + ni;
    sBI.clear(); sBVCount = 0;

    static PsConst lastPc; static OutKey lastOut;
    static ID3D11ShaderResourceView* lastSrv[8]; static ID3D11SamplerState* lastSmp[8]; static ID3D11PixelShader* lastPs;
    static u32 lastVfmt = ~0u, lastPcSerial;
    for (size_t bi = 0; ok && bi < sBatches.size(); bi++) {
        const Batch& b = sBatches[bi];
        const PendingState& st = b.st;
        if (!sDrawBound || (lastPcSerial != st.pcSerial && memcmp(&st.pc, &lastPc, sizeof(lastPc)))) { sCtx->UpdateSubresource(sPsConst, 0, NULL, &st.pc, 0, 0); lastPc = st.pc; }
        lastPcSerial = st.pcSerial;
        if (!sDrawBound) sCtx->OMSetRenderTargets(1, &sEfbRtv, sEfbDsv);
        if (!sDrawBound || memcmp(&st.out, &lastOut, sizeof(lastOut))) { SetOutputState(st.out); lastOut = st.out; }
        if (!sDrawBound || memcmp(st.srv, lastSrv, sizeof(lastSrv))) { sCtx->PSSetShaderResources(0, 8, st.srv); memcpy(lastSrv, st.srv, sizeof(lastSrv)); }
        if (!sDrawBound || memcmp(st.smp, lastSmp, sizeof(lastSmp))) { sCtx->PSSetSamplers(0, 8, st.smp); memcpy(lastSmp, st.smp, sizeof(lastSmp)); }
        UINT vbStride = VStride(st.vfmt), vbOffset = first + b.vb;
        sCtx->IASetVertexBuffers(0, 1, &sVb, &vbStride, &vbOffset);
        if (!sDrawBound || lastVfmt != st.vfmt) {
            const BOOL gv = st.vfmt >= 16;
            sCtx->IASetInputLayout(gv ? sLayoutsG[st.vfmt - 16] : sLayouts[st.vfmt]);
            sCtx->VSSetShader(gv ? sVsG[st.vfmt - 16] : sVs[st.vfmt], NULL, 0);
            if (gv) sCtx->VSSetShaderResources(0, 1, &sRecSrv);
            lastVfmt = st.vfmt;
        }
        if (!sDrawBound) {
            sCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            sCtx->PSSetConstantBuffers(0, 1, &sPsConst);
            sCtx->IASetIndexBuffer(sIb, DXGI_FORMAT_R16_UINT, 0);
        }
        if (!sDrawBound || st.ps != lastPs) { sCtx->PSSetShader(st.ps, NULL, 0); lastPs = st.ps; }
        sDrawBound = TRUE;
        if (st.movie) {
            if (!sMovTex) {
                D3D11_TEXTURE2D_DESC d = {};
                d.Width = 640; d.Height = 480; d.MipLevels = 1; d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                d.SampleDesc.Count = 1; d.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
                if (SUCCEEDED(sDev->CreateTexture2D(&d, NULL, &sMovTex))) { sDev->CreateRenderTargetView(sMovTex, NULL, &sMovRtv); sDev->CreateShaderResourceView(sMovTex, NULL, &sMovSrv); }
            }
            if (sMovRtv && sMovSrv) {
                UINT nv = 1; D3D11_VIEWPORT ov = {};
                sCtx->RSGetViewports(&nv, &ov);             // the draw's own, as SetOutputState left it
                ID3D11BlendState* keep = NULL; FLOAT bf[4]; UINT sm;
                sCtx->OMGetBlendState(&keep, bf, &sm);
                f32 sq = st.out.squeeze;
                sCtx->OMSetRenderTargets(1, &sMovRtv, NULL);
                D3D11_VIEWPORT v = {-(640.0f / sq - 640.0f) / 2.0f, 0, 640.0f / sq, 480.0f, 0, 1};   // a narrowed quad fills the 640 again
                D3D11_RECT sc = {0, 0, 640, 480};
                sCtx->RSSetViewports(1, &v);
                sCtx->RSSetScissorRects(1, &sc);
                sCtx->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);  // the target keeps the colour and alpha the TEV made
                sCtx->DrawIndexed(b.ni, ifirst + b.idx, 0);
                static const f32 kKeepAlpha[4] = {0, 0, 0, -1};
                sBlitBlend = keep;
                Blit(sMovSrv, sEfbRtv, (u32)lroundf(ov.TopLeftX + ov.Width * (1.0f - sq) / 2.0f), (u32)lroundf(ov.TopLeftY),
                     (u32)lroundf(ov.Width * sq), (u32)lroundf(ov.Height), 0, 0, 1, 1, kKeepAlpha);   // (Blit leaves sDrawBound FALSE: the next batch binds everything again)
                sBlitBlend = NULL;
                if (keep) keep->Release();
                continue;
            }
        }
        sCtx->DrawIndexed(b.ni, ifirst + b.idx, 0);
    }
    sBatches.clear();
}

// ------------------------------------------------------------------ soft shadows
// The game draws its shadows as a second, flattened copy of each model whose TEV gets one more stage that outputs opaque
// black (fn_8003EA7C appends it: colour a, b, c, d all zero, alpha d = konst 1.0; fn_8003F1B8 battle, fn_8003ED08 field).
// With soft shadows such a draw writes only depth (as on the console) and stencil bit 0 = 1; every other draw sets it
// back to 0 where it draws. ShadowComposite then turns the marked pixels into a mask, blurs it (a Gaussian, sigma 2.5
// console pixels) and darkens the picture by it: shadow darkness * mask, so overlapping shadows darken only once.
static BOOL IsShadowPass(const PortGx* g) {
    u32 n = (g->bp[0x00] >> 10) & 15;                      // the last stage
    if (n == 0 || !(g->bp[0x41] & 1)) return FALSE;         // an added stage, drawn with blending
    u32 cc = g->bp[0xC0 + n * 2], ac = g->bp[0xC1 + n * 2];
    u32 ksel = g->bp[0xF6 + n / 2] >> ((n & 1) ? 14 : 4) & 0x3FF;
    return (cc & 0xFFFF) == 0xFFFF && ((cc >> 16) & 0xF7) == 0 &&                                      // colour: zero, no bias/subtract/scale, dest prev
           ((ac >> 4) & 0xFFF) == ((7u << 9) | (7u << 6) | (7u << 3) | 6u) && ((ac >> 16) & 0xF7) == 0 && // alpha: konst
           ((ksel >> 5) & 31) == 0;                                                                     // konst alpha 1.0
}
static ID3D11Texture2D* sShTex[3]; static ID3D11RenderTargetView* sShRtv[3]; static ID3D11ShaderResourceView* sShSrv[3];   // marked (multisampled, MSAA only), mask, blurred across
static ID3D11PixelShader *sShFillPs, *sShBlurPs; static ID3D11DepthStencilState* sShMarked; static ID3D11BlendState* sShMul;
static u32 sShW, sShH;
static const char kShadowSrc[] =
    "cbuffer C : register(b0) { float4 rect; float4 pick; };\n"   // pick: step (uv), darkness, 1 = darken the picture
    "Texture2D tex : register(t0); SamplerState smp : register(s0);\n"
    "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
    "float4 fill(VO i) : SV_Target { return float4(1, 1, 1, 1); }\n"
    "float4 blur(VO i) : SV_Target { float v = 0, ws = 0;\n"
    "  [unroll] for (int k = -6; k <= 6; k++) { float w = exp(-k * k / 8.0); v += w * tex.SampleLevel(smp, i.uv + pick.xy * k, 0).r; ws += w; }\n"
    "  v /= ws; return pick.w > 0 ? float4((1.0 - pick.z * v).xxx, 1) : float4(v, v, v, v); }\n";
static void ShadowPass(ID3D11PixelShader* ps, ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv, ID3D11DepthStencilState* dss,
                       ID3D11BlendState* bs, ID3D11ShaderResourceView* src, const f32* pick) {
    f32 c[8] = {0, 0, 1, 1, pick[0], pick[1], pick[2], pick[3]};
    sCtx->UpdateSubresource(sBlitConst, 0, NULL, c, 0, 0);
    sCtx->OMSetRenderTargets(1, &rtv, dsv);
    D3D11_VIEWPORT v = {0, 0, (f32)sEfbPw, (f32)sEfbPh, 0, 1};
    D3D11_RECT sc = {0, 0, (LONG)sEfbPw, (LONG)sEfbPh};
    sCtx->RSSetViewports(1, &v); sCtx->RSSetScissorRects(1, &sc);
    sCtx->OMSetBlendState(bs, NULL, 0xFFFFFFFF);
    sCtx->OMSetDepthStencilState(dss, 1);
    sCtx->RSSetState(NULL);
    sCtx->IASetInputLayout(NULL);
    sCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    sCtx->VSSetShader(sBlitVs, NULL, 0); sCtx->PSSetShader(ps, NULL, 0);
    sCtx->VSSetConstantBuffers(0, 1, &sBlitConst); sCtx->PSSetConstantBuffers(0, 1, &sBlitConst);
    sCtx->PSSetShaderResources(0, 1, &src); sCtx->PSSetSamplers(0, 1, &sLinearClamp);
    sCtx->Draw(3, 0);
    ID3D11ShaderResourceView* none = NULL;
    sCtx->PSSetShaderResources(0, 1, &none);
    sCtx->OMSetRenderTargets(0, NULL, NULL);
}
static u32 sShadowComposites;                           // for the summary at the end of a run
static void ShadowComposite() {
    sShadowDirty = FALSE;
    sShadowComposites++;
    sDrawBound = FALSE;
    if (!sShFillPs) {
        ID3DBlob* a = Compile(kShadowSrc, "fill", "ps_4_0"); ID3DBlob* b = Compile(kShadowSrc, "blur", "ps_4_0");
        if (a) { sDev->CreatePixelShader(a->GetBufferPointer(), a->GetBufferSize(), NULL, &sShFillPs); a->Release(); }
        if (b) { sDev->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), NULL, &sShBlurPs); b->Release(); }
        // Every field valid, also those of the disabled depth test: a zero DepthFunc is not a comparison (WARP takes it,
        // a hardware driver need not).
        D3D11_DEPTH_STENCIL_DESC d = {};
        d.DepthEnable = FALSE; d.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; d.DepthFunc = D3D11_COMPARISON_ALWAYS;
        d.StencilEnable = TRUE; d.StencilReadMask = 1; d.StencilWriteMask = 0;
        d.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        d.FrontFace.StencilPassOp = d.FrontFace.StencilFailOp = d.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        d.BackFace = d.FrontFace;
        sDev->CreateDepthStencilState(&d, &sShMarked);
        D3D11_BLEND_DESC bd = {};
        D3D11_RENDER_TARGET_BLEND_DESC& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE; rt.SrcBlend = D3D11_BLEND_ZERO; rt.DestBlend = D3D11_BLEND_SRC_COLOR; rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ZERO; rt.DestBlendAlpha = D3D11_BLEND_ONE; rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = 7;                       // the colour only: frame-buffer alpha stays the game's
        sDev->CreateBlendState(&bd, &sShMul);
    }
    if (sShW != sEfbPw || sShH != sEfbPh) {
        for (int i = 0; i < 3; i++) {
            if (sShSrv[i]) sShSrv[i]->Release();
            if (sShRtv[i]) sShRtv[i]->Release();
            if (sShTex[i]) sShTex[i]->Release();
            sShSrv[i] = NULL; sShRtv[i] = NULL; sShTex[i] = NULL;
            if (i == 0 && sSamples == 1) continue;          // without MSAA the marks go straight into [1]
            D3D11_TEXTURE2D_DESC d = {};
            d.Width = sEfbPw; d.Height = sEfbPh; d.MipLevels = d.ArraySize = 1; d.Format = DXGI_FORMAT_R8_UNORM;
            d.SampleDesc.Count = i == 0 ? sSamples : 1; d.Usage = D3D11_USAGE_DEFAULT;
            d.BindFlags = D3D11_BIND_RENDER_TARGET | (d.SampleDesc.Count == 1 ? D3D11_BIND_SHADER_RESOURCE : 0);
            sDev->CreateTexture2D(&d, NULL, &sShTex[i]);
            if (sShTex[i]) sDev->CreateRenderTargetView(sShTex[i], NULL, &sShRtv[i]);
            if (sShTex[i] && d.SampleDesc.Count == 1) sDev->CreateShaderResourceView(sShTex[i], NULL, &sShSrv[i]);
        }
        sShW = sEfbPw; sShH = sEfbPh;
    }
    // Anything missing (with MSAA the marks need a target of the frame buffer's sample count, like the depth buffer they
    // are tested against): soft shadows are switched off for this run and the shadow passes are drawn as the game does.
    if (!sShFillPs || !sShBlurPs || !sShMarked || !sShMul || !sShRtv[1] || !sShSrv[1] || !sShRtv[2] || !sShSrv[2] || (sSamples > 1 && !sShRtv[0])) {
        if (!sShadowBroken) PortLog("gfx: soft shadows are not available (a resource could not be made); the game's own shadows are drawn");
        sShadowBroken = TRUE;
        sCtx->ClearDepthStencilView(sEfbDsv, D3D11_CLEAR_STENCIL, 1.0f, 0);
        return;
    }
    const f32 zero[4] = {0, 0, 0, 0};
    ID3D11RenderTargetView* marked = sShRtv[0] ? sShRtv[0] : sShRtv[1];
    sCtx->ClearRenderTargetView(marked, zero);
    ShadowPass(sShFillPs, marked, sEfbDsv, sShMarked, NULL, NULL, zero);
    if (marked == sShRtv[0]) sCtx->ResolveSubresource(sShTex[1], 0, sShTex[0], 0, DXGI_FORMAT_R8_UNORM);
    const f32 sigma = 2.5f * (f32)sEfbPw / 640.0f, step = sigma / 2.0f;   // 13 taps over +-3 sigma
    const f32 h[4] = {step / sEfbPw, 0, 0, 0};
    ShadowPass(sShBlurPs, sShRtv[2], NULL, NULL, NULL, sShSrv[1], h);
    const f32 v[4] = {0, step / sEfbPh, gPortSettings.shadowDark / 100.0f, 1};
    ShadowPass(sShBlurPs, sEfbRtv, NULL, NULL, sShMul, sShSrv[2], v);
    sCtx->ClearDepthStencilView(sEfbDsv, D3D11_CLEAR_STENCIL, 1.0f, 0);
    DebugMessages("soft shadows");
}

// ------------------------------------------------------------------ frame-buffer copies
static void Blit(ID3D11ShaderResourceView* src, ID3D11RenderTargetView* dst, u32 dstX, u32 dstY, u32 dstW, u32 dstH, f32 u0, f32 v0, f32 du, f32 dv, const f32* pick) {
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
    sCtx->OMSetBlendState(sBlitBlend, NULL, 0xFFFFFFFF);
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
    static u64 lastTsc;
    const u64 tsc = Tsc();
    if (last && lastTsc && tsc > lastTsc && s->frameMs > 0.0f) {
        const f64 perTick = s->frameMs / (f64)(tsc - lastTsc);   // ms per time-stamp tick, from this frame itself
        const u64 other = sFrame.decodeTsc + sFrame.drawFlushTsc;
        s->decodeMs = (f32)(sFrame.decodeTsc * perTick);
        s->setupMs = (f32)((sFrame.drawTsc > other ? sFrame.drawTsc - other : 0) * perTick);
        s->flushMs = (f32)(sFrame.flushTsc * perTick);
        s->copyMs = (f32)(sFrame.copyTsc * perTick);
    }
    lastTsc = tsc;
    s->shaderMs = (f32)(sShaderTicks * ms);
    s->textureMs = (f32)(sTextureTicks * ms);
    sShaderTicks = sTextureTicks = 0;
    last = now; lastWait = wait;
    sTexGen++;
    sModBytes = 0;
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
    memset(&sFrame, 0, sizeof(sFrame));
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
// --d3d-debug: the debug layer's new messages into the log (each different text only its first 20 times).
static ID3D11InfoQueue* sInfoQ;
static void DebugMessages(const char* where) {
    if (!sInfoQ) return;
    static std::map<std::string, u32> seen;
    UINT64 n = sInfoQ->GetNumStoredMessages();
    for (UINT64 i = 0; i < n; i++) {
        SIZE_T len = 0;
        if (FAILED(sInfoQ->GetMessage(i, NULL, &len)) || !len) continue;
        std::vector<u8> buf; buf.resize(len);
        D3D11_MESSAGE* m = (D3D11_MESSAGE*)buf.data();
        if (FAILED(sInfoQ->GetMessage(i, m, &len)) || m->Severity > D3D11_MESSAGE_SEVERITY_WARNING) continue;
        std::string text(m->pDescription, m->DescriptionByteLength ? m->DescriptionByteLength - 1 : 0);
        if (++seen[text] <= 20) PortLog("d3d11 %s (%s): %s", m->Severity <= D3D11_MESSAGE_SEVERITY_ERROR ? "ERROR" : "warning", where, text.c_str());
    }
    sInfoQ->ClearStoredMessages();
}

static void PresentOut(BOOL fast) {
    DebugMessages("frame");
    sShownCount++;
    Blit(sOutSrv, sBackRtv, 0, 0, sOutW, sOutH, 0, 0, 1, 1, NULL);
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
        FlushDraws();                                   // (nothing may draw while the vertex ring is mapped)
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
    sCopyStart = Tsc();
    DebugMessages("draws");
    if (sShadowDirty) ShadowComposite();                    // soft shadows: into the picture before it is copied
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
        for (auto& sm : sSamplers) if (sm.second) sm.second->Release();
        sSamplers.clear();
        sDrawBound = FALSE;
    }
    s64 td0 = Ticks();
    if (control & 0x4000) {                                 // to the display: the picture's part of the output
        f32 black[4] = {0, 0, 0, 1};
        if (sPicW != sOutW || sPicH != sOutH) sCtx->ClearRenderTargetView(sOutRtv, black);
        Blit(sEfbSrv, sOutRtv, sPicX, sPicY, sPicW, sPicH, su0, sv0, sdu, sdv, NULL);
        sShownLines = h;
        sShown = TRUE;
        sDrawThread = GetCurrentThreadId();
        PortSamplerThread(sDrawThread);
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
    std::lock_guard<std::recursive_mutex> lk(sGfxLock);   // also called mid-run (--shot-at) while the render thread draws
    return SavePng(sOutTex, sOutW, sOutH, path);
}

// The shown picture without the overlay, scaled down (a savestate slot's picture).
BOOL PortGfxThumbnail(u32 w, u32 h, u8* rgba) {
    if (!sStarted || !sShown) return FALSE;
    std::lock_guard<std::recursive_mutex> lk(sGfxLock);
    D3D11_TEXTURE2D_DESC d;
    sOutTex->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING; d.BindFlags = 0; d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* st = NULL;
    if (FAILED(sDev->CreateTexture2D(&d, NULL, &st))) return FALSE;
    sCtx->CopyResource(st, sOutTex);
    D3D11_MAPPED_SUBRESOURCE map;
    if (FAILED(sCtx->Map(st, 0, D3D11_MAP_READ, 0, &map))) { st->Release(); return FALSE; }
    for (u32 y = 0; y < h; y++) {
        const u8* s = (const u8*)map.pData + (size_t)(sPicY + (y * 2 + 1) * sPicH / (h * 2)) * map.RowPitch;
        for (u32 x = 0; x < w; x++) {
            const u8* p = s + (sPicX + (x * 2 + 1) * sPicW / (w * 2)) * 4;
            u8* o = rgba + (y * w + x) * 4;
            o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = 255;
        }
    }
    sCtx->Unmap(st, 0);
    st->Release();
    return TRUE;
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
    const u64 tsc = Tsc(), flushed = sFrame.flushTsc;
    DrawPrims(g, prim, L, count, data);
    sFrame.drawTsc += Tsc() - tsc;
    sFrame.drawFlushTsc += sFrame.flushTsc - flushed;
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
    sCopyStart = 0;
    CopyEfb(g, control);                        // (a display copy ends the frame: its own time goes into the next one)
    if (sCopyStart) sFrame.copyTsc += Tsc() - sCopyStart;
    sFrame.renderTicks += Ticks() - t;
    sTotalCopy += Ticks() - t;
}
static const PortGxBackend kBackend = {Draw, Copy, LoadTlut};

BOOL PortGfxStart(BOOL window) {
    if (sStarted) return TRUE;
    sMovieOff = getenv("PORT_NO_MOVIE_TARGET") != NULL;
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
        else if (gPortSettings.fullscreen && !sHandoff) PortGfxSetFullscreen(TRUE);
    }
    static const D3D_DRIVER_TYPE kDrivers[2] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
    static const D3D_FEATURE_LEVEL kLevels[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = E_FAIL;
    // Without a window the game draws with the software rasteriser (WARP) unless --gpu, PORT_GPU=1, --bench or
    // --perf-log asks for the graphics card: many headless test runs at once next to a windowed game could make the
    // display driver time out and reset.
    const char* envGpu = getenv("PORT_GPU");
    BOOL useGpu = window || gPortOptions.gpu || (envGpu && *envGpu && *envGpu != '0') || gPortOptions.bench != 0 || gPortOptions.perfLog != NULL;
    UINT flags = gPortOptions.d3dDebug ? D3D11_CREATE_DEVICE_DEBUG : 0;
    for (int i = useGpu ? 0 : 1; i < 2 && FAILED(hr); i++) {
        if (window) {
            DXGI_SWAP_CHAIN_DESC sd = {};
            sd.BufferCount = 2; sd.BufferDesc.Width = sOutW; sd.BufferDesc.Height = sOutH; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = sWnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
            hr = D3D11CreateDeviceAndSwapChain(NULL, kDrivers[i], NULL, flags, kLevels, 2, D3D11_SDK_VERSION, &sd, &sSwap, &sDev, NULL, &sCtx);
        } else {
            hr = D3D11CreateDevice(NULL, kDrivers[i], NULL, flags, kLevels, 2, D3D11_SDK_VERSION, &sDev, NULL, &sCtx);
        }
        if (FAILED(hr) && flags) {                        // no debug layer installed (Windows optional feature "Graphics Tools")
            PortLog("gfx: --d3d-debug: the Direct3D debug layer is not available (0x%08lX); going on without it", hr);
            flags = 0; i--; continue;
        }
        if (SUCCEEDED(hr)) PortLog("gfx: Direct3D 11 on %s, output %ux%u, picture %ux%u (%s), frame buffer %ux%u (render scale %d%%)%s", i == 0 ? "the graphics card" : "the software rasteriser (WARP)",
                                   sOutW, sOutH, sPicW, sPicH, gPortSettings.wide ? "16:9" : "4:3", sEfbPw, sEfbPh, gPortSettings.renderScale, window ? "" : ", no window");
    }
    if (FAILED(hr)) { PortLog("gfx: no Direct3D 11 device (0x%08lX); nothing will be drawn", hr); return FALSE; }
    if (flags) sDev->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&sInfoQ);
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
    // The GPU vertex path: needs records the shader reads that can be appended to while earlier draws still use them.
    {
        D3D11_FEATURE_DATA_D3D11_OPTIONS opt = {};
        BOOL ok = SUCCEEDED(sDev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &opt, sizeof(opt))) && opt.MapNoOverwriteOnDynamicBufferSRV && !getenv("PORT_CPU_VERTS");
        D3D11_INPUT_ELEMENT_DESC eg[12] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},      // w: the record index (bits)
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},        // w: the flags (bits)
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        for (UINT i = 0; i < 8; i++) eg[4 + i] = {"TEXCOORD", i, DXGI_FORMAT_R32G32_FLOAT, 0, 40 + i * 8, D3D11_INPUT_PER_VERTEX_DATA, 0};
        for (u32 n = 0; ok && n <= 8; n++) {
            std::string src = VsGpuSource(n);
            ID3DBlob* vs = Compile(src.c_str(), "main", "vs_4_0");
            if (!vs || FAILED(sDev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), NULL, &sVsG[n])) ||
                FAILED(sDev->CreateInputLayout(eg, 4 + n, vs->GetBufferPointer(), vs->GetBufferSize(), &sLayoutsG[n]))) ok = FALSE;
            if (vs) vs->Release();
        }
        if (ok) {
            D3D11_BUFFER_DESC rd = {};
            rd.ByteWidth = REC_U4 * 16; rd.Usage = D3D11_USAGE_DYNAMIC; rd.BindFlags = D3D11_BIND_SHADER_RESOURCE; rd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            D3D11_SHADER_RESOURCE_VIEW_DESC sv = {};
            sv.Format = DXGI_FORMAT_R32G32B32A32_UINT; sv.ViewDimension = D3D11_SRV_DIMENSION_BUFFER; sv.Buffer.FirstElement = 0; sv.Buffer.NumElements = REC_U4;
            ok = SUCCEEDED(sDev->CreateBuffer(&rd, NULL, &sRecBuf)) && SUCCEEDED(sDev->CreateShaderResourceView(sRecBuf, &sv, &sRecSrv));
        }
        sGpuVerts = ok;
        PortLog("gfx: vertex transform and lighting on the %s", ok ? "GPU" : "CPU");
    }

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = VB_BYTES; bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
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
    const char* perfEnv = getenv("PORT_PERF_LOG");
    sSplit = gPortOptions.perfLog != NULL || (perfEnv && *perfEnv == '1');
    sStarted = TRUE;
    PortGxSetBackend(&kBackend);
    return TRUE;
}

const char* PortGfxStatsText(char* buf, u32 n) {
    if (sStarted) PortLog("gfx draws on the fast path (no register written since the draw before): %u", sFastHits);
    if (sStarted) PortLog("gfx shaders: %u compiled, %u read from the cache on disk", sShaderCompiles, sShaderHits);
    if (sStarted) PortLog("gfx record ring: %u segment changes, %u records started exactly at a segment end (each was a write without a GPU wait, or past the buffer, before the fix)", sRecWraps, sRecExactFits);
    if (sStarted) PortLog("gfx soft shadows: %u composites, %u shown frames", sShadowComposites, sGfxStats.presents);
    if (sStarted) PortLog("gfx copy time: flush %.1f s, resolve %.1f s, blit %.1f s", sTotalFlush / (double)Ticks_freq(), sTRes / (double)Ticks_freq(), sTBlit / (double)Ticks_freq()); if (sStarted) PortLog("gfx new textures %u: %.1f s, rtv %.1f s", sNewTex, sTNew / (double)Ticks_freq(), sTRtv / (double)Ticks_freq()); PortLog("gfx tex/display part %.1f s, clear+present %.1f s; disp %.1f", sTMk / (double)Ticks_freq(), sTPres / (double)Ticks_freq(), sTDisp / (double)Ticks_freq(), sTPres / (double)Ticks_freq());
    if (sStarted) PortLog("gfx vertices: %u of %u draws (%llu of %llu vertices) transformed on the GPU; %u batches drawn", sGfxStats.gpuDraws, sGfxStats.draws,
                          (unsigned long long)sGfxStats.gpuVerts, (unsigned long long)sGfxStats.verts, sGfxStats.flushes);
    if (!sStarted) { snprintf(buf, n, "no renderer (run with --window or --screenshot FILE)"); return buf; }
    snprintf(buf, n, "%u draw(s), %u triangle(s), %u picture(s) shown (+%u again while waiting for a retrace), %u copy(ies) to a texture, %u texture(s) decoded, %u shader(s), %u draw(s) skipped; backend time: draws %.1f s, copies %.1f s, %u draw(s) dropped by fast-forward",
             sGfxStats.draws, sGfxStats.tris, sGfxStats.presents, sExtraPictures, sGfxStats.texCopies, sGfxStats.textures, sGfxStats.shaders, sGfxStats.skipped,
             sTotalDraw / (double)Ticks_freq(), sTotalCopy / (double)Ticks_freq(), sFastSkipped);
    return buf;
}
// ------------------------------------------------------------------ savestates (port_state.cpp)
// What the renderer holds that the next frame is made from: the frame buffer's picture (a game that does not
// clear it draws over the last frame) and the frame-buffer copies (feedback effects read the last frame's).
// Textures, shaders and buffers are made again when used. The depth buffer is not kept: the game clears it.
static bool ReadTexture(ID3D11Texture2D* t, std::vector<u8>& out, u32* w, u32* h) {
    D3D11_TEXTURE2D_DESC d;
    t->GetDesc(&d);
    if (d.Format != DXGI_FORMAT_R8G8B8A8_UNORM || d.SampleDesc.Count != 1) return false;
    D3D11_TEXTURE2D_DESC sd = d;
    sd.Usage = D3D11_USAGE_STAGING; sd.BindFlags = 0; sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ; sd.MiscFlags = 0;
    ID3D11Texture2D* st = NULL;
    if (FAILED(sDev->CreateTexture2D(&sd, NULL, &st))) return false;
    sCtx->CopyResource(st, t);
    D3D11_MAPPED_SUBRESOURCE m;
    if (FAILED(sCtx->Map(st, 0, D3D11_MAP_READ, 0, &m))) { st->Release(); return false; }
    size_t at = out.size();
    out.resize(at + (size_t)d.Width * d.Height * 4);
    for (u32 y = 0; y < d.Height; y++) memcpy(&out[at + (size_t)y * d.Width * 4], (const u8*)m.pData + (size_t)y * m.RowPitch, (size_t)d.Width * 4);
    sCtx->Unmap(st, 0);
    st->Release();
    *w = d.Width; *h = d.Height;
    return true;
}
static void Put32(std::vector<u8>& out, u32 v) { out.insert(out.end(), (u8*)&v, (u8*)&v + 4); }

// The block a state stores (malloc'd; 0 when no renderer runs).
u32 PortGfxStateSave(u8** block) {
    *block = NULL;
    if (!sStarted || !sDev) return 0;
    std::lock_guard<std::recursive_mutex> lk(sGfxLock);
    FlushDraws();
    std::vector<u8> out;
    Put32(out, 0x31584647);                                  // "GFX1"
    Put32(out, sEfbPw); Put32(out, sEfbPh); Put32(out, sEfbMs ? 1 : 0); Put32(out, sShownLines);
    u32 w = 0, h = 0;
    if (sEfbMs || !ReadTexture(sEfbTex, out, &w, &h)) out.resize(out.size() + (size_t)sEfbPw * sEfbPh * 4);
    size_t countAt = out.size();
    Put32(out, 0);
    u32 count = 0;
    for (auto& e : sEfbCopies) {
        if (!e.second) continue;
        ID3D11Resource* res = NULL;
        e.second->GetResource(&res);
        ID3D11Texture2D* t = NULL;
        if (res) { res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&t); res->Release(); }
        if (!t) continue;
        D3D11_TEXTURE2D_DESC d;
        t->GetDesc(&d);
        auto rb = sEfbCopyRowBytes.find(e.first);
        size_t head = out.size();
        Put32(out, e.first); Put32(out, rb != sEfbCopyRowBytes.end() ? rb->second : 0); Put32(out, d.Width); Put32(out, d.Height);
        if (ReadTexture(t, out, &w, &h)) count++; else out.resize(head);
        t->Release();
    }
    memcpy(&out[countAt], &count, 4);
    *block = (u8*)malloc(out.size());
    if (!*block) return 0;
    memcpy(*block, out.data(), out.size());
    return (u32)out.size();
}

void PortGfxStateLoad(const u8* p, u32 n) {
    if (!sStarted || !sDev || n < 24) return;
    std::lock_guard<std::recursive_mutex> lk(sGfxLock);
    const u32* hd = (const u32*)p;
    if (hd[0] != 0x31584647) return;
    if (hd[1] != sEfbPw || hd[2] != sEfbPh || hd[3] != (sEfbMs ? 1u : 0u)) {
        PortLog("state: the pictures in the state are %ux%u%s, this run draws %ux%u%s: the first frames are drawn without the last frame's pictures",
                hd[1], hd[2], hd[3] ? " multisampled" : "", sEfbPw, sEfbPh, sEfbMs ? " multisampled" : "");
        return;
    }
    sShownLines = hd[4];
    const u8* q = p + 20;
    const u8* end = p + n;
    size_t efbBytes = (size_t)sEfbPw * sEfbPh * 4;
    if (q + efbBytes + 4 > end) return;
    if (!sEfbMs) sCtx->UpdateSubresource(sEfbTex, 0, NULL, q, sEfbPw * 4, 0);
    q += efbBytes;
    u32 count; memcpy(&count, q, 4); q += 4;
    for (u32 i = 0; i < count && q + 16 <= end; i++) {
        u32 rec[4]; memcpy(rec, q, 16); q += 16;
        size_t bytes = (size_t)rec[2] * rec[3] * 4;
        if (q + bytes > end) return;
        D3D11_TEXTURE2D_DESC d = {};
        d.Width = rec[2]; d.Height = rec[3]; d.MipLevels = 1; d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d.SampleDesc.Count = 1; d.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        D3D11_SUBRESOURCE_DATA init = {q, rec[2] * 4, 0};
        ID3D11Texture2D* t = NULL;
        if (SUCCEEDED(sDev->CreateTexture2D(&d, &init, &t))) {
            ID3D11ShaderResourceView*& slot = sEfbCopies[rec[0]];
            if (slot) slot->Release();
            slot = NULL;
            sDev->CreateShaderResourceView(t, NULL, &slot);
            t->Release();
            if (rec[1]) sEfbCopyRowBytes[rec[0]] = rec[1];
        }
        q += bytes;
    }
}

#endif
