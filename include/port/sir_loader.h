#pragma once
// PC port: converts big-endian Wii `.sir` data in place into the layout the decompiled code reads
// (same offsets, host byte order, 4-byte pointer slots holding host pointers).
// Design and format notes: docs/loader.md.
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SirError {
    SIR_OK = 0,
    SIR_ERR_NOT_SIR,          // bad magic / section table
    SIR_ERR_SECTIONS,         // global_binary.sir must have a 4 KB header section and an image section
    SIR_ERR_BOUNDS,           // a pointer or table leaves the image
    SIR_ERR_GAMEDB_LAYOUT,    // descriptor does not look like one (array callbacks differ, bad counts)
    SIR_ERR_GAMEDB_FIELD,     // unknown field type, or its size disagrees with the type
    SIR_ERR_GAMEDB_ROWSIZE,   // field sizes do not add up to the stored row size
    SIR_ERR_GAMEDB_OVERLAP,   // two tables share row storage
    SIR_ERR_CATALOG,          // catalog arrays do not fit in the file
    SIR_ERR_IO
} SirError;

const char* SirErrorText(SirError e);

// ---------------------------------------------------------------------------------------------
// Container (`PNP0`): header and section table. The table is read from the big-endian file
// bytes; nothing is modified.
// ---------------------------------------------------------------------------------------------
#define SIR_MAGIC          0x504E5030u   // 'PNP0'
#define SIR_SECTION_TABLE  0x40
#define SIR_SECTION_ENTRY  0x18
#define SIR_MAX_SECTIONS   8192

typedef struct SirSection {
    s32 type;      // -1 = header block, 0 = placed memory image, others not yet looked at
    u32 offset;    // file offset (a multiple of 0x800, one disc sector)
    u32 size;
    u32 unk0C;
    u32 unk10;
    u32 hash;
} SirSection;

// Number of sections, or -1 if `file` is not a PNP0 container.
s32  SirContainerSectionCount(const u8* file, u32 fileSize);
// Fills `out` for section `index`; FALSE if the entry is out of range or points outside the file.
BOOL SirContainerSection(const u8* file, u32 fileSize, s32 index, SirSection* out);

// ---------------------------------------------------------------------------------------------
// catalog.sir: the file table SirLoad (0x801E6138) searches. Same container header, but the
// section table lists the sections of every .sir file (0x18 bytes each), followed by one
// 0x40-byte record per file. The game reads the whole file into a buffer and copies both arrays
// out of it (fn_801E54E4), so the buffer is converted in place before that copy.
// ---------------------------------------------------------------------------------------------
typedef struct SirCatalogEntry {
    s32 type;        // 0x00: section type; SirLoad's last argument (-1 header block, 0 image, ...)
    u32 offset;      // 0x04: byte offset in the file (SirLoad reads from sector offset >> 11)
    u32 size;        // 0x08: size in memory
    u32 nameHash;    // 0x0C: StringHash of the file name, e.g. "global_binary.sir"
    u32 packedSize;  // 0x10: size on disc when compressed, else 0
    u32 hash;        // 0x14
} SirCatalogEntry;

typedef struct SirCatalogFile {
    u32  nameHash;   // 0x00
    u32  unk04;      // 0x04
    char name[0x38]; // 0x08: lower-case file name
} SirCatalogFile;

typedef struct SirCatalog {
    SirCatalogEntry* entries;
    u32              entryCount;
    SirCatalogFile*  files;
    u32              fileCount;
} SirCatalog;

// Converts a catalog.sir buffer in place and returns where its two arrays are. The header words
// at +0x04..+0x10 (version, entry count, file count, hash) are converted too; the game reads the
// counts from +0x08 and +0x0C.
SirError SirCatalogConvert(u8* file, u32 fileSize, SirCatalog* out);
// Entry for a file-name hash and section type, as SirLoad searches it; NULL if absent.
const SirCatalogEntry* SirCatalogFind(const SirCatalog* c, u32 nameHash, s32 type);
// Index of the file record with this name hash (the index of its handle in the loader), or -1.
s32 SirCatalogFileIndex(const SirCatalog* c, u32 nameHash);

// ---------------------------------------------------------------------------------------------
// Field schemas: one entry per run of same-typed fields. Anything a schema does not name is left
// untouched (strings, byte arrays, GX data, and fields nobody has typed yet).
// ---------------------------------------------------------------------------------------------
typedef enum SirFieldType {
    SIR_U16 = 1,   // u16 / s16
    SIR_U32 = 2,   // u32 / s32 / hash
    SIR_F32 = 3,
    SIR_PTR = 4    // 32-bit Wii address: byte-swapped, then relocated to a host pointer
} SirFieldType;

typedef struct SirField {
    u32 offset;
    u8  type;      // SirFieldType
    u16 count;     // consecutive fields of this type
} SirField;

// ---------------------------------------------------------------------------------------------
// Relocation: where each Wii address range lives on the host.
// ---------------------------------------------------------------------------------------------
#define SIR_MAX_RANGES 16

typedef struct SirRange {
    u32 wiiBase;
    u32 size;
    u8* host;
} SirRange;

// Maps an address inside main.dol to the PC build's object, or returns NULL if the port has no
// counterpart. No file converted so far needs it: the main.dol-looking words in global_binary.sir
// are not valid addresses of the retail executable (docs/loader.md) and are typed as plain u32.
typedef void* (*SirDolResolver)(u32 wiiAddr, void* user);

