// Missing functions batch 18: operating table (lightning manager) setup/teardown, entity part and
// pooled entity callbacks, lab book teardown, rematch queries, debug stats overlay, defect textures.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "game/part.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp
struct PathFollower;

extern "C" {
#include "game/script_callbacks.h"

void   BufferFree(void* p);                                          // 0x801D4980
void   MonsterBuildPreset(u8* m, s32 preset);                        // 0x8021E4B0
s32    PathGetNodeId(PathFollower* p);                               // 0x800D56F8
void   ScopedLockCtor(ScopedLock* l);                                // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                     // 0x801F53B0
void   SessionDisableFor(void* session, s32 id, f32 time);           // 0x80012820
int    SysSprintf(char* buf, const char* fmt, ...);                  // 0x80142EFC
void   WaypointParamsInit(void* params, int a);                      // 0x801ED314
s32    WorldGetPhase(void* world);                                   // 0x800D83A8
BOOL   ZoneIsLoaded(void* world, s32 zone);                          // 0x8012F408
void   __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);  // 0x8032F2E0
void   PartDestroy(PartInstance* p, s32 del);                        // 0x8021D374
void   operator_delete(void* p);                                     // 0x801D426C
u8*    fn_80021750(void);                                            // 0x80021750
s32    fn_800217A8(void);                                            // 0x800217A8
void   fn_80021BE4(void);                                            // 0x80021BE4
void   fn_8003DCE0(u8* p);                                           // 0x8003DCE0
void   fn_8004C718(u8* p, s32 idx, void* elem);                      // 0x8004C718
s32    fn_8004D2C8(u8* obj, const char* name, s32 a, s32 b);         // 0x8004D2C8
void   fn_8004D39C(u8* p);                                           // 0x8004D39C
void   fn_8001D718(u8* mgr, s32 idx, u8* slot);                      // 0x8001D718
void   fn_8005230C(void);                                            // 0x8005230C
u8*   fn_8005CA88(u8* p);                                           // 0x8005CA88
void*  fn_80060A28(void);                                            // 0x80060A28
void   fn_800D83B8(u8* a3, s32 a4);                                  // 0x800D83B8
void*  fn_80109330(const char* name);                                // 0x80109330 (texture lookup by name)
s32    fn_801138F8(void);                                            // 0x801138F8
s32    fn_80119B78(void);                                            // 0x80119B78
s32    fn_8012E7B0(u8* world, u8* obj, s32 a);                       // 0x8012E7B0
BOOL   fn_80142358(void);                                            // 0x80142358
u64    fn_801425DC(u8* timer);                                       // 0x801425DC
void   fn_8015EB28(u8* a, f32* outA, f32* outB);                     // 0x8015EB28
void   fn_8016D8C4(void);                                            // 0x8016D8C4
void   fn_8016DAE8(void);                                            // 0x8016DAE8
void   fn_80188E5C(u8* p);                                           // 0x80188E5C
void   fn_80188F00(const char* fmt, ...);                            // 0x80188F00
void   fn_8018A78C(void* p);                                         // 0x8018A78C
void   fn_801EC03C(u8* p);                                           // 0x801EC03C
void   fn_801EE078(void* params, u8* path, s32 a);                   // 0x801EE078
u8*    fn_8020E530(u8* p, s32 a);                                    // 0x8020E530 (a is really an owner pointer)
void   fn_8020E864(u8* p, s32 del);                                  // 0x8020E864
void   fn_8020EB44(u8* p, s32 del);                                  // 0x8020EB44
s32    fn_80333424(const char* s, void* file);                       // 0x80333424 (fputs-like)
void*  fn_801D40E0(s32 n);                                           // 0x801D40E0

