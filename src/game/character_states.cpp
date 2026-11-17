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
void  fn_80016FD0(Character* c, int animate);
void  ExplorationPromptClear(BOOL animate);            // 0x80016EA0
void  GameFlowSignal(s32 id, s32 a, s32 b);             // 0x8000C7F4
void  PlayCharacterAnimation(Character* c, int a, u32 anim, int loop);
void  SpeedUpNISCameraAnimation_8005100C(void);
BOOL  PathIsInConversation(PathFollower* p);            // 0x800D5964
u32   PadGetButtons(void* pad);                         // 0x801B9B40
s32   PathGetNodeId(PathFollower* p);                   // 0x800D56F8
void  SessionDisableFor(void* session, s32 id, f32 time);   // 0x80012820
void  CharacterSetInCombat(Character* c, BOOL on);      // 0x8004D2B8
BOOL  CharacterStartAttack(Character* c, int a);        // 0x8004D520
BOOL  CharacterAttackDone(Character* c);                // 0x8004D5BC
void  WorldSendEvent(void* world, u32 event, s32 from, s32 arg, BOOL queue);   // 0x800D87EC
Vec3  PathGetDirection(PathFollower* p);                // 0x800D5724
void  CharacterFaceDirection(Character* c, const Vec3* dir, BOOL now);   // 0x8005B650
void  ResumeAllExplorationCharacters(void);             // 0x80016DB0
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);   // 0x801DC91C
}

extern void* gEventMgr;            // 0x805FB6C8
extern u8    gGame[];              // 0x803B53A0
extern u8    gScriptCtx[];         // 0x803C4298
extern u8*   gBattle;              ORIG(0x805FBBB8)
extern u8*   lbl_805FBBC8;
extern u8*   gSaveSystem;          // 0x805FBBE0
extern void* gObjDb;               // 0x805FB180

typedef void* (*GetFighterFn)(Character*);

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
        ExplorationPromptClear(FALSE);
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
    ResumeAllExplorationCharacters();
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

#define EV_RESUME_MOVE 0x00F4D7ABu

// 0x80016CEC
extern "C" void PauseAllExplorationEnemies(void) {
    PauseOtherCharacters();
}

// 0x80016DB0
// Resume every paused moving character: those standing on a waypoint go back to idling,
// the others continue walking.
extern "C" void ResumeAllExplorationCharacters(void) {
    for (int i = 0; i < 32; i++) {
        Character* e = CHARACTERS[i];
        if (e == NULL || !(e->def->stateFlags & 1) || !(e->def->flags & 0x10)) continue;
        if (e == PLAYER || e->path == NULL) continue;
        if (PathAtWaypoint(e->path)) Send(e, EV_RESUME);
        else                         Send(e, EV_RESUME_MOVE);
    }
}

// 0x80017E5C
void character_challenge_exit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    if (lbl_805FBBC8[0x6C]) WorldSendEvent(WORLD, 0xACA6B62Cu, 0, 0, 0);   // challenge won
    else                    WorldSendEvent(WORLD, 0xADCA036Fu, 0, 0, 0);   // challenge lost
    ResumeAllExplorationCharacters();
    GameFlowSignal(0xACE44170, 0, 0);
    fn_80016FD0(c, 1);
}

// 0x80018174
void character_Interaction_exit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    ResumeAllExplorationCharacters();
    fn_80016FD0(c, 0);
    if (c->eventTarget != 0) {
        u8* hud = *(u8**)(gGame + 0xD634);
        hud[0x430] = 1;      // fade the HUD back in
        c->eventTarget = 0;
    }
}

// 0x800181E0
// While walking between waypoints, keep facing the walking direction.
void character_Interaction_update(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL || PathAtWaypoint(c->path)) return;
    c->def->pos = *PathGetPosition(c->path);
    Vec3 dir = PathGetDirection(c->path);
    if (dir.x * dir.x + dir.y * dir.y + 0.0f * 0.0f > 0.01f) {
        Vec3 d = PathGetDirection(c->path);
        CharacterFaceDirection(c, &d, FALSE);
    }
}

// 0x800186D8
BOOL target_node_reached(ScriptCtx* ctx) {
    BOOL reached = FALSE;
    Character* c = ctx->chr;
    if (c == NULL) return FALSE;
    Vec3 d = { c->targetPos.x - c->def->pos.x, c->targetPos.y - c->def->pos.y, c->targetPos.z - c->def->pos.z };
    if (c->path != NULL) reached = PathAtWaypoint(c->path);
    if (gObjDb == NULL && Vec3LengthSq(d) <= 0.01f) reached = TRUE;
    return reached;
}

