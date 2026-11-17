// AI / prop state callbacks (Patrol*, Pumpkin*, Hazard*, MovingChallenge*, ...), registered
// by name with the script system in RegisterAiStateCallbacks (0x80014350).
// Original object: .text 0x80014350 - 0x80016400 (auto_03_80014350_text.s)
//
// Each callback receives the script context; the character it drives is ctx->chr.
// Event ids are StringHash() values whose names aren't stored in the executable.
#include "game/character.h"
#include "game/game.h"

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  WorldSendEvent(void* world, s32 id, int a, int b, int c);    // 0x800D87EC
void  WorldSetFocus(void* world, s32 id);                         // 0x800D8854
void  PlayCharacterAnimation(Character* c, int a, u32 anim, int loop);
void  PlayMonsterAttackAnimation(void* player);
BOOL  IsMonsterAttackAnimationDone(void* player);
void  PauseAllExplorationCharacters(void);
void  GameFlowSignal(s32 id, s32 a, s32 b);                       // 0x8000C7F4
void  fn_80219004(void* p, s32 v);
void  SpeedUpNISCameraAnimation_800F6BBC(void);
void  fn_8004CE44(void* player);
void  UIElement_SetVisibleFlag(u32 hash, BOOL on);
void  UIScreen_PopScreen(u32 hash);
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);  // 0x801DC91C
BOOL  fn_800E6BD4(void);
s32   WorldGetPhase(void* world);                                 // 0x800D83A8
void  PauseAllExplorationEnemies(void);
void* UIFindElement(u32 hash);                                    // 0x80195464
void  CharacterFaceDirection(Character* c, const Vec3* dir, BOOL now);   // 0x8005B650
BOOL  fn_8004D170(void* chr);
BOOL  fn_80050F88(void);
}

extern void* gEventMgr;              // 0x805FB6C8
extern void* gObjDb;                 // 0x805FB180
extern u8*   gSaveSystem;            // 0x805FBBE0
extern u8*   lbl_805FBBC8;
extern u8    gGame[];                // 0x803B53A0
extern u8    gScriptCtx[];           // 0x803C4298

#define WORLD           (gGame + 0xD5F0)
#define PLAYER          (*(void**)(gGame + 0xD614))
#define CHAR_DEAD       0x80
#define CHAR_BOSS       0x400000

static inline void Kill(Character* c) {
    c->def->stateFlags |= CHAR_DEAD;
    VCALL(c, 0x58, CharKillFn)(c);
}

static inline void SendSelf(Character* c, u32 event) {
    EventSend(gEventMgr, c, NULL, event, 0, 0, 0, 0);
}

// 0x80014610
void PatrolIdleEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || gObjDb == NULL) return;

    u32 boss = c->def->stateFlags & CHAR_BOSS;
    if (boss && c->id == *(s32*)(gSaveSystem + 0x130)) {
        s32 phase = *(s32*)(gSaveSystem + 0x138);
        if (phase == 0 || phase == 3) {
            // The level's boss has been beaten: remove it and remember it.
            c->def->stateFlags &= ~CHAR_BOSS;
            c->def->stateFlags |= CHAR_DEAD;
            *(s32*)(gSaveSystem + 0x134) = c->id;
            if (*(s32*)(gSaveSystem + 0x138) == 0) WorldSetFocus(WORLD, c->id);
            VCALL(c, 0x58, CharKillFn)(c);
            return;
        }
    }

    if (boss) c->def->stateFlags &= ~CHAR_BOSS;
    c->velocity.x = c->velocity.y = c->velocity.z = 0.0f;
    c->unkC40 = 0.0f;
    if (c->path != NULL) PathSetMoving(c->path, TRUE);
}

// 0x80014E0C
void EntityReverseDirection(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL) return;
    PathReverse(c->path);
    c->def->pos = *PathGetPosition(c->path);
}

// 0x80014E70
void EntityWaitEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL) return;
    PathSetMoving(c->path, TRUE);
    if (c->def->flags & 0x2000) PlayCharacterAnimation(c, 1, 0xE3712F44 /* "IdleReg" */, 1);
}

// 0x80014EDC
void EntityWaitExit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL) return;
    PathSetMoving(c->path, FALSE);
}

// 0x80014F00
void EntityWaitUpdate(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80015020
void MovingChallengeGameEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    *(s32*)(gSaveSystem + 0x130) = c->id;   // the challenge's opponent
    fn_80219004(lbl_805FBBC8, c->unk9D8);
    GameFlowSignal(c->eventTarget, 0, 0);
    PauseAllExplorationCharacters();
}

