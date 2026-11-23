// Grow-juice level object updates: intro/outro timers, locator-based setup, tube/weapon event
// broadcasts and a game-flow watcher.
#include "types.h"
#include <math.h>
extern "C" {
#include "game/script_callbacks.h"
}

struct Locator;
extern "C++" void* gEventMgr;           // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;     // 0x805FB94C

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u32 RandomU32(void);                                                    // 0x80143460
s32 RaySceneTest(void* ray);                                            // 0x80070458
void UISetProgress(void* obj, f32 t);
void UISetText(void* el, const char* text, s32 a);                      // 0x8019EFC8
void* UIWidgetFindChild(void* widget, u32 hash);                        // 0x80199AEC
Locator* WorldFindLocator(void* world, u32 hash);                       // 0x8012AA50
void fn_8006AADC(u8* p, f32 delta);                                     // 0x8006AADC
void fn_80070460(u8* p);                                                // 0x80070460
void fn_800704AC(u8* p, f32 dt);                                        // 0x800704AC
void* fn_80070F4C(void);                                                // 0x80070F4C
void fn_800712BC(u8* p, s32 a, s32 b);                                  // 0x800712BC
void fn_800715B8(u8* p, s32 a);                                         // 0x800715B8
void fn_80071838(u8* arg0, u32 arg1);                                   // 0x80071838
void fn_800A81A8(u8* p);                                                // 0x800A81A8
void fn_800A8810(u8* p);                                                // 0x800A8810
void fn_800AA2D0(u8* self, f32 dt, f32 v);                              // 0x800AA2D0
void WiiMoteCheckSpike(u8* p, f32 dt);                                        // 0x800B49B8
void fn_800B5478(u8* p, f32 dt);                                        // 0x800B5478
s32 fn_800B66D0(u8* p);                                                 // 0x800B66D0
f32 fn_800B71A8(u8* p);                                                 // 0x800B71A8
f32 fn_800B7380(u8* p);                                                 // 0x800B7380
f32 fn_800B7388(u8* p);                                                 // 0x800B7388
void fn_801458B0(f32* quat, f32* a, f32* b);                            // 0x801458B0
void fn_8018BF60(u8** arg0, f32 farg0);                                 // 0x8018BF60
u8* fn_801B3308(void);                                                  // 0x801B3308
void fn_801B3310(u8* p, u8* q);                                         // 0x801B3310
void fn_801C4B90(u8* obj, f32* a, f32* b, s32 c, s32 d, u32 color);     // 0x801C4B90
void fn_801C5CC0(u8* obj, f32* a, f32* b);                              // 0x801C5CC0
void* fn_801D40E0(s32 n);                                               // 0x801D40E0
s32 fn_801EC880(u8* p, u32 hash);                                       // 0x801EC880
void fn_801ECD1C(u8* p, s32 idx, s32 immediate);                        // 0x801ECD1C
void fn_801F8628(u8* obj, s8* src, s32 n);                              // 0x801F8628
void fn_801F8F34(u8* p, f32 v);                                         // 0x801F8F34
void fn_801F8FA0(u8* p, f32 v);                                         // 0x801F8FA0
void* fn_80209814(void* v);                                             // 0x80209814
void fn_8020B1C8(void* handle, u32 hash, s32 a);                        // 0x8020B1C8
extern u8 gWorldState[];                // 0x803E6D00 (first word: world block pointer)
extern u8 lbl_803C4298[];               // 0x803C4298 (+0x4: object)
extern f32 lbl_80346258[];              // 0x80346258
extern f32 lbl_80346268[];              // 0x80346268
extern f32 lbl_80346278[];              // 0x80346278
extern f32 lbl_80346288[];              // 0x80346288
extern f32 lbl_80346CF0[];              // 0x80346CF0
extern void* lbl_805FBBAC;              // 0x805FBBAC (voice SFX handle)
}

typedef f32 (*FnF32Obj)(u8*);
typedef void (*FnVoidObj)(u8*);
typedef u8** (*FnPtrObj)(u8**);

