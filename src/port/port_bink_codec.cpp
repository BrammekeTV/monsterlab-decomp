/*
 * Bink 1 video + audio decoder and container parser for the PC port.
 *
 * Standalone port of FFmpeg 7.1 libavcodec/bink.c, binkdsp.c, binkaudio.c (DCT variant) and
 * libavformat/bink.c, without any of FFmpeg's infrastructure.
 * Copyright (c) 2009 Konstantin Shishkov, (C) 2011 Peter Ross and the FFmpeg developers.
 *
 * This file is part of FFmpeg, and is licensed under the GNU Lesser General Public License,
 * version 2.1 or (at your option) any later version; see third_party/bink/COPYING.LGPLv2.1.
 *
 * Supported: video revisions f, g, h, i (and k without alpha); no alpha planes, no revision b.
 * Audio: the DCT variant (the only one the game's discs use).
 */
#include "port/port_bink_codec.h"
#include "../../third_party/bink/binkdata.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

namespace bink {

namespace {

// ---------------------------------------------------------------------------------------------
// Little-endian bit reader (FFmpeg's get_bits with BITSTREAM_READER_LE)
// ---------------------------------------------------------------------------------------------
struct BitReader {
    const uint8_t* d = nullptr;
    size_t sizeBits = 0;
    size_t pos = 0;

    void setup(const uint8_t* data, size_t bytes) { d = data; sizeBits = bytes * 8; pos = 0; }
    int64_t left() const { return (int64_t)sizeBits - (int64_t)pos; }
    // 'd' is padded with at least 8 zero bytes by the caller.
    uint32_t peek(int n) const {   // n in 0..32
        if (n == 0) return 0;
        size_t byte = pos >> 3;
        int sh = (int)(pos & 7);
        uint64_t v = 0;
        memcpy(&v, d + byte, 8);
        v >>= sh;
        if (n == 32) return (uint32_t)v;
        return (uint32_t)(v & ((1u << n) - 1));
    }
    uint32_t get(int n) { uint32_t v = peek(n); pos += n; return v; }
    uint32_t get1() { return get(1); }
    void skip(int n) { pos += n; }
    size_t count() const { return pos; }
};

// A buffer with zero padding for the bit reader.
struct PaddedBuf {
    std::vector<uint8_t> v;
    void set(const uint8_t* p, size_t n) { v.assign(n + 16, 0); if (n) memcpy(v.data(), p, n); }
};

inline int ilog2(unsigned x) { int r = 0; while (x >>= 1) r++; return r; }
inline int falign(int x, int a) { return (x + a - 1) & ~(a - 1); }

// ---------------------------------------------------------------------------------------------
// Huffman trees (16 symbols, codes stored LSB first)
// ---------------------------------------------------------------------------------------------
struct VlcEntry { uint8_t sym, len; };
struct VlcTable { int bits; VlcEntry t[1 << 7]; };
VlcTable gTrees[16];
bool gTreesInit = false;

void InitTrees() {
    if (gTreesInit) return;
    for (int i = 0; i < 16; i++) {
        int maxbits = bink_tree_lens[i][15];
        gTrees[i].bits = maxbits;
        for (int s = 0; s < 16; s++) {
            int len = bink_tree_lens[i][s];
            unsigned code = bink_tree_bits[i][s];
            for (unsigned hi = 0; hi < (1u << (maxbits - len)); hi++) {
                VlcEntry& e = gTrees[i].t[code | (hi << len)];
                e.sym = (uint8_t)s;
                e.len = (uint8_t)len;
            }
        }
    }
    gTreesInit = true;
}

// ---------------------------------------------------------------------------------------------
// Video
// ---------------------------------------------------------------------------------------------
enum Sources {
    SRC_BLOCK_TYPES = 0, SRC_SUB_BLOCK_TYPES, SRC_COLORS, SRC_PATTERN, SRC_X_OFF, SRC_Y_OFF,
    SRC_INTRA_DC, SRC_INTER_DC, SRC_RUN, NB_SRC
};
enum BlockTypes {
    SKIP_BLOCK = 0, SCALED_BLOCK, MOTION_BLOCK, RUN_BLOCK, RESIDUE_BLOCK, INTRA_BLOCK, FILL_BLOCK,
    INTER_BLOCK, PATTERN_BLOCK, RAW_BLOCK
};

struct Tree { int vlcNum = 0; uint8_t syms[16] = {}; };
struct Bundle {
    int len = 0;
    Tree tree;
    uint8_t* data = nullptr;
    uint8_t* dataEnd = nullptr;
    uint8_t* curDec = nullptr;
    uint8_t* curPtr = nullptr;
};

inline int GetHuff(BitReader& gb, const Tree& t) {
    const VlcTable& vt = gTrees[t.vlcNum];
    const VlcEntry& e = vt.t[gb.peek(vt.bits)];
    gb.skip(e.len);
    return t.syms[e.sym];
}

// --- DSP (FFmpeg binkdsp.c) ---
#define A1 2896
#define A2 2217
#define A3 3784
#define A4 -5352
#define MUL(X, Y) ((int)((unsigned)(X) * (Y)) >> 11)

#define IDCT_TRANSFORM(dest, s0, s1, s2, s3, s4, s5, s6, s7, d0, d1, d2, d3, d4, d5, d6, d7, munge, src) { \
    const int a0 = (src)[s0] + (src)[s4]; \
    const int a1 = (src)[s0] - (src)[s4]; \
    const int a2 = (src)[s2] + (src)[s6]; \
    const int a3 = MUL(A1, (src)[s2] - (src)[s6]); \
    const int a4 = (src)[s5] + (src)[s3]; \
    const int a5 = (src)[s5] - (src)[s3]; \
    const int a6 = (src)[s1] + (src)[s7]; \
    const int a7 = (src)[s1] - (src)[s7]; \
    const int b0 = a4 + a6; \
    const int b1 = MUL(A3, a5 + a7); \
    const int b2 = MUL(A4, a5) - b0 + b1; \
    const int b3 = MUL(A1, a6 - a4) - b2; \
    const int b4 = MUL(A2, a7) + b3 - b1; \
    (dest)[d0] = munge(a0 + a2 + b0); \
    (dest)[d1] = munge(a1 + a3 - a2 + b2); \
    (dest)[d2] = munge(a1 - a3 + a2 + b3); \
    (dest)[d3] = munge(a0 - a2 - b4); \
    (dest)[d4] = munge(a0 - a2 + b4); \
    (dest)[d5] = munge(a1 - a3 + a2 - b3); \
    (dest)[d6] = munge(a1 + a3 - a2 - b2); \
    (dest)[d7] = munge(a0 + a2 - b0); \
}
#define MUNGE_NONE(x) (x)
#define IDCT_COL(dest, src) IDCT_TRANSFORM(dest, 0, 8, 16, 24, 32, 40, 48, 56, 0, 8, 16, 24, 32, 40, 48, 56, MUNGE_NONE, src)
#define MUNGE_ROW(x) (((x) + 0x7F) >> 8)
#define IDCT_ROW(dest, src) IDCT_TRANSFORM(dest, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, MUNGE_ROW, src)

inline void idct_col(int* dest, const int32_t* src) {
    if ((src[8] | src[16] | src[24] | src[32] | src[40] | src[48] | src[56]) == 0) {
        dest[0] = dest[8] = dest[16] = dest[24] = dest[32] = dest[40] = dest[48] = dest[56] = src[0];
    } else {
        IDCT_COL(dest, src);
    }
}
void idct_c(int32_t* block) {
    int temp[64];
    for (int i = 0; i < 8; i++) idct_col(&temp[i], &block[i]);
    for (int i = 0; i < 8; i++) { IDCT_ROW((&block[8 * i]), (&temp[8 * i])); }
}
void idct_add(uint8_t* dest, int linesize, int32_t* block) {
    idct_c(block);
    for (int i = 0; i < 8; i++, dest += linesize, block += 8)
        for (int j = 0; j < 8; j++) dest[j] = (uint8_t)(dest[j] + block[j]);
}
void idct_put(uint8_t* dest, int linesize, int32_t* block) {
    int temp[64];
    for (int i = 0; i < 8; i++) idct_col(&temp[i], &block[i]);
    for (int i = 0; i < 8; i++) { IDCT_ROW((&dest[i * linesize]), (&temp[8 * i])); }
}
void scale_block(const uint8_t* src, uint8_t* dst, int linesize) {
    for (int j = 0; j < 8; j++) {
        uint8_t* d1 = dst + (size_t)2 * j * linesize;
        uint8_t* d2 = d1 + linesize;
        for (int i = 0; i < 8; i++) {
            d1[2 * i] = d1[2 * i + 1] = d2[2 * i] = d2[2 * i + 1] = src[i];
        }
        src += 8;
    }
}
void add_pixels8(uint8_t* pixels, const int16_t* block, int line_size) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) pixels[j] = (uint8_t)(pixels[j] + block[j]);
        pixels += line_size;
        block += 8;
    }
}
void put_pixels8(uint8_t* dst, const uint8_t* src, int stride) {
    for (int i = 0; i < 8; i++) memmove(dst + i * stride, src + i * stride, 8);
}
void fill_block(uint8_t* dst, int v, int stride, int size) {
    for (int i = 0; i < size; i++) memset(dst + i * stride, v, size);
}

}  // namespace

