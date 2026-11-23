// Drafted functions in 0x800D..0x8012: spline segment builders, ring-buffer helpers, misc.
#include "types.h"
#include "engine/math.h"
#include "game/character.h"
#include <math.h>
#include <string.h>

struct StringTable;  // layout: strings pointer at +0, count at +0xC
extern "C++" StringTable* gStringTable;  // 0x805FB910

extern "C" {
void* MemAllocDefault(u32 size);  // 0x801D4650
extern f32 lbl_805FE794;  // initial "distance" (large)
extern f32 lbl_805FE79C;  // 1.0f
extern f32 lbl_805FEB04;  // 1.0f
extern f32 lbl_805FEB0C;  // 0.0f
void QuatGetAxisXY(const Quat* q, Vec3* out);  // 0x801453BC
Vec3 fn_80124610(u8* a, f32* b);               // 0x80124610

void CharacterSetInCombat(Character* c, BOOL on);  // 0x8004D2B8
void WorldPreUpdate(void* world);
void fn_8003DCE0(u8* p);
void fn_8004C718(u8* p, s32 idx, void* elem);
void fn_8004D760(u8* c, u8* monster);
void* fn_8004DAF8(void* c, s32 a, s32 b);
void fn_8004DCEC(u8* p, s32 idx);
u8* fn_8004EDD0(u8* obj, s32 flag, s32 a2);
void* fn_8004FFB4(void* id, void* part);
s32 fn_801A3010(void* obj, void* data);  // 0x801A3010
extern u8 gWorldState[];
extern s32 lbl_803C4E28[];
extern s32 lbl_805FB034;
extern s32 lbl_805FB038;
extern u8* lbl_805FBBE0;

s32 TextIdFromHash(u32 hash);  // 0x80120D34
int SysSprintf(char* buf, const char* fmt, ...);  // 0x80142EFC
u32 fn_80143974(s32 seed, void* data, u32 len);
u32 fn_8022CD34(u8* s, s32 idx);  // 0x8022CD34
void UIText_SetString(u32 hash, const char* text);  // 0x80190E48
void UIText_SetString_Int(u32 hash, s32 value);
void UIElement_SetVisibleFlag(u32 hash, BOOL on);
extern void* gStringTable2;
extern u8* gGameDb;
extern const char lbl_80354800[];  // "Not found"
extern const char lbl_8035480C[];  // "NOT FOUND"
extern const char lbl_805F8200[];  // ""
extern const char lbl_805F8F80[];  // slot name format
extern const char lbl_8036D290[];  // "min:sec" time format

typedef void (*VCamFn213)(u8* self);
typedef void (*VMtxFn213)(u8* self, f32* out);
typedef void* (*VObjFn213)(u8* self);

u32 WasButtonPressed(u32 button);
u32 WasEntityDrawnLastFrame(void* e);
void* fn_800E60BC(void);
u8* fn_800E5684(u8* p);
u8* fn_800E4D7C(u32 a0);
void fn_8016F854(f32* out, void* view, void* pos);
void fn_8018D68C(f32* out, void* sets);
void fn_800DB370(void* cam, f32* a, f32* b, f32* c);
void* fn_801DA758(void* v);
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void WorldSendEvent(void* world, s32 id, int a, int b, int c);  // 0x800D87EC
void CameraSetMatrix(void* camera, const f32 mtx[3][4]);  // 0x801D6814
void fn_80009840(u8* p);
void WorldResetQueues(void* world);
void* SirLoad(void* loader, const char* path, void* dst, u32 size, s32 flags);  // 0x801E6138
BOOL fn_801E6BD0(u8* obj, u32* handle);
u32 fn_801D3A84(s32 n);
u8* GetGlobalSettings(void);
void SessionBegin(void* session);
BOOL GetOptionsBrightness(void);
void SetOptionsBrightnessTmp(BOOL v);
u8* fn_8015EC9C(u8* tbl, f32 a, f32 b);
void fn_80107120(void);
void fn_800D8604(void* world);
void fn_8013006C(u8* w);
void fn_80132FE4(u8* w);
void fn_801F5414(u8* p);
BOOL fn_801F5220(void);
u32 fn_80129928(void);
void fn_8012A790(void* dst, void* src);
void fn_80129CC8(u8* p);
void* fn_8012F62C(void* tbl, s32 idx);
void* fn_8012F63C(void* tbl, s32 idx);
u8* fn_8012A748(u8* g);
u8* fn_8012A754(u8* w);
void fn_8012A75C(u8* p, u32 bits);
void fn_8012A76C(u8* p, u32 bits);
void fn_8012A77C(u32 a, u32 b);
u8* fn_8012A780(u8* g);
void fn_8012A79C(u8* a, u8* b);
void fn_8012847C(u8* o, u32 flags);
void fn_8012CDF8(u8* p);
extern "C++" void* gScreenSets;  // 0x805FB654
extern "C++" u32* gZoneBase;     // 0x803C7434
extern "C++" void* gEventMgr;
extern u8 gGame[];
extern u8 gIdPool[];
extern s32 gScreenWidth;
extern s32 gScreenHeight;
extern u8 gFlag805FB948;
extern f32 lbl_805FEE98;  // 0.0f
extern f32 lbl_805FEE9C;  // 0.5f
extern f32 lbl_805FEEA0;  // 1.0f
extern u8 lbl_803C4298[];
extern u8 lbl_8047C640[];
extern u8 lbl_8040D500[];
extern void* lbl_805FB5F8;
extern u8 lbl_805FB1A1;
extern s8 lbl_805FB210;
extern s32 lbl_805F95F8;
extern u8 lbl_805FB2F3;
extern "C++" u8 sWorldReady;       // 0x805FB2F0
extern "C++" u8 sReadyPending;     // 0x805FB2F1 (static in world_update.cpp)
extern "C++" u8 sReadyRequested;   // 0x805FB2F2 (static in world_update.cpp)
extern "C++" u8 sWorldSuspended;   // 0x805FB2F4 (static in world_update.cpp)
extern "C++" u32 sLoadFrames;      // 0x805FB2F8 (static in world_update.cpp)
extern const char lbl_805F8FEC[];
extern const char lbl_805F8FF4[];
static const u32 kHashMechanist213 = 0xA2C2A850u;  // "mechanist"

s32 fn_801C0830(u8* zone, u8* box, u8* pos);
void RayQueryInit(u8* q);  // 0x801B11DC
extern "C++" s32 gZoneResourceSize;  // 0x805FB94C
extern u8 lbl_80383A88[];  // vtable
extern u8 lbl_80383A38[];  // vtable
extern u8 lbl_80383B18[];  // vtable
extern u8 lbl_803839F0[];  // vtable
extern u8 lbl_80383AD0[];  // vtable

typedef void (*VCall213)(u8* self, s32 arg);

static f32 Sqrt213(f32 v) { return v <= 0.0f ? 0.0f : sqrtf(v); }

// Ring buffer of 0x1C-byte spline segments at owner+0xAF8: flag at +0, tail at +8, head at +0xC,
// capacity at +0x10, data pointer at +0x14.
static u8* Ring213Entry(u8* o, s32 idx) {
    s32 cap = *(s32*)(o + 0xB08);
    if (idx < 0 || !(idx < cap)) return NULL;
    s32 lo = *(s32*)(o + 0xB00);
    s32 hi = idx;
    s32 x = idx;
    if (idx < lo) {
        x = idx + cap;
        hi = idx + cap;
    }
    if (lo > x || x > hi) return NULL;
    return *(u8**)(o + 0xB0C) + idx * 0x1C;
}

// Appends a segment slot: advances the head, returns the new slot (NULL when the ring is full).
static u8* Ring213Push(u8* o) {
    s32 tail = *(s32*)(o + 0xB00);
    s32 nh = (*(s32*)(o + 0xB04) + 1) % *(s32*)(o + 0xB08);
    if (tail == nh) return NULL;
    *(s32*)(o + 0xB04) = nh;
    if (tail < 0) {
        *(s32*)(o + 0xB00) = nh;
        *(s32*)(o + 0xAF8) = 1;
    }
    return Ring213Entry(o, *(s32*)(o + 0xB04));
}

// 0x8010B2D8
// Builds the segment array of a path: for each pair of consecutive 0x28-byte points, stores the
// start position, the delta to the next point and the delta length; sums the lengths at +8.
void fn_8010B2D8(u8* arg0, u8* arg1) {
    s32 n = *(s32*)(arg1 + 0x24) - 1;
    *(s32*)(arg0 + 0x4) = n;
    *(void**)(arg0 + 0x0) = MemAllocDefault(n * 0x1C);
    *(f32*)(arg0 + 0x8) = 0.0f;
    s32 off = 0;
    for (s32 i = 0; i < *(s32*)(arg1 + 0x24) - 1; i++) {
        u8* p = *(u8**)arg1 + i * 0x28;
        u8* out = *(u8**)arg0 + off;
        *(f32*)(out + 0x0) = *(f32*)(p + 0xC);
        *(f32*)(out + 0x4) = *(f32*)(p + 0x10);
        *(f32*)(out + 0x8) = *(f32*)(p + 0x14);
        p = *(u8**)arg1 + i * 0x28;
        out = *(u8**)arg0 + off;
        f32 dx = *(f32*)(p + 0x34) - *(f32*)(p + 0xC);
        f32 dy = *(f32*)(p + 0x38) - *(f32*)(p + 0x10);
        f32 dz = *(f32*)(p + 0x3C) - *(f32*)(p + 0x14);
        *(f32*)(out + 0xC) = dx;
        *(f32*)(out + 0x10) = dy;
        *(f32*)(out + 0x14) = dz;
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 len = (len2 <= 0.0f) ? 0.0f : sqrtf(len2);
        *(f32*)(*(u8**)arg0 + off + 0x18) = len;
        *(f32*)(arg0 + 0x8) = *(f32*)(arg0 + 0x8) + *(f32*)(*(u8**)arg0 + off + 0x18);
        off += 0x1C;
    }
}

// 0x8010BB64
// Appends one segment per point pair of arg1 to the ring buffer in arg0: start position is the
// newest ring segment's position advanced by its direction scaled len/len, delta is the point
// delta rotated by the 3x3 matrix arg2 (columns summed), then its length at +0x18.
void fn_8010BB64(u8* arg0, u8* arg1, u8* arg2) {
    s32 off = 0;
    s32 n = *(s32*)(arg1 + 0x24) - 1;
    if (n <= 0) return;
    do {
        u8* cur = Ring213Entry(arg0, *(s32*)(arg0 + 0xB04));
        u8* out = Ring213Push(arg0);
        f32 len = *(f32*)(cur + 0x18);
        f32 s = len / len;
        *(f32*)(out + 0x0) = *(f32*)(cur + 0x0) + *(f32*)(cur + 0xC) * s;
        *(f32*)(out + 0x4) = *(f32*)(cur + 0x4) + *(f32*)(cur + 0x10) * s;
        *(f32*)(out + 0x8) = *(f32*)(cur + 0x8) + *(f32*)(cur + 0x14) * s;
        u8* p = *(u8**)arg1 + off;
        f32 dx = *(f32*)(p + 0x34) - *(f32*)(p + 0xC);
        f32 dy = *(f32*)(p + 0x38) - *(f32*)(p + 0x10);
        f32 dz = *(f32*)(p + 0x3C) - *(f32*)(p + 0x14);
        *(f32*)(out + 0xC) = *(f32*)(arg2 + 0x18) * dz + (*(f32*)(arg2 + 0x0) * dx + *(f32*)(arg2 + 0xC) * dy);
        *(f32*)(out + 0x10) = *(f32*)(arg2 + 0x1C) * dz + (*(f32*)(arg2 + 0x4) * dx + *(f32*)(arg2 + 0x10) * dy);
        *(f32*)(out + 0x14) = *(f32*)(arg2 + 0x20) * dz + (*(f32*)(arg2 + 0x8) * dx + *(f32*)(arg2 + 0x14) * dy);
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        *(f32*)(out + 0x18) = (len2 <= 0.0f) ? 0.0f : sqrtf(len2);
        off += 0x28;
    } while (--n != 0);
}

// 0x800DE780
// Point-vs-tapered-capsule test: segment a..b with radius lerping from r+s0 to r+s1. Finds the
// closest point on the segment to p; if p is inside the radius there, writes out[1] = distance and
// out[0] = radius - distance (penetration), out[2..4] = segment direction. The end caps are
// tested too (the larger-radius end first). Returns 0 for a degenerate segment, else 1.
s32 fn_800DE780(u8* self, void* p_, f32* a, f32* b, f32* out, f32 r, f32 s0, f32 s1) {
    f32* p = (f32*)p_;
    f32 lo = s0 + r;
    f32 hi = s1 + r;
    out[1] = lbl_805FE794;
    f32 dx = b[0] - a[0];
    f32 dy = b[1] - a[1];
    f32 dz = b[2] - a[2];
    f32 wx = p[0] - a[0];
    f32 wy = p[1] - a[1];
    f32 wz = p[2] - a[2];
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    if (len2 == 0.0f) return 0;
    out[2] = dx;
    out[3] = dy;
    out[4] = dz;
    f32 t = (wz * dz + (wx * dx + wy * dy)) / len2;
    f32 rad;
    f32 cx, cy, cz;
    if (t < 0.0f) {
        rad = lo;
        cx = a[0];
        cy = a[1];
        cz = a[2];
    } else if (t > lbl_805FE79C) {
        rad = hi;
        cx = b[0];
        cy = b[1];
        cz = b[2];
    } else {
        cx = a[0] + dx * t;
        cy = a[1] + dy * t;
        cz = a[2] + dz * t;
        rad = lo + (hi - lo) * t;
    }
    f32 qx = p[0] - cx;
    f32 qy = p[1] - cy;
    f32 qz = p[2] - cz;
    f32 q2 = qz * qz + (qx * qx + qy * qy);
    if (q2 < rad * rad && q2 < out[1] * out[1]) {
        f32 d = Sqrt213(q2);
        out[1] = d;
        out[0] = rad - d;
    }
    if (lo > hi) {
        f32 vx = a[0] - p[0];
        f32 vy = a[1] - p[1];
        f32 vz = a[2] - p[2];
        f32 k = r + lo;
        if ((vz * vz + (vx * vx + vy * vy)) - k * k <= 0.0f) {
            f32 ex = p[0] - a[0];
            f32 ey = p[1] - a[1];
            f32 ez = p[2] - a[2];
            f32 d = Sqrt213(ez * ez + (ex * ex + ey * ey));
            if (d < out[1]) {
                out[1] = d;
                out[0] = lo - d;
            }
        }
    } else {
        f32 vx = b[0] - p[0];
        f32 vy = b[1] - p[1];
        f32 vz = b[2] - p[2];
        f32 k = r + hi;
        if ((vz * vz + (vx * vx + vy * vy)) - k * k <= 0.0f) {
            f32 ex = p[0] - b[0];
            f32 ey = p[1] - b[1];
            f32 ez = p[2] - b[2];
            f32 d = Sqrt213(ez * ez + (ex * ex + ey * ey));
            if (d < out[1]) {
                out[1] = d;
                out[0] = hi - d;
            }
        }
    }
    return 1;
}

// 0x800E9E54
// Computes the launch direction for a projectile hash into out (default: +Y-ish constant, or the
// location's offset for hash 0). Special hashes derive it from the owner's aim axis, from the
// target object at +0xA14, or the owner's Y axis (optionally re-oriented by fn_80124610).
void fn_800E9E54(f32* out, u8* ent, u8* loc, u32 hash) {
    f32 zero = lbl_805FEB0C;
    out[2] = zero;
    out[0] = zero;
    out[1] = lbl_805FEB04;
    switch (hash) {
    case 0:
    case 0xB8875711u:
        out[0] = *(f32*)(loc + 0xC);
        out[1] = *(f32*)(loc + 0x10);
        out[2] = *(f32*)(loc + 0x14);
        break;
    case 0x0CFEF9E8u:
        if (*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x40) {
            QuatGetAxisXY((const Quat*)(ent + 0x17C), (Vec3*)out);
            *(u8*)(ent + 0x190) += 1;
        }
        break;
    case 0xC2C3D0ACu:
        if ((*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x40) && *(s32*)(ent + 0x18C) != 0) {
            QuatGetAxisXY((const Quat*)(ent + 0x17C), (Vec3*)out);
            out[2] = zero;
            Vec3 n = Vec3Normalized(Vec3{out[0], out[1], out[2]});
            out[0] = n.x;
            out[1] = n.y;
            out[2] = lbl_805FEB04;
            Vec3 m = Vec3Normalized(Vec3{out[0], out[1], out[2]});
            out[0] = m.x;
            out[1] = m.y;
            out[2] = m.z;
            *(u8*)(ent + 0x190) += 1;
        }
        break;
    case 0x00C12F4Au:
        if (*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x20) {
            u8* t = *(u8**)(ent + 0xA14);
            if (t != NULL) {
                f32 vx = *(f32*)(t + 0x18) - *(f32*)(loc + 0x24);
                f32 vy = *(f32*)(t + 0x1C) - *(f32*)(loc + 0x28);
                f32 vz = *(f32*)(t + 0x20) - *(f32*)(loc + 0x2C);
                out[0] = vx;
                out[1] = vy;
                out[2] = vz;
                Vec3 n = Vec3Normalized(Vec3{vx, vy, vz});
                out[0] = n.x;
                out[1] = n.y;
                out[2] = n.z;
            }
        }
        break;
    case 0xCEFC060Eu:
        if (*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x20) {
            u8* t = *(u8**)(ent + 0xA14);
            if (t != NULL) {
                u8* q = *(u8**)(t + 4);
                f32 vx = *(f32*)(q + 0) - *(f32*)(loc + 0x24);
                f32 vy = *(f32*)(q + 4) - *(f32*)(loc + 0x28);
                f32 vz = *(f32*)(q + 8) - *(f32*)(loc + 0x2C);
                (void)vz;
                out[0] = vx;
                out[1] = vy;
                out[2] = zero;
                Vec3 n = Vec3Normalized(Vec3{vx, vy, zero});
                out[0] = n.x;
                out[1] = n.y;
                out[2] = lbl_805FEB04;
                Vec3 m = Vec3Normalized(Vec3{out[0], out[1], out[2]});
                out[0] = m.x;
                out[1] = m.y;
                out[2] = m.z;
            }
        }
        break;
    case 0xC9323448u: {
        Vec3 a = QuatGetAxisY((const Quat*)(*(u8**)(ent + 4) + 0xC));
        out[0] = a.x;
        out[1] = a.y;
        out[2] = a.z;
        break;
    }
    case 0x2D6C64E5u: {
        Vec3 a = QuatGetAxisY((const Quat*)(*(u8**)(ent + 4) + 0xC));
        out[0] = a.x;
        out[1] = a.y;
        out[2] = a.z;
        if (*(u32*)(*(u8**)(ent + 4) + 0x1C) & 0x10) {
            Vec3 b = fn_80124610(ent + 0x7A4, out);
            out[0] = b.x;
            out[1] = b.y;
            out[2] = b.z;
        }
        break;
    }
    default:
        break;
    }
}

// 0x80117430
// Per-frame step of the character-swap sequence (owner at +0x28 holds up to 5 limb slots). Once
// all swap flags are set and no slot is mid-load (ids at +0xC6C.. are 0 or >= 1000), detaches the
// limbs, picks the new monster record and sends the owner event 0x1A. When every flag is set the
// limbs are re-created from the loaded ids, combat mode is entered and the flags are cleared.
void fn_80117430(u8* self) {
    if (*(s32*)(self + 0x38) == 0 && *(s32*)(self + 0x3C) == 0) {
        s32 ok = 1;
        for (s32 i = 0; i < lbl_805FB034; i++) {
            if (lbl_803C4E28[i] == 0) ok = 0;
        }
        if (ok != 0) {
            u8* ch = *(u8**)(self + 0x28);
            for (s32 i = 0; i < 5; i++) {
                u32 v = *(u32*)(ch + 0xC6C + i * 4);
                if (v != 0 && v < 0x3E8u) ok = 0;
            }
        }
        if (ok != 0) {
            u8* ui = *(u8**)(self + 0x24);
            if (ui != NULL) {
                fn_801A3010(ui, NULL);
                ui = *(u8**)(self + 0x24);
                (*(VCall213*)(*(u8**)(ui + 0x138) + 0x30))(ui, 0x1A);
            }
            for (s32 i = 0; i < 5; i++) {
                u8* limb = *(u8**)(*(u8**)(self + 0x28) + i * 4 + 0xC50);
                fn_8004C718(*(u8**)(self + 0x28), i, NULL);
                fn_8003DCE0(limb);
            }
            u8* mon = *(u8**)(lbl_805FBBE0 + *(s32*)(self + 0x2C) * 4 + 0xA30);
            *(u8**)(self + 0x20) = mon;
            if (mon != NULL) {
                fn_8004D760(*(u8**)(self + 0x28), mon);
                fn_8004DAF8(*(u8**)(self + 0x28), 1, 0);
                lbl_805FB038 = 5;
            }
            *(s32*)(self + 0x30) = *(s32*)(self + 0x2C);
        }
    }
    if (*(u8**)(self + 0x24) == NULL) return;
    WorldPreUpdate(gWorldState);
    for (s32 i = 0; i < lbl_805FB034; i++) {
        if (lbl_803C4E28[i] == 0) return;
    }
    for (s32 i = 0; i < 5; i++) {
        fn_8004DCEC(*(u8**)(self + 0x28), i);
        u8* ch = *(u8**)(self + 0x28);
        void* id = *(void**)(ch + i * 4 + 0xC6C);
        if (id != NULL) {
            u8* part = (u8*)fn_8004FFB4(id, *(void**)(*(u8**)(self + 0x20) + i * 4 + 4));
            if (part != NULL) {
                fn_8004C718(*(u8**)(self + 0x28), i, NULL);
                fn_8004C718(*(u8**)(self + 0x28), i, part);
                u8* limb = *(u8**)(*(u8**)(self + 0x28) + i * 4 + 0xC50);
                if (limb != NULL) {
                    fn_8004EDD0(limb, 0, 0);
                    limb = *(u8**)(*(u8**)(self + 0x28) + i * 4 + 0xC50);
                    *(u32*)(limb + 0x15C) |= 0x400000;
                }
            }
            *(void**)(*(u8**)(self + 0x28) + i * 4 + 0xC6C) = NULL;
        }
    }
    if (*(s32*)(self + 0x38) == 0) {
        u8* ui = *(u8**)(self + 0x24);
        if (ui != NULL) {
            fn_801A3010(ui, *(u8**)(self + 0x28));
            ui = *(u8**)(self + 0x24);
            (*(VCall213*)(*(u8**)(ui + 0x138) + 0x2C))(ui, 0x18);
            *(s32*)(self + 0x50) = 3;
        }
        CharacterSetInCombat(*(Character**)(self + 0x28), 1);
    }
    if (*(s32*)(self + 0x50) > 0) {
        s32 n = *(s32*)(self + 0x50) - 1;
        *(s32*)(self + 0x50) = n;
        if (n == 0) {
            u8* ui = *(u8**)(self + 0x24);
            (*(VCall213*)(*(u8**)(ui + 0x138) + 0x2C))(ui, 2);
        }
    }
    for (s32 i = 0; i < lbl_805FB034; i++) lbl_803C4E28[i] = 0;
    lbl_805FB034 = 0;
    if (*(s32*)(self + 0x2C) == *(s32*)(self + 0x30)) {
        *(s32*)(self + 0x38) = 1;
        lbl_805FB038 = 0;
    }
}

// Name/value pair gathered for the records screens.
struct Pair213 {
    const char* name;
    s32 val;
};

// String for a text id (same lookup as UIText_SetString_TextID): high bit selects the second table.
static const char* TextFor213(s32 id) {
    if (id == -1) return lbl_80354800;
    if (((u32)id & 0x80000000u) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** arr = *(const char***)(t + 0x0);
        u32 idx = (u32)id & 0x7FFFFFFFu;
        if (arr != NULL && idx < *(u32*)(t + 0xC)) return arr[idx];
        return lbl_805F8200;
    }
    u8* t = (u8*)gStringTable;
    const char** strings = *(const char***)t;
    if (strings != NULL && (u32)id < *(u32*)(t + 0xC)) return strings[(u32)id];
    return lbl_805F8200;
}

static void Swap213(Pair213* a, Pair213* b) {
    Pair213 t = *a;
    *a = *b;
    *b = t;
}

// Sorts n pairs by name (strcmp, ascending): median-of-3 quicksort on ranges larger than 4 with an
// explicit range stack, then a min-of-first-5 pass and an insertion pass (inlined std::sort).
static void SortPairs213(Pair213* base, s32 n) {
    if (n <= 1) return;
    if ((u32)n > 4u) {
        Pair213* lo = base;
        Pair213* hi = base + n - 1;
        Pair213* stkLo[32];
        Pair213* stkHi[32];
        s32 d = 0;
        for (;;) {
            Pair213* mid = lo + ((hi - lo) >> 1);
            if (strcmp(mid->name, lo->name) < 0) Swap213(mid, lo);
            if (strcmp(hi->name, mid->name) < 0) {
                Swap213(hi, mid);
                if (strcmp(mid->name, lo->name) < 0) Swap213(mid, lo);
            }
            Pair213* l = lo + 1;
            Pair213* r = hi - 1;
            do {
                while (strcmp(l->name, mid->name) < 0) l++;
                while (strcmp(mid->name, r->name) < 0) r--;
                if (l < r) {
                    Swap213(l, r);
                    if (mid == l) mid = r;
                    else if (mid == r) mid = l;
                    l++;
                    r--;
                } else if (l == r) {
                    l++;
                    r--;
                    break;
                }
            } while (l <= r);
            s32 leftN = (s32)(r - lo);
            s32 rightN = (s32)(hi - l);
            if (leftN > 4) {
                if (rightN <= 4) {
                    hi = r;
                } else if (leftN > rightN) {
                    stkLo[d] = lo;
                    stkHi[d] = r;
                    d++;
                    lo = l;
                } else {
                    stkLo[d] = l;
                    stkHi[d] = hi;
                    d++;
                    hi = r;
                }
            } else if (rightN <= 4) {
                if (d == 0) break;
                d--;
                lo = stkLo[d];
                hi = stkHi[d];
            } else {
                lo = l;
            }
        }
    }
    Pair213* last = base + n - 1;
    Pair213* lim = base + 4;
    if (lim > last) lim = last;
    Pair213* min = base;
    for (Pair213* p = base + 1; p <= lim; p++) {
        if (strcmp(p->name, min->name) < 0) min = p;
    }
    if (min != base) Swap213(min, base);
    for (Pair213* p = base + 1; p <= last; p++) {
        const char* key = p->name;
        Pair213* q = p - 1;
        while (strcmp(key, q->name) < 0) q--;
        q++;
        if (q != p) {
            Pair213 k = *p;
            for (Pair213* m = p; m > q; m--) *m = *(m - 1);
            *q = k;
        }
    }
}

static const s32 kSeedA213 = (s32)0xF9F39258u;
static const s32 kSeedB213 = (s32)0xEA239A84u;

// 0x8011D9C0
// Fills the 10 record rows (name + time as m:ss): collects the unlocked entries (flags at
// save+0x9A8) with their names, sorts them by name, then writes each slot's name and "min:sec"
// text, hiding the unused slots.
void fn_8011D9C0(u8* self) {  // self unused
    Pair213 pairs[10];
    s32 n = 0;
    for (s32 i = 0; i < 10; i++) {
        s32 v = *(s32*)(lbl_805FBBE0 + i * 4 + 0x9A8);
        if (v != 0) {
            u8* recs = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x80E0);
            u32 hash = *(u32*)(recs + i * 0x4C + 4);
            pairs[n].name = TextFor213(TextIdFromHash(hash));
            pairs[n].val = v;
            n++;
        }
    }
    SortPairs213(pairs, n);
    for (s32 i = 0; i < 10; i++) {
        char buf[0x40];
        s32 len = SysSprintf(buf, lbl_805F8F80, i + 1);
        u32 h1 = fn_80143974(kSeedA213, buf, len);
        u32 h2 = fn_80143974(kSeedB213, buf, len);
        if (i < n) {
            UIText_SetString(h1, pairs[i].name);
            u32 t = (u32)pairs[i].val;
            SysSprintf(buf, lbl_8036D290, t / 60, t % 60);
            UIText_SetString(h2, buf);
        } else {
            UIElement_SetVisibleFlag(h1, 0);
            UIElement_SetVisibleFlag(h2, 0);
        }
    }
}

