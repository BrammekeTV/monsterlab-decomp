// Mixed functions: id-name record reset, quaternion axis, widget child lists, robot answer
// reveal, and assorted 0x8007xxxx - 0x8011xxxx helpers (draft_miss60).
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "revolution.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

struct StringTable {
    const char** strings;
    u8           pad[8];
    u32          count;     // 0x0C
};

extern "C++" void*        gScreenSets;     // 0x805FB654
extern "C++" StringTable* gStringTable;    // 0x805FB910

static const u32 kHashEnhancement = 0xDCAEC94Cu;  // "Enhancement"
static const u32 kHashDefect      = 0xCA64FF01u;  // "defect"
static const u32 kHashInstalled   = 0x1C6314F7u;  // "installed"
static const u32 kHashProjection  = 0x8B9BD519u;  // "Projection"

// C++ linkage, as in small_8015.cpp.
void   fn_8015A2E8(u8* p, u32 unused4, u32 flush);                     // 0x8015A2E8

extern "C" {
void*  IdPoolNext(void* pool);                                         // 0x801E6EB0
void*  MemAllocDefault(u32 size);                                      // 0x801D4650
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);         // 0x80209E70
s32    RaySceneTest(void* ray);                                        // 0x80070458
BOOL   SavePartTierOwned(u8* s, u32 part, s32 tier);
void   ScopedLockCtor(ScopedLock* l);                                  // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                       // 0x801F53B0
void*  ScreenSetFindElement(void* set, u32 hash);                      // 0x80198F18
void*  ScreenSetFindWidget(void* set, u32 hash);                       // 0x80198E34
void*  ScreenSetsCurrent(void* sets);                                  // 0x8018D1E4
void   SirFinishLoad(void* loader, void** handle);                     // 0x801E6C44
void*  SirLoad(void* loader, const char* path, void* dst, u32 size, s32 flags);  // 0x801E6138
int    SysSprintf(char* buf, const char* fmt, ...);
void   TargetListDecode(void* t, const u32* mask, s32 side);           // 0x80216240
void*  TargetListDestroy(void* t, s32 del);
void   TargetListInit(void* t);                                        // 0x802161DC
s32    TextIdFromHash(u32 hash);                                       // 0x80120D34
void   TextureRelease(void* tex);                                      // 0x80184028
u32    RandomU32(void);
void   UIElement_PlayUIAnim(u32 element, u32 anim);
void   UIImage_SetTexture(u32 a0, BOOL a1);                            // 0x80191B68
void   UIScreen_PopScreen(u32 hash);
void   UIScreen_Push(u32 hash);
s32    WorldGetPhase(void* world);                                     // 0x800D83A8
BOOL   ZoneIsLoaded(void* world, s32 zone);                            // 0x8012F408
void*  ChoiceDestroy(void* c, s32 del);                                // 0x8020F394
void   EntityRefreshLinks(void* entity);                               // 0x8012CDFC
void   SpawnEntity(void* a0);
u8**   FighterGetSlot(u8* f, s32 i);                                   // 0x8020E794
s32    PartMatchup(s32 defA, s32 defB);                                // 0x80213988
BOOL   fn_80212E28(u8** slot);
void*  CharacterFind(s32 id);                                          // 0x800E876C
void   fn_80047230(u8* out);                                           // 0x80047230 (fills the chosen-action record)
s32    fn_8004935C(void);                                              // 0x8004935C (selected part slot)
BOOL   fn_8004D170(void* chr);
void   fn_8004D760(u8* c, u8* monster);
void*  fn_8004DAF8(void* c, s32 a, s32 b);
void   fn_8007A42C(u8* obj, f32* outA, f32* outB);                     // 0x8007A42C
s32    fn_8007C5D8(u8* obj);                                           // 0x8007C5D8
f32    fn_800C7408(u8* obj, f32* pos);                                 // 0x800C7408
void   fn_800C719C(u8* entry);                                         // 0x800C719C
void   fn_800D76FC(u8* tbl, s32 idx);                                  // 0x800D76FC
void   fn_800D795C(u8* tbl, s32 idx);                                  // 0x800D795C
s32    fn_800DE780(u8* self, void* a, f32* pa, f32* pb, f32* out, f32 dt, f32 fa, f32 fb);  // 0x800DE780
void   fn_800E6684(u8* self);                                          // 0x800E6684
void   fn_800E6CD0(u8* self);                                          // 0x800E6CD0
s32    fn_80109A0C(s32 id);                                            // 0x80109A0C
void   fn_80116E54(u8* p, s32 a, s32 b);
void   fn_80116EF0(u8* p, s32 a);
void   fn_8011813C(u8* p);
void   fn_8011A04C(void);
s32    fn_8012E420(u8* world, u8* obj, s32 a, s32 b);                  // 0x8012E420
u8*    fn_8015A210(u8* p, s32 a, s32* out, s32 b);                     // 0x8015A210
void   fn_8016964C(u8* mgr, u8* tex);                                  // 0x8016964C
u8*    fn_8016A08C(const char* name);                                  // 0x8016A08C
s32 fn_8016B5E0(u8* arg0, u8* arg1, s32 arg2);                       // 0x8016B5E0
u8*    fn_8016B724(u8* obj, s32 a, const char* name);                  // 0x8016B724
void   fn_8019BE68(u8* parent, u8* w);                                 // 0x8019BE68
u8*    fn_80198E20(u8* set);                                           // 0x80198E20
BOOL   fn_80198C34(void* set, void* w);                                // 0x80198C34
ScriptValue fn_801A1208(u8* el);                                       // 0x801A1208
s32    fn_801A3010(void* obj, void* data);
void   fn_801B4EA4(u8* obj, s32 flag);                                 // 0x801B4EA4
s32    fn_801D0C04(u8* obj, f32 dt);                                   // 0x801D0C04
void*  fn_801D40E0(s32 n);                                             // 0x801D40E0
u8*    fn_801DB2E4(u8* p, u32 key);                                    // 0x801DB2E4
void*  fn_801DB194(u8* tmpl);                                          // 0x801DB194
s32    fn_801E6BD0(void* pool, u32* key);                              // 0x801E6BD0
u32    fn_801EC880(u8* p, u32 hash);                                   // 0x801EC880
void   fn_801EC8C4(u8* p, u32 a, u32 b);                               // 0x801EC8C4
void*  fn_80209814(void* v);                                           // 0x80209814
void   fn_80218588(u8* lab, s32 id);
void   fn_802186D0(u8* lab, s32 id);
u8     fn_8021EE94(u8* p);
void   fn_8022E0B0(u8* p, u32 id);
}

extern u8*          gBattle;
extern u8           gGame[];
extern u8           gIdPool[];                // 0x8048A8F0
extern void*        gLab;                     // 0x805FBBC8
extern StringTable* gStringTable2;            // 0x805FB90C
extern u8           gWorldState[];            // 0x803E6D00
extern f32          lbl_803473A0[];
extern f32          lbl_803473B0[];
extern f32          lbl_803473C0[];
extern const char   lbl_80354800[];           // "Not found"
extern const char   lbl_8035480C[];           // "NOT FOUND"
extern const char   lbl_8036C8DC[];           // 0x8036C8DC (name for the ".sir" format)
extern s32          lbl_80347DF8[];           // 0x80347DF8 (five words, default 3)
extern s32          lbl_803C4E28[];
extern u8*          lbl_803C6A70[3];
extern u8           lbl_803C75C8[];           // 0x803C75C8 (0x7D0 records of 0x40 bytes)
extern char         lbl_803C7130[];           // 0x803C7130 (asset name buffer)
extern u8           lbl_803E6B28[];
extern u8           lbl_803E6B34[];
extern const char   lbl_805F8200[];           // empty string
extern const char   lbl_805F8EB8[];           // ".sir" file name format
extern const char   lbl_805F8EDC[];           // id-name buffer format
extern s32          lbl_805FB034;
extern s32          lbl_805FB038;
extern u8*          lbl_805FB220;             // texture load buffer
extern u8           lbl_805FB228;
extern s32          lbl_805FB22C;
extern s32          lbl_805FB234;
extern u8*          lbl_805FB298;
extern u8*          lbl_805FB29C;
extern u8*          lbl_805FB2A0;
extern u8*          lbl_805FB5A8;             // texture manager
extern void*        lbl_805FBBAC;             // voice SFX handle
extern u8*          lbl_805FBBE0;             // save block pointer

