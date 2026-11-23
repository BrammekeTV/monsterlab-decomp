// Small functions in 0x8020xxxx (incl. save file close).
#include <string.h>
#include "types.h"
#include "revolution.h"   // OSReport

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
void  fn_80200920(void* p, s32 zero, s32 idx);                   // 0x80200920
void  fn_802B4140(void* fn, s32* args);                          // 0x802B4140
void  fn_802B7DDC(void);                                         // 0x802B7DDC
void  fn_802BE220(void* h);                                      // 0x802BE220
s32   GXReadDrawSync(void);                                         // 0x8024B34C
s32   fn_80266E24(u8* buf);                                      // 0x80266E24
void  fn_8023A5B0(u8* p);                                        // 0x8023A5B0
void  fn_8023A68C(u8* p);                                        // 0x8023A68C
void  fn_801EF070(u8* obj, s32 v);                               // 0x801EF070
void* fn_801EF6E8(u8* obj, s32 a, s32 v);                        // 0x801EF6E8
void  fn_802E9C94(void* fn, s32 arg);                            // 0x802E9C94
void  fn_802F7A2C(void* fn);                                     // 0x802F7A2C
void  fn_802F7A4C(void* fn);                                     // 0x802F7A4C
void fn_80207AB8(s32 arg);                                         // 0x80207AB8
void  fn_80207A28(void);                                         // 0x80207A28
void fn_80207A2C(u32 unusedR3, const void* src);                                         // 0x80207A2C
void  fn_802063E4(...);                                         // 0x802063E4

extern s32 lbl_8051E5F0[];          // 0x8051E5F0 (s32 table)
extern u8  lbl_8051E680[];          // 0x8051E680 (0x28-byte records)
extern s32 lbl_805FBAC8;            // 0x805FBAC8
extern void* lbl_805FBACC;          // 0x805FBACC
extern s32 lbl_805FBAD0;            // 0x805FBAD0
extern f32 lbl_80600354;            // 0x80600354
extern s32 lbl_805FB6B8;            // 0x805FB6B8
extern s32 lbl_806003F8;            // 0x806003F8
extern s32 lbl_806003FC;            // 0x806003FC
extern void* lbl_805FBB20;          // 0x805FBB20
extern u8  lbl_80386C10[];          // 0x80386C10
extern u8  lbl_80527140[];          // 0x80527140 (save buffer, 0x8C bytes)
extern u8  lbl_80556A98[];          // 0x80556A98 (lock)
extern u8  lbl_80556AE0[];          // 0x80556AE0
extern const char lbl_80387380[];   // 0x80387380 (debug message)
extern s32 lbl_805FB374;            // 0x805FB374
extern s32 lbl_805FB36C;            // 0x805FB36C
extern s32 lbl_805FBB30;            // 0x805FBB30
}

