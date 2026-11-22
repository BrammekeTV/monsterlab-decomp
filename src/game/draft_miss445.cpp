// Camera look-at setup from a track object, part info panel fill, UI grid frame update, OBB
// segment test, OBB-vs-shape collision dispatcher, and the font glyph quad emitter.
// fn_800E43BC, fn_8011B060, fn_801A4994, fn_801AEE84, fn_801B0644, fn_80187798.
#include <math.h>
#include <string.h>
#include "types.h"
#include "game/part.h"

struct StringTable {
    const char** strings;   // 0x00
    u8           pad[8];
    u32          count;     // 0x0C
};

extern u8*          gSaveSystem;     // 0x805FBBE0
extern StringTable* gStringTable;    // 0x805FB910
extern StringTable* gStringTable2;   // 0x805FB90C

extern "C" {
void   CameraSetFov(void* camera, f32 fov);                                  // 0x801D8ACC
void   PartAnimKey(char* out, s32 limb, s32 defId);                          // 0x80103E04 (builds the part model key)
BOOL   SavePartTierOwned(u8* s, u32 part, s32 tier);                         // 0x802241F0
BOOL   SavePartTierSeen(u8* s, u32 part, s32 tier);                          // progress.cpp
u8*    fn_801094F4(char* name, s32 flags);                                   // 0x801094F4
s32    fn_80109A0C(s32 id);                                                  // 0x80109A0C
u8*    fn_801189C4(s32 idx);                                                 // 0x801189C4 (only r3 is read; caller also passes a record copy)
u8*    fn_80118B1C(s32 idx);                                                 // 0x80118B1C (only r3 is read)
s32    fn_80120990(s32 a, s32 b, s32 tier);                                  // 0x80120990
void   fn_8018A948(u8* obj, f32 t);                                          // 0x8018A948
void   fn_8018C3D0(void* p);                                                 // 0x8018C3D0
void   fn_8018D1FC(const char* fmt, ...);                                    // 0x8018D1FC (debug log)
void   fn_8018D2B8(const char* fmt, ...);                                    // 0x8018D2B8 (debug log)
void   fn_8018D370(const char* fmt, ...);                                    // 0x8018D370 (debug log)
u8*     fn_8019552C(u8* list, s32 index);                                     // 0x8019552C (leaf; returns a slot pointer)
s32    fn_8019555C(u8* list);                                                // 0x8019555C (leaf; element count)
s32    fn_80196678(void* gx, void* mat, f32 t);                              // 0x80196678
s32    fn_80196B10(u8* a3);                                                  // 0x80196B10 (leaf)
u8*    fn_80196B24(u8* obj);                                                 // 0x80196B24
s32    fn_8019B6CC(u8* arg0, f32 farg0);                                     // 0x8019B6CC (UI element base update)
s32    fn_801A6880(u8* arg0);                                                // 0x801A6880
s32    fn_801A6D7C(u8* arg0, s32 mode);                                      // 0x801A6D7C
s32    fn_801A72EC(u8* grid);                                                // 0x801A72EC
s32    fn_801A9CE0(u8* obj, f32 dt, f32 t);                                  // 0x801A9CE0
s32    fn_801A9E94(u8* obj, f32 a, f32 b, f32 c);                            // 0x801A9E94
u8*    fn_801AC184(u8* defs, s32 idx);                                       // 0x801AC184
void*  fn_801B7198(void* anim, s32 a);                                       // 0x801B7198
s32    fn_801C098C(u8* a, u8* b, u8* c, u8* d, u8* e, f32* out0, f32* out1); // 0x801C098C
s32    fn_801C10B4(u8* a, u8* b, u8* c, u8* d, u8* e, u8* f);                // 0x801C10B4
s32    fn_801C217C(u8* a, u8* b, u8* c, u8* d, u8* e, s32* n, f32* pos, f32* dist); // 0x801C217C
s32    fn_801C26A8(u8* a, u8* b, u8* c, u8* d, u8* e, s32* n, f32* pos, f32* dist); // 0x801C26A8
s32    fn_801ED538(u8* a3);                                                  // 0x801ED538
s32    fn_80187704(u8* table, s32 idx);                                            // 0x80187704
f64    fn_8033E8EC(f64 x);                                                   // 0x8033E8EC (atan)

extern const char lbl_80383448[];   // 0x80383448 (debug message block)
extern s32  lbl_80347DE0[];         // 0x80347DE0 (5 name hashes)
extern u8   lbl_803C4298[];         // 0x803C4298 (level info block; +4 = object)
extern u8   lbl_8047C640[];         // 0x8047C640 (camera)
extern u8   lbl_805FB1A1;           // 0x805FB1A1
extern u8   lbl_805FB1A4[];         // 0x805FB1A4
extern u8   lbl_805FB1C0;           // 0x805FB1C0 (mirror Y)
extern u8   lbl_805FB1C1;           // 0x805FB1C1 (mirror X)
extern u8*  lbl_805FB1C4;           // 0x805FB1C4 (optional look-at target object, +4 = position)
extern void* lbl_805FB658;          // 0x805FB658 (global render object)
}

typedef u8*  (*VFnPtr)(u8* self, u8* arg);
typedef void (*VFn0)(u8* self);
typedef void (*VFnInt)(u8* self, s32 arg);
typedef void (*VFnF32)(u8* self, f32 arg);