// Creates the voice SFX handle lbl_805FBBAC once (see small_801Dc.cpp).
static void EnsureVoiceSfxHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

extern "C" {

// 0x801098DC
// Resets slot `id` (1..0x7D0) of the id-name record table: frees a pending load if the slot's
// handle is stale, then clears the record. Always returns 0.
s32 fn_801098DC(s32 id) {
    if (id != 0 && (u32)id <= 0x7D0u) {
        u8* rec = lbl_803C75C8 + (id << 6);
        if (*(void**)rec == IdPoolNext(gIdPool)) {
            *(void**)rec = IdPoolNext(gIdPool);
            *(s32*)(rec + 0x4) = 0;
            *(s32*)(rec + 0x38) = 0;
            SysSprintf((char*)(rec + 8), lbl_805F8EDC);
            *(s32*)(rec + 0x3C) = 0;
            lbl_805FB234 = lbl_805FB234 - 1;
        } else {
            u32 key = *(u32*)rec;
            if (fn_801E6BD0(gIdPool, &key) == 0) {
                void* handle = *(void**)rec;
                SirFinishLoad(gIdPool, &handle);
            }
            *(void**)rec = IdPoolNext(gIdPool);
            *(s32*)(rec + 0x4) = 0;
            *(s32*)(rec + 0x38) = 0;
            SysSprintf((char*)(rec + 8), lbl_805F8EDC);
            *(s32*)(rec + 0x3C) = 0;
            lbl_805FB234 = lbl_805FB234 - 1;
            lbl_805FB228 = 0;
        }
    }
    return 0;
}

// 0x801455E0
// Writes three components of a rotation-matrix row/column from quaternion q (x,y,z,w) to out.
void fn_801455E0(f32* out, u8* q) {
    f32 x = *(f32*)(q + 0x0);
    f32 y = *(f32*)(q + 0x4);
    f32 z = *(f32*)(q + 0x8);
    f32 w = *(f32*)(q + 0xC);
    f32 x2 = 2.0f * x;
    f32 y2 = 2.0f * y;
    f32 z2 = 2.0f * z;
    out[1] = z2 * y - x2 * w;
    out[0] = z2 * x + y2 * w;
    out[2] = 1.0f - (x2 * x + y2 * y);
}

// 0x8019CAD4
// Attaches widget `w` as the last child of `parent` (or of the current screen set's root when
// parent is NULL), detaching it from its old parent first, then calls vtable slot 0x2C.
void fn_8019CAD4(u8* w, u8* parent) {
    u8* oldParent = *(u8**)(w + 0x128);
    if (oldParent != NULL) fn_8019BE68(oldParent, w);
    if (parent == NULL) {
        parent = fn_80198E20((u8*)ScreenSetsCurrent(gScreenSets));
    }
    u8* child = *(u8**)(parent + 0x120);
    if (child != NULL) {
        u8* next;
        while ((next = *(u8**)(child + 0x124)) != NULL) child = next;
        *(u8**)(child + 0x124) = w;
    } else {
        *(u8**)(parent + 0x120) = w;
    }
    *(u8**)(w + 0x128) = parent;
    *(u8**)(w + 0x124) = NULL;
    (*(void (**)(u8*, s32))(*(u8**)(w + 0x138) + 0x2C))(w, 0x28);
    *(u8**)(w + 0x128) = parent;
}

// 0x80092FD4
// Reveals the robot evolution answer: finds the entry whose +4 matches this element's handle,
// plays the show-answer UI animation and sounds, and resets three animation channels.
void RobotEvolutionExperiment_RobotShowAnswer(u8* self) {
    u8* entry = NULL;
    for (s32 i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A70[i] + 4) == *(u32*)(self + 0x10)) {
            entry = lbl_803C6A70[i];
            break;
        }
    }
    if (entry == NULL) return;
    UIElement_PlayUIAnim(*(u32*)(entry + 0x10), 0x15726E29u);
    EnsureVoiceSfxHandle();
    PlaySound(0xF6127B57u, 0, 0, 0, 0, 0);
    EnsureVoiceSfxHandle();
    *(s32*)(entry + 0x24) = PlaySound(0x7E9A2u, 0, 0, 0, 0, 0);
    u8* obj = *(u8**)(entry + 0x4);
    if (obj != NULL) {
        u8* list = obj + 0x94;
        fn_801EC8C4(list, fn_801EC880(list, 0x53B643B8u), 0x1833D84Cu);
        fn_801EC8C4(list, fn_801EC880(list, 0x37B4DA5Bu), 0x1833D84Cu);
        fn_801EC8C4(list, fn_801EC880(list, kHashProjection), 0xA80D43C2u);
    }
}

// 0x8007A68C
// Copies a rectangle of a scaled 4-bit bitmap into a packed 4-bit mask: for each cell, the source
// nibble's bit 1 (or bit 1 derived from bit 0 for odd columns) is OR-ed into the destination, and
// a source nibble with bit 0 set updates *flag (set to 1) and *state (0, or 2 when it was already
// non-zero and the destination nibble has bit 2).
void fn_8007A68C(u8* self, u8* rect, u8* flag, s32* state) {
    f32 sx, sy;
    s32 srcStride, dstStride;
    u8* srcObj = *(u8**)(self + 4);
    u8* srcImg = *(u8**)(srcObj + 4) + 4;
    u8* dstImg = *(u8**)(*(u8**)self + 0x24) + 4;
    fn_8007A42C(srcObj, &sx, &sy);
    u8* src = fn_8015A210(srcImg, 0, &srcStride, 1);
    u8* dst = fn_8015A210(dstImg, 0, &dstStride, 0);
    s32 rx = *(s32*)(rect + 0x0);
    s32 ry = *(s32*)(rect + 0x4);
    s32 dx = *(s32*)(rect + 0x8);
    s32 dy = *(s32*)(rect + 0xC);
    s32 w = *(s32*)(rect + 0x10);
    s32 h = *(s32*)(rect + 0x14);
    for (s32 y = 0; y < h; y++) {
        u8* srcRow = src + srcStride * (s32)(sy * (f32)(ry + y)) + ((s32)(sx * (f32)rx) >> 1);
        u8* dstRow = dst + dstStride * (dy + y) + (dx >> 1);
        for (s32 x = 0; x < w; x++) {
            s32 odd = ((x + dx) & 1) ^ 1;
            u8 sb = srcRow[(s32)(sx * (f32)(x + rx)) >> 1];
            s32 sn = sb & 0xF;
            s32 bits = sb & 2;
            if (odd != 0) bits = (sn * 0x10) & 0x20;
            s32 i = x >> 1;
            dstRow[i] |= (u8)bits;
            if (sn & 1) {
                u8 db = dstRow[i];
                s32 dn = db & 0xF;
                if (odd != 0) dn = (db >> 4) & 0xF;
                if (dn & 1) {
                    *state = 0;
                    *flag = 1;
                } else if ((dn & 4) && *state != 0) {
                    *state = 2;
                    *flag = 1;
                } else {
                    *flag = 1;
                }
            }
        }
    }
    fn_8015A2E8(srcImg, 0, 0);
    fn_8015A2E8(dstImg, 0, 1);
}

// 0x8007AC80
// Offsets rect corners A,B by the object's position, clamps them to the bounds C (max of the
// lower) / D (min of the upper) given limits E / F, and returns 1 (with the offset removed and
// the corners rewritten as A = (-B.x, ..), B = (-A.x, ..)) when the clipped rect is non-empty.
s32 fn_8007AC80(u8* self, f32* a, f32* b, f32* c, f32* d, f32* e, f32* f) {
    u8* sub = *(u8**)(*(u8**)(*(u8**)(self + 4)) + 4);
    f32 px = *(f32*)(sub + 0x0);
    f32 pz = *(f32*)(sub + 0x8);
    fn_8007C5D8(*(u8**)self);
    a[0] = a[0] + px;
    b[0] = b[0] + px;
    a[1] = a[1] + pz;
    b[1] = b[1] + pz;
    f32 v;
    v = c[0]; if (!(v > e[0])) v = e[0]; c[0] = v;
    v = d[0]; if (!(v < f[0])) v = f[0]; d[0] = v;
    v = a[0]; if (!(v > c[0])) v = c[0]; c[0] = v; a[0] = v;
    v = b[0]; if (!(v < d[0])) v = d[0]; d[0] = v; b[0] = v;
    v = a[1]; if (!(v > c[1])) v = c[1]; c[1] = v; a[1] = v;
    v = b[1]; if (!(v < d[1])) v = d[1]; d[1] = v; b[1] = v;
    if (!(d[0] - c[0] > 0.0f) || !(d[1] - c[1] > 0.0f)) return 0;
    a[0] = a[0] - px;
    b[0] = b[0] - px;
    a[1] = a[1] - pz;
    b[1] = b[1] - pz;
    f32 nbx = -b[0];
    f32 nax = -a[0];
    a[0] = nbx;
    b[0] = nax;
    return 1;
}

