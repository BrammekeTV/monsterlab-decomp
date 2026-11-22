// Small functions in 0x800B-0x802Fxxxx: spin/offset helpers, record release loops, sound
// playback wrappers, and a few math and table helpers.
#include "types.h"
#include "engine/math.h"

extern "C" {

void OSReport(const char* fmt, ...);
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);  // 0x80209E70
u32 RandomU32(void);                                         // 0x80143460
void TextureRelease(void* tex);                              // 0x80184028
f32 cosf(f32 x);
f32 sinf(f32 x);
f32 sqrtf(f32 x);

s32 fn_800BB7D0(u8* p, u8* item);                            // 0x800BB7D0
void fn_800BD064(u8* e, f32 dt);                              // 0x800BD064
void fn_800BDC14(u8* e, f32 dt);                              // 0x800BDC14
void fn_800BDC68(u8* item, f32 dt);                           // 0x800BDC68
s32 fn_800BE9E0(u8* item, u8* ctx, u8* flag, f32 dt);        // 0x800BE9E0
s32 fn_800BF044(u8* item, const f32* vec);                   // 0x800BF044
void fn_800BF158(u8* item);                                  // 0x800BF158
void fn_800BFF64(u8* p, f32 dt);                                      // 0x800BFF64
u8* fn_800C253C(u8** obj, s32 kind);                         // 0x800C253C
void fn_800C6F18(u8* item);                                   // 0x800C6F18
void fn_800C72C4(u8* item, u8* other);                       // 0x800C72C4
void fn_800C8984(u8* p);                                     // 0x800C8984
void fn_800C89F0(u8* p);                                     // 0x800C89F0
s32 fn_801098DC(void* h);                                    // 0x801098DC
s32 fn_80109A0C(void* h);                                    // 0x80109A0C
void fn_801458B0(f32* quat, f32* a, f32* b);                 // 0x801458B0
void fn_80146180(void* quat, const Vec3* axis, f32 angle);   // 0x80146180
s32 fn_80196678(void* gx, void* mat, f32 t);                 // 0x80196678
s32 fn_80196B4C(u8* arg0, const f32* arg1);                  // 0x80196B4C
s32 fn_80196B10(u8* a3);                                     // 0x80196B10
s32 fn_80196C88(u8* a3);                                     // 0x80196C88
s32 fn_8019B6CC(u8* arg0, f32 t);                            // 0x8019B6CC
void* fn_801D40E0(s32 n);                                    // 0x801D40E0
void* fn_80209814(void* v);                                  // 0x80209814
void fn_8020A398(s32 handle);                                // 0x8020A398
void* fn_80207814(void* unused, s32 value);                  // 0x80207814
void fn_8020788C(void* unused, s32 value);                   // 0x8020788C
const char* fn_8020716C(s32 code);                                       // 0x8020716C
s32 fn_80323644(void* tbl, void* a, void* b);                // 0x80323644
s32 fn_80323908(void);                                       // 0x80323908
f32 fn_8033F1FC(f32 y, f32 x);                               // 0x8033F1FC (MSL atan2)
f64 fn_8033F1F8(f64 x);                                      // 0x8033F1F8 (MSL asin)
void fn_801A9F5C(u8* obj, f32 t);                        // 0x801A9F5C

extern void* lbl_805FB658;      // 0x805FB658 (global object pointer)
extern u8 lbl_805FB1A1;         // 0x805FB1A1
extern void* lbl_805FBBAC;      // 0x805FBBAC (voice SFX handle)
extern s32 lbl_805FB380;        // 0x805FB380
extern u8* lbl_805FB298;        // 0x805FB298
extern u8* lbl_805FB29C;        // 0x805FB29C
extern u8* lbl_805FB2A0;        // 0x805FB2A0
extern u8 lbl_805FBAD4;         // 0x805FBAD4
extern u8 lbl_805FBAD5;         // 0x805FBAD5
extern f32 lbl_805FBAD8;        // 0x805FBAD8
extern u8 lbl_805FBAA4;         // 0x805FBAA4
extern u8 lbl_805F98C0;         // 0x805F98C0
extern u8 lbl_805F98C1;         // 0x805F98C1
extern u8 lbl_8051E6A8[];       // 0x8051E6A8
extern u8 lbl_80386ED0[];       // 0x80386ED0 (debug string block)
extern s32 lbl_805FBB4C;        // 0x805FBB4C
extern s32 lbl_805FBB50;        // 0x805FBB50
extern f32 lbl_805FFA74;        // 0x805FFA74

