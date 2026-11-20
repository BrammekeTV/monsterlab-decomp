// Functions in 0x800Axxxx (saw-blade boss prop: setup, state callbacks, std::map node insert/erase).
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFnSelf)(u8* self);              // virtual slot taking only the sub-object
typedef u8*  (*VFnRetPtr)(u8* self);            // virtual slot returning a pointer
typedef void (*VFnGetVec)(f32* out, u8* self);  // virtual slot writing a vec3

extern "C" {
s32   AnimSetFindClip(void* set, s32 anim);                      // 0x801AC2FC
void  EntitySyncTransform(void* entity, int a);                  // 0x801B4F4C
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
void  PropPlayAnim(void* prop, int a, int b, BOOL once);         // 0x80069350
s32   RaySceneTest(void* ray);                                   // 0x80070458 (ignores its argument)
void  fn_80070460(u8* p);                                        // 0x80070460
void  fn_80070FAC(u8* p, f32 v);                                 // 0x80070FAC
void  fn_800712BC(u8* p, s32 a, s32 b);                          // 0x800712BC
void  fn_800A0BAC(u8* p, f32 dt);                                // 0x800A0BAC
void fn_800A2AEC(u8* node, u8* root);                // 0x800A2AEC
void  fn_800A43C0(u8* p, s32 hash);                              // 0x800A43C0
void  fn_800A660C(u8* p);                                        // 0x800A660C
s32   fn_800A6694(u8* p);                                        // 0x800A6694 (returns a PlaySound handle)
void  fn_800A7020(u8* a, u8* b, u8* c, f32 dt);                  // 0x800A7020
void  fn_800A7064(u8* p, f32 dt);                                // 0x800A7064
void  fn_800A71C8(u8* p, u32 a);                                 // 0x800A71C8
void  fn_800A75C8(u8* p);                                        // 0x800A75C8
void  fn_800A76B4(u8* p);                                        // 0x800A76B4
void  fn_800A7828(u8* p);                                        // 0x800A7828
f32   fn_800A791C(u8* p);                                        // 0x800A791C
u8*   fn_800C2FF0(u8* p);                                        // 0x800C2FF0
s32   fn_8018BDE0(u8* p, s32 key);                               // 0x8018BDE0
f32   fn_8018BE30(u8* p, f32 t);                                 // 0x8018BE30
void  fn_8018BEA4(u8* p, f32 t);                                 // 0x8018BEA4
u8*   fn_8018C39C(u8* p, s32 idx);                               // 0x8018C39C
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
void* fn_80209814(void* v);                                      // 0x80209814
s32   fn_80336BF8(u8* file, const char* fmt, ...);               // 0x80336BF8
void  fn_8033B09C(void);                                         // 0x8033B09C
void  operator_delete(void* p);                                  // 0x801D426C

extern u8   gWorldState[];          // 0x803E6D00 (holds a pointer to the world block; +0xC frame delta)
extern u8   __files[];              // 0x803B4978
extern "C++" s32  gZoneResourceSize;      // 0x805FB94C
extern f32  lbl_80346A58[];         // 0x80346A58 (f32 table indexed by RaySceneTest)
extern f32  lbl_80346B18[];         // 0x80346B18 (vec3 offset)
extern f32  lbl_80346B28[];         // 0x80346B28 (f32 table indexed by RaySceneTest)
extern f32  lbl_803C6C20[];         // 0x803C6C20 (vec3 offset)
extern f32  lbl_805FDE40;           // 0x805FDE40 (0.0f)
extern f32  lbl_805FDE44;           // 0x805FDE44 (start value for +0xCF4)
extern f32  lbl_805FDE50;           // 0x805FDE50 (1.0f)
extern f32  lbl_805FDE78;           // 0x805FDE78 (x offset)
extern f32  lbl_805FDE7C;           // 0x805FDE7C (scale)
extern f32  lbl_805FDE80;           // 0x805FDE80 (threshold)
extern f32  lbl_805FDE84;           // 0x805FDE84 (time limit)
extern f32  lbl_805FDE88;           // 0x805FDE88 (threshold)
extern s8   lbl_805FB15C;           // 0x805FB15C
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)

