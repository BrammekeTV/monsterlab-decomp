// Miscellaneous small functions (event broadcast, entity copy, ...), draft batch miss54.
#include "types.h"
#include "game/part.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>

extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gEventMgr;           // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;     // 0x805FB94C
extern "C" {
extern u8 gWorldState[];                // 0x803E6D00
extern const u32 kAbilityEffects[];     // 0x80388258
u8* GetGlobalSettings(void);            // 0x801D2BF8
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void fn_801BC350(u8* a, u8* b);         // 0x801BC350
void fn_801BB478(u8* p, s32 a1);        // 0x801BB478
u8* fn_80216658(GameDb* db, s32 key);   // 0x80216658
u32 EffectEmitter(void* db, s32 effect, s32 limb);   // 0x800523B8
s32 fn_801EBFBC(u8* a, u8* b);          // 0x801EBFBC
s32 fn_801DA5F4(s32 a, u8* b, s32 c);          // 0x801DA5F4
void fn_8018C0A8(u8* h, void* a);       // 0x8018C0A8
void fn_8018B2CC(u8* obj, u8* entry, s32 a, s32 b, f32 c, f32 d);   // 0x8018B2CC
void fn_8018A948(u8* obj, f32 t);       // 0x8018A948
void EntityUpdate(void* entity);        // 0x80010298
void fn_8010B200(f32* out, void* a, f32 t);   // 0x8010B200
void fn_800776C8(u8* p);                // 0x800776C8
void fn_80077820(u8* p);                // 0x80077820
void fn_80077948(u8* p);                // 0x80077948
void fn_800779F4(u8* p);                // 0x800779F4
s32 fn_8021F6D8(s32 h, s32* v);         // 0x8021F6D8
s32 fn_80220170(s32 h);                 // 0x80220170
void fn_802205F0(s32 h, s32 v);         // 0x802205F0
void fn_80220178(s32 h);                // 0x80220178
void fn_802200EC(s32 h);                // 0x802200EC
void fn_8021F35C(u8* p);                // 0x8021F35C
f64 fn_8033ED6C(f32 a, f32 b, f32 c, f32 d, f32 e);   // 0x8033ED6C
u32 RandomU32(void);                    // 0x80143460
s32 fn_8007C50C(u8* p);                 // 0x8007C50C
s32 fn_8007CB68(u8* p, s32 mode);       // 0x8007CB68
s32 fn_8000849C(u8* p);                 // 0x8000849C
void fn_80078BB8(u8* p);                // 0x80078BB8
void fn_80079004(u8* p);                // 0x80079004
f64 fn_8033E8EC(f64 x);                 // 0x8033E8EC
void CameraSetFov(void* camera, f32 fov);   // 0x801D8ACC
s32 fn_800D8FC4(u8* p, s32* list, s32 count, f32 a, f32 b, f32 c);   // 0x800D8FC4
extern u8 lbl_803C4298[];               // 0x803C4298
extern u8 lbl_8047C640[];               // 0x8047C640 (camera)
extern f32 lbl_805FB144;                // 0x805FB144
void fn_800690D4(u8* p, s32 a, void* params, s32 b);   // 0x800690D4 (base ctor)
s32 fn_8007A394(u8* a, u8* b);          // 0x8007A394
u8* fn_8015EC9C(u8* tbl, f32 a, f32 b);   // 0x8015EC9C (stub, returns tbl)
u8* fn_801F9544(u8* p, s32 a4);         // 0x801F9544
extern u8 lbl_8035AED8[];               // 0x8035AED8 (vtable)
extern u8 lbl_8040D500[];               // 0x8040D500
void fn_8007B9E4(f32* out, u8* obj, u8* offs);          // 0x8007B9E4
void fn_8007B7BC(f32* out, u8* obj, f32 x, f32 z);      // 0x8007B7BC
u32 fn_801EC880(u8* p, u32 hash);       // 0x801EC880
void fn_80145F9C(f32* out, f32* a, f32* b, f32 t);   // 0x80145F9C
extern f32 lbl_806010E0[2];             // 0x806010E0 (x, z)
extern f32 lbl_806010E8[2];             // 0x806010E8 (x, z)
extern f32 lbl_80347450[3];             // 0x80347450
extern f32 lbl_8034745C[3];             // 0x8034745C
extern f32 lbl_80347468[3];             // 0x80347468
extern f32 lbl_80347474[3];             // 0x80347474
extern f32 lbl_80347480[3];             // 0x80347480
extern f32 lbl_8034748C[3];             // 0x8034748C
struct ScopedLock { u8 pad[4]; };       // as in props.cpp
void ScopedLockCtor(ScopedLock* l);     // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);   // 0x801F53B0
ScriptValue SetPropState(u8* prop, u32 state, s32 a2);   // src/game/draft_miss2.cpp
extern u8 lbl_80347D88[];               // 0x80347D88 (s32 table of prop states)
u8* fn_80081A34(void);                   // 0x80081A34
extern f32 lbl_803464E4[];              // 0x803464E4
void* fn_801B50FC(u8* p);               // 0x801B50FC
s32 fn_8008E5E0(u8* p, s32 a);          // 0x8008E5E0
s32 fn_8008D234(u8* h, f32 a, f32 b);   // 0x8008D234
s32 fn_8008D194(u8* h, u8* owner, f32 a, f32 b);   // 0x8008D194
struct Locator;
Locator* WorldFindLocator(void* world, u32 hash);   // 0x8012AA50
void fn_8008F1F8(u8* p);                // 0x8008F1F8
BOOL fn_80216BF0(u8* p);               // 0x80216BF0
s32 RaySceneTest(void* ray);            // 0x80070458
s32 fn_80216C60(u8* p, s32 a);          // 0x80216C60
void fn_801F8F34(u8* p, f32 v);          // 0x801F8F34
void fn_801F8FA0(u8* p, f32 v);          // 0x801F8FA0
s32 fn_801F8628(u8* p, s8* v, s32 n);   // 0x801F8628
u8* fn_801B3308(void);                  // 0x801B3308
void fn_801B3310(u8* p, u8* q);         // 0x801B3310
void fn_801B3378(u8* p, u8* key);       // 0x801B3378
void fn_80091E44(u8* p);                 // 0x80091E44
void fn_80093A88(u8* p);                // 0x80093A88
void fn_800923DC(u8* p);                // 0x800923DC
void TextureRelease(void* tex);         // 0x80184028
void fn_8020A398(s32 handle);           // 0x8020A398
void fn_801ECFA0(u8* p, s32 idx);       // 0x801ECFA0
void fn_801EC8C4(u8* p, u32 a, u32 b);  // 0x801EC8C4
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
void* fn_801D40E0(s32 n);               // 0x801D40E0
void* fn_80209814(void* v);             // 0x80209814
extern f32 lbl_80347200[4];             // 0x80347200
extern u8* lbl_803C6A70[3];             // 0x803C6A70
extern void* lbl_805FBBAC;              // 0x805FBBAC
u8* fn_801DA788(u8* a3, s32 i);         // 0x801DA788
void fn_80097BCC(f32* out, u8* obj);    // 0x80097BCC
f64 fn_8033F1F4(f64 x);                 // 0x8033F1F4
extern s8 lbl_805FB150;                 // 0x805FB150
extern f32 lbl_803C6AF0[4];             // 0x803C6AF0
extern s32 lbl_80347150[4];             // 0x80347150
extern s32 lbl_80347160[4];             // 0x80347160
extern u8 lbl_80348F38[];               // 0x80348F38
void* MemAllocSmall(u32 size);          // 0x801D41CC
void* fn_8032F028(void* mem, void* ctor, s32 a, s32 size, s32 count);   // 0x8032F028
void* memcpy(void* d, const void* s, size_t n);   // 0x80004338
s32 fn_800A5164(void);                  // 0x800A5164
void fn_800A50B4(u8** out, u8* tree, s32* key);   // 0x800A50B4
void fn_800A2AEC(u8* node, u8* root, s32 count);  // 0x800A2AEC
s32 fn_80336BF8(u8* file, const char* fmt, ...);  // 0x80336BF8
void fn_8033B09C(void);                 // 0x8033B09C
void operator_delete(void* p);          // 0x801D426C
extern u8 __files[];                    // 0x803B4978
extern u8 lbl_8035D4C0[];               // 0x8035D4C0 (vtable)
extern u8 lbl_8035D5C8[];               // 0x8035D5C8 (vtable)
extern u8 lbl_8035D6D0[];               // 0x8035D6D0 (vtable)
// @@DECLS
}

// Sum of the thirteen per-category counts in the global settings block.
static s32 SettingsSlotSum(void) {
    static const u8 offs[13] = {0x4C, 0x34, 0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x38, 0x3C, 0x40, 0x44, 0x48};
    s32 t = 0;
    for (int k = 0; k < 13; k++) t += *(s32*)(GetGlobalSettings() + offs[k]);
    return t;
}

