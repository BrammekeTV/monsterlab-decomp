// Assorted small functions from 0x8002BFDC - 0x80138864 (object helpers, path link-up, aim direction).
#include <math.h>
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void   ChoiceInit(u8* c);                                         // 0x8020F36C
void   EulerToQuat(Quat* out, const Vec3* euler);                 // 0x80145B00
u8*    GetGlobalSettings(void);                                   // 0x801D2BF8
int    SysSprintf(char* buf, const char* fmt, ...);
void*  fn_8003DDE8(void* part, s32 slot, s32 def, s32 flag, s32 a);   // 0x8003DDE8
void*  fn_8004FFB4(void* id, void* part);                         // 0x8004FFB4
void   fn_800D42F4(u8* db, u32 hash, u32 id);                     // 0x800D42F4
void   fn_800D4DE8(u8* dst, u8* src);                                    // 0x800D4DE8
void   fn_800D5110(u8* holder, u8* obj, u8* data);                // 0x800D5110
void   fn_800D8100(u8* world);                                    // 0x800D8100
s32    fn_801B5510(u8* a, u8* b, s32 c, f32* v, s32 d);           // 0x801B5510
const char* fn_801EB708(u32 hash);                                // 0x801EB708 (hash -> name for logs)
u32    fn_801EC880(u8* p, u32 hash);                              // 0x801EC880
u8*    fn_801F06FC(void);                                         // 0x801F06FC (returns the world params block)
f64    fn_8033F1F4(f64 x);                                        // 0x8033F1F4 (acos)
void   operator_delete(void* p);                                  // 0x801D426C
extern u8  gGame[];                                               // 0x803B53A0
extern u8  gWorldState[];                                         // 0x803E6D00
extern f32 lbl_805FCB80;                                          // 0.0f
extern f32 lbl_805FDB28;                                          // 0.0f
extern f32 lbl_805FDB38;                                          // -1.0f
extern f32 lbl_805FDB40;                                          // 0.25f
extern f32 lbl_805FDB64;                                          // 1.0f
extern f32 lbl_805FDB84;                                          // 2/pi
extern f32 lbl_805FDB94;                                          // 0.75f
extern f32 lbl_805FDB98;                                          // 1.25f
extern f32 lbl_805FDB9C;                                          // 1.75f
extern f32 lbl_805FE510;                                          // 0.0f
extern f32 lbl_805FE514;                                          // 1.0f
extern f32 lbl_805FE518;                                          // 2/pi
extern "C++" void HudReload(u8* obj);                              // 0x80006B60
extern "C++" s32 gZoneResourceSize;                               // 0x805FB94C
extern "C++" u8  lbl_8047C730[];                                  // 0x8047C730 (combat results state)
}

// Bounds-checked array element: an index outside [0, count] falls back to the first element.
static inline u8* ClampedAt(u8* base, s32 count, s32 i, s32 stride) {
    s32 c = i < 0 ? 0 : (i > count ? count : i);
    return i == c ? base + i * stride : base;
}

