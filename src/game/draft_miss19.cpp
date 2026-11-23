// Mixed small functions: monster limb damage, texture lookup, name entry, event manager timers,
// rocket spawn, UI slot cleanup (0x8004DE74 - 0x801ED314).
#include "types.h"
#include "game/node_ref.h"
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

typedef void (*FreeFn)(void* p, void* heap);
typedef s32  (*WidgetPosFn)(void* el, s32 a, f32 x, f32 y);
typedef void (*LockedVFn)(void* obj, s32 a);

extern "C++" void*        SpawnHelperAttached(void* a, void* owner, void* b);   // 0x8000C508 (C++ linkage, game_spawn.cpp)
extern "C++" void*        gEventMgr;       // 0x805FB6C8
extern "C++" void*        gScreenSets;     // 0x805FB654
extern "C++" StringTable* gStringTable;    // 0x805FB910

extern "C" {
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void*  IdPoolNext(void* pool);                                                                  // 0x801E6EB0
void   LimbStatusInit(u8** field, PartInstance** part);                                         // 0x80214A58
s32    PartGetCurrentHealth(const PartInstance* p);                                             // 0x8021EF68
void   PartSetHealth(PartInstance* p, s32 hp);                                                  // 0x8021D8EC
void   ScopedLockCtor(ScopedLock* l);                                                           // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                                                // 0x801F53B0
void*  ScreenSetFindWidget(void* set, u32 hash);                                                // 0x80198E34
void*  ScreenSetsCurrent(void* sets);                                                           // 0x8018D1E4
void   SirFinishLoad(void* loader, void** handle);                                              // 0x801E6C44
void*  SirLoad(void* loader, const char* path, void* dst, u32 size, s32 flags);                 // 0x801E6138
u32    StringHash(const char* s);                                                               // 0x801EB704
int    SysSprintf(char* buf, const char* fmt, ...);
void   UISetText(void* el, const char* text, s32 a);                                            // 0x8019EFC8
void*  UIWidgetFindChild(void* widget, u32 hash);                                               // 0x80199AEC
s32    WorldGetPhase(void* world);                                                              // 0x800D83A8
void   WorldSendEvent(void* world, s32 id, int a, int b, int c);                                // 0x800D87EC
BOOL   ZoneIsLoaded(void* world, s32 zone);                                                     // 0x8012F408
u8*    fn_80062C80(u8* p);                                                                      // 0x80062C80 (base ctor, returns this)
s32    fn_8012E7B0(u8* world, u8* obj, s32 a);                                                  // 0x8012E7B0
BOOL   fn_8013E464(void);                                                                       // 0x8013E464
void   fn_8016964C(u8* mgr, u8* tex);                                                           // 0x8016964C
u8*    fn_8016A08C(const char* name);                                                           // 0x8016A08C (texture cache lookup)
void fn_8016A2B8(u8* p);                                                                       // 0x8016A2B8
s32 CheckWiiFileVersion(u8* arg0, u8* arg1, s32 arg2);                                                // 0x8016B5E0
u8*    fn_8016B724(u8* obj, s32 a, const char* name);                                           // 0x8016B724
void   fn_8017493C(void* p);                                                                    // 0x8017493C
void   fn_8018A5F4(void* p);                                                                    // 0x8018A5F4
void   fn_8018D68C(f32* out, void* sets);                                                       // 0x8018D68C
BOOL   fn_80198C34(void* set, void* w);                                                         // 0x80198C34
char*  fn_801D8ADC(char* s);                                                                    // 0x801D8ADC
s32    fn_801E5F48(u8* pool, const char* name, u32 hash);                                       // 0x801E5F48
void   fn_801EBA20(void* list, void* item);                                                     // 0x801EBA20
void   fn_801EC03C(u8* p);                                                                      // 0x801EC03C

extern u8          gGame[];                  // 0x803B53A0
extern u8          gIdPool[];                // 0x8048A8F0
extern u8          gWorldState[];            // 0x803E6D00
extern u8          lbl_80359A28[];           // 0x80359A28 (vtable)
extern f32         lbl_80345FC0[];           // 0x80345FC0 (3 floats)
extern u8          lbl_803C75C8[];           // 0x803C75C8 (0x7D0 records of 0x40 bytes)
extern char        lbl_803F2230[];           // 0x803F2230 (name edit buffer)
extern MEMAllocator lbl_803F2870;            // 0x803F2870
extern const char  lbl_8036C8DC[];           // 0x8036C8DC (name for the ".sir" format)
extern const char  lbl_805F8200[];           // 0x805F8200 (empty string)
extern const char  lbl_805F8EB8[];           // 0x805F8EB8 (".sir" file name format)
extern const char  lbl_805F8EDC[];           // 0x805F8EDC (id-name buffer format)
extern f32         lbl_805F90C0;             // 0x805F90C0
extern f32         lbl_805F90C4;             // 0x805F90C4
extern f32         lbl_805F90C8;             // 0x805F90C8
extern char        lbl_805F90CC[];           // 0x805F90CC
extern s32         lbl_805FB038;             // 0x805FB038 (number of combat part zones)
extern u8*         lbl_805FB220;             // 0x805FB220 (texture load buffer)
extern u8*         lbl_805FB224;             // 0x805FB224
extern u8          lbl_805FB228;             // 0x805FB228
extern s32         lbl_805FB22C;             // 0x805FB22C
extern s32         lbl_805FB230;             // 0x805FB230
extern s32         lbl_805FB234;             // 0x805FB234
extern u8*         lbl_805FB2B8;             // 0x805FB2B8 name entry state (0x18 bytes)
extern s32         lbl_805FB408;             // 0x805FB408
extern s32         lbl_805FB40C;             // 0x805FB40C
extern u8*         lbl_805FB5A8;             // 0x805FB5A8 (texture manager)
extern u8*         lbl_805FB608;             // 0x805FB608 (heap-like object, free function at +4)
extern u8*         lbl_805FBBE0;             // 0x805FBBE0 (save block pointer)

static inline BOOL PartAlive(const PartInstance* p) { return p != NULL && PartGetCurrentHealth(p) > 0; }

// 0x801ED314
// Sets the flag bits of a waypoint-params byte: bit 7 (0x80) means "already set", otherwise
// mode 1 sets bit 7 and any other mode ORs in its low 4 bits.
// The reference word is read and written as a whole so the code does not depend on byte order.
void WaypointParamsInit(void* params, int a) {
    u8* p = (u8*)params;
    u8 v = NodeRefByte0(NodeRefWord(p));
    if (v & 0x80) return;
    if (a == 1) {
        NodeRefSetByte0(p, (u8)(v | 0x80));
    } else {
        NodeRefSetByte0(p, (u8)(v | (a & 0xF)));
    }
}

// 0x8004DE74
// Applies damage to the five limb parts of a monster: clamps health at 0 (calling slot 0x18 of
// the limb's effect object under a lock when a limb other than 1 or 4 dies), then, if it is the
// player's monster, sends the damage event and ends the fight when all limbs are gone.
void fn_8004DE74(void* mon, s32 kind) {
    u8* m = (u8*)mon;
    if (*(s32*)(m + 0xC4C) == 0) return;
    for (s32 i = 0; i < 5; i++) {
        u8* elem = *(u8**)(*(u8**)(m + 0xC4C) + i * 4);
        if (elem == NULL) continue;
        PartInstance* part = *(PartInstance**)(m + 0x89C + i * 4);
        s32 hp = PartGetCurrentHealth(part) - kind;
        if (hp <= 0) {
            hp = 0;
            if (i != 4 && i != 1) {
                u8* fx = *(u8**)(m + 0xC50 + i * 4);
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(LockedVFn*)(*(u8**)fx + 0x18))(fx, 0);
                ScopedLockDtor(&lock, -1);
            }
        }
        PartSetHealth(*(PartInstance**)(m + 0x89C + i * 4), hp);
        PartInstance* np = *(PartInstance**)(m + 0x89C + i * 4);
        *(PartInstance**)elem = np;
        if (np != NULL) LimbStatusInit((u8**)(elem + 0x28), (PartInstance**)elem);
    }
    if (m == *(u8**)(gGame + 0xD614)) {
        if (kind > 0) WorldSendEvent(gGame + 0xD5F0, 0x520CC75C, 0, 0, 0);
        BOOL allDead = FALSE;
        if (!PartAlive(*(PartInstance**)(m + 0x8AC)) ||
            (!PartAlive(*(PartInstance**)(m + 0x89C)) && !PartAlive(*(PartInstance**)(m + 0x8A4)) &&
             !PartAlive(*(PartInstance**)(m + 0x8A8)) && !PartAlive(*(PartInstance**)(m + 0x8A0)))) {
            allDead = TRUE;
        }
        if (allDead && WorldGetPhase(gGame + 0xD5F0) == 2) {
            GameFlowSignal(0x8A9EB7D1, 0, 0);
            *(s32*)(lbl_805FBBE0 + 0x104) = 9;
        }
    }
}

