// Small functions in 0x8008xxxx (brain toss minigame).
#include "types.h"

// The header declares the script callbacks (BrainToss*, SetPropState) with C linkage.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8  lbl_803C6970[];         // 0x803C6970 (init block for fn_8006FA44)
extern f32 lbl_803C6984[];         // 0x803C6984 (3 floats)
extern f32 lbl_803C699C[];         // 0x803C699C (3 floats)
extern f32 lbl_803C6990[];         // 0x803C6990 (3 floats)
extern u8  lbl_803465C0[];         // 0x803465C0 (s32 table indexed by RaySceneTest)
extern u8  lbl_80347D88[];         // 0x80347D88 (s32 table of prop states)
extern u8  lbl_8035B268[];         // 0x8035B268 (vtable)
extern u8  lbl_8035B638[];         // 0x8035B638 (vtable)
extern u8  lbl_803464C8[];         // 0x803464C8 (f32 scale table indexed by RaySceneTest)
extern f32 lbl_805FD770;           // 0x805FD770 (float constant)
extern f32 lbl_805FD788;           // 0x805FD788 (float constant)
extern f32 lbl_805FD800;           // 0x805FD800 (float constant)
extern f32 lbl_805FD824;           // 0x805FD824 (float constant)
extern f32 lbl_805FD834;           // 0x805FD834 (float constant)
extern f32 lbl_805FD878;           // 0x805FD878 (float constant)
extern f32 lbl_805FD8D0;           // 0x805FD8D0 (float constant)

extern "C" {
void  fn_80083770(u8* a3, s32 a4);                                // 0x80083770
u8*   fn_801B161C(u8* p, u32 key);                                // 0x801B161C
void  operator_delete(void* p);                                   // 0x801D426C
u8*   fn_8006FBDC(void);                                          // 0x8006FBDC
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8); // 0x8006FA44
u8*  fn_80062CE0(u8* p, s32 del);                                // 0x80062CE0 (base dtor)
u8*  fn_800836A0(void* obj, s32 a, void* params, s32 b);         // 0x800836A0 (base ctor)
void  fn_80080DC0(u8* a, u8* b);                                  // 0x80080DC0
s32   RaySceneTest(void* ray);                                    // 0x80070458

// 0x80080BEC
// Returns the power-up field at +0x2A0 of the brain toss state, or 0 if there is none.
u32 BrainTossGetPowerUp(void) {
    if (fn_8006FBDC() == 0) return 0;
    return *(u32*)(fn_8006FBDC() + 0x2A0);
}

// 0x80080C24
// Returns the available-chunks field at +0x2AC of the brain toss state, or 0 if there is none.
s32 BrainTossGetAvailableChunks(void) {
    if (fn_8006FBDC() == 0) return 0;
    return *(s32*)(fn_8006FBDC() + 0x2AC);
}

// 0x80080FD0
// Forwards the score update to fn_80080DC0 with the brain toss state.
void BrainTossUpdateScore(u32 a0) {
    fn_80080DC0(fn_8006FBDC(), (u8*)(uintptr_t)a0);
}

// 0x8008133C
// Adds delta to the counter at +0x2AC, clamped to the table limit selected by RaySceneTest.
void fn_8008133C(u8* self, s32 delta) {
    s32 idx = RaySceneTest(self);
    s32 v = *(s32*)(self + 0x2AC) + delta;
    s32 limit = ((s32*)lbl_803465C0)[idx];
    if (v >= limit) v = limit;
    *(s32*)(self + 0x2AC) = v;
}

// 0x80081AA8
// Sets three floats in lbl_803C6984 and tail-calls fn_8006FA44.
void fn_80081AA8(void) {
    f32 a = lbl_805FD770;
    f32 b = lbl_805FD788;
    lbl_803C6984[0] = a;
    lbl_803C6984[1] = b;
    lbl_803C6984[2] = a;
    fn_8006FA44(lbl_803C6970, (s32)0xEBE26F37u, 3, (s32)0xA3D4805Au, (s32)0x85AB5092u, (s32)0x714859F7u);
}

// 0x80081E24
// Copies a vec3 into obj->+4 and sets flag bits in its +0x20 word.
void fn_80081E24(u8* obj, f32* v) {
    u8* o = *(u8**)(obj + 4);
    ((f32*)o)[0] = v[0];
    ((f32*)o)[1] = v[1];
    ((f32*)o)[2] = v[2];
    *(u32*)(o + 0x20) |= 0x8000;
    *(u32*)(o + 0x20) |= 0x10000000;
    *(u32*)(o + 0x20) |= 0x4000;
    *(u32*)(o + 0x20) |= 0x10000000;
}

// 0x80082A74
// Scales a 4-float vector in place by a scalar.
void fn_80082A74(f32* v, f32* s) {
    f32 k = *s;
    v[0] = v[0] * k;
    v[1] = v[1] * k;
    v[2] = v[2] * k;
    v[3] = v[3] * k;
}

