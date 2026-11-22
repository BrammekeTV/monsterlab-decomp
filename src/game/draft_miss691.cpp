// Small functions around 0x80027Axx-0x8008Axxx (batch miss691), verified against asm.
#include "types.h"

// Pointer-slot helpers for virtual calls that go through a sub-object stored at +0x138.
// Loads the sub-object at obj+0x138, then calls its slot at +0x2C with (obj, arg).
static void CallSub138Slot2C(u8* obj, s32 arg) {
    u8* sub = *(u8**)(obj + 0x138);
    ((void (*)(u8*, s32))(*(void**)(sub + 0x2C)))(obj, arg);
}

// Virtual call through the vtable of obj (slot at +off), taking no arguments besides obj.
static u8* CallVirtualRet(u8* obj, s32 off) {
    return ((u8* (*)(u8*))(*(void**)(*(u8**)obj + off)))(obj);
}

extern "C++" void* gScreenSets;   // 0x805FB654 (defined in a C++ file)
extern "C++" void fn_801B503C(u8* p);   // 0x801B503C (defined in small_801Bb.cpp, C++ linkage)

extern "C" {
void     operator_delete(void* p);
void     fn_800222F0(void);
void*    ScreenSetsCurrent(void* sets);                                 // 0x8018D1E4
void*    ScreenSetFindElement(void* set, u32 hash);                     // 0x80198F18
u32      StringHash(const char* s);                                     // 0x801EB704
s32      UISetTexture(void* el, BOOL tex);                              // 0x801A11FC
u32      RandomU32(void);                                               // 0x80143460
void     __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);
u8*      fn_800216B8(u8* p, s32 del);
u8*      fn_80030270(u8* p, s32 del);                                   // 0x80030270
u8*      fn_80030A40(u8* p, s32 del);
u8*      fn_80030798(u8* p, s32 del);
u8*      fn_800308E8(u8* p, s32 del);
u8*      fn_80030648(u8* p, s32 del);
u8*      fn_80030BC8(u8* p, s32 del);
u8*      fn_801B3308(void);
void     fn_801B3378(u8* p, u8* key);
void     fn_8020A398(s32 handle);
u8*      fn_801F9C78(u8* p, s32 del);
u8*      fn_801F6238(u8* p, s32 del);
void     fn_801F916C(f32* out, u8* p, s32 a);
void     fn_800367CC(u8* p, s32 a, s32 b, s32 c);
void     fn_8003B994(u8* p, s32 mode);
void     fn_8003BC78(u8* p);
f32      fn_801AB7F0(u8* a3);
s32      fn_8019BBA4(u8* a, u8* b, s32 c);
void*    fn_802209D8(s32 kind);
s32      fn_80220A2C(void);
u32      fn_80220890(u8* data, s32 len);
s32      fn_801DA780(u8* a3);
u8*      fn_801DA788(u8* a3, s32 i);
u8*      fn_801ABF0C(u8* table, u32 hash, u8* key);
u8*      fn_801AC024(u8* table, u32 hash, u8* key, u8* prev);
void     fn_80073A7C(void* cam, const f32* pos, f32* outX, f32* outY);
s32      fn_8008C1BC(u8* cam, s32 x, s32 y);

extern u8  lbl_803579D8[];   // 0x803579D8 (address stored into the object)
extern void* lbl_805FAFE0;   // 0x805FAFE0
extern u8  lbl_805FCA28;     // 0x805FCA28 (6-byte key string)
extern u8  lbl_805FCA29;     // 0x805FCA29
extern u8  lbl_805FCA2A;     // 0x805FCA2A
extern u8  lbl_805FCA2B;     // 0x805FCA2B
extern u8  lbl_805FCA2C;     // 0x805FCA2C
extern u8  lbl_805FCA2D;     // 0x805FCA2D
extern f32 lbl_805FC990;     // 0x805FC990 (0.0f)
extern f32 lbl_805FCA30;     // 0x805FCA30 (0.1f)
extern f32 lbl_805FCBB0;     // 0x805FCBB0 (0.0f)
extern f32 lbl_805FCBC8;     // 0x805FCBC8 (-0.0001f)
extern f32 lbl_805FCBCC;     // 0x805FCBCC (0.0001f)
extern f32 lbl_805FCBF0;     // 0x805FCBF0 (5.0f)
extern f32 lbl_805FCBF4;     // 0x805FCBF4 (-5.0f)
extern f32 lbl_805FCD5C;     // 0x805FCD5C (2.0f)
extern f32 lbl_805FCFAC;     // 0x805FCFAC (0.0f)
extern f32 lbl_805FD194;     // 0x805FD194 (0.0f)
extern f32 lbl_805FD270;     // 0x805FD270
extern f32 lbl_805FD274;     // 0x805FD274
extern f32 lbl_805FD928;     // 0x805FD928 (0.0f)
}

