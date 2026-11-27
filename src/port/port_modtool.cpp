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
enum Kind { K_FILE, K_CONTAINER, K_SECTIONS, K_SECTION, K_HDR, K_SLOT, K_DB, K_TABLE, K_TEXLIST, K_TEX, K_STRINGS };

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
    BOOL level = strstr(base, "_level") || strstr(base, "_arena") || strstr(base, "castle_") || strstr(base, "trouble_scope");
    char b[64];
    snprintf(b, sizeof(b), "sections (%d)", count);
    int sn = Add(d, root, K_SECTIONS, b, SIR_SECTION_TABLE, (u32)count * SIR_SECTION_ENTRY);
    for (s32 i = 0; i < count; i++) {
        SirSection s = {};
        if (!SirContainerSection(d->raw.data(), n, i, &s)) { d->error = "a section lies outside the file"; s.offset = s.size = 0; }
        d->secs.push_back(s);
        Add(d, sn, K_SECTION, SectionLabel(i, s, level), s.offset, s.size, i);
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
};
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
            break;
        }
        case K_STRINGS:
            Push(f, "count", "u32", Int(d->strings->count), n.off + 0x0C, 4);
            if (t->row >= 0 && (u32)t->row < d->strings->count) {
                const char* s = d->strings->strings[t->row];
                Push(f, ("string " + Int(t->row)).c_str(), "char[]", s, d->Off(s), (u32)strlen(s) + 1);
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
        BOOL list = n.kind == K_TABLE || n.kind == K_STRINGS;
        f32 top = ImGui::GetContentRegionAvail().y;
        if (list) {
            ImGui::BeginChild("list", ImVec2(0, top * 0.38f), ImGuiChildFlags_Borders);
            DrawList(t);
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
                }
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

// The tool: a window until it is closed, or with `shot` one picture into that PNG. `open` is a disc path
// ("streams/global_binary.sir") or a file, `select` the start of a tree label, `row` a row or string to pick.
int PortModToolRun(const char* open, const char* select, int row, const char* shot) {
    u32 W = 1600, H = 900;
    Tool t;
    sTool = &t;
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
            if (!t.doc->converted || tables != GAMEDB_TABLE_COUNT || textures < 10 || slots < 10 || !t.doc->strings || f.empty() || !parts) fails++;
            // The typed bytes of a field are the disc bytes the loader converted: a 4-byte int column read
            // big-endian from the raw copy must give the value of the converted row.
            for (const Field& x : f)
                if (x.size == 4 && x.type == "int" && x.value.find("->") == std::string::npos &&
                    (s32)Be32(&t.doc->raw[x.off]) != atoi(x.value.c_str())) { fprintf(stderr, "[modtool] test: %s at %08X differs\n", x.name.c_str(), x.off); fails++; }
        }
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