// 0x8011DF20
// Fills the 10 score rows of the current page (self+0x20 == 1 selects the second page): collects
// the 12 owned entries with their names, sorts them by name and writes each slot's name and count.
void fn_8011DF20(u8* self) {
    Pair213 pairs[12];
    s32 n = 0;
    for (s32 i = 0; i < 12; i++) {
        s32 v = (s32)fn_8022CD34(lbl_805FBBE0, i);
        if (v != 0) {
            u8* recs = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x3E60);
            u32 hash = *(u32*)(recs + i * 0xC + 4);
            pairs[n].name = TextFor213(TextIdFromHash(hash));
            pairs[n].val = v;
            n++;
        }
    }
    SortPairs213(pairs, n);
    s32 start = (*(s32*)(self + 0x20) == 1) ? 10 : 0;
    for (s32 j = 0; j < 10; j++) {
        char buf[0x40];
        s32 len = SysSprintf(buf, lbl_805F8F80, j + 1);
        u32 h1 = fn_80143974(kSeedA213, buf, len);
        u32 h2 = fn_80143974(kSeedB213, buf, len);
        if (start + j < n) {
            UIText_SetString(h1, pairs[start + j].name);
            UIText_SetString_Int(h2, pairs[start + j].val);
        } else {
            UIElement_SetVisibleFlag(h1, 0);
            UIElement_SetVisibleFlag(h2, 0);
        }
    }
}

