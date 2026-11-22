// Camera/path helpers and the pickup-throw setup, hand-verified against the asm:
// fn_800AAAF8 (closest point on a character's collision shapes), fn_800BE9E0 (move an object by its
// velocity, with a collision query), fn_800C2C80 (pop a queued object and age the active list),
// fn_800CA290 (build the smoothed path node list), fn_800CABD0 (sort Vec3s by distance),
// fn_800CE7E8 (reset the throw state and place the camera target).
#include <math.h>

#include "types.h"
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef void (*VFn0)(u8* self);
typedef f32 (*VFnF)(u8* self);
typedef void (*VFn1)(u8* self, u8* a);
typedef void (*VFnInt)(u8* self, s32 a);
typedef void (*VFnF1)(u8* self, f32 a);
typedef void (*VFn2)(u8* self, u8* a, u8* b);
typedef void (*VFnOut)(f32* out, u8* self);

extern "C++" void* gEventMgr;  // 0x805FB6C8

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);                  // 0x801BA2FC
void* MemAllocSmall(u32 size);                                                              // 0x801D41CC
void MemFreeSmall(void* p);                                                                 // 0x801D4340
u32 RandomU32(void);                                                                        // 0x80143460
s32 RaySceneTest(void* ray);                                                                // 0x80070458
void ScalarSet(f32* out, f32 v);                                                            // 0x80053250
void ScopedLockCtor(ScopedLock* l);                                                         // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                                              // 0x801F53B0
void Vec3AddTo(Vec3* v, const Vec3* d);                                                     // 0x800544FC
void Vec3Copy(Vec3* out, const Vec3* v);                                                    // 0x80052640
f32 Vec3GetY(const Vec3* v);
void Vec3ScaleBy(Vec3* v, const f32* s);                                                    // 0x80054A3C
void Vec3SetY(Vec3* v, f32 y);
void fn_80052638(void);                                                                  // 0x80052638
void fn_8006FC04(void);                                                                     // 0x8006FC04
s32 fn_800ACAAC(u8* p, u8* a, f32* out);                                                    // 0x800ACAAC
s32 fn_800AD5A8(u8* p, u8* a, f32* out);                                                    // 0x800AD5A8
s32 fn_800AEF04(u8* p, u8* a, f32* out);                                                    // 0x800AEF04
s32 fn_800B1AD0(u8* p, u8* a, f32* out);                                                    // 0x800B1AD0
s32 fn_800B20A4(u8* p, u8* a, f32* out);                                                    // 0x800B20A4
s32 fn_800BF370(u8* p, f32* pos, f32 a, f32 b);                                            // 0x800BF370
s32 fn_800C0168(u8* a, u8* node, f32* move, f32* hit, f32* push, u8* flag);                // 0x800C0168
void fn_800C1798(void* out, void* arg);                                                     // 0x800C1798
void fn_800C7620(u8* p);                                                                    // 0x800C7620
u8* fn_800C9244(u8* p);                                                                     // 0x800C9244
s32 fn_800C98A8(u8* p);                                                                     // 0x800C98A8
void fn_800C9CF0(u8** arr, u32 n);                                                          // 0x800C9CF0
f32 fn_800C9FB8(u8* v);                                                                     // 0x800C9FB8
void fn_800C9FC0(u8** arr, u32 n);                                                          // 0x800C9FC0
void fn_800CA8F0(u8* p);                                                                    // 0x800CA8F0
f32 fn_800CA900(u8* a, u8* b);                                                              // 0x800CA900
void fn_800CA91C(u8* p, f32 v);                                                             // 0x800CA91C
void fn_800CA924(u8* p, f32 v);                                                             // 0x800CA924
void fn_800CA92C(u8* p, f32 v);                                                             // 0x800CA92C
s32 fn_800CA934(s32 v);                                                                     // 0x800CA934 (abs)
void fn_800CA938(u8* out, u8* a, u8* b);                                                    // 0x800CA938
f32 fn_800CA98C(u8* a, u8* b);                                                              // 0x800CA98C
void fn_800CA9B0(u8* out, u8* v, u8* div);                                                  // 0x800CA9B0
f32 fn_800CAA00(u8* v);                                                                     // 0x800CAA00
void fn_800CAA70(u8* out, u8* a, u8* b);                                                    // 0x800CAA70
void fn_800CAAC4(u8* p);                                                                    // 0x800CAAC4
s32 fn_800CAAE0(u8* ring, u8* item);                                                        // 0x800CAAE0
void fn_800CCA88(u8* p);                                                                    // 0x800CCA88
s32 fn_800CD59C(u8* p);                                                                     // 0x800CD59C
void fn_800CDC44(s32* p);                                                                   // 0x800CDC44
void* fn_800CF71C(void);                                                                    // 0x800CF71C
void fn_800CF7E8(u8* p);                                                                    // 0x800CF7E8
void fn_800CFDCC(u8* p, f32 x);                                                             // 0x800CFDCC
u8* fn_800D01C0(void);                                                                      // 0x800D01C0
u8* fn_8010C608(u8* path, s32 index);                                                       // 0x8010C608
s32 fn_801449A4(f32* mat, f32* out);                                                        // 0x801449A4
void fn_801458B0(f32* quat, f32* a, f32* b);                                                // 0x801458B0
void fn_80146180(void* quat, const Vec3* axis, f32 angle);                                  // 0x80146180
void fn_801B6F34(u8* p);                                                                    // 0x801B6F34
void* fn_801D40E0(s32 n);                                                                   // 0x801D40E0
void* fn_8032F028(void* mem, void* ctor, void* dtor, s32 size, s32 count);                  // 0x8032F028
void operator_delete(void* p);                                                              // 0x801D426C

