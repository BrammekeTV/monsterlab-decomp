// Object reset / waypoint-zone / projectile-aim helpers in 0x8005xxxx - 0x8006xxxx.
#include "types.h"
#include "engine/math.h"

extern "C" {
void EulerToQuat(Quat* out, const Vec3* euler);                                             // 0x80145B00
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u32  RandomU32(void);
void fn_80062DC4(u8* p, s32 a, s32 b);                                                       // 0x80062DC4
s32  fn_80138C40(void* world, f32* a, f32* b, f32* hit, void* p, s32 c, s32 d);              // 0x80138C40
void fn_801455E0(f32* out, u8* quat);                                                        // 0x801455E0
s32  fn_8018BDE0(u8* p, s32 key);                                                            // 0x8018BDE0
s32  fn_801AC6B4(u8* a, u8* b, u8* obj);                                                     // 0x801AC6B4
void fn_801B1778(void* p, s32 a);                                                            // 0x801B1778
void fn_801B4EA4(u8* obj, u8* flag);                                                         // 0x801B4EA4
void fn_801B55F4(u8* obj, u8* out, s32 idx);                                                 // 0x801B55F4
void fn_801BB478(u8* p, s32 a1);                                                             // 0x801BB478
u8*  fn_801C3DB8(u8* table, u32 key);                                                        // 0x801C3DB8
void* fn_801C3F6C(u8* table, u8* entry);                                                     // 0x801C3F6C
void fn_801C42A8(u8* obj);                                                                   // 0x801C42A8
void fn_801C4B90(u8* obj, f32* a, f32* b, s32 c, s32 d, u8* e);                              // 0x801C4B90
s32  fn_801C5C60(u8* a);                                                                     // 0x801C5C60
void fn_801C5C74(u8* obj, f32* a, f32* b);                                                   // 0x801C5C74
BOOL fn_801C6634(u8* col);                                                                   // 0x801C6634
u8*  fn_801C668C(u8* key);                                                                   // 0x801C668C
void fn_801C6724(u8* out, u8* obj, s32 zone);                                                // 0x801C6724
void fn_801C6838(u8* out, u8* a, u8* b);                                                     // 0x801C6838
void fn_801C83AC(u8* mgr, u8* item);                                                         // 0x801C83AC
s32  fn_801EC880(u8* p, u32 hash);                                                           // 0x801EC880
void fn_801ECFA0(u8* p, s32 idx);                                                            // 0x801ECFA0
void* memset(void* p, int c, unsigned long n);
s32  rand(void);
extern u8  gWorldState[];       // 0x803E6D00 (first word: world block pointer)
extern u8  lbl_803C4298[];      // 0x803C4298
extern u8  lbl_8047C760[];      // 0x8047C760 (effect manager)
extern u8  lbl_8047C7AC[];      // 0x8047C7AC
extern s32 lbl_805FB6B8;        // 0x805FB6B8
extern u32 lbl_805FB854;        // 0x805FB854 (32-bit frame counter)
}
extern "C++" void* gEventMgr;           // 0x805FB6C8
extern "C++" s32   gZoneResourceSize;   // 0x805FB94C

static const u32 kHashCC70BCDE = 0xCC70BCDEu;
static const u32 kHash9E216B17 = 0x9E216B17u;
static const u32 kHash206114EF = 0x206114EFu;
static const u32 kHashEventZoneNode = 0x30D6CC94u;

static const u32 kHashGoo     = 0x1B164D9Cu;  // "goo"
static const u32 kHashGrass   = 0x5F595D43u;  // "grass"
static const u32 kHashWater   = 0x0C67D66Au;  // "water"
static const u32 kHashWood    = 0x0A08F4B9u;  // "wood"
static const u32 kHashMat5    = 0xB5CE36F5u;
static const u32 kHashMetal   = 0x37363679u;  // "metal"
static const u32 kHashTile    = 0x401A44B0u;  // "tile"
static const u32 kHashDirt    = 0xAEF20C02u;  // "dirt"
static const u32 kHashMat9    = 0x1A811CBEu;
static const u32 kHashDefault = 0xDC1BFEC5u;  // "default"

// Uniform random float in [0, 1): u32 * 2^-32.
static inline f32 Rand01_313() {
    return 2.3283064e-10f * (f32)RandomU32();
}

