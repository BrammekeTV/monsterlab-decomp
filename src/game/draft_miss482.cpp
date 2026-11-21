// Small functions from the miss482 batch (0x800A..0x801F, mixed areas).
#include "types.h"

// Script callbacks (SetPropState, UpdateCombatHUD) come from the shared header with C linkage.
extern "C" {
#include "game/script_callbacks.h"
}

// Globals defined in C++ files use C++ linkage (see small_8019.cpp, ai_states.cpp).
extern "C++" void* gEventMgr;       // 0x805FB6C8
extern "C++" void* gScreenSets;     // 0x805FB654
extern void* lbl_805FB658;          // 0x805FB658 (UI root object)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern s32 lbl_805F8538;            // 0x805F8538 (combat HUD mode)
extern u8 lbl_803C4298[];           // 0x803C4298 (level info block)
extern u8 lbl_803C5240[];           // 0x803C5240
extern f32 lbl_803C5318[];          // 0x803C5318 (3 floats)
extern u8 lbl_8035E2B8[];           // 0x8035E2B8 (vtable)
extern u8 lbl_80358218[];           // 0x80358218 (vtable)
extern f32 lbl_80346BD0[];          // 0x80346BD0 (per-ray-result float table)
extern f32 lbl_805FD1C0;            // 0x805FD1C0 (float constant)
extern f32 lbl_805FD1C4;            // 0x805FD1C4 (float constant)
extern f32 lbl_805FE0B0;            // 0x805FE0B0 (lower clamp)
extern f32 lbl_805FE0B8;            // 0x805FE0B8 (table divisor)
extern f32 lbl_805FE0BC;            // 0x805FE0BC (upper clamp)
extern f32 lbl_805FDD08;            // 0x805FDD08 (float constant)
extern f32 lbl_805FF830;            // 0x805FF830 (default result)
extern f32 lbl_805FF930;            // 0x805FF930 (float constant)
extern f32 lbl_806000D8;            // 0x806000D8 (zero)
extern f32 lbl_806000F4;            // 0x806000F4 (epsilon)
extern f32 lbl_806000F8;            // 0x806000F8 (angle, returned for x > 0)
extern f32 lbl_806000FC;            // 0x806000FC (angle, returned for x <= 0)
extern f32 lbl_80600100;            // 0x80600100 (half turn, added for x > 0, y <= 0)
extern f32 lbl_80600104;            // 0x80600104 (negative half turn, added for x <= 0, y < 0)

// Virtual-call signatures used below (called through a vtable slot read from the object).
typedef void (*VFnObj)(u8*);
typedef void (*VFnObjF32)(u8*, f32);
typedef f32 (*VFnObjRetF32)(u8*);
typedef u8* (*VFnObjRetPtr)(u8*);

// C++ linkage: defined in small_8003c.cpp without extern "C".
void* fn_8003CDE0(void* p, s32 del);                                   // 0x8003CDE0

extern "C" {
void  operator_delete(void* p);                                        // 0x801D426C
void  ChoiceInit(u8* c);                                               // defined in battle_core.cpp
void* ChoiceDestroy(void* c, s32 del);                                 // 0x8020F394
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle); // 0x8032EFC8
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count); // 0x8032F2E0
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);          // 0x80209E70
s32   RaySceneTest(void* ray);                                         // 0x80070458
u32   RandomU32(void);                                                 // 0x80143460
void* ScreenSetsCurrent(void* sets);                                   // 0x8018D1E4
f64   fn_8033E8EC(f64 x);                                              // atan
void  fn_8006AADC(u8* p, f32 dt);                                      // 0x8006AADC
void  fn_8006FAF4(u8* p, s32 a);                                       // 0x8006FAF4
u8*   fn_8006FBDC(void);                                               // 0x8006FBDC
void  fn_800A17A0(u8* p);                                              // 0x800A17A0
void  fn_800A7D88(u8* obj, u32 hash);                                  // 0x800A7D88
void  fn_800B1D98(u8* p);                                              // 0x800B1D98
u8*   fn_800B1AD0(u8* p, s32 a4, s32 a5);                              // 0x800B1AD0
void  fn_800B3180(u8* p);                                              // 0x800B3180
u8*   fn_800B59A8(u8* p, s32 del);                                     // 0x800B59A8
u8*   fn_800216B8(u8* p, s32 del);                                     // 0x800216B8
u8*   fn_801B3308(void);                                               // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                     // 0x801B3378
u8*   fn_801F6238(u8* p, s32 del);                                     // 0x801F6238
u8*   fn_801F81E0(u8* p, s32 del);                                     // 0x801F81E0
u8*   fn_801F9598(u8* p, s32 del);                                     // 0x801F9598
u8*   fn_801FA048(u8* p, s32 del);                                     // 0x801FA048
void* fn_801D40E0(s32 n);                                              // 0x801D40E0
void* fn_80209814(void* v);                                            // 0x80209814
void  fn_8016A3F8(u8* p);                                              // 0x8016A3F8
void  fn_8016A43C(u8* p);                                              // 0x8016A43C
void  fn_8016A788(u8* p);                                              // 0x8016A788
void  fn_8019B4E8(u8* p);                                              // 0x8019B4E8
s32 fn_8019B684(u8* p);                                              // 0x8019B684
u32   fn_8019552C(u8* list, s32 index);                                // 0x8019552C (returns a slot pointer; declared u32 in small_8019c.cpp)
s32   fn_80196668(u8* p);                                              // 0x80196668
u8*   fn_80198E20(u8* set);                                         // 0x80198E20 (returns a pointer; declared s32 in its leaf file)
void  fn_8019C174(u8* obj, s32 (*cbA)(void*, f32*), s32 (*cbB)(void), f32* t); // 0x8019C174
s32   fn_80199C74(void* obj, f32* t);                                  // 0x80199C74
s32   fn_80199DA4(void);                                               // 0x80199DA4
void  fn_80040F44(u8* p);                                              // 0x80040F44
void  fn_800410AC(u8* p, f32 dt);                                      // 0x800410AC
void  fn_800412DC(u8* p, f32 dt);                                      // 0x800412DC
BOOL  fn_80147554(s32 button);                                         // 0x80147554
}