// ---------------------------------------------------------------------------------------------
struct VideoDecoder::Impl {
    int version = 0;
    int width = 0, height = 0;
    bool swapPlanes = false;
    bool haveLast = false;

    struct Frame {
        std::vector<uint8_t> buf[3];
        int stride[3] = {0, 0, 0};
        int rows[3] = {0, 0, 0};
    } cur, last;

    Bundle bundle[NB_SRC];
    Tree colHigh[16];
    int colLastval = 0;
    std::vector<uint8_t> bundleMem;

    BitReader gb;

    void alloc(Frame& f) {
        for (int p = 0; p < 3; p++) {
            int w = p ? (width + 1) >> 1 : width;
            int h = p ? (height + 1) >> 1 : height;
            f.stride[p] = falign(w, 32) + 32;
            f.rows[p] = falign(h, 32) + 16;
            f.buf[p].assign((size_t)f.stride[p] * f.rows[p], 0);
        }
    }

    void initLengths(int w, int bw) {
        w = falign(w, 8);
        bundle[SRC_BLOCK_TYPES].len = ilog2((w >> 3) + 511) + 1;
        bundle[SRC_SUB_BLOCK_TYPES].len = ilog2((w >> 4) + 511) + 1;
        bundle[SRC_COLORS].len = ilog2(bw * 64 + 511) + 1;
        bundle[SRC_INTRA_DC].len = bundle[SRC_INTER_DC].len = bundle[SRC_X_OFF].len =
            bundle[SRC_Y_OFF].len = ilog2((w >> 3) + 511) + 1;
        bundle[SRC_PATTERN].len = ilog2((bw << 3) + 511) + 1;
        bundle[SRC_RUN].len = ilog2(bw * 48 + 511) + 1;
    }

    static void merge(BitReader& g, uint8_t* dst, uint8_t* src, int size) {
        uint8_t* src2 = src + size;
        int size2 = size;
        do {
            if (!g.get1()) { *dst++ = *src++; size--; }
            else { *dst++ = *src2++; size2--; }
        } while (size && size2);
        while (size--) *dst++ = *src++;
        while (size2--) *dst++ = *src2++;
    }