extern "C" {
void  ControlsLock(void* player, int a);                // 0x8004CD28
void  ControlsUnlock(void* player);                     // 0x8004CE44
void  CharacterResetRace(Character* c);                 // 0x800534C0
void  CharacterSetBlend(Character* c, f32 t);           // 0x8004D248
void  UIScreen_Push(u32 hash);
void  UIScreen_PopScreen(u32 hash);
void  UIElement_SetVisibleFlag(u32 hash, BOOL on);
void  UISlider_SetValue(u32 hash, f32 value);
BOOL  PathIsAtJunction(PathFollower* p);                // 0x800D5924
void  HudUpdateScreens(void* screens, f32 dt);          // 0x801BB5BC
void  PlayFootStepSound(void* sfx);
}

#define RACE_WALK_SPEED 6.0f

// 0x80017930
// Races start from a standstill: the player (Wii Remote driven) begins at speed 0.
void character_race_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    CharacterResetRace(c);
    c->pathSpeed = 0.0f;
    *(s32*)(**(u8***)((u8*)c + 0xA4) + 0x2C) = 0;
    c->waitTimer = 0.0f;
    ControlsLock(c, 0);
    *((u8*)c + 0xC81) = 1;                       // racing
    PathSetSpeed(c->path, 0.0f);
    ExplorationPromptClear(FALSE);
    if (c->pathSpeed > 0.0f) PlayCharacterAnimation(c, 1, 0xF5770040 /* "explore1" */, 1);

    UIScreen_Push(0xE5BF8E82);
    UIElement_SetVisibleFlag(0x552FC6C7, TRUE);
    UIElement_SetVisibleFlag(0xC4E94BB0, FALSE);
    UIElement_SetVisibleFlag(0xB2601D05, FALSE);
    UIElement_SetVisibleFlag(0x1A9EE60F /* "Wiimote_down" */, TRUE);
    UIElement_SetVisibleFlag(0x8D6D65B1 /* "Wiimote_up" */, FALSE);
}

// 0x80017A44
void character_race_exit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    c->pathSpeed = RACE_WALK_SPEED;
    ControlsUnlock(c);
    *((u8*)c + 0xC81) = 0;
    CharacterSetBlend(c, 1.0f);
    PathSetSpeed(c->path, RACE_WALK_SPEED);
    UISlider_SetValue(0xA90A6046, 0.0f);
    UIElement_SetVisibleFlag(0x552FC6C7, FALSE);
    UIScreen_PopScreen(0xE5BF8E82);
}

// 0x80017308
// Arriving at a waypoint: report it to the world (the player's arrival also updates the HUD),
// snap to the node and play a footstep.
void character_exploration_move_exit(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || c->path == NULL || !PathAtWaypoint(c->path)) return;

    if (c == PLAYER) {
        if (PathIsAtJunction(c->path)) fn_80016FD0(c, 0);
        *(s32*)(gSaveSystem + 0x100) = 0;
        WorldSendEvent(WORLD, 0x6E9F46BC, c->id, PathGetNodeId(c->path), 1);
        u8* hud = *(u8**)(gGame + 0xD634);
        HudUpdateScreens(*(void**)(hud + 0x434), c->dt);
    } else {
        WorldSendEvent(WORLD, 0x00C72F9E, c->id, PathGetNodeId(c->path), 1);
    }

    c->def->pos = *PathGetPosition(c->path);
    if ((c->def->flags & 0x2000) && c != NULL) {
        void* sfx = *(void**)((u8*)c + 0xC54);
        if (sfx != NULL) PlayFootStepSound(sfx);
    }
}

extern "C" {
u32   PathGetChallengeId(PathFollower* p);              // 0x800D5984: hashed challenge name at the node
u32   StringHash(const char* s);                        // 0x801EB704
void  HudShowConversation(void* hud, s32 conversation); // 0x8001B0B8
}

// 0x80017D28
// Start the challenge configured on the current path node (a node value of "1" means the
// default challenge).
void character_challenge_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    u32 challenge = PathGetChallengeId(c->path);
    if (challenge == StringHash("1")) GameFlowSignal(0xBA5FF08B, 0, 0);
    else if (challenge != 0)          GameFlowSignal(challenge, 0, 0);
    *(s32*)(gSaveSystem + 0x130) = PathGetNodeId(c->path);
    ExplorationPromptClear(FALSE);
    PauseOtherCharacters();
}

