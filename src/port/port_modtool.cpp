// PC port: the mod tool (ml_modtool.exe, docs/concepts/mod-tools.md). The port's own executable started
// as a tool: it opens the .sir files of the player's disc (image or extracted folder) or of a mod, shows
// their structure as a tree, the bytes as they are on the disc and the fields the game reads from them,
// side by side. The files are converted by the port's own loader (sir_loader.cpp), so what the tool
// shows is what the game gets. It only reads: nothing on the disc or in orig/ is ever opened for writing.
//
// It runs instead of the game, from PortBoot once the disc is open: a window of its own drawn with
// Dear ImGui (as the first-run picker), or headless into a PNG (--modtool-shot) for tests and docs.
#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <unordered_map>

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "port/port.h"
#include "port/port_mods.h"
#include "port/sir_loader.h"
#include "port/port_modtool.h"

BOOL PortGxDecodeTexture(const u8* src, u32 w, u32 h, u32 fmt, const u8* tlut, u32 tlutFmt, u32* out);   // port_gfx_d3d11.cpp
u32  PortGxTextureBytes(u32 w, u32 h, u32 fmt);
BOOL PortPngWrite(const char* path, const u32* rgba, u32 w, u32 h);   // port_png.cpp
u32* PortPngRead(const char* path, u32* outW, u32* outH, const char** why);

#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL PortGfxSaveTexture(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, u32 w, u32 h, const char* path);