f32 fn_800CA288(u8* v);                                                                     // 0x800CA288

extern f32 lbl_80347290[];  // 0x80347290
extern s32 lbl_803472A0[];  // 0x803472A0
extern f32 lbl_803472B0[];  // 0x803472B0
extern f32 lbl_803472C0[];  // 0x803472C0
extern f32 lbl_803472D0[];  // 0x803472D0
extern f32 lbl_803472E0[];  // 0x803472E0
extern u8 lbl_803C4298[];   // 0x803C4298
extern f32 lbl_805FDF18;    // 0x805FDF18
extern f32 lbl_805FE308;    // 0x805FE308
extern f32 lbl_805FE320;    // 0x805FE320
extern f32 lbl_805FE328;    // 0x805FE328
extern f32 lbl_805FE390;    // 0x805FE390
extern f32 lbl_805FE4B8;    // 0x805FE4B8
extern f32 lbl_805FE4F0;    // 0x805FE4F0
extern f32 lbl_805FE4F4;    // 0x805FE4F4
extern f32 lbl_805FE4F8;    // 0x805FE4F8
extern f32 lbl_805FE4FC;    // 0x805FE4FC
extern f32 lbl_805FE558;    // 0x805FE558
extern f32 lbl_805FE574;    // 0x805FE574
extern f32 lbl_805FE578;    // 0x805FE578
extern f32 lbl_805FE57C;    // 0x805FE57C
extern f32 lbl_805FE580;    // 0x805FE580
extern f32 lbl_805FE594;    // 0x805FE594
extern f32 lbl_805FE598;    // 0x805FE598
}

static const u32 kHashStaplingGunA = 0x9B50E644u;  // "stapling_gunA"

// Squared distance from point c to point p, summed in the original order (z + (x + y)).
static inline f32 DistSq(const f32* c, const f32* p) {
    f32 dx = p[0] - c[0];
    f32 dy = p[1] - c[1];
    f32 dz = p[2] - c[2];
    return dz * dz + (dx * dx + dy * dy);
}

// Sqrt that returns zero (the constant) for non-positive input, as the original frsqrte path does.
static inline f32 SqrtGuard(f32 v) {
    return v <= lbl_805FE308 ? lbl_805FE308 : sqrtf(v);
}

// Keeps the candidate hit point when it is closer to the query point than the best so far.
static inline void KeepCloser(const f32* hit, u8* from, f32* bestDist, f32* best) {
    f32 dx = hit[0] - *(f32*)(from + 0x20);
    f32 dy = hit[1] - *(f32*)(from + 0x24);
    f32 dz = hit[2] - *(f32*)(from + 0x28);
    f32 d = dz * dz + (dx * dx + dy * dy);
    if (d < *bestDist) {
        *bestDist = d;
        best[0] = hit[0];
        best[1] = hit[1];
        best[2] = hit[2];
    }
}

// Swaps two 12-byte Vec3 records.
static inline void SwapVec3(u8* a, u8* b) {
    f32 x = *(f32*)(a + 0);
    f32 y = *(f32*)(a + 4);
    f32 z = *(f32*)(a + 8);
    *(f32*)(a + 0) = *(f32*)(b + 0);
    *(f32*)(a + 4) = *(f32*)(b + 4);
    *(f32*)(a + 8) = *(f32*)(b + 8);
    *(f32*)(b + 0) = x;
    *(f32*)(b + 4) = y;
    *(f32*)(b + 8) = z;
}

// Sets bits 0x8000, 0x10000000, 0x4000, 0x10000000 on the flags word at +0x20 of the position record.
static inline void MarkMoved(u8* rec) {
    *(u32*)(rec + 0x20) |= 0x8000;
    *(u32*)(rec + 0x20) |= 0x10000000;
    *(u32*)(rec + 0x20) |= 0x4000;
    *(u32*)(rec + 0x20) |= 0x10000000;
}

