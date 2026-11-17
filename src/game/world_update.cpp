// Per-frame world update: entity activation and culling, zone streaming, time steps,
// particle systems.
// Original: WorldTick 0x8012AB00 (auto_03_801268C8_text.s), 8.8 KB.
//
// The world keeps one table per loaded zone. Each zone lists its static entities (records at
// +0x8C, objects at +0x120) and dynamic ones (records at +0x90, objects at +0x12C). Records are
// 0x40 bytes and share the CharacterDef layout (flags at +0x1C, state flags at +0x20).
// Zone slots come from the world's slot table (+0xB0, 0x30 bytes each, type 6 = zone); the
// number of slots is the sum of 13 counters in the global settings block, plus zone 0.
#include "game/character.h"
#include "game/game.h"

extern "C" {
u8*   GetGlobalSettings(void);                                  // 0x801D2BF8 (was GetGlobalSettings)
void  WorldPreUpdate(void* world);                              // 0x80130418
void  WorldIdleTick(void);                                      // 0x801F5534
void  EventMgrTick(void* mgr, f32 dt);                          // 0x801BA4B4
void  EventMgrFlushFor(void* mgr, void* entity);                // 0x801BA108
void* EventPost(s32 id, void* target);                          // 0x801F3918: returns payload
BOOL  fn_801F5220(void);
void  EntityRefreshLinks(void* entity);                         // 0x8012CDFC
s32   WorldGetEntityZone(void* world, void* entity);            // 0x80132458
BOOL  ZoneIsLoaded(void* world, s32 zone);                      // 0x8012F408
void  SpawnEffectAt(void* fxMgr, void* effect, const f32 posAndScale[4], int a);   // 0x801C80E0
void  PlaySoundAt(void* sound, int a, const Vec3* pos, int b);  // 0x80209ABC
BOOL  ScriptIsPaused(void* script);                             // 0x8000B8A0
BOOL  ScriptIsStopped(void* script);                            // 0x8000B8AC
BOOL  EntityIsFrozen(void* entity);                             // 0x801B70E8
void  EntitySyncTransform(void* entity, int a);                 // 0x801B4F4C
void  fn_8012F340(void);
void  WorldPostUpdate(void* world);                             // 0x8012DA58
u8*   ParticleSystemFirst(void* list);                          // 0x80169838
void  ParticleEmitterUpdate(u8* system, void* emitter, void* params, f32 dt);   // 0x80174DF4
void  ParticleSystemUpdateExtra(u8* system, f32 dt);            // 0x80174FF0
void  ZoneEffectsUpdate(void* effects, f32 dt);                 // 0x801E97FC
void  fn_800E03F0(void* p, int a);
void  GameUpdateWorld(void* game, f32 dt);                      // 0x80006EF4
void  CameraFollowReset(void* follow);                          // 0x8000733C
BOOL  ScoreCullCandidate(void* entity, s32 zone, void* follow); // 0x80007464
BOOL  ScoreActivateCandidate(void* entity, s32 zone, void* follow);   // 0x80006F48
}
struct ScopedLock { u8 pad[4]; };
extern "C" void ScopedLockCtor(ScopedLock* l);                  // 0x801F5384
extern "C" void ScopedLockDtor(ScopedLock* l, s32 flags);       // 0x801F53B0

extern u8    gWorldState[];          // 0x803E6D00
extern u8    gGame[];                // 0x803B53A0
extern u8    gScriptCtx[];           // 0x803C4298
extern void* gEventMgr;              // 0x805FB6C8
extern s32   gZoneResourceSize;      // 0x805FB94C
extern u8    gFlag805FB948;          // 0x805FB948
extern f32   lbl_805F95A0;           // time scale (slow motion < 1)
extern f32   lbl_805FB300;           // cull budget
extern f32   lbl_805FB91C;           // event manager time step
extern void* lbl_805FB6D0;
extern void* lbl_805FB5AC;           // particle list pending an update outside zones
extern u8    lbl_8047C760[];         // effect manager
extern u32   lbl_805FB9A4;

