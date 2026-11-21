// Mixed small/medium functions: battle character render passes (0x8003ED08, 0x8003F1B8), a
// zone-slot object constructor (0x800DB9A4), a stat-table init (0x80122220), an entity zone
// switch (0x80128348), a light-table accumulator (0x80133A84) and the zone-slot lookups
// (0x80137684, 0x80138964).
#include "types.h"
#include <math.h>

extern "C" {
typedef struct ScopedLock { u8 pad[4]; } ScopedLock;
}

extern "C" {
void   CameraSetMatrix(void* camera, const f32 mtx[3][4]);          // 0x801D6814
u8*    GetGlobalSettings(void);                                     // 0x801D2BF8
void*  MemAllocDefault(u32 size);                                   // 0x801D4650
void   ScopedLockCtor(ScopedLock* l);                               // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                    // 0x801F53B0
f32    cosf(f32 x);                                                 // 0x8033EC98 (MSL)
void   fn_80009840(u8* p);                                          // 0x80009840
void   fn_8003EA7C(u8* p);                                          // 0x8003EA7C
f32    fn_800DE2AC(u8* zone);                                       // 0x800DE2AC
void fn_8006D774(u8* p);                              // 0x8006D774
void   fn_8014DE68(void);                                           // 0x8014DE68
s32    fn_8015BA30(void* p);                                        // 0x8015BA30
void   fn_801719DC(void* p);                                        // 0x801719DC
s32    fn_801DA780(u8* a3);                                         // 0x801DA780
u8*    fn_801DA788(u8* a3, s32 i);                                  // 0x801DA788

extern u8*  gBattle;               // 0x805FBBB8
extern u8   gWorldState[];         // 0x803E6D00 (first word: world block pointer)
extern u8   lbl_80348F38[];        // 0x80348F38 (per-type dispatch table)
extern s32  lbl_803C4E60[];        // 0x803C4E60 (saved per-object values, float bits)
extern u8   lbl_803C4298[];        // 0x803C4298 (level info block)
extern u8   lbl_803E6C20[];        // 0x803E6C20 (4 records of 0x1C bytes)
extern u8   lbl_803E9FC8[];        // 0x803E9FC8 (light table: array pointers)
extern u8   lbl_8047C640[];        // 0x8047C640 (camera)
extern s32  lbl_805FB054;          // 0x805FB054 (index into lbl_803C4E60)
extern f32  lbl_805FB320;          // 0x805FB320
extern u32  lbl_805FB324;          // 0x805FB324
extern s32  lbl_805FB4BC;          // 0x805FB4BC (current viewport index)
extern s32  lbl_805FB4C0;          // 0x805FB4C0 (display-list recording flag)
extern s32  lbl_805FB4C8;          // 0x805FB4C8 (render budget)

extern u8   lbl_8035FD14[];        // 0x8035FD14
extern u8   lbl_8035FF78[];        // 0x8035FF78
extern u8   lbl_8035FEF8[];        // 0x8035FEF8
extern u8   lbl_8035FE20[];        // 0x8035FE20
extern u8   lbl_8035FEA8[];        // 0x8035FEA8
}
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C
extern "C++" u8  sRendDLs[];          // 0x80401108 (display-list records, 0x94 bytes each)

// Per-type dispatch slots (table + type*4 + off): 0x30 reads a value, 0x24 writes one.
typedef s32  (*SlotGetFn)(u8* obj, s32 id);
typedef void (*SlotSetFn)(u8* obj, s32 id, s32 val);
typedef u8*  (*GetObjFn)(u8* self);
typedef void (*GetMtxFn)(u8* cam, f32* out);
typedef void (*VSetFn)(u8* obj, s32 v);

#define SLOT_GET(type) (*(SlotGetFn*)(lbl_80348F38 + (u32)(type) * 4 + 0x30))
#define SLOT_SET(type) (*(SlotSetFn*)(lbl_80348F38 + (u32)(type) * 4 + 0x24))

static const s32 kOneBits = 0x3F800000;   // 1.0f

