// PC port: big-endian `.sir` data loader (not decompiled code; there is no original address).
// Converts Wii data in place so the decompiled code can read it at the original offsets:
// scalars are byte-swapped by field type, 32-bit pointer slots become host pointers, and
// everything a schema does not name (strings, byte arrays, untyped regions) is left alone.
// Design: docs/loader.md.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/sir_loader.h"

static_assert(sizeof(GameDbTable) == GAMEDB_DESC_SIZE, "GameDbTable must match the file layout");
static_assert(sizeof(SirCatalogEntry) == SIR_SECTION_ENTRY && sizeof(SirCatalogFile) == 0x40, "catalog records");

// ------------------------------------------------------------------------------------ bytes

static inline u32 Be32(const u8* p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

static inline void Swap16(u8* p) {
    u8 t = p[0]; p[0] = p[1]; p[1] = t;
}

static inline void Swap32(u8* p) {
    u8 t = p[0]; p[0] = p[3]; p[3] = t;
    t = p[1]; p[1] = p[2]; p[2] = t;
}

static inline u32 Host32(const u8* p) {
    u32 v;
    memcpy(&v, p, 4);
    return v;
}

// A pointer slot is 4 bytes in the data; the Win32 build stores the host pointer in it.
static inline void StorePtr(u8* slot, const void* p) {
    memcpy(slot, &p, sizeof(void*));
}

// Main memory of the Wii (MEM1): main.dol code and data live here.
static inline bool IsDolAddr(u32 a) { return a >= 0x80000000u && a < 0x81800000u; }

// ------------------------------------------------------------------------------------ container

extern "C" s32 SirContainerSectionCount(const u8* file, u32 fileSize) {
    if (file == NULL || fileSize < SIR_SECTION_TABLE || Be32(file) != SIR_MAGIC) return -1;
    u32 count = Be32(file + 8);
    if (count > SIR_MAX_SECTIONS) return -1;
    if ((u64)SIR_SECTION_TABLE + (u64)count * SIR_SECTION_ENTRY > fileSize) return -1;
    return (s32)count;
}

extern "C" BOOL SirContainerSection(const u8* file, u32 fileSize, s32 index, SirSection* out) {
    s32 count = SirContainerSectionCount(file, fileSize);
    if (index < 0 || index >= count) return FALSE;
    const u8* e = file + SIR_SECTION_TABLE + (u32)index * SIR_SECTION_ENTRY;
    out->type = (s32)Be32(e + 0x00);
    out->offset = Be32(e + 0x04);
    out->size = Be32(e + 0x08);
    out->unk0C = Be32(e + 0x0C);
    out->unk10 = Be32(e + 0x10);
    out->hash = Be32(e + 0x14);
    if (out->offset == 0 || (u64)out->offset + out->size > fileSize) return FALSE;
    return TRUE;
}

// ------------------------------------------------------------------------------------ catalog.sir

#define CATALOG_FILE_SIZE 0x40

extern "C" SirError SirCatalogConvert(u8* file, u32 fileSize, SirCatalog* out) {
    memset(out, 0, sizeof(*out));
    if (file == NULL || fileSize < SIR_SECTION_TABLE || Be32(file) != SIR_MAGIC) return SIR_ERR_NOT_SIR;
    u32 entryCount = Be32(file + 0x08);
    u32 fileCount = Be32(file + 0x0C);
    u64 filesOff = (u64)SIR_SECTION_TABLE + (u64)entryCount * SIR_SECTION_ENTRY;
    if (filesOff + (u64)fileCount * CATALOG_FILE_SIZE > fileSize) return SIR_ERR_CATALOG;

    for (u32 o = 0x04; o < 0x14; o += 4) Swap32(file + o);         // version, counts, hash
    u8* p = file + SIR_SECTION_TABLE;
    for (u32 i = 0; i < entryCount * 6; i++, p += 4) Swap32(p);    // six u32 per entry
    for (u32 i = 0; i < fileCount; i++, p += CATALOG_FILE_SIZE) {  // two u32, then the name bytes
        Swap32(p);
        Swap32(p + 4);
    }
    out->entries = (SirCatalogEntry*)(file + SIR_SECTION_TABLE);
    out->entryCount = entryCount;
    out->files = (SirCatalogFile*)(file + (u32)filesOff);
    out->fileCount = fileCount;
    return SIR_OK;
}

extern "C" const SirCatalogEntry* SirCatalogFind(const SirCatalog* c, u32 nameHash, s32 type) {
    for (u32 i = 0; i < c->entryCount; i++)
        if (c->entries[i].type == type && c->entries[i].nameHash == nameHash) return &c->entries[i];
    return NULL;
}

extern "C" s32 SirCatalogFileIndex(const SirCatalog* c, u32 nameHash) {
    for (u32 i = 0; i < c->fileCount; i++)
        if (c->files[i].nameHash == nameHash) return (s32)i;
    return -1;
}

// ------------------------------------------------------------------------------------ relocation

extern "C" void SirRelocInit(SirReloc* r, SirDolResolver resolveDol, void* user) {
    memset(r, 0, sizeof(*r));
    r->resolveDol = resolveDol;
    r->resolveUser = user;
}

extern "C" BOOL SirRelocAddRange(SirReloc* r, u32 wiiBase, u32 size, u8* host) {
    if (r->rangeCount >= SIR_MAX_RANGES || host == NULL || size == 0) return FALSE;
    SirRange* g = &r->ranges[r->rangeCount++];
    g->wiiBase = wiiBase;
    g->size = size;
    g->host = host;
    return TRUE;
}

extern "C" u8* SirRelocToHost(const SirReloc* r, u32 wiiAddr) {
    for (s32 i = 0; i < r->rangeCount; i++) {
        const SirRange* g = &r->ranges[i];
        // One past the end is a valid pointer too (end-of-array pointers).
        if (wiiAddr >= g->wiiBase && wiiAddr - g->wiiBase <= g->size) return g->host + (wiiAddr - g->wiiBase);
    }
    return NULL;
}

// Converts one big-endian pointer slot.
static void ConvertPtr(SirReloc* r, u8* slot) {
    u32 wii = Be32(slot);
    if (wii == 0) return;   // NULL is the same in both byte orders
    u8* host = SirRelocToHost(r, wii);
    if (host != NULL) {
        StorePtr(slot, host);
        r->ptrImage++;
        return;
    }
    if (IsDolAddr(wii)) {
        void* p = r->resolveDol != NULL ? r->resolveDol(wii, r->resolveUser) : NULL;
        if (p != NULL) {
            StorePtr(slot, p);
            r->ptrDolResolved++;
            return;
        }
        r->ptrDolKept++;
    } else {
        r->ptrUnknown++;
    }
    // No host counterpart: keep the Wii address as a host-order value, so it is recognisable
    // in a debugger and faults when used (the port never maps the Wii's address ranges).
    Swap32(slot);
}

static u32 FieldSize(u8 type) {
    return type == SIR_U16 ? 2u : 4u;
}

extern "C" BOOL SirConvertRecord(SirReloc* r, u8* rec, u32 recSize, const SirField* fields, s32 n) {
    for (s32 i = 0; i < n; i++) {
        const SirField* f = &fields[i];
        if (f->type < SIR_U16 || f->type > SIR_PTR) return FALSE;
        if ((u64)f->offset + (u64)f->count * FieldSize(f->type) > recSize) return FALSE;
    }
    for (s32 i = 0; i < n; i++) {
        const SirField* f = &fields[i];
        u8* p = rec + f->offset;
        for (u32 k = 0; k < f->count; k++) {
            switch (f->type) {
            case SIR_U16: Swap16(p); p += 2; r->fieldsSwapped++; break;
            case SIR_U32:
            case SIR_F32: Swap32(p); p += 4; r->fieldsSwapped++; break;
            default:      ConvertPtr(r, p); p += 4; break;
            }
        }
    }
    return TRUE;
}

extern "C" const char* SirErrorText(SirError e) {
    switch (e) {
    case SIR_OK:                 return "ok";
    case SIR_ERR_NOT_SIR:        return "not a PNP0 .sir container";
    case SIR_ERR_SECTIONS:       return "unexpected sections (need a 0x1000 header block and an image)";
    case SIR_ERR_BOUNDS:         return "pointer or table outside the image";
    case SIR_ERR_GAMEDB_LAYOUT:  return "game database descriptor not recognised";
    case SIR_ERR_GAMEDB_FIELD:   return "game database field has an unknown type or a size that does not fit it";
    case SIR_ERR_GAMEDB_ROWSIZE: return "game database field sizes do not add up to the row size";
    case SIR_ERR_GAMEDB_OVERLAP: return "two game database tables share row storage";
    case SIR_ERR_CATALOG:        return "catalog arrays do not fit in the file";
    case SIR_ERR_INPUT:          return "input table not recognised (count, step pointer, step type or source count)";
    case SIR_ERR_TEXTURE:        return "texture list not recognised (links, header or data outside the image)";
    case SIR_ERR_STRINGS:        return "string table array or a string outside the image";
    case SIR_ERR_ACTIONS:        return "action table array or a record outside the image, or a record that is not a program (length, jump)";
    case SIR_ERR_UI:             return "UI root, font or UI object not recognised (pointer outside the image, font kind or page count)";
    case SIR_ERR_SCREENS:        return "screen state machine not recognised (count, node type or pointer outside the image)";
    case SIR_ERR_EFFECTS:        return "effect definition table, a texture or a pool pointer of it outside the image";
    case SIR_ERR_SCREEN_SETS:    return "screen set database not recognised (table or record outside the image, or keys not in ascending order)";
    case SIR_ERR_POOL:           return "object pool not recognised (counts, block, or a list pointer that is not one of its nodes)";
    case SIR_ERR_LEVEL:          return "level header block not recognised (zone slot table)";
    case SIR_ERR_ZONE_RECORD:    return "zone record of a level zone not recognised (list that leaves the zone or loops, bad animation player)";
    case SIR_ERR_ENTITY:         return "entity of a level zone not recognised (part list outside the zone, child list that loops)";
    case SIR_ERR_SOUND:          return "voice line is not a .dsp header with samples (format, sample rate)";
    case SIR_ERR_OVERLAP:        return "two objects of the image share bytes";
    case SIR_ERR_IO:             return "cannot read file";
    }
    return "?";
}

// ------------------------------------------------------------------------------------ game database

// GameDb object (header slot 0xDC8): three build-time addresses, then data and count: an array
// with one element, the block of table descriptors.
static const SirField kGameDbFields[] = {
    { 0x00, SIR_U32, 3 },    // addresses from the build that baked the file (see GameDbTable)
    { 0x0C, SIR_PTR, 1 },
    { 0x10, SIR_U32, 1 },
};
#define GAMEDB_OBJ_SIZE 0x14

// Table descriptor (GameDbTable). The flag byte at 0x424 needs no conversion.
static const SirField kGameDbTableFields[] = {
    { 0x000, SIR_U32, 2 },                       // buildAddr
    { 0x008, SIR_PTR, 1 },                       // rows
    { 0x00C, SIR_U32, 5 },                       // count, unk10, layoutHash, rowSize, count2
    { 0x020, SIR_U32, GAMEDB_MAX_FIELDS * 4 },   // fieldType, fieldSize, fieldName, fieldRef
    { 0x420, SIR_U32, 1 },                       // fieldCount
};

// Size in bytes a field type must have, 0 for a type code we have not seen.
static u32 GameDbTypeSize(u32 type) {
    switch (type) {
    case GAMEDB_T_INT:
    case GAMEDB_T_FLOAT:
    case GAMEDB_T_HASH:
    case GAMEDB_T_INT2:  return 4;
    case GAMEDB_T_SHORT: return 2;
    case GAMEDB_T_FLAG:
    case GAMEDB_T_BYTE:  return 1;
    }
    return 0;
}

// Row layout of one table, worked out from the big-endian descriptor at `desc`.
struct RowLayout {
    u32 rowsOff;      // offset of the rows in the image
    u32 count;
    u32 rowSize;
    u32 fieldCount;
    u16 fieldOff[GAMEDB_MAX_FIELDS];
    u8  fieldSize[GAMEDB_MAX_FIELDS];
};

static SirError ReadRowLayout(const u8* desc, u32 imageSize, RowLayout* out) {
    u32 rows = Be32(desc + 0x08);
    out->count = Be32(desc + 0x0C);
    out->rowSize = Be32(desc + 0x18);
    out->fieldCount = Be32(desc + 0x420);
    out->rowsOff = 0;
    if (out->fieldCount > GAMEDB_MAX_FIELDS) return SIR_ERR_GAMEDB_LAYOUT;
    if (out->count == 0) return SIR_OK;      // the empty table: its other words are uninitialised
    if (out->count != Be32(desc + 0x1C) || out->fieldCount == 0) return SIR_ERR_GAMEDB_LAYOUT;

    // Fields are packed in declaration order, each aligned to its own size; the row is padded
    // to 4 bytes. (Checked against the stored row size, so a wrong guess cannot go unnoticed.)
    u32 off = 0;
    for (u32 i = 0; i < out->fieldCount; i++) {
        u32 size = Be32(desc + 0x120 + i * 4);
        u32 want = GameDbTypeSize(Be32(desc + 0x020 + i * 4));
        if (want == 0 || size != want) return SIR_ERR_GAMEDB_FIELD;
        off = (off + size - 1) & ~(size - 1);
        if (off > 0xFFFF) return SIR_ERR_GAMEDB_ROWSIZE;
        out->fieldOff[i] = (u16)off;
        out->fieldSize[i] = (u8)size;
        off += size;
    }
    if (((off + 3) & ~3u) != out->rowSize) return SIR_ERR_GAMEDB_ROWSIZE;

    if (rows < GLOBAL_BINARY_WII_BASE) return SIR_ERR_BOUNDS;
    out->rowsOff = rows - GLOBAL_BINARY_WII_BASE;
    if ((u64)out->rowsOff + (u64)out->count * out->rowSize > imageSize) return SIR_ERR_BOUNDS;
    return SIR_OK;
}

extern "C" SirError GameDbConvert(SirReloc* r, u8* image, u32 imageSize, u32 dbWiiAddr, s32* failTable) {
    s32 dummy;
    if (failTable == NULL) failTable = &dummy;
    *failTable = -1;

    if (dbWiiAddr < GLOBAL_BINARY_WII_BASE) return SIR_ERR_BOUNDS;
    u32 dbOff = dbWiiAddr - GLOBAL_BINARY_WII_BASE;
    if ((u64)dbOff + GAMEDB_OBJ_SIZE > imageSize) return SIR_ERR_BOUNDS;
    u8* db = image + dbOff;

    u32 dataAddr = Be32(db + 0x0C);
    if (Be32(db + 0x10) != 1 || dataAddr < GLOBAL_BINARY_WII_BASE) return SIR_ERR_GAMEDB_LAYOUT;
    u32 dataOff = dataAddr - GLOBAL_BINARY_WII_BASE;
    if ((u64)dataOff + (u64)GAMEDB_TABLE_COUNT * GAMEDB_DESC_SIZE > imageSize) return SIR_ERR_BOUNDS;
    u8* data = image + dataOff;

    // Pass 1: validate every table before touching a byte, so a bad file is rejected whole.
    static RowLayout layouts[GAMEDB_TABLE_COUNT];   // 8 KB: too big for the stack of a game thread
    for (s32 t = 0; t < GAMEDB_TABLE_COUNT; t++) {
        const u8* desc = data + t * GAMEDB_DESC_SIZE;
        *failTable = t;
        // Every descriptor starts with the same two words as the database object (+4, +8).
        if (memcmp(desc, db + 4, 8) != 0) return SIR_ERR_GAMEDB_LAYOUT;
        SirError e = ReadRowLayout(desc, imageSize, &layouts[t]);
        if (e != SIR_OK) return e;
    }
    for (s32 a = 0; a < GAMEDB_TABLE_COUNT; a++) {
        const RowLayout* la = &layouts[a];
        if (la->count == 0) continue;
        u32 aEnd = la->rowsOff + la->count * la->rowSize;
        *failTable = a;
        // Rows must not lie inside the descriptor block or the database object either.
        if (la->rowsOff < dataOff + GAMEDB_TABLE_COUNT * GAMEDB_DESC_SIZE && aEnd > dataOff) return SIR_ERR_GAMEDB_OVERLAP;
        if (la->rowsOff < dbOff + GAMEDB_OBJ_SIZE && aEnd > dbOff) return SIR_ERR_GAMEDB_OVERLAP;
        for (s32 b = a + 1; b < GAMEDB_TABLE_COUNT; b++) {
            const RowLayout* lb = &layouts[b];
            if (lb->count == 0) continue;
            if (la->rowsOff < lb->rowsOff + lb->count * lb->rowSize && lb->rowsOff < aEnd) return SIR_ERR_GAMEDB_OVERLAP;
        }
    }
    *failTable = -1;

    // Pass 2: rows by field size, then the descriptors, then the database object.
    for (s32 t = 0; t < GAMEDB_TABLE_COUNT; t++) {
        const RowLayout* l = &layouts[t];
        u8* row = image + l->rowsOff;
        for (u32 i = 0; i < l->count; i++, row += l->rowSize) {
            for (u32 f = 0; f < l->fieldCount; f++) {
                u8* p = row + l->fieldOff[f];
                if (l->fieldSize[f] == 4)      { Swap32(p); r->fieldsSwapped++; }
                else if (l->fieldSize[f] == 2) { Swap16(p); r->fieldsSwapped++; }
            }
        }
        SirConvertRecord(r, data + t * GAMEDB_DESC_SIZE, GAMEDB_DESC_SIZE, kGameDbTableFields,
                         (s32)(sizeof(kGameDbTableFields) / sizeof(kGameDbTableFields[0])));
    }
    SirConvertRecord(r, db, GAMEDB_OBJ_SIZE, kGameDbFields, (s32)(sizeof(kGameDbFields) / sizeof(kGameDbFields[0])));
    return SIR_OK;
}

// ------------------------------------------------------------------------------------ image objects
//
// The objects below have no schema in the file. Each is checked on the big-endian bytes first and
// recorded as a region; only when every check passed, and no two regions share bytes, is the
// first byte changed. "code": a decompiled function reads the field with that type. "data": typed
// from the values in the shipped file only.

#define COUNTOF(a) ((s32)(sizeof(a) / sizeof((a)[0])))

enum RegionKind {
    REGION_INPUT_TABLE, REGION_INPUT_STEP, REGION_TEXTURE_LIST, REGION_TEXTURE, REGION_STRING_TABLE,
    REGION_ACTION_TABLE, REGION_PTR_ARRAY, REGION_U32_ARRAY,
    REGION_UI_ROOT, REGION_UI_FONT, REGION_UI_FONT_OBJ, REGION_UI_FONT_LIST, REGION_UI_FONT_DATA, REGION_UI_DRAW, REGION_UI_PAD,
    REGION_FSM, REGION_FSM_DEF, REGION_FSM_STATES, REGION_FSM_EVENTS, REGION_FSM_SHARED_EVENTS, REGION_FSM_NODE,
    REGION_FSM_TRANSITION, REGION_FSM_SHARED_TRANSITION, REGION_HUD, REGION_LEVEL_HUD,
    REGION_FX_BOLTS, REGION_FX_TRAILS, REGION_POOL, REGION_POOL_BLOCK,
    REGION_ENTITY, REGION_ENTITY_RECORD, REGION_ENTITY_PART, REGION_PTR_COUNT, REGION_COUNT_PTR, REGION_POOL_TABLE,
    REGION_ENTITY_PROP, REGION_PROP_DEF, REGION_KEY_PTR, REGION_ENTITY_TIMED,
    REGION_UI_TREE_NODE, REGION_UI_LIST, REGION_UI_WIDGET, REGION_UI_EVENT, REGION_UI_GRID, REGION_UI_FIXED,
    REGION_UI_SET, REGION_UI_SCREEN, REGION_UI_GRAPHIC, REGION_UI_TEXT,
    REGION_LOCATORS, REGION_UI_SLIDER,
    REGION_ZONE_EFFECT, REGION_ANIM_SET, REGION_ANIM_DEFS, REGION_ANIM_EVENTS, REGION_U16_ARRAY, REGION_ANIM_OBJ, REGION_SKELETON, REGION_BONE_OPS,
    REGION_ATTACH_POINTS, REGION_SCENE_LIGHT, REGION_GX_STATE,
    REGION_UI_ANIM, REGION_UI_ANIM_TRACK, REGION_UI_ANIM_EVENTS, REGION_UI_ANIM_KEYS, REGION_UI_ANIM_ENTRIES,
    REGION_ZONE_SYSTEM, REGION_ANIM_PLAYER, REGION_ANIM_ELEM, REGION_ANIM_TRACK, REGION_ANIM_KEYS, REGION_F32_ARRAY,
    REGION_SOUND_GROUP, REGION_SOUND, REGION_SOUND_INFO, REGION_DSP_HEADER,
    REGION_RUNTIME    // memory the game initialises itself: nothing to convert, but nothing else may be there
};

struct Region {
    u32 off;     // in the image
    u32 size;
    u8  kind;
};

#define TEXTURE_MAX 4096
#define REGION_MAX  (1u << 20)   // 12 MB of plan: far more than any image of this kind needs

// The list of objects found by the checks. It grows on the C heap and is freed when the
// conversion is done (or refused).
struct ImagePlan {
    Region* regions;
    u32     count;
    u32     capacity;
};

// Where the lists come from. The game's port sets its own heap (SirLoaderSetAllocator): a list is kept between
// two reads of a level (LevelZoneDone), and a savestate has to hold it.
static void* (*sPlanAlloc)(size_t) = malloc;
static void  (*sPlanFree)(void*) = free;
extern "C" void SirLoaderSetAllocator(void* (*alloc)(size_t), void (*release)(void*)) { sPlanAlloc = alloc; sPlanFree = release; }

static bool PlanAdd(ImagePlan* plan, u32 off, u32 size, u8 kind) {
    if (plan->count == plan->capacity) {
        u32 cap = plan->capacity ? plan->capacity * 2 : 1024;
        if (cap > REGION_MAX) return false;
        Region* grown = (Region*)sPlanAlloc((size_t)cap * sizeof(Region));
        if (grown == NULL) return false;
        if (plan->count) memcpy(grown, plan->regions, (size_t)plan->count * sizeof(Region));
        sPlanFree(plan->regions);
        plan->regions = grown;
        plan->capacity = cap;
    }
    Region* g = &plan->regions[plan->count++];
    g->off = off;
    g->size = size;
    g->kind = kind;
    return true;
}

static void PlanFree(ImagePlan* plan) {
    sPlanFree(plan->regions);
    plan->regions = NULL;
    plan->count = plan->capacity = 0;
}

// True when an object of this kind at `off` is already in the plan (objects that several others
// point to are walked once).
static bool PlanHas(const ImagePlan* plan, u32 off, u8 kind) {
    for (u32 i = 0; i < plan->count; i++)
        if (plan->regions[i].off == off && plan->regions[i].kind == kind) return true;
    return false;
}

static int RegionCompare(const void* a, const void* b) {
    const Region* ra = (const Region*)a;
    const Region* rb = (const Region*)b;
    if (ra->off != rb->off) return ra->off < rb->off ? -1 : 1;
    if (ra->size != rb->size) return ra->size < rb->size ? -1 : 1;
    return (int)ra->kind - (int)rb->kind;
}

// Sorts the plan, drops an object that was found more than once (same place, size and kind) and
// reports whether any two of the others share bytes.
static bool PlanFinish(ImagePlan* plan) {
    if (plan->count == 0) return true;
    qsort(plan->regions, plan->count, sizeof(Region), RegionCompare);
    u32 n = 1;
    for (u32 i = 1; i < plan->count; i++) {
        const Region* prev = &plan->regions[n - 1];
        const Region* g = &plan->regions[i];
        if (g->off == prev->off && g->size == prev->size && g->kind == prev->kind) continue;
        if (g->off < prev->off + prev->size) return false;   // the ones kept so far are disjoint, so the last one ends last
        plan->regions[n++] = *g;
    }
    plan->count = n;
    return true;
}

// PORT_LOADER_WATCH=<hex Wii address>: log every object the level loader converts within 0x200 bytes of it
// (which zone's pass, where, how long, which kind), to find a field that is converted twice or not at all.
static u32 sWatchAddr = 0, sConvertBase = 0;
static u32 LoaderWatch(void) {
    const char* v = getenv("PORT_LOADER_WATCH");     // read every time: a savestate brings this file's statics back
    sWatchAddr = v != NULL ? (u32)strtoul(v, NULL, 16) : 0;
    return sWatchAddr;
}

// Wii address of the image being checked and converted: global_binary.sir's, or a level zone's
// while LevelZoneConvert runs. (One file is converted at a time, on the thread that read it.)
static u32 sImageBase = GLOBAL_BINARY_WII_BASE;
// Zone 0's image while another zone is converted (LevelZoneOther), and what this conversion adds in it.
static const u8* sOtherImage = NULL;
static u32 sOtherSize = 0, sOtherBase = 0;
static const ImagePlan* sOtherDone = NULL;
static ImagePlan sOtherNew = { NULL, 0, 0 };

static int OtherSpan(u32 addr, u32 size, u32* off) {
    if (sOtherImage == NULL || addr < sOtherBase || (u64)(addr - sOtherBase) + size > sOtherSize) return 0;
    *off = addr - sOtherBase;
    return 1;
}

// Notes an object of the other image for conversion, unless that image's earlier conversions
// already converted exactly this object.
static bool OtherAdd(u32 off, u32 size, u8 kind) {
    for (u32 i = 0; sOtherDone != NULL && i < sOtherDone->count; i++) {
        const Region* g = &sOtherDone->regions[i];
        if (g->off == off && g->size == size && g->kind == kind) return true;
    }
    return PlanAdd(&sOtherNew, off, size, kind);
}

static bool sLevelZone = false;   // the image is a level zone

// An object of the image being converted or of the other image (zone 0), by Wii address.
//   done: for the other image, an earlier conversion converted exactly this object (it is in host
//         form); for this image, it is in the plan already (still big-endian until the plan runs).
struct AnyObj { bool found, other, done; u32 off; };

static bool PlanHas(const ImagePlan* plan, u32 off, u8 kind);
static bool ImageSpan(u32 addr, u64 size, u32 imageSize, u32* off);

static AnyObj AnyFind(const ImagePlan* plan, u32 imageSize, u32 addr, u32 size, u8 kind) {
    AnyObj o = { false, false, false, 0 };
    if (addr == 0) return o;
    if (ImageSpan(addr, size, imageSize, &o.off)) {
        o.found = true;
        o.done = PlanHas(plan, o.off, kind);
    } else if (OtherSpan(addr, size, &o.off)) {
        o.found = o.other = true;
        for (u32 i = 0; sOtherDone != NULL && i < sOtherDone->count && !o.done; i++) {
            const Region* g = &sOtherDone->regions[i];
            o.done = g->off == o.off && g->size == size && g->kind == kind;
        }
    }
    return o;
}

static bool AnyAdd(ImagePlan* plan, const AnyObj* o, u32 size, u8 kind) {
    if (o->done) return true;
    return o->other ? PlanAdd(&sOtherNew, o->off, size, kind) : PlanAdd(plan, o->off, size, kind);
}

// A number of the object, whichever form it is in.
static u32 AnyWord(const u8* image, const AnyObj* o, u32 at, u32 bytes) {
    const u8* p = (o->other ? sOtherImage : image) + o->off + at;
    if (o->other && o->done) return bytes == 2 ? *(const u16*)p : Host32(p);
    return bytes == 2 ? (u32)((p[0] << 8) | p[1]) : Be32(p);
}

// A pointer of the object as a Wii address. A converted pointer leads into one of the two images,
// or is a Wii address that was kept.
static u32 AnyPtr(const u8* image, u32 imageSize, const AnyObj* o, u32 at) {
    if (!(o->other && o->done)) return Be32((o->other ? sOtherImage : image) + o->off + at);
    const u8* p;
    memcpy(&p, sOtherImage + o->off + at, sizeof(p));
    if (p >= sOtherImage && p < sOtherImage + sOtherSize) return sOtherBase + (u32)(p - sOtherImage);
    if (p >= image && p < image + imageSize) return sImageBase + (u32)(p - image);
    return (u32)(uintptr_t)p;
}

// Offset in the image of the `size` bytes at Wii address `addr`; false if they are not all inside.
static bool ImageSpan(u32 addr, u64 size, u32 imageSize, u32* off) {
    if (addr < sImageBase) return false;
    u32 o = addr - sImageBase;
    if ((u64)o + size > imageSize) return false;
    *off = o;
    return true;
}

// ---- input table (header slot 0xDC4)

// Sequence (InputSequence). RemoteInputUpdate (0x80020070) reads every field.
static const SirField kInputSequenceFields[] = {
    { 0x00, SIR_U32, 2 },                 // code: s32 stepCount, s32 current
    { 0x08, SIR_F32, 2 },                 // code: lastValue, value
    { 0x10, SIR_PTR, INPUT_SEQ_STEPS },   // code: steps (fn_80020334 walks stepCount of them)
    { 0x38, SIR_F32, 3 },                 // code: time, minTime, maxTime
    { 0x44, SIR_U32, 1 },                 // code: s32 timing
};
#define INPUT_SEQ_SIZE 0x48

// Table, after the sequences.
static const SirField kInputTableFields[] = {
    { INPUT_TABLE_INDEX, SIR_U32, INPUT_INDEX_COUNT },   // code: RemoteInputGet
    { INPUT_TABLE_COUNT, SIR_U32, 1 },                   // code: fn_800214D4
    { 0x1EEC, SIR_PTR, 6 },                              // data: the first six sequences at +0x240
    { 0x1F04, SIR_U32, 1 },                              // data: 0
    { 0x1F0C, SIR_F32, 1 },                              // data: 1.0
    { 0x1F10, SIR_U32, 2 },                              // data: 1, 0
    { 0x1F18, SIR_F32, 2 },                              // data: 320.0, 240.0
    { 0x1F20, SIR_U32, 1 },                              // data: 0
};

// Step, the part every class has. fn_80020040 (vtable slot 2 of every class) writes +0x00, +0x04 and +0x40,
// +0x44 as floats and +0x08, +0x3C as words; fn_80020334 switches on +0x48 and stores the vtable
// at +0x4C, so the address the file has there is never used.
static const SirField kInputStepFields[] = {
    { 0x00, SIR_F32, 2 },    // code: raw, value
    { 0x08, SIR_U32, 1 },    // code: s32 sourceCount
    { 0x3C, SIR_U32, 1 },    // code: s32 (fn_800205CC: zero outside the range instead of clamping)
    { 0x40, SIR_F32, 2 },    // code: range
    { 0x48, SIR_U32, 2 },    // code: type; build-time vtable address
};
// One source (fn_8001FCA8): u32 id, f32 value (written before it is read), u32 kind (0..0x1C).
static const SirField kInputSourceFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_F32, 1 }, { 0x08, SIR_U32, 1 },
};
// State behind the common part, by class.
static const SirField kInputStepPrev1[] = { { 0x50, SIR_F32, 1 } };                        // fn_800204D0, fn_800207A8, fn_800213A4
static const SirField kInputStepPrev4[] = { { 0x50, SIR_F32, INPUT_STEP_SOURCES } };       // fn_80021188, fn_8002129C: one per source
static const SirField kInputStepRotate[] = { { 0x50, SIR_U32, 1 }, { 0x54, SIR_F32, 3 } }; // fn_80020A64, fn_80020D88

// Size of a step in the file. The two-button toggle is 0x50 bytes like the classes without state,
// although fn_800208C0 uses +0x50..+0x5B: in the shipped file those bytes are the start of the
// next step. They are converted as that step's fields, not as toggle state.
static u32 InputStepSize(u32 type) {
    switch (type) {
    case INPUT_STEP_BUTTON_HELD:
    case INPUT_STEP_TWO_BUTTON_TOGGLE:
    case INPUT_STEP_ANY_BUTTONS_HELD: return 0x50;
    }
    return 0x60;
}

static u8* InputSequenceAt(u8* table, u32 i) {
    return table + (i < INPUT_SEQ_FIXED ? i * INPUT_SEQ_SIZE : INPUT_TABLE_SEQS + (i - INPUT_SEQ_FIXED) * INPUT_SEQ_SIZE);
}

static SirError InputTablePlan(ImagePlan* plan, u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, INPUT_TABLE_SIZE, imageSize, &off)) return SIR_ERR_INPUT;
    u8* table = image + off;
    u32 seqCount = Be32(table + INPUT_TABLE_COUNT);
    if (seqCount > INPUT_SEQ_MAX) return SIR_ERR_INPUT;
    for (u32 i = 0; i < INPUT_INDEX_COUNT; i++)
        if (Be32(table + INPUT_TABLE_INDEX + i * 4) >= INPUT_SEQ_MAX) return SIR_ERR_INPUT;
    if (!PlanAdd(plan, off, INPUT_TABLE_SIZE, REGION_INPUT_TABLE)) return SIR_ERR_INPUT;

    u32 firstStep = plan->count;
    for (u32 i = 0; i < INPUT_SEQ_FIXED + seqCount; i++) {
        const u8* seq = InputSequenceAt(table, i);
        u32 steps = Be32(seq);
        if (steps > INPUT_SEQ_STEPS) return SIR_ERR_INPUT;
        for (u32 k = 0; k < steps; k++) {
            u32 stepAddr = Be32(seq + 0x10 + k * 4), stepOff;
            if (stepAddr == 0) continue;                 // fn_80020334 skips these too
            if (!ImageSpan(stepAddr, 0x50, imageSize, &stepOff)) return SIR_ERR_INPUT;
            u32 type = Be32(image + stepOff + 0x48);
            if (type >= INPUT_STEP_TYPES || Be32(image + stepOff + 0x08) > INPUT_STEP_SOURCES) return SIR_ERR_INPUT;
            if (!ImageSpan(stepAddr, InputStepSize(type), imageSize, &stepOff)) return SIR_ERR_INPUT;
            bool seen = false;                           // a step can be in more than one sequence
            for (u32 j = firstStep; j < plan->count && !seen; j++) seen = plan->regions[j].off == stepOff;
            if (!seen && !PlanAdd(plan, stepOff, InputStepSize(type), REGION_INPUT_STEP)) return SIR_ERR_INPUT;
        }
    }
    return SIR_OK;
}

static void InputTableConvert(SirReloc* r, u8* table) {
    u32 seqCount = Be32(table + INPUT_TABLE_COUNT);
    for (u32 i = 0; i < INPUT_SEQ_FIXED + seqCount; i++)
        SirConvertRecord(r, InputSequenceAt(table, i), INPUT_SEQ_SIZE, kInputSequenceFields, COUNTOF(kInputSequenceFields));
    SirConvertRecord(r, table, INPUT_TABLE_SIZE, kInputTableFields, COUNTOF(kInputTableFields));
}