// Resolve a text hash to its string; bit 31 of the id selects the secondary table.
static const char* PartText(u32 hash) {
    s32 id = TextIdFromHash(hash);
    if (id == -1) return "Not found";
    if (((u32)id & 0x80000000u) && gStringTable2 != NULL) {
        u32 index = (u32)id & 0x7FFFFFFFu;
        if (gStringTable2->strings != NULL && index < gStringTable2->count) return gStringTable2->strings[index];
        return "";
    }
    if (gStringTable->strings != NULL && (u32)id < gStringTable->count) return gStringTable->strings[id];
    return "";
}

extern "C" {

// 0x800E43BC
// Builds the camera look-at for a track object: resolves the object, picks the joint positions
// (mirrored by the two flags), rotates them by the object's quaternion, writes the eye position
// to outPos and the unit look direction to outDir, and sets the FOV from the focus joint distance.
// Returns 0 when there is no usable object.
s32 fn_800E43BC(u8* arg0, f32* outPos, f32* outDir) {
    if (*(u8*)(arg0 + 0x54) == 0) return 0;
    s32 h = fn_801ED538(arg0 + 0x58);
    u8* o = (u8*)fn_801B7198(&h, 0);
    if (o == NULL || (*(s32*)(*(u8**)(o + 0x4) + 0x20) & 3) == 0) return 0;
    lbl_805FB1A4[1] = 0;
    u8* actor = (*(VFnPtr*)(*(u8**)o + 0x10))(o, lbl_805FB1A4);
    if (actor == NULL || *(s32*)(actor + 0xEC) == 0) {
        if (actor == NULL || *(s32*)(arg0 + 0x5C) == 0) {
            *(u8*)(arg0 + 0x74) = 0;
            return 0;
        }
        s32 h2 = fn_801ED538(arg0 + 0x58);
        u8* o2 = (u8*)fn_801B7198(&h2, 0);
        if (o2 != NULL) {
            s32 anim = *(s32*)(arg0 + 0x5C);
            if (anim != 0) {
                u8* a = fn_801AC184(*(u8**)(*(u8**)(o2 + 0x9C) + 0x48), anim);
                *(u32*)(a + 0xC) |= 0x10000;
                (*(VFnInt*)(*(u8**)o2 + 0x84))(o2, *(s32*)(arg0 + 0x5C));
                lbl_805FB1A1 = 0;
                u8* lvlObj = *(u8**)(lbl_803C4298 + 0x4);
                (*(VFn0*)(*(u8**)lvlObj + 0x10))(lvlObj);
            }
            (*(VFnF32*)(*(u8**)o2 + 0x90))(o2, 0.0f);
        }
    }
    if (*(u8*)(arg0 + 0x74) == 0) {
        *(u8*)(arg0 + 0x74) = 1;
        fn_8018C3D0(actor);
        fn_8018A948(actor, 0.0f);
    }
    u8* data = *(u8**)actor;
    if (*(s32*)(arg0 + 0x60) < 0) {
        // Look up the five named joints in the actor's name table (-1 when missing).
        for (s32 i = 0; i < 5; i++) {
            s32 n = *(s32*)(data + 0x8);
            s32 found = -1;
            for (s32 k = 0; k < n; k++) {
                if (lbl_80347DE0[i] == *(s32*)(*(u8**)(data + 0xC) + k * 8)) { found = k; break; }
            }
            *(s32*)(arg0 + 0x60 + i * 4) = found;
        }
    }

    // Quaternion (x, y, z, w at +0xC..+0x18 of the object block) to a rotation matrix.
    u8* q = *(u8**)(o + 0x4);
    f32 qa = *(f32*)(q + 0xC), qb = *(f32*)(q + 0x10), qc = *(f32*)(q + 0x14), qd = *(f32*)(q + 0x18);
    f32 b2 = 2.0f * qb;
    f32 c2 = 2.0f * qc;
    f32 a2 = 2.0f * qa;
    f32 bb = b2 * qb;
    f32 cc = c2 * qc;
    f32 cb = c2 * qb;
    f32 ca = c2 * qa;
    f32 aa = a2 * qa;
    f32 ad = a2 * qd;
    f32 bd = b2 * qd;
    f32 cd = c2 * qd;
    f32 ba = b2 * qa;
    f32 m00 = 1.0f - (bb + cc);
    f32 m01 = ba + cd;
    f32 m02 = ca - bd;
    f32 m10 = ba - cd;
    f32 m11 = 1.0f - (aa + cc);
    f32 m12 = cb + ad;
    f32 m20 = ca + bd;
    f32 m21 = cb - ad;
    f32 m22 = 1.0f - (aa + bb);

    // Eye joint, mirrored by the flags.
    u8* j = *(u8**)(data + 0x4) + *(s32*)(arg0 + 0x64) * 0x30;
    outPos[0] = *(f32*)(j + 0x24);
    outPos[1] = *(f32*)(j + 0x28);
    outPos[2] = *(f32*)(j + 0x2C);
    // Focus joint (the +0x6C joint is read by the original but never used).
    u8* jf = *(u8**)(data + 0x4) + *(s32*)(arg0 + 0x68) * 0x30;
    f32 fx = *(f32*)(jf + 0x24), fy = *(f32*)(jf + 0x28), fz = *(f32*)(jf + 0x2C);
    if (lbl_805FB1C0 != 0) {
        outPos[1] = -outPos[1];
        fy = -fy;
    }
    if (lbl_805FB1C1 != 0) {
        outPos[0] = -outPos[0];
        fx = -fx;
    }
    // Rotate into object space and add the object position.
    f32 ex = outPos[0], ey = outPos[1], ez = outPos[2];
    f32 nx = (m20 * ez) + ((m00 * ex) + (m10 * ey));
    f32 ny = (m21 * ez) + ((m01 * ex) + (m11 * ey));
    f32 nz = (m22 * ez) + ((m02 * ex) + (m12 * ey));
    u8* pos = *(u8**)(o + 0x4);
    outPos[0] = nx + *(f32*)(pos + 0x0);
    outPos[1] = ny + *(f32*)(pos + 0x4);
    outPos[2] = nz + *(f32*)(pos + 0x8);
    f32 tx = (m20 * fz) + ((m00 * fx) + (m10 * fy));
    f32 ty = (m21 * fz) + ((m01 * fx) + (m11 * fy));
    f32 tz = (m22 * fz) + ((m02 * fx) + (m12 * fy));
    pos = *(u8**)(o + 0x4);
    f32 targetX = tx + *(f32*)(pos + 0x0);
    f32 targetY = ty + *(f32*)(pos + 0x4);
    f32 targetZ = tz + *(f32*)(pos + 0x8);
    if (lbl_805FB1C4 != NULL) {
        u8* tp = *(u8**)(lbl_805FB1C4 + 0x4);
        targetX = *(f32*)(tp + 0x0);
        targetY = *(f32*)(tp + 0x4);
        targetZ = *(f32*)(tp + 0x8);
    }
    // Look direction = normalize(target - eye).
    f32 dx = targetX - outPos[0];
    f32 dy = targetY - outPos[1];
    f32 dz = targetZ - outPos[2];
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    f32 ux, uy, uz;
    if (len2 == 0.0f) {
        ux = uy = uz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);   // original: frsqrte with two Newton steps
        ux = dx * inv;
        uy = dy * inv;
        uz = dz * inv;
    }
    outDir[0] = ux;
    outDir[1] = uy;
    outDir[2] = uz;

    // FOV from the length of the +0x70 joint offset, clamped to 0..100.
    s32 fj = *(s32*)(arg0 + 0x70);
    if (fj >= 0) {
        u8* jp = *(u8**)(data + 0x4) + fj * 0x30;
        f32 px = *(f32*)(jp + 0x24), py = *(f32*)(jp + 0x28), pz = *(f32*)(jp + 0x2C);
        f32 plen2 = (pz * pz) + ((px * px) + (py * py));
        f32 dist = 0.0f;
        if (plen2 > 0.0f) dist = plen2 * (1.0f / sqrtf(plen2));
        f32 c = 0.0f;
        if (dist >= 0.0f) {
            c = 100.0f;
            if (!(dist > 100.0f)) c = dist;
        }
        f32 at = (f32)fn_8033E8EC((f64)(2.4f / ((c * 0.39f) + 1.0f)));
        CameraSetFov(lbl_8047C640, 1.5707964f * (2.0f * (0.63661975f * at)));
    }
    return 1;
}

