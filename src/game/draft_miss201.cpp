#include <string.h>

#include "types.h"
#include "game/part.h"

extern "C" {
void    BufferFree(void* p);                                            // 0x801D4980
void    CameraSetFov(void* camera, f32 fov);                            // 0x801D8ACC
void*   MemAllocDefault(u32 size);                                      // 0x801D4650
u8*     MonsterCtor(u8* m);
s32     RegisterScriptFunc(const char* name, s32 type, void* fn);      // 0x801DC91C
int     SysSprintf(char* buf, const char* fmt, ...);                    // 0x80142EF8
void*   fn_80109330(const char* name);
void    fn_8020F228(u8* p, s32 a2);
s32     fn_8024F1E8(char* path, u8* p);
void    fn_8032F1E8(void* obj, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);
f64     fn_8033E8EC(f64 x);
s32 fn_801EACF4(u8* arg0, u8* arg1, f32 t);
void    PartInit(PartInstance* p);                                      // 0x8021D320
void    PartDestroy(PartInstance* p, s32 del);                          // 0x8021D374
s32     InterpolateVector3Task_Update_LERP(u8* a, void* b);
s32     InterpolateVector3Task_Update_SLERP(u8* a, void* b);
void    InterpolateVector3Task_DefaultSetVector3Callback(void);
s32     InterpolateQuaternionTask_Update_LERP(u8* a, void* b);
s32     InterpolateQuaternionTask_Update_SLERP(u8* a, void* b);
void    InterpolateQuaternionTask_DefaultSetQuaternionCallback(void);
extern u8  lbl_80358C28[];      // 0x80358C28 (vtable)
extern u8  lbl_80388DC0[];      // 0x80388DC0 (vtable)
extern u8  lbl_80385CE8[];      // 0x80385CE8 (script callback name table)
extern u8  lbl_8047C640[];      // 0x8047C640 (camera)
extern u8  lbl_804DD0C8[];      // 0x804DD0C8 (vector3 interpolate ids)
extern u8  lbl_804DD0D4[];      // 0x804DD0D4 (quaternion interpolate ids)
extern const char* lbl_805F8F90;// 0x805F8F90
extern s32 lbl_805FB8E0;        // 0x805FB8E0
extern s32 lbl_805FB8E4;        // 0x805FB8E4
extern f32 lbl_805FD200;        // 0x805FD200
extern f32 lbl_805FD5E0;        // 0x805FD5E0
extern f32 lbl_805FD5E4;        // 0x805FD5E4
extern f32 lbl_805FD5E8;        // 0x805FD5E8
extern f32 lbl_805FD5EC;        // 0x805FD5EC
extern f32 lbl_80600028;        // 0x80600028
}

