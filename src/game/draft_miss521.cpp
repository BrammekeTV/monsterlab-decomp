// Small functions from the draft batch miss521: deleting destructors, a per-object update wrapper,
// a few record lookups and float table fills, and registration of global objects.
#include "types.h"

// C++ linkage, as defined in game_ui.cpp and engine.cpp.
extern void* gEventMgr;     // 0x805FB6C8
extern u8    gWorldState[]; // 0x803E6D00

// fn_800C52F0 is defined with C++ linkage in small_800C.cpp.
u8* fn_800C52F0(u8* p, s32 del);                   // 0x800C52F0 (deleting dtor)

extern "C" {
void     operator_delete(void* p);
void     CatchMinigame_UpdateBase(void* g, f32 dt);     // 0x800224C4
void     EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
s32      RaySceneTest(void* ray);                       // 0x80070458
void     __register_global_object(void* obj, void (*dtor)(void*), void* handle);
void     fn_800223DC(u8* p);                            // 0x800223DC
u8*      fn_80062CE0(u8* p, s32 del);                   // 0x80062CE0 (deleting dtor)
void     fn_800704AC(u8* p, f32 dt);                    // 0x800704AC
void     fn_80088BC0(u8* p, f32 dt);                    // 0x80088BC0
void     fn_8008BA80(u8* p, f32 dt);                    // 0x8008BA80
void     fn_80088004(u8* p, f32 dt);                    // 0x80088004
void     fn_801268C8(u8* world, u8* data, f32 v);       // 0x801268C8
u8*      fn_801292D4(u8* p, s32 del);                   // 0x801292D4 (deleting dtor)
u8*      fn_8020EAD4(u8* p);                            // 0x8020EAD4
void     fn_8020EB44(u8* p, s32 del);                   // 0x8020EB44 (deleting dtor)

extern f32 lbl_805FEEC8;   // 0x805FEEC8 (float constant)
extern f32 lbl_805FEEE0;   // 0x805FEEE0 (float constant)
extern f32 lbl_805FEFB4;   // 0x805FEFB4 (float constant)
extern f32 lbl_805FEFBC;   // 0x805FEFBC (float constant)
extern f32 lbl_805FCAB0;   // 0x805FCAB0 (float constant, scale factor)
extern f32 lbl_805FCBB0;   // 0x805FCBB0 (float constant, update step)
extern f32 lbl_805FD8CC;   // 0x805FD8CC (float constant, default value)
extern f32 lbl_805FFA74;   // 0x805FFA74 (float constant, threshold)
extern f32 lbl_803466E0[]; // 0x803466E0 (float table indexed by RaySceneTest)
extern u8  lbl_803E9F98[]; // 0x803E9F98 (three-float block)
extern u8  lbl_803E6CF0[]; // 0x803E6CF0 (global object handle)
extern u8  lbl_803F2200[]; // 0x803F2200 (three-float block)
extern u8  lbl_803F20C4[]; // 0x803F20C4 (global object)
extern u8  lbl_803F20B8[]; // 0x803F20B8 (global object handle)
extern u32 lbl_805F8C78[]; // 0x805F8C78 (two record tags)
}

