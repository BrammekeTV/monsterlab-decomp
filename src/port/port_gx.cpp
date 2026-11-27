// PC platform layer: the GX command stream. No original code. Design: docs/port_runtime.md.
//
// On the console every GX command is a run of big-endian bytes written to the write-gather pipe
// (0xCC008000). The bytes land in the FIFO the CPU is attached to; the graphics processor
// executes the FIFO it is attached to. The game uses both ways the SDK allows:
//   - "immediate": CPU and GP share the main FIFO;
//   - display lists: the CPU FIFO is pointed at a buffer (VideoBeginCapture), and the render
//     thread later attaches the GP to that buffer (VideoRunDL).
// This file keeps that model: `GXWGFifo._u8 = x` appends bytes (PortGxWrite*), and attaching the
// GP runs the interpreter over them. The interpreter keeps the three register files (BP, CP, XF),
// works out the size of each draw command's vertices, raises the draw-done and draw-sync
// interrupts where the stream asks for them, and hands draws and frame-buffer copies to the
// renderer (PortGxBackend, none yet: they are counted).
#ifdef _WIN32
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "port/port_gx.h"
#include "revolution.h"

PORT_SAVED_BEGIN   // the emulated console state of this file goes into a savestate (docs/savestates.md)

struct PortFifo { u8* base; u32 size; u8* rd; u8* wr; };
static_assert(sizeof(PortFifo) <= sizeof(GXFifoObj), "FIFO state must fit in GXFifoObj");

volatile PPCWGPipe GXWGFifo;
PortGx gPortGxRegs;

static GXFifoObj sMainFifo;
static PortFifo* sCpu;                 // where pipe writes go
static PortFifo* sGp;                  // what the "graphics processor" executes
static const PortGxBackend* sBackend;
PORT_SAVED_END
static const PortGxFilter* sFilter;    // draws the port takes out of the stream (mods), NULL: none. Not part of a savestate:
PORT_SAVED_BEGIN                       // a state made without mods is loaded with them, and the other way round

typedef void (*GXDrawDoneCallback)(void);
typedef void (*GXDrawSyncCallback)(u16 token);
static GXDrawDoneCallback sDrawDoneCb;
static GXDrawSyncCallback sDrawSyncCb;

static PortGxStats sStats;
static u32 sLostBytes;                 // written with no CPU FIFO attached
static char sError[160];               // first malformed command

// ------------------------------------------------------------------ the pipe
// The processor interface's FIFO write pointer (0xCC003014). The game reads the register itself
// to learn how many bytes the display list it is recording holds (fn_8015BA30: register minus the
// list's base), and moves on to the next list when this one is nearly full.
#define PI_FIFO_WRITE_PTR (*(volatile u32*)(uintptr_t)(PORT_HW_BASE + 0x3014))
static inline void Put(u8 b) {
    PortFifo* f = sCpu;
    if (!f || !f->base) { sLostBytes++; return; }
    *f->wr++ = b;
    if (f->wr >= f->base + f->size) f->wr = f->base;       // the hardware wraps too
    PI_FIFO_WRITE_PTR = (u32)(uintptr_t)f->wr;
}
extern "C" void PortGxFlushState(void);            // port_gxstate.cpp
extern "C" void PortGxWrite8(u8 v) { Put(v); }
extern "C" void PortGxWrite16(u16 v) { Put((u8)(v >> 8)); Put((u8)v); }
extern "C" void PortGxWrite32(u32 v) { Put((u8)(v >> 24)); Put((u8)(v >> 16)); Put((u8)(v >> 8)); Put((u8)v); }
extern "C" void PortGxWriteF32(f32 v) { u32 u; memcpy(&u, &v, 4); PortGxWrite32(u); }

// ------------------------------------------------------------------ interrupts
static void DrawDoneInterrupt(void* arg) {
    (void)arg;
    if (sDrawDoneCb) sDrawDoneCb();
    PortSignalChange();
}
static void DrawSyncInterrupt(void* arg) {
    if (sDrawSyncCb) sDrawSyncCb((u16)(uintptr_t)arg);
    PortSignalChange();
}

// ------------------------------------------------------------------ vertex layout
static const u8 kCompSize[8] = {1, 1, 2, 2, 4, 0, 0, 0};       // u8, s8, u16, s16, f32
static const u8 kColorSize[8] = {2, 3, 4, 2, 3, 4, 0, 0};      // 565, 888, 888x, 4444, 6666, 8888

