// PC platform layer: reading the player's own Wii disc image (.iso) directly, so nothing has to be
// extracted. The image's header says which game it is; the game partition's title key is unwrapped
// with the console's common key, its 0x8000-byte clusters (0x400 bytes of hashes, 0x7C00 of data)
// are decrypted as they are read, and files are found through the partition's file table (FST).
// The image can also be a .wbfs (Wii Backup Manager: the disc cut into blocks, only the used ones
// stored; see Wbfs* below). The DVD functions (port_sdk.cpp) and the main.dol loader (port_runtime.cpp) read through this
// when the game data is an image and not an extracted folder.
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"

#pragma comment(lib, "bcrypt.lib")

#define CLUSTER      0x8000u
#define CLUSTER_DATA 0x7C00u
#define WII_MAGIC    0x5D1C9EA3u
#define GAME_ID      "RM7P4F"

static const u8 kCommonKey[16] = {0xEB, 0xE4, 0x2A, 0x22, 0x5E, 0x85, 0x93, 0xE4, 0x48, 0xD9, 0xC5, 0x45, 0x73, 0x81, 0xAA, 0xF7};

enum { KIND_ISO, KIND_WBFS };

struct Cluster { u64 index; u32 used; BOOL valid; u8 data[CLUSTER_DATA]; };
static struct {
    HANDLE file;
    int    kind;                   // KIND_*: how the image stores the disc
    u32    wbfsShift, wbfsCount;   // .wbfs: log2 of the block size, number of blocks of the disc
    u16*   wbfsMap;                // .wbfs: the file block (in blocks) of every disc block, 0 = not stored (reads as zeros)
    BOOL   open, plain;            // plain: an image stored without encryption and hashes
    u64    dataOff;                // the partition's data in the image
    BCRYPT_KEY_HANDLE key;         // the title key
    u8*    fst;
    u32    fstSize, entries;
    u64    dolOff;
    Cluster cache[8];
    u32    tick;
} sDisc;

static inline u32 Be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
static inline void Put32(u8* p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }

static BOOL RawRead(HANDLE h, u64 off, void* dst, u32 len) {
    LARGE_INTEGER pos;
    pos.QuadPart = (LONGLONG)off;
    DWORD got = 0;
    return SetFilePointerEx(h, pos, NULL, FILE_BEGIN) && ReadFile(h, dst, len, &got, NULL) && got == len;
}

// Like RawRead, but a file that ends early is not an error: the rest reads as zeros.
static void RawReadSome(HANDLE h, u64 off, void* dst, u32 len) {
    LARGE_INTEGER pos;
    pos.QuadPart = (LONGLONG)off;
    DWORD got = 0;
    memset(dst, 0, len);
    if (SetFilePointerEx(h, pos, NULL, FILE_BEGIN)) ReadFile(h, dst, len, &got, NULL);
}

// .wbfs: a header sector (magic, sector sizes as powers of two, a table of the discs), then for each
// disc a record (a copy of the first 0x100 bytes of the disc, then one 16-bit big-endian block number
// per block of the disc, 0 = not stored). Block numbers count blocks of the WBFS block size in the file.
// Only the first disc of the file is used; split files (.wbf1) are not.
static BOOL WbfsLoad(HANDLE h, u8* discHead, u32* shift, u32* count, u16** map) {
    u8 b[0x20];
    if (!RawRead(h, 0, b, sizeof(b)) || memcmp(b, "WBFS", 4)) return FALSE;
    u32 hdShift = b[8], blkShift = b[9];
    if (hdShift < 9 || hdShift > 12 || blkShift < 15 || blkShift > 28) return FALSE;
    u32 hdSize = 1u << hdShift, n = ((u32)2 * 143432 * CLUSTER + (1u << blkShift) - 1) >> blkShift;   // up to a dual-layer disc
    u8 table[4096];
    RawReadSome(h, 0, table, hdSize < sizeof(table) ? hdSize : sizeof(table));
    u32 slot = 12;
    while (slot < hdSize && slot < sizeof(table) && !table[slot]) slot++;
    if (slot >= hdSize || slot >= sizeof(table)) return FALSE;                 // no disc in the file
    u64 recSize = ((u64)0x100 + n * 2 + hdSize - 1) & ~(u64)(hdSize - 1);
    u64 rec = hdSize + (u64)(slot - 12) * recSize;
    u16* m = (u16*)malloc(n * 2);
    u8* raw = (u8*)malloc(n * 2);
    if (!m || !raw || !RawRead(h, rec, discHead, 0x100)) { free(m); free(raw); return FALSE; }
    RawReadSome(h, rec + 0x100, raw, n * 2);
    for (u32 i = 0; i < n; i++) m[i] = (u16)(raw[i * 2] << 8 | raw[i * 2 + 1]);
    free(raw);
    *shift = blkShift; *count = n; *map = m;
    return TRUE;
}

