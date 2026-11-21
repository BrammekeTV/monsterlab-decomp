// Small functions from draft batch miss481: cached texture release, timed sequence step,
// object destructors and flag walkers.
#include "types.h"

extern "C" {
void  MemFreeSmall(void* p);                                    // 0x801D4340
s32   RaySceneTest(void* ray);                                  // 0x80070458
void  TextureRelease(void* tex);                                // 0x80184028
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count); // 0x8032F2E0
void  operator_delete(void* p);                                 // 0x801D426C
u8*   fn_800216B8(u8* p, s32 del);                              // 0x800216B8 (base destructor)
u8* fn_8002B8C4(u8* p, s32 del);                              // 0x8002B8C4 (return value unused)
u8*   fn_8002B514(u8* p, s32 del);                              // 0x8002B514 (callback)
u8*   fn_8002AB4C(u8* p, s32 del);                              // 0x8002AB4C (callback)
void  fn_80031090(u8* p, s32 del);                              // 0x80031090 (base destructor)
void  fn_8006AADC(u8* p, f32 delta);                            // 0x8006AADC
u8*   fn_801B3308(void);                                        // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                              // 0x801B3378
u8*   fn_801F6238(u8* p, s32 del);                              // 0x801F6238
u8*   fn_801F94EC(u8* p, s32 del);                              // 0x801F94EC
void  fn_8032F358(void* obj, void* cb);                         // 0x8032F358
}

// Globals (C++ linkage, as defined in the other files).
extern "C++" u8*    lbl_803C5308[];      // 0x803C5308 (4 cached texture slots)
extern "C++" u8     lbl_80357528[];      // 0x80357528 (vtable of the object at p)
extern "C++" u8     lbl_80358050[];      // 0x80358050 (vtable of the object at p)
extern "C++" void*  lbl_805FAFC0;        // 0x805FAFC0
extern "C++" u8*    lbl_805FB008;        // 0x805FB008 (global object pointer)
extern "C++" s32    lbl_805FB090;        // 0x805FB090 (cache use counter)
extern "C++" void*  lbl_805FB094;        // 0x805FB094 (cached block)
extern "C++" f32    lbl_803473F0[];      // 0x803473F0 (f32 table indexed by RaySceneTest)
extern "C++" f32    lbl_805FE47C;        // 0x805FE47C (lower clamp)
extern "C++" f32    lbl_805FE488;        // 0x805FE488 (upper clamp)
extern "C++" f32    lbl_805FE668;        // 0x805FE668 (float constant used as zero)
extern "C++" void*  fn_800385A8(u8* p, s32 del);                // 0x800385A8 (C++ linkage)

// Walks the linked list at p+0xB0 (node: +4 entry, +8 next; entry: +4 object) and sets or
// clears bit 0 of each object's flags at +0x20.
static void SetListObjBit0(u8* p, s32 on) {
    u8* node = *(u8**)(p + 0xB0);
    while (node != NULL) {
        u8* entry = *(u8**)(node + 4);
        if (entry != NULL) {
            u8* obj = *(u8**)(entry + 4);
            if (on) {
                *(s32*)(obj + 0x20) = *(s32*)(obj + 0x20) | 1;
            } else {
                *(s32*)(obj + 0x20) = *(s32*)(obj + 0x20) & ~1;
            }
        }
        node = *(u8**)(node + 8);
    }
}

