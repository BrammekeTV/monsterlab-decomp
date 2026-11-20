// Misc decompiled functions (batch miss295): sweeping-move setup for a moving entity (0x80064xxx),
// nested std::map "insert empty entry" helper (0x800A43C0) and world zone-slot layout (0x80129CC8).
#include <math.h>

#include "types.h"

typedef s32 (*ShapeFn)(u8*, u8*);

extern "C" {
u8*   GetGlobalSettings(void);                                                  // 0x801DB5E4
u32   RandomU32(void);
void  operator_delete(void* p);                                                 // 0x801D426C
void  fn_800A3188(u8* tree, u8* node);                                          // 0x800A3188
void  fn_800A4EBC(u8* pair, s32 key, u8* srcTree);                              // 0x800A4EBC
u8*   fn_800A526C(u8* map, u8* parent, u8 isLeft, u8 allGreater, u8* pair);   // 0x800A526C (returns the new node)
void  fn_800A53B8(u8* dst, u8* src);                                            // 0x800A53B8
u8*   fn_800A5AEC(u8* p, u32* keyp, void** outNode, u8* outLeft, u8* outAll);   // 0x800A5AEC
s32   fn_80067C30(u8* obj, u8* ctx);                                            // 0x80067C30 (shape callback)
void  fn_8006784C(u8* self);                                                    // 0x8006784C
s32   fn_80138C40(void* world, f32* a, f32* b, f32* hit, void* p, s32 c, s32 d); // 0x80138C40
void  fn_801B154C(u8* group, u32 key, ShapeFn fn, u8* arg);                     // 0x801B154C
void  fn_801B1778(u8* p, s32 a);                                                // 0x801B1778
u8*   fn_801F06FC(void);                                                        // 0x801F06FC (world params block)
}

extern "C++" u8   gWorldState[];          // 0x803E6D00
extern "C++" s32  gZoneResourceSize;      // 0x805FB94C

static const u32 kHashBase = 0xF62113D5u;  // "BAse"

static inline s32 GS(u32 off) { return *(s32*)(GetGlobalSettings() + off); }

// Sum of the 13 per-type zone counters at settings +0x34..+0x64.
static inline s32 ZoneTypeCountSum(void) {
    s32 s = 0;
    for (u32 off = 0x34; off <= 0x64; off += 4) s += GS(off);
    return s;
}

// Length of (x, y, z): zero when the squared length is not positive.
static inline f32 Len3(f32 x, f32 y, f32 z) {
    f32 d2 = (x * x + y * y) + z * z;
    return d2 <= 0.0f ? 0.0f : sqrtf(d2);
}

