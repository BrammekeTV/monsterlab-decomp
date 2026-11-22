// Misc game functions in 0x8006xxxx-0x800Cxxxx (object cleanup, camera setup, rail/ledge helpers).
#include "types.h"
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  CameraSetFov(void* camera, f32 fov);                                                    // 0x801D8ACC
void* EventPost(s32 id, void* sender);                                                        // 0x801F3918
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void* MemAllocDefault(u32 size);                                                              // 0x801D4650
s32   RaySceneTest(void* ray);                                                                // 0x80070458 (ignores its argument)
void  ScopedLockCtor(ScopedLock* l);                                                          // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                                               // 0x801F53B0
s32   TriggerDoAction(void* action, void* a1, void* a2);                                     // 0x801E0054
void  fn_80062F58(u8* p);                                                                     // 0x80062F58
void  fn_8007B560(u8* p, u8* pos);                                                            // 0x8007B560
void  fn_800943CC(u8* self);                                                                  // 0x800943CC
void  fn_80094C0C(u8* p);                                                                     // 0x80094C0C
void  fn_800B6A88(u8* p, f32 dt);                                                             // 0x800B6A88
void  fn_800B6E48(u8* p, f32 dt);                                                             // 0x800B6E48
s32   fn_800B964C(u8* obj);                                                                   // 0x800B964C
f32   fn_800B9914(u8* obj);                                                                   // 0x800B9914
u8*   fn_800D887C(u8* arg0, u8* arg1, s32 arg2);                                              // 0x800D887C
u8*   fn_800D8F70(u8* a, u8* out);                                                            // 0x800D8F70
void  fn_801BF398(u8* list, u8* item);                                                        // 0x801BF398
void  fn_801C849C(u8* mgr, u8* item);                                                         // 0x801C849C
f32   fn_801D8AD4(u8* p);                                                                     // 0x801D8AD4
f64   fn_8033E8EC(f64 x);                                                                     // 0x8033E8EC
extern f32 lbl_80346098[];      // 0x80346098 (f32 table indexed by RaySceneTest)
extern f32 lbl_80346208[];      // 0x80346208 (f32 table indexed by RaySceneTest)
extern u8  lbl_8035ADA0[];      // 0x8035ADA0 (vtable)
extern u8  lbl_803C4298[];      // 0x803C4298
extern u8  lbl_8047C640[];      // 0x8047C640 (camera)
extern u8  lbl_8047C760[];      // 0x8047C760 (effect manager)
extern u8  lbl_8047C790[];      // 0x8047C790 (list)
extern u8  lbl_805FB140[];      // 0x805FB140 (pointer slot)
extern f32 lbl_805FB144;        // 0x805FB144
}
extern "C++" void* gEventMgr;   // 0x805FB6C8

typedef void (*VirtVoidFn)(u8*, s32);
typedef void (*VirtFloatFn)(u8*, f32);
typedef s32  (*VirtPtrFn)(u8*, u8*);
typedef s32  (*VirtIntFn)(u8*, s32);

static const u32 kEvtA29C69BA = 0xA29C69BAu;
static const u32 kEvt6CCBDF09 = 0x6CCBDF09u;

