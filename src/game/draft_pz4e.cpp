// Missing functions batch pz4e: animation track blend, ray vs sphere / box tests, interpolation
// task setup, compound-task creation, audio fade finish, sound-name dispatch, ...
#include <string.h>

#include "types.h"
#include "engine/math.h"

static const f32 kTwoOverPi = 0.63661975f;  // 0x80600048

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtGuard(f32 v) {
    return v <= 0.0f ? 0.0f : sqrtf(v);
}

extern "C" {

void  EulerToQuat(Quat* out, const Vec3* euler);                                  // 0x80145B00
void  SleepMicroseconds(int unused, u32 us);                                      // 0x80142818
u32   StringHash(const char* s);
void  fn_80142ED0(u8* base, s32 size);                                            // 0x80142ED0
u32   fn_80143A5C(const char* s);                                                 // 0x80143A5C
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);                               // 0x80145F9C
void  fn_8016A3F8(u8* p);                                                         // 0x8016A3F8
void  fn_8016A43C(u8* p);                                                         // 0x8016A43C
void  fn_8016A788(u8* p);                                                         // 0x8016A788
void  fn_80173944(u8* obj);                                                       // 0x80173944
s32 fn_801B8474(u8* arg0);                                           // 0x801B8474 (task update callback; signature not checked)
void  fn_801B72A0(u8* p, u8* target);                                             // 0x801B72A0
s32   fn_801C1960(f32* tmin, f32* tmax, f32 dir, f32 dist);                       // 0x801C1960
u8*   fn_801C6BCC(u8* key);                                                       // 0x801C6BCC
s32   fn_800E2F10(u8* p, f32 dt);                                                 // 0x800E2F10
BOOL  fn_801E7460(u8* base, u16* id);                                             // 0x801E7460
void  fn_801E86FC(u8* out, u8* slot, u8* obj, u8* anim);                          // 0x801E86FC
void  fn_801E8D74(void* out, void* list, void* owner, void* anim);                // 0x801E8D74
s32   fn_801E9678(void* list, void* handle);                                      // 0x801E9678
void* fn_801E9D18(void* p, u32 hash, s32 size);                                   // 0x801E9D18
u8* fn_801E9E50(u8* task);                                                      // 0x801E9E50
void  fn_801E9EE4(u8* task, u8** pts, s32 a, s32 b, s32 c, s32 d, s32 e, f32 t0, f32 t1);  // 0x801E9EE4
f32   fn_801EA350(const u8* task);                                                // 0x801EA350
u8* fn_801EA59C(u8* task);                                                      // 0x801EA59C
void  fn_801EA62C(u8* task, u8** pts, s32 a, s32 b, s32 c, f32 t0, f32 t1);       // 0x801EA62C
s32   InterpolateVector3Task_Update_LERP(u8* task, void* arg);                    // 0x801EA134
s32   InterpolateQuaternionTask_Update_SLERP(u8* task, void* arg);                // 0x801EA7F8
u8*   fn_801E6570(s32 a3, s32 a4, s32 a5);                                        // 0x801E6570
s32   fn_801E65A0(void);                                                          // 0x801E65A0
void  fn_801FF2A8(u8* audio, s32 handle);                                         // 0x801FF2A8
void  fn_801FF910(f32 v);                                                         // 0x801FF910
void  fn_801FFC58(f32 v);                                                         // 0x801FFC58
void  fn_80200920(void* p, s32 a, s32 idx);                                       // 0x80200920
void  fn_80200F34(void* p, s32 a, s32 idx);                                       // 0x80200F34
void  fn_80201190(void* p, s32 a, s32 idx);                                       // 0x80201190
void  fn_802044B8(u8* p, f32 v);                                                  // 0x802044B8
void  fn_80209810(void);                                                          // 0x80209810

extern u8   gIdPool[];                 // 0x8048A8F0
extern u8   lbl_80413400[];            // 0x80413400
extern s32  lbl_8051E5F0[];            // 0x8051E5F0 (s32 table)
extern u8   lbl_8051E6A8[];            // 0x8051E6A8 (sound voices, 0x228 each)
extern u8   lbl_80524E28[];            // 0x80524E28 (sound table entries, 0x58 each, 0x60 of them)
extern u32  lbl_805F969C;              // 0x805F969C (invalid-handle sentinel)
extern f32  lbl_805F98D4;              // 0x805F98D4
extern u8   lbl_805FB1B8;              // 0x805FB1B8 (CompoundTask hash cached)
extern s32  lbl_805FB6B8;              // 0x805FB6B8
extern u8   lbl_805FB7E8[];            // 0x805FB7E8
extern u8*  lbl_805FB8D8;              // 0x805FB8D8 (effect pool: 0x80-byte entries)
extern s32  lbl_805FB8EC;              // 0x805FB8EC
extern s32  lbl_805FBA88;              // 0x805FBA88
extern s32  lbl_805FBAE0;              // 0x805FBAE0 (audio busy flag)
extern s32  lbl_805FBAE4;              // 0x805FBAE4
extern f32  lbl_805FBAEC;              // 0x805FBAEC
extern f32  lbl_805FBAF0;              // 0x805FBAF0
extern f32  lbl_805FBAF4;              // 0x805FBAF4
extern s32  lbl_805FBAF8;              // 0x805FBAF8
extern u32  lbl_806010F0;              // 0x806010F0 (hash of "CompoundTask")

