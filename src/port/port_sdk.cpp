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

struct DvdFile { FILE* f; u32 size; char name[120]; char path[100]; };
static DvdFile sDvdFiles[64];
static u32 sDvdOpens, sDvdReads, sDvdMisses;
static u64 sDvdBytes;

BOOL DVDInit(void) {
    PortMilestone("DVDInit");
    return TRUE;
}

// The game polls this in a loop while a read is pending (fn_80204210). On the console the drive's
// interrupt preempts that loop; here the poll is where the interrupt thread gets to run.
s32 DVDGetDriveStatus(void) {
    PortYield();
    return 0;                                    // idle, no error
}

// DVDOpen
s32 DVDOpen(char* path, u8* infoBytes) {
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
        snprintf(d->path, sizeof(d->path), "%s", rel);
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
void DVDClose(u8* infoBytes) {
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
    if (got > 0) PortFileRead(d->path, (u32)offset, (u8*)addr, (u32)got);
    // The drive transfers whole 32-byte units, so a read that ends within the file size rounded
    // up to 32 delivers the length that was asked for (fn_80146C44 asks for the rounded length
    // of a file's last part and takes anything else as a failed read). The rest is zero.
    if (got >= 0 && got < len && (u32)offset + (u32)len <= ((d->size + 0x1Fu) & ~0x1Fu)) {
        memset((u8*)addr + got, 0, (size_t)(len - got));
        got = len;
    }
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
        {0x8025D2E0, "audio: AXFX effect init (fn_8025D2E0)"},
        {0x8025E3E4, "audio: AXFX effect init (fn_8025E3E4)"},
        {0x8025B4C0, "audio: AXFX effect init (fn_8025B4C0)"},
        {0x80264CC8, "audio: sequencer float setter (fn_80264CC8)"},
        {0x80264CD0, "audio: sequencer float setter (fn_80264CD0)"},
        {0x80263FAC, "audio: sequencer frame, no mixer yet (fn_80263FAC)"},
        {0x802623A8, "audio: mixer frame, no mixer yet (fn_802623A8)"},
        {0x8025B398, "audio: AX getter, no mixer yet (fn_8025B398)"},
        {0x8025B390, "audio: AX getter, no mixer yet (fn_8025B390)"},
        // OSSetPeriodicAlarm: its only caller so far is the audio tick (fn_80202CAC), which needs AX.
        {0x80234394, "OSSetPeriodicAlarm for the audio tick (fn_80234394): not started"},
        // The SDK's HOME-button menu library (HBM), used by HomeButtonInit and the game's
        // home-menu update and draw. There is no HOME menu on PC yet: every call does nothing
        // and returns 0 ("no button selected").
        {0x802C8AB4, "HOME menu: create (fn_802C8AB4)"},
        {0x802C8DE4, "HOME menu: aspect adjust (fn_802C8DE4)"},
        {0x802C8E00, "HOME menu: sound set-up (fn_802C8E00)"},
        {0x802C8C04, "HOME menu: fn_802C8C04"},
        {0x802C8C38, "HOME menu: fn_802C8C38"},
        {0x802C8C88, "HOME menu: fn_802C8C88"},
        {0x802C8DF4, "HOME menu: fn_802C8DF4"},
        {0x802C8E04, "HOME menu: fn_802C8E04"},
        // Wii Remote library calls that do nothing while there is no controller on PC.
        {0x802727E0, "WPAD: rumble motor (WPADDisconnect)"},
        {0x80272AB0, "WPAD: WPADSetDataFormat"},
        {0x80272B64, "WPAD: fn_80272B64"},
        {0x802737A8, "WPAD: speaker control (fn_802737A8)"},
        {0x80274F80, "WPAD: can send speaker data (fn_80274F80)"},
        {0x80275004, "WPAD: send speaker data (fn_80275004)"},
        {0x8027D270, "KPAD: pointer parameter (KPADSetPosParam)"},
        {0x8027D28C, "KPAD: pointer parameter (KPADSetHoriParam)"},
        // Registers a function and calls it with two words (from fn_80204478, argument fn_802B7DDC,
        // SDK code). What library this starts is not known yet.
        {0x802B4140, "library start through fn_802B4140 (fn_802B7DDC): not started"},
        // Bink (RAD Video Tools) is not ported yet: the open fails (returns NULL), so MoviePlay
        // gives up after its retries and the movie is skipped.
        {0x802B47A0, "Bink: sound track selection (fn_802B47A0)"},
        {0x802B4B30, "Bink: open (fn_802B4B30): no movie yet, it is skipped"},
    };
    for (const auto& e : kSoft) if (e.addr == addr) return e.what;
    return NULL;
}

