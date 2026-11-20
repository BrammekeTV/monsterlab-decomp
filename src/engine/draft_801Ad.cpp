// UI slider / entity element functions in 0x801A2Fxx-0x801A44xx.
#include "types.h"

typedef void (*VFn)(void*);
typedef void (*VFnI)(void*, s32);
typedef void (*VFnII)(void*, s32, s32);
typedef void (*VFnF)(void*, f32);
typedef u8* (*VFnP)(void*);

extern "C" {
void   CameraSetMatrix(void* camera, const f32 mtx[3][4]);   // 0x801D6814
void   fn_80154294(void);                                    // 0x80154294
void   fn_8016FB20(void*);                                   // 0x8016FB20
void   fn_80170044(void*);                                   // 0x80170044
s32    fn_8018BDE0(u8* p, s32 key);                          // 0x8018BDE0
void   fn_8018D2B8(const char* fmt, ...);                    // 0x8018D2B8 (debug log)
void   fn_8018D370(const char* fmt, ...);                    // 0x8018D370 (debug log)
u32    fn_8019552C(u8* list, s32 index);                     // 0x8019552C (leaf)
s32    fn_8019555C(u8* list);                                // 0x8019555C (leaf)
void   fn_80195564(u8* list, void* item);                     // 0x80195564 (list append)
void   fn_80195570(u8* list, s32 index);                     // 0x80195570 (list remove)
s32    fn_8019600C(void* gx);                                // 0x8019600C
s32 fn_80196678(void* gx, void* mat, f32 t);                // 0x80196678
s32    fn_80196B10(u8* a3);                                  // 0x80196B10 (leaf)
s32 fn_8019B6CC(u8* arg0, f32 farg0);                        // 0x8019B6CC (UI element base update)
void   fn_8019C348(void* obj);                               // 0x8019C348
u8*    fn_8019F9B8(s32 kind);                                // 0x8019F9B8 (UI element factory)
f32    fn_801A0DF0(u8* a3);                                  // 0x801A0DF0 (leaf)
f32    fn_801A0DF8(u8* a3);                                  // 0x801A0DF8 (leaf)
f32    fn_801A0E08(u8* a3);                                  // 0x801A0E08 (leaf)
void   fn_801A0E10(void* obj, f32* box);                     // 0x801A0E10
s32    fn_801A11C4(void* obj, f32* out);                     // 0x801A11C4
s32    fn_801A413C(u8* arg0);                                // 0x801A413C
u8*    fn_801B70F8(u8* a3, s32 a4);                          // 0x801B70F8
void*  fn_801B7198(void* anim, s32 a);                       // 0x801B7198
void   fn_801B72A0(void* anim, void* child);                 // 0x801B72A0
const char* fn_801EB708(u32 hash);                           // 0x801EB708 (hash -> name for logs)

extern const char lbl_80382F80[];   // 0x80382F80 ("UIEntity.cpp")
extern const char lbl_80383190[];   // 0x80383190 (string)
extern const char lbl_803831B8[];   // 0x803831B8 (string)
extern const char lbl_803831F0[];   // 0x803831F0 (string)
extern const char lbl_8038321C[];   // 0x8038321C (string)
extern const char lbl_80383248[];   // 0x80383248 (string)
extern u8 lbl_8047C640[];           // 0x8047C640 (camera)
extern "C++" void* gScreenSets;           // 0x805FB654
struct View;
extern "C++" View* gMainView;             // 0x805FB75C
extern void* lbl_805FB658;          // 0x805FB658 (global render object)
extern f32 lbl_805FF9EC;            // 0x805FF9EC (0.999999f)

// 0x801A2F78
// If the element has a child at +0x140: draws it into the main view with the screen-set camera and
// calls its first-vtable slot 0x34. Returns 0.
s32 fn_801A2F78(u8* arg0) {
    if (fn_801B7198(arg0 + 0x140, 0) != NULL) {
        fn_8016FB20(gMainView);
        fn_80154294();
        *(s32*)((u8*)gMainView + 0x188) = 1;
        CameraSetMatrix(lbl_8047C640, (const f32(*)[4])((u8*)gScreenSets + 0x3C));
        void* child = fn_801B7198(arg0 + 0x140, 0);
        (*(VFn*)(*(u8**)child + 0x34))(child);
        fn_80170044(gMainView);
        fn_8019600C(lbl_805FB658);
    }
    return 0;
}

// 0x801A3010
// Swaps the child at +0x140: releases the old one (slot 0x3C, flag cleared via fn_801B70F8), then
// readies the new one (slot 0x38, fn_801B70F8 with 1) and stores it. Returns 0.
s32 fn_801A3010(void* obj, void* data) {
    u8* arg0 = (u8*)obj;
    u8* arg1 = (u8*)data;
    if (fn_801B7198(arg0 + 0x140, 0) != NULL) {
        void* old = fn_801B7198(arg0 + 0x140, 0);
        (*(VFn*)(*(u8**)old + 0x3C))(old);
        fn_801B70F8((u8*)fn_801B7198(arg0 + 0x140, 0), 0);
    }
    if (arg1 != NULL) {
        (*(VFn*)(*(u8**)arg1 + 0x38))(arg1);
        fn_801B70F8(arg1, 1);
    }
    fn_801B72A0(arg0 + 0x140, arg1);
    return 0;
}

// 0x801A30C0
// Selects the bone/anchor hash at +0x144 on the child's model: resolves it with fn_8018BDE0 and
// logs + returns -1 when the child, its model or the hash is missing. Returns 0 on success.
s32 fn_801A30C0(void* obj, u32 arg1) {
    u8* arg0 = (u8*)obj;
    *(s32*)(arg0 + 0x144) = 0;
    void* child = fn_801B7198(arg0 + 0x140, 0);
    if (child == NULL) {
        fn_8018D2B8(lbl_80382F80 + 0x48);
    } else if ((*(VFnP*)(*(u8**)child + 0x10))(child) == NULL) {
        fn_8018D370(lbl_80382F80 + 0x88);
        return -1;
    }
    child = fn_801B7198(arg0 + 0x140, 0);
    u8* model = (*(VFnP*)(*(u8**)child + 0x10))(child);
    if (fn_8018BDE0(model, (s32)arg1) <= -1) {
        fn_8018D370(lbl_80382F80 + 0xAC, fn_801EB708(arg1));
        return -1;
    }
    *(u32*)(arg0 + 0x144) = arg1;
    return 0;
}

// 0x801A3798
// Per-frame update of a container element: base update, forwards state bits 0x10/0x20 to itself
// (second-vtable slot 0x30), then updates each child in the list at +0x15C (forwarding the bits,
// copying the 4-float rect at +0x80 when the child has bits 0x38). Returns 0.
s32 fn_801A3798(u8* arg0, f32 farg0) {
    s32 mask = 0;
    fn_8019B6CC(arg0, farg0);
    if (*(s32*)(arg0 + 0x64) & 0x10) {
        mask = 0x10;
        (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x10);
    }
    if (*(s32*)(arg0 + 0x64) & 0x20) {
        mask |= 0x20;
        (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x20);
    }
    fn_80196678(lbl_805FB658, arg0 + 0xF0, farg0);
    u8* list = *(u8**)(arg0 + 0x15C);
    s32 count = fn_8019555C(list);
    for (s32 i = 0; i < count; i++) {
        u8* ent = *(u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x15C), i);
        if (mask != 0) {
            (*(VFnI*)(*(u8**)(ent + 0x138) + 0x2C))(ent, mask);
        }
        if (*(s32*)(ent + 0x64) & 0x38) {
            *(f32*)(ent + 0x80) = *(f32*)(arg0 + 0x80);
            *(f32*)(ent + 0x84) = *(f32*)(arg0 + 0x84);
            *(f32*)(ent + 0x88) = *(f32*)(arg0 + 0x88);
            *(f32*)(ent + 0x8C) = *(f32*)(arg0 + 0x8C);
            (*(VFnF*)(*(u8**)(ent + 0x138) + 0x18))(ent, farg0);
            (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x38);
        }
    }
    fn_80196B10((u8*)lbl_805FB658);
    return 0;
}

