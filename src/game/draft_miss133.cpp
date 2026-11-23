// Decompiled from drafts: fn_800A39A4, fn_800A71F0, fn_800A79A0, fn_800B2384, fn_800B82E0,
// fn_800B9AA0, fn_800C8468, fn_800CE480, fn_801088F0, fn_80117AE4, fn_801186FC, fn_801191C8.
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "game/part.h"

extern "C" {
typedef void (*VoidVFn)(void* self);
typedef void (*SetupVFn)(void* self, f32 a, f32 b, f32 c);
typedef s32 (*QueryVFn)(void* self, f32* a, void* b, f32* c, s32 d, s32* e, s32 f);

void*  ScreenSetFindElement(void* set, u32 hash);      // 0x80198F18
void*  ScreenSetsCurrent(void* sets);                  // 0x8018D1E4
void   CameraSetFov(void* camera, f32 fov);            // 0x801D8ACC
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u8*    GetGlobalSettings(void);                        // 0x801D2BF8
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);  // 0x80209E70
u32    RandomU32(void);
s32    RaySceneTest(void* ray);                        // 0x80070458 (actually returns the global at 0x805FB124)
int    SysSprintf(char* buf, const char* fmt, ...);
u8*    fn_8006FA74(u8* self, s32 type);                // 0x8006FA74 (base ctor, returns self)
void fn_8006FC04(void);                             // 0x8006FC04
void   fn_80075264(u8* p, s32 v);                      // 0x80075264
s32    fn_800A4320(u8* obj, f32* pos);                // 0x800A4320
u8*    fn_800A43A8(u8* p, s32 group, s32 idx);         // 0x800A43A8 (returns the entry pointer)
void   fn_800A81A8(u8* p);                             // 0x800A81A8
void   fn_800B2740(u8* p);                             // 0x800B2740
void   fn_800B5990(u8* p);                             // 0x800B5990
void   fn_800B8830(u8* p, f32 v);                      // 0x800B8830
void   fn_800B9D94(u8* p);                             // 0x800B9D94
void fn_800BF97C(u8* arg0);               // 0x800BF97C
void fn_800BFAC0(u8* arg0, f32* arg1);       // 0x800BFAC0
s32    fn_800C0AE8(u8* p);                             // 0x800C0AE8
u8*    fn_800C2FF0(u8* p);                             // 0x800C2FF0
extern "C++" u8* fn_800C42B8(u8* p);                             // 0x800C42B8
void   fn_800C719C(u8* entry);                         // 0x800C719C
void*  fn_800C73D0(u8* p);                             // 0x800C73D0 (returns a pointer)
f32    fn_800C7408(u8* obj, f32* pos);                 // 0x800C7408
void   fn_800C749C(u8* p);                             // 0x800C749C
u8*    fn_800CD43C(u8* p);                             // 0x800CD43C
void   fn_800CD9A4(u8* p);                             // 0x800CD9A4
void   fn_800CF7E8(u8* p);                             // 0x800CF7E8
void*  fn_80109330(const char* name);                  // 0x80109330
void   fn_80116BE8(u8* p);                             // 0x80116BE8
void   fn_80116CA8(u8* p);                             // 0x80116CA8
void   fn_80116E54(u8* p, s32 a, s32 b);               // 0x80116E54
void   fn_80116EF0(u8* p, s32 a);                      // 0x80116EF0
void   fn_801455E0(f32* out, u8* quat);                // 0x801455E0
ScriptValue fn_801A1208(u8* el);                       // 0x801A1208
u8*    fn_801B3308(void);                              // 0x801B3308
void   fn_801B3310(u8* p, u8* q);                      // 0x801B3310
void*  fn_801D40E0(s32 n);                             // 0x801D40E0
u8* fn_801F61AC(u8* arg0, s32 arg1);                      // 0x801F61AC
void fn_801F8628(u8* obj, s8* src, s32 n);               // 0x801F8628
void   fn_801F8F34(u8* p, f32 v);                      // 0x801F8F34
void   fn_801F8FA0(u8* p, f32 v);                      // 0x801F8FA0
void   fn_801F94B0(u8* p, s32 v);                      // 0x801F94B0
u8*    fn_801F9544(u8* p, s32 a4);                     // 0x801F9544
u8*    fn_801F9D3C(u8* p, s32 a);                      // 0x801F9D3C
void*  fn_80209814(void* v);                           // 0x80209814
void   fn_8020A398(s32 handle);                        // 0x8020A398
u32    fn_8020A9A8(s32 handle);                        // 0x8020A9A8
void   fn_8020B1C8(void* handle, u32 hash, s32 a);     // 0x8020B1C8
f64    fn_8033E8EC(f64 x);                             // 0x8033E8EC
f64    tan(f64 x);                             // 0x8033F17C
f64    fn_8033F1F4(f64 x);                             // 0x8033F1F4

