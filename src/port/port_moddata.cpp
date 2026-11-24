// PC port: data mods. A mod's data/strings.json overrides entries of the game's text table (global_binary.sir,
// header slot 0xDA8: 4,274 strings that the game's string lookups read by number). --dump-data DIR writes the
// original table from the player's own disc in the same format, so it can be edited and put back.
// See docs/modding.md. The dump is game data: it never goes into the repository.
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "port/port.h"
#include "port/port_mods.h"
#include "port/sir_loader.h"

namespace {

// ---- a small JSON reader for {"strings": {"<id>": "<text>", ...}}: errors say where
struct Reader {
    const char* p;
    const char* start;
    std::string err;
    void Fail(const char* what) {
        if (!err.empty()) return;
        int line = 1, col = 1;
        for (const char* c = start; c < p; c++) { if (*c == '\n') { line++; col = 1; } else col++; }
        char buf[220];
        snprintf(buf, sizeof(buf), "line %d, column %d: %s", line, col, what);
        err = buf;
    }
    void Space() { while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++; }
    bool Eat(char c) { Space(); if (*p == c) { p++; return true; } return false; }
    bool Expect(char c) {
        if (Eat(c)) return true;
        char buf[40];
        snprintf(buf, sizeof(buf), "'%c' expected", c);
        Fail(buf);
        return false;
    }
    // A string as bytes of the game's text (single byte: U+0000-U+00FF).
    bool Str(std::string* out) {
        Space();
        if (*p != '"') { Fail("a string in quotes expected"); return false; }
        p++;
        out->clear();
        for (;;) {
            unsigned char c = (unsigned char)*p;
            if (!c) { Fail("the string is not closed"); return false; }
            if (c == '"') { p++; return true; }
            u32 cp;
            if (c == '\\') {
                p++;
                char e = *p++;
                switch (e) {
                case '"': cp = '"'; break;
                case '\\': cp = '\\'; break;
                case '/': cp = '/'; break;
                case 'b': cp = 8; break;
                case 'f': cp = 12; break;
                case 'n': cp = 10; break;
                case 'r': cp = 13; break;
                case 't': cp = 9; break;
                case 'u': {
                    cp = 0;
                    for (int i = 0; i < 4; i++) {
                        char h = *p++;
                        int d = h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : h >= 'A' && h <= 'F' ? h - 'A' + 10 : -1;
                        if (d < 0) { p--; Fail("four hex digits expected after \\u"); return false; }
                        cp = cp << 4 | (u32)d;
                    }
                    break;
                }
                default: p--; Fail("unknown escape in a string"); return false;
                }
            } else if (c < 0x20) {
                Fail("a line break or control character inside a string (write \\n)");
                return false;
            } else if (c < 0x80) {
                cp = c;
                p++;
            } else {                                         // UTF-8
                int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : -1;
                if (n < 0) { Fail("the file is not UTF-8"); return false; }
                cp = c & (0x3F >> n);
                p++;
                for (int i = 0; i < n; i++) {
                    if ((*p & 0xC0) != 0x80) { Fail("the file is not UTF-8"); return false; }
                    cp = cp << 6 | (u32)(*p++ & 0x3F);
                }
            }
            if (cp > 0xFF) {
                char buf[120];
                snprintf(buf, sizeof(buf), "character U+%04X is not in the game's single-byte text (only U+0000-U+00FF)", cp);
                Fail(buf);
                return false;
            }
            if (cp == 0) { Fail("a string cannot hold U+0000"); return false; }
            out->push_back((char)cp);
        }
    }
};

struct Change { u32 id; std::string text; };

// Parses the whole file; fills `out` or sets r.err. Nothing is applied here.
static void ParseStrings(Reader& r, u32 count, std::vector<Change>* out) {
    if (!r.Expect('{')) return;
    bool seen = false;
    if (!r.Eat('}')) {
        for (;;) {
            std::string key;
            r.Space();
            const char* keyAt = r.p;
            if (!r.Str(&key)) return;
            if (!r.Expect(':')) return;
            if (key == "format") {
                r.Space();
                if (*r.p != '1') { r.Fail("\"format\" must be 1"); return; }
                r.p++;
            } else if (key == "table") {
                std::string t;
                if (!r.Str(&t)) return;
                if (t != "strings") { r.Fail("\"table\" must be \"strings\""); return; }
            } else if (key == "count") {
                r.Space();
                while (*r.p >= '0' && *r.p <= '9') r.p++;    // informational, written by the dump
            } else if (key == "strings") {
                seen = true;
                if (!r.Expect('{')) return;
                if (!r.Eat('}')) {
                    for (;;) {
                        std::string id, text;
                        r.Space();
                        const char* at = r.p;
                        if (!r.Str(&id)) return;
                        char* end = NULL;
                        unsigned long v = id.empty() ? 0 : strtoul(id.c_str(), &end, 10);
                        if (id.empty() || id[0] < '0' || id[0] > '9' || *end) { r.p = at; r.Fail("a string number in quotes expected (\"12\")"); return; }
                        if (v >= count) {
                            char buf[120];
                            snprintf(buf, sizeof(buf), "string %lu does not exist (the game has %u: 0-%u)", v, count, count - 1);
                            r.p = at;
                            r.Fail(buf);
                            return;
                        }
                        if (!r.Expect(':') || !r.Str(&text)) return;
                        out->push_back({(u32)v, text});
                        if (r.Eat(',')) continue;
                        if (!r.Expect('}')) return;
                        break;
                    }
                }
            } else {
                r.p = keyAt;
                std::string msg = "unknown key \"" + key + "\" (expected format, table, count, strings)";
                r.Fail(msg.c_str());
                return;
            }
            if (r.Eat(',')) continue;
            if (!r.Expect('}')) return;
            break;
        }
    }
    r.Space();
    if (*r.p) { r.Fail("text after the end of the object"); return; }
    if (!seen) r.Fail("no \"strings\" object");
}

static bool ReadWhole(const char* path, std::string* out) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    char buf[4096];
    size_t n;
    out->clear();
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out->append(buf, n);
    fclose(f);
    if (out->size() >= 3 && !memcmp(out->data(), "\xEF\xBB\xBF", 3)) out->erase(0, 3);   // a BOM
    return true;
}

