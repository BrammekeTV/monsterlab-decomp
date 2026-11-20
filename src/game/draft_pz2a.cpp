// Small functions in 0x8019xxxx-0x801Fxxxx: screen-set helpers, UI animation track lookup,
// stream/buffer cursor helpers and object reset/free helpers.
#include "types.h"

// Globals owned by other files (C++ linkage, as in the files that define them).
extern "C++" void* gScreenSets;     // 0x805FB654
extern void* lbl_805FB670;          // 0x805FB670
extern void* lbl_805FBA18;          // 0x805FBA18 (callback pointer)
extern u8 lbl_803864E0[];           // 0x803864E0 (table, stored as an object's first word)
extern const char lbl_80383820[];   // 0x80383820 (debug string)
extern f32 lbl_80600290;            // 0x80600290 (float default)
extern f32 lbl_805FFB68;            // 0x805FFB68 (float zero)

// C++ linkage, as in src/engine/small_8019.cpp where it is defined.
BOOL fn_801982A8(void* obj, u32 mask);  // 0x801982A8

extern "C" {
void    fn_8018D2B8(const char* fmt, ...);                      // 0x8018D2B8 (debug log)
void*   ScreenSetsCurrent(void* sets);                          // 0x8018D1E4
u8*     fn_80198E20(u8* set);                                   // 0x80198E20 (leaf declares s32; returns a pointer)
void    fn_8019BFF8(void* obj, void* cb, void* arg);            // 0x8019BFF8
void    fn_80199828(void);                                      // 0x80199828 (callback; signature unknown)
s32     fn_8019CCAC(void);                                      // 0x8019CCAC
void    UIPlayAnim(void* el, u32 anim);                         // 0x8019BE40
s32     fn_8019555C(u8* list);                                  // 0x8019555C (leaf)
u32     fn_8019552C(u8* list, s32 index);                       // 0x8019552C (leaf)
f32     fn_801AA728(u8* entry);                                 // 0x801AA728 (leaf)
s32     fn_801B7F40(u8* arg0, u8* arg1);                        // 0x801B7F40 (return type unknown)
void    fn_801C9260(u8* node, u8* owner, f32 t);                // 0x801C9260 (arg order from registers)
s32     fn_801EE394(u8* arg0);                                  // 0x801EE394 (return type unknown)
void    BufferFree(void* p);                                    // 0x801D4980
void    operator_delete(void* p);                               // 0x801D426C
u8*     fn_801FA7CC(u8* p, s32 a4);                             // 0x801FA7CC
u8*     fn_801FA82C(u8* p, s32 del);                            // 0x801FA82C (deleting dtor)
}

typedef void (*VoidFn)(void);
typedef u8* (*GetObjFn)(u8* obj);

