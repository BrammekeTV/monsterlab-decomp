// Test tool for the PC port's .sir loader (src/port/sir_loader.cpp).
//
//   sir_loader_test --selftest                 synthetic big-endian file built in memory (runs in CI)
//   sir_loader_test --dump <global_binary.sir> loads the real file and prints every converted value
//                                              in the text format of `tools/sir_decoder.py --gamedb`
//
// tools/loader_check.py builds this, runs both modes and compares the dump with the Python
// reference decoder. The real file is game data: it stays in orig/ and is never committed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/part.h"
#include "port/sir_loader.h"

static int gFailed = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); gFailed++; } } while (0)

// ------------------------------------------------------------------------------------ dump

static u32 Bits(f32 f) {
    u32 v;
    memcpy(&v, &f, 4);
    return v;
}

// Pointer slot as text: "img+0x..." inside the image, "wii:XXXXXXXX" for an unresolved Wii address.
static void PrintPtr(const u8* slot, const GlobalBinaryFile* f) {
    const u8* p;
    memcpy(&p, slot, sizeof(p));
    if (p == NULL) printf("null");
    else if (p >= f->image && p <= f->image + f->imageSize) printf("img+0x%x", (u32)(p - f->image));
    else printf("wii:%08x", *(const u32*)slot);
}

static int Dump(const char* path) {
    SirReloc r;
    GlobalBinaryFile f;
    s32 failTable = -1;
    SirRelocInit(&r, NULL, NULL);
    SirError e = GlobalBinaryLoadFile(path, &r, &f, &failTable);
    if (e != SIR_OK) {
        fprintf(stderr, "%s: %s (table %d)\n", path, SirErrorText(e), failTable);
        return 1;
    }

    // Header pointer slots, read the way the game does: *(void**)(sSirHeader + off).
    static const u32 kPtrSlots[] = { 0x10, 0x418, 0x824, 0x828, 0x830, 0x83C, 0x844, 0xD94, 0xD98, 0xD9C,
                                     0xDA8, 0xDB0, 0xDB4, 0xDB8, 0xDBC, 0xDC0, 0xDC4, 0xDC8 };
    for (u32 off : kPtrSlots) {
        printf("hdrptr 0x%03x ", off);
        PrintPtr(f.hdr + off, &f);
        printf("\n");
    }
    static const u32 kWordSlots[] = { 0x8, 0x82C, 0x838, 0x848, 0x85C, 0x860, 0x864, 0x86C, 0x870, 0x874, 0x878, 0x960, 0xD8C };
    for (u32 off : kWordSlots) printf("hdrword 0x%03x %08x\n", off, *(u32*)(f.hdr + off));
    printf("hdrbyte 0x820 %u\nhdrbyte 0x834 %u\nhdrbyte 0x840 %u\n", f.hdr[0x820], f.hdr[0x834], f.hdr[0x840]);

    GameDb* db = *(GameDb**)(f.hdr + GLOBAL_BINARY_HDR_GAMEDB);
    GameDbTable* tables = (GameDbTable*)db->data;
    printf("gamedb ");
    PrintPtr((const u8*)&db, &f);
    printf(" data ");
    PrintPtr((const u8*)&db->data, &f);
    printf("\n");

    for (s32 t = 0; t < GAMEDB_TABLE_COUNT; t++) {
        const GameDbTable* tb = &tables[t];
        printf("table %d rows ", t);
        PrintPtr((const u8*)&tb->rows, &f);
        printf(" fn ");
        PrintPtr((const u8*)&tb->arrayFn[0], &f);
        printf(" ");
        PrintPtr((const u8*)&tb->arrayFn[1], &f);
        printf(" count %u rowsize %u fields %u present %u", tb->count, tb->rowSize, tb->fieldCount, tb->present);
        if (tb->count != 0) printf(" layout %08x", tb->layoutHash);
        printf("\n");
        if (tb->count == 0) continue;
        for (u32 i = 0; i < tb->fieldCount; i++)
            printf("field %d %u type %u size %u name %08x ref %08x\n", t, i, tb->fieldType[i], tb->fieldSize[i],
                   tb->fieldName[i], tb->fieldRef[i]);
        for (u32 i = 0; i < tb->count; i++) {
            const u8* row = tb->rows + i * tb->rowSize;
            u32 off = 0;
            printf("row %d %u", t, i);
            for (u32 k = 0; k < tb->fieldCount; k++) {
                u32 size = tb->fieldSize[k];
                off = (off + size - 1) & ~(size - 1);
                switch (tb->fieldType[k]) {
                case GAMEDB_T_INT:
                case GAMEDB_T_INT2:  printf(" %d", *(const s32*)(row + off)); break;
                case GAMEDB_T_FLOAT: printf(" f%08x", Bits(*(const f32*)(row + off))); break;
                case GAMEDB_T_HASH:  printf(" h%08x", *(const u32*)(row + off)); break;
                case GAMEDB_T_SHORT: printf(" %d", *(const s16*)(row + off)); break;
                default:             printf(" %u", row[off]); break;
                }
                off += size;
            }
            printf("\n");
        }
    }

    // The same data through the structs and offsets the decompiled code uses (include/game/part.h).
    const u8* data = db->data;
    const PartDef* parts = *(const PartDef**)(data + 0x5B78);
    s32 partCount = *(const s32*)(data + 0x5B7C);
    for (s32 i = 0; i < partCount; i++) {
        const PartDef* p = &parts[i];
        printf("part %d index %d tiers %08x %08x %08x %08x action1 %d slot %d discipline %d action2 %d\n", i, p->index,
               p->tierName[0], p->tierName[1], p->tierName[2], p->tierName[3], p->action1, p->slot, p->discipline, p->action2);
    }
    const PartAction* actions = *(const PartAction**)(data + 0x430);
    s32 actionCount = *(const s32*)(data + 0x434);
    const PartActionType* types = *(const PartActionType**)(data + 0x08);
    for (s32 i = 0; i < actionCount; i++) {
        const PartAction* a = &actions[i];
        printf("action %d key %u type %d name %08x\n", i, a->key, a->type, types[a->type].nameHash);
    }
    // Tuning values: 8-byte rows {s32 id, f32 value}, read as battle_object.cpp does.
    const u8* tuning = *(const u8**)(data + 0x99D0);
    s32 tuningCount = *(const s32*)(data + 0x99D4);
    for (s32 i = 0; i < tuningCount; i++)
        printf("tuning %d id %d value %.9g\n", i, *(const s32*)(tuning + i * 8), (double)*(const f32*)(tuning + i * 8 + 4));

    printf("report image %u dolkept %u dolresolved %u unknown %u\n", r.ptrImage, r.ptrDolKept, r.ptrDolResolved, r.ptrUnknown);
    GlobalBinaryFree(&f);
    return 0;
}

