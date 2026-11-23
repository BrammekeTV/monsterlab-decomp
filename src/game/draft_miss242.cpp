// World / session / entity-link functions (0x8001xxxx, 0x800Bxxxx, 0x8012xxxx, 0x8013xxxx).
#include "types.h"
#include <string.h>
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };

extern "C" {
void  BufferFree(void* p);                                        // 0x801D4980
void* EventPost(s32 id, void* sender);                            // 0x801F3918
u8*   GetGlobalSettings(void);                                    // 0x801D2BF8
void* IdPoolNext(void* pool);                                     // 0x801E6EB0
void  ScopedLockCtor(ScopedLock* l);                              // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                   // 0x801F53B0
s32   WorldGetEntityZone(void* world, void* entity);              // 0x80132458
s32   fn_800130E8(u8* obj, s32 zone, s32 flag);                   // 0x800130E8
void  fn_800B61C0(u8* e);                                         // 0x800B61C0
void  fn_800B7B08(u8* e, u32 a, u32 b);                             // 0x800B7B08
s32   fn_800B7C74(u8* e, u8* src, u32 hash);                      // 0x800B7C74
void  fn_800B7F8C(u8* e, u32 a, u32 b, u32 c);                    // 0x800B7F8C
s32   fn_800B8624(u8* a, u8* b);                                  // 0x800B8624
s32   fn_800B8708(u8* a, u8* b, u32 hash);                        // 0x800B8708
void  fn_800B8A3C(u8* e, s32 idx);                                // 0x800B8A3C
void  fn_800B8D5C(u8* e);                                         // 0x800B8D5C
BOOL  fn_800E6BD4(void);                                          // 0x800E6BD4
void  WorldReset(u8* w, s32 arg);                                // 0x80128500
void  fn_80130A38(u8* w);                                         // 0x80130A38
void  fn_80130B50(u8* w);                                         // 0x80130B50
void  fn_80130FC8(u8* w, void* entity, s32 arg);                  // 0x80130FC8
void  fn_80131500(u8* w);                                         // 0x80131500
void  fn_80131728(u8* w);                                         // 0x80131728
void  fn_80132F04(u8* a);                                          // 0x80132F04
void  fn_80135220(void);                                          // 0x80135220
u8*   fn_80138864(u8* w, s32 idx);                                // 0x80138864
void  fn_801458B0(f32* quat, f32* a, f32* b);                     // 0x801458B0
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);       // 0x80146180
s32   fn_801C0830(u8* zone, u8* box, u8* pos);                    // 0x801C0830
s32   fn_801DA780(u8* a3);                                        // 0x801DA780
u8*   fn_801DA788(u8* a3, s32 i);                                 // 0x801DA788
BOOL  fn_801E6BD0(u8* obj, u32* handle);                          // 0x801E6BD0
u32   fn_801EC880(u8* p, u32 hash);                               // 0x801EC880
BOOL  fn_801F5220(void);                                          // 0x801F5220

extern u8  lbl_80348F38[];   // 0x80348F38 (shader/overlay dispatch table)
extern u8  lbl_803C4298[];   // 0x803C4298
extern u8  lbl_805FB2F3;     // 0x805FB2F3
extern s8  lbl_805FB210;     // 0x805FB210
}

extern "C++" void SessionRestoreZoneObjects(u8* s);   // 0x80012534
extern u8  gGame[];            // 0x803B53A0
extern u8  gIdPool[];          // 0x8048A8F0
extern u8  gWorldState[];      // 0x803E6D00 (+0 holds the world pointer)
extern u8  gZoneLists[];       // 0x803E9FD4 (8 lists of { data, count, owned }, 0x18 apart)
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C
extern "C++" u8  sWorldReady;         // 0x805FB2F0
extern "C++" u8  sReadyPending;       // 0x805FB2F1
extern "C++" u8  sReadyRequested;     // 0x805FB2F2
extern u32 lbl_805FB9A4;       // 0x805FB9A4 (lock flag)

