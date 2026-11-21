#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gEventMgr;              // 0x805FB6C8
extern "C++" void* gLab;                   // 0x805FBBC8

extern "C" {
void  BufferFree(void* p);                                                  // 0x801D4980
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);
void  MemFreeSmall(void* p);
s32   RaySceneTest(void* ray);
void  operator_delete(void* p);
u8*   fn_800216B8(u8* p, s32 del);
void  fn_80023160(void);
void  fn_80070460(u8* p);
void  fn_800704AC(u8* p, f32 dt);
void  fn_800712BC(u8* p, s32 a, s32 b);
void  fn_800AA8C0(u8* p);
void  fn_800AC0F0(u8* p, f32 dt);                                           // arg0 read as a pointer slot
void  fn_800B1D98(u8* p);
void  fn_800B2F98(u8* p);
u8*   fn_800C6CA4(u8* p, s32 del);
void  fn_800C7620(u8* p);
u8*   fn_800C82C0(u8* p, s32 del);
s32   fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);
u8*   fn_801B3308(void);
void  fn_801B3378(u8* p, u8* key);
u8*   fn_801F6238(u8* p, s32 del);
u8*   fn_801F9C78(u8* p, s32 del);

extern f32 lbl_805FCFAC;
extern f32 lbl_805FCFB8;
extern f32 lbl_805FD070;
extern f32 lbl_805FE0B0;
extern f32 lbl_805FE0B8;
extern f32 lbl_805FE0BC;
extern f32 lbl_805FE11C;
extern f32 lbl_805FE15C;
extern f32 lbl_805FDF18;
extern f32 lbl_805FDF50;
extern f32 lbl_80346BB0[];
extern u8  lbl_80357E80[];
extern u8  lbl_80357F58[];
extern u8  lbl_8037617C[];
}