// ------------------------------------------------------------------------------------ self-test

static void Put32(u8* p, u32 v) {
    p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v;
}
static void Put16(u8* p, u16 v) {
    p[0] = (u8)(v >> 8); p[1] = (u8)v;
}

#define T_FN_A   0x80072D74u
#define T_FN_B   0x80072D98u
#define T_VTABLE 0x8049F444u
#define T_DB     0x100u
#define T_DATA   0x200u
#define T_ROWS_A (T_DATA + GAMEDB_TABLE_COUNT * GAMEDB_DESC_SIZE)
#define T_ROWS_B (T_ROWS_A + 0x40)
#define T_SIZE   (T_ROWS_B + 0x40)

static u8 gFakeVtable[4];
static void* TestResolver(u32 addr, void*) {
    return addr == T_VTABLE ? gFakeVtable : NULL;
}

// A miniature global_binary image: the database object, 42 descriptors (two with rows), in the
// byte order and with the fill pattern of the real file.
static void BuildImage(u8* hdr, u8* img) {
    memset(hdr, 0, GLOBAL_BINARY_HDR_SIZE);
    for (u32 o = 0; o < T_SIZE; o += 4) Put32(img + o, 0x7F800001);
    Put32(hdr + GLOBAL_BINARY_HDR_GAMEDB, GLOBAL_BINARY_WII_BASE + T_DB);
    Put32(hdr + 0x874, 0x3F9E0419);                   // 1.2345f
    hdr[0x820] = 1;
    memcpy(hdr + 0x900, "text", 4);                   // not in the schema: must stay as it is

    u8* db = img + T_DB;
    Put32(db + 0x00, T_VTABLE);
    Put32(db + 0x04, T_FN_A);
    Put32(db + 0x08, T_FN_B);
    Put32(db + 0x0C, GLOBAL_BINARY_WII_BASE + T_DATA);
    Put32(db + 0x10, 1);

    for (s32 t = 0; t < GAMEDB_TABLE_COUNT; t++) {
        u8* d = img + T_DATA + t * GAMEDB_DESC_SIZE;
        Put32(d + 0x00, T_FN_A);
        Put32(d + 0x04, T_FN_B);
        Put32(d + 0x08, 0);
        Put32(d + 0x0C, 0);
        Put32(d + 0x10, 0);
        Put32(d + 0x18, 0);
        Put32(d + 0x1C, 0);
        Put32(d + 0x420, 0);
        d[0x424] = 0;
    }
    // Table 3: {s32, hash, f32, s16, u8, flag} = 0x10 bytes, 2 rows.
    static const u32 typesA[] = { GAMEDB_T_INT, GAMEDB_T_HASH, GAMEDB_T_FLOAT, GAMEDB_T_SHORT, GAMEDB_T_BYTE, GAMEDB_T_FLAG };
    static const u32 sizesA[] = { 4, 4, 4, 2, 1, 1 };
    u8* d = img + T_DATA + 3 * GAMEDB_DESC_SIZE;
    Put32(d + 0x08, GLOBAL_BINARY_WII_BASE + T_ROWS_A);
    Put32(d + 0x0C, 2); Put32(d + 0x10, 1); Put32(d + 0x14, 0xEA241361); Put32(d + 0x18, 0x10); Put32(d + 0x1C, 2);
    for (u32 i = 0; i < 6; i++) {
        Put32(d + 0x020 + i * 4, typesA[i]);
        Put32(d + 0x120 + i * 4, sizesA[i]);
        Put32(d + 0x220 + i * 4, 0x11110000 + i);
        Put32(d + 0x320 + i * 4, 0xFFFFFFFF);
    }
    Put32(d + 0x420, 6);
    d[0x424] = 1;
    for (u32 i = 0; i < 2; i++) {
        u8* row = img + T_ROWS_A + i * 0x10;
        Put32(row + 0x0, (u32)(-5 - (s32)i));
        Put32(row + 0x4, 0xB7344B14);
        Put32(row + 0x8, 0x41F00000);   // 30.0f
        Put16(row + 0xC, (u16)(-300));
        row[0xE] = 0x64;
        row[0xF] = 1;
    }
    // Table 7: {s32, u8} = 8 bytes (5 used, 3 bytes of fill), 1 row.
    d = img + T_DATA + 7 * GAMEDB_DESC_SIZE;
    Put32(d + 0x08, GLOBAL_BINARY_WII_BASE + T_ROWS_B);
    Put32(d + 0x0C, 1); Put32(d + 0x10, 1); Put32(d + 0x14, 0x80F68318); Put32(d + 0x18, 8); Put32(d + 0x1C, 1);
    Put32(d + 0x020, GAMEDB_T_INT);  Put32(d + 0x120, 4);
    Put32(d + 0x024, GAMEDB_T_BYTE); Put32(d + 0x124, 1);
    Put32(d + 0x420, 2);
    d[0x424] = 1;
    Put32(img + T_ROWS_B, 0x12345678);
    img[T_ROWS_B + 4] = 0xAB;           // followed by the fill bytes 80 00 01
}

