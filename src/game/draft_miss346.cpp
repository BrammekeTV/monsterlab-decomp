// Missing functions batch 346: lightning/rope node builder, particle-source helpers, effect lists.
#include "types.h"
#include <string.h>

#include "engine/math.h"

extern "C" {
void  fn_8018B2CC(u8* obj, u8* entry, s32 a, s32 b, f32 c, f32 d);  // 0x8018B2CC
void  fn_801BCD88(u8* node, s32 a, s32 b, f32 scale);               // 0x801BCD88
void  fn_801BEDBC(u8* def, f32* a, f32* b, s32 flag, u8* def2);     // 0x801BEDBC
void* fn_801EB94C(void* p);                                         // 0x801EB94C
u32   RandomU32(void);                                              // 0x80143460
void  QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);   // 0x80145EE8
void  fn_801C4B90(u8* obj, f32* a, f32* b, s32 c, s32 d, u32 color);  // 0x801C4B90
s32   fn_80146808(u8* file, const char* name, const char* mode);   // 0x80146808
s32   fn_80146930(u8* file, void* buf, s32 size);                  // 0x80146930
void  fn_801469FC(u8* file);                                       // 0x801469FC
s32   fn_80146A58(u8* file);                                       // 0x80146A58
void* MemAllocDefault(u32 size);                                   // 0x801D4650
void  fn_801C5E30(char* text);                                     // 0x801C5E30
char* fn_801C5F9C(char* line, u8* entry);                          // 0x801C5F9C
char* fn_80338BC0(char* s, const char* sub);                       // 0x80338BC0 (strstr)
BOOL  ZoneIsLoaded(void* world, s32 zone);                         // 0x8012F408
void* ObjAlloc(u32 size);                                          // 0x801D4414
void  BufferFree(void* p);                                         // 0x801D4980
void  fn_801EB748(void* p);                                        // 0x801EB748
void  fn_801EB858(void* pool, s32 elemSize, s32 align, void* buf, u32 size);  // 0x801EB858
s32   fn_801EBD1C(u8* node, void* owner);                          // 0x801EBD1C
void* fn_801EBD2C(void* node);                                     // 0x801EBD2C
void  fn_801CEC68(void* item, u8* obj, f32* pos, void* fxMgr);    // 0x801CEC68

extern f32 lbl_805FFAA8;  // 0x805FFAA8
extern u8 gWorldState[];  // 0x803E6D00
extern void* lbl_805FB700;  // 0x805FB700
extern void* lbl_805FB704;  // 0x805FB704
extern s32 lbl_805FB708;  // 0x805FB708
extern s32 lbl_805FB718;  // 0x805FB718
}

extern "C++" s32 gZoneResourceSize;  // 0x805FB94C

// Finds the 0x70-byte record with the given key in the table at tbl (+0xC count, +0x10 array,
// +0x14 sorted flag): binary search when sorted, linear otherwise. NULL if absent.
static u8* FindRecord(u8* tbl, u32 key) {
    s32 count = *(s32*)(tbl + 0xC);
    if (*(u8*)(tbl + 0x14) != 0) {
        s32 lo = -1;
        s32 hi = count;
        while (hi - lo > 1) {
            s32 mid = (hi + lo) / 2;
            if (key <= *(u32*)(*(u8**)(tbl + 0x10) + mid * 0x70)) hi = mid;
            if (key > *(u32*)(*(u8**)(tbl + 0x10) + mid * 0x70)) lo = mid;
        }
        u8* e = *(u8**)(tbl + 0x10) + hi * 0x70;
        if (key == *(u32*)e) return e;
        return NULL;
    }
    for (s32 i = 0; i < count; i++) {
        u8* base = *(u8**)(tbl + 0x10);
        if (key == *(u32*)(base + i * 0x70)) return base + i * 0x70;
    }
    return NULL;
}

// Random displacement in [-amp, amp).
static f32 RandSpread(f32 amp) {
    return amp * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - amp;
}

