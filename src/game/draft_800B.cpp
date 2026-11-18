// Grow juice weapon functions in 0x800Bxxxx (aim/reload logic and constructors).
#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void EulerToQuat(Quat* out, const Vec3* euler);                  // 0x80145B00
void fn_80146180(void* quat, const Vec3* axis, f32 angle);       // 0x80146180 (rotate quat about axis)
void fn_800690D4(u8* p, s32 a, void* params, s32 b);             // 0x800690D4 (base ctor)
void fn_800A7D88(void* obj, u32 hash);                           // 0x800A7D88 (send event hash to obj)
void fn_8006AADC(u8* p, f32 delta);                              // 0x8006AADC (base update)
void fn_801B7020(u8* p);                                         // 0x801B7020

extern u8 lbl_8035DE40[];                                        // 0x8035DE40 (vtable)

typedef void (*UpdateFn)(u8*, f32);

// Marks the render object's transform dirty (same four flag ors the original does after each write).
static inline void MarkXformDirty(u8* t) {
    *(u32*)(t + 0x20) |= 0x8000;
    *(u32*)(t + 0x20) |= 0x10000000;
    *(u32*)(t + 0x20) |= 0x4000;
    *(u32*)(t + 0x20) |= 0x10000000;
}

// 0x800B0118
// Rotates the aim quat (+0x270) by the angle change since last call, then places the render
// object at base position + aim Y axis * length (+0x280).
void fn_800B0118(u8* p, f32 angle) {
    f32 base_z = *(f32*)(p + 0xD0);
    f32 bx = *(f32*)(p + 0x264) + 0.0f;
    f32 by = *(f32*)(p + 0x268) + 0.0f;
    f32 bz = *(f32*)(p + 0x26C) + base_z;
    f32 diff = angle - *(f32*)(p + 0x284);
    if (fabsf(diff) > 0.0f) {
        Vec3 axis = { 0.0f, 0.0f, 1.0f };
        fn_80146180(p + 0x270, &axis, diff);
    }
    *(f32*)(p + 0x284) = angle;
    Vec3 dir = QuatGetAxisY((Quat*)(p + 0x270));
    f32 len = *(f32*)(p + 0x280);
    u8* t = *(u8**)(p + 0x4);
    *(f32*)(t + 0x0) = bx + dir.x * len;
    *(f32*)(t + 0x4) = by + dir.y * len;
    *(f32*)(t + 0x8) = bz + dir.z * len;
    MarkXformDirty(*(u8**)(p + 0x4));
}

// 0x800B02C0
// Moves the first loaded round (slots +0x2AC..+0x2BC) into the chamber (+0x288): copies the
// weapon's position/rotation onto the round, clears the slot and sends the loaded event.
void GrowJuiceWeapon_LoadIntoChamber(void* a0) {
    u8* p = (u8*)a0;
    if (p == NULL) return;

    s32 idx;
    if (*(s32*)(p + 0x2AC) != 0) idx = 0;
    else if (*(s32*)(p + 0x2B0) != 0) idx = 1;
    else if (*(s32*)(p + 0x2B4) != 0) idx = 2;
    else if (*(s32*)(p + 0x2B8) != 0) idx = 3;
    else if (*(s32*)(p + 0x2BC) != 0) idx = 4;
    else idx = -1;

    u8* slot = p + idx * 4;
    u8* src = *(u8**)(p + 0x4);
    u8* round = *(u8**)(slot + 0x2AC);
    *(u8**)(p + 0x288) = round;

    u8* dst = *(u8**)(round + 0x4);
    *(f32*)(dst + 0x0) = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4) = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x8) = *(f32*)(src + 0x8);
    MarkXformDirty(*(u8**)(round + 0x4));

    src = *(u8**)(p + 0x4);
    round = *(u8**)(p + 0x288);
    dst = *(u8**)(round + 0x4);
    *(f32*)(dst + 0xC) = *(f32*)(src + 0xC);
    *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
    *(f32*)(dst + 0x14) = *(f32*)(src + 0x14);
    *(f32*)(dst + 0x18) = *(f32*)(src + 0x18);
    MarkXformDirty(*(u8**)(round + 0x4));

    *(u8**)(slot + 0x2AC) = NULL;
    fn_800A7D88(*(void**)(p + 0x288), 0x51126A4Au);
    fn_800A7D88(p, 0x51126A4Au);
}

// 0x800B04A8
// Round finished its flight: finds it in the in-flight slots (+0x298), clears that slot and
// puts it in the first free slot of the loaded array (+0x2AC).
void GrowJuiceWeapon_RoundDoneFlight(void* a0, void* a1) {
    u8* p = (u8*)a0;
    if (p == NULL || a1 == NULL) return;

    for (s32 i = 0; i < 5; i++) {
        if (*(void**)(p + 0x298 + i * 4) == a1) {
            *(void**)(p + 0x298 + i * 4) = NULL;
            for (s32 j = 0; j < 5; j++) {
                if (*(void**)(p + 0x2AC + j * 4) == NULL) {
                    *(void**)(p + 0x2AC + j * 4) = a1;
                    return;
                }
            }
            return;
        }
    }
}