// 0x80082AC4
// Adds a 4-float vector b into a in place.
void fn_80082AC4(f32* a, f32* b) {
    a[0] = a[0] + b[0];
    a[1] = a[1] + b[1];
    a[2] = a[2] + b[2];
    a[3] = a[3] + b[3];
}

// 0x80082B90
// Stores the state index at +0x264, applies the matching prop state, and sets flag 0x400000.
void fn_80082B90(u8* obj, s32 idx) {
    *(s32*)(obj + 0x264) = idx;
    SetPropState((u8*)obj, (BOOL)((s32*)lbl_80347D88)[idx], 0);
    *(u32*)(obj + 0x15C) |= 0x400000;
}

// 0x80082F68
// Writes the negation of a 4-float vector.
void fn_80082F68(f32* out, f32* in) {
    out[0] = -in[0];
    out[1] = -in[1];
    out[2] = -in[2];
    out[3] = -in[3];
}

// 0x80083668
// Initializes two 3-float tables.
void fn_80083668(void) {
    f32 a = lbl_805FD800;
    f32 b = lbl_805FD824;
    f32 c = lbl_805FD834;
    lbl_803C699C[0] = a;
    lbl_803C699C[1] = b;
    lbl_803C699C[2] = a;
    lbl_803C6990[0] = a;
    lbl_803C6990[1] = a;
    lbl_803C6990[2] = c;
}

// 0x80083710
// Base destructor call on a non-null object, then operator_delete if del > 0. Returns obj.
void* fn_80083710(void* obj, s32 del) {
    if (obj != 0) {
        fn_80062CE0((u8*)obj, 0);
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x80083A6C
// Constructor: forwards to the base ctor, then sets the vtable and the 0x284..0x28C fields.
void* fn_80083A6C(void* obj, s32 a, void* params, s32 b) {
    fn_800836A0(obj, a, params, b);
    f32 f = lbl_805FD878;
    u8* p = (u8*)obj;
    *(u8*)(p + 0x284) = 0;
    *(u8**)p = lbl_8035B268;
    *(u8*)(p + 0x285) = 0;
    *(f32*)(p + 0x288) = f;
    *(u8*)(p + 0x28C) = 0;
    return obj;
}

// 0x80083AC0
// Deleting destructor for the 0x80083A6C object type.
void* fn_80083AC0(void* obj, s32 del) {
    if (obj != 0) {
        fn_80083710(obj, 0);
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x80083B18
// Brain toss power-up "freebie": runs the shared start (fn_80083770 with the incoming value), takes
// the new time from the object's virtual slot 0x10 (+0xB0) and scales the +0x24 value of the child
// object 0x17C7F120's transform by a table entry picked by RaySceneTest.
void fn_80083B18(u8* p, s32 value) {
    fn_80083770(p, value);
    p[0x284] = 0;
    u8* info = (*(u8* (**)(u8*))(*(u8**)p + 0x10))(p);
    if (info != NULL) *(f32*)(p + 0x288) = *(f32*)(info + 0xB0);

    u8* holder = *(u8**)(p + 0x8C);
    u8* first = *(u32*)(holder + 0x10) == 0 ? NULL : **(u8***)(holder + 0xC);
    u8* child = fn_801B161C(first, 0x17C7F120u);
    s32 idx = RaySceneTest(0);  // the original passes the child on; RaySceneTest ignores it
    *(f32*)(*(u8**)(child + 8) + 0x24) *= ((f32*)lbl_803464C8)[idx];
    p[0x28C] = 0;
}

// 0x80084E94
// Deleting destructor: operator_delete if obj is non-null and del > 0. Returns obj.
void* fn_80084E94(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80084ED4
// Deleting destructor: operator_delete if obj is non-null and del > 0. Returns obj.
void* fn_80084ED4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80084FA4
// Deleting destructor: operator_delete if obj is non-null and del > 0. Returns obj.
void* fn_80084FA4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80084FE4
// Advances the counter at obj->+4 by obj->+0x10 and sets obj->+0xC to a constant.
void fn_80084FE4(u8* obj) {
    fn_8008133C(*(u8**)(obj + 4), *(s32*)(obj + 0x10));
    *(f32*)(obj + 0xC) = lbl_805FD8D0;
}

// 0x80085020
// Sets the vtable, copies a float at +0x10 into obj->+4 +0xBC, and deletes if del > 0.
void* fn_80085020(void* obj, s32 del) {
    u8* p = (u8*)obj;
    if (p != 0) {
        f32 f = *(f32*)(p + 0x10);
        u8* inner = *(u8**)(p + 4);
        *(u8**)p = lbl_8035B638;
        *(f32*)(inner + 0xBC) = f;
        if (del > 0) operator_delete(obj);
    }
    return obj;
}
}