    bool readTree(Tree& tree) {
        uint8_t tmp1[16] = {0}, tmp2[16], *in = tmp1, *out = tmp2;
        if (gb.left() < 4) return false;
        tree.vlcNum = gb.get(4);
        if (!tree.vlcNum) {
            for (int i = 0; i < 16; i++) tree.syms[i] = (uint8_t)i;
            return true;
        }
        if (gb.get1()) {
            int len = gb.get(3);
            for (int i = 0; i <= len; i++) {
                tree.syms[i] = (uint8_t)gb.get(4);
                tmp1[tree.syms[i]] = 1;
            }
            for (int i = 0; i < 16 && len < 16 - 1; i++)
                if (!tmp1[i]) tree.syms[++len] = (uint8_t)i;
        } else {
            int len = gb.get(2);
            for (int i = 0; i < 16; i++) in[i] = (uint8_t)i;
            for (int i = 0; i <= len; i++) {
                int size = 1 << i;
                for (int t = 0; t < 16; t += size << 1) merge(gb, out + t, in + t, size);
                uint8_t* tp = in; in = out; out = tp;
            }
            memcpy(tree.syms, in, 16);
        }
        return true;
    }

    bool readBundle(int n) {
        if (n == SRC_COLORS) {
            for (int i = 0; i < 16; i++)
                if (!readTree(colHigh[i])) return false;
            colLastval = 0;
        }
        if (n != SRC_INTRA_DC && n != SRC_INTER_DC)
            if (!readTree(bundle[n].tree)) return false;
        bundle[n].curDec = bundle[n].curPtr = bundle[n].data;
        return true;
    }

#define CHECK_READ_VAL(b, t) \
    if (!b->curDec || (b->curDec > b->curPtr)) return true; \
    t = gb.get(b->len); \
    if (!t) { b->curDec = nullptr; return true; }

    bool readRuns(Bundle* b) {
        int t;
        CHECK_READ_VAL(b, t);
        const uint8_t* decEnd = b->curDec + t;
        if (decEnd > b->dataEnd) return false;
        if (gb.left() < 1) return false;
        if (gb.get1()) {
            int v = gb.get(4);
            memset(b->curDec, v, t);
            b->curDec += t;
        } else {
            while (b->curDec < decEnd) *b->curDec++ = (uint8_t)GetHuff(gb, b->tree);
        }
        return true;
    }

    bool readMotionValues(Bundle* b) {
        int t;
        CHECK_READ_VAL(b, t);
        const uint8_t* decEnd = b->curDec + t;
        if (decEnd > b->dataEnd) return false;
        if (gb.left() < 1) return false;
        if (gb.get1()) {
            int v = gb.get(4);
            if (v) { int sign = -(int)gb.get1(); v = (v ^ sign) - sign; }
            memset(b->curDec, v, t);
            b->curDec += t;
        } else {
            while (b->curDec < decEnd) {
                int v = GetHuff(gb, b->tree);
                if (v) { int sign = -(int)gb.get1(); v = (v ^ sign) - sign; }
                *b->curDec++ = (uint8_t)v;
            }
        }
        return true;
    }

    bool readBlockTypes(Bundle* b) {
        static const uint8_t rlelens[4] = {4, 8, 12, 32};
        int t, last = 0;
        CHECK_READ_VAL(b, t);
        if (version == 'k') {
            t ^= 0xBB;
            if (t == 0) { b->curDec = nullptr; return true; }
        }
        const uint8_t* decEnd = b->curDec + t;
        if (decEnd > b->dataEnd) return false;
        if (gb.left() < 1) return false;
        if (gb.get1()) {
            int v = gb.get(4);
            memset(b->curDec, v, t);
            b->curDec += t;
        } else {
            while (b->curDec < decEnd) {
                int v = GetHuff(gb, b->tree);
                if (v < 12) {
                    last = v;
                    *b->curDec++ = (uint8_t)v;
                } else {
                    int run = rlelens[v - 12];
                    if (decEnd - b->curDec < run) return false;
                    memset(b->curDec, last, run);
                    b->curDec += run;
                }
            }
        }
        return true;
    }

    bool readPatterns(Bundle* b) {
        int t;
        CHECK_READ_VAL(b, t);
        const uint8_t* decEnd = b->curDec + t;
        if (decEnd > b->dataEnd) return false;
        while (b->curDec < decEnd) {
            if (gb.left() < 2) return false;
            int v = GetHuff(gb, b->tree);
            v |= GetHuff(gb, b->tree) << 4;
            *b->curDec++ = (uint8_t)v;
        }
        return true;
    }

    bool readColors(Bundle* b) {
        int t;
        CHECK_READ_VAL(b, t);
        const uint8_t* decEnd = b->curDec + t;
        if (decEnd > b->dataEnd) return false;
        if (gb.left() < 1) return false;
        if (gb.get1()) {
            colLastval = GetHuff(gb, colHigh[colLastval]);
            int v = GetHuff(gb, b->tree);
            v = (colLastval << 4) | v;
            if (version < 'i') {
                int sign = ((int8_t)v) >> 7;
                v = ((v & 0x7F) ^ sign) - sign;
                v += 0x80;
            }
            memset(b->curDec, v, t);
            b->curDec += t;
        } else {
            while (b->curDec < decEnd) {
                if (gb.left() < 2) return false;
                colLastval = GetHuff(gb, colHigh[colLastval]);
                int v = GetHuff(gb, b->tree);
                v = (colLastval << 4) | v;
                if (version < 'i') {
                    int sign = ((int8_t)v) >> 7;
                    v = ((v & 0x7F) ^ sign) - sign;
                    v += 0x80;
                }
                *b->curDec++ = (uint8_t)v;
            }
        }
        return true;
    }

