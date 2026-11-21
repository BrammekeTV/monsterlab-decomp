// PC platform layer: what happens to a disc file's bytes after they are read. The .sir files are
// big-endian; the game code reads them as host data, so each section is converted in place when
// its read completes (docs/loader.md, "Where it plugs into the game"). No original code.
#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"
#include "port/sir_loader.h"

// A copy of the catalog's two tables (the game's buffer is reused for the next file).
static SirCatalogEntry* sEntries;
static u32              sEntryCount;
static SirCatalogFile*  sFiles;
static u32              sFileCount;

static u8* sGlobalHdr;                 // header section of global_binary.sir, once read
static u32 sConverted, sUnconverted;

static const char* BaseName(const char* path) {
    const char* b = path;
    for (const char* c = path; *c; c++) if (*c == '/' || *c == '\\') b = c + 1;
    return b;
}

static const SirCatalogFile* FindFile(const char* name) {
    for (u32 i = 0; i < sFileCount; i++) if (_stricmp(sFiles[i].name, name) == 0) return &sFiles[i];
    return NULL;
}

// The section of `file` a read at `offset` starts.
static const SirCatalogEntry* FindSection(const SirCatalogFile* file, u32 offset) {
    for (u32 i = 0; i < sEntryCount; i++) {
        const SirCatalogEntry* e = &sEntries[i];
        if (e->nameHash == file->nameHash && e->offset == offset) return e;
    }
    return NULL;
}

static void Unconverted(const char* name, const SirCatalogEntry* e, const char* why) {
    static char seen[64][80];
    char key[80];
    snprintf(key, sizeof(key), "%s type %d", name, e ? e->type : -99);
    for (s32 i = 0; i < 64; i++) {
        if (strcmp(seen[i], key) == 0) return;
        if (!seen[i][0]) { strcpy(seen[i], key); break; }
    }
    sUnconverted++;
    if (e) PortLog("disc: %s section type %d (%u bytes, %u packed) is not converted: %s", name, e->type, e->size, e->packedSize, why);
    else   PortLog("disc: %s: %s", name, why);
}

void PortFileRead(const char* path, u32 offset, u8* dst, u32 len) {
    const char* name = BaseName(path);
    const char* ext = strrchr(name, '.');
    if (!ext || _stricmp(ext, ".sir") != 0) return;

    if (_stricmp(name, "catalog.sir") == 0) {
        if (offset != 0) { Unconverted(name, NULL, "read in pieces, not converted"); return; }
        SirCatalog cat;
        SirError err = SirCatalogConvert(dst, len, &cat);
        if (err != SIR_OK) PortStop(8, "catalog.sir: %s", SirErrorText(err));
        free(sEntries); free(sFiles);
        sEntries = (SirCatalogEntry*)malloc(sizeof(SirCatalogEntry) * (cat.entryCount + 1));
        sFiles = (SirCatalogFile*)malloc(sizeof(SirCatalogFile) * (cat.fileCount + 1));
        memcpy(sEntries, cat.entries, sizeof(SirCatalogEntry) * cat.entryCount);
        memcpy(sFiles, cat.files, sizeof(SirCatalogFile) * cat.fileCount);
        sEntryCount = cat.entryCount;
        sFileCount = cat.fileCount;
        sConverted++;
        static char line[96];   // the milestone list keeps the pointer
        snprintf(line, sizeof(line), "catalog.sir converted: %u sections in %u files", sEntryCount, sFileCount);
        PortMilestone(line);
        return;
    }

    const SirCatalogFile* file = FindFile(name);
    if (!file) { Unconverted(name, NULL, "not in the catalog, left big-endian"); return; }
    const SirCatalogEntry* e = FindSection(file, offset);
    if (!e) { Unconverted(name, NULL, "a read that starts no section, left as it is"); return; }
    if (e->packedSize != 0) { Unconverted(name, e, "packed"); return; }
    if (len < e->size) { Unconverted(name, e, "read in pieces"); return; }

    if (_stricmp(name, "global_binary.sir") == 0) {
        if (e->type == -1) { sGlobalHdr = dst; return; }      // converted with the image
        if (e->type == 0 && sGlobalHdr) {
            static SirReloc reloc;
            s32 failTable = -1;
            SirRelocInit(&reloc, NULL, NULL);
            SirError err = GlobalBinaryConvert(&reloc, sGlobalHdr, dst, e->size, &failTable);
            if (err != SIR_OK) PortStop(8, "global_binary.sir: %s (table %d)", SirErrorText(err), failTable);
            sConverted++;
            static char line[128];   // the milestone list keeps the pointer
            snprintf(line, sizeof(line), "global_binary.sir converted: %u pointers, %u fields", reloc.ptrImage, reloc.fieldsSwapped);
            PortMilestone(line);
            return;
        }
    }
    Unconverted(name, e, "no converter for this section type yet");
}

const char* PortFileStats(char* buf, u32 n) {
    snprintf(buf, n, "%u section(s) converted, %u kind(s) of section left big-endian", sConverted, sUnconverted);
    return buf;
}
#endif
