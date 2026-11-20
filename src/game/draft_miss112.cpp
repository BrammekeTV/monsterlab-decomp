// Misc decompiled functions: effect-pool entry unlink, part-slot init, lightning tower, matchup/inspect UI, level preload, ...
#include <math.h>
#include <stdint.h>

#include "types.h"
#include "game/part.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct StringTable;
struct Seeker;
struct Mover;

extern "C++" {
void  PredictTargetPos(u8* self, Seeker* seeker, Mover* target, Vec3* out, f32 dt);   // 0x8000C604 (game_spawn.cpp)
u8*   fn_801B55A8(u8* ent);                                           // 0x801B55A8 (locator count, returned as an integer value)
BOOL  SaveFileExists(const char* dir, const char* file);              // 0x80011960
s32   SaveGameRead(const char* name, void* data, u32 size);           // 0x800113CC
s32   SaveGameWrite(const char* name, const void* data, u32 size);    // 0x80011024
}

extern "C" {
u32   AlertGetLastError(void);                                         // 0x80140B58
u8*   GetGlobalSettings(void);
BOOL  MatchupIsActive(void);
void  MatchupSetReady(BOOL ready);                                     // 0x8013F2A4
void  PartSetDef(PartInstance* p, s32 def);                            // 0x8021D43C
void  PartSetDefect(u8* p, s32 d);                                     
void  PartSetEnhancement(u8* p, s32 e);
void  PartSetHealth(PartInstance* p, s32 hp);
void  PartSetQuality(PartInstance* p, s32 q);
void  PartSetTier(PartInstance* p, u8 tier);
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);         // 0x80209E70
u32   RandomU32(void);                                                 // 0x80143460
void* ScreenSetFindElement(void* set, u32 hash);                       // 0x80198F18
void* ScreenSetFindWidget(void* set, u32 hash);
void* ScreenSetsCurrent(void* sets);                                   // 0x8018D1E4
void* SpawnHelper(void* owner, s32 arg);                               // 0x8000C478
s32   SpecPower(const u8* spec);                                       // 0x8021B25C
u32   StringHash(const char* s);
int   SysSprintf(char* buf, const char* fmt, ...);                     // 0x80142EFC
void  UIPlayAnim(void* el, u32 anim);                                  // 0x8019BE40
void  UISetProgress(void* obj, f32 t);
void  UISetText(void* el, const char* text, s32 a);
void* UIWidgetFindChild(void* widget, u32 hash);                       // 0x80199AEC
void  WorldLoadLevelZones(s32 level);                                  // 0x80126AB0
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);   // 0x8032F2E0
void  fn_8001D4FC(u8* mgr);
void  fn_8004C718(u8* p, s32 idx, void* elem);
s32   fn_8004D3F8(void* c, const char* name, s32 a, s32 b);
s32   fn_8004E300(void* a, s32 b);
u8*   fn_8004EDD0(u8* obj, s32 flag, s32 a2);
s32   fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6);   // 0x80065200
u8*   fn_8006FBDC(void);                                               // 0x8006FBDC
void  fn_800974AC(u8* obj, s32 a);                                     // 0x800974AC
void  fn_80123954(s32 v, u8* save);                                    // 0x80123954
void  fn_80133698(u8* world, s32 zone);                                // 0x80133698
s32   fn_80133840(u8* world, s32 zone);                                // 0x80133840
const char* fn_8013E3F0(void);                                         // 0x8013E3F0 (returns &lbl_803F2088)
f32   fn_8013E3FC(void);                                               // 0x8013E3FC
f32   fn_8013E404(void);                                               // 0x8013E404
f32   fn_8013E40C(void);                                               // 0x8013E40C
f32   fn_8013E414(void);                                               // 0x8013E414
u8    fn_8013E444(void);                                               // 0x8013E444 (returns lbl_805FB3B3)
s32   fn_8013E44C(void);                                               // 0x8013E44C (returns lbl_805FB3CC)
const char* fn_8013E774(void);                                         // 0x8013E774 (returns &lbl_803F2210)
void  fn_80140B8C(s32 mask);                                           // 0x80140B8C (clears bits in lbl_805FB418)
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);            // 0x80146180 (rotate quat about axis)
void  fn_8018A948(u8* obj, f32 t);                                     // 0x8018A948
s32   fn_801A3010(void* obj, void* data);                              // 0x801A3010
void* fn_801B7198(void* anim, s32 a);                                  // 0x801B7198
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                          // 0x801B55F4
BOOL  fn_801C6634(u8* col);                                            // 0x801C6634
s32   fn_801C71EC(u8* key);                                            // 0x801C71EC
u32   fn_801D3A84(s32 n);                                              // 0x801D3A84
void* fn_801D40E0(s32 n);                                              // 0x801D40E0
s32   fn_801E5F48(u8* pool, const char* name, u32 hash);               // 0x801E5F48
s32   fn_801E6040(u8* pool, u8* name, s32 hash);                       // 0x801E6040
s32   fn_801E6374(u8* pool, u8* name, u8* name2, u32 dst, u32 size, s32 hash);   // 0x801E6374
void  fn_801F5724(u32 button);                                         // 0x801F5724
void* fn_80209814(void* v);                                            // 0x80209814
void* fn_8022ED60(void* p);                                            // 0x8022ED60
u8*   fn_8022ED98(u8* p, s32 del);                                     // 0x8022ED98
void  fn_8032F1E8(void* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);   // 0x8032F1E8
void* memcpy(void* d, const void* s, size_t n);
void* memset(void* p, int c, unsigned long n);
void  operator_delete(void* p);                                        // 0x801D426C
char* strcpy(char* d, const char* s);

