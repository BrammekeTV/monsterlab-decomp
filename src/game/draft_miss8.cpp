// Miscellaneous functions from the eighth "missing" draft batch: reaction names for limbs, prop
// hit handling and state steps, HUD tint pushes, small object-array helpers, conversation HUD
// hooks, effect spawn/despawn and a callback-table removal.
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };   // as in props.cpp

typedef u8* (*GetFn)(void*);                       // object vtable +0xC / +0xCC: returns a sub-object
typedef void (*VisFn)(void*, s32);                 // object vtable slot (self, arg)
typedef void (*UIAngleFn)(void*, s32, f32, f32);   // UI element (+0x138) vtable +0x2C
typedef void (*OverlayFn)(void*, s32, u32);        // overlay table (lbl_80348F38) +0x24: (target, id, float bits)

extern "C" {
void   BufferFree(void* p);                                               // 0x801D4980
void*  EventPost(s32 id, void* sender);                                   // 0x801F3918
void   EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);   // 0x801BA2FC
void   HudShowConversation(void* hud, s32 conversation);                  // 0x8001B0B8
void   HudUpdateScreens(void* screens, f32 dt);                           // 0x801BB5BC
void*  MemAllocSmall(u32 size);                                           // 0x801D41CC
s32    PlaySoundAt(void* sound, void* owner, const void* pos, int b);     // 0x80209ABC
u32    RandomU32(void);                                                   // 0x80143460
void   ReactionBuildNamed(char* out, s32 limb, void* fighter, s32 reaction);   // 0x802156B0
s32    RaySceneTest(void* ray);                                           // 0x80070458
void   ScopedLockCtor(ScopedLock* l);                                     // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                          // 0x801F53B0
void   SpawnEffectAt(void* fxMgr, u32 effect, const f32* posAndScale, u8** owner);   // 0x801C80E0
u32    StringHash(const char* s);                                         // 0x801EB704
int    SysSprintf(char* buf, const char* fmt, ...);                       // 0x80142EFC
void*  UIWidgetFindChild(void* widget, u32 hash);                         // 0x80199AEC
void   fn_8001B09C(u8* hud, s32 a);                                       // 0x8001B09C
s32    fn_8001B094(u8* hud);                                              // 0x8001B094
u8*    fn_8001B044(u8* hud, u32 key);                                     // 0x8001B044
void   fn_8001B128(void* hud);                                            // 0x8001B128
void   fn_8001B164(u8* hud);                                              // 0x8001B164
void   fn_8004EED0(u8* limb, u32 reactionHash, s32 a);                    // 0x8004EED0
u8*    fn_80062C80(u8* p);                                                // 0x80062C80
s32    fn_80063D6C(u8* self, u8* a, void* pos, u8* b, s32 c, s32 d, s32 e);   // 0x80063D6C
void   fn_800645C8(u8* self, void* pos);                                  // 0x800645C8
void   fn_800653F4(u8* self, s32 a, s32 b, s32 c);                        // 0x800653F4
void*  fn_80070F4C(void);                                                 // 0x80070F4C
void   fn_800C148C(u8* p);                                                // 0x800C148C
u8*    fn_800D3AC4(u8* db, f32* pos);                                     // 0x800D3AC4
u32    fn_800D5660(u8* a3);                                               // 0x800D5660
s32    fn_800D5680(u8* a3);                                               // 0x800D5680
s32    fn_800D56A0(u8* a3);                                               // 0x800D56A0
s32    fn_800D56C0(u8* a3);                                               // 0x800D56C0
void   fn_800DE5B4(u8* p);                                                // 0x800DE5B4
void   fn_801C83AC(u8* mgr, u8* item);                                    // 0x801C83AC
void*  fn_801D40E0(s32 n);                                                // 0x801D40E0
s32    fn_801DA780(u8* a3);                                               // 0x801DA780
u8*    fn_801DA788(u8* a3, s32 i);                                        // 0x801DA788
u32    fn_801F38F8(u8* a3);                                               // 0x801F38F8
void*  fn_80209814(void* v);                                              // 0x80209814
void   operator_delete(void* p);                                          // 0x801D426C