// Marks the entity record as moved (flags +0x20: 0x8000, 0x10000000, 0x4000, 0x10000000).
static void MarkMoved(u8* o) {
    *(u32*)(o + 0x20) |= 0x8000;
    *(u32*)(o + 0x20) |= 0x10000000;
    *(u32*)(o + 0x20) |= 0x4000;
    *(u32*)(o + 0x20) |= 0x10000000;
}

// Calls virtual slot +0x14(obj, 1) under a scoped lock.
static void LockedVCall14(u8* obj) {
    ScopedLock l;
    ScopedLockCtor(&l);
    (*(void (**)(u8*, s32))(*(u8**)obj + 0x14))(obj, 1);
    ScopedLockDtor(&l, -1);
}

static const u32 kHashProjection = 0x8B9BD519u;   // "Projection"

// Drops one reference of a texture-like object; releases it when the count (+0x24) hits zero.
static void ReleaseRef(u8* tex) {
    s32 n = *(s32*)(tex + 0x24) - 1;
    *(s32*)(tex + 0x24) = n;
    if (n == 0) TextureRelease(tex);
}

// Runs fn_801ECFA0 for the three effect slots of the monster record list at +0x94.
static void ClearEffectSlots(u8* list) {
    fn_801ECFA0(list, fn_801EC880(list, 0x53B643B8u));
    fn_801ECFA0(list, fn_801EC880(list, 0x37B4DA5Bu));
    fn_801ECFA0(list, fn_801EC880(list, kHashProjection));
}

// In-order successor in a tree whose nodes hold left +0, right +4 and parent +8 (low bit = colour).
static u8* TreeNext(u8* n) {
    if (*(u8**)(n + 4) != NULL) {
        n = *(u8**)(n + 4);
        while (*(u8**)n != NULL) n = *(u8**)n;
        return n;
    }
    for (;;) {
        u8* parent = (u8*)((uintptr_t) * (u8**)(n + 8) & ~(uintptr_t)1);
        bool done = (n == *(u8**)parent);
        n = parent;
        if (done) return n;
    }
}

extern "C" {

// 0x8012EB74
// Sends an event to every object of every zone slot whose type is 6 (or the current slot); the
// objects are filtered by bit arg4 of their +0x1C mask and by arg5 against their +0x20 mask.
s32 fn_8012EB74(u8* arg0, u32 ev, s32 a, s32 b, s32 arg4, s32 arg5) {
    s32 i = 0;
    s32 off = 0;
    s32 total;
    while (total = SettingsSlotSum(), i < total + 1) {
        s32 slot = -1;
        if (i == SettingsSlotSum()) {
            slot = 0;
        } else {
            u8* e = *(u8**)(gWorldState + 0xB0) + off;
            if (*(s32*)(e + 4) == 6) slot = *(s32*)(e + 8);
        }
        if (slot >= 0) {
            s32 j = 0, o64 = 0, o4 = 0;
            u8* z = *(u8**)(*(u8**)gWorldState + 0x1A4) + slot * gZoneResourceSize;
            s32 n = *(s32*)(z + 0x94) + *(s32*)(z + 0x98);
            for (; j < n; j++, o64 += 0x40, o4 += 4) {
                z = *(u8**)(*(u8**)gWorldState + 0x1A4) + slot * gZoneResourceSize;
                s32 cnt = *(s32*)(z + 0x94);
                u8* target;
                u8* rec;
                if (j < cnt) {
                    target = *(u8**)(*(u8**)(z + 0x120) + o4);
                    rec = *(u8**)(z + 0x8C) + o64;
                } else {
                    s32 k = j - cnt;
                    target = *(u8**)(*(u8**)(z + 0x12C) + k * 4);
                    rec = *(u8**)(z + 0x90) + (k << 6);
                }
                if ((arg4 == 0 || (*(s32*)(rec + 0x1C) & (1 << arg4))) &&
                    (arg5 == 0 || (*(s32*)(rec + 0x20) & arg5)) && target != NULL) {
                    EventSend(gEventMgr, target, target, ev, a, b, 1, 0);
                }
            }
        }
        off += 0x30;
        i++;
    }
    return total;
}

// 0x801BBC9C
// Walks a chain of linked nodes (arg2), notifying the owner (arg1) through its vtable and
// updating the cursor (+0x4 index / +0xC last flagged index / +0x8 previous index) in arg0.
void fn_801BBC9C(u8* arg0, u8* arg1, u8* arg2) {
    typedef void (*VFn)(u8*, u8*);
    typedef u8* (*VNext)(u8*);
    u8* owner = arg1;
    u8* node = arg2;
    *(s32*)(arg0 + 0x28) |= 0xC;
    if (*(s32*)(arg2 + 0x10) != 0) {
        fn_801BC350(node, arg0);
    } else {
        s32 prev = *(s32*)(arg0 + 0x4);
        while (node != NULL) {
            fn_801BC350(node, arg0);
            (*(VFn*)(*(u8**)owner + 0x14))(owner, arg0);
            owner = *(u8**)(node + 0x4);
            if (*(s32*)(owner + 0x8) == 1) {
                if (owner == NULL) {
                    *(s32*)(arg0 + 0x4) = -1;
                } else {
                    u8* tbl = *(u8**)arg0;
                    u8* base = tbl ? *(u8**)tbl : NULL;
                    s32 idx = (s32)((owner - base) / 0x30);
                    *(s32*)(arg0 + 0x4) = idx;
                    if (tbl != NULL && idx > -1 && idx < *(s32*)(tbl + 0x4)) {
                        if (*(u32*)(*(u8**)tbl + idx * 0x30 + 0x1C) & 1) *(s32*)(arg0 + 0xC) = idx;
                    } else {
                        *(s32*)(arg0 + 0x4) = -1;
                    }
                }
            } else {
                *(s32*)(arg0 + 0x4) = -1;
            }
            *(s32*)(arg0 + 0x8) = prev;
            (*(VFn*)(*(u8**)owner + 0x10))(owner, arg0);
            node = (*(VNext*)(*(u8**)owner + 0x18))(owner);
        }
        *(s32*)(arg0 + 0x28) |= 0x30;
    }
    if (*(s32*)(arg0 + 0x4) == -1) fn_801BB478(arg0, 0);
}

// 0x8004E8E8
// Once per entity (flag +0x38E): plays the three ability effects of its first part at the
// matching emitters when the game db flag at +0x10 is 1.
void fn_8004E8E8(u8* arg0) {
    typedef u8* (*GetPart)(u8*);
    #define PART() ((*(GetPart*)(*(u8**)arg0 + 0xCC))(arg0))
    if (*(u8*)(arg0 + 0x38F) != 0 && PART() != 0 && *(void**)PART() != NULL && *(u8*)(arg0 + 0x38E) == 0) {
        u8* list = fn_80216658(gGameDb, **(s32**)PART());
        if (list != NULL && *(s32*)((u8*)gGameDb + 0x10) == 1) {
            u8* e = list;
            for (s32 i = 0; i < 3; i++, e += 4) {
                if (*(s32*)(e + 0x4) != -1 && *(s32*)(e + 0x10) != -1) {
                    u32 em = EffectEmitter(gGameDb, *(s32*)(e + 0x10), *(s32*)(PART() + 0x4));
                    PlayAttachedParticleEffectAtEmitter(arg0, kAbilityEffects[*(s32*)(e + 0x4)], em);
                }
            }
        }
        *(u8*)(arg0 + 0x38E) = 1;
    }
    #undef PART
}

// 0x8004FCB4
// Copies the state (+0x9C, +0x94 list, colour/param block +0xC8..+0xF8) from arg1 into arg0 and
// refreshes arg0's model instance. Returns 1.
s32 fn_8004FCB4(u8* arg0, u8* arg1) {
    typedef u8* (*VFn0)(u8*);
    typedef void (*VFn20)(u8*, s32, f32);
    if (arg1 != NULL) {
        *(s32*)(arg0 + 0x9C) = *(s32*)(arg1 + 0x9C);
        if (*(s32*)(arg1 + 0x94) > 0) fn_801EBFBC(arg0 + 0x94, arg1 + 0x94);
        if (*(s32*)(arg0 + 0x90) != 0) {
            u8* m = (*(VFn0*)(*(u8**)arg1 + 0xC))(arg1);
            if (m != 0) fn_801DA5F4(*(s32*)(arg0 + 0x90), m, 1);
            for (s32 o = 0xC8; o <= 0xE8; o += 4) *(f32*)(arg0 + o) = *(f32*)(arg1 + o);
            for (s32 o = 0xEC; o <= 0xF8; o += 4) *(s32*)(arg0 + o) = *(s32*)(arg1 + o);
            fn_8018C0A8(*(u8**)(arg0 + 0xA0), (*(VFn0*)(*(u8**)arg0 + 0xC))(arg0));
            u8* h = *(u8**)(arg1 + 0xA0);
            fn_8018B2CC(*(u8**)(arg0 + 0xA0), *(u8**)(*(u8**)(*(u8**)h + 0x14) + 0x14), 0, 0, 0.075f, 0.075f);
            fn_8018A948(*(u8**)(arg0 + 0xA0), 0.0f);
            *(s32*)(arg0 + 0xB8) |= 4;
            EntityUpdate(arg0);
            (*(VFn20*)(*(u8**)arg0 + 0x20))(arg0, 0, 0.0f);
        }
    }
    return 1;
}

// 0x8005FD4C
// Inserts the 0x20-byte record arg1 into the 8-entry list at arg0+0xA04, kept sorted by the
// float at +0x1C (descending). A record with an existing key (where it would not rank higher)
// is dropped. Inserting a record with key 0x14DB00B3 at the head sets the byte at +0xBDC.
void fn_8005FD4C(u8* arg0, u8* arg1) {
    for (u32 i = 0; i < 8; i++) {
        u8* slot = arg0 + i * 0x20;
        if (*(f32*)(slot + 0xA20) < *(f32*)(arg1 + 0x1C)) {
            for (s32 j = 7; j > (s32)i; j--) {
                u8* d = arg0 + 0xA04 + j * 0x20;
                u8* sr = d - 0x20;
                *(u32*)(d + 0x0) = *(u32*)(sr + 0x0);
                *(u32*)(d + 0x4) = *(u32*)(sr + 0x4);
                *(u8*)(d + 0x8) = *(u8*)(sr + 0x8);
                *(u8*)(d + 0x9) = *(u8*)(sr + 0x9);
                *(u16*)(d + 0xA) = *(u16*)(sr + 0xA);
                *(u8*)(d + 0xC) = *(u8*)(sr + 0xC);
                *(u8*)(d + 0xD) = *(u8*)(sr + 0xD);
                *(u16*)(d + 0xE) = *(u16*)(sr + 0xE);
                *(u32*)(d + 0x10) = *(u32*)(sr + 0x10);
                *(u32*)(d + 0x14) = *(u32*)(sr + 0x14);
                *(u32*)(d + 0x18) = *(u32*)(sr + 0x18);
                *(f32*)(d + 0x1C) = *(f32*)(sr + 0x1C);
            }
            u8* d = arg0 + 0xA04 + i * 0x20;
            *(u32*)(d + 0x0) = *(u32*)(arg1 + 0x0);
            *(u32*)(d + 0x4) = *(u32*)(arg1 + 0x4);
            *(u8*)(d + 0x8) = *(u8*)(arg1 + 0x8);
            *(u8*)(d + 0x9) = *(u8*)(arg1 + 0x9);
            *(u16*)(d + 0xA) = *(u16*)(arg1 + 0xA);
            *(u8*)(d + 0xC) = *(u8*)(arg1 + 0xC);
            *(u8*)(d + 0xD) = *(u8*)(arg1 + 0xD);
            *(u16*)(d + 0xE) = *(u16*)(arg1 + 0xE);
            *(u32*)(d + 0x10) = *(u32*)(arg1 + 0x10);
            *(u32*)(d + 0x14) = *(u32*)(arg1 + 0x14);
            *(u32*)(d + 0x18) = *(u32*)(arg1 + 0x18);
            *(f32*)(d + 0x1C) = *(f32*)(arg1 + 0x1C);
            if (i == 0 && *(s32*)(arg0 + 0xA04) == 0x14DB00B3) *(u8*)(arg0 + 0xBDC) = 1;
            return;
        }
        if (*(u32*)arg1 == *(u32*)(slot + 0xA04)) return;
    }
}

// 0x80076BA4
// Advances a timed position blend (+0x268 elapsed / +0x264 duration, curve at +0x270) and writes
// the result into the position of the linked object (+0x4). At the end of the blend it clears
// the active byte, sends event 0x3B46FE41 and snaps the object to the start of the curve.
void fn_80076BA4(u8* arg0, f32 dt) {
    f32 pos[3];
    f32 t = *(f32*)(arg0 + 0x268) + dt;
    f32 frac = t / *(f32*)(arg0 + 0x264);
    *(f32*)(arg0 + 0x268) = t;
    if (frac > 1.0f) frac = 1.0f;
    fn_8010B200(pos, *(void**)(arg0 + 0x270), frac);
    u8* o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0x0) = pos[0];
    *(f32*)(o + 0x4) = pos[1];
    *(f32*)(o + 0x8) = pos[2];
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    if (frac >= 1.0f) {
        *(u8*)(arg0 + 0x26C) = 0;
        EventSend(gEventMgr, arg0, NULL, 0x3B46FE41u, 0, 0, 1, 0);
        fn_8010B200(pos, *(void**)(arg0 + 0x270), 0.0f);
        o = *(u8**)(arg0 + 0x4);
        *(f32*)(o + 0x0) = pos[0];
        *(f32*)(o + 0x4) = pos[1];
        *(f32*)(o + 0x8) = pos[2];
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    }
}

