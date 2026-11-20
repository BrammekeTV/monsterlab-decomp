// Misc decompiled functions: reactor-leak meter update, screen-set push, skeleton matrix helpers,
// and the experiment-object constructors (instrument, welding, dance, ...).
#include <math.h>
#include <stdint.h>

#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
u32   RandomU32(void);                                                // 0x80143460
s32   RaySceneTest(void* ray);                                        // 0x80070458
u32   StringHash(const char* s);                                      // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);
void  ZoneLog(void* loader, const char* fmt, ...);
u8*   fn_8006FA74(u8* self, s32 n);                                   // 0x8006FA74
void  fn_80075264(u8* a3, s32 a4);                                    // 0x80075264
u8*   fn_80076D38(u8* self, s32 a);                                   // 0x80076D38
void  fn_800759E8(u8* self);                                          // 0x800759E8
void  fn_800784EC(u8* self);                                          // 0x800784EC
void  fn_80078BB8(u8* p);                                             // 0x80078BB8
void  fn_80079004(u8* p);                                             // 0x80079004
void  fn_80086F84(u8* self);                                          // 0x80086F84
void  fn_8007B20C(u8* obj);                                           // 0x8007B20C
void  fn_8007C4AC(u8* p);                                             // 0x8007C4AC
u8*   fn_8007C4CC(u8* p, s32 del);                                    // 0x8007C4CC
u8*   fn_8007CED8(u8* self);                                          // 0x8007CED8
void  fn_8007D860(u8* self);                                          // 0x8007D860
void  fn_8007DDEC(u8* self, f32 a);                                   // 0x8007DDEC
void  fn_8008F068(u8* p);                                             // 0x8008F068
void  fn_8009210C(u8* p);                                             // 0x8009210C
u8*   fn_8009215C(u8* p, s32 del);                                    // 0x8009215C
void  fn_800923DC(u8* p);                                             // 0x800923DC
void  fn_800938A4(u8* p);                                             // 0x800938A4
u8*   fn_800938E0(u8* p, s32 del);                                    // 0x800938E0
void  fn_80091D04(u8* p);                                             // 0x80091D04
void* fn_80091D28(void* p, s32 del);                                  // 0x80091D28
void  fn_80091E44(u8* p);                                             // 0x80091E44
void  fn_80093A88(u8* p);                                             // 0x80093A88
void  fn_800943CC(u8* self);                                          // 0x800943CC
void  fn_80097574(u8* p);                                             // 0x80097574
u8*   fn_80097590(u8* p, s32 del);                                    // 0x80097590
f32 fn_800999B4(u8* obj, f32 v);                                    // 0x800999B4
void  fn_80099D08(u8* self);                                          // 0x80099D08
u8*   fn_8009B264(u8* p);                                             // 0x8009B264
void fn_8009B320(u8* self);                                             // 0x8009B320
u8*   fn_8009BE14(u8* p);                                             // 0x8009BE14
void  fn_8010B16C(u8* p);                                             // 0x8010B16C
void* fn_8010B184(void* p, s32 del);                                  // 0x8010B184
u8*   fn_800E4BFC(u8* p);                                             // 0x800E4BFC
void fn_800E4CB0(u8* arg0);                                      // 0x800E4CB0
u8*   fn_800E4D7C(u32 a0);                                            // 0x800E4D7C
void  fn_8018A948(u8* obj, f32 t);                                    // 0x8018A948
void  fn_8018D2B8(const char* fmt, ...);                              // 0x8018D2B8
void  fn_8018DEF4(void* sets);                                        // 0x8018DEF4
s32   fn_80198290(u8* a3, s32 a4);                                    // 0x80198290
s32   fn_8019829C(u8* a3, f32 f1);                                    // 0x8019829C
s32   fn_80199618(void* obj);                                         // 0x80199618
void  fn_8019A994(void* p);                                           // 0x8019A994
void  fn_8019BFF8(void* obj, void* cb, void* arg);                    // 0x8019BFF8
u8*   fn_801AC184(u8* defs, s32 idx);                                 // 0x801AC184
void* fn_801D40E0(s32 n);                                             // 0x801D40E0
u8*   fn_801DA788(u8* a3, s32 i);                                     // 0x801DA788
u8* fn_801F61AC(u8* arg0, s32 arg1);                                      // 0x801F61AC
void  fn_801F6848(u8* p, s32 a);                                      // 0x801F6848
void  fn_801F7800(u8* p, s32 a, void* b);                             // 0x801F7800
void  fn_801F94B0(u8* p, s32 v);                                      // 0x801F94B0
u8*   fn_801F9544(u8* p, s32 a4);                                     // 0x801F9544
void  fn_801FAAE0(u8* p, s32 a);                                      // 0x801FAAE0
u8*   fn_802168DC(u8* p);                                             // 0x802168DC
void  fn_8032F1E8(void* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);  // 0x8032F1E8
f64   fn_8033F1F4(f64 x);                                             // 0x8033F1F4 (acos)
void* memset(void* p, int c, unsigned long n);

