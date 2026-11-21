// PC platform layer: DVD (the extracted disc's files), VI (the 50 Hz retrace), and the SDK
// libraries that do nothing on PC yet (GX, AX, AI, SC, WPAD, KPAD, NAND): those only count their
// calls and log the first one. No original code. Design: docs/port_runtime.md.
//
// Functions are defined under the names the game code calls them by (symbols.txt), with the
// callers' parameter lists. A name like fn_8024F1E8 is an SDK function nobody has renamed yet;
// the comment says which one it is.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "revolution.h"

int PortSelfTestOs(void);

#define STUB(name) PortStubCall(#name)

extern "C" {

// ------------------------------------------------------------------ DVD
// DVDFileInfo, as the SDK lays it out (0x3C bytes); the game reads the length and the state.
struct DVDFileInfo {
    u8    cbPad0[0x0C];
    s32   state;          // 0x0C: 0 = done, 1 = busy
    u8    cbPad1[0x20];
    u32   startAddr;      // 0x30: on PC, index + 1 into sDvdFiles
    u32   length;         // 0x34
    void* callback;       // 0x38
};
static_assert(sizeof(DVDFileInfo) == 0x3C, "DVDFileInfo layout");
typedef void (*DVDCallback)(s32 result, DVDFileInfo* info);

struct DvdFile { FILE* f; u32 size; char name[120]; };
static DvdFile sDvdFiles[64];
static u32 sDvdOpens, sDvdReads, sDvdMisses;
static u64 sDvdBytes;

BOOL DVDInit(void) {
    PortMilestone("DVDInit");
    return TRUE;
}

s32 DVDGetDriveStatus(void) { return 0; }        // idle, no error

// DVDOpen
s32 fn_8024F1E8(char* path, u8* infoBytes) {
    DVDFileInfo* info = (DVDFileInfo*)infoBytes;
    char host[MAX_PATH + 160];
    const char* rel = path;
    while (*rel == '/' || *rel == '\\') rel++;
    snprintf(host, sizeof(host), "%s\\files\\%s", gPortOptions.dataDir ? gPortOptions.dataDir : ".", rel);
    for (char* c = host; *c; c++) if (*c == '/') *c = '\\';
    FILE* f = fopen(host, "rb");
    if (!f) {
        sDvdMisses++;
        PortLog("disc: %s not found", path);
        return 0;
    }
    for (s32 i = 0; i < 64; i++) {
        DvdFile* d = &sDvdFiles[i];
        if (d->f) continue;
        d->f = f;
        fseek(f, 0, SEEK_END);
        d->size = (u32)ftell(f);
        snprintf(d->name, sizeof(d->name), "disc: opened %s (%u bytes)", path, d->size);
        memset(info, 0, sizeof(DVDFileInfo));
        info->startAddr = (u32)i + 1;
        info->length = d->size;
        sDvdOpens++;
        PortMilestone(d->name);
        return 1;
    }
    fclose(f);
    PortStop(7, "more than 64 disc files open");
    return 0;
}

// DVDClose
void fn_8024F494(u8* infoBytes) {
    DVDFileInfo* info = (DVDFileInfo*)infoBytes;
    if (info->startAddr == 0 || info->startAddr > 64) return;
    DvdFile* d = &sDvdFiles[info->startAddr - 1];
    if (d->f) fclose(d->f);
    d->f = NULL;
    info->startAddr = 0;
}

struct DvdDone { DVDFileInfo* info; s32 result; DVDCallback cb; };
static DvdDone sDvdDone[64];
static u32 sDvdDoneNext;

static void DvdDeliver(void* arg) {
    DvdDone* d = (DvdDone*)arg;
    d->info->state = 0;
    if (d->cb) d->cb(d->result, d->info);
    PortSignalChange();
}

// DVDReadAsyncPrio. The data is read at once; the callback arrives later from the interrupt
// thread, as the drive's interrupt does on the console.
s32 DVDReadAsync(u8* infoBytes, void* addr, s32 len, s32 offset, void* cb, s32 prio) {
    DVDFileInfo* info = (DVDFileInfo*)infoBytes;
    if (info->startAddr == 0 || info->startAddr > 64 || !sDvdFiles[info->startAddr - 1].f) return 0;
    DvdFile* d = &sDvdFiles[info->startAddr - 1];
    if (offset < 0 || (u32)offset > d->size) return 0;
    fseek(d->f, offset, SEEK_SET);
    s32 got = (s32)fread(addr, 1, (size_t)len, d->f);
    sDvdReads++;
    sDvdBytes += (u32)got;
    info->state = 1;
    info->callback = cb;
    DvdDone* done = &sDvdDone[sDvdDoneNext++ % 64];
    done->info = info;
    done->result = got;
    done->cb = (DVDCallback)cb;
    PortQueueInterrupt(DvdDeliver, done);
    return 1;
}

}  // extern "C"

