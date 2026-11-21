// PC platform layer: NAND, the console's internal flash, where the game keeps its save files.
// On PC it is a folder: `save` beside the executable. No original code.
//
// The functions carry the names the game calls them by; the comment says which SDK function
// each one is (identified from the SDK's code at that address and from how the game uses it).
// Results are the SDK's: 0 ok, -6 exists already, -12 does not exist, -8 invalid, -1 other.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"

enum { NAND_OK = 0, NAND_EXISTS = -6, NAND_INVALID = -8, NAND_NOEXISTS = -12, NAND_UNKNOWN = -64 };

static char sRoot[MAX_PATH];           // host folder that stands for the title's data directory
static char sCwd[128] = "/";           // current directory inside it, with a trailing '/'
static const char kHome[] = "/title/00010000/524d3750/data";
static FILE* sOpen[16];

static void EnsureRoot(void) {
    if (sRoot[0]) return;
    GetModuleFileNameA(NULL, sRoot, MAX_PATH);
    char* slash = strrchr(sRoot, '\\');
    if (slash) *slash = 0;
    strncat(sRoot, "\\save", MAX_PATH - strlen(sRoot) - 1);
    _mkdir(sRoot);
    PortLog("nand: save files are in %s", sRoot);
}

// A game path (relative to the current directory, or absolute inside the home directory) as a
// host path.
static BOOL HostPath(const char* path, char* out, size_t n) {
    EnsureRoot();
    if (!path || strstr(path, "..")) return FALSE;
    char full[256];
    if (strncmp(path, kHome, sizeof(kHome) - 1) == 0) path += sizeof(kHome) - 1;
    if (path[0] == '/') snprintf(full, sizeof(full), "%s", path);
    else snprintf(full, sizeof(full), "%s%s", sCwd, path);
    snprintf(out, n, "%s%s", sRoot, full);
    for (char* c = out + strlen(sRoot); *c; c++) if (*c == '/') *c = '\\';
    size_t len = strlen(out);
    if (len > strlen(sRoot) + 1 && out[len - 1] == '\\') out[len - 1] = 0;
    return TRUE;
}

static DWORD Attr(const char* host) { return GetFileAttributesA(host); }

