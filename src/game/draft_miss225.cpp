// Renderer GX helpers in 0x801487CC..0x8015EB3C: BP/XF register writes through the write-gather
// pipe, texture/display-copy setup, procedural quad and grid vertex builders, texture header
// sizing and the GX copy/viewport reset.
#include "types.h"
#include "revolution.h"
#include "engine/rend.h"
#include "engine/math.h"

// fn_8015490C is defined in src/game/small_8015.cpp with C++ linkage.
extern "C++" void fn_8015490C(u8* p);                                     // 0x8015490C

extern "C" {
void Vec3Copy(Vec3* out, const Vec3* v);                                  // 0x80052640
s32  VideoIsPal50(RendVideo* video);                                      // 0x8015DE24
void fn_800359F4(u8* a3, f32 f1, f32 f2, f32 f3);                         // 0x800359F4
void fn_800526D0(u8* out);                                                // 0x800526D0
void fn_8005535C(u8* dst, const u8* src);                                 // 0x8005535C
void fn_800554DC(u8* out, const u8* a, const u8* b);                      // 0x800554DC
void fn_8014ACCC(u32 mode, s32 fmt, s32 w, s32 h, s32* outSize, s32* outStride, s32* outW, s32* outH);  // 0x8014ACCC
void fn_8014DE68(void);                                                   // 0x8014DE68
void fn_8014E3EC(s32 n);                                                  // 0x8014E3EC
u32  fn_8014EACC(void* a3, s32 a4, s32 a5, s32 a6);                       // 0x8014EACC
s32  fn_80151644(s32 a, s32 b, u8* quad);                                 // 0x80151644
s32  fn_8015BA30(void* p);                                                // 0x8015BA30
void fn_8015D660(void);                                                   // 0x8015D660
void GXSetVtxDesc(s32 a, s32 b);                                           // 0x8024A3A4
void GXClearVtxDesc(void);                                                   // 0x8024A7E4
void GXSetVtxAttrFmt(s32 a, s32 b, s32 c, s32 d, s32 e);                      // 0x8024A818
void GXInvalidateVtxCache(void);                                                   // 0x8024AC3C
void GXBegin(s32 a, s32 b, s32 c);                                    // 0x8024B974
void fn_8024BBE4(s32 x, s32 y, s32 w, s32 h);                             // 0x8024BBE4
void fn_8024BC24(s32 w, u32 h);                                           // 0x8024BC24
u32  fn_8024BC90(f32 yScale);                                             // 0x8024BC90
void fn_8024BD5C(u8* color, u32 z);                                       // 0x8024BD5C
void fn_8024BDD4(u32 aa, u8* pattern, s32 vf, u8* vfilter);               // 0x8024BDD4
void fn_8024BFB8(s32 a);                                                  // 0x8024BFB8
void fn_8024BFCC(void* fb, s32 clear);                                    // 0x8024BFCC
void fn_8024CB00(void);                                                   // 0x8024CB00
void GXSetColorUpdate(s32 a);                                                  // 0x8024DF28
void GXSetZMode(s32 a, s32 b, s32 c);                                    // 0x8024DF80
void GXSetPixelFmt(s32 a, s32 b);                                           // 0x8024DFE4
void GXSetDither(u32 a);                                                  // 0x8024E08C
void GXSetViewport(f32 left, f32 top, f32 w, f32 h, f32 nearZ, f32 farZ);   // 0x8024E48C
void GXSetScissor(s32 x, s32 y, s32 w, s32 h);                             // 0x8024E4B8

extern u8   lbl_804010A8[];         // 0x804010A8 (base matrix)
extern u8   sRendDLs[];             // 0x80401108 (display-list records, 0x94 bytes each)
extern u8   lbl_80348950[];         // 0x80348950 (texture address table, 0x20 bytes per format)
extern u8   lbl_80348A10[];         // 0x80348A10
extern u32  lbl_80348AD0[];         // 0x80348AD0
extern u8   lbl_803765A0[];         // 0x803765A0 (7-byte texture parameter entries)
extern u8   lbl_8040AE48[];         // 0x8040AE48 (display copy settings)
extern s32  lbl_8040ADF0[8];        // 0x8040ADF0
extern s32  lbl_8040AE10[8];        // 0x8040AE10
extern s32  gViewportIndex;         // 0x805FB4BC
extern s32  lbl_805FB4B0;           // 0x805FB4B0
extern s32  lbl_805FB4C4;           // 0x805FB4C4
extern s32  gFrameBudget;           // 0x805FB4C8
extern s32  lbl_805FB4CC;           // 0x805FB4CC
extern u8*  lbl_805FB4D0;           // 0x805FB4D0
extern s32  lbl_805FB500;           // 0x805FB500
extern s32  lbl_805FB504;           // 0x805FB504
extern s32  lbl_805FB508;           // 0x805FB508
extern u8   lbl_805FB50C;           // 0x805FB50C (GX shadow flags byte)
extern u8   lbl_805FB50D;           // 0x805FB50D (GX shadow mask byte)
extern s32  lbl_805FB514;           // 0x805FB514 (GX shadow word)
extern void* lbl_805FB538[];        // 0x805FB538 (frame buffer pointers)
extern s32  lbl_805FB598;           // 0x805FB598
extern u8   lbl_805F921C;           // 0x805F921C
extern u8   lbl_805F9254[];         // 0x805F9254 (copy vertical filter)
}