// 0x8011B060
// Fills the part info record out for a PartInstance: icon model, tier name, health/quality/madness,
// both action names, and the first matching lab-book entry (defect and enhancement icons and text).
void fn_8011B060(PartInstance* arg0, u8* out) {
    PartDef d;
    PartDbGetDef(&d, gGameDb, arg0->defIndex);
    u8* db = gGameDb->data;
    *(u32*)(out + 0x28) = d.tierName[0];
    u32 nameHash = d.tierName[0];
    switch (PartGetTier(arg0)) {
    case 2: nameHash = d.tierName[1]; break;
    case 3: nameHash = d.tierName[2]; break;
    case 4: nameHash = d.tierName[3]; break;
    default: break;
    }
    *(const char**)(out + 0x2C) = PartText(nameHash);
    *(s32*)(out + 0x3C) = *(s32*)(*(u8**)(db + 0x7890) + d.discipline * 8);   // discipline table entry
    *(s32*)(out + 0x40) = d.slot;
    *(s32*)(out + 0x30) = PartGetMaxHealth(arg0);
    *(s32*)(out + 0x34) = PartGetQuality(arg0);
    *(s32*)(out + 0x38) = PartGetMadness(arg0);

    // Action 1 name.
    {
        u8* data = gGameDb->data;
        u8* act = *(u8**)(data + 0x430) + d.action1 * 0x14;
        s32 type = *(s32*)(act + 0x4);
        u32 hash = *(u32*)(*(u8**)(data + 0x8) + type * 0x24 + 0x10);
        *(u32*)(out + 0x44) = hash;
        *(const char**)(out + 0x48) = PartText(hash);
    }
    // Action 2 name.
    {
        u8* data = gGameDb->data;
        u8* act = *(u8**)(data + 0x430) + d.action2 * 0x14;
        s32 type = *(s32*)(act + 0x4);
        u32 hash = *(u32*)(*(u8**)(data + 0x8) + type * 0x24 + 0x10);
        *(u32*)(out + 0x4C) = hash;
        *(const char**)(out + 0x50) = PartText(hash);
    }

    // Model: "ui/msa_parts/<animkey>", falling back to the placeholder.
    char key[0x100];
    char path[0x100];
    PartAnimKey(key, d.slot, d.index);
    strcpy(path, "ui/msa_parts/");
    strcat(path, key);
    u8* icon = fn_801094F4(path, 0);
    if (icon == NULL) {
        strcpy(path, "ui/msa_parts/");
        strcat(path, "placeholder");
        icon = fn_801094F4(path, 0);
    }
    *(u8**)(out + 0x4) = icon;

    // Find the first lab-book entry (0x24 bytes each, 0x9C of them) for this part that was seen.
    s32 i = 0;
    s32 off = 0;
    *(s32*)(out + 0x58) = -1;
    while (i < 0x9C && *(s32*)(out + 0x58) == -1) {
        u8* rec = *(u8**)(gGameDb->data + 0x6C18) + off;
        if (*(s32*)(rec + 0x20) == arg0->defIndex && SavePartTierSeen(gSaveSystem, *(u32*)rec, PartGetTier(arg0)) != 0) {
            *(s32*)(out + 0x58) = i;
        }
        off += 0x24;
        i++;
    }
    s32 entry = *(s32*)(out + 0x58);
    *(s32*)(out + 0x1C) = 0;
    *(s32*)(out + 0x10) = 0;
    if (entry != -1) {
        u8* dbd = gGameDb->data;
        u8* rec = *(u8**)(dbd + 0x6C18) + entry * 0x24;
        u8* defect = *(u8**)(dbd + 0x4F00) + *(s32*)(rec + 0x10) * 0x10;   // defect record (0x10 bytes)
        u8* defIcon = fn_801189C4(*(s32*)(rec + 0x10));
        *(u8**)(out + 0x14) = defIcon;
        if (fn_80109A0C((s32)(intptr_t)defIcon) != 0) {
            *(s32*)(out + 0x10) = *(s32*)(out + 0x14);
            *(s32*)(out + 0x14) = 0;
        }
        *(const char**)(out + 0x18) = PartText(*(u32*)(defect + 0x4));
        *(s32*)(out + 0x5C) = 0;
        if (SavePartTierOwned(gSaveSystem, *(u32*)rec, PartGetTier(arg0)) != 0) {
            s32 r = fn_80120990(*(s32*)(rec + 0x8), *(s32*)(rec + 0x14), PartGetTier(arg0));
            if (r != -1) {
                u8* enh = *(u8**)(gGameDb->data + 0x5328) + r * 0x18;   // enhancement record (0x18 bytes)
                *(s32*)(out + 0x5C) = 1;
                u8* enhIcon = fn_80118B1C(r);
                *(u8**)(out + 0x20) = enhIcon;
                if (fn_80109A0C((s32)(intptr_t)enhIcon) != 0) {
                    *(s32*)(out + 0x1C) = *(s32*)(out + 0x20);
                    *(s32*)(out + 0x20) = 0;
                }
                *(const char**)(out + 0x24) = PartText(*(u32*)(enh + 0x4));
            }
        }
    }
    *(s32*)(out + 0x8) = 0;
    *(s32*)(out + 0xC) = 0;
}

