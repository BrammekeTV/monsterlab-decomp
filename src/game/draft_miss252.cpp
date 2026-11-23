// World slot / zone distance helpers, minigame object setup and world reset (0x8008, 0x800A-0x800B,
// 0x8001, 0x8012-0x8013).
#include <string.h>

#include "types.h"
#include "engine/math.h"
#include "game/character.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef u8*  (*GetObjFn)(void* self);              // vtable + 0x10 in the wrestling prop
typedef s32  (*CheckFn)(void* self);               // vtable + 0xF0 on characters
typedef void (*UpdateFn)(void* self, s32 a);       // vtable + 0x18 on zone objects

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32   AnimSetFindClip(void* set, s32 anim);                                // 0x801AC2FC
void  BufferFree(void* p);                                                 // 0x801D4980
void  CharacterFaceDirection(Character* c, const Vec3* dir, BOOL now);     // 0x8005B650
u8*   GetGlobalSettings(void);                                             // 0x801D2BF8
void* IdPoolNext(void* pool);                                              // 0x801E6EB0
void* MemAllocDefault(u32 size);                                           // 0x801D4650
Vec3  PathGetDirection(PathFollower* p);                                   // 0x800D5724
void  PropPlayAnim(void* prop, int a, int b, BOOL once);                   // 0x80069350
u32   RandomU32(void);                                                     // 0x80143460
s32   RaySceneTest(void* ray);                                             // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                                       // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                            // 0x801F53B0
u32   StringHash(const char* s);                                           // 0x801EB704
void  ZoneLog(void* loader, const char* fmt, ...);                         // 0x8000CC30
void  fn_8004CEE8(void*);
void  fn_8004D760(u8* c, u8* monster);
void  fn_80053550(u8* ent, s32 a);                                         // 0x80053550
f32   fn_800B7464(u8* p);                                                  // 0x800B7464
u32   fn_800D63CC(void* path, s32 a);                                      // 0x800D63CC
f32   fn_80131370(u8* world, s32 mode, s32 zone);                          // 0x80131370
void  fn_80131A18(u8* world, s32 slot, s32 flag);                          // 0x80131A18
void  fn_80131A1C(u8* world, s32 slot);                                    // 0x80131A1C
void  fn_80131C2C(u8* world);                                              // 0x80131C2C
void  fn_80132094(void);                                                   // 0x80132094
s32   fn_801333B0(void* world, s32 zone);                                  // 0x801333B0
void  fn_801353C0(u8* world, s32 slot);                                    // 0x801353C0
u8*   fn_80138864(u8* p, s32 idx);                                         // 0x80138864
void  fn_801455E0(f32* out, u8* quat);                                     // 0x801455E0
u8*   fn_801B161C(u8* p, u32 key);                                         // 0x801B161C
u8*   fn_801B1724(u8* p, u32 key);                                         // 0x801B1724
s32   fn_801B5420(u8* a, u8* b, s32 c, f32* d, f32* e, s32 f);            // 0x801B5420
f32   fn_801C065C(const void* a, const void* b, const void* c);           // 0x801C065C
s32   fn_801D3A60(s32 zone);                                               // 0x801D3A60
void  fn_801E4918(u8* obj, s32 a, f32 t);
void  fn_801E5C3C(u8* obj, s32 a, f32 t);
BOOL  fn_801E5E24(u8* p);
u32   fn_801EC880(u8* p, u32 hash);                                        // 0x801EC880
void  fn_801F1A48(u8* p, Vec3* a, Vec3* b);                                // 0x801F1A48
extern const f32 lbl_803460D8[];     // 0x803460D8 (float table indexed by RaySceneTest)
extern const f32 lbl_80346A28[];     // 0x80346A28
extern const f32 lbl_80346A38[];     // 0x80346A38
extern s32 lbl_805FB30C;             // 0x805FB30C (world object count)
extern u8  lbl_8048A8B4[];           // 0x8048A8B4
}

extern u8  gFlag805FB948;            // 0x805FB948
extern u8  gGame[];                  // 0x803B53A0
extern u8  gIdPool[];                // 0x8048A8F0
extern u8  gWorldState[];            // 0x803E6D00
extern u8* gSaveSystem;              // 0x805FBBE0
extern "C++" s32 gZoneResourceSize;  // 0x805FB94C
extern u32 sLoadFrames;              // 0x805FB2F8
extern u8  sWorldReady;              // 0x805FB2F0
extern u8  sReadyPending;            // 0x805FB2F1
extern u8  sReadyRequested;          // 0x805FB2F2
extern u8  sWorldSuspended;          // 0x805FB2F4