extern u8   gGame[];                       // 0x803B53A0
extern u8   gWorldState[];                 // 0x803E6D00 (first word: pointer to the world block)
extern s32  lbl_803C4E28[];                // 0x803C4E28 (combat part zone-loaded flags)
extern u8   lbl_803F22D8[];                // 0x803F22D8
extern const char lbl_8036CF70[];          // 0x8036CF70 (defect texture path pieces)
extern const char lbl_80375E18[];          // 0x80375E18 ("EndTest\n")
extern const char lbl_80354800[];          // 0x80354800 ("Not found")
extern const char lbl_8035480C[];          // 0x8035480C ("NOT FOUND")
extern const char lbl_805F8200[];          // 0x805F8200 (empty string)
extern u8   lbl_80359ED8[];                // 0x80359ED8 (vtable)
extern const char lbl_80359ECC[];          // 0x80359ECC ("projectile")
extern u8   lbl_80412998[];                // 0x80412998 (render stats block)
extern u8*  lbl_805FAF78;                  // 0x805FAF78 (operating table / lightning manager)
extern void* lbl_805FB0DC;                 // 0x805FB0DC (active conversation object)
extern s32  lbl_805F90A0;                  // 0x805F90A0
extern s32  lbl_805FB034;                  // 0x805FB034
extern s32  lbl_805FB038;                  // 0x805FB038 (number of combat part zones)
extern u8   lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)
extern u8   lbl_805FB24C;                  // 0x805FB24C
extern const char* lbl_805FB258;           // 0x805FB258 (last defect text)
extern s32  lbl_805FB418;                  // 0x805FB418 (connection state flags)
extern s32  lbl_805FB4B0;                  // 0x805FB4B0
extern s32  lbl_805FB4B4;                  // 0x805FB4B4
extern u8   lbl_805FB594[];                // 0x805FB594
extern s32  lbl_805FB5C4;                  // 0x805FB5C4
extern s32  lbl_805FB5C8;                  // 0x805FB5C8
extern s32  lbl_805FB5CC;                  // 0x805FB5CC
extern s32  lbl_805FB5D0;                  // 0x805FB5D0
extern s32  lbl_805FB5E0;                  // 0x805FB5E0 (frame time ring index)
extern s32  lbl_805FB5E4;                  // 0x805FB5E4 (ring count)
extern u32  lbl_805FB5E8;                  // 0x805FB5E8 (last timestamp, high word)
extern u32  lbl_805FB5EC;                  // 0x805FB5EC (last timestamp, low word)
extern u8   lbl_805FB5F0[];                // 0x805FB5F0 (timer object)
extern s32  lbl_805FB628;                  // 0x805FB628
extern s32  lbl_805FB62C;                  // 0x805FB62C
extern s32  lbl_805FB630;                  // 0x805FB630
extern s32  lbl_805FB644;                  // 0x805FB644
}

extern "C++" s32 gZoneResourceSize;        // 0x805FB94C
struct StringTable;
extern "C++" StringTable* gStringTable;    // 0x805FB910
extern "C++" void* gStringTable2;         // 0x805FB90C

typedef void (*VFn)(void* self);
typedef void (*VFnA)(void* self, s32 a);
typedef void* (*VGetFn)(void* self);
typedef s32 (*VIntFn)(void* self);

