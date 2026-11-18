// Battle attack playback: per-frame update of the task that plays one attack exchange. It
// triggers the hit reactions on the impact frame, spawns ability particle effects on the hit
// limbs, plays knock-out / limb-loss animations, drives the battle camera and finishes once
// every limb has settled (or after 10 seconds).
// Original: AttackSequence_Update 0x80049928 (vtable 0x80345AB8)
#include <stddef.h>

#include "types.h"
#include "game/part.h"
#include "game/script_callbacks.h"

#define LIMBS 5
#define SEQUENCE_TIMEOUT 10.0f

extern "C" {
s32   RoundGetAttacker(void* round);                       // 0x80214F48
f32   AnimGetFrame(void* anim, f32 fps);                   // 0x8018C014
BOOL  CharacterAttackDone(void* ch);                       // 0x8004D5BC
void  ReactionBuild(u8* out, s32 a, void* fighter, const char* anim);   // 0x80215648
void  ReactionBuildHit(void* round, u8* out, s32 side);             // 0x80214F5C
void  ReactionBuildRecover(u8* out, void* fighter);        // 0x802159C4
void  LimbPlayReaction(void* limb, u8* reaction, BOOL special);   // 0x8004F398
void  CharacterStartReaction(void* ch);                    // 0x8004E3D0
void* AbilityEffectForLimb(void* db, s32 ability, s32 limb);   // 0x802167E4
void* AbilityEffect(void* db, s32 ability);                // 0x8021679C
u32   EffectEmitter(void* db, s32 effect, s32 limb);       // 0x800523B8
BOOL  LimbHasEffect(void* limb, u32 effect);               // 0x8005015C
void  BattleHudSetTurn(s32 side, BOOL on);                 // 0x80048BF4
void  fn_800F4FB0(void);
BOOL  BattleIsFinalRound(void* battle);                    // 0x80212BE4
void  BattleHudLimbLost(s32 limb);                         // 0x800477DC
void  AttackSequence_LoseLimb(void* seq, s32 side, void* part);   // 0x8004A9DC
BOOL  LimbUpdateFall(void* limb, f32 dt);                  // 0x8004F774
void  fn_801F0658(f32 t);
void* fn_8003F7DC(void);
f32   fn_8018BE30(void* obj, f32 a);
}

extern u8*   gBattle;               ORIG(0x805FBBB8)
extern u8    gBattleAnims;          ORIG(0x805F8598)
extern u8*   gBattleCamera;         ORIG(0x805FB014)
extern u8    lbl_805FB1C0, lbl_805FB1C1;
extern const u32 kAbilityEffects[]; // 0x80388258

struct AttackSequence {
    void** vtable;          // 0x00: +0x14 finish
    u8*    round;           // 0x04
    u8     pad08[4];
    s32    impactFrame[2];  // 0x0C: per side
    u8     counter[2];      // 0x14
    u8     pad16[2];
    s32    side;            // 0x18: side whose animation is timed
    s32    slot;            // 0x1C
    u8     impactDone;      // 0x20
    u8     done[2][6];      // 0x21: [side][limb], [side][5] = body
    u8     shakeDone;       // 0x2D
    u8     cameraDone;      // 0x2E
    u8     falling[2][5];   // 0x2F
    u8     pad39[3];
    f32    time;            // 0x3C
    f32    turnTime;        // 0x40
    u8     turnShown;       // 0x44
};

typedef void* (*GetFn)(void*);
#define VT(obj, off) (*(void**)(*(u8**)(obj) + (off)))

static inline u8* Fighter(s32 i) { return ((u8**)(gBattle + 0xA00))[i]; }
static inline u8* FighterSlot(u8* f, s32 i) { return ((u8**)f)[i]; }
// The limb's scene object; BattlePart +0x20 points 0x264 bytes into it.
static inline u8* LimbObj(u8* part) {
    u8* p = *(u8**)(part + 0x20);
    return p != NULL ? p - 0x264 : NULL;
}
// The fighter's character; Fighter +0x1C points 0xC48 bytes into it.
static inline u8* FighterChar(u8* f) {
    u8* p = *(u8**)(f + 0x1C);
    return p != NULL ? p - 0xC48 : NULL;
}
static inline void Finish(AttackSequence* a) { ((void (*)(void*))a->vtable[0x14 / 4])(a); }

static void PlayReactions(u8* fighter, u8* reaction, BOOL special) {
    for (s32 i = 0; i < LIMBS; i++)
        if (FighterSlot(fighter, i) != NULL) LimbPlayReaction(LimbObj(FighterSlot(fighter, i)), reaction + i * 0x100, special);
    CharacterStartReaction(FighterChar(fighter));
}

