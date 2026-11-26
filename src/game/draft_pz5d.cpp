// Decompiled helpers from several systems (UI quad batcher, ray/box clipping, allocator frees, ...):
// 0x80188768, 0x801C19F4 and others, see each marker.
#include <math.h>
#include "types.h"
#include "revolution.h"
#include "game/part.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" u8  sRendDLs[];         // 0x80401108 (display-list records, 0x94 bytes each)
extern "C++" s32 gViewportIndex;     // 0x805FB4BC
extern "C++" s32 gFrameBudget;       // 0x805FB4C8
extern "C++" u8 lbl_8047C730[];  // 0x8047C730 (combat results state)

extern "C" {
void BufferFree(void* p);                   // 0x801D4980
void* MemAllocDefault(u32 size);            // 0x801D4650
BOOL ScalarLess(const f32* a, const f32* b);               // 0x800533B0
void ScalarMul(f32* out, const f32* a, const f32* b);      // 0x80058214
void ScalarSet(f32* out, f32 v);            // 0x80053250
void Vec3Copy(Vec3* out, const Vec3* v);    // 0x80052640
void Vec3Dot(f32* out, const Vec3* a, const Vec3* b);      // 0x800549C0
void Vec3Length(f32* out, const Vec3* v);   // 0x80057128
void Vec3NormalizeTo(Vec3* out, const Vec3* v);            // 0x80055D9C
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);     // 0x80053370
f32  fn_800544B8(u8* out, u8* a, u8* b);    // 0x800544B8 (leaf: out = a - b)
f32  fn_800548EC(u8* p);                    // 0x800548EC (leaf)
BOOL fn_80058BE8(const f32* a, const f32* b);              // 0x80058BE8 (a < b)
u8*  fn_80067844(u8* p);                    // 0x80067844 (leaf: pointer at +0x5C)
f32  fn_80082F20(u8* a3, u8* a4);           // 0x80082F20 (copies a Vec3)
u32  fn_8010A9F4(u8* p, s32 mask);          // 0x8010A9F4 (leaf: p[0] & mask != 0)
void fn_8010AB3C(void);                     // 0x8010AB3C
void fn_8010AB70(void);                     // 0x8010AB70
void fn_80154318(void);                     // 0x80154318
extern "C++" void fn_80154358(void);                     // 0x80154358
void fn_8016F6F0(u8* out, u8* p);           // 0x8016F6F0 (leaf)
u32  fn_80174A60(s32 base, s32 i);          // 0x80174A60 (leaf: base + i*12 + 0x120)
s32  fn_801CB938(void);                     // 0x801CB938 (leaf)
u32  fn_801CB95C(s32 base, s32 i);          // 0x801CB95C (leaf: base + i*16 + 0xC0)
u32  fn_801CB96C(u8* list);                 // 0x801CB96C (leaf)
void fn_801CB988(u8* e);                    // 0x801CB988
u32  fn_801CBCDC(s32 list, u8* e);          // 0x801CBCDC (leaf)
s32  fn_801CBCF8(u8* p, s32 bit);           // 0x801CBCF8
u32  fn_801CBD10(u8* a, u8* b);             // 0x801CBD10 (leaf)
u8*  fn_801CBD28(u8* p);                    // 0x801CBD28 (leaf: p + 0x10)
f32  fn_801CBD30(u8* p);                    // 0x801CBD30 (leaf)
s32  fn_801CBD38(u8* p);                    // 0x801CBD38 (leaf)
void fn_801CBD40(u8* e);                    // 0x801CBD40
void fn_801CAD80(void* recs, s32 count);    // 0x801CAD80
s32  fn_801CC928(u8* e);                    // 0x801CC928
s32  fn_801CD044(u8* e, u8* ref, f32 v);    // 0x801CD044
u8*  fn_801D6D78(void);                     // 0x801D6D78 (camera record: axes at +0/+0xC/+0x18, position at +0x24)
u8*   fn_8019552C(u8* list, s32 index);      // 0x8019552C (leaf)
s32  fn_8019555C(u8* list);                 // 0x8019555C (leaf)
f32  fn_801AA728(u8* entry);                // 0x801AA728 (leaf)
s32  fn_801AA784(u8* entry);                // 0x801AA784 (leaf)
s32  fn_801AA78C(u8* entry);                // 0x801AA78C (leaf)
s32  fn_801AA794(u8* entry);                // 0x801AA794 (leaf)
void fn_801ABD48(s32 v);                    // 0x801ABD48 (leaf)
void fn_801AB3DC(u8* obj, f32 pos, s32 loop, s32 rev);  // 0x801AB3DC
void UIEvent_SendToAllScreens(u32 a0);      // 0x80195104
s32  PlaySoundAt(void* sound, int a, const void* pos, int b);  // 0x80209ABC
s32  TriggerDoAction(void* action, void* a1, void* a2);        // 0x801E0054
void* fn_801D40E0(s32 n);                   // 0x801D40E0
void* fn_80209814(void* v);                 // 0x80209814
s32  BattleListActions(u8* stats, void* out);  // 0x8021415C
s32  FighterStatsGet(u8* stats, s32 i);     // 0x80214950
s32  PartGetBattleEnergy(const PartInstance* torso, u8** limbs);  // 0x8021ED90
s32  fn_80220974(f32 r);                    // 0x80220974
void fn_8018D370(const char* fmt, ...);     // 0x8018D370 (debug log)
void NoticePosted(u8* p, s32 idx);          // 0x8022DF3C
u64  __shl2i(s32, s32, s32);                // 0x8032F9A0
u64  __shr2u(s32, s32, s32);                // 0x8032F9C4
void fn_8014DE68(void);                     // 0x8014DE68
s32  fn_8015BA30(void* p);                  // 0x8015BA30
void fn_801514F0(void);                     // 0x801514F0
void fn_80154290(void);                    // 0x80154290
void fn_801542DC(void);                     // 0x801542DC
void fn_80154974(void);                     // 0x80154974
void fn_80155D70(void);                     // 0x80155D70
void fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);  // 0x801566E8
void fn_8014E3EC(s32 n);                    // 0x8014E3EC
void fn_80151E38(s32 quads, void* verts);   // 0x80151E38
s32  fn_801C1960(f32* tmin, f32* tmax, f32 dir, f32 dist);  // 0x801C1960
}

