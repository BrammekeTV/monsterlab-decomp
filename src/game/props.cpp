// Prop and hazard-prop script functions (prop_*, hazard_prop_*, *_deathcount, triggers).
// Original objects: .text 0x800197A8 - 0x80019F90
#include "game/character.h"
#include "game/game.h"

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);   // 0x801DC91C
void  TriggerShowProximityText(void* trigger, s32 textId);        // 0x8006ED68
void  PropPlayAnim(void* prop, int a, int b, BOOL once);          // 0x80069350
void  PropPlayFx(void* prop, void* fx);                           // 0x801254F8
void  PropIncrementState(void* prop);                             // 0x80069FDC
void  TriggerDoAction(void* action, s32 param);                   // 0x801E0054
void  FollowWaypointPath(void* prop, void* params, f32 speed);
struct ScopedLock { u8 pad[4]; };
void  ScopedLockCtor(ScopedLock* l);                              // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                   // 0x801F53B0
void  WaypointParamsInit(void* params, int a);                    // 0x801ED314
}
extern void* gEventMgr;            // 0x805FB6C8

// Script context timer (seconds) used by prop_counting_down / prop_check_timer.
#define CTX_TIMER(ctx) (*(f32*)((u8*)(ctx) + 0x2C))

struct Prop {
    void** vtbl;
    CharacterDef* def;       // 0x004
    u8   pad008[0x1C];
    f32  dt;                 // 0x024
    u8   pad028[0x74];
    u8*  model;              // 0x09C: +0xB4 fx
    u8*  anim;               // 0x0A0: +0xF0 flags (0x10 = done)
    u8   pad0A4[0xB8];
    u32  doneFlags;          // 0x15C
    u8   pad160[4];
    f32  deathCount;         // 0x164
    s32  proximityText;      // 0x168
    u8   pad16C[0x40];
    s32  actionParam;        // 0x1AC
    u8   pad1B0[0x20];
    u8   action[0x94];       // 0x1D0
    f32  hazardTimer;        // 0x264
    s32  attacking;          // 0x268
    u8   pad26C[8];
    f32  idleTime;           // 0x274: seconds before attacking
    f32  attackTime;         // 0x278: seconds of attack
};
#define PROP(ctx) ((Prop*)(ctx)->chr)

// 0x80019930
void hazard_prop_idle_entry(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL) return;
    p->hazardTimer = 0.0f;
    p->attacking = 0;
}

// 0x80019950
void hazard_prop_idle_exit(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x80019954
void hazard_prop_idle_update(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL) return;
    p->hazardTimer += p->dt;
    if (p->hazardTimer >= p->idleTime) EventSend(gEventMgr, p, NULL, 0x1FF53675, 0, 0, 0, 0);
}

// 0x800199A8
void hazard_prop_attack_entry(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL) return;
    p->hazardTimer = 0.0f;
    p->attacking = 1;
}

// 0x800199C8
void hazard_prop_attack_exit(ScriptCtx* ctx) {
    (void)ctx;
}

// 0x800199CC
void hazard_prop_attack_update(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL) return;
    p->hazardTimer += p->dt;
    if (p->hazardTimer >= p->attackTime) EventSend(gEventMgr, p, NULL, 0xE9BAFAD5, 0, 0, 0, 0);
}

// 0x80019A20
void start_trigger_proximity_text(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL) return;
    TriggerShowProximityText(p, p->proximityText);
}

// 0x80019A38
void prop_play_anim_and_fx_once(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL) return;
    PropPlayAnim(p, 0, 0, TRUE);
    if (p->model != NULL && *(void**)(p->model + 0xB4) != NULL) PropPlayFx(p, *(void**)(p->model + 0xB4));
}

// 0x80019A9C
void prop_counting_down(ScriptCtx* ctx) {
    f32 t = 0.0f;
    if (PROP(ctx) != NULL) t = CTX_TIMER(ctx) - PROP(ctx)->dt;
    CTX_TIMER(ctx) = t;
}

// 0x80019AD8
BOOL prop_check_timer(ScriptCtx* ctx) {
    return CTX_TIMER(ctx) <= 0.0f;
}

typedef void (*SetHiddenFn)(Prop*, int);

// 0x80019B04
void prop_deactivate(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL) return;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    ((SetHiddenFn)p->vtbl[0x18 / 4])(p, 1);
    ScopedLockDtor(&lock, -1);
}

// 0x80019B60
BOOL prop_anim_done_80019B60(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL || p->anim == NULL || !(*(u32*)(p->anim + 0xF0) & 0x10)) return FALSE;
    p->doneFlags |= 1;
    return TRUE;
}

// 0x80019BA0
void increment_entity_state(ScriptCtx* ctx) {
    if (PROP(ctx) == NULL) return;
    PropIncrementState(PROP(ctx));
}

// 0x80019BB4
void reset_deathcount(ScriptCtx* ctx) {
    if (PROP(ctx) == NULL) return;
    PROP(ctx)->deathCount = 0.0f;
}

// 0x80019BCC
void increment_deathcount(ScriptCtx* ctx) {
    if (PROP(ctx) == NULL) return;
    PROP(ctx)->deathCount += 1.0f;
}

// 0x80019BEC
void do_trigger_action(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    if (p == NULL || !(p->def->flags & 0x100)) return;
    TriggerDoAction(p->action, p->actionParam);
}

// Waypoint-follow parameters built on the stack (4 bytes).
struct WaypointParams {
    u8  loop;       // 0x0
    u8  path;       // 0x1
    u16 start;      // 0x2
};