// First pass over an entity's attached list (0x70 + n*8): type 1 entries are handled by
// fn_8003EA7C, type 0 entries read their slot-5 value into lbl_803C4E60 and are set to 1.0.
static void SaveAndSetAttached(u8* info) {
    u32 count = *(u32*)(info + 0x6C);
    lbl_805FB054 = 0;
    for (u32 j = 0; j < count; j++) {
        u8* e = *(u8**)(info + j * 8 + 0x70);
        u8 type = *(u8*)(e + 0xC);
        switch (type) {
        case 1:
            fn_8003EA7C(e);
            break;
        case 0:
            if (e != NULL) {
                lbl_803C4E60[lbl_805FB054] = SLOT_GET(type)(*(u8**)(e + 0x8), 5);
                SLOT_SET(*(u8*)(e + 0xC))(*(u8**)(e + 0x8), 5, kOneBits);
                lbl_805FB054 += 1;
            }
            break;
        }
    }
}

// Second pass: type 1 entries have their slot-0x3C value decremented, type 0 entries get the
// saved slot-5 value written back.
static void RestoreAttached(u8* info) {
    u32 count = *(u32*)(info + 0x6C);
    lbl_805FB054 = 0;
    for (u32 j = 0; j < count; j++) {
        u8* e = *(u8**)(info + j * 8 + 0x70);
        u8 type = *(u8*)(e + 0xC);
        switch (type) {
        case 1:
            if (e != NULL) {
                s32 v = SLOT_GET(type)(*(u8**)(e + 0x8), 0x3C);
                SLOT_SET(*(u8*)(e + 0xC))(*(u8**)(e + 0x8), 0x3C, v - 1);
            }
            break;
        case 0:
            if (e != NULL) {
                SLOT_SET(*(u8*)(e + 0xC))(*(u8**)(e + 0x8), 5, lbl_803C4E60[lbl_805FB054]);
                lbl_805FB054 += 1;
            }
            break;
        }
    }
}

// Renders one entry (12-float transform block at entry+4) of a model object: squashes the
// transform to a point (z = height + 0.025), saves/resets the object's own rotation, sets up the
// camera matrix and lets the attached list draw, then restores everything.
static void RenderEntry(u8* obj, s32 i, f32 height) {
    u8* entry = fn_801DA788(obj, i);
    u8* xf = *(u8**)(entry + 0x4);
    f32 saved[12];
    for (s32 k = 0; k < 12; k++) saved[k] = ((f32*)xf)[k];
    *(f32*)(xf + 0x8)  = 0.0001f;
    *(f32*)(xf + 0x14) = 0.0001f;
    *(f32*)(xf + 0x20) = 0.0001f;
    *(f32*)(xf + 0x2C) = 0.025f + height;
    u8* info = *(u8**)(fn_801DA788(obj, i) + 0x10);
    SaveAndSetAttached(info);
    f32 sx = *(f32*)(obj + 0x38);
    f32 sy = *(f32*)(obj + 0x3C);
    f32 sz = *(f32*)(obj + 0x40);
    f32 sw = *(f32*)(obj + 0x44);
    *(f32*)(obj + 0x38) = 0.0f;
    *(f32*)(obj + 0x3C) = 0.0f;
    *(f32*)(obj + 0x40) = 0.0f;
    *(f32*)(obj + 0x44) = 1.0f;
    f32 mtx[3][4];
    u8* cam = *(u8**)(lbl_803C4298 + 0x4);
    (*(GetMtxFn*)(*(u8**)cam + 0x38))(cam, &mtx[0][0]);
    CameraSetMatrix(lbl_8047C640, mtx);
    fn_80009840(lbl_803C4298);
    fn_801719DC(entry);
    RestoreAttached(info);
    *(f32*)(obj + 0x38) = sx;
    *(f32*)(obj + 0x3C) = sy;
    *(f32*)(obj + 0x40) = sz;
    *(f32*)(obj + 0x44) = sw;
    for (s32 k = 0; k < 12; k++) ((f32*)xf)[k] = saved[k];
}