extern "C++" void* gEventMgr;                // 0x805FB6C8
extern "C++" s32   gZoneResourceSize;        // 0x805FB94C
extern void* gLab;                           // 0x805FBBC8
extern void* gObjDb;                         // 0x805FB180
extern u8 gLevelFlags[];                     // 0x80369D60: 0x20 bytes per level
extern u8 gWorldState[];                     // 0x803E6D00 (first word: pointer to the world block)
extern u8 lbl_80346AB8[];                    // 0x80346AB8 (f32 table)
extern u8 lbl_80348F38[];                    // 0x80348F38 (overlay dispatch table)
extern u8 lbl_8035A4C8[];                    // 0x8035A4C8 (vtable)
extern u8 lbl_8035FDF8[];                    // 0x8035FDF8 (vtable)
extern u8 lbl_803C4298[];                    // 0x803C4298
extern void* lbl_803F2830[];                 // 0x803F2830 (callback table)
extern u8 lbl_8047C760[];                    // 0x8047C760 (effect manager)
extern void* lbl_805FB1C8;                   // 0x805FB1C8 (state record pointer)
extern s32 lbl_805FB458;                     // 0x805FB458 (callback table count)
extern u32 lbl_805FB9A4;                     // 0x805FB9A4 (lock flag)
extern void* lbl_805FBBAC;                   // 0x805FBBAC (voice SFX handle)
extern f32 lbl_805FE8B0;                     // 0x805FE8B0
extern f32 lbl_805FEAF8;                     // 0x805FEAF8
extern u8 lbl_803C68B0[];                    // 0x803C68B0
extern u8 lbl_803C6900[];                    // 0x803C6900
extern u8 lbl_803C6970[];                    // 0x803C6970
extern u8 lbl_803C69E8[];                    // 0x803C69E8
extern u8 lbl_803C6A38[];                    // 0x803C6A38
extern u8 lbl_803C6AA0[];                    // 0x803C6AA0
extern u8 lbl_803C6B30[];                    // 0x803C6B30
extern u8 lbl_803C6BF0[];                    // 0x803C6BF0
extern u8 lbl_803C6C78[];                    // 0x803C6C78
extern u8 lbl_803C6D18[];                    // 0x803C6D18
extern u8 lbl_803C6D98[];                    // 0x803C6D98
extern u8 lbl_803C6F10[];                    // 0x803C6F10
extern u8 lbl_803C6F40[];                    // 0x803C6F40

static const u32 kHashBuzzsawNeedle = 0x1C42349Au;   // "Buzzsaw_needle"

// 0x8004F310
// Builds the name of limb reaction `arg1` (limb `arg2`) for the fighter and starts it on the limb.
void fn_8004F310(u8* arg0, s32 arg1, s32 arg2, s32 arg3) {
    char name[0x80];

    u8* fighter = (*(GetFn*)(*(u8**)arg0 + 0xCC))(arg0);
    ReactionBuildNamed(name, arg2, fighter, arg1);
    fn_8004EED0(arg0, StringHash(name), arg3);
}

// 0x80053418
// Binds the nearest free slot of the object database to the object (once) and caches its fields.
void fn_80053418(u8* arg0) {
    if (gObjDb != NULL && *(s32*)(arg0 + 0xA00) == 0 && *(u8*)(arg0 + 0x6B8) != 0) {
        u8* slot = fn_800D3AC4((u8*)gObjDb, *(f32**)(arg0 + 0x4));
        *(u8**)(arg0 + 0xA00) = slot;
        if (slot != NULL) {
            *(s32*)(arg0 + 0x87C) = fn_800D56C0(slot);
            *(u32*)(arg0 + 0x884) = fn_800D5660(*(u8**)(arg0 + 0xA00));
            *(s32*)(arg0 + 0x890) = fn_800D5680(*(u8**)(arg0 + 0xA00));
            *(s32*)(arg0 + 0x880) = fn_800D56A0(*(u8**)(arg0 + 0xA00));
            *(s32*)(arg0 + 0x88C) = 0;
            *(s32*)(arg0 + 0x888) = 0;
            *(u8*)(arg0 + 0x894) = 1;
            *(u8*)(arg0 + 0x895) = 0;
        }
    }
}

