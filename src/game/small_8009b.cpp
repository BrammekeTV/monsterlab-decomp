// More small functions in 0x8009xxxx (robot evolution experiment).
#include "types.h"

extern "C" {
void  GameFlowSignal(s32 id, s32 a, s32 b);                       // 0x8000C7F4
void  operator_delete(void* p);                                   // 0x801D426C
void  TextureRelease(void* tex);                                  // 0x80184028
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);             // 0x800690D4 (base ctor)
void  fn_80062CE0(u8* p, s32 del);                                // 0x80062CE0 (base dtor)
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8); // 0x8006FA44
void  fn_80093A88(u8* p);                                         // 0x80093A88 (base dtor)
void  fn_800980F8(u8* p, s32 b);                                  // 0x800980F8
void  fn_800995AC(u8* p, s32 b);                                  // 0x800995AC
void  fn_80099874(u8* p, s32 b);                                  // 0x80099874
void  fn_801F94B0(u8* p, s32 v);                                  // 0x801F94B0
extern f32 lbl_805FDB28;                                          // 0x805FDB28 (float)
extern f32 lbl_805FDB64;                                          // 0x805FDB64 (float)
extern f32 lbl_805FDB80;                                          // 0x805FDB80 (float)
extern f32 lbl_805FDBA0;                                          // 0x805FDBA0 (float)
extern f32 lbl_805FDBC8;                                          // 0x805FDBC8 (float)
extern f32 lbl_803C6AB4[3];                                       // 0x803C6AB4
extern u8  lbl_803C6AA0[];                                        // 0x803C6AA0
extern u8* lbl_803C6A70[3];                                       // 0x803C6A70 (robot table)
extern u8* lbl_803C6A88[3];                                       // 0x803C6A88 (smasher table)
extern u8  lbl_8035C538[];                                        // 0x8035C538 (vtable)
extern u8  lbl_8035C638[];                                        // 0x8035C638 (vtable)
extern u8  lbl_803C4298[];                                        // 0x803C4298 (level info block)

// 0x800929D8
// Releases the texture at +0xC (decrements its refcount at +0x24) and clears +0xC.
void fn_800929D8(u8* p) {
    u8* tex = *(u8**)(p + 0xC);
    if (tex == NULL) return;
    if (--*(s32*)(tex + 0x24) == 0) TextureRelease(tex);
    *(s32*)(p + 0xC) = 0;
}

// 0x80092E40
// Finds the robot table entry whose key at +4 matches obj+0x10, and sets its +8 to 0.
void RobotEvolutionExperiment_RobotReadyToLaunch(u8* obj) {
    u32 key = *(u32*)(obj + 0x10);
    u8* r = NULL;
    for (s32 i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A70[i] + 4) == key) { r = lbl_803C6A70[i]; break; }
    }
    if (r == NULL) return;
    *(s32*)(r + 0x8) = 0;
}

// 0x80093168
// Same lookup as RobotReadyToLaunch; sets the found entry's +8 to 3.
void RobotEvolutionExperiment_RobotStartFleeCycle(u8* obj) {
    u32 key = *(u32*)(obj + 0x10);
    u8* r = NULL;
    for (s32 i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A70[i] + 4) == key) { r = lbl_803C6A70[i]; break; }
    }
    if (r == NULL) return;
    *(s32*)(r + 0x8) = 3;
}

// 0x800938A4
// Resets a 0x28-byte struct: fields set to -1 / 0, byte at +0x1E set to 1.
void fn_800938A4(u8* p) {
    *(s32*)(p + 0x0)  = -1;
    *(s32*)(p + 0x4)  = 0;
    *(s32*)(p + 0x8)  = -1;
    *(s32*)(p + 0xC)  = 0;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = 0;
    *(s32*)(p + 0x18) = 0;
    *(u8*)(p + 0x1C)  = 0;
    *(u8*)(p + 0x1D)  = 0;
    *(u8*)(p + 0x1E)  = 1;
    *(u8*)(p + 0x24)  = 0;
}