// SDK functions the game calls that have no PC implementation yet and whose effect the boot does
// not depend on: a call logs once, returns 0 and is counted in the boot summary. Listed by
// original address, so a rename in symbols.txt does not matter. Everything not listed here stops
// the run when called. Add an entry only after checking in the asm what the function is.
const char* PortSoftSdk(u32 addr) {
    static const struct { u32 addr; const char* what; } kSoft[] = {
        {0x80264EE8, "audio: mixer init (fn_80264EE8)"},
        {0x80264F70, "audio: sequencer init (fn_80264F70)"},
        {0x8027FF24, "KPADReset (fn_8027FF24)"},
        // The game's audio init (fn_801FCFB4) and its effect set-up (fn_80203E8C). No sound yet.
        {0x8026099C, "audio: mixer set-up (fn_8026099C)"},
        {0x80263F18, "audio: sequencer defaults (fn_80263F18)"},
        {0x80259418, "audio: AX mode setter (fn_80259418)"},
        {0x80260AC4, "audio: mixer sound mode setter (fn_80260AC4)"},
        {0x80259B2C, "audio: AX frame callback registration (fn_80259B2C)"},
        {0x8025D2E0, "audio: AXFX effect init (fn_8025D2E0)"},
        {0x8025E3E4, "audio: AXFX effect init (fn_8025E3E4)"},
        {0x8025B4C0, "audio: AXFX effect init (fn_8025B4C0)"},
        {0x80264CC8, "audio: sequencer float setter (fn_80264CC8)"},
        {0x80264CD0, "audio: sequencer float setter (fn_80264CD0)"},
        // OSSetPeriodicAlarm: its only caller so far is the audio tick (fn_80202CAC), which needs AX.
        {0x80234394, "OSSetPeriodicAlarm for the audio tick (fn_80234394): not started"},
        // Registers a function and calls it with two words (from fn_80204478, argument fn_802B7DDC,
        // SDK code). What library this starts is not known yet.
        {0x802B4140, "library start through fn_802B4140 (fn_802B7DDC): not started"},
    };
    for (const auto& e : kSoft) if (e.addr == addr) return e.what;
    return NULL;
}

// SCGetSoundMode (the SDK reads item 0x11 and falls back to 1): 0 mono, 1 stereo, 2 surround.
extern "C" u8 fn_8026AC14(void) { return 1; }

const char* PortDvdStats(char* buf, u32 n) {
    snprintf(buf, n, "%u file(s) opened, %u read(s), %.1f MB read, %u file(s) not found",
             sDvdOpens, sDvdReads, sDvdBytes / 1048576.0, sDvdMisses);
    return buf;
}

extern "C" {

// ------------------------------------------------------------------ VI
static VIRetraceCallback sPreRetrace, sPostRetrace;
static u32   sRetraceCount;
static DWORD sNextRetraceMs;
static BOOL  sFramePending;

void VIInit(void) { PortMilestone("VIInit"); }
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb) { VIRetraceCallback p = sPreRetrace; sPreRetrace = cb; return p; }
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb) { VIRetraceCallback p = sPostRetrace; sPostRetrace = cb; return p; }
void VISetNextFrameBuffer(void* fb) { sFramePending = TRUE; }
void VISetBlack(BOOL black) { STUB(VISetBlack); }
u32  VIGetTvFormat(void) { return 1; }           // PAL

// GXRenderModeObj as VIConfigure last got it: the size and position of the picture.
u8  gPortViMode[0x3C];
static u32 sRetraceMs = 20;                      // PAL 50 Hz until VIConfigure says otherwise

// VIGetDTVStatus: no component cable, so no progressive scan
u32 fn_80245BF8(void) { return 0; }

