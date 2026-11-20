// Small functions in 0x8013xxxx-0x8014xxxx: hit-test toggles, the register-table and
// callback-table helpers, the 0x80146xxx stream reader and the GX write-gather command writers.
#include <string.h>
#include "types.h"
#include "revolution.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

// Reinterprets a register-table word as an f32 bit pattern (no numeric conversion).
static inline f32 RegBitsToF32(u32 v) {
    union {
        u32 u;
        f32 f;
    } cv;
    cv.u = v;
    return cv.f;
}

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
s32   fn_801478C8(u8* table, s32 idx);                           // 0x801478C8
void  fn_80147990(s32 a, s32 b);                                 // 0x80147990
void  fn_80148228(void);                                         // 0x80148228
void  fn_80148314(s32 n, u8* buf);                               // 0x80148314
void  fn_8027FF24(void);                                         // 0x8027FF24
void  fn_8003DCE0(u8* p);                                        // 0x8003DCE0
void  fn_8004C718(u8* p, s32 idx, void* elem);                   // 0x8004C718
void  fn_8020EDAC(u8* p);                                        // 0x8020EDAC
s32   fn_80333424(const char* s, void* file);                    // 0x80333424
void  fn_80333B60(void* obj);                                    // 0x80333B60
u8*   fn_80146C44(u8* p, u8* src, s32* count);                   // 0x80146C44
void  WPADSetConnectCallback(s32 chan, void (*cb)(s32, s32));    // SDK call, no game address

extern u8*   lbl_805FB3C0;                                       // 0x805FB3C0 (table A)
extern u8    lbl_805FB3C4[];                                     // 0x805FB3C4 (table A flag word)
extern u8*   lbl_805FB3CC;                                       // 0x805FB3CC (table B)
extern s32   lbl_805FB458;                                       // 0x805FB458 (callback table count)
extern void* lbl_803F2830[];                                     // 0x803F2830 (callback table)
extern u8*   lbl_805FB490;                                       // 0x805FB490 (current register table)
extern u8*   lbl_805FB494;                                       // 0x805FB494 (spare register table)
extern u8    lbl_80400240[];                                     // 0x80400240 (stream buffer)
extern s32   lbl_80400FE0;                                       // 0x80400FE0
extern u8    lbl_803F20C4[];                                     // 0x803F20C4
extern char  lbl_80375E5C[];                                     // 0x80375E5C (format string)
extern u8    lbl_805F9211;                                       // 0x805F9211 (GX value byte)
extern u8    lbl_805F9212;                                       // 0x805F9212 (copy filter value)
extern u8    lbl_805FB50C;                                       // 0x805FB50C (GX shadow flags byte)
extern u8    lbl_805FB50E;                                       // 0x805FB50E (GX shadow byte)
extern s32   lbl_805FB514;                                       // 0x805FB514 (GX shadow word)
extern f32   lbl_805FF080;                                       // 0x805FF080 (float constant)
extern f32   lbl_805FF084;                                       // 0x805FF084 (float constant)
extern f32   lbl_805FF088;                                       // 0x805FF088 (float scale)
extern f32   lbl_805FF0FC;                                       // 0x805FF0FC (float scale)

// 0x8013E264
// Enables the three hit-test groups when arg0 == 1, otherwise disables them.
void fn_8013E264(s32 arg0) {
    if ((u32)arg0 == 1U) {
        UIGroup_EnableHitTest(0x4DD41D3DU);
        UIGroup_EnableHitTest(0x4D2C3770U);
        UIGroup_EnableHitTest(0x7CC42DEDU);
        return;
    }
    UIGroup_DisableHitTest(0x4DD41D3DU);
    UIGroup_DisableHitTest(0x4D2C3770U);
    UIGroup_DisableHitTest(0x7CC42DEDU);
}

// 0x8013E2DC
// Frees the five entries of both entry tables (table pointers at +0xC50), then resets the tables.
void fn_8013E2DC(void) {
    s32 i;

    if (lbl_805FB3C0 != NULL) {
        for (i = 0; i < 5; i++) {
            u8* elem = *(u8**)(lbl_805FB3C0 + 0xC50 + i * 4);
            fn_8004C718(lbl_805FB3C0, i, NULL);
            fn_8003DCE0(elem);
        }
        lbl_805FB3C0 = NULL;
    }
    *(s32*)lbl_805FB3C4 = 0;
    if (lbl_805FB3CC != NULL) {
        for (i = 0; i < 5; i++) {
            u8* elem = *(u8**)(lbl_805FB3CC + 0xC50 + i * 4);
            fn_8004C718(lbl_805FB3CC, i, NULL);
            fn_8003DCE0(elem);
        }
        lbl_805FB3CC = NULL;
    }
    fn_8020EDAC(lbl_803F20C4);
}