extern "C" {

// 0x8003CF40
// Script callback: true when every combat part zone slot (lbl_805FB038 of them) is loaded.
u32 /*0x06*/ CombatPartZonesAreLoaded(void) {
    for (s32 i = 0; i < lbl_805FB038; i++) {
        if (lbl_803C4E28[i] == 0) return 0;
    }
    return 1;
}

// 0x8001C968
// Script callback: gives the current fighter object (first word of the manager) an empty monster:
// builds preset 0 at +0x898, allocates its 0x68-byte record and resets the five per-slot counters.
void CreateEmptyMonster(void) {
    u8* mgr = lbl_805FAF78;
    if (mgr != NULL && *(u8**)mgr != NULL) {
        u8* obj = *(u8**)mgr;
        fn_8004D2C8(obj, "hanging", 0, 1);
        obj = *(u8**)lbl_805FAF78;
        if ((*(s32(**)(void*))(*(u8**)obj + 0xF0))(obj) == 0) {
            obj = *(u8**)lbl_805FAF78;
            if (!(*(s32*)(*(u8**)(obj + 0x4) + 0x1C) & 0x4000)) {
                *(f32*)(obj + 0x9F4) = 6.0f;
            } else {
                *(f32*)(obj + 0x9F4) = 3.5f;
            }
            MonsterBuildPreset(*(u8**)lbl_805FAF78 + 0x898, 0);
            *(u8*)(*(u8**)lbl_805FAF78 + 0xC84) = 1;
            u8* rec = (u8*)fn_801D40E0(0x68);
            if (rec != NULL) {
                u8* owner = *(u8**)lbl_805FAF78;
                if (owner != NULL) owner += 0xC48;
                fn_8020E530(rec, (s32)(intptr_t)owner);  // lint:ok owner pointer passed through an s32 parameter
            }
            *(s32*)(rec + 0x14) = *(s32*)(*(u8**)lbl_805FAF78 + 0x8DC);
            *(u8**)(*(u8**)lbl_805FAF78 + 0xC4C) = rec;
        }
        for (s32 i = 0; i < 5; i++) {
            u8* slot = lbl_805FAF78 + i * 0x1A4;
            *(s32*)(slot + 0x198) = *(s32*)(slot + 0x1A0) - 1;
            fn_8001D718(lbl_805FAF78, i, slot);
        }
        *(s32*)(lbl_805FAF78 + 0xBF8) = 0;
        *(s32*)(lbl_805FAF78 + 0xAD8) = -1;
    }
}

// 0x80068EE8
// Script callback (type 0x7): constructs a projectile entity (base ctor, vtable lbl_80359ED8, child
// flags |= 0x80 and state 7), sets bits 3 of +0xB8 and names it "projectile". Always returns true.
u32 /*0x06*/ CreateProjectileEntityCallback(u32 /*0x08*/ a0, u32 /*0x08*/ a1) {
    u8* obj = (u8*)(uintptr_t)a0;
    if (obj != NULL) {
        fn_8005CA88(obj);
        u8* child = *(u8**)(obj + 0x4);
        *(void**)obj = lbl_80359ED8;
        if (child != NULL) {
            *(u32*)(child + 0x1C) |= 0x80;
            *(u8*)(*(u8**)(obj + 0x4) + 0x34) = 7;
        }
    }
    *(u32*)(obj + 0xB8) |= 3;
    strcpy((char*)(obj + 0x108), lbl_80359ECC);
    return 1;
}

// 0x8013EA68
// Script callback: true when the current lab selection equals lbl_805F90A0.
ScriptValue CurrentlySelected(void) {
    return fn_801138F8() == lbl_805F90A0;
}

// 0x8001B898
// Script callback: tears down the operating table manager: clears the zone-loaded flags, leaves the
// phase-7 world state, releases the five slot records and finally deletes the manager.
void DeInitializeOperatingTable(void) {
    if (lbl_805FAF78 != NULL) {
        for (s32 i = 0; i < 10; i++) lbl_803C4E28[i] = 0;
        lbl_805FB038 = 0;
        lbl_805FB034 = 0;
        if (WorldGetPhase(gGame + 0xD5F0) == 7) {
            fn_800D83B8(gGame + 0xD5F0, *(s32*)(*(u8**)gWorldState + 0x1E0));
        }
        if (*(u8**)lbl_805FAF78 != NULL) {
            fn_8004D39C(*(u8**)lbl_805FAF78);
        }
        for (s32 i = 0; i < 5; i++) {
            u8* obj = *(u8**)lbl_805FAF78;
            u8* rec = *(u8**)(obj + 0xC50 + i * 4);
            if (rec != NULL) {
                fn_8004C718(obj, i, NULL);
                fn_8003DCE0(rec);
            }
        }
        *(s32*)(lbl_805FAF78 + 0xAE4) = 0;
        if (*(u8**)lbl_805FAF78 != NULL) {
            *(u8**)lbl_805FAF78 = NULL;
        }
        u8* mgr = lbl_805FAF78;
        if (mgr != NULL) {
            u8* obj = *(u8**)mgr;
            if (obj != NULL) {
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(VFnA*)(*(u8**)obj + 0x18))(obj, 0);
                ScopedLockDtor(&lock, -1);
                u8* slot = mgr;
                for (s32 i = 0; i < 5; i++) {
                    u8* rec = *(u8**)(slot + 0x1A4);
                    if (rec != NULL) {
                        fn_8020E864(rec, 1);
                        *(s32*)(slot + 0x1A4) = 0;
                    }
                    slot += 0x1A4;
                }
            }
            fn_8020EB44(mgr + 0x940, -1);
            __destroy_arr(mgr + 0x83C, (void (*)(void*, s32))PartDestroy, 0x30, 5);
            operator_delete(mgr);
        }
        lbl_805FAF78 = NULL;
    }
}