// Fighter slot pointer -> its owning object (sub-object at +0x20 is embedded 0x264 bytes in).
static u8* SubToObj(u8* p) {
    u8* s = (p != NULL) ? *(u8**)(p + 0x20) : NULL;
    return (s != NULL) ? s - 0x264 : NULL;
}

// Is slot a valid zone-slot index (0 .. total of the 13 global-settings counters)?
static BOOL SlotInRange(s32 slot) {
    BOOL ok = FALSE;
    if (slot >= 0) {
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
        if (slot < m + (l + (k + (j + (i + (h + (g + (f + (e + (d + (c + (b + a))))))))))))
            ok = TRUE;
    }
    return ok;
}

extern "C" {

// 0x8003ED08
// Re-renders every entry of self's model object collapsed to self's position (for the shadow /
// reflection capture); skipped when the object's W component is ~0.
void fn_8003ED08(u8* self) {
    u8* obj = (*(GetObjFn*)(*(u8**)self + 0xC))(self);
    f32 w = *(f32*)(obj + 0x44);
    if (w <= 0.0001f && w >= -0.0001f) return;
    if (lbl_805FB4C0 != 0 && (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0) > lbl_805FB4C8) {
        fn_8014DE68();
    }
    for (s32 i = 0; i < fn_801DA780(obj); i++) {
        RenderEntry(obj, i, *(f32*)(*(u8**)(self + 0x4) + 0x8));
    }
}

// 0x8003F1B8
// Same as fn_8003ED08 for each of the five fighters of battle side arg0, using the height of
// fighter 1 for every entry.
void fn_8003F1B8(s32 arg0) {
    u8* rec = *(u8**)(gBattle + 0xA00 + arg0 * 4);
    if (rec == NULL) return;
    if (*(s32*)(rec + 0x1C) == 0) return;
    f32 height = *(f32*)(*(u8**)(SubToObj(*(u8**)(rec + 0x4)) + 0x4) + 0x8);
    for (s32 f = 0; f < 5; f++) {
        u8* self = SubToObj(*(u8**)(rec + f * 4));
        if (self == NULL) continue;
        u8* obj = (*(GetObjFn*)(*(u8**)self + 0xC))(self);
        f32 w = *(f32*)(obj + 0x44);
        if (w <= 0.0001f && w >= -0.0001f) continue;
        for (s32 i = 0; i < fn_801DA780(obj); i++) {
            RenderEntry(obj, i, height);
        }
    }
}

// 0x800DB9A4
// Constructor: sets the table pointers, allocates the 0x40-byte transform block (position 0,
// quaternion 0,0,0,1) and picks up the world's current zone block (0x2C0) with its position and
// the value from fn_800DE2AC. Returns the object.
u8* fn_800DB9A4(u8* p) {
    *(u8**)(p + 0x0)   = lbl_8035FD14;
    *(u8**)(p + 0x68)  = lbl_8035FF78;
    *(s32*)(p + 0x70)  = 6;
    *(u8**)(p + 0x140) = lbl_8035FEF8;
    *(s32*)(p + 0x148) = 7;
    *(u8**)(p + 0x17C) = lbl_8035FE20;
    *(s32*)(p + 0x184) = 0xC;
    *(f32*)(p + 0x1D8) = 0.0f;
    *(f32*)(p + 0x1DC) = 0.0f;
    *(u8**)(p + 0x1E0) = lbl_8035FEA8;
    *(s32*)(p + 0x1E8) = 0xB;
    *(s32*)(p + 0x8)   = 0;
    *(u8*)(p + 0x18)   = 0;
    u8* blk = (u8*)MemAllocDefault(0x40);
    *(u8**)(p + 0x4) = blk;
    *(f32*)(blk + 0x0) = 0.0f;
    *(f32*)(blk + 0x4) = 0.0f;
    *(f32*)(blk + 0x8) = 0.0f;
    blk = *(u8**)(p + 0x4);
    *(f32*)(blk + 0xC)  = 0.0f;
    *(f32*)(blk + 0x10) = 0.0f;
    *(f32*)(blk + 0x14) = 0.0f;
    *(f32*)(blk + 0x18) = 1.0f;
    *(s32*)(p + 0x2BC) = 0;
    *(f32*)(p + 0xC)   = 0.0f;
    *(f32*)(p + 0x10)  = 0.0f;
    *(f32*)(p + 0x14)  = 0.0f;
    *(s32*)(p + 0x1F8) = 0;
    *(u8*)(p + 0x21)   = 0;
    *(u8*)(p + 0x20)   = 0;
    *(f32*)(p + 0x28)  = 0.0f;
    *(f32*)(p + 0x24)  = 0.0f;
    *(u8*)(p + 0x2C)   = 0;
    *(f32*)(p + 0x30)  = 0.0f;
    *(f32*)(p + 0x3C)  = 0.0f;
    *(f32*)(p + 0x40)  = 0.0f;
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x8);
    *(u8**)(p + 0x2C0) = zone;
    if (zone != NULL) {
        u8* v = *(u8**)(zone + 0x4);
        *(f32*)(p + 0x44) = *(f32*)(v + 0x0);
        *(f32*)(p + 0x48) = *(f32*)(v + 0x4);
        *(f32*)(p + 0x4C) = *(f32*)(v + 0x8);
        if (*(u8*)(*(u8**)(zone + 0x4) + 0x34) == 4) {
            *(f32*)(p + 0x5C) = fn_800DE2AC(zone);
        } else {
            *(f32*)(p + 0x5C) = 0.0f;
        }
    } else {
        *(f32*)(p + 0x44) = 0.0f;
        *(f32*)(p + 0x48) = 0.0f;
        *(f32*)(p + 0x4C) = 0.0f;
        *(f32*)(p + 0x5C) = 0.0f;
    }
    *(f32*)(p + 0x50) = *(f32*)(p + 0x44);
    *(f32*)(p + 0x54) = *(f32*)(p + 0x48);
    *(f32*)(p + 0x58) = *(f32*)(p + 0x4C);
    *(f32*)(p + 0x60) = *(f32*)(p + 0x5C);
    return p;
}