// 0x800938E0
// Base dtor on a non-null object, then operator delete if del > 0. Returns the object.
u8* fn_800938E0(u8* p, s32 del) {
    if (p != NULL) {
        fn_80093A88(p);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80093CF0
// Returns the smasher table entry whose key at +4 matches key, or NULL.
u8* fn_80093CF0(u32 key) {
    for (s32 i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A88[i] + 4) == key) return lbl_803C6A88[i];
    }
    return NULL;
}

// 0x80093D3C
// Smasher table lookup by obj+0x10 key; sets the found entry's +8 to 0.
void RobotEvolutionExperiment_SmasherIdling(u8* obj) {
    u8* r = fn_80093CF0(*(u32*)(obj + 0x10));
    if (r == NULL) return;
    *(s32*)(r + 0x8) = 0;
}

// 0x80093D9C
// Smasher table lookup by obj+0x10 key; sets the found entry's +8 to 1.
void RobotEvolutionExperiment_SmasherDropping(u8* obj) {
    u8* r = fn_80093CF0(*(u32*)(obj + 0x10));
    if (r == NULL) return;
    *(s32*)(r + 0x8) = 1;
}

// 0x80093DFC
// Smasher table lookup by obj+0x10 key; sets the found entry's +8 to 2.
void RobotEvolutionExperiment_SmasherRaising(u8* obj) {
    u8* r = fn_80093CF0(*(u32*)(obj + 0x10));
    if (r == NULL) return;
    *(s32*)(r + 0x8) = 2;
}

// 0x8009584C
// Calls fn_800995AC and fn_80099874 on the pointer at +0x90 (second arg 0).
void fn_8009584C(u8* p) {
    fn_800995AC(*(u8**)(p + 0x90), 0);
    fn_80099874(*(u8**)(p + 0x90), 0);
}

// 0x80095A04
// Adds dt to the float at +0x8C; when it reaches lbl_805FDB64, signal the game flow.
void fn_80095A04(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x8C) + dt;
    *(f32*)(p + 0x8C) = t;
    if (!(t >= lbl_805FDB64)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x80095BB4
// Same as fn_80095A04 with threshold lbl_805FDB80.
void fn_80095BB4(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x8C) + dt;
    *(f32*)(p + 0x8C) = t;
    if (!(t >= lbl_805FDB80)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x80097530
// Fills the 3-float table at lbl_803C6AB4, then calls fn_8006FA44 on lbl_803C6AA0.
void fn_80097530(void) {
    lbl_803C6AB4[0] = lbl_805FDB28;
    lbl_803C6AB4[1] = lbl_805FDB64;
    lbl_803C6AB4[2] = lbl_805FDB28;
    fn_8006FA44(lbl_803C6AA0, 0, 3, (s32)0x0DA38580u, (s32)0x85AB5092u, 0);
}

// 0x80097590
// Operator delete if p is non-null and del > 0. Returns p.
u8* fn_80097590(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
    return p;
}

// 0x80097B78
// Calls fn_800980F8(p, 0), then initializes the fields at +0x264..+0x278.
void fn_80097B78(u8* p) {
    fn_800980F8(p, 0);
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = -1;
    *(f32*)(p + 0x270) = lbl_805FDBA0;
    *(f32*)(p + 0x274) = lbl_805FDBA0;
    *(f32*)(p + 0x278) = lbl_805FDBC8;
}

// 0x80098664
// Constructor: base ctor fn_800690D4, then sets vtable lbl_8035C538. Returns obj.
void* fn_80098664(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0) = lbl_8035C538;
    return obj;
}

// 0x800986A0
// Base dtor (del 0) on a non-null object, then operator delete if del > 0. Returns the object.
u8* fn_800986A0(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80098BD8
// Constructor: base ctor fn_800690D4, vtable lbl_8035C638, clears +0x264, then fn_801F94B0 on +0x268. Returns obj.
void* fn_80098BD8(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0) = lbl_8035C638;
    *(s32*)(p + 0x264) = 0;
    fn_801F94B0(p + 0x268, *(s32*)(lbl_803C4298 + 0x38));
    return obj;
}
}