static const u32 kHashTube1A = 0x0C46F507u;        // "grow_juice_tube1A"
static const u32 kHashTube2A = 0x276BA6C4u;        // "grow_juice_tube2A"
static const u32 kHashTube3A = 0x3E709785u;        // "grow_juice_tube3A"
static const u32 kHashTube4A = 0x71310142u;        // "grow_juice_tube4A"
static const u32 kHashTube5A = 0x682A3003u;        // "grow_juice_tube5A"
static const u32 kHashTube6A = 0x430763C0u;        // "grow_juice_tube6A"
static const u32 kHashSeedsA = 0xBA7D0996u;        // "grow_juice_seedsA"
static const u32 kHashWeaponA = 0x861FD133u;       // "grow_juice_weaponA"
static const u32 kHashGun02A = 0xBAC5C3F2u;        // "grow_juice_gun02A"
static const u32 kHashTentaclesA = 0xDCF3BBECu;    // "grow_juice_tentaclesA"
static const u32 kHashHeartA = 0x0BE4259Cu;        // "grow_juice_heartA"
static const u32 kHashGunA = 0x6D3BDFAAu;          // "grow_juice_gunA"
static const u32 kHashTentacles2A = 0x7EABE956u;   // "grow_juice_tentacles2A"

// Entity slot i of the current zone: *obj = entity pointer, *entry = its 0x40-byte table entry.
static void ZoneEntity(s32 i, u8** obj, u8** entry) {
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 n = *(s32*)(zone + 0x94);
    if (i < n) {
        *obj = *(u8**)(*(u8**)(zone + 0x120) + i * 4);
        *entry = *(u8**)(zone + 0x8C) + i * 0x40;
    } else {
        *obj = *(u8**)(*(u8**)(zone + 0x12C) + (i - n) * 4);
        *entry = *(u8**)(zone + 0x90) + (i - n) * 0x40;
    }
}

// Sends an event to an entity with its "sending" flag (bit 0 at def+0x20) raised for the call.
static void SendEventGuarded(u8* obj, u32 event) {
    u8* def = *(u8**)(obj + 4);
    u32 flags = *(u32*)(def + 0x20);
    if (!(flags & 1)) {
        *(u32*)(def + 0x20) = flags | 1;
        EventSend(gEventMgr, obj, NULL, event, 0, 0, 1, 0);
        *(u32*)(*(u8**)(obj + 4) + 0x20) &= 0xFFFFFFFEu;
    } else {
        EventSend(gEventMgr, obj, NULL, event, 0, 0, 1, 0);
    }
}

