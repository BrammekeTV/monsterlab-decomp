// Test tool for the PC port's .sir loader (src/port/sir_loader.cpp).
//
//   sir_loader_test --selftest                 synthetic big-endian file built in memory (runs in CI)
//   sir_loader_test --dump <global_binary.sir> loads the real file and prints every converted value
//                                              in the text format of `tools/sir_decoder.py --gamedb`
//   sir_loader_test --catalog <catalog.sir>    same for the file table (`sir_decoder.py --catalog`)
//
// tools/loader_check.py builds this, runs both modes and compares the dump with the Python
// reference decoder. The real file is game data: it stays in orig/ and is never committed.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <vector>

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

static u32 Fnv(const u8* p, u32 n) {
    u32 h = 0x811C9DC5u;
    for (u32 i = 0; i < n; i++) h = (h ^ p[i]) * 0x01000193u;
    return h;
}

// The hand-typed image objects, read through the structs and offsets the game code uses, in the
// text format of `tools/sir_decoder.py --image`.
static void DumpImage(const GlobalBinaryFile* f) {
    // Input table: fn_800214D4 walks the sequences, RemoteInputGet maps an id through the index.
    u8* table = *(u8**)(f->hdr + 0xDC4);
    s32 seqCount = *(s32*)(table + INPUT_TABLE_COUNT);
    printf("input ");
    PrintPtr((const u8*)&table, f);
    printf(" sequences %d\ninputindex", seqCount);
    for (s32 i = 0; i < INPUT_INDEX_COUNT; i++) printf(" %d", *(s32*)(table + INPUT_TABLE_INDEX + i * 4));
    printf("\ninputtail");
    for (s32 i = 0; i < 6; i++) { printf(" "); PrintPtr(table + 0x1EEC + i * 4, f); }
    printf(" %d %08x %d %d %08x %08x %d\n", *(s32*)(table + 0x1F04), Bits(*(f32*)(table + 0x1F0C)), *(s32*)(table + 0x1F10),
           *(s32*)(table + 0x1F14), Bits(*(f32*)(table + 0x1F18)), Bits(*(f32*)(table + 0x1F1C)), *(s32*)(table + 0x1F20));

    static u8* steps[(INPUT_SEQ_FIXED + INPUT_SEQ_MAX) * INPUT_SEQ_STEPS];
    s32 stepCount = 0;
    for (s32 i = 0; i < INPUT_SEQ_FIXED + seqCount; i++) {
        const InputSequence* q = (const InputSequence*)(i < INPUT_SEQ_FIXED ? table + i * 0x48 : table + INPUT_TABLE_SEQS + (i - INPUT_SEQ_FIXED) * 0x48);
        printf("inputseq %d steps %d current %d value %08x %08x time %08x %08x %08x timing %d ptrs", i, q->stepCount, q->current,
               Bits(q->lastValue), Bits(q->value), Bits(q->time), Bits(q->minTime), Bits(q->maxTime), q->timing);
        for (s32 k = 0; k < INPUT_SEQ_STEPS; k++) { printf(" "); PrintPtr((const u8*)&q->steps[k], f); }
        printf("\n");
        for (s32 k = 0; k < q->stepCount; k++) {
            bool seen = q->steps[k] == NULL;
            for (s32 j = 0; j < stepCount && !seen; j++) seen = steps[j] == q->steps[k];
            if (!seen) steps[stepCount++] = q->steps[k];
        }
    }
    for (s32 i = 0; i < stepCount; i++) {
        const u8* p = steps[i];
        u32 type = *(const u32*)(p + 0x48);
        s32 sources = *(const s32*)(p + 0x08);
        printf("inputstep ");
        PrintPtr((const u8*)&p, f);
        printf(" type %u sources %d raw %08x value %08x mode %d range %08x %08x vtable %08x", type, sources, Bits(*(const f32*)p),
               Bits(*(const f32*)(p + 0x04)), *(const s32*)(p + 0x3C), Bits(*(const f32*)(p + 0x40)), Bits(*(const f32*)(p + 0x44)),
               *(const u32*)(p + 0x4C));
        for (s32 k = 0; k < sources; k++)
            printf(" src %u %08x %u", *(const u32*)(p + 0x0C + k * 0xC), Bits(*(const f32*)(p + 0x10 + k * 0xC)), *(const u32*)(p + 0x14 + k * 0xC));
        switch (type) {
        case INPUT_STEP_BUTTON_JUST_RELEASED:
        case INPUT_STEP_BUTTON_JUST_PRESSED:
        case INPUT_STEP_TOGGLE:
            printf(" %08x", Bits(*(const f32*)(p + 0x50)));
            break;
        case INPUT_STEP_ANY_BUTTONS_RELEASED:
        case INPUT_STEP_ANY_BUTTONS_JUST_PRESSED:
            for (s32 k = 0; k < INPUT_STEP_SOURCES; k++) printf(" %08x", Bits(*(const f32*)(p + 0x50 + k * 4)));
            break;
        case INPUT_STEP_ROTATE_CW:
        case INPUT_STEP_ROTATE_CCW:
            printf(" %d %08x %08x %08x", *(const s32*)(p + 0x50), Bits(*(const f32*)(p + 0x54)), Bits(*(const f32*)(p + 0x58)),
                   Bits(*(const f32*)(p + 0x5C)));
            break;
        }
        printf("\n");
    }

    // Texture list at the start of the image.
    const u8* start = *(const u8**)(f->hdr + 0xD94);
    const SirTextureList* list = (const SirTextureList*)(start + 4);
    printf("texlist ");
    PrintPtr((const u8*)&start, f);
    printf(" word %u head ", *(const u32*)start);
    PrintPtr((const u8*)&list->head, f);
    printf(" tail ");
    PrintPtr((const u8*)&list->tail, f);
    printf(" count %u\n", list->count);
    const u8* node = list->head;
    const u8* slotTex[2] = { *(const u8**)(f->hdr + 0x824), *(const u8**)(f->hdr + 0x828) };
    s32 slotIndex[2] = { -1, -1 };
    for (u32 i = 0; i < list->count; i++) {
        const SirTexture* t = (const SirTexture*)(node - SIR_TEXTURE_NODE);
        for (s32 k = 0; k < 2; k++) if ((const u8*)t == slotTex[k]) slotIndex[k] = (s32)i;
        printf("texture %u ", i);
        PrintPtr((const u8*)&t, f);
        printf(" hdr %u size %ux%u gx %ux%u bytes ", t->headerSize, t->size[0], t->size[1], t->gxSize[0], t->gxSize[1]);
        for (s32 k = 0x0C; k < 0x18; k++) printf("%02x", ((const u8*)t)[k]);
        printf(" datasize %u data ", t->dataSize);
        PrintPtr((const u8*)&t->data, f);
        printf(" palette ");
        PrintPtr((const u8*)&t->palette, f);
        printf(" unk24 %u next ", t->unk24);
        PrintPtr((const u8*)&t->next, f);
        printf(" prev ");
        PrintPtr((const u8*)&t->prev, f);
        printf(" list ");
        PrintPtr((const u8*)&t->list, f);
        printf(" b34 %u name %s datasum %08x\n", t->unk34, t->name, Fnv(t->data, t->dataSize));
        node = t->next;
    }
    for (s32 k = 0; k < 2; k++)
        printf("slottexture 0x%03x %d %s\n", k == 0 ? 0x824 : 0x828, slotIndex[k], ((const SirTexture*)slotTex[k])->name);

    // String table, as the game's string lookups read it.
    const SirStringTable* st = *(const SirStringTable**)(f->hdr + 0xDA8);
    printf("strings ");
    PrintPtr((const u8*)&st, f);
    printf(" array ");
    PrintPtr((const u8*)&st->strings, f);
    printf(" unk04 ");
    PrintPtr((const u8*)&st->unk04, f);
    printf(" build %08x count %u text ", st->buildAddr, st->count);
    PrintPtr((const u8*)&st->text, f);
    printf("\n");
    for (u32 i = 0; i < st->count; i++) {
        printf("string %u ", i);
        PrintPtr((const u8*)&st->strings[i], f);
        printf(" %08x\n", st->strings[i] != NULL ? Fnv((const u8*)st->strings[i], (u32)strlen(st->strings[i])) : 0);
    }

    // Action table, as TriggerDoAction reads it.
    const SirActionTable* at = *(const SirActionTable**)(f->hdr + 0x844);
    printf("actions ");
    PrintPtr((const u8*)&at, f);
    printf(" count %u records ", at->count);
    PrintPtr((const u8*)&at->records, f);
    printf("\n");
    for (u32 i = 0; i < at->count; i++) {
        printf("actionrec %u ", i);
        PrintPtr((const u8*)&at->records[i], f);
        printf("\n");
    }
}