// 0x8011E440
// Same as fn_8011DF20 for the 6 flagged entries (flags at save+0xA04, names at +0x7040): sorted by
// name, written to the first 10 slots as name + count, unused slots hidden.
void fn_8011E440(u8* self) {  // self unused
    Pair213 pairs[6];
    s32 n = 0;
    for (s32 i = 0; i < 6; i++) {
        s32 v = *(s32*)(lbl_805FBBE0 + i * 4 + 0xA04);
        if (v != 0) {
            u8* recs = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x7040);
            u32 hash = *(u32*)(recs + i * 8 + 4);
            pairs[n].name = TextFor213(TextIdFromHash(hash));
            pairs[n].val = v;
            n++;
        }
    }
    SortPairs213(pairs, n);
    for (s32 i = 0; i < 10; i++) {
        char buf[0x40];
        s32 len = SysSprintf(buf, lbl_805F8F80, i + 1);
        u32 h1 = fn_80143974(kSeedA213, buf, len);
        u32 h2 = fn_80143974(kSeedB213, buf, len);
        if (i < n) {
            UIText_SetString(h1, pairs[i].name);
            UIText_SetString_Int(h2, pairs[i].val);
        } else {
            UIElement_SetVisibleFlag(h1, 0);
            UIElement_SetVisibleFlag(h2, 0);
        }
    }
}


