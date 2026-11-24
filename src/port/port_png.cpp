// PC platform layer: PNG files in memory order RGBA8 (red in the low byte of each u32), for the
// texture mods and --dump-textures (port_mods.cpp). Reading: 8 and 16 bits per channel, grey, RGB,
// palette, with or without alpha, not interlaced; inflate is the zlib the port already links
// (third_party/zlib). Writing: RGBA, stored (not compressed). No original code. See docs/modding.md.
#ifdef _WIN32
#define Z_PREFIX 1
#define Z_SOLO 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../../third_party/zlib/zlib.h"
#include "port/port.h"
#include "port/port_mods.h"

void* zcalloc(void* opaque, unsigned items, unsigned size);   // zl_zutil.cpp
void  zcfree(void* opaque, void* ptr);

static u32 Be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

static u32 Crc(const u8* p, size_t n, u32 crc) {
    static u32 table[256];
    if (!table[1]) for (u32 i = 0; i < 256; i++) { u32 c = i; for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1; table[i] = c; }
    for (size_t i = 0; i < n; i++) crc = table[(crc ^ p[i]) & 255] ^ (crc >> 8);
    return crc;
}

static void Chunk(FILE* f, const char* type, const u8* data, u32 n) {
    u8 len[4] = {(u8)(n >> 24), (u8)(n >> 16), (u8)(n >> 8), (u8)n};
    fwrite(len, 1, 4, f); fwrite(type, 1, 4, f);
    if (n) fwrite(data, 1, n, f);
    u32 crc = Crc((const u8*)type, 4, 0xFFFFFFFFu);
    if (n) crc = Crc(data, n, crc);
    crc ^= 0xFFFFFFFFu;
    u8 c[4] = {(u8)(crc >> 24), (u8)(crc >> 16), (u8)(crc >> 8), (u8)crc};
    fwrite(c, 1, 4, f);
}

BOOL PortPngWrite(const char* path, const u32* rgba, u32 w, u32 h) {
    if (!w || !h || w > 16384 || h > 16384) return FALSE;
    size_t row = 1 + (size_t)w * 4;
    std::vector<u8> raw(row * h);
    for (u32 y = 0; y < h; y++) {
        raw[y * row] = 0;                                    // filter: none
        memcpy(&raw[y * row + 1], rgba + (size_t)y * w, (size_t)w * 4);
    }
    std::vector<u8> z;
    z.reserve(raw.size() + raw.size() / 65535 * 5 + 16);
    z.push_back(0x78); z.push_back(0x01);
    u32 a = 1, b = 0;
    for (size_t at = 0; at < raw.size();) {
        u32 n = (u32)(raw.size() - at < 65535 ? raw.size() - at : 65535);
        z.push_back(at + n == raw.size() ? 1 : 0);
        z.push_back((u8)n); z.push_back((u8)(n >> 8)); z.push_back((u8)~n); z.push_back((u8)(~n >> 8));
        z.insert(z.end(), raw.begin() + at, raw.begin() + at + n);
        for (u32 i = 0; i < n; i++) { a = (a + raw[at + i]) % 65521; b = (b + a) % 65521; }
        at += n;
    }
    u32 adler = b << 16 | a;
    z.push_back((u8)(adler >> 24)); z.push_back((u8)(adler >> 16)); z.push_back((u8)(adler >> 8)); z.push_back((u8)adler);
    FILE* f = fopen(path, "wb");
    if (!f) return FALSE;
    static const u8 sig[8] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    u8 ihdr[13] = {(u8)(w >> 24), (u8)(w >> 16), (u8)(w >> 8), (u8)w, (u8)(h >> 24), (u8)(h >> 16), (u8)(h >> 8), (u8)h, 8, 6, 0, 0, 0};
    Chunk(f, "IHDR", ihdr, 13);
    Chunk(f, "IDAT", z.data(), (u32)z.size());
    Chunk(f, "IEND", NULL, 0);
    return fclose(f) == 0;
}

static u8 Paeth(int a, int b, int c) {
    int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    return (u8)(pa <= pb && pa <= pc ? a : pb <= pc ? b : c);
}

