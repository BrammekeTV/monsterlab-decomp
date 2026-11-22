// Drafted functions (round 675): free-look camera update, lab part list builder, UI text/element
// updates (0x8019CF58, 0x801A05E4), collision dispatcher by shape type, recorded-path peak scan.
#include "engine/math.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void   BufferFree(void* p);                                                 // 0x801D4980
u32    strlen(const char* s);
void   StickRead(s32 channel, f32* x, f32* y);                              // 0x80147704
void   UISetText(void* el, const char* text, s32 mode);                     // 0x8019EFC8
BOOL   SavePartTierOwned(u8* s, u32 part, s32 tier);                        // 0x802241F0
BOOL   SavePartTierSeen(u8* s, u32 part, s32 tier);                         // progress.cpp
void   fn_801458B0(f32* quat, f32* a, f32* b);                              // 0x801458B0
u8*    fn_801483BC(s32 idx);                                                // 0x801483BC
void*  fn_8011AE30(PartDef* def, s32 a);                                    // 0x8011AE30
void*  fn_8011AEE8(u8* out, s32 idx, u8* end);                              // 0x8011AEE8
u8*    fn_801189C4(s32 idx);                                                // 0x801189C4
u8*    fn_80118B1C(s32 idx);                                                // 0x80118B1C
s32    fn_80109A0C(s32 id);                                                 // 0x80109A0C
s32    fn_8019B6CC(u8* arg0, f32 farg0);                                    // 0x8019B6CC (UI element base update)
s32    fn_8019EB94(void* el);                                               // 0x8019EB94
s32    fn_8019F5A8(void* el);                                               // 0x8019F5A8
void   fn_8019E27C(void* el);                                               // 0x8019E27C
void   fn_8019E838(u8* el, f32 dt);                                         // 0x8019E838
u8*    fn_80196B24(u8* obj);                                                // 0x80196B24 (top of the transform stack)
u8*    fn_80196C9C(void* p);                                                // 0x80196C9C (current tint record)
s32    fn_801A1238(u8* obj, f32* colors);                                   // 0x801A1238 (4 vertex colours)
void   fn_801F76D4(u8* ring, u8* out);                                      // 0x801F76D4 (ring scan, fills 0x1C-byte result)
void*  MemAllocDefault(u32 size);                                           // 0x801D4650

void   Vec3Copy(Vec3* out, const Vec3* v);                                  // 0x80052640
void   Vec3Assign(Vec3* out, const Vec3* v);                                // 0x80053354
void   Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);                    // 0x80053370
f32    Vec3LengthSqOf(const Vec3* v);                                       // 0x80054A90
void   fn_80052638(Vec3* v);                                                // 0x80052638 (empty leaf)
u8*    fn_801ADF4C(u8* a3);                                                 // 0x801ADF4C
u8*    fn_801ADF54(u8* a3);                                                 // 0x801ADF54
u8*    fn_801ADF5C(u8* a3);                                                 // 0x801ADF5C
u8*    fn_801ADF64(u8* a3);                                                 // 0x801ADF64
s32    fn_801AD47C(u8* a, u8* b, u8* c, u8* out, s32 flag, f32 margin);     // 0x801AD47C
s32    fn_801AD6AC(u8* a, u8* ext, u8* rot, u8* b, u8* out, s32 flag, f32 margin);   // 0x801AD6AC
s32    fn_801ADB44(u8* arg0, u8* arg1, u8* arg2, s32 arg3, u8* arg4, f32 farg0, f32 farg1);   // 0x801ADB44
s32    fn_801AE544(u8* arg0, u8* arg1, u8* arg2, f32 farg0, f32 farg1);     // 0x801AE544
s32    fn_801AE9AC(u8* p0, u8* p1, u8* center, u8* half, u8* out, f32 margin);   // 0x801AE9AC
s32    fn_801AEE84(u8* a, u8* b, u8* c, u8* h, u8* m, u8* out, f32 margin); // 0x801AEE84
s32    fn_801AF770(u8* c, u8* a, u8* b, u8* out, s32 flag, f32 r0, f32 r1); // 0x801AF770 (r7 is overwritten inside)
s32    fn_801C2C60(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist);   // 0x801C2C60
s32    fn_801C3170(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist);   // 0x801C3170

extern u8        gGame[];            // 0x803B53A0
extern u8        lbl_803C4298[];     // 0x803C4298 (+0x38: pad channel)
extern u8*       lbl_805FBBE0;       // 0x805FBBE0 (save block pointer)
extern u8        lbl_805FB2B0[];     // 0x805FB2B0
extern void*     lbl_805FB658;       // 0x805FB658 (global render object)
extern const char lbl_805F8200[];    // 0x805F8200 (empty string)
extern const char lbl_80354800[];    // 0x80354800 ("Not found")
extern const char lbl_8035480C[];    // 0x8035480C ("NOT FOUND")

