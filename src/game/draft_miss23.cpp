// Misc functions: Sir loader (SirLoad / SirFinishLoad), GrowJuice (seed / launcher / tentacle /
// heart), HeadToFriendslist, HideAlertBox, IncOptionsBrightness, small field initialisers.
#include <math.h>
#include <string.h>

#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
f64    fn_8033F1F4(f64 x);                                  // 0x8033F1F4 (arc-cosine-like, f64 in/out)
u32    RandomU32(void);                                     // 0x80143460
s32    RaySceneTest(void* ray);                             // 0x80070458 (ignores its argument)
void*  ScreenSetFindWidget(void* set, u32 hash);            // 0x80198E34
void*  ScreenSetsCurrent(void* sets);                       // 0x8018D1E4
void*  UIWidgetFindChild(void* widget, u32 hash);
void   UIWidgetPlayAnim(void* w, u32 anim);                 // 0x80199B38
void   fn_800A7D88(void* obj, u32 hash);                    // 0x800A7D88 (send event hash to obj)
void   fn_800AEE74(void* obj, u32 hash);                    // 0x800AEE74
BOOL   fn_80142358(void);                                   // 0x80142358
u32    fn_80143A5C(const char* s);                          // 0x80143A5C (name hash)
void   fn_8014474C(u8* src, f32* out);                      // 0x8014474C (writes 4 floats to out)
u8*    fn_8015EC9C(u8* tbl, f32 a, f32 b);                  // 0x8015EC9C
f32    fn_8018BE30(void* obj, f32 a);                       // 0x8018BE30
BOOL   fn_80198C34(void* set, void* w);
s32    fn_8019CC7C(void* el, u32 flag);
BOOL   fn_801B50B0(void* unused, u8* obj);                  // 0x801B50B0
void   fn_801E4918(u8* obj, s32 a, f32 t);                  // 0x801E4918
u32 fn_801E65B8(u8* obj, const char* name, s32 user, s32 unused, s32 key, s32 val);  // 0x801E65B8
s32    fn_801E6FAC(u8* ent, s32 flag, f32 t);               // 0x801E6FAC
u32 fn_801E6934(u8* loader, void* a, void* dst, u32 b, u32 c, s32 d, s32 e, s32 f, s32 flags);  // 0x801E6934
void   fn_801EC8C4(u8* p, u32 a, u32 b);                    // 0x801EC8C4
BOOL   fn_801F56FC(s32 flags);                              // 0x801F56FC
void   SirFinishLoad(void* loader, void** handle);          // 0x801E6C44
void*  SirLoad(void* loader, const char* path, void* dst, u32 size, s32 flags);  // 0x801E6138

extern u8  gIdPool[];                  // 0x8048A8F0
extern u8  lbl_803C4298[];             // 0x803C4298
extern u8  lbl_8040D500[];             // 0x8040D500
extern u8  lbl_8048A8B4[];             // 0x8048A8B4
extern u8  gWorldState[];              // 0x803E6D00
extern u32 lbl_80346C80[];             // 0x80346C80 (weights indexed by RaySceneTest result)
extern u32 lbl_80346C90[];             // 0x80346C90
extern u32 lbl_80346CA0[];             // 0x80346CA0
extern s32 lbl_805FAE8C;               // 0x805FAE8C
extern s32 lbl_805FB418;               // 0x805FB418 (connection state flags)
extern s32 lbl_805FB438;               // 0x805FB438 (pending response id)
extern void* lbl_805FB964;             // 0x805FB964 (object with a virtual ready check at vtable +0x34)
extern s32 lbl_805FB970;               // 0x805FB970
extern s32 lbl_805FB974;               // 0x805FB974
extern s32 lbl_805FB978;               // 0x805FB978
extern u8* lbl_805FBBE0;               // 0x805FBBE0 (options record)
}
extern "C++" void* gScreenSets;        // 0x805FB654
extern "C++" s32   gZoneResourceSize;  // 0x805FB94C

// Ring buffer over capacity cap (head = first index, i = candidate index): true when i is a valid
// slot of the ring.
static inline bool RingIdxValid(s32 head, s32 i, s32 cap) {
    if (i < 0 || i >= cap) return false;
    s32 lo = i, hi = i;
    if (i < head) {
        lo = i + cap;
        hi = i + cap;
    }
    return head <= lo && lo <= hi;
}

