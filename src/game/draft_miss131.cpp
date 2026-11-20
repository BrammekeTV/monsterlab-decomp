// Small UI, fader and vector/quaternion helpers: fader done query, quat-to-matrix,
// matrix-times-vector, cross/normalize, scale, UI particle effect alloc and scan, UI pool init/free.
#include <math.h>

#include "types.h"
#include "engine/math.h"

typedef void (*VCallFn)(void*, s32);

extern "C" {
void BufferFree(void* p);
void* MemAllocDefault(u32 size);
void* ScreenSetsCurrent(void* sets);
void fn_8018D370(const char* fmt, ...);
void* fn_8018D428(s32 size, const char* file, void* tag, s32 line);
void fn_8019B2F4(void* obj);
u8* fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);
void fn_801EBA20(void* list, void* item);
s32 fn_801AB7E8(u8* a3);
s32 fn_801AB820(u8* arg0, u8* arg1);
void fn_801AB950(void* a0, void* a1);

extern const char lbl_803836A8[];  // 0x803836A8 ("UIParticleEffect.cpp")
extern const char lbl_803836C0[];  // 0x803836C0 (allocation failed message)
extern u8 lbl_803836EC[];          // 0x803836EC (UIParticleEffect vtable)
extern u8 lbl_805F94D0[];          // 0x805F94D0 (alloc tag)
extern u8 lbl_805FB7F0;            // 0x805FB7F0 (fader initialized)
extern f32 lbl_805FB7F4;           // 0x805FB7F4 (fader elapsed)
extern f32 lbl_805F960C;           // 0x805F960C (fader duration)
extern u8* lbl_805FB68C;           // 0x805FB68C (UI pool A)
extern u8* lbl_805FB690;           // 0x805FB690 (UI pool B)
extern u8* lbl_805FB694;           // 0x805FB694 (UI pool C)
extern u8* lbl_805FB698;           // 0x805FB698 (anim set table)
}

extern "C++" void* gScreenSets;    // 0x805FB654

extern "C" {

// 0x801D5F6C
// Returns 1 when the fader is not running or its elapsed time has reached the duration.
s32 fn_801D5F6C(void) {
    if (lbl_805FB7F0 == 0) {
        return 1;
    }
    return (lbl_805FB7F4 >= lbl_805F960C) ? 1 : 0;
}

// 0x800553DC
// Quaternion (x, y, z, w) to 3x4 rotation matrix.
void fn_800553DC(f32* arg0, u8* arg1) {
    f32 x = arg0[0];
    f32 y = arg0[1];
    f32 z = arg0[2];
    f32 w = arg0[3];
    f32* m = (f32*)arg1;

    m[9] = 0.0f;
    m[10] = 0.0f;
    m[11] = 0.0f;
    m[0] = 1.0f - ((2.0f * y) * y + (2.0f * z) * z);
    m[1] = (2.0f * y) * x + (2.0f * z) * w;
    m[2] = (2.0f * z) * x - (2.0f * y) * w;
    m[3] = (2.0f * y) * x - (2.0f * z) * w;
    m[4] = 1.0f - ((2.0f * x) * x + (2.0f * z) * z);
    m[5] = (2.0f * y) * z + (2.0f * x) * w;
    m[6] = (2.0f * z) * x + (2.0f * y) * w;
    m[7] = (2.0f * y) * z - (2.0f * x) * w;
    m[8] = 1.0f - ((2.0f * x) * x + (2.0f * y) * y);
}

// 0x80055828
// Transforms vector v by the 3x4 matrix mtx (translation in elements 9..11) into out.
void fn_80055828(f32* out, u8* mtx, const Vec3* v) {
    f32* m = (f32*)mtx;
    f32 vx = v->x;
    f32 vy = v->y;
    f32 vz = v->z;

    out[0] = m[9] + (m[6] * vz + (m[0] * vx + m[3] * vy));
    out[1] = m[10] + (m[7] * vz + (m[1] * vx + m[4] * vy));
    out[2] = m[11] + (m[8] * vz + (m[2] * vx + m[5] * vy));
}

// 0x8005A244
// Normalized cross product out = normalize(a x b); zero vector when a x b has zero length.
void fn_8005A244(Vec3* out, const Vec3* a, const Vec3* b) {
    f32 cx = (a->y * b->z) - (a->z * b->y);
    f32 cy = (a->z * b->x) - (a->x * b->z);
    f32 cz = (a->x * b->y) - (a->y * b->x);
    f32 len2 = (cz * cz) + ((cx * cx) + (cy * cy));

    if (len2 == 0.0f) {
        out->z = 0.0f;
        out->y = 0.0f;
        out->x = 0.0f;
        return;
    }
    f32 inv = 1.0f / sqrtf(len2);
    out->x = cx * inv;
    out->y = cy * inv;
    out->z = cz * inv;
}

// 0x8005C158
// Normalizes vector v into out.
void fn_8005C158(Vec3* out, const Vec3* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    f32 inv = 1.0f / sqrtf((z * z) + ((x * x) + (y * y)));

    out->x = x * inv;
    out->y = y * inv;
    out->z = z * inv;
}

// 0x8005C1CC
// Scales the four floats of q by s into out.
void fn_8005C1CC(f32* out, f32* q, f32 s) {
    out[0] = q[0] * s;
    out[1] = q[1] * s;
    out[2] = q[2] * s;
    out[3] = q[3] * s;
}

// 0x801A84F8
// Allocates and initializes a UIParticleEffect object; returns NULL on allocation failure.
u8* fn_801A84F8(void) {
    u8* p = (u8*)fn_8018D428(0x18C, lbl_803836A8, lbl_805F94D0, 0x2C);
    if (p == NULL) {
        fn_8018D370(lbl_803836C0);
        return NULL;
    }
    *(void**)(p + 0x138) = lbl_803836EC;
    fn_8019B2F4(p);
    *(s32*)(p + 0x60) = 2;
    *(s32*)(p + 0x13C) = 3;
    *(s32*)(p + 0x140) = -1;
    *(s32*)(p + 0x178) = 0;
    (*(VCallFn*)(*(u8**)(p + 0x138) + 0x2C))(p, 0x20000);
    *(s32*)(p + 0x144) = 0;
    *(u8*)(p + 0x188) = 1;
    return p;
}

// 0x801A904C
// Scans the current screen set's effect lists for an active effect bound to arg0's group.
void fn_801A904C(u8* arg0) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        return;
    }
    s32 count = *(s32*)(set + 0x60);
    for (s32 i = 0; i < count; i++) {
        u8* owner = *(u8**)(set + 0x40 + i * 4);
        s32 n = *(s32*)(owner + 0x13C);
        for (s32 j = 0; j < n; j++) {
            u8* el = *(u8**)(*(u8**)(owner + 0x140) + j * 4);
            if (*(s32*)(el + 0x60) == 2 && *(s32*)(el + 0x13C) == 3 &&
                *(s32*)(el + 0x140) == *(s32*)(arg0 + 0x140) && *(s32*)(el + 0x178) != 0 &&
                (*(s32*)(el + 0x64) & 2)) {
                return;
            }
        }
    }
}