typedef void (*FnV1)(void* self);
typedef void (*FnV2)(void* self, s32 a);
typedef u8*  (*FnP1)(void* self);
typedef s32  (*FnI3)(void* self, s32 a, s32 b);
typedef void (*OverlayFn)(void* h, s32 mode, u32 bits);

// Hashes of the part names that SessionBegin / fn_800B28E0 sort the arm and light objects by.
static const u32 kHashWrestlingArmA     = 0xB90A07A0u;  // "wrestling_armA"
static const u32 kHashWrestlingArmHandA = 0x8E7FD4E3u;  // "wrestling_arm_handA"
static const u32 kHashGooArmA           = 0xB8E87466u;  // "goo_armA"
static const u32 kHashLightBaseA        = 0x5C07B860u;  // "light_baseA"
static const u32 kHashLight02A          = 0x1CE74895u;  // "light_02A"
static const u32 kHashLight01A          = 0x37CA1B56u;  // "light_01A"
static const u32 kHashLight03A          = 0x05FC79D4u;  // "light_03A"
static const u32 kHashArmBolt01A        = 0x93226470u;  // "wrestling_arm_bolt01A"
static const u32 kHashArmBolt02A        = 0xB80F37B3u;  // "wrestling_arm_bolt02A"
static const u32 kHashArmBolt03A        = 0xA11406F2u;  // "wrestling_arm_bolt03A"
static const u32 kHashArmGooPotA        = 0x8CFF596Cu;  // "arm_goo_potA"
static const u32 kHashConnecter         = 0xAAA1B30Bu;  // "connecter"
static const u32 kHashBAse              = 0xF62113D5u;  // "BAse"

static inline s32 GS(s32 off) { return *(s32*)(GetGlobalSettings() + off); }

// Number of entity slots: the sum of the 13 counters at +0x34..+0x64 of the global settings.
static s32 SlotCountAll(void) {
    s32 n = 0;
    for (s32 off = 0x34; off <= 0x64; off += 4) n += GS(off);
    return n;
}

static inline u32 F2U(f32 f) {
    union { f32 f; u32 u; } c;
    c.f = f;
    return c.u;
}