extern "C" {
extern u8  lbl_805FB50C;   // 0x805FB50C (GX shadow flags byte)
extern s32 lbl_805FB4C4;   // 0x805FB4C4
extern s32 lbl_805FB4CC;   // 0x805FB4CC
extern u8* lbl_805FB4D0;   // 0x805FB4D0
extern const char lbl_803816F8[];  // 0x803816F8 (log string)
extern u8*  gBattle;       // 0x805FBBB8 (+0xA00 fighters[2])
extern f32 lbl_8060055C;   // 0x8060055C (float constant)
extern void* lbl_805FBBAC;  // 0x805FBBAC (voice SFX handle)
extern u8  lbl_80478570[];  // 0x80478570
extern s32 lbl_805F95D4;    // 0x805F95D4
extern s32 lbl_805F95D8;    // 0x805F95D8
extern s32 lbl_805F98A0;    // 0x805F98A0
extern s32 lbl_805FB720;    // 0x805FB720
extern s32 lbl_805FB724;    // 0x805FB724
extern s8  lbl_805FB738;    // 0x805FB738
extern f32 lbl_805FB73C;    // 0x805FB73C
extern s8  lbl_805FB740;    // 0x805FB740
extern f32 lbl_805FB744;    // 0x805FB744
extern s8  lbl_805FB748;    // 0x805FB748
extern f32 lbl_805FB74C;    // 0x805FB74C
extern f32 lbl_805FFD98;    // 0x805FFD98 (float constant)
extern f32 lbl_805FFD9C;    // 0x805FFD9C (float constant)
extern f32 lbl_805FFDA0;    // 0x805FFDA0 (float constant)
extern f32 lbl_805FFDA4;    // 0x805FFDA4 (float constant)
extern s32* lbl_805FB798;  // 0x805FB798 (per-pool requested byte counters)
extern s32* lbl_805FB79C;  // 0x805FB79C (per-pool allocation counters)
extern s32* lbl_805FB7A0;  // 0x805FB7A0 (per-pool allocation counters)
extern u8*  gGlobalSettings;  // 0x805FB7A4 (pointer to the settings block)
extern u8* lbl_805FBBE0;   // 0x805FBBE0 (save block pointer)
extern s32 lbl_80600620;   // 0x80600620 (table index)
extern s32 lbl_80600624;   // 0x80600624 (table index)
extern s32 lbl_80600628;   // 0x80600628 (flag id)
extern s32 lbl_8060062C;   // 0x8060062C (flag id)
extern s32 lbl_805FB514;   // 0x805FB514 (GX shadow word)
extern f32 lbl_805FF7E4;   // 0x805FF7E4 (float constant)
extern f32 lbl_805FF7F0;   // 0x805FF7F0 (float constant, -1.0)
extern f32 lbl_805FF7F4;   // 0x805FF7F4 (float constant, 1.0)
extern f32 lbl_805FF7F8;   // 0x805FF7F8 (float constant, 0.0)
extern f32 lbl_805FF7FC;   // 0x805FF7FC (float constant, 255.0)
extern f32 lbl_805FFC70;   // 0x805FFC70 (float constant, zero)
extern f32 lbl_805FFC74;   // 0x805FFC74 (float constant)
extern f32 lbl_805FFC78;   // 0x805FFC78 (float constant)
extern f32 lbl_805FFC7C;   // 0x805FFC7C (float constant)
extern f32 lbl_805FFC80;   // 0x805FFC80 (float constant, zero)
extern f32 lbl_805FFC98;   // 0x805FFC98 (float constant, one)
}

static inline u32 Bswap32(u32 v) {
    return (v >> 24) | ((v >> 8) & 0xFF00u) | ((v << 8) & 0xFF0000u) | (v << 24);
}

