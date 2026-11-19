// Functions from the miss41 draft batch, rewritten and checked against the asm.
#include "types.h"
#include <math.h>
#include <string.h>

struct Locator;

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {

// Callees and globals used by this file.
s32       GetRegion(void);
void      LabShowBattleResult(void* lab, const void* result);
void      SaveRewardChallenge(u8* s, u32 challenge);
Locator*  WorldFindLocator(void* world, u32 hash);
s32       WorldGetPhase(void* world);
BOOL      ZoneIsLoaded(void* world, s32 zone);
u32       StringHash(const char* s);
int       SysSprintf(char* buf, const char* fmt, ...);
void*     fn_80109330(const char* name);
void      fn_80219004(void* p, s32 v);
void      fn_8020B024(f32 v);
void      fn_8020B078(f32 v);
void      fn_8020B160(f32 v);
void*     fn_801D40E0(s32 n);
void*     fn_80209814(void* v);
void*     fn_801E9D18(void* p, u32 hash, s32 size);
BOOL      fn_801E7460(u8* base, u16* id);
s32       fn_801E9678(void* list, void* handle);
void      fn_801EAE20(u8* p, f32 v);
s32       fn_800D59D4(u8* a3);
s32       fn_80022608(void);
void      fn_800226F0(void);
void      fn_800603AC(u32 a0, s32* goal);
void      fn_800602E8(u32 a0, s32* goal);
void      fn_8012649C(f32 v);
const char* fn_800FA608(u32 a0, s32 lang, s32 flag);
void      fn_8016E2CC(void);
void      fn_8016A254(void);
void      fn_8017630C(void);
void      fn_80183F60(void);
void      fn_801736E8(void);
void      fn_801704F4(void);
void      fn_80184264(void);
void      fn_80173878(void);
void      fn_801731C4(void);
void      fn_80174A20(void);
void      fn_8016B894(void);

extern u8    gGame[];
extern void* gLab;
extern u8    gWorldState[];
extern "C++" s32   gTextLanguage;
extern u8    lbl_803C4298[];
extern u8    lbl_805FAF9B;
extern s32   lbl_805FB028;
extern u8*   lbl_805FB134;
extern void* lbl_805FB208[];
extern u8*   lbl_805FB8D8;
extern void* lbl_805FBBAC;
extern u8*   lbl_805FBBE0;
extern f32   lbl_805FD300;   // 0.0f
extern char  lbl_803F2210[];  // monster name text buffer

static const u32 kHashNormal = 0xD17DAF62u;               // "normal"
static const u32 kHashFollowWaypointPath = 0x80B4120Fu;   // "FollowWaypointPath"

// 0x80022610
// Records the final challenge score: shows the battle result, then, if the challenge flag is set
// in the world and the save has a reward slot, stores the challenge id and runs the reward hook.
void SetFinalChallengeScore(void) {
    u8 result = lbl_805FAF9B;
    if (lbl_805FB134 != NULL) {
        fn_80022608();   // score value is not used by the caller
    }
    LabShowBattleResult(gLab, &result);
    if (result != 0 && *(s32*)(*(u8**)gWorldState + 0x1E0) == 2) {
        s32 flag = *(s32*)(*(u8**)(gGame + 0xD614) + 0xA00);
        if (flag != 0) {
            s32 mode = *(s32*)((u8*)gLab + 0x4C);
            if (mode != 1 && mode != 5) {
                // fn_800D59D4 receives the leftover r3 (the flag value loaded above).
                fn_80219004(gLab, fn_800D59D4((u8*)(intptr_t)flag));
            }
            SaveRewardChallenge(lbl_805FBBE0, (u32)*(s32*)((u8*)gLab + 0x4C));
            fn_800226F0();
        }
    }
}

// 0x800F6068
// Points the camera at the locator pair (from hash a0, to hash a1) and sets its zoom a2.
void SetFixedCamera(u32 a0, u32 a1, f32 a2) {
    u8* from = (u8*)WorldFindLocator(gWorldState, a0);
    u8* to = (u8*)WorldFindLocator(gWorldState, a1);
    if (from != NULL && to != NULL) {
        u8* cam = *(u8**)(lbl_803C4298 + 4);
        (*(void (**)(u8*, s32))(*(u8**)cam + 0xC))(cam, 6);
        (*(void (**)(u8*, u8*, u8*))(*(u8**)cam + 0x2C))(cam, from + 8, to + 8);
        (*(void (**)(u8*, f32))(*(u8**)cam + 0x30))(cam, a2);
    }
}

// 0x8005C664
// Scales the 3D vector at +0x850 (x, y, z) to unit length and multiplies it by a1.
// The asm uses frsqrte with two Newton steps; this uses 1/sqrtf, equal to float precision.
void SetKnockbackSpeed(void* a0, f32 a1) {
    u8* p = (u8*)a0;
    f32 x = *(f32*)(p + 0x850);
    f32 y = *(f32*)(p + 0x854);
    f32 z = *(f32*)(p + 0x858);
    f32 lenSq = (z * z) + ((x * x) + (y * y));
    f32 nx = 0.0f;
    f32 ny = 0.0f;
    f32 nz = 0.0f;
    if (lenSq != lbl_805FD300) {
        f32 inv = 1.0f / sqrtf(lenSq);
        nx = x * inv;
        ny = y * inv;
        nz = z * inv;
    }
    *(f32*)(p + 0x850) = nx * a1;
    *(f32*)(p + 0x854) = ny * a1;
    *(f32*)(p + 0x858) = nz * a1;
}

// 0x8003F75C
// Maps a combat type hash to its index in lbl_805FB028 (normal, 1, 2, 3). Unknown hashes do nothing.
void SetNextCombatType(u32 a0) {
    switch (a0) {
    case kHashNormal:
        lbl_805FB028 = 0;
        break;
    case 0x463377A0u:
        lbl_805FB028 = 2;
        break;
    case 0x7EDD0A3Du:
        lbl_805FB028 = 1;
        break;
    case 0xAFB915F6u:
        lbl_805FB028 = 3;
        break;
    }
}

// 0x800F63F4
// Restores the normal camera when the world is in phase 2 (copies the saved value and blends
// over 1.5 s); phases 1 and 7 call the short camera reset instead.
void SetNormalCamera(void) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        *(s32*)(*(u8**)(lbl_803C4298 + 8) + 0x2C0) = *(s32*)(gGame + 0xD614);
        u8* cam = *(u8**)(lbl_803C4298 + 4);
        (*(void (**)(u8*))(*(u8**)cam + 0x10))(cam);
        (*(void (**)(u8*, f32))(*(u8**)cam + 0x30))(cam, 1.5f);   // 1.5f (lbl_805FEB74, value assumed)
        return;
    }
    if (WorldGetPhase(gGame + 0xD5F0) == 1 || WorldGetPhase(gGame + 0xD5F0) == 7) {
        fn_8012649C(1.5f);
    }
}