// 0x8005CA88
// Constructor: runs the base constructor, installs the vtable and, if the object has a
// definition at +4, sets flag 8 in its +0x1C and byte 3 at +0x34.
u8* fn_8005CA88(u8* p) {
    fn_80062C80(p);
    u8* def = *(u8**)(p + 0x4);
    *(void**)p = lbl_80359A28;
    if (def != NULL) {
        *(s32*)(def + 0x1C) = *(s32*)(def + 0x1C) | 8;
        *(u8*)(*(u8**)(p + 0x4) + 0x34) = 3;
    }
    return p;
}

// 0x80109330
// Looks up a shared texture by name: returns the cached one (bumping its refcount at +0x24) or,
// on a miss, loads "<shared>.sir" into the shared buffer (initialising the 0x7D0-entry id table and
// buffer on first use) and registers the new texture with the texture manager.
void* TextureLoadShared(const char* name) {
    char path1[0x80];
    char path2[0x80];
    u8   obj[12];
    void* handle;

    u32 hash = StringHash(name);
    SysSprintf(path1, lbl_805F8EB8, lbl_8036C8DC);
    s32 id = fn_801E5F48(gIdPool, path1, hash);
    char* shortName = fn_801D8ADC((char*)name);
    u8* tex = fn_8016A08C(shortName);
    if (tex != NULL) {
        *(s32*)(tex + 0x24) = *(s32*)(tex + 0x24) + 1;
        return tex;
    }
    if (lbl_805FB220 == NULL) {
        lbl_805FB230 = 1;
        u8* rec = lbl_803C75C8;
        lbl_805FB22C = 1;
        lbl_805FB234 = 0;
        lbl_805FB228 = 0;
        for (s32 i = 0; i < 0x7D0; i++) {
            *(void**)rec = IdPoolNext(gIdPool);
            *(s32*)(rec + 0x4) = 0;
            *(s32*)(rec + 0x38) = 0;
            SysSprintf((char*)(rec + 8), lbl_805F8EDC);
            *(s32*)(rec + 0x3C) = 0;
            rec += 0x40;
        }
        if (lbl_805FB220 == NULL) {
            u8* buf = (u8*)MEMAllocFromAllocator(&lbl_803F2870, 0xC0000);
            lbl_805FB224 = buf;
            lbl_805FB220 = buf;
        }
    }
    u8* dst = lbl_805FB220;
    SysSprintf(path2, lbl_805F8EB8, lbl_8036C8DC);
    void* loaded = SirLoad(gIdPool, path2, dst, 0xC0000, (s32)hash);
    BOOL ok;
    if (loaded == (void*)-1) {
        ok = FALSE;
    } else {
        handle = loaded;
        SirFinishLoad(gIdPool, &handle);
        ok = TRUE;
    }
    if (ok) {
        CheckWiiFileVersion(obj, lbl_805FB220, id);
        tex = fn_8016B724(obj, 0, shortName);
        if (tex != NULL) fn_8016964C(lbl_805FB5A8, tex);
    }
    return tex;
}

