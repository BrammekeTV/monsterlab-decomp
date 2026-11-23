// Large functions from 0x80205894..0x8022C7F0: textured-quad blit (movie frame), enemy-side
// target choice for the AI, and the story-flag / money milestone check.
#include "types.h"
#include "revolution.h"

extern "C" {
void NoticePosted(u8* p, s32 idx);                                           // 0x8022DF3C
s32  RandomRoll(void);                                                       // 0x80220800 (raw 32-bit random, used unsigned)
void TargetListInit(void* t);                                                // 0x802161DC
void TargetListDecode(void* t, const u32* mask, s32 side);                   // 0x80216240
void* TargetListDestroy(void* t, s32 del);                                   // 0x80216200
s32  fn_8020DD90(u8* obj, s32 a);                                            // 0x8020DD90
s32  fn_8020DEA0(u8* obj, s32 slot);                                         // 0x8020DEA0
BOOL fn_80212E28(u8** slot);                                                 // 0x80212E28
void fn_8020574C(void);                                                      // 0x8020574C
void GXSetVtxDesc(s32 a, s32 b);                                              // 0x8024A3A4
void GXClearVtxDesc(void);                                                      // 0x8024A7E4
void GXSetVtxAttrFmt(s32 a, s32 b, s32 c, s32 d, s32 e);                         // 0x8024A818
void GXSetTexCoordGen2(s32 coord, s32 func, s32 src, s32 mtx, s32 normalize, s32 postMtx);  // 0x8024AC4C
void GXSetNumTexGens(s32 a);                                                     // 0x8024AE74
void GXSetDrawSync(u32 token);                                                 // 0x8024B298 (GXSetDrawSync)
void GXBegin(s32 a, s32 b, s32 c);                                       // 0x8024B974
void GXSetNumChans(s32 a);                                                     // 0x8024C2F0
void GXLoadTexObj(u8* obj, s32 id);                                           // 0x8024C91C
void GXSetTevIndirect(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h, s32 i, s32 j);  // 0x8024D0BC (two stack args)
void GXSetIndTexMtx(s32 a, const f32* p, s32 c);                                // 0x8024D124
void GXSetIndTexOrder(s32 a, s32 b, s32 c);                                       // 0x8024D37C
void GXSetNumIndStages(s32 count);                                                 // 0x8024D448
void fn_8024D59C(s32 a, s32 b, s32 c, s32 d, s32 e);                         // 0x8024D59C
void fn_8024D5DC(s32 a, s32 b, s32 c, s32 d, s32 e);                         // 0x8024D5DC
void fn_8024D61C(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);                 // 0x8024D61C
void fn_8024D674(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);                 // 0x8024D674
void fn_8024D72C(s32 reg, const void* color);                                // 0x8024D72C
void GXSetTevKColor(s32 reg, const void* color);                                // 0x8024D790
void GXSetTevKColorSel(s32 stage, s32 sel);                                        // 0x8024D7EC
void GXSetTevKAlphaSel(s32 stage, s32 sel);                                        // 0x8024D83C
void GXSetTevSwapMode(s32 stage, s32 rasSel, s32 texSel);                         // 0x8024D88C
void GXSetTevSwapModeTable(s32 table, s32 r, s32 g, s32 b, s32 a);                     // 0x8024D8C8
void GXSetTevOrder(s32 a, s32 b, s32 c, s32 d);                                // 0x8024DA08
void GXSetNumTevStages(s32 a);                                                     // 0x8024DB64
void GXSetBlendMode(s32 a, s32 b, s32 c, s32 d);                                // 0x8024DED8
void fn_8024E348(f32* mtx, s32 id, s32 type);                                // 0x8024E348

extern u8*  gBattle;                // 0x805FBBB8
extern u8*  gGameDb;                // 0x805FBBC0
extern u8*  lbl_805FBBE0;           // 0x805FBBE0 (the save)
extern f32  lbl_80349020[];         // 0x80349020
extern f32  lbl_80349040[];         // 0x80349040
extern s32  lbl_80349060[];         // 0x80349060 (slot ids 0..4)
extern s32  lbl_80349D90[];         // 0x80349D90
extern s32  lbl_80349DA0[];         // 0x80349DA0
extern f32  lbl_80527100[];         // 0x80527100 (texture matrix, 3x4)
extern f32  lbl_80527120[];         // 0x80527120 (texture matrix, 3x4)
extern u16  lbl_80600438;           // 0x80600438
extern u16  lbl_8060043A;           // 0x8060043A
extern u16  lbl_8060043C;           // 0x8060043C
extern u16  lbl_8060043E;           // 0x8060043E
extern u8   lbl_80600440;           // 0x80600440
extern u8   lbl_80600441;           // 0x80600441
extern u8   lbl_80600442;           // 0x80600442
extern u8   lbl_80600443;           // 0x80600443
extern u8   lbl_80600444;           // 0x80600444
extern u8   lbl_80600445;           // 0x80600445
extern u8   lbl_80600446;           // 0x80600446
extern u8   lbl_80600447;           // 0x80600447
extern u8   lbl_80600448;           // 0x80600448
extern u8   lbl_80600449;           // 0x80600449
extern u8   lbl_8060044A;           // 0x8060044A
extern u8   lbl_8060044B;           // 0x8060044B
}

