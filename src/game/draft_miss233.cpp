// Drafts miss233: lab part-result quality screen (enter/update/exit), the aim-minigame power meter
// update (fn_800A68F8) and the 3-slot selection state machine (fn_80090D24).
#include "types.h"
#include <math.h>
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFnS32)(void* self, s32 a);             // virtual slot (self, int)
typedef void (*VFnF32)(void* self, f32 a);             // virtual slot (self, float)
typedef void (*VFnOutSelf)(f32* out, void* self);      // virtual slot (out vec3, self)
typedef void (*VFnSelf3F)(void* self, f32 a, f32 b, f32 c);

static const u32 kHashQualityGauge = 0x43F7F50Au;  // "QUALITY"

extern "C" {
BOOL  PadButtonHeld(s32 button);                                   // 0x801474EC
BOOL  PartAllowedAtRank(void* coll, s32 defIndex, s32 tier);       // 0x802191CC
void  PartComputeRequiredRank(void* lab, s32 defIndex, s32 tierIndex);
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);     // 0x80209E70
s32   RaySceneTest(void* ray);                                     // 0x80070458 (ignores its argument)
void* ScreenSetFindElement(void* set, u32 hash);                   // 0x80198F18
void* ScreenSetsCurrent(void* sets);                               // 0x8018D1E4
int   SysSprintf(char* buf, const char* fmt, ...);                 // 0x80142EFC
void  UISetProgress(void* obj, f32 t);
void  EntitySyncTransform(void* entity, int a);                    // 0x801B4F4C
void  fn_8009133C(u8*, s32);                                       // 0x8009133C
void  fn_800917FC(u8* p, s32 unused);                              // 0x800917FC
void  fn_800928F4(u8* p);                                          // 0x800928F4
void  fn_801458B0(f32* quat, f32* a, f32* b);                      // 0x801458B0
s32   fn_801A403C(u8* a3, f32 f1);                                 // 0x801A403C (leaf)
s32   fn_801A4050(u8* a3, f32 f1);                                 // 0x801A4050 (leaf)
s32   fn_801A4078(u8*, f32);                                       // 0x801A4078
f32   fn_801A4084(u8* a3);                                         // 0x801A4084
void* fn_801D40E0(s32 n);                                          // 0x801D40E0
void  fn_801EC080(u8* a, u8* b);                                   // 0x801EC080
void* fn_80209814(void* v);                                        // 0x80209814
f32   fn_800A6F00(void);                                           // 0x800A6F00

extern u32 lbl_803476C8[10];       // 0x803476C8 (5 element hashes per action slot, 2 slots)
extern u32 lbl_805FC838[2];        // 0x805FC838
extern u32 lbl_805FC840[2];        // 0x805FC840
extern u32 lbl_805FC860[2];        // 0x805FC860 (damage text hashes)
extern u32 lbl_805FC868[2];        // 0x805FC868 (cost text hashes)
extern u32 lbl_805FC870[2];        // 0x805FC870 (damage text hashes)
extern u32 lbl_805FC878[2];        // 0x805FC878 (cost text hashes)
extern f32 lbl_80346AA8[];         // 0x80346AA8 (f32 table indexed by RaySceneTest)
extern f32 lbl_80346AB8[];         // 0x80346AB8
extern f32 lbl_80346AC8[];         // 0x80346AC8
extern f32 lbl_80346AD8[];         // 0x80346AD8
extern f32 lbl_80346AE8[];         // 0x80346AE8
extern s32 lbl_803471F0[];         // 0x803471F0
extern u8  lbl_803C4298[];         // 0x803C4298
extern f32 lbl_805FB168;           // 0x805FB168
extern void* lbl_805FBBAC;         // 0x805FBBAC (voice SFX handle)
}
extern "C++" void* gLab;           // 0x805FBBC8
extern "C++" void* gScreenSets;    // 0x805FB654

