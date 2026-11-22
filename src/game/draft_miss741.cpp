// Catch minigame / battle reaction / character physics functions (batch miss741).
#include "types.h"
#include <math.h>

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32 AnimSetFindClip(void* set, s32 anim);                              // 0x801AC2FC
u8* AnimSetGetClip(void* set, s32 index);                              // 0x801AC164
void BattleHudSetTurn(s32 side, BOOL on);                              // 0x80048BF4
void CatchMinigame_UpdateBase(void* g, f32 dt);                        // 0x800224C4
void CharacterResetGroundState(u8* c, s32 a, s32 b);                   // 0x8005BAD4
void CharacterStartReaction(void* ch);                                 // 0x8004E3D0
void* ChoiceDestroy(void* c, s32 del);                                 // 0x8020F394
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void LimbPlayReaction(void* limb, u8* reaction, BOOL special);         // 0x8004F398
void* PhysicsBodyGet(void* handle);                                    // 0x801B1840
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);            // 0x80209E70
u32 RandomU32(void);                                                   // 0x80143460
void ReactionBuildHit(void* round, u8* out, s32 side);                 // 0x80214F5C
void ReactionBuildRecover(u8* out, void* fighter);                     // 0x802159C4
s32 RoundGetAttacker(void* round);                                     // 0x80214F48
u32 StringHash(const char* s);                                         // 0x801EB704
s32 TriggerDoAction(void* action, void* a1, void* a2);                 // 0x801E0054
s32 UISetTexture(u8* el, s32 tex);                                     // 0x801A11FC
void fn_8002C15C(u8* a, u8* b, u8* c, u8* d);                          // 0x8002C15C
s32 fn_80031114(u8* list, f32* v);                                     // 0x80031114
void fn_8003ABDC(u8* arg0, s32 arg1);                                  // 0x8003ABDC
void fn_8003AEF4(u8* arg0, f32 dt);                                    // 0x8003AEF4
void fn_8003B2DC(u8* arg0, f32 dt);                                    // 0x8003B2DC
void fn_8003B834(u8* arg0);                                            // 0x8003B834
void fn_8003C048(u8* arg0);                                            // 0x8003C048
void fn_8003C478(u8* arg0);                                            // 0x8003C478
void fn_8003C888(u8* arg0, f32 dt);                                    // 0x8003C888
void fn_8003CA78(u8* p);                                               // 0x8003CA78
s32 fn_8003CAE0(u8* p);                                                // 0x8003CAE0
u8 fn_8003CB08(u8* p);                                                 // 0x8003CB08
void fn_8003CBB0(u8* p, f32 dt);                                       // 0x8003CBB0
u8* fn_8003CF14(u8* p);                                                // 0x8003CF14
void* fn_8003F7DC(void);                                               // 0x8003F7DC
s32 fn_80139140(void* world, f32* p0, f32* p1, u8* hit, s32 a, s32 b); // 0x80139140
s32 fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);                         // 0x8019BBA4
void* fn_801B7198(void* anim, s32 a);                                  // 0x801B7198
void* fn_801D40E0(s32 n);                                              // 0x801D40E0
void* fn_80209814(void* v);                                            // 0x80209814
void fn_8020A398(s32 handle);                                          // 0x8020A398
char* fn_80338BC0(char* s, const char* sub);                           // 0x80338BC0 (strstr)
extern u8 gWorldState[];                                               // 0x803E6D00
extern u8 lbl_803C4298[];                                              // 0x803C4298
}

extern "C++" u8* gBattle;               // 0x805FBBB8
extern "C++" void* gEventMgr;           // 0x805FB6C8
extern "C++" u8* lbl_805FB014;          // 0x805FB014 (battle camera object)
extern "C++" u8 lbl_805FB1C0;           // 0x805FB1C0
extern "C++" u8 lbl_805FB1C1;           // 0x805FB1C1
extern "C++" s32 lbl_805FB6B8;          // 0x805FB6B8
extern "C++" void* lbl_805FBBAC;        // 0x805FBBAC (voice SFX handle)
extern "C++" u8 gBattleAnims;           // 0x805F8598

