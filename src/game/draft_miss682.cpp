// Small functions batch (miss682): verified against asm.
#include "types.h"
#include <math.h>

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
extern "C++" void* gEventMgr;              // 0x805FB6C8
u32  RandomU32(void);                      // 0x80143460
s32  RaySceneTest(void* ray);              // 0x80070458
void fn_800704AC(u8* p, f32 dt);           // 0x800704AC
void fn_80077218(u8* arg0);                // 0x80077218
u8*  fn_800ABD80(u8* arg0);                // 0x800ABD80
s32  fn_800AAAF8(u8* p, u8* q, f32* out);  // 0x800AAAF8
s32  fn_801C3624(u8* arg0, u8* arg1, u8* arg2, f32* arg3, u8* arg4);  // 0x801C3624
void* fn_801ACCA0(u8* arg0, s32 arg1);     // 0x801ACCA0
s32  fn_8016195C(void* p, s32 a, s32 b, void* c);  // 0x8016195C
void fn_800BB658(u8* p, f32 dt);           // 0x800BB658
s32  fn_800BC050(u8* p);                   // 0x800BC050
s32  fn_800C2628(s32 a, s32 b);            // 0x800C2628 (a = word at obj+0x60, b = node value)
s32  fn_800AEEA8(u8* a);            // 0x800AEEA8
s32  fn_800B2054(u8* p, u8* a);            // 0x800B2054
s32  fn_800B1A20(u8* p);            // 0x800B1A20
s32  fn_800AD340(u8* p, u8* a);            // 0x800AD340
s32  fn_800AC850(u8* p, u8* a);            // 0x800AC850
void fn_800AC390(u8* p, f32* a, f32 dt);  // 0x800AC390

extern f32 lbl_805FD654;
extern f32 lbl_805FD658;
extern f32 lbl_805FD65C;
extern f32 lbl_805FDD18;
extern f32 lbl_805FDD1C;
extern f32 lbl_805FDD20;
extern f32 lbl_805FDD54;
extern f32 lbl_805FDD60;
extern f32 lbl_805FDD64;
extern f32 lbl_805FDD68;
extern f32 lbl_805FDD6C;
extern f32 lbl_805FDD70;
extern f32 lbl_805FDF18;
extern f32 lbl_805FDF1C;
extern f32 lbl_805FDF28;
extern f32 lbl_805FE248;
extern f32 lbl_805FE390;
extern f32 lbl_805FE3B0;
extern f32 lbl_80346318[];
extern u8 lbl_80383A88[];
extern u8 lbl_8040E5F0[];
}