extern "C" {

// 0x800C7540
// Deleting destructor: tears down the three 100-entry arrays, then the base, frees if del > 0.
u8 * fn_800C7540(u8 * p, s32 del) {
    if (p != NULL) {
        fn_800C7620(p);
        __destroy_arr(p + 0xFB4, (void (*)(void*, s32))fn_800C6CA4, 0x14U, 0x64U);
        __destroy_arr(p + 0x7E4, (void (*)(void*, s32))fn_800C6CA4, 0x14U, 0x64U);
        __destroy_arr(p + 0x14, (void (*)(void*, s32))fn_800C6CA4, 0x14U, 0x64U);
        fn_800C82C0(p, -1);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800B1674
// Advances the float at +0x268 by dt scaled from a table lookup, clamped to [lbl_805FE0B0, lbl_805FE0BC].
void fn_800B1674(u8 * p, f32 dt) {
    f32 v;
    f32 clamped;

    v = *(f32*)(p + 0x268) + (dt * (lbl_80346BB0[RaySceneTest(p)] / lbl_805FE0B8));
    *(f32*)(p + 0x268) = v;
    if (v < lbl_805FE0B0) {
        clamped = lbl_805FE0B0;
    } else if (v > lbl_805FE0BC) {
        clamped = lbl_805FE0BC;
    } else {
        clamped = v;
    }
    *(f32*)(p + 0x268) = clamped;
    fn_800B1D98(p);
}

// 0x800B41EC
// Starts the music, sends the two events and runs the state setup with the int part of +0x674 and a flag.
void fn_800B41EC(u8 * p) {
    s32 n;

    app_PlayMusic(0xA34C55EDU, lbl_805FE11C);
    fn_80070460(p);
    EventSendTo(gEventMgr, 0x016A38A5U, NULL, 0xCA90298EU, 0, 0);
    // fctiw of the float at +0x674 goes through the stack; the C cast truncates instead of rounding to nearest.
    n = (s32)*(f32*)(p + 0x674);
    fn_800712BC(p, n, *(f32*)(p + 0x674) < lbl_805FE15C);
    fn_800B2F98(p);
}

// 0x80148498
// Resets a two-buffer record: frees both buffers, clears the count, frees the object if del > 0.
void * fn_80148498(u8 * p, s32 del) {
    if (p != NULL) {
        u8 * buf4 = *(u8**)(p + 0x4);
        *(void**)(p + 0x0) = lbl_8037617C;
        if (buf4 != NULL) {
            MemFreeSmall(buf4);
            *(void**)(p + 0x4) = NULL;
        }
        {
            u8 * buf8 = *(u8**)(p + 0x8);
            if (buf8 != NULL) {
                MemFreeSmall(buf8);
                *(void**)(p + 0x8) = NULL;
            }
        }
        *(s32*)(p + 0xC) = 0;
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x801EE160
// Clears a buffer record; frees the buffer first when its flag bytes are set.
void * fn_801EE160(u8 * p, s32 del) {
    if (p != NULL) {
        if (*(u8*)(p + 0x0) != 0) {
            u8 * buf = *(u8**)(p + 0x4);
            if (buf != NULL && *(u8*)(p + 0x14) != 0) {
                BufferFree(buf);
            }
        }
        *(void**)(p + 0x4) = NULL;
        *(u8*)(p + 0x14) = 0;
        *(s32*)(p + 0x10) = 0;
        *(u8*)(p + 0x0) = 0;
        *(s32*)(p + 0x8) = 0;
        *(s32*)(p + 0xC) = 0;
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x80034B04
// Deleting destructor: sets the vtable, detaches the flagged sub-object, tears down the sub-object at +0x90 and the base.
u8 * fn_80034B04(u8 * p, s32 del) {
    if (p != NULL) {
        *(void**)(p + 0x4) = lbl_80357E80;
        if (*(u8*)(p + 0x644) != 0) {
            fn_801B3378(fn_801B3308(), p + 0x90);
            *(u8*)(p + 0x644) = 0;
        }
        fn_801F9C78(p + 0x90, -1);
        fn_800216B8(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x8003755C
// Deleting destructor for the variant that also clears the global lab's +0x64 field.
u8 * fn_8003755C(u8 * p, s32 del) {
    if (p != NULL) {
        *(void**)(p + 0x4) = lbl_80357F58;
        fn_801B3378(fn_801B3308(), p + 0x90);
        if (gLab != NULL) {
            *(s32*)((u8*)gLab + 0x64) = -1;
        }
        fn_801F6238(p + 0x90, -1);
        fn_800216B8(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x8003C7F4
// Fires fn_80023160 once when the float at +0x6C is inside (0, 0.5 * +0x78) and the half of +0x984 is below +0x3B8.
void fn_8003C7F4(u8* p) {
    s32 half;

    half = *(s32*)(p + 0x984) / 2;
    if (*(f32*)(p + 0x6C) > lbl_805FCFAC &&
        *(f32*)(p + 0x6C) < *(f32*)(p + 0x78) * lbl_805FCFB8 &&
        *(u8*)(p + 0x988) == 0 &&
        half < *(s32*)(p + 0x3B8)) {
        fn_80023160();
        *(u8*)(p + 0x988) = 1;
    }
}

// 0x8003CB1C
// Resets the UI state record and sets the visibility of its four UI elements (first on, others off).
void fn_8003CB1C(u8* p) {
    *(u8*)(p + 0x2C) = 1;
    *(f32*)(p + 0x4) = lbl_805FD070;
    *(s32*)(p + 0x1C) = 1;
    UIElement_SetVisibleFlag(*(u32*)(*(u8**)(p + 0x8)), 1U);
    UIElement_SetVisibleFlag(*(u32*)(*(u8**)(p + 0xC)), 0U);
    UIElement_SetVisibleFlag(*(u32*)(*(u8**)(p + 0x10)), 0U);
    UIElement_SetVisibleFlag(*(u32*)(*(u8**)(p + 0x14)), 0U);
    if (*(u8**)(p + 0x18) != NULL) {
        fn_8019BBA4(*(u8**)(p + 0x8), *(u8**)(p + 0x18), 0xA);
    }
}

// 0x8008C2EC
// Sends event 0xEEC129BF to each target in the list at +0x294 (count at +0x35C), then clears the list state.
s32 fn_8008C2EC(u8* p) {
    s32 count;
    s32 i;

    count = *(s32*)(p + 0x35C);
    i = 0;
    while (i < *(s32*)(p + 0x35C)) {
        EventSend(gEventMgr, *(u8**)(p + 0x294 + i * 4), NULL, 0xEEC129BFU, 0, 0, 1, 0);
        i += 1;
    }
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x35C) = 0;
    *(s32*)(p + 0x370) = 0;
    *(s32*)(p + 0x374) = 0;
    return count;
}

// 0x8008C380
// Same as fn_8008C2EC but sends event 0x13161A1E.
s32 fn_8008C380(u8* p) {
    s32 count;
    s32 i;

    count = *(s32*)(p + 0x35C);
    i = 0;
    while (i < *(s32*)(p + 0x35C)) {
        EventSend(gEventMgr, *(u8**)(p + 0x294 + i * 4), NULL, 0x13161A1EU, 0, 0, 1, 0);
        i += 1;
    }
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x35C) = 0;
    *(s32*)(p + 0x370) = 0;
    *(s32*)(p + 0x374) = 0;
    return count;
}

// 0x800A8E3C
// Ticks the float at +0x158 down by dt (reset to lbl_805FDF50 when it reaches lbl_805FDF18), then updates 14 pointer slots from +0xE0.
void fn_800A8E3C(u8 * p, f32 dt) {
    f32 v;
    s32 i;

    fn_800704AC(p, dt);
    v = *(f32*)(p + 0x158) - dt;
    *(f32*)(p + 0x158) = v;
    if (v <= lbl_805FDF18) {
        *(f32*)(p + 0x158) = lbl_805FDF50;
        fn_800AA8C0(p);
    }
    i = 0;
    do {
        fn_800AC0F0(*(u8**)(p + 0xE0 + i * 4), dt);
        i += 1;
    } while (i < 0xE);
}

}  // extern "C"