    bool readDcs(Bundle* b, int startBits, int hasSign) {
        int16_t* dst = (int16_t*)b->curDec;
        int16_t* dstEnd = (int16_t*)b->dataEnd;
        int len;
        CHECK_READ_VAL(b, len);
        if (gb.left() < startBits - hasSign) return false;
        int v = gb.get(startBits - hasSign);
        if (v && hasSign) { int sign = -(int)gb.get1(); v = (v ^ sign) - sign; }
        if (dstEnd - dst < 1) return false;
        *dst++ = (int16_t)v;
        len--;
        for (int i = 0; i < len; i += 8) {
            int len2 = len - i < 8 ? len - i : 8;
            if (dstEnd - dst < len2) return false;
            int bsize = gb.get(4);
            if (bsize) {
                for (int j = 0; j < len2; j++) {
                    int v2 = gb.get(bsize);
                    if (v2) { int sign = -(int)gb.get1(); v2 = (v2 ^ sign) - sign; }
                    v += v2;
                    *dst++ = (int16_t)v;
                    if (v < -32768 || v > 32767) return false;
                }
            } else {
                for (int j = 0; j < len2; j++) *dst++ = (int16_t)v;
            }
        }
        b->curDec = (uint8_t*)dst;
        return true;
    }

    int getValue(int n) {
        if (n < SRC_X_OFF || n == SRC_RUN) return *bundle[n].curPtr++;
        if (n == SRC_X_OFF || n == SRC_Y_OFF) return (int8_t)*bundle[n].curPtr++;
        int ret = *(int16_t*)bundle[n].curPtr;
        bundle[n].curPtr += 2;
        return ret;
    }

    // returns quant idx or -1 on error
    int readDctCoeffs(int32_t block[64], int* coefCountOut, int coefIdx[64]) {
        int coefList[128], modeList[128];
        int listStart = 64, listEnd = 64, listPos;
        int coefCount = 0;

        if (gb.left() < 4) return -1;
        coefList[listEnd] = 4;  modeList[listEnd++] = 0;
        coefList[listEnd] = 24; modeList[listEnd++] = 0;
        coefList[listEnd] = 44; modeList[listEnd++] = 0;
        coefList[listEnd] = 1;  modeList[listEnd++] = 3;
        coefList[listEnd] = 2;  modeList[listEnd++] = 3;
        coefList[listEnd] = 3;  modeList[listEnd++] = 3;

        for (int bits = (int)gb.get(4) - 1; bits >= 0; bits--) {
            listPos = listStart;
            while (listPos < listEnd) {
                if (!(modeList[listPos] | coefList[listPos]) || !gb.get1()) { listPos++; continue; }
                int ccoef = coefList[listPos];
                int mode = modeList[listPos];
                switch (mode) {
                case 0:
                    coefList[listPos] = ccoef + 4;
                    modeList[listPos] = 1;
                    // fallthrough
                case 2:
                    if (mode == 2) {
                        coefList[listPos] = 0;
                        modeList[listPos++] = 0;
                    }
                    for (int i = 0; i < 4; i++, ccoef++) {
                        if (gb.get1()) {
                            coefList[--listStart] = ccoef;
                            modeList[listStart] = 3;
                        } else {
                            int t;
                            if (!bits) {
                                t = 1 - ((int)gb.get1() << 1);
                            } else {
                                t = gb.get(bits) | 1 << bits;
                                int sign = -(int)gb.get1();
                                t = (t ^ sign) - sign;
                            }
                            block[bink_scan[ccoef]] = t;
                            coefIdx[coefCount++] = ccoef;
                        }
                    }
                    break;
                case 1:
                    modeList[listPos] = 2;
                    for (int i = 0; i < 3; i++) {
                        ccoef += 4;
                        coefList[listEnd] = ccoef;
                        modeList[listEnd++] = 2;
                    }
                    break;
                case 3: {
                    int t;
                    if (!bits) {
                        t = 1 - ((int)gb.get1() << 1);
                    } else {
                        t = gb.get(bits) | 1 << bits;
                        int sign = -(int)gb.get1();
                        t = (t ^ sign) - sign;
                    }
                    block[bink_scan[ccoef]] = t;
                    coefIdx[coefCount++] = ccoef;
                    coefList[listPos] = 0;
                    modeList[listPos++] = 0;
                    break;
                }
                }
            }
        }
        int quantIdx = gb.get(4);
        *coefCountOut = coefCount;
        return quantIdx;
    }

    static void unquantize(int32_t block[64], const int32_t quant[64], int coefCount, int coefIdx[64]) {
        block[0] = (int)((unsigned)block[0] * (unsigned)quant[0]) >> 11;
        for (int i = 0; i < coefCount; i++) {
            int idx = coefIdx[i];
            block[bink_scan[idx]] = (int)((unsigned)block[bink_scan[idx]] * (unsigned)quant[idx]) >> 11;
        }
    }

