// Small functions in 0x8003xxxx (object resets, counters, UI hide helpers, script callback 143).
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern u8*  lbl_805FB008;            // 0x805FB008 (global object pointer, +0x68C..+0x698)
extern f32  lbl_805FCF00;            // 0x805FCF00 (float constant, 0.0f)
extern f32  lbl_805FCF40;            // 0x805FCF40 (float constant)
extern f32  lbl_805FCF44;            // 0x805FCF44 (float constant)
extern f32  lbl_805FCF80;            // 0x805FCF80 (float constant)
extern f32  lbl_805FD070;            // 0x805FD070 (float constant)
extern f32  lbl_805FD074;            // 0x805FD074 (float constant)
extern f32  lbl_805FD07C;            // 0x805FD07C (float constant)
extern f32  lbl_805FD080;            // 0x805FD080 (float constant)
extern f32  lbl_805FD084;            // 0x805FD084 (float constant)

extern "C" {
void  fn_800222F4(u8* p);                                   // 0x800222F4
void  fn_80022E10(void);                                    // 0x80022E10
void  fn_80022F28(void);                                    // 0x80022F28
void  fn_80022394(void);                                    // 0x80022394 (defined in small_8002)
void  fn_800223F0(u8* p, f32 v);                            // 0x800223F0
void  fn_80021ECC(void);                                    // 0x80021ECC
void  fn_80039440(u8* p);                                   // 0x80039440
void  fn_8003107C(u8* p);                                   // 0x8003107C (clears three words at +0, +4, +8)
void  fn_80031090(u8* p, s32 del);                          // 0x80031090 (base destructor)
void  fn_801A8654(u8* obj, s32 a1);                         // 0x801A8654
void  fn_801A8AFC(void* obj);                               // 0x801A8AFC (defined in small_8019c)
void  fn_8020A398(s32 handle);                              // 0x8020A398 (defined in small_8019c)
u8*   fn_801B3308(void);                                    // 0x801B3308 (defined in small_8002b)
void  fn_801B3378(u8* p, u8* key);                          // 0x801B3378 (defined in small_801Bb)
void  MemFreeSmall(void* p);                                // 0x801D4340
void  operator_delete(void* p);                             // 0x801D426C
}

// 0x800344B4
// Sets the flag at +0x624 after the shared reset call (fn_800222F4).
void fn_800344B4(u8* p) {
    fn_800222F4(p);
    *(s32*)(p + 0x624) = 1;
}

// 0x800344E8
// Sets the flag at +0x624 after the shared reset call (fn_80022394).
void fn_800344E8(u8* p) {
    fn_80022394();
    *(s32*)(p + 0x624) = 1;
}

// 0x80034ABC
// If the flag at +0x644 is set, removes the entry at +0x90 from the global list and clears the flag.
void fn_80034ABC(u8* p) {
    if (*(u8*)(p + 0x644) != 0) {
        fn_801B3378(fn_801B3308(), p + 0x90);
        *(u8*)(p + 0x644) = 0;
    }
}

// 0x800364D0
// Advances the counter at +0x63C; when it reaches 3 it resets to 0 and calls fn_80022E10.
void fn_800364D0(u8* p) {
    s32 n = *(s32*)(p + 0x63C);
    if (n < 3) {
        *(s32*)(p + 0x63C) = n + 1;
    } else {
        fn_80022E10();
        *(s32*)(p + 0x63C) = 0;
    }
}

// 0x8003651C
// Clears the counter at +0x63C, then advances the counter at +0x638; at 2 it resets to 0 and calls fn_80022F28.
void fn_8003651C(u8* p) {
    s32 n = *(s32*)(p + 0x638);
    *(s32*)(p + 0x63C) = 0;
    if (n < 2) {
        *(s32*)(p + 0x638) = n + 1;
    } else {
        fn_80022F28();
        *(s32*)(p + 0x638) = 0;
    }
}

// 0x80038578
// Zeroes the sub-record at +0xC and sets the three floats at +0, +4 and +8 to the constant lbl_805FCF00.
void fn_80038578(u8* p) {
    *(s32*)(p + 0xC) = 0;
    *(f32*)(p + 0x0) = lbl_805FCF00;
    *(f32*)(p + 0x4) = lbl_805FCF00;
    *(f32*)(p + 0x8) = lbl_805FCF00;
}

