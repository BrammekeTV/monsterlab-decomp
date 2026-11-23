// Small functions batch (draft_miss453): assorted 0x8002-0x800C routines.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gScreenSets;    // 0x805FB654
extern "C++" void fn_800C5348(u8* p, u8* owner);   // 0x800C5348 (C++ linkage in small_800C.cpp)

extern "C" {
extern u8 lbl_803C4298[];          // 0x803C4298 (+0x4: object, +0x8: camera, +0x38: pad channel)
extern u8 lbl_803C2A20[];          // 0x803C2A20 (+0x820: flag byte)
extern u8 lbl_8035AD08[];          // vtable
extern u8 lbl_8035DC48[];          // vtable
extern u8 lbl_805FB140[];          // pointer slot (welding state)
extern const char lbl_803582D0[];  // "Mob%d_%d_%d" format string
extern const char lbl_805F84D8[];  // "Flicker" name string
extern f32 lbl_805FD08C;           // float constant (100.0f)
extern f32 lbl_805FD650;           // float constant (lower clamp)
extern f32 lbl_805FDEA8;           // float constant
extern f32 lbl_805FE038;           // float constant
extern f32 lbl_805FE03C;           // float constant
extern f32 lbl_805FE070;           // float constant
extern f32 lbl_805FE074;           // float constant
extern f32 lbl_805FE088;           // float constant (2/pi)
extern f32 lbl_805FE230;           // float constant (0.0f)
extern f32 lbl_805FE238;           // float constant (1.0f)

void*  ScreenSetsCurrent(void* sets);                      // 0x8018D1E4
void*  ScreenSetFindElement(void* set, u32 hash);          // 0x80198F18
void*  UIAnim_FindByName(void* set, u32 hash);                   // 0x80198F1C
u32    StringHash(const char* s);
int    SysSprintf(char* buf, const char* fmt, ...);
u32    RandomU32(void);
void*  UIWidgetFindChild(void* widget, u32 hash);
void*  fn_80070F4C(void);                                  // 0x80070F4C
void   fn_800715B8(u8* p, s32 a);                          // 0x800715B8
void   fn_800716D0(u8* p, u32 a);                          // 0x800716D0
u32    PadGetButtons(s32 chan);
BOOL   PadButtonHeld(s32 button);
s32    RoundGetAttacker(void* round);
char*  fn_80040F20(s32 idx, u8* buf);                      // 0x80040F20
char*  fn_80040EFC(s32 idx, u8* buf);                      // 0x80040EFC
s32    fn_80041A8C(void);                                  // 0x80041A8C
u8*    AnimSetGetClip(void* set, s32 index);
s32    fn_801AC944(u8* clip, s32 a, u8* b, s32 c, u8* obj);  // 0x801AC944
void   fn_801AC8A0(u8* a, s32 b, u8* obj);                 // 0x801AC8A0
void   fn_801AC7F0(u8* a, u8* obj);                        // 0x801AC7F0
void   fn_801B5D94(u8* obj, u8* event);                    // 0x801B5D94
BOOL   fn_801B50B0(void* unused, u8* obj);                 // 0x801B50B0
u8*    fn_80062CE0(u8* p, s32 del);                        // 0x80062CE0
u8*    fn_8007C4CC(u8* p, s32 del);                        // 0x8007C4CC
void   fn_8006FAF4(u8* p, s32 a);                          // 0x8006FAF4
void   fn_8007B150(u8* p);                                 // 0x8007B150
u8*    fn_8007CF54(u8* p, s32 del);                       // 0x8007CF54
void   fn_80076654(u8* p, s32 mode);                       // 0x80076654
void   fn_8009BBCC(s32 a, s32 b, u8* obj);                 // 0x8009BBCC
void   __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);
void   BufferFree(void* p);
void   EulerToQuat(Quat* out, const Vec3* euler);
void   operator_delete(void* p);
}