// 0x8001507C
void MovingChallengeGameExit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    Kill(c);
    WorldSendEvent(WORLD, c->id, 0, 0, 0);
}

// 0x800150F4
void PumpkinWaitEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL) return;
    PathSetMoving(c->path, TRUE);
    c->waitTimer = 0.0f;
}

// 0x80015140
// After 2 seconds of waiting, the pumpkin moves on.
void PumpkinWaitUpdate(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    c->waitTimer += c->dt;
    if (c->waitTimer > 2.0f) EventSend(gEventMgr, c, NULL, 0x9BEE735B, 0, 0, 0, 0);
}

// 0x80015190
void PumpkinFreezeEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (!PathIsPaused(c->path)) SendSelf(c, 0x59CFBC0F);
}

// 0x800151F8
void PumpkinFreezeExit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    PathSetPaused(c->path, FALSE);
}

// 0x80015638
void PumpkinEscapeEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    c->waitTimer = 0.0f;
    c->unkC44 = 0.0f;
    PathSetMoving(c->path, FALSE);
    PathSetSpeed(c->path, c->pathSpeed);
}

// 0x8001568C
void PumpkinEscapeExit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    SpeedUpNISCameraAnimation_800F6BBC();
    c->unkC44 = 0.5f;
}

// 0x80015760
void PumpkinDeathEntry(ScriptCtx* ctx) {
    if (ctx->chr == NULL) return;
    PlayMonsterAttackAnimation(PLAYER);
}

// 0x80015784
void PumpkinDeathExit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c != NULL) Kill(c);
    SpeedUpNISCameraAnimation_800F6BBC();
    WorldSendEvent(WORLD, c->id, 0, 0, 0);
}

// 0x80015800
void fn_80015800(void) {
}

// 0x80015804
void PumpkinDeathUpdate(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (IsMonsterAttackAnimationDone(PLAYER)) EventSend(gEventMgr, c, c, 0xE9BAFAD5, 0, 0, 0, 0);
}

// 0x8001595C
void ObstacleShakeExit(ScriptCtx* ctx) {
    (void)ctx;
    fn_8004CE44(PLAYER);
    UIElement_SetVisibleFlag(0x552FC6C7, FALSE);
    UIScreen_PopScreen(0x2CFF8941);
    UIScreen_PopScreen(0xE5BF8E82);
}

// 0x80015B70
void HazardDeathEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    Kill(c);
}

// Hazards fire their effect event every 10 seconds of exposure.
static inline void HazardTick(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    c->hazardTimer += c->dt;
    if (c->hazardTimer > 10.0f) EventSend(gEventMgr, c, NULL, 0x874A902B, 0, 0, 0, 0);
}

// 0x80015BA0
void HazardPoisonGasUpdate(ScriptCtx* ctx) {
    HazardTick(ctx);
}

// 0x80015BF0
void HazardDustCloudUpdate(ScriptCtx* ctx) {
    HazardTick(ctx);
}

typedef BOOL (*IsBusyFn)(Character*);
typedef void (*StartActionFn)(Character*, Character*, int, int, int, f32);

// 0x80016204
void CharacterStartIdleAction(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (!VCALL(c, 0x4C, IsBusyFn)(c)) {
        VCALL(c, 0x50, StartActionFn)(c, c, 0, 0, 0, 10000000.0f);
        c->flags838 |= 0x80;
    }
}

void PatrolIdleExit(ScriptCtx* ctx);
void character_exploration_move_update(ScriptCtx* ctx);

// 0x80014A38
void PatrolIdleUpdate(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;

    if (!c->unk895) {
        if (fn_800E6BD4() || *(s32*)(gSaveSystem + 0xA1C) != 0 || WorldGetPhase(WORLD) != 2) {
            SendSelf(c, 0x4F9213C1);
            return;
        }
    }
    if (c->unk87C == 0) return;

    if (c->path != NULL && (!PathAtWaypoint(c->path) || c->unkC40 > c->unkC44)) {
        PathSetMoving(c->path, FALSE);
        SendSelf(c, 0x705B8B52);
    } else {
        c->unkC40 += c->dt;
    }
}

#define EVENT_START_COMBAT 0xDDE9264F

// 0x80014B54
void CombatPlaceholderEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;

    Character* player = (Character*)PLAYER;
    if (c == player) {
        GameFlowSignal(EVENT_START_COMBAT, 0, 0);
        PauseAllExplorationEnemies();
    } else {
        if (player != NULL) EventSend(gEventMgr, player, c, EVENT_START_COMBAT, 0, 0, 0, 0);
        c->def->stateFlags |= CHAR_BOSS;
        *(s32*)(gSaveSystem + 0x130) = c->id;
    }
    if (c->path != NULL) PathSetMoving(c->path, TRUE);
    c->unkC82 = 0;
    c->waitTimer = 0.0f;
}

