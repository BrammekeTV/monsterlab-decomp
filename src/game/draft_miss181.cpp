// Small functions batch miss181: init/ctor helpers, vector math, object size, camera and list helpers.
#include <math.h>
#include <string.h>
#include "types.h"

extern "C" {
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);   // 0x800690D4 (base ctor)
void  fn_8020EA84(u8* a3);                              // 0x8020EA84
u8*   fn_8006F61C(u8* p, s32 type, u8* rec, s32 reset);                            // 0x8006F61C (base ctor)
u32   fn_8016A768(u8* a3);                              // 0x8016A768
s32   fn_8019555C(u8* a3);                              // 0x8019555C
s32   fn_801AAEE4(u8* a3, u8* a4);                      // 0x801AAEE4
s32 fn_801AA79C(u8* dst, u8* src);                    // 0x801AA79C
void  fn_801C7264(u8* p, u8* a, s32 b);                 // 0x801C7264
void  fn_801C849C(u8* mgr, u8* item);                   // 0x801C849C
void  fn_801C3E7C(u8* tbl, u8* obj);                    // 0x801C3E7C
void  fn_801EBDF0(u8* a, u8* b);                        // 0x801EBDF0
BOOL  ZoneIsLoaded(void* world, s32 zone);              // 0x8012F408
f32   fn_801D8AD4(u8* p);                               // 0x801D8AD4
void  fn_801D89C8(f32 fov);                             // 0x801D89C8
u8*   fn_801D6808(u8* cam);                             // 0x801D6808
void  fn_801D6E4C(u8* cam, f32* m, u8* extra);          // 0x801D6E4C
}

extern "C++" s32 gZoneResourceSize;
extern u8 gWorldState[];
extern u8 lbl_80348F38[];
extern u8 lbl_80359180[];
extern u8 lbl_8035A3C8[];
extern u8 lbl_8035A588[];
extern f32 lbl_803C6990[];
extern f32 lbl_805FD770;
extern f32 lbl_805FD808;
extern u8 lbl_8047C640[];
extern u8 lbl_8047C7AC[];
extern u8* lbl_805FB75C;