// Blends / copies a run of keyframe records. mode (dst+4) 0: 0x1C-byte records (position blended
// srcA*(1-t) + srcB*t, quaternion via fn_80145F9C); mode 1: 0xC-byte records copied from srcA.
// The count is limited by maxCount and by what is left in both sources; dst's used length (+0x10)
// is raised to dstStart + count.
// 0x8016ACB8
void fn_8016ACB8(u8* dst, s32 dstStart, u8* srcA, s32 aStart, u8* srcB, s32 bStart, s32 maxCount, f32 t) {
    s32 count = *(s32*)(srcA + 0x10) - aStart;
    if (maxCount < count) {
        count = maxCount;
    }
    s32 countB = *(s32*)(srcB + 0x10) - bStart;
    if (count < countB) {
        countB = count;
    }
    count = countB;
    s32 len = dstStart + count;
    if (*(s32*)(dst + 0x10) > len) {
        len = *(s32*)(dst + 0x10);
    }
    s32 mode = *(s32*)(dst + 0x4);
    *(s32*)(dst + 0x10) = len;
    if (mode == 0) {
        f32 wb = t;
        f32 wa = 1.0f - wb;
        u8* d = dst + *(s32*)(dst + 0x8) + dstStart * 0x1C;
        u8* a = srcA + *(s32*)(srcA + 0x8) + aStart * 0x1C;
        u8* b = srcB + *(s32*)(srcB + 0x8) + bStart * 0x1C;
        for (s32 i = 0; i < count; i++) {
            f32 q[4];
            *(f32*)(d + 0x0) = *(f32*)(a + 0x0) * wa + *(f32*)(b + 0x0) * wb;
            *(f32*)(d + 0x4) = *(f32*)(a + 0x4) * wa + *(f32*)(b + 0x4) * wb;
            *(f32*)(d + 0x8) = *(f32*)(a + 0x8) * wa + *(f32*)(b + 0x8) * wb;
            fn_80145F9C(q, (f32*)(a + 0xC), (f32*)(b + 0xC), t);
            a += 0x1C;
            b += 0x1C;
            *(f32*)(d + 0xC) = q[0];
            *(f32*)(d + 0x10) = q[1];
            *(f32*)(d + 0x14) = q[2];
            *(f32*)(d + 0x18) = q[3];
            d += 0x1C;
        }
    } else if (mode == 1) {
        u8* d = dst + *(s32*)(dst + 0x8) + dstStart * 0xC;
        u8* a = srcA + *(s32*)(srcA + 0x8) + aStart * 0xC;
        for (s32 i = 0; i < count; i++) {
            *(s32*)(d + 0x0) = *(s32*)(a + 0x0);
            *(s32*)(d + 0x4) = *(s32*)(a + 0x4);
            *(s32*)(d + 0x8) = *(s32*)(a + 0x8);
            a += 0xC;
            d += 0xC;
        }
    }
}

// Sound-name dispatch: maps a name hash to (voice group, index) and starts it with the matching
// per-group helper (fn_80200920 / fn_80200F34 / fn_80201190). Clears the pending flag for slot idx
// first (starting index 0 with fn_80200920 when it was set). Unknown hashes leave group 0 and do nothing.
// 0x802005EC
void fn_802005EC(u8* p, u32 hash, s32 idx) {
    static const struct {
        u32 hash;
        s32 sub;
        s32 group;
    } kNames[] = {
        {0xDE309136u, 1, 1},  {0x4739C08Cu, 2, 1},  {0x329117F6u, 3, 1},  {0x8019C3C1u, 4, 1},
        {0x88A768AFu, 5, 1},  {0xA5237C5Fu, 6, 1},  {0x6F0A7224u, 7, 1},  {0x225157C4u, 8, 1},
        {0xE1B7910Bu, 9, 1},  {0x061A8B80u, 10, 1}, {0x59E612ABu, 11, 1}, {0x78C25129u, 12, 1},
        {0x287B8FAAu, 1, 2},  {0xB172DE10u, 2, 2},  {0xAE087CDCu, 3, 2},  {0x0516C72Bu, 4, 2},
        {0x9C1F9691u, 5, 2},  {0x5A6F665Bu, 1, 3},  {0xC36637E1u, 2, 3},  {0xB4610777u, 3, 3},
        {0x24764266u, 4, 3},  {0xBD7F13DCu, 5, 3},  {0xD09175D0u, 6, 3},  {0x4998246Au, 7, 3},
        {0x5F2E4F5Bu, 8, 3},  {0xC6271EE1u, 9, 3},
    };
    s32 sub = 0;
    s32 group = 0;
    for (u32 i = 0; i < sizeof(kNames) / sizeof(kNames[0]); i++) {
        if (hash == kNames[i].hash) {
            sub = kNames[i].sub;
            group = kNames[i].group;
            break;
        }
    }
    if (lbl_8051E5F0[idx] != 0) {
        fn_80200920(p, 0, idx);
        lbl_8051E5F0[idx] = 0;
    }
    switch (group) {
    case 1:
        fn_80200920(p, sub, idx);
        break;
    case 2:
        fn_80200F34(p, sub, idx);
        break;
    case 3:
        fn_80201190(p, sub, idx);
        break;
    }
}

