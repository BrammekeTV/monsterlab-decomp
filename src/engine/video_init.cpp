// Video bring-up: picks the TV mode, allocates the two frame buffers and the GX FIFO (MEM1), starts VI
// and GX and installs the draw-done / draw-sync callbacks the render thread waits on.
// Original: 0x8015BF40 - 0x8015D23C, GX default state 0x8015D660 - 0x8015DDEC
// (auto_03_80156AAC_text.s)
#include "engine/rend.h"
#include "system/sys.h"

// The SDK's GXRenderModeObj (0x3C bytes): one TV mode as VI and GX need it.
struct VideoRenderMode {
    u32 viTVMode;            // 0x00: (format << 2) | scan mode; bit 0 set = non-interlaced
    u16 fbWidth;             // 0x04
    u16 efbHeight;           // 0x06
    u16 xfbHeight;           // 0x08
    u16 viXOrigin;           // 0x0A
    u16 viYOrigin;           // 0x0C
    u16 viWidth;             // 0x0E
    u16 viHeight;            // 0x10
    u32 xfbMode;             // 0x14
    u8  fieldRendering;      // 0x18
    u8  aa;                  // 0x19
    u8  samplePattern[12][2];// 0x1A
    u8  vfilter[7];          // 0x32
};
static_assert(sizeof(VideoRenderMode) == 0x3C, "GXRenderModeObj layout");

