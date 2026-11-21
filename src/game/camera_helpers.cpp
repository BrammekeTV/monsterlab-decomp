// Camera object: helpers of the per-frame update (fn_800DC6C4).
// Original: 0x800DB708, 0x800DC514, 0x800DD1EC, 0x800DD8B4 (auto_03_800DB9A4_text.s)
#include "types.h"
#include <math.h>
#include <string.h>

extern "C" {
s32  fn_801C34CC(u8* centre, u8* a, u8* b, f32* t, u8* closest);              // 0x801C34CC
f32  fn_800DB490(f32* a, f32* b, u8* shapes);                                 // 0x800DB490
void fn_801F5F28(f32* out, const f32* cur, const f32* target, f32 lo, f32 hi, f32 range);  // 0x801F5F28
u32  PadGetButtons(s32 chan);                                                 // 0x801B9B40
u32  PadGetHeld(s32 chan);                                                    // 0x801B9B98
u32  fn_801B9B6C(s32 chan);                                                   // 0x801B9B6C
void fn_800E03F0(u8* p, s32 mode);                                            // 0x800E03F0
BOOL ZoneIsLoaded(void* world, s32 zone);                                     // 0x8012F408
u8*  GetGlobalSettings(void);                                                 // 0x801D2BB8
extern u8  gWorldState[];          // 0x803E6D00 (first word: world block pointer; +0xB0: zone records)
extern u8  lbl_803C4298[];         // 0x803C4298 (+0x4: object, +0x38: pad channel)
extern f32 lbl_8035FD08[3];        // 0x8035FD08 (0.1, 0, 0)
}
extern "C++" s32 gZoneResourceSize;            // 0x805FB94C

typedef u8*  (*ObjGetFn)(u8* obj);
typedef BOOL (*ObjBoolFn)(u8* obj);

// 0x800DB708
// How far along the segment a..b the object `e` (centre at +0x18, radius at +0x14, collision
// shapes at +0x8C) is hit: 1e20 when the segment misses its sphere or it has no shapes.
extern "C" f32 fn_800DB708(f32* a, f32* b, u8* e) {
    f32 t;
    f32 closest[3];
    if (fn_801C34CC(e + 0x18, (u8*)a, (u8*)b, &t, (u8*)closest) < 0) {
        return 1e20f;
    }
    if (t <= 0.0f) {
        closest[0] = a[0]; closest[1] = a[1]; closest[2] = a[2];
    } else if (t >= 1.0f) {
        closest[0] = b[0]; closest[1] = b[1]; closest[2] = b[2];
    }
    f32 dx = closest[0] - *(f32*)(e + 0x18);
    f32 dy = closest[1] - *(f32*)(e + 0x1C);
    f32 dz = closest[2] - *(f32*)(e + 0x20);
    f32 r = *(f32*)(e + 0x14);
    if (dz * dz + (dx * dx + dy * dy) > r * r) {
        return 1e20f;
    }
    u8* shapes = *(u8**)(e + 0x8C);
    if (shapes != NULL) {
        return fn_800DB490(a, b, shapes);
    }
    return 1e20f;
}