extern f32 lbl_80346AC8[];           // 0x80346AC8
extern f32 lbl_80346AD8[];           // 0x80346AD8
extern f32 lbl_80346AF8[];           // 0x80346AF8
extern f32 lbl_80346B08[];           // 0x80346B08
extern f32 lbl_80346CF0[];           // 0x80346CF0
extern s32 lbl_80346EA0[];           // 0x80346EA0
extern s32 lbl_80346EB0[];           // 0x80346EB0
extern f32 lbl_80346EC0[];           // 0x80346EC0
extern f32 lbl_80346ED0[];           // 0x80346ED0
extern f32 lbl_80346EE0[];           // 0x80346EE0
extern f32 lbl_80346F00[];           // 0x80346F00
extern s32 lbl_803472A0[];           // 0x803472A0
extern f32 lbl_803472B0[];           // 0x803472B0
extern f32 lbl_803472C0[];           // 0x803472C0
extern f32 lbl_803472D0[];           // 0x803472D0
extern f32 lbl_803472E0[];           // 0x803472E0
extern f32 lbl_80347380[];           // 0x80347380
extern f32 lbl_80347390[];           // 0x80347390
extern f32 lbl_803473B0[];           // 0x803473B0
extern f32 lbl_803473D0[];           // 0x803473D0
extern u8 lbl_8035D8B8[];            // 0x8035D8B8 (vtable)
extern u8 lbl_8035E2B8[];            // 0x8035E2B8 (vtable)
extern u8 lbl_8035E870[];            // 0x8035E870 (vtable)
extern u8 lbl_8035F7B8[];            // 0x8035F7B8 (vtable)
extern u8 lbl_8036D0C8[];            // 0x8036D0C8 (vtable)
extern u8 lbl_803C4298[];            // 0x803C4298 (+0x38 = controller)
extern u8 lbl_803C7390[];            // 0x803C7390 (+0x74 ranges, +0xA4 loaded flags, +0xBC valid flags)
extern u8 lbl_8047C640[];            // 0x8047C640 (camera)
extern const char* lbl_805F8F10;     // 0x805F8F10 ("ui/msa_ingredients/")
extern u8 lbl_805FB211;              // 0x805FB211
extern void* lbl_805FBBAC;           // 0x805FBBAC (voice SFX handle)
extern "C++" void* gEventMgr;        // 0x805FB6C8
extern "C++" void* gScreenSets;      // 0x805FB654

// Creates the voice SFX handle lbl_805FBBAC once (fn_801D40E0, then fn_80209814 if non-null).
static inline void EnsureVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// 0x800A39A4
// Sorts the entries (0xC8 bytes each) of the table at +0x98 into three index lists by name hash:
// list at +0x298 (count +0x268), list at +0x270 (count +0x264), list at +0x2C0 (count +0x26C).
void fn_800A39A4(u8* p) {
    s32 i = 0;
    s32 off = 0;
    while (i < *(s32*)(p + 0x94)) {
        u32 h = *(u32*)(*(u8**)(p + 0x98) + off);
        switch (h) {
        case 0x1DF09979:
        case 0x4638E534:
        case 0x4F8EA91F:
        case 0x48E36D06:
        case 0x313FD5A2:
        case 0x38899989:
        case 0xD1EA3CBC:
        case 0xD687F8A5:
        case 0xA6ED0C2A:
        case 0xA180C833:
            *(s32*)(p + *(s32*)(p + 0x268) * 4 + 0x298) = i;
            *(s32*)(p + 0x268) = *(s32*)(p + 0x268) + 1;
            break;
        case 0x4E0165E5:
        case 0x47B729CE:
        case 0x39065573:
        case 0x40DAEDD7:
        case 0x30B01958:
        case 0xD9D3BC6D:
        case 0xEC2C42BB:
        case 0xDEBE7874:
        case 0xAED48CFB:
        case 0xA9B948E2:
            *(s32*)(p + *(s32*)(p + 0x264) * 4 + 0x270) = i;
            *(s32*)(p + 0x264) = *(s32*)(p + 0x264) + 1;
            break;
        case 0x56321026:
        case 0x515FD43F:
        case 0x2658E4A9:
        case 0x213520B0:
        case 0xC1E0C9AE:
        case 0x14B60A4E:
        case 0xC8568585:
        case 0xB83C710A:
        case 0xBF51B513:
        case 0xB6E7F938:
            *(s32*)(p + *(s32*)(p + 0x26C) * 4 + 0x2C0) = i;
            *(s32*)(p + 0x26C) = *(s32*)(p + 0x26C) + 1;
            break;
        }
        off += 0xC8;
        i += 1;
    }
}

