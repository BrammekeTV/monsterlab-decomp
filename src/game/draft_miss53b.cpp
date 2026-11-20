// Miscellaneous game-object constructors, state updaters and helpers (0x8008xxxx-0x800Axxxx).
// Verified against the asm with tools/dump.sh.
#include "types.h"

// Game globals defined in other files (C++ linkage, defining types).
extern "C++" void* gEventMgr;                  // 0x805FB6C8
extern "C++" s32   gZoneResourceSize;          // 0x805FB94C
extern "C++" u8    gWorldState[];              // 0x803E6D00
extern "C++" u8    lbl_8047C760[];             // effect manager
extern "C++" u8    lbl_803C4298[];             // object table (+0x38 used)
extern "C++" u8*   lbl_803C6A88[3];            // smasher table
extern "C++" u8    lbl_80346700[];             // s32 table indexed by RaySceneTest
extern "C++" f32   lbl_80346FC0[];             // float table (zeros)
extern "C++" f32   lbl_80346FD0[];             // float table (100.0f)
extern "C++" u8    lbl_8035B6C0[];             // vtable
extern "C++" u8    lbl_8035B9E8[];             // vtable
extern "C++" u8    lbl_8035BAE0[];             // vtable
extern "C++" u8    lbl_8035CBA8[];             // vtable
extern "C++" u8    lbl_8035CCA0[];             // vtable
extern "C++" u8    lbl_8035CDE8[];             // vtable

// fn_800C5E90 is defined in small_800C.cpp with C++ linkage, so it is declared outside extern "C".
u8*   fn_800C5E90(u8* p, s32 a, void* params, s32 b);            // 0x800C5E90 (base ctor)

extern "C" {
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);            // 0x800690D4 (base ctor)
void  fn_8008E878(u8* a, s32 b);                                 // 0x8008E878
void  fn_8008D278(u8* a, u8* b);                                 // 0x8008D278
void  fn_8008D698(u8* a, s32 b);                                 // 0x8008D698
s32   fn_8008BEE8(u8* a);                                        // 0x8008BEE8
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190
s32   RaySceneTest(void* ray);                                   // 0x80070458
BOOL  ZoneIsLoaded(void* world, s32 zone);                       // 0x8012F408
void  fn_801C83AC(u8* mgr, u8* item);                            // 0x801C83AC
void  fn_8020A398(s32 handle);                                   // 0x8020A398
void  fn_801ECFA0(u8* p, s32 idx);                               // 0x801ECFA0
u32   fn_801EC880(u8* p, u32 hash);                              // 0x801EC880
void  fn_801F61AC(u8* p, s32 a);                                 // 0x801F61AC
u8*   fn_801F9544(u8* p, s32 a4);                                // 0x801F9544
void  TextureRelease(void* tex);                                 // 0x80184028
void  UIElement_PlayUIAnim(u32 element, u32 anim);               // defined in character_states.cpp
f32   AnimGetFrame(void* anim, f32 fps);                         // 0x8018C014
}

