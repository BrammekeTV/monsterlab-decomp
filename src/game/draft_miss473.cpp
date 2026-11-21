#include <stdint.h>

#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  BufferFree(void* p);
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);
s32   RaySceneTest(void* ray);
void  WPADControlMotor(s32 chan, u32 command);
void  fn_8006AADC(u8* p, f32 delta);
void  fn_800C60C0(u8* p, f32 dt);                 // 0x800C60C0
s32   fn_800C73B4(u8* p);                         // 0x800C73B4 (matches src/leaf definition; asm uses it as a pointer)
void  fn_800C7FB0(u8* p);                         // 0x800C7FB0
void  fn_800C8900(u8* p, f32 dt);                 // 0x800C8900
void  fn_80172708(void* p);
s32   fn_8019B6CC(u8* p, f32 dt);
void  fn_8019BFF8(void* obj, void* cb, void* arg);
void  fn_8019CE58(void);                          // 0x8019CE58 (alarm/callback target)
void  fn_801B5D94(u8* obj, u8* event);
void  fn_801B7020(u8* p);
void* fn_801B7198(void* anim, s32 a);
u8*   fn_80234848(void);                          // 0x80234848
u64   fn_8032FB38(u32 clock, f32 scale);          // 0x8032FB38
void  OSSetAlarm(void* alarm, u64 tick, void (*handler)(void));
extern const f32 lbl_803460C8[];
extern const f32 lbl_803460D8[];
extern u8  lbl_80347D88[];
extern u8  lbl_80348FF0[];
extern f32 lbl_805FD8C8;
extern f32 lbl_805FD8D0;
extern f32 lbl_805FD8F0;
extern f32 lbl_805FDC28;
extern f32 lbl_805FDC2C;
extern f32 lbl_805FDC44;
extern f32 lbl_805FE178;
extern f32 lbl_805FFAB8;
extern f32 lbl_805FFAFC;
extern f32 lbl_805FFF08;
extern f32 lbl_805FFF0C;
}

extern "C" {

// 0x80085078
// Prop-state update: when the stored value is negative, swap in the new value and mark the prop;
// otherwise push the target into the prop state when it changed.
void fn_80085078(u8* arg0) {
    f32 v = *(f32*)(arg0 + 0x10);
    if (v < lbl_805FD8C8) {
        u8* r4 = *(u8**)(arg0 + 0x4);
        f32 old = *(f32*)(r4 + 0xBC);
        *(f32*)(arg0 + 0x10) = old;
        *(f32*)(r4 + 0xBC) = lbl_805FD8F0;
        return;
    }
    u8* r4b = *(u8**)(arg0 + 0x4);
    u8* prop = *(u8**)(*(u8**)(r4b + 0x1F4) + 0x264);
    if (prop != NULL && v != *(f32*)(r4b + 0xBC)) {
        if (*(s32*)(prop + 0x264) != 1) {
            *(s32*)(prop + 0x264) = 1;
            SetPropState(prop, *(u32*)(lbl_80347D88 + 4), 0);
            *(s32*)(prop + 0x15C) = *(s32*)(prop + 0x15C) | 0x400000;
        }
        *(f32*)(*(u8**)(arg0 + 0x4) + 0xBC) = *(f32*)(arg0 + 0x10);
        *(f32*)(arg0 + 0xC) = lbl_805FD8D0;
    }
}

// 0x800B6598
// Advances the value at +0x2F4 by a ray-table step (scaled by farg0) and clamps +0x2F4 and +0x2F0.
void fn_800B6598(u8* arg0, f32 farg0, f32 farg1) {
    void* r3 = arg0;
    f32 step = farg1;
    if (farg1 == lbl_805FE178) {
        s32 idx = RaySceneTest(r3);
        step = lbl_803460C8[idx];
        r3 = (void*)(uintptr_t)idx;   // the asm passes the index on as r3
    }
    f32 delta = step * farg0;
    *(f32*)(arg0 + 0x2F4) = *(f32*)(arg0 + 0x2F4) + delta;
    *(f32*)(arg0 + 0x2F0) = *(f32*)(arg0 + 0x2F0) - delta;
    s32 idx2 = RaySceneTest(r3);
    if (*(f32*)(arg0 + 0x2F4) > lbl_803460D8[idx2]) {
        s32 idx3 = RaySceneTest((void*)(uintptr_t)idx2);
        *(f32*)(arg0 + 0x2F4) = lbl_803460D8[idx3];
    }
    if (*(f32*)(arg0 + 0x2F0) < lbl_805FE178) {
        *(f32*)(arg0 + 0x2F0) = lbl_805FE178;
    }
}

// 0x800C7EEC
// Walks the 0x14-byte records (count at +0x1784); for records of kind 2, 3 or 5 it runs the
// per-record update, and a record whose +0x268 equals 3 triggers fn_800C8900.
void fn_800C7EEC(u8* arg0, f32 farg0) {
    fn_800C7FB0(arg0);
    u8* rec = arg0;
    u8* slot = arg0 + 0x14;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x1784); i++) {
        s32 kind = *(s32*)(rec + 0x24);
        if (kind == 2 || kind == 3 || kind == 5) {
            u8* ent = (u8*)(uintptr_t)fn_800C73B4(slot);
            fn_800C60C0(ent, farg0);
            if (*(s32*)(ent + 0x268) == 3) {
                fn_800C8900(arg0, farg0);
            }
        }
        rec += 0x14;
        slot += 0x14;
    }
}

