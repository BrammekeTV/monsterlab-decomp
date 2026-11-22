// Brain toss minigame (0x8007E57C..0x800857A8): per-frame update, stick handling, prop setup,
// aim target roll and prop hand-off.
#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
void  AimMinigame_Update(u8* g);                                 // 0x8007FE30
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void  MtxFromLocatorRotation(f32 out[12], const void* rot);     // 0x80143CA8
u32   RandomU32(void);                                           // 0x80143460
s32   RaySceneTest(void* ray);                                   // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                             // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                  // 0x801F53B0
void  StickRead(s32 channel, f32* x, f32* y);                    // 0x80147704
void  Vec3Copy(Vec3* out, const Vec3* v);                        // 0x80052640
f32   Vec3GetX(const Vec3* v);
f32   Vec3GetY(const Vec3* v);
void  Vec3Negate(Vec3* out, const Vec3* v);
void  Vec3NormalizeTo(Vec3* out, const Vec3* v);                 // 0x80055D9C
void  Vec3SetZ(Vec3* v, f32 z);
void  Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);          // 0x80053370
BOOL  ZoneIsLoaded(void* world, s32 zone);                       // 0x8012F408
void  fn_800359F4(u8* a3, f32 f1, f32 f2, f32 f3);               // 0x800359F4
void  fn_80052638(void);                                      // 0x80052638 (empty leaf)
void  fn_800526D0(u8* out);                                      // 0x800526D0 (matrix identity)
u8*   fn_800526DC(u8* c);                                        // 0x800526DC
void  fn_8005535C(u8* dst, const u8* src);                       // 0x8005535C
void  fn_800554DC(u8* out, const u8* a, const u8* b);            // 0x800554DC
f32   fn_8005B4A4(const Vec3* a, const Vec3* b);                 // 0x8005B4A4 (dot)
void  fn_8005C204(u8* out, u8* in);                              // 0x8005C204 (matrix inverse)
u32   fn_80070D88(void* owner, u32 id);                          // 0x80070D88
void  fn_80070FAC(u8* p, f32 v);                                 // 0x80070FAC
void  fn_800712B4(void);                                         // 0x800712B4
void  fn_800712BC(u8* p, s32 a, s32 b);                          // 0x800712BC
f32   fn_8007DD7C(Vec3* v);                                      // 0x8007DD7C
u8*   fn_8007DD0C(u8* table, s32 idx);                           // 0x8007DD0C
s32   fn_8007DD38(u8* table, u32 key);                           // 0x8007DD38
u8*   fn_8007E0B8(u8* base, s32 idx);                            // 0x8007E0B8
void  fn_8007EF04(u8* mgr, u8* chunk, s32 idx, f32 t);           // 0x8007EF04
u8*   fn_8007F900(u8* mgr, s32 type);                            // 0x8007F900
void  fn_8007FD98(u8* mtx, u8* node);                            // 0x8007FD98
void  fn_8007FDCC(u8* out, u8* mtx);                             // 0x8007FDCC
void  fn_80081004(u8* out, u8* obj, s32 idx, f32 t);             // 0x80081004
f32   fn_80081210(u8* obj, u8* p);                               // 0x80081210
void  fn_80081404(u8* obj, f32 dt);                              // 0x80081404
u8*   fn_80081A34(void);                                         // 0x80081A34
void  fn_80082120(u8* obj, f32* v);                              // 0x80082120
void  fn_8008236C(u8* p, f32 dt);                                // 0x8008236C
s32   fn_80083930(u8* self, f32* pos);                           // 0x80083930
void  fn_80084D3C(f32* out, u8* ctx);                            // 0x80084D3C
void* fn_801B50FC(u8* p);                                        // 0x801B50FC (object transform matrix)
void  fn_801B6F34(u8* p);                                        // 0x801B6F34
f64   fn_8033F1F4(f64 x);                                        // 0x8033F1F4 (MSL acos)

void  fn_80081674(u8* arg0, f32 dt);                             // 0x80081674
void  fn_8007EB58(u8* arg0, f32 dt);                             // 0x8007EB58