// 0x80122220
// Fills the four 0x1C-byte records at lbl_803E6C20 with per-level values: three floats at
// +0xC/+0x10/+0x14, a halfword at +0x1A and the constant 0xB at +0x18.
void fn_80122220(void) {
    u8* r = lbl_803E6C20;
    for (s32 i = 0; i < 4; i++) {
        switch (i + 1) {
        case 1: *(f32*)(r + 0xC) = 4.0f; break;
        case 2: *(f32*)(r + 0xC) = 4.0f; break;
        case 3: *(f32*)(r + 0xC) = 6.0f; break;
        case 4: *(f32*)(r + 0xC) = 10.0f; break;
        default: *(f32*)(r + 0xC) = 0.0f; break;
        }
        switch (i + 1) {
        case 1: *(f32*)(r + 0x10) = 5.0f; break;
        case 2: *(f32*)(r + 0x10) = 8.0f; break;
        case 3: *(f32*)(r + 0x10) = 20.0f; break;
        case 4: *(f32*)(r + 0x10) = 30.0f; break;
        default: *(f32*)(r + 0x10) = 0.0f; break;
        }
        switch (i + 1) {
        case 1: *(f32*)(r + 0x14) = 10.0f; break;
        case 2: *(f32*)(r + 0x14) = 10.0f; break;
        case 3: *(f32*)(r + 0x14) = 15.0f; break;
        case 4: *(f32*)(r + 0x14) = 20.0f; break;
        default: *(f32*)(r + 0x14) = 0.0f; break;
        }
        switch (i + 1) {
        case 1: *(u16*)(r + 0x1A) = 3; break;
        case 2: *(u16*)(r + 0x1A) = 2; break;
        case 3: *(u16*)(r + 0x1A) = 4; break;
        case 4: *(u16*)(r + 0x1A) = 5; break;
        default: *(u16*)(r + 0x1A) = 0; break;
        }
        *(u16*)(r + 0x18) = 0xB;
        r += 0x1C;
    }
}