extern "C" {

// 0x80188768
// Draws the rectangles of one UI layer list as quads (4 vertices of 0x18 bytes) into the display list,
// scaled/offset by the rect at arg1+8 and flushed in batches of 32 quads. Does nothing if arg2 is set.
void fn_80188768(u8* arg0, u8* arg1, u8* arg2, s32 arg3) {
    if (arg2 != NULL) return;
    f32 scaleX = *(f32*)(arg1 + 0x8);
    f32 scaleY = *(f32*)(arg1 + 0xC);
    f32 offX = *(f32*)(arg1 + 0x10);
    f32 offY = *(f32*)(arg1 + 0x14);
    u8 flags = lbl_805FB50C;
    if (flags & 2) {
        lbl_805FB50C = flags & ~2;
        if (flags & 1) {
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE00000F;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE003C00;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x441;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x28380000;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC008F2A4;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC108E6F0;
        } else {
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE00000F;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE003C00;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x100E; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x28380000;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC008FFFA;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC108FFD0;
        }
    }
    if (lbl_805FB4CC < 0xC00) fn_8014E3EC(0xC00);
    s32 n = 0;
    u8* rect = *(u8**)(*(u8**)(arg0 + 0x4) + arg3 * 0xC + 0x4);
    u8* verts = lbl_805FB4D0 + lbl_805FB4C4;
    f32 zero = lbl_805FF7E4;
    while (rect != NULL) {
        s16 x = *(s16*)(rect + 0xC);
        s16 y = *(s16*)(rect + 0xE);
        s16 w = *(s16*)(rect + 0x8);
        s16 h = *(s16*)(rect + 0xA);
        u32 col = Bswap32(*(u32*)(rect + 0x4));
        f32 x0 = offX + (f32)x * scaleX;
        f32 y0 = offY + (f32)y * scaleY;
        f32 x1 = offX + (f32)(x + w) * scaleX;
        f32 y1 = offY + (f32)(y + h) * scaleY;
        u8* q = verts + n * 0x18;
        *(f32*)(q + 0x00) = zero; *(f32*)(q + 0x04) = zero; *(u32*)(q + 0x08) = col;
        *(f32*)(q + 0x0C) = x0;   *(f32*)(q + 0x10) = y0;   *(f32*)(q + 0x14) = zero;
        *(f32*)(q + 0x18) = zero; *(f32*)(q + 0x1C) = zero; *(u32*)(q + 0x20) = col;
        *(f32*)(q + 0x24) = x1;   *(f32*)(q + 0x28) = y0;   *(f32*)(q + 0x2C) = zero;
        *(f32*)(q + 0x30) = zero; *(f32*)(q + 0x34) = zero; *(u32*)(q + 0x38) = col;
        *(f32*)(q + 0x3C) = x0;   *(f32*)(q + 0x40) = y1;   *(f32*)(q + 0x44) = zero;
        *(f32*)(q + 0x48) = zero; *(f32*)(q + 0x4C) = zero; *(u32*)(q + 0x50) = col;
        *(f32*)(q + 0x54) = x1;   *(f32*)(q + 0x58) = y1;   *(f32*)(q + 0x5C) = zero;
        n += 4;
        if (n >= 0x80) {
            s32 size = (n * 0x18 + 0x1F) & ~0x1F;
            if (lbl_805FB4CC < size) fn_8014E3EC(size);
            lbl_805FB4CC -= size;
            lbl_805FB4C4 += size;
            fn_80151E38(n >> 2, verts);
            if (lbl_805FB4CC < 0xC00) fn_8014E3EC(0xC00);
            n = 0;
            verts = lbl_805FB4D0 + lbl_805FB4C4;
        }
        rect = *(u8**)rect;
    }
    if (n > 0) {
        s32 size = (n * 0x18 + 0x1F) & ~0x1F;
        if (lbl_805FB4CC < size) fn_8014E3EC(size);
        lbl_805FB4CC -= size;
        lbl_805FB4C4 += size;
        fn_80151E38(n >> 2, verts);
    }
}

// 0x801C19F4
// Clips the ray c + d*t against the box centred at a with half extents b (via fn_801C1960 per axis
// slab). Writes the number of hits (0..2) to *n, the hit points to pos (3 floats each) and their
// ray parameters to dist.
s32 fn_801C19F4(u8* a, u8* b, u8* c, u8* d, s32* n, f32* pos, f32* dist) {
    f32 vx = *(f32*)(c + 0x0) - *(f32*)(a + 0x0);
    f32 vy = *(f32*)(c + 0x4) - *(f32*)(a + 0x4);
    f32 vz = *(f32*)(c + 0x8) - *(f32*)(a + 0x8);
    f32 dx = *(f32*)(d + 0x0), dy = *(f32*)(d + 0x4), dz = *(f32*)(d + 0x8);
    f32 tmin = lbl_805FFC70;
    f32 tmax = lbl_805FFC74;
    s32 ok = 0;
    if (fn_801C1960(&tmin, &tmax, dx, -vx - *(f32*)(b + 0x0)) != 0 &&
        fn_801C1960(&tmin, &tmax, -dx, vx - *(f32*)(b + 0x0)) != 0 &&
        fn_801C1960(&tmin, &tmax, dy, -vy - *(f32*)(b + 0x4)) != 0 &&
        fn_801C1960(&tmin, &tmax, -dy, vy - *(f32*)(b + 0x4)) != 0 &&
        fn_801C1960(&tmin, &tmax, dz, -vz - *(f32*)(b + 0x8)) != 0 &&
        fn_801C1960(&tmin, &tmax, -dz, vz - *(f32*)(b + 0x8)) != 0) {
        ok = 1;
    }
    s32 hit = ok && (tmin != lbl_805FFC70 || tmax != lbl_805FFC74);
    if (hit) {
        if (tmin > lbl_805FFC70) {
            if (tmax < lbl_805FFC74) {
                *n = 2;
                pos[0] = *(f32*)(c + 0x0) + dx * tmin;
                pos[1] = *(f32*)(c + 0x4) + dy * tmin;
                pos[2] = *(f32*)(c + 0x8) + dz * tmin;
                pos[3] = *(f32*)(c + 0x0) + dx * tmax;
                pos[4] = *(f32*)(c + 0x4) + dy * tmax;
                pos[5] = *(f32*)(c + 0x8) + dz * tmax;
                dist[0] = tmin;
                dist[1] = tmax;
            } else {
                *n = 1;
                pos[0] = *(f32*)(c + 0x0) + dx * tmin;
                pos[1] = *(f32*)(c + 0x4) + dy * tmin;
                pos[2] = *(f32*)(c + 0x8) + dz * tmin;
                dist[0] = tmin;
            }
        } else if (tmax < lbl_805FFC74) {
            *n = 1;
            pos[0] = *(f32*)(c + 0x0) + dx * tmax;
            pos[1] = *(f32*)(c + 0x4) + dy * tmax;
            pos[2] = *(f32*)(c + 0x8) + dz * tmax;
            dist[0] = tmax;
        } else {
            *n = 0;
        }
    } else {
        *n = 0;
    }
    return hit;
}

// 0x801C26A8
// Clips the ray d + e*t against an oriented box: a is the box centre, b the half extents, c the 3x3
// rotation (rows at +0, +0xC, +0x18). The ray is brought into box space and clipped slab by slab.
// Writes the number of hits (0..2) to *n, hit points to pos and ray parameters to dist.
s32 fn_801C26A8(u8* a, u8* b, u8* c, u8* d, u8* e, s32* n, f32* pos, f32* dist) {
    f32 mx = *(f32*)(d + 0x0) - *(f32*)(a + 0x0);
    f32 my = *(f32*)(d + 0x4) - *(f32*)(a + 0x4);
    f32 mz = *(f32*)(d + 0x8) - *(f32*)(a + 0x8);
    f32 r00 = *(f32*)(c + 0x00), r01 = *(f32*)(c + 0x04), r02 = *(f32*)(c + 0x08);
    f32 r10 = *(f32*)(c + 0x0C), r11 = *(f32*)(c + 0x10), r12 = *(f32*)(c + 0x14);
    f32 r20 = *(f32*)(c + 0x18), r21 = *(f32*)(c + 0x1C), r22 = *(f32*)(c + 0x20);
    f32 ex = *(f32*)(e + 0x0), ey = *(f32*)(e + 0x4), ez = *(f32*)(e + 0x8);
    f32 lmx = mx * r00 + my * r01 + mz * r02;
    f32 ldx = ex * r00 + ey * r01 + ez * r02;
    f32 lmy = mx * r10 + my * r11 + mz * r12;
    f32 ldy = ex * r10 + ey * r11 + ez * r12;
    f32 lmz = mx * r20 + my * r21 + mz * r22;
    f32 ldz = ex * r20 + ey * r21 + ez * r22;
    f32 tmin = lbl_805FFC7C;
    f32 tmax = lbl_805FFC78;
    s32 ok = 0;
    if (fn_801C1960(&tmin, &tmax, ldx, -lmx - *(f32*)(b + 0x0)) != 0 &&
        fn_801C1960(&tmin, &tmax, -ldx, lmx - *(f32*)(b + 0x0)) != 0 &&
        fn_801C1960(&tmin, &tmax, ldy, -lmy - *(f32*)(b + 0x4)) != 0 &&
        fn_801C1960(&tmin, &tmax, -ldy, lmy - *(f32*)(b + 0x4)) != 0 &&
        fn_801C1960(&tmin, &tmax, ldz, -lmz - *(f32*)(b + 0x8)) != 0 &&
        fn_801C1960(&tmin, &tmax, -ldz, lmz - *(f32*)(b + 0x8)) != 0) {
        ok = 1;
    }
    s32 hit = ok && (tmin != lbl_805FFC7C || tmax != lbl_805FFC78);
    if (hit) {
        if (tmin >= lbl_805FFC70) {
            *n = 2;
            pos[0] = *(f32*)(d + 0x0) + ex * tmin;
            pos[1] = *(f32*)(d + 0x4) + ey * tmin;
            pos[2] = *(f32*)(d + 0x8) + ez * tmin;
            pos[3] = *(f32*)(d + 0x0) + ex * tmax;
            pos[4] = *(f32*)(d + 0x4) + ey * tmax;
            pos[5] = *(f32*)(d + 0x8) + ez * tmax;
            dist[0] = tmin;
            dist[1] = tmax;
        } else {
            *n = 1;
            pos[0] = *(f32*)(d + 0x0) + ex * tmax;
            pos[1] = *(f32*)(d + 0x4) + ey * tmax;
            pos[2] = *(f32*)(d + 0x8) + ez * tmax;
            dist[0] = tmax;
        }
    } else {
        *n = 0;
    }
    return hit;
}

// 0x801C2C60
// Ray/sphere test: sphere centre a, radius r; ray origin b, direction c. Writes the number of hits
// (0..2, only roots with t <= 1 and a hit in front) to *n, optionally the hit points (pos) and ray
// parameters (dist). Returns 1 if the ray hits.
s32 fn_801C2C60(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist) {
    f32 mx = *(f32*)(b + 0x0) - *(f32*)(a + 0x0);
    f32 my = *(f32*)(b + 0x4) - *(f32*)(a + 0x4);
    f32 mz = *(f32*)(b + 0x8) - *(f32*)(a + 0x8);
    f32 dx = *(f32*)(c + 0x0), dy = *(f32*)(c + 0x4), dz = *(f32*)(c + 0x8);
    f32 A = dx * dx + dy * dy + dz * dz;
    f32 B = mx * dx + my * dy + mz * dz;
    f32 C = mx * mx + my * my + mz * mz - r * r;
    f32 disc = B * B - A * C;
    f32 zero = lbl_805FFC80;
    if (disc < zero) {
        *n = 0;
        return 0;
    }
    if (disc > zero) {
        f32 sq = sqrtf(disc);
        f32 t0 = (-B - sq) * (lbl_805FFC98 / A);
        f32 t1 = (-B + sq) * (lbl_805FFC98 / A);
        if (t0 > lbl_805FFC98 || t1 < lbl_805FFC80) {
            *n = 0;
            return 0;
        }
        if (t0 >= lbl_805FFC80) {
            if (t1 > lbl_805FFC98) {
                *n = 1;
                if (dist) dist[0] = t0;
                if (pos) {
                    pos[0] = *(f32*)(b + 0x0) + dx * t0;
                    pos[1] = *(f32*)(b + 0x4) + dy * t0;
                    pos[2] = *(f32*)(b + 0x8) + dz * t0;
                }
                return 1;
            }
            *n = 2;
            if (dist) { dist[0] = t0; dist[1] = t1; }
            if (pos) {
                pos[0] = *(f32*)(b + 0x0) + dx * t0;
                pos[1] = *(f32*)(b + 0x4) + dy * t0;
                pos[2] = *(f32*)(b + 0x8) + dz * t0;
                pos[3] = *(f32*)(b + 0x0) + dx * t1;
                pos[4] = *(f32*)(b + 0x4) + dy * t1;
                pos[5] = *(f32*)(b + 0x8) + dz * t1;
            }
            return 1;
        }
        *n = 1;
        if (dist) dist[0] = t1;
        if (pos) {
            pos[0] = *(f32*)(b + 0x0) + dx * t1;
            pos[1] = *(f32*)(b + 0x4) + dy * t1;
            pos[2] = *(f32*)(b + 0x8) + dz * t1;
        }
        return 1;
    }
    // Tangent ray: single root.
    if (A == zero) {
        *n = 0;
        return 0;
    }
    f32 t = -B / A;
    if (zero <= t && t <= lbl_805FFC98) {
        *n = 1;
        if (dist) dist[0] = t;
        if (pos) {
            pos[0] = *(f32*)(b + 0x0) + dx * t;
            pos[1] = *(f32*)(b + 0x4) + dy * t;
            pos[2] = *(f32*)(b + 0x8) + dz * t;
        }
        return 1;
    }
    *n = 0;
    return 0;
}

static inline s32 ClampByte(s32 v) {
    if (v < 0) return 0;
    return v > 0xFF ? 0xFF : v;
}

// 0x80188FC0
// Finishes the 2D overlay pass: resets the GX state for a full-screen ortho view, sets the TEV/fog
// colour to opaque white, clears the dirty GX flags and ends with the fixed identity matrix block.
void fn_80188FC0(void) {
    fn_801542DC();
    fn_80154974();
    fn_80155D70();
    fn_801566E8(lbl_805FF7F0, lbl_805FF7F4, lbl_805FF7F0, lbl_805FF7F4, lbl_805FF7F8, lbl_805FF7F0);
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE000090;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x80000000;
    f32 col[4];
    col[0] = col[1] = col[2] = col[3] = lbl_805FF7F4;
    if (fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x1107 > gFrameBudget) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + gViewportIndex * 0x94);
    }
    u8 flags = lbl_805FB50C;
    if (flags & 1) {
        lbl_805FB50C = flags & 0xFE;
        if (flags & 2) {
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE00000F;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 1;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 1;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE003C00;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x100E; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x28380040;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC008F8AF;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC108F2F0;
        } else {
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE00000F;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE003C00;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x100E; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x28380000;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC008FFFA;
            GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xC108FFD0;
        }
    }
    s32 c0 = ClampByte((s32)(lbl_805FF7FC * col[0]));
    s32 c1 = ClampByte((s32)(lbl_805FF7FC * col[1]));
    s32 c2 = ClampByte((s32)(lbl_805FF7FC * col[2]));
    s32 c3 = ClampByte((s32)(lbl_805FF7FC * col[3]));
    GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x100C;
    GXWGFifo._u32 = (u32)(c3 | (c2 << 8) | (c0 << 24) | (c1 << 16));
    flags = lbl_805FB50C;
    if (flags & 4) {
        s32 v = lbl_805FB514 | 0x40;
        lbl_805FB50C = flags & ~4;
        lbl_805FB514 = v;
        GXWGFifo._u8 = 0x61; GXWGFifo._u32 = (u32)v | 0x43000000u;
        GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xF33F0000;
    }
    fn_801514F0();
    lbl_805FB50C &= ~8;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE00C000;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE000001;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x41000001;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0xFE000FE0;
    GXWGFifo._u8 = 0x61; GXWGFifo._u32 = 0x410004A0;
    GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0xB; GXWGFifo._u16 = 0x500;
    GXWGFifo._u32 = 0x3F800000; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0x3F800000; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0; GXWGFifo._u32 = 0;
    GXWGFifo._u32 = 0x3F800000; GXWGFifo._u32 = 0;
    fn_80154290();
}

