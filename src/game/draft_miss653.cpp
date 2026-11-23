// Weapon / camera-aim functions in 0x8009xxxx-0x800Cxxxx (aim update, slot sync, bounce reflection).
#include "types.h"
#include "engine/math.h"

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);   // 0x80145EE8
s32   RaySceneTest(void* ray);                                     // 0x80070458
u8*   fn_8006FBDC(void);                                           // 0x8006FBDC
u8*   fn_80073454(void);                                           // 0x80073454
void  fn_80073F58(f32* out, u8* cam, f32* pt, f32* a, f32* b, f32 x, f32 y);   // 0x80073F58
void  fn_800A7D88(void* obj, u32 hash);                            // 0x800A7D88 (send event hash to obj)
void  fn_800AA68C(u8* out, u8* cam, u8* entry);                    // 0x800AA68C
void  fn_800AB468(u8* p, f32 dt);                                  // 0x800AB468
void  fn_800AB5F8(u8* arg0, u8* arg1, u8* arg2);                   // 0x800AB5F8
void  fn_800AFB64(u8* p);                                          // 0x800AFB64
s32   fn_800AFBCC(void);                                           // 0x800AFBCC (plays a sound, ignores args)
u32   fn_800B0104(u8* p);                                          // 0x800B0104
void  fn_800B0CFC(u8* p);                                          // 0x800B0CFC
u32   fn_800B1464(u8* p);                                          // 0x800B1464
void  fn_800B387C(u8* p, s32 idx);                                 // 0x800B387C
void  fn_800B665C(u8* p, s32 v);                                   // 0x800B665C
void  fn_800B66D8(u8* p, s32 v);                                   // 0x800B66D8
u32   fn_800B66E0(u8* p, u8* slot, s32 idx);                       // 0x800B66E0
f32   fn_800B68B8(u8* p);                                          // 0x800B68B8
void  fn_800B7428(u8* p, s32 idx, s32 value);                      // 0x800B7428
void  fn_800B9138(u8* obj, f32 dt);                                // 0x800B9138
s32   fn_800B964C(u8* obj);                                        // 0x800B964C
f32   fn_800B9914(u8* obj);                                        // 0x800B9914
u8    fn_800B998C(u8* obj);                                        // 0x800B998C
void  fn_800B999C(u8* obj, s32 a, s32 b);                          // 0x800B999C
u8    fn_800B9A78(u8* obj);                                        // 0x800B9A78
void  fn_800C5F5C(void);                                           // 0x800C5F5C
s32   fn_800C0C30(u8* shape, u8* arg1, u8* pos, f32* normalOut, u8* flagOut);   // 0x800C0C30
BOOL  fn_80147554(s32 button);                                     // 0x80147554
const char* FormatHashString(u32 hash);                                 // 0x801EB708 (hash -> name for logs)
u32   fn_801EC880(u8* p, u32 hash);                                // 0x801EC880

extern f32 lbl_80346208[];   // 0x80346208
extern s32 lbl_803463B8[];   // 0x803463B8
extern f32 lbl_803463F8[];   // 0x803463F8
extern f32 lbl_80346408[];   // 0x80346408
extern f32 lbl_80346CC0[];   // 0x80346CC0
extern u32 lbl_80347DB8[];   // 0x80347DB8 (5 pairs: name hash, record id)
extern u8  lbl_803C4298[];   // 0x803C4298 (+0x38 = controller)
extern f32 lbl_805FDD78;     // 0.0
extern f32 lbl_805FDD80;     // -1.0
extern f32 lbl_805FDD90;     // 2/pi
extern f32 lbl_805FDF18;     // 0.0
extern f32 lbl_805FDF1C;     // 1.0
extern f32 lbl_805FDF54;     // 2.0
extern f32 lbl_805FDF60;     // 0.5
extern f32 lbl_805FDF64;     // 50.0
extern f32 lbl_805FDF68;     // -0.0001
extern f32 lbl_805FDF6C;     // 0.0001
extern f32 lbl_805FDF70;     // 0.25
extern f32 lbl_805FE108;     // 0.0
extern f32 lbl_805FE130;     // scale applied to the lbl_80346208 table
extern f32 lbl_805FE348;     // 0.0
extern f32 lbl_805FE354;     // 2.0
extern f32 lbl_805FE368;     // 0.4
extern f32 lbl_805FE36C;     // 0.6
extern f32 lbl_805FE370;     // 144.0
extern f32 lbl_805FE374;     // 12.0
}