extern f32 lbl_805FEA88;             // 0x805FEA88 (0.0)
extern f32 lbl_805FEA8C;             // 0x805FEA8C (1.0)
extern f32 lbl_805FEAA8;             // 0x805FEAA8 (-pi)
extern f32 lbl_805FEAAC;             // 0x805FEAAC (+pi)
extern f32 lbl_805FEAB0;             // 0x805FEAB0 (0.8)
extern f32 lbl_805FEAB4;             // 0x805FEAB4 (-0.5)
extern f32 lbl_805FEAB8;             // 0x805FEAB8 (0.5)
extern f32 lbl_805FEABC;             // 0x805FEABC (-0.1)
extern f32 lbl_805FEAC0;             // 0x805FEAC0 (0.1)
extern f32 lbl_805FEAC4;             // 0x805FEAC4 (180.0)
extern f32 lbl_805FEAC8;             // 0x805FEAC8 (max pitch, degrees)
extern f32 lbl_805FEACC;             // 0x805FEACC (degrees to radians)
extern f32 lbl_805FEAD0;             // 0x805FEAD0 (90.0)
extern f32 lbl_805FEAD4;             // 0x805FEAD4 (-2pi)
extern f32 lbl_805FEAD8;             // 0x805FEAD8 (+2pi)
extern f32 lbl_805FEADC;             // 0x805FEADC (25.0)
extern f32 lbl_805FEAE0;             // 0x805FEAE0 (1.5)
extern f32 lbl_805FF998;             // 0x805FF998 (1/255)
extern f32 lbl_805FF99C;             // 0x805FF99C (255.0)
extern f32 lbl_806001AC;             // 0x806001AC (1.0)
extern f32 lbl_806001C0;             // 0x806001C0 (0.0)
}

extern "C++" f32 GetSign10(u8* p);   // 0x80013F70
struct StringTable;
extern "C++" StringTable* gStringTable;   // 0x805FB910
extern "C++" void* gStringTable2;         // 0x805FB90C
extern "C++" void* gScreenSets;           // 0x805FB654

typedef s32 (*UIVirtFn)(u8* self, s32 a);

// Wraps an angle: clamp to +-2pi, fold into +-pi, clamp again (as the camera code does inline).
static f32 WrapAngle(f32 v) {
    if (v < lbl_805FEAD4) v = lbl_805FEAD4;
    else if (v > lbl_805FEAD8) v = lbl_805FEAD8;
    if (v < lbl_805FEAA8) v = v + lbl_805FEAD8;
    else if (v > lbl_805FEAAC) v = v - lbl_805FEAD8;
    if (v < lbl_805FEAA8) v = lbl_805FEAA8;
    else if (v > lbl_805FEAAC) v = lbl_805FEAAC;
    return v;
}

// Resolves a string id (bit 31 selects the secondary table); -1 gives "Not found".
static const char* StrLookup(s32 id) {
    if (id == -1) return lbl_80354800;
    u32 u = (u32)id;
    if ((u & 0x80000000u) != 0 && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** arr = *(const char***)(t + 0x0);
        if (arr != NULL && (u & 0x7FFFFFFFu) < *(u32*)(t + 0xC)) return arr[u & 0x7FFFFFFFu];
        return lbl_805F8200;
    }
    u8* t = (u8*)gStringTable;
    const char** arr = *(const char***)(t + 0x0);
    if (arr != NULL && u < *(u32*)(t + 0xC)) return arr[u];
    return lbl_805F8200;
}

// Same lookup through the second string array at +4 of each table; NULL when missing.
static const char* StrLookupAlt(u32 id) {
    if ((id & 0x80000000u) != 0 && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** arr = *(const char***)(t + 0x4);
        if (arr != NULL && (id & 0x7FFFFFFFu) < *(u32*)(t + 0xC)) return arr[id & 0x7FFFFFFFu];
        return NULL;
    }
    u8* t = (u8*)gStringTable;
    const char** arr = *(const char***)(t + 0x4);
    if (arr != NULL && id < *(u32*)(t + 0xC)) return arr[id];
    return NULL;
}

// Replaces every CR/LF of the element's text (+0x170) with a space.
static void FlattenText(char* s) {
    while (*s != 0) {
        if (*s == 10 || *s == 13) *s = 0x20;
        s++;
    }
}

// Composes the element's 3x4 matrix at +0xF0 with the matrix m and writes the result to o.
static void ComposeMatrix(const u8* m, const u8* e, u8* o) {
    f32 m0 = *(f32*)(m + 0x00), m1 = *(f32*)(m + 0x04), m2 = *(f32*)(m + 0x08);
    f32 m3 = *(f32*)(m + 0x0C), m4 = *(f32*)(m + 0x10), m5 = *(f32*)(m + 0x14);
    f32 m6 = *(f32*)(m + 0x18), m7 = *(f32*)(m + 0x1C), m8 = *(f32*)(m + 0x20);
    f32 m9 = *(f32*)(m + 0x24), m10 = *(f32*)(m + 0x28), m11 = *(f32*)(m + 0x2C);
    f32 a0 = *(f32*)(e + 0xF0), a1 = *(f32*)(e + 0xF4), a2 = *(f32*)(e + 0xF8);
    f32 a3 = *(f32*)(e + 0xFC), a4 = *(f32*)(e + 0x100), a5 = *(f32*)(e + 0x104);
    f32 a6 = *(f32*)(e + 0x108), a7 = *(f32*)(e + 0x10C), a8 = *(f32*)(e + 0x110);
    f32 a9 = *(f32*)(e + 0x114), a10 = *(f32*)(e + 0x118), a11 = *(f32*)(e + 0x11C);
    *(f32*)(o + 0x00) = (m6 * a2) + ((m0 * a0) + (m3 * a1));
    *(f32*)(o + 0x04) = (m7 * a2) + ((m1 * a0) + (m4 * a1));
    *(f32*)(o + 0x08) = (m8 * a2) + ((m2 * a0) + (m5 * a1));
    *(f32*)(o + 0x0C) = (m6 * a5) + ((m0 * a3) + (m3 * a4));
    *(f32*)(o + 0x10) = (m7 * a5) + ((m1 * a3) + (m4 * a4));
    *(f32*)(o + 0x14) = (m8 * a5) + ((m2 * a3) + (m5 * a4));
    *(f32*)(o + 0x18) = (m6 * a8) + ((m0 * a6) + (m3 * a7));
    *(f32*)(o + 0x1C) = (m7 * a8) + ((m1 * a6) + (m4 * a7));
    *(f32*)(o + 0x20) = (m8 * a8) + ((m2 * a6) + (m5 * a7));
    *(f32*)(o + 0x24) = m9 + ((m6 * a11) + ((m0 * a9) + (m3 * a10)));
    *(f32*)(o + 0x28) = m10 + ((m7 * a11) + ((m1 * a9) + (m4 * a10)));
    *(f32*)(o + 0x2C) = m11 + ((m8 * a11) + ((m2 * a9) + (m5 * a10)));
}

