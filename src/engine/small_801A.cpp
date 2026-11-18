// Small UI engine functions in 0x801Axxxx.
#include "types.h"
#include "revolution.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFn)(void*);
typedef void (*VFnU)(void*, u32);

extern "C" {
void    operator_delete(void* p);                                // 0x801D426C
void    fn_8018D2B8(const char* fmt, ...);                       // 0x8018D2B8 (debug log)
void    fn_8018D4CC(void* p);                                    // 0x8018D4CC (free)
s32     fn_801954E4(void* p);                                    // 0x801954E4 (frees child at +0x10, then obj)
void*   fn_8019B008(void* obj, char* name, u32 arg);                                   // 0x8019B008
u32     fn_801A65A0(void* obj);                                  // 0x801A65A0
u32     fn_801A665C(void* obj);                                  // 0x801A665C
}

extern "C++" void* gScreenSets;                       // 0x805FB654
extern u8    lbl_805FB50C;                      // 0x805FB50C (GX shadow flags byte)
extern u8    lbl_805F9212;                      // 0x805F9212 (copy filter value)
extern const char lbl_80383190[];               // 0x80383190 (string)
extern u8    lbl_80382EA0[];                    // 0x80382EA0 (vtable)
extern u8    lbl_80382E2C[];                    // 0x80382E2C (vtable)