// 0x80142358
// True when the HUD has the widget 0x28DCC9E9 but the other widget (0x1B6732BB) does not report
// a state, no combat part zones remain and fn_8013E464 is false.
BOOL fn_80142358(void) {
    BOOL result = FALSE;
    BOOL a = FALSE;
    BOOL b = FALSE;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        void* wa = ScreenSetFindWidget(set, 0x1B6732BBu);
        void* wb = ScreenSetFindWidget(set, 0x28DCC9E9u);
        if (wa != NULL) a = fn_80198C34(set, wa);
        if (wb != NULL) b = TRUE;
    }
    if (lbl_805FB038 <= 0 && !a && b && !fn_8013E464()) result = TRUE;
    return result;
}

// 0x801425DC
// Timer: keeps the stored timestamp (u64 at +0) from running up to 60000000 ticks ahead of the
// current time, then returns the elapsed time since it in milliseconds.
u64 fn_801425DC(u8* timer) {
    OSTime now = OSGetTime();
    OSTime last = *(OSTime*)timer;
    if ((u64)(last - now) < 60000000ull) {
        *(OSTime*)timer = now;
    }
    OSTime elapsed = now - *(OSTime*)timer;
    return (u64)(((s64)elapsed * 8) / (s64)(OSGetBusClock() / 4 / 125000));
}

