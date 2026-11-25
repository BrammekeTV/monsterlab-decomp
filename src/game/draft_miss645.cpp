// Mixed functions: staple-tile spin pick (0x800C98F8), level-intro camera setup (0x800CEB8C),
// free-look stick camera (0x800DF7C4), 3D text queue (0x80121508), UI tint pass (0x801A0200),
// collision contact dispatch (0x801AFC98).
#include <string.h>

#include "types.h"
#include "engine/math.h"

extern "C" {
u32   RandomU32(void);                                              // 0x80143460
s32   RaySceneTest(void* ray);                                      // 0x80070458
u32   PadGetHeld(s32 chan);                                         // 0x801B9B98
void  StickRead(s32 channel, f32* x, f32* y);                       // 0x80147704
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);    // 0x80145EE8
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);         // 0x80146180 (rotate quat about axis)
f64   fn_8033F1F4(f64 x);                                           // 0x8033F1F4 (MSL acos)
u8*   fn_800C965C(u8* a, f32* pos, u8* obj);                        // 0x800C965C (r3 unused by the callee)
void* fn_800CF71C(void);                                            // 0x800CF71C
void  fn_800D0174(u8* p);                                           // 0x800D0174 (resets the pos/quat at p+0x58)
void  fn_800CFDCC(u8* p, f32 x);                                    // 0x800CFDCC
void* fn_801EB94C(void* p);                                         // 0x801EB94C
void* fn_8019AF88(void* obj, char* name, u32 arg);                  // 0x8019AF88
u8*   fn_80196C9C(void* p);                                         // 0x80196C9C
s32   fn_801A1238(u8* obj, f32* colors);                            // 0x801A1238
s32   fn_801C08CC(u8* a, u8* b, u8* c, u8* d);                      // 0x801C08CC
s32   fn_801C098C(u8* a, u8* b, u8* c, u8* d, u8* e, f32* out0, f32* out1);   // 0x801C098C
s32   fn_801C19F4(u8* a, u8* b, u8* c, u8* d, s32* n, f32* pos, f32* dist);   // 0x801C19F4
s32   fn_801C1E04(u8* a, u8* b, u8* c, u8* d, s32* n, f32* pos, f32* dist);   // 0x801C1E04

extern u8   gGame[];              // 0x803B53A0
extern u8   lbl_803C2A20[];       // 0x803C2A20 (3D text queue)
extern u8   lbl_803C4298[];       // 0x803C4298 (+0x4/+0x8: objects with vtables, +0x38: pad channel, +0x3C: flags)
extern void* lbl_805FB658;        // 0x805FB658 (global render object)

extern f32 lbl_80347290[];        // 0x80347290
extern s32 lbl_803472A0[];        // 0x803472A0
extern f32 lbl_803472B0[];        // 0x803472B0
extern f32 lbl_803472C0[];        // 0x803472C0
extern f32 lbl_803472D0[];        // 0x803472D0
extern f32 lbl_803472E0[];        // 0x803472E0
extern f32 lbl_80347420[];        // 0x80347420
extern f32 lbl_80347430[];        // 0x80347430

extern f32 lbl_805FE4B8;          // 0x805FE4B8 (float constants, see asm)
extern f32 lbl_805FE4BC;          // 0x805FE4BC
extern f32 lbl_805FE4D8;          // 0x805FE4D8
extern f32 lbl_805FE4DC;          // 0x805FE4DC
extern f32 lbl_805FE4E0;          // 0x805FE4E0
extern f32 lbl_805FE4E4;          // 0x805FE4E4
extern f32 lbl_805FE558;          // 0x805FE558
extern f32 lbl_805FE574;          // 0x805FE574
extern f32 lbl_805FE578;          // 0x805FE578
extern f32 lbl_805FE57C;          // 0x805FE57C
extern f32 lbl_805FE580;          // 0x805FE580
extern f32 lbl_805FE594;          // 0x805FE594
extern f32 lbl_805FE598;          // 0x805FE598
extern f32 lbl_805FE830;          // 0x805FE830
extern f32 lbl_805FE850;          // 0x805FE850
extern f32 lbl_805FE854;          // 0x805FE854
extern f32 lbl_805FE858;          // 0x805FE858
extern f32 lbl_805FE860;          // 0x805FE860
extern f32 lbl_805FE864;          // 0x805FE864
extern f32 lbl_805FE868;          // 0x805FE868
extern f32 lbl_805FE86C;          // 0x805FE86C
extern f32 lbl_805FE870;          // 0x805FE870
extern f32 lbl_805FE874;          // 0x805FE874
extern f32 lbl_805FE878;          // 0x805FE878
extern f32 lbl_805FE87C;          // 0x805FE87C
extern f32 lbl_805FE880;          // 0x805FE880
extern f32 lbl_805FED90;          // 0x805FED90
extern f32 lbl_805FED94;          // 0x805FED94
extern f32 lbl_805FED98;          // 0x805FED98
extern f32 lbl_805FF998;          // 0x805FF998
extern f32 lbl_805FF99C;          // 0x805FF99C
extern f32 lbl_805FFAB8;          // 0x805FFAB8
extern f32 lbl_805FFABC;          // 0x805FFABC
extern f32 lbl_805FFAC4;          // 0x805FFAC4
}