extern "C" {

// 0x8002BFDC
// Zeroes the fields at +4..+0x30 (two Vec3 and five more floats); returns the object.
u8* fn_8002BFDC(u8* p) {
    f32 z = lbl_805FCB80;
    *(f32*)(p + 0x4)  = z;
    *(f32*)(p + 0x8)  = z;
    *(f32*)(p + 0xC)  = z;
    *(f32*)(p + 0x10) = z;
    *(f32*)(p + 0x14) = z;
    *(f32*)(p + 0x18) = z;
    *(f32*)(p + 0x1C) = z;
    *(f32*)(p + 0x20) = z;
    *(f32*)(p + 0x24) = z;
    *(f32*)(p + 0x28) = z;
    *(f32*)(p + 0x2C) = z;
    *(f32*)(p + 0x30) = z;
    return p;
}

// 0x8002C040
// Deleting destructor: frees the object when del > 0.
u8* fn_8002C040(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
    return p;
}

// 0x8003DA7C
// Walks the world param list and bumps each entry's value at +8 to (id + 1) when the id at +0xC
// lies within the global settings' summed count ranges.
void fn_8003DA7C(void) {
    s32 i = 0;
    s32 off = 0;
    u8* list = *(u8**)(fn_801F06FC() + 0x8);
    while (i < *(s32*)(list + 0x48) && *(s32*)(*(u8**)(list + 0x44) + off) != 0) {
        u32 id = *(u32*)(*(u8**)(*(u8**)(list + 0x44) + off) + 0xC) & 0xFFFF;
        if (*(s32*)(GetGlobalSettings() + 0x68) + 1 <= (s32)id) {
            s32 a = *(s32*)(GetGlobalSettings() + 0x68);
            s32 b = *(s32*)(GetGlobalSettings() + 0x7C);
            s32 c = *(s32*)(GetGlobalSettings() + 0x6C);
            s32 d = *(s32*)(GetGlobalSettings() + 0x70);
            s32 e = *(s32*)(GetGlobalSettings() + 0x74);
            s32 f = *(s32*)(GetGlobalSettings() + 0x78);
            if ((s32)id <= f + (e + (d + (c + (b + a))))) {
                *(s32*)(*(u8**)(*(u8**)(list + 0x44) + off) + 0x8) = id + 1;
            }
        }
        off += 4;
        i++;
    }
}

// 0x8003DFCC
// Builds the part for the given args and registers it (forwarding the caller's own arguments).
void* fn_8003DFCC(u8* part, s32 slot, s32 def, s32 flag, s32 a) {
    return fn_8004FFB4(fn_8003DDE8(part, slot, def, flag, a), part);
}

// 0x8003FB24
// Constructor: clears the state fields, inits the choice at +0x8C, and points ten link slots
// (every 0xC bytes from +0) back at the object.
u8* fn_8003FB24(u8* p) {
    *(s32*)(p + 0x84) = 0;
    *(u8*)(p + 0x88) = 0;
    ChoiceInit(p + 0x8C);
    *(u8*)(p + 0x98) = 0;
    *(u8**)(p + 0x0)  = p;
    *(u8**)(p + 0xC)  = p;
    *(u8**)(p + 0x18) = p;
    *(u8**)(p + 0x24) = p;
    *(u8**)(p + 0x30) = p;
    *(u8**)(p + 0x3C) = p;
    *(u8**)(p + 0x48) = p;
    *(u8**)(p + 0x54) = p;
    *(u8**)(p + 0x60) = p;
    *(u8**)(p + 0x6C) = p;
    return p;
}

// 0x80097064
// Classifies the xz direction of a vector (y cleared) against +z into one of 8 octants: the angle
// to +z in units of pi/2 selects 0..4, mirrored (7..5) when x >= 0.
s32 fn_80097064(f32* v) {
    f32 refLen = 0.0f;
    f32 vLen = 0.0f;
    f32 deg = lbl_805FDB28;
    f32 zero = lbl_805FDB28;
    f32 one = lbl_805FDB64;

    f32 rz = zero * zero;
    f32 rLen2 = one * one + (rz + rz);
    f32 dotRef = one * v[2] + (zero * v[0] + rz);
    v[1] = zero;
    if (!(rLen2 <= zero)) refLen = sqrtf(rLen2);  // original: frsqrte with two Newton steps
    f32 vLen2 = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
    if (!(vLen2 <= lbl_805FDB28)) vLen = sqrtf(vLen2);
    f32 prod = refLen * vLen;
    if (!(prod <= lbl_805FDB28)) {
        f32 c = dotRef / prod;
        f32 lo = lbl_805FDB38;
        f32 r = lo;
        if (!(c < lo)) {
            r = lbl_805FDB64;
            if (!(c > lbl_805FDB64)) r = c;
        }
        f32 c2 = r;
        r = lbl_805FDB38;
        if (!(c2 < lbl_805FDB38)) {
            r = lbl_805FDB64;
            if (!(c2 > lbl_805FDB64)) r = c2;
        }
        deg = lbl_805FDB84 * (f32)fn_8033F1F4((f64)r);
    }
    if (v[0] < lbl_805FDB28) {
        if (deg < lbl_805FDB40) return 0;
        if (deg < lbl_805FDB94) return 1;
        if (deg < lbl_805FDB98) return 2;
        if (deg < lbl_805FDB9C) return 3;
        return 4;
    }
    if (deg < lbl_805FDB40) return 0;
    if (deg < lbl_805FDB94) return 7;
    if (deg < lbl_805FDB98) return 6;
    if (deg < lbl_805FDB9C) return 5;
    return 4;
}

// 0x800CD020
// Finds the zone object with id 0xF8772B70 (stapling_reticleA), makes it the active one at
// arg0+0x274, tags it, rebuilds its transform, looks up its two node hashes and links it back.
void fn_800CD020(u8* arg0) {
    f32 zero = lbl_805FE510;
    f32 one = lbl_805FE514;
    f32 k = lbl_805FE518;
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + (gZoneResourceSize * 2 - gZoneResourceSize);
    s32 i = 0;
    while (i < *(s32*)(*(u8**)gWorldState + 0x4)) {
        s32 j = 0;
        s32 off = 0;
        while (j < *(s32*)(zone + 0x98)) {
            u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + off);
            u8* info = *(u8**)(obj + 0x9C);
            s32 id = info != NULL ? *(s32*)(info + 0x8) : 0;
            if (id == (s32)0xF8772B70u) {  // "stapling_reticleA"
                *(u8**)(arg0 + 0x274) = obj;
                *(u32*)(obj + 0xF0) = 0xD9D1E8B2u;
                *(u32*)(obj + 0xF4) = 0x757B006Fu;
                u8* o = *(u8**)(arg0 + 0x274);
                *(u32*)(o + 0x15C) |= 0x400000;
                u8* o2 = *(u8**)(arg0 + 0x274);
                u8* xf = *(u8**)(o2 + 0x4);
                *(f32*)(xf + 0x0) = *(f32*)(o2 + 0xC8);
                *(f32*)(xf + 0x4) = *(f32*)(o2 + 0xCC);
                *(f32*)(xf + 0x8) = *(f32*)(o2 + 0xD0);
                *(u32*)(*(u8**)(o2 + 0x4) + 0x20) |= 0x8000;
                *(u32*)(*(u8**)(o2 + 0x4) + 0x20) |= 0x10000000;
                *(u32*)(*(u8**)(o2 + 0x4) + 0x20) |= 0x4000;
                *(u32*)(*(u8**)(o2 + 0x4) + 0x20) |= 0x10000000;
                f32 q[4];
                f32 e[3];
                q[0] = zero;
                q[1] = zero;
                q[2] = zero;
                q[3] = one;
                u8* o3 = *(u8**)(arg0 + 0x274);
                e[0] = *(f32*)(o3 + 0xD4) * k;
                e[1] = *(f32*)(o3 + 0xD8) * k;
                e[2] = *(f32*)(o3 + 0xDC) * k;
                EulerToQuat((Quat*)q, (const Vec3*)e);
                u8* o4 = *(u8**)(arg0 + 0x274);
                u8* xf2 = *(u8**)(o4 + 0x4);
                *(f32*)(xf2 + 0xC) = q[0];
                *(f32*)(xf2 + 0x10) = q[1];
                *(f32*)(xf2 + 0x14) = q[2];
                *(f32*)(xf2 + 0x18) = q[3];
                *(u32*)(*(u8**)(o4 + 0x4) + 0x20) |= 0x8000;
                *(u32*)(*(u8**)(o4 + 0x4) + 0x20) |= 0x10000000;
                *(u32*)(*(u8**)(o4 + 0x4) + 0x20) |= 0x4000;
                *(u32*)(*(u8**)(o4 + 0x4) + 0x20) |= 0x10000000;
            }
            j++;
            off += 4;
        }
        i++;
    }
    u8* obj = *(u8**)(arg0 + 0x274);
    *(u32*)(obj + 0x264) = *(u32*)(arg0 + 0x7C4);
    *(s32*)(obj + 0x270) = -1;
    *(s32*)(obj + 0x274) = -1;
    *(u32*)(obj + 0x270) = fn_801EC880(obj + 0x94, 0x5B6F7FB7u);
    *(u32*)(obj + 0x274) = fn_801EC880(obj + 0x94, 0x985CF4A4u);
    if (*(u8*)(obj + 0x27D) == 0) {
        *(f32*)(obj + 0x280) = *(f32*)(obj + 0x8);
        *(f32*)(obj + 0x284) = *(f32*)(obj + 0xC);
        *(f32*)(obj + 0x288) = *(f32*)(obj + 0x10);
        *(u8*)(obj + 0x27D) = 1;
    }
    f32 s = lbl_805FE514;
    f32 sx = *(f32*)(obj + 0x280) * s;
    f32 sy = *(f32*)(obj + 0x284) * s;
    f32 sz = *(f32*)(obj + 0x288) * s;
    *(f32*)(obj + 0x8) = sx;
    *(f32*)(obj + 0xC) = sy;
    *(f32*)(obj + 0x10) = sz;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
    u8* active = *(u8**)(arg0 + 0x274);
    if (arg0 != NULL) {
        *(u8**)(active + 0x26C) = arg0;
        f32 z = lbl_805FE510;
        u8* node = *(u8**)(active + 0x98) + *(s32*)(active + 0x274) * 0xC8;
        f32 v[3];
        v[0] = *(f32*)(node + 0x2C) + (*(f32*)(node + 0x20) * z + (*(f32*)(node + 0x8) * z + *(f32*)(node + 0x14) * z));
        v[1] = *(f32*)(node + 0x30) + (*(f32*)(node + 0x24) * z + (*(f32*)(node + 0xC) * z + *(f32*)(node + 0x18) * z));
        v[2] = *(f32*)(node + 0x34) + (*(f32*)(node + 0x28) * z + (*(f32*)(node + 0x10) * z + *(f32*)(node + 0x1C) * z));
        if (fn_801B5510(active, arg0, *(s32*)(node + 0x4), v, 1) != 0) {
            *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) &= 0xFFFFFFFEu;
        }
    }
}

