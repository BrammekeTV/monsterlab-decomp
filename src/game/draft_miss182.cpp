// Mixed batch: operating table / lightning manager init, limb fall step, lob shot script callbacks,
// footstep effect, ray query cast, network chunk dispatcher, heap realloc, save play-time badges and
// the Home Button menu loader.
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct Seeker;
struct Mover;
struct Locator;
struct StringTable;
struct ScopedLock { u8 pad[4]; };   // as in props.cpp

typedef u8*  (*GetObjFn)(u8*);                                   // object vtable +0xC / +0xCC / +0x10
typedef u8   (*CastFn)(u8*, u8*, u8*, u8*, s32, s32, s32);       // ray query vtable +0x14
typedef void (*MsgFn)(u8*, u8*);                                 // battle object vtable +0x10 / +0x14

extern "C++" {
void  PredictTargetPos(u8* self, Seeker* seeker, Mover* target, Vec3* out, f32 dt);   // 0x8000C604 (game_spawn.cpp)
u8*   fn_801B55A8(u8* ent);                                           // 0x801B55A8 (locator count, returned as an integer value)
f32   ComputeLobSpeed(const Vec3* from, const Vec3* to, f32 angle, f32 gravity);   // 0x80019FB0 (projectile_math.cpp)
}

extern "C" {
void*  CharacterFind(s32 id);                                         // 0x800E876C
void*  EventPost(s32 id, void* sender);                               // 0x801F3918
void   GameFlowSignal(s32 id, s32 a, s32 b);                          // 0x8000C7F4
void   HideAlertBox(void);
void*  MEMAllocFromAllocator(MEMAllocator* allocator, u32 size);
void*  MemAllocDefault(u32 size);                                     // 0x801D4650
void   MtxFromLocatorRotation(f32 out[12], const void* rot);          // 0x80143CA8
void   NoticePosted(u8* p, s32 idx);                                  // 0x8022DF3C
u32    RandomU32(void);                                               // 0x80143460
void   ScopedLockCtor(ScopedLock* l);                                 // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                      // 0x801F53B0
u8     SCGetAspectRatio(void);                                        // 0x8026A9D8: 1 = 16:9
u8     SCGetLanguage(void);
void   SetResponse(u32 a3);
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);   // 0x801C80E0
void*  SpawnHelper(void* owner, s32 arg);                             // 0x8000C478
u32    VIGetTvFormat(void);
Locator* WorldFindLocator(void* world, u32 hash);                     // 0x8012AA50
s32    WorldGetPhase(void* world);                                    // 0x800D83A8
void   QuatFromMtx(Quat* out, const f32 m[12]);                       // 0x801456E4
void* fn_8004D2C8(u8* arg0, const char* arg1, s32 arg2, s32 arg3);          // 0x8004D2C8
s32    fn_8004D3F8(void* c, const char* name, s32 a, s32 b);          // 0x8004D3F8
s32    fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6);   // 0x80065200
s32    fn_800D5800(u8* p);                                            // 0x800D5800
void   fn_800D83B8(u8* a3, s32 a4);                                   // 0x800D83B8
s32    fn_801088F0(u8* p, s32 a, s32* out);                           // 0x801088F0
s32 fn_8010A49C(u8* p, s32 kind);                                            // 0x8010A49C
s32    fn_8011BC40(u8* p, s32 a);                                     // 0x8011BC40
void   fn_8013A4A0(s32 a, s32 b);                                     // 0x8013A4A0
void   fn_8013C06C(u8* p);                                            // 0x8013C06C
char* fn_8013E3BC(const char* src);                                            // 0x8013E3BC
void   fn_8013E3D0(f32 v);                                            // 0x8013E3D0
void   fn_8013E3D8(f32 v);                                            // 0x8013E3D8
void   fn_8013E3E0(f32 v);                                            // 0x8013E3E0
void   fn_8013E3E8(f32 v);                                            // 0x8013E3E8
void   fn_8013E41C(s32 v);                                            // 0x8013E41C
void   fn_8013E424(s32 v);                                            // 0x8013E424
void   fn_8013E42C(s32 v);                                            // 0x8013E42C
void   fn_8013E434(s32 v);                                            // 0x8013E434
void   fn_8013E43C(s32 v);                                            // 0x8013E43C
void   fn_8013E45C(s32 v);                                            // 0x8013E45C
void   fn_80140B30(void);                                             // 0x80140B30
void   fn_80140B7C(s32 mask);                                         // 0x80140B7C
void   fn_80140B8C(s32 mask);                                         // 0x80140B8C
void   fn_80142878(u8* cb);                                           // 0x80142878 (register per-frame callback)
void   fn_80146180(void* quat, const Vec3* axis, f32 angle);          // 0x80146180 (rotate quat about axis)
s32    fn_80146808(u8* file, const char* name, const char* mode);     // 0x80146808
s32    fn_80146930(u8* file, void* buf, s32 size);                    // 0x80146930
void   fn_801469FC(u8* file);                                         // 0x801469FC
s32    fn_80146A58(u8* file);                                         // 0x80146A58
f32    fn_8018BE30(void* obj, f32 a);                                 // 0x8018BE30
u8*    fn_8001B20C(u8* p);                                            // 0x8001B20C (constructor)
u8*    fn_8001BA70(u8* p);                                            // 0x8001BA70
void   fn_801B1988(u8* p);                                            // 0x801B1988
BOOL   fn_801B50B0(void* unused, u8* obj);                            // 0x801B50B0
void   fn_801B55F4(u8* obj, u8* out, s32 idx);                        // 0x801B55F4
void*  fn_801D40E0(s32 n);                                            // 0x801D40E0 (alloc)
u8*    fn_801E763C(u8* p);                                            // 0x801E763C
void   fn_801E7E28(void* p);                                          // 0x801E7E28
u32    fn_801EC880(u8* p, u32 hash);                                  // 0x801EC880
void   fn_801ECDE8(u8* p, s32 idx, u32 a, s32 immediate);             // 0x801ECDE8
void   fn_801ED050(u8* p, s32 flag);                                  // 0x801ED050
void*  fn_801EEB20(u8* heap, u32 size);                               // 0x801EEB20
void   fn_801EF070(u8* heap, void* p);                                // 0x801EF070
void*  fn_801EF484(u8* heap, u8* p, u32 size);                        // 0x801EF484
void   fn_801F0658(f32 t);                                            // 0x801F0658
void   fn_801F5724(u32 button);                                       // 0x801F5724
void   fn_801F57A4(s32 v);                                            // 0x801F57A4
void   fn_801F57B8(void);                                             // 0x801F57B8
void   fn_801F5814(s32 a);                                            // 0x801F5814
u8*    fn_8020E7EC(u8* p, u8* a, u8* b);                              // 0x8020E7EC (b is a pointer: the old decl takes s32)
BOOL   fn_80212E28(u8** slot);                                        // 0x80212E28
void   fn_80220A8C(u8* p, s32 seed);                                  // 0x80220A8C
s32    fn_80216754(GameDb* db, u32 key);                              // 0x80216754
void   fn_802C8AB4(u8* p);                                            // 0x802C8AB4
void   fn_802C8DE4(s32 v);                                            // 0x802C8DE4
void   fn_802C8E00(u8* a, u8* buf, s32 size);                         // 0x802C8E00
void   fn_801FB0D8(void);                                             // 0x801FB0D8 (sound callback)
void   fn_801FB760(void);                                             // 0x801FB760 (per-frame update)
void   SaveAddBadge(u8* s, s32 badge);                                // 0x802233D4
u32    SaveHasBadge(u8* s, u32 badge);                                // 0x80223308
char*  strcat(char* d, const char* s);
char*  strcpy(char* d, const char* s);

