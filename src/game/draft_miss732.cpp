// Fixed m2c drafts (batch miss732).
#include "types.h"
#include "engine/math.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void* AbilityEffect(void* db, s32 ability);                       // 0x8021679C
void* AbilityEffectForLimb(void* db, s32 ability, s32 limb);      // 0x802167E4
u32 EffectEmitter(void* db, s32 effect, s32 limb);                // 0x800523B8
void BattleHudSetTurn(s32 side, BOOL on);                         // 0x80048BF4
void RoundNoteAbility(void* round, s32 side, s32 limb, s32 ability);
s32 fn_802152E4(void* round, s32 side, s32 limb);                 // 0x802152E4
void fn_8004504C(void);                                           // 0x8004504C
int SysSprintf(char* buf, const char* fmt, ...);                  // 0x80142EF8
extern const u32 kAbilityEffects[];                               // 0x80388258
extern const char lbl_80358C68[];
extern const char lbl_805F8698[];
extern f32 lbl_803475C0[];                                        // 0x803475C0 (RGBA, 4 floats)
}
extern "C++" u8* gBattle;                                         // 0x805FBBB8
extern "C++" u8* gGameDb;                                         // 0x805FBBC0
extern "C++" char lbl_803C5358[];                                 // 0x803C5358 (gSysBuildInfo)

struct ScopedLock { u8 pad[4]; };                                 // as in props.cpp

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);
s32 TriggerDoAction(void* action, void* a1, void* a2);            // 0x801E0054
void fn_801B62D8(void* p, void* q, s32 a, s32 b, f32* vec, f32 scale);
void CharacterResetGroundState(u8* c, s32 a, s32 b);
s32 PlaySoundAt(void* sound, int a, const void* pos, int b);      // 0x80209ABC
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
u32 RandomU32(void);                                              // 0x80143460
void fn_801B1778(void* p, s32 a);
void ScopedLockCtor(ScopedLock* l);                               // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                    // 0x801F53B0
void QuatGetForward(Vec3* out, const Quat* q);                    // 0x801454CC
void fn_801458B0(f32* quat, f32* a, f32* b);                      // 0x801458B0
void fn_80070460(u8* p);                                          // 0x80070460
void fn_800712BC(u8* p, s32 a, s32 b);                            // 0x800712BC
void fn_8007E354(u8* p, u32 hash);
void AimMinigame_Update(u8* g);
s32 RaySceneTest(void* ray);                                      // 0x80070458 (ignores its argument)
void fn_8008C8E4(u8* a, s32 b);
void fn_80089B30(u8* p);
void fn_80088004(u8* p, f32 dt);
void fn_8008853C(u8* p, f32 dt);
void fn_80088BC0(u8* p, f32 dt);
extern f32 lbl_803464E4[];                                        // 0x803464E4
extern f32 lbl_803469A8[];                                        // 0x803469A8
extern f32 lbl_803469B8[];                                        // 0x803469B8
extern f32 lbl_80346A48[];                                        // 0x80346A48
extern u32 lbl_805FB854;                                          // 0x805FB854 (frame counter)
extern u32 lbl_805FB9A4;                                          // 0x805FB9A4
extern f32 lbl_80359840[];                                        // 0x80359840 (4 floats)
extern f32 lbl_80359850[];                                        // 0x80359850 (4 floats)
}
extern "C++" void* gEventMgr;                                     // 0x805FB6C8
extern "C++" u8 gFlag805FB948;                                    // 0x805FB948
extern "C++" u8 gWorldState[];                                    // 0x803E6D00
extern "C++" u8 lbl_8047C760[];                                   // effect manager

typedef void (*VCallFn)(void*, s32);
typedef f32 (*VCallF)(void*);