// SCGetSoundMode (the SDK reads item 0x11 and falls back to 1): 0 mono, 1 stereo, 2 surround.
extern "C" u8 fn_8026AC14(void) { return 1; }

// KPADRead(channel, samples, capacity): the Wii Remote's samples since the last call. No input
// on PC yet: no samples, which the game takes as "no remote connected".
extern "C" s32 KPADRead(s32 chan, u8* buf, s32 count) {
    (void)chan; (void)buf; (void)count;
    return 0;
}

// WPADProbe(channel, device type): -1 and type 253 = no controller on this channel.
extern "C" s32 WPADProbe(s32 chan, u32* type) {
    (void)chan;
    if (type) *type = 253;
    return -1;
}

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
// State the SDK-level GX calls record; the command stream is handled in port_gx.cpp.
PortGxState gPortGx;
static void BpLoad(u32 word);

static void XfLoad(u32 addr, const f32* values, u32 count) {
    PortGxWrite8(0x10);
    PortGxWrite16((u16)(count - 1));
    PortGxWrite16((u16)addr);
    for (u32 i = 0; i < count; i++) PortGxWriteF32(values[i]);
}
// ids 0-63 are rows of the position/texture matrix memory (four floats a row), 64 and up the
// post-transform matrices at 0x500; type 1 is a 2x4 matrix.
void GXLoadTexMtxImm(const Mtx m, u32 id, int type) {
    XfLoad(id >= 64 ? 0x500 + (id - 64) * 4 : id * 4, &m[0][0], type == 1 ? 8 : 12);
}


// The vertex descriptor and formats set through the SDK are sent as CP registers at the next
// GXBegin, as the SDK does (it marks them dirty and flushes on the first draw).
void PortGxFlushState(void);                    // port_gxstate.cpp: the SDK's dirty GX state
static BOOL sVcdDirty;
static u8   sVatDirty;                          // one bit per format

// GXClearVtxDesc
void fn_8024A7E4(void) {
    memset(gPortGx.vtxDesc, 0, sizeof(gPortGx.vtxDesc));
    gPortGx.vtxDesc[9] = 1;                      // position: direct
    sVcdDirty = TRUE;
}
// GXSetVtxDesc(attr, type)
void fn_8024A3A4(s32 attr, s32 type) {
    if ((u32)attr < 26) gPortGx.vtxDesc[attr] = (u8)type;
    sVcdDirty = TRUE;
}
// GXSetVtxAttrFmt(format, attr, component count, component type, fraction bits)
void fn_8024A818(s32 fmt, s32 attr, s32 cnt, s32 type, s32 frac) {
    if ((u32)fmt < 8 && (u32)attr < 26) {
        gPortGx.vtxFmt[fmt][attr].cnt = (u8)cnt;
        gPortGx.vtxFmt[fmt][attr].type = (u8)type;
        gPortGx.vtxFmt[fmt][attr].frac = (u8)frac;
        sVatDirty |= (u8)(1u << fmt);
    }
}

static void BpLoad(u32 word) {
    PortGxWrite8(0x61);
    PortGxWrite32(word);
}

static void CpLoad(u8 reg, u32 value) {
    PortGxWrite8(0x08);
    PortGxWrite8(reg);
    PortGxWrite32(value);
}