void PortGxVertexLayout(const PortGx* g, u32 vat, PortGxLayout* out) {
    memset(out, 0, sizeof(*out));
    u32 lo = g->cp[0x50], hi = g->cp[0x60];
    u32 a = g->cp[0x70 + vat], b = g->cp[0x80 + vat], c = g->cp[0x90 + vat];
    u32 off = 0;
    for (u32 i = 0; i < 9; i++) {                           // matrix indices: one byte each
        if (lo & (1u << i)) { out->attr[i].mode = 1; out->attr[i].offset = (u8)off; out->attr[i].size = 1; off++; }
    }
    struct { u32 mode, cnt, fmt, shift; } d[12];
    d[0] = {(lo >> 9) & 3, a & 1, (a >> 1) & 7, (a >> 4) & 31};                    // position
    d[1] = {(lo >> 11) & 3, (a >> 9) & 1, (a >> 10) & 7, 0};                       // normal
    d[2] = {(lo >> 13) & 3, (a >> 13) & 1, (a >> 14) & 7, 0};                      // colour 0
    d[3] = {(lo >> 15) & 3, (a >> 17) & 1, (a >> 18) & 7, 0};                      // colour 1
    d[4] = {hi & 3, (a >> 21) & 1, (a >> 22) & 7, (a >> 25) & 31};                 // tex 0
    d[5] = {(hi >> 2) & 3, b & 1, (b >> 1) & 7, (b >> 4) & 31};
    d[6] = {(hi >> 4) & 3, (b >> 9) & 1, (b >> 10) & 7, (b >> 13) & 31};
    d[7] = {(hi >> 6) & 3, (b >> 18) & 1, (b >> 19) & 7, (b >> 22) & 31};
    d[8] = {(hi >> 8) & 3, (b >> 27) & 1, (b >> 28) & 7, c & 31};
    d[9] = {(hi >> 10) & 3, (c >> 5) & 1, (c >> 6) & 7, (c >> 9) & 31};
    d[10] = {(hi >> 12) & 3, (c >> 14) & 1, (c >> 15) & 7, (c >> 18) & 31};
    d[11] = {(hi >> 14) & 3, (c >> 23) & 1, (c >> 24) & 7, (c >> 27) & 31};
    BOOL nrmIndex3 = (a >> 31) & 1;
    for (u32 i = 0; i < 12; i++) {
        PortGxAttr* at = &out->attr[9 + i];
        at->mode = (u8)d[i].mode;
        at->cnt = (u8)d[i].cnt;
        at->fmt = (u8)d[i].fmt;
        at->shift = (u8)d[i].shift;
        at->offset = (u8)off;
        if (d[i].mode == 0) continue;
        u32 comps, elem;
        if (i == 0) { comps = d[i].cnt ? 3 : 2; elem = kCompSize[d[i].fmt]; }
        else if (i == 1) { comps = d[i].cnt ? 9 : 3; elem = kCompSize[d[i].fmt]; }
        else if (i <= 3) { comps = 1; elem = kColorSize[d[i].fmt]; }
        else { comps = d[i].cnt ? 2 : 1; elem = kCompSize[d[i].fmt]; }
        at->comps = (u8)comps;
        at->elem = (u8)elem;
        u32 size;
        if (d[i].mode == 1) size = comps * elem;
        else {
            size = d[i].mode == 2 ? 1 : 2;
            if (i == 1 && d[i].cnt && nrmIndex3) size *= 3;         // one index per normal, binormal, tangent
        }
        at->size = (u8)size;
        off += size;
    }
    out->size = off;
}

// The layout of vertex format `vat`, kept while the five registers it is made from stay the same (the game draws
// thousands of times per frame with a handful of them).
static const PortGxLayout& CachedLayout(const PortGx* g, u32 vat) {
    static struct { u32 reg[5]; BOOL made; PortGxLayout layout; } sCache[8];
    auto& c = sCache[vat];
    const u32 reg[5] = {g->cp[0x50], g->cp[0x60], g->cp[0x70 + vat], g->cp[0x80 + vat], g->cp[0x90 + vat]};
    if (!c.made || memcmp(reg, c.reg, sizeof(reg))) {
        PortGxVertexLayout(g, vat, &c.layout);
        memcpy(c.reg, reg, sizeof(reg));
        c.made = TRUE;
    }
    return c.layout;
}

// ------------------------------------------------------------------ interpreter
static inline u32 Be16(const u8* p) { return (u32)p[0] << 8 | p[1]; }
static inline u32 Be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

