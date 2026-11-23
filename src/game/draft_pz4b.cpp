// Misc functions: pad/unit vector setup, video frame present, GX BP setup, audio alarm callback, text wrap.
#include <math.h>
#include <string.h>
#include "types.h"
#include "revolution.h"   // OSSemaphore, GXWGFifo, GXGetGPStatus, VIFlush, VISetNextFrameBuffer

extern "C++" void* gScreenSets;     // 0x805FB654
extern OSSemaphore gVideoDrawDone;  // 0x8040AE30

extern "C" {
s32   OSDisableInterrupts(void);                                    // 0x802393C4
s32   OSRestoreInterrupts(s32 level);                               // 0x802393EC
s32   OSWaitSemaphore(OSSemaphore* sem);
u8    SCGetAspectRatio(void);                                       // 0x8026A9D8: 1 = 16:9
void  fn_8015E74C(void* unusedR3, s32 frame, s32 unusedR5, s32 unusedR6, s32 value);   // 0x8015E74C
u8*   fn_8018C9DC(void* sets, s32 idx);                             // 0x8018C9DC
s32   fn_801ABD84(u8* rec, s32 key);                                // 0x801ABD84
void  fn_8019F4C8(u8* el);                                          // 0x8019F4C8
s32   fn_801FB254(u32 type, f32* out);                              // 0x801FB254
s32   fn_80244950(void);                                            // 0x80244950
void  PSMTXIdentity(f32* mtx);                                        // 0x80248050
void  C_MTXOrtho(f32* mtx, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);   // 0x8024849C
void  GXSetVtxDesc(s32 a, s32 b);                                    // 0x8024A3A4
void  GXClearVtxDesc(void);                                            // 0x8024A7E4
void  GXSetVtxAttrFmt(s32 a, s32 b, s32 c, s32 d, s32 e);              // 0x8024A818
void  GXSetNumTexGens(s32 a);                                           // 0x8024AE74
s32   fn_8024B358(void);                                            // 0x8024B358
void  GXSetCullMode(s32 a);                                           // 0x8024BB64
void  GXCopyDisp(void* fb, s32 clear);                             // 0x8024BFCC
void  GXSetNumChans(s32 a);                                           // 0x8024C2F0
void  GXSetTevOp(s32 a, s32 b);                                    // 0x8024D508
void  GXSetTevOrder(s32 a, s32 b, s32 c, s32 d);                      // 0x8024DA08
void  GXSetNumTevStages(s32 a);                                           // 0x8024DB64
void  GXSetBlendMode(s32 a, s32 b, s32 c, s32 d);                      // 0x8024DED8
void  GXSetColorUpdate(s32 a);                                           // 0x8024DF28
void  GXSetZMode(s32 a, s32 b, s32 c);                             // 0x8024DF80
void  GXSetProjection(f32* mtx, s32 a);                                 // 0x8024E1D0
void  GXLoadPosMtxImm(f32* mtx, s32 a);                                 // 0x8024E280
void  GXSetCurrentMtx(s32 a);                                           // 0x8024E328
s32   AXRmtGetSamples(s32 a, u8* buf, s32 len);                         // 0x80259B70
s32   AXRmtAdvancePtr(s32 a);                                           // 0x80259C34
void  AXFXReverbHiExpInit(u8* p);                                           // 0x8025B4C0
void  fn_8025C36C(u8* p);                                           // 0x8025C36C
void  fn_8025D2E0(u8* p);                                           // 0x8025D2E0
void  fn_8025DC3C(u8* p);                                           // 0x8025DC3C
void  fn_8025E3E4(u8* p);                                           // 0x8025E3E4
void  fn_8025EE54(u8* p);                                           // 0x8025EE54
void  WENCGetEncodeData(u8* dst, s32 a, u8* src, s32 len, u8* out);       // 0x8026B174
s32   WPADProbe(s32 chan, u32* out);                              // 0x802728A4
s32   WPADControlSpeaker(s32 a, s32 b, s32 c);                             // 0x802737A8
s32   WPADCanSendStreamData(s32 a);                                           // 0x80274F80
s32   WPADSendStreamData(s32 a, u8* buf, s32 len);                         // 0x80275004
s32   KPADRead(s32 chan, u8* buf, s32 len);                      // 0x8027F504

extern u8   lbl_804FB8A0[];       // 0x804FB8A0 (4 pad records of 0x10: buf ptr, x, y, status)
extern u8   lbl_804FB8E0[];       // 0x804FB8E0 (4 pad buffers of 0x840)
extern u8   lbl_804FDAA0[];       // 0x804FDAA0
extern u8   lbl_8051E630[];       // 0x8051E630 (0x28-byte buffer)
extern u8   lbl_8051E680[];       // 0x8051E680 (0x28-byte records)
extern s32  lbl_805FB518;         // 0x805FB518 (frame counter)
extern void* lbl_805FB538[];      // 0x805FB538 (frame buffer pointers)
extern u8   lbl_805FB594[];       // 0x805FB594 (render thread state)
extern u8*  lbl_805FB608;         // 0x805FB608 (heap object, slot +0 = alloc)
extern u8   lbl_805FBA9C;         // 0x805FBA9C
extern s32  lbl_805FBABC;         // 0x805FBABC (count of unpaused looping voices)
extern s32  lbl_805FBAC0;         // 0x805FBAC0 (saved copy of lbl_805FBABC)
extern s32  lbl_805FBAE0;         // 0x805FBAE0 (audio busy flag)
}

