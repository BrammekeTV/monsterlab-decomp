// Small functions batch (miss512): deleting destructors, init helpers, and a few record updaters.
#include "types.h"

extern "C++" void* gEventMgr;   // 0x805FB6C8 (defined in game_ui.cpp)

extern "C" {
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);  // 0x801BA2FC
void  fn_80070460(u8* p);                                       // 0x80070460
void  fn_8008751C(u8* p);                                       // 0x8008751C
void  fn_800712BC(u8* p, s32 a, s32 b);                         // 0x800712BC
void  fn_801F8F34(u8* p, f32 v);                                // 0x801F8F34
void  fn_801F8FA0(u8* p, f32 v);                                // 0x801F8FA0
void  fn_801F8628(u8* p, s8* src, s32 n);                       // 0x801F8628
u8*   fn_801B3308(void);                                        // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                                // 0x801B3310
void  fn_8006FAF4(u8* p, s32 a);                                // 0x8006FAF4
void  fn_8009A354(u8* p);                                       // 0x8009A354
u8*   fn_8009B2AC(u8* p, s32 del);                              // 0x8009B2AC
void  fn_8009BE68(u8* p, s32 del);                              // 0x8009BE68
void  fn_800A8810(u8* p);                                       // 0x800A8810
void  fn_800BA2E8(u8* p);                                       // 0x800BA2E8
void  fn_800AF960(u8* p);                                       // 0x800AF960
u8*   fn_801F94EC(u8* p, s32 del);                              // 0x801F94EC
u8*   fn_801F6238(u8* p, s32 del);                              // 0x801F6238
u8*   fn_800BFA58(u8* p, s32 del);                             // 0x800BFA58
u8*   fn_80062CE0(u8* p, s32 del);                              // 0x80062CE0
void  fn_8018D4CC(void* p);                                     // 0x8018D4CC
void  fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c);  // 0x8032F1E8
void  fn_800526D0(u8* p);                                       // 0x800526D0
void  fn_801467D4(void);                                        // 0x801467D4
void  fn_80023160(void);                                        // 0x80023160
void  fn_8002BE88(u8* obj, f32* v, u8* el);                     // 0x8002BE88
u8*   fn_8003FB98(u8* p, s32 del);                             // 0x8003FB98 (deleting dtor)
u8*   fn_8020F42C(u8* p, s32 del);                              // 0x8020F42C
void* memcpy(void* d, const void* s, unsigned long n);
void  operator_delete(void* p);                                 // 0x801D426C

extern u8  lbl_8035C890[];   // 0x8035C890 (vtable)
extern u8  lbl_8035D8B8[];   // 0x8035D8B8 (vtable)
extern u8  lbl_8035E870[];   // 0x8035E870 (vtable)
extern u8  lbl_8035A3C8[];   // 0x8035A3C8 (vtable)
extern u8  lbl_8035DD48[];   // 0x8035DD48 (vtable)
extern u8  lbl_803584D4[];   // 0x803584D4 (vtable)
extern u8  lbl_80478570[];   // 0x80478570
extern u8  lbl_805FAFC4;     // 0x805FAFC4 (flag, byte)
extern s32 lbl_805FAFD0;     // 0x805FAFD0
extern f32 lbl_805FD954;     // 0x805FD954 (compare threshold)
extern f32 lbl_805FDC10;     // 0x805FDC10
extern f32 lbl_805FDC14;     // 0x805FDC14
extern f32 lbl_805FDC18;     // 0x805FDC18
extern f32 lbl_805FDFC0;     // 0x805FDFC0
extern f32 lbl_805FDFC4;     // 0x805FDFC4
extern f32 lbl_805FFD98;     // 0x805FFD98
extern f32 lbl_805FCA38;     // 0x805FCA38
extern f32 lbl_805FCAA0;     // 0x805FCAA0
extern f32 lbl_805FCB74;     // 0x805FCB74
}

