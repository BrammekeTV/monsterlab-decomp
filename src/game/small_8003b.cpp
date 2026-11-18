// More small functions in 0x8003xxxx.
#include "types.h"
#include "game/script_callbacks.h"

extern u8  lbl_80357C10[];   // 0x80357C10 (script registration names)
extern u8  lbl_805FAFE8[];   // 0x805FAFE8 (global object pointer)
extern s32 lbl_805FAFF0;     // 0x805FAFF0 (global value)
extern void* gEventMgr;      // 0x805FB6C8
extern f32 lbl_805FCCA0;     // 0x805FCCA0
extern f32 lbl_805FCD08;     // 0x805FCD08
extern f32 lbl_805FCD30;     // 0x805FCD30
extern f32 lbl_805FCD38;     // 0x805FCD38
extern f32 lbl_805FCD7C;     // 0x805FCD7C
extern f32 lbl_805FCE24;     // 0x805FCE24

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
void  fn_800222F0(void);                                        // 0x800222F0
void  fn_8002238C(void);                                        // 0x8002238C
void  fn_800223D4(void);                                        // 0x800223D4
void  fn_800223EC(void);                                        // 0x800223EC
void  fn_80022E10(void);                                        // 0x80022E10
void  fn_80022F28(void);                                        // 0x80022F28
void  fn_80023160(void);                                        // 0x80023160
void  fn_80021ECC(u8* p);                                       // 0x80021ECC
void  fn_800223F0(u8* p, f32 v);                                // 0x800223F0
void  fn_8003451C(u8* p);                                       // 0x8003451C
s32   fn_800304DC(u8* p, const f32* v);                         // 0x800304DC
s32   CheckResult(void);                                        // 0x800320A4
s32   GainCrystal(void);                                        // 0x800320B4
void  MonsterFeelSuccess(void);                                 // 0x800320F8
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32   RegisterScriptFunc(const char* name, s32 type, void* fn); // 0x801DC91C

// 0x800304DC
// Returns 1 when the point v lies in front of sub-object +0x4's position (+0x68..+0x70) along x and
// within the radius-squared stored at +0x28; otherwise 0.
s32 fn_800304DC(u8* p, const f32* v) {
    u8* q = *(u8**)(p + 0x4);
    f32 dx = *(f32*)(q + 0x68) - v[0];
    f32 dy = *(f32*)(q + 0x6c) - v[1];
    f32 dz = *(f32*)(q + 0x70) - v[2];
    if (dx > lbl_805FCCA0) {
        f32 d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < *(f32*)(p + 0x28)) return 1;
    }
    return 0;
}

// 0x8003056C
// Counter at +0x14 ticks up each call; once it exceeds the threshold at +0x18 it fires the
// virtual method at slot 0x18 and restarts from 0.
void fn_8003056C(u8* p) {
    s32 t = *(s32*)(p + 0x14);
    f32 th = *(f32*)(p + 0x18);
    if ((f32)t > th) {
        (*(void (**)(u8*))(*(u8**)p + 0x18))(p);
        *(s32*)(p + 0x14) = 0;
    } else {
        *(s32*)(p + 0x14) = t + 1;
    }
}

// Advances the 4-frame cycle counter at +0x24 and swaps the image at +0x4 to the texture for the new frame.
static void CycleImageFrame(u8* p, u32 texHash) {
    s32 frame = (*(s32*)(p + 0x24) + 1) & 3;
    *(s32*)(p + 0x24) = frame;
    BOOL tex = UITextureList_GetTexture(texHash, frame);
    if (tex != 0) {
        UIImage_SetTexture(*(u32*)(*(u8**)(p + 0x4)), tex);
    }
}

// 0x800306E0
void fn_800306E0(u8* p) {
    CycleImageFrame(p, 0x81F6E981);
}

// 0x80030830
void fn_80030830(u8* p) {
    CycleImageFrame(p, 0xACCFADBF);
}

// 0x80030980
void fn_80030980(u8* p) {
    CycleImageFrame(p, 0xE3B53643);
}

// 0x80031E98
// Sends event 0x117741C3 to the target at +0x90 when either flag byte +0x80 or +0x81 is set.
void fn_80031E98(u8* p) {
    fn_800223EC();
    if (*(u8*)(p + 0x80) != 0 || *(u8*)(p + 0x81) != 0) {
        EventSend(gEventMgr, *(void**)(p + 0x90), NULL, 0x117741C3, 0, 0, 1, 0);
    }
}

// 0x80031F1C
// Registers the CheckResult, GainCrystal and MonsterFeelSuccess script callbacks.
void fn_80031F1C(void) {
    RegisterScriptFunc((const char*)(lbl_80357C10 + 0x20), 0x1d, (void*)CheckResult);
    RegisterScriptFunc((const char*)(lbl_80357C10 + 0x2c), 0x1d, (void*)GainCrystal);
    RegisterScriptFunc((const char*)(lbl_80357C10 + 0x38), 0x1d, (void*)MonsterFeelSuccess);
}

// 0x800320B4
// Script query: returns 1 if any entry in the list at +0xC0 (count at +0xA8, 0x60 bytes each)
// has type 3 at +0x8; otherwise 0.
s32 GainCrystal(void) {
    u8* g = *(u8**)lbl_805FAFE8;
    s32 count = *(s32*)(g + 0xa8);
    u32 off = 0;
    for (s32 i = 0; i < count; i++, off += 0x60) {
        u8* entry = *(u8**)(g + 0xc0) + off;
        if (*(s32*)(entry + 0x8) == 3) return 1;
    }
    return 0;
}