// Value of the unsigned-int to float scale used by RandomU32 callers (1 / 2^32).
static const f32 kRandUnit = 2.3283064e-10f;

// Picks a random point inside the arena box (x in +0x1C..+0x24, z in +0x20..+0x28, fixed height from
// the object at +0x104) until the point list at +0x124 accepts it. The point is stored at dst
// (x, z, height); when the neighbouring point at ref exists, points closer than 100 units to it
// are rejected.
static void PickSpawnPoint(u8* arg0, s32 dst, s32 ref) {
    for (;;) {
        f32 hiX = *(f32*)(arg0 + 0x28);
        f32 height = *(f32*)(*(u8**)(arg0 + 0x104) + 0x70);
        f32 loX = *(f32*)(arg0 + 0x20);
        f32 z = loX + ((hiX - loX) * (kRandUnit * (f32)RandomU32()));
        f32 hiZ = *(f32*)(arg0 + 0x24);
        f32 loZ = *(f32*)(arg0 + 0x1C);
        *(f32*)(arg0 + dst + 4) = z;
        *(f32*)(arg0 + dst + 8) = height;
        f32 x = loZ + ((hiZ - loZ) * (kRandUnit * (f32)RandomU32()));
        *(f32*)(arg0 + dst) = x;
        if (arg0 + ref != NULL) {
            f32 dx = *(f32*)(arg0 + ref) - x;
            f32 dz = *(f32*)(arg0 + ref + 4) - z;
            f32 dh = *(f32*)(arg0 + ref + 8) - height;
            if (((dh * dh) + ((dx * dx) + (dz * dz))) < 10000.0f) {
                continue;
            }
        }
        if (fn_80031114(*(u8**)(arg0 + 0x124), (f32*)(arg0 + dst)) != 0) {
            break;
        }
    }
}