// Writes one BP register command (opcode 0x61) through the write-gather pipe.
static inline void GxBp(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// Writes one CP/XF register command (opcode 0x10): 16-bit index 0, 16-bit register, 32-bit value.
static inline void GxXf(u16 reg, u32 value) {
    GXWGFifo._u8  = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = reg;
    GXWGFifo._u32 = value;
}

// Clamps v to 0..255 (the float-to-byte conversion used for colors).
static inline s32 ClampByte(f32 v) {
    s32 i = (s32)v;
    if (i < 0) return 0;
    if (i > 0xFF) return 0xFF;
    return i;
}

// Clamps v to -0x400..0x3FF (11-bit signed field).
static inline s32 ClampS11(f32 v) {
    s32 i = (s32)v;
    if (i < -0x400) return -0x400;
    if (i > 0x3FF) return 0x3FF;
    return i;
}

// Builds a Vec3 through the vector constructor and copies it to dst.
static inline void SetVec3(u8* dst, f32 x, f32 y, f32 z) {
    Vec3 t;
    fn_800359F4((u8*)&t, x, y, z);
    Vec3Copy((Vec3*)dst, &t);
}

// Reads the u16 at p + off.
static inline s32 U16(const u8* p, s32 off) { return *(const u16*)(p + off); }

// Callback table slot 0 of gRendMem: (size, align, type, name, extra) -> block.
typedef void* (*RendAllocFn)(u32 size, u32 align, s32 type, const char* name, void* extra);

extern "C" {

// 0x801487CC
// Sets up the TEV color registers: first makes sure the display list has room and sends the
// one-time vertex descriptor setup (variant chosen by flag bit 1), then packs the colors in
// arg3 (0..1 floats, 4 values), the signed colors in arg0 and the four values in arg1 into BP
// register writes.
void fn_801487CC(const f32* arg0, const f32* arg1, const f32* arg2, const f32* arg3) {
    (void)arg2;
    if (fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x1139 > gFrameBudget) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + gViewportIndex * 0x94);
    }
    if (!(lbl_805FB50C & 1)) {
        u8 flags = lbl_805FB50C | 1;
        lbl_805FB50C = flags;
        if (flags & 2) {
            GxBp(0xFE00000F);
            GxBp(1);
            GxXf(0x103F, 1);
            GxBp(0xFE003C00);
            GxBp(0x400);
            GxXf(0x1010, 0x441);
            GxBp(0x283C0000);
            GxBp(0xC008F2A4);
            GxBp(0xC108E6F0);
            GxBp(0xC208F80F);
            GxBp(0xC308F070);
        } else {
            GxBp(0xFE00000F);
            GxBp(0);
            GxXf(0x103F, 0);
            GxBp(0xFE003C00);
            GxBp(0);
            GxXf(0x1010, 0x441);
            GxBp(0x28380000);
            GxBp(0xC008F2A4);
            GxBp(0xC108E6F0);
        }
        GxXf(0x100E, (u32)lbl_805FB50D * 4 | 0x742);
    }
    // arg3: four colors 0..1 packed into two BP words (E2 = r/a, E3 = b/g).
    s32 c3 = ClampByte(255.0f * arg3[3]);
    s32 c0 = ClampByte(255.0f * arg3[0]);
    s32 c1 = ClampByte(255.0f * arg3[1]);
    s32 c2 = ClampByte(255.0f * arg3[2]);
    u32 w2 = (((u32)c3 & 0xFF) << 12) | ((u32)c0 & 0xFF) | 0xE2000000;
    u32 w3 = ((u32)c2 & 0xFF) | 0xE3000000;
    w3 = (w3 & ~0xFF000u) | (((u32)c1 & 0xFF) << 12);
    GxBp(w2);
    GxBp(w3);
    GxBp(w3);
    GxBp(w3);
    // arg0: three signed 11-bit values (E4 and E5, the latter sent three times).
    s32 q0 = ClampS11(255.0f * arg0[0]);
    s32 q1 = ClampS11(255.0f * arg0[1]);
    s32 q2 = ClampS11(255.0f * arg0[2]);
    GxBp(((u32)q0 & 0x7FF) | 0xFF000 | 0xE4000000);
    u32 w5 = ((u32)q2 & 0x7FF) | 0xE5000000;
    w5 = (w5 & ~0x7FF000u) | (((u32)q1 << 12) & 0x7FF000);
    GxBp(w5);
    GxBp(w5);
    GxBp(w5);
    // arg1: four 0..1 values packed into a CP register word.
    s32 d0 = ClampByte(255.0f * arg1[0]);
    s32 d1 = ClampByte(255.0f * arg1[1]);
    s32 d2 = ClampByte(255.0f * arg1[2]);
    s32 d3 = ClampByte(255.0f * arg1[3]);
    GxXf(0x100C, (u32)d3 | ((u32)d2 << 8) | (((u32)d0 << 24) | ((u32)d1 << 16)) | 0xFF);
}