extern "C++" u8    gIgcMode;       // 0x805FAF64
extern "C++" void* gScreenSets;    // 0x805FB654
extern "C++" StringTable* gStringTable;   // 0x805FB910
extern "C++" u32*  gZoneBase;      // 0x803C7434 (world_alloc.cpp)
extern u8   gIdPool[];             // 0x8048A8F0
extern u8   gLevelFlags[];         // 0x80369D60: 0x20 bytes per level, name pointer at +0, hash at +4
extern u8   gWorldState[];         // 0x803E6D00
extern s8   lbl_803F2098[];        // 0x803F2098 (text buffer)
extern u8   lbl_803F20C4[];        // 0x803F20C4
extern f32  lbl_80346014[];        // 0x80346014 (3-float offset)
extern u8   lbl_80556E80[];        // 0x80556E80
extern const char lbl_8036A5F0[];  // 0x8036A5F0 (string)
extern const char lbl_805F8200[];  // 0x805F8200 (empty string)
extern f32  lbl_805F9070;          // 0x805F9070 (inspect widget position x, -1 = unset)
extern f32  lbl_805F9074;          // 0x805F9074
extern f32  lbl_805F9078;          // 0x805F9078
extern u8   lbl_805F9080;          // 0x805F9080
extern u8   lbl_805F9081;          // 0x805F9081
extern f32  lbl_805F9084;          // 0x805F9084 (matchup element position x, -1 = unset)
extern f32  lbl_805F9088;          // 0x805F9088
extern f32  lbl_805F908C;          // 0x805F908C
extern f32  lbl_805F9090;          // 0x805F9090
extern f32  lbl_805F9094;          // 0x805F9094
extern f32  lbl_805F9098;          // 0x805F9098
extern u8*  lbl_805FAF78;          // 0x805FAF78 (lightning manager)
extern s32  lbl_805FB2C8;          // 0x805FB2C8
extern s32  lbl_805FB2CC;          // 0x805FB2CC
extern u8*  lbl_805FB2D0;          // 0x805FB2D0
extern u8   lbl_805FB2D5;          // 0x805FB2D5
extern s32  lbl_805FB3A8;          // 0x805FB3A8
extern s32  lbl_805FB3AC;          // 0x805FB3AC
extern s8   lbl_805FB390;          // 0x805FB390
extern s32  lbl_805FB394;          // 0x805FB394
extern s8   lbl_805FB3B0;          // 0x805FB3B0
extern s8   lbl_805FB3B1;          // 0x805FB3B1
extern u8   lbl_805FB3B2;          // 0x805FB3B2
extern u8*  lbl_805FB3BC;          // 0x805FB3BC
extern u8*  lbl_805FB3C8;          // 0x805FB3C8
extern s32  lbl_805FB3CC;          // 0x805FB3CC
extern s32  lbl_805FB3E0;          // 0x805FB3E0
extern s32  lbl_805FB3E8;          // 0x805FB3E8
extern f32  lbl_805FB410;          // 0x805FB410
extern u8   lbl_805FB414;          // 0x805FB414
extern u8   lbl_805FB710[];        // 0x805FB710 (3-byte default key)
extern u8*  lbl_805FB8D8;          // 0x805FB8D8 (effect pool: 0xFF entries of 0x80 bytes, free list at +0x7F80, count at +0x837C)
extern void* lbl_805FBBAC;         // 0x805FBBAC
extern u8*  lbl_805FBBE0;          // 0x805FBBE0 (save block pointer)
extern f32  lbl_80600020;          // 0x80600020 (0.0f)
extern f32  lbl_805FDC48;          // 0x805FDC48 (0.16f)
extern f32  lbl_805FEFA0;          // 0x805FEFA0 (-1.0f sentinel)
extern f32  lbl_805FEFA4;          // 0x805FEFA4 (0.0f)
extern f32  lbl_805FEFB0;          // 0x805FEFB0 (-1.0f sentinel)
}

static const u32 kHashHealth = 0x3E22E495u;   // "health"
static const u32 kHashEnergy = 0x67E9BE17u;   // "energy"
static const u32 kHashAttack = 0xB738EABDu;   // "attack"
static const u32 kHashDefense = 0xE4E00B6Fu;  // "defense"
static const u32 kHashTimer = 0x9D841CAAu;    // "timer"
static const u32 kHashDeactivate = 0xF5A9CE6Cu;   // "deactivate"
static const u32 kHashConnecting = 0x42AFE7BCu;   // "connecting"
static const u32 kHashAttention = 0x4CE15F62u;    // "attention"
static const u32 kHashAlertSound = 0xB74304F3u;   // "ui_general_action_alert"
static const u32 kHashEmptyLevel = 0xE4EBDBE3u;   // "empty_level"

// Returns the effect-pool record for a handle (low byte = index, high byte = generation), or NULL.
static u8* PoolRecord(u16 h) {
    u8 idx = (u8)h;
    if (idx < 0xFF && ((h >> 8) & 0xFF) != 0) return lbl_805FB8D8 + ((u32)idx << 7);
    return NULL;
}

// Handle type: bit 7 set means type 1, otherwise the low nibble.
static u32 RefType(u8 b) {
    return (b & 0x80) ? 1 : (b & 0xF);
}

// Nibble field of a reference byte (0 when bit 7 is set).
static u8 RefNibble(u8 b) {
    return (b & 0x80) ? 0 : ((b >> 4) & 0xF);
}