// 0x801A3910
// Collects child pointers into the out list (arg1: count at +4, pointer array at +8): every child
// whose type is not 2, type-2 children unless flag 0x20000 is set, and (again) every child when the
// fraction at +0x140 has reached ~1.0. Returns 0.
s32 fn_801A3910(u8* arg0, u8* arg1) {
    f32 full = lbl_805FF9EC;
    s32 count = fn_8019555C(*(u8**)(arg0 + 0x15C));
    for (s32 i = 0; i < count; i++) {
        u8* ent = *(u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x15C), i);
        if (*(u32*)ent != 2u) {
            s32 n = *(s32*)(arg1 + 4);
            (*(u8***)(arg1 + 8))[n] = ent;
            *(s32*)(arg1 + 4) = n + 1;
        } else if (!(*(s32*)(arg0 + 0x64) & 0x20000)) {
            s32 n = *(s32*)(arg1 + 4);
            (*(u8***)(arg1 + 8))[n] = ent;
            *(s32*)(arg1 + 4) = n + 1;
        }
        if (*(f32*)(arg0 + 0x140) >= full) {
            s32 n = *(s32*)(arg1 + 4);
            (*(u8***)(arg1 + 8))[n] = ent;
            *(s32*)(arg1 + 4) = n + 1;
        }
    }
    return 0;
}

