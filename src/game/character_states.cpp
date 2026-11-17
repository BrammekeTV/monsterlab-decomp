// Character state callbacks: character_combat_*, character_exploration_*, character_race_*,
// character_challenge_*, character_conversation_*, character_Interaction_*, target_node_* and
// CharacterCombat* (registered in RegisterCharacterStateCallbacks, 0x80016400).
// Original object: .text 0x80016400 - 0x80018C04 (auto_03_80016400_text.s)
#include "game/character.h"
#include "game/game.h"

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32   WorldGetPhase(void* world);                       // 0x800D83A8
BOOL  fn_800E6BD4(void);
void  fn_80016FD0(Character* c, int a);
void  fn_80016EA0(int);
void  GameFlowSignal(s32 id, s32 a, s32 b);             // 0x8000C7F4
void  PlayCharacterAnimation(Character* c, int a, u32 anim, int loop);
void  SpeedUpNISCameraAnimation_80016DB0(Character* c);
void  SpeedUpNISCameraAnimation_8005100C(void);
BOOL  PathIsInConversation(PathFollower* p);            // 0x800D5964
u32   PadGetButtons(void* pad);                         // 0x801B9B40
s32   PathGetNodeId(PathFollower* p);                   // 0x800D56F8
void  SessionDisableFor(void* session, s32 id, f32 time);   // 0x80012820
void  CharacterSetInCombat(Character* c, BOOL on);      // 0x8004D2B8
BOOL  CharacterStartAttack(Character* c, int a);        // 0x8004D520
BOOL  CharacterAttackDone(Character* c);                // 0x8004D5BC
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);   // 0x801DC91C
}

extern void* gEventMgr;            // 0x805FB6C8
extern u8    gGame[];              // 0x803B53A0
extern u8    gScriptCtx[];         // 0x803C4298
extern u8*   gBattle;              ORIG(0x805FBBB8)

#define WORLD       (gGame + 0xD5F0)
#define PLAYER      ((Character*)*(void**)(gGame + 0xD614))
#define CHARACTERS  ((Character**)(gGame + 0x140))     // 32 slots

// Event ids (StringHash values; names unknown).
#define EV_PAUSE          0x4F9213C1u
#define EV_RESUME         0x33EE55C6u
#define EV_COMBAT_IDLE    0x58DED561u
#define EV_ACTION_DONE    0xC7EA30FBu
#define ID_NO_COMBAT_IDLE 0xA9638202u   // character id that never leaves combat idle

enum { WORLD_PHASE_EXPLORE = 2, WORLD_PHASE_4 = 4, WORLD_PHASE_6 = 6 };

static inline void Send(Character* to, u32 ev) {
    EventSend(gEventMgr, to, NULL, ev, 0, 0, 0, 0);
}

// Pause every other active moving character that walks a path.
static inline void PauseOtherCharacters(void) {
    for (int i = 0; i < 32; i++) {
        Character* e = CHARACTERS[i];
        if (e == NULL || !(e->def->stateFlags & 1) || !(e->def->flags & 0x10)) continue;
        if (e == PLAYER || e->path == NULL) continue;
        Send(e, EV_PAUSE);
    }
}

// 0x80016788
void character_combat_idle_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    s32 phase = WorldGetPhase(WORLD);
    if (phase == WORLD_PHASE_EXPLORE || WorldGetPhase(WORLD) == WORLD_PHASE_4) {
        if ((u32)c->id == ID_NO_COMBAT_IDLE) return;
        Send(c, EV_COMBAT_IDLE);
        if (c == PLAYER && !fn_800E6BD4()) fn_80016FD0(c, 1);
    } else if (WorldGetPhase(WORLD) == WORLD_PHASE_6) {
        if (gScriptCtx[0x1C] != 0x1C) Send(c, 0x5ED5DBDFu);
        else                          Send(c, 0xAD86D6D5u);
    }
}

// 0x800168D8
void character_combat_idle_exit(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x800168DC
void character_combat_idle_update(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (WorldGetPhase(WORLD) != WORLD_PHASE_EXPLORE) return;
    if ((u32)c->id == ID_NO_COMBAT_IDLE) return;
    Send(c, EV_COMBAT_IDLE);
}

// 0x80016960
void character_exploration_idle_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    c->waitTimer = 0.0f;
    c->velocity.x = c->velocity.y = c->velocity.z = 0.0f;
    c->unkC80 = 0;
}

// 0x800169A0
void character_exploration_idle_exit(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80017290
void character_exploration_move_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (c->path != NULL) {
        PathSetSpeed(c->path, c->pathSpeed);
        PathSetMoving(c->path, FALSE);
    }
    if (c == PLAYER) {
        fn_80016EA0(0);
        c->unkC80 = 0;
    }
}

// 0x80017620
void character_exploration_exit_entry(ScriptCtx* ctx) {
    if (ctx->chr == NULL) return;
    GameFlowSignal(0x06B0A31C, 0, 0);
    PauseOtherCharacters();
}

