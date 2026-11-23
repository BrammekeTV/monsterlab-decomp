// Misc functions: sound slot allocation, save banner, rumble pause, part slot checks, vector helpers.
#include "types.h"
#include "game/part.h"
#include "system/sys.h"
#include <math.h>
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32   fn_801ABD84(u8* rec, s32 key);                              // 0x801ABD84
void  fn_801B72A0(u8* anim, u8* target);                          // 0x801B72A0
s32   fn_801B82A8(u8* p, u8* a4);                                 // 0x801B82A8
u8*   fn_801724D4(s32 flag);                                      // 0x801724D4
u8*   fn_80174524(u8* src);                                       // 0x80174524
void fn_80174AC4(u8* p);                                  // 0x80174AC4
BOOL  fn_80147554(s32 button);                                    // 0x80147554
s32   fn_801481FC(s32 chan);                                      // 0x801481FC
void  fn_801FB624(void);                                          // 0x801FB624
u32   fn_801FCA24(void);                                          // 0x801FCA24
void  fn_802030A8(void);                                          // 0x802030A8
BOOL  fn_801F5220(void);                                          // 0x801F5220
s32   fn_801F581C(void);                                          // 0x801F581C
void  fn_801F5724(u32 button);                                    // 0x801F5724
void  fn_801F5814(s32 a);                                         // 0x801F5814
void  fn_801F5794(s32 a);                                         // 0x801F5794
void  fn_801F5734(void* arg);                                     // 0x801F5734
void  fn_801F51DC(s32 flag);                                      // 0x801F51DC
u8*   fn_801FA7CC(u8* p, s32 a4);                                 // 0x801FA7CC
s32   NANDInitBanner(u8* p, s32 a, s16* name, s16* comment);         // 0x80267F44
void  fn_80206E40(const char* name, u8* dst, s32 w, s32 h, s32 size, s32 fmt);  // 0x80206E40
void  fn_80206B6C(s32 code, const u8* msg);                       // 0x80206B6C
s32   fn_80265C18(const char* path, s32 mode, s32 a);            // 0x80265C18
s32   NANDOpen(const char* name, u8* handle, s32 flags);      // 0x80266BA4
s32   fn_80266060(u8* h, void* buf, u32 n);                       // 0x80266060
s32   NANDClose(u8* handle);                                    // 0x80266E24
int   SysSprintf(char* buf, const char* fmt, ...);                // 0x80142EF8
BOOL  PartAllowedAtRank(void* coll, s32 defIndex, s32 tier);     // 0x802191CC

extern u8   lbl_80559D78[];         // 0x80559D78 (0x28-byte voice records)
extern s32  lbl_805FB6B8;           // 0x805FB6B8
extern s32  lbl_803861A8[];         // 0x803861A8 (3 columns x 4 rows of s32/f32 defaults)
extern u8   lbl_803861D8[];         // 0x803861D8 (vtable)
extern u8   lbl_80386C10[];         // 0x80386C10 (save error messages)
extern const char lbl_80386E98[];   // 0x80386E98 ("wiisave/banner")
extern const char lbl_80386EA8[];   // 0x80386EA8 (icon file name format)
extern u8   lbl_805271E0[];         // 0x805271E0 (save file buffer)
extern u8   lbl_805FBB28;           // 0x805FBB28
extern void* lbl_805FB964;          // 0x805FB964
extern s32  lbl_805FB970;           // 0x805FB970
extern s32  lbl_805FB974;           // 0x805FB974
extern s32  lbl_805FB978;           // 0x805FB978
extern s32  lbl_805FBA30;           // 0x805FBA30
extern void (*lbl_805FBA1C)(void);  // 0x805FBA1C
extern s8   lbl_805FBA34;           // 0x805FBA34
}
extern "C++" u8* lbl_805FB608;      // 0x805FB608: heap-like object; +0 alloc fn
extern "C++" void* gLab;            // 0x805FBBC8

typedef s32 (*VBoolFn)(void* self);
typedef u8* (*AllocFn)(s32 size, s32 a, s32 b, s32 c, s32 d, u8* e);