static void FlushVertexState(void) {
    if (sVcdDirty) {
        const u8* d = gPortGx.vtxDesc;
        u32 lo = 0, hi = 0;
        for (u32 i = 0; i < 9; i++) if (d[i]) lo |= 1u << i;             // matrix indices are 0/1
        u32 nrm = d[25] ? d[25] : d[10];                                 // attribute 25: normal + binormal + tangent
        lo |= (u32)(d[9] & 3) << 9 | (nrm & 3) << 11 | (u32)(d[11] & 3) << 13 | (u32)(d[12] & 3) << 15;
        for (u32 i = 0; i < 8; i++) hi |= (u32)(d[13 + i] & 3) << (i * 2);
        CpLoad(0x50, lo);
        CpLoad(0x60, hi);
        sVcdDirty = FALSE;
    }
    for (u32 f = 0; f < 8; f++) {
        if (!(sVatDirty & (1u << f))) continue;
        const auto* v = gPortGx.vtxFmt[f];
        u32 nbt = gPortGx.vtxDesc[25] ? 1u : 0u;
        const auto& n = nbt ? v[25] : v[10];
        u32 a = (u32)(v[9].cnt & 1) | (u32)(v[9].type & 7) << 1 | (u32)(v[9].frac & 31) << 4 |
                (u32)(n.cnt != 0) << 9 | (u32)(n.type & 7) << 10 |
                (u32)(v[11].cnt & 1) << 13 | (u32)(v[11].type & 7) << 14 |
                (u32)(v[12].cnt & 1) << 17 | (u32)(v[12].type & 7) << 18 |
                (u32)(v[13].cnt & 1) << 21 | (u32)(v[13].type & 7) << 22 | (u32)(v[13].frac & 31) << 25 |
                1u << 30 | (u32)(n.cnt == 2) << 31;
        u32 b = (u32)(v[14].cnt & 1) | (u32)(v[14].type & 7) << 1 | (u32)(v[14].frac & 31) << 4 |
                (u32)(v[15].cnt & 1) << 9 | (u32)(v[15].type & 7) << 10 | (u32)(v[15].frac & 31) << 13 |
                (u32)(v[16].cnt & 1) << 18 | (u32)(v[16].type & 7) << 19 | (u32)(v[16].frac & 31) << 22 |
                (u32)(v[17].cnt & 1) << 27 | (u32)(v[17].type & 7) << 28 | 1u << 31;
        u32 c = (u32)(v[17].frac & 31) |
                (u32)(v[18].cnt & 1) << 5 | (u32)(v[18].type & 7) << 6 | (u32)(v[18].frac & 31) << 9 |
                (u32)(v[19].cnt & 1) << 14 | (u32)(v[19].type & 7) << 15 | (u32)(v[19].frac & 31) << 18 |
                (u32)(v[20].cnt & 1) << 23 | (u32)(v[20].type & 7) << 24 | (u32)(v[20].frac & 31) << 27;
        CpLoad((u8)(0x70 + f), a);
        CpLoad((u8)(0x80 + f), b);
        CpLoad((u8)(0x90 + f), c);
    }
    sVatDirty = 0;
}
// GXInvalidateVtxCache
void fn_8024AC3C(void) { STUB(GXInvalidateVtxCache); }
// GXInvalidateTexAll
void fn_8024CB00(void) { STUB(GXInvalidateTexAll); }
// GXBegin(primitive, vertex format, vertex count); the vertices follow through GXWGFifo
void fn_8024B974(s32 prim, s32 fmt, s32 count) {
    PortGxFlushState();
    FlushVertexState();
    PortGxWrite8((u8)(prim | fmt));
    PortGxWrite16((u16)count);
}

// GXSetViewport
void fn_8024E48C(f32 left, f32 top, f32 w, f32 h, f32 nearZ, f32 farZ) {
    PortGxWrite8(0x10); PortGxWrite16(5); PortGxWrite16(0x101A);
    PortGxWriteF32(w * 0.5f); PortGxWriteF32(-h * 0.5f); PortGxWriteF32((farZ - nearZ) * 16777215.0f);
    PortGxWriteF32(left + w * 0.5f + 342.0f); PortGxWriteF32(top + h * 0.5f + 342.0f); PortGxWriteF32(farZ * 16777215.0f);
    gPortGx.viewport[0] = left; gPortGx.viewport[1] = top; gPortGx.viewport[2] = w;
    gPortGx.viewport[3] = h;    gPortGx.viewport[4] = nearZ; gPortGx.viewport[5] = farZ;
}
// GXSetScissor
void fn_8024E4B8(s32 x, s32 y, s32 w, s32 h) {
    STUB(GXSetScissor);
    gPortGx.scissor[0] = x; gPortGx.scissor[1] = y; gPortGx.scissor[2] = w; gPortGx.scissor[3] = h;
    BpLoad(0x20000000 | (u32)((y + 342) & 0xFFF) | (u32)((x + 342) & 0xFFF) << 12);
    BpLoad(0x21000000 | (u32)((y + h + 341) & 0xFFF) | (u32)((x + w + 341) & 0xFFF) << 12);
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
    gPortGx.copySrc[0] = x; gPortGx.copySrc[1] = y; gPortGx.copySrc[2] = w; gPortGx.copySrc[3] = h;
    BpLoad(0x49000000 | (u32)(x & 0x3FF) | (u32)(y & 0x3FF) << 10);
    BpLoad(0x4A000000 | (u32)((w - 1) & 0x3FF) | (u32)((h - 1) & 0x3FF) << 10);
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
    memcpy(gPortGx.clearColor, color, 4);
    gPortGx.clearZ = z;
    BpLoad(0x4F000000 | (u32)color[3] << 8 | color[0]);     // alpha, red
    BpLoad(0x50000000 | (u32)color[1] << 8 | color[2]);     // green, blue
    BpLoad(0x51000000 | (z & 0xFFFFFF));
}
// GXCopyDisp(frame buffer, clear): copies the picture to the frame buffer VI shows
void fn_8024BFCC(void* fb, s32 clear) {
    gPortGx.lastCopyDst = fb;
    BpLoad(0x4B000000 | (((u32)(uintptr_t)fb & 0x3FFFFFFF) >> 5));
    BpLoad(0x52000000 | 0x4000 | (clear ? 0x800u : 0u));    // to the display, then clear
}