// Bytes of the disc as the image holds them (a game partition is still encrypted).
static BOOL DiscRaw(u64 off, void* dstv, u32 len) {
    if (sDisc.kind == KIND_ISO) return RawRead(sDisc.file, off, dstv, len);
    u8* dst = (u8*)dstv;
    u64 size = 1ull << sDisc.wbfsShift;
    while (len) {
        u64 k = off >> sDisc.wbfsShift;
        u32 at = (u32)(off & (size - 1)), n = (u32)(size - at);
        if (n > len) n = len;
        if (k >= sDisc.wbfsCount || !sDisc.wbfsMap[k]) memset(dst, 0, n);
        else if (!RawRead(sDisc.file, ((u64)sDisc.wbfsMap[k] << sDisc.wbfsShift) + at, dst, n)) return FALSE;
        dst += n; off += n; len -= n;
    }
    return TRUE;
}

// AES-128 in CBC mode, through Windows.
static BCRYPT_KEY_HANDLE AesKey(const u8* key) {
    static BCRYPT_ALG_HANDLE alg;
    if (!alg) {
        if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0) < 0) { alg = NULL; return NULL; }
        BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    }
    BCRYPT_KEY_HANDLE h = NULL;
    if (BCryptGenerateSymmetricKey(alg, &h, NULL, 0, (PUCHAR)key, 16, 0) < 0) return NULL;
    return h;
}
static BOOL Aes(BCRYPT_KEY_HANDLE key, BOOL encrypt, const u8* iv, const u8* in, u8* out, u32 len) {
    u8 v[16];
    memcpy(v, iv, 16);
    ULONG got = 0;
    NTSTATUS r = encrypt ? BCryptEncrypt(key, (PUCHAR)in, len, NULL, v, 16, out, len, &got, 0)
                         : BCryptDecrypt(key, (PUCHAR)in, len, NULL, v, 16, out, len, &got, 0);
    return r >= 0 && got == len;
}

void PortDiscClose(void) {
    if (sDisc.file && sDisc.file != INVALID_HANDLE_VALUE) CloseHandle(sDisc.file);
    if (sDisc.key) BCryptDestroyKey(sDisc.key);
    free(sDisc.fst);
    free(sDisc.wbfsMap);
    memset(&sDisc, 0, sizeof(sDisc));
}

BOOL PortDiscIsOpen(void) { return sDisc.open; }
u64  PortDiscDolOffset(void) { return sDisc.dolOff; }

// Bytes of the game partition, decrypted.
BOOL PortDiscRead(u64 off, void* dstv, u32 len) {
    u8* dst = (u8*)dstv;
    if (sDisc.plain) return DiscRaw(sDisc.dataOff + off, dst, len);
    static u8 raw[CLUSTER];
    while (len) {
        u64 index = off / CLUSTER_DATA;
        u32 at = (u32)(off % CLUSTER_DATA), n = CLUSTER_DATA - at;
        if (n > len) n = len;
        Cluster* c = NULL;
        Cluster* oldest = &sDisc.cache[0];                  // an empty slot, or the one used longest ago
        for (Cluster& k : sDisc.cache) {
            if (k.valid && k.index == index) { c = &k; break; }
            if (oldest->valid && (!k.valid || k.used < oldest->used)) oldest = &k;
        }
        if (!c) {
            c = oldest;
            c->valid = FALSE;
            if (!DiscRaw(sDisc.dataOff + index * CLUSTER, raw, CLUSTER)) return FALSE;
            if (!Aes(sDisc.key, FALSE, raw + 0x3D0, raw + 0x400, c->data, CLUSTER_DATA)) return FALSE;
            c->index = index;
            c->valid = TRUE;
        }
        c->used = ++sDisc.tick;
        memcpy(dst, c->data + at, n);
        dst += n; off += n; len -= n;
    }
    return TRUE;
}