// 0x80154398
// Builds the six cube-face view matrices (and four scaled variants) and uploads each one with
// fn_8014EACC / fn_8015490C.
void fn_80154398(void) {
    u8 mFaceA[0x30];   // sp368
    u8 mFaceB[0x30];   // sp338
    u8 mProj0[0x30];   // sp308
    u8 mOut[0x30];     // sp2D8
    u8 mFaceC[0x30];   // sp2A8
    u8 mProj1[0x30];   // sp278
    u8 mT0[0x30];      // sp248
    u8 mT1[0x30];      // sp218
    u8 mT2[0x30];      // sp1E8
    u8 mT3[0x30];      // sp1B8
    u8 mT4[0x30];      // sp188

    fn_800526D0(mFaceA);
    fn_800526D0(mFaceB);
    fn_800526D0(mProj0);
    fn_800526D0(mOut);
    SetVec3(mFaceB + 0x00, -1.0f, 0.0f, 0.0f);
    SetVec3(mFaceB + 0x0C, 0.0f, 0.0f, -1.0f);
    SetVec3(mFaceB + 0x18, 0.0f, 1.0f, 0.0f);
    SetVec3(mFaceB + 0x24, 0.0f, 0.0f, 1.0f);
    SetVec3(mFaceA + 0x00, 1.0f, 0.0f, 0.0f);
    SetVec3(mFaceA + 0x0C, 0.0f, 0.0f, 1.0f);
    SetVec3(mFaceA + 0x18, 0.0f, 1.0f, 0.0f);
    SetVec3(mFaceA + 0x24, 0.0f, 0.0f, 1.0f);
    SetVec3(mProj0 + 0x00, 0.45f, 0.0f, 0.0f);
    SetVec3(mProj0 + 0x0C, 0.0f, -0.45f, 0.0f);
    SetVec3(mProj0 + 0x18, 0.5f, 0.5f, 1.0f);
    SetVec3(mProj0 + 0x24, 0.0f, 0.0f, 0.0f);
    fn_800554DC(mT1, lbl_804010A8, mFaceA);
    fn_800554DC(mT0, mT1, mProj0);
    fn_8005535C(mOut, mT0);
    fn_8014EACC(&lbl_805FB598, 0x500, 0xCC, 0xC);
    fn_8015490C(mOut);
    fn_800554DC(mT3, lbl_804010A8, mFaceB);
    fn_800554DC(mT2, mT3, mProj0);
    fn_8005535C(mOut, mT2);
    fn_8014EACC(&lbl_805FB598, 0x500, 0xD8, 0xC);
    fn_8015490C(mOut);
    fn_800526D0(mFaceC);
    fn_800526D0(mProj1);
    SetVec3(mProj1 + 0x00, 0.5f, 0.0f, 0.0f);
    SetVec3(mProj1 + 0x0C, 0.0f, -0.5f, 0.0f);
    SetVec3(mProj1 + 0x18, 0.5f, 0.5f, 1.0f);
    SetVec3(mProj1 + 0x24, 0.0f, 0.0f, 0.0f);
    SetVec3(mFaceC + 0x00, 1.0f, 0.0f, 0.0f);
    SetVec3(mFaceC + 0x0C, 0.0f, 1.0f, 0.0f);
    SetVec3(mFaceC + 0x18, 0.0f, 0.0f, 0.0f);
    SetVec3(mFaceC + 0x24, 0.0f, 0.0f, 1.0f);
    fn_800554DC(mT4, mFaceC, mProj1);
    fn_8005535C(mOut, mT4);
    fn_8014EACC(&lbl_805FB598, 0x500, 0xE4, 0xC);
    fn_8015490C(mOut);
    SetVec3(mOut + 0x00, 0.125f, 0.0f, 0.0f);
    SetVec3(mOut + 0x0C, 0.0f, 0.125f, 0.0f);
    SetVec3(mOut + 0x18, 0.0f, 0.0f, 0.0f);
    SetVec3(mOut + 0x24, 0.0f, 0.0f, 1.0f);
    fn_8014EACC(&lbl_805FB598, 0x500, 0xA8, 0xC);
    fn_8015490C(mOut);
    SetVec3(mOut + 0x00, 0.25f, 0.0f, 0.0f);
    SetVec3(mOut + 0x0C, 0.0f, 0.25f, 0.0f);
    SetVec3(mOut + 0x18, 0.0f, 0.0f, 0.0f);
    SetVec3(mOut + 0x24, 0.0f, 0.0f, 1.0f);
    fn_8014EACC(&lbl_805FB598, 0x500, 0xB4, 0xC);
    fn_8015490C(mOut);
    SetVec3(mOut + 0x00, 0.5f, 0.0f, 0.0f);
    SetVec3(mOut + 0x0C, 0.0f, 0.5f, 0.0f);
    SetVec3(mOut + 0x18, 0.0f, 0.0f, 0.0f);
    SetVec3(mOut + 0x24, 0.0f, 0.0f, 1.0f);
    fn_8014EACC(&lbl_805FB598, 0x500, 0xC0, 0xC);
    fn_8015490C(mOut);
}