namespace {

// The game's StringHash: CRC-32 of the upper-cased string (as port_modpatch.cpp).
u32 NameHash(const char* s) {
    u32 c = 0xFFFFFFFFu;
    for (; *s; s++) {
        c ^= (u8)toupper((u8)*s);
        for (int k = 0; k < 8; k++) c = c >> 1 ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

static inline u32 Be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

// ---------------------------------------------------------------------------------------- names
// Hashes are shown with the text behind them when one of the strings of main.dol or of an open file
// hashes to it: the game's own names only.
std::unordered_map<u32, std::string>& Dict() {
    static auto* d = new std::unordered_map<u32, std::string>;   // no static objects in port files (port_runtime starts first)
    return *d;
}

void DictScan(const u8* p, size_t n) {
    for (size_t i = 0; i < n;) {
        size_t j = i;
        while (j < n && p[j] >= 0x20 && p[j] < 0x7F) j++;
        if (j < n && p[j] == 0 && j - i >= 2 && j - i < 80) {
            std::string s((const char*)p + i, j - i);
            Dict().emplace(NameHash(s.c_str()), s);
        }
        i = j + 1;
    }
}

// main.dol as LoadDol placed it: every committed page of MEM1.
void DictScanDol() {
    u8* p = (u8*)(uintptr_t)PORT_MEM1_BASE;
    u8* end = p + PORT_MEM1_SIZE;
    while (p < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(p, &mbi, sizeof(mbi))) break;
        u8* next = (u8*)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
            DictScan(p, (size_t)((next < end ? next : end) - p));
        p = next;
    }
}

std::string Hex32(u32 v) { char b[16]; snprintf(b, sizeof(b), "0x%08X", v); return b; }

std::string HashText(u32 h) {
    auto it = Dict().find(h);
    return it == Dict().end() ? Hex32(h) : it->second;
}

// ---------------------------------------------------------------------------------------- document
enum Kind { K_FILE, K_CONTAINER, K_SECTIONS, K_SECTION, K_HDR, K_SLOT, K_DB, K_TABLE, K_TEXLIST, K_TEX, K_STRINGS, K_STEX };   // K_STEX: a texture section (shared_texture.sir)

struct Node {
    std::string label;
    int kind;
    u32 off, size;        // byte range in the file
    int a;                // section, slot offset, table, texture offset in the image
    int parent;
    std::vector<int> kids;
};

struct Field {
    std::string name, type, value;
    u32 off, size;        // in the file
};

struct GameDbView { u8 pad00[0xC]; u8* data; };   // include/game/part.h GameDb

struct Doc {
    std::string path, from, error;
    BOOL zones = FALSE;               // the sections are zone images (levels, arenas, rooms, shared_parts, shared_ui)
    std::vector<u8> raw;              // the file as on the disc
    std::vector<Node> nodes;          // [0] is the file
    std::vector<SirSection> secs;
    // global_binary.sir, converted by the loader as the game does
    BOOL converted = FALSE;
    std::vector<u8> hdr, image;
    u32 hdrOff = 0, imgOff = 0;
    u8* db = NULL;                    // table descriptors
    SirStringTable* strings = NULL;

    // File offset of a host pointer into the converted copy, or ~0u.
    u32 Off(const void* p) const {
        const u8* b = (const u8*)p;
        if (!image.empty() && b >= image.data() && b < image.data() + image.size()) return imgOff + (u32)(b - image.data());
        if (!hdr.empty() && b >= hdr.data() && b < hdr.data() + hdr.size()) return hdrOff + (u32)(b - hdr.data());
        return ~0u;
    }
    const GameDbTable* Table(int t) const { return db && t >= 0 && t < GAMEDB_TABLE_COUNT ? (const GameDbTable*)(db + t * GAMEDB_DESC_SIZE) : NULL; }
};

int Add(Doc* d, int parent, int kind, const std::string& label, u32 off, u32 size, int a = 0) {
    Node n{label, kind, off, size, a, parent, {}};
    d->nodes.push_back(n);
    int i = (int)d->nodes.size() - 1;
    if (parent >= 0) d->nodes[parent].kids.push_back(i);
    return i;
}

// What the known slots of global_binary.sir's header hold (sir_loader.h).
const char* SlotName(u32 off) {
    switch (off) {
        case 0x010: case 0x418: return "object pool";
        case 0x824: case 0x828: return "texture";
        case 0x82C: case 0x830: case 0x834: case 0x838: case 0x83C: case 0x840: return "effect definitions";
        case 0x844: return "action table";
        case 0xD94: return "texture list";
        case 0xD98: return "screen list";
        case 0xD9C: return "HUD";
        case 0xDA8: return "string table";
        case 0xDB0: return "UI root";
        case 0xDB4: return "font list";
        case 0xDB8: return "UI draw";
        case 0xDBC: return "UI pad";
        case 0xDC0: return "screen sets";
        case 0xDC4: return "input table";
        case GLOBAL_BINARY_HDR_GAMEDB: return "game database";
    }
    return NULL;
}

// Tables the decompiled code reads at a fixed place of GameDb::data (include/game/part.h); the others have
// only their layout hash.
const char* TableName(int t) {
    switch (t) {
        case 0: return "part action types";   // +0x008
        case 1: return "part actions";        // +0x430
        case 22: return "parts";              // +0x5B78 PartDef*
    }
    return NULL;
}

const char* kGxFormat(u32 f) {
    static const char* const k[] = {"I4", "I8", "IA4", "IA8", "RGB565", "RGB5A3", "RGBA8", "?7", "C4", "C8", "C14X2"};
    return f < 11 ? k[f] : f == 14 ? "CMPR" : "?";
}

// A texture stream section (sir_loader.cpp TextureStreamDims, fn_801840E4): {"SG21" "WI00", two words, u32 size,
// name, format byte and three more, padding to 4, u16 width, u16 height, pixels, palette}.
u32 StreamTexFormatAt(const u8* d, u32 size) {            // offset of the format byte, or 0
    u32 end = 0x10 + Be32(d + 0x10), p = 0x14;
    if (end > size) return 0;
    while (p < end && d[p]) p++;
    return p + 1 < end ? p + 1 : 0;
}
u32 StreamTexDims(const u8* d, u32 size) {                 // offset of the width, or 0
    if (size < 0x20) return 0;
    u32 rec = Be32(d + 0x10);
    if (rec < 12 || rec > size - 0x10) return 0;
    u32 p = StreamTexFormatAt(d, size);
    if (!p) return 0;
    u32 q = (p + 7) & ~3u;                                   // as fn_801840E4 rounds the cursor (p - 1 + 8)
    return q + 4 <= 0x10 + rec ? q : 0;
}

// ---------------------------------------------------------------------------------------- textures
struct TexRef {
    std::string name;
    u32 w = 0, h = 0, fmt = 0, tlutFmt = 0, mips = 1;
    const u8* data = NULL;            // level 0 in GX layout, as on the disc
    const u8* tlut = NULL;            // big-endian palette entries, or NULL
    u32 bytes = 0;                    // of level 0
    std::vector<u8> tlutCopy;         // a stream texture's palette, padded to the whole GX palette
};

BOOL IsPaletted(u32 fmt) { return fmt == 8 || fmt == 9 || fmt == 10; }
u32 PaletteBytes(u32 fmt) { return fmt == 8 ? 32 : fmt == 9 ? 512 : fmt == 10 ? 0x8000 : 0; }

// A texture stream record as the game builds its header from it: fn_801840E4 reads {engine format, palette
// kind, flags, ...}; fn_801499F8 gives the palette size, fn_8015A5A4 the GX format, the palette format, the
// sizes rounded to powers of two and the mip levels when flag 4 is set. Pixels of every level, then the palette.
BOOL StreamTex(const u8* sec, u32 size, TexRef* t) {
    u32 q = StreamTexDims(sec, size), p = StreamTexFormatAt(sec, size);
    if (!q) return FALSE;
    u32 engine = sec[p], pal = sec[p + 1], flags = sec[p + 2];
    u32 w = (u32)(sec[q] << 8 | sec[q + 1]), h = (u32)(sec[q + 2] << 8 | sec[q + 3]);
    u32 entries = engine == 4 || engine == 11 ? 16 : engine == 5 ? 256 : engine == 12 ? 32 : engine == 13 ? 8 : 0;
    if (entries) {
        t->fmt = engine == 11 ? 8 : 9;
        t->tlutFmt = pal == 4 ? 0 : pal == 6 ? 2 : 1;
    } else {
        switch (engine) {
            case 8: t->fmt = 14; break;  case 15: t->fmt = 0; break;  case 16: t->fmt = 2; break;
            case 17: t->fmt = 1; break;  case 18: t->fmt = 3; break;  case 19: t->fmt = 4; break;
            case 23: t->fmt = 5; break;  default: t->fmt = 6; break;
        }
    }
    u32 levels = 1, total = 0;
    if (flags & 4) {                                         // mip-mapped: powers of two
        u32 lw = 0, lh = 0;
        for (s32 v = (s32)w - 1; v > 0; v >>= 1) lw++;
        for (s32 v = (s32)h - 1; v > 0; v >>= 1) lh++;
        w = 1u << lw; h = 1u << lh;
        levels = (lw < lh ? lw : lh) + 1;
        if (engine == 8) levels = levels >= 3 ? levels - 2 : 1;
    }
    for (u32 i = 0; i < levels; i++) total += PortGxTextureBytes(w >> i ? w >> i : 1, h >> i ? h >> i : 1, t->fmt);
    t->name = (const char*)sec + 0x14;
    t->w = w; t->h = h; t->mips = levels;
    t->data = sec + q + 4;
    t->bytes = PortGxTextureBytes(w, h, t->fmt);
    u32 palBytes = entries * (pal == 3 ? 4 : 2);
    if (q + 4 + total + palBytes > size) return FALSE;
    if (entries) {                                           // the renderer reads a whole C4/C8 palette: pad a short one
        t->tlutCopy.assign(t->data + total, t->data + total + palBytes);
        t->tlutCopy.resize(PaletteBytes(t->fmt) > palBytes ? PaletteBytes(t->fmt) : palBytes, 0);
        t->tlut = t->tlutCopy.data();
    }
    return TRUE;
}

BOOL TexOf(const Doc* d, const Node& n, TexRef* t) {
    if (n.kind == K_TEX) {
        const SirTexture* tx = (const SirTexture*)(d->image.data() + n.a);
        t->name = tx->name; t->w = tx->size[0]; t->h = tx->size[1]; t->fmt = tx->format;
        t->tlutFmt = tx->tlutFormat & 3; t->mips = tx->mipCount ? tx->mipCount : 1;
        t->data = tx->data; t->tlut = IsPaletted(t->fmt) ? tx->palette : NULL;
        if (d->Off(tx->data) == ~0u || (t->tlut && d->Off(t->tlut) == ~0u)) return FALSE;
        t->bytes = PortGxTextureBytes(t->w, t->h, t->fmt);
    } else if (n.kind == K_STEX) {
        if (!StreamTex(d->raw.data() + n.off, n.size, t)) return FALSE;
    } else return FALSE;
    return t->w && t->h && t->w <= 4096 && t->h <= 4096 && (!IsPaletted(t->fmt) || t->tlut);
}

BOOL Decode(const TexRef& t, std::vector<u32>* rgba) {
    rgba->assign((size_t)t.w * t.h, 0);
    return PortGxDecodeTexture(t.data, t.w, t.h, t.fmt, t.tlut, t.tlutFmt, rgba->data());
}

// The name --dump-textures gives it (PortModsDumpTexture): a mod's textures folder matches the content hash in
// it, or <name>.png once renamed.
std::string PackName(const TexRef& t) {
    u64 hash = PortModsTextureHash(t.data, t.bytes, t.tlut, t.tlut ? PaletteBytes(t.fmt) : 0, t.w, t.h, t.fmt);
    std::string clean = t.name;
    for (char& c : clean) if (strchr("\\/:*?\"<>|", c) || (u8)c < 32) c = '_';
    char b[200];
    if (clean.empty()) snprintf(b, sizeof(b), "tex_%ux%u_f%u_%016llX", t.w, t.h, t.fmt, (unsigned long long)hash);
    else snprintf(b, sizeof(b), "tex_%ux%u_f%u_%.80s_%016llX", t.w, t.h, t.fmt, clean.c_str(), (unsigned long long)hash);
    return b;
}

// BC1 (opaque) or BC3 (with alpha) DDS of one level, which port_dds.cpp reads. Plain end points: the box of the
// block's colours, then the nearest of the steps. Width and height must be multiples of 4.
u16 To565(u32 c) { return (u16)(((c & 0xF8) << 8) | ((c >> 5) & 0x7E0) | ((c >> 19) & 0x1F)); }
u32 From565x(u16 v) {
    u32 r = v >> 11, g = v >> 5 & 63, b = v & 31;
    return (r << 3 | r >> 2) | (g << 2 | g >> 4) << 8 | (b << 3 | b >> 2) << 16;
}
void Bc1Block(const u32* px, u8* out) {
    u32 lo[3] = {255, 255, 255}, hi[3] = {0, 0, 0};
    for (int i = 0; i < 16; i++)
        for (int k = 0; k < 3; k++) { u32 v = px[i] >> (k * 8) & 255; if (v < lo[k]) lo[k] = v; if (v > hi[k]) hi[k] = v; }
    u16 c0 = To565(hi[0] | hi[1] << 8 | hi[2] << 16), c1 = To565(lo[0] | lo[1] << 8 | lo[2] << 16);
    if (c0 < c1) { u16 x = c0; c0 = c1; c1 = x; }
    u32 pal[4] = {From565x(c0), From565x(c1), 0, 0};
    for (int k = 0; k < 3; k++) {
        u32 a = pal[0] >> (k * 8) & 255, b = pal[1] >> (k * 8) & 255;
        pal[2] |= ((2 * a + b) / 3) << (k * 8);
        pal[3] |= ((a + 2 * b) / 3) << (k * 8);
    }
    u32 idx = 0;
    for (int i = 0; i < 16; i++) {
        int best = 0, bestD = 1 << 30;
        for (int j = 0; j < (c0 == c1 ? 1 : 4); j++) {
            int dd = 0;
            for (int k = 0; k < 3; k++) { int e = (int)(px[i] >> (k * 8) & 255) - (int)(pal[j] >> (k * 8) & 255); dd += e * e; }
            if (dd < bestD) { bestD = dd; best = j; }
        }
        idx |= (u32)best << (i * 2);
    }
    out[0] = (u8)c0; out[1] = (u8)(c0 >> 8); out[2] = (u8)c1; out[3] = (u8)(c1 >> 8);
    memcpy(out + 4, &idx, 4);
}
void AlphaBlock(const u32* px, u8* out) {
    static const u8 kCode[8] = {0, 2, 3, 4, 5, 6, 7, 1};    // step 0 is a0, step 7 is a1
    u32 lo = 255, hi = 0;
    for (int i = 0; i < 16; i++) { u32 a = px[i] >> 24; if (a < lo) lo = a; if (a > hi) hi = a; }
    out[0] = (u8)hi; out[1] = (u8)lo;
    u64 bits = 0;
    for (int i = 0; i < 16; i++) {
        u32 a = px[i] >> 24, step = hi == lo ? 0 : ((hi - a) * 7 + (hi - lo) / 2) / (hi - lo);
        bits |= (u64)kCode[step] << (i * 3);
    }
    for (int k = 0; k < 6; k++) out[2 + k] = (u8)(bits >> (k * 8));
}
BOOL WriteDds(const char* path, const u32* rgba, u32 w, u32 h) {
    if (!w || !h || (w & 3) || (h & 3)) return FALSE;
    BOOL alpha = FALSE;
    for (u32 i = 0; i < w * h && !alpha; i++) alpha = (rgba[i] >> 24) != 255;
    u32 blockBytes = alpha ? 16 : 8;
    std::vector<u8> data((size_t)(w / 4) * (h / 4) * blockBytes);
    u8* o = data.data();
    for (u32 by = 0; by < h; by += 4)
        for (u32 bx = 0; bx < w; bx += 4, o += blockBytes) {
            u32 px[16];
            for (u32 y = 0; y < 4; y++) for (u32 x = 0; x < 4; x++) px[y * 4 + x] = rgba[(by + y) * w + bx + x];
            if (alpha) { AlphaBlock(px, o); Bc1Block(px, o + 8); }
            else Bc1Block(px, o);
        }
    u8 head[128] = {};
    auto le = [&](u32 off, u32 v) { memcpy(head + off, &v, 4); };
    memcpy(head, "DDS ", 4);
    le(4, 124); le(8, 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000); le(12, h); le(16, w); le(20, (u32)data.size()); le(28, 1);
    le(76, 32); le(80, 4); memcpy(head + 84, alpha ? "DXT5" : "DXT1", 4); le(108, 0x1000);
    FILE* f = fopen(path, "wb");
    if (!f) return FALSE;
    BOOL ok = fwrite(head, 1, 128, f) == 128 && fwrite(data.data(), 1, data.size(), f) == data.size();
    fclose(f);
    return ok;
}

// Writes <dir>\<pack name>.png (and .dds when it can); the message for the status line.
std::string ExportTexture(const TexRef& t, const char* dir, BOOL dds) {
    std::vector<u32> rgba;
    if (!Decode(t, &rgba)) return "this texture format is not decoded";
    CreateDirectoryA(dir, NULL);
    std::string base = std::string(dir) + "\\" + PackName(t);
    if (!PortPngWrite((base + ".png").c_str(), rgba.data(), t.w, t.h)) return "cannot write " + base + ".png";
    if (dds && !WriteDds((base + ".dds").c_str(), rgba.data(), t.w, t.h)) return "wrote " + base + ".png (no DDS: the size is not a multiple of 4)";
    return "wrote " + base + (dds ? ".png and .dds" : ".png");
}

std::string SectionLabel(int i, const SirSection& s, BOOL level) {
    char b[96];
    const char* what = s.type == -1 ? "header block" : s.type == 0 ? (level ? "zone 0" : "image") : NULL;
    if (what) snprintf(b, sizeof(b), "section %d: %s, 0x%X bytes", i, what, s.size);
    else snprintf(b, sizeof(b), "section %d: %s %d, 0x%X bytes", i, level ? "zone" : "type", s.type, s.size);
    return b;
}

void BuildGlobalBinary(Doc* d, int root) {
    const SirSection& hs = d->secs[0];
    const SirSection& is = d->secs[1];
    d->hdrOff = hs.offset;
    d->imgOff = is.offset;
    d->hdr.assign(d->raw.begin() + hs.offset, d->raw.begin() + hs.offset + GLOBAL_BINARY_HDR_SIZE);
    d->image.assign(d->raw.begin() + is.offset, d->raw.begin() + is.offset + is.size);
    SirReloc r;
    SirRelocInit(&r, NULL, NULL);
    s32 fail = -1;
    void (*note)(const void*, const char*) = gSirTextureNote;
    gSirTextureNote = NULL;                              // not the game's textures: the renderer must not learn their names
    SirError e = GlobalBinaryConvert(&r, d->hdr.data(), d->image.data(), is.size, &fail);
    gSirTextureNote = note;
    if (e != SIR_OK) {
        char b[160];
        snprintf(b, sizeof(b), "the loader refuses it: %s (table %d)", SirErrorText(e), fail);
        d->error = b;
        d->hdr.clear(); d->image.clear();
        return;
    }
    d->converted = TRUE;
    // Header slots, from the disc bytes (Wii addresses).
    int hn = Add(d, root, K_HDR, "header slots", hs.offset, GLOBAL_BINARY_HDR_SIZE);
    for (u32 off = 0; off < GLOBAL_BINARY_HDR_SIZE; off += 4) {
        u32 v = Be32(&d->raw[hs.offset + off]);
        if (!v) continue;
        char b[96];
        const char* name = SlotName(off);
        snprintf(b, sizeof(b), "0x%03X %s%s%s", off, name ? name : "", name ? " " : "", Hex32(v).c_str());
        Add(d, hn, K_SLOT, b, hs.offset + off, 4, (int)off);
    }
    GameDbView* gdb = NULL;
    memcpy(&gdb, d->hdr.data() + GLOBAL_BINARY_HDR_GAMEDB, 4);
    if (gdb && d->Off(gdb) != ~0u && d->Off(gdb->data) != ~0u) {
        d->db = gdb->data;
        int dn = Add(d, root, K_DB, "game database (42 tables)", d->Off(d->db), GAMEDB_TABLE_COUNT * GAMEDB_DESC_SIZE);
        for (int t = 0; t < GAMEDB_TABLE_COUNT; t++) {
            const GameDbTable* tb = d->Table(t);
            char b[160];
            const char* name = TableName(t);
            snprintf(b, sizeof(b), "table %d: %s (%u rows, %u columns)", t, name ? name : HashText(tb->layoutHash).c_str(), tb->count, tb->fieldCount);
            Add(d, dn, K_TABLE, b, d->Off(tb), GAMEDB_DESC_SIZE, t);
        }
    }
    u8* texHead = NULL;
    memcpy(&texHead, d->hdr.data() + 0xD94, 4);
    if (texHead && d->Off(texHead) != ~0u) {
        const SirTextureList* list = (const SirTextureList*)(texHead + 4);
        char b[96];
        snprintf(b, sizeof(b), "textures (%u)", list->count);
        int tn = Add(d, root, K_TEXLIST, b, d->Off(list), sizeof(SirTextureList));
        u32 n = 0;
        for (u8* node = list->head; node && d->Off(node) != ~0u && n < list->count; node = ((SirTexture*)(node - SIR_TEXTURE_NODE))->next, n++) {
            const SirTexture* tx = (const SirTexture*)(node - SIR_TEXTURE_NODE);
            snprintf(b, sizeof(b), "%.48s  %ux%u %s", tx->name, tx->size[0], tx->size[1], kGxFormat(tx->format));
            Add(d, tn, K_TEX, b, d->Off(tx), SIR_TEXTURE_HEADER, (int)(d->Off(tx) - d->imgOff));
        }
    }
    memcpy(&d->strings, d->hdr.data() + 0xDA8, 4);
    if (d->strings && d->Off(d->strings) != ~0u) {
        char b[64];
        snprintf(b, sizeof(b), "string table (%u)", d->strings->count);
        Add(d, root, K_STRINGS, b, d->Off(d->strings), sizeof(SirStringTable));
        for (u32 i = 0; i < d->strings->count; i++)
            if (d->Off(d->strings->strings[i]) != ~0u) Dict().emplace(NameHash(d->strings->strings[i]), d->strings->strings[i]);
    } else d->strings = NULL;
}

Doc* BuildDoc(const std::string& path, const std::string& from, std::vector<u8>&& bytes) {
    Doc* d = new Doc;
    d->path = path;
    d->from = from;
    d->raw = std::move(bytes);
    u32 n = (u32)d->raw.size();
    const char* base = strrchr(path.c_str(), '/');
    base = base ? base + 1 : path.c_str();
    int root = Add(d, -1, K_FILE, base, 0, n);
    s32 count = SirContainerSectionCount(d->raw.data(), n);
    if (count < 0) { d->error = "not a .sir container (no PNP0 header)"; return d; }
    DictScan(d->raw.data(), d->raw.size());
    Add(d, root, K_CONTAINER, "container header", 0, SIR_SECTION_TABLE);
    BOOL level = strstr(base, "_level") || strstr(base, "_arena") || strstr(base, "castle_") || strstr(base, "trouble_scope") ||
                 strstr(base, "shared_parts") || strstr(base, "shared_ui");
    d->zones = level;
    char b[64];
    snprintf(b, sizeof(b), "sections (%d)", count);
    int sn = Add(d, root, K_SECTIONS, b, SIR_SECTION_TABLE, (u32)count * SIR_SECTION_ENTRY);
    for (s32 i = 0; i < count; i++) {
        SirSection s = {};
        if (!SirContainerSection(d->raw.data(), n, i, &s)) { d->error = "a section lies outside the file"; s.offset = s.size = 0; }
        d->secs.push_back(s);
        std::string label = SectionLabel(i, s, level);
        int kind = K_SECTION;
        const u8* sec = d->raw.data() + s.offset;
        u32 q = s.size >= 0x20 && !memcmp(sec, "SG21WI00", 8) ? StreamTexDims(sec, s.size) : 0;
        if (q) {                                             // a texture of its own: name, size, format
            char lb[160];
            TexRef r;
            if (StreamTex(sec, s.size, &r)) snprintf(lb, sizeof(lb), "%.60s  %ux%u %s", r.name.c_str(), r.w, r.h, kGxFormat(r.fmt));
            else snprintf(lb, sizeof(lb), "%.60s  (texture record not understood)", (const char*)sec + 0x14);
            label = lb;
            kind = K_STEX;
        }
        Add(d, sn, kind, label, s.offset, s.size, i);
    }
    if (!strcmp(base, "global_binary.sir") && count == 2 && d->secs[0].type == -1 && d->secs[0].size == GLOBAL_BINARY_HDR_SIZE &&
        d->secs[1].type == 0 && d->secs[1].size)
        BuildGlobalBinary(d, root);
    return d;
}

// ---------------------------------------------------------------------------------------- files
std::string Lower(std::string s) { for (char& c : s) c = (char)tolower((u8)c); return s; }

BOOL ReadHost(const char* path, std::vector<u8>* out) {
    FILE* f = fopen(path, "rb");
    if (!f) return FALSE;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    out->resize(len > 0 ? (size_t)len : 0);
    BOOL ok = len >= 0 && fread(out->data(), 1, out->size(), f) == out->size();
    fclose(f);
    return ok;
}

// A file of the disc by its path ("streams/global_binary.sir"): the last enabled mod's copy, else the disc.
BOOL ReadDiscFile(const std::string& discPath, std::vector<u8>* out, std::string* from) {
    char host[MAX_PATH];
    const char* mod = NULL;
    if (!gPortOptions.noMods && PortModsFindFile(discPath.c_str(), host, sizeof(host), &mod) && ReadHost(host, out)) {
        *from = std::string("mod ") + (mod ? mod : "?");
        return TRUE;
    }
    if (PortDiscIsOpen()) {
        u64 off;
        u32 size;
        if (!PortDiscFind(discPath.c_str(), &off, &size)) return FALSE;
        out->resize(size);
        *from = "disc image";
        return PortDiscRead(off, out->data(), size);
    }
    if (!gPortOptions.dataDir) return FALSE;
    std::string p = std::string(gPortOptions.dataDir) + "\\files\\" + discPath;
    for (char& c : p) if (c == '/') c = '\\';
    *from = "extracted disc";
    return ReadHost(p.c_str(), out);
}

struct Tool {
    std::vector<std::string> files;   // disc paths of the .sir files
    Doc* doc = NULL;
    int sel = 0;                      // node
    int row = -1;                     // table row / string id picked in the details
    int hover = -1;                   // field under the mouse
    std::string filter, status;
    int openFrames = 0;               // frames that open the tree down to the selection
    HWND wnd = NULL;
    BOOL quit = FALSE;
    u32 resizeW = 0, resizeH = 0;
    // texture view
    ID3D11Device* dev = NULL;
    ID3D11ShaderResourceView* srv = NULL;
    const Doc* texDoc = NULL;
    int texNode = -1;
    u32 texW = 0, texH = 0;
    f32 zoom = 0;                     // 0: fit
    char exportDir[MAX_PATH] = "";
    std::unordered_map<int, ID3D11ShaderResourceView*> thumbs;   // node -> picture, for the gallery of the open file
    // model view
    ID3D11DeviceContext* ctx = NULL;
    MtView* view = NULL;
    MtZone zone;
    const Doc* zoneDoc = NULL;
    int zoneNode = -1;
    int batch = -1;                   // -1: the whole zone
    MtCamera cam;
};

ID3D11ShaderResourceView* MakeSrv(ID3D11Device* dev, const TexRef& r) {
    std::vector<u32> rgba;
    if (!dev || !Decode(r, &rgba)) return NULL;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = r.w; td.Height = r.h; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd = {rgba.data(), r.w * 4, 0};
    ID3D11Texture2D* tx = NULL;
    ID3D11ShaderResourceView* srv = NULL;
    if (SUCCEEDED(dev->CreateTexture2D(&td, &sd, &tx))) {
        dev->CreateShaderResourceView(tx, NULL, &srv);
        tx->Release();
    }
    return srv;
}

void DropThumbs(struct Tool* t);
Tool* sTool;

void ListFiles(Tool* t) {
    t->files.clear();
    std::vector<u8> cat;
    std::string from;
    if (ReadDiscFile("catalog.sir", &cat, &from)) {
        SirCatalog c = {};
        if (SirCatalogConvert(cat.data(), (u32)cat.size(), &c) == SIR_OK)
            for (u32 i = 0; i < c.fileCount; i++) {
                char name[0x39];
                memcpy(name, c.files[i].name, 0x38);
                name[0x38] = 0;
                t->files.push_back(std::string("streams/") + name);
            }
    }
    t->files.push_back("catalog.sir");
}

void OpenDoc(Tool* t, const std::string& path) {
    std::vector<u8> bytes;
    std::string from;
    BOOL ok = ReadDiscFile(path, &bytes, &from);
    if (!ok && ReadHost(path.c_str(), &bytes)) { ok = TRUE; from = "file"; }
    if (!ok) { t->status = "cannot read " + path; return; }
    std::string p = path;
    for (char& c : p) if (c == '\\') c = '/';
    DropThumbs(t);
    delete t->doc;
    t->doc = BuildDoc(p, from, std::move(bytes));
    t->sel = 0;
    t->row = -1;
    t->status = t->doc->error;
}

// Picks the first node whose label starts with `what` (headless runs, --modtool-select).
void Select(Tool* t, const char* what) {
    if (!t->doc || !what) return;
    for (size_t i = 0; i < t->doc->nodes.size(); i++)
        if (!strncmp(t->doc->nodes[i].label.c_str(), what, strlen(what))) { t->sel = (int)i; t->openFrames = 2; return; }
}

// ---------------------------------------------------------------------------------------- fields
void Push(std::vector<Field>* f, const char* name, const char* type, const std::string& value, u32 off, u32 size) {
    f->push_back(Field{name, type, value, off, size});
}

std::string Num(const char* fmt, double v) { char b[64]; snprintf(b, sizeof(b), fmt, v); return b; }
std::string Int(s64 v) { char b[32]; snprintf(b, sizeof(b), "%lld", (long long)v); return b; }

const char* kColType(u32 t) {
    switch (t) {
        case GAMEDB_T_INT: return "int"; case GAMEDB_T_FLOAT: return "float"; case GAMEDB_T_HASH: return "hash";
        case GAMEDB_T_SHORT: return "short"; case GAMEDB_T_FLAG: return "flag"; case GAMEDB_T_BYTE: return "byte";
        case GAMEDB_T_INT2: return "int";
    }
    return "?";
}

// The column's name as a patch writes it: the game's name, else #index.
std::string ColName(const GameDbTable* tb, u32 i) {
    auto it = Dict().find(tb->fieldName[i]);
    return it != Dict().end() ? it->second : "#" + Int(i);
}

void RowFields(const Doc* d, int t, int row, std::vector<Field>* out) {
    const GameDbTable* tb = d->Table(t);
    if (!tb || row < 0 || (u32)row >= tb->count) return;
    const u8* r = tb->rows + (u32)row * tb->rowSize;
    u32 base = d->Off(r), off = 0;
    for (u32 i = 0; i < tb->fieldCount && i < GAMEDB_MAX_FIELDS; i++) {
        u32 size = tb->fieldSize[i];
        if (size != 1 && size != 2 && size != 4) break;
        off = (off + size - 1) & ~(size - 1);
        if (off + size > tb->rowSize) break;
        const u8* p = r + off;
        std::string v;
        switch (tb->fieldType[i]) {
            case GAMEDB_T_FLOAT: v = Num("%g", *(const f32*)p); break;
            case GAMEDB_T_HASH: v = HashText(*(const u32*)p); break;
            case GAMEDB_T_SHORT: v = Int(*(const s16*)p); break;
            case GAMEDB_T_FLAG: case GAMEDB_T_BYTE: v = Int(*p); break;
            default: v = Int(*(const s32*)p); break;
        }
        if (tb->fieldRef[i] != 0xFFFFFFFFu && tb->fieldType[i] != GAMEDB_T_HASH) v += "  -> " + HashText(tb->fieldRef[i]);
        out->push_back(Field{ColName(tb, i), kColType(tb->fieldType[i]), v, base + off, size});
        off += size;
    }
}

void NodeFields(const Tool* t, std::vector<Field>* f) {
    const Doc* d = t->doc;
    const Node& n = d->nodes[t->sel];
    const u8* raw = d->raw.data();
    switch (n.kind) {
        case K_FILE: case K_CONTAINER:
            if (d->raw.size() < SIR_SECTION_TABLE || SirContainerSectionCount(raw, (u32)d->raw.size()) < 0) break;
            Push(f, "magic", "char[4]", "PNP0", 0, 4);
            for (u32 o = 4; o < 0x14; o += 4)
                Push(f, o == 8 ? "section count" : ("+" + Hex32(o).substr(6)).c_str(), "u32", Int(Be32(raw + o)), o, 4);
            break;
        case K_SECTION: {
            u32 e = SIR_SECTION_TABLE + (u32)n.a * SIR_SECTION_ENTRY;
            const SirSection& s = d->secs[n.a];
            Push(f, "type", "s32", Int(s.type), e, 4);
            Push(f, "offset", "u32", Hex32(s.offset), e + 4, 4);
            Push(f, "size", "u32", Hex32(s.size), e + 8, 4);
            Push(f, "+0x0C", "u32", Hex32(s.unk0C), e + 12, 4);
            Push(f, "+0x10", "u32", Hex32(s.unk10), e + 16, 4);
            Push(f, "hash", "u32", HashText(s.hash), e + 20, 4);
            break;
        }
        case K_SLOT: {
            u32 v = Be32(raw + n.off);
            const char* name = SlotName((u32)n.a);
            Push(f, name ? name : "slot", "pointer", Hex32(v), n.off, 4);
            if (v >= GLOBAL_BINARY_WII_BASE && v - GLOBAL_BINARY_WII_BASE < d->image.size())
                Push(f, "points at", "file offset", Hex32(d->imgOff + (v - GLOBAL_BINARY_WII_BASE)), d->imgOff + (v - GLOBAL_BINARY_WII_BASE), 4);
            break;
        }
        case K_TABLE: {
            const GameDbTable* tb = d->Table(n.a);
            u32 o = n.off;
            if (t->row >= 0) {                                   // the picked row first, then the table's layout
                RowFields(d, n.a, t->row, f);
                f->push_back(Field{"", "", "", 0, 0});
            }
            Push(f, "rows", "pointer", Hex32(d->Off(tb->rows)), o + 0x08, 4);
            Push(f, "count", "u32", Int(tb->count), o + 0x0C, 4);
            Push(f, "layout", "hash", HashText(tb->layoutHash), o + 0x14, 4);
            Push(f, "row size", "u32", Int(tb->rowSize), o + 0x18, 4);
            Push(f, "columns", "u32", Int(tb->fieldCount), o + 0x420, 4);
            for (u32 i = 0; i < tb->fieldCount && i < GAMEDB_MAX_FIELDS; i++) {
                std::string v = std::string(kColType(tb->fieldType[i])) + ", " + Int(tb->fieldSize[i]) + " bytes";
                if (tb->fieldRef[i] != 0xFFFFFFFFu) v += ", refers to " + HashText(tb->fieldRef[i]);
                Push(f, ("column " + ColName(tb, i)).c_str(), "column", v, o + 0x220 + i * 4, 4);
            }
            break;
        }
        case K_TEX: {
            const SirTexture* tx = (const SirTexture*)(d->image.data() + n.a);
            u32 o = n.off;
            Push(f, "name", "char[]", tx->name, o + 0x35, (u32)strlen(tx->name) + 1);
            Push(f, "width", "u16", Int(tx->size[0]), o + 0x04, 2);
            Push(f, "height", "u16", Int(tx->size[1]), o + 0x06, 2);
            Push(f, "format", "u8", std::string(kGxFormat(tx->format)) + " (" + Int(tx->format) + ")", o + 0x14, 1);
            Push(f, "mip levels", "u8", Int(tx->mipCount), o + 0x13, 1);
            Push(f, "palette format", "u8", Int(tx->tlutFormat), o + 0x15, 1);
            Push(f, "data size", "u32", Hex32(tx->dataSize), o + 0x18, 4);
            Push(f, "data", "pointer", Hex32(d->Off(tx->data)), o + 0x1C, 4);
            Push(f, "palette", "pointer", tx->palette ? Hex32(d->Off(tx->palette)) : "none", o + 0x20, 4);
            TexRef r;
            if (TexOf(d, n, &r)) Push(f, "texture pack name", "", PackName(r), 0, 0);
            break;
        }
        case K_STEX: {
            const u8* sec = raw + n.off;
            u32 q = StreamTexDims(sec, n.size), p = StreamTexFormatAt(sec, n.size);
            Push(f, "tags", "char[8]", "SG21 WI00", n.off, 8);
            Push(f, "record size", "u32", Hex32(Be32(sec + 0x10)), n.off + 0x10, 4);
            Push(f, "name", "char[]", (const char*)sec + 0x14, n.off + 0x14, p - 0x14);
            TexRef g;
            BOOL ok = StreamTex(sec, n.size, &g);
            Push(f, "engine format", "u8", Int(sec[p]) + (ok ? std::string(" (GX ") + kGxFormat(g.fmt) + ")" : ""), n.off + p, 1);
            Push(f, "palette kind", "u8", Int(sec[p + 1]), n.off + p + 1, 1);
            Push(f, "flags", "u8", Int(sec[p + 2]) + (sec[p + 2] & 4 ? " (mip-mapped)" : ""), n.off + p + 2, 1);
            Push(f, "+3", "u8", Int(sec[p + 3]), n.off + p + 3, 1);
            if (ok && g.mips > 1) Push(f, "mip levels", "", Int(g.mips), 0, 0);
            Push(f, "width", "u16", Int(sec[q] << 8 | sec[q + 1]), n.off + q, 2);
            Push(f, "height", "u16", Int(sec[q + 2] << 8 | sec[q + 3]), n.off + q + 2, 2);
            TexRef r;
            if (TexOf(d, n, &r)) {
                Push(f, "pixels", "GX", Int(r.bytes) + " bytes", n.off + q + 4, r.bytes);
                Push(f, "texture pack name", "", PackName(r), 0, 0);
            }
            break;
        }
        case K_STRINGS:
            Push(f, "count", "u32", Int(d->strings->count), n.off + 0x0C, 4);
            if (t->row >= 0 && (u32)t->row < d->strings->count) {
                const char* s = d->strings->strings[t->row];
                Push(f, ("string " + Int(t->row)).c_str(), "char[]", s, d->Off(s), (u32)strlen(s) + 1);
                std::string json = "\"" + Int(t->row) + "\": \"";          // the line a mod's data/strings.json takes
                for (const char* c = s; *c; c++) {
                    if (*c == '"' || *c == '\\') json += '\\';
                    if (*c == '\n') { json += "\\n"; continue; }
                    json += *c;
                }
                Push(f, "in strings.json", "", json + "\"", 0, 0);
            }
            break;
    }
}

// ---------------------------------------------------------------------------------------- drawing
void DrawHex(const Doc* d, u32 off, u32 size, const Field* hi) {
    if (off >= d->raw.size()) return;
    if (size > d->raw.size() - off) size = (u32)d->raw.size() - off;
    u32 first = off & ~15u, lines = (off + size - first + 15) / 16;
    ImGuiListClipper clip;
    clip.Begin((int)lines);
    ImVec4 dim = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), lit(1.0f, 0.82f, 0.35f, 1.0f);
    while (clip.Step())
        for (int l = clip.DisplayStart; l < clip.DisplayEnd; l++) {
            u32 a = first + (u32)l * 16;
            ImGui::TextDisabled("%08X ", a);
            for (u32 k = 0; k < 16; k++) {
                ImGui::SameLine(0, k == 8 ? 10 : 4);
                u32 x = a + k;
                if (x >= d->raw.size()) { ImGui::TextUnformatted("  "); continue; }
                BOOL inside = x >= off && x < off + size;
                BOOL hot = hi && x >= hi->off && x < hi->off + hi->size;
                ImGui::TextColored(hot ? lit : inside ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : dim, "%02X", d->raw[x]);
            }
            char asc[17];
            for (u32 k = 0; k < 16; k++) { u32 x = a + k; u8 c = x < d->raw.size() ? d->raw[x] : ' '; asc[k] = c >= 0x20 && c < 0x7F ? (char)c : '.'; }
            asc[16] = 0;
            ImGui::SameLine(0, 14);
            ImGui::TextDisabled("%s", asc);
        }
}

void DrawTree(Tool* t, int i) {
    const Node& n = t->doc->nodes[i];
    ImGuiTreeNodeFlags fl = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (n.kids.empty()) fl |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (i == t->sel) fl |= ImGuiTreeNodeFlags_Selected;
    if (i == 0) fl |= ImGuiTreeNodeFlags_DefaultOpen;
    if (t->openFrames && !n.kids.empty()) {                // open the way to the selection
        for (int p = t->doc->nodes[t->sel].parent; p >= 0; p = t->doc->nodes[p].parent)
            if (p == i) { ImGui::SetNextItemOpen(true); break; }
    }
    if (t->openFrames && i == t->sel) ImGui::SetScrollHereY(0.3f);
    BOOL open = ImGui::TreeNodeEx((void*)(intptr_t)i, fl, "%s", n.label.c_str());
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) { t->sel = i; t->row = -1; }
    if (open && !n.kids.empty()) {
        for (int k : n.kids) DrawTree(t, k);
        ImGui::TreePop();
    }
}

