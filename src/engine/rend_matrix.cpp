// Renderer: takes a new view matrix (world to view, 12 floats in column order: three axes, then
// the translation), combines it with the current model matrix and loads the result as GX
// position matrix 0; also keeps the view matrix and a normalised, transposed copy of its 3x3.
// Original: 0x80154E54 (auto_03_801467F8_text.s)
#include <math.h>
#include "rend_dl_inline.h"

extern "C" {
void fn_80154398(void);                 // 0x80154398

// The renderer's matrix block. Offsets used here:
//   +0x10  position matrix as sent to GX (row order, 3x4)
//   +0x58  model matrix (column order)
//   +0x88  view x model (column order)
//   +0xB8  normalised transpose of the view matrix's 3x3, no translation
//   +0xE8  the view matrix
extern u8 lbl_80400FF0[];               // 0x80400FF0
}

// The original refines frsqrte twice; the result is 1 / sqrt to float precision.
static inline void Normalise3(f32* out, f32 x, f32 y, f32 z) {
    f32 len2 = z * z + (x * x + y * y);
    if (len2 == 0.0f) {
        out[0] = out[1] = out[2] = 0.0f;
    } else {
        f32 r = (f32)(1.0 / sqrt((f64)len2));
        out[0] = x * r;
        out[1] = y * r;
        out[2] = z * r;
    }
}

// 0x80154E54
extern "C" void fn_80154E54(u8* viewMtx) {
    const f32* a = (const f32*)viewMtx;
    f32* gx = (f32*)(lbl_80400FF0 + 0x10);
    const f32* model = (const f32*)(lbl_80400FF0 + 0x58);
    f32* comb = (f32*)(lbl_80400FF0 + 0x88);
    f32* nrm = (f32*)(lbl_80400FF0 + 0xB8);
    f32* view = (f32*)(lbl_80400FF0 + 0xE8);

    f32 in[12];
    for (s32 i = 0; i < 12; i++) {
        in[i] = a[i];
    }
    f32 c[12];
    for (s32 j = 0; j < 4; j++) {
        for (s32 i = 0; i < 3; i++) {
            f32 v = in[6 + i] * model[3 * j + 2] + (in[i] * model[3 * j] + in[3 + i] * model[3 * j + 1]);
            if (j == 3) {
                v = in[9 + i] + v;
            }
            c[3 * j + i] = v;
        }
    }
    for (s32 i = 0; i < 12; i++) {
        view[i] = in[i];
    }
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

    // Transpose of the view's 3x3, each row normalised.
    f32 t[9];
    for (s32 r = 0; r < 3; r++) {
        for (s32 k = 0; k < 3; k++) {
            t[r * 3 + k] = view[k * 3 + r];
        }
    }
    Normalise3(&nrm[0], t[0], t[1], t[2]);
    Normalise3(&nrm[3], t[3], t[4], t[5]);
    Normalise3(&nrm[6], t[6], t[7], t[8]);
    nrm[9] = 0.0f;
    nrm[10] = 0.0f;
    nrm[11] = 0.0f;
    fn_80154398();
}