// Shared helper for the two spin-offset functions: scrambles a random offset of the
// base vector and rotates a fixed axis by it. lo and span are in degrees (scaled by 1/90).
static void SpinOffset(f32* out, const f32* base, f32 lo, f32 span) {
    f32 mag = 0.011111111f * (lo + (span * (2.3283064e-10f * (f32)RandomU32())));
    f32 scale = 7.5f + (1.0f * (2.3283064e-10f * (f32)RandomU32()));
    Quat q;
    f32 a[3] = {0.0f, 0.0f, 1.0f};
    f32 b[3] = {0.0f, 1.0f, 0.0f};
    fn_801458B0((f32*)&q, a, b);
    Vec3 up;
    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;
    fn_80146180(&q, &up, (RandomU32() & 1) ? mag : -mag);
    Vec3 ay = QuatGetAxisY(&q);
    out[0] = base[0] + ay.x * scale;
    out[1] = base[1] + ay.y * scale;
    out[2] = base[2] + ay.z * scale;
}

// Voice-handle slot helper: lazily creates the shared object in lbl_805FBBAC.
static void EnsureVoiceSlot(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// Reads a slot of the object's table at obj+0x138: *(*(obj+0x138) + off).
static inline void* SlotAt(u8* obj, s32 off) {
    return *(void**)(*(u8**)(obj + 0x138) + off);
}

// Returns the low 32 bits of a u64 as a pointer (matches the PPC r4 half of a 64-bit return).
static inline u8* LowWord(u64 v) {
    return (u8*)(u64)(u32)v;
}

typedef void (*SlotVoidS32)(u8* self, s32 arg);
typedef f32 (*SlotF32S32)(u8* self, s32 arg);
typedef void (*SlotVoidF32)(u8* self, f32 arg);

// 0x800BB658
// Per-frame update of the 18 + 8 + 50 entry tables: calls the per-entry update helpers with dt.
void fn_800BB658(u8* p, f32 dt) {
    s32 i;
    fn_800BFF64(p + 0x240, dt);
    for (i = 0; i < 0x12; i++) {
        u8* e = *(u8**)(p + 4 * i + 0x94);
        if (*(u8*)(e + 0x298) != 0) {
            fn_800BD064(e, dt);
        }
    }
    for (i = 0; i < 8; i++) {
        u8* e = *(u8**)(p + 4 * i + 0x130);
        if (*(u8*)(e + 0x264) != 0) {
            fn_800BDC14(e, dt);
            fn_800BDC68(e, dt);
        }
    }
    for (i = 0; i < 0x32; i++) {
        u8* e = *(u8**)(p + 4 * i + 0x170);
        if ((u32)(*(s32*)(e + 0x290) - 3) <= 2U) {
            u8 flag = 0;
            fn_800BE9E0(e, p + 0x240, &flag, dt);
            if (flag == 0) {
                if (fn_800BB7D0(p, e) != 0) {
                    fn_800BF158(e);
                } else {
                    u8* src = *(u8**)(*(u8**)(p + 0x238) + 4);
                    f32 vec[3];
                    vec[0] = *(f32*)(src + 0x0);
                    vec[1] = *(f32*)(src + 0x4);
                    vec[2] = *(f32*)(src + 0x8);
                    if (fn_800BF044(e, vec) != 0) {
                        fn_800BF158(e);
                    }
                }
            }
        }
    }
}

// 0x800BD6D4
// Random offset of arg1 into arg0 (spin by 60 + 75*rand degrees, scaled).
void fn_800BD6D4(u8* arg0, u8* arg1) {
    SpinOffset((f32*)arg0, (const f32*)arg1, 60.0f, 75.0f);
}

// 0x800BE148
// Same as fn_800BD6D4 with a 45 + 90*rand degree spin.
void fn_800BE148(f32* arg0, u8* arg1) {
    SpinOffset(arg0, (const f32*)arg1, 45.0f, 90.0f);
}

// 0x800C7FB0
// Walks three record tables (0x14-byte stride); for types 2..5 it refreshes the item and,
// for type 5, forwards the item to the callbacks fn_800C89F0 / fn_800C8984.
void fn_800C7FB0(u8* p) {
    s32 i;
    for (i = 0; i < *(s32*)(p + 0x1784); i++) {
        u8* rec = p + 0x14 * i;
        u8* item = p + 0x14 + 0x14 * i;
        s32 type = *(s32*)(rec + 0x24);
        if ((u32)(type - 2) <= 3U) {
            s32 flagged = (type == 2 || type == 3 || type == 5) ? 1 : 0;
            fn_800C6F18(item); // returns nothing; r4 is leftover below
            if (*(s32*)(rec + 0x24) == 5) {
                fn_800C72C4(item, 0 /* leftover r4 */);
                if (flagged) {
                    fn_800C89F0(p);
                }
            }
        }
    }
    for (i = 0; i < *(s32*)(p + 0x1788); i++) {
        u8* rec = p + 0x14 * i;
        u8* item = p + 0x7E4 + 0x14 * i;
        s32 type = *(s32*)(rec + 0x7F4);
        if ((u32)(type - 2) <= 3U) {
            fn_800C6F18(item); // returns nothing; r4 is leftover below
            if (*(s32*)(rec + 0x7F4) == 5) {
                fn_800C72C4(item, 0 /* leftover r4 */);
                fn_800C8984(p);
            }
        }
    }
    for (i = 0; i < *(s32*)(p + 0x178C); i++) {
        u8* rec = p + 0x14 * i;
        u8* item = p + 0xFB4 + 0x14 * i;
        s32 type = *(s32*)(rec + 0xFC4);
        if ((u32)(type - 2) <= 3U) {
            fn_800C6F18(item); // returns nothing; r4 is leftover below
            if (*(s32*)(rec + 0xFC4) == 5) {
                fn_800C72C4(item, 0 /* leftover r4 */);
            }
        }
    }
}

// 0x800CD7D8
// Picks a kind (1, 2 or 3) from a random roll and the object's current state at +0x20.
u8* fn_800CD7D8(u8* arg0, u8* arg1, s32 arg2) {
    s32 state = *(s32*)(arg0 + 0x20);
    u32 r;
    if (state == 3) {
        if ((RandomU32() & 0xF) < 0xEU) {
            return fn_800C253C((u8**)arg0, 1);
        }
        *(s32*)(arg0 + 0x20) = 2;
        return fn_800C253C((u8**)arg0, 2);
    }
    if (state == 2) {
        if ((RandomU32() & 0xF) < 0xEU) {
            return fn_800C253C((u8**)arg0, 1);
        }
        *(s32*)(arg0 + 0x20) = 3;
        return fn_800C253C((u8**)arg0, 3);
    }
    r = RandomU32() % 18U;
    if (r < 0xEU) {
        return fn_800C253C((u8**)arg0, 1);
    }
    if (r < 0x10U) {
        *(s32*)(arg0 + 0x20) = 2;
        return fn_800C253C((u8**)arg0, 2);
    }
    *(s32*)(arg0 + 0x20) = 3;
    return fn_800C253C((u8**)arg0, 3);
}

// 0x800DD3D4
// Copies the transform (7 floats from src+0..0x18), stores its axis-Y and the two points,
// normalises the direction arg2 - arg1 and builds the quaternion from it into arg0+0x1A0.
void fn_800DD3D4(u8* arg0, u8* arg1, u8* arg2) {
    u8* src = *(u8**)(arg0 + 0x4);
    Vec3 ax;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 len2;
    f32 inv;
    f32 vz[3];

    *(f32*)(arg0 + 0x1B0) = *(f32*)(src + 0x0);
    *(f32*)(arg0 + 0x1B4) = *(f32*)(src + 0x4);
    *(f32*)(arg0 + 0x1B8) = *(f32*)(src + 0x8);
    *(f32*)(arg0 + 0x1C8) = *(f32*)(src + 0xC);
    *(f32*)(arg0 + 0x1CC) = *(f32*)(src + 0x10);
    *(f32*)(arg0 + 0x1D0) = *(f32*)(src + 0x14);
    *(f32*)(arg0 + 0x1D4) = *(f32*)(src + 0x18);
    ax = QuatGetAxisY((const Quat*)(src + 0xC));
    *(f32*)(arg0 + 0x1BC) = ax.x;
    *(f32*)(arg0 + 0x1C0) = ax.y;
    *(f32*)(arg0 + 0x1C4) = ax.z;
    *(f32*)(arg0 + 0x188) = *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x18C) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x190) = *(f32*)(arg1 + 0x8);
    dx = *(f32*)(arg2 + 0x0) - *(f32*)(arg1 + 0x0);
    dy = *(f32*)(arg2 + 0x4) - *(f32*)(arg1 + 0x4);
    dz = *(f32*)(arg2 + 0x8) - *(f32*)(arg1 + 0x8);
    len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    inv = 1.0f / sqrtf(len2);
    *(f32*)(arg0 + 0x194) = dx * inv;
    *(f32*)(arg0 + 0x198) = dy * inv;
    *(f32*)(arg0 + 0x19C) = dz * inv;
    vz[0] = 0.0f;
    vz[1] = 0.0f;
    vz[2] = 1.0f;
    fn_801458B0((f32*)(arg0 + 0x1A0), (f32*)(arg0 + 0x194), vz);
    lbl_805FB1A1 = 1;
}

