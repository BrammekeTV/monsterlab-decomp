// Render/DVD helpers: texture header setup, dispatch table add, render request ring, async read wait.
#include "types.h"
#include "revolution.h"

extern "C" {

s32   DVDReadAsync(u8* fileInfo, void* addr, s32 len, s32 offset, void* cb, s32 prio);  // SDK
s32   DVDGetDriveStatus(void);                                                       // 0x8025394C
u32   OSGetBusClock(void);
s32   OSSignalSemaphore(OSSemaphore* sem);
s32   OSWaitSemaphore(OSSemaphore* sem);
void  fn_80146B18(void);                                                             // 0x80146B18 (read-done callback)
void  fn_8015A5A4(u8* p, const char* name);                                          // 0x8015A5A4
void  fn_8023D3C8(s32 hi, u32 lo);                                                   // 0x8023D3C8 (sleep ticks)

extern u8  lbl_805FB470[];     // 0x805FB470 (function-pointer slot, called with 0 when the disc needs attention)
extern u8  lbl_805FB47C[];     // 0x805FB47C (s32: async read pending)
extern u8  lbl_805FB480[];     // 0x805FB480 (s32: async read result)
extern s32 lbl_805FB544;       // 0x805FB544 (dispatch table count)
extern u8  lbl_805FB548[];     // 0x805FB548 (dispatch table keys)
extern u8  lbl_8040AE88[];     // 0x8040AE88 (dispatch table handlers)
extern s32 lbl_805FB560;       // 0x805FB560 (render request ring write index)
extern u8  sRendThread[];      // 0x8040AEA8

// 0x80146B28
// Starts an asynchronous DVD read (fileInfo, buf, size, offset) and blocks until fn_80146B18 reports
// completion, polling the drive status: a fatal error stops the wait, "no disc"/"wrong disc"/"retry"
// call the user callback in lbl_805FB470, other states sleep 5 ms. Returns the read result, or 0 if
// the read could not be queued.
u32 fn_80146B28(u8* fileInfo, u8* buf, u32 size, u32 offset) {
    s32 waiting;
    *(s32*)lbl_805FB47C = 1;
    *(s32*)lbl_805FB480 = 0;
    if (DVDReadAsync(fileInfo, buf, (s32)size, (s32)offset, (void*)fn_80146B18, 2) == 0) {
        return 0;
    }
    waiting = 1;
    while (*(s32*)lbl_805FB47C == 1 && waiting != 0) {
        u32 idx = (u32)(DVDGetDriveStatus() + 1);
        if (idx <= 12) {
            switch (idx) {
            case 0:                       // fatal error: give up waiting
                waiting = 0;
                continue;
            case 1: case 2: case 3:       // ready / busy / waiting: just poll again
                continue;
            case 5: case 7: case 12: {    // no disc / wrong disc / retry: ask the user callback
                void* cb = *(void**)lbl_805FB470;
                if (cb != NULL) {
                    ((void (*)(s32))cb)(0);
                }
                continue;
            }
            default:                      // 4, 6, 8..11: fall through to the delay
                break;
            }
        }
        {
            // OSMicrosecondsToTicks(5000)
            u64 ticks = (u64)(OSGetBusClock() / 4 / 125000) * 5000 / 8;
            fn_8023D3C8((s32)(ticks >> 32), (u32)ticks);
        }
    }
    return *(u32*)lbl_805FB480;
}
}  // extern "C"