static void PrintWords(const u8* p, u32 n) {
    for (u32 i = 0; i < n; i++) printf("%s%08x", i ? " " : "", *(const u32*)(p + i * 4));
}

static void PrintPtrs(const u8* p, u32 n, const GlobalBinaryFile* f) {
    for (u32 i = 0; i < n; i++) {
        if (i) printf(" ");
        PrintPtr(p + i * 4, f);
    }
}

// Bytes the loader must leave alone, as hex.
static void PrintRaw(const u8* p, u32 n) {
    for (u32 i = 0; i < n; i++) printf("%02x", p[i]);
}

static void AddUnique(std::vector<const u8*>& list, const u8* p) {
    if (p != NULL && std::find(list.begin(), list.end(), p) == list.end()) list.push_back(p);
}

// UI root, fonts, the objects of slots 0xDB4-0xDBC and the screen state machines, read at the
// offsets the game code uses (ScreenSetsInit, fn_8018C9DC, HudReset, HudUpdateScreens, fn_801BB8A0,
// fn_801BC5F0, ...), in the text format of `tools/sir_decoder.py --image`.
static void DumpUi(const GlobalBinaryFile* f) {
    const u8* root = *(const u8**)(f->hdr + 0xDB0);
    printf("uiroot ");
    PrintPtr((const u8*)&root, f);
    printf(" raw ");
    PrintRaw(root, 0x10);
    printf(" w10 %08x fonts ", *(const u32*)(root + 0x10));
    PrintPtrs(root + 0x14, UI_ROOT_FONTS, f);
    printf(" view ");
    PrintWords(root + 0x20, 19);
    printf(" run ");
    PrintWords(root + 0x6C, 5);
    printf(" ");
    PrintWords(root + 0x84, 4);
    printf(" bytes %02x %02x\n", root[0x80], root[0x94]);

    std::vector<const u8*> fontObjs;
    for (u32 i = 0; i < UI_ROOT_FONTS; i++) {
        const u8* font = *(const u8**)(root + 0x14 + i * 4);
        if (font == NULL) continue;
        printf("uifont %u ", i);
        PrintPtr((const u8*)&font, f);
        printf(" name %s obj ", (const char*)font + 4);
        PrintPtr(font + 0xCC, f);
        printf("\n");
        AddUnique(fontObjs, *(const u8**)(font + 0xCC));
    }
    const u8* fontList = *(const u8**)(f->hdr + 0xDB4);
    printf("uifontlist ");
    PrintPtr((const u8*)&fontList, f);
    printf(" ");
    PrintPtrs(fontList + 4, 2, f);
    printf("\n");
    AddUnique(fontObjs, *(const u8**)(fontList + 4));
    AddUnique(fontObjs, *(const u8**)(fontList + 8));
    for (const u8* obj : fontObjs) {
        printf("uifontobj ");
        PrintPtr((const u8*)&obj, f);
        printf(" vtable %08x kind %u data ", *(const u32*)obj, *(const u32*)(obj + 4));
        PrintPtr(obj + 8, f);
        if (*(const u32*)(obj + 4) == UI_FONT_KIND_1) {
            u32 pages = *(const u32*)(obj + 0x124);
            printf(" pages %u", pages);
            for (u32 i = 0; i < pages; i++) { printf(" "); PrintPtr(obj + 0x24 + i * 4, f); }
        }
        printf("\n");
    }
    const u8* draw = *(const u8**)(f->hdr + 0xDB8);
    printf("uidraw ");
    PrintPtr((const u8*)&draw, f);
    printf(" raw ");
    PrintRaw(draw, 4);
    printf(" w04 %08x fonts ", *(const u32*)(draw + 4));
    PrintPtr(draw + 8, f);
    printf(" raw0c ");
    PrintRaw(draw + 0xC, 4);
    printf(" w40 %d w404 %d\n", *(const s32*)(draw + 0x40), *(const s32*)(draw + 0x404));
    const u8* pad = *(const u8**)(f->hdr + 0xDBC);
    printf("uipad ");
    PrintPtr((const u8*)&pad, f);
    printf(" raw ");
    PrintRaw(pad, 4);
    printf(" ");
    PrintWords(pad + 4, 7);
    printf("\n");

    const u8* hud = *(const u8**)(f->hdr + 0xD9C);
    u32 count = *(const u32*)(hud + 0x428);
    u32 spare = 0;
    for (u32 i = count; i < HUD_MACHINES; i++) spare += *(const u8**)(hud + 0x28 + i * 4) != NULL;
    printf("hud ");
    PrintPtr((const u8*)&hud, f);
    printf(" depth %d stack ", *(const s32*)hud);
    PrintPtrs(hud + 4, 8, f);
    printf(" w24 %08x count %u current ", *(const u32*)(hud + 0x24), count);
    PrintPtr(hud + 0x42C, f);
    printf(" flag %u spare %u\n", hud[0x430], spare);

    std::vector<const u8*> machines, defs, trans, sharedTrans;
    AddUnique(machines, *(const u8**)(f->hdr + 0xD98));
    for (u32 i = 0; i < count; i++) {
        printf("hudmachine %u ", i);
        PrintPtr(hud + 0x28 + i * 4, f);
        printf("\n");
        AddUnique(machines, *(const u8**)(hud + 0x28 + i * 4));
    }
    for (const u8* m : machines) {
        printf("fsm ");
        PrintPtr((const u8*)&m, f);
        printf(" def ");
        PrintPtr(m, f);
        printf(" state %d %d %d owner ", *(const s32*)(m + 4), *(const s32*)(m + 8), *(const s32*)(m + 0xC));
        PrintPtr(m + 0x10, f);
        printf(" words ");
        PrintWords(m + 0x14, 6);
        printf(" floats ");
        for (u32 i = 0; i < 10; i++) printf("%s%08x", i ? " " : "", Bits(*(const f32*)(m + 0x2C + i * 4)));
        printf(" values ");
        PrintPtr(m + 0x54, f);
        printf(" raw ");
        PrintRaw(m + 0x58, 0x18);
        printf("\n");
        const u32* values = *(const u32**)(m + 0x54);
        if (values != NULL) {   // as fn_801BBE40 reads it: count, then the values
            u32 h = 0x811C9DC5u;
            for (u32 i = 1; i <= values[0]; i++)
                for (s32 shift = 24; shift >= 0; shift -= 8) h = (h ^ ((values[i] >> shift) & 0xFF)) * 0x01000193u;
            printf("fsmvalues ");
            PrintPtr((const u8*)&values, f);
            printf(" count %u sum %08x\n", values[0], h);
        }
        AddUnique(defs, *(const u8**)m);
    }
    for (const u8* d : defs) {
        s32 states = *(const s32*)(d + 4), nodes = *(const s32*)(d + 0x1C), shared = *(const s32*)(d + 0x2C);
        printf("fsmdef ");
        PrintPtr((const u8*)&d, f);
        printf(" states ");
        PrintPtr(d, f);
        printf(" %d strings ", states);
        PrintPtr(d + 0xC, f);
        printf(" nodes ");
        PrintPtr(d + 0x10, f);
        printf(" %d shared ", nodes);
        PrintPtr(d + 0x20, f);
        printf(" %d key %08x start %08x %08x %u ", shared, *(const u32*)(d + 0x30), *(const u32*)(d + 0x34), *(const u32*)(d + 0x38),
               *(const u32*)(d + 0x3C));
        PrintPtr(d + 0x40, f);
        printf(" b44 %u raw ", d[0x44]);
        PrintRaw(d + 0x08, 4);
        printf(" ");
        PrintRaw(d + 0x14, 8);
        printf(" ");
        PrintRaw(d + 0x24, 8);
        printf("\n");
        trans.push_back(*(const u8**)(d + 0x40));
        for (s32 i = 0; i < states; i++) {
            const u8* s = *(const u8**)d + i * FSM_STATE_SIZE;
            s32 events = *(const s32*)(s + 0x2C);
            printf("fsmstate ");
            PrintPtr((const u8*)&d, f);
            printf(" %d vtable %08x key %08x type %u actions ", i, *(const u32*)s, *(const u32*)(s + 4), *(const u32*)(s + 8));
            PrintWords(s + 0xC, 3);
            printf(" timeout %08x flags %08x events ", Bits(*(const f32*)(s + 0x18)), *(const u32*)(s + 0x1C));
            PrintPtr(s + 0x20, f);
            printf(" %d raw ", events);
            PrintRaw(s + 0x24, 8);
            printf("\n");
            for (s32 k = 0; k < events; k++) {
                const u8* e = *(const u8**)(s + 0x20) + k * FSM_EVENT_SIZE;
                printf("fsmevent ");
                PrintPtr((const u8*)&d, f);
                printf(" %d %d id %08x cond %08x to ", i, k, *(const u32*)e, *(const u32*)(e + 4));
                PrintPtr(e + 8, f);
                printf("\n");
                trans.push_back(*(const u8**)(e + 8));
            }
        }
        for (s32 i = 0; i < nodes; i++) {
            const u8* n = (*(const u8***)(d + 0x10))[i];
            u32 type = *(const u32*)(n + 8);
            printf("fsmnode ");
            PrintPtr((const u8*)&n, f);
            printf(" vtable %08x key %08x type %u", *(const u32*)n, *(const u32*)(n + 4), type);
            if (type == FSM_NODE_START) {
                printf(" to ");
                PrintPtr(n + 0xC, f);
                trans.push_back(*(const u8**)(n + 0xC));
            } else if (type == FSM_NODE_ACTION) {
                printf(" action %08x to ", *(const u32*)(n + 0xC));
                PrintPtr(n + 0x10, f);
                printf(" %d %d", *(const s32*)(n + 0x14), *(const s32*)(n + 0x18));
                trans.push_back(*(const u8**)(n + 0x10));
            } else if (type == FSM_NODE_CHOICE) {
                s32 choices = *(const s32*)(n + 0x14);
                printf(" action %08x choices ", *(const u32*)(n + 0xC));
                PrintPtr(n + 0x10, f);
                printf(" %d result %d %d to", choices, *(const s32*)(n + 0x1C), *(const s32*)(n + 0x20));
                for (s32 k = 0; k < choices; k++) {
                    const u8* slot = *(const u8**)(n + 0x10) + k * 4;
                    printf(" ");
                    PrintPtr(slot, f);
                    trans.push_back(*(const u8**)slot);
                }
            }
            printf("\n");
        }
        for (s32 i = 0; i < shared; i++) {
            const u8* e = *(const u8**)(d + 0x20) + i * FSM_EVENT_SIZE;
            printf("fsmshared ");
            PrintPtr((const u8*)&d, f);
            printf(" %d id %08x to ", i, *(const u32*)e);
            PrintPtr(e + 4, f);
            printf(" cond %08x\n", *(const u32*)(e + 8));
            sharedTrans.push_back(*(const u8**)(e + 4));
        }
    }
    for (std::vector<const u8*>* list : { &trans, &sharedTrans }) {
        std::sort(list->begin(), list->end());
        list->erase(std::unique(list->begin(), list->end()), list->end());
        if (!list->empty() && list->front() == NULL) list->erase(list->begin());
    }
    for (const u8* t : trans) {
        printf("fsmtrans ");
        PrintPtr((const u8*)&t, f);
        printf(" vtable %08x target ", *(const u32*)t);
        PrintPtr(t + 4, f);
        printf(" cond %08x action %08x %d %d raw ", *(const u32*)(t + 8), *(const u32*)(t + 0xC), *(const s32*)(t + 0x10), *(const s32*)(t + 0x14));
        PrintRaw(t + 0x18, 8);
        printf("\n");
    }
    for (const u8* t : sharedTrans) {
        s32 keys = *(const s32*)(t + 0x20);
        printf("fsmsharedtrans ");
        PrintPtr((const u8*)&t, f);
        printf(" vtable %08x target ", *(const u32*)t);
        PrintPtr(t + 4, f);
        printf(" cond %08x action %08x %d %d flags %08x keys ", *(const u32*)(t + 8), *(const u32*)(t + 0xC), *(const s32*)(t + 0x10),
               *(const s32*)(t + 0x14), *(const u32*)(t + 0x18));
        PrintPtr(t + 0x1C, f);
        printf(" %d", keys);
        for (s32 k = 0; k < keys; k++) printf(" %08x", (*(const u32**)(t + 0x1C))[k]);
        printf("\n");
    }
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
        printf(" build %08x %08x count %u rowsize %u fields %u present %u", tb->buildAddr[0], tb->buildAddr[1], tb->count, tb->rowSize, tb->fieldCount, tb->present);
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

    DumpImage(&f);
    DumpUi(&f);
    printf("report image %u dolkept %u dolresolved %u unknown %u\n", r.ptrImage, r.ptrDolKept, r.ptrDolResolved, r.ptrUnknown);
    GlobalBinaryFree(&f);
    return 0;
}