// 0x800773CC
// Per-frame update of the scrolling table (active byte +0x123C): integrates the position
// (+0x0/+0x8 by velocity +0x4/+0xC times dt, +0x10 decays), then runs the four channel
// handles (+0x14..) and the follow-up updates.
void fn_800773CC(u8* arg0, f32 dt) {
    if (*(u8*)(arg0 + 0x123C) != 0) {
        f32 dx = dt * *(f32*)(arg0 + 0x4);
        f32 dz = dt * *(f32*)(arg0 + 0xC);
        f32 x = *(f32*)(arg0 + 0x0) + dx;
        f32 z = *(f32*)(arg0 + 0x8) + dz;
        *(f32*)(arg0 + 0x0) = x;
        *(f32*)(arg0 + 0x8) = z;
        *(f32*)(arg0 + 0x10) = *(f32*)(arg0 + 0x10) - dt;
        s32 n = (s32)(f32)fn_8033ED6C(x, z, dz, x, dx);
        *(f32*)(arg0 + 0x0) = *(f32*)(arg0 + 0x0) - (f32)(u32)n;
        for (s32 i = 0; i < 4; i++) {
            s32 h = *(s32*)(arg0 + 0x14 + i * 4);
            s32 v = n;
            s32 r = 0;
            while (r == 0) {
                r = fn_8021F6D8(h, &v);
                if (r == 0) {
                    u8* slot = arg0 + fn_80220170(h) * 4;
                    if (*(s32*)(slot + 0x1028) == -1) {
                        RandomU32();
                        *(s32*)(slot + 0x1028) = 0;
                    }
                    fn_802205F0(h, *(s32*)(slot + 0x1028));
                    fn_80220178(h);
                }
            }
        }
        fn_800776C8(arg0);
        fn_80077820(arg0);
        fn_80077948(arg0);
        fn_800779F4(arg0);
        for (s32 i = 0; i < 4; i++) fn_802200EC(*(s32*)(arg0 + 0x14 + i * 4));
        fn_8021F35C(arg0 + 0x24);
    }
}

// 0x800792B8
// Resets and re-registers the view list: clears the 0x8C entries, the two extra items, then
// collects their +0x8 words and recomputes the camera/viewport field of view from the sizes of
// the layout node (+0x758).
void fn_800792B8(u8* arg0) {
    s32 list[22];
    for (s32 i = 0; i < *(s32*)(arg0 + 0x8C); i++) {
        fn_8007C50C(*(u8**)(arg0 + 0x90 + i * 4));
        fn_8007CB68(*(u8**)(arg0 + 0x90 + i * 4), 2);
    }
    fn_8007C50C(arg0 + 0x1D0);
    fn_8007C50C(arg0 + 0x210);
    fn_8000849C(lbl_803C4298);
    *(s32*)(arg0 + 0x8C) = 0xF;
    fn_80078BB8(arg0);
    fn_80079004(arg0);
    list[0] = *(s32*)(arg0 + 0x1D8);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x8C); i++) list[i + 1] = *(s32*)(*(u8**)(arg0 + 0x90 + i * 4) + 0x8);
    list[*(s32*)(arg0 + 0x8C) + 1] = *(s32*)(arg0 + 0x218);
    u8* node = *(u8**)(arg0 + 0x758);
    s32 count = *(s32*)(arg0 + 0x8C) + 2;
    CameraSetFov(lbl_8047C640, 1.5707964f * (2.0f * (0.63661975f * (f32)fn_8033E8EC((0.5f * 4.8f) / *(f32*)(node + 0x28)))));
    f32 a = (f32)fn_8033E8EC((0.5f * 6.4f) / *(f32*)(node + 0x28));
    f32 b = 0.63661975f * a;
    f32 c = 2.0f * b;
    lbl_805FB144 = 90.0f * c;
    fn_800D8FC4(node, list, count, c, b, a);
}