static void SpawnAbilityEffects(AttackSequence* a, s32 att, s32 frame) {
    u8* round = a->round;
    u8* def = Fighter(1 - att);
    u8* defSide = round + (1 - att) * 0x38;
    u8* defLimb = LimbObj(FighterSlot(def, *(s32*)(defSide + 0x38)));
    u8* attLimb = LimbObj(FighterSlot(Fighter(att), *(s32*)(round + att * 0x38 + 0x38)));

    u8* attPart = (u8*)((GetFn)VT(attLimb, 0xCC))(attLimb);
    s32 ability = *(s32*)(*(u8**)(attPart + 0x28) + 0x20);
    s32 effect = -1;
    if (ability != -1) effect = *(s32*)(*(u8**)(gGameDb->data + 0x3610) + ability * 0x24 + 0x10);
    ((GetFn)VT(defLimb, 0xCC))(defLimb);

    s32 kind = *(s32*)round;
    if (kind == 6 || kind == 7 || kind == 4 || kind == 3) return;

    if (kind != 5 && effect != -1) {
        if (ability == 0 || ability == 1 || ability == 6) {
            for (s32 i = 0; i < LIMBS; i++) {
                u8* rec = *(u8**)(defSide + 4 + i * 4);
                if (rec == NULL || *(s32***)(rec + 4) == NULL) continue;
                if (-**(*(s32***)(rec + 4)) <= 0) continue;     // no damage on this limb
                u8* e = (u8*)AbilityEffectForLimb(gGameDb, ability, i);
                u32 em = EffectEmitter(gGameDb, *(s32*)(e + 0xC), i);
                if (FighterSlot(def, i) != NULL)
                    PlayAttachedParticleEffectAtEmitter(LimbObj(FighterSlot(def, i)), kAbilityEffects[effect], em);
            }
        } else if (ability == 3) {
            u8* e = (u8*)AbilityEffect(gGameDb, ability);
            s32 limb = *(s32*)(e + 8);
            u32 em = EffectEmitter(gGameDb, *(s32*)(e + 0xC), limb);
            PlayAttachedParticleEffectAtEmitter(LimbObj(FighterSlot(def, limb)), kAbilityEffects[effect], em);
        }
    }

    if (ability == 0xD) {
        for (s32 i = 0; i < LIMBS; i++) {
            if (i == 4 || *(u8**)(defSide + 4 + i * 4) == NULL) continue;
            u8* e = (u8*)AbilityEffectForLimb(gGameDb, 0xD, i);
            u32 em = EffectEmitter(gGameDb, *(s32*)(e + 0xC), i);
            if (FighterSlot(def, i) == NULL) continue;
            u8* obj = LimbObj(FighterSlot(def, i));
            if (obj != NULL && !LimbHasEffect(obj, kAbilityEffects[effect]))
                PlayAttachedParticleEffectAtEmitter(obj, kAbilityEffects[effect], em);
        }
    } else if (ability == 0xE) {
        u8* e = (u8*)AbilityEffect(gGameDb, 0xE);
        s32 limb = *(s32*)(e + 8);
        u32 em = EffectEmitter(gGameDb, *(s32*)(e + 0xC), limb);
        if (FighterSlot(def, limb) == NULL) return;
        u8* obj = LimbObj(FighterSlot(def, limb));
        if (obj != NULL && !LimbHasEffect(obj, kAbilityEffects[effect]))
            PlayAttachedParticleEffectAtEmitter(obj, kAbilityEffects[effect], em);
    }
    (void)frame;
}

static void UpdateCamera(AttackSequence* a) {
    u8* cam = *(u8**)(gBattleCamera + 0x4D4);
    if (cam == NULL) {
        a->cameraDone = 1;
        return;
    }
    if (!(a->time > 0.0001f)) return;
    u8* shot = (u8*)((GetFn)VT(cam, 0x10))(cam);
    if (*(void**)(shot + 0xEC) == fn_8003F7DC()) {
        a->cameraDone = 1;
    } else if (shot != NULL) {
        f32 t = fn_8018BE30(shot, 0.0f) - 1.0f;
        if (t > -0.0001f && t < 0.0001f) {
            a->cameraDone = 1;
            lbl_805FB1C0 = 0;
            lbl_805FB1C1 = 0;
            ((void (*)(void*, void*))VT(cam, 0x84))(cam, fn_8003F7DC());
        } else {
            a->cameraDone = 0;
        }
    } else {
        a->cameraDone = 0;
    }
}

