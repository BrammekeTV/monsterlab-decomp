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
    REGION_ACTION_TABLE, REGION_PTR_ARRAY
};

struct Region {
    u32 off;     // in the image
    u32 size;
    u8  kind;
};

#define TEXTURE_MAX 4096
#define REGION_MAX  ((INPUT_SEQ_FIXED + INPUT_SEQ_MAX) * INPUT_SEQ_STEPS + TEXTURE_MAX + 16)

struct ImagePlan {
    Region regions[REGION_MAX];
    u32    count;
};
static ImagePlan sPlan;   // 70 KB: too big for the stack of a game thread

static bool PlanAdd(ImagePlan* plan, u32 off, u32 size, u8 kind) {
    if (plan->count >= REGION_MAX) return false;
    Region* g = &plan->regions[plan->count++];
    g->off = off;
    g->size = size;
    g->kind = kind;
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

// ---- all of them

static SirError ImagePlanBuild(ImagePlan* plan, const u8* hdr, u8* image, u32 imageSize) {
    plan->count = 0;
    SirError e = SIR_OK;
    u32 addr;
    if ((addr = Be32(hdr + 0xDC4)) != 0) e = InputTablePlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xD94)) != 0) e = TextureListPlan(plan, image, imageSize, addr);
    if (e == SIR_OK && (addr = Be32(hdr + 0xDA8)) != 0)
        e = PtrArrayPlan(plan, image, imageSize, addr, STRING_TABLE_SIZE, 0x0C, 0x00, REGION_STRING_TABLE, SIR_ERR_STRINGS);
    if (e == SIR_OK && (addr = Be32(hdr + 0x844)) != 0)
        e = PtrArrayPlan(plan, image, imageSize, addr, ACTION_TABLE_SIZE, 0x00, 0x04, REGION_ACTION_TABLE, SIR_ERR_ACTIONS);
    if (e != SIR_OK) return e;
    for (u32 a = 0; a < plan->count; a++) {
        const Region* ga = &plan->regions[a];
        for (u32 b = a + 1; b < plan->count; b++) {
            const Region* gb = &plan->regions[b];
            if (ga->off < gb->off + gb->size && gb->off < ga->off + ga->size) return SIR_ERR_OVERLAP;
        }
    }
    return SIR_OK;
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
    SirError e = ImagePlanBuild(&sPlan, hdr, image, imageSize);
    if (e != SIR_OK) return e;
    u32 db = Be32(hdr + GLOBAL_BINARY_HDR_GAMEDB);
    e = GameDbConvert(r, image, imageSize, db, failTable);
    if (e != SIR_OK) return e;
    ImagePlanConvert(&sPlan, r, image);
    return GlobalBinaryConvertHeader(r, hdr);
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