extern "C" {

// 0x80085C34
void fn_80085C34(u8* arg0, s32 arg1) {
    s32 t = *(s32*)(arg0 + 0x288) - arg1;
    *(s32*)(arg0 + 0x288) = t;
    if (t > 0) {
        return;
    }
    if (*(u8*)(arg0 + 0x270) == 0) {
        *(f32*)(arg0 + 0x268) = *(f32*)(arg0 + 0x26C);
    }
    *(s32*)(arg0 + 0x288) = *(s32*)(lbl_80346700 + RaySceneTest(arg0) * 4);
}

// 0x80086780
void* fn_80086780(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(void**)p = lbl_8035B6C0;
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = -1;
    return obj;
}

// Ends a 3-stage sequence: each object in the table at +0x294 gets an event and a flag, then
// the state fields are cleared. Returns the element count it had on entry.
// 0x8008C204
s32 fn_8008C204(u8* arg0) {
    s32 count = *(s32*)(arg0 + 0x35C);
    u8* slot = arg0;
    s32 i = 0;
    s32 mode = 0;
    while (i < *(s32*)(arg0 + 0x35C)) {
        u8* obj = *(u8**)(slot + 0x294);
        fn_8008E878(obj, mode);
        fn_8008D278(obj, arg0 + 0x4C0);
        fn_8008D698(obj, (i & 1) ? 0 : 1);
        EventSend(gEventMgr, obj, NULL, 0xA97843C1u, 0, 0, 1, 0);
        mode += 2;
        slot += 4;
        i += 1;
    }
    *(s32*)(arg0 + 0x264) = 1;
    *(s32*)(arg0 + 0x35C) = 0;
    *(s32*)(arg0 + 0x370) = 0;
    *(s32*)(arg0 + 0x374) = 0;
    return count;
}

// 0x8008C9F8
void* fn_8008C9F8(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(void**)p = lbl_8035B9E8;
    *(s32*)(p + 0x264) = 0;
    *(f32*)(p + 0x268) = 0.0f;
    return obj;
}

// 0x8008CBB0
void* fn_8008CBB0(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(void**)p = lbl_8035BAE0;
    *(s32*)(p + 0x264) = -1;
    *(s32*)(p + 0x26C) = 1;
    *(u8*)(p + 0x274) = 0;
    *(f32*)(p + 0x278) = 0.0f;
    *(s32*)(p + 0x280) = 0;
    *(s32*)(p + 0x284) = 0;
    *(s32*)(p + 0x288) = 0;
    *(f32*)(p + 0x28C) = 0.0f;
    *(f32*)(p + 0x290) = 0.0f;
    *(f32*)(p + 0x294) = 0.0f;
    *(f32*)(p + 0x298) = 0.0f;
    *(f32*)(p + 0x29C) = 0.0f;
    *(f32*)(p + 0x2A0) = 0.0f;
    *(f32*)(p + 0x2A4) = 0.0f;
    *(f32*)(p + 0x2A8) = 0.0f;
    *(f32*)(p + 0x2AC) = 1.0f;
    *(f32*)(p + 0x2B0) = 0.0f;
    *(f32*)(p + 0x2B4) = 0.0f;
    *(u8*)(p + 0x2D0) = 0;
    *(f32*)(p + 0x2D4) = 1.0f;
    *(s32*)(p + 0x2D8) = 0;
    *(s32*)(p + 0x2DC) = 0;
    *(s32*)(p + 0x304) = 0;
    *(f32*)(p + 0x308) = 0.0f;
    *(u8*)(p + 0x310) = 0;
    return obj;
}

// Returns a state code for the object at +0x284: 0 when the phase at +0x270 is not 0, 1 default,
// 2 when the two timestamps differ, 3 when the child check passes and the flag at +0x2D0 is clear.
// 0x8008E290
s32 fn_8008E290(u8* arg0) {
    if (*(s32*)(arg0 + 0x270) != 0) {
        return 0;
    }
    u8* tgt = *(u8**)(arg0 + 0x284);
    if (tgt == NULL) {
        return 1;
    }
    s32 tgtVal = *(s32*)(tgt + 0x370);
    s32 cur = *(s32*)(arg0 + 0x268);
    if (cur != tgtVal && tgtVal != 0 && cur != 0) {
        return 2;
    }
    if (fn_8008BEE8(tgt) != 0 && *(u8*)(arg0 + 0x2D0) == 0) {
        return 3;
    }
    return 1;
}

// 0x800928F4
void fn_800928F4(u8* arg0) {
    if (*(u8**)(arg0 + 0xC) != NULL) {
        UIElement_PlayUIAnim(*(u32*)(arg0 + 0x10), 0xB729E3E6u);
        u8* tex = *(u8**)(arg0 + 0xC);
        s32 n = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = n;
        if (n == 0) {
            TextureRelease(tex);
        }
        *(u8**)(arg0 + 0xC) = NULL;
    }
    s32 h = *(s32*)(arg0 + 0x24);
    if (h != -1) {
        fn_8020A398(h);
        *(s32*)(arg0 + 0x24) = -1;
    }
    u8* sub = *(u8**)(arg0 + 0x4);
    if (sub != NULL) {
        u8* tbl = sub + 0x94;
        fn_801ECFA0(tbl, (s32)fn_801EC880(tbl, 0x53B643B8u));
        fn_801ECFA0(tbl, (s32)fn_801EC880(tbl, 0x37B4DA5Bu));
        fn_801ECFA0(tbl, (s32)fn_801EC880(tbl, 0x8B9BD519u));
    }
}

// 0x80092C40
void fn_80092C40(u8* arg0, s32 arg1) {
    if (*(s32*)(arg0 + 0x8) == 1) {
        EventSend(gEventMgr, *(void**)(arg0 + 0x4), NULL, 0xE495E9F9u, 0, 0, 1, 0);
    }
    *(s32*)(arg0 + 0x8) = (arg1 != 0) ? 4 : 5;
}

// 0x80093A88
void fn_80093A88(u8* arg0) {
    lbl_803C6A88[*(s32*)arg0] = NULL;
    u8* t14 = *(u8**)(arg0 + 0x14);
    *(s32*)(arg0 + 0x0) = -1;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = -1;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    if (t14 != NULL) {
        fn_801C83AC(lbl_8047C760, t14);
        *(u8**)(arg0 + 0x14) = NULL;
    }
    u8* t18 = *(u8**)(arg0 + 0x18);
    if (t18 != NULL) {
        fn_801C83AC(lbl_8047C760, t18);
        *(u8**)(arg0 + 0x18) = NULL;
    }
    *(u8*)(arg0 + 0x24) = 0;
    *(u8*)(arg0 + 0x1C) = 0;
    *(u8*)(arg0 + 0x1D) = 0;
    *(u8*)(arg0 + 0x1E) = 1;
}

// Sets the done flags at +0x1C/+0x1D once the animation frame reaches the stored frame.
// 0x80093B3C
void fn_80093B3C(u8* arg0, s32 arg1, f32 farg0) {
    *(u8*)(arg0 + 0x1C) = 0;
    *(u8*)(arg0 + 0x1D) = 0;
    if (*(s32*)(arg0 + 0x8) == 1 && *(u8*)(arg0 + 0x1E) == 1) {
        u8* sub = *(u8**)(arg0 + 0x4);
        f32 frame = AnimGetFrame(*(void**)(sub + 0xA0), 30.0f);
        if (frame >= (f32)*(s32*)(arg0 + 0x20)) {
            *(u8*)(arg0 + 0x1C) = 1;
            *(u8*)(arg0 + 0x1D) = (u8)(arg1 == 0);
            *(u8*)(arg0 + 0x1E) = 0;
        }
    }
}

// Adds v to the value at +0x298 and clamps it into the [lbl_80346FC0, lbl_80346FD0] range that
// RaySceneTest selects. The asm also leaves (new - old) in f1 on return; the existing
// declaration is void and callers ignore it.
// 0x800999B4
f32 fn_800999B4(u8* obj, f32 v) {
    f32 old = *(f32*)(obj + 0x298);
    *(f32*)(obj + 0x298) = old + v;
    s32 i1 = RaySceneTest(obj);
    f32 hi = lbl_80346FD0[i1];
    // The second call receives the first result still in r3.
    s32 i2 = RaySceneTest((void*)(uintptr_t)i1);
    f32 lo = lbl_80346FC0[i2];
    f32 cur = *(f32*)(obj + 0x298);
    f32 res;
    if (lo < hi) {
        if (cur < lo) {
            res = lo;
        } else if (cur > hi) {
            res = hi;
        } else {
            res = cur;
        }
    } else {
        if (cur < hi) {
            res = hi;
        } else if (cur > lo) {
            res = lo;
        } else {
            res = cur;
        }
    }
    *(f32*)(obj + 0x298) = res;
    return res - old;  // net change after clamping
}

// Finds the first object of type 0x11 in the loaded zones (the argument is not read).
// 0x8009A47C
void* fn_8009A47C(u8* p) {
    (void)p;
    u8* found = NULL;
    s32 zone = 0;
    while (zone <= *(s32*)(*(u8**)gWorldState + 0x4) && found == NULL) {
        if (ZoneIsLoaded(gWorldState, zone) != 0) {
            u8* zlist = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
            s32 idx = 0;
            s32 off = 0;
            while (idx < *(u8*)(zlist + 0xAC) && found == NULL) {
                u8* e = *(u8**)(*(u8**)(zlist + 0xA8) + off);
                if (*(s32*)(e + 0x8) == 0x11) {
                    found = e;
                }
                idx += 1;
                off += 4;
            }
        }
        zone += 1;
    }
    return found;
}

// 0x8009C2C8
void* fn_8009C2C8(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(void**)p = lbl_8035CBA8;
    *(s32*)(p + 0x264) = 0;
    fn_801F61AC(p + 0x268, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F9544(p + 0x324, *(s32*)(lbl_803C4298 + 0x38));
    *(f32*)(p + 0x3E8) = 0.0f;
    *(f32*)(p + 0x3EC) = 0.0f;
    *(u8*)(p + 0x3F0) = 0;
    *(f32*)(p + 0x3F4) = 1.0f;
    *(s32*)(p + 0x3F8) = 0;
    *(s32*)(p + 0x3FC) = 0;
    *(s32*)(p + 0x400) = 0;
    u8* src = *(u8**)(p + 0x4);
    *(f32*)(p + 0x3D8) = *(f32*)(src + 0xC);
    *(f32*)(p + 0x3DC) = *(f32*)(src + 0x10);
    *(f32*)(p + 0x3E0) = *(f32*)(src + 0x14);
    *(f32*)(p + 0x3E4) = *(f32*)(src + 0x18);
    return obj;
}

// 0x8009D834
void* fn_8009D834(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800C5E90(p, a, params, b);
    *(void**)p = lbl_8035CCA0;
    *(f32*)(p + 0x274) = 0.0f;
    *(u8*)(p + 0x278) = 0;
    return obj;
}

// 0x8009E164
void* fn_8009E164(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800C5E90(p, a, params, b);
    *(void**)p = lbl_8035CDE8;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 0.0f;
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x290) = 0.0f;
    *(f32*)(p + 0x294) = 12.0f;
    *(f32*)(p + 0x298) = 0.0f;
    *(s32*)(p + 0x29C) = -1;
    return obj;
}

typedef u8* (*PtrRetFn)(u8*);
typedef void (*VoidFn)(u8*);

// Tears down the three sub-object slots: each slot's object gets its virtual 0xC0 call and is
// cleared. The vtable at +0x0 is used to fetch the owned child before the flag at +0x8 is cleared.
// 0x800A424C
void fn_800A424C(u8* arg0) {
    if ((*(PtrRetFn*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* o = (*(PtrRetFn*)(*(u8**)arg0 + 0xC))(arg0);
        *(s32*)(o + 0x8) &= ~2;
    }
    u8* t4 = *(u8**)(arg0 + 0x4);
    *(s32*)(t4 + 0x20) &= ~2;
    *(u8*)(arg0 + 0x364) = 0;
    for (s32 i = 0; i < 3; i++) {
        u8* base = arg0 + 0x28 * i;
        for (s32 j = 0; j < *(s32*)(arg0 + 4 * i + 0x264); j++) {
            u8* obj = *(u8**)(base + 4 * j + 0x2E8);
            (*(VoidFn*)(*(u8**)obj + 0xC0))(obj);
            *(u8**)(base + 4 * j + 0x2E8) = NULL;
        }
    }
}

}  // extern "C"