// 0x80014D5C
void CombatPlaceholderUpdate(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;

    Character* player = (Character*)PLAYER;
    if (c != player) {
        if (player != NULL) EventSend(gEventMgr, player, c, EVENT_START_COMBAT, 0, 0, 0, 0);
        return;
    }
    c->waitTimer += c->dt;
    if (c->waitTimer > 1.0f) {
        GameFlowSignal(EVENT_START_COMBAT, 0, 0);
        c->waitTimer = 0.0f;
    }
}

// 0x800156C8
void PumpkinEscapeUpdate(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (PathAtWaypoint(c->path)) {
        if (c->waitTimer > 2.0f) SendSelf(c, 0x33EE55C6);
        else                     PatrolIdleExit(ctx);
    }
    character_exploration_move_update(ctx);
}

// UI element (secondary vtable at +0x138; CodeWarrior multiple inheritance).
struct UIElement { u8 pad[0x64]; u32 flags; u8 pad68[0xD0]; void** vtbl2; };
typedef void (*UIStateFn)(UIElement*, int);

// 0x80015C40
// Blink the "shake" prompt: swap which of two button icons is lit (player 1 uses its own pair).
BOOL ShakePromptBlink(s32 player, f32 phase) {
    UIElement_SetVisibleFlag(0x552FC6C7, TRUE);
    if (!(phase > 0.5f)) return FALSE;

    UIElement* a = (UIElement*)UIFindElement(0x1A9EE60F);
    UIElement* b = (UIElement*)UIFindElement(0x8D6D65B1);
    if (player == 1) {
        a = (UIElement*)UIFindElement(0xC4E94BB0);
        b = (UIElement*)UIFindElement(0xB2601D05);
    }
    if (a != NULL && b != NULL) {
        if (a->flags & 2) {
            ((UIStateFn)a->vtbl2[0x30 / 4])(a, 2);
            ((UIStateFn)b->vtbl2[0x2C / 4])(b, 2);
        } else {
            ((UIStateFn)a->vtbl2[0x2C / 4])(a, 2);
            ((UIStateFn)b->vtbl2[0x30 / 4])(b, 2);
        }
    }
    return TRUE;
}

// 0x80014C24
// Leaving combat: combat-only characters disappear; everyone else in the level is told
// whether this one stopped at a waypoint.
void CombatPlaceholderExit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    c->unkC82 = 1;
    if (c->path == NULL) return;
    PathSetMoving(c->path, FALSE);

    if (c->def->flags & 0x4000) {
        Kill(c);
        return;
    }
    Character** chars = (Character**)(gGame + 0x140);
    for (int i = 0; i < 32; i++) {
        Character* e = chars[i];
        if (e == NULL || !(e->def->stateFlags & 1) || !(e->def->flags & 0x4000)) continue;
        if (PathAtWaypoint(c->path)) EventSend(gEventMgr, e, NULL, 0x33EE55C6, 0, 0, 0, 0);
        else                         EventSend(gEventMgr, e, NULL, 0x00F4D7AB, 0, 0, 0, 0);
    }
}

// 0x80014F04
void MovingChallengeFaceCamera(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    const Vec3* cam = *(Vec3**)(*(u8**)(gScriptCtx + 4) + 4);
    Vec3 d = { cam->x - c->def->pos.x, cam->y - c->def->pos.y, cam->z - c->def->pos.z };
    Vec3 dir = Vec3Normalized(d);
    CharacterFaceDirection(c, &dir, TRUE);
}

// 0x800160C4
// Combat starts when an active enemy comes within sqrt(2) units of the exploring player.
BOOL PlayerInCombatRange(ScriptCtx* ctx) {
    BOOL inRange = FALSE;
    Character* c = ctx->chr;
    Character* player = (Character*)PLAYER;
    if (c == NULL || player == NULL) return FALSE;
    if (fn_8004D170(player) || !c->unkBDF || fn_800E6BD4() || !fn_80050F88()) return FALSE;
    if (WorldGetPhase(WORLD) != 2) return FALSE;

    u32 f = c->def->stateFlags;
    if ((f & (CHAR_BOSS | CHAR_DEAD)) || !(f & 1)) return FALSE;

    const Vec3& p = player->def->pos;
    Vec3 d = { p.x - c->def->pos.x, p.y - c->def->pos.y, p.z - c->def->pos.z };
    if (Vec3LengthSq(d) < 2.0f) {
        *(s32*)(gSaveSystem + 0x138) = -1;
        player->unkC80 = 1;
        inRange = TRUE;
    }
    return inRange;
}

