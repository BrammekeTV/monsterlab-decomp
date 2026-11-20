// Misc 0x8015xxxx-0x8016xxxx functions: render display-list ring, GX BP helpers, texture-manager
// cleanup, frame-timing stats.
#include <string.h>
#include "types.h"
#include "revolution.h"
#include "engine/rend.h"

extern "C" {
void   fn_80142ED0(u8* base, s32 size);                                    // 0x80142ED0 (DCFlushRange)
void   fn_8014E090(void);                                                  // 0x8014E090
void   fn_8014E198(void);                                                  // 0x8014E198
void   fn_8014E2D8(void);                                                  // 0x8014E2D8
void   fn_8014E2E4(void);                                                  // 0x8014E2E4
void   fn_8014EAF4(void);                                                  // 0x8014EAF4
void   fn_8014ED64(s32 a);                                                 // 0x8014ED64
void   fn_8014FCB8(void);                                                  // 0x8014FCB8
void   fn_80150148(void);                                                  // 0x80150148
void   fn_801542DC(void);                                                  // 0x801542DC
void   fn_80151264(void);                                                  // 0x80151264
void   fn_80156898(s32 x, s32 y, s32 w, s32 h);                            // 0x80156898
void   fn_801598AC(void);                                                  // 0x801598AC
void   fn_80159C60(s32 a);                                                 // 0x80159C60
void   fn_8015A02C(void);                                                  // 0x8015A02C
void   fn_801601DC(u8* p);                                                 // 0x801601DC
void   fn_801608C8(u8* p);                                                 // 0x801608C8
s32    fn_8014ACCC(s32, s32, s32, s32, s32*, s32*, s32, s32);              // 0x8014ACCC
u64    fn_801425DC(u8* timer);                                             // 0x801425DC
void   fn_8016A2B8(u8* p);                                                 // 0x8016A2B8
void   fn_80172E84(u8* obj);                                               // 0x80172E84
void*  fn_80172F70(s32 a, const void* b);                                  // 0x80172F70
void   fn_801731C8(void* p);                                               // 0x801731C8
void   fn_801732B0(u8* p);                                                 // 0x801732B0
s32    fn_80174C2C(u8* p);                                                 // 0x80174C2C
void   fn_80188D24(void);                                                  // 0x80188D24
void   fn_80188F00(const char* fmt, ...);                                  // 0x80188F00
void   TextureRelease(void* tex);                                          // 0x80184028

extern u8   sRendDLs[];             // 0x80401108 (display-list records, 0x94 bytes each)
extern u8   sRendDLRecords[];       // 0x80401ADC (submit records, 0x14 bytes each)
extern s32  gViewportIndex;         // 0x805FB4BC (display list being recorded)
extern s32  gViewportCount;         // 0x805FB4C0 (1 while recording)
extern s32  gFrameBudget;           // 0x805FB4C8 (bytes still needing a cache flush start here)
extern s32  lbl_805FB4B0;           // 0x805FB4B0
extern s32  lbl_805FB4B4;           // 0x805FB4B4
extern s32  lbl_805FB4C4;           // 0x805FB4C4 (display-list capacity)
extern s32  lbl_805FB4CC;           // 0x805FB4CC
extern u8*  lbl_805FB4D0;           // 0x805FB4D0 (display-list write cursor)
extern s32  lbl_805FB4D4;           // 0x805FB4D4 (defer-submit flag)
extern s32  lbl_805FB4D8;           // 0x805FB4D8 (defer queue head)
extern s32  lbl_805FB4DC;           // 0x805FB4DC (defer queue count)
extern s32  lbl_805FB4E0;           // 0x805FB4E0 (keep-display-lists flag)
extern s32  lbl_805FB4E8;           // 0x805FB4E8 (kept display-list count)
extern s32  lbl_805FB4EC;           // 0x805FB4EC (display-list balance)
extern s32 (*lbl_805FB4F4)(void);   // 0x805FB4F4 (optional "can render" callback)
extern s32  lbl_805FB520;           // 0x805FB520 (pending render-mode change)
extern u8   lbl_805FB524;           // 0x805FB524 (fog enable)
extern void* lbl_805FB528;          // 0x805FB528
extern void* lbl_805FB52C;          // 0x805FB52C
extern u8*  lbl_805FB5A8;           // 0x805FB5A8 (texture manager)
extern void* lbl_805FB5AC;          // 0x805FB5AC
extern u8*  lbl_805FB5B0;           // 0x805FB5B0
extern u8*  lbl_805FB5DC;           // 0x805FB5DC (default light)
extern s32  lbl_805FB5E0;           // 0x805FB5E0 (frame-time ring index)
extern s32  lbl_805FB5E4;           // 0x805FB5E4 (frame-time ring count)
extern u32  lbl_805FB5E8;           // 0x805FB5E8 (previous timer, high word)
extern u32  lbl_805FB5EC;           // 0x805FB5EC (previous timer, low word)
extern u8   lbl_805FB5F0[];         // 0x805FB5F0 (timer)
extern u8*  lbl_805FB608;           // 0x805FB608 (allocator object; alloc fn at +0)
extern s32  lbl_805FB628;           // 0x805FB628
extern s32  lbl_805FB62C;           // 0x805FB62C
extern s32  lbl_805FB630;           // 0x805FB630
extern s32  lbl_805FB63C;           // 0x805FB63C
extern u8   lbl_805F9230;           // 0x805F9230 (fog type: 0 = linear)
extern f32  lbl_805F9220;           // 0x805F9220 (fog near)
extern f32  lbl_805F9224;           // 0x805F9224 (fog far)
extern f32  lbl_805F9228;           // 0x805F9228 (fog range start)
extern f32  lbl_805F922C;           // 0x805F922C (fog range end)
extern u8   lbl_80401C40[];         // 0x80401C40 (grid positions, 12 bytes each)
extern u8   lbl_80403740[];         // 0x80403740 (grid colours, 4 bytes each)
extern u8   lbl_8040ADD0[];         // 0x8040ADD0
extern u8   lbl_8040D500[];         // 0x8040D500
extern u8   lbl_8040E260[];         // 0x8040E260
extern u8   lbl_80412998[];         // 0x80412998 (frame stats, 2 x 0x44 + ring at +0xD8)
extern s32  lbl_80412A2C[];         // 0x80412A2C (second stats block)
extern const char lbl_80376A10[];   // 0x80376A10 (stat format strings)
}

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