// What the image's header says. The game identifier is 6 characters: RM7 is Monster Lab, the
// fourth is the region (P = Europe), the last two the publisher.
int PortDiscCheck(const char* path, char* why, u32 n) {
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { snprintf(why, n, "The file cannot be opened: %s", path); return PORT_DISC_NO_FILE; }
    u8 hdr[0x60];
    BOOL ok = RawRead(h, 0, hdr, sizeof(hdr));
    if (ok && !memcmp(hdr, "WBFS", 4)) {                                   // the disc's own header is copied into the file
        u8 head[0x100]; u32 shift, count; u16* map;
        ok = WbfsLoad(h, head, &shift, &count, &map);
        if (ok) { free(map); memcpy(hdr, head, sizeof(hdr)); }
    }
    CloseHandle(h);
    if (ok && !memcmp(hdr, "RVZ", 4)) { snprintf(why, n, "This is a .rvz image. It is not read yet: convert it to .iso (Dolphin can), or pick an extracted folder."); return PORT_DISC_NOT_WII; }
    if (!ok || Be32(hdr + 0x18) != WII_MAGIC) {
        snprintf(why, n, ok && Be32(hdr + 0x1C) == 0xC2339F3Du ? "This is a GameCube disc image, not Monster Lab for Wii." : "This file is not a Wii disc image (.iso).");
        return PORT_DISC_NOT_WII;
    }
    char id[7], title[0x41];
    memcpy(id, hdr, 6); id[6] = 0;
    memcpy(title, hdr + 0x20, 0x40); title[0x40] = 0;
    for (char* c = id; *c; c++) if (*c < 0x20 || *c > 0x7E) *c = '?';
    for (char* c = title; *c; c++) if (*c < 0x20 || *c > 0x7E) *c = '?';
    if (!memcmp(id, GAME_ID, 6)) { snprintf(why, n, "Monster Lab (%s)", id); return PORT_DISC_OK; }
    if (!memcmp(id, GAME_ID, 3)) {
        snprintf(why, n, "This is Monster Lab for another region (%s). Only the European release (%s) is supported for now.", id, GAME_ID);
        return PORT_DISC_WRONG_REGION;
    }
    snprintf(why, n, "This disc is \"%s\" (%s), not Monster Lab (%s).", title, id, GAME_ID);
    return PORT_DISC_WRONG_GAME;
}

