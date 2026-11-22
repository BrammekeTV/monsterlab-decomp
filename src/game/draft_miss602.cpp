// Small functions in 0x8008xxxx-0x800Bxxxx (ray-scene checks, UI setup, event broadcast, vector helpers).
#include "types.h"
#include <math.h>

// Script/event slot helper type: a 16-byte scoped lock used around virtual calls.
struct ScopedLock { u8 pad[4]; };

// Helper types for the vtable and dispatch calls in this file.
typedef void (*ObjMethod14Fn)(void* obj, s32 arg);
typedef u8* (*GetPtrFn)(void* self);
typedef u8* (*SetFloatsFn)(void* self, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);
typedef s32 (*DispatchFn)(void* obj, s32 id);
typedef f32 (*GetFloatFn)(void* self);
typedef void (*VecOutFn)(void* out, void* self, f32 t);

extern "C++" u8 gWorldState[];
extern "C++" s32 gZoneResourceSize;
extern "C++" void* gEventMgr;
extern "C++" void* lbl_805FBBAC;

extern "C" {
void    ScopedLockCtor(ScopedLock* l);                                                   // 0x801F5384
void    ScopedLockDtor(ScopedLock* l, s32 flags);                                        // 0x801F53B0
void    EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
s32     RaySceneTest(void* ray);                                                         // 0x80070458
void    UISetNumber(void* el, s32 n);                                                    // 0x8019F2A8
void    UISetText(void* el, const char* text, s32 a);                                    // 0x8019EFC8
void    UISetProgress(void* obj, f32 t);                                                 // 0x801A3FA0
void*   UIWidgetFindChild(void* widget, u32 hash);                                      // 0x80199AEC
void    UIElement_PlayUIAnim(u32 element, u32 anim);                                     // 0x801903CC
s32     PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);                          // 0x80209E70
u32     RandomU32(void);                                                                 // 0x80143460
void    MemFreeSmall(void* p);                                                           // 0x801D4340
void    operator_delete(void* p);                                                        // 0x801D426C
u8*     fn_80062CE0(u8* p, s32 del);                                                     // 0x80062CE0
void    fn_80070460(u8* p);                                                              // 0x80070460
void*   fn_80070F4C(void);                                                               // 0x80070F4C
void    fn_80070FAC(u8* p, f32 v);                                                       // 0x80070FAC
void    fn_800712BC(u8* p, s32 a, s32 b);                                                // 0x800712BC
s32     fn_801A403C(u8* a3, f32 f1);                                                     // 0x801A403C
s32     fn_801A4050(u8* a3, f32 f1);                                                     // 0x801A4050
void*   fn_801D40E0(s32 n);                                                              // 0x801D40E0
void*   fn_80209814(void* v);                                                            // 0x80209814
void*   fn_8009A47C(u8* p);                                                              // 0x8009A47C
u8*     fn_8009EC0C(u8* a3);                                                             // 0x8009EC0C
void    fn_801C83AC(u8* mgr, u8* item);                                                  // 0x801C83AC
u8*     fn_801B3308(void);                                                               // 0x801B3308
void    fn_801B3378(u8* p, u8* key);                                                     // 0x801B3378
void    fn_80162990(u8* p, s32 h);                                                       // 0x80162990
void    fn_801458B0(f32* quat, f32* a, f32* b);                                          // 0x801458B0
BOOL    fn_80147554(s32 button);                                                         // 0x80147554
f32     fn_800C3E84(u8* a);                                                              // 0x800C3E84
void    fn_800C324C(u8* a, u8* b, f32 c);                                                // 0x800C324C
void    fn_800AD29C(s32 a, u8* b);                                                       // 0x800AD29C
u8*     fn_801DA788(u8* a3, s32 i);                                                      // 0x801DA788
void    fn_8009C4EC(u8* arg0);                                                           // 0x8009C4EC

