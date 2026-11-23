// Small engine functions in 0x801Dxxxx (fader, shader, frame effects).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
void  fn_801D9028(u8* p);                                        // 0x801D9028 (base constructor)
void  fn_801D9160(u8* p);                                        // 0x801D9160 (base cleanup, no-op)
void  fn_801D8BB0(u8* p);                                        // 0x801D8BB0 (base cleanup, no-op)
void  fn_80173164(void* p);                                      // 0x80173164
void  fn_80185B4C(void* p);                                      // 0x80185B4C
void  fn_80184488(void* a, void* b);                             // 0x80184488
void  fn_801842E0(void* a, s32 b, s32 c);                        // 0x801842E0
s32  fn_8016195C(void* p, s32 a, s32 b, void* c);               // 0x8016195C
s32 fn_801DA34C(u8* a, u8* b);                                         // 0x801DA34C (callback stored by fn_801DA7C8)
void  RegisterEngineScriptFuncs(void);                           // 0x801DC9F4
void  RegisterInterpolateVector3TaskScripts(void);                                         // 0x801E9DB8
void  RegisterInterpolateQuaternionTaskScripts(void);                                         // 0x801EA508
void  RegisterInterpolateToNextWaypointTaskScripts(void);                                         // 0x801EAA08
void  fn_801B8550(void);                                         // 0x801B8550
}

extern f32 lbl_805FFDF0;            // 0x805FFDF0 (float constant)
extern f32 lbl_805FFEC8;            // 0x805FFEC8 (float constant)
extern u8  lbl_8047D248[];          // 0x8047D248 (fader target colour, 3 floats used)
extern u8  lbl_80348F38[];          // 0x80348F38 (shader overlay dispatch table)
extern u8  lbl_8040E5F0[];          // 0x8040E5F0 (frame effects object)
extern u8  lbl_80384470[];          // 0x80384470 (vtable)
extern u8  lbl_803844A8[];          // 0x803844A8 (vtable)
extern u8  lbl_803844E0[];          // 0x803844E0 (vtable)
extern u8  lbl_80384518[];          // 0x80384518 (vtable)

