#pragma once
// PC port: the GX command stream interpreter's state and its renderer interface (port_gx.cpp).
#include "types.h"

// The three register files of the graphics processor, as the command stream last set them.
typedef struct PortGx {
    u32 bp[256];          // BP registers, 24 bits each: TEV, textures, blending, copies
    u32 bpMask;           // set by BP register 0xFE for the next BP write
    u32 cp[256];          // CP registers: vertex descriptor (0x50, 0x60), formats (0x70+, 0x80+, 0x90+),
                          // array bases (0xA0+) and strides (0xB0+), matrix indices (0x30, 0x40)
    u32 xf[0x58];         // XF registers 0x1000-0x1057: lighting, texgens, viewport (0x1A), projection (0x20)
    f32 xfMem[0x800];     // XF memory: position/texture matrices (0x000), normal (0x400), post (0x500), lights (0x600)
    // BP 0xE0-0xE7 hold two things each, chosen by bit 23 of the write: the four TEV colour
    // registers (previous, 0, 1, 2) and the four constant colours. r, g, b, a; 11-bit signed.
    s16 tevReg[4][4];
    s16 tevKonst[4][4];
} PortGx;
extern PortGx gPortGxRegs;

// One vertex attribute of the current vertex format. Index: 0 position-matrix index, 1-8 texture
// matrix indices, 9 position, 10 normal, 11-12 colours, 13-20 texture coordinates.
typedef struct PortGxAttr {
    u8 mode;              // 0 absent, 1 in the vertex, 2 8-bit index into an array, 3 16-bit index
    u8 cnt, fmt, shift;   // as in the vertex format registers
    u8 comps, elem;       // components and bytes per component (colours: 1 component)
    u8 offset, size;      // where it is in a vertex of the command stream, and how many bytes
} PortGxAttr;
typedef struct PortGxLayout {
    PortGxAttr attr[21];
    u32        size;      // bytes per vertex in the stream
} PortGxLayout;
void PortGxVertexLayout(const PortGx* g, u32 vat, PortGxLayout* out);
// Host pointer for a physical address from a register (array bases, display lists, textures).
u8*  PortGxHostPtr(u32 phys);

// What the interpreter hands to a renderer. Any member may be NULL.
typedef struct PortGxBackend {
    // prim: 0x80 quads, 0x90 triangles, 0x98 strip, 0xA0 fan, 0xA8 lines, 0xB0 line strip, 0xB8 points.
    // `data` is the big-endian vertex data from the stream (count * layout->size bytes).
    void (*draw)(const PortGx* g, u32 prim, const PortGxLayout* layout, u32 count, const u8* data);
    // BP register 0x52: copy the frame buffer to a texture, or to the display (bit 14).
    void (*copy)(const PortGx* g, u32 control);
    void (*loadTlut)(const PortGx* g, u32 source, u32 dest);
} PortGxBackend;
void PortGxSetBackend(const PortGxBackend* backend);

typedef struct PortGxStats {
    u32 lists, bp, cp, xf, xfIndexed, calls, draws, vertices, copies, tluts, drawDone, tokens;
} PortGxStats;
const PortGxStats* PortGxGetStats(void);
const char* PortGxStatsText(char* buf, u32 bufSize);
int  PortSelfTestGx(void);            // 0 = ok

// The Direct3D 11 renderer (port_gfx_d3d11.cpp). `window` FALSE renders off screen.
BOOL PortGfxStart(BOOL window, int scale);
// Writes the last presented picture as a PNG; FALSE if nothing was presented.
BOOL PortGfxSaveScreenshot(const char* path);
const char* PortGfxStatsText(char* buf, u32 bufSize);