extern "C" {
u8   SCGetProgressiveMode(void);                 // 0x8026AB6C: SCGetProgressiveMode
u8   SCGetEuRgb60Mode(void);                 // 0x8026AAA0: SCGetEuRgb60Mode
u32  fn_80245BF8(void);                 // 0x80245BF8: VIGetDTVStatus (1 = component cable)
void fn_80244ED0(u8* renderMode);       // 0x80244ED0: VIConfigure
GXFifoObj* GXInit(void* base, u32 size);  // 0x802488E8
s32  fn_80244950(void);                 // 0x80244950: VIWaitForRetrace
s32  fn_8024B358(void);                 // 0x8024B358: GXDrawDone
void* fn_8024B5D4(void* cb);            // 0x8024B5D4: GXSetDrawDoneCallback, returns the old one
void* fn_8024B50C(void* cb);            // 0x8024B50C: GXSetDrawSyncCallback, returns the old one
// GX functions of the default-state reset (identified from the SDK's __GXInitGX, which
// fn_8015D660 repeats call for call)
void GXSetTexCoordGen2(s32 coord, s32 func, s32 src, s32 mtx, s32 normalize, s32 postMtx);   // 0x8024AC4C: GXSetTexCoordGen2
void GXSetNumTexGens(s32 a);                // 0x8024AE74: GXSetNumTexGens
void GXClearVtxDesc(void);                 // 0x8024A7E4: GXClearVtxDesc
void GXInvalidateVtxCache(void);                 // 0x8024AC3C: GXInvalidateVtxCache
void GXSetLineWidth(s32 width, s32 texOffsets);   // 0x8024BAC0: GXSetLineWidth
void GXSetPointSize(s32 size, s32 texOffsets);    // 0x8024BAF4: GXSetPointSize
void GXEnableTexOffsets(s32 coord, s32 line, s32 point);   // 0x8024BB28: GXEnableTexOffsets
void PSMTXIdentity(f32* mtx);             // 0x80248050: PSMTXIdentity
void GXLoadPosMtxImm(f32* mtx, s32 a);      // 0x8024E280: GXLoadPosMtxImm
void GXLoadNrmMtxImm(f32* mtx, s32 id);     // 0x8024E2D0: GXLoadNrmMtxImm
void GXSetCurrentMtx(s32 a);                // 0x8024E328: GXSetCurrentMtx
void fn_8024E348(f32* mtx, s32 id, s32 type);   // 0x8024E348: fn_8024E348
void GXSetViewport(f32 left, f32 top, f32 w, f32 h, f32 nearZ, f32 farZ);   // 0x8024E48C: GXSetViewport
void GXSetCoPlanar(s32 enable);           // 0x8024BB8C: GXSetCoPlanar
void GXSetCullMode(s32 a);                // 0x8024BB64: GXSetCullMode
void GXSetClipMode(s32 mode);             // 0x8024E55C: GXSetClipMode
void GXSetScissor(s32 x, s32 y, s32 w, s32 h);   // 0x8024E4B8: GXSetScissor
void GXSetScissorBoxOffset(s32 x, s32 y);         // 0x8024E520: GXSetScissorBoxOffset
void GXSetNumChans(s32 a);                // 0x8024C2F0: GXSetNumChans
void GXSetChanCtrl(s32 chan, s32 enable, s32 ambSrc, s32 matSrc, s32 lightMask, s32 diffFn, s32 attnFn);   // 0x8024C314: GXSetChanCtrl
void GXSetChanAmbColor(s32 chan, u8* color);  // 0x8024C140: GXSetChanAmbColor
void GXSetChanMatColor(s32 chan, u8* color);  // 0x8024C218: GXSetChanMatColor
void GXInvalidateTexAll(void);                 // 0x8024CB00: GXInvalidateTexAll
void GXSetTevOrder(s32 a, s32 b, s32 c, s32 d);   // 0x8024DA08: GXSetTevOrder(stage, coord, map, colour channel)
void GXSetNumTevStages(s32 a);                // 0x8024DB64: GXSetNumTevStages
void GXSetTevOp(s32 a, s32 b);         // 0x8024D508: GXSetTevOp
void GXSetAlphaCompare(s32 comp0, s32 ref0, s32 op, s32 comp1, s32 ref1);   // 0x8024D944: GXSetAlphaCompare
void GXSetZTexture(s32 op, s32 fmt, s32 bias);    // 0x8024D97C: GXSetZTexture
void GXSetTevKColorSel(s32 stage, s32 sel);   // 0x8024D7EC: GXSetTevKColorSel
void GXSetTevKAlphaSel(s32 stage, s32 sel);   // 0x8024D83C: GXSetTevKAlphaSel
void GXSetTevSwapMode(s32 stage, s32 rasSel, s32 texSel);   // 0x8024D88C: GXSetTevSwapMode
void GXSetTevSwapModeTable(s32 table, s32 r, s32 g, s32 b, s32 a);   // 0x8024D8C8: GXSetTevSwapModeTable
void GXSetTevDirect(s32 stage);            // 0x8024D468: GXSetTevDirect
void GXSetNumIndStages(s32 count);            // 0x8024D448: GXSetNumIndStages
void GXSetIndTexCoordScale(s32 stage, s32 scaleS, s32 scaleT);   // 0x8024D278: GXSetIndTexCoordScale
void GXSetFog(s32 type, u8* color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ);   // 0x8024DB88: GXSetFog
void GXSetFogRangeAdj(s32 enable, s32 center, void* table);   // 0x8024DDB4: GXSetFogRangeAdj
void GXSetBlendMode(s32 a, s32 b, s32 c, s32 d);   // 0x8024DED8: GXSetBlendMode
void GXSetColorUpdate(s32 a);                // 0x8024DF28: GXSetColorUpdate
void GXSetAlphaUpdate(s32 enable);           // 0x8024DF54: GXSetAlphaUpdate
void GXSetZMode(s32 a, s32 b, s32 c);  // 0x8024DF80: GXSetZMode
void GXSetZCompLoc(s32 beforeTex);        // 0x8024DFB4: GXSetZCompLoc
void GXSetDither(u32 a);                // 0x8024E08C: GXSetDither
void GXSetDstAlpha(s32 enable, s32 alpha);   // 0x8024E0B8: GXSetDstAlpha
void GXSetPixelFmt(s32 a, s32 b);         // 0x8024DFE4: GXSetPixelFmt
void GXSetFieldMask(s32 odd, s32 even);    // 0x8024E0E8: GXSetFieldMask
void GXSetFieldMode(s32 fieldMode, s32 halfAspect);   // 0x8024E11C: GXSetFieldMode
void GXSetCopyClear(u8* color, u32 z);     // 0x8024BD5C: GXSetCopyClear
void GXSetDispCopySrc(s32 x, s32 y, s32 w, s32 h);   // 0x8024BBE4: GXSetDispCopySrc
void GXSetDispCopyDst(s32 w, u32 h);         // 0x8024BC24: GXSetDispCopyDst
u32  GXSetDispCopyYScale(f32 yScale);           // 0x8024BC90: GXSetDispCopyYScale
void GXSetCopyClamp(s32 clamp);            // 0x8024BC68: GXSetCopyClamp
void GXSetCopyFilter(u32 aa, u8* pattern, s32 vf, u8* vfilter);   // 0x8024BDD4: GXSetCopyFilter
void fn_8024BFB8(s32 a);                // 0x8024BFB8: GXSetDispCopyGamma
void GXSetDispCopyFrame2Field(s32 mode);             // 0x8024BC48: GXSetDispCopyFrame2Field
void GXClearBoundingBox(void);                 // 0x8024C108: GXClearBoundingBox
void GXPokeColorUpdate(s32 enable);           // 0x8024B4B0: GXPokeColorUpdate
void GXPokeAlphaUpdate(s32 enable);           // 0x8024B440: GXPokeAlphaUpdate
void GXPokeDither(s32 enable);           // 0x8024B4DC: GXPokeDither
void GXPokeBlendMode(s32 type, s32 src, s32 dst, s32 op);   // 0x8024B454: GXPokeBlendMode
void GXPokeAlphaMode(s32 func, s32 threshold);      // 0x8024B418: GXPokeAlphaMode
void GXPokeAlphaRead(s32 mode);             // 0x8024B428: GXPokeAlphaRead
void GXPokeDstAlpha(s32 enable, s32 alpha);        // 0x8024B4C4: GXPokeDstAlpha
void GXPokeZMode(s32 enable, s32 func, s32 update);   // 0x8024B4F0: GXPokeZMode
void GXSetGPMetric(s32 perf0, s32 perf1); // 0x8024E60C: GXSetGPMetric
void fn_8024EE38(void);                 // 0x8024EE38: GXClearGPMetric
void fn_8015BBC0(s32 clear);            // 0x8015BBC0: resets GX state for a frame, clears the frame buffers
s32  fn_8015D2E0(void);                 // 0x8015D2E0: draw-done callback, signals gVideoDrawDone
}
void fn_8015D23C(u32 arg);              // 0x8015D23C: draw-sync callback, dispatches the token

