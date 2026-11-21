#include "types.h"

// World zone-slot state machine: per-slot step (fn_801353C0), zone unload (fn_801360BC) and
// finishing a zone load (fn_8013786C).

extern "C" {
typedef struct ScopedLock { u8 pad[4]; } ScopedLock;
}

extern "C" {
u8* GetGlobalSettings(void);
void ScopedLockCtor(ScopedLock* l);                                                 // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                                      // 0x801F53B0
void* EventPost(s32 id, void* sender);                                              // 0x801F3918
s32 PlaySoundAt(void* sound, int a, const void* pos, int b);                        // 0x80209ABC
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);  // 0x801C80E0
s32 WorldGetEntityZone(void* world, void* entity);                                  // 0x80132458
BOOL ZoneIsLoaded(void* world, s32 zone);                                           // 0x8012F408
void ZoneSlotInit(void* slot);                                                      // 0x801F2B40
void fn_80122B00(void* a, u8* b);                                                   // 0x80122B00
s32 fn_80128318(void* ent, s32 zone, s32 arg);                                      // 0x80128318
s32 fn_80128348(void* ent, s32 zone, s32 arg);                                      // 0x80128348
void fn_80135854(u8* world, s32 slot);                                              // 0x80135854
void fn_80135BC0(u8* world, s32 slot);                                              // 0x80135BC0
void fn_80137684(u8* world, s32 slot);                                              // 0x80137684
void fn_801384B0(u8* world, s32 slot, s32 a);                                       // 0x801384B0
s32 fn_8018CF1C(void* sets, void* widget);                                         // 0x8018CF1C
void fn_801C83AC(u8* mgr, u8* item);                                                // 0x801C83AC
void fn_801DB248(u8* p);                                                            // 0x801DB248
void fn_801E839C(u8* p);                                                            // 0x801E839C
BOOL fn_801F5220(void);                                                             // 0x801F5220
void fn_8020A68C(s32 zone);                                                         // 0x8020A68C
void fn_8013786C(u8* world, s32 slot);                                              // 0x8013786C
void fn_801360BC(u8* world, s32 slot, s32 arg);                                     // 0x801360BC
extern u8 gGame[];                                                                  // 0x803B53A0
extern u8 gWorldState[];                                                            // 0x803E6D00
extern u8 lbl_8047C760[];                                                           // effect manager
extern f32 lbl_805FB924;                                                            // 0x805FB924
extern f32 lbl_805F9008;                                                            // 0x805F9008
}
struct WorldState;
void WorldOnZoneLoaded(WorldState* w, s32 slotIndex);                                // 0x801369C8 (C++ linkage)
extern "C++" s32 gZoneResourceSize;                                                 // 0x805FB94C
extern "C++" void* gScreenSets;                                                     // 0x805FB654

typedef void (*VoidIntFn)(u8*, s32);
typedef u8*  (*GetPtrFn)(u8*);
typedef void (*VoidFn)(u8*);
typedef void (*RespawnFn)(u8*, u8*, s32, s32, s32, f32);

// Sum of the global-settings counters at +lo .. +hi (step 4).
static inline s32 SettingsSum(u32 lo, u32 hi) {
    s32 n = 0;
    for (u32 off = lo; off <= hi; off += 4) n += *(s32*)(GetGlobalSettings() + off);
    return n;
}
// Number of zone slots (13 per-category counters at +0x34 .. +0x64).
static inline s32 SlotCount(void) { return SettingsSum(0x34, 0x64); }
// Number of zones: counters at +0x68 .. +0x98 plus zone 0.
static inline s32 ZoneCount(void) { return 1 + SettingsSum(0x68, 0x98); }