// 0x8019CD8C
// Base UI update, then sends the flagged events (bits 0x10 / 0x20 of +0x64) through the
// object's virtual handler at +0x138 and, if any were set, queues them with fn_8019BFF8.
s32 fn_8019CD8C(u8* arg0, f32 farg0) {
    s32 flags = 0;
    s32 ret = fn_8019B6CC(arg0, farg0);
    if (*(s32*)(arg0 + 0x64) & 0x10) {
        flags |= 0x10;
        ((void (*)(u8*, s32))(*(void**)(*(u8**)(arg0 + 0x138) + 0x30)))(arg0, 0x10);
    }
    if (*(s32*)(arg0 + 0x64) & 0x20) {
        flags |= 0x20;
        ((void (*)(u8*, s32))(*(void**)(*(u8**)(arg0 + 0x138) + 0x30)))(arg0, 0x20);
    }
    if (flags != 0) {
        fn_8019BFF8(arg0, (void*)fn_8019CE58, &flags);
    }
    return ret;
}

// 0x801AE544
// Tests the segment between arg0 and arg1 against a plane: returns 0 when the squared
// distance is over (farg0 + farg1)^2, else 1 when the dot with the plane normal arg2 is <= farg0.
s32 fn_801AE544(u8* arg0, u8* arg1, u8* arg2, f32 farg0, f32 farg1) {
    f32 sum = farg0 + farg1;
    f32 dx = *(f32*)(arg0 + 0x0) - *(f32*)(arg1 + 0x0);
    f32 dy = *(f32*)(arg0 + 0x4) - *(f32*)(arg1 + 0x4);
    f32 dz = *(f32*)(arg0 + 0x8) - *(f32*)(arg1 + 0x8);
    f32 distSq = dx * dx + dy * dy + dz * dz - sum * sum;
    if (distSq > lbl_805FFAB8) {
        return 0;
    }
    f32 dot = dx * *(f32*)(arg2 + 0x0) + dy * *(f32*)(arg2 + 0x4) + dz * *(f32*)(arg2 + 0x8);
    return (dot <= farg0) ? 1 : 0;
}

// 0x801DA280
// Releases the child array at +0x58: clears matching back-references, calls fn_80172708 on
// each entry, frees both buffers and zeroes the count at +0x48.
void fn_801DA280(u8* arg0) {
    u8* arr = *(u8**)(arg0 + 0x58);
    if (arr != NULL) {
        u8* key = *(u8**)(*(u8**)arr + 0xB0);
        s32 i = 0;
        s32 off = 0;
        while (i < *(s32*)(arg0 + 0x54)) {
            u8* elem = *(u8**)(*(u8**)(arg0 + 0x58) + off);
            u8* ev = *(u8**)(elem + 0xB0);
            if (i != 0 && ev != NULL && ev == key) {
                *(u8**)(elem + 0xB0) = NULL;
            }
            fn_80172708(elem);
            off += 4;
            i += 1;
        }
        BufferFree(*(void**)(arg0 + 0x58));
        *(u8**)(arg0 + 0x58) = NULL;
        *(s32*)(arg0 + 0x54) = 0;
        BufferFree(*(void**)(arg0 + 0x5C));
        *(u8**)(arg0 + 0x5C) = NULL;
    }
    *(s32*)(arg0 + 0x48) = 0;
}

