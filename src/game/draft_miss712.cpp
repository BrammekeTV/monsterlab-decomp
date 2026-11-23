// Small functions from draft_miss712 (verified against tools/dump.sh asm).
// Skipped (see report): fn_801A5830, fn_801A5C94 (leftover r4 passed to a 1-arg helper),
// fn_801A8B88 (4-byte-stride pointer list), fn_801AD248 (leftover r4 from Vec3Assign return).
#include "types.h"
#include <math.h>

extern "C" {
s32   fn_800DECD8(u8* p, f32 dt);                                        // 0x800DECD8
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);           // 0x80209ABC
s32   TriggerDoAction(void* action, void* a1, void* a2);                 // 0x801E0054
void  fn_8020A398(s32 handle);                                           // 0x8020A398
void  fn_8018D1FC(const char* fmt, ...);                                 // 0x8018D1FC
void  fn_8018D2B8(const char* fmt, ...);                                 // 0x8018D2B8 (debug log)
u8*    fn_8019552C(u8* list, s32 index);                                  // 0x8019552C
s32   fn_801A6384(u8* grid);                                             // grid helper
s32   fn_801A6060(u8* grid);                                             // grid helper
u32   fn_801A665C(void* obj);                                            // 0x801A665C
u32   fn_801A65A0(void* obj);                                            // 0x801A65A0
void  fn_801F5724(u32 button);                                           // 0x801F5724
BOOL  fn_801F56FC(s32 flags);                                            // 0x801F56FC
s32   fn_801F43B0(void);                                       // 0x801F43B0 (result unused by callers here)
void  OSUnlockMutex(u8* p);                                                // 0x8023A68C
void  OSSleepTicks(s32 hi, u32 lo);                                       // 0x8023D3C8
f32   fn_8033F1FC(f32 y, f32 x);                                         // 0x8033F1FC (MSL atan2)
u8*   fn_801483BC(s32 idx);                                              // 0x801483BC
void  fn_8020F384(void* choice, s32 action, s32 flag, s32 limb);         // 0x8020F384
void* ChoiceDestroy(void* c, s32 del);                                   // 0x8020F394
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle); // 0x8032EFC8
void  fn_8020890C(u8* self);                                             // 0x8020890C
u32   OSGetBusClock(void);                                               // 0x800000F8 read

extern u8  lbl_803C4298[];
extern u8  lbl_8055CA08[];
extern u8  lbl_80556AB0[];
extern u8  lbl_80556AC8[];
extern u8  lbl_805FBB54[];
extern s32 lbl_805F9410;
extern f32 lbl_805F9408;
extern f32 lbl_805FF958;
extern f32 lbl_805FF95C;
extern f32 lbl_805FF978;
extern f32 lbl_805FE978;
extern f32 lbl_805FE97C;
extern f32 lbl_805FE980;
extern f32 lbl_805FE984;
extern f32 lbl_805FE988;
extern s32 lbl_805FB380;
extern void* lbl_805FB964;
extern s32 lbl_805FB970;
extern s32 lbl_805FB974;
extern s32 lbl_805FB978;
extern s32 lbl_805FB994;
extern f32 lbl_805FB918;
extern f32 lbl_805FBB3C;
extern s32 lbl_805FBB6C;
extern f32 lbl_80600110;
extern f32 lbl_80600114;
extern f32 lbl_80600118;
extern f32 lbl_8060011C;
extern f32 lbl_80600120;
extern f32 lbl_80600124;
extern f32 lbl_80600128;
extern f32 lbl_8060012C;
extern f32 lbl_80600480;
extern f32 lbl_80600484;
extern f32 lbl_805FFAB8;
extern f32 lbl_805FFABC;
extern f32 lbl_805FFAC4;
}

