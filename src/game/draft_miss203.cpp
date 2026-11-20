// Mixed small/medium functions: battle scene setup (0x8004xxxx), camera pickup/blend helpers
// (0x8008xxxx), robot evolution lab handlers (0x8008F1F8 / 0x8009133C), shell record setup
// (0x80097xxx) and astral-zone object setup (0x80099D08).
#include <math.h>

#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef u8*  (*VGetPtrFn)(void* obj);                 // vtable +0x10
typedef void (*VCallIntFn)(void* obj, s32 a);         // vtable +0x14 / +0x18
typedef s32  (*VGetIntFn)(void* obj);                 // vtable +0x34 / +0xBC
typedef f32  (*VGetScoreFn)(void* obj);               // vtable +0x64

extern "C" {
void  BattleSetFighter(u8* b, s32 side, u8* f, u32 owner);                      // 0x80212E84
void* CharacterFind(s32 id);                                                    // 0x800E876C
void  EulerToQuat(Quat* out, const Vec3* euler);                                // 0x80145B00
void  FighterStatsSetOwner(u8* stats, u32 v);                                   // 0x80213F84
void* MemAllocSmall(u32 size);                                                  // 0x801D41CC
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);                  // 0x80209E70
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);               // 0x80145EE8
void  QuatFromMtx(Quat* out, const f32 m[12]);                                  // 0x801456E4
u32   RandomU32(void);                                                          // 0x80143460
s32   RaySceneTest(void* ray);                                                  // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                                            // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                                 // 0x801F53B0
u32   StringHash(const char* s);                                                // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);
void  TextureRelease(void* tex);                                                // 0x80184028
void  app_PlayMusic(u32 hash, f32 vol);
s32   fn_8003F8D4(void);                                                        // 0x8003F8D4
void fn_80040F44(void);                                                       // 0x80040F44
void  fn_80041874(u8* p);                                                       // 0x80041874
u8*   fn_800418E0(u8* p, s32 a2);                                               // 0x800418E0
void  fn_80041AB4(u8* p);                                                       // 0x80041AB4
u8* fn_80042C38(u8* p);                                                     // 0x80042C38
void fn_80042E04(u8* p);                                                        // 0x80042E04
void* fn_80043910(void* a0, s32 idx, s32 kind);                                 // 0x80043910
void  fn_800445E4(u8* p);                                                       // 0x800445E4
u8*   fn_800445F4(u8* p, s32 del);                                              // 0x800445F4
void  fn_80044C58(u8* p);                                                       // 0x80044C58
u8*   fn_80044C7C(u8* p, s32 del);                                              // 0x80044C7C
void* fn_80044F54(void);                                                        // 0x80044F54
u8*   fn_800497F4(u8* p);                                                       // 0x800497F4
void  fn_8004AC68(u8* p);                                                       // 0x8004AC68
void  fn_80070FAC(u8* p, f32 v);                                                // 0x80070FAC
void  fn_800712B4(void);                                                        // 0x800712B4
void  fn_800712B8(void);                                                        // 0x800712B8
void  fn_80091950(u8* p);                                                       // 0x80091950
void  fn_80091E28(u8* slot, s32 lane, u8* obj);                                 // 0x80091E28
void  fn_80091EE8(u8* p);                                                       // 0x80091EE8
void  fn_80091F18(u8* p, f32 v);                                                // 0x80091F18
void  fn_8009231C(u8* slot, s32 lane, u8* obj, u32 hash, s32 a, s32 b, s32 c);  // 0x8009231C
void  fn_800925A4(u8* p);                                                       // 0x800925A4
void  fn_800925B0(u8* p, f32 v);                                                // 0x800925B0
void  fn_80092A30(u8* obj, s32 playEffects);                                    // 0x80092A30
void  fn_80092D10(void);                                                        // 0x80092D10
void  fn_800939EC(u8* slot, s32 lane, u8* obj, s32 a, s32 b);                   // 0x800939EC
void  fn_80093C20(u8* p, s32 on);                                               // 0x80093C20
void  fn_80097BFC(u8* p);                                                       // 0x80097BFC
void  fn_800980F8(u8* p, s32 b);                                                // 0x800980F8
void fn_8009C414(u8* p, u8* arg1);                                    // 0x8009C414
void  fn_801455E0(f32* out, u8* quat);                                          // 0x801455E0
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);                             // 0x80145F9C
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);                     // 0x80146180
s32   fn_8018BDE0(u8* p, s32 key);                                              // 0x8018BDE0
void  fn_801B4160(u8* p);                                                       // 0x801B4160
void* fn_801D40E0(s32 n);                                                       // 0x801D40E0
u32   fn_801EC880(u8* p, u32 hash);                                             // 0x801EC880
void* fn_80209814(void* v);                                                     // 0x80209814
s32   fn_80213130(u8* battle, u8* x);                                           // 0x80213130
s32   fn_80213150(u8* battle, u8* x);                                           // 0x80213150
void  fn_80213810(u8* battle, u8* x);                                           // 0x80213810
BOOL  fn_80217094(u8* lab, s32 level);                                          // 0x80217094
BOOL  fn_80217134(void* unused, s32 id, f32* a, f32* b, f32* c, f32* d);        // 0x80217134
void  fn_8032F1E8(void* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);  // 0x8032F1E8