// Direction/slot picker on a table of candidate vectors (count +0x1E0, vectors at +0xA8 with 0xC
// stride, scores at +0x40C). Writes to out the vector with the highest score that is above -1 and
// below a limit. depth 0: the limit is 1e10 (no restriction). depth > 0: the limit is the score of
// the vector the previous depth-1 call returned (falling back to the depth 0 result when that was
// the zero vector, -1 if it is not in the table). The zero-vector normalisations the original
// computes from constant zeros are folded to 0.
// 0x801F916C
void fn_801F916C(f32* out, u8* obj, s32 depth) {
    f32 limit;
    if (depth == 0) {
        limit = 1e10f;
    } else {
        f32 v[3];
        fn_801F916C(v, obj, depth - 1);
        f32 w[3];
        w[0] = v[0];
        w[1] = v[1];
        w[2] = v[2];
        if (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f) {
            fn_801F916C(w, obj, 0);
        }
        limit = -1.0f;
        s32 n = *(s32*)(obj + 0x1E0);
        for (s32 i = 0; i < n; i++) {
            u8* e = obj + 0xA8 + i * 0xC;
            if (w[0] == *(f32*)(e + 0x0) && w[1] == *(f32*)(e + 0x4) && w[2] == *(f32*)(e + 0x8)) {
                limit = *(f32*)(obj + 0x40C + i * 4);
                break;
            }
        }
    }
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    f32 best = -1.0f;
    for (s32 i = 0; i < *(s32*)(obj + 0x1E0); i++) {
        f32 d = *(f32*)(obj + 0x40C + i * 4);
        if (!(d >= limit) && !(d <= best)) {
            u8* e = obj + 0xA8 + i * 0xC;
            best = d;
            out[0] = *(f32*)(e + 0x0);
            out[1] = *(f32*)(e + 0x4);
            out[2] = *(f32*)(e + 0x8);
        }
    }
}

// Ray vs sphere: sphere centre a, ray origin b, direction c, radius r. *n = number of hits
// (0, 1 or 2) with ray parameters in dist[] (if non-null) and hit points in pos[] (if non-null,
// 3 floats each). Only hits with t >= 0 count. Returns 1 if any.
// 0x801C3170
s32 fn_801C3170(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist) {
    f32 r2 = r * r;
    f32 dx = *(f32*)(b + 0x0) - *(f32*)(a + 0x0);
    f32 dy = *(f32*)(b + 0x4) - *(f32*)(a + 0x4);
    f32 dz = *(f32*)(b + 0x8) - *(f32*)(a + 0x8);
    f32 cx = *(f32*)(c + 0x0);
    f32 cy = *(f32*)(c + 0x4);
    f32 cz = *(f32*)(c + 0x8);
    f32 qa = cz * cz + (cx * cx + cy * cy);
    f32 qb = dz * cz + (dx * cx + dy * cy);
    f32 qc = (dz * dz + (dx * dx + dy * dy)) - r2;
    f32 disc = qb * qb - qa * qc;
    if (disc < 0.0f) {
        *n = 0;
        return 0;
    }
    if (disc > 0.0f) {
        f32 root = sqrtf(disc);  // original: frsqrte with two Newton steps
        f32 inv = 1.0f / qa;
        f32 nb = -qb;
        f32 t1 = (nb - root) * inv;
        f32 t2 = (nb + root) * inv;
        if (t1 >= 0.0f) {
            *n = 2;
            if (dist) {
                dist[0] = t1;
                dist[1] = t2;
            }
            if (pos) {
                pos[0] = *(f32*)(b + 0x0) + cx * t1;
                pos[1] = *(f32*)(b + 0x4) + cy * t1;
                pos[2] = *(f32*)(b + 0x8) + cz * t1;
                pos[3] = *(f32*)(b + 0x0) + cx * t2;
                pos[4] = *(f32*)(b + 0x4) + cy * t2;
                pos[5] = *(f32*)(b + 0x8) + cz * t2;
            }
            return 1;
        }
        if (t2 >= 0.0f) {
            *n = 1;
            if (dist) {
                dist[0] = t2;
            }
            if (pos) {
                pos[0] = *(f32*)(b + 0x0) + cx * t2;
                pos[1] = *(f32*)(b + 0x4) + cy * t2;
                pos[2] = *(f32*)(b + 0x8) + cz * t2;
            }
            return 1;
        }
        *n = 0;
        return 0;
    }
    f32 t = -qb / qa;
    if (t >= 0.0f) {
        *n = 1;
        if (dist) {
            dist[0] = t;
        }
        if (pos) {
            pos[0] = *(f32*)(b + 0x0) + cx * t;
            pos[1] = *(f32*)(b + 0x4) + cy * t;
            pos[2] = *(f32*)(b + 0x8) + cz * t;
        }
        return 1;
    }
    *n = 0;
    return 0;
}