static void DumpStrings(const SirStringTable* t, const char* dir) {
    CreateDirectoryA(dir, NULL);
    char path[MAX_PATH + 32];
    snprintf(path, sizeof(path), "%s\\strings.json", dir);
    FILE* f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "[port] --dump-data: cannot write %s\n", path); return; }
    fprintf(f, "{\n  \"format\": 1,\n  \"table\": \"strings\",\n  \"count\": %u,\n  \"strings\": {\n", t->count);
    for (u32 i = 0; i < t->count; i++) {
        fprintf(f, "    \"%u\": \"", i);
        const char* s = t->strings[i];
        for (; s && *s; s++) {
            unsigned char c = (unsigned char)*s;
            if (c == '"') fputs("\\\"", f);
            else if (c == '\\') fputs("\\\\", f);
            else if (c == '\n') fputs("\\n", f);
            else if (c < 0x20 || c >= 0x7F) fprintf(f, "\\u%04X", c);
            else fputc(c, f);
        }
        fprintf(f, "\"%s\n", i + 1 < t->count ? "," : "");
    }
    fputs("  }\n}\n", f);
    fclose(f);
    fprintf(stderr, "[port] --dump-data: %u strings written to %s\n", t->count, path);
}

}  // namespace

int PortModsApplyStrings(const char* json, const char** strings, u32 count, char* err, u32 errSize) {
    Reader r;
    r.p = r.start = json;
    std::vector<Change> changes;
    ParseStrings(r, count, &changes);
    if (!r.err.empty()) { snprintf(err, errSize, "%s", r.err.c_str()); return -1; }
    for (const Change& c : changes) {
        char* copy = (char*)malloc(c.text.size() + 1);       // lives as long as the game
        if (!copy) continue;
        memcpy(copy, c.text.c_str(), c.text.size() + 1);
        strings[c.id] = copy;
    }
    return (int)changes.size();
}

void PortModsApplyData(u8* globalHdr) {
    SirStringTable* t = NULL;
    if (globalHdr) memcpy(&t, globalHdr + 0xDA8, sizeof(t));
    if (!t || !t->strings || !t->count) return;
    if (gPortOptions.dumpData) {
        DumpStrings(t, gPortOptions.dumpData);
        fflush(stderr);
        ExitProcess(0);                                      // the original is what was asked for: stop before any mod is applied
    }
    if (gPortOptions.noMods) return;
    for (int i = 0; i < gPortModCount; i++) {
        const PortMod* m = &gPortMods[i];
        if (!m->enabled || !m->present) continue;
        char path[MAX_PATH + 64];
        if ((u32)snprintf(path, sizeof(path), "%s\\%s\\data\\strings.json", PortModsDir(), m->name) >= sizeof(path)) continue;
        std::string text;
        char err[256];
        if (!ReadWhole(path, &text)) continue;
        int n = PortModsApplyStrings(text.c_str(), t->strings, t->count, err, sizeof(err));
        if (n < 0) fprintf(stderr, "[port] mods: %s: %s: %s; the file is ignored\n", m->name, path, err);
        else PortLog("mods: %s changes %d string%s (data/strings.json)", m->name, n, n == 1 ? "" : "s");
    }
}
#endif