extern u8  gWorldState[];          // 0x803E6D00
extern f32 lbl_80346FC0[];         // 0x80346FC0 (float table)
extern s32 lbl_80347170[];         // 0x80347170
extern f32 lbl_80347190[];         // 0x80347190
extern f32 lbl_80347200[4];        // 0x80347200
extern f32 lbl_80347210[4];        // 0x80347210
extern s32 lbl_803471C0[];         // 0x803471C0 (score table indexed by RaySceneTest)
extern u8  lbl_803C4298[];         // 0x803C4298
extern s32 lbl_803C51D8[];         // 0x803C51D8
extern u8  lbl_8047C7D8[];         // 0x8047C7D8
extern s32 lbl_805F8538;           // 0x805F8538
extern s32 lbl_805FB028;           // 0x805FB028
extern void* lbl_805FB058;         // 0x805FB058
extern u8  lbl_805FB070[];         // 0x805FB070
extern u8* lbl_805FB964;           // 0x805FB964
extern s32 lbl_805FB970;           // 0x805FB970
extern s32 lbl_805FB974;           // 0x805FB974
}
extern "C++" u8*   gBattle;                // 0x805FBBB8
extern "C++" s32   gZoneResourceSize;      // 0x805FB94C
extern "C++" void* lbl_805FBBAC;           // 0x805FBBAC (voice SFX handle)

static const u32 kHashLightBeam     = 0x041ED221u;  // "light_beamA"
static const u32 kHashRobotCurtains = 0xEFA3FBB0u;  // "robot_evolution_curtinsA"
static const u32 kHashRobotGears    = 0xCFFA4661u;  // "robot_evolution_gearsA"
static const u32 kHashRobotKid      = 0x0C898A89u;  // "robotkidA"
static const u32 kHashEvolutionDoor = 0xB4CA5278u;  // "robot_evolution_door1A"
static const u32 kHashSmasher       = 0x5DB90DD2u;  // "robot_evolution_smasherA"
static const u32 kHashAstralAvatar  = 0x64073331u;  // "astral_avatar"
static const u32 kHashAstralPickup  = 0xEAE9C331u;  // "astral_pickupA"
static const u32 kHashAstralCurved  = 0x05B0C2D7u;  // "astral_curvedA"

// Hamilton product a*b in the engine's operation order.
static inline Quat QuatMulRaw(const Quat& a, const Quat& b) {
    Quat r;
    r.x = ((a.y * b.z) + ((a.w * b.x) + (a.x * b.w))) - (a.z * b.y);
    r.y = (a.z * b.x) + ((a.y * b.w) + ((a.w * b.y) - (a.x * b.z)));
    r.z = (a.z * b.w) + (((a.w * b.z) + (a.x * b.y)) - (a.y * b.x));
    r.w = (((a.w * b.w) - (a.x * b.x)) - (a.y * b.y)) - (a.z * b.z);
    return r;
}

// Flags the scene node (obj+4) as moved/rotated so it gets re-uploaded (four flag ors).
static inline void NodeMarkDirty(u8* obj) {
    *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x10000000;
}

static inline u8* ZoneRecord(void) {
    return *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
}

static inline s32 ZoneEntryCount(void) {
    u8* res = ZoneRecord();
    return *(s32*)(res + 0x94) + *(s32*)(res + 0x98);
}

// Object and flag record of zone entry i (first the static list, then the dynamic one).
static inline void ZoneEntry(s32 i, u8** obj, u8** rec) {
    u8* res = ZoneRecord();
    s32 first = *(s32*)(res + 0x94);
    if (i < first) {
        *obj = ((u8**)*(u8**)(res + 0x120))[i];
        *rec = *(u8**)(res + 0x8C) + i * 0x40;
    } else {
        s32 j = i - first;
        *obj = ((u8**)*(u8**)(res + 0x12C))[j];
        *rec = *(u8**)(res + 0x90) + j * 0x40;
    }
}

// Script object type hash (info at +0x9C, hash at +8), 0 if there is no info block.
static inline u32 ObjTypeHash(u8* obj) {
    u8* info = *(u8**)(obj + 0x9C);
    return info != NULL ? *(u32*)(info + 0x8) : 0;
}

