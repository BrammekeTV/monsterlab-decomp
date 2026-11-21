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
    SIR_ERR_INPUT,            // input table (header slot 0xDC4): bad count, step pointer, step type or source count
    SIR_ERR_TEXTURE,          // texture list (header slot 0xD94): broken links, bad header or data outside the image
    SIR_ERR_STRINGS,          // string table (header slot 0xDA8): array or a string outside the image
    SIR_ERR_ACTIONS,          // action table (header slot 0x844): array or a record outside the image, or not a program
    SIR_ERR_UI,               // UI root, fonts or the objects of slots 0xDB4-0xDBC: pointer outside the image, font kind
    SIR_ERR_SCREENS,          // screen state machines (slots 0xD98, 0xD9C): bad count, node type or pointer
    SIR_ERR_EFFECTS,          // effect definitions (header slots 0x82C-0x840): table or one of its pointers outside the image
    SIR_ERR_POOL,             // object pool (header slots 0x10, 0x418): bad counts, block or list pointer
    SIR_ERR_LEVEL,            // level file header block: zone slot table not laid out as fn_80129CC8 does
    SIR_ERR_ENTITY,           // entity of a level zone: part list outside the zone, child list that loops
    SIR_ERR_OVERLAP,          // two of the objects above share bytes
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

// ---------------------------------------------------------------------------------------------
// Input table (header slot 0xDC4): the abstract inputs RemoteInputGet (0x80021550) hands out.
// Layout from fn_800214D4, fn_80020334, RemoteInputGet, RemoteInputUpdate and the ten step classes.
// ---------------------------------------------------------------------------------------------
#define INPUT_SEQ_FIXED     8        // sequences at +0x000, initialised separately by fn_800214D4
#define INPUT_SEQ_MAX       100      // sequences at +0x240 (room up to the index at +0x1E60)
#define INPUT_SEQ_STEPS     10
#define INPUT_INDEX_COUNT   34       // RemoteInputGet: ids 0..33 (0x22 returns the table itself)
#define INPUT_TABLE_SEQS    0x240
#define INPUT_TABLE_INDEX   0x1E60   // s32[34]: sequence number for each input id
#define INPUT_TABLE_COUNT   0x1EE8   // s32: sequences in use at +0x240
#define INPUT_TABLE_SIZE    0x1F24

// One input: a sequence of steps that have to happen in order (most have one step).
typedef struct InputSequence {
    s32   stepCount;                 // 0x00
    s32   current;                   // 0x04: step being waited for
    f32   lastValue;                 // 0x08
    f32   value;                     // 0x0C: what RemoteInputShake returns
    u8*   steps[INPUT_SEQ_STEPS];    // 0x10: InputStep*
    f32   time;                      // 0x38
    f32   minTime;                   // 0x3C
    f32   maxTime;                   // 0x40: 0 = no limit
    s32   timing;                    // 0x44
} InputSequence;

// Step classes, by the type word at step +0x48 (fn_80020334 installs the vtable for it at +0x4C).
// Names are the classes' RTTI names in main.dol (CAbstractInput...).
enum {
    INPUT_STEP_BUTTON_HELD = 0,             // 0x50 bytes
    INPUT_STEP_BUTTON_JUST_RELEASED = 1,    // 0x60: f32 previous at +0x50
    INPUT_STEP_BUTTON_JUST_PRESSED = 2,     // 0x60: f32 previous at +0x50
    INPUT_STEP_TWO_BUTTON_TOGGLE = 3,       // 0x50 in the file, although the class uses +0x50..+0x5B
    INPUT_STEP_ANY_BUTTONS_HELD = 4,        // 0x50
    INPUT_STEP_ANY_BUTTONS_RELEASED = 5,    // 0x60: f32 previous[4] at +0x50
    INPUT_STEP_ANY_BUTTONS_JUST_PRESSED = 6,// 0x60: f32 previous[4] at +0x50
    INPUT_STEP_ROTATE_CW = 7,               // 0x60: s32 direction (-1), f32 timer, f32 time, f32 steps
    INPUT_STEP_ROTATE_CCW = 8,              // 0x60: same
    INPUT_STEP_TOGGLE = 9,                  // 0x60: f32 previous at +0x50
    INPUT_STEP_TYPES = 10
};
#define INPUT_STEP_SOURCES 4
// Step, common part: f32 raw +0x00, f32 value +0x04, s32 sourceCount +0x08, sources at +0x0C
// ({u32 id, f32 value, u32 kind}, 12 bytes each, up to 4), s32 +0x3C, f32 min +0x40, f32 max +0x44,
// u32 type +0x48, vtable +0x4C (a build-time address in the file; set at boot).