// Volume level 0..10 (values >= 10 clamp to 10) as a 0..1 scale.
static f32 VolumeLevelScale(BOOL level) {
    s32 v = 10;
    if ((u32)level < 10u) {
        v = level;
    }
    return (f32)v / 10.0f;   // divisor lbl_805FEB6C, assumed 10.0f
}

// Creates the voice SFX handle on first use (lbl_805FBBAC).
static void EnsureVolumeHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) {
            h = fn_80209814(h);
        }
        lbl_805FBBAC = h;
    }
}

// 0x800FAA9C
// Sets the music volume from level a0 (0..10) and stores the level in the save block.
void SetOptionsMusicVolume(BOOL a0) {
    EnsureVolumeHandle();
    fn_8020B024(VolumeLevelScale(a0));
    *(s16*)(lbl_805FBBE0 + 0x916) = (s16)a0;
}

// 0x800FAA14
// Sets the music volume from level a0 (0..10) without saving it.
void SetOptionsMusicVolumeTmp(BOOL a0) {
    EnsureVolumeHandle();
    fn_8020B024(VolumeLevelScale(a0));
}

// 0x800FAC50
// Sets the sound effects volume from level a0 (0..10) and stores the level in the save block.
void SetOptionsSfxVolume(BOOL a0) {
    fn_8020B078(VolumeLevelScale(a0));
    *(s16*)(lbl_805FBBE0 + 0x914) = (s16)a0;
}

// 0x800FAC00
// Sets the sound effects volume from level a0 (0..10) without saving it.
void SetOptionsSfxVolumeTmp(BOOL a0) {
    fn_8020B078(VolumeLevelScale(a0));
}

// 0x800FADCC
// Sets the voice volume from level a0 (0..10) and stores the level in the save block.
void SetOptionsVoiceVolume(BOOL a0) {
    EnsureVolumeHandle();
    fn_8020B160(VolumeLevelScale(a0));
    *(s16*)(lbl_805FBBE0 + 0x918) = (s16)a0;
}

// 0x800FAD44
// Sets the voice volume from level a0 (0..10) without saving it.
void SetOptionsVoiceVolumeTmp(BOOL a0) {
    EnsureVolumeHandle();
    fn_8020B160(VolumeLevelScale(a0));
}