static u32 FloatBits(f32 v) {
    union { f32 f; u32 u; } c;
    c.f = v;
    return c.u;
}

extern "C" {

// 0x80150014
// Applies a pending render-mode change (lbl_805FB520: 1 = reset viewport, 2..3 = full re-setup).
void fn_80150014(void) {
    if (lbl_805FB520 == 0) {
        return;
    }
    if (lbl_805FB4F4 != NULL && lbl_805FB4F4() == 0) {
        fn_80150148();
        return;
    }
    if (lbl_805FB520 == 1) {
        fn_8014EAF4();
        s32 h = VideoGetHeight(&gVideo);
        s32 w = VideoGetWidth(&gVideo);
        fn_80156898(0, 0, w, h);
        fn_801598AC();
        fn_8015A02C();
        lbl_805FB520 = 0;
    } else if (lbl_805FB520 >= 1 && lbl_805FB520 < 4) {
        fn_8014E090();
        fn_8014E2D8();
        fn_8014E198();
        fn_80159C60(1);
        fn_8014ED64(0);
        fn_8014E090();
        fn_8014E2E4();
        fn_8014E198();
        fn_8014FCB8();
        fn_80159C60(0);
        fn_8014EAF4();
        s32 h = VideoGetHeight(&gVideo);
        s32 w = VideoGetWidth(&gVideo);
        fn_80156898(0, 0, w, h);
        fn_8014FCB8();
        fn_8015A02C();
        lbl_805FB520 = 0;
    }
    fn_8014FCB8();
    if (lbl_805FB520 == 0) {
        fn_801542DC();
    }
}

// 0x80150DEC
// Finishes the display list being recorded (flush, submit or defer), advances to the next
// display-list slot and starts recording into it.
// (The original passes the render-thread object &lbl_805FB594 as a hidden first argument to the
// Rend* thread calls; the repo's prototypes drop it.)
void fn_80150DEC(void) {
    if (gViewportCount == 0) {
        return;
    }
    RendDLEnd((RendDL*)(sRendDLs + gViewportIndex * 0x94));
    s32 unflushed = 0x24000 - gFrameBudget;
    if (unflushed != 0) {
        fn_80142ED0(RendDLGetBase((RendDL*)(sRendDLs + gViewportIndex * 0x94)) + gFrameBudget, unflushed);
    }
    if (lbl_805FB4D4 != 0) {
        lbl_805FB4DC += 1;
    } else {
        RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + gViewportIndex * 0x14), lbl_805FB4E0 == 0);
    }
    gViewportCount = 0;
    lbl_805FB4D0 = NULL;
    lbl_805FB4C4 = 0;
    gFrameBudget = 0;
    lbl_805FB4CC = 0;
    if (lbl_805FB4E0 != 0) {
        lbl_805FB4E8 += 1;
    }
    gViewportIndex = gViewportIndex + 1;
    if (gViewportIndex >= 0x11) {
        gViewportIndex = 0;
    }
    if (lbl_805FB4D4 != 0 && !RendHasFreeDL() && lbl_805FB4D4 != 0) {
        RendThreadSync();
        while (lbl_805FB4DC > 0) {
            RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + lbl_805FB4D8 * 0x14), lbl_805FB4E0 == 0);
            lbl_805FB4D8 = lbl_805FB4D8 + 1;
            if (lbl_805FB4D8 >= 0x11) {
                lbl_805FB4D8 = 0;
            }
            lbl_805FB4DC = lbl_805FB4DC - 1;
        }
        lbl_805FB4D4 = 0;
        lbl_805FB4D8 = 0;
        lbl_805FB4DC = 0;
    }
    RendWaitFreeDL();
    RendDLBegin((RendDL*)(sRendDLs + gViewportIndex * 0x94), (RendDLRecord*)(sRendDLRecords + gViewportIndex * 0x14));
    gViewportCount = 1;
    lbl_805FB4C4 = 0x24000;
    gFrameBudget = 0x24000;
    lbl_805FB4CC = 0;
    lbl_805FB4D0 = RendDLGetBase((RendDL*)(sRendDLs + gViewportIndex * 0x94));
    lbl_805FB4EC -= 1;
}