// Releases a pooled UI slot object: drops the refcount of its resource at +0x14 (calling
// fn_8016A2B8 when it reaches 0), clears the slot and frees it through the heap object.
static void FreeUiSlot(u8* obj) {
    u8* res = *(u8**)(obj + 0x14);
    if (res != NULL) {
        s32 rc = *(s32*)(res + 0x10) - 1;
        *(s32*)(res + 0x10) = rc;
        if (rc == 0) fn_8016A2B8(res);
        *(void**)(obj + 0x14) = NULL;
    }
    *(s32*)obj = 0;
    (*(FreeFn*)(lbl_805FB608 + 4))(obj, lbl_805FB608);
}

// 0x8018A78C
// Destructor: frees the three 0x24-byte slots (each with an owned object at +0x1C) and the
// three extra slot objects at +0x74/+0x78/+0x7C, then runs the base destructor part.
void fn_8018A78C(u8* arg0) {
    u8* e = arg0;
    for (s32 i = 0; i < 3; i++) {
        if (*(s32*)(e + 0x8) != 0) {
            fn_8017493C(*(void**)(e + 0x8));
            *(void**)(e + 0x8) = NULL;
            FreeUiSlot(*(u8**)(e + 0x1C));
            *(void**)(e + 0x1C) = NULL;
        }
        e += 0x24;
    }
    for (s32 off = 0x74; off <= 0x7C; off += 4) {
        u8* slot = *(u8**)(arg0 + off);
        if (slot != NULL) {
            FreeUiSlot(slot);
            *(void**)(arg0 + off) = NULL;
        }
    }
    fn_8018A5F4(arg0);
}

// 0x8019CD14
// Gets the current screen-set size and forwards it to slot 0x38 of the element's secondary
// vtable (+0x138), returning its result.
s32 fn_8019CD14(void* el) {
    f32 pos[3];   // fn_8018D68C writes x, y, z
    fn_8018D68C(pos, gScreenSets);
    return (*(WidgetPosFn*)(*(u8**)((u8*)el + 0x138) + 0x38))(el, 0, pos[0], pos[1]);
}