extern "C" {

// 0x801D04C0
// Returns 1 when the four floats at +0..+0xC all equal the global constant, else 0.
s32 fn_801D04C0(u8* p) {
    f32 v = lbl_805FFDF0;
    if (v == *(f32*)(p + 0x0) && v == *(f32*)(p + 0x4) &&
        v == *(f32*)(p + 0x8) && v == *(f32*)(p + 0xc)) {
        return 1;
    }
    return 0;
}

// 0x801D2BB8
// Deleting destructor for a plain object: frees it when del > 0. Returns p.
u8* fn_801D2BB8(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801D55F8
// Deleting destructor for a plain object: frees it when del > 0. Returns p.
u8* fn_801D55F8(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801D57FC
// Deleting destructor: base cleanup (fn_801D8BB0), then frees the object when del > 0.
u8* fn_801D57FC(u8* p, s32 del) {
    if (p) {
        fn_801D8BB0(p);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801D65F8
// Stores the fader target colour: the first three floats of the global colour block.
// The fourth float is written back to itself, so it is left alone.
void FaderSetTargetColour(f32 a0, f32 a1, f32 a2) {
    f32* c = (f32*)lbl_8047D248;
    c[0] = a0;
    c[1] = a1;
    c[2] = a2;
}

// 0x801D8BB4
// Per-frame timed value. If a source pointer is set at +0xC4, copies its float into +0xBC.
// Otherwise counts the remaining time at +0xD4 down by dt and scales the start value
// at +0xC0 by remaining / duration (+0xD0); when the time is used up it holds at zero.
void fn_801D8BB4(u8* p, f32 dt) {
    u8* src = (u8*)(uintptr_t)*(u32*)(p + 0xc4);
    if (src) {
        *(f32*)(p + 0xbc) = *(f32*)src;
        return;
    }
    f32 remain = *(f32*)(p + 0xd4);
    f32 zero = lbl_805FFEC8;
    if (remain > zero) {
        remain = remain - dt;
        f32 frac = remain / *(f32*)(p + 0xd0);
        *(f32*)(p + 0xd4) = remain;
        *(f32*)(p + 0xbc) = *(f32*)(p + 0xc0) * frac;
    } else {
        *(f32*)(p + 0xbc) = zero;
    }
}

// 0x801D94CC
// Constructor: base init, then sets the vtable to lbl_80384470.
void fn_801D94CC(u8* p) {
    fn_801D9028(p);
    *(void**)(p + 0x0) = (void*)lbl_80384470;
}

// 0x801D9A24
// Constructor: base init, then sets the vtable to lbl_803844A8.
void fn_801D9A24(u8* p) {
    fn_801D9028(p);
    *(void**)(p + 0x0) = (void*)lbl_803844A8;
}

// 0x801D9AE4
// Releases the object at +0x48 (if any): clears +0x34 of this object, frees the
// object through fn_80173164, and clears the pointer.
void fn_801D9AE4(u8* p) {
    u8* obj = (u8*)(uintptr_t)*(u32*)(p + 0x48);
    if (obj) {
        *(u32*)(p + 0x34) = 0;
        fn_80173164(obj);
        *(u32*)(p + 0x48) = 0;
    }
}

// 0x801DA1CC
// Constructor: base init, then sets the vtable to lbl_803844E0.
void fn_801DA1CC(u8* p) {
    fn_801D9028(p);
    *(void**)(p + 0x0) = (void*)lbl_803844E0;
}

// 0x801DA7C8
// Points each entry of the pointer array at +0x58 (count at +0x54) back at this object:
// stores fn_801DA34C at +0x30 and this object at +0x34 of every entry.
void fn_801DA7C8(u8* p) {
    if (*(u32*)(p + 0x58) == 0) return;
    if (*(s32*)(p + 0x54) <= 0) return;
    u8* arr = (u8*)(uintptr_t)*(u32*)(p + 0x58);
    for (s32 i = 0; i < *(s32*)(p + 0x54); i++) {
        u8* entry = (u8*)(uintptr_t)*(u32*)(arr + i * 4);
        *(void**)(entry + 0x30) = (void*)fn_801DA34C;
        *(void**)(entry + 0x34) = (void*)p;
    }
}

// 0x801DA81C
// Constructor: base init, then sets the vtable to lbl_80384518.
void fn_801DA81C(u8* p) {
    fn_801D9028(p);
    *(void**)(p + 0x0) = (void*)lbl_80384518;
}

// 0x801DA8D0
// Cleanup: releases the object at +0x4C (if any), clears the pointer, then base cleanup.
void fn_801DA8D0(u8* p) {
    u8* obj = (u8*)(uintptr_t)*(u32*)(p + 0x4c);
    if (obj) {
        fn_80185B4C(obj);
        *(u32*)(p + 0x4c) = 0;
    }
    fn_801D9160(p);
}

// 0x801DAA40
// Forwards an event to fn_80184488 when the event type at +0x4 is 1 or 2, and both
// the object at a+0x4C and the payload at b+0x48 are set. Otherwise does nothing.
// Return value of the tail call is not carried through (uncertain).
void fn_801DAA40(u8* a, u8* b) {
    s32 type = *(s32*)(b + 0x4);
    if (type != 1 && type != 2) return;
    u8* obj = (u8*)(uintptr_t)*(u32*)(a + 0x4c);
    if (!obj) return;
    u8* payload = (u8*)(uintptr_t)*(u32*)(b + 0x48);
    if (!payload) return;
    fn_80184488(obj, payload);
}

// 0x801DB160
// Clears the two words at +0x30/+0x34 of the object at +0x48 (if any), then calls
// fn_801842E0 on the object at +0x4C with zeros (if any).
void fn_801DB160(u8* p) {
    u8* a = (u8*)(uintptr_t)*(u32*)(p + 0x48);
    if (a) {
        *(u32*)(a + 0x30) = 0;
        *(u32*)(a + 0x34) = 0;
    }
    u8* b = (u8*)(uintptr_t)*(u32*)(p + 0x4c);
    if (!b) return;
    fn_801842E0(b, 0, 0);
}

// 0x801DB2E4
// Looks up a 0x2C-byte entry by its first word. Entries are at base (+0) with count (+4).
// Returns the matching entry, or null if none matches.
u8* fn_801DB2E4(u8* p, u32 key) {
    u32 count = *(u32*)(p + 0x4);
    u8* base = (u8*)(uintptr_t)*(u32*)(p + 0x0);
    for (u32 i = 0; i < count; i++) {
        u8* e = base + i * 0x2c;
        if (*(u32*)e == key) return e;
    }
    return nullptr;
}

// 0x801DB460
// Registers the engine script callbacks and runs the four subsystem hooks.
void RegisterHooks(void) {
    RegisterEngineScriptFuncs();
    RegisterInterpolateVector3TaskScripts();
    RegisterInterpolateQuaternionTaskScripts();
    RegisterInterpolateToNextWaypointTaskScripts();
    fn_801B8550();
}

// 0x801DB4C4
// Sets the overlay alpha of a shader object (argument is a handle; null does nothing).
// The object's type byte at +0xC picks a dispatch entry in lbl_80348F38; that entry is
// called with the object's sub-handle at +0x8, mode 0x10 and the alpha value.
void ShaderSetOverlayAlpha(u32 a0, f32 a1) {
    u8* obj = (u8*)(uintptr_t)a0;
    if (!obj) return;
    union { f32 f; u32 u; } alpha;
    alpha.f = a1;
    u32 type = *(u8*)(obj + 0xc);
    typedef void (*OverlayFn)(void* h, s32 mode, u32 alphaBits);
    OverlayFn fn = (OverlayFn)(uintptr_t)*(u32*)(lbl_80348F38 + type * 4 + 0x24);
    fn((void*)(uintptr_t)*(u32*)(obj + 0x8), 0x10, alpha.u);
}

// 0x801DB89C
// Starts the ripple frame effect (effect 9) with the three float parameters.
void FrameFxRipple(f32 a0, f32 a1, f32 a2) {
    f32 v[3] = {a0, a1, a2};
    fn_8016195C(lbl_8040E5F0, 9, 0, v);
}

// 0x801DB8DC
// Starts the flash frame effect (effect 3) with the three float parameters.
void FrameFxFlash(f32 a0, f32 a1, f32 a2) {
    f32 v[3] = {a0, a1, a2};
    fn_8016195C(lbl_8040E5F0, 3, 0, v);
}
}