extern "C++" void* gEventMgr;     // 0x805FB6C8
extern u8   gWorldState[];        // 0x803E6D00 (first word: world block pointer)
extern u8   lbl_803C4298[];       // 0x803C4298 (+0x4: object, +0x38: pad channel)
extern f32  lbl_803464E4[];       // 0x803464E4
extern f32  lbl_803464F0[];       // 0x803464F0
extern f32  lbl_80346500[];       // 0x80346500
extern f32  lbl_80346510[];       // 0x80346510
extern f32  lbl_80346580[];       // 0x80346580
extern f32  lbl_80346590[];       // 0x80346590
extern f32  lbl_803465A0[];       // 0x803465A0
extern f32  lbl_803465B0[];       // 0x803465B0
extern s32  lbl_803465C0[];       // 0x803465C0
extern s32  lbl_803465E0[];       // 0x803465E0
extern f32  lbl_803465F0[];       // 0x803465F0
extern f32  lbl_80346600[];       // 0x80346600
extern f32  lbl_80346610[];       // 0x80346610
}

typedef u8*  (*GetSubFn)(u8*);
typedef void (*SetFlagFn)(u8*, s32);
typedef void (*TickFn)(u8*, f32);
typedef void (*DeleteFn)(u8*, s32);
typedef void (*VoidFn)(u8*);
typedef f32  (*ScoreFn)(u8*);

static const f32 kHalfPi = 1.5707964f;
static const f32 kTwoOverPi = 0.63661975f;
static const f32 kU32ToUnit = 2.3283064e-10f;  // 1 / 2^32