// Slot record (0x30 bytes each, list at world+0xB0), or NULL when the index is out of range.
static inline u8* SlotAt(u8* w, s32 idx) {
    if (idx >= 0 && idx < SlotCount()) return *(u8**)(w + 0xB0) + idx * 0x30;
    return NULL;
}
// Zone number stored in a slot (+0x8), -2 when the index is out of range.
static inline s32 SlotZoneOf(u8* w, s32 idx) {
    if (idx >= 0 && idx < SlotCount()) return *(s32*)(*(u8**)(w + 0xB0) + idx * 0x30 + 8);
    return -2;
}
// Zone resource block, or NULL when the zone number is out of range.
static inline u8* ZoneRes(u8* w, s32 zone) {
    if (zone >= 0 && zone < ZoneCount()) return *(u8**)(*(u8**)w + 0x1A4) + zone * gZoneResourceSize;
    return NULL;
}
// Zone resource block of the global world.
static inline u8* ZoneResG(s32 zone) {
    return *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
}
// Entity i of a zone (statics first, then dynamics); *rec = its 0x40-byte record.
static inline u8* ZoneEnt(u8* z, s32 i, u8** rec) {
    s32 statics = *(s32*)(z + 0x94);
    if (i < statics) {
        *rec = *(u8**)(z + 0x8C) + i * 0x40;
        return (*(u8***)(z + 0x120))[i];
    }
    *rec = *(u8**)(z + 0x90) + (i - statics) * 0x40;
    return (*(u8***)(z + 0x12C))[i - statics];
}

// Effect + sound at an entity's position (model block at entity +0x9C).
static inline void DespawnEffect(u8* e) {
    u8* model = *(u8**)(e + 0x9C);
    if (model == NULL) return;
    f32 p[4] = { *(f32*)(e + 0x18), *(f32*)(e + 0x1C), *(f32*)(e + 0x20), 0.0f };
    SpawnEffectAt(lbl_8047C760, *(u32*)(model + 0x1E4), p, NULL);
    PlaySoundAt(*(void**)(model + 0x1F4), 0, e + 0x18, 0);
}

// Clears bit 1 of the object returned by virtual slot 0xC (when there is one).
static inline void ClearBit1(u8* e) {
    if ((*(GetPtrFn*)(*(u8**)e + 0xC))(e) != NULL) {
        u8* v = (*(GetPtrFn*)(*(u8**)e + 0xC))(e);
        *(u32*)(v + 8) &= 0xFFFFFFFDu;
    }
}

