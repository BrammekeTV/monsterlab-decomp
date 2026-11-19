// Fixed m2c drafts from build/drafts/draft_miss61.cpp (batch 61b), verified against the asm.
// Text-table lookups, save/tier helpers, a table-slot compactor and small world/event helpers.
#include "types.h"
#include "game/part.h"

struct StringTable {
    const char** strings;
    u8           pad[8];
    u32          count;     // 0x0C
};

extern "C++" u8 gGame[];                        // 0x803B53A0
extern "C++" u8 gWorldState[];                  // 0x803E6D00
extern "C++" StringTable* gStringTable;         // 0x805FB910
extern "C++" StringTable* gStringTable2;        // 0x805FB90C
extern "C++" void* gEventMgr;                   // 0x805FB6C8
extern "C++" f32 gDebrisVolumeUnit;             // 0x805FB2DC
extern "C++" u8* lbl_805FB2E8;                  // world/event block pointer
extern "C++" u8 lbl_805FBBE0[];                 // save block pointer (value at this address)
extern "C++" u8 lbl_805FB210;                   // flag byte
extern "C++" s32 lbl_805FB2D8;                  // counter
extern "C++" void* lbl_805FB5A8;                // handle passed to fn_80169AA8 / fn_80169C6C
extern "C++" u8 lbl_8047C760[];
extern "C++" u8 lbl_8047C790[];
extern "C++" u8 lbl_8047C7AC[];
extern "C++" u8 lbl_8047C640[];
extern "C++" f32 lbl_805FEE40;
extern "C++" f32 lbl_805FEE60;
extern "C++" f32 lbl_805FEE64;
extern "C++" f32 lbl_805FEE68;
extern "C++" f32 lbl_805FEE6C;
extern const char lbl_805F8200[];               // empty string, fallback text
extern const char lbl_80354800[];               // "Not found"
extern const char lbl_8035480C[];               // "NOT FOUND"
extern const char lbl_80375950[];               // SysSprintf format string

extern "C" {
BOOL SavePartTierSeen(u8* s, u32 part, s32 tier);                  // 0x8022397C
int  SysSprintf(char* buf, const char* fmt, ...);
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190
void fn_80116EF0(u8* a3, s32 a4);
void fn_801F5528(void);
void fn_801C7570(u8* list, s32 n);
s32  fn_801D3A60(s32 id);
void fn_801C9D74(u8* p);
void fn_8020A6C8(void* a0);
void fn_801C3924(void* a0);
void fn_801BDA0C(void* a0);
void fn_801D8D7C(void);
void fn_80169AA8(void* a0);
void fn_80169C6C(void* a0, s32 a1);
void fn_80128C20(void* world);
void fn_801D94CC(u8* p);
void fn_801DA81C(u8* p);
void fn_801DA1CC(u8* p);
void fn_801DA7C8(u8* p);
void fn_801DB160(u8* p);
void fn_801D9A24(u8* p);
}

typedef s32 (*VFnGet)(u8*);