// 0x800DC514
// Zone tracking: copies the position of the camera's object (0x2C0) to 0x44 and moves the
// followed point (0x50) towards it. The speed is 3 + 5, or 3 + the speed of the object (vector
// at +0x770, over a frame time held in 1/30..0.1) when the type byte of its transform (+0x34)
// is 4 and bit 0x04000000 of the transform's flags (+0x20) is clear.
extern "C" void fn_800DC514(u8* cam, f32 dt) {
    u8* obj = *(u8**)(cam + 0x2C0);
    const f32* v = *(const f32**)(obj + 0x4);
    *(f32*)(cam + 0x44) = v[0];
    *(f32*)(cam + 0x48) = v[1];
    *(f32*)(cam + 0x4C) = v[2];
    f32 frame = 0.033333335f;
    if (!(dt < 0.033333335f)) {
        frame = 0.1f;
        if (dt <= 0.1f) frame = dt;
    }
    f32 inv = 1.0f / frame;
    obj = *(u8**)(cam + 0x2C0);
    u8* tr = *(u8**)(obj + 0x4);
    f32 speed;
    if (*(u32*)(tr + 0x20) & 0x04000000) {
        speed = 5.0f;
    } else if (*(u8*)(tr + 0x34) == 4) {
        f32 x = *(f32*)(obj + 0x770), y = *(f32*)(obj + 0x774), z = *(f32*)(obj + 0x778);
        f32 len2 = z * z + (x * x + y * y);
        f32 len = 0.0f;
        if (!(len2 <= 0.0f)) len = sqrtf(len2);
        speed = inv * len;
    } else {
        speed = 5.0f;
    }
    f32 target[3] = { *(f32*)(cam + 0x44), *(f32*)(cam + 0x48), *(f32*)(cam + 0x4C) };
    f32 cur[3]    = { *(f32*)(cam + 0x50), *(f32*)(cam + 0x54), *(f32*)(cam + 0x58) };
    f32 out[3];
    f32 rate = 3.0f + speed;
    fn_801F5F28(out, cur, target, (0.1f * rate) * dt, rate * dt, 0.25f);
    *(f32*)(cam + 0x50) = out[0];
    *(f32*)(cam + 0x54) = out[1];
    *(f32*)(cam + 0x58) = out[2];
}

// 0x800DD1EC
// Buttons of the camera: stores the held buttons (0x1C) and, in mode 0 only, tracks the two
// camera buttons (masks at 0x34 / 0x38): a flag while down (0x20 / 0x21) and the time since the
// press (0x24 / 0x28). Pressing one while the other was pressed less than the time at 0x3C ago
// sets the byte at 0x2C and restarts its timer (0x30). Before that, when slot 0x7C of the object
// at lbl_803C4298+4 says so and the value at 0x13C is not 2, the follow state gets mode 1.
//
// The original gets the index of the other button through the integer abs at 0x80332ED0 (named
// __cvt_fp2signed in symbols.txt), also once in the release loop without using the result.
extern "C" void fn_800DD1EC(u8* cam, f32 dt) {
    u8* c = lbl_803C4298;
    u32 pressed = PadGetButtons(*(s32*)(c + 0x38));
    *(u32*)(cam + 0x1C) = PadGetHeld(*(s32*)(c + 0x38));
    u32 released = fn_801B9B6C(*(s32*)(c + 0x38));
    if (*(s32*)(cam + 0x8) != 0) {
        return;
    }
    u8* o = *(u8**)(c + 0x4);
    if (((*(ObjBoolFn*)(*(u8**)o + 0x7C))(o) & 1) && *(s32*)(cam + 0x13C) != 2) {
        fn_800E03F0(cam + 0x68, 1);
    }
    for (s32 i = 0; i < 2; i++) {
        if (pressed & *(u32*)(cam + 0x34 + i * 4)) {
            *(u8*)(cam + 0x20 + i) = 1;
            *(f32*)(cam + 0x24 + i * 4) = 0.0f;
            s32 other = i - 1;
            if (other < 0) other = -other;
            if (*(u8*)(cam + 0x20 + other) != 0 && *(f32*)(cam + 0x24 + other * 4) < *(f32*)(cam + 0x3C)) {
                *(u8*)(cam + 0x2C) = 1;
                *(f32*)(cam + 0x30) = 0.0f;
            }
        }
    }
    for (s32 i = 0; i < 2; i++) {
        if (released & *(u32*)(cam + 0x34 + i * 4)) {
            *(u8*)(cam + 0x20 + i) = 0;
        }
    }
    *(f32*)(cam + 0x24) = *(f32*)(cam + 0x24) + (*(u8*)(cam + 0x20) != 0 ? dt : 0.0f);
    *(f32*)(cam + 0x28) = *(f32*)(cam + 0x28) + (*(u8*)(cam + 0x21) != 0 ? dt : 0.0f);
    *(f32*)(cam + 0x30) = *(f32*)(cam + 0x30) + (*(u8*)(cam + 0x2C) != 0 ? dt : 0.0f);
}

