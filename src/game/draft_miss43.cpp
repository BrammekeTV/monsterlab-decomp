// Missing functions batch 43: sound/animation script callbacks (PlayStrikeSoundFX, PlayInjuredAnimation,
// PlaySustainedAnimation, PlayMovie, ...), positional camera shake, waypoint-path pause, path search,
// particle emitter transform.
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "game/part.h"
#include "game/character.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

void QueueMovie(const char* name, u32 duration);   // 0x8000F86C (C++ linkage, game_flow.cpp)

extern "C" {
void   CharacterStartReaction(void* ch);                              // 0x8004E3D0
void*  ChoiceDestroy(void* c, s32 del);                               // 0x8020F394
void*  EventPost(s32 id, void* sender);                               // 0x801F3918
const char* GetText(s32 id);                                          // 0x801401FC
void   LimbPlayReaction(void* limb, u8* reaction, BOOL special);      // 0x8004F398
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);        // 0x80209E70
s32    PlaySoundAt(void* sound, void* owner, const void* pos, s32 b); // 0x80209ABC
void   ReactionBuildHit(void* round, u8* out, s32 side);              // 0x80214F5C
s32    RoundGetAttacker(void* round);                                 // 0x80214F48
void   SpawnEffectAt(void* fxMgr, u32 effect, const f32 posAndScale[4], int a);   // 0x801C80E0
void   TargetListDecode(void* t, const u32* mask, s32 side);          // 0x80216240
void*  TargetListDestroy(void* t, s32 del);                           // 0x80216200
void   TargetListInit(void* t);                                       // 0x802161DC
s32    fn_8000B0E4(u8* camera, u8* movieArgs);                        // 0x8000B0E4
s32    fn_800D5FDC(PathFollower* p, u8* cur, s32 node, s32 a);        // 0x800D5FDC
s32    fn_800D61D8(PathFollower* p, u8* cur, s32 node, s32 a);        // 0x800D61D8
void   fn_8016A64C(void* sys, f32 dt);                                        // 0x8016A64C
void*  fn_801D40E0(s32 n);                                            // 0x801D40E0
s32    fn_801DC9B0(void* hash);                                       // 0x801DC9B0 (takes a u32 hash)
s32    fn_801DFAC0(u8* out, s32 a, s32 b);                            // 0x801DFAC0
s32    fn_801DFAE0(void);                                             // 0x801DFAE0
BOOL   fn_801E7460(u8* base, u16* id);                                // 0x801E7460
void   fn_801E86FC(u8* out, u8* slot, u8* obj, u8* anim);             // 0x801E86FC
s32    fn_801E9678(void* list, void* handle);                         // 0x801E9678
void*  fn_801E9D18(void* p, u32 hash, s32 size);                      // 0x801E9D18
void   fn_801EB568(u8* p, s32 a, const f32* v3, s32 b, const f32* v4, f32 f);   // 0x801EB568
s32 fn_801EB5B0(u8* arg0, u8* arg1);                                             // 0x801EB5B0 (animation callback)
s32    fn_801EC880(u8* p, u32 hash);                                  // 0x801EC880
BOOL   fn_801F5220(void);                                             // 0x801F5220
u32    fn_801F38F8(u8* a3);                                           // 0x801F38F8
void*  fn_80209814(void* v);                                          // 0x80209814
void   fn_8020A398(s32 handle);                                       // 0x8020A398
BOOL   fn_80212E28(u8** slot);                                        // 0x80212E28
s32    fn_802166AC(GameDb* db, s32 a, s32 b);                         // 0x802166AC

extern u8*  gBattle;                // 0x805FBBB8
extern u8   gGame[];                // 0x803B53A0
extern u8   lbl_803C4298[];         // 0x803C4298 (+4: camera object with vtable)
extern u8   lbl_8047C760[];         // 0x8047C760 (effect manager)
extern s32  lbl_805FB1D0;           // 0x805FB1D0 (script function index of PlayMovie)
extern u8*  lbl_805FB8D8;           // 0x805FB8D8 (animation slot table, 0x80 bytes per entry)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
}

static const u32 kHashFollowWaypointPath = 0x80B4120Fu;   // "FollowWaypointPath"
static const u32 kHashPause              = 0x20CE505Du;   // "PAUSe"
static const u32 kHashPlayMovie          = 0x0FBC624Au;   // "PlayMovie"
static const u32 kHashWaypointAnim       = 0x9028DA4Eu;

typedef u8* (*ObjGetFn)(u8*);
typedef void (*CamSetFn)(u8*, f32);

