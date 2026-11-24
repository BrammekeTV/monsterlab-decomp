// Misc decompiled functions: ray-test spawn positions, grid placement, world size, alert lookup,
// zone teardown, vector/matrix orthogonalisation helpers.
#include <math.h>

#include "types.h"
#include "engine/math.h"

extern "C" {
u32   AlertGetLastError(void);                                  // 0x801D3F3C
s32   AnimSetFindClip(void* set, s32 anim);                     // 0x801AC2FC
void* EventPost(s32 id, void* sender);                          // 0x801F3918
u8*   GetGlobalSettings(void);                                  // 0x801D2BF8
void  PropPlayAnim(void* prop, int a, int b, BOOL once);        // 0x80069350
s32   RaySceneTest(void* ray);                                  // 0x80070458
void  fn_8003DA7C(void);                                        // 0x8003DA7C
void  fn_800CD020(u8* obj);                                     // 0x800CD020
void  fn_800D3D50(void* db);                                    // 0x800D3D50 (clears three counters of the object database)
void  PathCheckFullyConnected(void* db);                                    // 0x800D3D6C
void  fn_800D41F4(void* db, u8* p);                             // 0x800D41F4
void  fn_800D424C(void* db, u8* p);                             // 0x800D424C
u8*   fn_800D8724(u8* a, u32 key);                              // 0x800D8724
void  fn_801070B0(s32 a);                                       // 0x801070B0
void  fn_8012F8B8(u8* p, s32 a1, s32 a2);                       // 0x8012F8B8
void  fn_8012FD0C(u8* self, u8* zones);                         // 0x8012FD0C
void  fn_8012FFA4(void);                                        // 0x8012FFA4
void  fn_80140B30(void);                                        // 0x80140B30
s32   fn_80156A80(void);                                        // 0x80156A80
s32   fn_80156A88(void);                                        // 0x80156A88
u8*   fn_801B161C(u8* p, u32 key);                              // 0x801B161C
u8*   fn_801B1724(u8* p, u32 key);                              // 0x801B1724
u8*   fn_801B3308(void);                                        // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                                // 0x801B3310
void  fn_801B6F34(u8* p);                                       // 0x801B6F34
void  fn_801DA81C(u8* p);                                       // 0x801DA81C
void  fn_801DB160(u8* p);                                       // 0x801DB160
void  fn_801E7E28(void* p);                                     // 0x801E7E28
s32   fn_801EC880(u8* p, u32 hash);                             // 0x801EC880
void* fn_801F06FC(void);                                        // 0x801F06FC
void  fn_801F5724(u32 button);                                  // 0x801F5724
void  fn_801F5814(s32 a);                                       // 0x801F5814
void  fn_801F8628(u8* obj, s8* src, s32 n);                     // 0x801F8628
void  fn_801F8F34(u8* p, f32 v);                                // 0x801F8F34
void  fn_801F8FA0(u8* p, f32 v);                                // 0x801F8FA0
void  OSLockMutex(u8* p);                                       // 0x8023A5B0
void  OSUnlockMutex(u8* p);                                       // 0x8023A68C
void  KPADSetPosParam(s32 a, f32 b, f32 c);                         // 0x8027D270
void  fn_802E9CBC(s32 counter);                                 // 0x802E9CBC

extern u8    gGame[];             // 0x803B53A0
extern void* gLab;                // 0x805FBBC8
extern void* gObjDb;              // 0x805FB180
extern u8    gWorldState[];       // 0x803E6D00 (first word: pointer to the world block)
extern u8    lbl_80556AB0[];      // 0x80556AB0
extern u8    lbl_80556E80[];      // 0x80556E80 (table of 64-bit ids, stride 0x20 at +0x350)
extern s32   lbl_805FB388;        // 0x805FB388
extern s32   lbl_805FB38C;        // 0x805FB38C
extern s32   lbl_805FBB48;        // 0x805FBB48
extern u8*   lbl_805FBBE0;        // 0x805FBBE0 (save block pointer)

// Per-ray-result tables of f32 (indexed by the RaySceneTest result).
extern f32 lbl_80346D70[];        // 0x80346D70
extern f32 lbl_80346D80[];        // 0x80346D80
extern f32 lbl_80346D90[];        // 0x80346D90
extern f32 lbl_80346DA0[];        // 0x80346DA0
extern f32 lbl_80346DB0[];        // 0x80346DB0
extern f32 lbl_80346DC0[];        // 0x80346DC0
extern f32 lbl_80346DD0[];        // 0x80346DD0
extern f32 lbl_80346DE0[];        // 0x80346DE0
extern f32 lbl_80346DF0[];        // 0x80346DF0
extern f32 lbl_80346E00[];        // 0x80346E00
extern f32 lbl_80346E10[];        // 0x80346E10
extern f32 lbl_80346E20[];        // 0x80346E20
extern f32 lbl_80346E40[];        // 0x80346E40
extern f32 lbl_80346E50[];        // 0x80346E50
extern f32 lbl_80346E60[];        // 0x80346E60
}