typedef struct SirReloc {
    SirRange       ranges[SIR_MAX_RANGES];
    s32            rangeCount;
    SirDolResolver resolveDol;
    void*          resolveUser;
    // Report, filled while converting.
    u32 ptrImage;        // pointers into a mapped range, relocated
    u32 ptrDolResolved;  // main.dol pointers the resolver mapped
    u32 ptrDolKept;      // main.dol pointers left as their Wii address (no counterpart yet)
    u32 ptrUnknown;      // non-null values in no known range, left as they were (after the swap)
    u32 fieldsSwapped;   // scalar fields converted
} SirReloc;

void SirRelocInit(SirReloc* r, SirDolResolver resolveDol, void* user);
BOOL SirRelocAddRange(SirReloc* r, u32 wiiBase, u32 size, u8* host);
// Host pointer for a Wii address inside a mapped range, else NULL.
u8*  SirRelocToHost(const SirReloc* r, u32 wiiAddr);

// Converts the fields `fields[0..n)` of the record at `rec` (recSize bytes). FALSE (and nothing
// changed) if a field lies outside the record.
BOOL SirConvertRecord(SirReloc* r, u8* rec, u32 recSize, const SirField* fields, s32 n);

// ---------------------------------------------------------------------------------------------
// global_binary.sir
// ---------------------------------------------------------------------------------------------
#define GLOBAL_BINARY_WII_BASE   0x90400000u   // where the Wii places section 1
#define GLOBAL_BINARY_HDR_SIZE   0x1000
#define GLOBAL_BINARY_HDR_GAMEDB 0xDC8         // header slot: GameDb*

// Game database: GameDb::data is a block of table descriptors, each followed by nothing; the rows
// live elsewhere in the image. Every descriptor describes its own row layout.
#define GAMEDB_TABLE_COUNT   42
#define GAMEDB_DESC_SIZE     0x428
#define GAMEDB_MAX_FIELDS    64

// Field type codes stored in a table descriptor (names are ours: the code only shows sizes and
// how the values are used).
enum {
    GAMEDB_T_INT   = 3,    // s32: number, row id or reference to another table's row
    GAMEDB_T_FLOAT = 4,    // f32
    GAMEDB_T_HASH  = 5,    // u32 StringHash (text key, asset name)
    GAMEDB_T_SHORT = 6,    // s16
    GAMEDB_T_FLAG  = 7,    // u8, 0/1
    GAMEDB_T_BYTE  = 8,    // u8
    GAMEDB_T_INT2  = 11    // 4 bytes; one field (action table), -1 in every row
};

// Table descriptor as the decompiled code sees it after conversion (0x428 bytes).
typedef struct GameDbTable {
    u32   buildAddr[2];                     // 0x000: addresses from the build that baked the file; not valid in
                                            //        the retail executable (same two values in every table)
    u8*   rows;                             // 0x008
    u32   count;                            // 0x00C
    u32   unk10;                            // 0x010: 1 (0 in the empty table)
    u32   layoutHash;                       // 0x014
    u32   rowSize;                          // 0x018
    u32   count2;                           // 0x01C: same as count
    u32   fieldType[GAMEDB_MAX_FIELDS];     // 0x020: GAMEDB_T_*
    u32   fieldSize[GAMEDB_MAX_FIELDS];     // 0x120: 1, 2 or 4
    u32   fieldName[GAMEDB_MAX_FIELDS];     // 0x220: StringHash of the column name
    u32   fieldRef[GAMEDB_MAX_FIELDS];      // 0x320: StringHash of the table an id refers to, or -1
    u32   fieldCount;                       // 0x420
    u8    present;                          // 0x424: 1, 0 in the empty table
    u8    pad425[3];
} GameDbTable;


// Converts the game database reached from header slot 0xDC8. `image` is section 1 (big-endian,
// as read from disc); `r` must already map GLOBAL_BINARY_WII_BASE to `image`.
// `dbWiiAddr` is the value of the header slot. On error `*failTable` is the table index (-1 for
// the database object itself); the image may then be partly converted and must be discarded.
SirError GameDbConvert(SirReloc* r, u8* image, u32 imageSize, u32 dbWiiAddr, s32* failTable);

// Converts the 4 KB header block (the pointer table the game keeps in sSirHeader).
SirError GlobalBinaryConvertHeader(SirReloc* r, u8* hdr);

// Whole file: converts header and image in place. `hdr` and `image` are the two sections copied
// out of the file (hdr: 0x1000 bytes). Maps the image in `r`, converts the database, then the header.
SirError GlobalBinaryConvert(SirReloc* r, u8* hdr, u8* image, u32 imageSize, s32* failTable);

// Test/tool helper: reads the file with stdio, allocates the two sections with malloc and
// converts them. The game itself goes through SirLoad; see docs/loader.md for the hook.
typedef struct GlobalBinaryFile {
    u8* hdr;         // 0x1000 bytes, converted
    u8* image;       // section 1, converted
    u32 imageSize;
} GlobalBinaryFile;

SirError GlobalBinaryLoadFile(const char* path, SirReloc* r, GlobalBinaryFile* out, s32* failTable);
void     GlobalBinaryFree(GlobalBinaryFile* f);

#ifdef __cplusplus
}
#endif
