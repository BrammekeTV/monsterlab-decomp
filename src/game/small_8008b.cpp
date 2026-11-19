// More small functions in 0x8008xxxx (brain toss / brain head).
#include "types.h"

// The header declares the script callbacks (BrainHeadEatMeat, SetPropState, GetPropCurrentState) with C linkage.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8  gWorldState[];          // 0x803E6D00
extern u8  lbl_8035B160[];         // 0x8035B160 (vtable)
extern u8  lbl_8035B3A0[];         // 0x8035B3A0 (vtable)
extern u8  lbl_8035B570[];         // 0x8035B570 (vtable)
extern u8  lbl_8035B5A0[];         // 0x8035B5A0 (vtable)
extern u8  lbl_803466A0[];         // 0x803466A0 (f32 table indexed by RaySceneTest)
extern u8  lbl_803466B0[];         // 0x803466B0 (f32 table indexed by RaySceneTest)
extern u8  lbl_80346700[];         // 0x80346700 (s32 table indexed by RaySceneTest)
extern u8  lbl_80347D88[];         // 0x80347D88 (s32 table of prop states)
extern f32 lbl_805FD800;           // 0x805FD800 (float constant)
extern f32 lbl_805FD804;           // 0x805FD804 (float constant)
extern f32 lbl_805FD848;           // 0x805FD848 (float constant)
extern f32 lbl_805FD87C;           // 0x805FD87C (float constant)

extern "C" {
void  operator_delete(void* p);                                   // 0x801D426C
s32   RaySceneTest(void* ray);                                    // 0x80070458
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);             // 0x800690D4 (base ctor)
void* fn_80083710(void* obj, s32 del);                            // 0x80083710 (base dtor call + delete)
void  fn_80083770(u8* a3, s32 a4);                                // 0x80083770
void  fn_8008378C(u8* a3);                                        // 0x8008378C
u8*   fn_801B161C(u8* p, u32 key);                                // 0x801B161C

// 0x80081A04
// Freebie duration (f32) for the RaySceneTest-selected entry. Named with the address because
// BrainTossFreebieDuration is already defined at 0x80083B18 in small_8008.cpp.
f32 BrainTossFreebieDuration_80081A04(void* ray) {
    s32 idx = RaySceneTest(ray);  // the original forwards its incoming r3 unchanged
    return ((f32*)lbl_803466B0)[idx];
}

// 0x8008139C
// Replaces the object held at +0x2A8. If an old object is present, it gets a 1 call on its
// vtable slot 0x8 (release) and the slot is cleared, then the new value is stored.
void fn_8008139C(u8* self, void* val) {
    void* old = *(void**)(self + 0x2A8);
    if (old != 0) {
        (*(void (**)(void*, s32))(*(u8**)old + 0x8))(old, 1);
        *(void**)(self + 0x2A8) = 0;
    }
    *(void**)(self + 0x2A8) = val;
}

// 0x80081A34
// Finds the first entry of the list in gWorldState (count +0x24, base +0x28, 0x20 bytes each)
// whose first word is 0xAAE1A370 (the original compares word + 0x551F0000 with 0xA370).
// Returns the entry, or 0 when none matches.
u8* fn_80081A34(void) {
    u8* list = *(u8**)gWorldState;
    s32 count = *(s32*)(list + 0x24);
    u8* base = *(u8**)(list + 0x28);
    s32 idx = -1;
    for (s32 i = 0; i < count; i++) {
        if (*(u32*)(base + i * 0x20) == 0xAAE1A370u) {
            idx = i;
            break;
        }
    }
    if (idx < 0 || idx >= count) return 0;
    return base + idx * 0x20;
}

// 0x80081E84
// Resets the brain toss sub-object: zeroes the vector at +0 (x, y = lbl_805FD800, z = lbl_805FD804),
// sets flag bits 0x8000, 0x4000 and 0x10000000 in its word at +0x20, and clears the state fields.
// (The original's stack temporaries are dead and are omitted.)
void fn_80081E84(u8* self) {
    *(s32*)(self + 0x2C0) = 0;
    u8* sub = *(u8**)(self + 0x4);
    *(f32*)(sub + 0x0) = lbl_805FD800;
    *(f32*)(sub + 0x4) = lbl_805FD800;
    *(f32*)(sub + 0x8) = lbl_805FD804;
    *(u32*)(sub + 0x20) |= 0x1000C000u;
    *(f32*)(self + 0x274) = lbl_805FD800;
    *(u8*)(self + 0x268) = 0;
}