// 0x800DF630
// Sets arg0 from the quaternion's axis: yaw (+0xC), pitch (+0x10), then a point at arg1
// offset by 0.8 times the axis (+0x14..+0x1C), clearing +0x20, +0x24 and +0x28.
void fn_800DF630(u8* arg0, u8* arg1, u8* quat) {
    Vec3 axis = QuatGetAxisY((const Quat*)quat);
    f32 len2;
    f32 len = 0.0f;
    f32 zt;

    *(f32*)(arg0 + 0xC) = fn_8033F1FC(axis.y, axis.x);
    len2 = (axis.z * axis.z) + ((axis.x * axis.x) + (axis.y * axis.y));
    if (len2 > 0.0f) {
        len = sqrtf(len2);
    }
    if (len < 0.001f) {
        *(f32*)(arg0 + 0x10) = 0.0f;
        *(f32*)(arg0 + 0xC) = 0.0f;
    } else {
        zt = axis.z / len;
        if (zt < -1.0f) {
            *(f32*)(arg0 + 0x10) = -3.1415927f;
        }
        if (zt > 1.0f) {
            *(f32*)(arg0 + 0x10) = 3.1415927f;
        } else {
            *(f32*)(arg0 + 0x10) = (f32)fn_8033F1F8((f64)zt);
        }
    }
    *(f32*)(arg0 + 0x14) = *(f32*)(arg1 + 0x0) + (axis.x * 0.8f);
    *(f32*)(arg0 + 0x18) = *(f32*)(arg1 + 0x4) + (axis.y * 0.8f);
    *(f32*)(arg0 + 0x1C) = *(f32*)(arg1 + 0x8) + (axis.z * 0.8f);
    *(f32*)(arg0 + 0x20) = 0.0f;
    *(f32*)(arg0 + 0x24) = 0.0f;
    *(f32*)(arg0 + 0x28) = 0.0f;
}

