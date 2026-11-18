// Debris when something breaks: particle effects scattered over each collision shape of the
// broken object, more for bigger shapes (up to 20 per shape).
// Original: DebrisSpawnForCollider 0x801249C4, DebrisLongestAxis 0x80124900
// (auto_03_801243B8_text.s)
#include "types.h"
#include "engine/math.h"

extern "C" {
BOOL SpawnEffectAtPos(u8* mgr, u8* effect, const f32* pos, u8** owner);   // 0x801C81D8
u32   RandomU32(void);
}

extern u8   lbl_8047C760[];         // effect manager
extern f32  gDebrisVolumeUnit;      ORIG(0x805FB2DC)
extern f32  gDebrisVolume;          ORIG(0x805FB2E0)   // running total for this break
extern u8*  gDebrisEffect;          ORIG(0x805FB2E4)   // +0x8: pieces per volume unit

enum { SHAPE_SPHERE = 0, SHAPE_BOX = 4, SHAPE_OBB = 7 };
#define MAX_PIECES 20

// Collider: +0x0C shape, +0x14 centre; sphere: +0x3C radius; box: +0x3C half extents;
// oriented box: +0x3C three axes (3 floats each), +0x6C half extents.
#define COL_SHAPE(c)  (*(s32*)((c) + 0x0C))
#define COL_CENTER(c) ((const f32*)((c) + 0x14))
#define COL_V3(c, o)  ((const f32*)((c) + (o)))

static inline f32 RandRange(f32 lo, f32 hi) {
    return lo + (hi - lo) * (2.3283064e-10f * (f32)RandomU32());
}

static inline f32 SphereRadius(u8* col) {
    f32 r = *(f32*)(col + 0x3C);
    BOOL zero = r > -0.0001f && r < 0.0001f;
    return (zero || r < 0.0f) ? 0.5f : r;
}

static inline void Spawn(f32 x, f32 y, f32 z) {
    f32 p[4] = { x, y, z, 0.0f };
    SpawnEffectAtPos(lbl_8047C760, gDebrisEffect, p, 0);
}

// 0x80124900
// Pick the longest of the three half extents; returns its axis index, length and direction.
extern "C" void DebrisLongestAxis(const f32 ext[3], const f32 axes[9], s32* index, f32* len, f32 dir[3]) {
    if (ext[0] >= ext[1] && ext[0] >= ext[2]) {
        *len = ext[0];
        *index = 0;
    } else if (ext[1] >= ext[0] && ext[1] >= ext[2]) {
        *len = ext[1];
        *index = 1;
    } else {
        *len = ext[2];
        *index = 2;
    }
    dir[0] = axes[*index * 3 + 0];
    dir[1] = axes[*index * 3 + 1];
    dir[2] = axes[*index * 3 + 2];
}

