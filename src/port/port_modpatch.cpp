// PC port: table patches and added parts (include/port/port_modpatch.h, docs/modding.md).
// mods/<name>/patches/*.json:
//   {"tables": [{"table": 5, "row": 3, "set": {"#2": 10}},                change a row
//               {"table": 5, "append": true, "copy": 3, "set": {...}}],   add a row (a copy of row "copy")
//    "parts":  [{"id": "head", "copy": 73, "set": {...}, "names": ["A", ...], "data": {"k": "v"}}]}
// A column is "#<number>", "@<byte offset in the row>" or the column's name (the table stores name hashes).
// Mods apply in load order, "tables" before "parts" so a part copies the patched row. What the tables then
// point at lies in PortHostAlloc memory, which savestates hold.
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#include "game/part.h"
#include "port/port.h"
#include "port/port_json.h"
#include "port/port_modpatch.h"
#include "port/port_mods.h"

#define PART_TABLE 22
#define TEXT_KEY   0x4D4F4400u      // name hashes of mod parts: TEXT_KEY | (row - 186) << 2 | tier
#define NAME_SLOTS ((PORT_MAX_PART_ROWS - PORT_VANILLA_PARTS) * 4)

// What a savestate must hold: the game's tables point at memory these describe.
PORT_SAVED_BEGIN
static BOOL sApplied;                    // the patches are in the tables
static s32  sTextId[NAME_SLOTS];         // text id of a mod part's tier name, 0 = none
PORT_SAVED_END