// Entry of a 0x88-byte slot array at +0x1C of the loader for ring index i, or NULL.
static inline u8* RingEntry(u8* loader, s32 i) {
    if (RingIdxValid(*(s32*)(loader + 0x10), i, *(s32*)(loader + 0x18)))
        return *(u8**)(loader + 0x1C) + i * 0x88;
    return NULL;
}

// Drops the newest ring index (tail at +0x14, head at +0x10, capacity at +0x18).
static inline void RingPopBack(u8* r) {
    s32 tail = *(s32*)(r + 0x14);
    if (tail == *(s32*)(r + 0x10)) {
        *(s32*)(r + 0x14) = -1;
        *(s32*)(r + 0x10) = -1;
    } else {
        s32 v = tail - 1;
        if (v < 0) v = *(s32*)(r + 0x18) - 1;
        *(s32*)(r + 0x14) = v;
    }
}

extern "C" {

// 0x801E6C44
// Finishes the loads queued for the handle: pops entries off the loader's request ring and runs
// them (fn_801E6FAC) until the handle's slot reports done.
void SirFinishLoad(void* loader_, void** handle) {
    u8* loader = (u8*)loader_;
    u32 h = *(u32*)handle;
    u8* slot;
    if (h == 0xFFFFFFFFu) {
        slot = NULL;
    } else {
        slot = *(u8**)(loader + 0x4) + (h & 0x7F) * 0x88;
        if ((h >> 7) != *(u32*)(slot + 0x74)) slot = NULL;
    }
    while (slot != NULL && *(s32*)(slot + 0x4) != 0) {
        fn_801E4918(lbl_8048A8B4, 1, 0.0f);
        if (*(s32*)(loader + 0x10) >= 0 && *(s32*)(loader + 0x14) >= 0) {
            u8* ent = RingEntry(loader, *(s32*)(loader + 0x14));
            while (ent != NULL && *(s32*)(ent + 0x70) != 0) {
                RingPopBack(loader);
                ent = RingEntry(loader, *(s32*)(loader + 0x14));
            }
            if (ent != NULL && fn_801E6FAC(ent, 1, 0.01f) == 1) {
                *(s32*)(ent + 0x4) = 0;
                RingPopBack(gIdPool);
            }
        }
    }
}

// 0x801E6138
// Starts loading the named entry into dst. If an earlier request with the same name and flags is
// pending it is resumed (fn_801E65B8); otherwise the name is hashed and looked up in the file
// table and the load starts (fn_801E6934).
void* SirLoad(void* loader_, const char* path, void* dst, u32 size, s32 flags) {
    u8* loader = (u8*)loader_;
    s32 sp10 = 0;
    if (*(u8*)loader != 0) {
        for (s32 i = 0; i < 8; i++) {
            if (*(s32*)(loader + 0x46C + i * 4) != 0 && (u32)flags == *(u32*)(loader + 0x42C + i * 4) &&
                strcmp((const char*)(loader + 0x22C + i * 0x40), path) == 0) {
                void* r = (void*)(uintptr_t)fn_801E65B8(loader, (const char*)(loader + 0x2C + i * 0x40), (s32)(uintptr_t)dst, size, flags,
                                      *(s32*)(loader + 0x44C + i * 4));  // lint:ok 32-bit handle
                *(s32*)(loader + 0x46C + i * 4) = 0;
                return r;
            }
        }
    } else {
        for (s32 i = 0; i < 8; i++) *(s32*)(loader + 0x46C + i * 4) = 0;
    }

    u8* tbl = *(u8**)(loader + 0x7B4);
    char name[0x38];
    memset(name, 0, sizeof(name));
    u32 n = 0;
    while (*path != 0 && n < 0x38) {
        name[n] = *path;
        n++;
        path++;
    }
    name[0x37] = 0;
    u32 hash = fn_80143A5C(name);

    u8* ent = NULL;
    u32 count = *(u32*)(tbl + 0x4);
    for (u32 i = 0; i < count; i++) {
        u8* e = *(u8**)(tbl + 0x0) + i * 0x18;
        if ((u32)flags == *(u32*)(e + 0x0) && hash == *(u32*)(e + 0xC)) {
            u32 cnt2 = *(u32*)(tbl + 0x10);
            sp10 = (s32)(cnt2 + 1);
            for (u32 j = 0; j < cnt2; j++) {
                if (hash == *(u32*)(*(u8**)(tbl + 0xC) + j * 0x40)) {
                    sp10 = (s32)j;
                    break;
                }
            }
            ent = e;
            break;
        }
    }
    s32 compressed = (*(s32*)(ent + 0x10) != 0) ? 1 : 0;
    return (void*)(uintptr_t)fn_801E6934(loader, *(void**)(*(u8**)(loader + 0x7A8) + sp10 * 4), dst, *(u32*)(ent + 0x8),
                       *(u32*)(ent + 0x4) >> 11, compressed, *(s32*)(ent + 0x10), *(s32*)(ent + 0x14), flags);
}

// 0x8012E7B0
// Records resource obj of zone idx in the context's 12-byte entry list (+0x294: obj, index inside
// the zone table, kind 0/1). Returns 1 when found and recorded.
s32 fn_8012E7B0(u8* ctx, u8* obj, s32 idx) {
    if (idx < 0) return 0;
    u8* zone = *(u8**)(*(u8**)ctx + 0x1A4) + idx * gZoneResourceSize;
    if (obj == NULL) return 0;
    u8* sub = *(u8**)(obj + 4);
    if (sub == NULL) return 0;
    s32 kind = ((*(u32*)(*(u8**)(obj + 4) + 0x1C) & 8) != 0) ? 0 : 1;
    s32 n = kind == 0 ? *(s32*)(zone + 0x94) : *(s32*)(zone + 0x98);
    u32 listOff = kind == 0 ? 0x120 : 0x12C;
    u32 baseOff = kind == 0 ? 0x8C : 0x90;
    for (s32 i = 0; i < n; i++) {
        if (obj == *(u8**)(*(u8**)(zone + listOff) + i * 4) && sub == *(u8**)(zone + baseOff) + i * 0x40) {
            s32 cnt = *(s32*)(ctx + 0x3294);
            *(u8**)(ctx + cnt * 0xC + 0x294) = obj;
            *(s32*)(ctx + *(s32*)(ctx + 0x3294) * 0xC + 0x298) = i;
            *(u8*)(ctx + *(s32*)(ctx + 0x3294) * 0xC + 0x29C) = (u8)kind;
            *(s32*)(ctx + 0x3294) = *(s32*)(ctx + 0x3294) + 1;
            return 1;
        }
    }
    return 0;
}

// 0x8018A5F4
// Initialises the record: three 0x24-byte sub-records at +4/+0x28/+0x4C (ids -1, scales 0.075,
// 1.0, flag 0x20000) followed by the remaining state fields.
void fn_8018A5F4(void* arg) {
    u8* p = (u8*)arg;
    *(s32*)(p + 0x4) = -1;
    *(s32*)(p + 0x8) = 0;
    *(f32*)(p + 0xC) = 0.0f;
    *(f32*)(p + 0x10) = 0.075f;
    *(f32*)(p + 0x14) = 0.075f;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1C) = 0;
    *(f32*)(p + 0x20) = 1.0f;
    *(s32*)(p + 0x24) = -1;
    *(s32*)(p + 0x24) = -1;
    *(u32*)(p + 0x18) |= 0x20000;
    *(s32*)(p + 0x28) = -1;
    *(s32*)(p + 0x2C) = 0;
    *(f32*)(p + 0x30) = 0.0f;
    *(f32*)(p + 0x34) = 0.075f;
    *(f32*)(p + 0x38) = 0.075f;
    *(s32*)(p + 0x3C) = 0;
    *(s32*)(p + 0x40) = 0;
    *(f32*)(p + 0x44) = 1.0f;
    *(s32*)(p + 0x48) = -1;
    *(s32*)(p + 0x48) = -1;
    *(u32*)(p + 0x3C) |= 0x20000;
    *(s32*)(p + 0x4C) = -1;
    *(s32*)(p + 0x50) = 0;
    *(f32*)(p + 0x54) = 0.0f;
    *(f32*)(p + 0x58) = 0.075f;
    *(f32*)(p + 0x5C) = 0.075f;
    *(s32*)(p + 0x60) = 0;
    *(s32*)(p + 0x64) = 0;
    *(f32*)(p + 0x68) = 1.0f;
    *(s32*)(p + 0x6C) = -1;
    *(s32*)(p + 0x6C) = -1;
    *(u32*)(p + 0x60) |= 0x20000;
    *(s32*)(p + 0xEC) = 0;
    *(s32*)(p + 0x70) = 0;
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x74) = 0;
    *(s32*)(p + 0x78) = 0;
    *(s32*)(p + 0x7C) = 0;
    *(f32*)(p + 0x9C) = 0.0f;
    *(f32*)(p + 0x94) = 0.0f;
    *(f32*)(p + 0x98) = 0.0f;
    *(s32*)(p + 0xF0) = 0;
    *(s32*)(p + 0xA0) = 0;
    *(s32*)(p + 0xA4) = 0;
    *(f32*)(p + 0xB0) = 1.0f;
    *(f32*)(p + 0xB4) = 1.0f;
    *(f32*)(p + 0xB8) = 0.0f;
    *(f32*)(p + 0xC0) = 0.0f;
    *(f32*)(p + 0xC4) = 0.0f;
    *(f32*)(p + 0xC8) = 0.0f;
    *(f32*)(p + 0xCC) = 0.0f;
    *(f32*)(p + 0xD0) = 0.0f;
    *(f32*)(p + 0xD4) = 0.0f;
    *(s32*)(p + 0xD8) = 0;
    *(f32*)(p + 0xDC) = 0.0f;
    *(f32*)(p + 0xE0) = 0.0f;
    *(f32*)(p + 0xE4) = 0.0f;
    *(f32*)(p + 0xE8) = 1.0f;
    *(f32*)(p + 0xA8) = 0.075f;
    *(f32*)(p + 0xAC) = 0.075f;
    *(u8*)(p + 0x8C) = 0;
    *(s32*)(p + 0xBC) = 0;
}