// 0x8011AAC0
// Releases the texture and handle references held by each 0x60-byte record (count at +0x25A4).
void fn_8011AAC0(u8* arg0) {
    s32 i;
    u8* rec = arg0;
    lbl_805FB298 = NULL;
    lbl_805FB29C = NULL;
    lbl_805FB2A0 = NULL;
    for (i = 0; i < *(s32*)(arg0 + 0x25A4); i++) {
        u8* t;
        void* h;

        t = *(u8**)(rec + 0x1C);
        if (t != NULL) {
            s32 rc = *(s32*)(t + 0x24) - 1;
            *(s32*)(t + 0x24) = rc;
            if (rc == 0) {
                TextureRelease(t);
            }
            *(u8**)(rec + 0x1C) = NULL;
        }

        h = *(void**)(rec + 0x20);
        if (h != NULL) {
            if (fn_80109A0C(h) == 0) {
                fn_801098DC(h);
            } else {
                s32 rc = *(s32*)((u8*)h + 0x24) - 1;
                *(s32*)((u8*)h + 0x24) = rc;
                if (rc == 0) {
                    TextureRelease(h);
                }
            }
            *(void**)(rec + 0x20) = NULL;
        }

        t = *(u8**)(rec + 0x2C);
        if (t != NULL) {
            s32 rc = *(s32*)(t + 0x24) - 1;
            *(s32*)(t + 0x24) = rc;
            if (rc == 0) {
                TextureRelease(t);
            }
            *(u8**)(rec + 0x2C) = NULL;
        }

        h = *(void**)(rec + 0x30);
        if (h != NULL) {
            if (fn_80109A0C(h) == 0) {
                fn_801098DC(h);
            }
            *(void**)(rec + 0x30) = NULL;
        }

        t = *(u8**)(rec + 0x38);
        if (t != NULL) {
            s32 rc = *(s32*)(t + 0x24) - 1;
            *(s32*)(t + 0x24) = rc;
            if (rc == 0) {
                TextureRelease(t);
            }
            *(u8**)(rec + 0x38) = NULL;
        }

        h = *(void**)(rec + 0x3C);
        if (h != NULL) {
            if (fn_80109A0C(h) == 0) {
                fn_801098DC(h);
            }
            *(void**)(rec + 0x3C) = NULL;
        }

        rec += 0x60;
    }
}