// 0x801A3D84
// Loads a slider graphic of the given type (0..5): creates an element, sets it up with the two
// extra args (second-vtable slot 0x44), tags it with the type, appends it to the list at +0x15C
// (types 3..5 also become the fill element at +0x160), re-lays out and shows it (slot 0x2C, 0x28).
// Returns -1 on a bad type, else 0.
s32 fn_801A3D84(u8* arg0, s32 arg1, s32 arg2, s32 arg3) {
    u8* sp8;
    if (arg1 < 0 || arg1 >= 6) {
        fn_8018D370(lbl_803831B8, arg1);
        return -1;
    }
    sp8 = fn_8019F9B8(0);
    (*(VFnII*)(*(u8**)(sp8 + 0x138) + 0x44))(sp8, arg2, arg3);
    *(s32*)sp8 = arg1;
    if (arg1 >= 3) {
        if (arg1 >= 6) {
            fn_8018D370(lbl_803831F0, arg1);
            return -1;
        }
        *(u8**)(arg0 + 0x160) = sp8;
    } else if (arg1 < 0) {
        fn_8018D370(lbl_803831F0, arg1);
        return -1;
    }
    fn_80195564(*(u8**)(arg0 + 0x15C), &sp8);
    fn_801A413C(arg0);
    (*(VFnI*)(*(u8**)(sp8 + 0x138) + 0x2C))(sp8, 0x28);
    return 0;
}

// 0x801A3E9C
// Unloads the slider graphic of the given type (0..5): finds it in the list at +0x15C, calls its
// slot 0x14 (second vtable), removes it and clears the fill element at +0x160 for types 3..5.
// Returns -1 on a bad type, else 0 (logging when no such graphic is loaded).
s32 fn_801A3E9C(u8* arg0, s32 arg1) {
    if (arg1 < 0 || arg1 >= 6) {
        fn_8018D370(lbl_803831B8, arg1);
        return -1;
    }
    s32 count = fn_8019555C(*(u8**)(arg0 + 0x15C));
    for (s32 i = 0; i < count; i++) {
        u8* ent = *(u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x15C), i);
        if ((u32)arg1 == *(u32*)ent) {
            (*(VFn*)(*(u8**)(ent + 0x138) + 0x14))(ent);
            fn_80195570(*(u8**)(arg0 + 0x15C), i);
            if (arg1 < 6 && arg1 >= 3) {
                *(s32*)(arg0 + 0x160) = 0;
            }
            return 0;
        }
    }
    fn_8018D2B8(lbl_8038321C, arg1);
    return 0;
}

