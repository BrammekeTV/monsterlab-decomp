// PC platform layer, development aid: `--show-texture N`. When the boot stops (a missing
// function, a crash), the main thread comes here instead and draws texture N of
// global_binary.sir for a few frames with the engine's own functions: RendBeginFrame,
// DrawFullscreenTexture, RendEndFrame, the render thread, the display copy. It exercises the
// whole path from game code to the window while the boot does not reach the game's own screens.
// No original code.
#ifdef _WIN32
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "port/port_gx.h"
#include "port/sir_loader.h"

extern "C" {
void RendBeginFrame(void);              // 0x8014F0A0
void RendEndFrame(void);
void DrawFullscreenTexture(void* tex);
void fn_8015B7D4(void);                 // 0x8015B7D4
void fn_80150DEC(void);                 // 0x80150DEC
void fn_80151014(void);                 // 0x80151014
void fn_800446BC(u8* arg0, u8* arg1);   // 0x800446BC: binds a texture (texture map 0) and its GX state
void fn_80151644(s32 prim, s32 count, u8* verts);   // 0x80151644: draws from an array of {u, v, colour, x, y, z}
void* MemAllocDefault(u32 size);        // 0x801D4650
extern s32 lbl_805FB51C;                // 0x805FB51C: a frame is open
}

void PortDemoMain(void) {
    PortLockEnter();
    // The texture list is at image + 4; every node is at its texture's + 0x28.
    const SirTextureList* list = (const SirTextureList*)(uintptr_t)(GLOBAL_BINARY_WII_BASE + 4);
    u32 want = (u32)gPortOptions.showTexture;
    if (list->count == 0 || list->count > 4096 || !list->head) PortStop(9, "--show-texture: global_binary.sir is not loaded yet");
    if (want >= list->count) PortStop(9, "--show-texture: the file has %u textures (0 to %u)", list->count, list->count - 1);
    u8* node = list->head;
    for (u32 i = 0; i < want && node; i++) node = *(u8**)node;
    SirTexture* tex = (SirTexture*)(node - SIR_TEXTURE_NODE);
    PortLog("showing texture %u of %u: \"%s\", %ux%u, GX format %u", want, list->count, tex->name, tex->size[0], tex->size[1], tex->format);

    for (int frame = 0; frame < 4; frame++) {
        fn_8015B7D4();
        if (lbl_805FB51C == 0) RendBeginFrame();
        // DrawFullscreenTexture sets the 2D state and projection (x, y from -1 to 1, view z from
        // 0 to 1). Its own quad is at z = -1, outside that range, and the call it once made to
        // bind its argument (fn_80154290) is an empty function in this build: it draws nothing
        // visible. So the texture is bound with the engine's bind function and a quad inside the
        // range is drawn with the engine's array draw.
        fn_800446BC(NULL, (u8*)tex);
        DrawFullscreenTexture(tex);
        struct DemoVtx { f32 u, v; u32 colour; f32 x, y, z; };
        DemoVtx* q = (DemoVtx*)MemAllocDefault(sizeof(DemoVtx) * 4);
        f32 half = 0.8f, aspect = 480.0f / 640.0f;          // a square picture on the 4:3 frame
        q[0] = {0, 0, 0xFFFFFFFFu, -half * aspect, -half, 0.5f};
        q[1] = {1, 0, 0xFFFFFFFFu, half * aspect, -half, 0.5f};
        q[2] = {0, 1, 0xFFFFFFFFu, -half * aspect, half, 0.5f};
        q[3] = {1, 1, 0xFFFFFFFFu, half * aspect, half, 0.5f};
        fn_80151644(0x98, 4, (u8*)q);
        fn_80150DEC();
        fn_80151014();
        RendEndFrame();
    }
    for (int i = 0; i < 20; i++) PortWaitChange();           // let the render thread finish the last list
    PortStop(0, "texture \"%s\" shown", tex->name);
}
#endif