extern "C" {

// 0x800E3040
s32 fn_800E3040(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    if (fn_800DECD8(arg0, farg0) == 0) {
        return 0;
    }
    u8* cam = *(u8**)(lbl_803C4298 + 0x8);
    f32 camX = *(f32*)(cam + 0x80);
    f32 camY = *(f32*)(cam + 0x84);
    f32 camZ = *(f32*)(cam + 0x88);
    f32 camA = *(f32*)(cam + 0x98);
    f32 camB = *(f32*)(cam + 0x9C);
    f32 camC = *(f32*)(cam + 0xA0);

    // Blend weight from the cosine of the angle at +0x44.
    f32 c = cosf(lbl_805FE97C * (lbl_805FE980 * *(f32*)(arg0 + 0x44)));
    f32 t = lbl_805FE984 * (lbl_805FE988 + c);
    f32 w = lbl_805FE988 * (lbl_805FE988 - t) + lbl_805FE978 * t;

    *(f32*)(arg0 + 0x68) = camA;
    *(f32*)(arg0 + 0x6C) = camB;
    *(f32*)(arg0 + 0x70) = camC;

    u8* pos = *(u8**)(arg0 + 0x58);
    f32 ox = (*(f32*)(pos + 0x8) - camX) * w + camX;
    f32 oy = (*(f32*)(pos + 0xC) - camY) * w + camY;
    f32 oz = (*(f32*)(pos + 0x10) - camZ) * w + camZ;
    *(f32*)(arg0 + 0x5C) = ox;
    *(f32*)(arg0 + 0x60) = oy;
    *(f32*)(arg0 + 0x64) = oz;
    *(f32*)(arg1 + 0x0) = ox;
    *(f32*)(arg1 + 0x4) = oy;
    *(f32*)(arg1 + 0x8) = oz;

    f32 vx = camA - ox;
    f32 vy = camB - oy;
    f32 vz = camC - oz;
    *(f32*)(arg2 + 0x0) = vx;
    *(f32*)(arg2 + 0x4) = vy;
    *(f32*)(arg2 + 0x8) = vz;

    f32 d2 = vz * vz + (vx * vx + vy * vy);
    if (d2 == lbl_805FE978) {
        *(f32*)(arg2 + 0x0) = lbl_805FE978;
        *(f32*)(arg2 + 0x4) = lbl_805FE978;
        *(f32*)(arg2 + 0x8) = lbl_805FE978;
    } else {
        // asm uses frsqrte + Newton steps; sqrtf gives the same single-precision result.
        f32 inv = 1.0f / sqrtf(d2);
        *(f32*)(arg2 + 0x0) = vx * inv;
        *(f32*)(arg2 + 0x4) = vy * inv;
        *(f32*)(arg2 + 0x8) = vz * inv;
    }
    return 1;
}

// 0x8019E838
void fn_8019E838(u8* arg0, f32 farg0) {
    // Per-entry alpha for the color table at +0x178 from the progress value at +0x14C.
    s32 idx = 0;
    s32 off = 0;
    for (; idx < *(s32*)(arg0 + 0x150); idx++, off += 4) {
        f32 a = *(f32*)(arg0 + 0x14C) * lbl_805F9408 - (f32)idx;
        if (a < lbl_805FF958) {
            a = lbl_805FF958;
        } else if (a > lbl_805FF95C) {
            a = lbl_805FF95C;
        }
        u8* arr = *(u8**)(arg0 + 0x178);
        *(u32*)(arr + off) = *(u32*)(arr + off) & 0xFFFFFFu;
        arr = *(u8**)(arg0 + 0x178);
        *(u32*)(arr + off) = *(u32*)(arr + off) | ((u32)(s32)(lbl_805FF978 * a) << 24);
    }

    // Walk the 12-byte event list (type, time, data) up to the current progress.
    u8* head = *(u8**)(arg0 + 0x144);
    u8* e;
    if (head != NULL) {
        if (*(s32*)head == -1) {
            return;
        }
        e = head + 0xC;
    } else {
        e = *(u8**)(arg0 + 0x140);
    }
    if (e == NULL || *(s32*)e == -1) {
        return;
    }
    f32 progress = *(f32*)(arg0 + 0x14C) * lbl_805F9408;
    for (;;) {
        if (!(progress >= (f32)*(s32*)(e + 0x4))) {
            return;
        }
        s32 type = *(s32*)(e + 0x0);
        if (type == -1) {
            *(u8**)(arg0 + 0x144) = e;
            return;
        }
        if (type == 0) {
            fn_8018D1FC("color tag at index %d\n");
        } else if (type == 1) {
            if (lbl_805F9410 != -1) {
                fn_8020A398(lbl_805F9410);
                lbl_805F9410 = -1;
            }
            lbl_805F9410 = PlaySoundAt((void*)(intptr_t)*(s32*)(e + 0x8), 0, NULL, 0);
        } else if (type == 2) {
            s32 action = *(s32*)(e + 0x8);
            TriggerDoAction(&action, NULL, NULL);
        }
        *(u8**)(arg0 + 0x144) = e;
        e += 0xC;
    }
}

// 0x801A5608
s32 fn_801A5608(u8* arg0) {
    s32 var_r30 = 0;
    *(s32*)(arg0 + 0x154) = *(s32*)(arg0 + 0x14C);
    *(s32*)(arg0 + 0x158) = *(s32*)(arg0 + 0x150);
    if (*(s32*)(arg0 + 0x64) & 0x40000) {
        if (fn_801A6384(arg0) == 0) {
            fn_8018D2B8("UIGrid : unable to scroll grid to valid item ...\n");
            return -1;
        }
        return 0;
    }
    for (;;) {
        var_r30 += 1;
        s32 temp_r4 = *(s32*)(arg0 + 0x144);
        s32 temp_r0 = *(s32*)(arg0 + 0x14C) + 1;
        *(s32*)(arg0 + 0x14C) = temp_r0;
        if (temp_r0 >= temp_r4) {
            *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x154);
            if ((*(s32*)(arg0 + 0x64) & 0x80000) || (temp_r4 < *(s32*)(arg0 + 0x13C))) {
                if (fn_801A6384(arg0) == 0) {
                    u8* b = *(u8**)(arg0 + 0x1B4);
                    if (b != NULL && !(*(s32*)(b + 0x64) & 0x100000)) {
                        *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x154);
                    } else {
                        *(s32*)(arg0 + 0x14C) = 0;
                        if ((s32)fn_801A665C(arg0) > *(s32*)(arg0 + 0x144)) {
                            *(s32*)(arg0 + 0x15C) = -1;
                            fn_801A6384(arg0);
                        }
                    }
                }
            } else {
                u8* b = *(u8**)(arg0 + 0x1B4);
                if (b != NULL && (*(s32*)(b + 0x64) & 0x100000)) {
                    *(s32*)(arg0 + 0x14C) = 0;
                } else if (*(s32*)(arg0 + 0x14C) == *(s32*)(arg0 + 0x154)) {
                    return 0;
                }
            }
        }

        s32 r4 = *(s32*)(arg0 + 0x14C) + *(s32*)(arg0 + 0x15C);
        s32 r5 = *(s32*)(arg0 + 0x150) + *(s32*)(arg0 + 0x160);
        if (r4 < 0) {
            r4 += *(s32*)(arg0 + 0x13C);
        } else if (r4 >= *(s32*)(arg0 + 0x13C)) {
            r4 -= *(s32*)(arg0 + 0x13C);
        }
        if (r5 < 0) {
            r5 += *(s32*)(arg0 + 0x140);
        } else if (r5 >= *(s32*)(arg0 + 0x140)) {
            r5 -= *(s32*)(arg0 + 0x140);
        }
        u8** slot = (u8**)fn_8019552C(*(u8**)(arg0 + 0x1BC), r5 + r4 * *(s32*)(arg0 + 0x140));
        u8* elem = (slot != NULL) ? *slot : NULL;
        s32 var_r27 = 0;
        if (elem != NULL) {
            var_r27 = *(s32*)(elem + 0x64) & 1;
        }
        s32 var_r0 = 0;
        if (elem != NULL && var_r27 != 0) {
            var_r0 = 1;
        }
        if (var_r0 != 0 || var_r30 > *(s32*)(arg0 + 0x13C)) {
            if (var_r30 > *(s32*)(arg0 + 0x13C)) {
                *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x154);
            }
            break;
        }
    }
    return 0;
}