static void InputStepConvert(SirReloc* r, u8* step, u32 size) {
    u32 type = Be32(step + 0x48);
    u32 sources = Be32(step + 0x08);
    for (u32 i = 0; i < sources; i++)
        SirConvertRecord(r, step + 0x0C + i * 0xC, 0xC, kInputSourceFields, COUNTOF(kInputSourceFields));
    switch (type) {
    case INPUT_STEP_BUTTON_JUST_RELEASED:
    case INPUT_STEP_BUTTON_JUST_PRESSED:
    case INPUT_STEP_TOGGLE:
        SirConvertRecord(r, step, size, kInputStepPrev1, COUNTOF(kInputStepPrev1));
        break;
    case INPUT_STEP_ANY_BUTTONS_RELEASED:
    case INPUT_STEP_ANY_BUTTONS_JUST_PRESSED:
        SirConvertRecord(r, step, size, kInputStepPrev4, COUNTOF(kInputStepPrev4));
        break;
    case INPUT_STEP_ROTATE_CW:
    case INPUT_STEP_ROTATE_CCW:
        SirConvertRecord(r, step, size, kInputStepRotate, COUNTOF(kInputStepRotate));
        break;
    }
    SirConvertRecord(r, step, size, kInputStepFields, COUNTOF(kInputStepFields));
}

// ---- textures (header slot 0xD94: the start of the image)

// Texture header (SirTexture). The bytes at 0x0C-0x17 and the name need no conversion; pixel
// data and palette stay in GX layout for the port's GX layer.
static const SirField kTextureFields[] = {
    { 0x00, SIR_U32, 1 },    // data: 0x54
    { 0x04, SIR_U16, 4 },    // code: fn_801FCA38 (+4, +6), fn_80195A58 (+8, +0xA)
    { 0x18, SIR_U32, 1 },    // data: size of the pixel data
    { 0x1C, SIR_PTR, 2 },    // code: fn_80195A58 (pixel data, palette)
    { 0x24, SIR_U32, 1 },    // data
    { 0x28, SIR_PTR, 3 },    // data: next, prev, list
};
// Image start: a word, then the list {head, tail, count}. Typed from the data only.
static const SirField kTextureListFields[] = {
    { 0x00, SIR_U32, 1 },    // data: 0x42
    { 0x04, SIR_PTR, 2 },    // data: first and last texture (+0x28 of each)
    { 0x0C, SIR_U32, 1 },    // data: number of textures
};
#define TEXTURE_LIST_SIZE 0x10

static SirError TextureListPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, TEXTURE_LIST_SIZE, imageSize, &off)) return SIR_ERR_TEXTURE;
    u32 count = Be32(image + off + 0x0C);
    if (count > TEXTURE_MAX || !PlanAdd(plan, off, TEXTURE_LIST_SIZE, REGION_TEXTURE_LIST)) return SIR_ERR_TEXTURE;

    // Every node must name the node before it, so no texture can be visited twice.
    u32 node = Be32(image + off + 0x04), prev = 0;
    for (u32 i = 0; i < count; i++) {
        u32 texOff, dataOff;
        if (node < sImageBase + SIR_TEXTURE_NODE ||
            !ImageSpan(node - SIR_TEXTURE_NODE, SIR_TEXTURE_HEADER, imageSize, &texOff)) return SIR_ERR_TEXTURE;
        const u8* tex = image + texOff;
        if (Be32(tex) != SIR_TEXTURE_HEADER || Be32(tex + 0x2C) != prev || Be32(tex + 0x30) != addr + 4) return SIR_ERR_TEXTURE;
        if (!ImageSpan(Be32(tex + 0x1C), Be32(tex + 0x18), imageSize, &dataOff)) return SIR_ERR_TEXTURE;
        u32 palette = Be32(tex + 0x20);
        if (palette != 0 && !ImageSpan(palette, 2, imageSize, &dataOff)) return SIR_ERR_TEXTURE;
        if (!PlanAdd(plan, texOff, SIR_TEXTURE_HEADER, REGION_TEXTURE)) return SIR_ERR_TEXTURE;
        prev = node;
        node = Be32(tex + SIR_TEXTURE_NODE);
    }
    if (node != 0 || Be32(image + off + 0x08) != prev) return SIR_ERR_TEXTURE;
    return SIR_OK;
}

// ---- string table (header slot 0xDA8) and action table (header slot 0x844)

static const SirField kStringTableFields[] = {
    { 0x00, SIR_PTR, 2 },    // code: strings (LookupString in combat_hud.cpp); data: +4
    { 0x08, SIR_U32, 2 },    // data: build-time address; code: count
    { 0x10, SIR_PTR, 1 },    // data: first string
};
#define STRING_TABLE_SIZE 0x14

static const SirField kActionTableFields[] = {
    { 0x00, SIR_U32, 1 },    // code: count (TriggerDoAction)
    { 0x04, SIR_PTR, 1 },    // code: records; each element is passed on as the record's address
};
#define ACTION_TABLE_SIZE 0x08

// An object holding a count and a pointer to that many pointers, each NULL or into the image.
static SirError PtrArrayPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 objSize, u32 countAt,
                             u32 arrayAt, u8 kind, SirError err) {
    u32 off, arrayOff, dummy;
    if (!ImageSpan(addr, objSize, imageSize, &off)) return err;
    u32 count = Be32(image + off + countAt);
    if (!ImageSpan(Be32(image + off + arrayAt), (u64)count * 4, imageSize, &arrayOff)) return err;
    for (u32 i = 0; i < count; i++) {
        u32 p = Be32(image + arrayOff + i * 4);
        if (p != 0 && !ImageSpan(p, 1, imageSize, &dummy)) return err;
    }
    if (!PlanAdd(plan, off, objSize, kind)) return err;
    if (count != 0 && !PlanAdd(plan, arrayOff, count * 4, REGION_PTR_ARRAY)) return err;
    return SIR_OK;
}

// ---- UI root, fonts and the UI objects next to it (header slots 0xDB0-0xDBC)

// UI root (gScreenSets). The words at +0x00 (1) and +0x04-+0x0C (pointers to the objects of slots
// 0xDB8, 0xDB4 and 0xDBC) have no reader in the code and are left as they are.
static const SirField kUiRootFields[] = {
    { 0x10, SIR_U32, 1 },     // code: fn_801A0200 and fn_801A270C read a pointer here; the file has the fill word
    { 0x14, SIR_PTR, UI_ROOT_FONTS },   // code: ScreenSetsInit, fn_8018C9DC (font n is root[0x14 + (n - 1) * 4])
    { 0x20, SIR_F32, 19 },    // code: fn_8018CE64 writes, fn_801A31BC and fn_801A9174 read (+0x20..+0x68)
    { 0x6C, SIR_U32, 5 },     // code: fn_8019CF58 (+0x6C), fn_80198438 (+0x70, +0x74), ScreenSetsActivate (+0x78, +0x7C)
    // 0x80: u8 (ScreenSetsActivate, fn_8018CA38)
    { 0x84, SIR_U32, 4 },     // code: ScreenSetsCurrent (+0x84), fn_8018CF1C (+0x84..+0x90), UiInit clears them
    // 0x94: u8 (fn_80199684)
};

// Font: the root's three objects. Only +0xCC is read (fn_8018C9DC, ScreenSetsInit); the name at
// +0x04 is text.
static const SirField kUiFontFields[] = {
    { 0xCC, SIR_PTR, 1 },
};

// Font object (what +0xCC points at; also the object of slot 0xDB4). The kind word picks the
// class: ScreenSetsInit stores the vtable for it at +0x00, so the address the file has there is
// not used. Kind 1 (fn_801970D4, 0x128 bytes): font data at +0x08 (fn_80197D98, fn_80197EC4),
// page textures from +0x24 and their number at +0x124 (fn_80197118, fn_80198014). Kind 0
// (fn_80196E98, 0xC bytes): a pointer at +0x08.
static const SirField kUiFontObjFields[] = {
    { 0x00, SIR_U32, 2 },     // build-time vtable address; code: kind
    { 0x08, SIR_PTR, 1 },
};
static const SirField kUiFontObjPages[] = {
    { 0x124, SIR_U32, 1 },
};

// Object of slot 0xDB4 (lbl_805FB660): fn_80196D0C falls back to the font object at +0x04,
// fn_80196E7C copies +0x04 to +0x08 or stores another font object there.
static const SirField kUiFontListFields[] = {
    { 0x04, SIR_PTR, 2 },
};

// Object of slot 0xDB8 (lbl_805FB658). +0x00 (1) and the texture pointer at +0x0C have no reader.
static const SirField kUiDrawFields[] = {
    { 0x04, SIR_U32, 1 },     // code: UiInit stores gCamera here at boot (the file has a build-time address)
    { 0x08, SIR_PTR, 1 },     // code: fn_8019578C
    { 0x40, SIR_U32, 1 },     // code: fn_80196678, fn_80196B10, fn_80196B24
    { 0x404, SIR_U32, 1 },    // code: fn_80196B4C, fn_80196C88, fn_80196C9C
};

// Object of slot 0xDBC (lbl_805FB670). +0x00 (1) has no reader.
static const SirField kUiPadFields[] = {
    { 0x04, SIR_U32, 5 },     // code: fn_80198350 (+0x04), fn_801982A8 (+0x08..+0x10), fn_801982E4 (+0x14)
    { 0x18, SIR_F32, 2 },     // code: fn_801981FC
};

// NULL or a pointer into the image.
static bool ImagePtrOk(u32 addr, u32 imageSize) {
    u32 off;
    return addr == 0 || ImageSpan(addr, 1, imageSize, &off);
}

// Font data (what a kind 1 font object's +0x08 names; fn_8018979C, fn_80189EFC, fn_80189988,
// fn_801894F4): bytes +0x00..+0x07 and +0x0D..., then u16 glyph count +0x08 and kerning count
// +0x0A, 64 u16 hash buckets at +0x0C, `pages` (byte +3) page texture pointers at +0x8C, glyph
// records of 0x10 bytes (four u16 first, count + 1 of them) and kerning records of 6 bytes (three
// u16, count + 1 of them). The blobs in the file are big-endian; the game byte-swaps the ones it
// reads from a file with fn_801894F4.
#define UI_FONT_DATA_HEADER 0x8C
#define UI_FONT_GLYPH_SIZE  0x10
#define UI_FONT_KERN_SIZE   6

static inline u32 Be16(const u8* p) {
    return ((u32)p[0] << 8) | (u32)p[1];
}

static u32 UiFontDataSize(const u8* font) {
    return UI_FONT_DATA_HEADER + (u32)font[3] * 4 + (Be16(font + 8) + 1) * UI_FONT_GLYPH_SIZE +
           (Be16(font + 0xA) + 1) * UI_FONT_KERN_SIZE;
}

static SirError UiFontDataPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, UI_FONT_DATA_HEADER, imageSize, &off)) return SIR_ERR_UI;
    if (PlanHas(plan, off, REGION_UI_FONT_DATA)) return SIR_OK;
    const u8* font = image + off;
    u32 size = UiFontDataSize(font), pagesEnd = UI_FONT_DATA_HEADER + (u32)font[3] * 4;
    if (!ImageSpan(addr, size, imageSize, &off)) return SIR_ERR_UI;
    for (u32 k = UI_FONT_DATA_HEADER; k < pagesEnd; k += 4)
        if (!ImagePtrOk(Be32(font + k), imageSize)) return SIR_ERR_UI;
    return PlanAdd(plan, off, size, REGION_UI_FONT_DATA) ? SIR_OK : SIR_ERR_UI;
}

static void UiFontDataConvert(SirReloc* r, u8* font) {
    u32 glyphs = Be16(font + 8) + 1, kerns = Be16(font + 0xA) + 1, pages = font[3];
    for (u32 k = 0x8; k < 0x8C; k += 2) { Swap16(font + k); r->fieldsSwapped++; }   // counts, buckets
    for (u32 k = 0; k < pages; k++) ConvertPtr(r, font + UI_FONT_DATA_HEADER + k * 4);
    u8* g = font + UI_FONT_DATA_HEADER + pages * 4;
    for (u32 i = 0; i < glyphs; i++, g += UI_FONT_GLYPH_SIZE)
        for (u32 k = 0; k < 8; k += 2) { Swap16(g + k); r->fieldsSwapped++; }
    for (u32 i = 0; i < kerns; i++, g += UI_FONT_KERN_SIZE)
        for (u32 k = 0; k < 6; k += 2) { Swap16(g + k); r->fieldsSwapped++; }
}

static SirError UiFontObjPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, UI_FONT_OBJ_SIZE_KIND0, imageSize, &off)) return SIR_ERR_UI;
    u32 kind = Be32(image + off + 0x04);
    if (!ImagePtrOk(Be32(image + off + 0x08), imageSize)) return SIR_ERR_UI;
    if (kind == UI_FONT_KIND_0) return PlanAdd(plan, off, UI_FONT_OBJ_SIZE_KIND0, REGION_UI_FONT_OBJ) ? SIR_OK : SIR_ERR_UI;
    if (kind != UI_FONT_KIND_1 || !ImageSpan(addr, UI_FONT_OBJ_SIZE_KIND1, imageSize, &off)) return SIR_ERR_UI;
    u32 pages = Be32(image + off + 0x124);
    if (pages > UI_FONT_PAGES) return SIR_ERR_UI;
    for (u32 i = 0; i < pages; i++)
        if (!ImagePtrOk(Be32(image + off + 0x24 + i * 4), imageSize)) return SIR_ERR_UI;
    u32 data = Be32(image + off + 0x08);
    if (data != 0) {
        SirError e = UiFontDataPlan(plan, image, imageSize, data);
        if (e != SIR_OK) return e;
    }
    return PlanAdd(plan, off, UI_FONT_OBJ_SIZE_KIND1, REGION_UI_FONT_OBJ) ? SIR_OK : SIR_ERR_UI;
}

static void UiFontObjConvert(SirReloc* r, u8* obj, u32 size) {
    if (size == UI_FONT_OBJ_SIZE_KIND1) {
        u32 pages = Be32(obj + 0x124);
        for (u32 i = 0; i < pages; i++) ConvertPtr(r, obj + 0x24 + i * 4);
        SirConvertRecord(r, obj, size, kUiFontObjPages, COUNTOF(kUiFontObjPages));
    }
    SirConvertRecord(r, obj, size, kUiFontObjFields, COUNTOF(kUiFontObjFields));
}

static SirError UiRootPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, UI_ROOT_SIZE, imageSize, &off) || !PlanAdd(plan, off, UI_ROOT_SIZE, REGION_UI_ROOT)) return SIR_ERR_UI;
    for (u32 i = 0; i < UI_ROOT_FONTS; i++) {
        u32 font = Be32(image + off + 0x14 + i * 4), fontOff;
        if (font == 0) continue;
        if (!ImageSpan(font, UI_FONT_SIZE, imageSize, &fontOff)) return SIR_ERR_UI;
        if (PlanHas(plan, fontOff, REGION_UI_FONT)) continue;
        if (!PlanAdd(plan, fontOff, UI_FONT_SIZE, REGION_UI_FONT)) return SIR_ERR_UI;
        u32 obj = Be32(image + fontOff + 0xCC);
        if (obj == 0) continue;
        SirError e = UiFontObjPlan(plan, image, imageSize, obj);
        if (e != SIR_OK) return e;
    }
    return SIR_OK;
}

static SirError UiFontListPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, UI_FONT_LIST_SIZE, imageSize, &off) || !PlanAdd(plan, off, UI_FONT_LIST_SIZE, REGION_UI_FONT_LIST)) return SIR_ERR_UI;
    for (u32 i = 0; i < 2; i++) {
        u32 obj = Be32(image + off + 0x04 + i * 4);
        if (obj == 0) continue;
        SirError e = UiFontObjPlan(plan, image, imageSize, obj);
        if (e != SIR_OK) return e;
    }
    return SIR_OK;
}

static SirError UiDrawPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, UI_DRAW_SIZE, imageSize, &off) || !ImagePtrOk(Be32(image + off + 0x08), imageSize)) return SIR_ERR_UI;
    return PlanAdd(plan, off, UI_DRAW_SIZE, REGION_UI_DRAW) ? SIR_OK : SIR_ERR_UI;
}

static SirError UiPadPlan(ImagePlan* plan, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, UI_PAD_SIZE, imageSize, &off)) return SIR_ERR_UI;
    return PlanAdd(plan, off, UI_PAD_SIZE, REGION_UI_PAD) ? SIR_OK : SIR_ERR_UI;
}

// ---- screen state machines (header slots 0xD98 and 0xD9C)
//
// A state machine (0x70 bytes in the file) points at its definition and keeps the run-time state.
// HudUpdateScreens (0x801BB5BC) steps it; fn_801BB478 resets it.
static const SirField kFsmFields[] = {
    { 0x00, SIR_PTR, 1 },     // code: definition
    { 0x04, SIR_U32, 3 },     // code: s32 current, previous and pending state (-1)
    { 0x10, SIR_PTR, 1 },     // code: owner, handed to TriggerDoAction (fn_801BBE40)
    { 0x14, SIR_U32, 6 },     // code: event arguments (+0x14, +0x18: fn_801BBB68), +0x1C, +0x20 (fn_801BBF24),
                              //       +0x24 (RequestComplete), flags +0x28
    { 0x2C, SIR_F32, 10 },    // code: fn_801BB478 (+0x2C..+0x48), timer +0x4C, fn_801BBFD0 (+0x50)
    { 0x54, SIR_PTR, 1 },     // code: value table {u32 count, u32 value[count]} (fn_801BBE40)
};
// +0x58, +0x5C (0) and +0x60 (-1) have no reader in the decompiled code and are left.

// Definition. fn_801BC5F0 walks the states and nodes; fn_801BC6F0 the shared events. The words
// at +0x08, +0x14, +0x18, +0x24, +0x28 (sizes next to the three arrays) and everything behind
// the byte at +0x44 have no reader and are left.
static const SirField kFsmDefFields[] = {
    { 0x00, SIR_PTR, 1 },     // code: states (0x30 bytes each)
    { 0x04, SIR_U32, 1 },     // code: s32 state count
    { 0x0C, SIR_PTR, 1 },     // code: fn_801BBE40 copies it to gStringTable2 (0 in every definition)
    { 0x10, SIR_PTR, 1 },     // code: node pointers
    { 0x1C, SIR_U32, 1 },     // code: s32 node count
    { 0x20, SIR_PTR, 1 },     // code: shared events (0xC bytes each)
    { 0x2C, SIR_U32, 1 },     // code: s32 shared event count
    { 0x30, SIR_U32, 1 },     // code: key (fn_8001B044)
    { 0x34, SIR_U32, 3 },     // the start node, a node of type 2: build-time vtable address (fn_801BC574 stores the
                              // real one), key, type (fn_801BBC9C reads the type of a transition's target)
    { 0x40, SIR_PTR, 1 },     // code: its transition (fn_801BC790)
    // 0x44: u8 (fn_801BB530, fn_801BB57C)
};

// State (0x30 bytes). +0x24 and +0x28 have no reader.
static const SirField kFsmStateFields[] = {
    { 0x00, SIR_U32, 3 },     // build-time vtable address (fn_801BC5F0 stores the real one); code: key, type (1)
    { 0x0C, SIR_U32, 3 },     // code: action words: enter (fn_801BBFD0), update (fn_801BC098), exit (fn_801BC12C)
    { 0x18, SIR_F32, 1 },     // code: time-out, copied to the machine's timer on enter (fn_801BBFD0)
    { 0x1C, SIR_U32, 1 },     // code: flags (HudUpdateScreens)
    { 0x20, SIR_PTR, 1 },     // code: events (fn_801BC1C0, fn_801BB8A0)
    { 0x2C, SIR_U32, 1 },     // code: s32 event count
};
// Event of a state (fn_801BB8A0): id, condition word, transition.
static const SirField kFsmEventFields[] = {
    { 0x00, SIR_U32, 2 }, { 0x08, SIR_PTR, 1 },
};
// Shared event of a definition (fn_801BC6F0, fn_801BB8A0): id, transition, condition word.
static const SirField kFsmSharedEventFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_PTR, 1 }, { 0x08, SIR_U32, 1 },
};

// Node (0x30 bytes), by the type word at +0x08. fn_801BC5F0 stores the vtable for the type.
static const SirField kFsmNodeHead[] = {
    { 0x00, SIR_U32, 3 },     // build-time vtable address; code: key (fn_801BC4C4), type
};
static const SirField kFsmNodeType2[] = {
    { 0x0C, SIR_PTR, 1 },     // code: transition (fn_801BC790)
};
static const SirField kFsmNodeType3[] = {
    { 0x0C, SIR_U32, 1 },     // code: action word (fn_801BBF24)
    { 0x10, SIR_PTR, 1 },     // code: transition (fn_801BBFC8)
    { 0x14, SIR_U32, 2 },     // code: fn_801BBF24 copies them to the machine's +0x1C and +0x20
};
static const SirField kFsmNodeType4[] = {
    { 0x0C, SIR_U32, 1 },     // code: action word; its result picks the transition (fn_801BC20C)
    { 0x10, SIR_PTR, 1 },     // code: transition pointers (fn_801BC2B8)
    { 0x14, SIR_U32, 1 },     // code: s32 transition count
    { 0x1C, SIR_U32, 2 },     // code: result, last choice
};

// Transition (0x20 bytes; +0x18 and +0x1C have no reader in this class).
static const SirField kFsmTransitionFields[] = {
    { 0x00, SIR_U32, 1 },     // build-time vtable address; nothing stores a vtable here and no code calls through it
    { 0x04, SIR_PTR, 1 },     // code: target state or node (fn_801BBC9C)
    { 0x08, SIR_U32, 2 },     // code: condition word (fn_801BC41C), action word (fn_801BC350)
    { 0x10, SIR_U32, 2 },     // code: +0x10 (fn_801BBC9C), +0x14 (fn_801BC350)
};
// Transition of a shared event (0x30 bytes): the same, then the states it does or does not apply to.
static const SirField kFsmSharedTransitionFields[] = {
    { 0x00, SIR_U32, 1 },
    { 0x04, SIR_PTR, 1 },
    { 0x08, SIR_U32, 2 },
    { 0x10, SIR_U32, 2 },
    { 0x18, SIR_U32, 1 },     // code: bit 0: the list names the states it does not apply to (fn_801BC4C4)
    { 0x1C, SIR_PTR, 1 },     // code: state keys
    { 0x20, SIR_U32, 1 },     // code: s32 key count
};

// HUD (header slot 0xD9C): the state machines of the game's screens.
static const SirField kHudFields[] = {
    { 0x00, SIR_U32, 1 },     // code: s32 depth of the stack below (fn_8001B128; HudInit clears it)
    { 0x04, SIR_PTR, 8 },     // code: stack of state machines (fn_8001B128)
    { 0x24, SIR_U32, 1 },     // code: HudInit stores the address of the screen-list slot (the file has a build-time address)
    { 0x28, SIR_PTR, HUD_MACHINES },   // code: HudReset, fn_8001B044
    { 0x428, SIR_U32, 1 },    // code: machine count
    { 0x42C, SIR_PTR, 1 },    // code: HudShowConversation
    // 0x430: u8 (UiUpdate)
};

// HUD of a level file (0x438 bytes): the same, with the screen-list slot inside it.
static const SirField kLevelHudFields[] = {
    { 0x00, SIR_U32, 1 },
    { 0x04, SIR_PTR, 8 },
    { 0x24, SIR_PTR, 1 },     // code: address of the screen-list slot (fn_8001B128 reads through it)
    { 0x28, SIR_PTR, HUD_MACHINES },
    { 0x428, SIR_U32, 1 },
    { 0x42C, SIR_PTR, 1 },
    { 0x434, SIR_PTR, 1 },    // code: the screen list (HudReload)
};

static SirError FsmTransitionPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, bool shared) {
    u32 off, size = shared ? FSM_SHARED_TRANSITION_USED : FSM_TRANSITION_USED;
    if (!ImageSpan(addr, size, imageSize, &off) || !ImagePtrOk(Be32(image + off + 0x04), imageSize)) return SIR_ERR_SCREENS;
    if (shared) {
        u32 keys = Be32(image + off + 0x20), keysOff;
        if (keys != 0) {
            if (!ImageSpan(Be32(image + off + 0x1C), (u64)keys * 4, imageSize, &keysOff) ||
                !PlanAdd(plan, keysOff, keys * 4, REGION_U32_ARRAY)) return SIR_ERR_SCREENS;
        } else if (!ImagePtrOk(Be32(image + off + 0x1C), imageSize)) {
            return SIR_ERR_SCREENS;
        }
    }
    return PlanAdd(plan, off, size, shared ? REGION_FSM_SHARED_TRANSITION : REGION_FSM_TRANSITION) ? SIR_OK : SIR_ERR_SCREENS;
}

// A NULL transition is accepted (the plan has nothing to convert for it).
static SirError FsmTransitionPlanOpt(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, bool shared) {
    return addr == 0 ? SIR_OK : FsmTransitionPlan(plan, image, imageSize, addr, shared);
}

static SirError FsmNodePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, FSM_NODE_SIZE, imageSize, &off)) return SIR_ERR_SCREENS;
    const u8* node = image + off;
    SirError e = SIR_OK;
    switch (Be32(node + 0x08)) {
    case FSM_NODE_PLAIN:
        break;
    case FSM_NODE_START:
        e = FsmTransitionPlanOpt(plan, image, imageSize, Be32(node + 0x0C), false);
        break;
    case FSM_NODE_ACTION:
        e = FsmTransitionPlanOpt(plan, image, imageSize, Be32(node + 0x10), false);
        break;
    case FSM_NODE_CHOICE: {
        u32 count = Be32(node + 0x14), arrayOff;
        if (count == 0) {
            if (!ImagePtrOk(Be32(node + 0x10), imageSize)) return SIR_ERR_SCREENS;
            break;
        }
        if (!ImageSpan(Be32(node + 0x10), (u64)count * 4, imageSize, &arrayOff) ||
            !PlanAdd(plan, arrayOff, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        for (u32 i = 0; i < count && e == SIR_OK; i++)
            e = FsmTransitionPlanOpt(plan, image, imageSize, Be32(image + arrayOff + i * 4), false);
        break;
    }
    default:
        return SIR_ERR_SCREENS;   // fn_801BC5F0 has no class for it (states, type 1, are not in the node array)
    }
    if (e != SIR_OK) return e;
    return PlanAdd(plan, off, FSM_NODE_SIZE, REGION_FSM_NODE) ? SIR_OK : SIR_ERR_SCREENS;
}

static void FsmNodeConvert(SirReloc* r, u8* node) {
    switch (Be32(node + 0x08)) {
    case FSM_NODE_START:  SirConvertRecord(r, node, FSM_NODE_SIZE, kFsmNodeType2, COUNTOF(kFsmNodeType2)); break;
    case FSM_NODE_ACTION: SirConvertRecord(r, node, FSM_NODE_SIZE, kFsmNodeType3, COUNTOF(kFsmNodeType3)); break;
    case FSM_NODE_CHOICE: SirConvertRecord(r, node, FSM_NODE_SIZE, kFsmNodeType4, COUNTOF(kFsmNodeType4)); break;
    }
    SirConvertRecord(r, node, FSM_NODE_SIZE, kFsmNodeHead, COUNTOF(kFsmNodeHead));
}

static SirError FsmDefPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, FSM_DEF_USED, imageSize, &off)) return SIR_ERR_SCREENS;
    if (PlanHas(plan, off, REGION_FSM_DEF)) return SIR_OK;
    if (!PlanAdd(plan, off, FSM_DEF_USED, REGION_FSM_DEF)) return SIR_ERR_SCREENS;
    const u8* def = image + off;
    SirError e;

    u32 states = Be32(def + 0x04), statesOff;
    if (states != 0) {
        if (!ImageSpan(Be32(def + 0x00), (u64)states * FSM_STATE_SIZE, imageSize, &statesOff) ||
            !PlanAdd(plan, statesOff, states * FSM_STATE_SIZE, REGION_FSM_STATES)) return SIR_ERR_SCREENS;
        for (u32 i = 0; i < states; i++) {
            const u8* state = image + statesOff + i * FSM_STATE_SIZE;
            u32 events = Be32(state + 0x2C), eventsOff;
            if (events == 0) {
                if (!ImagePtrOk(Be32(state + 0x20), imageSize)) return SIR_ERR_SCREENS;
                continue;
            }
            if (!ImageSpan(Be32(state + 0x20), (u64)events * FSM_EVENT_SIZE, imageSize, &eventsOff) ||
                !PlanAdd(plan, eventsOff, events * FSM_EVENT_SIZE, REGION_FSM_EVENTS)) return SIR_ERR_SCREENS;
            for (u32 k = 0; k < events; k++) {
                e = FsmTransitionPlanOpt(plan, image, imageSize, Be32(image + eventsOff + k * FSM_EVENT_SIZE + 0x08), false);
                if (e != SIR_OK) return e;
            }
        }
    } else if (!ImagePtrOk(Be32(def + 0x00), imageSize)) {
        return SIR_ERR_SCREENS;
    }

    u32 nodes = Be32(def + 0x1C), nodesOff;
    if (nodes != 0) {
        if (!ImageSpan(Be32(def + 0x10), (u64)nodes * 4, imageSize, &nodesOff) ||
            !PlanAdd(plan, nodesOff, nodes * 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        for (u32 i = 0; i < nodes; i++) {
            e = FsmNodePlan(plan, image, imageSize, Be32(image + nodesOff + i * 4));   // fn_801BC5F0 reads every node: none may be NULL
            if (e != SIR_OK) return e;
        }
    } else if (!ImagePtrOk(Be32(def + 0x10), imageSize)) {
        return SIR_ERR_SCREENS;
    }

    u32 shared = Be32(def + 0x2C), sharedOff;
    if (shared != 0) {
        if (!ImageSpan(Be32(def + 0x20), (u64)shared * FSM_EVENT_SIZE, imageSize, &sharedOff) ||
            !PlanAdd(plan, sharedOff, shared * FSM_EVENT_SIZE, REGION_FSM_SHARED_EVENTS)) return SIR_ERR_SCREENS;
        for (u32 i = 0; i < shared; i++) {
            e = FsmTransitionPlanOpt(plan, image, imageSize, Be32(image + sharedOff + i * FSM_EVENT_SIZE + 0x04), true);
            if (e != SIR_OK) return e;
        }
    } else if (!ImagePtrOk(Be32(def + 0x20), imageSize)) {
        return SIR_ERR_SCREENS;
    }

    if (!ImagePtrOk(Be32(def + 0x0C), imageSize)) return SIR_ERR_SCREENS;
    return FsmTransitionPlanOpt(plan, image, imageSize, Be32(def + 0x40), false);
}

static SirError FsmPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, FSM_USED, imageSize, &off)) return SIR_ERR_SCREENS;
    if (PlanHas(plan, off, REGION_FSM)) return SIR_OK;
    const u8* fsm = image + off;
    if (!ImagePtrOk(Be32(fsm + 0x10), imageSize) || !PlanAdd(plan, off, FSM_USED, REGION_FSM)) return SIR_ERR_SCREENS;

    u32 values = Be32(fsm + 0x54), valuesOff;
    if (values != 0 && sLevelZone && !ImageSpan(values, 4, imageSize, &valuesOff)) {
        // A level's machines keep their value tables in another zone's image: the pointer is
        // converted here, the table when that zone is (OtherFsmValuesPlan).
    } else if (values != 0) {
        if (!ImageSpan(values, 4, imageSize, &valuesOff)) return SIR_ERR_SCREENS;
        u32 count = Be32(image + valuesOff);
        if (!ImageSpan(values, 4 + (u64)count * 4, imageSize, &valuesOff) ||
            !PlanAdd(plan, valuesOff, 4 + count * 4, REGION_U32_ARRAY)) return SIR_ERR_SCREENS;
    }
    u32 def = Be32(fsm + 0x00);
    return def == 0 ? SIR_OK : FsmDefPlan(plan, image, imageSize, def);
}

// The value tables that zone 0's machines keep in the zone being converted ({count, action words}:
// a choice node's or a state's action word of type 3 is an index into it, fn_801BC20C). Zone 0 is
// converted, so the machine's pointer at +0x54 is a host pointer.
static SirError OtherFsmValuesPlan(ImagePlan* plan, const u8* image, u32 imageSize) {
    for (u32 i = 0; sOtherDone != NULL && i < sOtherDone->count; i++) {
        const Region* g = &sOtherDone->regions[i];
        if (g->kind != REGION_FSM || g->size < FSM_USED) continue;
        const u8* values;
        memcpy(&values, sOtherImage + g->off + 0x54, sizeof(values));
        // A converted pointer leads into this image where it is now, or is the address it will have (AnyPtr).
        u32 addr = values >= image && values < image + imageSize ? sImageBase + (u32)(values - image) : (u32)(uintptr_t)values, off;
        if (!ImageSpan(addr, 4, imageSize, &off)) continue;
        if (PlanHas(plan, off, REGION_U32_ARRAY)) continue;        // two machines with one table
        u32 count = Be32(image + off);
        if ((u64)off + 4 + (u64)count * 4 > imageSize || !PlanAdd(plan, off, 4 + count * 4, REGION_U32_ARRAY)) return SIR_ERR_SCREENS;
    }
    return SIR_OK;
}

// `level`: the HUD of a level file. It carries the screen-list slot itself, at +0x434, and +0x24
// already points at that slot (in global_binary.sir HudInit stores the address of a header slot).
static SirError HudPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, bool level) {
    u32 off, size = level ? LEVEL_HUD_SIZE : HUD_SIZE;
    if (!ImageSpan(addr, size, imageSize, &off)) return SIR_ERR_SCREENS;
    const u8* hud = image + off;
    u32 count = Be32(hud + 0x428);
    if (count > HUD_MACHINES || !PlanAdd(plan, off, size, level ? REGION_LEVEL_HUD : REGION_HUD)) return SIR_ERR_SCREENS;
    if (level) {
        u32 list = Be32(hud + 0x434);
        if (!ImagePtrOk(Be32(hud + 0x24), imageSize)) return SIR_ERR_SCREENS;
        if (list != 0) {
            SirError e = FsmPlan(plan, image, imageSize, list);
            if (e != SIR_OK) return e;
        }
    }
    for (u32 i = 0; i < 8; i++)
        if (!ImagePtrOk(Be32(hud + 0x04 + i * 4), imageSize)) return SIR_ERR_SCREENS;
    if (!ImagePtrOk(Be32(hud + 0x42C), imageSize)) return SIR_ERR_SCREENS;
    for (u32 i = 0; i < HUD_MACHINES; i++) {
        u32 fsm = Be32(hud + 0x28 + i * 4);
        if (i >= count) {   // unused slots: nothing reads them, but they are converted as pointers
            if (!ImagePtrOk(fsm, imageSize)) return SIR_ERR_SCREENS;
            continue;
        }
        if (fsm == 0) continue;
        SirError e = FsmPlan(plan, image, imageSize, fsm);
        if (e != SIR_OK) return e;
    }
    return SIR_OK;
}

static const SirField kPoolTableEntryFields[5] = {
    { 0x00, SIR_U32, 1 },     // code: the pool's name hash (HashTableFind: SpawnHelper finds its template by it)
    { 0x04, SIR_PTR, 1 },     // code: pool (fn_801DB1AC -> fn_801EBC74, fn_801EBBE0)
    { 0x0C, SIR_U32, 1 },     // code: s32 script function index, -1 for none
    { 0x10, SIR_U32, 1 },     // code: s32 index of the script function run when the zone is unloaded (fn_801DB248)
    { 0x1C, SIR_U32, 1 },     // code: entity type FixUpEntityCallback passes to fn_8012F8B8
};

