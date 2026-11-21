#include <stdint.h>
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

// Data labels (global scope, same linkage as the other files that name them).
extern u8* lbl_805FB2E8;      // list of event targets, written by fn_80126878
extern u8 lbl_804DD0F0[];     // global object handle for fn_801EFC2C
extern u8 lbl_804DD100[];     // bump pool block (see fn_801EE144)
extern u8 lbl_803E6CE0[];     // 12-byte float block written by fn_80126878
extern f32 lbl_805FEE98;      // float constant (fn_80126878)
extern f32 lbl_805FEEA0;      // float constant (fn_80126878)
extern f32 lbl_805FFCB0;      // float constant (fn_801C37F4)
extern f32 lbl_805FFCB4;      // float constant (fn_801C37F4)

typedef void (*VFnObj)(u8*);
typedef void (*VFnObjF32)(u8*, f32);

extern "C" {
void __register_global_object(void* obj, void (*dtor)(void*), void* handle);
void fn_800696C4(u8* p, s32 a1);
void fn_8006AADC(u8* p, f32 delta);
void fn_800C5F50(u8* p);
void fn_80126718(u8* arg0);
void fn_8018D4CC(void* p);
void* fn_801D40E0(s32 n);
void fn_801ED130(u8* p);
void fn_801EE144(u8* p);
void* fn_801EE160(u8* p, s32 del);       // 0x801EE160, dtor of the lbl_804DD100 pool
}

extern "C" {

// 0x801A7E4C
s32 fn_801A7E4C(u8* arg0) {
    u8* temp;

    temp = *(u8**)(arg0 + 0x190);
    if (temp != NULL) {
        (*(VFnObj*)(*(u8**)(temp + 0x138) + 0x14))(temp);
    }
    fn_8018D4CC(arg0);
    return 0;
}

// 0x8004ED00
void fn_8004ED00(u8* arg0, s32 arg1) {
    fn_800696C4(arg0, arg1);
    if (arg1 != 0) {
        fn_801ED130(arg0 + 0x94);
        *(u8*)(arg0 + 0x38E) = 0;
    }
}

// 0x8004ED50
void fn_8004ED50(u8* arg0, f32 farg0) {
    fn_8006AADC(arg0, farg0);
    (*(VFnObjF32*)(*(u8**)arg0 + 0x74))(arg0, farg0);
}

// 0x8008C0B4
// Pops the last entry off the 4-byte list at +0x294 (count at +0x35C); when the list
// empties, moves the value at +0x370 into +0x374 and clears it.
s32 fn_8008C0B4(u8* arg0) {
    s32 count;
    u8* slot;
    s32 value;

    count = *(s32*)(arg0 + 0x35C);
    if (count <= 0) {
        return 0;
    }
    count -= 1;
    *(s32*)(arg0 + 0x35C) = count;
    slot = arg0 + count * 4;
    value = *(s32*)(slot + 0x294);
    *(s32*)(slot + 0x294) = 0;
    if (*(s32*)(arg0 + 0x35C) == 0) {
        *(s32*)(arg0 + 0x374) = *(s32*)(arg0 + 0x370);
        *(s32*)(arg0 + 0x370) = 0;
    }
    return value;
}

// 0x80126878
void fn_80126878(void) {
    u8* var;

    *(f32*)(lbl_803E6CE0 + 0x0) = lbl_805FEE98;
    *(f32*)(lbl_803E6CE0 + 0x4) = lbl_805FEEA0;
    *(f32*)(lbl_803E6CE0 + 0x8) = lbl_805FEE98;
    var = (u8*)fn_801D40E0(0x150);
    if (var != NULL) {
        fn_80126718(var);
    }
    lbl_805FB2E8 = var;
}

// 0x801EFFFC
// Copies the name in arg1 into a 0x40-byte stack buffer up to NUL or ']', then sets
// the flags word at +0x8 to 0x3FFFF.
void fn_801EFFFC(u8* arg0, u8* arg1) {
    u8 buf[0x40];
    u8* dst;
    u8* limit;
    u8* src;
    s8 c;

    src = arg1;
    dst = buf;
    limit = buf + 0x3F;
    for (;;) {
        c = (s8)*src;
        src += 1;
        if (c == 0 || c == 0x5D) {
            break;
        }
        *dst = (u8)c;
        dst += 1;
        if (dst >= limit) {
            break;
        }
    }
    *dst = 0;
    *(s32*)(arg0 + 0x8) = 0x3FFFF;
}

// 0x8009E260
void fn_8009E260(u8* arg0) {
    fn_800C5F50(arg0);
    if (GetPropCurrentState((u32)(uintptr_t)arg0) != 0) {
        SetPropState(arg0, 0U, 0);
    }
}

// 0x8008C1BC
// Returns 1 when both (arg1, arg2) are within 0x1F of the stored point at +0x364 / +0x368.
s32 fn_8008C1BC(u8* arg0, s32 arg1, s32 arg2) {
    s32 dx;
    s32 dy;

    dx = arg1 - *(s32*)(arg0 + 0x364);
    if (dx < 0) {
        dx = -dx;
    }
    if (dx < 0x20) {
        dy = arg2 - *(s32*)(arg0 + 0x368);
        if (dy < 0) {
            dy = -dy;
        }
        if (dy < 0x20) {
            return 1;
        }
    }
    return 0;
}

// 0x800BC6BC
// Scans up to 50 pointers in the table at +0x170 and returns the first whose state at
// +0x290 is outside 2..5, or NULL.
u8* fn_800BC6BC(u8* arg0) {
    s32 i;
    u8* obj;

    for (i = 0; i < 0x32; i++) {
        obj = *(u8**)(arg0 + 0x170 + i * 4);
        if ((u32)(*(s32*)(obj + 0x290) - 2) > 3U) {
            return *(u8**)(arg0 + 0x170 + i * 4);
        }
    }
    return NULL;
}

// 0x801C37F4
void fn_801C37F4(u8* arg0) {
    *(u8*)(arg0 + 0x20) = 0;
    *(s32*)(arg0 + 0x18) = 0;
    *(u8*)(arg0 + 0x28) = 0;
    *(u8*)(arg0 + 0x29) = 0;
    *(u8*)(arg0 + 0x0) = 1;
    *(u8*)(arg0 + 0x1) = 0;
    *(u8*)(arg0 + 0x2) = 0;
    *(u8*)(arg0 + 0x3) = 0;
    *(u8*)(arg0 + 0x4) = 0;
    *(u8*)(arg0 + 0x5) = 0;
    *(u8*)(arg0 + 0x6) = 1;
    *(f32*)(arg0 + 0x8) = lbl_805FFCB0;
    *(f32*)(arg0 + 0xC) = lbl_805FFCB4;
}

// 0x801EFC2C
// Initialises the bump pool at lbl_804DD100 and registers its destructor.
void fn_801EFC2C(void) {
    fn_801EE144(lbl_804DD100);
    __register_global_object(lbl_804DD100, (void (*)(void*))fn_801EE160, lbl_804DD0F0);
}

}  // extern "C"