// 0x801D2E9C
// Frees the three per-pool counter arrays and the global settings block (each through the counted
// heap free), then clears all four pointers.
void fn_801D2E9C(void) {
    if (lbl_805FB79C != NULL) BufferFree(lbl_805FB79C);
    if (lbl_805FB798 != NULL) BufferFree(lbl_805FB798);
    if (lbl_805FB7A0 != NULL) BufferFree(lbl_805FB7A0);
    u8* settings = gGlobalSettings;
    lbl_805FB7A0 = NULL;
    lbl_805FB798 = NULL;
    lbl_805FB79C = NULL;
    if (settings != NULL) BufferFree(settings);
    gGlobalSettings = NULL;
}

extern "C" void BadgeWhy(const char* fmt, ...);   // port: badge award log (progress.cpp)
extern "C" void BadgeLogAward(s32 id);

// Bit `id` (0..255) of the 256-bit flag set at p+0x20: four big-endian 64-bit groups, the highest
// ids first (ids 0..63 at +0x38, 64..127 at +0x30, 128..191 at +0x28, 192..255 at +0x20).
static inline u8* FlagGroup(u8* p, u32 id, u32* shift) {
    if (id < 0x40) { *shift = id;        return p + 0x38; }
    if (id < 0x80) { *shift = id - 0x40; return p + 0x30; }
    if (id < 0xC0) { *shift = id - 0x80; return p + 0x28; }
    *shift = id - 0xC0;
    return p + 0x20;
}

