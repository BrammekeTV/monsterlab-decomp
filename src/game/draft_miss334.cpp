// Decompiled from drafts: WorldPostUpdate, fn_80131370, fn_80131A1C, fn_8014762C, fn_8012636C, fn_8012CD90.
// fn_8012636C / fn_8012CD90 access 4-byte pointer slots in game data: written with `*(u8**)` at the
// original offsets (the PC build is Win32 for now, so the slots keep their 4-byte size).
#include "types.h"

struct Locator;

extern "C" {
void     BufferFree(void* p);                                    // 0x801D4980
u8*      GetGlobalSettings(void);                                // 0x801D2BF8
void*    IdPoolNext(void* pool);                                 // 0x801E6EB0
void*    MemAllocDefault(u32 size);                              // 0x801D4650
Locator* WorldFindLocator(void* world, u32 hash);                // 0x8012AA50
s32      __cvt_fp2signed(s32 v);                                 // 0x80332ED0 (integer abs)
s32      fn_801478C8(u8* table, s32 idx);                        // 0x801478C8
void     fn_801B1778(void* p, s32 a);                            // 0x801B1778
void     fn_801B4EA4(u8* obj, s32 flag);                         // 0x801B4EA4
void     fn_8012D468(u8** list, s32 count);                      // 0x8012D468 (sorts the object list)
void     fn_80137684(u8* world, s32 slot);                       // 0x80137684

extern u8  gIdPool[];             // 0x8048A8F0
extern u8  gWorldState[];         // 0x803E6D00
extern u8* lbl_805FB2E8;          // 0x805FB2E8 (list of event targets, count at +0x114)
extern s32 lbl_805FB30C;          // 0x805FB30C (world object count)
extern u8* lbl_805FB318;          // 0x805FB318 (nearest overlapping object found by WorldPostUpdate)
extern f32 lbl_805F8FF8;          // 0x805F8FF8 (its squared distance, -1 if none)
extern u8* lbl_805FB490;          // 0x805FB490 (current register table)
extern u8* lbl_805FB494;          // 0x805FB494 (spare register table)
extern f32 lbl_805FEEE4;          // 0x805FEEE4 (-1.0f)
extern f32 lbl_805FEEE8;          // 0x805FEEE8 (2.0f)
extern f32 lbl_805FEEF8;          // 0x805FEEF8 (30.0f)
extern f32 lbl_805FEF00;          // 0x805FEF00 (0.5f)
extern f32 lbl_805FF084;          // 0x805FF084 (0.0f)
}
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C
extern "C++" void fn_801B51E0(u8* obj);                 // 0x801B51E0
extern "C++" void fn_801B5248(u8* obj, u8* other);      // 0x801B5248

