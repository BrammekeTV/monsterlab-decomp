// Decompiled functions from the miss56 batch (0x8002xxxx - 0x8018xxxx): script registrars,
// welding/experiment helpers, menu and effect objects.
#include "types.h"
#include <math.h>

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);   // 0x801DC91C
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);     // 0x80209E70
s32   RaySceneTest(void* ray);                                      // 0x80070458 (ignores its argument)
u32   RandomU32(void);
void  fn_8027D28C(s32 chan, f32 a, f32 b);                          // 0x8027D28C
void  fn_801458B0(f32* quat, f32* a, f32* b);                       // 0x801458B0 (look-at quaternion)
s32   AnimSetFindClip(void* set, s32 anim);                         // 0x801AC2FC
void  PropPlayAnim(void* prop, int a, int b, BOOL once);            // 0x80069350
void  fn_801C4B90(u8* obj, f32* hi, f32* lo, s32 a, s32 b, u32 color);  // 0x801C4B90
void  fn_801C5CC0(u8* obj, f32* hi, f32* lo);                       // 0x801C5CC0
void  fn_801C3E7C(u8* tbl, void* handle);                           // 0x801C3E7C
s32   fn_801C3E88(u8* tbl, void* handle);                           // 0x801C3E88
void  fn_800B28E0(u8* p);                                           // 0x800B28E0
void  fn_800B33CC(u8* p);                                           // 0x800B33CC
void  fn_801FA0A0(u8* p, f32 v);                                    // 0x801FA0A0
void  fn_8020B1C8(void* handle, u32 hash, s32 a);                   // 0x8020B1C8
u8*   fn_801B3308(void);                                            // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                                    // 0x801B3310
int   SysSprintf(char* buf, const char* fmt, ...);
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);   // 0x801BA2FC
void  fn_800B8180(u8* p);                                           // 0x800B8180
void  fn_800B7B50(u8* p);                                           // 0x800B7B50
void  fn_800B60A0(u8* p);                                           // 0x800B60A0
void  fn_800B9008(u8* p);                                           // 0x800B9008
void  fn_801B3378(u8* p, u8* key);                                  // 0x801B3378
void  fn_8020B1D4(void* handle, s32 a);                             // 0x8020B1D4
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void fn_801B6F34(u8* p);                                      // 0x801B6F34
void* MemAllocDefault(u32 size);                                    // 0x801D4650
void  fn_800C1DC0(u8* p);                                           // 0x800C1DC0
void  fn_800C1F18(u8* p);                                           // 0x800C1F18
void  fn_800C6D98(u8* slot, u8* owner, u8* e);                      // 0x800C6D98
void  fn_800C6CE4(u8* slot, u8* owner, u8* e);                      // 0x800C6CE4
void  fn_800C6E48(u8* slot, u8* owner, u8* e);                      // 0x800C6E48
void  fn_800C5FC8(u8* e);                                           // 0x800C5FC8
void  fn_800712B8(void);                                            // 0x800712B8
u8*   fn_800C407C(u8* p, s32 a, void* params, s32 b);               // 0x800C407C (base ctor)
void* memset(void* p, int c, size_t n);
s32   fn_800CD4E8(u8* p);                                            // 0x800CD4E8
s32   fn_800CD744(u8* p);                                           // 0x800CD744
s32   fn_801EC880(u8* p, u32 hash);                                 // 0x801EC880
u32   StringHash(const char* s);                                    // 0x801EB704
f32   fn_800C8118(u8* a, u8* b, u8* flag);                          // 0x800C8118
u8*   fn_800C73EC(u8* h);                                           // 0x800C73EC
BOOL  fn_801B50B0(void* unused, u8* obj);                           // 0x801B50B0
f64   fn_8033F1F4(f64 x);                                           // 0x8033F1F4 (arc-cosine-like, f64 in/out)
void  fn_8017493C(void* p);                                         // 0x8017493C
void fn_8016A2B8(u8* p);                                            // 0x8016A2B8
void  fn_8018A5F4(void* p);                                         // 0x8018A5F4
void  EntitySyncTransform(void* entity, int a);                     // 0x801B4F4C
void  fn_801EC080(u8* a, u8* b);                                    // 0x801EC080
void  fn_8018BEA4(u8* p, f32 t);                                    // 0x8018BEA4
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);               // 0x800690D4 (base ctor)
void* fn_801D40E0(s32 n);                                           // 0x801D40E0
void* fn_80209814(void* v);                                         // 0x80209814
u8*   fn_80220168(u8* a3);                                          // 0x80220168
u8*   fn_8021F350(u8* p, s32 idx);                                  // 0x8021F350
u8*   fn_8006FBDC(void);                                            // 0x8006FBDC
s32   fn_8006FBFC(u8* obj);                                         // 0x8006FBFC
void  fn_800775A4(u8* a, u8* entry);                                // 0x800775A4
u8*   fn_80075964(u8* a, s32 b);                                    // 0x80075964
void  fn_80076B5C(u8* p, void* a, f32 f);                           // 0x80076B5C
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);    // 0x80145EE8
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);                 // 0x80145F9C (quaternion slerp)
s32   fn_8007AE90(u8* pair, u8* flag, u8* state, u8* b);              // 0x8007AE90
void  fn_8007C8CC(f32* out, u8* obj, f32* pt);                      // 0x8007C8CC
void  fn_8007C958(u8* obj, f32* pt);                                // 0x8007C958

s32   DiggingRockAbsent(void);                                      // 0x800232A4
s32   DiggingRockPresent(void);                                     // 0x800232D0
s32   DiggingRockIntact(void);
s32   DiggingRockCrushed(void);
s32   DiggingCheckChallengeFinished(void);
s32   ShovelDirection(void);
void  DiggingDecrementScoop(void);
void  DiggingDecrementRock(void);
void  DiggingClearRockGraphic(void);
void  DiggingPlayThrustAnim(void);
void  DiggingPlayLiftAnim(void);
void  DiggingPlayHitAnim(void);
void  DiggingPlayClangAnim(void);
void  DiggingPlayDirtAnim(void);
u8*   DiggingOutputTest(void);
void  PrintIdle(void);
void  SetDirtPile(void);
void  ResetDirtPile(void);
void  DiggingSetDustCloud(void);
void  DiggingPlayDirtFallingAnim(void);
u32   MachineIsOn(void);
s32   IsFull(void);
s32   IsNewlyFull(void);
s32   IsSpinning(void);
s32   NeedWheelSound(void);
s32   StoppedSpinning(void);
void  HighlightSelected(void);
void  FieldRepairParticleOn(void);
void  FieldRepairParticleUpdate(void);
void  FieldRepairParticleOff(void);
void  PlayHealthRecoverySound(void);
s32 WeldingOverPlate(u8* arg0);
void  WeldingGunOffUpdate(ScriptCtx* a0);
void  WeldingGunOnUpdate(ScriptCtx* a0);
s32   WeldingOverTrench(ScriptCtx* a0);
s32   WeldingOverDangerZone(void);
void  WeldingResetShakeCount(u8* a3);
void  WeldingUpdateShake(u8* a3);
void  WeldingResetOverheatCounter(u8* a3);
void  StartWeldLightingEffect(u8* a3);
void  StartBrokenGunLightingEffect(u8* a3);
void  StopWeldLightingEffect(u8* a3);
void  WeldingGunHeatDecrease(u8* a3);
}

extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern f32   lbl_80347250[];        // per-difficulty float table
extern f32   lbl_80347498[];        // float table (fill rate)
extern f32   lbl_803474A8[];        // float table (drain rate)
extern s32   lbl_80346A78[];         // 3 weights per difficulty
extern s32   lbl_80346A84[];
extern s32   lbl_80346A90[];
extern s32   lbl_80346A9C[];
extern u8    lbl_8035D6D0[];        // 0x8035D6D0 (vtable)
extern u8    lbl_8035D3C0[];        // 0x8035D3C0 (vtable)
extern f32   lbl_80346AB8[];         // float table
extern f32   lbl_80346B8C[];        // float table
extern f32   lbl_805FB168;          // float
extern u8    lbl_8047C7AC[];        // 0x8047C7AC (handle table)
extern u8*   lbl_805FB170;           // 0x805FB170 (tentacle in berserk state)
extern f32   lbl_80346BE0[];
extern f32   lbl_80346BF0[];
extern f32   lbl_80346C20[];
extern f32   lbl_80346C10[];
extern f32   lbl_80346C00[];
extern f32   lbl_80346C30[];
extern f32   lbl_80346C40[];
extern f32   lbl_80346C50[];
extern f32   lbl_80346268[];         // float table
extern "C++" void* gEventMgr;     // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;    // 0x805FB94C
extern f32   lbl_803473D0[];
extern f32   lbl_803473C0[];
extern u8    lbl_8035F258[];        // 0x8035F258 (vtable)
extern u8*   lbl_805FB608;           // 0x805FB608 (heap-like object, free function at +4)
extern u8    gWorldState[];         // 0x803E6D00 (+0xC = frame delta)

