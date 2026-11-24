// Missing functions batch 441: chest-smash/catch mini-game per-frame updates and related.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "game/character.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
#include "game/script_callbacks.h"

typedef void (*ObjVFn)(void* obj, s32 arg);         // virtual call taking one int argument
typedef void (*ObjVFn0)(void* obj);                 // virtual call without arguments
typedef void (*ObjVFnF)(void* obj, s32 a, f32 b);   // virtual call (int, float)
typedef void (*ObjVFnD)(void* obj, f32 a);          // virtual call taking a float
typedef void (*ObjVFnU)(void* obj, u32 arg);        // virtual call taking a hash
typedef u8* (*GetFn)(void* obj);                    // virtual getter returning a pointer

s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);        // 0x80209E70
int   SysSprintf(char* buf, const char* fmt, ...);                    // 0x80142EF8
u32   StringHash(const char* s);                                      // 0x801EB704
void  ScopedLockCtor(ScopedLock* l);                                  // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                       // 0x801F53B0
void* ChoiceDestroy(void* c, s32 del);                                // 0x8020F394
void  BattleAiPickAction(void* choice, u8* stats);                    // 0x802147E0
void  ReactionBuildRecover(u8* out, void* fighter);                   // 0x802159C4
void  LimbPlayReaction(void* limb, u8* reaction, BOOL special);       // 0x8004F398
void  CharacterStartReaction(void* ch);                               // 0x8004E3D0
void  CatchMinigame_UpdateBase(void* g, f32 dt);                      // 0x800224C4
void  EntitySyncTransform(void* entity, int a);                       // 0x801B4F4C
void  CharacterSetMoved(u8* c);                                       // 0x800533C8
void  CharacterSetRotated(u8* c);                                     // 0x800533EC
void  CharacterResetGroundState(u8* c, s32 a, s32 b);                 // 0x8005BAD4
u8*   CharacterGetGround(u8* c);                                      // 0x8005A520
void* PhysicsBodyGet(void* handle);                                   // 0x801B1840
f32   AnimGetLength(void* anim);                                      // 0x8018C07C
f32   AnimGetTime(void* anim);                                        // 0x8018BE78
void  ScalarSet(f32* out, f32 v);                                     // 0x80053250
void  Vec3Assign(Vec3* out, const Vec3* v);                           // 0x80053354
void  Vec3Copy(Vec3* out, const Vec3* v);                             // 0x80052640
void  Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);               // 0x80053370
void  Vec3AddTo(Vec3* v, const Vec3* d);                              // 0x800544FC
void  Vec3SubFrom(Vec3* v, const Vec3* d);                            // 0x8005461C
void  Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);          // 0x80053224
void  Vec3ScaleBy(Vec3* v, const f32* s);                             // 0x80054A3C
void  Vec3Dot(f32* out, const Vec3* a, const Vec3* b);                // 0x800549C0
void  Vec3SetZero(Vec3* out);                                         // 0x800531A8
void  Vec3SetZ(Vec3* v, f32 z);
void  Vec3SetUnitZ(Vec3* v);                                          // 0x800549A8
f32   Vec3LengthSqOf(const Vec3* v);                                  // 0x80054A90
Vec3  fn_80124610(const f32* a, const f32* b);                        // 0x80124610
void  fn_8001B128(void* hud);                                         // 0x8001B128
void  fn_8001B164(u8* hud);                                           // 0x8001B164
void  fn_80022E10(void);                                              // 0x80022E10
s32   fn_80031114(void* list, f32* v);                                // 0x80031114
void  fn_80039268(u8* c, f32 dt);                                     // 0x80039268
u32   fn_8003F858(void);                                              // 0x8003F858
void  fn_80042364(u8* c, void* choice, u8* fighter, s32 flag);        // 0x80042364
void  fn_80042408(u8* c, const s32* kind, s32 reaction, u8* fighter, s32 flag);   // 0x80042408
void  fn_8004504C(void);                                              // 0x8004504C
void  fn_8004784C(void);                                              // 0x8004784C
void  fn_80052638(void);                                           // 0x80052638
void  fn_8005265C(u8* obj);                                           // 0x8005265C
void  fn_800526D0(u8* out);                                           // 0x800526D0
u8*   fn_800526D4(u8* mtx);                                           // 0x800526D4
void  fn_800531BC(u8* c, s32 a);                                      // 0x800531BC
f32   fn_800531E4(u8* dst, u8* src);                                  // 0x800531E4
s32   fn_80053258(u8* obj);                                           // 0x80053258
u8*   fn_80053410(u8* c);                                             // 0x80053410
void* fn_80053BF0(u8* p);                                             // 0x80053BF0
u32   fn_80053ACC(u8* c, s32 mask);                                   // 0x80053ACC
void  fn_80054484(Vec3* out, const Vec3* a, const Vec3* b);           // 0x80054484
f32   fn_800544B8(f32* out, f32* a, f32* b);                          // 0x800544B8
void  fn_800544CC(f32* out);                                          // 0x800544CC
u32   fn_800544D8(u8* c, s32 bit);                                    // 0x800544D8
void  fn_80054530(f32* out, f32* a, f32* b);                          // 0x80054530
f64   fn_800545EC(u8* out, u8* src);                                  // 0x800545EC
void  fn_80054650(u8* out, u8* q, u8* v);                             // 0x80054650
void  fn_8005484C(f32* q);                                            // 0x8005484C
f32   fn_800548F4(u8* c);                                             // 0x800548F4
u32   fn_80054988(u8* flags, s32 mask);                               // 0x80054988
f32   fn_800549F4(u8* obj);                                           // 0x800549F4
void  fn_800549FC(Vec3* a, const Vec3* b);                            // 0x800549FC
u32   fn_80054A30(u8* c);                                             // 0x80054A30
u32   fn_80054A68(u8* c);                                             // 0x80054A68
u32   fn_80054A74(u8* c);                                             // 0x80054A74
u8    fn_80054A88(u8* c);                                             // 0x80054A88
void  fn_80054AB4(u8* obj, f32* a, f32* b, s32 c, s32 d);             // 0x80054AB4
void  fn_80054AC0(u8* obj, const Vec3* v);                            // 0x80054AC0
void  fn_80054B38(u8* flags, s32 mask);                               // 0x80054B38
void  fn_800596F0(void* obj, s32 on);                                 // 0x800596F0
void  fn_8005979C(void* obj, s32 on);                                 // 0x8005979C
void  fn_80059A88(u8* c);                                             // 0x80059A88
void  fn_80059EB0(u8* c);                                             // 0x80059EB0
void  fn_8005A178(u8* c);                                             // 0x8005A178
void  fn_8005CB40(u8* p, f32 t, s32 flag);                            // 0x8005CB40
void  fn_800E75E8(s32 a);                                             // 0x800E75E8
s32   fn_80147554(s32 button);                                        // 0x80147554
s32   fn_8016195C(void* p, s32 a, s32 b, void* c);                    // 0x8016195C
void  fn_8018A948(u8* obj, f32 t);                                    // 0x8018A948
f32   fn_8018BE30(void* obj, f32 a);                                  // 0x8018BE30
void  fn_801A8654(u8* obj, s32 a1);                                   // 0x801A8654
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                         // 0x801B55F4
void  fn_801B6F34(u8* p);                                             // 0x801B6F34
u8*   fn_801C3EA4(u8* table, s32 key);                                // 0x801C3EA4
void  fn_801C42A8(u8* obj);                                           // 0x801C42A8
void* fn_801D40E0(s32 n);                                             // 0x801D40E0
s32   fn_801ED1F8(u8* arg0, u32 key);                                 // 0x801ED1F8
void* fn_80209814(void* v);                                           // 0x80209814
s32   fn_80222994(u8* s);                                             // 0x80222994

