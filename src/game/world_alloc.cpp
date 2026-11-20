// World memory: one allocation holding the world header, every zone record with its entity
// slot arrays, and four per-zone tables.
// Original: WorldDataCreate 0x801F0704 (auto_03_801F0704_text.s)
#include <stdint.h>
#include <string.h>

#include "types.h"

extern "C" {
u8*   GetGlobalSettings(void);          // 0x801D2BF8
void* MemAllocDefault(u32 size);        // 0x801D4650
}

// Slot counts for zones that hold many entities.
struct WorldParams {
    s32   dynamicSlots;   // 0x00
    s32   staticSlots;    // 0x04
    void** out;           // 0x08: receives the world data
};

WorldParams* sWorldParams;       ORIG(0x805FB944)
s32                 gZoneResourceSize;  ORIG(0x805FB94C) // bytes per zone record
static u32          sEntitySlotBytes;   ORIG(0x805FB940)

static inline s32 S(u32 off) {
    return *(s32*)(GetGlobalSettings() + off);
}

// Number of zones: 13 per-category counters (+0x68 .. +0x98) plus zone 0.
static inline s32 ZoneCount(void) {
    return S(0x80) + S(0x68) + S(0x84) + S(0x88) + S(0x8C) + S(0x90) + S(0x94) + S(0x98) +
           S(0x6C) + S(0x70) + S(0x74) + S(0x78) + S(0x7C) + 1;
}

static inline u32 ZoneRecordSize(void) {
    return S(0x34) * 0x54 + 0x148;
}

// Size of everything after the shared entity slots.
static inline u32 FixedSize(void) {
    u32 size = ZoneRecordSize() * ZoneCount() + 0x29E0;
    size += ZoneCount() * 4;
    size += ZoneCount() * 4;
    size += ZoneCount() * 0xC;
    size += ZoneCount() * 0x18;
    return size;
}

// Zones in these counter ranges only ever hold one static and one dynamic entity.
static inline BOOL IsSmallZone(s32 z) {
    s32 a = S(0x6C) + S(0x68) + S(0x70) + S(0x74) + S(0x78) + S(0x7C);
    if (!(a + 1 > z) && z <= a + S(0x80)) return TRUE;
    s32 b = a + S(0x80);
    if (!(b + 1 > z) && z <= b + S(0x84)) return TRUE;
    s32 c = b + S(0x84);
    if (!(c + 1 > z) && z <= c + S(0x88)) return TRUE;
    if (!(S(0x68) + 1 > z) && z <= a) return TRUE;
    return FALSE;
}

struct ZoneTable { u8* data; s32 count; s32 used; };

// 0x801F0704
void WorldDataCreate(WorldParams* params) {
    sWorldParams = params;
    gZoneResourceSize = ZoneRecordSize();

    sEntitySlotBytes = (S(0x68) + 1) * ((sWorldParams->staticSlots + sWorldParams->dynamicSlots) * 4);
    u32 size = FixedSize() + sEntitySlotBytes;

    u8* raw = (u8*)MemAllocDefault(size + 0x24);
    u8* w = (u8*)(((uintptr_t)raw + 0x23) & ~(uintptr_t)0x1F);
    ((u8**)w)[-1] = raw;
    sEntitySlotBytes = (S(0x68) + 1) * ((sWorldParams->staticSlots + sWorldParams->dynamicSlots) * 4);
    memset(w, 0, FixedSize() + sEntitySlotBytes);

    // Zone records.
    u8* shared = w + 0x21E0;
    ZoneTable* zones = (ZoneTable*)(w + 0x1A4);
    zones->data = shared;
    zones->count = ZoneCount();
    zones->used = 0;
    shared += ZoneRecordSize() * ZoneCount();

    for (s32 z = 0; z < ZoneCount(); z++) {
        u8* zi = zones->data + z * gZoneResourceSize;
        u8* own = zi + 0x140;
        *(u8**)(zi + 0x114) = own;
        *(s32*)(zi + 0x118) = S(0x34);
        *(s32*)(zi + 0x11C) = 0;
        own += S(0x34) * 0x54;

        s32 statics = sWorldParams->staticSlots;
        s32 dynamics = sWorldParams->dynamicSlots;
        u8** cursor = &own;
        if (IsSmallZone(z)) {
            statics = 1;
            dynamics = 1;
        } else {
            own += 8;
            cursor = &shared;
        }

        *(u8**)(zi + 0x120) = *cursor;     // static entity slots
        *(s32*)(zi + 0x124) = statics;
        *(s32*)(zi + 0x128) = 0;
        *cursor += statics * 4;
        *(u8**)(zi + 0x12C) = *cursor;     // dynamic entity slots
        *(s32*)(zi + 0x130) = dynamics;
        *(s32*)(zi + 0x134) = 0;
        *cursor += dynamics * 4;
    }

    // Per-zone tables: two pointer arrays, effects (0xC each), and 0x18-byte records.
    static const u32 kEntrySize[4] = { 4, 4, 0xC, 0x18 };
    for (int t = 0; t < 4; t++) {
        ZoneTable* tbl = (ZoneTable*)(w + 0x1B0 + t * 0xC);
        tbl->data = shared;
        tbl->count = ZoneCount();
        tbl->used = 0;
        shared += ZoneCount() * kEntrySize[t];
    }

    *params->out = w;
}

