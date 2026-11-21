#pragma once
// Shared by the renderer files that write GX commands into the current display list: the
// display-list ring's globals, the pipe helpers, and the "close this list, open the next" steps
// the original inlines everywhere.
#include "engine/rend.h"

extern "C" {
void RendDLBegin(RendDL* dl, RendDLRecord* rec);          // 0x8015B9F8
void RendDLEnd(RendDL* dl);                               // 0x8015BA4C
u8*  RendDLGetBase(RendDL* dl);                           // 0x8015BBB8
void RendThreadSubmitDL(RendDLRecord* rec, BOOL signalFree);   // 0x8015E5D0
void RendThreadSync(void);                                // 0x8015E930
void RendWaitFreeDL(void);                                // 0x8015EA34
BOOL RendHasFreeDL(void);                                 // 0x8015EAF4
void RendSetDefaultState(void);                           // 0x8014855C
void fn_8015E9D8(void* unused, s32 frame, s32 maxAhead);  // 0x8015E9D8: wait for the render thread
void fn_8015E7C8(void* unusedR3, s32 unusedR4, s32 unusedR5, s32 unusedR6, s32 value);   // 0x8015E7C8: queue "timer start"
void fn_8014ED64(s32 field);                              // 0x8014ED64
void fn_80142ED0(u8* base, s32 size);                     // 0x80142ED0
s32  fn_8015BA30(void* dl);                               // 0x8015BA30: bytes written to a display list
void fn_8015B7D4(void);                                   // 0x8015B7D4

extern u8   sRendDLs[];                 // 0x80401108 (display-list buffers, 0x94 bytes each)
extern u8   sRendDLRecords[];           // 0x80401ADC (display-list records, 0x14 bytes each)
extern u8   lbl_803488F0[];             // 0x803488F0 (24 bytes: copy-filter sample pattern)
extern u8   lbl_805FB594[];             // 0x805FB594 (render thread object)
extern s32  lbl_805FB4BC;               // 0x805FB4BC (current display list)
extern s32  lbl_805FB4C0;               // 0x805FB4C0 (a display list is open)
extern s32  lbl_805FB4C4;               // 0x805FB4C4
extern s32  lbl_805FB4C8;               // 0x805FB4C8
extern s32  lbl_805FB4CC;               // 0x805FB4CC
extern u8*  lbl_805FB4D0;               // 0x805FB4D0 (base of the open display list)
extern s32  lbl_805FB4D4;               // 0x805FB4D4 (submissions are being held back)
extern s32  lbl_805FB4D8;               // 0x805FB4D8 (first held-back record)
extern s32  lbl_805FB4DC;               // 0x805FB4DC (number of held-back records)
extern s32  lbl_805FB4E0;               // 0x805FB4E0
extern s32  lbl_805FB4E4;               // 0x805FB4E4
extern s32  lbl_805FB4E8;               // 0x805FB4E8
extern s32  lbl_805FB4EC;               // 0x805FB4EC (display lists left)
extern s32  (*lbl_805FB4F0)(void);      // 0x805FB4F0 (optional callback asked at frame start)
extern s32  lbl_805FB500;               // 0x805FB500
extern s32  lbl_805FB504;               // 0x805FB504
extern u8   lbl_805FB50C;               // 0x805FB50C (GX shadow flags byte)
extern u8   lbl_805FB50D;               // 0x805FB50D (GX shadow mask byte)
extern s32  lbl_805FB514;               // 0x805FB514 (GX shadow word)
extern s32  lbl_805FB518;               // 0x805FB518 (frame counter)
extern s32  lbl_805FB51C;               // 0x805FB51C
extern s32  lbl_805FB520;               // 0x805FB520
extern s32  lbl_805F9214;               // 0x805F9214
extern s32  lbl_805F9234;               // 0x805F9234 (viewport width)
extern s32  lbl_805F9238;               // 0x805F9238 (viewport height)
}

static inline void Bp(u32 v)  { GXWGFifo._u8 = 0x61; GXWGFifo._u32 = v; }
static inline void Cp(u8 reg, u32 v) { GXWGFifo._u8 = 8; GXWGFifo._u8 = reg; GXWGFifo._u32 = v; }
static inline void XfHead(u16 countMinus1, u16 reg) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = countMinus1;
    GXWGFifo._u16 = reg;
}

static inline RendDL* CurDL(void) { return (RendDL*)(sRendDLs + lbl_805FB4BC * 0x94); }
static inline RendDLRecord* Record(s32 i) { return (RendDLRecord*)(sRendDLRecords + i * 0x14); }

// Submits the records held back while submissions were deferred, if the render thread has no
// free display list left; then waits for a free one and opens it. (Inlined three times in the
// original.)
static inline void OpenNextDL(void) {
    if (lbl_805FB4D4 != 0 && !RendHasFreeDL() && lbl_805FB4D4 != 0) {
        RendThreadSync();
        while (lbl_805FB4DC > 0) {
            RendThreadSubmitDL(Record(lbl_805FB4D8), lbl_805FB4E0 == 0);
            if (++lbl_805FB4D8 >= 0x11) {
                lbl_805FB4D8 = 0;
            }
            lbl_805FB4DC--;
        }
        lbl_805FB4D4 = 0;
        lbl_805FB4D8 = 0;
        lbl_805FB4DC = 0;
    }
    RendWaitFreeDL();
    RendDLBegin(CurDL(), Record(lbl_805FB4BC));
    lbl_805FB4C0 = 1;
    lbl_805FB4C4 = 0x24000;
    lbl_805FB4C8 = 0x24000;
    lbl_805FB4CC = 0;
    lbl_805FB4D0 = RendDLGetBase(CurDL());
    lbl_805FB4EC--;
}

// Ends the open display list and submits it (or counts it as held back).
static inline void CloseDL(void) {
    RendDLEnd(CurDL());
    s32 rem = 0x24000 - lbl_805FB4C8;
    if (rem != 0) {
        fn_80142ED0(RendDLGetBase(CurDL()) + lbl_805FB4C8, rem);
    }
    if (lbl_805FB4D4 != 0) {
        lbl_805FB4DC++;
    } else {
        RendThreadSubmitDL(Record(lbl_805FB4BC), lbl_805FB4E0 == 0);
    }
    lbl_805FB4C0 = 0;
    lbl_805FB4D0 = NULL;
    lbl_805FB4C4 = 0;
    lbl_805FB4C8 = 0;
    lbl_805FB4CC = 0;
}

// Makes sure the open display list has `need` more bytes; moves on to the next one if not.
// (Inlined in the original; fn_8014DE68 is the out-of-line form of the inner part.)
static inline void EnsureDLRoom(s32 need) {
    if (fn_8015BA30(CurDL()) + need > lbl_805FB4C8) {
        if (lbl_805FB4C0 != 0) {
            CloseDL();
            if (lbl_805FB4E0 != 0) {
                lbl_805FB4E8++;
            }
            if (++lbl_805FB4BC >= 0x11) {
                lbl_805FB4BC = 0;
            }
            OpenNextDL();
        }
        fn_8015BA30(CurDL());
    }
}