// Zone record for zone index z (zone table pointer at world+0x1A4, gZoneResourceSize each).
static inline u8* ZoneInfo(s32 z) {
    return *(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize;
}

// Zone j-th object: the first *(Z+0x94) come from the static list, the rest from the dynamic list.
static inline u8* ZoneObj(u8* Z, s32 j, u8** rec) {
    s32 n = *(s32*)(Z + 0x94);
    if (j < n) {
        *rec = *(u8**)(Z + 0x8C) + j * 0x40;
        return ((u8**)*(void**)(Z + 0x120))[j];
    }
    *rec = *(u8**)(Z + 0x90) + (j - n) * 0x40;
    return ((u8**)*(void**)(Z + 0x12C))[j - n];
}

// Looks up the zone record for zone number z in the zone table `zones` (the world's data block).
// Zones inside the table's own range are found directly; otherwise the global settings'
// region sizes decide whether z is a valid zone number (NULL if not).
static u8* ZoneLookup(u8* zones, s32 z) {
    if (z > 0 && z <= *(s32*)(zones + 4)) {
        return *(u8**)(zones + 0x1A4) + z * gZoneResourceSize;
    }
    s32 c68 = GS(0x68), c6c = GS(0x6C), c70 = GS(0x70), c74 = GS(0x74);
    s32 c78 = GS(0x78), c7c = GS(0x7C), c80 = GS(0x80), c84 = GS(0x84), c88 = GS(0x88);
    s32 base6 = c68 + c6c + c70 + c74 + c78 + c7c;   // 68..7C
    s32 base7 = base6 + c80;                          // 68..80
    s32 base8 = base7 + c84;                          // 68..84
    bool hit = (c68 + 1 <= z && z <= base6)
            || (base7 + 1 <= z && z <= base7 + c84)
            || (base8 + 1 <= z && z <= base8 + c88)
            || (base6 + 1 <= z && z <= base6 + c80);
    if (!hit) return NULL;
    return *(u8**)(zones + 0x1A4) + z * gZoneResourceSize;
}

// Frees a { data, count, owned } block if it owns its memory (owned 1 or 2), then clears it.
static void FreeBlock(u8* b) {
    void* p = *(void**)b;
    if (p != NULL) {
        s32 owned = *(s32*)(b + 8);
        if (owned == 2) BufferFree(p);
        else if (owned == 1) BufferFree(p);
    }
    *(void**)b = NULL;
    *(s32*)(b + 4) = 0;
    *(s32*)(b + 8) = 0;
}

// Calls overlay dispatch entry for `item`'s type with parameter id and a float value.
static inline void OverlaySetFloat(u8* item, s32 id, f32 v) {
    OverlayFn fn = (OverlayFn) * (void**)(lbl_80348F38 + *(u8*)(item + 0xC) * 4 + 0x24);
    fn(*(void**)(item + 8), id, F2U(v));
}

// Pushes the record's two angle-derived axes and colours to every object of the entity's model.
static void EntityApplyRecord(u8* arg0, u8* rec) {
    u8* model = (*(FnP1*)(*(u8**)arg0 + 0xC))(arg0);
    Quat q;
    Vec3 fwd = { 0.0f, 0.0f, 1.0f };
    Vec3 up = { 0.0f, 1.0f, 0.0f };
    fn_801458B0((f32*)&q, (f32*)&fwd, (f32*)&up);
    Vec3 axisX = { 1.0f, 0.0f, 0.0f };
    fn_80146180(&q, &axisX, 0.63661975f * *(f32*)(rec + 0x18));
    Vec3 axisY = { 0.0f, 1.0f, 0.0f };
    fn_80146180(&q, &axisY, 0.63661975f * *(f32*)(rec + 0x1C));
    for (s32 i = 0; i < fn_801DA780(model); i++) {
        Vec3 v = QuatGetAxisY(&q);
        u8* part = fn_801DA788(model, i);
        *(f32*)(part + 0xA4) = v.x;
        *(f32*)(part + 0xA8) = v.y;
        *(f32*)(part + 0xAC) = v.z;
        if ((*(u32*)(rec + 0x828) & 3) != 3) continue;
        s32 off = 0;
        for (s32 j = 0; j < *(s32*)(*(u8**)(fn_801DA788(model, i) + 0x10) + 0x6C); j++, off += 8) {
            u8* item = *(u8**)(*(u8**)(fn_801DA788(model, i) + 0x10) + off + 0x70);
            if (item == NULL) continue;
            OverlaySetFloat(item, 0x135, *(f32*)(rec + 0x0));
            OverlaySetFloat(item, 0x136, *(f32*)(rec + 0x4));
            OverlaySetFloat(item, 0x137, *(f32*)(rec + 0x8));
            OverlaySetFloat(item, 0x138, 1.0f);
            if ((*(u32*)(rec + 0x828) & 2) == 2) {
                OverlaySetFloat(item, 0x13A, *(f32*)(rec + 0xC));
                OverlaySetFloat(item, 0x13B, *(f32*)(rec + 0x10));
                OverlaySetFloat(item, 0x13C, *(f32*)(rec + 0x14));
                OverlaySetFloat(item, 0x13D, 1.0f);
            }
        }
    }
}

extern "C" {

// 0x80010298
// Applies the world's light/colour records (0x82C bytes each, from world+0x1F8) to an entity's
// model: the first record flagged 0x10, else the record whose id list contains the entity's id.
void EntityUpdate(u8* arg0) {
    u8* world = *(u8**)gWorldState;
    s32 count = *(s32*)(world + 0x1A7C);
    u8* rec = world + 0x1F8;
    for (s32 i = 0; i < count; i++, rec += 0x82C) {
        if (*(u32*)(rec + 0x828) & 0x10) {
            EntityApplyRecord(arg0, rec);
            return;
        }
        s32 n = *(s32*)(rec + 0x824);
        for (s32 k = 0; k < n; k++) {
            if (*(u32*)(*(u8**)(arg0 + 0x9C) + 8) == *(u32*)(rec + 0x24 + k * 4)) {
                EntityApplyRecord(arg0, rec);
                return;
            }
        }
    }
}

// 0x80130418
// Per-frame world pre-update: re-ids stale entity slots, tracks the player's zone, runs the
// sub-updates and flags whether a nearby entity is still loading.
void WorldPreUpdate(void* world) {
    u8* w = (u8*)world;
    if (*(u8*)(w + 0x110) == 0) return;
    *(u32*)(w + 0xBC) &= ~1u;

    for (s32 i = 0; i < SlotCountAll(); i++) {
        u8* e = fn_80138864(w, i);
        if (e == NULL) break;
        u32 handle = *(u32*)(e + 0x20);
        if (fn_801E6BD0(gIdPool, &handle)) {
            *(void**)(e + 0x20) = IdPoolNext(gIdPool);
            if (*(s32*)(e + 4) == 1) *(s32*)(e + 4) = 2;
            else *(s32*)(e + 4) = 0;
        }
    }

    if (*(u8*)(w + 0x110) != 0) {
        void* ent = *(void**)(*(u8**)w + 8);
        s32 z = WorldGetEntityZone(w, ent);
        if (z > 0) {
            if (*(s32*)(w + 0x1A8) != z) {
                *(s32*)(w + 0x1A8) = z;
                *(u8**)(w + 0xE0) = ZoneLookup(*(u8**)w, z);
            }
            if (ent != NULL) *(void**)(*(u8**)w + 0xC) = ent;
            else fn_80130A38(w);
        }
    }

    fn_80135220();
    fn_80130B50(w);
    fn_80130FC8(w, *(void**)(*(u8**)w + 8), 0);
    fn_80131500(w);
    fn_80131728(w);

    *(s32*)(w + 0x114) = 0;
    for (s32 i = 0; i < SlotCountAll(); i++) {
        u8* e = fn_80138864(w, i);
        if (e == NULL) continue;
        s32 zone = *(s32*)(e + 0x10);
        if (zone <= 0) continue;
        if (zone != *(s32*)(w + 0x1A8)
            && !(*(f32*)(*(u8**)(*(u8**)w + 0x1A4) + zone * gZoneResourceSize + 0x20) < 30.0f)) continue;
        if (*(s32*)(e + 8) != *(s32*)(e + 0x10) || *(s32*)(e + 4) != 6) {
            *(s32*)(w + 0x114) = 1;
            return;
        }
    }
}

// 0x8012CDFC
// Re-evaluates whether an entity is still linked into its zone: if its position is no longer
// inside any slot's zone box it is released (virtual 0x30 decides keep/drop) and the entity's
// state flags are cleared. Returns 1 when the entity was dropped via the virtual 0x30 path.
BOOL EntityRefreshLinks(void* entity) {
    u8* e = (u8*)entity;
    if (e == NULL) return 0;
    u8* d = *(u8**)(e + 4);
    if ((*(u32*)(d + 0x20) >> 10) & 1) return 0;
    if (*(s32*)(d + 0x28) <= 0) return 0;

    if (*(s32*)(gWorldState + 0x1A8) != -1 && !((*(u32*)(*(u8**)(e + 4) + 0x1C) >> 10) & 1)) {
        s32 i = 0;
        for (;; i++) {
            if (i >= SlotCountAll()) { i = -1; break; }
            u8* slot = *(u8**)(gWorldState + 0xB0) + i * 0x30;
            if (*(s32*)(slot + 4) != 6) continue;
            u8* zp = ZoneLookup(*(u8**)gWorldState, *(s32*)(slot + 8));
            if (zp != NULL && fn_801C0830(zp, zp + 0xC, e + 0xC8) != 0) break;
        }
        if (i == -1) return 0;
    }

    if (*(u32*)(e + 0xEC) != 0x6C938971u && fn_801F5220()) {
        u32 saved = lbl_805FB9A4;
        lbl_805FB9A4 = 0;
        void* p = EventPost(0x79, e);
        if (p != NULL) *(s32*)((u8*)p + 4) = 0;
        lbl_805FB9A4 = saved;
    }

    if ((*(FnI3*)(*(u8**)e + 0x30))(e, 0, 0) != 0) {
        if ((*(FnP1*)(*(u8**)e + 0xC))(e) != NULL) {
            u8* p = (*(FnP1*)(*(u8**)e + 0xC))(e);
            *(u32*)(p + 8) &= 0xFFFFFFFDu;
        }
        d = *(u8**)(e + 4);
        *(u32*)(d + 0x20) &= 0xFFFDFCFCu;
        d = *(u8**)(e + 4);
        *(u32*)(d + 0x20) |= 0x400u;
        return 1;
    }
    if ((*(FnP1*)(*(u8**)e + 0xC))(e) != NULL) {
        u8* p = (*(FnP1*)(*(u8**)e + 0xC))(e);
        *(u32*)(p + 8) &= 0xFFFFFFFDu;
    }
    d = *(u8**)(e + 4);
    *(u32*)(d + 0x20) &= 0xFFFDF8FCu;
    return 0;
}

// 0x80128C20
// Tears down a loaded world: clears the ready flags and game tables, releases zone objects
// and the shared zone lists, and frees the slot / zone arrays.
void fn_80128C20(u8* arg0) {
    if (*(u8*)(arg0 + 4) == 0) return;

    fn_80132F04(arg0);
    sWorldReady = 0;
    sReadyPending = 0;
    sReadyRequested = 0;
    lbl_805FB2F3 = 0;
    lbl_805FB210 = 0;
    *(u8*)(gGame + 0x1C0) = 0;
    for (u8 i = 0; i != 0xFF; i++) ((s32*)(gGame + 0x1C4))[i] = 0;
    *(u8*)(gGame + 0x13C) = 0;
    for (u8 i = 0; i != 0x20; i++) ((s32*)(gGame + 0x140))[i] = 0;
    WorldReset(arg0, 1);

    if (*(u8*)(arg0 + 0x121) != 0) {
        u8* z0 = *(u8**)(*(u8**)gWorldState + 0x1A4);
        s32 total = *(s32*)(z0 + 0x94) + *(s32*)(z0 + 0x98);
        for (s32 j = 0; j < total; j++) {
            u8* Z = *(u8**)(*(u8**)gWorldState + 0x1A4);
            u8* obj;
            if (j < *(s32*)(Z + 0x94)) obj = ((u8**)*(void**)(Z + 0x120))[j];
            else obj = ((u8**)*(void**)(Z + 0x12C))[j - *(s32*)(Z + 0x94)];
            if (obj == NULL || *(s32*)(obj + 4) == 0) continue;
            (*(FnV1*)(*(u8**)obj + 0x40))(obj);
        }
    }

    *(s32*)(*(u8**)arg0 + 0x8) = 0;
    *(s32*)(*(u8**)arg0 + 0x10) = 0;
    *(s32*)(*(u8**)arg0 + 0x14) = 0;

    if (*(u8*)(arg0 + 0x121) != 0) {
        u8* p = *(u8**)(*(u8**)(*(u8**)arg0 + 0x1B0));
        if (p != NULL) (*(FnV1*)(*(u8**)p + 0x10))(p);

        FreeBlock(arg0 + 0xC8);
        FreeBlock(arg0 + 0xD4);
        for (s32 l = 0; l < 8; l++) FreeBlock(gZoneLists + l * 0x18);
        FreeBlock(arg0 + 0x284);

        if (*(u8*)(arg0 + 5) != 0) {
            for (s32 zi = 0; zi <= *(s32*)(*(u8**)arg0 + 4); zi++) {
                s32 off = zi * gZoneResourceSize;
                if (*(s32*)(*(u8**)(*(u8**)arg0 + 0x1A4) + off + 0x50) == 0) continue;
                bool shared = false;
                for (s32 k = 0; k < *(s32*)(*(u8**)(*(u8**)arg0 + 0x1A4) + 0x98); k++) {
                    u8* Z = *(u8**)(*(u8**)arg0 + 0x1A4);
                    if (*(void**)(Z + off + 0x50) == ((void**)*(void**)(Z + 0x12C))[k]) {
                        shared = true;
                        break;
                    }
                }
                if (shared) continue;
                u8* o = *(u8**)(*(u8**)(*(u8**)arg0 + 0x1A4) + zi * gZoneResourceSize + 0x50);
                (*(FnV1*)(*(u8**)o + 0x40))(o);
                BufferFree(*(void**)(*(u8**)(*(u8**)arg0 + 0x1A4) + zi * gZoneResourceSize + 0x50));
                *(void**)(*(u8**)(*(u8**)arg0 + 0x1A4) + zi * gZoneResourceSize + 0x50) = NULL;
            }
        }
    }

    FreeBlock(arg0 + 0xB0);
    *(void**)(arg0 + 0xE0) = NULL;
    *(u8*)(arg0 + 4) = 0;
}

// 0x800B28E0
// Sorts a freshly loaded zone-1 model's objects by part-name hash into the arm / light /
// bolt slots of the wrestling-arm rig (flagging each as claimed), then wires the rig up.
void fn_800B28E0(u8* a) {
    u8* arm = a;
    u8* hand = a + 8;
    u8* lightBase = a;
    s32 nArm = 0, nBase = 0, nHandLeft = 2, nL2 = 0, nL1 = 0, nL3 = 0;
    u8* z1 = ZoneInfo(1);
    s32 total = *(s32*)(z1 + 0x94) + *(s32*)(z1 + 0x98);

    for (s32 i = 0; i < total; i++) {
        u8* rec;
        u8* obj = ZoneObj(ZoneInfo(1), i, &rec);
        if (!(*(u32*)(rec + 0x1C) & 0x40) || obj == NULL) continue;
        u8* mdl = *(u8**)(obj + 0x9C);
        u32 hash = mdl ? *(u32*)(mdl + 8) : 0;
        switch (hash) {
        case kHashWrestlingArmA:
            if (nArm >= 2) break;
            *(u8**)(arm + 0x98) = obj;
            *(u32*)(obj + 0x15C) |= 0x400000;
            *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
            *(u32*)(obj + 0xF4) = 0x016A38A5;
            fn_800B7F8C(obj, kHashConnecter, 0xC0D24151u, 0xE6A2C82Au);
            arm += 4;
            nArm++;
            break;
        case kHashWrestlingArmHandA:
            if (nHandLeft <= 0) break;
            *(u8**)(hand + 0x9C) = obj;
            nHandLeft--;
            hand -= 4;
            *(u32*)(obj + 0x15C) |= 0x400000;
            *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
            *(u32*)(obj + 0xF4) = 0x016A38A5;
            fn_800B7B08(obj, kHashBAse, 0x07F0CBA7u);
            break;
        case kHashGooArmA:
            *(u8**)(a + 0x94) = obj;
            *(u32*)(obj + 0x15C) |= 0x400000;
            *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
            *(u32*)(obj + 0xF4) = 0x016A38A5;
            fn_800B61C0(obj);
            break;
        case kHashLightBaseA:
            if (nBase < 5) {
                *(u8**)(lightBase + 0xA8) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
                *(u32*)(obj + 0xF4) = 0x016A38A5;
                fn_800B8D5C(obj);
                lightBase += 0x14;
                nBase++;
            } else {
                *(u32*)(obj + 0x15C) |= 0x400000;
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(FnV2*)(*(u8**)obj + 0x18))(obj, 1);
                ScopedLockDtor(&lock, -1);
            }
            break;
        case kHashLight02A:
            if (nL2 < 5) {
                s32 off = nL2 * 0xC;
                nL2++;
                *(u32*)(obj + 0x15C) |= 0x400000;
                *(u8**)(a + off + 0x114) = obj;
            }
            break;
        case kHashLight01A:
            if (nL1 < 5) {
                s32 off = nL1 * 0xC;
                nL1++;
                *(u32*)(obj + 0x15C) |= 0x400000;
                *(u8**)(a + off + 0x10C) = obj;
            }
            break;
        case kHashLight03A:
            if (nL3 < 5) {
                s32 off = nL3 * 0xC;
                nL3++;
                *(u32*)(obj + 0x15C) |= 0x400000;
                *(u8**)(a + off + 0x110) = obj;
            }
            break;
        case kHashArmBolt01A:
            *(u32*)(obj + 0x15C) |= 0x400000;
            *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
            *(u32*)(obj + 0xF4) = 0x016A38A5;
            if (*(u8**)(a + 0x148) == NULL) *(u8**)(a + 0x148) = obj;
            else if (*(u8**)(a + 0x154) == NULL) *(u8**)(a + 0x154) = obj;
            break;
        case kHashArmBolt02A:
            *(u32*)(obj + 0x15C) |= 0x400000;
            *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
            *(u32*)(obj + 0xF4) = 0x016A38A5;
            if (*(u8**)(a + 0x14C) == NULL) *(u8**)(a + 0x14C) = obj;
            else if (*(u8**)(a + 0x158) == NULL) *(u8**)(a + 0x158) = obj;
            break;
        case kHashArmBolt03A:
            *(u32*)(obj + 0x15C) |= 0x400000;
            *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
            *(u32*)(obj + 0xF4) = 0x016A38A5;
            if (*(u8**)(a + 0x150) == NULL) *(u8**)(a + 0x150) = obj;
            else if (*(u8**)(a + 0x15C) == NULL) *(u8**)(a + 0x15C) = obj;
            break;
        case kHashArmGooPotA:
            *(u32*)(obj + 0x15C) |= 0x400000;
            *(u32*)(obj + 0xF0) = mdl ? *(u32*)(mdl + 8) : 0;
            *(u32*)(obj + 0xF4) = 0x016A38A5;
            *(u32*)(obj + 0x15C) |= 0x400000;
            break;
        default:
            *(u32*)(obj + 0x15C) |= 0x400000;
            break;
        }
    }

    *(u32*)(a + 0x1D8) = fn_801EC880(*(u8**)(a + 0x94) + 0x94, 0xE3D96AECu);
    *(u32*)(a + 0x1DC) = fn_801EC880(*(u8**)(a + 0x94) + 0x94, 0x58115C13u);
    *(u32*)(a + 0x1E0) = fn_801EC880(*(u8**)(a + 0x98) + 0x94, 0xC0D24151u);
    *(u32*)(a + 0x1E4) = fn_801EC880(*(u8**)(a + 0x9C) + 0x94, 0xC0D24151u);
    *(u32*)(a + 0x1E8) = fn_801EC880(*(u8**)(a + 0x98) + 0x94, 0xE6A2C82Au);
    *(u32*)(a + 0x1EC) = fn_801EC880(*(u8**)(a + 0x9C) + 0x94, 0xE6A2C82Au);
    for (s32 k = 0; k < 2; k++) fn_800B8A3C(*(u8**)(a + 0x98 + k * 4), k);
    fn_800B7C74(*(u8**)(a + 0xA0), *(u8**)(a + 0x94), 0xE3D96AECu);
    fn_800B7C74(*(u8**)(a + 0xA4), *(u8**)(a + 0x94), 0x58115C13u);
    fn_800B8624(*(u8**)(a + 0x98), *(u8**)(a + 0xA0));
    fn_800B8624(*(u8**)(a + 0x9C), *(u8**)(a + 0xA4));
    fn_800B8708(*(u8**)(a + 0x98), *(u8**)(a + 0x148), 0x1C928056u);
    fn_800B8708(*(u8**)(a + 0x98), *(u8**)(a + 0x14C), 0x8570E657u);
    fn_800B8708(*(u8**)(a + 0x98), *(u8**)(a + 0x150), 0x44FE3997u);
    fn_800B8708(*(u8**)(a + 0x9C), *(u8**)(a + 0x154), 0x1C928056u);
    fn_800B8708(*(u8**)(a + 0x9C), *(u8**)(a + 0x158), 0x8570E657u);
    fn_800B8708(*(u8**)(a + 0x9C), *(u8**)(a + 0x15C), 0x44FE3997u);
}

}  // extern "C"