// ---------------------------------------------------------------- world state setup

extern "C" {
void  WorldLoadLevelZones(s32 level);              // 0x80126AB0
void  ZoneSlotInit(void* slot);                    // 0x801F2B40
void  WorldResetQueues(void* world);               // 0x8013022C
void* IdPoolNext(void* pool);                      // 0x801E6EB0
void  SetCharacterPerformanceCapToDefault(void);
void* memset(void* p, int c, size_t n);
}
extern u8   gGame[];                                // 0x803B53A0
extern u8   gLevelFlags[];                          // 0x80369D60: 0x20 bytes per level
extern u8   gIdPool[];                              // 0x8048A8F0
s32  sZoneBudget;                            ORIG(0x805FB30C)
static u8   sZonesDirty;                            ORIG(0x805FB310)

// Eight growable arrays { data, count, owned } shared by all zones, 0x18 bytes apart.
extern u8   gZoneLists[];                           // 0x803E9FD4

static inline s32 SlotCountAll(void) {
    s32 n = 0;
    for (u32 off = 0x34; off <= 0x64; off += 4) n += S(off);
    return n;
}

static inline void ArrayAlloc(u8* w, u32 off, s32 count, u32 elem) {
    *(s32*)(w + off + 8) = 1;
    *(s32*)(w + off + 4) = count;
    *(void**)(w + off) = MemAllocDefault(count * elem);
}

// 0x80127680
// Reset the world state for `level`: clear the game's per-level tables, allocate the zone
// slot array, the zone tables and the six shared zone lists, and reset the event queues.
void WorldStateInit(u8* w, s32 level) {
    // Levels flagged small only get half the slot budget.
    sZoneBudget = (gLevelFlags[(u8)level * 0x20 + 0x18] >> 2) & 1 ? S(0x34) / 2 : S(0x34);
    *(f32*)(w + 8) = 0.0f;
    *(f32*)(w + 0xC) = 0.0f;

    *(s32*)(gGame + 0x1C0) = 0;
    for (u8 i = 0; i != 0xFF; i++) ((s32*)(gGame + 0x1C4))[i] = 0;
    *(s32*)(gGame + 0x13C) = 0;
    for (u8 i = 0; i != 0x20; i++) ((s32*)(gGame + 0x140))[i] = 0;
    sZonesDirty = 0;
    WorldLoadLevelZones(level);
    *(s32*)(w + 0x10) = 0;

    // Zone slots (0x30 each).
    ArrayAlloc(w, 0xB0, SlotCountAll(), 0x30);
    for (s32 i = 0; i < *(s32*)(w + 0xB4); i++) {
        u8* slot = *(u8**)(w + 0xB0) + i * 0x30;
        if (slot != NULL) {
            *(s32*)slot = 0;
            ZoneSlotInit(slot);
        }
    }

    ArrayAlloc(w, 0x284, SlotCountAll(), 0x10);
    memset(*(void**)(w + 0x284), 0, *(s32*)(w + 0x288) * 0x10);
    WorldResetQueues(w);
    w[0x121] = 0;
    w[0x120] = 0;
    w[0x122] = 0;
    w[0x123] = 0;
    *(void**)(w + 0x118) = IdPoolNext(gIdPool);
    *(void**)(w + 0x11C) = IdPoolNext(gIdPool);

    ArrayAlloc(w, 0xC8, ZoneCount(), 4);
    for (s32 i = 0; i < *(s32*)(w + 0xCC); i++) ((void**)*(void**)(w + 0xC8))[i] = NULL;
    ArrayAlloc(w, 0xD4, ZoneCount(), 4);
    for (s32 i = 0; i < *(s32*)(w + 0xD8); i++) ((void**)*(void**)(w + 0xD4))[i] = NULL;

    static const u32 kListElem[8] = { 0x10, 0x10, 0x10, 0xC, 4, 4, 4, 4 };
    for (s32 l = 0; l < 8; l++) ArrayAlloc(gZoneLists, l * 0x18, ZoneCount(), kListElem[l]);

    *(s32*)(w + 0x10C) = 0;
    w[4] = 1;
    w[5] = 0;
    u8* data = *(u8**)w;
    *(s32*)(data + 0x10) = 0;
    *(s32*)(data + 0x14) = 0;
    *(s32*)(data + 8) = 0;
    *(s32*)(data + 0xC) = 0;
    *(u32*)(w + 0x3294) = 0;
    SetCharacterPerformanceCapToDefault();
}