// Returns the entry (0x14 bytes each, state word at +0x10 in {2,3,5}) closest to pos according to
// fn_800C7408, or NULL when no entry is in an active state.
static u8* NearestActiveEntry(u8* entries, s32 count, f32* pos) {
    u8* best = NULL;
    f32 bestDist = 0.0f;
    u8* e = entries;
    for (s32 i = 0; i < count; i++, e += 0x14) {
        s32 st = *(s32*)(e + 0x10);
        if (st == 2 || st == 3 || st == 5) {
            f32 p[3];
            p[0] = pos[0];
            p[1] = pos[1];
            p[2] = pos[2];
            if (best == NULL) {
                best = e;
                bestDist = fn_800C7408(e, p);
            } else {
                f32 d = fn_800C7408(e, p);
                if (d < bestDist) {
                    best = e;
                    bestDist = d;
                }
            }
        }
    }
    return best;
}

// 0x800C7A70
// Nearest active entry of the second table (0x7E4, count at +0x1788) to pos.
u8* fn_800C7A70(u8* self, f32* pos) {
    return NearestActiveEntry(self + 0x7E4, *(s32*)(self + 0x1788), pos);
}

// 0x800C7B84
// Nearest active entry of the first table (0x14, count at +0x1784) to pos.
u8* fn_800C7B84(u8* self, f32* pos) {
    return NearestActiveEntry(self + 0x14, *(s32*)(self + 0x1784), pos);
}

// Ring-buffer slot check used by fn_800C7C98: is `idx` inside the occupied span [S+8, S+0xC]
// (wrapping at the capacity S+0x10)?
static BOOL RingIndexValid(u8* s, s32 idx) {
    s32 cap = *(s32*)(s + 0x10);
    if (idx < 0 || !(idx < cap)) return FALSE;
    s32 lo = *(s32*)(s + 0x8);
    s32 hi = *(s32*)(s + 0xC);
    s32 x = idx;
    if (hi < lo) {
        if (idx <= hi) x = idx + cap;
        hi += cap;
    }
    return lo <= x && x <= hi;
}

// 0x800C7C98
// Walks the ring buffer of object pointers in the block at +0x2F04 (+0x64) from its newest slot
// backwards to the oldest, and returns the object whose position (obj->+4) is nearest to pos.
u8* fn_800C7C98(u8* self, f32* pos) {
    u8* best = NULL;
    f32 bestDist = 0.0f;
    u8* s = *(u8**)(self + 0x2F04) + 0x64;
    if (s == NULL) return best;
    u8* item = NULL;
    if (*(s32*)(s + 0x8) >= 0 && *(s32*)(s + 0xC) >= 0) {
        s32 idx = *(s32*)(s + 0xC);
        u8** slot = RingIndexValid(s, idx) ? *(u8***)(s + 0x14) + idx : NULL;
        *(u8***)(s + 0x18) = slot;
        item = slot != NULL ? *slot : NULL;
    }
    while (item != NULL) {
        u8* p = *(u8**)(item + 4);
        f32 dx = pos[0] - *(f32*)(p + 0);
        f32 dy = pos[1] - *(f32*)(p + 4);
        f32 dz = pos[2] - *(f32*)(p + 8);
        f32 d = dz * dz + (dx * dx + dy * dy);
        if (best == NULL) {
            best = item;
            bestDist = d;
        } else if (d < bestDist) {
            best = item;
            bestDist = d;
        }
        u8** cur = *(u8***)(s + 0x18);
        if (cur == NULL) {
            item = NULL;
        } else {
            u8** base = *(u8***)(s + 0x14);
            s32 pos2 = (s32)(cur - base);
            u8** slot;
            if (pos2 == *(s32*)(s + 0x8)) {
                slot = NULL;
            } else {
                s32 prev = pos2 - 1;
                if (prev < 0) prev = *(s32*)(s + 0x10) - 1;
                slot = RingIndexValid(s, prev) ? base + prev : NULL;
            }
            *(u8***)(s + 0x18) = slot;
            item = slot != NULL ? *slot : NULL;
        }
    }
    return best;
}

// 0x800C8334
// If the squared distance from entry `target` to `pos` is within the limit table value
// (lbl_803473A0[RaySceneTest()]), adds the matching bonus from lbl_803473C0 (scaled by
// lbl_803473B0 when the distance is almost zero) to the float at +0xC, calls fn_800C719C on
// the entry and returns 1; otherwise 0.
u32 fn_800C8334(u8* self, u8* target, f32* pos) {
    if (target == NULL) return 0;
    f32 p[3];
    p[0] = pos[0];
    p[1] = pos[1];
    p[2] = pos[2];
    f32 dist = fn_800C7408(target, p);
    if (dist <= lbl_803473A0[RaySceneTest(NULL)]) {
        if (dist > -0.0001f && dist < 0.0001f) {
            f32 scale = lbl_803473B0[RaySceneTest(NULL)];
            *(f32*)(self + 0xC) = *(f32*)(self + 0xC) + lbl_803473C0[RaySceneTest(NULL)] * scale;
        } else {
            *(f32*)(self + 0xC) = *(f32*)(self + 0xC) + lbl_803473C0[RaySceneTest(NULL)];
        }
        fn_800C719C(target);
        return 1;
    }
    return 0;
}

// 0x800D6E90
// Per-frame tick of the timed entries (0x28 bytes each, count at +0x320) while the world phase
// is 2 and the current character is not in the state tested by fn_8004D170: active entries
// (flag byte +0x20 set, word +0x24 zero) count down the float at +0x14 and, on reaching zero,
// are handled by fn_800D795C (when +0x1C is -1 and +0x18 is not) or fn_800D76FC.
void fn_800D6E90(u8* self, f32 dt) {
    if (fn_8004D170(*(void**)(gGame + 0xD614))) return;
    if (WorldGetPhase(gGame + 0xD5F0) != 2) return;
    u8* e = self;
    for (s32 i = 0; i < *(s32*)(self + 0x320); i++, e += 0x28) {
        if (*(u8*)(e + 0x20) == 0) continue;
        if (*(s32*)(e + 0x24) != 0) continue;
        *(f32*)(e + 0x14) = *(f32*)(e + 0x14) - dt;
        if (*(f32*)(e + 0x14) <= 0.0f) {
            if (*(s32*)(e + 0x1C) == -1 && *(s32*)(e + 0x18) != -1) {
                fn_800D795C(self, i);
            } else {
                fn_800D76FC(self, i);
            }
        }
    }
}