extern "C" {

// 0x801E93EC
// Unlinks an effect-pool entry from the doubly linked list (head at +0, tail at +2, count at +4;
// entry links at +0x78/+0x7A, own handle at +0x7C), clears the pool record and returns its index
// to the free list.
void fn_801E93EC(u8* list, u8* entry) {
    if (*(u16*)(list + 6) & 1) return;
    u16 h = *(u16*)(entry + 0x7C);
    u16 head = *(u16*)(list + 0);
    if (h == head) {
        if (head == *(u16*)(list + 2)) {
            *(u16*)(list + 2) = 0xFFFF;
            *(u16*)(list + 0) = 0xFFFF;
        } else {
            u8* next = PoolRecord(*(u16*)(entry + 0x78));
            *(u16*)(next + 0x7A) = 0xFFFF;
            *(u16*)(list + 0) = *(u16*)(next + 0x7C);
        }
    } else if (h == *(u16*)(list + 2)) {
        u8* prev = PoolRecord(*(u16*)(entry + 0x7A));
        *(u16*)(prev + 0x78) = 0xFFFF;
        *(u16*)(list + 2) = *(u16*)(prev + 0x7C);
    } else {
        u8* prev = PoolRecord(*(u16*)(entry + 0x7A));
        u8* next = PoolRecord(*(u16*)(entry + 0x78));
        *(u16*)(prev + 0x78) = *(u16*)(next + 0x7C);
        *(u16*)(next + 0x7A) = *(u16*)(prev + 0x7C);
    }
    u8* pool = lbl_805FB8D8;
    s32 valid = 0;
    if ((u8)h < 0xFF && ((h >> 8) & 0xFF) != 0 && *(u16*)(pool + ((u32)(u8)h << 7) + 0x7C) == h) valid = 1;
    if (valid) {
        u32 off = ((u32)h << 7) & 0x7F80;
        memset(pool + off, 0, 0x64);
        u8* rec = pool + off;
        *(s32*)(rec + 0x64) = 0;
        *(s32*)(rec + 0x68) = 0;
        *(s32*)(rec + 0x6C) = 0;
        *(f32*)(rec + 0x70) = lbl_80600020;
        *(u16*)(rec + 0x78) = 0xFFFF;
        *(u16*)(rec + 0x7A) = 0xFFFF;
        *(u16*)(rec + 0x7C) = 0xFFFF;
        *(s32*)(rec + 0x74) = 0;
        *(u8*)(rec + 0x7E) = 0;
        *(u8*)(rec + 0x7F) = 0;
        if (*(s32*)(pool + 0x837C) + 1 != 0xFF) {
            s32 n = *(s32*)(pool + 0x837C) + 1;
            *(s32*)(pool + 0x837C) = n;
            *(s32*)(pool + 0x7F80 + n * 4) = (u8)h;
        }
    }
    *(u16*)(list + 4) = (u16)(*(u16*)(list + 4) - 1);
}

// 0x801ED368
// Validates a reference record: type 8 is always valid, type 2 needs a valid 3-byte key (falling
// back to the default key), type 3 needs a valid color record. 0xFFFF in the second field is invalid.
s32 fn_801ED368(u8* ref) {
    if (RefType(*(u8*)ref) == 8) return 1;
    if (*(u16*)(ref + 2) == 0xFFFF) return 0;
    if (RefType(*(u8*)ref) == 2) {
        u8 key[4];
        key[0] = lbl_805FB710[0];
        key[1] = lbl_805FB710[1];
        *(u16*)(key + 2) = *(u16*)(lbl_805FB710 + 2);
        if (RefType(*(u8*)ref) == 2) {
            key[1] = *(u8*)(ref + 1);
            key[0] = RefNibble(*(u8*)ref);
            *(u16*)(key + 2) = *(u16*)(ref + 2);
        }
        u8 key2[4];
        if (fn_801C71EC(key) != 0) {
            key2[0] = key[0];
            key2[1] = key[1];
            *(u16*)(key2 + 2) = *(u16*)(key + 2);
        } else {
            key2[0] = lbl_805FB710[0];
            key2[1] = lbl_805FB710[1];
            *(u16*)(key2 + 2) = *(u16*)(lbl_805FB710 + 2);
        }
        if (fn_801C71EC(key2) == 0) return 0;
    }
    if (RefType(*(u8*)ref) == 3) {
        u8 col[4];
        col[0] = 0xFF;
        col[1] = 0;
        *(s16*)(col + 2) = 0;
        if (RefType(*(u8*)ref) == 3) {
            col[0] = (u8) * (u16*)(ref + 2);
            col[1] = *(u8*)(ref + 1);
            *(s16*)(col + 2) = (s16)(u8)RefNibble(*(u8*)ref);
        }
        if (fn_801C6634(col) == 0) return 0;
    }
    return 1;
}

// 0x8020EDF0
// Initializes the five part slots of an opponent record from a spec: calls virtual +0xC first,
// fills each slot's inline PartInstance (def, quality, tier, defect, enhancement), sets every
// part to full health, then copies the id, the two flags and the spec power.
void fn_8020EDF0(u8* dst, u8* src) {
    struct SlotSrc { u8 def, quality, tier, defect, enh; };
    static const SlotSrc kSlots[5] = {
        {0x14, 0x35, 0x3F, 0x20, 0x44},
        {0x2C, 0x42, 0x41, 0x24, 0x43},
        {0x30, 0x38, 0x37, 0x10, 0x39},
        {0x1C, 0x34, 0x36, 0x18, 0x40},
        {0x0C, 0x3D, 0x3C, 0x28, 0x3E},
    };
    typedef void (*Fn)(u8*);
    (*(Fn*)(*(u8**)dst + 0xC))(dst);
    for (s32 i = 0; i < 5; i++) {
        const SlotSrc& s = kSlots[i];
        if (*(s32*)(src + s.def) != -1) {
            u8* inst = dst + 0x4C + i * 0x30;
            *(u8**)(dst + 4 + i * 4) = inst;
            PartSetDef((PartInstance*)inst, *(s32*)(src + s.def));
            PartSetQuality(*(PartInstance**)(dst + 4 + i * 4), *(s8*)(src + s.quality));
            PartSetTier(*(PartInstance**)(dst + 4 + i * 4), *(u8*)(src + s.tier));
            PartSetDefect(*(u8**)(dst + 4 + i * 4), *(s32*)(src + s.defect));
            if (*(u8*)(src + s.enh) != 0) {
                PartDef def;
                PartDbGetDef(&def, gGameDb, *(s32*)(src + s.def));
                PartSetEnhancement(*(u8**)(dst + 4 + i * 4), def.unk20);
            }
        }
    }
    for (s32 i = 0; i < 5; i++) {
        PartInstance* p = *(PartInstance**)(dst + 4 + i * 4);
        if (p != NULL && p->defIndex != -1) {
            PartSetHealth(*(PartInstance**)(dst + 4 + i * 4), PartGetMaxHealth(p));
        }
    }
    *(s32*)(dst + 0x44) = *(s32*)(src + 0x8);
    *(u8*)(dst + 0x48) = (*(u8*)(src + 0x3A) == 0);
    *(u8*)(dst + 0x49) = (*(u8*)(src + 0x3B) == 0);
    *(s32*)(dst + 0x18) = SpecPower(src);
}

// 0x800EAB70
// Script callback FireShotAtFeet: picks a locator on the shooter (by index or by name slot), spawns
// the projectile object and fires it at the target's feet with a random spread.
void FireShotAtFeet(void* a0, u32 /*0x30*/ a1, u32 /*0x38*/ a2, f32 a3, f32 a4) {
    u8* obj = (u8*)a0;
    u8* ref = (u8*)(uintptr_t)a1;
    u8* target = (u8*)(uintptr_t)a2;
    f32 loc[12];
    if (gIgcMode != 0) return;
    u16 flags = *(u16*)ref;
    s32 idx;
    if (flags & 1) idx = *(u8*)(ref + 2);
    else idx = -1;
    s32 have;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(obj)) {
            fn_801B55F4(obj, (u8*)loc, idx);
            have = 1;
        } else {
            have = 0;
        }
    } else {
        s32 idx2;
        if (flags & 2) idx2 = *(u8*)(ref + 2);
        else idx2 = -1;
        if (idx2 != -1) {
            have = 1;
            u8* src = *(u8**)(obj + 0x98) + idx2 * 0xC8;
            for (s32 i = 0; i < 12; i++) loc[i] = *(f32*)(src + 0x38 + i * 4);
        } else {
            have = 0;
        }
    }
    if (have == 0) return;
    s32 spawnArg = 0;
    s32 objFlags = *(s32*)(*(u8**)(obj + 4) + 0x1C);
    s32 refId = *(s32*)ref;
    if (objFlags & 0x20) {
        spawnArg = *(s32*)(obj + 0xA14);
    } else if (objFlags & 0x40) {
        *(s32*)(obj + 0x178) = refId;
        spawnArg = *(s32*)(obj + 0x18C);
        *(u8*)(obj + 0x190) = (u8)(*(u8*)(obj + 0x190) + 1);
    }
    u8* shot;
    if (spawnArg == 0) shot = NULL;
    else shot = (u8*)SpawnHelper(target, spawnArg);
    if (shot == NULL) return;
    f32 tgt[3];
    PredictTargetPos(target, (Seeker*)obj, *(Mover**)(shot + 0x248), (Vec3*)tgt, a4);
    f32 ox = a3 * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - a3;
    f32 oy = a3 * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - a3;
    f32 oz = a3 * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - a3;   // result is overwritten below, the RNG still advances
    (void)oz;
    tgt[0] = tgt[0] + ox;
    tgt[1] = tgt[1] + oy;
    f32 dx = tgt[0] - loc[9];
    f32 dy = tgt[1] - loc[10];
    tgt[2] = *(f32*)(*(u8**)(*(u8**)(shot + 0x248) + 4) + 0x8);
    f32 dz = tgt[2] - loc[11];
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    f32 dir[3];
    if (len2 == 0.0f) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        dir[0] = dx * inv;
        dir[1] = dy * inv;
        dir[2] = dz * inv;
    }
    fn_80065200(shot, (u8*)&loc[9], dir, obj, 0, 0, 0);
}

