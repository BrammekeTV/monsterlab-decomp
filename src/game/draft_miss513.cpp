#include "types.h"

// Globals defined in C++ files (C++ linkage).
extern "C++" void* gEventMgr;   // 0x805FB6C8 (ai_states.cpp)

extern "C" {
// Callees (matched to existing definitions where they exist).
void*  ChoiceDestroy(void* c, s32 del);                               // 0x8020F394
s32    RaySceneTest(void* ray);                                       // 0x80070458
void   __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);
void   fn_80021F2C(u8* p, f32 v);                                     // 0x80021F2C
void   fn_8002DC04(u8* p);                                     // 0x8002DC04 (r3, f1 forwarded unchanged)
void   fn_8002C080(u8* p, u8* a, u8* b);                              // 0x8002C080
u8*    fn_8002C614(u8* p);                                            // 0x8002C614
void   fn_8008C41C(u8* p, f32 fa, f32 fb); // 0x8008C41C
void   fn_800A7D88(void* obj, u32 hash);                              // 0x800A7D88
void*  fn_800C0BA4(void* p, s32 del);                                 // 0x800C0BA4 (element dtor, returns this)
void   fn_800C5F50(u8* p);                                            // 0x800C5F50
u8*    fn_80116C04(u8* p, s32 del);                                   // 0x80116C04
u8*    fn_80116D00(u8* p, s32 del);                                   // 0x80116D00
u8*    fn_801B37C4(u8* p, s32 del);                                   // 0x801B37C4
void   fn_8020EB44(u8* p, s32 del);                                   // 0x8020EB44
u8*    fn_8005CAE8(u8* arg0, s32 arg1);                               // 0x8005CAE8
u32    fn_8021F454(u8* seqData);                                      // 0x8021F454
void   operator_delete(void* p);                                      // 0x801D426C
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190

// Data
extern u8  lbl_8035FD14[];        // 0x8035FD14 (vtable)
extern f32 lbl_80346428[];        // 0x80346428 (float divisor table)
extern f32 lbl_805FCBB0;          // 0x805FCBB0 (float threshold)
extern f32 lbl_805FCBE4;          // 0x805FCBE4 (float reset value)
}