// 0x80128348
// Entity zone switch: when zone differs from the entity's current zone, calls vtable slot 0x18
// (arg 1 when the entity is locked in, else 0); in the locked case also clears bit 1 of the
// object returned by slot 0xC and resets the flags at +0x20/+0x35. Always returns 1.
s32 fn_80128348(void* entp, s32 zone, s32 arg2) {
    u8* ent = (u8*)entp;
    if (zone == arg2) return 1;
    u8* info = *(u8**)(ent + 0x4);
    if (info == NULL) return 1;
    u16 cur = *(u16*)(info + 0x32);
    if (zone == (s32)cur) return 1;
    if (cur != 0 && (*(u32*)(info + 0x20) & 3) != 0) {
        ScopedLock l1;
        ScopedLockCtor(&l1);
        (*(VSetFn*)(*(u8**)ent + 0x18))(ent, 1);
        ScopedLockDtor(&l1, -1);
        if ((*(GetObjFn*)(*(u8**)ent + 0xC))(ent) != NULL) {
            u8* o = (*(GetObjFn*)(*(u8**)ent + 0xC))(ent);
            *(u32*)(o + 0x8) &= ~2u;
        }
        u8* a = *(u8**)(ent + 0x4);
        *(u32*)(a + 0x20) &= 0xFFFDFCFCu;
        u8* b = *(u8**)(ent + 0x4);
        *(u32*)(b + 0x20) |= 0x400;
        *(u8*)(*(u8**)(ent + 0x4) + 0x35) = 0;
    } else {
        ScopedLock l2;
        ScopedLockCtor(&l2);
        (*(VSetFn*)(*(u8**)ent + 0x18))(ent, 0);
        ScopedLockDtor(&l2, -1);
    }
    return 1;
}

// 0x80133A84
// Appends one light record for the zone object to the lbl_803E9FC8 tables at index
// lbl_805FB324 (colours, directions, weights from the object) and computes a blend weight from
// the distance to the player: 0 beyond 1.05 * the object's own extent, cosine falloff inside.
// The weight is stored and added to the running total lbl_805FB320. Returns 1.
s32 fn_80133A84(u8* o, s32 arg1) {
    if (*(s32*)(o + 0x18) != 0) {
        f32 x = *(f32*)(o + 0xC);
        f32 y = *(f32*)(o + 0x10);
        f32 z = *(f32*)(o + 0x14);
        f32 len2 = z * z + (x * x + y * y);
        f32 radius = 0.0f;
        if (!(len2 <= 0.0f)) radius = sqrtf(len2);   // original: frsqrte with two Newton steps
        u8* t;
        t = *(u8**)(lbl_803E9FC8 + 0xC) + lbl_805FB324 * 0x10;
        *(f32*)(t + 0x0) = *(f32*)(o + 0x54);
        *(f32*)(t + 0x4) = *(f32*)(o + 0x58);
        *(f32*)(t + 0x8) = *(f32*)(o + 0x5C);
        *(f32*)(t + 0xC) = *(f32*)(o + 0x60);
        t = *(u8**)(lbl_803E9FC8 + 0x3C) + lbl_805FB324 * 0x10;
        *(f32*)(t + 0x0) = *(f32*)(o + 0x64);
        *(f32*)(t + 0x4) = *(f32*)(o + 0x68);
        *(f32*)(t + 0x8) = *(f32*)(o + 0x6C);
        *(f32*)(t + 0xC) = *(f32*)(o + 0x70);
        t = *(u8**)(lbl_803E9FC8 + 0x54) + lbl_805FB324 * 0xC;
        *(f32*)(t + 0x0) = *(f32*)(o + 0x74);
        *(f32*)(t + 0x4) = *(f32*)(o + 0x78);
        *(f32*)(t + 0x8) = *(f32*)(o + 0x7C);
        t = *(u8**)(lbl_803E9FC8 + 0x24) + lbl_805FB324 * 0x10;
        *(f32*)(t + 0x0) = *(f32*)(o + 0x30);
        *(f32*)(t + 0x4) = *(f32*)(o + 0x34);
        *(f32*)(t + 0x8) = *(f32*)(o + 0x38);
        *(f32*)(t + 0xC) = *(f32*)(o + 0x3C);
        ((f32*)*(u8**)(lbl_803E9FC8 + 0x6C))[lbl_805FB324] = *(f32*)(o + 0x40);
        ((f32*)*(u8**)(lbl_803E9FC8 + 0x84))[lbl_805FB324] = *(f32*)(o + 0x44);
        ((f32*)*(u8**)(lbl_803E9FC8 + 0x9C))[lbl_805FB324] = *(f32*)(o + 0x48);
        u8* pl = *(u8**)(*(u8**)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0) + 0x4);
        f32 dx = *(f32*)(o + 0x0) - *(f32*)(pl + 0x0);
        f32 dy = *(f32*)(o + 0x4) - *(f32*)(pl + 0x4);
        f32 dz = *(f32*)(o + 0x8) - *(f32*)(pl + 0x8);
        f32 d2 = dz * dz + (dx * dx + dy * dy);
        f32 dist = 0.0f;
        if (!(d2 <= 0.0f)) dist = sqrtf(d2);   // original: frsqrte with two Newton steps
        f32 lim = 1.05f * radius;
        if (dist > lim) {
            ((f32*)*(u8**)(lbl_803E9FC8 + 0xB4))[lbl_805FB324] = 0.0f;
        } else {
            f32 t2 = 0.5f * (1.0f + cosf(1.5707964f * (2.0f * (dist / lim))));
            ((f32*)*(u8**)(lbl_803E9FC8 + 0xB4))[lbl_805FB324] = 0.0f * (1.0f - t2) + 1.0f * t2;
        }
        u32 k = lbl_805FB324;
        f32 sum = lbl_805FB320 + ((f32*)*(u8**)(lbl_803E9FC8 + 0xB4))[k];
        lbl_805FB324 = k + 1;
        lbl_805FB320 = sum;
    }
    return 1;
}