// 0x801A408C
// Recomputes the progress fraction at +0x140 from itself: value = lo + frac * (hi - lo) plus the
// step at +0x154 (negated when flag is 0), clamped to [lo, hi] (+0x14C, +0x150), then mapped back
// to 0..1. Logs when the range is empty.
void fn_801A408C(u8* arg0, s32 arg1) {
    f32 lo = *(f32*)(arg0 + 0x14C);
    f32 hi = *(f32*)(arg0 + 0x150);
    f32 step = *(f32*)(arg0 + 0x154);
    f32 v = lo + *(f32*)(arg0 + 0x140) * (hi - lo);
    if (arg1 == 0) {
        step = step * -1.0f;
    }
    v = v + step;
    if (v > *(f32*)(arg0 + 0x150)) {
        v = *(f32*)(arg0 + 0x150);
    } else if (v < *(f32*)(arg0 + 0x14C)) {
        v = *(f32*)(arg0 + 0x14C);
    }
    if (v < *(f32*)(arg0 + 0x14C)) {
        v = *(f32*)(arg0 + 0x14C);
    } else if (v > *(f32*)(arg0 + 0x150)) {
        v = *(f32*)(arg0 + 0x150);
    }
    hi = *(f32*)(arg0 + 0x150);
    lo = *(f32*)(arg0 + 0x14C);
    if (hi != lo) {
        *(f32*)(arg0 + 0x140) = (v - lo) / (hi - lo);
        return;
    }
    fn_8018D2B8(lbl_80383190);
}

// 0x801A413C
// Lays out every graphic in the list at +0x15C by its type at +0: sets scale (+0x74..0x7C),
// offset (+0x68..0x6C) and mode (+0x70). Types 1/2 scale from the fill element at +0x160, type 3
// and 4 also store a scale factor at +0x164, type 5 is unsupported (logged). Returns -1 on an
// unknown type, else 0 (also 0 when there is no fill element).
s32 fn_801A413C(u8* arg0) {
    u8* fill = *(u8**)(arg0 + 0x160);
    if (fill == NULL) {
        return 0;
    }
    s32 count = fn_8019555C(*(u8**)(arg0 + 0x15C));
    for (s32 i = 0; i < count; i++) {
        u8* ent = *(u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x15C), i);
        s32 type = *(s32*)ent;
        switch (type) {
        case 0:
            *(f32*)(ent + 0x74) = 1.0f;
            *(f32*)(ent + 0x78) = 1.0f;
            *(f32*)(ent + 0x7C) = 1.0f;
            *(f32*)(ent + 0x68) = 0.0f;
            *(f32*)(ent + 0x6C) = 0.0f;
            *(f32*)(ent + 0x70) = 5.0f;
            break;
        case 1: {
            f32 sy = *(f32*)(fill + 0x78);
            *(f32*)(ent + 0x78) = sy;
            f32 h = fn_801A0E08(ent);
            f32 w = fn_801A0DF0(ent);
            *(f32*)(ent + 0x74) = ((1.0f / sy) * w) / h;
            *(f32*)(ent + 0x7C) = 1.0f;
            if (*(s32*)(arg0 + 0x160) != 0) {
                *(f32*)(ent + 0x68) = 1.0f + fn_801A0DF8(ent);
            } else {
                *(f32*)(ent + 0x68) = 0.0f;
            }
            *(f32*)(ent + 0x6C) = 0.0f;
            *(f32*)(ent + 0x70) = 3.0f;
            break;
        }
        case 2: {
            f32 sy = *(f32*)(fill + 0x78);
            *(f32*)(ent + 0x78) = sy;
            f32 h = fn_801A0E08(ent);
            f32 w = fn_801A0DF0(ent);
            *(f32*)(ent + 0x74) = ((1.0f / sy) * w) / h;
            *(f32*)(ent + 0x7C) = 1.0f;
            if (*(s32*)(arg0 + 0x160) != 0) {
                *(f32*)(ent + 0x68) = fn_801A0DF8(*(u8**)(arg0 + 0x160)) - 1.0f;
            } else {
                *(f32*)(ent + 0x68) = 0.0f;
            }
            *(f32*)(ent + 0x6C) = 0.0f;
            *(f32*)(ent + 0x70) = 3.0f;
            break;
        }
        case 3:
            *(f32*)(ent + 0x74) = 1.0f;
            *(f32*)(ent + 0x78) = 1.0f;
            *(f32*)(ent + 0x7C) = 1.0f;
            *(f32*)(ent + 0x68) = 0.0f;
            *(f32*)(ent + 0x6C) = 0.0f;
            *(f32*)(ent + 0x70) = 4.0f;
            *(f32*)(arg0 + 0x164) = 1.0f;
            break;
        case 5:
            *(f32*)(ent + 0x70) = 2.0f;
            fn_8018D2B8(lbl_80383248);
            break;
        case 4: {
            f32 sy = *(f32*)(arg0 + 0x78);
            *(f32*)(ent + 0x78) = sy;
            f32 k = sy / fn_801A0E08(ent);
            *(f32*)(ent + 0x74) = fn_801A0DF0(ent) * k;
            *(f32*)(ent + 0x7C) = 1.0f;
            f32 sy2 = *(f32*)(ent + 0x78);
            f32 r = sy2 / fn_801A0E08(ent);
            *(f32*)(ent + 0x74) = r;
            *(f32*)(ent + 0x78) = 1.0f;
            *(f32*)(ent + 0x7C) = 1.0f;
            *(f32*)(ent + 0x68) = 0.0f;
            *(f32*)(ent + 0x6C) = 0.0f;
            *(f32*)(ent + 0x70) = 0.0f;
            *(f32*)(arg0 + 0x164) = r;
            break;
        }
        default:
            fn_8018D370(lbl_803831F0, type);
            return -1;
        }
    }
    return 0;
}