extern "C" {

// 0x8001A6B0
// Sets up the part-result "quality" screen: name, quality gauge and percentage, defect or
// enhancement icon, the two action panels (attack limb highlights), part type icon, rank warning.
void minigame_gameflow_control_result_animate_quality_enter(void) {
    char buf[16];
    u32 limbs[10];
    u32 attackIcon[2];
    u32 defendIcon[2];

    u8* part = gLab != NULL ? *(u8**)((u8*)gLab + 0x3C) : NULL;
    if (gLab != NULL && part != NULL) {
        PartQuery_SetPart();
    }
    UIText_SetString_TextID(0xB7CE8A9Du, PartQuery_GetName());
    UIText_SetString_Int(0x4B75FCD1u, 0);
    s32 quality = PartQuery_GetQuality();
    SysSprintf(buf, "%d%%", quality);
    UIText_SetString(0xA03393A2u, (BOOL)(intptr_t)buf);  // lint:ok header takes BOOL
    u8* gauge = (u8*)ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), kHashQualityGauge);
    fn_801A403C(gauge, 0.0f);
    fn_801A4050(gauge, 100.0f);
    (*(VFnS32*)(*(u8**)(gauge + 0x138) + 0x30))(gauge, 0x80000);
    UISetProgress(gauge, (f32)quality);
    (*(VFnF32*)(*(u8**)(gauge + 0x138) + 0x18))(gauge, 0.0f);
    fn_801A4078(gauge, 0.33333334f);
    if (PartQuery_GetDefectName() != 0) {
        UIText_SetString_TextID(0xA83D90D0u, PartQuery_GetDefectName());
        UIImage_SetTexture(0xA98E73C6u, (BOOL)(intptr_t)PartQuery_GetDefectTexture());
        UIElement_SetVisibleFlag(0x938AC70Du, 1u);
        UIElement_SetVisibleFlag(0xCDC803FBu, 0u);
    } else if (PartQuery_GetEnhancementName() != 0) {
        UIText_SetString_TextID(0xA83D90D0u, PartQuery_GetEnhancementName());
        UIImage_SetTexture(0xA98E73C6u, (BOOL)(intptr_t)PartQuery_GetEnhancementTexture());
        UIElement_SetVisibleFlag(0xCDC803FBu, 1u);
        UIElement_SetVisibleFlag(0x938AC70Du, 0u);
    } else {
        UIElement_SetVisibleFlag(0xA83D90D0u, 0u);
        UIElement_SetVisibleFlag(0xA98E73C6u, 0u);
        UIElement_SetVisibleFlag(0xB204BD0Fu, 0u);
        UIElement_SetVisibleFlag(0xCDC803FBu, 0u);
        UIElement_SetVisibleFlag(0x938AC70Du, 0u);
    }
    UIText_SetString_TextID(0x1285C538u, PartQuery_GetActionName(0u));
    UIText_SetString_Int(0x5C9C9652u, 0);
    UIText_SetString_Int(0xC595C7E8u, 0);
    UIText_SetString_TextID(0x8B8C9482u, PartQuery_GetActionName(1u));
    UIText_SetString_Int(0xB292F77Eu, 0);
    UIText_SetString_Int(0x2CF662DDu, 0);
    for (s32 i = 0; i < 10; i++) limbs[i] = lbl_803476C8[i];
    attackIcon[0] = lbl_805FC838[0];
    attackIcon[1] = lbl_805FC838[1];
    defendIcon[0] = lbl_805FC840[0];
    defendIcon[1] = lbl_805FC840[1];
    for (s32 a = 0; a < 2; a++) {
        if (PartQuery_IsActionAttack(a) != 0) {
            for (s32 l = 0; l < 5; l++) {
                if (PartQuery_AttackTargetsLimb(a, l) != 0) {
                    UIElement_PlayUIAnim(limbs[a * 5 + l], 0xE12D1897u);
                } else {
                    UIElement_PlayUIAnim(limbs[a * 5 + l], 0xB4E9B641u);
                }
            }
            UIElement_SetVisibleFlag(attackIcon[a], 1u);
            UIElement_SetVisibleFlag(defendIcon[a], 0u);
        } else {
            for (s32 l = 0; l < 5; l++) {
                UIElement_PlayUIAnim(limbs[a * 5 + l], 0xB4E9B641u);
            }
            UIElement_SetVisibleFlag(attackIcon[a], 0u);
            UIElement_SetVisibleFlag(defendIcon[a], 1u);
        }
    }
    if (PartQuery_PartIsMech() != 0) {
        UIElement_SetVisibleFlag(0xF25835CBu, 1u);
        UIElement_SetVisibleFlag(0x2D8AF405u, 0u);
        UIElement_SetVisibleFlag(0x92825F01u, 0u);
        UIElement_PlayUIAnim(0x522D4954u, 0x0FA0645Fu);
    } else if (PartQuery_PartIsBio() != 0) {
        UIElement_SetVisibleFlag(0x2D8AF405u, 1u);
        UIElement_SetVisibleFlag(0xF25835CBu, 0u);
        UIElement_SetVisibleFlag(0x92825F01u, 0u);
        UIElement_PlayUIAnim(0x522D4954u, 0x01A85F3Du);
    } else if (PartQuery_PartIsAlchemy() != 0) {
        UIElement_SetVisibleFlag(0x92825F01u, 1u);
        UIElement_SetVisibleFlag(0xF25835CBu, 0u);
        UIElement_SetVisibleFlag(0x2D8AF405u, 0u);
        UIElement_PlayUIAnim(0x522D4954u, 0xC197D336u);
    } else {
        UIElement_SetVisibleFlag(0x92825F01u, 0u);
        UIElement_SetVisibleFlag(0xF25835CBu, 0u);
        UIElement_SetVisibleFlag(0x2D8AF405u, 0u);
    }
    UIText_SetString_Int(0x252A1B41u, *(s32*)((u8*)gLab + 0x78));
    UIElement_SetDoHitTest(0x77758B5Fu, 0u);
    UIElement_SetDoHitTest(0x275D8DCAu, 0u);
    UIElement_SetDoHitTest(0x185208B1u, 0u);
    UIElement_SetDoHitTest(0x6D1292E3u, 0u);
    UIElement_SetDoHitTest(0x820A9463u, 0u);
    UIElement_SetDoHitTest(0x3F7E2AA9u, 0u);
    UIElement_SetDoHitTest(0x195A4D9Eu, 0u);
    UIElement_SetDoHitTest(0x3119B8A8u, 0u);
    UIElement_SetVisibleFlag(0xF9FBD51Bu, 1u);
    UIElement_SetVisibleFlag(0x47598FEEu, 1u);
    UIElement_SetVisibleFlag(0x0718ECD0u, 1u);
    if (PartQuery_GetTexture() != NULL) {
        UIImage_SetTexture(0x4EAF4810u, (BOOL)(intptr_t)PartQuery_GetTexture());
    } else {
        UIElement_SetVisibleFlag(0x4EAF4810u, 0u);
    }
    if (part != NULL) {
        PartComputeRequiredRank(gLab, *(s32*)part, *(u8*)(part + 0x11) - 1);
    }
    if (part != NULL && PartAllowedAtRank(gLab, *(s32*)part, *(u8*)(part + 0x11) - 1) != 0) {
        UIElement_SetVisibleFlag(0xB62E1096u, 0u);
    } else {
        UIElement_SetVisibleFlag(0xB62E1096u, 1u);
    }
}