// ---------------------------------------------------------------------------------------------
// Textures. Header slot 0xD94 points at the start of the image: a word, then the list of every
// texture in the file. Slots 0x824 and 0x828 point at two of them.
// ---------------------------------------------------------------------------------------------
typedef struct SirTexture {
    u32   headerSize;     // 0x00: 0x54 (data)
    u16   size[2];        // 0x04: width, height (code: fn_801FCA38 safe-area test)
    u16   gxSize[2];      // 0x08: width, height written to GX TX_SETIMAGE0 (code: fn_80195A58)
    u8    unk0C[4];       // 0x0C: no reader found; +0x0E is the bits per pixel in every texture
    u8    unk10;
    u8    unk11;          // 0x11: code: non-zero selects the palette path; (unk11 >> 1) goes to the TLUT register
    u8    unk12;
    u8    mipCount;       // 0x13: code: (mipCount - 1) << 4 is the max LOD
    u8    format;         // 0x14: code: GX texture format (6 RGBA8, 9 CI8, 2 IA4)
    u8    tlutFormat;     // 0x15: code
    u8    unk16;          // 0x16: code: written to TX_SETMODE0 << 5
    u8    unk17;          // 0x17: code: an index
    u32   dataSize;       // 0x18: (data) bytes of pixel data
    u8*   data;           // 0x1C: code: pixel data, in GX layout (not converted)
    u8*   palette;        // 0x20: code: big-endian TLUT entries (not converted), or NULL
    u32   unk24;          // 0x24: (data) small count, 2..15
    u8*   next;           // 0x28: (data) list node: points at the next texture's +0x28
    u8*   prev;           // 0x2C: (data)
    u8*   list;           // 0x30: (data) the SirTextureList it is in
    u8    unk34;
    char  name[1];        // 0x35: NUL-terminated, can run past 0x54
} SirTexture;
#define SIR_TEXTURE_HEADER 0x54
#define SIR_TEXTURE_NODE   0x28

typedef struct SirTextureList {   // at image +4 (typed from the data only)
    u8*   head;           // first texture's +0x28
    u8*   tail;
    u32   count;
} SirTextureList;

// ---------------------------------------------------------------------------------------------
// String table (header slot 0xDA8, gStringTable): the string lookups (LookupString in
// src/game/combat_hud.cpp) read strings[id] for id < count.
// ---------------------------------------------------------------------------------------------
typedef struct SirStringTable {
    const char** strings;   // 0x00: code
    u8*   unk04;            // 0x04: (data) points at zeroed words after the pointer array
    u32   buildAddr;        // 0x08: (data) address from the build that baked the file (0x930DDD40)
    u32   count;            // 0x0C: code
    u8*   text;             // 0x10: (data) start of the string bytes
} SirStringTable;

// Action table (header slot 0x844, gGlobalBinaryField844): TriggerDoAction (0x801E0054) runs
// records[index] for index < count. A record is a program of 32-bit words for the interpreter
// fn_801E1BD4: {opcode bits 0-7, length in words bits 8-15, function index or jump distance bits
// 16-27}, then length - 1 operand words; opcode 0xFF ends it. All of it is converted as words.
typedef struct SirActionTable {
    u32   count;            // 0x00: code
    u8**  records;          // 0x04: code
} SirActionTable;