typedef u8* (*HeapAllocFn)(s32 size, s32 a, s32 b, s32 c, s32 d);
typedef void (*TextMeasureFn)(void* font, u8* text, u32 start, u32 end, f32* w, f32* h, f32 scale);

extern "C" {

// 0x801F7240
// Normalizes the vector *v and stores it at +0x90 (zero vector stays zero).
void fn_801F7240(u8* p, f32* v) {
    f32 x = v[0];
    f32 y = v[1];
    f32 z = v[2];
    f32 len2 = z * z + (x * x + y * y);
    f32 ox, oy, oz;
    if (len2 == 0.0f) {
        ox = 0.0f;
        oy = 0.0f;
        oz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        ox = x * inv;
        oy = y * inv;
        oz = z * inv;
    }
    *(f32*)(p + 0x90) = ox;
    *(f32*)(p + 0x94) = oy;
    *(f32*)(p + 0x98) = oz;
}

// 0x8015D5A4
// Presents the next frame: runs the queued render command, waits for the GPU to go idle and
// flips the frame buffer.
void fn_8015D5A4(void) {
    void* fb = lbl_805FB538[lbl_805FB518 & 1];
    fn_8015E74C(lbl_805FB594, lbl_805FB518, 0, 0, 0);  // r5 = &lbl_805FB538 (unused); r6/r7 stale
    lbl_805FB518 += 1;
    GXSetZMode(1, 3, 1);
    GXSetColorUpdate(1);
    GXCopyDisp(fb, 1);
    fn_8024B358();
    OSWaitSemaphore(&gVideoDrawDone);
    u8 overhi, underlow, readIdle, cmdIdle, brkpt;
    do {
        GXGetGPStatus(&overhi, &underlow, &readIdle, &cmdIdle, &brkpt);
    } while (!readIdle || !cmdIdle);
    VISetNextFrameBuffer(fb);
    VIFlush();
    fn_80244950();
}

// 0x801AC024
// Finds the next 0x1C-byte record after `prev` in the table (base +0x10, count +0x14) whose first
// word equals `hash`, until fn_801ABD84(record, key) accepts one; NULL if none.
u8* fn_801AC024(u8* table, u32 hash, s32 key, u8* prev) {
    u8* e;
    {
        u8* base = *(u8**)(table + 0x10);
        s32 count = *(s32*)(table + 0x14);
        s32 i = (s32)((prev - base) / 0x1C) + 1;
        e = NULL;
        for (; i < count; i++) {
            if (hash == *(u32*)(base + i * 0x1C)) {
                e = base + i * 0x1C;
                break;
            }
        }
    }
    while (e != NULL) {
        if (fn_801ABD84(e, key) != 0) return e;
        u8* base = *(u8**)(table + 0x10);
        s32 count = *(s32*)(table + 0x14);
        s32 i = (s32)((e - base) / 0x1C) + 1;
        e = NULL;
        for (; i < count; i++) {
            if (hash == *(u32*)(base + i * 0x1C)) {
                e = base + i * 0x1C;
                break;
            }
        }
    }
    return NULL;
}

// 0x801FB10C
// Sets up the 2D screen state: GX/video init calls, a view matrix and an orthographic projection
// (wider for 16:9).
void fn_801FB10C(void) {
    f32 proj[16];
    f32 view[12];
    GXClearVtxDesc();
    GXSetVtxAttrFmt(4, 9, 0, 4, 0);
    GXSetVtxAttrFmt(4, 0xB, 0, 1, 0);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xB, 1);
    GXSetNumChans(1);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetTevOp(0, 4);
    GXSetBlendMode(0, 0, 0, 0);
    GXSetZMode(1, 3, 1);
    GXSetCurrentMtx(3);
    PSMTXIdentity(view);
    GXLoadPosMtxImm(view, 3);
    GXSetCullMode(0);
    if (SCGetAspectRatio() == 1) {
        C_MTXOrtho(proj, 228.0f, -228.0f, -416.0f, 416.0f, 0.0f, 500.0f);
    } else {
        C_MTXOrtho(proj, 228.0f, -228.0f, -304.0f, 304.0f, 0.0f, 500.0f);
    }
    GXSetProjection(proj, 1);
}