extern "C" {

// Writes the object's vtable and part array, then runs its virtual slot 0xC.
// 0x8020EAD4
u8* fn_8020EAD4(u8* arg0) {
    MonsterCtor(arg0);
    *(void**)arg0 = lbl_80388DC0;
    fn_8032F1E8(arg0 + 0x4C, (void (*)(void))PartInit, (void (*)(void))PartDestroy, 0x30, 5);
    (*(void (**)(u8*))(*(u8**)arg0 + 0xC))(arg0);
    return arg0;
}

// Resets an object to its default state (vtable, flags, counters).
// 0x800497F4
u8* fn_800497F4(u8* arg0) {
    fn_8020F228(arg0, 3);
    *(void**)(arg0 + 0x00) = lbl_80358C28;
    *(s32*)(arg0 + 0x18) = 0;
    *(s32*)(arg0 + 0x1C) = 4;
    *(u8*)(arg0 + 0x20) = 0;
    *(f32*)(arg0 + 0x3C) = lbl_805FD200;
    *(f32*)(arg0 + 0x40) = lbl_805FD200;
    *(u8*)(arg0 + 0x44) = 0;
    *(s32*)(arg0 + 0x0C) = -1;
    *(u8*)(arg0 + 0x14) = 0;
    *(s32*)(arg0 + 0x10) = -1;
    *(u8*)(arg0 + 0x15) = 0;
    for (s32 i = 0x21; i <= 0x2E; i++) *(u8*)(arg0 + i) = 1;
    for (s32 i = 0x2F; i <= 0x38; i++) *(u8*)(arg0 + i) = 0;
    return arg0;
}

// Sets the camera FOV from a vertical distance-style value.
// 0x8006FB68
void fn_8006FB68(u8* p, f32 v) {
    f32 r = (f32)fn_8033E8EC((f64)(lbl_805FD5E0 / v));
    CameraSetFov(lbl_8047C640, lbl_805FD5E8 * (lbl_805FD5EC * (lbl_805FD5E4 * r)));
}

// Loads the quest icon texture for the index, falling back to the placeholder.
// 0x8011ECCC
void* fn_8011ECCC(s8 arg0) {
    char buf[0x108];
    void* tex;

    SysSprintf(buf, "%squest_icon_%03d", lbl_805F8F90, arg0);
    tex = fn_80109330(buf);
    if (tex == NULL) {
        SysSprintf(buf, "%squest_icon_placeholder", lbl_805F8F90);
        tex = fn_80109330(buf);
    }
    return tex;
}

// Copies the path with backslashes turned into slashes, then resolves it.
// 0x80146A7C
s32 fn_80146A7C(u8* p, const char* path) {
    char buf[0x404];
    char* c;

    strcpy(buf, path);
    for (c = buf; *c != 0; c++) {
        if (*c == 0x5C) *c = 0x2F;
    }
    if (fn_8024F1E8(buf, p) == 0) return 0;
    *(s32*)(p + 0x48) = 0;
    *(s32*)(p + 0x3C) = *(s32*)(p + 0x34);
    *(s32*)(p + 0x40) = 0;
    *(s32*)(p + 0x44) = 0;
    *(s32*)(p + 0x4C) = 0;
    *(s32*)(p + 0x50) = 0;
    return 1;
}

// Copies two float triples into the object (both current and target slots).
// 0x801C5C74
void fn_801C5C74(u8* obj, f32* a, f32* b) {
    *(f32*)(obj + 0x44) = a[0];
    *(f32*)(obj + 0x14) = a[0];
    *(f32*)(obj + 0x48) = a[1];
    *(f32*)(obj + 0x18) = a[1];
    *(f32*)(obj + 0x4C) = a[2];
    *(f32*)(obj + 0x1C) = a[2];
    *(f32*)(obj + 0x5C) = b[0];
    *(f32*)(obj + 0x2C) = b[0];
    *(f32*)(obj + 0x60) = b[1];
    *(f32*)(obj + 0x30) = b[1];
    *(f32*)(obj + 0x64) = b[2];
    *(f32*)(obj + 0x34) = b[2];
}

// Registers the vector3 interpolate script callbacks and stores their ids.
// 0x801E9DB8
void fn_801E9DB8(void) {
    *(s32*)(lbl_804DD0C8 + 4) = RegisterScriptFunc((const char*)lbl_80385CE8, 0xB,
                                                   (void*)InterpolateVector3Task_Update_LERP);
    *(s32*)(lbl_804DD0C8 + 8) = RegisterScriptFunc((const char*)(lbl_80385CE8 + 0x24), 0xB,
                                                   (void*)InterpolateVector3Task_Update_SLERP);
    *(s32*)(lbl_804DD0C8 + 0) = *(s32*)(lbl_804DD0C8 + 4);
    lbl_805FB8E0 = RegisterScriptFunc((const char*)(lbl_80385CE8 + 0x4C), 0x90,
                                      (void*)InterpolateVector3Task_DefaultSetVector3Callback);
}

// Registers the quaternion interpolate script callbacks and stores their ids.
// 0x801EA508
void fn_801EA508(void) {
    s32 slerp;

    *(s32*)(lbl_804DD0D4 + 4) = RegisterScriptFunc((const char*)(lbl_80385CE8 + 0x80), 0xB,
                                                   (void*)InterpolateQuaternionTask_Update_LERP);
    slerp = RegisterScriptFunc((const char*)(lbl_80385CE8 + 0xA8), 0xB,
                               (void*)InterpolateQuaternionTask_Update_SLERP);
    *(s32*)(lbl_804DD0D4 + 8) = slerp;
    *(s32*)(lbl_804DD0D4 + 0) = slerp;
    lbl_805FB8E4 = RegisterScriptFunc((const char*)(lbl_80385CE8 + 0xD0), 0x8A,
                                      (void*)InterpolateQuaternionTask_DefaultSetQuaternionCallback);
}

// Clears the object to defaults and installs its update callback.
// 0x801EAA5C
void fn_801EAA5C(u8* arg0) {
    *(u16*)(arg0 + 0x78) = 0xFFFF;
    *(u16*)(arg0 + 0x7A) = 0xFFFF;
    *(u16*)(arg0 + 0x7C) = 0xFFFF;
    memset(arg0, 0, 0x64);
    *(s32*)(arg0 + 0x64) = 0;
    *(s32*)(arg0 + 0x68) = 0;
    *(f32*)(arg0 + 0x70) = lbl_80600028;
    *(u16*)(arg0 + 0x78) = 0xFFFF;
    *(u16*)(arg0 + 0x7A) = 0xFFFF;
    *(u16*)(arg0 + 0x7C) = 0xFFFF;
    *(s32*)(arg0 + 0x74) = 0;
    *(u8*)(arg0 + 0x7E) = 0;
    *(u8*)(arg0 + 0x7F) = 0;
    *(void**)(arg0 + 0x6C) = (void*)fn_801EACF4;
}

// Unlinks a node (arg1 - 8) from the list in arg0 and re-links it at the tail.
// 0x801EBDF0
void fn_801EBDF0(u8* arg0, u8* arg1) {
    u8* node = arg1 - 8;
    u8* prev = *(u8**)(node + 0);
    u8* next;
    u8* head;
    u8* x;

    if (prev != NULL) {
        *(u8**)(prev + 4) = *(u8**)(node + 4);
    }
    next = *(u8**)(node + 4);
    if (next != NULL) {
        *(u8**)(next + 0) = *(u8**)(node + 0);
    }
    head = *(u8**)(arg0 + 0);
    if (node == head) {
        u8* newHead = *(u8**)(head + 4);
        *(u8**)(arg0 + 0) = newHead;
        if (newHead != NULL) {
            *(u8**)(newHead + 0) = NULL;
        }
    }
    x = *(u8**)(arg0 + 4);
    *(u8**)(node + 4) = *(u8**)(x + 0x10);
    *(u8**)(node + 0) = NULL;
    if (*(u8**)(x + 0x10) != NULL) {
        *(u8**)(*(u8**)(x + 0x10) + 0) = node;
    }
    *(u8**)(x + 0x10) = node;
}

// Replaces the array in arg0 with a copy of arg1's entries (0xC8 bytes each).
// 0x801EBFBC
void fn_801EBFBC(u8* arg0, u8* arg1) {
    s32 n;

    if (*(s32*)(arg0 + 0) > 0) {
        *(s32*)(arg0 + 0) = 0;
        BufferFree(*(void**)(arg0 + 4));
        *(void**)(arg0 + 4) = NULL;
    }
    n = *(s32*)(arg1 + 0);
    *(s32*)(arg0 + 0) = n;
    *(void**)(arg0 + 4) = MemAllocDefault(n * 0xC8);
    memcpy(*(void**)(arg0 + 4), *(void**)(arg1 + 4), *(s32*)(arg0 + 0) * 0xC8);
}

}  // extern "C"