// 0x800743E0
// Script callback GetExperimentTypeFromStation: maps the station's model hash (object +0x9C ->
// +8) to the experiment type 0..11, or -1 when unknown.
u32 /*0x4D*/ GetExperimentTypeFromStation(u32 /*0x32*/ a0) {
    struct StationType { u32 hash; s32 type; };
    static const StationType kTypes[] = {
        {0x92871260u, 0}, {0x0B8E43DAu, 0}, {0x7C89734Cu, 0},            // 0x92871260 "brain_machineA"
        {0xDB05B1CCu, 1}, {0x350BD0E0u, 1}, {0x420CE076u, 1},            // 0xDB05B1CC "saw_machine_01A", 0x420CE076 "saw_machine_01B"
        {0xA3600262u, 2}, {0xD46732F4u, 2}, {0x4D6E634Eu, 2},            // 0x4D6E634E "reactor_machineA"
        {0x9B6A2315u, 3}, {0x026372AFu, 3}, {0x75644239u, 3},            // 0x9B6A2315 "robotic_evolutionA", 0x026372AF "robotic_evolutionB"
        {0xBD1461D1u, 4}, {0xCA135147u, 4}, {0x531A00FDu, 4},            // 0x531A00FD "fusion_weldingA"
        {0x9413299Eu, 5}, {0x0D1A7824u, 5}, {0x7A1D48B2u, 5},            // 0x9413299E "spirit_machineA", 0x0D1A7824 "spirit_machineB"
        {0x9709A54Au, 6}, {0xE00E95DCu, 6}, {0x7907C466u, 6},            // 0x7907C466 "astral_machineA"
        {0xBE109380u, 7}, {0xC917A316u, 7}, {0x2719C23Au, 7},            // 0x2719C23A "castle_brazier_machineA"
        {0x82BCADE3u, 8}, {0x1BB5FC59u, 8}, {0x6CB2CCCFu, 8},            // 0x82BCADE3 "staple_machineA", 0x1BB5FC59 "staple_machineB"
        {0xABDD70FCu, 9}, {0xDCDA406Au, 9}, {0x45D311D0u, 9},            // 0x45D311D0 "arm_wrestle_machineA"
        {0x011B533Au, 10}, {0xEF153216u, 10}, {0x761C63ACu, 10},         // 0xEF153216 "mystic_brew_machineA", 0x761C63AC "mystic_brew_machineB"
        {0xB0ED5F96u, 11}, {0xC7EA6F00u, 11}, {0x29E40E2Cu, 11},         // 0xB0ED5F96 "grow_juice_machineB", 0x29E40E2C "grow_juice_machineA"
    };
    u8* station = (u8*)(uintptr_t)a0;
    u32 hash = 0;
    u8* info = *(u8**)(station + 0x9C);
    if (info != NULL) hash = *(u32*)(info + 8);
    for (u32 i = 0; i < sizeof(kTypes) / sizeof(kTypes[0]); i++) {
        if (kTypes[i].hash == hash) return (u32)kTypes[i].type;
    }
    return (u32)-1;
}