namespace {

typedef std::vector<std::pair<std::string, std::string>> Fields;   // column or data name -> value as written

struct TablePatch { int table, row, copy; BOOL append; Fields set; };
struct PartPatch {
    std::string key, modDir;
    int copy;
    Fields set, data;
    std::vector<std::string> names;
};
struct ModPatches { std::string mod; std::vector<TablePatch> tables; std::vector<PartPatch> parts; };

struct Reg { s32 row, copy; std::string key; };

struct State {
    std::vector<PartPatch> parts;        // the enabled mods' parts that are in the table
    std::vector<PortModPart> list;       // the same, as the API gives them out
};
// Made at first use: the mods are looked at before this file's static objects are built (port_runtime.cpp).
State& S(void) { static State* s = new State; return *s; }

// ---------------------------------------------------------------- reading the files

BOOL Number(Reader& r, long long* out) {
    r.Space();
    char* end = NULL;
    *out = strtoll(r.p, &end, 10);
    if (end == r.p || *end == '.' || *end == 'e') { r.Fail("a whole number expected"); return FALSE; }
    r.p = end;
    return TRUE;
}

BOOL ReadInt(Reader& r, int* out) {
    long long v;
    if (!Number(r, &v)) return FALSE;
    if (v < 0 || v > 100000) { r.Fail("the number is out of range"); return FALSE; }
    *out = (int)v;
    return TRUE;
}

// A value kept as text: a string, a number (any form strtod reads) or true / false.
BOOL Scalar(Reader& r, std::string* out) {
    r.Space();
    if (*r.p == '"') return r.Str(out);
    if (!strncmp(r.p, "true", 4)) { r.p += 4; *out = "1"; return TRUE; }
    if (!strncmp(r.p, "false", 5)) { r.p += 5; *out = "0"; return TRUE; }
    const char* a = r.p;
    while (*r.p && strchr("+-.0123456789eExXabcdfABCDF", *r.p)) r.p++;
    if (r.p == a) { r.Fail("a number or a string expected"); return FALSE; }
    out->assign(a, r.p);
    return TRUE;
}

// {"key": ...}: calls `member` for each key.
template <class F> BOOL Object(Reader& r, F member) {
    if (!r.Expect('{')) return FALSE;
    if (r.Eat('}')) return TRUE;
    do {
        std::string key;
        if (!r.Str(&key) || !r.Expect(':') || !member(key) || !r.err.empty()) return FALSE;
    } while (r.Eat(','));
    return r.Expect('}');
}

template <class F> BOOL Array(Reader& r, F element) {
    if (!r.Expect('[')) return FALSE;
    if (r.Eat(']')) return TRUE;
    do {
        if (!element() || !r.err.empty()) return FALSE;
    } while (r.Eat(','));
    return r.Expect(']');
}

BOOL Pairs(Reader& r, Fields* out) {
    return Object(r, [&](const std::string& k) {
        std::string v;
        if (!Scalar(r, &v)) return FALSE;
        out->push_back(std::make_pair(k, v));
        return TRUE;
    });
}

BOOL Unknown(Reader& r) { r.Fail("unknown key"); return FALSE; }

BOOL ParseFile(const std::string& text, const char* mod, const std::string& modDir, ModPatches* out, std::string* err) {
    const char* p = text.c_str();
    if (!strncmp(p, "\xEF\xBB\xBF", 3)) p += 3;
    Reader r;
    r.p = r.start = p;
    ModPatches found;
    Object(r, [&](const std::string& top) {
        if (top == "tables") return Array(r, [&]() {
            TablePatch t = {-1, -1, -1, FALSE, Fields()};
            if (!Object(r, [&](const std::string& k) {
                    if (k == "table") return ReadInt(r, &t.table);
                    if (k == "row") return ReadInt(r, &t.row);
                    if (k == "copy") return ReadInt(r, &t.copy);
                    if (k == "append") { std::string v; if (!Scalar(r, &v)) return FALSE; t.append = v == "1"; return TRUE; }
                    if (k == "set") return Pairs(r, &t.set);
                    return Unknown(r);
                })) return FALSE;
            if (t.table < 0 || (t.append ? t.copy < 0 : t.row < 0)) { r.Fail("a table entry needs \"table\" and \"row\", or \"append\" and \"copy\""); return FALSE; }
            found.tables.push_back(t);
            return TRUE;
        });
        if (top == "parts") return Array(r, [&]() {
            PartPatch part;
            part.copy = -1;
            part.modDir = modDir;
            std::string id;
            if (!Object(r, [&](const std::string& k) {
                    if (k == "id") return (BOOL)r.Str(&id);
                    if (k == "copy") return ReadInt(r, &part.copy);
                    if (k == "set") return Pairs(r, &part.set);
                    if (k == "data") return Pairs(r, &part.data);
                    if (k == "names") return Array(r, [&]() { std::string n; if (!r.Str(&n)) return FALSE; part.names.push_back(n); return TRUE; });
                    return Unknown(r);
                })) return FALSE;
            BOOL good = !id.empty() && id.size() <= 40;
            for (char c : id) good = good && (isalnum((u8)c) || c == '_' || c == '-');
            if (!good) { r.Fail("a part needs an \"id\" of letters, digits, _ or -"); return FALSE; }
            if (part.copy < 0 || part.copy >= PORT_VANILLA_PARTS) { r.Fail("a part needs \"copy\": a vanilla part's row, 0-185"); return FALSE; }
            if (part.names.size() > 4) { r.Fail("a part has at most four names (one per tier)"); return FALSE; }
            part.key = std::string(mod) + ":" + id;
            found.parts.push_back(part);
            return TRUE;
        });
        return Unknown(r);
    });
    r.Space();
    if (r.err.empty() && *r.p) r.Fail("text after the end");
    if (!r.err.empty()) { *err = r.err; return FALSE; }
    out->tables.insert(out->tables.end(), found.tables.begin(), found.tables.end());
    out->parts.insert(out->parts.end(), found.parts.begin(), found.parts.end());
    return TRUE;
}

BOOL ReadWhole(const char* path, std::string* out) {
    FILE* f = fopen(path, "rb");
    if (!f) return FALSE;
    char buf[4096];
    size_t n;
    out->clear();
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0 && out->size() < (1u << 22)) out->append(buf, n);
    fclose(f);
    return TRUE;
}