int PortDiscOpen(const char* path, char* why, u32 n) {
    int r = PortDiscCheck(path, why, n);
    if (r != PORT_DISC_OK) return r;
    PortDiscClose();
    sDisc.file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    const char* bad = "the image cannot be read";
    u8 b[0x2C0], tables[0x20];
    u64 part = 0;
    do {
        if (sDisc.file == INVALID_HANDLE_VALUE || !RawRead(sDisc.file, 0, b, 0x100)) break;
        if (!memcmp(b, "WBFS", 4)) {
            sDisc.kind = KIND_WBFS;
            if (!WbfsLoad(sDisc.file, b, &sDisc.wbfsShift, &sDisc.wbfsCount, &sDisc.wbfsMap)) { bad = "its .wbfs structure is damaged"; break; }
        }
        sDisc.plain = b[0x61] != 0;
        // The partition tables: four of (count, offset / 4); the game is the partition of type 0.
        bad = "it has no game partition";
        if (!DiscRaw(0x40000, tables, sizeof(tables))) break;
        for (u32 t = 0; t < 4 && !part; t++) {
            u32 count = Be32(tables + t * 8);
            u64 at = (u64)Be32(tables + t * 8 + 4) << 2;
            for (u32 i = 0; i < count && i < 16 && !part; i++) {
                u8 e[8];
                if (!DiscRaw(at + i * 8, e, 8)) break;
                if (Be32(e + 4) == 0) part = (u64)Be32(e) << 2;
            }
        }
        if (!part || !DiscRaw(part, b, sizeof(b))) break;
        sDisc.dataOff = part + ((u64)Be32(b + 0x2B8) << 2);
        if (!sDisc.plain) {
            // The ticket: the title key (0x1BF) is wrapped with the common key, the title identifier (0x1DC) is the IV.
            bad = "its title key cannot be unwrapped";
            if (b[0x1F1] != 0) { bad = "it uses another common key (a Korean disc?)"; break; }
            u8 iv[16] = {}, titleKey[16];
            memcpy(iv, b + 0x1DC, 8);
            BCRYPT_KEY_HANDLE common = AesKey(kCommonKey);
            BOOL ok = common && Aes(common, FALSE, iv, b + 0x1BF, titleKey, 16);
            if (common) BCryptDestroyKey(common);
            if (!ok || !(sDisc.key = AesKey(titleKey))) break;
        }
        // The partition starts with the disc header again; then the executable and the file table.
        bad = "its game partition does not decrypt to Monster Lab";
        u8 boot[0x440];
        if (!PortDiscRead(0, boot, sizeof(boot)) || memcmp(boot, GAME_ID, 6)) break;
        sDisc.dolOff = (u64)Be32(boot + 0x420) << 2;
        u64 fstOff = (u64)Be32(boot + 0x424) << 2;
        sDisc.fstSize = Be32(boot + 0x428) << 2;
        bad = "its file table is damaged";
        if (sDisc.fstSize < 12 || sDisc.fstSize > 0x1000000) break;
        sDisc.fst = (u8*)malloc(sDisc.fstSize);
        if (!sDisc.fst || !PortDiscRead(fstOff, sDisc.fst, sDisc.fstSize)) break;
        sDisc.entries = Be32(sDisc.fst + 8);
        if (sDisc.entries == 0 || (u64)sDisc.entries * 12 > sDisc.fstSize) break;
        sDisc.open = TRUE;
        return PORT_DISC_OK;
    } while (0);
    PortDiscClose();
    snprintf(why, n, "This looks like Monster Lab, but %s.", bad);
    return PORT_DISC_UNREADABLE;
}

// The file table: entry 0 is the root; a directory's last word is the index after its contents,
// a file's words are its offset / 4 and its size. Names are compared without case.
BOOL PortDiscFind(const char* path, u64* offset, u32* size) {
    if (!sDisc.open) return FALSE;
    const u8* names = sDisc.fst + sDisc.entries * 12;
    u32 namesSize = sDisc.fstSize - sDisc.entries * 12;
    u32 i = 1, end = sDisc.entries;
    while (*path == '/' || *path == '\\') path++;
    for (;;) {
        size_t len = strcspn(path, "/\\");
        BOOL last = path[len] == 0;
        BOOL found = FALSE;
        while (i < end) {
            const u8* e = sDisc.fst + i * 12;
            u32 nameOff = Be32(e) & 0xFFFFFF, next = e[0] ? Be32(e + 8) : i + 1;
            const char* name = nameOff < namesSize ? (const char*)names + nameOff : "";
            if (e[0] && (next <= i || next > sDisc.entries)) return FALSE;
            if (strlen(name) == len && !_strnicmp(name, path, len)) {
                if (last) {
                    if (e[0]) return FALSE;
                    *offset = (u64)Be32(e + 4) << 2;
                    *size = Be32(e + 8);
                    return TRUE;
                }
                if (!e[0]) return FALSE;
                i++; end = next;
                found = TRUE;
                break;
            }
            i = next;
        }
        if (!found) return FALSE;
        path += len;
        while (*path == '/' || *path == '\\') path++;
    }
}

