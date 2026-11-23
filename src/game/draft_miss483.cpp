#include "types.h"
#include "revolution.h"

extern "C" {
typedef u8* (*GetChildFn)(u8* self);
typedef void (*ChildDtorFn)(u8* self, s32 del);

s32   RaySceneTest(void* ray);
u8*   SaveDtor(u8* s, s32 del);                                 // defined in player_api.cpp
u32   StringHash(const char* s);                                // 0x801EB704
void  TextureRelease(void* tex);                                // 0x80184028
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count); // 0x8032F2E0
void  __div2i(u32 nhi, u32 nlo, u32 dhi, u32 dlo);              // 0x8032F678 (64-bit divide, result unused here)
void  fn_80023160(void);
u8*   fn_80062CE0(u8* p, s32 del);                              // 0x80062CE0
void  fn_8008378C(u8* a3);                                      // 0x8008378C
void  fn_800ABA44(void* p);                                     // 0x800ABA44
u8* fn_800D7FB0(u8* p, s32 del);                              // 0x800D7FB0 (return unused)
void fn_800D9044(u8* p, f32 delta);                            // 0x800D9044 (defined in draft_miss481.cpp)
s32   fn_801098DC(s32 a0);                                      // 0x801098DC
s32   fn_80109A0C(s32 id);                                      // 0x80109A0C
u8* fn_8010FD2C(u8* p, s32 del);                              // 0x8010FD2C (return unused)
void  fn_801A904C(u8* p);                                       // 0x801A904C
u8*   fn_801B161C(u8* p, u32 key);                              // 0x801B161C
void  fn_801C83AC(u8* mgr, u8* item);                           // 0x801C83AC
u8*   fn_8021824C(u8* lab, s32 del);                            // 0x8021824C
void  fn_8020EB44(u8* p, s32 del);                              // 0x8020EB44
s32   fn_80338A2C(const char* a, const char* b, u32 n);        // 0x80338A2C (strncmp)
s32   fn_8033861C(const char* s, const char* fmt, ...);         // 0x8033861C (sscanf)
void  operator_delete(void* p);                                 // 0x801D426C

extern u8  lbl_8035D950[];   // 0x8035D950 (vtable)
extern u8  lbl_8036D158[];   // 0x8036D158 (vtable)
extern u8  lbl_8036D258[];   // 0x8036D258 (vtable)
extern u8  lbl_8047C760[];   // 0x8047C760 (effect manager)
extern f32 lbl_805FDF98;     // 0x805FDF98
extern f32 lbl_805FCCD4;     // 0x805FCCD4
extern f32 lbl_805FCCD8;     // 0x805FCCD8
extern f32 lbl_805FCF74;     // 0x805FCF74
extern f32 lbl_803464C8[];   // 0x803464C8 (float table indexed by RaySceneTest)
extern const char lbl_805F9708[];  // 0x805F9708 ("default")
extern const char lbl_80385F40[];  // 0x80385F40 ("emphasis")
extern const char lbl_805F9710[];  // 0x805F9710 (scan format "%x")
}