// 0x8007AFC4
// Constructor: base ctor, vtable, then caches 14 float parameters from the lbl_8040D500
// table (the lookups are stubs that return the table itself).
void* fn_8007AFC4(u8* arg0, s32 arg1, void* arg2, s32 arg3) {
    fn_800690D4(arg0, arg1, arg2, arg3);
    *(void**)arg0 = lbl_8035AED8;
    fn_8007A394(arg0 + 0x264, lbl_8035AED8);
    *(s32*)(arg0 + 0x29C) = 0;
    *(f32*)(arg0 + 0x298) = 0.0f;
    fn_801F9544(arg0 + 0x2A0, *(s32*)(lbl_803C4298 + 0x38));
    u8* t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f);
    *(f32*)(arg0 + 0x358) = *(f32*)(t + 0x60);
    *(f32*)(arg0 + 0x35C) = *(f32*)(t + 0x64);
    *(f32*)(arg0 + 0x360) = *(f32*)(t + 0x68);
    *(f32*)(arg0 + 0x364) = *(f32*)(t + 0x6C);
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f);
    *(f32*)(arg0 + 0x368) = *(f32*)(t + 0x70);
    *(f32*)(arg0 + 0x36C) = *(f32*)(t + 0x74);
    *(f32*)(arg0 + 0x370) = *(f32*)(t + 0x78);
    *(f32*)(arg0 + 0x374) = *(f32*)(t + 0x7C);
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f);
    *(f32*)(arg0 + 0x378) = *(f32*)(t + 0x80);
    *(f32*)(arg0 + 0x37C) = *(f32*)(t + 0x84);
    *(f32*)(arg0 + 0x380) = *(f32*)(t + 0x88);
    *(f32*)(arg0 + 0x384) = *(f32*)(t + 0x8C);
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f);
    *(f32*)(arg0 + 0x388) = *(f32*)(t + 0x24);
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f);
    *(f32*)(arg0 + 0x38C) = *(f32*)(t + 0x1C);
    return arg0;
}

// 0x8007B320
// Places the linked object (+0x4) at the offset computed by fn_8007B9E4 plus arg1, sets its
// orientation block (+0xC..) from fn_8007B7BC, then moves the secondary object (+0x264->+0x4)
// by the part-table delta (record hash 0x1EB241CB, +0x5C/+0x64 minus the primary's old position).
void fn_8007B320(u8* arg0, u8* arg1) {
    f32 off[3] = {0.0f, 0.0f, 0.0f};
    f32 q[4];
    fn_8007B9E4(off, arg0, arg1);
    fn_8007B7BC(q, arg0, off[0], off[2]);
    u8* o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0xC) = q[0];
    *(f32*)(o + 0x10) = q[1];
    *(f32*)(o + 0x14) = q[2];
    *(f32*)(o + 0x18) = q[3];
    MarkMoved(*(u8**)(arg0 + 0x4));
    u8* rec = *(u8**)(arg0 + 0x98) + fn_801EC880(arg0 + 0x94, 0x1EB241CBu) * 0xC8;
    f32 rx = *(f32*)(rec + 0x5C);
    f32 rz = *(f32*)(rec + 0x64);
    o = *(u8**)(arg0 + 0x4);
    f32 ox = *(f32*)(o + 0x0);
    f32 oz = *(f32*)(o + 0x8);
    f32 x = off[0] + *(f32*)(arg1 + 0x0);
    f32 z = off[2] + *(f32*)(arg1 + 0x8);
    *(f32*)(o + 0x0) = x;
    *(f32*)(o + 0x4) = 0.0f;
    *(f32*)(o + 0x8) = z;
    MarkMoved(*(u8**)(arg0 + 0x4));
    u8* o2 = *(u8**)(*(u8**)(arg0 + 0x264) + 0x4);
    *(f32*)(o2 + 0x0) = (rx - ox) + x;
    *(f32*)(o2 + 0x4) = 0.0f + 0.0f;
    *(f32*)(o2 + 0x8) = (rz - oz) + z;
    MarkMoved(o2);
}

// 0x8007B560
// Places the linked object (+0x4) at the fixed point lbl_806010E0 plus arg1 (y = 0) and
// refreshes its orientation block from fn_8007B7BC(0, 0).
void fn_8007B560(u8* arg0, u8* arg1) {
    f32 q[4];
    u8* o = *(u8**)(arg0 + 0x4);
    f32 x = lbl_806010E0[0] + *(f32*)(arg1 + 0x0);
    f32 z = lbl_806010E0[1] + *(f32*)(arg1 + 0x8);
    *(f32*)(o + 0x0) = x;
    *(f32*)(o + 0x4) = 0.0f;
    *(f32*)(o + 0x8) = z;
    MarkMoved(*(u8**)(arg0 + 0x4));
    fn_8007B7BC(q, arg0, 0.0f, 0.0f);
    o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0xC) = q[0];
    *(f32*)(o + 0x10) = q[1];
    *(f32*)(o + 0x14) = q[2];
    *(f32*)(o + 0x18) = q[3];
    MarkMoved(*(u8**)(arg0 + 0x4));
}

// 0x8007B690
// Same as fn_8007B560 but the base point is the origin.
void fn_8007B690(u8* arg0, u8* arg1) {
    f32 q[4];
    f32 x = 0.0f + *(f32*)(arg1 + 0x0);
    f32 z = 0.0f + *(f32*)(arg1 + 0x8);
    u8* o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0x0) = x;
    *(f32*)(o + 0x4) = 0.0f;
    *(f32*)(o + 0x8) = z;
    MarkMoved(*(u8**)(arg0 + 0x4));
    fn_8007B7BC(q, arg0, 0.0f, 0.0f);
    o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0xC) = q[0];
    *(f32*)(o + 0x10) = q[1];
    *(f32*)(o + 0x14) = q[2];
    *(f32*)(o + 0x18) = q[3];
    MarkMoved(*(u8**)(arg0 + 0x4));
}

// 0x8007BABC
// Advances the blend timer (+0x390) and moves the linked object (+0x4) from the fixed point
// lbl_806010E0 toward the offset from fn_8007B9E4 (plus arg1), interpolating the orientation
// with fn_80145F9C. Sends event 0x20B1A890 once the timer reaches 1.
void fn_8007BABC(u8* arg0, u8* arg1, f32 dt) {
    f32 t = *(f32*)(arg0 + 0x390) + dt;
    f32 off[3];
    f32 qa[4], qb[4], qr[4];
    *(f32*)(arg0 + 0x390) = t;
    fn_8007B9E4(off, arg0, arg1);
    fn_8007B7BC(qa, arg0, 0.0f, 0.0f);
    fn_8007B7BC(qb, arg0, off[0], off[2]);
    fn_80145F9C(qr, qa, qb, t);
    u8* o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0xC) = qr[0];
    *(f32*)(o + 0x10) = qr[1];
    *(f32*)(o + 0x14) = qr[2];
    *(f32*)(o + 0x18) = qr[3];
    MarkMoved(*(u8**)(arg0 + 0x4));
    f32 gx = lbl_806010E0[0];
    f32 gz = lbl_806010E0[1];
    f32 x = ((off[0] - gx) * t + gx) + *(f32*)(arg1 + 0x0);
    f32 z = ((off[2] - gz) * t + gz) + *(f32*)(arg1 + 0x8);
    o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0x0) = x;
    *(f32*)(o + 0x4) = 0.0f;
    *(f32*)(o + 0x8) = z;
    MarkMoved(*(u8**)(arg0 + 0x4));
    if (t >= 1.0f) EventSend(gEventMgr, arg0, NULL, 0x20B1A890u, 0, 0, 1, 0);
}

// 0x8007BCCC
// Reverse of fn_8007BABC: moves from the fn_8007B9E4 offset toward the fixed point
// lbl_806010E8, then sends event 0x20B1A890 once the timer reaches 1.
void fn_8007BCCC(u8* arg0, u8* arg1, f32 dt) {
    f32 t = *(f32*)(arg0 + 0x390) + dt;
    f32 off[3];
    f32 qa[4], qb[4], qr[4];
    *(f32*)(arg0 + 0x390) = t;
    fn_8007B9E4(off, arg0, arg1);
    fn_8007B7BC(qa, arg0, 0.0f, 0.0f);
    fn_8007B7BC(qb, arg0, off[0], off[2]);
    fn_80145F9C(qr, qb, qa, t);
    u8* o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0xC) = qr[0];
    *(f32*)(o + 0x10) = qr[1];
    *(f32*)(o + 0x14) = qr[2];
    *(f32*)(o + 0x18) = qr[3];
    MarkMoved(*(u8**)(arg0 + 0x4));
    f32 gx = lbl_806010E8[0];
    f32 gz = lbl_806010E8[1];
    f32 x = ((gx - off[0]) * t + off[0]) + *(f32*)(arg1 + 0x0);
    f32 y = ((0.0f - off[1]) * t + off[1]);
    f32 z = ((gz - off[2]) * t + off[2]) + *(f32*)(arg1 + 0x8);
    (void)y;
    o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0x0) = x;
    *(f32*)(o + 0x4) = 0.0f;
    *(f32*)(o + 0x8) = z;
    MarkMoved(*(u8**)(arg0 + 0x4));
    if (t >= 1.0f) EventSend(gEventMgr, arg0, NULL, 0x20B1A890u, 0, 0, 1, 0);
}