static void ConvertRecords(SirReloc* r, u8* p, u32 size, u32 recSize, const SirField* fields, s32 n) {
    for (u32 o = 0; o + recSize <= size; o += recSize) SirConvertRecord(r, p + o, recSize, fields, n);
}

// ---- action records (what the action table's pointers lead to)
//
// A record is a program for the interpreter fn_801E1BD4 (0x801E1BD4): 32-bit words, each
// instruction one word {opcode bits 0-7, length in words bits 8-15, bits 16-27 a function index
// or a jump distance in words} followed by length - 1 operand words; opcode 0xFF ends it. Every
// operand is a 32-bit value: an int (opcode 18), a float (19), an action word (23), or the
// arguments of a script function (22), which the interpreter copies as words and hands to
// ScriptDispatchEngine. So the whole program is converted as u32 words.
//
// Two things the loader cannot see, because they depend on the script function table, which the
// game fills after this file is loaded (docs/loader.md, "Action records"):
// - a function whose signature has an argument of type 0xA (Print) has bytes behind its
//   instruction word, not words. The shipped file has no such call.
// - arguments of type 0xB and 0xD are pointers into the image (42 in the shipped file). They are
//   converted as words, which is right only while the image sits at its original address, as it
//   does in the PC build. The objects they point at are sound objects (kSirSoundCalls below).
#define ACTION_OP_HEADER    0x01   // fn_801E2368: a record that does not start with it is not run
#define ACTION_OP_JUMP_MIN  0x04   // 4 jump, 5 jump if the result is zero, 6-17 compare and jump
#define ACTION_OP_JUMP_MAX  0x11
#define ACTION_OP_PUSH_INT  0x12
#define ACTION_OP_PUSH_F32  0x13
#define ACTION_OP_CALL      0x16
#define ACTION_OP_ACTION    0x17
#define ACTION_OP_LAST      0x1B   // opcodes above it (except the end) are skipped as one word
#define ACTION_OP_END       0xFF
#define ACTION_CALL_ARGS    8      // the interpreter's argument buffer: the instruction word and 8 arguments

static inline u32 ActionOp(u32 word)   { return word & 0xFF; }
static inline u32 ActionLen(u32 word)  { return (word >> 8) & 0xFF; }
static inline u32 ActionJump(u32 word) { return (word >> 16) & 0xFFF; }   // fn_801E239C

// Words an instruction occupies; 0 if the interpreter could not step over it.
static u32 ActionInstructionWords(u32 word) {
    u32 op = ActionOp(word), len = ActionLen(word);
    if (op > ACTION_OP_LAST) return 1;
    if (len == 0) return 0;
    if (op == ACTION_OP_CALL && len > 1 + ACTION_CALL_ARGS) return 0;
    if ((op == ACTION_OP_PUSH_INT || op == ACTION_OP_PUSH_F32 || op == ACTION_OP_ACTION) && len < 2) return 0;
    return len;
}

// ---- sound objects (what a sound argument of a script call points at)
//
// A script function whose signature has an argument of type 0xB or 0xD (a voice line: PlayVoiceSFX
// and LT_SetTutorialText hand it to PlaySoundAt) gets a pointer to a sound object:
// one or more 0x30-byte variations (PlaySoundAt picks one at random; the count is the byte at +0x04
// of the first, 0 meaning one). A variation names its sample by a pointer to a sound info, which
// names a DSP header (the layout of the first 0x60 bytes of a .dsp file) with the sample data behind
// it. The functions are known by their index in gScriptFuncs (kSirSoundCalls: the registration order
// of the retail build, which the port checks against the live table).
//   variation: +0x00 info (PlaySoundAt), +0x04 count, +0x06 volume, +0x07 volume spread (bytes),
//              +0x08 pitch, +0x10 float (PlaySoundAt), +0x14 kind, +0x15 / +0x16 distances, +0x17
//              last pick, +0x2D / +0x2E flags (bytes); +0x0C and +0x18..+0x2B have no reader yet
//              and are words in the shipped files (0, a hash, zeros)
//   info:      +0x00 word and +0x04 DSP header (fn_801FE52C: both 0 = not loaded yet), +0x0C u16
//              (PlaySoundAt), +0x0E s16 zone (fn_801FF154), +0x18 name (bytes); +0x08 a word (size)
//   DSP header: fn_80203340
extern "C" const SirSoundCall kSirSoundCalls[SIR_SOUND_CALLS] = {
    {   48, 0x1, "PlayGlobalSound" },        {   49, 0x1, "PlayGlobalSoundSynced" },  {   50, 0x1, "PlayGlobalSoundWithEffect" },
    {   51, 0x2, "PlayLoopingSoundFX" },     {   52, 0x2, "PlayLoopingSoundFXWithEffect" },
    {   55, 0x2, "PlaySoundFX" },            {   56, 0x2, "PlaySoundFXWithEffect" },
    {   59, 0x1, "PlayVoiceSFX" },           {  103, 0x1, "StopVoiceSFX" },           { 1015, 0x2, "LT_SetTutorialText" },
    { 1216, 0x3, "PlayBackgroundSFX" },      { 1217, 0x3, "PlayBackgroundSFXWithEffect" },
    { 1238, 0x1, "PlaySoundEffect" },        { 1239, 0x1, "PlaySoundEffectWithEffect" },
    { 1297, 0x2, "QueueSoundFX" },           { 1330, 0x2, "RegisterExperimentSound" }, { 1332, 0x1, "RegisterMidiLoop" },
    { 1684, 0x3, "StopBackgroundSFX" },      { 1693, 0x1, "StopSoundEffect" },
};

#define SOUND_SIZE       0x30
#define SOUND_INFO_USED  0x18
#define DSP_HEADER_SIZE  0x60

static const SirField kSoundFields[] = {
    { 0x00, SIR_PTR, 1 },     // code: the sound info (PlaySoundAt)
    { 0x08, SIR_F32, 1 },     // code: pitch (PlaySoundAt)
    { 0x0C, SIR_U32, 1 },     // data: 0
    { 0x10, SIR_F32, 1 },     // code: float handed on with the sound (PlaySoundAt)
    { 0x18, SIR_U32, 5 },     // data: a hash, zeros
};
static const SirField kSoundGroupFields[] = {
    { 0x00, SIR_U32, 1 },     // code: the group's name hash (PlaySound)
    { 0x04, SIR_PTR, 10 },    // code: the variations, sound objects (PlaySound; unused entries are 0)
    { 0x2C, SIR_U32, 2 },     // code: variation count, last pick (PlaySound)
    { 0x34, SIR_PTR, 1 },     // code: next group in the chain
};
static const SirField kSoundInfoFields[] = {
    { 0x00, SIR_PTR, 2 },     // code: word and DSP header, both tested against 0 (fn_801FE52C)
    { 0x08, SIR_U32, 1 },     // data: a size
    { 0x0C, SIR_U16, 2 },     // code: u16 (PlaySoundAt), s16 zone (fn_801FF154)
    { 0x10, SIR_U32, 2 },     // data: zeros
};
static const SirField kDspHeaderFields[] = {
    { 0x00, SIR_U32, 3 },     // code: samples, nibbles, sample rate (fn_80203340 reads the rate)
    { 0x0C, SIR_U16, 2 },     // code: loop flag, format (fn_80203340)
    { 0x10, SIR_U32, 3 },     // code: loop start, end and current address (fn_80203340)
    { 0x1C, SIR_U16, 34 },    // code: coefficients, gain, predictor and history, loop state (fn_80203340)
};

static SirError SoundPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off, info, dsp;
    if (addr == 0 || !ImageSpan(addr, SOUND_SIZE, imageSize, &off)) return SIR_OK;   // none, or in another image
    if (PlanHas(plan, off, REGION_SOUND)) return SIR_OK;
    u32 count = image[off + 4] != 0 ? image[off + 4] : 1;
    if ((u64)off + (u64)count * SOUND_SIZE > imageSize) return SIR_ERR_ACTIONS;
    if (!PlanAdd(plan, off, count * SOUND_SIZE, REGION_SOUND)) return SIR_ERR_ACTIONS;
    for (u32 i = 0; i < count; i++) {
        if (!ImageSpan(Be32(image + off + i * SOUND_SIZE), SOUND_INFO_USED, imageSize, &info) || PlanHas(plan, info, REGION_SOUND_INFO)) continue;
        if (!PlanAdd(plan, info, SOUND_INFO_USED, REGION_SOUND_INFO)) return SIR_ERR_ACTIONS;
        if (!ImageSpan(Be32(image + info + 4), DSP_HEADER_SIZE, imageSize, &dsp) || PlanHas(plan, dsp, REGION_DSP_HEADER)) continue;
        if (!PlanAdd(plan, dsp, DSP_HEADER_SIZE, REGION_DSP_HEADER)) return SIR_ERR_ACTIONS;
    }
    return SIR_OK;
}

// A zone's chain of sound groups (slot +0x110, PlaySound), SOUND_GROUP_SIZE bytes each.
#define SOUND_GROUP_SIZE 0x38
static SirError SoundGroupPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    while (ImageSpan(addr, SOUND_GROUP_SIZE, imageSize, &off) && !PlanHas(plan, off, REGION_SOUND_GROUP)) {
        if (!PlanAdd(plan, off, SOUND_GROUP_SIZE, REGION_SOUND_GROUP)) return SIR_ERR_ACTIONS;
        for (u32 i = 0; i < 10; i++) {
            SirError e = SoundPlan(plan, image, imageSize, Be32(image + off + 4 + i * 4));
            if (e != SIR_OK) return e;
        }
        addr = Be32(image + off + 0x34);
    }
    return SIR_OK;
}

// The sound objects the calls of one checked program name.
static SirError ActionSoundsPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 off, u32 words) {
    for (u32 i = 0; i < words; i += ActionInstructionWords(Be32(image + off + i * 4))) {
        u32 word = Be32(image + off + i * 4);
        if (ActionOp(word) != ACTION_OP_CALL) continue;
        for (u32 c = 0; c < SIR_SOUND_CALLS; c++) {
            if (kSirSoundCalls[c].func != ActionJump(word)) continue;
            for (u32 a = 0; a + 1 < ActionLen(word); a++) {
                if (!(kSirSoundCalls[c].argMask >> a & 1)) continue;
                SirError e = SoundPlan(plan, image, imageSize, Be32(image + off + (i + 1 + a) * 4));
                if (e != SIR_OK) return e;
            }
        }
    }
    return SIR_OK;
}

// Checks one program and records it. Walks the instructions in file order to the end word, then
// checks that every jump lands on an instruction of the same program.
static SirError ActionRecordPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, 4, imageSize, &off)) return SIR_ERR_ACTIONS;
    if (ActionOp(Be32(image + off)) != ACTION_OP_HEADER)      // never run: only its first word is read
        return PlanAdd(plan, off, 4, REGION_U32_ARRAY) ? SIR_OK : SIR_ERR_ACTIONS;

    u32 words = 0;
    for (;;) {
        if ((u64)off + ((u64)words + 1) * 4 > imageSize) return SIR_ERR_ACTIONS;
        u32 word = Be32(image + off + words * 4);
        if (ActionOp(word) == ACTION_OP_END) break;
        u32 n = ActionInstructionWords(word);
        if (n == 0) return SIR_ERR_ACTIONS;
        words += n;
    }
    u32 total = words + 1;   // with the end word
    if ((u64)off + (u64)total * 4 > imageSize) return SIR_ERR_ACTIONS;

    u8* starts = (u8*)calloc(total, 1);
    if (starts == NULL) return SIR_ERR_ACTIONS;
    for (u32 i = 0; i < words; i += ActionInstructionWords(Be32(image + off + i * 4))) starts[i] = 1;
    starts[words] = 1;
    bool ok = true;
    for (u32 i = 0; i < words && ok; i += ActionInstructionWords(Be32(image + off + i * 4))) {
        u32 word = Be32(image + off + i * 4), op = ActionOp(word);
        if (op < ACTION_OP_JUMP_MIN || op > ACTION_OP_JUMP_MAX) continue;
        u32 target = i + ActionJump(word);
        ok = target < total && starts[target];
    }
    free(starts);
    if (!ok) return SIR_ERR_ACTIONS;
    SirError e = ActionSoundsPlan(plan, image, imageSize, off, words);
    if (e != SIR_OK) return e;
    return PlanAdd(plan, off, total * 4, REGION_U32_ARRAY) ? SIR_OK : SIR_ERR_ACTIONS;
}

// Every record of the action table at `addr` (already checked by PtrArrayPlan).
static SirError ActionRecordsPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off = addr - sImageBase;
    u32 count = Be32(image + off + 0x00);
    u32 array = Be32(image + off + 0x04) - sImageBase;
    for (u32 i = 0; i < count; i++) {
        u32 rec = Be32(image + array + i * 4);
        if (rec == 0) continue;
        SirError e = ActionRecordPlan(plan, image, imageSize, rec);
        if (e != SIR_OK) return e;
    }
    return SIR_OK;
}

// ---- effect definitions (header slots 0x82C-0x840) and the two pools of slots 0x10 and 0x418
//
// fn_80007D68 (the start of a level load) hands both tables to their managers: {count 0x82C,
// entries 0x830, sorted flag 0x834} to lbl_8047C790 and {0x838, 0x83C, 0x840} to lbl_8047C7AC. It
// stores the manager in every entry and sets up the entry's pools with fn_801EB908. An effect
// instance keeps its definition at +0, so the fields are read as *(*(node) + off).

// Entry of the first table (0x70 bytes; "lightning_scrying", ...). The name at +0x08 is text.
// +0x04 is read byte by byte (the colour, fn_801BEDBC) and is left; +0x58 and +0x60 have no reader
// that cares about their order (+0x60 is only tested against 0) and are left.
static const SirField kFxBoltFields[] = {
    { 0x00, SIR_U32, 1 },     // code: name hash, the search key (FindRecord in draft_miss346.cpp; sorted when the flag is set)
    { 0x28, SIR_PTR, 1 },     // code: texture (fn_801BF714)
    { 0x2C, SIR_U32, 3 },     // code: draw mode (fn_801BF714 -> fn_801CA7B0), +0x30, +0x34 (fn_801BDB6C)
    { 0x38, SIR_U32, 1 },     // code: deepest level that still gets branches (fn_801BEDBC; 2 for lightning_boss)
    { 0x3C, SIR_U32, 2 },     // code: fn_801BDB6C
    { 0x44, SIR_F32, 4 },     // code: fn_801BDB6C (+0x44, +0x48, +0x50), fn_801BD278, fn_801BE620, fn_801BCD88 (+0x48, +0x4C)
    { 0x54, SIR_F32, 1 },     // code: fn_801BF4B8
    { 0x5C, SIR_F32, 1 },     // code: fn_801BDB6C
    { 0x64, SIR_U32, 1 },     // code: fn_80007D68 stores the manager here (the file has a build-time address)
    { 0x68, SIR_PTR, 1 },     // code: pool of the instances (fn_80007D68, fn_801BE44C)
    { 0x6C, SIR_PTR, 1 },     // code: callback run when an instance ends (fn_801BE44C); NULL in the file
};

// Entry of the second table (0x80 bytes; "trail_plant", ...). +0x64 and +0x68 have no reader.
static const SirField kFxTrailFields[] = {
    { 0x00, SIR_U32, 2 },     // code: name hash (fn_801C3EA4); colour word (fn_801C5AAC reads, fn_801C3F6C writes)
    { 0x28, SIR_U32, 2 },     // code: fn_801C3F6C (+0x28), fn_801C5444 (+0x2C)
    { 0x30, SIR_F32, 7 },     // code: fn_801C5444 (+0x30, +0x34, +0x40..+0x48), fn_801C4338 (+0x38), fn_801C3F6C (+0x3C..+0x48)
    { 0x4C, SIR_U32, 1 },     // code: fn_801C3F6C, fn_801C3924 (which pool an instance comes from)
    { 0x50, SIR_F32, 1 },     // code: fn_801C4B90, fn_801C5444
    { 0x54, SIR_U32, 1 },     // code: fn_801C4338, fn_801C5444
    { 0x58, SIR_F32, 3 },     // code: fn_801C4338 (+0x58, +0x60), fn_801C5444 (+0x5C)
    { 0x6C, SIR_U32, 1 },     // code: fn_80007D68 stores the manager here (the file has a build-time address)
    { 0x70, SIR_PTR, 1 },     // code: texture (fn_801C5B0C, fn_801C4338)
    { 0x74, SIR_U32, 1 },     // code: draw mode (fn_801C5B0C -> fn_801CA7B0)
    { 0x78, SIR_PTR, 2 },     // code: the two pools of the instances (fn_80007D68, fn_801C3F6C)
};

// The memory an entry's pool pointer names: fn_801EB908 writes +0x00..+0x27 of it at level load
// before anything reads it. The file has fill words there and nothing is converted, but it is a
// region of its own, so no other object may share it.
static SirError FxPoolSlotPlan(ImagePlan* plan, u32 imageSize, u32 addr) {
    u32 off;
    if (addr == 0) return SIR_OK;   // fn_80007D68 skips a NULL pool
    if (!ImageSpan(addr, FX_POOL_HEAD_SIZE, imageSize, &off)) return SIR_ERR_EFFECTS;
    if (PlanHas(plan, off, REGION_RUNTIME)) return SIR_ERR_OVERLAP;   // two entries would share one pool
    return PlanAdd(plan, off, FX_POOL_HEAD_SIZE, REGION_RUNTIME) ? SIR_OK : SIR_ERR_EFFECTS;
}

static SirError FxTablePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 count, bool trail) {
    u32 off, size = trail ? FX_TRAIL_SIZE : FX_BOLT_SIZE;
    if (count == 0) return ImagePtrOk(addr, imageSize) ? SIR_OK : SIR_ERR_EFFECTS;
    if (!ImageSpan(addr, (u64)count * size, imageSize, &off) ||
        !PlanAdd(plan, off, count * size, trail ? REGION_FX_TRAILS : REGION_FX_BOLTS)) return SIR_ERR_EFFECTS;
    for (u32 i = 0; i < count; i++) {
        const u8* e = image + off + i * size;
        if (!ImagePtrOk(Be32(e + (trail ? 0x70 : 0x28)), imageSize)) return SIR_ERR_EFFECTS;
        SirError err = FxPoolSlotPlan(plan, imageSize, Be32(e + (trail ? 0x78 : 0x68)));
        if (err == SIR_OK && trail) err = FxPoolSlotPlan(plan, imageSize, Be32(e + 0x7C));
        if (err != SIR_OK) return err;
        if (!trail && !ImagePtrOk(Be32(e + 0x6C), imageSize)) return SIR_ERR_EFFECTS;
    }
    return SIR_OK;
}

// Object pool (fn_801EB7A4 builds one at run time; the two of header slots 0x10 and 0x418 come
// ready-made in the file). fn_801EB94C takes an item from the free list, fn_801EBA20 gives it back.
static const SirField kPoolFields[] = {
    { 0x00, SIR_U32, 4 },     // code: items per block, item size, alignment, items in use
    { 0x10, SIR_PTR, 4 },     // code: free list, used list, parent pool, block list
    // 0x20: u8: grow when empty
    { 0x24, SIR_PTR, 1 },     // code: block to use instead of allocating one (fn_801EBAB8)
    { 0x28, SIR_U32, 1 },     // code: s32, -2 selects the allocator (fn_801EBAB8)
};

// The block of a pool as fn_801EBAB8 lays it out: the next-block pointer, then, from the first
// address that puts an item on the pool's alignment, `count` nodes {prev, next, item}.
struct PoolLayout {
    u32 blockOff;      // in the image
    u32 firstOff;      // first node
    u32 stride;        // item size + 8
    u32 count;
};

static bool PoolLayoutRead(const u8* pool, u32 imageSize, PoolLayout* out) {
    u32 count = Be32(pool + 0x00), itemSize = Be32(pool + 0x04), align = Be32(pool + 0x08), block = Be32(pool + 0x24);
    if (count == 0 || count > POOL_MAX_ITEMS || itemSize == 0 || itemSize > 0x10000 || (itemSize & 3) != 0) return false;
    if (align < 4 || align > 0x1000 || (align & (align - 1)) != 0) return false;
    if (block < sImageBase || Be32(pool + 0x1C) != block) return false;
    u32 first = ((block + 4 + align + 7) & ~(align - 1)) - 8;
    out->stride = itemSize + 8;
    out->count = count;
    if (!ImageSpan(block, (u64)(first - block) + (u64)count * out->stride, imageSize, &out->blockOff)) return false;
    out->firstOff = first - sImageBase;
    return true;
}

// NULL or the address of one of the pool's nodes.
static bool PoolNodeOk(const PoolLayout* l, u32 addr) {
    if (addr == 0) return true;
    if (addr < sImageBase) return false;
    u32 off = addr - sImageBase;
    return off >= l->firstOff && (off - l->firstOff) % l->stride == 0 && (off - l->firstOff) / l->stride < l->count;
}

// `typedItems`: the items are objects that get regions of their own (a level's entity pools), so
// only the block's first word and the node headers are recorded for the block.
static SirError PoolPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, bool typedItems) {
    u32 off;
    PoolLayout l;
    if (!ImageSpan(addr, POOL_SIZE, imageSize, &off) || !PoolLayoutRead(image + off, imageSize, &l)) return SIR_ERR_POOL;
    const u8* pool = image + off;
    // The lists start at a node; the pool has one block and no parent.
    if (!PoolNodeOk(&l, Be32(pool + 0x10)) || !PoolNodeOk(&l, Be32(pool + 0x14)) || Be32(pool + 0x18) != 0 ||
        Be32(image + l.blockOff) != 0) return SIR_ERR_POOL;
    for (u32 i = 0; i < l.count; i++) {
        const u8* node = image + l.firstOff + i * l.stride;
        if (!PoolNodeOk(&l, Be32(node + 0)) || !PoolNodeOk(&l, Be32(node + 4))) return SIR_ERR_POOL;
    }
    if (!PlanAdd(plan, off, POOL_SIZE, REGION_POOL)) return SIR_ERR_POOL;
    if (typedItems) {
        if (!PlanAdd(plan, l.blockOff, 4, REGION_POOL_BLOCK)) return SIR_ERR_POOL;
        for (u32 i = 0; i < l.count; i++)
            if (!PlanAdd(plan, l.firstOff + i * l.stride, 8, REGION_POOL_BLOCK)) return SIR_ERR_POOL;
        return SIR_OK;
    }
    // The block is one region: its items are not converted, but the pool hands them out.
    return PlanAdd(plan, l.blockOff, (l.firstOff - l.blockOff) + l.count * l.stride, REGION_POOL_BLOCK) ? SIR_OK : SIR_ERR_POOL;
}

// Converts a pool and its block. The block is reached from the pool, so both are done here, when
// the pool's region comes up, while the pool's words are still big-endian.
static void PoolConvert(SirReloc* r, u8* image, u8* pool, u32 imageSize) {
    PoolLayout l;
    if (PoolLayoutRead(pool, imageSize, &l)) {
        ConvertPtr(r, image + l.blockOff);                       // next block
        for (u32 i = 0; i < l.count; i++) {
            u8* node = image + l.firstOff + i * l.stride;
            ConvertPtr(r, node + 0);                             // previous node
            ConvertPtr(r, node + 4);                             // next node
        }
    }
    SirConvertRecord(r, pool, POOL_SIZE, kPoolFields, COUNTOF(kPoolFields));
}

// ---- entities of a level zone
//
// An entity (prop, character, ...) is an object the file holds ready-made; the level start runs
// fn_8012F8B8 on each, which installs the vtable for its type and prepares what hangs off it.
// Only the fields that function and the constructors it calls read are converted so far
// (docs/loader.md, "Entities"): this is a first layer of a much larger object.
static const SirField kEntityFields[] = {
    { 0x000, SIR_U32, 1 },    // build-time vtable address; the constructor for the type stores the real one
    { 0x004, SIR_PTR, 1 },    // code: the entity's record (fn_8012F8B8, fn_801B4334)
    { 0x008, SIR_F32, 3 },    // code: scale; EntitySyncTransform hands +0x08 to the scene object's transform
    { 0x02C, SIR_U32, 2 },    // code: id, which fn_800D8724 compares for every entity of a zone's tables; +0x30 (fn_801B6444)
    { 0x080, SIR_U32, 1 },    // code: fn_801260C8 tests its bits 0x78000000 (0x0003FFFF in the files)
    { 0x084, SIR_PTR, 1 },    // code: a table fn_801B6AD4 searches (the table itself is not converted)
    { 0x014, SIR_F32, 1 },    // code: a prop's anchor height (fn_8006AADC); for every type the camera's sensor radius (fn_800DECD8
                              //       passes the camera target's +0x14 to the volume test: unconverted it made every camera volume apply)
    { 0x08C, SIR_PTR, 2 },    // code: part container (fn_8012F6B0), sub-object (fn_8012F8B8)
    { 0x094, SIR_U32, 1 },    // code: how many attach points the entity has (fn_801EC080; fn_801B4334 clears it),
    { 0x098, SIR_PTR, 1 },    //       the attach points (kAttachPointFields)
    { 0x09C, SIR_PTR, 1 },    // code: fn_8012F8B8 reads the id at +8 of it for type 6
    { 0x0A0, SIR_PTR, 1 },    // code: animation object (fn_801B4520 frees it; a prop's: fn_80069180, fn_8006A764)
    { 0x0A4, SIR_PTR, 1 },    // code: state machine holder (fn_8012F8B8)
    { 0x0AC, SIR_U32, 1 },    // code: fn_801B6554, fn_801B66EC
    { 0x0B0, SIR_PTR, 1 },    // code: list of child entities (fn_8012F8B8)
    { 0x0B8, SIR_U32, 1 },    // code: flags; UpdateZoneEntities updates an entity without bit 0
    { 0x0C8, SIR_F32, 3 },    // code: a position: EntityRefreshLinks tests it against the loaded zones' boxes before it
                              //       wakes an entity (vtable slot 0x30); a prop's: fn_8006A764
    { 0x0D4, SIR_F32, 3 },    // code: its rotation as Euler angles. Every entity that starts through fn_80062DC4 (characters,
                              //       props, trigger volumes) gets its record's position from +0xC8 and its quaternion from
                              //       these (fn_801B5BD8). Unconverted they read as 0: the enemy of a battle, turned half a
                              //       circle in the arena file, stood with its back to the player
    { 0x0EC, SIR_U32, 3 },    // code: the event that wakes a sleeping entity (fn_801B9F14; WorldTick compares it with
                              //       0x6C938971), two event ids (EventMgrFlushFor)
    { 0x158, SIR_F32, 1 },    // code: fn_800690D4
    { 0x194, SIR_PTR, 1 },    // code: more part containers (fn_8012F8B8),
    { 0x1A4, SIR_U32, 2 },    //       their count; +0x1A8 (fn_800690D4)
    { 0x1B0, SIR_U32, 1 },    // code: flags (fn_8012F8B8)
};
// What a type 9 entity (trigger_timed_script, fn_8006F6BC) has on top of kEntityFields: the action words at +0x50 / +0x54 (the
// thunder script of the castle is the first) and the time-outs
// SetTimeoutFromProperty (fn_8006E9D8) reads, base values +0x15C / +0x160 and the spread +0x170 / +0x174 (+0x158 is
// converted with every entity).
static const SirField kTimedFields[] = {
    { 0x050, SIR_U32, 2 },    // code: the activate / deactivate action words (CallActivateHandler, CallDeactivateHandler)
    { 0x15C, SIR_F32, 2 }, { 0x170, SIR_F32, 2 },
};
// An entity's attach points (+0x98, count +0x94): where effects hang on it. fn_801EC080 updates
// them every frame: +0x00 the point's name hash (fn_801EC880 finds a point by it), +0x04 the bone
// it follows (-1: the entity itself), +0x08 its matrix on that bone, +0x38 the resulting world
// matrix, +0x68 eight effect ids, +0x88 the eight running effects, +0xA8 eight started flags.
static const SirField kAttachPointFields[] = {
    { 0x00, SIR_U32, 2 }, { 0x08, SIR_F32, 24 }, { 0x68, SIR_U32, 8 }, { 0x88, SIR_PTR, 8 }, { 0xA8, SIR_U32, 8 },
};
#define ATTACH_POINT_SIZE 0xC8
// The record an entity points at (0x40 bytes): position, rotation, scale, layer and flags.
// fn_801B4334 resets all of it; the bytes at +0x34..+0x3D need no conversion.
static const SirField kEntityRecordFields[] = {
    { 0x00, SIR_F32, 7 },     // code: fn_801B4334 (two vectors of three, then 1.0)
    { 0x1C, SIR_U32, 2 },     // code: layer mask and flags (fn_8012F8B8, fn_801B4334)
    { 0x24, SIR_F32, 1 },     // code: fn_800690D4
    { 0x28, SIR_U32, 1 },     // code: s32; EntityRefreshLinks leaves an entity asleep when it is <= 0
    { 0x2C, SIR_U32, 1 },     // code: fn_801B4334
    { 0x30, SIR_U16, 2 },     // code: fn_801B4334; +0x32 picks the clip list of an animation table (fn_800699E4)
};
// What a prop (entity type 6, vtable lbl_8035A0B4) has on top of kEntityFields: the fields the
// methods of that vtable read. The level's player object is one of these. Action words are the
// ones TriggerDoAction gets the address of; the words between them have no reader yet.
static const SirField kPropFields[] = {
    { 0x028, SIR_F32, 1 },    // code: fn_8006BDF4
    { 0x038, SIR_U32, 3 },    // code: action words (fn_8006A2D0)
    { 0x048, SIR_U32, 4 },    // code: action words (fn_800696C4, fn_8006BDF4, fn_80069638)
    { 0x068, SIR_U32, 1 },    // code: action word (fn_80069180)
    { 0x15C, SIR_U32, 1 },    // code: flags (fn_80069180, fn_8006A764, fn_8006A2D0)
    { 0x1A0, SIR_PTR, 1 },    // code: the prop's states: +0x1A4 pointers to definitions (fn_800699E4)
};
// A prop's definition (entity +0x9C), read through the same methods.
static const SirField kPropDefFields[] = {
    { 0x08, SIR_U32, 1 },     // code: id (fn_8012F8B8, fn_80069180, fn_8006AADC)
    { 0x0C, SIR_U32, 4 },     // code: flags, animation id, two words copied to the prop's +0xF0 / +0xF4 (fn_800699E4)
    { 0x24, SIR_F32, 1 },     // code: fn_80069180 copies it to the entity's +0x158 and the record's +0x24
    { 0x2C, SIR_U32, 1 },     // code: fn_8006A2D0
    { 0x48, SIR_PTR, 1 },     // code: animation table (fn_800699E4 -> fn_801AC184; the table is not converted)
    { 0x68, SIR_U32, 3 },     // code: action words (fn_8006A2D0)
    { 0x78, SIR_U32, 4 },     // code: action words (fn_800696C4, fn_8006BDF4, fn_80069638)
    { 0x98, SIR_U32, 1 },     // code: action word (fn_80069180)
    { 0xBC, SIR_U32, 1 },     // code: flags, copied to the entity's +0x15C (fn_80069180)
};
// A prop definition's animation table (+0x48), as fn_800699E4 uses it:
//   table:  +0x04 how many entries, one u16 per group (fn_801AC184; there is one group: fn_8000E488
//           sets the count lbl_805FB698 points at to 1), +0x0C the entries, +0x10 two lists of
//           clips, picked by the two u16 keys at +0x18, +0x1C how many clips a list has (fn_801AC460)
//   entry:  0x44 bytes: +0x10 its events and +0x14 how many, +0x2C handle (fn_801AC184), +0x34 its
//           table and +0x38 the clip's index in the lists (fn_801AC460)
//   event:  0x1C bytes: +0x00 kind (0xA83AFCFD: an action), +0x04 a condition (action word, 0x0003FFFF
//           = none), +0x08 / +0x0C the time window in seconds, +0x18 the action word to run. The
//           entity's update (draft_pz3e.cpp) asks fn_801ABF0C / fn_801AC024 for the events of the
//           playing animation whose window holds the current time (fn_801ABD84) and hands +0x18 to
//           TriggerDoAction. These are an attack's camera moves (PlayCameraAnimation), particle
//           effects and sounds, and every other animation's timed actions.
// The clips themselves are not converted yet. The table and the lists may lie in different zones.
static const SirField kAnimSetFields[] = {
    { 0x04, SIR_PTR, 1 }, { 0x0C, SIR_PTR, 3 }, { 0x18, SIR_U16, 2 }, { 0x1C, SIR_U32, 1 },
};
static const SirField kAnimDefFields[] = {
    { 0x00, SIR_U32, 4 },     // code: speed (+0x00, fn_801AC51C), flags (+0x0C)
    { 0x10, SIR_PTR, 1 },     // code: events (fn_801ABF0C, fn_801AC024),
    { 0x14, SIR_U32, 6 },     //       their count; floats (+0x1C, +0x20) handed to fn_8018B2CC
    { 0x2C, SIR_U32, 2 }, { 0x34, SIR_PTR, 1 }, { 0x38, SIR_U32, 1 },
    { 0x3C, SIR_U32, 2 },
};
#define ANIM_SET_USED 0x20
#define ENTITY_DEF_USED 0x4C   // of the definition of an entity that is not a prop (entity +0x9C): id +0x08, animation set +0x48
#define ANIM_DEF_SIZE 0x44
static const SirField kAnimEventFields[] = {
    { 0x00, SIR_U32, 2 }, { 0x08, SIR_F32, 2 }, { 0x10, SIR_U32, 3 },
};
#define ANIM_EVENT_SIZE 0x1C
// A character definition (entity +0x9C of types 5 and 13) is 0x250 bytes, laid out one after the other: +0x08 id, +0x1C flags,
// +0x24 a float, +0x34 and +0x40..+0x4C objects, +0x48 the animation set, +0x60 a hash and 20 action words (0x0003FFFF = none),
// +0xB8 the animation speed (fn_80053C2C multiplies the tick by it), then floats (0x7F800001 = unset) up to +0x1F8, flags at +0x240.
// Every field is a 32-bit word. Told from the other objects an entity can point at by the default hash at +0x60.
#define CHAR_DEF_SIZE 0x250

static bool CharDefLooksFull(const u8* image, u32 imageSize, u32 o) {
    return (u64)o + CHAR_DEF_SIZE <= imageSize && Be32(image + o + 0x60) == 0xDC1BFEC5u;
}

static SirError AnimClipPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr);
static SirError AnimObjPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr);