// VIConfigure. The TV format is bits 2+ of viTVMode: PAL (1) is 50 fields per second, the
// others (NTSC, MPAL, EURGB60) are 60.
void fn_80244ED0(u8* renderMode) {
    memcpy(gPortViMode, renderMode, sizeof(gPortViMode));
    u32 format = *(u32*)renderMode >> 2;
    sRetraceMs = (format == 1) ? 20 : 17;
    static char what[96];
    snprintf(what, sizeof(what), "VIConfigure: %ux%u, %u Hz", *(u16*)(renderMode + 4), *(u16*)(renderMode + 8),
             format == 1 ? 50 : 60);
    PortMilestone(what);
}

// VIWaitForRetrace: sleeps until the next retrace interrupt
s32 fn_80244950(void) {
    u32 count = sRetraceCount;
    while (count == sRetraceCount) PortWaitChange();
    return 0;
}

void VIFlush(void) {
    if (!sFramePending) return;
    sFramePending = FALSE;
    PortMilestone("first frame presented (VISetNextFrameBuffer + VIFlush)");
    PortFrameDone();
}

// ------------------------------------------------------------------ GX (nothing is drawn yet)
volatile PPCWGPipe GXWGFifo;
// State a GX backend will need; today it is only recorded.
PortGxState gPortGx;
static GXFifoObj sGxFifo;
struct PortFifo { void* base; u32 size; void* rd; void* wr; };

GXFifoObj* GXInit(void* base, u32 size) {
    PortMilestone("GXInit");
    GXInitFifoBase(&sGxFifo, base, size);
    return &sGxFifo;
}
void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size) {
    PortFifo* f = (PortFifo*)fifo;
    f->base = f->rd = f->wr = base;
    f->size = size;
}
void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr) {
    PortFifo* f = (PortFifo*)fifo;
    f->rd = readPtr;
    f->wr = writePtr;
}
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr, void** writePtr) {
    PortFifo* f = (PortFifo*)fifo;
    *readPtr = f->rd;
    *writePtr = f->wr;
}
void GXSetCPUFifo(GXFifoObj* fifo) { STUB(GXSetCPUFifo); }

// The draw-done ("PE finish") and draw-sync ("PE token") interrupts.
typedef void (*GXDrawDoneCallback)(void);
typedef void (*GXDrawSyncCallback)(u16 token);
static GXDrawDoneCallback sDrawDoneCb;
static GXDrawSyncCallback sDrawSyncCb;
static u32 sGxDisplayLists;

static void GxDrawDoneInterrupt(void* arg) {
    if (sDrawDoneCb) sDrawDoneCb();
    PortSignalChange();
}

// Attaching a FIFO that holds commands to the graphics processor makes it run them. Every
// display list of the game ends with the draw-done token (RendDLEnd), so the draw-done
// interrupt follows. The commands themselves are not executed yet, and draw-sync tokens inside
// the list are not delivered.
void GXSetGPFifo(GXFifoObj* fifo) {
    PortFifo* f = (PortFifo*)fifo;
    if (f != (PortFifo*)&sGxFifo && f->rd != f->wr) {
        sGxDisplayLists++;
        f->rd = f->wr;
        PortQueueInterrupt(GxDrawDoneInterrupt, NULL);
    }
}
void GXSaveCPUFifo(GXFifoObj* fifo) { STUB(GXSaveCPUFifo); }
void GXSaveGPFifo(GXFifoObj* fifo) { STUB(GXSaveGPFifo); }
void GXGetGPStatus(u8* overhi, u8* underlow, u8* readIdle, u8* cmdIdle, u8* brkpt) {
    *overhi = *underlow = *brkpt = 0;
    *readIdle = *cmdIdle = 1;                    // the "graphics processor" has always caught up
}
void GXFlush(void) { STUB(GXFlush); }
void GXLoadTexMtxImm(const Mtx m, u32 id, int type) {
    STUB(GXLoadTexMtxImm);
    f32* dst = NULL;
    if (id >= 64 && id <= 124) dst = &gPortGx.postTexMtx[(id - 64) / 3][0][0];
    else if (id >= 30 && id <= 57) dst = &gPortGx.texMtx[(id - 30) / 3][0][0];
    if (dst) {
        memset(dst, 0, sizeof(gPortGx.texMtx[0]));
        memcpy(dst, m, type == 1 ? 8 * sizeof(f32) : 12 * sizeof(f32));   // type 1 = 2x4
    }
}