extern u8* gBattle;
extern u8  gGame[];                      // 0x803B53A0
extern u8* gGlobalSettings;              // 0x805FB7A4
extern u8  gWorldState[];                // 0x803E6D00 (first word points to the world block)
extern const s32 lbl_80349C30[];         // 0x80349C30 (play-time badge: threshold table rows)
extern const s32 lbl_80349C44[];         // 0x80349C44 (play-time badge ids)
extern const u8 lbl_80349010[];          // 0x80349010 ("HomeButton2/")
extern const char lbl_80386528[];        // 0x80386528 (home button archive suffixes, 0x10 apart per language)
extern u32 lbl_8035916C[];               // 0x8035916C
extern u8  lbl_80383D68[];               // 0x80383D68 (vtable)
extern const u32 lbl_803881B4[];         // 0x803881B4 (emitter id -> emitter hash)
extern const u32 kAbilityEffects[];      // 0x80388258
extern s32 lbl_803C4E28[];               // 0x803C4E28
extern u8* lbl_803C7404;                 // 0x803C7404 (allocation pool table, 0x20 bytes each)
extern u8  lbl_803E69D8[];               // 0x803E69D8 (5 records of 0x10)
extern MEMAllocator lbl_803F2870;        // 0x803F2870
extern u8  lbl_803F2250[];               // 0x803F2250
extern u8  lbl_8047C760[];               // 0x8047C760 (effect manager)
extern u8  lbl_8047CAC0[];               // 0x8047CAC0 (main heap; second heap at +0x394)
extern u8  lbl_804FB860[];               // 0x804FB860 (home button data record)
extern u8  lbl_804FB8A0[];               // 0x804FB8A0 (4 records of 0x10)
extern u8  lbl_80556E80[];               // 0x80556E80 (player data block)
extern const char lbl_805F8200[];        // 0x805F8200 (empty string)
extern u32 lbl_805F9030;                 // 0x805F9030
extern s32 lbl_805F95F8;                 // 0x805F95F8 (current allocation pool index, -1 = main heap)
extern const char lbl_805F98AC[];        // 0x805F98AC ("rb")
extern u8* lbl_805FAF78;                 // 0x805FAF78 (lightning manager)
extern s32 lbl_805FB034;                 // 0x805FB034
extern s32 lbl_805FB038;                 // 0x805FB038
extern s32 lbl_805FB1C4;                 // 0x805FB1C4
extern u8  lbl_805FB211;                 // 0x805FB211
extern u32 lbl_805FB23C;                 // 0x805FB23C
extern s32 lbl_805FB378;                 // 0x805FB378
extern s32* lbl_805FB7A0;                // 0x805FB7A0 (per-pool allocation counters)
extern u8  lbl_805FB7A8[];               // 0x805FB7A8 (+4: second heap enabled)
extern u32 lbl_805FB7D0;                 // 0x805FB7D0 (alloc call counter, high word)
extern u32 lbl_805FB7D4;                 // 0x805FB7D4 (alloc call counter, low word)
extern u8  lbl_805FB7E1[];               // 0x805FB7E1 (nonzero: allocation disabled)
extern u8* lbl_805FBA24;                 // 0x805FBA24 (home button layout archive)
extern u8* lbl_805FBA28;                 // 0x805FBA28 (home button work memory)
extern s32 lbl_805FBA2C;                 // 0x805FBA2C (bytes allocated by the home button loader)
extern u8* lbl_805FBBE0;                 // 0x805FBBE0 (save block pointer)
}

extern "C++" u8  gIgcMode;               // 0x805FAF64
extern "C++" StringTable* gStringTable;  // 0x805FB910

// Bump-allocates size bytes (16-byte aligned) from an allocation pool; falls back to a fresh
// block from the default allocator when the pool is full. Pool: +0xC cursor, +0x10 end,
// +0x14 bytes used, +0x18 allocation count, +0x1C overflow bytes.
static inline void* PoolAlloc(u8* pool, u32 size) {
    u32 end = *(u32*)(pool + 0x10);
    u32 cur = *(u32*)(pool + 0xC);
    u32 aligned = (cur + 0xF) & ~0xFu;
    u32 newCur = aligned + size;
    uintptr_t result;
    if (cur < end - 1 && newCur < end) {
        result = aligned;
    } else {
        s32 n = (s32)(size + (aligned - cur));
        s32 m = (s32)((size + 0x10) & ~0xFu);
        *(s32*)(pool + 0x1C) += n;
        if (m > n) n = m;
        result = ((uintptr_t)MemAllocDefault((u32)n) + 0xF) & ~(uintptr_t)0xF;
    }
    *(s32*)(pool + 0x14) += (s32)(aligned - cur);
    *(u32*)(pool + 0xC) = newCur;
    *(s32*)(pool + 0x18) += 1;
    return (void*)result;
}

// The heap record (main or second heap) whose block range contains p, or NULL.
static u8* HeapOf(u8* p) {
    if ((uintptr_t)p >= (uintptr_t)*(u8**)(lbl_8047CAC0 + 0x37C) - 0x10 &&
        (uintptr_t)p < (uintptr_t)*(u8**)(lbl_8047CAC0 + 0x384)) {
        return lbl_8047CAC0;
    }
    if (*(s32*)(lbl_805FB7A8 + 4) != 0) {
        if ((uintptr_t)p >= (uintptr_t)*(u8**)(lbl_8047CAC0 + 0x710) - 0x10 &&
            (uintptr_t)p < (uintptr_t)*(u8**)(lbl_8047CAC0 + 0x718)) {
            return lbl_8047CAC0 + 0x394;
        }
    }
    return NULL;
}

// Resolves the locator that a script reference selects: by locator index (bit 0, via fn_801B55F4) or by
// the slot table at obj+0x98 (bit 1, 0xC8-byte slots whose 12 floats at +0x38 are the locator).
// Returns 0 when the reference selects nothing.
static s32 ResolveLocator(u8* obj, u8* ref, f32 loc[12]) {
    u16 flags = *(u16*)ref;
    s32 idx;
    if (flags & 1) idx = *(u8*)(ref + 2);
    else idx = -1;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(obj)) {
            fn_801B55F4(obj, (u8*)loc, idx);
            return 1;
        }
        return 0;
    }
    s32 idx2;
    if (flags & 2) idx2 = *(u8*)(ref + 2);
    else idx2 = -1;
    if (idx2 != -1) {
        u8* src = *(u8**)(obj + 0x98) + idx2 * 0xC8;
        for (s32 i = 0; i < 12; i++) loc[i] = *(f32*)(src + 0x38 + i * 4);
        return 1;
    }
    return 0;
}

// Picks the shot template id for the shooter and spawns the projectile entity from the target's pool.
static u8* SpawnShot(u8* obj, u8* ref, u8* target) {
    s32 spawnArg = 0;
    s32 refId = *(s32*)ref;
    s32 objFlags = *(s32*)(*(u8**)(obj + 4) + 0x1C);
    if (objFlags & 0x20) {
        spawnArg = *(s32*)(obj + 0xA14);
    } else if (objFlags & 0x40) {
        *(s32*)(obj + 0x178) = refId;
        spawnArg = *(s32*)(obj + 0x18C);
        *(u8*)(obj + 0x190) = (u8)(*(u8*)(obj + 0x190) + 1);
    }
    if (spawnArg == 0) return NULL;
    return (u8*)SpawnHelper(target, spawnArg);
}