// Zone number of slot i for SessionBegin: slot count means zone 0; a type-6 slot gives its zone;
// anything else gives -1.
static s32 SessionSlotZone(s32 i) {
    if (i == SlotCountAll()) return 0;
    u8* slot = *(u8**)(gWorldState + 0xB0) + i * 0x30;
    if (*(s32*)(slot + 4) == 6) return *(s32*)(slot + 8);
    return -1;
}

// Pauses/updates a zone object once for the session start (under a lock), then clears its
// "session-pending" flag.
static void SessionReleaseObject(u8* obj) {
    if (!(*(u32*)(obj + 0xB8) & 1)) {
        u32 f = *(u32*)(*(u8**)(obj + 4) + 0x1C);
        if (((f & 0x40) && (*(u32*)(*(u8**)(obj + 4) + 0x20) & 0x80)) || (f & 0x1000)) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(FnV2*)(*(u8**)obj + 0x18))(obj, 0);
            ScopedLockDtor(&lock, -1);
            *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x80;
        }
    }
    *(u32*)(*(u8**)(obj + 4) + 0x20) &= ~0x00200000u;
}

extern "C" {

// 0x80013690
// Begins a play session: if a saved zone state exists, restores it, copies the saved value
// blocks back, releases the objects of every zone, and resets the camera blend.
void SessionBegin(void* session) {
    u8* s = (u8*)session;
    if (*(s32*)(s + 0xA18) != 0) {
        SessionRestoreZoneObjects(s + 0xA10);
        memcpy(s, s + 0xA1C, 0x100);
        memcpy(s + 0x100, s + 0xB1C, 0x100);
        *(s32*)(s + 0xA14) = 0;

        for (s32 i = 0; i < SlotCountAll() + 1; i++) {
            s32 z = SessionSlotZone(i);
            if (z < 0) continue;
            u8* Z0 = ZoneInfo(z);
            s32 total = *(s32*)(Z0 + 0x94) + *(s32*)(Z0 + 0x98);
            for (s32 j = 0; j < total; j++) {
                u8* rec;
                u8* obj = ZoneObj(ZoneInfo(z), j, &rec);
                if (!(*(u32*)(rec + 0x1C) & 0x40) || obj == NULL) continue;
                SessionReleaseObject(obj);
            }
        }

        s32 i = 0;
        for (; i < SlotCountAll() + 1; i++) {
            s32 z = SessionSlotZone(i);
            if (z < 0) continue;
            u8* Z0 = ZoneInfo(z);
            s32 total = *(s32*)(Z0 + 0x94) + *(s32*)(Z0 + 0x98);
            bool found = false;
            for (s32 j = 0; j < total; j++) {
                u8* Z = ZoneInfo(z);
                u8* obj;
                if (j < *(s32*)(Z + 0x94)) obj = ((u8**)*(void**)(Z + 0x120))[j];
                else obj = ((u8**)*(void**)(Z + 0x12C))[j - *(s32*)(Z + 0x94)];
                if (obj != NULL && fn_800130E8(obj, z, 0) == 0) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        for (s32 zi = 0; zi < 2; zi++) {
            u8* Z0 = ZoneInfo(zi);
            s32 total = *(s32*)(Z0 + 0x94) + *(s32*)(Z0 + 0x98);
            for (s32 j = 0; j < total; j++) {
                u8* Z = ZoneInfo(zi);
                u8* obj;
                if (j < *(s32*)(Z + 0x94)) obj = ((u8**)*(void**)(Z + 0x120))[j];
                else obj = ((u8**)*(void**)(Z + 0x12C))[j - *(s32*)(Z + 0x94)];
                if (obj == NULL) continue;
                SessionReleaseObject(obj);
            }
        }

        *(f32*)(s + 0x204) = 0.5f;
        u8* p = *(u8**)(lbl_803C4298 + 4);
        (*(FnV1*)(*(u8**)p + 0x94))(p);
    }

    u8* g = *(u8**)(gGame + 0xD614);
    if (*(s32*)(g + 0xA00) != 0 && !fn_800E6BD4()) *(u8*)(g + 0x9F8) = 1;
}

}  // extern "C"
