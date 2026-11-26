// PC platform layer: mods. A folder mods/<name>/ next to the exe is a mod; what is under its
// files/ folder replaces the disc file of the same path, whether the game runs from a disc image or
// from an extracted folder (DVDOpen asks here first); a PNG or DDS under its textures/ folder replaces a
// game texture, matched by content hash (the renderer asks here when it decodes one). The list, its order and which mods are on are
// kept in monsterlab.ini ([mods]); a later mod wins over an earlier one. See docs/modding.md.
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <set>
#include <string>

#include "port/port.h"
#include "port/port_mods.h"
#include "port/sir_loader.h"

PortMod gPortMods[PORT_MAX_MODS];
int     gPortModCount;

static char sDir[MAX_PATH];

const char* PortModsDir(void) {
    if (!sDir[0]) {
        DWORD n = GetModuleFileNameA(NULL, sDir, MAX_PATH - 8);
        char* slash = n && n < MAX_PATH - 8 ? strrchr(sDir, '\\') : NULL;
        if (slash) strcpy(slash + 1, "mods");
        else strcpy(sDir, "mods");
    }
    return sDir;
}

void PortModsSetDir(const char* dir) {
    snprintf(sDir, sizeof(sDir), "%s", dir);
    for (char* c = sDir; *c; c++) if (*c == '/') *c = '\\';
    size_t n = strlen(sDir);
    if (n > 3 && sDir[n - 1] == '\\') sDir[n - 1] = 0;
}

// A mod's name is one folder name: no path parts, so the ini cannot point outside the mods folder.
static BOOL GoodName(const char* name) {
    if (!name[0] || strlen(name) >= sizeof(gPortMods[0].name) || !strcmp(name, ".") || !strcmp(name, "..")) return FALSE;
    return strpbrk(name, "\\/:*?\"<>|") == NULL;
}

PortMod* PortModsAdd(const char* name, BOOL enabled) {
    if (!GoodName(name)) return NULL;
    for (int i = 0; i < gPortModCount; i++)
        if (!_stricmp(gPortMods[i].name, name)) { gPortMods[i].enabled = enabled; return &gPortMods[i]; }
    if (gPortModCount >= PORT_MAX_MODS) return NULL;
    PortMod* m = &gPortMods[gPortModCount++];
    memset(m, 0, sizeof(*m));
    strcpy(m->name, name);
    m->enabled = enabled;
    return m;
}

static u32 WalkTree(const PortMod* m, const char* dir, const char* rel);
void PortModsRefreshTextures(void);

void PortModsScan(void) {
    for (int i = 0; i < gPortModCount; i++) { gPortMods[i].present = FALSE; gPortMods[i].fileCount = 0; }
    char pattern[MAX_PATH + 8];
    snprintf(pattern, sizeof(pattern), "%s\\*", PortModsDir());
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) { PortModsRefreshTextures(); return; }
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !GoodName(fd.cFileName)) continue;
        PortMod* m = NULL;
        for (int i = 0; i < gPortModCount && !m; i++)
            if (!_stricmp(gPortMods[i].name, fd.cFileName)) m = &gPortMods[i];
        if (!m) m = PortModsAdd(fd.cFileName, TRUE);       // a folder dropped in is on until switched off
        if (m) {
            m->present = TRUE;
            char dir[MAX_PATH];
            m->fileCount = (u32)snprintf(dir, sizeof(dir), "%s\\%s\\files", PortModsDir(), m->name) < sizeof(dir) ? WalkTree(NULL, dir, "") : 0;
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    PortModsRefreshTextures();
}

BOOL PortModsMove(int index, int dir) {
    int other = index + (dir < 0 ? -1 : 1);
    if (index < 0 || index >= gPortModCount || other < 0 || other >= gPortModCount) return FALSE;
    PortMod t = gPortMods[index];
    gPortMods[index] = gPortMods[other];
    gPortMods[other] = t;
    return TRUE;
}