extern u8  gIdPool[];             // 0x8048A8F0
extern u8  gWorldState[];         // 0x803E6D00 (first word = world block pointer)
extern f32 lbl_80347020[];        // 0x80347020
extern f32 lbl_80347030[];        // 0x80347030
extern s32 lbl_80347170[];        // 0x80347170
extern f32 lbl_803462C8[];        // 0x803462C8
extern u8  lbl_80348F38[];        // 0x80348F38 (shader/object dispatch table)
extern u8  lbl_8035A950[];        // 0x8035A950 (vtable)
extern s32 lbl_8035A938;          // 0x8035A938
extern u8  lbl_8035AD08[];        // 0x8035AD08 (vtable)
extern u8  lbl_8035AFD0[];        // 0x8035AFD0 (vtable)
extern u8  lbl_8035B830[];        // 0x8035B830 (vtable)
extern u8  lbl_8035BC90[];        // 0x8035BC90 (vtable)
extern u8  lbl_8035C208[];        // 0x8035C208 (vtable)
extern u8  lbl_8035C890[];        // 0x8035C890 (vtable)
extern u8  lbl_803C4298[];        // 0x803C4298 (+0x38 = controller)
extern f32 lbl_803C6AC0[];        // 0x803C6AC0 (function-local static colour A)
extern f32 lbl_803C6AD0[];        // 0x803C6AD0 (function-local static colour B)
extern u8  lbl_805FB138[];        // 0x805FB138 (dance object pointer slot)
extern u8  lbl_805FB140[];        // 0x805FB140 (welding object pointer slot)
extern u8  lbl_805FB148;          // 0x805FB148 (init guard for lbl_803C6AC0)
extern u8  lbl_805FB149;          // 0x805FB149 (init guard for lbl_803C6AD0)
}

extern "C++" s32   gZoneResourceSize;   // 0x805FB94C
extern "C++" void* gScreenSets;         // 0x805FB654
extern "C++" void* lbl_805FB670;        // 0x805FB670
extern "C++" s32   fn_80199054(void* obj);  // 0x80199054 (C++ linkage in this repo)

static const u32 kHashCastleArcWelderA = 0x832EC9EBu;  // "castle_arc_welderA"
static const u32 kHashScrewBoltA       = 0x9EEDA0B8u;  // "screw_boltA"
static const u32 kHashScrewFlatA       = 0x71003AC5u;  // "screw_flatA"
static const u32 kHashScrewPhilA       = 0x9DCA0651u;  // "screw_philA"

typedef u8*  (*GetObjFn)(u8* self);
typedef u32  (*GetBitsFn)(void* self, s32 idx, u8* entry);

