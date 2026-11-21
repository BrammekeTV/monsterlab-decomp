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
    case SIR_ERR_ACTIONS:        return "action table array or a record outside the image";
    case SIR_ERR_UI:             return "UI root, font or UI object not recognised (pointer outside the image, font kind or page count)";
    case SIR_ERR_SCREENS:        return "screen state machine not recognised (count, node type or pointer outside the image)";
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
    REGION_UI_ROOT, REGION_UI_FONT, REGION_UI_FONT_OBJ, REGION_UI_FONT_LIST, REGION_UI_DRAW, REGION_UI_PAD,
    REGION_FSM, REGION_FSM_DEF, REGION_FSM_STATES, REGION_FSM_EVENTS, REGION_FSM_SHARED_EVENTS, REGION_FSM_NODE,
    REGION_FSM_TRANSITION, REGION_FSM_SHARED_TRANSITION, REGION_HUD
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

static bool PlanAdd(ImagePlan* plan, u32 off, u32 size, u8 kind) {
    if (plan->count == plan->capacity) {
        u32 cap = plan->capacity ? plan->capacity * 2 : 1024;
        if (cap > REGION_MAX) return false;
        Region* grown = (Region*)realloc(plan->regions, (size_t)cap * sizeof(Region));
        if (grown == NULL) return false;
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
    free(plan->regions);
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

// Offset in the image of the `size` bytes at Wii address `addr`; false if they are not all inside.
static bool ImageSpan(u32 addr, u64 size, u32 imageSize, u32* off) {
    if (addr < GLOBAL_BINARY_WII_BASE) return false;
    u32 o = addr - GLOBAL_BINARY_WII_BASE;
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
        if (node < GLOBAL_BINARY_WII_BASE + SIR_TEXTURE_NODE ||
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
    if (values != 0) {
        if (!ImageSpan(values, 4, imageSize, &valuesOff)) return SIR_ERR_SCREENS;
        u32 count = Be32(image + valuesOff);
        if (!ImageSpan(values, 4 + (u64)count * 4, imageSize, &valuesOff) ||
            !PlanAdd(plan, valuesOff, 4 + count * 4, REGION_U32_ARRAY)) return SIR_ERR_SCREENS;
    }
    u32 def = Be32(fsm + 0x00);
    return def == 0 ? SIR_OK : FsmDefPlan(plan, image, imageSize, def);
}

static SirError HudPlan(ImagePlan* plan, const u8* image, u32 imageSize, u32 addr) {
    u32 off;
    if (!ImageSpan(addr, HUD_SIZE, imageSize, &off)) return SIR_ERR_SCREENS;
    const u8* hud = image + off;
    u32 count = Be32(hud + 0x428);
    if (count > HUD_MACHINES || !PlanAdd(plan, off, HUD_SIZE, REGION_HUD)) return SIR_ERR_SCREENS;
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

static void ConvertRecords(SirReloc* r, u8* p, u32 size, u32 recSize, const SirField* fields, s32 n) {
    for (u32 o = 0; o + recSize <= size; o += recSize) SirConvertRecord(r, p + o, recSize, fields, n);
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
    if (e == SIR_OK && (addr = Be32(hdr + 0xDB0)) != 0) e = UiRootPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDB4)) != 0) e = UiFontListPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDB8)) != 0) e = UiDrawPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDBC)) != 0) e = UiPadPlan(plan, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xD98)) != 0) e = FsmPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xD9C)) != 0) e = HudPlan(plan, image, imageSize, addr);
    if (e != SIR_OK) return e;
    return PlanFinish(plan) ? SIR_OK : SIR_ERR_OVERLAP;
}

static void ImagePlanConvert(const ImagePlan* plan, SirReloc* r, u8* image) {
    for (u32 i = 0; i < plan->count; i++) {
        const Region* g = &plan->regions[i];
        u8* p = image + g->off;
        switch (g->kind) {
        case REGION_INPUT_TABLE:  InputTableConvert(r, p); break;
        case REGION_INPUT_STEP:   InputStepConvert(r, p, g->size); break;
        case REGION_TEXTURE_LIST: SirConvertRecord(r, p, g->size, kTextureListFields, COUNTOF(kTextureListFields)); break;
        case REGION_TEXTURE:      SirConvertRecord(r, p, g->size, kTextureFields, COUNTOF(kTextureFields)); break;
        case REGION_STRING_TABLE: SirConvertRecord(r, p, g->size, kStringTableFields, COUNTOF(kStringTableFields)); break;
        case REGION_ACTION_TABLE: SirConvertRecord(r, p, g->size, kActionTableFields, COUNTOF(kActionTableFields)); break;
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
    { 0x010, SIR_PTR, 1 },    // data
    { 0x418, SIR_PTR, 1 },    // data
    // 0x820: u8 flag (code: cleared as a byte)
    { 0x824, SIR_PTR, 2 },    // code: texture "Icon-HomeButtonMenu_is_not_available", model "Generic_box"
    { 0x82C, SIR_U32, 1 },    // data: 0x1B
    { 0x830, SIR_PTR, 1 },    // data
    // 0x834: u8 flag (data)
    { 0x838, SIR_U32, 1 },    // data: 0x46
    { 0x83C, SIR_PTR, 1 },    // data
    // 0x840: u8 flag (data)
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
        ImagePlanConvert(&plan, r, image);
        e = GlobalBinaryConvertHeader(r, hdr);
    }
    PlanFree(&plan);
    return e;
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
