// Drafts miss763: renderer-state records at 0x80176xxx-0x8017Axxx: property setter/getter pairs,
// the record stream reader/writer, and the GX state setup for drawing one record.
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"

extern "C++" u8 sRendDLs[];     // 0x80401108 (display-list records, 0x94 bytes each)
extern "C++" void fn_80154358(void);   // 0x80154358 (defined in small_8015.cpp)

extern "C" {
void fn_80148D94(const f32* v);                                          // 0x80148D94
void fn_801487CC(const f32* a, const f32* b, const f32* c, const f32* d); // 0x801487CC
void fn_80149594(void);                                                 // 0x80149594
void fn_8014DE68(void);                                                 // 0x8014DE68
void fn_8015428C(s32 a);                                                // 0x8015428C
void fn_80154318(void);                                                 // 0x80154318
void fn_8015A39C(u8* a);                                                // 0x8015A39C (the call site also passes r4 = r31 + 4 and f1 = the width, which it ignores)
s32  fn_8015BA30(void* p);                                              // 0x8015BA30
void fn_80176D18(u8* p);                                                // 0x80176D18
void fn_801776C8(f32* m);                                               // 0x801776C8
void fn_80177748(u8* p);                                                // 0x80177748
u16  fn_80178CC4(u32 idx);                                              // 0x80178CC4
s32  fn_80178D2C(s32 id);                                               // 0x80178D2C
u16  fn_80178DEC(u32 idx);                                              // 0x80178DEC
s32  fn_80178E9C(s32 id);                                               // 0x80178E9C

extern u8   lbl_80348950[];     // 0x80348950 (0x20-byte records, u32 at +0)
extern u8   lbl_80348A10[];     // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32  lbl_80348AD0;       // 0x80348AD0
extern u8   lbl_80375E08[];     // 0x80375E08 (stream writer slots: +0 u16, +4 u32, +8 f32)
extern s32  lbl_8040ADF0[8];    // 0x8040ADF0
extern s32  lbl_8040AE10[8];    // 0x8040AE10
extern u8   lbl_805F9212;       // 0x805F9212 (copy filter value)
extern s32  lbl_805FB4B0;       // 0x805FB4B0
extern s32  lbl_805FB4BC;       // 0x805FB4BC (current viewport index)
extern s32  lbl_805FB4C8;       // 0x805FB4C8 (render budget)
extern u8   lbl_805FB50C;       // 0x805FB50C (GX shadow flags byte)
extern u8   lbl_805FB50D;       // 0x805FB50D
extern s32  lbl_805FB5C0;       // 0x805FB5C0 (disables the record's pipeline when set)
extern f32* lbl_805FB5D4;       // 0x805FB5D4 (override matrix block: 20 floats, or NULL)
}

// Stream writer slots in lbl_80375E08 (called with the stream cursor pointer).
typedef void (*WrU16Fn)(u8** s, u32 v);
typedef void (*WrU32Fn)(u8** s, u32 v);
typedef void (*WrF32Fn)(u8** s, f32 v);
#define WR_U16(s, v) (*(WrU16Fn*)(lbl_80375E08 + 0x0))((s), (v))
#define WR_U32(s, v) (*(WrU32Fn*)(lbl_80375E08 + 0x4))((s), (v))
#define WR_F32(s, v) (*(WrF32Fn*)(lbl_80375E08 + 0x8))((s), (v))

static inline void PutByte(u8** s, u8 v) { *(*s)++ = v; }
static inline u8   GetByte(u8** s) { return *(*s)++; }
static inline u16  GetU16(u8** s) { u16 v = *(u16*)*s; *s += 2; return v; }
static inline u32  GetU32(u8** s) { u32 v = *(u32*)*s; *s += 4; return v; }
static inline f32  GetF32(u8** s) { f32 v = *(f32*)*s; *s += 4; return v; }

