// Small functions in 0x800Axxxx.
#include "types.h"
#include "game/script_callbacks.h"

// Virtual slot 0xC on an object that returns a pointer.
typedef u8* (*VFnRetPtr)(u8* self);

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
s32   RaySceneTest(void* ray);                                   // 0x80070458
void  PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);    // 0x80209E70
u8*   fn_8006FBDC(void);                                         // 0x8006FBDC
u8*   fn_80062CE0(u8* p, s32 del);                               // 0x80062CE0 (deleting dtor)
u8*   fn_800C40C0(u8* p, s32 del);                               // 0x800C40C0 (deleting dtor)
u8*   fn_800C407C(u8* p, s32 a, void* params, s32 b);            // 0x800C407C (base ctor)
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);            // 0x800690D4 (base ctor)
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
void* fn_80209814(void* v);                                      // 0x80209814
void  fn_800A71C8(u8* p, u32 a);                                 // 0x800A71C8
void  fn_800A75C8(u8* p);                                        // 0x800A75C8

extern f32 lbl_80346B58[];          // 0x80346B58 (f32 table indexed by RaySceneTest)
extern f32 lbl_80346B38[];          // 0x80346B38 (f32 table indexed by RaySceneTest)
extern f32 lbl_80346B48[];          // 0x80346B48 (f32 table indexed by RaySceneTest)
extern f32 lbl_805FDE74;            // 0x805FDE74 (upper clamp)
extern f32 lbl_805FDE8C;            // 0x805FDE8C (threshold)
extern f32 lbl_805FDEB0;            // 0x805FDEB0 (default float)
extern u8  lbl_8035D120[];          // 0x8035D120 (vtable)
extern u8  lbl_8035D280[];          // 0x8035D280 (vtable)
extern u8  lbl_8035D7C8[];          // 0x8035D7C8 (vtable)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)

// 0x800A1758
// Adds the f32 from lbl_80346B58 indexed by RaySceneTest(self) to the value at +0xCF4.
void fn_800A1758(u8* p) {
    s32 idx = RaySceneTest(p);
    *(f32*)(p + 0xcf4) += lbl_80346B58[idx];
}

// 0x800A17A0
// Adds lbl_80346B38[RaySceneTest(self)] to the value at +0xCF4, clamped to at most lbl_805FDE74.
void fn_800A17A0(u8* p) {
    s32 idx = RaySceneTest(p);
    f32 v = *(f32*)(p + 0xcf4) + lbl_80346B38[idx];
    if (v > lbl_805FDE74) v = lbl_805FDE74;
    *(f32*)(p + 0xcf4) = v;
}

// 0x800A17F8
// Adds lbl_80346B48[RaySceneTest(self)] to the value at +0xCF4, clamped to at most lbl_805FDE74.
void fn_800A17F8(u8* p) {
    s32 idx = RaySceneTest(p);
    f32 v = *(f32*)(p + 0xcf4) + lbl_80346B48[idx];
    if (v > lbl_805FDE74) v = lbl_805FDE74;
    *(f32*)(p + 0xcf4) = v;
}

// 0x800A1BEC
// Passes the object at +0xC8C to fn_800A71C8 with the constant 0x437B3D65, then to fn_800A75C8.
void fn_800A1BEC(u8* p) {
    fn_800A71C8(*(u8**)(p + 0xc8c), 0x437B3D65u);
    fn_800A75C8(*(u8**)(p + 0xc8c));
}

