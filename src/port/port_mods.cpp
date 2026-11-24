// PC platform layer: mods. A folder mods/<name>/ next to the exe is a mod; what is under its
// files/ folder replaces the disc file of the same path, whether the game runs from a disc image or
// from an extracted folder (DVDOpen asks here first); a PNG under its textures/ folder replaces a
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
// A PNG under mods/<name>/textures/ (any depth of folders) replaces the game texture whose content
// hash is the last 16 characters of its name: "anything_0123456789ABCDEF.png", the name --dump-textures
// gives it. The hash is over the texture as the game holds it (size, format, pixel data, palette),
// so it is the same on every disc of this game and no game data is needed to tell which is which.
// Made at first use: the mods are scanned before this file's static objects are built (port_runtime.cpp).
struct TexState {
    std::map<u64, std::string> files;            // hash -> PNG, from the last enabled mod that has it
    std::map<u64, std::string> mods;             // hash -> that mod's name
    std::set<u64> dumped, logged;
};
static TexState& Tex(void) { static TexState* t = new TexState; return *t; }
#define sTexFiles  Tex().files
#define sTexMods   Tex().mods
#define sTexDumped Tex().dumped
#define sTexLogged Tex().logged
static BOOL sDumpDirMade;

u64 PortModsTextureHash(const u8* data, u32 bytes, const u8* palette, u32 paletteBytes, u32 w, u32 h, u32 fmt) {
    u64 hash = 14695981039346656037ull;
    const u8 head[5] = {(u8)w, (u8)(w >> 8), (u8)h, (u8)(h >> 8), (u8)fmt};
    for (u8 c : head) hash = (hash ^ c) * 1099511628211ull;
    for (u32 i = 0; i < bytes; i++) hash = (hash ^ data[i]) * 1099511628211ull;
    for (u32 i = 0; i < paletteBytes; i++) hash = (hash ^ palette[i]) * 1099511628211ull;
    return hash;
}

