// Small functions in 0x801DBA3C-0x801DDD60: frame-effect script callbacks, random intervals,
// particle and sound script callbacks, global music/SFX volume, script-table lookup,
// activate/deactivate handlers and the cosine interpolation helper.
#include "types.h"

// Script table entry (see src/engine/small_801F.cpp and script_dispatch.cpp).
struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
// Callees (prototypes copied from their definitions or callers).
u32   RandomU32(void);                                              // 0x80143460
s32  fn_8016195C(void* p, s32 a, s32 b, void* c);                  // 0x8016195C
s32   fn_80162B6C(void* obj, s32 n);                                // 0x80162B6C
void  fn_801629D0(void* obj, s32 n);                                // 0x801629D0
void  fn_801ED788(f32* out, u8* a1);                                // 0x801ED788
void  SpawnEffectAt(void* fxMgr, u32 effect, const f32 posAndScale[4], int a); // 0x801C80E0
void* EventPost(s32 id, void* sender);                              // 0x801F3918
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);      // 0x80209ABC
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);       // 0x80209E70
void  fn_8020A398(s32 handle);                                      // 0x8020A398
void* fn_801D40E0(s32 n);                                           // 0x801D40E0
void* fn_80209814(void* v);                                         // 0x80209814
void  fn_8020B024(f32 v);                                           // 0x8020B024
f32   fn_8020B05C(void);                                            // 0x8020B05C
f32   fn_8020B154(void);                                            // 0x8020B154
void  TriggerDoAction(void* action, void* a4, void* a5);            // 0x801E0054
f32   cosf(f32 x);                                                  // 0x8033EC98 (MSL)

// Globals.
extern u8  lbl_8040E5F0[];          // 0x8040E5F0 (frame effects object)
extern u8  lbl_8047C760[];          // 0x8047C760 (effect manager)
extern u8  lbl_805FB860;            // 0x805FB860 (challenge frame-fx active flag)
extern s32 lbl_805F9980;            // 0x805F9980 (looping group sound handle, -1 when none)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern s32 lbl_805FB880;            // 0x805FB880 (script table entry count)
extern ScriptFunc gScriptFuncs[];   // 0x80484230

extern f32 lbl_805FFF78;            // 0x805FFF78 (float, 4th component of the spawn position)
extern f32 lbl_805FFF9C;            // 0x805FFF9C (float, vortex scale)
extern f32 lbl_805FFFA0;            // 0x805FFFA0 (float, u32 -> [0,1) scale)
extern f32 lbl_805FFFB0;            // 0x805FFFB0 (float, cosine phase scale)
extern f32 lbl_805FFFB4;            // 0x805FFFB4 (float, cosine phase scale)
extern f32 lbl_805FFFB8;            // 0x805FFFB8 (float, interpolation weight scale)
extern f32 lbl_805FFFBC;            // 0x805FFFBC (float, interpolation constant)
}

// Frame effects: each one fills a float parameter block and starts effect 0xC on the
// frame-effects object. The trailing integer argument is stored by the original but never used.
// 0x801DBA3C
void FrameFxBlurBulge(f32 a0, f32 a1, f32 a2, f32 a3, s32 a4) {
    f32 v[4] = { a0, a1, a2, a3 };
    (void)a4;
    fn_8016195C(lbl_8040E5F0, 0xC, 0, v);
}

// Vortex effect (id 0xD). The third parameter is scaled by lbl_805FFF9C.
// 0x801DBA84
void FrameFxVortex(f32 a0, f32 a1, f32 a2, f32 a3, f32 a4, s32 a5) {
    f32 v[5] = { a0, a1, lbl_805FFF9C * a2, a3, a4 };
    (void)a5;
    fn_8016195C(lbl_8040E5F0, 0xD, 0, v);
}

// Tunnel effect (id 0xE), six float parameters.
// 0x801DBAD8
void FrameFxTunnel(f32 a0, f32 a1, f32 a2, f32 a3, f32 a4, f32 a5, s32 a6) {
    f32 v[6] = { a0, a1, a2, a3, a4, a5 };
    (void)a6;
    fn_8016195C(lbl_8040E5F0, 0xE, 0, v);
}

// Starts the challenge frame effect (id 2) unless it is already running; the flag is set.
// 0x801DBB28
void StartChallengeFrameFx(void) {
    if (fn_80162B6C(lbl_8040E5F0, 2)) {
        fn_801629D0(lbl_8040E5F0, 2);
        lbl_805FB860 = 1;
    }
}

// Stops the challenge frame effect (id 2) if the flag says it was started.
// 0x801DBB78
void StopChallengeFrameFx(void) {
    if (lbl_805FB860) {
        fn_8016195C(lbl_8040E5F0, 2, 0, NULL);
        lbl_805FB860 = 0;
    }
}

// Stops the NIS frame effects 5, 0x10 and 0x12.
// 0x801DBC4C
void StopNISFrameFx(void) {
    fn_801629D0(lbl_8040E5F0, 5);
    fn_801629D0(lbl_8040E5F0, 0x10);
    fn_801629D0(lbl_8040E5F0, 0x12);
}

// Stops the NIS TV-static frame effects 0x10 and 0x12.
// 0x801DBD74
void StopNISTVStaticFrameFx(void) {
    fn_801629D0(lbl_8040E5F0, 0x10);
    fn_801629D0(lbl_8040E5F0, 0x12);
}

// Random integer in [lo, hi] (inclusive): the RNG value modulo the range, offset by lo.
// 0x801DBDD0
s32 IntervalRandomInt(s32 lo, s32 hi) {
    u32 r = RandomU32();
    u32 range = (u32)hi - (u32)lo + 1;
    return (s32)(r % range) + lo;
}