// A physical address from a register, as the game wrote it: MEM1 or MEM2.
u8* PortGxHostPtr(u32 phys) {
    phys &= 0x3FFFFFFF;
    if (phys < PORT_MEM1_SIZE) return (u8*)(uintptr_t)(PORT_MEM1_BASE + phys);
    if (phys >= 0x10000000 && phys < 0x10000000 + PORT_MEM2_SIZE) return (u8*)(uintptr_t)(PORT_MEM2_BASE + (phys - 0x10000000));
    return NULL;
}

static void Fail(const char* what, u32 op, const u8* at, const u8* start) {
    if (sError[0]) return;
    snprintf(sError, sizeof(sError), "%s (byte 0x%02X at +0x%X of a command list)", what, op, (u32)(at - start));
    PortLog("GX: %s", sError);
    // The bytes before it show which command the stream lost track at.
    const u8* from = at - start > 0x60 ? at - 0x60 : start;
    for (const u8* row = from; row < at + 0x20; row += 0x10) {
        char line[80];
        int k = snprintf(line, sizeof(line), "+0x%04X ", (u32)(row - start));
        for (int i = 0; i < 0x10; i++) k += snprintf(line + k, sizeof(line) - k, "%s%02X", row + i == at ? ">" : " ", row[i]);
        PortLog("GX:   %s", line);
    }
}

static void XfWrite(PortGx* g, u32 addr, u32 value) {
    g->xfGen++;
    if (addr < 0x800) { memcpy(&g->xfMem[addr], &value, 4); if (addr >= 0x600 && addr < 0x680) g->xfLightGen++; else if (addr < 0x500) g->xfMtxGen++; return; }   // matrices and lights: raw float bits
    if (addr >= 0x1000 && addr < 0x1058) g->xf[addr - 0x1000] = value;
}

static void BpWrite(PortGx* g, u32 word) {
    u32 reg = word >> 24;
    u32 value = word & 0x00FFFFFF;
    u32 mask = g->bpMask;
    g->bpMask = 0x00FFFFFF;                                 // the mask applies to one write
    if (reg == 0xFE) { g->bpMask = value; return; }
    value = (g->bp[reg] & ~mask) | (value & mask);
    g->bp[reg] = value;
    g->bpGen++;
    if (reg >= 0xE0 && reg < 0xE8) {                        // TEV colour register or constant colour
        s16 (*dst)[4] = (value & 0x800000) ? g->tevKonst : g->tevReg;
        u32 i = (reg - 0xE0) >> 1;
        s16 lo = (s16)((s32)(value << 21) >> 21), hi = (s16)((s32)(value << 9) >> 21);
        if (reg & 1) { dst[i][2] = lo; dst[i][1] = hi; }    // BG
        else         { dst[i][0] = lo; dst[i][3] = hi; }    // RA
    }
    sStats.bp++;
    switch (reg) {
    case 0x45:                                              // PE done
        if (value & 2) { sStats.drawDone++; PortQueueInterrupt(DrawDoneInterrupt, NULL); }
        break;
    case 0x48:                                              // PE token with interrupt
        sStats.tokens++;
        PortQueueInterrupt(DrawSyncInterrupt, (void*)(uintptr_t)(value & 0xFFFF));
        break;
    case 0x52:                                              // copy the frame buffer (to a texture or to the display)
        sStats.copies++;
        if (sFilter && sFilter->beforeCopy) sFilter->beforeCopy();
        if (sBackend && sBackend->copy) sBackend->copy(g, value);
        break;
    case 0x64: case 0x65:                                   // load a palette
        if (reg == 0x65) { sStats.tluts++; if (sBackend && sBackend->loadTlut) sBackend->loadTlut(g, g->bp[0x64], value); }
        break;
    default:
        break;
    }
}

