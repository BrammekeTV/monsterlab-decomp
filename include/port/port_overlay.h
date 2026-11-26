// PC port: the stats overlay drawn over the picture with Dear ImGui (port_overlay.cpp).
#ifndef PORT_OVERLAY_H
#define PORT_OVERLAY_H

#include "types.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;

// One shown frame, measured by the renderer at each display copy.
typedef struct PortFrameStats {
    f32 frameMs;              // from the previous display copy to this one
    f32 renderMs;             // inside the Direct3D backend (draws and copies)
    f32 waitMs;               // asleep in VIWaitForRetrace
    f32 gameMs;               // the rest: game logic and the GX command stream
    f32 decodeMs, setupMs, flushMs, copyMs;   // renderMs split, with --perf-log only: vertex decode, the rest of each draw, batch upload and draw calls, frame-buffer copies
    u32 draws, verts;         // draw calls and GX vertices
    u32 efbCopies;            // frame-buffer copies to a texture
    u32 textures;             // decoded textures alive, plus the frame-buffer copy textures
    u32 voices, voiceCount;   // audio voices playing, of how many
    u32 outW, outH, picW, picH;
    BOOL wide;
} PortFrameStats;

void PortPerfOpen(const char* path);                       // --perf-log (port_perf.cpp)
BOOL PortPerfActive(void);
void PortPerfFrame(const PortFrameStats* s, f32 presentMs);
void PortPerfFinish(void);

extern BOOL gPortOverlayOn;   // F3, --stats
extern BOOL gPortGuiOpen;     // the settings screen (F1 or Esc, --settings): it has the keyboard and mouse, the game does not
extern int  gPortGuiPage;     // the page to open on (0 display, 1 audio, 2 controls), then -1

// The settings screen (port_settings_gui.cpp), built inside the overlay's ImGui frame.
void PortGuiSetOpen(BOOL open);
void PortGuiKeys(void);
void PortGuiInput(u32 w, u32 h);
void PortGuiBuild(u32 w, u32 h);
// Renderer hooks the settings screen uses (port_gfx_d3d11.cpp).
void PortGfxSetFullscreen(BOOL on);
void* PortOverlayThumb(int slot, unsigned long long stamp);   // port_overlay.cpp: a savestate slot's picture, or NULL
f32  PortGfxTakeWheel(void);  // mouse wheel notches since the last call

void PortOverlayStart(ID3D11Device* dev, ID3D11DeviceContext* ctx);
// Records a shown frame and reads the F3 key.
void PortOverlayFrame(const PortFrameStats* s);
// With the overlay on, draws the last recorded frame's numbers into target (w x h pixels).
void PortOverlayDraw(ID3D11RenderTargetView* target, u32 w, u32 h);
// The text of the overlay for these numbers (also what the self-test checks).
int  PortOverlayText(const PortFrameStats* s, f32 fps, f32 avgFps, f32 displayFps, char* buf, u32 n);
f32  PortGfxDisplayFps(void);   // pictures shown per second, repeats included (port_gfx_d3d11.cpp)
int  PortSelfTestOverlay(void);

#endif