// ---------------------------------------------------------------- zone memory map

extern u32* gZoneBase;        ORIG(0x803C7434)   // MEM2 load address per zone
extern u8*  gZoneLoaded;      ORIG(0x803C744C)
extern u32* gZoneState;       ORIG(0x803C7464)
u8          gLevelSingleBank; ORIG(0x805FB7E0)   // level flag: one bank of zone memory

#define MEM2_ZONES 0x90400000u

// First zone index of each category; categories are laid out in this order.
static inline s32 CatStart(u32 off) {
    static const u32 kOrder[] = { 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C, 0x80, 0x84, 0x88 };
    s32 n = 1;
    for (u32 i = 0; kOrder[i] != off; i++) n += S(kOrder[i]);
    return n;
}

// 0x80126AB0
// Assign every zone the MEM2 address its .sir data is loaded to. Zone 0 and the first category
// share the start of the area; the five streamed categories alternate between two banks
// (one bank if the level is flagged for it).
extern "C" void WorldLoadLevelZones(s32 level) {
    gLevelSingleBank = (gLevelFlags[(u8)level * 0x20 + 0x18] >> 3) & 1;
    for (s32 i = 0; i < ZoneCount(); i++) {
        gZoneLoaded[i] = 0;
        gZoneState[i] = 0;
        gZoneBase[i] = 0;
    }

    const u32 base = (u32)S(0x00) + MEM2_ZONES;
    const u32 slots = base + (u32)(S(0x34) * S(0x08) + S(0x04));
    const u32 sizes = (u32)(S(0x1C) + (S(0x18) + (S(0x14) + (S(0x0C) + S(0x10)))));

    gZoneBase[0] = base;
    for (s32 i = 1; i <= S(0x68); i++)
        gZoneBase[i] = S(0x34) > 0 ? base + (u32)(S(0x08) * ((i - 1) % S(0x34)) + S(0x04)) : base + (u32)S(0x04);

    for (s32 i = CatStart(0x80); i < CatStart(0x80) + S(0x80); i++) gZoneBase[i] = sizes + slots + (u32)(S(0x24) * S(0x50));
    for (s32 i = CatStart(0x84); i < CatStart(0x84) + S(0x84); i++) gZoneBase[i] = sizes + slots;
    for (s32 i = CatStart(0x88); i < CatStart(0x88) + S(0x88); i++) gZoneBase[i] = slots;

    // Streamed categories: each has its own block inside a bank of `sizes` bytes.
    const s32 step = gLevelSingleBank ? 1 : 2;
    static const u32 kCats[5] = { 0x6C, 0x70, 0x74, 0x78, 0x7C };
    u32 blockOfs = 0;
    for (s32 c = 0; c < 5; c++) {
        s32 first = CatStart(kCats[c]);
        for (s32 i = first; i <= first + S(kCats[c]) - 1; i += step) {
            s32 bank = (i - first) % 2;
            gZoneBase[i] = blockOfs + (slots - (u32)(bank * (s32)sizes));
        }
        blockOfs += (u32)S(0x0C + c * 4);
    }
}