// 0x800D3D6C
// Links the path records (0x1AC bytes each at +0x13284, four slots per record) to the nodes
// (0xDC bytes each at +0x575C) whose hash matches, then warns about nodes left without both links.
void fn_800D3D6C(u8* db) {
    char buf[0x100];
    u8* aBase = db + 0x13284;
    u8* bBase = db + 0x575C;

    for (s32 a = *(s32*)(db + 0x2DCD8) - 1; a > -1; a--) {
        for (s32 k = 0, off = 0; k < 4; k++, off += 4) {
            s32 nA = *(s32*)(db + 0x2DCD8);
            u8* rec = ClampedAt(aBase, nA, a, 0x1AC);
            if (*(u32*)(rec + off + 0x14) == 0) {
                *(u32*)(ClampedAt(aBase, nA, a, 0x1AC) + off + 0x4) = 0;
                continue;
            }
            for (s32 b = *(s32*)(db + 0x13280) - 1; b > -1; b--) {
                s32 nB = *(s32*)(db + 0x13280);
                if (*(u32*)(ClampedAt(aBase, nA, a, 0x1AC) + off + 0x14) !=
                    *(u32*)ClampedAt(bBase, nB, b, 0xDC)) {
                    continue;
                }
                *(u8**)(ClampedAt(aBase, nA, a, 0x1AC) + off + 0x4) = ClampedAt(bBase, nB, b, 0xDC);
                u8* node = bBase + b * 0xDC;
                for (s32 j = 0, joff = 0; j < 2; j++, joff += 4) {
                    nB = *(s32*)(db + 0x13280);
                    u8* sel = ClampedAt(bBase, nB, b, 0xDC);
                    if (*(u32*)(sel + joff + 0xC) == *(u32*)ClampedAt(aBase, nA, a, 0x1AC)) {
                        if (b != (b < 0 ? 0 : (b > nB ? nB : b))) node = bBase;
                        *(u8**)(node + j * 4 + 0x4) = ClampedAt(aBase, nA, a, 0x1AC);
                        break;
                    }
                }
                break;
            }
        }
    }
    for (s32 b = *(s32*)(db + 0x13280) - 1; b > -1; b--) {
        s32 nB = *(s32*)(db + 0x13280);
        if (*(s32*)(ClampedAt(bBase, nB, b, 0xDC) + 0x4) == 0 ||
            *(s32*)(ClampedAt(bBase, nB, b, 0xDC) + 0x8) == 0) {
            SysSprintf(buf, "Path %s not fully connected\n",
                       fn_801EB708(*(u32*)ClampedAt(bBase, nB, b, 0xDC)));
        }
    }
    fn_800D5110(db, db + 0x404, db + 0x13284);
}

