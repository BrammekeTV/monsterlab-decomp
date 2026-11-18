// More small functions in 0x8009xxxx.
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                   // 0x801D426C
void  MemFreeSmall(void* p);                                      // 0x801D4340
s32   RaySceneTest(void* ray);                                    // 0x80070458
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);             // 0x800690D4 (base ctor)
void  fn_80062CE0(u8* p, s32 del);                                // 0x80062CE0 (base dtor)
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8); // 0x8006FA44
u8*   fn_800C4530(u8* p, s32 a, void* params, s32 b);             // 0x800C4530 (ctor)
u8*   fn_800C456C(u8* p, s32 del);                                // 0x800C456C (deleting dtor)
u8*   fn_800C52B4(u8* p);                                         // 0x800C52B4 (ctor)
void  fn_800C20FC(u8* p);                                         // 0x800C20FC (sets vtable at +0xC)
u8*   fn_800C210C(u8* p, s32 del);                                // 0x800C210C (deleting dtor)
void  fn_800C2388(u8* p);                                         // 0x800C2388 (list cleanup)
u8*   fn_801F94EC(u8* p, s32 del);                                // 0x801F94EC (deleting dtor)
u32   fn_801EC880(u8* p, u32 hash);                               // 0x801EC880
void  fn_800988F0(u8* p, s32 b);                                  // 0x800988F0
void* fn_8009A47C(u8* p);                                         // 0x8009A47C
void  fn_8009E9C0(void* a, u8* p);                                // 0x8009E9C0
u8*   fn_801B3308(void);                                          // 0x801B3308 (global list)
void  fn_801B3378(u8* p, u8* key);                                // 0x801B3378 (list remove)
extern u8  lbl_803C4298[];                                        // 0x803C4298 (level info block)
extern f32 lbl_805FDBA0;                                          // 0x805FDBA0 (float)
extern f32 lbl_805FDBC8;                                          // 0x805FDBC8 (float)
extern f32 lbl_805FDC28;                                          // 0x805FDC28 (float)
extern f32 lbl_805FDC50;                                          // 0x805FDC50 (float, 0.0)
extern f32 lbl_805FDC58;                                          // 0x805FDC58 (float)
extern f32 lbl_805FDC5C;                                          // 0x805FDC5C (float)
extern f32 lbl_805FDC78;                                          // 0x805FDC78 (float)
extern f32 lbl_803462C8;                                          // 0x803462C8 (float)
extern f32 lbl_803C6B44[3];                                       // 0x803C6B44
extern u8  lbl_803C6B30[];                                        // 0x803C6B30
extern u8  lbl_803C6B60[];                                        // 0x803C6B60 (hash table: count +0xC, ptr +0x8)
extern f32 lbl_80346FC0[];                                        // 0x80346FC0 (float table)
extern u8  lbl_8035C2E8[];                                        // 0x8035C2E8 (vtable)
extern u8  lbl_8035C790[];                                        // 0x8035C790 (vtable)
extern u8  lbl_8035C928[];                                        // 0x8035C928 (vtable)
extern u8  lbl_8035CAC0[];                                        // 0x8035CAC0 (vtable)
extern u8  lbl_8035CB18[];                                        // 0x8035CB18 (vtable)
}