extern "C" {

// 0x800AAAF8
// Finds the point on the character's collision shapes (7 + 1 + 1 + 6 + 14 pieces) closest to the
// query's previous position (+0x20). Writes it to out if given; returns 1 when it differs from the
// query's current position (+0x14).
s32 fn_800AAAF8(u8* p, u8* q, f32* out) {
    f32 hit[3];
    f32 best[3];
    f32 bestDist;
    s32 i;

    hit[2] = lbl_805FDF18;
    hit[1] = lbl_805FDF18;
    hit[0] = lbl_805FDF18;
    f32 x = *(f32*)(q + 0x14);
    f32 y = *(f32*)(q + 0x18);
    f32 z = *(f32*)(q + 0x1C);
    f32 dx = x - *(f32*)(q + 0x20);
    f32 dy = y - *(f32*)(q + 0x24);
    f32 dz = z - *(f32*)(q + 0x28);
    best[0] = x;
    best[1] = y;
    best[2] = z;
    bestDist = dz * dz + (dx * dx + dy * dy);

    for (i = 0; i < 7; i++) {
        if (fn_800AEF04(*(u8**)(p + 0x118 + i * 4), q, hit) != 0) KeepCloser(hit, q, &bestDist, best);
    }
    if (fn_800B20A4(*(u8**)(p + 0x90), q, hit) != 0) KeepCloser(hit, q, &bestDist, best);
    if (fn_800B1AD0(*(u8**)(p + 0x94), q, hit) != 0) KeepCloser(hit, q, &bestDist, best);
    for (i = 0; i < 6; i++) {
        if (fn_800AD5A8(*(u8**)(p + 0x98 + i * 4), q, hit) != 0) KeepCloser(hit, q, &bestDist, best);
    }
    for (i = 0; i < 14; i++) {
        u8* slot = *(u8**)(p + 0xE0 + i * 4);
        if (*(s32*)(slot + 0x264) == 1 && fn_800ACAAC(slot, q, hit) != 0) KeepCloser(hit, q, &bestDist, best);
    }
    if (out != NULL) {
        out[0] = best[0];
        out[1] = best[1];
        out[2] = best[2];
    }
    if (best[0] != *(f32*)(q + 0x14) || best[1] != *(f32*)(q + 0x18) || best[2] != *(f32*)(q + 0x1C)) return 1;
    return 0;
}

// 0x800BE9E0
// Moves the object by velocity (+0x264..+0x26C) * dt through the collision query. With state 3 it
// first updates the position pair in the linked record chain. On a hit it switches to state 4,
// turns the push-out into a new velocity and reorients; otherwise it just advances the position.
void fn_800BE9E0(u8* p, u8* a, u8* out, f32 dt) {
    f32 hit[3];
    f32 push[3];
    f32 move[3];
    f32 dir[3];
    f32 base[3];
    f32 quat[4];
    f32 up[3];
    f32 tmp[3];
    u8 flag;
    u8* pos;
    u8* node;

    if (a == NULL) return;
    flag = 0;
    hit[2] = lbl_805FE308;
    hit[1] = lbl_805FE308;
    hit[0] = lbl_805FE308;
    push[2] = lbl_805FE308;
    push[1] = lbl_805FE308;
    push[0] = lbl_805FE308;
    move[0] = *(f32*)(p + 0x264) * dt;
    move[1] = *(f32*)(p + 0x268) * dt;
    move[2] = *(f32*)(p + 0x26C) * dt;
    if (*(s32*)(p + 0x290) == 3) {
        u8* n1 = *(u8**)(p + 0x8C);
        node = NULL;
        if (n1 != NULL && *(u32*)(n1 + 0x10) == 1) {
            u8* n2 = **(u8***)(n1 + 0xC);
            if (n2 != NULL && *(u32*)(n2 + 0x28) == 1) {
                node = **(u8***)(n2 + 0x24);
                if (*(s32*)(node + 0xC) == 0) {
                    *(f32*)(node + 0x20) = *(f32*)(node + 0x14);
                    *(f32*)(node + 0x24) = *(f32*)(node + 0x18);
                    *(f32*)(node + 0x28) = *(f32*)(node + 0x1C);
                    u8* cur = *(u8**)(p + 0x4);
                    *(f32*)(node + 0x14) = *(f32*)(cur + 0);
                    *(f32*)(node + 0x18) = *(f32*)(cur + 4);
                    *(f32*)(node + 0x1C) = *(f32*)(cur + 8);
                    if (*(s32*)(p + 0x290) != 5) {
                        *(f32*)(node + 0x24) = lbl_805FE308;
                        *(f32*)(node + 0x18) = lbl_805FE308;
                    }
                } else {
                    node = NULL;
                }
            }
        }
        // As in the original, a missing node is still written through below.
        pos = *(u8**)(p + 0x4);
        *(f32*)(node + 0x20) = *(f32*)(pos + 0);
        *(f32*)(node + 0x24) = *(f32*)(pos + 4);
        *(f32*)(node + 0x28) = *(f32*)(pos + 8);
        pos = *(u8**)(p + 0x4);
        f32 nx = *(f32*)(pos + 0) + move[0];
        f32 ny = *(f32*)(pos + 4) + move[1];
        f32 nz = *(f32*)(pos + 8) + move[2];
        *(f32*)(node + 0x14) = nx;
        *(f32*)(node + 0x18) = ny;
        *(f32*)(node + 0x1C) = nz;
        *out = (u8)fn_800C0168(a, node, move, hit, push, &flag);
    }
    if (*out != 0) {
        f32 lenMove = SqrtGuard(move[2] * move[2] + (move[0] * move[0] + move[1] * move[1]));
        pos = *(u8**)(p + 0x4);
        f32 ex = hit[0] - *(f32*)(pos + 0);
        f32 ey = hit[1] - *(f32*)(pos + 4);
        f32 ez = hit[2] - *(f32*)(pos + 8);
        f32 lenHit = SqrtGuard(ez * ez + (ex * ex + ey * ey));
        *(s32*)(p + 0x290) = 4;
        f32 f31 = lenMove - lenHit;
        f32 vx = push[0] / dt;
        f32 vy = push[1] / dt;
        f32 vz = push[2] / dt;
        *(f32*)(p + 0x264) = vx;
        *(f32*)(p + 0x268) = vy;
        *(f32*)(p + 0x26C) = vz;
        base[0] = hit[0];
        base[1] = hit[1];
        base[2] = hit[2];
        f32 vlen2 = vz * vz + (vx * vx + vy * vy);
        if (vlen2 == lbl_805FE308) {
            dir[2] = lbl_805FE308;
            dir[1] = lbl_805FE308;
            dir[0] = lbl_805FE308;
        } else {
            f32 inv = 1.0f / sqrtf(vlen2);
            dir[0] = vx * inv;
            dir[1] = *(f32*)(p + 0x268) * inv;
            dir[2] = *(f32*)(p + 0x26C) * inv;
        }
        f32 nx = base[0] + dir[0] * f31;
        f32 ny = base[1] + dir[1] * f31;
        f32 nz = base[2] + dir[2] * f31;
        pos = *(u8**)(p + 0x4);
        *(f32*)(pos + 0) = nx;
        *(f32*)(pos + 4) = ny;
        *(f32*)(pos + 8) = nz;
        MarkMoved(*(u8**)(p + 0x4));
        quat[2] = lbl_805FE308;
        quat[1] = lbl_805FE308;
        quat[0] = lbl_805FE308;
        quat[3] = lbl_805FE320;
        up[2] = lbl_805FE308;
        up[0] = lbl_805FE308;
        up[1] = lbl_805FE320;
        fn_801458B0(quat, dir, up);
        pos = *(u8**)(p + 0x4);
        *(f32*)(pos + 0xC) = quat[0];
        *(f32*)(pos + 0x10) = quat[1];
        *(f32*)(pos + 0x14) = quat[2];
        *(f32*)(pos + 0x18) = quat[3];
        MarkMoved(*(u8**)(p + 0x4));
        EventSend(gEventMgr, p, NULL, 0x96C877C4u, 0, 0, 1, 0);
        if (flag != 0) {
            f32 c = lbl_805FE328;
            tmp[0] = hit[0];
            tmp[1] = hit[1];
            tmp[2] = hit[2];
            fn_800BF370(p, tmp, c, f31);
            tmp[0] = hit[0];
            tmp[1] = hit[1];
            tmp[2] = hit[2];
            fn_800BF370(p, tmp, -c, f31);
        }
    } else {
        pos = *(u8**)(p + 0x4);
        *(f32*)(pos + 0) = *(f32*)(pos + 0) + move[0];
        *(f32*)(pos + 4) = *(f32*)(pos + 4) + move[1];
        *(f32*)(pos + 8) = *(f32*)(pos + 8) + move[2];
        MarkMoved(*(u8**)(p + 0x4));
    }
    fn_801B6F34(p);
}

// 0x800C2C80
// Pops the next queued object from the ring buffer (head +0x6C, tail +0x70, capacity +0x74, slots
// at +0x78), makes it current, then ages the active list (count +0x5C, slots +0xC): objects whose
// timer (+0x264) drops below zero are released, and the survivors are compacted to the front.
void fn_800C2C80(u8* p) {
    s32 head = *(s32*)(p + 0x6C);
    s32 empty = 1;
    u8* item;
    s32 i;

    if (head >= 0 && *(s32*)(p + 0x70) >= 0) empty = 0;
    if (empty == 0) {
        s32 tail = *(s32*)(p + 0x70);
        s32 idx = tail;
        s32 valid;
        s32 cap;
        if (tail < 0 || !(tail < (cap = *(s32*)(p + 0x74)))) {
            valid = 0;
        } else {
            s32 hi = tail;
            if (tail < head) {
                if (tail <= tail) idx = tail + cap;
                hi = tail + cap;
            }
            valid = 0;
            if (!(head > idx) && !(idx > hi)) valid = 1;
        }
        u8** slot = NULL;
        if (valid != 0) slot = (u8**)(*(u8**)(p + 0x78) + tail * 4);
        *(u8***)(p + 0x7C) = slot;
        if (slot != NULL) {
            item = *slot;
            *slot = NULL;
            s32 t = *(s32*)(p + 0x70);
            if (t == *(s32*)(p + 0x6C)) {
                *(s32*)(p + 0x70) = -1;
                *(s32*)(p + 0x6C) = -1;
            } else {
                s32 n = t - 1;
                if (n < 0) n = *(s32*)(p + 0x74) - 1;
                *(s32*)(p + 0x70) = n;
            }
        } else {
            item = NULL;
        }
    } else {
        item = NULL;
    }

    if (item != NULL) {
        (*(VFn0*)(*(u8**)item + 0xCC))(item);
        *(u8**)p = item;
        (*(VFn1*)(*(u8**)(p + 0xD4) + 0x38))(p, item);
        f32 age = (*(VFnF*)(*(u8**)item + 0xD0))(item);
        s32 released = 0;
        for (i = 0; i < *(s32*)(p + 0x5C); i++) {
            u8* obj = *(u8**)(p + 0xC + i * 4);
            f32 t = *(f32*)(obj + 0x264) - age;
            *(f32*)(obj + 0x264) = t;
            if (t < lbl_805FE390) {
                obj = *(u8**)(p + 0xC + i * 4);
                (*(VFn0*)(*(u8**)obj + 0xC0))(obj);
                u8* o2 = *(u8**)(p + 0xC + i * 4);
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(VFnInt*)(*(u8**)o2 + 0x18))(o2, 0);
                ScopedLockDtor(&lock, -1);
                released++;
            }
        }
        if (released > 0) {
            s32 count = *(s32*)(p + 0x5C);
            *(s32*)(p + 0x5C) = count - released;
            u8* dst = p;
            u8* src = p + released * 4;
            for (i = 0; i < *(s32*)(p + 0x5C); i++) {
                *(s32*)(dst + 0xC) = *(s32*)(src + 0xC);
                src += 4;
                dst += 4;
            }
        }
    }
}

}  // extern "C"