// 0x80049928
void AttackSequence_Update(AttackSequence* a, f32 dt) {
    s32 att = RoundGetAttacker(a->round);
    if (a->round[0x74 + att] != 0 || !gBattleAnims) {
        Finish(a);
        return;
    }

    // Current frame of the timed fighter's animation.
    u8* timed = FighterSlot(Fighter(a->side), a->slot);
    u8* timedObj = timed != NULL ? *(u8**)(timed + 0x20) : NULL;
    if (timedObj != NULL) timedObj -= 0x264;
    void* anim = ((GetFn)VT(timedObj, 0x10))(timedObj);
    s32 frame = 0;
    if (anim != NULL) frame = (s32)(1.0f + AnimGetFrame(anim, 30.0f));

    const s32 hitFrame = a->impactFrame[a->side];

    // Impact: the defender reacts ("bodyshake" for a heavy hit).
    if (!a->impactDone && frame >= hitFrame - a->impactFrame[1 - a->side]) {
        a->impactDone = 1;
        u8* def = Fighter(1 - a->side);
        u8 reaction[5 * 0x100];
        BOOL special = FALSE;
        if (FighterChar(def)[0xC86]) {
            ReactionBuild(reaction, 0, def, "bodyshake");
            special = TRUE;
        } else {
            ReactionBuildHit(a->round, reaction, 1 - a->side);
        }
        PlayReactions(def, reaction, special);
    }

    BOOL shake = TRUE;
    u8* hud = *(u8**)(gBattle + 0xA54 + RoundGetAttacker(a->round) * 4);
    if (hud != NULL) shake = hud[0x44];

    if (!a->shakeDone && frame >= hitFrame) SpawnAbilityEffects(a, att, frame);

    if (a->time >= 1.0f) BattleHudSetTurn(att, TRUE);
    if (frame >= hitFrame) {
        if (!a->turnShown && hitFrame != -1) {
            a->turnShown = 1;
            BattleHudSetTurn(att, TRUE);
            a->turnTime = a->time;
        } else {
            f32 d = a->time - a->turnTime;
            if (d >= 1.0f)      BattleHudSetTurn(1 - att, TRUE);
            else if (d >= 0.0f) BattleHudSetTurn(1 - att, FALSE);
        }
    }

    if (!a->shakeDone && shake && frame >= hitFrame) {
        fn_800F4FB0();
        a->shakeDone = 1;
    }

    BOOL lastRound = BattleIsFinalRound(gBattle) && !gBattle[0xA5C];
    BOOL anyFalling = FALSE;
    for (s32 f = 0; f < 2; f++) {
        u8* fighter = Fighter(f);
        u8* ch = FighterChar(fighter);

        if (CharacterAttackDone(ch) && !a->done[f][5]) {
            u8 reaction[5 * 0x100];
            if (ch[0xC85]) {
                ReactionBuild(reaction, 1, fighter, "switchback");
                PlayReactions(fighter, reaction, FALSE);
                ch[0xC85] = 0;
            } else {
                ReactionBuildRecover(reaction, fighter);
                PlayReactions(fighter, reaction, TRUE);
                a->done[f][5] = 1;
            }
        }

        // In the deciding round the loser collapses.
        BOOL loser = lastRound && *(u8**)(gBattle + 0xA4C) != fighter;
        if (loser) {
            if (frame >= hitFrame && !a->done[f][5])
                ((void (*)(void*, u32, s32, s32))VT(ch, 0xA8))(ch, 0x044F5122, 0, 0);
            a->done[f][5] = 1;
        }

        for (s32 i = 0; i < LIMBS; i++) {
            u8* limb = ((u8**)(ch + 0xC50))[i];
            if (limb == NULL) {
                a->done[f][i] = 1;
                continue;
            }
            u8* part = (u8*)((GetFn)VT(limb, 0xCC))(limb);
            if (frame >= hitFrame) {
                s32 slot = *(s32*)(part + 4);
                if (!loser && (slot == PART_SLOT_TORSO || slot == PART_SLOT_LEGS || a->done[f][i])) {
                    a->done[f][i] = 1;
                    continue;
                }
                u8* status = *(u8**)(part + 0x28);
                if (loser || *(s32*)status <= 0) {
                    if (f == 0) BattleHudLimbLost(i);
                    if (*(s32*)(status + 0x28) == 0 && frame >= hitFrame) {
                        AttackSequence_LoseLimb(a, f, part);
                        a->falling[f][i] = 1;
                    }
                }
                a->done[f][i] = !a->falling[f][i];
            }
            if (a->falling[f][i]) {
                anyFalling = TRUE;
                u8 landed = (u8)LimbUpdateFall(LimbObj(part), dt);
                a->done[f][i] = landed;
                a->falling[f][i] = !landed;
            }
        }
    }
    if (!anyFalling) fn_801F0658(1.0f);

    UpdateCamera(a);

    if (a->done[att][5] && a->counter[1 - att]) a->done[1 - att][5] = 1;

    BOOL all = TRUE;
    for (s32 i = 0; i < 12; i++) all = all && (&a->done[0][0])[i];
    a->time += dt;
    if (a->time > SEQUENCE_TIMEOUT) all = TRUE;
    if (all) Finish(a);
}