// 0x80065200
// Prop hit: runs the hit test under the lock, plays the hit sound, calls the prop's vtable +0x14
// handler and posts an event with the prop's two vectors. Returns 0 when the test fails, else 1.
s32 fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6) {
    ScopedLock lock;

    ScopedLockCtor(&lock);
    if (fn_80063D6C(arg0, arg1, arg2, arg3, arg4, arg5, arg6) == 0) {
        ScopedLockDtor(&lock, -1);
        return 0;
    }
    fn_800645C8(arg0, arg2);
    u8* snd = *(u8**)(*(u8**)(arg0 + 0x228) + 0x108);
    if (snd != NULL) {
        if (*(u8*)(snd + 0x5) & 1) {
            if (*(s32*)(arg0 + 0x250) < 0) {
                if (lbl_805FBBAC == NULL) {
                    void* v = fn_801D40E0(1);
                    if (v != NULL) v = fn_80209814(v);
                    lbl_805FBBAC = v;
                }
                *(s32*)(arg0 + 0x250) = PlaySoundAt(*(u8**)(*(u8**)(arg0 + 0x228) + 0x108), arg0, *(void**)(arg0 + 0x4), 0);
            }
        } else {
            PlaySoundAt(snd, NULL, *(void**)(arg0 + 0x4), 0);
        }
    }
    (*(VisFn*)(*(u8**)arg0 + 0x14))(arg0, 0);
    s32 x = *(s32*)(*(u8**)(arg0 + 0x228) + 0x10);
    if (x != 0) {
        fn_800653F4(arg0, x, 1, 1);
    }
    ScopedLockDtor(&lock, -1);
    if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
        u8* ev;
        if (*(u8**)(arg0 + 0x23C) != NULL && fn_801F38F8(*(u8**)(arg0 + 0x23C)) != 0) {
            ev = (u8*)EventPost(0, *(u8**)(arg0 + 0x23C));
        } else {
            ev = (u8*)EventPost(0, NULL);
        }
        if (ev != NULL) {
            *(f32*)(ev + 0x4) = *(f32*)(arg0 + 0x194);
            *(f32*)(ev + 0x8) = *(f32*)(arg0 + 0x198);
            *(f32*)(ev + 0xC) = *(f32*)(arg0 + 0x19C);
            *(f32*)(ev + 0x10) = *(f32*)(arg0 + 0x1AC);
            *(f32*)(ev + 0x14) = *(f32*)(arg0 + 0x1B0);
            *(f32*)(ev + 0x18) = *(f32*)(arg0 + 0x1B4);
            *(u8**)(ev + 0x1C) = *(u8**)(arg0 + 0x228);
            *(s32*)(ev + 0x20) = *(s32*)(arg0 + 0x30);
        }
    }
    return 1;
}

// 0x80069EEC
// Prop kill step: if the prop's flags (bits 0..1) are set, invalidates its timer, sends the death
// event once, then calls vtable +0x18 (state 2), under the lock when it is not already held.
void fn_80069EEC(u8* arg0) {
    ScopedLock lock;

    u8* rec = *(u8**)(arg0 + 0x4);
    if (*(u32*)(rec + 0x20) & 3) {
        *(f32*)(rec + 0x24) = -1.0f;
        if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x40000)) {
            s32 target = *(s32*)(arg0 + 0xF8);
            if (target != 0) {
                EventSendTo(gEventMgr, (u32)target, arg0, 0xEDA76DD2u, 0, 0);
            }
            u8* r2 = *(u8**)(arg0 + 0x4);
            *(u32*)(r2 + 0x20) = *(u32*)(r2 + 0x20) | 0x40000;
        }
        if (lbl_805FB9A4 != 0) {
            (*(VisFn*)(*(u8**)arg0 + 0x18))(arg0, 2);
            return;
        }
        if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
            lbl_805FB9A4 = 0;
        }
        ScopedLockCtor(&lock);
        (*(VisFn*)(*(u8**)arg0 + 0x18))(arg0, 2);
        ScopedLockDtor(&lock, -1);
    }
}