// ---- video state ----
extern "C" {
extern OSSemaphore  gVideoDrawDone;     // 0x8040AE30
extern OSSemaphore  gVideoGXLock;       // 0x8040AE3C
extern u8           lbl_8040AE48[];     // 0x8040AE48: the active render mode (VideoRenderMode)
extern GXFifoObj*   gVideoMainFifo;     // 0x805FB530
extern void*        lbl_805FB534;       // 0x805FB534: memory of the main GX FIFO (64 KB)
extern void*        lbl_805FB538[];     // 0x805FB538: the two external frame buffers
extern u8           lbl_805FB540;       // 0x805FB540: a display list is being captured
extern s32          lbl_805FB544;       // 0x805FB544: number of draw-sync handlers
extern u8           lbl_805FB548[];     // 0x805FB548: draw-sync handler keys (the token's high byte)
extern u8           lbl_8040AE88[];     // 0x8040AE88: draw-sync handlers (function pointers)
extern s32          lbl_805FB550;       // 0x805FB550: frame buffer width
extern s32          lbl_805FB554;       // 0x805FB554: frame buffer height
extern u8           lbl_805F9250;       // 0x805F9250: when set, the picture is narrowed (viWidth 666/670)
// TV mode tables (.data, GXRenderModeObj each)
extern u8 lbl_803982F0[];               // 0x803982F0: NTSC interlaced
extern u8 lbl_8039832C[];               // 0x8039832C: NTSC progressive
extern u8 lbl_80398368[];               // 0x80398368: MPAL interlaced
extern u8 lbl_803983A4[];               // 0x803983A4: MPAL progressive
extern u8 lbl_803983E0[];               // 0x803983E0: PAL 50 Hz
extern u8 lbl_8039841C[];               // 0x8039841C: EURGB60 interlaced
extern u8 lbl_80398458[];               // 0x80398458: EURGB60 progressive
}

#define VIDEO_FRAME_BUFFER_SIZE 0xCA800   // 0xD0000 - 0x5800
#define VIDEO_FIFO_SIZE         0x10000