// Sum of consecutive words of the global settings, from offset `from` to `to` inclusive.
static inline s32 SettingsSum(s32 from, s32 to) {
    s32 sum = 0;
    for (s32 off = from; off <= to; off += 4) sum += *(s32*)(GetGlobalSettings() + off);
    return sum;
}

// The zone resource with the number `zi`: one of the zones of the level (1..world +4), or one
// in the ranges the global settings give (counts at +0x68..+0x88), else NULL.
static inline u8* ZoneResource(s32 zi) {
    u8* world = *(u8**)gWorldState;
    if (zi > 0 && zi <= *(s32*)(world + 0x4)) {
        return *(u8**)(world + 0x1A4) + zi * gZoneResourceSize;
    }
    if ((*(s32*)(GetGlobalSettings() + 0x68) + 1 <= zi && zi <= SettingsSum(0x68, 0x7C)) ||
        (SettingsSum(0x68, 0x80) + 1 <= zi && zi <= SettingsSum(0x68, 0x84)) ||
        (SettingsSum(0x68, 0x84) + 1 <= zi && zi <= SettingsSum(0x68, 0x88)) ||
        (SettingsSum(0x68, 0x7C) + 1 <= zi && zi <= SettingsSum(0x68, 0x80))) {
        return *(u8**)(*(u8**)gWorldState + 0x1A4) + zi * gZoneResourceSize;
    }
    return NULL;
}