// 0x80138FB8
// Copies the source point at arg0+0x20 into arg1 (nudging a near-zero vector to z = 1), then
// computes arg1 = arg0 + t * (arg0.end - arg0.start) and copies the t value; always returns 1.
s32 fn_80138FB8(u8* arg0, u8* arg1) {
    if (*(f32*)(arg0 + 0x18) < *(f32*)(arg1 + 0x18)) {
        u8* v = *(u8**)(arg0 + 0x20);
        f32 dx;
        f32 dy;
        f32 dz;
        f32 t;

        if (*(f32*)(v + 0x0) <= 0.0001f && *(f32*)(v + 0x0) >= -0.0001f &&
            *(f32*)(v + 0x4) <= 0.0001f && *(f32*)(v + 0x4) >= -0.0001f &&
            *(f32*)(v + 0x8) <= 0.0001f && *(f32*)(v + 0x8) >= -0.0001f) {
            *(f32*)(v + 0x8) = 1.0f;
        }
        v = *(u8**)(arg0 + 0x20);
        *(f32*)(arg1 + 0xC) = *(f32*)(v + 0x0);
        *(f32*)(arg1 + 0x10) = *(f32*)(v + 0x4);
        *(f32*)(arg1 + 0x14) = *(f32*)(v + 0x8);

        dx = *(f32*)(arg0 + 0xC) - *(f32*)(arg0 + 0x0);
        dy = *(f32*)(arg0 + 0x10) - *(f32*)(arg0 + 0x4);
        t = *(f32*)(arg0 + 0x18);
        dz = *(f32*)(arg0 + 0x14) - *(f32*)(arg0 + 0x8);
        *(f32*)(arg1 + 0x0) = *(f32*)(arg0 + 0x0) + (dx * t);
        *(f32*)(arg1 + 0x4) = *(f32*)(arg0 + 0x4) + (dy * t);
        *(f32*)(arg1 + 0x8) = *(f32*)(arg0 + 0x8) + (dz * t);
        *(f32*)(arg1 + 0x18) = *(f32*)(arg0 + 0x18);

        if (arg1 != NULL) {
            u8* a = *(u8**)(arg1 + 0x4C);
            if (a != NULL) {
                u8* c = *(u8**)(*(u8**)(a + 0x4C) + 0x1C);
                if (*(s32*)(c + 0xC) > 0) {
                    *(s32*)(arg1 + 0x48) = *(s32*)*(u8**)(arg0 + 0x1C);
                }
            }
        }
    }
    return 1;
}

// 0x8019B4E8
// Copies the 16 float fields at +0x14..+0x50 to +0x68..+0xA4, then calls the slot functions
// selected by the flags at +0x54 (bit 2 and bit 1) and one more with 0x28.
s32 fn_8019B4E8(u8* arg0) {
    s32 off;
    s32 flags;

    for (off = 0x14; off <= 0x50; off += 4) {
        *(f32*)(arg0 + off + 0x54) = *(f32*)(arg0 + off);
    }
    flags = *(s32*)(arg0 + 0x54);
    if (flags & 2) {
        ((SlotVoidS32)SlotAt(arg0, 0x2C))(arg0, 2);
    } else {
        ((SlotVoidS32)SlotAt(arg0, 0x30))(arg0, 2);
    }
    if (*(s32*)(arg0 + 0x54) & 1) {
        ((SlotVoidS32)SlotAt(arg0, 0x2C))(arg0, 1);
    } else {
        ((SlotVoidS32)SlotAt(arg0, 0x30))(arg0, 1);
    }
    ((SlotVoidS32)SlotAt(arg0, 0x2C))(arg0, 0x28);
    return 0;
}