// 0x800B058C
// Copies the non-null slot pointers at +0x298..+0x2A8 into out[]; returns 0 if out (size max) is
// too small, else 1.
s32 fn_800B058C(u8* p, void** out, s32 max) {
    s32 count = 0;
    for (s32 off = 0x298; off <= 0x2A8; off += 4) {
        void* v = *(void**)(p + off);
        if (v != NULL) {
            if (count >= max) return 0;
            *out++ = v;
            count++;
        }
    }
    return 1;
}

// 0x800B068C
// Constructor: base ctor, sets the vtable, resets the aim state (quats identity, scalars zero)
// and clears the slot arrays. Returns the object.
void* fn_800B068C(u8* p, s32 a, void* params, s32 b) {
    fn_800690D4(p, a, params, b);
    *(u8**)(p + 0x0) = lbl_8035DE40;
    *(f32*)(p + 0x264) = 0.0f;
    *(f32*)(p + 0x268) = 0.0f;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 0.0f;
    *(f32*)(p + 0x27C) = 1.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(f32*)(p + 0x288) = 0.0f;
    *(f32*)(p + 0x28C) = 1.0f;
    *(f32*)(p + 0x290) = 0.0f;
    *(f32*)(p + 0x294) = 0.0f;
    *(s32*)(p + 0x298) = -1;
    *(s32*)(p + 0x2A0) = 0;
    *(s32*)(p + 0x2B4) = 0;
    *(s32*)(p + 0x2C8) = 0;
    *(s32*)(p + 0x2A4) = 0;
    *(s32*)(p + 0x2B8) = 0;
    *(s32*)(p + 0x2CC) = 0;
    *(s32*)(p + 0x2A8) = 0;
    *(s32*)(p + 0x2BC) = 0;
    *(s32*)(p + 0x2D0) = 0;
    *(s32*)(p + 0x2AC) = 0;
    *(s32*)(p + 0x2C0) = 0;
    *(s32*)(p + 0x2D4) = 0;
    *(s32*)(p + 0x2B0) = 0;
    *(s32*)(p + 0x2C4) = 0;
    *(s32*)(p + 0x2D8) = 0;
    return p;
}

// 0x800B0B88
// Resets the aim state, copies the placement (+0xC8 position, +0xD4 euler in quarter turns) onto
// the render object and clears the slot arrays.
void fn_800B0B88(u8* p) {
    *(f32*)(p + 0x264) = 0.0f;
    *(f32*)(p + 0x268) = 0.0f;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 0.0f;
    *(f32*)(p + 0x27C) = 1.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(f32*)(p + 0x288) = 0.0f;
    *(f32*)(p + 0x28C) = 1.0f;
    *(f32*)(p + 0x290) = 0.0f;
    *(f32*)(p + 0x294) = 0.0f;
    *(s32*)(p + 0x298) = -1;

    u8* t = *(u8**)(p + 0x4);
    *(f32*)(t + 0x0) = *(f32*)(p + 0xC8);
    *(f32*)(t + 0x4) = *(f32*)(p + 0xCC);
    *(f32*)(t + 0x8) = *(f32*)(p + 0xD0);
    Vec3 euler;
    euler.x = *(f32*)(p + 0xD4) * 0.63661975f;
    euler.y = *(f32*)(p + 0xD8) * 0.63661975f;
    euler.z = *(f32*)(p + 0xDC) * 0.63661975f;
    EulerToQuat((Quat*)(*(u8**)(p + 0x4) + 0xC), &euler);

    *(s32*)(p + 0x2A0) = 0;
    *(s32*)(p + 0x2B4) = 0;
    *(s32*)(p + 0x2C8) = 0;
    *(s32*)(p + 0x2A4) = 0;
    *(s32*)(p + 0x2B8) = 0;
    *(s32*)(p + 0x2CC) = 0;
    *(s32*)(p + 0x2A8) = 0;
    *(s32*)(p + 0x2BC) = 0;
    *(s32*)(p + 0x2D0) = 0;
    *(s32*)(p + 0x2AC) = 0;
    *(s32*)(p + 0x2C0) = 0;
    *(s32*)(p + 0x2D4) = 0;
    *(s32*)(p + 0x2B0) = 0;
    *(s32*)(p + 0x2C4) = 0;
    *(s32*)(p + 0x2D8) = 0;
}

