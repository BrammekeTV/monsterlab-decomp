// Drafts miss762: hard cases skipped earlier (jump tables, tail calls, float-heavy bodies).
#include <math.h>
#include <stdint.h>
#include "types.h"
#include "revolution.h"
#include "engine/math.h"

extern "C++" void* gEventMgr;            // 0x805FB6C8

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  GameFlowSignal(s32 id, s32 a, s32 b);                     // 0x8000C7F4
void  PlayExperimentSound(void* owner, u32 id, s32 vol);
s32   RaySceneTest(void* ray);                                  // 0x80070458 (ignores its argument)
void  TimedRumble(s32 chan, f32 strength, u32 a2);
u8*   fn_8006FBDC(void);                                        // 0x8006FBDC
void  fn_800712B4(void);                                        // 0x800712B4
void  fn_800712B8(void);                                        // 0x800712B8
s32   fn_8009D130(u8* p, u8* q, f32* out);                      // 0x8009D130
void  fn_8009D710(u8* p);                                       // 0x8009D710
void  fn_800CFDCC(u8* p, f32 x);                                // 0x800CFDCC
void* fn_800CF71C(void);                                        // 0x800CF71C
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);     // 0x80146180 (rotate quat about axis)

extern f32 lbl_80346328[];      // 0x80346328
extern f32 lbl_80346348[];      // 0x80346348
extern f32 lbl_80346368[];      // 0x80346368
extern f32 lbl_80346498[];      // 0x80346498
extern f32 lbl_803464A8[];      // 0x803464A8
extern f32 lbl_80347290[];      // 0x80347290
extern u8  lbl_803C4298[];      // 0x803C4298 (+0x4/+0x8: objects with vtables, +0x38: pad channel)
extern f32 lbl_805FDD78;        // 0x805FDD78 (0.0)
extern f32 lbl_805FDDB0;        // 0x805FDDB0 (100000.0)
extern f32 lbl_805FDDB4;        // 0x805FDDB4 (100.0)
extern f32 lbl_805FDDB8;        // 0x805FDDB8 (0.2)
extern f32 lbl_805FE558;        // 0x805FE558 (0.0)
extern f32 lbl_805FE594;        // 0x805FE594 (-70.0)
extern f32 lbl_805FE598;        // 0x805FE598 (1.0)
extern f32 lbl_805FE5A4;        // 0x805FE5A4 (0.2)

extern u8  lbl_80348950[];     // 0x80348950 (0x20-byte records, u32 at +0)
extern u8  lbl_80348A10[];     // 0x80348A10 (0x20-byte records, s32 at +0)
extern s32 lbl_8040ADF0[8];    // 0x8040ADF0
extern s32 lbl_8040AE10[8];    // 0x8040AE10
extern s32 lbl_805FB4B0;       // 0x805FB4B0

void  EulerToQuat(Quat* out, const Vec3* euler);                // 0x80145B00
Vec3  QuatGetAxisY(const Quat* q);                              // 0x801452A8
void  fn_800C1798(void* out, void* arg);                        // 0x800C1798
void  fn_800C1B78(u8* p);                                       // 0x800C1B78
void  fn_800C72B4(u8* item, u8* base);                          // 0x800C72B4
void* fn_800C73D0(u8* p);                                       // 0x800C73D0 (returns a pointer)
u8*   fn_800C7974(u8* p);                                       // 0x800C7974
void  fn_800C98F8(u8* arg0, u8* arg1, u8* arg2);                // 0x800C98F8
void  fn_800CA290(u8* arg0, u8* arg1);                          // 0x800CA290
s32   fn_800CD4E8(u8* p);                                       // 0x800CD4E8
s32   fn_800CD744(u8* p);                                       // 0x800CD744
void  fn_8010B4AC(u8* a3);                                      // 0x8010B4AC
void  fn_8010B4C0(u8* obj, s32 v);                              // 0x8010B4C0
void  fn_8010B7C8(u8* arg0, u8* arg1, u8* arg2);                // 0x8010B7C8
s32   fn_801449A4(f32* mat, f32* out);                          // 0x801449A4 (3x3 matrix to euler angles)
void  fn_801458B0(f32* quat, f32* a, f32* b);                   // 0x801458B0

extern f32 lbl_805FE4B8;       // 0x805FE4B8 (0.0)
extern f32 lbl_805FE4BC;       // 0x805FE4BC (1.0)
extern f32 lbl_805FE708;       // 0x805FE708 (0.0)
extern f32 lbl_805FE70C;       // 0x805FE70C (1.0)
extern f32 lbl_805FE74C;       // 0x805FE74C (3.0)
extern f32 lbl_805FE758;       // 0x805FE758 (0.5)
extern f32 lbl_805FE76C;       // 0x805FE76C (0.01)
extern f32 lbl_805FF6F8;       // 0x805FF6F8 (1.0)
extern u8  lbl_805FB1A1;       // 0x805FB1A1
extern u8* lbl_805FB608;       // 0x805FB608 (heap object: slot +0 = alloc)

int   SysSprintf(char* buf, const char* fmt, ...);              // 0x80142EF8
void  fn_800D2B1C(u8* arg0, u8* arg1, f32 dt);                  // 0x800D2B1C
void  fn_80121508(char* text, s32 x, s32 y, s32 a, s32 b, s32 c, f32 d, f32 e);  // 0x80121508 (draws a text line)
s32   fn_80156A80(void);                                        // 0x80156A80 (screen width)
extern s32 gScreenHeight;      // 0x805F938C
extern s32 gScreenWidth;       // 0x805F9388
extern u8  lbl_803C2A20[];     // 0x803C2A20 (+0x820: flag byte)
extern u8  lbl_8035F898[];     // 0x8035F898 (debug overlay strings)
extern const char lbl_805F8CB0[];   // 0x805F8CB0 ("%s")
BOOL  PadButtonHeld(s32 button);                                // 0x801474EC
BOOL  fn_80147554(s32 button);                                  // 0x80147554
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);  // 0x80209E70
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
void* fn_80209814(void* v);                                     // 0x80209814
void  fn_80070FAC(u8* p, f32 v);                                // 0x80070FAC
void  fn_800B59E8(u8* p, f32 dt);                               // 0x800B59E8
void  fn_800B5B20(u8* p, f32 v);                                // 0x800B5B20
void  fn_800B5B30(u8* p);                                       // 0x800B5B30
f32   fn_800B5B44(u8* p);                                       // 0x800B5B44
s32   fn_800B5B4C(u8* p);                                       // 0x800B5B4C
void* fn_800B5B54(u8* p);                                       // 0x800B5B54
void  fn_800B6510(u8* p, f32 a, f32 b);                         // 0x800B6510
void  fn_800B6598(u8* p, f32 a, f32 b);                         // 0x800B6598
s32   fn_800B66D0(u8* p);                                       // 0x800B66D0
s32   fn_800B6848(u8* p);                                       // 0x800B6848
void  fn_800B68C0(u8* p, s32 mode, f32 a, f32 b);               // 0x800B68C0
void  fn_800B71B0(u8* p, f32 unused, f32 scale);                // 0x800B71B0
void  fn_800B7228(u8* p, f32 unused, f32 scale);                // 0x800B7228
void  fn_800B72A0(u8* p, f32 scale);                            // 0x800B72A0
void  fn_800B7310(u8* p, f32 scale);                            // 0x800B7310
f32   fn_800B7400(u8* p, f32 a, f32 b);                         // 0x800B7400
f32   fn_800B7414(u8* p, f32 a, f32 b);                         // 0x800B7414
s32   fn_800B964C(u8* obj);                                     // 0x800B964C
void  fn_800B999C(u8* obj, s32 a, s32 b);                       // 0x800B999C
void  fn_800712B4(void);                                        // 0x800712B4
void  fn_800712B8(void);                                        // 0x800712B8