// 0x801A9CE0
// If flag bit 0 is set: optionally resets via fn_801A9F5C, computes the fade from fn_8019B6CC,
// applies bits 0x10 / 0x20 to the child list at +0x120, and returns 0.
s32 fn_801A9CE0(u8* arg0, f32 farg0, f32 farg1) {
    s32 mask = 0;
    f32 t;
    u8* c;

    if (!(*(s32*)(arg0 + 0x64) & 1)) {
        return 0;
    }
    if (farg1 >= lbl_805FFA74) {
        fn_801A9F5C(arg0, farg1);
    }
    fn_8019B6CC(arg0, farg0);
    t = 0.0f; // original passes f1 left over from fn_8019B6CC (not a defined value)
    if (*(s32*)(arg0 + 0x64) & 0x10) {
        mask |= 0x10;
        t = ((SlotF32S32)SlotAt(arg0, 0x30))(arg0, 0x10);
    }
    if (*(s32*)(arg0 + 0x64) & 0x20) {
        mask |= 0x20;
        t = ((SlotF32S32)SlotAt(arg0, 0x30))(arg0, 0x20);
    }
    if (mask != 0) {
        fn_80196678(lbl_805FB658, arg0 + 0xF0, t);
        fn_80196B4C((u8*)lbl_805FB658, (const f32*)(arg0 + 0x80));
        c = *(u8**)(arg0 + 0x120);
        while (c != NULL) {
            ((SlotVoidS32)SlotAt(c, 0x2C))(c, mask);
            ((SlotVoidF32)SlotAt(c, 0x18))(c, farg0);
            c = *(u8**)(c + 0x124);
        }
        fn_80196C88((u8*)lbl_805FB658);
        fn_80196B10((u8*)lbl_805FB658);
    }
    return 0;
}

// 0x801B6554
// Plays sound arg1 for the object arg0. When arg2 is set the previous sound handle at +0xAC
// is stopped first and the new handle is stored there (arg3 selects the 0 position variant).
void fn_801B6554(u8* arg0, s32 arg1, s32 arg2, s32 arg3) {
    if (arg1 == 0) {
        return;
    }
    if (arg2 != 0) {
        s32 h = *(s32*)(arg0 + 0xAC);
        if (h != -1) {
            fn_8020A398(h);
            *(s32*)(arg0 + 0xAC) = -1;
        }
        EnsureVoiceSlot();
        if (arg3 != 0) {
            *(s32*)(arg0 + 0xAC) = PlaySound((u32)arg1, 0, 0, 0, arg2, 0);
        } else {
            *(s32*)(arg0 + 0xAC) = PlaySound((u32)arg1, (s32)(u64)arg0, *(s32*)(arg0 + 0x4), 0, arg2, 0);
        }
    } else {
        EnsureVoiceSlot();
        if (arg3 != 0) {
            PlaySound((u32)arg1, 0, 0, 0, arg2, 0);
        } else {
            PlaySound((u32)arg1, (s32)(u64)arg0, *(s32*)(arg0 + 0x4), 0, arg2, 0);
        }
    }
}

// 0x801B66EC
// Same as fn_801B6554 with an extra sound parameter (arg4) passed as the last PlaySound argument.
void fn_801B66EC(u8* arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    if (arg1 == 0) {
        return;
    }
    if (arg2 != 0) {
        s32 h = *(s32*)(arg0 + 0xAC);
        if (h != -1) {
            fn_8020A398(h);
            *(s32*)(arg0 + 0xAC) = -1;
        }
        EnsureVoiceSlot();
        if (arg3 != 0) {
            *(s32*)(arg0 + 0xAC) = PlaySound((u32)arg1, 0, 0, 0, arg2, arg4);
        } else {
            *(s32*)(arg0 + 0xAC) = PlaySound((u32)arg1, (s32)(u64)arg0, *(s32*)(arg0 + 0x4), 0, arg2, arg4);
        }
    } else {
        EnsureVoiceSlot();
        if (arg3 != 0) {
            PlaySound((u32)arg1, 0, 0, 0, arg2, arg4);
        } else {
            PlaySound((u32)arg1, (s32)(u64)arg0, *(s32*)(arg0 + 0x4), 0, arg2, arg4);
        }
    }
}