// 0x801A4414
// Per-frame slider update: eases the shown value at +0x144 toward the target at +0x140 (speed +0x158,
// instant unless flag 0x80000), pushes it to the fill element at +0x160 (scale and rect), shows the
// element (second-vtable slot 0x2C, 0x10) and runs the container update. Returns 0.
s32 fn_801A4414(u8* arg0, f32 farg0) {
    f32 cur = *(f32*)(arg0 + 0x144);
    f32 tgt = *(f32*)(arg0 + 0x140);
    if (cur == tgt) {
        fn_801A3798(arg0, farg0);
        *(f32*)(arg0 + 0x148) = *(f32*)(arg0 + 0x140);
        return 0;
    }
    if (*(s32*)(arg0 + 0x64) & 0x80000) {
        if (cur < tgt) {
            f32 n = cur + farg0 * *(f32*)(arg0 + 0x158);
            *(f32*)(arg0 + 0x144) = n;
            if (n > tgt) {
                *(f32*)(arg0 + 0x144) = tgt;
            }
        } else if (cur > tgt) {
            f32 n = cur - farg0 * *(f32*)(arg0 + 0x158);
            *(f32*)(arg0 + 0x144) = n;
            if (n < tgt) {
                *(f32*)(arg0 + 0x144) = tgt;
            }
        }
    } else {
        *(f32*)(arg0 + 0x144) = tgt;
    }
    u8* fill = *(u8**)(arg0 + 0x160);
    if (fill != NULL) {
        *(f32*)(fill + 0x74) = *(f32*)(arg0 + 0x144) * *(f32*)(arg0 + 0x164);
        f32 out[4];
        f32 box[4];
        fn_801A11C4(*(u8**)(arg0 + 0x160), out);
        f32 v = *(f32*)(arg0 + 0x144);
        out[3] = v;
        box[0] = out[0];
        box[1] = out[1];
        box[2] = out[2];
        box[3] = v;
        fn_801A0E10(*(u8**)(arg0 + 0x160), box);
        fn_8019C348(*(u8**)(arg0 + 0x160));
    }
    (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x10);
    fn_801A3798(arg0, farg0);
    return 0;
}

}  // extern "C"