extern u8  lbl_80348F38[];   // 0x80348F38 (dispatch table)
extern u8  lbl_803C4298[];   // 0x803C4298
extern u8  lbl_8040E5F0[];   // 0x8040E5F0 (frame effects object)
extern u8  lbl_8047C760[];   // 0x8047C760 (effect manager)
extern u8  lbl_8035E378[];   // 0x8035E378 (vtable)
extern const char lbl_805F8A28[];  // 0x805F8A28 (text)
extern s32 lbl_80346908[];   // 0x80346908
extern s32 lbl_80346918[];   // 0x80346918
extern s32 lbl_80346928[];   // 0x80346928
extern s32 lbl_80346938[];   // 0x80346938
extern s32 lbl_80346948[];   // 0x80346948
extern f32 lbl_80346208[];   // 0x80346208
extern f32 lbl_80346218[];   // 0x80346218
extern f32 lbl_80347200[];   // 0x80347200
extern f32 lbl_805FDA90;     // 0x805FDA90
extern f32 lbl_805FDAA8;     // 0x805FDAA8
extern f32 lbl_805FDB28;     // 0x805FDB28
extern f32 lbl_805FDB68;     // 0x805FDB68
extern f32 lbl_805FDC50;     // 0x805FDC50
extern f32 lbl_805FDD18;     // 0x805FDD18
extern f32 lbl_805FDD1C;     // 0x805FDD1C
extern f32 lbl_805FDD24;     // 0x805FDD24
extern f32 lbl_805FDF98;     // 0x805FDF98
extern f32 lbl_805FDFB4;     // 0x805FDFB4
extern f32 lbl_805FE070;     // 0x805FE070
extern f32 lbl_805FE074;     // 0x805FE074
extern f32 lbl_805FE108;     // 0x805FE108
extern f32 lbl_805FE11C;     // 0x805FE11C
extern f32 lbl_805FE148;     // 0x805FE148
extern f64 lbl_805FE150;     // 0x805FE150 (double)
extern f32 lbl_805FE158;     // 0x805FE158
extern f32 lbl_805FE170;     // 0x805FE170
extern f32 lbl_805FE178;     // 0x805FE178
extern f32 lbl_805FE17C;     // 0x805FE17C

// Clamp a RaySceneTest result to the 0..3 table range.
static s32 ClampIdx3(s32 v) {
    if (v < 0) return 0;
    if (v > 3) return 3;
    return v;
}

// Reinterprets a 32-bit return value as a float (the dispatch call hands back raw bits).
static f32 S32BitsToF32(s32 v) {
    union { s32 i; f32 f; } u;
    u.i = v;
    return u.f;
}

// Sets the four flag bits the render/update code uses on a sub-object at +0x20.
static void SetSubFlags(u8* sub) {
    *(u32*)(sub + 0x20) |= 0x8000;
    *(u32*)(sub + 0x20) |= 0x10000000;
    *(u32*)(sub + 0x20) |= 0x4000;
    *(u32*)(sub + 0x20) |= 0x10000000;
}

// Returns the zone record: *(gWorldState)->0x1A4 plus gZoneResourceSize bytes.
static u8* ZoneRecord(void) {
    return *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
}

// True for the three message hashes that trigger the EventSend in fn_8008FEAC.
static s32 IsEventKey(s32 v) {
    if (v == 0x0C898A89) return 1;
    if (v < 0x0C898A89) return v == (s32)0xB4CA5278U;
    return v == 0x5DB90DD2;
}

// Maps the 0..3 index to the button id used by fn_800B5CB0.
static s32 ButtonIdForIndex(s32 idx) {
    if (idx == 0) return 0x1089;
    if (idx == 1) return 0x108A;
    if (idx == 2) return 0x109C;
    return 0x109B;
}

// Sends an event to the 0x90 object, every entry in the 0x9C array (count at 0x1B4), and the 0x94 object.
// Each object gets a method-0x14 call with arg 1 under a scoped lock first.
static void BroadcastSlotEvents(u8* self, u32 event) {
    u8* a = *(u8**)(self + 0x90);
    if (a != NULL) {
        ScopedLock lk;
        ScopedLockCtor(&lk);
        (*(ObjMethod14Fn*)(*(u8**)a + 0x14))(a, 1);
        ScopedLockDtor(&lk, -1);
        EventSend(gEventMgr, a, NULL, event, 0, 0, 1, 0);
    }
    for (s32 i = 0; i < *(s32*)(self + 0x1B4); i++) {
        u8* o = *(u8**)(self + 0x9C + i * 4);
        if (o != NULL) {
            ScopedLock lk2;
            ScopedLockCtor(&lk2);
            (*(ObjMethod14Fn*)(*(u8**)o + 0x14))(o, 1);
            ScopedLockDtor(&lk2, -1);
            EventSend(gEventMgr, o, NULL, event, 0, 0, 1, 0);
        }
    }
    u8* b = *(u8**)(self + 0x94);
    if (b != NULL) {
        ScopedLock lk3;
        ScopedLockCtor(&lk3);
        (*(ObjMethod14Fn*)(*(u8**)b + 0x14))(b, 1);
        ScopedLockDtor(&lk3, -1);
        EventSend(gEventMgr, b, NULL, event, 0, 0, 1, 0);
    }
}