static const u32 kHashBAse  = 0xF62113D5u;  // "BAse"
static const u32 kHashHead  = 0x91661B28u;  // "head"
static const u32 kHashNode1 = 0x49D5C738u;  // "node1"

// Total number of world slots: the sum of the 13 per-type counts in the global settings.
static s32 WorldSlotCount(void) {
    u8* s = GetGlobalSettings();
    return *(s32*)(s + 0x48) + *(s32*)(s + 0x44) + *(s32*)(s + 0x40) + *(s32*)(s + 0x3c) +
           *(s32*)(s + 0x38) + *(s32*)(s + 0x64) + *(s32*)(s + 0x60) + *(s32*)(s + 0x5c) +
           *(s32*)(s + 0x58) + *(s32*)(s + 0x54) + *(s32*)(s + 0x50) + *(s32*)(s + 0x34) +
           *(s32*)(s + 0x4c);
}

// Pointer to the zone record with the given index; stride is gZoneResourceSize.
static inline u8* ZoneRecord(u8* world, s32 idx) {
    return *(u8**)(*(u8**)world + 0x1A4) + idx * gZoneResourceSize;
}

// Distance slot of a zone record: +0x24 for mode 1, +0x20 for every other mode.
static inline f32* ZoneDistSlot(u8* world, s32 idx, s32 mode) {
    return (f32*)(ZoneRecord(world, idx) + (mode == 1 ? 0x24 : 0x20));
}

// Releases the pending object slots queued at +0x294 (12 bytes each: flag, slot index, kind byte)
// and clears the queue count at +0x3294.
static void ReleasePendingSlots(u8* w) {
    if (*(s32*)(w + 0x3294) != 0) {
        u8* e = w;
        u32 i;
        for (i = 0; i < *(u32*)(w + 0x3294); i++) {
            if (*(s32*)(e + 0x294) != 0) {
                u8* z = *(u8**)(*(u8**)w + 0x1A4);
                if (*(u8*)(e + 0x29C) == 0) {
                    s32 last = *(s32*)(z + 0x94) - 1;
                    if (*(s32*)(e + 0x298) == last) *(s32*)(z + 0x94) = last;
                    *(void**)(*(u8**)(z + 0x120) + *(s32*)(e + 0x298) * 4) = NULL;
                } else {
                    s32 last = *(s32*)(z + 0x98) - 1;
                    if (*(s32*)(e + 0x298) == last) *(s32*)(z + 0x98) = last;
                    *(void**)(*(u8**)(z + 0x12C) + *(s32*)(e + 0x298) * 4) = NULL;
                }
            }
            e += 0xC;
        }
        *(s32*)(w + 0x3294) = 0;
    }
}

// Applies the colour set for the given mode (2, 3 or 4) to the record at +0x90.
static void SetModeColor(u8* self, s32 mode) {
    u8* c = *(u8**)(self + 0x90);
    if (mode == 2) {
        *(f32*)(c + 0x38) = 0.0f;
        *(f32*)(c + 0x3C) = 0.2f;
        *(f32*)(c + 0x40) = 1.0f;
        *(f32*)(c + 0x44) = 1.0f;
    } else if (mode == 3) {
        *(f32*)(c + 0x38) = 1.0f;
        *(f32*)(c + 0x3C) = 1.0f;
        *(f32*)(c + 0x40) = 0.5f;
        *(f32*)(c + 0x44) = 1.0f;
    } else if (mode == 4) {
        *(f32*)(c + 0x38) = 0.1f;
        *(f32*)(c + 0x3C) = 1.0f;
        *(f32*)(c + 0x40) = 0.1f;
        *(f32*)(c + 0x44) = 1.0f;
    }
}