// GXDrawDone: sends the draw-done token and waits for the graphics processor to reach it
s32 fn_8024B358(void) {
    STUB(GXDrawDone);
    if (sDrawDoneCb) sDrawDoneCb();
    return 0;
}
// GXSetDrawDoneCallback
void* fn_8024B5D4(void* cb) { void* old = (void*)sDrawDoneCb; sDrawDoneCb = (GXDrawDoneCallback)cb; return old; }
// GXSetDrawSyncCallback
void* fn_8024B50C(void* cb) { void* old = (void*)sDrawSyncCb; sDrawSyncCb = (GXDrawSyncCallback)cb; return old; }

// GXClearVtxDesc
void fn_8024A7E4(void) {
    STUB(GXClearVtxDesc);
    memset(gPortGx.vtxDesc, 0, sizeof(gPortGx.vtxDesc));
    gPortGx.vtxDesc[9] = 1;                      // position: direct
}
// GXSetVtxDesc(attr, type)
void fn_8024A3A4(s32 attr, s32 type) {
    STUB(GXSetVtxDesc);
    if ((u32)attr < 26) gPortGx.vtxDesc[attr] = (u8)type;
}
// GXSetVtxAttrFmt(format, attr, component count, component type, fraction bits)
void fn_8024A818(s32 fmt, s32 attr, s32 cnt, s32 type, s32 frac) {
    STUB(GXSetVtxAttrFmt);
    if ((u32)fmt < 8 && (u32)attr < 26) {
        gPortGx.vtxFmt[fmt][attr].cnt = (u8)cnt;
        gPortGx.vtxFmt[fmt][attr].type = (u8)type;
        gPortGx.vtxFmt[fmt][attr].frac = (u8)frac;
    }
}
// GXInvalidateVtxCache
void fn_8024AC3C(void) { STUB(GXInvalidateVtxCache); }
// GXInvalidateTexAll
void fn_8024CB00(void) { STUB(GXInvalidateTexAll); }
// GXBegin(primitive, vertex format, vertex count); the vertices follow through GXWGFifo
void fn_8024B974(s32 prim, s32 fmt, s32 count) { STUB(GXBegin); }