// 0x800D8D00
// Builds the segment table (0x1C bytes each) for a set of paths: total segment count (sum of
// point count - 1) at +0xC, table at +8, total length at +0x20. Each segment holds the start
// position (first segment from the point's +0xC, later ones start at previous start + delta),
// the delta vector to the next point (+0xC..+0x14) and its length at +0x18.
void fn_800D8D00(u8* self, u8** paths, u32 count) {
    *(s32*)(self + 0xC) = 0;
    for (u32 i = 0; i < count; i++) {
        *(s32*)(self + 0xC) = *(s32*)(self + 0xC) + *(s32*)(paths[i] + 0x24) - 1;
    }
    *(void**)(self + 8) = MemAllocDefault(*(s32*)(self + 0xC) * 0x1C);
    s32 seg = 0;
    *(f32*)(self + 0x20) = 0.0f;
    for (u32 i = 0; i < count; i++) {
        s32 npts = *(s32*)(paths[i] + 0x24) - 1;
        for (s32 k = 0; k < npts; k++, seg++) {
            u8* pts = *(u8**)paths[i] + k * 0x28;
            u8* out = *(u8**)(self + 8) + seg * 0x1C;
            if (seg == 0) {
                *(f32*)(out + 0x0) = *(f32*)(pts + 0xC);
                *(f32*)(out + 0x4) = *(f32*)(pts + 0x10);
                *(f32*)(out + 0x8) = *(f32*)(pts + 0x14);
            } else {
                *(f32*)(out + 0x0) = *(f32*)(out - 0x1C) + *(f32*)(out - 0x10);
                *(f32*)(out + 0x4) = *(f32*)(out - 0x18) + *(f32*)(out - 0xC);
                *(f32*)(out + 0x8) = *(f32*)(out - 0x14) + *(f32*)(out - 0x8);
            }
            f32 dx = *(f32*)(pts + 0x34) - *(f32*)(pts + 0xC);
            f32 dy = *(f32*)(pts + 0x38) - *(f32*)(pts + 0x10);
            f32 dz = *(f32*)(pts + 0x3C) - *(f32*)(pts + 0x14);
            *(f32*)(out + 0xC) = dx;
            *(f32*)(out + 0x10) = dy;
            f32 len2 = dz * dz + (dx * dx + dy * dy);
            *(f32*)(out + 0x14) = dz;
            f32 len = 0.0f;
            if (!(len2 <= 0.0f)) len = sqrtf(len2);  // original: frsqrte with two Newton steps
            *(f32*)(out + 0x18) = len;
            *(f32*)(self + 0x20) = *(f32*)(self + 0x20) + *(f32*)(out + 0x18);
        }
    }
}

// 0x800DE678
// Tests every segment (a byte pair of node indices at +0xC, count at +8, nodes of 0x14 bytes at
// +4) with fn_800DE780 and keeps the hit with the smallest distance (float at +4 of the 5-float
// result block `best`, preset to 9999). Returns 1 if any segment hit.
s32 fn_800DE678(u8* self, void* arg1, f32* best, f32 dt) {
    f32 hit[5];
    s32 found = 0;
    best[1] = 9999.0f;
    for (s32 i = 0; i < *(s32*)(self + 0x8); i++) {
        u8* nodes = *(u8**)(self + 0x4);
        u8* idx = *(u8**)(self + 0xC) + i * 2;
        u8* nodeA = nodes + idx[0] * 0x14;
        u8* nodeB = nodes + idx[1] * 0x14;
        if (fn_800DE780(self, arg1, (f32*)(nodeA + 4), (f32*)(nodeB + 4), hit, dt,
                        *(f32*)(nodeA + 0x10), *(f32*)(nodeB + 0x10)) != 0 &&
            hit[1] < best[1]) {
            found = 1;
            best[0] = hit[0];
            best[1] = hit[1];
            best[2] = hit[2];
            best[3] = hit[3];
            best[4] = hit[4];
        }
    }
    return found;
}

// Clears an id-name record: fresh pool marker, zeroed fields, name buffer reset.
static void ResetIdRecord(u8* rec) {
    *(void**)rec = IdPoolNext(gIdPool);
    *(s32*)(rec + 0x4) = 0;
    *(s32*)(rec + 0x38) = 0;
    SysSprintf((char*)(rec + 8), lbl_805F8EDC);
    *(s32*)(rec + 0x3C) = 0;
}

// Moves the current id-record index forward, wrapping 0x7D0 back to 1.
static void AdvanceIdRecord(void) {
    u32 next = (u32)lbl_805FB22C + 1;
    lbl_805FB22C = (s32)next;
    if (next >= 0x7D0u) lbl_805FB22C = 1;
}

// 0x80109600
// Polls the texture load queue for the request held in *slot. Looks for the next pending record
// (skipping idle ones), starts its load (or takes an already cached texture, storing it in *slot
// and returning 1), and when *slot holds this record's id and its load has finished, builds the
// texture, stores it in *slot and returns 1. Returns 0 while still waiting.
s32 fn_80109600(void* slot) {
    u8* rec = NULL;
    if (lbl_805FB234 != 0) {
        for (;;) {
            rec = lbl_803C75C8 + ((u32)lbl_805FB22C << 6);
            BOOL idle = FALSE;
            if (*(s32*)(rec + 0x38) == 0) {
                if (*(void**)rec == IdPoolNext(gIdPool)) idle = TRUE;
            }
            if (!idle) break;
            AdvanceIdRecord();
        }
        if (lbl_805FB228 == 0) {
            u8* tex = fn_8016A08C((const char*)(rec + 8));
            if (tex != NULL) {
                *(s32*)(tex + 0x24) = *(s32*)(tex + 0x24) + 1;
                *(u8**)slot = tex;
                ResetIdRecord(lbl_803C75C8 + ((u32)lbl_805FB22C << 6));
                AdvanceIdRecord();
                lbl_805FB228 = 0;
                lbl_805FB234 = lbl_805FB234 - 1;
                return 1;
            }
            u8* buf = lbl_805FB220;
            s32 mode = *(s32*)(lbl_803C75C8 + ((u32)lbl_805FB22C << 6) + 0x38);
            char path[0x40];
            SysSprintf(path, lbl_805F8EB8, lbl_8036C8DC);
            void* handle = SirLoad(gIdPool, path, buf, 0xC0000, mode);
            lbl_805FB228 = 1;
            *(void**)(lbl_803C75C8 + ((u32)lbl_805FB22C << 6)) = handle;
        }
    }
    uintptr_t id = (uintptr_t)*(void**)slot;
    if (id > 0x7D0) return 1;
    if ((u32)lbl_805FB22C != (u32)id) return 0;
    u32 key = *(u32*)(lbl_803C75C8 + ((u32)lbl_805FB22C << 6));
    if (fn_801E6BD0(gIdPool, &key) == 0) return 0;
    u8 obj[12];
    u8* r = lbl_803C75C8 + ((u32)lbl_805FB22C << 6);
    fn_8016B5E0(obj, lbl_805FB220, *(s32*)(r + 4));
    u8* tex = fn_8016B724(obj, 0, (const char*)(r + 8));
    *(u8**)slot = tex;
    if (tex != NULL) fn_8016964C(lbl_805FB5A8, tex);
    ResetIdRecord(lbl_803C75C8 + ((u32)lbl_805FB22C << 6));
    AdvanceIdRecord();
    lbl_805FB228 = 0;
    lbl_805FB234 = lbl_805FB234 - 1;
    return 1;
}

// 0x800E6468
// Per-frame update of the on-screen texture/photo widget 0x353388F2: pops it when nothing is
// queued (count at lbl_805FBBE0+0x8E8 <= 0 and +0x11 clear, releasing the held texture), else
// pushes it, advances the timer at +0x14, plays the 0x9C543022 transition animation or calls
// fn_800E6CD0 once the timer passes 1.0 and swaps in a freshly loaded texture from fn_80109600.
void fn_800E6468(u8* self, f32 dt) {
    fn_800E6684(self);
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    void* widget = ScreenSetFindWidget(set, 0x353388F2u);
    s32 queued = *(s32*)(lbl_805FBBE0 + 0x8E8);
    if (queued <= 0 && *(u8*)(self + 0x11) == 0) {
        if (widget != NULL && fn_80198C34(set, widget) != 0 && UIElement_IsUIAnimPlaying(0x24542991u) == 0) {
            u8* tex = *(u8**)self;
            if (tex != NULL) {
                s32 rc = *(s32*)(tex + 0x24) - 1;
                *(s32*)(tex + 0x24) = rc;
                if (rc == 0) TextureRelease(tex);
                *(u8**)self = NULL;
            }
            UIScreen_PopScreen(0x353388F2u);
        }
    } else if (widget != NULL && queued > 0) {
        BOOL shown = fn_80198C34(set, widget);
        if (shown == 0) {
            UIScreen_Push(0x353388F2u);
            shown = fn_80198C34(set, widget);
        }
        if (shown != 0 && UIElement_IsUIAnimPlaying(0x24542991u) == 0) {
            f32 t = *(f32*)(self + 0x14) + dt;
            *(f32*)(self + 0x14) = t;
            if (t > 1.0f) {
                if (*(u8*)(self + 0x11) != 0) {
                    UIElement_PlayUIAnim(0x24542991u, 0x9C543022u);
                    *(u8*)(self + 0x11) = 0;
                    *(f32*)(self + 0x14) = 0.0f;
                    fn_8022E0B0(lbl_805FBBE0, *(u32*)(self + 0x24));
                } else {
                    fn_800E6CD0(self);
                }
            }
            if (*(u8**)(self + 4) != NULL && fn_80109A0C(*(s32*)(self + 4)) == 0 && *(u8*)(self + 0x11) != 0) {
                *(f32*)(self + 0x14) = 0.0f;
                if (fn_80109600(self + 4) != 0) {
                    u8* tex = *(u8**)(self + 4);
                    *(u8**)self = tex;
                    *(u8**)(self + 4) = NULL;
                    UIImage_SetTexture(0x5881687Au, (BOOL)(intptr_t)tex);
                    UIElement_PlayUIAnim(0x24542991u, 0x5C877396u);
                }
            }
        }
    }
}