// 0x801736EC
// Allocates a 0x64-byte block holding six unit vectors (1,0,0,0 rows on the diagonal of a 6x4
// float table) and the word 0x56 at +0x60. Returns it.
u8* fn_801736EC(void) {
    u8* p = (*(HeapAllocFn*)lbl_805FB608)(0x64, 0x10, 0xF, 0, 0);
    for (s32 i = 0; i < 6; i++) {
        *(f32*)(p + i * 0x10 + 0x0) = 1.0f;
        *(f32*)(p + i * 0x10 + 0x4) = 0.0f;
        *(f32*)(p + i * 0x10 + 0x8) = 0.0f;
        *(f32*)(p + i * 0x10 + 0xC) = 0.0f;
    }
    *(s32*)(p + 0x60) = 0x56;
    return p;
}

// 0x80202CAC
// Audio alarm callback: re-syncs the saved voice count (pause/resume) under the busy flag,
// reads a 0x28-byte block and, when record 0 is active, advances its state machine.
void fn_80202CAC(void) {
    s32 did = 0;
    if (lbl_805FBABC != lbl_805FBAC0) {
        if (lbl_805FBABC == 0) {
            s32 saved = lbl_805FBAE0;
            lbl_805FBAE0 = 1;
            if (WPADControlSpeaker(0, 2, 0) == 0) lbl_805FBAC0 = lbl_805FBABC;
            lbl_805FBAE0 = saved;
        } else if (lbl_805FBABC >= 1 && lbl_805FBAC0 == 0) {
            s32 saved = lbl_805FBAE0;
            lbl_805FBAE0 = 1;
            if (WPADControlSpeaker(0, 3, 0) == 0) lbl_805FBAC0 = lbl_805FBABC;
            lbl_805FBAE0 = saved;
        }
    }
    if (AXRmtGetSamples(0, lbl_8051E630, 0x28) == 0x28) {
        did = 1;
        if (*(u8*)lbl_8051E680 != 0) {
            u8 sp8[0x14];
            s32 level = OSDisableInterrupts();
            if (WPADCanSendStreamData(0) != 0) {
                s32 st = *(s32*)(lbl_8051E680 + 0x24);
                if (st == 3 || st == 4) {
                    s32 flag = 1;
                    if (st == 3) {
                        *(s32*)(lbl_8051E680 + 0x24) = 4;
                        flag = 0;
                    }
                    WENCGetEncodeData(lbl_8051E680 + 1, flag, lbl_8051E630, 0x28, sp8);
                    WPADSendStreamData(0, sp8, 0x14);
                }
            }
            OSRestoreInterrupts(level);
        }
    }
    if (did != 0) AXRmtAdvancePtr(0x28);
}