// 0x800A71F0
// Per-frame "grab" check: finds the first active entry (groups 0..2 of the object at +0x2A4 of
// fn_800C2FF0(arg1)) whose collision query hits arg0, then releases it and applies the penalty
// (group 0/1: adjust the float meters at +0x264/+0x26C, group 2: bump counter +0x360).
// Plays the loop sound at +0x298 when the active flag at +0x274 turns on, stops it when it turns off.
void fn_800A71F0(u8* arg0, u8* arg1) {
    u8* hit = NULL;
    s32 group = -1;
    u8* obj = *(u8**)(fn_800C2FF0(arg1) + 0x2A4);
    if (obj != NULL) {
        u8* cur = obj;
        s32 g = 0;
        do {
            s32 count = *(s32*)(cur + 0x264);
            s32 j;
            for (j = 0; j < count; j++) {
                u8* e = fn_800A43A8(obj, g, j);
                if (*(u8*)(e + 0x265) == 0) continue;
                u8* a = *(u8**)(e + 0x8C);
                u8* ia = (*(s32*)(a + 0x10) == 0) ? NULL : **(u8***)(a + 0xC);
                void* argA = (*(s32*)(ia + 0x28) == 0) ? NULL : **(void***)(ia + 0x24);
                u8* b = *(u8**)(arg0 + 0x8C);
                u8* ib = (*(s32*)(b + 0x10) == 0) ? NULL : **(u8***)(b + 0xC);
                u8* ob = (*(s32*)(ib + 0x28) == 0) ? NULL : **(u8***)(ib + 0x24);
                f32 r0[7] = {0}, r1[7] = {0}, r2[6] = {0};
                s32 out;
                (void)r2;
                if ((*(QueryVFn*)(*(u8**)ob + 0x14))(ob, r0, argA, r1, 0, &out, 0) != 0) {
                    hit = e;
                    group = g;
                    break;
                }
            }
            if (hit != NULL) break;
            g++;
            cur += 4;
        } while (g < 3);
        fn_800A4320(obj, *(f32**)(arg0 + 0x4));
    }
    u8 active = 0;
    if (hit != NULL) {
        switch (group) {
        case 0: {
            (*(VoidVFn*)(*(u8**)hit + 0xC4))(hit);
            f32* hp = *(f32**)(hit + 0x4);
            f32* ap = *(f32**)(arg0 + 0x4);
            f32 dy = ap[1] - hp[1];
            if (dy > 0.0f) dy = dy - 1.8f; else dy = dy + 1.8f;
            f32 ady = (f32)fabs(dy);
            *(f32*)(arg0 + 0x26C) = *(f32*)(arg0 + 0x26C) + dy;
            f32 pen = ady / 1.8f;
            pen = pen * lbl_80346B08[RaySceneTest(NULL)];
            *(f32*)(arg0 + 0x264) = *(f32*)(arg0 + 0x264) - pen;
            if (*(f32*)(arg0 + 0x264) < lbl_80346AC8[RaySceneTest(NULL)]) {
                *(f32*)(arg0 + 0x264) = lbl_80346AC8[RaySceneTest(NULL)];
            }
            break;
        }
        case 1:
            active = 1;
            if (*(u8*)(hit + 0x266) != 0) {
                *(f32*)(arg0 + 0x264) = *(f32*)(arg0 + 0x264) - lbl_80346AF8[RaySceneTest(NULL)];
                if (*(f32*)(arg0 + 0x264) < lbl_80346AD8[RaySceneTest(NULL)]) {
                    *(f32*)(arg0 + 0x264) = lbl_80346AD8[RaySceneTest(NULL)];
                }
            }
            (*(VoidVFn*)(*(u8**)hit + 0xC4))(hit);
            break;
        case 2:
            (*(VoidVFn*)(*(u8**)hit + 0xC4))(hit);
            *(s32*)(obj + 0x360) = *(s32*)(obj + 0x360) + 1;
            break;
        }
    }
    if (*(u8*)(arg0 + 0x274) != active) {
        if (active != 0) {
            EnsureVoiceHandle();
            *(s32*)(arg0 + 0x298) = PlaySound(0xD3D4DE81u, 0, 0, 0, 0, 0);
        } else {
            fn_8020A398(*(s32*)(arg0 + 0x298));
        }
    }
    *(u8*)(arg0 + 0x274) = active;
}