extern "C" {

// 0x80199738
// Starts the screen-set transition animation on arg0 when both screen sets are ready and
// the current screen differs from the pending one.
void fn_80199738(u8* arg0) {
    if (*(s32*)((u8*)gScreenSets + 0x70) != 0) return;
    if (!(*(s32*)(arg0 + 0x64) & 2)) return;
    u8* other = *(u8**)((u8*)gScreenSets + 0x7C);
    if (other == NULL) return;
    if (!(*(s32*)(other + 0x64) & 2)) return;
    fn_8019BFF8(arg0, (void*)fn_80199828, (u8*)gScreenSets + 0x70);
    if (*(s32*)((u8*)gScreenSets + 0x70) == 0) return;

    if (fn_8019CCAC() == 0) {
        s32 pending = *(s32*)(*(u8**)((u8*)gScreenSets + 0x70) + 0xBC);
        if (pending == 0) return;
        u8* cur = fn_80198E20((u8*)ScreenSetsCurrent(gScreenSets));
        if (*(s32*)(cur + 0x144) == pending) return;
    }

    if (fn_801982A8(lbl_805FB670, 0x40) == 0) return;
    u8* el = *(u8**)((u8*)gScreenSets + 0x70);
    if (*(s32*)(el + 0x64) & 1) {
        UIPlayAnim(el, 0x43D82AB2U);
    } else {
        UIPlayAnim(el, 0x045B4317U);
    }
}

// 0x801AA85C
// Stores the track's value and source pointer; when the flag byte at +0x1C is set, copies
// four floats from the source into +0x20..+0x2C.
s32 fn_801AA85C(u8* arg0, u8* arg1, s32 arg2) {
    *(s32*)(arg0 + 0x14) = arg2;
    *(void**)(arg0 + 0x18) = arg1;
    if (*(u8*)(arg0 + 0x1C) != 0) {
        *(f32*)(arg0 + 0x20) = *(f32*)(arg1 + 0x0);
        *(f32*)(arg0 + 0x24) = *(f32*)(arg1 + 0x4);
        *(f32*)(arg0 + 0x28) = *(f32*)(arg1 + 0x8);
        *(f32*)(arg0 + 0x2C) = *(f32*)(arg1 + 0xC);
    }
    return 0;
}

// 0x801AA898
// Finds the first key frame in the track's list whose value equals farg0. Returns NULL
// after logging when the list is empty; returns NULL when a key is past farg0 first.
u8* fn_801AA898(u8* arg0, s32 a4, f32 farg0) {
    u8* list = *(u8**)(arg0 + 0x4);
    if (list == NULL) {
        fn_8018D2B8(lbl_80383820, a4);
        return NULL;
    }
    s32 count = fn_8019555C(list);
    u8* entry = NULL;
    s32 i = 0;
    while (i < count) {
        entry = (u8*)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x4), i);
        if (farg0 == fn_801AA728(entry)) break;
        if (fn_801AA728(entry) > farg0) {
            entry = NULL;
            break;
        }
        i++;
    }
    if (i < count) return entry;
    return NULL;
}

// 0x801B7E7C
// Dispatches on the kind byte at +0x7F. Kind 0x20 forwards to fn_801B7F40; kinds 0x8 and 0x10
// return the stored value at +0x10, and kind 0x10 also updates the distance at +0xC from the
// object reached through arg1's vtable slot 0xC. Kind 0x40 returns 3, others return 0.
s32 fn_801B7E7C(u8* arg0, u8* arg1) {
    u8 kind = *(u8*)(arg0 + 0x7F);
    switch (kind) {
    case 0x20:
        return fn_801B7F40(arg0, arg1);
    case 0x10:
    case 0x8:
        if (kind == 0x10) {
            u8* obj = (*(GetObjFn*)(*(u8**)arg1 + 0xC))(arg1);
            f32 dist = *(f32*)(arg0 + 0x0) - *(f32*)(obj + 0x30);
            *(f32*)(arg0 + 0xC) = dist;
            if (lbl_805FFB68 == dist) return 3;
        }
        return *(s32*)(arg0 + 0x10);
    case 0x40:
        return 3;
    default:
        return 0;
    }
}

// 0x801C9B60
// Walks the linked list of entries of arg0's owner (head at owner+0x14, next link stored
// 4 bytes before each entry) and calls fn_801C9260 on entries whose id matches arg0's
// field at +0x24. Does nothing once the flag at +0x20 of arg0's sub-object is set.
void fn_801C9B60(u8* arg0, f32 farg0) {
    if (*(u8*)(*(u8**)(arg0 + 0x1C) + 0x20) != 0) return;
    u8* head = *(u8**)(*(u8**)(arg0 + 0x18) + 0x14);
    u8* node = (head != NULL) ? head + 8 : NULL;
    while (node != NULL) {
        u8* link = *(u8**)(node - 4);
        u8* next = (link != NULL) ? link + 8 : NULL;
        if (*(s32*)(arg0 + 0x24) == *(s32*)(node + 0x7C)) {
            fn_801C9260(node, arg0, farg0);
        }
        node = next;
    }
}