// Ray vs axis-aligned box: box centre a with half extents e, ray origin b, direction c. Clips the
// parameter range [0, FLT_MAX] with the six slabs (fn_801C1960). *n = 2 (entry and exit, entry
// > 0), 1 (origin inside: only the exit) or 0; dist[] gets the parameters and pos[] the points.
// Returns 1 on a hit.
// 0x801C1E04
s32 fn_801C1E04(u8* a, u8* e, u8* b, u8* c, s32* n, f32* pos, f32* dist) {
    f32 tmin = 0.0f;
    f32 tmax = 3.4028235e38f;
    f32 dx = *(f32*)(b + 0x0) - *(f32*)(a + 0x0);
    f32 dy = *(f32*)(b + 0x4) - *(f32*)(a + 0x4);
    f32 dz = *(f32*)(b + 0x8) - *(f32*)(a + 0x8);
    f32 cx = *(f32*)(c + 0x0);
    f32 cy = *(f32*)(c + 0x4);
    f32 cz = *(f32*)(c + 0x8);
    s32 ok = 0;
    if (fn_801C1960(&tmin, &tmax, cx, -dx - *(f32*)(e + 0x0)) &&
        fn_801C1960(&tmin, &tmax, -cx, dx - *(f32*)(e + 0x0)) &&
        fn_801C1960(&tmin, &tmax, cy, -dy - *(f32*)(e + 0x4)) &&
        fn_801C1960(&tmin, &tmax, -cy, dy - *(f32*)(e + 0x4)) &&
        fn_801C1960(&tmin, &tmax, cz, -dz - *(f32*)(e + 0x8)) &&
        fn_801C1960(&tmin, &tmax, -cz, dz - *(f32*)(e + 0x8))) {
        ok = 1;
    }
    s32 hit = 0;
    if (ok && (tmin != 0.0f || tmax != 3.4028235e38f)) {
        hit = 1;
    }
    if (!hit) {
        *n = 0;
        return hit;
    }
    if (tmin > 0.0f) {
        *n = 2;
        pos[0] = *(f32*)(b + 0x0) + cx * tmin;
        pos[1] = *(f32*)(b + 0x4) + cy * tmin;
        pos[2] = *(f32*)(b + 0x8) + cz * tmin;
        pos[3] = *(f32*)(b + 0x0) + cx * tmax;
        pos[4] = *(f32*)(b + 0x4) + cy * tmax;
        pos[5] = *(f32*)(b + 0x8) + cz * tmax;
        dist[0] = tmin;
        dist[1] = tmax;
    } else {
        *n = 1;
        pos[0] = *(f32*)(b + 0x0) + cx * tmax;
        pos[1] = *(f32*)(b + 0x4) + cy * tmax;
        pos[2] = *(f32*)(b + 0x8) + cz * tmax;
        dist[0] = tmax;
    }
    return hit;
}

// Looks up the keyframe/waypoint record for the 4-byte key at p (two bytes + a halfword).
static u8* LookupKey(const u8* p) {
    u8 key[4];
    key[0] = p[0];
    key[1] = p[1];
    *(u16*)(key + 2) = *(const u16*)(p + 2);
    return fn_801C6BCC(key);
}

