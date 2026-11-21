// Camera object: reset to the world's current zone, and a virtual-call thunk.
// Original: 0x800DBEF0, 0x800DE524 (auto_03_800DB9A4_text.s)
#include "types.h"

extern "C" {
f32 fn_800DE2AC(u8* zone);         // 0x800DE2AC
extern u8 gGame[];                 // 0x803B53A0
extern u8 gWorldState[];           // 0x803E6D00 (first word: world block pointer)
}

// 0x800DBEF0
// Slot 0x8C of the camera vtable. Clears the blend state (0x20-0x40; 0x3C = 0.2, 0x40 = 0.3,
// 0x34 = 0x10, 0x38 = 0x20), resets the sub-object at +0x68 (its slot 0x24, with a zero vector
// and the identity quaternion; r6 = 0x10 and r7 = 0 are live at the call and passed on), picks up
// the world's current zone block (0x2C0) with its position and the value from fn_800DE2AC as in
// the constructor (fn_800DB9A4), copies them to the target values (0x50-0x60), sets the flag at
// 0x18 and calls its own slot 0x98 with 0.0.
extern "C" void fn_800DBEF0(u8* p) {
    f32 quat[4];
    f32 vec[3];
    *(f32*)(p + 0x3C) = 0.0f;
    *(f32*)(p + 0x40) = 0.0f;
    *(u8*)(p + 0x21)  = 0;
    *(u8*)(p + 0x20)  = 0;
    *(f32*)(p + 0x28) = 0.0f;
    *(f32*)(p + 0x24) = 0.0f;
    *(u8*)(p + 0x2C)  = 0;
    *(f32*)(p + 0x30) = 0.0f;
    *(f32*)(p + 0x40) = 0.3f;
    *(f32*)(p + 0x3C) = 0.2f;
    *(s32*)(p + 0x34) = 0x10;
    *(s32*)(p + 0x38) = 0x20;
    quat[2] = 0.0f; quat[1] = 0.0f; quat[0] = 0.0f; quat[3] = 1.0f;
    vec[2] = 0.0f; vec[1] = 0.0f; vec[0] = 0.0f;
    u8* sub = p + 0x68;
    typedef void (*ResetFn)(u8*, f32*, f32*, s32, s32);
    (*(ResetFn*)(*(u8**)sub + 0x24))(sub, vec, quat, 0x10, 0);
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x8);
    *(u8**)(p + 0x2C0) = zone;
    if (zone != NULL) {
        u8* v = *(u8**)(zone + 0x4);
        *(f32*)(p + 0x44) = *(f32*)(v + 0x0);
        *(f32*)(p + 0x48) = *(f32*)(v + 0x4);
        *(f32*)(p + 0x4C) = *(f32*)(v + 0x8);
        if (*(u8*)(*(u8**)(*(u8**)(gGame + 0xD614) + 0x4) + 0x34) == 4) {
            *(f32*)(p + 0x5C) = fn_800DE2AC(zone);
        } else {
            *(f32*)(p + 0x5C) = 0.0f;
        }
    } else {
        *(f32*)(p + 0x44) = 0.0f;
        *(f32*)(p + 0x48) = 0.0f;
        *(f32*)(p + 0x4C) = 0.0f;
        *(f32*)(p + 0x5C) = 0.0f;
    }
    *(f32*)(p + 0x50) = *(f32*)(p + 0x44);
    *(f32*)(p + 0x54) = *(f32*)(p + 0x48);
    *(f32*)(p + 0x58) = *(f32*)(p + 0x4C);
    *(f32*)(p + 0x60) = *(f32*)(p + 0x5C);
    *(u8*)(p + 0x18) = 1;
    typedef void (*Fn98)(u8*, f32);
    (*(Fn98*)(*(u8**)p + 0x98))(p, 0.0f);
    *(s32*)(p + 0x2BC) = 0;
    *(s32*)(p + 0x1F8) = 0;
}

// 0x800DE524
// Slot 0x94 of the camera vtable: jumps to slot 0x8C of the same object (`bctr`, a tail call, so
// the original passes every argument register through). The callers decompiled so far pass the
// object only (`CamCall(cam, 0x94)` in session_update.cpp).
extern "C" void fn_800DE524(void* self) {
    typedef void (*Fn)(void*);
    Fn f = *(Fn*)(*(u8**)self + 0x8C);
    f(self);
}