static SirError ConvertBuilt(u8* hdr, u8* img, SirReloc* r, s32* failTable) {
    SirRelocInit(r, TestResolver, NULL);
    return GlobalBinaryConvert(r, hdr, img, T_SIZE, failTable);
}

static int SelfTest(void) {
    u8* hdr = (u8*)malloc(GLOBAL_BINARY_HDR_SIZE);
    u8* img = (u8*)malloc(T_SIZE);
    u8* copy = (u8*)malloc(T_SIZE);
    SirReloc r;
    s32 failTable = 0;

    BuildImage(hdr, img);
    CHECK(ConvertBuilt(hdr, img, &r, &failTable) == SIR_OK);
    CHECK(failTable == -1);

    GameDb* db = *(GameDb**)(hdr + GLOBAL_BINARY_HDR_GAMEDB);
    CHECK((u8*)db == img + T_DB);
    CHECK(*(void**)db == (void*)gFakeVtable);                // main.dol pointer the resolver knows
    CHECK(*(u32*)((u8*)db + 4) == T_FN_A);                   // one it does not know: Wii address kept
    CHECK(db->data == img + T_DATA);
    CHECK(*(f32*)(hdr + 0x874) == 1.2345f);
    CHECK(hdr[0x820] == 1);
    CHECK(memcmp(hdr + 0x900, "text", 4) == 0);

    GameDbTable* tables = (GameDbTable*)db->data;
    CHECK(tables[0].count == 0 && tables[0].rows == NULL && tables[0].present == 0);
    const GameDbTable* a = &tables[3];
    CHECK(a->rows == img + T_ROWS_A && a->count == 2 && a->rowSize == 0x10 && a->fieldCount == 6 && a->present == 1);
    CHECK(a->layoutHash == 0xEA241361 && a->fieldType[2] == GAMEDB_T_FLOAT && a->fieldSize[3] == 2);
    CHECK(a->fieldName[5] == 0x11110005 && a->fieldRef[0] == 0xFFFFFFFF);
    for (s32 i = 0; i < 2; i++) {
        const u8* row = a->rows + i * 0x10;
        CHECK(*(const s32*)(row + 0x0) == -5 - i);
        CHECK(*(const u32*)(row + 0x4) == 0xB7344B14);
        CHECK(*(const f32*)(row + 0x8) == 30.0f);
        CHECK(*(const s16*)(row + 0xC) == -300);
        CHECK(row[0xE] == 0x64 && row[0xF] == 1);
    }
    const GameDbTable* b = &tables[7];
    CHECK(*(const u32*)b->rows == 0x12345678);
    // The byte field and the fill behind it are untouched (a blind word swap would move 0xAB).
    CHECK(b->rows[4] == 0xAB && b->rows[5] == 0x80 && b->rows[6] == 0x00 && b->rows[7] == 0x01);
    CHECK(r.ptrDolResolved == 1 && r.ptrUnknown == 0);
    CHECK(r.ptrDolKept == 2 + 2 * GAMEDB_TABLE_COUNT);

    // A file the loader does not understand is refused before any byte changes.
    struct Bad { u32 off; u32 value; SirError want; s32 table; };
    static const Bad bad[] = {
        { T_DATA + 3 * GAMEDB_DESC_SIZE + 0x18, 0x14, SIR_ERR_GAMEDB_ROWSIZE, 3 },          // row size disagrees
        { T_DATA + 3 * GAMEDB_DESC_SIZE + 0x28, 9, SIR_ERR_GAMEDB_FIELD, 3 },               // unknown type code
        { T_DATA + 3 * GAMEDB_DESC_SIZE + 0x12C, 4, SIR_ERR_GAMEDB_FIELD, 3 },              // s16 with size 4
        { T_DATA + 7 * GAMEDB_DESC_SIZE + 0x08, GLOBAL_BINARY_WII_BASE + T_SIZE - 4, SIR_ERR_BOUNDS, 7 },
        { T_DATA + 7 * GAMEDB_DESC_SIZE + 0x08, GLOBAL_BINARY_WII_BASE + T_ROWS_A + 8, SIR_ERR_GAMEDB_OVERLAP, 3 },
        { T_DATA + 5 * GAMEDB_DESC_SIZE + 0x00, 0x80000000, SIR_ERR_GAMEDB_LAYOUT, 5 },     // not a descriptor
        { T_DATA + 7 * GAMEDB_DESC_SIZE + 0x420, 65, SIR_ERR_GAMEDB_LAYOUT, 7 },            // too many fields
    };
    for (const Bad& c : bad) {
        BuildImage(hdr, img);
        Put32(img + c.off, c.value);
        memcpy(copy, img, T_SIZE);
        SirError e = ConvertBuilt(hdr, img, &r, &failTable);
        CHECK(e == c.want);
        CHECK(failTable == c.table);
        CHECK(memcmp(copy, img, T_SIZE) == 0);
    }

    // Container parsing.
    u8 file[0x1000];
    memset(file, 0, sizeof(file));
    memcpy(file, "PNP0", 4);
    Put32(file + 4, 3);
    Put32(file + 8, 2);
    Put32(file + 0x40, 0xFFFFFFFF); Put32(file + 0x44, 0x800); Put32(file + 0x48, 0x100); Put32(file + 0x54, 0xA92E7101);
    Put32(file + 0x58, 0);          Put32(file + 0x5C, 0x900); Put32(file + 0x60, 0x800);   // runs past the end
    SirSection s;
    CHECK(SirContainerSectionCount(file, sizeof(file)) == 2);
    CHECK(SirContainerSection(file, sizeof(file), 0, &s) && s.type == -1 && s.offset == 0x800 && s.size == 0x100 && s.hash == 0xA92E7101);
    CHECK(!SirContainerSection(file, sizeof(file), 1, &s));
    CHECK(!SirContainerSection(file, sizeof(file), 2, &s));
    file[0] = 'X';
    CHECK(SirContainerSectionCount(file, sizeof(file)) == -1);

    free(hdr);
    free(img);
    free(copy);
    printf(gFailed ? "selftest: %d FAILED\n" : "selftest: ok\n", gFailed);
    return gFailed ? 1 : 0;
}

int main(int argc, char** argv) {
    if (argc == 2 && strcmp(argv[1], "--selftest") == 0) return SelfTest();
    if (argc == 3 && strcmp(argv[1], "--dump") == 0) return Dump(argv[2]);
    fprintf(stderr, "usage: sir_loader_test --selftest | --dump <global_binary.sir>\n");
    return 2;
}
