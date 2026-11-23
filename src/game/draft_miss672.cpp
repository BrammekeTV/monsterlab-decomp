// Drafts miss672: hit-sweep for the thrown prop (0x80068450), effect spawn / script event handler
// (0x800584D0, 0x8006E600), monster spawn-position pick (0x80085CF8), HUD / timer updates
// (0x8008853C, 0x80088BC0). Verified against the asm (tools/dump.sh).
#include <math.h>

#include "types.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C++" void* gEventMgr;            // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;      // 0x805FB94C

typedef s32 (*ShapeFn)(u8*, u8*);

extern "C" {
void*  EventPost(s32 id, void* sender);                                                    // 0x801F3918
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
u8*    GetGlobalSettings(void);                                                            // 0x801D2BF8
BOOL   GetPropCurrentState(u8* prop);                                                      // 0x8006BB24 (asm reads a pointer; old decl takes u32)
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);                              // 0x80209E70
u32    RandomU32(void);                                                                    // 0x80143460
s32    RaySceneTest(void* ray);                                                            // 0x80070458 (ignores its argument)
void   ScopedLockCtor(ScopedLock* l);                                                      // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                                           // 0x801F53B0
ScriptValue SetPropState(u8* prop, u32 state, s32 a2);                                     // src/game/draft_miss2.cpp
s32    SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);               // 0x801C80E0
int    SysSprintf(char* buf, const char* fmt, ...);                                        // 0x80142EF8
void   UISetProgress(void* obj, f32 t);
void   UISetText(void* el, const char* text, s32 a);                                       // 0x8019EFC8
s32    fn_80067C30(u8* obj, u8* ctx);                                                      // 0x80067C30 (shape callback)
void   fn_8006EF78(u8* p, u8* out);                                                        // 0x8006EF78
void*  fn_80070F4C(void);                                                                  // 0x80070F4C
void   fn_80081004(u8* out, u8* obj, s32 idx, f32 t);                                      // 0x80081004
void   fn_8008BA14(u8* p);                                                                 // 0x8008BA14 (caller also sets f1 = dt, unused)
void   fn_8008CABC(void);                                                                  // 0x8008CABC (leaf, ignores r3)
void   fn_8008CAC0(u8* p, f32 v);                                                          // 0x8008CAC0
f32    fn_8008CDFC(u8* a3, u8* a4, f32 f1);                                                // 0x8008CDFC (a4 is a pointer, old decl s32)
void   fn_8008E5E8(u8* a, u8* b);                                                          // 0x8008E5E8 (signature from the call sites)
s32    fn_800893C4(u8* arg0, s32 arg1, s32 arg2);                                          // 0x800893C4
void   fn_80121EC0(u8* a, u8* b, f32* v);                                                  // 0x80121EC0 (signature from the call site)
void   fn_801458B0(f32* quat, f32* a, f32* b);                                             // 0x801458B0
f32    fn_801A4084(u8* a3);                                                                // 0x801A4084
void   fn_801B154C(u8* group, u32 key, ShapeFn fn, u8* arg);                               // 0x801B154C
u8*    fn_801B161C(u8* p, u32 key);                                                        // 0x801B161C
void   fn_801B1778(void* p, s32 a);                                                        // 0x801B1778
void*  fn_801B18E4(u8* list);                                                              // 0x801B18E4
void   fn_801B5D94(u8* obj, u8* event);                                                    // 0x801B5D94
u8*    fn_801BE620(u8* arg0, u32 arg1, f32* arg2, f32* arg3, u32 arg4, s32 arg5);          // 0x801BE620
void   fn_801BF398(u8* list, u8* item);                                                    // 0x801BF398
void   fn_801C849C(u8* mgr, u8* item);                                                     // 0x801C849C
void*  fn_801D40E0(s32 n);                                                                 // 0x801D40E0
void*  fn_80209814(void* v);                                                               // 0x80209814
f64    floor(f64 x);                                                                 // 0x8033ED6C (floor)

extern u8   gWorldState[];            // 0x803E6D00
extern u8   lbl_8047C760[];           // 0x8047C760 (effect manager)
extern u8   lbl_8047C790[];           // 0x8047C790 (effect list)
extern void* lbl_805FBBAC;            // 0x805FBBAC (voice SFX handle)

extern f32  lbl_803466F0[];           // 0x803466F0
extern f32  lbl_80346898[];           // 0x80346898
extern f32  lbl_803468A8[];           // 0x803468A8
extern s32  lbl_803468B8[];           // 0x803468B8
extern s32  lbl_803468C8[];           // 0x803468C8
extern f32  lbl_80346968[];           // 0x80346968
extern f32  lbl_80346978[];           // 0x80346978
extern s32  lbl_80346988[];           // 0x80346988
extern s32  lbl_80346998[];           // 0x80346998
extern f32  lbl_803469C8[];           // 0x803469C8
extern f32  lbl_803469D8[];           // 0x803469D8
extern f32  lbl_803469E8[];           // 0x803469E8
extern u32  lbl_80347D98[];           // 0x80347D98

