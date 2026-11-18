// Core game state block (gGame, 0x803B53A0): reset, two-player turn timers, camera helpers.
// Original object: .text 0x80006B60 - 0x80007A8C (auto_03_80006B60_text.s)
#include "engine/math.h"
#include "game/game.h"

extern "C" {
void  ScreenMgrLoad(void* screens);                 // 0x801BC574
void  HudReset(void* hud);                          // 0x8001AFCC
BOOL  fn_801F5220(void);
void* EventPost(s32 id, s32 flags);                 // 0x801F3918: returns payload
void  ScriptAdvanceTurn(void* script);              // 0x800084B8 (provisional)
void  fn_8001360C(void* p);
void  WorldUpdate(void* world, int a, f32 dt);      // 0x800D83C8
}

extern u8   gScriptCtx[];                    // 0x803C4298
extern u8   gGame[];                         // 0x803B53A0
extern u8   gWorldState[];                   // 0x803E6D00
extern u8   gFlag805FB948;                   ORIG(0x805FB948)  // two-player mode?
extern u8*  gSaveSystem;                     // 0x805FBBE0
extern u8*  lbl_805FBBC8;
extern f32  lbl_805F95A0, lbl_805FB314, lbl_805FB300;
extern u8   gIgcMode, lbl_805FB1A1, lbl_805FB1A0;     // gIgcMode: camera_script.cpp
extern s32  gCameraLocks;
extern void* gCameraOwner;

// Partial layout of gGame.
struct GameState {
    u32   unk000[3];          // 0x000
    u32   unk00C, unk010;     // 0x00C
    u8    unk014;             // 0x014
    u8    pad015[3];
    f32   unk018;             // 0x018
    f32   unk01C;             // 0x01C
    f32   unk020;             // 0x020
    u8    unk024;             // 0x024
    u8    pad025[0x103];
    s32   unk128;             // 0x128
    f32   turnTimer[2];       // 0x12C: per player, < 0 = stopped
    s32   unk134;             // 0x134
    s32   unk138;             // 0x138
    u8    unk13C;             // 0x13C
    u8    pad13D[3];
    u32   unk140[0x20];       // 0x140 - 0x1BC
    u8    pad1C0[0x404];
    u32   flags5C4;           // 0x5C4
};

#define TURN_TIME_LIMIT 6.0f

// 0x80006B60
void HudReload(u8* obj) {
    u8* hud = *(u8**)(obj + 0x14);
    void** screens = *(void***)(hud + 0x434);
    if (screens != NULL && screens[0] != NULL) ScreenMgrLoad(screens);
    HudReset(*(u8**)(obj + 0x14));
}

// 0x80006BB4
extern "C" void GameStateReset(GameState* g) {
    g->unk000[2] = 0;
    g->unk000[0] = 0;
    g->unk000[1] = 0;
    g->unk00C = 0;
    g->unk010 = 0;
    g->unk014 = 100;
    g->unk018 = 0.0f;
    g->unk01C = 1.0f;
    g->flags5C4 &= ~2u;
    g->unk128 = 0;
    g->unk138 = 0;
    g->turnTimer[0] = -1.0f;
    g->turnTimer[1] = -1.0f;
    lbl_805F95A0 = 1.0f;
    lbl_805FB314 = 0.0f;
    gIgcMode = 0;
    lbl_805FB1A1 = 0;
    gCameraLocks = 0;
    gCameraOwner = 0;
    lbl_805FB1A0 = 0;
    g->unk13C = 0;
    for (int i = 0; i < 0x20; i++) g->unk140[i] = 0;
}

// 0x80006CA8
extern "C" void GameStateInit(GameState* g) {
    g->flags5C4 = 0;
    gSaveSystem = (u8*)g + 0xC470;
    lbl_805FBBC8 = (u8*)g + 0xD570;
    g->unk020 = 0.0f;
    g->unk024 = 0;
}

// 0x80006CD4
// Two-player turn timers: once a running timer passes 6 s, the turn is handed over.
void GameUpdateTurnTimers(GameState* g, f32 dt) {
    if (!gFlag805FB948 || !fn_801F5220()) return;

    u8 running[2] = { 0, 0 };
    u8 expired[2] = { 0, 0 };
    for (int i = 0; i < 2; i++) {
        if (g->turnTimer[i] >= 0.0f) {
            g->turnTimer[i] += dt;
            running[i] = 1;
        }
        if (g->turnTimer[i] > TURN_TIME_LIMIT) expired[i] = 1;
    }

    u8* world = *(u8**)gWorldState;
    void* p8 = *(void**)(world + 0x8);
    int cur = (p8 != NULL && *(void**)(world + 0x14) == p8) ? 1 : 0;
    int other = cur == 0 ? 1 : 0;

    if (!expired[cur] && !expired[other]) return;

    if (!gFlag805FB948 || (running[cur] && running[other])) {
        u8* s = (u8*)EventPost(0x22, 0);
        if (s != NULL) *(s32*)(s + 4) = 0;
        ScriptAdvanceTurn(gScriptCtx);
        fn_8001360C(gGame + 0x5C8);
        g->turnTimer[cur] = -1.0f;
        g->turnTimer[other] = -1.0f;
    } else if (running[cur]) {
        ScriptAdvanceTurn(gScriptCtx);
        g->turnTimer[cur] = -1.0f;
    } else if (running[other]) {
        u8* s = (u8*)EventPost(0x22, 0);
        if (s != NULL) *(s32*)(s + 4) = 1;
        g->turnTimer[other] = -1.0f;
    }
}

// 0x80006EF4
extern "C" void GameUpdateWorld(GameState* g, f32 dt) {
    GameUpdateTurnTimers(g, dt);
    WorldUpdate((u8*)g + 0xD5F0, 0, dt);
}

// 0x80006F40
void fn_80006F40(void) {
}

// 0x80006F44
extern "C" void fn_80006F44(void) {
}

struct CameraFollow {
    f32  unk00;     // 0x00
    s32  unk04;     // 0x04
    f32  maxDist;   // 0x08
    s32  unk0C;     // 0x0C
    f32  unk10;     // 0x10
    Vec3 heading;   // 0x14: flattened (XY plane) facing of the player
    s32  unk20;     // 0x20
    s32  target;    // 0x24
};

struct SceneObj { u8 pad[4]; Transform* xform; };

// 0x8000733C
extern "C" void CameraFollowReset(CameraFollow* c) {
    SceneObj* player = *(SceneObj**)(*(u8**)gWorldState + 0x8);
    if (player != NULL) {
        Vec3 y = QuatGetAxisY(&player->xform->rot);
        c->heading.x = y.x;
        c->heading.y = y.y;
        c->heading.z = 0.0f;
        c->heading = Vec3Normalized(c->heading);
    }
    c->maxDist = FLT_MAX;
    c->unk04 = 0;
    c->unk10 = 0.0f;
    c->unk0C = 0;
    c->unk20 = 0;
    c->target = -1;
    c->unk00 = lbl_805FB300;
}
