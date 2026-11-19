// Missing functions batch 12: arm-wrestle arm movement, brain chunk updates, astral pickup,
// field-repair health assignment, battle HUD turn marker, limb loss sequence, misc script callbacks.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "game/part.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
#include "game/script_callbacks.h"

u32   EffectEmitter(void* db, s32 effect, s32 limb);                 // 0x800523B8
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);       // 0x80209E70
Vec3  QuatGetAxisY(const Quat* q);                                   // 0x801452A8
s32   RaySceneTest(void* ray);                                       // 0x80070458 (ignores its argument)
s32   RoundGetAttacker(void* round);                                 // 0x80214F48
void  ScopedLockCtor(ScopedLock* l);                                 // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                      // 0x801F53B0
void* ScreenSetFindElement(void* set, u32 hash);                     // 0x80198F18
void* ScreenSetsCurrent(void* sets);                                 // 0x8018D1E4
void  UIPlayAnim(void* el, u32 anim);                                // 0x8019BE40
f32   fn_8002D524(s32 slot);                                         // 0x8002D524
s32   fn_80048CB4(s32 side);                                         // 0x80048CB4
void  fn_8004B938(u8* chr);                                          // 0x8004B938
void  fn_8004BA50(u8* chr);                                          // 0x8004BA50
void  fn_8004C710(u8* chr, s32 a, s32 b);                            // 0x8004C710
void  fn_8004E060(u8* fighters, s32 slot, s32 delta);                // 0x8004E060
s32   fn_8004E2A4(u8* fighters, s32 slot);                           // 0x8004E2A4
s32   fn_8004E2C4(u8* fighters, s32 slot);                           // 0x8004E2C4
u32   fn_8004E2E4(u8* fighters, s32 slot);                           // 0x8004E2E4
void  fn_8004F3E4(u8* limb, s32 on);                                 // 0x8004F3E4
u8*   fn_8006FBDC(void);                                             // 0x8006FBDC
void  fn_800712B4(void);                                             // 0x800712B4
void  fn_800712B8(void);                                             // 0x800712B8
void  fn_80082474(u8* chunk, s32 a, f32 dt);                         // 0x80082474
void  fn_8008684C(f32* out, u8* obj);                                // 0x8008684C
void  fn_8011CE04(u8* badges, s32 type);                             // 0x8011CE04
void  fn_80146180(Quat* out, const Vec3* axis, f32 angle);           // 0x80146180
f32   fn_8018BE30(u8* p, f32 t);                                     // 0x8018BE30
void  fn_801F0658(f32 t);                                            // 0x801F0658
u8*   fn_80216658(GameDb* db, s32 key);                              // 0x80216658
void  fn_80229030(u8* save, s32 n);                                  // 0x80229030

extern u8  gGame[];                  // 0x803B53A0
extern u8  gWorldState[];            // 0x803E6D00 (+0xC frame delta)
extern s32 lbl_805FB0B0[];           // 0x805FB0B0
extern u8  lbl_805FB248[];           // 0x805FB248 (lab books section table pointer)
extern u8* lbl_805FBBE0;             // 0x805FBBE0 (save block pointer)
extern const u32 lbl_805F8630[];     // 0x805F8630 (HUD element hashes)
extern const u32 lbl_80358A08[];     // 0x80358A08 (HUD animation hashes)
extern const s32 lbl_80347A60[];     // 0x80347A60 (limb index -> emitter id)
extern const u32 lbl_803881B4[];     // 0x803881B4 (emitter id -> emitter)
extern const u32 lbl_805FDDC0[];     // 0x805FDDC0 (pickup sound ids)
extern const f32 lbl_80346398[];     // 0x80346398
extern const f32 lbl_803462E8[];     // 0x803462E8
extern const f32 lbl_803463A8[];     // 0x803463A8
extern const f32 lbl_803460B8[];     // 0x803460B8
extern const f32 lbl_803460C8[];     // 0x803460C8
extern const f32 lbl_803460D8[];     // 0x803460D8
extern u8  lbl_8055C9A8[];           // 0x8055C9A8 (Vec3 listener forward vectors, 12 bytes per id)
extern u8  lbl_8055C9D8[];           // 0x8055C9D8 (Vec3 listener up vectors, 12 bytes per id)
}

extern void* gScreenSets;            // 0x805FB654
extern const u32 kAbilityEffects[];  // 0x80388258