// 0x80018044
void character_Interaction_entry(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;
    s32 conversation = (s32)PathIsInConversation(c->path);
    if (conversation == 0) {
        c->eventTarget = 0;
        EventSend(gEventMgr, c, c, 0x2524AD6Du, 0, 0, 0, 0);
        return;
    }
    c->eventTarget = 1;          // remembered so the exit fades the HUD back in
    HudShowConversation(*(void**)(gGame + 0xD634), conversation);
    PauseOtherCharacters();
}

extern "C" {
void* ScreenSetsCurrent(void* sets);                    // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);         // 0x80198E34
BOOL  ScreenSetWidgetVisible(void* set, void* widget);  // 0x80198C34
BOOL  UIElement_IsUIAnimPlaying(u32 hash);
void  ExplorationHudSetFighter(void* set, void* fighter, int a);   // 0x80045138
}
extern void* gScreenSets;          // 0x805FB654

// 0x80017454
void character_exploration_move_update(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;

    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        void* w = ScreenSetFindWidget(set, 0xDB1E4D0C /* "exploration_screen" */);
        if (w != NULL && ScreenSetWidgetVisible(set, w)) {
            u8* pending = (u8*)c + 0x9F8;
            if (*pending) {
                fn_80016FD0(c, 1);
                *pending = 0;
            }
            if (c == PLAYER && !UIElement_IsUIAnimPlaying(0x664BFF35)) {
                ExplorationHudSetFighter(set, VCALL(c, 0xF0, GetFighterFn)(c), 0);
            }
        }
    }

    c->waitTimer += c->dt;
    if (c->path == NULL) return;
    Vec3 dir = PathGetDirection(c->path);
    dir.z = 0.0f;
    if (PathAtWaypoint(c->path) || Vec3LengthSq(dir) <= 0.01f) return;

    c->def->pos = *PathGetPosition(c->path);
    Vec3 face = PathGetDirection(c->path);
    CharacterFaceDirection(c, &face, FALSE);
    c->def->stateFlags |= 0x8000;
    c->def->stateFlags |= 0x10000000;
    c->def->stateFlags |= 0x4000;
    c->def->stateFlags |= 0x10000000;
}

// ---------------------------------------------------------------- exploration prompt
// The exploration screen shows a two-line prompt (header + text) for the node the player
// stands on: a challenge, a shop, a door, ...

extern "C" {
void  UIElement_PlayUIAnim(u32 element, u32 anim);
void  UIEvent_SendToScreen(u32 event, u32 screen);
void  UIText_SetString_TextID(u32 hash, s32 textId);
void* UIWidgetFindChild(void* widget, u32 hash);        // 0x80199AEC
const char* UITextGetString(void* text);                // 0x8019F3AC
s32   PathGetNodeType(PathFollower* p);                 // 0x800D5944
s32   PathGetNodeText(PathFollower* p);                 // 0x800D581C: explicit text id, or 0
void  fn_80044F20(void);
int   strcmp(const char* a, const char* b);
}

#define UI_EXPLORATION_SCREEN 0xDB1E4D0Cu   // "exploration_screen"
#define UI_PROMPT_TEXT        0x640078E4u
#define UI_PROMPT_HEADER      0x9E8A6F47u   // "header"
#define UI_PROMPT_ANIM_TARGET 0x664BFF35u
#define TEXT_NONE             0x557

enum { NODE_CHALLENGE = 2, NODE_TYPE_4 = 4, NODE_TYPE_8 = 8, NODE_TYPE_16 = 16 };

// 0x80016EA0
// Clear the prompt (optionally with the hide animation).
extern "C" void ExplorationPromptClear(BOOL animate) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    void* w = ScreenSetFindWidget(set, UI_EXPLORATION_SCREEN);
    if (w != NULL && animate) UIElement_PlayUIAnim(UI_PROMPT_ANIM_TARGET, 0x784097C0);
    if (!ScreenSetWidgetVisible(set, w)) return;

    void* text = UIWidgetFindChild(w, UI_PROMPT_TEXT);
    void* header = UIWidgetFindChild(w, UI_PROMPT_HEADER);
    BOOL shown = (text != NULL && strcmp(UITextGetString(text), "") != 0) ||
                 (header != NULL && strcmp(UITextGetString(header), "") != 0);
    if (!shown) return;

    UIEvent_SendToScreen(0x798F99BC, UI_EXPLORATION_SCREEN);
    UIText_SetString_TextID(UI_PROMPT_TEXT, TEXT_NONE);
    UIText_SetString_TextID(UI_PROMPT_HEADER, TEXT_NONE);
}