// 0x801578D8
// Texture setup for an image header p (u16 width/height, format byte at +9, address at +0x18):
// mode 0 = progressive, 1 = even field, 2 = odd field of an interlaced image. Sends the BP
// registers describing the image (size, address, format, filter) and finally the shadow word
// lbl_805FB514.
void fn_801578D8(u8* p, u32 a, s32 mode) {
    s32 sizeOut = 0;
    s32 strideOut = 0;
    s32 pal = VideoIsPal50(&gVideo);
    u32 w = *(u16*)(p + 0x0);
    u32 h = *(u16*)(p + 0x2);
    fn_8014ACCC(4, *(u8*)(p + 0x9), (s32)w, (s32)h, &sizeOut, &strideOut, NULL, NULL);
    u32 addr = *(u32*)(p + 0x18);
    if (mode != 0) {
        h = (u16)((s32)h >> 1);
        if (mode == 2) addr += strideOut * h;
    }
    const u8* e;
    s32 sel;
    u32 flags;
    s32 fmt;
    if (mode == 1) {
        sel = 0;
        h = pal == 0 ? h + 2 : 0x106;
        flags = lbl_805FB508 | 0x42;
        e = lbl_803765A0 + lbl_805F921C * 7;
        fmt = 3;
    } else if (mode == 2) {
        sel = 2;
        h = pal == 0 ? h : 0x104;
        flags = lbl_805FB508 | 0x42;
        e = lbl_803765A0 + lbl_805F921C * 7;
        fmt = 2;
    } else {
        e = lbl_803765A0;
        sel = 0;
        flags = lbl_805FB500 | 0x40;
        fmt = 3;
    }
    u32 top = a >> 24;
    u32 lo = flags & 7;
    if (top != 0) lo = top;
    GxBp(lo | (flags & 0x18) | 0x43000000);
    GxBp(((u32)e[3] << 18) | (((u32)e[2] << 12) | ((u32)e[0] | ((u32)e[1] << 6))) | 0x53000000);
    GxBp((u32)e[4] | ((u32)e[5] << 6) | (((u32)e[6] << 12) | 0x54000000));
    GxBp(((u32)sel << 10) | 0x49000000);
    GxBp(((h - 1) << 10) | ((w - 1) | 0x4A000000));
    GxBp(((addr >> 5) & 0xFFFFFF) | 0x4B000000);
    GxBp(((s32)w >> ((a >> 16) & 0xFF)) | 0x4D000000);
    GxBp((a & 0xFFFF) | (u32)fmt | 0x52000000);
    GxBp((u32)lbl_805FB514 | 0x43000000);
}

// 0x80157B28
// Same as fn_801578D8 for a double-width image (width and field height doubled, first table
// entry, fixed format 0x203).
void fn_80157B28(u8* p, u32 a, s32 mode) {
    s32 sizeOut = 0;
    s32 strideOut = 0;
    s32 pal = VideoIsPal50(&gVideo);
    u32 w = *(u16*)(p + 0x0);
    u32 h = *(u16*)(p + 0x2);
    fn_8014ACCC(4, *(u8*)(p + 0x9), (s32)w, (s32)h, &sizeOut, &strideOut, NULL, NULL);
    u32 addr = *(u32*)(p + 0x18);
    if (mode != 0) {
        h = (u16)((s32)h >> 1);
        if (mode == 2) addr += strideOut * h;
    }
    u32 h2 = h * 2;
    s32 sel;
    u32 flags;
    if (mode == 1) {
        sel = 0;
        h2 = pal != 0 ? 0x108 : h2;
        flags = lbl_805FB508 | 0x42;
    } else if (mode == 2) {
        sel = pal != 0 ? 6 : 2;
        h2 = pal != 0 ? 0x100 : h2;
        flags = lbl_805FB508 | 0x42;
        addr += strideOut * (pal != 0 ? 2 : 0);
    } else {
        sel = 0;
        flags = lbl_805FB500 | 0x40;
    }
    u32 top = a >> 24;
    u32 lo = flags & 7;
    if (top != 0) lo = top;
    const u8* e = lbl_803765A0;
    GxBp(lo | (flags & 0x18) | 0x43000000);
    GxBp(((u32)e[3] << 18) | (((u32)e[2] << 12) | ((u32)e[0] | ((u32)e[1] << 6))) | 0x53000000);
    GxBp((u32)e[4] | ((u32)e[5] << 6) | (((u32)e[6] << 12) | 0x54000000));
    GxBp(((u32)sel << 10) | 0x49000000);
    GxBp(((h2 - 1) << 10) | 0x4A000000 | ((w * 2) - 1));
    GxBp(((addr >> 5) & 0xFFFFFF) | 0x4B000000);
    GxBp(((s32)w >> ((a >> 16) & 0xFF)) | 0x4D000000);
    GxBp((a & 0xFFFF) | 0x52000000 | 0x203);
    GxBp((u32)lbl_805FB514 | 0x43000000);
}