// The rows of a table or the strings, one line each; a click picks one for the fields.
void DrawList(Tool* t) {
    const Doc* d = t->doc;
    const Node& n = d->nodes[t->sel];
    if (n.kind == K_TABLE) {
        const GameDbTable* tb = d->Table(n.a);
        u32 cols = tb->fieldCount < 12 ? tb->fieldCount : 12;
        if (!cols || !ImGui::BeginTable("rows", (int)cols + 1, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                                        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable, ImVec2(0, ImGui::GetContentRegionAvail().y)))
            return;
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("row");
        for (u32 c = 0; c < cols; c++) ImGui::TableSetupColumn(ColName(tb, c).c_str());
        ImGui::TableHeadersRow();
        ImGuiListClipper clip;
        clip.Begin((int)tb->count);
        std::vector<Field> f;
        while (clip.Step())
            for (int r = clip.DisplayStart; r < clip.DisplayEnd; r++) {
                f.clear();
                RowFields(d, n.a, r, &f);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                char id[16];
                snprintf(id, sizeof(id), "%d", r);
                if (ImGui::Selectable(id, t->row == r, ImGuiSelectableFlags_SpanAllColumns)) t->row = r;
                for (u32 c = 0; c < cols && c < f.size(); c++) { ImGui::TableNextColumn(); ImGui::TextUnformatted(f[c].value.c_str()); }
            }
        ImGui::EndTable();
    } else if (n.kind == K_STRINGS) {
        static char find[64];
        ImGui::SetNextItemWidth(260);
        ImGui::InputTextWithHint("##find", "find text", find, sizeof(find));
        std::vector<int> hits;
        for (u32 i = 0; i < d->strings->count; i++)
            if (!find[0] || strstr(Lower(d->strings->strings[i]).c_str(), Lower(find).c_str())) hits.push_back((int)i);
        ImGui::BeginChild("strings", ImVec2(0, 0), ImGuiChildFlags_Borders);
        if (t->openFrames && t->row >= 0)                     // bring the picked string into view
            for (size_t k = 0; k < hits.size(); k++)
                if (hits[k] == t->row) { ImGui::SetScrollY(k * ImGui::GetTextLineHeightWithSpacing()); break; }
        ImGuiListClipper clip;
        clip.Begin((int)hits.size());
        while (clip.Step())
            for (int k = clip.DisplayStart; k < clip.DisplayEnd; k++) {
                int i = hits[k];
                char line[200];
                snprintf(line, sizeof(line), "%5d  %.180s", i, d->strings->strings[i]);
                if (ImGui::Selectable(line, t->row == i)) t->row = i;
            }
        ImGui::EndChild();
    }
}