static inline f32 U2F(u32 v) { f32 f; memcpy(&f, &v, 4); return f; }
static inline u32 F2U(f32 f) { u32 v; memcpy(&v, &f, 4); return v; }
static inline s32 ClampS(s32 v, s32 lo, s32 hi) { return v < lo ? lo : (v > hi ? hi : v); }

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBp(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// One XF register-load header plus a single word: opcode 0x10, count - 1 (0), register, value.
static void GxXf1(u16 reg, u32 value) {
    GXWGFifo._u8  = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = reg;
    GXWGFifo._u32 = value;
}

// The 11-write setup shared by the two line-state variants: 'alt' picks the first register set.
static void GxStateAlt() {
    GxBp(0xFE00000F);
    GxBp(0x00000001);
    GxXf1(0x103F, 1);
    GxBp(0xFE003C00);
    GxBp(0x00000400);
    GxXf1(0x1010, 0x441);
    GxBp(0x283C0000);
    GxBp(0xC008F2A4);
    GxBp(0xC108E6F0);
    GxBp(0xC208F80F);
    GxBp(0xC308F070);
}

extern "C" {

// 0x801767BC
// Property setter by index (0..0x1A): copies a float/bool/word value into the record. Float
// properties are raw 32-bit copies; the 0x4C and 0x54 properties also re-derive the owner's class
// and flag bytes (fn_80176D18).
void fn_801767BC(u8* obj, u32 idx, s32 val) {
    switch (idx) {
    case 0x00: *(u8*)(obj + 0x04) = (val != 0); break;
    case 0x01: *(u8*)(obj + 0x05) = (val != 0); break;
    case 0x02: *(s32*)(obj + 0x08) = val; break;
    case 0x03: *(s32*)(obj + 0x0C) = val; break;
    case 0x04: *(s32*)(obj + 0x10) = val; break;
    case 0x05:
        *(s32*)(obj + 0x4C) = val;
        fn_80176D18(obj);
        break;
    case 0x06: *(s32*)(obj + 0x18) = val; break;
    case 0x07: *(s32*)(obj + 0x1C) = val; break;
    case 0x08: *(s32*)(obj + 0x20) = val; break;
    case 0x09: *(s32*)(obj + 0x28) = val; break;
    case 0x0A: *(s32*)(obj + 0x2C) = val; break;
    case 0x0B: *(s32*)(obj + 0x30) = val; break;
    case 0x0C: *(s32*)(obj + 0x38) = val; break;
    case 0x0D: *(s32*)(obj + 0x3C) = val; break;
    case 0x0E: *(s32*)(obj + 0x40) = val; break;
    case 0x0F: *(s32*)(obj + 0x48) = val; break;
    case 0x10: *(s32*)(obj + 0x50) = val; break;
    case 0x11:
        *(s32*)(obj + 0x54) = val;
        fn_80176D18(obj);
        break;
    case 0x12: *(u8*)(obj + 0x58) = (val != 0); break;
    case 0x13: *(u8*)(obj + 0x59) = (val != 0); break;
    case 0x14: *(u8*)(obj + 0x5A) = (val != 0); break;
    case 0x15: *(s32*)(obj + 0x5C) = val; break;
    case 0x16: *(s32*)(obj + 0x60) = val; break;
    case 0x17: *(u8*)(obj + 0x64) = (val != 0); break;
    case 0x18: *(u8*)(obj + 0x65) = (val != 0); break;
    case 0x19: *(u8*)(obj + 0x66) = (val != 0); break;
    case 0x1A: *(u8*)(obj + 0x67) = (val != 0); break;
    default: break;
    }
}

// 0x80176E20
// Draws one record: composes the record's 3x4 transform with the camera transform b (or loads
// camera b's second block when the record is disabled), selects the line pipeline for the owner,
// then applies the fog / blend / cull / copy-filter state selected by the record's mode fields.
void fn_80176E20(u8* a, u8* b) {
    if (*(u8*)(a + 4) != 0) {
        f32 m[20];
        f32* bf = (f32*)b;
        m[0] = *(f32*)(a + 0x08);
        m[1] = *(f32*)(a + 0x0C);
        m[2] = *(f32*)(a + 0x10);
        m[3] = *(f32*)(a + 0x14);
        m[4] = *(f32*)(a + 0x18);
        m[5] = *(f32*)(a + 0x1C);
        m[6] = *(f32*)(a + 0x20);
        m[7] = *(f32*)(a + 0x24);
        m[8] = 1.0f;
        m[9] = 1.0f;
        m[10] = 1.0f;
        m[11] = *(f32*)(a + 0x4C);
        m[12] = *(f32*)(a + 0x28);
        m[13] = *(f32*)(a + 0x2C);
        m[14] = *(f32*)(a + 0x30);
        m[15] = *(f32*)(a + 0x34);
        m[16] = *(f32*)(a + 0x38);
        m[17] = *(f32*)(a + 0x3C);
        m[18] = *(f32*)(a + 0x40);
        m[19] = *(f32*)(a + 0x44);
        if (lbl_805FB5D4 != NULL) {
            for (s32 i = 0; i < 20; i++) m[i] = lbl_805FB5D4[i];
        }
        if (*(u8*)(a + 5) == 0) {
            m[16] = 0.0f;
            m[17] = 0.0f;
            m[18] = 0.0f;
            m[19] = 0.0f;
        }
        f32 va[4] = {m[0] * bf[0] + m[12], m[1] * bf[1] + m[13], m[2] * bf[2] + m[14],
                     m[3] * bf[3] + m[15]};
        f32 vb[4] = {m[4] * bf[4], m[5] * bf[5], m[6] * bf[6], m[7] * bf[7]};
        f32 vc[4] = {m[16], m[17], m[18], m[19]};
        f32 vd[4] = {m[8] * bf[4], m[9] * bf[5], m[10] * bf[6], m[11] * bf[7]};
        fn_801487CC(va, vb, vc, vd);
    } else {
        f32 bf[4] = {*(f32*)(b + 0x10), *(f32*)(b + 0x14), *(f32*)(b + 0x18), *(f32*)(b + 0x1C)};
        fn_80148D94(bf);
    }

    u8* owner = *(u8**)(a + 0);
    u8* r30 = *(u8**)(owner + 0x1C);
    u8* r31 = *(u8**)(owner + 0x20);
    if (*(u8**)(b + 0x64) != NULL) {
        r30 = *(u8**)(b + 0x64);
        r31 = NULL;
    }
    if (lbl_805FB5C0 != 0) {
        r30 = NULL;
        r31 = NULL;
    }
    if (r30 != NULL) {
        f32* xf = *(f32**)(owner + 0x3C);
        if (xf != NULL) fn_801776C8(xf);
        u8 c64 = *(u8*)(a + 0x64);
        u8 c65 = *(u8*)(a + 0x65);
        if (c64 == 0 || c65 == 0) {
            GxBp(0xFE00000F);
            GxBp(0x80000000u | (c64 != 0) | ((u32)(c65 != 0) << 2));
        }
        u8 c66 = *(u8*)(a + 0x66);
        u8 c67 = *(u8*)(a + 0x67);
        if (c66 == 0 || c67 == 0) {
            GxBp(0xFE000090);
            GxBp(0x80000000u | ((u32)(c66 != 0) << 7) | ((u32)(c67 != 0) << 4));
        }
        if (r31 != NULL) {
            // Width for the second image: max(a[0x50], b[0x60]), or lbl_805F9320 when it is >= 0.
            // fn_8015A39C ignores it (it takes only the descriptor), so it is not forwarded.
            fn_8015A39C(r30 + 4);
        } else {
            fn_80177748(r30 + 4);
        }
    } else {
        fn_80149594();
    }

    u32 mode = (u32)*(s32*)(a + 0x54);
    if (mode != 0 && mode <= 7) {
        static const u32 kTev[8] = {0, 0x41000420, 0x41000920, 0x41000360,
                                    0x410002A0, 0x41000420, 0x41000920, 0x41000100};
        GxBp(0xFE000FE0);
        GxBp(kTev[mode]);
    }
    if (*(u8*)(a + 0x58) == 0) fn_80154358();
    if (*(u8*)(a + 0x59) == 0) {
        GxBp(0xFE000018);
        GxBp(0x41000000);
    }
    if (*(u8*)(a + 0x5A) == 0) fn_8015428C(0);

    u32 z = (u32)*(s32*)(a + 0x5C);
    if (z != 5 && z <= 7) {
        static const u32 kZ[8] = {0x40000000, 0x4000000E, 0x40000004, 0x4000000A,
                                  0x40000002, 0, 0x40000008, 0x4000000C};
        GxBp(0xFE00000E);
        GxBp(kZ[z]);
    }

    switch (*(s32*)(a + 0x60)) {
    case 0:
        lbl_805FB50C = (u8)(lbl_805FB50C & ~8);
        GxBp(0xFE00C000);
        GxBp(0);
        break;
    case 1: {
        u8 flags = lbl_805FB50C;
        lbl_805F9212 = 1;
        if (flags & 8) {
            GxBp(0xFE00C000);
            GxBp(0x4000);
        }
        break;
    }
    default:
        break;
    }
}

// 0x80177748
// Programs the GX texture state for the record at p: BP loads for the tex size / format / address
// (only when the texture address or generation changed), then the scissor-style size registers.
void fn_80177748(u8* arg0) {
    GxBp(0xFE000060);
    GxBp(((u32)*(u8*)(arg0 + 0x12) << 5) | 0x80000000u);
    if ((u32)lbl_8040ADF0[0] != *(u32*)(arg0 + 0x18) || lbl_8040AE10[0] != 0) {
        lbl_805FB4B0 = lbl_805FB4B0 + 1;
        u32 cfg = lbl_80348AD0;
        lbl_8040ADF0[0] = *(s32*)(arg0 + 0x18);
        lbl_8040AE10[0] = 0;
        u8 tex = *(u8*)(arg0 + 0x13);
        s32 second = *(s32*)(lbl_80348A10 + tex * 0x20);
        GxBp((((u32)*(u8*)(arg0 + 0xF) - 1) & 0xF) << 12 | 0x84000000u);
        GxBp(((u32)*(u16*)(arg0 + 0x4) - 1) | (((u32)*(u16*)(arg0 + 0x6) - 1) << 10) |
             ((u32)*(u8*)(arg0 + 0x10) << 20) | 0x88000000u);
        GxBp((*(u32*)(lbl_80348950 + tex * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
        if (second >= 0) {
            GxBp(((u32)second >> 5) | 0x900D0000u | 0x8000u);
        } else {
            GxBp(0x90000000u);
        }
        GxBp(((*(u32*)(arg0 + 0x18) >> 5) & 0xFFFFFF) | 0x94000000u);
        if (*(u8*)(arg0 + 0xD) != 0) {
            u32 g = (cfg >> 9) & 0x3FF;
            GxBp(((g | 0x98000000u) & ~0x3FC00u) | (((u32)*(u8*)(arg0 + 0x11) << 10) & 0x3FC00u));
            GxBp(0xFEFFFF00u);
            GxBp(0x0F000000u);
            GxBp(((*(u32*)(arg0 + 0x1C) >> 5) & 0xFFFFFF) | 0x64000000u);
            GxBp(((g | 0x65000000u) & ~0x1FC00u) |
                 ((((s32)*(u8*)(arg0 + 0xD) >> 1) << 10) & 0x1FC00u));
            GxBp(0xFEFFFF00u);
            GxBp(0x0F000000u);
        }
    }
    GxBp(((u32)*(u16*)(arg0 + 0x4) - 1) | 0x30000000u);
    GxBp(((u32)*(u16*)(arg0 + 0x6) - 1) | 0x31000000u);
}

// 0x80177940
// Applies the render state for drawing the record a (arg1 = the draw parameters): first-use setup
// of the colour / constant registers and the line pipeline, texture or no-texture pipeline choice,
// then the fog / blend / cull / copy-filter fields of the record.
void fn_80177940(u8* arg0, u8* arg1) {
    if (*(u8*)(arg0 + 4) == 0) {
        f32 sp38 = 0.0f, sp3C = 0.0f, sp40 = 0.0f;
        f32 sp48 = 0.0f, sp4C = 0.0f, sp50 = 0.0f, sp54 = 0.0f;
        f32 sp68 = 1.0f, sp6C = 1.0f, sp70 = 1.0f, sp74 = 1.0f;
        if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x1139 > lbl_805FB4C8) {
            fn_8014DE68();
            fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
        }
        if (!(lbl_805FB50C & 1)) {
            u8 flags = lbl_805FB50C | 1;
            lbl_805FB50C = flags;
            if (flags & 2) {
                GxStateAlt();
            } else {
                GxBp(0xFE00000F);
                GxBp(0);
                GxXf1(0x103F, 0);
                GxBp(0xFE003C00);
                GxBp(0);
                GxXf1(0x1010, 0x441);
                GxBp(0x28380000);
                GxBp(0xC008F2A4);
                GxBp(0xC108E6F0);
            }
            GxXf1(0x100E, ((u32)lbl_805FB50D << 2) | 0x742);
        }
        s32 c74 = ClampS((s32)(255.0f * sp74), 0, 0xFF);
        s32 c68 = ClampS((s32)(255.0f * sp68), 0, 0xFF);
        s32 c6C = ClampS((s32)(255.0f * sp6C), 0, 0xFF);
        s32 c70 = ClampS((s32)(255.0f * sp70), 0, 0xFF);
        u32 e2 = (((u32)c74 & 0xFF) << 12) | ((u32)c68 & 0xFF);
        u32 e3 = 0xE3000000u | ((u32)c70 & 0xFF) | (((u32)c6C & 0xFF) << 12);
        GxBp(e2 | 0xE2000000u);
        GxBp(e3);
        GxBp(e3);
        GxBp(e3);
        s32 d38 = ClampS((s32)(255.0f * sp38), -0x400, 0x3FF);
        s32 d3C = ClampS((s32)(255.0f * sp3C), -0x400, 0x3FF);
        s32 d40 = ClampS((s32)(255.0f * sp40), -0x400, 0x3FF);
        GxBp(((u32)d38 & 0x7FF) | 0xF0000u | 0xF000u | 0xE4000000u);
        u32 e5 = (((u32)d40 & 0x7FF) | 0xE5000000u) & ~0x7FF000u;
        e5 |= ((u32)d3C << 12) & 0x7FF000u;
        GxBp(e5);
        GxBp(e5);
        GxBp(e5);
        s32 t48 = (s32)(255.0f * sp48);
        s32 t4C = (s32)(255.0f * sp4C);
        s32 t50 = (s32)(255.0f * sp50);
        s32 t54 = (s32)(255.0f * sp54);
        u32 r3 = (u32)ClampS(t48, 0, 0xFF);
        u32 r7 = (u32)ClampS(t4C, 0, 0xFF);
        u32 r8 = (u32)ClampS(t50, 0, 0xFF);
        u32 r9 = (u32)ClampS(t54, 0, 0xFF);
        GxXf1(0x100C, r9 | (r8 << 8) | ((r3 << 24) | (r7 << 16)) | 0xFF);
    }

    u8* owner = *(u8**)(arg0 + 0);
    void* tex = *(void**)(owner + 0x1C);
    if (*(void**)(arg1 + 0x64) != NULL) tex = *(void**)(arg1 + 0x64);
    if (lbl_805FB5C0 != 0) tex = NULL;
    if (tex != NULL) {
        if (*(void**)(owner + 0x3C) != NULL) {
            GXWGFifo._u8  = 0x10;
            GXWGFifo._u16 = 0xB;
            GXWGFifo._u16 = 0x500;
            GXWGFifo._u32 = 0x3F800000;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0x3F800000;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0;
            GXWGFifo._u32 = 0x3F800000;
            GXWGFifo._u32 = 0;
        }
        if (*(u8*)(arg0 + 0x64) == 0 || *(u8*)(arg0 + 0x65) == 0) {
            GxBp(0xFE00000F);
            GxBp(0x80000005);
        }
        if (*(u8*)(arg0 + 0x66) == 0 || *(u8*)(arg0 + 0x67) == 0) {
            GxBp(0xFE000090);
            GxBp(0x80000090);
        }
    } else if (!(lbl_805FB50C & 2)) {
        u8 flags = lbl_805FB50C | 2;
        lbl_805FB50C = flags;
        if (flags & 1) {
            GxStateAlt();
        } else {
            GxBp(0xFE00000F);
            GxBp(1);
            GxXf1(0x103F, 1);
            GxBp(0xFE003C00);
            GxBp(0);
            GxXf1(0x100E, 0x442);
            GxXf1(0x1010, 0x442);
            GxBp(0x28380040);
            GxBp(0xC008F8AF);
            GxBp(0xC108F2F0);
        }
    }

    if (*(s32*)(arg0 + 0x54) != 0) {
        GxBp(0xFE000FE0);
        GxBp(0x410004A0);
    }
    if (*(u8*)(arg0 + 0x58) == 0) fn_80154318();
    if (*(u8*)(arg0 + 0x59) == 0) {
        GxBp(0xFE000018);
        GxBp(0x41000018);
    }
    if (*(u8*)(arg0 + 0x5A) == 0) fn_8015428C(1);
    if (*(s32*)(arg0 + 0x5C) != 5) {
        GxBp(0xFE00000E);
        GxBp(0x40000006);
    }
    if (*(s32*)(arg0 + 0x60) != 2) {
        u8 flags = lbl_805FB50C;
        u8 old = lbl_805F9212;
        lbl_805FB50C = flags | 8;
        GxBp(0xFE00C000);
        GxBp((u32)old << 14);
        lbl_805F9212 = 2;
        if (lbl_805FB50C & 8) {
            GxBp(0xFE00C000);
            GxBp(2u << 14);
        }
    }
}

// 0x80178300
// Writes the record to the stream cursor *arg1: 12 flag bytes, zero padding to 4-byte alignment,
// then the 12 floats of the transform blocks through the stream's float writer.
void fn_80178300(u8* arg0, u8** arg1) {
    PutByte(arg1, *(u8*)(arg0 + 0x04));
    PutByte(arg1, *(u8*)(arg0 + 0x05));
    PutByte(arg1, (u8)*(s32*)(arg0 + 0x54));
    PutByte(arg1, *(u8*)(arg0 + 0x58));
    PutByte(arg1, *(u8*)(arg0 + 0x64));
    PutByte(arg1, *(u8*)(arg0 + 0x65));
    PutByte(arg1, *(u8*)(arg0 + 0x59));
    PutByte(arg1, *(u8*)(arg0 + 0x5A));
    PutByte(arg1, (u8)*(s32*)(arg0 + 0x5C));
    PutByte(arg1, (u8)*(s32*)(arg0 + 0x60));
    PutByte(arg1, *(u8*)(arg0 + 0x66));
    PutByte(arg1, *(u8*)(arg0 + 0x67));
    while (((uintptr_t)*arg1 & 3) != 0) PutByte(arg1, 0);
    WR_F32(arg1, *(f32*)(arg0 + 0x08));
    WR_F32(arg1, *(f32*)(arg0 + 0x0C));
    WR_F32(arg1, *(f32*)(arg0 + 0x10));
    WR_F32(arg1, *(f32*)(arg0 + 0x18));
    WR_F32(arg1, *(f32*)(arg0 + 0x1C));
    WR_F32(arg1, *(f32*)(arg0 + 0x20));
    WR_F32(arg1, *(f32*)(arg0 + 0x28));
    WR_F32(arg1, *(f32*)(arg0 + 0x2C));
    WR_F32(arg1, *(f32*)(arg0 + 0x30));
    WR_F32(arg1, *(f32*)(arg0 + 0x38));
    WR_F32(arg1, *(f32*)(arg0 + 0x3C));
    WR_F32(arg1, *(f32*)(arg0 + 0x40));
    WR_F32(arg1, *(f32*)(arg0 + 0x48));
    WR_F32(arg1, *(f32*)(arg0 + 0x4C));
    WR_F32(arg1, *(f32*)(arg0 + 0x50));
}

// 0x801787E0
// Reads a record back from the stream cursor *arg1 (the layout fn_8017A0F4 writes): the state
// word, three 3-float groups (the last two get an implicit w = 1.0f), eight 0x18-byte sub-records
// and the trailing parameter block.
void fn_801787E0(u8* arg0, u8** arg1) {
    *(u32*)(arg0 + 0x54) = GetU32(arg1);
    *(f32*)(arg0 + 0x00) = GetF32(arg1);
    *(f32*)(arg0 + 0x04) = GetF32(arg1);
    *(f32*)(arg0 + 0x08) = GetF32(arg1);
    *(f32*)(arg0 + 0x0C) = GetF32(arg1);
    f32 x = GetF32(arg1);
    f32 y = GetF32(arg1);
    f32 z = GetF32(arg1);
    *(f32*)(arg0 + 0x10) = x;
    *(f32*)(arg0 + 0x14) = y;
    *(f32*)(arg0 + 0x18) = z;
    *(f32*)(arg0 + 0x1C) = 1.0f;
    x = GetF32(arg1);
    y = GetF32(arg1);
    z = GetF32(arg1);
    *(f32*)(arg0 + 0x20) = x;
    *(f32*)(arg0 + 0x24) = y;
    *(f32*)(arg0 + 0x28) = z;
    *(f32*)(arg0 + 0x2C) = 1.0f;
    for (s32 off = 0x30; off <= 0x4C; off += 4) *(f32*)(arg0 + off) = GetF32(arg1);
    u8* p = arg0;
    for (s32 i = 0; i < 8; i++) {
        *(u16*)(p + 0x58) = GetU16(arg1);
        *(u16*)(p + 0x5A) = GetU16(arg1);
        *(u32*)(p + 0x5C) = GetU32(arg1);
        *(u16*)(p + 0x60) = GetU16(arg1);
        for (s32 k = 0x62; k <= 0x6D; k++) *(u8*)(p + k) = GetByte(arg1);
        p += 0x18;
        *arg1 += 2;
    }
    *(f32*)(arg0 + 0x118) = GetF32(arg1);
    *(u16*)(arg0 + 0x11C) = GetU16(arg1);
    *(u16*)(arg0 + 0x11E) = GetU16(arg1);
    *(u16*)(arg0 + 0x120) = GetU16(arg1);
    *(u16*)(arg0 + 0x122) = GetU16(arg1);
    *(u16*)(arg0 + 0x124) = GetU16(arg1);
    *(u16*)(arg0 + 0x126) = GetU16(arg1);
    *(u32*)(arg0 + 0x128) = GetU32(arg1);
    *(u32*)(arg0 + 0x12C) = GetU32(arg1);
    *(u16*)(arg0 + 0x130) = GetU16(arg1);
    *(u16*)(arg0 + 0x132) = GetU16(arg1);
    *(u16*)(arg0 + 0x134) = GetU16(arg1);
    *(u8*)(arg0 + 0x136) = GetByte(arg1);
    *(u8*)(arg0 + 0x137) = GetByte(arg1);
    *(u8*)(arg0 + 0x138) = GetByte(arg1);
    *(u8*)(arg0 + 0x139) = GetByte(arg1);
    *arg1 += 2;
}

// 0x80178FC0
// Property setter by id: 0x3A4 sets the state word, 0x3A5..0x3B6 are raw float copies, 0x3B7..0x476
// address eight 0x18-byte sub-records (24 properties each), 0x477..0x48C the trailing parameter
// block. Float values arrive as raw 32-bit words (val).
void fn_80178FC0(u8* obj, s32 idx, s32 val) {
    if (idx < 0x3A5) {
        if (idx == 0x3A4) *(s32*)(obj + 0x54) = val;
        return;
    }
    if (idx < 0x3B7) {
        static const u8 kOff[18] = {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x20, 0x24,
                                    0x28, 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C};
        *(s32*)(obj + kOff[idx - 0x3A5]) = val;
        return;
    }
    if (idx < 0x477) {
        s32 n = (idx - 0x3B7) / 0x18;
        u8* rec = obj + n * 0x18;
        u32 k = (u32)(idx - n * 0x18 - 0x3B7);
        f32 f = U2F((u32)val);
        u32 w;
        switch (k) {
        case 0: *(u16*)(rec + 0x58) = (u16)ClampS((s32)lrintf(256.0f * f), 0, 0xC00); break;
        case 1: *(u16*)(rec + 0x5A) = (u16)ClampS((s32)lrintf(256.0f * f), 0, 0xC00); break;
        case 2: *(u16*)(rec + 0x60) = (u16)((u32)lrintf(256.0f * f) & 0x1FFF); break;
        case 3: *(u8*)(rec + 0x62) = (u8)(val + 1); break;
        case 4: *(u8*)(rec + 0x63) = (u8)(val + 1); break;
        case 5: *(u8*)(rec + 0x64) = (u8)(val + 1); break;
        case 6:
            w = *(u32*)(rec + 0x5C) & 0xFF00FFFFu;
            *(u32*)(rec + 0x5C) = w | ((u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF) << 16);
            break;
        case 7:
            w = *(u32*)(rec + 0x5C) & 0xFFFF00FFu;
            *(u32*)(rec + 0x5C) = w | ((u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF) << 8);
            break;
        case 8:
            w = *(u32*)(rec + 0x5C) & 0xFFFFFF00u;
            *(u32*)(rec + 0x5C) = w | (u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF);
            break;
        case 9:
            w = *(u32*)(rec + 0x5C) & 0x00FFFFFFu;
            *(u32*)(rec + 0x5C) = w | ((u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF) << 24);
            break;
        case 10: *(u8*)(rec + 0x65) = (u8)val; break;
        case 11: *(u8*)(rec + 0x66) = (u8)val; break;
        case 12: *(u8*)(rec + 0x67) = (u8)((*(u8*)(rec + 0x67) & 0xFE) | (val != 0 ? 0x01 : 0)); break;
        case 13: *(u8*)(rec + 0x67) = (u8)((*(u8*)(rec + 0x67) & ~0x02) | (val != 0 ? 0x02 : 0)); break;
        case 14: *(u8*)(rec + 0x67) = (u8)((*(u8*)(rec + 0x67) & ~0x04) | (val != 0 ? 0x04 : 0)); break;
        case 15:
            if (val == 0) *(u8*)(rec + 0x68) = 1;
            else if (val == 1) *(u8*)(rec + 0x68) = 2;
            else if (val == 2) *(u8*)(rec + 0x68) = 7;
            break;
        case 16:
            if (val == 0) *(u8*)(rec + 0x69) = 1;
            else if (val == 1) *(u8*)(rec + 0x69) = 2;
            else if (val == 2) *(u8*)(rec + 0x69) = 4;
            break;
        case 17:
            if (val == 0) *(u8*)(rec + 0x6A) = 1;
            else if (val == 1) *(u8*)(rec + 0x6A) = 2;
            break;
        case 18: *(u8*)(rec + 0x6B) = (u8)(val + 1); break;
        case 19: *(u8*)(rec + 0x6C) = (u8)val; break;
        case 20: *(u8*)(rec + 0x6D) = (u8)((*(u8*)(rec + 0x6D) & 0xFE) | (val != 0 ? 0x01 : 0)); break;
        case 21: *(u8*)(rec + 0x6D) = (u8)((*(u8*)(rec + 0x6D) & ~0x02) | (val != 0 ? 0x02 : 0)); break;
        case 22: *(u8*)(rec + 0x6D) = (u8)((*(u8*)(rec + 0x6D) & ~0x04) | (val != 0 ? 0x04 : 0)); break;
        case 23: *(u8*)(rec + 0x6D) = (u8)(*(u8*)(rec + 0x6D) | (val != 0 ? 0x08 : 0)); break;  // only sets
        default: break;
        }
        return;
    }
    u32 k = (u32)(idx - 0x477);
    if (k > 0x15) return;
    f32 f = U2F((u32)val);
    u32 w;
    switch (k) {
    case 0: *(s32*)(obj + 0x118) = val; break;
    case 1: {
        *(u8*)(obj + 0x136) = (u8)val;
        u8 v = (u8)val;
        u8* sub = *(u8**)(obj + 0x50);
        if (v == 2) *(u8*)(sub + 0xD) = 2;
        else if (v == 3) *(u8*)(sub + 0xD) = 3;
        else *(u8*)(sub + 0xD) = 1;
        *(u8*)(*(u8**)(obj + 0x50) + 0xE) = 1;
        break;
    }
    case 2: *(u16*)(obj + 0x11C) = fn_80178CC4((u32)val); break;
    case 3: *(u16*)(obj + 0x11E) = fn_80178DEC((u32)val); break;
    case 4: *(u16*)(obj + 0x120) = fn_80178DEC((u32)val); break;
    case 5: *(u16*)(obj + 0x122) = fn_80178CC4((u32)val); break;
    case 6: *(u16*)(obj + 0x124) = fn_80178DEC((u32)val); break;
    case 7: *(u16*)(obj + 0x126) = fn_80178DEC((u32)val); break;
    case 8:
        w = *(u32*)(obj + 0x128) & 0xFF00FFFFu;
        *(u32*)(obj + 0x128) = w | ((u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF) << 16);
        break;
    case 9:
        w = *(u32*)(obj + 0x128) & 0xFFFF00FFu;
        *(u32*)(obj + 0x128) = w | ((u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF) << 8);
        break;
    case 10:
        w = *(u32*)(obj + 0x128) & 0xFFFFFF00u;
        *(u32*)(obj + 0x128) = w | (u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF);
        break;
    case 11:
        w = *(u32*)(obj + 0x128) & 0x00FFFFFFu;
        *(u32*)(obj + 0x128) = w | ((u32)ClampS((s32)lrintf(255.0f * f), 0, 0xFF) << 24);
        break;
    case 12: *(u16*)(obj + 0x134) = (val != 0) ? (u16)(val + 0x14FF) : 0; break;
    case 13: *(u16*)(obj + 0x132) = (u16)(val + 0x200); break;
    case 14: *(u32*)(obj + 0x12C) = (*(u32*)(obj + 0x12C) & ~0x00010000u) | (val != 0 ? 0x00010000u : 0); break;
    case 15: *(u32*)(obj + 0x12C) = (*(u32*)(obj + 0x12C) & ~0x00000100u) | (val != 0 ? 0x00000100u : 0); break;
    case 16: *(u32*)(obj + 0x12C) = (*(u32*)(obj + 0x12C) & ~0x00000001u) | (val != 0 ? 0x00000001u : 0); break;
    case 17: *(u32*)(obj + 0x12C) = (*(u32*)(obj + 0x12C) & ~0x01000000u) | (val != 0 ? 0x01000000u : 0); break;
    case 18: *(u8*)(obj + 0x137) = (val != 0); break;
    case 19: {
        u32 r = 0x405;
        if (val == 0) r = 0;
        else if (val == 1) r = 0x404;
        else if (val == 2) r = 0x405;
        else if (val == 3) r = 0x408;
        *(u16*)(obj + 0x130) = (u16)r;
        break;
    }
    case 20: *(u8*)(obj + 0x138) = (val != 0); break;
    case 21: *(u8*)(obj + 0x139) = (val != 0); break;
    default: break;
    }
}

// 0x801799BC
// Property getter matching fn_80178FC0 (same ids): returns the value as a raw 32-bit word (floats
// as their bit pattern; sub-record ids select one of eight 0x18-byte records). 0x3A3 reads 1.
u32 fn_801799BC(u8* obj, s32 idx) {
    u32 r = 0;
    if (idx < 0x3A5) {
        if (idx == 0x3A4) return (u32) * (s32*)(obj + 0x54);
        if (idx >= 0x3A3) r = 1;
        return r;
    }
    if (idx < 0x3B7) {
        static const u8 kOff[18] = {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x20, 0x24,
                                    0x28, 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C};
        return *(u32*)(obj + kOff[idx - 0x3A5]);
    }
    if (idx < 0x477) {
        s32 n = (idx - 0x3B7) / 0x18;
        u8* rec = obj + n * 0x18;
        u32 k = (u32)(idx - n * 0x18 - 0x3B7);
        u32 w;
        switch (k) {
        case 0: r = F2U(0.00390625f * (f32)*(u16*)(rec + 0x58)); break;
        case 1: r = F2U(0.00390625f * (f32)*(u16*)(rec + 0x5A)); break;
        case 2: {
            u32 hi = (u32)*(u16*)(rec + 0x60) >> 12;
            s32 v = (s32)((~(hi - 1) & ~0xFFFu) | *(u16*)(rec + 0x5A));
            r = F2U(0.00390625f * (f32)v);
            break;
        }
        case 3: r = (u32)(*(u8*)(rec + 0x62) - 1); break;
        case 4: r = (u32)(*(u8*)(rec + 0x63) - 1); break;
        case 5: r = (u32)(*(u8*)(rec + 0x64) - 1); break;
        case 6: w = *(u32*)(rec + 0x5C); r = F2U((1.0f / 255.0f) * (f32)((w >> 16) & 0xFF)); break;
        case 7: w = *(u32*)(rec + 0x5C); r = F2U((1.0f / 255.0f) * (f32)((w >> 8) & 0xFF)); break;
        case 8: w = *(u32*)(rec + 0x5C); r = F2U((1.0f / 255.0f) * (f32)(w & 0xFF)); break;
        case 9: w = *(u32*)(rec + 0x5C); r = F2U((1.0f / 255.0f) * (f32)(w >> 24)); break;
        case 10: r = *(u8*)(rec + 0x65); break;
        case 11: r = *(u8*)(rec + 0x66); break;
        case 12: r = *(u8*)(rec + 0x67) & 1; break;
        case 13: r = (*(u8*)(rec + 0x67) >> 1) & 1; break;
        case 14: r = (*(u8*)(rec + 0x67) >> 2) & 1; break;
        case 15: {
            u8 c = *(u8*)(rec + 0x68);
            r = (c == 2) ? 1 : (c == 7) ? 2 : 0;
            break;
        }
        case 16: {
            u8 c = *(u8*)(rec + 0x69);
            r = (c == 2) ? 1 : (c == 4) ? 2 : 0;
            break;
        }
        case 17: r = (*(u8*)(rec + 0x6A) == 2) ? 1 : 0; break;
        case 18: r = (u32)(*(u8*)(rec + 0x6B) - 1); break;
        case 19: r = *(u8*)(rec + 0x6C); break;
        case 20: r = *(u8*)(rec + 0x6D) & 1; break;
        case 21: r = (*(u8*)(rec + 0x6D) >> 1) & 1; break;
        case 22: r = (*(u8*)(rec + 0x6D) >> 2) & 1; break;
        case 23: r = (*(u8*)(rec + 0x6D) >> 3) & 1; break;
        default: break;
        }
        return r;
    }
    u32 k = (u32)(idx - 0x477);
    if (k > 0x15) return 0;
    u32 w;
    switch (k) {
    case 0: r = *(u32*)(obj + 0x118); break;
    case 1: r = *(u8*)(obj + 0x136); break;
    case 2: r = (u32)fn_80178D2C(*(u16*)(obj + 0x11C)); break;
    case 3: r = (u32)fn_80178E9C(*(u16*)(obj + 0x11E)); break;
    case 4: r = (u32)fn_80178E9C(*(u16*)(obj + 0x120)); break;
    case 5: r = (u32)fn_80178D2C(*(u16*)(obj + 0x122)); break;
    case 6: r = (u32)fn_80178E9C(*(u16*)(obj + 0x124)); break;
    case 7: r = (u32)fn_80178E9C(*(u16*)(obj + 0x126)); break;
    case 8: w = *(u32*)(obj + 0x128); r = F2U((1.0f / 255.0f) * (f32)((w >> 16) & 0xFF)); break;
    case 9: w = *(u32*)(obj + 0x128); r = F2U((1.0f / 255.0f) * (f32)((w >> 8) & 0xFF)); break;
    case 10: w = *(u32*)(obj + 0x128); r = F2U((1.0f / 255.0f) * (f32)(w & 0xFF)); break;
    case 11: w = *(u32*)(obj + 0x128); r = F2U((1.0f / 255.0f) * (f32)(w >> 24)); break;
    case 12: {
        u32 v = *(u16*)(obj + 0x134);
        r = (v != 0) ? (v - 0x14FF) : 0;
        break;
    }
    case 13: r = (u32)(*(u16*)(obj + 0x132) - 0x200); break;
    case 14: r = (*(u32*)(obj + 0x12C) >> 16) & 1; break;
    case 15: r = (*(u32*)(obj + 0x12C) >> 8) & 1; break;
    case 16: r = *(u32*)(obj + 0x12C) & 1; break;
    case 17: r = (*(u32*)(obj + 0x12C) >> 24) & 1; break;
    case 18: r = (*(u8*)(obj + 0x137) != 0); break;
    case 19: r = 0; break;
    case 20: r = (*(u8*)(obj + 0x138) != 0); break;
    case 21: r = (*(u8*)(obj + 0x139) != 0); break;
    default: break;
    }
    return r;
}

// 0x8017A0F4
// Writes the record to the stream cursor *arg1 through the stream writer slots: the layout
// fn_801787E0 reads back (state word, floats, eight 0x18-byte sub-records, parameter block).
void fn_8017A0F4(u8* arg0, u8** arg1) {
    WR_U32(arg1, *(u32*)(arg0 + 0x54));
    WR_F32(arg1, *(f32*)(arg0 + 0x00));
    WR_F32(arg1, *(f32*)(arg0 + 0x04));
    WR_F32(arg1, *(f32*)(arg0 + 0x08));
    WR_F32(arg1, *(f32*)(arg0 + 0x0C));
    WR_F32(arg1, *(f32*)(arg0 + 0x10));
    WR_F32(arg1, *(f32*)(arg0 + 0x14));
    WR_F32(arg1, *(f32*)(arg0 + 0x18));
    WR_F32(arg1, *(f32*)(arg0 + 0x20));
    WR_F32(arg1, *(f32*)(arg0 + 0x24));
    WR_F32(arg1, *(f32*)(arg0 + 0x28));
    for (s32 off = 0x30; off <= 0x4C; off += 4) WR_F32(arg1, *(f32*)(arg0 + off));
    u8* p = arg0;
    for (s32 i = 0; i < 8; i++) {
        WR_U16(arg1, *(u16*)(p + 0x58));
        WR_U16(arg1, *(u16*)(p + 0x5A));
        WR_U32(arg1, *(u32*)(p + 0x5C));
        WR_U16(arg1, *(u16*)(p + 0x60));
        for (s32 k = 0x62; k <= 0x6D; k++) PutByte(arg1, *(u8*)(p + k));
        WR_U16(arg1, 0);
        p += 0x18;
    }
    WR_F32(arg1, *(f32*)(arg0 + 0x118));
    WR_U16(arg1, *(u16*)(arg0 + 0x11C));
    WR_U16(arg1, *(u16*)(arg0 + 0x11E));
    WR_U16(arg1, *(u16*)(arg0 + 0x120));
    WR_U16(arg1, *(u16*)(arg0 + 0x122));
    WR_U16(arg1, *(u16*)(arg0 + 0x124));
    WR_U16(arg1, *(u16*)(arg0 + 0x126));
    WR_U32(arg1, *(u32*)(arg0 + 0x128));
    WR_U32(arg1, *(u32*)(arg0 + 0x12C));
    WR_U16(arg1, *(u16*)(arg0 + 0x130));
    WR_U16(arg1, *(u16*)(arg0 + 0x132));
    WR_U16(arg1, *(u16*)(arg0 + 0x134));
    for (s32 k = 0x136; k <= 0x139; k++) PutByte(arg1, *(u8*)(arg0 + k));
    WR_U16(arg1, 0);
}

}  // extern "C"
