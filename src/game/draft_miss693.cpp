#include "types.h"

// Chain-update helper shared by fn_800CDEA0 / fn_800CDFE0 / fn_800CE124 / fn_800CE258:
// writes the new position into the object's transform (+0x4) and sets the four flag bits.
static void chain_apply_pos(u8* p, f32 x, f32 y, f32 z) {
    u8* sub = *(u8**)(p + 0x4);
    *(f32*)(sub + 0x0) = x;
    *(f32*)(sub + 0x4) = y;
    *(f32*)(sub + 0x8) = z;
    *(s32*)(sub + 0x20) |= 0x8000;
    *(s32*)(sub + 0x20) |= 0x10000000;
    *(s32*)(sub + 0x20) |= 0x4000;
    *(s32*)(sub + 0x20) |= 0x10000000;
}

extern "C" {
void* EventPost(s32 id, void* sender);                                           // 0x801F3918
u32   StringHash(const char* s);                                                 // 0x801EB704
void  UIPlayAnim(void* el, u32 anim);                                            // 0x8019BE40
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle);    // 0x8032EFC8
s32   fn_80109600(u8* p);
s32   fn_80109A0C(s32 id);
u32   fn_8019552C(u8* a3, s32 a4);          // 0x8019552C; asm dereferences the result as a pointer
u32   fn_801F38F8(u8* a3);
s32   fn_801A3798(u8* p, f32 dt);
s32   fn_8019B6CC(u8* p, f32 dt);
void  fn_8019C348(u8* p);
void  fn_8019CBEC(u8* p);
s32   fn_801A9174(u8* p, f32* pos, f32* out);
void  fn_801CA624(u8* e, f32* v);
void  fn_801B72A0(void* out, u8* a1);
void  fn_801B4028(u8* p);                   // asm passes one pointer (leaf decl says void(void))
void  fn_801BF36C(u8* p);
void  fn_801C37F4(u8* p);
void  fn_801C7614(u8* p);
void  fn_801C763C(u8* p);
void  fn_801FCDE4(u8* p);
u8*   fn_801FCF74(u8* p, s32 del);
void  fn_8020441C(u8* p);
u8*   fn_801EF894(u8* p);
u8*   fn_801D57FC(u8* p, s32 del);
u8*   fn_80204430(u8* p, s32 del);
void  fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c);

extern "C++" void* gScreenSets;             // 0x805FB654
extern u8  lbl_8047C630[];                  // global object block
extern u8  lbl_805FB2A4[];                  // byte flag
extern u8  lbl_805FB2B0[];                  // byte flag
extern u8  lbl_805FB7E8[];
extern s32 lbl_805FB6B8;
extern const char lbl_805F94B0[];           // StringHash key
extern f32 lbl_805FE540;                    // float constant (accumulator seed)
extern f32 lbl_805FF9F0;
extern f32 lbl_805FFA04;
extern f32 lbl_805FFA38;
extern f32 lbl_805FFB28;
extern f32 lbl_805FFE90;
extern f32 lbl_805FFE94;
extern f32 lbl_805FFE98;
extern f32 lbl_805FFE9C;
}