// 0x80082B20
// Copies four floats from src into the sub-object at +0xC..+0x18 and sets the same flag bits as
// fn_80081E84 (0x8000, 0x4000, 0x10000000) on its word at +0x20.
void fn_80082B20(u8* self, f32* src) {
    u8* sub = *(u8**)(self + 0x4);
    *(f32*)(sub + 0xC)  = src[0];
    *(f32*)(sub + 0x10) = src[1];
    *(f32*)(sub + 0x14) = src[2];
    *(f32*)(sub + 0x18) = src[3];
    *(u32*)(sub + 0x20) |= 0x1000C000u;
}

// 0x800836A0
// Base constructor: runs fn_800690D4 on the object, then sets the vtable (lbl_8035B160) and clears
// the timers and state fields at +0x264..+0x280 to zero. Returns the object.
void* fn_800836A0(void* obj, s32 a, void* params, s32 b) {
    fn_800690D4((u8*)obj, a, params, b);
    u8* self = (u8*)obj;
    *(void**)(self + 0x0) = lbl_8035B160;
    *(s32*)(self + 0x264) = 0;
    *(f32*)(self + 0x268) = lbl_805FD848;
    *(f32*)(self + 0x26C) = lbl_805FD848;
    *(u8*)(self + 0x270) = 0;
    *(f32*)(self + 0x274) = lbl_805FD848;
    *(f32*)(self + 0x278) = lbl_805FD848;
    *(f32*)(self + 0x27C) = lbl_805FD848;
    *(s32*)(self + 0x280) = 0;
    return obj;
}

// 0x800846F0
// Brain head "eat meat" tick, called from script. Adds the frame step (gWorldState +0xC) to the
// timer at +0x268. While that timer is exactly lbl_805FD87C, it also picks a value from
// lbl_803466A0 (indexed by RaySceneTest) into +0x26C.
void BrainHeadEatMeat(u32 a0) {
    u8* self = (u8*)(uintptr_t)a0;
    f32 dt = *(f32*)(gWorldState + 0xC);
    if (lbl_805FD87C == *(f32*)(self + 0x268)) {
        s32 idx = RaySceneTest(self);
        *(f32*)(self + 0x26C) = ((f32*)lbl_803466A0)[idx];
    }
    *(f32*)(self + 0x268) += dt;
}

// 0x80084D3C
// Copies a 3-float vector (+0x14..+0x1C) from the entry looked up by hash 0x17C7F120 into out.
// The lookup argument is the double-dereferenced pointer at sub+0xC, or 0 when sub+0x10 is 0.
void fn_80084D3C(f32* out, u8* ctx) {
    u8* sub = *(u8**)(ctx + 0x8C);
    u8* arg = 0;
    if (*(s32*)(sub + 0x10) != 0) {
        arg = *(u8**)(*(u8**)(sub + 0xC));
    }
    u8* v = fn_801B161C(arg, 0x17C7F120u);
    out[0] = *(f32*)(v + 0x14);
    out[1] = *(f32*)(v + 0x18);
    out[2] = *(f32*)(v + 0x1C);
}

// 0x8008513C
// Deleting destructor with no body: frees the object when del > 0. Returns the object.
void* fn_8008513C(void* obj, s32 del) {
    if (obj != 0 && del > 0) operator_delete(obj);
    return obj;
}