// Moves the table object onto the locator 0x42BCE9A5 (position and orientation), marks it moved and
// clears its flag byte at +0xC83.
static void PlaceOnTableLocator(u8* t) {
    u8* loc = (u8*)WorldFindLocator(gWorldState, 0x42BCE9A5u);
    if (loc != NULL) {
        u8* d = *(u8**)(*(u8**)t + 4);
        *(f32*)(d + 0x0) = *(f32*)(loc + 0x8);
        *(f32*)(d + 0x4) = *(f32*)(loc + 0xC);
        *(f32*)(d + 0x8) = *(f32*)(loc + 0x10);
        f32 m[12] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
        f32 rot[3];
        rot[0] = *(f32*)(loc + 0x14) * 0.63661975f;
        rot[1] = *(f32*)(loc + 0x18) * 0.63661975f;
        rot[2] = *(f32*)(loc + 0x1C) * 0.63661975f;
        MtxFromLocatorRotation(m, rot);
        QuatFromMtx((Quat*)(*(u8**)(*(u8**)t + 4) + 0xC), m);
    }
    u8* d2 = *(u8**)(*(u8**)t + 4);
    *(u32*)(d2 + 0x20) = *(u32*)(d2 + 0x20) | 0x8000;
    *(u8*)(*(u8**)t + 0xC83) = 0;
}

// Loads one home button archive: opens `path`, reads it into a 32-byte aligned allocation and adds the
// allocation size to the running total. Returns the buffer (NULL when the file can't be opened);
// *readSize receives the read result.
static u8* LoadHomeArc(u8* file, const char* path, s32* readSize) {
    u8* buf;
    if (fn_80146808(file, path, lbl_805F98AC) == 0) {
        buf = NULL;
    } else {
        s32 size = fn_80146A58(file);
        s32 alloc = (size + 0x1F) & ~0x1F;
        buf = (u8*)MEMAllocFromAllocator(&lbl_803F2870, (u32)alloc);
        s32 r = fn_80146930(file, buf, size);
        fn_801469FC(file);
        lbl_805FBA2C += alloc;
        if (readSize != NULL) *readSize = r;
    }
    return buf;
}