// The picked texture, decoded as the renderer does, with zoom and export.
void DropThumbs(Tool* t) {
    for (auto& kv : t->thumbs) if (kv.second) kv.second->Release();
    t->thumbs.clear();
}

// Every texture under the picked node as a picture; a click opens it.
void DrawGallery(Tool* t) {
    const Doc* d = t->doc;
    std::vector<int> kids;
    for (int k : d->nodes[t->sel].kids) if (d->nodes[k].kind == K_TEX || d->nodes[k].kind == K_STEX) kids.push_back(k);
    const f32 cell = 112;
    int cols = (int)(ImGui::GetContentRegionAvail().x / (cell + 8));
    if (cols < 1) cols = 1;
    int rows = ((int)kids.size() + cols - 1) / cols;
    ImGuiListClipper clip;
    clip.Begin(rows, cell + 30);
    int pick = -1;
    while (clip.Step())
        for (int r = clip.DisplayStart; r < clip.DisplayEnd; r++)
            for (int c = 0; c < cols; c++) {
                int i = r * cols + c;
                if (i >= (int)kids.size()) break;
                int k = kids[i];
                auto it = t->thumbs.find(k);
                if (it == t->thumbs.end()) {
                    TexRef ref;
                    it = t->thumbs.emplace(k, TexOf(d, d->nodes[k], &ref) ? MakeSrv(t->dev, ref) : NULL).first;
                }
                if (c) ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::PushID(k);
                if (it->second) {
                    if (ImGui::ImageButton("t", (ImTextureID)(uintptr_t)it->second, ImVec2(cell, cell))) pick = k;
                } else if (ImGui::Button("?", ImVec2(cell + 6, cell + 6))) pick = k;
                ImGui::PopID();
                const std::string& lb = d->nodes[k].label;
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cell);
                ImGui::TextDisabled("%.18s", lb.substr(0, lb.find("  ")).c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndGroup();
            }
    if (pick >= 0) { t->sel = pick; t->openFrames = 2; }
}