extern "C" {

// 0x800ACEEC
// Walks the list at +0xB0 and sets bit 0 of each node's inner flag word, calls the base update
// at 0x8006AADC, runs the virtual update at vtable slot 0x74, then clears bit 0 again.
void fn_800ACEEC(u8* arg0, f32 farg0) {
    u8* n = *(u8**)(arg0 + 0xB0);
    while (n != NULL) {
        u8* c = *(u8**)(n + 0x4);
        if (c != NULL) {
            u8* d = *(u8**)(c + 0x4);
            *(s32*)(d + 0x20) |= 1;
        }
        n = *(u8**)(n + 0x8);
    }
    fn_8006AADC(arg0, farg0);
    (*(VFnObjF32*)(*(u8**)arg0 + 0x74))(arg0, farg0);
    n = *(u8**)(arg0 + 0xB0);
    while (n != NULL) {
        u8* c = *(u8**)(n + 0x4);
        if (c != NULL) {
            u8* d = *(u8**)(c + 0x4);
            *(s32*)(d + 0x20) &= ~1;
        }
        n = *(u8**)(n + 0x8);
    }
}

// 0x801A4824
// For each of the (w * h) slots of the grid at +0x1BC, calls the function at +0xC of each node
// of the object's child list (+0x120, linked by +0x124).
s32 fn_801A4824(u8* arg0) {
    s32 i;
    s32 count;

    fn_8019B4E8(arg0);
    count = *(s32*)(arg0 + 0x13C) * *(s32*)(arg0 + 0x140);
    for (i = 0; i < count; i++) {
        u8** slot = (u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC), i);
        u8* obj = (slot != NULL) ? *slot : NULL;
        if (obj != NULL) {
            u8* n = *(u8**)(obj + 0x120);
            while (n != NULL) {
                u8* sub = *(u8**)(n + 0x138);
                (*(VFnObj*)(sub + 0xC))(n);
                n = *(u8**)(n + 0x124);
            }
        }
    }
    return 0;
}

// 0x801A48DC
// Same walk as fn_801A4824, but calls the function at +0x10 and uses fn_8019B684 as the prologue.
s32 fn_801A48DC(u8* arg0) {
    s32 i;
    s32 count;

    fn_8019B684(arg0);
    count = *(s32*)(arg0 + 0x13C) * *(s32*)(arg0 + 0x140);
    for (i = 0; i < count; i++) {
        u8** slot = (u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC), i);
        u8* obj = (slot != NULL) ? *slot : NULL;
        if (obj != NULL) {
            u8* n = *(u8**)(obj + 0x120);
            while (n != NULL) {
                u8* sub = *(u8**)(n + 0x138);
                (*(VFnObj*)(sub + 0x10))(n);
                n = *(u8**)(n + 0x124);
            }
        }
    }
    return 0;
}