// 0x80157D6C
// Draws image header src as a screen quad at pixel position (x, y): programs sampler 0 from src
// (skipped when it already holds the same image), then builds a 4-vertex quad (0x18 bytes each:
// u, v, color, x, y, z) in the vertex scratch buffer and submits it through fn_80151644 and
// fn_801578D8(dst).
void fn_80157D6C(u8* dst, u8* src, s32 x, s32 y) {
    GxBp(0xFE000060);
    GxBp((u32)*(u8*)(src + 0x12) << 5 | 0x80000000);
    if ((u32)lbl_8040ADF0[0] != *(u32*)(src + 0x18) || lbl_8040AE10[0] != 0) {
        u32 maskAD0 = lbl_80348AD0[0];
        lbl_805FB4B0 += 1;
        lbl_8040ADF0[0] = *(s32*)(src + 0x18);
        lbl_8040AE10[0] = 0;
        u32 fmt = *(u8*)(src + 0x13);
        GxBp((((u32)*(u8*)(src + 0xF) - 1) & 0xF) << 12 | 0x84000000);
        u32 tbl = *(u32*)(lbl_80348950 + fmt * 0x20);
        s32 tbl2 = *(s32*)(lbl_80348A10 + fmt * 0x20);
        GxBp(((u32)*(u8*)(src + 0x10) << 20) | 0x88000000 | (((u32)U16(src, 6) - 1) << 10) |
             ((u32)U16(src, 4) - 1));
        GxBp((tbl >> 5) | 0x8C0D0000 | 0x8000);
        if (tbl2 >= 0) {
            GxBp(((u32)tbl2 >> 5) | 0x900D0000 | 0x8000);
        } else {
            GxBp(0x90000000);
        }
        GxBp(((*(u32*)(src + 0x18) >> 5) & 0xFFFFFF) | 0x94000000);
        if (*(u8*)(src + 0xD) != 0) {
            u32 m = (maskAD0 >> 9) & 0x3FF;
            u32 w98 = m | 0x98000000;
            w98 = (w98 & ~0x3FC00u) | (((u32)*(u8*)(src + 0x11) << 10) & 0x3FC00);
            GxBp(w98);
            GxBp(0xFEFFFF00);
            GxBp(0x0F000000);
            GxBp(((*(u32*)(src + 0x1C) >> 5) & 0xFFFFFF) | 0x64000000);
            u32 w65 = m | 0x65000000;
            w65 = (w65 & ~0x1FC00u) | ((((s32)*(u8*)(src + 0xD) >> 1) << 10) & 0x1FC00);
            GxBp(w65);
            GxBp(0xFEFFFF00);
            GxBp(0x0F000000);
        }
    }
    GxBp(((u32)U16(src, 4) - 1) | 0x30000000);
    GxBp(((u32)U16(src, 6) - 1) | 0x31000000);
    s32 vw = VideoGetWidth(&gVideo);
    f32 sy = (2.0f * (f32)y) / (f32)VideoGetHeight(&gVideo);
    f32 u1 = (f32)U16(src, 0) / (f32)U16(src, 4);
    f32 v1 = (f32)U16(src, 2) / (f32)U16(src, 6);
    if (lbl_805FB4CC < 0x60) fn_8014E3EC(0x60);
    lbl_805FB4CC -= 0x60;
    u8* q = lbl_805FB4D0 + lbl_805FB4C4;
    f32 sx = -1.0f + ((2.0f * (f32)x) / (f32)vw);
    f32 ny = 1.0f - sy;
    lbl_805FB4C4 += 0x60;
    // Vertex 0: top-left.
    *(f32*)(q + 0x00) = 0.0f;
    *(f32*)(q + 0x04) = 0.0f;
    *(u32*)(q + 0x08) = 0xFFFFFFFF;
    *(f32*)(q + 0x0C) = -1.0f;
    *(f32*)(q + 0x10) = 1.0f;
    *(f32*)(q + 0x14) = -1.0f;
    // Vertex 1: top-right.
    *(f32*)(q + 0x18) = u1;
    *(f32*)(q + 0x1C) = 0.0f;
    *(u32*)(q + 0x20) = 0xFFFFFFFF;
    *(f32*)(q + 0x24) = sx;
    *(f32*)(q + 0x28) = 1.0f;
    *(f32*)(q + 0x2C) = -1.0f;
    // Vertex 2: bottom-left.
    *(f32*)(q + 0x30) = 0.0f;
    *(f32*)(q + 0x34) = v1;
    *(u32*)(q + 0x38) = 0xFFFFFFFF;
    *(f32*)(q + 0x3C) = -1.0f;
    *(f32*)(q + 0x40) = ny;
    *(f32*)(q + 0x44) = -1.0f;
    // Vertex 3: bottom-right.
    *(f32*)(q + 0x48) = u1;
    *(f32*)(q + 0x4C) = v1;
    *(u32*)(q + 0x50) = 0xFFFFFFFF;
    *(f32*)(q + 0x54) = sx;
    *(f32*)(q + 0x58) = ny;
    *(f32*)(q + 0x5C) = -1.0f;
    fn_80151644(0x98, 4, q);
    fn_801578D8(dst, 0x10060, 0);
}

// 0x80158228
// Programs image sampler `idx` (0..7) from the image header p: size, format, address and, for
// images with a second plane (byte +0xD), the extra address / tmem registers. `arg` selects the
// table column (0..3); the programming is skipped when the sampler already holds the same image.
void fn_80158228(u8* p, s32 idx, s32 arg) {
    GxBp(0xFE000060);
    GxBp(((u32)(idx + 0x80) << 24) | (((u32)*(u8*)(p + 0x12) << 5) & 0x1FE0));
    s32 k = (idx & 3) | (idx >> 3);
    if ((u32)lbl_8040ADF0[k] != *(u32*)(p + 0x18) || arg != lbl_8040AE10[k]) {
        lbl_8040AE10[k] = arg;
        lbl_805FB4B0 += 1;
        lbl_8040ADF0[k] = *(s32*)(p + 0x18);
        GxBp((((u32)*(u8*)(p + 0xF) - 1) & 0xF) << 12 | ((u32)(idx + 0x84) << 24));
        GxBp(((u32)(idx + 0x88) << 24) | ((u32)*(u8*)(p + 0x10) << 20) |
             (((u32)U16(p, 6) - 1) << 10) | ((u32)U16(p, 4) - 1));
        s32 off = arg * 4 + ((s32)*(u8*)(p + 0x13) << 5);
        u32 tbl = *(u32*)(lbl_80348950 + off);
        s32 tbl2 = *(s32*)(lbl_80348A10 + off);
        u32 mask = lbl_80348AD0[arg];
        GxBp(((u32)(idx + 0x8C) << 24) | 0xD8000 | (tbl >> 5));
        if (tbl2 >= 0) {
            GxBp(((u32)tbl2 >> 5) | 0xD8000 | ((u32)(idx + 0x90) << 24));
        } else {
            GxBp((u32)(idx + 0x90) << 24);
        }
        u32 planes = *(u8*)(p + 0xD);
        GxBp(((u32)(idx + 0x94) << 24) | ((*(u32*)(p + 0x18) >> 5) & 0xFFFFFF));
        if (planes != 0) {
            GxBp(((u32)(idx + 0x98) << 24) | ((u32)*(u8*)(p + 0x11) << 10) | ((mask >> 9) & 0x3FF));
            GxBp(0xFEFFFF00);
            GxBp(0x0F000000);
            GxBp(((*(u32*)(p + 0x1C) >> 5) & 0xFFFFFF) | 0x64000000);
            GxBp(((mask >> 9) & 0x3FF) | 0x65000000 | ((((s32)planes >> 1) << 10) & 0x1FC00));
            GxBp(0xFEFFFF00);
            GxBp(0x0F000000);
        }
    }
}