extern "C" {

// 0x8002B098
// Advances the 10-entry point ring at +0x3C (12 bytes each): while the cursor at +0xB4 is 5, refills
// the two slots at +0x48 and +0x3C with fresh random points, then feeds the three ring neighbours
// to fn_8002C15C (result at +0xB8) and moves the cursor on, wrapping at 10.
void fn_8002B098(u8* arg0) {
    if (*(s32*)(arg0 + 0xB4) == 5) {
        if (fn_80031114(*(u8**)(arg0 + 0x124), (f32*)(arg0 + 0x48)) == 0) {
            PickSpawnPoint(arg0, 0x48, 0x60);
        }
        if (fn_80031114(*(u8**)(arg0 + 0x124), (f32*)(arg0 + 0x3C)) == 0) {
            PickSpawnPoint(arg0, 0x3C, 0x54);
        }
    }
    s32 cur = *(s32*)(arg0 + 0xB4);
    fn_8002C15C(arg0 + 0xB8,
                arg0 + (((cur + 9) % 10) * 0xC) + 0x3C,
                arg0 + (cur * 0xC) + 0x3C,
                arg0 + (((cur + 1) % 10) * 0xC) + 0x3C);
    s32 next = *(s32*)(arg0 + 0xB4) + 1;
    *(s32*)(arg0 + 0xB4) = next;
    if (next >= 10) {
        *(s32*)(arg0 + 0xB4) = 0;
    }
}

// 0x8003A858
// Catch minigame per-frame update: ticks the 7 target slots (0x34 bytes each from +0x90), handles
// a finished slot (respawn + sfx), ends the game when no slot is alive, counts down the texture
// timer, spawns a new target when the spawn timer expires, then runs the sub-updates.
void fn_8003A858(u8* arg0, f32 dt) {
    CatchMinigame_UpdateBase(arg0, dt);
    u8* slots = arg0 + 0x90;
    s32 anyAlive = 0;
    *(f32*)(arg0 + 0x3B0) = *(f32*)(arg0 + 0x3B0) + dt;
    for (s32 i = 0; i < 7; i++) {
        u8* slot = slots + i * 0x34;
        fn_8003CBB0(slot, dt);
        if (fn_8003CB08(slot) != 0) {
            fn_8003ABDC(arg0, i);
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) {
                    v = fn_80209814(v);
                }
                lbl_805FBBAC = v;
            }
            PlaySound(0x9A15501FU, 0, 0, 0, 0, 0);
        }
        if (*(u8*)(arg0 + i * 0x34 + 0xBE) == 0) {
            anyAlive = 1;
        }
    }
    if (anyAlive == 0) {
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
        UIElement_SetVisibleFlag(0x9D841CAAU, 0U);  // "timer"
        UIElement_SetVisibleFlag(0x0CAF9C6FU, 0U);
        GameFlowSignal((s32)0xC608C842U, 0, 0);
        fn_8020A398(*(s32*)(arg0 + 0x3C4));
        return;
    }
    f32 texTimer = *(f32*)(arg0 + 0x958);
    if (texTimer > 0.0f) {
        texTimer -= dt;
        *(f32*)(arg0 + 0x958) = texTimer;
        if (texTimer < 0.0f) {
            UISetTexture(*(u8**)(arg0 + 0x924), *(s32*)(arg0 + 0x3C8));
            fn_8019BBA4(*(u8**)(arg0 + 0x924), *(u8**)(arg0 + 0x940), 10);
            *(f32*)(arg0 + 0x958) = 0.0f;
        }
    }
    if (*(f32*)(arg0 + 0x3B0) >= *(f32*)(arg0 + 0x3AC) && *(s32*)(arg0 + 0x3A4) < *(s32*)(arg0 + 0x3A8)) {
        s32 free[7] = {0, 0, 0, 0, 0, 0, 0};
        s32 nFree = 0;
        for (s32 i = 0; i < 7; i++) {
            if (fn_8003CAE0(slots + i * 0x34) == 0) {
                free[nFree] = i;
                nFree++;
            }
        }
        if (nFree > 0) {
            u32 r = RandomU32();
            *(f32*)(arg0 + 0x3B0) = *(f32*)(arg0 + 0x3B0) - *(f32*)(arg0 + 0x3AC);
            fn_8003CA78(arg0 + (free[r % (u32)nFree] * 0x34) + 0x90);
            *(s32*)(arg0 + 0x3A4) = *(s32*)(arg0 + 0x3A4) + 1;
        }
    }
    fn_8003B834(arg0);
    fn_8003AEF4(arg0, dt);
    fn_8003B2DC(arg0, dt);
    fn_8003C048(arg0);
    fn_8003C478(arg0);
    if (*(f32*)(arg0 + 0x6C) <= 0.0f) {
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
        GameFlowSignal(0x117741C3, 0, 0);
    }
    (*(void (**)(u8*))(*(u8**)(arg0 + 4) + 0x6C))(arg0);
    fn_8003C888(arg0, dt);
    s32 total = *(s32*)(arg0 + 0x984);
    f32 maxW = *(f32*)(arg0 + 0x9B8);
    f32 w = (maxW * (f32)(total - *(s32*)(arg0 + 0x3B8))) / (f32)total;
    if (w > maxW) {
        w = maxW;
    }
    f32 cx = *(f32*)(arg0 + 0x9BC);
    f32 cy = *(f32*)(arg0 + 0x9C0);
    f32 cz = *(f32*)(arg0 + 0x9C4);
    u8* bar = *(u8**)(arg0 + 0x9B4);
    *(f32*)(bar + 0x68) = cx;
    *(f32*)(bar + 0x6C) = cy;
    *(f32*)(bar + 0x70) = cz;
    bar = *(u8**)(arg0 + 0x9B4);
    *(f32*)(bar + 0x74) = w;
    bar = *(u8**)(arg0 + 0x9B4);
    (*(void (**)(u8*, s32))(*(u8**)(bar + 0x138) + 0x2C))(bar, 0x1A);
}

