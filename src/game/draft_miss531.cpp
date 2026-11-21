#include "types.h"

// Record fields used here: 0x264..0x284 hold the cached vectors, 0x264 is an active flag.
extern "C++" void* gEventMgr;  // 0x805FB6C8

extern "C" {
void  operator_delete(void* p);
void  TextureRelease(void* tex);
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
void  fn_800ABA44(u8* p);                                       // 0x800ABA44
void  fn_800AB6A0(u8* p, u32 id);                               // 0x800AB6A0 (return type unknown)
void  fn_8009BBCC(s32 a, u8* b, u8* obj);                       // 0x8009BBCC
s32   fn_800584D0(u8* arg0, u8* arg1);                          // 0x800584D0
s32   fn_800C2AB0(u8* p, u8* v);                                // 0x800C2AB0 (signature guessed from call)
u8*   fn_800C2FF0(u8* p);                                       // 0x800C2FF0
u8*   fn_8021F644(u8* p, s32 del);                              // 0x8021F644
u8*   fn_8020F42C(u8* p, s32 del);                              // 0x8020F42C
u8*   fn_80116C04(u8* p, s32 del);                              // 0x80116C04

extern f32 lbl_805FDF98;  // 0x805FDF98
extern f32 lbl_805FE2C8;  // 0x805FE2C8
extern f32 lbl_805FFAB8;  // 0x805FFAB8
}

extern "C" {

// 0x8005E130
// Looks up the current key (object at +0x9C of the record at +0x5C) in the 8-entry table at +0xBE0;
// returns 1 when the matching entry's flag is 0, otherwise forwards to fn_800584D0.
s32 fn_8005E130(u8* arg0, u8* arg1) {
    u8* p = *(u8**)(arg1 + 0x5c);
    if (p != NULL && (*(u32*)(*(u8**)(p + 0x4) + 0x1c) & 0x20)) {
        u8* obj = *(u8**)(p + 0x9c);
        u32 key = (obj != NULL) ? *(u32*)(obj + 0x8) : 0;
        u8 flag = 0;
        for (s32 i = 0; i < 8; i++) {
            if (*(u32*)(arg0 + 0xbe0 + i * 8) == key) {
                flag = *(u8*)(arg0 + 0xbe4 + i * 8);
                break;
            }
        }
        if (flag == 0) {
            return 1;
        }
    }
    return fn_800584D0(arg0, arg1);
}

// 0x8009C254
// Forwards to fn_8009BBCC unless the key of arg1's sub-object is one of two special values.
void fn_8009C254(u8* arg0, u8* arg1) {
    u8* sub = *(u8**)(arg1 + 0x9c);
    u32 key = (sub != NULL) ? *(u32*)(sub + 0x8) : 0;
    if (key + 0x71590000u == 0x54dau) {
        return;
    }
    sub = *(u8**)(arg1 + 0x9c);
    key = (sub != NULL) ? *(u32*)(sub + 0x8) : 0;
    if (key - 0x05b00000u == 0xc2d7u) {
        return;
    }
    fn_8009BBCC(*(s32*)(arg0 + 0x60), arg1, arg0);
}

// 0x8009F894
// Returns 0 if the value at arg0 already equals arg1, otherwise stores arg1 via fn_800C2AB0.
s32 fn_8009F894(u8* arg0, u8* arg1) {
    if (fn_800C2FF0(arg0) == arg1) {
        return 0;
    }
    return fn_800C2AB0(arg0, arg1);
}

// 0x800AB0D4
// Zeroes the six cached vector components (+0x264..+0x278) to lbl_805FDF98 and runs fn_800ABA44.
void fn_800AB0D4(u8* arg0) {
    *(f32*)(arg0 + 0x264) = lbl_805FDF98;
    *(f32*)(arg0 + 0x268) = lbl_805FDF98;
    *(f32*)(arg0 + 0x26c) = lbl_805FDF98;
    *(f32*)(arg0 + 0x270) = lbl_805FDF98;
    *(f32*)(arg0 + 0x274) = lbl_805FDF98;
    *(f32*)(arg0 + 0x278) = lbl_805FDF98;
    fn_800ABA44(arg0);
}

// 0x800AB5F8
// Copies two vectors into the cached fields at +0x264 and +0x270..+0x284, copies arg1 into the
// transform at *(+4), sets four flag bits there, then tail-calls fn_800AB6A0 with id 0x48AA31F8.
void fn_800AB5F8(u8* arg0, u8* arg1, u8* arg2) {
    u8* sub = *(u8**)(arg0 + 0x4);
    *(f32*)(sub + 0x0) = *(f32*)(arg1 + 0x0);
    *(f32*)(sub + 0x4) = *(f32*)(arg1 + 0x4);
    *(f32*)(sub + 0x8) = *(f32*)(arg1 + 0x8);

    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000u;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000u;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000u;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000u;

    *(f32*)(arg0 + 0x264) = *(f32*)(arg2 + 0x0);
    *(f32*)(arg0 + 0x268) = *(f32*)(arg2 + 0x4);
    *(f32*)(arg0 + 0x26c) = *(f32*)(arg2 + 0x8);
    *(f32*)(arg0 + 0x270) = *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x274) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x278) = *(f32*)(arg1 + 0x8);
    *(f32*)(arg0 + 0x27c) = *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x280) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x284) = *(f32*)(arg1 + 0x8);
    fn_800AB6A0(arg0, 0x48AA31F8u);
}