extern s32 gScreenWidth;     // 0x805F9388
extern s32 gScreenHeight;    // 0x805F938C
extern "C++" void* gEventMgr;   // 0x805FB6C8

typedef u8* (*GetFn)(u8*);
typedef void (*SetFn)(u8*, s32);

// Marks the render object's transform dirty (same four flag ors the original does after each write).
static inline void MarkXformDirty(u8* t) {
    *(u32*)(t + 0x20) |= 0x8000;
    *(u32*)(t + 0x20) |= 0x10000000;
    *(u32*)(t + 0x20) |= 0x4000;
    *(u32*)(t + 0x20) |= 0x10000000;
}

// Copies the owner's position, then its rotation, onto the attached render object `o`.
static void CopyPlacement(u8* owner, u8* o) {
    u8* src = *(u8**)(owner + 0x4);
    u8* dst = *(u8**)(o + 0x4);
    *(f32*)(dst + 0x0) = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4) = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x8) = *(f32*)(src + 0x8);
    MarkXformDirty(dst);
    src = *(u8**)(owner + 0x4);
    dst = *(u8**)(o + 0x4);
    *(f32*)(dst + 0xC)  = *(f32*)(src + 0xC);
    *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
    *(f32*)(dst + 0x14) = *(f32*)(src + 0x14);
    *(f32*)(dst + 0x18) = *(f32*)(src + 0x18);
    MarkXformDirty(dst);
}

