// Small functions from draft miss511 (fixed against the asm).
#include "types.h"
#include "game/part.h"

// Virtual slot signatures used by the objects below.
typedef s32 (*VFnS32)(u8* self);
typedef u8* (*VFnPtr)(u8* self);
typedef void (*VFnMode)(u8* self, s32 mode);

// Sends mode 2 through the slot-0x30 virtual of the sub-vtable stored at +0x138.
static void SetObjMode2(u8* obj) {
    (*(VFnMode*)(*(u8**)(obj + 0x138) + 0x30))(obj, 2);
}

extern "C" {
void operator_delete(void* p);                                              // 0x801D426C
s32 PartGetCurrentHealth(const PartInstance* p);                            // 0x8021EF68
void PartSetHealth(PartInstance* p, s32 hp);                                // 0x8021D8EC
void* ScreenSetsCurrent(void* sets);                                        // 0x8018D1E4
void* ScreenSetFindElement(void* set, u32 hash);                            // 0x80198F18
u32 StringHash(const char* s);                                              // 0x801EB704
s32 RaySceneTest(void* ray);                                                // 0x80070458
void __register_global_object(void* obj, void (*dtor)(void*), void* handle); // 0x8032EFC8
void fn_80023160(void);                                                     // 0x80023160
void fn_8006FAF4(u8* p, s32 a);                                             // 0x8006FAF4
u8* fn_80146EEC(u8* p);                                                     // 0x80146EEC
void fn_80146FB8(u8* p);                                                    // 0x80146FB8
void fn_80147130(u8* p);                                                    // 0x80147130
void fn_801A2510(u8* el);                                                   // 0x801A2510
void fn_801F5540(void);                                                     // 0x801F5540
u8* fn_801F5D34(u8* p, s32 del);                                            // 0x801F5D34
u8* fn_801F5D8C(u8* p, s32 del);                                            // 0x801F5D8C
u8* fn_801F5DE4(u8* p, s32 del);                                            // 0x801F5DE4
u8* fn_801F6238(u8* p, s32 del);                                            // 0x801F6238
u8* fn_801F9598(u8* p, s32 del);                                            // 0x801F9598
void fn_80087720(u8* p);                                        // 0x80087720
void fn_8020E950(u8* p);                                                    // 0x8020E950
void fn_8023A5B0(u8* p);                                                    // 0x8023A5B0
void fn_8023A68C(u8* p);                                                    // 0x8023A68C
s32 fn_802E6528(s32* args, s32 a, s32 b, s32 c, s32 d);                     // 0x802E6528
void fn_802E9CBC(s32 counter);                                              // 0x802E9CBC
s32 fn_802EB534(void);                                                      // 0x802EB534

extern "C++" void* gScreenSets;                                             // 0x805FB654
extern const f32 lbl_803460B8[];                                            // 0x803460B8
extern u8 lbl_8035B830[];                                                   // 0x8035B830 (vtable)
extern u8 lbl_804FB718[];                                                   // 0x804FB718 (global object)
extern u8 lbl_80556AB0[];                                                   // 0x80556AB0 (lock)
extern s32 lbl_805FB994;                                                    // 0x805FB994
extern u32 lbl_805FB9F0;                                                    // 0x805FB9F0
extern f32 lbl_805FBB3C;                                                    // 0x805FBB3C
extern f32 lbl_80600480;                                                    // 0x80600480 (float constant)
extern f32 lbl_805FE178;                                                    // 0x805FE178 (float constant, 0.0)
extern f32 lbl_805FCC20;                                                    // 0x805FCC20 (float constant)
extern f32 lbl_805FCC3C;                                                    // 0x805FCC3C (float constant)
}

extern "C" {

// 0x80030184
// Sets the flag at +0xF7C and calls fn_80023160 once, when the value at +0x6C is inside
// (0, +0x78 * k) and the bits of (+0xF60 ^ 5) give a negative test.
void fn_80030184(u8* p) {
    f32 pos = *(f32*)(p + 0x6C);
    s32 x = *(s32*)(p + 0xF60) ^ 5;
    s32 bits = (x >> 1) - (x & 5);
    if (pos > lbl_805FCC20 && pos < *(f32*)(p + 0x78) * lbl_805FCC3C &&
        *(u8*)(p + 0xF7C) == 0 && bits < 0) {
        fn_80023160();
        *(u8*)(p + 0xF7C) = 1;
    }
}

// 0x80037EA0
// Clears the "AnimatedBackground" element if it is in the current screen set, zeroes +0x14C, and sends
// mode 2 to the object at +0x154.
void fn_80037EA0(u8* p) {
    u32 hash = StringHash("AnimatedBackground");
    void* el = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), hash);
    if (el != NULL) fn_801A2510((u8*)el);
    u8* obj = *(u8**)(p + 0x154);
    *(s32*)(p + 0x14C) = 0;
    SetObjMode2(obj);
}

// 0x8004DDEC
// For each of the five part slots at +0xC4C that is set: runs fn_8020E950 on it and sets the matching
// part at +0x89C + 4*i to the part's current health.
void fn_8004DDEC(u8* p) {
    if (*(u8**)(p + 0xC4C) != NULL) {
        s32 i;
        for (i = 0; i < 5; i++) {
            u8* el = *(u8**)(*(u8**)(p + 0xC4C) + i * 4);
            if (el != NULL) {
                fn_8020E950(el);
                PartSetHealth((PartInstance*)*(u8**)(p + 0x89C + i * 4),
                              PartGetCurrentHealth((const PartInstance*)*(u8**)el));
            }
        }
    }
}