// Random float in [lo, hi): the RNG value scaled to [0, 1) by lbl_805FFFA0 (the original
// converts the u32 through the 0x43300000 double idiom, which is exact).
// 0x801DBE20
f32 IntervalRandomFloat(f32 lo, f32 hi) {
    u32 r = RandomU32();
    f32 frac = lbl_805FFFA0 * (f32)r;
    return lo + (hi - lo) * frac;
}

// Sets the sleep bit (0x200) in the state word at +0x28 of the FSM sub-object at +0xA4.
// Does nothing if the entity, its sub-object or the FSM pointer is missing.
// 0x801DBE94
void Entity_SleepFSM(void* a0) {
    u8* obj = (u8*)a0;
    if (obj == NULL) return;
    u8* sub = *(u8**)(obj + 0xA4);
    if (sub == NULL) return;
    u8* fsm = *(u8**)sub;
    if (fsm == NULL) return;
    *(u32*)(fsm + 0x28) |= 0x200;
}

// Spawns effect a0 at the position given by a1 (fn_801ED788 fills x, y, z), with a
// fixed fourth component.
// 0x801DBED4
void SpawnParticleAt(u32 a0, u32 a1) {
    f32 p[4];
    fn_801ED788(p, (u8*)(uintptr_t)a1);
    p[3] = lbl_805FFF78;
    SpawnEffectAt(lbl_8047C760, a0, p, 0);
}

// Plays sound a0 through the global sound path. If the event object from EventPost(0x68)
// exists, the sound id is stored in its +4 slot.
// 0x801DC1D0
void PlayGlobalSoundSynced(u32 a0) {
    u8* ev = (u8*)EventPost(0x68, NULL);
    if (ev != NULL) *(u32*)(ev + 4) = a0;
    PlaySoundAt((void*)(intptr_t)a0, 0, NULL, 0);
}

// Starts sound a0 as the looping group sound. The previous handle in lbl_805F9980 (if not
// -1) is released with fn_8020A398 first, and the new handle is stored.
// 0x801DC23C
void PlayGlobalLoopingGroupSound_NoLoad(u32 a0) {
    if (lbl_805F9980 != -1) fn_8020A398(lbl_805F9980);
    lbl_805F9980 = PlaySound(a0, 0, 0, 0, 0, 0);
}

// Creates the voice SFX handle lbl_805FBBAC once (fn_801D40E0, then fn_80209814 if that
// returned something). Shared by the global volume functions.
static void EnsureVoiceSfxHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// Sets the global music volume.
// 0x801DC2C8
void SetGlobalMusicVolume(f32 a0) {
    EnsureVoiceSfxHandle();
    fn_8020B024(a0);
}

// Returns the global music volume.
// 0x801DC320
f32 GetGlobalMusicVolume(void) {
    EnsureVoiceSfxHandle();
    return fn_8020B05C();
}

// Returns the global SFX volume.
// 0x801DC364
f32 GetGlobalSfxVolume(void) {
    EnsureVoiceSfxHandle();
    return fn_8020B154();
}

// Looks up the script table entry whose hash equals the key and returns its index,
// or -1 if none matches. The count is lbl_805FB880.
// 0x801DC9B0
extern "C" s32 fn_801DC9B0(void* p) {
    u32 key = (u32)(uintptr_t)p;
    u32 n = (u32)lbl_805FB880;
    for (u32 i = 0; i < n; i++) {
        if (gScriptFuncs[i].hash == key) return (s32)i;
    }
    return -1;
}

// Runs the activate handlers of obj: the one at obj+0x9C (sub-object +0x80) and the one at
// obj+0x50. A handler runs when its 4-bit trigger field (bits 1-4, IBM numbering) is nonzero.
// 0x801DDC08
void CallActivateHandler(void* a0) {
    u8* obj = (u8*)a0;
    if (obj == NULL) return;
    u8* sub = *(u8**)(obj + 0x9C);
    if (sub != NULL) {
        u32 w = *(u32*)(sub + 0x80);
        if (((w >> 27) & 0xF) != 0) TriggerDoAction(sub + 0x80, obj, NULL);
    }
    u32 w2 = *(u32*)(obj + 0x50);
    if (((w2 >> 27) & 0xF) != 0) TriggerDoAction(obj + 0x50, obj, NULL);
}

// Runs the deactivate handlers of obj: the one at obj+0x9C (sub-object +0x84) and the one at
// obj+0x54, with the same trigger test as CallActivateHandler.
// 0x801DDC78
void CallDeactivateHandler(void* a0) {
    u8* obj = (u8*)a0;
    if (obj == NULL) return;
    u8* sub = *(u8**)(obj + 0x9C);
    if (sub != NULL) {
        u32 w = *(u32*)(sub + 0x84);
        if (((w >> 27) & 0xF) != 0) TriggerDoAction(sub + 0x84, obj, NULL);
    }
    u32 w2 = *(u32*)(obj + 0x54);
    if (((w2 >> 27) & 0xF) != 0) TriggerDoAction(obj + 0x54, obj, NULL);
}

// Cosine-eased blend of a0 and a1 by a2: w = k * (c + cos(phase)), result = a0 * w + a1 * (c - w),
// with phase = lbl_805FFFB0 * lbl_805FFFB4 * a2.
// 0x801DDCE8
f32 CosInterp(f32 a0, f32 a1, f32 a2) {
    f32 phase = lbl_805FFFB0 * (lbl_805FFFB4 * a2);
    f32 w = lbl_805FFFB8 * (lbl_805FFFBC + cosf(phase));
    return a1 * (lbl_805FFFBC - w) + a0 * w;
}