// 0x8001BB90
// Script callback LightningTowerShowMonster: for each of the 5 tower slots either shows the monster
// (assigning the selected one) or, with a0 == 0, hides all of them; then triggers the "hanging" anim.
void LightningTowerShowMonster(u32 /*0x06*/ a0) {
    typedef u8* (*VFn)(u8*);
    if (lbl_805FAF78 != NULL && *(u8**)lbl_805FAF78 != NULL) {
        if (a0 != 0) {
            for (s32 i = 0; i < 5; i++) {
                u8* e = lbl_805FAF78 + i * 0x1A4;
                s32 sel = -1;
                if (*(s32*)(e + 0x1A0) > 0) {
                    s32 idx = *(s32*)(e + 8 + 0x190);
                    if (idx >= 0) {
                        s32 v = *(s32*)(e + 8 + idx * 4);
                        if (v >= 0) sel = **(s32**)(lbl_805FBBE0 + v * 4 + 0xA58);
                    }
                }
                u8* cur = *(u8**)(*(u8**)lbl_805FAF78 + i * 4 + 0xC50);
                u8* want = *(u8**)(lbl_805FAF78 + i * 4 + 0xC04);
                if (sel != -1 && cur != want) {
                    if (cur != NULL) fn_8004EDD0(cur, 1, 0);
                    u8* vobj = *(u8**)(lbl_805FAF78 + i * 0x1A4 + 0x1A8);
                    if ((*(VFn*)(*(u8**)vobj + 0xC))(vobj) != NULL) {
                        u8* r = (*(VFn*)(*(u8**)vobj + 0xC))(vobj);
                        *(u32*)(r + 8) = *(u32*)(r + 8) & 0xFFFFFFFDu;
                    }
                    u8* o = *(u8**)(vobj + 4);
                    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) & 0xFFFFFFFDu;
                    o = *(u8**)(*(u8**)(lbl_805FAF78 + i * 0x1A4 + 0x1A8) + 4);
                    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x8000u;
                } else if (sel == -1) {
                    u8* mgr = lbl_805FAF78;
                    u8* m = *(u8**)mgr;
                    u8* cur2 = *(u8**)(m + i * 4 + 0xC50);
                    u8* want2 = *(u8**)(mgr + i * 4 + 0xC04);
                    if (cur2 != want2 && cur2 == NULL) {
                        *(s32*)(mgr + i * 0x1A4 + 0x1A4) = 0;
                        fn_8004C718(m, i, NULL);
                        if (want2 != NULL) {
                            u8* elem = (*(VFn*)(*(u8**)want2 + 0xCC))(want2);
                            fn_8004C718(m, i, elem);
                            u8* cur3 = *(u8**)(m + i * 4 + 0xC50);
                            if (cur3 != NULL) fn_8004EDD0(cur3, 0, 1);
                            if (lbl_805FAF78 != NULL && *(u8**)lbl_805FAF78 != NULL) {
                                fn_8004D3F8(*(u8**)lbl_805FAF78, "hanging", 0, 1);
                            }
                        }
                    }
                }
            }
        } else {
            for (s32 i = 0; i < 5; i++) {
                u8* cur = *(u8**)(*(u8**)lbl_805FAF78 + i * 4 + 0xC50);
                if (cur != NULL) fn_8004EDD0(cur, 0, 1);
                u8* vobj = *(u8**)(lbl_805FAF78 + i * 0x1A4 + 0x1A8);
                if ((*(VFn*)(*(u8**)vobj + 0xC))(vobj) != NULL) {
                    u8* r = (*(VFn*)(*(u8**)vobj + 0xC))(vobj);
                    *(u32*)(r + 8) = *(u32*)(r + 8) | 2u;
                }
                u8* o = *(u8**)(vobj + 4);
                *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 2u;
                o = *(u8**)(*(u8**)(lbl_805FAF78 + i * 0x1A4 + 0x1A8) + 4);
                *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x8000u;
                fn_8001D4FC(lbl_805FAF78);
            }
        }
    }
    if (lbl_805FAF78 != NULL && *(u8**)lbl_805FAF78 != NULL) {
        fn_8004D3F8(*(u8**)lbl_805FAF78, "hanging", 0, 1);
    }
}