extern "C" {

// NANDGetHomeDir
s32 fn_80267B14(char* out) { strcpy(out, kHome); return NAND_OK; }

// NANDChangeDir
s32 fn_80267A54(const char* path) {
    char host[MAX_PATH];
    if (!HostPath(path, host, sizeof(host))) return NAND_INVALID;
    DWORD a = Attr(host);
    if (a == INVALID_FILE_ATTRIBUTES || !(a & FILE_ATTRIBUTE_DIRECTORY)) return NAND_NOEXISTS;
    if (strncmp(path, kHome, sizeof(kHome) - 1) == 0) path += sizeof(kHome) - 1;
    char next[128];
    if (path[0] == '/' || path[0] == 0) snprintf(next, sizeof(next), "%s", path[0] ? path : "/");
    else snprintf(next, sizeof(next), "%s%s", sCwd, path);
    size_t len = strlen(next);
    if (len == 0 || next[len - 1] != '/') strncat(next, "/", sizeof(next) - len - 1);
    strcpy(sCwd, next);
    return NAND_OK;
}

// NANDCreateDir(path, permissions, attributes)
s32 fn_8026631C(const char* path, s32 perm, s32 attr) {
    (void)perm; (void)attr;
    char host[MAX_PATH];
    if (!HostPath(path, host, sizeof(host))) return NAND_INVALID;
    if (Attr(host) != INVALID_FILE_ATTRIBUTES) return NAND_EXISTS;
    return _mkdir(host) == 0 ? NAND_OK : NAND_UNKNOWN;
}

// NANDCreate(path, permissions, attributes): an empty file
s32 fn_80265C18(const char* path, s32 perm, s32 attr) {
    (void)perm; (void)attr;
    char host[MAX_PATH];
    if (!HostPath(path, host, sizeof(host))) return NAND_INVALID;
    if (Attr(host) != INVALID_FILE_ATTRIBUTES) return NAND_EXISTS;
    FILE* f = fopen(host, "wb");
    if (!f) return NAND_UNKNOWN;
    fclose(f);
    return NAND_OK;
}

// NANDDelete
s32 fn_80265D78(const char* path) {
    char host[MAX_PATH];
    if (!HostPath(path, host, sizeof(host))) return NAND_INVALID;
    DWORD a = Attr(host);
    if (a == INVALID_FILE_ATTRIBUTES) return NAND_NOEXISTS;
    BOOL ok = (a & FILE_ATTRIBUTE_DIRECTORY) ? RemoveDirectoryA(host) : DeleteFileA(host);
    return ok ? NAND_OK : NAND_UNKNOWN;
}

// NANDOpen(path, file info, access: 1 read, 2 write, 3 both). The file must exist. The info block
// is the SDK's to use; its first word holds the slot + 1 here.
s32 fn_80266BA4(const char* path, u8* info, s32 access) {
    char host[MAX_PATH];
    if (!HostPath(path, host, sizeof(host))) return NAND_INVALID;
    DWORD a = Attr(host);
    if (a == INVALID_FILE_ATTRIBUTES || (a & FILE_ATTRIBUTE_DIRECTORY)) return NAND_NOEXISTS;
    for (s32 i = 0; i < 16; i++) {
        if (sOpen[i]) continue;
        sOpen[i] = fopen(host, access == 1 ? "rb" : "r+b");
        if (!sOpen[i]) return NAND_UNKNOWN;
        *(s32*)info = i + 1;
        return NAND_OK;
    }
    return NAND_UNKNOWN;
}

static FILE* FileOf(u8* info) {
    s32 slot = *(s32*)info;
    return slot >= 1 && slot <= 16 ? sOpen[slot - 1] : NULL;
}

// NANDClose
s32 fn_80266E24(u8* info) {
    FILE* f = FileOf(info);
    if (!f) return NAND_INVALID;
    fclose(f);
    sOpen[*(s32*)info - 1] = NULL;
    *(s32*)info = 0;
    return NAND_OK;
}

// NANDRead: the number of bytes read
s32 fn_80265F80(u8* info, void* buf, u32 n) {
    FILE* f = FileOf(info);
    return f ? (s32)fread(buf, 1, n, f) : NAND_INVALID;
}

// NANDWrite: the number of bytes written
s32 fn_80266060(u8* info, void* buf, u32 n) {
    FILE* f = FileOf(info);
    if (!f) return NAND_INVALID;
    s32 done = (s32)fwrite(buf, 1, n, f);
    fflush(f);
    return done;
}

// NANDGetLength
s32 fn_80266614(u8* info, u32* out) {
    FILE* f = FileOf(info);
    if (!f) return NAND_INVALID;
    long at = ftell(f);
    fseek(f, 0, SEEK_END);
    *out = (u32)ftell(f);
    fseek(f, at, SEEK_SET);
    return NAND_OK;
}

// NANDGetType(path, type): 1 file, 2 directory
s32 fn_80266944(u8* path, s32* out) {
    char host[MAX_PATH];
    if (!HostPath((const char*)path, host, sizeof(host))) return NAND_INVALID;
    DWORD a = Attr(host);
    if (a == INVALID_FILE_ATTRIBUTES) return NAND_NOEXISTS;
    *(u8*)out = (a & FILE_ATTRIBUTE_DIRECTORY) ? 2 : 1;
    return NAND_OK;
}

// NANDMove(path, destination directory)
s32 fn_802665B0(const char* path, const char* destDir) {
    char from[MAX_PATH], to[MAX_PATH];
    if (!HostPath(path, from, sizeof(from)) || !HostPath(destDir, to, sizeof(to))) return NAND_INVALID;
    if (Attr(from) == INVALID_FILE_ATTRIBUTES) return NAND_NOEXISTS;
    const char* name = strrchr(from, '\\');
    strncat(to, name ? name : "", sizeof(to) - strlen(to) - 1);
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) ? NAND_OK : NAND_UNKNOWN;
}

// NANDCheck(blocks needed, inodes needed, answer): answer 0 = there is room
s32 NANDCheck(u32 blocks, u32 inodes, u32* answer) {
    (void)blocks; (void)inodes;
    *answer = 0;
    return NAND_OK;
}

// NANDFreeBlocks(free bytes, free inodes)
s32 fn_8026829C(u32* freeBytes, u32* freeInodes) {
    *freeBytes = 0x10000000;
    *freeInodes = 4000;
    return NAND_OK;
}

// NANDInitBanner(banner, flags, title, comment): the header of the save's banner file; the title
// and comment are 16-bit text of at most 32 characters each.
s32 fn_80267F44(u8* banner, s32 flags, s16* title, s16* comment) {
    memset(banner, 0, 0xA0);
    banner[0] = 'W'; banner[1] = 'I'; banner[2] = 'B'; banner[3] = 'N';
    banner[4] = (u8)(flags >> 24); banner[5] = (u8)(flags >> 16); banner[6] = (u8)(flags >> 8); banner[7] = (u8)flags;
    for (int i = 0; i < 32 && title && title[i]; i++) memcpy(banner + 0x20 + i * 2, &title[i], 2);
    for (int i = 0; i < 32 && comment && comment[i]; i++) memcpy(banner + 0x60 + i * 2, &comment[i], 2);
    return NAND_OK;
}

}  // extern "C"
#endif