// 0x80032124
// If the counter at +0xE4 is 1 or more, runs fn_80022E10 and clears it.
void fn_80032124(void) {
    s32 c = *(s32*)(*(u8**)lbl_805FAFE8 + 0xe4);
    if (c >= 1) {
        fn_80022E10();
        *(s32*)(*(u8**)lbl_805FAFE8 + 0xe4) = 0;
    }
}

// 0x80032160
// If the counter at +0xE0 is 6 or more, runs fn_80022F28 and clears it.
void fn_80032160(void) {
    s32 c = *(s32*)(*(u8**)lbl_805FAFE8 + 0xe0);
    if (c >= 6) {
        fn_80022F28();
        *(s32*)(*(u8**)lbl_805FAFE8 + 0xe0) = 0;
    }
}

// 0x8003219C
// If the value at +0x5BC of the global sub-object +0xCC is at least lbl_805FCD30, runs fn_80023160
// and sets that value to lbl_805FCD08.
void fn_8003219C(void) {
    u8* o = *(u8**)(*(u8**)lbl_805FAFE8 + 0xcc);
    if (*(f32*)(o + 0x5bc) >= lbl_805FCD30) {
        fn_80023160();
        o = *(u8**)(*(u8**)lbl_805FAFE8 + 0xcc);
        *(f32*)(o + 0x5bc) = lbl_805FCD08;
    }
}

// 0x80032574
// Copies the global value to +0x44. If the flag at +0x41 is set, clears the global first. When the
// flag at +0x198 is set, raises +0x110 to +0x19C (if larger) and sets +0x44 to 5. Writes the result
// back to the global and returns it.
s32 fn_80032574(u8* p) {
    if (*(u8*)(p + 0x41) != 0) {
        lbl_805FAFF0 = 0;
        *(u8*)(p + 0x41) = 0;
    }
    u8 flag = *(u8*)(p + 0x198);
    *(s32*)(p + 0x44) = lbl_805FAFF0;
    if (flag != 0) {
        f32 t = *(f32*)(p + 0x19c);
        if (t > lbl_805FCD38) *(f32*)(p + 0x110) = t;
        *(s32*)(p + 0x44) = 5;
    }
    lbl_805FAFF0 = *(s32*)(p + 0x44);
    return *(s32*)(p + 0x44);
}

// 0x80032A48
// When +0x10 holds an object and +0x44 is 5, sets +0x48 from that object's type at +0x8:
// 2 -> 2, 1 -> 1, anything else -> 0.
void fn_80032A48(u8* p) {
    u8* q = *(u8**)(p + 0x10);
    if (q == NULL) return;
    if (*(s32*)(p + 0x44) != 5) return;
    s32 k = *(s32*)(q + 0x8);
    if (k == 2) {
        *(s32*)(p + 0x48) = 2;
    } else if (k == 1) {
        *(s32*)(p + 0x48) = 1;
    } else {
        *(s32*)(p + 0x48) = 0;
    }
}

// 0x80032E44
// Stores obj at +0xC, hides it through its second vtable (slot 0x30, arg 2), then sets its
// position vector at +0x68 to (C, C, 0).
void fn_80032E44(u8* p, u8* obj) {
    *(u8**)(p + 0xc) = obj;
    (*(void (**)(u8*, s32))(*(u8**)(obj + 0x138) + 0x30))(obj, 2);
    u8* o = *(u8**)(p + 0xc);
    *(f32*)(o + 0x68) = lbl_805FCD7C;
    *(f32*)(o + 0x6c) = lbl_805FCD7C;
    *(f32*)(o + 0x70) = lbl_805FCD38;
}

// 0x80032F7C
// Deleting destructor: frees p when del > 0; returns p.
u8* fn_80032F7C(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x80033958
// Sets the state at +0x8 to 7, hides the object at +0x18 through its second vtable (slot 0x30, arg 0x1A),
// and clears +0x14.
void fn_80033958(u8* p) {
    *(s32*)(p + 0x8) = 7;
    u8* o = *(u8**)(p + 0x18);
    (*(void (**)(u8*, s32))(*(u8**)(o + 0x138) + 0x30))(o, 0x1a);
    *(s32*)(p + 0x14) = 0;
}

// 0x80034394
// Runs fn_80021ECC, sets the value with fn_800223F0 to lbl_805FCE24, then shows UI element 0x0CAF9C6F.
void fn_80034394(u8* p) {
    fn_80021ECC(p);
    fn_800223F0(p, lbl_805FCE24);
    UIElement_SetVisibleFlag(0x0CAF9C6F, 1);
}

// 0x800343DC
// Runs fn_800223D4, then calls the virtual at slot 0x20 of the vtable at +0x4 with the float argument.
void fn_800343DC(u8* p, f32 f) {
    fn_800223D4();
    (*(void (**)(u8*, f32))(*(u8**)(p + 0x4) + 0x20))(p, f);
}

// 0x8003442C
// Runs fn_8002238C, then calls the virtual at slot 0x20 of the vtable at +0x4 with the float argument.
void fn_8003442C(u8* p, f32 f) {
    fn_8002238C();
    (*(void (**)(u8*, f32))(*(u8**)(p + 0x4) + 0x20))(p, f);
}

// 0x8003447C
// Runs fn_800222F0, then fn_8003451C on p. (The original passes fn_8003451C's r3 through as the
// return value; its type is not known yet, so this returns nothing.)
void fn_8003447C(u8* p) {
    fn_800222F0();
    fn_8003451C(p);
}
}
