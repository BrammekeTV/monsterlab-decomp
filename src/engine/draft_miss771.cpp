// GX render-state flush: fn_8017B9BC (0x8017B9BC..0x8017D840).
// Takes the wanted render-state record (cur) and the state the GPU already has (prev) and writes
// only what changed into the GX write-gather pipe: XF matrices, TEV/texture setup, colour
// channels, light registers, TEV stages, tex-coord generators, fog/alpha-compare/z registers.
#include "types.h"
#include "revolution.h"
#include <math.h>

extern "C" {
extern u8  lbl_80348950[];    // 0x80348950
extern u8  lbl_80348A10[];    // 0x80348A10
extern u32 lbl_80348AD0;      // 0x80348AD0 (u32 table, indexed by the texture format byte)
extern s32 lbl_80348FB0[];    // 0x80348FB0 (per texture slot: base BP register of the texture-unit block)
extern s32 lbl_80348FD0[];    // 0x80348FD0 (per tev-stage pair: base BP register)
extern s32 lbl_8040ADF0[8];   // 0x8040ADF0 (cached texture image address per slot)
extern s32 lbl_8040AE10[8];   // 0x8040AE10 (cached tlut/format index per slot)
extern f32 lbl_805F9208;      // 0x805F9208
extern s32 lbl_805F920C;      // 0x805F920C
extern s32 lbl_805FB4B0;      // 0x805FB4B0 (texture load counter)
extern u8  lbl_805FB50D;      // 0x805FB50D (GX shadow flags byte)
}

// Field access on the raw state records.
static inline s32  S32At(const u8* p, u32 o) { return *(const s32*)(p + o); }
static inline u32  U32At(const u8* p, u32 o) { return *(const u32*)(p + o); }
static inline u8   U8At(const u8* p, u32 o)  { return *(const u8*)(p + o); }
static inline u16  U16At(const u8* p, u32 o) { return *(const u16*)(p + o); }
static inline f32  F32At(const u8* p, u32 o) { return *(const f32*)(p + o); }

// BP (0x61) register write.
static inline void BpWrite(u32 v) {
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = v;
}

// CP/XF (0x10) command header: word count - 1 and register address.
static inline void XfHeader(u16 lenMinus1, u16 addr) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = lenMinus1;
    GXWGFifo._u16 = addr;
}

// CP/XF register write of one data word.
static inline void XfWrite(u16 addr, u32 v) {
    XfHeader(0, addr);
    GXWGFifo._u32 = v;
}

// float -> int like fctiwz (saturating), then clamp.
static inline s32 TruncSat(f32 v) {
    if (v != v) return (s32)0x80000000;
    if (v >= 2147483648.0f) return 0x7FFFFFFF;
    if (v <= -2147483648.0f) return (s32)0x80000000;
    return (s32)v;
}

// Two 255-scaled colour components packed as (a << 12) | b. s10 mode keeps 11-bit signed
// components (-0x400..0x3FF), otherwise 8-bit (0..255).
static inline u32 PackColorPair(f32 a, f32 b, bool s10) {
    s32 lo = s10 ? -0x400 : 0;
    s32 hi = s10 ? 0x3FF : 0xFF;
    u32 mask = s10 ? 0x7FFu : 0xFFu;
    s32 ia = TruncSat(255.0f * a);
    if (ia < lo) ia = lo; else if (ia > hi) ia = hi;
    s32 ib = TruncSat(255.0f * b);
    if (ib < lo) ib = lo; else if (ib > hi) ib = hi;
    return (((u32)ia & mask) << 12) | ((u32)ib & mask);
}

// One of the four colour-channel (XF) blocks at base 0x24/0x40/0x6C/0x88.
static inline bool ChanDiffers(const u8* c, const u8* p, u32 b) {
    return U8At(p, b) != U8At(c, b) || S32At(p, b + 4) != S32At(c, b + 4) ||
           S32At(p, b + 8) != S32At(c, b + 8) || U8At(p, b + 1) != U8At(c, b + 1) ||
           S32At(p, b + 0xC) != S32At(c, b + 0xC) || S32At(p, b + 0x10) != S32At(c, b + 0x10);
}
static inline u32 ChanWord(const u8* c, u32 b) {
    u8 f1 = U8At(c, b + 1);
    return (u32)S32At(c, b + 8) | ((u32)U8At(c, b) << 1) | (((u32)(f1 & lbl_805FB50D)) << 2) |
           ((u32)S32At(c, b + 4) << 6) | ((u32)S32At(c, b + 0xC) << 7) |
           ((u32)S32At(c, b + 0x10) << 9) | (((u32)f1 << 7) & 0xFFFFF800u);
}