extern "C" {

// Text lookup by id (TextIdFromHash result). -1 gives "Not found"; high bit selects gStringTable2.
static const char* TextStringFromId(s32 id) {
    if (id == -1) return lbl_80354800;
    if (((u32)id & 0x80000000u) && gStringTable2 != NULL) {
        u8* st = (u8*)gStringTable2;
        u8* arr = (u8*)(uintptr_t)*(u32*)st;
        if (arr != NULL) {
            u32 i = (u32)id & 0x7FFFFFFFu;
            if (i < *(u32*)(st + 0xC)) return (const char*)(uintptr_t)*(u32*)(arr + i * 4);
        }
        return lbl_805F8200;
    }
    u8* st = (u8*)gStringTable;
    u8* arr = (u8*)(uintptr_t)*(u32*)st;
    if (arr != NULL && (u32)id < *(u32*)(st + 0xC)) return (const char*)(uintptr_t)*(u32*)(arr + (u32)id * 4);
    return lbl_805F8200;
}

// 0x801201B4
const char* fn_801201B4(u8* p) {
    s32 idx = *(s32*)(p + 0x925C);
    if (idx < *(s32*)(p + 0xA644)) {
        u8* entry = (u8*)(uintptr_t)*(u32*)(p + 0x9264 + idx * 4);
        return (const char*)(uintptr_t)*(u32*)(entry + 0x8);
    }
    return TextStringFromId(TextIdFromHash(0x557));
}

// 0x8012029C
const char* fn_8012029C(u8* p, s32 arg1) {
    s32 idx = arg1 + *(s32*)(p + 0xA648) * 4;
    if (idx >= 0 && idx < *(s32*)(p + 0xA644)) {
        u8* entry = (u8*)(uintptr_t)*(u32*)(p + 0x9264 + idx * 4);
        return (const char*)(uintptr_t)*(u32*)(entry + 0x8);
    }
    return TextStringFromId(TextIdFromHash(0x557));
}

// 0x80120534
// Steps the cursor at +0x4 / +0xA654 by arg1 through six states (3 is skipped, 5 maps to 1).
void fn_80120534(u8* p, s32 arg1) {
    s32 r6 = *(s32*)(p + 0xA654) + arg1;
    s32 r5 = (arg1 < 0) ? -1 : 1;
    u32 ctr = (u32)(5 - r6) / 6u;
    if (r6 < 0) {
        do {
            r6 += 6;
        } while (--ctr != 0);
    }
    if (r6 >= 6) r6 %= 6;
    if (r6 == 3) r6 += r5;
    *(s32*)(p + 0xA654) = r6;
    *(s32*)(p + 0xA648) = 0;
    *(s32*)(p + 0x4) = r6;
    if (r6 == 5) {
        *(s32*)(p + 0x4) = 1;
        return;
    }
    if (r6 < 3) {
        *(s32*)(p + 0x4) = r6 + 2;
        return;
    }
    *(s32*)(p + 0x4) = r6 + 1;
}

// 0x801205EC
// Clamps the sub-step counter at +0xA648 (ceil(count / 4) blocks, advanced by 3 * arg1) and then
// tail-calls fn_80116EF0 with the new counter.
void fn_801205EC(u8* p, s32 arg1) {
    s32 blocks = (*(s32*)(p + 0xA644) + 3) >> 2;   // ceil(n / 4)
    if (blocks <= 0) return;
    s32 r5 = *(s32*)(p + 0xA648) + arg1 * 3;
    if (blocks > r5) {
        if (r5 < 0) r5 = 0;
        *(s32*)(p + 0xA648) = r5;
    }
    fn_80116EF0(p + 0xA658, *(s32*)(p + 0xA648));
}

// 0x801206A0
// Whether any part of the given kind (table at gGameDb->data + 0x6C18, 0x9C entries of 0x24 bytes,
// kind at +0xC) has a seen tier in the save block.
s32 fn_801206A0(u8* unusedR3, s32 kind) {
    u8* table = (u8*)(uintptr_t)*(u32*)(gGameDb->data + 0x6C18);
    for (s32 part = 0; part < 0x9C; part++) {
        u8* entry = table + part * 0x24;
        if (*(s32*)(entry + 0xC) != kind) continue;
        for (s32 tier = 0; tier < 4; tier++) {
            if (SavePartTierSeen(*(u8**)lbl_805FBBE0, (u32)part, tier) != 0) return 1;
        }
    }
    return 0;
}

// 0x80122A10
// Registers a 32-bit handle in the active list at gGame+0x1C4 (count at +0x1C0, max 0xFF) and
// writes its slot index to p+0x40 (0xFF when the list is full).
void fn_80122A10(u8* p, u32 handle) {
    u8* g = gGame;
    u8 count = g[0x1C0];
    for (u8 i = 0; i < count; i++) {
        if (*(u32*)(g + 0x1C4 + i * 4) == handle) {
            p[0x40] = i;
            return;
        }
    }
    for (u8 i = 0; i < count; i++) {
        if (*(u32*)(g + 0x1C4 + i * 4) == 0) {
            *(u32*)(g + 0x1C4 + i * 4) = handle;
            p[0x40] = i;
            return;
        }
    }
    if (count < 0xFF) {
        *(u32*)(g + 0x1C4 + count * 4) = handle;
        g[0x1C0] = (u8)(count + 1);
        p[0x40] = count;
        return;
    }
    char msg[256];
    SysSprintf(msg, lbl_80375950, 0xFF);
    p[0x40] = 0xFF;
}

// 0x80124824
// Adds a volume contribution to gDebrisVolumeUnit depending on the entry type at +0xC
// (0, 4 or 7), then bumps the counter at lbl_805FB2D8. Always returns 1.
s32 fn_80124824(u8* p) {
    s32 type = *(s32*)(p + 0xC);
    if (type == 4) {
        f32 t = *(f32*)(p + 0x3C) * *(f32*)(p + 0x40);
        t = *(f32*)(p + 0x44) * t;
        gDebrisVolumeUnit = gDebrisVolumeUnit + t;
    } else if (type == 7) {
        f32 t = *(f32*)(p + 0x6C) * *(f32*)(p + 0x70);
        t = *(f32*)(p + 0x74) * t;
        gDebrisVolumeUnit = gDebrisVolumeUnit + t;
    } else if (type == 0) {
        f32 f2 = *(f32*)(p + 0x3C);
        bool inRange = (f2 > lbl_805FEE60) && (f2 < lbl_805FEE64);
        if (inRange || !(f2 >= lbl_805FEE40)) f2 = lbl_805FEE68;
        f32 t = lbl_805FEE6C * f2;
        t = t * f2;
        t = f2 * t;
        gDebrisVolumeUnit = gDebrisVolumeUnit + t;
    }
    lbl_805FB2D8 = lbl_805FB2D8 + 1;
    return 1;
}

// Queues event 0x7E57AD29 for the entry at world+count*4 (count at +0x104, max 0x40), then sets the
// count to 0x40.
static void SendQueuedEvent(u8* world) {
    s32 count = *(s32*)(world + 0x104);
    if (count >= 0x40) return;
    void* target = (void*)(uintptr_t)*(u32*)(world + count * 4);
    EventSend(gEventMgr, target, NULL, 0x7E57AD29u, 0, 0, 1, 1);
    *(s32*)(world + 0x104) = 0x40;
}

// 0x8012660C
void fn_8012660C(s32 v) {
    u8* world = lbl_805FB2E8;
    *(s32*)(world + 0x14C) = v;
    if (v == 0) return;
    SendQueuedEvent(world);
}

// 0x80126684
void fn_80126684(s32 v) {
    u8* world = lbl_805FB2E8;
    *(s32*)(world + 0x148) = v;
    if (v != 0) {
        *(s32*)(world + 0x14C) = 0;
        return;
    }
    SendQueuedEvent(world);
}

// 0x80129438
// Tears down the world-side objects when byte +5 is set, then clears the flags at +4 and +5.
void fn_80129438(u8* p) {
    lbl_805FB210 = 0;
    fn_80128C20(p);
    if (p[5] != 0) {
        fn_801C9D74(lbl_8047C760);
        fn_8020A6C8(lbl_8047C760);
        fn_801C3924(lbl_8047C7AC);
        fn_801BDA0C(lbl_8047C790);
    }
    fn_801D8D7C();
    fn_80169AA8(lbl_805FB5A8);
    fn_80169C6C(lbl_805FB5A8, 1);
    p[4] = 0;
    p[5] = 0;
}

// 0x8012AAA4
void fn_8012AAA4(u8* world) {
    fn_801F5528();
    *(s32*)(world + 0x10C) = 1;
}

// 0x8012F340
// Collects the position of each flagged object in the gGame+0x140 table (32 slots) into a list of
// 0x10-byte records {x, y, z, 8} and passes the list to fn_801C7570.
void fn_8012F340(void) {
    u8 buf[0x200];
    u8* out = buf;
    s32 count = 0;
    for (s32 i = 0; i < 0x20; i++) {
        u8* obj = (u8*)(uintptr_t)*(u32*)(gGame + 0x140 + i * 4);
        if (obj == NULL) continue;
        if ((*(VFnGet*)(*(u8**)obj + 0x4C))(obj) != 0) continue;
        u8* sub = (u8*)(uintptr_t)*(u32*)(obj + 4);
        if ((*(u32*)(sub + 0x1C) & 0x20u) == 0) continue;
        *(f32*)(out + 0x0) = *(f32*)(sub + 0x0);
        *(f32*)(out + 0x4) = *(f32*)(sub + 0x4);
        *(f32*)(out + 0x8) = *(f32*)(sub + 0x8);
        *(u32*)(out + 0xC) = 8;
        out += 0x10;
        count++;
    }
    if (count > 0) fn_801C7570(buf, count);
}

// 0x8012F59C
// Whether id is 0, 0x63 (always true), or an entry of the world table (gWorldState+0xB0, stride 0x30)
// with type 3 and matching id, within the count stored at the table header.
s32 fn_8012F59C(u8* p, s32 id) {
    if (id == 0 || id == 0x63) return 1;
    u8* tbl = (u8*)(uintptr_t)*(u32*)p;
    if (id > *(s32*)(tbl + 4)) return 0;
    s32 idx = fn_801D3A60(id);
    u8* base = (u8*)(uintptr_t)*(u32*)(gWorldState + 0xB0);
    u8* e = base + idx * 0x30;
    if (*(s32*)(e + 4) == 3 && *(s32*)(e + 8) == id) return 1;
    return 0;
}

// 0x8012F818
// Dispatches a teardown by the type at +4 (1, 2, 3 or 4); does nothing for a null pointer.
void fn_8012F818(u8* p) {
    if (p == NULL) return;
    switch (*(s32*)(p + 4)) {
    case 1:
        fn_801D94CC(p);
        break;
    case 2:
        fn_801DA81C(p);
        fn_801DB160(p);
        break;
    case 3:
        fn_801DA1CC(p);
        fn_801DA7C8(p);
        break;
    case 4:
        fn_801D9A24(p);
        break;
    default:
        break;
    }
}

}  // extern "C"