// Sets up the vector (and, if +0x2C is not -1, rotation) interpolation tasks of a waypoint-path
// follower: looks up the 4 spline points (keys at +4, +8, +0xC, ... of obj, the first/last point
// repeated at the ends), starts a position task with fn_801E9EE4 and a quaternion task with
// fn_801EA62C, giving both the elapsed time t, and stores the task handles at +0x24 and +0x34.
// Returns 0 if either start point is missing or both keys are equal, else 1.
// 0x801EAFA4
s32 fn_801EAFA4(u8* obj, f32 t) {
    u8 A[0x80];
    u8 B[0x80];
    u8* pts[4];
    u8* qpts[4];
    Quat q[4];
    u8 res[4];

    memset(A, 0, 0x64);
    *(s32*)(A + 0x64) = 0;
    *(s32*)(A + 0x68) = 0;
    *(void**)(A + 0x6C) = (void*)InterpolateVector3Task_Update_LERP;
    *(f32*)(A + 0x70) = 0.0f;
    *(u16*)(A + 0x78) = 0xFFFF;
    *(u16*)(A + 0x7A) = 0xFFFF;
    *(u16*)(A + 0x7C) = 0xFFFF;
    *(s32*)(A + 0x74) = 0;
    A[0x7E] = 0;
    A[0x7F] = 0;
    *(s32*)(A + 0x3C) = 0;

    u32 flag = (*(u32*)(obj + 0x38) >> 1) & 1;
    u8* cur = LookupKey(obj + 4);
    u8* next = LookupKey(obj + 8);
    if (cur == NULL || next == NULL) {
        return 0;
    }
    if (*(u16*)(obj + 0x46) == *(u16*)(obj + 6) && obj[0x45] == obj[5]) {
        return 0;
    }

    u8* keyp = obj + 4;
    for (u32 i = 0; i < 4; i++) {
        pts[i] = cur + 0x18;
        if (*(s32*)(obj + 0x2C) != -1) {
            Vec3 e;
            e.x = *(f32*)(cur + 0x24) * kTwoOverPi;
            e.y = *(f32*)(cur + 0x28) * kTwoOverPi;
            e.z = *(f32*)(cur + 0x2C) * kTwoOverPi;
            EulerToQuat(&q[i], &e);
            qpts[i] = (u8*)&q[i];
        }
        if (next != NULL) {
            cur = next;
            next = LookupKey(keyp + 4);
            keyp += 4;
        }
    }
    fn_801E9EE4(A, pts, *(s32*)(obj + 0x18), lbl_805FB8EC, (s32)flag, 1, *(s32*)(obj + 0x20),
                *(f32*)(obj + 0x14), 0.0f);
    *(f32*)(A + 0x70) = t;
    fn_801E86FC(res, *(u8**)(obj + 0x3C), obj, A);
    *(u16*)(obj + 0x24) = *(u16*)res;
    if (*(s32*)(obj + 0x2C) != -1) {
        memset(B, 0, 0x64);
        *(s32*)(B + 0x64) = 0;
        *(s32*)(B + 0x68) = 0;
        *(void**)(B + 0x6C) = (void*)InterpolateQuaternionTask_Update_SLERP;
        *(f32*)(B + 0x70) = 0.0f;
        *(u16*)(B + 0x78) = 0xFFFF;
        *(u16*)(B + 0x7A) = 0xFFFF;
        *(u16*)(B + 0x7C) = 0xFFFF;
        *(s32*)(B + 0x74) = 0;
        B[0x7E] = 0;
        B[0x7F] = 0;
        // First float of the position task (duration set by fn_801E9EE4) decides the speed scale.
        f32 dur = *(f32*)A;
        f32 scale;
        if (dur > -0.0001f && dur < 0.0001f) {
            scale = 0.0f;
        } else {
            scale = 1.0f / dur;
        }
        fn_801EA62C(B, qpts, *(s32*)(obj + 0x28), *(s32*)(obj + 0x2C), *(s32*)(obj + 0x30), scale, 0.0f);
        *(f32*)(B + 0x70) = t;
        fn_801E86FC(res, *(u8**)(obj + 0x3C), *(u8**)(obj + 0x40), B);
        *(u16*)(obj + 0x34) = *(u16*)res;
    }
    return 1;
}