extern "C" {

// 0x801353C0
// Steps one zone slot by its state (slot +0x4); transitions start/finish loading and unloading.
// Marks the world dirty (world +0xBC bit 0) whenever something changed.
void fn_801353C0(u8* self, s32 idx) {
    u8* slot = SlotAt(self, idx);
    if (slot == NULL) return;
    u32 type = *(u32*)(slot + 4);
    if (type > 6) return;
    switch (type) {
    case 0:
        if (*(s32*)(slot + 0x10) > 0) {
            *(s32*)(slot + 8) = *(s32*)(slot + 0x10);
            fn_80135854(self, idx);
            *(u32*)(self + 0xBC) |= 1;
        }
        break;
    case 1:
        if (lbl_805FB924 > *(f32*)(slot + 0x14)) {
            fn_801384B0(self, idx, 0);
            *(u32*)(self + 0xBC) |= 1;
        }
        break;
    case 2: {
        s32 target = *(s32*)(slot + 0x10);
        if (*(s32*)(slot + 8) == target || target == -1) {
            fn_80135BC0(self, idx);
        } else {
            fn_801384B0(self, idx, 0);
        }
        *(u32*)(self + 0xBC) |= 1;
        *(s32*)(slot + 0x1C) = 0;
        break;
    }
    case 3: {
        s32 target = *(s32*)(slot + 0x10);
        if (*(s32*)(slot + 8) == target) {
            u8* s2 = SlotAt(self, idx);
            if (s2 != NULL && *(s32*)(s2 + 4) == 3) {
                *(s32*)(s2 + 4) = 4;
                *(f32*)(s2 + 0x14) = lbl_805FB924 + lbl_805F9008;
            }
            *(u32*)(self + 0xBC) |= 1;
        } else if (target > 0) {
            fn_801360BC(self, idx, 0);
            *(u32*)(self + 0xBC) |= 1;
        }
        break;
    }
    case 4:
        if (*(s32*)(slot + 8) == *(s32*)(slot + 0x10)) {
            WorldOnZoneLoaded((WorldState*)self, idx);
            *(u32*)(self + 0xBC) |= 1;
        } else {
            u8* s2 = SlotAt(self, idx);
            if (s2 != NULL && *(s32*)(s2 + 4) == 4) *(s32*)(s2 + 4) = 3;
            *(u32*)(self + 0xBC) |= 1;
            *(s32*)(slot + 0x1C) = 0;
        }
        break;
    case 6:
        if (*(s32*)(slot + 8) != *(s32*)(slot + 0x10)) {
            fn_8013786C(self, idx);
            if (*(s32*)(slot + 0x10) > 0) fn_801360BC(self, idx, 0);
            *(u32*)(self + 0xBC) |= 1;
        }
        break;
    default:
        break;
    }
}

// 0x801360BC
// Unloads the zone of slot `idx` (zone > 0): parks its entities, lets entities refer to the
// zone clean up, drops game-list entries tied to the zone and resets the zone slot.
void fn_801360BC(u8* self, s32 idx, s32 arg) {
    u8* slot = SlotAt(self, idx);
    s32 zone = SlotZoneOf(self, idx);
    u8* z = ZoneRes(self, SlotZoneOf(self, idx));
    if (z == NULL || zone <= 0) return;

    ScopedLock lock;
    ScopedLockCtor(&lock);

    // Park every entity of the zone: statics, then dynamics.
    for (s32 pass = 0; pass < 2; pass++) {
        s32 n = *(s32*)(z + (pass == 0 ? 0x94 : 0x98));
        for (s32 i = 0; i < n; i++) {
            u8* o = (*(u8***)(z + (pass == 0 ? 0x120 : 0x12C)))[i];
            if (o == NULL) continue;
            u32 fl = *(u32*)(*(u8**)(o + 4) + 0x20);
            if (fl & 1) {
                (*(VoidIntFn*)(*(u8**)o + 0x18))(o, 1);
                *(u32*)(*(u8**)(o + 4) + 0x20) |= 0x800;
            } else if (fl & 0x400) {
                ClearBit1(o);
                *(u32*)(*(u8**)(o + 4) + 0x20) &= 0xFFFFF8FCu;
                *(u32*)(*(u8**)(o + 4) + 0x20) |= 0x20000;
                (*(s32*)(*(u8**)(o + 4) + 0x28))++;
            }
        }
    }

    // Let the zone's entities react; stop at the first that answers 0.
    if (zone >= 0) {
        u8* zr = ZoneResG(zone);
        s32 total = *(s32*)(zr + 0x94) + *(s32*)(zr + 0x98);
        for (s32 i = 0; i < total; i++) {
            u8* zi = ZoneResG(zone);
            u8* ent = (i < *(s32*)(zi + 0x94)) ? (*(u8***)(zi + 0x120))[i]
                                               : (*(u8***)(zi + 0x12C))[i - *(s32*)(zi + 0x94)];
            if (ent != NULL && *(u8**)(ent + 4) != NULL) {
                if (fn_80128318(ent, zone, arg) == 0) break;
            }
        }
    }

    // Is any entity of zone 0 homed in this zone and refusing?
    BOOL refused = FALSE;
    {
        u8* z0 = *(u8**)(*(u8**)gWorldState + 0x1A4);
        s32 total = *(s32*)(z0 + 0x94) + *(s32*)(z0 + 0x98);
        for (s32 i = 0; i < total; i++) {
            u8* zi = *(u8**)(*(u8**)gWorldState + 0x1A4);
            u8* ent = (i < *(s32*)(zi + 0x94)) ? (*(u8***)(zi + 0x120))[i]
                                               : (*(u8***)(zi + 0x12C))[i - *(s32*)(zi + 0x94)];
            if (ent == NULL) continue;
            u8* def = *(u8**)(ent + 4);
            if (def == NULL || *(u16*)(def + 0x30) != (u16)zone) continue;
            if (fn_80128348(ent, zone, zone) == 0) { refused = TRUE; break; }
        }
    }
    if (!refused && ZoneIsLoaded(gWorldState, zone) && zone >= 0) {
        u8* zr = ZoneResG(zone);
        s32 total = *(s32*)(zr + 0x94) + *(s32*)(zr + 0x98);
        for (s32 i = 0; i < total; i++) {
            u8* zi = ZoneResG(zone);
            u8* ent = (i < *(s32*)(zi + 0x94)) ? (*(u8***)(zi + 0x120))[i]
                                               : (*(u8***)(zi + 0x12C))[i - *(s32*)(zi + 0x94)];
            if (ent != NULL && fn_80128348(ent, zone, zone) == 0) break;
        }
    }

    // Drop game-list entries (255 slots at gGame+0x1C4) that belong to this zone.
    for (s32 i = 0; i < 0xFF; i++) {
        u8* item = *(u8**)(gGame + 0x1C4 + i * 4);
        if (item == NULL) continue;
        u8* d = *(u8**)(item + 4);
        if (d != NULL && !((*(u32*)(d + 0x1C) & 2) && zone == *(u16*)(d + 0x32))) continue;
        u8* r = (*(GetPtrFn*)(*(u8**)item + 0xB8))(item);
        if (r != NULL) {
            fn_80122B00(r, item);
        } else {
            *(u8**)(gGame + 0x1C4 + (u8)i * 4) = NULL;
            if ((u8)i == gGame[0x1C0]) gGame[0x1C0] = (u8)(gGame[0x1C0] - 1);
        }
    }

    // Remove the same kind of entries from the 32-slot list at gGame+0x140.
    for (s32 k = 0; k < 0x20; k++) {
        u8* item = *(u8**)(gGame + 0x140 + k * 4);
        if (item == NULL) continue;
        u8* d = *(u8**)(item + 4);
        if (d != NULL && zone != *(u16*)(d + 0x32)) continue;
        for (;;) {
            u8 count = gGame[0x13C];
            s32 found = -1;
            for (u8 j = 0; j < count; j++) {
                if (item == *(u8**)(gGame + 0x140 + j * 4)) { found = j; break; }
            }
            if (found == -1) break;
            *(u8**)(gGame + 0x140 + found * 4) = NULL;
            if (found == gGame[0x13C]) gGame[0x13C] = (u8)(gGame[0x13C] - 1);
        }
    }

    // Per-slot object at world+0xC8: let it react (virtual slot 0x10).
    u8* obj = (*(u8***)(self + 0xC8))[idx];
    if (obj != NULL) (*(VoidFn*)(*(u8**)obj + 0x10))(obj);

    fn_801DB248(z + 0xE4);
    fn_801E839C(*(u8**)(*(u8**)self + 0x1C8) + zone * 0xC);
    ZoneSlotInit(slot);
    ScopedLockDtor(&lock, -1);
}

// 0x8013786C
// Finishes loading the zone of slot `idx` (slot type 6 -> 3): entities whose home zone changed
// are moved (or despawned; the player is respawned), placed effects of the zone are stopped,
// and the zone's screen-set widget is released.
void fn_8013786C(u8* self, s32 idx) {
    u8* slot = SlotAt(self, idx);
    if (slot == NULL || *(s32*)(slot + 4) != 6) return;
    s32 zone = SlotZoneOf(self, idx);
    if (!(zone >= 0 && zone < ZoneCount())) return;
    u8* z = ZoneRes(self, SlotZoneOf(self, idx));
    if (z == NULL || *(s32*)(z + 0x18) != zone) return;

    ScopedLock lockA;
    ScopedLockCtor(&lockA);
    fn_80137684(self, idx);
    *(s32*)(slot + 4) = 3;

    if (zone >= 0) {
        u8* zr = ZoneResG(zone);
        s32 count = *(s32*)(zr + 0x94) + *(s32*)(zr + 0x98);
        for (s32 i = 0; i < count; i++) {
            u8* rec;
            u8* ent = ZoneEnt(ZoneResG(zone), i, &rec);
            if (!(*(u32*)(rec + 0x20) & 1) || ent == NULL) continue;
            u8* def = *(u8**)(ent + 4);
            if (def == NULL) continue;
            if (!(*(u32*)(def + 0x20) & 3) || *(u16*)(def + 0x30) == 0 || (*(u32*)(def + 0x1C) & 0x400)) continue;

            s32 ez = WorldGetEntityZone(gWorldState, ent);
            if (ez > 0 && ez <= *(s32*)(*(u8**)gWorldState + 4) && *(void**)(ent + 0x90) != NULL) {
                if (*(u16*)(def + 0x30) == ez && ZoneIsLoaded(gWorldState, *(u16*)(def + 0x32))) continue;
                ScopedLock lockB;
                ScopedLockCtor(&lockB);
                if (ZoneIsLoaded(gWorldState, ez) && ZoneIsLoaded(gWorldState, *(u16*)(*(u8**)(ent + 4) + 0x32))) {
                    (*(VoidIntFn*)(*(u8**)ent + 0x18))(ent, 0);
                    *(u16*)(*(u8**)(ent + 4) + 0x30) = (u16)ez;
                    (*(VoidIntFn*)(*(u8**)ent + 0x14))(ent, 1);
                } else if (*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x20) {
                    DespawnEffect(ent);
                    (*(VoidIntFn*)(*(u8**)ent + 0x18))(ent, 1);
                    if (!(*(u32*)(*(u8**)(ent + 4) + 0x20) & 0x04000000)) {
                        s32* ev = (s32*)EventPost(0x64, ent);
                        if (ev != NULL) ev[1] = 1;
                    }
                    ClearBit1(ent);
                    u8* d = *(u8**)(ent + 4);
                    *(u32*)(d + 0x20) &= 0xFFFDF8FCu;
                    *(u32*)(*(u8**)(ent + 4) + 0x20) |= 0x20000;
                    *(u8*)(*(u8**)(ent + 4) + 0x35) = 0;
                    (*(s32*)(*(u8**)(ent + 4) + 0x28))++;
                }
                ScopedLockDtor(&lockB, -1);
            } else if (ez == -1) {
                u8* player = *(u8**)(*(u8**)gWorldState + 8);
                if (ent == player) {
                    (*(RespawnFn*)(*(u8**)ent + 0x50))(ent, player, 7, 0, 0, 10.0f * *(f32*)(*(u8**)(player + 4) + 0x24));
                } else if (fn_801F5220() && (*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x20)) {
                    DespawnEffect(ent);
                    ScopedLock lockC;
                    ScopedLockCtor(&lockC);
                    (*(VoidIntFn*)(*(u8**)ent + 0x18))(ent, 2);
                    ScopedLockDtor(&lockC, -1);
                    if (!(*(u32*)(*(u8**)(ent + 4) + 0x20) & 0x04000000)) {
                        s32* ev = (s32*)EventPost(0x64, ent);
                        if (ev != NULL) ev[1] = 0;
                    }
                    ClearBit1(ent);
                    u8* d = *(u8**)(ent + 4);
                    *(u32*)(d + 0x20) &= 0xFFFDF8FCu;
                    *(u32*)(*(u8**)(ent + 4) + 0x20) |= 0x20000;
                    *(u8*)(*(u8**)(ent + 4) + 0x35) = 0;
                    s32 n = *(s32*)(*(u8**)(ent + 4) + 0x28);
                    if (n > 1000) {
                        *(s32*)(*(u8**)(ent + 4) + 0x28) = n + 1;
                        if (*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x10) (*(s32*)(gWorldState + 0xE4))++;
                    }
                }
            }
        }
    }

    // Stop the zone's placed effects.
    for (u32 j = 0; j < *(u32*)(z + 0x9C); j++) {
        u8* fx = (*(u8***)(z + 0x80))[j];
        if (fx != NULL && *(u8**)(fx + 0x1C) != NULL) fn_801C83AC(lbl_8047C760, *(u8**)(fx + 0x1C));
    }
    fn_8020A68C(zone);
    if (*(u8**)(z + 0x100) != NULL) {
        fn_8018CF1C(gScreenSets, *(u8**)(z + 0x100));
        *(u8**)(z + 0x100) = NULL;
    }
    ScopedLockDtor(&lockA, -1);
}

} // extern "C"