// Normalises (x, y, z) into out; the zero vector when the squared length is exactly zero.
static inline void Normalize3(f32* out, f32 x, f32 y, f32 z) {
    f32 d2 = (x * x + y * y) + z * z;
    if (d2 == 0.0f) {
        out[0] = out[1] = out[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(d2);
        out[0] = x * inv;
        out[1] = y * inv;
        out[2] = z * inv;
    }
}

// Frees a (up to four levels unrolled in the original) subtree of a std::map temp tree.
static inline void FreeTree(u8* tree, u8* root) {
    if (root != NULL) fn_800A3188(tree, root);
}

extern "C" {

// 0x800645C8
// Starts a movement of the entity self (arg0) along the vector dir (arg1): scales it by the
// movement params, sweeps a segment from the entity position along it against the world
// (fn_80138C40) and the nearby zone entities (a sphere test through fn_80067C30), and works
// out the travel speed (+0x230) and a random side vector (+0x210) for the move.
void fn_800645C8(u8* arg0, u8* arg1) {
    u8* params = *(u8**)(arg0 + 0x228);
    f32 scale = *(f32*)(params + 0xB8);

    *(f32*)(arg0 + 0x1AC) = *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x1B0) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x1B4) = *(f32*)(arg1 + 0x8);

    if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000) && !(*(u32*)(arg0 + 0x244) & 0x4000)) {
        f32 rot = *(f32*)(params + 0xD8);
        if (rot != 0.0f) {
            // Tilt the direction toward +z by rot * (pi / 2).
            f32 ang = 1.5707964f * rot;
            f32 c = cosf(ang);
            f32 s = sinf(ang);
            f32 x = *(f32*)(arg0 + 0x1AC) * c + 0.0f * s;
            f32 y = *(f32*)(arg0 + 0x1B0) * c + 0.0f * s;
            f32 z = *(f32*)(arg0 + 0x1B4) * c + 1.0f * s;
            *(f32*)(arg0 + 0x1AC) = x;
            *(f32*)(arg0 + 0x1B0) = y;
            *(f32*)(arg0 + 0x1B4) = z;
        }
        *(f32*)(arg0 + 0x1AC) = *(f32*)(arg0 + 0x1AC) * scale;
        *(f32*)(arg0 + 0x1B0) = *(f32*)(arg0 + 0x1B0) * scale;
        *(f32*)(arg0 + 0x1B4) = *(f32*)(arg0 + 0x1B4) * scale;
    }

    u32 state = *(u32*)(arg0 + 0x244);
    if (state & 0x20) {
        if (*(f32*)(*(u8**)(arg0 + 0x228) + 0xD4) != 0.0f) return;

        *(u32*)(arg0 + 0x244) = state & 0xFBFFFFFFu;

        // Hit record filled by the world segment query; +0x50/+0x54 are the query filter masks.
        u8 hitBuf[0x6C];
        for (s32 i = 0; i < 0x6C; i++) hitBuf[i] = 0;
        *(s32*)(hitBuf + 0x50) = 0;
        *(s32*)(hitBuf + 0x54) = 4;

        params = *(u8**)(arg0 + 0x228);
        f32 speedMul = *(f32*)(params + 0xD0);
        if (!(speedMul > 0.0f)) speedMul = 1.0f;
        f32 distDiv = *(f32*)(params + 0xB8);
        if (!(distDiv > 0.0f)) distDiv = 1.0f;

        u8* ent = *(u8**)(arg0 + 0x4);
        f32 p0[3], p1[3];
        p0[0] = *(f32*)(ent + 0x0);
        p0[1] = *(f32*)(ent + 0x4);
        p0[2] = *(f32*)(ent + 0x8);
        p1[0] = p0[0] + *(f32*)(arg0 + 0x1AC) * speedMul;
        p1[1] = p0[1] + *(f32*)(arg0 + 0x1B0) * speedMul;
        p1[2] = p0[2] + *(f32*)(arg0 + 0x1B4) * speedMul;

        s32 worldHit = fn_80138C40(gWorldState, p0, p1, (f32*)hitBuf, NULL, 0, 0);

        f32 segLen = Len3(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]);

        f32 mid[3];
        mid[0] = (p1[0] + p0[0]) * 0.5f;
        mid[1] = (p1[1] + p0[1]) * 0.5f;
        mid[2] = (p1[2] + p0[2]) * 0.5f;

        // Context for the per-entity callback fn_80067C30.
        u8 ctx[0x34];
        *(f32*)(ctx + 0x00) = *(f32*)(arg1 + 0x0);
        *(f32*)(ctx + 0x04) = *(f32*)(arg1 + 0x4);
        *(f32*)(ctx + 0x08) = *(f32*)(arg1 + 0x8);
        *(f32*)(ctx + 0x0C) = mid[0];
        *(f32*)(ctx + 0x10) = mid[1];
        *(f32*)(ctx + 0x14) = mid[2];
        *(f32*)(ctx + 0x18) = 0.0f;
        *(f32*)(ctx + 0x1C) = 0.0f;
        *(f32*)(ctx + 0x20) = 0.0f;
        *(u8**)(ctx + 0x24) = arg0;
        *(f32*)(ctx + 0x28) = *(f32*)(*(u8**)(arg0 + 0x22C) + 0x3C) + segLen;
        *(u8**)(ctx + 0x2C) = NULL;
        *(u8**)(ctx + 0x30) = NULL;

        // Remember the swept segment in the move record.
        u8* rec = *(u8**)(arg0 + 0x22C);
        *(f32*)(rec + 0x20) = p0[0];
        *(f32*)(rec + 0x24) = p0[1];
        *(f32*)(rec + 0x28) = p0[2];
        rec = *(u8**)(arg0 + 0x22C);
        *(f32*)(rec + 0x14) = p1[0];
        *(f32*)(rec + 0x18) = p1[1];
        *(f32*)(rec + 0x1C) = p1[2];

        // Sphere test of the swept segment against the collidable entities of the type-6 zones.
        // (The original's "break" flag is never set: every path continues with the next entity.)
        s32 zoneBound = ZoneTypeCountSum();
        for (s32 zi = 0; zi < zoneBound + 1; zi++) {
            s32 zone = -1;
            if (zi == ZoneTypeCountSum()) {
                zone = 0;
            } else {
                u8* e = *(u8**)(gWorldState + 0xB0) + zi * 0x30;
                if (*(s32*)(e + 0x4) == 6) zone = *(s32*)(e + 0x8);
            }
            if (zone < 0) continue;

            u8* zoneBase = *(u8**)(gWorldState + 0x1A4) + zone * gZoneResourceSize;
            s32 nA = *(s32*)(zoneBase + 0x94);
            s32 total = nA + *(s32*)(zoneBase + 0x98);
            for (s32 k = 0; k < total; k++) {
                zoneBase = *(u8**)(gWorldState + 0x1A4) + zone * gZoneResourceSize;
                nA = *(s32*)(zoneBase + 0x94);
                u8* cand;
                u8* node;
                if (k < nA) {
                    cand = (*(u8***)(zoneBase + 0x120))[k];
                    node = *(u8**)(zoneBase + 0x8C) + k * 0x40;
                } else {
                    s32 j = k - nA;
                    cand = (*(u8***)(zoneBase + 0x12C))[j];
                    node = *(u8**)(zoneBase + 0x90) + j * 0x40;
                }
                if (!(*(u32*)(node + 0x1C) & 0x50) || !(*(u32*)(node + 0x20) & 2) || cand == NULL) continue;
                if (cand == *(u8**)(arg0 + 0x23C)) continue;
                u8* lockTarget = *(u8**)(arg0 + 0x248);
                if ((*(u32*)(arg0 + 0x244) & 0x8000) && lockTarget != NULL && lockTarget != cand) continue;

                u8* shape = *(u8**)(cand + 0x8C);
                if (shape == NULL) continue;
                f32 dx = mid[0] - *(f32*)(cand + 0x18);
                f32 dy = mid[1] - *(f32*)(cand + 0x1C);
                f32 dz = mid[2] - *(f32*)(cand + 0x20);
                f32 reach = *(f32*)(ctx + 0x28) + *(f32*)(cand + 0x14);
                f32 d2 = (dx * dx + dy * dy) + dz * dz;
                if (d2 <= reach * reach) {
                    if (*(u32*)(*(u8**)(cand + 0x4) + 0x20) & 0x4000) fn_801B1778(shape, 0);
                    *(u8**)(ctx + 0x2C) = cand;
                    fn_801B154C(*(u8**)(cand + 0x8C), kHashBase, fn_80067C30, ctx);
                }
            }
        }

        // The callback may have stored the entity the move ends on at ctx+0x30.
        s32 hasEnt = 0;
        f32 entPos[3] = { 0.0f, 0.0f, 0.0f };
        u8* endEnt = *(u8**)(ctx + 0x30);
        if (endEnt != NULL &&
            (!(*(u32*)(*(u8**)(endEnt + 0x4) + 0x1C) & 0x40) || !(*(u32*)(arg0 + 0x244) & 0x40))) {
            hasEnt = 1;
            entPos[0] = *(f32*)(endEnt + 0x18);
            entPos[1] = *(f32*)(endEnt + 0x1C);
            entPos[2] = *(f32*)(endEnt + 0x20);
        }

        // Travel speed: limited by the world hit, the entity found, or the full speed multiplier.
        f32 hitT = *(f32*)(hitBuf + 0x18);
        f32 speed;
        if (worldHit != 0 && hasEnt != 0) {
            f32 toEnt = Len3(entPos[0] - p0[0], entPos[1] - p0[1], entPos[2] - p0[2]);
            speed = hitT * *(f32*)(*(u8**)(arg0 + 0x228) + 0xD0);
            f32 alt = toEnt / distDiv;
            if (speed >= alt) speed = alt;
        } else if (worldHit != 0) {
            speed = hitT * *(f32*)(*(u8**)(arg0 + 0x228) + 0xD0);
        } else if (hasEnt != 0) {
            speed = Len3(entPos[0] - p0[0], entPos[1] - p0[1], entPos[2] - p0[2]) / distDiv;
        } else {
            speed = speedMul;
        }
        *(f32*)(arg0 + 0x230) = speed;

        // Side vector: dir x (0,0,1), normalised, flipped at random; then tilted toward +z.
        f32 dirx = *(f32*)(arg1 + 0x0);
        f32 diry = *(f32*)(arg1 + 0x4);
        f32 dirz = *(f32*)(arg1 + 0x8);
        f32 side[3];
        Normalize3(side, diry * 1.0f - dirz * 0.0f, dirz * 0.0f - dirx * 1.0f, dirx * 0.0f - diry * 0.0f);
        *(f32*)(arg0 + 0x210) = side[0];
        *(f32*)(arg0 + 0x214) = side[1];
        *(f32*)(arg0 + 0x218) = side[2];
        if (RandomU32() & 1) {
            *(f32*)(arg0 + 0x210) = -*(f32*)(arg0 + 0x210);
            *(f32*)(arg0 + 0x214) = -*(f32*)(arg0 + 0x214);
            *(f32*)(arg0 + 0x218) = -*(f32*)(arg0 + 0x218);
        }
        f32 tilt[3];
        Normalize3(tilt, *(f32*)(arg0 + 0x210) + 0.0f, *(f32*)(arg0 + 0x214) + 0.0f,
                   *(f32*)(arg0 + 0x218) + 1.0f);

        f32 spd = *(f32*)(arg0 + 0x230);
        *(f32*)(arg0 + 0x210) = tilt[0];
        *(f32*)(arg0 + 0x218) = tilt[2];
        *(f32*)(arg0 + 0x214) = tilt[1];
        *(f32*)(arg0 + 0x220) = spd;
        *(f32*)(arg0 + 0x224) = 1.0f / spd;
        *(f32*)(arg0 + 0x21C) = spd * *(f32*)(*(u8**)(arg0 + 0x228) + 0xB8) * 0.25f;
    } else if (state & 0x800) {
        fn_8006784C(arg0);
    }
}