// GXSetViewport
void fn_8024E48C(f32 left, f32 top, f32 w, f32 h, f32 nearZ, f32 farZ) {
    STUB(GXSetViewport);
    gPortGx.viewport[0] = left; gPortGx.viewport[1] = top; gPortGx.viewport[2] = w;
    gPortGx.viewport[3] = h;    gPortGx.viewport[4] = nearZ; gPortGx.viewport[5] = farZ;
}
// GXSetScissor
void fn_8024E4B8(s32 x, s32 y, s32 w, s32 h) {
    STUB(GXSetScissor);
    gPortGx.scissor[0] = x; gPortGx.scissor[1] = y; gPortGx.scissor[2] = w; gPortGx.scissor[3] = h;
}
// GXSetDispCopyYScale: returns the number of frame buffer lines the copy produces
// (the SDK's __GXGetNumXfbLines on the copy source height).
u32 fn_8024BC90(f32 yScale) {
    STUB(GXSetDispCopyYScale);
    gPortGx.copyYScale = yScale;
    u32 scale = (u32)(256.0f / yScale) & 0x1FF;
    u32 srcLines = (u32)gPortGx.copySrc[3];
    if (srcLines == 0) srcLines = *(u16*)(gPortViMode + 6);   // GXInit's default: the mode's EFB height
    if (scale == 0 || srcLines == 0) return srcLines;
    u32 lines = ((srcLines - 1) * 256) / scale + 1;
    if (scale > 0x80 && scale < 0x100) {
        u32 odd = scale;
        while ((odd & 1) == 0) odd >>= 1;
        if (srcLines % odd == 0) lines++;
    }
    if (lines > 0x400) lines = 0x400;
    return lines;
}
// GXSetDispCopySrc
void fn_8024BBE4(s32 x, s32 y, s32 w, s32 h) {
    STUB(GXSetDispCopySrc);
    gPortGx.copySrc[0] = x; gPortGx.copySrc[1] = y; gPortGx.copySrc[2] = w; gPortGx.copySrc[3] = h;
}
// GXSetDispCopyDst(frame buffer width, lines)
void fn_8024BC24(s32 w, u32 h) {
    STUB(GXSetDispCopyDst);
    gPortGx.copyDst[0] = w; gPortGx.copyDst[1] = (s32)h;
}
// GXSetCopyFilter(antialias, sample pattern[12][2], vertical filter on, coefficients[7])
void fn_8024BDD4(u32 aa, u8* pattern, s32 vf, u8* vfilter) {
    STUB(GXSetCopyFilter);
    gPortGx.copyAa = (u8)aa;
    gPortGx.copyVf = (u8)vf;
    if (pattern) memcpy(gPortGx.copyPattern, pattern, sizeof(gPortGx.copyPattern));
    if (vfilter) memcpy(gPortGx.copyVfilter, vfilter, sizeof(gPortGx.copyVfilter));
}
// GXSetDispCopyGamma
void fn_8024BFB8(s32 gamma) { STUB(GXSetDispCopyGamma); gPortGx.copyGamma = gamma; }
// GXSetCopyClear(colour, z): what the frame buffer is cleared to after a copy
void fn_8024BD5C(u8* color, u32 z) {
    STUB(GXSetCopyClear);
    memcpy(gPortGx.clearColor, color, 4);
    gPortGx.clearZ = z;
}
// GXCopyDisp(frame buffer, clear): copies the picture to the frame buffer VI shows
void fn_8024BFCC(void* fb, s32 clear) { STUB(GXCopyDisp); gPortGx.lastCopyDst = fb; }
// GXSetPixelFmt(pixel format, z format)
void fn_8024DFE4(s32 pixFmt, s32 zFmt) { STUB(GXSetPixelFmt); gPortGx.pixelFmt = pixFmt; gPortGx.zFmt = zFmt; }
// GXSetDither
void fn_8024E08C(u32 on) { STUB(GXSetDither); gPortGx.dither = (u8)on; }
// GXSetColorUpdate
void fn_8024DF28(s32 on) { STUB(GXSetColorUpdate); gPortGx.colorUpdate = (u8)on; }
// GXSetZMode(compare on, function, update on)
void fn_8024DF80(s32 enable, s32 func, s32 update) {
    STUB(GXSetZMode);
    gPortGx.zEnable = (u8)enable; gPortGx.zFunc = (u8)func; gPortGx.zUpdate = (u8)update;
}

// ---- the rest of the GX state the game's default-state reset (fn_8015D660) sets ----
// Geometry
// GXSetTexCoordGen2(coord, function, source, matrix, normalize, post-transform matrix)
void fn_8024AC4C(s32 coord, s32 func, s32 src, s32 mtx, s32 normalize, s32 postMtx) {
    STUB(GXSetTexCoordGen2);
    if ((u32)coord < 8) {
        gPortGx.texGen[coord].func = (u8)func;
        gPortGx.texGen[coord].src = (u8)src;
        gPortGx.texGen[coord].mtx = (u8)mtx;
        gPortGx.texGen[coord].normalize = (u8)normalize;
        gPortGx.texGen[coord].postMtx = (u8)postMtx;
    }
}
// GXSetNumTexGens
void fn_8024AE74(s32 count) { STUB(GXSetNumTexGens); gPortGx.numTexGens = (u8)count; }
// GXSetLineWidth(width in 1/6 pixels, texture offset)
void fn_8024BAC0(s32 width, s32 texOffsets) { STUB(GXSetLineWidth); gPortGx.lineWidth = (u8)width; }
// GXSetPointSize
void fn_8024BAF4(s32 size, s32 texOffsets) { STUB(GXSetPointSize); gPortGx.pointSize = (u8)size; }
// GXEnableTexOffsets(coord, for lines, for points)
void fn_8024BB28(s32 coord, s32 line, s32 point) { STUB(GXEnableTexOffsets); }
// GXSetCoPlanar
void fn_8024BB8C(s32 enable) { STUB(GXSetCoPlanar); }
// GXSetCullMode(0 none, 1 front, 2 back, 3 all)
void fn_8024BB64(s32 mode) { STUB(GXSetCullMode); gPortGx.cullMode = (u8)mode; }
// GXSetClipMode(0 = clipping on)
void fn_8024E55C(s32 mode) { STUB(GXSetClipMode); gPortGx.clipMode = (u8)mode; }
// GXSetScissorBoxOffset
void fn_8024E520(s32 x, s32 y) { STUB(GXSetScissorBoxOffset); gPortGx.scissorOffset[0] = x; gPortGx.scissorOffset[1] = y; }