BOOL PortModsFindFile(const char* discPath, char* host, u32 n, const char** modName) {
    while (*discPath == '/' || *discPath == '\\') discPath++;
    if (gPortOptions.noMods || !*discPath || strstr(discPath, "..")) return FALSE;
    for (int i = gPortModCount - 1; i >= 0; i--) {
        const PortMod* m = &gPortMods[i];
        if (!m->enabled || !m->present) continue;
        if ((u32)snprintf(host, n, "%s\\%s\\files\\%s", PortModsDir(), m->name, discPath) >= n) continue;
        for (char* c = host; *c; c++) if (*c == '/') *c = '\\';
        DWORD a = GetFileAttributesA(host);
        if (a == INVALID_FILE_ATTRIBUTES || (a & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (modName) *modName = m->name;
        return TRUE;
    }
    return FALSE;
}

// Every file under dir (a mod's files folder), as its disc path: counted, and logged for a mod m.
static u32 WalkTree(const PortMod* m, const char* dir, const char* rel) {
    char pattern[MAX_PATH + 8];
    if ((u32)snprintf(pattern, sizeof(pattern), "%s\\*", dir) >= sizeof(pattern)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    u32 count = 0;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        char sub[MAX_PATH], subRel[MAX_PATH];
        if ((u32)snprintf(sub, sizeof(sub), "%s\\%s", dir, fd.cFileName) >= sizeof(sub)) continue;
        snprintf(subRel, sizeof(subRel), "%s%s%s", rel, rel[0] ? "/" : "", fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { count += WalkTree(m, sub, subRel); continue; }
        count++;
        if (!m) continue;
        // Shadowed when a later mod has the file too.
        char host[MAX_PATH];
        const char* winner = NULL;
        PortModsFindFile(subRel, host, sizeof(host), &winner);
        if (winner && _stricmp(winner, m->name)) PortLog("mods: %s has %s, but %s comes later and wins", m->name, subRel, winner);
        else PortLog("mods: %s replaces %s", m->name, subRel);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return count;
}

void PortModsLogOverrides(void) {
    if (gPortOptions.noMods) return;
    for (int i = 0; i < gPortModCount; i++) {
        const PortMod* m = &gPortMods[i];
        if (!m->present) { PortLog("mods: %s is listed in monsterlab.ini but not in %s", m->name, PortModsDir()); continue; }
        if (!m->enabled) { PortLog("mods: %s is switched off", m->name); continue; }
        char dir[MAX_PATH];
        snprintf(dir, sizeof(dir), "%s\\%s\\files", PortModsDir(), m->name);
        if (!WalkTree(m, dir, "") && !m->textureCount) PortLog("mods: %s replaces no disc file and no texture", m->name);
        if (m->textureCount) PortLog("mods: %s has %u texture%s", m->name, m->textureCount, m->textureCount == 1 ? "" : "s");
    }
}

// ------------------------------------------------------------------ textures
// A PNG or DDS under mods/<name>/textures/ (any depth of folders) replaces the game texture whose content
// hash is the last 16 characters of its name (before the extension): "anything_0123456789ABCDEF.png", the name --dump-textures
// gives it. The hash is over the texture as the game holds it (size, format, pixel data, palette),
// so it is the same on every disc of this game and no game data is needed to tell which is which.
// Made at first use: the mods are scanned before this file's static objects are built (port_runtime.cpp).
struct TexState {
    std::map<u64, std::string> files;            // hash -> PNG or DDS, from the last enabled mod that has it
    std::map<u64, std::string> mods;             // hash -> that mod's name
    std::set<u64> dumped, logged;
    std::map<std::string, std::string> byName;   // lower-case texture name -> PNG (a file whose name has no hash)
    std::map<std::string, std::string> nameMods; // lower-case name -> that mod's name
    std::map<uintptr_t, std::string> names;      // pixel data of a texture the loader converted -> its name
    std::set<std::string> loggedNames;
};
static TexState& Tex(void) { static TexState* t = new TexState; return *t; }
#define sTexFiles  Tex().files
#define sTexMods   Tex().mods
#define sTexDumped Tex().dumped
#define sTexLogged Tex().logged
static char sDumpingTo[MAX_PATH];                 // the folder sTexDumped is about
static u32 sDumpWritten, sDumpThere, sDumpFailed;

static std::string Lower(const char* s) {
    std::string r = s;
    for (char& c : r) if (c >= 'A' && c <= 'Z') c += 32;
    return r;
}

// The loader calls this for every texture it converts: which pixel data has which name.
static void NoteTexture(const void* pixels, const char* name) { Tex().names[(uintptr_t)pixels] = name; }
static void HookNames(void) { gSirTextureNote = NoteTexture; }

const char* PortModsTextureName(const void* pixels) {
    auto it = Tex().names.find((uintptr_t)pixels);
    return it == Tex().names.end() ? NULL : it->second.c_str();
}

u64 PortModsTextureHash(const u8* data, u32 bytes, const u8* palette, u32 paletteBytes, u32 w, u32 h, u32 fmt) {
    u64 hash = 14695981039346656037ull;
    const u8 head[5] = {(u8)w, (u8)(w >> 8), (u8)h, (u8)(h >> 8), (u8)fmt};
    for (u8 c : head) hash = (hash ^ c) * 1099511628211ull;
    for (u32 i = 0; i < bytes; i++) hash = (hash ^ data[i]) * 1099511628211ull;
    for (u32 i = 0; i < paletteBytes; i++) hash = (hash ^ palette[i]) * 1099511628211ull;
    return hash;
}

static BOOL IsExt(const char* name, const char* ext) {
    size_t n = strlen(name);
    return n > 4 && !_stricmp(name + n - 4, ext);
}

// The hash a texture file's name ends in ("..._0123456789abcdef.png" or ".dds"), FALSE for another name.
static BOOL NameHash(const char* name, u64* hash) {
    size_t n = strlen(name);
    if (n < 20) return FALSE;
    u64 v = 0;
    for (size_t i = n - 20; i < n - 4; i++) {
        char c = name[i];
        int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (d < 0) return FALSE;
        v = v << 4 | (u64)d;
    }
    *hash = v;
    return TRUE;
}

// Puts a file into the table under `key`. A later mod overwrites an earlier one; within one mod a DDS
// wins over a PNG of the same texture.
template <class K>
static void TexAdd(std::map<K, std::string>& files, std::map<K, std::string>& mods, const K& key, const char* path, const PortMod* m, const char* what) {
    auto old = mods.find(key);
    if (old != mods.end()) {
        if (_stricmp(old->second.c_str(), m->name)) PortLog("mods: %s and %s both replace texture %s: %s comes later and wins", old->second.c_str(), m->name, what, m->name);
        else if (IsExt(files[key].c_str(), ".dds") && !IsExt(path, ".dds")) return;
    }
    files[key] = path;
    mods[key] = m->name;
}

// Every PNG and DDS under dir goes into the table. `seen` collects the textures this mod replaces.
static void TexTree(const PortMod* m, const char* dir, BOOL add, std::set<std::string>& seen) {
    char pattern[MAX_PATH + 8];
    if ((u32)snprintf(pattern, sizeof(pattern), "%s\\*", dir) >= sizeof(pattern)) return;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        char sub[MAX_PATH];
        if ((u32)snprintf(sub, sizeof(sub), "%s\\%s", dir, fd.cFileName) >= sizeof(sub)) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { TexTree(m, sub, add, seen); continue; }
        if (!IsExt(fd.cFileName, ".png") && !IsExt(fd.cFileName, ".dds")) continue;
        u64 hash;
        if (NameHash(fd.cFileName, &hash)) {
            char what[24];
            snprintf(what, sizeof(what), "%016llX", hash);
            seen.insert(what);
            if (add) TexAdd(sTexFiles, sTexMods, hash, sub, m, what);
        } else {                                             // no hash: the name of the texture in the game's data
            std::string key = Lower(fd.cFileName);
            key.resize(key.size() - 4);
            seen.insert(key);
            if (add) TexAdd(Tex().byName, Tex().nameMods, key, sub, m, key.c_str());
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

void PortModsRefreshTextures(void) {
    sTexFiles.clear();
    sTexMods.clear();
    sTexLogged.clear();
    Tex().byName.clear();
    Tex().nameMods.clear();
    Tex().loggedNames.clear();
    HookNames();
    for (int i = 0; i < gPortModCount; i++) {
        PortMod* m = &gPortMods[i];
        m->textureCount = 0;
        if (!m->present) continue;
        char dir[MAX_PATH];
        if ((u32)snprintf(dir, sizeof(dir), "%s\\%s\\textures", PortModsDir(), m->name) >= sizeof(dir)) continue;
        std::set<std::string> seen;
        TexTree(m, dir, m->enabled && !gPortOptions.noMods, seen);
        m->textureCount = (u32)seen.size();
    }
}

BOOL PortModsTexturesWanted(void) { return !sTexFiles.empty() || !Tex().byName.empty() || PortModsDumpDir() != NULL; }

BOOL PortModsTexture(u64 hash, const char* name, PortModImage* out) {
    const std::string* file = NULL;
    const std::string* mod = NULL;
    std::string key;
    if (name) {                                              // by the name in the game's data first, the hash is the fallback
        key = Lower(name);
        auto n = Tex().byName.find(key);
        if (n != Tex().byName.end()) { file = &n->second; mod = &Tex().nameMods[key]; }
    }
    if (!file) {
        auto it = sTexFiles.find(hash);
        if (it == sTexFiles.end()) return FALSE;
        file = &it->second;
        mod = &sTexMods[hash];
        key.clear();
    }
    const char* why = NULL;
    memset(out, 0, sizeof(*out));
    if (IsExt(file->c_str(), ".dds")) PortDdsRead(file->c_str(), out, &why);
    else {
        out->data = (u8*)PortPngRead(file->c_str(), &out->w, &out->h, &why);
        out->levels = 1;
    }
    BOOL first = key.empty() ? sTexLogged.insert(hash).second : Tex().loggedNames.insert(key).second;   // once per texture: the game decodes some again and again
    if (first) {
        char kind[48] = "";
        if (out->dxgi) snprintf(kind, sizeof(kind), ", %s, %u mip level%s", out->dxgi == PORT_DXGI_BC1 ? "BC1" : out->dxgi == PORT_DXGI_BC3 ? "BC3" : "BC7", out->levels, out->levels == 1 ? "" : "s");
        if (out->data && key.empty()) PortLog("mods: texture %016llX comes from %s (%ux%u%s, %s)", hash, mod->c_str(), out->w, out->h, kind, file->c_str());
        else if (out->data) PortLog("mods: texture %s (%016llX) comes from %s by name (%ux%u%s, %s)", name, hash, mod->c_str(), out->w, out->h, kind, file->c_str());
        else fprintf(stderr, "[port] mods: %s %s; the game's texture is used\n", file->c_str(), why ? why : "cannot be read");
    }
    return out->data != NULL;
}

const char* PortModsDumpFolder(void) {
    static char folder[MAX_PATH];
    const char* set = gPortSettings.dumpDir;
    if (set[0] && (set[0] == '\\' || set[0] == '/' || set[1] == ':')) return set;
    DWORD n = GetModuleFileNameA(NULL, folder, MAX_PATH);    // none set, or a relative one: from the exe's folder
    char* slash = n && n < MAX_PATH ? strrchr(folder, '\\') : NULL;
    size_t at = slash ? (size_t)(slash + 1 - folder) : 0;
    snprintf(folder + at, sizeof(folder) - at, "%s", set[0] ? set : "dump\\textures");
    return folder;
}

const char* PortModsDumpDir(void) {
    if (gPortOptions.dumpTextures) return gPortOptions.dumpTextures;   // the command line wins
    return gPortSettings.dumpTextures ? PortModsDumpFolder() : NULL;
}

// Makes the folder and those above it.
static void MakeFolders(const char* dir) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s", dir);
    for (char* c = path + 1; *c; c++)
        if ((*c == '\\' || *c == '/') && c[-1] != ':') { char keep = *c; *c = 0; CreateDirectoryA(path, NULL); *c = keep; }
    CreateDirectoryA(path, NULL);
}

void PortModsDumpTexture(u64 hash, const char* name, const u32* rgba, u32 w, u32 h, u32 fmt) {
    const char* dir = PortModsDumpDir();
    if (!dir) return;
    if (strcmp(dir, sDumpingTo)) {                           // another folder: it gets every texture drawn from now on
        snprintf(sDumpingTo, sizeof(sDumpingTo), "%s", dir);
        sTexDumped.clear();
        sDumpWritten = sDumpThere = sDumpFailed = 0;
    }
    if (!sTexDumped.insert(hash).second) return;
    char path[MAX_PATH + 160];
    if (name && *name) {                                     // tex_<w>x<h>_f<fmt>_<name>_<hash>.png: works by hash as it is; rename it to <name>.png to match by name
        char clean[96];
        snprintf(clean, sizeof(clean), "%s", name);
        for (char* c = clean; *c; c++) if (strchr("\\/:*?\"<>|", *c) || (u8)*c < 32) *c = '_';
        snprintf(path, sizeof(path), "%s\\tex_%ux%u_f%u_%s_%016llX.png", dir, w, h, fmt, clean, hash);
    } else
        snprintf(path, sizeof(path), "%s\\tex_%ux%u_f%u_%016llX.png", dir, w, h, fmt, hash);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) { sDumpThere++; return; }
    MakeFolders(dir);
    if (PortPngWrite(path, rgba, w, h)) sDumpWritten++;
    else if (!sDumpFailed++) fprintf(stderr, "[port] texture dumping: cannot write %s\n", path);
}

u32 PortModsDumpCount(void) { return sDumpWritten; }
u32 PortModsDumpPresent(void) { return sDumpThere; }
u32 PortModsDumpFailed(void) { return sDumpFailed; }

// Two mods made in a temporary folder (no game data): order, switching off, missing files.
int PortSelfTestMods(void) {
    int fails = 0;
    PortMod keep[PORT_MAX_MODS];
    int keepCount = gPortModCount;
    char keepDir[MAX_PATH];
    BOOL keepOff = gPortOptions.noMods;
    gPortOptions.noMods = FALSE;
    memcpy(keep, gPortMods, sizeof(keep));
    strcpy(keepDir, sDir);
    char base[MAX_PATH], path[MAX_PATH + 64];
    DWORD n = GetTempPathA(MAX_PATH - 40, base);
    snprintf(base + n, sizeof(base) - n, "monsterlab_selftest_mods_%lu", GetCurrentProcessId());
    static const char* const kFiles[] = {"first\\files\\Streams\\a.sir", "first\\files\\only_first.bin", "second\\files\\Streams\\a.sir"};
    static const char* const kDirs[] = {"", "\\first", "\\first\\files", "\\first\\files\\Streams", "\\second", "\\second\\files", "\\second\\files\\Streams"};
    for (const char* d : kDirs) { snprintf(path, sizeof(path), "%s%s", base, d); CreateDirectoryA(path, NULL); }
    for (const char* f : kFiles) {
        snprintf(path, sizeof(path), "%s\\%s", base, f);
        FILE* fp = fopen(path, "wb");
        if (fp) { fputs("mod", fp); fclose(fp); } else { PortLog("selftest mods: cannot write %s", path); fails++; }
    }
    gPortModCount = 0;
    PortModsSetDir(base);
    PortModsAdd("second", TRUE);                       // the ini lists second, then gone; the scan appends first
    PortModsAdd("gone", TRUE);
    PortModsScan();
    char host[MAX_PATH];
    const char* who = NULL;
#define MOD_CHECK(cond, ...) do { if (!(cond)) { PortLog("selftest mods: " __VA_ARGS__); fails++; } } while (0)
    MOD_CHECK(gPortModCount == 3 && !strcmp(gPortMods[2].name, "first") && gPortMods[2].enabled && gPortMods[0].present && !gPortMods[1].present,
              "the scan gave %d mods", gPortModCount);
    MOD_CHECK(PortModsFindFile("/Streams/a.sir", host, sizeof(host), &who) && !strcmp(who, "first"), "the later mod did not win (%s)", who ? who : "none");
    MOD_CHECK(PortModsFindFile("streams\\A.SIR", host, sizeof(host), &who), "a path with other case and slashes was not found");
    gPortMods[2].enabled = FALSE;
    MOD_CHECK(PortModsFindFile("Streams/a.sir", host, sizeof(host), &who) && !strcmp(who, "second"), "a mod switched off still wins");
    MOD_CHECK(!PortModsFindFile("only_first.bin", host, sizeof(host), &who), "a file of a mod switched off was found");
    gPortMods[2].enabled = TRUE;
    MOD_CHECK(PortModsFindFile("only_first.bin", host, sizeof(host), &who), "only_first.bin was not found");
    MOD_CHECK(!PortModsFindFile("Streams/none.sir", host, sizeof(host), &who) && !PortModsFindFile("Streams", host, sizeof(host), &who) &&
              !PortModsFindFile("../first/files/only_first.bin", host, sizeof(host), &who), "a file that no mod replaces was found");
    MOD_CHECK(gPortMods[2].fileCount == 2 && gPortMods[0].fileCount == 1, "the scan counted %u and %u files", gPortMods[2].fileCount, gPortMods[0].fileCount);
    MOD_CHECK(PortModsMove(2, -1) && PortModsMove(1, -1) && !PortModsMove(0, -1) && !strcmp(gPortMods[0].name, "first") &&
              PortModsFindFile("Streams/a.sir", host, sizeof(host), &who) && !strcmp(who, "second"), "moving a mod up did not change who wins");
    MOD_CHECK(!PortModsAdd("..\\outside", TRUE) && !PortModsAdd("", TRUE), "a name that is no folder name was accepted");

    // Textures: a real PNG (deflated, three row filters, a transparent colour), the writer's own output, the name rule.
    {
        static const u8 kPng[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x08, 0x02, 0x00, 0x00, 0x00, 0xD4, 0x54, 0x52, 0xAF, 0x00, 0x00, 0x00, 0x06, 0x74, 0x52, 0x4E, 0x53, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0xC9, 0x4B, 0xAB, 0xF5, 0x00, 0x00, 0x00, 0x2D, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0x64, 0x60, 0xF8, 0x6F, 0xC4, 0x7E, 0x03, 0x8E, 0x58, 0x44, 0xA2, 0x18, 0x44, 0xD8, 0x19, 0xE6, 0x2E, 0x0C, 0x71, 0x4B, 0x6C, 0x01, 0x32, 0x98, 0xE5, 0xDA, 0x1B, 0x94, 0x0D, 0xDF, 0xC4, 0xA7, 0x38, 0x01, 0x49, 0x20, 0x02, 0x00, 0x4B, 0xC3, 0x0E, 0x97, 0xF6, 0x97, 0x2F, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};
        static const u32 kWant[15] = {0xFFFF0000u, 0xFFD70732u, 0xFFAF0E64u, 0xFF871596u, 0xFF5F1CC8u, 0xFFFF5A14u, 0xFFD76146u, 0x00030201u, 0xFF876FAAu, 0xFF5F76DCu, 0xFFFFB428u, 0xFFD7BB5Au, 0xFFAFC28Cu, 0xFF87C9BEu, 0xFF5FD0F0u};
        static const char* const kTexDirs[] = {"\\first\\textures", "\\first\\textures\\ui", "\\second\\textures"};
        static const char* const kTexFiles[] = {"first\\textures\\ui\\logo_00000000000000AB.png", "second\\textures\\x_00000000000000ab.png",
                                                "second\\textures\\own_00000000000000CD.png", "second\\textures\\noname.png"};
        for (const char* d : kTexDirs) { snprintf(path, sizeof(path), "%s%s", base, d); CreateDirectoryA(path, NULL); }
        for (int i = 0; i < 2; i++) {
            snprintf(path, sizeof(path), "%s\\%s", base, kTexFiles[i]);
            FILE* fp = fopen(path, "wb");
            if (fp) { fwrite(kPng, 1, sizeof(kPng), fp); fclose(fp); } else fails++;
        }
        u32 own[6] = {0x11223344u, 0xFF000000u, 0x00FFFFFFu, 0x80402010u, 1, 0xFFFFFFFFu};
        snprintf(path, sizeof(path), "%s\\%s", base, kTexFiles[2]);
        MOD_CHECK(PortPngWrite(path, own, 3, 2), "the PNG writer failed");
        snprintf(path, sizeof(path), "%s\\%s", base, kTexFiles[3]);
        PortPngWrite(path, own, 3, 2);
        gPortMods[0].enabled = gPortMods[1].enabled = gPortMods[2].enabled = TRUE;   // order now: first, second, gone
        PortModsRefreshTextures();
        u32 tw = 0, th = 0;
        // The RGBA picture of a replacement, as the checks below want it.
        auto read = [](u64 hash, const char* name, u32* pw, u32* ph) -> u32* {
            PortModImage im;
            if (!PortModsTexture(hash, name, &im)) return NULL;
            *pw = im.w; *ph = im.h;
            return (u32*)im.data;
        };
        u32* px = read(0xABull, NULL, &tw, &th);
        MOD_CHECK(px && tw == 5 && th == 3 && !memcmp(px, kWant, sizeof(kWant)), "a deflated PNG was not read right (%ux%u)", tw, th);
        free(px);
        MOD_CHECK(sTexMods.count(0xAB) && sTexMods[0xAB] == "second", "the later mod's texture did not win");
        px = read(0xCDull, NULL, &tw, &th);
        MOD_CHECK(px && tw == 3 && th == 2 && !memcmp(px, own, sizeof(own)), "the PNG writer's file was not read back");
        free(px);
        MOD_CHECK(gPortMods[0].textureCount == 1 && gPortMods[1].textureCount == 3 && sTexFiles.size() == 2, "the texture scan found %u and %u", gPortMods[0].textureCount, gPortMods[1].textureCount);
        // By name: noname.png has no hash, so it replaces the texture called "NoName" (any case); the name wins over the hash.
        px = read(0x1234ull, "NoName", &tw, &th);
        MOD_CHECK(px && tw == 3 && th == 2 && !memcmp(px, own, sizeof(own)), "a texture was not found by its name");
        free(px);
        MOD_CHECK(!read(0x1234ull, "Other", &tw, &th) && PortModsTexturesWanted(), "a texture was found by the wrong name");
        px = read(0xABull, "NoName", &tw, &th);
        MOD_CHECK(px && tw == 3, "the name did not win over the hash");
        free(px);
        px = read(0xABull, "Other", &tw, &th);
        MOD_CHECK(px && tw == 5, "the hash is not the fallback for a name no mod has");
        free(px);
        gPortMods[1].enabled = FALSE;
        PortModsRefreshTextures();
        MOD_CHECK(sTexMods.count(0xAB) && sTexMods[0xAB] == "first" && !read(0xCDull, NULL, &tw, &th), "a texture of a mod switched off is used");
        // DDS: an 8x4 BC1 file with two mip levels wins over the PNG of the same hash in its mod; a cut one is refused.
        {
            u8 dds[128 + 16 + 8] = {'D', 'D', 'S', ' ', 124};
            dds[12] = 4; dds[16] = 8; dds[28] = 2; dds[76] = 32; dds[80] = 4;
            memcpy(dds + 84, "DXT1", 4);
            for (int i = 128; i < (int)sizeof(dds); i++) dds[i] = (u8)i;
            static const char* const kDds[] = {"first\\textures\\logo_00000000000000AB.dds", "first\\textures\\cut.dds", "first\\textures\\logo_00000000000000AB.png"};
            for (int i = 0; i < 3; i++) {
                snprintf(path, sizeof(path), "%s\\%s", base, kDds[i]);
                if (i == 2) { PortPngWrite(path, own, 3, 2); continue; }
                FILE* fp = fopen(path, "wb");
                if (fp) { fwrite(dds, 1, i ? sizeof(dds) - 1 : sizeof(dds), fp); fclose(fp); } else fails++;
            }
            PortModsRefreshTextures();
            PortModImage im;
            BOOL ok = PortModsTexture(0xABull, NULL, &im);
            MOD_CHECK(ok && im.dxgi == PORT_DXGI_BC1 && im.w == 8 && im.h == 4 && im.levels == 2 && !memcmp(im.data, dds + 128, 24),
                      "a DDS was not read or did not win over a PNG (%ux%u, format %u)", im.w, im.h, im.dxgi);
            if (ok) free(im.data);
            MOD_CHECK(!PortModsTexture(0x1234ull, "cut", &im), "a DDS that is cut short was accepted");
            MOD_CHECK(gPortMods[0].textureCount == 2, "a PNG and a DDS of one texture were counted as %u", gPortMods[0].textureCount);
            for (const char* f : kDds) { snprintf(path, sizeof(path), "%s\\%s", base, f); DeleteFileA(path); }
            PortModsRefreshTextures();
        }
        const u8 bytes[4] = {1, 2, 3, 4};
        MOD_CHECK(PortModsTextureHash(bytes, 4, NULL, 0, 8, 8, 6) != PortModsTextureHash(bytes, 4, NULL, 0, 8, 4, 6) &&
                  PortModsTextureHash(bytes, 4, bytes, 2, 8, 8, 9) != PortModsTextureHash(bytes, 4, bytes, 3, 8, 8, 9), "the texture hash ignores the size or the palette");
        for (const char* f : kTexFiles) { snprintf(path, sizeof(path), "%s\\%s", base, f); DeleteFileA(path); }
        for (int i = 2; i >= 0; i--) { snprintf(path, sizeof(path), "%s%s", base, kTexDirs[i]); RemoveDirectoryA(path); }
    }
    // Dumping: off by default; switched on in the settings it writes each texture once into the folder.
    {
        const char* keepCli = gPortOptions.dumpTextures;
        PortSettings keepSet = gPortSettings;
        gPortOptions.dumpTextures = NULL;
        gPortSettings.dumpTextures = FALSE;
        MOD_CHECK(!PortModsDumpDir(), "textures are dumped while it is off");
        gPortSettings.dumpTextures = TRUE;
        gPortSettings.dumpDir[0] = 0;
        MOD_CHECK(PortModsDumpDir() && strstr(PortModsDumpDir(), "\\dump\\textures"), "the default dump folder is wrong");
        snprintf(gPortSettings.dumpDir, sizeof(gPortSettings.dumpDir), "%s\\dumps\\sub", base);
        const u32 one[4] = {1, 2, 3, 4};
        PortModsDumpTexture(0x11, NULL, one, 2, 2, 6);
        PortModsDumpTexture(0x11, NULL, one, 2, 2, 6);
        PortModsDumpTexture(0x12, "name", one, 2, 2, 6);
        MOD_CHECK(PortModsDumpCount() == 2 && !PortModsDumpFailed(), "%u textures were dumped, %u failed", PortModsDumpCount(), PortModsDumpFailed());
        sDumpingTo[0] = 0;                                   // as in a later run: the files are there
        PortModsDumpTexture(0x11, NULL, one, 2, 2, 6);
        MOD_CHECK(PortModsDumpCount() == 0 && PortModsDumpPresent() == 1, "a dumped file was written again");
        for (const char* file : {"dumps\\sub\\tex_2x2_f6_0000000000000011.png", "dumps\\sub\\tex_2x2_f6_name_0000000000000012.png"}) {
            snprintf(path, sizeof(path), "%s\\%s", base, file);
            MOD_CHECK(DeleteFileA(path), "%s was not written", file);
        }
        snprintf(path, sizeof(path), "%s\\dumps\\sub", base);
        RemoveDirectoryA(path);
        snprintf(path, sizeof(path), "%s\\dumps", base);
        RemoveDirectoryA(path);
        gPortOptions.dumpTextures = keepCli;
        gPortSettings = keepSet;
        sTexDumped.clear();
        sDumpingTo[0] = 0;
        sDumpWritten = sDumpThere = sDumpFailed = 0;
    }
    // Text table: a JSON file changes strings; a mistake is reported with its place and changes nothing.
    {
        const char* table[4] = {"a", "b", "c", "d"};
        char err[256];
        int n = PortModsApplyStrings("{\"format\": 1, \"strings\": {\"1\": \"B\\u00E9\\n\", \"3\": \"D\\u00E9\"}}", table, 4, err, sizeof(err));
        MOD_CHECK(n == 2 && !strcmp(table[0], "a") && !strcmp(table[1], "B\xE9\n") && !strcmp(table[3], "D\xE9"), "strings.json was not applied (%d, %s)", n, err);
        MOD_CHECK(PortModsApplyStrings("{\"strings\": {\"0\": \"x\",\n \"4\": \"y\"}}", table, 4, err, sizeof(err)) < 0 && strstr(err, "line 2") && strstr(err, "does not exist") && !strcmp(table[0], "a"),
                  "a string number out of range was accepted or not placed (%s)", err);
        MOD_CHECK(PortModsApplyStrings("{\"strings\": {\"0\": \"\\u0416\"}}", table, 4, err, sizeof(err)) < 0 && strstr(err, "U+0416"), "a character outside U+00FF was accepted (%s)", err);
        MOD_CHECK(PortModsApplyStrings("{\"strings\": {\"0\": \"x\"", table, 4, err, sizeof(err)) < 0, "a file that stops early was accepted");
        MOD_CHECK(PortModsApplyStrings("{\"strngs\": {}}", table, 4, err, sizeof(err)) < 0 && strstr(err, "unknown key"), "an unknown key was accepted (%s)", err);
        MOD_CHECK(PortModsApplyStrings("{\"strings\": {\"zero\": \"x\"}}", table, 4, err, sizeof(err)) < 0, "a string number that is no number was accepted");
        MOD_CHECK(!strcmp(table[0], "a") && !strcmp(table[2], "c"), "a broken file changed a string");
    }
#undef MOD_CHECK
    for (const char* f : kFiles) { snprintf(path, sizeof(path), "%s\\%s", base, f); DeleteFileA(path); }
    for (int i = (int)(sizeof(kDirs) / sizeof(kDirs[0])) - 1; i >= 0; i--) { snprintf(path, sizeof(path), "%s%s", base, kDirs[i]); RemoveDirectoryA(path); }
    memcpy(gPortMods, keep, sizeof(keep));
    gPortModCount = keepCount;
    strcpy(sDir, keepDir);
    gPortOptions.noMods = keepOff;
    PortModsRefreshTextures();
    if (!fails) PortLog("selftest mods: ok (file lookup, order, moving, switched off, PNG and DDS textures, dumping, text table)");
    return fails;
}
#endif