// 0x800A43C0
// std::map<u32, std::map<...>>::operator[]-style insert: when no entry with the key arg1 exists in
// the map arg0, builds a temporary (key, empty sub-map) pair and inserts it, then frees the
// temporary trees (unrolled four levels deep in the original, deeper nodes through fn_800A3188).
void fn_800A43C0(u8* arg0, u32 arg1) {
    // lower_bound(key): last node whose key is not below arg1.
    u8* end = arg0 + 4;
    u8* lb = end;
    u8* cur = *(u8**)(arg0 + 4);
    while (cur != NULL) {
        if (*(u32*)(cur + 0xC) >= arg1) {
            lb = cur;
            cur = *(u8**)(cur + 0x0);
        } else {
            cur = *(u8**)(cur + 0x4);
        }
    }
    if (lb == end || arg1 < *(u32*)(lb + 0xC)) lb = end;
    if (lb != end) return;

    // Temporary empty sub-map trees: {count, root, &root}.
    u8* tmp3C[3] = { NULL, NULL, NULL };
    u8* tmp30[3] = { NULL, NULL, NULL };
    tmp3C[2] = (u8*)&tmp3C[1];
    tmp30[2] = (u8*)&tmp30[1];
    fn_800A53B8((u8*)tmp30, (u8*)tmp3C);

    // pair20 = (key, copy of tmp30); pair10 = (key, copy of pair20's tree).
    u8* pair20[4];
    fn_800A4EBC((u8*)pair20, (s32)arg1, (u8*)tmp30);
    u8* pair10[4] = { NULL, NULL, NULL, NULL };
    u32 key = *(u32*)&pair20[0];
    *(u32*)&pair10[0] = key;
    pair10[3] = (u8*)&pair10[2];
    fn_800A53B8((u8*)&pair10[1], (u8*)&pair20[1]);

    void* where = NULL;
    u8 isLeft, allGreater;
    u8* parent = fn_800A5AEC(arg0, (u32*)&pair10[0], &where, &isLeft, &allGreater);
    if (where == NULL || *(u32*)((u8*)where + 0xC) < *(u32*)&pair10[0]) {
        fn_800A526C(arg0, parent, isLeft, allGreater, (u8*)pair10);
    }

    FreeTree((u8*)&pair10[1], pair10[2]);
    FreeTree((u8*)&pair20[1], pair20[2]);
    FreeTree((u8*)tmp30, tmp30[1]);
    FreeTree((u8*)tmp3C, tmp3C[1]);
}