// 0x800A5C2C
// Sets the two flag bytes at +0x264/+0x265, ORs 2 into the record returned by the slot-0xC virtual (when
// the slot-0xC check says yes), ORs 2 into +0x20 of the sub-object at +4, and ORs 0x600000 into +0x15C.
void fn_800A5C2C(u8* p) {
    *(u8*)(p + 0x264) = 1;
    *(u8*)(p + 0x265) = 1;
    if ((*(VFnS32*)(*(u8**)p + 0xC))(p) != 0) {
        u8* rec = (*(VFnPtr*)(*(u8**)p + 0xC))(p);
        *(s32*)(rec + 0x8) |= 2;
    }
    u8* sub = *(u8**)(p + 0x4);
    *(s32*)(sub + 0x20) |= 2;
    *(s32*)(p + 0x15C) |= 0x600000;
}

// 0x800B4874
// Sends mode 2 to the four objects held at +0xA38, +0xA3C, +0xA40 and +0xA44.
void fn_800B4874(u8* p) {
    SetObjMode2(*(u8**)(p + 0xA38));
    SetObjMode2(*(u8**)(p + 0xA3C));
    SetObjMode2(*(u8**)(p + 0xA40));
    SetObjMode2(*(u8**)(p + 0xA44));
}

// 0x800B6510
// Drains +0x2F4 by delta = m * farg0, where m is farg1 or, when farg1 is the 0 constant, a table value
// picked by RaySceneTest. Adds delta to +0x2F0 and clamps +0x2F4 at 0.
void fn_800B6510(u8* p, f32 farg0, f32 farg1) {
    f32 m = farg1;
    if (farg1 == lbl_805FE178) {
        m = lbl_803460B8[RaySceneTest(p)];
    }
    f32 delta = m * farg0;
    f32 left = *(f32*)(p + 0x2F4) - delta;
    *(f32*)(p + 0x2F4) = left;
    *(f32*)(p + 0x2F0) = *(f32*)(p + 0x2F0) + delta;
    if (left < lbl_805FE178) {
        *(f32*)(p + 0x2F4) = lbl_805FE178;
    }
}

// 0x801EBE74
// Binary search over the table at *(p+0): count at +4, 8-byte entries of (u32 key, s32 value).
// Returns the value stored for key, or def when the key is not in the table.
s32 fn_801EBE74(u8* p, u32 key, s32 def) {
    s32 count = *(s32*)(p + 0x4);
    if (count != 0) {
        u8* entries = *(u8**)p;
        s32 lo = 0;
        s32 hi = count - 1;
        s32 idx = -1;
        for (;;) {
            s32 mid = (lo + hi) / 2;
            u32 k = *(u32*)(entries + mid * 8);
            if (key == k) {
                idx = mid;
                break;
            }
            if (key < k) {
                hi = mid - 1;
            } else {
                lo = mid + 1;
            }
            if (lo > hi) break;
        }
        if (idx >= 0) {
            return *(s32*)(entries + idx * 8 + 4);
        }
    }
    return def;
}

// 0x801F5CAC
// Sets the shared handle to -1, initializes the three global objects inside lbl_804FB718 and registers
// their destructors.
void fn_801F5CAC(void) {
    lbl_805FB9F0 = (u32)-1;
    fn_80146EEC(lbl_804FB718 + 0x10);
    __register_global_object(lbl_804FB718 + 0x10, (void (*)(void*))fn_801F5D34, lbl_804FB718);
    fn_80146FB8(lbl_804FB718 + 0xD4);
    __register_global_object(lbl_804FB718 + 0xD4, (void (*)(void*))fn_801F5D8C, lbl_804FB718 + 0xC8);
    fn_80147130(lbl_804FB718 + 0x110);
    __register_global_object(lbl_804FB718 + 0x110, (void (*)(void*))fn_801F5DE4, lbl_804FB718 + 0x100);
}

// 0x80208C1C
// Takes the lock lbl_80556AB0 when the byte at +0xB0 is 0, runs fn_802E9CBC on the result of fn_802EB534,
// releases the lock if it was taken, sends a zeroed two-word block to fn_802E6528, and clears two globals.
void fn_80208C1C(u8* p) {
    s32 took = 0;
    s32 args[2];
    if (*(u8*)(p + 0xB0) == 0) {
        fn_8023A5B0(lbl_80556AB0);
        took = 1;
    }
    fn_802E9CBC(fn_802EB534());
    if (took != 0) {
        fn_8023A68C(lbl_80556AB0);
    }
    args[0] = 0;
    args[1] = 0;
    // r5..r7 are not set in this function (stale), so they are passed as 0.
    fn_802E6528(args, 8, 0, 0, 0);
    fn_801F5540();
    lbl_805FB994 = 0;
    lbl_805FBB3C = lbl_80600480;
}

// 0x80086F00
// Deleting destructor: stores the vtable at +4, runs the base cleanups (fn_80087720, fn_801F9598 on +0x730,
// fn_801F6238 on +0x3C0, fn_8006FAF4), then frees the object if del > 0. Returns p.
u8* fn_80086F00(u8* p, s32 del) {
    if (p != NULL) {
        *(void**)(p + 0x4) = lbl_8035B830;
        fn_80087720(p);
        fn_801F9598(p + 0x730, -1);
        fn_801F6238(p + 0x3C0, -1);
        fn_8006FAF4(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

}  // extern "C"