extern "C" {

// 0x800CDEA0
// For each of 3 slots: walks the 3-entry chain (+0x34 sources), then the last entry
// attaches the root at +0x264 and writes the accumulated position to the root's transform.
void fn_800CDEA0(u8* arg0) {
    s32 i;
    for (i = 0; i < 3; i++) {
        u8* base = arg0 + i * 4;
        u8* root = *(u8**)(base + 0x28);
        f32 ax = lbl_805FE540;
        f32 ay = lbl_805FE540;
        f32 az = lbl_805FE540;
        s32 j;
        for (j = 0; j < 3; j++) {
            u8* slot = base + j * 0xC;
            if (j != 2) {
                u8* src = *(u8**)(slot + 0x34);
                u8* sub;
                *(u8**)(slot + 0x28) = src;
                sub = *(u8**)(src + 0x4);
                ax = *(f32*)(sub + 0x0);
                ay = *(f32*)(sub + 0x4);
                az = *(f32*)(sub + 0x8);
            } else {
                *(u8**)(slot + 0x28) = root;
                ax = ax + *(f32*)(root + 0x264);
                ay = ay + lbl_805FE540;
                az = az + lbl_805FE540;
                chain_apply_pos(root, ax, ay, az);
            }
        }
    }
}

// 0x800CDFE0
// Mirror of fn_800CDEA0: walks the chain backwards (+0x1C sources, root at +0x40) and subtracts.
void fn_800CDFE0(u8* arg0) {
    s32 i;
    for (i = 0; i < 3; i++) {
        u8* base = arg0 + i * 4;
        u8* root = *(u8**)(base + 0x40);
        f32 ax = lbl_805FE540;
        f32 ay = lbl_805FE540;
        f32 az = lbl_805FE540;
        s32 j;
        for (j = 2; j >= 0; j--) {
            u8* slot = base + j * 0xC;
            if (j != 0) {
                u8* src = *(u8**)(slot + 0x1C);
                u8* sub;
                *(u8**)(slot + 0x28) = src;
                sub = *(u8**)(src + 0x4);
                ax = *(f32*)(sub + 0x0);
                ay = *(f32*)(sub + 0x4);
                az = *(f32*)(sub + 0x8);
            } else {
                *(u8**)(slot + 0x28) = root;
                ax = ax - *(f32*)(root + 0x264);
                ay = ay - lbl_805FE540;
                az = az - lbl_805FE540;
                chain_apply_pos(root, ax, ay, az);
            }
        }
    }
}

// 0x800CE124
// Same chain walk as fn_800CDEA0 with the root taken from +0x28 and the Z term from +0x26C.
void fn_800CE124(u8* arg0) {
    s32 i;
    for (i = 0; i < 3; i++) {
        u8* base = arg0 + i * 0xC;
        u8* root = *(u8**)(base + 0x28);
        f32 ax = lbl_805FE540;
        f32 ay = lbl_805FE540;
        f32 az = lbl_805FE540;
        s32 j;
        for (j = 0; j < 3; j++) {
            u8* slot = base + j * 4;
            if (j != 2) {
                u8* src = *(u8**)(slot + 0x2C);
                u8* sub;
                *(u8**)(slot + 0x28) = src;
                sub = *(u8**)(src + 0x4);
                ax = *(f32*)(sub + 0x0);
                ay = *(f32*)(sub + 0x4);
                az = *(f32*)(sub + 0x8);
            } else {
                *(u8**)(slot + 0x28) = root;
                ax = ax + lbl_805FE540;
                ay = ay + lbl_805FE540;
                az = az + *(f32*)(root + 0x26C);
                chain_apply_pos(root, ax, ay, az);
            }
        }
    }
}

// 0x800CE258
// Reverse-order chain walk (sources at +0x24, root at +0x30) subtracting the Z term from +0x26C.
void fn_800CE258(u8* arg0) {
    s32 i;
    for (i = 0; i < 3; i++) {
        u8* base = arg0 + i * 0xC;
        u8* root = *(u8**)(base + 0x30);
        f32 ax = lbl_805FE540;
        f32 ay = lbl_805FE540;
        f32 az = lbl_805FE540;
        s32 j;
        for (j = 2; j >= 0; j--) {
            u8* slot = base + j * 4;
            if (j != 0) {
                u8* src = *(u8**)(slot + 0x24);
                u8* sub;
                *(u8**)(slot + 0x28) = src;
                sub = *(u8**)(src + 0x4);
                ax = *(f32*)(sub + 0x0);
                ay = *(f32*)(sub + 0x4);
                az = *(f32*)(sub + 0x8);
            } else {
                *(u8**)(slot + 0x28) = root;
                ax = ax - lbl_805FE540;
                ay = ay - lbl_805FE540;
                az = az - *(f32*)(root + 0x26C);
                chain_apply_pos(root, ax, ay, az);
            }
        }
    }
}

// 0x8011AC34
// Per-slot pass over 0x60-byte entries (count at +0x25A4): moves pending handles
// (+0x20/+0x30/+0x3C) into their active fields (+0x1C/+0x2C/+0x38) once fn_80109A0C/fn_80109600 accept them.
void fn_8011AC34(u8* arg0) {
    u32 i;
    s32 done = 0;
    lbl_805FB2A4[0] = 0;
    *(s32*)(arg0 + 0x2DE0) = 0;
    for (i = 0; i < *(u32*)(arg0 + 0x25A4); i++) {
        u8* p = arg0 + i * 0x60;
        u8* ka = arg0 + 0x20 + i * 0x60;
        u8* kb = arg0 + 0x30 + i * 0x60;
        u8* kc = arg0 + 0x3C + i * 0x60;
        s32 v = *(s32*)(p + 0x20);
        if (v != 0) {
            if (fn_80109A0C(v) == 0) {
                fn_80109600(ka);
            } else if (i <= *(u32*)(arg0 + 0x2DE0)) {
                *(s32*)(p + 0x1C) = *(s32*)(p + 0x20);
                *(s32*)(p + 0x20) = 0;
                lbl_805FB2A4[0] = 1;
            }
        }
        if (done == 0) {
            if (*(s32*)(p + 0x1C) != 0) {
                *(u32*)(arg0 + 0x2DE0) = i + 1;
            } else {
                done = 1;
            }
        }
        if (*(s32*)(p + 0x30) != 0 && fn_80109A0C(*(s32*)(p + 0x30)) == 0 && fn_80109600(kb) != 0) {
            *(s32*)(p + 0x2C) = *(s32*)(p + 0x30);
            *(s32*)(p + 0x30) = 0;
            lbl_805FB2A4[0] = 1;
        }
        if (*(s32*)(p + 0x3C) != 0 && fn_80109A0C(*(s32*)(p + 0x3C)) == 0 && fn_80109600(kc) != 0) {
            *(s32*)(p + 0x38) = *(s32*)(p + 0x3C);
            *(s32*)(p + 0x3C) = 0;
            lbl_805FB2A4[0] = 1;
        }
    }
}

// 0x8011FDC8
// Same pass as fn_8011AC34 over 0x3C-byte entries (count at +0x9260, done index at +0xA684).
void fn_8011FDC8(u8* arg0) {
    u32 i;
    s32 done = 0;
    lbl_805FB2B0[0] = 0;
    *(s32*)(arg0 + 0xA684) = 0;
    for (i = 0; i < *(u32*)(arg0 + 0x9260); i++) {
        u8* p = arg0 + i * 0x3C;
        u8* ka = arg0 + 0x20 + i * 0x3C;
        u8* kb = arg0 + 0x3C + i * 0x3C;
        u8* kc = arg0 + 0x48 + i * 0x3C;
        s32 v = *(s32*)(p + 0x20);
        if (v != 0) {
            if (fn_80109A0C(v) == 0) {
                fn_80109600(ka);
            } else if (i <= *(u32*)(arg0 + 0xA684)) {
                *(s32*)(p + 0x1C) = *(s32*)(p + 0x20);
                *(s32*)(p + 0x20) = 0;
                lbl_805FB2B0[0] = 1;
            }
        }
        if (done == 0) {
            if (*(s32*)(p + 0x1C) != 0) {
                *(u32*)(arg0 + 0xA684) = i + 1;
            } else {
                done = 1;
            }
        }
        if (*(s32*)(p + 0x3C) != 0 && fn_80109A0C(*(s32*)(p + 0x3C)) == 0 && fn_80109600(kb) != 0) {
            *(s32*)(p + 0x38) = *(s32*)(p + 0x3C);
            *(s32*)(p + 0x3C) = 0;
            lbl_805FB2B0[0] = 1;
        }
        if (*(s32*)(p + 0x48) != 0 && fn_80109A0C(*(s32*)(p + 0x48)) == 0 && fn_80109600(kc) != 0) {
            *(s32*)(p + 0x44) = *(s32*)(p + 0x48);
            *(s32*)(p + 0x48) = 0;
            lbl_805FB2B0[0] = 1;
        }
    }
}

// 0x801A4558
// Moves the current value (+0x144) toward the target (+0x140) at rate +0x158 scaled by dt,
// then pushes the result into the linked object (+0x160) and fires the vtable slot at +0x138.
s32 fn_801A4558(u8* arg0, f32 farg0) {
    f32 cur;
    f32 target;
    f32 f0;
    f32 t;
    f32 u;
    f32 v;
    u8* r3;

    fn_801A3798(arg0, farg0);
    cur = *(f32*)(arg0 + 0x144);
    target = *(f32*)(arg0 + 0x140);
    if (cur == target) {
        *(f32*)(arg0 + 0x148) = target;
        return 0;
    }
    if (*(s32*)(arg0 + 0x64) & 0x80000) {
        if (cur < target) {
            f0 = cur + farg0 * *(f32*)(arg0 + 0x158);
            *(f32*)(arg0 + 0x144) = f0;
            if (f0 > target) {
                *(f32*)(arg0 + 0x144) = target;
            }
        } else if (cur > target) {
            f0 = cur - farg0 * *(f32*)(arg0 + 0x158);
            *(f32*)(arg0 + 0x144) = f0;
            if (f0 < target) {
                *(f32*)(arg0 + 0x144) = target;
            }
        }
    } else {
        *(f32*)(arg0 + 0x144) = target;
    }
    r3 = *(u8**)(arg0 + 0x160);
    if (r3 != NULL) {
        if (*(f32*)(arg0 + 0x98) != lbl_805FF9F0) {
            t = *(f32*)(arg0 + 0x68) + *(f32*)(arg0 + 0x74);
            t = t - *(f32*)(arg0 + 0x68);
            u = cur * t;
            u = *(f32*)(arg0 + 0x68) + u;
            v = lbl_805FFA04 * *(f32*)(r3 + 0x74);
            u = u - v;
            *(f32*)(r3 + 0x68) = u;
        } else {
            t = *(f32*)(arg0 + 0x6C) + *(f32*)(arg0 + 0x78);
            t = t - *(f32*)(arg0 + 0x6C);
            u = cur * t;
            u = *(f32*)(arg0 + 0x6C) + u;
            v = lbl_805FFA04 * *(f32*)(r3 + 0x78);
            u = u - v;
            *(f32*)(r3 + 0x6C) = u;
        }
        fn_8019C348(r3);
    }
    {
        u8* vt = *(u8**)(arg0 + 0x138);
        ((void (*)(u8*, s32))*(void**)(vt + 0x2C))(arg0, 0x10);
    }
    return 0;
}

// 0x801A8318
// When the selection index (+0x150/+0x14C) or the active screen changed, swaps the
// current/previous indices, plays the change animations and refreshes the entry.
void fn_801A8318(u8* arg0) {
    u8* ent;
    u8* var3;
    u8* var30;
    s32 r6;
    s32 r7;

    if (!(*(s32*)(arg0 + 0x64) & 1)) {
        return;
    }
    if (*(s32*)(arg0 + 0x1F0) == *(s32*)(arg0 + 0x150) &&
        *(s32*)(arg0 + 0x1F4) == *(s32*)(arg0 + 0x14C) &&
        *(u8**)((u8*)gScreenSets + 0x74) == arg0) {
        return;
    }
    ent = (u8*)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC),
                                      *(s32*)(arg0 + 0x150) + *(s32*)(arg0 + 0x14C) * *(s32*)(arg0 + 0x140));
    var3 = (ent != NULL) ? *(u8**)ent : NULL;
    if (var3 != NULL && *(u8*)(arg0 + 0x16C) != 0) {
        UIPlayAnim(var3, 0xF5A9CE6Cu);
        UIPlayAnim(arg0, 0xF5A9CE6Cu);
        *(u8*)(arg0 + 0x16C) = 0;
    }
    r7 = *(s32*)(arg0 + 0x1F4);
    r6 = *(s32*)(arg0 + 0x1F0);
    *(s32*)(arg0 + 0x154) = *(s32*)(arg0 + 0x14C);
    *(s32*)(arg0 + 0x158) = *(s32*)(arg0 + 0x150);
    *(s32*)(arg0 + 0x150) = r6;
    *(s32*)(arg0 + 0x14C) = r7;
    *(u8*)(arg0 + 0x17C) = 0;
    ent = (u8*)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC),
                                      r6 + r7 * *(s32*)(arg0 + 0x140));
    var30 = (ent != NULL) ? *(u8**)ent : NULL;
    if (var30 != NULL) {
        UIPlayAnim(var30, 0x705598E1u);
    }
    UIPlayAnim(arg0, StringHash(lbl_805F94B0));
    if (var30 != NULL) {
        *(u8*)(arg0 + 0x16C) = 1;
    }
    fn_8019CBEC(arg0);
}