static inline bool FlagTest(u8* p, u32 id) {
    u32 sh;
    u8* g = FlagGroup(p, id, &sh);
    return ((*(u64*)g >> sh) & 1) != 0;   // native u64, as SaveHasBadge (progress.cpp)
}

static inline void FlagSet(u8* p, u32 id) {
    u32 sh;
    u8* g = FlagGroup(p, id, &sh);
    *(u64*)g |= (u64)1 << sh;             // native u64, as SaveAddBadge (progress.cpp)
}

// Queues a notice record (up to 15) of the given type and id in the save block's notice list.
static inline void PostNotice(u8* p, s32 type, s32 id) {
    s32 n = *(s32*)(p + 0x8E8);
    if (n < 0xF) {
        u8* rec = p + n * 0x1C;
        *(s32*)(rec + 0x744) = type;
        *(s32*)(rec + 0x748) = id;
        *(s32*)(rec + 0x74C) = 0;
        *(s32*)(rec + 0x750) = 0;
        *(s32*)(rec + 0x754) = 0;
        *(s32*)(rec + 0x758) = -1;
        s32 serial = *(s32*)(p + 0x910) + 1;
        *(s32*)(p + 0x910) = serial;
        *(s32*)(rec + 0x75C) = serial;
        *(s32*)(p + 0x8E8) = *(s32*)(p + 0x8E8) + 1;
        NoticePosted(p, n);
    }
}