extern "C" {

// Pushes an (x, y) pair into child+0x68/+0x6C, keeps child+0x70, then calls the
// slot-0x2C function of child+0x138 with (child, 0x18, x, keep).
static void PushPairToChild(u8* child, f32 x, f32 y) {
    f32 keep = *(f32*)(child + 0x70);
    *(f32*)(child + 0x68) = x;
    *(f32*)(child + 0x6C) = y;
    *(f32*)(child + 0x70) = keep;
    u8* slot = *(u8**)(child + 0x138);
    (*(void (**)(u8*, s32, f32, f32))(slot + 0x2C))(child, 0x18, x, keep);
}

// 0x80029F30
// Stores the (x, y) pair from arg1 at +0x2C/+0x30 of the owner, then pushes it to every child
// in the pointer list at +0x104 (count from the owner's slot 0x14) and to the child at +0x114.
void fn_80029F30(u8* arg0, u8* arg1) {
    s32 i = 0;
    *(f32*)(arg0 + 0x30) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x2C) = *(f32*)(arg1 + 0x0);
    while (i < (*(s32 (**)(u8*))(*(u8**)arg0 + 0x14))(arg0)) {
        PushPairToChild(*(u8**)(arg0 + 0x104 + i * 4), *(f32*)(arg0 + 0x2C), *(f32*)(arg0 + 0x30));
        i++;
    }
    PushPairToChild(*(u8**)(arg0 + 0x114), *(f32*)(arg0 + 0x2C), *(f32*)(arg0 + 0x30));
}

// 0x8003CE20
// Looks up the four "Mob%d_%d_%d" elements of the current screen set (arg1, index, arg2) into
// +0x8..+0x14, the "Flicker" element into +0x18, and a random phase into +0x4 (scaled by +0x20).
void fn_8003CE20(u8* arg0, s32 arg1, s32 arg2) {
    char name[16];
    void* set = ScreenSetsCurrent(gScreenSets);
    s32 i;
    for (i = 0; i < 4; i++) {
        SysSprintf(name, lbl_803582D0, arg1, i, arg2);
        *(void**)(arg0 + 0x8 + i * 4) = ScreenSetFindElement(set, StringHash(name));
    }
    *(void**)(arg0 + 0x18) = UIAnim_FindByName(set, StringHash(lbl_805F84D8));
    {
        u32 m = (u32)(s32)(lbl_805FD08C * *(f32*)(arg0 + 0x20));
        *(f32*)(arg0 + 0x4) = (f32)(RandomU32() % m) / lbl_805FD08C;
    }
}

// 0x80041994
// Per-frame menu check: when no pad button is held, fn_80041A8C() bit 0 is set and the byte at
// +0xC is nonzero, refreshes the two name buffers for both sides (with the side flag at lbl_803C2A20+0x820
// forced to 1); otherwise calls the object's slot-0x14 update.
void fn_80041994(u8* arg0) {
    u8 buf0[0x100];
    u8 buf1[0x100];
    if (PadGetButtons(*(s32*)(lbl_803C4298 + 0x38)) != 0 || !(fn_80041A8C() & 1) || *(u8*)(arg0 + 0xC) == 0) {
        (*(void (**)(u8*))(*(u8**)arg0 + 0x14))(arg0);
        return;
    }
    u8* round = *(u8**)(arg0 + 0x4);
    u8 saved = *(u8*)(lbl_803C2A20 + 0x820);
    s32 i = 0;
    *(u8*)(lbl_803C2A20 + 0x820) = 1;
    do {
        s32 isAttacker = (RoundGetAttacker(round) == i);
        fn_80040F20(isAttacker, buf0);
        fn_80040EFC(i, buf1);
        i += 1;
    } while (i < 2);
    *(u8*)(lbl_803C2A20 + 0x820) = saved;
}

// 0x80053AF8
// Applies one script state change: op = bits 20..26 of arg1[0]. Op 0x7C plays clip arg1[1] (or
// the -1 / -2 special entries) on the owner's set at +0x9C; ops 0x75..0x76 copy the vector at
// owner+4 into owner+0x74C. Always finishes with fn_801B5D94.
void fn_80053AF8(u8* arg0, u8* arg1) {
    s32 op = (s32)(((u32)*(s32*)arg1 >> 20) & 0x7F);
    if (op == 0x7C) {
        u8* set = *(u8**)(*(u8**)(arg0 + 0x9C) + 0x48);
        if (set != NULL) {
            s32 idx = *(s32*)(arg1 + 0x4);
            if (idx >= 0) {
                u8* clip = AnimSetGetClip(set, idx);
                fn_801AC944(clip, *(s32*)(arg1 + 0x8), *(u8**)(arg0 + 0xA0), -1, arg0);
            } else if (idx == -1) {
                fn_801AC8A0(*(u8**)(arg0 + 0xA0), *(s32*)(arg1 + 0x8), arg0);
            } else if (idx == -2) {
                fn_801AC7F0(*(u8**)(arg0 + 0xA0), arg0);
            }
        }
    } else if (op >= 0x75 && op < 0x77) {
        u8* src = *(u8**)(arg0 + 0x4);
        *(f32*)(arg0 + 0x74C) = *(f32*)(src + 0x0);
        *(f32*)(arg0 + 0x750) = *(f32*)(src + 0x4);
        *(f32*)(arg0 + 0x754) = *(f32*)(src + 0x8);
    }
    fn_801B5D94(arg0, arg1);
}