// Dispatches through the table at lbl_80348F38: the object's type byte at +0xC picks the
// entry, and the function at entry+0x30 is called with (sub-object at +0x8, id).
static s32 DispatchByType(u8* d, s32 id) {
    u8* obj = *(u8**)(d + 0x8);
    u32 type = *(u8*)(d + 0xC);
    return ((DispatchFn)*(void**)(lbl_80348F38 + type * 4 + 0x30))(obj, id);
}

// 0x8008B264
// Checks the ray-scene chain of up to five hits against the flag tables; returns 1 if blocked.
s32 fn_8008B264(u8* arg0, s32 arg1) {
    s32 c0 = ClampIdx3(RaySceneTest(arg0));
    if (lbl_80346908[c0] > 0 && arg1 != 2) return 1;
    s32 c1 = ClampIdx3(RaySceneTest((void*)(uintptr_t)c0));
    if (lbl_80346918[c1] > 0 && arg1 != 3) return 1;
    s32 c2 = ClampIdx3(RaySceneTest((void*)(uintptr_t)c1));
    if (lbl_80346928[c2] > 0 && arg1 != 4) return 1;
    s32 c3 = ClampIdx3(RaySceneTest((void*)(uintptr_t)RaySceneTest((void*)(uintptr_t)c2)));
    if (lbl_80346938[c3] > 0) return 1;
    s32 c4 = ClampIdx3(RaySceneTest((void*)(uintptr_t)c3));
    return lbl_80346948[c4] > 0;
}

// 0x8008F660
// Resets the HUD widgets and plays the four UI animations; the bar progress comes from two ray lookups.
void fn_8008F660(u8* arg0) {
    fn_800712BC(arg0, (s32)*(f32*)(arg0 + 0x7E8), 0);
    fn_80070FAC(arg0, lbl_805FDAA8);
    u8* w = (u8*)fn_80070F4C();
    if (w != NULL) {
        UISetNumber(UIWidgetFindChild(w, 0x57EFBC28U), 0);
        UISetText(UIWidgetFindChild(w, 0xCEE6ED92U), lbl_805F8A28, 1);
        UISetNumber(UIWidgetFindChild(w, 0xB9E1DD04U), 0);
        u8* bar = (u8*)UIWidgetFindChild(w, 0x5A8F51E9U);
        s32 r1 = RaySceneTest((void*)(uintptr_t)fn_801A403C(bar, lbl_805FDA90));
        f32 f1v = (r1 <= 3 && r1 >= 0) ? lbl_80347200[r1] : lbl_80347200[0];
        s32 r2 = RaySceneTest((void*)(uintptr_t)fn_801A4050(bar, f1v));
        f32 f2v = (r2 <= 3 && r2 >= 0) ? lbl_80347200[r2] : lbl_80347200[0];
        UISetProgress(bar, f2v);
    }
    UIElement_PlayUIAnim(0x7DBA57FFU, 0x3D242667U);
    UIElement_PlayUIAnim(0x90252014U, 0x3D242667U);
    UIElement_PlayUIAnim(0xA1CD3A89U, 0x3D242667U);
    UIElement_PlayUIAnim(0x07BA313DU, 0x3D242667U);
}

// 0x8008FEAC
// Walks the zone's entity list and sends the 0xF15918A6 event to entities whose message key matches,
// then creates the voice SFX handle once and plays the cue.
void fn_8008FEAC(void) {
    u8* z0 = ZoneRecord();
    s32 total = *(s32*)(z0 + 0x94) + *(s32*)(z0 + 0x98);
    for (s32 i = 0; i < total; i++) {
        u8* z = ZoneRecord();
        u8* ent;
        u8* rec;
        if (i < *(s32*)(z + 0x94)) {
            u8** arr = *(u8***)(z + 0x120);
            ent = arr[i];
            rec = *(u8**)(z + 0x8C) + i * 0x40;
        } else {
            s32 k = i - *(s32*)(z + 0x94);
            ent = *(u8**)(*(u8**)(z + 0x12C) + k * 4);
            rec = *(u8**)(z + 0x90) + k * 0x40;
        }
        if ((*(s32*)(rec + 0x1C) & 0x40) && ent != NULL) {
            u8* p9c = *(u8**)(ent + 0x9C);
            s32 key = (p9c != NULL) ? *(s32*)(p9c + 0x8) : 0;
            if (IsEventKey(key)) {
                EventSend(gEventMgr, ent, NULL, 0xF15918A6U, 0, 0, 1, 0);
            }
        }
    }
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(0x58D10FEFU, 0, 0, 0, 0, 0);
}