extern "C" {

// 0x8003C478
// Six pairs of objects: each is tinted lbl_803475C0 when within 128 units (xy) of the
// reference object at +0x924, otherwise white; then its virtual slot 0x2C is called with 0x20.
void fn_8003C478(u8* arg0) {
    u8* ref = *(u8**)(arg0 + 0x924);
    f32 rx = *(f32*)(ref + 0x68);
    f32 ry = *(f32*)(ref + 0x6C);
    for (s32 i = 0; i < 6; i++) {
        for (s32 k = 0; k < 2; k++) {
            u8* o = *(u8**)(arg0 + (k == 0 ? 0x22C : 0x27C) + i * 4);
            f32 dx = rx - *(f32*)(o + 0x68);
            f32 dy = ry - *(f32*)(o + 0x6C);
            if (0.0f * 0.0f + (dx * dx + dy * dy) <= 16384.0f) {
                *(f32*)(o + 0x80) = lbl_803475C0[0];
                *(f32*)(o + 0x84) = lbl_803475C0[1];
                *(f32*)(o + 0x88) = lbl_803475C0[2];
                *(f32*)(o + 0x8C) = lbl_803475C0[3];
            } else {
                *(f32*)(o + 0x80) = 255.0f;
                *(f32*)(o + 0x84) = 255.0f;
                *(f32*)(o + 0x88) = 255.0f;
                *(f32*)(o + 0x8C) = 255.0f;
            }
            o = *(u8**)(arg0 + (k == 0 ? 0x22C : 0x27C) + i * 4);
            (*(VCallFn*)(*(u8**)(o + 0x138) + 0x2C))(o, 0x20);
        }
    }
}

// 0x8004B0E4
// End-of-battle-turn ability cleanup for both sides: removes the per-limb ability particle
// effects, and when a limb's counter is positive (and no turn lock) notes ability 7 and
// plays its effect.
void fn_8004B0E4(u8* arg0, s32* arg1) {
    SysSprintf(lbl_803C5358, lbl_80358C68 + 0x80);
    *(u8*)(arg0 + 0x14) = 0;
    for (s32 side = 0; side < 2; side++) {
        s32 noted = 0;
        s32* cnt = arg1 + side * 5;
        void* round = 0;
        for (s32 limb = 0; limb < 5; limb++) {
            round = *(void**)(gBattle + 0xA0C);
            u8* obj = (*(u8***)(gBattle + 0xA00 + side * 4))[limb];
            if (obj != 0) {
                SysSprintf(lbl_803C5358, lbl_80358C68 + 0x8C);
                u8* emitterObj = *(u8**)(obj + 0x20);
                if (emitterObj != 0) emitterObj -= 0x264;
                s32 ab = fn_802152E4(round, side, limb);
                if (ab != -1 && ab < 0x10) {
                    u8* rec = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x3610) + ab * 0x24;
                    u32 eff = kAbilityEffects[*(s32*)(rec + 0x10)];
                    u8* e = (u8*)AbilityEffectForLimb(gGameDb, ab, limb);
                    RemoveSpecificAttachedParticleEffectAtEmitter(
                        emitterObj, eff, EffectEmitter(gGameDb, *(s32*)(e + 0xC), limb));
                }
            }
            if (cnt[limb] > 0 && *(s32*)(gBattle + 0xA4C) == 0) {
                SysSprintf(lbl_803C5358, lbl_80358C68 + 0x98);
                noted = 1;
                RoundNoteAbility(round, side, limb, 7);
                BattleHudSetTurn(side, 0);
                if (*(f32*)(arg0 + 0x8) < 1.0f) *(f32*)(arg0 + 0x8) = 1.0f;
            }
        }
        if (noted != 0) {
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0xA4);
            s32 effect = *(s32*)((u8*)AbilityEffect(gGameDb, 7) + 0xC);
            s32 limb = *(s32*)((u8*)AbilityEffect(gGameDb, 7) + 0x8);
            u32 em = EffectEmitter(gGameDb, effect, limb);
            u8* tbl = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x3610);
            u8* obj = (*(u8***)(gBattle + 0xA00 + side * 4))[limb];
            if (obj != 0) {
                u8* emitterObj = *(u8**)(obj + 0x20);
                if (emitterObj != 0) emitterObj -= 0x264;
                PlayAttachedParticleEffectAtEmitter(emitterObj, kAbilityEffects[*(s32*)(tbl + 0x10C)], em);
            }
        }
    }
    fn_8004504C();
    *(u8*)(arg0 + 0x6) = 0;
    SysSprintf(lbl_803C5358, lbl_805F8698);
}