extern "C" {

// 0x80024BDC
// Registers the script callbacks of this minigame (type 0x1D = query, 0x4E = action).
s32 fn_80024BDC(void) {
    RegisterScriptFunc("DiggingRockAbsent", 0x1d, (void*)DiggingRockAbsent);
    RegisterScriptFunc("DiggingRockPresent", 0x1d, (void*)DiggingRockPresent);
    RegisterScriptFunc("DiggingRockIntact", 0x1d, (void*)DiggingRockIntact);
    RegisterScriptFunc("DiggingRockCrushed", 0x1d, (void*)DiggingRockCrushed);
    RegisterScriptFunc("DiggingCheckChallengeFinished", 0x1d, (void*)DiggingCheckChallengeFinished);
    RegisterScriptFunc("ShovelDirection", 0x1d, (void*)ShovelDirection);
    RegisterScriptFunc("DiggingDecrementScoop", 0x4e, (void*)DiggingDecrementScoop);
    RegisterScriptFunc("DiggingDecrementRock", 0x4e, (void*)DiggingDecrementRock);
    RegisterScriptFunc("DiggingClearRockGraphic", 0x4e, (void*)DiggingClearRockGraphic);
    RegisterScriptFunc("DiggingPlayThrustAnim", 0x4e, (void*)DiggingPlayThrustAnim);
    RegisterScriptFunc("DiggingPlayLiftAnim", 0x4e, (void*)DiggingPlayLiftAnim);
    RegisterScriptFunc("DiggingPlayHitAnim", 0x4e, (void*)DiggingPlayHitAnim);
    RegisterScriptFunc("DiggingPlayClangAnim", 0x4e, (void*)DiggingPlayClangAnim);
    RegisterScriptFunc("DiggingPlayDirtAnim", 0x4e, (void*)DiggingPlayDirtAnim);
    RegisterScriptFunc("DiggingOutputTest", 0x4e, (void*)DiggingOutputTest);
    RegisterScriptFunc("PrintIdle", 0x4e, (void*)PrintIdle);
    RegisterScriptFunc("SetDirtPile", 0x4e, (void*)SetDirtPile);
    RegisterScriptFunc("ResetDirtPile", 0x4e, (void*)ResetDirtPile);
    RegisterScriptFunc("DiggingSetDustCloud", 0x4e, (void*)DiggingSetDustCloud);
    return RegisterScriptFunc("DiggingPlayDirtFallingAnim", 0x4e, (void*)DiggingPlayDirtFallingAnim);
}

// 0x8002D400
// Registers the script callbacks of this minigame (type 0x1D = query, 0x4E = action).
s32 fn_8002D400(void) {
    RegisterScriptFunc("MachineIsOn", 0x1d, (void*)MachineIsOn);
    RegisterScriptFunc("IsFull", 0x1d, (void*)IsFull);
    RegisterScriptFunc("IsNewlyFull", 0x1d, (void*)IsNewlyFull);
    RegisterScriptFunc("IsSpinning", 0x1d, (void*)IsSpinning);
    RegisterScriptFunc("NeedWheelSound", 0x1d, (void*)NeedWheelSound);
    RegisterScriptFunc("StoppedSpinning", 0x1d, (void*)StoppedSpinning);
    RegisterScriptFunc("HighlightSelected", 0x4e, (void*)HighlightSelected);
    RegisterScriptFunc("FieldRepairParticleOn", 0x4e, (void*)FieldRepairParticleOn);
    RegisterScriptFunc("FieldRepairParticleUpdate", 0x4e, (void*)FieldRepairParticleUpdate);
    RegisterScriptFunc("FieldRepairParticleOff", 0x4e, (void*)FieldRepairParticleOff);
    return RegisterScriptFunc("PlayHealthRecoverySound", 0x4e, (void*)PlayHealthRecoverySound);
}

// 0x8007919C
// Registers the script callbacks of this minigame (type 0x1D = query, 0x4E = action).
s32 fn_8007919C(void) {
    RegisterScriptFunc("WeldingGunOffUpdate", 0x4e, (void*)WeldingGunOffUpdate);
    RegisterScriptFunc("WeldingGunOnUpdate", 0x4e, (void*)WeldingGunOnUpdate);
    RegisterScriptFunc("WeldingOverPlate", 0x1d, (void*)WeldingOverPlate);
    RegisterScriptFunc("WeldingOverTrench", 0x1d, (void*)WeldingOverTrench);
    RegisterScriptFunc("WeldingOverDangerZone", 0x1d, (void*)WeldingOverDangerZone);
    RegisterScriptFunc("WeldingResetShakeCount", 0x4e, (void*)WeldingResetShakeCount);
    RegisterScriptFunc("WeldingUpdateShake", 0x4e, (void*)WeldingUpdateShake);
    RegisterScriptFunc("WeldingResetOverheatCounter", 0x4e, (void*)WeldingResetOverheatCounter);
    RegisterScriptFunc("StartWeldLightingEffect", 0x4e, (void*)StartWeldLightingEffect);
    RegisterScriptFunc("StartBrokenGunLightingEffect", 0x4e, (void*)StartBrokenGunLightingEffect);
    RegisterScriptFunc("StopWeldLightingEffect", 0x4e, (void*)StopWeldLightingEffect);
    return RegisterScriptFunc("WeldingGunHeatDecrease", 0x4e, (void*)WeldingGunHeatDecrease);
}

// 0x800776C8
// Walks the type-9 entries of two entry lists (of the object at +0x14 and the one at +0x24);
// the first list calls fn_800775A4, the second plays a sound per entry (create the voice SFX
// handle first if needed): hash 0xF7544DF9 when entry byte 1 == 2, else 0x3F2E4C43.
void fn_800776C8(u8* arg0) {
    u8* list = fn_80220168(*(u8**)(arg0 + 0x14));
    for (u32 i = 0; i < *(u32*)(list + 0x1000); i++) {
        u8* e = fn_8021F350(list, (s32)i);
        if (((*e >> 4) & 0xF) == 9) {
            fn_800775A4(arg0, e);
        }
    }
    for (u32 i = 0; i < *(u32*)(arg0 + 0x1024); i++) {
        u8* e = fn_8021F350(arg0 + 0x24, (s32)i);
        if (((*e >> 4) & 0xF) == 9) {
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(e[1] == 2 ? 0xF7544DF9u : 0x3F2E4C43u, 0, 0, 0, 0, 0);
        }
    }
}

// 0x80077820
// For each of 4 slots whose list (fn_8022014C(*(arg0+0x18), slot) = base + slot*0x1004 + 0x5C)
// is non-empty, calls fn_80076B5C(fn_80075964(obj, idx), sub + 0x20C, rate) once per entry with a rate taken from
// lbl_80347250[RaySceneTest(slot object)] / arg0->0xC.
void fn_80077820(u8* arg0) {
    u8* obj = fn_8006FBDC();
    if (obj != NULL && fn_8006FBFC(obj) == 1) {
        s32 off = 0;
        for (s32 slot = 0; (u32)slot < 4u; slot++, off += 0xC) {
            u8* list = *(u8**)(arg0 + 0x18) + slot * 0x1004 + 0x5C;
            if (*(s32*)(list + 0x1000) == 0) continue;
            u8* sub = obj + off;
            s32 idx = 4;
            switch (slot) {
                case 0: idx = 0; break;
                case 1: idx = 3; break;
                case 2: idx = 1; break;
                case 3: idx = 2; break;
            }
            f32 rate = lbl_80347250[RaySceneTest(sub)] / *(f32*)(arg0 + 0xC);
            for (u32 j = 0; j < *(u32*)(list + 0x1000); j++) {
                u8* r = fn_80075964(obj, idx);
                fn_80076B5C(r, sub + 0x20C, rate);
            }
        }
    }
}

// 0x80079004
// Recomputes the cached point arrays of the object: arg0+0xE0 / +0xEC hold fn_8007C8CC results
// for the source point and each of the n (at +0x8C) child objects (pointers at +0x90, stride 4),
// the last child's result goes to a local that is then pushed to the +0x210 object; then each
// distinct child (first occurrence only) is updated with fn_8007C958.
void fn_80079004(u8* arg0) {
    f32 pt[3], r0[3], rl[3], tmp[3], last[3];
    u8* src = **(u8***)(arg0 + 0x1D8);
    pt[0] = *(f32*)(src + 0xC);
    pt[1] = *(f32*)(src + 0x10);
    pt[2] = *(f32*)(src + 0x14);
    fn_8007C8CC(r0, arg0 + 0x1D0, pt);
    *(f32*)(arg0 + 0xE0) = r0[0];
    *(f32*)(arg0 + 0xE4) = r0[1];
    *(f32*)(arg0 + 0xE8) = r0[2];
    fn_8007C958(arg0 + 0x1D0, pt);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x8C); i++) {
        u8* child = *(u8**)(arg0 + 0x90 + i * 4);
        f32* in = (f32*)(arg0 + 0xE0 + i * 0xC);
        if (i + 1 >= *(s32*)(arg0 + 0x8C)) {
            fn_8007C8CC(rl, child, in);
            last[0] = rl[0];
            last[1] = rl[1];
            last[2] = rl[2];
        } else {
            fn_8007C8CC(tmp, child, in);
            *(f32*)(arg0 + 0xEC + i * 0xC) = tmp[0];
            *(f32*)(arg0 + 0xF0 + i * 0xC) = tmp[1];
            *(f32*)(arg0 + 0xF4 + i * 0xC) = tmp[2];
        }
    }
    fn_8007C958(arg0 + 0x210, last);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x8C); i++) {
        s32 j = 0;
        for (; j < i; j++) {
            if (*(u8**)(arg0 + 0x90 + i * 4) == *(u8**)(arg0 + 0x90 + j * 4)) break;
        }
        if (j == i) {
            fn_8007C958(*(u8**)(arg0 + 0x90 + i * 4), (f32*)(arg0 + 0xE0 + i * 0xC));
        }
    }
}

