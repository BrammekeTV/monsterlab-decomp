// More small functions in 0x800Cxxxx (incl. stitching bug sounds).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
u8*   fn_80062CE0(u8* p, s32 del);                              // 0x80062CE0 (deleting dtor)
void  fn_8010B4C0(u8* obj, s32 v);                              // 0x8010B4C0
f32   fn_8010B05C(u8* obj, s32 i);                              // 0x8010B05C
void  fn_800C76F4(u8* p);                                       // 0x800C76F4
f32   fn_800C8300(u8* p);                                       // 0x800C8300
void  fn_800C8A74(u8* p);                                       // 0x800C8A74
void  fn_800712B4(void);                                        // 0x800712B4
void  fn_8020A398(s32 handle);                                  // 0x8020A398
s32   RaySceneTest(void* ray);                                  // 0x80070458
}

// Defined in small_800C.cpp without an extern "C" block, so they have C++ linkage.
u8* fn_800C2720(u8* p, s32 del);                                // 0x800C2720
extern "C" u8* fn_800C40C0(u8* p, s32 del);                                // 0x800C40C0

extern u8  lbl_8035ED08[];  // 0x8035ED08 (vtable)
extern s32 lbl_805FB178;    // 0x805FB178 (skitter sound user count)
extern s32 lbl_805F8C38;    // 0x805F8C38 (skitter sound handle, -1 = none)
extern s32 lbl_805FB17C;    // 0x805FB17C (eating sound user count)
extern s32 lbl_805F8C3C;    // 0x805F8C3C (eating sound handle, -1 = none)
extern f32 lbl_805FE3A0;    // 0x805FE3A0
extern f32 lbl_805FE440;    // 0x805FE440
extern f32 lbl_805FE460;    // 0x805FE460
extern f32 lbl_805FE47C;    // 0x805FE47C (lower clamp)
extern f32 lbl_805FE488;    // 0x805FE488 (upper clamp)
extern f32 lbl_803473E0[];  // 0x803473E0 (f32 table indexed by RaySceneTest)
extern f32 lbl_80347400[];  // 0x80347400 (f32 table indexed by RaySceneTest)
extern f32 lbl_80347410[];  // 0x80347410 (f32 table indexed by RaySceneTest)

