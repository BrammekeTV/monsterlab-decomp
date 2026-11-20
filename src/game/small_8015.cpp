// Small functions in 0x8015xxxx.
#include "types.h"
#include "revolution.h"

extern "C" {
void     fn_8014DE68(void);                                                        // 0x8014DE68
void     fn_80152D84(u32 a, u32 b, s32 c, u32 d, u8* e, u32 f, u32 g, u32 h);      // 0x80152D84
s32      fn_8015BA30(void* p);                                                     // 0x8015BA30
void     fn_8015AB70(u8* p);                                                       // 0x8015AB70
void     fn_8015BBC0(s32 a);                                                       // 0x8015BBC0
void     fn_8015EA40(void* p);                                                     // 0x8015EA40
GXFifoObj* GXInit(void* base, u32 size);                                           // 0x802488E8

extern u8  lbl_805F9231;            // 0x805F9231 (GX display-state flag)
extern u8  lbl_805F9232;            // 0x805F9232 (GX display-state value)
extern u8  sRendDLs[];              // 0x80401108 (display-list records, 0x94 bytes each)
extern u8  sRendThread[];           // 0x8040AEA8 (render-thread request queue)
extern s32 gViewportIndex;          // 0x805FB4BC
extern s32 gFrameBudget;            // 0x805FB4C8
extern s32 lbl_805FB544;            // 0x805FB544 (dispatch table count)
extern u8  lbl_805FB548[];          // 0x805FB548 (dispatch table keys)
extern u8  lbl_805FB540;            // 0x805FB540 (GX FIFO active flag)
extern s32 lbl_805FB560;            // 0x805FB560 (render request index)
extern s32 lbl_805FB594;            // 0x805FB594 (GX init state block)
extern void* lbl_805FB534;          // 0x805FB534 (GX init arena pointer)
extern s32 lbl_8040ADF0[8];         // 0x8040ADF0
extern s32 lbl_8040AE10[8];         // 0x8040AE10
extern u8  lbl_8040AE88[];          // 0x8040AE88 (dispatch table handlers)
extern OSSemaphore gVideoGXLock;    // 0x8040AE3C
extern GXFifoObj* gVideoMainFifo;   // 0x805FB530
}

// Writes one BP register command (opcode 0x61) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// 0x80154294
// One-shot: when the flag is clear, sets it and sends two BP writes (the second carries lbl_805F9232).
extern "C" void fn_80154294(void) {
    if (lbl_805F9231 != 0) return;
    lbl_805F9231 = 1;
    u32 v = lbl_805F9232;
    GxBpWrite(0xFE000011);
    GxBpWrite(0x40000001 | (v << 4));
}

// 0x801542DC
// Clears the flag and, if it was set, sends two BP writes that reset the display state.
extern "C" void fn_801542DC(void) {
    if (lbl_805F9231 == 0) return;
    lbl_805F9231 = 0;
    GxBpWrite(0xFE000011);
    GxBpWrite(0x40000000);
}

extern "C" void fn_80154318(void);
// 0x80154318
// Sets lbl_805F9232 to 1; if the flag is set, sends two BP writes.
extern "C" void fn_80154318(void) {
    u8 flag = lbl_805F9231;
    lbl_805F9232 = 1;
    if (flag == 0) return;
    GxBpWrite(0xFE000011);
    GxBpWrite(0x40000011);
}

// 0x80154358
// Sets lbl_805F9232 to 0; if the flag is set, sends two BP writes.
void fn_80154358(void) {
    u8 flag = lbl_805F9231;
    lbl_805F9232 = 0;
    if (flag == 0) return;
    GxBpWrite(0xFE000011);
    GxBpWrite(0x40000001);
}

// 0x8015490C
// Streams a 4x3 float matrix (rows of three floats, 0xC bytes apart) to the GX pipe, column by column.
void fn_8015490C(u8* p) {
    for (s32 c = 0; c < 3; c++) {
        for (s32 r = 0; r < 4; r++) {
            GXWGFifo._f32 = *(f32*)(p + r * 0xc + c * 4);
        }
    }
}

extern "C" {
// 0x8015846C
// Sends one packed BP register word built from ten fields (a3 is unused).
void fn_8015846C(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u8 a11, u32 a12, u32 a13) {
    (void)a3;
    u32 v = ((a4 + 0x80) << 24) | (a12 << 19) | (a13 << 21) | (a8 << 5) | (a7 << 7) |
            (a10 << 8) | (a11 << 9) | a5 | (a6 << 2) | (a9 << 4);
    GxBpWrite(v);
}

// 0x801584E8
// Sends two BP writes with the 16-bit fields at +4 and +6 of p, each minus one.
void fn_801584E8(u8* p, u32 idx) {
    u32 r = idx << 1;
    GxBpWrite(((r + 0x30) << 24) | ((u32)*(u16*)(p + 0x4) - 1));
    GxBpWrite(((r + 0x31) << 24) | ((u32)*(u16*)(p + 0x6) - 1));
}

// 0x80158870
// Calls fn_80152D84 for 17 consecutive 0x300-byte blocks starting at p.
// r5-r7 are not forwarded (r5 and r7 are set per call, r6 is always 0x40); r8-r10 pass through.
void fn_80158870(u8* p, s32 a4, s32 unused5, s32 unused6, s32 unused7, s32 a8, s32 a9, s32 a10) {
    (void)unused5; (void)unused6; (void)unused7;
    for (s32 i = 0; i < 0x11; i++) {
        fn_80152D84(0x98, 0x40, a4, 0x40, p + i * 0x300, a8, a9, a10);
    }
}

// 0x80158BC4
// Checks the render budget for display-list record gViewportIndex; if over, flushes and runs the
// budget-exceeded path.
void fn_80158BC4(s32 extra) {
    s32 used = fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + extra;
    if (used + 0x10C0 > gFrameBudget) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + gViewportIndex * 0x94);
    }
}