// 0x80158530
// Builds a 32 x 18 grid of vertices in the vertex scratch buffer (*outVerts, 0x3600 bytes, two
// vertices of 0x18 bytes per step: u, v, color, x, y, z) from the position table `pos` (0x18 bytes
// per vertex pair) and the color table `col` (byte-reversed 4-byte colors), with texture
// coordinates scaled by (sx, sy), plus the matching u16 index buffer (*outIdx, 0x80 bytes).
void fn_80158530(u8* pos, u8* col, u8** outVerts, u8** outIdx, f32 sx, f32 sy) {
    if (lbl_805FB4CC < 0x3600) fn_8014E3EC(0x3600);
    s32 base = lbl_805FB4C4;
    lbl_805FB4C4 = base + 0x3600;
    u8* v = lbl_805FB4D0 + base;
    lbl_805FB4CC -= 0x3600;
    *outVerts = v;
    for (s32 row = 0; row < 0x12; row++) {
        const u8* ps = pos;
        const u8* cs = col;
        s32 c = 0;
        f32 ty = ((f32)row / 17.0f) * sy;
        for (s32 n = 0; n < 0x10; n++) {
            s32 c0 = c;
            s32 c1 = c + 1;
            c = c1 + 1;
            *(f32*)(v + 0x00) = ((f32)c0 / 31.0f) * sx;
            *(f32*)(v + 0x04) = ty;
            *(f32*)(v + 0x0C) = *(const f32*)(ps + 0x00);
            *(f32*)(v + 0x10) = *(const f32*)(ps + 0x04);
            *(f32*)(v + 0x14) = -*(const f32*)(ps + 0x08);
            v[0x08] = cs[3];
            v[0x09] = cs[2];
            v[0x0A] = cs[1];
            v[0x0B] = cs[0];
            *(f32*)(v + 0x18) = ((f32)c1 / 31.0f) * sx;
            *(f32*)(v + 0x1C) = ty;
            *(f32*)(v + 0x24) = *(const f32*)(ps + 0x0C);
            *(f32*)(v + 0x28) = *(const f32*)(ps + 0x10);
            *(f32*)(v + 0x2C) = -*(const f32*)(ps + 0x14);
            v[0x20] = cs[7];
            v[0x21] = cs[6];
            v[0x22] = cs[5];
            v[0x23] = cs[4];
            ps += 0x18;
            cs += 8;
            v += 0x30;
        }
        col += 0x80;
        pos += 0x180;
    }
    if (lbl_805FB4CC < 0x80) fn_8014E3EC(0x80);
    s32 base2 = lbl_805FB4C4;
    lbl_805FB4C4 = base2 + 0x80;
    lbl_805FB4CC -= 0x80;
    *outIdx = lbl_805FB4D0 + base2;
    for (s32 j = 0; j < 0x20; j++) {
        *(u16*)(*outIdx + j * 4) = (u16)j;
        *(u16*)(*outIdx + j * 4 + 2) = (u16)(j + 0x20);
    }
}

// 0x80158D58
// Sends one packed BP register word built from twelve fields (a3 is unused). The bit layout
// depends on bit 1 of a9.
void fn_80158D58(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u32 a11, u32 a12, u32 a13, u32 a14, u32 a15) {
    (void)a3;
    u32 v;
    if (a9 & 2) {
        v = a14 | ((a15 * 4) | ((a8 * 0x10) | (a7 << 7))) |
            ((a6 << 10) | ((a5 << 13) | (((a12 << 19) & ~0x40000u) | ((a9 << 18) & 0x40000) | 0x30000))) |
            (((a9 << 18) & 0xFFF00000u) | (a13 << 22) | (((a4 * 2) + 0xC1) << 24));
    } else {
        v = a14 | ((a15 * 4) | ((a8 * 0x10) | ((a7 << 7) | (a6 << 10)))) |
            ((a5 << 13) | ((a10 << 16) | ((a9 << 18) | (a12 << 19))) |
             ((a11 << 20) | (a13 << 22) | (((a4 * 2) + 0xC1) << 24)));
    }
    GxBp(v);
}

