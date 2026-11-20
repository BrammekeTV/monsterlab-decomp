// Decompiled from drafts: fn_80076D38, fn_8007B20C, fn_8007DDEC, fn_80086F84, fn_800943CC,
// fn_8009B320, fn_800B8830, fn_80119C6C, fn_801223C0, fn_8019B2F4, fn_801AC184, fn_801F8628.
#include <math.h>
#include <string.h>

#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp
struct Locator;

extern "C" {
BOOL   EntryAllowedAtRank(u8* e);                                     // rank gate for a lab entry
void   GameLog(int level, int category, const char* fmt, ...);
void*  MemAllocSmall(u32 size);
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);        // 0x80209E70
s32    RaySceneTest(void* ray);                                       // 0x80070458
void   ScopedLockCtor(ScopedLock* l);                                 // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                      // 0x801F53B0
u32    StringHash(const char* s);                                     // 0x801EB704
Locator* WorldFindLocator(void* world, u32 hash);                     // 0x8012AA50
void   fn_80077090(u8* self, const char* name);                       // 0x80077090
void fn_8007A4B4(u8* obj, u8* src);                                            // 0x8007A4B4
void   fn_80087144(u8* p);                                            // 0x80087144
void   fn_8009487C(u8* p);                                            // 0x8009487C
void   fn_800946E0(u8* p);                                            // 0x800946E0
void   fn_80096F6C(u8* p);                                            // 0x80096F6C
void   fn_800980C8(u8* obj, s32 mode);                                // 0x800980C8
void   fn_800988F0(u8* p, s32 b);                                     // 0x800988F0
void   fn_800C2180(u8* self, u8* tbl, s32 n);                         // 0x800C2180
void   fn_8018BEA4(u8* p, f32 t);                                     // 0x8018BEA4
u8*    fn_801B3308(void);                                             // 0x801B3308
void   fn_801B3310(u8* p, u8* q);                                     // 0x801B3310
void*  fn_801D40E0(s32 n);                                            // 0x801D40E0
const char* fn_801EB708(u32 hash);                                    // 0x801EB708
void   fn_801F6CC4(u8* p, const f32* fwd, const f32* up);             // 0x801F6CC4
void*  fn_80209814(void* v);                                          // 0x80209814
u32    fn_8020A9A8(s32 handle);                                       // 0x8020A9A8
void   fn_8020B1C8(void* handle, u32 hash, s32 a);                    // 0x8020B1C8
u8*    fn_8021F32C(u8* p);                                            // 0x8021F32C
u8*    fn_8021F368(u8* p, u8* seq);                                   // 0x8021F368
u32    fn_8021F7C0(u8* p);                                            // 0x8021F7C0
u32    fn_8021F7E8(u8* p);                                            // 0x8021F7E8
void   fn_8021FAA8(void);                                             // 0x8021FAA8
void   fn_8021FAFC(void);                                             // 0x8021FAFC
void   fn_8021FB28(void);                                             // 0x8021FB28
u8*    fn_8021FFA4(u8* p, u8* seq, s32 idx, void* cbA, void* cbB, f32 f);  // 0x8021FFA4
void   fn_8027D270(s32 a, f32 b, f32 c);                              // 0x8027D270

extern u8 gWorldState[];                 // 0x803E6D00 (first word: world block pointer)
extern void* gLab;
extern const char* lbl_805F8958;
extern f32 lbl_80347010[];               // table indexed by RaySceneTest
extern f32 lbl_80347230[];
extern f32 lbl_80347240[];
extern f32 lbl_80347250[];
extern f32 lbl_803469A8[];               // table indexed by RaySceneTest (clamped 0..3)
extern u8 lbl_803C6B60[];
extern s32 lbl_805F8F24;
extern s32 lbl_805FB27C;
extern s32 lbl_805FB280;
extern s32 lbl_805FB284;
extern void* lbl_805FBBAC;               // voice SFX handle
extern s32* lbl_805FB698;                // group count pointer
}
extern "C++" s32 gZoneResourceSize;      // 0x805FB94C