extern "C" {

// Marks a prop active: state byte +0x268 = 1 (plus the inlined "wake" flags the first time), then
// sets bits 0x8000/0x10000000/0x4000/0x10000000 in the flags word at +0x20 of its sub-object (+4).
static void ActivateProp(u8* o) {
    u8* p;
    if (*(u8*)(o + 0x268) != 1) {
        *(u8*)(o + 0x268) = 1;
        if ((*(GetSubFn*)(*(u8**)o + 0xC))(o) != NULL) {
            u8* q = (*(GetSubFn*)(*(u8**)o + 0xC))(o);
            *(u32*)(q + 0x8) |= 2;
        }
        p = *(u8**)(o + 0x4);
        *(u32*)(p + 0x20) |= 2;
        *(s32*)(o + 0x2B4) = 0;
    }
}

// Sets the position (x, y, z) of the prop's sub-object (+4) and the four flag bits in +0x20.
static void PlaceProp(u8* o, f32 x, f32 y, f32 z) {
    u8* p = *(u8**)(o + 0x4);
    *(f32*)(p + 0x0) = x;
    *(f32*)(p + 0x4) = y;
    *(f32*)(p + 0x8) = z;
    p = *(u8**)(o + 0x4);
    *(u32*)(p + 0x20) |= 0x8000;
    p = *(u8**)(o + 0x4);
    *(u32*)(p + 0x20) |= 0x10000000;
    p = *(u8**)(o + 0x4);
    *(u32*)(p + 0x20) |= 0x4000;
    p = *(u8**)(o + 0x4);
    *(u32*)(p + 0x20) |= 0x10000000;
}

// 0x8007E57C
// Per-frame update: counts the timer down, advances the meter, signals win/lose, ticks the
// sub-objects and the slot spawner, then the aim minigame.
void fn_8007E57C(u8* arg0, f32 dt) {
    *(f32*)(arg0 + 0x324) = *(f32*)(arg0 + 0x324) - dt;
    f32 rate = dt * lbl_80346590[RaySceneTest(arg0)];
    f32 meter = *(f32*)(arg0 + 0x32C) + rate;
    *(f32*)(arg0 + 0x32C) = meter;
    if (meter > 5.0f) {
        fn_800712B4();
        *(f32*)(arg0 + 0x32C) = 0.0f;
    }
    f32 v = *(f32*)(arg0 + 0x328) - rate;
    f32 clamped;
    if (v < 0.0f) clamped = 0.0f;
    else if (v > 100.0f) clamped = 100.0f;
    else clamped = v;
    *(f32*)(arg0 + 0x328) = clamped;
    f32 score = (*(ScoreFn*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
    fn_80070FAC(arg0, score / 100.0f);
    if (score <= lbl_803465A0[RaySceneTest(NULL)]) {
        GameFlowSignal(0x117741C3, 0, 0);
        return;
    }
    if (*(f32*)(arg0 + 0x324) <= 0.0f) {
        if (score >= lbl_80346580[RaySceneTest(NULL)]) {
            GameFlowSignal((s32)0xC608C842u, 0, 0);
            return;
        }
        GameFlowSignal(0x117741C3, 0, 0);
        return;
    }
    u8* o = *(u8**)(arg0 + 0x2A8);
    if (o != NULL) {
        (*(TickFn*)(*(u8**)o + 0xC))(o, dt);
        o = *(u8**)(arg0 + 0x2A8);
        if (*(f32*)(o + 0xC) < 0.0f) {
            if (o != NULL) (*(DeleteFn*)(*(u8**)o + 0x8))(o, 1);
            *(void**)(arg0 + 0x2A8) = NULL;
        }
    }
    fn_80081404(arg0, dt);
    fn_80081674(arg0, dt);
    f32 t = *(f32*)(arg0 + 0x324);
    fn_800712BC(arg0, (s32)t, t <= 10.0f);
    fn_8007EB58(arg0, dt);
    fn_801B6F34(*(u8**)(arg0 + 0x29C));
    u8* slot = arg0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x298); i++) {
        fn_8008236C(*(u8**)(slot + 0x1F8), dt);
        slot += 4;
    }
    s32 limit = lbl_803465E0[RaySceneTest(NULL)];
    f32 interval = lbl_803465F0[RaySceneTest(NULL)];
    if (*(s32*)(arg0 + 0x2AC) < limit) {
        f32 acc = *(f32*)(arg0 + 0x350) + dt;
        *(f32*)(arg0 + 0x350) = acc;
        if (acc > interval) {
            s32 next = *(s32*)(arg0 + 0x2AC) + 1;
            s32 cap = lbl_803465C0[RaySceneTest(NULL)];
            if (next < cap) cap = next;
            *(s32*)(arg0 + 0x2AC) = cap;
            *(f32*)(arg0 + 0x350) = 0.0f;
        }
    }
    AimMinigame_Update(arg0);
}

// 0x8007EB58
// Reads the stick (squared, keeping sign), eases the value at +0xB8 toward it by at most 1 per
// call, plays/stops the turning sounds, turns the angle at +0xA8 within its limits and hands the
// selected slot index over to the experiment object (+0x1F4). The second argument is unused.
void fn_8007EB58(u8* arg0, f32 /*unused dt*/) {
    f32 sy = 0.0f;
    f32 sx = 0.0f;
    StickRead(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0xE, &sx, &sy);
    f32 sign = 0.0f;
    if (sx != 0.0f) {
        if (sx < 0.0f) sign = -1.0f;
        else sign = 1.0f;
    }
    f32 target = sx * (sx * sign);
    sx = target;
    f32 cur = *(f32*)(arg0 + 0xB8);
    if (cur < target) {
        f32 n = 1.0f + cur;
        f32 r;
        if (n < cur) r = cur;
        else if (n > target) r = target;
        else r = n;
        *(f32*)(arg0 + 0xB8) = r;
    } else if (cur > target) {
        f32 n = cur - 1.0f;
        f32 r;
        if (n < target) {
            r = target;
        } else {
            if (n > cur) n = cur;
            r = n;
        }
        *(f32*)(arg0 + 0xB8) = r;
    }
    if (0.0f != *(f32*)(arg0 + 0xB8)) {
        void* owner = *(void**)(arg0 + 0x1F4);
        if (0.0f == cur) {
            PlayExperimentSound(owner, 0x867F5E9Fu, 1);
        } else if (fn_80070D88(owner, 0x867F5E9Fu) == 0 && *(u8*)(arg0 + 0x394) == 0) {
            PlayExperimentSound(owner, 0x8BF23844u, 1);
            *(u8*)(arg0 + 0x394) = 1;
        }
        f32 lo = -*(f32*)(arg0 + 0xB0);
        f32 hi = 0.35f + *(f32*)(arg0 + 0xAC);
        f32 ang = *(f32*)(arg0 + 0xA8) + 0.017453292f * (-*(f32*)(arg0 + 0xB8) * *(f32*)(arg0 + 0xC4));
        f32 r;
        if (lo < hi) {
            if (ang < lo) r = lo;
            else if (ang > hi) r = hi;
            else r = ang;
        } else {
            if (ang < hi) r = hi;
            else if (ang > lo) r = lo;
            else r = ang;
        }
        *(f32*)(arg0 + 0xA8) = r;
    } else if (0.0f != cur) {
        *(u8*)(arg0 + 0x394) = 0;
        StopExperimentSound(*(void**)(arg0 + 0x1F4), 0x8BF23844u);
        PlayExperimentSound(*(void**)(arg0 + 0x1F4), 0x97FDFC6Fu, 1);
    }
    u8 held = *(u8*)(arg0 + 0x1C0);
    if (held != 0 && *(u8*)(arg0 + 0x1EC) == 0) {
        u8* exp = *(u8**)(arg0 + 0x1F4);
        if (*(void**)(exp + 0x264) != NULL) *(s32*)(exp + 0x268) = 0;
    }
    if (*(u8*)(arg0 + 0x158) != 0) {
        f32 f = *(f32*)(arg0 + 0x15C);
        if (f > 0.0f) *(f32*)(arg0 + 0xD0) = f;
    }
    if (*(u8*)(arg0 + 0x158) != 0) {
        if (*(void**)(*(u8**)(arg0 + 0x1F4) + 0x264) != NULL) {
            s32 idx;
            if (*(u8*)(arg0 + 0xC0) != 0) {
                u8* sub = *(u8**)(arg0 + 0x29C);
                idx = fn_80083930(sub, *(f32**)(sub + 0x4));
            } else {
                idx = *(s32*)(arg0 + 0x170);
            }
            u8* exp = *(u8**)(arg0 + 0x1F4);
            s32 state = idx + 1;
            *(s32*)(exp + 0x268) = state;
            u8* other = *(u8**)(exp + 0x264);
            if (other != NULL && state > 0) {
                *(f32*)(other + 0x2B0) = lbl_803464F0[idx];
            }
        }
    } else if (*(u8*)(arg0 + 0x1EC) != 0 && held == 0) {
        *(s32*)(*(u8**)(arg0 + 0x1F4) + 0x268) = -1;
    }
    *(u8*)(arg0 + 0x1EC) = held;
}

// 0x8007FA1C
// Finds the target-zone record (key 0x38425456) in the loaded zones, then builds the aim frame:
// axes at +0x90/+0x9C, angle limits at +0xAC/+0xB0, range at +0xB4, and the two locator
// matrices at +0x2BC/+0x2EC, then runs the aim minigame update.
void fn_8007FA1C(u8* arg0) {
    f32 mA[12];
    f32 mB[12];
    f32 mC[12];
    f32 mD[12];
    f32 mE[12];
    f32 mF[12];
    f32 mG[12];
    Vec3 sp74, sp68, sp50, sp44, sp38, sp2C, sp20, sp14, sp8;
    f32 sp5C[3];
    u8* zone;
    s32 i = 0;

    while (i <= *(s32*)(*(u8**)gWorldState + 0x4) && *(u8**)(arg0 + 0x8C) == NULL) {
        if (ZoneIsLoaded(gWorldState, i)) {
            zone = fn_8007E0B8(*(u8**)gWorldState + 0x1A4, i);
            for (s32 j = 0; j < *(u8*)(zone + 0xAC); j++) {
                u8* e = *(u8**)(*(u8**)(zone + 0xA8) + j * 4);
                if (*(s32*)(e + 0x8) == 0 && *(u32*)(e + 0x4) == 0x38425456u) {
                    *(u8**)(arg0 + 0x8C) = e;
                    break;
                }
            }
        }
        i++;
    }
    u8* ref = fn_80081A34();
    u8* world = *(u8**)gWorldState;
    u8* lo = fn_8007DD0C(world, fn_8007DD38(world, 0x064FA84Bu));
    world = *(u8**)gWorldState;
    u8* hi = fn_8007DD0C(world, fn_8007DD38(world, 0xBD7D6A9Cu));
    world = *(u8**)gWorldState;
    u8* base = fn_8007DD0C(world, fn_8007DD38(world, 0x834C7963u));
    u8* rec = *(u8**)(arg0 + 0x8C);
    Vec3Sub(&sp68, (Vec3*)(*(u8**)(rec + 0x58) + 8), (Vec3*)(*(u8**)(rec + 0x5C) + 8));
    Vec3Copy((Vec3*)(arg0 + 0x9C), &sp68);
    Vec3SetZ((Vec3*)(arg0 + 0x9C), 0.0f);
    f32 negX = -Vec3GetX((Vec3*)(arg0 + 0x9C));
    fn_800359F4((u8*)sp5C, Vec3GetY((Vec3*)(arg0 + 0x9C)), negX, 0.0f);
    Vec3Copy((Vec3*)(arg0 + 0x90), (Vec3*)sp5C);
    u8* lvlObj = *(u8**)(lbl_803C4298 + 0x4);
    (*(VoidFn*)(*(u8**)lvlObj + 0x10))(lvlObj);
    fn_80052638();
    Vec3Sub(&sp50, (Vec3*)(hi + 8), (Vec3*)(ref + 8));
    Vec3Copy(&sp74, &sp50);
    Vec3Negate(&sp44, (Vec3*)(arg0 + 0x9C));
    *(f32*)(arg0 + 0xAC) = kHalfPi * fn_8005B4A4(&sp44, &sp74);
    Vec3Sub(&sp38, (Vec3*)(lo + 8), (Vec3*)(ref + 8));
    Vec3Copy(&sp74, &sp38);
    Vec3Negate(&sp2C, (Vec3*)(arg0 + 0x9C));
    *(f32*)(arg0 + 0xB0) = kHalfPi * fn_8005B4A4(&sp2C, &sp74);
    *(f32*)(arg0 + 0xB4) = 3.0f * fn_8007DD7C((Vec3*)(arg0 + 0x90));
    Vec3NormalizeTo(&sp20, (Vec3*)(arg0 + 0x90));
    Vec3Copy((Vec3*)(arg0 + 0x90), &sp20);
    Vec3NormalizeTo(&sp14, (Vec3*)(arg0 + 0x9C));
    Vec3Copy((Vec3*)(arg0 + 0x9C), &sp14);
    Vec3Sub(&sp8, (Vec3*)fn_800526DC(*(u8**)(arg0 + 0x1F4)), (Vec3*)(base + 8));
    Vec3Copy((Vec3*)(arg0 + 0x2B0), &sp8);
    fn_800526D0((u8*)mA);
    MtxFromLocatorRotation(mA, ref + 0x14);
    fn_8007FD98((u8*)mA, ref + 8);
    fn_800526D0((u8*)mB);
    MtxFromLocatorRotation(mB, base + 0x14);
    fn_8007FD98((u8*)mB, base + 8);
    fn_8005C204((u8*)mC, (u8*)mA);
    fn_8005535C((u8*)mA, (u8*)mC);
    fn_800554DC((u8*)mD, (u8*)mA, (u8*)mB);
    fn_8005535C(arg0 + 0x2BC, (u8*)mD);
    fn_8007FDCC((u8*)mE, (u8*)fn_801B50FC(*(u8**)(arg0 + 0x1F4)));
    fn_8005C204((u8*)mF, (u8*)mB);
    fn_8005535C((u8*)mB, (u8*)mF);
    fn_800554DC((u8*)mG, (u8*)mB, (u8*)mE);
    fn_8005535C(arg0 + 0x2EC, (u8*)mG);
    AimMinigame_Update(arg0);
}

// 0x80081674
// When the spawn timer (+0x38C) runs out, re-arms it and launches a prop: activates it at the
// spawn point, sends it the launch event and gives it a random kick direction (unit vector in
// the XY plane, scaled by 10).
void fn_80081674(u8* arg0, f32 dt) {
    f32 pos[3];
    f32 rnd[3];
    f32 dir[3];
    f32 kick[3];
    ScopedLock lock;

    f32 t = *(f32*)(arg0 + 0x38C) - dt;
    *(f32*)(arg0 + 0x38C) = t;
    if (!(t <= 0.0f)) return;
    f32 div = lbl_80346590[RaySceneTest(arg0)];
    *(f32*)(arg0 + 0x38C) = lbl_803465B0[RaySceneTest(NULL)] / div;
    u8* o = fn_8007F900(arg0, 0);
    if (o == NULL) return;
    ActivateProp(o);
    fn_80084D3C(pos, *(u8**)(arg0 + 0x29C));
    PlaceProp(o, pos[0], pos[1], pos[2]);
    *(f32*)(o + 0x280) = pos[0];
    *(f32*)(o + 0x284) = pos[1];
    *(f32*)(o + 0x288) = pos[2];
    *(f32*)(o + 0x274) = 0.0f;
    *(f32*)(o + 0x278) = 0.6f;
    *(f32*)(o + 0x2B0) = lbl_803464F0[0];
    ScopedLockCtor(&lock);
    (*(SetFlagFn*)(*(u8**)o + 0x14))(o, 1);
    ScopedLockDtor(&lock, -1);
    *(s32*)(o + 0x2B8) = 0;
    EventSend(gEventMgr, o, NULL, 0x6A19C907u, 0, 0, 1, 0);
    f32 ry = -1.0f + 2.0f * (kU32ToUnit * (f32)RandomU32());
    f32 rx = -1.0f + 2.0f * (kU32ToUnit * (f32)RandomU32());
    rnd[1] = ry;
    rnd[2] = 0.0f;
    rnd[0] = rx;
    f32 lenSq = 0.0f * 0.0f + (rx * rx + ry * ry);
    if (lenSq == 0.0f) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(lenSq);
        dir[0] = rx * inv;
        dir[1] = ry * inv;
        dir[2] = 0.0f * inv;
    }
    f32 kx = dir[0] * 10.0f;
    f32 ky = dir[1] * 10.0f;
    dir[0] = kx;
    dir[1] = ky;
    dir[2] *= 10.0f;
    kick[0] = pos[0] + kx;
    kick[1] = pos[1] + ky;
    kick[2] = 0.0f;
    fn_80082120(o, kick);
}