// Finds a table entry (in obj+0x7B4: entries of 0x18 bytes) whose key is `key` and whose hash
// (+0xC) equals the hash of `name`, allocates a handle from the gIdPool slots (0x88 bytes each),
// fills it from the entry (state 1, or state 2 with the two callbacks fn_801E6570/fn_801E65A0 when
// the entry has a +0x10 value) and returns the handle (lbl_805F969C if no slot was free).
// As in the original, an entry that is not found is dereferenced (never happens in practice).
// 0x801E65B8
u32 fn_801E65B8(u8* obj, const char* name, s32 user, s32 unused, s32 key, s32 val) {
    u8* tbl = *(u8**)(obj + 0x7B4);
    char buf[0x38];
    memset(buf, 0, sizeof(buf));
    u32 i = 0;
    while (*name != 0 && i < 0x38) {
        buf[i] = *name;
        i++;
        name++;
    }
    buf[0x37] = 0;
    u32 hash = fn_80143A5C(buf);

    u8* ent = NULL;
    s32 idx = 0;
    u32 count = *(u32*)(tbl + 4);
    for (u32 k = 0; k < count; k++) {
        u8* e = *(u8**)tbl + k * 0x18;
        if ((u32)key == *(u32*)e && hash == *(u32*)(e + 0xC)) {
            u32 n2 = *(u32*)(tbl + 0x10);
            idx = (s32)(n2 + 1);
            for (u32 j = 0; j < n2; j++) {
                if (hash == *(u32*)(*(u8**)(tbl + 0xC) + j * 0x40)) {
                    idx = (s32)j;
                    break;
                }
            }
            ent = *(u8**)tbl + k * 0x18;
            break;
        }
    }

    // Inlined pool allocation from the ring in gIdPool (head +0x10, tail +0x14, capacity +0x18,
    // slots +0x1C, generation counter +0x28).
    u8* slot;
    s32 cap = *(s32*)(gIdPool + 0x18);
    if (*(s32*)(gIdPool + 0x10) == (*(s32*)(gIdPool + 0x14) + 1) % cap) {
        slot = NULL;
    } else {
        s32 head = *(s32*)(gIdPool + 0x10) - 1;
        if (head < 0) {
            head = cap - 1;
        }
        *(s32*)(gIdPool + 0x10) = head;
        if (*(s32*)(gIdPool + 0x14) < 0) {
            *(s32*)(gIdPool + 0x14) = head;
            *(s32*)(gIdPool + 0x8) = 0;
        }
        s32 h2 = *(s32*)(gIdPool + 0x10);
        s32 h3 = h2;
        s32 inRange;
        if (h2 < 0 || h2 >= *(s32*)(gIdPool + 0x18)) {
            inRange = 0;
        } else {
            s32 tail = *(s32*)(gIdPool + 0x14);
            if (tail < h2) {
                if (!(h2 > tail)) {
                    h3 = h2 + *(s32*)(gIdPool + 0x18);
                }
                tail += *(s32*)(gIdPool + 0x18);
            }
            inRange = 0;
            if (!(h2 > h3) && !(h3 > tail)) {
                inRange = 1;
            }
        }
        if (inRange) {
            slot = *(u8**)(gIdPool + 0x1C) + h2 * 0x88;
        } else {
            slot = NULL;
        }
    }
    u32 handle = lbl_805F969C;
    if (slot != NULL) {
        *(s32*)(slot + 0x4) = 1;
        *(s32*)(slot + 0x70) = 0;
        *(s32*)(slot + 0x8) = 0;
        *(u32*)(slot + 0x74) = *(u32*)(gIdPool + 0x28);
        *(u32*)(gIdPool + 0x28) = *(u32*)(gIdPool + 0x28) + 1;
        *(s32*)(slot + 0x84) = 0;
        if (*(u32*)(gIdPool + 0x28) >= 0x1FFFFFFu) {
            *(u32*)(gIdPool + 0x28) = 0;
        }
        handle = (*(u32*)(slot + 0x74) << 7) | (u32)((s32)(slot - *(u8**)(gIdPool + 0x1C)) / 0x88);
    }

    // Handle -> record in the owner's table (obj+4), checking the generation.
    u8* rec;
    if (handle == 0xFFFFFFFFu) {
        rec = NULL;
    } else {
        rec = *(u8**)(obj + 0x4) + (handle & 0x7F) * 0x88;
        if ((handle >> 7) != *(u32*)(rec + 0x74)) {
            rec = NULL;
        }
    }
    *(s32*)(rec + 0x54) = *(s32*)(*(u8**)(obj + 0x7A8) + idx * 4);
    *(s32*)(rec + 0x58) = key;
    *(s32*)(rec + 0x5C) = user;
    *(s32*)(rec + 0x60) = *(s32*)(ent + 0x8);
    if (*(s32*)(ent + 0x10) == 0) {
        *(s32*)(rec + 0x0) = 1;
        *(s32*)(rec + 0xC) = val;
        return handle;
    }
    *(s32*)(rec + 0x0) = 2;
    *(s32*)(rec + 0x44) = *(s32*)(ent + 0x10);
    *(s32*)(rec + 0xC) = val;
    *(s32*)(rec + 0x10) = *(s32*)(ent + 0x10);
    *(s32*)(rec + 0x14) = 0;
    *(s32*)(rec + 0x18) = user;
    *(s32*)(rec + 0x1C) = *(s32*)(ent + 0x8);
    *(s32*)(rec + 0x20) = 0;
    *(void**)(rec + 0x2C) = (void*)fn_801E6570;
    *(void**)(rec + 0x30) = (void*)fn_801E65A0;
    *(s32*)(rec + 0x48) = *(s32*)(ent + 0x14);
    return handle;
}