// 0x80094FB8
// Broadcasts event 0xCA90298E to the slot objects, then sets the value at +0x358 as the state word.
void fn_80094FB8(u8* arg0) {
    fn_80070460(arg0);
    BroadcastSlotEvents(arg0, 0xCA90298EU);
    f32 f2 = *(f32*)(arg0 + 0x358);
    fn_800712BC(arg0, (s32)f2, (s32)(f2 <= lbl_805FDB68));
}

// 0x8009588C
// Broadcasts event 0xC608C842 to the slot objects and clears the float at +0x8C.
void fn_8009588C(u8* arg0) {
    BroadcastSlotEvents(arg0, 0xC608C842U);
    *(f32*)(arg0 + 0x8C) = lbl_805FDB28;
}

// 0x80095A3C
// Broadcasts event 0x117741C3 to the slot objects and clears the float at +0x8C.
void fn_80095A3C(u8* arg0) {
    BroadcastSlotEvents(arg0, 0x117741C3U);
    *(f32*)(arg0 + 0x8C) = lbl_805FDB28;
}

// 0x8009875C
// Computes the four floats of the target vector from the dispatch table and writes the offset result
// (vector difference) into the sub-object at +0x5C..+0x68.
void fn_8009875C(u8* arg0, u8* arg1) {
    u8* t = fn_801DA788((*(GetPtrFn*)(*(u8**)arg0 + 0xC))(arg0), 0);
    u8* obj = *(u8**)(t + 0x10);
    u8* d = *(u8**)(obj + 0x70);
    f32 A = S32BitsToF32(DispatchByType(d, 0x135));
    f32 B = S32BitsToF32(DispatchByType(d, 0x136));
    f32 C = S32BitsToF32(DispatchByType(d, 0x137));
    f32 D = S32BitsToF32(DispatchByType(d, 0x138));
    f32 dx = *(f32*)(arg1 + 0x0) - A;
    f32 dy = *(f32*)(arg1 + 0x4) - B;
    f32 dz = *(f32*)(arg1 + 0x8) - C;
    f32 dw = *(f32*)(arg1 + 0xC) - D;
    u8* t2 = fn_801DA788((*(SetFloatsFn*)(*(u8**)arg0 + 0xC))(arg0, dz, *(f32*)(arg1 + 0x8), dy, *(f32*)(arg1 + 0x4), dx, D), 0);
    *(f32*)(t2 + 0x5C) = dx;
    *(f32*)(t2 + 0x60) = dy;
    *(f32*)(t2 + 0x64) = dz;
    *(f32*)(t2 + 0x68) = dw;
}

// 0x8009A354
// Resets the state of an object: clears two floats, runs two notify callbacks, frees the item list,
// and clears the three pending item slots.
void fn_8009A354(u8* arg0) {
    f32 z = lbl_805FDC50;
    *(f32*)(arg0 + 0x1C0) = z;
    *(f32*)(arg0 + 0x90) = z;
    *(f32*)(arg0 + 0x94) = z;
    ((void (*)(void*))*(void**)(*(u8**)(arg0 + 0x170) + 0x10))(arg0 + 0x9C);
    ((void (*)(void*))*(void**)(*(u8**)(arg0 + 0x18C) + 0x10))(arg0 + 0x180);
    fn_8009C4EC(*(u8**)(arg0 + 0x198));
    *(void**)(arg0 + 0x198) = NULL;
    u8* arr = *(u8**)(arg0 + 0x19C);
    if (arr != NULL) {
        for (s32 i = 0; i < *(s32*)(arg0 + 0x1A0); i++) {
            u8* e = *(u8**)(arr + i * 4);
            *(u8*)(e + 0x270) = 0;
        }
        MemFreeSmall(*(void**)(arg0 + 0x19C));
        *(void**)(arg0 + 0x19C) = NULL;
        *(s32*)(arg0 + 0x1A0) = 0;
    }
    *(u8*)(*(u8**)(lbl_803C4298 + 4) + 0xFC) = 0;
    fn_8009EC0C((u8*)fn_8009A47C(arg0));
    for (s32 i = 0; i < 3; i++) {
        *(s32*)(arg0 + 0x1A4 + i * 4) = 0;
        u8* item = *(u8**)(arg0 + 0x1B0 + i * 4);
        if (item != NULL) {
            fn_801C83AC(lbl_8047C760, item);
            *(void**)(arg0 + 0x1B0 + i * 4) = NULL;
        }
    }
}