// 0x801249C4
// Pieces are spread along the longest axis of a box with some jitter across it, or randomly
// inside a sphere.
extern "C" BOOL DebrisSpawnForCollider(u8* col) {
    f32 volume = 0.0f;
    switch (COL_SHAPE(col)) {
    case SHAPE_BOX: {
        const f32* e = COL_V3(col, 0x3C);
        volume = e[2] * (e[0] * e[1]);
        break;
    }
    case SHAPE_OBB: {
        const f32* e = COL_V3(col, 0x6C);
        volume = e[2] * (e[0] * e[1]);
        break;
    }
    case SHAPE_SPHERE: {
        f32 r = SphereRadius(col);
        volume = r * (4.1887894f * r * r);
        break;
    }
    }
    f32 units = volume / gDebrisVolumeUnit;
    s32 count = (s32)(units * (f32)*(s32*)(gDebrisEffect + 8));
    if (count < 1) count = 1;
    if (count > MAX_PIECES) count = MAX_PIECES;
    gDebrisVolume += units;

    const f32* c = COL_CENTER(col);
    switch (COL_SHAPE(col)) {
    case SHAPE_BOX: {
        const f32* e = COL_V3(col, 0x3C);
        f32 len, axis[3];
        if (e[0] >= e[1] && e[0] >= e[2]) {
            len = e[0]; axis[0] = 1.0f; axis[1] = 0.0f; axis[2] = 0.0f;
        } else if (e[1] >= e[0] && e[1] >= e[2]) {
            len = e[1]; axis[0] = 0.0f; axis[1] = 1.0f; axis[2] = 0.0f;
        } else {
            len = e[2]; axis[0] = 0.0f; axis[1] = 0.0f; axis[2] = 1.0f;
        }
        f32 step = (2.0f * len) / (f32)(count + 1);
        f32 back = len - step;
        f32 jx = e[0] * 0.3f, jy = e[1] * 0.3f, jz = e[2] * 0.3f;
        f32 p[3] = { c[0] - axis[0] * back, c[1] - axis[1] * back, c[2] - axis[2] * back };
        for (s32 i = 0; i < count; i++) {
            f32 d[3] = { 0.0f, 0.0f, 0.0f };
            if (axis[0] > 0.5f) {
                d[1] = RandRange(-jy, jy);
                d[2] = RandRange(-jz, jz);
            } else if (axis[1] > 0.5f) {
                d[0] = RandRange(-jx, jx);
                d[2] = RandRange(-jz, jz);
            } else {
                d[0] = RandRange(-jx, jx);
                d[1] = RandRange(-jy, jy);
            }
            Spawn(p[0] + d[0], p[1] + d[1], p[2] + d[2]);
            p[0] += axis[0] * step;
            p[1] += axis[1] * step;
            p[2] += axis[2] * step;
        }
        break;
    }
    case SHAPE_OBB: {
        f32 e[3] = { COL_V3(col, 0x6C)[0], COL_V3(col, 0x6C)[1], COL_V3(col, 0x6C)[2] };
        const f32* axes = COL_V3(col, 0x3C);
        s32 idx;
        f32 len, dir[3];
        DebrisLongestAxis(e, axes, &idx, &len, dir);
        f32 step = (2.0f * len) / (f32)(count + 1);
        f32 back = len - step;
        f32 p[3] = { c[0] - dir[0] * back, c[1] - dir[1] * back, c[2] - dir[2] * back };
        for (s32 i = 0; i < count; i++) {
            f32 jx = e[0] * 0.3f, jy = e[1] * 0.3f, jz = e[2] * 0.3f;
            // Jitter along the two axes that are not the long one.
            s32 a1, a2;
            f32 r1, r2;
            if (idx == 0) {
                a1 = 1; a2 = 2;
                r1 = RandRange(-jy, jy);
                r2 = RandRange(-jz, jz);
            } else if (idx == 1) {
                a1 = 0; a2 = 2;
                r1 = RandRange(-jx, jx);
                r2 = RandRange(-jz, jz);
            } else {
                a1 = 0; a2 = 1;
                r1 = RandRange(-jx, jx);
                r2 = RandRange(-jy, jy);
            }
            f32 d[3];
            for (s32 k = 0; k < 3; k++) d[k] = (0.0f + axes[a1 * 3 + k] * r1) + axes[a2 * 3 + k] * r2;
            Spawn(p[0] + d[0], p[1] + d[1], p[2] + d[2]);
            p[0] += dir[0] * step;
            p[1] += dir[1] * step;
            p[2] += dir[2] * step;
        }
        break;
    }
    case SHAPE_SPHERE: {
        f32 r = SphereRadius(col);
        if (r == 0.0f) r = 1.0f;
        f32 half = 0.5f * r - 0.0f;
        for (s32 i = 0; i < count; i++) {
            f32 x = 0.0f + half * (2.3283064e-10f * (f32)RandomU32());
            f32 y = 0.0f + half * (2.3283064e-10f * (f32)RandomU32());
            f32 z = 0.0f + half * (2.3283064e-10f * (f32)RandomU32());
            Spawn(x + c[0], y + c[1], z + c[2]);
        }
        break;
    }
    }
    return TRUE;
}