struct TargetList5 { s32 limbs[5]; s32 count; };

// 0x800F4564
// Picks the matchup result for the action chosen by fn_80047230 against the opponent's parts:
// returns 2 when the action type is 1 or no targets decode, 3 when the targets give mixed
// matchup results (a value other than 3 that differs from the first), else the first result
// (0..2 per target from PartMatchup, 3 when unset).
s32 fn_800F4564(void) {
    u8 choice[12];
    fn_80047230(choice);
    u8* db = gGameDb->data;
    s32 actionIdx = *(s32*)choice;
    u8* action = *(u8**)(db + 0x430) + actionIdx * 0x14;
    u8** fighters = (u8**)(gBattle + 0xA00);
    u8* mine = fighters[0];
    u8* opp = fighters[0];
    if (opp == mine) opp = fighters[1];
    u8* atype = *(u8**)(db + 0x8) + *(s32*)(action + 4) * 0x24;
    u8* myPart = *(u8**)(((u8**)mine)[fn_8004935C()]);
    if (*(s32*)(atype + 8) == 1) {
        ChoiceDestroy(choice, -1);
        return 2;
    }
    TargetList5 list;
    TargetListInit(&list);
    u32 mask = *(u32*)(action + 8);
    TargetListDecode(&list, &mask, fn_8004935C());
    s32 count = list.count;
    if (count == 0) {
        TargetListDestroy(&list, -1);
        ChoiceDestroy(choice, -1);
        return 2;
    }
    s32 result[5];
    for (s32 i = 0; i < 5; i++) result[i] = lbl_80347DF8[i];
    for (s32 k = 0; k < count; k++) {
        s32 slot = list.limbs[k];
        if (fn_80212E28(FighterGetSlot(opp, slot))) slot = 4;
        s32 m = PartMatchup(*(s32*)myPart, ***(s32***)(((u8**)opp) + slot));
        if (m < 3 && m >= 0) result[k] = m;
    }
    s32 first = result[0];
    for (s32 k = 1; k < count; k++) {
        if (result[k] != 3 && result[k] != first) {
            TargetListDestroy(&list, -1);
            ChoiceDestroy(choice, -1);
            return 3;
        }
    }
    TargetListDestroy(&list, -1);
    ChoiceDestroy(choice, -1);
    return result[0];
}

// 0x8010A76C
// Random jitter of the entries (0x28 bytes each, count at +0x24) while flag 2 of +0x30 is set:
// counts down the timer at +0x44 (dt clamped to 0..1/30 s); when it expires picks a random
// entry with a zero state byte (+0x24, up to 5 tries) and a new timer of 3..10 s. The picked
// entry's vector at +0x18 is offset by random amounts in [20, 100] * scale (+0x3C) for the
// duration of fn_801D0C04 and then restored. Sets +0x48 to 1.
void fn_8010A76C(u8* self, f32 dt) {
    if ((*(s32*)(self + 0x30) & 2) == 0) return;
    f32 d = 0.0f;
    if (!(dt < 0.0f)) {
        d = 0.0333333f;
        if (dt <= d) d = dt;
    }
    s32 pick = -1;
    f32 saved[3];
    saved[0] = 0.0f;
    saved[1] = 0.0f;
    saved[2] = 0.0f;
    f32 timer = *(f32*)(self + 0x44) - d;
    *(f32*)(self + 0x44) = timer;
    if (timer <= 0.0f) {
        s32 tries = 5;
        s32 i;
        do {
            tries--;
            i = rand() % *(s32*)(self + 0x24);
        } while (*(u8*)(*(u8**)self + i * 0x28 + 0x24) != 0 && tries > 0);
        if (tries > 0) pick = i;
        *(f32*)(self + 0x44) = 3.0f + 7.0f * (2.3283064e-10f * (f32)RandomU32());
    }
    if (pick >= 0) {
        f32 scale = *(f32*)(self + 0x3C);
        f32 hi = 100.0f * scale;
        f32 lo = 20.0f * scale;
        u8* e = *(u8**)self + pick * 0x28;
        saved[0] = *(f32*)(e + 0x18);
        saved[1] = *(f32*)(e + 0x1C);
        saved[2] = *(f32*)(e + 0x20);
        f32 ox = lo + (hi - lo) * (2.3283064e-10f * (f32)RandomU32());
        f32 oy = lo + (hi - lo) * (2.3283064e-10f * (f32)RandomU32());
        e = *(u8**)self + pick * 0x28;
        f32 r3 = (f32)RandomU32();
        *(f32*)(e + 0x18) = *(f32*)(e + 0x18) + ox;
        *(f32*)(e + 0x1C) = *(f32*)(e + 0x1C) + oy;
        f32 oz = lo + (hi - lo) * (2.3283064e-10f * r3);
        *(f32*)(e + 0x20) = *(f32*)(e + 0x20) + oz;
    }
    fn_801D0C04(self, d);
    if (pick >= 0) {
        u8* e = *(u8**)self + pick * 0x28;
        *(f32*)(e + 0x18) = saved[0];
        *(f32*)(e + 0x1C) = saved[1];
        *(f32*)(e + 0x20) = saved[2];
    }
    *(u8*)(self + 0x48) = 1;
}

// Entry `idx` (0x1C bytes) of the ring buffer at self+0xAF8, or NULL when the index is not in
// the occupied span.
static u8* RingEntry(u8* self, s32 idx) {
    if (!RingIndexValid(self + 0xAF8, idx)) return NULL;
    return *(u8**)(self + 0xB0C) + idx * 0x1C;
}

// 0x8010BFB8
// Advances a timeline by dt over a ring of keyed segments (duration float at +0x18 of each
// 0x1C-byte entry): the elapsed time at +0 accumulates; when it passes the current segment's
// duration the cursor at +4 moves on (wrapping at 100) and the remainder carries into the next
// segment. Stops at the last (newest) segment, clamping the time to its duration.
void fn_8010BFB8(u8* self, f32 dt) {
    while (dt > 0.0f) {
        u8* cur = RingEntry(self, *(s32*)(self + 4));
        f32 t = *(f32*)self + dt;
        *(f32*)self = t;
        if (t < *(f32*)(cur + 0x18)) {
            dt = 0.0f;
        } else {
            u8* last = RingEntry(self, *(s32*)(self + 0xB04));
            u8* now = RingEntry(self, *(s32*)(self + 4));
            if (last == now) {
                dt = 0.0f;
                *(f32*)self = *(f32*)(cur + 0x18);
            } else {
                s32 next = *(s32*)(self + 4) + 1;
                dt = *(f32*)self - *(f32*)(cur + 0x18);
                *(f32*)self = 0.0f;
                *(s32*)(self + 4) = next;
                if (next >= 0x64) *(s32*)(self + 4) = 0;
            }
        }
    }
}

// 0x8010C194
// Elapsed time of the timeline: sums the durations of the ring entries from the oldest up to
// (not including) the cursor entry, plus the time within the current entry at +0.
f32 fn_8010C194(u8* self) {
    f32 sum = 0.0f;
    u8* e = RingEntry(self, *(s32*)(self + 0xB00));
    s32 cursor = *(s32*)(self + 4);
    while (e != RingEntry(self, cursor)) {
        u8* data = *(u8**)(self + 0xB0C);
        sum += *(f32*)(e + 0x18);
        s32 idx = (s32)((e - data) / 0x1C);
        if (idx == *(s32*)(self + 0xB04)) {
            e = NULL;
        } else {
            s32 next = (idx + 1) % *(s32*)(self + 0xB08);
            e = RingIndexValid(self + 0xAF8, next) ? data + next * 0x1C : NULL;
        }
    }
    return sum + *(f32*)self;
}

