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

static WorldParams* sWorldParams;       ORIG(0x805FB944)
s32                 gZoneResourceSize;  ORIG(0x805FB94C)   // bytes per zone record
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