// Spawns the "CompoundTask" effect on obj: builds a position task (obj+4 position -> from, or the
// origin) in slot of the effect pool, then, with a nonzero extra quaternion, a second task from
// the object's quaternion (obj+4 +0xC) to extra. col is a 4-byte colour (b, b, u16), id a handle
// value, dur the duration.
// 0x801B8DDC
void fn_801B8DDC(void* objp, f32* from, u8* extra, s32 a3, u8* col, s32 id, f32 dur) {
    u8* obj = (u8*)objp;
    u8 T1[0x80];
    u8 T2[0x80];
    u8 T3[0x80];
    u8 T4[0x80];
    u8* pts[4];
    f32 endPos[3];
    f32 startPos[3];
    u8 res[4];
    u8 buf[4];

    memset(T1, 0, 0x64);
    *(s32*)(T1 + 0x64) = 0;
    *(s32*)(T1 + 0x68) = 0;
    *(void**)(T1 + 0x6C) = (void*)fn_800E2F10;
    *(f32*)(T1 + 0x70) = 0.0f;
    *(u16*)(T1 + 0x78) = 0xFFFF;
    *(u16*)(T1 + 0x7A) = 0xFFFF;
    *(u16*)(T1 + 0x7C) = 0xFFFF;
    *(s32*)(T1 + 0x74) = 0;
    T1[0x7E] = 0;
    T1[0x7F] = 0;
    if ((s8)lbl_805FB1B8 == 0) {
        lbl_806010F0 = StringHash("CompoundTask");
        lbl_805FB1B8 = 1;
    }
    *(u32*)(T1 + 0x64) = lbl_806010F0;

    fn_801E9E50(T2);
    u8* pose = *(u8**)(obj + 0x4);
    endPos[0] = *(f32*)(pose + 0x0);
    endPos[1] = *(f32*)(pose + 0x4);
    endPos[2] = *(f32*)(pose + 0x8);
    startPos[0] = 0.0f;
    startPos[1] = 0.0f;
    startPos[2] = 0.0f;
    if (from != NULL) {
        startPos[0] = from[0];
        startPos[1] = from[1];
        startPos[2] = from[2];
    }
    pts[0] = (u8*)endPos;
    pts[1] = (u8*)startPos;
    pts[2] = NULL;
    pts[3] = NULL;

    memset(T3, 0, 0x64);
    *(s32*)(T3 + 0x64) = 0;
    *(s32*)(T3 + 0x68) = 0;
    *(void**)(T3 + 0x6C) = (void*)fn_801B8474;
    *(f32*)(T3 + 0x70) = 0.0f;
    *(u16*)(T3 + 0x78) = 0xFFFF;
    *(u16*)(T3 + 0x7A) = 0xFFFF;
    *(u16*)(T3 + 0x7C) = 0xFFFF;
    *(s32*)(T3 + 0x74) = 0;
    T3[0x7E] = 0;
    T3[0x7F] = 0;

    u8* p = (u8*)fn_801E9D18(obj + 0xFC, 0x7EB9DC41u, 0xF8);
    if (p != NULL) {
        u16 v = *(u16*)(p + 0x7C);
        fn_801E9678(obj + 0xFC, &v);
    }
    *(u32*)(T1 + 0x64) = 0x7EB9DC41u;
    fn_801E8D74(res, obj + 0xFC, obj, T1);
    u16 handle = *(u16*)res;
    u16 hv = handle;
    u8* pool = lbl_805FB8D8;
    u8* slot;
    if (fn_801E7460(pool, &hv)) {
        slot = pool + ((u32)(handle & 0xFF) << 7);
    } else {
        slot = NULL;
    }
    if (slot == NULL) {
        return;
    }
    fn_801E9EE4(T2, pts, 1, *(s32*)lbl_80413400, a3, 1, -1, dur, 0.0f);
    *(u32*)(T2 + 0x64) = 0x79E5C42Fu;
    fn_801E8D74(res, slot, obj, T2);

    *(s32*)buf = lbl_805FB6B8;
    u16 colHalf = *(u16*)(col + 2);
    u8 col1 = col[1];
    u8 col0 = col[0];
    fn_801B72A0(buf, obj);
    *(u16*)(T3 + 0x0) = handle;
    *(s32*)(T3 + 0x4) = *(s32*)buf;
    *(u16*)(T3 + 0xA) = colHalf;
    T3[0x9] = col1;
    T3[0x8] = col0;
    *(s32*)(T3 + 0xC) = id;
    fn_801E8D74(res, slot, obj, T3);

    if (extra != NULL) {
        f32 qEnd[4];
        f32 qStart[4];
        u8* qpts[4];
        fn_801EA59C(T4);
        u8* pose2 = *(u8**)(obj + 0x4);
        qEnd[0] = *(f32*)(pose2 + 0xC);
        qEnd[1] = *(f32*)(pose2 + 0x10);
        qEnd[2] = *(f32*)(pose2 + 0x14);
        qEnd[3] = *(f32*)(pose2 + 0x18);
        qStart[0] = *(f32*)(extra + 0x0);
        qStart[1] = *(f32*)(extra + 0x4);
        qStart[2] = *(f32*)(extra + 0x8);
        qStart[3] = *(f32*)(extra + 0xC);
        qpts[0] = (u8*)qEnd;
        qpts[1] = (u8*)qStart;
        qpts[2] = NULL;
        qpts[3] = NULL;
        f32 scale = fn_801EA350(T2);
        fn_801EA62C(T4, qpts, 2, *(s32*)(lbl_80413400 + 4), -1, scale, 0.0f);
        *(u32*)(T4 + 0x64) = 0x9762A28Eu;
        fn_801E86FC(res, slot, obj, T4);
    }
}