// 0x8003ABDC
// Catch minigame: launches target slot `arg1` from a random spawn side. Picks the left/right side
// (biased, forced to flip after 10 repeats), finds the first free sprite on that side, places it at
// the slot's position, and sets its speed and fly-in offsets.
void fn_8003ABDC(u8* arg0, s32 arg1) {
    f32 scale = *(f32*)(arg0 + 0x380);
    u8** sprites = (u8**)(arg0 + 0x244);
    u8** other = (u8**)(arg0 + 0x214);
    u8* offs = arg0 + 0x2E8;
    u8* busy = arg0 + 0x29A;
    f32* speeds = (f32*)(arg0 + 0x360);
    f32 base = *(f32*)(arg0 + 0x38C);
    f32 speed = (*(f32*)(arg0 + 0x388) + (2.0f * (base * (0.01f * (f32)(RandomU32() % 100u))))) - base;
    s32 side = !((0.0f + (1.0f * (kRandUnit * (f32)RandomU32()))) < 0.45f);
    if (side == *(s32*)(arg0 + 0x39C)) {
        s32 repeats = *(s32*)(arg0 + 0x3A0) + 1;
        *(s32*)(arg0 + 0x3A0) = repeats;
        if (repeats > 9) {
            side = (side == 0);
        }
    }
    *(s32*)(arg0 + 0x39C) = side;
    if (side == 0) {
        scale = *(f32*)(arg0 + 0x378);
        sprites = (u8**)(arg0 + 0x1FC);
        offs = arg0 + 0x2A0;
        other = (u8**)(arg0 + 0x214);
        busy = arg0 + 0x294;
        speeds = (f32*)(arg0 + 0x348);
    }
    for (s32 i = 0; i < 6; i++) {
        if (*(f32*)(sprites[i] + 0x68) == -1000.0f && busy[i] == 0 &&
            (side != 0 || !(*(f32*)(other[i] + 0x68) > -1000.0f))) {
            u8* slot = arg0 + (arg1 * 0x34) + 0x90;
            UIElement_SetPos(*(u32*)sprites[i], (f32*)fn_8003CF14(slot));
            u8* sp = sprites[i];
            *(f32*)(sp + 0x70) = 45.0f;
            *(f32*)(sp + 0x74) = scale;
            *(f32*)(sp + 0x78) = scale;
            *(f32*)(sp + 0x7C) = 1.0f;
            s32 targetX = (s32)(RandomU32() % 400u) + 0x78;
            s32 targetY = (s32)(RandomU32() % 290u) + 0x5A;
            s32 dx = (s32)(((f32)targetX - *(f32*)fn_8003CF14(slot)) / speed);
            f32 curY = *(f32*)(fn_8003CF14(slot) + 4);
            speeds[i] = speed;
            *(f32*)(offs + i * 0xC) = (f32)dx;
            s32 dy = (s32)(((f32)targetY - curY) / speed);
            *(f32*)(offs + i * 0xC + 4) = (f32)dy;
            *(f32*)(offs + i * 0xC + 8) = 0.0f;
            sp = sprites[i];
            (*(void (**)(u8*, s32))(*(u8**)(sp + 0x138) + 0x2C))(sp, 0x18);
            return;
        }
    }
}