extern "C" {

// 0x8011CB0C
// Walks the entry table (stride 0x1C, count at +0x12F0): releases the texture at +0x30 and the handle at +0x34.
void fn_8011CB0C(u8* arg0) {
    s32 i;
    u8* e = arg0;

    for (i = 0; i < *(s32*)(arg0 + 0x12F0); i++, e += 0x1C) {
        u8* tex = *(u8**)(e + 0x30);
        if (tex != NULL) {
            s32 refs = *(s32*)(tex + 0x24) - 1;
            *(s32*)(tex + 0x24) = refs;
            if (refs == 0) {
                TextureRelease(tex);
            }
            *(u8**)(e + 0x30) = NULL;
        }
        s32 h = *(s32*)(e + 0x34);
        if (h != 0) {
            if (fn_80109A0C(h) == 0) {
                fn_801098DC(h);
            }
            *(s32*)(e + 0x34) = 0;
        }
    }
}

// 0x801A8860
// Clears the flag at +0 of every element in the list at +0x178 and drops the list; stores the string hash of arg1 at +0x140.
s32 fn_801A8860(u8* arg0, const char* arg1) {
    if (*(u8**)(arg0 + 0x178) != NULL) {
        fn_801A904C(arg0);
        u8* list = *(u8**)(arg0 + 0x178);
        if (list != NULL) {
            for (s32 i = 0; i < *(s32*)(list + 0xC); i++) {
                u8* elem = *(u8**)(list + 0x14 + i * 4);
                if (elem != NULL) {
                    *(s32*)(elem + 0x74) = 0;
                }
            }
        }
        fn_801C83AC(lbl_8047C760, *(u8**)(arg0 + 0x178));
        *(u8**)(arg0 + 0x178) = NULL;
    }
    *(u32*)(arg0 + 0x140) = StringHash(arg1);
    return 0;
}

// 0x80007A8C
// Deleting destructor: base-destroys the sub-objects at +0xD5F0, +0xD570, +0xC470 and the array at +0xAB98, frees if del > 0. Returns p.
u8* fn_80007A8C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        fn_800D7FB0(arg0 + 0xD5F0, -1);
        fn_8021824C(arg0 + 0xD570, -1);
        if (arg0 + 0x5C8 != NULL) {
            SaveDtor(arg0 + 0xC470, -1);
            if (arg0 + 0xFD8 != NULL) {
                __destroy_arr(arg0 + 0xAB98, (void (*)(void*, s32))fn_8020EB44, 0x13C, 0x14);
            }
        }
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80083BD4
// Per-frame update: clears the byte at +0x284, copies the float at +0x288 into the child returned by virtual slot +0x10,
// then divides the float at +0x24 of the record found via fn_801B161C by the table entry picked by RaySceneTest.
void fn_80083BD4(u8* arg0) {
    fn_8008378C(arg0);
    *(u8*)(arg0 + 0x284) = 0;

    u8* child = (*(GetChildFn*)(*(u8**)arg0 + 0x10))(arg0);
    if (child != NULL) {
        *(f32*)(child + 0xB0) = *(f32*)(arg0 + 0x288);
    }

    u8* w = *(u8**)(arg0 + 0x8C);
    u8* base;
    if (*(s32*)(w + 0x10) == 0) {
        base = NULL;
    } else {
        base = *(u8**)(*(u8**)(w + 0xC));
    }

    u8* rec = fn_801B161C(base, 0x17C7F120u);
    s32 idx = RaySceneTest(rec);
    u8* body = *(u8**)(rec + 0x8);
    *(f32*)(body + 0x24) = *(f32*)(body + 0x24) / lbl_803464C8[idx];
}

// 0x800AAF7C
// Deleting destructor: sets the vtable, zeroes six floats at +0x264..+0x278, runs fn_800ABA44, base-destroys with fn_80062CE0, frees if del > 0. Returns p.
u8* fn_800AAF7C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        f32 zero = lbl_805FDF98;
        *(void**)(arg0 + 0x0) = lbl_8035D950;
        *(f32*)(arg0 + 0x264) = zero;
        *(f32*)(arg0 + 0x268) = zero;
        *(f32*)(arg0 + 0x26C) = zero;
        *(f32*)(arg0 + 0x270) = zero;
        *(f32*)(arg0 + 0x274) = zero;
        *(f32*)(arg0 + 0x278) = zero;
        fn_800ABA44(arg0);
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800D90D4
// Maps the point at +0x4 along the segment record selected by index +0x10 (stride 0x1C) of the table at +0x8,
// scaled by the ratio of the float at +0x14 to the segment's length at +0x18.
void fn_800D90D4(u8* arg0) {
    fn_800D9044(arg0, 0.0f);  // f1 is not set before the call in the asm (stale): pass 0

    u8* out = *(u8**)(arg0 + 0x4);
    u8* e = *(u8**)(arg0 + 0x8) + *(s32*)(arg0 + 0x10) * 0x1C;
    f32 scale = *(f32*)(arg0 + 0x14) / *(f32*)(e + 0x18);

    *(f32*)(out + 0x0) = *(f32*)(e + 0x0) + *(f32*)(e + 0xC) * scale;
    *(f32*)(out + 0x4) = *(f32*)(e + 0x4) + *(f32*)(e + 0x10) * scale;
    *(f32*)(out + 0x8) = *(f32*)(e + 0x8) + *(f32*)(e + 0x14) * scale;
}

// 0x8011A474
// Deleting destructor: sets the vtable to lbl_8036D158, calls virtual slot +0x8 (arg 1) on each of the four children at +0x8..+0x14 and clears them,
// base-destroys with fn_8010FD2C, frees if del > 0. Returns p. (Slots are 4-byte in the original layout.)
u8* fn_8011A474(u8* arg0, s32 del) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x0) = lbl_8036D158;
        for (s32 i = 0; i < 4; i++) {
            u8* child = *(u8**)(arg0 + 0x8 + i * 4);
            if (child != NULL) {
                (*(ChildDtorFn*)(*(u8**)child + 0x8))(child, 1);
                *(void**)(arg0 + 0x8 + i * 4) = NULL;
            }
        }
        fn_8010FD2C(arg0, 0);
        if (del > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8011D280
// Same as fn_8011A474 with the vtable lbl_8036D258.
u8* fn_8011D280(u8* arg0, s32 del) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x0) = lbl_8036D258;
        for (s32 i = 0; i < 4; i++) {
            u8* child = *(u8**)(arg0 + 0x8 + i * 4);
            if (child != NULL) {
                (*(ChildDtorFn*)(*(u8**)child + 0x8))(child, 1);
                *(void**)(arg0 + 0x8 + i * 4) = NULL;
            }
        }
        fn_8010FD2C(arg0, 0);
        if (del > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80142744
// Elapsed-time helper: the 64-bit timestamp at +0 (hi) / +4 (lo) is reset to now if it is less than 60000000 ticks away from now,
// then set to now; the elapsed ticks times 8 go to __div2i with divisor (bus clock / 4) * 2^50 / 1e6 style, result unused.
void fn_80142744(u8* arg0) {
    u64 now = (u64)OSGetTime();
    u64 stamp = ((u64)*(u32*)(arg0 + 0x0) << 32) | *(u32*)(arg0 + 0x4);

    if (stamp - now < 60000000ull) {
        stamp = now;
    }
    u64 dt = now - stamp;

    *(u32*)(arg0 + 0x4) = (u32)now;
    *(u32*)(arg0 + 0x0) = (u32)(now >> 32);

    u64 scaled = dt << 3;
    u32 divisor = (u32)((((u64)0x431BDE83u * (u64)(*(u32*)0x800000F8 >> 2)) >> 32) >> 15);
    __div2i((u32)(scaled >> 32), (u32)scaled, 0, divisor);
}

// 0x801EFF50
// Parses the text at arg1 into the word at +0x8: "default" -> 0x01010101, "emphasis" -> 0x01010102, otherwise sscanf "%x".
void fn_801EFF50(u8* arg0, const char* arg1) {
    s32 v;

    if (fn_80338A2C(arg1, lbl_805F9708, 7) == 0) {
        *(s32*)(arg0 + 0x8) = 0x01010101;
        return;
    }
    if (fn_80338A2C(arg1, lbl_80385F40, 8) == 0) {
        *(s32*)(arg0 + 0x8) = 0x01010102;
        return;
    }
    v = 0;
    fn_8033861C(arg1, lbl_805F9710, &v);
    *(s32*)(arg0 + 0x8) = v;
}

// 0x80030C20
// Trigger check for the point at arg1 against the link transform at +0x4: latches the flag at +0x38 and returns 1 when the
// x offset is above lbl_805FCCD4 (flag at +0x20 set) and below lbl_805FCCD8. Returns 0 if already latched.
s32 fn_80030C20(u8* arg0, u8* arg1) {
    s32 hit = 0;
    s32 inside = 0;

    if (*(s32*)(arg0 + 0x38) != 0) {
        return 0;
    }
    u8* t = *(u8**)(arg0 + 0x4);
    f32 dx = *(f32*)(arg1 + 0x0) - *(f32*)(t + 0x68);

    if (*(u8*)(arg0 + 0x20) != 0 && dx > lbl_805FCCD4) {
        inside = 1;
    }
    if (inside != 0 && dx < lbl_805FCCD8) {
        hit = 1;
    }
    if (hit != 0) {
        *(s32*)(arg0 + 0x38) = 1;
    }
    return hit;
}

// 0x80039BA0
// Latches the flag at +0x6DC once the float at +0x6C is below lbl_805FCF74 * (float at +0x78); if two or fewer of the
// slots +0x600/+0x628/+0x650/+0x678 are filled, calls fn_80023160 first.
void fn_80039BA0(u8* arg0) {
    if (*(u8*)(arg0 + 0x6DC) == 0 && *(f32*)(arg0 + 0x6C) < *(f32*)(arg0 + 0x78) * lbl_805FCF74) {
        s32 n = 0;
        if (*(s32*)(arg0 + 0x600) == 0) n = 1;
        if (*(s32*)(arg0 + 0x628) == 0) n += 1;
        if (*(s32*)(arg0 + 0x650) == 0) n += 1;
        if (*(s32*)(arg0 + 0x678) == 0) n += 1;
        if (n <= 2) {
            fn_80023160();
        }
        *(u8*)(arg0 + 0x6DC) = 1;
    }
}

}  // extern "C"