// 0x80049574
// Initialises the three-float block at lbl_803C5318 and the choice and float records in lbl_803C5240,
// and registers the choice destructor for shutdown.
void fn_80049574(void) {
    u8* base = lbl_803C5240;
    f32 c0 = lbl_805FD1C0;
    f32 c4 = lbl_805FD1C4;

    lbl_803C5318[0] = c0;
    lbl_803C5318[1] = c4;
    lbl_803C5318[2] = c0;

    ChoiceInit(base + 0x5C);
    __register_global_object(base + 0x5C, (void (*)(void*))ChoiceDestroy, base + 0x50);

    *(f32*)(base + 0x68) = c0;
    *(f32*)(base + 0x6C) = c0;
    *(f32*)(base + 0x70) = c0;
    *(f32*)(base + 0x74) = c0;
    *(f32*)(base + 0x78) = c0;
    *(f32*)(base + 0x7C) = c0;
    *(f32*)(base + 0x80) = c0;
    *(f32*)(base + 0x84) = c0;
    *(f32*)(base + 0x88) = c0;
    *(f32*)(base + 0x8C) = c4;
    *(f32*)(base + 0x90) = c0;
    *(f32*)(base + 0x94) = c0;
    *(f32*)(base + 0x98) = c0;
    *(f32*)(base + 0x9C) = c4;
}

// 0x8009C134
// Counts the wrapped range [+0x70 .. +0x6C] (both must be >= 0), and if it exceeds +0xD0, returns
// whether +0xD8 is greater than the result of the object's vtable slot 0xD0 plus lbl_805FDD08.
s32 fn_8009C134(u8* arg0, u8* arg1) {
    s32 t6 = *(s32*)(arg0 + 0x6C);
    s32 count;

    if (t6 >= 0 && *(s32*)(arg0 + 0x70) >= 0) {
        s32 v = *(s32*)(arg0 + 0x70);
        if (v < t6) {
            v += *(s32*)(arg0 + 0x74);
        }
        count = (v - t6) + 1;
    } else {
        count = 0;
    }
    if (count > *(s32*)(arg0 + 0xD0)) {
        f32 ret = (*(VFnObjRetF32*)(*(u8**)arg1 + 0xD0))(arg1);
        return *(f32*)(arg0 + 0xD8) > (lbl_805FDD08 + ret);
    }
    return 0;
}