// 0x8015BF40
extern "C" void VideoInit(RendVideo* video) {
    (void)video;
    VideoRenderMode* mode = (VideoRenderMode*)lbl_8040AE48;

    VIInit();
    OSInitSemaphore(&gVideoDrawDone, 0);
    OSInitSemaphore(&gVideoGXLock, 1);
    for (int i = 0; i < 2; i++) lbl_805FB538[i] = MEMAllocFromAllocator(&gSysAllocator, VIDEO_FRAME_BUFFER_SIZE);
    lbl_805FB534 = MEMAllocFromAllocator(&gSysAllocator, VIDEO_FIFO_SIZE);
    lbl_805FB540 = 0;
    lbl_805FB544 = 0;

    // Progressive scan needs both the console setting and a component cable.
    u8 progressiveSetting = SCGetProgressiveMode();
    BOOL progressive = FALSE;
    if (fn_80245BF8() != 0 && progressiveSetting == 1) progressive = TRUE;

    BOOL pal50 = FALSE;
    const u8* table;
    switch ((s32)VIGetTvFormat()) {
    case 2:   // MPAL
        table = progressive ? lbl_803983A4 : lbl_80398368;
        break;
    case 5:   // EURGB60
        table = progressive ? lbl_80398458 : lbl_8039841C;
        break;
    case 1:   // PAL: 60 Hz when the console is set to it, else 50 Hz
        if (progressive) {
            table = lbl_80398458;
        } else if (SCGetEuRgb60Mode() == 1) {
            table = lbl_8039841C;
        } else {
            pal50 = TRUE;
            table = lbl_803983E0;
        }
        break;
    default:  // NTSC and everything else
        table = progressive ? lbl_8039832C : lbl_803982F0;
        break;
    }
    *mode = *(const VideoRenderMode*)table;

    if (lbl_805F9250) {
        mode->viWidth = (VIGetTvFormat() == 1) ? 0x29A : 0x29E;
        mode->viXOrigin = (u16)((s32)(0x2D0 - mode->viWidth) / 2);
    }
    if (pal50) {
        // 50 Hz: 520 lines, centred in the 574 visible ones.
        mode->xfbHeight = 0x208;
        mode->efbHeight = 0x208;
        mode->viHeight = 0x208;
        mode->viYOrigin = (0x23E - 0x208) / 2;
    }
    lbl_805FB550 = mode->fbWidth;
    lbl_805FB554 = mode->xfbHeight;

    fn_80244ED0((u8*)mode);
    VISetNextFrameBuffer(lbl_805FB538[0]);
    VIFlush();
    fn_80244950();
    if (mode->viTVMode & 1) fn_80244950();

    gVideoMainFifo = GXInit(lbl_805FB534, VIDEO_FIFO_SIZE);
    fn_8015BBC0(1);
    fn_8024B358();
    fn_8024B5D4((void*)fn_8015D2E0);
    fn_8024B50C((void*)fn_8015D23C);
}

// 0x8015D2B8
// Registers a handler for draw-sync tokens whose high byte is `key` (see fn_8015D23C).
extern "C" void fn_8015D2B8(RendVideo* video, s32 key, void (*handler)(void)) {
    (void)video;
    s32 count = lbl_805FB544;
    lbl_805FB548[count] = (u8)key;
    *(void (**)(void))(lbl_8040AE88 + count * 4) = handler;
    lbl_805FB544 = count + 1;
}