// 0x8004FE6C
// Script callback (type 0x7): releases an entity part: lets its sub-object (virtual slot 0xC) run
// its slot 0x10, frees the list at +0x94 when it has entries and the buffer at +0xA0. Returns true.
u32 /*0x06*/ DeallocEntityPartCallback(u32 /*0x08*/ a0, u32 /*0x08*/ a1) {
    u8* obj = (u8*)(uintptr_t)a0;
    if ((*(VIntFn*)(*(u8**)obj + 0xC))(obj) != 0) {
        u8* sub = (u8*)(*(VGetFn*)(*(u8**)obj + 0xC))(obj);
        (*(VFn*)(*(u8**)sub + 0x10))(sub);
    }
    if (*(s32*)(obj + 0x94) > 0) {
        fn_801EC03C(obj + 0x94);
    }
    u8* buf = *(u8**)(obj + 0xA0);
    if (buf != NULL) {
        fn_8018A78C(buf);
    }
    return 1;
}

// 0x8016DAF4
// Per-frame debug overlay: records the frame time in a 32-entry ring (clamped to 0x7FFFFFFF),
// snapshots and clears the render counters, then prints vertex/strip and FPS lines.
void DebugDrawStats(void) {
    char sp210[0x100];
    char sp110[0x100];
    char sp10[0x100];
    f32 spC;
    f32 sp8;

    u64 t = fn_801425DC(lbl_805FB5F0);
    u32 lo = (u32)t;
    u32 hi = (u32)(t >> 32);
    u64 prev = ((u64)lbl_805FB5E8 << 32) | lbl_805FB5EC;
    u64 diff = t - prev;
    s32 delta = 0x7FFFFFFF;
    if (diff <= 0x7FFFFFFFu) delta = (s32)diff;
    lbl_805FB5EC = lo;
    s32 idx = lbl_805FB5E0 + 1;
    lbl_805FB5E8 = hi;
    lbl_805FB5E0 = idx;
    if (idx >= 0x20) {
        idx = 0;
        lbl_805FB5E0 = 0;
    }
    *(s32*)(lbl_80412998 + 0xD8 + idx * 4) = delta;
    if (lbl_805FB5E4 < 0x20) lbl_805FB5E4 += 1;

    u8* src = lbl_80412998 + 0x50;
    u8* dst = lbl_80412998 + 0x94;
    for (s32 off = 0; off <= 0x3C; off += 4) {
        if (off != 0x38) *(s32*)(dst + off) = *(s32*)(src + off);
    }
    *(s32*)(dst + 0x38) = lbl_805FB4B0;
    *(s32*)(dst + 0x40) = lbl_805FB4B4;
    lbl_805FB4B0 = 0;
    lbl_805FB4B4 = 0;
    memset(src, 0, 0x44);

    sp210[0] = 0;
    if (lbl_805FB5C4 != 0) {
        s32 tristrips = *(s32*)(dst + 0x1C) + *(s32*)(dst + 0x2C);
        s32 verts = *(s32*)(dst + 0x18) + *(s32*)(dst + 0x28);
        f32 avg = 0.0f;
        if (tristrips > 0) {
            avg = ((f32)verts / (f32)tristrips) - 2.0f;
        }
        SysSprintf(sp110, "%d verts, %d tristrips, avg strip=%.2f tris", verts, tristrips, avg);
        strcat(sp210, sp110);
    }
    if (lbl_805FB5CC != 0) {
        fn_8015EB28(lbl_805FB594, &spC, &sp8);
        SysSprintf(sp10, "  FPS=%.1f/%d (%.2fmsec)", spC, (s32)(0.5f + sp8), 1000.0f / spC);
        strcat(sp210, sp10);
    }
    if (sp210[0] != 0) {
        s32 v = lbl_805FB644;
        lbl_805FB62C = 0;
        lbl_805FB628 = v;
        lbl_805FB630 = -1;
        fn_80188F00("%s", sp210, v);
    }
    if (lbl_805FB5C8 != 0) fn_8016D8C4();
    if (lbl_805FB5D0 != 0) fn_8016DAE8();
    fn_80188E5C(NULL);
}