extern "C" {

// 0x80087AEC
// Refreshes the record's state: sends an event, then updates with the truncated float at +0x47C
// and the comparison result against lbl_805FD954.
void fn_80087AEC(u8* arg0) {
    f32 f;
    s32 r;

    fn_80070460(arg0);
    EventSendTo(gEventMgr, 0x016A38A5U, NULL, 0xCA90298EU, 0, 0);
    f = *(f32*)(arg0 + 0x47C);
    r = (s32)f;   // fctiw rounds to nearest; (s32) truncates. Verify if this matters.
    fn_800712BC(arg0, r, (s32)(f < lbl_805FD954));
    fn_8008751C(arg0);
}

// 0x80098CA8
// Resets the sub-object at +0x268 to fixed values and clears the counter at +0x300.
void fn_80098CA8(u8* arg0) {
    s8 buf[3];

    fn_801F8F34(arg0 + 0x268, lbl_805FDC10);
    fn_801F8FA0(arg0 + 0x268, lbl_805FDC14);
    buf[0] = 0;
    buf[1] = -1;
    buf[2] = 0;
    fn_801F8628(arg0 + 0x268, buf, 1);
    *(s32*)(arg0 + 0x300) = 0;
    *(f32*)(arg0 + 0x2FC) = lbl_805FDC18;
    fn_801B3310(fn_801B3308(), arg0 + 0x268);
}

// 0x80099C84
// Deleting destructor: sets vtable, tears down sub-objects at +0x180 and +0x9C, then frees if del > 0.
u8* fn_80099C84(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_8035C890;
        fn_8009A354(arg0);
        fn_8009B2AC(arg0 + 0x180, -1);
        fn_8009BE68(arg0 + 0x9C, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800A7D04
// Deleting destructor: sets vtable, tears down sub-objects at +0x21C and +0x160, then frees if del > 0.
u8* fn_800A7D04(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_8035D8B8;
        fn_800A8810(arg0);
        fn_801F94EC(arg0 + 0x21C, -1);
        fn_801F6238(arg0 + 0x160, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800AC06C
// Resets the state block at +0x264..+0x2A0 to default values (ids -1, floats 0 or 1, counter 0).
void fn_800AC06C(u8* arg0) {
    *(s32*)(arg0 + 0x264) = -1;
    *(s32*)(arg0 + 0x268) = -1;
    *(f32*)(arg0 + 0x26C) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x270) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x274) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x278) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x27C) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x280) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x284) = lbl_805FDFC4;
    *(f32*)(arg0 + 0x288) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x28C) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x290) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x294) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x298) = lbl_805FDFC0;
    *(f32*)(arg0 + 0x29C) = lbl_805FDFC0;
    *(s32*)(arg0 + 0x2A0) = 0;
}