    void readResidue(int16_t block[64], int masksCount) {
        int coefList[128], modeList[128];
        int listStart = 64, listEnd = 64, listPos;
        int nzCoeff[64];
        int nzCount = 0;

        coefList[listEnd] = 4;  modeList[listEnd++] = 0;
        coefList[listEnd] = 24; modeList[listEnd++] = 0;
        coefList[listEnd] = 44; modeList[listEnd++] = 0;
        coefList[listEnd] = 0;  modeList[listEnd++] = 2;

        for (int mask = 1 << gb.get(3); mask; mask >>= 1) {
            for (int i = 0; i < nzCount; i++) {
                if (!gb.get1()) continue;
                if (block[nzCoeff[i]] < 0) block[nzCoeff[i]] -= mask;
                else block[nzCoeff[i]] += mask;
                masksCount--;
                if (masksCount < 0) return;
            }
            listPos = listStart;
            while (listPos < listEnd) {
                if (!(coefList[listPos] | modeList[listPos]) || !gb.get1()) { listPos++; continue; }
                int ccoef = coefList[listPos];
                int mode = modeList[listPos];
                switch (mode) {
                case 0:
                    coefList[listPos] = ccoef + 4;
                    modeList[listPos] = 1;
                    // fallthrough
                case 2:
                    if (mode == 2) {
                        coefList[listPos] = 0;
                        modeList[listPos++] = 0;
                    }
                    for (int i = 0; i < 4; i++, ccoef++) {
                        if (gb.get1()) {
                            coefList[--listStart] = ccoef;
                            modeList[listStart] = 3;
                        } else {
                            nzCoeff[nzCount++] = bink_scan[ccoef];
                            int sign = -(int)gb.get1();
                            block[bink_scan[ccoef]] = (int16_t)((mask ^ sign) - sign);
                            masksCount--;
                            if (masksCount < 0) return;
                        }
                    }
                    break;
                case 1:
                    modeList[listPos] = 2;
                    for (int i = 0; i < 3; i++) {
                        ccoef += 4;
                        coefList[listEnd] = ccoef;
                        modeList[listEnd++] = 2;
                    }
                    break;
                case 3: {
                    nzCoeff[nzCount++] = bink_scan[ccoef];
                    int sign = -(int)gb.get1();
                    block[bink_scan[ccoef]] = (int16_t)((mask ^ sign) - sign);
                    coefList[listPos] = 0;
                    modeList[listPos++] = 0;
                    masksCount--;
                    if (masksCount < 0) return;
                    break;
                }
                }
            }
        }
    }

    bool putPixels(uint8_t* dst, uint8_t* prev, int stride, uint8_t* refStart, uint8_t* refEnd) {
        int xoff = getValue(SRC_X_OFF);
        int yoff = getValue(SRC_Y_OFF);
        uint8_t* ref = prev + xoff + yoff * stride;
        if (ref < refStart || ref > refEnd) return false;
        put_pixels8(dst, ref, stride);
        return true;
    }

    bool decodePlane(int plane, bool isChroma) {
        int32_t dctblock[64];
        int16_t block[64];
        uint8_t ublock[64];
        int coordmap[64], coefCount, coefIdx[64];

        Frame& f = cur;
        const int stride = f.stride[plane];
        int bw = isChroma ? (width + 15) >> 4 : (width + 7) >> 3;
        int bh = isChroma ? (height + 15) >> 4 : (height + 7) >> 3;
        int w = width >> isChroma;
        int h = height >> isChroma;

        if (version == 'k' && gb.get1()) {
            int fill = gb.get(8);
            for (int i = 0; i < h; i++) memset(f.buf[plane].data() + (size_t)i * stride, fill, w);
            goto end;
        }

        initLengths(w > 8 ? w : 8, bw);
        for (int i = 0; i < NB_SRC; i++)
            if (!readBundle(i)) return false;

        {
            uint8_t* lastBase = haveLast ? last.buf[plane].data() : f.buf[plane].data();
            int lastStride = haveLast ? last.stride[plane] : stride;
            uint8_t* refStart = lastBase;
            uint8_t* refEnd = refStart + (bw - 1 + lastStride * (bh - 1)) * 8;

            for (int i = 0; i < 64; i++) coordmap[i] = (i & 7) + (i >> 3) * stride;

            for (int by = 0; by < bh; by++) {
                if (!readBlockTypes(&bundle[SRC_BLOCK_TYPES])) return false;
                if (!readBlockTypes(&bundle[SRC_SUB_BLOCK_TYPES])) return false;
                if (!readColors(&bundle[SRC_COLORS])) return false;
                if (!readPatterns(&bundle[SRC_PATTERN])) return false;
                if (!readMotionValues(&bundle[SRC_X_OFF])) return false;
                if (!readMotionValues(&bundle[SRC_Y_OFF])) return false;
                if (!readDcs(&bundle[SRC_INTRA_DC], 11, 0)) return false;
                if (!readDcs(&bundle[SRC_INTER_DC], 11, 1)) return false;
                if (!readRuns(&bundle[SRC_RUN])) return false;

                uint8_t* dst = f.buf[plane].data() + (size_t)8 * by * stride;
                uint8_t* prev = lastBase + (size_t)8 * by * stride;
                for (int bx = 0; bx < bw; bx++, dst += 8, prev += 8) {
                    int blk = getValue(SRC_BLOCK_TYPES);
                    if (((by & 1) || (bx & 1)) && blk == SCALED_BLOCK) {
                        bx++; dst += 8; prev += 8;
                        continue;
                    }
                    switch (blk) {
                    case SKIP_BLOCK:
                        put_pixels8(dst, prev, stride);
                        break;
                    case SCALED_BLOCK: {
                        blk = getValue(SRC_SUB_BLOCK_TYPES);
                        switch (blk) {
                        case RUN_BLOCK: {
                            if (gb.left() < 4) return false;
                            const uint8_t* scan = bink_patterns[gb.get(4)];
                            int i = 0;
                            do {
                                int run = getValue(SRC_RUN) + 1;
                                i += run;
                                if (i > 64) return false;
                                if (gb.get1()) {
                                    int v = getValue(SRC_COLORS);
                                    for (int j = 0; j < run; j++) ublock[*scan++] = (uint8_t)v;
                                } else {
                                    for (int j = 0; j < run; j++) ublock[*scan++] = (uint8_t)getValue(SRC_COLORS);
                                }
                            } while (i < 63);
                            if (i == 63) ublock[*scan++] = (uint8_t)getValue(SRC_COLORS);
                            break;
                        }
                        case INTRA_BLOCK: {
                            memset(dctblock, 0, sizeof(dctblock));
                            dctblock[0] = getValue(SRC_INTRA_DC);
                            int q = readDctCoeffs(dctblock, &coefCount, coefIdx);
                            if (q < 0) return false;
                            unquantize(dctblock, bink_intra_quant[q], coefCount, coefIdx);
                            idct_put(ublock, 8, dctblock);
                            break;
                        }
                        case FILL_BLOCK: {
                            int v = getValue(SRC_COLORS);
                            fill_block(dst, v, stride, 16);
                            break;
                        }
                        case PATTERN_BLOCK: {
                            int col[2];
                            for (int i = 0; i < 2; i++) col[i] = getValue(SRC_COLORS);
                            for (int j = 0; j < 8; j++) {
                                int v = getValue(SRC_PATTERN);
                                for (int i = 0; i < 8; i++, v >>= 1) ublock[i + j * 8] = (uint8_t)col[v & 1];
                            }
                            break;
                        }
                        case RAW_BLOCK:
                            for (int j = 0; j < 8; j++)
                                for (int i = 0; i < 8; i++) ublock[i + j * 8] = (uint8_t)getValue(SRC_COLORS);
                            break;
                        default:
                            return false;
                        }
                        if (blk != FILL_BLOCK) scale_block(ublock, dst, stride);
                        bx++; dst += 8; prev += 8;
                        break;
                    }
                    case MOTION_BLOCK:
                        if (!putPixels(dst, prev, stride, refStart, refEnd)) return false;
                        break;
                    case RUN_BLOCK: {
                        const uint8_t* scan = bink_patterns[gb.get(4)];
                        int i = 0;
                        do {
                            int run = getValue(SRC_RUN) + 1;
                            i += run;
                            if (i > 64) return false;
                            if (gb.get1()) {
                                int v = getValue(SRC_COLORS);
                                for (int j = 0; j < run; j++) dst[coordmap[*scan++]] = (uint8_t)v;
                            } else {
                                for (int j = 0; j < run; j++) dst[coordmap[*scan++]] = (uint8_t)getValue(SRC_COLORS);
                            }
                        } while (i < 63);
                        if (i == 63) dst[coordmap[*scan++]] = (uint8_t)getValue(SRC_COLORS);
                        break;
                    }
                    case RESIDUE_BLOCK: {
                        if (!putPixels(dst, prev, stride, refStart, refEnd)) return false;
                        memset(block, 0, sizeof(block));
                        int v = gb.get(7);
                        readResidue(block, v);
                        add_pixels8(dst, block, stride);
                        break;
                    }
                    case INTRA_BLOCK: {
                        memset(dctblock, 0, sizeof(dctblock));
                        dctblock[0] = getValue(SRC_INTRA_DC);
                        int q = readDctCoeffs(dctblock, &coefCount, coefIdx);
                        if (q < 0) return false;
                        unquantize(dctblock, bink_intra_quant[q], coefCount, coefIdx);
                        idct_put(dst, stride, dctblock);
                        break;
                    }
                    case FILL_BLOCK: {
                        int v = getValue(SRC_COLORS);
                        fill_block(dst, v, stride, 8);
                        break;
                    }
                    case INTER_BLOCK: {
                        if (!putPixels(dst, prev, stride, refStart, refEnd)) return false;
                        memset(dctblock, 0, sizeof(dctblock));
                        dctblock[0] = getValue(SRC_INTER_DC);
                        int q = readDctCoeffs(dctblock, &coefCount, coefIdx);
                        if (q < 0) return false;
                        unquantize(dctblock, bink_inter_quant[q], coefCount, coefIdx);
                        idct_add(dst, stride, dctblock);
                        break;
                    }
                    case PATTERN_BLOCK: {
                        int col[2];
                        for (int i = 0; i < 2; i++) col[i] = getValue(SRC_COLORS);
                        for (int i = 0; i < 8; i++) {
                            int v = getValue(SRC_PATTERN);
                            for (int j = 0; j < 8; j++, v >>= 1) dst[i * stride + j] = (uint8_t)col[v & 1];
                        }
                        break;
                    }
                    case RAW_BLOCK:
                        for (int i = 0; i < 8; i++) memcpy(dst + i * stride, bundle[SRC_COLORS].curPtr + i * 8, 8);
                        bundle[SRC_COLORS].curPtr += 64;
                        break;
                    default:
                        return false;
                    }
                }
            }
        }
    end:
        if (gb.count() & 0x1F) gb.skip(32 - (int)(gb.count() & 0x1F));
        return true;
    }
};

