// Misc decompiled functions (batch miss332): zone-slot object refresh (fn_80131C2C) and the
// per-frame zone object draw pass (fn_80133E60), 0x80131C2C and 0x80133E60.
#include <math.h>
#include <string.h>

#include "types.h"
#include "revolution.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void   BufferFree(void* p);                                  // 0x801D4980
u8*    GetGlobalSettings(void);                              // 0x801D2BF8
void*  MemAllocDefault(u32 size);                            // 0x801D4650
BOOL   ZoneIsLoaded(void* world, s32 zone);                  // 0x8012F408
void fn_8003ED08(u8* self);                                    // 0x8003ED08
void   fn_8003F1B8(s32 a);                                   // 0x8003F1B8
s32    fn_80133A84(u8* zone, s32 a);                         // 0x80133A84
s32    fn_80133D74(u8* world, u8* ent);                      // 0x80133D74
void   fn_801384B0(u8* world, s32 slot, s32 a);              // 0x801384B0
s32    fn_80138964(u8* world, s32 slot);                     // 0x80138964
s32    fn_8016EC2C(u8* view, u8* a, u8* b, u8* c);           // 0x8016EC2C
void   fn_8016FB20(void* view);                              // 0x8016FB20
void   fn_80170044(void* view);                              // 0x80170044
void   fn_80182ABC(u8* p);                                   // 0x80182ABC
void   fn_80183CFC(u8* p);                                   // 0x80183CFC
void   fn_80183E88(u8* p);                                   // 0x80183E88
s32    fn_801D3A60(s32 zone);                                // 0x801D3A60
void   fn_801D6780(void);                                    // 0x801D6780
void   fn_801D7F18(u8* cam);                                 // 0x801D7F18
void   fn_801DAAFC(void* zoneObj);                           // 0x801DAAFC
void   fn_801DAFEC(void* obj, f32* e, f32* d, f32* c);      // 0x801DAFEC
void   fn_801DB0A0(void* obj, s32 a, s32 b);                 // 0x801DB0A0

extern u8   gWorldState[];       // 0x803E6D00
extern u8*  gBattle;             // 0x805FBBB8
extern u8   lbl_803C4298[];      // 0x803C4298 (level info block)
extern u8   lbl_803E9FC8[];      // 0x803E9FC8 (accumulated light/shadow data)
extern u8   lbl_80412B20[];      // 0x80412B20
extern u8   lbl_8047C640[];      // 0x8047C640 (camera)
extern u8   lbl_805F9000;        // 0x805F9000
extern u8   lbl_805F9212;        // 0x805F9212 (copy filter value)
extern f32  lbl_805FB320;        // 0x805FB320
extern u32  lbl_805FB324;        // 0x805FB324
extern u8   lbl_805FB328;        // 0x805FB328
extern u8   lbl_805FB50C;        // 0x805FB50C (GX shadow flags byte)
extern u8*  lbl_805FB75C;        // 0x805FB75C (main view)
}

extern "C++" s32 gZoneResourceSize;   // 0x805FB94C

typedef void (*VFn)(void*);

static const u32 kHashCompositeCharacter = 0xE1C807EFu;  // "composite_character"
static const u32 kHashMechanist          = 0xA2C2A850u;  // "mechanist"

// Sum of the global settings words at byte offsets [from, to] (step 4).
static s32 SettingsSum(u32 from, u32 to) {
    s32 s = 0;
    for (u32 o = from; o <= to; o += 4) {
        s += *(s32*)(GetGlobalSettings() + o);
    }
    return s;
}

// Total entity slot count: the sum of the thirteen counters at settings +0x34..+0x64.
static s32 SettingsTotal(void) {
    return SettingsSum(0x34, 0x64);
}

