// Small functions in 0x8007xxxx (dance animation, welding gun intro/outro positions,
// gun camera FOV, per-object destructors and the welding float timers).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                         // 0x801D426C
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  CameraSetFov(void* camera, f32 fov);                              // 0x801D8ACC
s32   RaySceneTest(void* ray);                                          // 0x80070458
u8*   fn_800D8F70(u8* a3, u8* a4);                                      // 0x800D8F70
void  fn_800D9180(u8* a3, f32 f1);                                      // 0x800D9180
void  fn_800D8A40(u8* p, s32 a);                                        // 0x800D8A40
void  fn_800792B8(u8* p);                                               // 0x800792B8
void  fn_801C9D74(u8* p);                                               // 0x801C9D74
void  fn_8007D24C(u8* p);                                               // 0x8007D24C
f32   fn_8033F200(f32 a, f32 b);                                        // 0x8033F200
void  fn_8018BEA4(u8* p, f32 t);                                        // 0x8018BEA4
u8*   fn_801B3308(void);                                                // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                      // 0x801B3378
void  fn_8027D270(s32 a, f32 b, f32 c);                                 // 0x8027D270
u8*   fn_801F9598(u8* p, s32 del);                                      // 0x801F9598
u8*   fn_80062CE0(u8* p, s32 del);                                      // 0x80062CE0
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8);      // 0x8006FA44
void  fn_8007A598(u8* p);                                               // 0x8007A598
void  fn_8007B560(u8* p, u8* pos);                                      // 0x8007B560
void  fn_8007B690(u8* p, u8* pos);                                      // 0x8007B690
void  fn_8007BABC(u8* p, u8* pos, f32 t);                               // 0x8007BABC
void  fn_8007BCCC(u8* p, u8* pos, f32 t);                               // 0x8007BCCC
void  fn_8007BED8(u8* p);                                               // 0x8007BED8
void  fn_8007C0CC(u8* p);                                               // 0x8007C0CC
void  fn_8007A440(u8* p);                                               // 0x8007A440
u8*   fn_8007A3B4(u8* p, s32 del);                                      // 0x8007A3B4
}

extern "C++" void* gEventMgr;                 // 0x805FB6C8
extern u8    gWorldState[];             // 0x803E6D00 (+0xC: frame delta)
extern u8    lbl_805FB138[];            // dance object pointer block
extern u8    lbl_805FB140[];            // welding state pointer block
extern u8    lbl_8035ADA0[];            // vtable
extern u8    lbl_8047C640[];            // camera
extern u8    lbl_8047C760[];            // effect manager
extern u8    lbl_803C6900[];            // block passed to fn_8006FA44
extern f32   lbl_803C6914[];            // float triple
extern f32   lbl_803474A8[];            // float table indexed by RaySceneTest
extern f32   lbl_805FD68C;              // float constant
extern f32   lbl_805FD6B8;              // float constant
extern f32   lbl_805FD6BC;              // float threshold
extern f32   lbl_805FD6F0;              // float constant (floor)
extern f32   lbl_805FD6F4;              // float constant
extern f32   lbl_805FD6F8;              // float constant

