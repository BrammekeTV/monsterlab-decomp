// Small functions from the miss463 batch (0x8002xxxx to 0x8020xxxx).
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

// C++ linkage globals and functions (defined in C++ translation units).
extern "C++" void* gScreenSets;                  // 0x805FB654
extern "C" void fn_802077B8(void* unused, s32 value);
extern "C" void fn_802041B0(u8* p);
extern "C++" u8* fn_80135380(u8* p, s32 del);
extern "C++" u8* fn_80135340(u8* p, s32 del);
extern "C++" u8* fn_80135300(u8* p, s32 del);
extern u8 gWorldState[];                         // 0x803E6D00 (holds a pointer at offset 0)
extern u8 gIdPool[];                             // 0x8048A8F0

extern "C" {
void  operator_delete(void* p);                  // 0x801D426C
void  MemFreeSmall(void* p);                     // 0x801D4340
void* IdPoolNext(void* pool);                    // 0x801E6EB0
u32   StringHash(const char* s);                 // 0x801EB704
char* fn_801D8ADC(char* s);                      // 0x801D8ADC
void  TextureRelease(void* tex);                 // 0x80184028
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle);  // 0x8032EFC8
u8*   fn_800DE5E4(u8* p, s32 del);               // 0x800DE5E4
void  fn_8018D4CC(void* p);                      // 0x8018D4CC
void* fn_8018E8D8(void* table, u32 hash);        // 0x8018E8D8
void  fn_80169704(void* a, void* b);             // 0x80169704
u32   fn_8019552C(u8* a3, s32 a4);               // 0x8019552C
void  fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c);  // 0x8032F1E8
void  fn_8020415C(u8* a3);                       // 0x8020415C
void  fn_8020418C(u8* a3);                       // 0x8020418C
void  fn_801ACD08(u8* a, u8* b, u8* out, f32 x, f32 y);  // 0x801ACD08
s32   fn_80109A0C(s32 id);                       // 0x80109A0C
s32   fn_801098DC(s32 id);                       // 0x801098DC
s32   NANDInit(void);                            // 0x802675BC
s32   fn_802078E8(void);                         // 0x802078E8
s32   fn_80207DCC(void);                         // 0x80207DCC
void  fn_80207734(void);                         // 0x80207734
void  fn_802E49B8(s32 a, const char* name, s32 b, void* cb0, void* cb1, f32 timeout);  // 0x802E49B8
void  fn_802E4D98(s32 id);                       // 0x802E4D98

extern u8  lbl_8035CEF8[];                       // 0x8035CEF8 (vtable)
extern u8  lbl_8035CF68[];                       // 0x8035CF68 (vtable)
extern u8  lbl_803C4298[];                       // 0x803C4298
extern u8  lbl_803E9FC8[];                       // 0x803E9FC8
extern u8  lbl_803EA0B4[];                       // 0x803EA0B4 (three floats)
extern u8  lbl_80382380[];                       // 0x80382380 (vtable)
extern u8  lbl_804FDAA0[];                       // 0x804FDAA0
extern u8  lbl_805FBB54[];                       // 0x805FBB54 (byte flag)
extern u8  lbl_8038743C[];                       // 0x8038743C (string)
extern s32 lbl_805FB368;                         // 0x805FB368
extern s32 lbl_805FB380;                         // 0x805FB380
extern s32 lbl_805FBB30;                         // 0x805FBB30
extern s32 lbl_805FBB38;                         // 0x805FBB38
extern s32 lbl_805FBB40;                         // 0x805FBB40
extern void* lbl_805FBA94;                       // 0x805FBA94
extern f32 lbl_805F991C;                         // 0x805F991C
extern f32 lbl_805FBB3C;                         // 0x805FBB3C
extern f32 lbl_805FCBA0;                         // 0x805FCBA0 (float constant, 1.0)
extern f32 lbl_805FCBA4;                         // 0x805FCBA4 (float constant, 2.0)
extern f32 lbl_805FEF18;                         // 0x805FEF18 (float constant)
extern f32 lbl_805FEF40;                         // 0x805FEF40 (float constant)
extern f32 lbl_805FFAC0;                         // 0x805FFAC0 (float constant, 1/3)
extern f32 lbl_80600348;                         // 0x80600348 (float constant)
extern f32 lbl_80600354;                         // 0x80600354 (float constant)
extern f32 lbl_80600358;                         // 0x80600358 (float constant)
extern f32 lbl_8060047C;                         // 0x8060047C (float constant)
extern f32 lbl_80600480;                         // 0x80600480 (float constant)
extern f32 lbl_806003D0;                         // 0x806003D0 (float constant)
}