extern "C" {

// 0x80040824
// Battle scene constructor: two element arrays, then the camera/target slot tables cleared.
u8* fn_80040824(u8* arg0) {
    fn_8032F1E8(arg0, (void (*)(void))fn_80044C58, (void (*)(void))fn_80044C7C, 0x148, 2);
    fn_8032F1E8(arg0 + 0x290, (void (*)(void))fn_800445E4, (void (*)(void))fn_800445F4, 0xBC, 2);
    *(s32*)(arg0 + 0x4EC) = 2;
    *(s32*)(arg0 + 0x408) = 0;
    *(s32*)(arg0 + 0x4D4) = 0;
    *(s32*)(arg0 + 0x4D8) = 4;
    *(s32*)(arg0 + 0x4DC) = 0;
    *(u8*)(arg0 + 0x4E0) = 0;
    *(f32*)(arg0 + 0x4E4) = 0.0f;
    *(u8*)(arg0 + 0x4E8) = 0;
    *(s32*)(arg0 + 0x4F0) = 0;
    *(s32*)(arg0 + 0x4F4) = -1;
    // Ten slots: a position (3 floats), a count and a timer each.
    for (s32 k = 0; k < 10; k++) {
        *(f32*)(arg0 + 0x40C + k * 12) = 0.0f;
        *(f32*)(arg0 + 0x410 + k * 12) = 0.0f;
        *(f32*)(arg0 + 0x414 + k * 12) = 0.0f;
        *(s32*)(arg0 + 0x484 + k * 4) = 0;
        *(f32*)(arg0 + 0x4AC + k * 4) = 0.0f;
    }
    return arg0;
}

// 0x80040ACC
// Starts the battle scene: registers both fighters, builds the battle helper objects, binds the
// stats owners and HUD records, picks the music.
void fn_80040ACC(u8* arg0, u8* arg1, u8* arg2) {
    lbl_805F8538 = 8;
    *(s32*)(gBattle + 0xAD0) = lbl_805FB028;
    BattleSetFighter(gBattle, 0, arg1, 0);
    BattleSetFighter(gBattle, 1, arg2, 0);

    u8* n = (u8*)fn_801D40E0(0x10);
    if (n != NULL) n = fn_800418E0(n, 1);
    fn_80213130(gBattle, n);

    n = (u8*)fn_801D40E0(0x24);
    if (n != NULL) fn_80041AB4(n);
    fn_80213150(gBattle, n);

    n = (u8*)fn_801D40E0(0x48);
    if (n != NULL) n = fn_800497F4(n);
    fn_80213130(gBattle, n);

    n = (u8*)fn_801D40E0(0x18);
    if (n != NULL) fn_8004AC68(n);
    fn_80213810(gBattle, n);

    n = (u8*)fn_801D40E0(1);
    if (n != NULL) fn_80041874(n);
    *(u8**)(arg0 + 0x408) = n;

    n = (u8*)fn_801D40E0(0x264);
    if (n != NULL) fn_80042C38(n);
    lbl_805FB058 = n;
    fn_80042E04(NULL);  // r3 stale in asm

    for (s32 i = 0; i < 2; i++) {
        FighterStatsSetOwner(((u8**)(gBattle + 0xA00))[i] + 0x20,
                             lbl_805FB070[i] == 0 ? lbl_803C51D8[i * 2] : lbl_803C51D8[i * 2 + 1]);
        ((void**)(gBattle + 0xA54))[i] = NULL;
        ((void**)(gBattle + 0xA54))[i] = fn_80043910(lbl_805FB058, i, lbl_805FB070[i] != 0);
    }

    u8* ch = (u8*)CharacterFind(0x01AE9B07);
    *(u8**)(arg0 + 0x4D4) = ch;
    *(u32*)(ch + 0x15C) |= 0x400000;
    fn_801B4160(lbl_8047C7D8);

    u8* f0 = ((u8**)(gBattle + 0xA00))[0];
    ((void**)(gBattle + 0xA54))[0] = NULL;
    if (lbl_803C4298[0x1C] == 1) {
        FighterStatsSetOwner(f0 + 0x20, lbl_803C51D8[1]);
        ((void**)(gBattle + 0xA54))[0] = fn_80043910(lbl_805FB058, 0, 1);
    } else {
        FighterStatsSetOwner(f0 + 0x20, lbl_803C51D8[4]);
        ((void**)(gBattle + 0xA54))[0] = fn_80043910(lbl_805FB058, 0, 0);
    }

    u8* f1 = ((u8**)(gBattle + 0xA00))[1];
    ((void**)(gBattle + 0xA54))[1] = NULL;
    BOOL alt = FALSE;
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
        (*(VGetIntFn*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) {
        alt = TRUE;
    }
    if (alt) {
        FighterStatsSetOwner(f1 + 0x20, lbl_803C51D8[5]);
        ((void**)(gBattle + 0xA54))[1] = fn_80043910(lbl_805FB058, 1, 2);
    } else {
        FighterStatsSetOwner(f1 + 0x20, lbl_803C51D8[3]);
        ((void**)(gBattle + 0xA54))[1] = fn_80043910(lbl_805FB058, 1, 1);
    }

    *(void**)(arg0 + 0x4F0) = fn_80044F54();
    if (lbl_803C4298[0x1C] == 0x1D) fn_80040F44();
    if (fn_8003F8D4() != 0) {
        app_PlayMusic(0x06BE5C73u, 1.0f);
    } else {
        app_PlayMusic(0x98A15A88u, 1.0f);
    }
}

// 0x8008B850
// Sets up the nine preset Euler orientations (+0x454) and their quaternions (+0x37C), and resets
// the blend state.
void fn_8008B850(u8* arg0) {
    static const f32 kEuler[27] = {
        -0.33333334f, 0.0f,  0.7777778f,
        -0.22222222f, 0.0f,  0.0f,
        -0.33333334f, 0.0f, -0.7777778f,
        -0.8888889f,  0.0f,  0.8888889f,
        -0.8888889f,  0.0f,  0.0f,
        -0.8888889f,  0.0f, -0.8888889f,
        -0.6666667f,  0.0f,  0.5f,
        -0.6666667f,  0.0f,  0.0f,
        -0.6666667f,  0.0f, -0.5f,
    };
    for (s32 i = 0; i < 9; i++) {
        f32* e = (f32*)(arg0 + 0x454 + i * 12);
        e[0] = kEuler[i * 3];
        e[1] = kEuler[i * 3 + 1];
        e[2] = kEuler[i * 3 + 2];
    }
    for (s32 i = 0; i < 9; i++) {
        EulerToQuat((Quat*)(arg0 + 0x37C + i * 0x18), (const Vec3*)(arg0 + 0x454 + i * 12));
    }
    *(s32*)(arg0 + 0x264) = 0;
    *(u8*)(arg0 + 0x290) = 0;
    *(f32*)(arg0 + 0x8) = 1.0f;
    *(f32*)(arg0 + 0xC) = 1.0f;
    *(f32*)(arg0 + 0x10) = 1.0f;
    *(u8*)(arg0 + 0x268) = 1;
}

// 0x8008BA80
// Blends the preset orientations: weights the four nearest of the nine presets to the point
// (+0x364, +0x368) by inverse distance, slerps them into the quaternion at +0x280, advances the
// time at +0x27C and pushes the result to the node.
void fn_8008BA80(u8* arg0, f32 dt) {
    f32 w[4]   = { -1.0f, -1.0f, -1.0f, -1.0f };   // squared distances of the chosen presets
    s32 idx[4] = { -1, -1, -1, -1 };               // preset index per slot
    f32 inv[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    f32 sum = 0.0f;
    s32 iMax = -1;   // slot holding the largest distance (next to replace), -1 = unknown
    s32 iMin = 0;    // slot holding the smallest distance

    for (s32 i = 0; i < 9; i++) {
        u8* e = arg0 + i * 0x18;
        f32 fy = (f32)*(s32*)(arg0 + 0x368) - *(f32*)(e + 0x390);
        f32 fx = (f32)*(s32*)(arg0 + 0x364) - *(f32*)(e + 0x38C);
        f32 fz = 0.0f;
        f32 d = (fz * fz) + ((fx * fx) + (fy * fy));
        if (i < 4 && w[i] < 0.0f) {
            sum += d;
            w[i] = d;
            idx[i] = i;
        } else {
            if (iMax == -1) {
                iMax = 0;
                iMin = 0;
                for (s32 k = 0; k < 4; k++) {
                    if (w[k] < w[iMin]) iMin = k;
                    if (w[k] > w[iMax]) iMax = k;
                }
            }
            if (iMax >= 0) {
                f32 old = w[iMax];
                if (d < old) {
                    w[iMax] = d;
                    sum = (sum - old) + d;
                    idx[iMax] = i;
                    if (d < w[iMin]) iMin = iMax;
                    iMax = -1;
                }
            }
        }
    }

    f32 acc = 0.0f;
    BOOL blend = TRUE;
    *(f32*)(arg0 + 0x280) = 0.0f;
    *(f32*)(arg0 + 0x284) = 0.0f;
    *(f32*)(arg0 + 0x288) = 0.0f;
    *(f32*)(arg0 + 0x28C) = 1.0f;
    for (s32 j = 0; j < 4; j++) {
        if (w[j] > 0.0f) {
            f32 t = w[j] / sum;
            if (t < 0.0f) t = 0.0f;
            else if (t > 1.0f) t = 1.0f;
            f32 r = 1.0f / t;
            acc += r;
            inv[j] = r;
        } else {
            // Exact hit: take that preset directly.
            blend = FALSE;
            u8* q = arg0 + idx[j] * 0x18;
            *(f32*)(arg0 + 0x280) = *(f32*)(q + 0x37C);
            *(f32*)(arg0 + 0x284) = *(f32*)(q + 0x380);
            *(f32*)(arg0 + 0x288) = *(f32*)(q + 0x384);
            *(f32*)(arg0 + 0x28C) = *(f32*)(q + 0x388);
            break;
        }
    }
    if (blend) {
        for (s32 k = 0; k < 4; k++) {
            f32 out[4];
            fn_80145F9C(out, (f32*)(arg0 + 0x280), (f32*)(arg0 + idx[k] * 0x18 + 0x37C), inv[k] / acc);
            *(f32*)(arg0 + 0x280) = out[0];
            *(f32*)(arg0 + 0x284) = out[1];
            *(f32*)(arg0 + 0x288) = out[2];
            *(f32*)(arg0 + 0x28C) = out[3];
        }
    }

    *(f32*)(arg0 + 0x27C) = *(f32*)(arg0 + 0x27C) + dt;
    *(f32*)(arg0 + 0x26C) = *(f32*)(arg0 + 0x280);
    *(f32*)(arg0 + 0x270) = *(f32*)(arg0 + 0x284);
    *(f32*)(arg0 + 0x274) = *(f32*)(arg0 + 0x288);
    *(f32*)(arg0 + 0x278) = *(f32*)(arg0 + 0x28C);
    f32 axis[4];
    fn_801455E0(axis, arg0 + 0x280);
    fn_80146180(arg0 + 0x26C, (const Vec3*)axis, 4.0f * *(f32*)(arg0 + 0x27C));

    u8* node = *(u8**)(arg0 + 0x4);
    *(f32*)(node + 0xC)  = *(f32*)(arg0 + 0x280);
    *(f32*)(node + 0x10) = *(f32*)(arg0 + 0x284);
    *(f32*)(node + 0x14) = *(f32*)(arg0 + 0x288);
    *(f32*)(node + 0x18) = *(f32*)(arg0 + 0x28C);
    NodeMarkDirty(arg0);
}

// 0x8008CCC4
// Resets the display state: scale 1 on the object, node moved to (0, -5, -5), vectors cleared.
void fn_8008CCC4(u8* arg0) {
    f32 s = 1.0f;
    *(s32*)(arg0 + 0x264) = 1;
    *(u8*)(arg0 + 0x2D0) = 0;
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    *(f32*)(arg0 + 0x2D4) = s;
    u8* node = *(u8**)(arg0 + 0x4);
    *(f32*)(arg0 + 0x8) = s;
    *(f32*)(arg0 + 0xC) = s;
    *(f32*)(arg0 + 0x10) = s;
    *(f32*)(node + 0x0) = 0.0f;
    *(f32*)(node + 0x4) = -5.0f;
    *(f32*)(node + 0x8) = -5.0f;
    NodeMarkDirty(arg0);
    *(f32*)(arg0 + 0x2E0) = 0.0f;
    *(f32*)(arg0 + 0x2E4) = 0.0f;
    *(f32*)(arg0 + 0x2E8) = 0.0f;
    *(f32*)(arg0 + 0x2EC) = 0.0f;
    *(f32*)(arg0 + 0x2F0) = 0.0f;
    *(f32*)(arg0 + 0x2F4) = 0.0f;
    *(f32*)(arg0 + 0x2F8) = -1.0f;
    *(f32*)(arg0 + 0x2FC) = -1.0f;
    *(f32*)(arg0 + 0x300) = -1.0f;
    *(f32*)(arg0 + 0x2B4) = 0.0f;
    *(f32*)(arg0 + 0x2A4) = 0.0f;
    *(u8*)(arg0 + 0x310) = 0;
    if (*(u8*)(arg0 + 0x274) != 0) {
        *(f32*)(arg0 + 0x278) = 0.0f;
        *(s32*)(arg0 + 0x268) = 1;
    }
}

// 0x8008F1F8
// Registers the robot-evolution lab objects of the zone: every flagged object is classified by
// its type hash and bound to the matching lane slot (left / middle / right by its x sign).
void fn_8008F1F8(u8* arg0) {
    s32 total = ZoneEntryCount();
    for (s32 i = 0; i < total; i++) {
        u8* obj;
        u8* rec;
        ZoneEntry(i, &obj, &rec);
        if (!(*(u32*)(rec + 0x1C) & 0x40) || obj == NULL) continue;
        u32 type = ObjTypeHash(obj);
        if (type == kHashLightBeam) {
            *(u32*)(obj + 0x15C) |= 0x400000;
            ScopedLock sl;
            ScopedLockCtor(&sl);
            (*(VCallIntFn*)(*(u8**)obj + 0x18))(obj, 1);
            ScopedLockDtor(&sl, -1);
            f32 s = **(f32**)(obj + 4);
            if (s >= 1.0f)       *(u8**)(arg0 + 0x98) = obj;
            else if (s <= -1.0f) *(u8**)(arg0 + 0xA0) = obj;
            else                 *(u8**)(arg0 + 0x9C) = obj;
        } else if (type == kHashRobotCurtains) {
            *(u8**)(arg0 + 0xA4) = obj;
            *(u32*)(obj + 0x15C) |= 0x400000;
        } else if (type == kHashRobotGears) {
            *(u8**)(arg0 + 0x90) = obj;
            *(u32*)(obj + 0x15C) |= 0x400000;
            u8* g = *(u8**)(arg0 + 0x90);
            u8* r = (*(VGetPtrFn*)(*(u8**)g + 0x10))(g);
            if (r != NULL) *(f32*)(arg0 + 0x94) = *(f32*)(r + 0xB0);
        } else if (type == kHashRobotKid) {
            *(u32*)(obj + 0x15C) |= 0x600000;
            f32 s = **(f32**)(obj + 4);
            if (s >= 1.0f) {
                fn_8009231C(arg0 + 0xA8, 0, obj, 0x90252014u, *(s32*)(arg0 + 0x290), *(s32*)(arg0 + 0x29C), *(s32*)(arg0 + 0x284));
            } else if (s <= -1.0f) {
                fn_8009231C(arg0 + 0x138, 2, obj, 0x07BA313Du, *(s32*)(arg0 + 0x298), *(s32*)(arg0 + 0x2A4), *(s32*)(arg0 + 0x28C));
            } else {
                fn_8009231C(arg0 + 0xF0, 1, obj, 0xA1CD3A89u, *(s32*)(arg0 + 0x294), *(s32*)(arg0 + 0x2A0), *(s32*)(arg0 + 0x288));
            }
        } else if (type == kHashEvolutionDoor) {
            *(u32*)(obj + 0x15C) |= 0x400000;
            f32 s = **(f32**)(obj + 4);
            if (s >= 1.0f)       fn_80091E28(arg0 + 0x1F8, 0, obj);
            else if (s <= -1.0f) fn_80091E28(arg0 + 0x220, 2, obj);
            else                 fn_80091E28(arg0 + 0x20C, 1, obj);
        } else if (type == kHashSmasher) {
            *(u32*)(obj + 0x15C) |= 0x400000;
            f32 s = **(f32**)(obj + 4);
            if (s >= 1.0f) {
                fn_800939EC(arg0 + 0x180, 0, obj, *(s32*)(arg0 + 0x290), *(s32*)(arg0 + 0x284));
            } else if (s <= -1.0f) {
                fn_800939EC(arg0 + 0x1D0, 2, obj, *(s32*)(arg0 + 0x298), *(s32*)(arg0 + 0x28C));
            } else {
                fn_800939EC(arg0 + 0x1A8, 1, obj, *(s32*)(arg0 + 0x294), *(s32*)(arg0 + 0x288));
            }
        } else {
            *(u32*)(obj + 0x15C) |= 0x400000;
            ScopedLock sl;
            ScopedLockCtor(&sl);
            (*(VCallIntFn*)(*(u8**)obj + 0x18))(obj, 1);
            ScopedLockDtor(&sl, -1);
        }
    }
    for (s32 i = 0; i < 5; i++) {
        f32 a, b, c, d;
        fn_80217134(arg0 + 0x2A8, i, &a, &b, &c, &d);
        *(f32*)(arg0 + i * 0x10 + 0x234) = 0.1f;
        *(f32*)(arg0 + i * 0x10 + 0x238) = 0.1f;
        *(f32*)(arg0 + i * 0x10 + 0x23C) = 0.1f;
        *(f32*)(arg0 + i * 0x10 + 0x240) = 0.0f;
    }
}

// 0x8009133C
// Lab state change (state 0 reset, 1 start round with shuffled slot scores, 2 win, 3 plain,
// 4 end: hide the selected slot, release the texture, score and refresh the slots).
void fn_8009133C(u8* p, s32 state) {
    if (state == 0) {
        *(s32*)(p + 0x7FC) = state;
        for (s32 i = 0; i < 3; i++) {
            fn_800925A4(p + 0xA8 + i * 0x48);
            fn_80091EE8(p + 0x1F8 + i * 0x14);
        }
    } else if (state == 1) {
        s32 r = RaySceneTest(NULL);
        *(f32*)(p + 0x7EC) = lbl_80347200[(r >= 0 && r <= 3) ? r : 0];
        if (fn_80217094(p + 0x2A8, *(s32*)(p + 0x354))) {
            r = RaySceneTest(NULL);
            *(f32*)(p + 0x7F0) = lbl_80347210[(r >= 0 && r <= 3) ? r : 0];
        } else {
            *(f32*)(p + 0x7F0) = 1.0f + *(f32*)(p + 0x7EC);
        }
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0xD05A9099u, 0, 0, 0, 0, 0);
        fn_80091950(p);
        *(s32*)(p + 0x7FC) = state;
        f32 v[3];
        v[0] = 0.5f + (0.0f + (0.0625f * (2.3283064e-10f * (f32)RandomU32())));
        v[1] = 0.5f + (0.09375f + (0.0625f * (2.3283064e-10f * (f32)RandomU32())));
        v[2] = 0.5f + (0.1875f + (0.0625f * (2.3283064e-10f * (f32)RandomU32())));
        for (s32 k = 0; k < 5; k++) {
            u32 a = RandomU32() % 3;
            u32 b = RandomU32() % 3;
            if (a != b) {
                f32 t = v[a];
                v[a] = v[b];
                v[b] = t;
            }
        }
        for (s32 i = 0; i < 3; i++) {
            fn_800925B0(p + 0xA8 + i * 0x48, v[i]);
            fn_80091F18(p + 0x1F8 + i * 0x14, v[i]);
        }
    } else if (state == 2) {
        *(s32*)(p + 0x7FC) = state;
        *(u8*)(p + 0x7E4) = 1;
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0x876455A2u, 0, 0, 0, 0, 0);
        if (*(u8**)(p + 0x358) != NULL) UIElement_PlayUIAnim(0x7DBA57FFu, 0x15726E29u);
    } else if (state == 3) {
        *(s32*)(p + 0x7FC) = state;
    } else if (state == 4) {
        s32 sel = *(s32*)(p + 0x7E0);
        if (sel != -1) {
            fn_80093C20(p + sel * 0x28 + 0x180, 0);
            u8* obj = *(u8**)(p + *(s32*)(p + 0x7E0) * 4 + 0x98);
            ScopedLock sl;
            ScopedLockCtor(&sl);
            (*(VCallIntFn*)(*(u8**)obj + 0x18))(obj, 1);
            ScopedLockDtor(&sl, -1);
        }
        u8* tex = *(u8**)(p + 0x358);
        *(s32*)(p + 0x7E0) = -1;
        if (tex != NULL) {
            UIElement_PlayUIAnim(0x7DBA57FFu, 0xB729E3E6u);
            tex = *(u8**)(p + 0x358);
            s32 rc = *(s32*)(tex + 0x24) - 1;
            *(s32*)(tex + 0x24) = rc;
            if (rc == 0) TextureRelease(tex);
            *(u8**)(p + 0x358) = NULL;
        }
        u8 won = *(u8*)(p + 0x808);
        *(s32*)(p + 0x7FC) = state;
        if (won == 0) {
            fn_800712B4();
            for (s32 i = 0; i < 3; i++) {
                if (*(s32*)(p + i * 0x48 + 0xB0) != 2) {
                    s32 pts = lbl_803471C0[RaySceneTest(NULL)];
                    *(s32*)(p + 0x7F4) = *(s32*)(p + 0x7F4) + pts;
                    fn_80070FAC(p, (*(VGetScoreFn*)(*(u8**)(p + 0x4) + 0x64))(p) / 100.0f);
                }
            }
        } else {
            fn_800712B8();
            *(s32*)(p + 0x7F8) = *(s32*)(p + 0x7F8) + 1;
        }
        fn_80092D10();
        fn_80092A30(p + 0xA8, *(u8*)(p + 0x808));
        fn_80092A30(p + 0xF0, *(u8*)(p + 0x808));
        fn_80092A30(p + 0x138, *(u8*)(p + 0x808));
    }
}

// 0x800975D0
// Shell record setup: owner object, the two bytes of the definition, then the shell's position
// index looked up by name ("shell_%d"); the record's last position is nudged halfway toward the
// previous flagged one.
void fn_800975D0(u8* p, u8* def, u8* owner, s32 idx) {
    u8 b0 = def[0];
    u8 b1 = def[1];
    *(u8**)p = owner;
    *(u8*)(p + 8) = b0;
    *(u8*)(p + 9) = b1;
    char name[16];
    SysSprintf(name, "shell_%d", idx + 1);
    u32 hash = StringHash(name);
    u8* obj = *(u8**)p;
    *(s32*)(p + 4) = fn_8018BDE0((*(VGetPtrFn*)(*(u8**)obj + 0x10))(obj), (s32)hash);
    obj = *(u8**)p;
    s32 n = *(s32*)(p + 4);
    u8* tab = *(u8**)(*(VGetPtrFn*)(*(u8**)obj + 0x10))(obj);
    s32 sel = 0;
    for (s32 i = n - 1; i > 0; i--) {
        if (*(u32*)(*(u8**)(tab + 0xC) + i * 8 + 4) & 1) {
            sel = i;
            break;
        }
    }
    u8* base = *(u8**)(tab + 0x4);
    u8* a = base + sel * 0x30;
    u8* b = base + n * 0x30;
    f32 dx = *(f32*)(b + 0x24) - *(f32*)(a + 0x24);
    f32 dy = *(f32*)(b + 0x28) - *(f32*)(a + 0x28);
    f32 dz = *(f32*)(b + 0x2C) - *(f32*)(a + 0x2C);
    *(f32*)(b + 0x24) = *(f32*)(b + 0x24) + dx * 0.5f;
    *(f32*)(b + 0x28) = *(f32*)(b + 0x28) + dy * 0.5f;
    *(f32*)(b + 0x2C) = *(f32*)(b + 0x2C) + dz * 0.5f;
}

// 0x80097A18
// Shell instance setup: binds the owner, picks the "shell_%d_%cLoc%d" location by name ('b' or
// 's' by the virtual query at +0xBC), orients it and rolls the initial direction flag.
void fn_80097A18(u8* p, u8* owner, s32 a, s32 b) {
    ScopedLock sl;
    ScopedLockCtor(&sl);
    (*(VCallIntFn*)(*(u8**)p + 0x14))(p, 1);
    ScopedLockDtor(&sl, -1);
    *(u8**)(p + 0x264) = owner;
    s32 kind = (*(VGetIntFn*)(*(u8**)p + 0xBC))(p);
    s32 c = 0x62;
    if (kind == 0) c = 0x73;
    char name[16];
    SysSprintf(name, "shell_%d_%cLoc%d", a + 1, c, b + 1);
    *(u32*)(p + 0x268) = fn_801EC880(owner + 0x94, StringHash(name));
    fn_800980F8(p, 0);
    fn_80097BFC(p);
    f32 r = 2.3283064e-10f * (f32)RandomU32();
    *(u8*)(p + 0x26D) = r < lbl_80347190[RaySceneTest(NULL)] ? 1 : 0;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = -1.0f;
    *(u8*)(p + 0x26E) = 1;
}

// 0x80097BFC
// Poses the shell node: its orientation is the entry's matrix quaternion turned -1 rad about X,
// its position the entry's position pushed along the facing (forward when +0x26C is set, back
// by a small step otherwise).
void fn_80097BFC(u8* p) {
    u8* owner = *(u8**)(p + 0x264);
    Quat q;
    QuatFromMtx(&q, (const f32*)(*(u8**)(owner + 0x98) + *(s32*)(p + 0x268) * 0xC8 + 0x38));
    Vec3 axis = { 1.0f, 0.0f, 0.0f };
    Quat r;
    QuatFromAxisAngle(&r, &axis, -1.0f);
    q = QuatMulRaw(q, r);
    u8* node = *(u8**)(p + 0x4);
    *(f32*)(node + 0xC)  = q.x;
    *(f32*)(node + 0x10) = q.y;
    *(f32*)(node + 0x14) = q.z;
    *(f32*)(node + 0x18) = q.w;
    NodeMarkDirty(p);

    u8* ent = *(u8**)(owner + 0x98) + *(s32*)(p + 0x268) * 0xC8;
    Vec3 pos = { *(f32*)(ent + 0x5C), *(f32*)(ent + 0x60), *(f32*)(ent + 0x64) };
    Vec3 fwd;
    if (*(u8*)(p + 0x26C) != 0) {
        f32 t = (f32)*(u8*)(p + 0x26C) / (f32)lbl_80347170[RaySceneTest(NULL)];
        fn_801455E0(&fwd.x, (u8*)&q);
        Vec3 n = Vec3Normalized(fwd);
        f32 s = 0.2f * t;
        pos.x = pos.x + n.x * s;
        pos.y = pos.y + n.y * s;
        pos.z = pos.z + n.z * s;
    } else {
        fn_801455E0(&fwd.x, (u8*)&q);
        Vec3 n = Vec3Normalized(fwd);
        pos.x = pos.x - n.x * 0.05f;
        pos.y = pos.y - n.y * 0.05f;
        pos.z = pos.z - n.z * 0.05f;
    }
    node = *(u8**)(p + 0x4);
    *(f32*)(node + 0x0) = pos.x;
    *(f32*)(node + 0x4) = pos.y;
    *(f32*)(node + 0x8) = pos.z;
    NodeMarkDirty(p);
}

// 0x80098E74
// Resets the shell timer state: index 0, a table value by difficulty at +0x298, +0x270 = 0 and
// +0x274 = 0.16.
void fn_80098E74(u8* arg0) {
    s32 d = RaySceneTest(NULL);
    *(s32*)(arg0 + 0x268) = 0;
    *(f32*)(arg0 + 0x298) = lbl_80346FC0[d];
    *(f32*)(arg0 + 0x270) = 0.0f;
    *(f32*)(arg0 + 0x274) = 0.16f;
}

// 0x80099D08
// Astral zone setup: counts the pickup objects, allocates the pickup list, then binds the avatar
// (scaled 5x, handed to fn_8009C414), collects the pickups (hidden) and turns the curved
// objects; finally sends flagged helper objects far away.
void fn_80099D08(u8* arg0) {
    *(s32*)(arg0 + 0x1A0) = 0;
    s32 total = ZoneEntryCount();
    for (s32 i = 0; i < total; i++) {
        u8* obj;
        u8* rec;
        ZoneEntry(i, &obj, &rec);
        if ((*(u32*)(rec + 0x1C) & 0x40) && obj != NULL) {
            if (ObjTypeHash(obj) == kHashAstralPickup) *(s32*)(arg0 + 0x1A0) += 1;
        }
    }
    if (*(s32*)(arg0 + 0x1A0) > 0) *(void**)(arg0 + 0x19C) = MemAllocSmall(*(s32*)(arg0 + 0x1A0) * sizeof(void*));

    s32 np = 0;
    total = ZoneEntryCount();
    for (s32 i = 0; i < total; i++) {
        u8* obj;
        u8* rec;
        ZoneEntry(i, &obj, &rec);
        if (!(*(u32*)(rec + 0x1C) & 0x40) || obj == NULL) continue;
        u32 type = ObjTypeHash(obj);
        if (type == kHashAstralAvatar) {
            *(u8**)(arg0 + 0x198) = obj;
            u8* node = *(u8**)(obj + 4);
            *(f32*)(node + 0) = 0.0f;
            *(f32*)(node + 4) = 0.0f;
            *(f32*)(node + 8) = 0.0f;
            NodeMarkDirty(obj);
            u8* av = *(u8**)(arg0 + 0x198);
            *(f32*)(av + 0x8) = 5.0f;
            *(f32*)(av + 0xC) = 5.0f;
            *(f32*)(av + 0x10) = 5.0f;
            NodeMarkDirty(av);
            fn_8009C414(*(u8**)(arg0 + 0x198), arg0);
        } else if (type == kHashAstralPickup) {
            u8** list = *(u8***)(arg0 + 0x19C);
            list[np] = obj;
            *(u32*)(list[np] + 0x15C) |= 0x400000;
            u8* o = (*(u8***)(arg0 + 0x19C))[np];
            ScopedLock sl;
            ScopedLockCtor(&sl);
            (*(VCallIntFn*)(*(u8**)o + 0x18))(o, 0);
            ScopedLockDtor(&sl, -1);
            u8* o2 = (*(u8***)(arg0 + 0x19C))[np];
            np++;
            *(u8*)(o2 + 0x270) = 0;
        } else if (type == kHashAstralCurved) {
            Vec3 axis = { 0.0f, 1.0f, 0.0f };
            Quat r;
            QuatFromAxisAngle(&r, &axis, 0.5f);
            u8* node = *(u8**)(obj + 4);
            Quat a = { *(f32*)(node + 0xC), *(f32*)(node + 0x10), *(f32*)(node + 0x14), *(f32*)(node + 0x18) };
            Quat m = QuatMulRaw(r, a);
            *(f32*)(node + 0xC)  = m.x;
            *(f32*)(node + 0x10) = m.y;
            *(f32*)(node + 0x14) = m.z;
            *(f32*)(node + 0x18) = m.w;
            NodeMarkDirty(obj);
        }
    }

    total = ZoneEntryCount();
    for (s32 i = 0; i < total; i++) {
        u8* obj;
        u8* rec;
        ZoneEntry(i, &obj, &rec);
        if ((*(u32*)(rec + 0x1C) & 0x20000) && obj != NULL) {
            (*(VGetIntFn*)(*(u8**)obj + 0xBC))(obj);
            u8* node = *(u8**)(obj + 4);
            *(f32*)(node + 0) = -20.0f;
            *(f32*)(node + 4) = -20.0f;
            *(f32*)(node + 8) = -20.0f;
            NodeMarkDirty(obj);
        }
    }
}

}  // extern "C"