// 0x80083D68
// Rolls the aim target for a throw: picks slot 0/1/2 from a random number, solves the angle
// (law of cosines, range from the +0x264 object) for the nearest and farthest edge of the slot,
// picks a random angle between them with a random sign, clamps it to the angle limits and stores
// the resulting aim vector at +0x274 and whether it hit the current slot at +0x28C.
void fn_80083D68(u8* arg0) {
    f32 aim[3];
    f32 r0 = lbl_80346600[RaySceneTest(arg0)];
    f32 r1 = lbl_80346610[RaySceneTest(NULL)];
    s32 cur = fn_80083930(arg0, *(f32**)(arg0 + 0x4));
    u8* pad = *(u8**)(arg0 + 0x264);
    f32 range = *(f32*)(pad + 0xB4);
    f32 r1sq = r1 * r1;
    f32 r0sq = r0 * r0;
    f32 d = lbl_803464E4[cur] * range;
    f32 dsq = d * d;
    f32 base = fn_80081210(pad, *(u8**)(arg0 + 0x4));
    u32 rv = RandomU32();
    f32 roll = kU32ToUnit * (f32)rv;
    roll = roll - lbl_80346500[RaySceneTest(NULL)];
    s32 slot = 0;
    if (roll > 0.0f) slot = 1;
    roll = roll - lbl_80346510[RaySceneTest(NULL)];
    if (roll > 0.0f) slot = 2;
    f32 e = lbl_803464E4[slot] * *(f32*)(*(u8**)(arg0 + 0x264) + 0xB4);
    f32 esq = e * e;
    f32 den = (-2.0f * d) * e;
    f32 c0 = (r0sq - (dsq + esq)) / den;
    f32 cl;
    if (c0 < -1.0f) cl = -1.0f;
    else if (c0 > 1.0f) cl = 1.0f;
    else cl = c0;
    f32 c1 = (r1sq - (dsq + esq)) / den;
    f32 a0 = kHalfPi * (kTwoOverPi * (f32)fn_8033F1F4((f64)cl));
    if (c1 < -1.0f) cl = -1.0f;
    else if (c1 > 1.0f) cl = 1.0f;
    else cl = c1;
    f32 a1 = kHalfPi * (kTwoOverPi * (f32)fn_8033F1F4((f64)cl));
    f32 ang = a0 + (a1 - a0) * (kU32ToUnit * (f32)RandomU32());
    f32 sgn;
    if (RandomU32() & 1) sgn = -1.0f;
    else sgn = 1.0f;
    f32 delta = ang * sgn;
    u8* obj = *(u8**)(arg0 + 0x264);
    f32 res = base + delta;
    if (res > *(f32*)(obj + 0xB0) || res < -*(f32*)(obj + 0xAC)) {
        res = base + -delta;
    }
    f32 hi = *(f32*)(obj + 0xB0);
    f32 lo = -*(f32*)(obj + 0xAC);
    f32 clampedRes;
    if (lo < hi) {
        if (res < lo) clampedRes = lo;
        else if (res > hi) clampedRes = hi;
        else clampedRes = res;
    } else {
        if (res < hi) clampedRes = hi;
        else if (res > lo) clampedRes = lo;
        else clampedRes = res;
    }
    f32 scaled = clampedRes * 0.9f;
    if (slot == 0) scaled *= 0.8f;
    fn_80081004((u8*)aim, obj, slot, scaled);
    *(f32*)(arg0 + 0x274) = aim[0];
    *(f32*)(arg0 + 0x278) = aim[1];
    *(f32*)(arg0 + 0x27C) = aim[2];
    *(u8*)(arg0 + 0x28C) = (u8)(cur == slot);
}