// 0x8006C3FC
// Sets the object's white tint (0x210..0x21C) and unit scale (0x228..0x230), then pushes the world
// vector at world+0x1F8 into every overlay entry of every group (overlay ids 0x135..0x137).
void fn_8006C3FC(u8* arg0) {
    f32 c = 0.003921569f * (f32)255;

    *(f32*)(arg0 + 0x210) = c;
    *(f32*)(arg0 + 0x228) = 1.0f;
    *(f32*)(arg0 + 0x214) = c;
    *(f32*)(arg0 + 0x22C) = 1.0f;
    *(f32*)(arg0 + 0x218) = c;
    *(f32*)(arg0 + 0x21C) = c;
    *(f32*)(arg0 + 0x230) = 1.0f;
    u8* world = *(u8**)gWorldState;
    f32 vx = *(f32*)(world + 0x1F8);
    f32 vy = *(f32*)(world + 0x1FC);
    f32 vz = *(f32*)(world + 0x200);

    for (s32 i = 0; i < fn_801DA780((*(GetFn*)(*(u8**)arg0 + 0xC))(arg0)); i++) {
        s32 off = 0;
        for (s32 j = 0; j < *(s32*)(*(u8**)(fn_801DA788((*(GetFn*)(*(u8**)arg0 + 0xC))(arg0), i) + 0x10) + 0x6C); j++) {
            u8* e = *(u8**)(*(u8**)(fn_801DA788((*(GetFn*)(*(u8**)arg0 + 0xC))(arg0), i) + 0x10) + off + 0x70);
            if (e != NULL) {
                f32 t = vx;
                (*(OverlayFn*)(lbl_80348F38 + *(u8*)(e + 0xC) * 4 + 0x24))(*(void**)(e + 0x8), 0x135, *(u32*)&t);
                t = vy;
                (*(OverlayFn*)(lbl_80348F38 + *(u8*)(e + 0xC) * 4 + 0x24))(*(void**)(e + 0x8), 0x136, *(u32*)&t);
                t = vz;
                (*(OverlayFn*)(lbl_80348F38 + *(u8*)(e + 0xC) * 4 + 0x24))(*(void**)(e + 0x8), 0x137, *(u32*)&t);
            }
            off += 8;
        }
    }
}

// 0x8006F61C
// Constructor: base constructor, sets the vtable and, if the child at +4 exists, flags it (|4) and
// sets its state byte to 2. Returns the object.
u8* fn_8006F61C(u8* arg0) {
    fn_80062C80(arg0);
    u8* child = *(u8**)(arg0 + 0x4);
    *(void**)arg0 = lbl_8035A4C8;
    if (child != NULL) {
        *(u32*)(child + 0x1C) = *(u32*)(child + 0x1C) | 4;
        *(u8*)(*(u8**)(arg0 + 0x4) + 0x34) = 2;
    }
    return arg0;
}

// 0x800701A4
// Returns the HUD config block for the current lab state (gLab+0x48, -1..11), or NULL.
void* fn_800701A4(void) {
    switch (*(s32*)((u8*)gLab + 0x48)) {
    case -1: return lbl_803C6F40;
    case 0:  return lbl_803C6970;
    case 1:  return lbl_803C6BF0;
    case 2:  return lbl_803C6AA0;
    case 3:  return lbl_803C6A38;
    case 4:  return lbl_803C6900;
    case 5:  return lbl_803C68B0;
    case 6:  return lbl_803C6B30;
    case 7:  return lbl_803C69E8;
    case 8:  return lbl_803C6F10;
    case 9:  return lbl_803C6D18;
    case 10: return lbl_803C6D98;
    case 11: return lbl_803C6C78;
    }
    return NULL;
}