extern f32 lbl_805FD300;              // 0x805FD300 (0.0)
extern f32 lbl_805FD304;              // 0x805FD304 (1.0)
extern f32 lbl_805FD368;              // 0x805FD368 (0.5)
extern f32 lbl_805FD378;              // 0x805FD378 (0.0001)
extern f32 lbl_805FD37C;              // 0x805FD37C (-0.0001)
extern f32 lbl_805FD468;              // 0x805FD468 (0.0)
extern f32 lbl_805FD480;              // 0x805FD480 (0.5)
extern f32 lbl_805FD498;              // 0x805FD498 (-1.0)
extern f32 lbl_805FD598;              // 0x805FD598 (0.0)
extern f32 lbl_805FD5A0;              // 0x805FD5A0 (0.0001)
extern f32 lbl_805FD5A4;              // 0x805FD5A4 (-0.0001)
extern f32 lbl_805FD5A8;              // 0x805FD5A8 (1.0)
extern f32 lbl_805FD8C8;              // 0x805FD8C8 (0.0)
extern f32 lbl_805FD8E4;              // 0x805FD8E4 (2.3283064e-10, 1 / 2^32)
extern f32 lbl_805FD8FC;              // 0x805FD8FC (2.999)
extern f32 lbl_805FD900;              // 0x805FD900 (0.9)
extern f32 lbl_805FD918;              // 0x805FD918 (10.0)
extern f32 lbl_805FD928;              // 0x805FD928 (0.0)
extern f32 lbl_805FD934;              // 0x805FD934 (1.0)
extern f32 lbl_805FD938;              // 0x805FD938 (0.05)
extern f32 lbl_805FD940;              // 0x805FD940 (0.5)
extern f32 lbl_805FD944;              // 0x805FD944 (100.0)
extern f32 lbl_805FD948;              // 0x805FD948 (0.2)
extern f32 lbl_805FD950;              // 0x805FD950 (0.1)
extern f32 lbl_805FD968;              // 0x805FD968 (2.3283064e-10, 1 / 2^32)
extern f32 lbl_805FD978;              // 0x805FD978 (0.02)
extern f32 lbl_805FD97C;              // 0x805FD97C (0.12)
extern f32 lbl_805FD980;              // 0x805FD980 (0.9)
}

static const u32 kHashLegs = 0xCB8DAE3Eu;   // "Legs"
static const u32 kHashBase = 0xF62113D5u;   // "BAse"

// Total number of zone slots: sum of the slot counts in the global settings.
static s32 ZoneSlotTotal(void) {
    s32 a = *(s32*)(GetGlobalSettings() + 0x4C);
    s32 b = *(s32*)(GetGlobalSettings() + 0x34);
    s32 c = *(s32*)(GetGlobalSettings() + 0x50);
    s32 sum = b + a;
    s32 d = *(s32*)(GetGlobalSettings() + 0x54);
    sum = c + sum;
    s32 e = *(s32*)(GetGlobalSettings() + 0x58);
    sum = d + sum;
    s32 f = *(s32*)(GetGlobalSettings() + 0x5C);
    sum = e + sum;
    s32 g = *(s32*)(GetGlobalSettings() + 0x60);
    sum = f + sum;
    s32 h = *(s32*)(GetGlobalSettings() + 0x64);
    sum = g + sum;
    s32 i = *(s32*)(GetGlobalSettings() + 0x38);
    sum = h + sum;
    s32 j = *(s32*)(GetGlobalSettings() + 0x3C);
    sum = i + sum;
    s32 k = *(s32*)(GetGlobalSettings() + 0x40);
    sum = j + sum;
    s32 l = *(s32*)(GetGlobalSettings() + 0x44);
    sum = k + sum;
    s32 m = *(s32*)(GetGlobalSettings() + 0x48);
    return m + (l + sum);
}

// Difficulty tier: RaySceneTest result clamped to 0..3 (inlined at every use).
static s32 TierClamped(void) {
    s32 i = RaySceneTest(NULL);   // r3 is leftover in the asm, the callee ignores it
    if (i < 0) i = 0;
    else if (i > 3) i = 3;
    return i;
}

// HUD bar alpha: 0.1 + 0.9 * fn_801A4084(el), clamped to 0.1..1.0 (the inlined clamp calls
// fn_801A4084 again for each test).
static f32 BarAlpha(u8* el) {
    f32 lo = lbl_805FD950;
    if (lbl_805FD950 + lbl_805FD980 * fn_801A4084(el) < lo) return lo;
    f32 hi = lbl_805FD934;
    if (lbl_805FD950 + lbl_805FD980 * fn_801A4084(el) > hi) return hi;
    return lbl_805FD950 + lbl_805FD980 * fn_801A4084(el);
}