// 0x80019C18
// Second "prop_anim_done" registration: start following the prop's waypoint path at speed 2.
void prop_anim_done_80019C18(ScriptCtx* ctx) {
    Prop* p = PROP(ctx);
    WaypointParams wp;
    wp.start = 0xFFFF;
    wp.loop = 0;
    wp.path = 0;
    wp.path = (u8)*(u16*)((u8*)p->def + 0x32);
    wp.start = *((u8*)p->def + 0x3A);
    WaypointParamsInit(&wp, 2);
    WaypointParams copy = wp;
    FollowWaypointPath(p, &copy, 2.0f);
}

// 0x800197A8
void RegisterPropScriptFuncs(void) {
    RegisterScriptFunc("prop_play_anim_and_fx_once", 0x4E, (void*)prop_play_anim_and_fx_once);
    RegisterScriptFunc("prop_deactivate", 0x4E, (void*)prop_deactivate);
    RegisterScriptFunc("prop_counting_down", 0x4E, (void*)prop_counting_down);
    RegisterScriptFunc("prop_check_timer", 0x1D, (void*)prop_check_timer);
    RegisterScriptFunc("prop_anim_done", 0x1D, (void*)prop_anim_done_80019B60);
    RegisterScriptFunc("prop_anim_done", 0x4E, (void*)prop_anim_done_80019C18);
    RegisterScriptFunc("reset_deathcount", 0x4E, (void*)reset_deathcount);
    RegisterScriptFunc("increment_deathcount", 0x4E, (void*)increment_deathcount);
    RegisterScriptFunc("increment_entity_state", 0x4E, (void*)increment_entity_state);
    RegisterScriptFunc("do_trigger_action", 0x4E, (void*)do_trigger_action);
    RegisterScriptFunc("start_trigger_proximity_text", 0x4E, (void*)start_trigger_proximity_text);
    RegisterScriptFunc("hazard_prop_idle_entry", 0x4E, (void*)hazard_prop_idle_entry);
    RegisterScriptFunc("hazard_prop_idle_exit", 0x4E, (void*)hazard_prop_idle_exit);
    RegisterScriptFunc("hazard_prop_idle_update", 0x4E, (void*)hazard_prop_idle_update);
    RegisterScriptFunc("hazard_prop_attack_entry", 0x4E, (void*)hazard_prop_attack_entry);
    RegisterScriptFunc("hazard_prop_attack_exit", 0x4E, (void*)hazard_prop_attack_exit);
    RegisterScriptFunc("hazard_prop_attack_update", 0x4E, (void*)hazard_prop_attack_update);
}

// ---------------------------------------------------------------- delayed zone events
// A trigger can send up to three events to its zone, each after its own delay.

extern "C" void TriggerSendToZone(void* trigger, u32 event);   // 0x8006EBC4

struct DelayedTrigger {
    void** vtbl;
    CharacterDef* def;       // 0x004
    u8   pad008[0x20];
    f32  now;                // 0x028: trigger clock
    u8   pad02C[0x78];
    u8** stateMachine;       // 0x0A4
    u8   pad0A8[0xB0];
    f32  delay[3];           // 0x158
    f32  fireAt;             // 0x164
    u32  event[3];           // 0x168
    s32  stage;              // 0x174
};

#define EV_DEACTIVATE   0xF5A9CE6Cu   // "Deactivate"
#define STATE_DISABLED  0xBD1B1FC9u

static inline u32 CurrentStateId(DelayedTrigger* t) {
    struct Machine { struct { u8* entries; }* table; s32 current; };
    Machine* m = (Machine*)*t->stateMachine;
    u8* entry = (m->current > -1) ? m->table->entries + m->current * 0x30 : NULL;
    return entry ? *(u32*)(entry + 4) : 0;
}

// 0x80019D30
BOOL check_delayed_timer(DelayedTrigger* t) {
    if (t == NULL || !(t->def->flags & 0x100)) return FALSE;
    if (t->stateMachine != NULL && *t->stateMachine != NULL && CurrentStateId(t) == STATE_DISABLED) return FALSE;
    return t->fireAt < t->now;
}

static inline void SignalDone(DelayedTrigger* t) {
    if (t->stateMachine != NULL && *t->stateMachine != NULL) EventSend(gEventMgr, t, t, EV_DEACTIVATE, 0, 0, 1, 0);
}

// 0x80019DCC
void send_delayed_event_to_zone(DelayedTrigger* t) {
    if (t == NULL || !(t->def->flags & 0x100)) return;
    switch (t->stage) {
    case 0:
        TriggerSendToZone(t, t->event[0]);
        t->stage = 1;
        t->fireAt = t->delay[1] + t->now;
        if (t->event[1] == 0) SignalDone(t);
        break;
    case 1:
        TriggerSendToZone(t, t->event[1]);
        t->stage = 2;
        t->fireAt = t->delay[2] + t->now;
        if (t->event[2] == 0) SignalDone(t);
        break;
    case 2:
        TriggerSendToZone(t, t->event[2]);
        t->stage = 3;
        SignalDone(t);
        break;
    }
}

// 0x80019F68
void reset_trigger_send_delayed_events(ScriptCtx* ctx) {
    DelayedTrigger* t = (DelayedTrigger*)ctx->chr;
    if (t == NULL) return;
    t->fireAt = t->delay[0] + t->now;
    t->stage = 0;
}

// 0x80019CC8
void RegisterDelayedEventFuncs(void) {
    RegisterScriptFunc("check_delayed_timer", 0x16, (void*)check_delayed_timer);
    RegisterScriptFunc("send_delayed_event_to_zone", 0x2F, (void*)send_delayed_event_to_zone);
    RegisterScriptFunc("reset_trigger_send_delayed_events", 0x4E, (void*)reset_trigger_send_delayed_events);
}