// 0x800A75C8
// Swings the HUD needle widget: maps the ratio at +0x264 (over the table entry) to an angle between
// -62.68 and 108.04 degrees (via 26.4 at the midpoint), stores it in radians and refreshes the widget.
void fn_800A75C8(u8* arg0) {
    u8* hud = (u8*)fn_80070F4C();
    if (hud != NULL) {
        u8* needle = (u8*)UIWidgetFindChild(hud, kHashBuzzsawNeedle);
        f32 t = *(f32*)(arg0 + 0x264) / *(f32*)(lbl_80346AB8 + RaySceneTest(needle) * 4);
        f32 lo, hi, k;
        if (t > 0.5f) {
            lo = 26.4f;
            hi = 108.04f;
            k = (f32)(2.0 * ((f64)t - 0.5));
        } else {
            lo = -62.68f;
            k = 2.0f * t;
            hi = 26.4f;
        }
        *(f32*)(needle + 0x90) = 0.0f;
        *(f32*)(needle + 0x94) = 0.0f;
        f32 deg = lo + k * (hi - lo);
        *(f32*)(needle + 0x98) = 0.017453292f * deg;
        (*(UIAngleFn*)(*(u8**)(needle + 0x138) + 0x2C))(needle, 0x18, 0.0f, deg);
    }
}

// 0x800C2388
// Frees and clears an array of 0x18-byte slots (two owned buffers each, freed when their kind is
// 1 or 2), then frees and clears the array itself.
void fn_800C2388(u8* arg0) {
    s32 i;

    for (i = 0; i < *(s32*)(arg0 + 0x4); i++) {
        u8* e = *(u8**)arg0 + i * 0x18;
        void* p = *(void**)e;
        if (p != NULL) {
            s32 kind = *(s32*)(e + 0x8);
            if (kind == 2) BufferFree(p);
            else if (kind == 1) BufferFree(p);
        }
        *(s32*)(e + 0x0) = 0;
        *(s32*)(e + 0x4) = 0;
        *(s32*)(e + 0x8) = 0;
        void* q = *(void**)(e + 0xC);
        if (q != NULL) {
            s32 kind = *(s32*)(e + 0x14);
            if (kind == 2) BufferFree(q);
            else if (kind == 1) BufferFree(q);
        }
        *(void**)(e + 0xC) = NULL;
        *(s32*)(e + 0x10) = 0;
        *(s32*)(e + 0x14) = 0;
    }
    for (i = 0; i < *(s32*)(arg0 + 0x4); i++) {
        u8* e = *(u8**)arg0 + i * 0x18;
        if (e != NULL) {
            void* p = *(void**)e;
            if (p != NULL) {
                s32 kind = *(s32*)(e + 0x8);
                if (kind == 2) BufferFree(p);
                else if (kind == 1) BufferFree(p);
            }
            *(void**)e = NULL;
            *(s32*)(e + 0x4) = 0;
            *(s32*)(e + 0x8) = 0;
            void* q = *(void**)(e + 0xC);
            if (q != NULL) {
                s32 kind = *(s32*)(e + 0x14);
                if (kind == 2) BufferFree(q);
                else if (kind == 1) BufferFree(q);
            }
            *(void**)(e + 0xC) = NULL;
            *(s32*)(e + 0x10) = 0;
            *(s32*)(e + 0x14) = 0;
        }
    }
    void* data = *(void**)arg0;
    if (data != NULL) {
        s32 kind = *(s32*)(arg0 + 0x8);
        if (kind == 2) BufferFree(data);
        else if (kind == 1) BufferFree(data);
    }
    *(void**)arg0 = NULL;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
}

// 0x800C253C
// Picks a random entry from group `arg1` of the 0x18-byte group array whose flag byte at +0x26C is
// 0 (NULL when none).
u8* fn_800C253C(u8** arg0, s32 arg1) {
    u8* grp = *arg0 + arg1 * 0x18;
    s32* idx = (s32*)MemAllocSmall(*(s32*)(grp + 0x10) * 4);
    s32 n = 0;

    for (s32 i = 0; i < *(s32*)(grp + 0x10); i++) {
        u8* item = *(u8**)(*(u8**)(grp + 0xC) + i * 4);
        if (*(u8*)(item + 0x26C) == 0) {
            idx[n] = i;
            n++;
        }
    }
    u8* result = NULL;
    if (n > 0) {
        result = *(u8**)(*(u8**)(grp + 0xC) + idx[RandomU32() % (u32)n] * 4);
    }
    operator_delete(idx);
    return result;
}