extern "C" {

// 0x8009BE68
// Deleting destructor: base dtor (fn_800C52F0) on the object, then frees it if del > 0. Returns the object.
u8* fn_8009BE68(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_800C52F0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80087B70
// Per-frame update: runs the base update, the sub-update at +0x354 when that flag is set, then the tail update.
void fn_80087B70(u8* arg0, f32 farg0) {
    fn_800704AC(arg0, farg0);
    fn_80088BC0(arg0, farg0);
    s32 flag = *(s32*)(arg0 + 0x354);
    if (flag != 0) {
        // The flag value is still in r3 when the original calls fn_8008BA80.
        fn_8008BA80((u8*)(uintptr_t)flag, farg0);
    }
    fn_80088004(arg0, farg0);
}

// 0x800CD6E0
// Returns 1 when the object's record at +0x9C has the tag of either of the two global tags, else 0.
s32 fn_800CD6E0(u8* arg0) {
    u8* rec = *(u8**)(arg0 + 0x9C);
    u32 tag = (rec != NULL) ? *(u32*)(rec + 0x8) : 0U;
    if (lbl_805F8C78[0] == tag) {
        return 1;
    }
    if (lbl_805F8C78[1] == tag) {
        return 1;
    }
    return 0;
}

// 0x8012F64C
// Sets up the world's three-float block (0, 1, 0 style defaults from the constant pool), then
// registers the world with the global-object list.
void fn_8012F64C(void) {
    u8* st = lbl_803E9F98;
    *(f32*)(st + 0x0) = lbl_805FEEC8;
    *(f32*)(st + 0x4) = lbl_805FEEE0;
    *(f32*)(st + 0x8) = lbl_805FEEC8;
    fn_801268C8(gWorldState, st, lbl_805FEEC8);
    __register_global_object(gWorldState, (void (*)(void*))fn_801292D4, lbl_803E6CF0);
}

// 0x8013E46C
// Same as fn_8012F64C for the second global object: sets its three-float block, runs its init, then
// registers its deleting destructor.
void fn_8013E46C(void) {
    u8* st = lbl_803F2200;
    *(f32*)(st + 0x0) = lbl_805FEFB4;
    *(f32*)(st + 0x4) = lbl_805FEFBC;
    *(f32*)(st + 0x8) = lbl_805FEFB4;
    fn_8020EAD4(lbl_803F20C4);
    __register_global_object(lbl_803F20C4, (void (*)(void*))fn_8020EB44, lbl_803F20B8);
}

// 0x801A9E30
// If the object is flagged (bit 2 of +0x64), appends every flagged child with a size above the
// threshold (linked list at +0x120, next at +0x124) to the list in arg1 (count at +0x4, array at +0x8).
s32 fn_801A9E30(u8* arg0, u8* arg1) {
    if (!(*(s32*)(arg0 + 0x64) & 2)) {
        return 0;
    }
    u8* child = *(u8**)(arg0 + 0x120);
    while (child != NULL) {
        if ((*(s32*)(child + 0x64) & 2) && (*(f32*)(child + 0x8C) > lbl_805FFA74)) {
            s32 count = *(s32*)(arg1 + 0x4);
            *(u8**)(*(u8**)(arg1 + 0x8) + count * 4) = child;
            *(s32*)(arg1 + 0x4) = count + 1;
        }
        child = *(u8**)(child + 0x124);
    }
    return 0;
}

// 0x800296C4
// Sets the object's index field at +0x124 and its four bounds (+0x1C..+0x28) from the two corner points
// arg1 and arg2, shrunk by half the size stored in the owner record at +0x104 (+0x74 width, +0x78 height).
void fn_800296C4(u8* arg0, u8* arg1, u8* arg2, s32 arg3) {
    *(s32*)(arg0 + 0x124) = arg3;
    u8* owner = *(u8**)(arg0 + 0x104);
    *(f32*)(arg0 + 0x1C) = *(f32*)(arg1 + 0x0) + (*(f32*)(owner + 0x74) * lbl_805FCAB0);
    *(f32*)(arg0 + 0x24) = *(f32*)(arg2 + 0x0) - (*(f32*)(owner + 0x74) * lbl_805FCAB0);
    *(f32*)(arg0 + 0x20) = *(f32*)(arg1 + 0x4) + (*(f32*)(owner + 0x78) * lbl_805FCAB0);
    *(f32*)(arg0 + 0x28) = *(f32*)(arg2 + 0x4) - (*(f32*)(owner + 0x78) * lbl_805FCAB0);
}

// 0x8002CF2C
// Runs the base init, a catch-minigame update step, then sends event 0xF15918A6 to the object's target at +0x9C.
void fn_8002CF2C(u8* arg0) {
    fn_800223DC(arg0);
    CatchMinigame_UpdateBase(arg0, lbl_805FCBB0);
    EventSend(gEventMgr, *(u8**)(arg0 + 0x9C), NULL, 0xF15918A6U, 0, 0, 1, 0);
}

// 0x8006D250
// Deleting destructor: base dtor (fn_80062CE0) on the object if non-null, then frees it when del > 0. Returns the object.
u8* fn_8006D250(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80085C98
// Chooses the object's range value at +0x26C: a table value picked by three RaySceneTest passes
// when the flag at +0x270 is set, otherwise the default constant.
void fn_80085C98(u8* arg0) {
    s32 idx = RaySceneTest(arg0);
    // Each pass feeds the previous result back in as the pointer argument.
    idx = RaySceneTest((void*)(uintptr_t)idx);
    idx = RaySceneTest((void*)(uintptr_t)idx);
    f32 temp_f0 = lbl_803466E0[idx];
    if (*(u8*)(arg0 + 0x270) != 0) {
        *(f32*)(arg0 + 0x26C) = temp_f0;
        return;
    }
    *(f32*)(arg0 + 0x26C) = lbl_805FD8CC;
}

}  // extern "C"