// Prompt text per challenge (StringHash of the challenge id stored on the node).
static inline s32 ChallengePromptText(u32 challenge) {
    switch (challenge) {
    case 0xA41D84B9: return 0x3CB;
    case 0xC4CE4FC3: return 0x3C1;
    case 0x711A757D: return 0x3C5;
    case 0x0144CE0D: return 0x3D1;
    case 0x1EF3AA63: return 0x3C3;
    case 0xF3CC643A: return 0x3CD;
    case 0x01C00E16: return 0x3DC;
    case 0x4B9FE6EB: return 0x3C2;
    case 0xBAC3287F: return 0x3D6;
    case 0x5D664B82: return 0x3DA;
    }
    return TEXT_NONE;
}

// 0x80016FD0
// Show the prompt for the node `c` is on. If the exploration screen isn't up yet, remember
// to do it later (character_exploration_move_update picks the flag up).
extern "C" void fn_80016FD0(Character* c, int animate) {
    if (c == NULL || fn_800E6BD4()) return;

    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        void* w = ScreenSetFindWidget(set, UI_EXPLORATION_SCREEN);
        if (w == NULL || !ScreenSetWidgetVisible(set, w)) {
            *((u8*)c + 0x9F8) = 1;
            return;
        }
        if (animate) {
            fn_80044F20();
            UIElement_PlayUIAnim(UI_PROMPT_ANIM_TARGET, 0x4A4CBE89);
        }
    }
    if (c->path == NULL) return;

    s32 text = TEXT_NONE, header = TEXT_NONE;
    BOOL show = FALSE;
    switch (PathGetNodeType(c->path)) {
    case NODE_TYPE_8:
        text = 0x3CC;
        show = TRUE;
        break;
    case NODE_CHALLENGE:
        header = 0x567;
        text = ChallengePromptText((u32)PathGetChallengeId(c->path));
        show = TRUE;
        break;
    case NODE_TYPE_4:
        text = 0x3D8;
        show = TRUE;
        break;
    case NODE_TYPE_16:
        text = 0x3D3;
        show = TRUE;
        break;
    }
    if (!PathIsAtJunction(c->path)) show = FALSE;
    if (PathGetNodeText(c->path) != 0) {
        text = PathGetNodeText(c->path);
        header = TEXT_NONE;
        show = TRUE;
    }
    if (!show) return;

    UIEvent_SendToScreen(0xFE181559, UI_EXPLORATION_SCREEN);
    UIText_SetString_TextID(UI_PROMPT_TEXT, text);
    UIText_SetString_TextID(UI_PROMPT_HEADER, header);
}

extern "C" BOOL ShakePromptBlink(s32 player, f32 phase);   // 0x80015C40

#define RACE_MAX_SPEED 11.0f      // full speed meter

// 0x80017AE0
// Racing along the route: each waypoint reached takes the next route exit; at the end of the
// route the racer stops. The speed meter shows pathSpeed (driven by shaking) out of 11.
void character_race_update(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return;

    void* set = ScreenSetsCurrent(gScreenSets);
    ExplorationHudSetFighter(set, VCALL(c, 0xF0, GetFighterFn)(c), 0);
    c->waitTimer += c->dt;

    if (PathAtWaypoint(c->path)) {
        WorldSendEvent(WORLD, 0x6E9F46BC, 0, 0, 1);
        BOOL atEnd = c->routeLength <= 0 || c->route[c->routeIndex] == -1;
        if (atEnd) {
            if (c->pathSpeed > 0.0f) {
                PlayCharacterAnimation(c, 1, 0xE3712F44 /* "IdleReg" */, 1);
                c->pathSpeed = 0.0f;
                c->def->pos = *PathGetPosition(c->path);
                CharacterSetBlend(c, 1.0f);
            }
            return;
        }
        if (c->route != NULL) {
            PathTakeExit(c->path, c->route[c->routeIndex], 0);
            c->routeIndex++;
        }
    }

    UIElement_SetVisibleFlag(0x552FC6C7, TRUE);
    UISlider_SetValue(0xA90A6046, c->pathSpeed / RACE_MAX_SPEED);
    if (ShakePromptBlink(0, c->waitTimer) && ShakePromptBlink(1, c->waitTimer)) c->waitTimer = 0.0f;

    Vec3 dir = PathGetDirection(c->path);
    dir.z = 0.0f;
    if (c->path == NULL || PathAtWaypoint(c->path) || Vec3LengthSq(dir) <= 0.01f) return;
    c->def->pos = *PathGetPosition(c->path);
    Vec3 face = PathGetDirection(c->path);
    CharacterFaceDirection(c, &face, FALSE);
}

// Not yet decompiled callbacks.
BOOL target_node_selected(ScriptCtx* ctx);
BOOL target_node_changed(ScriptCtx* ctx);
void character_exploration_idle_update(ScriptCtx* ctx);

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