extern "C" {

// 0x8005B4A4
// Angle between two vectors, scaled to 0..2 (acos(dot / (|a| |b|)) * 2/pi). Returns 0 when a length is 0.
f32 fn_8005B4A4(const Vec3* a, const Vec3* b) {
    f32 ax = a->x, ay = a->y, az = a->z;
    f32 la2 = az * az + (ax * ax + ay * ay);
    f32 la = 0.0f;
    if (la2 > 0.0f) la = sqrtf(la2);  // original: frsqrte with two Newton steps
    f32 bx = b->x, by = b->y, bz = b->z;
    f32 lb2 = bz * bz + (bx * bx + by * by);
    f32 lb = 0.0f;
    if (lb2 > 0.0f) lb = sqrtf(lb2);
    f32 denom = la * lb;
    if (denom <= 0.0f) return 0.0f;
    f32 c = (az * bz + (ax * bx + ay * by)) / denom;
    f32 v = c;
    if (c < -1.0f) v = -1.0f;
    else if (c > 1.0f) v = 1.0f;
    f32 w = v;
    if (v < -1.0f) w = -1.0f;
    else if (v > 1.0f) w = 1.0f;
    return 0.63661975f * (f32)fn_8033F1F4((f64)w);
}

// 0x8005C020
// Builds a 3x4 matrix (rotation from the quaternion at +0xC..+0x18, translation from +0..+8) of
// the idx-th bone record of the skeleton data and writes it to outMtx.
void fn_8005C020(u8* skel, s32 idx, u8* outMtx) {
    u8* data = *(u8**)(skel + 0x14);
    u8* rec = data + *(s32*)(data + 0x8) + idx * *(s32*)(data + 0x18);
    f32 qx = *(f32*)(rec + 0xC);
    f32 qy = *(f32*)(rec + 0x10);
    f32 qz = *(f32*)(rec + 0x14);
    f32 qw = *(f32*)(rec + 0x18);
    f32 x2 = 2.0f * qx;
    f32 z2 = 2.0f * qz;
    f32 y2 = 2.0f * qy;
    f32 xx = x2 * qx;
    f32 zz = z2 * qz;
    f32 yy = y2 * qy;
    f32 xw = x2 * qw;
    f32 zy = z2 * qy;
    *(f32*)(outMtx + 0x14) = zy + xw;
    *(f32*)(outMtx + 0x0)  = 1.0f - (yy + zz);
    *(f32*)(outMtx + 0x10) = 1.0f - (xx + zz);
    *(f32*)(outMtx + 0x1C) = zy - xw;
    f32 yw = y2 * qw;
    f32 zx = z2 * qx;
    *(f32*)(outMtx + 0x20) = 1.0f - (xx + yy);
    f32 zw = z2 * qw;
    *(f32*)(outMtx + 0x8)  = zx - yw;
    *(f32*)(outMtx + 0x18) = zx + yw;
    f32 yx = y2 * qx;
    *(f32*)(outMtx + 0x4)  = yx + zw;
    *(f32*)(outMtx + 0xC)  = yx - zw;
    *(f32*)(outMtx + 0x24) = *(f32*)(rec + 0x0);
    *(f32*)(outMtx + 0x28) = *(f32*)(rec + 0x4);
    *(f32*)(outMtx + 0x2C) = *(f32*)(rec + 0x8);
}

// 0x8005C204
// Inverts a 3x4 matrix made of a (possibly scaled) rotation: transposes the 3x3 part, divides each
// row by its squared length and rotates the negated translation into the new frame.
void fn_8005C204(u8* out, u8* in) {
    f32 m0 = *(f32*)(in + 0x0);
    f32 m1 = *(f32*)(in + 0x4);
    f32 m3 = *(f32*)(in + 0xC);
    f32 m4 = *(f32*)(in + 0x10);
    f32 m5 = *(f32*)(in + 0x14);
    f32 m6 = *(f32*)(in + 0x18);
    f32 m7 = *(f32*)(in + 0x1C);
    f32 m2 = *(f32*)(in + 0x8);
    f32 m8 = *(f32*)(in + 0x20);
    f32 n1 = m5 * m5 + (m3 * m3 + m4 * m4);
    f32 inv1 = 1.0f / n1;
    f32 n0 = m2 * m2 + (m0 * m0 + m1 * m1);
    f32 inv0 = 1.0f / n0;
    f32 o3 = m1 * inv1;
    f32 o4 = m4 * inv1;
    f32 o0 = m0 * inv0;
    f32 o1 = m3 * inv0;
    f32 o5 = m7 * inv1;
    f32 o2 = m6 * inv0;
    f32 n2 = m8 * m8 + (m6 * m6 + m7 * m7);
    f32 inv2 = 1.0f / n2;
    f32 o7 = m5 * inv2;
    f32 o6 = m2 * inv2;
    f32 o8 = m8 * inv2;
    *(f32*)(out + 0x00) = o0;
    *(f32*)(out + 0x04) = o1;
    *(f32*)(out + 0x08) = o2;
    *(f32*)(out + 0x0C) = o3;
    *(f32*)(out + 0x10) = o4;
    *(f32*)(out + 0x14) = o5;
    *(f32*)(out + 0x18) = o6;
    *(f32*)(out + 0x1C) = o7;
    *(f32*)(out + 0x20) = o8;
    f32 t0 = -*(f32*)(in + 0x24);
    f32 t1 = -*(f32*)(in + 0x28);
    f32 t2 = -*(f32*)(in + 0x2C);
    *(f32*)(out + 0x24) = o6 * t2 + (o0 * t0 + o3 * t1);
    *(f32*)(out + 0x28) = o7 * t2 + (o1 * t0 + o4 * t1);
    *(f32*)(out + 0x2C) = o8 * t2 + (o2 * t0 + o5 * t1);
}

// 0x800756E0
// Instrument-object constructor: base setup, two controller-bound sub-objects, an array of four,
// zeroed slot tables, a 0x4428-byte helper object, and the "E_instr_haunted_organ" ids.
u8* fn_800756E0(u8* arg0) {
    fn_8006FA74(arg0, 1);
    *(void**)(arg0 + 0x4) = (void*)lbl_8035A950;
    fn_801FAAE0(arg0 + 0x8C, *(s32*)(lbl_803C4298 + 0x38));
    fn_801FAAE0(arg0 + 0x14C, *(s32*)(lbl_803C4298 + 0x38));
    fn_8032F1E8(arg0 + 0x20C, (void (*)(void))fn_8010B16C, (void (*)(void))fn_8010B184, 0xC, 4);
    *(s32*)(arg0 + 0x5D4) = 0;
    for (u8* e = arg0 + 0x5D8; e < arg0 + 0x6B8; e += 0x38) {
        *(s32*)(e + 0x8) = 0;
        *(s32*)(e + 0x2C) = 0;
        *(s32*)(e + 0x30) = 0;
        memset(e + 0xC, 0, 0x20);
    }
    *(f32*)(arg0 + 0x6B8) = 0.0f;
    *(f32*)(arg0 + 0x6BC) = 50.0f;
    *(s32*)(arg0 + 0x6C0) = 0;
    *(s32*)(arg0 + 0x6C4) = 0;
    *(s32*)(arg0 + 0x6C8) = 0;
    *(void**)(arg0 + 0x6CC) = NULL;
    *(s32*)(arg0 + 0x6D0) = 0;
    *(s32*)(arg0 + 0x6D4) = 0;
    *(s32*)(arg0 + 0x6D8) = 0;
    *(s32*)(arg0 + 0x6DC) = 0;
    *(u8**)lbl_805FB138 = arg0;
    memset(arg0 + 0x23C, 0, 0x190);
    memset(arg0 + 0x3E0, 0, 0x190);
    memset(arg0 + 0x3CC, 0, 0x14);
    memset(arg0 + 0x570, 0, 0x14);
    memset(arg0 + 0x584, 0, 0x10);
    fn_800759E8(arg0);
    u8* helper = (u8*)fn_801D40E0(0x4428);
    if (helper != NULL) helper = fn_80076D38(helper, lbl_8035A938);
    *(u8**)(arg0 + 0x6CC) = helper;
    fn_80075264(arg0, 0x377);
    *(s32*)(arg0 + 0x8) = 0x1013;
    *(s32*)(arg0 + 0xC) = 0x1014;
    *(u32*)(arg0 + 0x30) = 0x0C9524ACu;  // "E_instr_haunted_organ_1"
    *(u32*)(arg0 + 0x34) = 0x959C7516u;  // "E_instr_haunted_organ_2"
    return arg0;
}

// 0x80077D1C
// Welding-object constructor: base setup, two sub-objects and an array of twenty, clears the
// "active" bit of every prop in the world's zones, creates the helper, then scans zone 1's objects
// for the arc welder (stored at +0x75C) and the screws/bolts (their flag bits are cleared).
u8* fn_80077D1C(u8* arg0) {
    fn_8006FA74(arg0, 0);
    *(s32*)(arg0 + 0x8C) = 0;
    *(void**)(arg0 + 0x4) = (void*)lbl_8035AD08;
    fn_8007C4AC(arg0 + 0x1D0);
    fn_8007C4AC(arg0 + 0x210);
    *(s32*)(arg0 + 0x250) = 0;
    fn_8032F1E8(arg0 + 0x258, (void (*)(void))fn_8007C4AC, (void (*)(void))fn_8007C4CC, 0x40, 0x14);
    *(s32*)(arg0 + 0x758) = 0;
    *(void**)(arg0 + 0x75C) = NULL;
    *(void**)(arg0 + 0x760) = NULL;
    *(s32*)(arg0 + 0x10C4) = 0;
    *(s32*)(arg0 + 0x10C8) = 0;
    *(s32*)(arg0 + 0x10CC) = 0;
    *(s32*)(arg0 + 0x10E8) = 0;
    *(s32*)(arg0 + 0x10EC) = 0;
    *(s32*)(arg0 + 0x10F0) = 0;
    *(u8**)lbl_805FB140 = arg0;
    *(s32*)(arg0 + 0x8C) = 0xF;
    for (s32 i = 0; i <= *(s32*)(*(u8**)gWorldState + 0x4); i++) {
        for (u8 j = 0; j < *(u8*)(*(u8**)(*(u8**)gWorldState + 0x1A4) + i * gZoneResourceSize + 0xA4); ) {
            u8* e = *(u8**)(*(u8**)(*(u8**)gWorldState + 0x1A4) + i * gZoneResourceSize + 0xA0) + (u8)j * 0x4C;
            j++;
            *(u32*)(e + 0x30) &= ~1u;
        }
    }
    u8* helper = (u8*)fn_801D40E0(0x24);
    if (helper != NULL) helper = fn_8007CED8(helper);
    *(u8**)(arg0 + 0x760) = helper;
    fn_800784EC(arg0);
    fn_80078BB8(arg0);
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + (gZoneResourceSize * 2 - gZoneResourceSize);
    for (s32 i = 0; i < *(s32*)(*(u8**)gWorldState + 0x4); i++) {
        for (s32 k = 0; k < *(s32*)(zone + 0x98); k++) {
            u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + k * 4);
            u8* info = *(u8**)(obj + 0x9C);
            u32 type = (info != NULL) ? *(u32*)(info + 0x8) : 0;
            switch (type) {
            case kHashCastleArcWelderA:
                *(u8**)(arg0 + 0x75C) = obj;
                fn_8007B20C(obj);
                break;
            case kHashScrewBoltA:
            case kHashScrewFlatA:
            case kHashScrewPhilA: {
                u8* st = (*(GetObjFn*)(*(u8**)obj + 0xC))(obj);
                if (st != NULL) {
                    st = (*(GetObjFn*)(*(u8**)obj + 0xC))(obj);
                    *(u32*)(st + 0x8) &= ~2u;
                }
                u8* link = *(u8**)(obj + 0x4);
                *(u32*)(link + 0x20) &= ~2u;
                break;
            }
            }
        }
    }
    fn_80079004(arg0);
    fn_80075264(arg0, 0x3A0);
    *(s32*)(arg0 + 0x8) = 0x1091;
    *(s32*)(arg0 + 0xC) = 0x1092;
    *(u32*)(arg0 + 0x30) = 0xEC5A0BBEu;
    *(u32*)(arg0 + 0x34) = 0x75535A04u;
    return arg0;
}