// 0x801A89A4
// Refreshes the child list at +0x178: runs the update hook (+0x138 slot 0x30) when flag 0x10 is set,
// then writes the computed position into each entry and feeds the list to fn_801CA624.
s32 fn_801A89A4(u8* arg0, f32 farg0) {
    f32 pos[3];
    f32 out[3];
    f32 x;
    f32 y;
    f32 z;
    f32 c;
    u8* list;
    u8* vt;
    u32 k;

    if (*(u8**)(arg0 + 0x178) == NULL && ((*(s32*)(arg0 + 0x64) >> 1) & 1) == 0) {
        return 0;
    }
    fn_8019B6CC(arg0, farg0);
    if (*(s32*)(arg0 + 0x64) & 0x10) {
        vt = *(u8**)(arg0 + 0x138);
        ((void (*)(u8*, s32))*(void**)(vt + 0x30))(arg0, 0x10);
        if (*(u8**)(arg0 + 0x178) != NULL) {
            fn_801A9174(arg0, pos, out);
            x = pos[0];
            y = pos[1];
            z = pos[2];
            c = lbl_805FFA38;
            list = *(u8**)(arg0 + 0x178);
            for (k = 0; k < (u32)*(s32*)(list + 0xC); k++) {
                u8* e = *(u8**)(list + 0x14 + k * 4);
                if (e != NULL) {
                    *(f32*)(e + 0x10) = x;
                    *(f32*)(e + 0x14) = y;
                    *(f32*)(e + 0x18) = z;
                    *(f32*)(e + 0x1C) = c;
                }
            }
            for (k = 0; k < (u32)*(s32*)(list + 0xC); k++) {
                u8* e = *(u8**)(list + 0x14 + k * 4);
                if (e != NULL) {
                    fn_801CA624(e, out);
                }
            }
        }
    }
    return 0;
}