extern "C" {

// 0x802099B8
// Allocates a 0x28-byte voice record (first free slot of 256) for a sound; returns handle 0x0AB00000|index or -1.
s32 fn_802099B8(u8* arg0, s32 arg1, u8* arg2, s32 arg3) {
    u8* r = lbl_80559D78;
    s32 i = 0;
    while (i < 0x100 && *(s32*)r != 0) {
        r += 0x28;
        i += 1;
    }
    if (i >= 0x100) return -1;
    u8* rec = lbl_80559D78 + i * 0x28;
    *(s32*)rec = (s32)(intptr_t)arg0;  // lint:ok  (32-bit slot in the original layout)
    *(s32*)(rec + 0x1C) = arg3;
    if (arg1 != 0) {
        s32 sp8 = lbl_805FB6B8;
        fn_801B72A0((u8*)&sp8, (u8*)(intptr_t)arg1);
        *(s32*)(rec + 0x4) = sp8;
        *(s32*)(rec + 0x24) = 2;
    } else if (arg2 != NULL) {
        *(f32*)(rec + 0x8) = *(f32*)(arg2 + 0x0);
        *(f32*)(rec + 0xC) = *(f32*)(arg2 + 0x4);
        *(f32*)(rec + 0x10) = *(f32*)(arg2 + 0x8);
        *(s32*)(rec + 0x24) = 1;
    } else {
        *(s32*)(rec + 0x24) = 0;
    }
    *(s32*)(rec + 0x14) = -1;
    return i | 0x0AB00000;
}

// 0x801ABF0C
// Finds, in a table of 0x1C-byte records (array at +0x10, count at +0x14), the first record with key
// `hash` for which fn_801ABD84(rec, key) is nonzero; returns it or NULL.
u8* fn_801ABF0C(u8* table, u32 hash, s32 key) {
    s32 count = *(s32*)(table + 0x14);
    u8* rec = NULL;
    for (s32 i = 0; i < count; i++) {
        u8* base = *(u8**)(table + 0x10);
        if (hash == *(u32*)(base + i * 0x1C)) {
            rec = base + i * 0x1C;
            break;
        }
    }
    while (rec != NULL) {
        if (fn_801ABD84(rec, key) != 0) return rec;
        u8* base = *(u8**)(table + 0x10);
        s32 n = *(s32*)(table + 0x14);
        s32 j = (s32)((rec - base) / 0x1C) + 1;
        rec = NULL;
        for (; j < n; j++) {
            if (hash == *(u32*)(base + j * 0x1C)) {
                rec = base + j * 0x1C;
                break;
            }
        }
    }
    return NULL;
}

// 0x801FB760
// Home-button check: if any pad pressed the home button (0x108B + 0x1000*chan, or +0x1E on chan type 8),
// pauses audio and rumble, runs the home menu loop (fn_801FB624), then unpauses rumble.
void fn_801FB760(void) {
    if (lbl_805FBA30 != 0 || SysGetRequestedExitMode() != 0) return;
    if (fn_801FCA24() != 0) return;
    s32 pressed = 0;
    s32 button = 0x108B;
    for (s32 chan = 0; chan < 4; chan++) {
        s32 type = fn_801481FC(chan);
        if (type != 0 && fn_80147554(button) != 0) pressed = 1;
        if (type == 8 && fn_80147554(button + 0x1E) != 0) pressed = 1;
        button += 0x1000;
    }
    if (pressed == 0) return;
    if (lbl_805FBA1C != NULL) lbl_805FBA1C();
    fn_802030A8();
    for (s32 c = 0; c < 4; c++) PauseRumble(c);
    lbl_805FBA34 = 1;
    fn_801FB624();
    lbl_805FBA34 = 0;
    for (s32 c = 0; c < 4; c++) UnpauseRumble(c);
}

// 0x801F7800
// Constructor: base init, vtable, default floats at +0xAC..+0xC4, zeroes +0xF8..+0x120, and copies the
// 4x3 default table lbl_803861A8 into +0xC8..+0xF4.
u8* fn_801F7800(u8* p, s32 a4) {
    fn_801FA7CC(p, a4);
    *(u8**)p = lbl_803861D8;
    *(f32*)(p + 0xAC) = 0.5f;
    *(f32*)(p + 0xB0) = 1.0f;
    *(u8*)(p + 0xB4) = 0;
    *(f32*)(p + 0xB8) = 0.2f;
    *(f32*)(p + 0xBC) = 0.4f;
    *(f32*)(p + 0xC0) = 0.25f;
    *(f32*)(p + 0xC4) = 0.05f;
    *(u8*)(p + 0xF8) = 0;
    for (s32 off = 0xFC; off <= 0x120; off += 4) *(s32*)(p + off) = 0;
    s32* t = lbl_803861A8;
    *(s32*)(p + 0xC8) = t[0];
    *(f32*)(p + 0xD4) = *(f32*)&t[3];
    *(s32*)(p + 0xE0) = t[6];
    *(f32*)(p + 0xEC) = *(f32*)&t[9];
    *(s32*)(p + 0xCC) = t[1];
    *(f32*)(p + 0xD8) = *(f32*)&t[4];
    *(s32*)(p + 0xE4) = t[7];
    *(f32*)(p + 0xF0) = *(f32*)&t[10];
    *(s32*)(p + 0xD0) = t[2];
    *(f32*)(p + 0xDC) = *(f32*)&t[5];
    *(s32*)(p + 0xE8) = t[8];
    *(f32*)(p + 0xF4) = *(f32*)&t[11];
    return p;
}

// 0x801B8184
// Callback keyed on the type byte at +0x7F: 0x20 -> fn_801B82A8; 0x10/0x08 -> +0x2C (for 0x10, 3 when the
// extent from +0x00 to +0x0C is tiny on all axes; the extent is stored at +0x18); 0x40 -> 3; else 0.
s32 fn_801B8184(u8* p, u8* a4) {
    switch (*(u8*)(p + 0x7F)) {
    case 0x20:
        return fn_801B82A8(p, a4);
    case 0x10:
    case 0x8:
        if (*(u8*)(p + 0x7F) == 0x10) {
            f32 dx = *(f32*)(p + 0xC) - *(f32*)(p + 0x0);
            f32 dy = *(f32*)(p + 0x10) - *(f32*)(p + 0x4);
            f32 dz = *(f32*)(p + 0x14) - *(f32*)(p + 0x8);
            *(f32*)(p + 0x18) = dx;
            *(f32*)(p + 0x1C) = dy;
            *(f32*)(p + 0x20) = dz;
            if (dx <= 0.0001f && dx >= -0.0001f && dy <= 0.0001f && dy >= -0.0001f &&
                dz <= 0.0001f && dz >= -0.0001f)
                return 3;
            return *(s32*)(p + 0x2C);
        }
        return *(s32*)(p + 0x2C);
    case 0x40:
        return 3;
    }
    return 0;
}

// 0x80206F14
// Builds the Wii save-file banner: widens the two titles to UTF-16 (s16 per char), then loads the banner
// texture (0xC0x0x40) and 7 icon textures, marking each icon in the 2-bit-per-icon field at +0x8.
void fn_80206F14(u8* p, const char* title, const char* subtitle) {
    s16 wTitle[0x100];
    s16 wSub[0x100];
    char name[0x80];
    s16* d = wTitle;
    while (*title != 0) *d++ = (s16)(s8)*title++;
    *d = 0;
    d = wSub;
    while (*subtitle != 0) *d++ = (s16)(s8)*subtitle++;
    *d = 0;
    NANDInitBanner(p, 0x11, wTitle, wSub);
    fn_80206E40(lbl_80386E98, p + 0xA0, 0xC0, 0x40, 0x6000, 0x17);
    u8* icon = p + 0x60A0;
    s32 shift = 0;
    for (s32 i = 0; i < 7; i++) {
        SysSprintf(name, lbl_80386EA8, i);
        fn_80206E40(name, icon, 0x30, 0x30, 0x1200, 0x17);
        u16 v = *(u16*)(p + 0x8);
        *(u16*)(p + 0x8) = (u16)((v & ~(3 << shift)) | (1 << shift));
        shift += 2;
        icon += 0x1200;
    }
}

// 0x8020703C
// Loads the save file into lbl_805271E0 (building the banner on first use); *outSize gets the read size.
// Returns 1 on success, 0 after reporting the failing step via fn_80206B6C.
s32 fn_8020703C(const char* title, const char* subtitle, u32* outSize) {
    u8 handle[0x98];
    s32 r = fn_80265C18((const char*)(lbl_80386C10 + 0x90), 0x30, 0);
    if (r != 0) {
        fn_80206B6C(r, lbl_80386C10 + 0x68);
        return 0;
    }
    r = NANDOpen((const char*)(lbl_80386C10 + 0x90), handle, 2);
    if (r != 0) {
        fn_80206B6C(r, lbl_80386C10 + 0x74);
        return 0;
    }
    if (lbl_805FBB28 == 0) {
        fn_80206F14(lbl_805271E0, title, subtitle);
        lbl_805FBB28 = 1;
    }
    u32 n = (u32)fn_80266060(handle, lbl_805271E0, 0xDEA0);
    *outSize = n;
    if (n != 0xDEA0) {
        fn_80206B6C((s32)n, lbl_80386C10 + 0x2A8);
        NANDClose(handle);
        return 0;
    }
    r = NANDClose(handle);
    if (r != 0) {
        fn_80206B6C(r, lbl_80386C10 + 0x2B4);
        return 0;
    }
    return 1;
}

// 0x80207AB8
// Sends the home/A button request to the movie player when it is active (lbl_805FB964 virtual +0x34),
// then clears the pending input state.
void fn_80207AB8(s32 arg) {
    if (arg == 0) fn_801F5220();
    if (fn_801F581C() == 0) {
        s32 active = 0;
        if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
            (*(VBoolFn*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0)
            active = 1;
        if (active != 0 && lbl_805FB978 != 0) {
            fn_801F5724(8);
            goto done;
        }
    }
    {
        s32 active = 0;
        if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
            (*(VBoolFn*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0)
            active = 1;
        if (active != 0) fn_801F5724(0x1000);
    }
done:
    fn_801F5814(0);
    fn_801F5794(0);
    fn_801F5734(NULL);
    fn_801F51DC(0);
}

// 0x801725C8
// Copy constructor: allocates via fn_801724D4, clones the child at +0xB0 (and its 0x30-byte-per-entry buffer
// through the heap object), shares the refcounted object at +0x10, and resets the animation fields.
u8* fn_801725C8(u8* src) {
    s32 f4 = *(s32*)(src + 0x4);
    u8* o = fn_801724D4(f4 != 0);
    u8* child = *(u8**)(src + 0xB0);
    if (child != NULL) {
        u8* c = fn_80174524(child);
        *(u8**)(o + 0xB0) = c;
        *(s32*)(c + 0x10) = *(s32*)(o + 0x4);
        u8* c2 = *(u8**)(o + 0xB0);
        *(u8**)(o + 0x38) = (*(AllocFn*)lbl_805FB608)(*(s32*)(c2 + 0x8) * 0x30, 0x10, 9, 0, 0, c2);
    } else {
        *(s32*)(o + 0xB0) = 0;
        *(s32*)(o + 0x38) = 0;
    }
    *(s32*)(o + 0xB4) = -1;
    u8* shared = *(u8**)(src + 0x10);
    *(u8**)(o + 0x10) = shared;
    if (shared != NULL) *(s32*)(shared + 0x10) = *(s32*)(shared + 0x10) + 1;
    *(s32*)(o + 0x30) = 0;
    *(s32*)(o + 0x34) = 0;
    *(s32*)(o + 0xB8) = 0;
    *(s32*)(o + 0xBC) = 0;
    *(s32*)(o + 0xC0) = 0;
    *(u32*)(o + 0xC) = *(u32*)(src + 0xC) & 0xFFFFFFF3u;
    *(f32*)(o + 0x8) = 1.0f;
    fn_80174AC4(o + 0x3C);
    for (s32 off = 0x14; off <= 0x2C; off += 4) *(f32*)(o + off) = 0.0f;
    return o;
}

// 0x801EA3C8
// Sets p[0] = scale / |p[4..6] - p[1..3]|; a length or result within (-0.0001, 0.0001) becomes 0.0001.
void fn_801EA3C8(u8* p, f32 scale) {
    f32 dy = *(f32*)(p + 0x10) - *(f32*)(p + 0x4);
    f32 dx = *(f32*)(p + 0x14) - *(f32*)(p + 0x8);
    f32 dz = *(f32*)(p + 0x18) - *(f32*)(p + 0xC);
    f32 sq = (dz * dz) + ((dy * dy) + (dx * dx));
    f32 len = 0.0f;
    if (sq > 0.0f) len = sqrtf(sq);
    if (len > -0.0001f && len < 0.0001f) len = 0.0001f;
    f32 r = scale / len;
    *(f32*)p = r;
    if (r > -0.0001f && r < 0.0001f) *(f32*)p = 0.0001f;
}

// 0x801F6B78
// Normalizes v; if it is (nearly) perpendicular to the axis at +0x90, stores it at +0xA8 and stores the
// cross product (axis x v) at +0xB4.
void fn_801F6B78(u8* p, f32* v) {
    f32 x = v[0], y = v[1], z = v[2];
    f32 sq = (z * z) + ((x * x) + (y * y));
    f32 nx, ny, nz;
    if (sq == 0.0f) {
        nx = ny = nz = 0.0f;
    } else {
        f32 r = 1.0f / sqrtf(sq);
        nx = x * r;
        ny = y * r;
        nz = z * r;
    }
    f32 ax = *(f32*)(p + 0x90), ay = *(f32*)(p + 0x94), az = *(f32*)(p + 0x98);
    f32 dot = (az * nz) + ((ax * nx) + (ay * ny));
    if (dot > -0.0001f && dot < 0.0001f) {
        *(f32*)(p + 0xA8) = nx;
        *(f32*)(p + 0xAC) = ny;
        *(f32*)(p + 0xB0) = nz;
        *(f32*)(p + 0xB4) = ny * az - nz * ay;
        *(f32*)(p + 0xB8) = nz * ax - nx * az;
        *(f32*)(p + 0xBC) = nx * ay - ny * ax;
    }
}

// 0x8022EC08
// Returns 1 if the lab already holds enough other parts (list at +0xA58, count at +0x10EC) of the same
// slot type as `part` that are allowed at their tier; arm slots (2/3) count together and need two.
s32 fn_8022EC08(u8* lab, PartInstance* part) {
    PartDef mine, cur;
    PartDbGetDef(&mine, gGameDb, part->defIndex);
    s32 need = ((u32)(mine.slot - 2) <= 1u) ? 2 : 1;
    s32 count = *(s32*)(lab + 0x10EC);
    for (s32 i = 0; i < count; i++) {
        PartInstance* other = *(PartInstance**)(lab + 0xA58 + i * 4);
        PartDbGetDef(&cur, gGameDb, other->defIndex);
        if (part == other) continue;
        BOOL same = 0;
        if ((u32)(cur.slot - 2) <= 1u) {
            if ((u32)(mine.slot - 2) <= 1u) same = 1;
        } else if (cur.slot == mine.slot) {
            same = 1;
        }
        if (!same) continue;
        if (PartAllowedAtRank(gLab, other->defIndex, (s32)PartGetTier(other) - 1) == 0) continue;
        if (--need <= 0) return 1;
    }
    return 0;
}

// 0x8020BE20
// Initializes the input-mapping table of the child object: owner pointer, default button index table
// (+0x88..+0x108), ten zeroed floats at +0xC..+0x30 and zeroed ints at +0x10C..+0x130.
void* fn_8020BE20(u8* obj, u8* owner) {
    *(s32*)(obj + 0xD0) = 0xB;
    *(u8**)obj = owner;
    *(s32*)(obj + 0x88) = 0;
    *(s32*)(obj + 0x8C) = 1;
    *(s32*)(obj + 0x90) = 1;
    *(s32*)(obj + 0x94) = 2;
    *(s32*)(obj + 0x98) = -1;
    *(s32*)(obj + 0x9C) = 3;
    *(s32*)(obj + 0xA0) = -1;
    *(s32*)(obj + 0xA4) = -1;
    *(s32*)(obj + 0xA8) = 9;
    *(s32*)(obj + 0xB4) = 7;
    *(s32*)(obj + 0xC0) = 5;
    *(s32*)(obj + 0xCC) = 3;
    *(s32*)(obj + 0xD8) = 1;
    *(s32*)(obj + 0xAC) = 0xE;
    *(s32*)(obj + 0xB8) = 0xD;
    *(s32*)(obj + 0xC4) = 0xC;
    *(s32*)(obj + 0xDC) = 0xA;
    *(s32*)(obj + 0xB0) = 8;
    *(s32*)(obj + 0xBC) = 6;
    *(s32*)(obj + 0xC8) = 4;
    *(s32*)(obj + 0xD4) = 2;
    *(s32*)(obj + 0xE0) = 0;
    *(s32*)(obj + 0xE4) = 0x11;
    *(s32*)(obj + 0xE8) = 0x11;
    *(s32*)(obj + 0xEC) = 0x10;
    *(s32*)(obj + 0xF0) = 0xF;
    *(s32*)(obj + 0xF4) = 0xF;
    *(s32*)(obj + 0xF8) = 2;
    *(s32*)(obj + 0xFC) = 2;
    *(s32*)(obj + 0x100) = 1;
    *(s32*)(obj + 0x104) = 0;
    *(s32*)(obj + 0x108) = 0;
    for (s32 off = 0xC; off <= 0x30; off += 4) *(f32*)(obj + off) = 0.0f;
    for (s32 off = 0x10C; off <= 0x130; off += 4) *(s32*)(obj + off) = 0;
    *(s32*)(obj + 0x4) = *(s32*)(owner + 0x44);
    return obj;
}

}  // extern "C"