// 0x800D424C
// Appends a fresh path record (up to 0xFF) and then registers each hash in arg1's list (+0x68, count
// at +0x90) against the id at arg1+0.
void fn_800D424C(u8* db, u8* arg1) {
    s32 n = *(s32*)(db + 0x2DCD8);
    if (n < 0xFF) {
        fn_800D4DE8(db + 0x13284 + n * 0x1AC, arg1);
        *(s32*)(db + 0x2DCD8) = *(s32*)(db + 0x2DCD8) + 1;
    }
    u8* p = arg1;
    for (s32 i = 0; i < *(s32*)(arg1 + 0x90); i++) {
        fn_800D42F4(db, *(u32*)(p + 0x68), *(u32*)(arg1 + 0x0));
        p += 4;
    }
}

// 0x8012FFA4
// Reloads the HUD and the world, then copies the combat-results setup fields out of the world state
// into lbl_8047C730 and points each result entry (0x1F4 bytes) back at it.
void fn_8012FFA4(void) {
    u8* ws = *(u8**)gWorldState;
    HudReload(ws + 0x1E0);
    fn_800D8100(gGame + 0xD5F0);
    ws = *(u8**)gWorldState;
    *(s32*)(lbl_8047C730 + 0x10) = *(s32*)(ws + 0x80);
    *(s32*)(lbl_8047C730 + 0xC) = *(s32*)(ws + 0x7C);
    *(u8*)(lbl_8047C730 + 0x14) = *(u8*)(ws + 0x84);
    u8* entries = *(u8**)(ws + 0x8C);
    *(u8**)(lbl_8047C730 + 0x4) = entries;
    *(s32*)(lbl_8047C730 + 0x0) = *(s32*)(ws + 0x88);
    *(u8*)(lbl_8047C730 + 0x8) = *(u8*)(ws + 0x90);
    if (*(s32*)(ws + 0x80) != 0 && entries != NULL) {
        s32 i = 0;
        s32 off = 0;
        while (i < *(s32*)(lbl_8047C730 + 0x0)) {
            u8* e = *(u8**)(lbl_8047C730 + 0x4) + off;
            if (e != NULL) *(u8**)(e + 0x1DC) = lbl_8047C730;
            i++;
            off += 0x1F4;
        }
    }
}