extern "C" {

// 0x80076050
// Resets the object's tuning fields and fires the start event to the object's 7 sub-targets.
void fn_80076050(u8* arg0) {
    s32 i;

    fn_80077218(*(u8**)(arg0 + 0x6CC));
    *(f32*)(arg0 + 0x138) = lbl_805FD658;
    *(f32*)(arg0 + 0x140) = lbl_805FD65C;
    *(s32*)(arg0 + 0x13C) = 3;
    *(s32*)(arg0 + 0x144) = 0x28;
    *(f32*)(arg0 + 0x1F8) = lbl_805FD658;
    *(f32*)(arg0 + 0x200) = lbl_805FD65C;
    *(s32*)(arg0 + 0x1FC) = 3;
    *(s32*)(arg0 + 0x204) = 0x28;
    *(f32*)(arg0 + 0x6BC) = lbl_805FD654;
    *(s32*)(arg0 + 0x6C0) = 0;
    *(s32*)(arg0 + 0x6C4) = 0;
    EventSend(gEventMgr, *(void**)(arg0 + 0x5D4), NULL, 0x6CCBDF09U, 0, 0, 1, 0);
    for (i = 0; i < 4; i++) {
        u8* q = arg0 + i * 4;
        EventSend(gEventMgr, *(void**)(q + 0x5A4), NULL, 0x6CCBDF09U, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(q + 0x5B4), NULL, 0x6CCBDF09U, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(q + 0x5C4), NULL, 0x6CCBDF09U, 0, 0, 1, 0);
    }
}

// 0x8009D710
// Sets the flag at 0x3F0 and builds two effect parameter blocks from a ray-scene lookup.
void fn_8009D710(u8* arg0) {
    f32 vecA[7];  // stack 0x30..0x48
    f32 vecB[6];  // stack 0x18..0x2C (last slot is an integer 1)
    f32 t;

    *(u8*)(arg0 + 0x3F0) = 1;
    vecA[0] = lbl_805FDD60;
    vecA[1] = lbl_805FDD18;
    vecA[2] = lbl_805FDD18;
    vecA[3] = lbl_805FDD1C;
    vecA[4] = lbl_805FDD1C;
    t = lbl_80346318[RaySceneTest(arg0)];
    vecA[5] = lbl_805FDD20 * t;
    vecA[6] = lbl_805FDD64 * t;
    *(s32*)(arg0 + 0x3F8) = fn_8016195C(lbl_8040E5F0, 4, 0, vecA);

    vecB[0] = lbl_805FDD1C;
    vecB[1] = lbl_805FDD54;
    vecB[2] = lbl_805FDD68;
    vecB[3] = lbl_805FDD6C;
    vecB[4] = lbl_805FDD70;
    *(s32*)&vecB[5] = 1;
    *(s32*)(arg0 + 0x3FC) = fn_8016195C(lbl_8040E5F0, 0xD, 1, vecB);
    *(f32*)(arg0 + 0x3F4) = lbl_805FDD1C;
}

// 0x800AA68C
// Writes a 3-float output vector: either the interpolated point from fn_801C3624 or a stored default.
void fn_800AA68C(u8* arg0, u8* arg1, u8* arg2) {
    u8 blk[0x40];  // stack frame at sp+0x18: name pointer, count, floats, scratch at +0x3C
    f32 sp8;
    f32 spC;
    f32 sp10;
    f32 sp14;

    *(f32*)(arg0 + 0x8) = lbl_805FDF18;
    *(f32*)(arg0 + 0x4) = lbl_805FDF18;
    *(f32*)(arg0 + 0x0) = lbl_805FDF18;
    *(void**)(blk + 0x0) = lbl_80383A88;
    *(s32*)(blk + 0x4) = 0;
    *(f32*)(blk + 0x3C) = *(f32*)(fn_800ABD80(*(u8**)(arg1 + 0xB8)) + 0x3C);
    *(f32*)(blk + 0x14) = *(f32*)(arg1 + 0x6C4);
    *(f32*)(blk + 0x18) = *(f32*)(arg1 + 0x6C8);
    *(f32*)(blk + 0x1C) = *(f32*)(arg1 + 0x6CC);
    *(f32*)(blk + 0x20) = *(f32*)(arg1 + 0x6B8);
    *(f32*)(blk + 0x24) = *(f32*)(arg1 + 0x6BC);
    *(f32*)(blk + 0x28) = *(f32*)(arg1 + 0x6C0);

    if (fn_800AAAF8(arg1, blk, (f32*)arg0) != 0) {
        sp8 = lbl_805FDF18;
        spC = lbl_805FDF18;
        sp10 = lbl_805FDF18;
        sp14 = lbl_805FDF18;
        fn_801C3624(arg0, arg1 + 0x6B8, arg1 + 0x6C4, &sp8, (u8*)&spC);
        *(f32*)(arg0 + 0x0) = spC;
        *(f32*)(arg0 + 0x4) = sp10;
        *(f32*)(arg0 + 0x8) = sp14;
    } else {
        *(f32*)(arg0 + 0x0) = *(f32*)(arg1 + 0x6AC);
        *(f32*)(arg0 + 0x4) = *(f32*)(arg1 + 0x6B0);
        *(f32*)(arg0 + 0x8) = *(f32*)(arg1 + 0x6B4);
    }
    fn_801ACCA0(blk, 0);
}

// 0x800AA7A8
// Picks one of the 7 entries whose two linked sub-objects are both inactive (random choice) and
// blends its position with a fraction of the 0x6A0 timer, then calls fn_800AC390 on it.
void fn_800AA7A8(u8* arg0) {
    s32 count = 0;
    s32 i;
    u32 target;

    for (i = 0; i < 7; i++) {
        u8* e = arg0 + i * 4;
        if (*(s32*)(*(u8**)(e + 0x118) + 0x264) == 0 && *(s32*)(*(u8**)(e + 0xE0) + 0x264) == 0) {
            count += 1;
        }
    }
    if (count > 0) {
        target = RandomU32() % (u32)count;
        for (i = 0; i < 7; i++) {
            u8* e = arg0 + i * 4;
            if (*(s32*)(*(u8**)(e + 0x118) + 0x264) == 0 && *(s32*)(*(u8**)(e + 0xE0) + 0x264) == 0) {
                if (target == 0) {
                    f32 frac = *(f32*)(arg0 + 0x6A0) / lbl_805FDF28;
                    u8* src = *(u8**)(*(u8**)(e + 0x118) + 0x4);
                    f32 pos[3];
                    pos[0] = *(f32*)(src + 0x0);
                    pos[1] = *(f32*)(src + 0x4);
                    pos[2] = *(f32*)(src + 0x8);
                    // r6 is not set before the call: passed as 0.
                    fn_800AC390(*(u8**)(e + 0xE0), pos, lbl_805FDF1C - frac); /* asm also sets r5/r6/f2: unused by the callee */
                    return;
                }
                target -= 1;
            }
        }
    }
}

// 0x800AA9D4
// Tries each child-list in turn; returns the first result that is not -1 (or -1 if none).
u32 fn_800AA9D4(u8* arg0, u8* arg1) {
    s32 i;
    s32 ret;

    for (i = 0; i < 7; i++) {
        ret = fn_800AEEA8(*(u8**)(arg0 + i * 4 + 0x118)); /* r4 (arg1) is unused by the callee */
        if (ret != -1) {
            return (u32)ret;
        }
    }
    ret = fn_800B2054(*(u8**)(arg0 + 0x90), arg1);
    if (ret != -1) {
        return (u32)ret;
    }
    ret = fn_800B1A20(*(u8**)(arg0 + 0x94)); /* r4 (arg1) is unused by the callee */
    if (ret != -1) {
        return (u32)ret;
    }
    for (i = 0; i < 6; i++) {
        ret = fn_800AD340(*(u8**)(arg0 + i * 4 + 0x98), arg1);
        if (ret != -1) {
            return (u32)ret;
        }
    }
    for (i = 0; i < 14; i++) {
        u8* o = *(u8**)(arg0 + i * 4 + 0xE0);
        if (*(s32*)(o + 0x264) == 1) {
            ret = fn_800AC850(o, arg1);
            if (ret != -1) {
                return (u32)ret;
            }
        }
    }
    return 0xFFFFFFFFu;  // r3 still holds the last (-1) call result
}

// 0x800BA864
// Advances the 0x124 step index when the position crosses its target this frame, then
// advances the 0x128 counter up to 0x124 by calling fn_800BC050 once per missing step.
void fn_800BA864(u8* arg0, f32 farg0) {
    s32 cur = *(s32*)(arg0 + 0x124);
    s32 lim = *(s32*)(arg0 + 0x12C);
    f32 pos = *(f32*)(arg0 + 0x6C);
    f32 posEnd = pos + farg0;
    f32 target = (lbl_805FE248 * (f32)(cur + 1)) / (f32)(lim + 1);
    s32 n;
    s32 mn;
    s32 count;
    s32 k;

    if (pos <= target && posEnd >= target && cur < lim) {
        *(s32*)(arg0 + 0x124) = cur + 1;
    }
    fn_800704AC(arg0, farg0);
    n = *(s32*)(arg0 + 0x124);
    mn = *(s32*)(arg0 + 0x128);
    if (mn < n) {
        count = n - mn;
        for (k = 0; k < count; k++) {
            if (fn_800BC050(arg0) != 0) {
                *(s32*)(arg0 + 0x128) += 1;
            }
        }
    }
    fn_800BB658(arg0, farg0);
}

// 0x800C2EE4
// Looks up the element at index 0x6C in the 0x78 array (range 0x70..0x70+0x74) and, if valid,
// passes its value through fn_800C2628; returns the byte at obj+0x26D after a virtual call.
u8 fn_800C2EE4(u8* arg0, u8* arg1) {
    typedef void (*FnF32)(u8*, f32);
    s32 v = 0;
    s32 idx = *(s32*)(arg0 + 0x6C);

    if (idx >= 0 && *(s32*)(arg0 + 0x70) >= 0) {
        s32 size = *(s32*)(arg0 + 0x74);
        s32 start = *(s32*)(arg0 + 0x70);
        s32 inRange = 0;
        u8* node = NULL;

        if (idx < size) {
            if (start < idx) {
                start += size;
            }
            inRange = (idx <= start);
        }
        if (inRange) {
            node = *(u8**)(arg0 + 0x78) + idx * 4;
        }
        *(u8**)(arg0 + 0x7C) = node;
        if (node != NULL) {
            v = *(s32*)node;
        }
    }
    if (fn_800C2628(*(s32*)(arg0 + 0x60), v) != 0) {
        return 0;
    }
    (*(FnF32*)(*(u8**)arg1 + 0xC4))(arg1, lbl_805FE390);
    return *(u8*)(arg1 + 0x26D);
}

// 0x800C4B70
// Returns the length of the segment from the object's 0x268 point to the 0x264 point (or to
// the origin when 0x264 is negative), scaled by 0x2B0 when the 0x2AC value is non-negative.
f32 fn_800C4B70(u8* arg0) {
    f32 pos = *(f32*)(arg0 + 0x2AC);

    if (pos < lbl_805FE3B0) {
        f32 base[3];
        s32 k = *(s32*)(arg0 + 0x264);
        u8* t;
        f32 dx;
        f32 dy;
        f32 dz;
        f32 total;

        base[0] = lbl_805FE3B0;
        base[1] = lbl_805FE3B0;
        base[2] = lbl_805FE3B0;
        if (k >= 0) {
            u8* e = *(u8**)(arg0 + 0x98) + k * 0xC8;
            base[0] = *(f32*)(e + 0x2C);
            base[1] = *(f32*)(e + 0x30);
            base[2] = *(f32*)(e + 0x34);
        }
        t = *(u8**)(arg0 + 0x98) + *(s32*)(arg0 + 0x268) * 0xC8;
        dx = *(f32*)(t + 0x2C) - base[0];
        dy = *(f32*)(t + 0x30) - base[1];
        dz = *(f32*)(t + 0x34) - base[2];
        total = (dz * dz) + ((dx * dx) + (dy * dy));
        if (!(total <= lbl_805FE3B0)) {
            return sqrtf(total);
        }
        return lbl_805FE3B0;
    }
    return pos * *(f32*)(arg0 + 0x2B0);
}

// 0x800CBFC8
// Maps a logical index (0..count-1 over the 0x8..0x10 ring) to a pointer into the 0x14 array.
u8* fn_800CBFC8(u8* arg0, s32 arg1) {
    s32 start = *(s32*)(arg0 + 0x8);
    s32 end = *(s32*)(arg0 + 0xC);
    s32 size = *(s32*)(arg0 + 0x10);
    s32 count;
    s32 mapped;
    s32 flag;

    if (start < 0 || end < 0) {
        return NULL;
    }
    if (arg1 < 0) {
        return NULL;
    }
    {
        s32 e = end;
        if (e < start) {
            e += size;
        }
        count = (e - start) + 1;
    }
    if (arg1 >= count) {
        return NULL;
    }

    flag = *(s32*)(arg0 + 0x0);
    if (flag == 1) {
        if (*(s32*)(arg0 + 0x4) == 0) {
            mapped = arg1 + start;
            if (mapped >= size) {
                mapped -= size;
            }
        } else {
            mapped = end - arg1;
            if (mapped < 0) {
                mapped += size;
            }
        }
    } else {
        if (*(s32*)(arg0 + 0x4) == 0) {
            mapped = end - arg1;
            if (mapped < 0) {
                mapped += size;
            }
        } else {
            mapped = arg1 + start;
            if (mapped >= size) {
                mapped -= size;
            }
        }
    }
    return *(u8**)(arg0 + 0x14) + mapped * 4;
}

}  // extern "C"
