// Draft miss373: GX index-list quad draw (0x80151E38), model matrix upload (0x80155724), in-place
// texture tile deswizzle (0x8015B19C) and the effect-parameter dispatcher (0x80160BD4).
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "engine/rend.h"
#include "../engine/rend_dl_inline.h"

extern "C" {
void  fn_8014ACCC(u32 mode, s32 fmt, s32 w, s32 h, s32* outSize, s32* outStride, s32* outW, s32* outH);   // 0x8014ACCC
s32   fn_80142EE0(void);                 // 0x80142EE0: size of the renderer's scratch buffer
void* fn_80142ED4(void);                 // 0x80142ED4: the renderer's scratch buffer
// Effect parameter initialisers: (manager, record, parameter block, link). Only dst (r4) and src (r5) are used.
void fn_80162D4C(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80162D4C
void fn_80162FD0(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80162FD0
void fn_80163418(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80163418
void fn_80163750(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80163750
void fn_80163A14(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80163A14
void fn_8016481C(u8* mgr, u8* dst, u8* src, u8* link);   // 0x8016481C
void fn_80165320(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80165320
void fn_80165D70(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80165D70
void fn_80166034(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80166034
void fn_80166314(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80166314
void fn_801664C4(u8* mgr, u8* dst, u8* src, u8* link);   // 0x801664C4
void fn_80166B68(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80166B68
void fn_80166EB0(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80166EB0
void fn_80168100(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80168100
void fn_80168350(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80168350
void fn_801688D4(u8* mgr, u8* dst, u8* src, u8* link);   // 0x801688D4
void fn_80168D1C(u8* mgr, u8* dst, u8* src, u8* link);   // 0x80168D1C
void fn_801691A8(u8* mgr, u8* dst, u8* src, u8* link);   // 0x801691A8

extern u8  lbl_80400FF0[];               // 0x80400FF0 (renderer matrix block: +0x10 GX matrix, +0x58 model, +0x88 view x model, +0xE8 view)
extern f32 lbl_805FB510;                 // 0x805FB510
extern f32 lbl_805FF2A8;   // 0x805FF2A8 (default float of the effect parameter block)
extern f32 lbl_805FF2AC;   // 0x805FF2AC (default float of the effect parameter block)
extern f32 lbl_805FF2B0;   // 0x805FF2B0 (default float of the effect parameter block)
extern f32 lbl_805FF2B4;   // 0x805FF2B4 (default float of the effect parameter block)
extern f32 lbl_805FF2B8;   // 0x805FF2B8 (default float of the effect parameter block)
extern f32 lbl_805FF2BC;   // 0x805FF2BC (default float of the effect parameter block)
extern f32 lbl_805FF2C0;   // 0x805FF2C0 (default float of the effect parameter block)
extern f32 lbl_805FF2C4;   // 0x805FF2C4 (default float of the effect parameter block)
extern f32 lbl_805FF2C8;   // 0x805FF2C8 (default float of the effect parameter block)
extern f32 lbl_805FF2CC;   // 0x805FF2CC (default float of the effect parameter block)
extern f32 lbl_805FF2D0;   // 0x805FF2D0 (default float of the effect parameter block)
extern f32 lbl_805FF2D4;   // 0x805FF2D4 (default float of the effect parameter block)
extern f32 lbl_805FF2D8;   // 0x805FF2D8 (default float of the effect parameter block)
extern f32 lbl_805FF2DC;   // 0x805FF2DC (default float of the effect parameter block)
extern f32 lbl_805FF2E0;   // 0x805FF2E0 (default float of the effect parameter block)
extern f32 lbl_805FF2E4;   // 0x805FF2E4 (default float of the effect parameter block)
extern f32 lbl_805FF2E8;   // 0x805FF2E8 (default float of the effect parameter block)
extern f32 lbl_805FF2EC;   // 0x805FF2EC (default float of the effect parameter block)
extern f32 lbl_805FF2F0;   // 0x805FF2F0 (default float of the effect parameter block)
extern f32 lbl_805FF2F4;   // 0x805FF2F4 (default float of the effect parameter block)
extern f32 lbl_805FF2F8;   // 0x805FF2F8 (default float of the effect parameter block)
extern f32 lbl_805FF2FC;   // 0x805FF2FC (default float of the effect parameter block)
extern f32 lbl_805FF300;   // 0x805FF300 (default float of the effect parameter block)
extern f32 lbl_805FF304;   // 0x805FF304 (default float of the effect parameter block)
extern f32 lbl_805FF308;   // 0x805FF308 (default float of the effect parameter block)
extern f32 lbl_805FF30C;   // 0x805FF30C (default float of the effect parameter block)
extern f32 lbl_805FF310;   // 0x805FF310 (default float of the effect parameter block)
extern f32 lbl_805FF314;   // 0x805FF314 (default float of the effect parameter block)
extern f32 lbl_805FF318;   // 0x805FF318 (default float of the effect parameter block)
extern f32 lbl_805FF31C;   // 0x805FF31C (default float of the effect parameter block)
extern f32 lbl_805FF320;   // 0x805FF320 (default float of the effect parameter block)
extern f32 lbl_805FF324;   // 0x805FF324 (default float of the effect parameter block)
extern f32 lbl_805FF328;   // 0x805FF328 (default float of the effect parameter block)
extern f32 lbl_805FF32C;   // 0x805FF32C (default float of the effect parameter block)
extern f32 lbl_805FF330;   // 0x805FF330 (default float of the effect parameter block)
extern f32 lbl_805FF334;   // 0x805FF334 (default float of the effect parameter block)
extern f32 lbl_805FF338;   // 0x805FF338 (default float of the effect parameter block)
extern f32 lbl_805FF33C;   // 0x805FF33C (default float of the effect parameter block)
extern f32 lbl_805FF340;   // 0x805FF340 (default float of the effect parameter block)
extern f32 lbl_805FF344;   // 0x805FF344 (default float of the effect parameter block)
extern f32 lbl_805FF348;   // 0x805FF348 (default float of the effect parameter block)
extern f32 lbl_805FF34C;   // 0x805FF34C (default float of the effect parameter block)
}

// Writes the quad index list: per quad four indices, each sent three times (n, n+1, n+3, n+2), the
// base advancing by 4. Indices are bytes or halfwords depending on the vertex count.
static void WriteQuadIndices(s32 n, bool wide) {
    u32 idx = 0;
    for (s32 i = 0; i < n; i++) {
        u32 order[4] = {idx, idx + 1, idx + 3, idx + 2};
        for (s32 k = 0; k < 4; k++) {
            for (s32 t = 0; t < 3; t++) {
                if (wide) GXWGFifo._u16 = (u16)order[k];
                else GXWGFifo._u8 = (u8)order[k];
            }
        }
        idx = wide ? (u16)(idx + 4) : (u8)(idx + 4);
    }
}

extern "C" {

// 0x80151E38
// Draws `quads` quads from the vertex array (0x18 bytes per vertex, 4 vertices per quad) as an
// indexed GX draw: points the CP array registers at the (flushed) vertex data, makes room in the
// display list, then writes the 0x80 draw command with the index list. Fewer than 0x40 quads use
// byte indices, otherwise halfword indices.
void fn_80151E38(s32 quads, void* verts) {
    u8* v = (u8*)verts;
    s32 vcount = quads << 2;
    fn_80142ED0(v, vcount * 0x18);
    XfHead(0, 0x1008);
    GXWGFifo._u32 = 0x11;
    Cp(0xA0, (u32)((uintptr_t)(v + 0xC) & 0x3FFFFFFF));
    Cp(0xA2, (u32)((uintptr_t)(v + 8) & 0x3FFFFFFF));
    Cp(0xA4, (u32)((uintptr_t)v & 0x3FFFFFFF));
    Cp(0xB0, 0x18);
    Cp(0xB2, 0x18);
    Cp(0xB4, 0x18);
    if (quads < 0x40) {
        Cp(0x50, 0x4400);
        Cp(0x60, 2);
        GXWGFifo._u8 = 0x48;
        EnsureDLRoom(quads * 0xC + 0x10C0);
    } else {
        Cp(0x50, 0x6600);
        Cp(0x60, 3);
        GXWGFifo._u8 = 0x48;
        EnsureDLRoom(quads * 0x18 + 0x10C0);
    }
    GXWGFifo._u8 = 0x80;
    GXWGFifo._u16 = (u16)vcount;
    WriteQuadIndices(quads, quads >= 0x40);
}

// 0x80155724
// Sets the renderer's model matrix (3x4, column order) from m: stores it at +0x58, builds
// view x model into +0x88 (the view matrix is at +0xE8), then loads the result as GX position
// matrix 0 (transposed into +0x10 and sent through the pipe).
void fn_80155724(u8* m) {
    const f32* view = (const f32*)(lbl_80400FF0 + 0xE8);
    f32* model = (f32*)(lbl_80400FF0 + 0x58);
    f32* comb = (f32*)(lbl_80400FF0 + 0x88);
    f32* gx = (f32*)(lbl_80400FF0 + 0x10);

    f32 in[12];
    for (s32 i = 0; i < 12; i++) {
        in[i] = ((const f32*)m)[i];
    }
    f32 c[12];
    for (s32 j = 0; j < 4; j++) {
        for (s32 i = 0; i < 3; i++) {
            f32 r = view[6 + i] * in[3 * j + 2] + (view[i] * in[3 * j] + view[3 + i] * in[3 * j + 1]);
            if (j == 3) {
                r = view[9 + i] + r;
            }
            c[3 * j + i] = r;
        }
    }
    for (s32 i = 0; i < 12; i++) {
        model[i] = in[i];
    }
    lbl_805FB510 = 0.0f;
    for (s32 i = 0; i < 12; i++) {
        comb[i] = c[i];
    }

    EnsureDLRoom(0x10FC);

    for (s32 row = 0; row < 3; row++) {
        for (s32 col = 0; col < 4; col++) {
            gx[row * 4 + col] = comb[col * 3 + row];
        }
    }
    XfHead(0xB, 0);
    for (s32 i = 0; i < 12; i++) {
        GXWGFifo._f32 = gx[i];
    }
}

// 0x8015B19C
// Converts a texture's pixel data from GX tiled order to linear rows, in place, for one mip level
// (skipping `skip` levels first). tex: +4 width, +6 height, +9 format, +0xA bits per pixel, +0x18
// pixel data. Each band of rows is rearranged in a scratch buffer and copied back.
void fn_8015B19C(u8* tex, s32 skip) {
    if (tex[9] == 8) return;
    s32 w = *(u16*)(tex + 4);
    s32 h = *(u16*)(tex + 6);
    u8* data = *(u8**)(tex + 0x18);
    s32 size, stride, outW, outH;
    for (;;) {
        fn_8014ACCC(4, tex[9], w, h, &size, &stride, &outW, &outH);
        if (--skip < 0) break;
        w >>= 1;
        h >>= 1;
        data += size;
    }
    u8 bpp = tex[0xA];
    if (bpp == 0x20) {
        s32 n = stride * 4;
        u8* buf = (n > fn_80142EE0()) ? (u8*)gRendMem->allocTemp((u32)n) : (u8*)fn_80142ED4();
        s32 bands = outH >> 2;
        s32 tiles = stride >> 4;
        while (bands-- > 0) {
            u8* src = data;
            for (s32 t = 0; t < tiles; t++) {
                for (s32 k = 0; k < 4; k++) {
                    u8* d = buf + k * stride + t * 0x10;
                    for (s32 j = 0; j < 4; j++) {
                        *(u16*)(d + j * 4) = *(u16*)(src + k * 8 + j * 2);
                        *(u16*)(d + j * 4 + 2) = *(u16*)(src + 0x20 + k * 8 + j * 2);
                    }
                }
                src += 0x40;
            }
            memcpy(data, buf, (u32)n);
            data += n;
        }
        if (n > fn_80142EE0()) gRendMem->freeAny(buf);
    } else if (bpp == 0x10 || bpp == 8) {
        s32 n = stride * 4;
        u8* buf = (n > fn_80142EE0()) ? (u8*)gRendMem->allocTemp((u32)n) : (u8*)fn_80142ED4();
        s32 bands = outH >> 2;
        s32 tiles = stride >> 3;
        while (bands-- > 0) {
            u8* src = data;
            for (s32 t = 0; t < tiles; t++) {
                for (s32 k = 0; k < 4; k++) {
                    u8* d = buf + k * stride + t * 8;
                    *(u32*)(d + 0) = *(u32*)(src + k * 8 + 0);
                    *(u32*)(d + 4) = *(u32*)(src + k * 8 + 4);
                }
                src += 0x20;
            }
            memcpy(data, buf, (u32)n);
            data += n;
        }
        if (n > fn_80142EE0()) gRendMem->freeAny(buf);
    } else if (bpp == 4) {
        s32 n = stride * 8;
        u8* buf = (n > fn_80142EE0()) ? (u8*)gRendMem->allocTemp((u32)n) : (u8*)fn_80142ED4();
        s32 bands = outH >> 3;
        s32 tiles = stride >> 2;
        while (bands-- > 0) {
            u8* src = data;
            for (s32 t = 0; t < tiles; t++) {
                for (s32 k = 0; k < 8; k++) {
                    *(u32*)(buf + k * stride + t * 4) = *(u32*)(src + k * 4);
                }
                src += 0x20;
            }
            memcpy(data, buf, (u32)n);
            data += n;
        }
        if (n > fn_80142EE0()) gRendMem->freeAny(buf);
    }
}

// 0x80160BD4
// Starts effect kind `a` (0..0x14) for a record: when the caller gave no parameter block, one with
// that kind's default values is built on the stack; then the kind's initialiser copies the block
// into the record (item). Kinds 7 and 10 always use the caller's block. Returns 0 for an unknown
// kind, else 1.
s32 fn_80160BD4(u8* mgr, s32 a, u8* item, void* c, u8* link) {
    u8* params = (u8*)c;
    u8 def[0x20];
    if ((u32)a > 0x14) return 0;
    switch (a) {
    case 0:
        if (params == NULL) {
            params = def;
            *(s32*)(def + 0x0) = 1;
            *(f32*)(def + 0x4) = lbl_805FF2A8;
            *(f32*)(def + 0x8) = lbl_805FF2AC;
        }
        fn_80162D4C(mgr, item, params, link);
        break;
    case 1:
        if (params == NULL) {
            params = def;
            *(s32*)(def + 0x0) = 2;
            *(f32*)(def + 0x4) = lbl_805FF2A8;
            *(f32*)(def + 0x8) = lbl_805FF2AC;
        }
        fn_80162D4C(mgr, item, params, link);
        break;
    case 2:
        if (params == NULL) {
            params = def;
            *(s32*)(def + 0x0) = 3;
            *(f32*)(def + 0x4) = lbl_805FF2B0;
            *(f32*)(def + 0x8) = lbl_805FF2B4;
        }
        fn_80162D4C(mgr, item, params, link);
        break;
    case 3:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2B8;
            *(f32*)(def + 0x4) = lbl_805FF2BC;
            *(f32*)(def + 0x8) = lbl_805FF2C0;
        }
        fn_80166314(mgr, item, params, link);
        break;
    case 4:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2C4;
            *(f32*)(def + 0x4) = lbl_805FF2C8;
            *(f32*)(def + 0x8) = lbl_805FF2C8;
            *(f32*)(def + 0xC) = lbl_805FF2C8;
            *(f32*)(def + 0x10) = lbl_805FF2A8;
            *(f32*)(def + 0x14) = lbl_805FF2CC;
            *(f32*)(def + 0x18) = lbl_805FF2D0;
        }
        fn_801688D4(mgr, item, params, link);
        break;
    case 5:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2D4;
            *(f32*)(def + 0x4) = lbl_805FF2D4;
            *(f32*)(def + 0x8) = lbl_805FF2D4;
            *(f32*)(def + 0xC) = lbl_805FF2C4;
            *(f32*)(def + 0x10) = lbl_805FF2D8;
            *(f32*)(def + 0x14) = lbl_805FF2D8;
            *(f32*)(def + 0x18) = lbl_805FF2D8;
            *(f32*)(def + 0x1C) = lbl_805FF2C4;
        }
        fn_801664C4(mgr, item, params, link);
        break;
    case 6:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2C4;
            *(f32*)(def + 0x4) = lbl_805FF2DC;
            *(f32*)(def + 0x8) = lbl_805FF2E0;
            *(f32*)(def + 0xC) = lbl_805FF2C4;
            *(f32*)(def + 0x10) = lbl_805FF2E4;
            *(f32*)(def + 0x14) = lbl_805FF2E8;
            *(f32*)(def + 0x18) = lbl_805FF2EC;
            *(f32*)(def + 0x1C) = lbl_805FF2C4;
        }
        fn_801664C4(mgr, item, params, link);
        break;
    case 7:
        fn_80168350(mgr, item, params, link);
        break;
    case 8:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2F0;
            *(f32*)(def + 0x4) = lbl_805FF2F4;
            *(f32*)(def + 0x8) = lbl_805FF2F8;
        }
        fn_80163750(mgr, item, params, link);
        break;
    case 9:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2A8;
            *(f32*)(def + 0x4) = lbl_805FF2A8;
            *(f32*)(def + 0x8) = lbl_805FF2C4;
        }
        fn_80168100(mgr, item, params, link);
        break;
    case 10:
        fn_80166034(mgr, item, params, link);
        break;
    case 11:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2FC;
            *(f32*)(def + 0x4) = lbl_805FF300;
            *(f32*)(def + 0x8) = lbl_805FF304;
            *(f32*)(def + 0xC) = lbl_805FF2C8;
            *(f32*)(def + 0x10) = lbl_805FF2A8;
            *(s32*)(def + 0x14) = 0;
        }
        fn_80162FD0(mgr, item, params, link);
        break;
    case 12:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF308;
            *(f32*)(def + 0x4) = lbl_805FF2C4;
            *(f32*)(def + 0x8) = lbl_805FF30C;
            *(f32*)(def + 0xC) = lbl_805FF310;
            *(s32*)(def + 0x10) = 2;
        }
        fn_80163418(mgr, item, params, link);
        break;
    case 13:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF308;
            *(f32*)(def + 0x4) = lbl_805FF2A8;
            *(f32*)(def + 0x8) = lbl_805FF314;
            *(f32*)(def + 0xC) = lbl_805FF304;
            *(f32*)(def + 0x10) = lbl_805FF318;
            *(s32*)(def + 0x14) = 1;
        }
        fn_801691A8(mgr, item, params, link);
        break;
    case 14:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2F0;
            *(f32*)(def + 0x4) = lbl_805FF2C4;
            *(f32*)(def + 0x8) = lbl_805FF318;
            *(f32*)(def + 0xC) = lbl_805FF31C;
            *(f32*)(def + 0x10) = lbl_805FF320;
            *(f32*)(def + 0x14) = lbl_805FF324;
            *(s32*)(def + 0x18) = 2;
        }
        fn_80168D1C(mgr, item, params, link);
        break;
    case 15:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2C0;
            *(f32*)(def + 0x4) = lbl_805FF2C4;
            *(f32*)(def + 0x8) = lbl_805FF328;
            *(f32*)(def + 0xC) = lbl_805FF328;
            *(f32*)(def + 0x10) = lbl_805FF328;
            *(f32*)(def + 0x14) = lbl_805FF328;
        }
        fn_80166EB0(mgr, item, params, link);
        break;
    case 16:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2C4;
            *(f32*)(def + 0x4) = lbl_805FF32C;
            *(f32*)(def + 0x8) = lbl_805FF2C4;
            *(f32*)(def + 0xC) = lbl_805FF2C4;
            *(f32*)(def + 0x10) = lbl_805FF2C4;
            *(f32*)(def + 0x14) = lbl_805FF308;
        }
        fn_8016481C(mgr, item, params, link);
        break;
    case 17:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2C4;
            *(s32*)(def + 0x4) = 30;
            *(f32*)(def + 0x8) = lbl_805FF330;
            *(f32*)(def + 0xC) = lbl_805FF32C;
        }
        fn_80163A14(mgr, item, params, link);
        break;
    case 18:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2C4;
            *(f32*)(def + 0x4) = lbl_805FF2C0;
            *(f32*)(def + 0x8) = lbl_805FF334;
            *(f32*)(def + 0xC) = lbl_805FF32C;
        }
        fn_80165D70(mgr, item, params, link);
        break;
    case 19:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF2C4;
            *(f32*)(def + 0x4) = lbl_805FF338;
            *(f32*)(def + 0x8) = lbl_805FF308;
            *(f32*)(def + 0xC) = lbl_805FF2C4;
            *(f32*)(def + 0x10) = lbl_805FF2BC;
            *(f32*)(def + 0x14) = lbl_805FF33C;
        }
        fn_80165320(mgr, item, params, link);
        break;
    case 20:
        if (params == NULL) {
            params = def;
            *(f32*)(def + 0x0) = lbl_805FF340;
            *(f32*)(def + 0x4) = lbl_805FF2C4;
            *(f32*)(def + 0x8) = lbl_805FF344;
            *(f32*)(def + 0xC) = lbl_805FF2C4;
            *(f32*)(def + 0x10) = lbl_805FF348;
            *(f32*)(def + 0x14) = lbl_805FF34C;
            *(f32*)(def + 0x18) = lbl_805FF348;
            *(f32*)(def + 0x1C) = lbl_805FF2C4;
        }
        fn_80166B68(mgr, item, params, link);
        break;
    }
    return 1;
}

}  // extern "C"