static SirError AnimSetPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    AnyObj set = AnyFind(plan, imageSize, addr, ANIM_SET_USED, REGION_ANIM_SET);
    if (!set.found) return SIR_OK;
    bool walked = set.done && !set.other;                    // in this plan already: its parts are too
    if (!AnyAdd(plan, &set, ANIM_SET_USED, REGION_ANIM_SET)) return SIR_ERR_ENTITY;
    if (walked) return SIR_OK;
    AnyObj counts = AnyFind(plan, imageSize, AnyPtr(image, imageSize, &set, 0x04), 2, REGION_U16_ARRAY);
    if (counts.found) {
        u32 total = AnyWord(image, &counts, 0, 2);
        if (!AnyAdd(plan, &counts, 2, REGION_U16_ARRAY)) return SIR_ERR_ENTITY;
        AnyObj defs = AnyFind(plan, imageSize, AnyPtr(image, imageSize, &set, 0x0C), total * ANIM_DEF_SIZE, REGION_ANIM_DEFS);
        bool defsDone = defs.done;
        if (total != 0 && (!defs.found || !AnyAdd(plan, &defs, total * ANIM_DEF_SIZE, REGION_ANIM_DEFS))) return SIR_ERR_ENTITY;
        // Each entry's events (entries that are converted already have had their turn).
        for (u32 i = 0; defs.found && !defsDone && i < total; i++) {
            u32 n = AnyWord(image, &defs, i * ANIM_DEF_SIZE + 0x14, 4);
            if (n == 0) continue;
            if (n > imageSize / ANIM_EVENT_SIZE) return SIR_ERR_ENTITY;
            AnyObj events = AnyFind(plan, imageSize, AnyPtr(image, imageSize, &defs, i * ANIM_DEF_SIZE + 0x10), n * ANIM_EVENT_SIZE, REGION_ANIM_EVENTS);
            if (events.found && !AnyAdd(plan, &events, n * ANIM_EVENT_SIZE, REGION_ANIM_EVENTS)) return SIR_ERR_ENTITY;
        }
    }
    u32 clips = AnyWord(image, &set, 0x1C, 4);
    if (clips > imageSize / 4) return SIR_ERR_ENTITY;
    for (u32 k = 0; k < 2 && clips != 0; k++) {
        AnyObj list = AnyFind(plan, imageSize, AnyPtr(image, imageSize, &set, 0x10 + k * 4), clips * 4, REGION_PTR_ARRAY);
        if (!list.found) continue;
        bool listed = list.done;
        if (!AnyAdd(plan, &list, clips * 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        // The clips of a list in this image (a list in zone 0 that is converted already has had its turn).
        for (u32 i = 0; !list.other && !listed && i < clips; i++) {
            SirError e = AnimClipPlan(plan, image, imageSize, Be32(image + list.off + i * 4));
            if (e != SIR_OK) return e;
        }
    }
    return SIR_OK;
}

// A prop's definition, in this image or in zone 0's, and its animation table.
static SirError PropDefPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    AnyObj def = AnyFind(plan, imageSize, addr, PROP_DEF_USED, REGION_PROP_DEF);
    if (LoaderWatch() != 0 && addr - sWatchAddr < 0x200)
        fprintf(stderr, "[loader watch] prop definition 0x%08X: found %d, in zone 0 %d, done %d (zone 0 at 0x%08X size 0x%X, this image at 0x%08X size 0x%X)\n",
                addr, def.found, def.other, def.done, sOtherBase, sOtherSize, sImageBase, imageSize);
    if (!def.found) return SIR_OK;
    if (!AnyAdd(plan, &def, PROP_DEF_USED, REGION_PROP_DEF)) return SIR_ERR_ENTITY;
    return AnimSetPlan(plan, image, imageSize, AnyPtr(image, imageSize, &def, 0x48));
}

// {pointer, count}: a part container's items (+0x0C, +0x10) and an item's parts (+0x24, +0x28),
// both walked by fn_8012F6B0.
static const SirField kPtrCountFields[] = {
    { 0x00, SIR_PTR, 1 }, { 0x04, SIR_U32, 1 },
};
// A part (fn_8012F6B0) is a collision shape: vtable slot, a word the function clears, +0x08 its
// definition, +0x0C the kind that picks the vtable, +0x10 the entity, +0x14 and +0x20 its position
// now and before (fn_801AD050), +0x2C the group key, +0x30 a bone's name hash and +0x34 the bone's
// index (-1: not looked up yet, fn_801ACD08), +0x3C the radius. The fields a kind adds after
// +0x40 are not converted yet (only kind 0, the sphere, is in the first levels).
// The definition: +0x00 kind, +0x04 key, +0x08 bone hash, +0x0C offset on the bone (3 floats),
// +0x24 radius.
static const SirField kEntityPartFields[] = {
    { 0x00, SIR_U32, 2 }, { 0x08, SIR_PTR, 1 }, { 0x0C, SIR_U32, 1 }, { 0x10, SIR_PTR, 1 }, { 0x14, SIR_U32, 11 },
};
#define ENTITY_PART_USED 0x40
#define ENTITY_PART_DEF_USED 0x28

#define ENTITY_DEPTH_MAX 64
#define MAP_PATH_SIZE     0x1AC
#define MAP_JUNCTION_SIZE 0xDC

// In a level zone a pointer may lead into another zone's image, which is not in memory when this
// one is converted. Such an object is skipped (its pointer is still converted); 1 = inside.
static int LevelSpan(u32 addr, u64 size, u32 imageSize, u32* off) {
    if (addr == 0) return 0;
    return ImageSpan(addr, size, imageSize, off) ? 1 : 0;
}

// Materials. A mesh part {material, batch} is drawn by fn_80173438: it takes the material's pass
// byte (+0x0D), calls the "before" function of the column the type byte (+0x0C) picks in
// lbl_80348F38 with the record at +0x08, draws the batch (fn_801873C0), then calls the "after"
// function. The level files only have types 0 and 1. A material starts {0x53, a word (2 to 7), record
// pointer, type, pass, ...} (data only for the two words), and the material is at its record's +0x00.
//   material, type 0 (fn_80176E20, fn_80177940): +0x1C and +0x20 textures (a header of
//     SIR_TEXTURE_HEADER bytes: the draw adds 4 and reads its size and format bytes, fn_80177748),
//     +0x3C a 3x4 float matrix (fn_801776C8)
//   record, type 0: +0x04 and +0x05 enable bytes, +0x08..+0x44 sixteen floats, +0x4C and +0x50
//     floats, +0x54 mode word, +0x58..+0x5A and +0x64..+0x67 bytes, +0x5C and +0x60 words. +0x48
//     and the bytes +0x06 / +0x07 have no reader.
#define MATERIAL_TAG    0x53
#define MATERIAL_USED   0x40
#define MATERIAL_REC0   0x68
//   material, type 1 (fn_80181DE0 before the batch, fn_80181F80 after it): +0x1C eight textures
//     and +0x3C eight 3x4 float matrices, one per texture unit (fn_8017B9BC)
//   record, type 1: the GX state fn_8017B9BC sends when it differs from the last one sent
//     (lbl_80412B10[pass]), SIR_GX_STATE_SIZE bytes of words except where SirGxStateSlot says:
//     +0x04 colour channels, +0x08 tex-gens, +0x0C TEV stages, +0x10 indirect stages, +0x14 two
//     colour channels {light enable and address twice, two 0x1C-byte channel blocks}, +0xA4 eight
//     0x24-byte texture units, +0x1C4 eight 0x34-byte tex-gens, +0x364 three TEV colours and
//     +0x3A0 four constant colours {mode, four floats}, +0x3F0 four swap tables, +0x430 sixteen
//     0x68-byte TEV stages, +0xAB0 four indirect stages, +0xAF0 sixteen 0x20-byte indirect
//     settings of the stages, +0xCF0 a float and the alpha, depth and cull settings.
extern "C" u8 SirGxStateSlot(u32 off) {
    if (off == 0) return SIR_GX_PTR;
    if (off == 0x14 || off == 0x1C || off == 0x24 || off == 0x40 || off == 0x5C || off == 0x64 || off == 0x6C || off == 0x88) return SIR_GX_BYTES;
    if (off >= 0xA4 && off < 0x1C4) { u32 f = (off - 0xA4) % 0x24; return f == 0x14 || f == 0x1C ? SIR_GX_BYTES : SIR_GX_WORD; }
    if (off >= 0x1C4 && off < 0x364) {
        u32 f = (off - 0x1C4) % 0x34;
        return f == 0 ? SIR_GX_BYTES_U16 : f == 4 ? SIR_GX_U16_BYTES : f == 0xC || f == 0x2C ? SIR_GX_BYTES : SIR_GX_WORD;
    }
    if (off >= 0x430 && off < 0xAB0) { u32 f = (off - 0x430) % 0x68; return f == 0 || f == 0x38 || f == 0x5C ? SIR_GX_BYTES : SIR_GX_WORD; }
    if (off >= 0xAF0 && off < 0xCF0) return (off - 0xAF0) % 0x20 == 0x18 ? SIR_GX_BYTES : SIR_GX_WORD;
    if (off == 0xCFC || off == 0xD10 || off == 0xD1C) return SIR_GX_BYTES;
    return SIR_GX_WORD;
}

static void ConvertGxState(SirReloc* r, u8* p, u32 size) {
    for (u32 off = 0; off + 4 <= size && off < SIR_GX_STATE_SIZE; off += 4) {
        switch (SirGxStateSlot(off)) {
        case SIR_GX_WORD:      Swap32(p + off); r->fieldsSwapped++; break;
        case SIR_GX_PTR:       ConvertPtr(r, p + off); break;
        case SIR_GX_BYTES_U16: Swap16(p + off + 2); r->fieldsSwapped++; break;
        case SIR_GX_U16_BYTES: Swap16(p + off); r->fieldsSwapped++; break;
        default: break;
        }
    }
}

static SirError MaterialTexturePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 t, d;
    if (!LevelSpan(addr, SIR_TEXTURE_HEADER, imageSize, &t) || PlanHas(plan, t, REGION_TEXTURE)) return SIR_OK;
    const u8* tex = image + t;
    // Pixel data and palette stay in GX layout, as in the textures of global_binary.sir.
    if (Be32(tex) != SIR_TEXTURE_HEADER || !ImageSpan(Be32(tex + 0x1C), Be32(tex + 0x18), imageSize, &d)) return SIR_ERR_TEXTURE;
    u32 palette = Be32(tex + 0x20);
    if (palette != 0 && !ImageSpan(palette, 2, imageSize, &d)) return SIR_ERR_TEXTURE;
    return PlanAdd(plan, t, SIR_TEXTURE_HEADER, REGION_TEXTURE) ? SIR_OK : SIR_ERR_TEXTURE;
}

static SirError MaterialPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 m, r;
    if (!LevelSpan(addr, MATERIAL_USED, imageSize, &m) || PlanHas(plan, m, REGION_U32_ARRAY)) return SIR_OK;
    const u8* mat = image + m;
    if (Be32(mat) != MATERIAL_TAG || mat[0x0C] > 1) return SIR_ERR_ENTITY;
    u32 matrix;
    SirError e;
    if (mat[0x0C] == 1) {
        // The same regions as ZoneListPlan, which walks the materials that have animated matrices (+0x5C: their players).
        if (!LevelSpan(addr, 0x80, imageSize, &m) || !PlanAdd(plan, m, 8, REGION_U32_ARRAY) || !PlanAdd(plan, m + 0x08, 4, REGION_PTR_ARRAY) ||
            !PlanAdd(plan, m + 0x1C, 0x20, REGION_PTR_ARRAY) || !PlanAdd(plan, m + 0x3C, 4, REGION_PTR_ARRAY) ||
            !PlanAdd(plan, m + 0x40, 0x40, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        for (u32 i = 0; i < 8; i++) {
            if ((e = MaterialTexturePlan(plan, image, imageSize, Be32(mat + 0x1C + i * 4))) != SIR_OK) return e;
            if (LevelSpan(Be32(mat + 0x3C + i * 4), 0x30, imageSize, &matrix) && !PlanHas(plan, matrix, REGION_F32_ARRAY) &&
                !PlanAdd(plan, matrix, 0x30, REGION_F32_ARRAY)) return SIR_ERR_ENTITY;
        }
        if (!LevelSpan(Be32(mat + 0x08), SIR_GX_STATE_SIZE, imageSize, &r)) return SIR_OK;
        if (Be32(image + r) != addr) return SIR_ERR_ENTITY;                               // the record names its material
        return PlanAdd(plan, r, SIR_GX_STATE_SIZE, REGION_GX_STATE) ? SIR_OK : SIR_ERR_ENTITY;
    }
    if (!PlanAdd(plan, m, 8, REGION_U32_ARRAY) || !PlanAdd(plan, m + 0x08, 4, REGION_PTR_ARRAY) ||
        !PlanAdd(plan, m + 0x1C, 8, REGION_PTR_ARRAY) || !PlanAdd(plan, m + 0x3C, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
    if (LevelSpan(Be32(mat + 0x3C), 0x30, imageSize, &matrix) && !PlanHas(plan, matrix, REGION_F32_ARRAY) &&
        !PlanAdd(plan, matrix, 0x30, REGION_F32_ARRAY)) return SIR_ERR_ENTITY;
    if ((e = MaterialTexturePlan(plan, image, imageSize, Be32(mat + 0x1C))) != SIR_OK ||
        (e = MaterialTexturePlan(plan, image, imageSize, Be32(mat + 0x20))) != SIR_OK) return e;
    if (mat[0x0C] != 0 || !LevelSpan(Be32(mat + 0x08), MATERIAL_REC0, imageSize, &r)) return SIR_OK;
    if (Be32(image + r) != addr) return SIR_ERR_ENTITY;                                   // the record names its material
    if (!PlanAdd(plan, r, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, r + 0x08, 0x40, REGION_U32_ARRAY) ||
        !PlanAdd(plan, r + 0x4C, 0x0C, REGION_U32_ARRAY) || !PlanAdd(plan, r + 0x5C, 8, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
    return SIR_OK;
}

// Scene object: what an entity's +0x90 points at, and what the header block's per-zone tables
// (+0x1B0, +0x1BC) hold. The type word at +4 picks the class; its constructor stores the vtable
// (fn_801D94CC 1, fn_801DA81C 2, fn_801DA1CC 3, fn_801D9A24 4).
//   type 2: +0x48 an object whose +0x30 / +0x34 are cleared, +0x4C a list object: {+0x04 head,
//           +0x08 tail, +0x0C count} and at +0x38 how many of its nodes are flagged (fn_80184314,
//           fn_8017323C). A node is the link {next, prev, owner list} at +0xB8 of an item, whose
//           flags are at +0x0C (fn_801DB160, fn_801842E0, fn_80184314)
//   type 3: +0x54 count, +0x58 pointers to entries whose +0x30 / +0x34 are set (fn_801DA7C8); the
//           entries are such items (fn_801DA91C hands each to fn_80184314);
//           +0x48 the entity that uses the object (fn_801DA344 stores it), +0x50 which entry is
//           current (fn_801DA750), +0x5C one pointer per entry to its 3x4 float matrix
//           (fn_801DA798 returns matrix[current]; fn_801D9164 fills it)
// A light of a type 2 object's list object (+0x10 head, +0x14 tail, +0x18 count: the same list
// as the items', linked through the light's +0x04). The draw picks the three best lights of
// every visible item from it (fn_80185988: fn_80185200 for a point light, fn_8018542C for kind 2):
// +0x04 next, previous and the list, +0x10 kind (1 point, 3 ambient in the files) and +0x12
// flags, +0x14 a factor, +0x20 position, +0x2C three attenuation coefficients, +0x38 direction
// and colours up to +0x60 (fn_80184604). The name behind them is text.
static const SirField kSceneLightFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_PTR, 3 }, { 0x10, SIR_U16, 2 }, { 0x14, SIR_F32, 19 },
};
#define SCENE_LIGHT_USED 0x60
static const SirField kCountPtrFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_PTR, 1 },
};

// What the draw passes read of an item (the culling pass fn_80184C18 with fn_80171ABC, and
// fn_80171228, which queues the item's mesh parts), at image offset `item`:
//   +0x04 its matrix: twelve floats, and behind them twelve more that fn_80171228 writes
//   +0x08 scale; +0x10 its mesh; +0x14 seven floats fn_80171ABC writes (two positions, a radius)
//   +0x30 callback (a build-time address in the file) and +0x34 its argument
//   +0x3C four floats the culling pass writes; +0x58 float; +0xB0 pointer (non-null: fn_801704F8)
//   +0x3C..+0x5B is also the block `buf` that fn_80173438 hands to the material functions
//   (fn_80176E20: eight floats at buf +0x00..+0x1C, the last four are used when the material's
//   record is disabled), so +0x4C..+0x57 are floats too (the file has 1.0 at +0x4C);
//   +0x9C a float and +0xA0 a texture pointer that overrides the material's (buf +0x60 / +0x64)
// and of the mesh: +0x20 centre and radius, +0x48 three floats, +0x54 scale, +0x60 the kind (3 = a particle mesh, fn_801CBD40), +0x64 a pointer,
// +0x6C the number of parts, +0x70 the parts {material, batch}; of a batch the flags at +0x0C.
static SirError SkeletonPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr);