// 0x80099274
// Script callback ReactorHeartUpdateRotation: advances the heart's rotation about its axis
// (+0x288) by t = elapsed/duration, clamped to 0..1; at t == 1 the pending effect is finished and
// the particle emitter at +0x26C is removed. Then writes the quaternion to the object and flags it dirty.
void ReactorHeartUpdateRotation(void* a0) {
    u8* p = (u8*)a0;
    typedef u8* (*VFn)(u8*);
    if (*(s32*)(p + 0x268) == 0) return;
    f32 t = 0.0f;
    f32 ratio = *(f32*)(p + 0x270) / *(f32*)(p + 0x274);
    if (ratio < 0.0f) {
    } else {
        t = 1.0f;
        if (ratio > 1.0f) {
        } else {
            t = ratio;
        }
    }
    f32 q[4];
    q[0] = *(f32*)(p + 0x278);
    q[1] = *(f32*)(p + 0x27C);
    q[2] = *(f32*)(p + 0x280);
    q[3] = *(f32*)(p + 0x284);
    f32 neg = -*(f32*)(p + 0x294);
    fn_80146180(q, (const Vec3*)(p + 0x288), neg * t);
    if (t >= 1.0f) {
        fn_800974AC(fn_8006FBDC(), *(s32*)(p + 0x268));
        *(s32*)(p + 0x268) = 0;
        u8* emitter = *(u8**)(p + 0x26C);
        *(f32*)(p + 0x270) = 0.0f;
        if (emitter != NULL) {
            RemoveAttachedParticleEffectAtEmitterImmediate(emitter, 0x60D49509u);
            *(u8**)(p + 0x26C) = NULL;
        }
        *(f32*)(p + 0x274) = lbl_805FDC48;
    }
    u8* o = *(u8**)(p + 4);
    *(f32*)(o + 0xC) = q[0];
    *(f32*)(o + 0x10) = q[1];
    *(f32*)(o + 0x14) = q[2];
    *(f32*)(o + 0x18) = q[3];
    o = *(u8**)(p + 4);
    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x8000u;
    o = *(u8**)(p + 4);
    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x10000000u;
    o = *(u8**)(p + 4);
    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x4000u;
    o = *(u8**)(p + 4);
    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x10000000u;
    fn_8018A948((*(VFn*)(*(u8**)p + 0x10))(p), 0.0f);
}