// 0x8007B7BC
// Builds the orientation quaternion *arg0 from a normalised 2D input: x in [-3.2, 3.2] and
// y in [-2.4, 2.4] are clamped to 0..1, each selects a slerp between two fixed rotations
// (about Z for x, about X for y), and the two results are multiplied.
void fn_8007B7BC(f32* arg0, u8* arg1, f32 farg0, f32 farg1) {
    f32 tx, ty;
    if (farg0 <= -3.2f) {
        tx = 0.0f;
    } else if (farg0 >= 3.2f) {
        tx = 1.0f;
    } else {
        tx = (farg0 - -3.2f) / 6.4f;
    }
    if (farg1 <= -2.4f) {
        ty = 0.0f;
    } else if (farg1 >= 2.4f) {
        ty = 1.0f;
    } else {
        ty = (farg1 - -2.4f) / 4.8f;
    }
    Vec3 axisZ = {0.0f, 0.0f, 1.0f};
    Vec3 axisX = {1.0f, 0.0f, 0.0f};
    Quat q70, q60, q50;
    f32 qa[4], qb[4];
    QuatFromAxisAngle((Quat*)arg0, &axisZ, 2.1111112f);
    QuatFromAxisAngle(&q70, &axisZ, 1.3333334f);
    QuatFromAxisAngle(&q60, &axisX, 0.44444445f);
    QuatFromAxisAngle(&q50, &axisX, -0.11111111f);
    fn_80145F9C(qa, (f32*)&q70, arg0, tx);
    arg0[0] = qa[0];
    arg0[1] = qa[1];
    arg0[2] = qa[2];
    arg0[3] = qa[3];
    fn_80145F9C(qb, (f32*)&q50, (f32*)&q60, ty);
    f32 a = qb[0], b = qb[1], c = qb[2], d = qb[3];
    f32 w = arg0[3], x = arg0[0], y = arg0[1], z = arg0[2];
    f32 nx = ((y * c) + ((w * a) + (x * d))) - (z * b);
    f32 ny = (z * a) + ((y * d) + ((w * b) - (x * c)));
    f32 nw = (((w * d) - (x * a)) - (y * b)) - (z * c);
    f32 nz = (z * d) + (((w * c) + (x * b)) - (y * a));
    arg0[0] = nx;
    arg0[1] = ny;
    arg0[3] = nw;
    arg0[2] = nz;
}

// 0x8007BEE4
// Tests every distinct object in list (n entries) with fn_8007AE90 (pair = {object, arg0+0x264}),
// sets the state flag at +0x290 to 1 if any test succeeded (0 otherwise), then moves the gauge
// at +0x298 up (state 0, clamped to 1.0) or down (state 1, clamped to 0.0) by delta * table[RaySceneTest].
s32 fn_8007BEE4(u8* arg0, u8** arg1, s32 arg2, u8* arg3) {
    s32 done[14];
    u8 flag = 0;
    s32 cnt = 0;
    *(s32*)(arg0 + 0x290) = 1;
    for (s32 i = 0; i < arg2; i++) {
        s32 j = 0;
        for (; j < cnt; j++) {
            if (arg1[done[j]] == arg1[i]) break;
        }
        if (j == cnt) {
            u8* pair[2];   // fn_8007A680(pair, arg1[i], arg0 + 0x264) stores these two words
            pair[0] = arg1[i];
            pair[1] = arg0 + 0x264;
            if (fn_8007AE90((u8*)pair, &flag, arg0 + 0x290, arg3) != 0) {
                done[cnt++] = i;
            }
        }
    }
    if (flag == 0) {
        *(s32*)(arg0 + 0x290) = 0;
    }
    s32 idx;
    if (*(s32*)(arg0 + 0x290) == 0) {
        f32 dt = *(f32*)(gWorldState + 0xC);
        idx = RaySceneTest(gWorldState);
        f32 v = *(f32*)(arg0 + 0x298) + dt * lbl_80347498[idx];
        *(f32*)(arg0 + 0x298) = v;
        if (v > 1.0f) *(f32*)(arg0 + 0x298) = 1.0f;
    } else {
        f32 dt = *(f32*)(gWorldState + 0xC);
        idx = RaySceneTest(gWorldState);
        f32 v = *(f32*)(arg0 + 0x298) - dt * lbl_803474A8[idx];
        *(f32*)(arg0 + 0x298) = v;
        if (v < 0.0f) *(f32*)(arg0 + 0x298) = 0.0f;
    }
    return idx;
}

typedef u8* (*GetObjFn)(u8* self);

// Sets or clears bit 1 of the flags at +8 of the object returned by vtable slot 0xC (if any),
// and of the flags at +0x20 of the object at +4.
static void SetObjHidden(u8* o, BOOL set) {
    if ((*(GetObjFn*)(*(u8**)o + 0xC))(o) != NULL) {
        u8* v = (*(GetObjFn*)(*(u8**)o + 0xC))(o);
        if (set) *(u32*)(v + 0x8) |= 2;
        else *(u32*)(v + 0x8) &= 0xFFFFFFFD;
    }
    u8* t = *(u8**)(o + 4);
    if (set) *(u32*)(t + 0x20) |= 2;
    else *(u32*)(t + 0x20) &= 0xFFFFFFFD;
}

// 0x8007CB68
// Swaps which of the two linked objects (at +0 optional, at +4) is hidden: mode 0 hides the
// first and shows the second, mode 1 the opposite, mode 2 shows both; other modes do nothing.
void fn_8007CB68(u8* arg0, s32 arg1) {
    if (arg1 == 1) {
        u8* a = *(u8**)arg0;
        if (a != NULL) SetObjHidden(a, FALSE);
        SetObjHidden(*(u8**)(arg0 + 4), TRUE);
    } else if (arg1 >= 0 && arg1 < 1) {
        u8* a = *(u8**)arg0;
        if (a != NULL) SetObjHidden(a, TRUE);
        SetObjHidden(*(u8**)(arg0 + 4), FALSE);
    } else if (arg1 == 2) {
        u8* a = *(u8**)arg0;
        if (a != NULL) SetObjHidden(a, FALSE);
        SetObjHidden(*(u8**)(arg0 + 4), FALSE);
    }
}

// 0x800A5164
// Weighted random pick of 0, 1 or 2 using three weights chosen by RaySceneTest() (a 0..3
// selector, argument ignored): roll in [0, w0+w1+w2); below w0 -> 0, else 1 or 2 by w1.
s32 fn_800A5164(void) {
    s32 w0 = 0, w1 = 0, w2 = 0;
    const s32* t = NULL;
    switch (RaySceneTest(NULL)) {
        case 0: t = lbl_80346A78; break;
        case 1: t = lbl_80346A84; break;
        case 2: t = lbl_80346A90; break;
        case 3: t = lbl_80346A9C; break;
    }
    if (t != NULL) {
        w0 = t[0];
        w1 = t[1];
        w2 = t[2];
    }
    s32 roll = (s32)(RandomU32() % (u32)(w2 + (w0 + w1)));
    if (roll < w0) {
        return 0;
    }
    s32 r4 = roll - w0;
    u32 ca = (u32)r4 >= (u32)w1;
    return (s32)(((u32)w1 >> 31) - ((u32)r4 >> 31) - 1 + ca) + 2;
}

