// Drafts miss315: camera-facing beam draw (0x801D82AC), camera matrix selection (0x801D5850),
// line-strip vertex upload for the two draw modes (0x801E2BB4 / 0x801E32E4) and the catalog
// loader start-up (0x801E54E4).
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"

extern "C++" u8 sRendDLs[];     // 0x80401108 (display-list records, 0x94 bytes each)
void fn_80154358(void);         // 0x80154358 (C++ linkage, defined in small_8015.cpp)

extern "C" {
f32   FloatAbs(f32 v);                                          // 0x80058240
void* MemAllocDefault(u32 size);                                // 0x801D4650
void* MemAllocSmall(u32 size);                                  // 0x801D41CC
void* ObjAlloc(u32 size);                                       // 0x801D4414
int   SysSprintf(char* buf, const char* fmt, ...);              // 0x80142EF8
void  Vec3Assign(Vec3* out, const Vec3* v);                     // 0x80053354
void  Vec3Copy(Vec3* out, const Vec3* v);                       // 0x80052640
f32   Vec3GetX(const Vec3* v);
f32   Vec3GetY(const Vec3* v);
f32   Vec3GetZ(const Vec3* v);
void  Vec3NormalizeTo(Vec3* out, const Vec3* v);                // 0x80055D9C
void  Vec3SetUnitZ(Vec3* v);                                    // 0x800549A8
void  ZoneLog(void* loader, const char* fmt, ...);              // 0x801D7340
void  fn_800359F4(u8* a3, f32 f1, f32 f2, f32 f3);              // 0x800359F4
u8*   fn_80044634(s32 n);                                       // 0x80044634
void  fn_800446BC(void* cam, s32 v);                            // 0x800446BC
void  fn_80052638(void);                                     // 0x80052638 (empty leaf; the caller passes the vector)
void  fn_800526D0(u8* out);                                     // 0x800526D0 (matrix identity)
void  fn_8005535C(u8* dst, const u8* src);                      // 0x8005535C
u8*   fn_800554D4(u8* mtx);                                     // 0x800554D4
void  fn_800557B4(u8* out);                                     // 0x800557B4
void  fn_8007FD98(u8* mtx, u8* node);                           // 0x8007FD98
void  fn_800C09C0(Vec3* out);                                   // 0x800C09C0
void  fn_800C11A4(Vec3* out, const Vec3* a, const Vec3* b);     // 0x800C11A4 (cross product)
void  fn_8010AB3C(void);                                        // 0x8010AB3C
void  fn_8010AB70(void);                                        // 0x8010AB70
void  fn_80149094(Vec3* out);                                   // 0x80149094
void  fn_8014DE68(void);                                        // 0x8014DE68
void  fn_8014E3EC(s32 n);                                       // 0x8014E3EC
void   fn_80152620(s32 a, s32 b, u8* quad);                      // 0x80152620
void   fn_80151644(s32 a, s32 b, u8* quad);                      // 0x80151644
void  fn_80154318(void);                                        // 0x80154318
void  fn_80155724(u8* m);                                       // 0x80155724
s32   fn_8015BA30(void* p);                                     // 0x8015BA30
u8*   fn_8016EB20(u8* m);                                       // 0x8016EB20 (returns its argument)
u8*   fn_8016EB4C(u8* m);                                       // 0x8016EB4C (returns m + 0x18)
void  fn_80174A24(u8* p, u32 color);                            // 0x80174A24
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
u8*   fn_801D6790(u8* cam);                                     // 0x801D6790 (returns the camera's matrix block)
u8*   fn_801D6D78(void);                                        // 0x801D6D78 (camera record)
f32   fn_801D7900(u8* a3, u8* a4);                              // 0x801D7900
void  fn_801D7E9C(f32* m, f32* v);                              // 0x801D7E9C
void  fn_801D8090(void);                                        // 0x801D8090
s32   fn_801CB938(void);                                        // 0x801CB938
void  fn_801E29D4(u8* p);                                       // 0x801E29D4
u8*   fn_801E4B98(u8* pool);                                    // 0x801E4B98
s32   fn_801E4BE4(u8* pool, u8* rec, const char* name, const char* mode); // 0x801E4BE4
s32   fn_801E4E20(u8* pool, void* rec);                         // 0x801E4E20
s32   fn_801E4FE0(u8* pool, void* rec, u8* buf, u32 size, s32 b); // 0x801E4FE0
void  fn_801E5220(u8* pool, s32* handle);                       // 0x801E5220

extern u8  lbl_80348950[];     // 0x80348950 (0x20-byte records, u32 at +0)
extern u8  lbl_80348A10[];     // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32 lbl_80348AD0;       // 0x80348AD0
extern s32 lbl_8040ADF0[8];    // 0x8040ADF0
extern s32 lbl_8040AE10[8];    // 0x8040AE10
extern u8  lbl_8047C640[];     // 0x8047C640 (camera)
extern u8  lbl_8047D1E8[];     // 0x8047D1E8 (camera-facing 3x4 matrix, 12 floats)
extern u8  lbl_8047D218[];     // 0x8047D218 (view matrix copy, 12 floats)
extern u8  lbl_8047D498[];     // 0x8047D498 (34 x 0x10-byte colour/vertex records)
extern u8  lbl_8048A8B4[];     // 0x8048A8B4 (async read queue)
extern u32 lbl_805F9614;       // 0x805F9614 (view matrix mode)
extern u32 lbl_805F969C;       // 0x805F969C (invalid-handle sentinel)
extern s32 lbl_805FB4B0;       // 0x805FB4B0
extern s32 lbl_805FB4BC;       // 0x805FB4BC (current viewport index)
extern s32 lbl_805FB4C0;       // 0x805FB4C0 (display-list recording flag)
extern s32 lbl_805FB4C4;       // 0x805FB4C4 (vertex buffer write offset)
extern s32 lbl_805FB4C8;       // 0x805FB4C8 (render budget)
extern s32 lbl_805FB4CC;       // 0x805FB4CC (vertex buffer bytes left)
extern u8* lbl_805FB4D0;       // 0x805FB4D0 (vertex buffer base)
extern u8  lbl_805FB50C;       // 0x805FB50C (GX shadow flags byte)
extern u8  lbl_805FB82C;       // 0x805FB82C (beam colour table initialised)
}

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// One XF-style write: opcode byte, two halfwords, one word.
static void GxXfWrite(u8 op, u16 a, u16 b, u32 value) {
    GXWGFifo._u8  = op;
    GXWGFifo._u16 = a;
    GXWGFifo._u16 = b;
    GXWGFifo._u32 = value;
}