// 0x80123D9C
// Script callback SaveManager_EraseData: removes save slot a0 (-1 = the current one): fixes up the
// current/last slot indices, loads the save file, shifts the later slots down, resets the last
// slot to a fresh one, rewrites the file (twice when the save block has no completed flag).
void SaveManager_EraseData(s32 a0) {
    s32 slot = a0;
    if (lbl_805FB2D5 != 0) return;
    if (a0 == -1) slot = lbl_805FB2C8;
    if (lbl_805FB2C8 == slot) lbl_805FB2C8 = 0;
    else if (slot < lbl_805FB2C8) lbl_805FB2C8 = lbl_805FB2C8 - 1;
    if (lbl_805FB2CC == slot) lbl_805FB2CC = -1;
    else if (slot < lbl_805FB2CC) lbl_805FB2CC = lbl_805FB2CC - 1;
    u8* save = lbl_805FB2D0;
    if (save == NULL) {
        save = (u8*)fn_801D40E0(0x31C0);
        if (save != NULL) {
            fn_8032F1E8(save + 0xB98, (void (*)(void))fn_8022ED60, (void (*)(void))fn_8022ED98, 0x988, 4);
        }
        SaveGameRead("save", save, 0x31C0);
    }
    if (slot < 3) {
        for (; slot < 3; slot++) {
            memcpy(save + 0xB98 + slot * 0x988, save + 0xB98 + (slot + 1) * 0x988, 0x988);
        }
    }
    u8 fresh[0x988];
    fn_8022ED60(fresh);
    memcpy(save + 0x2830, fresh, 0x988);
    fn_8022ED98(fresh, -1);
    s32 complete = *(s32*)(lbl_805FBBE0 + 0x924);
    fn_80123954(lbl_805FB2CC, save);
    SaveGameWrite("save", save, 0x31C0);
    if (complete == 0) {
        fn_80123954(lbl_805FB2CC, save);
        SaveGameWrite("save", save, 0x31C0);
    }
    if (lbl_805FB2D0 == NULL && save != NULL) {
        __destroy_arr(save + 0xB98, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
        operator_delete(save);
    }
}

// 0x8013C6CC
// Script callback InspectOpponentUpdate: while a matchup is active, fills the stat bars and name
// text once, keeps the inspect widget positioned (with the +0x80346014 offset while its animation
// is in state 13), and updates the matchup ready flag and the countdown text.
void InspectOpponentUpdate(void) {
    typedef void (*Fn)(u8*, s32);
    if (MatchupIsActive() == 0) return;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* widget = (u8*)ScreenSetFindWidget(set, 0xFEE5B157u);
    if (widget != NULL) {
        if (lbl_805FB390 == 0) {
            lbl_805FB390 = 1;
            u8* bar = (u8*)UIWidgetFindChild(widget, kHashHealth);
            if (bar != NULL) UISetProgress(bar, fn_8013E3FC());
            bar = (u8*)UIWidgetFindChild(widget, kHashEnergy);
            if (bar != NULL) UISetProgress(bar, fn_8013E404());
            bar = (u8*)UIWidgetFindChild(widget, kHashAttack);
            if (bar != NULL) UISetProgress(bar, fn_8013E40C());
            bar = (u8*)UIWidgetFindChild(widget, kHashDefense);
            if (bar != NULL) UISetProgress(bar, fn_8013E414());
            bar = (u8*)UIWidgetFindChild(widget, 0x2EADCD52u);
            if (bar != NULL) UISetText(bar, fn_8013E3F0(), 1);
        }
        u8* el = (u8*)UIWidgetFindChild(widget, 0x60B15D91u);
        if (el != NULL) {
            if (lbl_805FEFA0 == lbl_805F9070) {
                lbl_805F9070 = *(f32*)(el + 0x68);
                lbl_805F9074 = *(f32*)(el + 0x6C);
                lbl_805F9078 = *(f32*)(el + 0x70);
            }
            fn_801A3010(el, (void*)(intptr_t)fn_8013E44C());
            f32 pos[3];
            pos[1] = lbl_805F9074;
            pos[0] = lbl_805F9070;
            pos[2] = lbl_805F9078;
            void* anim = fn_801B7198(el + 0x140, 0);
            if (anim != NULL && fn_8004E300(anim, 1) == 0xD) {
                pos[0] = pos[0] + lbl_80346014[0];
                pos[1] = pos[1] + lbl_80346014[1];
                pos[2] = pos[2] + lbl_80346014[2];
            }
            *(f32*)(el + 0x68) = pos[0];
            *(f32*)(el + 0x6C) = pos[1];
            *(f32*)(el + 0x70) = pos[2];
            (*(Fn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
            if (!(*(s32*)(el + 0x64) & 2) && lbl_805FB394 == 0) lbl_805FB394 = 3;
            if (lbl_805FB394 > 0) {
                lbl_805FB394 = lbl_805FB394 - 1;
                if (lbl_805FB394 == 0) {
                    UIElement_SetVisibleFlag(0x379F79BDu, 0);
                    (*(Fn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
                }
            }
        }
    }
    if (fn_8013E444() == 1) MatchupSetReady(1);
    else MatchupSetReady(0);
    if (lbl_805FB414 == 1) {
        u8* w = (u8*)ScreenSetFindWidget(set, 0x43538242u);
        if (w != NULL && UIWidgetFindChild(w, kHashTimer) != NULL) {
            u8* txt = (u8*)UIWidgetFindChild(w, 0x29DD4A4Eu);
            if (txt != NULL) {
                if (lbl_805FEFA4 == lbl_805FB410) fn_801F5724(0x2000);
                char buf[16];
                SysSprintf(buf, "0:%0.02d", (s32)lbl_805FB410);
                UISetText(txt, buf, 1);
            }
        }
    }
}

// 0x8013CC60
// Script callback MatchupInit: resets the matchup UI state, caches the two screen elements'
// positions, fills the name/stat texts of the matchup widget, then clears two flag bits and
// switches to the MP section with hit boxes on. Does nothing while the last alert is 0x9D31BDAB.
void MatchupInit(void) {
    if (AlertGetLastError() == 0x9D31BDABu) return;
    lbl_805FB3B0 = 0;
    lbl_805FB3B1 = 0;
    lbl_805FB3B2 = 0;
    lbl_805F9080 = 1;
    lbl_805F9081 = 1;
    lbl_805FB3E8 = 0;
    lbl_805FB3E0 = 1;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        u8* el = (u8*)ScreenSetFindElement(set, 0x8FB60C54u);
        lbl_805FB3BC = el;
        if (el != NULL && lbl_805FEFB0 == lbl_805F9084 && lbl_805FEFB0 == lbl_805F9088 && lbl_805FEFB0 == lbl_805F908C) {
            lbl_805F9084 = *(f32*)(el + 0x68);
            lbl_805F9088 = *(f32*)(el + 0x6C);
            lbl_805F908C = *(f32*)(el + 0x70);
        }
        el = (u8*)ScreenSetFindElement(set, 0x6530D136u);
        lbl_805FB3C8 = el;
        if (el != NULL) {
            lbl_805F9090 = *(f32*)(el + 0x68);
            lbl_805F9094 = *(f32*)(el + 0x6C);
            lbl_805F9098 = *(f32*)(el + 0x70);
        }
        if (lbl_805FB3CC != 0) *(s32*)(lbl_803F20C4 + 0x44) = 0x39;
        u8* widget = (u8*)ScreenSetFindWidget(set, 0x9631294Eu);
        if (widget != NULL && UIWidgetFindChild(widget, 0x4DD41D3Du) != NULL) {
            u8* a = (u8*)UIWidgetFindChild(widget, 0x514126CCu);
            u8* b = (u8*)UIWidgetFindChild(widget, 0xC3F30972u);
            if (a != NULL && b != NULL) {
                UISetText(a, (const char*)(lbl_80556E80 + 0xB40), 1);
                UISetText(b, fn_8013E774(), 1);
            }
            char buf[16];
            a = (u8*)UIWidgetFindChild(widget, 0x9C492A14u);
            b = (u8*)UIWidgetFindChild(widget, 0x674FFA87u);
            if (a != NULL && b != NULL) {
                SysSprintf(buf, "%d", *(s32*)(lbl_805FBBE0 + 0xA28));
                UISetText(a, buf, 1);
                SysSprintf(buf, "%d", *(s32*)(lbl_805FBBE0 + 0xA2C));
                UISetText(b, buf, 1);
            }
            a = (u8*)UIWidgetFindChild(widget, 0xC8487776u);
            if (a != NULL) UISetText(a, (const char*)lbl_803F2098, 1);
            a = (u8*)UIWidgetFindChild(widget, 0xEBD7F8E4u);
            b = (u8*)UIWidgetFindChild(widget, 0xDA859649u);
            if (a != NULL && b != NULL) {
                SysSprintf(buf, "%d", lbl_805FB3A8);
                UISetText(a, buf, 1);
                SysSprintf(buf, "%d", lbl_805FB3AC);
                UISetText(b, buf, 1);
            }
        }
    }
    fn_80140B8C(0x10);
    fn_80140B8C(8);
    SetMPSection(0x9D31BDABu);
    SetHitBoxes(1);
}

// 0x80107580
// Script callback PreloadLevel: for the level whose name hash is a0 ("QUIT" = level 0x43), picks
// the zone files ("<level>.sir" / "<level>c.sir") and preloads them for zones 0..3 into the zone
// memory slots while the pool budget lasts.
void PreloadLevel(u32 a0) {
    if (gIdPool[0] == 0) return;
    u8 level = 0;
    s32 found = 0;
    if (a0 == StringHash("QUIT")) {
        level = 0x43;
        found = 1;
    } else if (a0 != 0) {
        u8 i = 0;
        for (s32 n = 0; n < 0x44; n++, i++) {
            if (a0 == *(u32*)(gLevelFlags + ((u32)i << 5) + 4)) {
                level = i;
                found = 1;
                break;
            }
        }
    }
    if (found == 0) return;
    u8* lvl = gLevelFlags + ((u32)level << 5);
    char sir[0x40];
    char sirc[0x40];
    WorldLoadLevelZones(0);
    SysSprintf(sir, "%s.%s", *(const char**)lvl, "sir");
    SysSprintf(sirc, "%sc.%s", *(const char**)lvl, "sir");
    u8* gs = GetGlobalSettings();
    s32 total = *(s32*)(gs + 0x6C) + *(s32*)(gs + 0x68) + *(s32*)(gs + 0x70) + *(s32*)(gs + 0x74) + *(s32*)(gs + 0x78) +
                *(s32*)(gs + 0x7C) + *(s32*)(gs + 0x80) + *(s32*)(gs + 0x84);
    u32 budget = fn_801D3A84(total + 1);
    u32 dst = gZoneBase[total + 1];
    for (s32 pass = 0; pass < 4; pass++) {
        u32 hash = (u32)-1;
        if (pass != 0) hash = (u32)(pass - 1);
        if (hash == 2) {
            if (a0 != kHashEmptyLevel) continue;
            strcpy(sir, lbl_8036A5F0);
            hash = 0x1E0;
        }
        u8* name = (u8*)sirc;
        s32 size = fn_801E6040(gIdPool, name, (s32)hash);
        if (size == 0) {
            name = (u8*)sir;
            size = fn_801E5F48(gIdPool, (const char*)name, hash);
        }
        u32 need = ((u32)size + 0x1F) & 0xFFFFFFE0u;
        if (budget < need) continue;
        if (gWorldState[4] != 0) {
            u8* g = GetGlobalSettings();
            s32 t2 = *(s32*)(g + 0x6C) + *(s32*)(g + 0x68) + *(s32*)(g + 0x70) + *(s32*)(g + 0x74) + *(s32*)(g + 0x78) +
                     *(s32*)(g + 0x7C) + *(s32*)(g + 0x80) + *(s32*)(g + 0x84);
            fn_80133840(gWorldState, t2 + 1);
            fn_80133698(gWorldState, *(s32*)(GetGlobalSettings() + 0x68) + 1);
        }
        fn_801E6374(gIdPool, name, (u8*)sir, dst, need, (s32)hash);
        dst += need;
        budget -= need;
    }
}

// 0x8013FE90
// Script callback InitProgressScreen: plays the deactivate anim on the screen-set element at +0x74,
// pushes the "connecting" screen and, for the known progress ids, sets its text from the string
// table and sends a state message to the widget, then switches hit boxes off and plays the alert sound.
void InitProgressScreen(u32 a0) {
    typedef void (*Fn)(u8*, s32);
    u8* anim = *(u8**)((u8*)gScreenSets + 0x74);
    if (anim != NULL) UIPlayAnim(anim, kHashDeactivate);
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* widget = (u8*)ScreenSetFindWidget(set, kHashConnecting);
    if (widget == NULL) return;
    UIScreen_Push(kHashConnecting);
    u8* text = (u8*)UIWidgetFindChild(widget, kHashAttention);
    u8* ctl = (u8*)UIWidgetFindChild(widget, 0xAF6D8A6Du);
    if (text == NULL || ctl == NULL) return;
    u32 strIdx;
    s32 vslot;
    switch (a0) {
        case 0xE6E5D82Fu: strIdx = 0x579; vslot = 0x2C; break;
        case 0x880B96BDu: strIdx = 0x560; vslot = 0x30; break;
        case 0x97580311u: strIdx = 0x58B; vslot = 0x30; break;
        case 0xD259F014u: strIdx = 0x57A; vslot = 0x2C; break;
        case 0x19C5DBC6u: strIdx = 0x597; vslot = 0x30; break;
        case 0x1DB3F980u: strIdx = 0x68B; vslot = 0x30; break;
        default: return;
    }
    {
        u8* t = (u8*)gStringTable;
        const char** arr = *(const char***)t;
        const char* str = lbl_805F8200;
        if (arr != NULL && *(u32*)(t + 0xC) > strIdx) str = arr[strIdx];
        UISetText(text, str, 1);
    }
    (*(Fn*)(*(u8**)(ctl + 0x138) + vslot))(ctl, 2);
    SetHitBoxes(0);
    if (lbl_805FBBAC == NULL) {
        void* p = fn_801D40E0(1);
        if (p != NULL) p = fn_80209814(p);
        lbl_805FBBAC = p;
    }
    PlaySound(kHashAlertSound, 0, 0, 0, 0, 0);
}

}  // extern "C"