// 0x801590A0
// One-shot: when flag bit 1 of lbl_805FB50C is clear, sets it and sends the vertex descriptor
// setup (variant chosen by bit 0).
void fn_801590A0(void) {
    if (!(lbl_805FB50C & 2)) {
        u8 flags = lbl_805FB50C | 2;
        lbl_805FB50C = flags;
        if (flags & 1) {
            GxBp(0xFE00000F);
            GxBp(1);
            GxXf(0x103F, 1);
            GxBp(0xFE003C00);
            GxBp(0x400);
            GxXf(0x1010, 0x441);
            GxBp(0x283C0000);
            GxBp(0xC008F2A4);
            GxBp(0xC108E6F0);
            GxBp(0xC208F80F);
            GxBp(0xC308F070);
        } else {
            GxBp(0xFE00000F);
            GxBp(1);
            GxXf(0x103F, 1);
            GxBp(0xFE003C00);
            GxBp(0);
            GxXf(0x100E, 0x442);
            GxXf(0x1010, 0x442);
            GxBp(0x28380040);
            GxBp(0xC008F8AF);
            GxBp(0xC108F2F0);
        }
    }
}

// 0x8015A5A4
// Fills in the derived fields of an image header p: power-of-two sizes (+4/+6), mip level count
// (+0xF), data size (+0x14), then allocates the data block (+0x18) and optional second plane
// (+0x1C) and sets the sampler format bytes (+0x10..+0x13). `a` is the allocation tag.
void fn_8015A5A4(u8* p, s32 a) {
    if (p[8] & 4) {
        // Mip-mapped: round width/height up to powers of two, level count from the smaller.
        s32 t = U16(p, 0) - 1;
        s32 lw = 0;
        while (t > 0) { t >>= 1; lw++; }
        t = U16(p, 2) - 1;
        s32 lh = 0;
        while (t > 0) { t >>= 1; lh++; }
        *(u16*)(p + 4) = (u16)(1 << lw);
        *(u16*)(p + 6) = (u16)(1 << lh);
        if (lw < lh) lh = lw;
        p[0xF] = (u8)(lh + 1);
        if (p[9] == 8) {
            s32 v = (u8)p[0xF] - 2;
            p[0xF] = (u8)(v >= 1 ? v : 1);
        }
        *(u32*)(p + 0x14) = 0;
        for (s32 i = 0; i < p[0xF]; i++) {
            s32 sz = 0;
            fn_8014ACCC(4, p[9], U16(p, 4) >> i, U16(p, 6) >> i, &sz, NULL, NULL, NULL);
            *(u32*)(p + 0x14) += sz;
        }
    } else {
        p[0xF] = 1;
        s32 sz = 0, w = 0, h = 0;
        fn_8014ACCC(4, p[9], U16(p, 0), U16(p, 2), &sz, NULL, &w, &h);
        *(u32*)(p + 0x14) = sz;
        *(u16*)(p + 4) = (u16)w;
        *(u16*)(p + 6) = (u16)h;
    }
    if (p[8] & 8) {
        *(void**)(p + 0x18) = NULL;
        *(void**)(p + 0x1C) = NULL;
    } else {
        s32 spec[4];
        spec[0] = U16(p, 0);
        spec[1] = U16(p, 2);
        spec[2] = p[9];
        spec[3] = -1;
        s32 guard = 0;
        if (p[8] & 0x10) guard = 0x20;
        u8* buf = (u8*)(*(RendAllocFn*)gRendMem)(*(u32*)(p + 0x14) + guard, 0x20, 6,
                                                  (const char*)(intptr_t)a, spec);
        *(u8**)(p + 0x18) = buf;
        if (p[8] & 0x10) {
            // Guard pattern after the pixel data.
            for (s32 i = 0; i < guard; i++) {
                buf[*(u32*)(p + 0x14) + (u8)i] = (u8)(0xA5 - i);
            }
        }
        if (p[0xD] != 0) {
            s32 spec2[3];
            spec2[0] = p[0xD];
            spec2[1] = p[0xE];
            spec2[2] = -1;
            *(u8**)(p + 0x1C) = (u8*)(*(RendAllocFn*)gRendMem)((u32)(p[0xD] * p[0xE]), 0x20, 7,
                                                               (const char*)(intptr_t)a, spec2);
        } else {
            *(void**)(p + 0x1C) = NULL;
        }
    }
    if (p[0xD] != 0) {
        if (p[0xF] > 1) {
            p[0x13] = 5;
            p[0x12] = 1;
        } else {
            p[0x13] = 2;
            p[0x12] = 0;
        }
        p[0x10] = p[9] == 0xB ? 8 : 9;
        switch (p[0xC]) {
        case 4: p[0x11] = 0; break;
        case 6: p[0x11] = 2; break;
        default: p[0x11] = 1; break;
        }
    } else {
        if (p[9] == 0x16) {
            p[0x13] = p[0xF] > 1 ? 4 : 1;
        } else {
            p[0x13] = p[0xF] > 1 ? 3 : 0;
        }
        p[0x12] = p[0xF] > 1 ? 2 : 0;
        switch (p[9]) {
        case 8:  p[0x10] = 0xE; break;
        case 15: p[0x10] = 0; break;
        case 16: p[0x10] = 2; break;
        case 17: p[0x10] = 1; break;
        case 18: p[0x10] = 3; break;
        case 19: p[0x10] = 4; break;
        case 23: p[0x10] = 5; break;
        default: p[0x10] = 6; break;
        }
        p[0x11] = 0;
    }
}