// 0x801A5A6C
s32 fn_801A5A6C(u8* arg0) {
    s32 var_r30 = 0;
    *(s32*)(arg0 + 0x158) = *(s32*)(arg0 + 0x150);
    *(s32*)(arg0 + 0x154) = *(s32*)(arg0 + 0x14C);
    if (*(s32*)(arg0 + 0x64) & 0x40000) {
        if (fn_801A6060(arg0) == 0) {
            fn_8018D2B8("UIGrid : unable to scroll grid to valid item ...\n");
            return -1;
        }
        return 0;
    }
    for (;;) {
        var_r30 += 1;
        s32 temp_r4 = *(s32*)(arg0 + 0x148);
        s32 temp_r0 = *(s32*)(arg0 + 0x150) + 1;
        *(s32*)(arg0 + 0x150) = temp_r0;
        if (temp_r0 >= temp_r4) {
            *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x158);
            if ((*(s32*)(arg0 + 0x64) & 0x10000) || (temp_r4 < *(s32*)(arg0 + 0x140))) {
                if (fn_801A6060(arg0) == 0) {
                    u8* b = *(u8**)(arg0 + 0x1B4);
                    if (b != NULL && !(*(s32*)(b + 0x64) & 0x20000)) {
                        *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x158);
                    } else {
                        *(s32*)(arg0 + 0x150) = 0;
                        if ((s32)fn_801A65A0(arg0) > *(s32*)(arg0 + 0x148)) {
                            *(s32*)(arg0 + 0x160) = -1;
                            fn_801A6060(arg0);
                        }
                    }
                }
            } else {
                u8* b = *(u8**)(arg0 + 0x1B4);
                if (b != NULL && (*(s32*)(b + 0x64) & 0x20000)) {
                    *(s32*)(arg0 + 0x150) = 0;
                } else if (*(s32*)(arg0 + 0x150) == *(s32*)(arg0 + 0x158)) {
                    return 0;
                }
            }
        }

        s32 r4 = *(s32*)(arg0 + 0x14C) + *(s32*)(arg0 + 0x15C);
        s32 r5 = *(s32*)(arg0 + 0x150) + *(s32*)(arg0 + 0x160);
        if (r4 < 0) {
            r4 += *(s32*)(arg0 + 0x13C);
        } else if (r4 >= *(s32*)(arg0 + 0x13C)) {
            r4 -= *(s32*)(arg0 + 0x13C);
        }
        if (r5 < 0) {
            r5 += *(s32*)(arg0 + 0x140);
        } else if (r5 >= *(s32*)(arg0 + 0x140)) {
            r5 -= *(s32*)(arg0 + 0x140);
        }
        u8** slot = (u8**)fn_8019552C(*(u8**)(arg0 + 0x1BC), r5 + r4 * *(s32*)(arg0 + 0x140));
        u8* elem = (slot != NULL) ? *slot : NULL;
        s32 var_r27 = 0;
        if (elem != NULL) {
            var_r27 = *(s32*)(elem + 0x64) & 1;
        }
        s32 var_r0 = 0;
        if (elem != NULL && var_r27 != 0) {
            var_r0 = 1;
        }
        if (var_r0 != 0 || var_r30 > *(s32*)(arg0 + 0x140)) {
            if (var_r30 > *(s32*)(arg0 + 0x140)) {
                *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x158);
            }
            break;
        }
    }
    return 0;
}