// 0x80068E28
// When arg1's header type bits (word >> 20, 7 bits) equal 1, looks up the record for its id,
// stores the position from arg1 at +0x1D0 and calls the virtual handler at vtable +0x58.
// Always finishes with fn_801B5D94.
void fn_80068E28(u8* arg0, u8* arg1) {
    if (((*(u32*)arg1 >> 0x14) & 0x7F) == 1) {
        u32 id = *(u32*)arg1;
        u8* rec = (u8*)fn_801B7198(&id, 1);
        *(u8**)(arg0 + 0x24C) = rec;
        if (rec != NULL) {
            u8* sub = *(u8**)(rec + 0x9C);
            if (sub != NULL) {
                *(s32*)(arg0 + 0x14C) = *(s32*)(sub + 0x60);
            }
        }
        f32 fx = *(f32*)(arg1 + 0x4);
        f32 fy = *(f32*)(arg1 + 0x8);
        f32 fz = *(f32*)(arg1 + 0xC);
        *(f32*)(arg0 + 0x1D0) = fx;
        *(f32*)(arg0 + 0x1D4) = fy;
        *(f32*)(arg0 + 0x1D8) = fz;
        // Virtual call at vtable +0x58 with f1 = y, f2 = x (z is spilled to the stack only).
        ((void (*)(u8*, f32, f32))(*(void**)(*(u8**)arg0 + 0x58)))(arg0, fy, fx);
    }
    fn_801B5D94(arg0, arg1);
}

// 0x800998E4
// Builds a quaternion about a fixed axis (constants lbl_805FDC28/2C) with angle farg0 * lbl_805FDC44,
// copies it into the sub-record at +0x264 -> +0x4 and sets the flag bits 0x8000/0x10000000/0x4000.
void fn_800998E4(u8* arg0, f32 farg0) {
    Quat q;
    Vec3 axis;
    axis.x = lbl_805FDC28;
    axis.y = lbl_805FDC2C;
    axis.z = lbl_805FDC28;
    QuatFromAxisAngle(&q, &axis, lbl_805FDC44 * farg0);
    u8* rec = *(u8**)(arg0 + 0x264);
    if (rec != NULL) {
        u8* sub = *(u8**)(rec + 0x4);
        *(f32*)(sub + 0xC) = q.x;
        *(f32*)(sub + 0x10) = q.y;
        *(f32*)(sub + 0x14) = q.z;
        *(f32*)(sub + 0x18) = q.w;
        *(s32*)(sub + 0x20) = *(s32*)(sub + 0x20) | 0x8000;
        *(s32*)(sub + 0x20) = *(s32*)(sub + 0x20) | 0x10000000;
        *(s32*)(sub + 0x20) = *(s32*)(sub + 0x20) | 0x4000;
        *(s32*)(sub + 0x20) = *(s32*)(sub + 0x20) | 0x10000000;
    }
}

// 0x800B1F94
// Sets flag bit 0 on every entry of the list at +0xB0 (entry+4 -> +0x20), runs fn_8006AADC and
// the virtual update at vtable +0x74, clears the bit again and then calls fn_801B7020.
void fn_800B1F94(u8* arg0, f32 farg0) {
    u8* node = *(u8**)(arg0 + 0xB0);
    while (node != NULL) {
        u8* sub = *(u8**)(node + 0x4);
        if (sub != NULL) {
            u8* inner = *(u8**)(sub + 0x4);
            *(s32*)(inner + 0x20) = *(s32*)(inner + 0x20) | 1;
        }
        node = *(u8**)(node + 0x8);
    }
    fn_8006AADC(arg0, farg0);
    ((void (*)(u8*, f32))(*(void**)(*(u8**)arg0 + 0x74)))(arg0, farg0);
    node = *(u8**)(arg0 + 0xB0);
    while (node != NULL) {
        u8* sub = *(u8**)(node + 0x4);
        if (sub != NULL) {
            u8* inner = *(u8**)(sub + 0x4);
            *(s32*)(inner + 0x20) = *(s32*)(inner + 0x20) & ~1;
        }
        node = *(u8**)(node + 0x8);
    }
    fn_801B7020(arg0);
}

