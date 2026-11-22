#include "types.h"
#include <math.h>

extern "C" {
void BufferFree(void* p);
void* MemAllocDefault(u32 size);
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);
int SysSprintf(char* buf, const char* fmt, ...);
void UIPlayAnim(void* el, u32 anim);
void fn_800CDEA0(u8* obj);     // 0x800CDEA0
void fn_800CDFE0(u8* obj);     // 0x800CDFE0
void fn_800CE124(u8* obj);     // 0x800CE124
void fn_800CE258(u8* obj);     // 0x800CE258
void fn_800D8D00(u8* obj, void* a1, s32 a2);   // 0x800D8D00
void fn_801458B0(f32* quat, f32* a, f32* b);   // 0x801458B0
void fn_801A904C(u8* p);                       // 0x801A904C
s32 fn_801A9174(u8* p, f32* pos, f32* out);    // 0x801A9174
void fn_801C83AC(u8* mgr, u8* item);           // 0x801C83AC
u8* fn_801483BC(s32 idx);                      // 0x801483BC
u8* fn_80169770(u8* arg0, const char* name);    // 0x80169770
s32 fn_8016B5E0(u8* arg0, u8* arg1, s32 arg2); // 0x8016B5E0
s32 fn_8016B6DC(u8* state);                    // 0x8016B6DC
void fn_8016B704(u8* state);                   // 0x8016B704
u8* fn_8016B724(u8* obj, u8* list, const char* name);  // 0x8016B724 (list is a pointer in asm)
s32 fn_801EE46C(u8* handle, const char* path); // 0x801EE46C (two args in asm)
s32 fn_801EE504(u8* handle);                   // 0x801EE504
s32 fn_801EE50C(u8* handle, u8* buf, s32 size, s32 flag);  // 0x801EE50C
void fn_801EE4B8(u8* handle);                 // 0x801EE4B8
void fn_8018D2B8(const char* fmt, ...);        // 0x8018D2B8
u32 fn_8019552C(u8* list, s32 index);          // 0x8019552C
s32 fn_8019555C(u8* list);                     // 0x8019555C
void fn_801F5B30(u8* slot);                    // 0x801F5B30
void fn_801F3BB8(void);                        // 0x801F3BB8
void fn_801F5E3C(void* a, const void* b, s32 c);  // 0x801F5E3C
void fn_801F5E48(void* a, s32 b);              // 0x801F5E48
void fn_801F5E4C(void* a, s32 b);              // 0x801F5E4C

extern u8 lbl_8035FC48[];
extern u8 lbl_8047C760[];
extern u8 lbl_804DD180[];
extern u8 lbl_804DD568[];
extern u8 lbl_804EC590[];
extern u8 lbl_804FB5A8[];
extern u8 lbl_804FB5C4[];
extern u8 lbl_80385FC0[];
extern const char lbl_8038236C[];
extern const char lbl_803827D8[];
extern const char lbl_805F93C0[];
extern const char lbl_805F93C8[];
extern u8* lbl_805FB5A8;
extern u8* lbl_805FB95C;
extern s32 lbl_805F97F8;
extern s32 lbl_805F97FC;
extern s32 lbl_805FB980;
extern s32 lbl_805FB984;
extern s32 lbl_805FB988;
extern s32 lbl_805FB98C;
extern s32 lbl_805FB990;
extern s32 lbl_805FB994;
extern s32 lbl_805FB9A8;
extern s32 lbl_805FB9AC;
extern u32 lbl_805FB9B0;
extern u32 lbl_805FB9B4;
extern s32 lbl_805FB9B8;
extern u32 lbl_805FB9BC;
extern u32 lbl_805FB9C0;
extern u32 lbl_805FB9C4;
extern u32 lbl_805FB9C8;
extern u32 lbl_805FB9CC;
extern s32 lbl_805FB9D4;
extern u32 lbl_805FB96C;
extern f32 lbl_805FE540;
extern f32 lbl_805FE668;
extern f32 lbl_805FE66C;
extern f32 lbl_805FE680;
extern f32 lbl_805FE684;
extern f32 lbl_805FE688;
extern f32 lbl_805FED58;
extern f32 lbl_805FFA38;
extern f32 lbl_80600280;
extern f32 lbl_80600284;
extern f32 lbl_80600288;
extern f32 lbl_8060028C;
}

