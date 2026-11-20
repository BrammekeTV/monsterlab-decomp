// Video bring-up: picks the TV mode, allocates the two frame buffers and the GX FIFO, starts VI
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
u8   fn_8026AB6C(void);                 // 0x8026AB6C: SCGetProgressiveMode
u8   fn_8026AAA0(void);                 // 0x8026AAA0: SCGetEuRgb60Mode
u32  fn_80245BF8(void);                 // 0x80245BF8: VIGetDTVStatus (1 = component cable)
void fn_80244ED0(u8* renderMode);       // 0x80244ED0: VIConfigure
GXFifoObj* GXInit(void* base, u32 size);  // 0x802488E8
s32  fn_80244950(void);                 // 0x80244950: VIWaitForRetrace
s32  fn_8024B358(void);                 // 0x8024B358: GXDrawDone
void* fn_8024B5D4(void* cb);            // 0x8024B5D4: GXSetDrawDoneCallback, returns the old one
void* fn_8024B50C(void* cb);            // 0x8024B50C: GXSetDrawSyncCallback, returns the old one
// GX functions of the default-state reset (identified from the SDK's __GXInitGX, which
// fn_8015D660 repeats call for call)
void fn_8024AC4C(s32 coord, s32 func, s32 src, s32 mtx, s32 normalize, s32 postMtx);   // 0x8024AC4C: GXSetTexCoordGen2
void fn_8024AE74(s32 a);                // 0x8024AE74: GXSetNumTexGens
void fn_8024A7E4(void);                 // 0x8024A7E4: GXClearVtxDesc
void fn_8024AC3C(void);                 // 0x8024AC3C: GXInvalidateVtxCache
void fn_8024BAC0(s32 width, s32 texOffsets);   // 0x8024BAC0: GXSetLineWidth
void fn_8024BAF4(s32 size, s32 texOffsets);    // 0x8024BAF4: GXSetPointSize
void fn_8024BB28(s32 coord, s32 line, s32 point);   // 0x8024BB28: GXEnableTexOffsets
void fn_80248050(f32* mtx);             // 0x80248050: PSMTXIdentity
void fn_8024E280(f32* mtx, s32 a);      // 0x8024E280: GXLoadPosMtxImm
void fn_8024E2D0(f32* mtx, s32 id);     // 0x8024E2D0: GXLoadNrmMtxImm
void fn_8024E328(s32 a);                // 0x8024E328: GXSetCurrentMtx
void fn_8024E348(f32* mtx, s32 id, s32 type);   // 0x8024E348: GXLoadTexMtxImm
void fn_8024E48C(f32 left, f32 top, f32 w, f32 h, f32 nearZ, f32 farZ);   // 0x8024E48C: GXSetViewport
void fn_8024BB8C(s32 enable);           // 0x8024BB8C: GXSetCoPlanar
void fn_8024BB64(s32 a);                // 0x8024BB64: GXSetCullMode
void fn_8024E55C(s32 mode);             // 0x8024E55C: GXSetClipMode
void fn_8024E4B8(s32 x, s32 y, s32 w, s32 h);   // 0x8024E4B8: GXSetScissor
void fn_8024E520(s32 x, s32 y);         // 0x8024E520: GXSetScissorBoxOffset
void fn_8024C2F0(s32 a);                // 0x8024C2F0: GXSetNumChans
void fn_8024C314(s32 chan, s32 enable, s32 ambSrc, s32 matSrc, s32 lightMask, s32 diffFn, s32 attnFn);   // 0x8024C314: GXSetChanCtrl
void fn_8024C140(s32 chan, u8* color);  // 0x8024C140: GXSetChanAmbColor
void fn_8024C218(s32 chan, u8* color);  // 0x8024C218: GXSetChanMatColor
void fn_8024CB00(void);                 // 0x8024CB00: GXInvalidateTexAll
void fn_8024DA08(s32 a, s32 b, s32 c, s32 d);   // 0x8024DA08: GXSetTevOrder(stage, coord, map, colour channel)
void fn_8024DB64(s32 a);                // 0x8024DB64: GXSetNumTevStages
void fn_8024D508(s32 a, s32 b);         // 0x8024D508: GXSetTevOp
void fn_8024D944(s32 comp0, s32 ref0, s32 op, s32 comp1, s32 ref1);   // 0x8024D944: GXSetAlphaCompare
void fn_8024D97C(s32 op, s32 fmt, s32 bias);    // 0x8024D97C: GXSetZTexture
void fn_8024D7EC(s32 stage, s32 sel);   // 0x8024D7EC: GXSetTevKColorSel
void fn_8024D83C(s32 stage, s32 sel);   // 0x8024D83C: GXSetTevKAlphaSel
void fn_8024D88C(s32 stage, s32 rasSel, s32 texSel);   // 0x8024D88C: GXSetTevSwapMode
void fn_8024D8C8(s32 table, s32 r, s32 g, s32 b, s32 a);   // 0x8024D8C8: GXSetTevSwapModeTable
void fn_8024D468(s32 stage);            // 0x8024D468: GXSetTevDirect
void fn_8024D448(s32 count);            // 0x8024D448: GXSetNumIndStages
void fn_8024D278(s32 stage, s32 scaleS, s32 scaleT);   // 0x8024D278: GXSetIndTexCoordScale
void fn_8024DB88(s32 type, u8* color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ);   // 0x8024DB88: GXSetFog
void fn_8024DDB4(s32 enable, s32 center, void* table);   // 0x8024DDB4: GXSetFogRangeAdj
void fn_8024DED8(s32 a, s32 b, s32 c, s32 d);   // 0x8024DED8: GXSetBlendMode
void fn_8024DF28(s32 a);                // 0x8024DF28: GXSetColorUpdate
void fn_8024DF54(s32 enable);           // 0x8024DF54: GXSetAlphaUpdate
void fn_8024DF80(s32 a, s32 b, s32 c);  // 0x8024DF80: GXSetZMode
void fn_8024DFB4(s32 beforeTex);        // 0x8024DFB4: GXSetZCompLoc
void fn_8024E08C(u32 a);                // 0x8024E08C: GXSetDither
void fn_8024E0B8(s32 enable, s32 alpha);   // 0x8024E0B8: GXSetDstAlpha
void fn_8024DFE4(s32 a, s32 b);         // 0x8024DFE4: GXSetPixelFmt
void fn_8024E0E8(s32 odd, s32 even);    // 0x8024E0E8: GXSetFieldMask
void fn_8024E11C(s32 fieldMode, s32 halfAspect);   // 0x8024E11C: GXSetFieldMode
void fn_8024BD5C(u8* color, u32 z);     // 0x8024BD5C: GXSetCopyClear
void fn_8024BBE4(s32 x, s32 y, s32 w, s32 h);   // 0x8024BBE4: GXSetDispCopySrc
void fn_8024BC24(s32 w, u32 h);         // 0x8024BC24: GXSetDispCopyDst
u32  fn_8024BC90(f32 yScale);           // 0x8024BC90: GXSetDispCopyYScale
void fn_8024BC68(s32 clamp);            // 0x8024BC68: GXSetCopyClamp
void fn_8024BDD4(u32 aa, u8* pattern, s32 vf, u8* vfilter);   // 0x8024BDD4: GXSetCopyFilter
void fn_8024BFB8(s32 a);                // 0x8024BFB8: GXSetDispCopyGamma
void fn_8024BC48(s32 mode);             // 0x8024BC48: GXSetDispCopyFrame2Field
void fn_8024C108(void);                 // 0x8024C108: GXClearBoundingBox
void fn_8024B4B0(s32 enable);           // 0x8024B4B0: GXPokeColorUpdate
void fn_8024B440(s32 enable);           // 0x8024B440: GXPokeAlphaUpdate
void fn_8024B4DC(s32 enable);           // 0x8024B4DC: GXPokeDither
void fn_8024B454(s32 type, s32 src, s32 dst, s32 op);   // 0x8024B454: GXPokeBlendMode
void fn_8024B418(s32 func, s32 threshold);      // 0x8024B418: GXPokeAlphaMode
void fn_8024B428(s32 mode);             // 0x8024B428: GXPokeAlphaRead
void fn_8024B4C4(s32 enable, s32 alpha);        // 0x8024B4C4: GXPokeDstAlpha
void fn_8024B4F0(s32 enable, s32 func, s32 update);   // 0x8024B4F0: GXPokeZMode
void fn_8024E60C(s32 perf0, s32 perf1); // 0x8024E60C: GXSetGPMetric
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
extern s32          lbl_805FB550;       // 0x805FB550: frame buffer width
extern s32          lbl_805FB554;       // 0x805FB554: frame buffer height
extern u8           lbl_805F9250;       // 0x805F9250: when set, the picture is narrowed (viWidth 666/670)
extern MEMAllocator lbl_803F2870;       // 0x803F2870
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
    for (int i = 0; i < 2; i++) lbl_805FB538[i] = MEMAllocFromAllocator(&lbl_803F2870, VIDEO_FRAME_BUFFER_SIZE);
    lbl_805FB534 = MEMAllocFromAllocator(&lbl_803F2870, VIDEO_FIFO_SIZE);
    lbl_805FB540 = 0;
    lbl_805FB544 = 0;

    // Progressive scan needs both the console setting and a component cable.
    u8 progressiveSetting = fn_8026AB6C();
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
        } else if (fn_8026AAA0() == 1) {
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
    for (int i = 0; i < 8; i++) fn_8024AC4C(i, 1, 4 + i, 0x3C, 0, 0x7D);
    fn_8024AE74(1);
    fn_8024A7E4();
    fn_8024AC3C();
    fn_8024BAC0(6, 0);
    fn_8024BAF4(6, 0);
    for (int i = 0; i < 8; i++) fn_8024BB28(i, 0, 0);

    fn_80248050(&identity[0][0]);
    fn_8024E280(&identity[0][0], 0);
    fn_8024E2D0(&identity[0][0], 0);
    fn_8024E328(0);
    fn_8024E348(&identity[0][0], 0x3C, 0);
    fn_8024E348(&identity[0][0], 0x7D, 0);
    fn_8024E48C(0.0f, 0.0f, (f32)mode->fbWidth, (f32)mode->xfbHeight, 0.0f, 1.0f);

    fn_8024BB8C(0);
    fn_8024BB64(2);
    fn_8024E55C(0);
    fn_8024E4B8(0, 0, mode->fbWidth, mode->efbHeight);
    fn_8024E520(0, 0);

    // Both colour channels: unlit, vertex colour, black ambient, white material.
    fn_8024C2F0(0);
    for (int chan = 4; chan <= 5; chan++) {
        fn_8024C314(chan, 0, 0, 1, 0, 0, 2);
        color[0] = color[1] = color[2] = color[3] = 0;
        fn_8024C140(chan, color);
        color[0] = white[0]; color[1] = white[1]; color[2] = white[2]; color[3] = white[3];
        fn_8024C218(chan, color);
    }

    fn_8024CB00();
    for (int i = 0; i < 8; i++) fn_8024DA08(i, i, i, 4);
    for (int i = 8; i < 16; i++) fn_8024DA08(i, 0xFF, 0xFF, 0xFF);
    fn_8024DB64(1);
    fn_8024D508(0, 3);
    fn_8024D944(7, 0, 0, 7, 0);
    fn_8024D97C(0, 0x11, 0);
    for (int i = 0; i < 16; i++) {
        fn_8024D7EC(i, 6);
        fn_8024D83C(i, 0);
        fn_8024D88C(i, 0, 0);
    }
    fn_8024D8C8(0, 0, 1, 2, 3);
    fn_8024D8C8(1, 0, 0, 0, 3);
    fn_8024D8C8(2, 1, 1, 1, 3);
    fn_8024D8C8(3, 2, 2, 2, 3);
    for (int i = 0; i < 16; i++) fn_8024D468(i);
    fn_8024D448(0);
    for (int i = 0; i < 4; i++) fn_8024D278(i, 0, 0);

    color[0] = color[1] = color[2] = color[3] = 0;
    fn_8024DB88(0, color, 0.0f, 1.0f, 0.1f, 1.0f);
    fn_8024DDB4(0, 0, NULL);
    fn_8024DED8(0, 4, 5, 0);
    fn_8024DF28(1);
    fn_8024DF54(1);
    fn_8024DF80(1, 3, 1);
    fn_8024DFB4(1);
    fn_8024E08C(1);
    fn_8024E0B8(0, 0);
    fn_8024DFE4(0, 0);
    fn_8024E0E8(1, 1);
    fn_8024E11C(mode->fieldRendering, mode->xfbHeight * 2 == mode->viHeight);

    color[0] = clearColor[0]; color[1] = clearColor[1]; color[2] = clearColor[2]; color[3] = clearColor[3];
    fn_8024BD5C(color, 0xFFFFFF);
    fn_8024BBE4(0, 0, mode->fbWidth, mode->efbHeight);
    fn_8024BC24(mode->fbWidth, mode->efbHeight);
    fn_8024BC90((f32)mode->xfbHeight / (f32)mode->efbHeight);
    fn_8024BC68(3);
    fn_8024BDD4(mode->aa, &mode->samplePattern[0][0], 1, mode->vfilter);
    fn_8024BFB8(0);
    fn_8024BC48(0);
    fn_8024C108();

    // The same for direct ("poke") access to the frame buffer, and no performance counters.
    fn_8024B4B0(1);
    fn_8024B440(1);
    fn_8024B4DC(0);
    fn_8024B454(0, 0, 1, 0xF);
    fn_8024B418(7, 0);
    fn_8024B428(1);
    fn_8024B4C4(0, 0);
    fn_8024B4F0(1, 7, 1);
    fn_8024E60C(0x23, 0x16);
    fn_8024EE38();
}