extern f32 lbl_803462B8[];     // 0x803462B8
extern f32 lbl_80346088[];     // 0x80346088
extern f32 lbl_80346298[];     // 0x80346298
extern f32 lbl_803462A8[];     // 0x803462A8
extern f32 lbl_80346228[];     // 0x80346228
extern f32 lbl_80346238[];     // 0x80346238
extern f32 lbl_80346058[];     // 0x80346058
extern f32 lbl_805FE108;       // 0x805FE108 (0.0)
extern f32 lbl_805FE114;       // 0x805FE114 (0.5)
extern f32 lbl_805FE118;       // 0x805FE118 (100.0)
extern f32 lbl_805FE11C;       // 0x805FE11C (1.0)
extern f32 lbl_805FE160;       // 0x805FE160 (15.0)
extern f32 lbl_805FE164;       // 0x805FE164 (2.0)
extern f32 lbl_805FE168;       // 0x805FE168 (0.25)
extern f32 lbl_805FE16C;       // 0x805FE16C (20.0)
extern void* lbl_805FBBAC;     // 0x805FBBAC (voice SFX handle)

u32   RandomU32(void);                                          // 0x80143460
s32   fn_80332EE0(s32 x);                                       // 0x80332EE0 (abs)
f64   fn_8033ED6C(f64 x);                                       // 0x8033ED6C (MSL floor)
void  fn_800C176C(f32* out, u8* obj);                           // 0x800C176C
void  fn_800C694C(u8* arg0, u8* arg1);                          // 0x800C694C
f32   fn_800C6A0C(u8* a3, u8* a4);                              // 0x800C6A0C
s32   fn_800C73B4(u8* p);                                       // 0x800C73B4 (returns a pointer; the generated leaf declares s32)
u8*   fn_800C7928(u8* p);                                       // 0x800C7928
void  fn_800CABD0(u8* center, u8* arr, u32 count);              // 0x800CABD0
s32   fn_800CBD4C(u8* arg0, f32* arg1, f32* arg2);              // 0x800CBD4C
s32   fn_800CD51C(u8* p);                                       // 0x800CD51C
s32   fn_800CD59C(u8* p);                                       // 0x800CD59C
s32   fn_800CD67C(u8* p);                                       // 0x800CD67C
s32   fn_800CD6E0(u8* p);                                       // 0x800CD6E0

extern f32 lbl_80347320[];     // 0x80347320
extern f32 lbl_80347330[];     // 0x80347330
extern f32 lbl_80347340[];     // 0x80347340
extern f32 lbl_80347350[];     // 0x80347350
extern f32 lbl_805FE4D8;       // 0x805FE4D8 (-1.0)
extern f32 lbl_805FE4E4;       // 0x805FE4E4 (2^-32)
extern f32 lbl_805FE500;       // 0x805FE500 (0.5)
extern f32 lbl_805FE504;       // 0x805FE504 (1e-10)

void  fn_800CB2D8(u8* arg0);                                    // 0x800CB2D8
}

// Writes one BP register through the write-gather pipe (opcode 0x61 then the 32-bit word).
static inline void GxBp(u32 v) {
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = v;
}