static const u32 kHashBadges = 0x880E941Cu;          // "badges"
static const u32 kHashHudScreen = 0x1B66AC94u;       // "UI_MSA_Combat_HUD"
static const u32 kHashAstralAvatar = 0x64073331u;    // "astral_avatar"

// Defined in progress.cpp with C++ linkage.
struct SaveProgress;
extern "C++" {
u32  SaveGetFieldRepairRecord(SaveProgress* s);                      // 0x8022954C
void SaveSetFieldRepairRecord(SaveProgress* s, u32 points);          // 0x80229554
}

typedef u8* (*GetFn)(u8*);

// Flags on a scene object definition: position/orientation dirty bits (0x8000, 0x4000, 0x10000000).
static inline void MarkTransformDirty(u8* def) {
    *(u32*)(def + 0x20) |= 0x8000;
    *(u32*)(def + 0x20) |= 0x10000000;
    *(u32*)(def + 0x20) |= 0x4000;
    *(u32*)(def + 0x20) |= 0x10000000;
}

extern "C" {

// 0x801035C8
// True when the entity's altitude (+0x7D4) is below the given height.
s32 AltitudeLessThan(u32 /*0x2F*/ a0, f32 a1) {
    return *(f32*)((u8*)(uintptr_t)a0 + 0x7D4) < a1;
}

// 0x8005C82C
// True when one of the two players' entities is playing the given anim def, is closer than 4 units
// and is facing (positive dot with its Y axis) the vector towards this entity.
u32 /*0x06*/ AmIThreatenedByPlayersAnimDef(u32 /*0x2E*/ a0, u32 a1) {
    u8* self = (u8*)(uintptr_t)a0;
    for (s32 i = 0; i < 2; i++) {
        u8* other = *(u8**)(*(u8**)gWorldState + i * 4 + 0x10);
        if (other == NULL || a1 != GetCurAnimDef((u32)(uintptr_t)other)) continue;  // lint:ok header takes u32
        u8* mine = *(u8**)(self + 4);
        u8* theirs = *(u8**)(other + 4);
        f32 dx = *(f32*)(mine + 0) - *(f32*)(theirs + 0);
        f32 dy = *(f32*)(mine + 4) - *(f32*)(theirs + 4);
        f32 dz = *(f32*)(mine + 8) - *(f32*)(theirs + 8);
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 len = 0.0f;
        if (len2 > 0.0f) len = sqrtf(len2);
        if (len >= 4.0f) continue;
        Vec3 axis = QuatGetAxisY((const Quat*)(theirs + 0xC));
        if (dx * axis.x + dy * axis.y + dz * axis.z > 0.0f) return 1;
    }
    return 0;
}

// 0x8005C724
// Adds a knockback push of the given strength along the entity's current velocity direction
// (+0x850..0x858), when the velocity is longer than ~0.7 units.
void AmplifyKnockback(u32 /*0x2E*/ a0, f32 a1) {
    u8* p = (u8*)(uintptr_t)a0;
    f32 vx = *(f32*)(p + 0x850);
    f32 vy = *(f32*)(p + 0x854);
    f32 vz = *(f32*)(p + 0x858);
    f32 len2 = vz * vz + (vx * vx + vy * vy);
    if (len2 > 0.5f) {
        f32 nx = 0.0f, ny = 0.0f, nz = 0.0f;
        if (len2 != 0.0f) {
            f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
            nx = vx * inv;
            ny = vy * inv;
            nz = vz * inv;
        }
        f32 px = nx * a1;
        f32 py = ny * a1;
        f32 pz = nz * a1;
        *(f32*)(p + 0x850) = *(f32*)(p + 0x850) + px;
        *(f32*)(p + 0x854) = *(f32*)(p + 0x854) + py;
        *(f32*)(p + 0x858) = *(f32*)(p + 0x858) + pz;
    }
}

// 0x800EC7EC
// True when one of the three anim slots at +0xA0 plays the given anim and has finished
// (flag 0x10 set, flag 0x20000 clear).
s32 AnimDoneUB(void* a0, u32 a1) {
    for (s32 i = 0; i < 3; i++) {
        u8* slot = *(u8**)((u8*)a0 + 0xA0);
        if (slot != NULL && a1 == *(u32*)(slot + i * 0x24 + 4)) {
            s32 flags = *(s32*)(slot + i * 0x24 + 0x18);
            if (!(flags & 0x20000) && (flags & 0x10)) return 1;
        }
    }
    return 0;
}

// 0x801035DC
// Forwards to the object's virtual method at +0x90 (set percent complete) if there is an object.
void AnimSetPercentComplete(void* a0, f32 a1) {
    if (a0 == NULL) return;
    (*(void (**)(void*, f32))(*(u8**)a0 + 0x90))(a0, a1);
}

// 0x8018BE78
// Current time of the anim held in the first slot, or 0 when there is none.
f32 AnimGetTime(void* anim) {
    u8* a = *(u8**)anim;
    if (a != NULL) {
        u8* data = *(u8**)(a + 0x14);
        if (*(s32*)(data + 0x14) != 0) return *(f32*)(data + 0x24);
    }
    return 0.0f;
}

// 0x800B778C
// Arm wrestle: moves the arm "down": raises +0x2F4, lowers +0x2F0, clamped to [0, table limit].
void ArmWrestleArmMoveDown(void* a0) {
    u8* p = (u8*)a0;
    f32 dt = *(f32*)(gWorldState + 0xC);
    f32 speed = 0.0f;
    if (speed == speed) speed = lbl_803460C8[RaySceneTest(NULL)];
    f32 step = speed * dt;
    *(f32*)(p + 0x2F4) = *(f32*)(p + 0x2F4) + step;
    *(f32*)(p + 0x2F0) = *(f32*)(p + 0x2F0) - step;
    f32 limit = lbl_803460D8[RaySceneTest(NULL)];
    if (*(f32*)(p + 0x2F4) > limit) *(f32*)(p + 0x2F4) = lbl_803460D8[RaySceneTest(NULL)];
    if (*(f32*)(p + 0x2F0) < 0.0f) *(f32*)(p + 0x2F0) = 0.0f;
}

// 0x800B7644
// Arm wrestle: rotates the arm left (angle at +0x2C4 grows, capped at 0.5555556), rebuilds the arm
// quaternion at +0x27C and copies it into the scene object's orientation.
void ArmWrestleArmMoveLeft(void* a0) {
    u8* p = (u8*)a0;
    f32 scale = 1.0f;
    if (scale > 10.0f) scale = 10.0f;
    f32 angle = *(f32*)(p + 0x2C4) + 0.011111111f * (5.0f * scale) * *(f32*)(gWorldState + 0xC);
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(f32*)(p + 0x288) = 1.0f;
    *(f32*)(p + 0x2C4) = angle;
    if (angle > 0.5555556f) *(f32*)(p + 0x2C4) = 0.5555556f;
    Vec3 axis = QuatGetAxisY((const Quat*)(p + 0x28C));
    fn_80146180((Quat*)(p + 0x27C), &axis, *(f32*)(p + 0x2C4));
    u8* def = *(u8**)(p + 4);
    *(f32*)(def + 0xC) = *(f32*)(p + 0x27C);
    *(f32*)(def + 0x10) = *(f32*)(p + 0x280);
    *(f32*)(def + 0x14) = *(f32*)(p + 0x284);
    *(f32*)(def + 0x18) = *(f32*)(p + 0x288);
    MarkTransformDirty(*(u8**)(p + 4));
}

// 0x800B746C
// Arm wrestle: rotates the arm right (angle at +0x2C4 shrinks, floored at -0.5555556), rebuilds the
// arm quaternion at +0x27C and copies it into the scene object's orientation.
void ArmWrestleArmMoveRight(void* a0) {
    u8* p = (u8*)a0;
    f32 scale = 1.0f;
    if (scale > 10.0f) scale = 10.0f;
    f32 angle = *(f32*)(p + 0x2C4) - 0.011111111f * (5.0f * scale) * *(f32*)(gWorldState + 0xC);
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(f32*)(p + 0x288) = 1.0f;
    *(f32*)(p + 0x2C4) = angle;
    if (angle < -0.5555556f) *(f32*)(p + 0x2C4) = -0.5555556f;
    Vec3 axis = QuatGetAxisY((const Quat*)(p + 0x28C));
    fn_80146180((Quat*)(p + 0x27C), &axis, *(f32*)(p + 0x2C4));
    u8* def = *(u8**)(p + 4);
    *(f32*)(def + 0xC) = *(f32*)(p + 0x27C);
    *(f32*)(def + 0x10) = *(f32*)(p + 0x280);
    *(f32*)(def + 0x14) = *(f32*)(p + 0x284);
    *(f32*)(def + 0x18) = *(f32*)(p + 0x288);
    MarkTransformDirty(*(u8**)(p + 4));
}

// 0x800B75B4
// Arm wrestle: moves the arm "up": lowers +0x2F4, raises +0x2F0; +0x2F4 is floored at 0.
void ArmWrestleArmMoveUp(void* a0) {
    u8* p = (u8*)a0;
    f32 dt = *(f32*)(gWorldState + 0xC);
    f32 speed = 0.0f;
    if (speed == speed) speed = lbl_803460B8[RaySceneTest(NULL)];
    f32 step = speed * dt;
    f32 down = *(f32*)(p + 0x2F4) - step;
    *(f32*)(p + 0x2F4) = down;
    *(f32*)(p + 0x2F0) = *(f32*)(p + 0x2F0) + step;
    if (down < 0.0f) *(f32*)(p + 0x2F4) = 0.0f;
}

// 0x800F5C1C
// Field repair: for each of the 5 limbs, changes its health by (current - max * factor), clamped to
// [-max, 0]; the total health lost is recorded as the field repair record. Reaching the factor on
// every limb awards the field repair save flag.
void AssignFieldRepairHealthPointsToPlayer(void) {
    s32 allRepaired = 1;
    u32 total = 0;
    u8* fighters = *(u8**)(gGame + 0xD614);
    if (fighters != NULL && (*(s32*)(*(u8**)(fighters + 4) + 0x1C) & 0x2000)) {
        for (s32 i = 0; i < 5; i++) {
            if (fn_8004E2E4(fighters, i) == 0) continue;
            s32 cur = fn_8004E2A4(fighters, i);
            f32 maxHp = (f32)fn_8004E2C4(fighters, i);
            f32 repaired = maxHp * fn_8002D524(i);
            s32 delta = (s32)((f32)cur - repaired);
            s32 lo = (s32)(-maxHp);
            s32 change;
            if (delta < lo) {
                change = lo;
            } else {
                change = delta > 0 ? 0 : delta;
            }
            fn_8004E060(fighters, i, change);
            total -= (u32)change;
            if (repaired < maxHp) allRepaired = 0;
        }
    }
    if (allRepaired != 0) fn_80229030(lbl_805FBBE0, 1);
    if (SaveGetFieldRepairRecord((SaveProgress*)lbl_805FBBE0) < total) {
        SaveSetFieldRepairRecord((SaveProgress*)lbl_805FBBE0, total);
    };
}

// 0x8009E600
// Astral avatar picked up a pickup (kind at +0x29C: 0 = energy, 1 = health): plays the sound,
// deactivates the pickup, then adds the table amount to the player's stat (clamped) and records the pickup.
void AstralPickUpCollisionCallback(void* a0, void* a1) {
    u8* pickup = (u8*)a0;
    u8* other = *(u8**)((u8*)a1 + 0x9C);
    s32 id = 0;
    if (other != NULL) id = *(s32*)(other + 8);
    if ((u32)id != kHashAstralAvatar) return;

    PlayExperimentSound(NULL, lbl_805FDDC0[*(s32*)(pickup + 0x29C)], 0);
    *(u8*)(pickup + 0x270) = 0;
    *(u8*)(pickup + 0x268) = 1;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    (*(void (**)(u8*, s32))(*(u8**)pickup + 0x18))(pickup, 0);
    ScopedLockDtor(&lock, -1);

    u8* player = fn_8006FBDC();
    s32 kind = *(s32*)(pickup + 0x29C);
    if (kind == 0) {
        f32 amount = lbl_80346398[RaySceneTest(NULL)];
        u8* stats = *(u8**)(player + 0x198);
        f32 v = *(f32*)(stats + 0x3E8) + amount;
        f32 limit = lbl_803462E8[RaySceneTest(NULL)];
        f32 result;
        if (0.0f < limit) {
            if (v < 0.0f) {
                result = 0.0f;
            } else {
                result = v > limit ? limit : v;
            }
        } else {
            if (v < limit) {
                result = limit;
            } else {
                result = v > 0.0f ? 0.0f : v;
            }
        }
        *(f32*)(stats + 0x3E8) = result;
    } else if (kind == 1) {
        f32 amount = lbl_803463A8[RaySceneTest(NULL)];
        if (amount > 0.0f) {
            fn_800712B8();
        } else if (amount < 0.0f) {
            fn_800712B4();
        }
        f32 v = *(f32*)(player + 0x1C0) + amount;
        f32 result;
        if (v < 0.0f) {
            result = 0.0f;
        } else {
            result = v > 100.0f ? 100.0f : v;
        }
        *(f32*)(player + 0x1C0) = result;
    }
    s32 k = *(s32*)(pickup + 0x29C);
    *(s32*)(player + 0x98) = k;
    *(f32*)(player + 0x90) = (k != -1) ? 3.0f : -1.0f;
}

// 0x8004A9DC
// A limb is lost during an attack sequence: updates the fighter's limb state, plays the camera and
// hit sounds and spawns the ability particle effects on the lost limb.
void AttackSequence_LoseLimb(void* seq, s32 side, void* part) {
    u8* partp = (u8*)part;
    u8* limb = *(u8**)(partp + 0x20);
    s32 limbIdx = *(s32*)(partp + 4);
    if (limb != NULL) limb -= 0x264;
    u8* chr = *(u8**)(*(u8**)(partp + 0x24) + 0x1C);
    if (chr != NULL) chr -= 0xC48;
    u8* chr2 = *(u8**)(chr + 0xC60);

    fn_8004BA50(chr);
    if (limbIdx == 4) {
        fn_8004C710(chr, 1, limbIdx);
    } else if (limbIdx != 1) {
        fn_8004C710(chr, 4, limbIdx);
    }
    fn_8004B938(chr);
    *(s32*)(*(u8**)(partp + 0x28) + 0x28) = 1;
    if (side != RoundGetAttacker(*(void**)((u8*)seq + 4))) PlayCameraAnimation(0x95F2DC8Fu);
    PlaySound(0x66BB2D2Du, 0, 0, 0, 0, 0);
    fn_8004F3E4(limb, 1);
    fn_801F0658(0.25f);

    u8* limbPart = (*(GetFn*)(*(u8**)limb + 0xCC))(limb);
    u8* list = fn_80216658(gGameDb, **(s32**)limbPart);
    if (list != NULL && *(s32*)((u8*)gGameDb + 0x10) == 1) {
        for (s32 i = 0; i < 3; i++) {
            u8* e = list + i * 4;
            if (*(s32*)(e + 0x1C) != -1) {
                s32 emitter = *(s32*)(e + 0x28);
                if (emitter != -1) {
                    PlayAttachedParticleEffectAtEmitter(limb, kAbilityEffects[*(s32*)(e + 0x1C)],
                                                        EffectEmitter(gGameDb, emitter, limbIdx));
                }
            }
        }
    }

    u8* chrPart = (*(GetFn*)(*(u8**)chr2 + 0xCC))(chr2);
    list = fn_80216658(gGameDb, **(s32**)chrPart);
    if (list != NULL) {
        s32 wanted = lbl_80347A60[limbIdx];
        for (s32 i = 0; i < 3; i++) {
            u8* e = list + i * 4;
            s32 effect = *(s32*)(e + 0x1C);
            if (effect != -1) {
                s32 emitter = *(s32*)(e + 0x28);
                if (emitter == wanted) {
                    PlayAttachedParticleEffectAtEmitter(limb, kAbilityEffects[effect], lbl_803881B4[emitter]);
                }
            }
        }
    }
    SendNISSignal(0x2516A098u);
}

// 0x8020AA3C
// Sets listener id's forward vector.
void AudioSetListenerFwd(s32 id, const Vec3* v) {
    f32* dst = (f32*)(lbl_8055C9A8 + id * 12);
    dst[0] = v->x;
    dst[1] = v->y;
    dst[2] = v->z;
}

// 0x8020AA68
// Sets listener id's up vector.
void AudioSetListenerUp(s32 id, const Vec3* v) {
    f32* dst = (f32*)(lbl_8055C9D8 + id * 12);
    dst[0] = v->x;
    dst[1] = v->y;
    dst[2] = v->z;
}

// 0x80115D60
// Increments the given badge type if the lab books section table has an object that answers the
// "badges" query.
void Badges_IncType(s32 a0) {
    u8* table = *(u8**)lbl_805FB248;
    u8* obj = NULL;
    if (table != NULL) obj = *(u8**)(table + 8);
    u8* badges = NULL;
    if (obj != NULL) badges = (*(u8* (**)(u8*, u32))(*(u8**)obj + 0x28))(obj, kHashBadges);
    if (badges != NULL) fn_8011CE04(badges, a0);
}

// 0x80048BF4
// Starts the HUD turn-marker animation for the side when its state changes (only on the combat HUD).
void BattleHudSetTurn(s32 side, BOOL on) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    if (*(u32*)set != kHashHudScreen) return;
    void* el = ScreenSetFindElement(set, lbl_805F8630[side]);
    if (el != NULL && on != lbl_805FB0B0[side] && (on == 1 || fn_80048CB4(side) != 0)) {
        lbl_805FB0B0[side] = on;
        UIPlayAnim(el, lbl_80358A08[side * 2 + on]);
    }
}