// 0x80116A20
// Looks up the texture of defect `arg0`: builds "<prefix><formatted defect record>" and finds it by
// name (falling back to "<prefix><default suffix>"), and stores the defect's display text (string
// table lookup of its name hash) in lbl_805FB258.
void* DefectGetTexture(s32 arg0) {
    char sp128[0x100];
    char sp28[0x100];

    u8* e = *(u8**)(gGameDb->data + 0x31E8) + (arg0 << 5);
    u32 hash = *(u32*)(e + 0x8);
    SysSprintf(sp128, lbl_8036CF70 + 0x58, (s32)*(s8*)(e + 0x1C), *(s32*)(e + 0x14), *(s32*)(e + 0x10),
               *(s32*)(e + 0xC), *(s32*)(e + 0x8), *(s32*)(e + 0x4));
    strcpy(sp28, lbl_8036CF70 + 0x48);
    strcat(sp28, sp128);
    void* tex = fn_80109330(sp28);
    s32 id = TextIdFromHash(hash);
    const char* str;
    if (id == -1) {
        str = lbl_80354800;
    } else if ((id & 0x80000000) && gStringTable2 != NULL) {
        const char** arr = *(const char***)((u8*)gStringTable2 + 0x0);
        u32 i = (u32)id & 0x7FFFFFFFu;
        if (arr != NULL && i < *(u32*)((u8*)gStringTable2 + 0xC)) str = arr[i];
        else str = lbl_805F8200;
    } else {
        const char** arr = *(const char***)((u8*)gStringTable + 0x0);
        if (arr != NULL && (u32)id < *(u32*)((u8*)gStringTable + 0xC)) {
            str = arr[id];
        } else {
            str = lbl_805F8200;
            if (id == -1) str = lbl_8035480C;
        }
    }
    lbl_805FB258 = str;
    if (tex == NULL) {
        strcpy(sp28, lbl_8036CF70 + 0x48);
        strcat(sp28, lbl_8036CF70 + 0x68);
        tex = fn_80109330(sp28);
    }
    return tex;
}

// 0x800EFC0C
// Script callback: when the current challenge object has flag 0x2000 and the minigame state is not
// 8, disables its path node's session for 65 seconds. Always finishes with fn_80021BE4.
void DeinitChallenge(void) {
    u8* obj = *(u8**)(gGame + 0xD614);
    if (obj != NULL && (*(s32*)(*(u8**)(obj + 0x4) + 0x1C) & 0x2000)) {
        fn_80021750();
        if (fn_800217A8() != 8) {
            u8* path = *(u8**)(obj + 0xA00);
            if (path != NULL) {
                SessionDisableFor(gGame + 0x5C8, PathGetNodeId((PathFollower*)path), 65.0f);
            }
        }
    }
    fn_80021BE4();
}

// 0x8004FF60
// Script callback (type 0x7): if the part has a record at +4, detaches it from the world and clears
// it. Returns true.
u32 /*0x06*/ DemolishEntityPartCallback(u32 /*0x08*/ a0, u32 /*0x08*/ a1) {
    u8* obj = (u8*)(uintptr_t)a0;
    if (*(s32*)(obj + 0x4) != 0) {
        fn_8012E7B0(gWorldState, obj, 0);
        *(s32*)(obj + 0x4) = 0;
    }
    return 1;
}

// 0x8010C818
// Script callback (type 0x7): a0 points to an object pointer; runs its virtual slot 0x40, frees the
// buffer and clears the pointer. Returns true.
u32 /*0x06*/ DemolishPooledEntityCallback(u32 /*0x08*/ a0, u32 /*0x08*/ a1) {
    u8** slot = (u8**)(uintptr_t)a0;
    u8* obj = *slot;
    if (obj != NULL) {
        (*(VFn*)(*(u8**)obj + 0x40))(obj);
        BufferFree(obj);
        *slot = NULL;
    }
    return 1;
}

// 0x8010C7C0
// Script callback (type 0x7): a0 points to an object pointer; runs its virtual slot 0x10 and frees
// the buffer (the pointer is left as is). Returns true.
u32 /*0x06*/ DemolishRenderObjectCallback(u32 /*0x08*/ a0, u32 /*0x08*/ a1) {
    u8** slot = (u8**)(uintptr_t)a0;
    u8* obj = *slot;
    if (obj != NULL) {
        (*(VFn*)(*(u8**)obj + 0x10))(obj);
        BufferFree(obj);
    }
    return 1;
}