// 0x800DD8B4
// Fades the objects that stand between the camera and its object (0x2C0). The camera keeps up
// to sixteen entries of 0xC bytes at 0x1FC (object, alpha, zone number; count at 0x2BC).
//
// 1. Entries of zones that are no longer loaded lose their object.
// 2. For every zone record of kind 6 (gWorldState+0xB0, 0x30 bytes each, as many as the counts
//    of the global settings at +0x34..+0x64 add up to): every object of the list of the zone
//    resource (+0x12C, count +0x98) with bit 0x40 of the flags of its transform (+0x1C) set,
//    bit 0x100000 of the flags at +0x20 clear and a non-NULL result of its slot 0xC, that the
//    segment from the camera to the followed object hits (fn_800DB708 <= 1), gets the wanted
//    alpha 0.25 when the followed object is nearer than 2 to its sphere (centre +0x18, radius
//    +0x14), else 0. It is added to the entries with alpha 1 if it is not there yet.
// 3. The alpha of every entry moves to its wanted value (1 when not hit this frame) by at most
//    dt, is held in 0..1 and written to the fourth float of the colour that the result of slot
//    0xC has at +0x38. An entry is removed when it has no object, is back at 1, or its alpha is
//    no longer the one in that colour.
//
// As in the original, removing an entry moves the later entries down but not their wanted values.
extern "C" void fn_800DD8B4(u8* cam, f32 dt) {
    f32 want[16];
    for (s32 i = 0; i < *(s32*)(cam + 0x2BC); i++) {
        want[i] = 1.0f;
        if (!ZoneIsLoaded(gWorldState, *(s32*)(cam + i * 0xC + 0x204))) {
            *(u8**)(cam + i * 0xC + 0x1FC) = NULL;
        }
    }

    const f32* objPos = *(const f32**)(*(u8**)(cam + 0x2C0) + 0x4);
    const f32* camPos = *(const f32**)(cam + 0x4);
    f32 dx = objPos[0] - camPos[0];
    f32 dy = objPos[1] - camPos[1];
    f32 dz = objPos[2] - camPos[2];
    if (dz * dz + (dx * dx + dy * dy) < 0.0001f) {
        dx = lbl_8035FD08[0];
        dy = lbl_8035FD08[1];
        dz = lbl_8035FD08[2];
    }
    camPos = *(const f32**)(cam + 0x4);
    f32 target[3] = { camPos[0] + dx, camPos[1] + dy, camPos[2] + dz };

    for (s32 zr = 0; zr < SettingsSum(0x34, 0x64); zr++) {
        BOOL valid = FALSE;
        if (zr >= 0 && zr < SettingsSum(0x34, 0x64)) valid = TRUE;
        if (!(valid && *(s32*)(*(u8**)(gWorldState + 0xB0) + zr * 0x30 + 0x4) == 6)) {
            continue;
        }
        u8* res = ZoneResource(*(s32*)(*(u8**)(gWorldState + 0xB0) + zr * 0x30 + 0x8));
        if (res == NULL) {
            continue;
        }
        s32 count = *(s32*)(res + 0x98);
        for (s32 k = 0; k < count; k++) {
            u8* e = (*(u8***)(res + 0x12C))[k];
            if (e == NULL) continue;
            if (!(*(u32*)(*(u8**)(e + 0x4) + 0x1C) & 0x40)) continue;
            if (*(u32*)(*(u8**)(e + 0x4) + 0x20) & 0x100000) continue;
            if ((*(ObjGetFn*)(*(u8**)e + 0xC))(e) == NULL) continue;
            if (!(fn_800DB708(*(f32**)(cam + 0x4), target, e) <= 1.0f)) continue;
            f32 ex = *(f32*)(e + 0x18) - objPos[0];
            f32 ey = *(f32*)(e + 0x1C) - objPos[1];
            f32 ez = *(f32*)(e + 0x20) - objPos[2];
            f32 len2 = ez * ez + (ex * ex + ey * ey);
            f32 dist = 0.0f;
            if (!(len2 <= 0.0f)) dist = sqrtf(len2);
            f32 gap = dist - *(f32*)(e + 0x14);
            if (gap < 0.0f) gap = 0.0f;
            f32 alpha = gap < 2.0f ? 0.25f : 0.0f;
            for (s32 j = 0; ; j++) {
                s32 n = *(s32*)(cam + 0x2BC);
                if (j == n) {
                    if (n != 0x10) {
                        want[j] = alpha;
                        u8* slot = cam + j * 0xC;
                        *(u8**)(slot + 0x1FC) = e;
                        *(f32*)(slot + 0x200) = 1.0f;
                        *(s32*)(slot + 0x204) = *(s32*)(res + 0x18);
                        *(s32*)(cam + 0x2BC) = *(s32*)(cam + 0x2BC) + 1;
                    }
                    break;
                }
                if (*(u8**)(cam + j * 0xC + 0x1FC) == e) {
                    want[j] = alpha;
                    break;
                }
            }
        }
    }

    f32 colour[4];
    for (s32 i = 0; i < *(s32*)(cam + 0x2BC); i++) {
        u8* slot = cam + i * 0xC;
        BOOL remove = FALSE;
        if (*(u8**)(slot + 0x1FC) == NULL) {
            remove = TRUE;
        } else if (want[i] == 1.0f && *(f32*)(slot + 0x200) == 1.0f) {
            remove = TRUE;
        } else {
            u8* o = *(u8**)(slot + 0x1FC);
            const u8* m = (*(ObjGetFn*)(*(u8**)o + 0xC))(o);
            colour[0] = *(const f32*)(m + 0x38);
            colour[1] = *(const f32*)(m + 0x3C);
            colour[2] = *(const f32*)(m + 0x40);
            colour[3] = *(const f32*)(m + 0x44);
            if (*(f32*)(slot + 0x200) != colour[3]) remove = TRUE;
        }
        if (remove) {
            memmove(slot + 0x1FC, slot + 0x208, (*(s32*)(cam + 0x2BC) - i - 1) * 0xC);
            i--;
            *(s32*)(cam + 0x2BC) = *(s32*)(cam + 0x2BC) - 1;
            continue;
        }
        f32 d = want[i] - *(f32*)(slot + 0x200);
        if (d > dt) d = dt;
        else if (d < -dt) d = -dt;
        f32 a = *(f32*)(slot + 0x200) + d;
        *(f32*)(slot + 0x200) = a;
        if (a < 0.0f) a = 0.0f;
        else if (a > 1.0f) a = 1.0f;
        colour[3] = a;
        *(f32*)(slot + 0x200) = a;
        u8* o = *(u8**)(slot + 0x1FC);
        u8* m = (*(ObjGetFn*)(*(u8**)o + 0xC))(o);
        *(f32*)(m + 0x38) = colour[0];
        *(f32*)(m + 0x3C) = colour[1];
        *(f32*)(m + 0x40) = colour[2];
        *(f32*)(m + 0x44) = colour[3];
    }
}