static u32   sLoadFrames;            ORIG(0x805FB2F8)
u8           sWorldReady;            ORIG(0x805FB2F0)
static u8    sReadyPending;          ORIG(0x805FB2F1)
static u8    sReadyRequested;        ORIG(0x805FB2F2)
static u8    sWorldSuspended;        ORIG(0x805FB2F4)
static f32   sNextCullTime;          ORIG(0x805FB2FC)
static f32   sLastCullTime;          ORIG(0x805FB304)

// ---------------------------------------------------------------- world layout

struct WorldData {
    u8    pad00[4];
    s32   zoneCount;        // 0x04: zones 0..zoneCount
    void* player;           // 0x08
    u8    pad0C[0x198];
    u8*   zones;            // 0x1A4: gZoneResourceSize bytes each
    u8    pad1A8[0x20];
    u8*   zoneEffects;      // 0x1C8: 0xC bytes per zone
};
struct ZoneSlot { u8 pad00[4]; s32 type; s32 zone; u8 pad0C[0x24]; };   // 0x30 bytes
struct PendingRemoval { void* obj; s32 index; u8 dynamic; u8 pad[3]; };  // 0xC bytes

struct WorldState {
    WorldData* data;        // 0x000
    u8         pad004[4];
    f32        time;        // 0x008
    f32        dt;          // 0x00C
    u8         pad010[0xA0];
    ZoneSlot*  slots;       // 0x0B0
    u8         pad0B4[0x30];
    s32        despawned;   // 0x0E4
    u8         pad0E8[0x24];
    s32        idleFrames;  // 0x10C
    u8         pad110[4];
    s32        loading;     // 0x114
    u8         pad118[0x90];
    s32        loadZone;    // 0x1A8
    u8         pad1AC[0xE8];
    PendingRemoval pending[0x400];   // 0x294
    u32        pendingCount;         // 0x3294
};

static inline u8* ZoneInfo(s32 zone) {
    return ((WorldState*)gWorldState)->data->zones + zone * gZoneResourceSize;
}
static inline s32  ZoneStaticCount(u8* z)  { return *(s32*)(z + 0x94); }
static inline s32  ZoneDynamicCount(u8* z) { return *(s32*)(z + 0x98); }

// Sum of the 13 per-category slot counters in the global settings block (+0x34 .. +0x64).
static inline s32 ZoneSlotCount(void) {
    s32 n = 0;
    for (u32 off = 0x34; off <= 0x64; off += 4) n += *(s32*)(GetGlobalSettings() + off);
    return n;
}

// Zone index for slot i: the last slot is zone 0, others must be zone slots (type 6).
static inline s32 SlotZone(s32 slot) {
    if (slot == ZoneSlotCount()) return 0;
    ZoneSlot* s = &((WorldState*)gWorldState)->slots[slot];
    return s->type == 6 ? s->zone : -1;
}

// Entity `i` of a zone: statics first, then dynamics.
static inline Character* ZoneEntity(u8* z, s32 i, CharacterDef** rec) {
    s32 statics = ZoneStaticCount(z);
    if (i < statics) {
        *rec = (CharacterDef*)(*(u8**)(z + 0x8C) + i * 0x40);
        return (*(Character***)(z + 0x120))[i];
    }
    *rec = (CharacterDef*)(*(u8**)(z + 0x90) + (i - statics) * 0x40);
    return (*(Character***)(z + 0x12C))[i - statics];
}