// 0x80058CEC
// Applies `scale` damage and `arg3` hit-strength of kind `arg2` to a character: fires the
// hit actions, scales the damage, reduces health, applies knockback from the vector `arg4`,
// and on death triggers the death event/effects. Finishes with the base handler fn_801B62D8.
void fn_80058CEC(u8* arg0, void* arg1, s32 arg2, s32 arg3, f32* arg4, f32 scale) {
    u8* obj = *(u8**)(arg0 + 0x4);
    if (!(*(u32*)(obj + 0x20) & 0x2)) return;
    if (*(u32*)(obj + 0x20) & 0x80000) return;

    s32 ok1 = 1;
    s32 ok2 = 1;
    s32 savedC0 = *(s32*)(arg0 + 0xC0);
    *(s32*)(arg0 + 0xC0) = arg2;
    if ((*(u32*)(arg0 + 0x38) >> 27) & 0xF) ok1 = TriggerDoAction(arg0 + 0x38, arg0, arg1);
    if (ok1 == 1) {
        u8* d = *(u8**)(arg0 + 0x9C);
        if (d != 0 && ((*(u32*)(d + 0x68) >> 27) & 0xF)) ok2 = TriggerDoAction(d + 0x68, arg0, arg1);
    }
    if (!(ok1 == 1 && ok2 == 1)) {
        // an action vetoed the hit
        fn_801B62D8(arg0, arg1, arg2, arg3, arg4, scale);
        if (*(u32*)(*(u8**)(arg0 + 0x4) + 0x1C) & 0x20)
            EventSend(gEventMgr, arg0, arg0, 0x66749C25u, arg2, (u32)0.0f, 1, 0);
        *(s32*)(arg0 + 0xC0) = savedC0;
        return;
    }

    f32 mult = 1.0f;
    if (arg2 < 0xE) mult = *(f32*)(*(u8**)(arg0 + 0x9C) + arg2 * 4 + 0x204);
    if (gFlag805FB948 == 0 || lbl_805FB9A4 == 0) scale *= mult;

    if (arg2 == 0) {
        s32 minStr = *(s32*)(arg0 + 0x634);
        if (minStr >= 0 && arg3 < minStr) arg3 = minStr;
        f32 minDmg = *(f32*)(arg0 + 0x630);
        if (minDmg >= 0.0f && scale < minDmg) scale = minDmg;
    }

    if (!(*(u32*)(arg0 + 0x838) & 0x20000))
        PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x9C) + 0x2C), 0, *(void**)(arg0 + 0x4), 0);

    if ((*(u32*)(arg0 + 0x838) & 0x20000) && scale < 1000.0f) {
        EventSend(gEventMgr, arg0, arg0, 0xC9698F8Au, arg2, (u32)scale, 1, 0);
        return;
    }

    if (!(*(u32*)(arg0 + 0x838) & 0x20000) || scale > 1000.0f) {
        obj = *(u8**)(arg0 + 0x4);
        f32 health = *(f32*)(obj + 0x24);
        if (health > 0.0f) {
            *(f32*)(obj + 0x24) = health - scale;
            u32 now = lbl_805FB854;
            if (now - *(u32*)(arg0 + 0xC4) > 3u) {
                u8* d = *(u8**)(arg0 + 0x9C);
                if (d != 0 && ((*(u32*)(d + 0x6C) >> 27) & 0xF)) TriggerDoAction(d + 0x6C, arg0, arg1);
                if ((*(u32*)(arg0 + 0x3C) >> 27) & 0xF) TriggerDoAction(arg0 + 0x3C, arg0, arg1);
            }
            u8* d2 = *(u8**)(arg0 + 0x9C);
            *(u32*)(arg0 + 0xC4) = now;
            if (d2 != 0 && ((*(u32*)(d2 + 0x70) >> 27) & 0xF)) TriggerDoAction(d2 + 0x70, arg0, arg1);
            if ((*(u32*)(arg0 + 0x40) >> 27) & 0xF) TriggerDoAction(arg0 + 0x40, arg0, arg1);
        }
        if (scale > 0.0f) *(f32*)(arg0 + 0x5FC) = *(f32*)(arg0 + 0x28);

        if (arg4 != 0) {
            f32 vz = arg4[2];
            if (vz < 0.0f) vz *= -1.0f;
            arg4[2] = vz;
            *(f32*)(arg0 + 0x850) = arg4[0];
            *(f32*)(arg0 + 0x854) = arg4[1];
            *(f32*)(arg0 + 0x858) = arg4[2];
            if (arg2 == 0) {
                f32 maxLen = *(f32*)(arg0 + 0x62C);
                if (maxLen >= 0.0f) {
                    f32 len = 0.0f;
                    f32 x = arg4[0], y = arg4[1], z = arg4[2];
                    f32 sum = z * z + (x * x + y * y);
                    if (!(sum <= 0.0f)) len = sqrtf(sum);
                    if (len < maxLen) {
                        f32 vx = *(f32*)(arg0 + 0x850);
                        f32 vy = *(f32*)(arg0 + 0x854);
                        f32 vzz = *(f32*)(arg0 + 0x858);
                        f32 s2 = vzz * vzz + (vx * vx + vy * vy);
                        f32 nx, ny, nz;
                        if (s2 == 0.0f) {
                            nx = ny = nz = 0.0f;
                        } else {
                            f32 inv = 1.0f / sqrtf(s2);
                            nx = vx * inv;
                            ny = vy * inv;
                            nz = vzz * inv;
                        }
                        *(f32*)(arg0 + 0x850) = nx * maxLen;
                        *(f32*)(arg0 + 0x854) = ny * maxLen;
                        *(f32*)(arg0 + 0x858) = nz * maxLen;
                    }
                }
            }
            f32 upZ = *(f32*)(arg0 + 0x858) + *(f32*)(arg0 + 0x84C);
            if (upZ > 0.0f) {
                CharacterResetGroundState(arg0, 1, 0);
                if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000))
                    *(u32*)(arg0 + 0x838) |= 0x800;
            }
        }

        s32 str = arg3;
        s32 prev = (s8) * (u8*)(arg0 + 0x61C);
        if (prev > arg3) str = prev;
        *(u8*)(arg0 + 0x61C) = (u8)str;
        obj = *(u8**)(arg0 + 0x4);
        *(s32*)(arg0 + 0x840) = (s8)(u8)str;
        f32 maxHealth = *(f32*)(arg0 + 0x5F4);
        if (*(f32*)(obj + 0x24) > maxHealth) *(f32*)(obj + 0x24) = maxHealth;
    }

    obj = *(u8**)(arg0 + 0x4);
    if (*(f32*)(obj + 0x24) <= 0.0f) {
        // health is gone: death handling
        u32 flags = *(u32*)(obj + 0x20);
        if (!(flags & 0x1000)) {
            if (flags & 0x4000) {
                void* p = *(void**)(arg0 + 0x8C);
                if (p != 0) fn_801B1778(p, 0);
            }
            EventSend(gEventMgr, arg0, arg0, 0x233E0AD4u, 0, 0, 1, 0);
            if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x40000)) {
                s32 ev = *(s32*)(arg0 + 0xF8);
                if (ev != 0) EventSendTo(gEventMgr, (u32)ev, arg0, 0x73B5A74Eu, 0, 0);
                *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x40000;
            }
            if (*(u8*)(*(u8**)(arg0 + 0x9C) + 0x244) != 0 &&
                ((arg2 != 0 && scale > 0.66f * *(f32*)(arg0 + 0x5F4)) ||
                 (arg3 == 3 && 2.3283064e-10f * (f32)RandomU32() < 0.1f) ||
                 (arg3 == 4 && 2.3283064e-10f * (f32)RandomU32() < 0.3f))) {
                u8* spawned = 0;
                s32 effect = *(s32*)(*(u8**)(arg0 + 0x9C) + 0x1E8);
                if (effect != 0) {
                    f32 pos[4];
                    pos[0] = *(f32*)(arg0 + 0x18);
                    pos[1] = *(f32*)(arg0 + 0x1C);
                    pos[2] = *(f32*)(arg0 + 0x20);
                    pos[3] = 0.0f;
                    SpawnEffectAt(lbl_8047C760, (u32)effect, pos, &spawned);
                }
                if (spawned != 0) {
                    if (arg4 != 0) {
                        f32 x = arg4[0], y = arg4[1], z = arg4[2];
                        if (z * z + (x * x + y * y) > 0.0f) {
                            // near-zero direction -> default (0,0,1); else the hit vector
                            s32 tiny = 0;
                            if (x <= 0.0001f && x >= -0.0001f && y <= 0.0001f && y >= -0.0001f &&
                                z <= 0.0001f && z >= -0.0001f && 0.0f <= 0.0001f && 0.0f >= -0.0001f)
                                tiny = 1;
                            if (tiny != 0) {
                                for (s32 i = 0; i < *(s32*)(spawned + 0xC); i++) {
                                    u8* e = *(u8**)(spawned + i * 4 + 0x14);
                                    if (e != 0) {
                                        *(f32*)(e + 0x30) = 0.0f;
                                        *(f32*)(e + 0x34) = 0.0f;
                                        *(f32*)(e + 0x38) = 1.0f;
                                        *(f32*)(e + 0x3C) = 0.0f;
                                    }
                                }
                            } else {
                                for (s32 i = 0; i < *(s32*)(spawned + 0xC); i++) {
                                    u8* e = *(u8**)(spawned + i * 4 + 0x14);
                                    if (e != 0) {
                                        *(f32*)(e + 0x30) = x;
                                        *(f32*)(e + 0x34) = y;
                                        *(f32*)(e + 0x38) = z;
                                        *(f32*)(e + 0x3C) = 0.0f;
                                    }
                                }
                            }
                        }
                    }
                    s32* owner = *(s32**)spawned;
                    if (owner != 0) {
                        *owner = 0;
                        *(s32*)spawned = 0;
                    }
                }
                void* snd = *(void**)(*(u8**)(arg0 + 0x9C) + 0x1F8);
                if (snd != 0) PlaySoundAt(snd, 0, *(void**)(arg0 + 0x4), 0);
                EventSend(gEventMgr, arg0, arg0, 0x233E0AD4u, 0, 0, 1, 0);
                if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
                    ScopedLock lock;
                    ScopedLockCtor(&lock);
                    (*(VCallFn*)(*(u8**)arg0 + 0x18))(arg0, 2);
                    ScopedLockDtor(&lock, -1);
                }
            }
        }
    }

    u8* fx = *(u8**)(arg0 + 0x90);
    if (fx != 0 && scale > 0.0f && arg2 != 4) {
        const f32* col = (arg2 == 9) ? lbl_80359840 : lbl_80359850;
        *(f32*)(fx + 0x1C) = col[0];
        *(f32*)(fx + 0x20) = col[1];
        *(f32*)(fx + 0x24) = col[2];
        *(f32*)(fx + 0x28) = col[3];
    }
    if (*(u8**)(arg0 + 0x9C) != 0 && *(u8**)(arg0 + 0x90) != 0 && *(s32*)*(u8**)(arg0 + 0xA4) != 0)
        EventSend(gEventMgr, arg0, arg0, 0xC9698F8Au, arg2, (u32)scale, 1, 0);
    fn_801B62D8(arg0, arg1, arg2, arg3, arg4, scale);
    *(s32*)(arg0 + 0x600) = arg3;
    *(f32*)(arg0 + 0x604) = scale;
    *(s32*)(arg0 + 0x608) = arg2;
}