// 0x801B62D8
// Posts event 0x77 (sender arg0) and fills its payload: position (arg4 or zero), ints, the
// float at farg0, and the target from arg1 (flag 0x80000 cleared unless a handle is resolved).
void fn_801B62D8(u8* arg0, u8* arg1, s32 arg2, s32 arg3, u8* arg4, f32 farg0) {
    u8* ev;
    s32 tmp;

    ev = (u8*)EventPost(0x77, arg0);
    if (ev == NULL) {
        return;
    }
    if (arg4 != NULL) {
        *(f32*)(ev + 0x4) = *(f32*)(arg4 + 0x0);
        *(f32*)(ev + 0x8) = *(f32*)(arg4 + 0x4);
        *(f32*)(ev + 0xC) = *(f32*)(arg4 + 0x8);
    } else {
        *(f32*)(ev + 0x4) = lbl_805FFB28;
        *(f32*)(ev + 0x8) = lbl_805FFB28;
        *(f32*)(ev + 0xC) = lbl_805FFB28;
    }
    *(s32*)(ev + 0x14) = arg3;
    *(f32*)(ev + 0x18) = farg0;
    *(s32*)(ev + 0x10) = arg2;
    *(f32*)(ev + 0x1C) = *(f32*)(*(u8**)(arg0 + 0x4) + 0x24);
    if (arg1 != NULL && fn_801F38F8(arg1) != 0) {
        tmp = lbl_805FB6B8;
        fn_801B72A0(&tmp, arg1);
        *(s32*)(ev + 0x20) = tmp;
        *(s32*)(ev + 0x24) = -1;
        return;
    }
    if (arg1 != NULL && *(s32*)(arg1 + 0x30) != -1) {
        *(s32*)(ev + 0x20) &= ~0x80000;
        *(s32*)(ev + 0x24) = *(s32*)(arg1 + 0x30);
        return;
    }
    *(s32*)(ev + 0x20) &= ~0x80000;
    *(s32*)(ev + 0x24) = -1;
}