// 0x801CA624
// Rotates the vector at arg0+0x5C+0x6C (x, y, z) by the three angles in arg1 (x, y, z times pi/2),
// then stores the result into arg0+0x30..0x38 and the constant from +0x78 into +0x3C.
void fn_801CA624(u8* arg0, u8* arg1) {
    u8* tr = *(u8**)(arg0 + 0x5C);
    f32 vx = *(f32*)(tr + 0x6C);
    f32 vy = *(f32*)(tr + 0x70);
    f32 vz = *(f32*)(tr + 0x74);
    f32 a0 = 1.5707964f * *(f32*)(arg1 + 0x0);
    f32 a1 = 1.5707964f * *(f32*)(arg1 + 0x4);
    f32 a2 = 1.5707964f * *(f32*)(arg1 + 0x8);
    f32 c;
    f32 s;
    f32 ny;
    f32 nz;
    f32 nx;

    c = (f32)cosf(a0);
    s = (f32)sinf(a0);
    ny = (vy * c) - (vz * s);
    nz = (vz * c) + (vy * s);
    vy = ny;
    vz = nz;

    c = (f32)cosf(a1);
    s = (f32)sinf(a1);
    nz = (vz * c) - (vx * s);
    nx = (vx * c) + (vz * s);
    vx = nx;
    vz = nz;

    c = (f32)cosf(a2);
    s = (f32)sinf(a2);
    ny = (vy * c) + (vx * s);
    nx = (vx * c) - (vy * s);
    vy = ny;
    vx = nx;

    *(f32*)(arg0 + 0x3C) = *(f32*)(tr + 0x78);
    *(f32*)(arg0 + 0x38) = vz;
    *(f32*)(arg0 + 0x34) = vy;
    *(f32*)(arg0 + 0x30) = vx;
}