// 0x80082384
// Marks the brain chunk in use / not in use: sets or clears flag 2 on its collider and scene
// object, and resets the state timer at +0x2B4.
void BrainChunkSetInUse(u32 /*0x56*/ a0, u32 /*0x06*/ a1) {
    u8* chunk = (u8*)(uintptr_t)a0;
    u8 on = a1 != 0;
    if (*(u8*)(chunk + 0x268) != on) {
        *(u8*)(chunk + 0x268) = on;
        typedef u8* (*GetColliderFn)(u8*);
        if (on == 0) {
            if ((*(GetColliderFn*)(*(u8**)chunk + 0xC))(chunk) != NULL) {
                u8* collider = (*(GetColliderFn*)(*(u8**)chunk + 0xC))(chunk);
                *(u32*)(collider + 8) &= ~2u;
            }
            *(u32*)(*(u8**)(chunk + 4) + 0x20) &= ~2u;
        } else {
            if ((*(GetColliderFn*)(*(u8**)chunk + 0xC))(chunk) != NULL) {
                u8* collider = (*(GetColliderFn*)(*(u8**)chunk + 0xC))(chunk);
                *(u32*)(collider + 8) |= 2u;
            }
            *(u32*)(*(u8**)(chunk + 4) + 0x20) |= 2u;
        }
        *(s32*)(chunk + 0x2B4) = 0;
    }
}