// 0x801D56A4
// Initializes the global object block at lbl_8047C630 (+0x10 subobject) and registers its
// destructors and sub-objects with the global-constructor runtime.
void fn_801D56A4(void) {
    u8* base = lbl_8047C630;
    u8* o = base + 0x10;

    *(f32*)(o + 0xBC) = lbl_805FFE90;
    *(s32*)(o + 0xC4) = 0;
    *(f32*)(base + 0x10) = lbl_805FFE94;
    *(f32*)(o + 0x4) = lbl_805FFE94;
    *(f32*)(o + 0x8) = lbl_805FFE94;
    *(f32*)(o + 0xC) = lbl_805FFE94;
    *(f32*)(o + 0x1C) = lbl_805FFE90;
    *(f32*)(o + 0x20) = lbl_805FFE90;
    *(f32*)(o + 0x24) = lbl_805FFE90;
    *(f32*)(o + 0x28) = lbl_805FFE90;
    *(f32*)(o + 0x2C) = lbl_805FFE90;
    *(f32*)(o + 0x30) = lbl_805FFE90;
    *(f32*)(o + 0x34) = lbl_805FFE90;
    *(f32*)(o + 0x38) = lbl_805FFE90;
    *(f32*)(o + 0x3C) = lbl_805FFE90;
    *(u8*)(o + 0x40) = 0;
    *(f32*)(o + 0x50) = lbl_805FFE90;
    *(f32*)(o + 0x54) = lbl_805FFE90;
    *(f32*)(o + 0x58) = lbl_805FFE90;
    *(f32*)(o + 0x5C) = lbl_805FFE90;
    *(f32*)(o + 0x60) = lbl_805FFE98;
    *(f32*)(o + 0x64) = lbl_805FFE9C;
    *(f32*)(o + 0x68) = lbl_805FFE9C;
    *(s32*)(o + 0x78) = 0;
    *(s32*)(o + 0x70) = 0;
    *(s32*)(o + 0x80) = 0;
    __register_global_object(o, (void (*)(void*))fn_801D57FC, base);
    fn_801C7614(base + 0x100);
    fn_801C763C(base + 0x130);
    fn_801BF36C(base + 0x160);
    fn_801C37F4(base + 0x17C);
    fn_801B4028(base + 0x1A8);
    fn_801FCDE4(base + 0x1C0);
    __register_global_object(base + 0x1C0, (void (*)(void*))fn_801FCF74, base + 0x1B4);
    fn_8020441C(lbl_805FB7E8);
    __register_global_object(lbl_805FB7E8, (void (*)(void*))fn_80204430, base + 0x484);
    fn_8032F1E8(base + 0x490, (void (*)(void))fn_801EF894, 0, 0x394, 2);
}