// Entry following `e` in the ring buffer at self+0xAF8, or NULL after the newest entry.
static u8* RingNext(u8* self, u8* e) {
    u8* data = *(u8**)(self + 0xB0C);
    s32 idx = (s32)((e - data) / 0x1C);
    if (idx == *(s32*)(self + 0xB04)) return NULL;
    s32 next = (idx + 1) % *(s32*)(self + 0xB08);
    return RingIndexValid(self + 0xAF8, next) ? data + next * 0x1C : NULL;
}

// 0x8010C354
// Samples the timeline at time t: skips whole segments (duration at +0x18) from the oldest, then
// writes position + delta * (t / duration) of the segment reached to out (3 floats; segment
// start at +0, delta at +0xC). When t runs exactly past the last segment, the last one is used
// at ratio 1.
void fn_8010C354(f32* out, u8* self, f32 t) {
    u8* e = RingEntry(self, *(s32*)(self + 0xB00));
    while (e != NULL && *(f32*)(e + 0x18) < t) {
        t -= *(f32*)(e + 0x18);
        e = RingNext(self, e);
    }
    if (t > -0.0001f && t < 0.0001f && e == NULL) {
        u8* last = RingEntry(self, *(s32*)(self + 0xB04));
        f32 x = *(f32*)(last + 0);
        f32 y = *(f32*)(last + 4);
        f32 z = *(f32*)(last + 8);
        out[0] = x;
        out[1] = y;
        out[2] = z;
        f32 dur = *(f32*)(last + 0x18);
        f32 r = dur / *(f32*)(last + 0x18);
        out[0] = x + *(f32*)(last + 0xC) * r;
        out[1] = y + *(f32*)(last + 0x10) * r;
        out[2] = z + *(f32*)(last + 0x14) * r;
        return;
    }
    f32 x = *(f32*)(e + 0);
    f32 y = *(f32*)(e + 4);
    f32 z = *(f32*)(e + 8);
    out[0] = x;
    out[1] = y;
    out[2] = z;
    f32 r = t / *(f32*)(e + 0x18);
    out[0] = x + *(f32*)(e + 0xC) * r;
    out[1] = y + *(f32*)(e + 0x10) * r;
    out[2] = z + *(f32*)(e + 0x14) * r;
}

// 0x8010C90C
// Spawns an entity from the template with id `key` at pos: takes a free entity slot, registers
// it with the world, copies the template's 0x3E-byte state block (template +0x20) into the
// entity's state (entity +4), places it at pos (also stored at +0x18 and +0xC8) with unit
// scale at +8, sets the placement flags and zone id, and runs SpawnEntity. Returns the entity
// or NULL when no template/slot/registration is available. `flag` sets state bit 0x04000000
// before registering.
void* fn_8010C90C(u32 key, f32* pos, s32 flag) {
    ScopedLock lock;
    ScopedLockCtor(&lock);
    f32 px = pos[0];
    f32 py = pos[1];
    f32 pz = pos[2];
    u8* tmpl = fn_801DB2E4(*(u8**)(*(u8**)gWorldState + 0x1A4) + 0xE4, key);
    if (tmpl == NULL) {
        ScopedLockDtor(&lock, -1);
        return NULL;
    }
    u8** slot = (u8**)fn_801DB194(tmpl);
    if (slot == NULL) {
        ScopedLockDtor(&lock, -1);
        return NULL;
    }
    u8* ent = *slot;
    if (ent == NULL) {
        ScopedLockDtor(&lock, -1);
        return NULL;
    }
    if (flag != 0) *(u32*)(*(u8**)(ent + 4) + 0x20) |= 0x04000000u;
    *(u8***)(ent + 0xBC) = slot;
    if (fn_8012E420(gWorldState, ent, *(u8*)(*(u8**)(ent + 4) + 0x34), 0) == 0) {
        ScopedLockDtor(&lock, -1);
        return NULL;
    }
    u8* src = *(u8**)(tmpl + 0x20);
    u8* dst = *(u8**)(ent + 4);
    for (s32 o = 0; o < 0x30; o += 4) *(u32*)(dst + o) = *(u32*)(src + o);
    *(u16*)(dst + 0x30) = *(u16*)(src + 0x30);
    *(u16*)(dst + 0x32) = *(u16*)(src + 0x32);
    for (s32 o = 0x34; o < 0x3E; o++) *(u8*)(dst + o) = *(u8*)(src + o);
    *(f32*)(ent + 0xC8) = px;
    *(f32*)(ent + 0xCC) = py;
    *(f32*)(ent + 0xD0) = pz;
    *(f32*)(ent + 0x18) = px;
    *(f32*)(ent + 0x1C) = py;
    *(f32*)(ent + 0x20) = pz;
    *(f32*)(ent + 0x8) = 1.0f;
    *(f32*)(ent + 0xC) = 1.0f;
    *(f32*)(ent + 0x10) = 1.0f;
    u8* st = *(u8**)(ent + 4);
    *(f32*)(st + 0x0) = px;
    *(f32*)(st + 0x4) = py;
    *(f32*)(st + 0x8) = pz;
    *(u32*)(*(u8**)(ent + 4) + 0x20) |= 0x8000u;
    *(u32*)(*(u8**)(ent + 4) + 0x20) |= 0x10000000u;
    *(u32*)(*(u8**)(ent + 4) + 0x20) |= 0x4000u;
    *(u32*)(*(u8**)(ent + 4) + 0x20) |= 0x10000000u;
    u8* zone = *(u8**)(gWorldState + 0xE0);
    if (zone != NULL && ZoneIsLoaded(gWorldState, *(s32*)(zone + 0x18)) != 0) {
        *(u16*)(*(u8**)(ent + 4) + 0x30) = (u16) * (s32*)(zone + 0x18);
    }
    *(u16*)(*(u8**)(ent + 4) + 0x32) = 0;
    fn_801B4EA4(ent, 0);
    EntityRefreshLinks(ent);
    SpawnEntity(ent);
    ScopedLockDtor(&lock, -1);
    return ent;
}

// 0x80116F08
// Animates a screen element (hash at +0x18) over `total` (+0x1C) steps of progress `step`
// (+0x20): while running, its value at +0x6C is start (+4) + (end (+0x28) - base (+0x24)) * step
// / total (or final (+0x10) - base once done); with no steps it just takes start. Writes the
// element's +0x78 (base or end) and calls its vtable slot 0x2C with 0x18 to refresh it.
void fn_80116F08(u8* self) {
    if (*(u32*)(self + 0x18) == 0) return;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* el = (u8*)ScreenSetFindElement(set, *(u32*)(self + 0x18));
    if (el == NULL) return;
    s32 total = *(s32*)(self + 0x1C);
    if (total > 0) {
        s32 step = *(s32*)(self + 0x20);
        f32 base = *(f32*)(self + 0x24);
        f32 a = *(f32*)(el + 0x74);
        f32 b = *(f32*)(el + 0x7C);
        f32 delta = (*(f32*)(self + 0x28) - base) * ((f32)step / (f32)total);
        if (step < total) {
            *(f32*)(el + 0x6C) = delta + *(f32*)(self + 0x4);
        } else {
            *(f32*)(el + 0x6C) = *(f32*)(self + 0x10) - base;
        }
        *(f32*)(el + 0x74) = a;
        *(f32*)(el + 0x78) = base;
        *(f32*)(el + 0x7C) = b;
    } else {
        f32 b = *(f32*)(el + 0x7C);
        f32 end = *(f32*)(self + 0x28);
        f32 a = *(f32*)(el + 0x74);
        *(f32*)(el + 0x6C) = *(f32*)(self + 0x4);
        *(f32*)(el + 0x74) = a;
        *(f32*)(el + 0x78) = end;
        *(f32*)(el + 0x7C) = b;
    }
    (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
}

// 0x80117844
// Sets up the lab monster-inventory scene: looks up the UI element 0x60B15D91 (+0x24), clears
// the five-word table lbl_803C4E28, and on first run finds the character 0xA9638202 (+0x28),
// readies it (fn_8004D760 / fn_8004DAF8 with the monster at +0x20) and refreshes the element.
// Then, while the lightning tower reports a monster on the operating table, repositions the
// monster inventory (+0x2C).
void fn_80117844(u8* self) {
    *(void**)(self + 0x24) = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), 0x60B15D91u);
    lbl_803C4E28[0] = 0;
    lbl_803C4E28[1] = 0;
    lbl_803C4E28[2] = 0;
    lbl_803C4E28[3] = 0;
    lbl_803C4E28[4] = 0;
    lbl_805FB038 = 5;
    lbl_805FB034 = 0;
    if (*(void**)(self + 0x28) == NULL) {
        void* chr = CharacterFind((s32)0xA9638202);
        *(void**)(self + 0x28) = chr;
        if (chr != NULL) {
            u8* monster = *(u8**)(self + 0x20);
            if (monster != NULL) {
                fn_8004D760((u8*)chr, monster);
                fn_8004DAF8(*(void**)(self + 0x28), 1, 0);
                lbl_805FB038 = 5;
            }
            u8* el = *(u8**)(self + 0x24);
            if (el != NULL) {
                fn_801A3010(el, NULL);
                el = *(u8**)(self + 0x24);
                (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x30))(el, 0x1A);
            }
        }
        *(s32*)(self + 0x3C) = 1;
    }
    if (LightningTowerGetOnOperatingTable() > 0) {
        SetMonsterInventoryPos(*(s32*)(self + 0x2C));
    }
}