// 0x800C4118
// Searches every zone's 0x4C-byte entries for the one whose id (+0x2C) matches an id in the
// object's 0xC8-byte table (+0x98); stores it at +0x2A0 and clears its low flag bit.
void fn_800C4118(u8* arg0) {
    s32 found = 0;
    s32 z = 0;

    fn_800C148C(arg0);
    *(u8**)(arg0 + 0x2A0) = NULL;
    while (z <= *(s32*)(*(u8**)gWorldState + 0x4)) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize;
        u8* entries = *(u8**)(zone + 0xA0);
        u8 b = 0;
        while (b < *(u8*)(*(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize + 0xA4)) {
            u8* entry = entries + b * 0x4C;
            for (s32 j = 0; j < *(s32*)(arg0 + 0x94); j++) {
                if (*(u32*)(entry + 0x2C) == *(u32*)(*(u8**)(arg0 + 0x98) + j * 0xC8)) {
                    *(u8**)(arg0 + 0x2A0) = entry;
                    found = 1;
                    *(u32*)(entry + 0x30) = *(u32*)(entry + 0x30) & 0xFFFFFFFEu;
                    break;
                }
            }
            if (found != 0) break;
            b++;
        }
        if (found != 0) break;
        z++;
    }
}

// 0x800D5C48
// Starts (arg1 != 0) or stops (arg1 == 0) the looping effect of the object's record at +0x14:
// type 4 and type 16 spawn their effect at the record's position and keep a handle at +0x50.
void fn_800D5C48(u8* arg0, s32 arg1) {
    u8* rec = *(u8**)(*(u8**)arg0 + 0x14);
    if (rec != NULL) {
        if (arg1 != 0 && *(u8**)(rec + 0x50) == NULL) {
            switch (*(s32*)(rec + 0x40)) {
            case 4: {
                f32 pos[4];
                pos[0] = *(f32*)(rec + 0x34);
                pos[1] = *(f32*)(rec + 0x38);
                pos[2] = *(f32*)(rec + 0x3C);
                pos[3] = 0.0f;
                SpawnEffectAt(lbl_8047C760, 0x8BEBBC3Du, pos, (u8**)(rec + 0x50));
                break;
            }
            case 16: {
                f32 pos[4];
                pos[0] = *(f32*)(rec + 0x34);
                pos[1] = *(f32*)(rec + 0x38);
                pos[2] = *(f32*)(rec + 0x3C);
                pos[3] = 0.0f;
                SpawnEffectAt(lbl_8047C760, 0xE8774AA7u, pos, (u8**)(rec + 0x50));
                break;
            }
            }
        } else if (arg1 == 0) {
            u8* handle = *(u8**)(rec + 0x50);
            if (handle != NULL) {
                fn_801C83AC(lbl_8047C760, handle);
                *(u8**)(*(u8**)(*(u8**)arg0 + 0x14) + 0x50) = NULL;
            }
        }
    }
}

// 0x800D68CC
// Sets slot `arg1` (clamped to 0..3) of the record at +0x14 to `arg2` if that slot is in use
// (word at +4+4*slot nonzero). Returns 1 when written.
s32 fn_800D68CC(u8* arg0, s32 arg1, s32 arg2) {
    s32 slot;
    s32 mask;
    u8* rec;

    if (arg1 < 0) {
        slot = 0;
    } else {
        slot = 3;
        if (arg1 <= 3) slot = arg1;
    }
    rec = *(u8**)(*(u8**)arg0 + 0x14);
    if (rec == NULL) {
        mask = 0;
    } else {
        mask = 0;
        if (*(s32*)(rec + 0x4) != 0) mask = 1 << 0;
        if (*(s32*)(rec + 0x8) != 0) mask |= 1 << 1;
        if (*(s32*)(rec + 0xC) != 0) mask |= 1 << 2;
        if (*(s32*)(rec + 0x10) != 0) mask |= 1 << 3;
    }
    if ((1 << slot) & mask) {
        *(s32*)(rec + slot * 4 + 0x24) = arg2;
        return 1;
    }
    return 0;
}