// 0x801A4994
// Per-frame update of a UI grid: pushes the flag-driven show/hide calls, composes the element
// transform with the render object's top matrix, drives the 0..1 fade timer (cosine ease) and
// updates every cell. Returns 0, -1 on a bad cell count, -2 when the cell list is empty.
s32 fn_801A4994(u8* arg0, f32 dt) {
    f32 fade = -1.0f;
    s32 firstFrame = 0;
    u8* cells = *(u8**)(arg0 + 0x1BC);
    if (cells == NULL || fn_8019555C(cells) < 1) {
        fn_8018D2B8(lbl_80383448 + 0x98);
        return -2;
    }
    if ((*(s32*)(arg0 + 0x64) & 1) == 0) {
        u8* sub = *(u8**)(arg0 + 0x1B4);
        if (sub != NULL) {
            *(f32*)(sub + 0x80) = *(f32*)(sub + 0x14C);
            *(f32*)(sub + 0x84) = *(f32*)(sub + 0x150);
            *(f32*)(sub + 0x88) = *(f32*)(sub + 0x154);
            *(f32*)(sub + 0x8C) = *(f32*)(sub + 0x158);
            sub = *(u8**)(arg0 + 0x1B4);
            (*(VFnInt*)(*(u8**)(sub + 0x138) + 0x2C))(sub, 0x20);
        }
    }
    if (*(s32*)(arg0 + 0x64) & 0x40000) {
        (*(VFnInt*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x10000);
        if (*(s32*)(arg0 + 0x148) % 2 != 1) {
            fn_8018D370(lbl_80383448 + 0xC8);
            return -1;
        }
    }
    if (*(s32*)(arg0 + 0x64) & 0x200000) {
        (*(VFnInt*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x80000);
        if (*(s32*)(arg0 + 0x144) % 2 != 1) {
            fn_8018D370(lbl_80383448 + 0x104);
            return -1;
        }
    }
    if (*(u8*)(arg0 + 0x17C) == 0) {
        fn_801A72EC(arg0);
        if (*(s32*)(arg0 + 0x64) & 0x40000) *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x148) / 2;
        if (*(s32*)(arg0 + 0x64) & 0x200000) *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x144) / 2;
        fn_801A6D7C(arg0, 1);
        (*(VFnInt*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x18);
    }
    fn_8019B6CC(arg0, dt);
    if (*(s32*)(arg0 + 0x64) & 0x10) {
        // Local matrix (+0xF0, 12 floats) composed with the render object's top matrix into +0x1C0.
        u8* a = fn_80196B24((u8*)lbl_805FB658);
        if (a != NULL) {
            f32 a0 = *(f32*)(a + 0x0), a1 = *(f32*)(a + 0x4), a2 = *(f32*)(a + 0x8), a3 = *(f32*)(a + 0xC);
            f32 a4 = *(f32*)(a + 0x10), a5 = *(f32*)(a + 0x14), a6 = *(f32*)(a + 0x18), a7 = *(f32*)(a + 0x1C);
            f32 a8 = *(f32*)(a + 0x20), a9 = *(f32*)(a + 0x24), a10 = *(f32*)(a + 0x28), a11 = *(f32*)(a + 0x2C);
            f32 b0 = *(f32*)(arg0 + 0xF0), b1 = *(f32*)(arg0 + 0xF4), b2 = *(f32*)(arg0 + 0xF8);
            f32 b3 = *(f32*)(arg0 + 0xFC), b4 = *(f32*)(arg0 + 0x100), b5 = *(f32*)(arg0 + 0x104);
            f32 b6 = *(f32*)(arg0 + 0x108), b7 = *(f32*)(arg0 + 0x10C), b8 = *(f32*)(arg0 + 0x110);
            f32 b9 = *(f32*)(arg0 + 0x114), b10 = *(f32*)(arg0 + 0x118), b11 = *(f32*)(arg0 + 0x11C);
            *(f32*)(arg0 + 0x1C0) = (a6 * b2) + ((a0 * b0) + (a3 * b1));
            *(f32*)(arg0 + 0x1C4) = (a7 * b2) + ((a1 * b0) + (a4 * b1));
            *(f32*)(arg0 + 0x1C8) = (a8 * b2) + ((a2 * b0) + (a5 * b1));
            *(f32*)(arg0 + 0x1CC) = (a6 * b5) + ((a0 * b3) + (a3 * b4));
            *(f32*)(arg0 + 0x1D0) = (a7 * b5) + ((a1 * b3) + (a4 * b4));
            *(f32*)(arg0 + 0x1D4) = (a8 * b5) + ((a2 * b3) + (a5 * b4));
            *(f32*)(arg0 + 0x1D8) = (a6 * b8) + ((a0 * b6) + (a3 * b7));
            *(f32*)(arg0 + 0x1DC) = (a7 * b8) + ((a1 * b6) + (a4 * b7));
            *(f32*)(arg0 + 0x1E0) = (a8 * b8) + ((a2 * b6) + (a5 * b7));
            *(f32*)(arg0 + 0x1E4) = a9 + ((a6 * b11) + ((a0 * b9) + (a3 * b10)));
            *(f32*)(arg0 + 0x1E8) = a10 + ((a7 * b11) + ((a1 * b9) + (a4 * b10)));
            *(f32*)(arg0 + 0x1EC) = a11 + ((a8 * b11) + ((a2 * b9) + (a5 * b10)));
        } else {
            for (s32 k = 0; k < 12; k++) *(f32*)(arg0 + 0x1C0 + k * 4) = *(f32*)(arg0 + 0xF0 + k * 4);
        }
        *(f32*)(arg0 + 0x5C) = *(f32*)(arg0 + 0x1EC);
        (*(VFnInt*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x10);
        (*(VFnInt*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x10);
    }
    if (*(s32*)(arg0 + 0x64) & 0x20) (*(VFnInt*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x20);
    fn_801A6D7C(arg0, 0);
    u8* sub = *(u8**)(arg0 + 0x1B4);
    if (sub != NULL) {
        *(f32*)(sub + 0x80) = *(f32*)(sub + 0x13C);
        *(f32*)(sub + 0x84) = *(f32*)(sub + 0x140);
        *(f32*)(sub + 0x88) = *(f32*)(sub + 0x144);
        *(f32*)(sub + 0x8C) = *(f32*)(sub + 0x148);
        sub = *(u8**)(arg0 + 0x1B4);
        (*(VFnInt*)(*(u8**)(sub + 0x138) + 0x2C))(sub, 0x20);
        fn_801A6880(arg0);
    }
    fn_80196678(lbl_805FB658, arg0 + 0xF0, 0.0f);   // the original passes a leftover f1 (unused by the callee)

    // Fade timer: runs while >= 0 (-1 = idle), cosine ease from 1 to 0.
    f32 timer = *(f32*)(arg0 + 0x174);
    if (timer >= 0.0f) {
        if (timer == 0.0f) firstFrame = 1;
        f32 t = *(f32*)(arg0 + 0x174) + (dt / *(f32*)(arg0 + 0x170));
        *(f32*)(arg0 + 0x174) = t;
        if (t > 1.0f) {
            t = 1.0f;
            *(u8*)(arg0 + 0x17B) = 0;
            *(f32*)(arg0 + 0x174) = -1.0f;
            *(u8*)(arg0 + 0x17A) = 0;
            *(u8*)(arg0 + 0x179) = 0;
            *(u8*)(arg0 + 0x178) = 0;
        }
        f32 c = cosf(1.5707964f * (0.011111111f * (180.0f * t)));
        fade = 1.0f - (0.5f * (1.0f + c));
        fn_8018D1FC(lbl_80383448 + 0x140, (f64)fade);
    }
    s32 count = *(s32*)(arg0 + 0x13C) * *(s32*)(arg0 + 0x140);
    for (s32 i = 0; i < count; i++) {
        u8** slot = (u8**)fn_8019552C(*(u8**)(arg0 + 0x1BC), i);
        u8* cell = slot != NULL ? *slot : NULL;
        if (cell != NULL) {
            if (firstFrame != 0) fn_801A9E94(cell, *(f32*)(cell + 0x68), *(f32*)(cell + 0x6C), *(f32*)(cell + 0x70));
            fn_801A9CE0(cell, dt, fade);
        }
    }
    if (count > 0) {
        u8* s = *(u8**)(arg0 + 0x1B4);
        if (s != NULL) (*(VFnF32*)(*(u8**)(s + 0x138) + 0x18))(s, dt);
    }
    fn_80196B10((u8*)lbl_805FB658);
    return 0;
}

// 0x801AEE84
// Segment vs. oriented box. The segment a..b and box centre c are in world space, m is the 3x3
// box orientation (rows at +0, +0xC, +0x18), h the box half extents, margin a slab expansion.
// out receives the hit position (+0), the world normal (+0xC) and the remaining length (+0x18).
// Returns 1 on a hit inside (-margin, length + margin).
s32 fn_801AEE84(u8* a, u8* b, u8* c, u8* h, u8* m, u8* out, f32 margin) {
#define E(i) (*(f32*)(m + (i) * 4))
    f32 d0x = *(f32*)(a + 0x0) - *(f32*)(c + 0x0);
    f32 d0y = *(f32*)(a + 0x4) - *(f32*)(c + 0x4);
    f32 d1x = *(f32*)(b + 0x0) - *(f32*)(c + 0x0);
    f32 d0z = *(f32*)(a + 0x8) - *(f32*)(c + 0x8);
    f32 d1y = *(f32*)(b + 0x4) - *(f32*)(c + 0x4);
    f32 d1z = *(f32*)(b + 0x8) - *(f32*)(c + 0x8);
    // Segment ends in box space.
    f32 l0x = (E(2) * d0z) + ((E(0) * d0x) + (E(1) * d0y));
    f32 l0y = (E(5) * d0z) + ((E(3) * d0x) + (E(4) * d0y));
    f32 l0z = (E(8) * d0z) + ((E(6) * d0x) + (E(7) * d0y));
    f32 l1x = (E(2) * d1z) + ((E(0) * d1x) + (E(1) * d1y));
    f32 l1y = (E(5) * d1z) + ((E(3) * d1x) + (E(4) * d1y));
    f32 l1z = (E(8) * d1z) + ((E(6) * d1x) + (E(7) * d1y));
    f32 dx = l1x - l0x;
    f32 dy = l1y - l0y;
    f32 dz = l1z - l0z;
    f32 len2 = (dz * dz) + ((dx * dx) + (dy * dy));
    f32 hx = *(f32*)(h + 0x0), hy = *(f32*)(h + 0x4), hz = *(f32*)(h + 0x8);
    f32 box[6];   // min xyz, max xyz
    box[0] = 0.0f - hx;
    box[1] = 0.0f - hy;
    box[2] = 0.0f - hz;
    box[3] = 0.0f + hx;
    box[4] = 0.0f + hy;
    box[5] = 0.0f + hz;
    f32 len = 0.0f;
    f32 nx, ny, nz;
    if (len2 == 0.0f) {
        nx = ny = nz = 0.0f;
    } else {
        if (len2 > 0.0f) len = len2 * (1.0f / sqrtf(len2));   // original: frsqrte with two Newton steps
        f32 inv = 1.0f / len;
        nx = dx * inv;
        ny = dy * inv;
        nz = dz * inv;
    }
    box[0] -= margin;
    box[1] -= margin;
    box[2] -= margin;
    box[3] += margin;
    box[4] += margin;
    box[5] += margin;
    s32 xneg = nx < 0.0f;
    s32 yneg = ny < 0.0f;
    s32 zneg = nz < 0.0f;
    if (nx == 0.0f) nx = 0.00001f;
    if (ny == 0.0f) ny = 0.00001f;
    if (nz == 0.0f) nz = 0.00001f;
    f32 ix = 1.0f / nx;
    f32 iy = 1.0f / ny;
    f32 iz = 1.0f / nz;
    s32 hit = 0;
    do {
        // Slab test, x then y then z; a negative direction swaps near and far planes.
        f32 txEnter = (box[xneg * 3 + 0] - l0x) * ix;
        f32 txExit = (box[(1 - xneg) * 3 + 0] - l0x) * ix;
        f32 tyExit = (box[(1 - yneg) * 3 + 1] - l0y) * iy;
        f32 tyEnter = (box[yneg * 3 + 1] - l0y) * iy;
        if (txEnter > tyExit || tyEnter > txExit) break;
        if (tyEnter > txEnter) txEnter = tyEnter;
        if (tyExit < txExit) txExit = tyExit;
        f32 tzExit = (box[(1 - zneg) * 3 + 2] - l0z) * iz;
        f32 tzEnter = (box[zneg * 3 + 2] - l0z) * iz;
        if (txEnter > tzExit || tzEnter > txExit) break;
        if (tzEnter > txEnter) txEnter = tzEnter;
        f32 t = txEnter;
        f32 px = l0x + (nx * t);
        f32 py = l0y + (ny * t);
        f32 pz = l0z + (nz * t);
        *(f32*)(out + 0xC) = -nx;
        *(f32*)(out + 0x10) = -ny;
        *(f32*)(out + 0x14) = -nz;
        *(f32*)(out + 0x0) = px;
        *(f32*)(out + 0x4) = py;
        *(f32*)(out + 0x8) = pz;
        *(f32*)(out + 0x18) = (1.0f - t) * len;
        if (t > -margin && t < len + margin) hit = 1;
    } while (0);

    // Rotate the normal and the hit position back to world space (this also runs on a miss,
    // using whatever out holds).
    f32 ox = *(f32*)(out + 0xC), oy = *(f32*)(out + 0x10), oz = *(f32*)(out + 0x14);
    f32 vx = (E(6) * oz) + ((E(0) * ox) + (E(3) * oy));
    f32 vy = (E(7) * oz) + ((E(1) * ox) + (E(4) * oy));
    f32 vz = (E(8) * oz) + ((E(2) * ox) + (E(5) * oy));
    f32 vlen2 = (vz * vz) + ((vx * vx) + (vy * vy));
    f32 wx, wy, wz;
    if (vlen2 == 0.0f) {
        wx = wy = wz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(vlen2);   // original: frsqrte with two Newton steps
        wx = vx * inv;
        wy = vy * inv;
        wz = vz * inv;
    }
    *(f32*)(out + 0xC) = wx;
    *(f32*)(out + 0x10) = wy;
    *(f32*)(out + 0x14) = wz;
    f32 qx = *(f32*)(out + 0x0), qy = *(f32*)(out + 0x4), qz = *(f32*)(out + 0x8);
    f32 rx = (E(6) * qz) + ((E(0) * qx) + (E(3) * qy));
    f32 ry = (E(7) * qz) + ((E(1) * qx) + (E(4) * qy));
    f32 rz = (E(8) * qz) + ((E(2) * qx) + (E(5) * qy));
    *(f32*)(out + 0x0) = rx + *(f32*)(c + 0x0);
    *(f32*)(out + 0x4) = ry + *(f32*)(c + 0x4);
    *(f32*)(out + 0x8) = rz + *(f32*)(c + 0x8);
#undef E
    return hit;
}

// 0x801B0644
// Collision test between shape a and shape c, dispatched on c's shape type (+0xC): 0 = box vs
// sphere-like (separating-axis test, fills the contact normal at +0xC, depth at +0x18 and the
// contact point at +0 of o1, or of o2 when useSecond), 4/5/6/7 = the specialised queries.
// flag is cleared by the box test when the z overlap is positive. The r8 argument is unused.
// Returns 1 on contact.
s32 fn_801B0644(u8* a, f32* o1, u8* c, f32* o2, s32 flag, s32 unused, s32 useSecond) {
    switch (*(s32*)(c + 0xC)) {
    case 0: {
        f32 dx = *(f32*)(c + 0x14) - *(f32*)(a + 0x14);
        f32 dy = *(f32*)(c + 0x18) - *(f32*)(a + 0x18);
        f32 dz = *(f32*)(c + 0x1C) - *(f32*)(a + 0x1C);
        f32 px = (*(f32*)(a + 0x44) * dz) + ((*(f32*)(a + 0x3C) * dx) + (*(f32*)(a + 0x40) * dy));
        f32 py = (*(f32*)(a + 0x50) * dz) + ((*(f32*)(a + 0x48) * dx) + (*(f32*)(a + 0x4C) * dy));
        f32 pz = (*(f32*)(a + 0x5C) * dz) + ((*(f32*)(a + 0x54) * dx) + (*(f32*)(a + 0x58) * dy));
        f32 size = *(f32*)(c + 0x3C);
        f32 ox = (size + *(f32*)(a + 0x6C)) - fabsf(px);
        if (!(ox >= 0.0001f)) return 0;
        f32 oy = (size + *(f32*)(a + 0x70)) - fabsf(py);
        if (!(oy >= 0.0001f)) return 0;
        f32 oz = (size + *(f32*)(a + 0x74)) - fabsf(pz);
        if (!(oz >= 0.0001f)) return 0;
        if (oz > 0.0f) flag = 0;
        // Axis of least penetration.
        s32 axis;
        if (fabsf(ox) < fabsf(oy)) {
            axis = (flag != 0 || fabsf(ox) < fabsf(oz)) ? 0 : 2;
        } else {
            axis = (flag != 0 || fabsf(oy) < fabsf(oz)) ? 1 : 2;
        }
        f32 n[3] = { 0.0f, 0.0f, 0.0f };
        f32 depth;
        if (axis == 0) {
            depth = fabsf(ox);
            n[0] = px < 0.0f ? -1.0f : 1.0f;
        } else if (axis == 1) {
            depth = fabsf(oy);
            n[1] = py < 0.0f ? -1.0f : 1.0f;
        } else {
            depth = fabsf(oz);
            n[2] = pz < 0.0f ? -1.0f : 1.0f;
        }
        // Normal to world space.
        f32 wx = (*(f32*)(a + 0x54) * n[2]) + ((*(f32*)(a + 0x3C) * n[0]) + (*(f32*)(a + 0x48) * n[1]));
        f32 wy = (*(f32*)(a + 0x58) * n[2]) + ((*(f32*)(a + 0x40) * n[0]) + (*(f32*)(a + 0x4C) * n[1]));
        f32 wz = (*(f32*)(a + 0x5C) * n[2]) + ((*(f32*)(a + 0x44) * n[0]) + (*(f32*)(a + 0x50) * n[1]));
        f32 wlen2 = (wz * wz) + ((wx * wx) + (wy * wy));
        f32 ux, uy, uz;
        if (wlen2 == 0.0f) {
            ux = uy = uz = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(wlen2);   // original: frsqrte with two Newton steps
            ux = wx * inv;
            uy = wy * inv;
            uz = wz * inv;
        }
        f32* t = useSecond != 0 ? o2 : o1;
        t[3] = ux;
        t[4] = uy;
        t[5] = uz;
        t[6] = depth;
        f32 cx = t[3] * depth;
        f32 cy = t[4] * depth;
        f32 cz = t[5] * depth;
        t[0] = *(f32*)(c + 0x14) + cx;
        t[1] = *(f32*)(c + 0x18) + cy;
        t[2] = *(f32*)(c + 0x1C) + cz;
        return 1;
    }
    case 4:
        return fn_801C098C(a + 0x14, a + 0x6C, a + 0x3C, c + 0x14, c + 0x3C, (f32*)((u8*)o1 + 0xC), (f32*)((u8*)o2 + 0xC));
    case 5: {
        s32 n;
        f32 pos[3];
        f32 dist;
        if (fn_801C217C(a + 0x14, a + 0x6C, a + 0x3C, c + 0x3C, c + 0x48, &n, pos, &dist) != 0) {
            if (n > 0) {
                o2[0] = pos[0];
                o2[1] = pos[1];
                o2[2] = pos[2];
                o2[6] = dist;
            }
            return 1;
        }
        return 0;
    }
    case 6: {
        s32 n;
        f32 pos[3];
        f32 dist;
        if (fn_801C26A8(a + 0x14, a + 0x6C, a + 0x3C, c + 0x3C, c + 0x48, &n, pos, &dist) != 0) {
            if (n > 0) {
                o2[0] = pos[0];
                o2[1] = pos[1];
                o2[2] = pos[2];
                o2[6] = dist;
            }
            return 1;
        }
        return 0;
    }
    case 7:
        return fn_801C10B4(a + 0x14, a + 0x6C, a + 0x3C, c + 0x14, c + 0x6C, c + 0x3C);
    default:
        return 0;
    }
}

// Looks up the glyph record for character ch in the font table's hash chain (0x10-byte records,
// first u16 = character code, chains stay within the same ch & 0x3F bucket). NULL when absent.
static u8* FontFindGlyph(u8* table, u8 ch) {
    u8* blk = *(u8**)(table + 0x10);
    u8* p = blk + 0x8C + blk[3] * 4 + *(u16*)(blk + (ch & 0x3F) * 2 + 0xC) * 0x10;
    for (;;) {
        u32 v = *(u16*)p;
        if (v == 0) return NULL;
        if (v == ch) return p;
        if ((v & 0x3F) != (u32)(ch & 0x3F)) return NULL;
        p += 0x10;
    }
}

// Queues one glyph quad: takes a node from the table's free list (+0xC), appends it to the list of
// the glyph's texture page (0xC-byte heads at +4, page chosen through the two-entry cache at +0x14)
// and fills in its position, size, tag and page offset.
static void FontQueueGlyph(u8* table, u8* g, s32 x, s32 y, s32 tag) {
    if ((s8)g[0xC] == 0) return;
    s32 page = g[0xE];
    s32 idx;
    s16 c0 = *(s16*)(table + 0x14);
    if (c0 == page) {
        idx = *(s16*)(table + 0x18);
    } else {
        s16 c1 = *(s16*)(table + 0x16);
        if (c1 == page) {
            idx = *(s16*)(table + 0x1A);
            s16 i0 = *(s16*)(table + 0x18);
            *(s16*)(table + 0x16) = c0;
            *(s16*)(table + 0x14) = c1;
            *(s16*)(table + 0x1A) = i0;
            *(s16*)(table + 0x18) = (s16)idx;
        } else {
            idx = fn_80187704(table, page);
        }
    }
    u8* node = *(u8**)(table + 0xC);
    if (node == NULL) return;
    *(u8**)(table + 0xC) = *(u8**)(node + 0x0);
    *(u8**)(node + 0x0) = NULL;
    u8* head = *(u8**)(table + 0x4) + idx * 0xC;
    u8** tail = *(u8***)(head + 0x8);
    if (tail != NULL) *tail = node;
    else *(u8**)(head + 0x4) = node;
    *(u8**)(*(u8**)(table + 0x4) + idx * 0xC + 0x8) = node;
    *(s32*)(node + 0x4) = tag;
    *(s16*)(node + 0xC) = (s16)(x + (s8)g[0xA]);
    *(s16*)(node + 0xE) = (s16)(y + (s8)g[0xB]);
    *(u16*)(node + 0x10) = *(u16*)(g + 0x2);
    *(u16*)(node + 0x12) = *(u16*)(g + 0x4);
    *(s16*)(node + 0x8) = (s8)g[0xC];
    *(s16*)(node + 0xA) = (s8)g[0xD];
}

// 0x80187798
// Lays out len characters of text at (x, y) with the font table: queues a quad per glyph and
// advances by the glyph advance plus the pair kerning (looked up in the table at +8 of the block).
void fn_80187798(u8* table, const char* text, s32 len, s32 x, s32 y, s32 tag) {
    const u8* s = (const u8*)text;
    for (;;) {
        // Find the first glyph.
        u8* g;
        do {
            if (--len < 0) return;
            g = FontFindGlyph(table, *s++);
        } while (g == NULL);
        FontQueueGlyph(table, g, x, y, tag);
        x += (s8)g[0x8];
        // Following glyphs: apply kerning against the previous glyph.
        for (;;) {
            if (--len < 0) return;
            u8 ch = *s++;
            u8* g2 = FontFindGlyph(table, ch);
            if (g2 == NULL) continue;
            s32 kern = 0;
            if ((g[0xF] & 2) != 0 && (g2[0xF] & 1) != 0) {
                u8* blk = *(u8**)(table + 0x10);
                u16 prev = *(u16*)(g + 0x0);
                u16 cur = *(u16*)(g2 + 0x0);
                u8* k = blk + 0x8C + blk[3] * 4 + *(u16*)(blk + 0x8) * 0x10 + *(u16*)(g + 0x6) * 6;
                for (;;) {
                    if (*(u16*)k != prev) break;
                    u16 v = *(u16*)(k + 0x2);
                    if (v > cur) break;
                    if (v == cur) {
                        kern = *(s16*)(k + 0x4);
                        break;
                    }
                    k += 6;
                }
            }
            g = g2;
            x += kern;
            FontQueueGlyph(table, g, x, y, tag);
            x += (s8)g[0x8];
        }
    }
}

}  // extern "C"