// 0x80138864
// Returns element idx (0x30 bytes each, array at +0xB0) when idx is below the sum of the global
// settings' thirteen count fields, else NULL.
u8* fn_80138864(u8* obj, s32 idx) {
    s32 ok = 0;
    if (idx >= 0) {
        s32 a = *(s32*)(GetGlobalSettings() + 0x4C);
        s32 b = *(s32*)(GetGlobalSettings() + 0x34);
        s32 c = *(s32*)(GetGlobalSettings() + 0x50);
        s32 d = *(s32*)(GetGlobalSettings() + 0x54);
        s32 e = *(s32*)(GetGlobalSettings() + 0x58);
        s32 f = *(s32*)(GetGlobalSettings() + 0x5C);
        s32 g = *(s32*)(GetGlobalSettings() + 0x60);
        s32 h = *(s32*)(GetGlobalSettings() + 0x64);
        s32 i = *(s32*)(GetGlobalSettings() + 0x38);
        s32 j = *(s32*)(GetGlobalSettings() + 0x3C);
        s32 k = *(s32*)(GetGlobalSettings() + 0x40);
        s32 l = *(s32*)(GetGlobalSettings() + 0x44);
        s32 m = *(s32*)(GetGlobalSettings() + 0x48);
        if (idx < m + (l + (k + (j + (i + (h + (g + (f + (e + (d + (c + (b + a))))))))))))
            ok = 1;
    }
    if (ok != 0) return *(u8**)(obj + 0xB0) + idx * 0x30;
    return NULL;
}

}  // extern "C"