// 0x8007C140
// Fills the shared parameter table lbl_8040D500 (three 4-float rows at +0x60/+0x70/+0x80 and
// two scalars) with preset A.
void fn_8007C140(void) {
    u8* t;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x60) = lbl_80347450[0];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x64) = lbl_80347450[1];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x68) = lbl_80347450[2];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x6C) = 1.0f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x70) = lbl_80347468[0];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x74) = lbl_80347468[1];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x78) = lbl_80347468[2];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x7C) = 1.0f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x80) = lbl_8034745C[0];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x84) = lbl_8034745C[1];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x88) = lbl_8034745C[2];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x8C) = 1.0f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x24) = 2.4f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x1C) = 0.5f;
}

// 0x8007C278
// Same as fn_8007C140 with preset B.
void fn_8007C278(void) {
    u8* t;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x60) = lbl_80347474[0];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x64) = lbl_80347474[1];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x68) = lbl_80347474[2];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x6C) = 1.0f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x70) = lbl_8034748C[0];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x74) = lbl_8034748C[1];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x78) = lbl_8034748C[2];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x7C) = 1.0f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x80) = lbl_80347480[0];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x84) = lbl_80347480[1];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x88) = lbl_80347480[2];
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x8C) = 1.0f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x24) = 1.4f;
    t = fn_8015EC9C(lbl_8040D500, 0.0f, 0.0f); *(f32*)(t + 0x1C) = 0.9f;
}

// 0x8007E354
// Broadcasts event arg1 to the sub-objects of arg0: the objects at +0x1F4, +0x29C and +0x2A0
// and the +0x298 entries at +0x1F8. Each is first poked through vtable slot 0x14 under a lock;
// the +0x2A0 object also has bit 1 cleared (on itself and its +0x4 record) if it was not set.
void fn_8007E354(u8* arg0, u32 arg1) {
    typedef u8* (*VGet)(u8*);
    u8* o = *(u8**)(arg0 + 0x1F4);
    if (o != NULL) {
        LockedVCall14(o);
        EventSend(gEventMgr, *(u8**)(arg0 + 0x1F4), NULL, arg1, 0, 0, 1, 0);
    }
    o = *(u8**)(arg0 + 0x29C);
    if (o != NULL) {
        LockedVCall14(o);
        EventSend(gEventMgr, *(u8**)(arg0 + 0x29C), NULL, arg1, 0, 0, 1, 0);
    }
    o = *(u8**)(arg0 + 0x2A0);
    if (o != NULL) {
        u32 bit1 = (*(u32*)(*(u8**)(o + 0x4) + 0x20) >> 1) & 1;
        LockedVCall14(o);
        if (bit1 == 0) {
            u8* p = *(u8**)(arg0 + 0x2A0);
            if ((*(VGet*)(*(u8**)p + 0xC))(p) != NULL) {
                u8* q = (*(VGet*)(*(u8**)p + 0xC))(p);
                *(u32*)(q + 0x8) &= ~2u;
            }
            *(u32*)(*(u8**)(p + 0x4) + 0x20) &= ~2u;
        }
        EventSend(gEventMgr, *(u8**)(arg0 + 0x2A0), NULL, arg1, 0, 0, 1, 0);
    }
    for (s32 i = 0; i < *(s32*)(arg0 + 0x298); i++) {
        o = *(u8**)(arg0 + 0x1F8 + i * 4);
        if (o != NULL) {
            LockedVCall14(o);
            EventSend(gEventMgr, *(u8**)(arg0 + 0x1F8 + i * 4), NULL, arg1, 0, 0, 1, 0);
        }
    }
}

// 0x8007F900
// Picks a free prop slot (byte +0x268 == 0, the last one wins) among the +0x298 entries at
// +0x1F8, else the entry with a matching key (0xB487463E) and the smallest +0x270 timer, and
// starts it in state arg1 (state table lbl_80347D88). Returns the slot or NULL.
u8* fn_8007F900(u8* arg0, s32 arg1) {
    s32 count = *(s32*)(arg0 + 0x298);
    u8* best = NULL;
    for (s32 i = 0; i < count; i++) {
        u8* e = *(u8**)(arg0 + 0x1F8 + i * 4);
        if (*(u8*)(e + 0x268) == 0) best = e;
    }
    if (best == NULL) {
        f32 minT = 1e10f;
        for (s32 i = 0; i < count; i++) {
            u8* e = *(u8**)(arg0 + 0x1F8 + i * 4);
            u8* cursor = **(u8***)(e + 0xA4);
            s32 idx = *(s32*)(cursor + 0x4);
            u8* entry = idx > -1 ? **(u8***)cursor + idx * 0x30 : NULL;
            u32 key = entry ? *(u32*)(entry + 0x4) : 0;
            if (key == 0xB487463Eu) {
                f32 t = *(f32*)(e + 0x270);
                if (t < minT) {
                    best = e;
                    minT = t;
                }
            }
        }
    }
    if (best != NULL) {
        *(s32*)(best + 0x26C) = 1;
        *(s32*)(best + 0x264) = arg1;
        SetPropState(best, *(u32*)(lbl_80347D88 + arg1 * 4), 0);
        *(u32*)(best + 0x15C) |= 0x400000;
    }
    return best;
}

// 0x80083930
// Classifies the distance (in the XZ plane) from the reference object (fn_80081A34) to the
// point arg1, scaled by the range at +0xB4 of the +0x264 object: 0 if within 0.05 of
// lbl_803464E4[0], 2 if within 0.05 of lbl_803464E4[2], else 1.
s32 fn_80083930(u8* arg0, f32* arg1) {
    u8* ref = fn_80081A34();
    f32 dx = arg1[0] - *(f32*)(ref + 0x8);
    f32 dz = arg1[1] - *(f32*)(ref + 0xC);
    f32 sq = 0.0f * 0.0f + (dx * dx + dz * dz);
    f32 len = 0.0f;
    if (!(sq <= 0.0f)) len = (f32)sqrt((f64)sq);   // original: frsqrte with two Newton steps
    f32 r = len / *(f32*)(*(u8**)(arg0 + 0x264) + 0xB4);
    if (fabsf(lbl_803464E4[0] - r) < 0.05) return 0;
    if (fabsf(lbl_803464E4[2] - r) < 0.05) return 2;
    return 1;
}

// 0x8008684C
// Looks up the record with key 0x1DC9D499 in the table of arg1's vtable slot 0x10 result,
// takes its vector (+0x24..+0x2C) and transforms it by arg1's 3x4 matrix (fn_801B50FC),
// storing the result in arg0.
void fn_8008684C(f32* arg0, u8* arg1) {
    typedef u8** (*VGet)(u8*);
    u8* tbl = *(*(VGet*)(*(u8**)arg1 + 0x10))(arg1);
    s32 count = *(s32*)(tbl + 0x8);
    s32 idx = -1;
    for (s32 i = 0; i < count; i++) {
        if (*(u32*)(*(u8**)(tbl + 0xC) + i * 8) == 0x1DC9D499u) {
            idx = i;
            break;
        }
    }
    u8* e = *(u8**)(tbl + 0x4) + idx * 0x30;
    arg0[0] = *(f32*)(e + 0x24);
    arg0[1] = *(f32*)(e + 0x28);
    arg0[2] = *(f32*)(e + 0x2C);
    f32* m = (f32*)fn_801B50FC(arg1);
    f32 x = arg0[0], y = arg0[1], z = arg0[2];
    f32 ox = m[0x24 / 4] + ((m[0x18 / 4] * z) + ((m[0x0 / 4] * x) + (m[0xC / 4] * y)));
    f32 oy = m[0x28 / 4] + ((m[0x1C / 4] * z) + ((m[0x4 / 4] * x) + (m[0x10 / 4] * y)));
    f32 oz = m[0x2C / 4] + ((m[0x20 / 4] * z) + ((m[0x8 / 4] * x) + (m[0x14 / 4] * y)));
    arg0[0] = ox;
    arg0[1] = oy;
    arg0[2] = oz;
}