// 0x8007D4DC
// Constructor for an experiment object (vtable lbl_8035AFD0): base setup, a controller-bound
// sub-object at +0xC8, zeroed state fields (some are bytes) and two follow-up init calls.
u8* fn_8007D4DC(u8* arg0) {
    fn_8006FA74(arg0, 3);
    *(s32*)(arg0 + 0x8C) = 0;
    *(void**)(arg0 + 0x4) = (void*)lbl_8035AFD0;
    *(f32*)(arg0 + 0xA8) = 0.0f;
    *(f32*)(arg0 + 0xB4) = 0.0f;
    *(f32*)(arg0 + 0xB8) = 0.0f;
    *(f32*)(arg0 + 0xBC) = 0.0f;
    *(u8*)(arg0 + 0xC0) = 0;
    fn_801F7800(arg0 + 0xC8, *(s32*)(lbl_803C4298 + 0x38), (void*)lbl_8035AFD0);
    *(s32*)(arg0 + 0x1F4) = 0;
    *(s32*)(arg0 + 0x298) = 0;
    *(s32*)(arg0 + 0x29C) = 0;
    *(s32*)(arg0 + 0x2A0) = 0;
    *(s32*)(arg0 + 0x2A8) = 0;
    *(s32*)(arg0 + 0x2AC) = 0;
    *(s32*)(arg0 + 0x31C) = 0;
    *(u8*)(arg0 + 0x320) = 0;
    *(f32*)(arg0 + 0x324) = 0.0f;
    *(f32*)(arg0 + 0x328) = 0.0f;
    *(u8*)(arg0 + 0x384) = 0;
    *(f32*)(arg0 + 0x388) = -1.0f;
    *(f32*)(arg0 + 0x38C) = 0.0f;
    *(u8*)(arg0 + 0x394) = 0;
    *(s32*)(arg0 + 0x398) = 0;
    *(s32*)(arg0 + 0x39C) = 0;
    *(s32*)(arg0 + 0x3A0) = 0;
    *(s32*)(arg0 + 0x3A4) = 0;
    *(s32*)(arg0 + 0x3A8) = 0;
    *(s32*)(arg0 + 0x3AC) = 0;
    *(f32*)(arg0 + 0x9C) = 0.0f;
    *(f32*)(arg0 + 0xA0) = 0.0f;
    *(f32*)(arg0 + 0xA4) = 0.0f;
    *(f32*)(arg0 + 0x90) = 0.0f;
    *(f32*)(arg0 + 0x94) = 0.0f;
    *(f32*)(arg0 + 0x98) = 0.0f;
    *(f32*)(arg0 + 0xB0) = 0.0f;
    *(f32*)(arg0 + 0xAC) = 0.0f;
    fn_8007DDEC(arg0, 0.0f);
    fn_8007D860(arg0);
    fn_80075264(arg0, 0x37B);
    *(s32*)(arg0 + 0x8) = 0x298;
    *(s32*)(arg0 + 0xC) = 0x299;
    *(u32*)(arg0 + 0x30) = 0xDB15586Cu;  // "E_instr_IQ_injector_1"
    *(u32*)(arg0 + 0x34) = 0x421C09D6u;
    return arg0;
}