extern "C++" u32 fn_80141C38(void);    // 0x80141C38 (C++ linkage, small_8014.cpp)
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C

// Marks the object's body (+4) as moved: sets the four dirty flags at +0x20.
static inline void MarkMoved(u8* obj) {
    u8* body = *(u8**)(obj + 4);
    *(u32*)(body + 0x20) |= 0x8000;
    body = *(u8**)(obj + 4);
    *(u32*)(body + 0x20) |= 0x10000000;
    body = *(u8**)(obj + 4);
    *(u32*)(body + 0x20) |= 0x4000;
    body = *(u8**)(obj + 4);
    *(u32*)(body + 0x20) |= 0x10000000;
}

static inline s32 GS(u32 off) {
    return *(s32*)(GetGlobalSettings() + off);
}

// Number of zones: 13 per-category counters plus zone 0.
static inline s32 ZoneCountAll(void) {
    return GS(0x80) + GS(0x68) + GS(0x84) + GS(0x88) + GS(0x8C) + GS(0x90) + GS(0x94) + GS(0x98) +
           GS(0x6C) + GS(0x70) + GS(0x74) + GS(0x78) + GS(0x7C) + 1;
}

// Appends v to the 8-byte-stride slot list at ent+base (count stored at ent+cntOff, max 8).
static inline void PushSlot(u8* ent, u32 base, u32 cntOff, u8* v) {
    s32 n = *(s32*)(ent + cntOff);
    if (n < 8) {
        *(u8**)(ent + base + n * 8) = v;
        *(s32*)(ent + cntOff) = *(s32*)(ent + cntOff) + 1;
    }
}