// A mesh part's batch, as fn_801873C0 draws it (the pass where its second argument is zero):
//   +0x04 and +0x08 words it adds to the draw statistics (lbl_804129E8), +0x0C flags (bits 5 and 6
//   pick a bounding-volume shape), +0x18 the offset of the display list from the batch, +0x24 its
//   size; the batch then hands the display list to the GX FIFO (command 0x40: its address and size).
// The display list and the vertex arrays behind it stay in GX layout. Not converted: +0x00, +0x10,
// +0x14, +0x20 (no reader on this path).
// The skinned pass (fn_801870AC) reads +0x1C, the number of vertex groups, and from +0x28 one entry per
// group: {u32 k, then k times {u32 offset into the matrix palette, f32 weight}}, 4 + 8k bytes each
// (the first influence initialises the group's matrix, the others are added to it).
#define BATCH_USED 0x28
static SirError BatchPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 b, d;
    if (!LevelSpan(addr, BATCH_USED, imageSize, &b) || PlanHas(plan, b + 0x0C, REGION_U32_ARRAY)) return SIR_OK;
    const u8* batch = image + b;
    // The display list lies inside the zone.
    if (Be32(batch + 0x18) > imageSize || !ImageSpan(addr + Be32(batch + 0x18), Be32(batch + 0x24), imageSize, &d)) return SIR_ERR_ENTITY;
    if (!PlanAdd(plan, b + 0x04, 0x0C, REGION_U32_ARRAY) || !PlanAdd(plan, b + 0x18, 4, REGION_U32_ARRAY) ||
        !PlanAdd(plan, b + 0x24, 4, REGION_U32_ARRAY) || !PlanAdd(plan, b + 0x1C, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
    u32 groups = Be32(batch + 0x1C), at = b + BATCH_USED;
    u32 end = b + Be32(batch + 0x18);                                                  // the display list ends the batch's data
    for (u32 i = 0; i < groups; i++) {
        if (at + 4 > end || at + 4 > imageSize) return SIR_ERR_ENTITY;
        u32 k = Be32(image + at);
        if (k == 0 || k > 64 || at + 4 + (u64)k * 8 > end) return SIR_ERR_ENTITY;
        if (!PlanAdd(plan, at, 4 + k * 8, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        at += 4 + k * 8;
    }
    return SIR_OK;
}

static SirError SceneItemDrawPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 item) {
    u32 x, m;
    if (PlanHas(plan, item + 0x10, REGION_PTR_ARRAY)) return SIR_OK;
    if (!PlanAdd(plan, item + 0x04, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, item + 0x08, 4, REGION_U32_ARRAY) ||
        !PlanAdd(plan, item + 0x10, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, item + 0x14, 0x1C, REGION_U32_ARRAY) ||
        !PlanAdd(plan, item + 0x30, 4, REGION_U32_ARRAY) || !PlanAdd(plan, item + 0x34, 4, REGION_PTR_ARRAY) ||
        !PlanAdd(plan, item + 0x3C, 0x10, REGION_U32_ARRAY) || !PlanAdd(plan, item + 0x58, 4, REGION_U32_ARRAY) ||
        !PlanAdd(plan, item + 0xB0, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, item + 0x38, 4, REGION_PTR_ARRAY) ||
        !PlanAdd(plan, item + 0x4C, 0x0C, REGION_U32_ARRAY) || !PlanAdd(plan, item + 0x9C, 4, REGION_U32_ARRAY) ||
        !PlanAdd(plan, item + 0xA0, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
    // +0xB0: the item's skeleton; +0x38: a block freed with the item (both fn_80172708).
    if (SkeletonPlan(plan, image, imageSize, Be32(image + item + 0xB0)) != SIR_OK) return SIR_ERR_ENTITY;
    if (LevelSpan(Be32(image + item + 0x04), 0x60, imageSize, &x) &&
        (!PlanAdd(plan, x, 0x30, REGION_U32_ARRAY) || !PlanAdd(plan, x + 0x30, 0x30, REGION_U32_ARRAY))) return SIR_ERR_ENTITY;
    if (!LevelSpan(Be32(image + item + 0x10), 0x70, imageSize, &m) || PlanHas(plan, m + 0x6C, REGION_U32_ARRAY)) return SIR_OK;
    u32 parts = Be32(image + m + 0x6C);
    if (parts > imageSize / 8 || (u64)m + 0x70 + (u64)parts * 8 > imageSize) return SIR_ERR_ENTITY;
    if (!PlanAdd(plan, m + 0x20, 0x10, REGION_U32_ARRAY) || !PlanAdd(plan, m + 0x48, 0x10, REGION_U32_ARRAY) ||
        !PlanAdd(plan, m + 0x60, 4, REGION_U32_ARRAY) || !PlanAdd(plan, m + 0x64, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, m + 0x6C, 4, REGION_U32_ARRAY) ||
        (parts != 0 && !PlanAdd(plan, m + 0x70, parts * 8, REGION_PTR_ARRAY))) return SIR_ERR_ENTITY;
    for (u32 i = 0; i < parts; i++) {
        SirError e = MaterialPlan(plan, image, imageSize, Be32(image + m + 0x70 + i * 8));
        if (e != SIR_OK) return e;
        if ((e = BatchPlan(plan, image, imageSize, Be32(image + m + 0x74 + i * 8))) != SIR_OK) return e;
    }
    return SIR_OK;
}

// Effect library: header +0x7C records, array at +0x80 (fn_8012FFA4 hands it to the manager lbl_8047C730; SpawnEffectAt
// binary-searches it by the key when the sorted flag at +0x84 is set). A record (0xEC bytes): +0x00 the effect's name hash,
// +0x04 the number of emitters (up to 8), from +0x0C the emitters (0x1C bytes: a pointer to the emitter descriptor, six floats).
// A descriptor (0x1F4 bytes, found by the stride between them) is all words: hashes and floats, no pointers.
#define FX_EFFECT_SIZE 0xEC
#define FX_EMITTER_SIZE 0x1C
#define FX_EMITTER_DESC_SIZE 0x1F4
static SirError FxEffectLibraryPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 count) {
    u32 off, o;
    if (count == 0 || addr == 0) return SIR_OK;
    if (count > 0x10000 || !ImageSpan(addr, (u64)count * FX_EFFECT_SIZE, imageSize, &off)) return SIR_ERR_EFFECTS;
    for (u32 i = 0; i < count; i++) {
        u32 rec = off + i * FX_EFFECT_SIZE, emitters = Be32(image + rec + 4);
        if (emitters > 8 || !PlanAdd(plan, rec, 8, REGION_U32_ARRAY)) return SIR_ERR_EFFECTS;
        for (u32 k = 0; k < emitters; k++) {
            u32 e = rec + 0x0C + k * FX_EMITTER_SIZE;
            if (!PlanAdd(plan, e, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, e + 4, FX_EMITTER_SIZE - 4, REGION_U32_ARRAY)) return SIR_ERR_EFFECTS;
            if (!ImageSpan(Be32(image + e), FX_EMITTER_DESC_SIZE, imageSize, &o)) return SIR_ERR_EFFECTS;
            if (PlanHas(plan, o, REGION_U32_ARRAY)) continue;
            // +0x1D4 a texture record (a sprite emitter), +0x1D8 a scene item whose mesh is the particle (fn_801CBD40).
            if (!PlanAdd(plan, o, 0x1D4, REGION_U32_ARRAY) || !PlanAdd(plan, o + 0x1D4, 8, REGION_PTR_ARRAY) ||
                !PlanAdd(plan, o + 0x1DC, FX_EMITTER_DESC_SIZE - 0x1DC, REGION_U32_ARRAY)) return SIR_ERR_EFFECTS;
            SirError te = MaterialTexturePlan(plan, image, imageSize, Be32(image + o + 0x1D4));
            u32 it;
            if (te == SIR_OK && LevelSpan(Be32(image + o + 0x1D8), 0xC4, imageSize, &it)) te = SceneItemDrawPlan(plan, image, imageSize, it);
            if (te != SIR_OK) return te;
        }
    }
    return SIR_OK;
}

// A collision tree (fn_8016CA24): +0x00 a version word (0x43), +0x04 how many vertices, +0x08
// triangles, +0x0C flag words, +0x10 nodes, +0x14 the size of the node buffer, +0x18 the vertices
// (three floats), +0x1C the flag words (one per surface; a triangle names its own), +0x20 the node
// buffer. A node is 0x20 bytes: box minimum, link, box maximum, a word. With bit 31 of the link
// clear it has two children, at the offsets `link` and the word; with it set it is a leaf of
// `word` triangles at the offset `link & 0x7FFFFFFF`: four u16 each (three vertices, the flag word).
#define COLLISION_TREE_USED 0x24
static SirError CollisionTreePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off, verts, flags, nodes;
    if (!LevelSpan(addr, COLLISION_TREE_USED, imageSize, &off) || PlanHas(plan, off, REGION_U32_ARRAY)) return SIR_OK;
    if (!PlanAdd(plan, off, 0x18, REGION_U32_ARRAY) || !PlanAdd(plan, off + 0x18, 0xC, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
    u32 vertCount = Be32(image + off + 4), flagCount = Be32(image + off + 0xC), size = Be32(image + off + 0x14);
    if (vertCount > imageSize / 12 || flagCount > imageSize / 4 || size > imageSize) return SIR_ERR_ENTITY;
    if (vertCount != 0 && (!LevelSpan(Be32(image + off + 0x18), (u64)vertCount * 12, imageSize, &verts) ||
                           !PlanAdd(plan, verts, vertCount * 12, REGION_U32_ARRAY))) return SIR_ERR_ENTITY;
    if (flagCount != 0 && (!LevelSpan(Be32(image + off + 0x1C), (u64)flagCount * 4, imageSize, &flags) ||
                           !PlanAdd(plan, flags, flagCount * 4, REGION_U32_ARRAY))) return SIR_ERR_ENTITY;
    if (size < 0x20 || Be32(image + off + 8) == 0) return SIR_OK;
    if (!LevelSpan(Be32(image + off + 0x20), size, imageSize, &nodes)) return SIR_ERR_ENTITY;
    u32 stack[64], depth = 0, at = 0, guard = size / 0x20 + 1;
    for (;;) {
        if (guard-- == 0 || (u64)at + 0x20 > size) return SIR_ERR_ENTITY;
        if (!PlanAdd(plan, nodes + at, 0x20, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;      // a node reached twice is an overlap
        u32 link = Be32(image + nodes + at + 0xC), word = Be32(image + nodes + at + 0x1C);
        if (link & 0x80000000u) {
            u32 tris = link & 0x7FFFFFFFu;
            if (word > size / 8 || (u64)tris + (u64)word * 8 > size) return SIR_ERR_ENTITY;
            if (word != 0 && !PlanAdd(plan, nodes + tris, word * 8, REGION_U16_ARRAY)) return SIR_ERR_ENTITY;
            if (depth == 0) break;
            at = stack[--depth];
        } else {
            if (depth == 64) return SIR_ERR_ENTITY;
            stack[depth++] = word;
            at = link;
        }
    }
    return SIR_OK;
}

static SirError SceneObjectPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off, o;
    if (!LevelSpan(addr, 8, imageSize, &off) || PlanHas(plan, off + 4, REGION_U32_ARRAY)) return SIR_OK;
    if (!PlanAdd(plan, off + 4, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
    u32 type = Be32(image + off + 4);
    if (type == 2) {
        if (!ImageSpan(addr, 0x50, imageSize, &off) || !PlanAdd(plan, off + 0x48, 8, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        if (!LevelSpan(Be32(image + off + 0x4C), 0x3C, imageSize, &o)) return SIR_OK;
        if (PlanHas(plan, o + 4, REGION_PTR_ARRAY)) return SIR_OK;                     // two objects with one list
        if (!PlanAdd(plan, o + 4, 8, REGION_PTR_ARRAY) || !PlanAdd(plan, o + 0xC, 4, REGION_U32_ARRAY) ||
            !PlanAdd(plan, o + 0x38, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        // The collision tree of the zone's ground (+0x1C; fn_80139684 hands it to fn_8016CA24).
        if (!PlanAdd(plan, o + 0x1C, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        { SirError te = CollisionTreePlan(plan, image, imageSize, Be32(image + o + 0x1C)); if (te != SIR_OK) return te; }
        // The list's lights (+0x10, +0x14, count +0x18).
        if (!PlanAdd(plan, o + 0x10, 8, REGION_PTR_ARRAY) || !PlanAdd(plan, o + 0x18, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        u32 lightNode = Be32(image + o + 0x10), light, lights = 0;
        while (lightNode != 0) {
            if (lightNode < sImageBase + 4 || !ImageSpan(lightNode - 4, SCENE_LIGHT_USED, imageSize, &light) || PlanHas(plan, light, REGION_SCENE_LIGHT)) break;
            if (!PlanAdd(plan, light, SCENE_LIGHT_USED, REGION_SCENE_LIGHT) || ++lights > REGION_MAX / 2) return SIR_ERR_ENTITY;
            lightNode = Be32(image + light + 4);
        }
        // The nodes can also be entries of a type 3 object: the same words, found twice.
        u32 node = Be32(image + o + 4), guard = imageSize / 4 + 1, seen = 0;
        while (node != 0) {
            if (guard-- == 0 || node < sImageBase + 0xAC || !ImageSpan(node - 0xAC, 0xB8, imageSize, &o)) return SIR_ERR_ENTITY;
            if (!PlanAdd(plan, o, 4, REGION_U32_ARRAY) || !PlanAdd(plan, o + 0xAC, 0xC, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
            if (o < 0xC || SceneItemDrawPlan(plan, image, imageSize, o - 0xC) != SIR_OK) return SIR_ERR_ENTITY;
            if (++seen > REGION_MAX / 2) return SIR_ERR_ENTITY;                         // a list that loops
            node = Be32(image + o + 0xAC);
        }
    } else if (type == 3) {
        if (!ImageSpan(addr, 0x5C, imageSize, &off) || !PlanAdd(plan, off + 0x54, 8, REGION_COUNT_PTR)) return SIR_ERR_ENTITY;
        u32 count = Be32(image + off + 0x54), array = Be32(image + off + 0x58);
        if (!ImageSpan(addr, 0x60, imageSize, &off) || !PlanAdd(plan, off + 0x48, 4, REGION_PTR_ARRAY) ||
            !PlanAdd(plan, off + 0x50, 4, REGION_U32_ARRAY) || !PlanAdd(plan, off + 0x5C, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        // The key-track record (fn_801D90A0 resets it, fn_801DA34C reads it every frame): flags, two quads, +0x38..+0x44 the
        // tint the items take (1.0 in the files; unconverted it reads as 0 and the item is invisible).
        if (!PlanAdd(plan, off + 0x08, 0x40, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        if (count != 0 && array != 0) {
            if (!ImageSpan(array, (u64)count * 4, imageSize, &o) || !PlanAdd(plan, o, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
            // The entries: the flags and the list link of each.
            u32 item;
            for (u32 i = 0; i < count; i++)
                if (LevelSpan(Be32(image + o + i * 4), 0xC4, imageSize, &item) &&
                    (!PlanAdd(plan, item + 0xC, 4, REGION_U32_ARRAY) || !PlanAdd(plan, item + 0xB8, 0xC, REGION_PTR_ARRAY) ||
                     SceneItemDrawPlan(plan, image, imageSize, item) != SIR_OK)) return SIR_ERR_ENTITY;
        }
        // The matrices: twelve floats each (a float is swapped like a word).
        u32 matrices = Be32(image + off + 0x5C), m;
        if (count != 0 && LevelSpan(matrices, (u64)count * 4, imageSize, &o)) {
            if (!PlanAdd(plan, o, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
            for (u32 i = 0; i < count; i++)
                if (LevelSpan(Be32(image + o + i * 4), 0x30, imageSize, &m) && !PlanAdd(plan, m, 0x30, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        }
    }
    return SIR_OK;
}

static SirError EntityContainerPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off, itemsOff, partsOff, partOff;
    if (!LevelSpan(addr, 0x14, imageSize, &off) || PlanHas(plan, off + 0x0C, REGION_PTR_COUNT)) return SIR_OK;
    if (!PlanAdd(plan, off + 0x0C, 8, REGION_PTR_COUNT)) return SIR_ERR_ENTITY;
    if (!PlanAdd(plan, off + 8, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;          // the entity it belongs to (fn_801B1778)
    u32 items = Be32(image + off + 0x10);
    if (items == 0) return SIR_OK;
    if (!ImageSpan(Be32(image + off + 0x0C), (u64)items * 4, imageSize, &itemsOff) ||
        !PlanAdd(plan, itemsOff, items * 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
    for (u32 i = 0; i < items; i++) {
        u32 itemOff;
        if (!LevelSpan(Be32(image + itemsOff + i * 4), 0x2C, imageSize, &itemOff)) continue;
        if (PlanHas(plan, itemOff + 0x24, REGION_PTR_COUNT)) continue;
        if (!PlanAdd(plan, itemOff + 0x24, 8, REGION_PTR_COUNT)) return SIR_ERR_ENTITY;
        // +0x04 the container, +0x08 the group's key (fn_801B154C), +0x20 flags (fn_801B1778).
        if (!PlanAdd(plan, itemOff + 4, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, itemOff + 8, 4, REGION_U32_ARRAY) ||
            !PlanAdd(plan, itemOff + 0x20, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        u32 parts = Be32(image + itemOff + 0x28);
        if (parts == 0) continue;
        if (!ImageSpan(Be32(image + itemOff + 0x24), (u64)parts * 4, imageSize, &partsOff) ||
            !PlanAdd(plan, partsOff, parts * 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        for (u32 k = 0; k < parts; k++) {
            if (!LevelSpan(Be32(image + partsOff + k * 4), ENTITY_PART_USED, imageSize, &partOff)) continue;
            if (!PlanAdd(plan, partOff, ENTITY_PART_USED, REGION_ENTITY_PART)) return SIR_ERR_ENTITY;
            // The shape's definition (+0x08), often in zone 0.
            AnyObj def = AnyFind(plan, imageSize, Be32(image + partOff + 8), ENTITY_PART_DEF_USED, REGION_U32_ARRAY);
            if (def.found && !AnyAdd(plan, &def, ENTITY_PART_DEF_USED, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        }
    }
    return SIR_OK;
}

static u32 sPooledEntityType;   // set while PoolTablePlan walks a pool of entities, else 0
static SirError EntityPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 depth) {
    u32 off, o;
    if (!LevelSpan(addr, ENTITY_USED, imageSize, &off) || PlanHas(plan, off, REGION_ENTITY) || PlanHas(plan, off, REGION_ENTITY_PROP) || PlanHas(plan, off, REGION_ENTITY_TIMED))
        return SIR_OK;
    const u8* ent = image + off;
    // The type is the byte at +0x34 of the record (fn_8012F8B8 picks the constructor by it).
    // The record of an entity outside zone 0 lies in zone 0, and so does a prop's definition.
    u32 otherRec = 0, q;
    bool recordHere = LevelSpan(Be32(ent + 0x04), ENTITY_RECORD_SIZE, imageSize, &o) != 0;
    bool recordOther = !recordHere && OtherSpan(Be32(ent + 0x04), ENTITY_RECORD_SIZE, &otherRec);
    // A pooled entity has no record in the file (fn_8012E420 gives it one when it is spawned): its
    // type is the one its pool table entry names (+0x1C, which FixUpEntityCallback hands to fn_8012F8B8).
    u32 kind = recordHere ? image[o + 0x34] : recordOther ? sOtherImage[otherRec + 0x34] : sPooledEntityType;
    // Types 0x0F (fn_8006CCC4; its vtable slot 0x30, fn_8006CD24, is the prop's fn_80069180) and 0x10 (the
    // challenge projector, fn_8006CFA4) derive from the prop: same layout and definition.
    // So does type 0x0C (fn_8004E684 calls the prop's constructor fn_800690D4; arena props).
    bool prop = kind == ENTITY_TYPE_PROP || kind == 0x0C || kind == 0x0F || kind == 0x10;
    if (recordOther && !OtherAdd(otherRec, ENTITY_RECORD_SIZE, REGION_ENTITY_RECORD)) return SIR_ERR_ENTITY;
    if (depth > ENTITY_DEPTH_MAX || !PlanAdd(plan, off, ENTITY_USED, prop ? REGION_ENTITY_PROP : kind == 9 ? REGION_ENTITY_TIMED : REGION_ENTITY)) return SIR_ERR_ENTITY;
    SirError e;

    if (LevelSpan(Be32(ent + 0x04), ENTITY_RECORD_SIZE, imageSize, &o) && !PlanAdd(plan, o, ENTITY_RECORD_SIZE, REGION_ENTITY_RECORD))
        return SIR_ERR_ENTITY;
    if ((e = EntityContainerPlan(plan, image, imageSize, Be32(ent + 0x8C))) != SIR_OK) return e;
    // Scene object (+0x90). Object at +0x9C: fn_8012F8B8 reads the id at +8 for a type 6 entity.
    if ((e = SceneObjectPlan(plan, image, imageSize, Be32(ent + 0x90))) != SIR_OK) return e;
    // The entity's attach points (+0x98, count +0x94).
    u32 points = Be32(ent + 0x94);
    if (points != 0 && points <= imageSize / ATTACH_POINT_SIZE && LevelSpan(Be32(ent + 0x98), (u64)points * ATTACH_POINT_SIZE, imageSize, &o) &&
        !PlanHas(plan, o, REGION_ATTACH_POINTS) && !PlanAdd(plan, o, points * ATTACH_POINT_SIZE, REGION_ATTACH_POINTS)) return SIR_ERR_ENTITY;
    // The entity's animation object (+0xA0).
    if ((e = AnimObjPlan(plan, image, imageSize, Be32(ent + 0xA0))) != SIR_OK) return e;
    // For a prop it is the prop's definition.
    if (prop) {
        if ((e = PropDefPlan(plan, image, imageSize, Be32(ent + 0x9C))) != SIR_OK) return e;
        // The prop's states (fn_800699E4): +0x1A4 pointers to definitions at +0x1A0. Every prop of
        // the shipped levels has one state and names its own field +0x9C as the list: that word
        // is converted with the entity.
        u32 states = Be32(ent + 0x1A4), list, def;
        if (states != 0 && LevelSpan(Be32(ent + 0x1A0), (u64)states * 4, imageSize, &list) && !(states == 1 && list == off + 0x9C)) {
            if (!PlanAdd(plan, list, states * 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
            for (u32 i = 0; i < states; i++) {
                if ((e = PropDefPlan(plan, image, imageSize, Be32(image + list + i * 4))) != SIR_OK) return e;
            }
        }
    } else if (LevelSpan(Be32(ent + 0x9C), 0xC, imageSize, &o)) {
        if (!PlanAdd(plan, o + 8, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        // +0x48: the definition's animation set (fn_8005BEC8 looks a conversation's clip up in it).
        if ((u64)o + ENTITY_DEF_USED <= imageSize && !PlanAdd(plan, o + 0x48, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        if ((kind == 5 || kind == 13) && CharDefLooksFull(image, imageSize, o) && (!PlanAdd(plan, o + 0x0C, 0x3C, REGION_U32_ARRAY) || !PlanAdd(plan, o + 0x4C, CHAR_DEF_SIZE - 0x4C, REGION_U32_ARRAY))) return SIR_ERR_ENTITY;
    } else if (OtherSpan(Be32(ent + 0x9C), 0xC, &q)) {
        if (!OtherAdd(q + 8, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        if ((u64)q + ENTITY_DEF_USED <= sOtherSize && !OtherAdd(q + 0x48, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        if ((kind == 5 || kind == 13) && CharDefLooksFull(sOtherImage, sOtherSize, q) && (!OtherAdd(q + 0x0C, 0x3C, REGION_U32_ARRAY) || !OtherAdd(q + 0x4C, CHAR_DEF_SIZE - 0x4C, REGION_U32_ARRAY))) return SIR_ERR_ENTITY;
    }
    if (!prop && Be32(ent + 0x9C) != 0) {
        // The animation set itself (fn_800526E4 -> fn_801AC6B4 reads its entries when the entity wakes).
        AnyObj setPtr = AnyFind(plan, imageSize, Be32(ent + 0x9C) + 0x48, 4, REGION_PTR_ARRAY);
        if (setPtr.found && (e = AnimSetPlan(plan, image, imageSize, AnyPtr(image, imageSize, &setPtr, 0))) != SIR_OK) return e;
    }
    // State machine holder: {machine, object}; fn_8012F8B8 hands the machine's definition to fn_801BC574.
    if (LevelSpan(Be32(ent + 0xA4), 8, imageSize, &o)) {
        if (!PlanAdd(plan, o, 8, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        u32 machine = Be32(image + o), m;
        if (LevelSpan(machine, FSM_USED, imageSize, &m) && (e = FsmPlan(plan, image, imageSize, machine)) != SIR_OK) return e;
    }
    // Type 8 (fn_8006D2D0, a trigger volume) and type 9 (fn_8006F6BC, derived from it) have more behind the common part: +0x1BC a float, +0x1C4 four
    // action words, +0x1FE a u16, +0x204 a filter and +0x208 a word. The filter (fn_801B7A4C): +0x00 a list
    // of ids, +0x04 how many entity ids it holds or +0x08 how many definition ids, +0x0C flags.
    if ((kind == 8 || kind == 9) && (u64)off + 0x20C <= imageSize) {
        if (!PlanAdd(plan, off + 0x1BC, 4, REGION_U32_ARRAY) || !PlanAdd(plan, off + 0x1C4, 0x10, REGION_U32_ARRAY) ||
            !PlanAdd(plan, off + 0x1FE, 2, REGION_U16_ARRAY) || !PlanAdd(plan, off + 0x204, 4, REGION_PTR_ARRAY) ||
            !PlanAdd(plan, off + 0x208, 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        if (LevelSpan(Be32(ent + 0x204), 0x10, imageSize, &o) && !PlanHas(plan, o, REGION_PTR_ARRAY)) {
            u32 ids = Be32(image + o + 4) != 0 ? Be32(image + o + 4) : Be32(image + o + 8), list;
            if (!PlanAdd(plan, o, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, o + 4, 12, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
            if (ids != 0 && ids <= imageSize / 4 && LevelSpan(Be32(image + o), (u64)ids * 4, imageSize, &list) &&
                !PlanHas(plan, list, REGION_U32_ARRAY) && !PlanAdd(plan, list, ids * 4, REGION_U32_ARRAY)) return SIR_ERR_ENTITY;
        }
    }
    // More part containers.
    u32 extra = Be32(ent + 0x1A4);
    if (extra != 0 && LevelSpan(Be32(ent + 0x194), (u64)extra * 4, imageSize, &o)) {
        // A list of one can be the entity's own slot at +0x8C, which is converted with the entity.
        bool ownSlot = extra == 1 && o == off + 0x8C;
        if (!ownSlot && !PlanAdd(plan, o, extra * 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        for (u32 i = 0; i < extra; i++)
            if ((e = EntityContainerPlan(plan, image, imageSize, Be32(image + o + i * 4))) != SIR_OK) return e;
    }
    // Children: nodes {+4 entity, +8 next}.
    u32 node = Be32(ent + 0xB0), guard = imageSize / 12 + 1;
    while (LevelSpan(node, 0xC, imageSize, &o)) {
        if (guard-- == 0) return SIR_ERR_ENTITY;                 // a list that loops
        if (PlanHas(plan, o + 4, REGION_PTR_ARRAY)) break;
        if (!PlanAdd(plan, o + 4, 8, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
        if ((e = EntityPlan(plan, image, imageSize, Be32(image + o + 4), depth + 1)) != SIR_OK) return e;
        node = Be32(image + o + 8);
    }
    return SIR_OK;
}

// ---- zone record of a level zone
//
// A zone slot's +0xFC points at the zone record, at the start of the zone's image. WorldTick
// walks its list at +0x10 (ParticleSystemFirst): objects linked through a node at their +0x10.
// Each has eight animation players (+0x5C) with an output each (+0x3C), and one more player at
// +0x7C. In the level files these are materials whose texture matrix is animated.
// Only the list at +0x10 has a known reader; the record's other lists are not converted.
// Regions: the node's next pointer (+0x10, code: WorldTick); +0x08 (handed to the per-type function,
// ParticleSystemUpdateExtra) and +0x3C..+0x7F (8 outputs, 8 players, the player at +0x7C) are
// plain pointer arrays.
static const SirField kZoneSystemFields[] = {
    { 0x00, SIR_PTR, 1 },
};
// Animation player (fn_8016A64C, fn_8016A43C, fn_8016A788). Key pointers follow at +0x28, one
// per channel; the elements (one per channel, `stride` bytes) are at +[+0x08].
static const SirField kAnimPlayerFields[] = {
    { 0x04, SIR_U32, 2 },     // code: mode (0 or 1), offset of the elements
    { 0x0C, SIR_U32, 1 },     // code: how many channels it has room for (AnimPlayerBind in draft_miss163.cpp, fn_8016B478)
    { 0x10, SIR_U32, 1 },     // code: channel count
    { 0x14, SIR_PTR, 1 },     // code: track
    { 0x18, SIR_U32, 3 },     // code: element stride (ParticleEmitterUpdate), wrapped flag, paused flag
    { 0x24, SIR_F32, 1 },     // code: time
};
// Element of a mode 1 player: an id (ParticleSystemUpdateExtra) and at +8 the value, which
// fn_8016A788 copies as a word from the key and ParticleEmitterUpdate reads as a float.
static const SirField kAnimElemFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x08, SIR_U32, 1 },
};
// Track, from +0x20: length (fn_8016A64C) and the offset of its channel table, whose 8-byte
// entries start with the offset of the channel's keys.
static const SirField kAnimTrackFields[] = {
    { 0x00, SIR_F32, 1 }, { 0x04, SIR_U32, 1 },
};

// A clip: the track of a mode 0 player (a skeleton's animation). AnimPlayerBind reads +0x10 (how
// many players use it) and +0x18 (channels); +0x20 is the length and +0x24 the offset of the
// channel table, as in every track; +0x2C is the offset of six floats, scale and offset of the
// positions (fn_8016A788). A channel is a run of 16-byte keys of eight 16-bit numbers {position
// x y z, time, rotation x y z w}; fn_8016A43C steps on while the next key's time (+0x16) is below
// the position, so the run ends with a key at time 0xFFFF. The second word of a table entry
// (the key count) has no reader.
static SirError AnimClipPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 t;
    if (!LevelSpan(addr, 0x30, imageSize, &t) || PlanHas(plan, t + 0x20, REGION_ANIM_TRACK)) return SIR_OK;
    if (!PlanAdd(plan, t + 0x10, 4, REGION_U32_ARRAY) || !PlanAdd(plan, t + 0x18, 4, REGION_U32_ARRAY) ||
        !PlanAdd(plan, t + 0x20, 8, REGION_ANIM_TRACK) || !PlanAdd(plan, t + 0x2C, 4, REGION_U32_ARRAY)) return SIR_ERR_ZONE_RECORD;
    u32 channels = Be32(image + t + 0x18);
    u64 scale = (u64)t + Be32(image + t + 0x2C), table = (u64)t + Be32(image + t + 0x24);
    if (channels > imageSize / 8 || scale + 0x18 > imageSize || table + (u64)channels * 8 > imageSize ||
        !PlanAdd(plan, (u32)scale, 0x18, REGION_F32_ARRAY)) return SIR_ERR_ZONE_RECORD;
    for (u32 i = 0; i < channels; i++) {
        u32 entry = (u32)table + i * 8;
        if (!PlanAdd(plan, entry, 4, REGION_U32_ARRAY)) return SIR_ERR_ZONE_RECORD;
        u64 first = (u64)t + Be32(image + entry), k = first;
        for (;; k += 0x10) {
            if (k + 0x10 > imageSize) return SIR_ERR_ZONE_RECORD;
            if (image[k + 6] == 0xFF && image[k + 7] == 0xFF) break;
        }
        if (!PlanAdd(plan, (u32)first, (u32)(k + 0x10 - first), REGION_U16_ARRAY)) return SIR_ERR_ZONE_RECORD;
    }
    return SIR_OK;
}

// A mode 1 channel is a run of 8-byte keys {u16 time, -, value}; fn_8016A43C steps to the next
// key while that one's time is below the position (at most 0xFFFF), so the run ends with a key
// at time 0xFFFF.
static SirError AnimPlayerPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off, t;
    if (!LevelSpan(addr, ANIM_PLAYER_HEAD, imageSize, &off) || PlanHas(plan, off, REGION_ANIM_PLAYER)) return SIR_OK;
    const u8* p = image + off;
    u32 mode = Be32(p + 0x04), elems = Be32(p + 0x08), count = Be32(p + 0x10), stride = Be32(p + 0x18);
    if (count > imageSize / 4 || (u64)off + ANIM_PLAYER_HEAD + (u64)count * 4 > imageSize) return SIR_ERR_ZONE_RECORD;
    if (!PlanAdd(plan, off, ANIM_PLAYER_HEAD, REGION_ANIM_PLAYER)) return SIR_ERR_ZONE_RECORD;
    if (count != 0 && !PlanAdd(plan, off + ANIM_PLAYER_HEAD, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_ZONE_RECORD;
    if (mode == 0) return AnimClipPlan(plan, image, imageSize, Be32(p + 0x14));   // its elements are written (fn_8016A570)
    if (mode != 1) return SIR_OK;
    if (stride < 0xC || elems < ANIM_PLAYER_HEAD + count * 4 || (u64)off + elems + (u64)count * stride > imageSize) return SIR_ERR_ZONE_RECORD;
    for (u32 i = 0; i < count; i++)
        if (!PlanAdd(plan, off + elems + i * stride, 0xC, REGION_ANIM_ELEM)) return SIR_ERR_ZONE_RECORD;
    // Players can share a track: every one gets the channels of its own count checked.
    if (!LevelSpan(Be32(p + 0x14), 0x28, imageSize, &t)) return SIR_OK;
    if (!PlanHas(plan, t + 0x20, REGION_ANIM_TRACK) && !PlanAdd(plan, t + 0x20, 8, REGION_ANIM_TRACK)) return SIR_ERR_ZONE_RECORD;
    u64 table = (u64)t + Be32(image + t + 0x24);
    if (table + (u64)count * 8 > imageSize) return SIR_ERR_ZONE_RECORD;
    for (u32 i = 0; i < count; i++) {
        u32 entry = (u32)table + i * 8;
        if (!PlanAdd(plan, entry, 4, REGION_U32_ARRAY)) return SIR_ERR_ZONE_RECORD;
        u64 first = (u64)t + Be32(image + entry), k = first;
        for (;; k += 8) {
            if (k + 8 > imageSize) return SIR_ERR_ZONE_RECORD;
            if (image[k] == 0xFF && image[k + 1] == 0xFF) break;
        }
        if (!PlanAdd(plan, (u32)first, (u32)(k + 8 - first), REGION_ANIM_KEYS)) return SIR_ERR_ZONE_RECORD;
    }
    return SIR_OK;
}

// A prop's animation object (prop +0xA0), as fn_8018B2CC, fn_8018A948 and fn_8018AF28 read it:
//   object: +0x00 its skeleton; three 0x24-byte slots from +0x00, each with a skeleton at +0x08 and
//           a block at +0x1C that fn_8018A78C frees; +0x74 / +0x78 / +0x7C three players, +0x80 / +0x84 two indices,
//           +0x88..+0xB0 times and speeds, +0xA0 / +0xA4 two bone indices, +0xC0..+0xE8 floats and
//           a word at +0xD8, +0xEC a word, +0xF0 flags. The byte at +0x8C needs no conversion.
//   skeleton: +0x04 its bones (0x30 bytes each, twelve floats), +0x08 their count, +0x0C one 8-byte
//           entry per bone whose second word says how the walk goes on (fn_80173944), +0x14 its
//           player, +0x18 a block that is freed with it (fn_8017493C), +0x1C the matrix the walk
//           starts from (fn_80173944). A scene item's +0xB0 is such a skeleton (fn_80172708).
//           +0x00 a pointer to one 0x30-byte matrix (twelve floats) per bone: a[i] of fn_801704F8,
//           which multiplies it with b[i] from the bones at +0x04 (the same block as +0x18 in the
//           files).
static const SirField kAnimObjFields[] = {
    { 0x00, SIR_PTR, 1 }, { 0x08, SIR_PTR, 1 }, { 0x1C, SIR_PTR, 1 }, { 0x2C, SIR_PTR, 1 }, { 0x40, SIR_PTR, 1 },
    { 0x50, SIR_PTR, 1 }, { 0x64, SIR_PTR, 1 }, { 0x74, SIR_PTR, 3 }, { 0x80, SIR_U32, 2 }, { 0x88, SIR_F32, 1 }, { 0x90, SIR_F32, 2 },
    { 0x9C, SIR_F32, 1 }, { 0xA0, SIR_U32, 2 }, { 0xA8, SIR_F32, 3 }, { 0xC0, SIR_F32, 6 }, { 0xD8, SIR_U32, 1 },
    { 0xDC, SIR_F32, 4 }, { 0xEC, SIR_U32, 2 },
};
#define ANIM_OBJ_USED 0xF4
// The skeleton from +0x04 on.
static const SirField kSkeletonFields[] = {
    { 0x00, SIR_PTR, 1 }, { 0x04, SIR_U32, 1 }, { 0x08, SIR_PTR, 1 }, { 0x10, SIR_PTR, 3 },
};
#define SKELETON_USED 0x20

static SirError SkeletonPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 s, b;
    if (!LevelSpan(addr, SKELETON_USED, imageSize, &s) || PlanHas(plan, s + 4, REGION_SKELETON)) return SIR_OK;
    if (!PlanAdd(plan, s + 4, SKELETON_USED - 4, REGION_SKELETON) || !PlanAdd(plan, s, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
    u32 bones = Be32(image + s + 8);
    if (bones != 0) {
        if (LevelSpan(Be32(image + s), (u64)bones * 0x30, imageSize, &b) && !PlanHas(plan, b, REGION_F32_ARRAY) &&
            !PlanAdd(plan, b, bones * 0x30, REGION_F32_ARRAY)) return SIR_ERR_ENTITY;
        if (bones > imageSize / 0x30 || !LevelSpan(Be32(image + s + 4), (u64)bones * 0x30, imageSize, &b)) return SIR_ERR_ENTITY;
        if (!PlanHas(plan, b, REGION_F32_ARRAY) && !PlanAdd(plan, b, bones * 0x30, REGION_F32_ARRAY)) return SIR_ERR_ENTITY;
        if (!LevelSpan(Be32(image + s + 0xC), (u64)bones * 8, imageSize, &b)) return SIR_ERR_ENTITY;
        if (!PlanHas(plan, b, REGION_BONE_OPS) && !PlanAdd(plan, b, bones * 8, REGION_BONE_OPS)) return SIR_ERR_ENTITY;
    }
    if (LevelSpan(Be32(image + s + 0x1C), 0x30, imageSize, &b) && !PlanHas(plan, b, REGION_F32_ARRAY) &&
        !PlanAdd(plan, b, 0x30, REGION_F32_ARRAY)) return SIR_ERR_ENTITY;
    return AnimPlayerPlan(plan, image, imageSize, Be32(image + s + 0x14));
}

static SirError AnimObjPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 a;
    SirError e;
    if (!LevelSpan(addr, ANIM_OBJ_USED, imageSize, &a) || PlanHas(plan, a, REGION_ANIM_OBJ)) return SIR_OK;
    if (!PlanAdd(plan, a, ANIM_OBJ_USED, REGION_ANIM_OBJ)) return SIR_ERR_ENTITY;
    for (u32 k = 0; k < 3; k++) {
        if ((e = AnimPlayerPlan(plan, image, imageSize, Be32(image + a + 0x74 + k * 4))) != SIR_OK) return e;
        if ((e = SkeletonPlan(plan, image, imageSize, Be32(image + a + 0x08 + k * 0x24))) != SIR_OK) return e;
    }
    return SkeletonPlan(plan, image, imageSize, Be32(image + a));
}

// Walks a zone record's list from its first node. Every zone's record is in zone 0's image; the
// objects are in the record's own zone, so the walk ends at a node outside this image.
static SirError ZoneListPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 node) {
    u32 o, q, guard = imageSize / ZONE_SYSTEM_USED + 1;
    SirError e;
    while (node >= sImageBase && node - sImageBase < imageSize) {
        if (guard-- == 0 || node < sImageBase + 0x10 || !ImageSpan(node - 0x10, ZONE_SYSTEM_USED, imageSize, &o)) return SIR_ERR_ZONE_RECORD;
        if (PlanHas(plan, o + 0x10, REGION_ZONE_SYSTEM)) return SIR_ERR_ZONE_RECORD;       // a list that loops
        // The object is a material (MaterialPlan adds the rest of it): only the fields WorldTick reads are regions here.
        if (!PlanAdd(plan, o + 0x10, 4, REGION_ZONE_SYSTEM) || !PlanAdd(plan, o + 0x08, 4, REGION_PTR_ARRAY) ||
            !PlanAdd(plan, o + 0x3C, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, o + 0x40, 0x40, REGION_PTR_ARRAY)) return SIR_ERR_ZONE_RECORD;
        for (u32 k = 0; k < 8; k++) {
            u32 player = Be32(image + o + 0x5C + k * 4);
            if (player == 0) continue;
            // The player's output: twelve floats, all written by ParticleEmitterUpdate.
            if (LevelSpan(Be32(image + o + 0x3C + k * 4), 0x30, imageSize, &q) && !PlanAdd(plan, q, 0x30, REGION_F32_ARRAY)) return SIR_ERR_ZONE_RECORD;
            if ((e = AnimPlayerPlan(plan, image, imageSize, player)) != SIR_OK) return e;
        }
        if ((e = AnimPlayerPlan(plan, image, imageSize, Be32(image + o + 0x7C))) != SIR_OK) return e;
        node = Be32(image + o + 0x10);
    }
    return SIR_OK;
}

static SirError ZoneRecordPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!LevelSpan(addr, 0x14, imageSize, &off) || PlanHas(plan, off + 0x10, REGION_PTR_ARRAY)) return SIR_OK;
    if (!PlanAdd(plan, off + 0x10, 4, REGION_PTR_ARRAY)) return SIR_ERR_ZONE_RECORD;
    return ZoneListPlan(plan, image, imageSize, Be32(image + off + 0x10));
}

// ---- screen set database (header slot 0xDC0, the game's lbl_805FB218)
//
// UIEngine_LoadUIScreenSet looks a screen set up by name hash: fn_801EBE74 does a binary search
// in the table object at +0x20 of the database, {entries, count}, whose 8-byte entries are
// {key, record}; of the record it reads the zone at +0x44. The records are 0x50 bytes with the
// set's name in front; nothing else of them, or of the database object, has a known reader.
#define SCREEN_SET_RECORD_USED 0x48
static SirError ScreenSetDbPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off, t, e, rec;
    if (!ImageSpan(addr, 0x24, imageSize, &off) || !PlanAdd(plan, off + 0x20, 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREEN_SETS;
    u32 table = Be32(image + off + 0x20);
    if (table == 0) return SIR_OK;
    if (!ImageSpan(table, 8, imageSize, &t) || !PlanAdd(plan, t, 8, REGION_PTR_COUNT)) return SIR_ERR_SCREEN_SETS;
    u32 count = Be32(image + t + 4);
    if (count == 0) return SIR_OK;
    if (count > imageSize / 8 || !ImageSpan(Be32(image + t), (u64)count * 8, imageSize, &e) ||
        !PlanAdd(plan, e, count * 8, REGION_KEY_PTR)) return SIR_ERR_SCREEN_SETS;
    for (u32 i = 0; i < count; i++) {
        // The search needs the keys in ascending order.
        if (i != 0 && Be32(image + e + i * 8) <= Be32(image + e + (i - 1) * 8)) return SIR_ERR_SCREEN_SETS;
        if (!ImageSpan(Be32(image + e + i * 8 + 4), SCREEN_SET_RECORD_USED, imageSize, &rec) ||
            !PlanAdd(plan, rec + 0x44, 4, REGION_U32_ARRAY)) return SIR_ERR_SCREEN_SETS;
    }
    return SIR_OK;
}

// A locator (header +0x28, count at +0x24), 0x20 bytes: +0x00 the hash of its name
// (WorldFindLocator), +0x08 position and +0x14 rotation (PlaceOnTableLocator in draft_miss182.cpp).
// +0x04 is the locator's zone (fn_801EDF58, for a node reference of kind 4).
static const SirField kLocatorFields[] = {
    { 0x00, SIR_U32, 2 }, { 0x08, SIR_F32, 6 },
};
#define LOCATOR_SIZE 0x20

// A zone slot's placed effects (slot +0x80, count at +0x9C): pointers to 0x20-byte records
// {+0x00 the effect's name hash, +0x04 position, +0x10 direction, +0x1C the running instance},
// which WorldOnZoneLoaded hands to SpawnEffectAt.
static const SirField kZoneEffectFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_F32, 6 }, { 0x1C, SIR_PTR, 1 },
};
#define ZONE_EFFECT_SIZE 0x20

// ---- screen set of a shared zone (zone slot +0x100)
//
// fn_80135BC0 hands the slot's +0x100 to fn_80199024, which walks the binary tree behind the
// table at +0x28 (fn_8018E2F4: table +0 is the root, +4 and +8 the nodes with the lowest and the
// highest key, which fn_8018E808 compares a searched key with; a node is {+0x00 key, +0x04 widget,
// +0x0C left, +0x10 right}) and for each node's widget its children (fn_8019BFF8: first child at
// +0x120, next sibling at +0x124; fn_80199534 reads a child's first word). Of the set itself that
// is all that is typed so far; of the widgets, the part every kind has (kUiWidgetFields).
static const SirField kUiTreeNodeFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_PTR, 1 }, { 0x0C, SIR_PTR, 2 },
};
#define UI_WIDGET_USED 0x138
#define UI_DEPTH_MAX   256

// The part every widget has, up to the vtable slot at +0x138 that the game fills in. The bytes at
// +0xB8 / +0xB9 (a cell of the grid, fn_8019CBEC) need no conversion. No reader yet, left alone:
// the words at +0xAC and +0xB4 (fn_8019B2F4 sets them like the action words), +0x12C, +0x134.
static const SirField kUiWidgetFields[] = {
    { 0x000, SIR_U32, 1 },    // code: type, the hash of its name (fn_80199534, fn_80198C78)
    { 0x004, SIR_PTR, 3 },    // code: its actions, animations and sounds by event (fn_8019BD18)
    { 0x010, SIR_PTR, 1 },    // code: animation player (fn_8019B6CC)
    { 0x014, SIR_F32, 16 },   // code: start values of +0x68..+0xA4 (fn_8019B4E8)
    { 0x054, SIR_U32, 1 },    // code: start flags (fn_8019B4E8)
    { 0x058, SIR_U32, 1 },    // code: handed to fn_8019645C before the widget draws (fn_8019B7D8)
    { 0x05C, SIR_F32, 1 },    // code: draw order (fn_80199540)
    { 0x060, SIR_U32, 2 },    // code: kind (fn_8019914C), flags (fn_8019B6CC)
    { 0x068, SIR_F32, 16 },   // code: position, scale (fn_8019C348), colour (fn_801A05E4), rotation, pivot (fn_8019C348)
    { 0x0A8, SIR_U32, 1 },    // code: action word (fn_80199DD4)
    { 0x0B0, SIR_U32, 1 },    // code: action word (fn_8019B6CC)
    { 0x0BC, SIR_PTR, 1 },    // code: activation grid (fn_8019CBEC)
    { 0x0C0, SIR_F32, 24 },   // code: two 3x4 matrices (fn_8019C348 reads the parent's +0xC0, fn_8019DC58 +0xF0)
    { 0x120, SIR_PTR, 3 },    // code: first child, next sibling (fn_8019BFF8), parent (fn_8019C348)
    { 0x130, SIR_PTR, 1 },    // code: a widget drawn before this one (fn_801985BC)
};
// A node of the three event lists: next, event, and an action word, animation hash or sound hash
// (fn_8019BD18).
static const SirField kUiEventFields[] = {
    { 0x00, SIR_PTR, 1 }, { 0x04, SIR_U32, 2 },
};
#define UI_EVENT_SIZE 0x0C
// An activation grid: +0x00 flags (fn_80199E80), +0x04 a list of columns, each a list of pointers
// to widgets; +0x0C..+0x18 the current and the previous cell (fn_8019AA3C). +0x08 is written
// before it is read.
static const SirField kUiGridFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_PTR, 1 }, { 0x0C, SIR_U32, 4 },
};
#define UI_GRID_USED 0x1C
// The screen set itself. Its four lookup tables are trees like the one of the screens
// (fn_8018E8D8); what the nodes of the other three point at is not typed yet. +0x40..+0x60 is the
// stack of screens, which the game fills.
static const SirField kUiSetFields[] = {
    { 0x00, SIR_U32, 1 },     // code: the set's name hash (ShowSelectionHUD and SetPartSelected compare it with "UI_MSA_Combat_HUD",
                              //       GameSessionUpdate with "UI_MSA_Loading")
    { 0x04, SIR_U32, 1 },     // code: the set's zone (fn_80198438)
    { 0x08, SIR_U32, 2 },     // code: draw list: capacity and count (fn_80199CE8, fn_801985BC),
    { 0x10, SIR_PTR, 1 },     //       and its entries, which the game writes (fn_8019B860)
    { 0x20, SIR_PTR, 1 },     // code: table of animations (fn_80198F1C)
    { 0x28, SIR_PTR, 1 },     // code: table of screens (fn_80199024, ScreenSetFindWidget)
    { 0x34, SIR_PTR, 1 },     // code: a table (fn_80198F74)
    { 0x3C, SIR_PTR, 1 },     // code: a table (fn_80198FCC)
};
#define UI_SET_USED 0x40
// What a screen (the widget of a node of the set's screen table) has from +0x13C on: how many
// named widgets it has and pointers to them (UIWidgetFindChild), its activation grid
// (fn_80199ABC), a float handed on when the screen comes to the top (fn_801987BC, fn_80198C78)
// and a handle (fn_80199C30). The byte at +0x150 (fn_80199684) needs no conversion.
static const SirField kUiScreenFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_PTR, 2 }, { 0x0C, SIR_F32, 1 }, { 0x10, SIR_U32, 1 },
};
#define UI_SCREEN_AT   0x13C
#define UI_SCREEN_SIZE 0x14
enum { UI_LIST_PLAIN, UI_LIST_FIXUP, UI_LIST_LISTS, UI_LIST_WIDGETS };
// What a picture (kind 2, sub-kind 0, vtable lbl_80382E2C) has from +0x140 on that is read before
// the game writes it: +0x140 a pointer to the word that holds its texture (fn_801A0D04,
// fn_801A1208), +0x144 a texture set by a script, which wins (fn_801A0D04), +0x148 its size
// (fn_801A0DF0, fn_801A0DF8) and +0x180 the texture coordinates of its four corners
// (fn_801A1238). The corner colours (+0x1A0), the quad (+0x1E0), the matrix (+0x240) and the
// tinted colour (+0x270) are written first (fn_801A1238, fn_801A05E4).
static const SirField kUiGraphicFields[] = {
    { 0x00, SIR_PTR, 2 }, { 0x08, SIR_F32, 2 }, { 0x40, SIR_F32, 8 },
};
#define UI_GRAPHIC_AT   0x140
#define UI_GRAPHIC_SIZE 0x60
// What a text (kind 1, vtable lbl_80382DA4) has from +0x13C on, offsets from +0x13C:
//   +0x13C string table (fn_8019CF58), +0x140 markers of the text (fn_8019EB94), +0x144 the
//   current one (fn_8019E838); +0x148 a byte; +0x14C a time (fn_8019E838);
//   +0x150 length (fn_8019EB94), +0x154 / +0x158 (fn_8019D5E8), +0x15C / +0x160 (fn_8019F5A8),
//   +0x164, +0x168 capacity (UISetText), +0x16C text id (fn_8019CF58);
//   +0x170 the text, +0x174 the string it was set from (UISetText, fn_8019EB94), +0x178 a word
//   per character (fn_8019E27C; all bits set in the file, so the words themselves are left);
//   +0x1B0, +0x1B4, +0x1D4 floats (fn_8019F810); +0x1E0 a second buffer (fn_8019D5E8),
//   +0x1E4, +0x1E8, +0x1EC words (fn_8019D5E8, UISetText). +0x1F0 is a byte.
// The matrix at +0x17C is written before it is read (fn_8019CF58).
static const SirField kUiTextFields[] = {
    { 0x00, SIR_PTR, 3 }, { 0x10, SIR_F32, 1 }, { 0x14, SIR_U32, 8 }, { 0x34, SIR_PTR, 3 },
    { 0x74, SIR_F32, 2 }, { 0x98, SIR_F32, 1 }, { 0xA4, SIR_PTR, 1 }, { 0xA8, SIR_U32, 3 },
};
#define UI_TEXT_AT   0x13C
#define UI_TEXT_SIZE 0xB4

// What a slider (kind 3, sub-kind 0-2 at +0x13C) has from +0x13C on, offsets from +0x13C: the target
// value (+0x140), the shown one and the speed it moves at (fn_801A4414: +0x144, +0x158), floats
// (+0x148 to +0x154), the list of its picture widgets (+0x15C, fixed up by fn_8019914C), the fill
// picture among them (+0x160), its scale (+0x164) and two floats that hold the "not set" NaN
// (+0x168, +0x16C). The widget ends at +0x170, where its list begins.
static const SirField kUiSliderFields[] = {
    { 0x00, SIR_U32, 1 }, { 0x04, SIR_F32, 7 }, { 0x20, SIR_PTR, 2 }, { 0x28, SIR_F32, 3 },
};
#define UI_SLIDER_SIZE 0x34

// A list of the UI code: +0x00 count (fn_8019555C), +0x04 capacity, +0x08 element size and +0x10
// the elements (fn_8019552C). The widget lists hold pointers.
static const SirField kUiListFields[] = {
    { 0x00, SIR_U32, 3 }, { 0x10, SIR_PTR, 1 },
};

static SirError UiFixupPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 depth);

static SirError UiBasePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 depth);

static SirError UiListPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, int mode, u32 depth) {
    u32 l, d;
    SirError e;
    if (!LevelSpan(addr, 0x14, imageSize, &l) || PlanHas(plan, l, REGION_UI_LIST)) return SIR_OK;
    if (!PlanAdd(plan, l, 0x14, REGION_UI_LIST)) return SIR_ERR_SCREENS;
    u32 count = Be32(image + l), capacity = Be32(image + l + 4), size = Be32(image + l + 8);
    if (count == 0) return SIR_OK;
    if (size != 4 || count > capacity || count > imageSize / 4 || !LevelSpan(Be32(image + l + 0x10), (u64)count * 4, imageSize, &d) ||
        !PlanAdd(plan, d, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
    for (u32 i = 0; mode != UI_LIST_PLAIN && i < count; i++) {
        u32 item = Be32(image + d + i * 4);
        e = mode == UI_LIST_FIXUP ? UiFixupPlan(plan, image, imageSize, item, depth + 1)
          : mode == UI_LIST_LISTS ? UiListPlan(plan, image, imageSize, item, UI_LIST_PLAIN, depth + 1)
          : UiBasePlan(plan, image, imageSize, item, depth + 1);
        if (e != SIR_OK) return e;
    }
    return SIR_OK;
}

static SirError UiGridPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 depth) {
    u32 g;
    if (!LevelSpan(addr, UI_GRID_USED, imageSize, &g) || PlanHas(plan, g, REGION_UI_GRID)) return SIR_OK;
    if (!PlanAdd(plan, g, UI_GRID_USED, REGION_UI_GRID)) return SIR_ERR_SCREENS;
    return UiListPlan(plan, image, imageSize, Be32(image + g + 4), UI_LIST_LISTS, depth);
}

// The part every widget has, with its event lists, its activation grid and the widget drawn
// before it. Once per widget.
static SirError UiBasePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 depth) {
    u32 o, n;
    SirError e;
    if (!LevelSpan(addr, UI_WIDGET_USED, imageSize, &o) || PlanHas(plan, o, REGION_UI_WIDGET)) return SIR_OK;
    if (depth > UI_DEPTH_MAX || !PlanAdd(plan, o, UI_WIDGET_USED, REGION_UI_WIDGET)) return SIR_ERR_SCREENS;
    for (u32 k = 0; k < 3; k++) {
        u32 node = Be32(image + o + 4 + k * 4), guard = imageSize / UI_EVENT_SIZE + 1;
        while (LevelSpan(node, UI_EVENT_SIZE, imageSize, &n) && !PlanHas(plan, n, REGION_UI_EVENT)) {
            if (guard-- == 0 || !PlanAdd(plan, n, UI_EVENT_SIZE, REGION_UI_EVENT)) return SIR_ERR_SCREENS;
            node = Be32(image + n);
        }
    }
    if ((e = UiGridPlan(plan, image, imageSize, Be32(image + o + 0xBC), depth)) != SIR_OK) return e;
    if (Be32(image + o + 0x60) == 2 && (u64)o + UI_GRAPHIC_AT + UI_GRAPHIC_SIZE <= imageSize && Be32(image + o + 0x13C) == 0) {
        u32 t;
        if (!PlanAdd(plan, o + UI_GRAPHIC_AT, UI_GRAPHIC_SIZE, REGION_UI_GRAPHIC)) return SIR_ERR_SCREENS;
        // The word the picture's +0x140 points at: its texture, shared by the pictures that show it.
        if (LevelSpan(Be32(image + o + 0x140), 4, imageSize, &t) && !PlanHas(plan, t, REGION_PTR_ARRAY) &&
            !PlanAdd(plan, t, 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        // The texture records behind it (fn_801A0D04 draws with the one at +0x144, or with the one in the word at +0x140;
        // fn_80195A58 reads the header). A word that holds no texture record is left alone.
        if (LevelSpan(Be32(image + o + 0x140), 4, imageSize, &t)) (void)MaterialTexturePlan(plan, image, imageSize, Be32(image + t));
        (void)MaterialTexturePlan(plan, image, imageSize, Be32(image + o + 0x144));
    }
    if (Be32(image + o + 0x60) == 1 && (u64)o + UI_TEXT_AT + UI_TEXT_SIZE <= imageSize &&
        !PlanAdd(plan, o + UI_TEXT_AT, UI_TEXT_SIZE, REGION_UI_TEXT)) return SIR_ERR_SCREENS;
    return UiBasePlan(plan, image, imageSize, Be32(image + o + 0x130), depth + 1);
}

// A slideshow (kind 2, sub-kind 4; vtable lbl_80382EA0, its name "UISlideshow"): +0x140 a holder
// {+0x00 a hash, +0x04 a list of texture pointers} (fn_801A21CC hands the element at the index
// +0x148 to UISetTexture of the widget at +0x164, fn_8019AC00 reads the list), +0x148 / +0x14C /
// +0x150 indices and a flag, +0x154..+0x160 floats (fn_801A1F40, fn_801A21CC; +0x15C and +0x160
// are written first), and the two picture widgets at +0x164 and +0x168 (fn_801A2330).
static SirError UiSlideshowPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 o) {
    u32 h, l, d;
    SirError e;
    if ((u64)o + 0x16C > imageSize || !PlanAdd(plan, o + 0x140, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, o + 0x148, 0x1C, REGION_U32_ARRAY))
        return SIR_ERR_SCREENS;
    if (!LevelSpan(Be32(image + o + 0x140), 8, imageSize, &h) || PlanHas(plan, h + 4, REGION_PTR_ARRAY)) return SIR_OK;
    if (!PlanAdd(plan, h + 4, 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
    u32 list = Be32(image + h + 4);
    if ((e = UiListPlan(plan, image, imageSize, list, UI_LIST_PLAIN, 0)) != SIR_OK) return e;
    if (!LevelSpan(list, 0x14, imageSize, &l)) return SIR_OK;
    u32 count = Be32(image + l);
    if (count == 0 || !LevelSpan(Be32(image + l + 0x10), (u64)count * 4, imageSize, &d)) return SIR_OK;
    for (u32 i = 0; i < count; i++)
        if ((e = MaterialTexturePlan(plan, image, imageSize, Be32(image + d + i * 4))) != SIR_OK) return SIR_ERR_SCREENS;
    return SIR_OK;
}

// A grid (kind 4, update fn_801A4994): what it has from +0x13C on besides the two widget pointers.
//   +0x13C rows, +0x140 columns, +0x144 / +0x148 the rows and columns shown (fn_801A72EC), +0x14C / +0x150 the
//   selected cell, +0x154 / +0x158 floats, +0x15C / +0x160 the scroll offset, +0x164 a flag (11 words);
//   +0x170 / +0x174 floats (the fade timer, fn_801A4994); +0x180 / +0x184 the cell size and the floats up to
//   +0x1B0 (fn_801A6D7C); +0x1B8 a list with one record of floats per row (0x34 bytes; fn_801A6D7C reads
//   the first through fn_8019552C).
// +0x168 and +0x16C hold stray bytes in the file, +0x178..+0x17C are bytes: those are left alone.
static SirError UiGridFieldsPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 o) {
    u32 l, d;
    if (!PlanAdd(plan, o + 0x13C, 0x2C, REGION_U32_ARRAY) || !PlanAdd(plan, o + 0x170, 8, REGION_F32_ARRAY) ||
        !PlanAdd(plan, o + 0x180, 0x34, REGION_F32_ARRAY) || !PlanAdd(plan, o + 0x1B8, 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
    if (!LevelSpan(Be32(image + o + 0x1B8), 0x14, imageSize, &l) || PlanHas(plan, l, REGION_UI_LIST)) return SIR_OK;
    if (!PlanAdd(plan, l, 0x14, REGION_UI_LIST)) return SIR_ERR_SCREENS;
    u32 count = Be32(image + l), capacity = Be32(image + l + 4), size = Be32(image + l + 8);
    if (count == 0) return SIR_OK;
    if ((size & 3) || size == 0 || count > capacity || (u64)count * size > imageSize ||
        !LevelSpan(Be32(image + l + 0x10), (u64)count * size, imageSize, &d) ||
        !PlanAdd(plan, d, count * size, REGION_F32_ARRAY)) return SIR_ERR_SCREENS;
    return SIR_OK;
}

// What fn_8019914C reads of a widget to install the vtable for its kind: +0x60 the kind; for kinds
// 2 and 3 the sub-kind at +0x13C; kind 3 a list at +0x15C (its entries only get a vtable); kind 4
// a widget at +0x1B4 and a list of widgets at +0x1BC; kind 5 a widget at +0x190; kind 6 its
// children. Those widgets go through the same function.
static SirError UiFixupPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 depth) {
    u32 o, co;
    SirError e = SIR_OK;
    if (!LevelSpan(addr, UI_WIDGET_USED, imageSize, &o) || PlanHas(plan, o, REGION_UI_FIXED)) return SIR_OK;
    if (depth > UI_DEPTH_MAX || !PlanAdd(plan, o, 0, REGION_UI_FIXED)) return SIR_ERR_SCREENS;           // an empty marker
    if ((e = UiBasePlan(plan, image, imageSize, addr, depth)) != SIR_OK) return e;
    u32 kind = Be32(image + o + 0x60);
    u32 need = kind == 2 ? 0x140 : kind == 3 ? 0x160 : kind == 4 ? 0x1C0 : kind == 5 ? 0x194 : 0;
    if ((u64)o + need > imageSize) return SIR_ERR_SCREENS;
    if (kind == 2 && !PlanAdd(plan, o + 0x13C, 4, REGION_U32_ARRAY)) return SIR_ERR_SCREENS;
    if (kind == 2 && Be32(image + o + 0x13C) == 4) {
        e = UiSlideshowPlan(plan, image, imageSize, o);
    } else if (kind == 3) {
        if ((u64)o + 0x13C + UI_SLIDER_SIZE > imageSize || !PlanAdd(plan, o + 0x13C, UI_SLIDER_SIZE, REGION_UI_SLIDER)) return SIR_ERR_SCREENS;
        e = UiListPlan(plan, image, imageSize, Be32(image + o + 0x15C), UI_LIST_WIDGETS, depth);
    } else if (kind == 4) {
        if (!PlanAdd(plan, o + 0x1B4, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, o + 0x1BC, 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        if ((e = UiGridFieldsPlan(plan, image, imageSize, o)) != SIR_OK) return e;
        if ((e = UiFixupPlan(plan, image, imageSize, Be32(image + o + 0x1B4), depth + 1)) != SIR_OK) return e;
        e = UiListPlan(plan, image, imageSize, Be32(image + o + 0x1BC), UI_LIST_FIXUP, depth);
    } else if (kind == 5) {
        if (!PlanAdd(plan, o + 0x190, 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        e = UiFixupPlan(plan, image, imageSize, Be32(image + o + 0x190), depth + 1);
    } else if (kind == 6) {
        u32 c = Be32(image + o + 0x120), guard = imageSize / UI_WIDGET_USED + 1;
        while (e == SIR_OK && LevelSpan(c, UI_WIDGET_USED, imageSize, &co)) {
            if (guard-- == 0) return SIR_ERR_SCREENS;
            e = UiFixupPlan(plan, image, imageSize, c, depth + 1);
            c = Be32(image + co + 0x124);
        }
    }
    return e;
}

static SirError UiWidgetPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, bool child, u32 depth) {
    u32 o;
    SirError fix;
    if (!LevelSpan(addr, UI_WIDGET_USED, imageSize, &o)) return SIR_OK;
    if (depth > UI_DEPTH_MAX) return SIR_ERR_SCREENS;
    if ((fix = UiBasePlan(plan, image, imageSize, addr, depth)) != SIR_OK) return fix;
    if (child && (fix = UiFixupPlan(plan, image, imageSize, addr, depth)) != SIR_OK) return fix;   // fn_80199534 -> fn_8019914C
    // The children, once per widget.
    if (PlanHas(plan, o, REGION_RUNTIME)) return SIR_OK;                        // an empty marker at the widget
    if (!PlanAdd(plan, o, 0, REGION_RUNTIME)) return SIR_ERR_SCREENS;
    // A kind 2, sub-kind 4 widget: two widgets at +0x164 and +0x168, whose vtables fn_801A1B2C installs
    // (fn_8019914C installs this one's, by the sub-kind at +0x13C).
    if (Be32(image + o + 0x60) == 2 && Be32(image + o + 0x13C) == 4 && (u64)o + 0x16C <= imageSize) {
        if (!PlanAdd(plan, o + 0x164, 8, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        for (u32 k = 0; k < 2; k++)
            if ((fix = UiWidgetPlan(plan, image, imageSize, Be32(image + o + 0x164 + k * 4), false, depth + 1)) != SIR_OK) return fix;
    }
    u32 c = Be32(image + o + 0x120), guard = imageSize / UI_WIDGET_USED + 1, co;
    SirError e;
    while (LevelSpan(c, UI_WIDGET_USED, imageSize, &co)) {
        if (guard-- == 0) return SIR_ERR_SCREENS;                // a sibling list that loops
        if ((e = UiWidgetPlan(plan, image, imageSize, c, true, depth + 1)) != SIR_OK) return e;
        c = Be32(image + co + 0x124);
    }
    return SIR_OK;
}

// An animation (the value of a node of the set's table at +0x20). fn_801ABAF4 makes a player from
// it and fn_801AB4C4 copies it there:
//   animation: +0x04 five tracks (position, scale, colour, rotation, pivot: fn_801AB820), +0x18 its
//              events, +0x1C flags, +0x20 length, +0x24 start position, +0x28 speed, +0x2C a word
//   track:     +0x04 a list of keys, +0x08..+0x14 the cursor, copied as words (fn_801AA79C)
//   events:    +0x04 a list of entries, +0x08..+0x10 copied as words (fn_801AAEE4)
//   list:      as kUiListFields, with 0x30-byte elements in place (fn_8019552C)
//   key:       +0x00 time (fn_801AA728), +0x10 four floats (fn_801AA760), +0x20 the curve to the
//              next key (fn_801AA218)
//   entry:     +0x00 time, +0x24 kind, +0x10 an action word (kind 1), an event (2) or a sound (3),
//              which is handed on as a pointer (AnimEntryFire in draft_pz5d.cpp, fn_801AA784)
static const SirField kUiAnimFields[] = {
    { 0x04, SIR_PTR, 6 }, { 0x1C, SIR_U32, 1 }, { 0x20, SIR_F32, 3 }, { 0x2C, SIR_U32, 1 },
};
static const SirField kUiAnimTrackFields[] = {
    { 0x04, SIR_PTR, 1 }, { 0x08, SIR_U32, 1 }, { 0x0C, SIR_PTR, 2 }, { 0x14, SIR_U32, 1 },
};
static const SirField kUiAnimEventsFields[] = {
    { 0x04, SIR_PTR, 1 }, { 0x08, SIR_U32, 3 },
};
#define UI_ANIM_USED        0x30
#define UI_ANIM_TRACK_USED  0x18
#define UI_ANIM_EVENTS_USED 0x14
#define UI_ANIM_KEY_SIZE    0x30

// A list whose elements are 0x30-byte keys or entries.
static SirError UiAnimListPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u8 kind) {
    u32 l, d;
    if (!LevelSpan(addr, 0x14, imageSize, &l) || PlanHas(plan, l, REGION_UI_LIST)) return SIR_OK;
    if (!PlanAdd(plan, l, 0x14, REGION_UI_LIST)) return SIR_ERR_SCREENS;
    u32 count = Be32(image + l), capacity = Be32(image + l + 4), size = Be32(image + l + 8);
    if (count == 0) return SIR_OK;
    if (size != UI_ANIM_KEY_SIZE || count > capacity || count > imageSize / UI_ANIM_KEY_SIZE ||
        !LevelSpan(Be32(image + l + 0x10), (u64)count * UI_ANIM_KEY_SIZE, imageSize, &d) ||
        !PlanAdd(plan, d, count * UI_ANIM_KEY_SIZE, kind)) return SIR_ERR_SCREENS;
    if (kind == REGION_UI_ANIM_ENTRIES) {   // kind 3 (+0x24) entries play the sound object at +0x10 (PlaySoundAt)
        for (u32 k = 0; k < count; k++) {
            const u8* en = image + d + k * UI_ANIM_KEY_SIZE;
            if (Be32(en + 0x24) != 3) continue;
            SirError e = SoundPlan(plan, image, imageSize, Be32(en + 0x10));
            if (e != SIR_OK) return e;
        }
    }
    return SIR_OK;
}

static SirError UiAnimPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 a, t;
    SirError e;
    if (!LevelSpan(addr, UI_ANIM_USED, imageSize, &a) || PlanHas(plan, a, REGION_UI_ANIM)) return SIR_OK;
    if (!PlanAdd(plan, a, UI_ANIM_USED, REGION_UI_ANIM)) return SIR_ERR_SCREENS;
    for (u32 k = 0; k < 5; k++) {
        if (!LevelSpan(Be32(image + a + 4 + k * 4), UI_ANIM_TRACK_USED, imageSize, &t) || PlanHas(plan, t, REGION_UI_ANIM_TRACK)) continue;
        if (!PlanAdd(plan, t, UI_ANIM_TRACK_USED, REGION_UI_ANIM_TRACK)) return SIR_ERR_SCREENS;
        if ((e = UiAnimListPlan(plan, image, imageSize, Be32(image + t + 4), REGION_UI_ANIM_KEYS)) != SIR_OK) return e;
    }
    if (!LevelSpan(Be32(image + a + 0x18), UI_ANIM_EVENTS_USED, imageSize, &t) || PlanHas(plan, t, REGION_UI_ANIM_EVENTS)) return SIR_OK;
    if (!PlanAdd(plan, t, UI_ANIM_EVENTS_USED, REGION_UI_ANIM_EVENTS)) return SIR_ERR_SCREENS;
    return UiAnimListPlan(plan, image, imageSize, Be32(image + t + 4), REGION_UI_ANIM_ENTRIES);
}

// A screen's own fields, the widgets it names and its grid.
static SirError UiScreenPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 w, d;
    SirError e;
    if (!LevelSpan(addr, UI_SCREEN_AT + UI_SCREEN_SIZE, imageSize, &w) || PlanHas(plan, w + UI_SCREEN_AT, REGION_UI_SCREEN)) return SIR_OK;
    if (!PlanAdd(plan, w + UI_SCREEN_AT, UI_SCREEN_SIZE, REGION_UI_SCREEN)) return SIR_ERR_SCREENS;
    u32 count = Be32(image + w + UI_SCREEN_AT);
    if (count != 0) {
        if (count > imageSize / 4 || !LevelSpan(Be32(image + w + 0x140), (u64)count * 4, imageSize, &d) ||
            !PlanAdd(plan, d, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        for (u32 i = 0; i < count; i++)
            if ((e = UiBasePlan(plan, image, imageSize, Be32(image + d + i * 4), 0)) != SIR_OK) return e;
    }
    return UiGridPlan(plan, image, imageSize, Be32(image + w + 0x144), 0);
}

// A tree of one of the set's tables. The values of the screens' nodes (+0x28) and of the
// animations' (+0x20) are followed.
static SirError UiTreePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 node, u32 depth, u32 table) {
    u32 n;
    SirError e;
    if (!LevelSpan(node, 0x14, imageSize, &n)) return SIR_OK;
    if (depth > 4096 || PlanHas(plan, n, REGION_UI_TREE_NODE)) return SIR_ERR_SCREENS;   // not a tree
    if (!PlanAdd(plan, n, 0x14, REGION_UI_TREE_NODE)) return SIR_ERR_SCREENS;
    if ((e = UiTreePlan(plan, image, imageSize, Be32(image + n + 0x0C), depth + 1, table)) != SIR_OK) return e;
    if ((e = UiTreePlan(plan, image, imageSize, Be32(image + n + 0x10), depth + 1, table)) != SIR_OK) return e;
    if (table == 0x20) return UiAnimPlan(plan, image, imageSize, Be32(image + n + 0x04));
    if (table != 0x28) return SIR_OK;
    if ((e = UiScreenPlan(plan, image, imageSize, Be32(image + n + 0x04))) != SIR_OK) return e;
    return UiWidgetPlan(plan, image, imageSize, Be32(image + n + 0x04), false, 0);
}

static SirError ScreenSetPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    static const u32 kTables[] = { 0x28, 0x20, 0x34, 0x3C };
    u32 off, t;
    SirError e;
    if (!LevelSpan(addr, UI_SET_USED, imageSize, &off)) return SIR_OK;
    if (!PlanAdd(plan, off, UI_SET_USED, REGION_UI_SET)) return SIR_ERR_SCREENS;
    // A table: the root, and the nodes with the lowest and the highest key (fn_8018E808).
    for (u32 i = 0; i < COUNTOF(kTables); i++) {
        if (!LevelSpan(Be32(image + off + kTables[i]), 0xC, imageSize, &t)) continue;
        if (!PlanAdd(plan, t, 0xC, REGION_PTR_ARRAY)) return SIR_ERR_SCREENS;
        if ((e = UiTreePlan(plan, image, imageSize, Be32(image + t), 0, kTables[i])) != SIR_OK) return e;
    }
    return SIR_OK;
}

// ---- all of them

static SirError ImagePlanBuild(ImagePlan* plan, const u8* hdr, u8* image, u32 imageSize) {
    SirError e = SIR_OK;
    u32 addr;
    if ((addr = Be32(hdr + 0xDC4)) != 0) e = InputTablePlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xD94)) != 0) e = TextureListPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDA8)) != 0)
        e = PtrArrayPlan(plan, image, imageSize, addr, STRING_TABLE_SIZE, 0x0C, 0x00, REGION_STRING_TABLE, SIR_ERR_STRINGS);
    if (e == SIR_OK && (addr = Be32(hdr + 0x844)) != 0)
        e = PtrArrayPlan(plan, image, imageSize, addr, ACTION_TABLE_SIZE, 0x00, 0x04, REGION_ACTION_TABLE, SIR_ERR_ACTIONS);
    if (e == SIR_OK && addr != 0) e = ActionRecordsPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDB0)) != 0) e = UiRootPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDB4)) != 0) e = UiFontListPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDB8)) != 0) e = UiDrawPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDBC)) != 0) e = UiPadPlan(plan, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xD98)) != 0) e = FsmPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xD9C)) != 0) e = HudPlan(plan, image, imageSize, addr, false);
    if (e == SIR_OK) e = FxTablePlan(plan, image, imageSize, Be32(hdr + 0x830), Be32(hdr + 0x82C), false);
    if (e == SIR_OK) e = FxTablePlan(plan, image, imageSize, Be32(hdr + 0x83C), Be32(hdr + 0x838), true);
    if (e == SIR_OK && (addr = Be32(hdr + 0x010)) != 0) e = PoolPlan(plan, image, imageSize, addr, false);
    if (e == SIR_OK && (addr = Be32(hdr + 0x418)) != 0) e = PoolPlan(plan, image, imageSize, addr, false);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDC0)) != 0) e = ScreenSetDbPlan(plan, image, imageSize, addr);
    if (e != SIR_OK) return e;
    return PlanFinish(plan) ? SIR_OK : SIR_ERR_OVERLAP;
}

void (*gSirTextureNote)(const void* pixels, const char* name) = NULL;

// Tells the port which pixel data belongs to which texture name (called after the header is converted).
static void TextureNote(const u8* p, const u8* end) {
    if (gSirTextureNote == NULL || p + 0x36 > end) return;
    const void* pixels;
    memcpy(&pixels, p + 0x1C, sizeof(pixels));
    const u8* e = p + 0x35;
    while (e < end && *e) e++;
    if (pixels != NULL && e < end && e > p + 0x35) gSirTextureNote(pixels, (const char*)(p + 0x35));
}

static void ImagePlanConvert(const ImagePlan* plan, SirReloc* r, u8* image, u32 imageSize) {
    for (u32 i = 0; i < plan->count; i++) {
        const Region* g = &plan->regions[i];
        u8* p = image + g->off;
        if (LoaderWatch() != 0 && sConvertBase != 0 && sConvertBase + g->off < sWatchAddr + 0x200 && sConvertBase + g->off + g->size + 0x200 > sWatchAddr)
            fprintf(stderr, "[loader watch] 0x%08X size 0x%X kind %u (image at 0x%08X, converted at %p)\n", sConvertBase + g->off, g->size, g->kind, sConvertBase, image);
        switch (g->kind) {
        case REGION_INPUT_TABLE:  InputTableConvert(r, p); break;
        case REGION_INPUT_STEP:   InputStepConvert(r, p, g->size); break;
        case REGION_TEXTURE_LIST: SirConvertRecord(r, p, g->size, kTextureListFields, COUNTOF(kTextureListFields)); break;
        case REGION_TEXTURE:      SirConvertRecord(r, p, g->size, kTextureFields, COUNTOF(kTextureFields)); TextureNote(p, image + imageSize); break;
        case REGION_STRING_TABLE: SirConvertRecord(r, p, g->size, kStringTableFields, COUNTOF(kStringTableFields)); break;
        case REGION_ACTION_TABLE: SirConvertRecord(r, p, g->size, kActionTableFields, COUNTOF(kActionTableFields)); break;
        case REGION_UI_SLIDER:    SirConvertRecord(r, p, g->size, kUiSliderFields, COUNTOF(kUiSliderFields)); break;
        case REGION_PTR_ARRAY:
            for (u32 k = 0; k < g->size; k += 4) ConvertPtr(r, p + k);
            break;
        case REGION_U32_ARRAY:
            for (u32 k = 0; k < g->size; k += 4) { Swap32(p + k); r->fieldsSwapped++; }
            break;
        case REGION_UI_ROOT:      SirConvertRecord(r, p, g->size, kUiRootFields, COUNTOF(kUiRootFields)); break;
        case REGION_UI_FONT:      SirConvertRecord(r, p, g->size, kUiFontFields, COUNTOF(kUiFontFields)); break;
        case REGION_UI_FONT_OBJ:  UiFontObjConvert(r, p, g->size); break;
        case REGION_UI_FONT_LIST: SirConvertRecord(r, p, g->size, kUiFontListFields, COUNTOF(kUiFontListFields)); break;
        case REGION_UI_FONT_DATA: UiFontDataConvert(r, p); break;
        case REGION_UI_DRAW:      SirConvertRecord(r, p, g->size, kUiDrawFields, COUNTOF(kUiDrawFields)); break;
        case REGION_UI_PAD:       SirConvertRecord(r, p, g->size, kUiPadFields, COUNTOF(kUiPadFields)); break;
        case REGION_FSM:          SirConvertRecord(r, p, g->size, kFsmFields, COUNTOF(kFsmFields)); break;
        case REGION_FSM_DEF:      SirConvertRecord(r, p, g->size, kFsmDefFields, COUNTOF(kFsmDefFields)); break;
        case REGION_FSM_STATES:   ConvertRecords(r, p, g->size, FSM_STATE_SIZE, kFsmStateFields, COUNTOF(kFsmStateFields)); break;
        case REGION_FSM_EVENTS:   ConvertRecords(r, p, g->size, FSM_EVENT_SIZE, kFsmEventFields, COUNTOF(kFsmEventFields)); break;
        case REGION_FSM_SHARED_EVENTS:
            ConvertRecords(r, p, g->size, FSM_EVENT_SIZE, kFsmSharedEventFields, COUNTOF(kFsmSharedEventFields));
            break;
        case REGION_FSM_NODE:     FsmNodeConvert(r, p); break;
        case REGION_FSM_TRANSITION:
            SirConvertRecord(r, p, g->size, kFsmTransitionFields, COUNTOF(kFsmTransitionFields));
            break;
        case REGION_FSM_SHARED_TRANSITION:
            SirConvertRecord(r, p, g->size, kFsmSharedTransitionFields, COUNTOF(kFsmSharedTransitionFields));
            break;
        case REGION_HUD:          SirConvertRecord(r, p, g->size, kHudFields, COUNTOF(kHudFields)); break;
        case REGION_LEVEL_HUD:    SirConvertRecord(r, p, g->size, kLevelHudFields, COUNTOF(kLevelHudFields)); break;
        case REGION_FX_BOLTS:     ConvertRecords(r, p, g->size, FX_BOLT_SIZE, kFxBoltFields, COUNTOF(kFxBoltFields)); break;
        case REGION_FX_TRAILS:    ConvertRecords(r, p, g->size, FX_TRAIL_SIZE, kFxTrailFields, COUNTOF(kFxTrailFields)); break;
        case REGION_POOL:         PoolConvert(r, image, p, imageSize); break;
        case REGION_ENTITY:       SirConvertRecord(r, p, g->size, kEntityFields, COUNTOF(kEntityFields)); break;
        case REGION_ENTITY_PROP:  SirConvertRecord(r, p, g->size, kEntityFields, COUNTOF(kEntityFields));
                                  SirConvertRecord(r, p, g->size, kPropFields, COUNTOF(kPropFields)); break;
        case REGION_ENTITY_TIMED: SirConvertRecord(r, p, g->size, kEntityFields, COUNTOF(kEntityFields));
                                  SirConvertRecord(r, p, g->size, kTimedFields, COUNTOF(kTimedFields)); break;
        case REGION_ZONE_SYSTEM:  SirConvertRecord(r, p, g->size, kZoneSystemFields, COUNTOF(kZoneSystemFields)); break;
        case REGION_ANIM_PLAYER:  SirConvertRecord(r, p, g->size, kAnimPlayerFields, COUNTOF(kAnimPlayerFields)); break;
        case REGION_ANIM_ELEM:    SirConvertRecord(r, p, g->size, kAnimElemFields, COUNTOF(kAnimElemFields)); break;
        case REGION_ANIM_TRACK:   SirConvertRecord(r, p, g->size, kAnimTrackFields, COUNTOF(kAnimTrackFields)); break;
        case REGION_ANIM_KEYS:
            for (u32 k = 0; k + 8 <= g->size; k += 8) { Swap16(p + k); Swap32(p + k + 4); r->fieldsSwapped += 2; }
            break;
        case REGION_F32_ARRAY:
            for (u32 k = 0; k < g->size; k += 4) { Swap32(p + k); r->fieldsSwapped++; }
            break;
        case REGION_KEY_PTR:
            for (u32 k = 0; k + 8 <= g->size; k += 8) { Swap32(p + k); r->fieldsSwapped++; ConvertPtr(r, p + k + 4); }
            break;
        case REGION_UI_LIST:      SirConvertRecord(r, p, g->size, kUiListFields, COUNTOF(kUiListFields)); break;
        case REGION_UI_WIDGET:    SirConvertRecord(r, p, g->size, kUiWidgetFields, COUNTOF(kUiWidgetFields)); break;
        case REGION_UI_EVENT:     SirConvertRecord(r, p, g->size, kUiEventFields, COUNTOF(kUiEventFields)); break;
        case REGION_UI_GRID:      SirConvertRecord(r, p, g->size, kUiGridFields, COUNTOF(kUiGridFields)); break;
        case REGION_UI_SET:       SirConvertRecord(r, p, g->size, kUiSetFields, COUNTOF(kUiSetFields)); break;
        case REGION_UI_SCREEN:    SirConvertRecord(r, p, g->size, kUiScreenFields, COUNTOF(kUiScreenFields)); break;
        case REGION_UI_GRAPHIC:   SirConvertRecord(r, p, g->size, kUiGraphicFields, COUNTOF(kUiGraphicFields)); break;
        case REGION_UI_TEXT:      SirConvertRecord(r, p, g->size, kUiTextFields, COUNTOF(kUiTextFields)); break;
        case REGION_SKELETON:     SirConvertRecord(r, p, g->size, kSkeletonFields, COUNTOF(kSkeletonFields)); break;
        case REGION_BONE_OPS:
            for (u32 k = 0; k + 8 <= g->size; k += 8) { Swap32(p + k); Swap32(p + k + 4); r->fieldsSwapped += 2; }   // bone name hash (the camera track looks joints up by it), then the walk
            break;
        case REGION_ANIM_OBJ:     SirConvertRecord(r, p, g->size, kAnimObjFields, COUNTOF(kAnimObjFields)); break;
        case REGION_ANIM_SET:     SirConvertRecord(r, p, g->size, kAnimSetFields, COUNTOF(kAnimSetFields)); break;
        case REGION_GX_STATE:     ConvertGxState(r, p, g->size); break;
        case REGION_SCENE_LIGHT:  SirConvertRecord(r, p, g->size, kSceneLightFields, COUNTOF(kSceneLightFields)); break;
        case REGION_ATTACH_POINTS: ConvertRecords(r, p, g->size, ATTACH_POINT_SIZE, kAttachPointFields, COUNTOF(kAttachPointFields)); break;
        case REGION_ANIM_DEFS:    ConvertRecords(r, p, g->size, ANIM_DEF_SIZE, kAnimDefFields, COUNTOF(kAnimDefFields)); break;
        case REGION_ANIM_EVENTS:  ConvertRecords(r, p, g->size, ANIM_EVENT_SIZE, kAnimEventFields, COUNTOF(kAnimEventFields)); break;
        case REGION_U16_ARRAY:
            for (u32 k = 0; k + 2 <= g->size; k += 2) { Swap16(p + k); r->fieldsSwapped++; }
            break;
        case REGION_LOCATORS:     ConvertRecords(r, p, g->size, LOCATOR_SIZE, kLocatorFields, COUNTOF(kLocatorFields)); break;
        case REGION_ZONE_EFFECT:  SirConvertRecord(r, p, g->size, kZoneEffectFields, COUNTOF(kZoneEffectFields)); break;
        case REGION_UI_ANIM:      SirConvertRecord(r, p, g->size, kUiAnimFields, COUNTOF(kUiAnimFields)); break;
        case REGION_UI_ANIM_TRACK: SirConvertRecord(r, p, g->size, kUiAnimTrackFields, COUNTOF(kUiAnimTrackFields)); break;
        case REGION_UI_ANIM_EVENTS: SirConvertRecord(r, p, g->size, kUiAnimEventsFields, COUNTOF(kUiAnimEventsFields)); break;
        case REGION_UI_ANIM_KEYS:
            for (u32 k = 0; k + UI_ANIM_KEY_SIZE <= g->size; k += UI_ANIM_KEY_SIZE) {
                Swap32(p + k);
                for (u32 f = 0x10; f <= 0x20; f += 4) Swap32(p + k + f);
                r->fieldsSwapped += 6;
            }
            break;
        case REGION_UI_ANIM_ENTRIES:
            for (u32 k = 0; k + UI_ANIM_KEY_SIZE <= g->size; k += UI_ANIM_KEY_SIZE) {
                Swap32(p + k); Swap32(p + k + 0x24);
                r->fieldsSwapped += 2;
                if (*(const u32*)(p + k + 0x24) == 3) ConvertPtr(r, p + k + 0x10);
                else { Swap32(p + k + 0x10); r->fieldsSwapped++; }
            }
            break;
        case REGION_UI_TREE_NODE: SirConvertRecord(r, p, g->size, kUiTreeNodeFields, COUNTOF(kUiTreeNodeFields)); break;
        case REGION_PROP_DEF:     SirConvertRecord(r, p, g->size, kPropDefFields, COUNTOF(kPropDefFields)); break;
        case REGION_ENTITY_RECORD: SirConvertRecord(r, p, g->size, kEntityRecordFields, COUNTOF(kEntityRecordFields)); break;
        case REGION_ENTITY_PART:  SirConvertRecord(r, p, g->size, kEntityPartFields, COUNTOF(kEntityPartFields)); break;
        case REGION_PTR_COUNT:    SirConvertRecord(r, p, g->size, kPtrCountFields, COUNTOF(kPtrCountFields)); break;
        case REGION_COUNT_PTR:    SirConvertRecord(r, p, g->size, kCountPtrFields, COUNTOF(kCountPtrFields)); break;
        case REGION_POOL_TABLE:   ConvertRecords(r, p, g->size, POOL_TABLE_ENTRY, kPoolTableEntryFields, COUNTOF(kPoolTableEntryFields)); break;
        case REGION_SOUND_GROUP:  SirConvertRecord(r, p, g->size, kSoundGroupFields, COUNTOF(kSoundGroupFields)); break;
        case REGION_SOUND:        ConvertRecords(r, p, g->size, SOUND_SIZE, kSoundFields, COUNTOF(kSoundFields)); break;
        case REGION_SOUND_INFO:   SirConvertRecord(r, p, g->size, kSoundInfoFields, COUNTOF(kSoundInfoFields)); break;
        case REGION_DSP_HEADER:   SirConvertRecord(r, p, g->size, kDspHeaderFields, COUNTOF(kDspHeaderFields)); break;
        case REGION_POOL_BLOCK:   // converted with its pool
        case REGION_RUNTIME:
            break;
        }
    }
}

// ------------------------------------------------------------------------------------ header block

// The 4 KB block the game reads into sSirHeader. "code": a decompiled function reads the field
// with this type. "data": typed from the values in the file only (pointer into the image, small
// integer, float, or a byte followed by zeros); confirm when the code that reads it is decompiled.
// Words not listed are 0 or -1 in the shipped file, which read the same in both byte orders.
static const SirField kGlobalBinaryHeaderFields[] = {
    { 0x008, SIR_U32, 1 },    // code: text hash of the save comment (-1)
    { 0x010, SIR_PTR, 1 },    // code: object pool (fn_8012189C); the 256 words behind it are 0 in the file
    { 0x414, SIR_U32, 1 },    // code: fn_8012189C clears it
    { 0x418, SIR_PTR, 1 },    // code: object pool (fn_8012189C)
    { 0x81C, SIR_U32, 1 },    // code: fn_8012189C clears it
    // 0x820: u8 flag (code: cleared as a byte)
    { 0x824, SIR_PTR, 2 },    // code: texture "Icon-HomeButtonMenu_is_not_available", model "Generic_box"
    { 0x82C, SIR_U32, 1 },    // code: number of effect definitions of the first kind (fn_80007D68)
    { 0x830, SIR_PTR, 1 },    // code: their 0x70-byte entries
    // 0x834: u8 flag (code: the entries are sorted by hash)
    { 0x838, SIR_U32, 1 },    // code: number of effect definitions of the second kind
    { 0x83C, SIR_PTR, 1 },    // code: their 0x80-byte entries
    // 0x840: u8 flag (code: sorted)
    { 0x844, SIR_PTR, 1 },    // code: action table, kept in gGlobalBinaryField844 (TriggerDoAction)
    { 0x848, SIR_U32, 6 },    // data: 0x3FFFF each
    { 0x860, SIR_U32, 5 },    // data: hash, 0x1AB80, 0, 0x37, 1234
    { 0x874, SIR_F32, 2 },    // data: 1.2345, 0.0123
    { 0x960, SIR_U32, 268 },  // data: -1 up to 0xD8C
    { 0xD94, SIR_PTR, 3 },    // data: image start (texture list); code: 0xD98 screen list, 0xD9C HUD
    { 0xDA8, SIR_PTR, 1 },    // code: string table (UiInit stores it in gStringTable)
    { 0xDB0, SIR_PTR, 7 },    // code: 0xDB0-0xDBC (UiInit), 0xDC4 input table, 0xDC8 GameDb; data: 0xDC0
};

extern "C" SirError GlobalBinaryConvertHeader(SirReloc* r, u8* hdr) {
    if (!SirConvertRecord(r, hdr, GLOBAL_BINARY_HDR_SIZE, kGlobalBinaryHeaderFields,
                          (s32)(sizeof(kGlobalBinaryHeaderFields) / sizeof(kGlobalBinaryHeaderFields[0]))))
        return SIR_ERR_BOUNDS;
    return SIR_OK;
}

extern "C" SirError GlobalBinaryConvert(SirReloc* r, u8* hdr, u8* image, u32 imageSize, s32* failTable) {
    if (!SirRelocAddRange(r, GLOBAL_BINARY_WII_BASE, imageSize, image)) return SIR_ERR_BOUNDS;
    if (failTable != NULL) *failTable = -1;
    // Check everything before the first byte changes: the hand-typed objects here, the database
    // inside GameDbConvert.
    ImagePlan plan = { NULL, 0, 0 };
    SirError e = ImagePlanBuild(&plan, hdr, image, imageSize);
    if (e == SIR_OK) e = GameDbConvert(r, image, imageSize, Be32(hdr + GLOBAL_BINARY_HDR_GAMEDB), failTable);
    if (e == SIR_OK) {
        ImagePlanConvert(&plan, r, image, imageSize);
        e = GlobalBinaryConvertHeader(r, hdr);
    }
    PlanFree(&plan);
    return e;
}

// ------------------------------------------------------------------------------------ level files
//
// A level file (`empty_level.sir`, `village_level.sir`, ...) has a header block (section type -1,
// 0x47800 bytes) and one image per zone (section type = zone number). fn_801294CC reads the header
// block into the world state (`*gWorldState`), runs fn_80129CC8 on it, then reads zone 0 to
// gZoneBase[0]. The header block is the world state itself: a few top-level fields and, from
// +0x21E0, one slot per zone. Only fields with a reader in the decompiled code are converted;
// docs/loader.md lists them and what is left.

// Top of the world state.
static const SirField kLevelHeaderFields[] = {
    { 0x004, SIR_U32, 1 },    // code: s32 last zone index (fn_8013006C loops i <= it)
    { 0x008, SIR_PTR, 4 },    // code: current zone slots, set by fn_801294CC (+0x08, +0x0C; +0x10, +0x14 by index)
    { 0x024, SIR_U32, 1 },    // code: how many locators (WorldFindLocator),
    { 0x028, SIR_PTR, 1 },    //       and the locators: 0x20-byte entries in a zone
    { 0x02C, SIR_PTR, 1 },    // code: object list (fn_8013006C registers it with the object database)
    { 0x030, SIR_U32, 1 },    // code: its count
    { 0x038, SIR_PTR, 1 },    // code: second object list
    { 0x03C, SIR_U32, 1 },    // code: its count
    { 0x044, SIR_PTR, 1 },    // code: the part list (fn_8003DB78, fn_8003DA7C, fn_8003DCE0): pointers to 0x10-byte entries,
    { 0x048, SIR_U32, 1 },    //       and how many (PartListPlan)
    { 0x074, SIR_PTR, 2 },    // code: the objects projectiles are spawned from (SpawnCoffinProjectile +0x74, FireRocketAtEntity +0x78)
    { 0x07C, SIR_U32, 1 },    // code: fn_8012FFA4 copies +0x7C..+0x90 to the manager lbl_8047C730: count,
    { 0x080, SIR_PTR, 1 },    //       entries, (u8 flag at +0x84),
    { 0x088, SIR_U32, 1 },    //       count,
    { 0x08C, SIR_PTR, 1 },    //       0x1F4-byte entries (u8 flag at +0x90)
    { 0x094, SIR_U32, 3 },    // code: three action words GameSessionUpdate runs when the level is entered (TriggerDoAction):
                              //       the level's set-up program (junction states, locked paths, enemy spawns by quest)
    { 0x1A4, SIR_PTR, 1 }, { 0x1A8, SIR_U32, 2 },   // code: zone slots, count, used (fn_80129CC8 sets all five tables)
    { 0x1B0, SIR_PTR, 1 }, { 0x1B4, SIR_U32, 2 },
    { 0x1BC, SIR_PTR, 1 }, { 0x1C0, SIR_U32, 2 },
    { 0x1C8, SIR_PTR, 1 }, { 0x1CC, SIR_U32, 2 },
    { 0x1D4, SIR_PTR, 1 }, { 0x1D8, SIR_U32, 2 },
    { 0x1E0, SIR_U32, 3 },    // code: +0x1E0 (GameSessionUpdate), +0x1E4 (PlayFootStepEffect), +0x1E8 (fn_800D7FF0)
    // 0x1EC: u8 (WorldPostUpdate)
    { 0x1F0, SIR_U32, 1 },    // code: GameSessionUpdate
    { 0x1F4, SIR_PTR, 1 },    // code: the level's HUD (HudReload, fn_800D8100)
    // 0x1F8: three light records of 0x82C bytes, of which +0x1A7C are in use. EntityUpdate gives an
    // entity the first record that has flag 0x10 or lists its definition's id: +0x00 six floats
    // (colours: fn_8003E560, fn_8006C3FC, EntityUpdate), +0x18 and +0x1C angles, +0x20 a word
    // without a reader, +0x24 up to 512 definition ids, +0x824 how many, +0x828 flags.
    { 0x1F8, SIR_F32, 8 },  { 0x218, SIR_U32, 515 },
    { 0xA24, SIR_F32, 8 },  { 0xA44, SIR_U32, 515 },
    { 0x1250, SIR_F32, 8 }, { 0x1270, SIR_U32, 515 },
    { 0x1A7C, SIR_U32, 1 },   // code: EntityUpdate, fn_8003E560
    // 0x1A80: the level's picture settings, 0xA0 bytes: two flag bytes, then 39 floats
    { 0x1A84, SIR_F32, 39 },  // code: fn_8012A79C copies them all to lbl_8040D500 (fn_8015F4CC builds the
                              //       colour tables from them); the first two also SetOptionsBrightness
};

// One zone slot. Words marked "word" are read with lwz by the function named; whether they hold
// a pointer or a number is not known yet, which makes no difference while the zones sit at their
// original addresses.
static const SirField kLevelSlotFields[] = {
    { 0x000, SIR_F32, 6 },    // code: WorldGetEntityZone (+0x00..+0x08), fn_80131370 (+0x0C..+0x14)
    { 0x018, SIR_U32, 1 },    // code: fn_80133D74
    { 0x01C, SIR_F32, 3 },    // code: WorldGetEntityZone (+0x1C), fn_80130FC8 (+0x20, +0x24)
    { 0x030, SIR_F32, 7 },    // code: fn_80133A84 (+0x30..+0x3C fog colour, +0x40 / +0x44 fog range, +0x48 far distance; +0x4C / +0x50 have no reader)
    { 0x054, SIR_F32, 11 },   // code: fn_80133A84 (+0x54..+0x70 two four-float colours, +0x74..+0x7C a direction)
    { 0x050, SIR_PTR, 1 },    // code: resource released by fn_8013006C (fn_8012F8B8)
    { 0x080, SIR_PTR, 1 },    // code: pointers to the zone's placed effects (WorldOnZoneLoaded), their count at +0x9C
    { 0x08C, SIR_PTR, 2 },    // code: SessionBegin, SessionRestoreZoneObjects (two pointers in every slot of the file)
    { 0x094, SIR_U32, 2 },    // code: CharacterFind, SessionBegin
    { 0x09C, SIR_U32, 1 },    // code: how many placed effects (WorldOnZoneLoaded)
    { 0x0A0, SIR_PTR, 1 },    // code: the zone's particle systems (0x4C bytes each, SlotRopesPlan), their count in the byte at +0xA4 (fn_800098F4)
    { 0x0A8, SIR_PTR, 1 },    // code: pointers to the zone's objects, their count in the byte at +0xAC (fn_8012FD0C)
    { 0x0BC, SIR_U32, 1 },    // word: fn_801C668C (u8 at +0xC0: fn_8005E1B8)
    { 0x0CC, SIR_U32, 2 },    // word: fn_801C6BCC, fn_801C6C80
    { 0x0D4, SIR_PTR, 1 },    // code: the zone's action table (TriggerDoAction)
    { 0x0E4, SIR_PTR, 1 },    // code: pool table: 0x2C-byte entries (fn_8012FD0C -> fn_801DB1AC),
    { 0x0E8, SIR_U32, 1 },    //       their count
    { 0x0FC, SIR_PTR, 1 },    // code: the zone's record at the start of its image (WorldTick)
    { 0x100, SIR_U32, 1 },    // word: OnZoneScreensLoaded
    { 0x10C, SIR_U32, 1 },    // word: fn_8004DAF8
    { 0x110, SIR_PTR, 1 },    // code: the zone's chain of sound groups (PlaySound)
    { 0x114, SIR_PTR, 1 }, { 0x118, SIR_U32, 2 },   // code: three tables fn_80129CC8 places: pointer, capacity, used
    { 0x120, SIR_PTR, 1 }, { 0x124, SIR_U32, 2 },
    { 0x12C, SIR_PTR, 1 }, { 0x130, SIR_U32, 2 },
};

extern "C" u32 LevelHeaderWiiBase(const u8* hdr, u32 size) {
    if (hdr == NULL || size < LEVEL_HEADER_SLOTS + 4) return 0;
    u32 slots = Be32(hdr + 0x1A4);
    return slots >= LEVEL_HEADER_SLOTS ? slots - LEVEL_HEADER_SLOTS : 0;
}

// Size of one zone slot: fn_80129CC8 points each slot's first table (+0x114) at slot + 0x140, so
// the distance between two slots' tables is the slot size. 0 if the block is not laid out so.
static u32 LevelSlotSize(const u8* hdr, u32 size, u32 base, u32 count) {
    if (count < 2 || size < LEVEL_HEADER_SLOTS + 0x118) return LEVEL_SLOT_MIN_SIZE;
    u32 first = Be32(hdr + LEVEL_HEADER_SLOTS + 0x114);
    if (first != base + LEVEL_HEADER_SLOTS + 0x140) return 0;
    for (u32 stride = LEVEL_SLOT_MIN_SIZE; stride <= LEVEL_SLOT_MAX_SIZE; stride += 4) {
        if ((u64)LEVEL_HEADER_SLOTS + stride + 0x118 > size) break;
        if (Be32(hdr + LEVEL_HEADER_SLOTS + stride + 0x114) == first + stride) return stride;
    }
    return 0;
}

// A table of pointers inside the header block: `count` words at Wii address `addr`. Marks its
// words in `used` (one byte per header word); false if it leaves the block or shares a word with
// a table marked before or with the slots.
static bool LevelTableMark(u8* used, u32 size, u32 base, u32 addr, u32 count) {
    if (count == 0) return true;
    if (addr < base || (addr & 3) != 0) return false;
    u32 off = addr - base;
    if ((u64)off + (u64)count * 4 > size) return false;
    for (u32 i = 0; i < count; i++) {
        if (used[off / 4 + i]) return false;
        used[off / 4 + i] = 1;
    }
    return true;
}

static void LevelTableConvert(SirReloc* r, u8* hdr, u32 base, u32 addr, u32 count) {
    for (u32 i = 0; i < count; i++) ConvertPtr(r, hdr + (addr - base) + i * 4);
}

// The pointer tables of the header block: per slot the two entity tables fn_80129CC8 places
// (+0x120 and +0x12C, capacity behind each; fn_8012FD0C walks them), and the three per-zone tables
// of the block (+0x1B0, +0x1BC; fn_8013006C and others read *table[i]). Their entries are
// pointers into the zones. The table at slot +0x114 (0x54-byte entries) and the one at +0x1D4
// (12-byte entries) have no typed reader yet and are left.
static const u32 kLevelSlotTables[] = { 0x120, 0x12C };
static const u32 kLevelBlockTables[] = { 0x1B0, 0x1BC };
// The table at +0x1C8 has one 12-byte entry per zone (fn_80129CC8 steps 0xC for it): the zone's
// list of running effects, {u16 first, u16 last, u16 count, u16 flags, word} (PoolSpawn in
// draft_miss285.cpp, fn_801E7E28). The files have it all zero.
#define LEVEL_EFFECT_LISTS 0x1C8

extern "C" SirError LevelHeaderConvert(SirReloc* r, u8* hdr, u32 size) {
    u32 base = LevelHeaderWiiBase(hdr, size);
    if (base == 0) return SIR_ERR_LEVEL;
    u32 count = Be32(hdr + 0x1A8);
    u32 stride = LevelSlotSize(hdr, size, base, count);
    u64 slotsEnd = (u64)LEVEL_HEADER_SLOTS + (u64)count * stride;
    if (stride == 0 || count > LEVEL_MAX_ZONES || slotsEnd > size) return SIR_ERR_LEVEL;

    // Check every table before the first byte changes.
    u8* used = (u8*)calloc(size / 4 + 1, 1);
    if (used == NULL) return SIR_ERR_LEVEL;
    // The top of the block and the fixed part of every slot are taken. A slot's first table
    // (+0x114: 0x54-byte entries) starts at slot + 0x140; some slots keep their two pointer tables
    // in the words behind it, the others in the space behind the slot array (fn_80129CC8).
    memset(used, 1, LEVEL_HEADER_SLOTS / 4);
    bool ok = true;
    for (u32 i = 0; i < count && ok; i++) {
        const u8* slot = hdr + LEVEL_HEADER_SLOTS + i * stride;
        u32 entries = Be32(slot + 0x118);
        ok = entries <= (stride - LEVEL_SLOT_MIN_SIZE + 8) / 0x54;
        if (ok) memset(used + (LEVEL_HEADER_SLOTS + i * stride) / 4, 1, (0x140 + entries * 0x54) / 4);
    }
    for (u32 i = 0; i < count && ok; i++) {
        const u8* slot = hdr + LEVEL_HEADER_SLOTS + i * stride;
        for (u32 t = 0; t < 2 && ok; t++)
            ok = LevelTableMark(used, size, base, Be32(slot + kLevelSlotTables[t]), Be32(slot + kLevelSlotTables[t] + 4));
    }
    for (u32 t = 0; t < COUNTOF(kLevelBlockTables) && ok; t++)
        ok = LevelTableMark(used, size, base, Be32(hdr + kLevelBlockTables[t]), Be32(hdr + kLevelBlockTables[t] + 4));
    u32 lists = Be32(hdr + LEVEL_EFFECT_LISTS), listCount = Be32(hdr + LEVEL_EFFECT_LISTS + 4);
    ok = ok && listCount <= LEVEL_MAX_ZONES && LevelTableMark(used, size, base, lists, listCount * 3);
    free(used);
    if (!ok) return SIR_ERR_LEVEL;

    for (u32 i = 0; i < count; i++) {
        u8* slot = hdr + LEVEL_HEADER_SLOTS + i * stride;
        for (u32 t = 0; t < 2; t++)
            LevelTableConvert(r, hdr, base, Be32(slot + kLevelSlotTables[t]), Be32(slot + kLevelSlotTables[t] + 4));
        SirConvertRecord(r, slot, stride, kLevelSlotFields, COUNTOF(kLevelSlotFields));
    }
    for (u32 t = 0; t < COUNTOF(kLevelBlockTables); t++)
        LevelTableConvert(r, hdr, base, Be32(hdr + kLevelBlockTables[t]), Be32(hdr + kLevelBlockTables[t] + 4));
    for (u32 i = 0; i < listCount; i++) {
        u8* e = hdr + (lists - base) + i * 0xC;
        for (u32 k = 0; k < 8; k += 2) Swap16(e + k);
        Swap32(e + 8);
        r->fieldsSwapped += 5;
    }
    SirConvertRecord(r, hdr, LEVEL_HEADER_SLOTS, kLevelHeaderFields, COUNTOF(kLevelHeaderFields));
    return SIR_OK;
}

// Entities of the zone at `zoneHost` that a converted header block names: every slot's +0x50
// and the entries of its two entity tables. Writes their Wii addresses to `out` (at most `max`)
// and returns how many there are.
// ---- what a converted header block names in one zone

struct RootList {
    u32* items;
    u32  count, capacity;
};

static bool RootAdd(RootList* l, u32 value) {
    if (l->count == l->capacity) {
        u32 cap = l->capacity ? l->capacity * 2 : 64;
        u32* grown = (u32*)realloc(l->items, (size_t)cap * sizeof(u32));
        if (grown == NULL) return false;
        l->items = grown;
        l->capacity = cap;
    }
    l->items[l->count++] = value;
    return true;
}

// Wii address of a host pointer slot's target if it lies in the zone, else 0.
static u32 ZoneAddr(const u8* slot, const u8* zoneHost, u32 zoneSize, u32 zoneWiiBase) {
    const u8* p;
    memcpy(&p, slot, sizeof(p));
    return p >= zoneHost && p < zoneHost + zoneSize ? zoneWiiBase + (u32)(p - zoneHost) : 0;
}

// A pointer table {pointer, capacity} of the header block; NULL if it is not inside the block.
static const u8* HeaderTable(const u8* hdr, u32 hdrSize, const u8* at, u32* entries) {
    const u8* table;
    memcpy(&table, at, sizeof(table));
    *entries = Host32(at + 4);
    if (table < hdr || table > hdr + hdrSize || (u64)(table - hdr) + (u64)*entries * 4 > hdrSize) return NULL;
    return table;
}

extern "C" void LevelZoneRootsFree(LevelZoneRoots* roots) {
    free(roots->entities);
    free(roots->sceneObjects);
    free(roots->poolTables);
    free(roots->slotObjects);
    free(roots->slotEffects);
    free(roots->slotRopes);
    free(roots->recordArrays);
    free(roots->actionTables);
    free(roots->soundGroups);
    free(roots->zoneRecords);
    free(roots->zoneLists);
    memset(roots, 0, sizeof(*roots));
}

// The slot size of a converted header block: the distance between the first tables of the first
// two slots (fn_80129CC8 puts each at slot + 0x140).
static bool HostSlotStride(const u8* hdr, u32 hdrSize, u32* countOut, u32* strideOut) {
    bool ok = hdrSize >= LEVEL_HEADER_SLOTS + 0x118;
    u32 count = ok ? Host32(hdr + 0x1A8) : 0, stride = LEVEL_SLOT_MIN_SIZE;
    const u8* slots = hdr + LEVEL_HEADER_SLOTS;
    if (ok && count > 1) {
        const u8* first;
        const u8* next = NULL;
        memcpy(&first, slots + 0x114, sizeof(first));
        for (;; stride += 4) {
            if (stride > LEVEL_SLOT_MAX_SIZE || (u64)LEVEL_HEADER_SLOTS + stride + 0x118 > hdrSize) { ok = false; break; }
            memcpy(&next, slots + stride + 0x114, sizeof(next));
            if (next == first + stride) break;
        }
    }
    *countOut = count;
    *strideOut = stride;
    return ok && count <= LEVEL_MAX_ZONES && (u64)LEVEL_HEADER_SLOTS + (u64)count * stride <= hdrSize;
}

extern "C" u32 LevelSlotSize(const u8* hdr, u32 hdrSize) {
    u32 count, stride;
    return HostSlotStride(hdr, hdrSize, &count, &stride) ? stride : 0;
}

// The roots of a shared zone, from the big-endian copy of its zone slot at the start of the image.
extern "C" BOOL LevelSharedZoneRootsCollect(const u8* image, u32 imageSize, u32 wiiBase, u32 slotSize,
                                            const LevelZoneOther* other, LevelZoneRoots* out) {
    memset(out, 0, sizeof(*out));
    if (slotSize < LEVEL_SLOT_MIN_SIZE || slotSize > LEVEL_SLOT_MAX_SIZE || slotSize > imageSize) return FALSE;
    RootList entities = { NULL, 0, 0 }, pools = { NULL, 0, 0 }, lists = { NULL, 0, 0 }, actions = { NULL, 0, 0 }, groups = { NULL, 0, 0 };
    bool ok = true;
    u32 addr;
    #define IN_ZONE(a, n) ((a) >= wiiBase && (u64)((a) - wiiBase) + (n) <= imageSize)
    if ((addr = Be32(image + 0x50)) != 0 && IN_ZONE(addr, 4)) ok = RootAdd(&entities, addr);
    for (u32 t = 0; ok && t < 2; t++) {                          // the slot's two entity tables, when they lie in the image
        u32 table = Be32(image + kLevelSlotTables[t]), entries = Be32(image + kLevelSlotTables[t] + 4);
        if (table == 0 || entries > imageSize / 4 || !IN_ZONE(table, (u64)entries * 4)) continue;
        for (u32 k = 0; ok && k < entries; k++)
            if ((addr = Be32(image + (table - wiiBase) + k * 4)) != 0 && IN_ZONE(addr, 4)) ok = RootAdd(&entities, addr);
    }
    if (ok && (addr = Be32(image + 0xE4)) != 0 && IN_ZONE(addr, 4)) ok = RootAdd(&pools, addr) && RootAdd(&pools, Be32(image + 0xE8));
    // The slot's action table at +0xD4 (TriggerDoAction kind 2): the screens' items run their activate / accept actions from it.
    if (ok && (addr = Be32(image + 0xD4)) != 0 && IN_ZONE(addr, 8)) ok = RootAdd(&actions, addr);
    if (ok && (addr = Be32(image + 0x110)) != 0 && IN_ZONE(addr, SOUND_GROUP_SIZE)) ok = RootAdd(&groups, addr);
    // The zone's record is in zone 0, converted: its list leads into this image.
    u32 rec = Be32(image + 0xFC);
    if (ok && other != NULL && rec >= other->wiiBase && (u64)(rec - other->wiiBase) + 0x14 <= other->imageSize) {
        // A host pointer into this image, or, when zone 0 was converted without a range for
        // this zone, the Wii address in host order (ConvertPtr keeps that).
        const u8* head;
        memcpy(&head, other->image + (rec - other->wiiBase) + 0x10, sizeof(head));
        u32 raw = Host32(other->image + (rec - other->wiiBase) + 0x10);
        if (head >= image && head < image + imageSize) ok = RootAdd(&lists, wiiBase + (u32)(head - image));
        else if (raw >= wiiBase && raw - wiiBase < imageSize) ok = RootAdd(&lists, raw);
    }
    #undef IN_ZONE
    out->entities = entities.items;         out->entityCount = entities.count;
    out->poolTables = pools.items;          out->poolTableCount = pools.count / 2;
    out->zoneLists = lists.items;           out->zoneListCount = lists.count;
    out->actionTables = actions.items;      out->actionTableCount = actions.count;
    out->soundGroups = groups.items;        out->soundGroupCount = groups.count;
    out->slotCopySize = slotSize;
    out->screenSet = Be32(image + 0x100);
    if (!ok) LevelZoneRootsFree(out);
    return ok ? TRUE : FALSE;
}

extern "C" void LevelZoneDoneFree(LevelZoneDone* done) {
    sPlanFree(done->regions);
    memset(done, 0, sizeof(*done));
}

extern "C" BOOL LevelZoneRootsCollect(const u8* hdr, u32 hdrSize, const u8* zoneHost, u32 zoneSize, u32 zoneWiiBase,
                                      const LevelZoneOther* other, LevelZoneRoots* out) {
    memset(out, 0, sizeof(*out));
    RootList entities = { NULL, 0, 0 }, scene = { NULL, 0, 0 }, pools = { NULL, 0, 0 }, records = { NULL, 0, 0 }, lists = { NULL, 0, 0 };
    RootList ropes = { NULL, 0, 0 };
    RootList objects = { NULL, 0, 0 }, effects = { NULL, 0, 0 }, actions = { NULL, 0, 0 }, groups = { NULL, 0, 0 }, recArrays = { NULL, 0, 0 };
    bool ok = hdrSize >= LEVEL_HEADER_SLOTS + 0x118;
    u32 count = ok ? Host32(hdr + 0x1A8) : 0, stride = LEVEL_SLOT_MIN_SIZE, entries, addr;
    const u8* slots = hdr + LEVEL_HEADER_SLOTS;
    if (ok && count > 1) {                                       // the slot size, as in LevelSlotSize
        const u8* first;
        const u8* next = NULL;
        memcpy(&first, slots + 0x114, sizeof(first));
        for (;; stride += 4) {
            if (stride > LEVEL_SLOT_MAX_SIZE || (u64)LEVEL_HEADER_SLOTS + stride + 0x118 > hdrSize) { ok = false; break; }
            memcpy(&next, slots + stride + 0x114, sizeof(next));
            if (next == first + stride) break;
        }
    }
    ok = ok && count <= LEVEL_MAX_ZONES && (u64)LEVEL_HEADER_SLOTS + (u64)count * stride <= hdrSize;

    if (ok) out->hud = ZoneAddr(hdr + 0x1F4, zoneHost, zoneSize, zoneWiiBase);
    if (ok && Host32(hdr + 0x24) != 0 && (out->locators = ZoneAddr(hdr + 0x28, zoneHost, zoneSize, zoneWiiBase)) != 0)
        out->locatorCount = Host32(hdr + 0x24);
    if (ok && Host32(hdr + 0x7C) != 0 && (out->effectLibrary = ZoneAddr(hdr + 0x80, zoneHost, zoneSize, zoneWiiBase)) != 0)
        out->effectLibraryCount = Host32(hdr + 0x7C);
    for (u32 k = 0; ok && k < 2; k++) out->spawnOwners[k] = ZoneAddr(hdr + 0x74 + k * 4, zoneHost, zoneSize, zoneWiiBase);
    if (ok && Host32(hdr + 0x48) != 0 && (out->partList = ZoneAddr(hdr + 0x44, zoneHost, zoneSize, zoneWiiBase)) != 0)
        out->partListCount = Host32(hdr + 0x48);
    if (ok && Host32(hdr + 0x30) != 0 && (out->paths = ZoneAddr(hdr + 0x2C, zoneHost, zoneSize, zoneWiiBase)) != 0)
        out->pathCount = Host32(hdr + 0x30);
    if (ok && Host32(hdr + 0x3C) != 0 && (out->junctions = ZoneAddr(hdr + 0x38, zoneHost, zoneSize, zoneWiiBase)) != 0)
        out->junctionCount = Host32(hdr + 0x3C);
    for (u32 t = 0; ok && t < 2; t++) {                          // scene objects: the tables at +0x1B0 and +0x1BC
        const u8* table = HeaderTable(hdr, hdrSize, hdr + kLevelBlockTables[t], &entries);
        for (u32 k = 0; ok && table != NULL && k < entries; k++)
            if ((addr = ZoneAddr(table + k * 4, zoneHost, zoneSize, zoneWiiBase)) != 0) ok = RootAdd(&scene, addr);
    }
    for (u32 i = 0; ok && i < count; i++) {
        const u8* slot = slots + i * stride;
        if ((addr = ZoneAddr(slot + 0x50, zoneHost, zoneSize, zoneWiiBase)) != 0) ok = RootAdd(&entities, addr);
        for (u32 t = 0; ok && t < 2; t++) {                      // the slot's two entity tables
            const u8* table = HeaderTable(hdr, hdrSize, slot + kLevelSlotTables[t], &entries);
            for (u32 k = 0; ok && table != NULL && k < entries; k++)
                if ((addr = ZoneAddr(table + k * 4, zoneHost, zoneSize, zoneWiiBase)) != 0) ok = RootAdd(&entities, addr);
        }
        if (ok && (addr = ZoneAddr(slot + 0xFC, zoneHost, zoneSize, zoneWiiBase)) != 0) ok = RootAdd(&records, addr);
        else if (ok && other != NULL) {                          // the record is in zone 0, converted: its list may be in this zone
            const u8* rec;
            memcpy(&rec, slot + 0xFC, sizeof(rec));
            // Zone 0's image may lie somewhere else than at its address (read to a buffer first).
            u32 recAddr = (u32)(uintptr_t)rec;
            if ((rec < other->image || rec >= other->image + other->imageSize) && recAddr >= other->wiiBase && recAddr - other->wiiBase < other->imageSize)
                rec = other->image + (recAddr - other->wiiBase);
            if (rec >= other->image && rec < other->image + other->imageSize && (u32)(other->image + other->imageSize - rec) >= 0x14 &&
                (addr = ZoneAddr(rec + 0x10, zoneHost, zoneSize, zoneWiiBase)) != 0) ok = RootAdd(&lists, addr);
        }
        // The slot's pool table {entries, count} at +0xE4: address and count, as a pair.
        if (ok && (addr = ZoneAddr(slot + 0xE4, zoneHost, zoneSize, zoneWiiBase)) != 0)
            ok = RootAdd(&pools, addr) && RootAdd(&pools, Host32(slot + 0xE8));
        // The slot's list of objects at +0xA8, as many as the byte at +0xAC says.
        if (ok && slot[0xAC] != 0 && (addr = ZoneAddr(slot + 0xA8, zoneHost, zoneSize, zoneWiiBase)) != 0)
            ok = RootAdd(&objects, addr) && RootAdd(&objects, slot[0xAC]);
        // The slot's action table at +0xD4 (TriggerDoAction, kind 2).
        if (ok && (addr = ZoneAddr(slot + 0xD4, zoneHost, zoneSize, zoneWiiBase)) != 0) ok = RootAdd(&actions, addr);
        if (ok && (addr = ZoneAddr(slot + 0x110, zoneHost, zoneSize, zoneWiiBase)) != 0) ok = RootAdd(&groups, addr);
        // The slot's two arrays of entity records (+0x8C statics, +0x90 dynamics; capacity at +0x124 / +0x130, as
        // fn_8012E420 hands them out). Records of pooled entities that no entity points at yet are converted too:
        // a projectile taken from the pool gets one of them and its spawn only sets some fields (the rest stay the
        // file's 0x7F800001, which the game then combines its flags with).
        for (u32 t = 0; ok && t < 2; t++)
            if ((addr = ZoneAddr(slot + 0x8C + t * 4, zoneHost, zoneSize, zoneWiiBase)) != 0)
                ok = RootAdd(&recArrays, addr) && RootAdd(&recArrays, Host32(slot + (t == 0 ? 0x124 : 0x130)));
        // The slot's particle systems at +0xA0, as many as the byte at +0xA4 says.
        if (ok && slot[0xA4] != 0 && (addr = ZoneAddr(slot + 0xA0, zoneHost, zoneSize, zoneWiiBase)) != 0)
            ok = RootAdd(&ropes, addr) && RootAdd(&ropes, slot[0xA4]);
        // The slot's placed effects at +0x80, as many as the word at +0x9C says.
        if (ok && Host32(slot + 0x9C) != 0 && (addr = ZoneAddr(slot + 0x80, zoneHost, zoneSize, zoneWiiBase)) != 0)
            ok = RootAdd(&effects, addr) && RootAdd(&effects, Host32(slot + 0x9C));
    }
    out->slotObjects = objects.items;       out->slotObjectCount = objects.count / 2;
    out->slotEffects = effects.items;       out->slotEffectCount = effects.count / 2;
    out->slotRopes = ropes.items;           out->slotRopeCount = ropes.count / 2;
    out->recordArrays = recArrays.items;    out->recordArrayCount = recArrays.count / 2;
    out->actionTables = actions.items;      out->actionTableCount = actions.count;
    out->soundGroups = groups.items;        out->soundGroupCount = groups.count;
    out->entities = entities.items;         out->entityCount = entities.count;
    out->sceneObjects = scene.items;        out->sceneObjectCount = scene.count;
    out->poolTables = pools.items;          out->poolTableCount = pools.count / 2;
    out->zoneRecords = records.items;       out->zoneRecordCount = records.count;
    out->zoneLists = lists.items;           out->zoneListCount = lists.count;
    if (!ok) LevelZoneRootsFree(out);
    return ok ? TRUE : FALSE;
}

// A projectile definition (level header +0x74 / +0x78; a spawned projectile's +0x228 and +0x9C), 0x170 bytes.
// SpawnHelper compares the type id at +0x00 with 0x37645F52. What the projectile code reads:
//   +0x08 id, +0x0C four words (+0x10: animation id, fn_80065200)
//   +0x60 a hash and 20 action words (+0x78: 0x10000005 for the coffin; the others are 0x0003FFFF, none)
//   +0xB4 a word (fn_80065D1C), +0xB8..+0xEC floats (speed, sizes, timers: fn_80063D6C, fn_800645C8,
//         fn_80065FF0, fn_80066E54, fn_800637AC), but +0xE8: the hash of its collision entry (fn_800637AC)
//   +0xF0..+0x100 effect hashes (spawn, trail, three kinds of hit: fn_80063D6C, fn_80066E54)
//   +0x104 name hash of the pool its model comes from (fn_80063D6C: HashTableFind in zone 0's pool table)
//   +0x108..+0x118 five sound objects (fn_80065200, fn_80065FF0, fn_80066E54)
//   +0x148 flags (fn_80063D6C), +0x14C an object (fn_80066E54 reads its s16 at +0x54; not typed),
//   +0x150 four floats and a duration (fn_80063D6C)
// Planned field by field: a definition may also be an entity's +0x9C, whose id at +0x08 is planned there.
#define PROJECTILE_DEF_SIZE 0x170
static SirError ProjectileDefPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    static const struct { u16 off, size; u8 kind; } kParts[] = {
        { 0x000, 0x04, REGION_U32_ARRAY }, { 0x008, 0x04, REGION_U32_ARRAY }, { 0x00C, 0x10, REGION_U32_ARRAY },
        { 0x060, 0xA8, REGION_U32_ARRAY }, { 0x108, 0x14, REGION_PTR_ARRAY }, { 0x148, 0x04, REGION_U32_ARRAY },
        { 0x14C, 0x04, REGION_PTR_ARRAY }, { 0x150, 0x14, REGION_U32_ARRAY },
    };
    u32 o;
    if (!LevelSpan(addr, PROJECTILE_DEF_SIZE, imageSize, &o)) return SIR_OK;   // none, or in another zone
    if (PlanHas(plan, o + 0x060, REGION_U32_ARRAY)) return SIR_OK;
    for (u32 i = 0; i < COUNTOF(kParts); i++) {
        if (PlanHas(plan, o + kParts[i].off, kParts[i].kind)) continue;
        if (!PlanAdd(plan, o + kParts[i].off, kParts[i].size, kParts[i].kind)) return SIR_ERR_LEVEL;
    }
    for (u32 i = 0; i < 5; i++) {
        SirError e = SoundPlan(plan, image, imageSize, Be32(image + o + 0x108 + i * 4));
        if (e != SIR_OK) return e;
    }
    return SIR_OK;
}

// The part list (level header +0x44, count +0x48): pointers to 0x10-byte entries, one per monster part model and
// player: {+0x00 use count, +0x04 name hash of the part's geometry ("p_hd_sha001_geoA"), +0x08 the part's object
// (a placeholder in the file; fn_8003DA7C stores the zone's resource index + 1), +0x0C the zone that holds the part}.
// fn_8003DB78 looks a part up by hash, claims the entry and asks for its zone (ZoneLoadAsync).
#define PART_LIST_ENTRY_SIZE 0x10
static SirError PartListPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 count) {
    u32 t;
    if (count == 0) return SIR_OK;
    if (count > imageSize / 4 || !LevelSpan(addr, (u64)count * 4, imageSize, &t)) return SIR_ERR_LEVEL;
    if (PlanHas(plan, t, REGION_PTR_ARRAY)) return SIR_OK;
    if (!PlanAdd(plan, t, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_LEVEL;
    for (u32 i = 0; i < count; i++) {
        u32 o;
        if (!LevelSpan(Be32(image + t + i * 4), PART_LIST_ENTRY_SIZE, imageSize, &o)) continue;   // none, or in another zone
        if (PlanHas(plan, o, REGION_U32_ARRAY)) continue;
        if (!PlanAdd(plan, o, PART_LIST_ENTRY_SIZE, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
    }
    return SIR_OK;
}

// A zone slot's pool table (slot +0xE4, count +0xE8): 0x2C-byte entries {+0x04 pool, +0x0C index
// of a script function} (kPoolTableEntryFields). fn_801DB1AC runs that function over the pool's
// items at level start.

// A zone slot's objects (slot +0xA8, count in the byte at +0xAC): pointers to objects whose type
// word at +0x08 picks the class fn_8012FD0C sets up again when the zone starts (types 0 to 5, 8,
// 9 and 15 to 18). That is all that is typed of them so far.
#define CAMERA_BASE_USED 0x64
#define CAMERA_NODE_SIZE 0x14
#define CAMERA_TRACK_TYPE 15
#define CAMERA_TRACK_USED 0x60
#define CAMERA_AIM_TYPE 16
#define CAMERA_AIM_USED 0xB8
static SirError SlotObjectsPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 count) {
    u32 off, o;
    if (count == 0) return SIR_OK;
    if (!LevelSpan(addr, (u64)count * 4, imageSize, &off) || !PlanAdd(plan, off, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_LEVEL;
    for (u32 i = 0; i < count; i++) {
        if (!LevelSpan(Be32(image + off + i * 4), 0x0C, imageSize, &o) || PlanHas(plan, o + 8, REGION_U32_ARRAY)) continue;
        if (!PlanAdd(plan, o + 8, 4, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
        // +0x04 is the class hash (0xCC05AC21 "CIAnimatedProp" on the animated camera prop): StopUsingAnimatedCamera
        // (fn_800F6774) looks for it, and without it never hands the camera back after the village fly-over.
        if (!PlanAdd(plan, o + 4, 4, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
        // What every type starts with (fn_800DECD8, fn_800DE678): +0x0C how many boundary nodes, +0x10
        // the nodes (0x14 bytes: a word, a position, a float), +0x14 how many segments, +0x18 the
        // segments (two node indices, bytes), +0x34..+0x53 eight floats (angle, timers, blend), +0x54
        // four bytes. Type 1 follows the object at +0x58 (fn_800E3040); type 2 has a position there.
        {
            u32 t, type = Be32(image + o + 8), nodes;
            if (LevelSpan(Be32(image + off + i * 4), CAMERA_BASE_USED, imageSize, &t) && !PlanHas(plan, t + 0x0C, REGION_U32_ARRAY)) {
                if (!PlanAdd(plan, t + 0x0C, 4, REGION_U32_ARRAY) || !PlanAdd(plan, t + 0x10, 4, REGION_PTR_ARRAY) ||
                    !PlanAdd(plan, t + 0x14, 4, REGION_U32_ARRAY) || !PlanAdd(plan, t + 0x18, 4, REGION_PTR_ARRAY) ||
                    !PlanAdd(plan, t + 0x34, 0x20, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
                if (type == 1 && !PlanAdd(plan, t + 0x58, 4, REGION_PTR_ARRAY)) return SIR_ERR_LEVEL;
                if (type == 2 && !PlanAdd(plan, t + 0x58, 12, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
                u32 count = Be32(image + t + 0x0C);
                if (count != 0 && count <= imageSize / CAMERA_NODE_SIZE && LevelSpan(Be32(image + t + 0x10), (u64)count * CAMERA_NODE_SIZE, imageSize, &nodes) &&
                    !PlanHas(plan, nodes, REGION_U32_ARRAY) && !PlanAdd(plan, nodes, count * CAMERA_NODE_SIZE, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
            }
        }
        // Type 15 is an animated camera track (vtable lbl_803601B8, fn_800E43BC): +0x58 is the handle of
        // the entity whose animation is the camera path and +0x5C the hash of its clip. Without them
        // the camera never finds its entity and the menu camera stays where the follow state puts it.
        if (Be32(image + o + 8) == CAMERA_TRACK_TYPE) {
            u32 t;
            if (LevelSpan(Be32(image + off + i * 4), CAMERA_TRACK_USED, imageSize, &t) && !PlanHas(plan, t + 0x58, REGION_U32_ARRAY) &&
                !PlanAdd(plan, t + 0x58, 8, REGION_U32_ARRAY))
                return SIR_ERR_LEVEL;
        }
        // Type 16 (vtable lbl_80360200, fn_800E5550) aims between locators: +0xA8 / +0xAC the two
        // it looks from and to, +0xB0 / +0xB4 two whose vectors it copies (fn_800E5684 resolves the
        // four reference words with fn_801ED684). The constructor resets the rest.
        if (Be32(image + o + 8) == CAMERA_AIM_TYPE) {
            u32 t;
            if (LevelSpan(Be32(image + off + i * 4), CAMERA_AIM_USED, imageSize, &t) && !PlanHas(plan, t + 0xA8, REGION_U32_ARRAY) &&
                !PlanAdd(plan, t + 0xA8, 16, REGION_U32_ARRAY))
                return SIR_ERR_LEVEL;
        }
    }
    return SIR_OK;
}

static SirError SlotEffectsPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 count) {
    u32 off, o;
    if (count == 0) return SIR_OK;
    if (count > imageSize / 4 || !LevelSpan(addr, (u64)count * 4, imageSize, &off) ||
        !PlanAdd(plan, off, count * 4, REGION_PTR_ARRAY)) return SIR_ERR_LEVEL;
    for (u32 i = 0; i < count; i++) {
        if (!LevelSpan(Be32(image + off + i * 4), ZONE_EFFECT_SIZE, imageSize, &o) || PlanHas(plan, o, REGION_ZONE_EFFECT)) continue;
        if (!PlanAdd(plan, o, ZONE_EFFECT_SIZE, REGION_ZONE_EFFECT)) return SIR_ERR_LEVEL;
    }
    return SIR_OK;
}

// A zone slot's particle systems (slot +0xA0, count in the byte at +0xA4), 0x4C bytes each: chains of points held
// together by links, which the game steps every frame (fn_800098F4 -> fn_8010A76C -> fn_801D0C04, fn_801D0DDC)
// and finds by name (fn_800784EC: the Weld-O-Tron's "tile_start_cm", "tile%d%d_%s_cm"; fn_800C4118, fn_8006FC04).
//   +0x00 the points and +0x04 how many: 0x28 bytes each, nine floats (position before, position now, push) and at
//         +0x24 the "held in place" byte with three more bytes, left as they are
//   +0x08 the links and +0x0C how many: 0x24 bytes, nine words (two point numbers, the kind, then floats)
//   +0x10 how many passes over the links, +0x14 a push on every point (three floats)
//   +0x20 a third list and +0x24 how many: 0x0C bytes {word, four colour bytes, float}; +0x28 a word
//   +0x2C the hash of the name, +0x30 flags (bit 0: switched off, bit 1: shakes), +0x34..+0x44 floats (+0x3C the
//         size of a shake, +0x44 the time to the next one); +0x48 four bytes
#define ROPE_SIZE       0x4C
#define ROPE_POINT_SIZE 0x28
#define ROPE_LINK_SIZE  0x24
#define ROPE_NODE_SIZE  0x0C
static SirError SlotRopesPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 count) {
    u32 off, o;
    if (count == 0 || !LevelSpan(addr, (u64)count * ROPE_SIZE, imageSize, &off) || PlanHas(plan, off, REGION_PTR_ARRAY)) return SIR_OK;
    for (u32 i = 0; i < count; i++) {
        u32 r = off + i * ROPE_SIZE;
        const u8* rec = image + r;
        if (!PlanAdd(plan, r, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, r + 0x04, 4, REGION_U32_ARRAY) ||
            !PlanAdd(plan, r + 0x08, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, r + 0x0C, 0x14, REGION_U32_ARRAY) ||
            !PlanAdd(plan, r + 0x20, 4, REGION_PTR_ARRAY) || !PlanAdd(plan, r + 0x24, 0x24, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
        u32 points = Be32(rec + 0x04), links = Be32(rec + 0x0C), nodes = Be32(rec + 0x24);
        if (points > imageSize / ROPE_POINT_SIZE || links > imageSize / ROPE_LINK_SIZE || nodes > imageSize / ROPE_NODE_SIZE) return SIR_ERR_LEVEL;
        if (points != 0 && LevelSpan(Be32(rec), (u64)points * ROPE_POINT_SIZE, imageSize, &o) && !PlanHas(plan, o, REGION_F32_ARRAY)) {
            for (u32 k = 0; k < points; k++)
                if (!PlanAdd(plan, o + k * ROPE_POINT_SIZE, 0x24, REGION_F32_ARRAY)) return SIR_ERR_LEVEL;
        }
        if (links != 0 && LevelSpan(Be32(rec + 0x08), (u64)links * ROPE_LINK_SIZE, imageSize, &o) && !PlanHas(plan, o, REGION_U32_ARRAY) &&
            !PlanAdd(plan, o, links * ROPE_LINK_SIZE, REGION_U32_ARRAY)) return SIR_ERR_LEVEL;
        if (nodes != 0 && LevelSpan(Be32(rec + 0x20), (u64)nodes * ROPE_NODE_SIZE, imageSize, &o) && !PlanHas(plan, o, REGION_U32_ARRAY)) {
            for (u32 k = 0; k < nodes; k++)
                if (!PlanAdd(plan, o + k * ROPE_NODE_SIZE, 4, REGION_U32_ARRAY) || !PlanAdd(plan, o + k * ROPE_NODE_SIZE + 8, 4, REGION_F32_ARRAY)) return SIR_ERR_LEVEL;
        }
    }
    return SIR_OK;
}

static SirError PoolTablePlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr, u32 count) {
    u32 off;
    if (count == 0) return SIR_OK;
    if (!ImageSpan(addr, (u64)count * POOL_TABLE_ENTRY, imageSize, &off) ||
        !PlanAdd(plan, off, count * POOL_TABLE_ENTRY, REGION_POOL_TABLE)) return SIR_ERR_POOL;
    for (u32 i = 0; i < count; i++) {
        const u8* entry = image + off + i * POOL_TABLE_ENTRY;
        u32 pool = Be32(entry + 0x04), fixUp = Be32(entry + 0x0C), poolOff, o;
        if (!LevelSpan(pool, POOL_SIZE, imageSize, &poolOff) || PlanHas(plan, poolOff, REGION_POOL)) continue;
        bool typed = fixUp >= LEVEL_FIXUP_ENTITY && fixUp <= LEVEL_FIXUP_FSM;
        SirError e = PoolPlan(plan, image, imageSize, pool, typed);
        if (e != SIR_OK) return e;
        if (!typed) continue;
        // What an item is follows from the function fn_801DB1AC runs over the pool's items.
        PoolLayout l;
        PoolLayoutRead(image + poolOff, imageSize, &l);
        for (u32 k = 0; k < l.count && e == SIR_OK; k++) {
            u32 itemOff = l.firstOff + k * l.stride + 8;
            u32 item = sImageBase + itemOff;
            switch (fixUp) {
            case LEVEL_FIXUP_ENTITY:
                sPooledEntityType = Be32(entry + 0x1C) < 0x20 ? Be32(entry + 0x1C) : 0;
                e = EntityPlan(plan, image, imageSize, item, 0);
                sPooledEntityType = 0;
                break;
            case LEVEL_FIXUP_ENTITY_PART:
                e = EntityPlan(plan, image, imageSize, item, 0);
                break;
            case LEVEL_FIXUP_PROJECTILE:       // an entity, and at +0x22C a part whose vtable the callback stores
                if (l.stride - 8 < 0x230 || !PlanAdd(plan, itemOff + 0x22C, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
                e = EntityPlan(plan, image, imageSize, item, 0);
                break;
            case LEVEL_FIXUP_POOLED_ENTITY:    // a slot holding an entity
                if (!PlanAdd(plan, itemOff, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
                e = EntityPlan(plan, image, imageSize, Be32(image + itemOff), 0);
                break;
            case LEVEL_FIXUP_RENDER_OBJECT:    // a slot holding a scene object, which fn_8012F818 sets up by its type
                if (!PlanAdd(plan, itemOff, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
                e = SceneObjectPlan(plan, image, imageSize, Be32(image + itemOff));
                break;
            case LEVEL_FIXUP_FSM:              // a slot holding a state machine
                if (!PlanAdd(plan, itemOff, 4, REGION_PTR_ARRAY)) return SIR_ERR_ENTITY;
                if (LevelSpan(Be32(image + itemOff), FSM_USED, imageSize, &o)) e = FsmPlan(plan, image, imageSize, Be32(image + itemOff));
                break;
            }
        }
        if (e != SIR_OK) return e;
    }
    return SIR_OK;
}

// An object of zone 0 (converted) by a pointer read from it: where it is now, or the address it will have.
static bool OtherOff(const u8* p, u64 size, u32* off) {
    if (p >= sOtherImage && p < sOtherImage + sOtherSize) *off = (u32)(p - sOtherImage);
    else if ((u32)(uintptr_t)p >= sOtherBase && (u32)(uintptr_t)p - sOtherBase < sOtherSize) *off = (u32)(uintptr_t)p - sOtherBase;
    else return false;
    return (u64)*off + size <= sOtherSize;
}

// The sound objects of the zone being converted that the action records of zone 0 name (a level's
// voice lines: the quest machines of zone 0 call PlayVoiceSFX with sounds of zone 1). Zone 0's
// tables and records are converted: host words, and the operand is the sound's address.
static SirError OtherActionSoundsPlan(ImagePlan* plan, const u8* image, u32 imageSize) {
    for (u32 t = 0; sOtherDone != NULL && t < sOtherDone->count; t++) {
        const Region* g = &sOtherDone->regions[t];
        if (g->kind != REGION_ACTION_TABLE || g->size < ACTION_TABLE_SIZE) continue;
        u32 count = Host32(sOtherImage + g->off), array;
        const u8* p;
        memcpy(&p, sOtherImage + g->off + 4, sizeof(p));
        if (!OtherOff(p, (u64)count * 4, &array)) continue;
        for (u32 i = 0; i < count; i++) {
            u32 rec;
            memcpy(&p, sOtherImage + array + i * 4, sizeof(p));
            if (p == NULL || !OtherOff(p, 4, &rec) || ActionOp(Host32(sOtherImage + rec)) != ACTION_OP_HEADER) continue;
            for (u32 k = 0; (u64)rec + ((u64)k + 1) * 4 <= sOtherSize;) {
                u32 word = Host32(sOtherImage + rec + k * 4), n = ActionInstructionWords(word);
                if (ActionOp(word) == ACTION_OP_END || n == 0 || (u64)rec + ((u64)k + n) * 4 > sOtherSize) break;
                for (u32 c = 0; ActionOp(word) == ACTION_OP_CALL && c < SIR_SOUND_CALLS; c++) {
                    if (kSirSoundCalls[c].func != ActionJump(word)) continue;
                    for (u32 a = 0; a + 1 < ActionLen(word); a++) {
                        if (!(kSirSoundCalls[c].argMask >> a & 1)) continue;
                        SirError e = SoundPlan(plan, image, imageSize, Host32(sOtherImage + rec + (k + 1 + a) * 4));
                        if (e != SIR_OK) return e;
                    }
                }
                k += n;
            }
        }
    }
    return SIR_OK;
}

extern "C" SirError LevelZoneConvert(SirReloc* r, u8* image, u32 imageSize, u32 wiiBase, const LevelZoneRoots* roots,
                                     LevelZoneDone* done, const LevelZoneOther* other) {
    ImagePlan plan = { NULL, 0, 0 }, merged = { NULL, 0, 0 };
    if (LoaderWatch() != 0)
        fprintf(stderr, "[loader watch] zone image at 0x%08X size 0x%X (converted at %p); zone 0 at 0x%08X size 0x%X (%p)%c",
                wiiBase, imageSize, image, other != NULL ? other->wiiBase : 0, other != NULL ? other->imageSize : 0, other != NULL ? other->image : NULL, 10);
    sImageBase = wiiBase;
    sLevelZone = true;
    if (other != NULL) {
        sOtherImage = other->image;
        sOtherSize = other->imageSize;
        sOtherBase = other->wiiBase;
        sOtherDone = (const ImagePlan*)other->done;
    }
    SirError e = SIR_OK;
    if (roots->hud != 0) e = HudPlan(&plan, image, imageSize, roots->hud, true);
    for (u32 i = 0; i < roots->entityCount && e == SIR_OK; i++) e = EntityPlan(&plan, image, imageSize, roots->entities[i], 0);
    for (u32 i = 0; i < roots->sceneObjectCount && e == SIR_OK; i++) e = SceneObjectPlan(&plan, image, imageSize, roots->sceneObjects[i]);
    if (e == SIR_OK && roots->effectLibraryCount != 0) e = FxEffectLibraryPlan(&plan, image, imageSize, roots->effectLibrary, roots->effectLibraryCount);
    for (u32 i = 0; i < roots->poolTableCount && e == SIR_OK; i++)
        e = PoolTablePlan(&plan, image, imageSize, roots->poolTables[i * 2], roots->poolTables[i * 2 + 1]);
    if (e == SIR_OK && roots->locatorCount != 0) {
        u32 l;
        if (roots->locatorCount > imageSize / LOCATOR_SIZE || !LevelSpan(roots->locators, (u64)roots->locatorCount * LOCATOR_SIZE, imageSize, &l) ||
            !PlanAdd(&plan, l, roots->locatorCount * LOCATOR_SIZE, REGION_LOCATORS)) e = SIR_ERR_LEVEL;
    }
    // The objects projectiles are spawned from (header +0x74, +0x78): projectile definitions.
    for (u32 k = 0; k < 2 && e == SIR_OK; k++) e = ProjectileDefPlan(&plan, image, imageSize, roots->spawnOwners[k]);
    if (e == SIR_OK) e = PartListPlan(&plan, image, imageSize, roots->partList, roots->partListCount);
    // The exploration map (header +0x2C / +0x38): fn_8013006C copies every path record (0x1AC bytes,
    // fn_800D4DE8: words, but for the bytes at +0x64 and +0x9C) and every junction record (0xDC
    // bytes, fn_800D5008: six words, up to twelve entries of three floats and a byte from +0x18,
    // their count at +0xD8) into the object database.
    for (u32 t = 0; t < 2 && e == SIR_OK; t++) {
        u32 list = t == 0 ? roots->paths : roots->junctions, count = t == 0 ? roots->pathCount : roots->junctionCount, l, rec;
        if (count == 0) continue;
        if (count > imageSize / 4 || !LevelSpan(list, (u64)count * 4, imageSize, &l) || !PlanAdd(&plan, l, count * 4, REGION_PTR_ARRAY)) { e = SIR_ERR_LEVEL; break; }
        for (u32 i = 0; i < count && e == SIR_OK; i++) {
            if (!LevelSpan(Be32(image + l + i * 4), t == 0 ? MAP_PATH_SIZE : MAP_JUNCTION_SIZE, imageSize, &rec)) continue;
            bool ok;
            if (t == 0) {
                ok = PlanAdd(&plan, rec, 0x64, REGION_U32_ARRAY) && PlanAdd(&plan, rec + 0x68, 0x34, REGION_U32_ARRAY) &&
                     PlanAdd(&plan, rec + 0xA0, MAP_PATH_SIZE - 0xA0, REGION_U32_ARRAY);
            } else {
                ok = PlanAdd(&plan, rec, 0x18, REGION_U32_ARRAY) && PlanAdd(&plan, rec + 0xD8, 4, REGION_U32_ARRAY);
                for (u32 k = 0; ok && k < 12; k++) ok = PlanAdd(&plan, rec + 0x18 + k * 0x10, 0xC, REGION_U32_ARRAY);
            }
            if (!ok) e = SIR_ERR_LEVEL;
        }
    }
    // A zone's action table is laid out like the one of global_binary.sir, and its records are
    // programs for the same interpreter (TriggerDoAction -> fn_801E1BD4).
    for (u32 i = 0; i < roots->actionTableCount && e == SIR_OK; i++) {
        e = PtrArrayPlan(&plan, image, imageSize, roots->actionTables[i], ACTION_TABLE_SIZE, 0x00, 0x04, REGION_ACTION_TABLE, SIR_ERR_ACTIONS);
        if (e == SIR_OK) e = ActionRecordsPlan(&plan, image, imageSize, roots->actionTables[i]);
    }
    for (u32 i = 0; i < roots->soundGroupCount && e == SIR_OK; i++) e = SoundGroupPlan(&plan, image, imageSize, roots->soundGroups[i]);
    for (u32 i = 0; i < roots->slotEffectCount && e == SIR_OK; i++)
        e = SlotEffectsPlan(&plan, image, imageSize, roots->slotEffects[i * 2], roots->slotEffects[i * 2 + 1]);
    for (u32 i = 0; i < roots->slotRopeCount && e == SIR_OK; i++)
        e = SlotRopesPlan(&plan, image, imageSize, roots->slotRopes[i * 2], roots->slotRopes[i * 2 + 1]);
    for (u32 i = 0; i < roots->slotObjectCount && e == SIR_OK; i++)
        e = SlotObjectsPlan(&plan, image, imageSize, roots->slotObjects[i * 2], roots->slotObjects[i * 2 + 1]);
    for (u32 i = 0; i < roots->recordArrayCount && e == SIR_OK; i++) {
        u32 addr = roots->recordArrays[i * 2], cap = roots->recordArrays[i * 2 + 1], o;
        if (cap > imageSize / ENTITY_RECORD_SIZE) { e = SIR_ERR_LEVEL; break; }
        for (u32 k = 0; k < cap; k++) {
            if (!LevelSpan(addr + k * ENTITY_RECORD_SIZE, ENTITY_RECORD_SIZE, imageSize, &o)) break;
            bool taken = false;                  // a record some entity leads to is planned already; others overlap nothing
            for (u32 j = 0; j < plan.count && !taken; j++)
                taken = plan.regions[j].off < o + ENTITY_RECORD_SIZE && o < plan.regions[j].off + plan.regions[j].size;
            if (!taken && !PlanAdd(&plan, o, ENTITY_RECORD_SIZE, REGION_ENTITY_RECORD)) { e = SIR_ERR_LEVEL; break; }
        }
    }
    for (u32 i = 0; i < roots->zoneRecordCount && e == SIR_OK; i++) e = ZoneRecordPlan(&plan, image, imageSize, roots->zoneRecords[i]);
    for (u32 i = 0; i < roots->zoneListCount && e == SIR_OK; i++) e = ZoneListPlan(&plan, image, imageSize, roots->zoneLists[i]);
    // A shared zone: the slot copy at the start (converted below, by the slot's own field table) and its screen set.
    if (e == SIR_OK && roots->slotCopySize != 0 && (roots->slotCopySize > imageSize || !PlanAdd(&plan, 0, roots->slotCopySize, REGION_RUNTIME)))
        e = SIR_ERR_LEVEL;
    // Its two entity tables (slot +0x120, +0x12C), when they lie in the image: a part zone's second table holds the
    // part's entity (fn_8012FD0C runs fn_8012F8B8 over both when the zone starts).
    for (u32 t = 0; e == SIR_OK && roots->slotCopySize != 0 && t < 2; t++) {
        u32 entries = Be32(image + kLevelSlotTables[t] + 4), o;
        if (entries == 0 || entries > imageSize / 4 || !LevelSpan(Be32(image + kLevelSlotTables[t]), (u64)entries * 4, imageSize, &o)) continue;
        if (!PlanHas(&plan, o, REGION_PTR_ARRAY) && !PlanAdd(&plan, o, entries * 4, REGION_PTR_ARRAY)) e = SIR_ERR_LEVEL;
    }
    if (e == SIR_OK && roots->screenSet != 0) e = ScreenSetPlan(&plan, image, imageSize, roots->screenSet);
    if (e == SIR_OK && other != NULL) e = OtherFsmValuesPlan(&plan, image, imageSize);
    if (e == SIR_OK && other != NULL) e = OtherActionSoundsPlan(&plan, image, imageSize);
    if (e == SIR_OK && !PlanFinish(&plan)) e = SIR_ERR_OVERLAP;
    // What this zone adds in zone 0: disjoint among itself and from what is converted there.
    if (e == SIR_OK && !PlanFinish(&sOtherNew)) e = SIR_ERR_OVERLAP;
    if (e == SIR_OK && sOtherDone != NULL && sOtherNew.count != 0) {
        for (u32 i = 0; i < sOtherDone->count && e == SIR_OK; i++)
            if (!PlanAdd(&merged, sOtherDone->regions[i].off, sOtherDone->regions[i].size, sOtherDone->regions[i].kind)) e = SIR_ERR_ENTITY;
        for (u32 i = 0; i < sOtherNew.count && e == SIR_OK; i++)
            if (!PlanAdd(&merged, sOtherNew.regions[i].off, sOtherNew.regions[i].size, sOtherNew.regions[i].kind)) e = SIR_ERR_ENTITY;
        if (e == SIR_OK && !PlanFinish(&merged)) e = SIR_ERR_OVERLAP;
    }
    if (e == SIR_OK) {
        sConvertBase = sImageBase;
        ImagePlanConvert(&plan, r, image, imageSize);
        if (roots->slotCopySize != 0) SirConvertRecord(r, image, roots->slotCopySize, kLevelSlotFields, COUNTOF(kLevelSlotFields));
        sConvertBase = sOtherBase;
        if (other != NULL) ImagePlanConvert(&sOtherNew, r, other->image, other->imageSize);
        sConvertBase = 0;
        if (merged.count != 0) {                                 // zone 0's list of converted objects grows
            LevelZoneDoneFree(other->done);
            other->done->regions = merged.regions;
            other->done->count = merged.count;
            other->done->capacity = merged.capacity;
            merged.regions = NULL;
        }
        if (done != NULL) {
            LevelZoneDoneFree(done);
            done->regions = plan.regions;
            done->count = plan.count;
            done->capacity = plan.capacity;
            plan.regions = NULL;
        }
    }
    PlanFree(&plan);
    PlanFree(&merged);
    PlanFree(&sOtherNew);
    sOtherImage = NULL;
    sOtherDone = NULL;
    sImageBase = GLOBAL_BINARY_WII_BASE;
    sLevelZone = false;
    return e;
}

// --------------------------------------------------------------------------- texture stream

// Offset of the width (the height follows) of the texture record at TEXTURE_STREAM_RECORD, or 0
// when the stream is not what fn_8016B5E0 and fn_801840E4 expect.
static u32 TextureStreamDims(const u8* data, u32 size) {
    if (data == NULL || size < TEXTURE_STREAM_RECORD + 8 || memcmp(data, "SG21WI00", 8) != 0) return 0;
    u32 recSize = Be32(data + TEXTURE_STREAM_RECORD);
    if (recSize < 12 || recSize > size - TEXTURE_STREAM_RECORD) return 0;
    u32 end = TEXTURE_STREAM_RECORD + recSize, p = TEXTURE_STREAM_RECORD + 4;
    while (p < end && data[p] != 0) p++;                         // the name
    if (p >= end) return 0;
    p += 1;                                                      // p: format and three more bytes
    u32 q = (p + 7) & ~3u;                                       // as fn_801840E4 rounds the cursor
    return q + 4 <= end ? q : 0;
}

extern "C" SirError TextureStreamConvert(u8* data, u32 size) {
    u32 q = TextureStreamDims(data, size);
    if (q == 0) return SIR_ERR_TEXTURE;
    Swap32(data);
    Swap32(data + 4);
    Swap32(data + TEXTURE_STREAM_RECORD);
    Swap16(data + q);
    Swap16(data + q + 2);
    return SIR_OK;
}

// A voice line of shared_voice.sir: the first 0x60 bytes of a .dsp file (format 0 ADPCM, 0x0A
// 16-bit, 0x19 8-bit: fn_80203340 accepts no other) with the samples behind them, which stay as
// they are. Nothing is changed when the header is not one (SIR_ERR_SOUND).
extern "C" SirError VoiceStreamConvert(u8* data, u32 size) {
    if (size < DSP_HEADER_SIZE) return SIR_ERR_SOUND;
    u32 format = (u32)((data[0xE] << 8) | data[0xF]), rate = Be32(data + 8);
    if ((format != 0 && format != 0x0A && format != 0x19) || rate == 0 || rate > 96000) return SIR_ERR_SOUND;
    SirReloc r;
    SirRelocInit(&r, NULL, NULL);
    SirConvertRecord(&r, data, DSP_HEADER_SIZE, kDspHeaderFields, COUNTOF(kDspHeaderFields));
    return SIR_OK;
}

// ------------------------------------------------------------------------------------ file helper

extern "C" void GlobalBinaryFree(GlobalBinaryFile* f) {
    free(f->hdr);
    free(f->image);
    f->hdr = f->image = NULL;
    f->imageSize = 0;
}

extern "C" SirError GlobalBinaryLoadFile(const char* path, SirReloc* r, GlobalBinaryFile* out, s32* failTable) {
    memset(out, 0, sizeof(*out));
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) return SIR_ERR_IO;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    u8* file = len > 0 ? (u8*)malloc((size_t)len) : NULL;
    bool ok = file != NULL && fread(file, 1, (size_t)len, fp) == (size_t)len;
    fclose(fp);
    if (!ok) {
        free(file);
        return SIR_ERR_IO;
    }

    SirError e = SIR_OK;
    SirSection hdrSec, imgSec;
    if (SirContainerSectionCount(file, (u32)len) < 0) {
        e = SIR_ERR_NOT_SIR;
    } else if (SirContainerSectionCount(file, (u32)len) != 2 || !SirContainerSection(file, (u32)len, 0, &hdrSec) ||
               !SirContainerSection(file, (u32)len, 1, &imgSec) || hdrSec.type != -1 ||
               hdrSec.size != GLOBAL_BINARY_HDR_SIZE || imgSec.type != 0 || imgSec.size == 0) {
        e = SIR_ERR_SECTIONS;
    } else {
        out->hdr = (u8*)malloc(GLOBAL_BINARY_HDR_SIZE);
        out->image = (u8*)malloc(imgSec.size);
        out->imageSize = imgSec.size;
        if (out->hdr == NULL || out->image == NULL) {
            e = SIR_ERR_IO;
        } else {
            memcpy(out->hdr, file + hdrSec.offset, GLOBAL_BINARY_HDR_SIZE);
            memcpy(out->image, file + imgSec.offset, imgSec.size);
            e = GlobalBinaryConvert(r, out->hdr, out->image, out->imageSize, failTable);
        }
    }
    free(file);
    if (e != SIR_OK) GlobalBinaryFree(out);
    return e;
}