// 0x80229030
// Adds `amount` to the running total at +0x9D8 and unlocks (flag set, counter at +0x120, notice of
// type 4, bonus added to the capped total at lbl_805FBBE0+0x12C) each of the two milestone entries
// whose threshold the total now reaches; then raises the level (+0x128, max 0x31) through the
// level table while its thresholds are below the new total, posting a type-1 notice.
void fn_80229030(u8* p, s32 amount) {
    s32 total = *(s32*)(p + 0x9D8) + amount;
    *(s32*)(p + 0x9D8) = total;
    s32 ids[2] = {lbl_80600628, lbl_8060062C};
    s32 idxs[2] = {lbl_80600620, lbl_80600624};
    for (s32 i = 1; i >= 0; i--) {
        s32 id = ids[i];
        if (FlagTest(p, (u8)id)) continue;
        u8* entry = *(u8**)(gGameDb->data + 0x99D0) + idxs[i] * 8;
        if (total < (s32)*(f32*)(entry + 4)) continue;
        if (id == -1) continue;
        if (FlagTest(p, (u8)id)) continue;
        *(s32*)(p + 0x120) = *(s32*)(p + 0x120) + 1;
        FlagSet(p, (u8)id);
        BadgeWhy("counter +0x9D8 = %d (+%d) >= %d", total, amount, (s32)*(f32*)(entry + 4));
        BadgeLogAward(id);
        PostNotice(p, 4, id);
        u8* reward = *(u8**)(gGameDb->data + 0x10A8) + id * 0x18;
        u8* g = lbl_805FBBE0;
        u32 sum = *(u32*)(g + 0x12C) + (u32)*(f32*)(reward + 0x10);
        *(u32*)(g + 0x12C) = sum;
        if (sum > 0x5F5E0FFu) *(u32*)(g + 0x12C) = 0x5F5E0FFu;
        s32 level = *(s32*)(g + 0x128);
        if (level < 0x31) {
            u32 next = (u32)level + 1;
            bool raised = false;
            u8* levels = *(u8**)(gGameDb->data + 0x63C8);
            while (next <= 0x31u &&
                   *(f32*)(levels + next * 0x10 + 8) < (f32)*(u32*)(g + 0x12C)) {
                *(s32*)(g + 0x128) = (s32)next;
                next++;
                raised = true;
            }
            if (raised) PostNotice(g, 1, *(s32*)(g + 0x128));
        }
    }
}

typedef void (*TreeCb)(void* a, void* b);

// Post-order walk of a binary tree (children at +0xC and +0x10, payload words at +0 and +4):
// left subtree, right subtree, then cb(node[0], node[4]). A NULL callback only logs.
static void TreeWalk(u8* node, TreeCb cb) {
    u8* left = *(u8**)(node + 0xC);
    if (left != NULL) {
        if (cb == NULL) fn_8018D370(lbl_803816F8);
        else TreeWalk(left, cb);
    }
    u8* right = *(u8**)(node + 0x10);
    if (right != NULL) {
        if (cb == NULL) fn_8018D370(lbl_803816F8);
        else TreeWalk(right, cb);
    }
    cb(*(void**)(node + 0), *(void**)(node + 4));
}

// 0x8018E2F4
// Runs cb over every node of the tree whose root pointer is at table+0 (children first). Returns -2
// (and logs) if the table, its root or the callback is NULL, else 0.
s32 fn_8018E2F4(void* table, void* cb) {
    u8* root = *(u8**)table;
    if (cb == NULL || root == NULL) {
        fn_8018D370(lbl_803816F8);
        return -2;
    }
    TreeWalk(root, (TreeCb)cb);
    return 0;
}

// Classifies the energy ratio into a band 0..4 (0 if fn_80220974 flags it, else by the thresholds
// in the tuning block).
static s32 EnergyBand(f32 ratio) {
    if (fn_80220974(ratio) != 0) return 0;
    u8* tune = *(u8**)(gGameDb->data + 0x99D0);
    if (ratio < *(f32*)(tune + 0xB4)) return 1;
    if (ratio < *(f32*)(tune + 0xAC)) return 2;
    if (ratio < lbl_8060055C) return 3;
    return 4;
}

// 0x8020C164
// Prepares the battle-AI choice tables for fighter obj[0]: picks a health tier (0..2) from its
// torso health fraction, an energy band (0..4) for itself and for its opponent, stores the chosen
// entry at +8, sums the stat modifiers of the two chosen entries into four shorts, lists the
// fighter's usable actions (+0x34, count at +0x84) and writes the matching summed value into the
// float slot at +0xC of each action's stat kind.
void fn_8020C164(u8* obj) {
    u8* fighter = *(u8**)obj;
    u8* torso = *(u8**)(fighter + 0x10);
    s32* health = *(s32**)(torso + 0x28);
    u8* tune = *(u8**)(gGameDb->data + 0x99D0);
    f32 hpRatio = (f32)health[0] / (f32)health[1];
    s32 tier;
    if (hpRatio < *(f32*)(tune + 0xE4)) tier = 0;
    else if (hpRatio < *(f32*)(tune + 0xDC)) tier = 1;
    else tier = 2;

    f32 energy = (f32)FighterStatsGet(fighter + 0x20, 3);
    energy = energy / (f32)PartGetBattleEnergy(*(PartInstance**)torso, (u8**)fighter);
    s32 band = EnergyBand(energy);
    *(s32*)(obj + 0x8) = *(s32*)(obj + 0xA8 + band * 0xC + tier * 4);

    u8* opp = *(u8**)(gBattle + 0xA00 + (1 - *(s32*)(fighter + 0x44)) * 4);
    f32 oppEnergy = (f32)FighterStatsGet(opp + 0x20, 3);
    oppEnergy = oppEnergy / (f32)PartGetBattleEnergy(*(PartInstance**)torso, (u8**)opp);
    s32 oppBand = EnergyBand(oppEnergy);

    s32 ids[2];
    ids[0] = *(s32*)(obj + 0x8);
    ids[1] = *(s32*)(obj + 0xE4 + oppBand * 4);
    s16 acc[4] = {0, 0, 0, 0};
    for (s32 k = 0; k < 2; k++) {
        u8* e = *(u8**)(gGameDb->data + 0x18F8) + ids[k] * 0x1C;
        acc[0] = (s16)(acc[0] + *(s16*)(e + 0x4));
        acc[1] = (s16)(acc[1] + *(s16*)(e + 0x6));
        acc[2] = (s16)(acc[2] + *(s16*)(e + 0x8));
        acc[3] = 0;
    }
    *(s32*)(obj + 0x84) = BattleListActions(fighter + 0x20, obj + 0x34);
    for (s32 i = 0; i < *(s32*)(obj + 0x84); i++) {
        s32 action = *(s32*)(obj + 0x38 + i * 8);
        u8* row = *(u8**)(gGameDb->data + 0x430) + action * 0x14;
        u8* type = *(u8**)(gGameDb->data + 0x8) + *(s32*)(row + 4) * 0x24;
        s32 slot = *(s32*)(obj + 0x88 + *(s32*)(type + 8) * 4);
        *(f32*)(obj + 0xC + slot * 4) = (f32)acc[slot];
    }
}