// 0x8004A5D4
// Battle intro: resets the attack-sequence record, builds each side's hit reaction names, finds the
// longest matching clip per side (+0xC), picks the side that has to take the hit (+0x18/+0x1C), and
// when battle anims are on plays the hit reaction on the victim and the recover reaction on the
// other fighter. Finishes by updating the turn HUD.
void fn_8004A5D4(u8* arg0) {
    u8 hit[0x500];     // 5 reaction names of 0x100 bytes
    u8 recover[0x500];
    u8 choice[12];     // local choice record (s32 +0, u8 +4, s32 +8)
    s32 sideVals[2] = {4, 4};

    *(u8*)(arg0 + 0x20) = 0;
    *(f32*)(arg0 + 0x3C) = 0.0f;
    *(f32*)(arg0 + 0x40) = 0.0f;
    for (s32 i = 0x21; i <= 0x2E; i++) {
        *(u8*)(arg0 + i) = 0;
    }
    *(s32*)(arg0 + 0x18) = 0;
    u8* round = *(u8**)(arg0 + 4);
    lbl_805FB1C0 = 0;
    lbl_805FB1C1 = 0;
    u8* entry = round;
    for (s32 side = 0; side < 2; side++) {
        *(s32*)(choice + 0) = *(s32*)(entry + 0x30);
        *(u8*)(choice + 4) = *(u8*)(entry + 0x34);
        s32 v = *(s32*)(entry + 0x38);
        *(s32*)(choice + 8) = v;
        sideVals[side] = v;
        ReactionBuildHit(round, hit, side);
        u8** team = *(u8***)(gBattle + side * 4 + 0xA00);
        if (fn_80338BC0((char*)(hit + 0x400), "idle") != NULL) {
            *(u8*)(arg0 + 0x14 + side) = 1;
        } else {
            *(u8*)(arg0 + 0x14 + side) = 0;
        }
        *(s32*)(arg0 + 0xC + side * 4) = -1;
        for (s32 j = 0; j < 5; j++) {
            u8* limbObj = team[j];
            if (limbObj != NULL) {
                u8* limb = *(u8**)(limbObj + 0x20);
                if (limb != NULL) {
                    limb -= 0x264;
                }
                u8* anim = *(u8**)(limb + 0x9C);
                s32 clip = -1;
                if (anim != NULL && *(void**)(anim + 0x48) != NULL) {
                    s32 idx = AnimSetFindClip(*(void**)(anim + 0x48), (s32)StringHash((const char*)(hit + j * 0x100)));
                    if (idx >= 0) {
                        clip = *(s32*)(AnimSetGetClip(*(void**)(anim + 0x48), idx) + 0x28);
                    }
                }
                if (clip > *(s32*)(arg0 + 0xC + side * 4)) {
                    *(s32*)(arg0 + 0xC + side * 4) = clip;
                }
            }
        }
        ChoiceDestroy(choice, -1);
        entry += 0x38;
    }
    *(s32*)(arg0 + 0x1C) = sideVals[0];
    if (*(s32*)(arg0 + 0x10) > *(s32*)(arg0 + 0xC)) {
        *(s32*)(arg0 + 0x18) = 1;
        *(s32*)(arg0 + 0x1C) = sideVals[1];
    }
    u8* cam = *(u8**)(lbl_805FB014 + 0x4D4);
    if (cam != NULL) {
        if ((*(s32 (**)(u8*))(*(u8**)cam + 0x10))(cam) != 0) {
            (*(void (**)(u8*, void*))(*(u8**)cam + 0x84))(cam, fn_8003F7DC());
        }
    }
    if (RoundGetAttacker(round) == 1) {
        lbl_805FB1C0 = 1;
    }
    if (gBattleAnims != 0) {
        s32 loser = *(s32*)(arg0 + 0x18);
        u8** victims = *(u8***)(gBattle + loser * 4 + 0xA00);
        ReactionBuildHit(round, hit, loser);
        for (s32 j = 0; j < 5; j++) {
            u8* limbObj = victims[j];
            if (limbObj != NULL) {
                u8* limb = *(u8**)(limbObj + 0x20);
                if (limb != NULL) {
                    limb -= 0x264;
                }
                LimbPlayReaction(limb, hit + j * 0x100, 0);
            }
        }
        u8* ch = *(u8**)((u8*)victims + 0x1C);
        if (ch != NULL) {
            ch -= 0xC48;
        }
        CharacterStartReaction(ch);
        u8** winners = *(u8***)(gBattle + (1 - *(s32*)(arg0 + 0x18)) * 4 + 0xA00);
        ReactionBuildRecover(recover, winners);
        for (s32 j = 0; j < 5; j++) {
            u8* limbObj = winners[j];
            if (limbObj != NULL) {
                u8* limb = *(u8**)(limbObj + 0x20);
                if (limb != NULL) {
                    limb -= 0x264;
                }
                LimbPlayReaction(limb, recover + j * 0x100, 1);
            }
        }
        ch = *(u8**)((u8*)winners + 0x1C);
        if (ch != NULL) {
            ch -= 0xC48;
        }
        CharacterStartReaction(ch);
    }
    BattleHudSetTurn(RoundGetAttacker(round), 0);
}