// 0x801F76D4
// Scans the value array (+0x10) over the index range [start, end] for the best (max) entry,
// counting ties, and writes the summary record into arg1 (range, best, averages).
void fn_801F76D4(u8* arg0, u8* arg1) {
    s32 start;
    s32 end;
    s32 best;
    s32 cnt;
    s32 i;
    s32 mid;
    u8* arr;

    start = *(s32*)(arg0 + 0x8) - *(s32*)(arg0 + 0xC);
    end = *(s32*)(arg0 + 0x8) + *(s32*)(arg0 + 0x4) - 1;
    arr = *(u8**)(arg0 + 0x10);
    best = start;
    cnt = 0;
    for (i = start; i <= end; i++) {
        f32 vi = *(f32*)(arr + i * 4);
        f32 vb = *(f32*)(arr + best * 4);
        if (vi > vb) {
            best = i;
            cnt = 1;
        } else if (vi == vb) {
            cnt += 1;
        }
    }
    *(s32*)(arg1 + 0x0) = *(s32*)(arg0 + 0xC) + *(s32*)(arg0 + 0x4);
    *(f32*)(arg1 + 0x4) = *(f32*)(arr + best * 4);
    *(s32*)(arg1 + 0x8) = cnt;
    *(s32*)(arg1 + 0xC) = best - start;
    *(f32*)(arg1 + 0x10) = (*(f32*)(arr + best * 4) - *(f32*)(arr + start * 4)) / (f32)(best - start);
    mid = best + cnt;
    *(s32*)(arg1 + 0x14) = end - mid;
    *(f32*)(arg1 + 0x18) = (*(f32*)(arr + end * 4) - *(f32*)(arr + mid * 4)) / (f32)(end - mid);
}

}  // extern "C"