VideoDecoder::VideoDecoder() : p(new Impl) { InitTrees(); }
VideoDecoder::~VideoDecoder() { delete p; }

bool VideoDecoder::Open(uint8_t revision, uint32_t videoFlags, int width, int height) {
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) return false;
    if (revision < 'f' || revision > 'k') return false;
    if (videoFlags & 0x00100000) return false;  // alpha plane: not used by the game
    p->version = revision;
    p->width = width;
    p->height = height;
    p->swapPlanes = revision >= 'h';
    p->haveLast = false;
    p->alloc(p->cur);
    p->alloc(p->last);
    int bw = (width + 7) >> 3, bh = (height + 7) >> 3;
    size_t blocks = (size_t)bw * bh;
    p->bundleMem.assign(blocks * 64 * 10, 0);
    for (int i = 0; i < NB_SRC; i++) {
        p->bundle[i].data = p->bundleMem.data() + blocks * 64 * i;
        p->bundle[i].dataEnd = p->bundle[i].data + blocks * 64;
    }
    frameNum = 0;
    return true;
}

bool VideoDecoder::Decode(const uint8_t* pkt, size_t size) {
    PaddedBuf pb;
    pb.set(pkt, size);
    Impl& c = *p;
    c.gb.setup(pb.v.data(), size);
    size_t bitsCount = size << 3;
    if (c.version >= 'i') c.gb.skip(32);
    frameNum++;
    for (int plane = 0; plane < 3; plane++) {
        int planeIdx = (!plane || !c.swapPlanes) ? plane : (plane ^ 3);
        if (!c.decodePlane(planeIdx, plane != 0)) return false;
        if (c.gb.count() >= bitsCount) break;
    }
    // The decoded frame becomes the reference for the next one.
    std::swap(c.cur, c.last);
    c.haveLast = true;
    return true;
}