// Visit every entity of every zone slot. The visitor returns FALSE to stop everything.
template <typename F> static inline void ForEachZoneEntity(BOOL staticsOnly, F visit) {
    for (s32 slot = 0; slot < ZoneSlotCount() + 1; slot++) {
        s32 zone = SlotZone(slot);
        if (zone < 0) continue;
        u8* z = ZoneInfo(zone);
        s32 count = staticsOnly ? ZoneStaticCount(z) : ZoneStaticCount(z) + ZoneDynamicCount(z);
        for (s32 i = 0; i < count; i++) {
            CharacterDef* rec;
            Character* e = ZoneEntity(ZoneInfo(zone), i, &rec);
            if (!visit(zone, rec, e)) return;
        }
    }
}

// ---------------------------------------------------------------- entity helpers

typedef BOOL  (*ActivateFn)(Character*, int, int);
typedef void  (*VoidIntFn)(Character*, int);
typedef void  (*VoidFn)(Character*);
typedef void  (*TickFn)(Character*, int, f32);
typedef void  (*StepFn)(Character*, f32);
typedef u8*   (*GetVisualFn)(Character*);
typedef void  (*StartActionFn)(Character*, Character*, int, int, int, f32);
typedef void  (*CamFloatFn)(void*, f32);

#define ENTITY_TYPE_NO_EVENTS 0x6C938971u   // type hash of entities that don't report activation

static inline void HideVisual(Character* e) {
    if (VCALL(e, 0xC, GetVisualFn)(e) != NULL) *(u32*)(VCALL(e, 0xC, GetVisualFn)(e) + 8) &= ~2u;
}

static inline void SetActive(Character* e, int on) {
    ScopedLock lock;
    ScopedLockCtor(&lock);
    VCALL(e, 0x14, VoidIntFn)(e, on);
    ScopedLockDtor(&lock, -1);
}

static inline void SetHidden(Character* e, int mode) {
    ScopedLock lock;
    ScopedLockCtor(&lock);
    VCALL(e, 0x18, VoidIntFn)(e, mode);
    ScopedLockDtor(&lock, -1);
}

static inline void SpawnDespawnEffect(Character* e) {
    u8* model = *(u8**)((u8*)e + 0x9C);
    if (model == NULL) return;
    const Vec3* pos = (const Vec3*)((u8*)e + 0x18);
    f32 p[4] = { pos->x, pos->y, pos->z, 0.0f };
    SpawnEffectAt(lbl_8047C760, *(void**)(model + 0x1E4), p, 0);
    PlaySoundAt(*(void**)(model + 0x1F4), 0, pos, 0);
}

static inline void MarkDespawned(Character* e) {
    HideVisual(e);
    e->def->stateFlags &= ~0x20703u;
    e->def->stateFlags |= 0x20000;
    ((u8*)e->def)[0x35] = 0;
}

// ---------------------------------------------------------------- update