// ------------------------------------------------------------------------------------ catalog

// StringHash: CRC-32 of the upper-cased name (the game's fn_80143A5C).
static u32 NameHash(const char* s) {
    u32 crc = 0xFFFFFFFFu;
    for (; *s != 0; s++) {
        u8 c = (u8)*s;
        if (c >= 'a' && c <= 'z') c -= 0x20;
        crc ^= c;
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
    }
    return ~crc;
}

static int DumpCatalog(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "%s: cannot open\n", path);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    u8* buf = (u8*)malloc((size_t)len);
    size_t got = fread(buf, 1, (size_t)len, fp);
    fclose(fp);
    SirCatalog c;
    SirError e = got == (size_t)len ? SirCatalogConvert(buf, (u32)len, &c) : SIR_ERR_IO;
    if (e != SIR_OK) {
        fprintf(stderr, "%s: %s\n", path, SirErrorText(e));
        return 1;
    }
    // Header words as the game reads them from the buffer.
    printf("catalog version %u entries %u files %u hash %08x\n", *(u32*)(buf + 4), *(u32*)(buf + 8), *(u32*)(buf + 0xC),
           *(u32*)(buf + 0x10));
    for (u32 i = 0; i < c.entryCount; i++) {
        const SirCatalogEntry* en = &c.entries[i];
        printf("entry %u type %d offset %u size %u name %08x packed %u hash %08x\n", i, en->type, en->offset, en->size,
               en->nameHash, en->packedSize, en->hash);
    }
    u32 named = 0;
    for (u32 i = 0; i < c.fileCount; i++) {
        const SirCatalogFile* f = &c.files[i];
        printf("file %u name %08x unk %08x %.56s\n", i, f->nameHash, f->unk04, f->name);
        if (NameHash(f->name) == f->nameHash) named++;
    }
    printf("namehash %u of %u\n", named, c.fileCount);
    // The two lookups GameStart makes, done the way SirLoad does them.
    static const s32 kTypes[] = { -1, 0 };
    for (s32 type : kTypes) {
        u32 h = NameHash("global_binary.sir");
        const SirCatalogEntry* en = SirCatalogFind(&c, h, type);
        if (en == NULL) printf("lookup global_binary.sir %d missing\n", type);
        else printf("lookup global_binary.sir %d file %d sector %u size %u packed %u\n", type, SirCatalogFileIndex(&c, h),
                    en->offset >> 11, en->size, en->packedSize);
    }
    free(buf);
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
// Hand-typed objects behind the database.
#define T_INPUT   (T_ROWS_B + 0x40)          // input table, 0x1F24 bytes
#define T_STEP_A  (T_INPUT + 0x1F40)         // button held (0x50), used by two sequences
#define T_STEP_B  (T_STEP_A + 0x50)          // rotate clockwise (0x60)
#define T_STEP_C  (T_STEP_B + 0x60)          // two-button toggle (0x50): the next step starts where its state would be
#define T_STEP_D  (T_STEP_C + 0x50)          // button just pressed (0x60)
#define T_TEXLIST (T_STEP_D + 0x60)          // word + list, 0x10 bytes
#define T_TEX0    (T_TEXLIST + 0x10)         // header and name 0x60, pixels 0x10
#define T_TEX1    (T_TEX0 + 0x70)            // header and name 0x60, pixels 0x10, palette 0x10
#define T_STR     (T_TEX1 + 0x80)            // string table 0x20, array 0x10, text 0x10
#define T_ACT     (T_STR + 0x40)             // action table 0x08, array 0x08
// UI objects and screen state machines.
#define T_UI_ROOT   (T_ACT + 0x10)           // 0xA0
#define T_FONT      (T_UI_ROOT + 0xA0)       // font 0xD0, then its object
#define T_FONT_OBJ  (T_FONT + 0xD0)          // kind 1, 0x128 bytes
#define T_FONT_DATA (T_FONT + 0x200)         // 0x10
#define T_FONTLIST  (T_FONT_DATA + 0x10)     // 0x10
#define T_FONT_OBJ0 (T_FONTLIST + 0x10)      // kind 0, 0x0C bytes
#define T_DRAW      (T_FONT_OBJ0 + 0x10)     // 0x408
#define T_PAD       (T_DRAW + 0x410)         // 0x20
#define T_FSM0      (T_PAD + 0x20)           // 0x70: the screen list, also the HUD's first machine
#define T_FSM1      (T_FSM0 + 0x70)          // 0x70
#define T_VALUES    (T_FSM1 + 0x70)          // count and two values
#define T_HUD       (T_VALUES + 0x10)        // 0x434
#define T_DEF       (T_HUD + 0x440)          // 0x80
#define T_STATES    (T_DEF + 0x80)           // two states
#define T_EVENTS    (T_STATES + 0x60)        // two events of state 0
#define T_NODEPTRS  (T_EVENTS + 0x20)        // four node pointers
#define T_NODES     (T_NODEPTRS + 0x10)      // nodes of type 0, 2, 3 and 4
#define T_CHOICES   (T_NODES + 0xC0)         // two transition pointers of the type 4 node
#define T_SHARED    (T_CHOICES + 0x10)       // one shared event
#define T_TRANS_A   (T_SHARED + 0x10)        // 0x20
#define T_TRANS_B   (T_TRANS_A + 0x20)       // 0x20
#define T_STRANS    (T_TRANS_B + 0x20)       // 0x30: transition of the shared event
#define T_KEYS      (T_STRANS + 0x30)        // two state keys
#define T_SIZE      (T_KEYS + 0x10)
#define T_ADDR(off) (GLOBAL_BINARY_WII_BASE + (off))