extern const char lbl_80358000[];   // 0x80358000 ("Chest_%d_Region%d_TextureList")
extern u8 lbl_803C4298[];           // 0x803C4298
extern u8 lbl_8040E5F0[];           // 0x8040E5F0 (frame effects object)
extern u8 lbl_8047C7AC[];           // 0x8047C7AC
extern s32 lbl_805F8560;            // 0x805F8560
extern s32 lbl_805F8564;            // 0x805F8564
extern s32 lbl_805F8568;            // 0x805F8568
extern s32 lbl_805F856C;            // 0x805F856C
extern s32 lbl_805F8570;            // 0x805F8570
extern s32 lbl_805F8574;            // 0x805F8574
extern u8* lbl_805FB008;            // 0x805FB008
extern u8* lbl_805FB014;            // 0x805FB014 (battle camera object)
extern u8 lbl_805FB068;             // 0x805FB068
extern u8* lbl_805FBBE0;            // 0x805FBBE0 (save block pointer)
extern u8 gGame[];                  // 0x803B53A0
}
extern "C++" u8* gBattle;           // 0x805FBBB8
extern "C++" void* lbl_805FBBAC;    // 0x805FBBAC (voice SFX handle)

extern "C" {

// Creates the voice SFX handle lbl_805FBBAC once (inlined four times in the original).
static void EnsureVoiceSfxHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// 0x80035164
// Per-frame update of the falling items of one team (1, 2, 4 or 8; anything else is treated as 8).
// Each active item accelerates down, drops to the floor line, then either gets caught (near the
// catcher at +0x59C) or lands and is lost; the catch/miss sounds, rumble and UI images follow.
void fn_80035164(u8* c, s32 team, f32 dt) {
    f32 reach;       // per-team catch radius
    u8** slots;      // item UI objects
    s32* count;      // items still falling
    s32* missed;     // items lost
    u8* active;      // per-item active flag
    f32* vel;        // per-item velocity (3 floats each)
    u8* landed;      // per-item "reached the floor line" flag
    s32 base;        // index of the first shadow slot of this team

    switch (team) {
    case 1:
        reach = *(f32*)(c + 0x5D4);
        slots = (u8**)(c + 0x198);
        count = (s32*)(c + 0x250);
        missed = (s32*)(c + 0x254);
        active = c + 0x1C0;
        vel = (f32*)(c + 0x1CC);
        landed = c + 0x244;
        base = 0;
        break;
    case 2:
        reach = *(f32*)(c + 0x5D8);
        slots = (u8**)(c + 0x2A8);
        count = (s32*)(c + 0x360);
        missed = (s32*)(c + 0x364);
        active = c + 0x2D0;
        vel = (f32*)(c + 0x2DC);
        landed = c + 0x354;
        base = 10;
        break;
    case 4:
        reach = *(f32*)(c + 0x5DC);
        slots = (u8**)(c + 0x368);
        count = (s32*)(c + 0x420);
        missed = (s32*)(c + 0x424);
        active = c + 0x390;
        vel = (f32*)(c + 0x39C);
        landed = c + 0x414;
        base = 20;
        break;
    default:
        reach = *(f32*)(c + 0x5E0);
        slots = (u8**)(c + 0x428);
        count = (s32*)(c + 0x4E0);
        missed = (s32*)(c + 0x4E4);
        active = c + 0x450;
        vel = (f32*)(c + 0x45C);
        landed = c + 0x4D4;
        base = 30;
        break;
    }

    u8* catcher = *(u8**)(c + 0x59C);
    f32 savedY = *(f32*)(catcher + 0x6C);
    f32 gravity = *(f32*)(c + 0x188) * dt;
    *(f32*)(catcher + 0x6C) = 25.0f + savedY;
    f32 radius = *(f32*)(c + 0x5E4);

    if (*count > 0) {
        for (s32 i = 0; i < 10; i++) {
            if (active[i] == 0) continue;

            u8* item = slots[i];
            f32* pos = (f32*)(item + 0x68);
            u8** shadowSlot = (u8**)(c + 0x4E8) + base + i;
            f32* v = vel + i * 3;

            // Shadow follows the item's x and the floor y, scaled by the item's height.
            *(f32*)(*shadowSlot + 0x68) = pos[0];
            *(f32*)(*shadowSlot + 0x6C) = *(f32*)(c + 0x5C8);
            *(f32*)(*shadowSlot + 0x8C) = 0.9f;
            u8* shadow = *shadowSlot;
            f32 sc = (2.0f - (0.5f + 0.5f * ((120.0f + *(f32*)(item + 0x6C)) /
                                             (120.0f + *(f32*)(c + 0x5C8))))) *
                     *(f32*)(item + 0x74);
            *(f32*)(shadow + 0x74) = sc;
            *(f32*)(shadow + 0x78) = sc;
            *(f32*)(shadow + 0x7C) = 1.0f;

            f32 dx = *(f32*)(catcher + 0x68) - pos[0];
            f32 dy = *(f32*)(catcher + 0x6C) - pos[1];
            f32 dz = *(f32*)(catcher + 0x70) - pos[2];
            f32 d2 = dz * dz + (dx * dx + dy * dy);
            f32 dist = 0.0f;
            if (!(d2 <= 0.0f)) dist = sqrtf(d2);

            // Integrate: velocity gets the gravity step, position moves by dt * velocity.
            f32 vx = v[0] + 0.0f;
            f32 vy = v[1] + gravity;
            f32 vz = v[2] + 0.0f;
            v[0] = vx;
            v[1] = vy;
            v[2] = vz;
            pos[0] = pos[0] + dt * vx;
            pos[1] = pos[1] + dt * vy;
            pos[2] = pos[2] + dt * vz;

            if (pos[1] >= 512.0f) {
                // Fell off the bottom: retire the item.
                *(f32*)(*shadowSlot + 0x68) = -200.0f;
                active[i] = 0;
                landed[i] = 0;
                *count -= 1;
            } else if (pos[1] >= *(f32*)(c + 0x5C8) - 0.5f * reach && landed[i] == 0) {
                // Reached the floor line: bounce it out with the stored hop velocity.
                landed[i] = 1;
                v[0] = *(f32*)(c + 0x18C);
                v[1] = *(f32*)(c + 0x190);
                v[2] = *(f32*)(c + 0x194);
                pos[2] = *(f32*)(c + 0x5F0);
                EnsureVoiceSfxHandle();
                PlaySound(0x7170FBC4u, 0, 0, 0, 0, 0);
                u8* tex = NULL;
                switch (team) {
                case 1: tex = (u8*)UITextureList_GetTexture(0xACCED4C1u, 1); break;
                case 2: tex = (u8*)UITextureList_GetTexture(0xD2D850FCu, 1); break;
                case 4: tex = (u8*)UITextureList_GetTexture(0x3095961Fu, 1); break;
                case 8: tex = (u8*)UITextureList_GetTexture(0x2EFDE767u, 1); break;
                }
                if (slots[i] != NULL) {
                    UIImage_SetTexture((u32)(uintptr_t) * (void**)slots[i], (BOOL)(intptr_t)tex);  // lint:ok header takes u32
                    (*(ObjVFn*)(*(u8**)(slots[i] + 0x138) + 0x2C))(slots[i], 0x1A);
                    *(f32*)(c + i * 4 + 0x258) = 0.5f;
                    *(s32*)(c + i * 4 + 0x280) = team;
                }
            } else if (dist < 0.5f * (reach + radius) && landed[i] == 0) {
                // Close enough to the catcher.
                if (team == 8 && *(u8*)(c + 0x5AC) == 0) {
                    *(u8*)(c + 0x5AC) = 1;
                    *(f32*)(c + 0x5B4) = 0.0f;
                    (*(ObjVFn0*)(*(u8**)(c + 4) + 0x64))(c);
                    v[0] = *(f32*)(c + 0x18C);
                    v[1] = *(f32*)(c + 0x190);
                    v[2] = *(f32*)(c + 0x194);
                    if (*(f32*)(c + 0x598) <= 0.25f) v[0] = -*(f32*)(c + 0x18C);
                    landed[i] = 1;
                    EnsureVoiceSfxHandle();
                    PlaySound(0x7170FBC4u, 0, 0, 0, 0, 0);
                    TimedRumble(*(s32*)(lbl_803C4298 + 0x38), 0.2f, 4);
                    u8* cp = *(u8**)(c + 0x59C);
                    u8* fx = *(u8**)(c + 0x604);
                    f32 px = *(f32*)(cp + 0x68);
                    f32 py = *(f32*)(cp + 0x6C);
                    f32 pz = *(f32*)(fx + 0x70);
                    fn_801A8654(fx, 1);
                    fx = *(u8**)(c + 0x604);
                    *(f32*)(fx + 0x68) = px;
                    *(f32*)(fx + 0x6C) = py;
                    *(f32*)(fx + 0x70) = pz;
                    fx = *(u8**)(c + 0x604);
                    (*(ObjVFn*)(*(u8**)(fx + 0x138) + 0x2C))(fx, 0x1A);
                } else if (*(u8*)(c + 0x5AC) == 0) {
                    // Caught by the catcher: item is consumed.
                    active[i] = 0;
                    *count -= 1;
                    s32 n = *missed;
                    *missed = n + 1;
                    (*(ObjVFn*)(*(u8**)(c + 4) + 0x60))(c, n);
                    pos[1] = 480.0f;
                    TimedRumble(*(s32*)(lbl_803C4298 + 0x38), 0.2f, 4);
                    *(f32*)(*shadowSlot + 0x68) = -200.0f;
                    EnsureVoiceSfxHandle();
                    PlaySound(0x5842E875u, 0, 0, 0, 0, 0);
                    ScriptValue t = UITextureList_GetTexture(0x6C375453u, 1);
                    UIImage_SetTexture((u32)(uintptr_t) * (void**)(*(u8**)(c + 0x59C)),  // lint:ok header takes u32
                                       (BOOL)t);
                    u8* cp = *(u8**)(c + 0x59C);
                    (*(ObjVFn*)(*(u8**)(cp + 0x138) + 0x2C))(cp, 0x1A);
                    *(f32*)(c + 0x640) = 0.5f;
                } else {
                    v[0] = *(f32*)(c + 0x18C);
                    v[1] = *(f32*)(c + 0x190);
                    v[2] = *(f32*)(c + 0x194);
                    if (*(f32*)(c + 0x598) <= 0.0f) v[0] = -*(f32*)(c + 0x18C);
                    landed[i] = 1;
                    pos[2] = *(f32*)(c + 0x5F0);
                    EnsureVoiceSfxHandle();
                    PlaySound(0x7170FBC4u, 0, 0, 0, 0, 0);
                }
            }

            UIElement_SetPos((u32)(uintptr_t) * (void**)slots[i], pos);  // lint:ok header takes u32
            (*(ObjVFn*)(*(u8**)(*shadowSlot + 0x138) + 0x2C))(*shadowSlot, 0x28);
        }
    }
    *(f32*)(catcher + 0x6C) = savedY;
}

// 0x80038B0C
// Per-frame update of the chest-smash mini-game: hit-test the tap position (+0x690) against the
// four chest regions, pick the damage texture from the remaining-hit ratio, play the hit/miss
// sounds, place the hit marker (+0x98/+0xA0 UI objects) and finish via the base update.
void fn_80038B0C(u8* c, f32 dt) {
    f32 markerTimer = *(f32*)(c + 0x6E0);
    if (markerTimer > 0.0f) {
        *(f32*)(c + 0x6E0) = markerTimer - dt;
    } else {
        // Hide both marker objects.
        for (s32 i = 0; i < 2; i++) {
            u8* m = *(u8**)(c + 0x98 + i * 4);
            *(f32*)(m + 0x68) = -1000.0f;
            m = *(u8**)(c + 0x98 + i * 4);
            (*(ObjVFn*)(*(u8**)(m + 0x138) + 0x2C))(m, 0x18);
            m = *(u8**)(c + 0x98 + i * 4);
            (*(ObjVFn*)(*(u8**)(m + 0x138) + 0x30))(m, 2);
        }
    }

    if (*(f32*)(c + 0x6C) <= 0.0f) {
        GameFlowSignal(0x117741C3, 0, 0);
    } else if (*(s32*)(c + 0x5E4) == 0) {
        // All chests broken: hide the HUD, wait out the counter at +0x6B4, then signal the end.
        UIElement_SetVisibleFlag(0x9D841CAAu, 0);
        UIElement_SetVisibleFlag(0x0CAF9C6Fu, 0);
        u8* m = *(u8**)(c + *(s32*)(c + 0x94) * 4 + 0x98);
        if (m != NULL) (*(ObjVFn*)(*(u8**)(m + 0x138) + 0x30))(m, 2);
        m = *(u8**)(c + 0xA0);
        if (m != NULL) (*(ObjVFn*)(*(u8**)(m + 0x138) + 0x30))(m, 2);
        s32 wait = *(s32*)(c + 0x6B4);
        if (wait > 0) {
            *(s32*)(c + 0x6B4) = wait - 1;
        } else {
            GameFlowSignal((s32)0xC608C842u, 0, 0);
        }
        return;
    }

    fn_80039268(c, dt);
    CatchMinigame_UpdateBase(c, dt);

    if (*(u8*)(c + 0x68C) != 0) {
        *(u8*)(c + 0x68C) = 0;

        // Find which of the four regions contains the tap.
        s32 region = 0;
        s32 off = 0;
        for (;;) {
            if (fn_80031114(lbl_805FB008 + off + 0x5EC, (f32*)(c + 0x690)) != 0) break;
            region++;
            off += 0x28;
            if (region >= 4) {
                region = -1;
                break;
            }
        }

        f32 nearest = 409600.0f;   // squared distance to the closest live hit spot
        s32 hit = 0;
        s32 hitBroken = 0;         // a spot was used up
        s32 chestBroken = 0;       // a whole chest ran out of hits
        *(s32*)(c + 0x6D8) = *(s32*)(c + 0x6D8) + 1;

        if (region > -1 && region < 4) {
            f32 quality = 0.1f;
            *(s32*)(c + 0x94) = 1;

            for (s32 r = 0; r < 4 && hit == 0; r++) {
                u8* chest = c + r * 0x28;
                s32 n = *(s32*)(chest + 0x5FC);
                for (s32 j = 0; j < n; j++) {
                    u8* spot = *(u8**)(chest + 0x5F8) + j * 0x10;
                    if (!(*(s32*)(spot + 0xC) > 0)) continue;
                    f32 dx = *(f32*)(c + 0x690) - *(f32*)(spot + 0x0);
                    f32 dy = *(f32*)(c + 0x694) - *(f32*)(spot + 0x4);
                    f32 d2 = 0.0f * 0.0f + (dx * dx + dy * dy);
                    if (d2 < *(f32*)(c + 0x6AC)) {
                        if (*(s32*)(spot + 0xC) > 0) *(s32*)(spot + 0xC) = *(s32*)(spot + 0xC) - 1;
                        if (!(*(s32*)(*(u8**)(chest + 0x5F8) + j * 0x10 + 0xC) > 0)) {
                            hitBroken = 1;
                            *(s32*)(chest + 0x600) = *(s32*)(chest + 0x600) - 1;
                            *(s32*)(c + 0x5E4) = *(s32*)(c + 0x5E4) - 1;
                        }
                        // Texture level from the remaining-hit ratio of this chest.
                        s32 level = -1;
                        f32 ratio = (f32)*(s32*)(chest + 0x600) / (f32)*(s32*)(chest + 0x5FC);
                        if (ratio <= 0.0f) level = 3;
                        else if (ratio < 0.33f) level = 2;
                        else if (ratio < 0.66f) level = 1;
                        if (level > 0) {
                            char name[0x40];
                            SysSprintf(name, lbl_80358000, *(s32*)(c + 0x5E8), r);
                            ScriptValue t = UITextureList_GetTexture(StringHash(name), level);
                            UIImage_SetTexture((u32)(uintptr_t) * (void**)(*(u8**)(c + r * 4 + 0x69C)),  // lint:ok header takes u32
                                               (BOOL)t);
                        }
                        hit = 1;
                        if (*(s32*)(chest + 0x600) == 0) chestBroken = 1;
                        *(s32*)(c + 0x94) = 0;
                        quality = 1.0f;
                        break;
                    }
                    if (nearest > d2) nearest = d2;
                }
            }

            if (hit != 0) {
                *(s32*)(c + 0x6D8) = 0;
                if (chestBroken != 0) {
                    if (*(s32*)(c + 0x5E4) == 0) {
                        PlaySound(0xC70D3D29u, 0, 0, 0, 0, 0);
                    } else {
                        PlaySound(0x18BB4DA3u, 0, 0, 0, 0, 0);
                    }
                    fn_80022E10();
                } else {
                    PlaySound(0xD8B6A366u, 0, 0, 0, 0, 0);
                    if (hitBroken != 0) PlaySound(0xE62C62CEu, 0, 0, 0, 0, 0);
                }
            } else {
                // Miss: marker size depends on how close the tap was to a live spot.
                f32 dist = 0.0f;
                if (!(nearest <= 0.0f)) dist = sqrtf(nearest);
                f32 t = dist / 20.0f / 10.0f;
                f32 clamped;
                if (t < 0.0f) clamped = 0.0f;
                else if (t > 1.0f) clamped = 1.0f;
                else clamped = t;
                quality = 1.0f - clamped;
                if (!(quality > 0.1f)) quality = 0.1f;
                PlaySound(0x4CEF2D0Du, 0, 0, 0, 0, 0);
            }

            TimedRumble(*(s32*)(lbl_803C4298 + 0x38), 0.1f, 1);

            // Place and size the marker object for this tap.
            u8* m = *(u8**)(c + *(s32*)(c + 0x94) * 4 + 0x98);
            *(f32*)(m + 0x68) = *(f32*)(c + 0x690);
            f32 size = 16.0f * (1.0f - quality) + 128.0f * quality;
            m = *(u8**)(c + *(s32*)(c + 0x94) * 4 + 0x98);
            *(f32*)(m + 0x6C) = *(f32*)(c + 0x694);
            m = *(u8**)(c + *(s32*)(c + 0x94) * 4 + 0x98);
            *(f32*)(m + 0x74) = size;
            m = *(u8**)(c + *(s32*)(c + 0x94) * 4 + 0x98);
            *(f32*)(m + 0x78) = size;
            m = *(u8**)(c + *(s32*)(c + 0x94) * 4 + 0x98);
            (*(ObjVFn*)(*(u8**)(m + 0x138) + 0x2C))(m, 0x1A);
            *(f32*)(c + 0x6E0) = 0.1f;

            if (*(s32*)(c + 0x5E4) == 0) {
                u8* last = *(u8**)(c + 0xA0);
                *(f32*)(last + 0x74) = 2.0f * *(f32*)(last + 0x74);
                *(f32*)(last + 0x78) = 2.0f * *(f32*)(last + 0x78);
            }

            if (*(s32*)(c + 0x94) == 0) {
                u8* other = *(u8**)(c + 0xA0);
                *(f32*)(other + 0x68) = *(f32*)(*(u8**)(c + 0x98) + 0x68);
                other = *(u8**)(c + 0xA0);
                *(f32*)(other + 0x6C) = *(f32*)(*(u8**)(c + 0x98) + 0x6C);
                other = *(u8**)(c + 0xA0);
                (*(ObjVFn*)(*(u8**)(other + 0x138) + 0x2C))(other, 0x1A);
                fn_801A8654(*(u8**)(c + 0xA0), 1);
            }
        }
    }

    (*(ObjVFn0*)(*(u8**)(c + 4) + 0x64))(c);
    (*(ObjVFn0*)(*(u8**)(c + 4) + 0x6C))(c);
}

// Sets / clears bits in the flag word at +0x20 of the node (+4) of a character part.
static void PartSetFlags(u8* part, u32 mask) {
    u8* node = *(u8**)(part + 4);
    *(u32*)(node + 0x20) = *(u32*)(node + 0x20) | mask;
}
static void PartClearFlags(u8* part, u32 mask) {
    u8* node = *(u8**)(part + 4);
    *(u32*)(node + 0x20) = *(u32*)(node + 0x20) & ~mask;
}

void EntityTick(void* e, f32 dt);   // world_update.cpp

// 0x8004BE38
// Per-frame update of a multi-part character (parts at +0xC50..+0xC60; only when the node flag
// 0x200000 is set): ticks the timer at +0xC88, shows/hides the parts, runs the update virtuals,
// mirrors the node transform onto the active part, and eases the walk/run speed at +0x9F4 towards
// the pad-driven target, starting the "idlereg" / "explore1" animations when it reaches 0 / leaves 0.
void fn_8004BE38(u8* c, s32 team, f32 dt) {
    typedef u8* (*GetFn)(void*);
    typedef u8** (*GetTableFn)(void*);
    typedef void (*BodyUpdateFn)(void*, f32);
    typedef void (*TickFn)(void*, s32, f32);

    if (!(*(u32*)(*(u8**)(c + 4) + 0x20) & 0x200000)) return;

    f32 timer = *(f32*)(c + 0xC88);
    if (timer > 0.0f) {
        timer = timer - dt;
        *(f32*)(c + 0xC88) = timer;
        if (!(timer >= 0.0f)) *(f32*)(c + 0xC88) = 0.0f;
    }
    {
        u8* node = *(u8**)(c + 4);
        *(u32*)(node + 0x20) = *(u32*)(node + 0x20) | 0x4000;
    }
    if ((*(GetFn*)(*(u8**)c + 0xC))(c) != NULL) {
        u8* p = (*(GetFn*)(*(u8**)c + 0xC))(c);
        *(u32*)(p + 8) = *(u32*)(p + 8) & ~2u;
    }

    if (*(u8*)(c + 0xC83) != 0) {
        u32 bit = (*(u32*)(*(u8**)(c + 4) + 0x20) >> 1) & 1;
        for (s32 i = 0; i < 5; i++) {
            u8* part = *(u8**)(c + 0xC50 + i * 4);
            if (part == NULL) continue;
            if (bit != 0) {
                if ((*(GetFn*)(*(u8**)part + 0xC))(part) != NULL) {
                    u8* p = (*(GetFn*)(*(u8**)part + 0xC))(part);
                    *(u32*)(p + 8) = *(u32*)(p + 8) | 2;
                }
                PartSetFlags(part, 2);
            } else {
                if ((*(GetFn*)(*(u8**)part + 0xC))(part) != NULL) {
                    u8* p = (*(GetFn*)(*(u8**)part + 0xC))(part);
                    *(u32*)(p + 8) = *(u32*)(p + 8) & ~2u;
                }
                PartClearFlags(part, 2);
            }
        }
    }

    for (s32 i = 0; i < 5; i++) {
        u8* part = *(u8**)(c + 0xC50 + i * 4);
        if (part != NULL) PartSetFlags(part, 1);
    }

    // Base body update, vtable slot 0xF8 (fn_8004B56C / fn_8004CB18, tail calls into fn_8005DC04 /
    // fn_80053C2C): the PowerPC passes (this, r4 = team, f1 = dt) and the targets read only this and f1.
    // Arguments are positional here, so the call is made with the targets' (this, dt) signature;
    // with (this, team, dt) they got dt = 0: no turning towards the walk direction, no animation step.
    (void)team;
    (*(BodyUpdateFn*)(*(u8**)c + 0xF8))(c, dt);

    // Active part: slot 1 if present, else slot 4.
    s32 idx = 4;
    if (*(u8**)(c + 0xC54) != NULL) idx = 1;

    // `busy`: the entry idx of the table returned by virtual +0xF0 has a nonzero field behind
    // two +0x28 hops.
    s32 busy = 0;
    if ((*(GetTableFn*)(*(u8**)c + 0xF0))(c) != NULL) {
        u8** tbl = (*(GetTableFn*)(*(u8**)c + 0xF0))(c);
        if (tbl[idx] != NULL) {
            tbl = (*(GetTableFn*)(*(u8**)c + 0xF0))(c);
            if (tbl[idx] + 0x28 != NULL) {
                tbl = (*(GetTableFn*)(*(u8**)c + 0xF0))(c);
                busy = *(s32*)(*(u8**)(tbl[idx] + 0x28) + 0x28) != 0;
            }
        }
    }

    u8* active = *(u8**)(c + 0xC50 + idx * 4);
    if (active != NULL && busy == 0) {
        // Copy the node position (3 floats) and orientation (4 floats) onto the active part.
        u8* src = *(u8**)(c + 4);
        u8* dst = *(u8**)(active + 4);
        *(f32*)(dst + 0x0) = *(f32*)(src + 0x0);
        *(f32*)(dst + 0x4) = *(f32*)(src + 0x4);
        *(f32*)(dst + 0x8) = *(f32*)(src + 0x8);
        PartSetFlags(active, 0x8000);
        PartSetFlags(active, 0x10000000);
        PartSetFlags(active, 0x4000);
        PartSetFlags(active, 0x10000000);
        src = *(u8**)(c + 4);
        active = *(u8**)(c + 0xC50 + idx * 4);
        dst = *(u8**)(active + 4);
        *(f32*)(dst + 0xC) = *(f32*)(src + 0xC);
        *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
        *(f32*)(dst + 0x14) = *(f32*)(src + 0x14);
        *(f32*)(dst + 0x18) = *(f32*)(src + 0x18);
        PartSetFlags(active, 0x8000);
        PartSetFlags(active, 0x10000000);
        PartSetFlags(active, 0x4000);
        PartSetFlags(active, 0x10000000);
    }

    // Tick every part whose virtual +0xCC result has a zero field at +8, then sync it.
    for (s32 i = 0; i < 5; i++) {
        u8* part = *(u8**)(c + 0xC50 + i * 4);
        if (part == NULL) continue;
        u8* st = (*(GetFn*)(*(u8**)part + 0xCC))(part);
        if (*(s32*)(st + 8) != 0) continue;
        f32 step = dt;
        if (*(u8*)(c + 0xC82) == 0) step = 0.0f;
        part = *(u8**)(c + 0xC50 + i * 4);
        EntityTick(part, step);
        EntitySyncTransform(*(u8**)(c + 0xC50 + i * 4), 0);
    }

    for (s32 i = 0; i < 5; i++) {
        u8* part = *(u8**)(c + 0xC50 + i * 4);
        if (part != NULL) PartSetFlags(part, 0x8000);
    }
    if (*(u8**)(c + 0xC50 + idx * 4) != NULL) EntitySyncTransform(*(u8**)(c + 0xC50 + idx * 4), 0);
    for (s32 i = 0; i < 5; i++) {
        u8* part = *(u8**)(c + 0xC50 + i * 4);
        if (part != NULL) PartClearFlags(part, 1);
    }

    // Speed target from the pad-driven object at +0xC64 / +0xC68.
    f32 speed = 0.0f;
    u8* pad = *(u8**)(c + 0xC64);
    if (pad != NULL && *(u8*)(c + 0xC81) != 0) {
        speed = *(f32*)(c + 0x9F4);
        s32 phase = *(s32*)(**(u8***)(c + 0xA4) + 0x2C);
        bool done = false;
        if (phase == 0) {
            if (*(u8*)(pad + 0x90) != 0) {
                f32 v = *(f32*)(pad + 0x94);
                if (v > 0.0f) *(f32*)(pad + 0x8) = v;
            }
            if (*(u8*)(pad + 0x90) != 0) {
                *(s32*)(**(u8***)(c + 0xA4) + 0x2C) = 1;
                done = true;
            }
        }
        if (!done && *(s32*)(**(u8***)(c + 0xA4) + 0x2C) == 1) {
            u8* pad2 = *(u8**)(c + 0xC68);
            if (*(u8*)(pad2 + 0x90) != 0) {
                f32 v = *(f32*)(pad2 + 0x94);
                if (v > 0.0f) *(f32*)(pad2 + 0x8) = v;
            }
            if (*(u8*)(pad2 + 0x90) != 0) {
                speed = speed + 4.5f;
                *(s32*)(**(u8***)(c + 0xA4) + 0x2C) = 0;
                *(f32*)(c + 0x9E4) = *(f32*)(c + 0x9E4) + 0.5f;
            }
        }
    }

    if (*(u8*)(c + 0xC81) == 0) return;

    speed = speed - 5.2f * dt;
    if (speed < 0.0f) speed = 0.0f;
    else if (speed > 11.0f) speed = 11.0f;

    f32 oldSpeed = *(f32*)(c + 0x9F4);
    if (oldSpeed > 0.0f && speed == 0.0f) {
        PlayCharacterAnimation((Character*)c, 1, 0xE3712F44u, 1);   // "idlereg"
    } else if (*(f32*)(c + 0x9F4) == 0.0f && speed > 0.0f) {
        PlayCharacterAnimation((Character*)c, 1, 0xF5770040u, 1);   // "explore1"
    }

    if (speed > 0.0f) {
        f32 rate = speed / 6.0f;
        for (s32 i = 0; i < 5; i++) {
            u8* part = *(u8**)(c + 0xC50 + i * 4);
            if (part == NULL) continue;
            u8* anim = (*(GetFn*)(*(u8**)part + 0x10))(part);
            *(f32*)(anim + 0xB0) = rate;
        }
    }
    *(f32*)(c + 0x9F4) = speed;
    PathSetSpeed(*(PathFollower**)(c + 0xA00), speed);
}

// 0x80053C2C
// Per-frame update of a character body (only while its base flag bit 1 is set): follows the
// ground it stands on (carries position / orientation along with the moving ground), builds the
// movement vector at +0x810 from the animation root motion, runs the move/collision virtuals,
// spawns the dust-trail effects when moving fast, and advances the queued-action counter.
void fn_80053C2C(u8* c, f32 dt) {
    typedef u8* (*GetFn)(void*);

    if (*(s32*)(c + 0x9FC) != 0) {
        s32 flag = 0;
        if (fn_801ED1F8(c + 0x94, 0x9AD02496u) != 0) flag = 1;
        if (fn_80222994(lbl_805FBBE0) != -1 && flag != 0) {
            fn_8005979C(c, 1);
        } else if (fn_80222994(lbl_805FBBE0) == -1 && flag == 0) {
            fn_800596F0(c, 0);
        }
    }

    if (fn_80053ACC(c, 1) == 0) return;

    if (fn_80053ACC(c, 0x04000000) == 0) {
        // Smooth the node's velocity (+0x770) from its displacement since the last frame (+0x74C).
        f32 ten[1], tenth[1], one[1], nine[1];
        Vec3 delta, scaled, sum, vel;
        u8* node = *(u8**)(c + 4);
        ScalarSet(ten, 10.0f);
        ScalarSet(tenth, 0.1f);
        Vec3Sub(&delta, (Vec3*)node, (Vec3*)(c + 0x74C));
        fn_800544CC(one);
        fn_800544B8(nine, ten, one);
        Vec3MulScalar(&scaled, (Vec3*)(c + 0x770), nine);
        fn_80054484(&sum, &scaled, &delta);
        Vec3MulScalar(&vel, &sum, tenth);
        Vec3Copy((Vec3*)(c + 0x770), &vel);
        Vec3Copy((Vec3*)(c + 0x74C), (Vec3*)*(u8**)(c + 4));

        u8* ground = CharacterGetGround(c);
        if (ground != NULL && fn_800544D8(ground, 6) != 0) {
            if (!(*(s32*)(ground + 0x15C) & 0x10000)) fn_801B6F34(CharacterGetGround(c));
            u8* anchor = *(u8**)(c + 0x7B4);
            if (anchor != NULL) {
                // Carry the character along with the moving ground.
                Vec3 move;
                f32 invRot[4], rot[4], newRot[4];
                Vec3 newPos;
                Vec3Sub(&move, (Vec3*)(anchor + 0x14), (Vec3*)(c + 0x7B8));
                Vec3Copy((Vec3*)(c + 0x7B8), (Vec3*)(*(u8**)(c + 0x7B4) + 0x14));
                Vec3AddTo(*(Vec3**)(c + 4), &move);
                void* body = PhysicsBodyGet(*(void**)fn_80053410(c));
                if (body != NULL) Vec3AddTo((Vec3*)((u8*)body + 0x14), &move);
                fn_800545EC((u8*)invRot, c + 0x7C4);
                // fn_800533A4(ground) = ground node orientation (node + 0xC), fn_800526DC = node
                fn_80054530(rot, (f32*)(*(u8**)(ground + 4) + 0xC), invRot);
                Vec3SubFrom(*(Vec3**)(c + 4), (Vec3*)*(u8**)(ground + 4));
                fn_80054650((u8*)&newPos, (u8*)rot, *(u8**)(c + 4));
                Vec3Copy(*(Vec3**)(c + 4), &newPos);
                Vec3AddTo(*(Vec3**)(c + 4), (Vec3*)*(u8**)(ground + 4));
                fn_80054530(newRot, (f32*)(*(u8**)(c + 4) + 0xC), rot);
                fn_800531E4(*(u8**)(c + 4) + 0xC, (u8*)newRot);
                fn_8005484C((f32*)(*(u8**)(c + 4) + 0xC));
                fn_800531E4(c + 0x7C4, *(u8**)(ground + 4) + 0xC);
                CharacterSetMoved(c);
                CharacterSetRotated(c);
            }
        }
    }

    // Time step scaled by the animation speed factors.
    f32 step = dt * (*(f32*)(*(u8**)(c + 0x9C) + 0xB8) * fn_800548F4(c));
    if (fn_80053258(c) == 0) {
        f32 hold = *(f32*)(c + 0x16C);
        if (hold > 0.0f) {
            *(f32*)(c + 0x16C) = hold - step;
            step = 0.0f;
        }
    }
    *(f32*)(c + 0x24) = step;
    u8* def = *(u8**)(c + 0x9C);
    fn_8018A948(*(u8**)(c + 0xA0), step);

    if (fn_80053ACC(c, 0x04000000) == 0) {
        // Build the root-motion movement vector at +0x810.
        Vec3 v, zero1, zero2;
        v = *(Vec3*)(*(u8**)(c + 0xA0) + 0xC0);
        if (fn_80054988(c + 0x838, 0x10000) != 0) {
            Vec3SetZero(&zero1);
            Vec3Copy(&v, &zero1);
        } else {
            Vec3SetZ(&v, 0.0f);
        }
        Vec3SetZero(&zero2);
        Vec3Copy((Vec3*)(c + 0x810), &zero2);
        if (fn_80054988(c + 0x838, 4) != 0) {
            Vec3 axisA, axisB, proj, side, sum;
            f32 dot[1], sideScale[1];
            Vec3SetUnitZ(&axisA);
            Vec3Dot(dot, &v, &axisA);
            Vec3SetUnitZ(&axisB);
            Vec3MulScalar(&proj, &axisB, dot);
            Vec3Copy(&v, &proj);
            ScalarSet(sideScale, fn_800549F4(c));
            Vec3MulScalar(&side, (Vec3*)(c + 0x63C), sideScale);
            fn_80054484(&sum, &v, &side);
            Vec3Copy(&v, &sum);
        }
        Vec3Copy((Vec3*)(c + 0x810), &v);
        Vec3 rotated;
        fn_80054650((u8*)&rotated, *(u8**)(c + 4) + 0xC, c + 0x810);
        Vec3Copy((Vec3*)(c + 0x810), &rotated);
        fn_800549FC((Vec3*)(c + 0x810), (const Vec3*)(c + 8));
        if (fn_80054A30(c) != 0) {
            Vec3 adj = fn_80124610((const f32*)(c + 0x7A4), (const f32*)(c + 0x810));
            Vec3Copy((Vec3*)(c + 0x810), &adj);
        }
        f32 scale[1];
        ScalarSet(scale, *(f32*)(c + 0x698));
        Vec3ScaleBy((Vec3*)(c + 0x810), scale);
        (*(ObjVFn0*)(*(u8**)c + 0xC4))(c);
        fn_80059A88(c);
        fn_8005A178(c);
        fn_80059EB0(c);
        if (fn_80053ACC(c, 1) != 0) {
            if (fn_80054A68(c) != 0) {
                CharacterResetGroundState(c, *(s32*)(c + 0x790), 0);
                fn_800531BC(c, 0);
            }
            (*(ObjVFnD*)(*(u8**)c + 0xCC))(c, step);
        }
    }

    if (fn_80054A74(c + 0x170) != 0) {
        fn_8001B128(c + 0x170);
    } else if (fn_80054A88(c + 0x170) != 0) {
        fn_8001B164(c + 0x170);
    }
    fn_8005CB40(c, step, 0);

    // Squared speed this frame (per second); fast movement or flag 0x10 spawns the trail effects.
    Vec3 moved;
    Vec3Sub(&moved, (Vec3*)*(u8**)(c + 4), (Vec3*)(c + 0x74C));
    f32 speedSq = 0.0f;
    if (step != 0.0f) speedSq = Vec3LengthSqOf(&moved) / (step * step);

    if (speedSq > 169.0f || fn_80053ACC(c, 0x10) != 0) {
        u8 mtx[0x30];
        Vec3 p0, p1;
        fn_800526D0(mtx);
        fn_80052638();
        fn_80052638();
        s32 n = *(s32*)(def + 0x114);
        for (s32 i = 0; i < n; i++) {
            u8** slot = (u8**)(c + 0x81C + i * 4);
            if (*slot == NULL) {
                if (fn_80053ACC(c, 0x10) != 0) {
                    if ((u32)(intptr_t)fn_80053BF0(**(u8***)(c + 0xA4)) == 0xF9032909u ||
                        (u32)(intptr_t)fn_80053BF0(**(u8***)(c + 0xA4)) == 0xF9561854u) {
                        s32 key = *(s32*)(c + 0x82C);
                        if (key != 0) {
                            *slot = fn_801C3EA4(lbl_8047C7AC, key);
                        } else {
                            *slot = fn_801C3EA4(lbl_8047C7AC, *(s32*)(def + i * 0x14 + 0xD4));
                        }
                    }
                } else if (speedSq > 169.0f) {
                    *slot = fn_801C3EA4(lbl_8047C7AC, (s32)0x9AE3C66Cu);
                }
                if (*slot != NULL) fn_801C42A8(*slot);
            }
        }
        for (s32 i = 0; i < *(s32*)(def + 0x114); i++) {
            u8* fx = *(u8**)(c + 0x81C + i * 4);
            if (fx != NULL) {
                fn_801B55F4(c, mtx, *(s32*)(def + i * 0x14 + 0xC8));
                Vec3Copy(&p0, (Vec3*)fn_800526D4(mtx));
                fn_801B55F4(c, mtx, *(s32*)(def + i * 0x14 + 0xD0));
                Vec3Copy(&p1, (Vec3*)fn_800526D4(mtx));
                fn_80054AB4(fx, (f32*)&p0, (f32*)&p1, 0, 0);
            }
        }
    }

    f32 cool = *(f32*)(c + 0x834) - step;
    *(f32*)(c + 0x834) = cool;
    if (!(cool >= 0.0f)) *(f32*)(c + 0x834) = 0.0f;

    // Queued actions: when the current animation reaches its end, run the next queued one.
    f32 len = AnimGetLength((*(GetFn*)(*(u8**)c + 0x10))(c));
    f32 frac = AnimGetTime((*(GetFn*)(*(u8**)c + 0x10))(c)) / len;
    s32 queued = *(s32*)(c + 0x5E8);
    if (queued > 0 && frac >= 1.0f) {
        s32 qi = *(s32*)(c + 0x5F0);
        *(s32*)(c + 0x5E8) = queued - 1;
        (*(ObjVFn*)(*(u8**)c + 0x80))(c, *(s32*)(c + qi * 4 + 0x5A8));
        s32 next = *(s32*)(c + 0x5F0) + 1;
        *(s32*)(c + 0x5F0) = next;
        if (next >= 0x10) *(s32*)(c + 0x5F0) = 0;
    }

    fn_8005265C(c);
    if (*(u8**)(c + 0x15C) != NULL) {
        Vec3 off;
        fn_80054484(&off, (const Vec3*)*(u8**)(c + 4), (const Vec3*)(c + 0x160));
        fn_80054AC0(*(u8**)(c + 0x15C), &off);
    }
    fn_80054B38(c + 0x838, 0x10);
}

// Subobject at +0x1C of a fighter back to its character base (the subobject sits 0xC48 into it).
static u8* FighterBase(u8* fighter) {
    u8* sub = *(u8**)(fighter + 0x1C);
    if (sub != NULL) sub -= 0xC48;
    return sub;
}

// Calls virtual `slot` of obj with argument 1 while holding a scoped lock.
static void LockedVirtual1(u8* obj, s32 slot) {
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(ObjVFn*)(*(u8**)obj + slot))(obj, 1);
    ScopedLockDtor(&lock, -1);
}