// 0x80125970
// Per-frame target picker for the lab scan reticle: restores the camera/scan state once, then
// every `self+0x140` frames projects each candidate entity (count at +0x100, pointers at +0)
// to the screen and picks the one nearest the screen centre; sends "deselect" / "select" events
// when the picked index (+0x104, 0x40 = none) changes. Button 1 confirms the current pick.
void fn_80125970(u8* self, f32 dt) {  // dt unused
    if (*(u8*)(self + 0x10C) != 0 && lbl_805FB1A1 == 0) {
        *(u8*)(self + 0x10C) = 0;
        u8* a = (u8*)fn_800E60BC();
        if (a != NULL) {
            *(u8*)(a + 0x54) = 1;
            fn_800E5684(a);
            u8* b = fn_800E4D7C(0);
            if (b != NULL) *(u8*)(b + 0x54) = 0;
            u8* cam = *(u8**)(lbl_803C4298 + 4);
            (*(VCamFn213*)(*(u8**)cam + 0x10))(cam);
        }
    }
    u8* p = fn_800E4D7C(0);
    if (p != NULL && *(u8*)(p + 0x54) != 0) return;
    u8* q = (u8*)fn_800E60BC();
    if (q != NULL && *(u8*)(q + 0xA4) != 0) return;

    if (*(s32*)(self + 0x104) < 0x40 && WasButtonPressed(1) != 0) {
        *(s32*)(lbl_805FBBE0 + 0x100) = 0;
        *(s32*)(self + 0x108) = *(s32*)(*(u8**)(self + *(s32*)(self + 0x104) * 4) + 0x2C);
        WorldSendEvent(gGame + 0xD5F0, (s32)0xAE2DA05Du, 0, 0, 1);
        if (*(s32*)(lbl_805FBBE0 + 0x100) == 0) {
            EventSend(gEventMgr, *(u8**)(self + *(s32*)(self + 0x104) * 4), NULL, 0x6D2D1C4Fu, 0, 0, 1, 1);
            *(s32*)(self + 0x108) = 0;
        }
        *(s32*)(self + 0x104) = 0x40;
        return;
    }
    s32 t = *(s32*)(self + 0x144) + 1;
    *(s32*)(self + 0x144) = t;
    if (t < *(s32*)(self + 0x140)) return;
    *(s32*)(self + 0x144) = 0;

    f32 mtx[3][4];
    u8* cam = *(u8**)(lbl_803C4298 + 4);
    (*(VMtxFn213*)(*(u8**)cam + 0x38))(cam, &mtx[0][0]);
    CameraSetMatrix(lbl_8047C640, mtx);
    fn_80009840(lbl_803C4298);
    void* view = lbl_805FB5F8;
    f32 center[3];   // fn_8018D68C writes x, y, z
    fn_8018D68C(center, gScreenSets);

    f32 bestD = lbl_805FEE98;
    s32 best = -1;
    for (s32 i = 0; i < *(s32*)(self + 0x100); i++) {
        u8* e = *(u8**)(self + i * 4);
        if (WasEntityDrawnLastFrame(e) == 0) continue;
        if ((*(u32*)(*(u8**)(e + 4) + 0x20) & 2) == 0) continue;
        u8* o = (u8*)fn_801DA758((*(VObjFn213*)(*(u8**)e + 0xC))(e));
        f32 rad = *(f32*)(o + 0x20);
        f32 pos[3] = {*(f32*)(o + 0x14), *(f32*)(o + 0x18), *(f32*)(o + 0x1C)};
        if (*(u32*)(*(u8**)(e + 0x9C) + 8) == kHashMechanist213) {
            pos[0] = *(f32*)(e + 0x18);
            pos[1] = *(f32*)(e + 0x1C);
            pos[2] = lbl_805FEE9C + *(f32*)(e + 0x20);
            rad = lbl_805FEEA0;
        }
        f32 sp[2];
        fn_8016F854(sp, view, pos);
        if (sp[0] < lbl_805FEE98) continue;
        if (sp[1] < lbl_805FEE98) continue;
        if (sp[0] > lbl_805FEEA0) continue;
        if (sp[1] > lbl_805FEEA0) continue;
        f32 va[3], vb[3], dir[3];
        fn_800DB370(*(u8**)(lbl_803C4298 + 4), va, vb, dir);
        Vec3 n = Vec3Normalized(Vec3{dir[0], dir[1], dir[2]});
        f32 off[3] = {n.x * rad, n.y * rad, n.z * rad};
        f32 pos2[3] = {pos[0] + off[0], pos[1] + off[1], pos[2] + off[2]};
        f32 sp2[2];
        fn_8016F854(sp2, view, pos2);
        f32 w = (f32)gScreenWidth;
        f32 h = (f32)gScreenHeight;
        f32 sx = w * sp[0];
        f32 dx = sx - center[0];
        f32 dy = h * sp[1] - center[1];
        f32 dist = dx * dx + dy * dy;
        f32 rpx = fabsf(sx - w * sp2[0]);
        if (rpx * rpx >= dist) {
            if (best == -1) {
                bestD = dist;
                best = i;
            } else if (dist < bestD) {
                best = i;
                bestD = dist;
            }
        }
    }
    s32 cur = *(s32*)(self + 0x104);
    if (best != -1) {
        if (cur == best) return;
        if (cur < 0x40) {
            EventSend(gEventMgr, *(u8**)(self + cur * 4), NULL, 0x7E57AD29u, 0, 0, 1, 1);
            *(s32*)(self + 0x104) = 0x40;
        } else {
            *(s32*)(self + 0x104) = best;
            EventSend(gEventMgr, *(u8**)(self + best * 4), NULL, 0x28BA6AC7u, 0, 0, 0, 1);
        }
    } else if (cur < 0x40) {
        EventSend(gEventMgr, *(u8**)(self + cur * 4), NULL, 0x7E57AD29u, 0, 0, 1, 1);
        *(s32*)(self + 0x104) = 0x40;
    }
}