static const u32 kHashSawBlade      = 0xF543EFD5u;  // "saw_bladeA"
static const u32 kHashSawTrack01    = 0x05C5F486u;  // "saw_track_01A"
static const u32 kHashSawTrackStart = 0xEEA13AE2u;  // "saw_track_startA"
static const u32 kHashSawTrackEnd   = 0xA93CF6ECu;  // "saw_track_endA"
static const u32 kHashSawRollers    = 0xC2B3381Eu;  // "saw_rollersA"
static const u32 kHashSawArm01      = 0x124C6C9Bu;  // "saw_arm_01A"

// Sets the "position written / dirty" flag bits on a transform block (0x20 word), one load/store each.
static void SetMoveFlags(u8* t) {
    *(u32*)(*(u8**)(t + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(t + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(t + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(t + 0x4) + 0x20) |= 0x10000000;
}

// Creates the shared voice SFX handle if it does not exist yet.
static void EnsureVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// 0x800A0FB4
// Scans the zone's entity list and stores the saw-blade, rollers and arm props (and the track
// pieces' extra data) into the boss object, then calls fn_800A660C on the blade.
void fn_800A0FB4(u8* p) {
    s32 i = 0;
    s32 off = 0;
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    while (i < *(s32*)(zone + 0x98)) {
        u8* ent = *(u8**)(*(u8**)(zone + 0x12C) + off);
        u8* def = *(u8**)(ent + 0x9C);
        u32 hash = def != NULL ? *(u32*)(def + 0x8) : 0;
        switch (hash) {
        case kHashSawBlade:
            *(u8**)(p + 0xC8C) = ent;
            break;
        case kHashSawTrack01:
        case kHashSawTrackStart:
        case kHashSawTrackEnd: {
            (*(VFnRetPtr*)(*(u8**)ent + 0xBC))(ent);
            u8* d = *(u8**)(ent + 0x9C);
            fn_800A43C0(p + 0xCD8, d != NULL ? *(s32*)(d + 0x8) : 0);
            break;
        }
        case kHashSawRollers: {
            *(u8**)(p + 0xCAC) = ent;
            *(u32*)(ent + 0x15C) |= 0x600000;
            u8* d = *(u8**)(*(u8**)(p + 0xCAC) + 0x9C);
            if (d != NULL) {
                u8* set = *(u8**)(d + 0x48);
                if (set != NULL) {
                    PropPlayAnim(*(u8**)(p + 0xCAC), AnimSetFindClip(set, (s32)0xE7CC3CFAu), 0, 1);
                    u8* roller = *(u8**)(p + 0xCAC);
                    u8* o = (*(VFnRetPtr*)(*(u8**)roller + 0x10))(roller);
                    *(s32*)(*(u8**)(*(u8**)o + 0x14) + 0x20) = 1;
                }
            }
            break;
        }
        case kHashSawArm01: {
            *(u8**)(p + 0xCD4) = ent;
            u8* o = (*(VFnRetPtr*)(*(u8**)ent + 0xC))(ent);
            if (o != NULL) {
                o = (*(VFnRetPtr*)(*(u8**)ent + 0xC))(ent);
                *(u32*)(o + 0x8) &= 0xFFFFFFFDu;
            }
            *(u32*)(*(u8**)(ent + 0x4) + 0x20) &= 0xFFFFFFFDu;
            break;
        }
        default:
            break;
        }
        off += 4;
        i += 1;
    }
    fn_800A660C(*(u8**)(p + 0xC8C));
}

// 0x800A1850
// Moves the arm prop to the target's position shifted by lbl_805FDE78 in x, marks it, and starts its
// anim clip 0xECFDEE97.
void fn_800A1850(u8* p, u8* target) {
    u8* tpos = *(u8**)(target + 0x4);
    u8* arm = *(u8**)(p + 0xCD4);
    f32 x = *(f32*)(tpos + 0x0);
    f32 y = *(f32*)(tpos + 0x4);
    f32 z = *(f32*)(tpos + 0x8);
    u8* xf = *(u8**)(arm + 0x4);
    *(f32*)(xf + 0x0) = lbl_805FDE78 + x;
    *(f32*)(xf + 0x4) = y;
    *(f32*)(xf + 0x8) = z;
    SetMoveFlags(arm);
    u8* arm2 = *(u8**)(p + 0xCD4);
    u8* o = (*(VFnRetPtr*)(*(u8**)arm2 + 0xC))(arm2);
    if (o != NULL) {
        o = (*(VFnRetPtr*)(*(u8**)arm2 + 0xC))(arm2);
        *(u32*)(o + 0x8) |= 2;
    }
    *(u32*)(*(u8**)(arm2 + 0x4) + 0x20) |= 2;
    u8* d = *(u8**)(*(u8**)(p + 0xCD4) + 0x9C);
    if (d != NULL) {
        u8* set = *(u8**)(d + 0x48);
        if (set != NULL) {
            PropPlayAnim(*(u8**)(p + 0xCD4), AnimSetFindClip(set, (s32)0xECFDEE97u), 0, 1);
        }
    }
}

// 0x800A19C0
// Boss start state: signals the blade, starts the two voice sounds, picks the timer from the
// RaySceneTest-indexed table, and places/animates the rollers relative to the boss.
void fn_800A19C0(u8* p) {
    f32 v[3];
    fn_80070460(p);
    fn_800A71C8(*(u8**)(p + 0xC8C), 0xCA90298Eu);
    EnsureVoiceHandle();
    *(s32*)(p + 0xCF8) = PlaySound(0xE2DCC1BAu, 0, 0, 0, 0, 0);
    EnsureVoiceHandle();
    *(s32*)(p + 0xCFC) = PlaySound(0xE7E15D02u, 0, 0, 0, 0, 0);
    fn_800A6694(*(u8**)(p + 0xC8C));
    f32 t = lbl_80346A58[RaySceneTest(NULL)];
    *(f32*)(p + 0xCD0) = t;
    fn_800712BC(p, (s32)t, 0);
    u8* vt = *(u8**)(p + 0x160);
    (*(VFnSelf*)(vt + 0x20))(p + 0x8C);
    (*(VFnGetVec*)(*(u8**)(p + 0x160) + 0x24))(v, p + 0x8C);
    u8* roller = *(u8**)(p + 0xCAC);
    u8* xf = *(u8**)(roller + 0x4);
    *(f32*)(xf + 0x0) = *(f32*)(p + 0xCB0) + v[0];
    *(f32*)(xf + 0x4) = *(f32*)(p + 0xCB4) + v[1];
    *(f32*)(xf + 0x8) = *(f32*)(p + 0xCB8) + v[2];
    SetMoveFlags(roller);
    f32 dt = *(f32*)(gWorldState + 0xC);
    f32 phase = *(f32*)(p + 0xCCC) + dt * fn_800A791C(*(u8**)(p + 0xC8C));
    *(f32*)(p + 0xCCC) = phase;
    if (phase > lbl_805FDE50) *(f32*)(p + 0xCCC) = phase - lbl_805FDE50;
    u8* r = *(u8**)(p + 0xCAC);
    fn_8018BEA4((*(VFnRetPtr*)(*(u8**)r + 0x10))(r), *(f32*)(p + 0xCCC));
    EntitySyncTransform(*(u8**)(p + 0xCAC), 0);
}

// 0x800A1C2C
// Boss update: advances the +0xCF4 and +0xCD0 timers (signalling and notifying the blade when they
// run out), moves the blade/rollers while active, and refreshes the HUD timer.
void fn_800A1C2C(u8* p, f32 dt) {
    f32 v[3];
    f32 v2[3];
    void* ray = (*(void*(**)(u8*))(*(u8**)(p + 0x160) + 0x14))(p + 0x8C);
    f32 t = *(f32*)(p + 0xCF4) + dt * lbl_80346B28[RaySceneTest(ray)];
    *(f32*)(p + 0xCF4) = t;
    fn_80070FAC(p, lbl_805FDE7C * t);
    if (*(f32*)(p + 0xCF4) <= lbl_805FDE40) {
        GameFlowSignal(0x117741C3, 0, 0);
        fn_800A71C8(*(u8**)(p + 0xC8C), 0x50B782D3u);
    }
    if (*(f32*)(p + 0xCD0) <= lbl_805FDE40) {
        GameFlowSignal((s32)0xC608C842u, 0, 0);
        fn_800A71C8(*(u8**)(p + 0xC8C), 0x50B782D3u);
    }
    if (*(u8*)(p + 0xCC8) != 0) {
        fn_800A7020(*(u8**)(p + 0xC8C), p + 0x8C, p + 0xCE8, *(f32*)(gWorldState + 0xC));
        (*(VFnSelf*)(*(u8**)(p + 0x160) + 0x20))(p + 0x8C);
        (*(VFnGetVec*)(*(u8**)(p + 0x160) + 0x24))(v, p + 0x8C);
        (*(VFnSelf*)(*(u8**)(p + 0x160) + 0x20))(p + 0x8C);
        (*(VFnGetVec*)(*(u8**)(p + 0x160) + 0x24))(v2, p + 0x8C);
        u8* roller = *(u8**)(p + 0xCAC);
        u8* xf = *(u8**)(roller + 0x4);
        *(f32*)(xf + 0x0) = *(f32*)(p + 0xCB0) + v2[0];
        *(f32*)(xf + 0x4) = *(f32*)(p + 0xCB4) + v2[1];
        *(f32*)(xf + 0x8) = *(f32*)(p + 0xCB8) + v2[2];
        SetMoveFlags(roller);
        f32 delta = *(f32*)(gWorldState + 0xC);
        f32 phase = *(f32*)(p + 0xCCC) + delta * fn_800A791C(*(u8**)(p + 0xC8C));
        *(f32*)(p + 0xCCC) = phase;
        if (phase > lbl_805FDE50) *(f32*)(p + 0xCCC) = phase - lbl_805FDE50;
        u8* r = *(u8**)(p + 0xCAC);
        fn_8018BEA4((*(VFnRetPtr*)(*(u8**)r + 0x10))(r), *(f32*)(p + 0xCCC));
        EntitySyncTransform(*(u8**)(p + 0xCAC), 0);
        fn_800A76B4(*(u8**)(p + 0xC8C));
    }
    fn_800A7828(*(u8**)(p + 0xC8C));
    fn_800A75C8(*(u8**)(p + 0xC8C));
    fn_800A0BAC(p, dt);
    fn_800C2FF0(p + 0x8C);
    f32 left = *(f32*)(p + 0xCD0) - dt;
    *(f32*)(p + 0xCD0) = left;
    if (left < lbl_805FDE40) *(f32*)(p + 0xCD0) = lbl_805FDE40;
    f32 cur = *(f32*)(p + 0xCD0);
    fn_800712BC(p, (s32)cur, cur < lbl_805FDE80);
}

// 0x800A1F08
// Resets the +0xD00 timer, puts the arm prop at the blade's position plus the lbl_80346B18 offset
// and starts its anim clip 0x0312511A.
void fn_800A1F08(u8* p) {
    *(f32*)(p + 0xD00) = lbl_805FDE40;
    u8* blade = *(u8**)(*(u8**)(p + 0xC8C) + 0x4);
    u8* arm = *(u8**)(p + 0xCD4);
    f32 x = *(f32*)(blade + 0x0);
    f32 y = *(f32*)(blade + 0x4);
    f32 z = *(f32*)(blade + 0x8);
    u8* xf = *(u8**)(arm + 0x4);
    *(f32*)(xf + 0x0) = x + lbl_80346B18[0];
    *(f32*)(xf + 0x4) = y + lbl_80346B18[1];
    *(f32*)(xf + 0x8) = z + lbl_80346B18[2];
    SetMoveFlags(arm);
    u8* d = *(u8**)(*(u8**)(p + 0xCD4) + 0x9C);
    if (d != NULL) {
        u8* set = *(u8**)(d + 0x48);
        if (set != NULL) {
            PropPlayAnim(*(u8**)(p + 0xCD4), AnimSetFindClip(set, 0x0312511A), 0, 1);
        }
    }
}

// 0x800A2020
// Per-frame while the arm is out: counts +0xD00 up (signal at the limit), then either drags the blade
// along with the arm until its anim passes the threshold, or snaps the blade to the arm's bone.
void fn_800A2020(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0xD00) + dt;
    *(f32*)(p + 0xD00) = t;
    if (t >= lbl_805FDE84) {
        GameFlowSignal((s32)0x88C6E941u, 0, 0);
    }
    if (lbl_805FB15C == 0) lbl_805FB15C = 1;
    if (*(u8*)(p + 0xCC8) != 0) {
        fn_800A7064(*(u8**)(p + 0xC8C), dt);
        fn_800A75C8(*(u8**)(p + 0xC8C));
        u8* bpos = *(u8**)(*(u8**)(p + 0xC8C) + 0x4);
        u8* arm = *(u8**)(p + 0xCD4);
        f32 x = *(f32*)(bpos + 0x0);
        f32 y = *(f32*)(bpos + 0x4);
        f32 z = *(f32*)(bpos + 0x8);
        u8* xf = *(u8**)(arm + 0x4);
        *(f32*)(xf + 0x0) = x + lbl_80346B18[0];
        *(f32*)(xf + 0x4) = y + lbl_80346B18[1];
        *(f32*)(xf + 0x8) = z + lbl_80346B18[2];
        SetMoveFlags(arm);
        u8* arm2 = *(u8**)(p + 0xCD4);
        u8* o = (*(VFnRetPtr*)(*(u8**)arm2 + 0x10))(arm2);
        if (fn_8018BE30(o, lbl_805FDE40) > lbl_805FDE88) {
            *(u8*)(p + 0xCC8) = 0;
            u8* arm3 = *(u8**)(p + 0xCD4);
            u8* bpos2 = *(u8**)(*(u8**)(p + 0xC8C) + 0x4);
            *(f32*)(p + 0xCBC) = *(f32*)(bpos2 + 0x0);
            *(f32*)(p + 0xCC0) = *(f32*)(bpos2 + 0x4);
            *(f32*)(p + 0xCC4) = *(f32*)(bpos2 + 0x8);
            u8* obj = (*(VFnRetPtr*)(*(u8**)arm3 + 0x10))(arm3);
            u8* bone = fn_8018C39C(obj, fn_8018BDE0(obj, 0x3FD0CD13));
            *(f32*)(p + 0xCBC) = *(f32*)(p + 0xCBC) - *(f32*)(bone + 0x24);
            *(f32*)(p + 0xCC0) = *(f32*)(p + 0xCC0) - *(f32*)(bone + 0x28);
            *(f32*)(p + 0xCC4) = *(f32*)(p + 0xCC4) - *(f32*)(bone + 0x2C);
        }
    } else {
        u8* arm = *(u8**)(p + 0xCD4);
        u8* obj = (*(VFnRetPtr*)(*(u8**)arm + 0x10))(arm);
        u8* bone = fn_8018C39C(obj, fn_8018BDE0(obj, 0x3FD0CD13));
        u8* blade = *(u8**)(p + 0xC8C);
        f32 x = *(f32*)(bone + 0x24) + lbl_803C6C20[0];
        f32 y = *(f32*)(bone + 0x28) + lbl_803C6C20[1];
        u8* xf = *(u8**)(blade + 0x4);
        *(f32*)(xf + 0x0) = x;
        f32 z = *(f32*)(bone + 0x2C) + lbl_803C6C20[2];
        *(f32*)(xf + 0x4) = y;
        *(f32*)(xf + 0x8) = z;
        SetMoveFlags(blade);
    }
}

// 0x800A232C
// Resets the boss: +0xCF4 timer to lbl_805FDE44, clears +0xC88, runs two virtual slots on the
// sub-object, notifies the blade (0x9B51AA64) and reloads the +0xCD0 timer from the table.
void fn_800A232C(u8* p) {
    *(f32*)(p + 0xCF4) = lbl_805FDE44;
    fn_800A75C8(*(u8**)(p + 0xC8C));
    *(s32*)(p + 0xC88) = 0;
    (*(VFnSelf*)(*(u8**)(p + 0x160) + 0x10))(p + 0x8C);
    (*(void (**)(u8*, u8*))(*(u8**)(p + 0x160) + 0xC))(p + 0x8C, p + 0xC74);
    fn_800A71C8(*(u8**)(p + 0xC8C), 0x9B51AA64u);
    fn_800A0BAC(p, lbl_805FDE40);
    f32 t = lbl_80346A58[RaySceneTest(NULL)];
    *(f32*)(p + 0xCD0) = t;
    fn_800712BC(p, (s32)t, 0);
}

// 0x800A29BC
// std::map-style node insert: allocates a 0x14-byte node under `parent` (left if `left` else right),
// copies the 8-byte value, bumps the count, rebalances via fn_800A2AEC and optionally records it as root.
u8* fn_800A29BC(u8* tree, u8* parent, s32 left, s32 setRoot, s32* value) {
    if ((u32)(*(s32*)tree + 0x10000) == 0xFFFFu) {
        fn_80336BF8(__files + 0xA0, "tree::insert length error");
        fn_8033B09C();
    }
    u8* node = (u8*)fn_801D40E0(0x14);
    if (node == NULL) {
        fn_80336BF8(__files + 0xA0, "Memory allocation failure");
        fn_8033B09C();
    }
    u8* guard = node;
    *(s32*)(node + 0xC) = value[0];
    *(s32*)(node + 0x10) = value[1];
    *(s32*)(node + 0x4) = 0;
    guard = NULL;
    *(s32*)(node + 0x0) = 0;
    *(u8**)(node + 0x8) = parent;
    if (left != 0) {
        *(u8**)(parent + 0x0) = node;
    } else {
        *(u8**)(parent + 0x4) = node;
    }
    s32 count = *(s32*)tree;
    *(s32*)tree = count + 1;
    fn_800A2AEC(node, *(u8**)(tree + 0x4));
    if (setRoot != 0) *(u8**)(tree + 0x8) = node;
    if (guard != NULL) operator_delete(guard);
    return node;
}

void fn_800A3188(u8* self, u8* node);

// 0x800A2E70
// Frees every node of the tree rooted at tree+8 (post-order, children first).
void fn_800A2E70(void* unused, u8* tree) {
    if (tree != NULL) {
        u8* root = *(u8**)(tree + 0x8);
        if (root != NULL) fn_800A3188(tree + 0x4, root);
    }
}

// 0x800A3188
// Recursively frees the subtree at `node`: left child, right child, then the node itself.
void fn_800A3188(u8* self, u8* node) {
    u8* l = *(u8**)(node + 0x0);
    if (l != NULL) fn_800A3188(self, l);
    u8* r = *(u8**)(node + 0x4);
    if (r != NULL) fn_800A3188(self, r);
    operator_delete(node);
}

}  // extern "C"