// A small image made here (no game data): a header, one encrypted game partition of two clusters
// with a file table and one file that crosses the cluster boundary.
int PortSelfTestDisc(void) {
    enum { PART = 0x50000, DATA = 0x20000, SIZE = PART + DATA + 2 * CLUSTER, FST = 0x1000, FILE_AT = 0x7BF0, FILE_SIZE = 0x40 };
    int fails = 0;
    u8* img = (u8*)calloc(1, SIZE);
    u8* plain = (u8*)calloc(1, 2 * CLUSTER_DATA);
    if (!img || !plain) return 1;
    memcpy(img, GAME_ID, 6); Put32(img + 0x18, WII_MAGIC); strcpy((char*)img + 0x20, "Self-test disc");
    Put32(img + 0x40000, 1); Put32(img + 0x40004, 0x40020 >> 2);
    Put32(img + 0x40020, PART >> 2); Put32(img + 0x40024, 0);
    u8 titleKey[16], iv[16] = {};
    for (int i = 0; i < 16; i++) titleKey[i] = (u8)(0x11 * i + 7);
    memcpy(img + PART + 0x1DC, "\0\1\0\0TEST", 8);
    memcpy(iv, img + PART + 0x1DC, 8);
    BCRYPT_KEY_HANDLE common = AesKey(kCommonKey), title = AesKey(titleKey);
    if (!common || !title || !Aes(common, TRUE, iv, titleKey, img + PART + 0x1BF, 16)) { PortLog("selftest disc: AES is not available"); free(img); free(plain); return 1; }
    Put32(img + PART + 0x2B8, DATA >> 2);
    // The partition's contents.
    memcpy(plain, GAME_ID, 6);
    Put32(plain + 0x420, 0x800 >> 2); Put32(plain + 0x424, FST >> 2); Put32(plain + 0x428, (3 * 12 + 14 + 3) >> 2);
    u8* fst = plain + FST;
    fst[0] = 1; Put32(fst + 8, 3);                                      // root: 3 entries
    fst[12] = 1; Put32(fst + 20, 3);                                    // "data", a directory to the end
    fst[24 + 3] = 5; Put32(fst + 28, FILE_AT >> 2); Put32(fst + 32, FILE_SIZE);
    memcpy(fst + 36, "data\0test.bin", 14);
    for (int i = 0; i < FILE_SIZE; i++) plain[FILE_AT + i] = (u8)(i * 3 + 1);
    for (u32 c = 0; c < 2; c++) {
        u8* raw = img + PART + DATA + c * CLUSTER;
        for (int i = 0; i < 16; i++) raw[0x3D0 + i] = (u8)(c * 16 + i + 1);
        Aes(title, TRUE, raw + 0x3D0, plain + c * CLUSTER_DATA, raw + 0x400, CLUSTER_DATA);
    }
    BCryptDestroyKey(common); BCryptDestroyKey(title);

    char path[MAX_PATH + 48], why[256];
    DWORD n = GetTempPathA(MAX_PATH, path);
    snprintf(path + n, sizeof(path) - n, "monsterlab_selftest_%lu.iso", GetCurrentProcessId());
    auto write = [&]() { FILE* f = fopen(path, "wb"); if (!f) return FALSE; BOOL ok = fwrite(img, 1, SIZE, f) == SIZE; fclose(f); return ok; };
#define DISC_CHECK(cond, ...) do { if (!(cond)) { PortLog("selftest disc: " __VA_ARGS__); fails++; } } while (0)
    if (!write()) { PortLog("selftest disc: cannot write %s", path); free(img); free(plain); return 1; }
    int r = PortDiscOpen(path, why, sizeof(why));
    DISC_CHECK(r == PORT_DISC_OK, "the image did not open: %s", why);
    if (r == PORT_DISC_OK) {
        u64 off = 0; u32 size = 0;
        u8 got[FILE_SIZE];
        DISC_CHECK(PortDiscFind("/DATA/Test.bin", &off, &size) && off == FILE_AT && size == FILE_SIZE, "data/test.bin: offset %llX size %X", off, size);
        DISC_CHECK(PortDiscRead(FILE_AT, got, FILE_SIZE) && !memcmp(got, plain + FILE_AT, FILE_SIZE), "the file read across two clusters is wrong");
        DISC_CHECK(!PortDiscFind("data/none.bin", &off, &size) && !PortDiscFind("data", &off, &size) && !PortDiscFind("test.bin", &off, &size), "a file that is not there was found");
        DISC_CHECK(PortDiscDolOffset() == 0x800, "executable offset %llX", PortDiscDolOffset());
    }
    PortDiscClose();
    {   // The same disc as a .wbfs: 128 KiB blocks stored in reverse order after the records, 512-byte sectors.
        enum { SH = 17, BLK = 1 << SH, N = SIZE / BLK, ENTRIES = (2 * 143432 * CLUSTER + BLK - 1) >> SH, REC = (0x100 + ENTRIES * 2 + 511) & ~511, FIRST = (512 + REC + BLK - 1) / BLK };
        u32 wsize = (FIRST + N) * BLK;
        u8* w = (u8*)calloc(1, wsize);
        if (!w) { fails++; } else {
            memcpy(w, "WBFS", 4); w[8] = 9; w[9] = SH; w[12] = 1;
            memcpy(w + 512, img, 0x100);
            for (int k = 0; k < N; k++) {
                u32 blk = FIRST + (N - 1 - k);
                w[512 + 0x100 + k * 2] = (u8)(blk >> 8); w[512 + 0x100 + k * 2 + 1] = (u8)blk;
                memcpy(w + (u64)blk * BLK, img + (u64)k * BLK, BLK);
            }
            char wpath[sizeof(path) + 8];
            snprintf(wpath, sizeof(wpath), "%s.wbfs", path);
            FILE* f = fopen(wpath, "wb");
            BOOL wrote = f && fwrite(w, 1, wsize, f) == wsize;
            if (f) fclose(f);
            DISC_CHECK(wrote, "cannot write %s", wpath);
            if (wrote) {
                r = PortDiscOpen(wpath, why, sizeof(why));
                DISC_CHECK(r == PORT_DISC_OK, "the .wbfs image did not open: %s", why);
                if (r == PORT_DISC_OK) {
                    u64 off = 0; u32 size = 0;
                    u8 got[FILE_SIZE];
                    DISC_CHECK(PortDiscFind("/DATA/Test.bin", &off, &size) && off == FILE_AT && size == FILE_SIZE, ".wbfs data/test.bin: offset %llX size %X", off, size);
                    DISC_CHECK(PortDiscRead(FILE_AT, got, FILE_SIZE) && !memcmp(got, plain + FILE_AT, FILE_SIZE), ".wbfs: the file read across two clusters is wrong");
                }
                PortDiscClose();
                w[512 + 0x100 + 2 * 2] = w[512 + 0x100 + 2 * 2 + 1] = 0;      // block 2 (the partition table) is not stored
                f = fopen(wpath, "wb"); if (f) { fwrite(w, 1, wsize, f); fclose(f); }
                DISC_CHECK(PortDiscOpen(wpath, why, sizeof(why)) == PORT_DISC_UNREADABLE, "a .wbfs without the partition table was not refused: %s", why);
            }
            DeleteFileA(wpath);
            free(w);
        }
    }
    PortDiscClose();
    img[3] = 'E'; write();
    DISC_CHECK(PortDiscOpen(path, why, sizeof(why)) == PORT_DISC_WRONG_REGION, "another region was not refused: %s", why);
    memcpy(img, "RSPE01", 6); write();
    DISC_CHECK(PortDiscOpen(path, why, sizeof(why)) == PORT_DISC_WRONG_GAME, "another game was not refused: %s", why);
    Put32(img + 0x18, 0); write();
    DISC_CHECK(PortDiscOpen(path, why, sizeof(why)) == PORT_DISC_NOT_WII, "a file that is no Wii disc was not refused: %s", why);
    memcpy(img, GAME_ID, 6); Put32(img + 0x18, WII_MAGIC); img[PART + 0x1BF] ^= 0x55; write();
    DISC_CHECK(PortDiscOpen(path, why, sizeof(why)) == PORT_DISC_UNREADABLE, "a wrong title key was not noticed: %s", why);
#undef DISC_CHECK
    PortDiscClose();
    DeleteFileA(path);
    free(img); free(plain);
    if (!fails) PortLog("selftest disc: ok (header check, title key, clusters, file table, .wbfs blocks)");
    return fails;
}
#endif