// 0x80129CC8
// Lays out the world's zone-slot tables inside the world state's shared pool (+0x21E0): the zone
// slot array (+0x1A4, count+1 entries of gZoneResourceSize bytes), each slot's two entity tables
// (+0x120 / +0x12C) and the per-slot arrays at +0x1B0, +0x1BC, +0x1C8 and +0x1D4.
void fn_80129CC8(u8* arg0) {
    u8* cursorB = arg0 + 0x21E0;     // shared pool cursor
    u8* cursorA;                     // cursor inside the current slot

    s32 n = GS(0x80) + GS(0x68) + GS(0x84) + GS(0x88) + GS(0x8C) + GS(0x90) + GS(0x94) + GS(0x98) +
            GS(0x6C) + GS(0x70) + GS(0x74) + GS(0x78) + GS(0x7C) + 1;
    *(u8**)(arg0 + 0x1A4) = cursorB;
    *(s32*)(arg0 + 0x1A8) = n;
    *(s32*)(arg0 + 0x1AC) = 0;

    s32 perSlot = GS(0x34) * 0x54;
    s32 slots = GS(0x80) + GS(0x68) + GS(0x84) + GS(0x88) + GS(0x8C) + GS(0x90) + GS(0x94) +
                GS(0x98) + GS(0x6C) + GS(0x70) + GS(0x74) + GS(0x78) + GS(0x7C) + 1;
    cursorB += (perSlot + 0x148) * slots;

    for (s32 i = 0;; i++) {
        s32 count = GS(0x80) + GS(0x68) + GS(0x84) + GS(0x88) + GS(0x8C) + GS(0x90) + GS(0x94) +
                    GS(0x98) + GS(0x6C) + GS(0x70) + GS(0x74) + GS(0x78) + GS(0x7C) + 1;
        if (!(i < count)) break;

        u8* slot = *(u8**)(arg0 + 0x1A4) + i * gZoneResourceSize;
        cursorA = slot + 0x140;
        *(u8**)(slot + 0x114) = slot + 0x140;
        *(s32*)(slot + 0x118) = GS(0x34);
        *(s32*)(slot + 0x11C) = 0;
        cursorA += GS(0x34) * 0x54;

        s32 nFirst = *(s32*)(fn_801F06FC() + 0x4);
        s32 nSecond = *(s32*)(fn_801F06FC() + 0x0);
        u8** cursor = &cursorA;

        // Slots in these index ranges keep one entry per table (bounds are cumulative counters).
        s32 c1 = GS(0x6C) + GS(0x68) + GS(0x70) + GS(0x74) + GS(0x78) + GS(0x7C);
        s32 c2 = c1 + GS(0x80);
        s32 c3 = c2 + GS(0x84);
        s32 c4 = c3 + GS(0x88);
        BOOL single = (i > c1 && i <= c2) || (i > c2 && i <= c3) || (i > c3 && i <= c4) ||
                      (i > GS(0x68) && i <= c1);
        if (single) {
            nFirst = 1;
            nSecond = 1;
        } else {
            cursorA += 8;
            cursor = &cursorB;
        }

        *(u8**)(slot + 0x120) = *cursor;
        *(s32*)(slot + 0x124) = nFirst;
        *(s32*)(slot + 0x128) = 0;
        *cursor += nFirst * 4;
        *(u8**)(slot + 0x12C) = *cursor;
        *(s32*)(slot + 0x130) = nSecond;
        *(s32*)(slot + 0x134) = 0;
        *cursor += nSecond * 4;
    }

    // Per-slot arrays that follow the slot tables in the pool (4, 4, 4 and 12 bytes per entry).
    s32 total = GS(0x80) + GS(0x68) + GS(0x84) + GS(0x88) + GS(0x8C) + GS(0x90) + GS(0x94) +
                GS(0x98) + GS(0x6C) + GS(0x70) + GS(0x74) + GS(0x78) + GS(0x7C) + 1;
    *(u8**)(arg0 + 0x1B0) = cursorB;
    *(s32*)(arg0 + 0x1B4) = total;
    *(s32*)(arg0 + 0x1B8) = 0;

    cursorB += total * 4;
    *(u8**)(arg0 + 0x1BC) = cursorB;
    *(s32*)(arg0 + 0x1C0) = total;
    *(s32*)(arg0 + 0x1C4) = 0;

    cursorB += total * 4;
    *(u8**)(arg0 + 0x1C8) = cursorB;
    *(s32*)(arg0 + 0x1CC) = total;
    *(s32*)(arg0 + 0x1D0) = 0;

    cursorB += total * 0xC;
    *(u8**)(arg0 + 0x1D4) = cursorB;
    *(s32*)(arg0 + 0x1D8) = total;
    *(s32*)(arg0 + 0x1DC) = 0;
}

}  // extern "C"