// 0x8001AD5C
// Animates the quality gauge: the displayed health / action damage / cost numbers are scaled by
// (1 - gaugeValue / quality%).
void minigame_gameflow_control_result_animate_quality_update(void) {
    u32 dmgHash[2];
    u32 costHash[2];

    s32 quality = PartQuery_GetQuality();
    u8* gauge = (u8*)ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), kHashQualityGauge);
    (*(VFnS32*)(*(u8**)(gauge + 0x138) + 0x2C))(gauge, 0x80000);
    UISetProgress(gauge, 0.0f);
    f32 k = 1.0f - (fn_801A4084(gauge) / ((f32)quality / 100.0f));
    UIText_SetString_Int(0x4B75FCD1u, (s32)(k * (f32)PartQuery_GetHealth()));
    dmgHash[0] = lbl_805FC870[0];
    dmgHash[1] = lbl_805FC870[1];
    costHash[0] = lbl_805FC878[0];
    costHash[1] = lbl_805FC878[1];
    for (s32 i = 0; i < 2; i++) {
        UIText_SetString_Int(dmgHash[i], (s32)(k * (f32)PartQuery_GetActionDamage(i)));
        UIText_SetString_Int(costHash[i], (s32)(k * (f32)PartQuery_GetActionCost((u32)i)));
    }
}