extern "C" {

// Sum of the thirteen consecutive counters at +0x34..+0x64 of the global settings, plus one
// (size of the per-zone / per-object lists WorldPostUpdate builds).
static s32 SettingsCountPlusOne() {
    s32 sum = 0;
    for (s32 off = 0x34; off <= 0x64; off += 4) {
        sum += *(s32*)(GetGlobalSettings() + off);
    }
    return sum + 1;
}

// Unloads/unlinks an object that is being replaced by an overlapping one: clears its pending flag
// (flag 0x8000), drops its sub-object (flag 0x4000) and then its children.
static void ReleaseObject(u8* obj) {
    u8* info = *(u8**)(obj + 0x4);
    if (*(u32*)(info + 0x20) & 0x8000) {
        fn_801B4EA4(obj, 0);
    }
    u8* sub = *(u8**)(obj + 0x8c);
    if (sub && (*(u32*)(*(u8**)(obj + 0x4) + 0x20) & 0x4000)) {
        fn_801B1778(sub, 0);
    }
    fn_801B51E0(obj);
}

// Squared centre distance between two objects when they overlap (squared distance <= squared sum of
// the two radii * 2), else -1.0.
static f32 OverlapDistSq(u8* a, u8* b) {
    u8* pb = *(u8**)(b + 0x4);
    u8* pa = *(u8**)(a + 0x4);
    f32 dx = *(f32*)(pa + 0x0) - *(f32*)(pb + 0x0);
    f32 dy = *(f32*)(pa + 0x4) - *(f32*)(pb + 0x4);
    f32 dz = *(f32*)(pa + 0x8) - *(f32*)(pb + 0x8);
    f32 r = lbl_805FEEE8 * *(f32*)(a + 0x14) + lbl_805FEEE8 * *(f32*)(b + 0x14);
    f32 d = dz * dz + (dx * dx + dy * dy);
    if (d <= r * r) return d;
    return lbl_805FEEE4;
}

// 0x8012DA58
// Post-update collision pass over the objects of the loaded zones. Gathers the live collidable objects
// (up to 0x28, sorted by fn_8012D468) and tests every pair, and then each against the dynamic objects
// of the zones; overlapping pairs on the same layer (+0x30 within 1) get the loser released and are
// notified through the vtable (+0x28) and fn_801B5248. Also remembers the nearest overlapping object
// the player (gWorldState +8) touches in lbl_805FB318 / lbl_805F8FF8.
void WorldPostUpdate(u8* arg0) {
    u8* root0 = *(u8**)gWorldState;
    if (*(u8*)(root0 + 0x1ec) == 0) return;

    lbl_805FB318 = NULL;
    lbl_805F8FF8 = lbl_805FEEE4;
    s32 count = 0;       // number of gathered objects (r22)
    u8* list[0x28];      // gathered objects (sp80)

    s32 cap = SettingsCountPlusOne();
    s32* zoneIds = (s32*)MemAllocDefault(cap * 4);   // zone index of each visited zone
    cap = SettingsCountPlusOne();
    s32* zoneEnd = (s32*)MemAllocDefault(cap * 4);   // running total of dynamic objects per visited zone

    s32 total = 0;       // running total of dynamic objects (r21)
    s32 visited = 0;     // zones visited (r25)
    for (s32 i = 0; i < lbl_805FB30C + 1; i++) {
        s32 zone = -1;
        if (lbl_805FB30C == i) {
            zone = 0;
        } else {
            u8* e = *(u8**)(gWorldState + 0xb0) + i * 0x30;
            if (*(s32*)(e + 0x4) == 6) zone = *(s32*)(e + 0x8);
        }
        if (zone < 0) continue;
        if (visited >= SettingsCountPlusOne()) break;

        zoneIds[visited] = zone;
        u8* blk;
        for (s32 j = 0;
             blk = *(u8**)(*(u8**)arg0 + 0x1a4) + zone * gZoneResourceSize,
             j < *(s32*)(blk + 0x94) + *(s32*)(blk + 0x98);
             j++) {
            u8* obj;
            if (j < *(s32*)(blk + 0x94)) {
                obj = (*(u8***)(blk + 0x120))[j];
            } else {
                obj = (*(u8***)(blk + 0x12c))[j - *(s32*)(blk + 0x94)];
            }
            if (obj == NULL) continue;
            u8* info = *(u8**)(obj + 0x4);
            if (info == NULL) continue;
            if (*(u8**)(obj + 0x8c) == NULL) continue;
            if (!((*(u32*)(info + 0x20) >> 28) & 1)) {
                if (!((*(u32*)(*(u8**)(obj + 0x4) + 0x1c) >> 3) & 1)) continue;
            }
            u32 f20 = *(u32*)(*(u8**)(obj + 0x4) + 0x20);
            if (!((f20 >> 1) & 1) && !(f20 & 1)) continue;
            if ((*(u32*)(*(u8**)(obj + 0x4) + 0x1c) >> 7) & 1) continue;

            ReleaseObject(obj);
            if (count < 0x28) {
                list[count] = obj;
                count++;
            }
        }
        total += *(s32*)(blk + 0x98);
        visited++;
        zoneEnd[visited - 1] = total;
    }

    if (count != 0) fn_8012D468(list, count);

    for (s32 a = 0; a < count; a++) {
        u8* o = list[a];
        if (o == NULL) continue;

        // Pairs among the gathered objects.
        for (s32 b = a + 1; b < count; b++) {
            u8* p = list[b];
            if (*(u8**)(o + 0x8c) == NULL || *(u8**)(p + 0x8c) == NULL) continue;
            u16 layerP = *(u16*)(*(u8**)(p + 0x4) + 0x30);
            u16 layerO = *(u16*)(*(u8**)(o + 0x4) + 0x30);
            if (__cvt_fp2signed((s32)layerO - (s32)layerP) > 1) continue;
            f32 d = OverlapDistSq(o, p);
            if (d == lbl_805FEEE4) continue;
            if (p != NULL) ReleaseObject(p);
            (*(void (**)(u8*, u8*))(*(u8**)o + 0x28))(o, p);
            fn_801B5248(o, p);
            fn_801B5248(p, o);
        }

        // Against the dynamic objects of every visited zone.
        s32 z = 0;
        for (s32 k = 0; k < total; k++) {
            s32 idx = k;
            if (k >= zoneEnd[z]) {
                while (k >= zoneEnd[z]) {
                    idx = k - zoneEnd[z];
                    z++;
                }
            } else if (z != 0) {
                idx = k - zoneEnd[z - 1];
            }
            u8* zblk = *(u8**)(*(u8**)arg0 + 0x1a4) + zoneIds[z] * gZoneResourceSize;
            u8* p = (*(u8***)(zblk + 0x12c))[idx];
            if (p == NULL) continue;
            u8* pinfo = *(u8**)(p + 0x4);
            if (pinfo != NULL && !((*(u32*)(pinfo + 0x20) >> 1) & 1)) continue;
            if (o == p) continue;
            if (*(u8**)(o + 0x8c) == NULL) continue;
            if (*(u8**)(p + 0x8c) == NULL) continue;
            u16 layerP = *(u16*)(*(u8**)(p + 0x4) + 0x30);
            u16 layerO = *(u16*)(*(u8**)(o + 0x4) + 0x30);
            if (__cvt_fp2signed((s32)layerO - (s32)layerP) > 1) continue;

            f32 d = OverlapDistSq(o, p);
            u8* player = *(u8**)gWorldState;
            player = *(u8**)(player + 0x8);
            if (o == player && d != lbl_805FEEE4
                && ((*(u32*)(*(u8**)(p + 0x4) + 0x1c) >> 6) & 1)
                && (*(u32*)(p + 0x15c) & 0x80000)
                && *(f32*)(*(u8**)(p + 0x4) + 0x8) < *(f32*)(*(u8**)(player + 0x4) + 0x8)
                && (lbl_805F8FF8 == lbl_805FEEE4 || d < lbl_805F8FF8)) {
                lbl_805F8FF8 = d;
                lbl_805FB318 = p;
            }
            if (d == lbl_805FEEE4) continue;

            u8* first = NULL;    // the object with flag +0x1c bit 8 set (r5)
            u8* other = NULL;    // its partner (r4)
            if ((*(u32*)(*(u8**)(o + 0x4) + 0x1c) >> 8) & 1) {
                first = o;
                other = p;
            } else if ((*(u32*)(*(u8**)(p + 0x4) + 0x1c) >> 8) & 1) {
                first = p;
                other = o;
            }
            if (first != NULL && ((*(u32*)(first + 0x1b0) >> 2) & 1)) {
                if (!((*(u32*)(*(u8**)(other + 0x4) + 0x1c) >> 4) & 1)) continue;
                if (*(u16*)(*(u8**)(other + 0x4) + 0x30) != *(u16*)(*(u8**)(first + 0x4) + 0x30)) continue;
            }
            if (p != NULL) ReleaseObject(p);
            (*(void (**)(u8*, u8*))(*(u8**)o + 0x28))(o, p);
            fn_801B5248(o, p);
            fn_801B5248(p, o);
        }
    }

    if (zoneIds) BufferFree(zoneIds);
    if (zoneEnd) BufferFree(zoneEnd);
}

// 0x80131370
// Distance budget for zone `zone` (mode 0 reads +0x20 of the zone records, mode 1 reads +0x24):
// starts at 30.0, then lowers it to (value of zone) - min(30, half the value of zone+2), then to
// (value of zone) - half the smallest of the zone+1 record's +0xC/+0x10/+0x14 floats.
f32 fn_80131370(u8* world, s32 mode, s32 zone) {
    f32 res = lbl_805FEEF8;
    if (zone < 0) return res;
    u8* root = *(u8**)world;
    s32 z2 = zone + 2;
    if (z2 > *(s32*)(root + 0x4)) return res;

    s32 off = (mode == 1) ? 0x24 : 0x20;
    f32 cur = *(f32*)(*(u8**)(root + 0x1a4) + zone * gZoneResourceSize + off);
    f32 next2 = *(f32*)(*(u8**)(*(u8**)world + 0x1a4) + z2 * gZoneResourceSize + off);

    f32 cap = lbl_805FEEF8;
    f32 half = lbl_805FEF00 * next2;
    if (cap >= half) cap = half;
    f32 lim = cur - cap;
    if (res >= lim) res = lim;

    root = *(u8**)world;
    s32 z1 = zone + 1;
    if (z1 > *(s32*)(root + 0x4)) return res;
    u8* blk = *(u8**)(root + 0x1a4) + z1 * gZoneResourceSize;
    f32 m = *(f32*)(blk + 0xc);
    f32 m2 = *(f32*)(blk + 0x10);
    if (m >= m2) m = m2;
    f32 m3 = *(f32*)(*(u8**)(*(u8**)world + 0x1a4) + z1 * gZoneResourceSize + 0x14);
    if (m >= m3) m = m3;
    lim = cur - m * lbl_805FEF00;
    if (res >= lim) res = lim;
    return res;
}

// 0x80131A1C
// Thin wrapper: tail-calls fn_80137684 with the same arguments.
void fn_80131A1C(u8* world, s32 slot) {
    return fn_80137684(world, slot);
}

// Register-table query: the top three bits of the entry select the form (1: bit 0; 2: the entry
// shifted left 3 read as a float is greater than 0.0; else false).
static BOOL RegisterHeld(u8* table, s32 button) {
    u32 v = (u32)fn_801478C8(table, button);
    u32 kind = v >> 29;
    if (kind == 1) return (BOOL)(v & 1);
    if (kind == 2) {
        u32 bits = v << 3;
        f32 f = *(f32*)&bits;
        return f > lbl_805FF084;
    }
    return 0;
}

// 0x8014762C
// Button released: held in the spare (previous) register table and not held in the current one.
s32 fn_8014762C(s32 button) {
    if (RegisterHeld(lbl_805FB494, button) == 0) return 0;
    return RegisterHeld(lbl_805FB490, button) == 0;
}

// 0x8012636C
// Appends an entry to the event-target list (lbl_805FB2E8, count at +0x114, 8 bytes per entry at
// +0x118): looks up the two locators by hash; the entry is kept (count++) only if both exist.
s32 fn_8012636C(u32 hashA, u32 hashB) {
    s32 ok = 0;
    u8* t = lbl_805FB2E8;
    void* a = WorldFindLocator(gWorldState, hashA);
    *(void**)(t + *(s32*)(t + 0x114) * 8 + 0x118) = a;
    t = lbl_805FB2E8;
    void* b = WorldFindLocator(gWorldState, hashB);
    *(void**)(t + *(s32*)(t + 0x114) * 8 + 0x11c) = b;
    u8* list = lbl_805FB2E8;
    s32 n = *(s32*)(list + 0x114);
    u8* entry = list + n * 8;
    if (*(u8**)(entry + 0x118) != NULL && *(u8**)(entry + 0x11c) != NULL) {
        ok = 1;
        *(s32*)(list + 0x114) = n + 1;
    }
    return ok;
}

// 0x8012CD90
// Resets the four flag bytes at +0x120..+0x123 and gives the object two fresh ids from gIdPool
// (pointer slots at +0x118 and +0x11C).
void fn_8012CD90(u8* arg0) {
    *(u8*)(arg0 + 0x121) = 0;
    *(u8*)(arg0 + 0x120) = 0;
    *(u8*)(arg0 + 0x122) = 0;
    *(u8*)(arg0 + 0x123) = 0;
    *(void**)(arg0 + 0x118) = IdPoolNext(gIdPool);
    *(void**)(arg0 + 0x11c) = IdPoolNext(gIdPool);
}

}  // extern "C"