// 0x800D81AC
// Conversation HUD tick (world mode 1 or 2): moves a pending conversation id from +0 into +0x30
// and shows it on the HUD at +0x44, deferring it back to +0 while the HUD is busy.
void fn_800D81AC(u8* arg0) {
    if ((u32)(*(s32*)(*(u8**)gWorldState + 0x1E0) - 1) <= 1U) {
        s32 pending = *(s32*)arg0;
        if (pending != 0 && *(s32*)(arg0 + 0x30) == 0) {
            *(s32*)(arg0 + 0x30) = pending;
            *(s32*)arg0 = 0;
        }
        s32 conv = *(s32*)(arg0 + 0x30);
        if (conv != 0) {
            if (*(s32*)(lbl_803C4298 + 0x18) != 5) {
                u8* hud = *(u8**)(arg0 + 0x44);
                if (*(s32*)(hud + 0x42C) != 0) {
                    *(s32*)arg0 = conv;
                    *(s32*)(arg0 + 0x30) = 0;
                    return;
                }
                HudShowConversation(hud, conv);
                return;
            }
            if (fn_8001B044(*(u8**)(arg0 + 0x44), (u32)conv) != 0) {
                HudShowConversation(*(u8**)(arg0 + 0x44), *(s32*)(arg0 + 0x30));
                fn_8001B128(*(u8**)(arg0 + 0x44));
            }
        }
    }
}

// 0x800D8280
// Shows the "<level>_questgiver_fsm" conversation on the HUD at +0x44 (world mode 1 or 2, game
// state 5), restoring the HUD's saved state around the switch.
void fn_800D8280(u8* arg0) {
    char name[0x100];

    if ((u32)(*(s32*)(*(u8**)gWorldState + 0x1E0) - 1) <= 1U) {
        SysSprintf(name, "%s_questgiver_fsm", *(const char**)(gLevelFlags + lbl_803C4298[0x1C] * 0x20));
        u32 hash = StringHash(name);
        if (fn_8001B044(*(u8**)(arg0 + 0x44), hash) != 0 && *(s32*)(lbl_803C4298 + 0x18) == 5) {
            u8* hud = *(u8**)(arg0 + 0x44);
            s32 saved = fn_8001B094(*(u8**)(arg0 + 0x44));
            u8 flag = *(u8*)(hud + 0x430);
            HudShowConversation(hud, (s32)hash);
            fn_8001B128(*(u8**)(arg0 + 0x44));
            HudUpdateScreens(*(void**)(*(u8**)(arg0 + 0x44) + 0x434), 1.0f);
            *(u8*)(*(u8**)(arg0 + 0x44) + 0x430) = 1;
            fn_8001B164(*(u8**)(arg0 + 0x44));
            if (saved != 0) {
                fn_8001B09C(*(u8**)(arg0 + 0x44), saved);
            }
            if (flag != 0) {
                *(u8*)(*(u8**)(arg0 + 0x44) + 0x430) = 1;
            }
        }
    }
}

// 0x800DEC78
// Constructor: sets the vtable, initialises the subobject at +0xC, stores the owner at +8 and
// zeroes three floats. Returns the object.
u8* fn_800DEC78(u8* arg0, s32 arg1) {
    *(void**)arg0 = lbl_8035FDF8;
    fn_800DE5B4(arg0 + 0xC);
    *(s32*)(arg0 + 0x8) = arg1;
    *(f32*)(arg0 + 0x50) = lbl_805FE8B0;
    *(f32*)(arg0 + 0x34) = lbl_805FE8B0;
    *(f32*)(arg0 + 0x3C) = lbl_805FE8B0;
    return arg0;
}