// 0x8008BF04
// Adds the monster arg1 to the party list (up to 50 entries at +0x294, count at +0x35C):
// rescales the existing entries' slots, appends arg1 and returns 1; 0 if full or invalid.
s32 fn_8008BF04(u8* arg0, u8* arg1) {
    if (*(s32*)(arg0 + 0x35C) == 0x32 || arg1 == NULL || *(u8*)(arg0 + 0x268) == 0) return 0;
    s32 v = *(s32*)(arg1 + 0x268);
    if (v != 0) {
        *(s32*)(arg0 + 0x370) = v;
        if (*(s32*)(arg0 + 0x35C) > 0) *(s32*)(arg0 + 0x374) = v;
    }
    s32 cnt = *(s32*)(arg0 + 0x35C);
    f32 base = 0.4f + 0.15f * (f32)(cnt - 1);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x35C); i++)
        fn_8008D234(*(u8**)(arg0 + 0x294 + i * 4), base,
                    (4.0f * (f32)(i + 1)) / (f32)(*(s32*)(arg0 + 0x35C) + 1));
    fn_8008E5E0(arg1, 1);
    *(u8**)(arg0 + 0x294 + *(s32*)(arg0 + 0x35C) * 4) = arg1;
    fn_8008D194(*(u8**)(arg0 + 0x294 + *(s32*)(arg0 + 0x35C) * 4), arg0, base, 4.0f);
    *(s32*)(arg0 + 0x35C) += 1;
    return 1;
}

// 0x8008E340
// Steers the object (+0x4) toward the goal at +0x2C4..+0x2CC: moves half a unit along the
// direction to the goal in Y/Z and accelerates it sideways in X (speed at +0x2B4, change of
// +-15/s, clamped to +-25). Sends event 0xC82F907B when within 0.5 of the goal in Z.
void fn_8008E340(u8* arg0, f32 dt) {
    *(s32*)(arg0 + 0x264) = 8;
    u8* o = *(u8**)(arg0 + 0x4);
    f32 px = *(f32*)(o + 0x0), py = *(f32*)(o + 0x4), pz = *(f32*)(o + 0x8);
    f32 gx = *(f32*)(arg0 + 0x2C4), gy = *(f32*)(arg0 + 0x2C8), gz = *(f32*)(arg0 + 0x2CC);
    f32 dx = gx - px, dy = gy - py, dz = gz - pz;
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 nx = 0.0f, ny = 0.0f, nz = 0.0f;
    if (len2 != 0.0f) {
        f32 inv = (f32)(1.0 / sqrt((f64)len2));   // original: frsqrte with two Newton steps
        nx = dx * inv;
        ny = dy * inv;
        nz = dz * inv;
    }
    f32 tx = px + nx * 0.5f;
    f32 ty = py + ny * 0.5f;
    f32 tz = pz + nz * 0.5f;
    f32 sideways = *(f32*)(arg0 + 0x2C4) - *(f32*)(o + 0x0);
    f32 dir = 0.0f;
    if (sideways != 0.0f) dir = sideways < 0.0f ? -1.0f : 1.0f;
    f32 v = *(f32*)(arg0 + 0x2B4) + 15.0f * dir * dt;
    *(f32*)(arg0 + 0x2B4) = v;
    if (fabsf(v) > 25.0f) *(f32*)(arg0 + 0x2B4) = v < -25.0f ? -25.0f : 25.0f;
    f32 nxp = *(f32*)(arg0 + 0x2B4) * dt + tx;
    o = *(u8**)(arg0 + 0x4);
    *(f32*)(o + 0x0) = nxp;
    *(f32*)(o + 0x4) = ty;
    *(f32*)(o + 0x8) = tz;
    MarkMoved(*(u8**)(arg0 + 0x4));
    if (fabsf(*(f32*)(arg0 + 0x2CC) - tz) < 0.5f) EventSend(gEventMgr, arg0, NULL, 0xC82F907Bu, 0, 0, 1, 0);
}

// 0x8008F068
// Setup: calls the owner slot +0x54, looks up nine locators (+0x284..+0x2A4), sets the ray
// test height (+0x7E8 = 70) and the matching +0x7EC value from lbl_80347200, then configures
// the +0x35C sub-object and registers it.
void fn_8008F068(u8* arg0) {
    (*(void (**)(u8*))(*(u8**)(arg0 + 0x4) + 0x54))(arg0);
    *(Locator**)(arg0 + 0x284) = WorldFindLocator(gWorldState, 0xB0D82495u);
    *(Locator**)(arg0 + 0x288) = WorldFindLocator(gWorldState, 0x29D1752Fu);
    *(Locator**)(arg0 + 0x28C) = WorldFindLocator(gWorldState, 0x5ED645B9u);
    *(Locator**)(arg0 + 0x290) = WorldFindLocator(gWorldState, 0x2663C0F0u);
    *(Locator**)(arg0 + 0x294) = WorldFindLocator(gWorldState, 0xBF6A914Au);
    *(Locator**)(arg0 + 0x298) = WorldFindLocator(gWorldState, 0xC86DA1DCu);
    *(Locator**)(arg0 + 0x29C) = WorldFindLocator(gWorldState, 0xCF0BE857u);
    *(Locator**)(arg0 + 0x2A0) = WorldFindLocator(gWorldState, 0x5602B9EDu);
    *(Locator**)(arg0 + 0x2A4) = WorldFindLocator(gWorldState, 0x2105897Bu);
    fn_8008F1F8(arg0);
    fn_80216BF0(arg0 + 0x2A8);
    *(f32*)(arg0 + 0x7E8) = 70.0f;
    s32 r = RaySceneTest(NULL);   // the argument is ignored (returns a global)
    *(f32*)(arg0 + 0x7EC) = (r <= 3 && r >= 0) ? lbl_80347200[r] : lbl_80347200[0];
    fn_801F8F34(arg0 + 0x35C, 0.1f);
    fn_801F8FA0(arg0 + 0x35C, 0.3f);
    s8 flags[3] = {0, -1, 0};
    fn_801F8628(arg0 + 0x35C, flags, 1);
    *(s32*)(arg0 + 0x3F4) = 0;
    fn_801B3310(fn_801B3308(), arg0 + 0x35C);
}

// 0x8008F7D4
// Teardown counterpart of fn_8008F068: unregisters +0x35C, resets the three sub-records
// (+0x1F8, +0xA8, +0x180 arrays), restores the owner +0xB0 float, drops the texture at
// +0x358 and clears the state fields.
void fn_8008F7D4(u8* arg0) {
    fn_801B3378(fn_801B3308(), arg0 + 0x35C);
    for (s32 i = 0; i < 3; i++) {
        *(s32*)(arg0 + 0x98 + i * 4) = 0;
        fn_80091E44(arg0 + 0x1F8 + i * 0x14);
        fn_800923DC(arg0 + 0xA8 + i * 0x48);
        fn_80093A88(arg0 + 0x180 + i * 0x28);
    }
    u8* owner = *(u8**)(arg0 + 0x90);
    u8* r = (*(u8* (**)(u8*))(*(u8**)owner + 0x10))(owner);
    if (r != NULL) *(f32*)(r + 0xB0) = *(f32*)(arg0 + 0x94);
    *(void**)(arg0 + 0x90) = NULL;
    *(s32*)(arg0 + 0xA4) = 0;
    *(s32*)(arg0 + 0x7E0) = -1;
    *(u8*)(arg0 + 0x7E4) = 0;
    if (*(u8**)(arg0 + 0x358) != NULL) {
        UIElement_PlayUIAnim(0x7DBA57FFu, 0x3D242667u);
        ReleaseRef(*(u8**)(arg0 + 0x358));
        *(void**)(arg0 + 0x358) = NULL;
    }
    *(s32*)(arg0 + 0x7F4) = 0;
    *(s32*)(arg0 + 0x7F8) = 0;
    *(s32*)(arg0 + 0x354) = 1;
    *(f32*)(arg0 + 0x800) = 0.0f;
    *(f32*)(arg0 + 0x804) = 0.0f;
    fn_80216C60(arg0 + 0x2A8, 0);
}