// 0x800B182C
// Picks the emitter of heart a1 nearest to a0's position (or its weapon target position when the
// owner is grow_juice_weaponA) and triggers effect a2 on it.
void GrowJuiceHeart_PlayEffectNear(void* a0_, void* a1_, u32 a2) {
    u8* a0 = (u8*)a0_;
    u8* a1 = (u8*)a1_;
    if (a0 == NULL || a1 == NULL) return;
    u8* pos = *(u8**)(a0 + 0x4);
    u8* emitters = a1 + 0x94;
    u8* owner = *(u8**)(a0 + 0x9C);
    s32 best = -1;
    f32 px = *(f32*)(pos + 0x0);
    f32 py = *(f32*)(pos + 0x4);
    f32 pz = *(f32*)(pos + 0x8);
    f32 bestD = 1000000.0f;
    u32 ownerId = owner != NULL ? *(u32*)(owner + 0x8) : 0;
    if (ownerId == 0x861FD133u) {  // "grow_juice_weaponA"
        px = *(f32*)(a0 + 0x27C);
        py = *(f32*)(a0 + 0x280);
        pz = *(f32*)(a0 + 0x284);
    }
    s32 count = *(s32*)emitters;
    for (s32 i = 0; i < count; i++) {
        u8* e = *(u8**)(emitters + 4) + i * 0xC8;
        f32 dx = *(f32*)(e + 0x5C) - px;
        f32 dy = *(f32*)(e + 0x60) - py;
        f32 dz = *(f32*)(e + 0x64) - pz;
        f32 d = dz * dz + (dx * dx + dy * dy);
        if (!(d >= bestD)) {
            best = i;
            bestD = d;
        }
    }
    if (best != -1) fn_801EC8C4(emitters, (u32)best, a2);
}