static void Run(PortGx* g, const u8* p, const u8* end, int depth) {
    const u8* start = p;
    while (p < end && !sError[0]) {
        u32 op = *p++;
        if (op == 0x00) continue;                           // NOP (padding)
        if (op == 0x08) {                                   // CP register
            if (end - p < 5) { Fail("truncated CP load", op, p - 1, start); return; }
            g->cp[p[0]] = Be32(p + 1);
            p += 5;
            sStats.cp++;
        } else if (op == 0x10) {                            // XF registers / memory
            if (end - p < 4) { Fail("truncated XF load", op, p - 1, start); return; }
            u32 n = Be16(p) + 1, addr = Be16(p + 2);
            p += 4;
            if (n > 16 || (u32)(end - p) < n * 4) { Fail("bad XF load length", op, p - 5, start); return; }
            for (u32 i = 0; i < n; i++, p += 4) XfWrite(g, addr + i, Be32(p));
            sStats.xf++;
        } else if (op == 0x20 || op == 0x28 || op == 0x30 || op == 0x38) {   // XF load from an array
            if (end - p < 4) { Fail("truncated indexed XF load", op, p - 1, start); return; }
            u32 index = Be16(p), spec = Be16(p + 2);
            p += 4;
            u32 n = (spec >> 12) + 1, addr = spec & 0xFFF;
            u32 arr = 0xC + ((op - 0x20) >> 3);             // arrays 12..15: position/normal/texture matrices, lights
            const u8* src = PortGxHostPtr(g->cp[0xA0 + arr]);
            if (src) {
                src += index * (g->cp[0xB0 + arr] & 0xFF);
                for (u32 i = 0; i < n; i++) { u32 v; memcpy(&v, src + i * 4, 4); XfWrite(g, addr + i, v); }   // host order
            }
            sStats.xfIndexed++;
        } else if (op == 0x40) {                            // call a display list
            if (end - p < 8) { Fail("truncated display-list call", op, p - 1, start); return; }
            u32 addr = Be32(p), size = Be32(p + 4);
            p += 8;
            const u8* dl = PortGxHostPtr(addr);
            sStats.calls++;
            if (!dl) { Fail("display-list call to an address outside memory", op, p - 9, start); return; }
            if (depth < 4) Run(g, dl, dl + size, depth + 1);
        } else if (op == 0x48) {                            // invalidate the vertex cache
        } else if (op == 0x61) {                            // BP register
            if (end - p < 4) { Fail("truncated BP load", op, p - 1, start); return; }
            BpWrite(g, Be32(p));
            p += 4;
        } else if (op >= 0x80 && op < 0xC0) {               // draw
            if (end - p < 2) { Fail("truncated draw", op, p - 1, start); return; }
            u32 count = Be16(p);
            p += 2;
            const PortGxLayout& layout = CachedLayout(g, op & 7);
            if (layout.size == 0 || (u32)(end - p) < count * layout.size) { Fail("draw runs past the end of the list", op, p - 3, start); return; }
            sStats.draws++;
            sStats.vertices += count;
            if (sFilter && sFilter->take(g, op & 0xF8, &layout, count, p)) { /* drawn later by the port */ }
            else if (sBackend && sBackend->draw) sBackend->draw(g, op & 0xF8, &layout, count, p);
            p += count * layout.size;
        } else {
            Fail("unknown GX command", op, p - 1, start);
            return;
        }
    }
}

// Runs what the GP's FIFO holds between its read and write pointers.
static void Drain(void) {
    PortFifo* f = sGp;
    if (!f || !f->base || f->rd == f->wr) return;
    if (f->wr >= f->rd) {
        Run(&gPortGxRegs, f->rd, f->wr, 0);
    } else {                                                // wrapped: only the main FIFO does
        static u8 joined[0x40000];
        u32 tail = (u32)(f->base + f->size - f->rd), head = (u32)(f->wr - f->base);
        if (tail + head <= sizeof(joined)) {
            memcpy(joined, f->rd, tail);
            memcpy(joined + tail, f->base, head);
            Run(&gPortGxRegs, joined, joined + tail + head, 0);
        }
    }
    f->rd = f->wr;
    sStats.lists++;
}