// 0x80142878
// Appends a callback pointer to the callback table (count in lbl_805FB458).
void fn_80142878(u8* arg0) {
    lbl_803F2830[lbl_805FB458] = arg0;
    lbl_805FB458 += 1;
}

// 0x8014328C
// Destructor: releases the handle at *p, frees p when del > 0, returns p.
void* fn_8014328C(u8* p, s32 del) {
    if (p != NULL) {
        if (*(u8**)p != NULL) {
            fn_80333424(lbl_80375E5C, *(u8**)p);
            fn_80333B60(*(void**)p);
        }
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x80146E44
// Copies count bytes from the stream at p+0x40 into dst; returns the bytes copied.
s32 fn_80146E44(u8* p, u8* dst, s32 count) {
    s32 total = 0;
    s32 n;
    u8* src;

    while (count != 0) {
        n = count;
        src = fn_80146C44(p, *(u8**)(p + 0x40), &n);
        if (n == 0) {
            break;
        }
        memcpy(dst, src, (u32)n);
        dst += n;
        *(u8**)(p + 0x40) = *(u8**)(p + 0x40) + n;
        count -= n;
        total += n;
    }
    return total;
}

// 0x80147800
// Decodes a register-table entry as a float: the top 3 bits pick the encoding.
f32 fn_80147800(s32 arg0) {
    u32 v = (u32)fn_801478C8(lbl_805FB490, arg0);

    switch (v >> 29) {
    case 1:
        return (v & 1) ? lbl_805FF080 : lbl_805FF084;
    case 2:
    case 4:
        return RegBitsToF32(v << 3);
    case 3:
        return lbl_805FF088 * (f32)(s32)((v >> 16) & 0x1FFF);
    default:
        return lbl_805FF084;
    }
}

// 0x8014826C
// Sets up pad state: clears the pad block, hooks connect callbacks, inits register tables.
void fn_8014826C(void) {
    s32 i;

    fn_8027FF24();
    memset(&lbl_80400FE0, 0, 0x10);
    for (i = 0; i <= 3; i++) {
        WPADSetConnectCallback(i, fn_80147990);
    }
    fn_80142878((u8*)(uintptr_t)&fn_80148228);
    memset(lbl_80400240, 0, 0xDA0);
    lbl_805FB494 = lbl_80400240;
    lbl_805FB490 = lbl_80400240 + 0x6D0;
    fn_80148314(0x64, lbl_80400240);
}

// 0x80149540
// Sets GX shadow flag bit 4 and writes the matching BP command pair.
void fn_80149540(void) {
    if (lbl_805FB50C & 4) {
        return;
    }
    lbl_805FB50C = (u8)(lbl_805FB50C | 4);
    lbl_805FB514 = (s32)((u32)lbl_805FB514 & 0xFFFFFFBFU);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)lbl_805FB514 | 0x43000000U;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)lbl_805FB50E | 0xF3380000U | ((u32)lbl_805F9211 << 16);
}

// 0x8014970C
// Stores the two GX values; when flag bit 4 is set, also writes the BP command pair.
void fn_8014970C(s32 arg0, s32 arg1) {
    lbl_805F9211 = (u8)arg0;
    lbl_805FB50E = (u8)arg1;
    if (lbl_805FB50C & 4) {
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)(u8)arg1 | 0xF3380000U | ((u32)(u8)arg0 << 16);
    }
}

// 0x80149744
// Stores the filter byte; when flag bit 8 is set, also writes the BP command pair.
void fn_80149744(s32 arg0) {
    lbl_805F9212 = (u8)arg0;
    if (lbl_805FB50C & 8) {
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0xFE00C000U;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)(u8)arg0 << 14;
    }
}

// 0x801497B0
// Scales farg0 by lbl_805FF0FC, clamps the integer to [6, 255] and writes the BP command pair.
void fn_801497B0(f32 farg0) {
    f32 t = lbl_805FF0FC * farg0;
    s32 v = (s32)t;

    if (v < 6) {
        v = 6;
    } else if (v > 0xFF) {
        v = 0xFF;
    }
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE0000FFU;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)(u8)v | 0x22000000U;
}

}  // extern "C"