// 0x80086BE8
// Constructor for an experiment object (vtable lbl_8035B830): base setup, two controller-bound
// sub-objects (+0x3C0, +0x730), a large zeroed state block (+0x94..+0x34F), default 320x240 / 1.0
// and 0.05 float fields, then the follow-up init.
u8* fn_80086BE8(u8* arg0) {
    fn_8006FA74(arg0, 8);
    *(s32*)(arg0 + 0x90) = 0;
    *(void**)(arg0 + 0x4) = (void*)lbl_8035B830;
    *(s32*)(arg0 + 0x350) = 0;
    *(s32*)(arg0 + 0x354) = 0;
    *(f32*)(arg0 + 0x3A8) = 0.0f;
    *(s32*)(arg0 + 0x3AC) = 0;
    *(s32*)(arg0 + 0x3B0) = 0;
    *(f32*)(arg0 + 0x3B4) = 0.0f;
    *(s32*)(arg0 + 0x3B8) = 0;
    *(f32*)(arg0 + 0x3BC) = 0.0f;
    fn_801F61AC(arg0 + 0x3C0, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(arg0 + 0x494) = 0;
    *(f32*)(arg0 + 0x484) = 0.0f;
    *(f32*)(arg0 + 0x488) = 0.0f;
    *(f32*)(arg0 + 0x48C) = 0.0f;
    *(f32*)(arg0 + 0x490) = 0.0f;
    *(s32*)(arg0 + 0x498) = 0;
    *(s32*)(arg0 + 0x49C) = 0;
    *(s32*)(arg0 + 0x4A0) = 0;
    *(s32*)(arg0 + 0x4A4) = 0;
    *(s32*)(arg0 + 0x4A8) = 5;
    *(f32*)(arg0 + 0x4AC) = 0.0f;
    fn_801F9544(arg0 + 0x730, *(s32*)(lbl_803C4298 + 0x38));
    *(u8*)(arg0 + 0x7E4) = 0;
    *(f32*)(arg0 + 0x7E8) = 0.0f;
    *(s32*)(arg0 + 0x7EC) = 0;
    for (s32 o = 0x80C; o <= 0x828; o += 4) *(s32*)(arg0 + o) = 0;
    *(f32*)(arg0 + 0x82C) = 320.0f;
    *(f32*)(arg0 + 0x830) = 240.0f;
    *(f32*)(arg0 + 0x834) = 1.0f;
    for (s32 o = 0x94; o < 0x350; o += 4) *(s32*)(arg0 + o) = 0;
    for (s32 o = 0; o < 10; o++) {
        *(s32*)(arg0 + 0x358 + o * 4) = 0;
        *(f32*)(arg0 + 0x380 + o * 4) = 0.05f;
    }
    fn_80086F84(arg0);
    fn_80075264(arg0, 0x378);
    *(s32*)(arg0 + 0x8) = 0x29D;
    *(s32*)(arg0 + 0xC) = 0x29E;
    *(u32*)(arg0 + 0x30) = 0xF0A517D5u;
    *(u32*)(arg0 + 0x34) = 0x69AC466Fu;
    return arg0;
}

// 0x8008ED38
// Constructor for an experiment object (vtable lbl_8035BC90): base setup, three arrays of three
// sub-objects (+0xA8, +0x180, +0x1F8), a controller-bound sub-object at +0x35C, zeroed state.
u8* fn_8008ED38(u8* arg0) {
    fn_8006FA74(arg0, 4);
    *(void**)(arg0 + 0x4) = (void*)lbl_8035BC90;
    *(s32*)(arg0 + 0x90) = 0;
    *(s32*)(arg0 + 0xA4) = 0;
    fn_8032F1E8(arg0 + 0xA8, (void (*)(void))fn_8009210C, (void (*)(void))fn_8009215C, 0x48, 3);
    fn_8032F1E8(arg0 + 0x180, (void (*)(void))fn_800938A4, (void (*)(void))fn_800938E0, 0x28, 3);
    fn_8032F1E8(arg0 + 0x1F8, (void (*)(void))fn_80091D04, (void (*)(void))fn_80091D28, 0x14, 3);
    fn_802168DC(arg0 + 0x2A8);
    *(s32*)(arg0 + 0x354) = 1;
    *(s32*)(arg0 + 0x358) = 0;
    fn_801F94B0(arg0 + 0x35C, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(arg0 + 0x7E0) = -1;
    *(u8*)(arg0 + 0x7E4) = 0;
    *(f32*)(arg0 + 0x7E8) = 0.0f;
    *(f32*)(arg0 + 0x7EC) = 0.0f;
    *(f32*)(arg0 + 0x7F0) = 0.0f;
    *(s32*)(arg0 + 0x7F4) = 0;
    *(s32*)(arg0 + 0x7F8) = 0;
    *(s32*)(arg0 + 0x7FC) = 0;
    *(f32*)(arg0 + 0x800) = 0.0f;
    *(f32*)(arg0 + 0x804) = 0.0f;
    *(s32*)(arg0 + 0x818) = 0;
    *(s32*)(arg0 + 0x81C) = 0;
    *(s32*)(arg0 + 0x820) = 0;
    *(s32*)(arg0 + 0x824) = 0;
    for (s32 i = 0; i < 3; i++) {
        *(s32*)(arg0 + i * 4 + 0x98) = 0;
        fn_800923DC(arg0 + i * 0x48 + 0xA8);
        fn_80093A88(arg0 + i * 0x28 + 0x180);
        fn_80091E44(arg0 + i * 0x14 + 0x1F8);
    }
    for (s32 o = 0x284; o <= 0x2A4; o += 4) *(s32*)(arg0 + o) = 0;
    fn_8008F068(arg0);
    fn_80075264(arg0, 0x3A1);
    *(s32*)(arg0 + 0x8) = 0xFCC;
    *(s32*)(arg0 + 0xC) = 0xFCD;
    *(u32*)(arg0 + 0x30) = 0x4436EAC4u;
    *(u32*)(arg0 + 0x34) = 0xDD3FBB7Eu;
    return arg0;
}

// 0x800940BC
// Constructor for an experiment object (vtable lbl_8035C208): base setup, two controller-bound
// sub-objects (+0x1E0, +0x294), a random float at +0x368 in [0,1), an array of 27 at +0x384 and a
// zeroed block (+0x9C..+0x1D7).
u8* fn_800940BC(u8* arg0) {
    fn_8006FA74(arg0, 5);
    *(void**)(arg0 + 0x4) = (void*)lbl_8035C208;
    *(s32*)(arg0 + 0x90) = 0;
    *(s32*)(arg0 + 0x94) = 0;
    *(s32*)(arg0 + 0x98) = 0;
    *(s32*)(arg0 + 0x1B4) = 0;
    *(s32*)(arg0 + 0x1D8) = 0;
    *(s32*)(arg0 + 0x1DC) = 0;
    fn_801F9544(arg0 + 0x1E0, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F6848(arg0 + 0x294, *(s32*)(lbl_803C4298 + 0x38));
    *(f32*)(arg0 + 0x358) = 0.0f;
    *(f32*)(arg0 + 0x35C) = 0.0f;
    *(f32*)(arg0 + 0x360) = 0.0f;
    *(f32*)(arg0 + 0x364) = 0.0f;
    u32 rnd = RandomU32();
    *(f32*)(arg0 + 0x36C) = 0.0f;
    *(u8*)(arg0 + 0x370) = 0;
    *(f32*)(arg0 + 0x374) = 0.0f;
    *(f32*)(arg0 + 0x378) = 0.0f;
    *(f32*)(arg0 + 0x368) = 2.3283064e-10f * (f32)rnd;
    *(s32*)(arg0 + 0x37C) = 0;
    fn_8032F1E8(arg0 + 0x384, (void (*)(void))fn_80097574, (void (*)(void))fn_80097590, 0x20, 0x1B);
    for (s32 o = 0x9C; o < 0x1D8; o += 4) *(s32*)(arg0 + o) = 0;
    fn_800943CC(arg0);
    fn_80075264(arg0, 0x3A3);
    *(s32*)(arg0 + 0x8) = 0xFB9;
    *(s32*)(arg0 + 0xC) = 0xFBA;
    *(u32*)(arg0 + 0x30) = 0xBA036DE3u;
    *(u32*)(arg0 + 0x34) = 0x230A3C59u;
    return arg0;
}

// 0x80099AF4
// Constructor for an experiment object (vtable lbl_8035C890): base setup, sub-objects at +0x9C and
// +0x180, zeroed state, then looks up the prop of the +0x180 sub-object, sets flag bit 0x10000 on
// its animation entry and bit 1 of its +0x15C word, and kicks its first object with time 0.
u8* fn_80099AF4(u8* arg0) {
    fn_8006FA74(arg0, 0xC);
    *(void**)(arg0 + 0x4) = (void*)lbl_8035C890;
    *(f32*)(arg0 + 0x8C) = 0.0f;
    *(f32*)(arg0 + 0x94) = 0.0f;
    *(s32*)(arg0 + 0x98) = -1;
    fn_8009BE14(arg0 + 0x9C);
    fn_8009B264(arg0 + 0x180);
    *(s32*)(arg0 + 0x198) = 0;
    *(s32*)(arg0 + 0x19C) = 0;
    *(s32*)(arg0 + 0x1A0) = 0;
    *(f32*)(arg0 + 0x1BC) = 0.0f;
    *(f32*)(arg0 + 0x1C0) = 0.0f;
    *(s32*)(arg0 + 0x1A4) = 0;
    *(s32*)(arg0 + 0x1B0) = 0;
    *(s32*)(arg0 + 0x1A8) = 0;
    *(s32*)(arg0 + 0x1B4) = 0;
    *(s32*)(arg0 + 0x1AC) = 0;
    *(s32*)(arg0 + 0x1B8) = 0;
    if (lbl_803462C8[0] > 0.0f) {
        *(f32*)(arg0 + 0x8C) = lbl_803462C8[RaySceneTest(NULL)];
    }
    fn_80099D08(arg0);
    fn_8009B320(arg0 + 0x180);
    u8* prop = fn_800E4D7C(0);  // TODO: asm passes whatever fn_8009B320 leaves in r3 (unknown)
    u8* anim = fn_801AC184(*(u8**)(*(u8**)(fn_800E4BFC(prop) + 0x9C) + 0x48), *(s32*)(prop + 0x5C));
    *(u32*)(anim + 0xC) |= 0x10000;
    *(u8*)(prop + 0x54) = 1;
    u8* holder = fn_800E4BFC(prop);
    u8* obj = (*(GetObjFn*)(*(u8**)holder + 0x10))(holder);
    fn_800E4CB0(prop);
    fn_8018A948(obj, 0.0f);
    u8* blk = fn_800E4BFC(prop);
    *(u32*)(blk + 0x15C) |= 1;
    fn_80075264(arg0, 0x379);
    *(s32*)(arg0 + 0x8) = 0x15C;
    *(s32*)(arg0 + 0xC) = 0x15D;
    *(u32*)(arg0 + 0x30) = 0x62455C26u;
    *(u32*)(arg0 + 0x34) = 0xFB4C0D9Cu;
    return arg0;
}

// 0x800980F8
// Reactor-leak seal level setter: when the level is non-zero and being set to 0, resets the timers,
// feeds the delta to fn_800999B4 and plays the seal-off sound. Otherwise clamps the new level to the
// per-difficulty maximum, stores it, and blends the two colour constants by level/max minus the
// object's current colour (from the shader-dispatch table), writing the colour delta to the target.
void fn_800980F8(u8* arg0, s32 arg1) {
    if (*(u8*)(arg0 + 0x26C) != 0 && arg1 == 0) {
        *(f32*)(arg0 + 0x270) = lbl_80347030[RaySceneTest(NULL)];
        *(f32*)(arg0 + 0x274) = fn_800999B4(*(u8**)(arg0 + 0x264), lbl_80347020[RaySceneTest(NULL)]);
        PlayExperimentSound(NULL, StringHash("reactor_leak_leak_seal_off_01"), 1);
    }
    s32 maxLevel = lbl_80347170[RaySceneTest(NULL)];
    s32 level = arg1;
    if (level < 0) level = 0;
    else if (level > maxLevel) level = maxLevel;
    *(u8*)(arg0 + 0x26C) = (u8)level;
    *(f32*)(arg0 + 0x278) = -1.0f;
    u8* obj = (*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0);
    u8* info = *(u8**)(fn_801DA788(obj, 0) + 0x10);
    u8* node = *(u8**)(info + 0x70);
    f32 cur[4];
    for (s32 i = 0; i < 4; i++) {
        u8* ent = lbl_80348F38 + *(u8*)(node + 0xC) * 4;
        u32 bits = (*(GetBitsFn*)(ent + 0x30))(*(void**)(node + 0x8), 0x135 + i, ent);
        cur[i] = *(f32*)&bits;
    }
    f32 t = (f32)*(u8*)(arg0 + 0x26C) / (f32)maxLevel;
    if ((s8)lbl_805FB148 == 0) {
        lbl_803C6AC0[0] = 0.2f;
        lbl_803C6AC0[1] = 0.2f;
        lbl_803C6AC0[2] = 0.2f;
        lbl_803C6AC0[3] = 1.0f;
        lbl_805FB148 = 1;
    }
    if ((s8)lbl_805FB149 == 0) {
        lbl_803C6AD0[0] = 1.0f;
        lbl_803C6AD0[1] = 0.7f;
        lbl_803C6AD0[2] = 0.2f;
        lbl_803C6AD0[3] = 1.0f;
        lbl_805FB149 = 1;
    }
    f32 dr = lbl_803C6AD0[0] - lbl_803C6AC0[0];
    f32 dg = lbl_803C6AD0[1] - lbl_803C6AC0[1];
    f32 db = lbl_803C6AD0[2] - lbl_803C6AC0[2];
    f32 da = lbl_803C6AD0[3] - lbl_803C6AC0[3];
    f32 r = lbl_803C6AC0[0] + dr * t;
    f32 g = lbl_803C6AC0[1] + dg * t;
    f32 b = lbl_803C6AC0[2] + db * t;
    f32 outR = r - cur[0];
    f32 outG = g - cur[1];
    f32 outB = b - cur[2];
    f32 outA = 1.0f - cur[3];
    u8* obj2 = (*(GetObjFn*)(*(u8**)arg0 + 0xC))(arg0);
    u8* dst = fn_801DA788(obj2, 0);
    *(f32*)(dst + 0x5C) = outR;
    *(f32*)(dst + 0x60) = outG;
    *(f32*)(dst + 0x64) = outB;
    *(f32*)(dst + 0x68) = outA;
    (void)da;
}

// 0x801987BC
// Pushes a screen onto the screen set's stack (up to the count at +0x60, entries from +0x40).
// Logs the push, refuses with -1 if the screen is already on the stack, otherwise activates it
// (virtual slot 0x2C of the vtable at +0x138), registers its callback, and refreshes the set.
s32 fn_801987BC(u8* arg0, u8* arg1) {
    char buf[64];
    memset(buf, 0, sizeof(buf));
    SysSprintf(buf, "PushScreen, Screen: %d", *(s32*)(arg1 + 0x0));
    ZoneLog(gIdPool, buf);
    s32 n = *(s32*)(arg0 + 0x60);
    for (s32 i = 0; i < n; i++) {
        if (*(u8**)(arg0 + 0x40 + i * 4) == arg1) {
            fn_8018D2B8("ScreenSet - Screen duplicate already on stack ...\n");
            return -1;
        }
    }
    typedef void (*ActivateFn)(u8* self, s32 a);
    (*(ActivateFn*)(*(u8**)(arg1 + 0x138) + 0x2C))(arg1, 1);
    fn_8019BFF8(arg1, (void*)fn_80199054, NULL);
    s32 cnt = *(s32*)(arg0 + 0x60);
    *(u8**)(arg0 + 0x40 + cnt * 4) = arg1;
    *(s32*)(arg0 + 0x60) = cnt + 1;
    if (*(s32*)(arg1 + 0x144) != 0) fn_8019A994(*(void**)(arg1 + 0x144));
    fn_80199618(arg1);
    fn_80198290((u8*)lbl_805FB670, 2);
    fn_8019829C((u8*)lbl_805FB670, *(f32*)(arg1 + 0x148));
    fn_8018DEF4(gScreenSets);
    *(s32*)((u8*)gScreenSets + 0x74) = 0;
    return 0;
}

}  // extern "C"