// 0x80158C34
// Sends one packed BP register word built from ten fields (r3 is unused).
void fn_80158C34(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u32 a11, u32 a12) {
    (void)a3;
    u32 v = ((a4 + 0x28) << 24) | (a11 << 18) | (a12 << 19) | (a7 << 6) | (a8 << 7) |
            (a9 << 15) | (a10 << 12) | a6 | (a5 << 3);
    GxBpWrite(v);
}

// 0x80158E88
// Sends one packed BP register word built from eleven fields (r3 is unused).
void fn_80158E88(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u32 a11, u32 a12, u32 a13) {
    (void)a3;
    u32 v = a5 | (a6 << 2) | (a7 << 4) | ((a4 + 0x10) << 24) | (a11 << 20) | (a12 << 19) |
            (a13 << 7) | (a8 << 9) | (a9 << 13) | (a10 << 16);
    GxBpWrite(v);
}

// 0x80158FBC
// Sends one packed BP register word built from five fields (r3 is unused).
void fn_80158FBC(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8) {
    (void)a3;
    u32 v = ((a4 + 0x25) << 24) | (a7 << 8) | (a8 << 12) | a5 | (a6 << 4);
    GxBpWrite(v);
}

// 0x80158FF4
// Sends one packed BP register word built from nine fields plus a fixed register address (r3 unused).
void fn_80158FF4(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11) {
    (void)a3;
    u32 v = a4 | 0x27000000 | (a11 << 21) | (a9 << 15) | (a10 << 18) |
            (a5 << 3) | (a6 << 6) | (a7 << 9) | (a8 << 12);
    GxBpWrite(v);
}
} // extern "C"

// 0x8015A2E8
// Clears bit 0x2 of the flags byte at +0xB, calls fn_8015AB70, then flushes the cache range
// (+0x18, size +0x14) when bit 0 of flush is set.
void fn_8015A2E8(u8* p, u32 unused4, u32 flush) {
    (void)unused4;
    p[0xb] &= ~0x2;
    fn_8015AB70(p);
    if (flush & 1) {
        DCFlushRange(*(void**)(p + 0x18), *(u32*)(p + 0x14));
    }
}

// 0x8015A368
// Clears bit 0x4 of the flags byte at +0xB; if bit 0 of flush is set and the count at +0xD is
// non-zero, flushes count * stride(+0xE) bytes starting at +0x1C.
void fn_8015A368(u8* p, u32 flush) {
    p[0xb] &= ~0x4;
    if (!(flush & 1)) return;
    u32 count = p[0xd];
    if (count == 0) return;
    DCFlushRange(*(void**)(p + 0x1c), count * p[0xe]);
}

// 0x8015B7D4
// Zeroes the two 8-word tables lbl_8040ADF0 and lbl_8040AE10.
extern "C" void fn_8015B7D4(void) {
    for (s32 i = 0; i < 8; i++) {
        lbl_8040ADF0[i] = 0;
        lbl_8040AE10[i] = 0;
    }
}

// 0x8015D23C
// Dispatches arg: the byte at bits 8..15 selects every entry of the table whose key matches, and
// the handler is called with the low byte.
typedef void (*DispatchFn)(u32);
void fn_8015D23C(u32 arg) {
    u32 key = (arg >> 8) & 0xff;
    u32 param = arg & 0xff;
    for (s32 i = 0; i < lbl_805FB544; i++) {
        if (lbl_805FB548[i] == key) {
            (*(DispatchFn*)(lbl_8040AE88 + i * 4))(param);
        }
    }
}

// 0x8015D530
// Initialises the video GX state: runs fn_8015EA40 on lbl_805FB594, takes gVideoGXLock, marks the
// FIFO active and calls GXInit with a 64 KB buffer.
void fn_8015D530(void) {
    fn_8015EA40((void*)&lbl_805FB594);
    OSWaitSemaphore(&gVideoGXLock);
    lbl_805FB540 = 1;
    gVideoMainFifo = GXInit(lbl_805FB534, 0x10000);
}

// 0x8015DDEC
// Releases gVideoGXLock, clears the FIFO active flag and calls fn_8015BBC0(0).
void fn_8015DDEC(void) {
    OSSignalSemaphore(&gVideoGXLock);
    lbl_805FB540 = 0;
    fn_8015BBC0(0);
}

// 0x8015E654
// Queues one render request: under the render semaphore, writes (1, a7, a4) into the next record
// of the 0x40-entry ring at sRendThread+0x2318, then signals.
void fn_8015E654(u32 r3, u32 a4, u32 r5, u32 r6, u32 a7) {
    (void)r3; (void)r5; (void)r6;
    OSWaitSemaphore((OSSemaphore*)(sRendThread + 0x2618));
    s32 idx = lbl_805FB560;
    u8* rec = sRendThread + 0x2318 + idx * 0xc;
    *(u32*)(rec + 0x0) = 1;
    *(u32*)(rec + 0x4) = a7;
    *(u32*)(rec + 0x8) = a4;
    s32 next = idx + 1;
    lbl_805FB560 = next;
    if (next >= 0x40) lbl_805FB560 = 0;
    OSSignalSemaphore((OSSemaphore*)(sRendThread + 0x2624));
}