extern "C" {

// 0x800984FC
// Deleting destructor: base dtor (fn_80062CE0) on p, then frees p if del > 0.
u8* fn_800984FC(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80098568
// Constructor: base init (fn_800690D4), clears the state at +0x264..+0x26D, sets +0x26E,
// the two float timers at +0x270/+0x274 and +0x278, then the vtable.
void* fn_80098568(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = -1;
    *(u8*)(p + 0x26C) = 0;
    *(u8*)(p + 0x26D) = 0;
    *(u8*)(p + 0x26E) = 1;
    *(f32*)(p + 0x270) = lbl_805FDBA0;
    *(f32*)(p + 0x274) = lbl_805FDBA0;
    *(f32*)(p + 0x278) = lbl_805FDBC8;
    *(void**)(p + 0x0) = lbl_8035C2E8;
    return obj;
}

// 0x800985D8
// Deleting destructor: base dtor (fn_80062CE0) on p, then frees p if del > 0.
u8* fn_800985D8(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80098700
// Finds the record for hash 0x60D49509 (fn_801EC880 index, 200-byte records at +0x98) and
// returns its float at +0x60. The original also copies +0x5C and +0x64 to the stack, unused.
f32 fn_80098700(u8* p) {
    u32 idx = fn_801EC880(p + 0x94, 0x60D49509u);
    u8* rec = *(u8**)(p + 0x98) + idx * 200u;
    return *(f32*)(rec + 0x60);
}

// 0x80098C30
// Deleting destructor: on a non-null p, destroys the member at +0x268 (fn_801F94EC with -1),
// then the base dtor (fn_80062CE0), then frees p if del > 0.
u8* fn_80098C30(u8* p, s32 del) {
    if (p) {
        fn_801F94EC(p + 0x268, -1);
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80098D2C
// Removes this object's list node (key at +0x268) from the global list.
void fn_80098D2C(u8* p) {
    fn_801B3378(fn_801B3308(), p + 0x268);
}

// 0x80098DA8
// Constructor: base init (fn_800690D4), clears the state fields, sets the vtable, then picks
// a float from lbl_80346FC0 by the index RaySceneTest returns (stored at +0x298).
void* fn_80098DA8(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(void**)(p + 0x0) = lbl_8035C790;
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = 0;
    *(s32*)(p + 0x26C) = 0;
    *(f32*)(p + 0x270) = lbl_805FDC28;
    s32 idx = RaySceneTest(lbl_8035C790);
    *(f32*)(p + 0x298) = lbl_80346FC0[idx];
    return obj;
}

// 0x80098E14
// Deleting destructor: base dtor (fn_80062CE0) on p, then frees p if del > 0.
u8* fn_80098E14(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80099874
// Sets the attached particle effect at +0x26C to b. If it changes, the old effect is removed
// first; a new non-null effect is passed to fn_800988F0.
void fn_80099874(u8* p, u8* b) {
    u8* old = *(u8**)(p + 0x26C);
    if (b == old) return;
    if (old != NULL) RemoveAttachedParticleEffectAtEmitterImmediate(old, 0x60D49509u);
    *(u8**)(p + 0x26C) = b;
    if (b != NULL) fn_800988F0(b, 0);
}

// 0x8009A304
// Calls fn_8009A47C on p, then fn_8009E9C0 with that result and p, then the virtual
// slot 0x10 of the object at +4 of the level info block.
void fn_8009A304(u8* p) {
    void* r = fn_8009A47C(p);
    fn_8009E9C0(r, p);
    u8* obj = *(u8**)(lbl_803C4298 + 4);
    void (*vfn)(u8*) = *(void (**)(u8*))(*(u8**)obj + 0x10);
    vfn(obj);
}

// 0x8009ABC8
// Adds dt to the float at +0x90; when it reaches lbl_805FDC78, signals the game flow.
void fn_8009ABC8(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x90) + dt;
    *(f32*)(p + 0x90) = t;
    if (!(t >= lbl_805FDC78)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x8009AC0C
// Same as fn_8009ABC8 with threshold lbl_805FDC5C.
void fn_8009AC0C(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x90) + dt;
    *(f32*)(p + 0x90) = t;
    if (!(t >= lbl_805FDC5C)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x8009B138
// Resets the 3-float table lbl_803C6B44 to {0, lbl_805FDC58, 0}, then calls fn_8006FA44 on
// lbl_803C6B30 with a mode of 3 if lbl_803462C8 is positive, otherwise 1.
void fn_8009B138(void) {
    s32 mode = (lbl_803462C8 > lbl_805FDC50) ? 3 : 1;
    lbl_803C6B44[0] = lbl_805FDC50;
    lbl_803C6B44[1] = lbl_805FDC58;
    lbl_803C6B44[2] = lbl_805FDC50;
    fn_8006FA44(lbl_803C6B30, (s32)0xA77059ACu, mode, (s32)0x011581D3u, (s32)0x85AB5092u, 0);
}

// 0x8009B19C
// Constructor: base init (fn_800C4530), then sets the vtable.
void* fn_8009B19C(void* obj, s32 a, void* params, s32 b) {
    fn_800C4530((u8*)obj, a, params, b);
    *(void**)((u8*)obj + 0x0) = lbl_8035C928;
    return obj;
}

// 0x8009B1D8
// Deleting destructor: base dtor (fn_800C456C) on p, then frees p if del > 0.
u8* fn_8009B1D8(u8* p, s32 del) {
    if (p) {
        fn_800C456C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8009B264
// Constructor: base init (fn_800C20FC), clears +0x10 and +0x14, sets the vtable at +0xC.
u8* fn_8009B264(u8* p) {
    fn_800C20FC(p);
    *(void**)(p + 0x10) = NULL;
    *(void**)(p + 0xC) = lbl_8035CAC0;
    *(s32*)(p + 0x14) = 0;
    return p;
}

// 0x8009B2AC
// Deleting destructor: sets the vtable at +0xC, calls its slot 0x10 on p, then the base
// cleanup (fn_800C210C), then frees p if del > 0.
u8* fn_8009B2AC(u8* p, s32 del) {
    if (p) {
        *(void**)(p + 0xC) = lbl_8035CAC0;
        void (*vfn)(u8*) = *(void (**)(u8*))(lbl_8035CAC0 + 0x10);
        vfn(p);
        fn_800C210C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8009B654
// Frees the buffer at +0x10 (if any) after the base cleanup fn_800C2388, then clears it
// together with the count at +0x14.
void fn_8009B654(u8* p) {
    fn_800C2388(p);
    void* buf = *(void**)(p + 0x10);
    if (buf != NULL) {
        MemFreeSmall(buf);
        *(void**)(p + 0x10) = NULL;
        *(s32*)(p + 0x14) = 0;
    }
}

// 0x8009BB8C
// Returns 1 if the hash key (r4) is in the table at lbl_803C6B60 (count +0xC, list +0x8), else 0.
s32 fn_8009BB8C(s32 unused, u32 key) {
    (void)unused;
    s32 count = *(s32*)(lbl_803C6B60 + 0xC);
    u32* ids = *(u32**)(lbl_803C6B60 + 0x8);
    for (s32 i = 0; i < count; i++) {
        if (key == ids[i]) return 1;
    }
    return 0;
}

// 0x8009BE14
// Constructor: base init (fn_800C52B4), then sets the vtable at +0xD4 and the fields at
// +0xD0, +0xDC and +0xE0.
u8* fn_8009BE14(u8* p) {
    fn_800C52B4(p);
    *(void**)(p + 0xD4) = lbl_8035CB18;
    *(s32*)(p + 0xDC) = 4;
    *(s32*)(p + 0xE0) = 0;
    *(s32*)(p + 0xD0) = 3;
    return p;
}

}  // extern "C"