// Switches the line-strip draw state on (flag bit 0 selects one of two register sets).
static void GxLineStateOn(bool alt) {
    if (alt) {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000001);
        GxXfWrite(0x10, 0, 0x103F, 1);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000400);
        GxXfWrite(0x10, 0, 0x1010, 0x441);
        GxBpWrite(0x283C0000);
        GxBpWrite(0xC008F2A4);
        GxBpWrite(0xC108E6F0);
        GxBpWrite(0xC208F80F);
        GxBpWrite(0xC308F070);
    } else {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000001);
        GxXfWrite(0x10, 0, 0x103F, 1);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x100E, 0x442);
        GxXfWrite(0x10, 0, 0x1010, 0x442);
        GxBpWrite(0x28380040);
        GxBpWrite(0xC008F8AF);
        GxBpWrite(0xC108F2F0);
    }
}

// Switches the line-strip draw state back off.
static void GxLineStateOff(bool alt) {
    if (alt) {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x103F, 0);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x1010, 0x441);
        GxBpWrite(0x28380000);
        GxBpWrite(0xC008F2A4);
        GxBpWrite(0xC108E6F0);
    } else {
        GxBpWrite(0xFE00000F);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x103F, 0);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0x00000000);
        GxXfWrite(0x10, 0, 0x100E, 0x442);
        GxXfWrite(0x10, 0, 0x1010, 0x442);
        GxBpWrite(0x28380000);
        GxBpWrite(0xC008FFFA);
        GxBpWrite(0xC108FFD0);
    }
}

// Reverses the byte order of a 32-bit word with byte accesses (the original swaps through a stack slot).
static u32 SwapBytes4(u32 v) {
    u8 b[4];
    *(u32*)b = v;
    u8 t0 = b[0];
    u8 t3 = b[3];
    u8 t1 = b[1];
    u8 t2 = b[2];
    b[0] = t3;
    b[3] = t0;
    b[1] = t2;
    b[2] = t1;
    return *(u32*)b;
}