// gGameDb->data: the loaded game database (pointer at +0xC).
static inline u8* GameDbData(void) { return *(u8**)(gGameDb + 0xC); }

// The 256-bit flag set at obj+0x20 is four big-endian 64-bit words, highest bit group first:
// ids 0..63 at +0x38, 64..127 at +0x30, 128..191 at +0x28, 192..255 at +0x20.
static inline u32 FlagWordOffset(u32 id) {
    if (id < 0x40) return 0x38;
    if (id < 0x80) return 0x30;
    if (id < 0xC0) return 0x28;
    return 0x20;
}

static inline bool FlagTest(u8* obj, u32 id) {
    u32 off = FlagWordOffset(id);
    u64 v = ((u64)*(u32*)(obj + off) << 32) | *(u32*)(obj + off + 4);
    return ((v >> (id & 0x3F)) & 1) != 0;
}

static inline void FlagSet(u8* obj, u32 id) {
    u32 off = FlagWordOffset(id);
    u64 bit = (u64)1 << (id & 0x3F);
    *(u32*)(obj + off)     |= (u32)(bit >> 32);
    *(u32*)(obj + off + 4) |= (u32)bit;
}

// float -> u32 conversion with saturation (__cvt_fp2unsigned).
static inline u32 FloatToU32(f32 v) {
    if (!(v > 0.0f)) return 0;
    if (v >= 4294967296.0f) return 0xFFFFFFFFu;
    return (u32)v;
}

// Appends a notice record (kind, value) to the queue at +0x744 (0x1C bytes each, count at +0x8E8,
// serial at +0x910) and posts it, if the queue (15 entries) has room.
static inline void PostNotice(u8* obj, s32 kind, s32 value) {
    s32 n = *(s32*)(obj + 0x8E8);
    if (n < 0xF) {
        u8* e = obj + n * 0x1C;
        *(s32*)(e + 0x744) = kind;
        *(s32*)(e + 0x748) = value;
        *(s32*)(e + 0x74C) = 0;
        *(s32*)(e + 0x750) = 0;
        *(s32*)(e + 0x754) = 0;
        *(s32*)(e + 0x758) = -1;
        s32 serial = *(s32*)(obj + 0x910) + 1;
        *(s32*)(obj + 0x910) = serial;
        *(s32*)(e + 0x75C) = serial;
        *(s32*)(obj + 0x8E8) = *(s32*)(obj + 0x8E8) + 1;
        NoticePosted(obj, n);
    }
}