// The picture as w * h RGBA8 words from malloc (the caller frees it), or NULL with the reason in why.
u32* PortPngRead(const char* path, u32* outW, u32* outH, const char** why) {
    const char* dummy;
    if (!why) why = &dummy;
    *why = "cannot be opened";
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<u8> file;
    BOOL got = size > 8 + 25 && size < 0x10000000;
    if (got) { file.resize((size_t)size); got = fread(file.data(), 1, (size_t)size, f) == (size_t)size; }
    fclose(f);
    *why = "is not a PNG file";
    static const u8 sig[8] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
    if (!got || memcmp(file.data(), sig, 8)) return NULL;

    u32 w = 0, h = 0, depth = 0, type = 0;
    u8 pal[256][4];
    u32 palCount = 0;
    BOOL haveKey = FALSE;
    u16 key[3] = {0, 0, 0};
    for (u32 i = 0; i < 256; i++) pal[i][0] = pal[i][1] = pal[i][2] = 0, pal[i][3] = 255;
    std::vector<u8> idat;
    BOOL header = FALSE, end = FALSE;
    for (size_t at = 8; at + 12 <= file.size() && !end;) {
        u32 n = Be32(&file[at]);
        const u8* t = &file[at + 4];
        const u8* d = t + 4;
        if (n > file.size() - at - 12) { *why = "is cut short"; return NULL; }
        if (!memcmp(t, "IHDR", 4) && n >= 13) {
            w = Be32(d); h = Be32(d + 4); depth = d[8]; type = d[9];
            if (d[12] != 0) { *why = "is interlaced (save it without interlacing)"; return NULL; }
            header = TRUE;
        } else if (!memcmp(t, "PLTE", 4)) {
            palCount = n / 3 < 256 ? n / 3 : 256;
            for (u32 i = 0; i < palCount; i++) memcpy(pal[i], d + i * 3, 3);
        } else if (!memcmp(t, "tRNS", 4)) {
            if (type == 3) for (u32 i = 0; i < n && i < 256; i++) pal[i][3] = d[i];
            else if (type == 0 && n >= 2) { haveKey = TRUE; key[0] = (u16)(d[0] << 8 | d[1]); }
            else if (type == 2 && n >= 6) { haveKey = TRUE; for (int k = 0; k < 3; k++) key[k] = (u16)(d[k * 2] << 8 | d[k * 2 + 1]); }
        } else if (!memcmp(t, "IDAT", 4)) idat.insert(idat.end(), d, d + n);
        else if (!memcmp(t, "IEND", 4)) end = TRUE;
        at += (size_t)n + 12;
    }
    u32 channels = type == 0 ? 1 : type == 2 ? 3 : type == 3 ? 1 : type == 4 ? 2 : type == 6 ? 4 : 0;
    BOOL depthOk = type == 3 ? (depth == 1 || depth == 2 || depth == 4 || depth == 8)
                 : type == 0 ? (depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16) : (depth == 8 || depth == 16);
    if (!header || !channels || !depthOk || !w || !h || w > 8192 || h > 8192) { *why = "has a size or pixel format that is not supported"; return NULL; }
    u32 bpp = (channels * depth + 7) / 8;                    // bytes per whole pixel, for the filters
    size_t row = ((size_t)w * channels * depth + 7) / 8;
    std::vector<u8> raw((row + 1) * h);

    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    zs.zalloc = (alloc_func)zcalloc;
    zs.zfree = (free_func)zcfree;
    *why = "has damaged picture data";
    if (inflateInit(&zs) != Z_OK) return NULL;
    zs.next_in = idat.data(); zs.avail_in = (uInt)idat.size();
    zs.next_out = raw.data(); zs.avail_out = (uInt)raw.size();
    int zr = inflate(&zs, Z_FINISH);
    BOOL whole = (zr == Z_STREAM_END || zr == Z_OK || zr == Z_BUF_ERROR) && zs.avail_out == 0;
    inflateEnd(&zs);
    if (!whole) return NULL;

    // Undo the row filters in place.
    for (u32 y = 0; y < h; y++) {
        u8* cur = &raw[(row + 1) * y + 1];
        const u8* up = y ? cur - (row + 1) : NULL;
        u8 filter = cur[-1];
        if (filter > 4) return NULL;
        for (size_t x = 0; x < row; x++) {
            int a = x >= bpp ? cur[x - bpp] : 0, b = up ? up[x] : 0, c = (up && x >= bpp) ? up[x - bpp] : 0;
            cur[x] = (u8)(cur[x] + (filter == 1 ? a : filter == 2 ? b : filter == 3 ? (a + b) / 2 : filter == 4 ? Paeth(a, b, c) : 0));
        }
    }

    u32* out = (u32*)malloc((size_t)w * h * 4);
    if (!out) { *why = "is too large"; return NULL; }
    u32 maxv = (1u << depth) - 1;
    for (u32 y = 0; y < h; y++) {
        const u8* s = &raw[(row + 1) * y + 1];
        for (u32 x = 0; x < w; x++) {
            u32 v[4] = {0, 0, 0, 0xFFFF};                    // samples, 16-bit range for the colour-key test
            for (u32 c = 0; c < channels; c++) {
                if (depth == 16) v[c] = (u32)s[(x * channels + c) * 2] << 8 | s[(x * channels + c) * 2 + 1];
                else if (depth == 8) v[c] = s[x * channels + c];
                else { u32 bit = x * depth; v[c] = (s[bit >> 3] >> (8 - depth - (bit & 7))) & maxv; }
            }
            u32 r, g, b, a = 255;
            if (type == 3) { const u8* p = pal[v[0] & 255]; r = p[0]; g = p[1]; b = p[2]; a = p[3]; }
            else {
                BOOL keyed = haveKey && v[0] == key[0] && (type == 0 || (v[1] == key[1] && v[2] == key[2]));
                u32 s8[4];
                for (u32 c = 0; c < channels; c++) s8[c] = depth == 16 ? v[c] >> 8 : depth == 8 ? v[c] : v[c] * 255 / maxv;
                if (type == 0) { r = g = b = s8[0]; }
                else if (type == 4) { r = g = b = s8[0]; a = s8[1]; }
                else { r = s8[0]; g = s8[1]; b = s8[2]; if (type == 6) a = s8[3]; }
                if (keyed) a = 0;
            }
            out[(size_t)y * w + x] = r | g << 8 | b << 16 | a << 24;
        }
    }
    *outW = w; *outH = h;
    *why = NULL;
    return out;
}
#endif