// 0x800571D4
// Applies the push vector (direction at arg1+0xC.. scaled by arg1+0x18) to the object's position and
// physics body, flags it as moved, removes the velocity component (at +0x844) heading against the
// push, and sends a hit event to the other object (arg1+0x5C) when it has a listener. Returns 1.
s32 fn_800571D4(u8* arg0, u8* arg1) {
    f32 s = *(f32*)(arg1 + 0x18);
    f32 vx = *(f32*)(arg1 + 0xC) * s;
    f32 vy = *(f32*)(arg1 + 0x10) * s;
    u8* other = *(u8**)(arg1 + 0x5C);
    f32 vz = *(f32*)(arg1 + 0x14) * s;
    u8* pos = *(u8**)(arg0 + 0x4);
    *(f32*)(pos + 0x0) = *(f32*)(pos + 0x0) + vx;
    *(f32*)(pos + 0x4) = *(f32*)(pos + 0x4) + vy;
    *(f32*)(pos + 0x8) = *(f32*)(pos + 0x8) + vz;
    void* handle = *(void**)(arg0 + 0x8C);
    if (handle != NULL && PhysicsBodyGet(handle) != NULL) {
        u8* body = (u8*)PhysicsBodyGet(*(void**)(arg0 + 0x8C));
        *(f32*)(body + 0x14) = *(f32*)(body + 0x14) + vx;
        *(f32*)(body + 0x18) = *(f32*)(body + 0x18) + vy;
        *(f32*)(body + 0x1C) = *(f32*)(body + 0x1C) + vz;
    }
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    f32 len2 = (vz * vz) + ((vx * vx) + (vy * vy));
    f32 nx, ny, nz;
    if (len2 == 0.0f) {
        nx = 0.0f;
        ny = 0.0f;
        nz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        nx = vx * inv;
        ny = vy * inv;
        nz = vz * inv;
    }
    f32 dot = (nz * *(f32*)(arg0 + 0x84C)) + ((nx * *(f32*)(arg0 + 0x844)) + (ny * *(f32*)(arg0 + 0x848)));
    if (dot < 0.0f) {
        *(f32*)(arg0 + 0x844) = *(f32*)(arg0 + 0x844) - (nx * dot);
        *(f32*)(arg0 + 0x848) = *(f32*)(arg0 + 0x848) - (ny * dot);
        *(f32*)(arg0 + 0x84C) = *(f32*)(arg0 + 0x84C) - (0.0f * dot);
    }
    if (*(u32*)(other + 0x838) & 0x4000) {
        // The planar direction away from `other` and its 16x scaling are computed here in the
        // original but never used.
        s32* listener = *(s32**)(other + 0xA4);
        if (listener == NULL) {
            return 1;
        }
        if (*listener == 0) {
            return 1;
        }
        EventSend(gEventMgr, other, other, 0x666DD44FU, 0, 0, 1, 0);
    }
    return 1;
}