extern "C" {

// 0x800584D0
// Impact response: spawns the hit effect midway between the two contact points (arg1+0x58 and
// +0x68), orients it along the contact direction, rotates the reference vector by the
// contact-normal quaternion (or fn_80121EC0), cancels the component of the velocity at
// arg0+0x844 pointing into the contact, sends event 0xC8B44BEE and calls two virtuals.
// Always returns 1.
s32 fn_800584D0(u8* arg0, u8* arg1) {
    typedef void (*Fn54)(u8*, u8*, f32*);
    typedef void (*Fn50)(u8*, u8*, s32, s32, f32*, f32);

    f32 v[3];
    v[0] = lbl_805FD300;
    v[1] = lbl_805FD304;
    v[2] = lbl_805FD300;
    u8* tgt = *(u8**)(arg1 + 0x5C);
    if (tgt == NULL) return 1;
    u8* info = *(u8**)(tgt + 0x88);
    if (info == NULL) return 1;

    u8* ptB = *(u8**)(arg1 + 0x68);
    u8* ptA = *(u8**)(arg1 + 0x58);
    s32 isZero = 0;
    f32 sx = *(f32*)(ptA + 0x14) + *(f32*)(ptB + 0x14);
    f32 sy = *(f32*)(ptA + 0x18) + *(f32*)(ptB + 0x18);
    f32 sz = *(f32*)(ptA + 0x1C) + *(f32*)(ptB + 0x1C);
    u8* fx = NULL;
    f32 mx = sx * lbl_805FD368;
    f32 my = sy * lbl_805FD368;
    f32 mz = sz * lbl_805FD368;
    f32 pos[4];
    pos[0] = mx;
    pos[1] = my;
    pos[2] = mz;
    pos[3] = lbl_805FD300;
    SpawnEffectAt(lbl_8047C760, *(u32*)(info + 0x58), pos, &fx);
    if (fx != NULL) {
        u8* c = *(u8**)(arg1 + 0x68);
        f32 d[4];
        d[0] = mx - *(f32*)(c + 0x14);
        d[1] = my - *(f32*)(c + 0x18);
        d[2] = mz - *(f32*)(c + 0x1C);
        d[3] = lbl_805FD300;
        if (d[0] <= lbl_805FD378 && d[0] >= lbl_805FD37C &&
            d[1] <= lbl_805FD378 && d[1] >= lbl_805FD37C &&
            d[2] <= lbl_805FD378 && d[2] >= lbl_805FD37C &&
            d[3] <= lbl_805FD378 && d[3] >= lbl_805FD37C) {
            isZero = 1;
        }
        s32 n = *(s32*)(fx + 0xC);
        if (isZero != 0) {
            // Direction is ~0: orient the effect nodes along +Z.
            for (s32 i = 0; i < n; i++) {
                u8* node = *(u8**)(fx + i * 4 + 0x14);
                if (node != NULL) {
                    *(f32*)(node + 0x30) = lbl_805FD300;
                    *(f32*)(node + 0x34) = lbl_805FD300;
                    *(f32*)(node + 0x38) = lbl_805FD304;
                    *(f32*)(node + 0x3C) = lbl_805FD300;
                }
            }
        } else {
            for (s32 i = 0; i < n; i++) {
                u8* node = *(u8**)(fx + i * 4 + 0x14);
                if (node != NULL) {
                    *(f32*)(node + 0x30) = d[0];
                    *(f32*)(node + 0x34) = d[1];
                    *(f32*)(node + 0x38) = d[2];
                    *(f32*)(node + 0x3C) = d[3];
                }
            }
        }
        u8** owner = *(u8***)fx;
        if (owner != NULL) {
            *owner = NULL;
            *(u8**)fx = NULL;
        }
    }

    s32 slot = *(s32*)(*(u8**)(arg0 + 0x9C) + 0x1EC);
    if (slot != -1) {
        u8* e = info + slot * 0xC;
        v[0] = *(f32*)(info + slot * 0xC);
        v[1] = *(f32*)(e + 0x4);
        v[2] = *(f32*)(e + 0x8);
    }
    if (*(s32*)(*(u8**)(tgt + 0x4) + 0x1C) & 0x40) {
        // Rotate v by the quaternion that takes +Z to the contact direction.
        f32 x = *(f32*)(arg1 + 0xC);
        f32 y = *(f32*)(arg1 + 0x10);
        f32 z = *(f32*)(arg1 + 0x14);
        f32 dir[3];
        f32 len2 = z * z + (x * x + y * y);
        if (lbl_805FD300 == len2) {
            dir[0] = lbl_805FD300;
            dir[1] = lbl_805FD300;
            dir[2] = lbl_805FD300;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            dir[0] = x * inv;
            dir[1] = y * inv;
            dir[2] = z * inv;
        }
        f32 up[3];
        up[0] = lbl_805FD300;
        up[1] = lbl_805FD300;
        up[2] = lbl_805FD304;
        f32 q[4];
        fn_801458B0(q, dir, up);
        f32 qx = q[0];
        f32 qy = q[1];
        f32 qz = q[2];
        f32 qw = q[3];
        f32 nqx = -qx;
        f32 nqz = -qz;
        f32 nqy = -qy;
        f32 vx = v[0];
        f32 vy = v[1];
        f32 vz = v[2];
        f32 zero = lbl_805FD300;
        // P = q * (v, 0)
        f32 p0 = (qy * vz + (qw * vx + qx * zero)) - qz * vy;
        f32 p1 = qz * vx + (qy * zero + (qw * vy - qx * vz));
        f32 p2 = qz * zero + ((qw * vz + qx * vy) - qy * vx);
        f32 p3 = ((qw * zero - qx * vx) - qy * vy) - qz * vz;
        // v = P * conj(q)
        v[0] = (p1 * nqz + (p3 * nqx + p0 * qw)) - p2 * nqy;
        v[1] = p2 * nqx + (p1 * qw + (p3 * nqy - p0 * nqz));
        v[2] = p2 * qw + ((p3 * nqz + p0 * nqy) - p1 * nqx);
    } else {
        fn_80121EC0(tgt, arg0, v);
    }

    // Remove the part of the velocity at arg0+0x844 that points along the contact direction.
    f32 nx = *(f32*)(arg1 + 0xC);
    f32 ny = *(f32*)(arg1 + 0x10);
    f32 nz = *(f32*)(arg1 + 0x14);
    f32 dot = *(f32*)(arg0 + 0x84C) * nz +
              (*(f32*)(arg0 + 0x844) * nx + *(f32*)(arg0 + 0x848) * ny);
    if (dot < lbl_805FD300) {
        f32 ax = nx * dot;
        f32 ay = ny * dot;
        f32 az = nz * dot;
        *(f32*)(arg0 + 0x844) = *(f32*)(arg0 + 0x844) - ax;
        *(f32*)(arg0 + 0x848) = *(f32*)(arg0 + 0x848) - ay;
        *(f32*)(arg0 + 0x84C) = *(f32*)(arg0 + 0x84C) - az;
    }
    EventSend(gEventMgr, tgt, arg0, 0xC8B44BEEu, 0, 0, 1, 0);
    f32 hitPos[3];
    hitPos[0] = mx;
    hitPos[1] = my;
    hitPos[2] = mz;
    (*(Fn54*)(*(u8**)tgt + 0x54))(tgt, arg0, hitPos);
    (*(Fn50*)(*(u8**)arg0 + 0x50))(arg0, tgt, *(s32*)(info + 0x4C), *(s16*)(info + 0x54), v,
                                   *(f32*)(info + 0x48));
    return 1;
}

// 0x80068450
// Sweeps the thrown prop's path (previous position at +0x1A0, current at def +0) against every
// zone object with the shape callback fn_80067C30. The context block passed to the callback holds
// the (normalised) velocity direction, the swept sphere centre and radius, the owner (arg0),
// the object being tested and the best hit. Returns 1 and stores the hit (+0x24C, +0x14C,
// hit point at +0x1D0) unless the hit is a flagged non-hittable, else 0.
s32 fn_80068450(u8* arg0) {
    f32 ctx[16];
    u8* def = *(u8**)(arg0 + 4);
    f32 dx = *(f32*)(def + 0x0) - *(f32*)(arg0 + 0x1A0);
    f32 dy = *(f32*)(def + 0x4) - *(f32*)(arg0 + 0x1A4);
    f32 dz = *(f32*)(def + 0x8) - *(f32*)(arg0 + 0x1A8);
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 len = lbl_805FD468;
    if (!(len2 <= lbl_805FD468)) len = sqrtf(len2);

    f32 vx = *(f32*)(arg0 + 0x1AC);
    f32 vy = *(f32*)(arg0 + 0x1B0);
    f32 vz = *(f32*)(arg0 + 0x1B4);
    f32 dirX, dirY, dirZ;
    f32 vlen2 = vz * vz + (vx * vx + vy * vy);
    if (lbl_805FD468 == vlen2) {
        dirX = lbl_805FD468;
        dirY = lbl_805FD468;
        dirZ = lbl_805FD468;
    } else {
        f32 inv = 1.0f / sqrtf(vlen2);
        dirX = vx * inv;
        dirY = vy * inv;
        dirZ = vz * inv;
    }

    s32 mask = 0x50;
    f32 sumX = *(f32*)(def + 0x0) + *(f32*)(arg0 + 0x1A0);
    f32 radius = *(f32*)(*(u8**)(arg0 + 0x22C) + 0x3C) + len;
    f32 sumY = *(f32*)(def + 0x4) + *(f32*)(arg0 + 0x1A4);
    f32 sumZ = *(f32*)(def + 0x8) + *(f32*)(arg0 + 0x1A8);
    // Context block (PPC layout): +0 dir, +0xC sphere centre, +0x18 hit point, +0x24 owner,
    // +0x28 radius, +0x2C tested object, +0x30 best hit.
    *(f32*)((u8*)ctx + 0x0) = dirX;
    *(f32*)((u8*)ctx + 0x4) = dirY;
    *(f32*)((u8*)ctx + 0x8) = dirZ;
    *(f32*)((u8*)ctx + 0xC) = sumX * lbl_805FD480;
    *(f32*)((u8*)ctx + 0x10) = sumY * lbl_805FD480;
    *(f32*)((u8*)ctx + 0x14) = sumZ * lbl_805FD480;
    *(f32*)((u8*)ctx + 0x18) = lbl_805FD468;
    *(f32*)((u8*)ctx + 0x1C) = lbl_805FD468;
    *(f32*)((u8*)ctx + 0x20) = lbl_805FD468;
    *(u8**)((u8*)ctx + 0x24) = arg0;
    *(f32*)((u8*)ctx + 0x28) = radius;
    *(u8**)((u8*)ctx + 0x2C) = NULL;
    *(u8**)((u8*)ctx + 0x30) = NULL;

    u8* sweep = *(u8**)(arg0 + 0x22C);
    *(f32*)(sweep + 0x20) = *(f32*)(arg0 + 0x1A0);
    *(f32*)(sweep + 0x24) = *(f32*)(arg0 + 0x1A4);
    *(f32*)(sweep + 0x28) = *(f32*)(arg0 + 0x1A8);
    u8* def2 = *(u8**)(arg0 + 4);
    sweep = *(u8**)(arg0 + 0x22C);
    *(f32*)(sweep + 0x14) = *(f32*)(def2 + 0x0);
    *(f32*)(sweep + 0x18) = *(f32*)(def2 + 0x4);
    *(f32*)(sweep + 0x1C) = *(f32*)(def2 + 0x8);

    if (*(s32*)(arg0 + 0x244) & 0x8000) {
        u8* held = *(u8**)(arg0 + 0x248);
        if (held != NULL) {
            s32 hf = *(s32*)(*(u8**)(held + 0x4) + 0x1C);
            if (hf & 0x40) {
                mask = 0x40;
            } else if (hf & 0x10) {
                mask = 0x10;
            }
        }
    }

    f32 neg1 = lbl_805FD498;
    s32 slotOff = 0;
    for (s32 i = 0; i < ZoneSlotTotal() + 1; i++, slotOff += 0x30) {
        s32 zone = -1;
        if (i == ZoneSlotTotal()) {
            zone = 0;
        } else {
            u8* e = *(u8**)(gWorldState + 0xB0) + slotOff;
            if (*(s32*)(e + 0x4) == 6) zone = *(s32*)(e + 0x8);
        }
        if (zone < 0) continue;

        s32 off64 = 0;
        s32 off4 = 0;
        u8* zb = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
        s32 total = *(s32*)(zb + 0x94) + *(s32*)(zb + 0x98);
        for (s32 j = 0; j < total; j++, off64 += 0x40, off4 += 4) {
            zb = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
            s32 nFirst = *(s32*)(zb + 0x94);
            u8* obj;
            u8* ent;
            if (j < nFirst) {
                obj = *(u8**)(*(u8**)(zb + 0x120) + off4);
                ent = *(u8**)(zb + 0x8C) + off64;
            } else {
                s32 k = j - nFirst;
                obj = *(u8**)(*(u8**)(zb + 0x12C) + k * 4);
                ent = *(u8**)(zb + 0x90) + (k << 6);
            }
            if ((mask == 0 || (*(s32*)(ent + 0x1C) & mask)) && (*(s32*)(ent + 0x20) & 2) && obj != NULL) {
                if (obj == *(u8**)(arg0 + 0x23C)) {
                    // the prop itself
                } else if ((*(s32*)(arg0 + 0x244) & 0x8000) && *(u8**)(arg0 + 0x248) != NULL &&
                           *(u8**)(arg0 + 0x248) != obj) {
                    // only the held object may be hit
                } else {
                    u8* shape = *(u8**)(obj + 0x8C);
                    if (shape != NULL) {
                        f32 ox = *(f32*)(obj + 0x18);
                        f32 oy = *(f32*)(obj + 0x1C);
                        f32 oz = *(f32*)(obj + 0x20);
                        f32 ex = *(f32*)((u8*)ctx + 0xC) - ox;
                        f32 ey = *(f32*)((u8*)ctx + 0x10) - oy;
                        f32 ez = *(f32*)((u8*)ctx + 0x14) - oz;
                        f32 rr = *(f32*)((u8*)ctx + 0x28) + *(f32*)(obj + 0x14);
                        f32 d2 = ez * ez + (ex * ex + ey * ey);
                        if (!(d2 <= rr * rr)) d2 = neg1;
                        if (neg1 != d2) {
                            if (shape != NULL && (*(s32*)(*(u8**)(obj + 0x4) + 0x20) & 0x4000)) {
                                fn_801B1778(shape, 0);
                            }
                            *(u8**)((u8*)ctx + 0x2C) = obj;
                            fn_801B154C(*(u8**)(obj + 0x8C), kHashBase, fn_80067C30, (u8*)ctx);
                        }
                    }
                }
                // The asm always continues with the next entry here (the break path is dead).
            }
        }
    }

    u8* hit = *(u8**)((u8*)ctx + 0x30);
    if (hit != NULL) {
        if (((*(s32*)(*(u8**)(hit + 0x4) + 0x1C) & 0x40) && (*(s32*)(arg0 + 0x244) & 0x40)) ||
            (*(s32*)(*(u8**)(hit + 0x4) + 0x20) & 0x40000000)) {
            return 0;
        }
        *(u8**)(arg0 + 0x24C) = hit;
        *(s32*)(arg0 + 0x14C) = *(s32*)(*(u8**)(hit + 0x9C) + 0x60);
        *(f32*)(arg0 + 0x1D0) = *(f32*)((u8*)ctx + 0x18);
        *(f32*)(arg0 + 0x1D4) = *(f32*)((u8*)ctx + 0x1C);
        *(f32*)(arg0 + 0x1D8) = *(f32*)((u8*)ctx + 0x20);
        return 1;
    }
    return 0;
}

// 0x8006E600
// Script event handler for the effect events 11..14 (field 0 bits 20..26 select the event):
// 11 spawns effect arg1+4 at +0x194 and orients its nodes along +0x1A0, 12 removes it, 13 / 14
// create / remove the two list entries at +0x17C. Always ends with fn_801B5D94.
void fn_8006E600(u8* arg0, u8* arg1) {
    u32 kind = (*(u32*)arg1 >> 20) & 0x7F;
    switch (kind) {
    case 11: {
        s32 effect = *(s32*)(arg1 + 0x4);
        if (effect != 0) {
            if (!(*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
                u8* ev = (u8*)EventPost(0xB, arg0);
                if (ev != NULL) *(s32*)(ev + 0x4) = effect;
            }
            if (*(u8**)(arg0 + 0x178) == NULL) {
                ScopedLock lock;
                ScopedLockCtor(&lock);
                f32 pos[4];
                pos[0] = *(f32*)(arg0 + 0x194);
                pos[1] = *(f32*)(arg0 + 0x198);
                pos[2] = *(f32*)(arg0 + 0x19C);
                pos[3] = lbl_805FD598;
                SpawnEffectAt(lbl_8047C760, (u32)effect, pos, (u8**)(arg0 + 0x178));
                u8* fx = *(u8**)(arg0 + 0x178);
                if (fx != NULL) {
                    f32 d[4];
                    d[0] = *(f32*)(arg0 + 0x1A0);
                    d[1] = *(f32*)(arg0 + 0x1A4);
                    d[2] = *(f32*)(arg0 + 0x1A8);
                    d[3] = lbl_805FD598;
                    s32 isZero = 0;
                    if (d[0] <= lbl_805FD5A0 && d[0] >= lbl_805FD5A4 &&
                        d[1] <= lbl_805FD5A0 && d[1] >= lbl_805FD5A4 &&
                        d[2] <= lbl_805FD5A0 && d[2] >= lbl_805FD5A4 &&
                        d[3] <= lbl_805FD5A0 && d[3] >= lbl_805FD5A4) {
                        isZero = 1;
                    }
                    s32 n = *(s32*)(fx + 0xC);
                    if (isZero != 0) {
                        // Direction is ~0: orient the effect nodes along +Z.
                        for (s32 i = 0; i < n; i++) {
                            u8* node = *(u8**)(fx + i * 4 + 0x14);
                            if (node != NULL) {
                                *(f32*)(node + 0x30) = lbl_805FD598;
                                *(f32*)(node + 0x34) = lbl_805FD598;
                                *(f32*)(node + 0x38) = lbl_805FD5A8;
                                *(f32*)(node + 0x3C) = lbl_805FD598;
                            }
                        }
                    } else {
                        for (s32 i = 0; i < n; i++) {
                            u8* node = *(u8**)(fx + i * 4 + 0x14);
                            if (node != NULL) {
                                *(f32*)(node + 0x30) = d[0];
                                *(f32*)(node + 0x34) = d[1];
                                *(f32*)(node + 0x38) = d[2];
                                *(f32*)(node + 0x3C) = d[3];
                            }
                        }
                    }
                }
                ScopedLockDtor(&lock, -1);
            }
        }
        break;
    }
    case 12: {
        if (!(*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
            EventPost(0xC, arg0);
        }
        if (*(u8**)(arg0 + 0x178) != NULL) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            fn_801C849C(lbl_8047C760, *(u8**)(arg0 + 0x178));
            *(u8**)(arg0 + 0x178) = NULL;
            ScopedLockDtor(&lock, -1);
        }
        break;
    }
    case 13: {
        if (!(*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
            EventPost(0xD, arg0);
        }
        u8* p = arg0;
        for (s32 i = 0; i < 2; i++, p += 4) {
            f32 pos[3];
            pos[0] = *(f32*)(arg0 + 0x194);
            pos[1] = *(f32*)(arg0 + 0x198);
            pos[2] = *(f32*)(arg0 + 0x19C);
            if (*(u8**)(p + 0x17C) == NULL) {
                u8* e = fn_801BE620(lbl_8047C790, *(u32*)(arg0 + 0x16C), pos, pos, 0, 0);
                *(u8**)(p + 0x17C) = e;
                if (e != NULL) fn_8006EF78(arg0, e);
            }
        }
        break;
    }
    case 14: {
        if (!(*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
            EventPost(0xE, arg0);
        }
        u8* p = arg0;
        for (s32 i = 0; i < 2; i++, p += 4) {
            u8* e = *(u8**)(p + 0x17C);
            if (e != NULL) {
                fn_801BF398(lbl_8047C790, e);
                *(u8**)(p + 0x17C) = NULL;
            }
        }
        *(s32*)(arg0 + 0x168) = 0;
        break;
    }
    default:
        break;
    }
    fn_801B5D94(arg0, arg1);
}

// 0x80085CF8
// Picks a new spawn point for a monster: random point on one of the 3 ring sectors around the
// controller at +0x264 (fn_80081004), retried until it is farther than the threshold from the
// object and farther than the two "Legs" radii from its feet. Stores the point at +0x274, moves
// the def position to it (z + 10), raises def flags, and switches to the prop state from the
// table indexed by +0x284.
void fn_80085CF8(u8* arg0) {
    f32 minDist2 = lbl_803466F0[RaySceneTest(arg0)];
    u8* a28 = (u8*)fn_801B18E4(*(u8**)(arg0 + 0x8C));
    u8* sub = *(u8**)(*(u8**)(*(u8**)(arg0 + 0x264) + 0x29C) + 0x8C);
    u8* list;
    if (*(s32*)(sub + 0x10) == 0) {
        list = NULL;
    } else {
        list = *(u8**)(*(u8**)(sub + 0xC));
    }
    u8* a29 = fn_801B161C(list, kHashLegs);
    f32 zero = lbl_805FD8C8;
    f32 zz = lbl_805FD8C8 * lbl_805FD8C8;
    f32 out[4];
    f32 len;
    f32 d2;
    u8* pos;
    do {
        f32 r = lbl_805FD8C8 + (lbl_805FD8FC * (lbl_805FD8E4 * (f32)RandomU32()));
        s32 idx = (s32)(f32)floor((f64)r);
        u8* ctl = *(u8**)(arg0 + 0x264);
        f32 hi = *(f32*)(ctl + 0xB0);
        f32 lo = -*(f32*)(ctl + 0xAC);
        f32 t = lo + ((hi - lo) * (lbl_805FD8E4 * (f32)RandomU32()));
        if (idx == 0) t = t * lbl_805FD900;
        fn_80081004((u8*)out, *(u8**)(arg0 + 0x264), idx, t);
        f32 ex = *(f32*)(a29 + 0x14) - out[0];
        f32 ey = *(f32*)(a29 + 0x18) - out[1];
        f32 l2 = zz + (ex * ex + ey * ey);
        if (l2 <= zero) {
            len = zero;
        } else {
            len = sqrtf(l2);
        }
        pos = *(u8**)(arg0 + 0x4);
        f32 cx = out[0] - *(f32*)(pos + 0x0);
        f32 cy = out[1] - *(f32*)(pos + 0x4);
        f32 cz = out[2] - *(f32*)(pos + 0x8);
        d2 = cz * cz + (cx * cx + cy * cy);
    } while (d2 <= minDist2 || len < *(f32*)(a28 + 0x3C) + *(f32*)(a29 + 0x3C));

    *(f32*)(arg0 + 0x274) = out[0];
    *(f32*)(arg0 + 0x278) = out[1];
    *(f32*)(arg0 + 0x27C) = out[2];
    *(f32*)(pos + 0x0) = out[0] + lbl_805FD8C8;
    *(f32*)(pos + 0x4) = out[1] + lbl_805FD8C8;
    *(f32*)(pos + 0x8) = out[2] + lbl_805FD918;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    u32 state = lbl_80347D98[*(u32*)(arg0 + 0x284)];
    if (state != (u32)GetPropCurrentState(arg0)) {
        SetPropState(arg0, state, 0);
    }
}

// 0x8008853C
// Per-frame update of the minigame object arg0 (dt = f1): ticks the sub-objects (0x350 / 0x354 /
// the two handle tables at +0xF8 and +0x94), adds dt to the ten timers at +0x380..+0x3A4, runs
// fn_8008E5E8 on every pair of handles, then drives the spawn timers: in mode 0 re-arms +0x3A8
// and spawns +0x3AC items, in mode 1 re-arms +0x3A8 and adds to the burst counter +0x3B0 (voice
// SFX), and fires queued items (+0x3B0) every 0.05 s. Sets state +0x4A8 = 5 when idle.
void fn_8008853C(u8* arg0, f32 dt) {
    typedef void (*Fn64)(u8*);

    fn_8008CABC();   // the asm passes arg0[0x350] in r3, the callee ignores it
    (*(Fn64*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
    fn_8008CAC0(*(u8**)(arg0 + 0x350), 0.0f);   // f1 is leftover after the virtual call
    fn_8008BA14(*(u8**)(arg0 + 0x354));
    for (s32 i = 0; i < 0x96; i++) {
        u8* h = *(u8**)(arg0 + 0xF8 + i * 4);
        if (h != NULL) fn_8008CDFC(h, NULL, dt);
    }
    for (s32 i = 0; i < 0x19; i++) {
        u8* h = *(u8**)(arg0 + 0x94 + i * 4);
        if (h != NULL) fn_8008CDFC(h, *(u8**)(arg0 + 0x354), dt);
    }
    *(f32*)(arg0 + 0x380) = *(f32*)(arg0 + 0x380) + dt;
    *(f32*)(arg0 + 0x384) = *(f32*)(arg0 + 0x384) + dt;
    *(f32*)(arg0 + 0x388) = *(f32*)(arg0 + 0x388) + dt;
    *(f32*)(arg0 + 0x38C) = *(f32*)(arg0 + 0x38C) + dt;
    *(f32*)(arg0 + 0x390) = *(f32*)(arg0 + 0x390) + dt;
    *(f32*)(arg0 + 0x394) = *(f32*)(arg0 + 0x394) + dt;
    *(f32*)(arg0 + 0x398) = *(f32*)(arg0 + 0x398) + dt;
    *(f32*)(arg0 + 0x39C) = *(f32*)(arg0 + 0x39C) + dt;
    *(f32*)(arg0 + 0x3A0) = *(f32*)(arg0 + 0x3A0) + dt;
    *(f32*)(arg0 + 0x3A4) = *(f32*)(arg0 + 0x3A4) + dt;
    for (s32 i = 0; i < 0x96; i++) {
        for (s32 j = 0; j < 0x96; j++) {
            u8* a = *(u8**)(arg0 + 0xF8 + i * 4);
            if (a != NULL) {
                u8* b = *(u8**)(arg0 + 0xF8 + j * 4);
                if (b != NULL) fn_8008E5E8(a, b);
            }
        }
    }
    for (s32 i = 0; i < 0x96; i++) {
        for (s32 j = 0; j < 0x19; j++) {
            u8* a = *(u8**)(arg0 + 0xF8 + i * 4);
            if (a != NULL) {
                u8* b = *(u8**)(arg0 + 0x94 + j * 4);
                if (b != NULL) fn_8008E5E8(a, b);
            }
        }
    }
    for (s32 i = 0; i < 0x19; i++) {
        for (s32 j = 0; j < 0x96; j++) {
            u8* a = *(u8**)(arg0 + 0x94 + i * 4);
            if (a != NULL) {
                u8* b = *(u8**)(arg0 + 0xF8 + j * 4);
                if (b != NULL) fn_8008E5E8(a, b);
            }
        }
    }
    for (s32 i = 0; i < 0x19; i++) {
        for (s32 j = 0; j < 0x19; j++) {
            u8* a = *(u8**)(arg0 + 0x94 + i * 4);
            if (a != NULL) {
                u8* b = *(u8**)(arg0 + 0x94 + j * 4);
                if (b != NULL) fn_8008E5E8(a, b);
            }
        }
    }

    f32 t1 = *(f32*)(arg0 + 0x3A8) - dt;
    f32 t2 = *(f32*)(arg0 + 0x3B4) - dt;
    s32 mode = *(s32*)(arg0 + 0x90);
    *(f32*)(arg0 + 0x3A8) = t1;
    *(f32*)(arg0 + 0x3B4) = t2;
    if (mode == 0 && t1 <= lbl_805FD928) {
        s32 lo = TierClamped();
        s32 hi = TierClamped();
        f32 fHi = lbl_80346978[hi];
        f32 fLo = lbl_80346968[lo];
        *(f32*)(arg0 + 0x3A8) = fLo + ((fHi - fLo) * (lbl_805FD968 * (f32)RandomU32()));
        lo = TierClamped();
        hi = TierClamped();
        s32 iHi = lbl_80346998[hi];
        s32 iLo = lbl_80346988[lo];
        f32 gHi = (f32)iHi;
        f32 gLo = (f32)iLo;
        *(s32*)(arg0 + 0x3AC) = (s32)(gLo + ((gHi - gLo) * (lbl_805FD968 * (f32)RandomU32())));
        for (s32 n = 0; n < *(s32*)(arg0 + 0x3AC); n++) {
            fn_800893C4(arg0, *(s32*)(arg0 + 0x90), *(s32*)(arg0 + 0x4A8));
        }
    } else if (mode == 1) {
        if (*(f32*)(arg0 + 0x3A8) <= lbl_805FD928) {
            if (*(s32*)(arg0 + 0x3B0) <= 0) {
                *(f32*)(arg0 + 0x3BC) = lbl_805FD928;
                *(s32*)(arg0 + 0x3B8) = 0;
            }
            s32 lo = TierClamped();
            s32 hi = TierClamped();
            f32 fHi = lbl_803468A8[hi];
            f32 fLo = lbl_80346898[lo];
            *(f32*)(arg0 + 0x3A8) = fLo + ((fHi - fLo) * (lbl_805FD968 * (f32)RandomU32()));
            lo = TierClamped();
            hi = TierClamped();
            s32 iHi = lbl_803468C8[hi];
            s32 iLo = lbl_803468B8[lo];
            f32 gHi = (f32)iHi;
            f32 gLo = (f32)iLo;
            *(s32*)(arg0 + 0x3B0) = *(s32*)(arg0 + 0x3B0) +
                                    (s32)(gLo + ((gHi - gLo) * (lbl_805FD968 * (f32)RandomU32())));
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(0xE99FEADBu, 0, 0, 0, 0, 0);
        }
    }

    s32 n;
    while ((n = *(s32*)(arg0 + 0x3B0)) > 0 && *(f32*)(arg0 + 0x3B4) <= lbl_805FD928) {
        if (fn_800893C4(arg0, 1, *(s32*)(arg0 + 0x4A8)) != 0) {
            s32 c = *(s32*)(arg0 + 0x3B0);
            *(f32*)(arg0 + 0x3B4) = lbl_805FD938;
            *(s32*)(arg0 + 0x3B0) = c - 1;
        }
    }
    if (n <= 0 && *(s32*)(arg0 + 0x3AC) <= 0) {
        *(s32*)(arg0 + 0x4A8) = 5;
    }
}

// 0x80088BC0
// HUD update (dt = f1): ticks the flash timer at +0x7E8 / counter +0x7EC, then updates three
// bars (+0x7F0 / +0x7F4 / +0x7F8: progress from the counters at +0x488 / +0x48C / +0x490 over
// the per-tier maximums, alpha from fn_801A4084, colours green-blue / yellow / green), turns
// them white while a flash is pending, and refreshes the text fields (+0x7FC..+0x804 "%i/%i",
// +0x808 "%i").
void fn_80088BC0(u8* arg0, f32 dt) {
    if (fn_80070F4C() == NULL) return;

    s32 flash = 0;
    s32 cnt = *(s32*)(arg0 + 0x7EC);
    f32 t = *(f32*)(arg0 + 0x7E8) - dt;
    *(f32*)(arg0 + 0x7E8) = t;
    if (cnt > 0) {
        if (t >= lbl_805FD978) flash = 1;
        if (*(f32*)(arg0 + 0x7E8) <= lbl_805FD928) {
            *(s32*)(arg0 + 0x7EC) = *(s32*)(arg0 + 0x7EC) - 1;
            *(f32*)(arg0 + 0x7E8) = lbl_805FD97C;
        }
    }

    if (*(u8**)(arg0 + 0x7F0) != NULL) {
        s32 i = TierClamped();
        UISetProgress(*(void**)(arg0 + 0x7F0), lbl_805FD944 * (*(f32*)(arg0 + 0x488) / lbl_803469C8[i]));
        f32 a = BarAlpha(*(u8**)(arg0 + 0x7F0));
        u8* el = *(u8**)(arg0 + 0x7F0);
        *(f32*)(el + 0x80) = lbl_805FD928;
        *(f32*)(el + 0x84) = lbl_805FD948;
        *(f32*)(el + 0x88) = lbl_805FD934;
        *(f32*)(el + 0x8C) = a;
    }
    if (*(u8**)(arg0 + 0x7F4) != NULL) {
        s32 i = TierClamped();
        UISetProgress(*(void**)(arg0 + 0x7F4), lbl_805FD944 * (*(f32*)(arg0 + 0x48C) / lbl_803469D8[i]));
        f32 a = BarAlpha(*(u8**)(arg0 + 0x7F4));
        u8* el = *(u8**)(arg0 + 0x7F4);
        *(f32*)(el + 0x80) = lbl_805FD934;
        *(f32*)(el + 0x84) = lbl_805FD934;
        *(f32*)(el + 0x88) = lbl_805FD940;
        *(f32*)(el + 0x8C) = a;
    }
    if (*(u8**)(arg0 + 0x7F8) != NULL) {
        s32 i = TierClamped();
        UISetProgress(*(void**)(arg0 + 0x7F8), lbl_805FD944 * (*(f32*)(arg0 + 0x490) / lbl_803469E8[i]));
        f32 a = BarAlpha(*(u8**)(arg0 + 0x7F8));
        u8* el = *(u8**)(arg0 + 0x7F8);
        *(f32*)(el + 0x80) = lbl_805FD950;
        *(f32*)(el + 0x84) = lbl_805FD934;
        *(f32*)(el + 0x88) = lbl_805FD950;
        *(f32*)(el + 0x8C) = a;
    }
    if (flash != 0) {
        if (*(u8**)(arg0 + 0x7F0) != NULL) {
            f32 a = BarAlpha(*(u8**)(arg0 + 0x7F0));
            u8* el = *(u8**)(arg0 + 0x7F0);
            *(f32*)(el + 0x80) = lbl_805FD934;
            *(f32*)(el + 0x84) = lbl_805FD934;
            *(f32*)(el + 0x88) = lbl_805FD934;
            *(f32*)(el + 0x8C) = a;
        }
        if (*(u8**)(arg0 + 0x7F4) != NULL) {
            f32 a = BarAlpha(*(u8**)(arg0 + 0x7F4));
            u8* el = *(u8**)(arg0 + 0x7F4);
            *(f32*)(el + 0x80) = lbl_805FD934;
            *(f32*)(el + 0x84) = lbl_805FD934;
            *(f32*)(el + 0x88) = lbl_805FD934;
            *(f32*)(el + 0x8C) = a;
        }
        if (*(u8**)(arg0 + 0x7F8) != NULL) {
            f32 a = BarAlpha(*(u8**)(arg0 + 0x7F8));
            u8* el = *(u8**)(arg0 + 0x7F8);
            *(f32*)(el + 0x80) = lbl_805FD934;
            *(f32*)(el + 0x84) = lbl_805FD934;
            *(f32*)(el + 0x88) = lbl_805FD934;
            *(f32*)(el + 0x8C) = a;
        }
    }
    if (*(u8**)(arg0 + 0x7FC) != NULL) {
        s32 i = TierClamped();
        SysSprintf((char*)(arg0 + 0x6B0), "%i/%i", (s32)*(f32*)(arg0 + 0x488), (s32)lbl_803469C8[i]);
        UISetText(*(void**)(arg0 + 0x7FC), (const char*)(arg0 + 0x6B0), 1);
    }
    if (*(u8**)(arg0 + 0x800) != NULL) {
        s32 i = TierClamped();
        SysSprintf((char*)(arg0 + 0x6B0), "%i/%i", (s32)*(f32*)(arg0 + 0x48C), (s32)lbl_803469D8[i]);
        UISetText(*(void**)(arg0 + 0x800), (const char*)(arg0 + 0x6B0), 1);
    }
    if (*(u8**)(arg0 + 0x804) != NULL) {
        s32 i = TierClamped();
        SysSprintf((char*)(arg0 + 0x6B0), "%i/%i", (s32)*(f32*)(arg0 + 0x490), (s32)lbl_803469E8[i]);
        UISetText(*(void**)(arg0 + 0x804), (const char*)(arg0 + 0x6B0), 1);
    }
    if (*(u8**)(arg0 + 0x808) != NULL) {
        SysSprintf((char*)(arg0 + 0x6B0), "%i", *(s32*)(arg0 + 0x4A0));
        UISetText(*(void**)(arg0 + 0x808), (const char*)(arg0 + 0x6B0), 0);
    }
}

}  // extern "C"