// 0x8010FE38
// Script callback: destroys the lab book section table: closes the current section's object (when a
// section is selected), releases both section objects, deletes the table and clears its pointer.
void DestroyLabBooks(void) {
    u8* top = *(u8**)lbl_805FB248;
    if (top != NULL) {
        s32 cur = *(s32*)top;
        if (cur != -1) {
            u8* sub = *(u8**)(*(u8**)(top + 4 + cur * 4) + 0x4);
            (*(VFn*)(*(u8**)sub + 0x10))(sub);
        }
        u8* table = *(u8**)lbl_805FB248;
        if (table != NULL) {
            for (s32 i = 0; i < 2; i++) {
                u8* sec = *(u8**)(table + 4 + i * 4);
                if (sec != NULL) {
                    (*(VFnA*)(*(u8**)sec + 0x8))(sec, 1);
                    *(u8**)(table + 4 + i * 4) = NULL;
                }
            }
            operator_delete(table);
        }
        *(u8**)lbl_805FB248 = NULL;
    }
    if (lbl_805FB24C != 0) fn_8005230C();
}

// 0x8013FD7C
// Script callback: true when the connection flag 4 is set and fn_80142358 reports true.
ScriptValue DidOpponentAcceptRematch(void) {
    if ((lbl_805FB418 & 4) && fn_80142358() != 0) return 1;
    return 0;
}

// 0x8013FD3C
// Script callback: true when the connection flag 2 is set and fn_80142358 reports true.
ScriptValue DidOpponentDeclineRematch(void) {
    if ((lbl_805FB418 & 2) && fn_80142358() != 0) return 1;
    return 0;
}

// 0x800F68A8
// Script callback: clears byte +0x54 of every object in every loaded zone's object list.
void DisableAllCameraInfluenceInZone(void) {
    for (s32 zone = 0; zone <= *(s32*)(*(u8**)gWorldState + 0x4); zone++) {
        if (ZoneIsLoaded(gWorldState, zone) != 0) {
            u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
            for (s32 i = 0; i < *(u8*)(res + 0xAC); i++) {
                u8* obj = *(u8**)(*(u8**)(res + 0xA8) + i * 4);
                if (obj != NULL) {
                    *(u8*)(obj + 0x54) = 0;
                }
            }
        }
    }
}

// 0x800EF354
// Script callback: makes enemy a0 fire lightning at the target position read from a1. Looks up (and
// caches at +0xA14) the active target object; fires only while that object's virtual slot 0x4C is 0.
void EnemyFireLightningAtActiveTarget(u32 /*0x2F*/ a0, u32 /*0x30*/ a1, u32 a2, u32 /*0x38*/ a3, f32 a4, f32 a5) {
    u8* enemy = (u8*)(uintptr_t)a0;
    if (*(void**)(enemy + 0xA14) == NULL) {
        *(void**)(enemy + 0xA14) = fn_80060A28();
    }
    u8* target = *(u8**)(enemy + 0xA14);
    if (target != NULL && (*(VIntFn*)(*(u8**)target + 0x4C))(target) == 0) {
        s32 params = 0;  // bytes: [0] = 0, [1] = 0, halfword at +2 = 0xFFFF
        u8* pb = (u8*)&params;
        pb[0] = 0;
        *(u16*)(pb + 2) = 0xFFFF;
        pb[1] = 0;
        WaypointParamsInit(pb, 6);
        fn_801EE078(pb, target, 0);
        s32 sp8 = *(s32*)pb;
        s32 spC = *(s32*)(uintptr_t)a1;
        FireLightningAtTarget(enemy, (u32)(uintptr_t)&spC, a2, a3, a4, a5, (u32)(uintptr_t)&sp8);  // lint:ok header takes u32 for these pointer args
    }
}

// 0x801124AC
// Script callback: returns fn_80119B78() (tail call).
s32 DoingIngredientsSelection(void) {
    return fn_80119B78();
}

// 0x80142F90
// Script callback: clears the word at lbl_803F22D8+0x304 and, when the log file handle in the first
// word is set, writes the line "EndTest\n" to it.
void EndTest(void) {
    void* file = *(void**)lbl_803F22D8;
    *(s32*)(lbl_803F22D8 + 0x304) = 0;
    if (file == NULL) return;
    fn_80333424(lbl_80375E18, file);
}

// 0x80051724
// Script callback: forwards a0 to the active conversation object's virtual slot 0xA0 (tail call).
void EnqueIdleAnim(u32 a0) {
    void* obj = lbl_805FB0DC;
    if (obj == NULL) return;
    (*(void (**)(void*, u32))(*(u8**)obj + 0xA0))(obj, a0);
}

}  // extern "C"