extern "C++" void* gScreenSets;                 // 0x805FB654
extern "C++" f32 GetSign10(u8* p);              // 0x80013F70
extern "C++" f32 GetSignFlip11(u8* p);          // 0x80013F54

static const u32 kHashStapleTile01A = 0xBB2F4E0Bu;   // "staple_tile_01A"
static const u32 kHashStapleTile02A = 0x90021DC8u;   // "staple_tile_02A"

typedef void (*VFnObjInt)(u8* obj, s32 a);
typedef void (*VFnObjFloat)(u8* obj, f32 x);
typedef void (*VFnObjVec)(u8* obj, f32* a);
typedef void (*VFnObjVec2)(u8* obj, f32* a, f32* b);
typedef void (*VFnOutObj)(f32* out, u8* obj);

// Square root that returns 0 for zero/negative input (the original's frsqrte path).
static inline f32 SqrtOrZero(f32 x) {
    return x > lbl_805FE4B8 ? sqrtf(x) : lbl_805FE4B8;
}

extern "C" {

// 0x800C98F8
// Re-aims a staple tile's orientation: measures the angle between the +X axis and the vector at
// the hit entry +0xC, turns it into a spin about +Y (random direction, extra random wobble for
// the two staple_tile kinds), writes the new quaternion into the entity and sets its dirty flags.
void fn_800C98F8(u8* arg0, u8* arg1, u8* arg2) {
    if (arg1 == NULL) return;

    f32 dir = (RandomU32() & 1) ? lbl_805FE4BC : lbl_805FE4D8;

    u8* cur = *(u8**)(arg1 + 0x4);
    f32 pos[3];
    pos[0] = *(f32*)(cur + 0x0);
    pos[1] = *(f32*)(cur + 0x4);
    pos[2] = *(f32*)(cur + 0x8);
    u8* hit = fn_800C965C(arg0, pos, arg2);
    if (hit == NULL) return;

    cur = *(u8**)(arg1 + 0x4);
    Quat q;
    q.x = *(f32*)(cur + 0xC);
    q.y = *(f32*)(cur + 0x10);
    q.z = *(f32*)(cur + 0x14);
    q.w = *(f32*)(cur + 0x18);

    // Reference vector (1, 0, 0) against the vector at hit+0xC.
    f32 ax = lbl_805FE4BC, ay = lbl_805FE4B8, az = lbl_805FE4B8;
    f32 bx = *(f32*)(hit + 0xC);
    f32 by = *(f32*)(hit + 0x10);
    f32 bz = *(f32*)(hit + 0x14);

    f32 lenA = SqrtOrZero(az * az + (ax * ax + ay * ay));
    f32 lenB = SqrtOrZero(bz * bz + (bx * bx + by * by));
    f32 dot = az * bz + (ax * bx + ay * by);

    f32 turn = lbl_805FE4B8;
    f32 prod = lenA * lenB;
    if (!(prod <= lbl_805FE4B8)) {
        f32 c = dot / prod;
        if (c < lbl_805FE4D8) c = lbl_805FE4D8;
        else if (c > lbl_805FE4BC) c = lbl_805FE4BC;
        turn = lbl_805FE4DC * (f32)fn_8033F1F4((f64)c);
    }
    f32 angle = lbl_805FE4BC - turn;
    if (bz < lbl_805FE4B8) angle = -angle;

    u8* info = *(u8**)(arg0 + 0x9C);
    s32 kind = (info != NULL) ? *(s32*)(info + 0x8) : 0;
    if (kind == (s32)kHashStapleTile01A || kind == (s32)kHashStapleTile02A) {
        // RaySceneTest is called with leftover register values (0xBB2F0000/0x90020000 then the
        // previous result); pass 0.
        s32 i0 = RaySceneTest(NULL);
        f32 lo = lbl_805FE4E0 * lbl_80347420[i0];
        s32 i1 = RaySceneTest(NULL);
        f32 hi = lbl_805FE4E0 * lbl_80347430[i1];
        f32 jitter = lo + (hi - lo) * (lbl_805FE4E4 * (f32)RandomU32());
        angle += jitter * ((dir < lbl_805FE4B8) ? lbl_805FE4BC : lbl_805FE4D8);
    }

    Vec3 axis;
    axis.x = lbl_805FE4B8;
    axis.y = lbl_805FE4BC;
    axis.z = lbl_805FE4B8;
    QuatFromAxisAngle(&q, &axis, angle);

    u8* o = *(u8**)(arg1 + 0x4);
    *(f32*)(o + 0xC) = q.x;
    *(f32*)(o + 0x10) = q.y;
    *(f32*)(o + 0x14) = q.z;
    *(f32*)(o + 0x18) = q.w;
    o = *(u8**)(arg1 + 0x4);
    *(u32*)(o + 0x20) |= 0x8000;
    o = *(u8**)(arg1 + 0x4);
    *(u32*)(o + 0x20) |= 0x10000000;
    o = *(u8**)(arg1 + 0x4);
    *(u32*)(o + 0x20) |= 0x4000;
    o = *(u8**)(arg1 + 0x4);
    *(u32*)(o + 0x20) |= 0x10000000;
}

// 0x800CEB8C
// Sets up the camera/helper object for a level: resets the entry found by fn_800CF71C, rolls the
// random orbit parameters (+0x90 flag, +0x94 spin speed, +0x98 distance), positions the camera
// from the helper at arg0+0x3020 and stores the resulting position/orientation back.
void fn_800CEB8C(u8* arg0) {
    u8* cam = *(u8**)(lbl_803C4298 + 0x4);
    (*(VFnObjInt*)(*(u8**)cam + 0x50))(cam, 0);
    (*(VFnObjInt*)(*(u8**)cam + 0xC))(cam, 6);

    u8* entry = (u8*)fn_800CF71C();
    fn_800D0174(entry);

    f32 zero = lbl_805FE558;
    f32 one = lbl_805FE598;
    Quat q;
    q.x = zero;
    q.y = zero;
    q.z = zero;
    q.w = one;
    Vec3 axis;
    axis.x = zero;
    axis.y = one;
    axis.z = zero;
    fn_80146180(&q, &axis, zero);
    *(f32*)(entry + 0x58) = zero;
    *(f32*)(entry + 0x5C) = zero;
    *(f32*)(entry + 0x60) = zero;
    *(f32*)(entry + 0x64) = q.x;
    *(f32*)(entry + 0x68) = q.y;
    *(f32*)(entry + 0x6C) = q.z;
    *(f32*)(entry + 0x70) = q.w;

    *(f32*)(arg0 + 0xA0) = lbl_805FE558;
    *(f32*)(arg0 + 0x9C) = lbl_805FE558;

    // RaySceneTest gets leftover register contents as its argument; pass 0.
    if (lbl_803472A0[RaySceneTest(NULL)] != 0) {
        *(u8*)(arg0 + 0x90) = 1;
        f32 a = lbl_803472C0[RaySceneTest(NULL)];
        f32 b = lbl_803472B0[RaySceneTest(NULL)];
        f32 r = lbl_805FE574 * (f32)RandomU32();
        *(f32*)(arg0 + 0x94) = lbl_805FE578 * (b + (a - b) * r);
        if (RandomU32() & 1) {
            *(f32*)(arg0 + 0x94) = *(f32*)(arg0 + 0x94) * lbl_805FE57C;
        }
        f32 c = lbl_803472E0[RaySceneTest(NULL)];
        f32 d = lbl_803472D0[RaySceneTest(NULL)];
        f32 r2 = lbl_805FE574 * (f32)RandomU32();
        *(f32*)(arg0 + 0x98) = d + (c - d) * r2;
    } else {
        *(u8*)(arg0 + 0x90) = 0;
        *(f32*)(arg0 + 0x98) = lbl_805FE580;
    }

    u8* helper = arg0 + 0x3020;
    u8* helperVt = *(u8**)(arg0 + 0x30F4);   // vtable of the embedded helper object
    f32 scale = lbl_805FE558 * lbl_80347290[RaySceneTest(NULL)];
    (*(VFnObjFloat*)(helperVt + 0x1C))(helper, scale);

    f32 look[3];
    look[0] = lbl_805FE558;
    look[1] = lbl_805FE594;
    look[2] = lbl_805FE558;
    (*(VFnObjFloat*)(helperVt + 0x20))(helper, lbl_805FE558);
    f32 off[3];
    (*(VFnOutObj*)(helperVt + 0x24))(off, helper);
    look[0] = look[0] + off[0];
    look[1] = look[1] + off[1];
    look[2] = look[2] + off[2];

    cam = *(u8**)(lbl_803C4298 + 0x4);
    (*(VFnObjVec2*)(*(u8**)cam + 0x2C))(cam, look, off);
    (*(VFnObjVec*)(*(u8**)cam + 0x1C))(cam, look);
    u8* cam2 = *(u8**)(lbl_803C4298 + 0x8);
    (*(VFnObjFloat*)(*(u8**)cam2 + 0x98))(cam2, lbl_805FE558);

    fn_800CFDCC(arg0 + 0x90, lbl_805FE558);

    u8* entry2 = (u8*)fn_800CF71C();
    Quat q2;
    q2.x = lbl_805FE558;
    q2.y = lbl_805FE558;
    q2.z = lbl_805FE558;
    q2.w = lbl_805FE598;
    Vec3 axis2;
    axis2.x = lbl_805FE558;
    axis2.y = lbl_805FE598;
    axis2.z = lbl_805FE558;
    fn_80146180(&q2, &axis2, *(f32*)(arg0 + 0xA0));
    *(f32*)(entry2 + 0x58) = look[0];
    *(f32*)(entry2 + 0x5C) = look[1];
    *(f32*)(entry2 + 0x60) = look[2];
    *(f32*)(entry2 + 0x64) = q2.x;
    *(f32*)(entry2 + 0x68) = q2.y;
    *(f32*)(entry2 + 0x6C) = q2.z;
    *(f32*)(entry2 + 0x70) = q2.w;
    cam2 = *(u8**)(lbl_803C4298 + 0x8);
    (*(VFnObjFloat*)(*(u8**)cam2 + 0x98))(cam2, lbl_805FE558);
}

// 0x800DF7C4
// Free-look camera step: reads the pad stick, turns it into yaw/pitch deltas (or, with the other
// held buttons, into a pan velocity), clamps yaw to +-pi and pitch to +-85 degrees, and blends
// the pan velocity at +0x20..+0x28 toward the target.
void fn_800DF7C4(u8* arg0, f32 dt) {
    f32 yawIn = lbl_805FE830;
    f32 pitchIn = yawIn;
    f32 pan[3];
    pan[0] = lbl_805FE830;   // sp10
    pan[1] = lbl_805FE830;   // sp14
    pan[2] = lbl_805FE830;   // sp18
    f32 sx, sy;              // stick x (spC), y (sp8)

    s32 chan = *(s32*)(lbl_803C4298 + 0x38);
    StickRead(chan * 16 + 0xE, &sx, &sy);

    if (PadGetHeld(*(s32*)(lbl_803C4298 + 0x38)) & 0x10) {
        yawIn = sx * GetSign10(gGame + 0xC448);
        pitchIn = -sy * GetSignFlip11(gGame + 0xC448);
    } else if (PadGetHeld(*(s32*)(lbl_803C4298 + 0x38)) & 0x20) {
        pan[0] = sx;
        pan[1] = lbl_805FE830;
        pan[2] = sy;
    } else if (PadGetHeld(*(s32*)(lbl_803C4298 + 0x38)) & 0x100) {
        s32 xDead = 0;
        if (sx > lbl_805FE860 && sx < lbl_805FE864) xDead = 1;
        bool skip = false;
        if (xDead != 0) {
            s32 yDead = 0;
            if (sy > lbl_805FE860 && sy < lbl_805FE864) yDead = 1;
            if (yDead != 0) skip = true;
        }
        if (!skip) {
            // Dominant stick axis becomes a +-1 step.
            s32 stepX = 0;
            s32 stepY = 0;
            if (fabsf(sx) > fabsf(sy)) {
                stepX = (sx > lbl_805FE830) ? 1 : -1;
            } else {
                stepY = (sy > lbl_805FE830) ? 1 : -1;
            }
            pan[0] = (f32)stepX + pan[0];
            pan[1] = (f32)stepY + pan[1];
        }
    } else {
        s32 xDead = 0;
        if (sx > lbl_805FE860 && sx < lbl_805FE864) xDead = 1;
        bool skip = false;
        if (xDead != 0) {
            s32 yDead = 0;
            if (sy > lbl_805FE860 && sy < lbl_805FE864) yDead = 1;
            if (yDead != 0) skip = true;
        }
        if (!skip) {
            yawIn = sx * GetSign10(gGame + 0xC448);
            pitchIn = -sy * GetSignFlip11(gGame + 0xC448);
        }
    }

    f32 t = lbl_805FE870 * (dt * lbl_805FE868);

    // Yaw (+0xC): add, clamp to +-2pi, wrap into +-pi, clamp again.
    f32 yaw = *(f32*)(arg0 + 0xC) + yawIn * t;
    *(f32*)(arg0 + 0xC) = yaw;
    f32 y1;
    if (yaw < lbl_805FE874) y1 = lbl_805FE874;
    else if (yaw > lbl_805FE878) y1 = lbl_805FE878;
    else y1 = yaw;
    if (y1 < lbl_805FE850) y1 = y1 + lbl_805FE878;
    else if (y1 > lbl_805FE858) y1 = y1 - lbl_805FE878;
    f32 y2;
    if (y1 < lbl_805FE850) y2 = lbl_805FE850;
    else if (y1 > lbl_805FE858) y2 = lbl_805FE858;
    else y2 = y1;

    // Pitch (+0x10): add, clamp to +-(85 degrees).
    f32 pitch = *(f32*)(arg0 + 0x10) + pitchIn * t;
    *(f32*)(arg0 + 0xC) = y2;
    f32 maxPitch = lbl_805FE870 * lbl_805FE86C;
    *(f32*)(arg0 + 0x10) = pitch;
    if (pitch >= maxPitch) pitch = maxPitch;
    f32 p2 = pitch;
    *(f32*)(arg0 + 0x10) = pitch;
    f32 minPitch = lbl_805FE870 * -lbl_805FE86C;
    if (!(p2 > minPitch)) p2 = minPitch;

    f32 cur = *(f32*)(arg0 + 0x20);
    f32 scaled0 = pan[0] * lbl_805FE87C;
    *(f32*)(arg0 + 0x10) = p2;
    pan[0] = scaled0;
    f32 blend = lbl_805FE880 * dt;
    pan[1] = pan[1] * lbl_805FE87C;
    pan[2] = pan[2] * lbl_805FE87C;
    if (fabsf(scaled0) < fabsf(cur)) {
        *(f32*)(arg0 + 0x20) = scaled0;
    } else {
        *(f32*)(arg0 + 0x20) = blend * scaled0 + (lbl_805FE854 - blend) * cur;
    }

    cur = *(f32*)(arg0 + 0x24);
    if (fabsf(pan[1]) < fabsf(cur)) {
        *(f32*)(arg0 + 0x24) = pan[1];
    } else {
        *(f32*)(arg0 + 0x24) = blend * pan[1] + (lbl_805FE854 - blend) * cur;
    }

    cur = *(f32*)(arg0 + 0x28);
    if (fabsf(pan[2]) < fabsf(cur)) {
        *(f32*)(arg0 + 0x28) = pan[2];
    } else {
        *(f32*)(arg0 + 0x28) = blend * pan[2] + (lbl_805FE854 - blend) * cur;
    }
}

// 0x80121508
// Queues a text string for 3D drawing: appends it (max 64 chars) to the active text buffer
// (the one at +0x10 or, when bit 1 of the flags at lbl_803C4298+0x3C is set, the one at +0x418)
// and fills a list node with the screen position (from pixel x/y), two floats and three ints.
void fn_80121508(char* text, s32 x, s32 y, s32 a, s32 b, s32 c, f32 d, f32 e) {
    u8* g = lbl_803C2A20;
    if (*(void**)(g + 0x10) == NULL) return;
    if (*(u8*)(g + 0x820) == 0) return;

    void* list;
    s32 offOff;    // offset of the buffer fill count
    s32 baseOff;   // offset of the buffer data
    if ((*(u32*)(lbl_803C4298 + 0x3C) >> 1) & 1) {
        list = *(void**)(g + 0x418);
        offOff = 0x81C;
        baseOff = 0x41C;
    } else {
        list = *(void**)(g + 0x10);
        offOff = 0x414;
        baseOff = 0x14;
    }

    u32 len = (u32)strlen(text);
    if (len + *(u32*)(g + offOff) + 1 > 0x400u) return;

    u8* node = (u8*)fn_801EB94C(list);
    if (node == NULL) return;

    *(u8**)node = g + *(s32*)(g + offOff) + baseOff;
    // Copy up to 64 chars, stopping after the terminator (the original unrolled this 4x16).
    for (s32 n = 0; n < 0x40; n++) {
        s32 off = *(s32*)(g + offOff);
        g[baseOff + off] = (u8)text[n];
        *(s32*)(g + offOff) = off + 1;
        if (text[n] == 0) break;
    }

    *(f32*)(node + 0x4) = lbl_805FED90 + lbl_805FED94 * (f32)x;
    *(f32*)(node + 0x8) = lbl_805FED98 - lbl_805FED94 * (f32)y;
    *(f32*)(node + 0xC) = d;
    *(f32*)(node + 0x10) = e;
    *(s32*)(node + 0x14) = a;
    *(s32*)(node + 0x18) = b;
    *(s32*)(node + 0x1C) = c;
}

// 0x801A0200
// Refreshes a UI element: caches the screen-set entry (+0x140) and its size (+0x148/+0x14C),
// multiplies the base colour at +0x80 by the render object's tint into +0x270, quantises it to
// 8 bits per channel and applies the same colour to all four vertices, then hides slot 0x20 via
// the second vtable.
s32 fn_801A0200(u8* arg0, char* name, u32 arg) {
    u8** entry = (u8**)fn_8019AF88(*(void**)((u8*)gScreenSets + 0x10), name, arg);
    *(u8***)(arg0 + 0x140) = entry;
    u8* tex = *entry;
    if (tex != NULL) {
        *(f32*)(arg0 + 0x148) = (f32)(s32)*(u16*)(tex + 0x4);
        *(f32*)(arg0 + 0x14C) = (f32)(s32)*(u16*)(*entry + 0x6);
    }

    u8* tint = fn_80196C9C(lbl_805FB658);
    if (tint != NULL) {
        *(f32*)(arg0 + 0x270) = *(f32*)(arg0 + 0x80) * *(f32*)(tint + 0x0);
        *(f32*)(arg0 + 0x274) = *(f32*)(arg0 + 0x84) * *(f32*)(tint + 0x4);
        *(f32*)(arg0 + 0x278) = *(f32*)(arg0 + 0x88) * *(f32*)(tint + 0x8);
        *(f32*)(arg0 + 0x27C) = *(f32*)(arg0 + 0x8C) * *(f32*)(tint + 0xC);
    } else {
        *(f32*)(arg0 + 0x270) = *(f32*)(arg0 + 0x80);
        *(f32*)(arg0 + 0x274) = *(f32*)(arg0 + 0x84);
        *(f32*)(arg0 + 0x278) = *(f32*)(arg0 + 0x88);
        *(f32*)(arg0 + 0x27C) = *(f32*)(arg0 + 0x8C);
    }

    // Quantise the four channels to 0..255 and back to floats.
    u32 packed = 0;
    for (s32 i = 0; i < 4; i++) {
        s32 v = (s32)(lbl_805FF99C * *(f32*)(arg0 + 0x270 + i * 4));
        if (v < 0) v = 0;
        else if (v > 0xFF) v = 0xFF;
        packed |= (u32)v << (i * 8);
    }
    f32 colors[16];
    for (s32 vtx = 0; vtx < 4; vtx++) {
        for (s32 i = 0; i < 4; i++) {
            colors[vtx * 4 + i] = lbl_805FF998 * (f32)(s32)((packed >> (i * 8)) & 0xFFu);
        }
    }
    fn_801A1238(arg0, colors);

    (*(VFnObjInt*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x20);
    return 0;
}

// 0x801AFC98
// Collision test of shape A (arg0) against shape C (arg2), dispatched on the type at C+0xC:
// 0 = box vs box (axis of least overlap, written as a contact to out1 or out2), 4/5/6/7 = other
// primitives through fn_801C08CC/19F4/1E04/098C. Returns nonzero on contact. arg5 (r8) is not used.
s32 fn_801AFC98(u8* arg0, f32* arg1, u8* arg2, f32* arg3, s32 arg4, s32 arg5, s32 arg6) {
    switch (*(s32*)(arg2 + 0xC)) {
    case 4:
        return fn_801C08CC(arg0 + 0x14, arg0 + 0x3C, arg2 + 0x14, arg2 + 0x3C) != 0;
    case 7:
        return fn_801C098C(arg2 + 0x14, arg2 + 0x6C, arg2 + 0x3C, arg0 + 0x14, arg0 + 0x3C,
                           arg3 + 0x3, arg1 + 0x3);
    case 0: {
        f32 dx = *(f32*)(arg2 + 0x14) - *(f32*)(arg0 + 0x14);
        f32 dy = *(f32*)(arg2 + 0x18) - *(f32*)(arg0 + 0x18);
        f32 dz = *(f32*)(arg2 + 0x1C) - *(f32*)(arg0 + 0x1C);
        f32 hx = *(f32*)(arg0 + 0x3C);
        f32 hy = *(f32*)(arg0 + 0x40);
        f32 hz = *(f32*)(arg0 + 0x44);
        f32 hc = *(f32*)(arg2 + 0x3C);
        f32 ovx = (hc + hx) - fabsf(dx);
        f32 ovy;
        f32 ovz;
        if (ovx < lbl_805FFAB8) return 0;
        ovy = (hc + hy) - fabsf(dy);
        if (ovy < lbl_805FFAB8) return 0;
        ovz = (hc + hz) - fabsf(dz);
        if (ovz < lbl_805FFAB8) return 0;
        if (ovz > lbl_805FFAB8) arg4 = 0;

        f32 nx, ny, nz, depth;
        if (fabsf(ovx) < fabsf(ovy)) {
            if (arg4 != 0 || fabsf(ovx) < fabsf(ovz)) {
                // X axis
                depth = fabsf(ovx);
                ny = lbl_805FFAB8;
                nz = ny;
                nx = (dx < ny) ? lbl_805FFAC4 : lbl_805FFABC;
            } else {
                // Z axis
                depth = fabsf(ovz);
                nx = lbl_805FFAB8;
                ny = nx;
                nz = (dz < nx) ? lbl_805FFAC4 : lbl_805FFABC;
            }
        } else if (arg4 != 0 || fabsf(ovy) < fabsf(ovz)) {
            // Y axis
            depth = fabsf(ovy);
            nx = lbl_805FFAB8;
            nz = nx;
            ny = (dy < nx) ? lbl_805FFAC4 : lbl_805FFABC;
        } else {
            // Z axis
            depth = fabsf(ovz);
            nx = lbl_805FFAB8;
            ny = nx;
            nz = (dz < nx) ? lbl_805FFAC4 : lbl_805FFABC;
        }

        f32 px = *(f32*)(arg0 + 0x14) + nx * depth;
        f32 py = *(f32*)(arg0 + 0x18) + ny * depth;
        f32 pz = *(f32*)(arg0 + 0x1C) + nz * depth;
        f32* out = (arg6 != 0) ? arg3 : arg1;
        out[0] = px;
        out[1] = py;
        out[2] = pz;
        out[3] = nx;
        out[4] = ny;
        out[5] = nz;
        out[6] = depth;
        return 1;
    }
    case 5: {
        s32 n;
        f32 pos[3];
        f32 dist;
        if (fn_801C19F4(arg0 + 0x14, arg0 + 0x3C, arg2 + 0x3C, arg2 + 0x48, &n, pos, &dist) != 0) {
            if (n > 0) {
                arg3[0] = pos[0];
                arg3[1] = pos[1];
                arg3[2] = pos[2];
                arg3[6] = dist;
            }
            return 1;
        }
        return 0;
    }
    case 6: {
        s32 n;
        f32 pos[3];
        f32 dist;
        if (fn_801C1E04(arg0 + 0x14, arg0 + 0x3C, arg2 + 0x3C, arg2 + 0x48, &n, pos, &dist) != 0) {
            if (n > 0) {
                arg3[0] = pos[0];
                arg3[1] = pos[1];
                arg3[2] = pos[2];
                arg3[6] = dist;
            }
            return 1;
        }
        return 0;
    }
    default:
        return 0;
    }
}

}  // extern "C"