// 0x8012AB00
void WorldTick(WorldState* w, f32 dt) {
    w->time += dt;
    w->dt = dt;

    // Apply entity removals queued during the last frame.
    if (w->pendingCount != 0) {
        for (u32 i = 0; i < w->pendingCount; i++) {
            PendingRemoval* r = &w->pending[i];
            if (r->obj == NULL) continue;
            u8* z0 = w->data->zones;
            if (!r->dynamic) {
                if (r->index == *(s32*)(z0 + 0x94) - 1) *(s32*)(z0 + 0x94) = r->index;
                (*(void***)(z0 + 0x120))[r->index] = NULL;
            } else {
                if (r->index == *(s32*)(z0 + 0x98) - 1) *(s32*)(z0 + 0x98) = r->index;
                (*(void***)(z0 + 0x12C))[r->index] = NULL;
            }
        }
        w->pendingCount = 0;
    }

    WorldPreUpdate(w);

    // A freshly loaded zone becomes "ready" after a few frames.
    if (w->loadZone != -1 && w->loading == 0) {
        sModeTable[0].unkC |= 2;
        if (++sLoadFrames > 2 && (!sWorldReady || sWorldSuspended) && !sReadyRequested) {
            sReadyPending = 1;
            *(u32*)(gScriptCtx + 0x3C) &= ~4u;
            sReadyRequested = 1;
            sWorldSuspended = 0;
        }
    }
    if (w->idleFrames <= 0) WorldIdleTick();
    else if (!gScriptCtx[0x4C]) w->idleFrames--;

    if (sReadyRequested && sReadyPending) {
        sWorldReady = 1;
        sReadyPending = 0;
        sReadyRequested = 0;
    }

    if (sWorldReady && !sWorldSuspended) {
        if (gScriptCtx[0x4C]) return;
        EventMgrTick(lbl_805FB6D0, lbl_805FB91C);

        ScopedLock outer;
        ScopedLockCtor(&outer);

        // Pass A: entities that asked to be activated (0x800) or deactivated (0x80).
        ForEachZoneEntity(FALSE, [](s32, CharacterDef* rec, Character* e) {
            if (!(rec->stateFlags & 0x20900) || e == NULL) return TRUE;
            ScopedLock lock;
            ScopedLockCtor(&lock);
            u32 f = e->def->stateFlags;
            if (f & 0x800) {
                if (*(u32*)((u8*)e + 0xEC) != ENTITY_TYPE_NO_EVENTS && fn_801F5220()) {
                    u32 saved = lbl_805FB9A4;
                    lbl_805FB9A4 = 0;
                    u8* ev = (u8*)EventPost(0x79, e);
                    if (ev != NULL) *(s32*)(ev + 4) = 1;
                    lbl_805FB9A4 = saved;
                }
                if (VCALL(e, 0x30, ActivateFn)(e, 0, 1)) {
                    HideVisual(e);
                    e->def->stateFlags &= ~0x20B03u;
                    e->def->stateFlags |= 0x400;
                } else {
                    HideVisual(e);
                    e->def->stateFlags &= ~0x20F03u;
                }
            } else if (f & 0x80) {
                e->def->stateFlags = f & ~0x20300u;
            } else {
                if (!(f & 0x200)) {
                    u8* mgr = (u8*)gEventMgr;
                    if (*(s32*)(mgr + 0x418) != *(s32*)(mgr + 0x410)) EventMgrFlushFor(mgr, e);
                }
                if (e->def->stateFlags & 0x20200) EntityRefreshLinks(e);
            }
            ScopedLockDtor(&lock, -1);
            return TRUE;
        });

        // Pass B: deactivate culled entities.
        ForEachZoneEntity(FALSE, [w](s32, CharacterDef* rec, Character* e) {
            if (!(rec->flags & 0x704) || !(rec->stateFlags & 0x400) || e == NULL) return TRUE;
            if (!(w->time > sNextCullTime)) return TRUE;
            sNextCullTime = 0.0f;
            u32 f = e->def->stateFlags;
            if (f & 0x80) {
                e->def->stateFlags = f & ~0x20300u;
                HideVisual(e);
                e->def->stateFlags &= ~3u;
            } else if (!(e->def->flags & 0x10)) {
                if (*(u32*)((u8*)e + 0xEC) != ENTITY_TYPE_NO_EVENTS) SetActive(e, 0);
                else VCALL(e, 0x14, VoidIntFn)(e, 0);
            }
            return TRUE;
        });
        ScopedLockDtor(&outer, -1);

        // Re-activate the player after a reload.
        Character* player = (Character*)w->data->player;
        if (player != NULL && (player->def->stateFlags & 0x200 || player->def->stateFlags & 0x800)) {
            u8* ev = (u8*)EventPost(0x79, w->data->player);
            if (ev != NULL) *(s32*)(ev + 4) = 0;
            VCALL(player, 0x48, VoidFn)(player);
            if (VCALL(player, 0x30, ActivateFn)(player, 0, 0)) {
                player = (Character*)w->data->player;
                HideVisual(player);
                player->def->stateFlags &= ~0x04020F03u;
                CharacterDef* d = ((Character*)w->data->player)->def;
                if (*(s32*)((u8*)d + 0x28) != 0) (*(s32*)((u8*)d + 0x28))--;
                SetActive((Character*)w->data->player, 0);
                void* cam = *(void**)(gScriptCtx + 4);
                ((CamFloatFn)(*(void***)cam)[0x9C / 4])(cam, 0.0f);
                fn_800E03F0(*(u8**)(gScriptCtx + 8) + 0x68, 0);
                *(f32*)((u8*)w->data->player + 0x16C) = 0.5f;
            }
        }

        // Choose which entity to activate / cull next, keeping within the budget.
        if (w->time >= *(f32*)(gGame + 0x18)) {
            struct Follow {
                f32 budget;          // 0x00
                Character* cull;     // 0x04
                f32 cullDist;        // 0x08
                Character* activate; // 0x0C
                f32 activateDist;    // 0x10
                u8  pad14[0xC];
                u32 cullPriority;    // 0x20
                u32 activatePriority;// 0x24
            } f;
            CameraFollowReset(&f);
            ForEachZoneEntity(TRUE, [&f](s32 zone, CharacterDef* rec, Character* e) {
                if (!(rec->flags & 0x10) || !(rec->stateFlags & 0x400) || e == NULL) return TRUE;
                return ScoreCullCandidate(e, zone, &f);
            });
            ForEachZoneEntity(TRUE, [&f](s32 zone, CharacterDef* rec, Character* e) {
                if (!(rec->flags & 0x10) || !(rec->stateFlags & 1) || e == NULL) return TRUE;
                return ScoreActivateCandidate(e, zone, &f);
            });

            if ((f.budget < 0.0f || f.cull != NULL) && f.activate != NULL) {
                BOOL go = f.budget < 0.0f;
                if (f.cull != NULL) {
                    f32 margin = gFlag805FB948 ? 400.0f : 130.0f;
                    if (f.cullPriority > f.activatePriority) go = TRUE;
                    else if (f.cullPriority == f.activatePriority && f.activateDist - f.cullDist > margin) go = TRUE;
                }
                if (go) {
                    Character* a = f.activate;
                    SetHidden(a, 1);
                    HideVisual(a);
                    a->def->stateFlags &= ~0x20303u;
                    a->def->stateFlags |= 0x400;
                    ((u8*)a->def)[0x35] = 0;
                    f.budget += *(f32*)(*(u8**)((u8*)f.activate + 0x9C) + 0xC0);
                }
            }
            Character* c = f.cull;
            if (c != NULL && *(f32*)(*(u8**)((u8*)c + 0x9C) + 0xC0) <= f.budget) {
                *(f32*)(gGame + 0x18) = w->time;
                sLastCullTime = w->time;
                SetActive(c, 0);
            }
            *(f32*)(gGame + 0x1C) = lbl_805FB300 - f.budget;
            if (f.budget < lbl_805FB300) sLastCullTime = w->time;
        }

        // Pass E: entities that walked into another zone.
        sSirHeader[0x820] = 0;
        ForEachZoneEntity(TRUE, [w](s32, CharacterDef* rec, Character* e) {
            if (!(rec->stateFlags & 1) || e == NULL) return TRUE;
            CharacterDef* d = e->def;
            if (d == NULL || !(d->stateFlags & 3)) return TRUE;
            u16* zoneRef = (u16*)((u8*)d + 0x30);   // [0] current zone, [1] home zone
            if (zoneRef[0] == 0 || (d->flags & 0x400)) return TRUE;

            s32 dest = WorldGetEntityZone(gWorldState, e);
            if (dest > 0 && dest <= w->data->zoneCount && *(s32*)((u8*)e + 0x90) != 0) {
                if (zoneRef[0] == dest && ZoneIsLoaded(gWorldState, zoneRef[1])) return TRUE;
                ScopedLock lock;
                ScopedLockCtor(&lock);
                if (ZoneIsLoaded(gWorldState, dest) && ZoneIsLoaded(gWorldState, ((u16*)((u8*)e->def + 0x30))[1])) {
                    VCALL(e, 0x18, VoidIntFn)(e, 0);
                    ((u16*)((u8*)e->def + 0x30))[0] = (u16)dest;
                    VCALL(e, 0x14, VoidIntFn)(e, 1);
                } else if (e->def->flags & 0x20) {
                    SpawnDespawnEffect(e);
                    VCALL(e, 0x18, VoidIntFn)(e, 1);
                    if (!(e->def->stateFlags & 0x04000000)) {
                        u8* ev = (u8*)EventPost(0x64, e);
                        if (ev != NULL) *(s32*)(ev + 4) = 1;
                    }
                    MarkDespawned(e);
                    (*(s32*)((u8*)e->def + 0x28))++;
                }
                ScopedLockDtor(&lock, -1);
            } else if (dest == -1) {
                // Left the map.
                if (e == (Character*)w->data->player) {
                    f32 damage = 10.0f * *(f32*)((u8*)e->def + 0x24);
                    VCALL(e, 0x50, StartActionFn)(e, e, 7, 0, 0, damage);
                } else if (fn_801F5220() && (e->def->flags & 0x20)) {
                    SpawnDespawnEffect(e);
                    SetHidden(e, 2);
                    if (!(e->def->stateFlags & 0x04000000)) {
                        u8* ev = (u8*)EventPost(0x64, e);
                        if (ev != NULL) *(s32*)(ev + 4) = 0;
                    }
                    MarkDespawned(e);
                    s32* deaths = (s32*)((u8*)e->def + 0x28);
                    if (*deaths > 1000) {
                        (*deaths)++;
                        if (e->def->flags & 0x10) (*(s32*)(gWorldState + 0xE4))++;
                    }
                }
            }
            return TRUE;
        });

        // Pass F: advance every active entity. Slow motion scales everything but the player.
        if (!ScriptIsPaused(gScriptCtx)) {
            f32 worldDt = dt, playerDt = dt;
            if (lbl_805F95A0 < 1.0f) worldDt = lbl_805F95A0 * (dt * lbl_805F95A0);
            ForEachZoneEntity(FALSE, [w, worldDt, playerDt](s32, CharacterDef* rec, Character* e) {
                if (!(rec->stateFlags & 1) || e == NULL || EntityIsFrozen(e)) return TRUE;
                e->dt = (e == (Character*)w->data->player) ? playerDt : worldDt;
                *(f32*)((u8*)e + 0x28) += e->dt;
                VCALL(e, 0x20, TickFn)(e, 0, e->dt);
                return TRUE;
            });
        }

        // Pass G: sync transforms of entities that moved.
        ForEachZoneEntity(FALSE, [](s32, CharacterDef* rec, Character* e) {
            if ((rec->stateFlags & 1) && e != NULL && (e->def->stateFlags & 0x8000)) EntitySyncTransform(e, 0);
            return TRUE;
        });

        fn_8012F340();
        WorldPostUpdate(w);

        // Pass H: late update.
        ForEachZoneEntity(FALSE, [](s32, CharacterDef* rec, Character* e) {
            if (!(rec->stateFlags & 1) || e == NULL) return TRUE;
            if (e->def->stateFlags & 0x8000) EntitySyncTransform(e, 0);
            VCALL(e, 0x74, StepFn)(e, e->dt);
            if (!(e->def->stateFlags & 0x04000000)) VCALL(e, 0x48, VoidFn)(e);
            e->def->stateFlags &= ~0x10000000u;
            return TRUE;
        });
    }

    if (w->data->player != NULL) {
        void* cam = *(void**)(gScriptCtx + 4);
        ((CamFloatFn)(*(void***)cam)[0x98 / 4])(cam, dt);
        GameUpdateWorld(gGame, dt);
    }
    if (ScriptIsStopped(gScriptCtx)) return;

    // Particle systems of every loaded zone, then any detached list.
    void* pending = lbl_805FB5AC;
    for (s32 zone = 0; zone <= w->data->zoneCount; zone++) {
        if (!ZoneIsLoaded(w, zone)) continue;
        void* list = *(void**)(ZoneInfo(zone) + 0xFC);
        if (list != NULL) {
            if (list == pending) pending = NULL;
            for (u8* s = ParticleSystemFirst(list); s != NULL; ) {
                for (int k = 0; k < 8; k++) {
                    void* em = *(void**)(s + 0x5C + k * 4);
                    if (em != NULL) ParticleEmitterUpdate(s, em, *(void**)(s + 0x3C + k * 4), dt);
                }
                if (*(void**)(s + 0x7C) != NULL) ParticleSystemUpdateExtra(s, dt);
                u8* next = *(u8**)(s + 0x10);
                s = next ? next - 0x10 : NULL;
            }
        }
        ZoneEffectsUpdate(w->data->zoneEffects + zone * 0xC, dt);
    }
    if (pending != NULL) {
        for (u8* s = ParticleSystemFirst(pending); s != NULL; ) {
            for (int k = 0; k < 8; k++) {
                void* em = *(void**)(s + 0x5C + k * 4);
                if (em != NULL) ParticleEmitterUpdate(s, em, *(void**)(s + 0x3C + k * 4), dt);
            }
            if (*(void**)(s + 0x7C) != NULL) ParticleSystemUpdateExtra(s, dt);
            u8* next = *(u8**)(s + 0x10);
            s = next ? next - 0x10 : NULL;
        }
    }
}