// 0x8009BBCC
// Picks one free entry of the list at +0x10 (count at +0x14), at random, and gives it a new
// position from arg1's method 0xD0 (float) and 0xD8 (vector); then sets its flag bits.
void fn_8009BBCC(u8* arg0, u8* arg1, u8* arg2) {
    s32 count = *(s32*)(arg0 + 0x14);
    s32 freeCount = 0;
    for (s32 i = 0; i < count; i++) {
        u8* e = *(u8**)(*(u8**)(arg0 + 0x10) + i * 4);
        if (*(u8*)(e + 0x270) == 0) freeCount++;
    }
    u32 rnd = RandomU32();
    s32 pick = (freeCount > 0) ? (s32)(rnd % (u32)freeCount) : (s32)rnd;
    for (s32 k = 0; k < count; k++) {
        u8* e = *(u8**)(*(u8**)(arg0 + 0x10) + k * 4);
        if (*(u8*)(e + 0x270) != 0) continue;
        if (pick > 0) {
            pick--;
            continue;
        }
        f32 t = (*(GetFloatFn*)(*(u8**)arg1 + 0xD0))(arg1);
        f32 c = fn_800C3E84(arg2);
        fn_800C324C(arg2, e, t + (c - t));
        f32 tmp[4];
        (*(VecOutFn*)(*(u8**)arg1 + 0xD8))(tmp, arg1, t);
        u8* sub = *(u8**)(e + 0x4);
        *(f32*)(sub + 0xC) = tmp[0];
        *(f32*)(sub + 0x10) = tmp[1];
        *(f32*)(sub + 0x14) = tmp[2];
        *(f32*)(sub + 0x18) = tmp[3];
        SetSubFlags(*(u8**)(e + 0x4));
        return;
    }
}

// 0x8009C4EC
// Resets the sub-object transform and flags, rebuilds two matrices, and releases three handles.
void fn_8009C4EC(u8* arg0) {
    *(s32*)(arg0 + 0x264) = 0;
    fn_801B3378(fn_801B3308(), arg0 + 0x268);
    fn_801B3378(fn_801B3308(), arg0 + 0x324);
    *(f32*)(arg0 + 0x3E8) = lbl_805FDD18;
    *(f32*)(arg0 + 0x3EC) = lbl_805FDD24;
    *(u8*)(arg0 + 0x3F0) = 0;
    *(f32*)(arg0 + 0x3F4) = lbl_805FDD1C;
    u8* sub = *(u8**)(arg0 + 0x4);
    *(f32*)(sub + 0xC) = *(f32*)(arg0 + 0x3D8);
    *(f32*)(sub + 0x10) = *(f32*)(arg0 + 0x3DC);
    *(f32*)(sub + 0x14) = *(f32*)(arg0 + 0x3E0);
    *(f32*)(sub + 0x18) = *(f32*)(arg0 + 0x3E4);
    SetSubFlags(*(u8**)(arg0 + 0x4));
    if (*(s32*)(arg0 + 0x3F8) != 0) {
        fn_80162990(lbl_8040E5F0, *(s32*)(arg0 + 0x3F8));
        *(s32*)(arg0 + 0x3F8) = 0;
    }
    if (*(s32*)(arg0 + 0x3FC) != 0) {
        fn_80162990(lbl_8040E5F0, *(s32*)(arg0 + 0x3FC));
        *(s32*)(arg0 + 0x3FC) = 0;
    }
    if (*(s32*)(arg0 + 0x400) != 0) {
        fn_80162990(lbl_8040E5F0, *(s32*)(arg0 + 0x400));
        *(s32*)(arg0 + 0x400) = 0;
    }
}