extern "C" {

// 0x801B4334
// Constructor: sets the vtable, clears the +0x94/+0x98 words when a pool is selected, then attaches the
// entity record `ent` at +0x4 (optionally resetting it to a unit transform in layer `layer`).
u8* fn_801B4334(u8* arg0, s32 layer, u8* ent, s32 reset) {
    *(u8**)arg0 = lbl_80383D68;
    if (lbl_805F95F8 != -1) {
        *(s32*)(arg0 + 0x94) = 0;
        *(s32*)(arg0 + 0x98) = 0;
    }
    fn_801E763C(arg0 + 0xFC);
    if (ent != NULL) {
        *(u8**)(arg0 + 0x4) = ent;
        if (reset != 0) {
            *(u8*)(ent + 0x34) = (u8)layer;
            *(s32*)(*(u8**)(arg0 + 0x4) + 0x1C) = (s32)PpcSlw(1u, layer);
            *(s32*)(*(u8**)(arg0 + 0x4) + 0x20) = 0;
            u8* d = *(u8**)(arg0 + 0x4);
            *(f32*)(d + 0x0) = 0.0f;
            *(f32*)(d + 0x4) = 0.0f;
            *(f32*)(d + 0x8) = 0.0f;
            d = *(u8**)(arg0 + 0x4);
            *(f32*)(d + 0xC) = 0.0f;
            *(f32*)(d + 0x10) = 0.0f;
            *(f32*)(d + 0x14) = 0.0f;
            *(f32*)(d + 0x18) = 1.0f;
            *(s16*)(*(u8**)(arg0 + 0x4) + 0x30) = 0;
            *(s32*)(*(u8**)(arg0 + 0x4) + 0x2C) = 0;
            *(u8*)(*(u8**)(arg0 + 0x4) + 0x3C) = 0;
            *(u8*)(*(u8**)(arg0 + 0x4) + 0x39) = 0;
            *(u8*)(*(u8**)(arg0 + 0x4) + 0x38) = 0;
            *(u8*)(*(u8**)(arg0 + 0x4) + 0x3A) = 0xFF;
            *(u8*)(*(u8**)(arg0 + 0x4) + 0x3B) = 0;
            *(u8*)(*(u8**)(arg0 + 0x4) + 0x3D) = 0;
        }
    } else if (lbl_805F95F8 != -1) {
        *(u8**)(arg0 + 0x4) = NULL;
    }
    if (lbl_805F95F8 == -1 && *(s32*)(arg0 + 0x8C) != 0) {
        fn_801B1988(*(u8**)(arg0 + 0x8C));
    }
    fn_801E7E28(arg0 + 0xFC);
    return arg0;
}

// 0x8001B310
// Creates the lightning manager (operating table) on first use: allocates and builds it, creates the
// main table object, looks up the five characters at +0xC04.., sets up the five selector records at
// +0x8.. (0x1A4 bytes each) and the table's effect hashes.
void InitializeOperatingTable(void) {
    if (lbl_805FAF78 == NULL) {
        for (s32 i = 0; i < 10; i++) lbl_803C4E28[i] = 0;
        lbl_805FB038 = 10;
        lbl_805FB034 = 0;
        u8* mem = (u8*)fn_801D40E0(0xC48);
        if (mem != NULL) mem = fn_8001B20C(mem);
        lbl_805FAF78 = mem;
    }
    u8* t = lbl_805FAF78;
    if (t == NULL) return;

    *(u8**)t = fn_8001BA70(t);
    u8* st = (u8*)gStringTable;
    u8* base = *(u8**)st;
    if (base != NULL && *(u32*)(st + 0xC) > 0x682u) {
        *(const char**)(t + 0xA90) = *(const char**)(base + 0x1A08);
    } else {
        *(const char**)(t + 0xA90) = lbl_805F8200;
    }
    if (*(u8**)t != NULL) {
        PlaceOnTableLocator(t);
        fn_8004D2C8(*(u8**)t, "hanging", 0, 1);
        KillHighlights();
    }

    u8* tt = lbl_805FAF78;
    *(void**)(tt + 0xC04) = CharacterFind((s32)0x5DABD780);
    *(void**)(tt + 0xC08) = CharacterFind(0x07406296);
    *(void**)(tt + 0xC14) = CharacterFind(0x08CA9823);
    s32 sel[5];
    sel[0] = 0x48;
    sel[1] = 0x72;
    sel[2] = 0;   // never read: characters 2 and 3 are not created here
    sel[3] = 0;
    sel[4] = 0x96;
    for (s32 i = 0; i < 5; i++) {
        u8* c = *(u8**)(tt + i * 4 + 0xC04);
        if (c != NULL) {
            *(s32*)(tt + i * 0x30 + 0x83C) = sel[i];
            u8* rec = (u8*)fn_801D40E0(0x40);
            if (rec != NULL) {
                u8* owner = *(u8**)(tt + i * 4 + 0xC04);
                if (owner != NULL) owner += 0x264;
                rec = fn_8020E7EC(rec, tt + i * 0x30 + 0x83C, owner);
            }
            c = *(u8**)(tt + i * 4 + 0xC04);
            *(u8**)(c + 0x268) = rec;
            c = *(u8**)(tt + i * 4 + 0xC04);
            u8* ent = (*(GetObjFn*)(*(u8**)c + 0xCC))(c);
            *(s32*)(ent + 0x4) = i;
        }
    }

    u8* sl = lbl_805FAF78;
    for (s32 j = 0; j < 5; j++) {
        u8* s = sl + j * 0x1A4;
        if ((u32)(j - 2) <= 1u) {
            s32 v = fn_8011BC40(s + 8, 2);
            *(s32*)(s + 0x1A0) = v;
            *(s32*)(s + 0x1A0) = *(s32*)(s + 0x1A0) + fn_8011BC40(s + 8 + v * 4, 3);
        } else {
            *(s32*)(s + 0x1A0) = fn_8011BC40(s + 8, j);
        }
        s32 n = *(s32*)(s + 0x1A0);
        *(s32*)(s + n * 4 + 8) = -1;
        *(s32*)(s + 0x1A0) = n + 1;
        *(s32*)(s + 0x198) = n;
        *(s32*)(s + 0x19C) = n;
    }

    u8* t2 = lbl_805FAF78;
    if (*(u8**)t2 != NULL) {
        PlaceOnTableLocator(t2);
    }
    LightningTowerShowMonster(0);
    if (lbl_805FAF78 != NULL) {
        u8* obj = *(u8**)lbl_805FAF78;
        if (obj != NULL) fn_8004D3F8(obj, "hanging", 0, 1);
    }
    *(s32*)(lbl_805FAF78 + 0xAE4) = 1;
    fn_800D83B8(gGame + 0xD5F0, 7);
    *(u8*)(lbl_805FAF78 + 0xC18) = 1;
    *(u32*)(lbl_805FAF78 + 0xC1C) = 0xC83D3EE7u;
    *(u32*)(lbl_805FAF78 + 0xC2C) = 0xF5D84CD2u;
    *(u32*)(lbl_805FAF78 + 0xC20) = 0xE8D268F2u;
    *(u32*)(lbl_805FAF78 + 0xC24) = 0xEBB4BC19u;
    *(u32*)(lbl_805FAF78 + 0xC28) = 0x28C540ACu;
    *(u32*)(lbl_805FAF78 + 0xC30) = 0x51A47EE5u;
    *(u32*)(lbl_805FAF78 + 0xC40) = 0xBA4581F0u;
    *(u32*)(lbl_805FAF78 + 0xC34) = 0xBCDA44A1u;
    *(u32*)(lbl_805FAF78 + 0xC38) = 0x9D8A82EFu;
    *(u32*)(lbl_805FAF78 + 0xC3C) = 0x3FB16D4Eu;
}

// 0x8004F774
// Per-frame fall of a limb that has been knocked off (flag at +0x394): integrates the stored velocity
// (+0x398 with gravity on z), bounces off the ground (speed damped by 0.75 / 0.4), spins the limb and
// counts down the effect timer in the limb's parent record. Returns 1 when that timer runs out. While
// the limb has not been knocked off it only waits for the hit marker and notifies the character.
BOOL LimbUpdateFall(void* limb, f32 dt) {
    u8* l = (u8*)limb;
    *(f32*)(l + 0x390) = *(f32*)(l + 0x390) + dt;
    u8* holder = (*(GetObjFn*)(*(u8**)l + 0xCC))(l);
    s32 emitter = *(s32*)(holder + 4);
    if (*(f32*)(l + 0x390) > 0.09375f) fn_801F0658(1.0f);
    if (*(u8*)(l + 0x394) == 0) {
        void* obj = (*(GetObjFn*)(*(u8**)l + 0x10))(l);
        if (fn_8018BE30(obj, 0.0f) == 1.0f) {
            void* ch;
            if (*(u32*)(*(u8**)(*(u8**)(*(u8**)(l + 0x268) + 0x24) + 0x1C) + 0x2C) == 0x9C2CDBD4u) {
                ch = CharacterFind((s32)0x4F8EC8A8);
            } else {
                ch = CharacterFind((s32)0xD6879912);
            }
            if (ch != NULL) fn_801B50B0(ch, l);
            fn_801F0658(1.0f);
            return 1;
        }
    }
    if (*(u8*)(l + 0x394) == 0) return 0;

    u8* d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x8000;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x4000;
    f32 vz = *(f32*)(l + 0x3A0) - (9.0f * dt);
    f32 sx = *(f32*)(l + 0x398) * dt;
    *(u32*)(l + 0x15C) = *(u32*)(l + 0x15C) | 0x400000;
    *(f32*)(l + 0x3A0) = vz;
    f32 sy = *(f32*)(l + 0x39C) * dt;
    d = *(u8**)(l + 4);
    f32 sz = vz * dt;
    f32 px = *(f32*)(d + 0x0) + sx;
    f32 py = *(f32*)(d + 0x4) + sy;
    f32 pz = *(f32*)(d + 0x8) + sz;

    u8* g = (*(GetObjFn*)(*(u8**)l + 0xCC))(l);
    u8* gp = *(u8**)(*(u8**)(g + 0x24) + 0x1C);
    if (gp != NULL) gp -= 0xC48;
    u8* ground = *(u8**)(gp + 4);
    f32 floorZ = 0.25f + *(f32*)(ground + 0x8);
    if (pz < floorZ) {
        f32 nvz = -*(f32*)(l + 0x3A0);
        f32 vx = *(f32*)(l + 0x398);
        f32 vy = *(f32*)(l + 0x39C);
        *(f32*)(l + 0x3A0) = nvz;
        pz = (1.4f * floorZ) - (0.4f * pz);
        *(f32*)(l + 0x398) = 0.75f * vx;
        *(f32*)(l + 0x39C) = 0.75f * vy;
        *(f32*)(l + 0x3A0) = 0.4f * nvz;
    }

    d = *(u8**)(l + 4);
    *(f32*)(d + 0x0) = px;
    *(f32*)(d + 0x4) = py;
    *(f32*)(d + 0x8) = pz;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x8000;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x10000000;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x4000;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x10000000;
    d = *(u8**)(l + 4);
    f32 q[4];
    q[0] = *(f32*)(d + 0xC);
    q[1] = *(f32*)(d + 0x10);
    q[2] = *(f32*)(d + 0x14);
    q[3] = *(f32*)(d + 0x18);
    fn_80146180(q, (const Vec3*)(l + 0x3A4), 3.0f * dt);
    d = *(u8**)(l + 4);
    *(f32*)(d + 0xC) = q[0];
    *(f32*)(d + 0x10) = q[1];
    *(f32*)(d + 0x14) = q[2];
    *(f32*)(d + 0x18) = q[3];
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x8000;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x10000000;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x4000;
    d = *(u8**)(l + 4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x10000000;

    // Count the timer in the parent record's fourth float (+0x44) down, clamped at zero.
    u8* rec = (*(GetObjFn*)(*(u8**)l + 0xC))(l);
    f32 ra = *(f32*)(rec + 0x38);
    f32 rb = *(f32*)(rec + 0x3C);
    f32 rc = *(f32*)(rec + 0x40);
    f32 left = *(f32*)(rec + 0x44) - dt;
    f32 stored = left;
    if (left < 0.0f) stored = 0.0f;
    rec = (*(GetObjFn*)(*(u8**)l + 0xC))(l);
    *(f32*)(rec + 0x38) = ra;
    *(f32*)(rec + 0x3C) = rb;
    *(f32*)(rec + 0x40) = rc;
    *(f32*)(rec + 0x44) = stored;
    BOOL done = (left < 0.0f);
    if (done) {
        lbl_805FB1C4 = 0;
        s32 idx = (s32)fn_801EC880(l + 0x94, lbl_8035916C[emitter]);
        if (idx >= 0) fn_801ECDE8(l + 0x94, idx, 0x5F73A0E4u, 0);
        *(u8*)(l + 0x38F) = 0;
        if ((*(GetObjFn*)(*(u8**)l + 0xC))(l) != NULL) {
            u8* r = (*(GetObjFn*)(*(u8**)l + 0xC))(l);
            *(u32*)(r + 8) = *(u32*)(r + 8) & 0xFFFFFFFDu;
        }
        d = *(u8**)(l + 4);
        *(u32*)(d + 0x20) = *(u32*)(d + 0x20) & 0xFFFFFFFDu;
        fn_801ED050(l + 0x94, 0);
        *(u8*)(l + 0x38E) = 0;
        d = *(u8**)(l + 4);
        *(u32*)(d + 0x20) = *(u32*)(d + 0x20) | 0x8000;
    }
    return done;
}

// 0x800EB338
// Script callback LobShot: lobs a projectile from the shooter's locator at the target with a random
// spread (a3) and a lead time a4. The launch velocity comes from ComputeLobSpeed; when it is positive
// the shot is aimed horizontally and tilted up by half-pi times the target's +0xD8 value.
void LobShot(void* a0, u32 /*0x30*/ a1, u32 /*0x38*/ a2, f32 a3, f32 a4) {
    u8* obj = (u8*)a0;
    u8* ref = (u8*)(uintptr_t)a1;
    u8* target = (u8*)(uintptr_t)a2;
    f32 spread = a3;
    f32 loc[12];
    if (gIgcMode != 0) return;
    if (ResolveLocator(obj, ref, loc) == 0) return;
    u8* shot = SpawnShot(obj, ref, target);
    if (shot == NULL) return;
    f32 tgt[3];
    PredictTargetPos(target, (Seeker*)obj, *(Mover**)(shot + 0x248), (Vec3*)tgt, a4);
    f32 ox = (spread * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - spread;
    f32 oy = (spread * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - spread;
    f32 oz = (spread * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - spread;
    f32 tx = tgt[0] + ox;
    f32 ty = tgt[1] + oy;
    f32 tz = tgt[2] + oz;
    f32 to[3];
    to[0] = tx;
    to[1] = ty;
    to[2] = tz;
    f32 d[3];
    d[0] = tx - loc[9];
    d[1] = ty - loc[10];
    d[2] = tz - loc[11];
    u8* proj = *(u8**)(shot + 0x228);
    f32 speed = ComputeLobSpeed((const Vec3*)&loc[9], (const Vec3*)to, *(f32*)(proj + 0xD8), -*(f32*)(proj + 0xD4));
    if (speed > 0.0f) {
        d[2] = 0.0f;
        f32 len2 = (d[2] * d[2]) + ((d[0] * d[0]) + (d[1] * d[1]));
        f32 n[3];
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
            n[0] = d[0] * inv;
            n[1] = d[1] * inv;
            n[2] = d[2] * inv;
        }
        f32 s = (f32)sinf(1.5707964f * *(f32*)(target + 0xD8));
        f32 c = (f32)cosf(1.5707964f * *(f32*)(target + 0xD8));
        f32 v[3];
        v[0] = ((n[0] * c) + 0.0f) * speed;
        v[1] = ((n[1] * c) + 0.0f) * speed;
        v[2] = ((n[2] * c) + s) * speed;
        fn_80065200(shot, (u8*)&loc[9], v, obj, 0, 0x4000, 0x4000);
    } else {
        f32 len2 = (d[2] * d[2]) + ((d[0] * d[0]) + (d[1] * d[1]));
        f32 n[3];
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
            n[0] = d[0] * inv;
            n[1] = d[1] * inv;
            n[2] = d[2] * inv;
        }
        fn_80065200(shot, (u8*)&loc[9], n, obj, 0, 0, 0);
    }
}

// 0x800EB860
// Script callback LobWarningShot: like LobShot but aims at a random point on a circle of radius a3
// (horizontal offset only) around the predicted target, and scales the projectile's own velocity
// vector (+0x1AC) by the lob speed, so the shot misses on purpose.
void LobWarningShot(void* a0, u32 /*0x30*/ a1, u32 /*0x38*/ a2, f32 a3, f32 a4) {
    u8* obj = (u8*)a0;
    u8* ref = (u8*)(uintptr_t)a1;
    u8* target = (u8*)(uintptr_t)a2;
    f32 radius = a3;
    f32 loc[12];
    if (gIgcMode != 0) return;
    if (ResolveLocator(obj, ref, loc) == 0) return;
    u8* shot = SpawnShot(obj, ref, target);
    if (shot == NULL) return;
    f32 tgt[3];
    PredictTargetPos(target, (Seeker*)obj, *(Mover**)(shot + 0x248), (Vec3*)tgt, a4);
    f32 rx = (1.0f * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - 1.0f;
    f32 ry = (1.0f * (2.0f * (2.3283064e-10f * (f32)RandomU32()))) - 1.0f;
    (void)RandomU32();   // third roll is discarded
    f32 rz = 0.0f;
    f32 rlen2 = (rz * rz) + ((rx * rx) + (ry * ry));
    f32 off[3];
    if (rlen2 == 0.0f) {
        off[0] = 0.0f;
        off[1] = 0.0f;
        off[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(rlen2);  // original: frsqrte with two Newton steps
        off[0] = rx * inv;
        off[1] = ry * inv;
        off[2] = rz * inv;
    }
    f32 ox = off[0] * radius;
    f32 oy = off[1] * radius;
    f32 oz = off[2] * radius;
    tgt[0] = tgt[0] + ox;
    tgt[1] = tgt[1] + oy;
    tgt[2] = tgt[2] + oz;
    f32 d[3];
    d[0] = tgt[0] - loc[9];
    d[1] = tgt[1] - loc[10];
    d[2] = tgt[2] - loc[11];
    u8* proj = *(u8**)(shot + 0x228);
    f32 speed = ComputeLobSpeed((const Vec3*)&loc[9], (const Vec3*)tgt, *(f32*)(proj + 0xD8), -*(f32*)(proj + 0xD4));
    if (speed > 0.0f) {
        d[2] = 0.0f;
        f32 len2 = (d[2] * d[2]) + ((d[0] * d[0]) + (d[1] * d[1]));
        f32 n[3];
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
            n[0] = d[0] * inv;
            n[1] = d[1] * inv;
            n[2] = d[2] * inv;
        }
        f32 s = (f32)sinf(1.5707964f * *(f32*)(target + 0xD8));
        f32 c = (f32)cosf(1.5707964f * *(f32*)(target + 0xD8));
        f32 v[3];
        v[0] = (n[0] * c) + 0.0f;
        v[1] = (n[1] * c) + 0.0f;
        v[2] = (n[2] * c) + s;
        *(f32*)(shot + 0x1AC) = *(f32*)(shot + 0x1AC) * speed;
        *(f32*)(shot + 0x1B0) = *(f32*)(shot + 0x1B0) * speed;
        *(f32*)(shot + 0x1B4) = *(f32*)(shot + 0x1B4) * speed;
        fn_80065200(shot, (u8*)&loc[9], v, obj, 0, 0x4000, 0x4000);
    } else {
        f32 len2 = (d[2] * d[2]) + ((d[0] * d[0]) + (d[1] * d[1]));
        f32 n[3];
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
            n[0] = d[0] * inv;
            n[1] = d[1] * inv;
            n[2] = d[2] * inv;
        }
        fn_80065200(shot, (u8*)&loc[9], n, obj, 0, 0, 0);
    }
}

// 0x800F548C
// Plays the footstep effect for a fighter: decides from two body points (hashes 0x14/0x19 of the
// emitter table) which side of the facing axis the step is on, then spawns the ability effect of the
// current ground type at that emitter.
void PlayFootStepEffect(void* a0) {
    u8* p = (u8*)a0;
    u8* ents = p + 0x94;
    u32 hashB = lbl_803881B4[0x14];
    s32 idxA = (s32)fn_801EC880(ents, lbl_803881B4[0x19]);
    s32 idxB = (s32)fn_801EC880(ents, hashB);
    s32 side = -1;
    if (idxA < 0 || idxB < 0) return;

    u8* body = *(u8**)(p + 4);
    u8* table = *(u8**)(ents + 4);
    f32 cx = *(f32*)(body + 0x0);
    f32 cy = *(f32*)(body + 0x4);
    f32 cz = *(f32*)(body + 0x8);
    u8* eB = table + idxB * 0xC8;
    u8* eA = table + idxA * 0xC8;
    f32 d1x = *(f32*)(eA + 0x5C) - cx;
    f32 d1y = *(f32*)(eA + 0x60) - cy;
    f32 d1z = *(f32*)(eA + 0x64) - cz;
    f32 d2x = *(f32*)(eB + 0x5C) - cx;
    f32 d2y = *(f32*)(eB + 0x60) - cy;
    f32 d2z = *(f32*)(eB + 0x64) - cz;
    Vec3 ax1 = QuatGetAxisY((const Quat*)(*(u8**)(p + 4) + 0xC));
    f32 dot1 = (ax1.z * d1z) + ((ax1.x * d1x) + (ax1.y * d1y));
    Vec3 ax2 = QuatGetAxisY((const Quat*)(*(u8**)(p + 4) + 0xC));
    f32 dot2 = (ax2.z * d2z) + ((ax2.x * d2x) + (ax2.y * d2y));
    if (dot1 > dot2) {
        side = 0x19;
    } else if (dot2 > dot1) {
        side = 0x14;
    }

    s32 ground = fn_80216754(gGameDb, *(u32*)(*(u8**)gWorldState + 0x1E4));
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* obj = (*(GetObjFn*)(*(u8**)p + 0xCC))(p);
        u8* t = *(u8**)(*(u8**)(obj + 0x24) + 0x1C);
        if (t != NULL) t -= 0xC48;
        u8* u = *(u8**)(t + 0xA00);
        if (u != NULL) {
            s32 v = fn_800D5800(u);
            if (v != -1) ground = v;
        }
    }
    if (*(s32*)((u8*)gGameDb + 0x10) != 1) return;
    if (ground == -1) ground = 0;
    s32 ev = *(s32*)(*(u8**)(*(u8**)((u8*)gGameDb + 0xC) + 0x46B0) + ground * 0xC + 8);
    u32 emitter = lbl_803881B4[side];
    u32 effect = (ev != -1) ? kAbilityEffects[ev] : kAbilityEffects[0x166];
    if (p == NULL) return;
    if ((*(u32*)(*(u8**)(p + 4) + 0x1C) & 0x1000) != 0) {
        u8* o = (*(GetObjFn*)(*(u8**)p + 0xCC))(p);
        if (o != NULL && gBattle != NULL) {
            u8* o2 = (*(GetObjFn*)(*(u8**)p + 0xCC))(p);
            if (fn_80212E28((u8**)(o2 + 0x28)) != 0) return;
        }
    }
    s32 idx = (s32)fn_801EC880(p + 0x94, emitter);
    if (idx < 0) return;
    u8* e = *(u8**)(p + 0x94 + 4) + idx * 0xC8;
    f32 pos[4];
    pos[0] = *(f32*)(e + 0x5C);
    pos[1] = *(f32*)(e + 0x60);
    pos[2] = *(f32*)(e + 0x64);
    pos[3] = 0.0f;
    SpawnEffectAt(lbl_8047C760, effect, pos, 0);
}

// 0x801B203C
// Casts the ray query q through the object list of a world: finds the world entry whose hash (+0x8)
// equals `filter`, then runs the query's virtual test (vtable +0x14) against each active object of
// that entry. A hit fills `hit` / `hit2` (hit entity, entry, object). For query types 5 and 6 the
// closest hit (smallest depth at +0x18) is kept and the search continues over all objects.
BOOL RayQueryCast(u8* q, void* world, u32 filter, u8* hit, u8* hit2) {
    u8* w = (u8*)world;
    if (w == NULL) return 0;
    u8* entry = NULL;
    s32 n = *(s32*)(w + 0x10);
    for (s32 i = 0; i < n; i++) {
        u8* e = *(u8**)(*(u8**)(w + 0xC) + i * 4);
        if (e != NULL && filter == *(u32*)(e + 0x8)) {
            entry = *(u8**)(*(u8**)(w + 0xC) + i * 4);
            break;
        }
    }
    if (entry == NULL || *(u8*)(entry + 0x1C) == 0) return 0;
    BOOL result = 0;
    if (q == NULL) return 0;
    if ((u32)(*(s32*)(q + 0xC) - 5) <= 1u) {
        *(f32*)(hit + 0x18) = 10000000.0f;
    }
    u32 count = *(u32*)(entry + 0x28);
    u32 save[27] = { 0 };   // copy of the hit record while the next object is tested
    for (u32 i = 0; i < count; i++) {
        u8* obj;
        if (i >= *(u32*)(entry + 0x28)) obj = NULL;
        else obj = *(u8**)(*(u8**)(entry + 0x24) + i * 4);
        if (obj == NULL || *(u8*)(obj + 0x38) == 0) continue;
        if ((u32)(*(s32*)(q + 0xC) - 5) <= 1u) memcpy(save, hit, 0x6C);
        if ((*(CastFn*)(*(u8**)q + 0x14))(q, hit, obj, hit2, 0, 0, 0) == 0) continue;
        if ((u32)(*(s32*)(q + 0xC) - 5) <= 1u && *(f32*)(hit + 0x18) > *(f32*)&save[6]) {
            memcpy(hit, save, 0x6C);   // not closer than the previous hit: restore it
        }
        result = 1;
        *(s32*)(hit + 0x5C) = *(s32*)(*(u8**)(entry + 0x4) + 0x8);
        *(u8**)(hit + 0x58) = q;
        *(u8**)(hit + 0x60) = *(u8**)(entry + 0x4);
        *(u8**)(hit + 0x64) = entry;
        *(u8**)(hit + 0x68) = obj;
        *(u8**)(hit2 + 0x5C) = NULL;
        *(u8**)(hit2 + 0x58) = obj;
        *(u8**)(hit2 + 0x60) = NULL;
        *(u8**)(hit2 + 0x64) = NULL;
        *(u8**)(hit2 + 0x68) = q;
        s32 type = *(s32*)(q + 0xC);
        if (type != 6 && type != 5) break;
    }
    return result;
}

// 0x80109C98
// Handles one decoded network message (type = bits 20..26 of the first word) for the game's net
// lobby: forwards the payload to the battle object, shows or hides the alert box, and adjusts the
// pad / HBM state. Returns 1 for handled types, 0 otherwise.
u32 /*0x06*/ GameNetDistributeChunkCallback(u32 /*0x18*/ a0) {
    u8* msg = (u8*)(uintptr_t)a0;   // really a message pointer (the signature table types it as a u32)
    u32 type = (*(u32*)msg >> 20) & 0x7F;
    switch (type) {
    case 9:
        return 1;
    case 50:
        fn_8010A49C(msg + 4, (s32)type);  // r4 still holds the type in the asm
        return 1;
    case 51:
        fn_8010A49C(msg + 4, (s32)type);  // r4 still holds the type in the asm
        if (gBattle != NULL) fn_80220A8C(gBattle + 4, *(s32*)(msg + 4 + 0x2C));
        return 1;
    case 52: {
        u8* q = msg + 4;
        fn_8010A49C(q, (s32)type);  // r4 still holds the type in the asm
        if (gBattle != NULL) {
            u8* base = *(u8**)(gBattle + 0xA04);
            if (base != NULL) {
                u8** slot = (u8**)(base + 0x20);
                if (slot != NULL) {
                    u8* o = *slot;
                    if (o != NULL) (*(MsgFn*)(*(u8**)o + 0x10))(o, q);
                }
            }
        }
        return 1;
    }
    case 53: {
        u8* q = msg + 4;
        fn_8010A49C(q, (s32)type);  // r4 still holds the type in the asm
        if (gBattle != NULL) {
            u8* o = *(u8**)(gBattle + 0xA58);
            if (o != NULL) (*(MsgFn*)(*(u8**)o + 0x14))(o, q);
        }
        return 1;
    }
    case 54:
        fn_801F57A4(1);
        if (lbl_805FB378 != 0) {
            SetResponse(0x099E8EEDu);
            HideAlertBox();
        } else {
            fn_801F57B8();
            fn_8013A4A0(*(s32*)(msg + 8), *(s32*)(msg + 0xC));
        }
        return 1;
    case 55:
        lbl_805FB378 = 0;
        SetResponse(0xAB6207C8u);
        HideAlertBox();
        fn_801F5814(1);
        EventPost(0x48, NULL);
        return 1;
    case 56:
        SetResponse(0x099E8EEDu);
        HideAlertBox();
        return 1;
    case 57:
        GameFlowSignal(0x7514E973, 0, 0);
        return 1;
    case 58: {
        ScopedLock lock;
        ScopedLockCtor(&lock);
        ScopedLockDtor(&lock, -1);
        return 1;
    }
    case 59:
        fn_801F5724(0x8000);
        return 1;
    case 60:
        fn_801F5724(0x1000);
        fn_801F5814(1);
        EventPost(0x48, NULL);
        return 1;
    case 61:
        lbl_805FB23C = *(u32*)(msg + 4);
        return 1;
    case 62:
        fn_8013E41C(*(s32*)(msg + 4));
        fn_8013E424(*(s32*)(msg + 8));
        if (lbl_805F9030 != 0x40) {
            if ((*(u32*)(lbl_80556E80 + lbl_805F9030 * 32 + 0x35C) & 1) != 0) fn_8013C06C(msg + 0xC);
        }
        return 1;
    case 63: {
        fn_8013E3BC((const char*)(msg + 0x14));
        fn_8013E3D0(*(f32*)(msg + 4));
        fn_8013E3D8(*(f32*)(msg + 8));
        fn_8013E3E0(*(f32*)(msg + 0xC));
        fn_8013E3E8(*(f32*)(msg + 0x10));
        u8* q = msg;
        u8* dst = lbl_803E69D8;
        for (s32 i = 0; i < 5; i++) {
            *(u32*)(dst + 0x0) = *(u32*)(q + 0x24);
            *(u8*)(dst + 0x4) = *(u8*)(q + 0x28);
            *(u8*)(dst + 0x5) = *(u8*)(q + 0x29);
            *(u32*)(dst + 0x8) = *(u32*)(q + 0x2C);
            *(u32*)(dst + 0xC) = *(u32*)(q + 0x30);
            switch (i) {
            case 0: *(u32*)(lbl_803F2250 + 0x14) = *(u32*)(q + 0x24); break;
            case 1: *(u32*)(lbl_803F2250 + 0x2C) = *(u32*)(q + 0x24); break;
            case 2: *(u32*)(lbl_803F2250 + 0x30) = *(u32*)(q + 0x24); break;
            case 3: *(u32*)(lbl_803F2250 + 0x1C) = *(u32*)(q + 0x24); break;
            case 4: *(u32*)(lbl_803F2250 + 0xC) = *(u32*)(q + 0x24); break;
            }
            q += 0x10;
            dst += 0x10;
        }
        fn_8013E45C(1);
        return 1;
    }
    case 64:
        fn_8013E42C(0);
        fn_8013E434(0);
        fn_8013E43C(0);
        return 1;
    case 65:
        fn_8013E43C(1);
        fn_80140B8C(0x10);
        return 1;
    case 66:
        fn_8013E434(0);
        fn_8013E43C(0);
        fn_80140B7C(0x10);
        return 1;
    case 67:
        fn_8013C06C(msg + 4);
        return 1;
    case 68:
        return 1;
    case 69:
        fn_801F5814(1);
        fn_80140B7C(1);
        fn_80140B8C(4);
        EventPost(0x48, NULL);
        return 1;
    case 70:
        fn_801F5814(1);
        fn_80140B7C(2);
        EventPost(0x48, NULL);
        fn_80140B30();
        return 1;
    case 71:
        fn_80140B7C(4);
        return 1;
    case 72:
        fn_801F5724(1);
        return 1;
    case 73:
        fn_8013E42C(1);
        return 1;
    case 74:
        fn_801F5814(1);
        fn_801F5724(0x4000);
        EventPost(0x48, NULL);
        return 1;
    case 75:
        fn_801F5814(1);
        SetResponse(0x0167B872u);
        HideAlertBox();
        KillMatchConnection();
        return 1;
    case 76: {
        u8* r = (u8*)EventPost(0x43, NULL);
        if (r != NULL) strcpy((char*)(r + 4), (const char*)(lbl_80556E80 + 0xB40));
        return 1;
    }
    case 77:
        fn_80140B7C(8);
        return 1;
    case 78:
        fn_801F5724(0x1000);
        fn_801F5814(1);
        EventPost(0x48, NULL);
        return 1;
    case 79:
        SetResponse(0xAFE5DE63u);
        HideAlertBox();
        return 1;
    default:
        return 0;
    }
}

// 0x801D4D30
// Reallocates: always a new block of `size` bytes with the old contents copied over (the old block is
// not freed here). Blocks inside the two fixed memory windows (starting at 0x90400000) and blocks of
// the active pool are bump-allocated from their pool; everything else comes from the main heap
// (fn_801EEB20) with a fallback to the second heap, whose old block is released again.
void* MemRealloc(void* ptr, u32 size, u32 flags) {
    u8* p = (u8*)ptr;
    (void)flags;
    void* res = NULL;
    BOOL inWindow = 0;
    if ((uintptr_t)p >= 0x90400000u &&
        (uintptr_t)p <= (uintptr_t)(u32)(*(s32*)gGlobalSettings + 0x90400000)) {
        inWindow = 1;
    }
    if (inWindow != 0 && lbl_805FB211 == 0) {
        u8* pool = lbl_803C7404;
        if (lbl_805FB7E1[0] != 0) res = (void*)1;
        else res = PoolAlloc(pool, size);
        if (res != NULL && p != NULL) memcpy(res, p, size);
        return res;
    }
    inWindow = 0;
    if ((uintptr_t)p >= 0x90400000u &&
        (uintptr_t)p <= (uintptr_t)(u32)(*(s32*)(gGlobalSettings + 0x30) + 0x90400000)) {
        inWindow = 1;
    }
    if (inWindow != 0 && lbl_805FB211 == 0) {
        u8* pool = lbl_803C7404;
        if (lbl_805FB7E1[0] != 0) res = (void*)1;
        else res = PoolAlloc(pool + 0x20, size);
        if (res != NULL && p != NULL) memcpy(res, p, size);
        return res;
    }
    s32 slot = 0;
    if (fn_801088F0(p, lbl_805F95F8, &slot) != 0) {
        if (lbl_805FB211 != 0) {
            lbl_805FB7A0[slot] = lbl_805FB7A0[slot] + 1;
            u8* pool = lbl_803C7404 + (lbl_805F95F8 << 5);
            if (lbl_805FB7E1[0] != 0) res = (void*)1;
            else res = PoolAlloc(pool, size);
            if (res != NULL && p != NULL) memcpy(res, p, size);
        }
        return res;
    }
    fn_801088F0(p, -1, &slot);
    void* out;
    if (p != NULL) {
        lbl_805FB7D4 += 1;
        if (lbl_805FB7D4 == 0) lbl_805FB7D0 += 1;
        out = fn_801EF484(HeapOf(p), p, size);
    } else {
        lbl_805FB7D4 += 1;
        if (lbl_805FB7D4 == 0) lbl_805FB7D0 += 1;
        out = fn_801EEB20(lbl_8047CAC0, size);
    }
    if (out == NULL && *(s32*)(lbl_805FB7A8 + 4) != 0) {
        out = fn_801EEB20(lbl_8047CAC0 + 0x394, size);
        if (p != NULL) {
            memcpy(out, p, *(u32*)(p - 4) & ~3u);
            fn_801EF070(HeapOf(p), p);
        }
    }
    return out;
}

// 0x80228604
// Awards the tiered badges `badges[0..n-1]`: a badge is given when it is not yet owned and `value`
// has reached the threshold in row `rows[i]` of the tuning table at gGameDb->data + 0x99D0 (8-byte
// rows, float at +4). Goes from the last tier down.
void SaveAddTieredBadges(u8* save, s32 value, s32 n, const s32* rows, const s32* badges) {
    for (s32 i = n - 1; i >= 0; i--) {
        s32 badge = badges[i];
        if (SaveHasBadge(save, (u8)badge)) continue;
        s32 threshold = (s32)*(f32*)(*(u8**)(gGameDb->data + 0x99D0) + rows[i] * 8 + 4);
        if (value < threshold) continue;
        if (badge == -1) continue;
        SaveAddBadge(save, badge);
    }
}

// 0x80225674
// Adds the time since the last call (`now` is in seconds, kept at +0x140) to the total play time
// (+0x900) and awards the five play-hours badges whose thresholds the total has reached.
void SaveAddPlayTime(void* save, s64 now) {
    u8* s = (u8*)save;
    s64 delta = now - *(s64*)(s + 0x140);
    *(s64*)(s + 0x140) = now;
    s64 total = *(s64*)(s + 0x900) + delta;
    *(s64*)(s + 0x900) = total;
    s32 hours = (s32)(u32)((u64)total / 60) / 60;
    SaveAddTieredBadges(s, hours, 5, lbl_80349C30, lbl_80349C44);
}

// 0x801FB87C
// Loads the Home Button menu data: the language archive plus four fixed archives from "HomeButton2/",
// registers the layout / sound / message buffers in the HBM record, allocates its work memory and the
// 0x1D128-byte archive memory, sets the aspect adjustment from the TV format, and registers the
// per-frame update.
void HomeButtonInit(void) {
    char path[0x80];
    u8 file[0x60];
    s32 readSize = 0;
    u8* buf;
    lbl_805FBA2C = 0;
    for (s32 i = 0; i < 4; i++) {
        *(f32*)(lbl_804FB8A0 + i * 0x10 + 0x4) = 0.0f;
        *(f32*)(lbl_804FB8A0 + i * 0x10 + 0x8) = 0.0f;
        *(s32*)(lbl_804FB8A0 + i * 0x10 + 0xC) = 0;
    }
    const char* dir = (const char*)lbl_80349010;
    strcpy(path, dir);
    u32 lang = SCGetLanguage();
    *(u32*)(lbl_804FB860 + 0x1C) = lang;
    if (lang > 6) {
        *(s32*)(lbl_804FB860 + 0x1C) = 0;
        strcat(path, lbl_80386528);
    } else {
        strcat(path, lbl_80386528 + lang * 0x10);
    }
    buf = LoadHomeArc(file, path, NULL);
    *(u8**)(lbl_804FB860 + 0x0) = buf;

    strcpy(path, dir);
    strcat(path, lbl_80386528 + 0x70);
    buf = LoadHomeArc(file, path, NULL);
    *(u8**)(lbl_804FB860 + 0x4) = buf;

    strcpy(path, dir);
    strcat(path, lbl_80386528 + 0x80);
    buf = LoadHomeArc(file, path, NULL);
    *(u8**)(lbl_804FB860 + 0x8) = buf;

    strcpy(path, dir);
    strcat(path, lbl_80386528 + 0x8C);
    buf = LoadHomeArc(file, path, &readSize);
    if (buf != NULL) *(s32*)(lbl_804FB860 + 0x28) = readSize;
    *(u8**)(lbl_804FB860 + 0xC) = buf;
    *(void**)(lbl_804FB860 + 0x14) = (void*)fn_801FB0D8;
    *(s32*)(lbl_804FB860 + 0x18) = 0;
    *(s32*)(lbl_804FB860 + 0x20) = 0;

    strcpy(path, dir);
    strcat(path, lbl_80386528 + 0x98);
    buf = LoadHomeArc(file, path, NULL);
    lbl_805FBA24 = buf;

    u8* mem = (u8*)MEMAllocFromAllocator(&lbl_803F2870, 0x80000);
    lbl_805FBA2C = lbl_805FBA2C + 0x80000;
    *(u8**)(lbl_804FB860 + 0x10) = mem;
    *(s32*)(lbl_804FB860 + 0x2C) = 0x80000;
    *(s32*)(lbl_804FB860 + 0x3C) = 0;
    *(s32*)(lbl_804FB860 + 0x24) = 0;
    *(f32*)(lbl_804FB860 + 0x34) = 1.3684211f;
    *(f32*)(lbl_804FB860 + 0x38) = 1.0f;
    u32 fmt = VIGetTvFormat();
    if (fmt == 1) {
        *(f32*)(lbl_804FB860 + 0x30) = 1.2f;
    } else if (fmt == 0 || fmt == 5) {
        *(f32*)(lbl_804FB860 + 0x30) = 1.0f;
    }
    fn_802C8AB4(lbl_804FB860);
    if (SCGetAspectRatio() == 1) fn_802C8DE4(1);
    else fn_802C8DE4(0);

    lbl_805FBA28 = (u8*)MEMAllocFromAllocator(&lbl_803F2870, 0x1D128);
    lbl_805FBA2C = lbl_805FBA2C + 0x1D128;
    fn_802C8E00(lbl_805FBA24, lbl_805FBA28, 0x1D128);
    fn_80142878((u8*)fn_801FB760);
}

}  // extern "C"