extern "C" {

// 0x800C0AE8
// Returns 0 if any of the words at +4, +8, +0xC or +0x10 is zero. Otherwise returns 1 unless the word
// at +0 is -1 (sign bit of (~a | (a + 1))).
s32 fn_800C0AE8(u8* p) {
    if (*(s32*)(p + 0x04) == 0) return 0;
    if (*(s32*)(p + 0x08) == 0) return 0;
    if (*(s32*)(p + 0x0c) == 0) return 0;
    if (*(s32*)(p + 0x10) == 0) return 0;
    s32 a = *(s32*)(p + 0x00);
    return (s32)(((u32)(~a) | ((u32)a + 1u)) >> 31);
}

// 0x800C1798
// Copies 12 floats (0x38..0x64) of the entry selected by the index at +0x268 out of the table at +0x98
// (entries are 0xC8 bytes each) into out.
void fn_800C1798(void* out, void* arg) {
    u8* src = (u8*)arg;
    s32 idx = *(s32*)(src + 0x268);
    u8* entry = *(u8**)(src + 0x98) + idx * 0xC8;
    f32* dst = (f32*)out;
    for (s32 i = 0; i < 12; i++) {
        dst[i] = *(f32*)(entry + 0x38 + i * 4);
    }
}

// 0x800C1D44
// Calls virtual slot 0xC on p; if it returns non-null, calls it again and clears bit 0x2 of the
// returned object's word at +8. Then clears bit 0x2 of the word at +0x20 of the sub-object at +4
// and clears the flag bytes at +0x26C and +0x26D.
void fn_800C1D44(u8* p) {
    typedef u8* (*VFn)(u8*);
    if ((*(VFn*)(*(u8**)p + 0x0c))(p) != nullptr) {
        u8* rec = (*(VFn*)(*(u8**)p + 0x0c))(p);
        *(s32*)(rec + 0x08) &= ~0x2;
    }
    u8* sub = *(u8**)(p + 0x04);
    *(s32*)(sub + 0x20) &= ~0x2;
    *(u8*)(p + 0x26c) = 0;
    *(u8*)(p + 0x26d) = 0;
}

// 0x800C210C
// Deleting destructor: sets the vtable at +0xC and calls its slot 0x10 on p (base cleanup),
// then frees p if del > 0.
u8* fn_800C210C(u8* p, s32 del) {
    if (p) {
        *(u8**)(p + 0x0c) = lbl_8035ED08;
        void (*vfn)(u8*) = *(void (**)(u8*))(*(u8**)(p + 0x0c) + 0x10);
        vfn(p);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C4224
// Sums fn_8010B05C(list, i) over the list at +0x2A0 for i = 0 .. count-2, where count is the word at +0x24
// of that list (the last entry is skipped). Returns the total.
f32 fn_800C4224(u8* p) {
    f32 sum = lbl_805FE3A0;
    s32 i = 0;
    while (i != *(s32*)(*(u8**)(p + 0x2a0) + 0x24) - 1) {
        sum += fn_8010B05C(*(u8**)(p + 0x2a0), i);
        i++;
    }
    return sum;
}

// 0x800C42FC
// Deleting destructor: releases the sub-object at +0xD8 (fn_8010B4C0 with -1), base dtor fn_800C2720
// on p, then frees p if del > 0.
u8* fn_800C42FC(u8* p, s32 del) {
    if (p) {
        fn_8010B4C0(p + 0xd8, -1);
        fn_800C2720(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C6AF8
// Stitching Bug skitter sound stop: drops the user count (lbl_805FB178); when it reaches zero, stops the
// sound handle in lbl_805F8C38 if one is playing and resets the handle to -1.
void Stitching_Bug_StopSkitterSound(void* a0) {
    (void)a0;
    lbl_805FB178 = lbl_805FB178 - 1;
    if (lbl_805FB178 <= 0) {
        if (lbl_805F8C38 != -1) {
            fn_8020A398(lbl_805F8C38);
            lbl_805F8C38 = -1;
        }
    }
}

// 0x800C6BC4
// Stitching Bug eating sound stop: drops the user count (lbl_805FB17C); when it reaches zero, stops the
// sound handle in lbl_805F8C3C if one is playing and resets the handle to -1.
void Stitching_Bug_StopEatingSound(void* a0) {
    (void)a0;
    lbl_805FB17C = lbl_805FB17C - 1;
    if (lbl_805FB17C <= 0) {
        if (lbl_805F8C3C != -1) {
            fn_8020A398(lbl_805F8C3C);
            lbl_805F8C3C = -1;
        }
    }
}

// 0x800C6C08
// Deleting destructor: base dtor fn_80062CE0(p, 0) when p is non-null, then frees p if del > 0.
u8* fn_800C6C08(u8* p, s32 del) {
    if (p) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800C6CA4
// Deleting wrapper: frees p if p is non-null and del > 0 (no base dtor call).
u8* fn_800C6CA4(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x800C7408
// Returns the squared horizontal distance from pos to the reference point at obj->+4->+4 (x and z only,
// since y is set to the constant lbl_805FE440 first). Returns lbl_805FE460 when obj->+4 is null.
f32 fn_800C7408(u8* obj, f32* pos) {
    u8* a = *(u8**)(obj + 0x04);
    if (a == nullptr) {
        return lbl_805FE460;
    }
    u8* ref = *(u8**)(a + 0x04);
    f32 c = lbl_805FE440;
    f32 dx = pos[0] - *(f32*)(ref + 0x00);
    f32 dy = c - c;
    f32 dz = pos[2] - *(f32*)(ref + 0x08);
    pos[1] = c;
    return dx * dx + dy * dy + dz * dz;
}

// 0x800C75E8
// Stores the word v at +0x2F04 of p, then runs fn_800C76F4(p) and fn_800C8300(p).
void fn_800C75E8(u8* p, s32 v) {
    *(s32*)(p + 0x2f04) = v;
    fn_800C76F4(p);
    fn_800C8300(p);
}

// 0x800C7928
// Scans the 0x14-byte entries at p for the count at +0x1784; returns the address of the entry after the
// first one whose word at +0x24 is outside 2..5, or null if all are inside that range.
u8* fn_800C7928(u8* p) {
    s32 n = *(s32*)(p + 0x1784);
    for (s32 i = 0; i < n; i++) {
        s32 v = *(s32*)(p + i * 0x14 + 0x24);
        if ((u32)(v - 2) > 3u) {
            return p + i * 0x14 + 0x14;
        }
    }
    return nullptr;
}

// 0x800C7974
// Same scan as fn_800C7928 but with the count at +0x1788 and the entries' test word at +0x7F4; returns
// the address of the entry at +0x7E4 past the matching entry.
u8* fn_800C7974(u8* p) {
    s32 n = *(s32*)(p + 0x1788);
    for (s32 i = 0; i < n; i++) {
        s32 v = *(s32*)(p + i * 0x14 + 0x7f4);
        if ((u32)(v - 2) > 3u) {
            return p + i * 0x14 + 0x7e4;
        }
    }
    return nullptr;
}

// 0x800C82C0
// Deleting wrapper: frees p if p is non-null and del > 0 (no base dtor call).
u8* fn_800C82C0(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x800C8860
// Adds the table entry for RaySceneTest(p) to the float at +0, clamped to [lbl_805FE47C, lbl_805FE488],
// then calls fn_800712B4.
void fn_800C8860(u8* p) {
    s32 i = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x00) + lbl_803473E0[i];
    if (v < lbl_805FE47C) {
        v = lbl_805FE47C;
    } else if (v > lbl_805FE488) {
        v = lbl_805FE488;
    }
    *(f32*)(p + 0x00) = v;
    fn_800712B4();
}

// 0x800C88D0
// Runs fn_800C8A74(p), then returns the float at +4 of p.
f32 fn_800C88D0(u8* p) {
    fn_800C8A74(p);
    return *(f32*)(p + 0x04);
}

// 0x800C8984
// Adds the lbl_80347410 table entry for RaySceneTest(p) to the float at +0, clamped to
// [lbl_805FE47C, lbl_805FE488].
void fn_800C8984(u8* p) {
    s32 i = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x00) + lbl_80347410[i];
    if (v < lbl_805FE47C) {
        v = lbl_805FE47C;
    } else if (v > lbl_805FE488) {
        v = lbl_805FE488;
    }
    *(f32*)(p + 0x00) = v;
}

// 0x800C89F0
// Adds the lbl_80347400 table entry for RaySceneTest(p) to the float at +0, clamped to
// [lbl_805FE47C, lbl_805FE488].
void fn_800C89F0(u8* p) {
    s32 i = RaySceneTest(p);
    f32 v = *(f32*)(p + 0x00) + lbl_80347400[i];
    if (v < lbl_805FE47C) {
        v = lbl_805FE47C;
    } else if (v > lbl_805FE488) {
        v = lbl_805FE488;
    }
    *(f32*)(p + 0x00) = v;
}

// 0x800C8DF8
// Deleting destructor: base dtor fn_800C40C0(p, 0) when p is non-null, then frees p if del > 0.
u8* fn_800C8DF8(u8* p, s32 del) {
    if (p) {
        fn_800C40C0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

}  // extern "C"