// 0x800AA8C0
// Picks one of six slot objects at random among those whose flag at +0x264 is clear, and calls fn_800AD29C on it.
void fn_800AA8C0(u8* arg0) {
    s32 freeCount = 0;
    for (s32 k = 0; k < 6; k++) {
        if (*(s32*)(*(u8**)(arg0 + 0xFC + k * 4) + 0x264) == 0) freeCount++;
    }
    if (freeCount > 0) {
        u32 pick = RandomU32() % (u32)freeCount;
        for (s32 k = 0; k < 6; k++) {
            if (*(s32*)(*(u8**)(arg0 + 0xFC + k * 4) + 0x264) == 0) {
                if (pick == 0) {
                    fn_800AD29C(*(s32*)(arg0 + 0x98 + k * 4), *(u8**)(arg0 + 0xFC + k * 4));
                    return;
                }
                pick--;
            }
        }
    }
}

// 0x800AB468
// Moves the sub-object by arg1 times a constant along the direction from the reference point at +0x270..+0x278
// (normalized), keeping the old position in +0x27C..+0x284, and sets the sub-object flags.
void fn_800AB468(u8* arg0, f32 arg1) {
    f32 dx = *(f32*)(arg0 + 0x264) - *(f32*)(arg0 + 0x270);
    f32 dy = *(f32*)(arg0 + 0x268) - *(f32*)(arg0 + 0x274);
    f32 dz = *(f32*)(arg0 + 0x26C) - *(f32*)(arg0 + 0x278);
    u8* sub = *(u8**)(arg0 + 0x4);
    f32 p0 = *(f32*)(sub + 0x0);
    f32 p1 = *(f32*)(sub + 0x4);
    f32 p2 = *(f32*)(sub + 0x8);
    f32 s = lbl_805FDFB4 * arg1;
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 nx;
    f32 ny;
    f32 nz;
    if (len2 == lbl_805FDF98) {
        nx = lbl_805FDF98;
        ny = lbl_805FDF98;
        nz = lbl_805FDF98;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        nx = dx * inv;
        ny = dy * inv;
        nz = dz * inv;
    }
    *(f32*)(arg0 + 0x27C) = p0;
    *(f32*)(arg0 + 0x280) = p1;
    *(f32*)(arg0 + 0x284) = p2;
    *(f32*)(sub + 0x0) = p0 + nx * s;
    *(f32*)(sub + 0x4) = p1 + ny * s;
    *(f32*)(sub + 0x8) = p2 + nz * s;
    SetSubFlags(*(u8**)(arg0 + 0x4));
}

// 0x800AFF8C
// Aims the sub-object at arg1: builds a rotation quaternion from the direction and up vector and stores it at +0xC..+0x18.
void fn_800AFF8C(u8* arg0, u8* arg1) {
    f32 Z = lbl_805FE070;
    f32 O = lbl_805FE074;
    u8* sub = *(u8**)(arg0 + 0x4);
    f32 dx = *(f32*)(sub + 0x0) - *(f32*)(arg1 + 0x0);
    f32 dy = *(f32*)(sub + 0x4) - *(f32*)(arg1 + 0x4);
    f32 dz = *(f32*)(sub + 0x8) - *(f32*)(arg1 + 0x8);
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 dir[3];
    if (len2 == Z) {
        dir[0] = Z;
        dir[1] = Z;
        dir[2] = Z;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        dir[0] = dx * inv;
        dir[1] = dy * inv;
        dir[2] = dz * inv;
    }
    f32 up[3];
    up[0] = Z;
    up[1] = Z;
    up[2] = O;
    f32 quat[4];
    quat[0] = Z;
    quat[1] = Z;
    quat[2] = Z;
    quat[3] = O;
    fn_801458B0(quat, dir, up);
    u8* sub2 = *(u8**)(arg0 + 0x4);
    *(f32*)(sub2 + 0xC) = quat[0];
    *(f32*)(sub2 + 0x10) = quat[1];
    *(f32*)(sub2 + 0x14) = quat[2];
    *(f32*)(sub2 + 0x18) = quat[3];
    SetSubFlags(*(u8**)(arg0 + 0x4));
}