// 0x800E03F0
// Switches the object's state (+0xD4) unless locked (+0x60): state 0 re-arms from states 2/3,
// state 2 and 3 snapshot the position floats and reset the motion fields.
void fn_800E03F0(u8* arg0, s32 arg1) {
    s32 old = *(s32*)(arg0 + 0xD4);
    if (old != arg1 && *(u8*)(arg0 + 0x60) == 0) {
        if (arg1 == 0) {
            if ((u32)(old - 2) <= 1U) {
                *(u8*)(arg0 + 0x95) = 1;
                *(f32*)(arg0 + 0x50) = lbl_805FE8B0;
            }
        } else if (arg1 == 2) {
            f32 z = lbl_805FE8B0;
            *(f32*)(arg0 + 0x5C) = *(f32*)(arg0 + 0x4C);
            *(f32*)(arg0 + 0x64) = *(f32*)(arg0 + 0x68);
            *(f32*)(arg0 + 0xA4) = z;
            *(f32*)(arg0 + 0xA8) = z;
            *(f32*)(arg0 + 0xAC) = z;
            *(f32*)(arg0 + 0x50) = z;
            *(s32*)(arg0 + 0x54) = 0;
            *(u8*)(arg0 + 0x95) = 0;
            *(u8*)(arg0 + 0x78) = 0;
            *(u8*)(arg0 + 0x94) = 0;
            *(s32*)(arg0 + 0xB0) = 0;
            *(f32*)(arg0 + 0xB4) = z;
        } else if (arg1 == 3) {
            f32 a = *(f32*)(arg0 + 0x4C);
            f32 b = *(f32*)(arg0 + 0x68);
            f32 z = lbl_805FE8B0;
            *(f32*)(arg0 + 0x5C) = a;
            *(f32*)(arg0 + 0x64) = b;
            *(u8*)(arg0 + 0x95) = 0;
            *(u8*)(arg0 + 0x78) = 0;
            *(u8*)(arg0 + 0x94) = 0;
            *(s32*)(arg0 + 0xB0) = 0;
            *(f32*)(arg0 + 0xB4) = z;
        }
        *(s32*)(arg0 + 0xD4) = arg1;
    }
}

// 0x800E69EC
// Shows or hides the conversation HUD info: stores the flag byte, and when showing again cancels
// the pending hide animation first.
void fn_800E69EC(s32 arg0) {
    *(u8*)((u8*)lbl_805FB1C8 + 0x10) = (u8)arg0;
    if (arg0 == 0 && *(u8*)((u8*)lbl_805FB1C8 + 0x11) != 0) {
        *(u8*)((u8*)lbl_805FB1C8 + 0x11) = 0;
        u8* rec = (u8*)lbl_805FB1C8;
        UIElement_PlayUIAnim(0x24542991u, 0x9C543022u);
        *(f32*)(rec + 0x14) = lbl_805FEAF8;
        *(u8*)(rec + 0x11) = 0;
    }
    if (arg0 != 0) {
        ConversationHideHUDInfo();
    } else {
        ConversationShowHUDInfo();
    }
}

// 0x8010720C
// Returns the word at +4 of level `arg0`'s 0x20-byte level record, or 0 for ids >= 0x43.
u32 fn_8010720C(u32 arg0) {
    if (arg0 < 0x43u) {
        return *(u32*)(gLevelFlags + ((arg0 & 0xFF) << 5) + 0x4);
    }
    return 0;
}

// 0x80142898
// Removes `arg0` from the callback table (first match), shifting later entries down by one.
void fn_80142898(void* arg0) {
    s32 i = 0;
    s32 n = lbl_805FB458;

    if (n > 0) {
        for (; i < n; i++) {
            if (lbl_803F2830[i] == arg0) break;
        }
    }
    if (i < lbl_805FB458) {
        s32 last = lbl_805FB458 - 1;
        lbl_805FB458 = last;
        for (s32 k = i; k < last; k++) {
            lbl_803F2830[k] = lbl_803F2830[k + 1];
        }
    }
}

}  // extern "C"