// Allocates a 0x18-byte path node, initialises it, copies the point into +0xC and queues it on
// the ring buffer at ctx+0x464.
static void QueuePathNode(u8* ctx, u8* rec) {
    u8* node = (u8*)fn_801D40E0(0x18);
    if (node != NULL) fn_800CAAC4(node);  // constructor; returns node
    Vec3Copy((Vec3*)(node + 0xC), (Vec3*)rec);
    fn_800CAAE0(ctx + 0x464, node);
}

extern "C" {

// 0x800CA290
// Builds the smoothed path node list for the character: collects the points around the path
// position (arg1) plus the character's own points (+0x2E0), orders them with the two sorters, then
// walks the result, cutting every segment whose length/offset limits fail, and queues a node for
// every kind-2 point and its kind-3 neighbours (+-2) on the ring buffer at +0x464.
void fn_800CA290(u8* arg0, u8* arg1) {
    Vec3 euler;
    f32 mat[12];
    f32 scaleSp;
    f32 divSp;
    u8 delta[0x14];
    u8 halved[0x14];
    u8 seg[0x14];
    f32 minLen = lbl_805FE4F0;

    fn_80052638();
    fn_800C1798(mat, arg0);
    fn_801449A4(mat, &euler.x);
    s32 nExtra = *(s32*)(arg0 + 0x394);
    s32 yaw = (s32)Vec3GetY(&euler);
    s32 nBase = *(s32*)(fn_800C9244(arg0) + 0x24);
    s32 total = nBase + nExtra * 6 + 0xA;
    s32 pathCount = fn_800C98A8(arg1);
    s32 lastIdx = 0;

    if (nExtra == 0) return;

    Vec3* tmp = (Vec3*)fn_8032F028(MemAllocSmall(nBase * 0xC + 0x10), NULL, NULL, 0xC, nBase);
    s32 baseM1 = nBase - 1;
    u8* dst = (u8*)tmp;
    s32 k = 0;
    s32 src = pathCount - baseM1;
    for (; k < baseM1; k++) {
        lastIdx = src;
        Vec3Copy((Vec3*)dst, (Vec3*)fn_8010C608(arg1, src));
        src++;
        dst += 0xC;
    }
    Vec3* tail = (Vec3*)((u8*)tmp + k * 0xC);
    Vec3Copy(tail, (Vec3*)(fn_8010C608(arg1, lastIdx) + 0xC));
    Vec3SetY(tail, lbl_805FE4B8);
    ScalarSet(&scaleSp, *(f32*)(fn_8010C608(arg1, lastIdx) + 0x18));
    Vec3ScaleBy(tail, &scaleSp);
    Vec3AddTo(tail, (Vec3*)fn_8010C608(arg1, lastIdx));
    Vec3SetY(tail, lbl_805FE4B8);

    s32 kept = 0;
    u8** arr = (u8**)MemAllocSmall(total * sizeof(u8*));
    s32 limit = nBase + nExtra;
    s32 baseOff = 0;
    s32 extraOff = 0;
    for (s32 n = 0; n < total; n++) {
        u8* rec = (u8*)fn_801D40E0(0x14);
        if (rec != NULL) fn_800CA8F0(rec);  // constructor; returns rec
        arr[n] = rec;
        if (n < limit) {
            if (n < nBase) {
                fn_800CA900(arr[n], (u8*)tmp + baseOff);
                baseOff += 0xC;
                kept++;
                *(s32*)(arr[n] + 0xC) = 1;
            } else {
                fn_800CA900(arr[n], arg0 + extraOff + 0x2E0);
                extraOff += 0xC;
                kept++;
                *(s32*)(arr[n] + 0xC) = 2;
            }
        } else {
            fn_800CA91C(arr[n], lbl_805FE4F4);
            fn_800CA924(arr[n], lbl_805FE4F4);
        }
        fn_800CA92C(arr[n], lbl_805FE4B8);
    }
    if (tmp != NULL) MemFreeSmall((u8*)tmp - 0x10);

    if (fn_800CA934(yaw) == 0) {
        fn_800C9CF0(arr, kept);
        if (fn_800CD59C(arg0) != 0) fn_800C9FC0(arr, kept >> 1);
    } else if (fn_800CA934(yaw) == 1) {
        fn_800C9FC0(arr, kept);
        if (fn_800CD59C(arg0) != 0) fn_800C9CF0(arr, kept >> 1);
    }

    f32 maxLen = lbl_805FE4FC;
    s32 pass = 0;
    while (kept < total) {
        u8** from = arr;
        u8** to = arr + kept;
        s32 moved = 0;
        s32 tries = 0;
        while (moved < kept - 1) {
            if (kept + moved >= total) break;
            if (tries >= total) break;
            if (pass <= 0 || *(s32*)(from[0] + 0xC) == 3 || *(s32*)(from[1] + 0xC) == 3) {
                fn_800CA938(delta, from[0], from[1]);
                ScalarSet(&divSp, lbl_805FE4F8);
                fn_800CA9B0(halved, delta, (u8*)&divSp);
                fn_800CA98C(delta, halved);
                fn_800CAA70(seg, from[0], delta);
                if (!(fn_800CAA00(seg) < minLen) && !(fn_800C9FB8(delta) > maxLen) &&
                    !(fn_800CA288(delta) > maxLen)) {
                    fn_800CA98C(to[0], delta);
                    from++;
                    to++;
                    moved++;
                }
            }
            tries++;
        }
        kept += moved;
        pass++;
        if (fn_800CA934(yaw) == 0) {
            fn_800C9CF0(arr, kept);
            if (fn_800CD59C(arg0) != 0) fn_800C9FC0(arr, kept >> 1);
        } else {
            fn_800C9FC0(arr, kept);
            if (fn_800CD59C(arg0) != 0) fn_800C9CF0(arr, kept >> 1);
        }
        if (moved == 0) break;
    }

    for (s32 n = 0; n < total; n++) {
        if (*(s32*)(arr[n] + 0xC) == 2) {
            if (n - 2 >= 0 && *(s32*)(arr[n - 2] + 0xC) == 3) {
                *(s32*)(arr[n - 2] + 0xC) = 4;
                QueuePathNode(arg0, arr[n - 2]);
            }
            if (n - 1 >= 0 && *(s32*)(arr[n - 1] + 0xC) == 3) {
                *(s32*)(arr[n - 1] + 0xC) = 4;
                QueuePathNode(arg0, arr[n - 1]);
            }
            *(s32*)(arr[n] + 0xC) = 4;
            QueuePathNode(arg0, arr[n]);
            if (n + 1 < total && *(s32*)(arr[n + 1] + 0xC) == 3) {
                *(s32*)(arr[n + 1] + 0xC) = 4;
                QueuePathNode(arg0, arr[n + 1]);
            }
            if (n + 2 < total && *(s32*)(arr[n + 2] + 0xC) == 3) {
                *(s32*)(arr[n + 2] + 0xC) = 4;
                QueuePathNode(arg0, arr[n + 2]);
            }
        }
    }
    for (s32 n = 0; n < total; n++) operator_delete(arr[n]);
    operator_delete(arr);
}

// 0x800CABD0
// Sorts the array of Vec3s (12 bytes each) by squared distance from the point at center:
// median-of-three quicksort with an explicit range stack down to partitions of five or fewer,
// then a minimum-first insertion sort.
void fn_800CABD0(u8* center, u8* arr, u32 count) {
    const f32* c = (const f32*)center;
    u8* stack[2 + 2 * 32];
    u8** sp = stack + 2;
    u8* lo = arr;
    u8* hi = arr + count * 0xC - 0xC;
    u8* r30;
    u8* r29;
    u8* mid;
    u32 i;

    for (i = 0; i < 2 + 2 * 32; i++) stack[i] = NULL;

    if (count > 4) {
        while (stack < sp) {
            mid = lo + (((hi - lo) / 12) / 2) * 12;
            if (DistSq(c, (f32*)mid) < DistSq(c, (f32*)lo)) SwapVec3(lo, mid);
            if (DistSq(c, (f32*)hi) < DistSq(c, (f32*)mid)) {
                SwapVec3(mid, hi);
                if (DistSq(c, (f32*)mid) < DistSq(c, (f32*)lo)) SwapVec3(lo, mid);
            }
            r30 = lo + 0xC;
            r29 = hi - 0xC;
            for (;;) {
                while (DistSq(c, (f32*)r30) < DistSq(c, (f32*)mid)) r30 += 0xC;
                while (DistSq(c, (f32*)mid) < DistSq(c, (f32*)r29)) r29 -= 0xC;
                if (r30 < r29) {
                    SwapVec3(r30, r29);
                    if (mid == r30) {
                        mid = r29;
                    } else if (mid == r29) {
                        mid = r30;
                    }
                    r30 += 0xC;
                    r29 -= 0xC;
                } else if (r30 == r29) {
                    r30 += 0xC;
                    r29 -= 0xC;
                    break;
                }
                if (!(r30 <= r29)) break;
            }
            s32 left = (s32)((r29 - lo) / 12);
            if (left <= 4) {
                if ((s32)((hi - r30) / 12) <= 4) {
                    lo = sp[-1];
                    hi = sp[-2];
                    sp -= 2;
                } else {
                    lo = r30;
                }
            } else {
                s32 right = (s32)((hi - r30) / 12);
                if (right <= 4) {
                    hi = r29;
                } else if (left > right) {
                    sp[1] = lo;
                    lo = r30;
                    sp[0] = r29;
                    sp += 2;
                } else {
                    sp[1] = r30;
                    sp[0] = hi;
                    hi = r29;
                    sp += 2;
                }
            }
        }
    }

    // Insertion sort: put the minimum of the first five at the front, then insert each element.
    u8* last = arr + count * 0xC - 0xC;
    u8* scanEnd = arr + 0x30;
    u8* minP = arr;
    if (scanEnd > last) scanEnd = last;
    u8* scan = arr + 0xC;
    if (!(scan > scanEnd)) {
        u32 n = (u32)((scanEnd + 0xC) - scan) / 12u;
        do {
            if (DistSq(c, (f32*)scan) < DistSq(c, (f32*)minP)) minP = scan;
            scan += 0xC;
        } while (--n != 0);
    }
    if (minP != arr) SwapVec3(minP, arr);

    u8* cur = arr + 0xC;
    cur += 0xC;
    while (cur <= last) {
        u8* q = cur - 0xC;
        while (DistSq(c, (f32*)cur) < DistSq(c, (f32*)q)) q -= 0xC;
        u8* insertAt = q + 0xC;
        if (insertAt != cur) {
            u8* r = cur + 0xC;
            for (;;) {
                r -= 0xC;
                if (!(r >= cur)) break;
                f32 t0 = *(f32*)(r + 0);
                f32 t1 = *(f32*)(r + 4);
                f32 t2 = *(f32*)(r + 8);
                u8* from = r;
                u8* to = r;
                for (;;) {
                    from -= 0xC;
                    if (!(from >= insertAt)) break;
                    *(f32*)(to + 0) = *(f32*)(from + 0);
                    *(f32*)(to + 4) = *(f32*)(from + 4);
                    *(f32*)(to + 8) = *(f32*)(from + 8);
                    to = from;
                }
                *(f32*)(to + 0) = t0;
                *(f32*)(to + 4) = t1;
                *(f32*)(to + 8) = t2;
            }
        }
        cur += 0xC;
    }
}

// 0x800CE7E8
// Resets the throw state: sends the two script events, clears the clutter state, picks the random
// spin (+0x94) and launch (+0x98) values from the per-scene tables, positions the camera target and
// stores the resulting position and orientation in the entry returned by fn_800CF71C.
void fn_800CE7E8(u8* p) {
    f32 pos[3];
    f32 off[3];
    f32 quat[4];
    f32 axis[3];

    EventSendTo(gEventMgr, kHashStaplingGunA, NULL, 0x9B51AA64u, 0, 0);
    fn_800CDC44((s32*)(p + 0xA8));
    fn_800CCA88(*(u8**)(p + 0xA4));
    (*(VFn0*)(*(u8**)(p + 0x30F4) + 0x10))(p + 0x3020);
    (*(VFn0*)(*(u8**)(p + 0x3008) + 0x10))(p + 0x2FFC);
    fn_800C7620(p + 0xF4);
    fn_800CF71C();
    fn_800D01C0();
    EventSendTo(gEventMgr, 0xD9D1E8B2u, NULL, 0x9B51AA64u, 0, 0);
    fn_8006FC04();
    fn_800CF7E8(p);
    *(f32*)(p + 0xA0) = lbl_805FE558;
    *(f32*)(p + 0x9C) = lbl_805FE558;
    // RaySceneTest ignores its argument (returns a global scene index).
    if (lbl_803472A0[RaySceneTest(NULL)] != 0) {
        *(u8*)(p + 0x90) = 1;
        f32 hi = lbl_803472C0[RaySceneTest(NULL)];
        f32 lo = lbl_803472B0[RaySceneTest(NULL)];
        *(f32*)(p + 0x94) = lbl_805FE578 * (lo + (hi - lo) * (lbl_805FE574 * (f32)RandomU32()));
        if (RandomU32() & 1) *(f32*)(p + 0x94) = *(f32*)(p + 0x94) * lbl_805FE57C;
        f32 hi2 = lbl_803472E0[RaySceneTest(NULL)];
        f32 lo2 = lbl_803472D0[RaySceneTest(NULL)];
        *(f32*)(p + 0x98) = lo2 + (hi2 - lo2) * (lbl_805FE574 * (f32)RandomU32());
    } else {
        *(u8*)(p + 0x90) = 0;
        *(f32*)(p + 0x98) = lbl_805FE580;
    }
    (*(VFnF1*)(*(u8**)(p + 0x30F4) + 0x1C))(p + 0x3020, lbl_805FE558 * lbl_80347290[RaySceneTest(NULL)]);
    pos[0] = lbl_805FE558;
    pos[1] = lbl_805FE594;
    pos[2] = lbl_805FE558;
    // f1 still holds the zero constant at this call; it may be a leftover rather than an argument.
    (*(VFnF1*)(*(u8**)(p + 0x30F4) + 0x20))(p + 0x3020, 0.0f);
    (*(VFnOut*)(*(u8**)(p + 0x30F4) + 0x24))(off, p + 0x3020);
    pos[0] = pos[0] + off[0];
    pos[1] = pos[1] + off[1];
    pos[2] = pos[2] + off[2];
    u8* camObj = *(u8**)(lbl_803C4298 + 4);
    (*(VFn2*)(*(u8**)camObj + 0x2C))(camObj, (u8*)pos, (u8*)off);
    camObj = *(u8**)(lbl_803C4298 + 4);
    (*(VFn1*)(*(u8**)camObj + 0x1C))(camObj, (u8*)pos);
    u8* other = *(u8**)(lbl_803C4298 + 8);
    (*(VFnF1*)(*(u8**)other + 0x98))(other, lbl_805FE558);
    fn_800CFDCC(p + 0x90, lbl_805FE558);
    u8* entry = (u8*)fn_800CF71C();
    quat[2] = lbl_805FE558;
    quat[1] = lbl_805FE558;
    quat[0] = lbl_805FE558;
    quat[3] = lbl_805FE598;
    axis[2] = lbl_805FE558;
    axis[0] = lbl_805FE558;
    axis[1] = lbl_805FE598;
    fn_80146180(quat, (const Vec3*)axis, *(f32*)(p + 0xA0));
    *(f32*)(entry + 0x58) = pos[0];
    *(f32*)(entry + 0x5C) = pos[1];
    *(f32*)(entry + 0x60) = pos[2];
    *(f32*)(entry + 0x64) = quat[0];
    *(f32*)(entry + 0x68) = quat[1];
    *(f32*)(entry + 0x6C) = quat[2];
    *(f32*)(entry + 0x70) = quat[3];
}

}  // extern "C"