// Not yet decompiled callbacks.
void PatrolIdleExit(ScriptCtx* ctx);
void PumpkinShakeEntry(ScriptCtx* ctx);
void PumpkinShakeExit(ScriptCtx* ctx);
void PumpkinShakeUpdate(ScriptCtx* ctx);
void ObstacleShakeEntry(ScriptCtx* ctx);
void ObstacleShakeUpdate(ScriptCtx* ctx);

// 0x80014350
void RegisterAiStateCallbacks(void) {
    RegisterScriptFunc("PatrolIdleEntry", 0x4E, (void*)PatrolIdleEntry);
    RegisterScriptFunc("PatrolIdleExit", 0x4E, (void*)PatrolIdleExit);
    RegisterScriptFunc("PatrolIdleUpdate", 0x4E, (void*)PatrolIdleUpdate);
    RegisterScriptFunc("CombatPlaceholderEntry", 0x4E, (void*)CombatPlaceholderEntry);
    RegisterScriptFunc("CombatPlaceholderExit", 0x4E, (void*)CombatPlaceholderExit);
    RegisterScriptFunc("CombatPlaceholderUpdate", 0x4E, (void*)CombatPlaceholderUpdate);
    RegisterScriptFunc("EntityReverseDirection", 0x4E, (void*)EntityReverseDirection);
    RegisterScriptFunc("EntityWaitEntry", 0x4E, (void*)EntityWaitEntry);
    RegisterScriptFunc("EntityWaitExit", 0x4E, (void*)EntityWaitExit);
    RegisterScriptFunc("EntityWaitUpdate", 0x4E, (void*)EntityWaitUpdate);
    RegisterScriptFunc("PumpkinWaitEntry", 0x4E, (void*)PumpkinWaitEntry);
    RegisterScriptFunc("PumpkinWaitUpdate", 0x4E, (void*)PumpkinWaitUpdate);
    RegisterScriptFunc("PumpkinFreezeEntry", 0x4E, (void*)PumpkinFreezeEntry);
    RegisterScriptFunc("PumpkinFreezeExit", 0x4E, (void*)PumpkinFreezeExit);
    RegisterScriptFunc("PumpkinShakeEntry", 0x4E, (void*)PumpkinShakeEntry);
    RegisterScriptFunc("PumpkinShakeExit", 0x4E, (void*)PumpkinShakeExit);
    RegisterScriptFunc("PumpkinShakeUpdate", 0x4E, (void*)PumpkinShakeUpdate);
    RegisterScriptFunc("ObstacleShakeEntry", 0x4E, (void*)ObstacleShakeEntry);
    RegisterScriptFunc("ObstacleShakeExit", 0x4E, (void*)ObstacleShakeExit);
    RegisterScriptFunc("ObstacleShakeUpdate", 0x4E, (void*)ObstacleShakeUpdate);
    RegisterScriptFunc("PumpkinEscapeEntry", 0x4E, (void*)PumpkinEscapeEntry);
    RegisterScriptFunc("PumpkinEscapeExit", 0x4E, (void*)PumpkinEscapeExit);
    RegisterScriptFunc("PumpkinEscapeUpdate", 0x4E, (void*)PumpkinEscapeUpdate);
    RegisterScriptFunc("PumpkinDeathEntry", 0x4E, (void*)PumpkinDeathEntry);
    RegisterScriptFunc("PumpkinDeathExit", 0x4E, (void*)PumpkinDeathExit);
    RegisterScriptFunc("PumpkinDeathUpdate", 0x4E, (void*)PumpkinDeathUpdate);
    RegisterScriptFunc("HazardDeathEntry", 0x4E, (void*)HazardDeathEntry);
    RegisterScriptFunc("HazardDustCloudUpdate", 0x4E, (void*)HazardDustCloudUpdate);
    RegisterScriptFunc("HazardPoisonGasUpdate", 0x4E, (void*)HazardPoisonGasUpdate);
    RegisterScriptFunc("MovingChallengeFaceCamera", 0x4E, (void*)MovingChallengeFaceCamera);
    RegisterScriptFunc("MovingChallengeGameEntry", 0x4E, (void*)MovingChallengeGameEntry);
    RegisterScriptFunc("MovingChallengeGameExit", 0x4E, (void*)MovingChallengeGameExit);
    RegisterScriptFunc("PlayerInCombatRange", 0x1D, (void*)PlayerInCombatRange);
}