// ---------------------------------------------------------------------------------------------
// UI root (header slot 0xDB0, gScreenSets) and the objects of slots 0xDB4, 0xDB8 and 0xDBC.
// Only the fields a decompiled function reads are converted; the schemas in sir_loader.cpp name
// the function for each and list what is left.
//   root   +0x14 font[3], +0x20 f32[19], +0x6C..+0x90 words used at run time
//   font   +0x04 name, +0xCC font object
//   font object: +0x00 vtable (stored by ScreenSetsInit for the kind), +0x04 kind, +0x08 data;
//          kind 1 also has page textures from +0x24 and their count at +0x124
// The font data behind +0x08 (glyph tables) is not converted yet.
// ---------------------------------------------------------------------------------------------
#define UI_ROOT_SIZE            0x98
#define UI_ROOT_FONTS           3
#define UI_FONT_SIZE            0xD0
#define UI_FONT_KIND_0          0      // class of vtable 0x803821D0 (fn_80196E98), 0xC bytes
#define UI_FONT_KIND_1          1      // class of vtable 0x80382380 (fn_801970D4), 0x128 bytes
#define UI_FONT_OBJ_SIZE_KIND0  0x0C
#define UI_FONT_OBJ_SIZE_KIND1  0x128
#define UI_FONT_PAGES           64     // page pointers at +0x24..+0x123
#define UI_FONT_LIST_SIZE       0x0C   // slot 0xDB4 (lbl_805FB660)
#define UI_DRAW_SIZE            0x408  // slot 0xDB8 (lbl_805FB658)
#define UI_PAD_SIZE             0x20   // slot 0xDBC (lbl_805FB670)

// ---------------------------------------------------------------------------------------------
// Screen state machines (header slot 0xD98: the screen list; slot 0xD9C: the HUD, which holds up
// to 256 of them). Layout from HudUpdateScreens (0x801BB5BC), fn_801BB8A0, fn_801BBB68,
// fn_801BBC9C, fn_801BC5F0 and the five classes fn_801BC5F0 installs.
//
//   machine     0x70 bytes: definition, current / previous / pending state, flags, timer, value table
//   definition  states (0x30 bytes each), node pointers, shared events, key, start node at +0x34
//   state       key, three action words (enter, update, exit), time-out, flags, events
//   event       {id, condition word, transition}; shared event: {id, transition, condition word}
//   node        0x30 bytes, by the type word at +0x08 (below)
//   transition  target state or node, condition word, action word; a shared event's transition
//               also has a list of state keys
// An action or condition word has a type in bits 27-30 (0 = none) and a value in bits 0-17; type
// 3 looks the value up in the machine's value table. The result goes to TriggerDoAction.
// ---------------------------------------------------------------------------------------------
enum {
    FSM_NODE_PLAIN  = 0,   // vtable 0x80383FC4: no fields
    FSM_NODE_STATE  = 1,   // vtable 0x80383F90: a state (in the state array, never in the node array)
    FSM_NODE_START  = 2,   // vtable 0x80383F58: one transition (also the definition's start node)
    FSM_NODE_ACTION = 3,   // vtable 0x80383F20: runs an action word, then one transition
    FSM_NODE_CHOICE = 4    // vtable 0x80383EE8: runs an action word; its result picks one of several transitions
};
#define FSM_USED                    0x58   // of the 0x70 bytes; the rest has no reader
#define FSM_DEF_USED                0x48
#define FSM_STATE_SIZE              0x30
#define FSM_EVENT_SIZE              0x0C
#define FSM_NODE_SIZE               0x30
#define FSM_TRANSITION_USED         0x18   // of 0x20
#define FSM_SHARED_TRANSITION_USED  0x24   // of 0x30
#define HUD_MACHINES                256
#define HUD_SIZE                    0x434

// ---------------------------------------------------------------------------------------------
// Effect definitions (header slots 0x82C-0x840) and object pools (slots 0x10, 0x418).
// fn_80007D68 gives the two definition tables to the managers lbl_8047C790 (0x70-byte entries)
// and lbl_8047C7AC (0x80-byte entries). The schemas in sir_loader.cpp name the reader of each
// field. An entry's pool pointers name memory the game initialises itself (fn_801EB908).
// A pool is the object fn_801EB7A4 builds: {items per block, item size, alignment, in use, free
// list, used list, parent, block list, grow flag, block, allocator}; its block holds the nodes
// {prev, next, item} as fn_801EBAB8 lays them out.
// ---------------------------------------------------------------------------------------------
#define FX_BOLT_SIZE       0x70
#define FX_TRAIL_SIZE      0x80
#define FX_POOL_HEAD_SIZE  0x28   // what fn_801EB908 writes
#define POOL_SIZE          0x2C
#define POOL_MAX_ITEMS     0x10000

// Converts the 4 KB header block (the pointer table the game keeps in sSirHeader).
SirError GlobalBinaryConvertHeader(SirReloc* r, u8* hdr);