// Sets dst (point +0x4C) to the midpoint of a and b plus a random offset of amplitude amp.
static void MidDisplace(u8* a, u8* b, u8* dst, f32 amp) {
    f32 mx = (*(f32*)(a + 0x4C) + *(f32*)(b + 0x4C)) * 0.5f;
    f32 my = (*(f32*)(a + 0x50) + *(f32*)(b + 0x50)) * 0.5f;
    f32 mz = (*(f32*)(a + 0x54) + *(f32*)(b + 0x54)) * 0.5f;
    f32 dx = RandSpread(amp);
    f32 dy = RandSpread(amp);
    f32 dz = RandSpread(amp);
    *(f32*)(dst + 0x4C) = mx + dx;
    *(f32*)(dst + 0x50) = my + dy;
    *(f32*)(dst + 0x54) = mz + dz;
}

extern "C" {

// 0x801AC6B4
// Looks up an entry through the two-slot id table at +0x34 of the attached def and applies it
// to the object; returns 1 on success and links the def into the state at +0x84.
s32 fn_801AC6B4(u8* arg0, u8* arg1, u8* arg2) {
    u8* def = *(u8**)(arg0 + 0xC);
    u8* tbl = *(u8**)(def + 0x34);
    u16 id = *(u16*)(*(u8**)(arg2 + 0x4) + 0x32);

    s32 first = 0;
    for (s32 i = 0; i < 2; i++) {
        if ((s32)id == (s32)*(u16*)(tbl + 0x18 + i * 2)) {
            first = *(s32*)(tbl + 0x10 + i * 4);
            break;
        }
    }
    if (first == 0) id = 0;

    s32 idx = *(s32*)(def + 0x38);
    u8** list = NULL;
    for (s32 i = 0; i < 2; i++) {
        if ((s32)id == (s32)*(u16*)(tbl + 0x18 + i * 2)) {
            list = *(u8***)(tbl + 0x10 + i * 4);
            break;
        }
    }
    u8* entry = NULL;
    if (list != NULL && idx < *(s32*)(tbl + 0x1C) && idx >= 0) entry = list[idx];

    if (entry != NULL) {
        fn_8018B2CC(arg1, entry, 0x10000, *(s32*)(def + 0x2C), lbl_805FFAA8, lbl_805FFAA8);
        *(f32*)(arg1 + 0xB0) = lbl_805FFAA8;
        *(f32*)(arg1 + 0xE8) = lbl_805FFAA8;
        *(u8**)(arg2 + 0x84) = def;
        return 1;
    }
    return 0;
}

// 0x801BE620
// Creates a lightning/rope strand between two points from the record keyed by arg1: allocates
// the node, fills its end points and length, then midpoint-displaces the interior points.
u8* fn_801BE620(u8* arg0, u32 arg1, f32* arg2, f32* arg3, u32 arg4, s32 arg5) {
    u8* e1 = FindRecord(arg0, arg1);
    u8* e2 = NULL;
    if (arg4 != 0) e2 = FindRecord(arg0, arg4);

    u8* node = NULL;
    if (e1 == NULL) return node;
    if (arg5 == 0 && *(s32*)(e1 + 0x60) != 0) return NULL;

    node = (u8*)fn_801EB94C(*(void**)(e1 + 0x68));
    if (node == NULL) return node;

    fn_801BEDBC(e1, arg2, arg3, 1, e2);
    *(f32*)(node + 0x4C) = arg2[0];
    *(f32*)(node + 0x50) = arg2[1];
    *(f32*)(node + 0x54) = arg2[2];
    s32 n1 = *(s32*)(node + 0x38) - 1;
    u8* last = node + n1 * 0xC;
    *(f32*)(last + 0x4C) = arg3[0];
    *(f32*)(last + 0x50) = arg3[1];
    *(f32*)(last + 0x54) = arg3[2];

    f32 dx = arg3[0] - arg2[0];
    f32 dy = arg3[1] - arg2[1];
    f32 dz = arg3[2] - arg2[2];
    f32 l2 = dz * dz + (dx * dx + dy * dy);
    f32 len = 0.0f;
    if (l2 > 0.0f) len = sqrtf(l2);  // original: frsqrte with two Newton steps
    *(f32*)(node + 0x34) = len;

    u8* cfg = *(u8**)node;
    s32 n = *(s32*)(node + 0x38) - 1;
    f32 f30 = *(f32*)(cfg + 0x48);
    s32 half = n / 2;
    if (n > 1) {
        u8* endp = node + n * 0xC;
        u8* mid = node + half * 0xC;
        f32 amp = f30 * *(f32*)(node + 0x34) * *(f32*)(cfg + 0x4C);
        MidDisplace(node, endp, mid, amp);
        if (n > 2) {
            s32 q = half / 2;
            if (half > 1) {
                f32 amp2 = 0.5f * f30 * *(f32*)(node + 0x34) * *(f32*)(*(u8**)node + 0x4C);
                MidDisplace(node, mid, node + q * 0xC, amp2);
                if (half > 2) {
                    fn_801BCD88(node, 0, q, 0.5f * (0.5f * f30));
                    fn_801BCD88(node, q, half, 0.5f * (0.5f * f30));
                }
            }
            s32 rest = n - half;
            s32 q2 = half + rest / 2;
            if (rest > 1) {
                f32 amp3 = 0.5f * f30 * *(f32*)(node + 0x34) * *(f32*)(*(u8**)node + 0x4C);
                MidDisplace(mid, endp, node + q2 * 0xC, amp3);
                if (rest > 2) {
                    fn_801BCD88(node, half, q2, 0.5f * (0.5f * f30));
                    fn_801BCD88(node, q2, n, 0.5f * (0.5f * f30));
                }
            }
        }
    }
    return node;
}

// 0x801C4F30
// Emits a ribbon segment through center (arg1, +0xC = angle) along the axis dir (arg2): the
// perpendicular to dir is rotated by the dir/angle quaternion and the trail gets the two end
// points center +/- rotated * scale, colored from the 4 floats in arg3 (0..1 -> RGBA bytes).
void fn_801C4F30(u8* arg0, f32* arg1, f32* arg2, f32* arg3, f32 scale) {
    Vec3 center = { arg1[0], arg1[1], arg1[2] };
    Vec3 n = Vec3Normalized(*(Vec3*)arg2);

    // cross(n, +Z), normalized
    f32 cx = n.y * 1.0f - n.z * 0.0f;
    f32 cy = n.z * 0.0f - n.x * 1.0f;
    f32 cz = n.x * 0.0f - n.y * 0.0f;
    Vec3 w = Vec3Normalized(Vec3{ cx, cy, cz });

    Quat q;
    QuatFromAxisAngle(&q, &n, arg1[3]);

    // p = q * (w, 0)
    f32 px = (q.y * w.z + (q.w * w.x + q.x * 0.0f)) - q.z * w.y;
    f32 py = q.z * w.x + (q.y * 0.0f + (q.w * w.y - q.x * w.z));
    f32 pz = q.z * 0.0f + ((q.w * w.z + q.x * w.y) - q.y * w.x);
    f32 pw = ((q.w * 0.0f - q.x * w.x) - q.y * w.y) - q.z * w.z;

    // r = p * conj(q)
    f32 nqx = -q.x;
    f32 nqy = -q.y;
    f32 nqz = -q.z;
    f32 rx = (py * nqz + (pw * nqx + px * q.w)) - pz * nqy;
    f32 ry = pz * nqx + (py * q.w + (pw * nqy - px * nqz));
    f32 rz = (pz * q.w) + ((pw * nqz + px * nqy) - py * nqx);

    f32 ox = rx * scale;
    f32 oy = ry * scale;
    f32 oz = rz * scale;
    f32 p1[3] = { center.x + ox, center.y + oy, center.z + oz };
    f32 p2[3] = { center.x - ox, center.y - oy, center.z - oz };

    u32 c[4];
    for (s32 i = 0; i < 4; i++) {
        s32 v = (s32)(255.0f * arg3[i]);
        if (v < 0) v = 0;
        else if (v > 0xFF) v = 0xFF;
        c[i] = (u32)v;
    }
    fn_801C4B90(arg0, p1, p2, 0, 0, c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24));
}