#define T_OBJECT_PTRS 68u   // image pointers in the hand-typed objects and their header slots

static u8 gFakeVtable[4];
static void* TestResolver(u32 addr, void*) {
    return addr == T_VTABLE ? gFakeVtable : NULL;
}

static void PutStep(u8* s, u32 type, u32 sources) {
    Put32(s + 0x00, 0);
    Put32(s + 0x04, 0);
    Put32(s + 0x08, sources);
    for (u32 i = 0; i < sources; i++) {
        Put32(s + 0x0C + i * 0xC, 0x0E + i);    // id; the value behind it keeps the fill word
        Put32(s + 0x14 + i * 0xC, 0x17 + i);    // kind
    }
    Put32(s + 0x3C, 1);
    Put32(s + 0x40, 0);
    Put32(s + 0x44, 0x3F800000);                // 1.0f
    Put32(s + 0x48, type);
    Put32(s + 0x4C, T_VTABLE);
}

static void PutTexture(u8* img, u32 off, u32 prevNode, u32 nextNode, u32 palette, const char* name) {
    u8* t = img + off;
    Put32(t + 0x00, SIR_TEXTURE_HEADER);
    Put16(t + 0x04, 56); Put16(t + 0x06, 40); Put16(t + 0x08, 64); Put16(t + 0x0A, 48);
    static const u8 bytes[12] = { 0x00, 0x05, 0x08, 0x01, 0x04, 0x20, 0x10, 0x01, 0x09, 0x00, 0x00, 0x02 };
    memcpy(t + 0x0C, bytes, 12);
    Put32(t + 0x18, 0x10);
    Put32(t + 0x1C, T_ADDR(off + 0x60));
    Put32(t + 0x20, palette);
    Put32(t + 0x24, 2);
    Put32(t + 0x28, nextNode);
    Put32(t + 0x2C, prevNode);
    Put32(t + 0x30, T_ADDR(T_TEXLIST + 4));
    t[0x34] = 2;
    strcpy((char*)t + 0x35, name);
    for (u32 i = 0; i < 0x10; i++) t[0x60 + i] = (u8)(0xA0 + i);   // pixels: GX layout, must not change
}

// Input table, texture list, string table and action table of the miniature image.
static void BuildObjects(u8* hdr, u8* img) {
    Put32(hdr + 0xDC4, T_ADDR(T_INPUT));
    Put32(hdr + 0xD94, T_ADDR(T_TEXLIST));
    Put32(hdr + 0x824, T_ADDR(T_TEX0));
    Put32(hdr + 0x828, T_ADDR(T_TEX1));
    Put32(hdr + 0xDA8, T_ADDR(T_STR));
    Put32(hdr + 0x844, T_ADDR(T_ACT));

    u8* table = img + T_INPUT;
    memset(table, 0, INPUT_TABLE_SIZE);
    Put32(table + 0x00, 1);                                    // fixed sequence 0: step A
    Put32(table + 0x10, T_ADDR(T_STEP_A));
    u8* seq0 = table + INPUT_TABLE_SEQS;                       // sequence 0: B then C, 0.15 s limit
    Put32(seq0 + 0x00, 2);
    Put32(seq0 + 0x10, T_ADDR(T_STEP_B));
    Put32(seq0 + 0x14, T_ADDR(T_STEP_C));
    Put32(seq0 + 0x40, 0x3E19999A);
    u8* seq1 = seq0 + 0x48;                                    // sequence 1: D then A again
    Put32(seq1 + 0x00, 2);
    Put32(seq1 + 0x10, T_ADDR(T_STEP_D));
    Put32(seq1 + 0x14, T_ADDR(T_STEP_A));
    Put32(table + INPUT_TABLE_INDEX + 5 * 4, 1);               // input id 5 is sequence 1
    Put32(table + INPUT_TABLE_COUNT, 2);
    Put32(table + 0x1EEC, T_ADDR(T_INPUT + INPUT_TABLE_SEQS));
    Put32(table + 0x1F18, 0x43A00000);                         // 320.0f
    PutStep(img + T_STEP_A, INPUT_STEP_BUTTON_HELD, 1);
    PutStep(img + T_STEP_B, INPUT_STEP_ROTATE_CW, 4);
    Put32(img + T_STEP_B + 0x50, 0xFFFFFFFF);
    Put32(img + T_STEP_B + 0x54, 0); Put32(img + T_STEP_B + 0x58, 0); Put32(img + T_STEP_B + 0x5C, 0x40400000);
    PutStep(img + T_STEP_C, INPUT_STEP_TWO_BUTTON_TOGGLE, 2);
    PutStep(img + T_STEP_D, INPUT_STEP_BUTTON_JUST_PRESSED, 1);
    Put32(img + T_STEP_D + 0x00, 0x40000000);                  // 2.0f, at the address of C's +0x50
    Put32(img + T_STEP_D + 0x50, 0x40800000);                  // 4.0f

    Put32(img + T_TEXLIST + 0x00, 0x42);
    Put32(img + T_TEXLIST + 0x04, T_ADDR(T_TEX0 + SIR_TEXTURE_NODE));
    Put32(img + T_TEXLIST + 0x08, T_ADDR(T_TEX1 + SIR_TEXTURE_NODE));
    Put32(img + T_TEXLIST + 0x0C, 2);
    PutTexture(img, T_TEX0, 0, T_ADDR(T_TEX1 + SIR_TEXTURE_NODE), 0, "Icon-One");
    PutTexture(img, T_TEX1, T_ADDR(T_TEX0 + SIR_TEXTURE_NODE), 0, T_ADDR(T_TEX1 + 0x70), "Generic_two");
    Put16(img + T_TEX1 + 0x70, 0x8123);                        // palette entry: big-endian for GX, must not change

    u8* st = img + T_STR;
    Put32(st + 0x00, T_ADDR(T_STR + 0x20));
    Put32(st + 0x04, 0);
    Put32(st + 0x08, 0x930DDD40);
    Put32(st + 0x0C, 3);
    Put32(st + 0x10, T_ADDR(T_STR + 0x30));
    Put32(st + 0x20, T_ADDR(T_STR + 0x31));
    Put32(st + 0x24, 0);
    Put32(st + 0x28, T_ADDR(T_STR + 0x35));
    memcpy(st + 0x30, "\0abc\0Wxyz\0", 11);

    u8* at = img + T_ACT;
    Put32(at + 0x00, 2);
    Put32(at + 0x04, T_ADDR(T_ACT + 0x08));
    Put32(at + 0x08, T_ADDR(T_ROWS_B + 0x20));
    Put32(at + 0x0C, T_ADDR(T_ROWS_B + 0x30));
}