// 0x801AD47C
s32 fn_801AD47C(u8* arg0, u8* arg1, u8* arg2, u8* arg3, s32 arg4, f32 farg0) {
    s32 useAxis = arg4;
    f32 dx = *(f32*)(arg2 + 0x0) - *(f32*)(arg0 + 0x0);
    f32 dy = *(f32*)(arg2 + 0x4) - *(f32*)(arg0 + 0x4);
    f32 dz = *(f32*)(arg2 + 0x8) - *(f32*)(arg0 + 0x8);
    f32 ox = (farg0 + *(f32*)(arg1 + 0x0)) - fabsf(dx);
    if (ox < lbl_805FFAB8) {
        return 0;
    }
    f32 oy = (farg0 + *(f32*)(arg1 + 0x4)) - fabsf(dy);
    if (oy < lbl_805FFAB8) {
        return 0;
    }
    f32 oz = (farg0 + *(f32*)(arg1 + 0x8)) - fabsf(dz);
    if (oz < lbl_805FFAB8) {
        return 0;
    }
    if (oz > lbl_805FFAB8) {
        useAxis = 0;
    }

    // Pick the axis of least penetration; nx/ny/nz is the push direction, m the depth.
    f32 m;
    f32 nx;
    f32 ny;
    f32 nz;
    if (fabsf(ox) < fabsf(oy)) {
        if (useAxis != 0 || fabsf(ox) < fabsf(oz)) {
            m = fabsf(ox);
            nx = (dx < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
            ny = lbl_805FFAB8;
            nz = lbl_805FFAB8;
        } else {
            m = fabsf(oz);
            nx = lbl_805FFAB8;
            ny = lbl_805FFAB8;
            nz = (dz < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
        }
    } else {
        if (useAxis != 0 || fabsf(oy) < fabsf(oz)) {
            m = fabsf(oy);
            nx = lbl_805FFAB8;
            ny = (dy < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
            nz = lbl_805FFAB8;
        } else {
            m = fabsf(oz);
            nx = lbl_805FFAB8;
            ny = lbl_805FFAB8;
            nz = (dz < lbl_805FFAB8) ? lbl_805FFAC4 : lbl_805FFABC;
        }
    }

    f32 px = *(f32*)(arg0 + 0x0) + nx * m;
    f32 py = *(f32*)(arg0 + 0x4) + ny * m;
    f32 pz = *(f32*)(arg0 + 0x8) + nz * m;
    *(f32*)(arg3 + 0x0) = px;
    *(f32*)(arg3 + 0x4) = py;
    *(f32*)(arg3 + 0x8) = pz;
    *(f32*)(arg3 + 0xC) = nx;
    *(f32*)(arg3 + 0x10) = ny;
    *(f32*)(arg3 + 0x14) = nz;
    *(f32*)(arg3 + 0x18) = m;
    return 1;
}

// 0x801F6610
f32 fn_801F6610(u8* arg0, s32 arg1) {
    u8* T = fn_801483BC(*(s32*)(arg0 + 0x4));
    s32 n = *(s32*)(T + 0xC);
    s32 s = *(s32*)(T + 0x10) + arg1;
    u8* arr = *(u8**)(T + 0x4);

    u8* e0 = arr + (s % n) * 0x84;
    f32 ang1 = fn_8033F1FC(*(f32*)(e0 + 0x38), *(f32*)(e0 + 0x34));
    f32 first = lbl_80600114 + lbl_80600114 * (lbl_80600110 * ang1);

    u8* e1 = arr + ((s + 1) % n) * 0x84;
    u8* e2 = arr + (s % n) * 0x84;
    u8* e3 = arr + ((s + 2) % n) * 0x84;
    u8* e4 = arr + ((s + 3) % n) * 0x84;
    u8* e5 = arr + ((s + 4) % n) * 0x84;

    f32 xa = lbl_80600118 * *(f32*)(e2 + 0xC) + lbl_8060011C * *(f32*)(e1 + 0xC);
    f32 ya = lbl_80600118 * *(f32*)(e2 + 0x10) + lbl_8060011C * *(f32*)(e1 + 0x10);
    f32 xb = lbl_80600120 * *(f32*)(e3 + 0xC) + xa;
    f32 yb = lbl_80600120 * *(f32*)(e3 + 0x10) + ya;
    f32 xc = lbl_8060011C * *(f32*)(e4 + 0xC) + xb;
    f32 yc = lbl_8060011C * *(f32*)(e4 + 0x10) + yb;
    f32 xd = lbl_80600118 * *(f32*)(e5 + 0xC) + xc;
    f32 yd = lbl_80600118 * *(f32*)(e5 + 0x10) + yc;
    f32 ang2 = fn_8033F1FC(yd, xd);

    f32 alt = lbl_80600114 * (-(lbl_80600110 * ang2));
    f32 f31 = first;
    if ((s8)*(s8*)(e0 + 0x5E) != 2) {
        f31 = alt;
    }
    f31 = f31 - lbl_80600114;
    while (f31 > lbl_80600128) {
        f31 = f31 - lbl_80600124;
    }
    while (f31 < lbl_8060012C) {
        f31 = f31 + lbl_80600124;
    }
    return f31;
}

// 0x802086B8
void fn_802086B8(u8* arg0) {
    if (lbl_805FB380 == 0) {
        return;
    }
    if (((s32 (*)(u8*))(*(void**)(*(u8**)arg0 + 0x5C)))(arg0) == 0) {
        *(s32*)(arg0 + 0xB4) = *(s32*)(arg0 + 0xB4) + 1;
        if (*(u8*)(arg0 + 0xB0) == 0) {
            OSUnlockMutex(lbl_80556AB0);
        } else {
            OSUnlockMutex(lbl_80556AC8);
        }
        OSSleepTicks(0, (OSGetBusClock() >> 2) / 4000);
        fn_8020890C(arg0);
    } else {
        lbl_805FBB6C = 0;
        if (*(u8*)(arg0 + 0xB0) == 0) {
            OSUnlockMutex(lbl_80556AB0);
        } else {
            OSUnlockMutex(lbl_80556AC8);
        }
    }
    if (((s32 (*)(u8*))(*(void**)(*(u8**)arg0 + 0x34)))(arg0) == 0) {
        return;
    }
    if (fn_801F56FC(4) != 0) {
        return;
    }
    if (lbl_805FBB54[0] != 0) {
        return;
    }

    s32 movieOk = 0;
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
        ((s32 (*)(u8*))(*(void**)(*(u8**)lbl_805FB964 + 0x34)))((u8*)lbl_805FB964) != 0) {
        movieOk = 1;
    }
    if (lbl_805FB994 != 0) {
        if (movieOk == 0) {
            lbl_805FB994 = 0;
        }
    }
    if (lbl_805FB994 != 0) {
        s32 movieOk2 = 0;
        if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
            ((s32 (*)(u8*))(*(void**)(*(u8**)lbl_805FB964 + 0x34)))((u8*)lbl_805FB964) != 0) {
            movieOk2 = 1;
        }
        s32 active = 0;
        if (movieOk2 != 0 && lbl_805FB978 != 0) {
            active = 1;
        }
        if (active != 0) {
            if (lbl_805FBB3C > lbl_80600480) {
                f32 t = lbl_805FBB3C - lbl_805FB918;
                lbl_805FBB3C = t;
                if (!(t > lbl_80600480)) {
                    lbl_805FBB3C = lbl_80600480;
                    fn_801F5724(0x84u);
                }
            } else {
                lbl_805FBB3C = lbl_80600484;
            }
        } else {
            lbl_805FBB3C = lbl_80600480;
        }
    } else {
        lbl_805FBB3C = lbl_80600480;
    }
    fn_801F43B0(); /* r3/r4 = 0 are unused by the callee */
}

// 0x8020F6A4
void fn_8020F6A4(void) {
    // (object offset from base, size, limb); handle = object offset - 0xA8.
    static const s32 kChoice[14][3] = {
        {0xA8, 0x68, 2},  {0xB4, 0x68, 3},  {0xC0, 0xDC, 4},  {0xCC, 0xBA, 1},
        {0xD8, 0x136, 0}, {0xE4, 0x68, 3},  {0xF0, 0x136, 0}, {0xFC, 0x68, 2},
        {0x108, 0x68, 3}, {0x114, 0xDC, 4}, {0x120, 0xBA, 1}, {0x12C, 0x136, 0},
        {0x138, 0x68, 3}, {0x144, 0x136, 0},
    };
    u8* base = lbl_8055CA08;
    for (s32 i = 0; i < 14; i++) {
        u8* obj = base + kChoice[i][0];
        fn_8020F384(obj, kChoice[i][1], 0, kChoice[i][2]);
        __register_global_object(obj, (void (*)(void*))ChoiceDestroy, base + (kChoice[i][0] - 0xA8));
    }
}

}  // extern "C"