extern "C" {

// Resets every slot whose type value falls in one of the three ranges after the slot-count
// ranges (settings +0x68..+0x88) unless it is the current object of gWorldState+0x14/0x18/0x1C.
// (The original also runs an empty counting loop over the entity total first; dropped.)
// 0x80131C2C
void fn_80131C2C(u8* arg0) {
    for (s32 slot = 0; slot < SettingsTotal(); slot++) {
        s32 v = fn_80138964(arg0, slot);
        BOOL inRange = 0;
        BOOL found = 0;

        if ((SettingsSum(0x68, 0x80) + 1 <= v && v <= SettingsSum(0x68, 0x84)) ||
            (SettingsSum(0x68, 0x7C) + 1 <= v && v <= SettingsSum(0x68, 0x80)) ||
            (SettingsSum(0x68, 0x84) + 1 <= v && v <= SettingsSum(0x68, 0x88))) {
            inRange = 1;
        }

        if (inRange) {
            for (u32 off = 0x14; off <= 0x1C; off += 4) {
                u8* cur = *(u8**)(gWorldState + off);
                if (cur != NULL && v == *(s32*)(cur + 0x18)) {
                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            fn_801384B0(arg0, slot, 0);
        }
    }
}

}  // extern "C"

// Writes one 0x61 BP register word to the GX FIFO.
static void BP(u32 v) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = v;
}

// True when `zone` is a valid zone slot index (0..SettingsTotal()-1).
static BOOL ZoneIndexValid(s32 zone) {
    return zone >= 0 && zone < SettingsTotal();
}

// Zone slot `zone` is valid and its entry in the gWorldState+0xB0 table (0x30 bytes each) has type 6.
static BOOL ZoneSlotIsType6(s32 zone) {
    if (!ZoneIndexValid(zone)) return 0;
    return *(s32*)(*(u8**)(gWorldState + 0xB0) + zone * 0x30 + 4) == 6;
}

// Draws one zone object with the shared light vectors (colour A/B, direction E) from lbl_803E9FC8.
static void DrawZoneObj(void* obj) {
    f32 c[4], d[4], e[3];
    c[0] = *(f32*)(lbl_803E9FC8 + 0xC0);
    c[1] = *(f32*)(lbl_803E9FC8 + 0xC4);
    c[2] = *(f32*)(lbl_803E9FC8 + 0xC8);
    c[3] = *(f32*)(lbl_803E9FC8 + 0xCC);
    d[0] = *(f32*)(lbl_803E9FC8 + 0xD0);
    d[1] = *(f32*)(lbl_803E9FC8 + 0xD4);
    d[2] = *(f32*)(lbl_803E9FC8 + 0xD8);
    d[3] = *(f32*)(lbl_803E9FC8 + 0xDC);
    e[0] = *(f32*)(lbl_803E9FC8 + 0xE0);
    e[1] = *(f32*)(lbl_803E9FC8 + 0xE4);
    e[2] = *(f32*)(lbl_803E9FC8 + 0xE8);
    fn_801DAFEC(obj, e, d, c);
    fn_801DB0A0(obj, 0, 0);
}

extern "C" {

// Zone draw pass: blends the per-zone light data into one set, sets up the camera, then draws the
// zone object and every extra object attached to the current zone (view-clipped ones with the
// view copied and restricted), and finally lets every type-6 slot object run its slot-0x24 hook.
// 0x80133E60
void fn_80133E60(u8* arg0) {
    u8* levelObj = *(u8**)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0);
    if (arg0[4] == 0 || levelObj == NULL) return;
    u8* levelInfo = *(u8**)(levelObj + 0x4);
    if (levelInfo == NULL) return;
    u16 zone = *(u16*)(levelInfo + 0x30);
    if (!ZoneIsLoaded(arg0, zone)) return;

    u8* world = *(u8**)arg0;
    u8* zoneRec = *(u8**)(world + 0x1A4) + zone * gZoneResourceSize;
    lbl_805FB50C |= 8;
    BP(0xFE00C000u);
    BP((u32)lbl_805F9212 << 14);
    fn_801DAAFC(((void**)*(u8**)(*(u8**)arg0 + 0x1B0))[zone]);

    if (lbl_805FB328 != 0) return;

    fn_801D6780();
    lbl_805FB324 = 0;
    lbl_805FB320 = 0.0f;
    // Find the first zone for which fn_80133A84 reports 0 (it fills the lbl_803E9FC8 tables).
    for (s32 i = 0; i <= *(s32*)(*(u8**)gWorldState + 0x4); i++) {
        if (fn_80133A84(*(u8**)(*(u8**)gWorldState + 0x1A4) + i * gZoneResourceSize, 0) == 0) break;
    }

    // Weighted accumulation of the table entries into the light sets.
    f32 sumA = 0.0f, sumB = 0.0f, sumC = 0.0f;
    f32 d[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    f32* A = (f32*)(lbl_803E9FC8 + 0xC0);   // vec4
    f32* B = (f32*)(lbl_803E9FC8 + 0xD0);   // vec4
    f32* E = (f32*)(lbl_803E9FC8 + 0xE0);   // vec3
    A[0] = A[1] = A[2] = A[3] = 0.0f;
    B[0] = B[1] = B[2] = B[3] = 0.0f;
    E[0] = E[1] = E[2] = 0.0f;

    u8* tbl0C = *(u8**)(lbl_803E9FC8 + 0x0C);
    u8* tbl3C = *(u8**)(lbl_803E9FC8 + 0x3C);
    u8* tbl54 = *(u8**)(lbl_803E9FC8 + 0x54);
    u8* tbl24 = *(u8**)(lbl_803E9FC8 + 0x24);
    u8* tbl6C = *(u8**)(lbl_803E9FC8 + 0x6C);
    u8* tbl84 = *(u8**)(lbl_803E9FC8 + 0x84);
    u8* tbl9C = *(u8**)(lbl_803E9FC8 + 0x9C);
    f32 total = lbl_805FB320;
    u32 count = lbl_805FB324;
    for (u32 i = 0; i < count; i++) {
        f32 w;
        if (total > -0.0001f && total < 0.0001f) {
            w = 1.0f;
        } else {
            w = *(f32*)(*(u8**)(lbl_803E9FC8 + 0xB4) + i * 4) / total;
        }
        f32* v0C = (f32*)(tbl0C + i * 0x10);
        f32* v3C = (f32*)(tbl3C + i * 0x10);
        f32* v54 = (f32*)(tbl54 + i * 0xC);
        f32* v24 = (f32*)(tbl24 + i * 0x10);
        A[0] += v0C[0] * w;  A[1] += v0C[1] * w;  A[2] += v0C[2] * w;  A[3] += v0C[3] * w;
        B[0] += v3C[0] * w;  B[1] += v3C[1] * w;  B[2] += v3C[2] * w;  B[3] += v3C[3] * w;
        E[0] += v54[0] * w;  E[1] += v54[1] * w;  E[2] += v54[2] * w;
        d[0] += v24[0] * w;  d[1] += v24[1] * w;  d[2] += v24[2] * w;  d[3] += v24[3] * w;
        sumA += *(f32*)(tbl6C + i * 4) * w;
        sumB += *(f32*)(tbl84 + i * 4) * w;
        sumC += *(f32*)(tbl9C + i * 4) * w;
    }

    // Normalise the direction E (zero stays zero).
    f32 e0 = E[0], e1 = E[1], e2 = E[2];
    f32 len2 = e2 * e2 + (e0 * e0 + e1 * e1);
    f32 nx, ny, nz;
    if (len2 == 0.0f) {
        nx = ny = nz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        nx = e0 * inv;
        ny = e1 * inv;
        nz = e2 * inv;
    }
    E[0] = nx;
    E[1] = ny;
    E[2] = nz;

    // Camera setup from the accumulated sets.
    *(f32*)(lbl_8047C640 + 0x0) = 1.0f;
    *(f32*)(lbl_8047C640 + 0x4) = 1.0f;
    *(f32*)(lbl_8047C640 + 0x8) = 1.0f;
    *(f32*)(lbl_8047C640 + 0xC) = 1.0f;
    *(f32*)(lbl_8047C640 + 0x50) = d[0];
    *(f32*)(lbl_8047C640 + 0x54) = d[1];
    *(f32*)(lbl_8047C640 + 0x58) = d[2];
    *(f32*)(lbl_8047C640 + 0x5C) = d[3];
    *(f32*)(lbl_8047C640 + 0x60) = sumA;
    *(f32*)(lbl_8047C640 + 0x64) = sumB;
    *(f32*)(lbl_8047C640 + 0x68) = sumC;
    fn_801D7F18(lbl_8047C640);
    fn_8016FB20(lbl_805FB75C);
    fn_80182ABC(lbl_80412B20);

    // The zone's own object.
    DrawZoneObj(((void**)*(u8**)(*(u8**)arg0 + 0x1B0))[0]);

    if (lbl_805F9000 == 0) {
        // Type-6 slot entries: draw the object of the zone they point at.
        for (s32 i = 0; i < SettingsTotal(); i++) {
            u8* ent = *(u8**)(gWorldState + 0xB0) + i * 0x30;
            if (*(s32*)(ent + 4) != 6) continue;
            u8* obj6 = *(u8**)(ent + 0x28);
            s32 z = fn_801D3A60(*(s32*)(obj6 + 0x18));
            if (ZoneSlotIsType6(z) && ((void**)*(u8**)(gWorldState + 0xC8))[z] != NULL &&
                fn_80133D74(gWorldState, obj6) != 0) {
                DrawZoneObj(((void**)*(u8**)(gWorldState + 0xC8))[z]);
            }
            // (The original keeps looping here: its "stop" flag is always 1.)
        }
    } else {
        // Gather the zone objects to draw (the current zone first, then the ones linked from its
        // list at +0x114), then draw them from last to first.
        s32 capacity = SettingsTotal();
        u8* tmp = (u8*)MemAllocDefault((u32)capacity * 0x58);
        s32 n = 0;
        s32 z0 = fn_801D3A60((s32)zone);
        if (ZoneSlotIsType6(z0)) {
            if (((void**)*(u8**)(arg0 + 0xC8))[z0] != NULL) {
                *(s32*)(tmp + 0x0) = z0;
                *(s32*)(tmp + 0x4) = (s32)zone;
                *(s32*)(tmp + 0x8) = 0;
                n = 1;
            }
            for (s32 j = 0; j < *(s32*)(GetGlobalSettings() + 0x34); j++) {
                u8* item = *(u8**)(zoneRec + 0x114) + j * 0x54;
                if (!(*(u32*)(item + 0x4) & 1)) continue;
                s32 id = *(s32*)(item + 0x0);
                if (id <= 0) continue;
                s32 z = fn_801D3A60(id);
                if (ZoneSlotIsType6(z) && ((void**)*(u8**)(arg0 + 0xC8))[z] != NULL) {
                    u8* rec = tmp + n * 0x58;
                    *(s32*)(rec + 0x0) = z;
                    *(s32*)(rec + 0x4) = *(s32*)(item + 0x0);
                    *(s32*)(rec + 0x8) = *(s32*)(item + 0x4);
                    if (*(u32*)(item + 0x4) & 2) {
                        for (u32 o = 0; o < 19 * 4; o += 4) {
                            *(f32*)(rec + 0xC + o) = *(f32*)(item + 0x8 + o);
                        }
                    }
                    n++;
                }
            }
        }
        for (s32 k = n - 1; k >= 0; k--) {
            u8* rec = tmp + k * 0x58;
            s32 z = *(s32*)rec;
            void* zobj = ((void**)*(u8**)(arg0 + 0xC8))[z];
            u8 viewCopy[0x1A4];
            memcpy(viewCopy, lbl_805FB75C, 0x1A4);
            if (*(u32*)(rec + 0x8) & 2) {
                // Entry carries a clip volume: restrict the copied view to it, draw, restore.
                if (fn_8016EC2C(viewCopy, rec + 0xC, rec + 0x1C, rec + 0x28) != 1) {
                    fn_8016FB20(viewCopy);
                    DrawZoneObj(zobj);
                    fn_80170044(viewCopy);
                }
            } else {
                DrawZoneObj(zobj);
            }
        }
        if (tmp != NULL) BufferFree(tmp);
    }

    fn_80183CFC(lbl_80412B20);
    fn_80183E88(lbl_80412B20);

    u32 charHash = *(u32*)(*(u8**)(*(u8**)(*(u8**)arg0 + 0x8) + 0x9C) + 0x8);
    if (charHash != kHashCompositeCharacter) {
        if (charHash == kHashMechanist) {
            fn_8003ED08(arg0);
        }
        if (gBattle != NULL) {
            fn_8003F1B8(0);
            fn_8003F1B8(1);
        }
    }
    fn_80170044(lbl_805FB75C);

    // Give every type-6 slot object (and the extra pass entry after the last slot) a chance to
    // run its +0x24 hook for each object of the zone it points at.
    for (s32 i = 0; i < SettingsTotal() + 1; i++) {
        s32 zi = -1;
        if (i == SettingsTotal()) {
            zi = 0;
        } else {
            u8* ent = *(u8**)(gWorldState + 0xB0) + i * 0x30;
            if (*(s32*)(ent + 4) == 6) zi = *(s32*)(ent + 8);
        }
        if (zi < 0) continue;
        u8* zr = *(u8**)(*(u8**)gWorldState + 0x1A4) + zi * gZoneResourceSize;
        s32 total2 = *(s32*)(zr + 0x94) + *(s32*)(zr + 0x98);
        for (s32 j = 0; j < total2; j++) {
            u8* zr2 = *(u8**)(*(u8**)gWorldState + 0x1A4) + zi * gZoneResourceSize;
            s32 firstCount = *(s32*)(zr2 + 0x94);
            u8* o;
            if (j < firstCount) {
                o = *(u8**)(*(u8**)(zr2 + 0x120) + j * 4);
            } else {
                o = *(u8**)(*(u8**)(zr2 + 0x12C) + (j - firstCount) * 4);
            }
            if (o == NULL) continue;
            if ((*(u32*)(*(u8**)(o + 0x4) + 0x20) >> 1) & 1) {
                (*(VFn*)(*(u8**)o + 0x24))(o);
            }
        }
    }
}

}  // extern "C"