std::vector<ModPatches> ReadMods(void) {
    std::vector<ModPatches> all;
    if (gPortOptions.noMods) return all;
    for (int i = 0; i < gPortModCount; i++) {
        const PortMod* m = &gPortMods[i];
        if (!m->enabled || !m->present) continue;
        std::string dir = std::string(PortModsDir()) + "\\" + m->name;
        ModPatches mod;
        mod.mod = m->name;
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((dir + "\\patches\\*.json").c_str(), &fd);   // NTFS gives the names in order
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            std::string path = dir + "\\patches\\" + fd.cFileName, text, err;
            if (!ReadWhole(path.c_str(), &text)) continue;
            if (!ParseFile(text, m->name, dir, &mod, &err))
                PortLog("mods: %s: %s: %s; the file is ignored", m->name, path.c_str(), err.c_str());
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        all.push_back(mod);
    }
    return all;
}

// ---------------------------------------------------------------- the tables

GameDbTable* Table(u8* db, int t) { return t >= 0 && t < GAMEDB_TABLE_COUNT ? (GameDbTable*)(db + t * GAMEDB_DESC_SIZE) : NULL; }

u32 NameHash(const char* s) {                     // the game's StringHash: CRC-32 of the upper-cased string
    u32 c = 0xFFFFFFFFu;
    for (; *s; s++) {
        c ^= (u8)toupper((u8)*s);
        for (int k = 0; k < 8; k++) c = c >> 1 ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

// The offset and type of the column `name` in a row of that table.
BOOL Column(const GameDbTable* t, const std::string& name, u32* offset, u32* type) {
    long index = -1, at = -1;
    if (name[0] == '#') index = strtol(name.c_str() + 1, NULL, 0);
    else if (name[0] == '@') at = strtol(name.c_str() + 1, NULL, 0);
    u32 off = 0, hash = NameHash(name.c_str());
    for (u32 i = 0; i < t->fieldCount && i < GAMEDB_MAX_FIELDS; i++) {
        u32 size = t->fieldSize[i];
        off = (off + size - 1) & ~(size - 1);    // packed in order, each aligned to its size (sir_loader.cpp)
        if (name[0] == '#' ? (long)i == index : name[0] == '@' ? (long)off == at : t->fieldName[i] == hash) {
            *offset = off;
            *type = t->fieldType[i];
            return off + size <= t->rowSize;
        }
        off += size;
    }
    return FALSE;
}

// Writes the fields into a row; all or nothing. On a mistake `why` says which.
BOOL SetFields(const GameDbTable* t, u8* row, const Fields& set, std::string* why) {
    std::vector<u8> copy(row, row + t->rowSize);
    for (const auto& f : set) {
        u32 off, type;
        if (f.first.empty() || !Column(t, f.first, &off, &type)) { *why = "there is no column " + f.first; return FALSE; }
        const char* v = f.second.c_str();
        char* end = NULL;
        if (type == GAMEDB_T_FLOAT) {
            f32 x = (f32)strtod(v, &end);
            memcpy(&copy[off], &x, 4);
        } else {
            long long x = strtoll(v, &end, 0);
            if (type == GAMEDB_T_HASH && (end == v || *end)) { x = NameHash(v); end = (char*)v + strlen(v); }
            u32 size = type == GAMEDB_T_SHORT ? 2 : type == GAMEDB_T_FLAG || type == GAMEDB_T_BYTE ? 1 : 4;
            long long low = size == 4 ? -2147483648LL : size == 2 ? -32768 : -128;
            long long high = size == 4 ? 4294967295LL : size == 2 ? 65535 : 255;
            if (x < low || x > high) { *why = "the value of " + f.first + " does not fit the column"; return FALSE; }
            memcpy(&copy[off], &x, size);        // little-endian: the low bytes are the value
        }
        if (end == v || *end) { *why = "the value of " + f.first + " is not a number"; return FALSE; }
    }
    memcpy(row, copy.data(), t->rowSize);
    return TRUE;
}

// The table with `count` rows: the old ones, then zeroed ones. FALSE when there is no memory.
BOOL Grow(GameDbTable* t, u32 count) {
    if (count <= t->count) return TRUE;
    u8* rows = (u8*)PortHostAlloc(count * t->rowSize);
    if (!rows) return FALSE;
    memcpy(rows, t->rows, t->count * t->rowSize);
    t->rows = rows;
    t->count = t->count2 = count;
    return TRUE;
}

void ApplyTable(u8* db, const char* mod, const TablePatch& p) {
    GameDbTable* t = Table(db, p.table);
    std::string why;
    if (!t || !t->count || !t->rows) why = "there is no such table";
    else if (p.append && p.table == PART_TABLE) why = "parts are added with \"parts\", which keeps saves working";
    else if (p.append) {
        std::vector<u8> row;
        if ((u32)p.copy >= t->count) why = "there is no row to copy";
        else {
            row.assign(t->rows + p.copy * t->rowSize, t->rows + (p.copy + 1) * t->rowSize);
            if (SetFields(t, row.data(), p.set, &why)) {
                u32 at = t->count;
                if (!Grow(t, at + 1)) why = "out of memory";
                else { memcpy(t->rows + at * t->rowSize, row.data(), t->rowSize); PortLog("mods: %s adds row %u to table %d", mod, at, p.table); }
            }
        }
    } else if ((u32)p.row >= t->count) why = "there is no such row";
    else if (SetFields(t, t->rows + p.row * t->rowSize, p.set, &why)) PortLog("mods: %s changes row %d of table %d", mod, p.row, p.table);
    if (!why.empty()) PortLog("mods: %s: table %d: %s; the entry is ignored", mod, p.table, why.c_str());
}

// ---------------------------------------------------------------- save.modparts and the parts

std::vector<Reg> ReadSidecar(const char* path) {
    std::vector<Reg> out;
    std::string text;
    if (!ReadWhole(path, &text)) return out;
    for (size_t a = 0; a < text.size();) {
        size_t b = text.find('\n', a);
        if (b == std::string::npos) b = text.size();
        std::string line = text.substr(a, b - a);
        a = b + 1;
        while (!line.empty() && (u8)line.back() <= ' ') line.pop_back();
        int row = 0, copy = 0, n = 0;
        if (sscanf(line.c_str(), "%d|%d|%n", &row, &copy, &n) < 2 || !n) continue;   // a comment, or not ours: left as it is
        if (row < PORT_VANILLA_PARTS || row >= PORT_MAX_PART_ROWS || copy < 0 || copy >= PORT_VANILLA_PARTS) continue;
        BOOL taken = FALSE;
        for (const Reg& g : out) taken = taken || g.row == row || g.key == line.substr(n);
        if (!taken) out.push_back(Reg{row, copy, line.substr(n)});
    }
    return out;
}

// Only ever appends: an entry that is there is never rewritten or removed.
BOOL AppendSidecar(const char* path, const Reg& g) {
    std::string old;
    ReadWhole(path, &old);
    FILE* f = fopen(path, "ab");
    if (!f) return FALSE;
    if (old.empty()) fputs("; Monster Lab PC port: which modded part each part number of the saves here is (row|vanilla row|mod:id).\n"
                           "; Keep this file with the saves.\n", f);
    else if (old.back() != '\n') fputc('\n', f);
    fprintf(f, "%d|%d|%s\n", g.row, g.copy, g.key.c_str());
    return fclose(f) == 0;
}

// Row `row` of the part table becomes a copy of row `from`, with its own number.
void CopyPart(GameDbTable* t, u32 row, u32 from) {
    u8* d = t->rows + row * t->rowSize;
    memcpy(d, t->rows + from * t->rowSize, t->rowSize);
    s32 index = (s32)row;
    memcpy(d, &index, 4);                        // column 0: PartDef::index, which becomes the part's defIndex
}

void ApplyParts(u8* db, SirStringTable* strings, const char* sidecar, const std::vector<ModPatches>& mods) {
    GameDbTable* t = Table(db, PART_TABLE);
    std::vector<Reg> reg = ReadSidecar(sidecar);
    std::vector<const PartPatch*> want;
    for (const ModPatches& m : mods)
        for (const PartPatch& p : m.parts) want.push_back(&p);
    if (reg.empty() && want.empty()) return;
    if (!t || t->count != PORT_VANILLA_PARTS || t->rowSize < 0x50) {
        PortLog("mods: the part table is not as expected (%u rows); no part is added", t ? t->count : 0);
        return;
    }
    // New parts get the next free row, in load order, and keep it.
    s32 next = PORT_VANILLA_PARTS;
    for (const Reg& g : reg) if (g.row >= next) next = g.row + 1;
    for (const PartPatch* p : want) {
        BOOL known = FALSE;
        for (const Reg& g : reg) known = known || g.key == p->key;
        if (known) continue;
        if (next >= PORT_MAX_PART_ROWS) { PortLog("mods: %s: no part number is left; the part is not added", p->key.c_str()); continue; }
        Reg g = {next, p->copy, p->key};
        // Not in the file means a save made now could not be read back as this part, so then it is not added.
        if (!AppendSidecar(sidecar, g)) { PortLog("mods: cannot write %s; %s is not added", sidecar, p->key.c_str()); continue; }
        next++;
        reg.push_back(g);
    }
    if (!Grow(t, (u32)next)) return;
    // Every row first holds a vanilla part: the one the sidecar names, so a missing mod's part still works.
    for (s32 row = PORT_VANILLA_PARTS; row < next; row++) {
        s32 from = 0;
        for (const Reg& g : reg) if (g.row == row) from = g.copy;
        CopyPart(t, (u32)row, (u32)from);
    }
    u32 nameCount = 0;
    for (const PartPatch* p : want) nameCount += (u32)p->names.size();
    const char** table = NULL;
    u32 id = 0;
    if (nameCount && strings && strings->strings && (table = (const char**)PortHostAlloc((strings->count + nameCount) * sizeof(char*))) != NULL) {
        memcpy(table, strings->strings, strings->count * sizeof(char*));
        id = strings->count;
        strings->strings = table;
        strings->count += nameCount;
    }
    for (const PartPatch* p : want) {
        s32 row = -1;
        for (const Reg& g : reg) if (g.key == p->key) row = g.row;
        BOOL twice = FALSE;
        for (const PartPatch& have : S().parts) twice = twice || have.key == p->key;
        if (row < 0 || twice) continue;
        CopyPart(t, (u32)row, (u32)p->copy);
        std::string why;
        if (!SetFields(t, t->rows + row * t->rowSize, p->set, &why)) PortLog("mods: %s: %s; \"set\" is ignored", p->key.c_str(), why.c_str());
        for (size_t tier = 0; tier < 4; tier++) {
            if (tier >= p->names.size() || !table) continue;
            char* text = (char*)PortHostAlloc((u32)p->names[tier].size() + 1);
            if (!text) continue;
            memcpy(text, p->names[tier].c_str(), p->names[tier].size() + 1);
            table[id] = text;
            u32 slot = (u32)(row - PORT_VANILLA_PARTS) * 4 + (u32)tier;
            sTextId[slot] = (s32)id++;
            u32 hash = TEXT_KEY | slot;
            memcpy(t->rows + row * t->rowSize + 4 + tier * 4, &hash, 4);   // PartDef::tierName
        }
        S().parts.push_back(*p);
        PortLog("mods: part %s is row %d of the part table", p->key.c_str(), row);
    }
    for (const Reg& g : reg) {
        BOOL on = FALSE;
        for (const PartPatch& p : S().parts) on = on || p.key == g.key;
        if (!on) PortLog("mods: part %s (row %d) is not there: saves keep it, and it shows as vanilla part %d", g.key.c_str(), g.row, g.copy);
    }
}

// The parts of the enabled mods with the rows the sidecar gives them, for the API (also after a savestate,
// whose tables are patched already).
void MakeList(const char* sidecar) {
    std::vector<Reg> reg = ReadSidecar(sidecar);
    S().list.clear();
    for (const PartPatch& p : S().parts)
        for (const Reg& g : reg)
            if (g.key == p.key) S().list.push_back(PortModPart{p.key.c_str(), g.row, p.copy, p.modDir.c_str()});
}

const char* SidecarPath(void) {
    static char path[MAX_PATH + 32];
    if (!path[0]) snprintf(path, sizeof(path), "%s\\save.modparts", PortSaveDir());
    return path;
}

// TRUE when anything could need the sidecar or the patches (so a run without mods does not touch the save folder early).
BOOL Wanted(void) {
    if (!gPortOptions.noMods)
        for (int i = 0; i < gPortModCount; i++)
            if (gPortMods[i].enabled && gPortMods[i].present) return TRUE;
    return GetFileAttributesA(SidecarPath()) != INVALID_FILE_ATTRIBUTES;
}

}  // namespace

void PortModPatchRun(u8* db, SirStringTable* strings, const char* sidecar) {
    std::vector<ModPatches> mods = ReadMods();
    S().parts.clear();
    memset(sTextId, 0, sizeof(sTextId));
    for (const ModPatches& m : mods)
        for (const TablePatch& p : m.tables) ApplyTable(db, m.mod.c_str(), p);
    ApplyParts(db, strings, sidecar, mods);
    MakeList(sidecar);
    sApplied = TRUE;
}

void PortModPatchApply(u8* globalHdr) {
    GameDb* gdb = NULL;
    SirStringTable* strings = NULL;
    memcpy(&gdb, globalHdr + GLOBAL_BINARY_HDR_GAMEDB, sizeof(gdb));
    memcpy(&strings, globalHdr + 0xDA8, sizeof(strings));
    sApplied = TRUE;
    if (gdb && gdb->data && Wanted()) PortModPatchRun(gdb->data, strings, SidecarPath());
}

// A savestate made with other mods (or none) skips the conversion: patch what it holds, or find out the
// parts for the API. `gGameDb` and `gStringTable` are the game's own pointers, restored by the state.
struct StringTable;
extern StringTable* gStringTable;

void PortModPatchFrame(void) {
    static BOOL listed;                          // host memory: false again in a process that loads a state
    if (listed) return;
    if (!sApplied) {
        if (!gGameDb || !gGameDb->data || !gStringTable) return;
        sApplied = TRUE;
        if (Wanted()) PortModPatchRun(gGameDb->data, (SirStringTable*)gStringTable, SidecarPath());
    } else if (S().parts.empty() && Wanted()) {  // the tables came with the state
        for (const ModPatches& m : ReadMods())
            for (const PartPatch& p : m.parts) S().parts.push_back(p);
        MakeList(SidecarPath());
    }
    listed = TRUE;
}

int PortModPartCount(void) { return (int)S().list.size(); }
const PortModPart* PortModPartAt(int i) { return i >= 0 && i < (int)S().list.size() ? &S().list[i] : NULL; }

s32 PortModPartIndex(const char* key) {
    for (const PortModPart& p : S().list)
        if (!_stricmp(p.key, key)) return p.defIndex;
    return -1;
}

const char* PortModPartData(int i, const char* name) {
    const PortModPart* at = PortModPartAt(i);
    if (!at) return NULL;
    for (const PartPatch& p : S().parts)
        if (p.key == at->key)
            for (const auto& d : p.data)
                if (d.first == name) return d.second.c_str();
    return NULL;
}

extern "C" s32 PortModTextId(u32 hash) {
    if ((hash & 0xFFFFFC00u) != TEXT_KEY) return -1;
    u32 slot = hash & 0x3FF;
    return slot < NAME_SLOTS && sTextId[slot] ? sTextId[slot] : -1;
}

// A save names a part row that the table does not have (save.modparts was lost): add copies of vanilla part 0
// up to it, so nothing reads past the table and saving again keeps the number.
extern "C" void PortModPartEnsureRow(s32 def) {
    if (def < PORT_VANILLA_PARTS || def >= PORT_MAX_PART_ROWS || !gGameDb || !gGameDb->data) return;
    GameDbTable* t = Table(gGameDb->data, PART_TABLE);
    u32 had = t->count;
    if ((u32)def < had || !had || !Grow(t, (u32)def + 1)) return;
    for (u32 row = had; row < (u32)def + 1; row++) CopyPart(t, row, 0);
    PortLog("mods: a save has part number %d, which no mod and no save.modparts explains; it shows as vanilla part 0", def);
}

// ---------------------------------------------------------------- self-test (no game data)

int PortSelfTestModPatch(void) {
    int fails = 0;
#define PCHECK(cond, ...) do { if (!(cond)) { PortLog("selftest modpatch: " __VA_ARGS__); fails++; } } while (0)
    static PortMod keep[PORT_MAX_MODS];
    memcpy(keep, gPortMods, sizeof(keep));
    int keepCount = gPortModCount;
    BOOL keepOff = gPortOptions.noMods;
    char keepDir[MAX_PATH], tmp[MAX_PATH], base[MAX_PATH + 64];
    snprintf(keepDir, sizeof(keepDir), "%s", PortModsDir());
    GetTempPathA(MAX_PATH, tmp);
    snprintf(base, sizeof(base), "%smonsterlab_selftest_patch_%lu", tmp, GetCurrentProcessId());
    CreateDirectoryA(base, NULL);
    std::string root = base, sidecar = root + "\\save.modparts";
    auto put = [&](const std::string& file, const std::string& text) {
        FILE* f = fopen((root + "\\" + file).c_str(), "wb");
        if (f) { fwrite(text.data(), 1, text.size(), f); fclose(f); } else fails++;
    };
    const char* names[3] = {"A", "B", "C"};
    for (int i = 0; i < 3; i++) {
        CreateDirectoryA((root + "\\" + names[i]).c_str(), NULL);
        CreateDirectoryA((root + "\\" + names[i] + "\\patches").c_str(), NULL);
        std::string json = "{\"parts\": [{\"id\": \"head\", \"copy\": " + std::to_string(10 + i) +
                           ", \"set\": {\"power\": " + std::to_string(100 + i) + "}, \"names\": [\"Head " + names[i] + "\"], \"data\": {\"model\": \"m.bin\"}}]}";
        put(std::string(names[i]) + "\\patches\\parts.json", json);
    }
    put("A\\patches\\tables.json", "{\"tables\": [{\"table\": 5, \"row\": 1, \"set\": {\"power\": -3, \"@0\": 7}},"
                                   "{\"table\": 5, \"append\": true, \"copy\": 1, \"set\": {\"#0\": 9}},"
                                   "{\"table\": 5, \"row\": 0, \"set\": {\"#0\": 1, \"nothing\": 2}}]}");
    put("A\\patches\\broken.json", "{\"tables\": [{\"table\": 5, \"row\": 1, \"sett\": {}}]}");

    // A game database of two tables: parts (22: index, four names, power) and table 5 (a number, power).
    u8* db = (u8*)calloc(GAMEDB_TABLE_COUNT, GAMEDB_DESC_SIZE);
    static const char* parts[PORT_VANILLA_PARTS];
    SirStringTable strings = {};
    auto fresh = [&]() {
        for (int k = 0; k < 2; k++) {
            GameDbTable* t = Table(db, k ? 5 : PART_TABLE);
            memset(t, 0, GAMEDB_DESC_SIZE);
            t->count = t->count2 = k ? 2 : PORT_VANILLA_PARTS;
            t->rowSize = k ? 8 : 0x50;
            t->fieldCount = k ? 2 : 20;
            for (u32 i = 0; i < t->fieldCount; i++) { t->fieldType[i] = GAMEDB_T_INT; t->fieldSize[i] = 4; t->fieldName[i] = 0; }
            t->fieldName[k ? 1 : 5] = NameHash("Power");
            t->rows = (u8*)PortHostAlloc(t->count * t->rowSize);
            for (u32 i = 0; i < t->count; i++) { s32* r = (s32*)(t->rows + i * t->rowSize); r[0] = (s32)i; if (!k) { r[1] = 0x1000 + i; r[5] = (s32)i; } }
        }
        strings.strings = parts;
        strings.count = PORT_VANILLA_PARTS;
        gPortModCount = 0;
        PortModsScan();
    };
    auto use = [&](const char* order) {          // the mods named, in this order; the others are off
        gPortModCount = 0;
        for (const char* c = order; *c; c++) { char n[2] = {*c, 0}; PortModsAdd(n, TRUE); }
        for (const char* n : names) if (!strchr(order, n[0])) PortModsAdd(n, FALSE);
        PortModsScan();
        PortModPatchRun(db, &strings, sidecar.c_str());
    };
    auto row = [&](int r) { return (const s32*)(Table(db, PART_TABLE)->rows + r * 0x50); };

    gPortOptions.noMods = FALSE;
    PortModsSetDir(base);
    fresh();
    use("AB");
    PCHECK(PortModPartIndex("A:head") == 186 && PortModPartIndex("b:HEAD") == 187 && PortModPartCount() == 2 && Table(db, PART_TABLE)->count == 188,
           "two mods did not get rows 186 and 187 (%d, %d)", PortModPartIndex("A:head"), PortModPartIndex("B:head"));
    PCHECK(row(186)[0] == 186 && row(186)[5] == 100 && row(187)[5] == 101 && row(10)[5] == 10, "a part is not a patched copy");
    s32 text = PortModTextId((u32)row(186)[1]);
    PCHECK(text >= PORT_VANILLA_PARTS && (u32)text < strings.count && !strcmp(strings.strings[text], "Head A") && row(186)[2] == 0,
           "a part's names are wrong");
    PCHECK(PortModPartData(0, "model") && !strcmp(PortModPartData(0, "model"), "m.bin") && !PortModPartData(0, "none"), "\"data\" is not passed on");
    {
        GameDbTable* t = Table(db, 5);
        const s32* r = (const s32*)t->rows;
        PCHECK(t->count == 3 && t->count2 == 3 && r[0] == 0 && r[1] == 0 && r[2] == 7 && r[3] == -3 && r[4] == 9 && r[5] == -3,
               "the table patches gave %d %d %d %d %d %d", r[0], r[1], r[2], r[3], r[4], r[5]);
    }
    fresh();
    use("BA");                                   // reordered
    PCHECK(PortModPartIndex("A:head") == 186 && PortModPartIndex("B:head") == 187, "changing the load order moved a part");
    fresh();
    use("B");                                    // A removed
    PCHECK(PortModPartIndex("A:head") == -1 && PortModPartIndex("B:head") == 187 && Table(db, PART_TABLE)->count == 188 &&
           row(186)[0] == 186 && row(186)[5] == 10 && row(186)[1] == 0x100A, "a removed mod's part is not shown as its vanilla part");
    fresh();
    use("CB");                                   // a new mod does not take the free row
    PCHECK(PortModPartIndex("C:head") == 188 && PortModPartIndex("B:head") == 187, "a new mod took a used row");
    fresh();
    use("");                                     // no mod: every row is still there
    PCHECK(PortModPartCount() == 0 && Table(db, PART_TABLE)->count == 189 && row(188)[5] == 12, "without mods the rows are gone");
    fresh();
    use("ABC");
    PCHECK(PortModPartIndex("A:head") == 186 && PortModPartIndex("B:head") == 187 && PortModPartIndex("C:head") == 188 && row(186)[5] == 100,
           "switching the mods on again did not restore their parts");
    std::string kept;
    ReadWhole(sidecar.c_str(), &kept);
    PCHECK(kept.find("186|10|A:head\n187|11|B:head\n188|12|C:head\n") != std::string::npos, "save.modparts is not as expected: %s", kept.c_str());
    {
        GameDb fake = {};
        fake.data = db;
        GameDb* keepDb = gGameDb;
        gGameDb = &fake;
        PortModPartEnsureRow(191);
        PCHECK(Table(db, PART_TABLE)->count == 192 && row(191)[0] == 191 && row(186)[5] == 100, "an unknown row of a save was not added");
        gGameDb = keepDb;
    }
#undef PCHECK
    DeleteFileA(sidecar.c_str());
    for (const char* n : names) {
        for (const char* f : {"\\patches\\parts.json", "\\patches\\tables.json", "\\patches\\broken.json"}) DeleteFileA((root + "\\" + n + f).c_str());
        RemoveDirectoryA((root + "\\" + n + "\\patches").c_str());
        RemoveDirectoryA((root + "\\" + n).c_str());
    }
    RemoveDirectoryA(base);
    free(db);
    S().parts.clear();
    S().list.clear();
    memset(sTextId, 0, sizeof(sTextId));
    sApplied = FALSE;
    memcpy(gPortMods, keep, sizeof(keep));
    gPortModCount = keepCount;
    gPortOptions.noMods = keepOff;
    PortModsSetDir(keepDir);
    PortModsRefreshTextures();
    if (!fails) PortLog("selftest modpatch: ok (table patches, added parts, names, save.modparts across adding, removing and reordering mods)");
    return fails;
}
#endif