// Lazily creates the voice SFX handle (inlined in the original).
static void EnsureVoiceSfx(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// The character of the fighter opposing the owner of the battle object a0 (Fighter +0x1C points
// 0xC48 bytes into the character); *side receives that fighter's index.
static u8* OpponentCharacter(u8* a0, s32* side) {
    u8* o = (*(ObjGetFn*)(*(u8**)a0 + 0xCC))(a0);
    u8* c = *(u8**)(*(u8**)(o + 0x24) + 0x1C);
    if (c != NULL) c -= 0xC48;
    u8* info = (*(ObjGetFn*)(*(u8**)c + 0xF0))(c);
    s32 s = 1 - *(s32*)(info + 0x44);
    u8* t = *(u8**)(((u8**)(gBattle + 0xA00))[s] + 0x1C);
    if (t != NULL) t -= 0xC48;
    *side = s;
    return t;
}

extern "C" {

// 0x801B6878
// Stops the sound handle at +0xAC of obj, then (if the sound has its flag set) posts the 0x72
// event and plays the sound, either unattached (a2 != 0) or at the object. Stores the new handle.
void fn_801B6878(u8* obj, u8* sound, s32 a2, s32 a3) {
    s32 handle = *(s32*)(obj + 0xAC);
    if (handle != -1) {
        fn_8020A398(handle);
        *(s32*)(obj + 0xAC) = -1;
    }
    if (sound == NULL) {
        *(s32*)(obj + 0xAC) = -1;
        return;
    }
    if (!(*(u8*)(sound + 0x5) & 1)) {
        *(s32*)(obj + 0xAC) = -1;
        return;
    }
    if (!(*(u32*)(*(u8**)(obj + 0x4) + 0x20) & 0x04000000)) {
        if (fn_801F38F8(obj) != 0) {
            u8* ev = (u8*)EventPost(0x72, obj);
            if (ev != NULL) {
                *(s32*)(ev + 0x4) = -1;
                *(u8**)(ev + 0x8) = sound;
            }
        } else if (*(s32*)(obj + 0x30) != -1) {
            u8* ev = (u8*)EventPost(0x72, NULL);
            if (ev != NULL) {
                *(s32*)(ev + 0x4) = *(s32*)(obj + 0x30);
                *(u8**)(ev + 0x8) = sound;
            }
        }
    }
    EnsureVoiceSfx();
    if (a2 != 0) {
        *(s32*)(obj + 0xAC) = PlaySoundAt(sound, NULL, NULL, 0);
    } else {
        *(s32*)(obj + 0xAC) = PlaySoundAt(sound, obj, *(u8**)(obj + 0x4), 0);
    }
}

// 0x80174DF4
// Builds the emitter's 3x4 UV transform in params from five keyframe floats of the emitter's
// data (angle in degrees, then four scale/offset values); the first argument is unused.
void ParticleEmitterUpdate(u8* system, void* emitter, void* params, f32 dt) {
    u8* e = (u8*)emitter;
    u8* out = (u8*)params;
    fn_8016A64C(e, dt);
    u8* base = e + *(s32*)(e + 0x8);
    s32 stride = *(s32*)(e + 0x18);
    f32 v0 = *(f32*)(base + 0x8);
    f32 angle = 1.5707964f * (0.011111111f * v0);
    f32 v1 = *(f32*)(base + stride + 0x8);
    f32 v2 = *(f32*)(base + stride * 2 + 0x8);
    f32 v3 = *(f32*)(base + stride * 3 + 0x8);
    f32 v4 = *(f32*)(base + stride * 4 + 0x8);
    f32 c = cosf(angle);
    f32 s = sinf(angle);
    f32 nc = -c;
    f32 ns = -s;
    f32 m00 = c * v1;
    f32 m03 = s * v2;
    f32 hs = 0.5f * s;
    f32 m01 = ns * v1;
    f32 m10 = c * v2;
    f32 t = ((0.5f * nc) + (c * v3)) + (ns * v2);
    f32 m21 = 0.5f + (hs + ((ns * v4) + t));
    f32 u = (hs + (ns * v3)) + (nc * v2);
    f32 m22 = 0.5f + ((0.5f * c) + ((nc * v4) + u));
    *(f32*)(out + 0x0) = m00;
    *(f32*)(out + 0x8) = 0.0f;
    *(f32*)(out + 0xC) = m03;
    *(f32*)(out + 0x14) = 0.0f;
    *(f32*)(out + 0x18) = 0.0f;
    *(f32*)(out + 0x1C) = 0.0f;
    *(f32*)(out + 0x20) = 1.0f;
    *(f32*)(out + 0x2C) = 0.0f;
    *(f32*)(out + 0x4) = m01;
    *(f32*)(out + 0x10) = m10;
    *(f32*)(out + 0x24) = m21;
    *(f32*)(out + 0x28) = m22;
}

// 0x800D6534
// Starts a path search from p's current node toward the node of `other`. Gives -1 when other is
// missing, p has no node, both are at the same position, or no target node is known.
s32 PathFindToward(PathFollower* p, PathFollower* other, int a) {
    u8* pu = (u8*)p;
    if (other == NULL) return -1;
    u8* st = *(u8**)pu;
    u8* ost = *(u8**)other;
    f32 dx = *(f32*)(st + 0x18) - *(f32*)(ost + 0x18);
    f32 dy = *(f32*)(st + 0x1C) - *(f32*)(ost + 0x1C);
    f32 dz = *(f32*)(st + 0x20) - *(f32*)(ost + 0x20);
    if (*(s32*)(st + 0x14) == 0) return -1;
    BOOL samePos = FALSE;
    if (dx <= 0.0001f && dx >= -0.0001f && dy <= 0.0001f && dy >= -0.0001f &&
        dz <= 0.0001f && dz >= -0.0001f) {
        samePos = TRUE;
    }
    if (samePos) return -1;
    s32* targetNode = *(s32**)(ost + 0x14);
    if (targetNode != NULL) {
        s32 node = *targetNode;
        if (node == 0) return -1;
        memset(pu + 4, 0, 0x3FC);
        *(s32*)(pu + 0x400) = 1;
        *(s32*)(pu + 4) = *(s32*)(*(u8**)pu + 0x14);
        u8* cur = *(u8**)(*(u8**)pu + 0x14);
        if (cur != NULL && *(u32*)cur != (u32)node) {
            return fn_800D5FDC(p, cur, node, a);
        }
        return -1;
    }
    // No node on the target: search with the node id from the target's alternate list at +0x10.
    memset(pu + 4, 0, 0x3FC);
    *(s32*)(pu + 0x400) = 1;
    *(s32*)(pu + 4) = *(s32*)(*(u8**)pu + 0x14);
    u8* ost2 = *(u8**)other;
    BOOL hasAlt = FALSE;
    if (*(s32*)(ost2 + 0x14) == 0) {
        hasAlt = TRUE;
        if (*(s32*)(ost2 + 0x10) == 0) hasAlt = FALSE;
    }
    u8* cur = *(u8**)(*(u8**)pu + 0x14);
    s32 node = 0;
    if (hasAlt) node = **(s32**)(ost2 + 0x10);
    return fn_800D61D8(p, cur, node, a);
}

// 0x801B8B3C
// Pauses the FollowWaypointPath animation of the object: finds its slot, queues the PAUSe
// handle, then fills a fresh animation record from the object's position and orientation and
// runs fn_801E86FC on it, then sets bits 0x10000000, 0x8000 and 0x4000 in the flags at def+0x20.
void PauseFollowingWaypointPath(void* a0, f32 a1) {
    u8* obj = (u8*)a0;
    u8* follow = (u8*)fn_801E9D18(obj + 0xFC, kHashFollowWaypointPath, 0x70);
    if (follow == NULL) return;
    u16 id = *(u16*)(follow + 0x7C);
    u8* tbl = lbl_805FB8D8;
    u16 idCopy = id;
    u8* slot;
    if (fn_801E7460(tbl, &idCopy) != 0) {
        slot = tbl + ((id & 0xFF) << 7);
    } else {
        slot = NULL;
    }
    u8 anim[0x80];
    memset(anim, 0, 0x64);
    *(s32*)(anim + 0x64) = 0;
    *(s32*)(anim + 0x68) = 0;
    *(void**)(anim + 0x6C) = (void*)&fn_801EB5B0;   // lint:ok callback slot (32-bit on the GC layout)
    *(f32*)(anim + 0x70) = 0.0f;
    *(s32*)(anim + 0x74) = 0;
    *(u16*)(anim + 0x78) = 0xFFFF;
    *(u16*)(anim + 0x7A) = 0xFFFF;
    *(u16*)(anim + 0x7C) = 0xFFFF;
    *(u8*)(anim + 0x7E) = 0;
    *(u8*)(anim + 0x7F) = 0;
    u8* pause = (u8*)fn_801E9D18(slot, kHashPause, 0xF8);
    if (pause != NULL) {
        u16 handle = *(u16*)(pause + 0x7C);
        fn_801E9678(slot, &handle);
    }
    u8* wp = (u8*)fn_801E9D18(slot, kHashWaypointAnim, 0x78);
    if (wp == NULL) return;
    f32 v4[4];
    f32 v3[3];
    u8* def = *(u8**)(obj + 0x4);
    v4[0] = *(f32*)(def + 0xC);
    v4[1] = *(f32*)(def + 0x10);
    v4[2] = *(f32*)(def + 0x14);
    v4[3] = *(f32*)(def + 0x18);
    def = *(u8**)(obj + 0x4);
    v3[0] = *(f32*)(def + 0x0);
    v3[1] = *(f32*)(def + 0x4);
    v3[2] = *(f32*)(def + 0x8);
    fn_801EB568(anim, *(s32*)(wp + 0x1C), v3, *(s32*)(wp + 0x2C), v4, a1);
    u8 result[0x10];
    fn_801E86FC(result, slot, obj, anim);
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
}

// 0x800F9550
// Plays the hit reaction on the opposing fighter's limbs once its pending-injury flag (+0xC86)
// is set, then clears the flag.
void PlayInjuredAnimation(void* a0) {
    if (*(u8*)(gGame + 0x24) != 0) return;
    if (gBattle == NULL) return;
    s32 side;
    u8* ch = OpponentCharacter((u8*)a0, &side);
    if (*(u8*)(ch + 0xC86) == 0) return;
    u8* round = *(u8**)(gBattle + 0xA0C);
    if (round != NULL) {
        u8 reactions[0x500];
        ReactionBuildHit(round, reactions, side);
        for (s32 k = 0; k < 5; k++) {
            u8** limbs = (u8**)(*(ObjGetFn*)(*(u8**)ch + 0xF0))(ch);
            u8* part = limbs[k];
            if (part != NULL) {
                u8* limb = *(u8**)(part + 0x20);
                if (limb != NULL) limb -= 0x264;
                LimbPlayReaction(limb, reactions + k * 0x100, 0);
            }
        }
        CharacterStartReaction(ch);
    }
    *(u8*)(ch + 0xC86) = 0;
}

// 0x800E76D0
// Script PlayMovie: in mode 0 queues the movie whose name is the text of id a0; in mode 3
// (when fn_801F5220) builds a movie request for the PlayMovie script function and hands it to
// the camera object.
void PlayMovie(u32 /*0x11*/ a0) {
    switch (fn_801DFAE0()) {
    case 0:
        QueueMovie(GetText((s32)a0), 0);
        return;
    case 3:
        if (fn_801F5220() != 0) {
            u8 request[0x18];
            if (lbl_805FB1D0 == 0) {
                lbl_805FB1D0 = fn_801DC9B0((void*)(uintptr_t)kHashPlayMovie);
            }
            fn_801DFAC0(request, lbl_805FB1D0, (s32)a0);
            fn_8000B0E4(lbl_803C4298, request);
        }
        return;
    }
}

// 0x800F4FFC
// Plays the strike sound of an attack: looks up the attacker's part and the target part from
// the current round, and plays the sound that fn_802166AC finds for the pair of part values.
void PlayStrikeSoundFX(void* a0) {
    struct Choice { s32 action; u8 flag; u8 pad[3]; s32 target; };
    struct TargetList { s32 ids[5]; s32 count; };
    Choice choice;
    TargetList targets;
    PartDef attackerDef;
    PartDef targetDef;
    u32 mask;

    if (gBattle == NULL) return;
    u8* round = *(u8**)(gBattle + 0xA0C);
    if (round == NULL) return;
    if (*(u32*)round > 2U) return;
    s32 attacker = RoundGetAttacker(round);
    u8* rec = *(u8**)(gBattle + 0xA0C) + attacker * 0x38;
    choice.action = *(s32*)(rec + 0x30);
    choice.flag = *(u8*)(rec + 0x34);
    s32 target = *(s32*)(rec + 0x38);
    choice.target = target;
    if (target == -1) {
        ChoiceDestroy(&choice, -1);
        return;
    }
    u8** attackerParts = (u8**)((u8**)(gBattle + 0xA00))[attacker];
    u8* attackerPart;
    if (attackerParts == NULL || (attackerPart = attackerParts[target]) == NULL) {
        ChoiceDestroy(&choice, -1);
        return;
    }
    PartDbGetDef(&attackerDef, gGameDb, **(s32**)attackerPart);
    u8* actions = *(u8**)(gGameDb->data + 0x430);
    TargetListInit(&targets);
    mask = *(u32*)(actions + choice.action * 0x14 + 0x8);
    TargetListDecode(&targets, &mask, target);
    s32 first = targets.ids[0];
    if (first == -1) {
        TargetListDestroy(&targets, -1);
        ChoiceDestroy(&choice, -1);
        return;
    }
    u8** targetParts = (u8**)((u8**)(gBattle + 0xA00))[1 - attacker];
    u8* targetPart;
    if (targetParts == NULL || (targetPart = targetParts[first]) == NULL) {
        TargetListDestroy(&targets, -1);
        ChoiceDestroy(&choice, -1);
        return;
    }
    PartDbGetDef(&targetDef, gGameDb, **(s32**)targetPart);
    if (*(s32*)((u8*)gGameDb + 0x10) == 1) {
        u32 sound = (u32)fn_802166AC(gGameDb, attackerDef.unk3C, targetDef.unk28);
        EnsureVoiceSfx();
        PlaySound(sound, 0, 0, 0, 0, 0);
    }
    TargetListDestroy(&targets, -1);
    ChoiceDestroy(&choice, -1);
}

// 0x800F942C
// Marks the opposing fighter's sustained-reaction flag (+0xC86) so its reaction keeps playing;
// only when no round is active or the round state is 0-2.
void PlaySustainedAnimation(void* a0) {
    if (*(u8*)(gGame + 0x24) != 0) return;
    if (gBattle == NULL) return;
    u8* round = *(u8**)(gBattle + 0xA0C);
    if (round == NULL || *(u32*)round <= 2U) {
        s32 side;
        u8* ch = OpponentCharacter((u8*)a0, &side);
        *(u8*)(ch + 0xC86) = 1;
    }
}

// 0x800F4B94
// Spawns effect a1 at the emitter named a2 (record 0xC8 bytes, position at +0x5C) of the object,
// unless the object is a battle character whose slot reports busy.
void PlayUnattachedParticleEffectAtEmitter(void* a0, u32 a1, u32 a2) {
    u8* obj = (u8*)a0;
    if (obj == NULL) return;
    if ((*(u32*)(*(u8**)(obj + 0x4) + 0x1C) & 0x1000) &&
        (*(ObjGetFn*)(*(u8**)obj + 0xCC))(obj) != NULL && gBattle != NULL &&
        fn_80212E28((u8**)((*(ObjGetFn*)(*(u8**)obj + 0xCC))(obj) + 0x28)) != 0) {
        return;
    }
    u8* list = obj + 0x94;
    s32 idx = fn_801EC880(list, a2);
    if (idx < 0) return;
    u8* emitter = *(u8**)(list + 0x4) + idx * 0xC8;
    f32 posAndScale[4];
    posAndScale[0] = *(f32*)(emitter + 0x5C);
    posAndScale[1] = *(f32*)(emitter + 0x60);
    posAndScale[2] = *(f32*)(emitter + 0x64);
    posAndScale[3] = 0.0f;
    SpawnEffectAt(lbl_8047C760, a1, posAndScale, 0);
}

// 0x800E7B34
// Camera shake with strengths a2/a3/a4 for objects within radius a1 of the camera node: queues
// event 0x25 when the camera is in event mode, otherwise calls the camera's three shake setters.
void PositionalCameraShake(void* a0, f32 a1, f32 a2, f32 a3, f32 a4) {
    if (fn_801F5220() == 0) return;
    u8* pos = *(u8**)((u8*)a0 + 0x4);
    u8* cam = *(u8**)(*(u8**)(gGame + 0xD614) + 0x4);
    f32 dx = *(f32*)(pos + 0x0) - *(f32*)(cam + 0x0);
    f32 dy = *(f32*)(pos + 0x4) - *(f32*)(cam + 0x4);
    f32 dz = *(f32*)(pos + 0x8) - *(f32*)(cam + 0x8);
    if (!((dz * dz) + ((dx * dx) + (dy * dy)) < (a1 * a1))) return;
    if (*(u32*)(cam + 0x20) & 0x04000000) {
        u8* ev = (u8*)EventPost(0x25, a0);
        if (ev != NULL) {
            *(f32*)(ev + 0x4) = a2;
            *(f32*)(ev + 0x8) = a3;
            *(f32*)(ev + 0xC) = a4;
        }
    } else {
        u8* camObj = *(u8**)(lbl_803C4298 + 0x4);
        (*(CamSetFn*)(*(u8**)camObj + 0xA0))(camObj, a2);
        camObj = *(u8**)(lbl_803C4298 + 0x4);
        (*(CamSetFn*)(*(u8**)camObj + 0xA8))(camObj, a3);
        camObj = *(u8**)(lbl_803C4298 + 0x4);
        (*(CamSetFn*)(*(u8**)camObj + 0xA4))(camObj, a4);
    }
}

}  // extern "C"