// 0x80085340
// Destructor: sets the vtable (lbl_8035B5A0), copies the byte at obj+0x10 into the sub-object at
// +0xC0, and frees the object when del > 0. Returns the object.
void* fn_80085340(void* obj, s32 del) {
    if (obj != 0) {
        u8* self = (u8*)obj;
        u8 flag = self[0x10];
        u8* sub = *(u8**)(self + 0x4);
        *(void**)self = lbl_8035B5A0;
        sub[0xC0] = flag;
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x800853B4
// Destructor: sets the vtable (lbl_8035B570), divides the sub-object float at +0xC4 by the float
// at obj+0x10, and frees the object when del > 0. Returns the object.
void* fn_800853B4(void* obj, s32 del) {
    if (obj != 0) {
        u8* self = (u8*)obj;
        f32 divisor = *(f32*)(self + 0x10);
        u8* sub = *(u8**)(self + 0x4);
        *(void**)self = lbl_8035B570;
        *(f32*)(sub + 0xC4) = *(f32*)(sub + 0xC4) / divisor;
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x80085424
// Deleting destructor with no body: frees the object when del > 0. Returns the object.
void* fn_80085424(void* obj, s32 del) {
    if (obj != 0 && del > 0) operator_delete(obj);
    return obj;
}

// 0x80085494
// Deleting destructor with no body: frees the object when del > 0. Returns the object.
void* fn_80085494(void* obj, s32 del) {
    if (obj != 0 && del > 0) operator_delete(obj);
    return obj;
}

// 0x800856B8
// Deleting destructor with no body: frees the object when del > 0. Returns the object.
void* fn_800856B8(void* obj, s32 del) {
    if (obj != 0 && del > 0) operator_delete(obj);
    return obj;
}

// 0x800856F8
// Lowers the float at obj+0xC by dt. The object at sub+0x1F4 -> +0x264 (if present) is put into
// state 2, given the state from lbl_80347D88[2] via SetPropState, and gets bit 0x400000 at +0x15C.
void fn_800856F8(u8* obj, f32 dt) {
    *(f32*)(obj + 0xC) -= dt;
    u8* sub = *(u8**)(obj + 0x4);
    u8* ent = *(u8**)(sub + 0x1F4);
    u8* prop = *(u8**)(ent + 0x264);
    if (prop != 0) {
        *(s32*)(prop + 0x264) = 2;
        SetPropState((u8*)prop, (BOOL)*(s32*)(lbl_80347D88 + 0x8), 0);
        *(u32*)(prop + 0x15C) |= 0x400000u;
    }
}

// 0x80085768
// Deleting destructor with no body: frees the object when del > 0. Returns the object.
void* fn_80085768(void* obj, s32 del) {
    if (obj != 0 && del > 0) operator_delete(obj);
    return obj;
}

// 0x80085AD8
// Constructor: runs the base constructor fn_800836A0, sets the vtable (lbl_8035B3A0), and sets
// the counter at +0x284 to -1 and the value at +0x288 to 0. Returns the object.
void* fn_80085AD8(void* obj, s32 a, void* params, s32 b) {
    fn_800836A0(obj, a, params, b);
    u8* self = (u8*)obj;
    *(void**)self = lbl_8035B3A0;
    *(s32*)(self + 0x284) = -1;
    *(s32*)(self + 0x288) = 0;
    return obj;
}

// 0x80085B24
// Destructor: runs the base destructor (fn_80083710 with del 0), then frees the object when
// del > 0. Returns the object.
void* fn_80085B24(void* obj, s32 del) {
    if (obj != 0) {
        fn_80083710(obj, 0);
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x80085B7C
// Advances the cycle counter at +0x284 (values 0..2, from -1 start) and stores the table value
// from lbl_80346700 for a RaySceneTest-selected index at +0x288. RaySceneTest is called with no
// meaningful argument (the original passes leftover register contents, which it ignores).
void fn_80085B7C(u8* obj, s32 a) {
    fn_80083770(obj, a);
    s32 cnt = *(s32*)(obj + 0x284);
    *(s32*)(obj + 0x284) = (cnt + 1) % 3;
    s32 idx = RaySceneTest(0);
    *(s32*)(obj + 0x288) = ((s32*)lbl_80346700)[idx];
}

// 0x80085BE8
// Calls fn_8008378C, then if the object's prop state is nonzero, sets it to 0 via SetPropState.
void fn_80085BE8(u8* obj) {
    fn_8008378C(obj);
    if (GetPropCurrentState((u32)(uintptr_t)obj)) {
        SetPropState((u8*)obj, 0, 0);
    }
}
}