// 0x801C63E4
// Loads a text file into lbl_805FB700 and parses it line by line into the table at
// lbl_805FB704 (0x10 bytes per line); returns 0 if the file cannot be opened.
s32 fn_801C63E4(const char* path) {
    u8 file[0x60];
    if (fn_80146808(file, path, "rt") == 0) return 0;

    lbl_805FB700 = MemAllocDefault(fn_80146A58(file) + 1);
    s32 off = 0;
    s32 got = fn_80146930(file, lbl_805FB700, fn_80146A58(file));
    ((char*)lbl_805FB700)[got] = 0;
    fn_801C5E30((char*)lbl_805FB700);
    lbl_805FB704 = MemAllocDefault((u32)(lbl_805FB708 * 0x10));

    char* p = (char*)lbl_805FB700;
    while (p != NULL && *p != 0) {
        u8* entry = (u8*)lbl_805FB704 + off;
        off += 0x10;
        char* next = fn_801C5F9C(p, entry);
        p = fn_80338BC0(next, "\n");
        if (p == NULL) p = next + strlen(next);
        if (*p != 0) p++;
    }
    fn_801469FC(file);
    return 1;
}

// 0x801C6724
// Picks the zone sphere (of zone arg2) nearest to the point arg1 and stores its index in
// out[0]; out[1] = zone, out[2..3] cleared. Stops early once the point is inside a sphere.
void fn_801C6724(u8* out, f32* pos, s32 zone) {
    *(u8*)(out + 0) = 0xFF;
    *(u8*)(out + 1) = 0;
    *(u16*)(out + 2) = 0;
    if (zone <= 0) return;
    *(u8*)(out + 1) = (u8)zone;

    f32 best = 1000000.0f;
    u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
    s32 count = *(u8*)(res + 0xC0);
    for (s32 i = 0; i < count; i++) {
        res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
        u8* e = *(u8**)(res + 0xBC) + i * 0x3C;
        f32 dx = pos[0] - *(f32*)(e + 0x4);
        f32 dy = pos[1] - *(f32*)(e + 0x8);
        f32 dz = pos[2] - *(f32*)(e + 0xC);
        f32 dz2 = dz * dz;
        f32 d2 = dz2 + (dx * dx + dy * dy);
        if (d2 > 1.0f && dz2 > 0.5f * d2) continue;
        f32 r = *(f32*)(e + 0x10);
        f32 v = d2 - r * r;
        if (v < best) {
            best = v;
            *(u8*)out = (u8)i;
            if (v < 0.0f) return;
        }
    }
}