extern "C" {

// 0x8004E684
// Init for the object: runs the base init, resets the vtable pair and the float/flag fields,
// and if the child at +4 exists, sets bit 0x1000 on it and its state byte to 0xC.
u8* fn_8004E684(u8* p, s32 a, u8* b, s32 c) {
    u8* child;

    fn_800690D4(p, a, b, c);
    fn_8020EA84(p + 0x264);
    child = *(u8**)(p + 0x4);
    *(void**)(p + 0x0) = lbl_80359180;
    *(void**)(p + 0x264) = lbl_80359180 + 0xBC;
    *(s32*)(p + 0x268) = 0;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 0.0f;
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 1.0f;
    *(f32*)(p + 0x288) = 1.0f;
    *(u8*)(p + 0x28C) = 0;
    *(u8*)(p + 0x38E) = 0;
    *(u8*)(p + 0x38F) = 1;
    *(f32*)(p + 0x390) = 0.0f;
    *(u8*)(p + 0x394) = 0;
    *(f32*)(p + 0x398) = 0.0f;
    *(f32*)(p + 0x39C) = 0.0f;
    *(f32*)(p + 0x3A0) = 0.0f;
    *(f32*)(p + 0x3A4) = 0.0f;
    *(f32*)(p + 0x3A8) = 0.0f;
    *(f32*)(p + 0x3AC) = 0.0f;
    if (child != NULL) {
        *(s32*)(child + 0x1C) |= 0x1000;
        *(u8*)(*(u8**)(p + 0x4) + 0x34) = 0xC;
    }
    return p;
}

// 0x80054530
// Quaternion product: out = a * b (components 0..2 are xyz, 3 is w).
void fn_80054530(f32* out, f32* a, f32* b) {
    f32 ax = a[0], ay = a[1], az = a[2], aw = a[3];
    f32 bx = b[0], by = b[1], bz = b[2], bw = b[3];
    out[3] = aw * bw - ax * bx - ay * by - az * bz;
    out[0] = ay * bz + (aw * bx + ax * bw) - az * by;
    out[1] = az * bx + (ay * bw + (aw * by - ax * bz));
    out[2] = az * bw + (aw * bz + ax * by) - ay * bx;
}

// 0x8005484C
// Normalizes a 4-component vector in place.
void fn_8005484C(f32* q) {
    f32 len2 = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    f32 inv = 1.0f / sqrtf(len2);
    q[0] = q[0] * inv;
    q[1] = q[1] * inv;
    q[2] = q[2] * inv;
    q[3] = q[3] * inv;
}

// 0x8006D2D0
// Init: runs the base init fn_8006F61C, sets the vtable, clears the state bytes and,
// if the child at +4 exists, sets bit 0x100 on it and its state byte to 8.
u8* fn_8006D2D0(u8* p, s32 type, u8* rec, s32 reset) {
    fn_8006F61C(p, type, rec, 0);
    *(void**)(p + 0x0) = lbl_8035A3C8;
    *(u16*)(p + 0x1FE) = 0xFFFF;
    *(u8*)(p + 0x1FD) = 0;
    *(u8*)(p + 0x1FC) = 0;
    if (*(u8**)(p + 0x4) != NULL) {
        *(s32*)(*(u8**)(p + 0x4) + 0x1C) |= 0x100;
        *(u8*)(*(u8**)(p + 0x4) + 0x34) = 8;
    }
    return p;
}

// 0x8006F6BC
// Init: runs fn_8006D2D0, sets the vtable, and if the child at +4 exists, sets bit 0x200 on it
// and its state byte to 9.
u8* fn_8006F6BC(u8* p, s32 type, u8* rec, s32 reset) {
    fn_8006D2D0(p, type, rec, reset);
    *(void**)(p + 0x0) = lbl_8035A588;
    if (*(u8**)(p + 0x4) != NULL) {
        *(s32*)(*(u8**)(p + 0x4) + 0x1C) |= 0x200;
        *(u8*)(*(u8**)(p + 0x4) + 0x34) = 9;
    }
    return p;
}

// 0x8007DD7C
// Length of a 3-component vector; returns the constant lbl_805FD770 for non-positive squared length.
f32 fn_8007DD7C(f32* v) {
    f32 sum = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (sum <= lbl_805FD770) return lbl_805FD770;
    return sqrtf(sum);
}

// 0x80082954
// Evaluates a cubic-style point at parameter t from the object's coefficient block (+0x280..+0x2B0)
// and the three-float table lbl_803C6990, writing the xyz result to out.
void fn_80082954(f32* out, u8* obj, f32 t) {
    f32 s = lbl_805FD808 * *(f32*)(obj + 0x2B0) * t * t;
    out[0] = lbl_803C6990[0] * s + *(f32*)(obj + 0x298) * t + *(f32*)(obj + 0x280);
    out[1] = lbl_803C6990[1] * s + *(f32*)(obj + 0x29C) * t + *(f32*)(obj + 0x284);
    out[2] = lbl_803C6990[2] * s + *(f32*)(obj + 0x2A0) * t + *(f32*)(obj + 0x288);
}

// 0x8016A324
// Byte size of a record: string at +0x34 plus a kind-dependent header, rounded to 16 bytes, plus
// count * 8 entries, and the product of the fields at +0x28 and +0x1C.
s32 fn_8016A324(u8* p) {
    s32 kind = *(s32*)(p + 0x14);
    s32 len = (s32)strlen((const char*)(p + 0x34));
    s32 a = (len + 0x38) & ~3;
    s32 k1 = (kind == 1) ? 4 : ((kind == 0) ? 0x18 : 0);
    s32 b = (a + k1 + 3) & ~3;
    s32 m = (kind == 1) ? 8 : ((kind == 0) ? 4 : 0);
    s32 count = *(s32*)(p + 0x18);
    s32 c = (b + count * m + 3) & ~3;
    s32 d = c + count * 8;
    s32 f = (d + 0xF) & ~0xF;
    return f + *(s32*)(p + 0x28) * *(s32*)(p + 0x1C);
}

// 0x80175088
// Total size: name at +0x80, eight optional slots at +0x3C/+0x5C, the slot at +0x7C, plus a
// dispatch through the overlay table lbl_80348F38 indexed by the byte at +0xC.
s32 fn_80175088(u8* p) {
    typedef s32 (*SizeFn)(u8*);
    s32 acc = (s32)strlen((const char*)(p + 0x80)) + 0x84;
    s32 i;
    for (i = 0; i < 8; i++) {
        u8* sub;
        if (*(s32*)(p + 0x3C + i * 4) != 0) acc += 0x30;
        sub = *(u8**)(p + 0x5C + i * 4);
        if (sub != NULL) acc += (s32)fn_8016A768(sub);
    }
    if (*(u8**)(p + 0x7C) != NULL) acc += (s32)fn_8016A768(*(u8**)(p + 0x7C));
    {
        u8 idx = *(u8*)(p + 0xC);
        SizeFn fn = (SizeFn)*(void**)(lbl_80348F38 + (idx << 2) + 0x6C);
        return acc + fn(*(u8**)(p + 0x8));
    }
}

// 0x801AB4C4
// Copies the header fields of src into dst, then re-links the five child slots and the tail slot.
s32 fn_801AB4C4(u8* dst, u8* src) {
    s32 i;
    *(s32*)(dst + 0x1C) = *(s32*)(src + 0x1C);
    *(f32*)(dst + 0x24) = *(f32*)(src + 0x24);
    *(f32*)(dst + 0x20) = *(f32*)(src + 0x20);
    *(f32*)(dst + 0x28) = *(f32*)(src + 0x28);
    *(s32*)(dst + 0x2C) = *(s32*)(src + 0x2C);
    for (i = 0; i < 5; i++) {
        u8* s = *(u8**)(src + 0x4 + i * 4);
        if (s != NULL) fn_801AA79C(*(u8**)(dst + 0x4 + i * 4), s);
    }
    if (*(u8**)(src + 0x18) != NULL) fn_801AAEE4(*(u8**)(dst + 0x18), *(u8**)(src + 0x18));
    return 0;
}

// 0x801BC4C4
// Membership test on the id list at +0x1C (count at +0x20; bit 0 of +0x18 selects the sense):
// returns 1 if id is absent from a reject-list, or present in an accept-list.
s32 fn_801BC4C4(u8* p, s32 id) {
    s32 i;
    s32 n;
    u8* arr;

    if (*(s32*)(p + 0x10) == 0) {
        u8* r5 = *(u8**)(p + 0x4);
        if (r5 != NULL && *(s32*)(r5 + 0x4) == id) return 1;
    }
    n = *(s32*)(p + 0x20);
    arr = *(u8**)(p + 0x1C);
    if (*(s32*)(p + 0x18) & 1) {
        for (i = 0; i < n; i++) {
            if (id == *(s32*)(arr + i * 4)) return 0;
        }
        return 1;
    }
    for (i = 0; i < n; i++) {
        if (id == *(s32*)(arr + i * 4)) return 1;
    }
    return 0;
}

// 0x801C7570
// For each loaded zone up to the zone count, calls fn_801C7264 on that zone's record.
void fn_801C7570(u8* arg0, s32 arg1) {
    s32 zone;
    if (arg1 != 0) {
        for (zone = 0; zone <= *(s32*)(*(u8**)gWorldState + 0x4); zone++) {
            if (ZoneIsLoaded(gWorldState, zone) != 0) {
                fn_801C7264(*(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize + 0xDC, arg0, arg1);
            }
        }
    }
}

// 0x801C79A4
// Records how many of the two slots at +0x28/+0x2C are in use (0..2), flags that slot, and stores the count at +0x24.
s32 fn_801C79A4(u8* p) {
    s32 n = 0;
    if (*(s32*)(p + 0x28) != -1) {
        n = 1;
        if (*(s32*)(p + 0x2C) != -1) n = 2;
    }
    *(s32*)(p + n * 4 + 0x28) = 1;
    *(s32*)(p + 0x24) = n;
    return n;
}

// 0x801D0B48
// Clears the back-link of each entry in the list at +0x90, releases the list and the handle at +0x94,
// then frees the item at +0x60 and decrements the counter at +0x64.
void fn_801D0B48(u8* arg0, u8* arg1, u8* arg2) {
    u8* list = *(u8**)(arg1 + 0x90);
    s32 i;

    if (list != NULL) {
        u8* cur = list;
        for (i = 0; i < *(s32*)(list + 0xC); i++) {
            u8* item = *(u8**)(cur + 0x14);
            if (item != NULL) *(s32*)(item + 0x74) = 0;
            cur += 4;
        }
    }
    if (list != NULL) fn_801C849C(arg2, list);
    if (*(u8**)(arg1 + 0x94) != NULL) fn_801C3E7C(lbl_8047C7AC, *(u8**)(arg1 + 0x94));
    fn_801EBDF0(*(u8**)(arg0 + 0x60), arg1);
    *(s32*)(arg0 + 0x64) = *(s32*)(arg0 + 0x64) - 1;
}

// 0x801D1AFC
// Refreshes the camera block and pushes its 12-float matrix to fn_801D6E4C.
void fn_801D1AFC(void) {
    f32 m[12];
    u8* src;
    s32 i;

    f32 fov = fn_801D8AD4(lbl_8047C640);        // stays in f1 for the next call
    fn_801D89C8(fov);
    src = fn_801D6808(lbl_8047C640);
    for (i = 0; i < 12; i++) {
        m[i] = *(f32*)(src + i * 4);
    }
    fn_801D6E4C(lbl_8047C640, m, lbl_805FB75C);
}

}  // extern "C"