// a + (b - a) * rand01
static inline f32 RandLerp_313(f32 a, f32 b) {
    return a + (b - a) * Rand01_313();
}

extern "C" {

// 0x800526E4
// Object (re)initialisation: clears the motion/aim fields, picks the random tint range,
// resolves the model node ids, snaps the object to the floor below it and runs the common reset.
void fn_800526E4(u8* p, s32 arg1, s32 arg2) {
    f32 zero = 0.0f;
    f32 one = 1.0f;
    f32 neg1 = -1.0f;

    if (arg2 == 0) {
        *(f32*)(*(u8**)(p + 0x4) + 0x24) = *(f32*)(p + 0x5F4);
    }
    *(f32*)(p + 0x758) = zero;
    *(f32*)(p + 0x75C) = zero;
    *(f32*)(p + 0x760) = zero;
    *(f32*)(p + 0x844) = zero;
    *(f32*)(p + 0x848) = zero;
    *(f32*)(p + 0x84C) = zero;
    *(f32*)(p + 0x770) = zero;
    *(f32*)(p + 0x774) = zero;
    *(f32*)(p + 0x778) = zero;
    *(f32*)(p + 0x764) = zero;
    *(f32*)(p + 0x768) = zero;
    *(f32*)(p + 0x76C) = zero;
    *(u32*)(p + 0x838) &= 0x30000;
    *(s32*)(p + 0x7B0) = lbl_805FB6B8;
    *(s32*)(p + 0x7B4) = 0;
    *(f32*)(p + 0x7B8) = zero;
    *(f32*)(p + 0x7BC) = zero;
    *(f32*)(p + 0x7C0) = zero;
    *(f32*)(p + 0x7C4) = zero;
    *(f32*)(p + 0x7C8) = zero;
    *(f32*)(p + 0x7CC) = zero;
    *(f32*)(p + 0x7D0) = one;
    *(f32*)(p + 0x7D8) = neg1;
    *(f32*)(p + 0x7EC) = one;
    *(s32*)(p + 0x78C) = 0;
    *(u32*)(p + 0x794) = lbl_805FB854;
    *(f32*)(p + 0x868) = zero;
    *(f32*)(p + 0x86C) = zero;
    *(f32*)(p + 0x870) = zero;
    *(f32*)(p + 0x874) = zero;
    *(f32*)(p + 0x878) = zero;

    if (arg2 == 0) {
        u8* tr = *(u8**)(p + 0x4);
        f32 euler[3];
        *(f32*)(tr + 0x0) = *(f32*)(p + 0xC8);
        *(f32*)(tr + 0x4) = *(f32*)(p + 0xCC);
        *(f32*)(tr + 0x8) = *(f32*)(p + 0xD0);
        euler[0] = *(f32*)(p + 0xD4) * 0.63661975f;
        euler[1] = *(f32*)(p + 0xD8) * 0.63661975f;
        euler[2] = *(f32*)(p + 0xDC) * 0.63661975f;
        EulerToQuat((Quat*)(*(u8**)(p + 0x4) + 0xC), (const Vec3*)euler);
    }

    // Remember the spawn position.
    {
        u8* tr = *(u8**)(p + 0x4);
        *(f32*)(p + 0x74C) = *(f32*)(tr + 0x0);
        *(f32*)(p + 0x750) = *(f32*)(tr + 0x4);
        *(f32*)(p + 0x754) = *(f32*)(tr + 0x8);
        *(f32*)(p + 0x798) = *(f32*)(tr + 0x0);
        *(f32*)(p + 0x79C) = *(f32*)(tr + 0x4);
        *(f32*)(p + 0x7A0) = *(f32*)(tr + 0x8);
    }
    *(s32*)(p + 0x748) = 0;

    // Objects whose def id is one of three known ids get the +0x80000 transform flag.
    {
        u32 id = *(u32*)(*(u8**)(p + 0x9C) + 0x8);
        s32 a = 1;
        s32 b = 1;
        if (id != 0x148685B7u && id != 0x06332A59u) a = 0;
        if (a == 0 && id != 0x84EA3654u) b = 0;
        if (b != 0) {
            u8* tr = *(u8**)(p + 0x4);
            *(u32*)(tr + 0x20) |= 0x80000;
        }
    }

    {
        u8* fx = *(u8**)(p + 0x15C);
        *(f32*)(p + 0x5F8) = zero;
        *(u8*)(p + 0x61C) = 0;
        *(f32*)(p + 0x614) = zero;
        *(f32*)(p + 0x618) = zero;
        *(f32*)(p + 0x610) = zero;
        *(f32*)(p + 0x16C) = zero;
        *(s32*)(p + 0x600) = 0;
        *(f32*)(p + 0x604) = zero;
        *(s32*)(p + 0x608) = 0xF;
        *(s32*)(p + 0x60C) = -1;
        *(f32*)(p + 0x6D0) = zero;
        if (fx != NULL) {
            fn_801C83AC(lbl_8047C760, fx);
            *(u8**)(p + 0x15C) = NULL;
        }
    }

    {
        s32 idx = fn_801EC880(p + 0x94, kHashCC70BCDE);
        if (idx >= 0) fn_801ECFA0(p + 0x94, idx);
    }
    *(f32*)(p + 0x638) = one;
    *(u32*)(p + 0x838) &= 0xEFFFFFFF;
    *(f32*)(p + 0x7FC) = Rand01_313();

    // Random per-channel tint: either bright (1 + [-v,0]*0.2) on the object colour at +0x800,
    // or a small random offset (0..v)*0.2 into the render object at +0x90.
    if (rand() % 2 == 0) {
        f32 v = *(f32*)(p + 0x7FC);
        f32 lo = -v;
        f32 hi = 0.0f;
        f32 c0, c1, c2, c3;
        *(f32*)(p + 0x800) = RandLerp_313(lo, hi);
        *(f32*)(p + 0x804) = RandLerp_313(lo, hi);
        *(f32*)(p + 0x808) = RandLerp_313(lo, hi);
        c3 = RandLerp_313(0.0f, 0.0f);
        c0 = *(f32*)(p + 0x800) * 0.2f;
        c1 = *(f32*)(p + 0x804) * 0.2f;
        c2 = *(f32*)(p + 0x808) * 0.2f;
        c3 = c3 * 0.2f;
        *(f32*)(p + 0x800) = c0 + 1.0f;
        *(f32*)(p + 0x804) = c1 + 1.0f;
        *(f32*)(p + 0x808) = c2 + 1.0f;
        *(f32*)(p + 0x80C) = c3 + 1.0f;
    } else {
        f32 v = *(f32*)(p + 0x7FC);
        f32 c0, c1, c2, c3;
        u8* rd;
        *(f32*)(p + 0x800) = one;
        *(f32*)(p + 0x804) = one;
        *(f32*)(p + 0x808) = one;
        *(f32*)(p + 0x80C) = one;
        c0 = RandLerp_313(0.0f, v);
        c1 = RandLerp_313(0.0f, v);
        c2 = RandLerp_313(0.0f, v);
        c3 = RandLerp_313(0.0f, 0.0f);
        rd = *(u8**)(p + 0x90);
        *(f32*)(rd + 0xC) = c0 * 0.2f;
        *(f32*)(rd + 0x10) = c1 * 0.2f;
        *(f32*)(rd + 0x14) = c2 * 0.2f;
        *(f32*)(rd + 0x18) = c3 * 0.2f;
    }

    {
        u8* rd = *(u8**)(p + 0x90);
        *(f32*)(rd + 0x38) = *(f32*)(p + 0x800);
        *(f32*)(rd + 0x3C) = *(f32*)(p + 0x804);
        *(f32*)(rd + 0x40) = *(f32*)(p + 0x808);
        *(f32*)(rd + 0x44) = *(f32*)(p + 0x80C);
    }

    *(s32*)(p + 0x674) = -1;
    *(s32*)(p + 0x678) = -1;
    *(f32*)(p + 0x670) = zero;
    *(f32*)(p + 0x694) = neg1;
    *(f32*)(p + 0x69C) = *(f32*)(p + 0x28);
    *(f32*)(p + 0x6A0) = zero;
    *(f32*)(p + 0x6A4) = zero;
    *(f32*)(p + 0x6A8) = zero;
    *(f32*)(p + 0x6AC) = zero;
    *(f32*)(p + 0x6B0) = zero;
    *(s32*)(p + 0x6BC) = 0;
    *(s32*)(p + 0x6C0) = -1;
    *(f32*)(p + 0x83C) = zero;
    *(u32*)(p + 0x838) &= 0xDFFFFFFF;
    *(u32*)(p + 0x838) &= 0xFBFFFFFF;
    *(f32*)(p + 0x850) = zero;
    *(f32*)(p + 0x854) = zero;
    *(f32*)(p + 0x858) = zero;
    *(f32*)(p + 0x844) = zero;
    *(f32*)(p + 0x848) = zero;
    *(f32*)(p + 0x84C) = zero;
    *(f32*)(p + 0x63C) = zero;
    *(f32*)(p + 0x640) = zero;
    *(f32*)(p + 0x644) = zero;
    *(f32*)(p + 0x620) = zero;
    *(f32*)(p + 0x624) = zero;
    *(f32*)(p + 0x628) = zero;
    *(f32*)(p + 0x62C) = neg1;
    *(f32*)(p + 0x630) = neg1;
    *(s32*)(p + 0x634) = -1;
    *(f32*)(p + 0x654) = zero;
    *(f32*)(p + 0x658) = zero;
    *(f32*)(p + 0x65C) = zero;
    *(s32*)(p + 0x660) = 0;
    *(f32*)(p + 0x648) = zero;
    *(f32*)(p + 0x64C) = zero;
    *(f32*)(p + 0x650) = zero;
    *(f32*)(p + 0x810) = zero;
    *(f32*)(p + 0x814) = zero;
    *(f32*)(p + 0x818) = zero;
    *(s32*)(p + 0x830) = -1;
    *(f32*)(p + 0x834) = zero;

    {
        u8** slot = *(u8***)(p + 0xA4);
        if (slot != NULL && *slot != NULL) fn_801BB478(*slot, 0);
    }

    if (*(u8**)(p + 0xA0) != NULL) {
        s32 node = fn_8018BDE0(*(u8**)(p + 0xA0), (s32)kHash9E216B17);
        *(s32*)(p + 0x678) = node;
        if (node < 0) {
            *(s32*)(p + 0x678) = fn_8018BDE0(*(u8**)(p + 0xA0), (s32)kHash9E216B17);
        }
        fn_801AC6B4(*(u8**)(*(u8**)(p + 0x9C) + 0x48), *(u8**)(p + 0xA0), p);
    }

    // If the def asks for it, drop the object onto the ground by tracing 5 units backwards
    // along its facing from its position.
    {
        u8* def = *(u8**)(p + 0x9C);
        if (*(s32*)(def + 0x240) != 0) {
            f32 hit[27];
            f32 pos[3];
            f32 dir[3];
            f32 end[3];
            f32 off[3];
            u8* tr;
            memset(hit, 0, sizeof(hit));
            *(s32*)((u8*)hit + 0x54) = 1;
            tr = *(u8**)(p + 0x4);
            pos[0] = *(f32*)(tr + 0x0);
            pos[1] = *(f32*)(tr + 0x4);
            pos[2] = *(f32*)(tr + 0x8);
            fn_801455E0(dir, *(u8**)(p + 0x4) + 0xC);
            off[0] = dir[0] * 5.0f;
            off[1] = dir[1] * 5.0f;
            off[2] = dir[2] * 5.0f;
            end[0] = pos[0] - off[0];
            end[1] = pos[1] - off[1];
            end[2] = pos[2] - off[2];
            if (fn_80138C40(gWorldState, pos, end, hit, NULL, 0, 0) != 0) {
                u8* t = *(u8**)(p + 0x4);
                f32 dz = *(f32*)(t + 0x8) - hit[2];
                if (dz < 1.0f) {
                    pos[0] = hit[0];
                    pos[1] = hit[1];
                    pos[2] = hit[2];
                    t = *(u8**)(p + 0x4);
                    *(f32*)(t + 0x0) = hit[0];
                    *(f32*)(t + 0x4) = pos[1];
                    *(f32*)(t + 0x8) = pos[2];
                    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x8000;
                    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x10000000;
                    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x4000;
                    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x10000000;
                }
            }
        }
    }

    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x4000;
    fn_801B4EA4(p, NULL);
    if (*(u8**)(p + 0x8C) != NULL) fn_801B1778(*(u8**)(p + 0x8C), 1);

    // Virtual call: slot +0x10 returns the model handle whose node 0x206114EF is the aim point.
    {
        typedef u8* (*GetModelFn)(u8*);
        u8* model = (*(GetModelFn*)(*(u8**)p + 0x10))(p);
        s32 node = fn_8018BDE0(model, (s32)kHash206114EF);
        *(s32*)(p + 0x7E8) = node;
        if (node != -1) {
            f32 xf[12];
            fn_801B55F4(p, (u8*)xf, node);
            *(f32*)(p + 0x7DC) = xf[9];
            *(f32*)(p + 0x7E0) = xf[10];
            *(f32*)(p + 0x7E4) = xf[11];
        } else {
            u8* tr = *(u8**)(p + 0x4);
            *(f32*)(p + 0x7DC) = *(f32*)(tr + 0x0);
            *(f32*)(p + 0x7E0) = *(f32*)(tr + 0x4);
            *(f32*)(p + 0x7E4) = *(f32*)(tr + 0x8);
        }
    }

    // Resolve the def's per-entry node ids (0x14-byte entries at def+0xC4).
    {
        u8* def = *(u8**)(p + 0x9C);
        u8* ent = def;
        u8* slot = p;
        s32 i;
        *(s32*)(p + 0x82C) = 0;
        for (i = 0; i < *(s32*)(def + 0x114); i++) {
            *(s32*)(ent + 0xC8) = fn_8018BDE0(*(u8**)(p + 0xA0), *(s32*)(ent + 0xC4));
            *(s32*)(ent + 0xD0) = fn_8018BDE0(*(u8**)(p + 0xA0), *(s32*)(ent + 0xCC));
            ent += 0x14;
            *(s32*)(slot + 0x81C) = 0;
            slot += 4;
        }
    }

    memset(p + 0x5A8, 0, 0x40);
    *(s32*)(p + 0x5E8) = 0;
    *(s32*)(p + 0x5EC) = 0;
    *(s32*)(p + 0x5F0) = 0;
    fn_80062DC4(p, 0, arg2);
}

// 0x8005E1B8
// Tracks the waypoint-zone node nearest the object: refreshes the current (+0xBA0) and next
// (+0xBA4) node ids, and when the object reaches the current node fires the zone event and
// advances along the route.
void fn_8005E1B8(u8* p) {
    u32 zone = *(u16*)(*(u8**)(p + 0x4) + 0x30);

    *(u32*)(p + 0xA08) &= ~0x20u;
    if (*(u8*)(*(u8**)(*(u8**)gWorldState + 0x1A4) + *(u16*)(*(u8**)(p + 0x4) + 0x30) * gZoneResourceSize + 0xC0) == 0)
        return;

    if (fn_801C6634(p + 0xBA0) == 0) {
        u8 tmp[4];
        fn_801C6724(tmp, *(u8**)(p + 0x4), (s32)zone);
        *(u8*)(p + 0xBA0) = tmp[0];
        *(u8*)(p + 0xBA1) = tmp[1];
        *(u16*)(p + 0xBA2) = *(u16*)(tmp + 2);
        if (fn_801C6634(p + 0xBA0) != 0) {
            u8* node = fn_801C668C(p + 0xBA0);
            if (node != NULL) {
                f32 x = *(f32*)(node + 0x4);
                f32 y = *(f32*)(node + 0x8);
                f32 z = *(f32*)(node + 0xC);
                *(f32*)(p + 0xB94) = x;
                *(f32*)(p + 0xB98) = y;
                *(f32*)(p + 0xB9C) = z;
                *(f32*)(p + 0x9E8) = x;
                *(f32*)(p + 0x9EC) = y;
                *(f32*)(p + 0x9F0) = z;
            }
            *(u32*)(p + 0xA08) |= 0x10;
        } else {
            *(u32*)(p + 0xA08) &= ~0x10u;
        }
    }
    if (fn_801C6634(p + 0xBA4) == 0) {
        u8 tmp[4];
        fn_801C6724(tmp, *(u8**)(p + 0x4), (s32)zone);
        *(u8*)(p + 0xBA4) = tmp[0];
        *(u8*)(p + 0xBA5) = tmp[1];
        *(u16*)(p + 0xBA6) = *(u16*)(tmp + 2);
    }
    if (fn_801C6634(p + 0xA0C) == 0) {
        *(u32*)(p + 0xA08) &= ~0x2u;
        *(u32*)(p + 0xA08) &= ~0x10u;
        return;
    }

    {
        u8* node = fn_801C668C(p + 0xBA0);
        if (node == NULL) return;
        {
            u8* tr = *(u8**)(p + 0x4);
            f32 dx = *(f32*)(node + 0x4) - *(f32*)(tr + 0x0);
            f32 dy = *(f32*)(node + 0x8) - *(f32*)(tr + 0x4);
            f32 dz = 0.0f;
            if (dz * dz + (dx * dx + dy * dy) < 0.01f) {
                EventSend(gEventMgr, p, NULL, kHashEventZoneNode, 0, 0, 0, 0);
                if (*(u8*)(p + 0xA0C) == *(u8*)(p + 0xBA0) && *(u8*)(p + 0xA0D) == *(u8*)(p + 0xBA1)) {
                    *(u32*)(p + 0xA08) |= 0x3;
                } else {
                    u8 cur[4];
                    u8 tgt[4];
                    u8 res[4];
                    u8 b1 = *(u8*)(p + 0xBA1);
                    u16 b2 = *(u16*)(p + 0xBA2);
                    *(u8*)(p + 0xBA4) = *(u8*)(p + 0xBA0);
                    *(u8*)(p + 0xBA5) = b1;
                    *(u16*)(p + 0xBA6) = b2;
                    cur[0] = *(u8*)(p + 0xA0C);
                    cur[1] = *(u8*)(p + 0xA0D);
                    *(u16*)(cur + 2) = *(u16*)(p + 0xA0E);
                    tgt[0] = *(u8*)(p + 0xBA0);
                    tgt[1] = *(u8*)(p + 0xBA1);
                    *(u16*)(tgt + 2) = *(u16*)(p + 0xBA2);
                    fn_801C6838(res, tgt, cur);
                    *(u8*)(p + 0xBA0) = res[0];
                    *(u8*)(p + 0xBA1) = res[1];
                    *(u16*)(p + 0xBA2) = *(u16*)(res + 2);
                    if (*(u8*)(p + 0xB84) == 2) *(u8*)(p + 0xB85) = 0;
                    if (fn_801C6634(p + 0xBA0) != 0) {
                        u8* n2 = fn_801C668C(p + 0xBA0);
                        if (n2 != NULL) {
                            f32 x = *(f32*)(n2 + 0x4);
                            f32 y = *(f32*)(n2 + 0x8);
                            f32 z = *(f32*)(n2 + 0xC);
                            *(f32*)(p + 0xB94) = x;
                            *(f32*)(p + 0xB98) = y;
                            *(f32*)(p + 0xB9C) = z;
                            *(f32*)(p + 0x9E8) = x;
                            *(f32*)(p + 0x9EC) = y;
                            *(f32*)(p + 0x9F0) = z;
                        }
                        *(u32*)(p + 0xA08) |= 0x10;
                    } else {
                        *(u32*)(p + 0xA08) &= ~0x10u;
                    }
                }
            }
        }
        if (*(u8*)(p + 0xA0C) != *(u8*)(p + 0xBA0) || *(u8*)(p + 0xA0D) != *(u8*)(p + 0xBA1)) {
            *(u32*)(p + 0xA08) &= ~0x2u;
        }
    }
}

// 0x800637AC
// Sets up the projectile's collision capsule: builds its side vector (+0x188), either from the
// cross product of the aim direction with the target / launch point or with the up axis, scales
// it by half the def's width, then registers the capsule with the collision manager.
// Returns 0 if the def's collision entry is missing, otherwise 1.
s32 fn_800637AC(u8* p, u8* arg1) {
    u8* def = *(u8**)(p + 0x228);
    u32 flags;
    f32 n[3];
    f32 len2;

    if (*(s32*)(def + 0xE8) == 0) return 1;

    flags = *(u32*)(p + 0x244);
    if (flags & 0x2000) {
        // Direction towards the camera target.
        u8* tr = *(u8**)(p + 0x4);
        u8* tgt = *(u8**)(*(u8**)(lbl_803C4298 + 4) + 4);
        f32 d[3];
        f32 v[3];
        f32 c[3];
        d[0] = *(f32*)(tgt + 0x0) - *(f32*)(tr + 0x0);
        d[1] = *(f32*)(tgt + 0x4) - *(f32*)(tr + 0x4);
        d[2] = *(f32*)(tgt + 0x8) - *(f32*)(tr + 0x8);
        if (flags & 0x20000) {
            v[0] = *(f32*)(p + 0x1A0) - *(f32*)(tr + 0x0);
            v[1] = *(f32*)(p + 0x1A4) - *(f32*)(tr + 0x4);
            v[2] = *(f32*)(p + 0x1A8) - *(f32*)(tr + 0x8);
        } else {
            v[0] = *(f32*)(p + 0x1AC);
            v[1] = *(f32*)(p + 0x1B0);
            v[2] = *(f32*)(p + 0x1B4);
        }
        c[0] = d[1] * v[2] - d[2] * v[1];
        c[1] = d[2] * v[0] - d[0] * v[2];
        c[2] = d[0] * v[1] - d[1] * v[0];
        len2 = c[2] * c[2] + (c[0] * c[0] + c[1] * c[1]);
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            n[0] = c[0] * inv;
            n[1] = c[1] * inv;
            n[2] = c[2] * inv;
        }
        {
            f32 s = 0.5f * *(f32*)(def + 0xEC);
            *(f32*)(p + 0x188) = n[0] * s;
            *(f32*)(p + 0x18C) = n[1] * s;
            *(f32*)(p + 0x190) = n[2] * s;
        }
    } else {
        // Side vector = v x (0,0,1), normalised, then scaled.
        f32 v[3];
        f32 x, y, z;
        if (flags & 0x20000) {
            u8* tr = *(u8**)(p + 0x4);
            v[0] = *(f32*)(p + 0x1A0) - *(f32*)(tr + 0x0);
            v[1] = *(f32*)(p + 0x1A4) - *(f32*)(tr + 0x4);
            v[2] = *(f32*)(p + 0x1A8) - *(f32*)(tr + 0x8);
        } else {
            v[0] = *(f32*)(p + 0x1AC);
            v[1] = *(f32*)(p + 0x1B0);
            v[2] = *(f32*)(p + 0x1B4);
        }
        *(f32*)(p + 0x188) = v[1] * 1.0f - v[2] * 0.0f;
        *(f32*)(p + 0x18C) = v[2] * 0.0f - v[0] * 1.0f;
        *(f32*)(p + 0x190) = v[0] * 0.0f - v[1] * 0.0f;
        x = *(f32*)(p + 0x188);
        y = *(f32*)(p + 0x18C);
        z = *(f32*)(p + 0x190);
        len2 = z * z + (x * x + y * y);
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);
            n[0] = x * inv;
            n[1] = y * inv;
            n[2] = z * inv;
        }
        *(f32*)(p + 0x188) = n[0];
        *(f32*)(p + 0x18C) = n[1];
        *(f32*)(p + 0x190) = n[2];
        {
            f32 s = 0.5f * *(f32*)(*(u8**)(p + 0x228) + 0xEC);
            *(f32*)(p + 0x188) = n[0] * s;
            *(f32*)(p + 0x18C) = n[1] * s;
            *(f32*)(p + 0x190) = n[2] * s;
        }
    }

    {
        u8* entry = fn_801C3DB8(lbl_8047C7AC, *(u32*)(*(u8**)(p + 0x228) + 0xE8));
        if (entry == NULL) return 0;
        if (*(s32*)(entry + 0x4C) != 0) {
            *(void**)(p + 0x238) = fn_801C3F6C(lbl_8047C7AC, entry);
        } else {
            *(void**)(p + 0x234) = fn_801C3F6C(lbl_8047C7AC, entry);
        }
    }

    if (*(u8**)(p + 0x234) != NULL) {
        u8* obj = *(u8**)(p + 0x234);
        u8* tr;
        f32 hi[3];
        f32 lo[3];
        fn_801C42A8(obj);
        tr = *(u8**)(p + 0x4);
        hi[0] = *(f32*)(tr + 0x0) + *(f32*)(p + 0x188);
        hi[1] = *(f32*)(tr + 0x4) + *(f32*)(p + 0x18C);
        hi[2] = *(f32*)(tr + 0x8) + *(f32*)(p + 0x190);
        tr = *(u8**)(p + 0x4);
        lo[0] = *(f32*)(tr + 0x0) - *(f32*)(p + 0x188);
        lo[1] = *(f32*)(tr + 0x4) - *(f32*)(p + 0x18C);
        lo[2] = *(f32*)(tr + 0x8) - *(f32*)(p + 0x190);
        obj = *(u8**)(p + 0x234);
        fn_801C4B90(obj, hi, lo, 0, 0, *(u8**)(*(u8**)obj + 0x4));
    } else if (*(u8**)(p + 0x238) != NULL) {
        u8* tr;
        f32 hi[3];
        f32 lo[3];
        fn_801C5C60(*(u8**)(p + 0x238));
        tr = *(u8**)(p + 0x4);
        hi[0] = *(f32*)(tr + 0x0) + *(f32*)(p + 0x188);
        hi[1] = *(f32*)(tr + 0x4) + *(f32*)(p + 0x18C);
        hi[2] = *(f32*)(tr + 0x8) + *(f32*)(p + 0x190);
        tr = *(u8**)(p + 0x4);
        lo[0] = *(f32*)(tr + 0x0) - *(f32*)(p + 0x188);
        lo[1] = *(f32*)(tr + 0x4) - *(f32*)(p + 0x18C);
        lo[2] = *(f32*)(tr + 0x8) - *(f32*)(p + 0x190);
        fn_801C5C74(*(u8**)(p + 0x238), hi, lo);
    }
    return 1;
}

