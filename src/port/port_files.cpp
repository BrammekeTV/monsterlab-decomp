// PC platform layer: what happens to a disc file's bytes after they are read. The .sir files are
// big-endian; the game code reads them as host data, so each section is converted in place when
// its read completes (docs/loader.md, "Where it plugs into the game"). No original code.
#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

static bool HasSection(const SirCatalogFile* file, s32 type) {
    for (u32 i = 0; i < sEntryCount; i++)
        if (sEntries[i].nameHash == file->nameHash && sEntries[i].type == type) return true;
    return false;
}

// A level file has a header block (the world state) and the images of zone 0 and zone 1: the
// seven *_level.sir, the arenas, the castle rooms and the trouble scope map. (The castle rooms'
// second files, *c.sir, are packed copies: their sections are reported as packed.)
static bool IsLevelFile(const SirCatalogFile* file) {
    return _stricmp(file->name, "global_binary.sir") != 0 && HasSection(file, -1) && HasSection(file, 0) && HasSection(file, 1);
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

// The script function table, to check the indices the level loader relies on (LEVEL_FIXUP_*).
struct PortScriptFunc { u32 type; void* fn; u32 hash; };
extern "C" { extern PortScriptFunc gScriptFuncs[]; u32 StringHash(const char* s); }

// Where the game loads a level's sections to: the header block to the world block (fn_801294CC:
// SirLoad(..., *world, size, -1)), zone N to gZoneBase[N]. A section is not always read there
// directly: the level the front end loads is read whole to a buffer first, and each section is
// copied from there by a thread when the game asks for it (fn_801E6FAC mode 1, fn_801E6F78). Its
// pointers are meant for the final place either way.
extern "C" { extern u8 gWorldState[]; }
extern u32* gZoneBase;
static u8* LevelFinalPlace(s32 type, u8* readTo) {
    u8* place = NULL;
    if (type == -1) memcpy(&place, gWorldState, sizeof(place));
    else if (gZoneBase != NULL && type >= 0 && type < 512) place = (u8*)(uintptr_t)gZoneBase[type];
    return place != NULL ? place : readTo;
}

static void CheckFixUpIndices(const char* file) {
    static const char* const names[] = { "FixUpEntityCallback", "FixUpEntityPartCallback", "FixUpPooledEntityCallback",
                                         "FixUpProjectileEntityCallback", "FixUpRenderObjectCallback", "FixupFSMPoolCallback" };
    for (u32 i = 0; i < 6; i++)
        if (gScriptFuncs[LEVEL_FIXUP_ENTITY + i].hash != StringHash(names[i]))
            PortStop(8, "%s: script function %u is not %s: the level loader's pool item kinds would be wrong", file, LEVEL_FIXUP_ENTITY + i, names[i]);
}

static void ConvertSection(const char* name, const SirCatalogFile* file, const SirCatalogEntry* e, u8* dst);

// ---- packed sections: the read brings the compressed bytes, the game's zlib calls (port_zlib.cpp)
// bring the section, so the conversion waits for the end of the stream.
struct PackedRead { u8* dst; const SirCatalogFile* file; const SirCatalogEntry* e; char name[64]; };
struct Inflating { void* strm; u8* out; PackedRead r; };
static PackedRead sPacked[8];
static u32        sPackedNext;
static Inflating  sInflating[8];

static void NotePackedRead(const char* name, const SirCatalogFile* file, const SirCatalogEntry* e, u8* dst) {
    PackedRead* p = &sPacked[sPackedNext++ & 7];
    p->dst = dst; p->file = file; p->e = e;
    snprintf(p->name, sizeof(p->name), "%s", name);
}
void PortFileInflateDrop(void* strm) {
    for (u32 i = 0; i < 8; i++) if (sInflating[i].strm == strm) sInflating[i].strm = NULL;
}
void PortFileInflateBegin(void* strm, const u8* nextIn, u8* nextOut) {
    PortFileInflateDrop(strm);
    for (u32 i = 0; i < 8; i++) {
        PackedRead* p = &sPacked[i];
        if (!p->e || p->dst != nextIn) continue;
        for (u32 k = 0; k < 8; k++) {
            if (sInflating[k].strm) continue;
            sInflating[k].strm = strm; sInflating[k].out = nextOut; sInflating[k].r = *p;
            p->e = NULL;
            return;
        }
    }
}
void PortFileInflateEnd(void* strm, u32 totalOut) {
    for (u32 i = 0; i < 8; i++) {
        Inflating* f = &sInflating[i];
        if (f->strm != strm) continue;
        f->strm = NULL;
        if (totalOut != f->r.e->size) { Unconverted(f->r.name, f->r.e, "inflated to another size than the catalog says"); return; }
        ConvertSection(f->r.name, f->r.file, f->r.e, f->out);
        return;
    }
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
    if (e->packedSize != 0) { NotePackedRead(name, file, e, dst); return; }
    if (len < e->size) { Unconverted(name, e, "read in pieces"); return; }
    ConvertSection(name, file, e, dst);
}

// Converts one whole section, as it lies at `dst` (read from disc, or inflated from a packed one).
// Where section images lie in memory (where they were read to and where the game uses them).
// What the loader does not convert in them stays in console byte order: the renderer asks here
// whether a vertex array is such data (a mesh's arrays) or one the game built at run time.
struct SectionRange { u8* lo; u8* hi; };
static SectionRange sSectionRanges[128];
static u32          sSectionRangeNext;
static void NoteSectionRange(u8* lo, u32 size) {
    if (lo == NULL || size == 0) return;
    for (SectionRange& r : sSectionRanges) if (r.lo == lo) { r.hi = lo + size; return; }
    SectionRange& r = sSectionRanges[sSectionRangeNext++ % 128];
    r.lo = lo; r.hi = lo + size;
}
BOOL PortIsSectionData(const void* p) {
    for (const SectionRange& r : sSectionRanges) if ((const u8*)p >= r.lo && (const u8*)p < r.hi) return TRUE;
    return FALSE;
}

static void ConvertSection(const char* name, const SirCatalogFile* file, const SirCatalogEntry* e, u8* dst) {
    if (e->type >= 0) {
        NoteSectionRange(dst, e->size);
        NoteSectionRange(LevelFinalPlace(e->type, dst), e->size);
    }

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
    // Level files: the header block is the world state, section type N the image of zone N.
    size_t n = strlen(name);
    static SirReloc reloc;
    static u32 hudAddr;            // header +0x1F4, kept for the zone that holds the HUD
    static u8* levelHdr;           // the header block (the world state), converted
    static u32 levelHdrSize;
    static u8* levelHdrCopy;       // a level read to a buffer first: the header with its pointers into this copy
    static u8* zone0Read;          // where zone 0's bytes were read to (the buffer, or zone0 itself)
    static u8* zone0;              // zone 0's image, converted, and what was converted in it
    static u32 zone0Size;
    static LevelZoneDone zone0Done;
    if (IsLevelFile(file)) {
        SirRelocInit(&reloc, NULL, NULL);
        if (e->type == -1) {
            u32 base = LevelHeaderWiiBase(dst, e->size);
            if (base == 0) PortStop(8, "%s: %s", name, SirErrorText(SIR_ERR_LEVEL));
            // The header points into itself and into the zones. The zones are read later, to
            // the addresses the console uses (MEM2 is mapped at its original address).
            u8* place = LevelFinalPlace(-1, dst);
            free(levelHdrCopy);
            levelHdrCopy = NULL;
            if (place != dst) {
                // The zones are converted before the game has copied this header to the world
                // block, so what they need of it is read from a copy whose pointers lead into itself.
                static SirReloc own;
                levelHdrCopy = (u8*)malloc(e->size);
                if (!levelHdrCopy) PortStop(8, "%s: out of memory", name);
                memcpy(levelHdrCopy, dst, e->size);
                SirRelocInit(&own, NULL, NULL);
                SirRelocAddRange(&own, base, e->size, levelHdrCopy);
                SirRelocAddRange(&own, 0x90000000u, 0x04000000u, (u8*)(uintptr_t)0x90000000u);
                SirError copyErr = LevelHeaderConvert(&own, levelHdrCopy, e->size);
                if (copyErr != SIR_OK) PortStop(8, "%s: %s", name, SirErrorText(copyErr));
            }
            SirRelocAddRange(&reloc, base, e->size, place);
            SirRelocAddRange(&reloc, 0x90000000u, 0x04000000u, (u8*)(uintptr_t)0x90000000u);
            SirError err = LevelHeaderConvert(&reloc, dst, e->size);
            if (err != SIR_OK) PortStop(8, "%s: %s", name, SirErrorText(err));
            hudAddr = *(u32*)(dst + 0x1F4);
            levelHdr = levelHdrCopy ? levelHdrCopy : dst;
            levelHdrSize = e->size;
            zone0 = NULL;
            LevelZoneDoneFree(&zone0Done);
            sConverted++;
            PortLog("%s header converted%s: %u pointers, %u fields (%u pointers outside the mapped ranges)", name,
                    place != dst ? " in the buffer it is copied from" : "", reloc.ptrImage, reloc.fieldsSwapped, reloc.ptrUnknown + reloc.ptrDolKept);
            return;
        }
        // Zone images are read to the addresses the console uses. The records and definitions of
        // the entities of a zone other than 0 lie in zone 0's image, which stays loaded.
        if (e->type == 0 || (e->type > 0 && zone0 != NULL)) {
            u8* place = LevelFinalPlace(e->type, dst);
            u32 base = (u32)(uintptr_t)place;
            SirRelocAddRange(&reloc, 0x90000000u, 0x04000000u, (u8*)(uintptr_t)0x90000000u);
            CheckFixUpIndices(name);
            LevelZoneRoots roots;
            LevelZoneOther other = { zone0Read, zone0Size, (u32)(uintptr_t)zone0, &zone0Done };
            const LevelZoneOther* in0 = e->type > 0 ? &other : NULL;
            // The header block's tables have been laid out by fn_80129CC8 by now (in the copy they
            // are still as the file has them, which is the same layout).
            if (!levelHdr || !LevelZoneRootsCollect(levelHdr, levelHdrSize, place, e->size, base, in0, &roots))
                PortStop(8, "%s zone %d: %s", name, e->type, SirErrorText(SIR_ERR_LEVEL));
            if (roots.hud == 0 && hudAddr >= base && hudAddr - base < e->size) roots.hud = hudAddr;
            u32 entityCount = roots.entityCount;
            SirError err = LevelZoneConvert(&reloc, dst, e->size, base, &roots, e->type == 0 ? &zone0Done : NULL, in0);
            LevelZoneRootsFree(&roots);
            if (err != SIR_OK) PortStop(8, "%s zone %d: %s", name, e->type, SirErrorText(err));
            if (e->type == 0) { zone0 = place; zone0Read = dst; zone0Size = e->size; }
            sConverted++;
            PortLog("%s zone %d%s: what the level start and WorldTick read of %u entities and the zone's other objects converted (%u pointers, %u fields); the rest of the zone is not",
                    name, e->type, place != dst ? " (in the buffer it is copied from)" : "", entityCount, reloc.ptrImage, reloc.fieldsSwapped);
            return;
        }
    }
    // Textures (shared_texture.sir): one texture record per section, the type is the name's hash.
    if (n >= 18 && _stricmp(name + n - 18, "shared_texture.sir") == 0) {
        SirError err = TextureStreamConvert(dst, e->size);
        if (err != SIR_OK) PortStop(8, "%s section %08x: %s", name, (u32)e->type, SirErrorText(err));
        sConverted++;
        return;
    }
    // Shared zones (shared_ui.sir, shared_parts.sir): section type N is the image of zone N. It
    // starts with a copy of the zone's slot, which fn_80135BC0 copies into the header block.
    bool shared = (n >= 13 && _stricmp(name + n - 13, "shared_ui.sir") == 0) || (n >= 16 && _stricmp(name + n - 16, "shared_parts.sir") == 0);
    if (shared && e->type > 0 && zone0 != NULL && levelHdr != NULL) {
        u32 base = (u32)(uintptr_t)dst;
        SirRelocInit(&reloc, NULL, NULL);
        SirRelocAddRange(&reloc, 0x90000000u, 0x04000000u, (u8*)(uintptr_t)0x90000000u);
        CheckFixUpIndices(name);
        LevelZoneOther other = { zone0, zone0Size, (u32)(uintptr_t)zone0, &zone0Done };
        LevelZoneRoots roots;
        if (!LevelSharedZoneRootsCollect(dst, e->size, base, LevelSlotSize(levelHdr, levelHdrSize), &other, &roots))
            PortStop(8, "%s zone %d: %s", name, e->type, SirErrorText(SIR_ERR_LEVEL));
        SirError err = LevelZoneConvert(&reloc, dst, e->size, base, &roots, NULL, &other);
        LevelZoneRootsFree(&roots);
        if (err != SIR_OK) PortStop(8, "%s zone %d: %s", name, e->type, SirErrorText(err));
        sConverted++;
        PortLog("%s zone %d: slot copy, zone objects, the screen set and the common part of its widgets converted (%u pointers, %u fields); the rest of the zone is not",
                name, e->type, reloc.ptrImage, reloc.fieldsSwapped);
        return;
    }
    Unconverted(name, e, "no converter for this section type yet");
}

const char* PortFileStats(char* buf, u32 n) {
    snprintf(buf, n, "%u section(s) converted, %u kind(s) of section left big-endian", sConverted, sUnconverted);
    return buf;
}
#endif