// 0x800923DC
// Resets a 0x48-byte slot record: clears the monster effect slots, frees the sound
// handles (+0x24/+0x28), drops the texture (+0xC) and puts all fields back to idle.
void fn_800923DC(u8* arg0) {
    u8* mon = *(u8**)(arg0 + 0x4);
    if (mon != NULL) ClearEffectSlots(mon + 0x94);
    lbl_803C6A70[*(s32*)arg0] = NULL;
    *(s32*)(arg0 + 0x0) = -1;
    *(void**)(arg0 + 0x4) = NULL;
    *(s32*)(arg0 + 0x8) = -1;
    if (*(u8**)(arg0 + 0xC) != NULL) {
        UIElement_PlayUIAnim(*(u32*)(arg0 + 0x10), 0x3D242667u);
        ReleaseRef(*(u8**)(arg0 + 0xC));
        *(void**)(arg0 + 0xC) = NULL;
    }
    s32 h = *(s32*)(arg0 + 0x24);
    *(u32*)(arg0 + 0x10) = 0;
    *(s32*)(arg0 + 0x18) = 0;
    *(s32*)(arg0 + 0x1C) = 0;
    *(s32*)(arg0 + 0x20) = 0;
    if (h != -1) {
        fn_8020A398(h);
        *(s32*)(arg0 + 0x24) = -1;
    }
    h = *(s32*)(arg0 + 0x28);
    if (h != -1) {
        fn_8020A398(h);
        *(s32*)(arg0 + 0x28) = -1;
    }
    *(s32*)(arg0 + 0x2C) = 0;
    *(s32*)(arg0 + 0x30) = 0;
    *(u8*)(arg0 + 0x34) = 0;
    *(f32*)(arg0 + 0x38) = 0.0f;
    *(s32*)(arg0 + 0x44) = -1;
}

// 0x80092678
// Marks the slot (+0x8 = 2), notifies its monster (event 0xD7A63AA2), drops the texture and
// sound handles, and clears the monster effect slots.
void fn_80092678(u8* arg0) {
    *(s32*)(arg0 + 0x8) = 2;
    EventSend(gEventMgr, *(u8**)(arg0 + 0x4), NULL, 0xD7A63AA2u, 0, 0, 1, 0);
    if (*(u8**)(arg0 + 0xC) != NULL) {
        UIElement_PlayUIAnim(*(u32*)(arg0 + 0x10), 0x3D242667u);
        ReleaseRef(*(u8**)(arg0 + 0xC));
        *(void**)(arg0 + 0xC) = NULL;
    }
    s32 h = *(s32*)(arg0 + 0x24);
    if (h != -1) {
        fn_8020A398(h);
        *(s32*)(arg0 + 0x24) = -1;
    }
    h = *(s32*)(arg0 + 0x28);
    if (h != -1) {
        fn_8020A398(h);
        *(s32*)(arg0 + 0x28) = -1;
    }
    u8* mon = *(u8**)(arg0 + 0x4);
    *(s32*)(arg0 + 0x2C) = 0;
    *(s32*)(arg0 + 0x30) = 0;
    if (mon != NULL) ClearEffectSlots(mon + 0x94);
}

// 0x800927B0
// Starts a slot: plays its UI animation and two sounds (creating the sound manager on first
// use), stores the second sound handle at +0x24 and sets the three effect slots of the monster.
void fn_800927B0(u8* arg0) {
    UIElement_PlayUIAnim(*(u32*)(arg0 + 0x10), 0x15726E29u);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(0xF6127B57u, 0, 0, 0, 0, 0);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    u8* mon = *(u8**)(arg0 + 0x4);
    *(s32*)(arg0 + 0x24) = PlaySound(0x7E9A2u, 0, 0, 0, 0, 0);
    if (mon != NULL) {
        u8* list = mon + 0x94;
        fn_801EC8C4(list, fn_801EC880(list, 0x53B643B8u), 0x1833D84Cu);
        fn_801EC8C4(list, fn_801EC880(list, 0x37B4DA5Bu), 0x1833D84Cu);
        fn_801EC8C4(list, fn_801EC880(list, kHashProjection), 0xA80D43C2u);
    }
}

// 0x800988F0
// Updates the colour fade of the HUD element: picks the target colour from the value arg1
// (red above the high threshold, orange above the low one, yellow if positive, else the
// default grey), reads the current colour through the class table lbl_80348F38 (
// keys 0x135..0x138) and stores the difference at +0x5C..+0x68 of the owner part.
void fn_800988F0(u8* arg0, s32 arg1) {
    typedef u8* (*VGet)(u8*);
    typedef u32 (*Getter)(u8*, s32, u8*);
    if (lbl_805FB150 == 0) {
        lbl_803C6AF0[0] = 0.2f;
        lbl_803C6AF0[1] = 0.2f;
        lbl_803C6AF0[2] = 0.2f;
        lbl_803C6AF0[3] = 1.0f;
        lbl_805FB150 = 1;
    }
    f32 col[4] = {lbl_803C6AF0[0], lbl_803C6AF0[1], lbl_803C6AF0[2], lbl_803C6AF0[3]};
    s32 lvl = RaySceneTest(lbl_803C6AF0);
    if (arg1 > lbl_80347160[lvl]) {
        col[0] = 1.0f; col[1] = 0.0f; col[2] = 0.0f; col[3] = 1.0f;
    } else if (arg1 > lbl_80347150[RaySceneTest(NULL)]) {   // argument ignored
        col[0] = 1.0f; col[1] = 0.5f; col[2] = 0.0f; col[3] = 1.0f;
    } else if (arg1 > 0) {
        col[0] = 1.0f; col[1] = 1.0f; col[2] = 0.0f; col[3] = 1.0f;
    }
    u8* part = fn_801DA788((*(VGet*)(*(u8**)arg0 + 0xC))(arg0), 0);
    u8* q = *(u8**)(*(u8**)(part + 0x10) + 0x70);
    f32 cur[4];
    static const s32 keys[4] = {0x135, 0x136, 0x137, 0x138};
    for (s32 i = 0; i < 4; i++) {
        u8* slot = lbl_80348F38 + *(u8*)(q + 0xC) * 4;
        u32 bits = (*(Getter*)(slot + 0x30))(*(u8**)(q + 0x8), keys[i], slot);
        cur[i] = *(f32*)&bits;
    }
    f32 d0 = col[0] - cur[0];
    f32 d1 = col[1] - cur[1];
    f32 d2 = col[2] - cur[2];
    f32 d3 = col[3] - cur[3];
    u8* out = fn_801DA788((*(VGet*)(*(u8**)arg0 + 0xC))(arg0), 0);
    *(f32*)(out + 0x5C) = d0;
    *(f32*)(out + 0x60) = d1;
    *(f32*)(out + 0x64) = d2;
    *(f32*)(out + 0x68) = d3;
}

// 0x800995AC
// Sets the target object (+0x268) and, if non-null, caches its rotation (+0x278..+0x284), the
// angle between its up vector and the world Y axis (+0x294, scaled by 2/pi) and the rotation
// axis (+0x288..+0x290, the normalized cross product). Clears the timer +0x270.
void fn_800995AC(u8* arg0, u8* arg1) {
    *(u8**)(arg0 + 0x268) = arg1;
    if (arg1 != NULL) {
        u8* o = *(u8**)(arg0 + 0x4);
        *(f32*)(arg0 + 0x278) = *(f32*)(o + 0xC);
        *(f32*)(arg0 + 0x27C) = *(f32*)(o + 0x10);
        *(f32*)(arg0 + 0x280) = *(f32*)(o + 0x14);
        *(f32*)(arg0 + 0x284) = *(f32*)(o + 0x18);
        f32 v[3];
        fn_80097BCC(v, arg1);
        const f32 up[3] = {0.0f, 1.0f, 0.0f};
        f32 refLen2 = 0.0f * 0.0f + (0.0f * 0.0f + (1.0f * 1.0f));
        f32 dot = (0.0f * v[2]) + ((0.0f * v[0]) + (1.0f * v[1]));
        f32 refLen = 0.0f;
        if (!(refLen2 <= 0.0f)) refLen = (f32)sqrt((f64)refLen2);   // original: frsqrte, 2 Newton steps
        f32 vLen2 = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
        f32 vLen = 0.0f;
        if (!(vLen2 <= 0.0f)) vLen = (f32)sqrt((f64)vLen2);
        f32 denom = refLen * vLen;
        f32 angle = 0.0f;
        if (!(denom <= 0.0f)) {
            f32 c = dot / denom;
            if (!(c < -1.0f)) c = c > 1.0f ? 1.0f : c; else c = -1.0f;
            angle = 0.63661975f * (f32)fn_8033F1F4((f64)c);
        }
        *(f32*)(arg0 + 0x294) = angle;
        f32 cx = up[1] * v[2] - up[2] * v[1];
        f32 cy = up[2] * v[0] - up[0] * v[2];
        f32 cz = up[0] * v[1] - up[1] * v[0];
        f32 n2 = cz * cz + (cx * cx + cy * cy);
        f32 n[3] = {0.0f, 0.0f, 0.0f};
        if (n2 != 0.0f) {
            f32 inv = (f32)(1.0 / sqrt((f64)n2));   // original: frsqrte, 2 Newton steps
            n[0] = cx * inv;
            n[1] = cy * inv;
            n[2] = cz * inv;
        }
        *(f32*)(arg0 + 0x288) = n[0];
        *(f32*)(arg0 + 0x28C) = n[1];
        *(f32*)(arg0 + 0x290) = n[2];
    }
    *(f32*)(arg0 + 0x270) = 0.0f;
}