extern "C" {

// 0x8008FB64
// Timer-driven cue: when the timer passes 0 restarts the three emitters with shuffled values,
// and at the 0/1/2 thresholds fires the next queued event at an emitter slot.
void fn_8008FB64(u8* arg0, f32 farg0) {
    f32 sp[3];
    f32 f31 = *(f32*)(arg0 + 0x6C);
    f32 f2 = *(f32*)(arg0 + 0x8C) + farg0;
    f32 f30 = f31 + farg0;
    *(f32*)(arg0 + 0x8C) = f2;
    if (f2 <= 1.0f) return;
    fn_800704AC(arg0, farg0);
    if (f31 == 0.0f) {
        sp[0] = 0.5f + (0.0f + (0.25f * (2.3283064e-10f * (f32)RandomU32())));
        sp[1] = 0.5f + (0.375f + (0.25f * (2.3283064e-10f * (f32)RandomU32())));
        sp[2] = 0.5f + (0.75f + (0.25f * (2.3283064e-10f * (f32)RandomU32())));
        for (s32 n = 0; n < 5; n++) {
            u32 a = RandomU32() % 3;
            u32 b = RandomU32() % 3;
            if ((s32)a != (s32)b) {
                f32 t = sp[a];
                sp[a] = sp[b];
                sp[b] = t;
            }
        }
        u8* slot = arg0;
        for (s32 i = 0; i < 3; i++) {
            u8* obj = *(u8**)(slot + 0xAC);
            u8** r = (*(FnPtrObj*)(*(u8**)obj + 0x10))((u8**)obj);
            if (r != NULL) fn_8018BF60(r, sp[i]);
            slot += 0x48;
        }
    }
    if (f31 <= 0.0f && f30 >= 0.0f) {
        s32 idx = *(s32*)(arg0 + 0x814);
        if (idx < 3) {
            EventSend(gEventMgr, *(void**)(arg0 + idx * 0x28 + 0x184), NULL, 0xD7A63AA2u, 0, 0, 1, 0);
            *(s32*)(arg0 + 0x814) = *(s32*)(arg0 + 0x814) + 1;
        }
    }
    if (f31 <= 1.0f && f30 >= 1.0f) {
        s32 idx = *(s32*)(arg0 + 0x814);
        if (idx < 3) {
            EventSend(gEventMgr, *(void**)(arg0 + idx * 0x28 + 0x184), NULL, 0xD7A63AA2u, 0, 0, 1, 0);
            *(s32*)(arg0 + 0x814) = *(s32*)(arg0 + 0x814) + 1;
        }
    }
    if (f31 <= 2.0f && f30 >= 2.0f) {
        s32 idx = *(s32*)(arg0 + 0x814);
        if (idx < 3) {
            EventSend(gEventMgr, *(void**)(arg0 + idx * 0x28 + 0x184), NULL, 0xD7A63AA2u, 0, 0, 1, 0);
            *(s32*)(arg0 + 0x814) = *(s32*)(arg0 + 0x814) + 1;
        }
    }
    u8* r4 = *(u8**)(arg0 + 0xA4);
    if (*(u32*)(*(u8**)(r4 + 0xA0) + 0xF0) & 0x10) {
        u8* tbl = r4 + 0x94;
        fn_801ECD1C(tbl, fn_801EC880(tbl, 0xF3C07FC1u), 0);
    }
}

// 0x800A7E20
// Sets up the two locator-defined axes (start/end points, direction quaternions, lengths) and
// then runs the common update with zero deltas.
void fn_800A7E20(u8* arg0) {
    u8* locA = (u8*)WorldFindLocator(gWorldState, 0x834C7963u);
    u8* locB = (u8*)WorldFindLocator(gWorldState, 0xAAE1A370u);
    if (locA != NULL && locB != NULL) {
        f32 zero = 0.0f;
        f32 one = 1.0f;
        f32 vB[3];      // sp70: locB - origin
        f32 vA[3];      // sp64: locA - origin
        f32 dir[3];     // sp3C
        f32 ref[3];     // sp30
        f32 len2;

        *(f32*)(arg0 + 0x6D0) = zero;
        *(f32*)(arg0 + 0x6D4) = zero;
        *(f32*)(arg0 + 0x6D8) = zero;
        *(f32*)(arg0 + 0x6DC) = *(f32*)(locB + 0x8);
        vB[0] = *(f32*)(locB + 0x8) - zero;
        *(f32*)(arg0 + 0x6E0) = *(f32*)(locB + 0xC);
        vB[1] = *(f32*)(locB + 0xC) - zero;
        vB[2] = zero;
        *(f32*)(arg0 + 0x6E4) = *(f32*)(locB + 0x10);
        *(f32*)(arg0 + 0x6E8) = *(f32*)(locA + 0x8);
        vA[0] = *(f32*)(locA + 0x8) - zero;
        *(f32*)(arg0 + 0x6EC) = *(f32*)(locA + 0xC);
        vA[1] = *(f32*)(locA + 0xC) - zero;
        vA[2] = zero;
        *(f32*)(arg0 + 0x6F0) = *(f32*)(locA + 0x10);
        len2 = (zero * zero) + ((vA[0] * vA[0]) + (vA[1] * vA[1]));
        *(f32*)(arg0 + 0x704) = zero;
        *(f32*)(arg0 + 0x708) = zero;
        *(f32*)(arg0 + 0x70C) = zero;
        *(f32*)(arg0 + 0x710) = one;
        if (len2 == 0.0f) {
            dir[0] = zero;
            dir[1] = zero;
            dir[2] = zero;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            dir[0] = vA[0] * inv;
            dir[1] = vA[1] * inv;
            dir[2] = vA[2] * inv;
        }
        ref[0] = zero;
        ref[1] = zero;
        ref[2] = one;
        fn_801458B0((f32*)(arg0 + 0x704), dir, ref);
        len2 = (vA[2] * vA[2]) + ((vA[0] * vA[0]) + (vA[1] * vA[1]));
        f32 lenA = 0.0f;
        if (!(len2 <= 0.0f)) lenA = sqrtf(len2);
        *(f32*)(arg0 + 0x718) = lenA;

        len2 = (vB[2] * vB[2]) + ((vB[0] * vB[0]) + (vB[1] * vB[1]));
        ref[0] = zero;
        ref[1] = zero;
        ref[2] = one;
        *(f32*)(arg0 + 0x6F4) = zero;
        *(f32*)(arg0 + 0x6F8) = zero;
        *(f32*)(arg0 + 0x6FC) = zero;
        *(f32*)(arg0 + 0x700) = one;
        if (len2 == 0.0f) {
            dir[0] = zero;
            dir[1] = zero;
            dir[2] = zero;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            dir[0] = vB[0] * inv;
            dir[1] = vB[1] * inv;
            dir[2] = vB[2] * inv;
        }
        fn_801458B0((f32*)(arg0 + 0x6F4), dir, ref);
        len2 = (vB[2] * vB[2]) + ((vB[0] * vB[0]) + (vB[1] * vB[1]));
        f32 lenB = 0.0f;
        if (!(len2 <= 0.0f)) lenB = sqrtf(len2);
        *(f32*)(arg0 + 0x71C) = zero;
        *(f32*)(arg0 + 0x714) = lenB;
        *(f32*)(arg0 + 0x720) = zero;
    }
    fn_800AA2D0(arg0, 0.0f, 0.0f);
}

// 0x800A8AFC
// Enter state: refreshes the common state, notifies the two children and every tube-type entity
// of the zone, and sets the +0x158 timer to 0.4.
void fn_800A8AFC(u8* arg0) {
    fn_80070460(arg0);
    f32 t = *(f32*)(arg0 + 0x6A0);
    fn_800712BC(arg0, (s32)t, t < 10.0f);
    fn_800715B8(arg0, 0);
    fn_80071838(arg0, 0u);
    SendEventGuarded(*(u8**)(arg0 + 0x90), 0xCA90298Eu);
    SendEventGuarded(*(u8**)(arg0 + 0x94), 0xCA90298Eu);
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 count = *(s32*)(zone + 0x94) + *(s32*)(zone + 0x98);
    for (s32 i = 0; i < count; i++) {
        u8* obj;
        u8* entry;
        ZoneEntity(i, &obj, &entry);
        if ((*(u32*)(entry + 0x1C) & 0x40) && obj != NULL) {
            u8* info = *(u8**)(obj + 0x9C);
            u32 type = info != NULL ? *(u32*)(info + 0x8) : 0;
            switch (type) {
            case kHashTube3A:
            case kHashTube1A:
            case kHashSeedsA:
            case kHashTube2A:
            case kHashTube5A:
            case kHashTube6A:
            case kHashTube4A:
                SendEventGuarded(obj, 0xF15918A6u);
                break;
            default:
                break;
            }
        }
    }
    *(f32*)(arg0 + 0x158) = 0.4f;
}

// 0x800A9800
// Enter state: pings every weapon/tube/tentacle entity of the zone, then resets the common state,
// sets the +0x6A0 timer to 70, starts the voice SFX and registers the two effect handles.
void fn_800A9800(u8* arg0) {
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 count = *(s32*)(zone + 0x94) + *(s32*)(zone + 0x98);
    for (s32 i = 0; i < count; i++) {
        u8* obj;
        u8* entry;
        ZoneEntity(i, &obj, &entry);
        if ((*(u32*)(entry + 0x1C) & 0x40) && obj != NULL) {
            u8* info = *(u8**)(obj + 0x9C);
            u32 type = info != NULL ? *(u32*)(info + 0x8) : 0;
            switch (type) {
            case kHashTube2A:
            case kHashTentaclesA:
            case kHashSeedsA:
            case kHashWeaponA:
            case kHashGun02A:
            case kHashTube1A:
            case kHashHeartA:
            case kHashGunA:
            case kHashTube6A:
            case kHashTube3A:
            case kHashTube5A:
            case kHashTentacles2A:
            case kHashTube4A:
                SendEventGuarded(obj, 0x9B51AA64u);
                break;
            default:
                break;
            }
        }
    }
    fn_800A8810(arg0);
    (*(FnVoidObj*)(*(u8**)(arg0 + 4) + 0x54))(arg0);
    fn_800A81A8(arg0);
    *(f32*)(arg0 + 0x6A0) = 70.0f;
    *(f32*)(arg0 + 0x150) = lbl_80346CF0[RaySceneTest(NULL)];
    fn_801F8F34(arg0 + 0x21C, 0.1f);
    fn_801F8FA0(arg0 + 0x21C, 0.3f);
    s8 src[3];
    src[0] = 0;
    src[1] = -1;
    src[2] = 0;
    fn_801F8628(arg0 + 0x21C, src, 1);
    *(s32*)(arg0 + 0x2B4) = 1;
    fn_801B3310(fn_801B3308(), arg0 + 0x160);
    fn_801B3310(fn_801B3308(), arg0 + 0x21C);
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
    fn_8020B1C8(lbl_805FBBAC, 0xA5237C5Fu, 0);
}

// 0x800AB128
// Per-frame update of a two-sided beam: computes the half-width vector perpendicular to the
// beam (towards the camera object when the +0x28C flag is set, else around the vertical axis)
// and feeds the two edge points to the attached trail (+0x290) and strip (+0x294).
void fn_800AB128(u8* arg0, f32 farg0) {
    fn_8006AADC(arg0, farg0);
    if (*(u8**)(arg0 + 0x290) == NULL && *(u8**)(arg0 + 0x294) == NULL) return;
    f32 dx = *(f32*)(arg0 + 0x264) - *(f32*)(arg0 + 0x270);
    f32 dy = *(f32*)(arg0 + 0x268) - *(f32*)(arg0 + 0x274);
    f32 dz = *(f32*)(arg0 + 0x26C) - *(f32*)(arg0 + 0x278);
    f32 d[3];
    d[0] = dx;
    d[1] = dy;
    d[2] = dz;
    if (*(u8*)(arg0 + 0x28C) != 0) {
        u8* pos = *(u8**)(arg0 + 4);
        u8* target = *(u8**)(*(u8**)(lbl_803C4298 + 4) + 4);
        f32 vx = *(f32*)(target + 0x0) - *(f32*)(pos + 0x0);
        f32 vy = *(f32*)(target + 0x4) - *(f32*)(pos + 0x4);
        f32 vz = *(f32*)(target + 0x8) - *(f32*)(pos + 0x8);
        f32 cx = (vy * dz) - (vz * dy);
        f32 cy = (vz * dx) - (vx * dz);
        f32 cz = (vx * dy) - (vy * dx);
        f32 len2 = (cz * cz) + ((cx * cx) + (cy * cy));
        f32 n[3];
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            n[0] = cx * inv;
            n[1] = cy * inv;
            n[2] = cz * inv;
        }
        f32 half = 0.5f * *(f32*)(arg0 + 0x288);
        *(f32*)(arg0 + 0x298) = n[0] * half;
        *(f32*)(arg0 + 0x29C) = n[1] * half;
        *(f32*)(arg0 + 0x2A0) = n[2] * half;
    } else {
        f32 ux = 0.0f;
        f32 uy = 0.0f;
        f32 uz = 1.0f;
        f32 cx = (dy * uz) - (dz * uy);
        f32 cy = (dz * ux) - (dx * uz);
        f32 cz = (dx * uy) - (dy * ux);
        f32 n[3];
        *(f32*)(arg0 + 0x298) = cx;
        *(f32*)(arg0 + 0x29C) = cy;
        *(f32*)(arg0 + 0x2A0) = cz;
        f32 len2 = (cz * cz) + ((cx * cx) + (cy * cy));
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            n[0] = cx * inv;
            n[1] = cy * inv;
            n[2] = cz * inv;
        }
        f32 half = 0.5f * *(f32*)(arg0 + 0x288);
        *(f32*)(arg0 + 0x298) = n[0] * half;
        *(f32*)(arg0 + 0x29C) = n[1] * half;
        *(f32*)(arg0 + 0x2A0) = n[2] * half;
    }
    f32 hi[3];
    f32 lo[3];
    hi[0] = *(f32*)(arg0 + 0x27C) + *(f32*)(arg0 + 0x298);
    hi[1] = *(f32*)(arg0 + 0x280) + *(f32*)(arg0 + 0x29C);
    hi[2] = *(f32*)(arg0 + 0x284) + *(f32*)(arg0 + 0x2A0);
    lo[0] = *(f32*)(arg0 + 0x27C) - *(f32*)(arg0 + 0x298);
    lo[1] = *(f32*)(arg0 + 0x280) - *(f32*)(arg0 + 0x29C);
    lo[2] = *(f32*)(arg0 + 0x284) - *(f32*)(arg0 + 0x2A0);
    u8* trail = *(u8**)(arg0 + 0x290);
    if (trail != NULL) {
        fn_801C4B90(trail, hi, lo, 0, 0, *(u32*)(*(u8**)trail + 4));
    }
    u8* strip = *(u8**)(arg0 + 0x294);
    if (strip != NULL) {
        fn_801C5CC0(strip, hi, lo);
    }
}