// 0x801ABA2C
// Lazily creates the three UI element pools.
void fn_801ABA2C(void) {
    if (lbl_805FB68C == NULL) {
        u8* p = (u8*)MemAllocDefault(0x2C);
        lbl_805FB68C = p;
        if (p != NULL) {
            fn_801EB7A4(p, 0x38, 0x10, 0x40, 0);
        }
    }
    if (lbl_805FB690 == NULL) {
        u8* p = (u8*)MemAllocDefault(0x2C);
        lbl_805FB690 = p;
        if (p != NULL) {
            fn_801EB7A4(p, 0x30, 0x10, 0x140, 0);
        }
    }
    if (lbl_805FB694 == NULL) {
        u8* p = (u8*)MemAllocDefault(0x2C);
        lbl_805FB694 = p;
        if (p != NULL) {
            fn_801EB7A4(p, 0x14, 0x10, 0x40, 0);
        }
    }
}

// 0x801ABC50
// Returns the element's five child slots (+0x4..+0x18) and its own slot to the UI pools.
void fn_801ABC50(void* h) {
    u8* arg0 = (u8*)h;
    u8* cur = arg0;

    for (s32 i = 0; i < 5; i++) {
        u8* item = *(u8**)(cur + 0x4);
        if (item != NULL) {
            fn_801EBA20(lbl_805FB690, item);
        }
        cur += 4;
        *(void**)cur = NULL;
    }
    {
        u8* item = *(u8**)(arg0 + 0x18);
        if (item != NULL) {
            fn_801EBA20(lbl_805FB694, item);
        }
    }
    *(void**)(arg0 + 0x18) = NULL;
    fn_801EBA20(lbl_805FB68C, arg0);
}

// 0x801ABCEC
// Runs the UI element update, then the owner hook when flag 4 is set.
void fn_801ABCEC(void* h, void* owner) {
    fn_801AB820((u8*)h, (u8*)owner);
    if (fn_801AB7E8((u8*)h) & 4) {
        fn_801AB950(h, owner);
    }
}

// 0x801ABD58
// Frees the anim set table and clears its pointer.
void fn_801ABD58(void) {
    BufferFree(lbl_805FB698);
    lbl_805FB698 = NULL;
}

}  // extern "C"