// The hash a texture file's name ends in ("..._0123456789abcdef.png"), FALSE for another name.
static BOOL NameHash(const char* name, u64* hash) {
    size_t n = strlen(name);
    if (n < 20 || _stricmp(name + n - 4, ".png")) return FALSE;
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

// Every PNG under dir with a hash in its name goes into the table (a later mod overwrites an earlier one).
static u32 TexTree(const PortMod* m, const char* dir, BOOL add) {
    char pattern[MAX_PATH + 8];
    if ((u32)snprintf(pattern, sizeof(pattern), "%s\\*", dir) >= sizeof(pattern)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    u32 count = 0;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        char sub[MAX_PATH];
        if ((u32)snprintf(sub, sizeof(sub), "%s\\%s", dir, fd.cFileName) >= sizeof(sub)) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { count += TexTree(m, sub, add); continue; }
        u64 hash;
        if (!NameHash(fd.cFileName, &hash)) {
            size_t n = strlen(fd.cFileName);
            if (add && n > 4 && !_stricmp(fd.cFileName + n - 4, ".png")) PortLog("mods: %s: %s has no texture hash at the end of its name (not used)", m->name, fd.cFileName);
            continue;
        }
        count++;
        if (!add) continue;
        auto old = sTexMods.find(hash);
        if (old != sTexMods.end() && _stricmp(old->second.c_str(), m->name)) PortLog("mods: %s and %s both replace texture %016llX: %s comes later and wins", old->second.c_str(), m->name, hash, m->name);
        sTexFiles[hash] = sub;
        sTexMods[hash] = m->name;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return count;
}

void PortModsRefreshTextures(void) {
    sTexFiles.clear();
    sTexMods.clear();
    sTexLogged.clear();
    for (int i = 0; i < gPortModCount; i++) {
        PortMod* m = &gPortMods[i];
        m->textureCount = 0;
        if (!m->present) continue;
        char dir[MAX_PATH];
        if ((u32)snprintf(dir, sizeof(dir), "%s\\%s\\textures", PortModsDir(), m->name) >= sizeof(dir)) continue;
        m->textureCount = TexTree(m, dir, m->enabled && !gPortOptions.noMods);
    }
}

BOOL PortModsTexturesWanted(void) { return !sTexFiles.empty() || gPortOptions.dumpTextures != NULL; }

u32* PortModsTexture(u64 hash, u32* w, u32* h) {
    auto it = sTexFiles.find(hash);
    if (it == sTexFiles.end()) return NULL;
    const char* why = NULL;
    u32* rgba = PortPngRead(it->second.c_str(), w, h, &why);
    if (sTexLogged.insert(hash).second) {                   // once per texture: the game decodes some again and again
        if (rgba) PortLog("mods: texture %016llX comes from %s (%ux%u, %s)", hash, sTexMods[hash].c_str(), *w, *h, it->second.c_str());
        else fprintf(stderr, "[port] mods: %s %s; the game's texture is used\n", it->second.c_str(), why ? why : "cannot be read");
    }
    return rgba;
}

void PortModsDumpTexture(u64 hash, const u32* rgba, u32 w, u32 h, u32 fmt) {
    const char* dir = gPortOptions.dumpTextures;
    if (!dir || !sTexDumped.insert(hash).second) return;
    if (!sDumpDirMade) { sDumpDirMade = TRUE; CreateDirectoryA(dir, NULL); }
    char path[MAX_PATH + 64];
    snprintf(path, sizeof(path), "%s\\tex_%ux%u_f%u_%016llX.png", dir, w, h, fmt, hash);
    if (!PortPngWrite(path, rgba, w, h) && sTexDumped.size() == 1) fprintf(stderr, "[port] --dump-textures: cannot write %s\n", path);
}

u32 PortModsDumpCount(void) { return (u32)sTexDumped.size(); }

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
        u32* px = PortModsTexture(0xABull, &tw, &th);
        MOD_CHECK(px && tw == 5 && th == 3 && !memcmp(px, kWant, sizeof(kWant)), "a deflated PNG was not read right (%ux%u)", tw, th);
        free(px);
        MOD_CHECK(sTexMods.count(0xAB) && sTexMods[0xAB] == "second", "the later mod's texture did not win");
        px = PortModsTexture(0xCDull, &tw, &th);
        MOD_CHECK(px && tw == 3 && th == 2 && !memcmp(px, own, sizeof(own)), "the PNG writer's file was not read back");
        free(px);
        MOD_CHECK(gPortMods[0].textureCount == 1 && gPortMods[1].textureCount == 2 && sTexFiles.size() == 2, "the texture scan found %u and %u", gPortMods[0].textureCount, gPortMods[1].textureCount);
        gPortMods[1].enabled = FALSE;
        PortModsRefreshTextures();
        MOD_CHECK(sTexMods.count(0xAB) && sTexMods[0xAB] == "first" && !PortModsTexture(0xCDull, &tw, &th), "a texture of a mod switched off is used");
        const u8 bytes[4] = {1, 2, 3, 4};
        MOD_CHECK(PortModsTextureHash(bytes, 4, NULL, 0, 8, 8, 6) != PortModsTextureHash(bytes, 4, NULL, 0, 8, 4, 6) &&
                  PortModsTextureHash(bytes, 4, bytes, 2, 8, 8, 9) != PortModsTextureHash(bytes, 4, bytes, 3, 8, 8, 9), "the texture hash ignores the size or the palette");
        for (const char* f : kTexFiles) { snprintf(path, sizeof(path), "%s\\%s", base, f); DeleteFileA(path); }
        for (int i = 2; i >= 0; i--) { snprintf(path, sizeof(path), "%s%s", base, kTexDirs[i]); RemoveDirectoryA(path); }
    }
#undef MOD_CHECK
    for (const char* f : kFiles) { snprintf(path, sizeof(path), "%s\\%s", base, f); DeleteFileA(path); }
    for (int i = (int)(sizeof(kDirs) / sizeof(kDirs[0])) - 1; i >= 0; i--) { snprintf(path, sizeof(path), "%s%s", base, kDirs[i]); RemoveDirectoryA(path); }
    memcpy(gPortMods, keep, sizeof(keep));
    gPortModCount = keepCount;
    strcpy(sDir, keepDir);
    gPortOptions.noMods = keepOff;
    PortModsRefreshTextures();
    if (!fails) PortLog("selftest mods: ok (file lookup, order, moving, switched off, PNG textures)");
    return fails;
}
#endif