extern "C" {

// 0x802014D4
// Sets the first argument's record through fn_80200920 and clears entry [idx] of the s32 table.
void fn_802014D4(u8* p, s32 idx) {
    fn_80200920(p, 0, idx);
    lbl_8051E5F0[idx] = 0;
}

// 0x80202FA4
// flag == 0: if record[idx] is in state 5, reset it to 0.
// flag == -1: nothing. Otherwise: register this function as a pending callback in state 5.
void fn_80202FA4(s32 idx, s32 flag) {
    if (flag == 0) {
        u8* rec = lbl_8051E680 + idx * 0x28;
        if (*(s32*)(rec + 0x24) == 5) *(s32*)(rec + 0x24) = 0;
        return;
    }
    if (flag == -1) return;
    lbl_805FBAC8 = 5;
    lbl_805FBACC = (void*)&fn_80202FA4;
    lbl_805FBAD0 = 0;
}

// 0x802041B0
// Resets a 0x58-byte record: ids to -1, counters to 0, a float to its default, and a global copied in.
void fn_802041B0(u8* p) {
    *(s32*)(p + 0x0)  = -1;
    *(s32*)(p + 0x4)  = -1;
    *(s32*)(p + 0x8)  = -1;
    *(s32*)(p + 0xc)  = 0;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = -1;
    *(s32*)(p + 0x44) = lbl_805FB6B8;
    *(s32*)(p + 0x4c) = 0;
    *(s32*)(p + 0x50) = 0;
    *(s32*)(p + 0x30) = 0;
    *(f32*)(p + 0x34) = lbl_80600354;
    *(s32*)(p + 0x54) = 0;
}

// 0x80204430
// Deleting destructor: frees p when del > 0. Returns p.
u8* fn_80204430(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x80204478
// Registers fn_802B7DDC with a two-word argument block copied from two globals.
void fn_80204478(void) {
    s32 args[2];
    args[0] = lbl_806003F8;
    args[1] = lbl_806003FC;
    fn_802B4140((void*)&fn_802B7DDC, args);
}

// 0x8020506C
// If the handle at +0x178 is set, releases it and clears the slot.
void fn_8020506C(u8* p) {
    void* h = *(void**)(p + 0x178);
    if (h) {
        fn_802BE220(h);
        *(void**)(p + 0x178) = nullptr;
    }
}

// 0x80206308
// Advances the index at +0x114 (wrapping at the count at +0x100) and, if the slot there is set,
// polls GXReadDrawSync until its low halfword no longer matches idx + 0x4444, then clears the slot.
// The advanced index is not written back (original behaviour).
void fn_80206308(u8* p) {
    s32 idx = *(s32*)(p + 0x114) + 1;
    if (idx >= *(s32*)(p + 0x100)) idx = 0;
    s32* slots = (s32*)(p + 0x1d0);
    if (slots[idx] != 0) {
        s32 want = idx + 0x4444;
        s32 v;
        do {
            v = GXReadDrawSync();
        } while ((v & 0xFFFF) == want);
        slots[idx] = 0;
    }
}

// 0x8020637C
// First call: records p in lbl_805FBB20 and clears the 0x8C-byte save buffer.
// Later calls: stash varargs through fn_802063E4 instead. Returns p.
u8* fn_8020637C(u8* p) {
    if (lbl_805FBB20 != nullptr) {
        fn_802063E4(lbl_80386C10);
    } else {
        lbl_805FBB20 = p;
        memset(lbl_80527140, 0, 0x8c);
    }
    return p;
}

// 0x802063E4
// Variadic register-spill helper. It only stores its arguments to its own stack frame, so it has no effect.
void fn_802063E4(...) {
}

// 0x80206434
// Deleting destructor: frees p when del > 0. Returns p.
u8* fn_80206434(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x802067BC
// Closes the save file. Returns 0 on success, or -1 if the close call fails (buffer is cleared on success).
s32 SaveClose(s32 handle) {
    (void)handle;
    s32 r = fn_80266E24(lbl_80527140);
    if (r != 0) return -1;
    memset(lbl_80527140, 0, 0x8c);
    return 0;
}

// 0x802077B8
// If value != 0, passes it to fn_801EF070 on the lbl_80556AE0 object, under the lock on lbl_80556A98.
void fn_802077B8(void* unused, s32 value) {
    (void)unused;
    if (value != 0) {
        fn_8023A5B0(lbl_80556A98);
        fn_801EF070(lbl_80556AE0, value);
        fn_8023A68C(lbl_80556A98);
    }
}

// 0x80207814
// Calls fn_801EF6E8 (size 0x20, value) on lbl_80556AE0 under the lock; reports a debug message on failure.
// Returns the call's result.
void* fn_80207814(void* unused, s32 value) {
    (void)unused;
    fn_8023A5B0(lbl_80556A98);
    void* r = fn_801EF6E8(lbl_80556AE0, 0x20, value);
    fn_8023A68C(lbl_80556A98);
    if (r == NULL) OSReport(lbl_80387380);
    return r;
}

// 0x8020788C
// Same as fn_802077B8: if value != 0, passes it to fn_801EF070 under the lock.
void fn_8020788C(void* unused, s32 value) {
    (void)unused;
    if (value != 0) {
        fn_8023A5B0(lbl_80556A98);
        fn_801EF070(lbl_80556AE0, value);
        fn_8023A68C(lbl_80556A98);
    }
}

// 0x80207BE4
// Sets the global flag lbl_805FB374 to 1. With flag == 0 registers the three callbacks and sets
// lbl_805FB36C/lbl_805FBB30 to 1; otherwise clears lbl_805FB36C and sets lbl_805FBB30 to 7.
void fn_80207BE4(s32 flag) {
    lbl_805FB374 = 1;
    if (flag == 0) {
        fn_802E9C94((void*)&fn_80207AB8, 0);
        fn_802F7A2C((void*)&fn_80207A28);
        fn_802F7A4C((void*)&fn_80207A2C);
        lbl_805FB36C = 1;
        lbl_805FBB30 = 1;
    } else {
        lbl_805FB36C = 0;
        lbl_805FBB30 = 7;
    }
}

// 0x80208140
// Deleting destructor: frees p when del > 0. Returns p.
u8* fn_80208140(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

}