extern "C" {

// 0x800BCA44
// Parks the object far below the world, ray-tests the scene and loads five per-hit-type floats
// (+0x26C..+0x27C) from the table set selected by mode (1..3).
s32 fn_800BCA44(u8* obj, s32 mode) {
    *(s32*)(obj + 0x264) = mode;
    u8* pos = *(u8**)(obj + 4);
    *(u8*)(obj + 0x298) = 0;
    *(f32*)(pos + 0x0) = 0.0f;
    *(f32*)(pos + 0x4) = -100.0f;
    *(f32*)(pos + 0x8) = 0.0f;
    MarkMoved(obj);
    *(f32*)(obj + 0x268) = 0.0f;
    s32 hit = RaySceneTest(obj);
    s32 m = *(s32*)(obj + 0x264);
    if (m == 1) {
        *(f32*)(obj + 0x26C) = lbl_80346D70[hit];
        *(f32*)(obj + 0x270) = lbl_80346DA0[hit];
        *(f32*)(obj + 0x274) = lbl_80346DD0[hit];
        *(f32*)(obj + 0x278) = lbl_80346E00[hit];
        *(f32*)(obj + 0x27C) = lbl_80346E40[hit];
    } else if (m == 2) {
        *(f32*)(obj + 0x26C) = lbl_80346D90[hit];
        *(f32*)(obj + 0x270) = lbl_80346DC0[hit];
        *(f32*)(obj + 0x274) = lbl_80346DF0[hit];
        *(f32*)(obj + 0x278) = lbl_80346E20[hit];
        *(f32*)(obj + 0x27C) = lbl_80346E60[hit];
    } else if (m == 3) {
        *(f32*)(obj + 0x26C) = lbl_80346D80[hit];
        *(f32*)(obj + 0x270) = lbl_80346DB0[hit];
        *(f32*)(obj + 0x274) = lbl_80346DE0[hit];
        *(f32*)(obj + 0x278) = lbl_80346E10[hit];
        *(f32*)(obj + 0x27C) = lbl_80346E50[hit];
    }
    fn_801B6F34(obj);
    return 1;
}

// 0x800BFBB0
// Registers obj in slot idx of the list (obj pointers at +4+idx*4): places its body 100 units
// below it, clears the entry's slot lists (0x48 bytes per entry) and refills them with the
// child nodes looked up by hash under obj+0x8C (which hashes depend on idx 0..3).
void fn_800BFBB0(u8* list, u8* obj, s32 idx) {
    *(u8**)(list + idx * 4 + 4) = obj;
    if (obj != NULL) {
        u8* o = *(u8**)(list + idx * 4 + 4);
        f32 y = *(f32*)(o + 0xCC);
        f32 x = *(f32*)(o + 0xC8);
        f32 z = *(f32*)(o + 0xD0);
        f32 ym = y - 100.0f;
        u8* pos = *(u8**)(o + 4);
        *(f32*)(pos + 0x0) = x;
        *(f32*)(pos + 0x4) = ym;
        *(f32*)(pos + 0x8) = z;
        MarkMoved(o);
    }
    s32 off = idx * 0x48;
    u8* ent = list + off;
    *(s32*)(ent + 0x14) = 0;
    *(s32*)(ent + 0x1C) = 0;
    *(s32*)(ent + 0x24) = 0;
    *(s32*)(ent + 0x2C) = 0;
    *(s32*)(ent + 0x34) = 0;
    *(s32*)(ent + 0x3C) = 0;
    *(s32*)(ent + 0x44) = 0;
    *(s32*)(ent + 0x4C) = 0;
    *(s32*)(ent + 0x54) = 0;
    *(s32*)(ent + 0x18) = 0;
    *(s32*)(ent + 0x20) = 0;
    *(s32*)(ent + 0x28) = 0;
    *(s32*)(ent + 0x30) = 0;
    *(s32*)(ent + 0x38) = 0;
    *(s32*)(ent + 0x40) = 0;
    *(s32*)(ent + 0x48) = 0;
    *(s32*)(ent + 0x50) = 0;
    *(s32*)(ent + 0x58) = 0;
    u8* src = *(u8**)(obj + 0x8C);
    if (src == NULL) return;
    u8* node = fn_801B1724(src, 0xF62113D5u);  // "BAse"
    if (node == NULL) node = fn_801B1724(src, 0x4C7ED036u);
    if (node == NULL) return;
    if (idx == 1) {
        u8* a = fn_801B161C(node, 0x8C9E3783u);
        PushSlot(ent, 0x14, 0x54, a);
        u8* b = fn_801B161C(node, 0x15976639u);
        PushSlot(ent, 0x14, 0x54, b);
    } else if (idx == 2) {
        u8* a = fn_801B161C(node, 0x5BC47CC3u);
        PushSlot(ent, 0x14, 0x54, a);
        u8* b = fn_801B161C(node, 0xC2CD2D79u);
        PushSlot(ent, 0x14, 0x54, b);
        u8* c = fn_801B161C(node, 0x325CED02u);
        PushSlot(ent, 0x18, 0x58, c);
        u8* d = fn_801B161C(node, 0xAC3878A1u);
        PushSlot(ent, 0x18, 0x58, d);
    } else if (idx == 3) {
        u8* a = fn_801B161C(node, 0x93A3A7D2u);
        PushSlot(ent, 0x14, 0x54, a);
        u8* b = fn_801B161C(node, 0x0AAAF668u);
        PushSlot(ent, 0x14, 0x54, b);
    } else if (idx == 0) {
        u8* a = fn_801B161C(node, 0xECBBA032u);
        PushSlot(ent, 0x14, 0x54, a);
        u8* b = fn_801B161C(node, 0x75B2F188u);
        PushSlot(ent, 0x14, 0x54, b);
    }
}

// 0x800CC854
// Sets up the object at its saved start position (captured on first call at +0x268..), plays the
// 0x73CDBFAC clip if the prop has an anim set, configures the sub-object at +0x334 and records the
// half-screen centre at +0x7B8/+0x7BC. Returns the raw result of fn_80156A80.
s32 fn_800CC854(u8* obj, u8* arg1) {
    *(u8**)(obj + 0x7C4) = arg1;
    if (*(u8*)(obj + 0x264) == 0) {
        *(f32*)(obj + 0x268) = *(f32*)(obj + 0x8);
        *(f32*)(obj + 0x26C) = *(f32*)(obj + 0xC);
        *(f32*)(obj + 0x270) = *(f32*)(obj + 0x10);
        *(u8*)(obj + 0x264) = 1;
    }
    f32 x = *(f32*)(obj + 0x268) * 1.0f;
    f32 y = *(f32*)(obj + 0x26C) * 1.0f;
    f32 z = *(f32*)(obj + 0x270) * 1.0f;
    *(f32*)(obj + 0x8) = x;
    *(f32*)(obj + 0xC) = y;
    *(f32*)(obj + 0x10) = z;
    MarkMoved(obj);
    fn_800CD020(obj);
    u8* set = *(u8**)(obj + 0x9C);
    if (set != NULL) {
        u8* anims = *(u8**)(set + 0x48);
        if (anims != NULL) {
            PropPlayAnim(obj, AnimSetFindClip(anims, 0x73CDBFAC), 0, 1);
            u8* r = (*(u8*(**)(u8*))(*(u8**)obj + 0x10))(obj);
            *(s32*)(*(u8**)((*(u8***)r)[0] + 0x14) + 0x20) = 1;
        }
    }
    fn_801F8F34(obj + 0x334, 0.3f);
    fn_801F8FA0(obj + 0x334, 0.4f);
    s8 mask[3];
    mask[0] = 0;
    mask[1] = -1;
    mask[2] = 0;
    fn_801F8628(obj + 0x334, mask, 1);
    *(s32*)(obj + 0x3CC) = 1;
    fn_801B3310(fn_801B3308(), obj + 0x278);
    fn_801B3310(fn_801B3308(), obj + 0x334);
    KPADSetPosParam(0, 0.0f, 1.0f);
    s32 w = fn_80156A88();
    f32 cx = (f32)(w / 2);
    s32 h = fn_80156A80();
    f32 cy = (f32)(h / 2);
    *(f32*)(obj + 0x7BC) = cx;
    *(f32*)(obj + 0x7C0) = 0.0f;
    *(s32*)(obj + 0x7C8) = 0;
    *(f32*)(obj + 0x7B8) = cy;
    *(f32*)(obj + 0x7D0) = 0.0f;
    return h;
}

// 0x800CD9F0
// Lays out a 3x3 grid of child objects (obj+4..) around the position at pos: spacing comes from
// the first child's +0x264/+0x26C, which are filled for every child from the distance to its
// 0xC8026704 marker (x2).
void fn_800CD9F0(u8* obj, f32* pos) {
    s32 i;
    for (i = 0; i < 9; i++) {
        u8* c = *(u8**)(obj + 4 + i * 4);
        *(f32*)(c + 0x264) = 0.0f;
        *(f32*)(c + 0x268) = 0.0f;
        *(f32*)(c + 0x26C) = 0.0f;
        s32 idx = fn_801EC880(c + 0x94, 0xC8026704u);
        if (idx > -1) {
            u8* base = *(u8**)(c + 4);
            u8* rec = *(u8**)(c + 0x98) + idx * 0xC8;
            f32 dx = *(f32*)(rec + 0x5C) - *(f32*)(base + 0x0);
            f32 dz = *(f32*)(rec + 0x64) - *(f32*)(base + 0x8);
            *(f32*)(c + 0x264) = dx * 2.0f;
            *(f32*)(c + 0x26C) = dz * 2.0f;
            *(f32*)(c + 0x268) = 0.0f;
        }
    }
    u8* first = *(u8**)(obj + 4);
    f32 stepX = *(f32*)(first + 0x264);
    f32 stepZ = *(f32*)(first + 0x26C);
    s32 row;
    for (row = 0; row < 3; row++) {
        s32 col;
        for (col = 0; col < 3; col++) {
            u8* c = *(u8**)(obj + 4 + (row + col * 3) * 4);
            u8* p = *(u8**)(c + 4);
            *(f32*)(p + 0x0) = pos[0] + (f32)(row - 1) * stepX;
            *(f32*)(p + 0x4) = pos[1] + 0.0f;
            *(f32*)(p + 0x8) = pos[2] + (f32)(col - 1) * stepZ;
            MarkMoved(c);
            *(u8**)(obj + 0x28 + row * 0xC + col * 4) = c;
        }
    }
}

// 0x800D8604
// Updates the zone-transition record: previous/current zone ids at +0x2C/+0x28, resolved entry
// at +0x24 (via fn_800D8724) and the transition hash at +0x34 for the special 6->1, 6->2 and 3->1
// zone changes.
void fn_800D8604(u8* self) {
    u8* w = *(u8**)gWorldState;
    s32 prev = *(s32*)(self + 0x28);
    s32 zone = *(s32*)(w + 0x1E0);
    *(u8**)(self + 0x24) = *(u8**)(self + zone * 4 + 4);
    *(s32*)(self + 0x2C) = prev;
    *(s32*)(self + 0x28) = zone;
    w = *(u8**)gWorldState;
    u8* entry = fn_800D8724(self, *(u32*)(w + 0x1E8));
    if (entry != NULL) {
        *(u8**)(self + 0x24) = entry;
        w = *(u8**)gWorldState;
        *(u8**)(self + *(s32*)(w + 0x1E0) * 4 + 4) = entry;
    }
    s32 cur = *(s32*)(self + 0x28);
    if (cur == 1 && *(s32*)(self + 0x2C) == 6) {
        *(u32*)(self + 0x34) = 0xD5030FA4u;
    } else if (cur == 2 && *(s32*)(self + 0x2C) == 6 && *(s32*)(lbl_805FBBE0 + 0x138) == 2) {
        *(u32*)(self + 0x34) = 0xF700C412u;
    } else if (cur == 1 && *(s32*)(self + 0x2C) == 3) {
        if (*(u8*)((u8*)gLab + 0x6C) != 0) {
            *(u32*)(self + 0x34) = 0xACA6B62Cu;
        } else {
            *(u32*)(self + 0x34) = 0xADCA036Fu;
        }
    }
}

// 0x80124610
// Component of b perpendicular to a, rescaled to |b|: normalize(a x (b x a)) * |b|. Returns b
// unchanged when the perpendicular direction degenerates (a parallel to b).
Vec3 fn_80124610(const f32* a, const f32* b) {
    f32 ax = a[0], ay = a[1], az = a[2];
    f32 bx = b[0], by = b[1], bz = b[2];
    f32 cx = by * az - bz * ay;   // b x a
    f32 cy = bz * ax - bx * az;
    f32 cz = bx * ay - by * ax;
    f32 dx = ay * cz - az * cy;   // a x c
    f32 dy = az * cx - ax * cz;
    f32 dz = ax * cy - ay * cx;
    f32 nx, ny, nz;
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    if (len2 == 0.0f) {
        nz = 0.0f;
        ny = 0.0f;
        nx = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        nx = dx * inv;
        ny = dy * inv;
        nz = dz * inv;
    }
    f32 n2 = (nx * nx + ny * ny) + nz * nz - 0.0f;
    if (n2 > -0.0001f && n2 < 0.0001f) {
        return Vec3{ b[0], b[1], b[2] };
    }
    f32 blen2 = (b[0] * b[0] + b[1] * b[1]) + b[2] * b[2];
    f32 blen = 0.0f;
    if (!(blen2 <= 0.0f)) blen = blen2 * (1.0f / sqrtf(blen2));
    return Vec3{ nx * blen, ny * blen, nz * blen };
}

// 0x80129928
// Total bytes of the world block: per-zone records and four per-zone tables (see WorldDataCreate),
// plus the shared entity slots (original also caches their size in sEntitySlotBytes).
u32 fn_80129928(void) {
    s32 recSize = GS(0x34) * 0x54;
    u32 size = (u32)((recSize + 0x148) * ZoneCountAll()) + 0x29E0;
    size += (u32)(ZoneCountAll() * 4);
    size += (u32)(ZoneCountAll() * 4);
    size += (u32)(ZoneCountAll() * 0xC);
    size += (u32)(ZoneCountAll() * 0x18);
    u8* params = (u8*)fn_801F06FC();
    s32 slots = (*(s32*)params + *(s32*)(params + 4)) * 4;
    u32 bytes = (u32)((GS(0x68) + 1) * slots);
    // Original also stores bytes to sEntitySlotBytes (0x805FB940, static in world_alloc.cpp; same
    // value WorldDataCreate computes right after).
    return size + bytes;
}

// 0x8012A79C
// Copies a 0xA0-byte record: two flag bytes, then floats at +4..+0x78 and +0x7C..+0x9C.
void fn_8012A79C(u8* dst, u8* src) {
    s32 off;
    for (off = 0x7C; off <= 0x98; off += 4) {
        *(f32*)(dst + off) = *(f32*)(src + off);
    }
    f32 last = *(f32*)(src + 0x9C);
    *(u8*)(dst + 0x0) = *(u8*)(src + 0x0);
    *(u8*)(dst + 0x1) = *(u8*)(src + 0x1);
    for (off = 4; off <= 0x78; off += 4) {
        *(f32*)(dst + off) = *(f32*)(src + off);
    }
    *(f32*)(dst + 0x9C) = last;
}

// 0x8013006C
// Tears down the loaded world: frees the zone resources, the two owned objects (+0x1B0, +0x1BC),
// releases each zone's +0x50 resource, and (when the object DB is live and not in mode gGame+0xFE0)
// re-registers the world's two object lists with it.
void fn_8013006C(u8* self) {
    fn_8012FFA4();
    fn_8012FD0C(self, *(u8**)(*(u8**)self + 0x1A4));
    fn_801E7E28(*(void**)(*(u8**)self + 0x1C8));
    u8* obj = **(u8***)(*(u8**)self + 0x1B0);
    if (obj != NULL) {
        fn_801DA81C(obj);
        fn_801DB160(obj);
    }
    obj = **(u8***)(*(u8**)self + 0x1BC);
    if (obj != NULL) {
        fn_801DA81C(obj);
        fn_801DB160(obj);
    }
    s32 i;
    for (i = 0; i <= *(s32*)(*(u8**)self + 4); i++) {
        u8* res = *(u8**)(*(u8**)(*(u8**)self + 0x1A4) + i * gZoneResourceSize + 0x50);
        if (res != NULL) fn_8012F8B8(res, 1, 0);
    }
    fn_801070B0(0);
    if (gObjDb != NULL && *(s32*)(gGame + 0xFE0) == 0) {
        fn_800D3D50(gObjDb);
        for (i = 0; i < *(s32*)(*(u8**)self + 0x30); i++) {
            fn_800D424C(gObjDb, *(u8**)(*(u8**)(*(u8**)self + 0x2C) + i * 4));
        }
        for (i = 0; i < *(s32*)(*(u8**)self + 0x3C); i++) {
            fn_800D41F4(gObjDb, *(u8**)(*(u8**)(*(u8**)self + 0x38) + i * 4));
        }
        PathCheckFullyConnected(gObjDb);
    }
    fn_8003DA7C();
}

// 0x8013A4A0
// Handles a 64-bit session id (hi, lo): if it is already in the 64-entry id table the alert
// error state decides between a retry prompt and an error event; otherwise the id is processed
// under the busy counter.
void fn_8013A4A0(s32 hi, s32 lo) {
    BOOL found = FALSE;
    u8* e = lbl_80556E80 + 0x350;
    s32 i;
    for (i = 0; i < 64; i++, e += 0x20) {
        u32 eh = *(u32*)e;
        u32 el = *(u32*)(e + 4);
        if ((el | eh) != 0 && ((el ^ (u32)lo) | (eh ^ (u32)hi)) == 0) {
            found = TRUE;
            break;
        }
    }
    if (found) {
        if ((AlertGetLastError() + 0x0A100000u == 0x817Au ||
             AlertGetLastError() + 0x76320000u == 0x136Au) &&
            fn_80141C38() == 0) {
            if (*(s32*)(lbl_805FBBE0 + 0x10E8) == 0) {
                fn_801F5814(1);
                fn_801F5724(0x400);
                EventPost(0x4B, NULL);
                return;
            }
            fn_801F5724(0x800);
            lbl_805FB38C = lo;
            lbl_805FB388 = hi;
            return;
        }
        EventPost(0x4F, NULL);
        fn_80140B30();
        return;
    }
    OSLockMutex(lbl_80556AB0);
    s32 counter = lbl_805FBB48;
    lbl_805FBB48 = counter + 1;
    fn_802E9CBC(counter);
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 = lbl_805FBB48 - 1;
}

// 0x801443B0
// Gram-Schmidt: orthonormalises the three row vectors of the 3x3 matrix at m (rows at +0, +0xC,
// +0x18), keeping row 0's direction.
void fn_801443B0(f32* m) {
    f32 x0 = m[0], y0 = m[1], z0 = m[2];
    f32 inv0 = 1.0f / sqrtf(z0 * z0 + (x0 * x0 + y0 * y0));  // original: frsqrte, two Newton steps
    x0 *= inv0;
    y0 *= inv0;
    z0 *= inv0;
    f32 d1 = z0 * m[5] + (x0 * m[3] + y0 * m[4]);
    m[0] = x0;
    m[1] = y0;
    m[2] = z0;
    f32 x1 = m[3] - x0 * d1;
    f32 y1 = m[4] - y0 * d1;
    f32 z1 = m[5] - z0 * d1;
    m[3] = x1;
    m[4] = y1;
    m[5] = z1;
    f32 inv1 = 1.0f / sqrtf(z1 * z1 + (x1 * x1 + y1 * y1));
    f32 d2 = z0 * m[8] + (x0 * m[6] + y0 * m[7]);
    f32 x2 = m[6] - x0 * d2;
    f32 y2 = m[7] - y0 * d2;
    f32 z2 = m[8] - z0 * d2;
    m[6] = x2;
    m[7] = y2;
    m[8] = z2;
    x1 *= inv1;
    y1 *= inv1;
    z1 *= inv1;
    m[3] = x1;
    m[4] = y1;
    m[5] = z1;
    f32 d3 = z1 * z2 + (x1 * x2 + y1 * y2);
    x2 -= x1 * d3;
    y2 -= y1 * d3;
    z2 -= z1 * d3;
    m[6] = x2;
    m[7] = y2;
    m[8] = z2;
    f32 inv2 = 1.0f / sqrtf(z2 * z2 + (x2 * x2 + y2 * y2));
    m[6] = x2 * inv2;
    m[7] = y2 * inv2;
    m[8] = z2 * inv2;
}

}  // extern "C"