extern "C" {

typedef u8* (*VirtPtrFn)(u8* obj);

// 0x800767E8
// Advances the dance animation: a speed value from the dance object is scaled by the frame
// delta, and the object's virtual slot 0x10 supplies the sub-object that is updated with it.
void UpdateDanceAnimation(void* a0, f32 a1) {
    u8* p = (u8*)a0;
    u8* g = *(u8**)lbl_805FB138;
    if (g != NULL) {
        f32 speed = *(f32*)(*(u8**)(g + 0x6cc) + 0x8);
        f32 t = fn_8033F200(speed, a1) / a1;
        u8* sub = (*(VirtPtrFn*)(*(u8**)p + 0x10))(p);
        fn_8018BEA4(sub, t);
    }
}

// 0x80079E8C
// Starts the welding-gun event: resets the timer at +0x110C to the constant, sends the
// event to the gun target, and refreshes its state via fn_8007BED8.
void fn_80079E8C(u8* p) {
    fn_800D9180(*(u8**)(p + 0x758), lbl_805FD68C);
    EventSend(gEventMgr, *(u8**)(p + 0x75C), NULL, 0x117741C3u, 0, 0, 1, 0);
    *(f32*)(p + 0x110C) = lbl_805FD68C;
    fn_8007BED8(*(u8**)(p + 0x75C));
}

// 0x80079EF8
// Adds the frame delta to the timer at +0x110C; once it reaches the threshold, signals the
// game flow with id 0x88C6E941.
void fn_80079EF8(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x110C) + dt;
    *(f32*)(p + 0x110C) = t;
    if (t >= lbl_805FD6BC) GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x80079F2C
// Same timer as fn_80079EF8 (identical code, second copy); signals 0x88C6E941 when it fires.
void fn_80079F2C(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x110C) + dt;
    *(f32*)(p + 0x110C) = t;
    if (t >= lbl_805FD6BC) GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x80079F68
// Clears the counter at +0x250 and resets the effect manager, the object at +0x75C (via
// fn_8007C0CC), the object at +0x760 (via fn_8007D24C) and the shared state fn_800792B8.
void fn_80079F68(u8* p) {
    *(s32*)(p + 0x250) = 0;
    fn_801C9D74(lbl_8047C760);
    fn_800792B8(p);
    fn_8007C0CC(*(u8**)(p + 0x75C));
    fn_8007D24C(*(u8**)(p + 0x760));
}

// 0x80079FD4
// Starts the welding music cue: prepares the sub-object at +0x264 of the gun object, sends
// event 0x79F48AF7 to it, then plays the music track with a 0x7CFB34EF id.
void fn_80079FD4(u8* p) {
    fn_8007A440(*(u8**)(p + 0x75C) + 0x264);
    EventSend(gEventMgr, *(u8**)(p + 0x75C), NULL, 0x79F48AF7u, 0, 0, 1, 0);
    app_PlayMusic(0x7CFB34EFu, lbl_805FD6B8);
}

// 0x8007A040
// Places the welding gun at its intro position: reads the position from the object at +0x758
// and applies it to the gun object at +0x75C.
void UpdateWeldingGunIntroPos(void) {
    u8* g = *(u8**)lbl_805FB140;
    if (g != NULL) {
        u8 pos[0x10];
        fn_800D8F70(*(u8**)(g + 0x758), pos);
        fn_8007B560(*(u8**)(*(u8**)lbl_805FB140 + 0x75C), pos);
    }
}

// 0x8007A084
// Places the welding gun at its outro position, same as UpdateWeldingGunIntroPos.
void UpdateWeldingGunOutroPos(void) {
    u8* g = *(u8**)lbl_805FB140;
    if (g != NULL) {
        u8 pos[0x10];
        fn_800D8F70(*(u8**)(g + 0x758), pos);
        fn_8007B690(*(u8**)(*(u8**)lbl_805FB140 + 0x75C), pos);
    }
}

// 0x8007A0C8
// Interpolates the gun from its intro position toward the current position, using the
// frame delta from the world state.
void InterpolateGunFromIntroPosition(void) {
    u8 pos[0x10];
    fn_800D8F70(*(u8**)(*(u8**)lbl_805FB140 + 0x758), pos);
    fn_8007BABC(*(u8**)(*(u8**)lbl_805FB140 + 0x75C), pos, *(f32*)(gWorldState + 0xC));
}

// 0x8007A110
// Interpolates the gun toward its outro position, using the frame delta from the world state.
void InterpolateGunToOutroPosition(void) {
    u8 pos[0x10];
    fn_800D8F70(*(u8**)(*(u8**)lbl_805FB140 + 0x758), pos);
    fn_8007BCCC(*(u8**)(*(u8**)lbl_805FB140 + 0x75C), pos, *(f32*)(gWorldState + 0xC));
}

// 0x8007A158
// Deleting destructor for the gun camera object: restores the camera FOV from +0x3C, runs the
// base destructor (del 0), and frees the object when del > 0.
u8* fn_8007A158(u8* p, s32 del) {
    if (p) {
        *(void**)p = (void*)lbl_8035ADA0;
        CameraSetFov(lbl_8047C640, *(f32*)(p + 0x3C));
        fn_800D8A40(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8007A34C
// Loads the welding constants: stores the two float values in lbl_803C6914 and calls
// fn_8006FA44 on the block at lbl_803C6900 with the packed id constants.
void fn_8007A34C(void) {
    f32* f = (f32*)lbl_803C6914;
    f[0] = lbl_805FD68C;
    f[1] = lbl_805FD6B8;
    f[2] = lbl_805FD68C;
    fn_8006FA44(lbl_803C6900, 0x7C6B0B7E, 1, 0x5AE4B3A6, (s32)0xE2F59EFA, 0);
}

// 0x8007A3B4
// Frees the object when it is non-null and del > 0; returns the pointer.
u8* fn_8007A3B4(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x8007A440
// Clears bit 0x2 of the status word at +0x8 of the object's sub-object (virtual slot 0xC
// result) and of the status word at +0x20 of the object's +0x4 sub-object.
void fn_8007A440(u8* p) {
    u8* obj = *(u8**)p;
    if ((*(VirtPtrFn*)(*(u8**)obj + 0xC))(obj) != NULL) {
        u8* sub = (*(VirtPtrFn*)(*(u8**)obj + 0xC))(obj);
        *(u32*)(sub + 0x8) &= ~0x2u;
    }
    u8* q = *(u8**)(obj + 0x4);
    *(u32*)(q + 0x20) &= ~0x2u;
}

// 0x8007B0D0
// Destructor for the welding object: base-destroys the sub-objects at +0x2A0 and +0x264, then
// the object itself (base dtor with 0), and frees it when del > 0.
u8* fn_8007B0D0(u8* p, s32 del) {
    if (p) {
        fn_801F9598(p + 0x2a0, -1);
        fn_8007A3B4(p + 0x264, -1);
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8007B150
// Resets the welding sub-object at +0x2A0 and sets the two float parameters with fn_8027D270.
void fn_8007B150(u8* p) {
    fn_801B3378(fn_801B3308(), p + 0x2a0);
    fn_8027D270(0, lbl_805FD6F0, lbl_805FD6F4);
}

// 0x8007B194
// Drains the welding value at +0x298 by the frame delta times a per-ray-result rate from
// lbl_803474A8, clamped at the floor lbl_805FD6F0.
void fn_8007B194(u8* p) {
    f32 dt = *(f32*)(gWorldState + 0xC);
    s32 idx = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x298) - dt * lbl_803474A8[idx];
    *(f32*)(p + 0x298) = v;
    if (v < lbl_805FD6F0) *(f32*)(p + 0x298) = lbl_805FD6F0;
}

// 0x8007C080
// Per-frame welding update: copies the value at +0x334 into +0x2A8 when it is above the floor
// (only when the flag at +0x330 is set); when the flag is set, bumps the counter at +0x29C.
void fn_8007C080(u8* p) {
    u8 flag = *(u8*)(p + 0x330);
    *(u8*)(p + 0x354) = 0;
    if (flag != 0) {
        f32 v = *(f32*)(p + 0x334);
        if (v > lbl_805FD6F0) *(f32*)(p + 0x2a8) = v;
    }
    if (*(u8*)(p + 0x330) == 0) return;
    s32 n = *(s32*)(p + 0x29c);
    *(u8*)(p + 0x354) = 1;
    *(s32*)(p + 0x29c) = n + 1;
}

// 0x8007C0CC
// Resets the welding sub-object at +0x264, clears the value at +0x298 to the floor and calls
// fn_8027D270 with the second constant.
void fn_8007C0CC(u8* p) {
    fn_8007A598(p + 0x264);
    *(f32*)(p + 0x298) = lbl_805FD6F0;
    fn_8027D270(0, lbl_805FD6F0, lbl_805FD6F8);
}

// 0x8007C4CC
// Frees the object when it is non-null and del > 0; returns the pointer.
u8* fn_8007C4CC(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

}