extern "C" {

// 0x80044FA4
// Drops one use of the cached texture set; on the last use releases the four textures in
// lbl_803C5308 that reach zero references and frees the cached block.
void fn_80044FA4(void) {
    lbl_805FB090 = lbl_805FB090 - 1;
    if (lbl_805FB090 != 0) {
        return;
    }
    void* block = lbl_805FB094;
    if (block == NULL) {
        return;
    }
    for (s32 i = 0; i < 4; i++) {
        u8* tex = lbl_803C5308[i];
        if (tex != NULL) {
            s32 refs = *(s32*)(tex + 0x24) - 1;
            *(s32*)(tex + 0x24) = refs;
            if (refs == 0) {
                TextureRelease(tex);
            }
            lbl_803C5308[i] = NULL;
        }
    }
    operator_delete(block);
}

// 0x800C8900
// Adds delta times the table entry for RaySceneTest(p) to the float at +0 and clamps the result
// to [lbl_805FE47C, lbl_805FE488].
void fn_800C8900(u8* p, f32 delta) {
    f32 v = *(f32*)p + delta * lbl_803473F0[RaySceneTest(p)];
    f32 out;
    if (v < lbl_805FE47C) {
        out = lbl_805FE47C;
    } else if (v > lbl_805FE488) {
        out = lbl_805FE488;
    } else {
        out = v;
    }
    *(f32*)p = out;
}

// 0x800D9044
// Advances a timed sequence at p by delta. +0x18 is the time scale, +0x1C the running total
// (clamped to +0x20), +0x8 the entry array (0x1C bytes each, duration at +0x18), +0xC the entry
// count, +0x10 the current entry index and +0x14 the time spent in that entry.
void fn_800D9044(u8* p, f32 delta) {
    f32 rem = delta * *(f32*)(p + 0x18);
    f32 total = *(f32*)(p + 0x1C) + rem;
    *(f32*)(p + 0x1C) = total;
    if (total > *(f32*)(p + 0x20)) {
        *(f32*)(p + 0x1C) = *(f32*)(p + 0x20);
    }
    while (rem > lbl_805FE668) {
        u32 idx = *(u32*)(p + 0x10);
        f32 elapsed = *(f32*)(p + 0x14) + rem;
        *(f32*)(p + 0x14) = elapsed;
        f32 dur = *(f32*)(*(u8**)(p + 0x8) + idx * 0x1C + 0x18);
        if (elapsed > dur) {
            u32 count = *(u32*)(p + 0xC);
            if (idx == count - 1) {
                *(f32*)(p + 0x14) = dur;
                return;
            }
            rem = elapsed - dur;
            *(u32*)(p + 0x10) = idx + 1;
            *(f32*)(p + 0x14) = lbl_805FE668;
        } else {
            rem = lbl_805FE668;
        }
    }
}

// 0x8019CE58
// Calls the function pointer at +0x2C of the object stored at p+0x138 with the value *arg.
// Returns -2 when p is null, otherwise 0.
s32 fn_8019CE58(u8* p, void* arg) {
    if (p == NULL) {
        return -2;
    }
    void* fn = *(void**)(*(u8**)(p + 0x138) + 0x2C);
    ((void (*)(u8*, s32))fn)(p, *(s32*)arg);
    return 0;
}

// 0x801C7E0C
// Sets (on != 0) or clears bit 0x2 of the flags at p+0, then sets or clears bit 0x8 of +0x98 on
// every entry of the linked list reached from p+0x60 (entry link at -4 from the entry body).
void fn_801C7E0C(u8* p, s32 on) {
    if (on != 0) {
        *(s32*)p = *(s32*)p | 2;
    } else {
        *(s32*)p = *(s32*)p & ~2;
    }
    u8* head = *(u8**)(p + 0x60);
    u8* first = *(u8**)head;
    u8* obj = (first != NULL) ? first + 8 : NULL;
    while (obj != NULL) {
        u8* nextLink = *(u8**)(obj - 4);
        if (on != 0) {
            *(s32*)(obj + 0x98) = *(s32*)(obj + 0x98) & ~8;
        } else {
            *(s32*)(obj + 0x98) = *(s32*)(obj + 0x98) | 8;
        }
        obj = (nextLink != NULL) ? nextLink + 8 : NULL;
    }
}

// 0x8021F3F0
// Destructor: frees the buffer at +0x8 and frees the object itself when del > 0. Returns p.
u8* fn_8021F3F0(u8* p, s32 del) {
    if (p != NULL) {
        MemFreeSmall(*(void**)(p + 0x8));
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800B6338
// Marks the list objects at p+0xB0 with bit 0, runs the vtable update at +0x74 with delta, then clears the bits again.
void fn_800B6338(u8* p, f32 delta) {
    SetListObjBit0(p, 1);
    fn_8006AADC(p, delta);
    (*(void (**)(u8*, f32))(*(u8**)p + 0x74))(p, delta);
    SetListObjBit0(p, 0);
}

// 0x800B7BA8
// Same as fn_800B6338 (list bit 0 marking around a vtable update at +0x74).
void fn_800B7BA8(u8* p, f32 delta) {
    SetListObjBit0(p, 1);
    fn_8006AADC(p, delta);
    (*(void (**)(u8*, f32))(*(u8**)p + 0x74))(p, delta);
    SetListObjBit0(p, 0);
}

// 0x800B8224
// Same as fn_800B6338 (list bit 0 marking around a vtable update at +0x74).
void fn_800B8224(u8* p, f32 delta) {
    SetListObjBit0(p, 1);
    fn_8006AADC(p, delta);
    (*(void (**)(u8*, f32))(*(u8**)p + 0x74))(p, delta);
    SetListObjBit0(p, 0);
}

// 0x800B907C
// Same as fn_800B6338 (list bit 0 marking around a vtable update at +0x74).
void fn_800B907C(u8* p, f32 delta) {
    SetListObjBit0(p, 1);
    fn_8006AADC(p, delta);
    (*(void (**)(u8*, f32))(*(u8**)p + 0x74))(p, delta);
    SetListObjBit0(p, 0);
}

// 0x800CC32C
// Same as fn_800B6338 (list bit 0 marking around a vtable update at +0x74).
void fn_800CC32C(u8* p, f32 delta) {
    SetListObjBit0(p, 1);
    fn_8006AADC(p, delta);
    (*(void (**)(u8*, f32))(*(u8**)p + 0x74))(p, delta);
    SetListObjBit0(p, 0);
}

// 0x80028288
// Destructor for the object at p: resets its vtable and sub-object pointers, runs the base
// destructor and frees the object when del > 0. Returns p.
u8* fn_80028288(u8* p, s32 del) {
    if (p != NULL) {
        *(void**)(p + 0x4) = lbl_80357528;
        lbl_805FAFC0 = NULL;
        fn_8032F358(*(void**)(p + 0xA4), (void*)fn_8002B514);
        fn_8032F358(*(void**)(p + 0xA8), (void*)fn_8002AB4C);
        fn_8002B8C4(*(u8**)(p + 0xA0), 1);
        fn_80031090(*(u8**)(p + 0xF8), 1);
        *(s32*)(p + 0x90) = 0;
        *(s32*)(p + 0x98) = 0;
        fn_800216B8(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800389D4
// Destructor for the large object at p: resets its vtable, releases the two list sub-objects,
// destroys the 0x28-byte entry array at +0x5EC, runs the base destructor and frees the object
// when del > 0. Returns p.
u8* fn_800389D4(u8* p, s32 del) {
    if (p != NULL) {
        *(void**)(p + 0x4) = lbl_80358050;
        fn_801B3378(fn_801B3308(), p + 0xA4);
        fn_801B3378(fn_801B3308(), p + 0x160);
        lbl_805FB008 = NULL;
        __destroy_arr(p + 0x5EC, (void (*)(void*, s32))fn_800385A8, 0x28, 4);
        fn_801F94EC(p + 0x160, -1);
        fn_801F6238(p + 0xA4, -1);
        fn_800216B8(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

}  // extern "C"