// ---------------------------------------------------------------- zone activation

extern "C" void WorldSetZoneActive(void* world, s32 zone);          // 0x80132344
extern "C" void WorldRefreshZones(void* world);                     // 0x80131A14
extern "C" void WorldBroadcastToZone(void* world, u32 type, s32 a, s32 b, s32 c, s32 zone, s32 d);   // 0x8012E93C

// Number of zones: the 13 per-category counters (+0x68 .. +0x98) plus zone 0.
static inline s32 ZoneCountAll(void) {
    s32 n = 1;
    for (u32 off = 0x68; off <= 0x98; off += 4) n += *(s32*)(GetGlobalSettings() + off);
    return n;
}

// Placed particle effect of a zone (zone +0x80 list, count +0x9C).
struct ZoneEffect {
    void* effect;           // 0x00
    f32   pos[3];           // 0x04
    f32   dir[3];           // 0x10: (0,0,0) = default up
    u8*   instance;         // 0x1C: filled by SpawnEffectAt
};

static inline BOOL NearZero(f32 v) { return v <= 0.0001f && v >= -0.0001f; }

static inline void SetEmitterDirections(u8* inst, const f32 d[4]) {
    for (s32 j = 0; j < *(s32*)(inst + 0xC); j++) {
        f32* e = *(f32**)(inst + 0x14 + j * 4);
        if (e == NULL) continue;
        e[0xC] = d[0]; e[0xD] = d[1]; e[0xE] = d[2]; e[0xF] = d[3];
    }
}