void DrawTexture(Tool* t) {
    const Doc* d = t->doc;
    if (t->texDoc != d || t->texNode != t->sel) {          // decode it into a D3D texture once
        if (t->srv) { t->srv->Release(); t->srv = NULL; }
        t->texDoc = d;
        t->texNode = t->sel;
        t->texW = t->texH = 0;
        TexRef r;
        std::vector<u32> rgba;
        if (t->dev && TexOf(d, d->nodes[t->sel], &r) && Decode(r, &rgba)) {
            D3D11_TEXTURE2D_DESC td = {};
            td.Width = r.w; td.Height = r.h; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA sd = {rgba.data(), r.w * 4, 0};
            ID3D11Texture2D* tx = NULL;
            if (SUCCEEDED(t->dev->CreateTexture2D(&td, &sd, &tx))) {
                t->dev->CreateShaderResourceView(tx, NULL, &t->srv);
                tx->Release();
                t->texW = r.w; t->texH = r.h;
            }
        }
    }
    if (!t->srv) { ImGui::TextDisabled("This texture cannot be decoded."); return; }
    if (ImGui::Button("Fit")) t->zoom = 0;
    for (int z : {1, 2, 4}) {
        ImGui::SameLine();
        char b[8];
        snprintf(b, sizeof(b), "%dx", z);
        if (ImGui::Button(b)) t->zoom = (f32)z;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("  %ux%u", t->texW, t->texH);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(320);
    ImGui::InputText("##dir", t->exportDir, sizeof(t->exportDir));
    ImGui::SameLine();
    TexRef r;
    BOOL have = TexOf(d, d->nodes[t->sel], &r);
    if (ImGui::Button("Export PNG") && have) t->status = ExportTexture(r, t->exportDir, FALSE);
    ImGui::SameLine();
    if (ImGui::Button("PNG + DDS") && have) t->status = ExportTexture(r, t->exportDir, TRUE);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    f32 z = t->zoom;
    if (z <= 0) {
        z = avail.x / t->texW < avail.y / t->texH ? avail.x / t->texW : avail.y / t->texH;
        if (z > 8) z = 8;
    }
    ImGui::BeginChild("pic", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar);
    ImVec2 at = ImGui::GetCursorScreenPos(), size(t->texW * z, t->texH * z);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (f32 y = 0; y < size.y; y += 16)                     // a checkerboard behind see-through pixels
        for (f32 x = 0; x < size.x; x += 16)
            dl->AddRectFilled(ImVec2(at.x + x, at.y + y), ImVec2(at.x + (x + 16 < size.x ? x + 16 : size.x), at.y + (y + 16 < size.y ? y + 16 : size.y)),
                              ((int)(x / 16) + (int)(y / 16)) & 1 ? IM_COL32(70, 70, 76, 255) : IM_COL32(48, 48, 52, 255));
    ImGui::Image((ImTextureID)(uintptr_t)t->srv, size);
    ImGui::EndChild();
}

// The meshes of a zone image: the list of batches and the picture, turned with the mouse.
void DrawModel(Tool* t) {
    const Doc* d = t->doc;
    const Node& n = d->nodes[t->sel];
    if (t->zoneDoc != d || t->zoneNode != t->sel) {
        t->zoneDoc = d;
        t->zoneNode = t->sel;
        t->batch = -1;
        MtZoneScan(d->raw.data() + n.off, n.size, &t->zone);
    }
    const MtZone& z = t->zone;
    if (z.batches.empty()) { ImGui::TextDisabled("No model here: %s.", z.error.c_str()); return; }
    ImGui::BeginChild("batches", ImVec2(230, 0), ImGuiChildFlags_Borders);
    ImGui::TextDisabled("zone at 0x%08X", z.base);
    if (ImGui::Selectable("whole zone", t->batch < 0)) t->batch = -1;
    ImGuiListClipper clip;
    clip.Begin((int)z.batches.size());
    while (clip.Step())
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; i++) {
            const MtBatch& b = z.batches[i];
            char lb[96];
            snprintf(lb, sizeof(lb), "list %06X  %u tris%s", b.list, b.count / 3, b.texture != ~0u ? "" : "  (no texture)");
            if (ImGui::Selectable(lb, t->batch == i)) t->batch = i;
        }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginGroup();
    bool tex = t->cam.textured, wire = t->cam.wire;
    ImGui::Checkbox("textures", &tex);
    ImGui::SameLine();
    ImGui::Checkbox("wireframe", &wire);
    ImGui::SameLine();
    if (ImGui::Button("reset view")) { MtCamera c; c.textured = tex; c.wire = wire; t->cam = c; }
    t->cam.textured = tex; t->cam.wire = wire;
    ImGui::SameLine();
    ImGui::TextDisabled("  drag to turn, wheel to zoom; drawn without the skeleton");
    ImVec2 avail = ImGui::GetContentRegionAvail();
    u32 w = avail.x > 64 ? (u32)avail.x : 64, h = avail.y > 64 ? (u32)avail.y : 64;
    if (!t->view && t->dev) t->view = MtViewCreate(t->dev);
    ID3D11ShaderResourceView* pic = t->view ? MtViewDraw(t->view, t->ctx, &t->zone, d->raw.data() + n.off, n.size, z, t->batch, w, h, t->cam) : NULL;
    if (pic) {
        ImGui::Image((ImTextureID)(uintptr_t)pic, ImVec2((f32)w, (f32)h));
        if (ImGui::IsItemHovered()) {
            ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                t->cam.yaw -= io.MouseDelta.x * 0.01f;
                t->cam.pitch += io.MouseDelta.y * 0.01f;
                if (t->cam.pitch > 1.5f) t->cam.pitch = 1.5f;
                if (t->cam.pitch < -1.5f) t->cam.pitch = -1.5f;
            }
            if (io.MouseWheel) t->cam.dist *= io.MouseWheel > 0 ? 0.9f : 1.1f;
        }
    }
    ImGui::EndGroup();
}