extern "C" {

// 0x800CDCC4
void fn_800CDCC4(u8* arg0, u8* arg1, u8* arg2) {
    f32 maxXNeg = lbl_805FE540;
    f32 maxXPos = lbl_805FE540;
    f32 maxZPos = lbl_805FE540;
    f32 maxZNeg = lbl_805FE540;
    s32 face;

    for (face = 0; face < 3; face++) {
        u8* vtx = arg0 + face * 4;
        s32 v;
        for (v = 0; v < 3; v++, vtx += 0xC) {
            u8* p = *(u8**)(*(u8**)(vtx + 0x28) + 0x4);
            f32 dx = *(f32*)(p + 0x0) - *(f32*)(arg1 + 0x0);
            f32 dy = *(f32*)(p + 0x4) - *(f32*)(arg1 + 0x4);
            f32 dz = *(f32*)(p + 0x8) - *(f32*)(arg1 + 0x8);
            f32 dot = (dz * *(f32*)(arg2 + 0x8)) + ((dx * *(f32*)(arg2 + 0x0)) + (dy * *(f32*)(arg2 + 0x4)));
            f32 ax = (f32)fabs(dx);
            f32 az = (f32)fabs(dz);
            if (dot > lbl_805FE540) {
                if (!(maxXPos > ax)) maxXPos = ax;
                if (!(maxZPos > az)) maxZPos = az;
            } else {
                if (!(maxXNeg > ax)) maxXNeg = ax;
                if (!(maxZNeg > az)) maxZNeg = az;
            }
        }
    }
    if (maxXNeg > maxXPos) {
        if (*(f32*)(arg2 + 0x0) > lbl_805FE540) {
            fn_800CDEA0(arg0);
        } else {
            fn_800CDFE0(arg0);
        }
    }
    if (maxZNeg > maxZPos) {
        if (*(f32*)(arg2 + 0x8) > lbl_805FE540) {
            fn_800CE124(arg0);
            return;
        }
        fn_800CE258(arg0);
    }
}

// 0x800D887C
u8* fn_800D887C(u8* arg0, u8* arg1, s32 arg2) {
    f32 vecA[3];
    f32 vecB[3];
    u8* p;
    u8* m;
    f32 x, z, len2, inv;
    f32 a = lbl_805FE668;
    f32 b = lbl_805FE66C;

    *(s32*)(arg0 + 0x8) = 0;
    *(u8**)(arg0 + 0x0) = lbl_8035FC48;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(f32*)(arg0 + 0x14) = a;
    *(f32*)(arg0 + 0x18) = a;
    *(f32*)(arg0 + 0x1C) = a;
    *(f32*)(arg0 + 0x20) = a;
    p = (u8*)MemAllocDefault(0x40);
    *(u8**)(arg0 + 0x4) = p;
    *(f32*)(p + 0x0) = a;
    *(f32*)(p + 0x4) = a;
    *(f32*)(p + 0x8) = a;
    *(f32*)(p + 0xC) = a;
    *(f32*)(p + 0x10) = a;
    *(f32*)(p + 0x14) = a;
    *(f32*)(p + 0x18) = b;

    fn_800D8D00(arg0, arg1, arg2);
    m = *(u8**)(arg0 + 0x8);
    x = *(f32*)(m + 0xC);
    z = *(f32*)(m + 0x14);

    /* cross-style terms as in the asm: f9, f10, f8 */
    {
        f32 t1 = a * a;
        f32 t0 = a * x;
        f32 t4 = a * z;
        f32 t3 = b * a;
        f32 t2 = b * x;
        f32 f9 = t1 - t0;
        f32 f10 = t4 - t3;
        f32 f8 = t2 - t4;
        len2 = (f9 * f9) + ((f10 * f10) + (f8 * f8));
        vecB[0] = a;
        vecB[1] = a;
        vecB[2] = b;
        if (a == len2) {
            vecA[0] = a;
            vecA[1] = a;
            vecA[2] = a;
        } else {
            inv = (f32)(1.0 / sqrt((f64)len2));
            vecA[0] = f10 * inv;
            vecA[1] = f8 * inv;
            vecA[2] = f9 * inv;
        }
    }
    fn_801458B0((f32*)(*(u8**)(arg0 + 0x4) + 0xC), vecA, vecB);
    (*(void (**)(u8*))(*(u8**)arg0 + 0x94))(arg0);
    return arg0;
}

// 0x800D8AD0
void fn_800D8AD0(u8* arg0, u8* arg1) {
    u8* q = *(u8**)(arg0 + 0x4);
    f32 qx = *(f32*)(q + 0xC);
    f32 qy = *(f32*)(q + 0x10);
    f32 qz = *(f32*)(q + 0x14);
    f32 qw = *(f32*)(q + 0x18);
    f32 n2 = (qx * qx) + ((qy * qy) + ((qz * qz) + (qw * qw)));
    f32 X, Y, Z, W;
    f32 t3, t12, t11, t8, t7, t0, t9, t4, t3b, t0b, t5, m0, t3c, m1, t0c, t9b, t4b, m2, t7b, t8b, t4c, t0d, t9c, t7c;

    if (n2 > lbl_805FE680 && n2 < lbl_805FE684) {
        X = lbl_805FE668;
        Y = lbl_805FE668;
        Z = lbl_805FE668;
        W = lbl_805FE66C;
    } else {
        f32 inv = lbl_805FE66C / n2;
        X = qx * inv;
        Y = qy * inv;
        Z = qz * inv;
        W = qw * inv;
    }

    t3 = lbl_805FE688 * X;
    t12 = lbl_805FE688 * Y;
    t11 = lbl_805FE688 * Z;
    t8 = t3 * X;
    t7 = t12 * Y;
    t0 = t11 * Z;
    t9 = t3 * W;
    t4 = t11 * Y;
    t3b = t7 + t0;
    t0b = t8 + t0;
    t5 = t4 + t9;
    m0 = lbl_805FE66C - t3b;
    t3c = t4 - t9;
    m1 = lbl_805FE66C - t0b;
    t0c = t8 + t7;
    t9b = t12 * W;
    t4b = t11 * X;
    m2 = lbl_805FE66C - t0c;
    t7b = t11 * W;
    t8b = t4b - t9b;
    t4c = t4b + t9b;
    t0d = t12 * X;
    t9c = t0d + t7b;
    t7c = t0d - t7b;

    *(f32*)(arg1 + 0x0) = m0;
    *(f32*)(arg1 + 0x4) = t9c;
    *(f32*)(arg1 + 0x8) = t8b;
    *(f32*)(arg1 + 0xC) = t7c;
    *(f32*)(arg1 + 0x10) = m1;
    *(f32*)(arg1 + 0x14) = t5;
    *(f32*)(arg1 + 0x18) = t4c;
    *(f32*)(arg1 + 0x1C) = t3c;
    *(f32*)(arg1 + 0x20) = m2;
    *(f32*)(arg1 + 0x24) = *(f32*)(q + 0x0);
    *(f32*)(arg1 + 0x28) = *(f32*)(q + 0x4);
    *(f32*)(arg1 + 0x2C) = *(f32*)(q + 0x8);
}

// 0x80120E00
void fn_80120E00(u8* arg0, f32 farg0) {
    f32 t30;
    f32 f31;
    u8 idx;

    if (*(s32*)(arg0 + 0x1C) == 0) {
        return;
    }
    t30 = *(f32*)(arg0 + 0x30);
    idx = *(u8*)(arg0 + 0x20);
    *(f32*)(arg0 + 0x4) = farg0;
    f31 = t30 + (farg0 * ((*(f32*)(arg0 + 0x34) - t30) / lbl_805FED58));

    if (idx < 5) {
        u8* nxt = *(u8**)(arg0 + 0x8 + (idx + 1) * 4);
        if (*(f32*)(nxt + 0x68) < f31) {
            u8* cur = *(u8**)(arg0 + 0x8 + idx * 4);
            u8 nidx;
            u8* cur2;
            u8* cur3;
            (*(void (**)(u8*, s32))(*(u8**)(cur + 0x138) + 0x30))(cur, 2);
            nidx = (u8)(*(u8*)(arg0 + 0x20) + 1);
            *(u8*)(arg0 + 0x20) = nidx;
            cur2 = *(u8**)(arg0 + 0x8 + nidx * 4);
            (*(void (**)(u8*, s32))(*(u8**)(cur2 + 0x138) + 0x2C))(cur2, 2);
            cur3 = *(u8**)(arg0 + 0x8 + *(u8*)(arg0 + 0x20) * 4);
            *(f32*)(arg0 + 0x24) = *(f32*)(cur3 + 0x68);
            *(f32*)(arg0 + 0x28) = *(f32*)(cur3 + 0x6C);
            *(f32*)(arg0 + 0x2C) = *(f32*)(cur3 + 0x70);
        }
    }

    idx = *(u8*)(arg0 + 0x20);
    if (idx >= 5) {
        return;
    }
    {
        u8* nx = *(u8**)(arg0 + 0x8 + (idx + 1) * 4);
        f32 f2 = *(f32*)(arg0 + 0x24);
        f32 f4 = (f31 - f2) / (*(f32*)(nx + 0x68) - f2);
        f32 f3 = *(f32*)(arg0 + 0x28);
        f32 f2b = *(f32*)(nx + 0x6C) - f3;
        f32 f0b = *(f32*)(arg0 + 0x2C);
        u8* cur = *(u8**)(arg0 + 0x8 + idx * 4);
        f32 v;
        u8* cur2;
        *(f32*)(cur + 0x68) = f31;
        v = f3 + (f4 * f2b);
        *(f32*)(cur + 0x6C) = v;
        *(f32*)(cur + 0x70) = f0b;
        cur2 = *(u8**)(arg0 + 0x8 + *(u8*)(arg0 + 0x20) * 4);
        (*(void (**)(u8*, s32, f32, f32, f32))(*(u8**)(cur2 + 0x138) + 0x2C))(cur2, 8, v, f2b, f3);
    }
}

// 0x80149828
void fn_80149828(u8* arg0, u32* arg1, u8* arg2, u8* arg3, u8* arg4, u8* arg5) {
    s32 kind = *(u8*)(arg0 + 0x9);
    s32 sub;
    u8* base;

    if (kind == 4 || kind == 11 || kind == 5) {
        sub = *(u8*)(arg0 + 0xC);
        base = *(u8**)(arg0 + 0x1C);
        if (sub == 6) {
            u32 v = *(u16*)(base + *arg1 * 2);
            *arg1 = v;
            if (((v >> 15) & 1) == 0) {
                *arg2 = (u8)((v >> 12) & 7);
                *arg3 = (u8)((v >> 8) & 0xF);
                *arg5 = (u8)(v & 0xF);
                *arg4 = (u8)((v >> 4) & 0xF);
                return;
            }
            *arg2 = 0xF;
            *arg3 = (u8)((v >> 10) & 0x1F);
            *arg5 = (u8)(v & 0x1F);
            *arg4 = (u8)((v >> 5) & 0x1F);
            return;
        }
        if (sub == 3) {
            u32 v = *(u32*)(base + *arg1 * 4);
            *arg1 = v;
            *arg2 = (u8)(v >> 24);
            *arg3 = (u8)v;
            *arg5 = (u8)(v >> 16);
            *arg4 = (u8)(v >> 8);
            return;
        }
        if (sub == 5) {
            u32 v = *(u16*)(base + *arg1 * 2);
            *arg1 = v;
            *arg2 = 0xF;
            *arg3 = (u8)((v >> 11) & 0x1F);
            *arg5 = (u8)(v & 0x1F);
            *arg4 = (u8)((v >> 5) & 0x3F);
            return;
        }
        if (sub == 4) {
            u32 v = *(u16*)(base + *arg1 * 2);
            *arg1 = v;
            *arg2 = (u8)(v >> 8);
            *arg3 = (u8)v;
            *arg5 = (u8)v;
            *arg4 = (u8)v;
            return;
        }
        *arg2 = 0xF;
        return;
    }
    if (kind == 22) {
        u32 v = *arg1;
        *arg2 = (u8)(v >> 24);
        *arg3 = (u8)(v >> 16);
        *arg5 = (u8)v;
        *arg4 = (u8)(v >> 8);
        return;
    }
    if (kind == 3) {
        u32 v = *arg1;
        *arg2 = (u8)(v >> 24);
        *arg3 = (u8)v;
        *arg5 = (u8)(v >> 16);
        *arg4 = (u8)(v >> 8);
        return;
    }
    *arg2 = 0xF;
}

// 0x80198014
s32 fn_80198014(u8* arg0, u8* arg1, const char* arg2) {
    u8 handle[0x10];
    u8 state[0x10];
    char name[0x100];
    char path[0x100];
    u8* buf;
    s32 size = 0;
    u8* list;

    SysSprintf(path, lbl_8038236C, arg2, lbl_805F93C0);
    if (fn_801EE46C(handle, path) == 0) {
        buf = NULL;
    } else {
        size = fn_801EE504(handle);
        buf = (u8*)MemAllocDefault((u32)size);
        fn_801EE50C(handle, buf, size, 1);
        fn_801EE4B8(handle);
    }
    if (buf == NULL) {
        return 0;
    }
    fn_8016B5E0(state, buf, size);
    list = lbl_805FB5A8;
    *(s32*)(arg0 + 0x124) = 0;
    for (;;) {
        s32 t = fn_8016B6DC(state);
        if (t == 0) {
            break;
        }
        if (t == 3) {
            s32 idx;
            u8* slot;
            SysSprintf(name, lbl_805F93C8, arg2, *(s32*)(arg0 + 0x124));
            idx = *(s32*)(arg0 + 0x124);
            *(u8**)(arg0 + idx * 4 + 0x24) = fn_8016B724(state, list, name);
            if (*(u8**)(arg0 + idx * 4 + 0x24) == NULL && list != NULL) {
                u8* r = fn_80169770(list, name);
                if (r != NULL) {
                    *(s32*)(r + 0x24) = *(s32*)(r + 0x24) + 1;
                    *(u8**)(arg0 + idx * 4 + 0x24) = r;
                }
            }
            slot = *(u8**)(arg0 + idx * 4 + 0x24);
            if (slot != NULL && idx < *(u8*)(arg1 + 0x3)) {
                *(u8**)(arg1 + idx * 4 + 0x8C) = slot + 4;
                *(s32*)(slot + 0x24) = *(s32*)(slot + 0x24) + 1;
                *(s32*)(arg0 + 0x124) = idx + 1;
                if (idx + 1 == 0x40) {
                    break;
                }
            }
        } else {
            fn_8016B704(state);
        }
    }
    BufferFree(buf);
    return *(s32*)(arg0 + 0x124);
}

// 0x8019AA3C
void fn_8019AA3C(u8* arg0, s32 arg1, s32 arg2) {
    s32 cur_y = *(s32*)(arg0 + 0x10);
    s32 old_x;
    u8* a3;

    if (cur_y != arg2 || *(s32*)(arg0 + 0xC) != arg1) {
        u8* cell = NULL;
        u8* sub = NULL;
        u8* spr = NULL;
        old_x = *(s32*)(arg0 + 0xC);
        if (fn_8019555C(*(u8**)(arg0 + 0x4)) > old_x) {
            cell = *(u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x4), old_x);
        }
        if (cell == NULL) {
            fn_8018D2B8("UIActivationGrid : index (%d,%d) not valid ...\n", old_x, cur_y);
            spr = NULL;
        } else {
            if (fn_8019555C(cell) > cur_y) {
                sub = (u8*)(uintptr_t)fn_8019552C(cell, cur_y);
            }
            if (sub == NULL) {
                fn_8018D2B8("UIActivationGrid : index (%d,%d) not valid ...\n", old_x, 0);  // 2nd %d: r5 clobbered by earlier calls, passed 0
                spr = NULL;
            } else {
                spr = *(u8**)sub;
            }
        }
        if (spr != NULL) {
            UIPlayAnim(spr, 0xF5A9CE6CU);
        }

        *(s32*)(arg0 + 0x18) = cur_y;
        *(s32*)(arg0 + 0x14) = old_x;
        *(s32*)(arg0 + 0xC) = arg1;
        *(s32*)(arg0 + 0x10) = arg2;

        cell = NULL;
        sub = NULL;
        if (fn_8019555C(*(u8**)(arg0 + 0x4)) > arg1) {
            cell = *(u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x4), arg1);
        }
        if (cell == NULL) {
            fn_8018D2B8("UIActivationGrid : index (%d,%d) not valid ...\n", arg1, arg2);
            a3 = NULL;
        } else {
            if (fn_8019555C(cell) > arg2) {
                sub = (u8*)(uintptr_t)fn_8019552C(cell, arg2);
            }
            if (sub == NULL) {
                fn_8018D2B8("UIActivationGrid : index (%d,%d) not valid ...\n", arg1, 0);  // 2nd %d: r5 clobbered by earlier calls, passed 0
                a3 = NULL;
            } else {
                a3 = *(u8**)sub;
            }
        }
        if (a3 != NULL) {
            UIPlayAnim(a3, 0x705598E1U);
        }
    }
}