// 0x80151264
// Sends the four BP fog-parameter registers (0xEE..0xF1) built from the fog range; sends the
// disabled setting when fog is off.
void fn_80151264(void) {
    if (lbl_805FB524 == 0) {
        f32 z = 0.0f;
        GxBpWrite(0xEE000000 | (FloatBits(z) >> 12));
        GxBpWrite(0xEF40000F);
        GxBpWrite(0xF0000001);
        GxBpWrite(0xF1000000 | (FloatBits(z) >> 12));
        return;
    }
    if (lbl_805F9230 != 0) {
        f32 a;       // f3
        f32 b;       // f4
        f32 c;       // f30
        if (lbl_805F9220 == lbl_805F9224 || lbl_805F9228 == lbl_805F922C) {
            a = 0.0f;
            b = 0.5f;
            c = 0.0f;
        } else {
            f32 d0 = lbl_805F9224 - lbl_805F9220;
            f32 d1 = lbl_805F922C - lbl_805F9228;
            b = lbl_805F9224 / d0;
            c = lbl_805F9228 / d1;
            a = (lbl_805F9224 * lbl_805F9220) / (d0 * d1);
        }
        s32 e = 1;
        while (b > 1.0f) {
            b *= 0.5f;
            e += 1;
        }
        while (b > 0.0f && b < 0.5f) {
            b *= 2.0f;
            e -= 1;
        }
        f32 s = a / (f32)(1 << e);
        u32 m = (u32)(8388638.0f * b);
        GxBpWrite(0xEE000000 | (FloatBits(s) >> 12));
        GxBpWrite(0xEF000000 | m);
        GxBpWrite(0xF0000000 | (u32)e);
        GxBpWrite(0xF1400000 | (FloatBits(c) >> 12));
        return;
    }
    f32 p;
    f32 q;
    if (lbl_805F9220 == lbl_805F9224 || lbl_805F9228 == lbl_805F922C) {
        p = 0.0f;
        q = 0.0f;
    } else {
        f32 d = lbl_805F922C - lbl_805F9228;
        p = (lbl_805F9224 - lbl_805F9220) / d;
        q = (lbl_805F9228 - lbl_805F9220) / d;
    }
    GxBpWrite(0xEE000000 | (FloatBits(p) >> 12));
    GxBpWrite(0xEF000000);
    GxBpWrite(0xF0000000);
    GxBpWrite(0xF1500000 | (FloatBits(q) >> 12));
}