extern "C" {

// 0x8009DA98
// Turns the aim quat by the difficulty-scaled spin rate (+0x274) and, while the aim is active,
// refreshes the length stored in each matching record of the owner's table.
void fn_8009DA98(u8* arg0, f32 dt) {
    fn_800C5F5C();
    u8* sceneObj = *(u8**)(fn_8006FBDC() + 0x198);
    if (!(*(f32*)(sceneObj + 0x3EC) >= lbl_805FDD78)) {
        f32 rate = *(f32*)(arg0 + 0x274);
        *(u32*)(arg0 + 0x15C) &= ~1u;
        // RaySceneTest argument is a leftover register; the function ignores it.
        f32 lo = lbl_803463F8[RaySceneTest(NULL)];
        f32 span = lbl_80346408[RaySceneTest(NULL)] - lo;
        if (rate < lbl_805FDD78) span *= lbl_805FDD80;
        s32 cur = *(s32*)(*(u8**)(arg0 + 0x26C) + 0xE0);
        f32 den = (f32)lbl_803463B8[RaySceneTest(NULL)];
        u8* xf = *(u8**)(arg0 + 0x4);
        Quat p;
        p.x = *(f32*)(xf + 0xC);
        p.y = *(f32*)(xf + 0x10);
        p.z = *(f32*)(xf + 0x14);
        p.w = *(f32*)(xf + 0x18);
        rate += span * ((f32)cur / den);
        f32 angle = lbl_805FDD90 * (rate * dt);
        Vec3 axis = QuatGetAxisY(&p);
        Quat q;
        QuatFromAxisAngle(&q, &axis, angle);
        // r = q * p
        f32 w = ((q.w * p.w - q.x * p.x) - q.y * p.y) - q.z * p.z;
        f32 x = (q.y * p.z + (q.w * p.x + q.x * p.w)) - q.z * p.y;
        f32 y = q.z * p.x + (q.y * p.w + (q.w * p.y - q.x * p.z));
        f32 z = q.z * p.w + ((q.w * p.z + q.x * p.y) - q.y * p.x);
        xf = *(u8**)(arg0 + 0x4);
        *(f32*)(xf + 0xC)  = x;
        *(f32*)(xf + 0x10) = y;
        *(f32*)(xf + 0x14) = z;
        *(f32*)(xf + 0x18) = w;
        MarkXformDirty(*(u8**)(arg0 + 0x4));
    } else {
        *(u32*)(arg0 + 0x15C) |= 1;
    }

    if (*(u8*)(arg0 + 0x278) != 0) {
        u8* ret = (*(GetFn*)(*(u8**)arg0 + 0x10))(arg0);
        u8* tbl = *(u8**)ret;            // record table: +4 records (0x30 each), +8 count, +0xC key pairs
        u8* groups = *(u8**)(arg0 + 0x8C);
        for (u32 i = 0; i < *(u32*)(groups + 0x10); i++) {
            u8* group = *(u8**)(*(u8**)(groups + 0xC) + i * 4);
            for (u32 j = 0; j < *(u32*)(group + 0x28); j++) {
                u8* ent = *(u8**)(*(u8**)(group + 0x24) + j * 4);
                if (*(s32*)(ent + 0xC) != 0) continue;
                s32 hash = *(s32*)(*(u8**)(ent + 0x8) + 0x8);
                if (hash == 0) continue;
                FormatHashString((u32)hash);
                for (s32 k = 0; k < 5; k++) {
                    u32* row = lbl_80347DB8 + k * 2;
                    u8* info = *(u8**)(ent + 0x8);
                    if (*(u32*)(info + 0x8) != row[0]) continue;
                    s32 n = *(s32*)(tbl + 0x8);
                    s32 idx = -1;
                    for (s32 m = 0; m < n; m++) {
                        if ((s32)row[1] == *(s32*)(*(u8**)(tbl + 0xC) + m * 8)) { idx = m; break; }
                    }
                    if (idx < 0) continue;
                    u8* recs = *(u8**)(tbl + 0x4);
                    u8* a = recs + idx * 0x30;
                    u8* b = recs + *(s32*)(ent + 0x34) * 0x30;
                    f32 dx = *(f32*)(b + 0x24) - *(f32*)(a + 0x24);
                    f32 dy = *(f32*)(b + 0x28) - *(f32*)(a + 0x28);
                    f32 dz = *(f32*)(b + 0x2C) - *(f32*)(a + 0x2C);
                    f32 len2 = dz * dz + (dx * dx + dy * dy);
                    f32 len = (len2 <= lbl_805FDD78) ? lbl_805FDD78 : sqrtf(len2);   // original: frsqrte, two Newton steps
                    *(f32*)(info + 0x24) = len;
                }
            }
        }
    }
}

// 0x800A9B9C
// Per-frame update of the aimed weapon: ticks the cooldown, eases the aim offset (+0x724) toward
// its target, projects the aim point to the screen, tints the reticle and handles fire/reload input.
void fn_800A9B9C(u8* arg0, f32 dt) {
    *(f32*)(arg0 + 0x15C) = *(f32*)(arg0 + 0x15C) - dt;
    if (*(u8*)(arg0 + 0x2AC) != 0) {
        f32 v = *(f32*)(arg0 + 0x2B0);
        if (v > lbl_805FDF18) *(f32*)(arg0 + 0x224) = v;
    }
    u8 aimLocked = *(u8*)(arg0 + 0x2AC);
    if (aimLocked != 0) {
        f32 old = *(f32*)(arg0 + 0x724);
        f32 lo, hi;
        f32 v;
        if (old >= lbl_805FDF18) {
            // RaySceneTest argument is a leftover register; the function ignores it.
            v = *(f32*)(arg0 + 0x724) - lbl_80346CC0[RaySceneTest(NULL)];
            lo = lbl_805FDF18;
            hi = lbl_805FDF64;
        } else {
            f32 top = lbl_805FDF64;
            v = *(f32*)(arg0 + 0x724) + lbl_80346CC0[RaySceneTest(NULL)];
            lo = -top;
            hi = lbl_805FDF18;
        }
        *(f32*)(arg0 + 0x724) = v;
        *(f32*)(arg0 + 0x724) = (v < lo) ? lo : ((v > hi) ? hi : v);
        f32 diff = old - *(f32*)(arg0 + 0x724);
        s32 settled = 0;
        if (diff > lbl_805FDF68 && diff < lbl_805FDF6C) settled = 1;
        if (settled == 0) aimLocked = 0;
    }

    u8* ui = *(u8**)(arg0 + 0x6A8);
    f32 halfX = *(f32*)(ui + 0x74) / lbl_805FDF54;
    f32 halfY = *(f32*)(ui + 0x78) / lbl_805FDF54;
    f32 ax = *(f32*)(arg0 + 0x1F0);
    f32 ay = *(f32*)(arg0 + 0x1F4);
    f32 hw = lbl_805FDF60 * (f32)gScreenWidth;
    f32 px = *(f32*)(arg0 + 0x210) * ax;
    f32 py = *(f32*)(arg0 + 0x214) * ay;
    f32 hh = lbl_805FDF60 * (f32)gScreenHeight;
    f32 sx = hw + hw * px;
    f32 sy = hh + hh * py;
    *(f32*)(ui + 0x68) = sx - halfX;
    *(f32*)(ui + 0x6C) = sy - halfY;
    *(f32*)(ui + 0x70) = lbl_805FDF1C;

    u8* cam = fn_80073454();
    f32 pt[3];
    f32 hit[3];
    pt[0] = *(f32*)(arg0 + 0x6D0);
    pt[1] = *(f32*)(arg0 + 0x6D4);
    pt[2] = *(f32*)(arg0 + 0x6D8);
    fn_80073F58(hit, cam, pt, (f32*)(arg0 + 0x6B8), (f32*)(arg0 + 0x6C4), sx, sy);
    *(f32*)(arg0 + 0x6AC) = hit[0];
    *(f32*)(arg0 + 0x6B0) = hit[1];
    *(f32*)(arg0 + 0x6B4) = hit[2];

    if (*(f32*)(arg0 + 0x15C) > lbl_805FDF18) {
        // cooling down: dim reticle
        u8* t = *(u8**)(arg0 + 0x6A8);
        *(f32*)(t + 0x80) = lbl_805FDF70;
        *(f32*)(t + 0x84) = lbl_805FDF70;
        *(f32*)(t + 0x88) = lbl_805FDF70;
        *(f32*)(t + 0x8C) = lbl_805FDF1C;
    } else {
        u8* t = *(u8**)(arg0 + 0x6A8);
        *(f32*)(t + 0x80) = lbl_805FDF1C;
        *(f32*)(t + 0x84) = lbl_805FDF1C;
        *(f32*)(t + 0x88) = lbl_805FDF1C;
        *(f32*)(t + 0x8C) = lbl_805FDF1C;
        s32 pad = *(s32*)(lbl_803C4298 + 0x38);
        if (fn_800B0104(*(u8**)(arg0 + 0xB0)) != 0) {
            f32 go = lbl_805FDF18;
            if (fn_800B1464(*(u8**)(arg0 + 0xB4)) == 0) {
                if (fn_80147554(pad * 16 + 0x1039) != 0 || fn_80147554(pad * 16 + 0x108A) != 0) go = lbl_805FDF1C;
            }
            if (go > lbl_805FDF18) fn_800AFB64(*(u8**)(arg0 + 0xB0));
        } else if (fn_80147554(pad * 16 + 0x1039) != 0 || fn_80147554(pad * 16 + 0x108A) != 0) {
            fn_800AFBCC();   // the passed weapon pointer is ignored by the callee
        }
        pad = *(s32*)(lbl_803C4298 + 0x38);
        f32 go = lbl_805FDF18;
        if (fn_80147554(pad * 16 + 0x103A) != 0 || aimLocked != 0) go = lbl_805FDF1C;
        if (go > lbl_805FDF18) fn_800B0CFC(*(u8**)(arg0 + 0xB4));
    }
    u8* t = *(u8**)(arg0 + 0x6A8);
    (*(SetFn*)(*(u8**)(t + 0x138) + 0x2C))(t, 0x38);
}

// 0x800AFBEC
// Keeps the three attached effect objects (+0x288/+0x28C/+0x290) on the owner, spawns the queued
// effect (+0x294) into a free slot (+0x298..) and ticks the active slots.
void fn_800AFBEC(u8* arg0, f32 dt) {
    u8* o = *(u8**)(arg0 + 0x288);
    if (o != NULL) CopyPlacement(arg0, o);
    o = *(u8**)(arg0 + 0x28C);
    if (o != NULL) CopyPlacement(arg0, o);
    o = *(u8**)(arg0 + 0x290);
    if (o != NULL) CopyPlacement(arg0, o);

    if (*(s32*)(arg0 + 0x294) != 0) {
        for (s32 i = 0; i < 5 && *(s32*)(arg0 + 0x294) != 0; i++) {
            u8* slot = arg0 + i * 4;
            if (*(s32*)(slot + 0x298) != 0) continue;
            u8* cam = fn_8006FBDC();
            *(void**)(slot + 0x298) = *(void**)(arg0 + 0x294);
            u8* tbl = arg0 + 0x94;
            u8* entry = *(u8**)(tbl + 0x4) + fn_801EC880(tbl, 0xCD7726D5u) * 0xC8;
            f32 pt[3];
            f32 out[3];
            pt[0] = *(f32*)(entry + 0x5C);
            pt[1] = *(f32*)(entry + 0x60);
            pt[2] = *(f32*)(entry + 0x64);
            fn_800AA68C((u8*)out, cam, entry);
            fn_800AB5F8(*(u8**)(arg0 + 0x294), (u8*)pt, (u8*)out);
            *(void**)(arg0 + 0x294) = NULL;
        }
    }
    for (s32 i = 0; i < 5; i++) {
        u8* eff = *(u8**)(arg0 + i * 4 + 0x298);
        if (eff != NULL) fn_800AB468(eff, dt);
    }
}

// 0x800B0D08
// Round-slot bookkeeping while the loader state (+0x298) is 1: pulls a ready round into the
// current slot, shifts the loaded rounds along, clears the state once all five are filled, then
// keeps every loaded/queued round object on the owner's transform.  (Takes no float argument.)
void fn_800B0D08(u8* arg0) {
    if (*(s32*)(arg0 + 0x298) == 1) {
        if (*(s32*)(arg0 + 0x2A0) == 0 && *(s32*)(arg0 + 0x2B4) == 0) {
            s32 first;
            if (*(s32*)(arg0 + 0x2C8) != 0) first = 0;
            else if (*(s32*)(arg0 + 0x2CC) != 0) first = 1;
            else if (*(s32*)(arg0 + 0x2D0) != 0) first = 2;
            else if (*(s32*)(arg0 + 0x2D4) != 0) first = 3;
            else if (*(s32*)(arg0 + 0x2D8) != 0) first = 4;
            else first = -1;
            u8* src = arg0 + first * 4;
            *(void**)(arg0 + 0x2B4) = *(void**)(src + 0x2C8);
            *(void**)(src + 0x2C8) = NULL;
            fn_800A7D88(*(void**)(arg0 + 0x2B4), 0xA61CF023u);
        }
        for (s32 s = 3; s >= 0; s--) {
            u8* q = arg0 + s * 4;
            if (*(s32*)(q + 0x2A0) != 0 && *(s32*)(q + 0x2A4) == 0 && *(s32*)(q + 0x2B8) == 0) {
                if (s + 1 == 4) fn_800A7D88(arg0, 0xA61CF023u);
                fn_800A7D88(*(void**)(q + 0x2A0), 0xA61CF023u);
                *(void**)(q + 0x2B8) = *(void**)(q + 0x2A0);
                *(void**)(q + 0x2A0) = NULL;
            }
        }
        if (*(s32*)(arg0 + 0x2A0) != 0 && *(s32*)(arg0 + 0x2A4) != 0 && *(s32*)(arg0 + 0x2A8) != 0 &&
            *(s32*)(arg0 + 0x2AC) != 0 && *(s32*)(arg0 + 0x2B0) != 0) {
            *(s32*)(arg0 + 0x298) = 0;
        }
    }
    for (s32 i = 0; i < 5; i++) {
        u8* slot = arg0 + i * 4;
        u8* round = *(u8**)(slot + 0x2A0);
        if (round != NULL && i < 4) CopyPlacement(arg0, round);
        u8* queued = *(u8**)(slot + 0x2B4);
        if (queued != NULL) CopyPlacement(arg0, queued);
    }
}

// 0x800B4664
// Ticks the five hold-slot timers (+0xA8 object, +0xB0 hold timer, +0xB4 release timer), then
// picks the slot whose value is closest to the current target (within the difficulty-scaled
// window) and pushes the per-slot state to the owner's table at +0x94.
void fn_800B4664(u8* arg0, f32 dt) {
    for (s32 i = 0; i < 5; i++) {
        u8* s = arg0 + i * 0x14;
        if (*(u8**)(s + 0xA8) == NULL) continue;
        f32 hold = *(f32*)(s + 0xB0);
        if (hold <= lbl_805FE108) {
            f32 rel = *(f32*)(s + 0xB4) - dt;
            *(f32*)(s + 0xB4) = rel;
            if (rel <= lbl_805FE108) fn_800B387C(arg0, i);
            else fn_800B999C(*(u8**)(s + 0xA8), 0, 0);
        } else {
            *(f32*)(s + 0xB0) = hold - dt;
            fn_800B999C(*(u8**)(s + 0xA8), 1, 0);
        }
    }
    u8* tbl = *(u8**)(arg0 + 0x94);
    f32 target = fn_800B68B8(tbl);
    s32 bestId = -1;
    s32 bestSlot = -1;
    // RaySceneTest argument is a leftover register; the function ignores it.
    s32 level = RaySceneTest(NULL);
    if (level < 0) level = 0;
    else if (level > 3) level = 3;
    f32 best = lbl_805FE130 * lbl_80346208[level];
    for (s32 j = 0; j < 5; j++) {
        u8* s = arg0 + j * 0x14;
        u8* obj = *(u8**)(s + 0xA8);
        if (obj != NULL) {
            fn_800B9138(obj, dt);
            if (fn_800B9A78(*(u8**)(s + 0xA8)) != 0) {
                f32 d = (f32)fabs(fn_800B9914(*(u8**)(s + 0xA8)) - target);
                if (d < best) {
                    best = d;
                    bestId = fn_800B964C(*(u8**)(s + 0xA8));
                    bestSlot = j;
                }
            }
            if (fn_800B998C(*(u8**)(s + 0xA8)) != 0 || fn_800B964C(*(u8**)(s + 0xA8)) == 1) {
                fn_800B7428(*(u8**)(arg0 + 0x94), fn_800B964C(*(u8**)(s + 0xA8)), j);
            }
        }
        fn_800B66E0(*(u8**)(arg0 + 0x94), arg0 + 0xA8 + j * 0x14, j);
    }
    fn_800B665C(*(u8**)(arg0 + 0x94), bestId);
    fn_800B66D8(*(u8**)(arg0 + 0x94), bestSlot);
}

// 0x800C0168
// Bounce response for the active shape (index at +0): asks the shape for its contact normal,
// sends the hit/miss event to the matching listener, reflects the direction (arg2) about the
// normal into arg4 and blends in the shape's travel vector (+0x140..+0x154), capping the
// result at length 12.  Returns 1 when a contact was found.  *arg5 is set to 1 for shape 3.
s32 fn_800C0168(u8* arg0, u8* arg1, u8* arg2, u8* arg3, u8* arg4, u8* arg5) {
    if (arg1 == NULL) return 0;
    *arg5 = 0;
    if (*(s32*)arg0 == 3) *arg5 = 1;
    u8 front = 1;
    Vec3 n;
    n.z = lbl_805FE348;
    n.y = lbl_805FE348;
    n.x = lbl_805FE348;
    if (fn_800C0C30(arg0 + *(s32*)arg0 * 0x48 + 0x14, arg1, arg3, &n.x, &front) == 0) return 0;

    void* listener = *(void**)(arg0 + *(s32*)arg0 * 4 + 0x4);
    if (front != 0) EventSend(gEventMgr, listener, NULL, 0xD69D8CB9u, 0, 0, 1, 0);
    else            EventSend(gEventMgr, listener, NULL, 0xFE41E291u, 0, 0, 1, 0);

    f32* d = (f32*)arg2;
    f32* out = (f32*)arg4;
    f32 dot = n.z * d[2] + (n.x * d[0] + n.y * d[1]);
    f32 px = n.x * dot;
    f32 py = n.y * dot;
    f32 pz = n.z * dot;
    f32 rx = d[0] - px * lbl_805FE354;
    f32 ry = d[1] - py * lbl_805FE354;
    f32 rz = d[2] - pz * lbl_805FE354;
    out[0] = rx;
    out[1] = ry;
    out[2] = rz;

    Vec3 v;
    v.x = *(f32*)(arg0 + 0x14C) - *(f32*)(arg0 + 0x140);
    v.y = *(f32*)(arg0 + 0x150) - *(f32*)(arg0 + 0x144);
    v.z = *(f32*)(arg0 + 0x154) - *(f32*)(arg0 + 0x148);
    if (front == 0) {
        v.x = -v.x;
        v.y = -v.y;
        v.z = -v.z;
    }
    f32 lv2 = v.z * v.z + (v.x * v.x + v.y * v.y);
    f32 lenV = (lv2 <= lbl_805FE348) ? lbl_805FE348 : sqrtf(lv2);   // original: frsqrte, two Newton steps
    f32 lo2 = out[2] * out[2] + (out[0] * out[0] + out[1] * out[1]);
    f32 lenO = (lo2 <= lbl_805FE348) ? lbl_805FE348 : sqrtf(lo2);
    if (lenV / (lenV + lenO) <= lbl_805FE368) {
        out[0] = out[0] + v.x;
        out[1] = out[1] + v.y;
        out[2] = out[2] + v.z;
    } else {
        f32 scale = (lbl_805FE368 * lenO) / lbl_805FE36C;
        Vec3 vn = Vec3Normalized(v);
        out[0] = out[0] + vn.x * scale;
        out[1] = out[1] + vn.y * scale;
        out[2] = out[2] + vn.z * scale;
    }
    f32 l2 = out[2] * out[2] + (out[0] * out[0] + out[1] * out[1]);
    if (l2 > lbl_805FE370) {
        Vec3 on = Vec3Normalized(Vec3{ out[0], out[1], out[2] });
        out[0] = on.x * lbl_805FE374;
        out[1] = on.y * lbl_805FE374;
        out[2] = on.z * lbl_805FE374;
    }
    return 1;
}

}  // extern "C"