// 0x80120B48
// Name entry: appends the character (or, for NUL, deletes the last one) in the name buffer
// (max 0xF characters; a full buffer overwrites the last one). Ignored while locked (+0x14)
// and for a leading space.
void EnterName_AddLetter(const char* a0) {
    u8* st = lbl_805FB2B8;
    if (*(u8*)(st + 0x14) != 0) return;
    s32 n = *(s32*)(st + 0x10);
    if (n == 0 && *a0 == 0x20) return;
    if (st == NULL) return;
    u8 raw = (u8)*a0;
    if ((s8)raw != 0) {
        if (n < 0xF) {
            s32 cur = *(s32*)(st + 0x10);
            lbl_805FB2B8[cur] = raw;
            *(s32*)(st + 0x10) = cur + 1;
            lbl_805FB2B8[*(s32*)(lbl_805FB2B8 + 0x10)] = 0;
            return;
        }
        st[n - 1] = raw;
        return;
    }
    if (n > 0) {
        *(s32*)(st + 0x10) = *(s32*)(st + 0x10) - 1;
        lbl_805FB2B8[*(s32*)(lbl_805FB2B8 + 0x10)] = 0;
    }
}

// 0x80120C24
// Name entry: adds a space unless the entry is locked.
void EnterName_AddSpace(void) {
    char c;
    if (*(u8*)(lbl_805FB2B8 + 0x14) == 0) {
        c = 0x20;
        EnterName_AddLetter(&c);
    }
}

// 0x80120BE8
// Name entry: deletes the last character unless the entry is locked.
void EnterName_DelLetter(void) {
    char c;
    if (*(u8*)(lbl_805FB2B8 + 0x14) == 0) {
        c = 0;
        EnterName_AddLetter(&c);
    }
}

// 0x8013EB3C
// Opens the scientist-name screen: clears the edit buffer, captures the name field's original
// colour on first use, sets the header text from the string table (index 0x5A4) and the name
// text to its default.
void EnterScientistNameInit(void) {
    lbl_803F2230[0] = 0;
    lbl_805FB408 = 0;
    void* set = *(void**)((u8*)gScreenSets + 0x88);
    if (set != NULL) {
        void* w = ScreenSetFindWidget(set, 0x68B693B2u);   // "NAME"
        if (w != NULL) {
            void* title = UIWidgetFindChild(w, 0x2D6ECFDEu);
            void* field = UIWidgetFindChild(w, 0xC0E2E2ABu);
            if (title != NULL && field != NULL) {
                if (lbl_805F90C0 == -1.0f) {
                    lbl_805F90C0 = *(f32*)((u8*)field + 0x68);
                    lbl_805F90C4 = *(f32*)((u8*)field + 0x6C);
                    lbl_805F90C8 = *(f32*)((u8*)field + 0x70);
                }
                const char* text;
                if (gStringTable->strings != NULL && gStringTable->count > 0x5A4) {
                    text = gStringTable->strings[0x5A4];
                } else {
                    text = lbl_805F8200;
                }
                UISetText(title, text, 1);
            }
            void* name = UIWidgetFindChild(w, 0x68B693B2u);   // "NAME"
            if (name != NULL) UISetText(name, lbl_805F90CC, 1);
        }
    }
    lbl_805FB40C = 1;
}

// 0x8004FEF8
// Zone-unload callback for an entity part: releases its list entry at +0x94 if any, and removes
// the object from the world if it has one at +4. Always returns 1.
u32 EntityPartZoneUnloadCallback(u8* a0, void* a1) {
    if (*(s32*)(a0 + 0x94) > 0) {
        fn_801EC03C(a0 + 0x94);
    }
    if (*(void**)(a0 + 0x4) != NULL) {
        fn_8012E7B0(gWorldState, a0, 1);
        *(void**)(a0 + 0x4) = NULL;
    }
    return 1;
}

// 0x801BA108
// Checks the pending-event ring buffer (head +0x410, tail +0x418, 0x40 entries of 16 bytes)
// for an event matching the entity's ids (+0xEC/+0xF0/+0xF4) and flags the entity (+0x4)->0x20
// with 0x200 on the first match.
void EventMgrFlushFor(u8* arg0, u8* arg1) {
    u32 idB = *(u32*)(arg1 + 0xF0);
    u32 idA = *(u32*)(arg1 + 0xEC);
    u32 idC = *(u32*)(arg1 + 0xF4);
    if (idB == 0 && idC == 0) return;
    u32 head = *(u32*)(arg0 + 0x410);
    u32 tail = *(u32*)(arg0 + 0x418);
    while (head != tail) {
        u8* ent = arg0 + 0x410 + head * 16;
        head++;
        if (head == 0x40) head = 0;
        if ((*(u32*)(ent + 0x10) == idA && *(u32*)(ent + 0xC) == idB) || *(u32*)(ent + 0xC) == idC) {
            u8* ctl = *(u8**)(arg1 + 0x4);
            *(s32*)(ctl + 0x20) = *(s32*)(ctl + 0x20) | 0x200;
            return;
        }
    }
}