// 0x801EE334
// Advances the cursor at +0xC by up to arg1 bytes, clamped to the end at +0x10, and moves
// the position at +0x8 by the same amount. Returns the position before the advance, or 0
// when fewer than arg1 bytes remain.
s32 fn_801EE334(u8* arg0, s32 arg1) {
    s32 n = arg1;
    if ((u32)(*(s32*)(arg0 + 0x10) - *(s32*)(arg0 + 0xC)) < (u32)n) {
        return 0;
    }
    s32 pos = *(s32*)(arg0 + 0xC);
    u32 limit = *(u32*)(arg0 + 0x10);
    s32 old8 = *(s32*)(arg0 + 0x8);
    if ((u32)(pos + n) > limit) {
        n = (s32)(limit - (u32)pos);
    }
    *(s32*)(arg0 + 0xC) = pos + n;
    *(s32*)(arg0 + 0x8) = old8 + n;
    return old8;
}

// 0x801EE46C
// Resets the cursor state: calls fn_801EE394 and, if it returned 0, clears +0x4 and +0x8;
// always clears +0x0 and +0xC. Returns the call's result.
s32 fn_801EE46C(u8* arg0) {
    s32 r = fn_801EE394(arg0);
    if (r == 0) {
        *(s32*)(arg0 + 0x4) = 0;
        *(s32*)(arg0 + 0x8) = 0;
    }
    *(s32*)(arg0 + 0x0) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    return r;
}

// 0x801EE4B8
// Frees the buffer at +0x4 (if any) and clears the pointer.
void fn_801EE4B8(u8* arg0) {
    u8* buf = *(u8**)(arg0 + 0x4);
    if (buf != NULL) {
        BufferFree(buf);
        *(s32*)(arg0 + 0x4) = 0;
    }
}

// 0x801F7920
// Releases the two buffers held by the object (at +0x120 and +0x10C), clears the fields
// that describe them, runs fn_801FA82C on the object and frees it when arg1 > 0.
// Returns arg0.
u8* fn_801F7920(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* buf = *(u8**)(arg0 + 0x120);
        if (buf != NULL) {
            BufferFree(buf);
            *(s32*)(arg0 + 0x120) = 0;
        }
        *(s32*)(arg0 + 0x110) = 0;
        *(s32*)(arg0 + 0x114) = 0;
        *(s32*)(arg0 + 0x118) = 0;
        *(s32*)(arg0 + 0x11C) = 0;

        buf = *(u8**)(arg0 + 0x10C);
        if (buf != NULL) {
            BufferFree(buf);
            *(s32*)(arg0 + 0x10C) = 0;
        }
        *(s32*)(arg0 + 0xFC) = 0;
        *(s32*)(arg0 + 0x100) = 0;
        *(s32*)(arg0 + 0x104) = 0;
        *(s32*)(arg0 + 0x108) = 0;

        fn_801FA82C(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x801FAAE0
// Constructor: runs the base constructor fn_801FA7CC on arg0, then sets the vtable-like
// table pointer at +0x0 and two float fields (+0xAC, +0xB4) to lbl_80600290 and two int
// fields (+0xB0, +0xB8) to 40. Clears +0xA8.
void fn_801FAAE0(u8* arg0, s32 arg1) {
    fn_801FA7CC(arg0, arg1);
    *(s32*)(arg0 + 0xA8) = 0;
    *(void**)(arg0 + 0x0) = (void*)lbl_803864E0;
    *(f32*)(arg0 + 0xAC) = lbl_80600290;
    *(s32*)(arg0 + 0xB0) = 0x28;
    *(f32*)(arg0 + 0xB4) = lbl_80600290;
    *(s32*)(arg0 + 0xB8) = 0x28;
}

// 0x801FAB3C
// Deleting destructor: base cleanup fn_801FA82C with del 0, then operator_delete when arg1 > 0.
// Returns arg0.
u8* fn_801FAB3C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_801FA82C(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x801FB0D8
// Calls the optional callback in lbl_805FBA18 when it is set.
void fn_801FB0D8(void) {
    if (lbl_805FBA18 != NULL) {
        ((VoidFn)lbl_805FBA18)();
    }
}

}  // extern "C"