// 0x800A79A0
// Constructor: base init (type 7), clears the state block, sets vtable lbl_8035D8B8, builds the two
// sub-objects at +0x160 / +0x21C (registered with the global list), difficulty-scaled value at +0x150.
u8* fn_800A79A0(u8* arg0) {
    s32 i;
    s8 mask[3];
    fn_8006FA74(arg0, 7);
    *(s32*)(arg0 + 0x90) = 0;
    *(void**)(arg0 + 0x4) = lbl_8035D8B8;
    for (i = 0x94; i <= 0x15C; i += 4) *(s32*)(arg0 + i) = 0;
    fn_801F61AC(arg0 + 0x160, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F94B0(arg0 + 0x21C, *(s32*)(lbl_803C4298 + 0x38));
    *(f32*)(arg0 + 0x6A0) = 0.0f;
    *(s32*)(arg0 + 0x6A4) = 0;
    *(s32*)(arg0 + 0x6A8) = 0;
    for (i = 0x6AC; i <= 0x6FC; i += 4) *(f32*)(arg0 + i) = 0.0f;
    // identity-style block at +0x700, then 0,0,0 and (320, 240, 1) at +0x720..+0x734
    *(f32*)(arg0 + 0x700) = 1.0f;
    *(f32*)(arg0 + 0x704) = 0.0f;
    *(f32*)(arg0 + 0x708) = 0.0f;
    *(f32*)(arg0 + 0x70C) = 0.0f;
    *(f32*)(arg0 + 0x710) = 1.0f;
    *(f32*)(arg0 + 0x714) = 0.0f;
    *(f32*)(arg0 + 0x718) = 0.0f;
    *(f32*)(arg0 + 0x71C) = 0.0f;
    *(f32*)(arg0 + 0x720) = 0.0f;
    *(f32*)(arg0 + 0x724) = 0.0f;
    *(f32*)(arg0 + 0x728) = 0.0f;
    *(f32*)(arg0 + 0x72C) = 320.0f;
    *(f32*)(arg0 + 0x730) = 240.0f;
    *(f32*)(arg0 + 0x734) = 1.0f;
    // (the original also fills a dead stack matrix here)
    (*(SetupVFn*)(*(u8**)(arg0 + 0x4) + 0x54))(arg0, 320.0f, 1.0f, 0.0f);
    fn_800A81A8(arg0);
    *(f32*)(arg0 + 0x6A0) = 70.0f;
    *(f32*)(arg0 + 0x150) = lbl_80346CF0[RaySceneTest(NULL)];
    fn_801F8F34(arg0 + 0x21C, 0.1f);
    fn_801F8FA0(arg0 + 0x21C, 0.3f);
    mask[0] = 0;
    mask[1] = -1;
    mask[2] = 0;
    fn_801F8628(arg0 + 0x21C, mask, 1);
    *(s32*)(arg0 + 0x2B4) = 1;
    fn_801B3310(fn_801B3308(), arg0 + 0x160);
    fn_801B3310(fn_801B3308(), arg0 + 0x21C);
    EnsureVoiceHandle();
    fn_8020B1C8(lbl_805FBBAC, 0xA5237C5Fu, 0);
    fn_80075264(arg0, 0x37D);
    *(s32*)(arg0 + 0x8) = 0x3E4;
    *(s32*)(arg0 + 0xC) = 0x3E5;
    *(u32*)(arg0 + 0x30) = 0x2A0BD86Fu;
    *(u32*)(arg0 + 0x34) = 0xB30289D5u;
    return arg0;
}

// 0x800B2384
// Constructor: base init (type 10), clears the 4 records of 0x14 bytes at +0xBC, builds sub-objects
// at +0x1F0/+0x2AC/+0x360/+0x460/+0x514 and +0x620, clears the float slots, then fn_800B2740.
u8* fn_800B2384(u8* arg0) {
    u8* e;
    fn_8006FA74(arg0, 0xA);
    *(void**)(arg0 + 0x4) = lbl_8035E2B8;
    *(f32*)(arg0 + 0x90) = 0.0f;
    *(s32*)(arg0 + 0x94) = 0;
    *(s32*)(arg0 + 0xA8) = 0;
    *(s32*)(arg0 + 0xAC) = 0;
    *(f32*)(arg0 + 0xB0) = 0.0f;
    *(f32*)(arg0 + 0xB4) = 0.0f;
    *(f32*)(arg0 + 0xB8) = 0.0f;
    for (e = arg0 + 0xBC; e < arg0 + 0x10C; e += 0x14) {
        *(s32*)(e + 0x0) = 0;
        *(s32*)(e + 0x4) = 0;
        *(f32*)(e + 0x8) = 0.0f;
        *(f32*)(e + 0xC) = 0.0f;
        *(f32*)(e + 0x10) = 0.0f;
    }
    fn_801F61AC(arg0 + 0x1F0, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F9544(arg0 + 0x2AC, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F9D3C(arg0 + 0x360, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F9544(arg0 + 0x460, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F9D3C(arg0 + 0x514, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(arg0 + 0x614) = 0;
    fn_800B5990(arg0 + 0x620);
    *(f32*)(arg0 + 0x674) = 0.0f;
    *(f32*)(arg0 + 0x678) = 0.0f;
    *(f32*)(arg0 + 0x67C) = 0.0f;
    *(f32*)(arg0 + 0x680) = 0.0f;
    *(f32*)(arg0 + 0x704) = 0.0f;
    *(f32*)(arg0 + 0x788) = 0.0f;
    *(f32*)(arg0 + 0x80C) = 0.0f;
    *(f32*)(arg0 + 0x890) = 0.0f;
    *(f32*)(arg0 + 0x994) = 0.0f;
    *(f32*)(arg0 + 0xA24) = 0.0f;
    *(f32*)(arg0 + 0xA28) = 0.0f;
    *(s32*)(arg0 + 0x148) = 0;
    *(s32*)(arg0 + 0x14C) = 0;
    *(s32*)(arg0 + 0x150) = 0;
    *(s32*)(arg0 + 0x154) = 0;
    *(s32*)(arg0 + 0x158) = 0;
    *(s32*)(arg0 + 0x15C) = 0;
    fn_800B2740(arg0);
    fn_80075264(arg0, 0x37A);
    *(s32*)(arg0 + 0x8) = 0x157;
    *(s32*)(arg0 + 0xC) = 0x158;
    *(u32*)(arg0 + 0x30) = 0xAF93C5C3u;
    *(u32*)(arg0 + 0x34) = 0x369A9479u;
    return arg0;
}

// Sends event 'hash' from arg0 to the global event manager with bit 0 of the flags word at
// (*(arg0+4))+0x20 set for the duration of the send (unless it was already set).
static void SendEventFlagged(u8* arg0, u32 hash) {
    u8* d = *(u8**)(arg0 + 0x4);
    u32 fl = *(u32*)(d + 0x20);
    if (!(fl & 1)) {
        *(u32*)(d + 0x20) = fl | 1;
        EventSend(gEventMgr, arg0, NULL, hash, 0, 0, 1, 0);
        u8* d2 = *(u8**)(arg0 + 0x4);
        *(u32*)(d2 + 0x20) = *(u32*)(d2 + 0x20) & 0xFFFFFFFEu;
    } else {
        EventSend(gEventMgr, arg0, NULL, hash, 0, 0, 1, 0);
    }
}

// 0x800B82E0
// Per-frame update: accumulates dt at +0x268, tracks the range (min +0x29C, max +0x2A0, span +0x2A4)
// of the distance between the current and previous entry (0xC8 bytes each, table at +0x98, field
// +0x5C), and fires a stop/start event pair (with the loop sound at +0x2C0) when the current entry's
// value stops/starts changing against the last-seen value at +0x2C8.
void fn_800B82E0(u8* arg0, f32 dt) {
    f32 len = 0.0f;
    u8* tbl;
    u8* cur;
    f32 diff;
    f32 d2;
    f32 delta;
    *(f32*)(arg0 + 0x268) = *(f32*)(arg0 + 0x268) + dt;
    tbl = *(u8**)(arg0 + 0x98);
    diff = *(f32*)(tbl + *(s32*)(arg0 + 0x290) * 0xC8 + 0x5C) -
           *(f32*)(tbl + *(s32*)(arg0 + 0x294) * 0xC8 + 0x5C);
    d2 = diff * diff;
    if (!(d2 <= 0.0f)) len = sqrtf(d2);
    len = (f32)fabs(len);
    *(f32*)(arg0 + 0x2AC) = len;
    if (len > *(f32*)(arg0 + 0x2A0)) *(f32*)(arg0 + 0x2A0) = len;
    if (*(f32*)(arg0 + 0x2AC) < *(f32*)(arg0 + 0x29C)) *(f32*)(arg0 + 0x29C) = *(f32*)(arg0 + 0x2AC);
    *(f32*)(arg0 + 0x2A4) = *(f32*)(arg0 + 0x2A0) - *(f32*)(arg0 + 0x29C);
    cur = *(u8**)(arg0 + 0x98) + *(s32*)(arg0 + 0x294) * 0xC8;
    delta = *(f32*)(cur + 0x5C) - *(f32*)(arg0 + 0x2C8);
    if (*(u8*)(arg0 + 0x264) != 0) fn_800B8830(arg0, delta);
    if (delta > -0.0001f && delta < 0.0001f) {
        if (*(u8*)(arg0 + 0x2C4) != 0) {
            EnsureVoiceHandle();
            if (fn_8020A9A8(*(s32*)(arg0 + 0x2C0)) == 0) {
                EnsureVoiceHandle();
                *(s32*)(arg0 + 0x2C0) = PlaySound(0x52CAA3D4u, 0, 0, 0, 0, 0);
            }
        }
        *(u8*)(arg0 + 0x2C4) = 0;
        SendEventFlagged(arg0, 0x3B28BA37u);
    } else {
        *(u8*)(arg0 + 0x2C4) = 1;
        SendEventFlagged(arg0, 0x0C95C5BFu);
    }
    *(f32*)(arg0 + 0x2C8) = *(f32*)(cur + 0x5C);
    *(f32*)(arg0 + 0x2CC) = *(f32*)(cur + 0x60);
    *(f32*)(arg0 + 0x2D0) = *(f32*)(cur + 0x64);
}

// 0x800B9AA0
// Constructor: base init (type 11), clears the state block (+0x94..+0x120, +0x124..+0x16C, +0x238,
// +0x488), vtable lbl_8035E870, sub-object at +0x240 and +0x3BC; difficulty-indexed tuning values
// (+0x3AC..+0x3B8) and a random counter at +0x12C within [lo, hi].
u8* fn_800B9AA0(u8* arg0) {
    f32 pos[3];
    u8* target;
    s32 idx;
    s32 lo;
    s32 i;
    fn_8006FA74(arg0, 0xB);
    *(void**)(arg0 + 0x4) = lbl_8035E870;
    *(f32*)(arg0 + 0x90) = 0.0f;
    fn_800BF97C(arg0 + 0x240);
    fn_801F61AC(arg0 + 0x3BC, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(arg0 + 0x488) = 0;
    for (i = 0x94; i <= 0x120; i += 4) *(s32*)(arg0 + i) = 0;
    for (i = 0x150; i <= 0x16C; i += 4) *(s32*)(arg0 + i) = 0;
    for (i = 0x130; i <= 0x14C; i += 4) *(s32*)(arg0 + i) = 0;
    *(s32*)(arg0 + 0x238) = 0;
    *(s32*)(arg0 + 0x23C) = 0;
    fn_800B9D94(arg0);
    target = *(u8**)(*(u8**)(arg0 + 0x238) + 0x4);
    pos[0] = *(f32*)(target + 0x0);
    pos[1] = *(f32*)(target + 0x4);
    pos[2] = *(f32*)(target + 0x8);
    fn_800BFAC0(arg0 + 0x240, pos);
    fn_800C0AE8(arg0 + 0x240);
    *(f32*)(arg0 + 0x90) = 70.0f;
    idx = RaySceneTest(NULL);
    lo = lbl_80346EA0[idx];
    *(f32*)(arg0 + 0x3AC) = lbl_80346EC0[idx];
    *(f32*)(arg0 + 0x3B0) = lbl_80346ED0[idx];
    *(f32*)(arg0 + 0x3B4) = lbl_80346EE0[idx];
    *(f32*)(arg0 + 0x3B8) = lbl_80346F00[idx];
    *(s32*)(arg0 + 0x124) = 0;
    *(s32*)(arg0 + 0x128) = 0;
    *(s32*)(arg0 + 0x12C) = lo;
    *(s32*)(arg0 + 0x12C) = *(s32*)(arg0 + 0x12C) + RandomU32() % (u32)(lbl_80346EB0[idx] - lo + 1);
    fn_801B3310(fn_801B3308(), arg0 + 0x3BC);
    fn_80075264(arg0, 0x376);
    *(s32*)(arg0 + 0x8) = 0x83D;
    *(s32*)(arg0 + 0xC) = 0x83E;
    *(u32*)(arg0 + 0x30) = 0x45965FC6u;
    *(u32*)(arg0 + 0x34) = 0xDC9F0E7Cu;
    return arg0;
}

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static f32 SqrtOrZero(f32 v) {
    return (v <= 0.0f) ? 0.0f : sqrtf(v);
}

// Clamps v to [lo, hi].
static f32 ClampF(f32 v, f32 lo, f32 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// 0x800C8468
// Scores a target 'arg1' against a query position (arg2) and facing quaternion (arg3): distance
// falloff and angle falloff are combined (average) and added to the float at +8 of arg0 scaled by
// difficulty tables. Returns 0 if the target is missing or either falloff is <= 0.
s32 fn_800C8468(u8* arg0, u8* arg1, f32* arg2, f32* arg3) {
    f32 pos[3];
    f32 q[4];
    f32 va[3];
    f32 vb[3];
    f32 distScore, angScore, angle, cosv, denom, dot;
    u8* obj;
    u8* qsrc;
    if (arg1 == NULL) return 0;
    obj = (u8*)fn_800C73D0(arg1);
    if (obj == NULL) return 0;
    pos[0] = arg2[0];
    pos[1] = arg2[1];
    pos[2] = arg2[2];
    distScore = 1.0f - fn_800C7408(arg1, pos) / ClampF(lbl_80347390[RaySceneTest(NULL)], 0.0f, 100.0f);
    if (distScore <= 0.0f) return 0;
    qsrc = *(u8**)(obj + 0x4);
    q[0] = *(f32*)(qsrc + 0xC);
    q[1] = *(f32*)(qsrc + 0x10);
    q[2] = *(f32*)(qsrc + 0x14);
    q[3] = *(f32*)(qsrc + 0x18);
    fn_801455E0(vb, (u8*)q);
    fn_801455E0(va, (u8*)arg3);
    denom = SqrtOrZero(va[2] * va[2] + (va[0] * va[0] + va[1] * va[1])) *
            SqrtOrZero(vb[2] * vb[2] + (vb[0] * vb[0] + vb[1] * vb[1]));
    angle = 0.0f;
    if (!(denom <= 0.0f)) {
        dot = va[2] * vb[2] + (va[0] * vb[0] + va[1] * vb[1]);
        cosv = ClampF(dot / denom, -1.0f, 1.0f);
        cosv = ClampF(cosv, -1.0f, 1.0f);
        angle = 0.63661975f * (f32)fn_8033F1F4((f64)cosv);
    }
    angle = (f32)fabs(angle);
    if (angle > 1.0f) angle = 2.0f - angle;
    angScore = 1.0f - angle / ClampF(0.011111111f * lbl_80347380[RaySceneTest(NULL)], 0.0f, 1.0f);
    if (angScore <= 0.0f) return 0;
    fn_800C719C(arg1);
    angScore = (angScore + distScore) * 0.5f;
    if (angScore - 1.0f > -0.0001f && angScore - 1.0f < 0.0001f) {
        f32 a = lbl_803473B0[RaySceneTest(NULL)];
        *(f32*)(arg0 + 0x8) = *(f32*)(arg0 + 0x8) + lbl_803473D0[RaySceneTest(NULL)] * a;
    } else {
        *(f32*)(arg0 + 0x8) = *(f32*)(arg0 + 0x8) + angScore * lbl_803473D0[RaySceneTest(NULL)];
    }
    return 1;
}

// 0x800CE480
// Constructor: base init (type 9), vtable lbl_8035F7B8, sub-objects at +0xA8/+0xF4/+0x2FFC/+0x3020,
// sets the camera FOV from atan(10 * tan(0.4636) / 70), then (difficulty permitting) picks a random
// drift speed (+0x94, random sign) and a random timer (+0x98); otherwise +0x98 = 500.
u8* fn_800CE480(u8* arg0) {
    f32 lo, hi;
    fn_8006FA74(arg0, 9);
    *(void**)(arg0 + 0x4) = lbl_8035F7B8;
    *(u8*)(arg0 + 0x90) = 0;
    *(f32*)(arg0 + 0x94) = 0.0f;
    *(f32*)(arg0 + 0x98) = 0.0f;
    *(f32*)(arg0 + 0x9C) = 0.0f;
    *(f32*)(arg0 + 0xA0) = 0.0f;
    fn_800CD9A4(arg0 + 0xA8);
    fn_800C749C(arg0 + 0xF4);
    fn_800CD43C(arg0 + 0x2FFC);
    fn_800C42B8(arg0 + 0x3020);
    CameraSetFov(lbl_8047C640,
                 2.0f * (1.5707964f * (0.63661975f * (f32)fn_8033E8EC((f64)((10.0f * (f32)tan(0.46364763f)) / 70.0f)))));
    fn_8006FC04();
    fn_800CF7E8(arg0);
    *(f32*)(arg0 + 0xA0) = 0.0f;
    *(f32*)(arg0 + 0x9C) = 0.0f;
    if (lbl_803472A0[RaySceneTest(NULL)] != 0) {
        *(u8*)(arg0 + 0x90) = 1;
        hi = lbl_803472C0[RaySceneTest(NULL)];
        lo = lbl_803472B0[RaySceneTest(NULL)];
        *(f32*)(arg0 + 0x94) = 0.011111111f * (lo + (hi - lo) * (2.3283064e-10f * (f32)RandomU32()));
        if (RandomU32() & 1) {
            *(f32*)(arg0 + 0x94) = *(f32*)(arg0 + 0x94) * -1.0f;
        }
        hi = lbl_803472E0[RaySceneTest(NULL)];
        lo = lbl_803472D0[RaySceneTest(NULL)];
        *(f32*)(arg0 + 0x98) = lo + (hi - lo) * (2.3283064e-10f * (f32)RandomU32());
    } else {
        *(u8*)(arg0 + 0x90) = 0;
        *(f32*)(arg0 + 0x98) = 500.0f;
    }
    fn_80075264(arg0, 0x37C);
    *(s32*)(arg0 + 0x8) = 0x1018;
    *(s32*)(arg0 + 0xC) = 0x1019;
    *(u32*)(arg0 + 0x30) = 0x75CDBF77u;
    *(u32*)(arg0 + 0x34) = 0xECC4EECDu;
    return arg0;
}

// Number of slots in the global settings table: the sum of the counts at +0x68..+0x98 and the
// counts at +0x6C..+0x7C (as the original re-reads the settings per term), plus one.
static s32 SlotCountPlusOne(void) {
    s32 n = *(s32*)(GetGlobalSettings() + 0x80);
    n = *(s32*)(GetGlobalSettings() + 0x68) + n;
    n += *(s32*)(GetGlobalSettings() + 0x84);
    n += *(s32*)(GetGlobalSettings() + 0x88);
    n += *(s32*)(GetGlobalSettings() + 0x8C);
    n += *(s32*)(GetGlobalSettings() + 0x90);
    n += *(s32*)(GetGlobalSettings() + 0x94);
    n += *(s32*)(GetGlobalSettings() + 0x98);
    n += *(s32*)(GetGlobalSettings() + 0x6C);
    n += *(s32*)(GetGlobalSettings() + 0x70);
    n += *(s32*)(GetGlobalSettings() + 0x74);
    n += *(s32*)(GetGlobalSettings() + 0x78);
    n += *(s32*)(GetGlobalSettings() + 0x7C);
    return n + 1;
}

// 0x801088F0
// Address lookup: does 'addr' fall inside the loaded range of slot 'idx' (or any loaded slot if
// idx == -1)? On a hit stores the slot index to *out (if non-NULL) and returns 1. When the flag
// lbl_805FB211 is 0 it instead checks addr against the heap window starting at 0x90400000 whose
// size is derived from the global settings (no slot index is reported).
s32 fn_801088F0(u8* addrp, s32 idx, s32* out) {
    uintptr_t addr = (uintptr_t)addrp;
    if (lbl_805FB211 == 0) {
        u32 total;
        if (addr >= 0x90400000u) {
            u32 t = *(u32*)(GetGlobalSettings() + 0x24);
            total = *(u32*)(GetGlobalSettings() + 0x50) * t;
            t = *(u32*)(GetGlobalSettings() + 0x20);
            u32 a = *(u32*)(GetGlobalSettings() + 0x4C) * t;
            t = *(u32*)(GetGlobalSettings() + 0xC);
            u32 b = (*(u32*)(GetGlobalSettings() + 0x38) - 1) * t;
            t = *(u32*)(GetGlobalSettings() + 0x8);
            u32 c = *(u32*)(GetGlobalSettings() + 0x34) * t;
            t = *(u32*)(GetGlobalSettings() + 0x4);
            c = c + t;
            b = b + *(u32*)GetGlobalSettings();
            t = *(u32*)(GetGlobalSettings() + 0x10);
            b = c + b;
            c = (*(u32*)(GetGlobalSettings() + 0x3C) - 1) * t;
            t = *(u32*)(GetGlobalSettings() + 0x14);
            b = c + b;
            c = (*(u32*)(GetGlobalSettings() + 0x40) - 1) * t;
            t = *(u32*)(GetGlobalSettings() + 0x18);
            b = c + b;
            c = (*(u32*)(GetGlobalSettings() + 0x44) - 1) * t;
            t = *(u32*)(GetGlobalSettings() + 0x1C);
            c = c + b;
            u32 d = (*(u32*)(GetGlobalSettings() + 0x48) - 1) * t + c;
            total = a + total;
            total = d + total;
            t = *(u32*)(GetGlobalSettings() + 0x28);
            u32 e = *(u32*)(GetGlobalSettings() + 0x58) * t;
            t = *(u32*)(GetGlobalSettings() + 0x2C);
            u32 f = *(u32*)(GetGlobalSettings() + 0x64) * t;
            total = e + total;
            if (addr <= (uintptr_t)(u32)(f + (total + 0x90400000u))) return 1;
        }
        return 0;
    }
    if (idx == -1) {
        s32 i = 0;
        s32 off = 0;
        for (;;) {
            if (i >= SlotCountPlusOne()) return 0;
            if (*(s32*)(*(u8**)(lbl_803C7390 + 0xA4) + i * 4) != 0 &&
                *(u8*)(*(u8**)(lbl_803C7390 + 0xBC) + i) != 0) {
                u8* ent = *(u8**)(lbl_803C7390 + 0x74) + off;
                if (addr >= *(u32*)ent && addr < *(u32*)(ent + 4)) {
                    if (out != NULL) *out = i;
                    return 1;
                }
            }
            off += 0x20;
            i++;
        }
    }
    if (idx < SlotCountPlusOne() && *(s32*)(*(u8**)(lbl_803C7390 + 0xA4) + idx * 4) != 0 &&
        *(u8*)(*(u8**)(lbl_803C7390 + 0xBC) + idx) != 0) {
        u8* ent = *(u8**)(lbl_803C7390 + 0x74) + (idx << 5);
        if (addr >= *(u32*)ent && addr < *(u32*)(ent + 4)) {
            if (out != NULL) *out = idx;
            return 1;
        }
    }
    return 0;
}

// 0x80117AE4
// Constructor: base init, vtable lbl_8036D0C8, clears the header, resets the 96 records of 0x20 bytes
// at +0x58 (fields: 0, -1, 0 x5), inits the sub-object at +0x14AC, and zeroes four 117-word tables
// at +0xD5C, +0xF30, +0x1104, +0x12D8.
u8* fn_80117AE4(u8* arg0) {
    u8* r;
    s32 i;
    fn_80116BE8(arg0);
    *(void**)(arg0 + 0x0) = lbl_8036D0C8;
    *(s32*)(arg0 + 0x1C) = -1;
    *(s32*)(arg0 + 0x28) = 0;
    *(s32*)(arg0 + 0x38) = 0;
    *(s32*)(arg0 + 0x3C) = -1;
    *(s32*)(arg0 + 0x40) = 0;
    *(s32*)(arg0 + 0x44) = 0;
    *(s32*)(arg0 + 0x48) = 0;
    *(s32*)(arg0 + 0x4C) = 0;
    *(s32*)(arg0 + 0x50) = 0;
    for (r = arg0 + 0x58; r < arg0 + 0xD58; r += 0x20) {
        *(s32*)(r + 0x0) = 0;
        *(s32*)(r + 0x4) = -1;
        *(s32*)(r + 0x8) = 0;
        *(s32*)(r + 0xC) = 0;
        *(s32*)(r + 0x10) = 0;
        *(s32*)(r + 0x14) = 0;
        *(s32*)(r + 0x18) = 0;
    }
    fn_80116CA8(arg0 + 0x14AC);
    *(s32*)(arg0 + 0x2C) = 0;
    *(s32*)(arg0 + 0x34) = 0;
    *(s32*)(arg0 + 0x14DC) = 0;
    for (i = 0; i < 117; i++) {
        *(s32*)(arg0 + 0xD5C + i * 4) = 0;
        *(s32*)(arg0 + 0xF30 + i * 4) = 0;
        *(s32*)(arg0 + 0x1104 + i * 4) = 0;
        *(s32*)(arg0 + 0x12D8 + i * 4) = 0;
    }
    return arg0;
}

// 0x801186FC
// Looks up the inventory icon texture for a record: kind 0 uses the table at db+0x4F00 (0x10-byte
// entries), kind 1 the table at db+0x5328 (0x18-byte entries). Tries "<prefix>inventory_icon_%03d"
// (kind 0) / "<prefix>inventory_icon_sec_%03d" (kind 1) and falls back to the placeholder icon.
void* fn_801186FC(u8* arg0) {
    char buf[0x100];
    u8* rec;
    void* tex;
    s32 kind = *(s32*)(arg0 + 0x0);
    if (kind == 0) {
        rec = *(u8**)(gGameDb->data + 0x4F00) + (*(s32*)(arg0 + 0x4) << 4);
        SysSprintf(buf, "%sinventory_icon_%03d", lbl_805F8F10, (s32)*(s8*)(rec + 0xA));
        tex = fn_80109330(buf);
        if (tex == NULL) {
            SysSprintf(buf, "%sinventory_icon_placeholder", lbl_805F8F10);
            return fn_80109330(buf);
        }
        return tex;
    }
    if (kind == 1) {
        rec = *(u8**)(gGameDb->data + 0x5328) + *(s32*)(arg0 + 0x4) * 0x18;
        SysSprintf(buf, "%sinventory_icon_sec_%03d", lbl_805F8F10, (s32)*(s8*)(rec + 0x12));
        tex = fn_80109330(buf);
        if (tex == NULL) {
            SysSprintf(buf, "%sinventory_icon_placeholder", lbl_805F8F10);
            return fn_80109330(buf);
        }
        return tex;
    }
    return NULL;
}

// Appends record 'rec' (and, if it has an icon id at +0xC, its name data at +0x1C and the icon
// element) to the parallel lists at +0xD5C / +0xF30 / +0x1104 / +0x12D8 (count at +0xD58).
static void AddListEntry(u8* arg0, u8* rec, u8* el) {
    s32 n = *(s32*)(arg0 + 0xD58);
    *(u8**)(arg0 + n * 4 + 0xD5C) = rec;
    n = *(s32*)(arg0 + 0xD58);
    *(s32*)(arg0 + n * 4 + 0xF30) = *(s32*)(rec + 0xC);
    if (*(s32*)(rec + 0xC) != 0) {
        n = *(s32*)(arg0 + 0xD58);
        *(u8**)(arg0 + n * 4 + 0x1104) = rec + 0x1C;
        n = *(s32*)(arg0 + 0xD58);
        *(u8**)(arg0 + n * 4 + 0x12D8) = el;
    }
    *(s32*)(arg0 + 0xD58) = *(s32*)(arg0 + 0xD58) + 1;
}

// 0x801191C8
// Rebuilds the visible item list: clears the four parallel lists, then collects the records
// (0x20 bytes each from +0x38, count at +0x34) that match the current filter (+0x1C / +0x20 / +0x28),
// and updates the scroll sub-object at +0x14AC with the page count (4 per page).
void fn_801191C8(u8* arg0) {
    s32 vals[0x9C];
    s32 nvals = 0;
    s32 i;
    s32 n;
    s32 pages;
    u8* rec;
    u8* el;
    *(s32*)(arg0 + 0x2C) = -1;
    el = (u8*)fn_801A1208((u8*)ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), 0x40703B82u));
    memset(arg0 + 0xF30, 0, 0x1D4);
    memset(arg0 + 0xD5C, 0, 0x1D4);
    memset(arg0 + 0x1104, 0, 0x1D4);
    memset(arg0 + 0x12D8, 0, 0x1D4);
    *(s32*)(arg0 + 0xD58) = 0;
    if (*(s32*)(arg0 + 0x1C) == -1 || *(s32*)(arg0 + 0x28) == 1) {
        rec = arg0 + 0x38;
        for (i = 0; i < *(s32*)(arg0 + 0x34); i++, rec += 0x20) {
            if (*(s32*)(rec + 0x4) == *(s32*)(arg0 + 0x28) &&
                (*(s32*)(arg0 + 0x20) == 3 || *(s32*)(rec + 0x0) == *(s32*)(arg0 + 0x20) ||
                 *(s32*)(rec + 0x0) == -1)) {
                AddListEntry(arg0, rec, el);
            }
        }
    } else if (*(s32*)(arg0 + 0x28) == 0) {
        // entries (0x24 bytes) of the table at db+0x6C18 whose +0xC matches the filter at +0x1C
        for (i = 0; i < 0x9C; i++) {
            u8* ent = *(u8**)(gGameDb->data + 0x6C18) + i * 0x24;
            if (*(s32*)(arg0 + 0x1C) == *(s32*)(ent + 0xC)) {
                vals[nvals] = *(s32*)(ent + 0x10);
                nvals++;
            }
        }
        rec = arg0 + 0x38;
        for (i = 0; i < *(s32*)(arg0 + 0x34); i++, rec += 0x20) {
            if (*(s32*)(rec + 0x4) == *(s32*)(arg0 + 0x28)) {
                s32 k;
                for (k = 0; k < nvals; k++) {
                    if (*(s32*)(rec + 0x8) == vals[k]) {
                        AddListEntry(arg0, rec, el);
                        break;
                    }
                }
            }
        }
    }
    n = *(s32*)(arg0 + 0xD58);
    pages = n / 4;
    if (n % 4 > 0) pages += 1;
    fn_80116E54(arg0 + 0x14AC, 3, pages);
    fn_80116EF0(arg0 + 0x14AC, *(s32*)(arg0 + 0x30));
}

}  // extern "C"
