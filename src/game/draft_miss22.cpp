#include "types.h"
#include "game/part.h"
#include "game/script_callbacks.h"

struct StringTable;
extern void* gLab;                  // 0x805FBBC8 (C++ linkage)
extern void* gScreenSets;           // 0x805FB654 (C++ linkage)
extern StringTable* gStringTable;   // 0x805FB910 (C++ linkage)
extern StringTable* gStringTable2;  // 0x805FB90C (C++ linkage)

extern "C" {

u32  RandomU32(void);
s32  RaySceneTest(void* ray);
void ScreenSetsFlush(void* sets);
u8*  fn_8006FBDC(void);
void fn_800AA5A0(void* p, s32 flag);            // 0x800AA5A0
void fn_800AA684(u8* a3);                       // 0x800AA684
void fn_800ABA44(void* p);                      // 0x800ABA44
BOOL fn_802188CC(u8* lab);                      // 0x802188CC
void fn_8033AE04(u32* out);                     // 0x8033AE04
u32  fn_801EC880(u8* p, u32 hash);
void fn_801EC8C4(u8* p, u32 a, u32 b);
void fn_801ECDE8(u8* p, s32 idx, u32 a, s32 immediate);

extern u8*  lbl_805FAF78;                       // 0x805FAF78 (current monster record)
extern u8*  lbl_805FBBE0;                       // 0x805FBBE0 (save block pointer)
extern u8   lbl_805FB0E0;                       // 0x805FB0E0 (byte flag)
extern u8*  lbl_805FB170;                       // 0x805FB170 (tentacle in berserk state)
extern f32  lbl_805FB174;                       // 0x805FB174 (float, tentacle berserk value)
extern f32  lbl_805FDF98;                       // 0x805FDF98
extern f32  lbl_805FDF9C;                       // 0x805FDF9C
extern f32  lbl_805FDFBC;                       // 0x805FDFBC
extern f32  lbl_805FDFC0;                       // 0x805FDFC0
extern f32  lbl_805FDFC4;                       // 0x805FDFC4
extern f32  lbl_805FE038;                       // 0x805FE038
extern f32  lbl_805FE03C;                       // 0x805FE03C
extern f32  lbl_80346C60[];                     // 0x80346C60 (float table, RaySceneTest-indexed)
extern f32  lbl_80346C70[];                     // 0x80346C70 (float table)
extern s32  lbl_80346C90[];                     // 0x80346C90 (s32 table, RaySceneTest-indexed)
extern s32  lbl_80346CA0[];                     // 0x80346CA0 (s32 table, RaySceneTest-indexed)
extern const char lbl_805F8200[];               // 0x805F8200 (empty string)
extern const char lbl_80354800[];               // 0x80354800 (string)
extern const char lbl_8035480C[];               // 0x8035480C (string)

// 0x800FA564
// Table row (+0x8) of the chosen experiment ingredient; 0 when no ingredient is chosen.
s32 GetExperimentScienceType(void) {
    if (fn_802188CC((u8*)gLab) != 0) {
        s32 idx = *(s32*)((u8*)gLab + 0x48);
        u8* tbl = *(u8**)(gGameDb->data + 0x3E60);
        return *(s32*)(tbl + idx * 0xC + 0x8);
    }
    return 0;
}

// 0x801DC3A8
// Identity on a hash value.
u32 GetHash(u32 a0) {
    return a0;
}

// 0x8010366C
// Health of an entity: float at +0x24 of the sub-object at +0x4, truncated to an integer.
s32 GetHealth(void* a0) {
    return (s32)*(f32*)(*(u8**)((u8*)a0 + 0x4) + 0x24);
}

// 0x8006F514
// Number of the five intersector slots (+0x1D4 .. +0x1F4, step 8) that are set; 0 for a null object.
s32 GetIntersectorCount(void* a0) {
    u8* p = (u8*)a0;
    s32 n = 0;
    if (p == NULL) {
        return 0;
    }
    if (*(s32*)(p + 0x1D4) != 0) n = 1;
    if (*(s32*)(p + 0x1DC) != 0) n += 1;
    if (*(s32*)(p + 0x1E4) != 0) n += 1;
    if (*(s32*)(p + 0x1EC) != 0) n += 1;
    if (*(s32*)(p + 0x1F4) != 0) n += 1;
    return n;
}

// 0x8001E704
// Name for the script: the +0xA90 value when +0xBF8 is 0 and +0xAD8 is -1, otherwise a pointer to
// the name text at +0xAE8; 0 when there is no current monster.
ScriptValue GetMonsterName(void) {
    u8* p = lbl_805FAF78;
    if (p == NULL) {
        return 0;
    }
    if (*(s32*)(p + 0xBF8) == 0 && *(s32*)(p + 0xAD8) == -1) {
        return (ScriptValue)*(s32*)(p + 0xA90);
    }
    return (ScriptValue)(p + 0xAE8);
}

// 0x8001DFC8
// 1 when the count at +0x10E8 of the save block is positive, else 0.
s32 GetMonstersAvailableToModify(void) {
    return *(s32*)(lbl_805FBBE0 + 0x10E8) > 0;
}

// 0x801427F0
// Current random seed, as written back by fn_8033AE04.
u32 GetRandomSeed(void) {
    u32 seed;
    fn_8033AE04(&seed);
    return seed;
}

// 0x800E76CC
// Identity on a pointer.
void* GetTHIS(void* a0) {
    return a0;
}

// 0x801401FC
// Text for an id: 0xFFFFFFFF gives lbl_80354800; ids with bit 31 index gStringTable2 (id & 0x7FFFFFFF),
// others index gStringTable; anything else gives the empty string.
const char* GetText(s32 id) {
    u32 uid = (u32)id;
    if (uid + 0x10000u == 0xFFFFu) {
        return lbl_80354800;
    }
    if ((uid & 0x80000000u) && gStringTable2 != NULL) {
        u8* tbl2 = (u8*)gStringTable2;
        const char** strings2 = *(const char***)tbl2;
        u32 idx = uid & 0x7FFFFFFFu;
        if (strings2 != NULL && idx < *(u32*)(tbl2 + 0xC)) {
            return strings2[idx];
        }
        return lbl_805F8200;
    }
    {
        u8* tbl = (u8*)gStringTable;
        const char** strings = *(const char***)tbl;
        if (strings != NULL && uid < *(u32*)(tbl + 0xC)) {
            return strings[uid];
        }
    }
    if (uid + 0x10000u == 0xFFFFu) {
        return lbl_8035480C;
    }
    return lbl_805F8200;
}

// 0x800ABD34
// True when the squared length of the position vector (record at +0x4, floats x/y/z at +0/+4/+8)
// is at least lbl_805FDFBC.
s32 GrowJuiceProjectile_PassedFarBoundary(void* a0) {
    if (a0 != NULL) {
        u8* rec = *(u8**)((u8*)a0 + 0x4);
        f32 x = *(f32*)(rec + 0x0);
        f32 y = *(f32*)(rec + 0x4);
        f32 z = *(f32*)(rec + 0x8);
        if ((z * z) + ((x * x) + (y * y)) >= lbl_805FDFBC) {
            return 1;
        }
    }
    return 0;
}

// 0x800ABC80
// Hides the projectile: runs fn_800ABA44, puts its position at (-A, -A, -B) with A = lbl_805FDF98,
// B = lbl_805FDF9C, and sets bits 0x8000, 0x10000000, 0x4000, 0x10000000 in the record's flags (+0x20).
void GrowJuiceProjectile_PositionToHide(void* a0) {
    if (a0 != NULL) {
        fn_800ABA44(a0);
        u8* rec = *(u8**)((u8*)a0 + 0x4);
        *(f32*)(rec + 0x0) = -lbl_805FDF98;
        *(f32*)(rec + 0x4) = -lbl_805FDF98;
        *(f32*)(rec + 0x8) = -lbl_805FDF9C;
        *(u32*)(rec + 0x20) |= 0x8000u;
        *(u32*)(rec + 0x20) |= 0x10000000u;
        *(u32*)(rec + 0x20) |= 0x4000u;
        *(u32*)(rec + 0x20) |= 0x10000000u;
    }
}

// 0x800AC728
// Hides the seed: clears the +0x264 counter, sets +0x268 to -1, puts the position at (-C, -C, -D)
// with C = lbl_805FDFC0, D = lbl_805FDFC4, sets the same record flags as the projectile, and sets
// the four floats at +0x38..+0x44 of the sub-object at +0x90 to D.
void GrowJuiceSeed_PositionToHide(void* a0) {
    if (a0 != NULL) {
        u8* self = (u8*)a0;
        *(s32*)(self + 0x264) = 0;
        *(s32*)(self + 0x268) = -1;
        u8* rec = *(u8**)(self + 0x4);
        *(f32*)(rec + 0x0) = -lbl_805FDFC0;
        *(f32*)(rec + 0x4) = -lbl_805FDFC0;
        *(f32*)(rec + 0x8) = -lbl_805FDFC4;
        *(u32*)(rec + 0x20) |= 0x8000u;
        *(u32*)(rec + 0x20) |= 0x10000000u;
        *(u32*)(rec + 0x20) |= 0x4000u;
        *(u32*)(rec + 0x20) |= 0x10000000u;
        u8* sub = *(u8**)(self + 0x90);
        *(f32*)(sub + 0x38) = lbl_805FDFC4;
        *(f32*)(sub + 0x3C) = lbl_805FDFC4;
        *(f32*)(sub + 0x40) = lbl_805FDFC4;
        *(f32*)(sub + 0x44) = lbl_805FDFC4;
    }
}

// 0x800AEA6C
// Sends the tentacle berserk: stores it in lbl_805FB170, sets the four floats at +0x38..+0x44 of
// its sub-object (+0x90) to (P, Q, Q, P), and does the same for its child (+0x26C), which also gets
// the 0xE2E205E8 / 0xBA5145F9 update.
void GrowJuiceTentacle_GoBerserk(void* a0) {
    lbl_805FB170 = (u8*)a0;
    u8* sub = *(u8**)((u8*)a0 + 0x90);
    *(f32*)(sub + 0x38) = lbl_805FE038;
    *(f32*)(sub + 0x3C) = lbl_805FE03C;
    *(f32*)(sub + 0x40) = lbl_805FE03C;
    *(f32*)(sub + 0x44) = lbl_805FE038;
    u8* child = *(u8**)(lbl_805FB170 + 0x26C);
    if (child != NULL) {
        u8* csub = *(u8**)(child + 0x90);
        *(f32*)(csub + 0x38) = lbl_805FE038;
        *(f32*)(csub + 0x3C) = lbl_805FE03C;
        *(f32*)(csub + 0x40) = lbl_805FE03C;
        *(f32*)(csub + 0x44) = lbl_805FE038;
        fn_801EC8C4(child + 0x94, fn_801EC880(child + 0x94, 0xE2E205E8u), 0xBA5145F9u);
    }
}

// 0x800AEA38
// Heart hit: when the object exists and fn_8006FBDC returns a record, calls fn_800AA684 on it.
void GrowJuiceTentacle_HitHeart(void* a0) {
    if (a0 != NULL) {
        u8* p = fn_8006FBDC();
        if (p != NULL) {
            fn_800AA684(p);
        }
    }
}

// 0x800AEA00
// Player hit: when the object exists and fn_8006FBDC returns a record, calls fn_800AA5A0(record, 1).
void GrowJuiceTentacle_HitPlayer(void* a0) {
    if (a0 != NULL) {
        u8* p = fn_8006FBDC();
        if (p != NULL) {
            fn_800AA5A0(p, 1);
        }
    }
}

// 0x800AE680
// Random attack roll: weights w0 = lbl_80346C90[ray] and w1 = lbl_80346CA0[ray]; attack with
// chance w1 / (w0 + w1). Returns 1 to attack.
s32 GrowJuiceTentacle_ShouldAttackPlayer(void* a0) {
    if (a0 == NULL) {
        return 0;
    }
    s32 idx = RaySceneTest(a0);
    s32 w0 = lbl_80346C90[idx];
    u32 w1 = (u32)lbl_80346CA0[RaySceneTest((void*)(intptr_t)idx)];
    u32 total = (u32)w0 + w1;
    if (total != 0 && RandomU32() % total < w1) {
        return 1;
    }
    return 0;
}

// 0x800AE960
// Berserk check: no while another tentacle is already berserk; the tentacle's float at +0x360 must
// reach lbl_80346C60[ray]; then the global lbl_805FB174 must be below lbl_805FE03C. Returns 1 to go berserk.
s32 GrowJuiceTentacle_ShouldGoBerserk(void* a0) {
    if (a0 == NULL) {
        return 0;
    }
    if (lbl_805FB170 != NULL) {
        return 0;
    }
    s32 idx = RaySceneTest(a0);
    if (*(f32*)((u8*)a0 + 0x360) < lbl_80346C60[idx]) {
        return 0;
    }
    return !(lbl_805FB174 >= lbl_805FE03C);
}

// 0x800EFB88
// Hides the instruction screen: flushes the screen sets unless lbl_805FB0E0 is set, hides three
// UI elements, pops the screen, and unpauses the exploration characters when a0 is nonzero.
void HideInstructionScreen(u32 a0) {
    if (lbl_805FB0E0 == 0) {
        ScreenSetsFlush(gScreenSets);
    }
    UIElement_SetVisibleFlag(0xBA036DE3u, 0u);
    UIElement_SetVisibleFlag(0x0C9524ACu, 0u);
    UIElement_SetVisibleFlag(0xFD805850u, 0u);
    UIScreen_PopScreen(0x0C4FC83Bu);
    if (a0 != 0) {
        UnPauseAllExplorationCharacters();
    }
}

}  // extern "C"