// 0x800A610C
// Constructor: base ctor, then runs twice with two successive vtables (lbl_8035D6D0 then
// lbl_8035D3C0): sets flags |= 0x600000 at +0x15C, hides the vtable-slot-0xC object and clears
// bit 1 (first pass also bit 0) of the flags at +0x20 of the object at +4.
void* fn_800A610C(u8* arg0, s32 arg1, void* arg2, s32 arg3) {
    fn_800690D4(arg0, arg1, arg2, arg3);
    *(u8*)(arg0 + 0x264) = 0;
    *(void**)arg0 = lbl_8035D6D0;
    *(u8*)(arg0 + 0x265) = 0;
    *(u32*)(arg0 + 0x15C) |= 0x600000;
    if ((*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* v = (*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(v + 0x8) &= 0xFFFFFFFD;
    }
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) &= ~3u;
    *(void**)arg0 = lbl_8035D3C0;
    *(u32*)(arg0 + 0x15C) |= 0x600000;
    if ((*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* v = (*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(v + 0x8) &= 0xFFFFFFFD;
    }
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) &= 0xFFFFFFFD;
    return arg0;
}

typedef void (*VCallSelf)(u8* self);
typedef void (*VCallOut)(f32* out, u8* self);

// 0x800A6704
// Sets up the camera/target object at +4 of arg0 from the entity arg1: position = entity
// position (+ (0,0,1)), orientation = look-at of the entity's vector (via vtable slots at
// *(arg1+0xD4) 0x20/0x24/0x28), marks it dirty, copies the position into +0x278 and clears
// the state fields; finally sets fn_8027D28C(0, 0.1, 0.8).
void fn_800A6704(u8* arg0, u8* arg1) {
    f32 off[3] = {0.0f, 0.0f, 1.0f};
    f32 pos[3], dir[3], quat[4], a[3], b[3];
    u8* vt = *(u8**)(arg1 + 0xD4);
    (*(VCallSelf*)(vt + 0x20))(arg1);
    (*(VCallOut*)(*(u8**)(arg1 + 0xD4) + 0x24))(pos, arg1);
    f32 px = pos[0] + off[0];
    f32 py = pos[1] + off[1];
    f32 pz = pos[2] + off[2];
    u8* p = *(u8**)(arg0 + 4);
    *(f32*)(p + 0x0) = px;
    *(f32*)(p + 0x4) = py;
    *(f32*)(p + 0x8) = pz;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    (*(VCallOut*)(*(u8**)(arg1 + 0xD4) + 0x28))(dir, arg1);
    a[0] = dir[1];
    a[1] = -dir[0];
    a[2] = dir[2];
    b[0] = 0.0f;
    b[1] = 0.0f;
    b[2] = 1.0f;
    fn_801458B0(quat, a, b);
    p = *(u8**)(arg0 + 4);
    *(f32*)(p + 0xC) = quat[0];
    *(f32*)(p + 0x10) = quat[1];
    *(f32*)(p + 0x14) = quat[2];
    *(f32*)(p + 0x18) = quat[3];
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(f32*)(arg0 + 0x26C) = 0.0f;
    p = *(u8**)(arg0 + 4);
    *(f32*)(arg0 + 0x278) = *(f32*)(p + 0x0);
    *(f32*)(arg0 + 0x27C) = *(f32*)(p + 0x4);
    *(f32*)(arg0 + 0x280) = *(f32*)(p + 0x8);
    *(f32*)(arg0 + 0x264) = 0.0f;
    *(u8*)(arg0 + 0x274) = 0;
    fn_8027D28C(0, 0.1f, 0.8f);
}

typedef u8* (*VCallRetPtr)(u8* self);

// 0x800A7064
// Per-frame update: the timer at +0x264 falls by 100*dt (min 0) and shifts the x position of
// the object at +4 by timer*dt (old position saved to +0x278..); then syncs the transform and
// advances the gauge at +0x270 by dt * (timer / table[RaySceneTest()]).
void fn_800A7064(u8* arg0, f32 farg0) {
    *(f32*)(arg0 + 0x264) = *(f32*)(arg0 + 0x264) - 100.0f * farg0;
    if (*(f32*)(arg0 + 0x264) < 0.0f) {
        *(f32*)(arg0 + 0x264) = 0.0f;
    }
    u8* p = *(u8**)(arg0 + 4);
    f32 x = *(f32*)(p + 0x0);
    f32 y = *(f32*)(p + 0x4);
    f32 z = *(f32*)(p + 0x8);
    f32 nx = *(f32*)(arg0 + 0x264) * farg0 + x;
    *(f32*)(arg0 + 0x278) = x;
    *(f32*)(arg0 + 0x27C) = *(f32*)(p + 0x4);
    *(f32*)(arg0 + 0x280) = *(f32*)(p + 0x8);
    *(f32*)(p + 0x0) = nx;
    *(f32*)(p + 0x4) = y;
    *(f32*)(p + 0x8) = z;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    EntitySyncTransform(arg0, 0);
    fn_801EC080(arg0 + 0x94, arg0);
    lbl_805FB168 = *(f32*)(arg0 + 0x264) / lbl_80346AB8[RaySceneTest(NULL)];
    f32 dt = *(f32*)(gWorldState + 0xC);
    f32 step = dt * (*(f32*)(arg0 + 0x264) / lbl_80346AB8[RaySceneTest(NULL)]);
    *(f32*)(arg0 + 0x270) = *(f32*)(arg0 + 0x270) + step;
    u8* r = (*(VCallRetPtr*)(*(u8**)arg0 + 0x10))(arg0);
    fn_8018BEA4(r, *(f32*)(arg0 + 0x270));
}

// 0x800A76B4
// Per-frame update: picks one of three animation clips from the animation set at
// *(arg0+0x9C)+0x48 (hashes 0x3E5CA6BC idle, 0x9A89E014 when +0x268 < 0, 0xD6F786CC otherwise
// when |+0x268| > 0.1 * table[RaySceneTest()]), restarts the animation if it changed, then
// advances the gauge at +0x270 by dt * (+0x264 / table[RaySceneTest()]).
void fn_800A76B4(u8* arg0) {
    u8* info = *(u8**)(arg0 + 0x9C);
    void* set = *(void**)(info + 0x48);
    u8 clipA = (u8)AnimSetFindClip(set, (s32)0x3E5CA6BCu);
    u8 clipB = (u8)AnimSetFindClip(set, (s32)0x9A89E014u);
    u8 clipC = (u8)AnimSetFindClip(set, (s32)0xD6F786CCu);
    f32 av = *(f32*)(arg0 + 0x268);
    f32 mag = av < 0.0f ? -av : av;
    u8 clip = clipA;
    if (mag > 0.1f * lbl_80346B8C[RaySceneTest(NULL)]) {
        if (*(f32*)(arg0 + 0x268) < 0.0f) clip = clipB;
        else clip = clipC;
    }
    if (clip != *(u8*)(*(u8**)(arg0 + 4) + 0x39) && *(void**)(info + 0x48) != NULL) {
        PropPlayAnim(arg0, clip, 1, 0);
        u8* r = (*(VCallRetPtr*)(*(u8**)arg0 + 0x10))(arg0);
        *(s32*)(*(u8**)(*(u8**)r + 0x14) + 0x20) = 1;
    }
    f32 dt = *(f32*)(gWorldState + 0xC);
    f32 step = dt * (*(f32*)(arg0 + 0x264) / lbl_80346AB8[RaySceneTest(NULL)]);
    *(f32*)(arg0 + 0x270) = *(f32*)(arg0 + 0x270) + step;
    u8* r2 = (*(VCallRetPtr*)(*(u8**)arg0 + 0x10))(arg0);
    fn_8018BEA4(r2, *(f32*)(arg0 + 0x270));
}

// 0x800ABA44
// Releases the two box-shaped handles at +0x290 / +0x294: sends the box (centre +0x27C..,
// half extents +0x298..) to the handle with fn_801C4B90 / fn_801C5CC0, and frees it.
void fn_800ABA44(u8* arg0) {
    f32 hi[3], lo[3];
    if (*(u8**)(arg0 + 0x290) != NULL) {
        hi[0] = *(f32*)(arg0 + 0x27C) + *(f32*)(arg0 + 0x298);
        hi[1] = *(f32*)(arg0 + 0x280) + *(f32*)(arg0 + 0x29C);
        hi[2] = *(f32*)(arg0 + 0x284) + *(f32*)(arg0 + 0x2A0);
        lo[0] = *(f32*)(arg0 + 0x27C) - *(f32*)(arg0 + 0x298);
        lo[1] = *(f32*)(arg0 + 0x280) - *(f32*)(arg0 + 0x29C);
        lo[2] = *(f32*)(arg0 + 0x284) - *(f32*)(arg0 + 0x2A0);
        u8* h = *(u8**)(arg0 + 0x290);
        fn_801C4B90(h, hi, lo, 0, 0, *(u32*)(*(u8**)h + 4));
        u8* p = *(u8**)(arg0 + 4);
        hi[0] = *(f32*)(p + 0x0) + *(f32*)(arg0 + 0x298);
        hi[1] = *(f32*)(p + 0x4) + *(f32*)(arg0 + 0x29C);
        hi[2] = *(f32*)(p + 0x8) + *(f32*)(arg0 + 0x2A0);
        p = *(u8**)(arg0 + 4);
        lo[0] = *(f32*)(p + 0x0) - *(f32*)(arg0 + 0x298);
        lo[1] = *(f32*)(p + 0x4) - *(f32*)(arg0 + 0x29C);
        lo[2] = *(f32*)(p + 0x8) - *(f32*)(arg0 + 0x2A0);
        *(f32*)(*(u8**)(arg0 + 0x290) + 0x390) = 0.0f;
        h = *(u8**)(arg0 + 0x290);
        fn_801C4B90(h, hi, lo, 0, 0, *(u32*)(*(u8**)h + 4));
        *(f32*)(*(u8**)(arg0 + 0x290) + 0x390) = 0.0f;
        h = *(u8**)(arg0 + 0x290);
        fn_801C4B90(h, hi, lo, 0, 0, *(u32*)(*(u8**)h + 4));
        fn_801C3E7C(lbl_8047C7AC, *(void**)(arg0 + 0x290));
        *(void**)(arg0 + 0x290) = NULL;
        return;
    }
    if (*(u8**)(arg0 + 0x294) != NULL) {
        hi[0] = *(f32*)(arg0 + 0x27C) + *(f32*)(arg0 + 0x298);
        hi[1] = *(f32*)(arg0 + 0x280) + *(f32*)(arg0 + 0x29C);
        hi[2] = *(f32*)(arg0 + 0x284) + *(f32*)(arg0 + 0x2A0);
        lo[0] = *(f32*)(arg0 + 0x27C) - *(f32*)(arg0 + 0x298);
        lo[1] = *(f32*)(arg0 + 0x280) - *(f32*)(arg0 + 0x29C);
        lo[2] = *(f32*)(arg0 + 0x284) - *(f32*)(arg0 + 0x2A0);
        fn_801C5CC0(*(u8**)(arg0 + 0x294), hi, lo);
        fn_801C3E88(lbl_8047C7AC, *(void**)(arg0 + 0x294));
        *(void**)(arg0 + 0x294) = NULL;
    }
}

// 0x800AE200
// Sets the active flag (+0x364) and the gauge rate (+0x368) from a per-difficulty table
// chosen by the event hash arg1 (unresolved hashes); hash 0xC2FDDCDC clears the flag instead.
// A rate of 0 becomes 0.01; +0x36C gets a copy of the rate.
void fn_800AE200(u8* arg0, u32 arg1) {
    *(u8*)(arg0 + 0x364) = 1;
    if (arg1 == 0xC2FDDCDCu) {
        *(u8*)(arg0 + 0x364) = 0;
    } else if (arg1 == 0xB5FAEC4Au) {
        *(f32*)(arg0 + 0x368) = lbl_80346BE0[RaySceneTest(NULL)];
    } else if (arg1 == 0x2CF3BDF0u) {
        *(f32*)(arg0 + 0x368) = lbl_80346BF0[RaySceneTest(NULL)];
    } else if (arg1 == 0x39214CD8u) {
        *(f32*)(arg0 + 0x368) = lbl_80346C20[RaySceneTest(NULL)];
    } else if (arg1 == 0x5BF48D66u) {
        if (lbl_805FB170 == arg0) {
            *(f32*)(arg0 + 0x368) = lbl_80346C10[RaySceneTest(NULL)];
        } else {
            *(f32*)(arg0 + 0x368) = lbl_80346C00[RaySceneTest(NULL)];
        }
    } else if (arg1 == 0xF27D9F7Du) {
        *(f32*)(arg0 + 0x368) = lbl_80346C30[RaySceneTest(NULL)];
    } else if (arg1 == 0x6B74CEC7u) {
        *(f32*)(arg0 + 0x368) = lbl_80346C40[RaySceneTest(NULL)];
    } else if (arg1 == 0xF8963B63u) {
        *(f32*)(arg0 + 0x368) = lbl_80346C50[RaySceneTest(NULL)];
    }
    if (*(f32*)(arg0 + 0x368) <= 0.0f) {
        *(f32*)(arg0 + 0x368) = 0.01f;
    }
    *(f32*)(arg0 + 0x36C) = *(f32*)(arg0 + 0x368);
}

// 0x800B20A4
// Swept sphere test: moves arg1 (previous position +0x20.., current +0x14.., radius +0x3C)
// against every sphere (centre +0x14.., radius +0x3C, skipping those with +0xC != 0) of every
// group in the list at +0x8C of arg0 (groups: +0x28 count, +0x24 items; list: +0x10 count,
// +0xC groups). Returns 1 on a hit with t <= 1 and writes the hit sphere centre to arg2
// (if non-null); otherwise zeroes arg2 and returns 0.
s32 fn_800B20A4(u8* arg0, u8* arg1, f32* arg2) {
    f32 dx = *(f32*)(arg1 + 0x14) - *(f32*)(arg1 + 0x20);
    f32 dy = *(f32*)(arg1 + 0x18) - *(f32*)(arg1 + 0x24);
    f32 dz = *(f32*)(arg1 + 0x1C) - *(f32*)(arg1 + 0x28);
    u8* list = *(u8**)(arg0 + 0x8C);
    if (list != NULL) {
        u32 nGroups = *(u32*)(list + 0x10);
        for (u32 gi = 0; gi < nGroups; gi++) {
            u8* group = *(u8**)(*(u8**)(list + 0xC) + gi * 4);
            if (group == NULL) continue;
            u32 nItems = *(u32*)(group + 0x28);
            for (u32 k = 0; k < nItems; k++) {
                u8* item = *(u8**)(*(u8**)(group + 0x24) + k * 4);
                if (*(s32*)(item + 0xC) != 0) continue;
                f32 px = *(f32*)(item + 0x14) - *(f32*)(arg1 + 0x20);
                f32 py = *(f32*)(item + 0x18) - *(f32*)(arg1 + 0x24);
                f32 pz = *(f32*)(item + 0x1C) - *(f32*)(arg1 + 0x28);
                f32 rr = *(f32*)(item + 0x3C) + *(f32*)(arg1 + 0x3C);
                f32 c = ((pz * pz) + ((px * px) + (py * py))) - (rr * rr);
                f32 vx = 0.0f - dx;
                f32 vy = 0.0f - dy;
                f32 vz = 0.0f - dz;
                f32 t = 0.0f;
                BOOL ok;
                if (c < 0.0f) {
                    ok = 1;   // already overlapping at the start: t = 0
                } else {
                    f32 a = (vz * vz) + ((vx * vx) + (vy * vy));
                    if (a < 0.0001f) {
                        ok = 0;
                    } else {
                        f32 b = (vz * pz) + ((vx * px) + (vy * py));
                        if (b >= 0.0f) {
                            ok = 0;
                        } else {
                            f32 disc = (b * b) - (a * c);
                            if (disc < 0.0f) {
                                ok = 0;
                            } else {
                                f32 root = 0.0f;
                                if (!(disc <= 0.0f)) {
                                    root = (f32)sqrt((double)disc);  // original: frsqrte + 2 Newton steps
                                }
                                ok = 1;
                                t = (-b - root) / a;
                            }
                        }
                    }
                }
                if (ok && t <= 1.0f) {
                    if (arg2 != NULL) {
                        arg2[0] = *(f32*)(item + 0x14);
                        arg2[1] = *(f32*)(item + 0x18);
                        arg2[2] = *(f32*)(item + 0x1C);
                    }
                    return 1;
                }
            }
        }
    }
    if (arg2 != NULL) {
        arg2[0] = 0.0f;
        arg2[1] = 0.0f;
        arg2[2] = 0.0f;
    }
    return 0;
}

typedef void (*VCallThis)(u8* self);

// 0x800B2740
// Init/reset: runs vtable slot 0x54 (via the table pointer at +4) and two helpers, resets a few
// state fields, sets two gauge floats to 0.2, registers five sub-objects with the manager, clears
// seven name buffers, sets the +0x67C rate from a table by RaySceneTest() (clamped to 0..3) and
// triggers sound 0xA5237C5F on the voice SFX handle (created on first use).
void fn_800B2740(u8* arg0) {
    (*(VCallThis*)(*(u8**)(arg0 + 4) + 0x54))(arg0);
    fn_800B28E0(arg0);
    fn_800B33CC(arg0);
    *(s32*)(arg0 + 0x344) = 0;
    *(f32*)(arg0 + 0x674) = 50.0f;
    *(s32*)(arg0 + 0x3F8) = 0;
    *(s32*)(arg0 + 0x4F8) = 1;
    *(s32*)(arg0 + 0x5AC) = 1;
    fn_801FA0A0(arg0 + 0x360, 0.2f);
    fn_801FA0A0(arg0 + 0x514, 0.2f);
    fn_801B3310(fn_801B3308(), arg0 + 0x1F0);
    fn_801B3310(fn_801B3308(), arg0 + 0x2AC);
    fn_801B3310(fn_801B3308(), arg0 + 0x360);
    fn_801B3310(fn_801B3308(), arg0 + 0x460);
    fn_801B3310(fn_801B3308(), arg0 + 0x514);
    SysSprintf((char*)(arg0 + 0x684), "");
    SysSprintf((char*)(arg0 + 0x708), "");
    SysSprintf((char*)(arg0 + 0x78C), "");
    SysSprintf((char*)(arg0 + 0x810), "");
    SysSprintf((char*)(arg0 + 0x894), "");
    SysSprintf((char*)(arg0 + 0x914), "");
    SysSprintf((char*)(arg0 + 0x998), "");
    s32 d = RaySceneTest(NULL);
    if (d < 0) d = 0;
    else if (d > 3) d = 3;
    *(f32*)(arg0 + 0x67C) = 0.5f * lbl_80346268[d];
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    fn_8020B1C8(lbl_805FBBAC, 0xA5237C5Fu, 0);
}

// 0x800B3180
// Shutdown: sends six events to the event manager, releases the handles in the slot tables
// (+0x98 x2, +0xA0 x2, +0x94, +0xA8 stride 0x14 x5), clears the state fields and unregisters the
// five sub-objects, then calls fn_8020B1D4(voice SFX handle, 0) (handle created on first use).
void fn_800B3180(u8* arg0) {
    EventSendTo(gEventMgr, 0xB90A07A0u, NULL, 0x9B51AA64u, 0, 0);
    EventSendTo(gEventMgr, 0x8E7FD4E3u, NULL, 0x9B51AA64u, 0, 0);
    EventSendTo(gEventMgr, 0xB8E87466u, NULL, 0x9B51AA64u, 0, 0);
    EventSendTo(gEventMgr, 0x93226470u, NULL, 0x9B51AA64u, 0, 0);
    EventSendTo(gEventMgr, 0xB80F37B3u, NULL, 0x9B51AA64u, 0, 0);
    EventSendTo(gEventMgr, 0xA11406F2u, NULL, 0x9B51AA64u, 0, 0);
    for (s32 i = 0; i < 2; i++) {
        u8* h = *(u8**)(arg0 + 0x98 + i * 4);
        if (h != NULL) {
            fn_800B8180(h);
            *(s32*)(arg0 + 0x98 + i * 4) = 0;
        }
    }
    for (s32 i = 0; i < 2; i++) {
        u8* h = *(u8**)(arg0 + 0xA0 + i * 4);
        if (h != NULL) {
            fn_800B7B50(h);
            *(s32*)(arg0 + 0xA0 + i * 4) = 0;
        }
    }
    if (*(u8**)(arg0 + 0x94) != NULL) {
        fn_800B60A0(*(u8**)(arg0 + 0x94));
        *(s32*)(arg0 + 0x94) = 0;
    }
    for (s32 i = 0; i < 5; i++) {
        u8* h = *(u8**)(arg0 + 0xA8 + i * 0x14);
        if (h != NULL) {
            fn_800B9008(h);
        }
        *(s32*)(arg0 + 0x10C + i * 0xC) = 0;
        *(s32*)(arg0 + 0x110 + i * 0xC) = 0;
        *(s32*)(arg0 + 0x114 + i * 0xC) = 0;
    }
    *(s32*)(arg0 + 0x148) = 0;
    *(s32*)(arg0 + 0x14C) = 0;
    *(s32*)(arg0 + 0x150) = 0;
    *(s32*)(arg0 + 0x154) = 0;
    *(s32*)(arg0 + 0x158) = 0;
    *(s32*)(arg0 + 0x15C) = 0;
    *(f32*)(arg0 + 0x674) = 0.0f;
    *(f32*)(arg0 + 0x678) = 0.0f;
    *(f32*)(arg0 + 0x67C) = 0.0f;
    *(f32*)(arg0 + 0x680) = 0.0f;
    *(f32*)(arg0 + 0x704) = 0.0f;
    fn_801B3378(fn_801B3308(), arg0 + 0x1F0);
    fn_801B3378(fn_801B3308(), arg0 + 0x2AC);
    fn_801B3378(fn_801B3308(), arg0 + 0x360);
    fn_801B3378(fn_801B3308(), arg0 + 0x460);
    fn_801B3378(fn_801B3308(), arg0 + 0x514);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    fn_8020B1D4(lbl_805FBBAC, 0);
}

// 0x800BE3A8
// Launches projectile arg0 from point arg1 toward arg2: stores speed fields (+0x288 = farg1,
// +0x28C = arg3, +0x290 = 3), flattens y (both points' y set to 0), sets the velocity (+0x264) to
// the normalised direction * farg0 and the orientation to a look-at of that velocity, marks the
// object dirty, sends event arg3 to it and calls fn_801B6F34.
s32 fn_800BE3A8(u8* arg0, f32* arg1, f32* arg2, s32 arg3, f32 farg0, f32 farg1) {
    f32 x1 = arg1[0];
    f32 dx = arg2[0] - x1;
    arg1[1] = 0.0f;
    u8* obj = *(u8**)(arg0 + 4);
    *(f32*)(arg0 + 0x288) = farg1;
    f32 y1 = arg1[1];
    f32 z1 = arg1[2];
    *(s32*)(arg0 + 0x290) = 3;
    f32 dz = arg2[2] - z1;
    *(s32*)(arg0 + 0x28C) = arg3;
    *(f32*)(obj + 0x0) = x1;
    *(f32*)(obj + 0x4) = y1;
    *(f32*)(obj + 0x8) = z1;
    f32 dy = 0.0f - y1;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    arg2[1] = 0.0f;
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    f32 n[3];
    if (len2 == 0.0f) {
        n[0] = 0.0f;
        n[1] = 0.0f;
        n[2] = 0.0f;
    } else {
        f32 inv = (f32)(1.0 / sqrt((double)len2));   // original: frsqrte + 2 Newton steps
        n[0] = dx * inv;
        n[1] = dy * inv;
        n[2] = dz * inv;
    }
    f32 vx = n[0] * farg0;
    f32 vy = n[1] * farg0;
    f32 vz = n[2] * farg0;
    *(f32*)(arg0 + 0x264) = vx;
    *(f32*)(arg0 + 0x268) = vy;
    *(f32*)(arg0 + 0x26C) = vz;
    f32 quat[4], up[3] = {0.0f, 1.0f, 0.0f};
    fn_801458B0(quat, (f32*)(arg0 + 0x264), up);
    u8* p = *(u8**)(arg0 + 4);
    *(f32*)(p + 0xC) = quat[0];
    *(f32*)(p + 0x10) = quat[1];
    *(f32*)(p + 0x14) = quat[2];
    *(f32*)(p + 0x18) = quat[3];
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    EventSend(gEventMgr, arg0, NULL, (u32)arg3, 0, 0, 1, 0);
    fn_801B6F34(arg0); return 0;  // r3 is left over from the callee
}

// 0x800C2180
// Builds a table of arg2 entries (0x18 bytes each, at *(arg0)) from the source array arg1
// (8-byte {data, count} items): each entry gets a copy of the source words (count at +4,
// allocation mode 1 at +8, data at +0) and is then processed by fn_800C1DC0 / fn_800C1F18.
void fn_800C2180(u8* arg0, u8* arg1, s32 arg2) {
    *(s32*)(arg0 + 4) = arg2;
    *(s32*)(arg0 + 8) = 1;
    *(void**)arg0 = MemAllocDefault((u32)(arg2 * 0x18));
    u8* src = arg1;
    for (s32 i = 0; i < arg2; i++, src += 8) {
        u8* e = *(u8**)arg0 + i * 0x18;
        s32 cnt = *(s32*)(src + 4);
        u8* data = *(u8**)src;
        *(s32*)(e + 4) = cnt;
        *(s32*)(e + 8) = 1;
        *(void**)e = MemAllocDefault((u32)(cnt * 4));
        for (s32 j = 0; j < cnt; j++) {
            *(s32*)(*(u8**)e + j * 4) = *(s32*)(data + j * 4);
        }
        fn_800C1DC0(e);
        fn_800C1F18(e);
    }
}

typedef void (*VCallThisInt)(u8* self, s32 a);

// 0x800C2760
// Init of a small ring-buffer container bound to owner arg1: clears the counters, asks the
// owner (vtable slot 0x14 of the table at +0xC) for an object, runs its slot 0xC8 with 0 and,
// if there is one, pushes it at the front of the ring (+0x6C head, +0x70 tail, +0x74 capacity,
// +0x78 slots, +0x7C last slot used).
void fn_800C2760(u8* arg0, u8* arg1) {
    *(s32*)(arg0 + 0x8) = 0;
    *(u8**)(arg0 + 0x60) = arg1;
    *(s32*)(arg0 + 0x0) = 0;
    *(s32*)(arg0 + 0x4) = 0;
    u8* obj = (*(VCallRetPtr*)(*(u8**)(arg1 + 0xC) + 0x14))(arg1);
    (*(VCallThisInt*)(*(u8**)obj + 0xC8))(obj, 0);
    if (obj != NULL) {
        s32 cap = *(s32*)(arg0 + 0x74);
        s32 head = *(s32*)(arg0 + 0x6C);
        u8** slot;
        if (head == (*(s32*)(arg0 + 0x70) + 1) % cap) {
            slot = NULL;   // full
        } else {
            s32 nh = head - 1;
            if (nh < 0) nh = cap - 1;
            *(s32*)(arg0 + 0x6C) = nh;
            if (*(s32*)(arg0 + 0x70) < 0) {
                *(s32*)(arg0 + 0x70) = nh;
                *(s32*)(arg0 + 0x64) = 0;
            }
            s32 idx = *(s32*)(arg0 + 0x6C);
            s32 a = idx;
            s32 valid;
            s32 cap2 = *(s32*)(arg0 + 0x74);
            if (idx < 0 || !(idx < cap2)) {
                valid = 0;
            } else {
                s32 b = *(s32*)(arg0 + 0x70);
                if (b < idx) {
                    if (!(idx > b)) a = idx + cap2;
                    b += cap2;
                }
                valid = (idx <= a && a <= b) ? 1 : 0;
            }
            slot = valid ? (u8**)(*(u8**)(arg0 + 0x78) + idx * 4) : NULL;
        }
        *(u8***)(arg0 + 0x7C) = slot;
        if (slot != NULL) *slot = obj;
    }
    *(s32*)(arg0 + 0x5C) = 0;
    *(s32*)(arg0 + 0x8) = *(s32*)(arg0 + 0x8) + 1;
}

// 0x800C28C4
// Clears the same ring container: pops every slot from the tail (calling vtable slot 0xCC on
// each popped object), then calls slot 0xC0 on each of the +0x5C objects stored at +0xC and
// zeroes the +0xC..+0x5C words.
void fn_800C28C4(u8* arg0) {
    while (*(s32*)(arg0 + 0x6C) >= 0 && *(s32*)(arg0 + 0x70) >= 0) {
        u8* item = NULL;
        s32 head = *(s32*)(arg0 + 0x6C);
        s32 tail = *(s32*)(arg0 + 0x70);
        if (!(head < 0 || tail < 0)) {
            s32 a = tail;
            s32 b = tail;
            s32 valid;
            s32 cap = *(s32*)(arg0 + 0x74);
            if (tail < 0 || !(tail < cap)) {
                valid = 0;
            } else {
                if (tail < head) {
                    a = tail + cap;
                    b = tail + cap;
                }
                valid = (head <= a && a <= b) ? 1 : 0;
            }
            u8** slot = valid ? (u8**)(*(u8**)(arg0 + 0x78) + tail * 4) : NULL;
            *(u8***)(arg0 + 0x7C) = slot;
            if (slot != NULL) {
                item = *slot;
                *(s32*)slot = 0;
                s32 t = *(s32*)(arg0 + 0x70);
                if (t == *(s32*)(arg0 + 0x6C)) {
                    *(s32*)(arg0 + 0x70) = -1;
                    *(s32*)(arg0 + 0x6C) = -1;
                } else {
                    s32 nt = t - 1;
                    if (nt < 0) nt = *(s32*)(arg0 + 0x74) - 1;
                    *(s32*)(arg0 + 0x70) = nt;
                }
            }
        }
        if (item != NULL) {
            (*(VCallThis*)(*(u8**)item + 0xCC))(item);
        }
    }
    for (s32 i = 0; i < *(s32*)(arg0 + 0x5C); i++) {
        u8* o = *(u8**)(arg0 + 0xC + i * 4);
        (*(VCallThis*)(*(u8**)o + 0xC0))(o);
    }
    for (s32 off = 0xC; off <= 0x5C; off += 4) {
        *(s32*)(arg0 + off) = 0;
    }
}

typedef f32 (*VCallRetF32)(u8* self);

// 0x800C2FF0
// Walks the ring container from the tail towards the head, subtracting each object's duration
// (vtable slot 0xD0) from the total (vtable slot 0x20 of the table at +0xD4) until the remainder
// is within +-0.0001 (return that object) or no longer positive (return the current object).
// +0x7C is kept pointing at the current slot.
u8* fn_800C2FF0(u8* arg0) {
    s32 head = *(s32*)(arg0 + 0x6C);
    s32 tail = *(s32*)(arg0 + 0x70);
    u8* item = NULL;
    if (head >= 0 && tail >= 0) {
        s32 a = tail, b = tail, valid;
        s32 cap = *(s32*)(arg0 + 0x74);
        if (tail < 0 || !(tail < cap)) {
            valid = 0;
        } else {
            if (tail < head) {
                a = tail + cap;
                b = tail + cap;
            }
            valid = (head <= a && a <= b) ? 1 : 0;
        }
        u8** slot = valid ? (u8**)(*(u8**)(arg0 + 0x78) + tail * 4) : NULL;
        *(u8***)(arg0 + 0x7C) = slot;
        item = slot != NULL ? *slot : NULL;
    }
    f32 remain = (*(VCallRetF32*)(*(u8**)(arg0 + 0xD4) + 0x20))(arg0);
    for (;;) {
        if (item == NULL) return item;
        if (!(remain > (*(VCallRetF32*)(*(u8**)item + 0xD0))(item))) {
            return item;
        }
        remain = remain - (*(VCallRetF32*)(*(u8**)item + 0xD0))(item);
        if (remain > -0.0001f && remain < 0.0001f) {
            return item;
        }
        u8** cur = *(u8***)(arg0 + 0x7C);
        if (cur != NULL) {
            u8* base = *(u8**)(arg0 + 0x78);
            s32 h = *(s32*)(arg0 + 0x6C);
            s32 idx = (s32)(((u8*)cur - base) / 4);
            u8** next;
            if (idx == h) {
                next = NULL;
            } else {
                s32 cap2;
                s32 ni = idx - 1;
                if (ni < 0) ni = *(s32*)(arg0 + 0x74) - 1;
                s32 av = ni;
                s32 valid2;
                if (ni < 0 || !(ni < (cap2 = *(s32*)(arg0 + 0x74)))) {
                    valid2 = 0;
                } else {
                    s32 bv = *(s32*)(arg0 + 0x70);
                    if (bv < h) {
                        if (ni <= bv) av = ni + cap2;
                        bv += cap2;
                    }
                    valid2 = (h <= av && av <= bv) ? 1 : 0;
                }
                next = valid2 ? (u8**)(base + ni * 4) : NULL;
            }
            *(u8***)(arg0 + 0x7C) = next;
            item = next != NULL ? *next : NULL;
        } else {
            item = NULL;
        }
    }
}

// 0x800C76F4
// Registers the zone's pickup objects with arg0 by their type hash (*(*(e+0x9C)+8)): three
// kinds go into the 0x14-byte-entry tables at +0x7E4 (count at +0x1788), +0x14 (+0x1784) and
// +0xFB4 (+0x178C); the last kind also latches its position (+0x268..) and scales it by 1.0.
void fn_800C76F4(u8* arg0) {
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    for (s32 i = 0; i < *(s32*)(*(u8**)gWorldState + 4); i++) {
        for (s32 j = 0; j < *(s32*)(zone + 0x98); j++) {
            u8* e = *(u8**)(*(u8**)(zone + 0x12C) + j * 4);
            u8* info = *(u8**)(e + 0x9C);
            s32 hash = info != NULL ? *(s32*)(info + 8) : 0;
            switch (hash) {
                case 0x7679BB8A:
                    fn_800C6D98(arg0 + *(s32*)(arg0 + 0x1788) * 0x14 + 0x7E4, arg0, e);
                    *(s32*)(arg0 + 0x1788) = *(s32*)(arg0 + 0x1788) + 1;
                    break;
                case 0x7D4CAD3F:
                    *(u32*)(e + 0xF0) = 0xD9D1E8B2u;
                    fn_800C5FC8(e);
                    fn_800C6CE4(arg0 + *(s32*)(arg0 + 0x1784) * 0x14 + 0x14, arg0, e);
                    *(s32*)(arg0 + 0x1784) = *(s32*)(arg0 + 0x1784) + 1;
                    break;
                case (s32)0xA2D827D5:
                    fn_800C6E48(arg0 + *(s32*)(arg0 + 0x178C) * 0x14 + 0xFB4, arg0, e);
                    *(s32*)(arg0 + 0x178C) = *(s32*)(arg0 + 0x178C) + 1;
                    *(u32*)(e + 0x15C) |= 0x400000;
                    if (*(u8*)(e + 0x264) == 0) {
                        *(f32*)(e + 0x268) = *(f32*)(e + 0x8);
                        *(f32*)(e + 0x26C) = *(f32*)(e + 0xC);
                        *(f32*)(e + 0x270) = *(f32*)(e + 0x10);
                        *(u8*)(e + 0x264) = 1;
                    }
                    *(f32*)(e + 0x8) = *(f32*)(e + 0x268) * 1.0f;
                    *(f32*)(e + 0xC) = *(f32*)(e + 0x26C) * 1.0f;
                    *(f32*)(e + 0x10) = *(f32*)(e + 0x270) * 1.0f;
                    *(u32*)(*(u8**)(e + 4) + 0x20) |= 0x8000;
                    *(u32*)(*(u8**)(e + 4) + 0x20) |= 0x10000000;
                    *(u32*)(*(u8**)(e + 4) + 0x20) |= 0x4000;
                    *(u32*)(*(u8**)(e + 4) + 0x20) |= 0x10000000;
                    break;
            }
        }
    }
}

// 0x800C8A74
// Recomputes the fill fraction p[1] (+0x4) from the two contributions p[2] (+0x8) and p[3] (+0xC),
// each scaled by a per-difficulty table (clamped to 0..1; -1 when both are 0), triggers
// fn_800712B8 when either has reached its limit, then accumulates p[3] + p[0] + p[2] into
// p[0] clamped to 0..100.
void fn_800C8A74(f32* p) {
    if (p[2] == 0.0f && p[3] == 0.0f) {
        p[1] = -1.0f;
    } else if (p[3] == 0.0f) {
        p[1] = p[2] / lbl_803473D0[RaySceneTest(NULL)];
    } else if (p[2] == 0.0f) {
        f32 t = (0.5f * p[3]) / lbl_803473C0[RaySceneTest(NULL)];
        p[1] = t;
        p[1] = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    } else {
        f32 t0 = (0.5f * p[2]) / lbl_803473D0[RaySceneTest(NULL)];
        f32 t = t0 + ((0.5f * p[3]) / lbl_803473C0[RaySceneTest(NULL)]);
        p[1] = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    }
    if (p[2] >= 0.75f * lbl_803473D0[RaySceneTest(NULL)] || p[3] >= lbl_803473C0[RaySceneTest(NULL)]) {
        fn_800712B8();
    }
    f32 sum = p[3] + (p[0] + p[2]);
    p[0] = sum < 0.0f ? 0.0f : (sum > 100.0f ? 100.0f : sum);
}

// 0x800C8C8C
// Constructor: base ctor, then sets the vtable (lbl_8035F258), clears two fields and sets up the
// 0x4B-slot ring container at +0x468 (head/tail -1, capacity 0x4B, slots at +0x480) plus 50
// zeroed words at +0x39C.
void* fn_800C8C8C(u8* arg0, s32 arg1, void* arg2, s32 arg3) {
    fn_800C407C(arg0, arg1, arg2, arg3);
    *(s32*)(arg0 + 0x394) = 0;
    *(void**)arg0 = lbl_8035F258;
    *(s32*)(arg0 + 0x398) = 0;
    u8* slot = arg0 + 0x480;
    for (s32 i = 0; i < 0x4B; i++, slot += 4) {
        memset(slot, 0, 4);
    }
    *(u8**)(arg0 + 0x478) = arg0 + 0x480;
    *(s32*)(arg0 + 0x474) = 0x4B;
    *(s32*)(arg0 + 0x470) = -1;
    *(s32*)(arg0 + 0x46C) = -1;
    *(s32*)(arg0 + 0x468) = 0;
    for (s32 off = 0x39C; off <= 0x460; off += 4) {
        *(s32*)(arg0 + off) = 0;
    }
    return arg0;
}

// 0x800C954C
// Collects the guide ids of the staple tile: for each difficulty letter ('e','m','h','x' from
// the data bytes lbl_805FE4C0..3) up to RaySceneTest(), looks up
// "staple_tile_%02d_%c_guide_%02d" for guide 1..*(+0x94) in the table at +0x94 and stores every
// found index at +0x2A4 (count in +0x394).
void fn_800C954C(u8* arg0) {
    if (fn_800CD4E8(arg0) != 0) return;
    if (fn_800CD744(arg0) != 0) return;
    static const char kLetters[4] = {'e', 'm', 'h', 'x'};
    char buf[0x40];
    *(s32*)(arg0 + 0x394) = 0;
    for (s32 i = 0; i < RaySceneTest(NULL) + 1; i++) {
        for (s32 j = 1; j <= *(s32*)(arg0 + 0x94); j++) {
            SysSprintf(buf, "staple_tile_%02d_%c_guide_%02d", *(s32*)(arg0 + 0x398), (s8)kLetters[i], j);
            s32 idx = fn_801EC880(arg0 + 0x94, StringHash(buf));
            if (idx > -1) {
                *(s32*)(arg0 + 0x2A4 + *(s32*)(arg0 + 0x394) * 4) = idx;
                *(s32*)(arg0 + 0x394) = *(s32*)(arg0 + 0x394) + 1;
            }
        }
    }
}

// 0x800CC3E8
// If a target id is set (+0x268): measures it with fn_800C8118(id +0x264, target, &flag), stores
// the distance (+0x278) and a "far/blocked" byte (+0x27C = 0 only when dist >= 0.75 and flag 0),
// then looks the target object up, reparents it (fn_801B50B0), raises its flag bit 0, copies
// its +0xCC float into its y position and clears the target id.
void fn_800CC3E8(u8* arg0) {
    fn_8006FBDC();
    if (*(s32*)(arg0 + 0x268) != 0) {
        u8 flag = 0;
        f64 d = fn_800C8118((u8*)(intptr_t)*(s32*)(arg0 + 0x264), (u8*)(intptr_t)*(s32*)(arg0 + 0x268), &flag);
        f32 df = (f32)d;
        *(f32*)(arg0 + 0x278) = (f32)d;
        if (df >= 0.75f && flag == 0) {
            *(u8*)(arg0 + 0x27C) = 0;
        } else {
            *(u8*)(arg0 + 0x27C) = 1;
        }
        u8* obj = fn_800C73EC((u8*)(intptr_t)*(s32*)(arg0 + 0x268));
        if (obj != NULL) {
            fn_801B50B0(arg0, obj);
            *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x1;
            u8* p = *(u8**)(obj + 4);
            f32 y = *(f32*)(obj + 0xCC);
            f32 x = *(f32*)(p + 0x0);
            f32 z = *(f32*)(p + 0x8);
            *(f32*)(p + 0x0) = x;
            *(f32*)(p + 0x4) = y;
            *(f32*)(p + 0x8) = z;
            *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x8000;
            *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x10000000;
            *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x4000;
            *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x10000000;
        }
        *(s32*)(arg0 + 0x268) = 0;
    }
}

// 0x800D4588
// Finds the entry (0x1AC bytes each, from arg0+0x13284, count at arg0+0x2DCD8) whose position
// (+0x34) is nearest to the point arg1 (initial limit 100000 squared-distance) and returns its
// first word, or NULL if none is closer.
u8* fn_800D4588(u8* arg0, u8* arg1) {
    f32 best = 100000.0f;
    u8* sel = NULL;
    s32 n = *(s32*)(arg0 + 0x2DCD8);
    for (s32 i = n - 1; i > -1; i--) {
        u8* e = arg0 + 0x13284 + i * 0x1AC;   // i is always inside 0..n-1 here
        f32 dx = *(f32*)(e + 0x34) - *(f32*)(arg1 + 0);
        f32 dy = *(f32*)(e + 0x38) - *(f32*)(arg1 + 4);
        f32 dz = *(f32*)(e + 0x3C) - *(f32*)(arg1 + 8);
        f32 d2 = (dz * dz) + ((dx * dx) + (dy * dy));
        if (d2 < best) {
            sel = e;
            best = d2;
        }
    }
    return sel != NULL ? *(u8**)sel : NULL;
}

// 0x800D4C64
// Clears a 0x1AC-byte record: zero words at +0..+0x30 and +0x50..+0x5C, floats +0x34..+0x3C = 0,
// flag words +0x40 and +0x60 = 1, bytes +0x64 = 0 and +0x9C = 1, and memsets at +0x44 (12 bytes)
// and +0x68 (0x28 bytes); +0x90..+0xA0 and +0xA4..+0x1A8 are zeroed. Returns p.
u8* fn_800D4C64(u8* arg0) {
    for (s32 off = 0; off <= 0x30; off += 4) {
        *(s32*)(arg0 + off) = 0;
    }
    *(f32*)(arg0 + 0x34) = 0.0f;
    *(f32*)(arg0 + 0x38) = 0.0f;
    *(f32*)(arg0 + 0x3C) = 0.0f;
    *(s32*)(arg0 + 0x40) = 1;
    memset(arg0 + 0x44, 0, 0xC);
    *(s32*)(arg0 + 0x50) = 0;
    *(s32*)(arg0 + 0x54) = 0;
    *(s32*)(arg0 + 0x58) = 0;
    *(s32*)(arg0 + 0x5C) = 0;
    *(s32*)(arg0 + 0x60) = 1;
    *(u8*)(arg0 + 0x64) = 0;
    memset(arg0 + 0x68, 0, 0x28);
    *(s32*)(arg0 + 0x90) = 0;
    *(s32*)(arg0 + 0x94) = 0;
    *(s32*)(arg0 + 0x98) = 0;
    *(u8*)(arg0 + 0x9C) = 1;
    *(s32*)(arg0 + 0xA0) = 0;
    for (s32 off = 0xA4; off <= 0x1A0; off += 4) {
        *(s32*)(arg0 + off) = 0;
    }
    *(s32*)(arg0 + 0x1A4) = 0;
    *(s32*)(arg0 + 0x1A8) = 0;
    return arg0;
}

// 0x80145F9C
// Quaternion slerp: out = slerp(a, b, t). Takes the shorter arc (negates b when dot < 0) and
// falls back to a linear blend when the quaternions are nearly parallel.
void fn_80145F9C(f32* arg0, f32* arg1, f32* arg2, f32 farg0) {
    f32 t = farg0;
    f32 bx = arg2[0], by = arg2[1], bz = arg2[2], bw = arg2[3];
    f32 dot = (arg1[3] * bw) + ((arg1[2] * bz) + ((arg1[0] * bx) + (arg1[1] * by)));
    f32 q[4];
    if (dot < 0.0f) {
        q[0] = -bx;
        q[1] = -by;
        q[2] = -bz;
        q[3] = -bw;
        dot = -dot;
    } else {
        q[0] = bx;
        q[1] = by;
        q[2] = bz;
        q[3] = bw;
    }
    f32 s0, s1 = t;
    if ((1.0f - dot) > 0.0001f) {
        f32 c = dot;
        if (c < -1.0f) c = -1.0f;
        else if (c > 1.0f) c = 1.0f;
        f32 k = 0.63661975f * (f32)fn_8033F1F4((f64)c);
        f32 inv = 1.0f / sinf(1.5707964f * k);
        s0 = inv * sinf(1.5707964f * ((1.0f - t) * k));
        s1 = inv * sinf(1.5707964f * (t * k));
    } else {
        s0 = 1.0f - t;
    }
    arg0[0] = (s0 * arg1[0]) + (s1 * q[0]);
    arg0[1] = (s0 * arg1[1]) + (s1 * q[1]);
    arg0[2] = (s0 * arg1[2]) + (s1 * q[2]);
    arg0[3] = (s0 * arg1[3]) + (s1 * q[3]);
}

typedef void (*FreeFn)(void* p, void* heap);

// Releases a pooled slot object: drops the refcount of its resource at +0x14 (calling
// fn_8016A2B8 when it reaches 0), clears the slot and frees it through the heap object.
static void FreeSlotObj(u8* obj) {
    u8* res = *(u8**)(obj + 0x14);
    if (res != NULL) {
        s32 rc = *(s32*)(res + 0x10) - 1;
        *(s32*)(res + 0x10) = rc;
        if (rc == 0) fn_8016A2B8(res);
        *(s32*)(obj + 0x14) = 0;
    }
    *(s32*)obj = 0;
    (*(FreeFn*)(lbl_805FB608 + 4))(obj, lbl_805FB608);
}

// Finds the index of the 8-byte entry whose first word is key in the table tbl (count at +8,
// entries at *(tbl+0xC)); -1 if there is none or tbl is NULL.
static s32 FindTableKey(u8* tbl, u32 key) {
    if (tbl == NULL) return -1;
    s32 n = *(s32*)(tbl + 8);
    for (s32 i = 0; i < n; i++) {
        if (*(u32*)(*(u8**)(tbl + 0xC) + i * 8) == key) return i;
    }
    return -1;
}

// 0x8018C0A8
// Resets the three 0x24-byte slots (each with an owned object at +0x1C, notified through
// fn_8017493C) and the three objects at +0x74/+0x78/+0x7C, runs fn_8018A5F4, stores arg1 at
// +0x70, then (after releasing the slots again) looks up the table at *(+0x70)+0x4C and caches the
// indices of the entries with keys 0x8AF38396 (+0xA0) and 0xBD126405 (+0xA4).
void fn_8018C0A8(u8* arg0, u8* arg1) {
    for (s32 i = 0; i < 3; i++) {
        u8* slot = arg0 + i * 0x24;
        if (*(s32*)(slot + 8) != 0) {
            fn_8017493C(*(void**)(slot + 8));
            *(s32*)(slot + 8) = 0;
            FreeSlotObj(*(u8**)(slot + 0x1C));
            *(s32*)(slot + 0x1C) = 0;
        }
    }
    for (s32 off = 0x74; off <= 0x7C; off += 4) {
        u8* o = *(u8**)(arg0 + off);
        if (o != NULL) {
            FreeSlotObj(o);
            *(s32*)(arg0 + off) = 0;
        }
    }
    fn_8018A5F4(arg0);
    *(u8**)(arg0 + 0x70) = arg1;
    *(s32*)(arg0 + 0) = 0;
    for (s32 i = 0; i < 3; i++) {
        u8* slot = arg0 + i * 0x24;
        if (*(s32*)(slot + 8) != 0) {
            fn_8017493C(*(void**)(slot + 8));
            *(s32*)(slot + 8) = 0;
            FreeSlotObj(*(u8**)(slot + 0x1C));
            *(s32*)(slot + 0x1C) = 0;
        }
    }
    u8* tbl = *(u8**)(*(u8**)(arg0 + 0x70) + 0x4C);   // fn_801DA7C0(*(arg0+0x70))
    *(u8**)arg0 = tbl;
    *(s32*)(arg0 + 0xA0) = FindTableKey(tbl, 0x8AF38396u);
    *(s32*)(arg0 + 0xA4) = FindTableKey(*(u8**)arg0, 0xBD126405u);
}

}  // extern "C"