// 0x801294CC
// World/level loader step: on first call resets the load flags and builds the asset path, loads
// the level's two .sir resources through the id pool, and once both are ready sets up the
// active zones (two-slot selection from the game state), session flags and options brightness.
void WorldLoadLevelFiles(u8* self, s32 level, const char* dir, const char* name) {
    if (*(u8*)(self + 0x120) == 0) {
        lbl_805FB210 = 1;
        *(u8*)(self + 5) = 1;
        lbl_805F95F8 = -1;
        sWorldReady = 0;
        sReadyPending = 0;
        sReadyRequested = 0;
        lbl_805FB2F3 = 0;
        sWorldSuspended = 0;
        sLoadFrames = 0;
        if (*(u8*)(self + 0x110) == 0) WorldResetQueues(self);
    }
    if (*(u8*)(self + 0x120) == 0) SysSprintf((char*)(self + 0x124), lbl_805F8FEC, name, lbl_805F8FF4);
    if (*(u8*)(self + 0x122) == 0) {
        u32 size = fn_80129928();
        void* h = SirLoad(gIdPool, (const char*)(self + 0x124), *(void**)self, size & ~0x7FFu, -1);
        fn_8012A790(self + 0x118, &h);
        *(u8*)(self + 0x122) = 1;
    }
    s32 ready = 0;
    if (*(u8*)(self + 0x122) != 0) {
        u32 key = *(u32*)(self + 0x118);
        if (fn_801E6BD0(gIdPool, &key)) ready = 1;
    }
    if (ready != 0) {
        if (*(u8*)(self + 0x123) == 0) {
            fn_80129CC8(*(u8**)self);
            u32 size = fn_801D3A84(0);
            u32* zone = (u32*)fn_8012F62C(&gZoneBase, 0);
            void* h = SirLoad(gIdPool, (const char*)(self + 0x124), (void*)(uintptr_t)*zone, size, 0);
            fn_8012A790(self + 0x11C, &h);
            *(u8*)(self + 0x123) = 1;
        }
        ready = 0;
        if (*(u8*)(self + 0x123) != 0) {
            u32 key = *(u32*)(self + 0x11C);
            if (fn_801E6BD0(gIdPool, &key)) ready = 1;
        }
        if (ready != 0) {
            fn_8013006C(self);
            *(u8*)(self + 0x121) = 1;
        }
    }
    if (*(u8*)(self + 0x121) != 0) {
        s32 a = *(s32*)(gGame + 0xC464);
        s32 b = *(s32*)(gGame + 0xC468);
        if (a < 0 || a > 1 || b < 0 || b > 1 || a == b) {
            a = 0;
            b = 1;
        }
        u8* base = *(u8**)self;
        u8* za = *(u8**)(base + a * 4 + 0x10);
        u8* zb = *(u8**)(base + b * 4 + 0x10);
        if (za != NULL || zb != NULL) {
            if (za != NULL && zb == NULL) zb = za;
            else if (za == NULL && zb != NULL) za = zb;
        }
        if (fn_801F5220()) *(u8**)(*(u8**)self + 8) = za;
        else *(u8**)(*(u8**)self + 8) = zb;
        *(u8**)(*(u8**)self + 0xC) = za;
        fn_8012A748(gGame);
        fn_800D8604(fn_8012A748(gGame));
        u8* w = fn_8012A748(gGame);
        if (fn_8012A754(w) != NULL) {
            if (*(u8**)(*(u8**)self + 0xC) == NULL) {
                u8* x = fn_8012A754(fn_8012A748(gGame));
                *(u8**)(*(u8**)self + 8) = x;
                *(u8**)(*(u8**)self + 0xC) = x;
            }
        } else if (*(u8**)(*(u8**)self + 8) == NULL) {
            u8* x = fn_8012A754(fn_8012A748(gGame));
            *(u8**)(*(u8**)self + 8) = x;
            *(u8**)(*(u8**)self + 0xC) = x;
        }
        if (gFlag805FB948 == 0) {
            u8* o = *(u8**)(*(u8**)self + b * 4 + 0x10);
            if (o != NULL) fn_8012847C(o, 0x100);
        }
        fn_80107120();
        *(u8*)(self + 6) = 0;
        fn_8012CDF8(*(u8**)(*(u8**)self + 0x10));
        fn_8012CDF8(*(u8**)(*(u8**)self + 0x14));
        u8* c = lbl_803C4298;
        if (*(u8*)(c + 0x1F) != 0) {
            fn_801F5220();
            SessionBegin(gGame + 0x5C8);
            fn_8012A75C(c, 0x80);
        } else {
            fn_8012A76C(c, 0x80);
        }
        *(u8*)(lbl_803C4298 + 0x1F) = 0;
        fn_801F5414((u8*)(uintptr_t)3);
        u32 off = *(u32*)(GetGlobalSettings() + 4);
        u32* zone = (u32*)fn_8012F62C(&gZoneBase, 0);
        u32 zbase = *zone;
        u32* e = (u32*)fn_8012F63C(*(u8**)self + 0x1B0, 0);
        fn_8012A77C(*e, zbase + off);
        u8* ws = fn_8012A780(gGame);
        fn_8012A79C(fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f), ws + 0x18A0);
        SetOptionsBrightnessTmp(GetOptionsBrightness());
        fn_80132FE4(self);
    }
    *(u8*)(self + 0x120) = 1;
}