extern "C" {

// 0x80027A68
// Resets the 8 slots at +0xB0 (virtual call with 2), then looks up the 4 named screen
// elements and resets the 4 entries of 0x14 bytes each.
void fn_80027A68(u8* arg0) {
    s32 n;
    s32 off;
    fn_800222F0();
    for (n = 0, off = 0; n < 8; n++, off += 0x48) {
        u8* e = *(u8**)(*(u8**)(arg0 + 0xB0) + off);
        if (e != NULL) {
            CallSub138Slot2C(e, 2);
        }
    }
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    u8 key[6];
    key[0] = lbl_805FCA28;
    key[1] = lbl_805FCA29;
    key[2] = lbl_805FCA2A;
    key[3] = lbl_805FCA2B;
    key[4] = lbl_805FCA2C;
    key[5] = lbl_805FCA2D;
    for (s32 i = 0; i < 4; i++) {
        u8* base = arg0 + i * 0x14;
        key[4] = (u8)(i + 0x30);
        u8* el = (u8*)ScreenSetFindElement(set, StringHash((const char*)key));
        if (el != NULL) {
            *(u8**)(base + 0x18C) = el;
            *(f32*)(el + 0x158) = lbl_805FC990;
            *(f32*)(el + 0x154) = lbl_805FCA30;
            CallSub138Slot2C(el, 2);
        }
        *(s32*)(base + 0x190) = 0;
        *(s32*)(base + 0x194) = -0x5A;
    }
}

// 0x8002D6E4
// Picks one of four slots from the signed value at +0x2EC (mode at +0x2E8) and
// stores the chosen index at +0xB0; returns a signed 5.0 / -5.0 / 0.0 result.
f32 fn_8002D6E4(u8* arg0) {
    s32 mode = *(s32*)(arg0 + 0x2E8);
    f32 v;
    if (mode == 1) {
        v = *(f32*)(arg0 + 0x2EC);
    } else if (mode == 2) {
        v = -*(f32*)(arg0 + 0x2EC);
    } else {
        v = lbl_805FCBB0;
    }
    if (v != v) {   // NaN guard
        v = lbl_805FCBB0;
    }
    f32 av = (v < 0.0f) ? -v : v;
    *(s32*)(arg0 + 0xB0) = 0;
    if (av > *(f32*)(arg0 + 0xD4)) *(s32*)(arg0 + 0xB0) = 0;
    if (av > *(f32*)(arg0 + 0xD4 + *(s32*)(arg0 + 0xB0) * 4)) *(s32*)(arg0 + 0xB0) = 1;
    if (av > *(f32*)(arg0 + 0xD4 + *(s32*)(arg0 + 0xB0) * 4)) *(s32*)(arg0 + 0xB0) = 2;
    if (av > *(f32*)(arg0 + 0xD4 + *(s32*)(arg0 + 0xB0) * 4)) *(s32*)(arg0 + 0xB0) = 3;
    if (v > lbl_805FCBC8 && v < lbl_805FCBCC) {
        *(s32*)(arg0 + 0xB0) = 0;
        return lbl_805FCBB0;
    }
    if (v > lbl_805FCBB0) {
        *(s32*)(arg0 + 0xB0) = 2;
        return lbl_805FCBF0;
    }
    *(s32*)(arg0 + 0xB0) = 2;
    return lbl_805FCBF4;
}

// 0x8002E230
// Deleting destructor for the big object: resets the tables, destroys the 5 arrays,
// then frees the object when del > 0.
u8* fn_8002E230(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_803579D8;
        fn_801B3378(fn_801B3308(), arg0 + 0x100);
        fn_801B3378(fn_801B3308(), arg0 + 0x1BC);
        s32 h = *(s32*)(arg0 + 0xF78);
        if (h > 0) {
            fn_8020A398(h);
        }
        lbl_805FAFE0 = NULL;
        __destroy_arr(arg0 + 0xBEC, (void (*)(void*, s32))fn_80030A40, 0x3C, 0xA);
        __destroy_arr(arg0 + 0x9BC, (void (*)(void*, s32))fn_80030798, 0x38, 0xA);
        __destroy_arr(arg0 + 0x78C, (void (*)(void*, s32))fn_800308E8, 0x38, 0xA);
        __destroy_arr(arg0 + 0x55C, (void (*)(void*, s32))fn_80030648, 0x38, 0xA);
        __destroy_arr(arg0 + 0x2B4, (void (*)(void*, s32))fn_80030BC8, 0x44, 0xA);
        fn_801F9C78(arg0 + 0x1BC, -1);
        fn_801F6238(arg0 + 0x100, -1);
        fn_80030270(arg0 + 0xC8, -1);
        fn_80030270(arg0 + 0x90, -1);
        fn_800216B8(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80032CFC
// Clamps the float rectangle stored in arg0 against the values in arg1, derives the
// 4 bounds (+/-2 margin) and writes the position of the owner object at +0x68/+0x6C.
void fn_80032CFC(u8* arg0, u8* arg1) {
    u8* obj = *(u8**)(arg0 + 0x0);
    f32 sp8 = *(f32*)(obj + 0x68) - *(f32*)(arg0 + 0x4);
    f32 f3 = *(f32*)(arg1 + 0x0);
    f32 f4 = *(f32*)(arg0 + 0x20);
    *(f32*)(arg0 + 0x4) = f3;
    f32 spC = *(f32*)(obj + 0x6C) - *(f32*)(arg0 + 0x8);
    *(f32*)(arg0 + 0x8) = *(f32*)(arg1 + 0x4);
    if (f3 >= f4) {
        f4 = f3;
    }
    *(f32*)(arg0 + 0x4) = f4;
    f32 f1 = *(f32*)(arg0 + 0x28);
    if (!(f4 > f1)) {
        f1 = f4;
    }
    *(f32*)(arg0 + 0x4) = f1;
    f32 f2 = *(f32*)(arg0 + 0x24);
    f32 h = *(f32*)(arg0 + 0x8);
    if (h >= f2) {
        f2 = h;
    }
    *(f32*)(arg0 + 0x8) = f2;
    f32 g1 = *(f32*)(arg0 + 0x2C);
    if (!(f2 > g1)) {
        g1 = f2;
    }
    *(f32*)(arg0 + 0x8) = g1;
    f32 c = lbl_805FCD5C;
    f32 x = *(f32*)(arg0 + 0x4);
    *(f32*)(arg0 + 0x30) = x - c;
    *(f32*)(arg0 + 0x34) = g1 - c;
    *(f32*)(arg0 + 0x38) = c + x;
    *(f32*)(arg0 + 0x3C) = c + g1;
    if (*(u8*)(arg0 + 0x40) != 0) {
        *(f32*)(obj + 0x68) = x + sp8;
        *(f32*)(obj + 0x6C) = g1 + spC;
    } else {
        *(f32*)(obj + 0x68) = x;
        *(f32*)(obj + 0x6C) = g1;
    }
    CallSub138Slot2C(obj, 0x1A);
}

// 0x8003639C
// Picks a bit mask (1, 2 or 4 for a single side, else the combined mask) from three
// counters at +0x254, +0x424 and +0x364, using RandomU32 when more than one side is set.
s32 fn_8003639C(u8* arg0) {
    s32 r30 = 1;
    s32 r7 = *(s32*)(arg0 + 0x254);
    s32 r5 = *(s32*)(arg0 + 0x424);
    s32 r6 = *(s32*)(arg0 + 0x364);
    if (r6 + (r5 + r7) < *(s32*)(arg0 + 0x5F4)) {
        return 0;
    }
    if (r7 < r5) {
        r7 = r5;
        r30 = 4;
    } else if (r7 == r5) {
        r30 |= 4;
    }
    if (r7 < r6) {
        r30 = 2;
    } else if (r7 == r6) {
        r30 |= 2;
    }
    s32 m = r30 & 7;
    s32 cnt = (r30 & 1) + ((r30 >> 1) & 1) + ((r30 >> 2) & 1);
    if (cnt > 1) {
        s32 pick = (s32)(RandomU32() % (u32)cnt) + 1;
        s32 b0 = m & 1;
        s32 b1 = (m >> 1) & 1;
        s32 b2 = (m >> 2) & 1;
        if (b0 == pick) {
            return 1;
        }
        if (b0 + b1 == pick) {
            return 2;
        }
        if (b0 + b1 + b2 == pick) {
            return 4;
        }
    }
    return r30;
}

// 0x80038248
// Picks one free slot (flag byte at +0xA2C) at random, avoiding the last pick, resets the
// flags when none is free, and initialises the picked entry via fn_800367CC.
// NOTE: fn_800367CC argument types are guessed from the lwz loads.
void fn_80038248(u8* arg0) {
    s32 freeCount = 0;
    s32 total = *(s32*)(arg0 + 0xA3C);
    for (s32 i = 0; i < total; i++) {
        if (*(u8*)(arg0 + 0xA2C + i) == 0) {
            freeCount += 1;
        }
    }
    if (freeCount == 0) {
        for (s32 i = 0; i < *(s32*)(arg0 + 0xA3C); i++) {
            *(u8*)(arg0 + 0xA2C + i) = 0;
        }
        freeCount = *(s32*)(arg0 + 0xA3C);
    }
    s32 cand = freeCount;
    s32 pick = (s32)(RandomU32() % (u32)cand);
    if (cand == *(s32*)(arg0 + 0xA3C) && cand != 1 && *(s32*)(arg0 + 0xA40) == pick) {
        s32 t = pick + cand - 1;
        if (RandomU32() & 1) {
            t = pick + 1;
        }
        pick = t % cand;
    }
    s32 n = *(s32*)(arg0 + 0xA3C);
    for (s32 i = 0; i < n; i++) {
        if (*(u8*)(arg0 + 0xA2C + i) == 0) {
            if (pick == 0) {
                fn_800367CC(arg0 + 0x170, *(s32*)(arg0 + 0x164),
                            *(s32*)(arg0 + i * 4 + 0x9B4), *(s32*)(arg0 + i * 4 + 0x9F0));
                *(u8*)(arg0 + 0xA2C + i) = 1;
                *(s32*)(arg0 + 0xA40) = i;
                return;
            }
            pick -= 1;
        }
    }
}

// 0x8003B834
// Updates the highlight: a timer at +0x478 feeds +0x3EC, a 2D direction from fn_801F916C
// selects one of four textures (plus the matching fn_8019BBA4 / fn_801AB7F0 calls).
void fn_8003B834(u8* arg0) {
    if (*(u8*)(arg0 + 0x474) != 0) {
        f32 t = *(f32*)(arg0 + 0x478);
        if (t > lbl_805FCFAC) {
            *(f32*)(arg0 + 0x3EC) = t;
        }
    }
    s32 v;
    if (*(u8*)(arg0 + 0x474) != 0) {
        f32 out[2];
        fn_801F916C(out, arg0 + 0x3E4, 0);
        if (!(out[1] >= lbl_805FCFAC)) {
            v = 2;
        } else if (out[1] > lbl_805FCFAC) {
            v = 1;
        } else if (!(out[0] >= lbl_805FCFAC)) {
            v = 8;
        } else if (out[0] > lbl_805FCFAC) {
            v = 4;
        } else {
            v = 0;
        }
    } else {
        v = 0;
    }
    if (v & 0xC) {
        fn_8003B994(arg0, v);
        if (v & 4) {
            UISetTexture(*(void**)(arg0 + 0x924), *(s32*)(arg0 + 0x3D4));
            fn_8019BBA4(*(u8**)(arg0 + 0x924), *(u8**)(arg0 + 0x934), 0xA);
            *(f32*)(arg0 + 0x958) = fn_801AB7F0(*(u8**)(arg0 + 0x934));
            return;
        }
        UISetTexture(*(void**)(arg0 + 0x924), *(s32*)(arg0 + 0x3D0));
        fn_8019BBA4(*(u8**)(arg0 + 0x924), *(u8**)(arg0 + 0x938), 0xA);
        *(f32*)(arg0 + 0x958) = fn_801AB7F0(*(u8**)(arg0 + 0x934));
        return;
    }
    if (v & 3) {
        fn_8003BC78(arg0);
        UISetTexture(*(void**)(arg0 + 0x924), *(s32*)(arg0 + 0x3CC));
        fn_8019BBA4(*(u8**)(arg0 + 0x924), *(u8**)(arg0 + 0x93C), 0xA);
        *(f32*)(arg0 + 0x958) = fn_801AB7F0(*(u8**)(arg0 + 0x934));
    }
}

// 0x8004434C
// Per-frame state step: a latched flag at +0x45 returns the value at +0x46; otherwise
// a countdown at +0x4CC is advanced by farg0 and, when it runs out, a record is queued.
u8 fn_8004434C(u8* arg0, f32 farg0) {
    if (*(u8*)(arg0 + 0x45) != 0) {
        *(u8*)(arg0 + 0x44) = 1;
        return *(u8*)(arg0 + 0x46);
    }
    if (*(u8*)(arg0 + 0xD8) != 0) {
        f32 f2 = *(f32*)(arg0 + 0xDC);
        if (f2 > lbl_805FD194) {
            *(f32*)(arg0 + 0x50) = f2;
        }
    }
    if (*(u8*)(arg0 + 0xD8) != 0) {
        *(u8*)(arg0 + 0x44) = 1;
        fn_801B3378(fn_801B3308(), arg0 + 0x48);
        u8* r = (u8*)fn_802209D8(3);
        if (r != NULL) {
            *(s32*)(r + 0x4) = fn_80220A2C();
            *(s32*)(r + 0xC) = 0x28;
            *(s32*)(r + 0x24) = 1;
            *(s32*)(r + 0x14) = 0;
            *(s32*)(r + 0x14) = (s32)fn_80220890(r, 0x28);
        }
        return 1;
    }
    f32 cd = *(f32*)(arg0 + 0x4CC);
    if (!(cd >= lbl_805FD194)) {
        *(u8*)(arg0 + 0x44) = 1;
        fn_801B3378(fn_801B3308(), arg0 + 0x48);
        u8* r = (u8*)fn_802209D8(3);
        if (r != NULL) {
            *(s32*)(r + 0x4) = fn_80220A2C();
            *(s32*)(r + 0xC) = 0x28;
            *(s32*)(r + 0x24) = 0;
            *(s32*)(r + 0x14) = 0;
            *(s32*)(r + 0x14) = (s32)fn_80220890(r, 0x28);
        }
        return 0;
    }
    *(f32*)(arg0 + 0x4CC) = cd - farg0;
    return 0;
}

// 0x80050014
// Walks the child list of obj (vtable slots +0xCC, +0xC) and resets the per-entry
// floats of each child, then calls fn_801B503C on obj.
void fn_80050014(u8* obj) {
    f32 f30 = lbl_805FD274;
    u8* t = CallVirtualRet(obj, 0xCC);
    if (t != NULL) {
        u8* t2 = *(u8**)(t + 0x24);
        if (t2 != NULL) {
            u8* t3 = *(u8**)(t2 + 0x1C);
            if (t3 != NULL) {
                t3 -= 0xC48;
            }
            if (t3 != NULL) {
                u8* t4 = CallVirtualRet(t3, 0xC);
                f30 = *(f32*)(t4 + 0x44);
            }
        }
    }
    u8* sub = CallVirtualRet(obj, 0xC);
    f32 f1 = *(f32*)(sub + 0x44);
    f32 f5 = f30 * f1;
    f32 f31 = f5 - lbl_805FD274;
    f30 = lbl_805FD270;
    s32 idx = 0;
    while (idx < fn_801DA780(sub)) {
        u8* e = fn_801DA788(sub, idx);
        *(f32*)(e + 0x7C) = f30;
        idx += 1;
        *(f32*)(e + 0x80) = f30;
        *(f32*)(e + 0x84) = f30;
        *(f32*)(e + 0x88) = f31;
    }
    fn_801B503C(obj);
}

// 0x80059950
// Sets the scale at +0xB0 of the object at +0xA0: from the value at +0x638 when flag
// bit 2 of +0x838 is set, otherwise from the keyed entries of the table at +0x84.
void fn_80059950(u8* arg0) {
    if (*(s32*)(arg0 + 0x838) & 2) {
        *(f32*)(*(u8**)(arg0 + 0xA0) + 0xB0) = *(f32*)(arg0 + 0x638);
        return;
    }
    u8* x = *(u8**)(arg0 + 0xA4);
    if (x == NULL) {
        return;
    }
    u8* ent = *(u8**)x;
    if (ent == NULL) {
        return;
    }
    s32 idx = *(s32*)(ent + 0x4);
    u8* elem = NULL;
    if (idx > -1) {
        u8* q = *(u8**)ent;
        u8* base = *(u8**)q;
        elem = base + idx * 0x30;
    }
    s32 var_r0 = 0;
    if (elem != NULL) {
        var_r0 = *(s32*)(elem + 0x4);
    }
    if (var_r0 != 0) {
        u8* r30 = *(u8**)(arg0 + 0x84);
        if (r30 != NULL) {
            f32 f31 = *(f32*)r30;
            u8* key = *(u8**)(arg0 + 0xA0);
            u8* r6 = fn_801ABF0C(r30, 0xAF1BBABDu, key);
            while (r6 != NULL) {
                f31 = f31 * *(f32*)(r6 + 0x10);
                r6 = fn_801AC024(r30, 0xAF1BBABDu, key, r6);
            }
            u8* r3 = fn_801ABF0C(r30, 0xB26372C0u, key);
            if (r3 != NULL) {
                f31 = *(f32*)(r3 + 0x10);
            }
            *(f32*)(*(u8**)(arg0 + 0xA0) + 0xB0) = f31;
        }
    }
}

// 0x8008928C
// Scans 150 pointer slots at +0xF8 for entries of type 3, projects each one through
// fn_80073A7C and fn_8008C1BC, and keeps in *arg1 the candidate with the larger +8 value.
// Returns 1 when *arg1 is set.
s32 fn_8008928C(u8* arg0, u8** arg1) {
    if (arg1 == NULL) {
        return 0;
    }
    f32 spC = lbl_805FD928;
    f32 sp8 = lbl_805FD928;
    f32 sp10;
    f32 sp14;
    f32 sp18;
    for (s32 i = 0; i < 0x96; i++) {
        u8* temp3 = *(u8**)(arg0 + 0xF8 + i * 4);
        if (temp3 != NULL && *(s32*)(temp3 + 0x264) == 3) {
            u8* temp7 = *(u8**)(temp3 + 0x4);
            sp10 = *(f32*)(temp7 + 0x0);
            sp14 = *(f32*)(temp7 + 0x4);
            sp18 = *(f32*)(temp7 + 0x8);
            fn_80073A7C(NULL, &sp10, &spC, &sp8);
            if (fn_8008C1BC(*(u8**)(arg0 + 0x354), (s32)spC, (s32)sp8) != 0) {
                u8* cur = *arg1;
                if (cur == NULL) {
                    *arg1 = temp3;
                } else {
                    f32 a = *(f32*)(*(u8**)(cur + 0x4) + 0x8);
                    f32 b = *(f32*)(*(u8**)(temp3 + 0x4) + 0x8);
                    if (a > b) {
                        *arg1 = temp3;
                    }
                }
            }
        }
    }
    return (*arg1 != NULL) ? 1 : 0;
}

}  // extern "C"