// 0x80017704
void character_exploration_wait_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (c->path != NULL) PathSetMoving(c->path, TRUE);
    if (c->def->flags & 0x2000) PlayCharacterAnimation(c, 1, 0xE3712F44 /* "IdleReg" */, 1);
}

// 0x80017770
void character_exploration_wait_exit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (c == PLAYER && !fn_800E6BD4()) fn_80016FD0(c, 1);
    if (c->path != NULL && PathAtWaypoint(c->path)) PathSetMoving(c->path, FALSE);
}

// 0x800177F4
void character_exploration_injure_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL) return;
    PathSetMoving(c->path, TRUE);
}

// 0x80017818
void character_exploration_injure_exit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL) return;
    if (PathAtWaypoint(c->path)) PathSetMoving(c->path, FALSE);
}

// 0x8001786C
void character_challenge_instruction_entry(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80017870
void character_challenge_instruction_exit(ScriptCtx* ctx) {
    (void)ctx;
}

#define PAD_BUTTON_A 0x40
#define PAD_BUTTON_B 0x80

// 0x80017874
// Challenge instructions: A accepts (the challenger steps aside for 65 s), B declines.
void character_challenge_instruction_update(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    u32 buttons = PadGetButtons(*(void**)(gScriptCtx + 0x38));
    if (buttons & PAD_BUTTON_A) {
        SessionDisableFor(gGame + 0x5C8, PathGetNodeId(c->path), 65.0f);
        Send(c, 0x74027504u);
    } else if (buttons & PAD_BUTTON_B) {
        Send(c, EV_RESUME);
    }
}

// 0x80017F14
void character_challenge_update(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80017F18
void character_conversation_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (!PathIsInConversation(c->path)) {
        EventSend(gEventMgr, c, c, 0x6877205Cu, 0, 0, 0, 0);
        return;
    }
    SpeedUpNISCameraAnimation_8005100C();
    PauseOtherCharacters();
}

// 0x8001802C
void character_conversation_exit(ScriptCtx* ctx) {
    if (ctx->chr == NULL) return;
    SpeedUpNISCameraAnimation_80016DB0(ctx->chr);
}

// 0x80018040
void character_conversation_update(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80018A34
void CharacterCombatIdleEntry(ScriptCtx* ctx) {
    if (ctx->chr == NULL) return;
    CharacterSetInCombat(ctx->chr, TRUE);
}

// 0x80018A4C
void CharacterCombatIdleExit(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80018A50
void CharacterCombatIdleUpdate(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80018A54
void CharacterCombatWaitForActionUpdate(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80018A58
void CharacterCombatActionEntry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (!CharacterStartAttack(c, 0)) Send(c, EV_ACTION_DONE);
}

typedef void* (*GetFighterFn)(Character*);

// 0x80018AC4
// Tell the opponent (the other fighter in gBattle) that our action finished.
void CharacterCombatActionExit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    void* me = VCALL(c, 0xF0, GetFighterFn)(c);
    u8* other = *(u8**)(gBattle + 0xA00);
    if (other == me) other = *(u8**)(gBattle + 0xA04);
    u8* owner = *(u8**)(other + 0x1C);
    Character* target = owner ? (Character*)(owner - 0xC48) : NULL;
    EventSend(gEventMgr, target, NULL, 0x72397992u, 0, 0, 0, 0);
}

// 0x80018B44
void CharacterCombatActionUpdate(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (CharacterAttackDone(c)) Send(c, EV_ACTION_DONE);
}

// 0x80018BAC
void CharacterCombatReactionEntry(ScriptCtx* ctx) {
    if (ctx->chr == NULL) return;
    CharacterSetInCombat(ctx->chr, TRUE);
}

// 0x80018BC4
void CharacterCombatReactionUpdate(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80018BC8
BOOL CharacterPreviewerActionDone(ScriptCtx* ctx) {
    if (ctx->chr == NULL) return TRUE;
    return CharacterAttackDone(ctx->chr);
}

// 0x80018BE0
void CharacterPreviewerIdleEntry(ScriptCtx* ctx) {
    (void)ctx;
}

// Not yet decompiled callbacks.
BOOL target_node_selected(ScriptCtx* ctx);
BOOL target_node_changed(ScriptCtx* ctx);
BOOL target_node_reached(ScriptCtx* ctx);
void character_exploration_idle_update(ScriptCtx* ctx);
void character_exploration_move_exit(ScriptCtx* ctx);
void character_exploration_move_update(ScriptCtx* ctx);
void character_race_entry(ScriptCtx* ctx);
void character_race_exit(ScriptCtx* ctx);
void character_race_update(ScriptCtx* ctx);
void character_challenge_entry(ScriptCtx* ctx);
void character_challenge_exit(ScriptCtx* ctx);
void character_Interaction_entry(ScriptCtx* ctx);
void character_Interaction_exit(ScriptCtx* ctx);
void character_Interaction_update(ScriptCtx* ctx);

// 0x80016400
void RegisterCharacterStateCallbacks(void) {
    RegisterScriptFunc("target_node_selected", 0x1D, (void*)target_node_selected);
    RegisterScriptFunc("target_node_changed", 0x1D, (void*)target_node_changed);
    RegisterScriptFunc("target_node_reached", 0x1D, (void*)target_node_reached);
    RegisterScriptFunc("CharacterPreviewerActionDone", 0x1D, (void*)CharacterPreviewerActionDone);
    RegisterScriptFunc("character_combat_idle_entry", 0x4E, (void*)character_combat_idle_entry);
    RegisterScriptFunc("character_combat_idle_exit", 0x4E, (void*)character_combat_idle_exit);
    RegisterScriptFunc("character_combat_idle_update", 0x4E, (void*)character_combat_idle_update);
    RegisterScriptFunc("character_exploration_idle_entry", 0x4E, (void*)character_exploration_idle_entry);
    RegisterScriptFunc("character_exploration_idle_exit", 0x4E, (void*)character_exploration_idle_exit);
    RegisterScriptFunc("character_exploration_idle_update", 0x4E, (void*)character_exploration_idle_update);
    RegisterScriptFunc("character_exploration_move_entry", 0x4E, (void*)character_exploration_move_entry);
    RegisterScriptFunc("character_exploration_move_exit", 0x4E, (void*)character_exploration_move_exit);
    RegisterScriptFunc("character_exploration_move_update", 0x4E, (void*)character_exploration_move_update);
    RegisterScriptFunc("character_race_entry", 0x4E, (void*)character_race_entry);
    RegisterScriptFunc("character_race_exit", 0x4E, (void*)character_race_exit);
    RegisterScriptFunc("character_race_update", 0x4E, (void*)character_race_update);
    RegisterScriptFunc("character_challenge_entry", 0x4E, (void*)character_challenge_entry);
    RegisterScriptFunc("character_challenge_exit", 0x4E, (void*)character_challenge_exit);
    RegisterScriptFunc("character_challenge_update", 0x4E, (void*)character_challenge_update);
    RegisterScriptFunc("character_challenge_instruction_entry", 0x4E, (void*)character_challenge_instruction_entry);
    RegisterScriptFunc("character_challenge_instruction_exit", 0x4E, (void*)character_challenge_instruction_exit);
    RegisterScriptFunc("character_challenge_instruction_update", 0x4E, (void*)character_challenge_instruction_update);
    RegisterScriptFunc("character_conversation_entry", 0x4E, (void*)character_conversation_entry);
    RegisterScriptFunc("character_conversation_exit", 0x4E, (void*)character_conversation_exit);
    RegisterScriptFunc("character_conversation_update", 0x4E, (void*)character_conversation_update);
    RegisterScriptFunc("character_Interaction_entry", 0x4E, (void*)character_Interaction_entry);
    RegisterScriptFunc("character_Interaction_exit", 0x4E, (void*)character_Interaction_exit);
    RegisterScriptFunc("character_Interaction_update", 0x4E, (void*)character_Interaction_update);
    RegisterScriptFunc("character_exploration_wait_entry", 0x4E, (void*)character_exploration_wait_entry);
    RegisterScriptFunc("character_exploration_wait_exit", 0x4E, (void*)character_exploration_wait_exit);
    RegisterScriptFunc("character_exploration_injure_entry", 0x4E, (void*)character_exploration_injure_entry);
    RegisterScriptFunc("character_exploration_injure_exit", 0x4E, (void*)character_exploration_injure_exit);
    RegisterScriptFunc("character_exploration_exit_entry", 0x4E, (void*)character_exploration_exit_entry);
    RegisterScriptFunc("CharacterCombatIdleEntry", 0x4E, (void*)CharacterCombatIdleEntry);
    RegisterScriptFunc("CharacterCombatIdleExit", 0x4E, (void*)CharacterCombatIdleExit);
    RegisterScriptFunc("CharacterCombatIdleUpdate", 0x4E, (void*)CharacterCombatIdleUpdate);
    RegisterScriptFunc("CharacterCombatWaitForActionUpdate", 0x4E, (void*)CharacterCombatWaitForActionUpdate);
    RegisterScriptFunc("CharacterCombatActionEntry", 0x4E, (void*)CharacterCombatActionEntry);
    RegisterScriptFunc("CharacterCombatActionExit", 0x4E, (void*)CharacterCombatActionExit);
    RegisterScriptFunc("CharacterCombatActionUpdate", 0x4E, (void*)CharacterCombatActionUpdate);
    RegisterScriptFunc("CharacterCombatReactionEntry", 0x4E, (void*)CharacterCombatReactionEntry);
    RegisterScriptFunc("CharacterCombatReactionUpdate", 0x4E, (void*)CharacterCombatReactionUpdate);
    RegisterScriptFunc("CharacterPreviewerIdleEntry", 0x4E, (void*)CharacterPreviewerIdleEntry);
}