extern "C" {

// 0x8009DF34
// Applies one hit of the experiment to the current object: nudges the +0x1C0 meter (clamped to
// 0..100) by a table value picked from RaySceneTest, and either rumbles and signals the object
// (when fn_8009D130 reports a clear result) or plays the failure sounds and stores a table value at +0x3F4.
void fn_8009DF34(u8* arg0) {
    f32 out;
    u8* p = fn_8006FBDC();
    u8* o = *(u8**)(p + 0x198);

    out = lbl_805FDDB0;
    if (fn_8009D130(o, arg0, &out) != 0) {
        f32 d = lbl_80346498[RaySceneTest(0)];
        if (d > lbl_805FDD78) {
            fn_800712B8();
        } else if (d < lbl_805FDD78) {
            fn_800712B4();
        }
        f32 v = *(f32*)(p + 0x1C0) + d;
        f32 r;
        if (v < lbl_805FDD78) {
            r = lbl_805FDD78;
        } else if (v > lbl_805FDDB4) {
            r = lbl_805FDDB4;
        } else {
            r = v;
        }
        *(f32*)(p + 0x1C0) = r;
        TimedRumble(*(s32*)(lbl_803C4298 + 0x38), lbl_805FDDB8, 4);
        EventSend(gEventMgr, o, NULL, 0x603649F2u, 0, 0, 1, 0);
        fn_8009D710(o);
    } else {
        f32 d = lbl_803464A8[RaySceneTest(0)];
        if (d > lbl_805FDD78) {
            fn_800712B8();
        } else if (d < lbl_805FDD78) {
            fn_800712B4();
        }
        f32 v = *(f32*)(p + 0x1C0) + d;
        f32 r;
        if (v < lbl_805FDD78) {
            r = lbl_805FDD78;
        } else if (v > lbl_805FDDB4) {
            r = lbl_805FDDB4;
        } else {
            r = v;
        }
        *(f32*)(p + 0x1C0) = r;
        PlayExperimentSound(NULL, 0xEFDBA405u, 0);
        if (out < lbl_80346368[RaySceneTest(0)]) {
            *(f32*)(o + 0x3F4) = lbl_80346348[RaySceneTest(0)];
        } else {
            PlayExperimentSound(NULL, 0x2A2C19C0u, 0);
            PlayExperimentSound(NULL, 0x27B8449Du, 0);
            *(f32*)(o + 0x3F4) = lbl_80346328[RaySceneTest(0)];
        }
    }
}

// 0x800CF2F8
// Per-frame step of a camera-attached effect: pushes the scale/offset through the vtable of the
// block at +0x3020, moves the level objects (lbl_803C4298 +4/+8), sets the effect record's position
// (+0x58) and rotation (+0x64), advances the timer at +0x8C and signals the game flow after 0.2 s.
void fn_800CF2F8(u8* arg0, f32 dt) {
    typedef void (*VFnF)(void* self, f32 v);
    typedef void (*VFnOut)(f32* out, void* self);
    typedef void (*VFnP2)(void* self, f32* a, f32* b);
    typedef void (*VFnP1)(void* self, f32* a);

    f32 pos[3];
    f32 off[3];
    f32 quat[4];
    f32 axis[3];
    void* blk = arg0 + 0x3020;

    (*(VFnF*)(*(u8**)(arg0 + 0x30F4) + 0x1C))(blk, dt * lbl_80347290[RaySceneTest(0)]);
    pos[0] = lbl_805FE558;
    pos[1] = lbl_805FE594;
    pos[2] = lbl_805FE558;
    (*(VFnF*)(*(u8**)(arg0 + 0x30F4) + 0x20))(blk, lbl_805FE558);
    (*(VFnOut*)(*(u8**)(arg0 + 0x30F4) + 0x24))(off, blk);
    pos[0] = pos[0] + off[0];
    pos[1] = pos[1] + off[1];
    pos[2] = pos[2] + off[2];

    void* o4 = *(void**)(lbl_803C4298 + 0x4);
    (*(VFnP2*)(*(u8**)o4 + 0x2C))(o4, pos, off);
    o4 = *(void**)(lbl_803C4298 + 0x4);
    (*(VFnP1*)(*(u8**)o4 + 0x1C))(o4, pos);
    void* o8 = *(void**)(lbl_803C4298 + 0x8);
    (*(VFnF*)(*(u8**)o8 + 0x98))(o8, lbl_805FE558);
    fn_800CFDCC(arg0 + 0x90, dt);

    u8* rec = (u8*)fn_800CF71C();
    quat[0] = lbl_805FE558;
    quat[1] = lbl_805FE558;
    quat[2] = lbl_805FE558;
    quat[3] = lbl_805FE598;
    axis[0] = lbl_805FE558;
    axis[1] = lbl_805FE598;
    axis[2] = lbl_805FE558;
    fn_80146180(quat, (const Vec3*)axis, *(f32*)(arg0 + 0xA0));
    *(f32*)(rec + 0x58) = pos[0];
    *(f32*)(rec + 0x5C) = pos[1];
    *(f32*)(rec + 0x60) = pos[2];
    *(f32*)(rec + 0x64) = quat[0];
    *(f32*)(rec + 0x68) = quat[1];
    *(f32*)(rec + 0x6C) = quat[2];
    *(f32*)(rec + 0x70) = quat[3];

    f32 t = *(f32*)(arg0 + 0x8C) + dt;
    *(f32*)(arg0 + 0x8C) = t;
    if (t >= lbl_805FE5A4) {
        GameFlowSignal((s32)0x88C6E941, 0, 0);
    }
}

// 0x8015A39C
// Programs the GX copy/texture state from a 0x20-byte descriptor: size, format, address
// (re-sent only when the address or the shadow word changed), optional second image.
void fn_8015A39C(u8* arg0) {
    GxBp(0xFE000060u);
    GxBp(((u32)*(u8*)(arg0 + 0x12) << 5) | 0x80000000u);
    if ((u32)lbl_8040ADF0[0] != *(u32*)(arg0 + 0x18) || lbl_8040AE10[0] != 0) {
        lbl_805FB4B0 = lbl_805FB4B0 + 1;
        lbl_8040ADF0[0] = *(s32*)(arg0 + 0x18);
        lbl_8040AE10[0] = 0;
        u8 tex = *(u8*)(arg0 + 0x13);
        s32 second = *(s32*)(lbl_80348A10 + tex * 0x20);
        GxBp((((u32)*(u8*)(arg0 + 0xF) - 1) & 0xF) << 12 | 0x84000000u);
        GxBp(((u32)*(u16*)(arg0 + 0x4) - 1) | (((u32)*(u16*)(arg0 + 0x6) - 1) << 10) |
             ((u32)*(u8*)(arg0 + 0x10) << 20) | 0x88000000u);
        GxBp((*(u32*)(lbl_80348950 + tex * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
        if (second >= 0) {
            GxBp(((u32)second >> 5) | 0x900D0000u | 0x8000u);
        } else {
            GxBp(0x90000000u);
        }
        GxBp(((*(u32*)(arg0 + 0x18) >> 5) & 0xFFFFFF) | 0x94000000u);
        if (*(u8*)(arg0 + 0xD) != 0) {
            GxBp(((u32)*(u8*)(arg0 + 0x11) << 10) | 0x98000200u);
            GxBp(0xFEFFFF00u);
            GxBp(0x0F000000u);
            GxBp(((*(u32*)(arg0 + 0x1C) >> 5) & 0xFFFFFF) | 0x64000000u);
            GxBp((((u32)*(u8*)(arg0 + 0xD) << 9) & 0xFFFFFC00u) | 0x65000200u);
            GxBp(0xFEFFFF00u);
            GxBp(0x0F000000u);
        }
    }
    GxBp(((u32)*(u16*)(arg0 + 0x4) - 1) | 0x30000000u);
    GxBp(((u32)*(u16*)(arg0 + 0x6) - 1) | 0x31000000u);
}

// Clamps the RaySceneTest result to the 0..3 table range.
static s32 ClampIdx3(s32 v) {
    if (v < 0) return 0;
    if (v > 3) return 3;
    return v;
}

// Makes sure the shared voice SFX handle exists.
static void EnsureVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
}

// Stops every active object in the five 0x14-byte slots at +0xA8 (state 2) and clears its timer at +0xB0.
static void ClearPowerupSlots(u8* p) {
    f32 zero = lbl_805FE108;
    u8* s = p;
    for (s32 i = 0; i < 5; i++) {
        u8* o = *(u8**)(s + 0xA8);
        if (o != NULL && fn_800B964C(o) == 2) {
            *(f32*)(s + 0xB0) = zero;
            fn_800B999C(*(u8**)(s + 0xA8), 0, 0);
        }
        s += 0x14;
    }
}

// 0x800B49B8
// Per-frame update of the shake / cyclic-motion minigame state in arg0: ages the six timers,
// turns a spike (Wii remote button 0x1039 / Nunchuk 0x103B or a recognized gesture) into a hit
// on the object at +0x94, applies the score changes and wind/shake feedback for each state of
// that object, formats the two cyclic-motion debug lines and runs the power-up (+0x620) result
// handling (succeed = state 3, failed = state 4).
void fn_800B49B8(u8* arg0, f32 dt) {
    s32 hitA = 0;
    s32 hitB = 0;
    u8* obj;

    *(s32*)(arg0 + 0x614) = *(s32*)(arg0 + 0x618);
    fn_800B59E8(arg0 + 0x620, dt);
    *(f32*)(arg0 + 0xA18) = *(f32*)(arg0 + 0xA18) + dt;
    *(f32*)(arg0 + 0xA20) = *(f32*)(arg0 + 0xA20) + dt;
    *(f32*)(arg0 + 0xA1C) = *(f32*)(arg0 + 0xA1C) + dt;
    *(f32*)(arg0 + 0x994) = *(f32*)(arg0 + 0x994) + dt;
    *(f32*)(arg0 + 0xA24) = *(f32*)(arg0 + 0xA24) + dt;
    *(f32*)(arg0 + 0xA28) = *(f32*)(arg0 + 0xA28) + dt;

    obj = *(u8**)(arg0 + 0x94);
    if (fn_800B6848(obj) != 3) {
        // Wii remote spike
        if (*(u8*)(arg0 + 0x33C) != 0) {
            f32 v = *(f32*)(arg0 + 0x340);
            if (v > lbl_805FE108) *(f32*)(arg0 + 0x2B4) = v;
        }
        s32 spike = 0;
        if (*(u8*)(arg0 + 0x33C) != 0 || fn_80147554(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0x1039) != 0) {
            spike = 1;
        }
        if (spike != 0) {
            *(f32*)(arg0 + 0x680) = lbl_805FE114;
            SysSprintf((char*)(arg0 + 0x684), "WiiMote: Spike Motion Detected!");
            f32 rate = lbl_805FE160 / dt;
            fn_800B68C0(*(u8**)(arg0 + 0x94), 2, rate, dt);
            fn_800B71B0(*(u8**)(arg0 + 0x94), rate, dt);
            *(s32*)(arg0 + 0x618) = 0;
            hitA = 1;
        }
        // Nunchuk spike
        if (*(u8*)(arg0 + 0x4F0) != 0) {
            f32 v = *(f32*)(arg0 + 0x4F4);
            if (v > lbl_805FE108) *(f32*)(arg0 + 0x468) = v;
        }
        spike = 0;
        if (*(u8*)(arg0 + 0x4F0) != 0 || fn_80147554(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0x103B) != 0) {
            spike = 1;
        }
        if (spike != 0) {
            *(f32*)(arg0 + 0x788) = lbl_805FE114;
            SysSprintf((char*)(arg0 + 0x78C), "Nunchuk: Spike Motion Detected!");
            f32 rate = lbl_805FE160 / dt;
            fn_800B68C0(*(u8**)(arg0 + 0x94), 1, rate, dt);
            fn_800B7228(*(u8**)(arg0 + 0x94), rate, dt);
            hitB = 1;
            *(s32*)(arg0 + 0x618) = 1;
        }
    }

    if (*(s32*)(arg0 + 0x618) != *(s32*)(arg0 + 0x614)) {
        if (fn_800B66D0(*(u8**)(arg0 + 0x94)) == 0) {
            s32 i = ClampIdx3(RaySceneTest(0));
            *(f32*)(arg0 + 0x67C) = *(f32*)(arg0 + 0x67C) + lbl_803462B8[i];
            fn_800712B8();
            fn_800B6510(*(u8**)(arg0 + 0x94), dt, lbl_805FE108);
            EventSend(gEventMgr, *(u8**)(arg0 + 0x94), NULL, 0x8D10DAEBu, 0, 0, 1, 0);
        }
    }
    if (hitA != 0) *(f32*)(arg0 + 0xA24) = lbl_805FE108;
    if (hitB != 0) *(f32*)(arg0 + 0xA28) = lbl_805FE108;

    if (fn_800B6848(*(u8**)(arg0 + 0x94)) != 3) {
        s32 i = ClampIdx3(RaySceneTest(0));
        if (*(f32*)(arg0 + 0xA24) > lbl_80346088[i]) {
            fn_800B72A0(*(u8**)(arg0 + 0x94), dt);
        }
        i = ClampIdx3(RaySceneTest(0));
        if (*(f32*)(arg0 + 0xA28) > lbl_80346088[i]) {
            fn_800B7310(*(u8**)(arg0 + 0x94), dt);
        }
    }
    if (hitA == 0 && hitB == 0) {
        fn_800B68C0(*(u8**)(arg0 + 0x94), 0, lbl_805FE108, dt);
    }

    // cyclic-motion debug lines (Wii remote at +0x708, Nunchuk at +0x810)
    s32 dirA = *(s32*)(arg0 + 0x42C);
    *(f32*)(arg0 + 0x704) = lbl_805FE164;
    if (dirA == 2) {
        SysSprintf((char*)(arg0 + 0x708), "Cycl-freq %.3f %s\n", *(f32*)(arg0 + 0x430), "CW");
    } else if (dirA == 1) {
        SysSprintf((char*)(arg0 + 0x708), "Cycl-freq %.3f %s\n", *(f32*)(arg0 + 0x430), "CCW");
    } else {
        SysSprintf((char*)(arg0 + 0x708), "Cycl-freq %.3f %s\n", *(f32*)(arg0 + 0x430), "NaN");
    }
    s32 dirB = *(s32*)(arg0 + 0x5E0);
    *(f32*)(arg0 + 0x80C) = lbl_805FE164;
    if (dirB == 2) {
        SysSprintf((char*)(arg0 + 0x810), "Cycl-freq %.3f Dir: %s\n", *(f32*)(arg0 + 0x5E4), "CW");
    } else if (dirB == 1) {
        SysSprintf((char*)(arg0 + 0x810), "Cycl-freq %.3f Dir: %s\n", *(f32*)(arg0 + 0x5E4), "CCW");
    } else {
        SysSprintf((char*)(arg0 + 0x810), "Cycl-freq %.3f Dir: %s\n", *(f32*)(arg0 + 0x5E4), "NaN");
    }

    // state 3: the cyclic motion hits the object
    if (fn_800B6848(*(u8**)(arg0 + 0x94)) == 3) {
        s32 m = *(s32*)(arg0 + 0x42C);
        f32 f = *(f32*)(arg0 + 0x430);
        s32 hit = 0;
        if (m == 1 || m == 2) {
            f32 v = *(f32*)(arg0 + 0x430);
            hit = v > lbl_80346058[ClampIdx3(RaySceneTest(0))];
        }
        if (hit == 0) {
            hit = PadButtonHeld(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0x1039) != 0;
        }
        if (hit != 0) {
            fn_800B7400(*(u8**)(arg0 + 0x94), f - lbl_80346058[ClampIdx3(RaySceneTest(0))], dt);
        }
        m = *(s32*)(arg0 + 0x5E0);
        f = *(f32*)(arg0 + 0x5E4);
        hit = 0;
        if (m == 1 || m == 2) {
            f32 v = *(f32*)(arg0 + 0x5E4);
            hit = v > lbl_80346058[ClampIdx3(RaySceneTest(0))];
        }
        if (hit == 0) {
            hit = PadButtonHeld(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0x103B) != 0;
        }
        if (hit != 0) {
            fn_800B7414(*(u8**)(arg0 + 0x94), f - lbl_80346058[ClampIdx3(RaySceneTest(0))], dt);
        }
    }

    if (fn_800B66D0(*(u8**)(arg0 + 0x94)) == 0) {
        if (*(f32*)(arg0 + 0xA18) > lbl_805FE168) {
            s32 i = ClampIdx3(RaySceneTest(0));
            *(f32*)(arg0 + 0x67C) = *(f32*)(arg0 + 0x67C) + dt * lbl_80346298[i];
            fn_800712B8();
        }
    } else {
        *(f32*)(arg0 + 0xA18) = lbl_805FE108;
    }

    if (fn_800B66D0(*(u8**)(arg0 + 0x94)) == 1 || fn_800B6848(*(u8**)(arg0 + 0x94)) == 3) {
        if (*(f32*)(arg0 + 0xA1C) > lbl_805FE168) {
            s32 i = ClampIdx3(RaySceneTest(0));
            *(f32*)(arg0 + 0x67C) = *(f32*)(arg0 + 0x67C) - dt * lbl_803462A8[i];
            fn_800712B4();
            if (*(f32*)(arg0 + 0xA1C) > lbl_805FE11C) {
                fn_800B6598(*(u8**)(arg0 + 0x94), dt, lbl_805FE108);
                *(f32*)(arg0 + 0xA1C) = lbl_805FE108;
                EventSend(gEventMgr, *(u8**)(arg0 + 0x94), NULL, 0x0114FDB1u, 0, 0, 1, 0);
            }
        }
    } else {
        *(f32*)(arg0 + 0xA1C) = lbl_805FE108;
    }

    if (fn_800B66D0(*(u8**)(arg0 + 0x94)) == 2) {
        if (*(f32*)(arg0 + 0xA20) > lbl_805FE168 && fn_800B5B4C(arg0 + 0x620) == 0) {
            *(f32*)(arg0 + 0xA20) = lbl_805FE108;
            s32 i = ClampIdx3(RaySceneTest(0));
            fn_800B5B20(arg0 + 0x620, lbl_80346228[i]);
            EnsureVoiceHandle();
            PlaySound(0xFED75A95u, 0, 0, 0, 0, 0);
        }
    } else {
        *(f32*)(arg0 + 0xA20) = lbl_805FE108;
    }

    // power-up status line
    if (fn_800B5B44(arg0 + 0x620) > lbl_805FE108) {
        if (fn_800B5B4C(arg0 + 0x620) == 2) {
            f32 left = fn_800B5B44(arg0 + 0x620);
            SysSprintf((char*)(arg0 + 0x914), "POWERUP, Press %s! %.2f seconds left\n", fn_800B5B54(arg0 + 0x620), left);
        }
    } else if (*(f32*)(arg0 + 0x994) > lbl_805FE164) {
        SysSprintf((char*)(arg0 + 0x914), "");
    }

    if (fn_800B5B4C(arg0 + 0x620) == 3) {
        SysSprintf((char*)(arg0 + 0x914), "POWERUP SUCCEED!\n");
        *(f32*)(arg0 + 0x67C) = *(f32*)(arg0 + 0x67C) + lbl_805FE16C;
        s32 i = ClampIdx3(RaySceneTest(0));
        fn_800B6510(*(u8**)(arg0 + 0x94), lbl_805FE11C, lbl_80346238[i]);
        fn_800712B8();
        ClearPowerupSlots(arg0);
        fn_800B5B30(arg0 + 0x620);
        EnsureVoiceHandle();
        PlaySound(0x057B8DDCu, 0, 0, 0, 0, 0);
        *(f32*)(arg0 + 0x994) = lbl_805FE108;
    } else if (fn_800B5B4C(arg0 + 0x620) == 4) {
        SysSprintf((char*)(arg0 + 0x914), "POWERUP FAILED!\n");
        ClearPowerupSlots(arg0);
        fn_800B5B30(arg0 + 0x620);
        EnsureVoiceHandle();
        PlaySound(0x48EB5531u, 0, 0, 0, 0, 0);
        *(f32*)(arg0 + 0x994) = lbl_805FE108;
    }

    // slot +0x64 of the object's vtable (at +4) returns a value in percent
    f32 pct = (*(f32 (**)(void*))(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
    fn_80070FAC(arg0, pct / lbl_805FE118);
}

// 0x800C8F84
// Per-frame update of the object list: for each entry (count at +0x394) finds its resource, builds
// the entry's orientation from the matrix in the table at +0x98 (stride 0xC8), writes position/quat
// and flags into the resource's transform, runs fn_800C98F8, copies the position to +0x2E0 and
// registers the resource in the first free slot of the 0x32-entry table at +0x39C. Then fn_800CA290 / fn_800CB2D8.
void fn_800C8F84(u8* arg0, u8* arg1) {
    alignas(16) u8 ctx[0xB10];
    u8 q30[0x30];
    f32 quat[4];
    f32 euler[3];
    f32 pos[3];
    u8* p28 = arg0;
    u8* p27 = arg0;

    fn_800C1B78(arg0);
    if (fn_800CD4E8(arg0) != 0) return;
    if (fn_800CD744(arg0) != 0) return;

    fn_8010B4AC(ctx);
    u8* r25 = *(u8**)(arg0 + 0x2A0);
    fn_800C1798(q30, arg1);
    fn_8010B7C8(ctx, r25, q30);

    for (s32 i = 0; i < *(s32*)(arg0 + 0x394); i++) {
        u8* res = fn_800C7974(*(u8**)(arg0 + 0x5AC));
        if (res != NULL) {
            u8* tr = (u8*)fn_800C73D0(res);
            if (tr != NULL) {
                s32 off = *(s32*)(p28 + 0x2A4) * 0xC8;
                u8* rec = *(u8**)(arg0 + 0x98) + off;
                f32 px = *(f32*)(rec + 0x5C);
                f32 pz = *(f32*)(rec + 0x64);
                euler[2] = lbl_805FE4B8;
                euler[1] = lbl_805FE4B8;
                euler[0] = lbl_805FE4B8;
                quat[2] = lbl_805FE4B8;
                quat[1] = lbl_805FE4B8;
                quat[0] = lbl_805FE4B8;
                quat[3] = lbl_805FE4BC;
                pos[0] = px;
                pos[2] = pz;
                pos[1] = *(f32*)(tr + 0xCC);
                fn_801449A4((f32*)(*(u8**)(arg0 + 0x98) + off + 0x38), euler);
                EulerToQuat((Quat*)quat, (const Vec3*)euler);
                u8* t = *(u8**)(tr + 0x4);
                *(f32*)(t + 0x0) = pos[0];
                *(f32*)(t + 0x4) = pos[1];
                *(f32*)(t + 0x8) = pos[2];
                // the asm reloads *(tr + 4) before every flag update; the flags are or-ed in this order
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x8000u;
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x10000000u;
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x4000u;
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x10000000u;
                t = *(u8**)(tr + 0x4);
                *(f32*)(t + 0xC) = quat[0];
                *(f32*)(t + 0x10) = quat[1];
                *(f32*)(t + 0x14) = quat[2];
                *(f32*)(t + 0x18) = quat[3];
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x8000u;
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x10000000u;
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x4000u;
                t = *(u8**)(tr + 0x4);
                *(u32*)(t + 0x20) |= 0x10000000u;

                fn_800C98F8(arg0, tr, ctx);
                t = *(u8**)(tr + 0x4);
                *(f32*)(p27 + 0x2E0) = *(f32*)(t + 0x0);
                *(f32*)(p27 + 0x2E4) = *(f32*)(t + 0x4);
                *(f32*)(p27 + 0x2E8) = *(f32*)(t + 0x8);
                // (the asm re-tests res != NULL here; always true on this path)
                for (s32 j = 0; j < 0x32; j++) {
                    if (*(s32*)(arg0 + 0x39C + j * 4) == 0) {
                        *(u8**)(arg0 + 0x39C + j * 4) = res;
                        fn_800C72B4(res, arg0);
                        break;
                    }
                }
            }
        }
        p28 += 4;
        p27 += 0xC;
    }
    fn_800CA290(arg0, ctx);
    fn_800CB2D8(arg0);
    fn_8010B4C0(ctx, -1);
}

// 0x800CB2D8
// Scatters a random number of effect objects around the object at +4 (e.g. a spray of pieces):
// builds a path of control points from the positions at +0x2E0 (fn_800CABD0 smooths it), samples
// it into up to 50 points (stack array), then offsets each point sideways (axis chosen by the
// rounded Y euler angle, 0 or 1) by a random amount in the radius band, spawns/positions the
// resource at each one (fn_800C694C / fn_800CBD4C / fn_800C6A0C) and registers it in the 0x32-slot
// table at +0x39C.
void fn_800CB2D8(u8* arg0) {
    u8 q[0x30];
    f32 euler[3];
    f32 endv[3];       // fn_800C176C output (the path's last point)
    f32 start[3];      // position of the object at +4
    f32 pts[0xB8 / 4];     // path control points, 3 floats each
    f32 outp[3 * 50];      // sampled points
    f32 lim[3];            // clamped reference point (sp98)
    f32 cand[3];           // candidate position (sp8c)
    f32 zero3[3];
    f32 tmp[3];
    f32 cx[3];

    fn_800C1798(q, arg0);
    fn_801449A4((f32*)q, euler);
    s32 axis = (s32)euler[1];
    s32 n = 0;

    f32 hi = (f32)*(s32*)(arg0 + 0x394) * lbl_80347320[RaySceneTest(0)];
    f32 lo = (f32)*(s32*)(arg0 + 0x394) * lbl_80347330[RaySceneTest(0)];
    if (hi < lo) {
        f32 t = lo;
        lo = hi;
        hi = t;
    }
    if (lo > lbl_805FE4B8) {
        n = (s32)(hi + (lo - hi) * (lbl_805FE4E4 * (f32)RandomU32()));
    }
    if (n == 0) return;

    u8* tgt = *(u8**)(arg0 + 0x4);
    start[0] = *(f32*)(tgt + 0x0);
    start[1] = *(f32*)(tgt + 0x4);
    start[2] = *(f32*)(tgt + 0x8);
    fn_800C176C(endv, arg0);
    start[1] = lbl_805FE4B8;
    endv[1] = lbl_805FE4B8;
    pts[0] = start[0];
    pts[1] = lbl_805FE4B8;
    pts[2] = start[2];
    for (s32 i = 0; i < *(s32*)(arg0 + 0x394); i++) {
        pts[3 + i * 3 + 0] = *(f32*)(arg0 + 0x2E0 + i * 0xC);
        pts[3 + i * 3 + 1] = *(f32*)(arg0 + 0x2E4 + i * 0xC);
        pts[3 + i * 3 + 2] = *(f32*)(arg0 + 0x2E8 + i * 0xC);
    }
    {
        s32 last = (*(s32*)(arg0 + 0x394) + 1) * 3;
        pts[last + 0] = endv[0];
        pts[last + 1] = endv[1];
        pts[last + 2] = endv[2];
    }
    cx[0] = start[0];
    cx[1] = start[1];
    cx[2] = start[2];
    fn_800CABD0((u8*)cx, (u8*)pts, (u32)(*(s32*)(arg0 + 0x394) + 2));

    // sample the path at n evenly spaced parameters
    for (s32 i = 0; i < n; i++) {
        f32 t = lbl_805FE500 + (f32)i * ((f32)(*(s32*)(arg0 + 0x394) + 1) / (f32)n);
        s32 j = (s32)t;
        f32* a = pts + j * 3;
        f32* b = pts + (j + 1) * 3;
        s32 fl = (s32)(f32)fn_8033ED6C((f64)t);
        f32 frac = t - (f32)fl;
        outp[i * 3 + 0] = a[0] + (b[0] - a[0]) * frac;
        outp[i * 3 + 1] = a[1] + (b[1] - a[1]) * frac;
        outp[i * 3 + 2] = a[2] + (b[2] - a[2]) * frac;
    }

    lim[0] = start[0];
    lim[1] = start[1];
    lim[2] = start[2];
    if (fn_800CD59C(arg0) != 0) {
        s32 ab = fn_80332EE0(axis);
        if (ab == 0) {
            f32 x = start[0] > endv[0] ? start[0] : endv[0];
            f32 y = start[1] > endv[1] ? start[1] : endv[1];
            f32 z = start[2] > endv[2] ? start[2] : endv[2];
            lim[0] = x;
            lim[1] = y;
            lim[2] = z;
            // (the asm passes stale f1/f2 here; the callee ignores them)
            if (fn_800CD6E0(arg0) != 0) {
                lim[2] = start[2] < endv[2] ? start[2] : endv[2];
            }
        } else if (ab == 1) {
            f32 x = start[0] < endv[0] ? start[0] : endv[0];
            f32 y = start[1] < endv[1] ? start[1] : endv[1];
            f32 z = start[2] < endv[2] ? start[2] : endv[2];
            lim[0] = x;
            lim[1] = y;
            lim[2] = z;
            if (fn_800CD67C(arg0) != 0) {
                lim[2] = start[2] > endv[2] ? start[2] : endv[2];
            }
        }
    }

    f32 r0 = lbl_80347340[RaySceneTest(0)];
    f32 r1 = lbl_80347350[RaySceneTest(0)];
    f32 zero = lbl_805FE4B8;
    for (s32 k = 0; k < n; k++) {
        f32* p = outp + k * 3;
        cand[2] = zero;
        cand[1] = zero;
        cand[0] = zero;
        f32 sign = (RandomU32() & 1) ? lbl_805FE4BC : lbl_805FE4D8;
        s32 ab = fn_80332EE0(axis);
        if (ab == 0) {
            if (fn_800CD51C(arg0) != 0) {
                r0 = sign * r0 + p[2];
                cand[0] = p[0];
                r1 = sign * r1 + p[2];
                cand[2] = r0 + (r1 - r0) * (lbl_805FE4E4 * (f32)RandomU32());
            } else {
                f32 dx = p[0] - lim[0];
                f32 dy = p[1] - lim[1];
                f32 dz = p[2] - lim[2];
                f32 len2 = dz * dz + (dx * dx + dy * dy);
                f32 len = zero;
                if (len2 > zero) len = sqrtf(len2);
                r0 = r0 + len;
                r1 = r1 + len;
                if (len == zero) len = lbl_805FE504;
                f32 nx = fabsf(dx / len);
                f32 nz = fabsf(dz / len);
                f32 u = r0 * nx;
                f32 w = u + (r1 * nx - u) * (lbl_805FE4E4 * (f32)RandomU32());
                f32 u2 = r0 * nz;
                f32 w2 = u2 + (r1 * nz - u2) * (lbl_805FE4E4 * (f32)RandomU32());
                if (fn_800CD6E0(arg0) != 0) {
                    cand[0] = lim[0] - w;
                    cand[2] = w2 + lim[2];
                } else if (fn_800CD67C(arg0) != 0) {
                    cand[0] = lim[0] - w;
                    cand[2] = lim[2] - w2;
                }
            }
        } else if (ab == 1) {
            if (fn_800CD51C(arg0) != 0) {
                r0 = sign * r0 + p[0];
                r1 = sign * r1 + p[0];
                cand[2] = p[2];
                cand[0] = r0 + (r1 - r0) * (lbl_805FE4E4 * (f32)RandomU32());
            } else {
                f32 dx = p[0] - lim[0];
                f32 dy = p[1] - lim[1];
                f32 dz = p[2] - lim[2];
                f32 len2 = dz * dz + (dx * dx + dy * dy);
                f32 len = zero;
                if (len2 > zero) len = sqrtf(len2);
                r0 = r0 + len;
                r1 = r1 + len;
                if (len == zero) len = lbl_805FE504;
                f32 nx = fabsf(dx / len);
                f32 nz = fabsf(dz / len);
                f32 u = r0 * nx;
                f32 w = u + (r1 * nx - u) * (lbl_805FE4E4 * (f32)RandomU32());
                f32 u2 = r0 * nz;
                f32 w2 = u2 + (r1 * nz - u2) * (lbl_805FE4E4 * (f32)RandomU32());
                if (fn_800CD6E0(arg0) != 0) {
                    cand[0] = w + lim[0];
                    cand[2] = w2 + lim[2];
                } else if (fn_800CD67C(arg0) != 0) {
                    cand[0] = w + lim[0];
                    cand[2] = lim[2] - w2;
                }
            }
        }

        u8* res = fn_800C7928(*(u8**)(arg0 + 0x5AC));
        if (res != NULL) {
            u8* obj = (u8*)(uintptr_t)fn_800C73B4(res);
            if (obj != NULL) {
                tmp[0] = cand[0];
                tmp[1] = cand[1];
                tmp[2] = cand[2];
                zero3[2] = zero;
                zero3[1] = zero;
                zero3[0] = zero;
                fn_800C694C(obj, (u8*)tmp);
                if (fn_800CBD4C(arg0, cand, zero3) != 0) {
                    f32 v[3];
                    v[0] = zero3[0];
                    v[1] = zero3[1];
                    v[2] = zero3[2];
                    fn_800C6A0C(obj, (u8*)v);
                }
                for (s32 j = 0; j < 0x32; j++) {
                    if (*(s32*)(arg0 + 0x39C + j * 4) == 0) {
                        *(u8**)(arg0 + 0x39C + j * 4) = res;
                        fn_800C72B4(res, arg0);
                        break;
                    }
                }
            }
        }
    }
}

// Debug overlay helpers for fn_800D1D3C.
static void DbgText(const char* text, s32 x, s32 y, f32 size) {
    fn_80121508((char*)text, x, y, -1, 1, 0, size, 0.0f);
}

// One "recognized / none" line: when the flag byte at flagOff is set and the float at valOff is positive
// it is copied to dstOff; the line shows the yes-text or the no-text.
static void DbgGestureLine(u8* o, s32 flagOff, s32 valOff, s32 dstOff, const char* yes, const char* no,
                           char* buf, s32 x, s32 y) {
    if (*(u8*)(o + flagOff) != 0) {
        f32 v = *(f32*)(o + valOff);
        if (v > 0.0f) *(f32*)(o + dstOff) = v;
    }
    if (*(u8*)(o + flagOff) != 0) {
        SysSprintf(buf, yes);
    } else {
        SysSprintf(buf, no);
    }
    DbgText(buf, x, y, 40.0f);
}

// 0x800D1D3C
// Debug overlay for the gesture-input test: draws two columns (Wii remote / Nunchuk) of recognizer
// state (cyclic direction, directional motion, drum, throw, spike, shake, tilt), the result of
// fn_800D2B1C for each recognizer, the key help, the screen position of the plane normal and the
// two gravity/normal vectors. The flag byte at lbl_803C2A20+0x820 is set while it draws.
void fn_800D1D3C(u8* arg0, f32 dt) {
    char buf[0x100];
    const char* str = (const char*)lbl_8035F898;
    s32 wasClear = 0;

    if (*(u8*)(lbl_803C2A20 + 0x820) == 0) {
        wasClear = 1;
        *(u8*)(lbl_803C2A20 + 0x820) = 1;
    }
    s32 xr = fn_80156A80() / 4;
    s32 w = fn_80156A80();
    s32 xl = (w * 4 - w) / 4;
    DbgText(str + 0x2C, xl, 0x28, 50.0f);
    DbgText(str + 0x38, xr, 0x28, 50.0f);

    s32 cyc = *(s32*)(arg0 + 0x1698);
    if (cyc == 1) {
        SysSprintf(buf, str + 0x48, *(f32*)(arg0 + 0x169C));
    } else if (cyc == 2) {
        SysSprintf(buf, str + 0x64, *(f32*)(arg0 + 0x169C));
    } else {
        SysSprintf(buf, str + 0x80, *(f32*)(arg0 + 0x169C));
    }
    DbgText(buf, xl, 0x41, 40.0f);
    cyc = *(s32*)(arg0 + 0x29C);
    if (cyc == 1) {
        SysSprintf(buf, str + 0x48, *(f32*)(arg0 + 0x2A0));
    } else if (cyc == 2) {
        SysSprintf(buf, str + 0x64, *(f32*)(arg0 + 0x2A0));
    } else {
        SysSprintf(buf, str + 0x80, *(f32*)(arg0 + 0x2A0));
    }
    DbgText(buf, xr, 0x41, 40.0f);

    // directional motion (+0x175C / +0x360)
    if (*(u8*)(arg0 + 0x175C) != 0) {
        f32 v = *(f32*)(arg0 + 0x1760);
        if (v > 0.0f) *(f32*)(arg0 + 0x16D4) = v;
    }
    if (*(u8*)(arg0 + 0x175C) != 0) {
        SysSprintf(buf, str + 0x9C, *(f32*)(arg0 + 0x1778), *(f32*)(arg0 + 0x177C));
    } else {
        SysSprintf(buf, str + 0xC4);
    }
    DbgText(buf, xl, 0x55, 40.0f);
    if (*(u8*)(arg0 + 0x360) != 0) {
        f32 v = *(f32*)(arg0 + 0x364);
        if (v > 0.0f) *(f32*)(arg0 + 0x2D8) = v;
    }
    if (*(u8*)(arg0 + 0x360) != 0) {
        SysSprintf(buf, str + 0x9C, *(f32*)(arg0 + 0x37C), *(f32*)(arg0 + 0x380));
    } else {
        SysSprintf(buf, str + 0xC4);
    }
    DbgText(buf, xr, 0x55, 40.0f);

    // drum motion (+0x1810 / +0x414)
    DbgGestureLine(arg0, 0x1810, 0x1814, 0x1788, str + 0xD8, str + 0xF0, buf, xl, 0x69);
    DbgGestureLine(arg0, 0x414, 0x418, 0x38C, str + 0xD8, str + 0xF0, buf, xr, 0x69);

    // fn_800D2B1C text for the six recognizers
    fn_800D2B1C(arg0 + 0x33CC, arg0 + 0x1840, dt);
    SysSprintf(buf, lbl_805F8CB0, arg0 + 0x33CC);
    DbgText(buf, xl, 0x7D, 40.0f);
    fn_800D2B1C(arg0 + 0x2A84, arg0 + 0x444, dt);
    SysSprintf(buf, lbl_805F8CB0, arg0 + 0x2A84);
    DbgText(buf, xr, 0x7D, 40.0f);
    fn_800D2B1C(arg0 + 0x36E4, arg0 + 0x1CC4, dt);
    SysSprintf(buf, lbl_805F8CB0, arg0 + 0x36E4);
    DbgText(buf, xl, 0x91, 40.0f);
    fn_800D2B1C(arg0 + 0x2D9C, arg0 + 0x8C8, dt);
    SysSprintf(buf, lbl_805F8CB0, arg0 + 0x2D9C);
    DbgText(buf, xr, 0x91, 40.0f);
    fn_800D2B1C(arg0 + 0x39FC, arg0 + 0x2148, dt);
    SysSprintf(buf, lbl_805F8CB0, arg0 + 0x39FC);
    DbgText(buf, xl, 0xA5, 40.0f);
    fn_800D2B1C(arg0 + 0x30B4, arg0 + 0xD4C, dt);
    SysSprintf(buf, lbl_805F8CB0, arg0 + 0x30B4);
    DbgText(buf, xr, 0xA5, 40.0f);

    // throw, spike, shake
    DbgGestureLine(arg0, 0x265C, 0x2660, 0x25D4, str + 0x104, str + 0x118, buf, xl, 0xB9);
    DbgGestureLine(arg0, 0x1260, 0x1264, 0x11D8, str + 0x104, str + 0x118, buf, xr, 0xB9);
    DbgGestureLine(arg0, 0x2780, 0x2784, 0x26F8, str + 0x124, str + 0x138, buf, xl, 0xCD);
    DbgGestureLine(arg0, 0x1384, 0x1388, 0x12FC, str + 0x124, str + 0x138, buf, xr, 0xCD);
    DbgGestureLine(arg0, 0x2834, 0x2838, 0x27AC, str + 0x144, str + 0x158, buf, xl, 0xE1);
    DbgGestureLine(arg0, 0x1438, 0x143C, 0x13B0, str + 0x144, str + 0x158, buf, xr, 0xE1);

    // tilt and planar tilt
    SysSprintf(buf, str + 0x164, *(f32*)(arg0 + 0x2900));
    DbgText(buf, xl, 0xF5, 40.0f);
    SysSprintf(buf, str + 0x164, *(f32*)(arg0 + 0x1504));
    DbgText(buf, xr, 0xF5, 40.0f);
    SysSprintf(buf, str + 0x174, *(f32*)(arg0 + 0x29A8));
    DbgText(buf, xl, 0x109, 40.0f);
    SysSprintf(buf, str + 0x174, *(f32*)(arg0 + 0x15AC));
    DbgText(buf, xr, 0x109, 40.0f);

    // key help
    DbgText(str + 0x188, xr, 0x145, 50.0f);
    DbgText(str + 0x1A0, xl, 0x145, 50.0f);
    DbgText(str + 0x1B4, xr, 0x15E, 50.0f);
    DbgText(str + 0x1D0, xl, 0x15E, 50.0f);

    // screen position of the plane-normal pointer
    s32 xc = fn_80156A80();
    xc = (xc + (s32)((u32)xc >> 31)) >> 1;
    f32 px = *(f32*)(arg0 + 0x2A58);
    f32 py = *(f32*)(arg0 + 0x2A5C);
    f32 hw = 0.5f * (f32)gScreenWidth;
    f32 hh = 0.5f * (f32)gScreenHeight;
    f32 sx = hw + hw * (*(f32*)(arg0 + 0x2A78) * px);
    f32 sy = hh + hh * (*(f32*)(arg0 + 0x2A7C) * py);
    SysSprintf(buf, str + 0x1F0, sx, sy);
    DbgText(buf, xc, 0x190, 40.0f);

    u8* g = arg0 + *(s32*)(arg0 + 0x184) * 0xC;
    SysSprintf(buf, str + 0x210, *(f32*)(g + 0x188), *(f32*)(g + 0x18C), *(f32*)(g + 0x190));
    DbgText(buf, xc, 0x1A9, 40.0f);
    g = arg0 + *(s32*)(arg0 + 0x180) * 0xC;
    SysSprintf(buf, str + 0x230, *(f32*)(g + 0x188), *(f32*)(g + 0x18C), *(f32*)(g + 0x190));
    DbgText(buf, xc, 0x1C2, 40.0f);

    if (wasClear != 0) {
        *(u8*)(lbl_803C2A20 + 0x820) = 0;
    }
}

// 0x800DD58C
// Refreshes the cached follow data of the state in arg0 for modes 3 and 4 (mode at +0x8):
// mode 3 copies position/orientation of the object at +0x4, queries the sub-object at +0x68
// (slot +0xC) for a point and a direction, builds the facing quaternion (fn_801458B0), stores the
// offset to the target (+0x188..) and derives a ramp value at +0x1D8/+0x1DC from its length
// (< 3.0 -> 0.5 * len / 3.0); mode 4 just stores the queried point at +0x1EC.
void fn_800DD58C(u8* arg0) {
    typedef void (*VFnQuery)(void* self, f32* a, f32* b, f32 v);
    f32 a[3];
    f32 b[3];
    f32 q[4];
    f32 up[3];
    f32 cc[3];

    s32 mode = *(s32*)(arg0 + 0x8);
    if (mode == 3) {
        u8* r5 = *(u8**)(arg0 + 0x4);
        *(f32*)(arg0 + 0x1B0) = *(f32*)(r5 + 0x0);
        *(f32*)(arg0 + 0x1B4) = *(f32*)(r5 + 0x4);
        *(f32*)(arg0 + 0x1B8) = *(f32*)(r5 + 0x8);
        Vec3 ax = QuatGetAxisY((const Quat*)(r5 + 0xC));
        *(f32*)(arg0 + 0x1BC) = ax.x;
        u8* r6 = *(u8**)(arg0 + 0x4);
        *(f32*)(arg0 + 0x1C0) = ax.y;
        *(f32*)(arg0 + 0x1C4) = ax.z;
        *(f32*)(arg0 + 0x1C8) = *(f32*)(r6 + 0xC);
        *(f32*)(arg0 + 0x1CC) = *(f32*)(r6 + 0x10);
        *(f32*)(arg0 + 0x1D0) = *(f32*)(r6 + 0x14);
        *(f32*)(arg0 + 0x1D4) = *(f32*)(r6 + 0x18);
        *(u8*)(arg0 + 0x18) = 1;
        (*(VFnQuery*)(*(u8**)(arg0 + 0x68) + 0xC))(arg0 + 0x68, a, b, lbl_805FE76C);

        f32 x = b[0], y = b[1], z = b[2];
        up[0] = lbl_805FE708;
        up[1] = lbl_805FE708;
        up[2] = lbl_805FE70C;
        f32 inv = 1.0f / sqrtf(z * z + (x * x + y * y));
        b[0] = x * inv;
        b[1] = y * inv;
        b[2] = z * inv;
        fn_801458B0(q, b, up);

        *(f32*)(arg0 + 0x188) = a[0];
        *(f32*)(arg0 + 0x18C) = a[1];
        *(f32*)(arg0 + 0x190) = a[2];
        Vec3 ay = QuatGetAxisY((const Quat*)q);
        *(f32*)(arg0 + 0x194) = ay.x;
        f32 dx = *(f32*)(arg0 + 0x188) - *(f32*)(arg0 + 0x1B0);
        *(f32*)(arg0 + 0x198) = ay.y;
        f32 dy = *(f32*)(arg0 + 0x18C) - *(f32*)(arg0 + 0x1B4);
        *(f32*)(arg0 + 0x19C) = ay.z;
        f32 dz = *(f32*)(arg0 + 0x190) - *(f32*)(arg0 + 0x1B8);
        f32 len = lbl_805FE708;
        *(f32*)(arg0 + 0x1A0) = q[0];
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        *(f32*)(arg0 + 0x1A4) = q[1];
        *(f32*)(arg0 + 0x1A8) = q[2];
        *(f32*)(arg0 + 0x1AC) = q[3];
        if (len2 > len) {
            len = sqrtf(len2);
        }
        if (len < lbl_805FE74C) {
            *(f32*)(arg0 + 0x1DC) = lbl_805FE758 * (len / lbl_805FE74C);
        }
        *(f32*)(arg0 + 0x1D8) = *(f32*)(arg0 + 0x1DC);
        lbl_805FB1A1 = 1;
    } else if (mode == 4) {
        *(u8*)(arg0 + 0x18) = 1;
        (*(VFnQuery*)(*(u8**)(arg0 + 0x68) + 0xC))(arg0 + 0x68, cc, b, lbl_805FE76C);
        *(f32*)(arg0 + 0x1EC) = cc[0];
        *(f32*)(arg0 + 0x1F0) = cc[1];
        *(f32*)(arg0 + 0x1F4) = cc[2];
        lbl_805FB1A1 = 1;
    }
}

// 0x80176458
// Reads a 0x68-byte record from the byte stream *arg1 into a new heap record owned by arg0:
// flag bytes, small ints, then four float triples (each followed by 1.0) and three more floats.
void fn_80176458(u8* arg0, u8** arg1) {
    u8* r = (*(u8* (**)(s32, s32, s32, u8*, s32))lbl_805FB608)(0x68, 0x10, 0x12, arg0 + 0x80, 0);
    *(u8**)(r + 0x0) = arg0;
    u8* s;
#define RD_FLAG(off) s = *arg1; *arg1 = s + 1; *(u8*)(r + (off)) = (*s != 0)
#define RD_BYTE(off) s = *arg1; *arg1 = s + 1; *(s32*)(r + (off)) = *s
    RD_FLAG(0x4);
    RD_FLAG(0x5);
    RD_BYTE(0x54);
    RD_FLAG(0x58);
    RD_FLAG(0x64);
    RD_FLAG(0x65);
    RD_FLAG(0x59);
    RD_FLAG(0x5A);
    RD_BYTE(0x5C);
    RD_BYTE(0x60);
    RD_FLAG(0x66);
    RD_FLAG(0x67);
#undef RD_FLAG
#undef RD_BYTE
    // the first float triple starts at the next 4-byte boundary
    s = (u8*)(((uintptr_t)*arg1 + 3) & ~(uintptr_t)3);
    for (s32 g = 0; g < 4; g++) {
        u8* d = r + 0x8 + g * 0x10;
        if (g != 0) s = *arg1;
        *(f32*)(d + 0x0) = *(f32*)(s + 0x0);
        *(f32*)(d + 0x4) = *(f32*)(s + 0x4);
        *(f32*)(d + 0x8) = *(f32*)(s + 0x8);
        *(f32*)(d + 0xC) = lbl_805FF6F8;
        *arg1 = s + 0xC;
    }
    s = *arg1; *arg1 = s + 4; *(f32*)(r + 0x48) = *(f32*)s;
    s = *arg1; *arg1 = s + 4; *(f32*)(r + 0x4C) = *(f32*)s;
    s = *arg1; *arg1 = s + 4; *(f32*)(r + 0x50) = *(f32*)s;
}

}  // extern "C"