// 0x801FCDE4
// Resets the object's state fields and the 12-entry table at lbl_8051E6A8 (stride 0x8A0).
void fn_801FCDE4(u8* arg0) {
    u8* p;
    s32 i;

    *(u8*)(arg0 + 0x29) = 0;
    *(u8*)(arg0 + 0xFD) = 0;
    *(u8*)(arg0 + 0x1D1) = 0;
    *(s32*)(arg0 + 0x20) = -1;
    *(s32*)(arg0 + 0x24) = -1;
    *(u8*)(arg0 + 0x28) = 0;
    *(s32*)(arg0 + 0x2C) = 0;
    *(s32*)(arg0 + 0x30) = 0;
    *(s32*)(arg0 + 0x34) = 0;
    *(s32*)(arg0 + 0x38) = 0;
    *(s32*)(arg0 + 0xF0) = 0;
    *(s32*)(arg0 + 0xF4) = -1;
    *(s32*)(arg0 + 0xF8) = -1;
    *(u8*)(arg0 + 0xFC) = 0;
    *(s32*)(arg0 + 0x100) = 0;
    *(s32*)(arg0 + 0x104) = 0;
    *(s32*)(arg0 + 0x108) = 0;
    *(s32*)(arg0 + 0x10C) = 0;
    *(s32*)(arg0 + 0x1C4) = 0;
    *(s32*)(arg0 + 0x1C8) = -1;
    *(s32*)(arg0 + 0x1CC) = -1;
    *(u8*)(arg0 + 0x1D0) = 0;
    *(s32*)(arg0 + 0x1D4) = 0;
    *(s32*)(arg0 + 0x1D8) = 0;
    *(s32*)(arg0 + 0x1DC) = 0;
    *(s32*)(arg0 + 0x1E0) = 0;
    *(s32*)(arg0 + 0x298) = 0;
    *(s32*)(arg0 + 0x29C) = -1;
    *(s32*)(arg0 + 0x2A0) = -1;
    *(s32*)(arg0 + 0x2A4) = -1;
    *(u8*)(arg0 + 0x2A8) = 0;
    lbl_805FBAD4 = 0;
    lbl_805FBAD5 = 0;
    lbl_805FBAD8 = 0.5f;
    *(u8*)(arg0 + 0x2AC) = 0;
    *(s32*)(arg0 + 0x2BC) = 0;
    *(s32*)(arg0 + 0x2C0) = 0;

    p = lbl_8051E6A8;
    for (i = 0; i < 12; i++) {
        *(u8*)(p + 0xFC) = 0;
        *(s32*)(p + 0xC) = 0;
        *(s32*)(p + 0x0) = 0;
        *(u8*)(p + 0xFE) = 0;
        *(u8*)(p + 0x105) = 0;
        *(s32*)(p + 0x110) = 0;
        *(u8*)(p + 0x210) = 0;
        *(s32*)(p + 0x120) = 0;
        *(s32*)(p + 0x114) = 0;
        *(u8*)(p + 0x212) = 0;
        *(u8*)(p + 0x219) = 0;
        *(s32*)(p + 0x224) = 0;
        *(u8*)(p + 0x324) = 0;
        *(s32*)(p + 0x234) = 0;
        *(s32*)(p + 0x228) = 0;
        *(u8*)(p + 0x326) = 0;
        *(u8*)(p + 0x32D) = 0;
        *(s32*)(p + 0x338) = 0;
        *(u8*)(p + 0x438) = 0;
        *(s32*)(p + 0x348) = 0;
        *(s32*)(p + 0x33C) = 0;
        *(u8*)(p + 0x43A) = 0;
        *(u8*)(p + 0x441) = 0;
        *(s32*)(p + 0x44C) = 0;
        *(u8*)(p + 0x54C) = 0;
        *(s32*)(p + 0x45C) = 0;
        *(s32*)(p + 0x450) = 0;
        *(u8*)(p + 0x54E) = 0;
        *(u8*)(p + 0x555) = 0;
        *(s32*)(p + 0x560) = 0;
        *(u8*)(p + 0x660) = 0;
        *(s32*)(p + 0x570) = 0;
        *(s32*)(p + 0x564) = 0;
        *(u8*)(p + 0x662) = 0;
        *(u8*)(p + 0x669) = 0;
        *(s32*)(p + 0x674) = 0;
        *(u8*)(p + 0x774) = 0;
        *(s32*)(p + 0x684) = 0;
        *(s32*)(p + 0x678) = 0;
        *(u8*)(p + 0x776) = 0;
        *(u8*)(p + 0x77D) = 0;
        *(s32*)(p + 0x788) = 0;
        *(u8*)(p + 0x888) = 0;
        *(s32*)(p + 0x798) = 0;
        *(s32*)(p + 0x78C) = 0;
        *(u8*)(p + 0x88A) = 0;
        *(u8*)(p + 0x891) = 0;
        *(s32*)(p + 0x89C) = 0;
        p += 0x8A0;
    }
    lbl_805F98C0 = 1;
    lbl_805F98C1 = 1;
    lbl_805FBAA4 = 0;
}

// 0x802078E8
// One-time setup: registers the two callbacks in a table and runs fn_80323644 / fn_80323908,
// logging the outcome. Returns 1 on success, 0 on failure.
s32 fn_802078E8(void) { // lint:ok
    void* tbl[2];
    s32 r;
    s32 r2;

    if (lbl_805FB380 != 0) {
        return 1;
    }
    OSReport((const char*)(lbl_80386ED0 + 0x4D0));
    tbl[0] = (void*)fn_80207814;
    tbl[1] = (void*)fn_8020788C;
    lbl_805FBB4C = 0;
    lbl_805FBB50 = 0;
    r = fn_80323644(tbl, (void*)fn_8020788C, (void*)fn_80207814);
    lbl_805FBB4C = r;
    if (r == -28 || r == -49) {
        OSReport((const char*)(lbl_80386ED0 + 0x4DC), lbl_805FBB4C, fn_8020716C(lbl_805FBB4C));
        return 0;
    }
    OSReport((const char*)(lbl_80386ED0 + 0x4F0));
    OSReport((const char*)(lbl_80386ED0 + 0x4FC));
    r2 = fn_80323908();
    lbl_805FBB50 = r2;
    if (r2 != 0 && (r2 >= 0 || r2 != -7)) {
        OSReport((const char*)(lbl_80386ED0 + 0x4DC), lbl_805FBB50, fn_8020716C(lbl_805FBB50));
        return 0;
    }
    OSReport((const char*)(lbl_80386ED0 + 0x4F0));
    return 1;
}

}  // extern "C"