Image VideoDecoder::GetImage() const {
    // After Decode the finished frame is in 'last'.
    const Impl& c = *p;
    Image im;
    for (int i = 0; i < 3; i++) {
        im.plane[i] = c.last.buf[i].data();
        im.stride[i] = c.last.stride[i];
    }
    im.width = c.width;
    im.height = c.height;
    return im;
}

// ---------------------------------------------------------------------------------------------
// Audio (DCT variant)
// ---------------------------------------------------------------------------------------------
namespace {
const uint16_t kCriticalFreqs[25] = {
    100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720, 2000,
    2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500, 24500,
};
const uint8_t kRleLengthTab[16] = {2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64};

// In-place radix-2 complex FFT (exp(+i...)), n a power of two.
struct Fft {
    int n = 0;
    std::vector<float> cs, sn;
    std::vector<int> rev;
    void plan(int size) {
        n = size;
        cs.resize(n / 2); sn.resize(n / 2); rev.resize(n);
        for (int i = 0; i < n / 2; i++) {
            double a = 2.0 * 3.14159265358979323846 * i / n;
            cs[i] = (float)cos(a); sn[i] = (float)sin(a);
        }
        int bits = ilog2(n);
        for (int i = 0; i < n; i++) {
            int r = 0;
            for (int b = 0; b < bits; b++) if (i & (1 << b)) r |= 1 << (bits - 1 - b);
            rev[i] = r;
        }
    }
    void run(float* re, float* im) const {
        for (int i = 0; i < n; i++) {
            int j = rev[i];
            if (j > i) { float t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
        }
        for (int len = 2; len <= n; len <<= 1) {
            int half = len >> 1, step = n / len;
            for (int i = 0; i < n; i += len) {
                for (int k = 0; k < half; k++) {
                    float wr = cs[k * step], wi = sn[k * step];
                    float xr = re[i + k + half] * wr - im[i + k + half] * wi;
                    float xi = re[i + k + half] * wi + im[i + k + half] * wr;
                    re[i + k + half] = re[i + k] - xr; im[i + k + half] = im[i + k] - xi;
                    re[i + k] += xr; im[i + k] += xi;
                }
            }
        }
    }
};
}  // namespace

struct AudioDecoder::Impl {
    bool versionB = false;
    bool first = true;
    int channels = 0;
    int frameLen = 0, overlapLen = 0, blockSize = 0, numBands = 0;
    float root = 0;
    unsigned bands[26] = {};
    float previous[2][2048 / 16 * 2] = {};
    float quantTable[96];
    BitReader gb;
    Fft fft;
    std::vector<float> fr, fi, twr, twi;

    // out[n] = scale * (c[0]*0.5*S0 ... ) : DCT-III of length frameLen
    void dct3(const float* c, float* out) {
        int N = frameLen, M = 2 * N;
        for (int k = 0; k < N; k++) {
            float ck = k ? c[k] : c[0] * 0.5f;   // DCT-III: the DC term has half weight
            fr[k] = ck * twr[k];
            fi[k] = ck * twi[k];
        }
        for (int k = N; k < M; k++) fr[k] = fi[k] = 0;
        fft.run(fr.data(), fi.data());
        float scale = 2.0f / (float)N;
        for (int n = 0; n < N; n++) out[n] = fr[n] * scale;
    }