// 0x8001AF04
// Final state of the quality screen: shows the full health, damage and cost numbers.
void minigame_gameflow_control_result_animate_quality_exit(void) {
    UIText_SetString_Int(0x4B75FCD1u, PartQuery_GetHealth());
    for (s32 i = 0; i < 2; i++) {
        UIText_SetString_Int(lbl_805FC860[i], PartQuery_GetActionDamage(i));
        UIText_SetString_Int(lbl_805FC868[i], PartQuery_GetActionCost((u32)i));
    }
}

// 0x800A68F8
// Aim-minigame power meter: the value at +0x264 drains over time (or is held up while the button is
// held or it is below the floor), is clamped to the per-mode tables, then drives the aimed
// object's transform: a rotation about the axis perpendicular to its direction and a position offset.
void fn_800A68F8(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    (void)arg2;
    f32 floorVal = *(f32*)(arg0 + 0x264);
    s32 idx = RaySceneTest(arg0);
    if (floorVal > lbl_80346AC8[idx]) {
        idx = RaySceneTest(NULL);
        floorVal = lbl_80346AC8[idx];
    }
    if (*(u8*)(arg0 + 0x274) != 0) {
        idx = RaySceneTest(NULL);
        *(f32*)(arg0 + 0x264) = *(f32*)(arg0 + 0x264) - farg0 * lbl_80346AE8[idx];
        idx = RaySceneTest(NULL);
        if (floorVal > lbl_80346AD8[idx]) {
            idx = RaySceneTest(NULL);
            floorVal = lbl_80346AD8[idx];
        }
    } else {
        *(f32*)(arg0 + 0x264) = *(f32*)(arg0 + 0x264) - 10.0f * farg0;
    }
    if (*(f32*)(arg0 + 0x264) < floorVal) {
        *(f32*)(arg0 + 0x264) = floorVal;
    }
    BOOL held = PadButtonHeld(*(s32*)(lbl_803C4298 + 0x38) * 0x10 + 0x108F);
    s32 charge = 0;
    if (held != 0) {
        charge = 1;
    } else {
        idx = RaySceneTest(NULL);
        if (*(f32*)(arg0 + 0x264) < lbl_80346AC8[idx]) charge = 1;
    }
    if (charge != 0) {
        if (0.0f == *(f32*)(arg0 + 0x264)) {
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(0xC34E71F0u, 0, 0, 0, 0, 0);
        }
        idx = RaySceneTest(NULL);
        *(f32*)(arg0 + 0x264) = *(f32*)(arg0 + 0x264) + farg0 * lbl_80346AA8[idx];
        idx = RaySceneTest(NULL);
        if (*(f32*)(arg0 + 0x264) >= lbl_80346AB8[idx]) {
            idx = RaySceneTest(NULL);
            *(f32*)(arg0 + 0x264) = lbl_80346AB8[idx];
        }
    }
    u8* vt = *(u8**)(arg1 + 0xD4);
    f32 step = *(f32*)(arg0 + 0x264) * farg0;
    f32 dir[3];
    (*(VFnOutSelf*)(vt + 0x28))(dir, arg1);
    (*(VFnF32*)(*(u8**)(arg1 + 0xD4) + 0x1C))(arg1, step);
    f32 from[3];
    f32 to[3];
    f32 quat[4];
    from[0] = dir[1];
    from[1] = -dir[0];
    from[2] = dir[2];
    to[0] = 0.0f;
    to[1] = 0.0f;
    to[2] = 1.0f;
    fn_801458B0(quat, from, to);
    u8* xf = *(u8**)(arg0 + 0x4);
    *(f32*)(xf + 0xC) = quat[0];
    *(f32*)(xf + 0x10) = quat[1];
    *(f32*)(xf + 0x14) = quat[2];
    *(f32*)(xf + 0x18) = quat[3];
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    f32 spin = fn_800A6F00();
    *(f32*)(arg0 + 0x268) = spin;
    idx = RaySceneTest(NULL);
    f32 ratio = *(f32*)(arg0 + 0x264) / lbl_80346AB8[idx];
    f32 lo = 0.1f;
    if (ratio >= 0.1f) lo = ratio;
    f32 hi = 0.8f;
    if (!(lo > 0.8f)) hi = lo;
    f32 ang = *(f32*)(arg0 + 0x26C) + farg0 * (*(f32*)(arg0 + 0x268) * ((hi - 0.1f) / 0.7f));
    *(f32*)(arg0 + 0x26C) = ang;
    f32 c1 = 7.0f;
    if (!(ang > 7.0f)) c1 = ang;
    *(f32*)(arg0 + 0x26C) = c1;
    f32 c2 = -7.0f;
    if (c1 >= -7.0f) c2 = c1;
    *(f32*)(arg0 + 0x26C) = c2;
    // axis = dir x (0,0,1), normalized
    f32 ax = dir[1] * 1.0f - dir[2] * 0.0f;
    f32 ay = dir[2] * 0.0f - dir[0] * 1.0f;
    f32 az = dir[0] * 0.0f - dir[1] * 0.0f;
    f32 len2 = az * az + (ax * ax + ay * ay);
    f32 nx, ny, nz;
    if (0.0f == len2) {
        nz = 0.0f;
        ny = 0.0f;
        nx = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        nx = ax * inv;
        ny = ay * inv;
        nz = az * inv;
    }
    f32 sx = nx * c2;
    f32 sy = ny * c2;
    f32 sz = nz * c2;
    (*(VFnSelf3F*)(*(u8**)(arg1 + 0xD4) + 0x20))(arg1, sy, sx, nz);
    f32 base[3];
    (*(VFnOutSelf*)(*(u8**)(arg1 + 0xD4) + 0x24))(base, arg1);
    xf = *(u8**)(arg0 + 0x4);
    *(f32*)(arg0 + 0x278) = *(f32*)(xf + 0x0);
    f32 px = sx + base[0];
    f32 py = sy + base[1];
    f32 pz = sz + base[2];
    *(f32*)(arg0 + 0x27C) = *(f32*)(xf + 0x4);
    *(f32*)(arg0 + 0x280) = *(f32*)(xf + 0x8);
    *(f32*)(xf + 0x0) = px + 0.0f;
    *(f32*)(xf + 0x4) = py + 0.0f;
    *(f32*)(xf + 0x8) = pz + 1.0f;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    EntitySyncTransform(arg0, 0);
    fn_801EC080(arg0 + 0x94, arg0);
    if (*(f32*)(arg0 + 0x264) < 0.2f) {
        *(s32*)(arg0 + 0x288) = 3;
    } else if (*(u8*)(arg0 + 0x274) != 0) {
        *(s32*)(arg0 + 0x288) = 1;
    } else {
        *(s32*)(arg0 + 0x288) = 0;
    }
    idx = RaySceneTest(NULL);
    lbl_805FB168 = *(f32*)(arg0 + 0x264) / lbl_80346AB8[idx];
}