// 0x800BDC14
// Counts the timer at +0x26C down by dt while the active flag at +0x264 is set; when it reaches
// zero or below, sends event 0x2B5DB1E2 to the event manager.
void fn_800BDC14(u8* arg0, f32 dt) {
    if (*(u8*)(arg0 + 0x264) == 0) {
        return;
    }
    f32 v = *(f32*)(arg0 + 0x26c) - dt;
    *(f32*)(arg0 + 0x26c) = v;
    if (!(v <= lbl_805FE2C8)) {
        return;
    }
    EventSend(gEventMgr, arg0, NULL, 0x2B5DB1E2u, 0, 0, 1, 0);
}

// 0x8011EA6C
// Deleting destructor: releases the sub-object via fn_80116C04, frees arg0 if del > 0. Returns arg0.
u8* fn_8011EA6C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_80116C04(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8011EC6C
// Drops one reference on the texture at +0x30 (releasing it at zero) and clears the pointer.
void fn_8011EC6C(u8* arg0) {
    u8* tex = *(u8**)(arg0 + 0x30);
    if (tex != NULL) {
        s32 refs = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = refs;
        if (refs == 0) {
            TextureRelease(tex);
        }
        *(u8**)(arg0 + 0x30) = NULL;
    }
}

// 0x801ACC48
// Initialises the record from the source object arg1 (fields +0, +4, +8) and the int arg2.
void fn_801ACC48(u8* arg0, u8* arg1, s32 arg2) {
    *(s32*)(arg0 + 0x2c) = *(s32*)(arg1 + 0x4);
    *(s32*)(arg0 + 0xc) = *(s32*)(arg1 + 0x0);
    *(u8**)(arg0 + 0x8) = arg1;
    *(u8**)(arg0 + 0x30) = *(u8**)(arg1 + 0x8);
    *(s32*)(arg0 + 0x10) = arg2;
    *(f32*)(arg0 + 0x14) = lbl_805FFAB8;
    *(f32*)(arg0 + 0x18) = lbl_805FFAB8;
    *(f32*)(arg0 + 0x1c) = lbl_805FFAB8;
    *(s32*)(arg0 + 0x34) = -1;
    *(u8*)(arg0 + 0x38) = 1;
}

// 0x8020F5B0
// Deleting destructor: base dtor fn_8020F42C on arg0, frees it if arg1 > 0. Returns arg0.
u8* fn_8020F5B0(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_8020F42C(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80220094
// Deleting destructor: base dtor fn_8021F644 on arg0, frees it if arg1 > 0. Returns arg0.
u8* fn_80220094(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_8021F644(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

}  // extern "C"