// 0x800B10BC
// Two-quat variant of fn_800B0118: rotates both quats (+0x280 and +0x270) by the angle change,
// positions the render object along the +0x280 Y axis (length +0x290) and copies +0x270 onto it.
void fn_800B10BC(u8* p, f32 angle) {
    f32 base_z = *(f32*)(p + 0xD0);
    f32 diff = angle - *(f32*)(p + 0x294);
    f32 bx = *(f32*)(p + 0x264) + 0.0f;
    f32 by = *(f32*)(p + 0x268) + 0.0f;
    f32 bz = *(f32*)(p + 0x26C) + base_z;
    if (fabsf(diff) > 0.0f) {
        Vec3 axis1 = { 0.0f, 0.0f, 1.0f };
        fn_80146180(p + 0x280, &axis1, diff);
        Vec3 axis2 = { 0.0f, 0.0f, 1.0f };
        fn_80146180(p + 0x270, &axis2, diff);
    }
    *(f32*)(p + 0x294) = angle;
    Vec3 dir = QuatGetAxisY((Quat*)(p + 0x280));
    f32 len = *(f32*)(p + 0x290);
    u8* t = *(u8**)(p + 0x4);
    *(f32*)(t + 0x0) = bx + dir.x * len;
    *(f32*)(t + 0x4) = by + dir.y * len;
    *(f32*)(t + 0x8) = bz + dir.z * len;
    MarkXformDirty(*(u8**)(p + 0x4));

    t = *(u8**)(p + 0x4);
    *(f32*)(t + 0xC) = *(f32*)(p + 0x270);
    *(f32*)(t + 0x10) = *(f32*)(p + 0x274);
    *(f32*)(t + 0x14) = *(f32*)(p + 0x278);
    *(f32*)(t + 0x18) = *(f32*)(p + 0x27C);
    MarkXformDirty(*(u8**)(p + 0x4));
}

// 0x800B12AC
// If a round is loaded in +0x2B0, walks the slots +0x2B0 down to +0x2A0: slot 4 moves to the first
// free spot of the +0x2C8 array; the others send an event, shift one slot up (+0x2B8 block) and clear.
void GrowJuiceWeaponBase_AttemptReload(void* a0) {
    u8* p = (u8*)a0;
    if (p == NULL || *(s32*)(p + 0x2B0) == 0) return;

    for (s32 i = 4; i >= 0; i--) {
        u8* slot = p + 0x2A0 + i * 4;
        void* round = *(void**)slot;
        if (round == NULL) continue;
        if (i == 4) {
            for (s32 j = 0; j < 5; j++) {
                if (*(s32*)(p + 0x2C8 + j * 4) == 0) {
                    *(void**)(p + 0x2C8 + j * 4) = round;
                    break;
                }
            }
            *(void**)slot = NULL;
        } else {
            if (i + 1 == 4) fn_800A7D88(p, 0xA61CF023u);
            fn_800A7D88(*(void**)slot, 0xA61CF023u);
            *(void**)(slot + 0x18) = *(void**)slot;
            *(void**)slot = NULL;
        }
    }
}

// 0x800B13B8
// Counts the non-null slots among the ten loaded/queued round slots.
s32 fn_800B13B8(u8* p) {
    s32 n = 0;
    if (*(s32*)(p + 0x2A0) != 0) n = 1;
    if (*(s32*)(p + 0x2B4) != 0) n += 1;
    if (*(s32*)(p + 0x2A4) != 0) n += 1;
    if (*(s32*)(p + 0x2B8) != 0) n += 1;
    if (*(s32*)(p + 0x2A8) != 0) n += 1;
    if (*(s32*)(p + 0x2BC) != 0) n += 1;
    if (*(s32*)(p + 0x2AC) != 0) n += 1;
    if (*(s32*)(p + 0x2C0) != 0) n += 1;
    if (*(s32*)(p + 0x2B0) != 0) n += 1;
    if (*(s32*)(p + 0x2C4) != 0) n += 1;
    return n;
}

// 0x800B15B4
// Update: sets the dirty bit (1) on every child's render transform in the list at +0xB0, runs the
// base update and the virtual update (vtable +0x74), clears the bit again, then fn_801B7020.
void fn_800B15B4(u8* p, f32 delta) {
    for (u8* n = *(u8**)(p + 0xB0); n != NULL; n = *(u8**)(n + 0x8)) {
        u8* child = *(u8**)(n + 0x4);
        if (child != NULL) {
            u8* t = *(u8**)(child + 0x4);
            *(u32*)(t + 0x20) |= 1;
        }
    }
    fn_8006AADC(p, delta);
    (*(UpdateFn*)(*(u8**)p + 0x74))(p, delta);
    for (u8* n = *(u8**)(p + 0xB0); n != NULL; n = *(u8**)(n + 0x8)) {
        u8* child = *(u8**)(n + 0x4);
        if (child != NULL) {
            u8* t = *(u8**)(child + 0x4);
            *(u32*)(t + 0x20) &= 0xFFFFFFFEu;
        }
    }
    fn_801B7020(p);
}

}  // extern "C"