// 0x80041B74
// Per-frame update of the battle intro: both fighters play their recover reaction, the intro
// camera animation (+0x18) runs, then a sequence of AI-chosen intro actions and camera cuts plays
// for each side (state at +0x10). Pressing the skip button during the intro jumps both fighters
// straight to their final pose. Returns 1 when the intro is finished.
s32 fn_80041B74(u8* c, f32 dt) {
    s32 choiceA[3];
    s32 choiceB[3];
    s32 result = 0;

    fn_8004784C();
    lbl_805FB068 = 0;

    if (UIScreen_Is_Pushed(0x3AB51700u) != 0 &&
        fn_80147554(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0x1089) != 0) {
        // Skip button: finish both fighters' intro immediately.
        GameFlowSignal((s32)0xF15918A6u, 0, 0);
        SendNISSignal(0xF15918A6u);
        *(s32*)(lbl_805FB014 + 0x4F4) = 1;
        *(u8*)(gGame + 0x24) = 0;

        u8* fighter = *(u8**)(gBattle + 0xA00);
        u8* base = FighterBase(fighter);
        LockedVirtual1(base, 0x14);
        base = FighterBase(fighter);
        (*(ObjVFnF*)(*(u8**)base + 0x20))(base, 0, 0.0f);
        fn_80042408(c, &lbl_805F8560, 0x30, fighter, 1);

        fighter = *(u8**)(gBattle + 0xA04);
        base = FighterBase(fighter);
        LockedVirtual1(base, 0x14);
        base = FighterBase(fighter);
        (*(ObjVFnF*)(*(u8**)base + 0x20))(base, 0, 0.0f);
        fn_80042408(c, &lbl_805F8564, 0x30, fighter, 1);

        u32 camAnim = fn_8003F858();
        u8* cam = *(u8**)(c + 0x1C);
        (*(ObjVFnU*)(*(u8**)cam + 0x84))(cam, camAnim);
        fn_8004504C();
        return 1;
    }

    f32 t = *(f32*)(c + 4) + dt;
    *(f32*)(c + 4) = t;
    if (*(u8*)(c + 0xB) == 0 && t >= 0.0) {
        // Start of the intro: both fighters play the recover reaction on all five limbs.
        for (s32 side = 0; side < 2; side++) {
            u8 recover[5 * 0x100];
            u8* fighter = *(u8**)(gBattle + 0xA00 + side * 4);
            ReactionBuildRecover(recover, fighter);
            for (s32 i = 0; i < 5; i++) {
                u8* limb = *(u8**)(fighter + i * 4);
                if (limb != NULL) {
                    u8* part = *(u8**)(limb + 0x20);
                    if (part != NULL) part -= 0x264;
                    LimbPlayReaction(part, recover + i * 0x100, 1);
                }
            }
            CharacterStartReaction(FighterBase(fighter));
        }
        u8* cam = *(u8**)(lbl_805FB014 + 0x4D4);
        *(u8**)(c + 0x1C) = cam;
        *(u8**)(c + 0x18) = (*(GetFn*)(*(u8**)cam + 0x10))(cam);
        CIAnimatedPropEnableByID(*(u32*)(*(u8**)(c + 0x1C) + 0x2C), 0x36013FB2u);
        u8* hud = *(u8**)(lbl_803C4298 + 4);
        (*(ObjVFnD*)(*(u8**)hud + 0x98))(hud, 0.0f);
        PlaySound(0xB9B09F83u, 0, 0, 0, 0, 0);
        *(u8*)(c + 0xB) = 1;
    }

    if (*(u8*)(c + 0xB) != 0) {
        if (fn_8018BE30(*(u8**)(c + 0x18), 0.0f) >= 1.0f) *(u8*)(c + 0xC) = 1;
    }

    if (!(*(f32*)(c + 4) >= 0.0f)) return 0;
    if (*(u8*)(c + 0xC) == 0) return 0;

    u8* fighterA = *(u8**)(gBattle + 0xA00);
    u8* fighterB = *(u8**)(gBattle + 0xA04);
    BattleAiPickAction(choiceA, fighterA + 0x20);
    BattleAiPickAction(choiceB, fighterB + 0x20);

    // Part (limb object) each side's AI picked.
    u8* pickA = *(u8**)(fighterA + choiceA[2] * 4);
    u8* partA = (pickA != NULL) ? *(u8**)(pickA + 0x20) : NULL;
    if (partA != NULL) partA -= 0x264;
    u8* pickB = *(u8**)(fighterB + choiceB[2] * 4);
    u8* partB = (pickB != NULL) ? *(u8**)(pickB + 0x20) : NULL;
    if (partB != NULL) partB -= 0x264;

    if (*(s32*)(c + 0x10) == 0) *(u8**)(c + 0x14) = (*(GetFn*)(*(u8**)partA + 0x10))(partA);
    if (*(u8*)(c + 0xA) == 0) {
        *(u8*)(gGame + 0x24) = 1;
        *(u8*)(c + 0xA) = 1;
    }

    s32 state = *(s32*)(c + 0x10);
    if (state == 3 || state == 7 || fn_8018BE30(*(u8**)(c + 0x14), 0.1f) >= 1.0f) {
        *(s32*)(c + 0x10) = state + 1;
    } else if (*(u8*)(c + 9) != 0 && state < 8) {
        ChoiceDestroy(choiceB, -1);
        ChoiceDestroy(choiceA, -1);
        return 0;
    }

    switch (*(u32*)(c + 0x10)) {
    case 0: {
        *(s32*)(lbl_805FB014 + 0x4F4) = 0;
        fn_8016195C(lbl_8040E5F0, 3, 0, NULL);
        fn_8004504C();
        u32 h = StringHash("combat3_monster2");
        u8* cam = *(u8**)(c + 0x1C);
        (*(ObjVFnU*)(*(u8**)cam + 0x84))(cam, h);
        fn_80042364(c, choiceA, fighterA, 0);
        u8* base = FighterBase(fighterB);
        ScopedLock lock;
        ScopedLockCtor(&lock);
        (*(ObjVFn*)(*(u8**)base + 0x18))(base, 1);
        ScopedLockDtor(&lock, -1);
        *(u8*)(c + 9) = 1;
        fn_800E75E8(0);
        break;
    }
    case 1:
        fn_80042408(c, &lbl_805F8568, 0x1D, fighterA, 0);
        break;
    case 2:
        break;
    case 3: {
        fn_8016195C(lbl_8040E5F0, 3, 0, NULL);
        u32 h = StringHash("combat2_monster1");
        u8* cam = *(u8**)(c + 0x1C);
        (*(ObjVFnU*)(*(u8**)cam + 0x84))(cam, h);
        fn_80042408(c, &lbl_805F856C, 0x30, fighterA, 1);
        break;
    }
    case 4: {
        LockedVirtual1(FighterBase(fighterB), 0x14);
        {
            u8* base = FighterBase(fighterA);
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(ObjVFn*)(*(u8**)base + 0x18))(base, 1);
            ScopedLockDtor(&lock, -1);
        }
        *(u8**)(c + 0x14) = (*(GetFn*)(*(u8**)partB + 0x10))(partB);
        fn_80042364(c, choiceB, fighterB, 0);
        break;
    }
    case 5:
        fn_80042408(c, &lbl_805F8570, 0x1D, fighterB, 0);
        break;
    case 6: {
        LockedVirtual1(FighterBase(fighterA), 0x14);
        u32 camAnim = fn_8003F858();
        u8* cam = *(u8**)(c + 0x1C);
        (*(ObjVFnU*)(*(u8**)cam + 0x84))(cam, camAnim);
        f32 shake[3];
        shake[0] = 3.0f;
        shake[1] = 0.02f;
        shake[2] = 0.2f;
        fn_8016195C(lbl_8040E5F0, 3, 0, shake);
        *(f32*)(c + 4) = 0.0f;
        break;
    }
    case 7:
        fn_80042408(c, &lbl_805F8574, 0x30, fighterB, 1);
        // falls through to the end-of-intro check
    default:
        if (*(f32*)(c + 4) < 1.7f) {
            ChoiceDestroy(choiceB, -1);
            ChoiceDestroy(choiceA, -1);
            return 0;
        }
        GameFlowSignal((s32)0xF15918A6u, 0, 0);
        SendNISSignal(0xF15918A6u);
        result = 1;
        *(s32*)(lbl_805FB014 + 0x4F4) = 1;
        *(u8*)(gGame + 0x24) = 0;
        break;
    }

    ChoiceDestroy(choiceB, -1);
    ChoiceDestroy(choiceA, -1);
    return result;
}

}  // extern "C"