// Matrices. Position/normal matrix ids are multiples of 3 (10 slots); texture matrix ids are
// 30 + 3n, 60 is the identity, post-transform ones are 64 + 3n with 125 the identity.
// GXLoadPosMtxImm
void fn_8024E280(f32* mtx, s32 id) {
    STUB(GXLoadPosMtxImm);
    if ((u32)id < 30) memcpy(gPortGx.posMtx[id / 3], mtx, sizeof(gPortGx.posMtx[0]));
}
// GXLoadNrmMtxImm (the upper 3x3 of a 3x4 matrix)
void fn_8024E2D0(f32* mtx, s32 id) {
    STUB(GXLoadNrmMtxImm);
    if ((u32)id < 30) memcpy(gPortGx.nrmMtx[id / 3], mtx, sizeof(gPortGx.nrmMtx[0]));
}
// GXSetCurrentMtx
void fn_8024E328(s32 id) { STUB(GXSetCurrentMtx); gPortGx.currentMtx = (u8)id; }
// GXLoadTexMtxImm, under its unnamed symbol (RendInit calls it by name)
void fn_8024E348(f32* mtx, s32 id, s32 type) { GXLoadTexMtxImm((const f32(*)[4])mtx, (u32)id, type); }

// Lighting channels
// GXSetNumChans
void fn_8024C2F0(s32 count) { STUB(GXSetNumChans); gPortGx.numChans = (u8)count; }
// GXSetChanCtrl(channel, lighting on, ambient source, material source, light mask, diffuse fn, attenuation fn)
void fn_8024C314(s32 chan, s32 enable, s32 ambSrc, s32 matSrc, s32 lightMask, s32 diffFn, s32 attnFn) { STUB(GXSetChanCtrl); }
// GXSetChanAmbColor(channel, colour)
void fn_8024C140(s32 chan, u8* color) { STUB(GXSetChanAmbColor); }
// GXSetChanMatColor(channel, colour)
void fn_8024C218(s32 chan, u8* color) { STUB(GXSetChanMatColor); }

// TEV
// GXSetTevOrder(stage, texture coordinate, texture map, colour channel)
void fn_8024DA08(s32 stage, s32 coord, s32 map, s32 color) { STUB(GXSetTevOrder); }
// GXSetNumTevStages
void fn_8024DB64(s32 count) { STUB(GXSetNumTevStages); gPortGx.numTevStages = (u8)count; }
// GXSetTevOp(stage, mode)
void fn_8024D508(s32 stage, s32 mode) { STUB(GXSetTevOp); }
// GXSetAlphaCompare(comp0, ref0, op, comp1, ref1)
void fn_8024D944(s32 comp0, s32 ref0, s32 op, s32 comp1, s32 ref1) {
    STUB(GXSetAlphaCompare);
    gPortGx.alphaComp[0] = (u8)comp0; gPortGx.alphaComp[1] = (u8)ref0; gPortGx.alphaComp[2] = (u8)op;
    gPortGx.alphaComp[3] = (u8)comp1; gPortGx.alphaComp[4] = (u8)ref1;
}
// GXSetZTexture(op, format, bias)
void fn_8024D97C(s32 op, s32 fmt, s32 bias) { STUB(GXSetZTexture); }
// GXSetTevKColorSel(stage, selection)
void fn_8024D7EC(s32 stage, s32 sel) { STUB(GXSetTevKColorSel); }
// GXSetTevKAlphaSel(stage, selection)
void fn_8024D83C(s32 stage, s32 sel) { STUB(GXSetTevKAlphaSel); }
// GXSetTevSwapMode(stage, rasterised colour table, texture colour table)
void fn_8024D88C(s32 stage, s32 rasSel, s32 texSel) { STUB(GXSetTevSwapMode); }
// GXSetTevSwapModeTable(table, r, g, b, a)
void fn_8024D8C8(s32 table, s32 r, s32 g, s32 b, s32 a) { STUB(GXSetTevSwapModeTable); }
// GXSetTevDirect(stage): no indirect texturing
void fn_8024D468(s32 stage) { STUB(GXSetTevDirect); }
// GXSetNumIndStages
void fn_8024D448(s32 count) { STUB(GXSetNumIndStages); }
// GXSetIndTexCoordScale(indirect stage, scale s, scale t)
void fn_8024D278(s32 stage, s32 scaleS, s32 scaleT) { STUB(GXSetIndTexCoordScale); }

