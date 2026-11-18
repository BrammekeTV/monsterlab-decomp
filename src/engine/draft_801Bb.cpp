// Collision shape queries (shape type at +0xC: 0 = sphere/point, 4, 7 = box-like) and
// container helpers in 0x801Bxxxx.
#include "types.h"

extern "C" {
void BufferFree(void* p);                                                                 // 0x801D4980
s32  fn_801C08CC(u8* a, u8* b, u8* c, u8* d);                                             // 0x801C08CC
s32  fn_801C098C(u8* a, u8* b, u8* c, u8* d, u8* e, f32* out0, f32* out1);                // 0x801C098C
s32  fn_801C10B4(u8* a, u8* b, u8* c, u8* d, u8* e, u8* f);                               // 0x801C10B4
s32  fn_801C19F4(u8* a, u8* b, u8* c, u8* d, s32* n, f32* pos, f32* dist);                // 0x801C19F4
s32  fn_801C1E04(u8* a, u8* b, u8* c, u8* d, s32* n, f32* pos, f32* dist);                // 0x801C1E04
s32  fn_801C217C(u8* a, u8* b, u8* c, u8* d, u8* e, s32* n, f32* pos, f32* dist);         // 0x801C217C
s32  fn_801C26A8(u8* a, u8* b, u8* c, u8* d, u8* e, s32* n, f32* pos, f32* dist);         // 0x801C26A8
s32  fn_801C2C60(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist);                // 0x801C2C60
s32  fn_801C3074(f32 r, u8* a, u8* b, u8* c);                                             // 0x801C3074
s32  fn_801C3170(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist);                // 0x801C3170

extern u8  lbl_803839F0[];   // 0x803839F0 (vtable)
extern u8  lbl_80383B60[];   // 0x80383B60 (vtable)
extern u8  lbl_805F95F8[];   // 0x805F95F8 (s32 flag)
extern f32 lbl_805FFAB8;     // 0x805FFAB8

typedef s32 (*ShapeFn)(u8*, u8*);
typedef void (*VFn0)(u8*);
typedef void (*VFn1)(u8*, s32);

// Copies a hit (position vec3 + distance) into a result record.
static void StoreHit(u8* dst, const f32* pos, f32 dist) {
    *(f32*)(dst + 0x0) = pos[0];
    *(f32*)(dst + 0x4) = pos[1];
    *(f32*)(dst + 0x8) = pos[2];
    *(f32*)(dst + 0x18) = dist;
}

// 0x801B00EC
// Does this shape overlap the other (dispatch on the other shape type)? Type 0 defers to its virtual (+0x1C).
s32 fn_801B00EC(u8* arg0, u8* arg1) {
    f32 sp14[3];
    f32 sp8[3];
    switch (*(s32*)(arg1 + 0xC)) {
    case 4:
        return fn_801C08CC(arg0 + 0x14, arg0 + 0x3C, arg1 + 0x14, arg1 + 0x3C) != 0;
    case 7:
        return fn_801C098C(arg1 + 0x14, arg1 + 0x6C, arg1 + 0x3C, arg0 + 0x14, arg0 + 0x3C, sp14, sp8);
    case 0:
        return (*(ShapeFn*)(*(u8**)arg1 + 0x1C))(arg1, arg0);
    default:
        return 0;
    }
}

// 0x801B0D7C
// Same overlap test with the shape roles swapped (type 0 defers to its virtual +0x1C).
s32 fn_801B0D7C(u8* arg0, u8* arg1) {
    f32 sp14[3];
    f32 sp8[3];
    switch (*(s32*)(arg1 + 0xC)) {
    case 0:
        return (*(ShapeFn*)(*(u8**)arg1 + 0x1C))(arg1, arg0);
    case 4:
        return fn_801C098C(arg0 + 0x14, arg0 + 0x6C, arg0 + 0x3C, arg1 + 0x14, arg1 + 0x3C, sp14, sp8);
    case 7:
        return fn_801C10B4(arg0 + 0x14, arg0 + 0x6C, arg0 + 0x3C, arg1 + 0x14, arg1 + 0x6C, arg1 + 0x3C);
    default:
        return 0;
    }
}

// 0x801B0E34
// Constructor: sets base vtable (calling its virtual +0xC when the flag is set), then the derived
// vtable and zeroes the two vec3s at +0x3C and +0x48.
u8* fn_801B0E34(u8* arg0) {
    f32 v;
    *(u8**)arg0 = lbl_80383B60;
    if (*(s32*)lbl_805F95F8 != -1) {
        (*(VFn0*)(*(u8**)arg0 + 0xC))(arg0);
    }
    *(u8**)arg0 = lbl_803839F0;
    if (*(s32*)lbl_805F95F8 != -1) {
        v = lbl_805FFAB8;
        *(f32*)(arg0 + 0x3C) = v;
        *(f32*)(arg0 + 0x40) = v;
        *(f32*)(arg0 + 0x44) = v;
        *(f32*)(arg0 + 0x48) = v;
        *(f32*)(arg0 + 0x4C) = v;
        *(f32*)(arg0 + 0x50) = v;
    }
    return arg0;
}

// 0x801B0ED4
// Sweep test against the other shape; on a hit with count > 0 writes the hit into arg3 (if arg6) or arg1.
s32 fn_801B0ED4(u8* arg0, u8* arg1, u8* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6) {
    f32 sp60[3];
    f32 sp48[3];
    f32 sp30[3];
    f32 sp28;
    f32 sp20;
    f32 sp18;
    s32 sp10;
    s32 spC;
    s32 sp8;
    switch (*(s32*)(arg2 + 0xC)) {
    case 4:
        if (fn_801C19F4(arg2 + 0x14, arg2 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &sp10, sp60, &sp28) != 0) {
            if (arg6 != 0) {
                if (sp10 > 0) StoreHit(arg3, sp60, sp28);
            } else if (sp10 > 0) {
                StoreHit(arg1, sp60, sp28);
            }
            return 1;
        }
        return 0;
    case 7:
        if (fn_801C217C(arg2 + 0x14, arg2 + 0x6C, arg2 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &spC, sp48, &sp20) != 0) {
            if (arg6 != 0) {
                if (spC > 0) StoreHit(arg3, sp48, sp20);
            } else if (spC > 0) {
                StoreHit(arg1, sp48, sp20);
            }
            return 1;
        }
        return 0;
    case 0:
        if (fn_801C2C60(*(f32*)(arg2 + 0x3C), arg2 + 0x14, arg0 + 0x3C, arg0 + 0x48, &sp8, sp30, &sp18) != 0) {
            if (arg6 != 0) {
                if (sp8 > 0) StoreHit(arg3, sp30, sp18);
            } else if (sp8 > 0) {
                StoreHit(arg1, sp30, sp18);
            }
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

// 0x801B111C
// Same sweep test without writing a hit record: returns the callee result.
s32 fn_801B111C(u8* arg0, u8* arg1) {
    f32 sp60[3];
    f32 sp48[3];
    f32 sp30[3];
    f32 sp28;
    f32 sp20;
    f32 sp18;
    s32 sp10;
    s32 spC;
    s32 sp8;
    switch (*(s32*)(arg1 + 0xC)) {
    case 4:
        return fn_801C19F4(arg1 + 0x14, arg1 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &sp10, sp60, &sp28);
    case 7:
        return fn_801C217C(arg1 + 0x14, arg1 + 0x6C, arg1 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &spC, sp48, &sp20);
    case 0:
        return fn_801C2C60(*(f32*)(arg1 + 0x3C), arg1 + 0x14, arg0 + 0x3C, arg0 + 0x48, &sp8, sp30, &sp18);
    default:
        return 0;
    }
}

// 0x801B1250
// Variant of fn_801B0ED4 using the second family of sweep routines.
s32 fn_801B1250(u8* arg0, u8* arg1, u8* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6) {
    f32 sp60[3];
    f32 sp48[3];
    f32 sp30[3];
    f32 sp28;
    f32 sp20;
    f32 sp18;
    s32 sp10;
    s32 spC;
    s32 sp8;
    switch (*(s32*)(arg2 + 0xC)) {
    case 4:
        if (fn_801C1E04(arg2 + 0x14, arg2 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &sp10, sp60, &sp28) != 0) {
            if (arg6 != 0) {
                if (sp10 > 0) StoreHit(arg3, sp60, sp28);
            } else if (sp10 > 0) {
                StoreHit(arg1, sp60, sp28);
            }
            return 1;
        }
        return 0;
    case 7:
        if (fn_801C26A8(arg2 + 0x14, arg2 + 0x6C, arg2 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &spC, sp48, &sp20) != 0) {
            if (arg6 != 0) {
                if (spC > 0) StoreHit(arg3, sp48, sp20);
            } else if (spC > 0) {
                StoreHit(arg1, sp48, sp20);
            }
            return 1;
        }
        return 0;
    case 0:
        if (fn_801C3170(*(f32*)(arg2 + 0x3C), arg2 + 0x14, arg0 + 0x3C, arg0 + 0x48, &sp8, sp30, &sp18) != 0) {
            if (arg6 != 0) {
                if (sp8 > 0) StoreHit(arg3, sp30, sp18);
            } else if (sp8 > 0) {
                StoreHit(arg1, sp30, sp18);
            }
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

// 0x801B1498
// Same as fn_801B111C using the second family of sweep routines (type 0 uses the 4-arg form).
s32 fn_801B1498(u8* arg0, u8* arg1) {
    f32 sp38[3];
    f32 sp20;
    f32 sp18;
    f32 sp10;
    s32 spC;
    s32 sp8;
    switch (*(s32*)(arg1 + 0xC)) {
    case 4:
        return fn_801C1E04(arg1 + 0x14, arg1 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &spC, sp38, &sp18);
    case 7:
        return fn_801C26A8(arg1 + 0x14, arg1 + 0x6C, arg1 + 0x3C, arg0 + 0x3C, arg0 + 0x48, &sp8, &sp20, &sp10);
    case 0:
        return fn_801C3074(*(f32*)(arg1 + 0x3C), arg1 + 0x14, arg0 + 0x3C, arg0 + 0x48);
    default:
        return 0;
    }
}

// 0x801B154C
// Calls fn(obj, arg) on every object of every group whose key (+8) matches; a zero result stops that group.
void fn_801B154C(u8* arg0, u32 arg1, ShapeFn arg2, u8* arg3) {
    u32 i, j;
    u8* grp;
    u8* obj;
    if (arg2 == NULL) return;
    for (i = 0; i < *(u32*)(arg0 + 0x10); i++) {
        grp = *(u8**)(*(u8**)(arg0 + 0xC) + i * 4);
        if (grp != NULL && arg1 == *(u32*)(grp + 0x8) && arg2 != NULL) {
            for (j = 0; j < *(u32*)(grp + 0x28); j++) {
                obj = *(u8**)(*(u8**)(grp + 0x24) + j * 4);
                if (obj != NULL) {
                    if (arg2(obj, arg3) == 0) break;
                }
            }
        }
    }
}

// 0x801B1668
// Frees every group of the container: its member buffers, the member table, then the group itself.
void fn_801B1668(u8* arg0) {
    u32 i, j;
    u8* grp;
    u8* p;
    for (i = 0; i < *(u32*)(arg0 + 0x10); i++) {
        grp = *(u8**)(*(u8**)(arg0 + 0xC) + i * 4);
        if (grp != NULL) {
            for (j = 0; j < *(u32*)(grp + 0x28); j++) {
                p = *(u8**)(*(u8**)(grp + 0x24) + j * 4);
                if (p != NULL) BufferFree(p);
            }
            p = *(u8**)(grp + 0x24);
            if (p != NULL) BufferFree(p);
            *(u8**)(grp + 0x24) = NULL;
            *(u32*)(grp + 0x28) = 0;
            BufferFree(*(u8**)(*(u8**)(arg0 + 0xC) + i * 4));
        }
    }
}

// 0x801B1778
// Calls each member virtual +0x10(arg1) and clears bit 2 of its group flags; then clears bit 14 of the flags of the object at (+8)->+4.
void fn_801B1778(u8* arg0, s32 arg1) {
    u32 i, j;
    u8* grp;
    u8* obj;
    u8* t;
    for (i = 0; i < *(u32*)(arg0 + 0x10); i++) {
        grp = *(u8**)(*(u8**)(arg0 + 0xC) + i * 4);
        if (grp != NULL) {
            for (j = 0; j < *(u32*)(grp + 0x28); j++) {
                obj = *(u8**)(*(u8**)(grp + 0x24) + j * 4);
                if (obj != NULL) {
                    (*(VFn1*)(*(u8**)obj + 0x10))(obj, arg1);
                    *(u32*)(grp + 0x20) &= 0xFFFFFFFBu;
                }
            }
        }
    }
    t = *(u8**)(*(u8**)(arg0 + 0x8) + 0x4);
    *(u32*)(t + 0x20) &= 0xFFFFBFFFu;
}

}  // extern "C"