// 0x8015150C
// Sets the fog range (a, b) and fog colour (RGBA floats 0..1, clamped to bytes) via BP register
// 0xF2; re-sends the fog parameters when fog is enabled.
void fn_8015150C(const f32* v, f32 a, f32 b) {
    s32 r = (s32)(255.0f * v[0]);
    s32 g = (s32)(255.0f * v[1]);
    s32 bl = (s32)(255.0f * v[2]);
    s32 al = (s32)(255.0f * v[3]);
    lbl_805F9228 = a;
    lbl_805F922C = b;
    s32 cr = r < 0 ? 0 : (r > 0xFF ? 0xFF : r);
    s32 cg = g < 0 ? 0 : (g > 0xFF ? 0xFF : g);
    s32 cb = bl < 0 ? 0 : (bl > 0xFF ? 0xFF : bl);
    s32 ca = al < 0 ? 0 : (al > 0xFF ? 0xFF : al);
    u32 packed = (u32)ca | ((u32)cb << 8) | (((u32)cr << 24) | ((u32)cg << 16));
    GxBpWrite((packed >> 8) | 0xF2000000);
    if (lbl_805FB524 != 0) {
        fn_80151264();
    }
}

// 0x80156AC0
// Fills the 18x32 grid of vertex positions (x in -1..1, y from 1 down, z = 1) and colours (-1)
// when `fill` is non-zero. Returns the position buffer.
u8* fn_80156AC0(s32 fill) {
    if (fill != 0) {
        for (s32 row = 0; row < 0x12; row++) {
            f32 y = 1.0f + (-2.0f * (0.05882353f * (f32)row));
            for (s32 i = 0; i < 0x20; i++) {
                s32 n = row * 0x20 + i;
                f32* pos = (f32*)(lbl_80401C40 + n * 0xC);
                pos[0] = (2.0f * (0.032258064f * (f32)i)) - 1.0f;
                pos[1] = y;
                pos[2] = 1.0f;
                *(s32*)(lbl_80403740 + n * 4) = -1;
            }
        }
    }
    return lbl_80401C40;
}

// 0x80157494
// Reads the table of 16-bit pairs at lbl_8040ADD0+0x18 (size from fn_8014ACCC) and interleaves
// it into the back buffer lbl_805FB52C (4 rows of 0xC0 bytes per pass, 9 passes), then swaps the
// two buffers lbl_805FB528 / lbl_805FB52C.
void fn_80157494(void) {
    s32 sp10 = 0;
    s32 spC = 0;
    fn_8014ACCC(4, *(u8*)(lbl_8040ADD0 + 0x9), *(u16*)lbl_8040ADD0, *(u16*)(lbl_8040ADD0 + 0x2), &sp10, &spC, 0, 0);
    u8* src = *(u8**)(lbl_8040ADD0 + 0x18);
    DCInvalidateRange(src, sp10);
    u8* base = (u8*)lbl_805FB52C;
    for (s32 pass = 0; pass < 9; pass++) {
        u8* row0 = base;
        u8* row1 = base + 0xC0;
        u8* row2 = base + 0x180;
        u8* row3 = base + 0x240;
        for (s32 k = 0; k < 0xC; k++) {
            for (s32 j = 0; j < 4; j++) {
                *(u16*)(row0 + j * 4)     = *(u16*)(src + 0x00 + j * 2);
                *(u16*)(row0 + j * 4 + 2) = *(u16*)(src + 0x20 + j * 2);
                *(u16*)(row1 + j * 4)     = *(u16*)(src + 0x08 + j * 2);
                *(u16*)(row1 + j * 4 + 2) = *(u16*)(src + 0x28 + j * 2);
                *(u16*)(row2 + j * 4)     = *(u16*)(src + 0x10 + j * 2);
                *(u16*)(row2 + j * 4 + 2) = *(u16*)(src + 0x30 + j * 2);
                *(u16*)(row3 + j * 4)     = *(u16*)(src + 0x18 + j * 2);
                *(u16*)(row3 + j * 4 + 2) = *(u16*)(src + 0x38 + j * 2);
            }
            row0 += 0x10;
            row1 += 0x10;
            row2 += 0x10;
            row3 += 0x10;
            src += 0x40;
        }
        base += 0x300;
    }
    void* tmp = lbl_805FB528;
    lbl_805FB528 = lbl_805FB52C;
    lbl_805FB52C = tmp;
}