static const u32 kHashBrainHand    = 0xEA744A2Eu;  // "brain_handA"
static const u32 kHashBrainChunks  = 0x1C88E0ABu;  // "brain_chunksA"
static const u32 kHashBrainBody    = 0xB2E0CC76u;  // "brain_bodyA"
static const u32 kHashBrainPowerup = 0xE50FB3E0u;  // "brain_powerupA"
static const u32 kHashBrainPointer = 0xA0787F34u;  // "brain_pointerA"
static const u32 kHashObstacle01   = 0xC78ADFBEu;  // "astral_obstacle_01A"
static const u32 kHashObstacle02   = 0xECA78C7Du;  // "astral_obstacle_02A"
static const u32 kHashObstacle03   = 0xF5BCBD3Cu;  // "astral_obstacle_03A"
static const u32 kHashObstacle04   = 0xBAFD2BFBu;  // "astral_obstacle_04A"
static const u32 kHashObstacle05   = 0xA3E61ABAu;  // "astral_obstacle_05A"
static const u32 kHashObstacle06   = 0x88CB4979u;  // "astral_obstacle_06A"

typedef u8* (*ObjVFn)(u8* self);
typedef void (*ObjVFnArg)(u8* self, s32 a);
typedef void (*HitVFn)(u8* self, u8* other, s16 a, s8 b, f32* dir, f32 force);

// Zone resource block for zone index idx (world block +0x1A4 array, stride gZoneResourceSize).
static inline u8* ZoneRes(s32 idx) {
    return *(u8**)(*(u8**)gWorldState + 0x1A4) + idx * gZoneResourceSize;
}

// Object and its record (stride 0x40) for combined index i of a zone: first the static list
// (+0x94 entries), then the dynamic one (+0x98 entries).
static inline void ZoneObjAt(u8* res, s32 i, u8** obj, u8** rec) {
    s32 first = *(s32*)(res + 0x94);
    if (i < first) {
        *obj = ((u8**)*(u8**)(res + 0x120))[i];
        *rec = *(u8**)(res + 0x8C) + i * 0x40;
    } else {
        s32 j = i - first;
        *obj = ((u8**)*(u8**)(res + 0x12C))[j];
        *rec = *(u8**)(res + 0x90) + j * 0x40;
    }
}

// Lazily creates the shared voice SFX handle.
static inline void EnsureVoiceHandle() {
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
}