static f32 AbsX(const Vec3* v) { return FloatAbs(Vec3GetX(v)); }
static f32 AbsY(const Vec3* v) { return FloatAbs(Vec3GetY(v)); }
static f32 AbsZ(const Vec3* v) { return FloatAbs(Vec3GetZ(v)); }

extern "C" {

// 0x801D82AC
// Draws one camera-facing beam: builds an orthonormal frame around the direction vec (using a helper
// axis picked from its smallest/largest component), posts it with the node, fills the colour records
// and submits the 0x22 quads (twice in the second mode).
void fn_801D82AC(u8* arg0, u8* arg1, u8* arg2, u32 arg3, s32 arg4, f32 farg0, f32 farg1) {
    u8   sp108[0x48];
    u8   spD8[0x30];
    Vec3 spC8;
    Vec3 spBC;
    Vec3 spB0;
    Vec3 spA4;
    u8   sp98[12];
    Vec3 tmp;

    if (fn_801CB938() == 0) return;

    fn_800526D0(sp108);
    fn_80052638();
    fn_800359F4(sp98, farg1, farg1, farg0);
    Vec3Copy(&spC8, (Vec3*)sp98);
    fn_800557B4(spD8);
    fn_8005535C(sp108, spD8);
    fn_80052638();
    fn_80052638();
    Vec3Assign(&spA4, (Vec3*)arg2);

    // Helper axis for the cross products: the axis along the smallest component first, then the largest.
    if (AbsX(&spA4) < AbsY(&spA4) && AbsX(&spA4) < AbsZ(&spA4)) {
        fn_80149094(&tmp);
        Vec3Copy(&spB0, &tmp);
    } else if (AbsY(&spA4) < AbsX(&spA4) && AbsY(&spA4) < AbsZ(&spA4)) {
        fn_800C09C0(&tmp);
        Vec3Copy(&spB0, &tmp);
    } else if (AbsZ(&spA4) < AbsX(&spA4) && AbsZ(&spA4) < AbsY(&spA4)) {
        Vec3SetUnitZ(&tmp);
        Vec3Copy(&spB0, &tmp);
    } else if (AbsX(&spA4) > AbsY(&spA4) && AbsX(&spA4) > AbsZ(&spA4)) {
        fn_800C09C0(&tmp);
        Vec3Copy(&spB0, &tmp);
    } else if (AbsY(&spA4) > AbsX(&spA4) && AbsY(&spA4) > AbsZ(&spA4)) {
        Vec3SetUnitZ(&tmp);
        Vec3Copy(&spB0, &tmp);
    } else if (AbsZ(&spA4) > AbsX(&spA4) && AbsZ(&spA4) > AbsY(&spA4)) {
        fn_80149094(&tmp);
        Vec3Copy(&spB0, &tmp);
    } else {
        Vec3SetUnitZ(&tmp);
        Vec3Copy(&spB0, &tmp);
    }

    Vec3NormalizeTo(&tmp, &spA4);
    Vec3Copy(&spA4, &tmp);
    fn_800C11A4(&tmp, &spA4, &spB0);
    Vec3Copy(&spBC, &tmp);
    Vec3NormalizeTo(&tmp, &spBC);
    Vec3Copy(&spBC, &tmp);
    fn_800C11A4(&tmp, &spBC, &spA4);
    Vec3Copy(&spB0, &tmp);
    Vec3NormalizeTo(&tmp, &spB0);
    Vec3Copy(&spB0, &tmp);

    Vec3Copy((Vec3*)fn_8016EB20(sp108), &spBC);
    Vec3Copy((Vec3*)fn_8016EB4C(sp108), (Vec3*)arg2);
    Vec3Copy((Vec3*)fn_800554D4(sp108), &spB0);
    fn_801D7E9C((f32*)sp108, (f32*)&spC8);
    fn_8007FD98(sp108, arg1);

    if (lbl_805FB82C == 0) {
        lbl_805FB82C = 1;
        fn_801D8090();
    }
    u8* rec = lbl_8047D498;
    for (s32 i = 0; i < 0x22; i++) {
        fn_80174A24(rec, arg3);
        rec += 0x10;
    }
    fn_800446BC(arg0, 0);
    fn_8010AB3C();
    fn_80154358();
    u8* quads = fn_80044634(0x220);
    memcpy(quads, lbl_8047D498, 0x220);
    fn_80155724(sp108);
    if (arg4 != 0) {
        fn_80152620(0x98, 0x22, quads);
    } else {
        fn_80152620(0xB0, 0x22, quads);
        u8* half = fn_80044634(0x220);
        u8* d = half;
        s32 off = 0;
        for (s32 i = 0; i < 0x10; i++) {
            fn_801D7900(d, lbl_8047D498 + off);
            fn_801D7900(half + (i + 0x10) * 0x10, lbl_8047D498 + off + 0x10);
            off += 0x20;
            d += 0x10;
        }
        fn_80152620(0xB0, 0x10, half);
        fn_80152620(0xB0, 0x10, half + 0x100);
    }
    fn_8010AB70();
    fn_80154318();
}

// 0x801D5850
// Returns the 3x4 view matrix block: a copy of the camera's matrix, or (mode 0) that matrix with the
// x axis column of every row negated and composed with the current camera record into a second block.
u8* fn_801D5850(void) {
    f32* src = (f32*)fn_801D6790(lbl_8047C640);
    f32 sv[12];
    for (s32 i = 0; i < 12; i++) sv[i] = src[i];
    f32* view = (f32*)lbl_8047D218;

    if ((s32)lbl_805F9614 != 0) {
        for (s32 i = 0; i < 12; i++) view[i] = sv[i];
        return lbl_8047D218;
    }

    for (s32 i = 0; i < 12; i++) view[i] = sv[i];
    f32* c = (f32*)fn_801D6D78();
    f32* out = (f32*)lbl_8047D1E8;

    f32 m0 = view[0] * -1.0f;
    f32 m1 = view[1] * 1.0f;
    f32 m2 = view[2] * 1.0f;
    f32 m3 = view[3] * -1.0f;
    f32 m4 = view[4] * 1.0f;
    f32 m5 = view[5] * 1.0f;
    f32 m6 = view[6] * -1.0f;
    f32 m7 = view[7] * 1.0f;
    f32 m8 = view[8] * 1.0f;
    f32 m9 = view[9] * -1.0f;
    f32 m10 = view[10] * 1.0f;
    f32 m11 = view[11] * 1.0f;
    view[0] = m0;
    view[1] = m1;
    view[2] = m2;
    view[3] = m3;
    view[4] = m4;
    view[5] = m5;
    view[6] = m6;
    view[7] = m7;
    view[8] = m8;
    view[9] = m9;
    view[10] = m10;
    view[11] = m11;

    f32 c0 = c[0], c3 = c[3], c6 = c[6];
    f32 c1 = c[1], c4 = c[4], c7 = c[7];
    f32 c2 = c[2], c5 = c[5], c8 = c[8];

    out[0] = c6 * m2 + (c0 * m0 + c3 * m1);
    out[1] = c7 * m2 + (c1 * m0 + c4 * m1);
    out[3] = c6 * m5 + (c0 * m3 + c3 * m4);
    out[4] = c7 * m5 + (c1 * m3 + c4 * m4);
    out[6] = c6 * m8 + (c0 * m6 + c3 * m7);
    out[7] = c7 * m8 + (c1 * m6 + c4 * m7);
    out[9] = c[9] + (c6 * m11 + (c0 * m9 + c3 * m10));
    out[10] = c[10] + (c7 * m11 + (c1 * m9 + c4 * m10));
    out[2] = c8 * m2 + (c2 * m0 + c5 * m1);
    out[5] = c8 * m5 + (c2 * m3 + c5 * m4);
    out[8] = c8 * m8 + (c2 * m6 + c5 * m7);
    out[11] = c[11] + (c8 * m11 + (c2 * m9 + c5 * m10));
    return lbl_8047D1E8;
}

// 0x801E2BB4
// Uploads the line-strip vertices of the draw object (3 vertices of 0x18 bytes per source triple) into
// the vertex buffer, switches the GX line state on (texture setup from the strip's texture record, or
// off when there is none) and submits the strip.
void fn_801E2BB4(u8* arg0) {
    if (!(*(s32*)(arg0 + 0x4C) & 0x80000)) return;
    fn_801E29D4(arg0);
    if (!(*(s32*)(arg0 + 0x4C) & 0x3FFFF)) return;

    s32 count3 = (s32)(*(u32*)(arg0 + 0x208) * 3);
    s32 size = (count3 * 0x18 + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < size) fn_8014E3EC(size);
    s32 pos = lbl_805FB4C4;
    u8* base = lbl_805FB4D0;
    lbl_805FB4C4 = pos + size;
    lbl_805FB4CC = lbl_805FB4CC - size;
    u8* dst = base + pos;
    if (dst == NULL) return;

    u8* idx = arg0 + 0x1D0;
    u8* d = dst;
    for (u32 i = 0; i < *(u32*)(arg0 + 0x208); i++) {
        for (s32 k = 0; k < 3; k++) {
            u8* e = arg0 + idx[k] * 0x18;
            *(f32*)(d + 0x0) = *(f32*)(e + 0x60);
            *(f32*)(d + 0x4) = *(f32*)(e + 0x64);
            *(u32*)(d + 0x8) = SwapBytes4(*(u32*)(e + 0x5C));
            *(f32*)(d + 0xC) = *(f32*)(e + 0x50);
            *(f32*)(d + 0x10) = *(f32*)(e + 0x54);
            *(f32*)(d + 0x14) = *(f32*)(e + 0x58);
            d += 0x18;
        }
        idx += 3;
    }

    u8* f = *(u8**)(arg0 + 0x34);
    if (f != NULL) {
        if (lbl_805FB4C0 != 0 && fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0 > lbl_805FB4C8) {
            fn_8014DE68();
        }
        if (!(lbl_805FB50C & 2)) {
            lbl_805FB50C = lbl_805FB50C | 2;
            GxLineStateOn((lbl_805FB50C & 1) != 0);
        }
        GxBpWrite(0xFE000060);
        GxBpWrite(((u32)*(u8*)(f + 0x16) << 5) | 0x80000000u);
        if ((u32)lbl_8040ADF0[0] != *(u32*)(f + 0x1C) || lbl_8040AE10[0] != 0) {
            lbl_805FB4B0 = lbl_805FB4B0 + 1;
            lbl_8040ADF0[0] = *(s32*)(f + 0x1C);
            lbl_8040AE10[0] = 0;
            u32 cfg = lbl_80348AD0;
            u8 tex = *(u8*)(f + 0x17);
            s32 second = *(s32*)(lbl_80348A10 + tex * 0x20);
            GxBpWrite((((u32)*(u8*)(f + 0x13) - 1) & 0xF) << 12 | 0x84000000u);
            GxBpWrite(((u32)*(u16*)(f + 0x8) - 1) | (((u32)*(u16*)(f + 0xA) - 1) << 10) |
                      ((u32)*(u8*)(f + 0x14) << 20) | 0x88000000u);
            GxBpWrite((*(u32*)(lbl_80348950 + tex * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
            if (second >= 0) {
                GxBpWrite(((u32)second >> 5) | 0x900D0000u | 0x8000u);
            } else {
                GxBpWrite(0x90000000u);
            }
            GxBpWrite(((*(u32*)(f + 0x1C) >> 5) & 0xFFFFFF) | 0x94000000u);
            if (*(u8*)(f + 0x11) != 0) {
                u32 g = (cfg >> 9) & 0x3FF;
                GxBpWrite(((g | 0x98000000u) & ~0x3FC00u) | (((u32)*(u8*)(f + 0x15) << 10) & 0x3FC00u));
                GxBpWrite(0xFEFFFF00u);
                GxBpWrite(0x0F000000u);
                GxBpWrite(((*(u32*)(f + 0x20) >> 5) & 0xFFFFFF) | 0x64000000u);
                GxBpWrite(((g | 0x65000000u) & ~0x1FC00u) |
                          ((((s32)*(u8*)(f + 0x11) >> 1) << 10) & 0x1FC00u));
                GxBpWrite(0xFEFFFF00u);
                GxBpWrite(0x0F000000u);
            }
        }
        GxBpWrite(((u32)*(u16*)(f + 0x8) - 1) | 0x30000000u);
        GxBpWrite(((u32)*(u16*)(f + 0xA) - 1) | 0x31000000u);
    } else if (lbl_805FB50C & 2) {
        u8 flags = lbl_805FB50C;
        lbl_805FB50C = flags & ~2;
        GxLineStateOff((flags & 1) != 0);
    }
    fn_80151644(0x90, count3, dst);
}

// 0x801E32E4
// Variant of fn_801E2BB4 for untextured strips: 3 vertices of 0x10 bytes per source triple, the line
// state is only switched off when it was on, and the strip goes out through fn_80152620.
void fn_801E32E4(u8* arg0) {
    if (!(*(s32*)(arg0 + 0x4C) & 0x80000)) return;
    fn_801E29D4(arg0);
    if (!(*(s32*)(arg0 + 0x4C) & 0x3FFFF)) return;

    s32 count3 = (s32)(*(u32*)(arg0 + 0x208) * 3);
    s32 size = (count3 * 0x10 + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < size) fn_8014E3EC(size);
    u8 flags = lbl_805FB50C;
    s32 left = lbl_805FB4CC - size;
    s32 pos = lbl_805FB4C4;
    u8* base = lbl_805FB4D0;
    lbl_805FB4CC = left;
    lbl_805FB4C4 = pos + size;
    u8* dst = base + pos;
    if (flags & 2) {
        lbl_805FB50C = flags & ~2;
        GxLineStateOff((flags & 1) != 0);
    }
    if (dst == NULL) return;

    u8* idx = arg0 + 0x1D0;
    u8* d = dst;
    for (u32 i = 0; i < *(u32*)(arg0 + 0x208); i++) {
        for (s32 k = 0; k < 3; k++) {
            u8* e = arg0 + idx[k] * 0x18;
            *(u32*)(d + 0x0) = SwapBytes4(*(u32*)(e + 0x5C));
            *(f32*)(d + 0x4) = *(f32*)(e + 0x50);
            *(f32*)(d + 0x8) = *(f32*)(e + 0x54);
            *(f32*)(d + 0xC) = *(f32*)(e + 0x58);
            d += 0x10;
        }
        idx += 3;
    }
    fn_80152620(0x90, count3, dst);
}

// 0x801E54E4
// Catalog loader start-up: allocates the loader's buffer and its 8 per-slot tables, reads the catalog
// file (name) into buf through the async queue, copies the catalog's two record tables
// (0x18-byte and 0x40-byte entries) into the loader and opens a stream handle for each named entry.
void fn_801E54E4(u8* arg0, const char* arg1, u8* arg2, u32 arg3, u32 arg4) {
    s32 mode;
    s32 handle;
    char name[0x100];

    void* mem = MemAllocDefault(0x4400);
    *(void**)(arg0 + 0x4) = mem;
    *(void**)(arg0 + 0x1C) = mem;
    *(s32*)(arg0 + 0x18) = 0x80;
    *(s32*)(arg0 + 0x14) = -1;
    *(s32*)(arg0 + 0x10) = -1;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x28) = 0;
    *(u8*)(arg0 + 0x0) = 1;
    u8* tbl = (u8*)MemAllocSmall(0x20);
    *(u8**)(arg0 + 0x48C) = tbl;
    for (s32 i = 0; i < 8; i++) {
        *(u8*)(arg0 + 0x2C + i * 0x40) = 0;
        *(u8*)(arg0 + 0x22C + i * 0x40) = 0;
        *(s32*)(arg0 + 0x42C + i * 4) = 0;
        *(s32*)(arg0 + 0x44C + i * 4) = 0;
        *(s32*)(arg0 + 0x46C + i * 4) = 0;
        *(u32*)(*(u8**)(arg0 + 0x48C) + i * 4) = lbl_805F969C;
    }
    ZoneLog(arg0, "Check catalog exists");
    ZoneLog(arg0, "Read catalog...");
    *(void**)(arg0 + 0x7B4) = fn_801D40E0(0x1C);

    u8* rec = fn_801E4B98(lbl_8048A8B4);
    fn_801E4BE4(lbl_8048A8B4, rec, arg1, "r");
    fn_801E4FE0(lbl_8048A8B4, rec, arg2, arg4, 0);
    handle = fn_801E4E20(lbl_8048A8B4, rec);
    fn_801E5220(lbl_8048A8B4, &handle);

    s32 n1 = *(s32*)(arg2 + 0x8);
    u8* src1 = arg2 + 0x40;
    u8* hdr = *(u8**)(arg0 + 0x7B4);
    u8* src2 = src1 + n1 * 0x18;
    if (n1 != 0) {
        *(s32*)(hdr + 0x4) = n1;
        mode = 1;
        *(s32*)(hdr + 0x8) = mode;
        if (mode == 2) {
            *(void**)(hdr + 0x0) = ObjAlloc(*(s32*)(hdr + 0x4) * 0x18);
        } else if (mode == 1) {
            *(void**)(hdr + 0x0) = MemAllocDefault(*(s32*)(hdr + 0x4) * 0x18);
        }
        u32 off = 0;
        for (u32 i = 0; i < *(u32*)(arg2 + 0x8); i++) {
            u8* d = *(u8**)(hdr + 0x0) + off;
            off += 0x18;
            *(u32*)(d + 0x0) = *(u32*)(src1 + 0x0);
            *(u32*)(d + 0x4) = *(u32*)(src1 + 0x4);
            *(u32*)(d + 0x8) = *(u32*)(src1 + 0x8);
            *(u32*)(d + 0xC) = *(u32*)(src1 + 0xC);
            *(u32*)(d + 0x10) = *(u32*)(src1 + 0x10);
            *(u32*)(d + 0x14) = *(u32*)(src1 + 0x14);
            src1 += 0x18;
        }
    }

    s32 n2 = *(s32*)(arg2 + 0xC);
    if (n2 != 0) {
        *(s32*)(hdr + 0x10) = n2;
        mode = 1;
        *(s32*)(hdr + 0x14) = mode;
        if (mode == 2) {
            *(void**)(hdr + 0xC) = ObjAlloc(*(u32*)(hdr + 0x10) << 6);
        } else if (mode == 1) {
            *(void**)(hdr + 0xC) = MemAllocDefault(*(u32*)(hdr + 0x10) << 6);
        }
        u32 off = 0;
        for (u32 i = 0; i < *(u32*)(arg2 + 0xC); i++) {
            u8* d = *(u8**)(hdr + 0xC) + off;
            *(u32*)(d + 0x0) = *(u32*)(src2 + 0x0);
            *(u32*)(d + 0x4) = *(u32*)(src2 + 0x4);
            for (s32 k = 8; k < 0x40; k++) d[k] = src2[k];
            src2 += 0x40;
            off += 0x40;
        }
    }

    *(s32*)(hdr + 0x18) = *(s32*)(arg2 + 0x10);
    s32 n3 = *(s32*)(*(u8**)(arg0 + 0x7B4) + 0x10);
    *(s32*)(arg0 + 0x7AC) = n3;
    mode = 1;
    *(s32*)(arg0 + 0x7B0) = mode;
    if (mode == 2) {
        *(void**)(arg0 + 0x7A8) = ObjAlloc(n3 * 4);
    } else if (mode == 1) {
        *(void**)(arg0 + 0x7A8) = MemAllocDefault(n3 * 4);
    }
    for (s32 i = 0; i < n3; i++) {
        *(u8**)(*(u8**)(arg0 + 0x7A8) + i * 4) = NULL;
        u8* ent = *(u8**)(*(u8**)(arg0 + 0x7B4) + 0xC) + i * 0x40 + 0x8;
        if ((s8)*ent != 0) {
            SysSprintf(name, "Streams/%s", ent);
            u8* srec = fn_801E4B98(lbl_8048A8B4);
            *(u8**)(*(u8**)(arg0 + 0x7A8) + i * 4) = srec;
            s32 h = fn_801E4BE4(lbl_8048A8B4, *(u8**)(*(u8**)(arg0 + 0x7A8) + i * 4), name, "r");
            if (h == -1) {
                *(u8**)(*(u8**)(arg0 + 0x7A8) + i * 4) = NULL;
            } else {
                handle = h;
                fn_801E5220(lbl_8048A8B4, &handle);
            }
        }
    }
    ZoneLog(arg0, "VerifyCatalogConsistency()");
    ZoneLog(arg0, "VolumeAsyncMgr Init finished");
}

}  // extern "C"