// 0x800385A8
// Destructor: frees the buffer at +0xC (if any), runs the base destructor, and frees the object when del > 0.
void* fn_800385A8(u8* p, s32 del) {
    if (p != 0) {
        u8* buf = *(u8**)(p + 0xC);
        if (buf != 0) {
            MemFreeSmall(buf - 0x10);
        }
        fn_80031090(p, -1);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800389A4
// Clears the three words at +0, +4 and +8 of the object and returns it.
u8* fn_800389A4(u8* p) {
    fn_8003107C(p);
    return p;
}

// 0x80038AA4
// Script callback (id 143): runs once (guarded by the byte at +0x68C). Copies the source record at +0x90
// into the global object's +0x690..+0x698, adding the constants lbl_805FCF40 / lbl_805FCF44 to x and y.
void fn_80038AA4(void) {
    u8* g = lbl_805FB008;
    if (*(u8*)(g + 0x68C) != 0) return;
    *(u8*)(g + 0x68C) = 1;
    u8* src = *(u8**)(g + 0x90);
    f32 x = *(f32*)(src + 0x68);
    f32 y = *(f32*)(src + 0x6C);
    f32 z = *(f32*)(src + 0x70);
    *(f32*)(g + 0x690) = lbl_805FCF40 + x;
    *(f32*)(g + 0x694) = lbl_805FCF44 + y;
    *(f32*)(g + 0x698) = z;
}

// 0x80039710
// Makes sure the sub-record at +0x6DD is initialised, sets the float at +0 to lbl_805FCF80
// and shows UI element 0x0CAF9C6F.
void fn_80039710(u8* p) {
    fn_80021ECC();
    if (*(u8*)(p + 0x6DD) == 0) {
        fn_80039440(p);
        *(u8*)(p + 0x6DD) = 1;
    }
    fn_800223F0(p, lbl_805FCF80);
    UIElement_SetVisibleFlag(0x0CAF9C6F, 1);
}

// 0x80039B60
// Counter at +0x6D8: once it reaches 10 or more, calls fn_80022F28 and resets the counter to 0.
void fn_80039B60(u8* p) {
    if (*(s32*)(p + 0x6D8) >= 10) {
        fn_80022F28();
        *(s32*)(p + 0x6D8) = 0;
    }
}

// 0x80039C48
// Resets state via fn_800222F4; if the object at +0xA0 exists, runs fn_801A8654(obj, 0) then fn_801A8AFC.
void fn_80039C48(u8* p) {
    fn_800222F4(p);
    if (*(u8**)(p + 0xA0) != 0) {
        fn_801A8654(*(u8**)(p + 0xA0), 0);
        fn_801A8AFC(*(void**)(p + 0xA0));
    }
}

// 0x80039C90
// Resets state via fn_80022394; if the object at +0xA0 exists, runs fn_801A8654(obj, 0) then fn_801A8AFC.
void fn_80039C90(u8* p) {
    fn_80022394();
    if (*(u8**)(p + 0xA0) != 0) {
        fn_801A8654(*(u8**)(p + 0xA0), 0);
        fn_801A8AFC(*(void**)(p + 0xA0));
    }
}

// 0x8003A02C
// Loops over the element pointers at +0x25C (count at +0x3B4) and calls each one's secondary-vtable
// slot 0x30 with argument 2.
void fn_8003A02C(u8* p) {
    for (s32 i = 0; i < *(s32*)(p + 0x3B4); i++) {
        u8* elem = *(u8**)(p + 0x25C + i * 4);
        u8* vt2 = *(u8**)(elem + 0x138);
        ((void (*)(u8*, s32))(*(void**)(vt2 + 0x30)))(elem, 2);
    }
}

// 0x8003A27C
// Releases the handle at +0x3C4 through fn_8020A398, then runs fn_80022394.
void fn_8003A27C(u8* p) {
    fn_8020A398(*(s32*)(p + 0x3C4));
    fn_80022394();
}

// 0x8003C744
// Advances the counter at +0x98C; when it reaches 2 it resets to 0 and calls fn_80022E10.
void fn_8003C744(u8* p) {
    s32 n = *(s32*)(p + 0x98C);
    if (n < 2) {
        *(s32*)(p + 0x98C) = n + 1;
    } else {
        fn_80022E10();
        *(s32*)(p + 0x98C) = 0;
    }
}

// 0x8003C790
// Clears the counter at +0x98C, then advances the counter at +0x990; at 2 it resets to 0 and calls fn_80022F28.
void fn_8003C790(u8* p) {
    s32 n = *(s32*)(p + 0x990);
    *(s32*)(p + 0x98C) = 0;
    if (n < 2) {
        *(s32*)(p + 0x990) = n + 1;
    } else {
        fn_80022F28();
        *(s32*)(p + 0x990) = 0;
    }
}

// 0x8003CA78
// Hides the UI elements referenced from +0x8 and +0xC, shows the one from +0x10, and sets the timer
// at +0x4 to lbl_805FD070 with state 4 at +0x1C.
void fn_8003CA78(u8* p) {
    u8* a = *(u8**)(p + 0x8);
    UIElement_SetVisibleFlag(*(u32*)a, 0);
    u8* b = *(u8**)(p + 0xC);
    UIElement_SetVisibleFlag(*(u32*)b, 0);
    u8* c = *(u8**)(p + 0x10);
    UIElement_SetVisibleFlag(*(u32*)c, 1);
    *(f32*)(p + 0x4) = lbl_805FD070;
    *(s32*)(p + 0x1C) = 4;
}

// 0x8003CD7C
// Initialises the state block at r3: clears the flags, sets the float fields to their constants and
// the state word at +0x1C to 1. (Floats at +0x20/+0x24 are written twice in the original; the final values are kept.)
void fn_8003CD7C(u8* p) {
    *(f32*)(p + 0x4) = lbl_805FD070;
    *(f32*)(p + 0x20) = lbl_805FD080;
    *(f32*)(p + 0x24) = lbl_805FD084;
    *(f32*)(p + 0x28) = lbl_805FD07C;
    *(f32*)(p + 0x30) = lbl_805FD074;
    *(u8*)(p + 0x0) = 0;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1C) = 1;
    *(u8*)(p + 0x2C) = 0;
    *(u8*)(p + 0x2D) = 0;
    *(u8*)(p + 0x2E) = 0;
}

// 0x8003CDE0
// Deleting-destructor wrapper: frees the object when del > 0 and returns it.
void* fn_8003CDE0(void* p, s32 del) {
    if (p != 0 && del > 0) {
        operator_delete(p);
    }
    return p;
}