// Pixel pipeline
// GXSetFog(type, colour, start z, end z, near z, far z)
void fn_8024DB88(s32 type, u8* color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ) {
    STUB(GXSetFog);
    gPortGx.fogType = type;
    memcpy(gPortGx.fogColor, color, 4);
    gPortGx.fog[0] = startZ; gPortGx.fog[1] = endZ; gPortGx.fog[2] = nearZ; gPortGx.fog[3] = farZ;
}
// GXSetFogRangeAdj(on, centre, table)
void fn_8024DDB4(s32 enable, s32 center, void* table) { STUB(GXSetFogRangeAdj); }
// GXSetBlendMode(type, source factor, destination factor, logic op)
void fn_8024DED8(s32 type, s32 src, s32 dst, s32 op) {
    STUB(GXSetBlendMode);
    gPortGx.blend[0] = (u8)type; gPortGx.blend[1] = (u8)src; gPortGx.blend[2] = (u8)dst; gPortGx.blend[3] = (u8)op;
}
// GXSetAlphaUpdate
void fn_8024DF54(s32 on) { STUB(GXSetAlphaUpdate); gPortGx.alphaUpdate = (u8)on; }
// GXSetZCompLoc(depth test before texturing)
void fn_8024DFB4(s32 beforeTex) { STUB(GXSetZCompLoc); gPortGx.zCompBeforeTex = (u8)beforeTex; }
// GXSetDstAlpha(on, alpha)
void fn_8024E0B8(s32 enable, s32 alpha) { STUB(GXSetDstAlpha); gPortGx.dstAlphaOn = (u8)enable; gPortGx.dstAlpha = (u8)alpha; }
// GXSetFieldMask(odd lines, even lines)
void fn_8024E0E8(s32 odd, s32 even) { STUB(GXSetFieldMask); }
// GXSetFieldMode(field rendering, half-height aspect)
void fn_8024E11C(s32 fieldMode, s32 halfAspect) { STUB(GXSetFieldMode); }
// GXSetCopyClamp
void fn_8024BC68(s32 clamp) { STUB(GXSetCopyClamp); }
// GXSetDispCopyFrame2Field
void fn_8024BC48(s32 mode) { STUB(GXSetDispCopyFrame2Field); }
// GXClearBoundingBox
void fn_8024C108(void) { STUB(GXClearBoundingBox); }

// Direct frame buffer access ("poke") settings and performance counters
void fn_8024B4B0(s32 enable) { STUB(GXPokeColorUpdate); }
void fn_8024B440(s32 enable) { STUB(GXPokeAlphaUpdate); }
void fn_8024B4DC(s32 enable) { STUB(GXPokeDither); }
void fn_8024B454(s32 type, s32 src, s32 dst, s32 op) { STUB(GXPokeBlendMode); }
void fn_8024B418(s32 func, s32 threshold) { STUB(GXPokeAlphaMode); }
void fn_8024B428(s32 mode) { STUB(GXPokeAlphaRead); }
void fn_8024B4C4(s32 enable, s32 alpha) { STUB(GXPokeDstAlpha); }
void fn_8024B4F0(s32 enable, s32 func, s32 update) { STUB(GXPokeZMode); }
void fn_8024E60C(s32 perf0, s32 perf1) { STUB(GXSetGPMetric); }
void fn_8024EE38(void) { STUB(GXClearGPMetric); }

// ------------------------------------------------------------------ MTX
// PSMTXIdentity (3x4)
void fn_80248050(f32* mtx) {
    for (int i = 0; i < 12; i++) mtx[i] = 0.0f;
    mtx[0] = mtx[5] = mtx[10] = 1.0f;
}

// ------------------------------------------------------------------ SC, AI, AX, WPAD, KPAD
void SCInit(void) { PortMilestone("SCInit"); }
s32  SCCheckStatus(void) { return 0; }           // settings are loaded
u8   SCGetAspectRatio(void) { return 1; }        // 16:9
u8   SCGetLanguage(void) { return 1; }           // English
// SCGetProgressiveMode: off
u8   fn_8026AB6C(void) { return 0; }
// SCGetEuRgb60Mode: on, so a PAL game runs at 60 Hz with 480 lines
u8   fn_8026AAA0(void) { return 1; }

