// Missing functions batch 13: brain lab (head, hand, power-up, target, tossed chunk), brazier bowl and
// symbol script callbacks, effect emitter lookup, hit-box toggle, quaternion rotate, reference position.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "game/node_ref.h"

extern "C" {
#include "game/script_callbacks.h"

void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u32   RandomU32(void);                                      // 0x80143460
void  fn_800493A8(s32 on);                                  // 0x800493A8
void  fn_8004947C(void);                                    // 0x8004947C
void  fn_80049494(void);                                    // 0x80049494
u8*   fn_8006FBDC(void);                                    // 0x8006FBDC
void  fn_8007EF04(u8* mgr, u8* chunk, s32 idx, f32 t);      // 0x8007EF04
u8*   fn_80080C5C(u8* self);                                // 0x80080C5C
s32   fn_80083930(u8* self, u8* pos);                       // 0x80083930
void  fn_80139FD0(s32 on);                                  // 0x80139FD0
void  fn_8013C444(s32 on);                                  // 0x8013C444
void  fn_8013E264(s32 on);                                  // 0x8013E264
void  fn_8013EADC(s32 on);                                  // 0x8013EADC
u8*   fn_801B161C(u8* p, u32 key);                          // 0x801B161C
void* fn_801B18E4(u8* list);                                // 0x801B18E4
void* fn_801B50FC(u8* p);                                   // 0x801B50FC (returns the object's transform matrix)
void* fn_801B7198(void* anim, s32 a);                       // 0x801B7198
u8*   fn_801C668C(u8* key);                                 // 0x801C668C
u8*   fn_801C6BCC(u8* key);                                 // 0x801C6BCC
s32   fn_801C71EC(u8* key);                                 // 0x801C71EC

extern u8  gWorldState[];                // 0x803E6D00 (+0xC frame delta; first word points at the world data)
extern "C++" void* gEventMgr;            // 0x805FB6C8
extern const u32 lbl_803881B4[];         // 0x803881B4 (emitter id -> emitter hash)
extern s32 lbl_805F90D8;                 // 0x805F90D8 (current screen hash)
extern s32 lbl_805FB6B8;                 // 0x805FB6B8
extern u8  lbl_805FB710[];               // 0x805FB710 (default id record)
}

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtGuard(f32 v) {
    return v <= 0.0f ? 0.0f : sqrtf(v);
}