// 0x80162840
// Initialises a record: 64 entries (stride 0xD8 from +0xC0) with {j|0x40, j+1, 0} at +0/+8/+0x10,
// plus the tail fields, then runs fn_801601DC / fn_801608C8 on the two global tables.
void fn_80162840(u8* arg0) {
    for (s32 j = 0; j < 0x40; j++) {
        u8* e = arg0 + 0xC0 + j * 0xD8;
        *(s32*)(e + 0x0) = j | 0x40;
        *(s32*)(e + 0x8) = j + 1;
        *(s32*)(e + 0x10) = 0;
    }
    *(s32*)(arg0 + 0x35F0) = -1;
    *(s32*)(arg0 + 0x3600) = 0;
    memset(arg0 + 0x3618, 0, 0xAA0);
    *(s32*)(arg0 + 0x40B8) = 0;
    *(f32*)(arg0 + 0x40C0) = 0.0f;
    *(s32*)(arg0 + 0x40BC) = 1;
    *(s32*)(arg0 + 0x3614) = 0;
    *(s32*)(arg0 + 0x3604) = 0;
    *(s32*)(arg0 + 0x3608) = 0;
    *(s32*)(arg0 + 0x360C) = 0;
    *(s32*)(arg0 + 0x3610) = 0;
    fn_801601DC(lbl_8040D500);
    fn_801608C8(lbl_8040E260);
}

// 0x80169AA8
// Releases every entry of the four resource lists (+0x28, +0x10, +0x4, +0x1C) that is owned by
// arg0: unlinks it and drops its refcount, freeing it when the count reaches zero.
void fn_80169AA8(u8* arg0) {
    u8* node = *(u8**)(arg0 + 0x28);
    while (node != NULL) {
        u8* item = node - 0x4;
        node = *(u8**)node;
        if (*(s32*)(item + 0x10) == 1) {
            u8* o = *(u8**)(item + 0xC);
            u8* owner = (o != NULL) ? o - 0x28 : NULL;
            if (owner == arg0) {
                fn_801731C8(item + 0x4);
                s32 rc = *(s32*)(item + 0x10) - 1;
                *(s32*)(item + 0x10) = rc;
                if (rc == 0) {
                    fn_801732B0(item);
                }
            }
        }
    }
    node = *(u8**)(arg0 + 0x10);
    while (node != NULL) {
        u8* item = node - 0x10;
        node = *(u8**)node;
        if (*(s32*)(item + 0x4) == 1) {
            u8* o = *(u8**)(item + 0x18);
            u8* owner = (o != NULL) ? o - 0x10 : NULL;
            if (owner == arg0) {
                fn_801731C8(item + 0x10);
                fn_80174C2C(item);
            }
        }
    }
    node = *(u8**)(arg0 + 0x4);
    while (node != NULL) {
        u8* item = node - 0x28;
        node = *(u8**)node;
        if (*(s32*)(item + 0x24) == 1) {
            u8* o = *(u8**)(item + 0x30);
            u8* owner = (o != NULL) ? o - 0x4 : NULL;
            if (owner == arg0) {
                fn_801731C8(item + 0x28);
                s32 rc = *(s32*)(item + 0x24) - 1;
                *(s32*)(item + 0x24) = rc;
                if (rc == 0) {
                    TextureRelease(item);
                }
            }
        }
    }
    node = *(u8**)(arg0 + 0x1C);
    while (node != NULL) {
        u8* item = node - 0x4;
        node = *(u8**)node;
        if (*(s32*)(item + 0x10) == 1) {
            u8* o = *(u8**)(item + 0xC);
            u8* owner = (o != NULL) ? o - 0x1C : NULL;
            if (owner == arg0) {
                fn_801731C8(item + 0x4);
                s32 rc = *(s32*)(item + 0x10) - 1;
                *(s32*)(item + 0x10) = rc;
                if (rc == 0) {
                    fn_8016A2B8(item);
                }
            }
        }
    }
}