// 0x8007617C
// Per-frame pad poll for the owner: copies the stored timer values into +0x94 / +0x154 when
// positive, and picks the mode (1 or 3, 0 or 2) from the pad buttons held on the owner's channel.
void fn_8007617C(u8* arg0) {
    if (*(u8*)(arg0 + 0x11C) != 0) {
        f32 v = *(f32*)(arg0 + 0x120);
        if (v > lbl_805FD650) *(f32*)(arg0 + 0x94) = v;
    }
    if (*(u8*)(arg0 + 0x11C) != 0) {
        s32 chan = *(s32*)(lbl_803C4298 + 0x38);
        if (PadButtonHeld(chan * 0x10 + 0x108A) != 0) {
            fn_80076654(arg0, 1);
        } else {
            fn_80076654(arg0, 3);
        }
    }
    if (*(u8*)(arg0 + 0x1DC) != 0) {
        f32 v = *(f32*)(arg0 + 0x1E0);
        if (v > lbl_805FD650) *(f32*)(arg0 + 0x154) = v;
    }
    if (*(u8*)(arg0 + 0x1DC) != 0) {
        s32 chan = *(s32*)(lbl_803C4298 + 0x38);
        if (PadButtonHeld(chan * 0x10 + 0x109B) != 0) {
            fn_80076654(arg0, 0);
            return;
        }
        fn_80076654(arg0, 2);
    }
}