// 0x8015BBC0
// Resets the GX state for a frame: viewport/scissor to the display size, display copy source and
// destination, copy filter, and (when clear != 0) the copy clear color and the three frame buffer
// clears. Also streams 27 zero words to the FIFO (matrix reset).
void fn_8015BBC0(s32 clear) {
    fn_8015D660();
    GXClearVtxDesc();
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xA, 0, 4, 0);
    GXSetVtxAttrFmt(0, 0xB, 1, 5, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    f32 zero = 0.0f;
    GXSetViewport(zero, zero, (f32)U16(lbl_8040AE48, 4), (f32)U16(lbl_8040AE48, 6), zero, 1.0f);
    GXSetScissor(0, 0, U16(lbl_8040AE48, 4), U16(lbl_8040AE48, 6));
    u16 yScale = (u16)fn_8024BC90(1.0f);
    fn_8024BBE4(0, 0, U16(lbl_8040AE48, 4), U16(lbl_8040AE48, 6));
    fn_8024BC24(U16(lbl_8040AE48, 4), yScale);
    fn_8024BDD4(lbl_8040AE48[0x19], lbl_8040AE48 + 0x1A, 0, lbl_805F9254);
    fn_8024BFB8(0);
    s32 a;
    switch (lbl_805FB500) {
    case 1: a = 1; break;
    case 2: a = 2; break;
    case 3: a = 3; break;
    case 4: a = 4; break;
    case 5: a = 7; break;
    default: a = 0; break;
    }
    s32 b;
    switch ((u32)lbl_805FB508) {
    case 8:  b = 1; break;
    case 16: b = 2; break;
    case 24: b = 3; break;
    default: b = 0; break;
    }
    GXSetPixelFmt(a, b);
    GXSetDither((u8)lbl_805FB504);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xA, 1);
    GXSetVtxDesc(0xB, 1);
    GXSetVtxDesc(0xD, 1);
    GXInvalidateVtxCache();
    fn_8024CB00();
    GXBegin(0x90, 0, 3);
    for (s32 i = 0; i < 6; i++) GXWGFifo._f32 = 0.0f;
    GXWGFifo._u32 = 0;
    for (s32 i = 0; i < 8; i++) GXWGFifo._f32 = 0.0f;
    GXWGFifo._u32 = 0;
    for (s32 i = 0; i < 8; i++) GXWGFifo._f32 = 0.0f;
    GXWGFifo._u32 = 0;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f;
    if (clear != 0) {
        u8 color[4] = {0, 0, 0, 0};
        fn_8024BD5C(color, 0xFFFFFF);
        GXSetZMode(1, 3, 1);
        GXSetColorUpdate(1);
        fn_8024BFCC(lbl_805FB538[0], 1);
        fn_8024BFCC(lbl_805FB538[0], 1);
        fn_8024BFCC(lbl_805FB538[1], 1);
    }
}

// 0x8015EB3C
// Initialises a 0xA0-byte light/fog parameter block: two enable flags, tuning floats
// (0.86, 0.76, 2.0, 0.01) and rows of four floats (axis-aligned unit rows).
void fn_8015EB3C(u8* p) {
    *(u8*)(p + 0x0) = 1;
    *(u8*)(p + 0x1) = 1;
    *(f32*)(p + 0x04) = 1.0f;
    *(f32*)(p + 0x08) = 0.0f;
    *(f32*)(p + 0x0C) = 0.86f;
    *(f32*)(p + 0x10) = 0.76f;
    *(f32*)(p + 0x14) = 2.0f;
    *(f32*)(p + 0x18) = 0.01f;
    *(f32*)(p + 0x1C) = 1.0f;
    *(f32*)(p + 0x20) = 1.0f;
    *(f32*)(p + 0x24) = 1.0f;
    *(f32*)(p + 0x28) = 0.0f;
    *(f32*)(p + 0x2C) = 1.0f;
    *(f32*)(p + 0x30) = 0.0f;
    *(f32*)(p + 0x34) = 0.0f;
    *(f32*)(p + 0x38) = 0.0f;
    *(f32*)(p + 0x3C) = 0.0f;
    *(f32*)(p + 0x40) = 1.0f;
    *(f32*)(p + 0x44) = 1.0f;
    *(f32*)(p + 0x48) = 1.0f;
    *(f32*)(p + 0x4C) = 0.0f;
    *(f32*)(p + 0x50) = 1.0f;
    *(f32*)(p + 0x54) = 1.0f;
    *(f32*)(p + 0x58) = 1.0f;
    *(f32*)(p + 0x5C) = 0.0f;
    *(f32*)(p + 0x60) = 1.0f;
    *(f32*)(p + 0x64) = 0.0f;
    *(f32*)(p + 0x68) = 0.0f;
    *(f32*)(p + 0x6C) = 0.0f;
    *(f32*)(p + 0x70) = 0.0f;
    *(f32*)(p + 0x74) = 1.0f;
    *(f32*)(p + 0x78) = 0.0f;
    *(f32*)(p + 0x7C) = 0.0f;
    *(f32*)(p + 0x80) = 0.0f;
    *(f32*)(p + 0x84) = 0.0f;
    *(f32*)(p + 0x88) = 1.0f;
    *(f32*)(p + 0x8C) = 0.0f;
    *(f32*)(p + 0x90) = 0.0f;
    *(f32*)(p + 0x94) = 0.0f;
    *(f32*)(p + 0x98) = 0.0f;
    *(f32*)(p + 0x9C) = 0.0f;
}

}  // extern "C"