// 0x80118C54
// Checks the save block's 0x69 owned-item slots (0xC bytes each at lbl_805FBBE0+0xBE8: kind at
// +0, index at +4, flag byte at +8) against database entry `idx` of the 0xC-byte table at
// data+0x3E60. mode 1: true if an owned kind-1 slot's row (data+0x5328, 0x18 bytes) has the
// entry's key (+8) at +0xC. mode 0: collects the 0x24-byte rows of table data+0x6C18 whose +0xC
// is idx and true if an owned kind-0 slot's row (data+0x4F00, 0x10 bytes) starts with one of
// their +0x10 values. The first argument is unused.
s32 fn_80118C54(u8* unused, s32 idx, s32 mode) {
    (void)unused;
    u8* data = gGameDb->data;
    u8* entry = *(u8**)(data + 0x3E60) + idx * 0xC;
    s32 key = *(s32*)(entry + 8);
    if (mode == 1) {
        u8* slot = lbl_805FBBE0 + 0xBE8;
        for (s32 i = 0; i < 0x69; i++, slot += 0xC) {
            if (*(u8*)(slot + 8) != 0 && *(s32*)slot == 1) {
                u8* row = *(u8**)(data + 0x5328) + *(s32*)(slot + 4) * 0x18;
                if (*(s32*)(row + 0xC) == key) return 1;
            }
        }
        return 0;
    }
    if (mode != 0) return 0;
    s32 keys[0x9C];
    s32 count = 0;
    for (s32 i = 0; i < 0x9C; i++) {
        u8* row = *(u8**)(data + 0x6C18) + i * 0x24;
        keys[count] = *(s32*)(row + 0x10);
        if (idx == *(s32*)(row + 0xC)) count++;
    }
    u8* slot = lbl_805FBBE0 + 0xBE8;
    for (s32 i = 0; i < 0x69; i++, slot += 0xC) {
        if (*(u8*)(slot + 8) != 0 && *(s32*)slot == 0) {
            u8* row = *(u8**)(data + 0x4F00) + *(s32*)(slot + 4) * 0x10;
            s32 v = *(s32*)row;
            for (s32 j = 0; j < count; j++) {
                if (v == keys[j]) return 1;
            }
        }
    }
    return 0;
}

// 0x80118FC0
// Steps the page by `delta`: releases the two texture slots (+0x44 texture, +0x48 texture or
// pending load id) of each of the `count` (+0x34) 0x20-byte entries, then wraps the page number
// at +0x28 into 0/1, records at +0x14D8 whether it was page 1, resets +0x30, sets +4 to
// page + 1 and rebuilds with fn_8011813C.
void fn_80118FC0(u8* self, s32 delta) {
    u8* e = self;
    for (s32 i = 0; i < *(s32*)(self + 0x34); i++, e += 0x20) {
        u8* tex = *(u8**)(e + 0x44);
        if (tex != NULL) {
            s32 rc = *(s32*)(tex + 0x24) - 1;
            *(s32*)(tex + 0x24) = rc;
            if (rc == 0) TextureRelease(tex);
            *(u8**)(e + 0x44) = NULL;
        }
        u8* slot = *(u8**)(e + 0x48);
        if (slot != NULL) {
            if (fn_80109A0C(*(s32*)(e + 0x48)) == 0) {
                fn_801098DC(*(s32*)(e + 0x48));
            } else {
                s32 rc = *(s32*)(slot + 0x24) - 1;
                *(s32*)(slot + 0x24) = rc;
                if (rc == 0) TextureRelease(slot);
            }
            *(u8**)(e + 0x48) = NULL;
        }
    }
    s32 page = *(s32*)(self + 0x28);
    *(s32*)(self + 0x28) = page + delta;
    *(u8*)(self + 0x14D8) = (page - 1) == 0;
    while (*(s32*)(self + 0x28) < 0) *(s32*)(self + 0x28) = *(s32*)(self + 0x28) + 2;
    s32 wrapped = *(s32*)(self + 0x28) & 1;
    *(s32*)(self + 0x30) = 0;
    *(s32*)(self + 0x28) = wrapped;
    *(s32*)(self + 4) = wrapped + 1;
    fn_8011813C(self);
}

// 0x801198A0
// Part `part` at tier `tier` was acquired: finds the first save slot (0x69 of 0xC bytes at
// lbl_805FBBE0+0xBE8) of kind 0 whose index equals the part row's key (+0x10 of the 0x24-byte
// row at data+0x6C18), marks it in lbl_803E6B28 (+4) when its flag byte is set and notifies the
// lab. If that part tier is then owned, finds the matching row (same tier byte, +0xC, +8 vs the
// part's +8/+0x14) among the first 0x3C rows at data+0x5328 and does the same for the kind-1
// slot with that row index in lbl_803E6B34. Finally calls fn_8011A04C.
void fn_801198A0(s32 part, s32 tier) {
    u8* row = *(u8**)(gGameDb->data + 0x6C18) + part * 0x24;
    s32 key = *(s32*)(row + 0x10);
    s32 i = 0;
    while (i < 0x69 && *(s32*)(lbl_803E6B28 + 4) == -1) {
        u8* slot = lbl_805FBBE0 + i * 0xC;
        if (*(s32*)(slot + 0xBEC) == key && *(s32*)(slot + 0xBE8) == 0) {
            if (*(u8*)(slot + 0xBF0) != 0) {
                *(s32*)(lbl_803E6B28 + 4) = i;
                if ((u8*)gLab != NULL) fn_80218588((u8*)gLab, i);
            }
            i = 0x69;
        }
        i++;
    }
    if (*(s32*)(lbl_803E6B28 + 4) != -1 && SavePartTierOwned(lbl_805FBBE0, (u32)part, (u8)tier) != 0) {
        s32 found = 0x3C;
        for (s32 j = 0; j < 0x3C; j++) {
            u8* r = *(u8**)(gGameDb->data + 0x5328) + j * 0x18;
            if (*(s8*)(r + 0x13) == tier && *(s32*)(r + 0xC) == *(s32*)(row + 8) &&
                *(s32*)(r + 0x8) == *(s32*)(row + 0x14)) {
                found = j;
                j = 0x3C;
            }
        }
        i = 0;
        while (i < 0x69 && *(s32*)(lbl_803E6B34 + 4) == -1) {
            u8* slot = lbl_805FBBE0 + i * 0xC;
            if (*(s32*)(slot + 0xBEC) == found && *(s32*)(slot + 0xBE8) == 1) {
                if (*(u8*)(slot + 0xBF0) != 0) {
                    *(s32*)(lbl_803E6B34 + 4) = found;
                    if ((u8*)gLab != NULL) fn_802186D0((u8*)gLab, found);
                }
                i = 0x69;
            }
            i++;
        }
    }
    fn_8011A04C();
}