// 0x80137684
// For the zone assigned to the given slot, walks all its objects (first the table at
// +0x8C/+0x120, then the extra table at +0x90/+0x12C) and calls fn_8006D774 for those whose
// record has flags 0x100 (+0x1C) and bit 0 (+0x20) set, with an owner (+4) that also has 0x100.
void fn_80137684(u8* world, s32 slot) {
    s32 zone = SlotInRange(slot) ? *(s32*)(*(u8**)(world + 0xB0) + slot * 0x30 + 0x8) : -2;
    if (zone > 0) {
        s32 off4 = 0;
        s32 off64 = 0;
        u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
        s32 total = *(s32*)(res + 0x94) + *(s32*)(res + 0x98);
        for (s32 n = 0; n < total; n++) {
            res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
            s32 first = *(s32*)(res + 0x94);
            u8* ent;
            u8* rec;
            if (n < first) {
                ent = *(u8**)(*(u8**)(res + 0x120) + off4);
                rec = *(u8**)(res + 0x8C) + off64;
            } else {
                s32 m = n - first;
                ent = *(u8**)(*(u8**)(res + 0x12C) + m * 4);
                rec = *(u8**)(res + 0x90) + (m << 6);
            }
            if ((*(u32*)(rec + 0x1C) & 0x100) != 0 && (*(u32*)(rec + 0x20) & 1) != 0 && ent != NULL) {
                u8* owner = *(u8**)(ent + 0x4);
                if (owner != NULL && (*(u32*)(owner + 0x1C) & 0x100) != 0) {
                    fn_8006D774(ent);
                }
            }
            off64 += 0x40;
            off4 += 4;
        }
    }
}

// 0x80138964
// Returns the zone assigned to a slot (word at +8 of the 0x30-byte slot record), or -2 when the
// slot index is out of range.
s32 fn_80138964(u8* world, s32 slot) {
    if (SlotInRange(slot)) {
        return *(s32*)(*(u8**)(world + 0xB0) + slot * 0x30 + 0x8);
    }
    return -2;
}

}