// UI root, one font, the objects of slots 0xDB4-0xDBC, and two state machines sharing one
// definition (two states, one node of each type, one shared event) in the HUD.
static void BuildUi(u8* hdr, u8* img) {
    Put32(hdr + 0xDB0, T_ADDR(T_UI_ROOT));
    Put32(hdr + 0xDB4, T_ADDR(T_FONTLIST));
    Put32(hdr + 0xDB8, T_ADDR(T_DRAW));
    Put32(hdr + 0xDBC, T_ADDR(T_PAD));
    Put32(hdr + 0xD98, T_ADDR(T_FSM0));
    Put32(hdr + 0xD9C, T_ADDR(T_HUD));

    u8* root = img + T_UI_ROOT;
    Put32(root + 0x00, 1);
    Put32(root + 0x04, T_ADDR(T_DRAW));                       // no reader: stays big-endian
    Put32(root + 0x14, T_ADDR(T_FONT));
    Put32(root + 0x18, 0);
    Put32(root + 0x1C, 0);
    for (u32 o = 0x20; o < 0x6C; o += 4) Put32(root + o, 0);
    Put32(root + 0x38, 0x3F800000);
    for (u32 o = 0x70; o < 0x94; o += 4) if (o != 0x80) Put32(root + o, 0);

    u8* font = img + T_FONT;
    Put32(font + 0x00, 1);
    strcpy((char*)font + 4, "test_rf");
    Put32(font + 0xCC, T_ADDR(T_FONT_OBJ));
    u8* obj = img + T_FONT_OBJ;
    Put32(obj + 0x00, T_VTABLE);
    Put32(obj + 0x04, UI_FONT_KIND_1);
    Put32(obj + 0x08, T_ADDR(T_FONT_DATA));
    Put32(obj + 0x24, T_ADDR(T_TEX0));
    Put32(obj + 0x28, T_ADDR(T_TEX1));
    Put32(obj + 0x124, 2);
    Put32(img + T_FONT_DATA, 0x01020304);                     // font data: not converted yet

    u8* list = img + T_FONTLIST;
    Put32(list + 0x00, 1);
    Put32(list + 0x04, T_ADDR(T_FONT_OBJ0));
    Put32(list + 0x08, T_ADDR(T_FONT_OBJ));                   // the font's object again
    Put32(img + T_FONT_OBJ0 + 0x00, T_VTABLE);
    Put32(img + T_FONT_OBJ0 + 0x04, UI_FONT_KIND_0);
    Put32(img + T_FONT_OBJ0 + 0x08, T_ADDR(T_FONT_DATA));

    u8* draw = img + T_DRAW;
    Put32(draw + 0x00, 1);
    Put32(draw + 0x04, T_VTABLE);
    Put32(draw + 0x08, T_ADDR(T_FONTLIST));
    Put32(draw + 0x0C, T_ADDR(T_TEX0));                       // no reader: stays big-endian
    Put32(draw + 0x40, 7);
    Put32(draw + 0x404, 9);

    u8* pad = img + T_PAD;
    Put32(pad + 0x00, 1);
    Put32(pad + 0x04, 2); Put32(pad + 0x08, 2);
    Put32(pad + 0x18, 0x3F000000); Put32(pad + 0x1C, 0);

    for (u32 k = 0; k < 2; k++) {
        u8* m = img + (k == 0 ? T_FSM0 : T_FSM1);
        Put32(m + 0x00, T_ADDR(T_DEF));
        Put32(m + 0x04, 0xFFFFFFFF); Put32(m + 0x08, 0xFFFFFFFF); Put32(m + 0x0C, 0xFFFFFFFF);
        for (u32 o = 0x10; o < 0x50; o += 4) Put32(m + o, 0);
        Put32(m + 0x4C, 0x40A00000);                          // timer 5.0f; +0x50 keeps the fill word
        Put32(m + 0x54, k == 0 ? T_ADDR(T_VALUES) : 0);
        Put32(m + 0x58, 0); Put32(m + 0x5C, 0); Put32(m + 0x60, 0xFFFFFFFE);   // no reader: stay big-endian
    }
    Put32(img + T_VALUES + 0x00, 2);
    Put32(img + T_VALUES + 0x04, 0x17FC0002);
    Put32(img + T_VALUES + 0x08, 0x17FC0004);

    u8* hud = img + T_HUD;
    memset(hud, 0, HUD_SIZE);
    Put32(hud + 0x24, T_VTABLE);
    Put32(hud + 0x28, T_ADDR(T_FSM0));
    Put32(hud + 0x2C, T_ADDR(T_FSM1));
    Put32(hud + 0x428, 2);

    u8* def = img + T_DEF;
    Put32(def + 0x00, T_ADDR(T_STATES)); Put32(def + 0x04, 2); Put32(def + 0x08, 2); Put32(def + 0x0C, 0);
    Put32(def + 0x10, T_ADDR(T_NODEPTRS)); Put32(def + 0x14, 4); Put32(def + 0x18, 2); Put32(def + 0x1C, 4);
    Put32(def + 0x20, T_ADDR(T_SHARED)); Put32(def + 0x24, 1); Put32(def + 0x28, 2); Put32(def + 0x2C, 1);
    Put32(def + 0x30, 0x10CD5CCF);
    Put32(def + 0x34, T_VTABLE); Put32(def + 0x38, 0x8162E3B5); Put32(def + 0x3C, FSM_NODE_START);
    Put32(def + 0x40, T_ADDR(T_TRANS_A));
    def[0x44] = 1;

    u8* s0 = img + T_STATES;
    Put32(s0 + 0x00, T_VTABLE); Put32(s0 + 0x04, 0x3CDA301E); Put32(s0 + 0x08, FSM_NODE_STATE);
    Put32(s0 + 0x0C, 0x18000000); Put32(s0 + 0x10, 0x18000001); Put32(s0 + 0x14, 0x0003FFFF);
    Put32(s0 + 0x18, 0x42780000);                             // 62.0f
    Put32(s0 + 0x1C, 1);
    Put32(s0 + 0x20, T_ADDR(T_EVENTS)); Put32(s0 + 0x24, 2); Put32(s0 + 0x28, 2); Put32(s0 + 0x2C, 2);
    u8* s1 = s0 + FSM_STATE_SIZE;
    Put32(s1 + 0x00, T_VTABLE); Put32(s1 + 0x04, 0xC80B2CFC); Put32(s1 + 0x08, FSM_NODE_STATE);
    Put32(s1 + 0x0C, 0x0003FFFF); Put32(s1 + 0x10, 0x0003FFFF); Put32(s1 + 0x14, 0x0003FFFF);
    Put32(s1 + 0x18, 0xBF800000); Put32(s1 + 0x1C, 0);
    Put32(s1 + 0x20, 0); Put32(s1 + 0x24, 0); Put32(s1 + 0x28, 2); Put32(s1 + 0x2C, 0);
    u8* ev = img + T_EVENTS;
    Put32(ev + 0x00, 0xBE048B88); Put32(ev + 0x04, 0x0003FFFF); Put32(ev + 0x08, T_ADDR(T_TRANS_A));
    Put32(ev + 0x0C, 0x7A1E1C4C); Put32(ev + 0x10, 0x18000001); Put32(ev + 0x14, T_ADDR(T_TRANS_B));

    for (u32 i = 0; i < 4; i++) {
        static const u32 types[4] = { FSM_NODE_PLAIN, FSM_NODE_START, FSM_NODE_ACTION, FSM_NODE_CHOICE };
        u8* n = img + T_NODES + i * FSM_NODE_SIZE;
        Put32(img + T_NODEPTRS + i * 4, T_ADDR(T_NODES + i * FSM_NODE_SIZE));
        Put32(n + 0x00, T_VTABLE); Put32(n + 0x04, 0x4DF33F00 + i); Put32(n + 0x08, types[i]);
    }
    u8* n1 = img + T_NODES + 1 * FSM_NODE_SIZE;
    Put32(n1 + 0x0C, T_ADDR(T_TRANS_B));
    u8* n2 = img + T_NODES + 2 * FSM_NODE_SIZE;
    Put32(n2 + 0x0C, 0x18000085); Put32(n2 + 0x10, T_ADDR(T_TRANS_A)); Put32(n2 + 0x14, 5); Put32(n2 + 0x18, 6);
    u8* n3 = img + T_NODES + 3 * FSM_NODE_SIZE;
    Put32(n3 + 0x0C, 0x18000086); Put32(n3 + 0x10, T_ADDR(T_CHOICES)); Put32(n3 + 0x14, 2);
    Put32(n3 + 0x1C, 0xFFFFFFFF); Put32(n3 + 0x20, 0xFFFFFFFF);
    Put32(img + T_CHOICES + 0, T_ADDR(T_TRANS_A));
    Put32(img + T_CHOICES + 4, 0);

    u8* sh = img + T_SHARED;
    Put32(sh + 0x00, 0x7A1E1C4C); Put32(sh + 0x04, T_ADDR(T_STRANS)); Put32(sh + 0x08, 0x0003FFFF);

    u8* ta = img + T_TRANS_A;
    Put32(ta + 0x00, T_VTABLE); Put32(ta + 0x04, T_ADDR(T_STATES + FSM_STATE_SIZE));
    Put32(ta + 0x08, 0x0003FFFF); Put32(ta + 0x0C, 0x18000004); Put32(ta + 0x10, 0); Put32(ta + 0x14, 0); Put32(ta + 0x18, 0);
    u8* tb = img + T_TRANS_B;
    Put32(tb + 0x00, T_VTABLE); Put32(tb + 0x04, T_ADDR(T_NODES + 3 * FSM_NODE_SIZE));
    Put32(tb + 0x08, 0x18000002); Put32(tb + 0x0C, 0x0003FFFF); Put32(tb + 0x10, 1); Put32(tb + 0x14, 0); Put32(tb + 0x18, 0);
    u8* ts = img + T_STRANS;
    Put32(ts + 0x00, T_VTABLE); Put32(ts + 0x04, T_ADDR(T_STATES));
    Put32(ts + 0x08, 0x0003FFFF); Put32(ts + 0x0C, 0x18000118); Put32(ts + 0x10, 0); Put32(ts + 0x14, 0);
    Put32(ts + 0x18, 1); Put32(ts + 0x1C, T_ADDR(T_KEYS)); Put32(ts + 0x20, 2); Put32(ts + 0x24, 0);
    Put32(img + T_KEYS + 0, 0x3CDA301E);
    Put32(img + T_KEYS + 4, 0xC80B2CFC);
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

    BuildObjects(hdr, img);
    BuildUi(hdr, img);
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
    CHECK(*(u32*)db == T_VTABLE && *(u32*)((u8*)db + 4) == T_FN_A);   // build-time addresses: plain words
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
    CHECK(a->buildAddr[0] == T_FN_A && a->buildAddr[1] == T_FN_B);
    CHECK(r.ptrImage == 4 + T_OBJECT_PTRS && r.ptrDolResolved == 0 && r.ptrDolKept == 0 && r.ptrUnknown == 0);

    // Input table, read the way RemoteInputGet, RemoteInputUpdate and the step classes do.
    {
        u8* table = *(u8**)(hdr + 0xDC4);
        CHECK(table == img + T_INPUT && *(s32*)(table + INPUT_TABLE_COUNT) == 2);
        const InputSequence* fixed0 = (const InputSequence*)table;
        CHECK(fixed0->stepCount == 1 && fixed0->steps[0] == img + T_STEP_A && fixed0->steps[1] == NULL);
        const InputSequence* seq = (const InputSequence*)(table + *(s32*)(table + INPUT_TABLE_INDEX + 5 * 4) * 0x48 + INPUT_TABLE_SEQS);
        CHECK(seq->stepCount == 2 && seq->steps[0] == img + T_STEP_D && seq->steps[1] == img + T_STEP_A);
        CHECK(((const InputSequence*)(table + INPUT_TABLE_SEQS))->maxTime == 0.15f);
        CHECK(*(u8**)(table + 0x1EEC) == table + INPUT_TABLE_SEQS && *(f32*)(table + 0x1F18) == 320.0f);
        const u8* a = img + T_STEP_A;      // in two sequences: converted once
        CHECK(*(const s32*)(a + 0x08) == 1 && *(const u32*)(a + 0x0C) == 0x0E && *(const u32*)(a + 0x14) == 0x17);
        CHECK(*(const u32*)(a + 0x10) == 0x7F800001);                             // source value: an f32, not yet written
        CHECK(a[0x18] == 0x7F && a[0x19] == 0x80 && a[0x1A] == 0x00 && a[0x1B] == 0x01);   // unused source: untouched
        CHECK(*(const s32*)(a + 0x3C) == 1 && *(const f32*)(a + 0x44) == 1.0f && *(const u32*)(a + 0x48) == INPUT_STEP_BUTTON_HELD);
        CHECK(*(const u32*)(a + 0x4C) == T_VTABLE);                               // build-time address: a plain word
        const u8* b = img + T_STEP_B;
        CHECK(*(const u32*)(b + 0x48) == INPUT_STEP_ROTATE_CW && *(const u32*)(b + 0x30) == 0x11 && *(const u32*)(b + 0x38) == 0x1A);
        CHECK(*(const s32*)(b + 0x50) == -1 && *(const f32*)(b + 0x5C) == 3.0f);
        // The toggle's file size is 0x50: what follows is step D, converted once as D's own fields.
        CHECK(*(const u32*)(img + T_STEP_C + 0x48) == INPUT_STEP_TWO_BUTTON_TOGGLE);
        const u8* d = img + T_STEP_D;
        CHECK(*(const f32*)d == 2.0f && *(const s32*)(d + 0x08) == 1 && *(const f32*)(d + 0x50) == 4.0f);
    }
    // Textures: header fields converted, bytes, name, pixels and palette as they were.
    {
        const u8* start = *(const u8**)(hdr + 0xD94);
        const SirTextureList* list = (const SirTextureList*)(start + 4);
        const SirTexture* t0 = *(const SirTexture**)(hdr + 0x824);
        const SirTexture* t1 = *(const SirTexture**)(hdr + 0x828);
        CHECK(*(const u32*)start == 0x42 && list->count == 2);
        CHECK(list->head == (const u8*)t0 + SIR_TEXTURE_NODE && list->tail == (const u8*)t1 + SIR_TEXTURE_NODE);
        CHECK(t0->headerSize == SIR_TEXTURE_HEADER && t0->size[0] == 56 && t0->size[1] == 40 && t0->gxSize[0] == 64 && t0->gxSize[1] == 48);
        CHECK(t0->unk0C[1] == 0x05 && t0->unk0C[2] == 0x08 && t0->unk11 == 0x20 && t0->mipCount == 1 && t0->format == 9 && t0->unk17 == 2);
        CHECK(t0->dataSize == 0x10 && t0->data == img + T_TEX0 + 0x60 && t0->palette == NULL && t0->unk24 == 2);
        CHECK(t0->next == (const u8*)t1 + SIR_TEXTURE_NODE && t0->prev == NULL && t0->list == (const u8*)list);
        CHECK(t0->unk34 == 2 && strcmp(t0->name, "Icon-One") == 0 && strcmp(t1->name, "Generic_two") == 0);
        CHECK(t0->data[0] == 0xA0 && t0->data[1] == 0xA1 && t0->data[15] == 0xAF);
        CHECK(t1->prev == (const u8*)t0 + SIR_TEXTURE_NODE && t1->next == NULL);
        CHECK(t1->palette == img + T_TEX1 + 0x70 && t1->palette[0] == 0x81 && t1->palette[1] == 0x23);
    }
    // String table and action table.
    {
        const SirStringTable* st = *(const SirStringTable**)(hdr + 0xDA8);
        CHECK(st->count == 3 && st->buildAddr == 0x930DDD40 && st->unk04 == NULL && st->text == (const u8*)img + T_STR + 0x30);
        CHECK(strcmp(st->strings[0], "abc") == 0 && st->strings[1] == NULL && strcmp(st->strings[2], "Wxyz") == 0);
        const SirActionTable* at = *(const SirActionTable**)(hdr + 0x844);
        CHECK(at->count == 2 && at->records[0] == img + T_ROWS_B + 0x20 && at->records[1] == img + T_ROWS_B + 0x30);
        CHECK(img[T_ROWS_B + 0x20] == 0x7F && img[T_ROWS_B + 0x23] == 0x01);     // the records are not converted
    }

    // UI root, font and the objects of slots 0xDB4-0xDBC, read the way the game code does.
    {
        const u8* root = *(const u8**)(hdr + 0xDB0);
        CHECK(root == img + T_UI_ROOT);
        CHECK(root[0x03] == 1 && root[0x04] == 0x90);                              // +0x00, +0x04: no reader, untouched
        CHECK(*(const u32*)(root + 0x10) == 0x7F800001);                           // a pointer the file leaves unset
        const u8* font = *(const u8**)(root + 0x14);                               // fn_8018C9DC(root, 1)
        CHECK(font == img + T_FONT && *(const u8**)(root + 0x18) == NULL);
        CHECK(*(const f32*)(root + 0x38) == 1.0f && *(const f32*)(root + 0x20) == 0.0f);
        CHECK(root[0x80] == 0x7F && *(const u32*)(root + 0x84) == 0);              // the byte at +0x80 keeps its fill
        CHECK(strcmp((const char*)font + 4, "test_rf") == 0 && font[0x03] == 1);
        const u8* obj = *(const u8**)(font + 0xCC);                                // what ScreenSetsInit reads
        CHECK(obj == img + T_FONT_OBJ && *(const s32*)(obj + 4) == UI_FONT_KIND_1 && *(const u32*)obj == T_VTABLE);
        CHECK(*(const u8**)(obj + 8) == img + T_FONT_DATA && *(const s32*)(obj + 0x124) == 2);
        CHECK(*(const u8**)(obj + 0x24) == img + T_TEX0 && *(const u8**)(obj + 0x28) == img + T_TEX1);
        CHECK(obj[0x2C] == 0x7F && obj[0x2F] == 0x01);                             // unused page slot: untouched
        CHECK(img[T_FONT_DATA] == 0x01 && img[T_FONT_DATA + 3] == 0x04);           // font data: not converted
        const u8* list = *(const u8**)(hdr + 0xDB4);
        const u8* obj0 = *(const u8**)(list + 4);
        CHECK(list == img + T_FONTLIST && obj0 == img + T_FONT_OBJ0 && *(const u8**)(list + 8) == obj);
        CHECK(*(const s32*)(obj0 + 4) == UI_FONT_KIND_0 && *(const u8**)(obj0 + 8) == img + T_FONT_DATA);
        const u8* draw = *(const u8**)(hdr + 0xDB8);
        CHECK(draw == img + T_DRAW && *(const u32*)(draw + 4) == T_VTABLE && *(const u8**)(draw + 8) == list);
        CHECK(draw[0x0C] == 0x90 && *(const s32*)(draw + 0x40) == 7 && *(const s32*)(draw + 0x404) == 9);
        const u8* pad = *(const u8**)(hdr + 0xDBC);
        CHECK(pad == img + T_PAD && *(const u32*)(pad + 4) == 2 && *(const u32*)(pad + 0xC) == 0x7F800001 && *(const f32*)(pad + 0x18) == 0.5f);
    }

    // Screen state machines, read the way HudReset, fn_801BC5F0, fn_801BB8A0 and fn_801BC4C4 do.
    {
        const u8* hud = *(const u8**)(hdr + 0xD9C);
        CHECK(hud == img + T_HUD && *(const u32*)(hud + 0x428) == 2 && *(const u32*)(hud + 0x24) == T_VTABLE);
        const u8* m0 = *(const u8**)(hud + 0x28);
        const u8* m1 = *(const u8**)(hud + 0x2C);
        CHECK(m0 == img + T_FSM0 && m0 == *(const u8**)(hdr + 0xD98) && m1 == img + T_FSM1 && *(const u8**)(hud + 0x30) == NULL);
        CHECK(*(const s32*)(m0 + 4) == -1 && *(const s32*)(m0 + 0xC) == -1 && *(const u32*)(m0 + 0x28) == 0);
        CHECK(*(const f32*)(m0 + 0x4C) == 5.0f && *(const u32*)(m0 + 0x50) == 0x7F800001);
        CHECK(m0[0x63] == 0xFE && m0[0x60] == 0xFF);                               // +0x58..: no reader, untouched
        const u32* values = *(const u32**)(m0 + 0x54);
        CHECK((const u8*)values == img + T_VALUES && values[0] == 2 && values[2] == 0x17FC0004 && *(const u8**)(m1 + 0x54) == NULL);
        const u8* def = *(const u8**)m0;
        CHECK(def == img + T_DEF && *(const u8**)m1 == def);
        CHECK(*(const s32*)(def + 4) == 2 && *(const s32*)(def + 0x1C) == 4 && *(const s32*)(def + 0x2C) == 1);
        CHECK(def[0x0B] == 2 && def[0x17] == 4 && def[0x27] == 1);                 // the words next to the counts: untouched
        CHECK(*(const u32*)(def + 0x30) == 0x10CD5CCF && *(const u32*)(def + 0x3C) == FSM_NODE_START && def[0x44] == 1);
        CHECK(*(const u8**)(def + 0x40) == img + T_TRANS_A && *(const u8**)(def + 0xC) == NULL);
        const u8* s0 = *(const u8**)def;
        const u8* s1 = s0 + FSM_STATE_SIZE;
        CHECK(s0 == img + T_STATES && *(const u32*)s0 == T_VTABLE && *(const u32*)(s0 + 4) == 0x3CDA301E && *(const u32*)(s0 + 8) == FSM_NODE_STATE);
        CHECK(*(const u32*)(s0 + 0xC) == 0x18000000 && *(const u32*)(s0 + 0x14) == 0x0003FFFF && *(const f32*)(s0 + 0x18) == 62.0f);
        CHECK(*(const u32*)(s0 + 0x1C) == 1 && *(const s32*)(s0 + 0x2C) == 2 && s0[0x27] == 2);
        CHECK(*(const f32*)(s1 + 0x18) == -1.0f && *(const u8**)(s1 + 0x20) == NULL && *(const s32*)(s1 + 0x2C) == 0);
        const u8* ev = *(const u8**)(s0 + 0x20);
        CHECK(ev == img + T_EVENTS && *(const u32*)ev == 0xBE048B88 && *(const u8**)(ev + 8) == img + T_TRANS_A);
        CHECK(*(const u32*)(ev + 0x10) == 0x18000001 && *(const u8**)(ev + 0x14) == img + T_TRANS_B);
        const u8* const* nodes = *(const u8* const**)(def + 0x10);
        CHECK(nodes[0] == img + T_NODES && *(const u32*)(nodes[0] + 8) == FSM_NODE_PLAIN && nodes[0][0x0C] == 0x7F);
        CHECK(*(const u32*)(nodes[1] + 8) == FSM_NODE_START && *(const u8**)(nodes[1] + 0xC) == img + T_TRANS_B);
        CHECK(*(const u32*)(nodes[2] + 0xC) == 0x18000085 && *(const u8**)(nodes[2] + 0x10) == img + T_TRANS_A);
        CHECK(*(const s32*)(nodes[2] + 0x14) == 5 && *(const s32*)(nodes[2] + 0x18) == 6);
        CHECK(*(const u32*)(nodes[3] + 4) == 0x4DF33F03 && *(const s32*)(nodes[3] + 0x14) == 2 && *(const s32*)(nodes[3] + 0x1C) == -1);
        const u8* const* choices = *(const u8* const**)(nodes[3] + 0x10);
        CHECK((const u8*)choices == img + T_CHOICES && choices[0] == img + T_TRANS_A && choices[1] == NULL);
        const u8* ta = img + T_TRANS_A;
        const u8* tb = img + T_TRANS_B;
        CHECK(*(const u8**)(ta + 4) == s1 && *(const u32*)(ta + 0xC) == 0x18000004 && *(const s32*)(ta + 0x10) == 0);
        CHECK(*(const u8**)(tb + 4) == nodes[3] && *(const u32*)(tb + 8) == 0x18000002 && *(const s32*)(tb + 0x10) == 1);
        CHECK(ta[0x1C] == 0x7F && ta[0x1F] == 0x01);                               // +0x18, +0x1C: no reader, untouched
        const u8* sh = *(const u8**)(def + 0x20);
        const u8* ts = *(const u8**)(sh + 4);
        CHECK(*(const u32*)sh == 0x7A1E1C4C && ts == img + T_STRANS && *(const u32*)(sh + 8) == 0x0003FFFF);
        CHECK(*(const u8**)(ts + 4) == s0 && *(const u32*)(ts + 0x18) == 1 && *(const s32*)(ts + 0x20) == 2);
        CHECK((*(const u32**)(ts + 0x1C))[1] == 0xC80B2CFC && ts[0x27] == 0);
    }

    // Pointer fields of a hand-written schema: image, main.dol (known and unknown), elsewhere, NULL.
    {
        static const SirField fields[] = { { 0x0, SIR_PTR, 5 }, { 0x14, SIR_U16, 2 }, { 0x18, SIR_F32, 1 } };
        u8 rec[0x20];
        memset(rec, 0xEE, sizeof(rec));
        Put32(rec + 0x00, GLOBAL_BINARY_WII_BASE + 0x10);
        Put32(rec + 0x04, T_VTABLE);
        Put32(rec + 0x08, T_FN_A);
        Put32(rec + 0x0C, 0x930DDD40);
        Put32(rec + 0x10, 0);
        Put16(rec + 0x14, 0x1234); Put16(rec + 0x16, 0xFFFE);
        Put32(rec + 0x18, 0x3F800000);
        CHECK(SirConvertRecord(&r, rec, sizeof(rec), fields, 3));
        CHECK(*(u8**)(rec + 0x00) == img + 0x10);
        CHECK(*(void**)(rec + 0x04) == (void*)gFakeVtable);
        CHECK(*(u32*)(rec + 0x08) == T_FN_A && *(u32*)(rec + 0x0C) == 0x930DDD40 && *(void**)(rec + 0x10) == NULL);
        CHECK(*(u16*)(rec + 0x14) == 0x1234 && *(s16*)(rec + 0x16) == -2 && *(f32*)(rec + 0x18) == 1.0f);
        CHECK(rec[0x1C] == 0xEE && rec[0x1F] == 0xEE);
        CHECK(r.ptrImage == 5 + T_OBJECT_PTRS && r.ptrDolResolved == 1 && r.ptrDolKept == 1 && r.ptrUnknown == 1);
        static const SirField outside[] = { { 0x1E, SIR_U32, 1 } };
        CHECK(!SirConvertRecord(&r, rec, sizeof(rec), outside, 1));
    }

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
        { T_INPUT + INPUT_TABLE_COUNT, INPUT_SEQ_MAX + 1, SIR_ERR_INPUT, -1 },              // more sequences than fit
        { T_INPUT + INPUT_TABLE_INDEX + 8, INPUT_SEQ_MAX, SIR_ERR_INPUT, -1 },              // index past the sequences
        { T_INPUT + INPUT_TABLE_SEQS, INPUT_SEQ_STEPS + 1, SIR_ERR_INPUT, -1 },             // more steps than slots
        { T_INPUT + INPUT_TABLE_SEQS + 0x10, T_ADDR(T_SIZE - 0x20), SIR_ERR_INPUT, -1 },    // step runs off the image
        { T_STEP_B + 0x48, INPUT_STEP_TYPES, SIR_ERR_INPUT, -1 },                           // unknown step class
        { T_STEP_D + 0x08, INPUT_STEP_SOURCES + 1, SIR_ERR_INPUT, -1 },                     // too many sources
        { T_INPUT + INPUT_TABLE_SEQS + 0x10, T_ADDR(T_INPUT + 0x100), SIR_ERR_OVERLAP, -1 },// step inside the table
        { T_INPUT + INPUT_TABLE_SEQS + 0x10, T_ADDR(T_STEP_A + 4), SIR_ERR_INPUT, -1 },     // "step" that is the middle of another
        { T_TEX1 + 0x00, 0x58, SIR_ERR_TEXTURE, -1 },                                       // not a texture header
        { T_TEX1 + 0x2C, 0, SIR_ERR_TEXTURE, -1 },                                          // does not name the texture before it
        { T_TEX1 + 0x28, T_ADDR(T_TEX0 + SIR_TEXTURE_NODE), SIR_ERR_TEXTURE, -1 },          // list loops
        { T_TEX0 + 0x18, T_SIZE, SIR_ERR_TEXTURE, -1 },                                     // pixels run off the image
        { T_TEX0 + 0x30, T_ADDR(T_TEXLIST), SIR_ERR_TEXTURE, -1 },                          // belongs to another list
        { T_TEXLIST + 0x0C, 3, SIR_ERR_TEXTURE, -1 },                                       // count disagrees with the links
        { T_STR + 0x0C, 0x40000000, SIR_ERR_STRINGS, -1 },                                  // array runs off the image
        { T_STR + 0x28, T_ADDR(T_SIZE), SIR_ERR_STRINGS, -1 },                              // string outside the image
        { T_ACT + 0x0C, 0x12345678, SIR_ERR_ACTIONS, -1 },                                  // record outside the image
        { T_ACT + 0x04, T_ADDR(T_STR + 0x24), SIR_ERR_OVERLAP, -1 },                        // two tables share an array
        { T_UI_ROOT + 0x14, T_ADDR(T_SIZE - 0x40), SIR_ERR_UI, -1 },                        // font runs off the image
        { T_FONT_OBJ + 0x04, 2, SIR_ERR_UI, -1 },                                           // font object of no known class
        { T_FONT_OBJ + 0x124, UI_FONT_PAGES + 1, SIR_ERR_UI, -1 },                          // more pages than slots
        { T_FONT_OBJ + 0x28, 0x12345678, SIR_ERR_UI, -1 },                                  // page outside the image
        { T_DRAW + 0x08, 0x80001234, SIR_ERR_UI, -1 },                                      // not an image pointer
        { T_HUD + 0x428, HUD_MACHINES + 1, SIR_ERR_SCREENS, -1 },                           // more machines than slots
        { T_FSM1 + 0x00, T_ADDR(T_SIZE - 8), SIR_ERR_SCREENS, -1 },                         // definition runs off the image
        { T_VALUES + 0x00, 0x40000000, SIR_ERR_SCREENS, -1 },                               // value table runs off the image
        { T_DEF + 0x04, 0x08000000, SIR_ERR_SCREENS, -1 },                                  // more states than fit
        { T_NODES + 0x08, FSM_NODE_STATE, SIR_ERR_SCREENS, -1 },                            // a state in the node array
        { T_NODES + 0x30 + 0x08, 5, SIR_ERR_SCREENS, -1 },                                  // unknown node type
        { T_NODEPTRS + 0x08, 0, SIR_ERR_SCREENS, -1 },                                      // NULL node (fn_801BC5F0 reads it)
        { T_EVENTS + 0x08, T_ADDR(T_SIZE - 4), SIR_ERR_SCREENS, -1 },                       // transition runs off the image
        { T_STRANS + 0x20, 0x40000000, SIR_ERR_SCREENS, -1 },                               // key list runs off the image
        { T_EVENTS + 0x08, T_ADDR(T_STRANS), SIR_ERR_OVERLAP, -1 },                         // one transition used as both kinds
        { T_STATES + 0x20, T_ADDR(T_HUD + 0x100), SIR_ERR_OVERLAP, -1 },                    // events inside the HUD
        { T_FSM1 + 0x54, T_ADDR(T_DEF + 0x04), SIR_ERR_OVERLAP, -1 },                       // value table inside the definition
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

    // Catalog: two entries and one file record.
    {
        u8 cat[0x40 + 2 * 0x18 + 0x40];
        memset(cat, 0, sizeof(cat));
        memcpy(cat, "PNP0", 4);
        Put32(cat + 0x04, 3); Put32(cat + 0x08, 2); Put32(cat + 0x0C, 1); Put32(cat + 0x10, 0xC628EA26);
        u32 h = NameHash("Global_Binary.SIR");
        CHECK(h == 0x756E99D4);
        Put32(cat + 0x40, 0xFFFFFFFF); Put32(cat + 0x44, 0x800);  Put32(cat + 0x48, 0x1000);   Put32(cat + 0x4C, h);
        Put32(cat + 0x58, 0);          Put32(cat + 0x5C, 0x1800); Put32(cat + 0x60, 0x10B9BC); Put32(cat + 0x64, h);
        Put32(cat + 0x68, 0x4321);     Put32(cat + 0x6C, 0x66767A5C);
        Put32(cat + 0x70, h); Put32(cat + 0x74, 0x11223344);
        memcpy(cat + 0x78, "global_binary.sir", 18);
        SirCatalog c;
        CHECK(SirCatalogConvert(cat, sizeof(cat) - 1, &c) == SIR_ERR_CATALOG);
        CHECK(cat[0x0B] == 2);                                   // refused: still big-endian
        CHECK(SirCatalogConvert(cat, sizeof(cat), &c) == SIR_OK);
        CHECK(c.entryCount == 2 && c.fileCount == 1 && *(u32*)(cat + 0x10) == 0xC628EA26);
        const SirCatalogEntry* en = SirCatalogFind(&c, h, 0);
        CHECK(en == &c.entries[1] && en->offset == 0x1800 && en->size == 0x10B9BC && en->packedSize == 0x4321 && en->hash == 0x66767A5C);
        CHECK(SirCatalogFind(&c, h, -1) == &c.entries[0] && SirCatalogFind(&c, h, 1) == NULL);
        CHECK(SirCatalogFileIndex(&c, h) == 0 && SirCatalogFileIndex(&c, 1) == -1);
        CHECK(c.files[0].unk04 == 0x11223344 && strcmp(c.files[0].name, "global_binary.sir") == 0);
        CHECK(memcmp(cat, "PNP0", 4) == 0);
    }

    free(hdr);
    free(img);
    free(copy);
    printf(gFailed ? "selftest: %d FAILED\n" : "selftest: ok\n", gFailed);
    return gFailed ? 1 : 0;
}

int main(int argc, char** argv) {
    if (argc == 2 && strcmp(argv[1], "--selftest") == 0) return SelfTest();
    if (argc == 3 && strcmp(argv[1], "--dump") == 0) return Dump(argv[2]);
    if (argc == 3 && strcmp(argv[1], "--catalog") == 0) return DumpCatalog(argv[2]);
    fprintf(stderr, "usage: sir_loader_test --selftest | --dump <global_binary.sir> | --catalog <catalog.sir>\n");
    return 2;
}