// Whole file: converts header and image in place. `hdr` and `image` are the two sections copied
// out of the file (hdr: 0x1000 bytes). Maps the image in `r`, checks the input table, texture list,
// string table, action table, UI objects and screen state machines (each is skipped when its header
// slot is 0), converts the database,
// then those objects, then the header. Nothing is changed when a check fails.
SirError GlobalBinaryConvert(SirReloc* r, u8* hdr, u8* image, u32 imageSize, s32* failTable);

// ---------------------------------------------------------------------------------------------
// Level files (`*_level.sir`): a header block (section type -1) that is the world state, and one
// image per zone (section type = zone number). See docs/loader.md, "Level files".
// ---------------------------------------------------------------------------------------------
#define LEVEL_HEADER_SLOTS    0x21E0   // offset of the zone slots (fn_80129CC8)
#define LEVEL_SLOT_MIN_SIZE   0x148    // a slot is 0x148 + 0x54 * n bytes (gZoneResourceSize)
#define LEVEL_SLOT_MAX_SIZE   0x1000
#define LEVEL_MAX_ZONES       4096
#define LEVEL_HUD_SIZE        0x438

// Wii address the header block was built for: its word +0x1A4 points at its own +0x21E0. 0 if
// the block does not look like a level header.
u32 LevelHeaderWiiBase(const u8* hdr, u32 size);
// Converts the header block in place. `r` must map the header's own range (LevelHeaderWiiBase)
// and the zones' address ranges, so that pointers into both become host pointers.
SirError LevelHeaderConvert(SirReloc* r, u8* hdr, u32 size);
#define ENTITY_USED          0x1B4   // of an entity: up to the last field with a known reader
#define ENTITY_RECORD_SIZE   0x40
#define ENTITY_TYPE_PROP     6       // byte +0x34 of the record: a prop (constructor fn_800690D4)
#define PROP_DEF_USED        0xC0    // of a prop's definition (entity +0x9C): up to the last field with a known reader
#define POOL_TABLE_ENTRY     0x2C

// A pool table entry names the script function the level start runs over the pool's items
// (fn_801DB1AC), by its index in gScriptFuncs. That function says what an item is. The indices
// are those of the retail build's registration order (the six are registered in a row); the
// port checks them against the live table before it converts a zone.
enum {
    LEVEL_FIXUP_ENTITY        = 651,   // FixUpEntityCallback: the item is an entity
    LEVEL_FIXUP_ENTITY_PART   = 652,   // FixUpEntityPartCallback: an entity
    LEVEL_FIXUP_POOLED_ENTITY = 653,   // FixUpPooledEntityCallback: a slot holding an entity
    LEVEL_FIXUP_PROJECTILE    = 654,   // FixUpProjectileEntityCallback: an entity with a part at +0x22C
    LEVEL_FIXUP_RENDER_OBJECT = 655,   // FixUpRenderObjectCallback: a slot holding a render object
    LEVEL_FIXUP_FSM           = 656    // FixupFSMPoolCallback: a slot holding a state machine
};

// What a converted header block names in one zone (Wii addresses).
typedef struct LevelZoneRoots {
    u32  hud;                // header +0x1F4, or 0 when the HUD is in another zone
    u32* entities;           // every slot's +0x50 and the entries of its two entity tables
    u32  entityCount;
    u32* sceneObjects;       // entries of the block's tables +0x1B0 and +0x1BC
    u32  sceneObjectCount;
    u32* poolTables;         // pairs {address, entry count}: every slot's pool table (+0xE4, +0xE8)
    u32  poolTableCount;
} LevelZoneRoots;

// Fills `out` from a converted header block for the zone whose image is at `zoneHost` in memory.
// FALSE if the block's slots cannot be walked. Free with LevelZoneRootsFree.
BOOL LevelZoneRootsCollect(const u8* hdr, u32 hdrSize, const u8* zoneHost, u32 zoneSize, u32 zoneWiiBase, LevelZoneRoots* out);
void LevelZoneRootsFree(LevelZoneRoots* roots);
// Converts what is typed so far of one zone image loaded for Wii address `wiiBase`: the HUD with
// its state machines, the entities (first layer: record, child entities, part lists, state
// machine, scene object), the scene objects and the pool tables with their pools. `r` must map
// the image. Nothing is changed when a check fails.
SirError LevelZoneConvert(SirReloc* r, u8* image, u32 imageSize, u32 wiiBase, const LevelZoneRoots* roots);

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