// 0x800B4070
// Finds how many whole steps of lbl_80346218 fit in the span from the table at lbl_80346208 for arg0's hit;
// on success writes a signed step count (scaled by the sign of t) to *arg1 and returns 1.
s32 fn_800B4070(u8* arg0, s32* arg1, f32 t) {
    s32 i0 = ClampIdx3(RaySceneTest(arg0));
    f32 f31 = lbl_80346208[i0];
    s32 i1 = ClampIdx3(RaySceneTest((void*)(uintptr_t)i0));
    f32 f3 = lbl_80346218[i1];
    f32 v = (f32)((f64)fabsf(lbl_805FE148 * t) - lbl_805FE150 * (f64)f31);
    s32 count = 0;
    s32 flag = 0;
    if (v <= lbl_805FE108) {
        flag = 1;
    } else {
        while (v > lbl_805FE108) {
            v = v - f3;
            if (v > lbl_805FE108) count++;
            if (v < f31) flag = 1;
            v = v - f31;
        }
    }
    if (flag == 0) return 0;
    f32 sg = lbl_805FE108;
    if (lbl_805FE108 != t) {
        sg = (t >= lbl_805FE108) ? lbl_805FE11C : lbl_805FE158;
    }
    *arg1 = (s32)((f32)count * sg);
    return 1;
}

// 0x800B5CB0
// Advances the four-step input sequence (state 2): tracks which button ids are held and moves the
// sequence index forward on a match, or resets the state.
void fn_800B5CB0(u8* arg0) {
    if (*(s32*)arg0 != 2) return;
    s32 held = 0;
    s32 anyHeld = 0;
    for (s32 r27 = 0; r27 < 4; r27++) {
        s32 id = ButtonIdForIndex(r27);
        if (fn_80147554(id + *(s32*)(lbl_803C4298 + 0x38) * 16)) {
            held = ButtonIdForIndex(r27);
            if (anyHeld == 0) anyHeld = 1;
        }
    }
    if (*(f32*)(arg0 + 0x2C) <= lbl_805FE170) {
        *(s32*)arg0 = 4;
        return;
    }
    if (anyHeld == 0) return;
    s32 idx = *(s32*)(arg0 + 0x30);
    if (held == *(s32*)(arg0 + 4 + idx * 4)) {
        s32 nidx = idx + 1;
        *(s32*)(arg0 + 0x30) = nidx;
        if (nidx >= 10 || *(s32*)(arg0 + 4 + nidx * 4) == 0) {
            *(f32*)(arg0 + 0x2C) = lbl_805FE170;
            *(s32*)arg0 = 3;
        }
    } else {
        *(f32*)(arg0 + 0x2C) = lbl_805FE170;
        *(s32*)arg0 = 4;
    }
}

// 0x800B5F20
// Initializes an object: sets its vtable, copies its position into the sub-object, and resets the
// motion state. If arg1 > 0 the object is freed afterwards. Returns arg0.
u8* fn_800B5F20(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)arg0 = (void*)lbl_8035E378;
        u8* sub = *(u8**)(arg0 + 0x4);
        *(f32*)(sub + 0x0) = *(f32*)(arg0 + 0x264);
        *(f32*)(sub + 0x4) = *(f32*)(arg0 + 0x268);
        *(f32*)(sub + 0x8) = *(f32*)(arg0 + 0x26C);
        SetSubFlags(*(u8**)(arg0 + 0x4));
        *(f32*)(arg0 + 0x27C) = lbl_805FE178;
        *(f32*)(arg0 + 0x280) = lbl_805FE178;
        *(f32*)(arg0 + 0x284) = lbl_805FE178;
        *(f32*)(arg0 + 0x288) = lbl_805FE17C;
        *(f32*)(arg0 + 0x2C4) = lbl_805FE178;
        *(f32*)(arg0 + 0x2CC) = lbl_805FE178;
        *(f32*)(arg0 + 0x2D0) = lbl_805FE178;
        *(s32*)(arg0 + 0x2B0) = 0;
        *(s32*)(arg0 + 0x2B8) = -1;
        *(s32*)(arg0 + 0x2C0) = -1;
        *(s32*)(arg0 + 0x2F8) = -1;
        *(s32*)(arg0 + 0x2FC) = -1;
        *(s32*)(arg0 + 0x300) = -1;
        *(s32*)(arg0 + 0x2B4) = 0;
        u8* sub2 = *(u8**)(arg0 + 0x4);
        *(f32*)(sub2 + 0xC) = lbl_805FE178;
        *(f32*)(sub2 + 0x10) = *(f32*)(arg0 + 0x280);
        *(f32*)(sub2 + 0x14) = *(f32*)(arg0 + 0x284);
        *(f32*)(sub2 + 0x18) = *(f32*)(arg0 + 0x288);
        SetSubFlags(*(u8**)(arg0 + 0x4));
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

}  // extern "C"