// 0x8011B898
// Rebuilds the filtered list of 0x60-byte records (count at +0x25A4, from +0x1C): keeps those
// whose +0x3C matches filter A (+0x2CAC, 3 = any) and +0x40 matches filter B (+0x2CB0, 5 = any),
// storing the record pointer (+0x25A8) and its +0, +8 and +0xC values (+0x2768/+0x2928/+0x2AE8)
// per kept record, count at +0x2CA8. Then sets up the pager (+0x2CB4) for 4 records per page and
// clamps the current page (+0x25A0) to the page count.
void fn_8011B898(u8* self) {
    memset(self + 0x2768, 0, 0x1C0);
    memset(self + 0x2928, 0, 0x1C0);
    memset(self + 0x2AE8, 0, 0x1C0);
    *(s32*)(self + 0x2CA8) = 0;
    u8* rec = self + 0x1C;
    for (s32 i = 0; i < *(s32*)(self + 0x25A4); i++, rec += 0x60) {
        s32 fa = *(s32*)(self + 0x2CAC);
        if (!(*(s32*)(rec + 0x3C) == fa || fa == 3)) continue;
        s32 fb = *(s32*)(self + 0x2CB0);
        if (!(*(s32*)(rec + 0x40) == fb || fb == 5)) continue;
        s32 n = *(s32*)(self + 0x2CA8);
        *(u8**)(self + 0x25A8 + n * 4) = rec;
        *(s32*)(self + 0x2768 + *(s32*)(self + 0x2CA8) * 4) = *(s32*)rec;
        if (*(s32*)rec != 0) {
            *(s32*)(self + 0x2928 + *(s32*)(self + 0x2CA8) * 4) = *(s32*)(rec + 8);
            *(s32*)(self + 0x2AE8 + *(s32*)(self + 0x2CA8) * 4) = *(s32*)(rec + 0xC);
        }
        *(s32*)(self + 0x2CA8) = *(s32*)(self + 0x2CA8) + 1;
    }
    s32 count = *(s32*)(self + 0x2CA8);
    s32 pages = count / 4;
    if (count % 4 > 0) pages++;
    fn_80116E54(self + 0x2CB4, 3, pages);
    s32 cur = *(s32*)(self + 0x25A0);
    if (pages > 0) {
        if (cur < 0) cur = 0;
        else if (cur > pages) cur = pages;
    } else if (cur < pages) {
        cur = pages;
    } else if (cur > 0) {
        cur = 0;
    }
    *(s32*)(self + 0x25A0) = cur;
    fn_80116EF0(self + 0x2CB4, cur);
}

// String for a text hash (or the "Not found" marker strings): the high bit of the id selects the
// second string table.
static const char* TextFromHash(u32 hash) {
    s32 id = TextIdFromHash(hash);
    if (id == -1) return lbl_80354800;
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        if (gStringTable2->strings != NULL && ((u32)id & 0x7FFFFFFFu) < gStringTable2->count) {
            return gStringTable2->strings[(u32)id & 0x7FFFFFFFu];
        }
        return lbl_805F8200;
    }
    if (gStringTable->strings != NULL && (u32)id < gStringTable->count) {
        return gStringTable->strings[id];
    }
    if (id == -1) return lbl_8035480C;
    return lbl_805F8200;
}

// String 0x43F of the main string table, or empty.
static const char* TextOther(void) {
    if (gStringTable->strings != NULL && gStringTable->count > 0x43F) {
        return gStringTable->strings[0x43F];
    }
    return lbl_805F8200;
}

// 0x8011BB28
// Name text of the current category (+0x2CB0) from the table at data+0xC80 (8-byte rows, hash
// at +4); category 5 returns string 0x43F of the main table instead.
const char* fn_8011BB28(u8* self) {
    s32 cat = *(s32*)(self + 0x2CB0);
    if (cat != 5) {
        u8* row = *(u8**)(gGameDb->data + 0xC80) + cat * 8;
        return TextFromHash(*(u32*)(row + 4));
    }
    return TextOther();
}

// 0x8011D038
// Name text of the current selection (+0x12F4, 0..2) from the table at data+0x14D0 (8-byte
// rows, hash at +4); larger values return string 0x43F of the main table.
const char* fn_8011D038(u8* self) {
    s32 sel = *(s32*)(self + 0x12F4);
    if (sel < 3) {
        u8* row = *(u8**)(gGameDb->data + 0x14D0) + sel * 8;
        return TextFromHash(*(u32*)(row + 4));
    }
    return TextOther();
}

// 0x8011C318
// Caches the three status icon textures (elements "Enhancement", "defect", "installed") and
// fills the list rows (0x60 bytes each, count copied from the save block's +0x10EC) with the
// icon matching each owned part: +0x24 = enhancement icon if the part has an enhancement
// (+8), else defect icon if it has a defect (+4); +0x28 = installed icon when fn_8021EE94
// says the part is installed.
void fn_8011C318(u8* self) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* el = ScreenSetFindElement(set, kHashEnhancement);
    if (el != NULL) lbl_805FB298 = (u8*)fn_801A1208((u8*)el);
    el = ScreenSetFindElement(set, kHashDefect);
    if (el != NULL) lbl_805FB29C = (u8*)fn_801A1208((u8*)el);
    el = ScreenSetFindElement(set, kHashInstalled);
    if (el != NULL) lbl_805FB2A0 = (u8*)fn_801A1208((u8*)el);
    *(s32*)(self + 0x25A4) = *(s32*)(lbl_805FBBE0 + 0x10EC);
    u8* row = self;
    for (s32 i = 0; i < *(s32*)(self + 0x25A4); i++, row += 0x60) {
        u8* part = *(u8**)(lbl_805FBBE0 + 0xA58 + i * 4);
        if (*(s32*)(part + 8) != -1) {
            *(u8**)(row + 0x24) = lbl_805FB298;
        } else if (*(s32*)(part + 4) != -1) {
            *(u8**)(row + 0x24) = lbl_805FB29C;
        }
        if (fn_8021EE94(part) != 0) *(u8**)(row + 0x28) = lbl_805FB2A0;
    }
}

// 0x8011CE04
// Steps the 4-way selection at +0x12F4 by `delta`, wrapping into 0..3 (the original unrolls
// "add 4 until non-negative"), stores it also at +4 (as 1 when it is 3, else value + 2).
void fn_8011CE04(u8* self, s32 delta) {
    s32 x = *(s32*)(self + 0x12F4) + delta;
    if (x < 0) x &= 3;
    else if (x >= 4) x %= 4;
    *(s32*)(self + 0x12F4) = x;
    *(s32*)(self + 4) = x;
    if (x == 3) {
        *(s32*)(self + 4) = 1;
        return;
    }
    *(s32*)(self + 4) = x + 2;
}

// 0x800FA608
// Builds the asset name for a splash/logo screen in the static buffer lbl_803C7130 and returns
// it: the strap screens (hashes 0x4771D601 / 0xDE7887BB) get "strapA_640x480_" / "strapB_640x480_"
// plus the language suffix chosen by `lang` (English / French / German / Italian / Spanish (US or
// EU by flag) / Dutch / jp; 6 and 7 add nothing); the logo hashes append "legal_sony",
// "logo_backbone", "logo_eidos", "legal" or "middleware_legal".
char* fn_800FA608(u32 hash, u32 lang, s32 flag) {
    memset(lbl_803C7130, 0, 0xFF);
    if (hash == 0x4771D601u || hash == 0xDE7887BBu) {
        strcat(lbl_803C7130, hash == 0x4771D601u ? "strapA_640x480_" : "strapB_640x480_");
        if (lang <= 8) {
            switch (lang) {
            case 0: strcat(lbl_803C7130, flag != 0 ? "English_US" : "English"); break;
            case 1: strcat(lbl_803C7130, "French"); break;
            case 2: strcat(lbl_803C7130, "German"); break;
            case 3: strcat(lbl_803C7130, "Italian"); break;
            case 4: strcat(lbl_803C7130, flag != 0 ? "Spanish_US" : "Spanish_EU"); break;
            case 5: strcat(lbl_803C7130, "Dutch"); break;
            case 8: strcat(lbl_803C7130, "jp"); break;
            default: break;
            }
        }
    }
    switch ((s32)hash) {
    case (s32)0xFF773E76: strcat(lbl_803C7130, "legal_sony"); break;       // "SOny"
    case 0x689EF871: strcat(lbl_803C7130, "logo_backbone"); break;          // "Backbone"
    case 0x423D410B: strcat(lbl_803C7130, "logo_eidos"); break;             // "Eidos"
    case (s32)0xD4DFE3CE: strcat(lbl_803C7130, "legal"); break;             // "Nintendo"
    case (s32)0xA65C8B4C: strcat(lbl_803C7130, "middleware_legal"); break;
    default: break;
    }
    return lbl_803C7130;
}

}  // extern "C"