extern "C" {

typedef u8* (*GetFn)(u8*);
typedef void (*DtorFn)(u8*, s32);

// 0x8008C148
// Stores two float args (as ints) at +0x364/+0x368 and forwards them to fn_8008C41C.
s32 fn_8008C148(u8* arg0, f32 farg0, f32 farg1) {
    s32 x = (s32)farg0;
    s32 y = (s32)farg1;
    *(s32*)(arg0 + 0x364) = x;
    *(s32*)(arg0 + 0x368) = y;
    // r4 carries the 0x43300000 float-convert high word left over from the conversion.
    fn_8008C41C(arg0, (f32)x, (f32)y);
    return 1;
}

// 0x8009DA24
// Divides the float at +0xB0 of the sub-object returned by virtual slot 0x10 by a table entry.
void fn_8009DA24(u8* arg0) {
    u8* sub = (*(GetFn*)(*(u8**)arg0 + 0x10))(arg0);
    if (sub != NULL) {
        *(f32*)(sub + 0xB0) = *(f32*)(sub + 0xB0) / lbl_80346428[RaySceneTest(sub)];
    }
    fn_800C5F50(arg0);
}

// 0x800ABD80
// Walks +0x8C -> (type 1) -> chain of sub-objects; returns the final object if its +0xC is 0.
u8* fn_800ABD80(u8* arg0) {
    u8* temp = *(u8**)(arg0 + 0x8C);
    if (temp == NULL) {
        return NULL;
    }
    if (*(s32*)(temp + 0x10) != 1) {
        return NULL;
    }
    u8* v = *(u8**)(*(u8**)(temp + 0xC));
    if (v == NULL) {
        return NULL;
    }
    if (*(s32*)(v + 0x28) != 1) {
        return NULL;
    }
    u8* w = *(u8**)(*(u8**)(v + 0x24));
    if (*(s32*)(w + 0xC) == 0) {
        return w;
    }
    return NULL;
}

// 0x8002CEC0
// Decrements the timer at +0xEC by farg0 (reset to lbl_805FCBE4 when it goes <= threshold), then fn_80021F2C.
void fn_8002CEC0(u8* arg0, f32 farg0) {
    f32 t;

    fn_8002DC04(arg0);
    t = *(f32*)(arg0 + 0xEC) - farg0;
    *(f32*)(arg0 + 0xEC) = t;
    if (!(t > lbl_805FCBB0)) {
        *(f32*)(arg0 + 0xEC) = lbl_805FCBE4;
    }
    fn_80021F2C(arg0, farg0);
}

// 0x8011F570
// Deleting destructor for the object at +0xA658 (fn_80116D00 -1), base cleanup, frees if del > 0.
u8* fn_8011F570(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_80116D00(arg0 + 0xA658, -1);
        fn_80116C04(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8021F94C
// Stores (u16)fn_8021F454(*arg0) * farg0 as an unsigned int at +0x10.
void fn_8021F94C(u8* arg0, f32 farg0) {
    u32 v = fn_8021F454(*(u8**)arg0) & 0xFFFFu;
    f32 prod = farg0 * (f32)v;
    *(u32*)(arg0 + 0x10) = (u32)prod;
}

// 0x80029CA0
// Passes the child at +0xB8 and the current slot (index +0xB4, 0xC-byte entries at +0x3C) to fn_8002C080, advances the index mod 10.
void fn_80029CA0(u8* arg0) {
    s32 t;

    fn_8002C080(arg0 + 0xB8, fn_8002C614(arg0 + 0xB8), arg0 + (*(s32*)(arg0 + 0xB4) * 0xC) + 0x3C);
    t = *(s32*)(arg0 + 0xB4) + 1;
    *(s32*)(arg0 + 0xB4) = t;
    if (t >= 0xA) {
        *(s32*)(arg0 + 0xB4) = 0;
    }
}

// 0x8003FB98
// Deleting destructor: releases the pointer at +0x84 (fn_801B37C4), destroys the choice at +0x8C, frees if del > 0.
u8* fn_8003FB98(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_801B37C4(*(u8**)(arg0 + 0x84), 1);
        ChoiceDestroy(arg0 + 0x8C, -1);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8005C9E4
// Deleting destructor: releases the object at +0x898 (fn_8020EB44 -1), base cleanup, frees if del > 0.
u8* fn_8005C9E4(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_8020EB44(arg0 + 0x898, -1);
        fn_8005CAE8(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800AFB64
// Releases the pointer at +0x28C (fn_800A7D88 on it and on this with hash 0x2AB014A3), copies it to +0x290, clears +0x28C.
void fn_800AFB64(u8* arg0) {
    u8* temp = *(u8**)(arg0 + 0x28C);
    if (temp != NULL) {
        fn_800A7D88(temp, 0x2AB014A3u);
        fn_800A7D88(arg0, 0x2AB014A3u);
        *(void**)(arg0 + 0x290) = *(u8**)(arg0 + 0x28C);
    }
    *(void**)(arg0 + 0x28C) = NULL;
}

// 0x800BFA58
// Deleting destructor: destroys the 4-element array at +0x14 (0x48-byte elements), frees if del > 0.
u8* fn_800BFA58(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        __destroy_arr(arg0 + 0x14, (void (*)(void*, s32))fn_800C0BA4, 0x48u, 4u);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800DBC64
// Deleting destructor: sets the vtable to lbl_8035FD14, calls its slot 0x90 with (this, del), frees if del > 0.
u8* fn_800DBC64(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)arg0 = lbl_8035FD14;
        (*(DtorFn*)(lbl_8035FD14 + 0x90))(arg0, arg1);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8007FDCC
// Copies a 12-float block (0x30 bytes) from arg1 to arg0.
void fn_8007FDCC(u8* arg0, u8* arg1) {
    *(f32*)(arg0 + 0x0) = *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x4) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x8) = *(f32*)(arg1 + 0x8);
    *(f32*)(arg0 + 0xC) = *(f32*)(arg1 + 0xC);
    *(f32*)(arg0 + 0x10) = *(f32*)(arg1 + 0x10);
    *(f32*)(arg0 + 0x14) = *(f32*)(arg1 + 0x14);
    *(f32*)(arg0 + 0x18) = *(f32*)(arg1 + 0x18);
    *(f32*)(arg0 + 0x1C) = *(f32*)(arg1 + 0x1C);
    *(f32*)(arg0 + 0x20) = *(f32*)(arg1 + 0x20);
    *(f32*)(arg0 + 0x24) = *(f32*)(arg1 + 0x24);
    *(f32*)(arg0 + 0x28) = *(f32*)(arg1 + 0x28);
    *(f32*)(arg0 + 0x2C) = *(f32*)(arg1 + 0x2C);
}

// 0x8008BA14
// Bumps the counter at +0x4CC, clears the flag at +0x264 when set, then sends an event to gEventMgr
// (tail call) if the state at +0x35C is 0x32.
void fn_8008BA14(u8* arg0) {
    s32 flag;
    s32 ok;

    *(s32*)(arg0 + 0x4CC) = *(s32*)(arg0 + 0x4CC) + 1;
    flag = *(s32*)(arg0 + 0x264);
    if (flag == 1) {
        *(s32*)(arg0 + 0x264) = 0;
    }
    ok = (*(s32*)(arg0 + 0x35C) == 0x32);
    if (ok == 0) {
        return;
    }
    EventSend(gEventMgr, arg0, NULL, 0x4C3FEC1Cu, 0, 0, 1, 0);
}

}  // extern "C"