// 0x800ACFA4
// Fires the loaded seed: moves its object to the launcher's slot position, copies the orientation
// from fn_8014474C, sets the object's flag bits, signals it and releases the emitter slot.
void GrowJuiceLauncher_FireSeed(void* a0_) {
    u8* a0 = (u8*)a0_;
    if (a0 == NULL) return;
    u8* obj = *(u8**)(a0 + 0x278);
    if (obj == NULL) return;
    fn_801B50B0(a0, obj);
    obj = *(u8**)(a0 + 0x278);
    u8* xf = *(u8**)(obj + 0x4);
    u8* slot = *(u8**)(a0 + 0x98) + *(s32*)(a0 + 0x264) * 0xC8;
    *(f32*)(xf + 0x0) = *(f32*)(slot + 0x5C);
    *(f32*)(xf + 0x4) = *(f32*)(slot + 0x60);
    *(f32*)(xf + 0x8) = *(f32*)(slot + 0x64);
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
    f32 q[4];
    fn_8014474C(*(u8**)(a0 + 0x98) + *(s32*)(a0 + 0x264) * 0xC8 + 0x38, q);
    u8* obj2 = *(u8**)(a0 + 0x278);
    u8* xf2 = *(u8**)(obj2 + 0x4);
    *(f32*)(xf2 + 0xC) = q[0];
    *(f32*)(xf2 + 0x10) = q[1];
    *(f32*)(xf2 + 0x14) = q[2];
    *(f32*)(xf2 + 0x18) = q[3];
    *(u32*)(*(u8**)(obj2 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(obj2 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(obj2 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(obj2 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(*(u8**)(a0 + 0x278) + 0x4) + 0x20) |= 1;
    fn_800A7D88(*(void**)(a0 + 0x278), 0x2AB014A3u);
    *(void**)(a0 + 0x278) = NULL;
    fn_801EC8C4(a0 + 0x94, (u32) * (s32*)(a0 + 0x264), 0x4A5503CEu);
}

// 0x800AC5F8
// Turns the seed into a tentacle: sends the matching event for its state, resets the state fields,
// places it at (-0, -0, -1) and sets the object's flag bits and the +0x38..0x44 scale to 1.0.
void GrowJuiceSeed_ConvertToTentacle(void* a0_) {
    u8* a0 = (u8*)a0_;
    if (a0 == NULL) return;
    s32 state = *(s32*)(a0 + 0x268);
    if (state == 0) {
        fn_800AEE74(*(void**)(a0 + 0x2A0), 0x8EAA2203u);
    } else if (state == 1) {
        fn_800AEE74(*(void**)(a0 + 0x2A0), 0x77F143B4u);
    }
    *(s32*)(a0 + 0x264) = 0;
    u8* pos = *(u8**)(a0 + 0x4);
    *(s32*)(a0 + 0x268) = -1;
    *(f32*)(pos + 0x0) = -0.0f;
    *(f32*)(pos + 0x4) = -0.0f;
    *(f32*)(pos + 0x8) = -1.0f;
    *(u32*)(*(u8**)(a0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(a0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(a0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(a0 + 0x4) + 0x20) |= 0x10000000;
    u8* scale = *(u8**)(a0 + 0x90);
    *(f32*)(scale + 0x38) = 1.0f;
    *(f32*)(scale + 0x3C) = 1.0f;
    *(f32*)(scale + 0x40) = 1.0f;
    *(f32*)(scale + 0x44) = 1.0f;
}

}  // extern "C"

// Angle measure between the tentacle's (negated) position vector and the (negated) vector at
// lbl_803C4298->+4->+4, scaled so 90 degrees = 1 (0 when either vector is degenerate).
static f32 TentacleAimAngle(u8* a0) {
    u8* p = *(u8**)(a0 + 0x4);
    u8* q = *(u8**)(*(u8**)(lbl_803C4298 + 0x4) + 0x4);
    f32 ax = -*(f32*)(p + 0x0);
    f32 ay = -*(f32*)(p + 0x4);
    f32 az = -*(f32*)(p + 0x8);
    f32 bx = -*(f32*)(q + 0x0);
    f32 by = -*(f32*)(q + 0x4);
    f32 bz = -*(f32*)(q + 0x8);
    f32 lenA2 = az * az + (ax * ax + ay * ay);
    f32 dot = az * bz + (ax * bx + ay * by);
    f32 lenA = 0.0f;
    if (!(lenA2 <= 0.0f)) lenA = sqrtf(lenA2);  // original: frsqrte with two Newton steps
    f32 lenB2 = bz * bz + (bx * bx + by * by);
    f32 lenB = 0.0f;
    if (!(lenB2 <= 0.0f)) lenB = sqrtf(lenB2);
    f32 ang = 0.0f;
    f32 prod = lenA * lenB;
    if (!(prod <= 0.0f)) {
        f32 c = dot / prod;
        f32 r = -1.0f;
        if (!(c < -1.0f)) {
            r = 1.0f;
            if (!(c > 1.0f)) r = c;
        }
        f32 c2 = r;
        f32 r2 = -1.0f;
        if (!(c2 < -1.0f)) {
            r2 = 1.0f;
            if (!(c2 > 1.0f)) r2 = c2;
        }
        ang = 0.63661975f * (f32)fn_8033F1F4(r2);
    }
    return ang;
}

extern "C" {

// 0x800AE71C
// True when the tentacle points within 22.5 degrees of the reference direction.
s32 GrowJuiceTentacle_CheckHitPlayer(void* a0) {
    if (a0 == NULL) return 0;
    return TentacleAimAngle((u8*)a0) < 0.011111111f * 22.5f ? 1 : 0;
}

// 0x800AE3B8
// Like CheckHitPlayer, but when aimed it additionally rolls a weighted random choice
// (tables indexed by RaySceneTest) to decide whether the tentacle reacts.
s32 GrowJuiceTentacle_ShouldReactToPlayer(void* a0) {
    if (a0 == NULL) return 0;
    if (TentacleAimAngle((u8*)a0) < 0.011111111f * 22.5f) {
        u32 w1 = lbl_80346C90[RaySceneTest(NULL)];
        u32 w2 = lbl_80346CA0[RaySceneTest(NULL)];
        u32 sum = w1 + w2;
        u32 total = lbl_80346C80[RaySceneTest(NULL)] + sum;
        if (total != 0 && (RandomU32() % total) < sum) return 1;
    }
    return 0;
}

// 0x800AEC44
// Sets the tentacle head size by blending start size (a2..a4) toward end size (a5..a7) by a
// growth factor chosen by mode a1 (1: timer ratio, 2: anim progress, else 1.0). Negative / zero
// start values default to the current size at +0x270. Result is clamped to 0.001 and written to
// the head object at +0x26C.
void GrowJuiceTentacle_SetHeadSizeGrowth(void* a0_, s32 a1, f32 a2, f32 a3, f32 a4, f32 a5, f32 a6, f32 a7) {
    u8* a0 = (u8*)a0_;
    f32 t;
    if (a0 == NULL) return;
    if (a1 == 1) {
        if (*(u8*)(a0 + 0x364) != 0) {
            f32 x = *(f32*)(a0 + 0x36C) / *(f32*)(a0 + 0x368);
            t = 0.0f;
            if (!(x < 0.0f)) {
                t = 1.0f;
                if (!(x > 1.0f)) t = x;
            }
        } else {
            t = 0.0f;
        }
    } else if (a1 == 2) {
        u8* anim = *(u8**)(a0 + 0xA0);
        t = anim != NULL ? fn_8018BE30(anim, 0.0f) : 0.0f;
    } else {
        t = 1.0f;
    }
    f32 cx = *(f32*)(a0 + 0x270);
    f32 cy = *(f32*)(a0 + 0x274);
    f32 cz = *(f32*)(a0 + 0x278);
    if (a2 < 0.0f) a2 = cx;
    if (a3 <= 0.0f) a3 = cy;
    if (a4 < 0.0f) a4 = cz;
    f32 sx = a2 + t * (a5 - a2);
    if (!(sx > 0.001f)) sx = 0.001f;
    f32 sy = a3 + t * (a6 - a3);
    if (!(sy > 0.001f)) sy = 0.001f;
    f32 sz = a4 + t * (a7 - a4);
    if (!(sz > 0.001f)) sz = 0.001f;
    u8* head = *(u8**)(a0 + 0x26C);
    if (head != NULL) {
        *(f32*)(head + 0x8) = sx;
        *(f32*)(head + 0xC) = sy;
        *(f32*)(head + 0x10) = sz;
    }
}

}  // extern "C"

// True when the session is ready: movie player flags set and its virtual ready check (+0x34) passes.
static inline bool MoviePlayerReady() {
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
        (*(s32 (**)(void*))(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0)
        return true;
    return false;
}

// Connection-state check: (flag 0x10 and fn_80142358) or ((flag 4 or flag 8) and fn_80142358).
static bool FriendsOnlineCheck() {
    bool a = fn_801F56FC(0x10) != 0 && fn_80142358() != 0;
    if (a) return true;
    bool b = (fn_801F56FC(4) != 0 || fn_801F56FC(8) != 0) && fn_80142358() != 0;
    return b;
}

// Hides one child widget of w if it exists.
static inline void HideChildWidget(void* w, u32 hash) {
    void* c = UIWidgetFindChild(w, hash);
    if (c != NULL) fn_8019CC7C(c, 0);
}

extern "C" {

// 0x801404F4
// Script check: returns 0 (do not head to the friends list) while the movie player is running,
// while online play is active, or while a connection state forbids it; otherwise 1.
ScriptValue HeadToFriendslist(void) {
    if (fn_801F56FC(0x80) == 0 && lbl_805FAE8C == 0) {
        bool ready = MoviePlayerReady();
        if (ready && lbl_805FB978 != 0) return 0;
    }
    if (FriendsOnlineCheck()) return 0;
    if (fn_801F56FC(0x100) != 0 && !FriendsOnlineCheck()) return 0;
    if ((lbl_805FB418 & 2) != 0 && fn_80142358() != 0) return 0;
    return 1;
}

// 0x80141EEC
// Hides the alert box widget's buttons and plays its close animation; if the box is not shown,
// signals the pending response (or the default one) to the game flow.
void HideAlertBox(void) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    void* box = ScreenSetFindWidget(set, 0x28DCC9E9u);
    if (box == NULL) return;
    if (fn_80198C34(set, box) != 0) {
        if (UIWidgetFindChild(box, 0x38569AA8u) == NULL) return;
        HideChildWidget(box, 0x123437AAu);
        HideChildWidget(box, 0x2C9F854Eu);
        HideChildWidget(box, 0xA992CF63u);
        HideChildWidget(box, 0x0980A68Du);
        UIWidgetPlayAnim(box, 0x7C61C5F9u);
    } else {
        if (lbl_805FB438 != 0) {
            GameFlowSignal(lbl_805FB438, 0, 0);
            lbl_805FB438 = 0;
        } else {
            GameFlowSignal(0x0CAA6159, 0, 0);
        }
    }
}

// 0x800FB258
// Changes the brightness option by a0 steps (0..10) and rewrites the two brightness curve
// points (+4, +8) of the table at lbl_8040D500 from the world-state limits.
void IncOptionsBrightness(s32 a0) {
    s32 v = (s32)*(u16*)(lbl_805FBBE0 + 0x91A) + a0;
    if (v < 0) {
        v = 0;
    } else if (v > 10) {
        v = 10;
    }
    s32 step = 10;
    if ((u32)v < 10u) step = v;
    u8* ws = *(u8**)gWorldState;
    f32 t = (f32)(u32)step / 10.0f;
    f32 g = *(f32*)(ws + 0x1A84);
    f32 hi = 1.0f;
    if (!(g < 1.0f)) {
        hi = 1.4f;
        if (!(g > 1.4f)) hi = g;
    }
    f32 o = *(f32*)(ws + 0x1A88);
    f32 oHi = -0.15f;
    if (!(o < -0.15f)) {
        oHi = 0.15f;
        if (!(o > 0.15f)) oHi = o;
    }
    if (t < 0.5f) {
        f32 k = t / 0.5f;
        f32 val = 1.4f + k * (hi - 1.4f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, 1.4f, k) + 0x4) = val;
        f32 k2 = t / 0.5f;
        f32 val2 = -0.15f + k2 * (oHi - -0.15f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, -0.15f, k2) + 0x8) = val2;
    } else {
        f32 k = (t - 0.5f) / 0.5f;
        f32 val = hi + k * (1.0f - hi);
        *(f32*)(fn_8015EC9C(lbl_8040D500, k, 0.5f) + 0x4) = val;
        f32 k2 = (t - 0.5f) / 0.5f;
        oHi = oHi + k2 * (0.15f - oHi);
        *(f32*)(fn_8015EC9C(lbl_8040D500, k2, 0.5f) + 0x8) = oHi;
    }
    *(u16*)(lbl_805FBBE0 + 0x91A) = (u16)v;
}

}  // extern "C"