// Returns 1 when each of the three slot states (+0xB0/+0xF8/+0x140, stride 0x48) is "2" exactly
// when it is not the selected index (+0x324 is the selected slot).
static s32 SlotStatesMatchSelection(u8* p) {
    static const s32 kOff[3] = {0xB0, 0xF8, 0x140};
    for (s32 i = 0; i < 3; i++) {
        s32 st = *(s32*)(p + kOff[i]);
        if (st != 2 && *(s32*)(p + 0x324) != i) return 0;
        if (st == 2 && *(s32*)(p + 0x324) == i) return 0;
    }
    return 1;
}

// 0x80090D24
// Per-frame state machine (state at +0x7FC) for the three-slot pick sequence: 0 waits for all
// slots idle, 1 waits for all ready, 2 counts down the timers and detects the chosen slot,
// 3 waits for the pick animation to finish, 4 counts rounds. Transitions via fn_8009133C.
void fn_80090D24(u8* arg0, f32 farg0) {
    switch (*(s32*)(arg0 + 0x7FC)) {
    case 0: {
        s32 ok = 1;
        if (*(s32*)(arg0 + 0xB0) != 0) ok = 0;
        if (*(s32*)(arg0 + 0x1FC) != 1) ok = 0;
        if (*(s32*)(arg0 + 0xF8) != 0) ok = 0;
        if (*(s32*)(arg0 + 0x210) != 1) ok = 0;
        if (*(s32*)(arg0 + 0x140) != 0) ok = 0;
        if (*(s32*)(arg0 + 0x224) != 1) ok = 0;
        if (ok != 0) fn_8009133C(arg0, 1);
        break;
    }
    case 1: {
        s32 ok = 1;
        if (*(s32*)(arg0 + 0xB0) != 1) ok = 0;
        if (*(s32*)(arg0 + 0xF8) != 1) ok = 0;
        if (*(s32*)(arg0 + 0x140) != 1) ok = 0;
        if (ok != 0) fn_8009133C(arg0, 2);
        break;
    }
    case 2: {
        *(f32*)(arg0 + 0x7EC) = *(f32*)(arg0 + 0x7EC) - farg0;
        *(f32*)(arg0 + 0x7F0) = *(f32*)(arg0 + 0x7F0) - farg0;
        s32 expired;
        if (lbl_803471F0[RaySceneTest(arg0)] == 0) {
            expired = (*(f32*)(arg0 + 0x7EC) <= 0.0f) || (*(f32*)(arg0 + 0x7E8) <= 0.0f);
        } else {
            expired = *(f32*)(arg0 + 0x7EC) <= 0.0f;
        }
        if (expired != 0) {
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(0xF48CB226u, 0, 0, 0, 0, 0);
            *(u8*)(arg0 + 0x7E4) = 0;
            s32 none;
            if (*(s32*)(arg0 + 0x188) == 1) none = 0;
            else if (*(s32*)(arg0 + 0x1B0) == 1) none = 0;
            else if (*(s32*)(arg0 + 0x1D8) == 1) none = 0;
            else none = 1;
            if ((u32)none == 1u) {
                *(u8*)(arg0 + 0x808) = 0;
                fn_8009133C(arg0, 4);
            } else {
                fn_8009133C(arg0, 3);
            }
            break;
        }
        if (*(f32*)(arg0 + 0x7F0) <= 0.0f) {
            u8* slot = arg0 + 0xA8;
            for (s32 i = 0; i < 3; i++, slot += 0x48) {
                fn_800928F4(slot);
            }
        }
        *(f32*)(arg0 + 0x804) = *(f32*)(arg0 + 0x804) + *(f32*)(arg0 + 0x800) * farg0;
        s32 found = 1;
        u8* a = arg0;
        u8* b = arg0;
        for (s32 i = 0; i < 3; i++, a += 0x48, b += 0x28) {
            s32 st = *(s32*)(a + 0xB0);
            s32 cur = *(s32*)(b + 0x188);
            s32 pending = (cur == 1 || cur == 2);
            if (st != 2 && pending == 0 && *(s32*)(arg0 + 0x324) != i) {
                found = 0;
                continue;
            }
            if (st == 2 || pending == 1) {
                if (*(s32*)(arg0 + 0x324) == i) {
                    found = 1;
                    break;
                }
            }
        }
        if (found != 0) {
            *(u8*)(arg0 + 0x7E4) = 0;
            *(u8*)(arg0 + 0x808) = (u8)SlotStatesMatchSelection(arg0);
            fn_8009133C(arg0, 3);
        }
        break;
    }
    case 3: {
        s32 none;
        if (*(s32*)(arg0 + 0x188) == 1) none = 0;
        else if (*(s32*)(arg0 + 0x1B0) == 1) none = 0;
        else if (*(s32*)(arg0 + 0x1D8) == 1) none = 0;
        else none = 1;
        if ((u32)none == 1u) {
            *(u8*)(arg0 + 0x808) = (u8)SlotStatesMatchSelection(arg0);
            fn_8009133C(arg0, 4);
        }
        break;
    }
    case 4: {
        s32 ok = 1;
        if (*(s32*)(arg0 + 0xB0) == 3) ok = 0;
        if (*(s32*)(arg0 + 0xF8) == 3) ok = 0;
        if (*(s32*)(arg0 + 0x140) == 3) ok = 0;
        if (ok != 0) {
            s32 rounds = *(s32*)(arg0 + 0x354) + 1;
            *(s32*)(arg0 + 0x354) = rounds;
            if (rounds > *(s32*)(arg0 + 0x2A8) || *(f32*)(arg0 + 0x7E8) <= 0.0f) {
                fn_800917FC(arg0, *(f32*)(arg0 + 0x7E8) <= 0.0f);
            } else {
                fn_8009133C(arg0, 0);
            }
        }
        break;
    }
    }
}

}  // extern "C"