// 0x8005AD90
// Character physics step: runs the trigger actions of the current ground object (flagging the
// character when the object has no owner), decays the velocity at +0x844 by the drag factor
// (+0x85C * +0x24), resets the ground contact, raycasts down from the position to refresh the
// ground height (+0x7D4), and when airborne-ish (no flag 0x400000, low z, no jump timer) cancels
// the velocity component along the ground normal (+0x7A4) and resets the ground state.
void fn_8005AD90(u8* arg0) {
    s32 groundId = *(s32*)(arg0 + 0x7B0);
    u8* ground = (u8*)fn_801B7198(&groundId, 0);
    if (ground != NULL) {
        if (*(s32*)(ground + 0x8C) == 0) {
            *(s32*)(arg0 + 0x790) = 1;
            *(u32*)(arg0 + 0x838) |= 0x02000000;
        }
        u8* owner = *(u8**)(ground + 0x9C);
        if (owner != NULL && ((*(u32*)(owner + 0xA4) >> 27) & 0xF)) {
            TriggerDoAction(owner + 0xA4, arg0, ground);
        }
        if ((*(u32*)(ground + 0x74) >> 27) & 0xF) {
            TriggerDoAction(ground + 0x74, arg0, ground);
        }
    }
    f32 drag = *(f32*)(arg0 + 0x85C) * *(f32*)(arg0 + 0x24);
    f32 dx = *(f32*)(arg0 + 0x844) * drag;
    f32 dy = *(f32*)(arg0 + 0x848) * drag;
    f32 dz = *(f32*)(arg0 + 0x84C) * drag;
    *(s32*)(arg0 + 0x7B4) = 0;
    *(s32*)(arg0 + 0x7B0) = lbl_805FB6B8;
    *(f32*)(arg0 + 0x7B8) = 0.0f;
    *(f32*)(arg0 + 0x7BC) = 0.0f;
    *(f32*)(arg0 + 0x7C0) = 0.0f;
    *(f32*)(arg0 + 0x7C4) = 0.0f;
    *(f32*)(arg0 + 0x7C8) = 0.0f;
    *(f32*)(arg0 + 0x7CC) = 0.0f;
    *(f32*)(arg0 + 0x7D0) = 1.0f;
    *(f32*)(arg0 + 0x844) = *(f32*)(arg0 + 0x844) - dx;
    *(f32*)(arg0 + 0x848) = *(f32*)(arg0 + 0x848) - dy;
    *(f32*)(arg0 + 0x84C) = *(f32*)(arg0 + 0x84C) - dz;

    // Ray query: start = position, direction (-0, -0, -1), result record cleared.
    f32 dir[3] = {-0.0f, -0.0f, -1.0f};
    f32 from[6];
    s32 hitBuf[0x1E];
    u8* hit = (u8*)hitBuf;
    u8* pos = *(u8**)(arg0 + 0x4);
    from[0] = *(f32*)(pos + 0x0);
    from[1] = *(f32*)(pos + 0x4);
    from[2] = *(f32*)(pos + 0x8);
    from[3] = dx;
    from[4] = dy;
    from[5] = dz;
    for (s32 i = 0; i < 0x1E; i++) {
        hitBuf[i] = 0;
    }
    hitBuf[(0xE8 - 0x94) / 4] = 7;
    fn_80139140(gWorldState, from, dir, hit, 0, 0);
    *(f32*)(arg0 + 0x7D4) = *(f32*)(hit + 0x18);
    if (!(*(u32*)(arg0 + 0x838) & 0x400000)) {
        f32 vz = *(f32*)(arg0 + 0x858) + *(f32*)(arg0 + 0x84C);
        f32 timer = (-6.0f * *(f32*)(arg0 + 0x7EC)) + *(f32*)(arg0 + 0x760);
        f32 vx = *(f32*)(arg0 + 0x850) + *(f32*)(arg0 + 0x844);
        *(f32*)(arg0 + 0x760) = timer;
        f32 vy = *(f32*)(arg0 + 0x854) + *(f32*)(arg0 + 0x848);
        if (vz < 0.5f) {
            if (*(f32*)(arg0 + 0x7D8) >= 0.0f) {
                if (timer <= 0.0f) {
                    f32 nx = *(f32*)(arg0 + 0x7A4);
                    f32 ny = *(f32*)(arg0 + 0x7A8);
                    f32 nz = *(f32*)(arg0 + 0x7AC);
                    f32 dot = (nz * vz) + ((nx * vx) + (ny * vy));
                    *(f32*)(arg0 + 0x844) = *(f32*)(arg0 + 0x844) - (nx * dot);
                    *(f32*)(arg0 + 0x848) = *(f32*)(arg0 + 0x848) - (ny * dot);
                    *(f32*)(arg0 + 0x84C) = *(f32*)(arg0 + 0x84C) - (nz * dot);
                    CharacterResetGroundState(arg0, 0, 0);
                }
            }
        }
    }
}

}  // extern "C"