extern "C" {

// 0x80205894
// Draws one screen-space textured quad (movie frame) with the TEV set-up for the video planes:
// 2D position (x, y), scale (sx, sy) applied to the texture size (w, h), constant alpha `a`
// (>= 0.99 opaque, <= 0.01 transparent). When obj+0x140 is set a fourth plane is blended in.
void fn_80205894(u8* obj, f32 x, f32 y, f32 sx, f32 sy, f32 a, s32 w, s32 h, s32 flag) {
    u8 alpha;
    s32 hasExtra;
    u16 c10[4];
    u8 c8[4];
    u8 c4[4];
    u8 c0[4];

    fn_8020574C();
    if (a >= 0.99f) {
        alpha = 0xFF;
    } else if (a <= 0.01f) {
        alpha = 0;
    } else {
        alpha = (u8)(s32)(0.5f + 255.0f * a);
    }
    hasExtra = (*(s32*)(obj + 0x140) != 0) ? 1 : 0;

    *(s32*)(obj + *(s32*)(obj + 0x114) * 4 + 0x1D0) = 1;
    GXSetDrawSync((u16)(*(s32*)(obj + 0x114) + 0x4444));
    GXSetVtxAttrFmt(0, 9, 1, 3, 0);
    GXSetVtxAttrFmt(0, 0xB, 1, 5, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xE, 1, 4, 0);
    GXClearVtxDesc();
    GXLoadTexObj(obj + (*(s32*)(obj + 0x114) << 7), 0);
    GXLoadTexObj(obj + 0x190, 1);
    GXLoadTexObj(obj + 0x1B0, 2);
    GXLoadTexObj(obj + (*(s32*)(obj + 0x114) << 7) + 0x20, 3);
    GXLoadTexObj(obj + (*(s32*)(obj + 0x114) << 7) + 0x40, 4);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xB, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxDesc(0xE, 1);
    if (hasExtra != 0) {
        GXLoadTexObj(obj + (*(s32*)(obj + 0x114) << 7) + 0x60, 5);
    }
    GXSetNumChans(1);
    GXSetNumTexGens(4);
    GXSetNumTevStages(4);
    GXSetNumIndStages(3);
    GXSetIndTexOrder(0, 1, 1);
    GXSetIndTexOrder(1, 2, 2);
    if (hasExtra != 0) {
        GXSetBlendMode(1, 4, 5, 0);
    } else if (alpha != 0xFF) {
        GXSetBlendMode(1, 4, 5, 0);
    } else {
        GXSetBlendMode(1, 1, 0, 0);
    }

    // TEV stage 0
    GXSetTevIndirect(0, 1, 0, 7, 1, 0, 0, 0, 0, 0);
    GXSetTevOrder(0, 0, 4, 0xFF);
    fn_8024D59C(0, 0xF, 8, 0xE, 2);
    fn_8024D61C(0, 0, 0, 0, 0, 0);
    fn_8024D5DC(0, 7, 4, 6, 1);
    fn_8024D674(0, 1, 0, 0, 0, 0);
    GXSetTevKColorSel(0, 0xC);
    GXSetTevKAlphaSel(0, 0x1C);
    GXSetTevSwapMode(0, 0, 0);
    // TEV stage 1
    GXSetTevIndirect(1, 1, 0, 7, 1, 0, 0, 0, 0, 0);
    GXSetTevOrder(1, 0, 3, 0xFF);
    fn_8024D59C(1, 0xF, 8, 0xE, 0);
    fn_8024D61C(1, 0, 0, 1, 0, 0);
    fn_8024D5DC(1, 7, 4, 6, 0);
    fn_8024D674(1, 1, 0, 0, 0, 0);
    GXSetTevKColorSel(1, 0xD);
    GXSetTevKAlphaSel(1, 0x1D);
    GXSetTevSwapMode(1, 0, 0);
    // TEV stage 2
    GXSetTevIndirect(2, 0, 0, 7, 1, 0, 0, 0, 0, 0);
    GXSetTevOrder(2, 3, 0, 0xFF);
    fn_8024D59C(2, 0xF, 8, 0xC, 0);
    fn_8024D61C(2, 0, 0, 0, 1, 0);
    fn_8024D5DC(2, 4, 7, 7, 0);
    fn_8024D674(2, 0, 0, 0, 1, 0);
    GXSetTevSwapMode(2, 0, 0);
    // TEV stage 3
    if (hasExtra != 0) {
        GXSetTevIndirect(3, 0, 0, 7, 1, 0, 0, 0, 0, 0);
        GXSetTevOrder(3, 3, 5, 4);
        fn_8024D59C(3, 1, 0, 0xE, 0xF);
        fn_8024D61C(3, 0, 0, 0, 1, 0);
        fn_8024D5DC(3, 7, 4, 5, 7);
        fn_8024D674(3, 0, 0, 0, 1, 0);
        GXSetTevSwapMode(3, 0, 0);
        GXSetTevKColorSel(3, 0xE);
    } else {
        GXSetTevOrder(3, 0xFF, 0xFF, 4);
        fn_8024D59C(3, 1, 0, 0xE, 0xF);
        fn_8024D61C(3, 0, 0, 0, 1, 0);
        fn_8024D5DC(3, 7, 6, 5, 7);
        fn_8024D674(3, 0, 0, 0, 1, 0);
        GXSetTevSwapMode(3, 0, 0);
        GXSetTevKColorSel(3, 0xE);
    }

    // TEV colour registers from constant data.
    c10[0] = lbl_80600438;
    c10[1] = lbl_8060043A;
    c10[2] = lbl_8060043C;
    c10[3] = lbl_8060043E;
    fn_8024D72C(1, c10);
    c8[0] = lbl_80600440;
    c8[1] = lbl_80600441;
    c8[2] = lbl_80600442;
    c8[3] = lbl_80600443;
    GXSetTevKColor(0, c8);
    c4[0] = lbl_80600444;
    c4[1] = lbl_80600445;
    c4[2] = lbl_80600446;
    c4[3] = lbl_80600447;
    GXSetTevKColor(1, c4);
    c0[0] = lbl_80600448;
    c0[1] = lbl_80600449;
    c0[2] = lbl_8060044A;
    c0[3] = lbl_8060044B;
    GXSetTevKColor(2, c0);
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetIndTexMtx(1, lbl_80349020, 4);

    // Texture matrices: scale = crop size / texture size for the two chroma planes.
    lbl_80527100[0] = (f32)(*(u32*)(obj + 0x104) >> 3) / (f32)*(u32*)(obj + 0x180);
    lbl_80527100[5] = (f32)*(u32*)(obj + 0x108) / (f32)*(u32*)(obj + 0x184);
    lbl_80527120[0] = (f32)(*(u32*)(obj + 0x10C) >> 3) / (f32)*(u32*)(obj + 0x188);
    lbl_80527120[5] = (f32)*(u32*)(obj + 0x110) / (f32)*(u32*)(obj + 0x18C);
    GXSetTexCoordGen2(0, 1, 4, 0x1E, 0, 0x7D);
    GXSetTexCoordGen2(1, 1, 4, 0x21, 0, 0x7D);
    GXSetTexCoordGen2(2, 1, 4, 0x24, 0, 0x7D);
    GXSetTexCoordGen2(3, 1, 5, 0x27, 0, 0x7D);
    fn_8024E348(lbl_80349040, 0x1E, 1);
    fn_8024E348(lbl_80527100, 0x21, 1);
    fn_8024E348(lbl_80527120, 0x24, 1);
    fn_8024E348(lbl_80349040, 0x27, 1);

    // Quad: 4 vertices, each s16 x, y, z + 4 colour bytes + 4 floats (two texcoord pairs).
    GXBegin(0x80, 0, 4);
    s32 x0 = (s32)x;
    s32 y0 = (s32)y;
    s32 x1 = x0 + (s32)((f32)(u32)w * sx);
    s32 y1 = y0 + (s32)((f32)(u32)h * sy);

    GXWGFifo._u16 = (u16)x0;
    GXWGFifo._u16 = (u16)y0;
    GXWGFifo._u16 = 0;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f;

    GXWGFifo._u16 = (u16)x1;
    GXWGFifo._u16 = (u16)y0;
    GXWGFifo._u16 = 0;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 0.0f;

    GXWGFifo._u16 = (u16)x1;
    GXWGFifo._u16 = (u16)y1;
    GXWGFifo._u16 = 0;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 1.0f;

    GXWGFifo._u16 = (u16)x0;
    GXWGFifo._u16 = (u16)y1;
    GXWGFifo._u16 = 0;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._u8 = alpha;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 1.0f;

    GXSetDrawSync(0);
}

// 0x8020D0E8
// Builds the AI's list of candidate action ids for this fighter (0x15 when the battle value at
// gBattle+0xA10 exceeds the tuning threshold, 0x14 when one of five per-slot values exceeds the
// integer threshold, 0x13/0x12 for wounded opponent slots) and adds each action record's s16
// modifiers (record +0xC/+0xE/+0x10/+0x12/+0x14) into the five float fields at +0xC..+0x1C.
s32 fn_8020D0E8(u8* obj) {
    s32 list[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    s32 count = 0;
    u8* tune = *(u8**)(GameDbData() + 0x99D0);
    f32 hpThreshold = *(f32*)(tune + 0x1D4);

    if ((f32)*(s32*)(gBattle + 0xA10) > hpThreshold) {
        u8* rec = *(u8**)(GameDbData() + 0x18F8) + *(s32*)(obj + 0x8) * 0x1C;
        count = 1;
        list[0] = 0x15;
        *(f32*)(obj + 0x10) = *(f32*)(obj + 0x10) + (f32)*(s16*)(rec + 0xE);
        rec = *(u8**)(GameDbData() + 0x18F8) + *(s32*)(obj + 0x8) * 0x1C;
        *(f32*)(obj + 0x14) = *(f32*)(obj + 0x14) + (f32)*(s16*)(rec + 0x10);
    } else {
        *(f32*)(obj + 0x10) = 0.0f;
        *(f32*)(obj + 0x14) = 0.0f;
    }

    {
        u8* rec = *(u8**)(GameDbData() + 0x18F8) + *(s32*)(obj + 0x8) * 0x1C;
        *(f32*)(obj + 0xC) = *(f32*)(obj + 0xC) + (f32)*(s16*)(rec + 0xC);
        rec = *(u8**)(GameDbData() + 0x18F8) + *(s32*)(obj + 0x8) * 0x1C;
        *(f32*)(obj + 0x18) = *(f32*)(obj + 0x18) + (f32)*(s16*)(rec + 0x12);
        rec = *(u8**)(GameDbData() + 0x18F8) + *(s32*)(obj + 0x8) * 0x1C;
        *(f32*)(obj + 0x1C) = *(f32*)(obj + 0x1C) + (f32)*(s16*)(rec + 0x14);
    }

    // Five (u32, s32) pairs at +0x104..: the first whose second word exceeds the threshold counts.
    {
        s32 thr = (s32)*(f32*)(*(u8**)(GameDbData() + 0x99D0) + 0x1D4);
        u8* p = obj;
        for (s32 i = 0; i < 5; i++) {
            if (*(s32*)(p + 0x10C) > thr) {
                list[count++] = 0x14;
                break;
            }
            p += 8;
        }
    }

    // Opponent slots (the other side): a living fighter whose hp fraction is below the first
    // threshold adds 0x13 once (or 0x12 for slot 4).
    {
        s32 slotIdx = count;
        s32 flag = 0;
        for (s32 i = 0; i < 5; i++) {
            s32 side = *(s32*)(*(u8**)obj + 0x44);
            u8** enemies = *(u8***)(gBattle + (1 - side) * 4 + 0xA00);
            u8* f = enemies[i];
            if (f != NULL && fn_80212E28((u8**)(f + 0x28)) == 0) {
                s32* hp = *(s32**)(f + 0x28);
                u8* t = *(u8**)(GameDbData() + 0x99D0);
                f32 ratio = (f32)hp[0] / (f32)hp[1];
                f32 lowThr = *(f32*)(t + 0xE4);
                s32 band;
                if (ratio < lowThr) {
                    band = 0;
                } else {
                    f32 midThr = *(f32*)(t + 0xDC);
                    band = (ratio < midThr) ? 1 : 2;
                }
                if (band == 0) {
                    if (i == 4) {
                        list[slotIdx++] = 0x12;
                        count++;
                    } else if (flag == 0) {
                        list[slotIdx++] = 0x13;
                        flag = 1;
                        count++;
                    }
                }
            }
        }
    }

    for (s32 k = 0; k < count; k++) {
        u8* rec = *(u8**)(GameDbData() + 0x18F8) + list[k] * 0x1C;
        *(f32*)(obj + 0x10) = *(f32*)(obj + 0x10) + (f32)*(s16*)(rec + 0xE);
        rec = *(u8**)(GameDbData() + 0x18F8) + list[k] * 0x1C;
        *(f32*)(obj + 0x14) = *(f32*)(obj + 0x14) + (f32)*(s16*)(rec + 0x10);
        rec = *(u8**)(GameDbData() + 0x18F8) + list[k] * 0x1C;
        *(f32*)(obj + 0xC) = *(f32*)(obj + 0xC) + (f32)*(s16*)(rec + 0xC);
        rec = *(u8**)(GameDbData() + 0x18F8) + list[k] * 0x1C;
        *(f32*)(obj + 0x18) = *(f32*)(obj + 0x18) + (f32)*(s16*)(rec + 0x12);
        rec = *(u8**)(GameDbData() + 0x18F8) + list[k] * 0x1C;
        *(f32*)(obj + 0x1C) = *(f32*)(obj + 0x1C) + (f32)*(s16*)(rec + 0x14);
    }
    return fn_8020DD90(obj, 5);
}

// 0x8020D788
// Picks which of the five opponent slots the AI targets for action mode `mode`:
// 0 = slot where the planned attacks (entries at obj+0x34, 8 bytes each, count at +0x84) do the
//     most damage relative to its hp, 1/2 = slot with the largest value at +0x10C+8*i /
//     +0x110+8*i, 3 = slot 4, 4 and others = none. If nothing is chosen a random living slot is
//     taken. Returns the slot, or -1 if none is left; fn_8020DEA0 may reject a slot, then the
//     next one is tried.
s32 fn_8020D788(u8* obj, s32 mode) {
    s32 cand[5];
    s32 score[5];
    u8* tgtOwner[5];
    s32 tgtAction[5];
    s32 left = 5;
    s32 pick;
    u8** enemies = *(u8***)(gBattle + (1 - *(s32*)(obj + 0x4)) * 4 + 0xA00);

    for (s32 i = 0; i < 5; i++) cand[i] = lbl_80349060[i];
    for (s32 i = 0; i < 5; i++) {
        u8* f = enemies[i];
        if (f == NULL || **(s32**)(f + 0x28) <= 0) {
            cand[i] = -1;
            left--;
        }
    }

    for (;;) {
        pick = -1;
        switch (mode) {
        case 0: {
            for (s32 i = 0; i < 5; i++) {
                tgtOwner[i] = NULL;
                tgtAction[i] = 0;
                score[i] = 0;
            }
            u8* entry = obj;
            for (s32 k = 0; k < *(s32*)(obj + 0x84); k++) {
                u8* action = *(u8**)(GameDbData() + 0x430) + *(s32*)(entry + 0x38) * 0x14;
                struct { s32 limbs[5]; s32 count; } t;
                TargetListInit(&t);
                u32 mask = *(u32*)(action + 8);
                TargetListDecode(&t, &mask, *(s32*)(*(u8**)(entry + 0x34) + 4));
                s32 dmg = 0;
                if (t.count > 1) {
                    f32 scale = *(f32*)(*(u8**)(GameDbData() + 0x99D0) + 0x8C);
                    dmg = (s32)((f32)*(s16*)(action + 0xE) * scale) / t.count;
                }
                for (s32 j = 0; j < t.count; j++) {
                    s32 limb = t.limbs[j];
                    if (dmg > 0 && limb == cand[limb] && score[limb] < dmg) {
                        tgtOwner[limb] = *(u8**)(entry + 0x34);
                        score[limb] = dmg;
                        tgtAction[limb] = *(s32*)(entry + 0x38);
                    }
                }
                TargetListDestroy(&t, -1);
                entry += 8;
            }
            f32 best = 0.0f;
            s32 bestSlot = 0;
            for (s32 i = 0; i < 5; i++) {
                if (i == cand[i] && i != 4) {
                    f32 v = 0.0f;
                    if (enemies[i] != NULL) {
                        v = (f32)score[i] / (f32)**(s32**)(enemies[i] + 0x28);
                    }
                    if (best < v) {
                        best = v;
                        bestSlot = i;
                    }
                }
            }
            if (best > 0.0f) {
                *(u8**)(obj + 0x34) = tgtOwner[bestSlot];
                *(s32*)(obj + 0x38) = tgtAction[bestSlot];
                *(s32*)(obj + 0x84) = 1;
                *(f32*)(obj + 0xC) = 1.0f;
                return bestSlot;
            }
            break;
        }
        case 1:
        case 2: {
            // mode 1 compares the values at +0x10C + 8*i, mode 2 those at +0x110 + 8*i.
            s32 base = (mode == 1) ? 0x10C : 0x110;
            s32 bestVal = -1;
            for (s32 i = 0; i < 5; i++) {
                if (cand[i] == i) {
                    s32 v = *(s32*)(obj + base + i * 8);
                    if (v > bestVal) {
                        pick = i;
                        bestVal = v;
                    }
                }
            }
            break;
        }
        case 3:
            if (cand[4] == 4) pick = 4;
            break;
        default:
            break;
        }

        if (pick == -1) {
            if (left < 1) return pick;
            pick = (s32)((u32)RandomRoll() % (u32)left);
            for (s32 i = 0; i <= pick; i++) {
                if (i != cand[i]) pick++;
            }
        }
        cand[pick] = -1;
        left--;
        if (fn_8020DEA0(obj, pick) != 0) return pick;
    }
}

// 0x8022C7F0
// Adds `delta` to the running total at +0x9FC, then for each of four milestone ids (table at
// lbl_80349DA0, thresholds indexed through lbl_80349D90 into the tuning table at +0x99D0, last
// first) whose bit is not yet set in the 256-bit flag set at +0x20: if the total reached the
// threshold, sets the bit, counts it (+0x120), posts a type-4 notice, pays the reward from the
// record table at +0x10A8 into the save's money (capped at 99999999) and advances the save's
// level (+0x128) through the table at +0x63C8 while the money exceeds each entry's threshold,
// posting a type-1 notice if it rose.
void fn_8022C7F0(u8* obj, s32 delta) {
    s32 idsA[4];
    s32 idsB[4];
    s32 total = *(s32*)(obj + 0x9FC) + delta;
    *(s32*)(obj + 0x9FC) = total;

    for (s32 i = 0; i < 4; i++) {
        idsB[i] = lbl_80349D90[i];
        idsA[i] = lbl_80349DA0[i];
    }

    for (s32 i = 3; i >= 0; i--) {
        s32 id = idsA[i];
        if (FlagTest(obj, id & 0xFF)) continue;

        u8* thrRec = *(u8**)(GameDbData() + 0x99D0) + idsB[i] * 8;
        s32 thr = (s32)*(f32*)(thrRec + 4);
        if (total < thr) continue;
        if (id == -1) continue;
        if (FlagTest(obj, id & 0xFF)) continue;

        *(s32*)(obj + 0x120) = *(s32*)(obj + 0x120) + 1;
        FlagSet(obj, id & 0xFF);
        PostNotice(obj, 4, id);

        u8* reward = *(u8**)(GameDbData() + 0x10A8) + id * 0x18;
        u8* save = lbl_805FBBE0;
        u32 money = *(u32*)(save + 0x12C) + FloatToU32(*(f32*)(reward + 0x10));
        *(u32*)(save + 0x12C) = money;
        if (money > 0x05F5E0FFu) *(u32*)(save + 0x12C) = 0x05F5E0FFu;

        s32 level = *(s32*)(save + 0x128);
        if (level < 0x31) {
            u32 k = (u32)(level + 1);
            s32 raised = 0;
            u8* ent = *(u8**)(GameDbData() + 0x63C8) + k * 0x10;
            f32 levelThr = *(f32*)(ent + 8);
            while (levelThr < (f32)*(u32*)(save + 0x12C) && k <= 0x31) {
                *(s32*)(save + 0x128) = (s32)k;
                k++;
                raised = 1;
                if (k <= 0x31) {
                    ent = *(u8**)(GameDbData() + 0x63C8) + k * 0x10;
                    levelThr = *(f32*)(ent + 8);
                }
            }
            if (raised != 0) {
                PostNotice(save, 1, *(s32*)(save + 0x128));
            }
        }
    }
}

}  // extern "C"
