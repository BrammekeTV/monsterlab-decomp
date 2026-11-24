// PC platform layer: GX state calls that write BP / XF registers into the command stream.
// No original code. Design: docs/port_runtime.md.
//
// Each function is the SDK's own, checked against the asm: the same register fields in the same
// places (`Rlwimi` is the PowerPC rlwimi, so the field positions are the asm's), the same
// register words. The SDK keeps a shadow of each register in __GXData and sends the whole word
// on every change; `Sh` below is that shadow, with each register's number in its top byte as
// GXInit leaves it. A few calls only mark state dirty (texture coordinate generation, colour
// channels, matrix indices, the generation mode); the SDK sends those at the next GXBegin,
// GXFlush or draw-done (__GXSetDirtyState, `PortGxFlushState`).
#ifdef _WIN32
#include <string.h>

#include "port/port.h"
#include "port/port_gx.h"
#include "revolution.h"

extern "C" {
void PortGxWrite8(u8 v);
void PortGxWrite16(u16 v);
void PortGxWrite32(u32 v);
}

namespace {

// ---- PowerPC rlwimi: insert rotl(s, sh) into a under mask bits mb..me (IBM bit numbers) ----
inline u32 Mask(int mb, int me) {
    if (mb <= me) return (0xFFFFFFFFu >> mb) & (0xFFFFFFFFu << (31 - me));
    return (0xFFFFFFFFu >> mb) | (0xFFFFFFFFu << (31 - me));
}
inline u32 Rotl(u32 s, int sh) { return sh ? (s << sh) | (s >> (32 - sh)) : s; }
inline u32 Rlwimi(u32 a, u32 s, int sh, int mb, int me) {
    u32 m = Mask(mb, me);
    return (Rotl(s, sh) & m) | (a & ~m);
}
inline u32 Be32At(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
inline u32 BitsOf(f32 f) { u32 u; memcpy(&u, &f, 4); return u; }

void Bp(u32 word) { PortGxWrite8(0x61); PortGxWrite32(word); }
void Xf1(u32 addr, u32 value) { PortGxWrite8(0x10); PortGxWrite32(addr); PortGxWrite32(value); }   // count - 1 = 0
void Cp(u8 reg, u32 value) { PortGxWrite8(0x08); PortGxWrite8(reg); PortGxWrite32(value); }

// Dirty flags (__GXData + 0x5FC)
enum {
    DIRTY_GENMODE = 0x00000004,
    DIRTY_AMBIENT = 0x00000100,     // << channel (0, 1)
    DIRTY_MATERIAL = 0x00000400,    // << channel (0, 1)
    DIRTY_CHAN = 0x00001000,        // << channel (0-3)
    DIRTY_TEXGEN = 0x00010000,      // << coordinate (0-7)
    DIRTY_NUMCHANS = 0x01000000,
    DIRTY_NUMTEXGENS = 0x02000000,
    DIRTY_MTXIDX = 0x04000000,
};

struct Sh {
    bool inited;
    u32 dirty;
    u32 mtxIdx[2];                  // CP 0x30 / 0x40 and XF 0x1018 / 0x1019: position matrix + texture matrices 0-3 / 4-7
    u32 amb[2], mat[2], chan[4];
    u32 dimTexGen[8], postTexGen[8];
    u32 suTs0[8];                   // BP 0x30 + 2n
    u32 lpSize;                     // BP 0x22
    u32 tref[8];                    // BP 0x28 + n
    u32 tevc[16], teva[16];         // BP 0xC0 + 2n, 0xC1 + 2n
    u32 ksel[8];                    // BP 0xF6 + n
    u32 indScale[2];                // BP 0x25, 0x26
    u32 texFlush;                   // BP 0x0F
    u32 cmode0, cmode1, zmode, peCtrl, genMode;
};
Sh sh;

void Init() {
    if (sh.inited) return;
    sh.inited = true;
    for (u32 i = 0; i < 8; i++) { sh.suTs0[i] = (0x30u + 2 * i) << 24; sh.tref[i] = (0x28u + i) << 24; sh.ksel[i] = (0xF6u + i) << 24; }
    for (u32 i = 0; i < 16; i++) { sh.tevc[i] = (0xC0u + 2 * i) << 24; sh.teva[i] = (0xC1u + 2 * i) << 24; }
    sh.lpSize = 0x22u << 24;
    sh.indScale[0] = 0x25u << 24; sh.indScale[1] = 0x26u << 24;
    sh.texFlush = 0x0Fu << 24;
    sh.cmode0 = 0x41u << 24; sh.cmode1 = 0x42u << 24; sh.zmode = 0x40u << 24; sh.peCtrl = 0x43u << 24;
    sh.genMode = 0;                                         // BP register 0
}

const u8 kChanToReg[10] = {0, 1, 0, 1, 0, 1, 7, 5, 6, 0};   // 0x80398528: colour channel -> raster colour select

void ChanColor(s32 chan, const u8* color, u32* store, u32 dirtyBase) {
    u32 idx, word;
    switch (chan) {
    case 0: idx = 0; word = Rlwimi(store[0], Be32At(color), 0, 0, 23); break;
    case 1: idx = 1; word = Rlwimi(store[1], Be32At(color), 0, 0, 23); break;
    case 2: idx = 0; word = Rlwimi(store[0], color[3], 0, 24, 31); break;
    case 3: idx = 1; word = Rlwimi(store[1], color[3], 0, 24, 31); break;
    case 4: idx = 0; word = Be32At(color); break;
    case 5: idx = 1; word = Be32At(color); break;
    default: return;
    }
    sh.dirty |= dirtyBase << idx;
    store[idx] = word;
}
}  // namespace

// The SDK's __GXSetDirtyState, without the parts the port does not need (texture state: the
// renderer reads the textures from the BP registers; viewport and projection: written at once).
extern "C" void PortGxFlushState(void) {
    Init();
    u32 d = sh.dirty;
    if (!d) return;
    if (d & DIRTY_GENMODE) Bp(sh.genMode);
    if (d & (DIRTY_AMBIENT << 0)) Xf1(0x100A, sh.amb[0]);
    if (d & (DIRTY_AMBIENT << 1)) Xf1(0x100B, sh.amb[1]);
    if (d & (DIRTY_MATERIAL << 0)) Xf1(0x100C, sh.mat[0]);
    if (d & (DIRTY_MATERIAL << 1)) Xf1(0x100D, sh.mat[1]);
    if (d & DIRTY_NUMCHANS) Xf1(0x1009, (sh.genMode >> 4) & 7);
    for (u32 i = 0; i < 4; i++) if (d & (DIRTY_CHAN << i)) Xf1(0x100E + i, sh.chan[i]);
    if (d & DIRTY_NUMTEXGENS) Xf1(0x103F, sh.genMode & 0xF);
    for (u32 i = 0; i < 8; i++) {
        if (!(d & (DIRTY_TEXGEN << i))) continue;
        Xf1(0x1040 + i, sh.dimTexGen[i]);
        Xf1(0x1050 + i, sh.postTexGen[i]);
    }
    if (d & DIRTY_MTXIDX) {                                 // __GXSetMatrixIndex(0) and (5)
        Cp(0x30, sh.mtxIdx[0]); Xf1(0x1018, sh.mtxIdx[0]);
        Cp(0x40, sh.mtxIdx[1]); Xf1(0x1019, sh.mtxIdx[1]);
    }
    sh.dirty = 0;
}

extern "C" {

// ------------------------------------------------------------------ pixel pipeline
// GXSetZMode(compare on, function, update on)
void GXSetZMode(s32 enable, s32 func, s32 update) {
    Init();
    u32 w = sh.zmode;
    w = Rlwimi(w, (u32)enable, 0, 31, 31);
    w = Rlwimi(w, (u32)func, 1, 28, 30);
    w = Rlwimi(w, (u32)update, 4, 27, 27);
    sh.zmode = w; Bp(w);
    gPortGx.zEnable = (u8)enable; gPortGx.zFunc = (u8)func; gPortGx.zUpdate = (u8)update;
}
// GXSetBlendMode(type, source factor, destination factor, logic op)
void GXSetBlendMode(s32 type, s32 src, s32 dst, s32 op) {
    Init();
    u32 w = sh.cmode0;
    w = Rlwimi(w, (u32)(type == 3), 11, 20, 20);            // subtract
    w = Rlwimi(w, (u32)type, 0, 31, 31);                    // blend (and subtract)
    w = Rlwimi(w, (u32)(type == 2), 1, 30, 30);             // logic op
    w = Rlwimi(w, (u32)op, 12, 16, 19);
    w = Rlwimi(w, (u32)src, 8, 21, 23);
    w = Rlwimi(w, (u32)dst, 5, 24, 26);
    sh.cmode0 = w; Bp(w);
    gPortGx.blend[0] = (u8)type; gPortGx.blend[1] = (u8)src; gPortGx.blend[2] = (u8)dst; gPortGx.blend[3] = (u8)op;
}
// GXSetColorUpdate
void GXSetColorUpdate(s32 on) { Init(); sh.cmode0 = Rlwimi(sh.cmode0, (u32)on, 3, 28, 28); Bp(sh.cmode0); gPortGx.colorUpdate = (u8)on; }
// GXSetAlphaUpdate
void GXSetAlphaUpdate(s32 on) { Init(); sh.cmode0 = Rlwimi(sh.cmode0, (u32)on, 4, 27, 27); Bp(sh.cmode0); gPortGx.alphaUpdate = (u8)on; }
// GXSetDither
void GXSetDither(u32 on) { Init(); sh.cmode0 = Rlwimi(sh.cmode0, on, 2, 29, 29); Bp(sh.cmode0); gPortGx.dither = (u8)on; }
// GXSetZCompLoc(depth test before texturing)
void GXSetZCompLoc(s32 beforeTex) { Init(); sh.peCtrl = Rlwimi(sh.peCtrl, (u32)beforeTex, 6, 25, 25); Bp(sh.peCtrl); gPortGx.zCompBeforeTex = (u8)beforeTex; }
// GXSetDstAlpha(on, alpha)
void GXSetDstAlpha(s32 enable, s32 alpha) {
    Init();
    u32 w = sh.cmode1;
    w = Rlwimi(w, (u32)alpha, 0, 24, 31);
    w = Rlwimi(w, (u32)enable, 8, 23, 23);
    Bp(w); sh.cmode1 = w;
    gPortGx.dstAlphaOn = (u8)enable; gPortGx.dstAlpha = (u8)alpha;
}
// GXSetPixelFmt(pixel format, z format). 0x80398550 maps the pixel format to the BP 0x43 field
void GXSetPixelFmt(s32 pixFmt, s32 zFmt) {
    static const u32 kPeFmt[8] = {0, 1, 2, 3, 4, 4, 4, 5};
    Init();
    u32 fmt = kPeFmt[pixFmt & 7];
    u32 old = sh.peCtrl;
    u32 w = Rlwimi(old, fmt, 0, 29, 31);
    w = Rlwimi(w, (u32)zFmt, 3, 26, 28);
    sh.peCtrl = w;
    if (old != w) {
        Bp(w);
        sh.genMode = Rlwimi(sh.genMode, pixFmt == 2 ? 32u : 0u, 4, 22, 22);   // the asm's cntlzw trick: set for format 2
        sh.dirty |= DIRTY_GENMODE;
    }
    if (fmt == 4) {
        u32 c = sh.cmode1;
        c = Rlwimi(c, (u32)(pixFmt - 4), 9, 21, 22);
        c = Rlwimi(c, 0x42, 24, 0, 7);
        sh.cmode1 = c; Bp(c);
    }
    gPortGx.pixelFmt = pixFmt; gPortGx.zFmt = zFmt;
}
// GXSetAlphaCompare(comp0, ref0, op, comp1, ref1)
void GXSetAlphaCompare(s32 comp0, s32 ref0, s32 op, s32 comp1, s32 ref1) {
    u32 w = 0xF3000000u;
    w = Rlwimi(w, (u32)ref0, 0, 24, 31);
    w = Rlwimi(w, (u32)ref1, 8, 16, 23);
    w = Rlwimi(w, (u32)comp0, 16, 13, 15);
    w = Rlwimi(w, (u32)comp1, 19, 10, 12);
    w = Rlwimi(w, (u32)op, 22, 8, 9);
    Bp(w);
    gPortGx.alphaComp[0] = (u8)comp0; gPortGx.alphaComp[1] = (u8)ref0; gPortGx.alphaComp[2] = (u8)op;
    gPortGx.alphaComp[3] = (u8)comp1; gPortGx.alphaComp[4] = (u8)ref1;
}
// GXSetZTexture(op, format, bias)
void GXSetZTexture(s32 op, s32 fmt, s32 bias) {
    u32 w0 = Rlwimi(0, (u32)bias, 0, 8, 31);
    w0 = Rlwimi(w0, 0xF4, 24, 0, 7);
    u32 type = fmt == 0x11 ? 0u : fmt == 0x13 ? 1u : 2u;
    u32 w1 = Rlwimi(0, type, 0, 30, 31);
    w1 = Rlwimi(w1, (u32)op, 2, 28, 29);
    w1 = Rlwimi(w1, 0xF5, 24, 0, 7);
    Bp(w0);
    Bp(w1);
}
// GXSetFog(type, colour, start z, end z, near z, far z)
void GXSetFog(s32 type, u8* color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ) {
    u32 ortho = ((u32)type >> 3) & 1, kind = (u32)type & 7;
    f32 a, c;
    u32 fog1 = 0, fog2 = 0;                                 // the orthographic case sends two zero words (as the asm does)
    if (ortho) {
        if (farZ == nearZ || endZ == startZ) { a = 0.0f; c = 0.0f; }
        else {
            f32 t = 1.0f / (endZ - startZ);
            a = t * (farZ - nearZ);
            c = t * (startZ - nearZ);
        }
    } else {
        f32 b;
        s32 expn = 0;
        if (farZ == nearZ || endZ == startZ) { a = 0.0f; b = 0.5f; c = 0.0f; }
        else {
            f32 d = farZ - nearZ;
            f32 e = endZ - startZ;
            a = (farZ * nearZ) / (d * e);
            b = farZ / d;
            c = startZ / e;
        }
        while (b > 1.0) { b *= 0.5f; expn++; }
        while (b > 0.0f && b < 0.5) { b *= 2.0f; expn--; }
        a = a / (f32)(s32)(1u << (expn + 1));
        u32 bMant = (u32)(8388638.0f * b);
        fog1 = Rlwimi(0, bMant, 0, 8, 31); fog1 = Rlwimi(fog1, 0xEF, 24, 0, 7);
        fog2 = Rlwimi(0, (u32)(expn + 1), 0, 27, 31); fog2 = Rlwimi(fog2, 0xF0, 24, 0, 7);
    }
    u32 ab = BitsOf(a), cb = BitsOf(c);
    u32 fog0 = Rlwimi(0, ab, 20, 21, 31);
    fog0 = Rlwimi(fog0, ab, 20, 13, 20);
    fog0 = Rlwimi(fog0, ab, 20, 12, 12);
    fog0 = Rlwimi(fog0, 0xEE, 24, 0, 7);
    u32 fog3 = Rlwimi(0, cb, 20, 21, 31);
    fog3 = Rlwimi(fog3, cb, 20, 13, 20);
    fog3 = Rlwimi(fog3, cb, 20, 12, 12);
    fog3 = Rlwimi(fog3, ortho, 20, 11, 11);
    fog3 = Rlwimi(fog3, kind, 21, 8, 10);
    fog3 = Rlwimi(fog3, 0xF1, 24, 0, 7);
    u32 fog4 = Rlwimi(0, Be32At(color), 24, 8, 31);
    fog4 = Rlwimi(fog4, 0xF2, 24, 0, 7);
    Bp(fog0); Bp(fog1); Bp(fog2); Bp(fog3); Bp(fog4);
    gPortGx.fogType = type;
    memcpy(gPortGx.fogColor, color, 4);
    gPortGx.fog[0] = startZ; gPortGx.fog[1] = endZ; gPortGx.fog[2] = nearZ; gPortGx.fog[3] = farZ;
}
// GXSetFogRangeAdj(on, centre, table: ten u16 values)
void GXSetFogRangeAdj(s32 enable, s32 center, void* table) {
    if (enable) {
        const u16* t = (const u16*)table;
        for (u32 i = 0; i < 5; i++) {
            u32 w = Rlwimi(0, t[2 * i], 0, 20, 31);
            w = Rlwimi(w, t[2 * i + 1], 12, 8, 19);
            w = Rlwimi(w, 0xE9 + i, 24, 0, 7);
            Bp(w);
        }
    }
    u32 w = Rlwimi(0, (u32)(center + 342), 0, 22, 31);
    w = Rlwimi(w, (u32)enable, 10, 21, 21);
    w = Rlwimi(w, 0xE8, 24, 0, 7);
    Bp(w);
}
// GXSetFieldMask(odd lines, even lines)
void GXSetFieldMask(s32 odd, s32 even) {
    u32 w = Rlwimi(0, (u32)even, 0, 31, 31);
    w = Rlwimi(w, (u32)odd, 1, 30, 30);
    w = Rlwimi(w, 0x44, 24, 0, 7);
    Bp(w);
}
// GXSetFieldMode(field rendering, half-height aspect)
void GXSetFieldMode(s32 fieldMode, s32 halfAspect) {
    Init();
    sh.lpSize = Rlwimi(sh.lpSize, (u32)halfAspect, 22, 9, 9);
    Bp(sh.lpSize);
    Bp(sh.texFlush);                                        // __GXFlushTextureState
    Bp((u32)fieldMode | 0x68000000u);
    Bp(sh.texFlush);
}

// ------------------------------------------------------------------ geometry
// GXSetTexCoordGen2(coord, function, source, matrix, normalize, post-transform matrix)
void GXSetTexCoordGen2(s32 coord, s32 func, s32 src, s32 mtx, s32 normalize, s32 postMtx) {
    Init();
    // 0x8039829C: per texture-generation source, the row of the source and whether it is a geometry row
    static const u8 kRow[21] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 5, 5, 5, 5, 5, 5, 5, 2, 2};
    static const u8 kIsGeom[21] = {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u32 row = 5, isGeom = 0;
    if ((u32)src <= 0x14) { row = kRow[src]; isGeom = kIsGeom[src]; }
    u32 w = 0;
    if (func == 1) {
        w = Rlwimi(w, isGeom, 2, 29, 29);
        w = Rlwimi(w, row, 7, 20, 24);
    } else if (func == 0) {
        w = 2;
        w = Rlwimi(w, isGeom, 2, 29, 29);
        w = Rlwimi(w, row, 7, 20, 24);
    } else if (func >= 2 && func < 10) {                    // bump mapping from a light's texture coordinate
        w = Rlwimi(w, isGeom, 2, 29, 29);
        w = Rlwimi(w, 1, 4, 25, 27);
        w = Rlwimi(w, row, 7, 20, 24);
        w = Rlwimi(w, (u32)(src - 12), 12, 17, 19);
        w = Rlwimi(w, (u32)(func - 2), 15, 14, 16);
    } else if (func == 10) {
        w = Rlwimi(w, isGeom, 2, 29, 29);
        w = Rlwimi(w, src == 0x13 ? 2u : 3u, 4, 25, 27);
        w = Rlwimi(w, 2, 7, 20, 24);
    }
    u32 c = (u32)coord & 7;
    sh.dimTexGen[c] = w;
    u32 p = Rlwimi(0, (u32)(postMtx - 0x40), 0, 26, 31);
    p = Rlwimi(p, (u32)normalize, 8, 23, 23);
    sh.postTexGen[c] = p;
    sh.dirty |= DIRTY_TEXGEN << c;
    // the matrix index of this coordinate, in CP 0x30 (coordinates 0-3) and 0x40 (4-7)
    static const struct { u8 word, sh, mb, me; } kIdx[8] = {
        {0, 6, 20, 25}, {0, 12, 14, 19}, {0, 18, 8, 13}, {0, 24, 2, 7},
        {1, 0, 26, 31}, {1, 6, 20, 25}, {1, 12, 14, 19}, {1, 18, 8, 13}};
    const auto& k = kIdx[c];
    sh.mtxIdx[k.word] = Rlwimi(sh.mtxIdx[k.word], (u32)mtx, k.sh, k.mb, k.me);
    sh.dirty |= DIRTY_MTXIDX;
    if ((u32)coord < 8) {
        gPortGx.texGen[coord].func = (u8)func; gPortGx.texGen[coord].src = (u8)src; gPortGx.texGen[coord].mtx = (u8)mtx;
        gPortGx.texGen[coord].normalize = (u8)normalize; gPortGx.texGen[coord].postMtx = (u8)postMtx;
    }
}
// GXSetNumTexGens
void GXSetNumTexGens(s32 count) {
    Init();
    sh.genMode = Rlwimi(sh.genMode, (u32)count, 0, 28, 31);
    sh.dirty |= DIRTY_NUMTEXGENS | DIRTY_GENMODE;
    gPortGx.numTexGens = (u8)count;
}
// GXSetLineWidth(width in 1/6 pixels, texture offset)
void GXSetLineWidth(s32 width, s32 texOffsets) {
    Init();
    sh.lpSize = Rlwimi(sh.lpSize, (u32)width, 0, 24, 31);
    sh.lpSize = Rlwimi(sh.lpSize, (u32)texOffsets, 16, 13, 15);
    Bp(sh.lpSize);
    gPortGx.lineWidth = (u8)width;
}
// GXSetPointSize
void GXSetPointSize(s32 size, s32 texOffsets) {
    Init();
    sh.lpSize = Rlwimi(sh.lpSize, (u32)size, 8, 16, 23);
    sh.lpSize = Rlwimi(sh.lpSize, (u32)texOffsets, 19, 10, 12);
    Bp(sh.lpSize);
    gPortGx.pointSize = (u8)size;
}
// GXEnableTexOffsets(coord, for lines, for points)
void GXEnableTexOffsets(s32 coord, s32 line, s32 point) {
    Init();
    u32 i = (u32)coord & 7;
    sh.suTs0[i] = Rlwimi(sh.suTs0[i], (u32)line, 18, 13, 13);
    sh.suTs0[i] = Rlwimi(sh.suTs0[i], (u32)point, 19, 12, 12);
    Bp(sh.suTs0[i]);
}
// GXSetCoPlanar
void GXSetCoPlanar(s32 enable) {
    Init();
    sh.genMode = Rlwimi(sh.genMode, (u32)enable, 19, 12, 12);
    Bp(0xFE080000u);                                        // write mask: only that bit
    Bp(sh.genMode);
}
// GXSetCullMode(0 none, 1 front, 2 back, 3 all)
void GXSetCullMode(s32 mode) {
    Init();
    u32 m = ((u32)mode >> 1) & 1;
    m = Rlwimi(m, (u32)mode, 1, 30, 30);
    sh.genMode = Rlwimi(sh.genMode, m, 14, 16, 17);
    sh.dirty |= DIRTY_GENMODE;
    gPortGx.cullMode = (u8)mode;
}
// GXSetClipMode(0 = clipping on)
void GXSetClipMode(s32 mode) { Xf1(0x1005, (u32)mode); gPortGx.clipMode = (u8)mode; }
// GXSetScissorBoxOffset
void GXSetScissorBoxOffset(s32 x, s32 y) {
    u32 w = Rlwimi(0, (u32)(x + 0x156), 31, 22, 31);
    w = Rlwimi(w, (u32)(y + 0x156), 9, 12, 21);
    w = Rlwimi(w, 0x59, 24, 0, 7);
    Bp(w);
    gPortGx.scissorOffset[0] = x; gPortGx.scissorOffset[1] = y;
}
// GXSetCurrentMtx
void GXSetCurrentMtx(s32 id) {
    Init();
    sh.mtxIdx[0] = Rlwimi(sh.mtxIdx[0], (u32)id, 0, 26, 31);
    sh.dirty |= DIRTY_MTXIDX;
    gPortGx.currentMtx = (u8)id;
}

// ------------------------------------------------------------------ lighting channels
// GXSetNumChans
void GXSetNumChans(s32 count) {
    Init();
    sh.genMode = Rlwimi(sh.genMode, (u32)count, 4, 25, 27);
    sh.dirty |= DIRTY_NUMCHANS | DIRTY_GENMODE;
    gPortGx.numChans = (u8)count;
}
// GXSetChanCtrl(channel, lighting on, ambient source, material source, light mask, diffuse fn, attenuation fn)
void GXSetChanCtrl(s32 chan, s32 enable, s32 ambSrc, s32 matSrc, s32 lightMask, s32 diffFn, s32 attnFn) {
    Init();
    u32 w = Rlwimi(0, (u32)enable, 1, 30, 30);
    w = Rlwimi(w, (u32)matSrc, 0, 31, 31);
    w = Rlwimi(w, (u32)ambSrc, 6, 25, 25);
    u32 diff = attnFn == 0 ? 0u : (u32)diffFn;
    w = Rlwimi(w, diff, 7, 23, 24);
    w = Rlwimi(w, (u32)(2 - attnFn) | (u32)(attnFn - 2), 10, 22, 22);
    w = Rlwimi(w, (u32)(-attnFn) | (u32)attnFn, 11, 21, 21);
    w = Rlwimi(w, (u32)lightMask, 2, 26, 29);
    w = Rlwimi(w, (u32)lightMask, 7, 17, 20);
    u32 i = (u32)chan & 3;
    sh.chan[i] = w;
    sh.dirty |= DIRTY_CHAN << i;
    if (chan == 4) { sh.chan[2] = w; sh.dirty |= 0x5000; }
    else if (chan == 5) { sh.chan[3] = w; sh.dirty |= 0xA000; }
}
// GXSetChanAmbColor(channel, colour)
void GXSetChanAmbColor(s32 chan, u8* color) { Init(); ChanColor(chan, color, sh.amb, DIRTY_AMBIENT); }
// GXSetChanMatColor(channel, colour)
void GXSetChanMatColor(s32 chan, u8* color) { Init(); ChanColor(chan, color, sh.mat, DIRTY_MATERIAL); }

// ------------------------------------------------------------------ TEV
// GXSetTevOrder(stage, texture coordinate, texture map, colour channel)
void GXSetTevOrder(s32 stage, s32 coord, s32 map, s32 color) {
    Init();
    u32 tmap = (u32)map & ~0x100u;
    if (tmap >= 8) tmap = 0;
    if (coord >= 8) coord = 0;                              // GX_TEXCOORD_NULL
    u32 chanSel = color == 0xFF ? 7u : (u32)color < 10 ? kChanToReg[color] : 0u;
    u32 enable = (map != 0xFF && !(map & 0x100)) ? 1u : 0u;
    u32 i = ((u32)stage >> 1) & 7;
    u32 w = sh.tref[i];
    if (stage & 1) {
        w = Rlwimi(w, tmap, 12, 17, 19);
        w = Rlwimi(w, (u32)coord, 15, 14, 16);
        w = Rlwimi(w, chanSel, 19, 10, 12);
        w = Rlwimi(w, enable, 18, 13, 13);
    } else {
        w = Rlwimi(w, tmap, 0, 29, 31);
        w = Rlwimi(w, (u32)coord, 3, 26, 28);
        w = Rlwimi(w, chanSel, 7, 22, 24);
        w = Rlwimi(w, enable, 6, 25, 25);
    }
    sh.tref[i] = w;
    Bp(w);
}
// GXSetNumTevStages
void GXSetNumTevStages(s32 count) {
    Init();
    sh.genMode = Rlwimi(sh.genMode, (u32)(count - 1), 10, 18, 21);
    sh.dirty |= DIRTY_GENMODE;
    gPortGx.numTevStages = (u8)count;
}
// GXSetTevOp(stage, mode). 0x803984D8: colour inputs for stage 0 / later stages, then alpha inputs
void GXSetTevOp(s32 stage, s32 mode) {
    static const u32 kTable[20] = {
        0xC008F8AF, 0xC008A89F, 0xC008AC8F, 0xC008FFF8, 0xC008FFFA,
        0xC008F80F, 0xC008089F, 0xC0080C8F, 0xC008FFF8, 0xC008FFF0,
        0xC108F2F0, 0xC108FFD0, 0xC108F2F0, 0xC108FFC0, 0xC108FFD0,
        0xC108F070, 0xC108FF80, 0xC108F070, 0xC108FFC0, 0xC108FF80};
    Init();
    const u32* col = kTable + (stage == 0 ? 0 : 5);
    const u32* alp = kTable + (stage == 0 ? 10 : 15);
    u32 i = (u32)stage & 15;
    u32 c = Rlwimi(sh.tevc[i] & 0xFF000000u, col[mode], 0, 8, 31);
    Bp(c); sh.tevc[i] = c;
    u32 a = Rlwimi(sh.teva[i] & Mask(28, 7), alp[mode], 0, 8, 27);
    Bp(a); sh.teva[i] = a;
}
// GXSetTevKColorSel(stage, selection)
void GXSetTevKColorSel(s32 stage, s32 sel) {
    Init();
    u32 i = ((u32)stage >> 1) & 7;
    sh.ksel[i] = (stage & 1) ? Rlwimi(sh.ksel[i], (u32)sel, 14, 13, 17) : Rlwimi(sh.ksel[i], (u32)sel, 4, 23, 27);
    Bp(sh.ksel[i]);
}
// GXSetTevKAlphaSel(stage, selection)
void GXSetTevKAlphaSel(s32 stage, s32 sel) {
    Init();
    u32 i = ((u32)stage >> 1) & 7;
    sh.ksel[i] = (stage & 1) ? Rlwimi(sh.ksel[i], (u32)sel, 19, 8, 12) : Rlwimi(sh.ksel[i], (u32)sel, 9, 18, 22);
    Bp(sh.ksel[i]);
}
// GXSetTevSwapMode(stage, rasterised colour table, texture colour table)
void GXSetTevSwapMode(s32 stage, s32 rasSel, s32 texSel) {
    Init();
    u32 i = (u32)stage & 15;
    sh.teva[i] = Rlwimi(sh.teva[i], (u32)rasSel, 0, 30, 31);
    sh.teva[i] = Rlwimi(sh.teva[i], (u32)texSel, 2, 28, 29);
    Bp(sh.teva[i]);
}
// GXSetTevSwapModeTable(table, r, g, b, a)
void GXSetTevSwapModeTable(s32 table, s32 r, s32 g, s32 b, s32 a) {
    Init();
    u32 i = ((u32)table & 3) * 2;
    sh.ksel[i] = Rlwimi(sh.ksel[i], (u32)r, 0, 30, 31);
    sh.ksel[i] = Rlwimi(sh.ksel[i], (u32)g, 2, 28, 29);
    Bp(sh.ksel[i]);
    sh.ksel[i + 1] = Rlwimi(sh.ksel[i + 1], (u32)b, 0, 30, 31);
    sh.ksel[i + 1] = Rlwimi(sh.ksel[i + 1], (u32)a, 2, 28, 29);
    Bp(sh.ksel[i + 1]);
}
// GXSetTevDirect(stage): the SDK's GXSetTevIndirect(stage, 0, 0, 0, 0, 0, 0, 0, 0, 0), BP 0x10 + stage
void GXSetTevDirect(s32 stage) { Bp((0x10u + (u32)stage) << 24); }
// GXSetNumIndStages
void GXSetNumIndStages(s32 count) {
    Init();
    sh.genMode = Rlwimi(sh.genMode, (u32)count, 16, 13, 15);
    sh.dirty |= DIRTY_GENMODE | 2;
}
// GXSetIndTexCoordScale(indirect stage, scale s, scale t)
void GXSetIndTexCoordScale(s32 stage, s32 scaleS, s32 scaleT) {
    Init();
    u32* r = &sh.indScale[(stage >> 1) & 1];
    switch (stage) {
    case 0: case 2: *r = Rlwimi(Rlwimi(*r, (u32)scaleS, 0, 28, 31), (u32)scaleT, 4, 24, 27); break;
    case 1: case 3: *r = Rlwimi(Rlwimi(*r, (u32)scaleS, 8, 20, 23), (u32)scaleT, 12, 16, 19); break;
    default: return;
    }
    Bp(*r);
}


// ---- texture objects (GXTexObj, 0x20 bytes: mode0, mode1, image0, image3 as BP words, then our own fields)
// GXInitTexObj(obj, image, width, height, format, wrap s, wrap t, mipmap)
void GXInitTexObj(u8* obj, void* image, u16 width, u16 height, s32 fmt, s32 wrapS, s32 wrapT, s32 mipmap) {
    u32* t = (u32*)obj;
    u32 mode0 = ((u32)wrapS & 3) | (((u32)wrapT & 3) << 2) | (1u << 4);   // mag filter linear
    mode0 |= (mipmap ? 6u : 1u) << 5;                                       // min filter
    mode0 |= 1u << 8;                                                       // edge LOD
    t[0] = mode0 | (0x80u << 24);
    t[1] = 0xFF00u | (0x84u << 24);                                         // LOD 0..15.9375
    t[2] = ((u32)(width - 1) & 0x3FF) | (((u32)(height - 1) & 0x3FF) << 10) | (((u32)fmt & 15) << 20) | (0x88u << 24);
    t[3] = (((u32)(uintptr_t)image & 0x3FFFFFFF) >> 5) | (0x94u << 24);
    t[4] = 0; t[5] = (u32)fmt; t[6] = 0xFFFFFFFFu; t[7] = mipmap ? 1 : 0;
}
// GXInitTexObjLOD(obj, min filter, mag filter, bias clamp, edge LOD, max anisotropy, min LOD, max LOD, LOD bias):
// the integers come first as the callers declare it (the floats travel in their own registers on the console)
void GXInitTexObjLOD(u8* obj, s32 minFilt, s32 magFilt, s32 biasClamp, s32 edgeLod, s32 aniso, f32 minLod, f32 maxLod, f32 bias) {
    static const u8 kMin[6] = {0, 2, 4, 1, 3, 5};
    u32* t = (u32*)obj;
    s32 b = (s32)(32.0f * bias);
    b = b < -128 ? -128 : b > 127 ? 127 : b;
    u32 m0 = t[0] & ~(0x1u << 4) & ~(7u << 5) & ~(1u << 8) & ~(0xFFu << 9) & ~(3u << 19) & ~(1u << 21);
    m0 |= ((u32)(magFilt == 1) << 4) | ((u32)kMin[(u32)minFilt % 6] << 5) | ((u32)(edgeLod ? 0 : 1) << 8) |
          (((u32)b & 0xFF) << 9) | (((u32)aniso & 3) << 19) | ((u32)(biasClamp != 0) << 21);
    t[0] = m0;
    f32 lo = minLod < 0 ? 0 : minLod > 10 ? 10 : minLod, hi = maxLod < 0 ? 0 : maxLod > 10 ? 10 : maxLod;
    t[1] = ((u32)(16.0f * lo) & 0xFF) | (((u32)(16.0f * hi) & 0xFF) << 8) | (0x84u << 24);
}
// GXLoadTexObj(obj, map): the object's four BP registers for that texture map
void GXLoadTexObj(u8* obj, s32 map) {
    Init();
    const u32* t = (const u32*)obj;
    u32 id = (u32)map & 7, base = id < 4 ? 0x80 : 0xA0, n = id & 3;
    Bp((t[0] & 0xFFFFFFu) | ((base + n) << 24));
    Bp((t[1] & 0xFFFFFFu) | ((base + 4 + n) << 24));
    Bp((t[2] & 0xFFFFFFu) | ((base + 8 + n) << 24));
    Bp((t[3] & 0xFFFFFFu) | ((base + 0x14 + n) << 24));
}


// ---- TEV inputs and operations (the SDK's GXSetTevColorIn / AlphaIn / ColorOp / AlphaOp, under their unnamed symbols)
// fn_8024D59C: GXSetTevColorIn(stage, a, b, c, d)
void fn_8024D59C(s32 stage, s32 a, s32 b, s32 c, s32 d) {
    Init();
    u32 i = (u32)stage & 15;
    u32 r = (sh.tevc[i] & ~0xFFFFu) | (((u32)a & 15) << 12) | (((u32)b & 15) << 8) | (((u32)c & 15) << 4) | ((u32)d & 15);
    sh.tevc[i] = r;
    Bp(r);
}
// fn_8024D5DC: GXSetTevAlphaIn(stage, a, b, c, d); the low four bits are the swap tables
void fn_8024D5DC(s32 stage, s32 a, s32 b, s32 c, s32 d) {
    Init();
    u32 i = (u32)stage & 15;
    u32 r = (sh.teva[i] & ~0xFFF0u) | (((u32)a & 7) << 13) | (((u32)b & 7) << 10) | (((u32)c & 7) << 7) | (((u32)d & 7) << 4);
    sh.teva[i] = r;
    Bp(r);
}
static u32 TevOp(u32 r, s32 op, s32 bias, s32 scale, s32 clamp, s32 out) {
    r = (r & ~(1u << 18)) | (((u32)op & 1) << 18);
    r = (r & ~(3u << 16)) | (((op <= 1) ? (u32)bias : 3u) << 16);
    r = (r & ~(3u << 20)) | (((op <= 1) ? (u32)scale : (((u32)op >> 1) & 3)) << 20);
    r = (r & ~(1u << 19)) | (((u32)clamp & 1) << 19);
    r = (r & ~(3u << 22)) | (((u32)out & 3) << 22);
    return r;
}
// fn_8024D61C: GXSetTevColorOp(stage, op, bias, scale, clamp, output register)
void fn_8024D61C(s32 stage, s32 op, s32 bias, s32 scale, s32 clamp, s32 out) {
    Init();
    u32 i = (u32)stage & 15;
    sh.tevc[i] = TevOp(sh.tevc[i], op, bias, scale, clamp, out);
    Bp(sh.tevc[i]);
}
// fn_8024D674: GXSetTevAlphaOp
void fn_8024D674(s32 stage, s32 op, s32 bias, s32 scale, s32 clamp, s32 out) {
    Init();
    u32 i = (u32)stage & 15;
    sh.teva[i] = TevOp(sh.teva[i], op, bias, scale, clamp, out);
    Bp(sh.teva[i]);
}
// fn_8024D72C: GXSetTevColorS10(register, four signed 16-bit values r, g, b, a)
void fn_8024D72C(s32 reg, s16* c) {
    Init();
    u32 id = (u32)reg & 3;
    Bp(((u32)c[0] & 0x7FF) | (((u32)c[3] & 0x7FF) << 12) | ((0xE0u + id * 2) << 24));
    u32 bg = ((u32)c[2] & 0x7FF) | (((u32)c[1] & 0x7FF) << 12) | ((0xE1u + id * 2) << 24);
    Bp(bg); Bp(bg); Bp(bg);
}
// GXSetTevKColor(id, colour bytes r, g, b, a): the constant colour registers share the addresses of the
// colour registers, the type bit (23) tells them apart
void GXSetTevKColor(s32 id, u8* c) {
    Init();
    u32 n = (u32)id & 3;
    Bp((u32)c[0] | ((u32)c[3] << 12) | (8u << 20) | ((0xE0u + n * 2) << 24));
    u32 bg = (u32)c[2] | ((u32)c[1] << 12) | (8u << 20) | ((0xE1u + n * 2) << 24);
    Bp(bg); Bp(bg); Bp(bg);
}

// ---- indirect texturing
// GXSetIndTexOrder(indirect stage, texture coordinate, texture map): BP 0x27
static u32 sIref;
void GXSetIndTexOrder(s32 stage, s32 coord, s32 map) {
    Init();
    u32 sh6 = ((u32)stage & 3) * 6;
    sIref = (sIref & ~(7u << sh6) & ~(7u << (sh6 + 3))) | (((u32)map & 7) << sh6) | (((u32)coord & 7) << (sh6 + 3));
    Bp((sIref & 0xFFFFFFu) | (0x27u << 24));
    sh.dirty |= 2;
}
// GXSetTevIndirect(tev stage, indirect stage, format, bias, matrix, wrap s, wrap t, add previous, use LOD, alpha select)
void GXSetTevIndirect(s32 tev, s32 ind, s32 fmt, s32 bias, s32 mtx, s32 wrapS, s32 wrapT, s32 addPrev, s32 lod, s32 alpha) {
    Init();
    u32 r = ((u32)ind & 3) | (((u32)fmt & 3) << 2) | (((u32)bias & 7) << 4) | (((u32)alpha & 3) << 7) |
            (((u32)mtx & 15) << 9) | (((u32)wrapS & 7) << 13) | (((u32)wrapT & 7) << 16) |
            (((u32)lod & 1) << 19) | (((u32)addPrev & 1) << 20) | ((0x10u + (u32)tev) << 24);
    Bp(r);
}
// GXSetIndTexMtx(matrix id 1..3 (or 5..7, 9..11), 2x3 matrix, scale exponent)
void GXSetIndTexMtx(s32 id, f32* m, s32 scaleExp) {
    Init();
    u32 n = id == 1 || id == 5 || id == 9 ? 0 : id == 2 || id == 6 || id == 10 ? 1 : 2;
    u32 e = (u32)(scaleExp + 17);
    auto q = [](f32 v) { return (u32)(s32)(1024.0f * v) & 0x7FF; };
    for (u32 k = 0; k < 3; k++) {
        u32 r = q(m[k]) | (q(m[3 + k]) << 11) | (((e >> (2 * k)) & 3) << 22) | ((6u + n * 3 + k) << 24);
        Bp(r);
    }
}

// ---- draw sync
static u16 sDrawToken;
// GXSetDrawSync(token): the token goes through the pipe and interrupts when the processor reaches it
void GXSetDrawSync(u16 token) {
    Init();
    sDrawToken = token;
    Bp(0x48000000u | token);
    Bp(0x47000000u | token);
}
// GXReadDrawSync: the last token the processor passed
s32 GXReadDrawSync(void) { PortGxFinish(); return sDrawToken; }   // (the token the GP passed: once it has caught up, the last one set)

// ---- projection
void PortGxWriteF32(f32 v);
// GXSetProjection(4x4 matrix, type: 0 perspective, 1 orthographic): XF registers 0x1020..0x1026
void GXSetProjection(f32* m, s32 type) {
    Init();
    PortGxWrite8(0x10);
    PortGxWrite32((6u << 16) | 0x1020);
    PortGxWriteF32(m[0]);
    PortGxWriteF32(type == 1 ? m[3] : m[2]);
    PortGxWriteF32(m[5]);
    PortGxWriteF32(type == 1 ? m[7] : m[6]);
    PortGxWriteF32(m[10]);
    PortGxWriteF32(m[11]);
    PortGxWrite32((u32)type);
}
// C_MTXOrtho(matrix, top, bottom, left, right, near, far)
void C_MTXOrtho(f32* m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f) {
    f32 tmp = 1.0f / (r - l);
    m[0] = 2.0f * tmp; m[1] = 0; m[2] = 0; m[3] = -(r + l) * tmp;
    tmp = 1.0f / (t - b);
    m[4] = 0; m[5] = 2.0f * tmp; m[6] = 0; m[7] = -(t + b) * tmp;
    tmp = 1.0f / (f - n);
    m[8] = 0; m[9] = 0; m[10] = -tmp; m[11] = -f * tmp;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1.0f;
}

}  // extern "C"
#endif