// 0x801A8E80
void fn_801A8E80(u8* arg0, s32 arg1) {
    f32 p8[3];
    f32 p14[3];
    f32 fx[4];

    if (arg1 == 0 && *(u8**)(arg0 + 0x178) != NULL) {
        fn_801A904C(arg0);
        {
            u8* L = *(u8**)(arg0 + 0x178);
            if (L != NULL) {
                s32 i;
                s32 off;
                for (i = 0, off = 0; i < *(s32*)(L + 0xC); i++, off += 4) {
                    u8* e = *(u8**)(L + off + 0x14);
                    if (e != NULL) {
                        *(s32*)(e + 0x74) = 0;
                    }
                }
            }
        }
        fn_801C83AC(lbl_8047C760, *(u8**)(arg0 + 0x178));
        *(u8**)(arg0 + 0x178) = NULL;
    }
    if (*(u8**)(arg0 + 0x178) == NULL && arg1 != 0 && (u32)(*(s32*)(arg0 + 0x140) + 0x10000) != 0xFFFFu) {
        fn_801A9174(arg0, p8, p14);
        fx[0] = p8[0];
        fx[1] = p8[1];
        fx[2] = p8[2];
        fx[3] = lbl_805FFA38;
        SpawnEffectAt(lbl_8047C760, (u32)*(s32*)(arg0 + 0x140), fx, (u8**)(arg0 + 0x178));
        {
            u8* L = *(u8**)(arg0 + 0x178);
            if (L != NULL) {
                s32 i;
                s32 off;
                *(u8*)(L + 0x8) = 0;
                for (i = 0, off = 0; i < *(s32*)(L + 0xC); i++, off += 4) {
                    u8* e = *(u8**)(L + off + 0x14);
                    if (e != NULL) {
                        *(u32*)e &= 0xFFFFFFFBu;
                    }
                }
                for (i = 0, off = 0; i < *(s32*)(L + 0xC); i++, off += 4) {
                    u8* e = *(u8**)(L + off + 0x14);
                    if (e != NULL) {
                        *(u8**)(e + 0x74) = arg0;
                        *(s32*)(e + 0x7C) = *(s32*)(lbl_8047C760 + 0x24);
                    }
                }
            }
        }
        (*(void (**)(u8*, s32))(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x10);
    }
}

// 0x801F5540
void fn_801F5540(void) {
    u8* base = lbl_804DD180;
    s32 k;

    lbl_805FB980 = 0;
    lbl_805FB984 = 0;
    lbl_805FB988 = 0;
    lbl_805FB98C = 0;
    lbl_805FB990 = 0;
    if (lbl_805FB95C != NULL) {
        fn_801F5B30(lbl_805FB95C);
        fn_801F3BB8();
    }
    lbl_805F97F8 = 1;
    lbl_805F97FC = 1;
    fn_801F5E3C(base + 0x3C4, lbl_80385FC0, 1);
    *(s32*)(base + 0x3C0) = 0;
    fn_801F5E4C(base + 0x3C4, 0);
    for (k = 0; k < 2; k++) {
        u8* p = base + k * 0x1E0;
        if (*(u8*)(p + 0x94) != 0) {
            *(u8*)(p + 0x94) = 0;
            *(s32*)(base + 0x3C0) = *(s32*)(base + 0x3C0) - 1;
            if (*(s32*)(base + 0x3C0) < 0) {
                *(s32*)(base + 0x3C0) = 0;
            }
        }
        if (*(u8*)(p + 0x134) != 0) {
            *(u8*)(p + 0x134) = 0;
            *(s32*)(base + 0x3C0) = *(s32*)(base + 0x3C0) - 1;
            if (*(s32*)(base + 0x3C0) < 0) {
                *(s32*)(base + 0x3C0) = 0;
            }
        }
        if (*(u8*)(p + 0x1D4) != 0) {
            *(u8*)(p + 0x1D4) = 0;
            *(s32*)(base + 0x3C0) = *(s32*)(base + 0x3C0) - 1;
            if (*(s32*)(base + 0x3C0) < 0) {
                *(s32*)(base + 0x3C0) = 0;
            }
        }
    }
    fn_801F5E48(base + 0x3C4, 0);
    *(s32*)(lbl_804DD568 + 0xC) = -1;
    *(s32*)(lbl_804DD568 + 0x8) = -1;
    *(s32*)(lbl_804EC590 + 0xC) = -1;
    *(s32*)(lbl_804EC590 + 0x8) = -1;
    lbl_805FB9A8 = 0;
    lbl_805FB9AC = 0;
    lbl_805FB96C = 0;
    lbl_805FB9D4 = 0;
    lbl_805FB9B0 = 0;
    lbl_805FB9B4 = 0;
    lbl_805FB9B8 = 0;
    lbl_805FB9BC = 0;
    lbl_805FB9C0 = 0;
    lbl_805FB9C4 = 0;
    lbl_805FB9C8 = 0;
    lbl_805FB9CC = 0;
    *(s32*)(lbl_804FB5C4 + 0x0) = 0;
    *(s32*)(lbl_804FB5C4 + 0xC) = 0;
    *(s32*)(lbl_804FB5C4 + 0x8) = 0;
    *(s32*)(lbl_804FB5C4 + 0x10) = 0;
    *(s32*)(lbl_804FB5A8 + 0x0) = 0;
    *(s32*)(lbl_804FB5A8 + 0xC) = 0;
    *(s32*)(lbl_804FB5A8 + 0x8) = 0;
    *(s32*)(lbl_804FB5A8 + 0x10) = 0;
    lbl_805FB994 = 0;
}

// 0x801FA920
void fn_801FA920(u8* arg0) {
    u8* list = fn_801483BC(*(s32*)(arg0 + 0x4));
    s32 idx = *(s32*)(list + 0x14) - 1;
    f32 thr = lbl_80600280;
    f32 zero = lbl_8060028C;

    for (; idx >= 0; idx--) {
        u8* e;
        f32 px, py, pz;
        s32 c;

        e = *(u8**)(list + 0x4) + (((*(s32*)(list + 0x10) + idx) % *(s32*)(list + 0xC)) * 0x84);
        if (*(s32*)(arg0 + 0x98) == 0) {
            px = *(f32*)(e + 0xC);
            py = *(f32*)(e + 0x10);
            pz = *(f32*)(e + 0x14);
        } else {
            px = *(f32*)(e + 0x68);
            py = *(f32*)(e + 0x6C);
            pz = *(f32*)(e + 0x70);
        }
        c = *(s32*)(arg0 + 0xA8);
        if (c != 0) {
            if (px > thr || py > thr || pz > thr) {
                *(s32*)(arg0 + 0xA8) = 0x3C;
            } else {
                *(s32*)(arg0 + 0xA8) = c - 1;
            }
        } else if ((f32)fabs(px) > thr) {
            *(s32*)(arg0 + 0xA8) = 0x3C;
            *(f32*)(arg0 + 0xAC) = (px > zero) ? lbl_80600284 : lbl_80600288;
            *(f32*)(arg0 + 0xB0) = zero;
            *(u8*)(arg0 + 0x90) = 1;
        } else if ((f32)fabs(py) > thr) {
            *(s32*)(arg0 + 0xA8) = 0x3C;
            *(f32*)(arg0 + 0xAC) = zero;
            *(f32*)(arg0 + 0xB0) = (py > zero) ? lbl_80600284 : lbl_80600288;
            *(u8*)(arg0 + 0x90) = 1;
        }
    }
}

}  // extern "C"