// 0x801369C8
// A zone slot finished loading (type 4): start its placed effects, mark it active (type 6) and
// move entities whose home zone changed. Entities that ended up nowhere (-1) are despawned,
// except the player, who is respawned.
void WorldOnZoneLoaded(WorldState* w, s32 slotIndex) {
    if (slotIndex < 0 || slotIndex >= ZoneSlotCount()) return;
    ZoneSlot* slot = &w->slots[slotIndex];
    if (slot->type != 4) return;
    s32 zone = slot->zone;
    if (zone < 0 || zone >= ZoneCountAll()) return;
    u8* z = w->data->zones + zone * gZoneResourceSize;
    if (*(s32*)(z + 0x18) != zone) return;

    typedef void* (*SpawnFn)(void*, void*, const f32*, u8**);
    for (u32 i = 0; i < *(u32*)(z + 0x9C); i++) {
        ZoneEffect* fx = (*(ZoneEffect***)(z + 0x80))[i];
        if (fx == NULL) continue;
        f32 pos[4] = { fx->pos[0], fx->pos[1], fx->pos[2], 0.0f };
        if (((SpawnFn)SpawnEffectAt)(lbl_8047C760, fx->effect, pos, &fx->instance) == NULL) continue;
        f32 dir[4] = { fx->dir[0], fx->dir[1], fx->dir[2], 0.0f };
        if (NearZero(dir[0]) && NearZero(dir[1]) && NearZero(dir[2]) && NearZero(dir[3])) {
            static const f32 kUp[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
            SetEmitterDirections(fx->instance, kUp);
        } else {
            SetEmitterDirections(fx->instance, dir);
        }
    }

    slot->type = 6;
    WorldSetZoneActive(w, zone);
    WorldRefreshZones(w);
    u32* slotFlags = (u32*)((u8*)slot + 0xC);
    if (!(*slotFlags & 1)) {
        WorldBroadcastToZone(w, ENTITY_TYPE_NO_EVENTS, 0, 0, 0, zone, 0);
        *slotFlags |= 1;
    }
    if (zone < 0) return;

    u8* zi = ZoneInfo(zone);
    s32 count = ZoneStaticCount(zi) + ZoneDynamicCount(zi);
    for (s32 i = 0; i < count; i++) {
        CharacterDef* rec;
        Character* e = ZoneEntity(ZoneInfo(zone), i, &rec);
        if (!(rec->stateFlags & 1) || e == NULL || e->def == NULL) continue;
        CharacterDef* def = e->def;
        u16 home = *(u16*)((u8*)def + 0x30);
        if (!(def->stateFlags & 3) || home == 0 || (def->flags & 0x400)) continue;

        s32 ez = WorldGetEntityZone(w, e);
        if (ez > 0 && ez <= w->data->zoneCount && *(void**)((u8*)e + 0x90) != NULL) {
            u16 other = *(u16*)((u8*)def + 0x32);
            if (home == ez && ZoneIsLoaded(w, other)) continue;
            ScopedLock lock;
            ScopedLockCtor(&lock);
            if (ZoneIsLoaded(w, ez) && ZoneIsLoaded(w, other)) {
                VCALL(e, 0x18, VoidIntFn)(e, 0);
                *(u16*)((u8*)def + 0x30) = (u16)ez;
                VCALL(e, 0x14, VoidIntFn)(e, 1);
            } else if (def->flags & 0x20) {
                SpawnDespawnEffect(e);
                VCALL(e, 0x18, VoidIntFn)(e, 1);
                if (!(def->stateFlags & 0x04000000)) {
                    s32* ev = (s32*)EventPost(0x64, e);
                    if (ev != NULL) ev[1] = 1;
                }
                MarkDespawned(e);
                (*(s32*)((u8*)def + 0x28))++;
            }
            ScopedLockDtor(&lock, -1);
        } else if (ez == -1) {
            if ((void*)e == w->data->player) {
                typedef void (*RespawnFn)(Character*, s32, s32, s32, f32);
                VCALL(e, 0x50, RespawnFn)(e, 7, 0, 0, 10.0f * *(f32*)((u8*)def + 0x24));
            } else if (fn_801F5220() && (def->flags & 0x20)) {
                SpawnDespawnEffect(e);
                SetHidden(e, 2);
                if (!(def->stateFlags & 0x04000000)) {
                    s32* ev = (s32*)EventPost(0x64, e);
                    if (ev != NULL) ev[1] = 0;
                }
                MarkDespawned(e);
                s32* respawns = (s32*)((u8*)def + 0x28);
                if (*respawns > 1000) {
                    (*respawns)++;
                    if (def->flags & 0x10) w->despawned++;
                }
            }
        }
    }
}