// Marks the object's transform (sub-object at +4) as changed: the four flag ORs the engine does after moving it.
static inline void MarkTransformDirty(u8* self) {
    *(u32*)(*(u8**)(self + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(self + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(self + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(self + 0x4) + 0x20) |= 0x10000000;
}

// Packed id record used by fn_801ED788: byte, byte, halfword.
struct IdRec {
    u8  a;
    u8  b;
    u16 c;
};

extern "C" {

// 0x800523B8
// Returns the emitter hash for effect id `effect`. Limbs 2 and 3 use a per-effect pair of hashes
// (effects 0x1E..0x26, limb 3 gets the second one); other limbs index the emitter table.
u32 EffectEmitter(void* db, s32 effect, s32 limb) {
    (void)db;
    if (limb != 3 && limb != 2) {
        if (effect == -1 || effect >= 0x28) return 0;
        return lbl_803881B4[effect];
    }
    switch ((u32)(effect - 0x1E)) {
    case 0: return limb != 3 ? 0xCE4CC037u : 0x163C6F3Eu;
    case 1:
    case 7:
    case 8: return limb != 3 ? 0xFBDEF319u : 0xF913A97Bu;
    case 2: return limb != 3 ? 0xD55155FFu : 0x16DBEE1Fu;
    case 3: return limb != 3 ? 0xC0F0855Fu : 0x1C85365Du;
    case 4: return limb != 3 ? 0x21EC130Fu : 0xA7D952F2u;
    case 5: return limb != 3 ? 0xBACCDB01u : 0x1972162Fu;
    case 6: return limb != 3 ? 0x0C9C52B7u : 0xCF16E957u;
    default: return 0;
    }
}

// 0x8007EEB8
// Throws a brain chunk: forwards to fn_8007EF04 with the brain lab object and the index stored in it.
void BrainTossThrowBrainChunk(u32 /*0x56*/ a0, f32 a1) {
    u8* lab = fn_8006FBDC();
    fn_8007EF04(lab, (u8*)(uintptr_t)a0, *(s32*)(*(u8**)(lab + 0x1F4) + 0x268), a1);
}

// 0x80083798
// Tail-calls virtual +0xC4 (pick a wait time) on the brain target.
void BrainTargetGenerateWaitTime(u32 /*0x57*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    (*(void (**)(u8*))(*(u8**)self + 0xC4))(self);
}

// 0x800837FC
// Tail-calls virtual +0xC8 (pick a destination) on the brain target.
void BrainTargetGenerateDestination(u32 /*0x57*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    (*(void (**)(u8*))(*(u8**)self + 0xC8))(self);
}

// 0x80083830
// Returns 1 once the target is within a tiny squared distance of its destination (+0x274); then snaps to it.
u32 /*0x06*/ BrainTargetDestinationReached(u32 /*0x57*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    u8* pos = *(u8**)(self + 0x4);
    f32 dx = *(f32*)(self + 0x274) - *(f32*)(pos + 0x0);
    f32 dy = *(f32*)(self + 0x278) - *(f32*)(pos + 0x4);
    f32 dz = *(f32*)(self + 0x27C) - *(f32*)(pos + 0x8);
    f32 d2 = dz * dz + (dx * dx + dy * dy);
    bool reached = d2 > -0.0001f && d2 < 0.0001f;
    if (reached) {
        *(f32*)(pos + 0x0) = *(f32*)(self + 0x274);
        *(f32*)(pos + 0x4) = *(f32*)(self + 0x278);
        *(f32*)(pos + 0x8) = *(f32*)(self + 0x27C);
        MarkTransformDirty(self);
    }
    return reached;
}

// 0x80084650
// Head follows the meat: copies the tracked chunk position to +0x274 and sets the flag byte at +0x28C
// when fn_80083930 gives the same result for that position and for the head's own position.
void BrainHeadFollowMeat(u32 /*0x58*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    u8* tracked = *(u8**)(self + 0x264);
    u8* chunk = tracked ? fn_80080C5C(tracked) : NULL;
    u8* src = *(u8**)(chunk + 0x4);
    u8* myPos = *(u8**)(self + 0x4);
    *(f32*)(self + 0x274) = *(f32*)(src + 0x0);
    *(f32*)(self + 0x278) = *(f32*)(src + 0x4);
    *(f32*)(self + 0x27C) = *(f32*)(src + 0x8);
    s32 a = fn_80083930(self, self + 0x274);
    s32 b = fn_80083930(self, myPos);
    *(u8*)(self + 0x28C) = (a == b);
}

// 0x80084D18
// Sets the "freebie is next" flag byte; returns the object.
u32 /*0x06*/ BrainHeadSetFreebieIsNext(u32 /*0x58*/ a0, u32 /*0x06*/ a1) {
    *(u8*)((u8*)(uintptr_t)a0 + 0x285) = (a1 != 0);
    return a0;
}

// 0x80084DA8
// Plays one of three idle animations at random through virtual +0x88.
void BrainHeadPlayIdleAnim(u32 /*0x58*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    u32 r = RandomU32() % 3;
    u32 anim;
    switch (r) {
    case 0: anim = 0xEAB158A1u; break;  // "idle"
    case 1: anim = 0x6681BC3Bu; break;
    case 2: anim = 0x11868CADu; break;
    default: return;
    }
    (*(void (**)(u8*, u32))(*(u8**)self + 0x88))(self, anim);
}

// 0x80086090
// Power-up arrival: after 0.5s of time at +0x268 it sits at its target (+0x274); before that it hovers
// 10 units above, dropping to the target.
void BrainPowerUpArrive(u32 /*0x59*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    f32 t = *(f32*)(self + 0x268) + *(f32*)(gWorldState + 0xC);
    f32 x = *(f32*)(self + 0x274);
    f32 y = *(f32*)(self + 0x278);
    f32 z = *(f32*)(self + 0x27C);
    f32 s = t / 0.5f;
    *(f32*)(self + 0x268) = t;
    if (s < 1.0f) {
        f32 lift = 10.0f * (1.0f - s);
        x = x + 0.0f;
        y = y + 0.0f;
        z = z + lift;
    }
    u8* pos = *(u8**)(self + 0x4);
    *(f32*)(pos + 0x0) = x;
    *(f32*)(pos + 0x4) = y;
    *(f32*)(pos + 0x8) = z;
    MarkTransformDirty(self);
}

// 0x80086650
// Returns 1 when the power-up's distance (in the xy plane) to the head's "legs" node is below the sum of
// the two radii (+0x3C of each).
u32 /*0x06*/ BrainPowerUpIsTouchingHead(u32 /*0x59*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    u8* mine = (u8*)fn_801B18E4(*(u8**)(self + 0x8C));
    u8* head = *(u8**)(self + 0x264);
    u8* list = *(u8**)(*(u8**)(head + 0x29C) + 0x8C);
    u8* first = (*(s32*)(list + 0x10) == 0) ? NULL : **(u8***)(list + 0xC);
    u8* legs = fn_801B161C(first, 0xCB8DAE3Eu);  // "legs"
    f32 dx = *(f32*)(legs + 0x14) - *(f32*)(mine + 0x14);
    f32 dy = *(f32*)(legs + 0x18) - *(f32*)(mine + 0x18);
    f32 dz = 0.0f;
    f32 dist = SqrtGuard(dz * dz + (dx * dx + dy * dy));
    return dist < *(f32*)(mine + 0x3C) + *(f32*)(legs + 0x3C);
}

// 0x800869A0
// Keeps the chunk held by the hand (+0x264) at the hand's attachment point: finds the entry with key
// 0x1DC9D499 in the hand's table (virtual +0x10), transforms its local point by the hand's matrix, then
// copies the hand's orientation (+0xC..+0x18) onto the chunk.
void BrainHandUpdateHeldChunk(u32 /*0x5A*/ a0) {
    u8* self = (u8*)(uintptr_t)a0;
    u8* held = *(u8**)(self + 0x264);
    if (held == NULL) return;

    u8* ret = (*(u8* (**)(u8*))(*(u8**)self + 0x10))(self);
    u8* tbl = *(u8**)ret;
    s32 idx = 0;
    s32 off = 0;
    s32 count = *(s32*)(tbl + 0x8);
    s32 found = -1;
    if (count > 0) {
        for (s32 n = count; n > 0; n--) {
            if (*(u32*)(*(u8**)(tbl + 0xC) + off) == 0x1DC9D499u) {
                found = idx;
                break;
            }
            off += 8;
            idx++;
        }
    }
    u8* entry = *(u8**)(tbl + 0x4) + found * 0x30;
    f32 x = *(f32*)(entry + 0x24);
    f32 y = *(f32*)(entry + 0x28);
    f32 z = *(f32*)(entry + 0x2C);

    f32* m = (f32*)fn_801B50FC(self);
    f32 ox = m[9]  + (m[6] * z + (m[0] * x + m[3] * y));
    f32 oy = m[10] + (m[7] * z + (m[1] * x + m[4] * y));
    f32 oz = m[11] + (m[8] * z + (m[2] * x + m[5] * y));
    u8* hpos = *(u8**)(held + 0x4);
    *(f32*)(hpos + 0x0) = ox;
    *(f32*)(hpos + 0x4) = oy;
    *(f32*)(hpos + 0x8) = oz;
    MarkTransformDirty(held);

    fn_801B50FC(self);
    u8* src = *(u8**)(self + 0x4);
    u8* dst = *(u8**)(held + 0x4);
    f32 q0 = *(f32*)(src + 0xC);
    f32 q1 = *(f32*)(src + 0x10);
    f32 q2 = *(f32*)(src + 0x14);
    f32 q3 = *(f32*)(src + 0x18);
    *(f32*)(dst + 0xC) = q0;
    *(f32*)(dst + 0x10) = q1;
    *(f32*)(dst + 0x14) = q2;
    *(f32*)(dst + 0x18) = q3;
    MarkTransformDirty(held);
}

// 0x8008CB28
// Fire size class from the bowl's fire amount (+0x268): 0 up to 15, 1 up to 70, else 2.
s32 BrazierBowlGetFireSize(void* a0) {
    f32 fire = *(f32*)((u8*)a0 + 0x268);
    if (fire <= 15.0f) return 0;
    if (fire <= 70.0f) return 1;
    return 2;
}

// 0x8008CB64
// Explosion size class from the bowl's level (+0x264): 0 up to 1, 1 for 2..3, 2 from 4.
s32 BrazierBowlGetExplosionSize(void* a0) {
    s32 level = *(s32*)((u8*)a0 + 0x264);
    if (level <= 1) return 0;
    return level >= 4 ? 2 : 1;
}

// 0x8008E8C8
// Resets a symbol to the unused state: scale 1, hidden at (0,-5,-5), motion fields cleared, direction -1.
void BrazierSymbolSetUnused(void* a0) {
    u8* self = (u8*)a0;
    *(s32*)(self + 0x264) = 1;
    *(u8*)(self + 0x2D0) = 0;
    *(f32*)(self + 0x2D4) = 1.0f;
    *(f32*)(self + 0x8) = 1.0f;
    *(f32*)(self + 0xC) = 1.0f;
    *(f32*)(self + 0x10) = 1.0f;
    u8* pos = *(u8**)(self + 0x4);
    *(f32*)(pos + 0x0) = 0.0f;
    *(f32*)(pos + 0x4) = -5.0f;
    *(f32*)(pos + 0x8) = -5.0f;
    MarkTransformDirty(self);
    *(f32*)(self + 0x2E0) = 0.0f;
    *(f32*)(self + 0x2E4) = 0.0f;
    *(f32*)(self + 0x2E8) = 0.0f;
    *(f32*)(self + 0x2EC) = 0.0f;
    *(f32*)(self + 0x2F0) = 0.0f;
    *(f32*)(self + 0x2F4) = 0.0f;
    *(f32*)(self + 0x2F8) = -1.0f;
    *(f32*)(self + 0x2FC) = -1.0f;
    *(f32*)(self + 0x300) = -1.0f;
    *(f32*)(self + 0x2B4) = 0.0f;
    *(f32*)(self + 0x2A4) = 0.0f;
    *(u8*)(self + 0x310) = 0;
    if (*(u8*)(self + 0x274) != 0) {
        *(f32*)(self + 0x278) = 0.0f;
        *(s32*)(self + 0x268) = 1;
    }
}

// 0x8008EA00
// Shrinks the symbol by 0.05 per call (scale +0x2D4, clamped to 0..1, applied to +8/+C/+10). Below 0.1 it
// snaps to 0.1; at or below 0.1 it sends event 0xE528A45C to itself.
void BrazierSymbolUpdateDisappearing(void* a0) {
    u8* self = (u8*)a0;
    *(s32*)(self + 0x264) = 0;
    f32 s = *(f32*)(self + 0x2D4) - 0.05f;
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    *(f32*)(self + 0x2D4) = s;
    *(f32*)(self + 0x8) = s;
    *(f32*)(self + 0xC) = s;
    *(f32*)(self + 0x10) = s;
    if (s < 0.1f) {
        s = 0.1f;
        *(f32*)(self + 0x2D4) = s;
        *(f32*)(self + 0x8) = s;
        *(f32*)(self + 0xC) = s;
        *(f32*)(self + 0x10) = s;
    }
    MarkTransformDirty(self);
    if (*(f32*)(self + 0x2D4) <= 0.1f) {
        EventSend(gEventMgr, self, NULL, 0xE528A45Cu, 0, 0, 1, 0);
    }
}

// 0x8008EB58
// Starts moving back: state 8, jumps to the home object's (+0x284) position, then sets the direction
// (+0x2AC, -1/0/+1 from the sign of x distance to +0x2C4) and speed (+0x2B4 = 10 * direction).
void BrazierSymbolStartMovingBack(void* a0) {
    u8* self = (u8*)a0;
    *(s32*)(self + 0x264) = 8;
    u8* pos = *(u8**)(self + 0x4);
    u8* home = *(u8**)(*(u8**)(self + 0x284) + 0x4);
    *(f32*)(pos + 0x0) = *(f32*)(home + 0x0);
    *(f32*)(pos + 0x4) = *(f32*)(home + 0x4);
    *(f32*)(pos + 0x8) = *(f32*)(home + 0x8);
    MarkTransformDirty(self);
    f32 dir = 0.0f;
    f32 d = *(f32*)(self + 0x2C4) - *(f32*)(*(u8**)(self + 0x4) + 0x0);
    if (d != 0.0f) {
        dir = (d < 0.0f) ? -1.0f : 1.0f;
    }
    *(f32*)(self + 0x2AC) = dir;
    *(f32*)(self + 0x2B4) = 10.0f * dir;
}

// 0x8008EC64
// Maps the symbol's model hash (+0x304) to a type: ball (water 2, fire 3, earth/air symbols 4/5),
// 1 for castle_good_ballA, 0 for anything else.
s32 BrazierSymbolGetSymbolHashType(void* a0) {
    switch (*(u32*)((u8*)a0 + 0x304)) {
    case 0x67B779E5u: return 1;  // "castle_good_ballA"
    case 0x8F25BB8Au: return 2;  // "castle_water_ballA"
    case 0xBAA6441Fu: return 3;  // "castle_fire_ballA"
    case 0xB57069FEu: return 4;  // "castle_earth_symbolA"
    case 0xA65DCF50u: return 5;  // "castle_air_symbolA"
    default: return 0;           // includes 0xF3B2D25E "castle_bad_symbolA"
    }
}

// 0x80140B9C
// Enables or disables hit boxes for the active screen (switch on its hash); the combat screen also
// toggles combat HUD input.
void SetHitBoxes(BOOL a0) {
    s32 on = (a0 != 0) ? 1 : 0;
    switch ((u32)lbl_805F90D8) {
    case 0xF5F0817Au:
        FriendListHideFloaty(1);
        fn_8013C444(on);
        break;
    case 0x9D31BDABu:
        fn_8013E264(on);
        break;
    case 0x13E20C39u:
        fn_8013EADC(on);
        break;
    case 0x89CE136Au:
        fn_80139FD0(on);
        break;
    case 0xE38C521Fu:
        fn_800493A8(on);
        if (on == 0) {
            fn_8004947C();
            CombatHUDDisableInput(0xAD4C5703u);
        } else {
            fn_80049494();
            CombatHUDEnableInput();
        }
        break;
    default:
        break;
    }
}

// 0x80146180
// Rotates quaternion q (x, y, z, w at +0, +4, +8, +C) about `axis` by an angle given in quarter turns
// (wrapped into 0..4, tiny values treated as 0); result is the product written back into q.
void fn_80146180(void* quat, const Vec3* axis, f32 angle) {
    f32* q = (f32*)quat;
    if (angle > -0.0001f && angle < 0.0001f) {
        angle = 0.0f;
    } else if (angle > 4.0f) {
        angle -= 4.0f;
    } else if (angle < 0.0f) {
        angle += 4.0f;
    }
    f32 half = 1.5707964f * (0.5f * angle);
    f32 c = (f32)cosf(half);
    f32 sn = (f32)sinf(half);
    f32 ax = sn * axis->x;
    f32 ay = sn * axis->y;
    f32 az = sn * axis->z;
    f32 q0 = q[0], q1 = q[1], q2 = q[2], q3 = q[3];
    f32 n0 = (ay * q2 + (c * q0 + ax * q3)) - az * q1;
    f32 n3 = ((c * q3 - ax * q0) - ay * q1) - az * q2;
    f32 n1 = az * q0 + (ay * q3 + (c * q1 - ax * q2));
    f32 n2 = az * q3 + ((c * q2 + ax * q1) - ay * q0);
    q[0] = n0;
    q[3] = n3;
    q[1] = n1;
    q[2] = n2;
}

// 0x801ED788
// Resolves a packed reference (type in the first byte: 1 = anim/object lookup by hash, 2 / 3 = id record
// lookups, 4 = world entry by index) to a position and writes x, y, z to out; 0,0,0 when not found.
// The reference word is read as a whole so the code does not depend on byte order.
void fn_801ED788(f32* out, u8* ref) {
    u32 w = NodeRefWord(ref);
    switch (NodeRefKind(w)) {
    case 1: {
        u32 hash = (u32)lbl_805FB6B8;
        if (NodeRefIsId(w)) hash = w;
        u8* node = (u8*)fn_801B7198(&hash, 0);
        if (node != NULL) {
            u8* src = *(u8**)(node + 0x4);
            out[0] = *(f32*)(src + 0x0);
            out[1] = *(f32*)(src + 0x4);
            out[2] = *(f32*)(src + 0x8);
            return;
        }
        break;
    }
    case 2: {
        IdRec rec;
        rec.a = lbl_805FB710[0];
        rec.b = lbl_805FB710[1];
        rec.c = *(u16*)(lbl_805FB710 + 2);
        if (NodeRefKind(w) == 2) {
            rec.b = NodeRefByte1(w);
            rec.a = (u8)NodeRefNibble(w);
            rec.c = NodeRefHalf2(w);
        }
        IdRec use;
        if (fn_801C71EC((u8*)&rec) != 0) {
            use = rec;
        } else {
            use.a = lbl_805FB710[0];
            use.b = lbl_805FB710[1];
            use.c = *(u16*)(lbl_805FB710 + 2);
        }
        u8* found = fn_801C6BCC((u8*)&use);
        if (found != NULL) {
            out[0] = *(f32*)(found + 0x18);
            out[1] = *(f32*)(found + 0x1C);
            out[2] = *(f32*)(found + 0x20);
            return;
        }
        break;
    }
    case 3: {
        IdRec rec;
        rec.a = 0xFF;
        rec.b = 0;
        rec.c = 0;
        if (NodeRefKind(w) == 3) {
            rec.a = (u8)NodeRefHalf2(w);
            rec.b = NodeRefByte1(w);
            rec.c = (u16)(u8)NodeRefNibble(w);
        }
        u8* found = fn_801C668C((u8*)&rec);
        if (found != NULL) {
            out[0] = *(f32*)(found + 0x4);
            out[1] = *(f32*)(found + 0x8);
            out[2] = *(f32*)(found + 0xC);
            return;
        }
        break;
    }
    case 4: {
        u32 idx = (NodeRefKind(w) == 4) ? (u32)NodeRefHalf2(w) : 0xFFFFFFFFu;
        u8* world = *(u8**)gWorldState;
        u8* entry = NULL;
        if (idx < *(u32*)(world + 0x24)) {
            entry = *(u8**)(world + 0x28) + (idx << 5);
        }
        if (entry != NULL) {
            out[0] = *(f32*)(entry + 0x8);
            out[1] = *(f32*)(entry + 0xC);
            out[2] = *(f32*)(entry + 0x10);
            return;
        }
        break;
    }
    default:
        break;
    }
    out[2] = 0.0f;
    out[1] = 0.0f;
    out[0] = 0.0f;
}

}  // extern "C"