// 0x8016A14C
// Allocates (0x34 bytes, align 0x10, via the allocator object's function at +0) and clears two
// list-head records (tag 0x42) into lbl_805FB5AC / lbl_805FB5B0; the texture manager pointer
// lbl_805FB5A8 ends up pointing at lbl_805FB5AC.
void fn_8016A14C(void) {
    typedef u8* (*AllocFn)(s32, s32, s32, s32, s32);
    lbl_805FB5A8 = NULL;
    u8* t = (*(AllocFn*)lbl_805FB608)(0x34, 0x10, 0, 0, 0);
    for (s32 o = 0x4; o <= 0x30; o += 4) {
        *(s32*)(t + o) = 0;
    }
    *(s32*)t = 0x42;
    lbl_805FB5AC = t;
    if (lbl_805FB5B0 == NULL) {
        u8* t2 = (*(AllocFn*)lbl_805FB608)(0x34, 0x10, 0, 0, 0);
        for (s32 o = 0x4; o <= 0x30; o += 4) {
            *(s32*)(t2 + o) = 0;
        }
        *(s32*)t2 = 0x42;
        lbl_805FB5B0 = t2;
    }
    lbl_805FB5A8 = (u8*)lbl_805FB5AC;
}

// 0x8016D8C4
// Formats the 17 frame-stat counters (lbl_80412A2C) with their format strings (lbl_80376A10,
// variable stride), setting up lbl_805FB628/62C/630 before each print.
void fn_8016D8C4(void) {
    static const u16 kOff[17] = {
        0x00, 0x14, 0x28, 0x3C, 0x50, 0x64, 0x78, 0x90, 0xA8,
        0xC0, 0xD8, 0xF0, 0x108, 0x11C, 0x130, 0x144, 0x158,
    };
    s32 step = lbl_805FB63C;
    lbl_805FB630 = -1;
    for (s32 k = 0; k < 17; k++) {
        lbl_805FB628 = 0;
        lbl_805FB62C = step * (k + 1);
        fn_80188F00(lbl_80376A10 + kOff[k], lbl_80412A2C[k]);
    }
}

// 0x8016DFF0
// Frame-stat/lighting init: creates the "defaultLight" object and sets its fields, clears the
// two stats blocks at lbl_80412998 (+0x00 and +0x50), then twice rolls the frame-time ring
// (delta of the 64-bit timer, clamped to 0x7FFFFFFF) and shifts the stats block at +0x50 into
// +0x94, taking the two counters lbl_805FB4B0/B4.
// (The original also fills unused stack locals with the same default values; omitted.)
void fn_8016DFF0(void) {
    fn_80188D24();
    lbl_805FB5DC = (u8*)fn_80172F70(0, "defaultLight");   // string at 0x80376BC4
    *(f32*)(lbl_805FB5DC + 0x20) = 0.0f;
    *(f32*)(lbl_805FB5DC + 0x24) = 0.0f;
    *(f32*)(lbl_805FB5DC + 0x28) = 0.0f;
    *(f32*)(lbl_805FB5DC + 0x38) = -0.0f;
    *(f32*)(lbl_805FB5DC + 0x3C) = -0.0f;
    *(f32*)(lbl_805FB5DC + 0x40) = -1.0f;
    *(f32*)(lbl_805FB5DC + 0x44) = 0.5f;
    *(f32*)(lbl_805FB5DC + 0x48) = 0.5f;
    *(f32*)(lbl_805FB5DC + 0x4C) = 0.5f;
    *(f32*)(lbl_805FB5DC + 0x50) = 1.0f;
    fn_80172E84(lbl_805FB5DC);

    *(f32*)(lbl_80412998 + 0x4C) = 0.0f;
    u8* cur = lbl_80412998 + 0x50;
    u8* prev = lbl_80412998 + 0x94;
    *(f32*)(lbl_80412998 + 0x00) = 0.0f;
    *(f32*)(lbl_80412998 + 0x04) = 0.0f;
    *(f32*)(lbl_80412998 + 0x08) = 0.0f;
    *(f32*)(lbl_80412998 + 0x0C) = 0.0f;
    *(f32*)(lbl_80412998 + 0x10) = 0.4f;
    *(f32*)(lbl_80412998 + 0x14) = 0.4f;
    *(f32*)(lbl_80412998 + 0x18) = 0.4f;
    *(f32*)(lbl_80412998 + 0x1C) = 1.0f;
    *(f32*)(lbl_80412998 + 0x20) = 0.0f;
    *(f32*)(lbl_80412998 + 0x24) = 0.0f;
    *(f32*)(lbl_80412998 + 0x28) = 0.0f;
    *(f32*)(lbl_80412998 + 0x2C) = 1.0f;
    *(f32*)(lbl_80412998 + 0x30) = 0.0f;
    *(f32*)(lbl_80412998 + 0x34) = 0.0f;
    *(f32*)(lbl_80412998 + 0x38) = 0.0f;
    *(f32*)(lbl_80412998 + 0x3C) = 0.0f;
    *(f32*)(lbl_80412998 + 0x40) = 0.0f;
    *(f32*)(lbl_80412998 + 0x44) = 0.0f;
    *(f32*)(lbl_80412998 + 0x48) = 0.0f;
    *(f32*)(lbl_80412998 + 0x4C) = 10.0f;
    for (s32 n = 0; n < 2; n++) {
        u64 t = fn_801425DC(lbl_805FB5F0);
        u32 lo = (u32)t;
        u32 hi = (u32)(t >> 32);
        u64 d = t - (((u64)lbl_805FB5E8 << 32) | lbl_805FB5EC);
        s32 delta = (s32)(u32)d;
        if (d > 0x7FFFFFFFull) {
            delta = 0x7FFFFFFF;
        }
        lbl_805FB5EC = lo;
        s32 idx = lbl_805FB5E0 + 1;
        lbl_805FB5E8 = hi;
        lbl_805FB5E0 = idx;
        if (idx >= 0x20) {
            idx = 0;
            lbl_805FB5E0 = 0;
        }
        *(s32*)(lbl_80412998 + 0xD8 + idx * 4) = delta;
        if (lbl_805FB5E4 < 0x20) {
            lbl_805FB5E4 += 1;
        }
        // Shift the current block (+0x50) into the previous block (+0x94); words +0x38/+0x40 are
        // replaced by the two global counters (the original copies them first, then overwrites).
        *(s32*)(prev + 0x00) = *(s32*)(cur + 0x00);
        for (s32 o = 0x4; o <= 0x34; o += 4) {
            *(s32*)(prev + o) = *(s32*)(cur + o);
        }
        *(s32*)(prev + 0x3C) = *(s32*)(cur + 0x3C);
        *(s32*)(prev + 0x38) = lbl_805FB4B0;
        *(s32*)(prev + 0x40) = lbl_805FB4B4;
        lbl_805FB4B0 = 0;
        lbl_805FB4B4 = 0;
        memset(cur, 0, 0x44);
    }
}