extern "C" {

// 0x801A01A4
// Destroys a UI element: if +0x140 points at a record with a nonzero first word, runs the
// second-vtable slot 0x48 on it, then frees the object. Returns 0.
s32 fn_801A01A4(void* obj) {
    u8* o = (u8*)obj;
    u8* rec = *(u8**)(o + 0x140);
    if (rec != NULL && *(s32*)rec != 0) {
        (*(VFn*)(*(u8**)(o + 0x138) + 0x48))(obj);
    }
    fn_8018D4CC(obj);
    return 0;
}

// 0x801A11C4
// Copies four floats (+0x184, +0x180, +0x19C, +0x198) into out[0..3]. Returns 0, or -2 when out is NULL.
s32 fn_801A11C4(void* obj, f32* out) {
    u8* o = (u8*)obj;
    if (out == NULL) {
        return -2;
    }
    out[0] = *(f32*)(o + 0x184);
    out[1] = *(f32*)(o + 0x180);
    out[2] = *(f32*)(o + 0x19C);
    out[3] = *(f32*)(o + 0x198);
    return 0;
}

// 0x801A1ABC
// Shows the element (second-vtable slot 0x2C) when flag is nonzero, hides it (slot 0x30) when flag is 0.
// Both calls pass 0x20000 as the second argument.
void fn_801A1ABC(void* obj, s32 flag) {
    u8* o = (u8*)obj;
    if (flag != 0) {
        (*(VFnU*)(*(u8**)(o + 0x138) + 0x2C))(obj, 0x20000);
    } else {
        (*(VFnU*)(*(u8**)(o + 0x138) + 0x30))(obj, 0x20000);
    }
}

// 0x801A1AEC
// Deleting-destructor tail: frees obj when del > 0. Returns obj.
void* fn_801A1AEC(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801A1B2C
// Sets the element's vtable to lbl_80382EA0, and the vtable of the children at +0x164 and +0x168
// (when present) to lbl_80382E2C.
void fn_801A1B2C(void* obj) {
    u8* o = (u8*)obj;
    *(void**)(o + 0x138) = lbl_80382EA0;
    u8* c0 = *(u8**)(o + 0x164);
    if (c0 != NULL) {
        *(void**)(c0 + 0x138) = lbl_80382E2C;
    }
    u8* c1 = *(u8**)(o + 0x168);
    if (c1 != NULL) {
        *(void**)(c1 + 0x138) = lbl_80382E2C;
    }
}

// 0x801A1EC8
// Destroys the two child elements at +0x164 and +0x168 (second-vtable slot 0x14 each, when present),
// then frees the object. Returns 0.
s32 fn_801A1EC8(void* obj) {
    u8* o = (u8*)obj;
    for (s32 i = 0; i < 2; i++) {
        void* child = *(void**)(o + 0x164 + i * 4);
        if (child != NULL) {
            (*(VFn*)(*(u8**)((u8*)child + 0x138) + 0x14))(child);
        }
    }
    fn_8018D4CC(obj);
    return 0;
}

// 0x801A2330
// Appends the children at +0x164 and +0x168 (when present) to the list at r4
// (count at +4, pointer array at +8). Returns 0.
s32 fn_801A2330(void* obj, void* list) {
    u8* o = (u8*)obj;
    u8* l = (u8*)list;
    void* c0 = *(void**)(o + 0x164);
    if (c0 != NULL) {
        s32 n = *(s32*)(l + 0x4);
        u8* arr = *(u8**)(l + 0x8);
        *(void**)(arr + n * 4) = c0;
        *(s32*)(l + 0x4) = n + 1;
    }
    void* c1 = *(void**)(o + 0x168);
    if (c1 != NULL) {
        s32 n = *(s32*)(l + 0x4);
        u8* arr = *(u8**)(l + 0x8);
        *(void**)(arr + n * 4) = c1;
        *(s32*)(l + 0x4) = n + 1;
    }
    return 0;
}

// 0x801A2388
// Two-phase GX BP register write: clears then sets bit 0x08 of the shadow byte lbl_805FB50C, sending a
// BP word (0xFE00C000, then 0 or lbl_805F9212 << 14) after each phase. Returns 0.
s32 fn_801A2388(void) {
    u8 flags = lbl_805FB50C & ~0x08;
    lbl_805FB50C = flags;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0;
    flags = lbl_805FB50C | 0x08;
    u8 filter = lbl_805F9212;
    lbl_805FB50C = flags;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)filter << 14;
    return 0;
}

// 0x801A2548
// Deleting-destructor tail: frees obj when del > 0. Returns obj.
void* fn_801A2548(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801A26BC
// Destroys a UI element: if +0x140 is set, runs the second-vtable slot 0x48 on it, then frees the object.
// Returns 0.
s32 fn_801A26BC(void* obj) {
    u8* o = (u8*)obj;
    if (*(void**)(o + 0x140) != NULL) {
        (*(VFn*)(*(u8**)(o + 0x138) + 0x48))(obj);
    }
    fn_8018D4CC(obj);
    return 0;
}

// 0x801A270C
// Stores the result of fn_8019B008 (looked up from the current screen set's +0x10 entry) at +0x140. Returns 0.
s32 fn_801A270C(void* obj) {
    u8* sets = (u8*)gScreenSets;
    void* sub = *(void**)(sets + 0x10);
    *(void**)((u8*)obj + 0x140) = fn_8019B008(sub, 0, 0);  // r4/r5 are not set by the original; unused leftovers
    return 0;
}

// 0x801A2D9C
// Deleting-destructor tail: frees obj when del > 0. Returns obj.
void* fn_801A2D9C(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801A3710
// Deleting-destructor tail: frees obj when del > 0. Returns obj.
void* fn_801A3710(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801A3750
// Frees the child at +0x15C (via fn_801954E4, when present), then the object. Returns 0.
s32 fn_801A3750(void* obj) {
    u8* o = (u8*)obj;
    void* child = *(void**)(o + 0x15C);
    if (child != NULL) {
        fn_801954E4(child);
    }
    fn_8018D4CC(obj);
    return 0;
}

// 0x801A3FA0
// Sets the progress fraction at +0x140: t is clamped to [+0x14C, +0x150], then mapped to 0..1 across
// that range. Logs and leaves the value alone when the range is empty.
void UISetProgress(void* obj, f32 t) {
    u8* o = (u8*)obj;
    f32 lo = *(f32*)(o + 0x14C);
    f32 hi = *(f32*)(o + 0x150);
    if (t < lo) {
        t = lo;
    } else if (t > hi) {
        t = hi;
    }
    if (hi == lo) {
        fn_8018D2B8(lbl_80383190);
    } else {
        *(f32*)(o + 0x140) = (t - lo) / (hi - lo);
    }
}

// 0x801A46D8
// Deleting-destructor tail: frees obj when del > 0. Returns obj.
void* fn_801A46D8(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801A4718
// Deleting-destructor tail: frees obj when del > 0. Returns obj.
void* fn_801A4718(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801A4758
// Deleting-destructor tail: frees obj when del > 0. Returns obj.
void* fn_801A4758(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801A6518
// Returns the sign bit of (fn_801A65A0(obj) - word at +0x148), computed with the original's
// xor/shift sequence (a signed-compare idiom). Returns 0 or 1.
BOOL fn_801A6518(void* obj) {
    u32 a = fn_801A65A0(obj);
    u32 x = a ^ *(u32*)((u8*)obj + 0x148);
    s32 r4 = (s32)x >> 1;
    s32 r0 = r4 - (s32)(x & a);
    return (BOOL)((u32)r0 >> 31);
}

// 0x801A655C
// Same as fn_801A6518 with fn_801A665C and the word at +0x144.
BOOL fn_801A655C(void* obj) {
    u32 a = fn_801A665C(obj);
    u32 x = a ^ *(u32*)((u8*)obj + 0x144);
    s32 r4 = (s32)x >> 1;
    s32 r0 = r4 - (s32)(x & a);
    return (BOOL)((u32)r0 >> 31);
}

}