// 0x80065974
// Orients the object (entity at +0x4) along the direction vector at +0x1AC..+0x1B4 projected
// onto the xy plane, building an up vector as direction x (0,0,1) (or the current forward if the
// direction is degenerate), then applies it with fn_801458B0 and sets the dirty flags.
void fn_80065974(u8* arg0) {
    if (*(s32*)(arg0 + 0x254) == 0) return;

    f32 dx = *(f32*)(arg0 + 0x1AC);
    f32 dy = *(f32*)(arg0 + 0x1B0);
    f32 dz = *(f32*)(arg0 + 0x1B4);
    if (dx <= 0.0001f && dx >= -0.0001f && dy <= 0.0001f && dy >= -0.0001f && dz <= 0.0001f &&
        dz >= -0.0001f)
        return;

    f32 n[3];  // normalized direction
    f32 sum = dz * dz + (dx * dx + dy * dy);
    if (sum == 0.0f) {
        n[0] = n[1] = n[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(sum);
        n[0] = dx * inv;
        n[1] = dy * inv;
        n[2] = dz * inv;
    }

    f32 flat[3] = {n[0], n[1], 0.0f};
    f32 up[3];
    if (flat[0] <= 0.0001f && flat[0] >= -0.0001f && flat[1] <= 0.0001f && flat[1] >= -0.0001f &&
        flat[2] <= 0.0001f && flat[2] >= -0.0001f) {
        Vec3 fwd;
        QuatGetForward(&fwd, (const Quat*)(*(u8**)(arg0 + 0x4) + 0xC));
        up[0] = fwd.x;
        up[1] = fwd.y;
        up[2] = fwd.z;
    } else {
        f32 s2 = flat[2] * flat[2] + (flat[0] * flat[0] + flat[1] * flat[1]);
        f32 d[3];
        if (s2 == 0.0f) {
            d[0] = d[1] = d[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(s2);
            d[0] = flat[0] * inv;
            d[1] = flat[1] * inv;
            d[2] = flat[2] * inv;
        }
        // up = d x (0,0,1)
        up[0] = d[1] * 1.0f - d[2] * 0.0f;
        up[1] = d[2] * 0.0f - d[0] * 1.0f;
        up[2] = d[0] * 0.0f - d[1] * 0.0f;
    }
    f32 c[3];  // up x n
    c[0] = up[1] * n[2] - up[2] * n[1];
    c[1] = up[2] * n[0] - up[0] * n[2];
    c[2] = up[0] * n[1] - up[1] * n[0];
    fn_801458B0((f32*)(*(u8**)(arg0 + 0x4) + 0xC), n, c);
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
}

// 0x8007E0DC
// Aim-minigame setup: places the target entity (arg0+0x29C) at a world-state record xy
// position offset by the (cos/sin-blended) aim vector times a scale, flags it dirty, then runs
// AimMinigame_Update.
void fn_8007E0DC(u8* arg0) {
    fn_80070460(arg0);
    fn_8007E354(arg0, 0xCA90298Eu);
    f32 f324 = *(f32*)(arg0 + 0x324);
    fn_800712BC(arg0, (s32)f324, f324 <= 10.0f);
    f32 scale = lbl_803464E4[1] * *(f32*)(arg0 + 0xB4);
    f32 c = cosf(0.0f);
    f32 ax = *(f32*)(arg0 + 0x9C);
    f32 ay = *(f32*)(arg0 + 0xA0);
    f32 s = sinf(0.0f);
    f32 bx = *(f32*)(arg0 + 0x90);
    f32 by = *(f32*)(arg0 + 0x94);
    f32 vx = bx * s + ax * c;
    f32 vy = by * s + ay * c;

    // find the world-state record with hash 0xAAE1A370
    u8* ws = *(u8**)gWorldState;
    u32 count = *(u32*)(ws + 0x24);
    s32 idx = -1;
    for (u32 i = 0; i < count; i++) {
        if (*(u32*)(*(u8**)(ws + 0x28) + i * 0x20) == 0xAAE1A370u) {
            idx = (s32)i;
            break;
        }
    }
    u8* rec;
    if (idx >= 0 && idx < *(s32*)(ws + 0x24)) rec = *(u8**)(ws + 0x28) + idx * 0x20;
    else rec = 0;

    u8* tgt = *(u8**)(arg0 + 0x29C);
    u8* ent = *(u8**)(tgt + 0x4);
    *(f32*)(ent + 0x0) = *(f32*)(rec + 0x8) - vx * scale;
    *(f32*)(ent + 0x4) = *(f32*)(rec + 0xC) - vy * scale;
    *(f32*)(ent + 0x8) = 0.0f;
    *(u32*)(*(u8**)(tgt + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(tgt + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(tgt + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(tgt + 0x4) + 0x20) |= 0x10000000;
    AimMinigame_Update(arg0);
}

// 0x80087BFC
// Per-frame update of the countdown at +0x47C: at 0.5s left stops the sound, at 0 signals the
// game flow (win/lose compared against the ray-scene difficulty table), eases the drift value
// at +0x484 toward its per-difficulty limit, then runs the sub-updates.
void fn_80087BFC(u8* arg0, f32 dt) {
    f32 t = *(f32*)(arg0 + 0x47C) - dt;
    *(f32*)(arg0 + 0x47C) = t;
    *(f32*)(arg0 + 0x480) = *(f32*)(arg0 + 0x480) + dt;
    if (t <= 0.5f && *(s32*)(*(u8**)(arg0 + 0x354) + 0x35C) != 0) {
        fn_8008C8E4(*(u8**)(arg0 + 0x354), 0);
        fn_80089B30(arg0);
    }
    if (*(f32*)(arg0 + 0x47C) <= 0.0f) {
        s32 r = RaySceneTest(0);
        if (r < 0) r = 0;
        else if (r > 3) r = 3;
        if ((*(VCallF*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0) >= lbl_803469B8[r])
            GameFlowSignal(-0x39F737BE, 0, 0);
        else
            GameFlowSignal(0x117741C3, 0, 0);
    }
    if ((*(VCallF*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0) <= 0.0f) GameFlowSignal(0x117741C3, 0, 0);
    if (*(f32*)(arg0 + 0x47C) > 10.0f || (*(VCallF*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0) < 100.0f) {
        s32 r = RaySceneTest(0);
        if (r < 0) r = 0;
        else if (r > 3) r = 3;
        *(f32*)(arg0 + 0x484) = *(f32*)(arg0 + 0x484) - dt * lbl_80346A48[r];
        r = RaySceneTest(0);
        if (r < 0) r = 0;
        else if (r > 3) r = 3;
        f32 lim = lbl_803469A8[r];
        f32 cur = *(f32*)(arg0 + 0x484);
        f32 res;
        if (lim > 0.0f) {
            if (cur < 0.0f) res = 0.0f;
            else res = (cur > lim) ? lim : cur;
        } else {
            if (cur < lim) res = lim;
            else res = (cur > 0.0f) ? 0.0f : cur;
        }
        *(f32*)(arg0 + 0x484) = res;
    }
    *(s32*)(arg0 + 0x90) = 0;
    fn_80088004(arg0, dt);
    fn_8008853C(arg0, dt);
    fn_80088BC0(arg0, dt);
    f32 left = *(f32*)(arg0 + 0x47C);
    fn_800712BC(arg0, (s32)lrintf(left), left < 10.0f);
}

}  // extern "C"