void AIInit(u8* stack) { STUB(AIInit); }
void AXInit(void) { STUB(AXInit); }

static WPADAlloc sWpadAlloc;
static WPADFree  sWpadFree;
void WPADRegisterAllocator(WPADAlloc alloc, WPADFree free) { sWpadAlloc = alloc; sWpadFree = free; }
void WPADSetConnectCallback(s32 chan, void (*cb)(s32, s32)) { STUB(WPADSetConnectCallback); }
void WPADControlMotor(s32 chan, u32 command) { STUB(WPADControlMotor); }
void KPADInit(void) { STUB(KPADInit); }

}  // extern "C"

void PortViTick(void) {
    DWORD now = GetTickCount();
    if (sNextRetraceMs == 0) sNextRetraceMs = now + sRetraceMs;
    if ((s32)(now - sNextRetraceMs) < 0) return;
    sNextRetraceMs += sRetraceMs;                // 50 or 60 fields per second (VIConfigure)
    if ((s32)(now - sNextRetraceMs) > 200) sNextRetraceMs = now + sRetraceMs;
    sRetraceCount++;
    if (sPreRetrace) sPreRetrace(sRetraceCount);
    if (sPostRetrace) sPostRetrace(sRetraceCount);
    PortSignalChange();
}

// ------------------------------------------------------------------ self-test (no game data)
static s32 sTestResult = -1;
static void TestReadDone(s32 result, DVDFileInfo* info) { sTestResult = result; }
static u32 sTestRetraces;
static void TestRetrace(u32 count) { sTestRetraces++; }

int PortSelfTestSdk(void) {
    int fails = PortSelfTestOs();
#define CHECK(cond) do { if (!(cond)) { fails++; PortLog("self-test FAILED: %s (line %d)", #cond, __LINE__); } } while (0)
    // A disc file: open by its disc path, read part of it into Wii memory, completion by callback.
    char dir[MAX_PATH], sub[MAX_PATH + 32], file[MAX_PATH + 64];
    GetTempPathA(MAX_PATH, dir);
    snprintf(sub, sizeof(sub), "%smonsterlab_selftest_%lu", dir, GetCurrentProcessId());
    CreateDirectoryA(sub, NULL);
    snprintf(file, sizeof(file), "%s\\files", sub);
    CreateDirectoryA(file, NULL);
    snprintf(file, sizeof(file), "%s\\files\\test.bin", sub);
    FILE* f = fopen(file, "wb");
    CHECK(f != NULL);
    if (f) {
        for (int i = 0; i < 4096; i++) fputc(i & 0xFF, f);
        fclose(f);
        const char* savedDir = gPortOptions.dataDir;
        gPortOptions.dataDir = sub;
        u8 info[0x3C];
        u8* dst = (u8*)(uintptr_t)(PORT_MEM2_BASE + 0x02000000);
        CHECK(fn_8024F1E8((char*)"/test.bin", info) == 1);
        CHECK(*(u32*)(info + 0x34) == 4096);
        CHECK(DVDReadAsync(info, dst, 0x100, 0x800, (void*)TestReadDone, 2) == 1);
        for (int i = 0; i < 200 && sTestResult < 0; i++) PortWaitChange();
        CHECK(sTestResult == 0x100 && dst[0] == 0 && dst[0xFF] == 0xFF && *(s32*)(info + 0x0C) == 0);
        fn_8024F494(info);
        CHECK(fn_8024F1E8((char*)"/missing.bin", info) == 0);
        gPortOptions.dataDir = savedDir;
        DeleteFileA(file);
        snprintf(file, sizeof(file), "%s\\files", sub);
        RemoveDirectoryA(file);
        RemoveDirectoryA(sub);
    }
    // The retrace interrupt arrives while the game thread polls the clock.
    VISetPostRetraceCallback(TestRetrace);
    OSTime until = OSGetTime() + OS_TIMER_CLOCK / 5;
    while (OSGetTime() < until && sTestRetraces < 3) {
    }
    CHECK(sTestRetraces >= 3);
    VISetPostRetraceCallback(NULL);
#undef CHECK
    return fails;
}

#endif  // _WIN32