// 0x8009E9C0
// Attaches the camera target arg1 (+0x58): if the byte at +0x5C is set, clears bit 1 on the
// flags of arg1 attachment (+0x198) and its record, else sets it. Resets the six 3-float
// rows at +0x60..+0x94 (identity-like: w = 1 in rows 4 and 6) and allocates the 5-entry ring
// of 12-byte positions (+0x9C) and ids (+0xA0) filled with zeros and increasing ids.
void fn_8009E9C0(u8* arg0, u8* arg1) {
    typedef u8* (*VGet)(u8*);
    u8 enabled = *(u8*)(arg0 + 0x5C);
    *(u8**)(arg0 + 0x58) = arg1;
    u8* att = *(u8**)(arg1 + 0x198);
    if (enabled != 0) {
        if ((*(VGet*)(*(u8**)att + 0xC))(att) != NULL) {
            u8* p = (*(VGet*)(*(u8**)att + 0xC))(att);
            *(u32*)(p + 0x8) &= ~2u;
        }
        *(u32*)(*(u8**)(att + 0x4) + 0x20) &= ~2u;
    } else {
        if ((*(VGet*)(*(u8**)att + 0xC))(att) != NULL) {
            u8* p = (*(VGet*)(*(u8**)att + 0xC))(att);
            *(u32*)(p + 0x8) |= 2;
        }
        *(u32*)(*(u8**)(att + 0x4) + 0x20) |= 2;
    }
    *(f32*)(arg0 + 0x60) = 0.0f;
    *(f32*)(arg0 + 0x64) = 0.0f;
    *(f32*)(arg0 + 0x68) = 0.0f;
    *(f32*)(arg0 + 0x6C) = 0.0f;
    *(f32*)(arg0 + 0x70) = 0.0f;
    *(f32*)(arg0 + 0x74) = 0.0f;
    *(f32*)(arg0 + 0x78) = 0.0f;
    *(f32*)(arg0 + 0x7C) = 0.0f;
    *(f32*)(arg0 + 0x80) = 0.0f;
    *(f32*)(arg0 + 0x84) = 1.0f;
    *(f32*)(arg0 + 0x88) = 0.0f;
    *(f32*)(arg0 + 0x8C) = 0.0f;
    *(f32*)(arg0 + 0x90) = 0.0f;
    *(f32*)(arg0 + 0x94) = 1.0f;
    *(s32*)(arg0 + 0xA4) = 5;
    *(void**)(arg0 + 0x9C) = fn_8032F028(MemAllocSmall(5 * 0xC + 0x10), NULL, 0, 0xC, 5);
    *(void**)(arg0 + 0xA0) = MemAllocSmall(*(s32*)(arg0 + 0xA4) * 4);
    f32 zero[3] = {0.0f, 0.0f, 0.0f};
    for (s32 i = 0; i < *(s32*)(arg0 + 0xA4); i++) {
        s32 cnt = *(s32*)(arg0 + 0xA4);
        *(s32*)(arg0 + 0xAC) = 1;
        s32 head = (cnt + *(s32*)(arg0 + 0xA8) - 1) % cnt;
        *(s32*)(arg0 + 0xA8) = head;
        memcpy(*(u8**)(arg0 + 0x9C) + head * 0xC, zero, 0xC);
        *(s32*)(*(u8**)(arg0 + 0xA0) + *(s32*)(arg0 + 0xA8) * 4) = *(s32*)(arg0 + 0xB0);
        s32 id = *(s32*)(arg0 + 0xB0) + 1;
        *(s32*)(arg0 + 0xB0) = id;
        if (id == 0) *(s32*)(arg0 + 0xB0) = id + 1;
    }
}

// 0x800A4EE4
// Picks a random active object (byte +0x364 == 0) from the inner tree of the entry with key
// arg1 in the tree arg0 (node: left +0, right +4, parent +8, key +0xC, inner tree +0x10).
// Returns NULL when there is no such entry or no active object.
u8* fn_800A4EE4(u8* arg0, u32 arg1) {
    u8* hdr = arg0 + 4;
    u8* best = hdr;
    for (u8* n = *(u8**)(arg0 + 4); n != NULL;) {
        if (*(u32*)(n + 0xC) >= arg1) {
            best = n;
            n = *(u8**)(n + 0x0);
        } else {
            n = *(u8**)(n + 0x4);
        }
    }
    if (best == hdr || arg1 < *(u32*)(best + 0xC)) best = hdr;
    s32 count = 0;
    s32 key = fn_800A5164();
    u8* range[2];
    fn_800A50B4(range, best + 0x10, &key);
    for (u8* n = range[0]; n != range[1]; n = TreeNext(n)) {
        if (*(u8*)(*(u8**)(n + 0x10) + 0x364) == 0) count++;
    }
    if (count == 0) return NULL;
    u32 pick = RandomU32() % (u32)count;
    fn_800A50B4(range, best + 0x10, &key);
    s32 idx = 0;
    for (u8* n = range[0]; n != range[1]; n = TreeNext(n)) {
        u8* obj = *(u8**)(n + 0x10);
        if (*(u8*)(obj + 0x364) == 0) {
            if (idx == (s32)pick) return obj;
            idx++;
        }
    }
    return NULL;
}

// 0x800A59BC
// std::map-style node insert (as fn_800A29BC for another tree type): allocates a 0x14-byte
// node under `parent` (left if `left` else right), copies the 8-byte value, bumps the count,
// rebalances via fn_800A2AEC and optionally records it as root.
u8* fn_800A59BC(u8* tree, u8* parent, u8 left, u8 setRoot, void* value) {
    if ((u32)(*(s32*)tree + 0x10000) == 0xFFFFu) {
        fn_80336BF8(__files + 0xA0, "tree::insert length error");
        fn_8033B09C();
    }
    u8* node = (u8*)fn_801D40E0(0x14);
    if (node == NULL) {
        fn_80336BF8(__files + 0xA0, "Memory allocation failure");
        fn_8033B09C();
    }
    u8* guard = node;
    *(s32*)(node + 0xC) = ((s32*)value)[0];
    *(s32*)(node + 0x10) = ((s32*)value)[1];
    *(s32*)(node + 0x4) = 0;
    guard = NULL;
    *(s32*)(node + 0x0) = 0;
    *(u8**)(node + 0x8) = parent;
    if (left != 0) {
        *(u8**)(parent + 0x0) = node;
    } else {
        *(u8**)(parent + 0x4) = node;
    }
    s32 count = *(s32*)tree;
    *(s32*)tree = count + 1;
    fn_800A2AEC(node, *(u8**)(tree + 0x4), count);
    if (setRoot != 0) *(u8**)(tree + 0x8) = node;
    if (guard != NULL) operator_delete(guard);
    return node;
}

// 0x800A5DD4
// Constructor: base ctor, then twice (vtable lbl_8035D6D0, then lbl_8035D5C8): sets flags
// the 0x600000 flags set, clears bit 1 on the +0x8 flags of the vtable-slot-0xC object and the low two
// bits of the +0x4 record flags.
void* fn_800A5DD4(u8* arg0, s32 arg1, void* arg2, s32 arg3) {
    typedef u8* (*VGet)(u8*);
    fn_800690D4(arg0, arg1, arg2, arg3);
    *(u8*)(arg0 + 0x264) = 0;
    *(void**)arg0 = lbl_8035D6D0;
    *(u8*)(arg0 + 0x265) = 0;
    *(u32*)(arg0 + 0x15C) |= 0x600000;
    if ((*(VGet*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* p = (*(VGet*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(p + 0x8) &= ~2u;
    }
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) &= ~3u;
    *(void**)arg0 = lbl_8035D5C8;
    *(u32*)(arg0 + 0x15C) |= 0x600000;
    if ((*(VGet*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* p = (*(VGet*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(p + 0x8) &= ~2u;
    }
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) &= ~3u;
    return arg0;
}

// 0x800A6008
// Same as fn_800A5DD4 with final vtable lbl_8035D4C0.
void* fn_800A6008(u8* arg0, s32 arg1, void* arg2, s32 arg3) {
    typedef u8* (*VGet)(u8*);
    fn_800690D4(arg0, arg1, arg2, arg3);
    *(u8*)(arg0 + 0x264) = 0;
    *(void**)arg0 = lbl_8035D6D0;
    *(u8*)(arg0 + 0x265) = 0;
    *(u32*)(arg0 + 0x15C) |= 0x600000;
    if ((*(VGet*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* p = (*(VGet*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(p + 0x8) &= ~2u;
    }
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) &= ~3u;
    *(void**)arg0 = lbl_8035D4C0;
    *(u32*)(arg0 + 0x15C) |= 0x600000;
    if ((*(VGet*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* p = (*(VGet*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(p + 0x8) &= ~2u;
    }
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) &= ~3u;
    return arg0;
}

}