extern "C" {

// 0x80076D38
// Constructor: zeroes the first five floats, builds the sequence player pair (+0x14..+0x20)
// from the sequence data loaded by fn_80077090 and derives the tempo scale at +0x4/+0xC.
u8* fn_80076D38(u8* self, const char* name) {
    *(f32*)(self + 0x0) = 0.0f;
    *(f32*)(self + 0x4) = 0.0f;
    *(f32*)(self + 0x8) = 0.0f;
    *(f32*)(self + 0xC) = 0.0f;
    *(f32*)(self + 0x10) = 0.0f;
    fn_8021F32C(self + 0x24);
    *(s32*)(self + 0x1228) = 0;
    *(s32*)(self + 0x122C) = 0;
    *(s32*)(self + 0x1230) = 0;
    *(void**)(self + 0x1234) = NULL;
    *(void**)(self + 0x1238) = NULL;
    *(u8*)(self + 0x123C) = 0;
    *(s32*)(self + 0x12E0) = 0;
    *(s32*)(self + 0x12E4) = -1;
    *(s32*)(self + 0x12E8) = -1;
    fn_80077090(self, name);
    u8* seq = (u8*)fn_801D40E0(0xC);
    if (seq != NULL) seq = fn_8021F368(seq, *(u8**)(self + 0x1234));
    *(u8**)(self + 0x1238) = seq;
    f32 f31 = lbl_80347250[RaySceneTest(NULL)];
    f32 f30 = f31 + lbl_80347230[RaySceneTest(NULL)];
    f32 f29 = f31 + lbl_80347240[RaySceneTest(NULL)];
    u8* p = (u8*)fn_801D40E0(0x5074);
    if (p != NULL) p = fn_8021FFA4(p, *(u8**)(self + 0x1238), RaySceneTest(NULL), (void*)fn_8021FAFC, (void*)fn_8021FAFC, f31);
    *(u8**)(self + 0x14) = p;
    p = (u8*)fn_801D40E0(0x5074);
    if (p != NULL) p = fn_8021FFA4(p, *(u8**)(self + 0x1238), RaySceneTest(NULL), (void*)fn_8021FB28, (void*)fn_8021FAA8, 0.0f);
    *(u8**)(self + 0x18) = p;
    p = (u8*)fn_801D40E0(0x5074);
    if (p != NULL) p = fn_8021FFA4(p, *(u8**)(self + 0x1238), RaySceneTest(NULL), (void*)fn_8021FB28, (void*)fn_8021FAA8, f30);
    *(u8**)(self + 0x1C) = p;
    p = (u8*)fn_801D40E0(0x5074);
    if (p != NULL) p = fn_8021FFA4(p, *(u8**)(self + 0x1238), RaySceneTest(NULL), (void*)fn_8021FB28, (void*)fn_8021FAA8, f29);
    *(u8**)(self + 0x20) = p;
    f32 rate = (f32)fn_8021F7E8(*(u8**)(self + 0x14));
    *(f32*)(self + 0x4) = rate;
    *(f32*)(self + 0xC) = rate;
    *(f32*)(self + 0xC) = *(f32*)(self + 0xC) * (1.0f / (f32)fn_8021F7C0(*(u8**)(self + 0x14)));
    memset(self + 0x1028, 0xFF, 0x200);
    return self;
}

// 0x8007B20C
// Looks the object named by lbl_805F8958 up in the zone lists, registers it via fn_8007A4B4 and
// flags the brain as active.
void fn_8007B20C(u8* self) {
    u32 hash = StringHash(lbl_805F8958);
    fn_8027D270(0, 0.0f, 0.45f);
    s32 found = 0;
    for (s32 z = 0; z < *(s32*)(*(u8**)gWorldState + 0x4) + 1; z++) {
        for (s32 j = 0; j < *(s32*)(ZoneRes(z) + 0x98); j++) {
            u8* obj = ((u8**)*(u8**)(ZoneRes(z) + 0x12C))[j];
            if (hash == *(u32*)(obj + 0x2C)) {
                fn_8007A4B4(self + 0x264, NULL);  // r4 leftover in asm
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    *(s32*)(self + 0x338) = 1;
    *(u32*)(self + 0x15C) |= 0x200000;
    fn_801B3310(fn_801B3308(), self + 0x2A0);
}

// 0x8007DDEC
// Scans the second zone's flagged objects and stores the brain parts (hand, chunks, body,
// powerup, pointer) into the slots at +0x1F4..+0x2A4, then finds the record at +0x31C.
// The asm never reads its second (float) argument.
void fn_8007DDEC(u8* self, f32 unused) {
    *(void**)(self + 0x2A4) = NULL;
    u8* res = ZoneRes(1);
    s32 total = *(s32*)(res + 0x94) + *(s32*)(res + 0x98);
    for (s32 i = 0; i < total; i++) {
        u8* obj;
        u8* rec;
        ZoneObjAt(ZoneRes(1), i, &obj, &rec);
        if ((*(u32*)(rec + 0x1C) & 0x40) && obj != NULL) {
            u8* info = *(u8**)(obj + 0x9C);
            u32 type = (info != NULL) ? *(u32*)(info + 0x8) : 0;
            switch (type) {
            case kHashBrainHand: {
                *(u8**)(self + 0x1F4) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                u8* o = *(u8**)(self + 0x1F4);
                *(f32*)(o + 0x8) = 0.8f;
                *(f32*)(o + 0xC) = 0.8f;
                *(f32*)(o + 0x10) = 0.8f;
                *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x8000;
                *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x10000000;
                *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x4000;
                *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x10000000;
                break;
            }
            case kHashBrainChunks: {
                s32 n = *(s32*)(self + 0x298);
                ((u8**)(self + 0x1F8))[n] = obj;
                *(s32*)(self + 0x298) = *(s32*)(self + 0x298) + 1;
                break;
            }
            case kHashBrainBody:
                *(u8**)(self + 0x29C) = obj;
                *(f32*)(obj + 0x8) = 1.2f;
                *(f32*)(obj + 0xC) = 1.2f;
                *(f32*)(obj + 0x10) = 1.2f;
                *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x8000;
                *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
                *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x4000;
                *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
                break;
            case kHashBrainPowerup:
                *(u8**)(self + 0x2A0) = obj;
                break;
            case kHashBrainPointer:
                *(u8**)(self + 0x2A4) = obj;
                break;
            }
        }
    }
    for (s32 z = 0; z <= *(s32*)(*(u8**)gWorldState + 0x4); z++) {
        u8* zr = ZoneRes(z);
        for (s32 j = 0; j < (s32)*(u8*)(zr + 0xA4); j++) {
            u8* rec = *(u8**)(zr + 0xA0) + j * 0x4C;
            if (*(u32*)(rec + 0x2C) == 0x31E1E0B1u) *(u8**)(self + 0x31C) = rec;
        }
    }
}

// 0x80086F84
// Init: runs the virtual setup, resolves the ten locators at +0x358.., resets the state and
// sets the difficulty-scaled value at +0x484.
void fn_80086F84(u8* self) {
    (*(ObjVFn*)(*(u8**)(self + 0x4) + 0x54))(self);
    *(Locator**)(self + 0x358) = WorldFindLocator(gWorldState, 0x6991059Bu);
    *(Locator**)(self + 0x35C) = WorldFindLocator(gWorldState, 0x8944EC85u);
    *(Locator**)(self + 0x360) = WorldFindLocator(gWorldState, 0x19FBF114u);
    *(Locator**)(self + 0x364) = WorldFindLocator(gWorldState, 0x6EFCC182u);
    *(Locator**)(self + 0x368) = WorldFindLocator(gWorldState, 0xFE43DC13u);
    *(Locator**)(self + 0x36C) = WorldFindLocator(gWorldState, 0x1E96350Du);
    *(Locator**)(self + 0x370) = WorldFindLocator(gWorldState, 0xF7F59038u);
    *(Locator**)(self + 0x374) = WorldFindLocator(gWorldState, 0xB842572Bu);
    *(Locator**)(self + 0x378) = WorldFindLocator(gWorldState, 0xF0985421u);
    *(Locator**)(self + 0x37C) = WorldFindLocator(gWorldState, 0x80F2A0AEu);
    fn_80087144(self);
    *(f32*)(self + 0x47C) = 70.0f;
    *(f32*)(self + 0x480) = 0.0f;
    *(s32*)(self + 0x90) = 0;
    *(s32*)(self + 0x7C8) = 1;
    fn_801B3310(fn_801B3308(), self + 0x730);
    fn_801B3310(fn_801B3308(), self + 0x3C0);
    s32 d = RaySceneTest(NULL);
    if (d < 0) d = 0;
    else if (d > 3) d = 3;
    *(f32*)(self + 0x484) = 0.5f * lbl_803469A8[d];
    EnsureVoiceHandle();
    fn_8020B1C8(lbl_805FBBAC, 0xA5237C5Fu, 0);
}

// 0x800943CC
// Init: releases the object at +0x1D8, sets the difficulty value at +0x35C, clears the eight
// slots at +0x194, sets defaults and the two fixed orientation vectors for fn_801F6CC4.
void fn_800943CC(u8* self) {
    u8* old = *(u8**)(self + 0x1D8);
    if (old != NULL) {
        fn_800980C8(old, 0);
        *(u8**)(self + 0x1DC) = *(u8**)(self + 0x1D8);
    }
    *(s32*)(self + 0x1D8) = 0;
    *(s32*)(self + 0x1DC) = 0;
    f32 diff = lbl_80347010[RaySceneTest(NULL)];
    *(f32*)(self + 0x360) = -1.0f;
    *(f32*)(self + 0x35C) = diff;
    fn_8009487C(self);
    fn_800946E0(self);
    fn_80096F6C(self);
    for (s32 i = 0; i < 8; i++) fn_800988F0(((u8**)(self + 0x194))[i], 0);
    *(f32*)(self + 0x358) = 50.0f;
    *(f32*)(self + 0x36C) = 0.0f;
    *(u8*)(self + 0x370) = 0;
    *(f32*)(self + 0x378) = 0.0f;
    *(s32*)(self + 0x37C) = 0;
    *(f32*)(self + 0x374) = -1.0f;
    *(s32*)(self + 0x278) = 1;
    *(f32*)(self + 0x274) = 0.25f;
    fn_801B3310(fn_801B3308(), self + 0x1E0);
    Vec3 fwd = Vec3Normalized(Vec3{ 0.0f, 0.0f, -1.0f });
    Vec3 up = Vec3Normalized(Vec3{ 0.0f, -1.0f, 0.0f });
    fn_801F6CC4(self + 0x294, &fwd.x, &up.x);
    fn_801B3310(fn_801B3308(), self + 0x294);
    EnsureVoiceHandle();
    fn_8020B1C8(lbl_805FBBAC, 0xA5237C5Fu, 0);
    *(s32*)(self + 0x380) = -1;
}

// True for the six astral_obstacle_0xA object types.
static inline bool IsAstralObstacle(u32 h) {
    return h == kHashObstacle01 || h == kHashObstacle02 || h == kHashObstacle03 ||
           h == kHashObstacle04 || h == kHashObstacle05 || h == kHashObstacle06;
}

// 0x8009B320
// Counts the astral obstacles of the second zone into +0x14, allocates the pointer array at +0x10
// and fills it, flagging each (0x400000), running its virtual +0x18 hook and clearing byte +0x270.
void fn_8009B320(u8* self) {
    fn_800C2180(self, lbl_803C6B60, 6);
    *(s32*)(self + 0x14) = 0;
    u8* res = ZoneRes(1);
    s32 total = *(s32*)(res + 0x94) + *(s32*)(res + 0x98);
    for (s32 i = 0; i < total; i++) {
        u8* obj;
        u8* rec;
        ZoneObjAt(ZoneRes(1), i, &obj, &rec);
        if ((*(u32*)(rec + 0x1C) & 0x40) && obj != NULL) {
            u8* info = *(u8**)(obj + 0x9C);
            u32 type = (info != NULL) ? *(u32*)(info + 0x8) : 0;
            if (IsAstralObstacle(type)) *(s32*)(self + 0x14) = *(s32*)(self + 0x14) + 1;
        }
    }
    // original allocates count * 4 (32-bit pointers)
    *(void**)(self + 0x10) = MemAllocSmall(*(s32*)(self + 0x14) * sizeof(u8*));
    res = ZoneRes(1);
    total = *(s32*)(res + 0x94) + *(s32*)(res + 0x98);
    s32 k = 0;
    for (s32 i = 0; i < total; i++) {
        u8* obj;
        u8* rec;
        ZoneObjAt(ZoneRes(1), i, &obj, &rec);
        if ((*(u32*)(rec + 0x1C) & 0x40) && obj != NULL) {
            u8* info = *(u8**)(obj + 0x9C);
            u32 type = (info != NULL) ? *(u32*)(info + 0x8) : 0;
            if (IsAstralObstacle(type)) {
                u8** arr = (u8**)*(u8**)(self + 0x10);
                arr[k] = obj;
                *(u32*)(arr[k] + 0x15C) |= 0x400000;
                u8* o = ((u8**)*(u8**)(self + 0x10))[k];
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(ObjVFnArg*)(*(u8**)o + 0x18))(o, 0);
                ScopedLockDtor(&lock, -1);
                *(u8*)(((u8**)*(u8**)(self + 0x10))[k] + 0x270) = 0;
                k++;
            }
        }
    }
}

// 0x800B8830
// Eases the gauge value at +0x2B4 towards the target at +0x2B0 (or follows the input v scaled
// by +0x2A4 and +0x2E4), clamps it to 0..1, pushes it to the UI element and plays the loop
// SFX while the value is changing.
void fn_800B8830(u8* self, f32 v) {
    f32 prev = *(f32*)(self + 0x2B4);
    *(f32*)(self + 0x2B8) = prev;
    if (fabsf(v) < 0.02f) {
        if (fabsf(*(f32*)(self + 0x2C8)) < 5.0f) {
            f32 target = *(f32*)(self + 0x2B0);
            if (target > prev) {
                *(f32*)(self + 0x2B4) = prev + 0.02f;
            } else if (target < prev) {
                *(f32*)(self + 0x2B4) = prev - 0.02f;
            }
            target = *(f32*)(self + 0x2B0);
            if (fabsf(target - *(f32*)(self + 0x2B4)) <= 0.02f) *(f32*)(self + 0x2B4) = target;
        }
    } else {
        f32 step = 0.0f;
        f32 scale = *(f32*)(self + 0x2A4);
        if (scale > 0.0f) step = v / scale;
        if (fabsf(step) > 0.01f) step *= 0.01f / fabsf(step);
        *(f32*)(self + 0x2B4) = *(f32*)(self + 0x2B4) + (f32)*(s32*)(self + 0x2E4) * step;
    }
    f32 val = *(f32*)(self + 0x2B4);
    if (val < 0.0f) val = 0.0f;
    else if (val > 1.0f) val = 1.0f;
    *(f32*)(self + 0x2B4) = val;
    u8* elem = (*(ObjVFn*)(*(u8**)self + 0x10))(self);
    fn_8018BEA4(elem, *(f32*)(self + 0x2B4));
    f32 delta = *(f32*)(self + 0x2B8) - *(f32*)(self + 0x2B4);
    bool still = (delta > -0.0001f && delta < 0.0001f);
    if (!still) {
        EnsureVoiceHandle();
        if (fn_8020A9A8(*(s32*)(self + 0x2BC)) == 0) {
            EnsureVoiceHandle();
            *(s32*)(self + 0x2BC) = PlaySound(0xA939364Fu, 0, 0, 0, 0, 1);
        }
    }
}

// 0x80119C6C
// Refreshes the lab menu: texture/placeholder pairs for the three slots, the hit-test group,
// the rank-gated difficulty meter and the four animations selected by lbl_805F8F24.
void fn_80119C6C(u8* unused) {
    UIElement_SetVisibleFlag(0xFB02A3B7u, 0);
    if (lbl_805FB27C != 0) {
        UIImage_SetTexture(0xB6B6D325u, lbl_805FB27C);
        UIElement_SetVisibleFlag(0x06BE9A4Fu, 0);
    } else {
        UIElement_SetVisibleFlag(0xB6B6D325u, 0);
        UIElement_SetVisibleFlag(0x06BE9A4Fu, 1);
    }
    if (lbl_805FB280 != 0) {
        UIImage_SetTexture(0xFF3A8971u, lbl_805FB280);
        UIElement_SetVisibleFlag(0xB2ACFB4Fu, 0);
    } else {
        UIElement_SetVisibleFlag(0xFF3A8971u, 0);
        UIElement_SetVisibleFlag(0xB2ACFB4Fu, 1);
    }
    if (lbl_805FB284 != 0) {
        UIImage_SetTexture(0x7F9A9D72u, lbl_805FB284);
        UIElement_SetVisibleFlag(0x96558200u, 0);
    } else {
        UIElement_SetVisibleFlag(0x7F9A9D72u, 0);
        UIElement_SetVisibleFlag(0x96558200u, 1);
    }
    if (lbl_805FB27C != 0 && lbl_805FB280 != 0) {
        UIEvent_SendToElement(0xEBFC9568u, 0x0165E8A7u);
        UIGroup_EnableHitTest(0x0165E8A7u);
    } else {
        UIEvent_SendToElement(0x7CBE0C37u, 0x0165E8A7u);
        UIGroup_DisableHitTest(0x0165E8A7u);
    }
    s32 locked = !(gLab != NULL && EntryAllowedAtRank((u8*)gLab));
    if (locked) {
        UIEvent_SendToElement(0xC86AAC8Fu, 0x7E1BF2E2u);  // 0x7E1BF2E2 = "Difficulty_Meter"
        UIElement_PlayUIAnim(0x1C74EC52u, 0x8FD4A14Fu);
    } else {
        UIEvent_SendToElement(0xE96F9947u, 0x7E1BF2E2u);
        UIElement_PlayUIAnim(0x1C74EC52u, 0xF07F4EDEu);
    }
    switch (lbl_805F8F24) {
    case 0:
        UIElement_PlayUIAnim(0x89CCAE46u, 0x121736A5u);
        UIElement_PlayUIAnim(0x10C5FFFCu, 0x65A15F69u);
        UIElement_PlayUIAnim(0x67C2CF6Au, 0x65A15F69u);
        UIElement_PlayUIAnim(0xF9A65AC9u, 0x65A15F69u);
        break;
    case 1:
        UIElement_PlayUIAnim(0x89CCAE46u, 0x8B1E671Fu);
        UIElement_PlayUIAnim(0x10C5FFFCu, 0x8B1E671Fu);
        UIElement_PlayUIAnim(0x67C2CF6Au, 0x65A15F69u);
        UIElement_PlayUIAnim(0xF9A65AC9u, 0x65A15F69u);
        break;
    case 2:
        UIElement_PlayUIAnim(0x89CCAE46u, 0xFC195789u);
        UIElement_PlayUIAnim(0x10C5FFFCu, 0xFC195789u);
        UIElement_PlayUIAnim(0x67C2CF6Au, 0xFC195789u);
        UIElement_PlayUIAnim(0xF9A65AC9u, 0x65A15F69u);
        break;
    case 3:
        UIElement_PlayUIAnim(0x89CCAE46u, 0x627DC22Au);
        UIElement_PlayUIAnim(0x10C5FFFCu, 0x627DC22Au);
        UIElement_PlayUIAnim(0x67C2CF6Au, 0x627DC22Au);
        UIElement_PlayUIAnim(0xF9A65AC9u, 0x627DC22Au);
        break;
    default:
        UIElement_PlayUIAnim(0x89CCAE46u, 0x65A15F69u);
        UIElement_PlayUIAnim(0x10C5FFFCu, 0x65A15F69u);
        UIElement_PlayUIAnim(0x67C2CF6Au, 0x65A15F69u);
        UIElement_PlayUIAnim(0xF9A65AC9u, 0x65A15F69u);
        break;
    }
}

// 0x801223C0
// Radial hit callback: for a hit info record (+0x0 other object, +0x4 hit data with position,
// radius +0xC, strength +0x10/+0x14, values +0x18/+0x1A) builds a push vector falling off with
// distance and calls the target's virtual +0x50 handler. Always returns 1.
s32 fn_801223C0(u8* self, s32 unused, u8** info) {
    if (self != info[0] && (*(s32(**)(u8*))(*(u8**)self + 0x4C))(self) == 0) {
        u8* pos = *(u8**)(self + 0x4);
        if ((*(u32*)(pos + 0x20) >> 1) & 1) {
            u8* other = info[0];
            u8* hit = *(u8**)((u8*)info + 0x4);
            Vec3 d;
            d.x = *(f32*)(pos + 0x0) - *(f32*)(hit + 0x0);
            d.y = *(f32*)(pos + 0x4) - *(f32*)(hit + 0x4);
            d.z = *(f32*)(pos + 0x8) - *(f32*)(hit + 0x8);
            f32 dist2 = d.z * d.z + (d.x * d.x + d.y * d.y);
            f32 radius = *(f32*)(hit + 0xC);
            f32 falloff = 1.0f - dist2 / (radius * radius);
            f32 force = *(f32*)(hit + 0x10) * falloff;
            s32 n = (s32)(0.25f + (f32)*(s16*)(hit + 0x1A) * falloff);
            Vec3 dir = Vec3Normalized(d);
            f32 s = *(f32*)(hit + 0x14) * falloff;
            f32 vec[3];
            vec[0] = dir.x * s;
            vec[1] = dir.y * s;
            vec[2] = dir.z * s;
            if (vec[2] < 1.0f) {
                vec[2] = 0.0f;
            } else if (vec[2] < 3.0f) {
                vec[2] = 3.0f;
            }
            if (self == *(u8**)(*(u8**)gWorldState + 0x8) && ((*(u32*)(*(u8**)(other + 0x4) + 0x1C) >> 6) & 1)) {
                force *= 0.33f;
            }
            (*(HitVFn*)(*(u8**)self + 0x50))(self, other, *(s16*)(hit + 0x18), (s8)n, vec, force);
        }
    }
    return 1;
}

// 0x8019B2F4
// UI element base init: identity matrices/transforms, white colours, no handles, flags 0x20003.
void fn_8019B2F4(u8* obj) {
    *(s32*)(obj + 0x0) = -1;
    *(s32*)(obj + 0x60) = -1;
    *(s32*)(obj + 0x64) = 0x20003;
    *(f32*)(obj + 0x68) = 0.0f;
    *(f32*)(obj + 0x6C) = 0.0f;
    *(f32*)(obj + 0x70) = 0.0f;
    *(f32*)(obj + 0x74) = 1.0f;
    *(f32*)(obj + 0x78) = 1.0f;
    *(f32*)(obj + 0x7C) = 1.0f;
    *(f32*)(obj + 0x80) = 1.0f;
    *(f32*)(obj + 0x84) = 1.0f;
    *(f32*)(obj + 0x88) = 1.0f;
    *(f32*)(obj + 0x8C) = 1.0f;
    *(f32*)(obj + 0x90) = 0.0f;
    *(f32*)(obj + 0x94) = 0.0f;
    *(f32*)(obj + 0x98) = 0.0f;
    *(f32*)(obj + 0x9C) = 0.0f;
    *(f32*)(obj + 0xA0) = 0.0f;
    *(f32*)(obj + 0xA4) = 0.0f;
    *(s32*)(obj + 0x58) = 1;
    *(f32*)(obj + 0x5C) = 0.0f;
    *(s32*)(obj + 0xA8) = 0x3FFFF;
    *(s32*)(obj + 0xAC) = 0x3FFFF;
    *(s32*)(obj + 0xB0) = 0x3FFFF;
    *(s32*)(obj + 0x120) = 0;
    *(s32*)(obj + 0xB4) = 0x3FFFF;
    *(s32*)(obj + 0x124) = 0;
    *(s32*)(obj + 0x12C) = 0;
    *(s32*)(obj + 0x128) = 0;
    *(f32*)(obj + 0xF0) = 1.0f;
    *(f32*)(obj + 0xF4) = 0.0f;
    *(f32*)(obj + 0xF8) = 0.0f;
    *(f32*)(obj + 0xFC) = 0.0f;
    *(f32*)(obj + 0x100) = 1.0f;
    *(f32*)(obj + 0x104) = 0.0f;
    *(f32*)(obj + 0x108) = 0.0f;
    *(f32*)(obj + 0x10C) = 0.0f;
    *(f32*)(obj + 0x110) = 1.0f;
    *(f32*)(obj + 0x114) = 0.0f;
    *(f32*)(obj + 0x118) = 0.0f;
    *(f32*)(obj + 0x11C) = 0.0f;
    *(f32*)(obj + 0x14) = 0.0f;
    *(f32*)(obj + 0x18) = 0.0f;
    *(f32*)(obj + 0x1C) = 0.0f;
    *(f32*)(obj + 0x20) = 1.0f;
    *(f32*)(obj + 0x24) = 1.0f;
    *(f32*)(obj + 0x28) = 1.0f;
    *(f32*)(obj + 0x2C) = 1.0f;
    *(f32*)(obj + 0x30) = 1.0f;
    *(f32*)(obj + 0x34) = 1.0f;
    *(f32*)(obj + 0x38) = 1.0f;
    *(f32*)(obj + 0x3C) = 0.0f;
    *(f32*)(obj + 0x40) = 0.0f;
    *(f32*)(obj + 0x44) = 0.0f;
    *(f32*)(obj + 0x48) = 0.0f;
    *(f32*)(obj + 0x4C) = 0.0f;
    *(f32*)(obj + 0x50) = 0.0f;
    *(s32*)(obj + 0x54) = 0x20003;
    *(s32*)(obj + 0x10) = 0;
    *(s32*)(obj + 0x4) = 0;
    *(s32*)(obj + 0x8) = 0;
    *(s32*)(obj + 0xC) = 0;
    *(s32*)(obj + 0x130) = 0;
    *(s32*)(obj + 0x134) = 0;
    *(u8*)(obj + 0xB9) = 0xFF;
    *(u8*)(obj + 0xB8) = 0xFF;
    *(s32*)(obj + 0xBC) = 0;
}

// 0x801AC184
// Finds the anim_def entry with the given handle: sums the u16 counts at +0x4 (one per group,
// group count read through lbl_805FB698), then scans that many 0x44-byte entries at +0xC.
u8* fn_801AC184(u8* defs, s32 handle) {
    if (handle == 0) return NULL;
    s32 total = 0;
    s32 groups = *lbl_805FB698;
    for (s32 i = 0; i < groups; i++) total += ((u16*)*(u8**)(defs + 0x4))[i];
    for (s32 i = 0; i < total; i++) {
        u8* entry = *(u8**)(defs + 0xC) + i * 0x44;
        if ((u32)handle == *(u32*)(entry + 0x2C)) return entry;
    }
    GameLog(4, 0, "Could not locate anim_def entry with handle %s\n", fn_801EB708((u32)handle));
    return NULL;
}

// 0x801F8628
// Stores the count at +0x1E0 and fills the unit vectors at +0xA8 (stride 0xC) from n signed-byte
// xyz triples (zero length stays zero).
void fn_801F8628(u8* obj, s8* src, s32 n) {
    *(s32*)(obj + 0x1E0) = n;
    u8* dst = obj;
    for (s32 i = 0; i < *(s32*)(obj + 0x1E0); i++) {
        Vec3 v = Vec3Normalized(Vec3{ (f32)src[0], (f32)src[1], (f32)src[2] });
        src += 3;
        *(f32*)(dst + 0xA8) = v.x;
        *(f32*)(dst + 0xAC) = v.y;
        *(f32*)(dst + 0xB0) = v.z;
        dst += 0xC;
    }
}

}  // extern "C"