    bool decodeBlock(float** out, int channelsHere, int chOffset) {
        float coeffs[4098];
        float quant[25];
        gb.skip(2);
        for (int ch = 0; ch < channelsHere; ch++) {
            if (gb.left() < 58) return false;
            auto getFloat = [&]() {
                int power = gb.get(5);
                float f = ldexpf((float)gb.get(23), power - 23);
                if (gb.get1()) f = -f;
                return f;
            };
            coeffs[0] = getFloat() * root;
            coeffs[1] = getFloat() * root;
            if (gb.left() < numBands * 8) return false;
            for (int i = 0; i < numBands; i++) {
                int value = gb.get(8);
                quant[i] = quantTable[value < 95 ? value : 95];
            }
            int k = 0;
            float q = quant[0];
            int i = 2;
            while (i < frameLen) {
                int j;
                int v = gb.get1();
                if (v) {
                    v = gb.get(4);
                    j = i + kRleLengthTab[v] * 8;
                } else {
                    j = i + 8;
                }
                if (j > frameLen) j = frameLen;
                int width = gb.get(4);
                if (width == 0) {
                    memset(coeffs + i, 0, (j - i) * sizeof(float));
                    i = j;
                    while (bands[k] < (unsigned)i) q = quant[k++];
                } else {
                    while (i < j) {
                        if (bands[k] == (unsigned)i) q = quant[k++];
                        int coeff = gb.get(width);
                        if (coeff) {
                            coeffs[i] = gb.get1() ? -q * coeff : q * coeff;
                        } else {
                            coeffs[i] = 0.0f;
                        }
                        i++;
                    }
                }
            }
            coeffs[0] /= 0.5f;
            dct3(coeffs, out[ch + chOffset]);
        }
        for (int ch = 0; ch < channelsHere; ch++) {
            int count = overlapLen * channelsHere;
            float* o = out[ch + chOffset];
            if (!first) {
                int j = ch;
                for (int i = 0; i < overlapLen; i++, j += channelsHere)
                    o[i] = (previous[ch + chOffset][i] * (count - j) + o[i] * j) / count;
            }
            memcpy(previous[ch + chOffset], &o[frameLen - overlapLen], overlapLen * sizeof(float));
        }
        first = false;
        return true;
    }
};

AudioDecoder::AudioDecoder() : p(new Impl) {}
AudioDecoder::~AudioDecoder() { delete p; }

bool AudioDecoder::Open(int sampleRate, int channels, bool dct, uint8_t revision) {
    if (!dct || channels < 1 || channels > 2 || sampleRate <= 0) return false;
    Impl& s = *p;
    int bits = sampleRate < 22050 ? 9 : sampleRate < 44100 ? 10 : 11;
    s.versionB = revision == 'b';
    s.channels = channels;
    s.frameLen = 1 << bits;
    s.overlapLen = s.frameLen / 16;
    s.blockSize = (s.frameLen - s.overlapLen) * channels;
    int half = (int)((sampleRate + 1LL) / 2);
    s.root = (float)(s.frameLen / (sqrt((double)s.frameLen) * 32768.0));
    for (int i = 0; i < 96; i++) s.quantTable[i] = expf(i * 0.15289164787221953823f) * s.root;
    for (s.numBands = 1; s.numBands < 25; s.numBands++)
        if (half <= kCriticalFreqs[s.numBands - 1]) break;
    s.bands[0] = 2;
    for (int i = 1; i < s.numBands; i++) s.bands[i] = (kCriticalFreqs[i - 1] * s.frameLen / half) & ~1;
    s.bands[s.numBands] = s.frameLen;
    s.first = true;
    // DCT-III through a 2N-point FFT: out[n] = Re(sum_k c[k] e^{i pi k/(2N)} e^{2 pi i k n/(2N)}).
    int N = s.frameLen;
    s.fft.plan(2 * N);
    s.fr.assign(2 * N, 0); s.fi.assign(2 * N, 0);
    s.twr.resize(N); s.twi.resize(N);
    for (int k = 0; k < N; k++) {
        double a = 3.14159265358979323846 * k / (2.0 * N);
        s.twr[k] = (float)cos(a); s.twi[k] = (float)sin(a);
    }
    return true;
}

void AudioDecoder::Reset() { p->first = true; }

bool AudioDecoder::DecodeAudio(const uint8_t* pkt, size_t size, std::vector<float>& out) {
    Impl& s = *p;
    if (size < 4) return false;
    PaddedBuf pb;
    pb.set(pkt, size);
    s.gb.setup(pb.v.data(), size);
    s.gb.skip(32);  // reported size
    std::vector<float> buf((size_t)s.frameLen * 2);
    float* chp[2] = {buf.data(), buf.data() + s.frameLen};
    int perBlock = s.frameLen - s.overlapLen;
    while (s.gb.left() > 0) {
        if (!s.decodeBlock(chp, s.channels, 0)) return false;
        size_t base = out.size();
        out.resize(base + (size_t)perBlock * s.channels);
        for (int i = 0; i < perBlock; i++)
            for (int ch = 0; ch < s.channels; ch++) out[base + (size_t)i * s.channels + ch] = chp[ch][i];
        // blocks are aligned to 32 bits
        int n = (int)((-(int64_t)s.gb.count()) & 31);
        if (n) s.gb.skip(n);
    }
    return true;
}

// ---------------------------------------------------------------------------------------------
// Container
// ---------------------------------------------------------------------------------------------
static uint32_t rl32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint32_t rl16(const uint8_t* p) { return p[0] | (p[1] << 8); }

int HeaderSize(const uint8_t* d, size_t n) {
    if (n < 48) return 0;
    if (d[0] != 'B' || d[1] != 'I' || d[2] != 'K') return -1;
    uint32_t frames = rl32(d + 8), tracks = rl32(d + 40);
    if (frames == 0 || frames > 1000000 || tracks > 256) return -1;
    return (int)(44 + 12 * (size_t)tracks + 4 * (size_t)frames);
}

bool ParseHeader(const uint8_t* d, size_t n, Header& h) {
    int need = HeaderSize(d, n);
    if (need <= 0 || n < (size_t)need) return false;
    h.revision = d[3];
    if (h.revision < 'f' || h.revision > 'i') return false;  // 'k' has an extra header field
    h.fileSize = rl32(d + 4) + 8;
    h.frames = rl32(d + 8);
    h.maxFrameSize = rl32(d + 12);
    h.width = rl32(d + 20);
    h.height = rl32(d + 24);
    h.fpsNum = rl32(d + 28);
    h.fpsDen = rl32(d + 32);
    h.videoFlags = rl32(d + 36);
    uint32_t nt = rl32(d + 40);
    if (!h.fpsNum || !h.fpsDen || !h.width || !h.height) return false;
    const uint8_t* q = d + 44 + 4 * nt;  // skip max decoded sizes
    h.tracks.resize(nt);
    for (uint32_t i = 0; i < nt; i++) {
        h.tracks[i].sampleRate = (int)rl16(q);
        uint32_t flags = rl16(q + 2);
        q += 4;
        h.tracks[i].dct = (flags & 0x1000) != 0;
        h.tracks[i].channels = (flags & 0x2000) ? 2 : 1;
    }
    for (uint32_t i = 0; i < nt; i++, q += 4) h.tracks[i].id = rl32(q);
    h.frameOffset.resize(h.frames + 1);
    h.frameKey.resize(h.frames + 1);
    for (uint32_t i = 0; i < h.frames; i++, q += 4) {
        uint32_t v = rl32(q);
        h.frameOffset[i] = v & ~1u;
        h.frameKey[i] = (uint8_t)(v & 1);
    }
    h.frameOffset[h.frames] = h.fileSize;
    h.headerBytes = (size_t)need;
    return true;
}

bool SplitFrame(const Header& h, const uint8_t* frame, uint32_t size, FrameSplit& out) {
    out.audio.clear();
    uint32_t remain = size;
    const uint8_t* q = frame;
    for (size_t t = 0; t < h.tracks.size(); t++) {
        if (remain < 4) return false;
        uint32_t asz = rl32(q);
        if (asz > remain - 4) return false;
        out.audio.push_back({q + 4, asz});
        q += 4 + asz;
        remain -= 4 + asz;
    }
    out.video = q;
    out.videoSize = remain;
    return true;
}

}  // namespace bink