// Scans the 0x48-byte records of the effect list for the position along the track (time 0 to the
// end in steps of 1/30) where the average distance of two marker points (indices obj+0xA0 and
// obj+0xA4) from the targets a and b is smallest, advancing/evaluating the animation track each
// step. Returns that time and sets the done flag at +0xD8.
// 0x8018AF28
f32 fn_8018AF28(u8* obj, f32* a, f32* b) {
    f32 bestT = 0.0f;
    f32 bestD = 99999.99f;
    f32 t = 0.0f;
    u8* root = *(u8**)obj;
    f32 endT = *(f32*)(*(u8**)(*(u8**)(root + 0x14) + 0x14) + 0x20);
    while (t < endT) {
        u8* track = *(u8**)(root + 0x14);
        f32 v = t;
        f32 old = *(f32*)(track + 0x24);
        if (!(t >= 0.0f)) {
            u8* clip = *(u8**)(track + 0x14);
            while (v < 0.0f) {
                v += *(f32*)(clip + 0x20);
            }
        }
        u8* clip = *(u8**)(track + 0x14);
        while (v > *(f32*)(clip + 0x20)) {
            v -= *(f32*)(clip + 0x20);
        }
        *(f32*)(track + 0x24) = v;
        *(s32*)(track + 0x1C) = 0;
        if (v < old) {
            fn_8016A3F8(track);
        }
        fn_8016A43C(track);
        fn_8016A788(track);
        fn_80173944(*(u8**)obj);
        root = *(u8**)obj;
        u8* base = *(u8**)(root + 0x4);
        u8* pa = base + *(s32*)(obj + 0xA0) * 0x30;
        u8* pb = base + *(s32*)(obj + 0xA4) * 0x30;
        f32 d1x = (*(f32*)(pa + 0x24) - *(f32*)(base + 0x24)) - a[0];
        f32 d1y = (*(f32*)(pa + 0x28) - *(f32*)(base + 0x28)) - a[1];
        f32 d1z = (*(f32*)(pa + 0x2C) - *(f32*)(base + 0x2C)) - a[2];
        f32 d2x = (*(f32*)(pb + 0x24) - *(f32*)(base + 0x24)) - b[0];
        f32 d2y = (*(f32*)(pb + 0x28) - *(f32*)(base + 0x28)) - b[1];
        f32 d2z = (*(f32*)(pb + 0x2C) - *(f32*)(base + 0x2C)) - b[2];
        f32 len1 = SqrtGuard(d1z * d1z + (d1x * d1x + d1y * d1y));
        f32 len2 = SqrtGuard(d2z * d2z + (d2x * d2x + d2y * d2y));
        f32 avg = 0.5f * (len1 + len2);
        if (avg < bestD) {
            bestT = t;
            bestD = avg;
        }
        t += 0.033333335f;
    }
    *(s32*)(obj + 0xD8) = 1;
    return bestT;
}

// Clamps v to [0, 1] (NaN becomes 0), as the original's compare chain does.
static f32 Clamp01(f32 v) {
    if (!(v >= 0.0f)) {
        return 0.0f;
    }
    if (!(v <= 1.0f)) {
        return 1.0f;
    }
    return v;
}

// Scales the +0x28 value (from +0x24) of each of the 0x2F voice pairs whose object (+0xC / +0x120
// of each 0x228 record) is in state 6 or 7 by volume f.
static void ApplyVoiceScale(f32 f) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    lbl_805F98D4 = f;
    u8* rec = lbl_8051E6A8 + 0x228;
    for (s32 i = 0; i < 0x2F; i++) {
        u8* v = *(u8**)(rec + 0xC);
        if (v != NULL && (u32)(*(s32*)(v + 0x14) - 6) <= 1u) {
            *(s32*)(v + 0x28) = (s32)((f32)*(s32*)(v + 0x24) * f);
        }
        v = *(u8**)(rec + 0x120);
        if (v != NULL && (u32)(*(s32*)(v + 0x14) - 6) <= 1u) {
            *(s32*)(v + 0x28) = (s32)((f32)*(s32*)(v + 0x24) * f);
        }
        rec += 0x228;
    }
    lbl_805FBAE0 = saved;
}

// Audio fade/shutdown step: releases every active entry of the sound table, clears the audio
// counters at +0x2B0/+0x2B4, then flushes lbl_805FBA88 and waits (1 ms sleeps) until it becomes
// nonzero. If a volume change was pending (lbl_805FBAF8) the three volumes (lbl_805FBAEC,
// lbl_805FBAF0, lbl_805FBAF4) are clamped to [0, 1] and applied. Finally clears lbl_805FBAE4.
// The original returns the leftover r3 (the argument); the value is unused.
// 0x802000F0
void fn_802000F0(u8* audio) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    u8* ent = lbl_80524E28;
    for (s32 i = 0; i < 0x60; i++) {
        if (*(s32*)(ent + 0x10) != 0) {
            fn_801FF2A8(audio, *(s32*)(ent + 0x0));
        }
        ent += 0x58;
    }
    lbl_805FBAE0 = saved;
    *(s32*)(audio + 0x2B0) = 0;
    *(s32*)(audio + 0x2B4) = 0;
    lbl_805FBA88 = 0;
    fn_80142ED0((u8*)&lbl_805FBA88, 4);
    while (lbl_805FBA88 == 0) {
        SleepMicroseconds(0, 1000);
    }
    if (lbl_805FBAF8 != 0) {
        lbl_805FBAF8 = 0;
        f32 v = Clamp01(lbl_805FBAEC);
        fn_802044B8(lbl_805FB7E8, v);
        if (lbl_805FBAF8 != 0) {
            lbl_805FBAEC = v;
        } else {
            fn_801FFC58(v);
            ApplyVoiceScale(Clamp01(v));
        }
        f32 v1 = Clamp01(lbl_805FBAF0);
        if (lbl_805FBAF8 != 0) {
            lbl_805FBAF0 = v1;
        } else {
            fn_801FF910(v1);
        }
        f32 v2 = Clamp01(lbl_805FBAF4);
        if (lbl_805FBAF8 != 0) {
            lbl_805FBAF4 = v2;
        } else {
            ApplyVoiceScale(Clamp01(v2));
        }
    }
    lbl_805FBAE4 = 0;
    fn_80209810();
}

}  // extern "C"