// 0x8008345C
// Bounce state update: clears the timer at +0x2B8 and runs the shared brain chunk update with dt.
void BrainChunkUpdateBounce(u32 /*0x56*/ a0) {
    u8* chunk = (u8*)(uintptr_t)a0;
    *(s32*)(chunk + 0x2B8) = 0;
    fn_80082474(chunk, 1, *(f32*)(gWorldState + 0xC));
}

// 0x80083478
// Load state update: follows the anim on the owner at +0x2C0 (position from the owner's anim, a
// small offset while the anim is still before the timer at +0x274, which resets otherwise) and copies
// the owner's orientation into the chunk's scene object.
void BrainChunkUpdateLoad(u32 /*0x56*/ a0) {
    u8* chunk = (u8*)(uintptr_t)a0;
    u8* owner = *(u8**)(chunk + 0x2C0);
    u8* animObj = *(u8**)(owner + 0x1F4);
    u8* anim = (*(u8* (**)(u8*, u8*))(*(u8**)animObj + 0x10))(animObj, owner);
    f32 time = AnimGetTime(anim);
    fn_8018BE30(anim, 0.0f);
    f32 pos[3];
    fn_8008684C(pos, *(u8**)(*(u8**)(chunk + 0x2C0) + 0x1F4));
    if (*(f32*)(chunk + 0x274) < time) {
        pos[2] += 5.0f * (1.0f - fn_8018BE30(anim, 0.0f));
    } else {
        *(f32*)(chunk + 0x274) = 0.0f;
    }
    u8* def = *(u8**)(chunk + 4);
    *(f32*)(def + 0x0) = pos[0];
    *(f32*)(def + 0x4) = pos[1];
    *(f32*)(def + 0x8) = pos[2];
    MarkTransformDirty(*(u8**)(chunk + 4));
    u8* rot = *(u8**)(*(u8**)(*(u8**)(chunk + 0x2C0) + 0x1F4) + 4);
    def = *(u8**)(chunk + 4);
    *(f32*)(def + 0xC) = *(f32*)(rot + 0xC);
    *(f32*)(def + 0x10) = *(f32*)(rot + 0x10);
    *(f32*)(def + 0x14) = *(f32*)(rot + 0x14);
    *(f32*)(def + 0x18) = *(f32*)(rot + 0x18);
    MarkTransformDirty(*(u8**)(chunk + 4));
}

}  // extern "C"