// 0x80103920
// Reads the s32 goal at a1 and passes it to the waypoint goal setter.
void SetPersistentWaypointGoal(u32 a0, s32* a1, f32 a2) {
    s32 goal = *a1;
    fn_800603AC(a0, &goal);
}

// 0x801B6DB0
// Sets (a1 != 0) or clears bit 0x2 in the render flags of object a0 and its sub-record at +4.
void SetRenderFlag(void* a0, u32 a1) {
    u8* obj = (u8*)a0;
    if (a1 != 0) {
        if ((*(void* (**)(u8*))(*(u8**)obj + 0xC))(obj) != NULL) {
            u8* t = (u8*)(*(void* (**)(u8*))(*(u8**)obj + 0xC))(obj);
            *(s32*)(t + 0x8) |= 2;
        }
        u8* sub = *(u8**)(obj + 0x4);
        *(s32*)(sub + 0x20) |= 2;
        return;
    }
    if ((*(void* (**)(u8*))(*(u8**)obj + 0xC))(obj) != NULL) {
        u8* t = (u8*)(*(void* (**)(u8*))(*(u8**)obj + 0xC))(obj);
        *(s32*)(t + 0x8) &= ~2;
    }
    u8* sub = *(u8**)(obj + 0x4);
    *(s32*)(sub + 0x20) &= ~2;
}

// 0x80103978
// Reads the s32 goal at a1 and passes it to the waypoint goal setter (variant).
void SetWaypointGoal(u32 a0, s32* a1, f32 a2) {
    s32 goal = *a1;
    fn_800602E8(a0, &goal);
}

// Table entry for the waypoint record at node+0x7C (NULL when the id is not in the table).
static u8* WaypointTableEntry(u8* node) {
    u16 id = *(u16*)(node + 0x7C);
    u16 key = id;
    u8* table = lbl_805FB8D8;
    if (!fn_801E7460(table, &key)) {
        return NULL;
    }
    return table + (((s32)id << 7) & 0x7F80);
}

// 0x801B8A68
// Sets the speed a1 on the waypoint path found under object a0 (two-level lookup).
void SetWaypointPathSpeed(void* a0, f32 a1) {
    u8* node = (u8*)fn_801E9D18((u8*)a0 + 0xFC, kHashFollowWaypointPath, 0x70);
    if (node == NULL) {
        return;
    }
    u8* entry = WaypointTableEntry(node);
    node = (u8*)fn_801E9D18(entry, 0x9028DA4Eu, 0x78);
    if (node != NULL) {
        u8* entry2 = WaypointTableEntry(node);
        fn_801EAE20(entry2, a1);
    }
}

// 0x800FA860
// Builds the splash screen texture path for index a0 (region-dependent) and stores the texture
// in slot a1 (only slots 0..2).
void SetupSplashScreen(u32 a0, BOOL a1) {
    char buf[0x10C];   // stack space r1+0x8..r1+0x113
    if ((u32)a1 <= 2u) {
        s32 isDefaultRegion = (GetRegion() == 0);
        SysSprintf(buf, "%s", "ui/msa_legal/");
        strcat(buf, fn_800FA608(a0, gTextLanguage, isDefaultRegion));
        lbl_805FB208[a1] = fn_80109330(buf);
    }
}

// 0x80169604
// Shuts down the game subsystems in order.
void SgShutdown(void) {
    fn_8016E2CC();
    fn_80183F60();
    fn_801736E8();
    fn_8017630C();
    fn_8016A254();
    fn_801704F4();
    fn_80184264();
    fn_80173878();
    fn_801731C4();
    fn_80174A20();
    fn_8016B894();
}

// 0x801F248C
// Removes the bouncing overlay entry for the name at a1+0x80 in the zone given by byte +1 of a0.
void ShaderStopBouncingOverlayAlpha(void* a0, void* a1) {
    if (a1 == NULL) {
        return;
    }
    u8 zone = *((u8*)a0 + 0x1);
    if (ZoneIsLoaded(gWorldState, zone) == 0) {
        return;
    }
    u8* list = *(u8**)(*(u8**)gWorldState + 0x1C8) + zone * 0xC;
    u8* node = (u8*)fn_801E9D18(list, StringHash((const char*)((u8*)a1 + 0x80)), 0xF8);
    if (node != NULL) {
        u16 id = *(u16*)(node + 0x7C);
        fn_801E9678(list, &id);
    }
}

// 0x8013E764
// Copies the monster name a0 into the monster name buffer and returns the buffer.
char* SetMyMonsterName(const char* a0) {
    return strcpy(lbl_803F2210, a0);
}

}  // extern "C"