// 0x800B4314
// Per-frame update of the grow-juice HUD/flow: counts the timers, signals game-flow win/lose
// events from the level's value against the per-difficulty thresholds, drains/clamps the +0x67C
// meter and refreshes the two progress bars and the text label.
void fn_800B4314(u8* arg0, f32 farg0) {
    f32 f3 = *(f32*)(arg0 + 0x674) - farg0;
    *(f32*)(arg0 + 0x674) = f3;
    *(f32*)(arg0 + 0x678) = *(f32*)(arg0 + 0x678) + farg0;
    if (f3 <= 0.0f) {
        s32 n = RaySceneTest(NULL);
        if (n < 0) n = 0;
        else if (n > 3) n = 3;
        f32 v = (*(FnF32Obj*)(*(u8**)(arg0 + 4) + 0x64))(arg0);
        if (v >= lbl_80346278[n]) {
            GameFlowSignal((s32)0xC608C842u, 0, 0);
        } else {
            GameFlowSignal(0x117741C3, 0, 0);
        }
    }
    if ((*(FnF32Obj*)(*(u8**)(arg0 + 4) + 0x64))(arg0) <= 0.0f) {
        GameFlowSignal(0x117741C3, 0, 0);
    }
    s32 n2 = RaySceneTest(NULL);
    if (n2 < 0) n2 = 0;
    else if (n2 > 3) n2 = 3;
    if (fn_800B71A8(*(u8**)(arg0 + 0x94)) > lbl_80346258[n2]) {
        GameFlowSignal((s32)0xC608C842u, 0, 0);
    }
    if ((*(f32*)(arg0 + 0x674) > 10.0f || (*(FnF32Obj*)(*(u8**)(arg0 + 4) + 0x64))(arg0) < 100.0f) &&
        *(u8**)(arg0 + 0x94) != NULL) {
        u8* sub = (u8*)(uintptr_t)fn_800B66D0(*(u8**)(arg0 + 0x94));
        if (sub != NULL) {
            s32 n3 = RaySceneTest(NULL);
            if (n3 < 0) n3 = 0;
            else if (n3 > 3) n3 = 3;
            *(f32*)(arg0 + 0x67C) = *(f32*)(arg0 + 0x67C) - (farg0 * lbl_80346288[n3]);
            s32 n4 = RaySceneTest(NULL);
            if (n4 < 0) n4 = 0;
            else if (n4 > 3) n4 = 3;
            f32 lim = lbl_80346268[n4];
            f32 cur = *(f32*)(arg0 + 0x67C);
            f32 out;
            if (lim > 0.0f) {
                if (cur < 0.0f) out = 0.0f;
                else out = (cur > lim) ? lim : cur;
            } else {
                if (cur < lim) out = lim;
                else out = (cur > 0.0f) ? 0.0f : cur;
            }
            *(f32*)(arg0 + 0x67C) = out;
        }
    }
    WiiMoteCheckSpike(arg0, farg0);
    fn_800B5478(arg0, farg0);
    void* hud = fn_80070F4C();
    if (hud != NULL) {
        if (*(void**)(arg0 + 0xA2C) != NULL) {
            UISetProgress(*(void**)(arg0 + 0xA2C), 100.0f * fn_800B7388(*(u8**)(arg0 + 0x94)));
        }
        if (*(void**)(arg0 + 0xA30) != NULL) {
            UISetProgress(*(void**)(arg0 + 0xA30), 100.0f * fn_800B7380(*(u8**)(arg0 + 0x94)));
        }
        void* label = UIWidgetFindChild(hud, 0x31AA5D1Au);
        if (label != NULL) {
            UISetText(label, (const char*)(arg0 + 0x914), 0);
        }
    }
    f32 t = *(f32*)(arg0 + 0x674);
    fn_800712BC(arg0, (s32)t, t < 10.0f);
}

}  // extern "C"