// 0x8015D660
// Puts every GX register back to the SDK's start-up defaults (the same calls as the SDK's own
// GX initialisation, with the game's TV mode): one texture coordinate, one replace stage, no
// lights, no fog, opaque blending, depth test on.
extern "C" void fn_8015D660(void) {
    VideoRenderMode* mode = (VideoRenderMode*)lbl_8040AE48;
    const u8 clearColor[4] = {0x40, 0x40, 0x40, 0xFF};    // 0x805FF200
    const u8 white[4] = {0xFF, 0xFF, 0xFF, 0xFF};         // 0x805FF204
    u8 color[4];
    f32 identity[3][4];

    // Texture coordinate i comes from vertex texture coordinate i, untransformed.
    for (int i = 0; i < 8; i++) GXSetTexCoordGen2(i, 1, 4 + i, 0x3C, 0, 0x7D);
    GXSetNumTexGens(1);
    GXClearVtxDesc();
    GXInvalidateVtxCache();
    GXSetLineWidth(6, 0);
    GXSetPointSize(6, 0);
    for (int i = 0; i < 8; i++) GXEnableTexOffsets(i, 0, 0);

    PSMTXIdentity(&identity[0][0]);
    GXLoadPosMtxImm(&identity[0][0], 0);
    GXLoadNrmMtxImm(&identity[0][0], 0);
    GXSetCurrentMtx(0);
    fn_8024E348(&identity[0][0], 0x3C, 0);
    fn_8024E348(&identity[0][0], 0x7D, 0);
    GXSetViewport(0.0f, 0.0f, (f32)mode->fbWidth, (f32)mode->xfbHeight, 0.0f, 1.0f);

    GXSetCoPlanar(0);
    GXSetCullMode(2);
    GXSetClipMode(0);
    GXSetScissor(0, 0, mode->fbWidth, mode->efbHeight);
    GXSetScissorBoxOffset(0, 0);

    // Both colour channels: unlit, vertex colour, black ambient, white material.
    GXSetNumChans(0);
    for (int chan = 4; chan <= 5; chan++) {
        GXSetChanCtrl(chan, 0, 0, 1, 0, 0, 2);
        color[0] = color[1] = color[2] = color[3] = 0;
        GXSetChanAmbColor(chan, color);
        color[0] = white[0]; color[1] = white[1]; color[2] = white[2]; color[3] = white[3];
        GXSetChanMatColor(chan, color);
    }

    GXInvalidateTexAll();
    for (int i = 0; i < 8; i++) GXSetTevOrder(i, i, i, 4);
    for (int i = 8; i < 16; i++) GXSetTevOrder(i, 0xFF, 0xFF, 0xFF);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetAlphaCompare(7, 0, 0, 7, 0);
    GXSetZTexture(0, 0x11, 0);
    for (int i = 0; i < 16; i++) {
        GXSetTevKColorSel(i, 6);
        GXSetTevKAlphaSel(i, 0);
        GXSetTevSwapMode(i, 0, 0);
    }
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetTevSwapModeTable(1, 0, 0, 0, 3);
    GXSetTevSwapModeTable(2, 1, 1, 1, 3);
    GXSetTevSwapModeTable(3, 2, 2, 2, 3);
    for (int i = 0; i < 16; i++) GXSetTevDirect(i);
    GXSetNumIndStages(0);
    for (int i = 0; i < 4; i++) GXSetIndTexCoordScale(i, 0, 0);

    color[0] = color[1] = color[2] = color[3] = 0;
    GXSetFog(0, color, 0.0f, 1.0f, 0.1f, 1.0f);
    GXSetFogRangeAdj(0, 0, NULL);
    GXSetBlendMode(0, 4, 5, 0);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(1);
    GXSetZMode(1, 3, 1);
    GXSetZCompLoc(1);
    GXSetDither(1);
    GXSetDstAlpha(0, 0);
    GXSetPixelFmt(0, 0);
    GXSetFieldMask(1, 1);
    GXSetFieldMode(mode->fieldRendering, mode->xfbHeight * 2 == mode->viHeight);

    color[0] = clearColor[0]; color[1] = clearColor[1]; color[2] = clearColor[2]; color[3] = clearColor[3];
    GXSetCopyClear(color, 0xFFFFFF);
    GXSetDispCopySrc(0, 0, mode->fbWidth, mode->efbHeight);
    GXSetDispCopyDst(mode->fbWidth, mode->efbHeight);
    GXSetDispCopyYScale((f32)mode->xfbHeight / (f32)mode->efbHeight);
    GXSetCopyClamp(3);
    GXSetCopyFilter(mode->aa, &mode->samplePattern[0][0], 1, mode->vfilter);
    fn_8024BFB8(0);
    GXSetDispCopyFrame2Field(0);
    GXClearBoundingBox();

    // The same for direct ("poke") access to the frame buffer, and no performance counters.
    GXPokeColorUpdate(1);
    GXPokeAlphaUpdate(1);
    GXPokeDither(0);
    GXPokeBlendMode(0, 0, 1, 0xF);
    GXPokeAlphaMode(7, 0);
    GXPokeAlphaRead(1);
    GXPokeDstAlpha(0, 0);
    GXPokeZMode(1, 7, 1);
    GXSetGPMetric(0x23, 0x16);
    fn_8024EE38();
}