// Virtual-style call: the function pointer at *(*(obj+0x138)+0x20), with obj as this.
static void CallSubObjSlot20(u8* obj, s32 arg) {
    u8* tbl = *(u8**)(obj + 0x138);
    ((void (*)(u8*, s32))(*(void**)(tbl + 0x20)))(obj, arg);
}

extern "C" {

// 0x80030F28
// Advances the 4-frame cycle counter at +0x24 and swaps the image at +0x4 to the texture for
// the new frame. The texture list depends on the kind at +0x38.
void fn_80030F28(u8* p) {
    u32 v = *(u32*)(p + 0x24) + 1u;
    u32 hi = v >> 31;
    u32 t = (v << 30) - hi;
    u32 frame = ((t << 2) | (t >> 30)) + hi;
    ScriptValue tex = 0;
    s32 kind;

    *(u32*)(p + 0x24) = frame;
    kind = *(s32*)(p + 0x38);
    if (kind == 0) {
        tex = UITextureList_GetTexture(0x4A95BAA7u, (s32)frame);
    } else if (kind == 2) {
        tex = UITextureList_GetTexture(0x33C872C9u, 0);
    } else if (kind == 3) {
        tex = UITextureList_GetTexture(0x20800DC8u, (s32)frame);
    } else if (kind == 5) {
        tex = UITextureList_GetTexture(0x20800DC8u, 1);
    }
    if (tex != 0) {
        UIImage_SetTexture(*(u32*)(*(u8**)(p + 0x4)), (BOOL)tex);
    }
}

// 0x8005BD80
// Picks the nearer of the two candidates in the world state (+0x10 and +0x14) to the point at
// p+0x4 (squared distance) and stores it at p+0x6BC. Falls back to whichever one exists.
void fn_8005BD80(u8* p) {
    u8* ws = *(u8**)gWorldState;
    u8* c = *(u8**)(ws + 0x14);
    u8* b;
    u8* pa;
    u8* pb;
    u8* pc;
    f32 bx, by, bz, cx, cy, cz, db, dc;

    if (c == 0) {
        *(void**)(p + 0x6BC) = *(void**)(ws + 0x10);
        return;
    }
    b = *(u8**)(ws + 0x10);
    if (b == 0) {
        *(u8**)(p + 0x6BC) = c;
        return;
    }
    pa = *(u8**)(p + 0x4);
    pb = *(u8**)(b + 0x4);
    pc = *(u8**)(c + 0x4);
    bx = *(f32*)(pb + 0x0) - *(f32*)(pa + 0x0);
    by = *(f32*)(pb + 0x4) - *(f32*)(pa + 0x4);
    bz = *(f32*)(pb + 0x8) - *(f32*)(pa + 0x8);
    cx = *(f32*)(pc + 0x0) - *(f32*)(pa + 0x0);
    cy = *(f32*)(pc + 0x4) - *(f32*)(pa + 0x4);
    cz = *(f32*)(pc + 0x8) - *(f32*)(pa + 0x8);
    db = (bz * bz) + ((bx * bx) + (by * by));
    dc = (cz * cz) + ((cx * cx) + (cy * cy));
    if (db < dc) {
        *(u8**)(p + 0x6BC) = b;
    } else {
        *(u8**)(p + 0x6BC) = c;
    }
}

// 0x8009E8E4
// Deleting destructor: resets the vtables of the object at p, frees the buffers it owns, runs
// the base destructor on the subobject at +0xC, and frees p when del > 0. Returns p.
u8* fn_8009E8E4(u8* p, s32 del) {
    if (p != 0) {
        *(void**)(p + 0x0) = (void*)lbl_8035CEF8;
        *(s32*)(p + 0x58) = 0;
        *(u8*)(*(u8**)(lbl_803C4298 + 0x4) + 0xFC) = 0;
        if (p + 0x98 != 0) {
            u8* q = *(u8**)(p + 0x9C);
            *(void**)(p + 0x98) = (void*)lbl_8035CF68;
            if (q != 0) {
                MemFreeSmall(q - 0x10);
            }
            *(void**)(p + 0x9C) = 0;
            if (*(u8**)(p + 0xA0) != 0) {
                MemFreeSmall(*(u8**)(p + 0xA0));
                *(void**)(p + 0xA0) = 0;
            }
            *(s32*)(p + 0xA4) = 0;
        }
        fn_800DE5E4(p + 0xC, -1);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x80135224
// Sets the three floats of lbl_803EA0B4 and registers the global destructors of the objects
// in lbl_803E9FC8 (one per 0x18-byte record, eight in all).
void fn_80135224(void) {
    u8* base = lbl_803E9FC8;

    *(f32*)(lbl_803EA0B4 + 0x0) = lbl_805FEF18;
    *(f32*)(lbl_803EA0B4 + 0x4) = lbl_805FEF40;
    *(f32*)(lbl_803EA0B4 + 0x8) = lbl_805FEF18;
    __register_global_object(base + 0xC,  (void (*)(void*))fn_80135380, base);
    __register_global_object(base + 0x24, (void (*)(void*))fn_80135380, base + 0x18);
    __register_global_object(base + 0x3C, (void (*)(void*))fn_80135380, base + 0x30);
    __register_global_object(base + 0x54, (void (*)(void*))fn_80135340, base + 0x48);
    __register_global_object(base + 0x6C, (void (*)(void*))fn_80135300, base + 0x60);
    __register_global_object(base + 0x84, (void (*)(void*))fn_80135300, base + 0x78);
    __register_global_object(base + 0x9C, (void (*)(void*))fn_80135300, base + 0x90);
    __register_global_object(base + 0xB4, (void (*)(void*))fn_80135300, base + 0xA8);
}

// 0x8019AEAC
// Looks up a child by name hash. For slots 0 and 1 it first scans the three gScreenSets
// entries by name; otherwise (or on a miss) it looks in the table at arg0 + 0x18 + 4*slot.
// Returns the found object's +0xCC sub-block, or NULL.
void* fn_8019AEAC(u8* arg0, u32 arg1, s32 arg2) {
    u32 slot;
    u8* found = NULL;
    s32 hit = 0;

    switch (arg2) {
    case 0:
        slot = 0;
        break;
    case 1:
        slot = 1;
        break;
    default:
        slot = 6;
        break;
    }
    if (slot <= 1u) {
        s32 i;
        for (i = 0; i < 3; i++) {
            u8* base = (u8*)gScreenSets;
            u8* entry = *(u8**)(base + i * 4 + 0x14);
            if (arg1 == StringHash((const char*)fn_801D8ADC((char*)(entry + 4)))) {
                found = *(u8**)(base + i * 4 + 0x14);
                hit = 1;
                break;
            }
        }
    }
    if (!hit) {
        found = (u8*)fn_8018E8D8(*(void**)(arg0 + slot * 4 + 0x18), arg1);
    }
    if (found != NULL) {
        return found + 0xCC;
    }
    return NULL;
}

// 0x801AFBBC
// Updates the transform block at arg0: copies the current position (+0x14..+0x1C) to +0x20..+0x28,
// computes the new vector through fn_801ACD08, and scales the vector from the owner at +0x8 by
// the average of the three values at +0x8..+0x10 of the record at +0x10. When arg1 != 0 it also
// stores the new position to +0x20..+0x28.
void fn_801AFBBC(u8* arg0, s32 arg1) {
    u8 frame[0x48];
    u8* t3;
    u8* t4;
    f32 f8, f7, f6, k, s, s2, m;

    *(f32*)(arg0 + 0x20) = *(f32*)(arg0 + 0x14);
    *(f32*)(arg0 + 0x24) = *(f32*)(arg0 + 0x18);
    *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x1C);
    fn_801ACD08(arg0, *(u8**)(arg0 + 0x8) + 0xC, frame + 0x18,
                *(f32*)(arg0 + 0x18), *(f32*)(arg0 + 0x14));
    f8 = *(f32*)(frame + 0x3C);
    f7 = *(f32*)(frame + 0x40);
    f6 = *(f32*)(frame + 0x44);
    t4 = *(u8**)(arg0 + 0x10);
    *(f32*)(arg0 + 0x14) = f8;
    t3 = *(u8**)(arg0 + 0x8);
    *(f32*)(arg0 + 0x18) = f7;
    *(f32*)(arg0 + 0x1C) = f6;
    k = lbl_805FFAC0;
    s = *(f32*)(t4 + 0x8) + *(f32*)(t4 + 0xC);
    s2 = *(f32*)(t4 + 0x10) + s;
    m = k * s2;
    *(f32*)(arg0 + 0x3C) = *(f32*)(t3 + 0x28) * m;
    *(f32*)(arg0 + 0x40) = *(f32*)(t3 + 0x2C) * m;
    *(f32*)(arg0 + 0x44) = *(f32*)(t3 + 0x30) * m;
    if (arg1 != 0) {
        *(f32*)(arg0 + 0x20) = f8;
        *(f32*)(arg0 + 0x24) = f7;
        *(f32*)(arg0 + 0x28) = f6;
    }
}

// 0x80204080
// Initializes the global id pool slot and the two 0x20000-byte tables in lbl_804FDAA0 (their
// element constructors are fn_8020415C, fn_8020418C and fn_802041B0).
void fn_80204080(void) {
    u8* base = lbl_804FDAA0;
    u8* r31 = base + 0x20000;

    lbl_805FBA94 = IdPoolNext(gIdPool);
    *(s32*)(r31 + 0x0) = 0;
    *(s32*)(r31 + 0x4) = 0;
    *(s32*)(r31 + 0x8) = 0;
    *(f32*)(r31 + 0xC) = lbl_80600358;
    *(f32*)(r31 + 0x10) = lbl_80600358;
    *(s32*)(r31 + 0x14) = 0;
    *(s32*)(r31 + 0x18) = 0;
    *(f32*)(r31 + 0x1C) = lbl_80600354;
    *(u8*)(r31 + 0x20) = 0;
    *(u8*)(r31 + 0x21) = 0;
    *(f32*)(r31 + 0x24) = lbl_806003D0;
    *(f32*)(r31 + 0x28) = lbl_80600348;
    fn_8032F1E8(r31 + 0x30, (void (*)(void))fn_8020415C, 0, 0xC0, 2);
    fn_8032F1E8(base + 0x20C08, (void (*)(void))fn_8020418C, 0, 0x114, 0x60);
    fn_8032F1E8(base + 0x27388, (void (*)(void))fn_802041B0, 0, 0x58, 0x60);
}

// 0x80117F90
// Releases the two resource handles of each of the arg0->+0x34 records (stride 0x20):
// the texture at +0x44 and the handle at +0x48. Clears both slots afterwards.
void fn_80117F90(u8* arg0) {
    s32 i;
    u8* p = arg0;

    *(s32*)(arg0 + 0x1C) = -1;
    for (i = 0; i < *(s32*)(arg0 + 0x34); i++, p += 0x20) {
        u8* t = *(u8**)(p + 0x44);
        s32 hid;
        if (t != 0) {
            s32 rc = *(s32*)(t + 0x24) - 1;
            *(s32*)(t + 0x24) = rc;
            if (rc == 0) {
                TextureRelease(t);
            }
            *(void**)(p + 0x44) = 0;
        }
        hid = *(s32*)(p + 0x48);
        if (hid != 0) {
            if (fn_80109A0C(hid) == 0) {
                fn_801098DC(hid);
            } else {
                u8* h = (u8*)(intptr_t)hid;
                s32 rc2 = *(s32*)(h + 0x24) - 1;
                *(s32*)(h + 0x24) = rc2;
                if (rc2 == 0) {
                    TextureRelease(h);
                }
            }
            *(s32*)(p + 0x48) = 0;
        }
    }
}

// 0x80197118
// Deleting destructor: sets the vtable, frees the subobject at +0x8, then for each of the
// +0x124 slots (from +0x24) releases the owned object at +0x30 - 4 and drops its refcount.
// Frees arg0 when arg1 > 0. Returns arg0.
void* fn_80197118(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        u8* slot;
        s32 i;

        *(void**)arg0 = (void*)lbl_80382380;
        fn_8018D4CC(*(u8**)(arg0 + 0x8));
        slot = arg0;
        for (i = 0; i < *(s32*)(arg0 + 0x124); i++, slot += 4) {
            u8* obj = *(u8**)(slot + 0x24);
            u8* q = *(u8**)(obj + 0x30);
            u8* var3 = (q != 0) ? q - 4 : 0;
            if (var3 != 0) {
                fn_80169704(var3, obj);
                {
                    u8* o2 = *(u8**)(slot + 0x24);
                    s32 rc = *(s32*)(o2 + 0x24) - 1;
                    *(s32*)(o2 + 0x24) = rc;
                    if (rc == 0) {
                        TextureRelease(o2);
                    }
                }
            }
        }
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x801A5124
// Forwards arg1 to the sub-object call (slot 0x20 of the table at +0x138) of every entry of the
// list at +0x1BC (count = +0x13C * +0x140), then of the object at +0x1B4 when its flag 2 is set.
s32 fn_801A5124(u8* arg0, s32 arg1) {
    s32 count = *(s32*)(arg0 + 0x13C) * *(s32*)(arg0 + 0x140);
    s32 i;

    for (i = 0; i < count; i++) {
        u8** ent = (u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC), i);
        u8* obj = (ent != 0) ? *ent : 0;
        if (obj != 0) {
            CallSubObjSlot20(obj, arg1);
        }
    }
    if (count > 0) {
        u8* o = *(u8**)(arg0 + 0x1B4);
        if (o != 0 && (*(s32*)(o + 0x64) & 2)) {
            CallSubObjSlot20(o, arg1);
        }
    }
    return 0;
}

// 0x8020844C
// One-time startup of the monlabwii module: clears the state, runs the slot 0x30 virtual of
// arg0, initializes NAND and starts the module. Returns 1 when already started or started now.
s32 fn_8020844C(u8* arg0) {
    if (lbl_805FB380 != 0) {
        return 1;
    }
    lbl_805FBB40 = 0;
    *(s32*)(arg0 + 0x4) = 0;
    {
        void* vt = *(void**)arg0;
        ((void (*)(u8*, s32))(*(void**)((u8*)vt + 0x30)))(arg0, 0);
    }
    *(u8*)lbl_805FBB54 = 0;
    NANDInit();
    if (fn_802078E8() == 0) {
        lbl_805FB368 = 0;
        lbl_805FBB30 = 7;
        return 0;
    }
    lbl_805FB368 = 1;
    lbl_805F991C = lbl_8060047C;
    lbl_805FBB3C = lbl_80600480;
    fn_802E49B8(1, (const char*)lbl_8038743C, lbl_805FBB38, (void*)fn_80207734, (void*)fn_802077B8, lbl_8060047C);
    fn_802E4D98(0x802A);
    return fn_80207DCC() != 0;
}

// 0x8002C540
// Advances the parameter at +0x2C of arg1 by farg0, clamps the ratio t = +0x2C / +0x28 to
// [0, 1] (stored at +0), and evaluates the quadratic Bezier of the control points at +0x4..+0x14
// and +0x34..+0x38 into +0x1C/+0x20 and arg0 (x, y, z).
void fn_8002C540(u8* arg0, u8* arg1, f32 farg0) {
    f32 acc = *(f32*)(arg1 + 0x2C) + farg0;
    f32 maxT;
    f32 t, t2, u, uu, tu2, x, y;

    *(f32*)(arg1 + 0x2C) = acc;
    maxT = *(f32*)(arg1 + 0x28);
    if (acc > maxT) {
        *(f32*)(arg1 + 0x0) = lbl_805FCBA0;
    } else {
        *(f32*)(arg1 + 0x0) = acc / maxT;
    }
    t = *(f32*)(arg1 + 0x0);
    t2 = t * t;
    u = lbl_805FCBA0 - t;
    uu = u * u;
    tu2 = (lbl_805FCBA4 * t) * u;
    x = (t2 * *(f32*)(arg1 + 0x10)) + ((uu * *(f32*)(arg1 + 0x4)) + (tu2 * *(f32*)(arg1 + 0x34)));
    y = (t2 * *(f32*)(arg1 + 0x14)) + ((uu * *(f32*)(arg1 + 0x8)) + (tu2 * *(f32*)(arg1 + 0x38)));
    *(f32*)(arg1 + 0x1C) = x;
    *(f32*)(arg1 + 0x20) = y;
    *(f32*)(arg0 + 0x0) = x;
    *(f32*)(arg0 + 0x4) = *(f32*)(arg1 + 0x20);
    *(f32*)(arg0 + 0x8) = *(f32*)(arg1 + 0x24);
}

}  // extern "C"