// Zone sphere record 0x3C bytes at (zone, idx), or NULL if idx is 0xFF or the zone is not loaded.
static u8* ZoneEntry(u8 idx, u8 zone) {
    if (idx != 0xFF && ZoneIsLoaded(gWorldState, zone)) {
        return *(u8**)(*(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize + 0xBC) + idx * 0x3C;
    }
    return NULL;
}

// One direction of the zone-link search for fn_801C6838: if the link pair (tblA[i], tblB[i])
// connects zone a to zone b, writes out and returns 1.
static s32 LinkPair(u8* out, u8* a, u8* b, u8* def, u32 i, s32 offA, s32 offB) {
    u8* tA = *(u8**)(*(u8**)gWorldState + offA) + i * 4;
    u8* tB = *(u8**)(*(u8**)gWorldState + offB) + i * 4;
    if (a[1] != tA[1] || b[1] != tB[1]) return 0;
    u8* def2 = ZoneEntry(tB[0], tB[1]);
    if (def2 == NULL) return 0;
    if (*(u8*)(*(u8**)(def2 + 0x38) + b[0]) == 0xFF) return 0;
    u8* m = *(u8**)(*(u8**)gWorldState + offA) + i * 4;
    if (a[0] == m[0] && a[1] == m[1]) {
        u8* src = *(u8**)(*(u8**)gWorldState + offB) + i * 4;
        out[0] = src[0];
        out[1] = src[1];
        *(u16*)(out + 2) = *(u16*)(src + 2);
    } else {
        out[1] = a[1];
        out[0] = (*(u8**)(def + 0x38))[(*(u8**)(*(u8**)gWorldState + offA))[i * 4]];
    }
    return 1;
}

// 0x801C6838
// Maps the zone-local sphere index b (key {index, zone}) to the matching index in zone a's
// sphere set, directly if both are in the same zone, otherwise through the zone link table
// (world +0x1C/+0x20, +0x18 entries). out = {index, zone, u16}; index 0xFF if no mapping.
void fn_801C6838(u8* out, u8* a, u8* b) {
    out[1] = 0;
    out[0] = 0xFF;
    *(u16*)(out + 2) = 0;

    u8* def = (a[0] != 0xFF && ZoneIsLoaded(gWorldState, a[1])) ? ZoneEntry(a[0], a[1]) : NULL;
    if (def == NULL) return;

    if (b[1] == a[1]) {
        out[1] = b[1];
        out[0] = (*(u8**)(def + 0x38))[b[0]];
        return;
    }
    for (u32 i = 0; i < *(u32*)(*(u8**)gWorldState + 0x18); i++) {
        u8* t1 = *(u8**)(*(u8**)gWorldState + 0x1C) + i * 4;
        if (a[1] == t1[1] && b[1] == (*(u8**)(*(u8**)gWorldState + 0x20))[i * 4 + 1]) {
            if (LinkPair(out, a, b, def, i, 0x1C, 0x20)) return;
        } else {
            if (LinkPair(out, a, b, def, i, 0x20, 0x1C)) return;
        }
    }
}

// 0x801C76FC
// Builds three object pools (0xAC-byte, 0x80-byte and 0x34-byte elements, each over a caller
// buffer) plus a lookup block sized from the second pool; sets the state at +0x20 to 4.
void fn_801C76FC(u8* arg0, void* buf0, u32 size0, void* buf1, u32 size1, void* buf2, u32 size2) {
    void* p = ObjAlloc(0x2C);
    *(void**)(arg0 + 0x14) = p;
    if (p != NULL) fn_801EB858(p, 0xAC, 0x10, buf0, size0);

    p = ObjAlloc(0x2C);
    *(void**)(arg0 + 0x18) = p;
    if (p != NULL) fn_801EB858(p, 0x80, 0x10, buf1, size1);

    u8* blk = (u8*)ObjAlloc(0x10);
    *(u8**)(arg0 + 0x10) = blk;
    if (blk != NULL) {
        u8* poolA = *(u8**)(arg0 + 0x14);
        *(s32*)(blk + 0x0) = *(s32*)*(u8**)(arg0 + 0x18);
        *(void**)(blk + 0xC) = ObjAlloc((u32)(*(s32*)(blk + 0x0) * 4));
        *(void**)(blk + 0x8) = ObjAlloc((u32)(*(s32*)(blk + 0x0) * 4));
        *(s32*)(blk + 0x4) = 0;
        for (s32 i = 0; i < *(s32*)(blk + 0x0); i++) {
            *(s32*)(*(u8**)(blk + 0x8) + i * 4) = 0;
            void* item = ObjAlloc(8);
            *(void**)(*(u8**)(blk + 0xC) + i * 4) = item;
            if (*(void**)(*(u8**)(blk + 0xC) + i * 4) != NULL) fn_801EBD1C(*(u8**)(*(u8**)(blk + 0xC) + i * 4), poolA);
        }
    }

    p = ObjAlloc(0x2C);
    *(void**)(arg0 + 0xC) = p;
    if (p != NULL) fn_801EB858(p, 0x34, 0x10, buf2, size2);
    *(s32*)(arg0 + 0x20) = 4;
}

// 0x801C7874
// Destructor for the structure built above: releases the three pools and the lookup block.
void fn_801C7874(u8* arg0) {
    if (*(u8**)(arg0 + 0xC) != NULL) {
        fn_801EB748(*(u8**)(arg0 + 0xC));
        BufferFree(*(u8**)(arg0 + 0xC));
        *(u8**)(arg0 + 0xC) = NULL;
    }
    u8* blk = *(u8**)(arg0 + 0x10);
    if (blk != NULL) {
        if (*(u8**)(blk + 0x8) != NULL) {
            BufferFree(*(u8**)(blk + 0x8));
            *(u8**)(blk + 0x8) = NULL;
        }
        if (*(u8**)(blk + 0xC) != NULL) {
            for (s32 i = 0; i < *(s32*)(blk + 0x0); i++) {
                void* item = *(void**)(*(u8**)(blk + 0xC) + i * 4);
                if (item != NULL) BufferFree(item);
            }
            BufferFree(*(u8**)(blk + 0xC));
            *(u8**)(blk + 0xC) = NULL;
        }
        BufferFree(*(u8**)(arg0 + 0x10));
        *(u8**)(arg0 + 0x10) = NULL;
    }
    if (*(u8**)(arg0 + 0x14) != NULL) {
        fn_801EB748(*(u8**)(arg0 + 0x14));
        BufferFree(*(u8**)(arg0 + 0x14));
        *(u8**)(arg0 + 0x14) = NULL;
    }
    if (*(u8**)(arg0 + 0x18) != NULL) {
        fn_801EB748(*(u8**)(arg0 + 0x18));
        BufferFree(*(u8**)(arg0 + 0x18));
        *(u8**)(arg0 + 0x18) = NULL;
    }
    *(s32*)(arg0 + 0x1C) = 0;
}

// Builds the emitter basis for direction d (xyz, normalized here) with the reference axis +Z
// (+Y when d is nearly parallel to Z), and returns the offset  right*c0 + d*c1 + up*c2.
static Vec3 EmitterOffset(const f32* d, f32 c0, f32 c1, f32 c2) {
    Vec3 dn = Vec3Normalized(Vec3{ d[0], d[1], d[2] });
    Vec3 ref = { 0.0f, 0.0f, 1.0f };
    f32 dot = ref.x * dn.x + ref.y * dn.y + ref.z * dn.z;
    f32 dev = (f32)fabs((f64)dot) - 1.0f;
    if (dev > -0.0001f && dev < 0.0001f) ref = Vec3{ 0.0f, 1.0f, 0.0f };
    Vec3 r = { dn.y * ref.z - dn.z * ref.y, dn.z * ref.x - dn.x * ref.z, dn.x * ref.y - dn.y * ref.x };
    Vec3 u = { r.y * dn.z - r.z * dn.y, r.z * dn.x - r.x * dn.z, r.x * dn.y - r.y * dn.x };
    Vec3 o;
    o.x = 0.0f + (u.x * c2 + (r.x * c0 + dn.x * c1));
    o.y = 0.0f + (u.y * c2 + (r.y * c0 + dn.y * c1));
    o.z = 0.0f + (u.z * c2 + (r.z * c0 + dn.z * c1));
    return o;
}

// 0x801C85F0
// Spawns count particle effects along the segment obj+0x10 -> obj+0x20 (xyz + a 4th "w"
// channel), optionally offset by the emitter's local-space offsets in its config (+0x3C..+0x48).
// The count is re-derived from the segment length when the step would be too long.
void fn_801C85F0(u8* obj, s32 count, void* fxMgr) {
    u8* cfg = *(u8**)(obj + 0x5C);
    f32 inv = 1.0f / (f32)count;
    f32 pos[4] = { *(f32*)(obj + 0x10), *(f32*)(obj + 0x14), *(f32*)(obj + 0x18), *(f32*)(obj + 0x1C) };
    f32 step[4];
    for (s32 i = 0; i < 4; i++) step[i] = (*(f32*)(obj + 0x20 + i * 4) - *(f32*)(obj + 0x10 + i * 4)) * inv;

    s32 noOffset = 0;
    if (*(f32*)(cfg + 0x3C) <= 0.0001f && *(f32*)(cfg + 0x3C) >= -0.0001f &&
        *(f32*)(cfg + 0x40) <= 0.0001f && *(f32*)(cfg + 0x40) >= -0.0001f &&
        *(f32*)(cfg + 0x44) <= 0.0001f && *(f32*)(cfg + 0x44) >= -0.0001f &&
        *(f32*)(cfg + 0x48) <= 0.0001f && *(f32*)(cfg + 0x48) >= -0.0001f) {
        noOffset = 1;
    }
    if (!noOffset) {
        f32 c0 = *(f32*)(cfg + 0x3C);
        f32 c1 = *(f32*)(cfg + 0x40);
        f32 c2 = *(f32*)(cfg + 0x44);
        f32 cw = *(f32*)(cfg + 0x48);
        // end point offset along the +0x40 direction
        Vec3 oe = EmitterOffset((f32*)(obj + 0x40), c0, c1, c2);
        f32 end[4] = { oe.x + *(f32*)(obj + 0x20), oe.y + *(f32*)(obj + 0x24),
                       oe.z + *(f32*)(obj + 0x28), cw + *(f32*)(obj + 0x2C) };
        // start point offset along the +0x30 direction
        Vec3 os = EmitterOffset((f32*)(obj + 0x30), c0, c1, c2);
        pos[0] = os.x + *(f32*)(obj + 0x10);
        pos[1] = os.y + *(f32*)(obj + 0x14);
        pos[2] = os.z + *(f32*)(obj + 0x18);
        pos[3] = cw + *(f32*)(obj + 0x1C);
        f32 inv2 = 1.0f / (f32)count;
        step[0] = (end[0] - pos[0]) * inv2;
        step[1] = (end[1] - pos[1]) * inv2;
        step[2] = (end[2] - pos[2]) * inv2;
        step[3] = (end[3] - pos[3]) * inv2;
    }

    cfg = *(u8**)(obj + 0x5C);
    if (step[2] * step[2] + (step[0] * step[0] + step[1] * step[1]) > *(f32*)(cfg + 0x1D0)) {
        f32 dx = *(f32*)(obj + 0x20) - *(f32*)(obj + 0x10);
        f32 dy = *(f32*)(obj + 0x24) - *(f32*)(obj + 0x14);
        f32 dz = *(f32*)(obj + 0x28) - *(f32*)(obj + 0x18);
        f32 dw = *(f32*)(obj + 0x2C) - *(f32*)(obj + 0x1C);
        f32 l2 = dz * dz + (dx * dx + dy * dy);
        f32 len = 0.0f;
        if (l2 > 0.0f) len = sqrtf(l2);  // original: frsqrte with two Newton steps
        count = (s32)(len / *(f32*)(cfg + 0x1CC));
        f32 inv3 = 1.0f / (f32)count;
        step[0] = dx * inv3;
        step[1] = dy * inv3;
        step[2] = dz * inv3;
        step[3] = dw * inv3;
    }

    f32 sx = step[0], sy = step[1], sz = step[2], sw = step[3];
    for (s32 i = 0; i < count; i++) {
        void* item = fn_801EBD2C(*(void**)(obj + 0x60));
        if (item == NULL) {
            s32 c = lbl_805FB718;
            lbl_805FB718 = c + 1;
            if (c % 100 == 0) lbl_805FB718 = 0;
            return;
        }
        lbl_805FB718 = 0;
        fn_801CEC68(item, obj, pos, fxMgr);
        *(s32*)(obj + 0x64) += 1;
        pos[0] = pos[0] + sx;
        pos[1] = pos[1] + sy;
        pos[2] = pos[2] + sz;
        pos[3] = pos[3] + sw;
    }
}

}  // extern "C"