// 0x800B9D10
// Deleting destructor: sets vtable, tears down sub-objects at +0x3BC and +0x240, then frees if del > 0.
u8* fn_800B9D10(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_8035E870;
        fn_800BA2E8(arg0);
        fn_801F6238(arg0 + 0x3BC, -1);
        fn_800BFA58(arg0 + 0x240, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8006D34C
// Deleting destructor: sets vtable, calls the virtual slot at +0x40 of the vtable (this = arg0),
// then base-dtor fn_80062CE0 on arg0, then frees if del > 0.
u8* fn_8006D34C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)arg0 = lbl_8035A3C8;
        ((void (*)(u8*))(*(void**)(lbl_8035A3C8 + 0x40)))(arg0);
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8019CEDC
// Clears the pointer and flag fields of the record at arg0 and releases the three owned sub-objects.
s32 fn_8019CEDC(u8* arg0) {
    *(s32*)(arg0 + 0x15C) = 0;
    fn_8018D4CC(*(u8**)(arg0 + 0x170));
    fn_8018D4CC(*(u8**)(arg0 + 0x178));
    fn_8018D4CC(*(u8**)(arg0 + 0x1E0));
    *(void**)(arg0 + 0x1E0) = NULL;
    *(s32*)(arg0 + 0x150) = 0;
    *(s32*)(arg0 + 0x168) = 0;
    *(s32*)(arg0 + 0x160) = 0;
    *(s32*)(arg0 + 0x164) = 0;
    *(s32*)(arg0 + 0x1E8) = 0;
    *(s32*)(arg0 + 0x1EC) = 1;
    fn_8018D4CC(arg0);
    return 0;
}

// 0x801EE50C
// Copies up to arg2 elements of size arg3 from the buffer at arg0 (read cursor at +0xC, end at +0x8)
// into arg1, advances the cursor, and returns the number of elements copied.
s32 fn_801EE50C(u8* arg0, u8* arg1, s32 arg2, u32 arg3) {
    s32 cursor;
    u32 count;
    u32 bytes;

    cursor = *(s32*)(arg0 + 0xC);
    count = (u32)(*(u32*)(arg0 + 0x8) - (u32)cursor) / arg3;
    if (arg2 < (s32)count) {
        count = (u32)arg2;
    }
    bytes = count * arg3;
    memcpy(arg1, *(u8**)(arg0 + 0x4) + cursor, bytes);
    *(s32*)(arg0 + 0xC) = cursor + (s32)bytes;
    return (s32)count;
}

// 0x800AF628
// Deleting destructor: sets vtable, calls fn_800AF960, then base-dtor fn_80062CE0, frees if del > 0.
u8* fn_800AF628(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)arg0 = lbl_8035DD48;
        fn_800AF960(arg0);
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x801CD718
// Zeroes three float fields of the global block and sets up two pooled arrays with their callbacks.
void fn_801CD718(void) {
    *(f32*)(lbl_80478570 + 0x24) = lbl_805FFD98;
    *(f32*)(lbl_80478570 + 0x28) = lbl_805FFD98;
    *(f32*)(lbl_80478570 + 0x2C) = lbl_805FFD98;
    fn_8032F1E8(lbl_80478570 + 0x48, (void (*)(void))fn_800526D0, 0, 0x30, 0x100);
    fn_8032F1E8(lbl_80478570 + 0x3048, fn_801467D4, 0, 0x10, 0x100);
}

// 0x80029334
// If the float at +0x6C is in (0, 0.5 * (+0x78)) and the count at +0xB4 is below half of +0xCC,
// fires fn_80023160 once (guarded by lbl_805FAFC4).
void fn_80029334(u8* arg0) {
    f32 v;
    s32 half;
    s32 cap;

    v = *(f32*)(arg0 + 0x6C);
    if (v > lbl_805FCA38 && v < *(f32*)(arg0 + 0x78) * lbl_805FCAA0) {
        cap = *(s32*)(arg0 + 0xCC);
        half = (s32)(((u32)cap >> 31) + cap) >> 1;
        if (*(s32*)(arg0 + 0xB4) < half && lbl_805FAFC4 == 0) {
            fn_80023160();
            lbl_805FAFC4 = 1;
        }
    }
}

// 0x8002BD60
// Resets the index at +0x8 to 0, sets lbl_805FAFD0 to 10, and computes a 3-float vector from
// the bounds at +0x1C..+0x28 and the element's float at +0x70, then calls fn_8002BE88.
void fn_8002BD60(u8* arg0) {
    f32 v[3];
    u8* el;
    s32 idx;

    *(s32*)(arg0 + 0x8) = 0;
    lbl_805FAFD0 = 0xA;
    idx = *(s32*)(arg0 + 0x8);
    el = *(u8**)(arg0 + idx * 4);
    v[2] = *(f32*)(el + 0x70);
    v[1] = (*(f32*)(arg0 + 0x20) + *(f32*)(arg0 + 0x28)) * lbl_805FCB74;
    v[0] = (*(f32*)(arg0 + 0x1C) + *(f32*)(arg0 + 0x24)) * lbl_805FCB74;
    fn_8002BE88(arg0, v, el);
}

// 0x8003F994
// Deleting destructor: sets vtable, calls fn_8003FB98 on the pointer at +0x4C, then base-dtor
// fn_8020F42C, frees if del > 0.
u8* fn_8003F994(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)arg0 = lbl_803584D4;
        fn_8003FB98(*(u8**)(arg0 + 0x4C), 1);
        fn_8020F42C(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

}  // extern "C"