// 0x80203E8C
// Fills the default parameter blocks of two effect objects (at +0x206D0.. and +0x20910..) and
// initialises them through two different init paths depending on lbl_805FBA9C == 2.
void fn_80203E8C(void) {
    u8* base = lbl_804FDAA0 + 0x20000;
    u8* a = base + 0x6D0;
    u8* b = base + 0x910;
    u8* c = base + 0x728;
    u8* d = base + 0x968;
    u8* e = base + 0x870;
    u8* f = base + 0xAB0;

    *(f32*)(a + 0x3C) = 80.0f;
    *(f32*)(a + 0x38) = 100.0f;
    *(f32*)(a + 0x40) = 0.6f;
    *(f32*)(a + 0x44) = 1.0f;
    *(s32*)(a + 0x48) = 0;
    *(s32*)(a + 0x4C) = 0;
    *(f32*)(a + 0x50) = 1.0f;
    *(f32*)(a + 0x54) = 0.0f;
    *(f32*)(e + 0x80) = 30.0f;
    *(f32*)(e + 0x84) = 0.3f;
    *(f32*)(e + 0x88) = 0.5f;
    *(f32*)(e + 0x8C) = 0.1f;
    *(s32*)(e + 0x90) = 0;
    *(s32*)(e + 0x94) = 0;
    *(f32*)(e + 0x98) = 1.0f;
    *(f32*)(e + 0x9C) = 0.0f;
    *(s32*)(c + 0x110) = 5;
    *(f32*)(c + 0x114) = 0.05f;
    *(f32*)(c + 0x118) = 0.02f;
    *(s32*)(c + 0x11C) = 2;
    *(f32*)(c + 0x120) = 1.0f;
    *(f32*)(c + 0x124) = 0.6f;
    *(f32*)(c + 0x128) = 0.4f;
    *(f32*)(c + 0x12C) = 0.1f;
    *(f32*)(c + 0x130) = 0.3f;
    *(f32*)(c + 0x134) = 0.3f;
    *(s32*)(c + 0x138) = 0;
    *(s32*)(c + 0x13C) = 0;
    *(f32*)(c + 0x140) = 1.0f;
    *(f32*)(c + 0x144) = 0.0f;
    *(f32*)(b + 0x3C) = 80.0f;
    *(f32*)(b + 0x38) = 100.0f;
    *(f32*)(b + 0x40) = 0.6f;
    *(f32*)(b + 0x44) = 1.0f;
    *(s32*)(b + 0x48) = 0;
    *(s32*)(b + 0x4C) = 0;
    *(f32*)(b + 0x50) = 1.0f;
    *(f32*)(b + 0x54) = 0.0f;
    *(f32*)(f + 0x80) = 30.0f;
    *(f32*)(f + 0x84) = 0.3f;
    *(f32*)(f + 0x88) = 0.5f;
    *(f32*)(f + 0x8C) = 0.1f;
    *(s32*)(f + 0x90) = 0;
    *(s32*)(f + 0x94) = 0;
    *(f32*)(f + 0x98) = 1.0f;
    *(f32*)(f + 0x9C) = 0.0f;
    *(s32*)(d + 0x110) = 5;
    *(f32*)(d + 0x114) = 0.05f;
    *(f32*)(d + 0x118) = 0.02f;
    *(s32*)(d + 0x11C) = 2;
    *(f32*)(d + 0x120) = 1.0f;
    *(f32*)(d + 0x124) = 0.6f;
    *(f32*)(d + 0x128) = 0.4f;
    *(f32*)(d + 0x12C) = 0.1f;
    *(f32*)(d + 0x130) = 0.3f;
    *(f32*)(d + 0x134) = 0.3f;
    *(s32*)(d + 0x138) = 0;
    *(s32*)(d + 0x13C) = 0;
    *(f32*)(d + 0x140) = 1.0f;
    *(f32*)(d + 0x144) = 0.0f;
    if (lbl_805FBA9C == 2) {
        fn_8025DC3C(base + 0x1B0);
        fn_8025EE54(base + 0x388);
        fn_8025C36C(base + 0x210);
        fn_8025DC3C(base + 0x440);
        fn_8025EE54(base + 0x618);
        fn_8025C36C(base + 0x4A0);
    } else {
        fn_8025D2E0(a);  // r3 left over from the first block address
        fn_8025E3E4(e);
        AXFXReverbHiExpInit(c);
        fn_8025D2E0(b);
        fn_8025E3E4(f);
        AXFXReverbHiExpInit(d);
    }
    *(s32*)(base + 0xB50) = 0;
    *(s32*)(base + 0xB54) = 0;
    *(s32*)(base + 0xB58) = 0;
}