// 0x8016F854
// Projects the point arg2 through the 3x4 plane rows at arg1+0xC0.. (six rows of four floats:
// 0xC0, 0xD0, 0xE0, 0xF0, 0x100, 0x110) and writes three perspective-style ratios
// t = n / (n + m) for row pairs (D,F), (B,A), (E,C) to out[1], out[2], out[0].
void fn_8016F854(f32* out, u8* arg1, const f32* arg2) {
    f32 x = arg2[0];
    f32 y = arg2[1];
    f32 z = arg2[2];
    f32 tB = (z * *(f32*)(arg1 + 0xD8) + (x * *(f32*)(arg1 + 0xD0) + y * *(f32*)(arg1 + 0xD4))) - *(f32*)(arg1 + 0xDC);
    f32 tA = (z * *(f32*)(arg1 + 0xC8) + (x * *(f32*)(arg1 + 0xC0) + y * *(f32*)(arg1 + 0xC4))) - *(f32*)(arg1 + 0xCC);
    f32 tC = (z * *(f32*)(arg1 + 0xE8) + (x * *(f32*)(arg1 + 0xE0) + y * *(f32*)(arg1 + 0xE4))) - *(f32*)(arg1 + 0xEC);
    f32 tD = (z * *(f32*)(arg1 + 0xF8) + (x * *(f32*)(arg1 + 0xF0) + y * *(f32*)(arg1 + 0xF4))) - *(f32*)(arg1 + 0xFC);
    f32 tE = (z * *(f32*)(arg1 + 0x108) + (x * *(f32*)(arg1 + 0x100) + y * *(f32*)(arg1 + 0x104))) - *(f32*)(arg1 + 0x10C);
    f32 tF = (z * *(f32*)(arg1 + 0x118) + (x * *(f32*)(arg1 + 0x110) + y * *(f32*)(arg1 + 0x114))) - *(f32*)(arg1 + 0x11C);
    out[0] = tE * (1.0f / (tC + tE));
    out[1] = tD * (1.0f / (tD + tF));
    out[2] = tB * (1.0f / (tA + tB));
}

}  // extern "C"