// ---- the rest of the GX state the game's default-state reset (fn_8015D660) sets ----
// Geometry

// Matrices. Position/normal matrix ids are multiples of 3 (10 slots); texture matrix ids are
// 30 + 3n, 60 is the identity, post-transform ones are 64 + 3n with 125 the identity.
// GXLoadPosMtxImm
void fn_8024E280(f32* mtx, s32 id) { XfLoad((u32)id * 4, mtx, 12); }
// GXLoadNrmMtxImm (the upper 3x3 of a 3x4 matrix)
void fn_8024E2D0(f32* mtx, s32 id) {
    f32 m[9] = {mtx[0], mtx[1], mtx[2], mtx[4], mtx[5], mtx[6], mtx[8], mtx[9], mtx[10]};
    XfLoad(0x400 + (u32)id * 3, m, 9);
}
// GXLoadTexMtxImm, under its unnamed symbol (RendInit calls it by name)
void fn_8024E348(f32* mtx, s32 id, s32 type) { GXLoadTexMtxImm((const f32(*)[4])mtx, (u32)id, type); }

// Lighting channels

// TEV

// Pixel pipeline
// GXSetCopyClamp
void fn_8024BC68(s32 clamp) { STUB(GXSetCopyClamp); }
// GXSetDispCopyFrame2Field
void fn_8024BC48(s32 mode) { STUB(GXSetDispCopyFrame2Field); }
// GXClearBoundingBox
void fn_8024C108(void) { STUB(GXClearBoundingBox); }

// Direct frame buffer access ("poke") settings and performance counters
void GXPokeColorUpdate(s32 enable) { STUB(GXPokeColorUpdate); }
void GXPokeAlphaUpdate(s32 enable) { STUB(GXPokeAlphaUpdate); }
void GXPokeDither(s32 enable) { STUB(GXPokeDither); }
void GXPokeBlendMode(s32 type, s32 src, s32 dst, s32 op) { STUB(GXPokeBlendMode); }
void GXPokeAlphaMode(s32 func, s32 threshold) { STUB(GXPokeAlphaMode); }
void GXPokeAlphaRead(s32 mode) { STUB(GXPokeAlphaRead); }
void GXPokeDstAlpha(s32 enable, s32 alpha) { STUB(GXPokeDstAlpha); }
void GXPokeZMode(s32 enable, s32 func, s32 update) { STUB(GXPokeZMode); }
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
        CHECK(DVDOpen((char*)"/test.bin", info) == 1);
        CHECK(*(u32*)(info + 0x34) == 4096);
        CHECK(DVDReadAsync(info, dst, 0x100, 0x800, (void*)TestReadDone, 2) == 1);
        for (int i = 0; i < 200 && sTestResult < 0; i++) PortWaitChange();
        CHECK(sTestResult == 0x100 && dst[0] == 0 && dst[0xFF] == 0xFF && *(s32*)(info + 0x0C) == 0);
        DVDClose(info);
        CHECK(DVDOpen((char*)"/missing.bin", info) == 0);
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