static inline u8* ListEntry(u8* list, s32 i) {
    return fn_8019552C(list, i);
}

// Fires the event stored in an anim-list entry, by its kind (+0x24): 1 triggers an action if the
// entry's word has any of bits 27..30 set, 2 sends a UI event to all screens, 3 plays a voice sound.
static void AnimEntryFire(u8* self, u8* e) {
    switch (*(s32*)(e + 0x24)) {
    case 1: {
        u32 action = (u32)fn_801AA784(e);
        if (((action >> 27) & 0xF) != 0) {
            fn_801ABD48(*(s32*)(self + 0x10));
            TriggerDoAction(&action, NULL, NULL);
            fn_801ABD48(0);
        }
        break;
    }
    case 2:
        UIEvent_SendToAllScreens((u32)fn_801AA78C(e));
        break;
    case 3: {
        s32 snd = fn_801AA794(e);
        if (snd != 0) {
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySoundAt((void*)(uintptr_t)snd, 0, NULL, 0);
        }
        break;
    }
    default:
        break;
    }
}

// 0x801AAF14
// Advances an anim-track player from oldPos to newPos (rev = playing backwards): first the entries
// passed when the position wrapped (current index +8 .. end, or down to 0 when reversed; the entry
// at the current index is fired each time, as in the original), then every following entry whose
// time (fn_801AA728) lies up to newPos (down to it when reversed), moving the index (+8; -2 = none).
s32 fn_801AAF14(u8* self, f32 oldPos, f32 newPos, s32 loop, s32 rev) {
    if (*(s32*)(self + 0x8) <= -1) fn_801AB3DC(self, oldPos, loop, rev);
    s32 n = fn_8019555C(*(u8**)(self + 0x4));
    if ((rev == 0 && oldPos > newPos) || (rev != 0 && oldPos < newPos)) {
        if (rev == 0) {
            s32 i = *(s32*)(self + 0x8);
            if (i != -2) {
                for (; i < n; i++) {
                    AnimEntryFire(self, ListEntry(*(u8**)(self + 0x4), *(s32*)(self + 0x8)));
                }
            }
            *(s32*)(self + 0x8) = (n > 0) ? 0 : -2;
        } else {
            s32 i = *(s32*)(self + 0x8);
            if (i != -2) {
                for (; i >= 0; i--) {
                    AnimEntryFire(self, ListEntry(*(u8**)(self + 0x4), *(s32*)(self + 0x8)));
                }
            }
            *(s32*)(self + 0x8) = (n > 0) ? n - 1 : -2;
        }
    }
    s32 cur = *(s32*)(self + 0x8);
    u8* e = (cur == -2) ? NULL : ListEntry(*(u8**)(self + 0x4), cur);
    while (e != NULL) {
        if (rev == 0) {
            if (!(fn_801AA728(e) <= newPos)) { e = NULL; continue; }
            AnimEntryFire(self, e);
            s32 next = *(s32*)(self + 0x8) + 1;
            *(s32*)(self + 0x8) = next;
            if (next < n) {
                e = ListEntry(*(u8**)(self + 0x4), next);
            } else {
                *(s32*)(self + 0x8) = -2;
                e = NULL;
            }
        } else {
            if (!(fn_801AA728(e) >= newPos)) { e = NULL; continue; }
            AnimEntryFire(self, e);
            s32 next = *(s32*)(self + 0x8) - 1;
            *(s32*)(self + 0x8) = next;
            if (next < n) {
                e = ListEntry(*(u8**)(self + 0x4), next);
            } else {
                *(s32*)(self + 0x8) = -2;
                e = NULL;
            }
        }
    }
    return 0;
}

