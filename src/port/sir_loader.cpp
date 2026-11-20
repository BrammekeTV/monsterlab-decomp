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
    case SIR_ERR_IO:             return "cannot read file";
    }
    return "?";
}

// ------------------------------------------------------------------------------------ game database

// GameDb object (header slot 0xDC8): a main.dol pointer, then an array {fn, fn, data, count}
// with one element, the block of table descriptors.
static const SirField kGameDbFields[] = {
    { 0x00, SIR_PTR, 4 },
    { 0x10, SIR_U32, 1 },
};
#define GAMEDB_OBJ_SIZE 0x14

// Table descriptor (GameDbTable). The flag byte at 0x424 needs no conversion.
static const SirField kGameDbTableFields[] = {
    { 0x000, SIR_PTR, 3 },                       // array callbacks, rows
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
        // Every descriptor starts with the same two array callbacks as the database object.
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
    { 0x844, SIR_PTR, 1 },    // code: kept in gGlobalBinaryField844
    { 0x848, SIR_U32, 6 },    // data: 0x3FFFF each
    { 0x860, SIR_U32, 5 },    // data: hash, 0x1AB80, 0, 0x37, 1234
    { 0x874, SIR_F32, 2 },    // data: 1.2345, 0.0123
    { 0x960, SIR_U32, 268 },  // data: -1 up to 0xD8C
    { 0xD94, SIR_PTR, 3 },    // data: image base; code: 0xD98 screen set list
    { 0xDA8, SIR_PTR, 1 },    // data: string table (0x10B2 entries)
    { 0xDB0, SIR_PTR, 7 },    // data 0xDB0-0xDC0; code: 0xDC4, 0xDC8 (GameDb)
};

extern "C" SirError GlobalBinaryConvertHeader(SirReloc* r, u8* hdr) {
    if (!SirConvertRecord(r, hdr, GLOBAL_BINARY_HDR_SIZE, kGlobalBinaryHeaderFields,
                          (s32)(sizeof(kGlobalBinaryHeaderFields) / sizeof(kGlobalBinaryHeaderFields[0]))))
        return SIR_ERR_BOUNDS;
    return SIR_OK;
}

extern "C" SirError GlobalBinaryConvert(SirReloc* r, u8* hdr, u8* image, u32 imageSize, s32* failTable) {
    if (!SirRelocAddRange(r, GLOBAL_BINARY_WII_BASE, imageSize, image)) return SIR_ERR_BOUNDS;
    u32 db = Be32(hdr + GLOBAL_BINARY_HDR_GAMEDB);
    SirError e = GameDbConvert(r, image, imageSize, db, failTable);
    if (e != SIR_OK) return e;
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