// ------------------------------------------------------------------ SDK: FIFO management
extern "C" {

GXFifoObj* GXInit(void* base, u32 size) {
    PortMilestone("GXInit");
    gPortGxRegs.bpMask = 0x00FFFFFF;
    GXInitFifoBase(&sMainFifo, base, size);
    sCpu = sGp = (PortFifo*)&sMainFifo;
    // The register writes at the end of __GXInitRevisionBits (0x802487B8): XF 0x1000 = 0x3F, XF 0x1012 = 1
    // (post-texgen "dual" matrices on: the game loads its animated texture matrices there, XF 0x500) and
    // BP 0x58 = 0xF.
    PortGxWrite8(0x10); PortGxWrite32(0x1000); PortGxWrite32(0x3F);
    PortGxWrite8(0x10); PortGxWrite32(0x1012); PortGxWrite32(1);
    PortGxWrite8(0x61); PortGxWrite32(0x5800000F);
    return &sMainFifo;
}
void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size) {
    PortFifo* f = (PortFifo*)fifo;
    f->base = f->rd = f->wr = (u8*)base;
    f->size = size;
}
void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr) {
    PortFifo* f = (PortFifo*)fifo;
    f->rd = (u8*)readPtr;
    f->wr = (u8*)writePtr;
    if (f == sCpu) PI_FIFO_WRITE_PTR = (u32)(uintptr_t)f->wr;
}
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr, void** writePtr) {
    PortFifo* f = (PortFifo*)fifo;
    *readPtr = f->rd;
    *writePtr = f->wr;
}
// The pointers live in the FIFO object itself here, so "saving" has nothing to copy. A shared
// (immediate) FIFO is run before the CPU moves away from it.
void GXSaveCPUFifo(GXFifoObj* fifo) { (void)fifo; if (sCpu == sGp) Drain(); }
void GXSaveGPFifo(GXFifoObj* fifo) { (void)fifo; }
void GXSetCPUFifo(GXFifoObj* fifo) {
    if (sCpu == sGp) Drain();
    sCpu = (PortFifo*)fifo;
    PI_FIFO_WRITE_PTR = (u32)(uintptr_t)sCpu->wr;
}
// Attaching the GP to a FIFO runs the commands in it: this is where a display list executes.
void GXSetGPFifo(GXFifoObj* fifo) {
    sGp = (PortFifo*)fifo;
    Drain();
}
void GXGetGPStatus(u8* overhi, u8* underlow, u8* readIdle, u8* cmdIdle, u8* brkpt) {
    *overhi = *underlow = *brkpt = 0;
    *readIdle = *cmdIdle = 1;                    // the interpreter has always caught up
}
void GXFlush(void) { PortGxFlushState(); if (sCpu == sGp) Drain(); }

// GXDrawDone: sends the draw-done token and waits for the graphics processor to reach it
s32 fn_8024B358(void) {
    PortGxWrite8(0x61);
    PortGxWrite32(0x45000002);
    GXFlush();
    return 0;
}
// GXSetDrawDoneCallback
void* fn_8024B5D4(void* cb) { void* old = (void*)sDrawDoneCb; sDrawDoneCb = (GXDrawDoneCallback)cb; return old; }
// GXSetDrawSyncCallback
void* fn_8024B50C(void* cb) { void* old = (void*)sDrawSyncCb; sDrawSyncCb = (GXDrawSyncCallback)cb; return old; }

}  // extern "C"

void PortGxSetBackend(const PortGxBackend* backend) { sBackend = backend; }
void PortGxSetFilter(const PortGxFilter* filter) { sFilter = filter; }
void PortGxDraw(const PortGx* g, u32 prim, const PortGxLayout* layout, u32 count, const u8* data) {
    if (sBackend && sBackend->draw) sBackend->draw(g, prim, layout, count, data);
}
const PortGxStats* PortGxGetStats(void) { return &sStats; }

const char* PortGxStatsText(char* buf, u32 n) {
    int k = snprintf(buf, n, "%u command list(s): %u BP, %u CP, %u XF (+%u indexed) loads, %u list call(s), %u draw(s) with %u vertices, "
                     "%u copy(ies), %u draw-done, %u token(s)",
                     sStats.lists, sStats.bp, sStats.cp, sStats.xf, sStats.xfIndexed, sStats.calls, sStats.draws, sStats.vertices,
                     sStats.copies, sStats.drawDone, sStats.tokens);
    if (sLostBytes && k > 0 && (u32)k < n) k += snprintf(buf + k, n - k, "; %u byte(s) written with no FIFO attached", sLostBytes);
    if (sError[0] && k > 0 && (u32)k < n) snprintf(buf + k, n - k, "; FIRST ERROR: %s", sError);
    return buf;
}

// ------------------------------------------------------------------ self-test (no game data)
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "[port] self-test FAILED: %s (port_gx.cpp:%d)\n", #cond, __LINE__); return 1; } } while (0)

static u32 sTestDone, sTestToken, sTestDraws, sTestVerts, sTestSize;
static void TestDone(void) { sTestDone++; }
static void TestToken(u16 t) { sTestToken = t; }
static void TestDraw(const PortGx* g, u32 prim, const PortGxLayout* layout, u32 count, const u8* data) {
    (void)g; (void)data;
    if (prim == 0x80) { sTestDraws++; sTestVerts += count; sTestSize = layout->size; }
}