// 0x801CB414
// Walks the entities of the list at +0x18 (up to +4 of them), collects those that pass the filters
// into 16-byte records (entity, distance along the camera direction plus its radius, flag, id word),
// sorts them and then handles each one by mode (arg1: 0 = only entities with +0x74 == 0,
// 1 = only those with +0x74 != 0): flagged ones are released with fn_801CBD40, the first remaining
// one is processed with fn_801CC928 or fn_801CD044 depending on the flags word at +0x20 (bit 1/2,
// otherwise by distance thresholds). Finally adds the counters at +0/+4/+8 to the running totals.
void fn_801CB414(u8* arg0, s32 arg1) {
    u8* cache = lbl_80478570;
    if (fn_801CB938() == 0) return;
    if (*(s32*)(arg0 + 0x18) == 0) return;
    if (lbl_805F98A0 != lbl_805F95D8 || lbl_805F95D4 != 0) lbl_805F95D8 = lbl_805F98A0;

    // Cache the camera's (normalised) axes and position.
    u8* cam = fn_801D6D78();
    Vec3 tmp;
    Vec3Copy((Vec3*)(cache + 0xC), (Vec3*)(cam + 0x0));
    Vec3NormalizeTo(&tmp, (Vec3*)(cache + 0xC));
    Vec3Copy((Vec3*)(cache + 0xC), &tmp);
    Vec3Copy((Vec3*)(cache + 0x18), (Vec3*)(cam + 0xC));
    Vec3NormalizeTo(&tmp, (Vec3*)(cache + 0x18));
    Vec3Copy((Vec3*)(cache + 0x18), &tmp);
    Vec3Copy((Vec3*)(cache + 0x30), (Vec3*)(cam + 0x18));
    Vec3NormalizeTo(&tmp, (Vec3*)(cache + 0x30));
    Vec3Copy((Vec3*)(cache + 0x30), &tmp);
    Vec3Copy((Vec3*)(cache + 0x3C), (Vec3*)(cam + 0x24));

    u8* recs = (u8*)MemAllocDefault((u32)(*(s32*)(arg0 + 0x4) << 4));
    s32 count = 0;
    s32 base = fn_801CB938();
    u8* ref = (u8*)(uintptr_t)fn_801CB95C(base, 1);
    u8* rec = recs;
    u8* ent = (u8*)(uintptr_t)fn_801CB96C(*(u8**)(arg0 + 0x18));
    while (ent != NULL) {
        fn_801CB988(ent);
        u8* next = (u8*)(uintptr_t)fn_801CBCDC(*(s32*)(arg0 + 0x18), ent);
        if (fn_801CBCF8(ent, 5) == 0 && fn_801CBD10(arg0, ent) != 0) {
            Vec3 entPos, refPos;
            f32 dot, refW, diff;
            *(u8**)(rec + 0x0) = ent;
            fn_80082F20((u8*)&entPos, fn_801CBD28(ent));
            fn_80082F20((u8*)&refPos, ref);
            Vec3Dot(&dot, &entPos, &refPos);
            fn_8016F6F0((u8*)&refW, ref);
            fn_800544B8((u8*)&diff, (u8*)&refW, (u8*)&dot);
            f32 radius = fn_801CBD30(fn_80067844(ent));
            *(f32*)(rec + 0x4) = fn_800548EC((u8*)&diff) + radius;
            count++;
            *(s32*)(rec + 0x8) = fn_801CBD38(*(u8**)(ent + 0x5C)) == 0;
            *(s32*)(rec + 0xC) = **(s32**)(ent + 0x5C);
            rec += 0x10;
            if (count >= *(s32*)(arg0 + 0x4)) break;
        }
        ent = next;
    }
    fn_801CAD80(recs, count);

    f32 spread = lbl_805FFD98;
    s32 first = 1;     // nothing processed yet in this pass
    s32 result = 0;
    if (fn_8010A9F4(arg0 + 0x20, 4) != 0) {
        Vec3* p0 = (Vec3*)(uintptr_t)fn_80174A60(base, 0);
        Vec3* p1 = (Vec3*)(uintptr_t)fn_80174A60(base, 1);
        Vec3* p3 = (Vec3*)(uintptr_t)fn_80174A60(base, 3);
        Vec3 d3, d1;
        f32 len3, len1, prod;
        Vec3Sub(&d3, p3, p0);
        Vec3Length(&len3, &d3);
        Vec3Sub(&d1, p1, p0);
        Vec3Length(&len1, &d1);
        ScalarMul(&prod, &len3, &len1);
        spread = fn_800548EC((u8*)&prod);
    }
    rec = recs;
    for (s32 i = 0; i < count; i++, rec += 0x10) {
        u8* e = *(u8**)(rec + 0x0);
        if (e == NULL) continue;
        s32 flag74 = *(s32*)(e + 0x74);
        if (flag74 != 0 && arg1 == 0) continue;
        if (flag74 == 0 && arg1 == 1) continue;
        if (*(s32*)(rec + 0x8) != 0) {
            first = 1;
            fn_801CBD40(e);
        } else if (result == 0) {
            if (first != 0) {
                first = 0;
                lbl_805FB720 = 0;
                lbl_805FB724 = 0;
                fn_80154358();
                fn_8010AB3C();
                fn_80155D70();
            }
            if (fn_8010A9F4(arg0 + 0x20, 1) != 0) {
                result = fn_801CC928(e);
            } else if (fn_8010A9F4(arg0 + 0x20, 2) != 0) {
                result = fn_801CD044(e, ref, spread);
            } else {
                if (lbl_805FB738 == 0) { ScalarSet(&lbl_805FB73C, lbl_805FFD9C); lbl_805FB738 = 1; }
                if (lbl_805FB740 == 0) { ScalarSet(&lbl_805FB744, lbl_805FFDA0); lbl_805FB740 = 1; }
                if (lbl_805FB748 == 0) { ScalarSet(&lbl_805FB74C, lbl_805FFDA4); lbl_805FB748 = 1; }
                f32 dist, h, limit;
                ScalarSet(&dist, *(f32*)(rec + 0x4));
                fn_8016F6F0((u8*)&h, fn_801CBD28(e));
                if (ScalarLess(&dist, &lbl_805FB73C) && fn_80058BE8(&h, &lbl_805FB744)) {
                    ScalarMul(&limit, &h, &lbl_805FB74C);
                    if (ScalarLess(&dist, &limit)) {
                        result = fn_801CD044(e, ref, spread);
                        continue;
                    }
                }
                result = fn_801CC928(e);
            }
        }
    }
    BufferFree(recs);
    fn_80154318();
    fn_8010AB70();
    if (first != 0) {
        lbl_805FB720 = 0;
        lbl_805FB724 = 0;
        fn_80155D70();
    }
    *(s32*)(lbl_8047C730 + 0x2C) += *(s32*)(arg0 + 0x8);
    *(s32*)(lbl_8047C730 + 0x28) += *(s32*)(arg0 + 0x4);
    *(s32*)(lbl_8047C730 + 0x24) += *(s32*)(arg0 + 0x0);
}

}  // extern "C"