// Copies the element's 12 matrix floats (+0xF0) to o.
static void CopyMatrix(const u8* e, u8* o) {
    for (s32 i = 0; i < 12; i++) *(f32*)(o + i * 4) = *(f32*)(e + 0xF0 + i * 4);
}

extern "C" {

// 0x800E59C0
// Free-look camera update: the stick turns yaw (+0x98, wrapped to +-pi and limited to the range
// at +0x68..+0x74), pitch (+0x9C) is limited, the velocity at +0x8C..+0x94 is smoothed and
// rotated by the view orientation into the position at +0x80. outDir gets the view direction and
// outPos the position pulled back 0.8 along it. Returns 0 if the enable byte at +0x54 is clear.
s32 fn_800E59C0(u8* p, f32* outPos, f32* outDir, f32 dt) {
    if (*(u8*)(p + 0x54) == 0) return 0;
    f32 yawIn = lbl_805FEA88;
    f32 pitchIn = yawIn;
    f32 vel[3];                 // sp+0xAC, +0xB0, +0xB4 (never filled by the stick, stay 0)
    vel[0] = lbl_805FEA88;
    vel[1] = lbl_805FEA88;
    vel[2] = lbl_805FEA88;
    f32 sx, sy;                 // sy is read but unused
    StickRead(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0xE, &sx, &sy);
    if (*(u8*)(p + 0xA4) != 0) {
        sx = (*(f32*)(p + 0x98) < *(f32*)(p + 0xA0)) ? lbl_805FEAB4 : lbl_805FEAB8;
    }
    if (sx < lbl_805FEABC || lbl_805FEAC0 < sx) {
        yawIn = sx * GetSign10(gGame + 0xC448);
    }
    f32 maxPitch = lbl_805FEAC8;
    f32 yawStep = lbl_805FEAD0 * dt;
    f32 pitchStep = dt * lbl_805FEAC4;
    yawStep = lbl_805FEACC * yawStep;
    pitchStep = lbl_805FEACC * pitchStep;
    f32 dYaw = yawStep * yawIn;
    f32 dPitch = pitchIn * pitchStep;

    // Would the new yaw leave the allowed range? Then no turning this frame.
    f32 trial = WrapAngle(*(f32*)(p + 0x98) + dYaw);
    f32 c = cosf(trial) * cosf(*(f32*)(p + 0x9C));
    f32 lo = *(f32*)(p + 0x68);
    f32 hi = *(f32*)(p + 0x74);
    s32 outOfRange = 0;
    if (hi > lo) {
        if (c < lo || c > hi) outOfRange = 1;
    } else if (hi < lo) {
        if (c > lo || c < hi) outOfRange = 1;
    } else {
        outOfRange = 1;
    }
    if (outOfRange != 0) {
        dYaw = lbl_805FEA88;
        dPitch = dYaw;
    }

    f32 oldYaw = *(f32*)(p + 0x98);
    f32 newYaw = oldYaw + dYaw;
    *(f32*)(p + 0x98) = newYaw;
    newYaw = WrapAngle(newYaw);
    *(f32*)(p + 0x98) = newYaw;
    if (*(u8*)(p + 0xA4) != 0) {
        f32 target = *(f32*)(p + 0xA0);
        if ((oldYaw > target && newYaw <= target) || (oldYaw < target && *(f32*)(p + 0x98) >= target)) {
            *(f32*)(p + 0x98) = *(f32*)(p + 0xA0);
        }
    }

    f32 pitch = *(f32*)(p + 0x9C) + dPitch;
    f32 pitchMax = lbl_805FEACC * maxPitch;
    *(f32*)(p + 0x9C) = pitch;
    if (pitch >= pitchMax) pitch = pitchMax;
    f32 pitchMin = lbl_805FEACC * -maxPitch;
    *(f32*)(p + 0x9C) = pitch;
    if (pitch <= pitchMin) pitch = pitchMin;
    *(f32*)(p + 0x9C) = pitch;

    // Smooth the velocity toward the (scaled) stick velocity.
    f32 scale = lbl_805FEADC;
    f32 blend;
    vel[0] = vel[0] * scale;
    vel[1] = vel[1] * scale;
    vel[2] = vel[2] * scale;
    blend = lbl_805FEAE0 * dt;
    for (s32 i = 0; i < 3; i++) {
        f32 cur = *(f32*)(p + 0x8C + i * 4);
        if (fabsf(vel[i]) < fabsf(cur)) {
            *(f32*)(p + 0x8C + i * 4) = vel[i];
        } else {
            *(f32*)(p + 0x8C + i * 4) = (blend * vel[i]) + ((lbl_805FEA8C - blend) * cur);
        }
    }

    f32 s1 = sinf(*(f32*)(p + 0x98));
    f32 c1 = cosf(*(f32*)(p + 0x98));
    f32 s2 = sinf(*(f32*)(p + 0x9C));
    f32 c2 = cosf(*(f32*)(p + 0x9C));
    outDir[2] = s2;
    outDir[0] = c1 * c2;
    outDir[1] = s1 * c2;

    f32 up[3];
    up[0] = lbl_805FEA88;
    up[1] = lbl_805FEA88;
    up[2] = lbl_805FEA8C;
    f32 v[3];
    v[0] = *(f32*)(p + 0x8C) * dt;
    v[1] = *(f32*)(p + 0x90) * dt;
    v[2] = *(f32*)(p + 0x94) * dt;
    f32 q[4];
    fn_801458B0(q, outDir, up);

    // Rotate v by the quaternion q (q * v * conj(q)) and add it to the position.
    f32 zero = lbl_805FEA88;
    f32 q0 = q[0], q1 = q[1], q2 = q[2], q3 = q[3];
    f32 nq0 = -q0, nq1 = -q1, nq2 = -q2;
    f32 t6 = ((q1 * v[2]) + ((q3 * v[0]) + (q0 * zero))) - (q2 * v[1]);
    f32 t5 = (q2 * v[0]) + ((q1 * zero) + ((q3 * v[1]) - (q0 * v[2])));
    f32 t3 = (((q3 * zero) - (q0 * v[0])) - (q1 * v[1])) - (q2 * v[2]);
    f32 t4 = (q2 * zero) + (((q3 * v[2]) + (q0 * v[1])) - (q1 * v[0]));
    f32 rx = ((t5 * nq2) + ((t3 * nq0) + (t6 * q3))) - (t4 * nq1);
    f32 ry = (t4 * nq0) + ((t5 * q3) + ((t3 * nq1) - (t6 * nq2)));
    f32 rz = (t4 * q3) + (((t3 * nq2) + (t6 * nq1)) - (t5 * nq0));
    f32 px = *(f32*)(p + 0x80) + rx;
    f32 py = *(f32*)(p + 0x84) + ry;
    f32 pz = *(f32*)(p + 0x88) + rz;
    *(f32*)(p + 0x84) = py;
    *(f32*)(p + 0x80) = px;
    *(f32*)(p + 0x88) = pz;

    f32 back = lbl_805FEAB0;
    outPos[0] = px - (outDir[0] * back);
    outPos[1] = py - (outDir[1] * back);
    outPos[2] = pz - (outDir[2] * back);
    return 1;
}

// 0x8011F5DC
// Builds the lab part-list rows (stride 0x3C at the list base, count at +0x9260) from the part
// catalog: every part/tier the save has seen that passes the two category filters (+0xA650 and
// +0xA654) gets an icon, names, and (if owned) its enhancement icon and name.
void fn_8011F5DC(u8* p) {
    *(s32*)(p + 0x9260) = 0;
    *(s32*)(p + 0x925C) = 0;
    *(s32*)(p + 0xA684) = 0;
    if (*(s32*)(p + 0xA64C) == -1) {
        *(s32*)(p + 0xA650) = 3;
        *(s32*)(p + 0xA654) = 5;
    }
    *(s32*)(p + 0x8) = 5;
    *(s32*)(p + 0xA648) = 0;
    lbl_805FB2B0[0] = 0;
    s32 filter = *(s32*)(p + 0xA654);
    *(s32*)(p + 0xA688) = -1;
    *(s32*)(p + 0x4) = filter;
    if (filter == 5) {
        *(s32*)(p + 0x4) = 1;
    } else if (filter < 3) {
        *(s32*)(p + 0x4) = filter + 2;
    } else {
        *(s32*)(p + 0x4) = filter + 1;
    }

    for (s32 part = 0; part < 0x9C; part++) {
        for (u32 tier = 1; tier <= 4; tier++) {
            if (SavePartTierSeen(lbl_805FBBE0, (u32)part, (s32)(u8)tier) == 0) continue;

            u8* entry = *(u8**)(gGameDb->data + 0x6C18) + part * 0x24;
            u32 partKey = *(u32*)(entry + 0x0);
            s32 entryKind = *(s32*)(entry + 0x8);
            s32 iconIdx = *(s32*)(entry + 0x10);
            s32 entryAux = *(s32*)(entry + 0x14);
            s32 defIndex = *(s32*)(entry + 0x20);
            PartDef def;
            PartDbGetDef(&def, gGameDb, defIndex);
            PartDef copy = def;

            s32 kindFilter = *(s32*)(p + 0xA650);
            if (kindFilter != 3 && entryKind != kindFilter) continue;
            s32 slotFilter = *(s32*)(p + 0xA654);
            if (slotFilter != 5 && copy.slot != slotFilter) {
                if ((u32)(copy.slot - 2) > 1u) continue;
                if (slotFilter != 2 && slotFilter != 3) continue;
            }

            u8* row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(void**)(row + 0x24) = fn_8011AEE8((u8*)&copy, (s32)(u8)tier, (u8*)&copy + 0x4C);
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(f32*)(row + 0x54) = (f32)(u8)tier;
            *(void**)(row + 0x20) = fn_8011AE30(&copy, 0);
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(s32*)(row + 0x28) = copy.discipline;
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(s32*)(row + 0x2C) = copy.slot;
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(s32*)(row + 0x30) = part;
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(s32*)(row + 0x34) = defIndex;

            u8* nameRec = *(u8**)(gGameDb->data + 0x4F00) + iconIdx * 0x10;
            u32 nameHash = *(u32*)(nameRec + 0x4);
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(u8**)(row + 0x3C) = fn_801189C4(iconIdx);
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            if (fn_80109A0C((s32)(intptr_t)*(u8**)(row + 0x3C)) != 0) {  // lint:ok id slot, see fn_80109A0C
                row = p + *(s32*)(p + 0x9260) * 0x3C;
                *(u8**)(row + 0x38) = *(u8**)(row + 0x3C);
                row = p + *(s32*)(p + 0x9260) * 0x3C;
                *(u8**)(row + 0x3C) = NULL;
            }
            const char* text = StrLookup(TextIdFromHash(nameHash));
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            *(const char**)(row + 0x40) = text;

            // Find the enhancement record (stride 0x18) matching this entry and tier.
            s32 found = -1;
            s32 count = *(s32*)(gGameDb->data + 0x532C);
            for (s32 k = 0; k < count; k++) {
                u8* rec = *(u8**)(gGameDb->data + 0x5328) + k * 0x18;
                if (*(s32*)(rec + 0xC) == entryKind && *(s32*)(rec + 0x8) == entryAux &&
                    (s8)*(u8*)(rec + 0x13) == (s32)(u8)tier) {
                    found = k;
                    break;
                }
            }
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            if (found != -1) {
                *(s32*)(row + 0x50) = SavePartTierOwned(lbl_805FBBE0, partKey, (s32)(u8)tier);
            } else {
                *(s32*)(row + 0x50) = 0;
            }
            row = p + *(s32*)(p + 0x9260) * 0x3C;
            if (*(s32*)(row + 0x50) != 0) {
                u8* rec = *(u8**)(gGameDb->data + 0x5328) + found * 0x18;
                s32 enhId = *(s32*)(rec + 0x0);
                u32 enhHash = *(u32*)(rec + 0x4);
                *(u8**)(row + 0x48) = fn_80118B1C(enhId);
                row = p + *(s32*)(p + 0x9260) * 0x3C;
                if (fn_80109A0C((s32)(intptr_t)*(u8**)(row + 0x48)) != 0) {  // lint:ok id slot, see fn_80109A0C
                    row = p + *(s32*)(p + 0x9260) * 0x3C;
                    *(u8**)(row + 0x44) = *(u8**)(row + 0x48);
                    row = p + *(s32*)(p + 0x9260) * 0x3C;
                    *(u8**)(row + 0x48) = NULL;
                }
                const char* enhText = StrLookup(TextIdFromHash(enhHash));
                row = p + *(s32*)(p + 0x9260) * 0x3C;
                *(const char**)(row + 0x4C) = enhText;
            } else {
                *(u8**)(row + 0x44) = NULL;
            }
            *(s32*)(p + 0x9260) = *(s32*)(p + 0x9260) + 1;
        }
    }
}

// 0x8019CF58
// UIText element update: refreshes the cached string for the text id at +0x16C (calling
// UISetText when it changed), flattens CR/LF in the text, rebuilds the 3x4 matrix at +0x17C from
// the element matrix and the render stack top when flag 0x10 is set, then runs the layout steps.
s32 fn_8019CF58(u8* p, f32 dt) {
    gStringTable2 = *(void**)(p + 0x13C);
    fn_8019B6CC(p, dt);
    if (*(u8*)(p + 0x1F0) == 0) {
        s32 id = *(s32*)(p + 0x16C);
        if (id != -1) {
            const char* alt = StrLookupAlt((u32)id);
            if (*(const char**)(p + 0x140) != alt) {
                *(const char**)(p + 0x140) = alt;
                *(s32*)(p + 0x144) = 0;
                *(u8*)(p + 0x148) = 0;
            }
            UISetText(p, StrLookup(id), 1);
            *(s32*)(p + 0x16C) = id;
            *(u8*)(p + 0x1F0) = 0;
        }
        *(u8*)(p + 0x1F0) = 1;
    }
    if (*(s32*)((u8*)gScreenSets + 0x6C) != 0 && (*(u32*)(p + 0x64) & 0x20000) != 0) {
        char* s = *(char**)(p + 0x170);
        if (s != NULL && strlen(s) != 0) {
            FlattenText(s);
            if ((*(u32*)(p + 0x64) & 0x20000) != 0) fn_8019EB94(p);
        }
        fn_8019EB94(p);
        fn_8019F5A8(p);
    }
    if ((*(u32*)(p + 0x64) & 0x10) != 0) {
        u8* m = fn_80196B24((u8*)lbl_805FB658);
        if (m != NULL) {
            ComposeMatrix(m, p, p + 0x17C);
        } else {
            CopyMatrix(p, p + 0x17C);
        }
        *(f32*)(p + 0x5C) = *(f32*)(p + 0x1A8);
        (*(UIVirtFn*)(*(u8**)(p + 0x138) + 0x30))(p, 0x10);
        char* s = *(char**)(p + 0x170);
        if (s != NULL && strlen(s) != 0) {
            FlattenText(s);
            if ((*(u32*)(p + 0x64) & 0x20000) != 0) fn_8019EB94(p);
        }
        fn_8019F5A8(p);
    }
    if (*(u8*)(p + 0x148) == 0 || (*(u32*)(p + 0x64) & 0x20) != 0) {
        fn_8019E27C(p);
    }
    if ((*(u32*)(p + 0x64) & 0x10000) != 0) {
        *(f32*)(p + 0x14C) = *(f32*)(p + 0x14C) + dt;
        fn_8019E838(p, dt);
    }
    return 0;
}

// Clamps a colour channel (scaled by 255) to 0..255.
static s32 ClampByteCh(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

// 0x801A05E4
// UI element update: when flag 0x20 is set, tints the element colour (+0x80) by the current tint
// record, converts it to four identical RGBA vertex colours and hands them to fn_801A1238; when
// flag 0x10 is set, rebuilds the 3x4 matrix at +0x240 like fn_8019CF58. Returns 0.
s32 fn_801A05E4(u8* p, f32 dt) {
    fn_8019B6CC(p, dt);
    if ((*(u32*)(p + 0x64) & 0x20) != 0) {
        u8* tint = fn_80196C9C(lbl_805FB658);
        if (tint != NULL) {
            f32 r = *(f32*)(p + 0x80) * *(f32*)(tint + 0x0);
            f32 g = *(f32*)(p + 0x84) * *(f32*)(tint + 0x4);
            f32 b = *(f32*)(p + 0x88) * *(f32*)(tint + 0x8);
            f32 a = *(f32*)(p + 0x8C) * *(f32*)(tint + 0xC);
            *(f32*)(p + 0x270) = r;
            *(f32*)(p + 0x274) = g;
            *(f32*)(p + 0x278) = b;
            *(f32*)(p + 0x27C) = a;
        } else {
            *(f32*)(p + 0x270) = *(f32*)(p + 0x80);
            *(f32*)(p + 0x274) = *(f32*)(p + 0x84);
            *(f32*)(p + 0x278) = *(f32*)(p + 0x88);
            *(f32*)(p + 0x27C) = *(f32*)(p + 0x8C);
        }
        f32 k255 = lbl_805FF99C;
        s32 ri = (s32)(k255 * *(f32*)(p + 0x270));
        s32 gi = (s32)(k255 * *(f32*)(p + 0x274));
        s32 bi = (s32)(k255 * *(f32*)(p + 0x278));
        s32 ai = (s32)(k255 * *(f32*)(p + 0x27C));
        u32 packed = (u32)ClampByteCh(ri) | ((u32)ClampByteCh(gi) << 8) | ((u32)ClampByteCh(bi) << 16) |
                     ((u32)ClampByteCh(ai) << 24);
        f32 inv = lbl_805FF998;
        f32 colors[16];
        for (s32 i = 0; i < 4; i++) {
            colors[i * 4 + 0] = inv * (f32)(packed & 0xFF);
            colors[i * 4 + 1] = inv * (f32)((packed >> 8) & 0xFF);
            colors[i * 4 + 2] = inv * (f32)((packed >> 16) & 0xFF);
            colors[i * 4 + 3] = inv * (f32)(packed >> 24);
        }
        fn_801A1238(p, colors);
        (*(UIVirtFn*)(*(u8**)(p + 0x138) + 0x30))(p, 0x20);
    }
    if ((*(u32*)(p + 0x64) & 0x10) != 0) {
        u8* m = fn_80196B24((u8*)lbl_805FB658);
        if (m != NULL) {
            ComposeMatrix(m, p, p + 0x240);
        } else {
            CopyMatrix(p, p + 0x240);
        }
        *(f32*)(p + 0x5C) = *(f32*)(p + 0x26C);
        (*(UIVirtFn*)(*(u8**)(p + 0x138) + 0x30))(p, 0x10);
    }
    return 0;
}

// 0x801AE608
// Collision dispatch by the shape type at b+0xC (0..7): picks the swept-sphere / box / capsule
// test for sphere a (centre +0x14 to +0x20, radius +0x3C) against shape b. `outside` is set when
// a's travel segment is longer than its radius. Types 1 and 2 (and anything above 7) report no
// hit. alt replaces out when e is zero.
s32 fn_801AE608(u8* a, u8* alt, u8* b, u8* out, s32 flag, u8* d, s32 e) {
    Vec3 seg;
    Vec3Sub(&seg, (Vec3*)(a + 0x14), (Vec3*)(a + 0x20));
    s32 outside = 0;
    f32 r = *(f32*)(a + 0x3C);
    if (Vec3LengthSqOf(&seg) > r * r) outside = 1;
    switch (*(u32*)(b + 0xC)) {
    case 0: {
        if (outside != 0) {
            if (e == 0) out = alt;
            Vec3 cA14, cA20, cB14;
            Vec3Assign(&cA14, (Vec3*)(a + 0x14));
            Vec3Assign(&cA20, (Vec3*)(a + 0x20));
            Vec3Assign(&cB14, (Vec3*)(b + 0x14));
            return fn_801AF770((u8*)&cB14, (u8*)&cA20, (u8*)&cA14, out, flag, *(f32*)(b + 0x3C), *(f32*)(a + 0x3C));
        }
        return fn_801ADB44(b + 0x14, a + 0x14, (e != 0) ? out : alt, flag, d, *(f32*)(b + 0x3C), *(f32*)(a + 0x3C));
    }
    case 3:
        return fn_801AE544(a + 0x14, b + 0x14, b + 0x40, *(f32*)(a + 0x3C), *(f32*)(b + 0x3C));
    case 4: {
        if (outside != 0) {
            if (e == 0) out = alt;
            Vec3 cB3C, cB14, cA14, cA20;
            Vec3Assign(&cB3C, (Vec3*)(b + 0x3C));
            Vec3Assign(&cB14, (Vec3*)(b + 0x14));
            Vec3Assign(&cA14, (Vec3*)(a + 0x14));
            Vec3Assign(&cA20, (Vec3*)(a + 0x20));
            // The asm also passes r8 = flag; fn_801AE9AC does not read it.
            return fn_801AE9AC((u8*)&cA20, (u8*)&cA14, (u8*)&cB14, (u8*)&cB3C, out, *(f32*)(a + 0x3C));
        }
        if (e == 0) out = alt;
        Vec3 cB3C;
        Vec3Assign(&cB3C, (Vec3*)(b + 0x3C));
        return fn_801AD47C(b + 0x14, (u8*)&cB3C, a + 0x14, out, flag, *(f32*)(a + 0x3C));
    }
    case 5: {
        Vec3 pos[2];
        s32 n;
        f32 dist[2];
        fn_80052638(&pos[0]);
        fn_80052638(&pos[1]);
        u8* c = fn_801ADF54(b);
        u8* bb = fn_801ADF4C(b);
        if (fn_801C2C60(*(f32*)(a + 0x3C), a + 0x14, bb, c, &n, (f32*)pos, dist) != 0) {
            if (n > 0) {
                Vec3Copy((Vec3*)out, &pos[0]);
                *(f32*)(out + 0x18) = dist[0];
            }
            return 1;
        }
        return 0;
    }
    case 6: {
        Vec3 pos[2];
        s32 n;
        f32 dist[2];
        fn_80052638(&pos[0]);
        fn_80052638(&pos[1]);
        u8* c = fn_801ADF64(b);
        u8* bb = fn_801ADF5C(b);
        if (fn_801C3170(*(f32*)(a + 0x3C), a + 0x14, bb, c, &n, (f32*)pos, dist) != 0) {
            if (n > 0) {
                Vec3Copy((Vec3*)out, &pos[0]);
                *(f32*)(out + 0x18) = dist[0];
            }
            return 1;
        }
        return 0;
    }
    case 7: {
        if (outside != 0) {
            if (e == 0) out = alt;
            Vec3 cB6C, cA14, cA20;
            Vec3Assign(&cB6C, (Vec3*)(b + 0x6C));
            Vec3Assign(&cA14, (Vec3*)(a + 0x14));
            Vec3Assign(&cA20, (Vec3*)(a + 0x20));
            // The asm also passes r9 = flag; fn_801AEE84 does not read it.
            return fn_801AEE84((u8*)&cA20, (u8*)&cA14, b + 0x14, (u8*)&cB6C, b + 0x3C, out, *(f32*)(a + 0x3C));
        }
        if (e == 0) out = alt;
        Vec3 cB6C;
        Vec3Assign(&cB6C, (Vec3*)(b + 0x6C));
        return fn_801AD6AC(b + 0x14, (u8*)&cB6C, b + 0x3C, a + 0x14, out, flag, *(f32*)(a + 0x3C));
    }
    default:
        return 0;
    }
}

// Result of fn_801F76D4 (peak scan over a ring of floats), 0x1C bytes.
struct RingScan {
    s32 i0;
    f32 f4;
    s32 i8;
    s32 iC;
    f32 f10;
    s32 i14;
    f32 f18;
};

// Fetches the track point `idx` (stride 0x84, ring of capacity +0xC starting at +0x10) into v[5]
// (x, y, z, w, extra): fields +0xC..+0x1C normally, +0x68..+0x78 when the mode at +0x98 is set.
static void TrackPoint(u8* q, s32 idx, s32 mode, f32* v, u8** recOut) {
    s32 cap = *(s32*)(q + 0xC);
    u8* rec = *(u8**)(q + 0x4) + (((*(s32*)(q + 0x10) + idx) % cap) * 0x84);
    if (mode == 0) {
        v[0] = *(f32*)(rec + 0xC);
        v[1] = *(f32*)(rec + 0x10);
        v[2] = *(f32*)(rec + 0x14);
        v[3] = *(f32*)(rec + 0x18);
        v[4] = *(f32*)(rec + 0x1C);
    } else {
        v[0] = *(f32*)(rec + 0x68);
        v[1] = *(f32*)(rec + 0x6C);
        v[2] = *(f32*)(rec + 0x70);
        v[3] = *(f32*)(rec + 0x74);
        v[4] = *(f32*)(rec + 0x78);
    }
    if (recOut != NULL) *recOut = rec;
}

// 0x801F79E4
// Scans the recorded track (newest to oldest). While recording (+0xF8) it appends height (v[2])
// and v[1] to two sample rings (+0xFC / +0x110) until the height drops below the limit at +0xB0;
// once recording stopped it classifies the peaks into bucket indices and sets +0x90 / +0xA8 from
// their weighted sum. Otherwise it looks for a downward crossing and starts a new recording
// (re-creating the sample rings), pre-filling them from the next 99 points.
void fn_801F79E4(u8* p) {
    u8* q = fn_801483BC(*(s32*)(p + 0x4));
    for (s32 i = *(s32*)(q + 0x14) - 1; i >= 0; i--) {
        f32 cur[5];
        u8* curRec;
        TrackPoint(q, i, *(s32*)(p + 0x98), cur, &curRec);
        if (*(u8*)(p + 0xF8) != 0) {
            f32 h = cur[2];
            if (h >= *(f32*)(p + 0xB0)) {
                s32 pushed;
                s32 n = *(s32*)(p + 0x100);
                if (n >= *(s32*)(p + 0xFC)) {
                    pushed = 0;
                } else {
                    pushed = 1;
                    *(f32*)(*(u8**)(p + 0x10C) + (*(s32*)(p + 0x104) + n) * 4) = h;
                    *(s32*)(p + 0x100) = *(s32*)(p + 0x100) + 1;
                }
                if (pushed == 0) *(u8*)(p + 0xF8) = 0;
                s32 n2 = *(s32*)(p + 0x11C) + 1;
                s32 cap2 = *(s32*)(p + 0x118);
                *(s32*)(p + 0x11C) = n2;
                if (n2 < cap2) *(f32*)(*(u8**)(p + 0x120) + (cap2 - n2) * 4) = cur[1];
            } else {
                *(u8*)(p + 0xF8) = 0;
            }
            if (*(u8*)(p + 0xF8) != 0) continue;

            RingScan s1, s2;
            fn_801F76D4(p + 0xFC, (u8*)&s1);
            fn_801F76D4(p + 0x110, (u8*)&s2);
            s32 bucketF;
            if (s1.f4 >= *(f32*)(p + 0xDC)) bucketF = 3;
            else if (s1.f4 >= *(f32*)(p + 0xD8)) bucketF = 2;
            else if (s1.f4 >= *(f32*)(p + 0xD4)) bucketF = 1;
            else bucketF = 0;
            s32 bucketC;
            if (s1.i8 >= *(s32*)(p + 0xE8)) bucketC = 3;
            else if (s1.i8 >= *(s32*)(p + 0xE4)) bucketC = 2;
            else if (s1.i8 >= *(s32*)(p + 0xE0)) bucketC = 1;
            else bucketC = 0;
            s32 bucketD;
            if (s1.i0 >= *(s32*)(p + 0xD0)) bucketD = 3;
            else if (s1.i0 >= *(s32*)(p + 0xCC)) bucketD = 2;
            else if (s1.i0 >= *(s32*)(p + 0xC8)) bucketD = 1;
            else bucketD = 0;
            f32 negPeak = -s2.f18;
            s32 bucketE;
            if (negPeak >= *(f32*)(p + 0xF4)) bucketE = 3;
            else if (negPeak >= *(f32*)(p + 0xF0)) bucketE = 2;
            else if (negPeak >= *(f32*)(p + 0xEC)) bucketE = 1;
            else bucketE = 0;
            f32 sum = ((f32)bucketF * *(f32*)(p + 0xBC)) + ((f32)bucketC * *(f32*)(p + 0xC0));
            sum = ((f32)bucketD * *(f32*)(p + 0xB8)) + sum;
            sum = ((f32)bucketE * *(f32*)(p + 0xC4)) + sum;
            s32 score = (s32)lrintf(sum);
            if (score != 0) {
                *(u8*)(p + 0x90) = 1;
                *(s32*)(p + 0xA8) = score - 1;
            }
        } else {
            f32 nxt[5];
            TrackPoint(q, i + 1, *(s32*)(p + 0x98), nxt, NULL);
            f32 zero = lbl_806001C0;
            f32 one = lbl_806001AC;
            f32 dn = (zero * nxt[2]) + ((zero * nxt[0]) + (one * nxt[1]));
            f32 dc = (zero * cur[2]) + ((zero * cur[0]) + (one * cur[1]));
            s32 dir;
            if (dn > zero && dc <= zero) dir = -1;
            else if (dn < zero && dc >= zero) dir = 1;
            else dir = 0;
            if (dir != -1) continue;

            f32 prevH = cur[2];
            if (!(prevH >= *(f32*)(p + 0xAC))) continue;
            if (*(u8*)(p + 0xB4) != 0 && !(*(f32*)(curRec + 0x58) < zero)) continue;

            // (Re)create the two sample rings: 200 heights (+0xFC, buffer +0x10C) and 4 values.
            if (*(void**)(p + 0x10C) != NULL) {
                if (*(s32*)(p + 0xFC) + *(s32*)(p + 0x104) != 0x190) {
                    BufferFree(*(void**)(p + 0x10C));
                    *(void**)(p + 0x10C) = NULL;
                    *(s32*)(p + 0xFC) = 0;
                    *(s32*)(p + 0x100) = 0;
                    *(s32*)(p + 0x104) = 0;
                    *(s32*)(p + 0x108) = 0;
                }
            }
            *(s32*)(p + 0xFC) = 0xC8;
            *(s32*)(p + 0x100) = 0;
            *(s32*)(p + 0x104) = 0xC8;
            *(s32*)(p + 0x108) = 0;
            if (*(void**)(p + 0x10C) == NULL) *(void**)(p + 0x10C) = MemAllocDefault(0x640);
            if (*(void**)(p + 0x120) != NULL) {
                if (*(s32*)(p + 0x110) + *(s32*)(p + 0x118) != 8) {
                    BufferFree(*(void**)(p + 0x120));
                    *(void**)(p + 0x120) = NULL;
                    *(s32*)(p + 0x110) = 0;
                    *(s32*)(p + 0x114) = 0;
                    *(s32*)(p + 0x118) = 0;
                    *(s32*)(p + 0x11C) = 0;
                }
            }
            *(s32*)(p + 0x110) = 4;
            *(s32*)(p + 0x114) = 0;
            *(s32*)(p + 0x118) = 4;
            *(s32*)(p + 0x11C) = 0;
            if (*(void**)(p + 0x120) == NULL) *(void**)(p + 0x120) = MemAllocDefault(0x20);

            // Pre-fill from the following points while the height keeps falling.
            for (s32 j = i + 1; j < i + 0x64; j++) {
                f32 pt[5];
                TrackPoint(q, j, *(s32*)(p + 0x98), pt, NULL);
                if (prevH >= pt[2] && pt[2] >= *(f32*)(p + 0xAC)) {
                    s32 n3 = *(s32*)(p + 0x108) + 1;
                    s32 cap3 = *(s32*)(p + 0x104);
                    *(s32*)(p + 0x108) = n3;
                    if (n3 < cap3) *(f32*)(*(u8**)(p + 0x10C) + (cap3 - n3) * 4) = pt[2];
                    s32 n4 = *(s32*)(p + 0x11C) + 1;
                    s32 cap4 = *(s32*)(p + 0x118);
                    *(s32*)(p + 0x11C) = n4;
                    if (n4 < cap4) *(f32*)(*(u8**)(p + 0x120) + (cap4 - n4) * 4) = pt[1];
                    prevH = pt[2];
                } else {
                    *(u8*)(p + 0xF8) = 1;
                    break;
                }
            }
        }
    }
}

}  // extern "C"