int PortSelfTestGx(void) {
    static u8 mainBuf[0x400], listBuf[0x400];
    static GXFifoObj list;
    PortGxStats savedStats = sStats;
    PortFifo* savedCpu = sCpu; PortFifo* savedGp = sGp;
    GXFifoObj savedMain = sMainFifo;
    void* oldDone = fn_8024B5D4((void*)TestDone);
    void* oldSync = fn_8024B50C((void*)TestToken);
    static const PortGxBackend kTest = {TestDraw, NULL, NULL};
    const PortGxBackend* oldBackend = sBackend;
    sBackend = &kTest;

    GXInitFifoBase(&sMainFifo, mainBuf, sizeof(mainBuf));
    sCpu = sGp = (PortFifo*)&sMainFifo;
    gPortGxRegs.bpMask = 0x00FFFFFF;

    // Record a display list: vertex format 0 = position 3 x f32 direct, colour RGBA8 direct, one
    // texture coordinate 2 x f32 by 8-bit index; a masked BP write; one quad; a token; draw-done.
    GXInitFifoBase(&list, listBuf, sizeof(listBuf));
    GXInitFifoPtrs(&list, listBuf, listBuf);
    GXSetCPUFifo(&list);
    GXWGFifo._u8 = 0x08; GXWGFifo._u8 = 0x50; GXWGFifo._u32 = (1u << 9) | (1u << 13);
    GXWGFifo._u8 = 0x08; GXWGFifo._u8 = 0x60; GXWGFifo._u32 = 2;
    GXWGFifo._u8 = 0x08; GXWGFifo._u8 = 0x70; GXWGFifo._u32 = 1u | (4u << 1) | (1u << 13) | (5u << 14) | (1u << 21) | (4u << 22);
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x41123456;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE0000FF;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x41ABCDEF;
    GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 1; GXWGFifo._u16 = 0x101A; GXWGFifo._f32 = 320.0f; GXWGFifo._f32 = -240.0f;
    GXWGFifo._u8 = 0x80; GXWGFifo._u16 = 4;
    for (int i = 0; i < 4; i++) {
        GXWGFifo._f32 = (f32)i; GXWGFifo._f32 = 1.0f; GXWGFifo._f32 = 2.0f;
        GXWGFifo._u32 = 0xFF8040FF;
        GXWGFifo._u8 = (u8)i;
    }
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x48001234;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x45000002;
    GXSaveCPUFifo(&list);
    GXSetCPUFifo(&sMainFifo);
    void* rd; void* wr;
    GXGetFifoPtrs(&list, &rd, &wr);
    CHECK(rd == listBuf);
    CHECK((u8*)wr - listBuf == 6 * 3 + 5 * 3 + 13 + 3 + 4 * 17 + 10);
    CHECK(listBuf[2] == 0x00 && listBuf[4] == 0x22);        // big-endian words in the stream
    CHECK(sTestDraws == 0);                                 // nothing runs until the GP is attached

    GXSetGPFifo(&list);                                     // run it
    GXSetGPFifo(&sMainFifo);
    CHECK(!sError[0]);
    CHECK(sTestDraws == 1 && sTestVerts == 4 && sTestSize == 17);
    CHECK(gPortGxRegs.bp[0x41] == 0x1234EF);                // the mask kept the upper 16 bits
    f32 vx; memcpy(&vx, &gPortGxRegs.xf[0x1A], 4);
    CHECK(vx == 320.0f);
    for (int i = 0; i < 200 && (sTestDone == 0 || sTestToken == 0); i++) PortWaitChange();
    CHECK(sTestDone == 1 && sTestToken == 0x1234);

    // Immediate mode: commands written to the shared FIFO run at the flush.
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x22000777;
    CHECK(gPortGxRegs.bp[0x22] != 0x777);
    GXFlush();
    CHECK(gPortGxRegs.bp[0x22] == 0x777);

    // A malformed list is reported, not followed.
    static u8 bad[] = {0x61, 0x00, 0x00, 0x00, 0x01, 0x77, 0x00};
    Run(&gPortGxRegs, bad, bad + sizeof(bad), 0);
    CHECK(sError[0] != 0);

    sError[0] = 0;
    memset(&gPortGxRegs, 0, sizeof(gPortGxRegs));
    fn_8024B5D4(oldDone);
    fn_8024B50C(oldSync);
    sBackend = oldBackend;
    sMainFifo = savedMain; sCpu = savedCpu; sGp = savedGp;
    sStats = savedStats;
    sLostBytes = 0;
    return 0;
}
PORT_SAVED_END

#endif