// 0x80078014
// Deleting destructor for the menu object: resets the vtable and globals, releases the
// sub-objects and arrays, and frees the object itself when arg1 > 0. Returns arg0.
u8* fn_80078014(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* h = *(u8**)(arg0 + 0x760);
        *(void**)(arg0 + 0x4) = lbl_8035AD08;
        if (h != NULL) {
            fn_8007CF54(h, 1);
            *(void**)(arg0 + 0x760) = NULL;
        }
        *(void**)(lbl_803C4298 + 0x4) = *(void**)(lbl_803C4298 + 0x8);
        u8* arr = *(u8**)(arg0 + 0x758);
        (*(void (**)(u8*, s32))(*(u8**)arr + 0x8))(arr, -1);
        BufferFree(*(void**)(arg0 + 0x758));
        fn_8007B150(*(u8**)(arg0 + 0x75C));
        *(void**)lbl_805FB140 = NULL;
        __destroy_arr(arg0 + 0x258, (void (*)(void*, s32))fn_8007C4CC, 0x40, 0x14);
        fn_8007C4CC(arg0 + 0x210, -1);
        fn_8007C4CC(arg0 + 0x1D0, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// 0x8009BEC0
// Looks up the entry of the list at +0x78 for index +0x6C (valid while within the window
// [+0x70, +0x70 + +0x74)), stores it at +0x7C, and passes its value to fn_8009BBCC.
// arg1 is forwarded to fn_800C5348 (the call takes the caller's second argument as is).
void fn_8009BEC0(u8* arg0, u8* arg1) {
    s32 r4v = 0;
    fn_800C5348(arg0, arg1);
    s32 a = *(s32*)(arg0 + 0x6C);
    *(s32*)(arg0 + 0xE0) = 0;
    s32 var_r3 = 1;
    if (a >= 0 && *(s32*)(arg0 + 0x70) >= 0) var_r3 = 0;
    if (var_r3 == 0) {
        s32 idx = a;
        s32 var_r0 = 0;
        if (a >= 0) {
            s32 n = *(s32*)(arg0 + 0x74);
            if (a < n) {
                s32 hi = *(s32*)(arg0 + 0x70);
                if (hi < a) {
                    if (a <= hi) idx = a + n;
                    hi += n;
                }
                var_r0 = (a <= idx && idx <= hi) ? 1 : 0;
            }
        }
        s32* slot = NULL;
        if (var_r0 != 0) slot = (s32*)(*(u8**)(arg0 + 0x78)) + a;
        *(void**)(arg0 + 0x7C) = slot;
        if (slot != NULL) r4v = *slot;
    }
    fn_8009BBCC(*(s32*)(arg0 + 0x60), r4v, arg0);
}

// 0x800A6210
// Object setup: flags the two children (+0x4 and the first virtual child), sets the prop state to 0,
// resets the scale words at +0x8..+0x10 to the default constant and sets the draw flag bits.
void fn_800A6210(u8* arg0) {
    *(u8*)(arg0 + 0x264) = 1;
    *(u8*)(arg0 + 0x265) = 1;
    if ((*(s32 (**)(u8*))(*(u8**)arg0 + 0xC))(arg0) != 0) {
        u8* t = (*(u8* (**)(u8*))(*(u8**)arg0 + 0xC))(arg0);
        *(s32*)(t + 0x8) |= 2;
    }
    *(s32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 2;
    *(s32*)(arg0 + 0x15C) |= 0x600000;
    SetPropState(arg0, 0u, 0);
    *(f32*)(arg0 + 0x8) = lbl_805FDEA8;
    *(f32*)(arg0 + 0xC) = lbl_805FDEA8;
    *(f32*)(arg0 + 0x10) = lbl_805FDEA8;
    *(s32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(s32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(s32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(s32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
}

// 0x800AD8D0
// Deleting destructor for the object: resets its vtable and fields, detaches the child at +0x26C,
// runs the base destructor and frees it when arg1 > 0. Returns arg0.
u8* fn_800AD8D0(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* child = *(u8**)(arg0 + 0x26C);
        *(void**)arg0 = lbl_8035DC48;
        *(s32*)(arg0 + 0x264) = -1;
        *(s32*)(arg0 + 0x330) = 0;
        if (child != NULL) {
            fn_801B50B0(arg0, child);
            u8* c4 = *(u8**)(*(u8**)(arg0 + 0x26C) + 0x4);
            *(s32*)(c4 + 0x20) |= 1;
            *(void**)(arg0 + 0x26C) = NULL;
        }
        *(s32*)(arg0 + 0x330) = 0;
        *(f32*)(arg0 + 0x270) = lbl_805FE038;
        *(f32*)(arg0 + 0x274) = lbl_805FE038;
        *(f32*)(arg0 + 0x278) = lbl_805FE038;
        *(f32*)(arg0 + 0x360) = lbl_805FE03C;
        *(u8*)(arg0 + 0x364) = 0;
        *(f32*)(arg0 + 0x368) = lbl_805FE03C;
        *(f32*)(arg0 + 0x36C) = lbl_805FE03C;
        *(s32*)(arg0 + 0x330) = 0;
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// 0x800AF960
// Resets the object's state words at +0x264..+0x2BC, copies the position from +0xC8..+0xD0 to the
// block at obj+0, and builds the quaternion at obj+0xC from the Euler angles at +0xD4..+0xDC (scaled by lbl_805FE088).
void fn_800AF960(u8* arg0) {
    f32 euler[3];
    *(f32*)(arg0 + 0x264) = lbl_805FE070;
    *(f32*)(arg0 + 0x268) = lbl_805FE070;
    *(f32*)(arg0 + 0x26C) = lbl_805FE070;
    *(f32*)(arg0 + 0x270) = lbl_805FE070;
    *(f32*)(arg0 + 0x274) = lbl_805FE070;
    *(f32*)(arg0 + 0x278) = lbl_805FE070;
    *(f32*)(arg0 + 0x27C) = lbl_805FE074;
    *(f32*)(arg0 + 0x280) = lbl_805FE070;
    *(f32*)(arg0 + 0x284) = lbl_805FE070;
    *(s32*)(arg0 + 0x288) = 0;
    *(s32*)(arg0 + 0x28C) = 0;
    *(s32*)(arg0 + 0x290) = 0;
    *(s32*)(arg0 + 0x294) = 0;
    *(s32*)(arg0 + 0x298) = 0;
    *(s32*)(arg0 + 0x2AC) = 0;
    *(s32*)(arg0 + 0x29C) = 0;
    *(s32*)(arg0 + 0x2B0) = 0;
    *(s32*)(arg0 + 0x2A0) = 0;
    *(s32*)(arg0 + 0x2B4) = 0;
    *(s32*)(arg0 + 0x2A4) = 0;
    *(s32*)(arg0 + 0x2B8) = 0;
    *(s32*)(arg0 + 0x2A8) = 0;
    *(s32*)(arg0 + 0x2BC) = 0;
    u8* obj = *(u8**)(arg0 + 0x4);
    *(f32*)(obj + 0x0) = *(f32*)(arg0 + 0xC8);
    *(f32*)(obj + 0x4) = *(f32*)(arg0 + 0xCC);
    *(f32*)(obj + 0x8) = *(f32*)(arg0 + 0xD0);
    euler[0] = *(f32*)(arg0 + 0xD4) * lbl_805FE088;
    euler[1] = *(f32*)(arg0 + 0xD8) * lbl_805FE088;
    euler[2] = *(f32*)(arg0 + 0xDC) * lbl_805FE088;
    EulerToQuat((Quat*)(*(u8**)(arg0 + 0x4) + 0xC), (const Vec3*)euler);
}

// 0x800BA988
// Per-frame widget update: looks up the child widget at +0x478 once, updates the enable flag
// from +0x488, and writes the flag (1.0 or 0.0) into the widget's +0x8C slot, then calls its slot-0x2C update.
void fn_800BA988(u8* arg0) {
    f32 sp8;
    f32 spC;
    f32 sp10;
    f32 sp14;
    void* root = fn_80070F4C();
    if (root != NULL) {
        *(void**)(arg0 + 0x478) = UIWidgetFindChild(root, 0x13902A64u);
    }
    fn_800715B8(arg0, *(s32*)(arg0 + 0x488));
    fn_800716D0(arg0, *(s32*)(arg0 + 0x488) > 0);
    u8* w = *(u8**)(arg0 + 0x478);
    if (w != NULL) {
        sp8 = *(f32*)(w + 0x80);
        spC = *(f32*)(w + 0x84);
        sp10 = *(f32*)(w + 0x88);
        sp14 = (*(s32*)(arg0 + 0x488) > 0) ? lbl_805FE238 : lbl_805FE230;
        *(f32*)(w + 0x80) = sp8;
        *(f32*)(w + 0x84) = spC;
        *(f32*)(w + 0x88) = sp10;
        *(f32*)(w + 0x8C) = sp14;
        u8* o = *(u8**)(arg0 + 0x478);
        (*(void (**)(u8*, s32, f32, f32, f32))(*(u8**)(o + 0x138) + 0x2C))(o, 0x38, sp10, spC, sp8);
    }
}

// 0x800CAAE0
// Ring buffer push: when the ring is not full (head != (tail + 1) % cap), moves the head back one
// slot, stores arg1 in that slot and returns 1; returns 0 when full or arg1 is 0.
s32 fn_800CAAE0(u8* arg0, s32 arg1) {
    if (arg1 == 0) return 0;
    s32 cap = *(s32*)(arg0 + 0x10);
    s32 head = *(s32*)(arg0 + 0x8);
    s32* slot;
    if (head == (*(s32*)(arg0 + 0xC) + 1) % cap) {
        slot = NULL;
    } else {
        s32 nh = head - 1;
        if (nh < 0) nh = cap - 1;
        *(s32*)(arg0 + 0x8) = nh;
        if (*(s32*)(arg0 + 0xC) < 0) {
            *(s32*)(arg0 + 0xC) = nh;
            *(s32*)(arg0 + 0x0) = 0;
        }
        s32 r7 = *(s32*)(arg0 + 0x8);
        s32 r5 = r7;
        s32 ok = 0;
        if (r7 >= 0 && r7 < *(s32*)(arg0 + 0x10)) {
            s32 c = *(s32*)(arg0 + 0xC);
            if (c < r7) {
                if (r7 <= c) r5 = r7 + cap;
                c += cap;
            }
            ok = (r7 <= r5 && r5 <= c) ? 1 : 0;
        }
        slot = ok ? (s32*)(*(u8**)(arg0 + 0x14)) + r7 : NULL;
    }
    *(void**)(arg0 + 0x18) = slot;
    if (slot != NULL) {
        *slot = arg1;
        return 1;
    }
    return 0;
}

}  // extern "C"