extern "C" {

// 0x8006D83C
// Teardown: fires the trigger actions and an event for each of the 5 pending entries (slots 8 bytes apart
// from +0x1D4), releases the two list entries at +0x17C, posts event 0xC unless flag 0x04000000 is set on the
// child at +4, releases the effect at +0x178 under a scoped lock, then calls fn_80062F58.
void fn_8006D83C(u8* arg0, s32 arg1) {
    u8* slot = arg0;
    s32 i;
    for (i = 0; i < 5; i++) {
        u8* entry = *(u8**)(slot + 0x1D4);
        if (entry != NULL) {
            *(u8**)(slot + 0x1D4) = NULL;
            u8* sub = *(u8**)(arg0 + 0x9C);
            if (sub != NULL) {
                if (((*(u32*)(sub + 0xA4) >> 27) & 0xF) != 0) {
                    TriggerDoAction(sub + 0xA4, entry, arg0);
                }
            }
            if (((*(u32*)(arg0 + 0x74) >> 27) & 0xF) != 0) {
                TriggerDoAction(arg0 + 0x74, entry, arg0);
            }
            EventSend(gEventMgr, arg0, arg0, kEvtA29C69BA, 0, 0, 1, 0);
        }
        slot += 8;
    }
    slot = arg0;
    for (i = 0; i < 2; i++) {
        u8* item = *(u8**)(slot + 0x17C);
        if (item != NULL) {
            fn_801BF398(lbl_8047C790, item);
            *(u8**)(slot + 0x17C) = NULL;
        }
        slot += 4;
    }
    if ((*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000) == 0) {
        EventPost(0xC, arg0);
    }
    if (*(u8**)(arg0 + 0x178) != NULL) {
        ScopedLock lock;
        ScopedLockCtor(&lock);
        fn_801C849C(lbl_8047C760, *(u8**)(arg0 + 0x178));
        *(u8**)(arg0 + 0x178) = NULL;
        ScopedLockDtor(&lock, -1);
    }
    fn_80062F58(arg0);  // asm also forwards arg1 (r4), which fn_80062F58 ignores
}

// 0x8007810C
// Builds the follow camera: collects +0x1D8, the +0x90 array entries' field 8 and +0x218 into a list, allocates
// the 0x40-byte camera object into +0x758, sets its vtable, defaults, FOV and a derived constant, registers it
// in lbl_803C4298+4 and runs its virtual slot 0x98; if a global object exists, copies its position over.
void fn_8007810C(u8* arg0) {
    u8* sp18[16];  // slots as in the asm frame at sp+0x18
    u8 sp8[16];
    s32 i;
    sp18[0] = *(u8**)(arg0 + 0x1D8);
    for (i = 0; i < *(s32*)(arg0 + 0x8C); i++) {
        sp18[i + 1] = *(u8**)(*(u8**)(arg0 + 0x90 + i * 4) + 0x8);
    }
    sp18[*(s32*)(arg0 + 0x8C) + 1] = *(u8**)(arg0 + 0x218);
    u8* cam = (u8*)MemAllocDefault(0x40);
    *(u8**)(arg0 + 0x758) = cam;
    if (cam != NULL) {
        fn_800D887C(cam, (u8*)sp18, *(s32*)(arg0 + 0x8C) + 2);
        *(u8**)(cam + 0x0) = lbl_8035ADA0;
        *(f32*)(cam + 0x24) = 0.0f;
        *(f32*)(cam + 0x28) = 11.25f;
        *(f32*)(cam + 0x2C) = 0.0f;
        *(f32*)(cam + 0x3C) = fn_801D8AD4(lbl_8047C640);
        CameraSetFov(lbl_8047C640, 1.5707964f * (0.63661975f * (2.0f * (f32)fn_8033E8EC((f64)((0.5f * 4.8f) / *(f32*)(cam + 0x28))))));
        lbl_805FB144 = 90.0f * (0.63661975f * (2.0f * (f32)fn_8033E8EC((f64)((0.5f * 6.4f) / *(f32*)(cam + 0x28)))));
    }
    *(u8**)(lbl_803C4298 + 0x4) = *(u8**)(arg0 + 0x758);
    u8* c2 = *(u8**)(arg0 + 0x758);
    (*(VirtFloatFn*)(*(u8**)c2 + 0x98))(c2, 0.0f);
    if (*(u8**)lbl_805FB140 != NULL) {
        fn_800D8F70(*(u8**)(*(u8**)lbl_805FB140 + 0x758), sp8);
        fn_8007B560(*(u8**)(*(u8**)lbl_805FB140 + 0x75C), sp8);
    }
}

// 0x80094E08
// Teardown: for the objects at +0x90, +0x9C.. (count at +0x1B4) and +0x94, calls virtual slot 0x14 (arg 1) under
// a scoped lock and then sends event 0x6CCBDF09 to each; finally calls fn_80094C0C and fn_800943CC.
void fn_80094E08(u8* arg0) {
    ScopedLock lock;
    u8* obj = *(u8**)(arg0 + 0x90);
    if (obj != NULL) {
        ScopedLockCtor(&lock);
        (*(VirtVoidFn*)(*(u8**)obj + 0x14))(obj, 1);
        ScopedLockDtor(&lock, -1);
        EventSend(gEventMgr, *(u8**)(arg0 + 0x90), NULL, kEvt6CCBDF09, 0, 0, 1, 0);
    }
    u8* slot = arg0;
    s32 i;
    for (i = 0; i < *(s32*)(arg0 + 0x1B4); i++) {
        obj = *(u8**)(slot + 0x9C);
        if (obj != NULL) {
            ScopedLockCtor(&lock);
            (*(VirtVoidFn*)(*(u8**)obj + 0x14))(obj, 1);
            ScopedLockDtor(&lock, -1);
            EventSend(gEventMgr, *(u8**)(slot + 0x9C), NULL, kEvt6CCBDF09, 0, 0, 1, 0);
        }
        slot += 4;
    }
    obj = *(u8**)(arg0 + 0x94);
    if (obj != NULL) {
        ScopedLockCtor(&lock);
        (*(VirtVoidFn*)(*(u8**)obj + 0x14))(obj, 1);
        ScopedLockDtor(&lock, -1);
        EventSend(gEventMgr, *(u8**)(arg0 + 0x94), NULL, kEvt6CCBDF09, 0, 0, 1, 0);
    }
    fn_80094C0C(arg0);
    fn_800943CC(arg0);
}

// 0x800A9508
// Tail call: virtual slot 0x30 of the object at +0x138 of the sub-object at +0x6A4, called with (sub, 2).
s32 fn_800A9508(u8* arg0) {
    u8* sub = *(u8**)(arg0 + 0x6A4);
    return (*(VirtIntFn*)(*(u8**)(sub + 0x138) + 0x30))(sub, 2);
}

// 0x800B63F4
// Per-frame update of the two timers at +0x2D4/+0x2D8: when flag +0x2CC or +0x2D0 is set, enters state 3 (timers
// reset), and leaves it (state 0, flags cleared) once a timer exceeds its table limit. Then runs
// fn_800B6A88 and advances the clock at +0x304 by dt.
void fn_800B63F4(u8* arg0, f32 farg0) {
    fn_800B6E48(arg0, farg0);
    if (*(f32*)(arg0 + 0x2CC) >= 1.0f || *(f32*)(arg0 + 0x2D0) >= 1.0f) {
        if (*(s32*)(arg0 + 0x2B4) != 3) {
            *(s32*)(arg0 + 0x2B4) = 3;
            *(f32*)(arg0 + 0x2D4) = 0.0f;
            *(f32*)(arg0 + 0x2D8) = 0.0f;
        } else {
            if ((*(f32*)(arg0 + 0x2CC) >= 1.0f && *(f32*)(arg0 + 0x2D4) > lbl_80346098[RaySceneTest(NULL /* stale r3 */)])
                || (*(f32*)(arg0 + 0x2D0) >= 1.0f && *(f32*)(arg0 + 0x2D8) > lbl_80346098[RaySceneTest(NULL /* stale r3 */)])) {
                *(s32*)(arg0 + 0x2B4) = 0;
                *(f32*)(arg0 + 0x2CC) = 0.0f;
                *(f32*)(arg0 + 0x2D0) = 0.0f;
            }
        }
    }
    fn_800B6A88(arg0, farg0);
    *(f32*)(arg0 + 0x304) = *(f32*)(arg0 + 0x304) + farg0;
}

// 0x800B66F0
// Scans the 5 slots at +0x2DC for the entry of kind arg1; returns 0 if it is within tolerance of +0x2C4,
// else 1/2 (above/below) when the slot index equals the selected index (+0x2F8/+0x2FC/+0x300 for kind 0/1/2),
// default 1.
s32 fn_800B66F0(u8* arg0, s32 arg1) {
    s32 sel = -1;
    switch (arg1) {
    case 0: sel = *(s32*)(arg0 + 0x2F8); break;
    case 1: sel = *(s32*)(arg0 + 0x2FC); break;
    case 2: sel = *(s32*)(arg0 + 0x300); break;
    }
    s32 result = 1;
    u8* slot = arg0;
    s32 i;
    for (i = 0; i < 5; i++) {
        u8** ent = *(u8***)(slot + 0x2DC);
        if (ent != NULL && arg1 == fn_800B964C(*ent)) {
            f32 pos = fn_800B9914(**(u8***)(slot + 0x2DC));
            result = 0;
            f32 diff = pos - *(f32*)(arg0 + 0x2C4);
            f32 ad = (f32)fabs(diff);
            if (ad < 0.011111111f * lbl_80346208[RaySceneTest(NULL /* stale r3 */)]) {
                result = 0;
            } else if (sel == i) {
                if (diff > 0.0f) {
                    result = 1;
                } else {
                    result = 2;
                }
            }
        }
        slot += 4;
    }
    return result;
}

// 0x800C2628
// Tail call: virtual slot 0x20 of the vtable at a+0xC, called with (a, b's +0x9C node's +8 value or NULL).
s32 fn_800C2628(u8* a, u8* b) {
    u8* n = *(u8**)(b + 0x9C);
    u8* v;
    if (n != NULL) {
        v = *(u8**)(n + 0x8);
    } else {
        v = NULL;
    }
    return (*(VirtPtrFn*)(*(u8**)(a + 0xC) + 0x20))(a, v);
}

}  // extern "C"