// 0x8006784C
// Traces a short ray along the object's velocity (+0x1AC) and, on a hit, records the ground
// material hash (+0x14C), the impact point (+0x1B8), normal (+0x1C4) and distance (+0x230).
void fn_8006784C(u8* p) {
    u32 flags = *(u32*)(p + 0x244);

    if ((flags & 0x8000) && *(s32*)(p + 0x248) != 0 && (flags & 0x10000)) return;

    {
        f32 hit[27];
        f32 start[3];
        f32 end[3];
        u8* tr;
        u8* def;
        f32 s;

        memset(hit, 0, sizeof(hit));
        *(s32*)((u8*)hit + 0x54) = 4;

        tr = *(u8**)(p + 0x4);
        start[0] = *(f32*)(tr + 0x0);
        start[1] = *(f32*)(tr + 0x4);
        start[2] = *(f32*)(tr + 0x8);
        def = *(u8**)(p + 0x228);
        s = *(f32*)(def + 0xD0);
        {
            f32 dx = *(f32*)(p + 0x1AC) * s;
            f32 dy = *(f32*)(p + 0x1B0) * s;
            f32 dz = *(f32*)(p + 0x1B4) * s;
            tr = *(u8**)(p + 0x4);
            end[0] = *(f32*)(tr + 0x0) + dx;
            end[1] = *(f32*)(tr + 0x4) + dy;
            end[2] = *(f32*)(tr + 0x8) + dz;
        }
        *(u32*)(p + 0x244) &= 0xFBFFFFFF;
        if (fn_80138C40(gWorldState, start, end, hit, NULL, 0, 0) == 0) return;

        {
            u32 mat = (*(u32*)((u8*)hit + 0x48) >> 12) & 0xF;
            u32 hash;
            switch (mat) {
            case 1:  hash = kHashGoo;   break;
            case 2:  hash = kHashGrass; break;
            case 3:  hash = kHashWater; break;
            case 4:  hash = kHashWood;  break;
            case 5:  hash = kHashMat5;  break;
            case 6:  hash = kHashMetal; break;
            case 7:  hash = kHashTile;  break;
            case 8:  hash = kHashDirt;  break;
            case 9:  hash = kHashMat9;  break;
            default: hash = kHashDefault; break;
            }
            *(u32*)(p + 0x14C) = hash;
        }
        {
            f32 k = 0.1f;
            f32 ox = hit[3] * k;
            f32 oy = hit[4] * k;
            f32 oz = hit[5] * k;
            u32 f2 = *(u32*)(p + 0x244);
            *(f32*)(p + 0x1B8) = hit[0] + ox;
            *(f32*)(p + 0x1BC) = hit[1] + oy;
            *(f32*)(p + 0x1C0) = hit[2] + oz;
            *(f32*)(p + 0x1C4) = hit[3];
            *(f32*)(p + 0x1C8) = hit[4];
            *(f32*)(p + 0x1CC) = hit[5];
            *(u32*)(p + 0x244) = f2 | 0x4000000;
            *(f32*)(p + 0x230) = hit[6] * *(f32*)(*(u8**)(p + 0x228) + 0xD0);
        }
    }
}

}  // extern "C"