// 0x800857A8
// Hand-off: remembers the current slot (+0x14) and, when the held prop (+0x10) has been dropped
// (no object at +0x264 of the experiment), activates two fresh props at its position and sends
// them off in opposite directions (+/- pi/8), then resets the timer at +0xC.
void fn_800857A8(u8* arg0) {
    f32 pos[3];
    u8* exp = *(u8**)(*(u8**)(arg0 + 0x4) + 0x1F4);
    s32 state = *(s32*)(exp + 0x268);
    u8* held = *(u8**)(exp + 0x264);
    if (state > 0) *(s32*)(arg0 + 0x14) = state;
    u8* prev = *(u8**)(arg0 + 0x10);
    if (prev != NULL && held == NULL) {
        pos[0] = *(f32*)(prev + 0x280);
        pos[1] = *(f32*)(prev + 0x284);
        pos[2] = *(f32*)(prev + 0x288);
        u8* o = fn_8007F900(*(u8**)(arg0 + 0x4), *(s32*)(prev + 0x264));
        PlaceProp(o, pos[0], pos[1], pos[2]);
        ActivateProp(o);
        fn_8007EF04(*(u8**)(arg0 + 0x4), o, *(s32*)(arg0 + 0x14), 0.3926991f);
        EventSend(gEventMgr, o, NULL, 0x49C2D2C1u, 0, 0, 1, 0);
        o = fn_8007F900(*(u8**)(arg0 + 0x4), *(s32*)(*(u8**)(arg0 + 0x10) + 0x264));
        PlaceProp(o, pos[0], pos[1], pos[2]);
        ActivateProp(o);
        fn_8007EF04(*(u8**)(arg0 + 0x4), o, *(s32*)(arg0 + 0x14), -0.3926991f);
        EventSend(gEventMgr, o, NULL, 0x49C2D2C1u, 0, 0, 1, 0);
        *(f32*)(arg0 + 0xC) = -1.0f;
    }
    *(u8**)(arg0 + 0x10) = held;
}

}  // extern "C"