// 0x800A22F4
// Adds delta to the value at +0xD00; when the total reaches lbl_805FDE8C or more, sends
// game-flow signal 0x88C6E941.
void fn_800A22F4(u8* p, f32 delta) {
    f32 v = *(f32*)(p + 0xd00) + delta;
    *(f32*)(p + 0xd00) = v;
    if (!(v >= lbl_805FDE8C)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x800A3128
// Binary search tree lookup for the u32 key at *keyp. The header holds the root at +4; nodes
// have left at +0, right at +4 and a u32 key at +0xC. Returns the last node visited (p+4 when
// the tree is empty). *outNode receives the last node the key was not below (0 if none);
// *outLeft is 1 while the last step went left; *outAll stays 1 only while every step went left.
u8* fn_800A3128(u8* p, u32* keyp, void** outNode, u8* outLeft, u8* outAll) {
    *outNode = NULL;
    u8* last = p + 4;
    u8* cur = *(u8**)(p + 4);
    *outLeft = 1;
    *outAll = 1;
    while (cur != NULL) {
        u32 key = *keyp;
        last = cur;
        if (key < *(u32*)(cur + 0xc)) {
            cur = *(u8**)(cur + 0x0);
            *outLeft = 1;
        } else {
            *outNode = cur;
            cur = *(u8**)(cur + 0x4);
            *outLeft = 0;
            *outAll = 0;
        }
    }
    return last;
}

// 0x800A34C0
// Constructor: base ctor fn_800C407C (the remaining arguments pass through), clears the
// s32 at +0x2A4 and sets the vtable lbl_8035D120.
void* fn_800A34C0(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800C407C(p, a, params, b);
    *(s32*)(p + 0x2a4) = 0;
    *(void**)p = lbl_8035D120;
    return p;
}

// 0x800A3504
// Deleting destructor: base dtor fn_800C40C0(p, 0) when p is non-null, then frees p when del > 0.
u8* fn_800A3504(u8* p, s32 del) {
    if (p) {
        fn_800C40C0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800A38F4
// Constructor: base ctor fn_800690D4 (the remaining arguments pass through), clears the byte
// at +0x364 and the s32s at +0x264, +0x268, +0x26C, and sets the vtable lbl_8035D280.
void* fn_800A38F4(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8*)(p + 0x364) = 0;
    *(void**)p = lbl_8035D280;
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = 0;
    *(s32*)(p + 0x26c) = 0;
    return p;
}

// 0x800A3944
// Deleting destructor: base dtor fn_80062CE0(p, 0) when p is non-null, then frees p when del > 0.
u8* fn_800A3944(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800A5AEC
// Same binary search tree lookup as fn_800A3128 (unsigned key compare).
u8* fn_800A5AEC(u8* p, u32* keyp, void** outNode, u8* outLeft, u8* outAll) {
    *outNode = NULL;
    u8* last = p + 4;
    u8* cur = *(u8**)(p + 4);
    *outLeft = 1;
    *outAll = 1;
    while (cur != NULL) {
        u32 key = *keyp;
        last = cur;
        if (key < *(u32*)(cur + 0xc)) {
            cur = *(u8**)(cur + 0x0);
            *outLeft = 1;
        } else {
            *outNode = cur;
            cur = *(u8**)(cur + 0x4);
            *outLeft = 0;
            *outAll = 0;
        }
    }
    return last;
}

// 0x800A5B4C
// Binary search tree lookup like fn_800A3128, but the key compare is signed.
u8* fn_800A5B4C(u8* p, s32* keyp, void** outNode, u8* outLeft, u8* outAll) {
    *outNode = NULL;
    u8* last = p + 4;
    u8* cur = *(u8**)(p + 4);
    *outLeft = 1;
    *outAll = 1;
    while (cur != NULL) {
        s32 key = *keyp;
        last = cur;
        if (key < *(s32*)(cur + 0xc)) {
            cur = *(u8**)(cur + 0x0);
            *outLeft = 1;
        } else {
            *outNode = cur;
            cur = *(u8**)(cur + 0x4);
            *outLeft = 0;
            *outAll = 0;
        }
    }
    return last;
}

// 0x800A5BCC
// Deleting destructor: base dtor fn_80062CE0(p, 0) when p is non-null, then frees p when del > 0.
u8* fn_800A5BCC(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800A5CB4
// Clears the flag byte at +0x264. If the object's virtual slot 0xC returns non-null, that
// object's slot 0xC call result gets bit 1 cleared at +8. Then bit 1 is cleared at +0x20 of
// the sub-object at +4.
void fn_800A5CB4(u8* p) {
    *(u8*)(p + 0x264) = 0;
    u8* sub = (*(VFnRetPtr*)(*(u8**)p + 0xc))(p);
    if (sub != NULL) {
        u8* sub2 = (*(VFnRetPtr*)(*(u8**)p + 0xc))(p);
        *(u32*)(sub2 + 0x8) &= ~0x2u;
    }
    u8* child = *(u8**)(p + 0x4);
    *(u32*)(child + 0x20) &= ~0x2u;
}

// 0x800A5F8C
// If the flag byte at +0x266 is set, clears it, makes sure the voice SFX handle lbl_805FBBAC
// exists, plays sound 0x9189B7A9 and runs fn_800A1758 on the global object.
void fn_800A5F8C(u8* p) {
    if (*(u8*)(p + 0x266) == 0) return;
    *(u8*)(p + 0x266) = 0;
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(0x9189B7A9u, 0, 0, 0, 0, 0);
    fn_800A1758(fn_8006FBDC());
}

// 0x800A63B4
// Clears the flag byte at +0x265, sets the prop state of this object (state 1), then runs
// fn_800A17F8 on the global object.
void fn_800A63B4(u8* p) {
    *(u8*)(p + 0x265) = 0;
    SetPropState((u32)(uintptr_t)p, 1, 0);
    fn_800A17F8(fn_8006FBDC());
}

// 0x800A63EC
// Deleting destructor: base dtor fn_80062CE0(p, 0) when p is non-null, then frees p when del > 0.
u8* fn_800A63EC(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800A6450
// Deleting destructor: base dtor fn_80062CE0(p, 0) when p is non-null, then frees p when del > 0.
u8* fn_800A6450(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800A64B4
// Deleting destructor: base dtor fn_80062CE0(p, 0) when p is non-null, then frees p when del > 0.
u8* fn_800A64B4(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800A6538
// Constructor: base ctor fn_800690D4 (the remaining arguments pass through), then sets the
// vtable lbl_8035D7C8, four floats at +0x264..+0x270 to lbl_805FDEB0, the byte at +0x274 to 0,
// s32 3 at +0x284 and +0x288, and zeroes the s32s at +0x29C..+0x2A8.
void* fn_800A6538(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    f32 v = lbl_805FDEB0;
    *(f32*)(p + 0x264) = v;
    *(void**)p = lbl_8035D7C8;
    *(f32*)(p + 0x268) = v;
    *(f32*)(p + 0x26c) = v;
    *(f32*)(p + 0x270) = v;
    *(u8*)(p + 0x274) = 0;
    *(s32*)(p + 0x284) = 3;
    *(s32*)(p + 0x288) = 3;
    *(s32*)(p + 0x29c) = 0;
    *(s32*)(p + 0x2a0) = 0;
    *(s32*)(p + 0x2a4) = 0;
    *(s32*)(p + 0x2a8) = 0;
    return p;
}
}