// 0x8011A5FC
// Switches the active child: if the virtual test at vtable +0x24 returns 0, releases the
// current child (vtable +0x10), picks a new index from vtable +0x1C and enters that child (+0xC).
void fn_8011A5FC(u8* arg0, s32 arg1) {
    if (((s32 (*)(u8*))(*(void**)(*(u8**)arg0 + 0x24)))(arg0) == 0) {
        s32 cur = *(s32*)(arg0 + 0x18);
        *(u8**)(arg0 + 0x4) = NULL;
        if (cur != -1) {
            u8* old = *(u8**)(arg0 + 0x8 + cur * 4);
            ((void (*)(u8*))(*(void**)(*(u8**)old + 0x10)))(old);
        }
        s32 nxt = ((s32 (*)(u8*, s32))(*(void**)(*(u8**)arg0 + 0x1C)))(arg0, arg1);
        *(s32*)(arg0 + 0x18) = nxt;
        if (nxt != -1) {
            u8* ent = *(u8**)(arg0 + 0x8 + nxt * 4);
            *(u8**)(arg0 + 0x4) = ent;
            ((void (*)(u8*))(*(void**)(*(u8**)ent + 0xC)))(ent);
        }
    }
}

// 0x801B2E10
// Controller rumble alarm: toggles the motor state byte at +0x48, drives WPADControlMotor and
// re-arms its alarm (at +0x18) with a period from the bus clock.
void fn_801B2E10(void) {
    u8* p = fn_80234848();
    f32 scale;
    if (*(u8*)(p + 0x48) != 0) {
        WPADControlMotor(*(s32*)(p + 0x4), 0);
        *(u8*)(p + 0x48) = 0;
        scale = lbl_805FFAFC * *(f32*)(p + 0xC);
    } else {
        *(u8*)(p + 0x48) = 1;
        WPADControlMotor(*(s32*)(p + 0x4), 1);
        scale = lbl_805FFAFC * *(f32*)(p + 0xC);
    }
    u32 clock = *(u32*)0x800000F8;
    u64 tick = fn_8032FB38(clock, scale * (f32)(clock >> 2));
    OSSetAlarm(p + 0x18, tick, fn_801B2E10);   // r4 is only EABI padding for the 64-bit tick
}

// 0x801D90A0
// Resets the transform record: type 2 at +0x8, unit scale at +0x38..+0x44, the constant
// quad from lbl_80348FF0 at +0xC..+0x18 and +0x1C..+0x28, and zeroes +0x2C/+0x34.
void fn_801D90A0(u8* arg0) {
    *(s32*)(arg0 + 0x34) = 0;
    *(s32*)(arg0 + 0x8) = 2;
    *(f32*)(arg0 + 0x38) = lbl_805FFF08;
    *(f32*)(arg0 + 0x3C) = lbl_805FFF08;
    *(f32*)(arg0 + 0x40) = lbl_805FFF08;
    *(f32*)(arg0 + 0x44) = lbl_805FFF08;
    f32 c0 = *(f32*)(lbl_80348FF0 + 0x0);
    f32 c1 = *(f32*)(lbl_80348FF0 + 0x4);
    f32 c2 = *(f32*)(lbl_80348FF0 + 0x8);
    f32 c3 = *(f32*)(lbl_80348FF0 + 0xC);
    *(f32*)(arg0 + 0x1C) = c0;
    *(f32*)(arg0 + 0x20) = c1;
    *(f32*)(arg0 + 0x24) = c2;
    *(f32*)(arg0 + 0x28) = c3;
    *(f32*)(arg0 + 0xC) = c0;
    *(f32*)(arg0 + 0x10) = c1;
    *(f32*)(arg0 + 0x14) = c2;
    *(f32*)(arg0 + 0x18) = c3;
    *(s32*)(arg0 + 0x2C) = 0;
    *(f32*)(arg0 + 0x30) = lbl_805FFF0C;
}

}  // extern "C"