extern "C" {

// 0x800B61C0
// Resets the aim object: copies its position to +0x264, clears the state fields, looks up the
// ray-scene scale, and moves the entity back along its forward vector by that scale.
void fn_800B61C0(u8* self) {
    u8* pos = *(u8**)(self + 0x4);
    f32 fwd[3];
    f32 scale;
    *(f32*)(self + 0x264) = *(f32*)(pos + 0x0);
    *(f32*)(self + 0x268) = *(f32*)(pos + 0x4);
    *(f32*)(self + 0x26C) = *(f32*)(pos + 0x8);
    scale = lbl_803460D8[RaySceneTest(self)];
    *(f32*)(self + 0x2F4) = scale;
    *(f32*)(self + 0x28C) = 0.0f;
    *(f32*)(self + 0x290) = 1.0f;
    *(f32*)(self + 0x294) = 0.0f;
    *(f32*)(self + 0x298) = 1.0f;
    *(f32*)(self + 0x2A8) = 0.0f;
    *(f32*)(self + 0x2AC) = 0.0f;
    *(f32*)(self + 0x2F0) = 0.0f;
    *(f32*)(self + 0x304) = 0.0f;
    *(s32*)(self + 0x308) = -1;
    *(s32*)(self + 0x2DC) = 0;
    *(s32*)(self + 0x2E0) = 0;
    *(s32*)(self + 0x2E4) = 0;
    *(s32*)(self + 0x2E8) = 0;
    *(s32*)(self + 0x2EC) = 0;
    fn_801455E0(fwd, self + 0x27C);
    pos = *(u8**)(self + 0x4);
    *(f32*)(pos + 0x0) = *(f32*)(self + 0x264) - fwd[0] * scale;
    *(f32*)(pos + 0x4) = *(f32*)(self + 0x268) - fwd[1] * scale;
    *(f32*)(pos + 0x8) = *(f32*)(self + 0x26C) - fwd[2] * scale;
    pos = *(u8**)(self + 0x4);
    *(u32*)(pos + 0x20) |= 0x8000;
    pos = *(u8**)(self + 0x4);
    *(u32*)(pos + 0x20) |= 0x10000000;
    pos = *(u8**)(self + 0x4);
    *(u32*)(pos + 0x20) |= 0x4000;
    pos = *(u8**)(self + 0x4);
    *(u32*)(pos + 0x20) |= 0x10000000;
}

// 0x800ADA70
// Clears the nine "BAse/node1" lookup slots at +0x334..+0x35C, then fills them from the model
// data: slot +0x35C from the "head" node of the model at +0x26C, the rest from node "BAse" of this
// object's own model at +0x8C.
void fn_800ADA70(u8* self) {
    u8* other;
    u8* base;
    *(void**)(self + 0x334) = NULL;
    *(void**)(self + 0x338) = NULL;
    *(void**)(self + 0x33C) = NULL;
    *(void**)(self + 0x340) = NULL;
    *(void**)(self + 0x344) = NULL;
    *(void**)(self + 0x348) = NULL;
    *(void**)(self + 0x34C) = NULL;
    *(void**)(self + 0x350) = NULL;
    *(void**)(self + 0x354) = NULL;
    *(s32*)(self + 0x358) = 0;
    *(void**)(self + 0x35C) = NULL;
    other = *(u8**)(self + 0x26C);
    if (other != NULL) {
        u8* model = *(u8**)(other + 0x8C);
        if (model != NULL) {
            u8* b = fn_801B1724(model, kHashBAse);
            if (b != NULL) {
                *(u8**)(self + 0x35C) = fn_801B161C(b, kHashHead);
            }
        }
    }
    base = *(u8**)(self + 0x8C);
    if (base != NULL) {
        u8* b = fn_801B1724(base, kHashBAse);
        if (b != NULL) {
            *(u8**)(self + 0x334) = fn_801B161C(b, 0x3ED2F7AEu);
            *(u8**)(self + 0x338) = fn_801B161C(b, kHashNode1);
            *(u8**)(self + 0x33C) = fn_801B161C(b, 0xD0DC9682u);
            *(u8**)(self + 0x340) = fn_801B161C(b, 0xA7DBA614u);
            *(u8**)(self + 0x344) = fn_801B161C(b, 0x39BF33B7u);
            *(u8**)(self + 0x348) = fn_801B161C(b, 0x4EB80321u);
            *(u8**)(self + 0x34C) = fn_801B161C(b, 0xD7B1529Bu);
            *(u8**)(self + 0x350) = fn_801B161C(b, 0xA0B6620Du);
            *(u8**)(self + 0x354) = fn_801B161C(b, 0x30097F9Cu);
        }
    }
}

// 0x800B7C74
// Locks onto target `other` (+0x268): seeds the aim point at +0x274 from the target's slot entry
// if it is still zero, builds the offset vectors and asks fn_801B5420 whether the path is clear.
// On success clears bit 0 of the entity flags and returns 1, else 0.
s32 fn_800B7C74(u8* self, u8* other, u32 hash) {
    u32 idx;
    u8* tgt;
    u8* tpos;
    u8* mpos;
    u8* ent;
    f32 d[3];
    f32 v[3];
    f32 dz;

    *(u8**)(self + 0x268) = other;
    idx = fn_801EC880(other + 0x94, hash);
    if (*(f32*)(self + 0x274) == 0.0f && *(f32*)(self + 0x278) == 0.0f && *(f32*)(self + 0x27C) == 0.0f) {
        u8* e = *(u8**)(*(u8**)(self + 0x268) + 0x98) + idx * 0xC8;
        *(f32*)(self + 0x274) = *(f32*)(e + 0x5C);
        *(f32*)(self + 0x278) = *(f32*)(e + 0x60);
        *(f32*)(self + 0x27C) = *(f32*)(e + 0x64);
    }
    tgt = *(u8**)(self + 0x268);
    tpos = *(u8**)(tgt + 0x4);
    d[0] = *(f32*)(self + 0x274) - *(f32*)(tpos + 0x0);
    d[1] = *(f32*)(self + 0x278) - *(f32*)(tpos + 0x4);
    dz = *(f32*)(self + 0x27C) - *(f32*)(tpos + 0x8);
    dz = dz - fn_800B7464(tgt);
    d[2] = dz;
    mpos = *(u8**)(self + 0x4);
    ent = *(u8**)(self + 0x98) + *(s32*)(self + 0x26C) * 0xC8;
    v[0] = *(f32*)(ent + 0x5C) - *(f32*)(mpos + 0x0);
    v[1] = *(f32*)(ent + 0x60) - *(f32*)(mpos + 0x4);
    v[2] = *(f32*)(ent + 0x64) - *(f32*)(mpos + 0x8);
    if (fn_801B5420(*(u8**)(self + 0x268), self, *(s32*)(self + 0x270), d, v, 1) == 1) {
        u8* p = *(u8**)(self + 0x4);
        *(u32*)(p + 0x20) &= 0xFFFFFFFEu;
        return 1;
    }
    return 0;
}

// 0x800B7F8C
// Starts the arm-extension attack: stores the start position at +0x2D4, the two slot indices
// (looked up by hash) at +0x290/+0x294 and the id at +0x298, plays "wrestling_arm_extension" if
// the model has an animation set, and initialises the motion state from the distance between
// the two slots' x coordinates.
void fn_800B7F8C(u8* self, s32 id, u32 hashA, u32 hashB) {
    u8* pos = *(u8**)(self + 0x4);
    u8* model;
    u8* slots;
    f32 dx;
    f32 len2;
    f32 len;
    f32 dist;

    *(f32*)(self + 0x2D4) = *(f32*)(pos + 0x0);
    *(f32*)(self + 0x2D8) = *(f32*)(pos + 0x4);
    *(f32*)(self + 0x2DC) = *(f32*)(pos + 0x8);
    *(u32*)(self + 0x290) = fn_801EC880(self + 0x94, hashA);
    model = *(u8**)(self + 0x9C);
    *(u32*)(self + 0x294) = fn_801EC880(self + 0x94, hashB);
    *(s32*)(self + 0x298) = id;
    *(f32*)(self + 0x27C) = 1.0f;
    *(f32*)(self + 0x280) = 0.0f;
    *(f32*)(self + 0x284) = 0.0f;
    *(f32*)(self + 0x288) = 1.0f;
    if (model != NULL && *(void**)(model + 0x48) != NULL) {
        u8* r;
        u8* a;
        u8* b;
        PropPlayAnim(self, AnimSetFindClip(*(void**)(model + 0x48), StringHash("wrestling_arm_extension")), 0, 1);
        r = (*(GetObjFn*)(*(u8**)self + 0x10))(self);
        a = *(u8**)r;
        b = *(u8**)(a + 0x14);
        *(s32*)(b + 0x20) = 1;
    }
    slots = *(u8**)(self + 0x98);
    dx = *(f32*)(slots + *(s32*)(self + 0x290) * 0xC8 + 0x5C) - *(f32*)(slots + *(s32*)(self + 0x294) * 0xC8 + 0x5C);
    len2 = dx * dx;
    len = (len2 <= 0.0f) ? 0.0f : sqrtf(len2);
    dist = fabsf(len);
    *(f32*)(self + 0x2B4) = 0.0f;
    *(f32*)(self + 0x2B0) = 0.7f;
    *(f32*)(self + 0x2AC) = dist;
    *(f32*)(self + 0x2A0) = dist;
    *(f32*)(self + 0x29C) = dist;
    *(f32*)(self + 0x2B8) = 0.0f;
    *(f32*)(self + 0x2C8) = 0.0f;
    *(f32*)(self + 0x2CC) = 0.0f;
    *(f32*)(self + 0x2D0) = 0.0f;
    *(u8*)(self + 0x2E8) = 0;
    *(f32*)(self + 0x2EC) = 0.0f;
    *(u8*)(self + 0x2C4) = 0;
}

// 0x80131500
// Zone-streaming check on the current zone object: it only walks the neighbouring zone
// distances (calling fn_801D3A60 / fn_80131370); every comparison result is unused in this build.
void fn_80131500(u8* self) {
    u8* w;
    u8* x;
    u8* y;
    s32 cur;
    s32 zone;
    f32 best;

    if (*(u8*)(self + 0x110) == 0) return;
    if (gFlag805FB948 == 0) return;
    cur = *(s32*)(self + 0x1A8);
    if (cur <= 0) return;
    w = *(u8**)self;
    x = *(u8**)(w + 0x8);
    if (x == NULL) return;
    y = *(u8**)(x + 0x4);
    if (y == NULL) return;
    zone = *(u16*)(y + 0x30);
    best = 30.0f;
    if (zone > 0 && x != *(u8**)(w + 0xC) && cur != zone) {
        s32 slot = fn_801D3A60(zone);
        s32 id;
        s32 prev;
        if (slot >= 0 && slot < lbl_805FB30C) {
            u8* e = *(u8**)(gWorldState + 0xB0) + slot * 0x30;
            id = (*(s32*)(e + 0x28) != 0) ? *(s32*)(e + 0x8) : -1;
        } else {
            id = -1;
        }
        if (id <= 0) return;
        if (id != *(s32*)(*(u8**)(self + 0xB0) + slot * 0x30 + 0x10) || id != zone) best = -1.0f;
        prev = *(s32*)(self + 0x1A8) - 2;
        if (prev > 0 && prev == zone - 1) {
            f32 d = fn_80131370(self, 0, prev);
            if (best >= d) best = d;
        }
        // The following next-zone distance comparison has no effect in the original.
        return;
    }
    if (zone > 0 && *(s32*)(self + 0x1A8) == zone) {
        s32 prev = *(s32*)(self + 0x1A8) - 1;
        f32 dist = 30.0f;
        if (prev > 0) {
            f32 d = fn_80131370(self, 0, prev);
            if (dist >= d) dist = d;
        }
        // The following next-zone distance comparison has no effect in the original.
    }
}

// 0x800130E8
// Per-frame update of a scripted character while its record in the game table is active: copies
// the saved state, snaps to the path waypoint, advances the path counter, and fires the combat /
// junction teleport hooks. Always returns 1.
s32 fn_800130E8(u8* self, s32 a, s32 b) {
    s32 idx = *(s32*)(gGame + 0xFDC);
    if (*(u32*)(*(u8**)(self + 0x4) + 0x1C) & 0x10) {
        if (*(void**)(self + 0x2C) == *(void**)(gGame + 0x10000 + idx * 4 - 0x5738)) {
            void* path;
            fn_80053550(self, *(s32*)(gGame + 0x10000 + idx * 4 - 0x54B8));
            memcpy(self + 0x87C, gGame + 0x10000 + idx * 0x1C - 0x56E8, 0x1C);
            path = *(void**)(self + 0xA00);
            if (path != NULL) {
                Vec3* p;
                u8* pos;
                Vec3 dir;
                if (PathAtWaypoint((PathFollower*)path)) PathSetMoving((PathFollower*)*(void**)(self + 0xA00), 0);
                p = PathGetPosition((PathFollower*)*(void**)(self + 0xA00));
                pos = *(u8**)(self + 0x4);
                *(f32*)(pos + 0x0) = p->x;
                *(f32*)(pos + 0x4) = p->y;
                *(f32*)(pos + 0x8) = p->z;
                dir = PathGetDirection((PathFollower*)*(void**)(self + 0xA00));
                CharacterFaceDirection((Character*)self, &dir, 1);
            }
            path = *(void**)(self + 0xA00);
            if (path != NULL && !PathAtWaypoint((PathFollower*)path)) {
                if (*(u8*)(self + 0x894) == 0) {
                    *(s32*)(self + 0x88C) = *(s32*)(self + 0x88C) + 1;
                } else {
                    s32 v = *(s32*)(self + 0x88C) - 1;
                    *(s32*)(self + 0x88C) = v;
                    if (v < 0) {
                        s32 k = *(s32*)(self + 0x880);
                        if (k == 0) {
                            *(s32*)(self + 0x88C) = *(s32*)(self + 0x890) - 1;
                        } else if ((u32)(k - 1) <= 1u) {
                            *(s32*)(self + 0x88C) = 0;
                        }
                    }
                }
            }
            if (*(u32*)(*(u8**)(self + 0x4) + 0x1C) & 0x2000) {
                u32 fl = *(u32*)(*(u8**)(self + 0x4) + 0x20);
                if (!(fl & 0x80) &&
                    (*(void**)(self + 0x2C) != *(void**)(gSaveSystem + 0x130) || *(s32*)(gSaveSystem + 0x138) == 2) &&
                    (fl & 0x800) && (*(CheckFn*)(*(u8**)self + 0xF0))(self) == 0) {
                    fn_8004D760(self, gGame + 0x10000 + idx * 0x13C - 0x5468);
                }
                if ((void*)self == *(void**)(gGame + 0x10000 - 0x29EC) && *(s32*)(gSaveSystem + 0x138) == 2) {
                    u32 j = fn_800D63CC(*(void**)(self + 0xA00), 0);
                    if (j != 0) TeleportPlayerToJunction(j);
                }
            }
            *(s32*)(gGame + 0xFDC) = *(s32*)(gGame + 0xFDC) + 1;
            *(u32*)(*(u8**)(self + 0x4) + 0x20) &= 0xFFDFFFFFu;
        }
    }
    return 1;
}

// 0x80131728
// Refreshes every world slot: slots whose entry value (+0x10) is positive are processed first
// (and marked in a temporary byte table), then all remaining slots are processed.
void fn_80131728(u8* self) {
    s32 n = WorldSlotCount();
    u8* seen = (u8*)MemAllocDefault((u32)n);
    s32 i;
    memset(seen, 0, (size_t)n);
    for (i = 0; i < WorldSlotCount(); i++) {
        if (*(s32*)(*(u8**)(self + 0xB0) + i * 0x30 + 0x10) > 0) {
            fn_801353C0(self, i);
            seen[i] = 1;
        }
    }
    for (i = 0; i < WorldSlotCount(); i++) {
        if (seen[i] == 0) fn_801353C0(self, i);
    }
    if (seen != NULL) BufferFree(seen);
}

// 0x80130FC8
// Computes, for every zone, its distance (field +0x20, or +0x24 when mode is 1) to the entity's
// position: the entity's own zone first, then its neighbour zones, then any zone still at the
// 1e8 "unset" value.
void fn_80130FC8(u8* world, u8* ent, s32 mode) {
    s32 i;
    s32 zone;
    s32 k;
    f32 d;
    if (ent == NULL) return;
    for (i = 1; i <= *(s32*)(*(u8**)world + 0x4); i++) {
        *ZoneDistSlot(world, i, mode) = 1e8f;
    }
    zone = *(u16*)(*(u8**)(ent + 0x4) + 0x30);
    {
        u8* zr = ZoneRecord(world, zone);
        d = fn_801C065C(zr, zr + 0xC, *(u8**)(ent + 0x4));
    }
    *ZoneDistSlot(world, zone, mode) = d;
    for (k = 0; k < *(s32*)(GetGlobalSettings() + 0x34); k++) {
        u8* nb = *(u8**)(ZoneRecord(world, zone) + 0x114) + k * 0x54;
        s32 n = *(s32*)nb;
        if (n > 0) {
            if (*(u32*)(nb + 0x4) & 2) {
                Vec3 lo;
                Vec3 hi;
                fn_801F1A48(nb + 0x8, &hi, &lo);
                d = fn_801C065C(&hi, &lo, *(u8**)(ent + 0x4));
            } else {
                u8* zr = ZoneRecord(world, n);
                d = fn_801C065C(zr, zr + 0xC, *(u8**)(ent + 0x4));
            }
            *ZoneDistSlot(world, n, mode) = d;
        }
    }
    for (i = 1; i <= *(s32*)(*(u8**)world + 0x4); i++) {
        if (*ZoneDistSlot(world, i, mode) == 1e8f) {
            u8* zr = ZoneRecord(world, i);
            d = fn_801C065C(zr, zr + 0xC, *(u8**)(ent + 0x4));
            *ZoneDistSlot(world, i, mode) = d;
        }
    }
}

// 0x8008D29C
// Re-rolls the animation mode (state at +0x268: 2, 3 or 4) when the flag at +0x274 is set, never
// repeating the current one, sets its colour, and picks a new random value at +0x278 between the
// two limits for the stored index (+0x2DC).
void fn_8008D29C(u8* self) {
    if (*(u8*)(self + 0x274) != 0) {
        u32 cur;
        u32 r;
        s32 idx;
        f32 lo;
        f32 hi;
        f32 t;
        switch (*(s32*)(self + 0x268)) {
            case 2: cur = 0; break;
            case 3: cur = 1; break;
            case 4: cur = 2; break;
            default: cur = 10; break;
        }
        do {
            r = RandomU32() % 3;
        } while (r == cur);
        if (r == 0) {
            *(s32*)(self + 0x268) = 2;
            SetModeColor(self, 2);
        } else if (r == 1) {
            *(s32*)(self + 0x268) = 3;
            SetModeColor(self, 3);
        } else if (r == 2) {
            *(s32*)(self + 0x268) = 4;
            SetModeColor(self, 4);
        }
        idx = *(s32*)(self + 0x2DC);
        hi = lbl_80346A38[idx];
        lo = lbl_80346A28[idx];
        t = 2.3283064e-10f * (f32)RandomU32();
        *(f32*)(self + 0x278) = lo + (hi - lo) * t;
    }
}

// 0x80130B50
// Resolves the world-slot values (+0x10 of each 0x30-byte entry at +0xB0): clears the ones flagged
// for reset, then fills type-6 and type-0 entries from the zone's neighbour table (+0x114 of
// the zone at +0xE0) or from the queued list at +0x284.
void fn_80130B50(u8* self) {
    s32 i;
    s32 j;
    if (*(s32*)(self + 0x1A8) <= 0) return;
    if (*(void**)(self + 0xE0) == NULL) return;
    for (i = 0; i < WorldSlotCount(); i++) {
        u8* e = *(u8**)(self + 0xB0) + i * 0x30;
        if (*(u32*)(e + 0xC) & 0x10) *(s32*)(e + 0x10) = -1;
    }
    for (i = 0; i < WorldSlotCount(); i++) {
        u8* e = *(u8**)(self + 0xB0) + i * 0x30;
        if (*(s32*)(e + 0x4) == 6 && *(s32*)(e + 0x10) < 0) {
            u8* o = *(u8**)e;
            s32 kind = (o != NULL) ? *(s32*)o : 0;
            if (kind == 0) {
                s32 id = *(s32*)(e + 0x8);
                if (*(s32*)(self + 0x1A8) == id) {
                    *(s32*)(e + 0x10) = id;
                } else {
                    for (j = 0; j < *(s32*)(GetGlobalSettings() + 0x34); j++) {
                        s32 z = *(s32*)(*(u8**)(*(u8**)(self + 0xE0) + 0x114) + j * 0x54);
                        if (z > 0 && i == fn_801D3A60(z)) {
                            u8* e2 = *(u8**)(self + 0xB0) + i * 0x30;
                            *(s32*)(e2 + 0x10) = *(s32*)(e2 + 0x8);
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < *(s32*)(self + 0x290); i++) {
        s32 slot = fn_801D3A60(*(s32*)(*(u8**)(self + 0x284) + i * 0x10));
        u8* q = *(u8**)(self + 0x284) + i * 0x10;
        if (*(s32*)(q + 0xC) != 0 || *(s32*)(*(u8**)(self + 0xB0) + slot * 0x30 + 0x10) < 0) {
            u8* e = *(u8**)(self + 0xB0) + slot * 0x30;
            *(s32*)(e + 0x10) = *(s32*)q;
            e = *(u8**)(self + 0xB0) + slot * 0x30;
            *(u32*)(e + 0xC) &= 0xFFFFFFEFu;
        }
    }
    for (i = 0; i < WorldSlotCount(); i++) {
        u8* e = *(u8**)(self + 0xB0) + i * 0x30;
        if (*(s32*)(e + 0x10) < 0) {
            u8* o = *(u8**)e;
            s32 kind = (o != NULL) ? *(s32*)o : 0;
            if (kind == 0) {
                if (i == fn_801D3A60(*(s32*)(self + 0x1A8))) {
                    *(s32*)(*(u8**)(self + 0xB0) + i * 0x30 + 0x10) = *(s32*)(self + 0x1A8);
                } else {
                    for (j = 0; j < *(s32*)(GetGlobalSettings() + 0x34); j++) {
                        s32 z = *(s32*)(*(u8**)(*(u8**)(self + 0xE0) + 0x114) + j * 0x54);
                        if (z > 0 && i == fn_801D3A60(z)) {
                            *(s32*)(*(u8**)(self + 0xB0) + i * 0x30 + 0x10) =
                                *(s32*)(*(u8**)(*(u8**)(self + 0xE0) + 0x114) + j * 0x54);
                        }
                    }
                }
            }
        }
    }
}

// 0x80128500
// World reset: waits (pumping the id pool and loader) until every type-1 object has been
// re-registered, releases the queued slots, runs the type-6 slot reset and the zone-object
// update hooks, then re-resolves the type-3 and type-6 slots and clears the ready/suspended state.
void WorldReset(u8* self, s32 flag) {
    s32 first = 1;
    s32 busy = 1;
    s32 i;

    while (busy != 0) {
        busy = 0;
        for (i = 0; i < WorldSlotCount(); i++) {
            u8* e = fn_80138864(self, i);
            if (e != NULL) {
                s32 pending = 0;
                if (*(s32*)(e + 0x4) == 1 && *(void**)(e + 0x20) != IdPoolNext(gIdPool)) pending = 1;
                if (pending != 0) busy = 1;
            }
        }
        if (fn_801E5E24(gIdPool) == 0) busy = 0;
        if (busy != 0) {
            if (first != 0) {
                ZoneLog(gIdPool, "World::Reset waiting...");
                first = 0;
            }
            fn_801E4918(lbl_8048A8B4, 1, 0.01f);
            fn_801E5C3C(gIdPool, 1, 0.01f);
        }
    }
    if (first == 0) ZoneLog(gIdPool, "World::Reset wait finished");
    fn_8004CEE8(NULL);
    ReleasePendingSlots(self);
    for (i = 0; i < WorldSlotCount(); i++) {
        if (*(s32*)(*(u8**)(gWorldState + 0xB0) + i * 0x30 + 0x4) == 6) fn_80131A1C(gWorldState, i);
    }
    {
        u8* z = *(u8**)(*(u8**)gWorldState + 0x1A4);
        s32 total = *(s32*)(z + 0x94) + *(s32*)(z + 0x98);
        s32 off4 = 0;
        s32 off40 = 0;
        for (i = 0; i < total; i++) {
            u8* obj;
            u8* hdr;
            z = *(u8**)(*(u8**)gWorldState + 0x1A4);
            if (i < *(s32*)(z + 0x94)) {
                obj = *(u8**)(*(u8**)(z + 0x120) + off4);
                hdr = *(u8**)(z + 0x8C) + off40;
            } else {
                s32 k = i - *(s32*)(z + 0x94);
                obj = *(u8**)(*(u8**)(z + 0x12C) + k * 4);
                hdr = *(u8**)(z + 0x90) + (k << 6);
            }
            if ((*(u32*)(hdr + 0x1C) & 0x80) && obj != NULL && *(s32*)(obj + 0x4) != 0) {
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(UpdateFn*)(*(u8**)obj + 0x18))(obj, 1);
                ScopedLockDtor(&lock, -1);
            }
            off40 += 0x40;
            off4 += 4;
        }
    }
    for (i = 0; i < WorldSlotCount(); i++) {
        u8* e = *(u8**)(gWorldState + 0xB0) + i * 0x30;
        if (*(s32*)(e + 0x4) == 3) {
            if (fn_801333B0(gWorldState, *(s32*)(*(u8**)(e + 0x28) + 0x18)) == 0) {
                fn_80131A18(gWorldState, i, flag != 0);
            }
        }
    }
    for (i = 0; i < WorldSlotCount(); i++) {
        u8* e = *(u8**)(gWorldState + 0xB0) + i * 0x30;
        if (*(s32*)(e + 0x4) == 6) {
            if (fn_801333B0(gWorldState, *(s32*)(*(u8**)(e + 0x28) + 0x18)) == 0) {
                fn_80131A18(gWorldState, i, flag != 0);
            }
        }
    }
    ReleasePendingSlots(self);
    fn_80131C2C(self);
    fn_80132094();
    sWorldReady = 0;
    sReadyPending = 0;
    sReadyRequested = 0;
    sWorldSuspended = 0;
    sLoadFrames = 0;
}

}  // extern "C"