// Number of world slots: the sum of the 13 counters in the global settings block (0x34..0x64).
static s32 SlotCount213(void) {
    u8* gs = GetGlobalSettings();
    return *(s32*)(gs + 0x4C) + *(s32*)(gs + 0x34) + *(s32*)(gs + 0x50) + *(s32*)(gs + 0x54) +
           *(s32*)(gs + 0x58) + *(s32*)(gs + 0x5C) + *(s32*)(gs + 0x60) + *(s32*)(gs + 0x64) +
           *(s32*)(gs + 0x38) + *(s32*)(gs + 0x3C) + *(s32*)(gs + 0x40) + *(s32*)(gs + 0x44) +
           *(s32*)(gs + 0x48);
}

// Zone record for zone id z, or NULL: ids 1..count come from the world's zone table; ids above
// the first counter (+0x68) up to the last category range map into the same table
// (the original tests four consecutive id ranges, which together form one range).
static u8* ZoneRecord213(s32 z) {
    u8* world = *(u8**)gWorldState;
    if (z > 0 && z <= *(s32*)(world + 4)) return *(u8**)(world + 0x1A4) + z * gZoneResourceSize;
    u8* gs = GetGlobalSettings();
    s32 s68 = *(s32*)(gs + 0x68);
    s32 s6c = *(s32*)(gs + 0x6C);
    s32 s70 = *(s32*)(gs + 0x70);
    s32 s74 = *(s32*)(gs + 0x74);
    s32 s78 = *(s32*)(gs + 0x78);
    s32 s7c = *(s32*)(gs + 0x7C);
    s32 s80 = *(s32*)(gs + 0x80);
    s32 s84 = *(s32*)(gs + 0x84);
    s32 s88 = *(s32*)(gs + 0x88);
    s32 s1 = s68 + s6c + s70 + s74 + s78 + s7c;
    // [s68+1, s1], [s1+1, s1+s80], [s1+s80+1, +s84], [.., +s88]
    if (s68 + 1 <= z && z <= s1 + s80 + s84 + s88) {
        return *(u8**)(world + 0x1A4) + z * gZoneResourceSize;
    }
    return NULL;
}

