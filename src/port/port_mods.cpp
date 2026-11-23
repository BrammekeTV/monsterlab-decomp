// PC platform layer: mods. A folder mods/<name>/ next to the exe is a mod; what is under its
// files/ folder replaces the disc file of the same path, whether the game runs from a disc image or
// from an extracted folder (DVDOpen asks here first). The list, its order and which mods are on are
// kept in monsterlab.ini ([mods]); a later mod wins over an earlier one. See docs/modding.md.
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <string.h>

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

void PortModsScan(void) {
    for (int i = 0; i < gPortModCount; i++) gPortMods[i].present = FALSE;
    char pattern[MAX_PATH + 8];
    snprintf(pattern, sizeof(pattern), "%s\\*", PortModsDir());
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !GoodName(fd.cFileName)) continue;
        PortMod* m = NULL;
        for (int i = 0; i < gPortModCount && !m; i++)
            if (!_stricmp(gPortMods[i].name, fd.cFileName)) m = &gPortMods[i];
        if (!m) m = PortModsAdd(fd.cFileName, TRUE);       // a folder dropped in is on until switched off
        if (m) m->present = TRUE;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

BOOL PortModsFindFile(const char* discPath, char* host, u32 n, const char** modName) {
    while (*discPath == '/' || *discPath == '\\') discPath++;
    if (!*discPath || strstr(discPath, "..")) return FALSE;
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

// Every file under dir (a mod's files folder), as its disc path.
static u32 LogTree(const PortMod* m, const char* dir, const char* rel) {
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
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { count += LogTree(m, sub, subRel); continue; }
        // Shadowed when a later mod has the file too.
        char host[MAX_PATH];
        const char* winner = NULL;
        PortModsFindFile(subRel, host, sizeof(host), &winner);
        if (winner && _stricmp(winner, m->name)) PortLog("mods: %s has %s, but %s comes later and wins", m->name, subRel, winner);
        else PortLog("mods: %s replaces %s", m->name, subRel);
        count++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return count;
}

void PortModsLogOverrides(void) {
    for (int i = 0; i < gPortModCount; i++) {
        const PortMod* m = &gPortMods[i];
        if (!m->present) { PortLog("mods: %s is listed in monsterlab.ini but not in %s", m->name, PortModsDir()); continue; }
        if (!m->enabled) { PortLog("mods: %s is switched off", m->name); continue; }
        char dir[MAX_PATH];
        snprintf(dir, sizeof(dir), "%s\\%s\\files", PortModsDir(), m->name);
        if (!LogTree(m, dir, "")) PortLog("mods: %s replaces no disc file", m->name);
    }
}

// Two mods made in a temporary folder (no game data): order, switching off, missing files.
int PortSelfTestMods(void) {
    int fails = 0;
    PortMod keep[PORT_MAX_MODS];
    int keepCount = gPortModCount;
    char keepDir[MAX_PATH];
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
    MOD_CHECK(!PortModsAdd("..\\outside", TRUE) && !PortModsAdd("", TRUE), "a name that is no folder name was accepted");
#undef MOD_CHECK
    for (const char* f : kFiles) { snprintf(path, sizeof(path), "%s\\%s", base, f); DeleteFileA(path); }
    for (int i = (int)(sizeof(kDirs) / sizeof(kDirs[0])) - 1; i >= 0; i--) { snprintf(path, sizeof(path), "%s%s", base, kDirs[i]); RemoveDirectoryA(path); }
    memcpy(gPortMods, keep, sizeof(keep));
    gPortModCount = keepCount;
    strcpy(sDir, keepDir);
    if (!fails) PortLog("selftest mods: ok (file lookup, order, switched off)");
    return fails;
}
#endif