// 0x801BA4B4
// Ticks the delayed-event list: counts each entry's timer (+0x14) down by dt; at zero it sends
// the event (if the target's zone is still loaded) and either removes the entry or re-arms the
// timer with its repeat interval (+0x18); entries with a dead target are removed.
void EventMgrTick(void* mgr, f32 dt) {
    u8* link = *(u8**)(*(u8**)mgr + 0x14);
    u8* it = link != NULL ? link + 8 : NULL;
    while (it != NULL) {
        u8* nextLink = *(u8**)(it - 4);
        u8* next = nextLink != NULL ? nextLink + 8 : NULL;
        f32 t = *(f32*)(it + 0x14) - dt;
        *(f32*)(it + 0x14) = t;
        if (t <= 0.0f) {
            if (*(void**)(it + 0x0) != NULL && ZoneIsLoaded(gWorldState, *(s32*)(it + 0x4))) {
                EventSend(gEventMgr, *(void**)(it + 0x0), NULL, *(u32*)(it + 0x8), *(s32*)(it + 0xC),
                          *(s32*)(it + 0x10), 1, 0);
                f32 repeat = *(f32*)(it + 0x18) - 0.0f;
                if (repeat > -0.0001f && repeat < 0.0001f) {
                    fn_801EBA20(*(void**)mgr, it);
                } else {
                    *(f32*)(it + 0x14) = *(f32*)(it + 0x18);
                }
            } else {
                fn_801EBA20(*(void**)mgr, it);
            }
        }
        it = next;
    }
}

// 0x800F88AC
// Fires a rocket at an entity: aims from the default muzzle point (or from just above the
// target when its flag bit is set) to the target's position, spawns the helper and stores the
// direction vector at +0x1AC of the spawned object.
void FireRocketAtEntity(void* a0) {
    if (a0 == NULL) return;
    u8* world = *(u8**)gWorldState;
    void* owner = *(void**)(world + 0x78);
    if (owner == NULL) return;
    u8* ent = (u8*)a0;
    u8* pos = *(u8**)(ent + 0x4);
    f32 target[3];
    f32 origin[3];
    target[0] = *(f32*)(pos + 0x0);
    target[1] = *(f32*)(pos + 0x4);
    target[2] = *(f32*)(pos + 0x8);
    origin[0] = lbl_80345FC0[0];
    origin[1] = lbl_80345FC0[1];
    origin[2] = lbl_80345FC0[2];
    if ((*(u32*)(*(u8**)(ent + 0x4) + 0x1C) >> 13) & 1) {
        origin[0] = target[0];
        origin[1] = target[1];
        origin[2] = (f32)(10.0 + (f64)target[2]);
    }
    f32 args[9];   // origin, direction, direction (passed to the spawn call as a block)
    f32 dx = target[0] - origin[0];
    f32 dy = target[1] - origin[1];
    f32 dz = target[2] - origin[2];
    args[3] = dx;
    args[4] = dy;
    args[5] = dz;
    args[6] = dx;
    args[7] = dy;
    args[8] = dz;
    args[0] = origin[0];
    args[1] = origin[1];
    args[2] = origin[2];
    u8* spawned = (u8*)SpawnHelperAttached(args, owner, target);
    if (spawned != NULL) {
        *(f32*)(spawned + 0x1AC) = args[6];
        *(f32*)(spawned + 0x1B0) = args[7];
        *(f32*)(spawned + 0x1B4) = args[8];
    }
}

}  // extern "C"