// 0x800A5ED8
// Sets the two active flag bytes, marks the child objects, sets bit 0x600000 in +0x15C,
// sets a random prop state and marks flag +0x266.
void fn_800A5ED8(u8* arg0) {
    *(u8*)(arg0 + 0x264) = 1;
    *(u8*)(arg0 + 0x265) = 1;
    if ((*(VFnObjRetPtr*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* t = (*(VFnObjRetPtr*)(*(u8**)arg0 + 0xC))(arg0);
        *(s32*)(t + 0x8) |= 2;
    }
    {
        u8* t2 = *(u8**)(arg0 + 0x4);
        *(s32*)(t2 + 0x20) |= 2;
    }
    *(s32*)(arg0 + 0x15C) |= 0x600000;
    SetPropState(arg0, (u32)RandomU32() % (u32)*(s32*)(arg0 + 0x1A4), 0);
    *(u8*)(arg0 + 0x266) = 1;
}

// 0x800B268C
// Deleting destructor: sets the vtable, runs the base and member destructors, frees if del > 0.
u8* fn_800B268C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_8035E2B8;
        fn_800B3180(arg0);
        fn_800B59A8(arg0 + 0x620, -1);
        fn_801FA048(arg0 + 0x514, -1);
        fn_801F9598(arg0 + 0x460, -1);
        fn_801FA048(arg0 + 0x360, -1);
        fn_801F9598(arg0 + 0x2AC, -1);
        fn_801F6238(arg0 + 0x1F0, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8018BF60
// Wraps the animation time at +0x24 of the player held by *arg0 into [0, length), resets the
// frame counter, and runs the update steps (the first one only if the time moved backwards).
void fn_8018BF60(u8** arg0, f32 farg0) {
    u8* p = *arg0;
    u8* r31;
    u8* q;
    f32 var;
    f32 old;

    if (p == NULL) {
        return;
    }
    r31 = *(u8**)(p + 0x14);
    if (*(u8**)(r31 + 0x14) == NULL) {
        return;
    }
    old = *(f32*)(r31 + 0x24);
    var = farg0;
    q = *(u8**)(r31 + 0x14);
    while (var < lbl_805FF830) {
        var += *(f32*)(q + 0x20);
    }
    while (var > *(f32*)(q + 0x20)) {
        var -= *(f32*)(q + 0x20);
    }
    *(f32*)(r31 + 0x24) = var;
    *(s32*)(r31 + 0x1C) = 0;
    if (var < old) {
        fn_8016A3F8(r31);
    }
    fn_8016A43C(r31);
    fn_8016A788(r31);
}

// 0x80199684
// Calls fn_8019C174 with the object and the time callbacks; the time value is farg0 when the
// screen set is inactive or the object is the current one, otherwise lbl_805FF930.
s32 fn_80199684(u8* arg0, f32 farg0) {
    f32 sp8;
    f32 spC;

    sp8 = farg0;
    fn_80196668((u8*)lbl_805FB658);
    if ((*(u32*)(arg0 + 0x64) & 1) != 0) {
        if (*(u8*)((u8*)gScreenSets + 0x94) == 0 ||
            arg0 == (u8*)(uintptr_t)fn_80198E20((u8*)ScreenSetsCurrent(gScreenSets)) ||
            *(u8*)(arg0 + 0x150) != 0) {
            fn_8019C174(arg0, fn_80199C74, fn_80199DA4, &sp8);
        } else {
            spC = lbl_805FF930;
            fn_8019C174(arg0, fn_80199C74, fn_80199DA4, &spC);
        }
    }
    return 0;
}

// 0x801F60F8
// atan2-style angle of (farg0, farg1): returns a quarter turn when farg1 is about zero, otherwise
// atan(farg0 / farg1) corrected into the right quadrant.
f32 fn_801F60F8(f32 farg0, f32 farg1) {
    f32 var;

    if ((f32)fabs(farg1) < lbl_806000F4) {
        if (farg0 > lbl_806000D8) {
            return lbl_806000F8;
        }
        return lbl_806000FC;
    }
    var = (f32)fn_8033E8EC((f64)(farg0 / farg1));
    if (farg0 > lbl_806000D8) {
        if (farg1 > lbl_806000D8) {
            return var;
        }
        return lbl_80600100 + var;
    }
    if (farg1 < lbl_806000D8) {
        return lbl_80600104 + var;
    }
    return var;
}

// 0x80039F7C
// Deleting destructor: sets the vtable, destroys the two sub-objects and the 7-entry array at +0x90,
// runs the base destructor, and frees the object when del > 0.
u8* fn_80039F7C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_80358218;
        fn_801B3378(fn_801B3308(), arg0 + 0x868);
        fn_801B3378(fn_801B3308(), arg0 + 0x3E4);
        fn_801F6238(arg0 + 0x868, -1);
        fn_801F81E0(arg0 + 0x3E4, -1);
        __destroy_arr(arg0 + 0x90, (void (*)(void*, s32))fn_8003CDE0, 0x34, 7);
        fn_800216B8(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80040E4C
// Per-frame HUD update: fires fn_80040F44 for the special level states, then calls the HUD
// update for the current HUD mode (8 or 7), then the common update.
void fn_80040E4C(u8* arg0, f32 farg0) {
    u8* g = lbl_803C4298;

    if ((*(u8*)(g + 0x1C) == 0x1D || *(u8*)(g + 0x1C) == 0x36) &&
        fn_80147554(*(s32*)(g + 0x38) * 0x10 + 0x108D) != 0) {
        fn_80040F44(arg0);
    }
    if ((u32)lbl_805F8538 == 8U) {
        UpdateCombatHUD(farg0);
    } else if ((u32)lbl_805F8538 == 7U) {
        fn_800412DC(arg0, farg0);
    }
    fn_800410AC(arg0, farg0);
}

// 0x800A6304
// Clears flag +0x265, sets prop state 2, runs the two update calls, sends the event 0xA768564F to
// the event manager, creates the voice SFX handle once, and plays sound 0x9904AB9B.
void fn_800A6304(u8* arg0) {
    void* var;

    *(u8*)(arg0 + 0x265) = 0;
    SetPropState(arg0, 2U, 0);
    fn_800A17A0(fn_8006FBDC());
    EventSend(gEventMgr, arg0, NULL, 0xA768564FU, 0, 0, 1, 0);
    if (lbl_805FBBAC == NULL) {
        var = fn_801D40E0(1);
        if (var != NULL) {
            var = fn_80209814(var);
        }
        lbl_805FBBAC = var;
    }
    PlaySound(0x9904AB9BU, 0, 0, 0, 0, 0);
}

// 0x800B1A20
// Adds the table value for the current ray result (lbl_80346BD0 / lbl_805FE0B8) to the float at
// +0x268, clamps it to [lbl_805FE0B0, lbl_805FE0BC], then runs the two follow-up calls.
s32 fn_800B1A20(u8* arg0) {
    u8* t;
    f32 v;

    // r4 is never set before the call in the asm (stale), so it is passed as 0.
    t = fn_800B1AD0(arg0, 0, 0);
    if (t != NULL) {
        v = *(f32*)(arg0 + 0x268) + (lbl_80346BD0[RaySceneTest(t)] / lbl_805FE0B8);
        if (v < lbl_805FE0B0) {
            v = lbl_805FE0B0;
        } else if (v > lbl_805FE0BC) {
            v = lbl_805FE0BC;
        }
        *(f32*)(arg0 + 0x268) = v;
        fn_800A7D88(arg0, 0xC8F251D4U);
        fn_800B1D98(arg0);
        return (s32)0xDC2ED9B5U;
    }
    return -1;
}

}  // extern "C"