// 0x8012EE30
// Finds the zone slot (type 6 entries of the world slot table, 0x30 bytes each) whose zone
// record contains the point pos (fn_801C0830 box test at record+0xC). Returns that slot's zone id
// (+8 of the slot) or -1.
s32 fn_8012EE30(u8* world, f32* pos) {
    s32 i = 0;
    for (;;) {
        if (i >= SlotCount213()) {
            i = -1;
            break;
        }
        u8* slot = *(u8**)(gWorldState + 0xB0) + i * 0x30;
        if (*(s32*)(slot + 4) == 6) {
            s32 z = *(s32*)(slot + 8);
            u8* rec = ZoneRecord213(z);
            if (rec != NULL && fn_801C0830(rec, rec + 0xC, (u8*)pos) != 0) break;
        }
        i++;
    }
    if (i == -1) return -1;
    return *(s32*)(*(u8**)(gWorldState + 0xB0) + i * 0x30 + 8);
}

// 0x8012F6B0
// Walks a container of entities (vector at +0xC, count at +0x10), and each entity's child vector
// (+0x24, count +0x28), and resets every child by its kind (+0xC): kinds 0, 3, 4, 5 and 7 clear the
// word at +4 and install their vtable; kind 6 runs RayQueryInit; kinds 1 and 2 are untouched.
void fn_8012F6B0(u8* c) {
    if (c == NULL) return;
    u32 n = *(u32*)(c + 0x10);
    for (u32 i = 0; i < n; i++) {
        u8* e = (i < *(u32*)(c + 0x10)) ? *(u8**)(*(u8**)(c + 0xC) + i * 4) : NULL;
        if (e == NULL) continue;
        u32 m = *(u32*)(e + 0x28);
        for (u32 j = 0; j < m; j++) {
            u8* o = (j < *(u32*)(e + 0x28)) ? *(u8**)(*(u8**)(e + 0x24) + j * 4) : NULL;
            if (o == NULL) continue;
            switch (*(u32*)(o + 0xC)) {
            case 0:
                *(s32*)(o + 4) = 0;
                *(void**)o = lbl_80383A88;
                break;
            case 3:
                *(s32*)(o + 4) = 0;
                *(void**)o = lbl_80383A38;
                break;
            case 4:
                *(s32*)(o + 4) = 0;
                *(void**)o = lbl_80383B18;
                break;
            case 5:
                *(s32*)(o + 4) = 0;
                *(void**)o = lbl_803839F0;
                break;
            case 6:
                RayQueryInit(o);
                break;
            case 7:
                *(s32*)(o + 4) = 0;
                *(void**)o = lbl_80383AD0;
                break;
            default:
                break;
            }
        }
    }
}

}  // extern "C"