void Build(Tool* t, f32 w, f32 h) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("modtool", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    const char* src = PortDiscIsOpen() ? "disc image" : gPortOptions.dataDir ? gPortOptions.dataDir : "no game data";
    ImGui::Text("Monster Lab mod tool");
    ImGui::SameLine();
    ImGui::TextDisabled("  source: %s%s   (read only: changes are saved as a mod)", src, gPortOptions.noMods ? ", mods off" : "");
    if (t->doc) {
        ImGui::SameLine();
        ImGui::TextDisabled("   %s  from %s, %u bytes", t->doc->path.c_str(), t->doc->from.c_str(), (u32)t->doc->raw.size());
    }
    if (!t->status.empty()) ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.4f, 1.0f), "%s", t->status.c_str());
    ImGui::Separator();
    f32 avail = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("files", ImVec2(250, avail), ImGuiChildFlags_Borders);
    static char filt[64];
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filter", "filter files", filt, sizeof(filt));
    for (const std::string& f : t->files) {
        if (filt[0] && !strstr(Lower(f).c_str(), Lower(filt).c_str())) continue;
        const char* name = strrchr(f.c_str(), '/');
        if (ImGui::Selectable(name ? name + 1 : f.c_str(), t->doc && t->doc->path == f)) OpenDoc(t, f);
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("tree", ImVec2(w * 0.22f, avail), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
    if (t->doc) DrawTree(t, 0);
    else ImGui::TextDisabled("Pick a file on the left.");
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("details", ImVec2(0, avail));
    if (t->doc) {
        const Node& n = t->doc->nodes[t->sel];
        std::vector<Field> fields;
        NodeFields(t, &fields);
        BOOL list = n.kind == K_TABLE || n.kind == K_STRINGS, tex = n.kind == K_TEX || n.kind == K_STEX;
        if (n.kind == K_SECTION && t->doc->zones && t->doc->secs[n.a].type >= 0) {
            ImGui::BeginChild("model", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.62f), ImGuiChildFlags_Borders);
            DrawModel(t);
            ImGui::EndChild();
        }
        BOOL gallery = FALSE;
        for (int k : n.kids) gallery |= t->doc->nodes[k].kind == K_TEX || t->doc->nodes[k].kind == K_STEX;
        if (gallery) {
            ImGui::BeginChild("gallery", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.7f), ImGuiChildFlags_Borders);
            DrawGallery(t);
            ImGui::EndChild();
        }
        f32 top = ImGui::GetContentRegionAvail().y;
        if (list || tex) {
            ImGui::BeginChild("list", ImVec2(0, top * (tex ? 0.55f : 0.38f)), ImGuiChildFlags_Borders);
            if (tex) DrawTexture(t);
            else DrawList(t);
            ImGui::EndChild();
        }
        ImGui::BeginChild("fields", ImVec2(ImGui::GetContentRegionAvail().x - 560, 0), ImGuiChildFlags_Borders);
        ImGui::TextDisabled("%s", n.label.c_str());
        t->hover = -1;
        if (!fields.empty() && ImGui::BeginTable("fields", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("field");
            ImGui::TableSetupColumn("type");
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("offset");
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < fields.size(); i++) {
                const Field& fd = fields[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (fd.size) {
                    ImGui::Selectable(fd.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                    if (ImGui::IsItemHovered()) t->hover = (int)i;
                } else ImGui::TextDisabled("%s", fd.name.c_str());
                ImGui::TableNextColumn(); ImGui::TextDisabled("%s", fd.type.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(fd.value.c_str());
                ImGui::TableNextColumn(); if (fd.size) ImGui::TextDisabled("%08X", fd.off);
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("hex", ImVec2(0, 0), ImGuiChildFlags_Borders);
        u32 off = n.off, size = n.size;
        if (n.kind == K_TABLE && t->row >= 0) {          // the picked row's bytes
            const GameDbTable* tb = t->doc->Table(n.a);
            off = t->doc->Off(tb->rows + (u32)t->row * tb->rowSize);
            size = tb->rowSize;
        } else if (n.kind == K_STRINGS && t->row >= 0 && fields.size() > 1) { off = fields[1].off; size = fields[1].size; }
        const Field* hi = t->hover >= 0 ? &fields[t->hover] : fields.size() == 1 || (n.kind == K_STRINGS && fields.size() > 1) ? &fields.back() : NULL;
        if (n.kind == K_SLOT && fields.size() > 1) { off = fields[1].off; size = 0x40; }
        ImGui::TextDisabled("disc bytes %08X-%08X", off, off + size);
        DrawHex(t->doc, off, size, hi);
        ImGui::EndChild();
        if (!t->doc->error.empty() && t->status.empty()) t->status = t->doc->error;
    }
    ImGui::EndChild();
    ImGui::End();
    if (t->openFrames) t->openFrames--;
}

LRESULT CALLBACK ToolProc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(w, m, wp, lp)) return 1;
    if (m == WM_CLOSE && sTool) { sTool->quit = TRUE; return 0; }
    if (m == WM_SIZE && sTool && wp != SIZE_MINIMIZED) { sTool->resizeW = LOWORD(lp); sTool->resizeH = HIWORD(lp); return 0; }
    return DefWindowProcA(w, m, wp, lp);
}

}  // namespace

// Exports go next to the exe (export	extures), never into the game data.
static void DefaultExportDir(Tool* t) {
    char exe[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
    char* slash = n && n < MAX_PATH ? strrchr(exe, '\\') : NULL;
    if (slash) *slash = 0;
    snprintf(t->exportDir, sizeof(t->exportDir), "%s\\export", slash ? exe : ".");
}

// The tool: a window until it is closed, or with `shot` one picture into that PNG. `open` is a disc path
// ("streams/global_binary.sir") or a file, `select` the start of a tree label, `row` a row or string to pick.
int PortModToolRun(const char* open, const char* select, int row, const char* shot) {
    u32 W = 1600, H = 900;
    Tool t;
    sTool = &t;
    DefaultExportDir(&t);
    DictScanDol();
    ListFiles(&t);
    if (open) OpenDoc(&t, open);
    Select(&t, select);
    t.row = row;
    if (!shot) {
        WNDCLASSA wc = {};
        wc.lpfnWndProc = ToolProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = "MonsterLabModTool";
        RegisterClassA(&wc);
        RECT r = {0, 0, (LONG)W, (LONG)H};
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
        t.wnd = CreateWindowA(wc.lpszClassName, "Monster Lab mod tool", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                              r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
        if (!t.wnd) { sTool = NULL; return 1; }
    }
    ID3D11Device* dev = NULL;
    ID3D11DeviceContext* ctx = NULL;
    IDXGISwapChain* swap = NULL;
    ID3D11Texture2D* target = NULL;
    ID3D11RenderTargetView* rtv = NULL;
    static const D3D_DRIVER_TYPE kDrivers[2] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
    HRESULT hr = E_FAIL;
    for (int i = shot ? 1 : 0; i < 2 && FAILED(hr); i++) {
        if (t.wnd) {
            DXGI_SWAP_CHAIN_DESC sd = {};
            sd.BufferCount = 2; sd.BufferDesc.Width = W; sd.BufferDesc.Height = H; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = t.wnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
            hr = D3D11CreateDeviceAndSwapChain(NULL, kDrivers[i], NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &swap, &dev, NULL, &ctx);
        } else {
            hr = D3D11CreateDevice(NULL, kDrivers[i], NULL, 0, NULL, 0, D3D11_SDK_VERSION, &dev, NULL, &ctx);
        }
    }
    auto makeTarget = [&]() {
        if (swap) swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&target);
        else {
            D3D11_TEXTURE2D_DESC d = {};
            d.Width = W; d.Height = H; d.MipLevels = 1; d.ArraySize = 1; d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            d.SampleDesc.Count = 1; d.BindFlags = D3D11_BIND_RENDER_TARGET;
            dev->CreateTexture2D(&d, NULL, &target);
        }
        if (target) dev->CreateRenderTargetView(target, NULL, &rtv);
    };
    if (SUCCEEDED(hr)) makeTarget();
    int result = 1;
    if (rtv) {
        ImGuiContext* gui = ImGui::CreateContext();
        ImGui::SetCurrentContext(gui);
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
        char font[MAX_PATH + 32];
        UINT len = GetWindowsDirectoryA(font, MAX_PATH);
        if (len && len < MAX_PATH) {
            strcat(font, "\\Fonts\\segoeui.ttf");
            if (GetFileAttributesA(font) != INVALID_FILE_ATTRIBUTES) io.Fonts->AddFontFromFileTTF(font, 17.0f);
        }
        ImGui::StyleColorsDark();
        if (t.wnd) ImGui_ImplWin32_Init(t.wnd);
        ImGui_ImplDX11_Init(dev, ctx);
        t.dev = dev;
        t.ctx = ctx;
        f32 clear[4] = {0.06f, 0.06f, 0.07f, 1.0f};
        for (int frame = 0; !t.quit; frame++) {
            if (t.wnd) {
                MSG msg;
                while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
                if (t.resizeW && t.resizeH) {
                    rtv->Release(); target->Release(); rtv = NULL; target = NULL;
                    swap->ResizeBuffers(0, t.resizeW, t.resizeH, DXGI_FORMAT_UNKNOWN, 0);
                    W = t.resizeW; H = t.resizeH; t.resizeW = t.resizeH = 0;
                    makeTarget();
                    if (!rtv) break;
                }
                ImGui_ImplWin32_NewFrame();
            } else {
                io.DisplaySize = ImVec2((f32)W, (f32)H);
                io.DeltaTime = 1.0f / 60.0f;
            }
            ImGui_ImplDX11_NewFrame();
            ImGui::NewFrame();
            Build(&t, io.DisplaySize.x, io.DisplaySize.y);
            ImGui::Render();
            ctx->OMSetRenderTargets(1, &rtv, NULL);
            ctx->ClearRenderTargetView(rtv, clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            if (swap) swap->Present(1, 0);
            else if (frame == 3) {                               // the layout has settled
                ctx->OMSetRenderTargets(0, NULL, NULL);
                result = PortGfxSaveTexture(dev, ctx, target, W, H, shot) ? 0 : 1;
                if (result) fprintf(stderr, "[modtool] cannot write %s\n", shot);
                break;
            }
        }
        if (t.wnd) result = 0;
        ImGui_ImplDX11_Shutdown();
        if (t.wnd) ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(gui);
    }
    if (rtv) rtv->Release();
    if (target) target->Release();
    if (swap) swap->Release();
    if (ctx) ctx->Release();
    if (dev) dev->Release();
    if (t.wnd) { DestroyWindow(t.wnd); UnregisterClassA("MonsterLabModTool", GetModuleHandleA(NULL)); }
    if (t.srv) t.srv->Release();
    MtViewDestroy(t.view);
    DropThumbs(&t);
    delete t.doc;
    sTool = NULL;
    return result;
}

// With the player's disc (tools/modtool_test.py): opens a few files and checks what the tree holds.
int PortModToolTest(void) {
    int fails = 0;
    DictScanDol();
    Tool t;
    ListFiles(&t);
    if (t.files.size() < 40) { fprintf(stderr, "[modtool] test: only %u files listed\n", (u32)t.files.size()); fails++; }
    struct Want { const char* path; int minSections; } wants[] = {
        {"catalog.sir", 0}, {"streams/global_binary.sir", 2}, {"streams/village_level.sir", 3}, {"streams/shared_texture.sir", 1}};
    for (const Want& w : wants) {
        OpenDoc(&t, w.path);
        if (!t.doc || (s32)t.doc->secs.size() < w.minSections || (w.minSections && !t.doc->error.empty())) {
            fprintf(stderr, "[modtool] test: %s: %s\n", w.path, t.doc ? t.doc->error.c_str() : t.status.c_str());
            fails++;
            continue;
        }
        fprintf(stderr, "[modtool] test: %s: %u bytes, %u sections, %u nodes\n", w.path, (u32)t.doc->raw.size(), (u32)t.doc->secs.size(),
                (u32)t.doc->nodes.size());
        if (!strcmp(w.path, "streams/global_binary.sir")) {
            int tables = 0, textures = 0, slots = 0;
            for (const Node& n : t.doc->nodes) tables += n.kind == K_TABLE, textures += n.kind == K_TEX, slots += n.kind == K_SLOT;
            const GameDbTable* parts = t.doc->Table(0);
            std::vector<Field> f;
            for (int k = 0; k < GAMEDB_TABLE_COUNT && f.empty(); k++)
                if (t.doc->Table(k)->count) RowFields(t.doc, k, 0, &f);
            fprintf(stderr, "[modtool] test: global_binary.sir: %d tables, %d textures, %d header slots, %u strings, first row %u fields\n",
                    tables, textures, slots, t.doc->strings ? t.doc->strings->count : 0, (u32)f.size());
            int decoded = 0;
            for (const Node& n : t.doc->nodes) {
                TexRef r;
                std::vector<u32> rgba;
                decoded += n.kind == K_TEX && TexOf(t.doc, n, &r) && Decode(r, &rgba);
            }
            fprintf(stderr, "[modtool] test: global_binary.sir: %d of %d textures decoded\n", decoded, textures);
            if (decoded != textures) fails++;
            if (!t.doc->converted || tables != GAMEDB_TABLE_COUNT || textures < 10 || slots < 10 || !t.doc->strings || f.empty() || !parts) fails++;
            // The typed bytes of a field are the disc bytes the loader converted: a 4-byte int column read
            // big-endian from the raw copy must give the value of the converted row.
            for (const Field& x : f)
                if (x.size == 4 && x.type == "int" && x.value.find("->") == std::string::npos &&
                    (s32)Be32(&t.doc->raw[x.off]) != atoi(x.value.c_str())) { fprintf(stderr, "[modtool] test: %s at %08X differs\n", x.name.c_str(), x.off); fails++; }
        }
    }
    // Models: the black hole's template head (shared_parts.sir zone 23) and the village's own zone.
    struct WantModel { const char* path; s32 zone; u32 minTris; } models[] = {{"streams/shared_parts.sir", 23, 300}, {"streams/village_level.sir", 1, 20000}};
    for (const WantModel& m : models) {
        OpenDoc(&t, m.path);
        BOOL found = FALSE;
        for (const Node& n : t.doc->nodes) {
            if (n.kind != K_SECTION || t.doc->secs[n.a].type != m.zone) continue;
            found = TRUE;
            MtZone z;
            bool ok = MtZoneScan(t.doc->raw.data() + n.off, n.size, &z);
            u32 textured = 0;
            for (const MtBatch& b : z.batches) {
                std::vector<u32> rgba;
                u32 w, h;
                textured += MtBatchTexture(t.doc->raw.data() + n.off, n.size, z, b, &rgba, &w, &h) ? 1 : 0;
            }
            fprintf(stderr, "[modtool] test: %s zone %d: base %08X, %u display lists (%u with a texture), %u triangles%s%s\n", m.path, m.zone, z.base,
                    (u32)z.batches.size(), textured, (u32)z.verts.size() / 3, ok ? "" : ": ", z.error.c_str());
            if (!ok || z.verts.size() / 3 < m.minTris || !textured) fails++;
        }
        if (!found) { fprintf(stderr, "[modtool] test: %s has no zone %d\n", m.path, m.zone); fails++; }
    }
    // Every texture of shared_texture.sir decodes; one goes out as PNG and DDS and comes back the same.
    OpenDoc(&t, "streams/shared_texture.sir");
    int texTotal = 0, texDecoded = 0;
    const Node* first = NULL;
    for (const Node& n : t.doc->nodes) {
        if (n.kind != K_STEX) continue;
        texTotal++;
        TexRef r;
        std::vector<u32> rgba;
        if (TexOf(t.doc, n, &r) && Decode(r, &rgba)) { texDecoded++; if (!first && !(r.w & 3) && !(r.h & 3)) first = &n; }
    }
    fprintf(stderr, "[modtool] test: shared_texture.sir: %d of %d textures decoded\n", texDecoded, texTotal);
    if (texTotal < 500 || texDecoded != texTotal || !first) fails++;
    else {
        char dir[MAX_PATH + 64];
        DWORD n = GetTempPathA(MAX_PATH, dir);
        snprintf(dir + n, sizeof(dir) - n, "modtool_test_%lu", GetCurrentProcessId());
        TexRef r;
        std::vector<u32> rgba;
        TexOf(t.doc, *first, &r);
        Decode(r, &rgba);
        std::string msg = ExportTexture(r, dir, TRUE);
        std::string base = std::string(dir) + "\\" + PackName(r);
        u32 w = 0, h = 0;
        const char* why = NULL;
        u32* back = PortPngRead((base + ".png").c_str(), &w, &h, &why);
        PortModImage dds = {};
        BOOL ddsOk = PortDdsRead((base + ".dds").c_str(), &dds, &why);
        BOOL same = back && w == r.w && h == r.h && !memcmp(back, rgba.data(), rgba.size() * 4);
        fprintf(stderr, "[modtool] test: export %s: %s; PNG %s, DDS %s\n", r.name.c_str(), msg.c_str(), same ? "same pixels" : "DIFFERS",
                ddsOk && dds.w == r.w && dds.h == r.h ? "read back" : "NOT READ");
        if (!same || !ddsOk || dds.w != r.w) fails++;
        free(back);
        free(dds.data);
        DeleteFileA((base + ".png").c_str());
        DeleteFileA((base + ".dds").c_str());
        RemoveDirectoryA(dir);
    }
    delete t.doc;
    fprintf(stderr, fails ? "MODTOOL FAIL (%d)\n" : "MODTOOL OK\n", fails);
    return fails ? 1 : 0;
}

// No game data (--selftest): a small container made here, through the same document code.
int PortSelfTestModTool(void) {
    int fails = 0;
    std::vector<u8> f(0x1000, 0);
    auto put = [&](u32 o, u32 v) { f[o] = (u8)(v >> 24); f[o + 1] = (u8)(v >> 16); f[o + 2] = (u8)(v >> 8); f[o + 3] = (u8)v; };
    put(0, SIR_MAGIC); put(4, 3); put(8, 2);
    put(0x40, (u32)-1); put(0x44, 0x800); put(0x48, 0x100); put(0x54, NameHash("SELFTEST"));
    put(0x58, 7); put(0x5C, 0x900); put(0x60, 0x200);
    memcpy(&f[0x900], "SELFTEST", 9);
    Doc* d = BuildDoc("streams/selftest_level.sir", "test", std::move(f));
    int sections = 0;
    for (const Node& n : d->nodes) sections += n.kind == K_SECTION;
    if (sections != 2 || !d->error.empty()) { PortLog("selftest modtool: %d sections, error '%s'", sections, d->error.c_str()); fails++; }
    else if (d->nodes.back().label != "section 1: zone 7, 0x200 bytes") { PortLog("selftest modtool: label '%s'", d->nodes.back().label.c_str()); fails++; }
    if (HashText(NameHash("selftest")) != "SELFTEST") { PortLog("selftest modtool: the name of a hash is not found"); fails++; }
    delete d;
    std::vector<u8> junk(16, 0x41);
    d = BuildDoc("x.sir", "test", std::move(junk));
    if (d->error.empty()) { PortLog("selftest modtool: a file without PNP0 was taken for a container"); fails++; }
    delete d;
    if (!fails) PortLog("selftest modtool: ok (container tree, names of hashes)");
    return fails;
}
#endif