// 0x801FB3F8
// Polls the four pad channels: reads status and the 0x840-byte pad buffer, and for connected
// pads (status 0, -2..-4 or -7) updates the record's stick position (+4/+8, clamped to +-1);
// if nothing moved and the pad reports a positive value at +0x5E, takes the position at +0x20/+0x24.
void fn_801FB3F8(void) {
    u32 sp8[4];
    s32 sp28[4];
    for (s32 i = 0; i < 4; i++) {
        u8* rec = lbl_804FB8A0 + i * 0x10;
        u8* buf = lbl_804FB8E0 + i * 0x840;
        sp28[i] = WPADProbe(i, &sp8[i]);
        *(u32*)(rec + 0xC) = sp8[i];
        s32 st = sp28[i];
        KPADRead(i, buf, 0x10);
        if (st == 0 || st == -7 || (st >= -4 && st <= -2)) {
            *(u8**)(rec + 0x0) = buf;
            memset(buf + 0x60, 0, 0x24);
            s32 r = fn_801FB254(*(u32*)(*(u8**)(rec + 0x0) + 0x60), (f32*)(rec + 4));
            u8* b = *(u8**)(rec + 0x0);
            f32 dx = *(f32*)(b + 0x6C) / 28.444445f;
            f32 dy = *(f32*)(b + 0x70) / 28.444445f;
            if (dx > 1.0f) dx = 1.0f;
            else if (dx < -1.0f) dx = -1.0f;
            if (dy > 1.0f) dy = 1.0f;
            else if (dy < -1.0f) dy = -1.0f;
            s32 moved;
            if (dx == 0.0f && dy == 0.0f) {
                moved = 0;
            } else {
                f32 nx = *(f32*)(rec + 0x4) + dx;
                if (nx > 1.0f) nx = 1.0f;
                else if (nx < -1.0f) nx = -1.0f;
                *(f32*)(rec + 0x4) = nx;
                f32 ny = *(f32*)(rec + 0x8) - dy;
                if (ny > 1.0f) ny = 1.0f;
                else if (ny < -1.0f) ny = -1.0f;
                *(f32*)(rec + 0x8) = ny;
                moved = 1;
            }
            if ((r | moved) == 0) {
                u8* b2 = *(u8**)(rec + 0x0);
                if (*(s8*)(b2 + 0x5E) > 0) {
                    *(f32*)(rec + 0x4) = *(f32*)(b2 + 0x20);
                    *(f32*)(rec + 0x8) = *(f32*)(*(u8**)(rec + 0x0) + 0x24);
                }
            }
        } else {
            *(u8**)(rec + 0x0) = NULL;
        }
    }
}

// 0x8019F5A8
// Word-wraps the text at +0x170 of a text element in place: for each line (split at '\n') whose
// measured width (times the scale at +0x74) exceeds the limit at +0x160, replaces the last
// space that fits by a newline. Returns 0.
s32 fn_8019F5A8(u8* el) {
    u8* cur = *(u8**)(el + 0x170);
    f32 w, h;
    while (cur != NULL && *(s8*)cur != 0) {
        u8* text = *(u8**)(el + 0x170);
        u32 start = (u32)(cur - text);
        u32 end = start + (u32)strlen((char*)cur);
        cur = (u8*)strchr((char*)cur, 0xA);
        if (cur != NULL) {
            end = (u32)(cur - text) - 1;
            while (*cur == 0xA) cur++;
        }
        u8* font = fn_8018C9DC(gScreenSets, *(s32*)(el + 0x15C));
        (*(TextMeasureFn*)(*(u8**)font + 0x10))(font, *(u8**)(el + 0x170), start, end, &w, &h, 1.0f);
        f32 scale = *(f32*)(el + 0x74);
        w = (f32)fabs(w * scale);
        while (w > (f32)*(s32*)(el + 0x160)) {
            u32 i = end - 1;
            for (;;) {
                if (*(s8*)(*(u8**)(el + 0x170) + i) == 0x20) {
                    font = fn_8018C9DC(gScreenSets, *(s32*)(el + 0x15C));
                    (*(TextMeasureFn*)(*(u8**)font + 0x10))(font, *(u8**)(el + 0x170), start, i, &w, &h, 1.0f);
                    w = (f32)fabs(w * scale);
                    f32 lim = (f32)*(s32*)(el + 0x160);
                    if (w > lim) {
                        i--;
                        if (i > start) continue;
                    }
                    break;
                }
                i--;
                if (!(i > start)) return 0;
            }
            // break the line at the space found
            *(*(u8**)(el + 0x170) + i) = 0xA;
            u8* sp = *(u8**)(el + 0x170) + i;
            if (sp[1] == 0x20) sp[1] = 9;
            start = i;
            font = fn_8018C9DC(gScreenSets, *(s32*)(el + 0x15C));
            (*(TextMeasureFn*)(*(u8**)font + 0x10))(font, *(u8**)(el + 0x170), i, end, &w, &h, 1.0f);
            w = w * scale;
        }
    }
    fn_8019F4C8(el);
    return 0;
}

// 0x801CA7B0
// Writes the GX BP register (mask 0xFE000FE0, then 0x41000000 | value) selected by mode 0..7;
// other modes write nothing.
void fn_801CA7B0(u32 mode) {
    static const u16 kVal[8] = {0x100, 0x4A0, 0x420, 0x920, 0x360, 0x2A0, 0x420, 0x920};
    if (mode > 7) return;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE000FE0u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x41000000u + kVal[mode];
}

}  // extern "C"