// Light block enable bytes: write the light address register when the light became enabled or
// its address changed.
static inline bool LightChanged(const u8* c, const u8* p, u32 flag, u32 addr) {
    return U8At(c, flag) != 0 && (U8At(p, flag) == 0 || U32At(p, addr) != U32At(c, addr));
}


extern "C" {

// 0x8017B9BC
// Flushes the render state record `cur` against `prev` (the state last sent to the GPU).
// `extra` supplies four float colour offsets (+0x20.., stride 0x10) added to the TEV register
// colours. The second argument is unused. Always returns 1.
s32 fn_8017B9BC(u8* cur, void* unused, u8* prev, u8* extra) {
    (void)unused;
    s32 i;

    lbl_805F9208 = F32At(cur, 0xCF0);
    lbl_805F920C = S32At(cur, 0xCF4);

    // Eight position matrices (3x4): sent when present, identity when only the old state had one.
    for (i = 0; i < 8; i++) {
        u8* arr = *(u8**)(cur + 0);
        u8* m = arr != NULL ? *(u8**)(arr + i * 4 + 0x3C) : NULL;
        u8* parr = *(u8**)(prev + 0);
        u8* pm = parr != NULL ? *(u8**)(parr + i * 4 + 0x3C) : NULL;
        if (m != NULL) {
            XfHeader(0xB, (u16)(i * 12) | 0x500);
            static const u32 kOrder[12] = { 0x0, 0xC, 0x18, 0x24, 0x4, 0x10, 0x1C, 0x28, 0x8, 0x14, 0x20, 0x2C };
            for (s32 k = 0; k < 12; k++) {
                GXWGFifo._f32 = F32At(m, kOrder[k]);
            }
        } else if (pm != NULL) {
            XfHeader(0xB, (u16)(i * 12) | 0x500);
            static const u32 kIdent[12] = { 0x3F800000, 0, 0, 0, 0, 0x3F800000, 0, 0, 0, 0, 0x3F800000, 0 };
            for (s32 k = 0; k < 12; k++) {
                GXWGFifo._u32 = kIdent[k];
            }
        }
    }

    // Fog/alpha-compare style register (BP 0x41, mask 0xFE00FFEB), three layouts by mode bytes.
    u8 mode0 = U8At(cur, 0xCFC);
    if (mode0 != 0) {
        if (U8At(prev, 0xCFC) != mode0 || U8At(prev, 0xD10) != U8At(cur, 0xD10) ||
            S32At(prev, 0xD00) != S32At(cur, 0xD00) || S32At(prev, 0xD08) != S32At(cur, 0xD08) ||
            S32At(prev, 0xD0C) != S32At(cur, 0xD0C)) {
            BpWrite(0xFE00FFEBu);
            BpWrite(((u32)S32At(cur, 0xD0C) << 5) | ((u32)S32At(cur, 0xD08) << 8) |
                    (((u32)S32At(cur, 0xD00) << 11) | 0x41000000u) |
                    (((u32)U8At(cur, 0xD10) * 8) | 1));
        }
    } else {
        u8 mode1 = U8At(cur, 0xCFD);
        if (mode1 != 0) {
            if (U8At(prev, 0xCFC) != mode0 || U8At(prev, 0xCFD) != mode1 ||
                U8At(prev, 0xD10) != U8At(cur, 0xD10) || S32At(prev, 0xD04) != S32At(cur, 0xD04)) {
                BpWrite(0xFE00FFEBu);
                BpWrite(((u32)S32At(cur, 0xD04) << 12) | 0x41000000u | 0x100u |
                        (((u32)U8At(cur, 0xD10) * 8) | 2));
            }
        } else if (U8At(prev, 0xCFC) != mode0 || U8At(prev, 0xCFD) != mode1 ||
                   U8At(prev, 0xD10) != U8At(cur, 0xD10)) {
            BpWrite(0xFE00FFEBu);
            BpWrite(((u32)U8At(cur, 0xD10) * 8) | 0x41000000u | 0x100u);
        }
    }

    // Raster/texgen config word (BP mask 0xFE0FFCFF).
    if (S32At(prev, 0x8) != S32At(cur, 0x8) || S32At(prev, 0x4) != S32At(cur, 0x4) ||
        S32At(prev, 0xC) != S32At(cur, 0xC) || S32At(prev, 0xD18) != S32At(cur, 0xD18) ||
        S32At(prev, 0x10) != S32At(cur, 0x10) || U8At(prev, 0xD1C) != U8At(cur, 0xD1C)) {
        BpWrite(0xFE0FFCFFu);
        BpWrite((u32)S32At(cur, 0x8) | ((u32)S32At(cur, 0x4) << 4) |
                (((u32)(S32At(cur, 0xC) - 1) << 10) | ((u32)S32At(cur, 0xD18) << 14) |
                 (((u32)U8At(cur, 0xD1C) << 19) | ((u32)S32At(cur, 0x10) << 16))));
    }

    // Z mode / cull style register (BP 0x40).
    u8 zMode = U8At(cur, 0xD12);
    if (zMode != 0) {
        if (U8At(prev, 0xD12) != zMode || S32At(prev, 0xD14) != S32At(cur, 0xD14) ||
            U8At(prev, 0xD11) != U8At(cur, 0xD11)) {
            BpWrite((u32)zMode | ((u32)S32At(cur, 0xD14) * 2) | (((u32)U8At(cur, 0xD11) * 0x10) | 0x40000000u));
        }
    } else if (U8At(prev, 0xD12) != zMode || U8At(prev, 0xD11) != U8At(cur, 0xD11)) {
        BpWrite((u32)zMode | ((u32)S32At(cur, 0xD14) * 2) | (((u32)U8At(cur, 0xD11) * 0x10) | 0x40000000u));
    }

    // Eight texture units: texture mode, image address, size, format, TLUT and optional extra setup.
    if (S32At(cur, 0x0) != 0) {
        for (i = 0; i < 8; i++) {
            u8* texInfo = *(u8**)(cur + 0);
            u8* tex = *(u8**)(texInfo + i * 4 + 0x1C);
            if (tex == NULL) continue;
            u8* slot = cur + i * 0x24;
            s32 base = lbl_80348FB0[i];
            BpWrite((u32)S32At(slot, 0xA4) | ((u32)S32At(slot, 0xA8) << 2) | ((u32)S32At(slot, 0xB0) << 4) |
                    ((u32)U8At(tex, 0x16) << 5) | ((u32)S32At(slot, 0xAC) << 7) |
                    ((u32)S32At(slot, 0xB4) << 8) | ((u32)U8At(slot, 0xB8) << 9) |
                    ((u32)S32At(slot, 0xBC) << 19) | ((u32)U8At(slot, 0xC0) << 21) |
                    ((u32)(base + 0x80) << 24));
            s32 cacheIdx = (base & 3) | (base >> 3);
            s32 fmtIdx = S32At(slot, 0xC4);
            if (U32At(tex, 0x1C) == (u32)lbl_8040ADF0[cacheIdx] && fmtIdx == lbl_8040AE10[cacheIdx]) {
                continue;
            }
            lbl_805FB4B0 += 1;
            u32 fmtWord = ((u32*)&lbl_80348AD0)[fmtIdx];
            lbl_8040ADF0[cacheIdx] = S32At(tex, 0x1C);
            lbl_8040AE10[cacheIdx] = fmtIdx;
            u32 tblOfs = (u32)fmtIdx * 4 + (u32)U8At(tex, 0x17) * 32;
            BpWrite((((u32)(U8At(tex, 0x13) - 1) & 0xFu) << 12) | ((u32)(base + 0x84) << 24));
            s32 second = *(s32*)(lbl_80348A10 + tblOfs);
            BpWrite((u32)(U16At(tex, 0x8) - 1) | ((u32)(U16At(tex, 0xA) - 1) << 10) |
                    ((u32)(base + 0x88) << 24) | ((u32)U8At(tex, 0x14) << 20));
            BpWrite((*(u32*)(lbl_80348950 + tblOfs) >> 5) | ((u32)(base + 0x8C) << 24) | 0xD0000u | 0x8000u);
            if (second >= 0) {
                BpWrite(((u32)second >> 5) | 0xD0000u | 0x8000u | ((u32)(base + 0x90) << 24));
            } else {
                BpWrite((u32)(base + 0x90) << 24);
            }
            BpWrite(((U32At(tex, 0x1C) >> 5) & 0xFFFFFFu) | ((u32)(base + 0x94) << 24));
            if (U8At(tex, 0x11) != 0) {
                u32 hi = (fmtWord >> 9) & 0x3FFu;
                BpWrite((((u32)U8At(tex, 0x15) << 10) | hi) | ((u32)(base + 0x98) << 24));
                BpWrite(0xFEFFFF00u);
                BpWrite(0x0F000000u);
                BpWrite(((U32At(tex, 0x20) >> 5) & 0xFFFFFFu) | 0x64000000u);
                BpWrite((((hi | 0x65000000u) & ~0x1FC00u)) | ((((u32)(s32)((s32)U8At(tex, 0x11) >> 1)) << 10) & 0x1FC00u));
                BpWrite(0xFEFFFF00u);
                BpWrite(0x0F000000u);
            }
        }
    }

    // Texture sizes / wrap per unit (BP 0x30..): size from the unit or from its texture.
    for (i = 0; i < S32At(cur, 0x8); i++) {
        u8* u = cur + i * 0x34;
        u16 w = U16At(u, 0x1C6);
        u16 h = U16At(u, 0x1C8);
        if (U8At(u, 0x1C4) == 0) {
            u8* texInfo = *(u8**)(cur + 0);
            if (texInfo != NULL) {
                u8* tex = *(u8**)(texInfo + S32At(u, 0x1CC) * 4 + 0x1C);
                if (tex != NULL) {
                    w = U16At(tex, 0x8);
                    h = U16At(tex, 0xA);
                }
            }
        }
        BpWrite((u32)(w - 1) | ((u32)U8At(u, 0x1D0) << 16) | (((u32)U8At(u, 0x1D2) << 17) | ((u32)(i * 2 + 0x30) << 24)));
        BpWrite((u32)(h - 1) | ((u32)U8At(u, 0x1D1) << 16) | (((u32)(i * 2 + 0x31) << 24) | ((u32)U8At(u, 0x1D3) << 17)));
    }

    // Three TEV colour registers (BP 0xE2..): s10 or u8 packed pairs.
    for (i = 0; i < 3; i++) {
        const u8* c = cur + i * 0x14;
        const u8* p = prev + i * 0x14;
        s32 mode = S32At(c, 0x364);
        if (mode == 0) continue;
        if (S32At(p, 0x364) == mode && F32At(p, 0x374) == F32At(c, 0x374) && F32At(p, 0x368) == F32At(c, 0x368) &&
            F32At(p, 0x36C) == F32At(c, 0x36C) && F32At(p, 0x370) == F32At(c, 0x370)) {
            continue;
        }
        bool s10 = mode == 2;
        u32 w1 = PackColorPair(F32At(c, 0x374), F32At(c, 0x368), s10);
        u32 w2 = PackColorPair(F32At(c, 0x36C), F32At(c, 0x370), s10);
        u32 reg = (u32)((i + 1) * 2);
        BpWrite(w1 | ((reg + 0xE0) << 24));
        u32 word2 = w2 | ((reg + 0xE1) << 24);
        BpWrite(word2);
        BpWrite(word2);
        BpWrite(word2);
    }

    // Four konstant colours (BP 0xE0..): own colour plus the matching offset from `extra`.
    for (i = 0; i < 4; i++) {
        const u8* c = cur + i * 0x14;
        const u8* p = prev + i * 0x14;
        s32 mode = S32At(c, 0x3A0);
        if (mode == 0) continue;
        f32 f5 = F32At(c, 0x3A4) + F32At(extra, 0x20 + i * 0x10);
        f32 f6 = F32At(c, 0x3A8) + F32At(extra, 0x24 + i * 0x10);
        f32 f2 = F32At(c, 0x3AC) + F32At(extra, 0x28 + i * 0x10);
        f32 f3 = F32At(c, 0x3B0) + F32At(extra, 0x2C + i * 0x10);
        if (S32At(p, 0x3A0) == mode && f3 == F32At(p, 0x3B0) && f5 == F32At(p, 0x3A4) &&
            f6 == F32At(p, 0x3A8) && f2 == F32At(p, 0x3AC)) {
            continue;
        }
        bool s10 = mode == 2;
        u32 w1 = PackColorPair(f3, f5, s10);
        u32 w2 = PackColorPair(f6, f2, s10);
        u32 reg = 0xE0u + (u32)i * 2;
        BpWrite(w1 | 0x800000u | (reg << 24));
        BpWrite(w2 | 0x800000u | ((reg + 1) << 24));
    }

    // Four TEV swap/konst-select register pairs (BP 0xF6..0xFD, mask 0xFE00000F).
    for (i = 0; i < 4; i++) {
        const u8* c = cur + i * 0x10;
        const u8* p = prev + i * 0x10;
        if (S32At(p, 0x3F0) != S32At(c, 0x3F0) || S32At(p, 0x3F4) != S32At(c, 0x3F4)) {
            BpWrite(0xFE00000Fu);
            BpWrite((u32)S32At(c, 0x3F0) | ((u32)S32At(c, 0x3F4) << 2) | ((u32)(i * 2 + 0xF6) << 24));
        }
        if (S32At(p, 0x3F8) != S32At(c, 0x3F8) || S32At(p, 0x3FC) != S32At(c, 0x3FC)) {
            BpWrite(0xFE00000Fu);
            BpWrite((u32)S32At(c, 0x3F8) | ((u32)S32At(c, 0x3FC) << 2) | ((u32)(i * 2 + 0xF7) << 24));
        }
    }

    // TEV stages in pairs: ras/tex order words (BP 0x28.. and 0xF6..), stride 0xD0 per pair.
    {
        s32 n = (S32At(cur, 0xC) + 1) & ~1;
        s32 pn = (S32At(prev, 0xC) + 1) & ~1;
        if (n > 0) {
            const u8* c = cur;
            const u8* p = prev;
            s32 idx = 0;
            for (s32 k = 0; k < (s32)((u32)(n + 1) >> 1); k++, idx += 2) {
                s32 base = lbl_80348FD0[k];
                if (idx >= pn || S32At(p, 0x438) != S32At(c, 0x438) || S32At(p, 0x434) != S32At(c, 0x434) ||
                    U8At(p, 0x430) != U8At(c, 0x430) || S32At(p, 0x43C) != S32At(c, 0x43C) ||
                    S32At(p, 0x4A0) != S32At(c, 0x4A0) || S32At(p, 0x49C) != S32At(c, 0x49C) ||
                    U8At(p, 0x498) != U8At(c, 0x498) || S32At(p, 0x4A4) != S32At(c, 0x4A4)) {
                    BpWrite((u32)S32At(c, 0x434) | (((u32)S32At(c, 0x438) << 3) | ((u32)U8At(c, 0x430) << 6)) |
                            (((u32)S32At(c, 0x43C) << 7) | (((u32)S32At(c, 0x49C) << 12) | ((u32)S32At(c, 0x4A0) << 15))) |
                            (((u32)U8At(c, 0x498) << 18) | ((u32)S32At(c, 0x4A4) << 19) | ((u32)(base + 0x28) << 24)));
                }
                if (idx >= pn || S32At(p, 0x444) != S32At(c, 0x444) || S32At(p, 0x440) != S32At(c, 0x440) ||
                    S32At(p, 0x4AC) != S32At(c, 0x4AC) || S32At(p, 0x4A8) != S32At(c, 0x4A8)) {
                    BpWrite(0xFEFFFFF0u);
                    BpWrite(((u32)S32At(c, 0x444) << 4) | ((u32)S32At(c, 0x440) << 9) |
                            (((u32)S32At(c, 0x4AC) << 14) | ((u32)S32At(c, 0x4A8) << 19) | ((u32)(base + 0xF6) << 24)));
                }
                c += 0xD0;
                p += 0xD0;
            }
        }
    }

    // TEV stage colour/alpha combiner words (BP 0xC0.. pairs), stride 0x68.
    {
        const u8* c = cur;
        const u8* p = prev;
        s32 reg = 0;
        for (i = 0; i < S32At(cur, 0xC); i++, c += 0x68, p += 0x68, reg += 2) {
            if (i >= S32At(prev, 0xC) || S32At(p, 0x458) != S32At(c, 0x458) || S32At(p, 0x45C) != S32At(c, 0x45C) ||
                S32At(p, 0x460) != S32At(c, 0x460) || S32At(p, 0x464) != S32At(c, 0x464) ||
                S32At(p, 0x450) != S32At(c, 0x450) || S32At(p, 0x46C) != S32At(c, 0x46C) ||
                S32At(p, 0x470) != S32At(c, 0x470) || U8At(p, 0x468) != U8At(c, 0x468) ||
                S32At(p, 0x454) != S32At(c, 0x454)) {
                u32 flags = (u32)S32At(c, 0x450);
                u32 v454 = (u32)S32At(c, 0x454);
                u32 v468 = U8At(c, 0x468);
                u32 v464 = (u32)S32At(c, 0x464);
                u32 v460 = (u32)S32At(c, 0x460);
                u32 v45C = (u32)S32At(c, 0x45C);
                u32 v458 = (u32)S32At(c, 0x458);
                u32 word;
                if (flags & 2) {
                    word = v464 | (v460 * 0x10) |
                           ((v45C << 8) | ((v458 << 12) | (((v468 << 19) & ~0x40000u) | ((flags << 18) & 0x40000u) | 0x30000u))) |
                           (((flags << 18) & 0xFFF00000u) | (v454 << 22) | ((u32)(reg + 0xC0) << 24));
                } else {
                    word = v464 | ((v460 * 0x10) | (v45C << 8)) |
                           ((v458 << 12) | (((u32)S32At(c, 0x46C) << 16) | ((flags << 18) | (v468 << 19)))) |
                           (((u32)S32At(c, 0x470) << 20) | (v454 << 22) | ((u32)(reg + 0xC0) << 24));
                }
                BpWrite(word);
            }
            if (i >= S32At(prev, 0xC) || S32At(p, 0x47C) != S32At(c, 0x47C) || S32At(p, 0x480) != S32At(c, 0x480) ||
                S32At(p, 0x484) != S32At(c, 0x484) || S32At(p, 0x488) != S32At(c, 0x488) ||
                S32At(p, 0x474) != S32At(c, 0x474) || S32At(p, 0x490) != S32At(c, 0x490) ||
                S32At(p, 0x494) != S32At(c, 0x494) || U8At(p, 0x48C) != U8At(c, 0x48C) ||
                S32At(p, 0x478) != S32At(c, 0x478) || S32At(p, 0x448) != S32At(c, 0x448) ||
                S32At(p, 0x44C) != S32At(c, 0x44C)) {
                u32 flags = (u32)S32At(c, 0x474);
                u32 v44C = (u32)S32At(c, 0x44C);
                u32 v448 = (u32)S32At(c, 0x448);
                u32 v478 = (u32)S32At(c, 0x478);
                u32 v48C = U8At(c, 0x48C);
                u32 v488 = (u32)S32At(c, 0x488);
                u32 v484 = (u32)S32At(c, 0x484);
                u32 v480 = (u32)S32At(c, 0x480);
                u32 v47C = (u32)S32At(c, 0x47C);
                u32 word;
                if (flags & 2) {
                    word = v448 | ((v44C * 4) | ((v488 * 0x10) | (v484 << 7))) |
                           ((v480 << 10) | ((v47C << 13) | (((v48C << 19) & ~0x40000u) | ((flags << 18) & 0x40000u) | 0x30000u))) |
                           (((flags << 18) & 0xFFF00000u) | (v478 << 22) | ((u32)(reg + 0xC1) << 24));
                } else {
                    word = v448 | ((v44C * 4) | ((v488 * 0x10) | ((v484 << 7) | (v480 << 10)))) |
                           ((v47C << 13) | (((u32)S32At(c, 0x490) << 16) | ((flags << 18) | (v48C << 19)))) |
                           (((u32)S32At(c, 0x494) << 20) | (v478 << 22) | ((u32)(reg + 0xC1) << 24));
                }
                BpWrite(word);
            }
        }
    }

    // Number of colour channels (XF 0x1009) and tex-gens (XF 0x103F).
    s32 chans = S32At(cur, 0x4);
    s32 pchans = S32At(prev, 0x4);
    if (pchans != chans) {
        XfWrite(0x1009, (u32)chans);
        if (pchans >= 1 && S32At(cur, 0x4) < 1) {
            XfWrite(0x100E, 0x400);
            XfWrite(0x1010, 0x400);
        }
        if (S32At(prev, 0x4) >= 2 && S32At(cur, 0x4) < 2) {
            XfWrite(0x100F, 0x400);
            XfWrite(0x1011, 0x400);
        }
    }
    s32 ntexgen = S32At(cur, 0x8);
    if (S32At(prev, 0x8) != ntexgen) {
        XfWrite(0x103F, (u32)ntexgen);
    }

    // Colour channel 0 (XF 0x100A, 0x100C, 0x100E, 0x1010).
    if (S32At(cur, 0x4) > 0) {
        if (LightChanged(cur, prev, 0x14, 0x18)) {
            XfWrite(0x100A, U32At(cur, 0x18));
        }
        if (LightChanged(cur, prev, 0x1C, 0x20)) {
            XfWrite(0x100C, U32At(cur, 0x20));
        }
        if (S32At(prev, 0x4) < 1 || ChanDiffers(cur, prev, 0x24)) {
            XfWrite(0x100E, ChanWord(cur, 0x24));
        }
        if (S32At(prev, 0x4) < 1 || ChanDiffers(cur, prev, 0x40)) {
            XfWrite(0x1010, ChanWord(cur, 0x40));
        }
    }

    // Colour channel 1 (XF 0x100B, 0x100D, 0x100F, 0x1011).
    if (S32At(cur, 0x4) > 1) {
        if (LightChanged(cur, prev, 0x5C, 0x60)) {
            XfWrite(0x100B, U32At(cur, 0x60));
        }
        if (LightChanged(cur, prev, 0x64, 0x68)) {
            XfWrite(0x100D, U32At(cur, 0x68));
        }
        if (S32At(prev, 0x4) < 2 || ChanDiffers(cur, prev, 0x6C)) {
            XfWrite(0x100F, ChanWord(cur, 0x6C));
        }
        if (S32At(prev, 0x4) < 2 || ChanDiffers(cur, prev, 0x88)) {
            XfWrite(0x1011, ChanWord(cur, 0x88));
        }
    }

    // Tex-coord generators (XF 0x1040+, 0x1050+): per unit, stride 0x34.
    {
        const u8* c = cur;
        const u8* p = prev;
        for (i = 0; i < S32At(cur, 0x8); i++, c += 0x34, p += 0x34) {
            if (S32At(prev, 0x8) <= i || S32At(p, 0x1D4) != S32At(c, 0x1D4) || S32At(p, 0x1D8) != S32At(c, 0x1D8) ||
                S32At(p, 0x1DC) != S32At(c, 0x1DC) || S32At(p, 0x1E0) != S32At(c, 0x1E0) ||
                S32At(p, 0x1E4) != S32At(c, 0x1E4) || S32At(p, 0x1E8) != S32At(c, 0x1E8)) {
                XfWrite((u16)(i + 0x1040),
                        ((u32)S32At(c, 0x1E0) << 1) | ((u32)S32At(c, 0x1DC) << 2) | ((u32)S32At(c, 0x1D4) << 4) |
                        ((u32)S32At(c, 0x1D8) << 7) | ((u32)S32At(c, 0x1E4) << 15) | ((u32)S32At(c, 0x1E8) << 12));
            }
            if (S32At(prev, 0x8) <= i || S32At(p, 0x1F4) != S32At(c, 0x1F4) || U8At(p, 0x1F0) != U8At(c, 0x1F0)) {
                XfWrite((u16)(i + 0x1050), ((u32)U8At(c, 0x1F0) << 8) | (u32)S32At(c, 0x1F4));
            }
        }
    }

    // Indirect-texture / ras registers (BP 0x25..0x27).
    if (S32At(cur, 0x10) > 0) {
        BpWrite((u32)S32At(cur, 0xAB0) | (((u32)S32At(cur, 0xAB4) << 3) | ((u32)S32At(cur, 0xAC0) << 6) | ((u32)S32At(cur, 0xAC4) << 9)) |
                (((u32)S32At(cur, 0xAD0) << 12) | ((u32)S32At(cur, 0xAD4) << 15) |
                 (((u32)S32At(cur, 0xAE4) << 21) | 0x27000000u | ((u32)S32At(cur, 0xAE0) << 18))));
        if (S32At(prev, 0x10) < 2 || S32At(prev, 0xAB8) != S32At(cur, 0xAB8) || S32At(prev, 0xABC) != S32At(cur, 0xABC) ||
            S32At(prev, 0xAC8) != S32At(cur, 0xAC8) || S32At(prev, 0xACC) != S32At(cur, 0xACC)) {
            BpWrite((u32)S32At(cur, 0xAB8) | (((u32)S32At(cur, 0xACC) << 12) | 0x25000000u) |
                    (((u32)S32At(cur, 0xABC) << 4) | ((u32)S32At(cur, 0xAC8) << 8)));
        }
        if (S32At(cur, 0x10) > 2 &&
            (S32At(prev, 0x10) < 4 || S32At(prev, 0xAD8) != S32At(cur, 0xAD8) || S32At(prev, 0xADC) != S32At(cur, 0xADC) ||
             S32At(prev, 0xAE8) != S32At(cur, 0xAE8) || S32At(prev, 0xAEC) != S32At(cur, 0xAEC))) {
            BpWrite((u32)S32At(cur, 0xAD8) | (((u32)S32At(cur, 0xAEC) << 12) | 0x26000000u) |
                    (((u32)S32At(cur, 0xADC) << 4) | ((u32)S32At(cur, 0xAE8) << 8)));
        }
    }

    // Per-stage indirect words (BP 0x10..), stride 0x20.
    {
        const u8* c = cur;
        const u8* p = prev;
        for (i = 0; i < S32At(cur, 0xC); i++, c += 0x20, p += 0x20) {
            if (i >= S32At(prev, 0xC) || S32At(p, 0xAF0) != S32At(c, 0xAF0) || S32At(p, 0xAF4) != S32At(c, 0xAF4) ||
                S32At(p, 0xAF8) != S32At(c, 0xAF8) || S32At(p, 0xAFC) != S32At(c, 0xAFC) ||
                S32At(p, 0xB00) != S32At(c, 0xB00) || S32At(p, 0xB04) != S32At(c, 0xB04) ||
                U8At(p, 0xB08) != U8At(c, 0xB08) || U8At(p, 0xB09) != U8At(c, 0xB09) ||
                S32At(p, 0xB0C) != S32At(c, 0xB0C)) {
                BpWrite((u32)S32At(c, 0xAF0) | (((u32)S32At(c, 0xAF4) << 2) | ((u32)S32At(c, 0xAF8) << 4)) |
                        (((u32)S32At(c, 0xB0C) << 7) | (((u32)S32At(c, 0xAFC) << 9) | (((u32)S32At(c, 0xB00) << 13) | ((u32)S32At(c, 0xB04) << 16))) |
                         (((u32)U8At(c, 0xB09) << 19) | ((u32)U8At(c, 0xB08) << 20) | ((u32)(i + 0x10) << 24))));
            }
        }
    }

    // Final: two packed words, sent as CP regs 0x30/0x40 and again through XF 0x1018 (2 words).
    u32 w1 = ((u32)S32At(cur, 0x288) << 24) | ((u32)S32At(cur, 0x254) << 18) |
             (((u32)S32At(cur, 0x220) << 12) | ((u32)S32At(cur, 0x1EC) << 6));
    u32 w2 = ((u32)S32At(cur, 0x358) << 18) | (((u32)S32At(cur, 0x324) << 12) | (u32)S32At(cur, 0x2BC) |
                                               ((u32)S32At(cur, 0x2F0) << 6));
    GXWGFifo._u8 = 8;
    GXWGFifo._u8 = 0x30;
    GXWGFifo._u32 = w1;
    GXWGFifo._u8 = 8;
    GXWGFifo._u8 = 0x40;
    GXWGFifo._u32 = w2;
    XfHeader(1, 0x1018);
    GXWGFifo._u32 = w1;
    GXWGFifo._u32 = w2;
    return 1;
}

}  // extern "C"
