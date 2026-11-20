// Small functions in 0x801Fxxxx-0x8020xxxx: audio slot table, cleanup and volume helpers.
#include "types.h"
#include "revolution.h"   // MEMAllocator, MEMFreeToAllocator

// Globals (C++ linkage, defined elsewhere or in the original binary).
extern u8  gWorldState[];           // 0x803E6D00
extern MEMAllocator lbl_803F2870;   // 0x803F2870 (heap allocator)
extern u8  lbl_804FD9E0[];          // 0x804FD9E0 (0x60-byte float block, reset by fn_801FC038)
extern u8  lbl_805F98B0[];          // 0x805F98B0 (flag byte, set by fn_801FBE04)
extern u8  lbl_805F98C1;            // 0x805F98C1 (flag byte checked by fn_801FD904)
extern f32 lbl_805F98D0;            // 0x805F98D0 (last clamped level, see fn_801FFF24)
extern u8  lbl_805FBA38[];          // 0x805FBA38
extern u8  lbl_805FBA3C[];          // 0x805FBA3C
extern u8  lbl_805FBA50[];          // 0x805FBA50
extern u8  lbl_805FBA54[];          // 0x805FBA54
extern s32 lbl_805FBA58;            // 0x805FBA58
extern s32 lbl_805FBA5C;            // 0x805FBA5C
extern s32 lbl_805FBA60;            // 0x805FBA60
extern s32 lbl_805FBA64;            // 0x805FBA64
extern s32 lbl_805FBA68;            // 0x805FBA68
extern u32 lbl_805FBA78;            // 0x805FBA78 (value stored by fn_801FC038)
extern void* lbl_805FBA90;          // 0x805FBA90 (allocated block freed by fn_801FD1F0)
extern void* lbl_805FBADC;          // 0x805FBADC (buffer freed by fn_801FD1F0)
extern u8  lbl_805FBAB8;            // 0x805FBAB8 (flag byte cleared by fn_801FD1F0)
extern s32 lbl_805FBAE0;            // 0x805FBAE0 (re-entrancy guard, set to 1 while a slot op runs)
extern u8  lbl_8051DAD0[];          // 0x8051DAD0 (state block; +0xB4 checked by fn_801FD904)
extern u8  lbl_8051E600[];          // 0x8051E600 (object passed to fn_80234418)
extern u8  lbl_8051E680[];          // 0x8051E680 (0x28-byte records)
extern u8  lbl_8051E6A8[];          // 0x8051E6A8 (0x228-byte records, see fn_801FFF24)
extern u8  lbl_80524E28[];          // 0x80524E28 (0x60 slot entries of 0x58 bytes)
extern f32 lbl_806002DC;            // 0x806002DC (float constant, value not verified)
extern f32 lbl_806002E0;            // 0x806002E0 (float constant, value not verified)
extern f32 lbl_80600354;            // 0x80600354 (lower clamp bound, fn_801FFF24)
extern f32 lbl_80600358;            // 0x80600358 (upper clamp bound, fn_801FFF24)

extern "C" {

// Callee prototypes (C linkage, matching their definitions elsewhere).
void  BufferFree(void* p);                                       // 0x801D4980
BOOL  ZoneIsLoaded(void* world, s32 zone);                       // 0x8012F408
s32   fn_801B62A4(u8* a3);                                       // 0x801B62A4
s32   fn_801FE52C(u8* audio, u8* desc, s32 c);                   // 0x801FE52C (return type from use)
void  fn_801FF2A8(u8* audio, s32 handle);                        // 0x801FF2A8 (return value unused)
void  fn_80234418(void* p);                                      // 0x80234418
void  fn_80257C88(void);                                         // 0x80257C88 (return value unused)
void  fn_80260AB0(void);                                         // 0x80260AB0 (return value unused)
void  fn_80263F4C(void);                                         // 0x80263F4C (return value unused)

}  // extern "C"

// Finds the slot entry for a 32-bit id: the high half is the slot index (0..0x5F), the low
// half must match the id stored at the start of that entry. Returns NULL for -1 and misses.
static u8* FindSlotEntry(s32 id) {
    s32 idx;
    u8* e;

    if (id == -1) {
        return NULL;
    }
    idx = id >> 16;
    if (idx < 0 || idx >= 0x60) {
        return NULL;
    }
    e = lbl_80524E28 + idx * 0x58;
    if ((u16)id != (u16)*(s32*)e) {
        return NULL;
    }
    return e;
}

extern "C" {

// 0x801FBE04
// Resets the audio state flags and counters to their idle values.
void fn_801FBE04(void) {
    *(u8*)lbl_805F98B0 = 1;
    *(u8*)lbl_805FBA38 = 0;
    *(s32*)lbl_805FBA3C = 0;
    *(s32*)lbl_805FBA50 = 0;
    *(s32*)lbl_805FBA54 = 0;
    lbl_805FBA58 = 0;
    lbl_805FBA5C = 0;
    lbl_805FBA60 = 0;
    lbl_805FBA64 = 0;
    lbl_805FBA68 = 0;
}

// 0x801FC038
// Stores the argument in lbl_805FBA78 and writes the float constants lbl_806002DC /
// lbl_806002E0 into fixed offsets of the 0x60-byte block at lbl_804FD9E0.
void fn_801FC038(u32 arg0) {
    lbl_805FBA78 = arg0;
    *(f32*)(lbl_804FD9E0 + 0x0) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0x4) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0xC) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0x10) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0x14) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x18) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x1C) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0x24) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x28) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0x2C) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x30) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0x34) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x3C) = lbl_806002DC;
    *(f32*)(lbl_804FD9E0 + 0x40) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x44) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x48) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x4C) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x54) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x58) = lbl_806002E0;
    *(f32*)(lbl_804FD9E0 + 0x5C) = lbl_806002E0;
}

// 0x801FD1F0
// Tears down the audio resources, if the object at arg0 is flagged as active.
// The original also has a branch that would call fn_802737A8 with a 5 stored into
// lbl_8051E680+0x24, but it tests the constant 0 and is never taken, so it is omitted.
void fn_801FD1F0(void* arg0) {
    u8* self = (u8*)arg0;
    s32 prev;

    prev = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (*(u8*)(self + 0x2AC) == 0) {
        lbl_805FBAE0 = prev;
        return;
    }
    if (lbl_805FBAB8 != 0) {
        fn_80234418(lbl_8051E600);
        lbl_805FBAB8 = 0;
    }
    *(s32*)(lbl_8051E680 + 0x24) = 0;
    fn_80263F4C();
    fn_80260AB0();
    fn_80257C88();
    BufferFree(lbl_805FBADC);
    lbl_805FBADC = NULL;
    MEMFreeToAllocator(&lbl_803F2870, lbl_805FBA90);
    lbl_805FBA90 = NULL;
    *(u8*)(self + 0x2AC) = 0;
    lbl_805FBAE0 = prev;
}

// 0x801FD904
// Counts down the timer at +0x2B4; once it reaches zero, sets the done flag at +0x29
// (unless the global flag is set and the state block's value is not 2).
void fn_801FD904(u8* arg0) {
    if (*(s32*)(arg0 + 0x2B4) > 0) {
        *(s32*)(arg0 + 0x2B4) = *(s32*)(arg0 + 0x2B4) - 1;
    }
    if (*(s32*)(arg0 + 0x2B4) != 0) {
        return;
    }
    if (lbl_805F98C1 != 0) {
        if (*(u32*)(lbl_8051DAD0 + 0xB4) != 2) {
            return;
        }
        *(u8*)(arg0 + 0x29) = 1;
    } else {
        *(u8*)(arg0 + 0x29) = 1;
    }
}

// 0x801FF090
// Plays the sound described by arg1 on the zone stored in its target record, after
// checking that the zone is loaded. Clamps the record's volume (+0x24) to its maximum (+0x28).
s32 fn_801FF090(u8* arg0, u8* arg1) {
    s32 prev;
    u8* target;
    s32 res;

    prev = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    target = *(u8**)arg1;
    if (target == NULL || *(s32*)(target + 0x4) == 0 ||
        ZoneIsLoaded(gWorldState, *(s16*)(target + 0xE)) == 0) {
        lbl_805FBAE0 = prev;
        return -1;
    }
    if (*(s32*)(arg1 + 0x4) == 0) {
        lbl_805FBAE0 = prev;
        return -1;
    }
    if (*(f32*)(arg1 + 0x24) > *(f32*)(arg1 + 0x28)) {
        *(f32*)(arg1 + 0x24) = *(f32*)(arg1 + 0x28);
    }
    res = fn_801FE52C(arg0, arg1, 2);
    lbl_805FBAE0 = prev;
    return res;
}

// 0x801FF154
// Same as fn_801FF090 but checks a second condition (fn_801B62A4 on +0x8) and uses kind 3.
s32 fn_801FF154(u8* arg0, u8* arg1) {
    s32 prev;
    u8* target;
    s32 res;

    prev = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    target = *(u8**)arg1;
    if (target == NULL || *(s32*)(target + 0x4) == 0 ||
        ZoneIsLoaded(gWorldState, *(s16*)(target + 0xE)) == 0) {
        lbl_805FBAE0 = prev;
        return -1;
    }
    if (fn_801B62A4((u8*)(uintptr_t)*(s32*)(arg1 + 0x8)) == 0) {
        lbl_805FBAE0 = prev;
        return -1;
    }
    if (*(f32*)(arg1 + 0x24) > *(f32*)(arg1 + 0x28)) {
        *(f32*)(arg1 + 0x24) = *(f32*)(arg1 + 0x28);
    }
    res = fn_801FE52C(arg0, arg1, 3);
    lbl_805FBAE0 = prev;
    return res;
}

// 0x801FF21C
// Finds the slot whose +0xC field equals arg1 and stops its handle (fn_801FF2A8).
void fn_801FF21C(u8* arg0, u8* arg1) {
    s32 prev;
    s32 i;

    prev = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (arg1 != NULL) {
        for (i = 0; i < 0x60; i++) {
            u8* e = lbl_80524E28 + i * 0x58;
            if (*(u8**)(e + 0xC) == arg1) {
                fn_801FF2A8(arg0, *(s32*)e);
                break;
            }
        }
    }
    lbl_805FBAE0 = prev;
}

// 0x801FF47C
// Stops every active slot (non-zero +0x10) through fn_801FF2A8.
void fn_801FF47C(u8* arg0) {
    s32 prev;
    s32 i;

    prev = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    for (i = 0; i < 0x60; i++) {
        u8* e = lbl_80524E28 + i * 0x58;
        if (*(s32*)(e + 0x10) != 0) {
            fn_801FF2A8(arg0, *(s32*)e);
        }
    }
    lbl_805FBAE0 = prev;
}

// 0x801FF500
// Like fn_801FF47C, but skips slots whose +0x50 flags have bit 0x200 set.
void fn_801FF500(u8* arg0) {
    s32 prev;
    s32 i;

    prev = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    for (i = 0; i < 0x60; i++) {
        u8* e = lbl_80524E28 + i * 0x58;
        if (*(s32*)(e + 0x10) != 0 && (*(s32*)(e + 0x50) & 0x200) == 0) {
            fn_801FF2A8(arg0, *(s32*)e);
        }
    }
    lbl_805FBAE0 = prev;
}

// 0x801FF620
// Returns 1 if the slot identified by arg1 exists and its +0x8 value is not -1, else 0.
s32 fn_801FF620(u8* arg0, s32 arg1) {
    u8* e;

    (void)arg0;
    e = FindSlotEntry(arg1);
    if (e == NULL) {
        return 0;
    }
    return (*(s32*)(e + 0x8) != -1) ? 1 : 0;
}

// 0x801FF6A4
// Stores arg2 as the callback pointer (+0x54) of the slot identified by arg1.
void fn_801FF6A4(u8* arg0, s32 arg1, u8* arg2) {
    u8* e;

    (void)arg0;
    e = FindSlotEntry(arg1);
    if (e != NULL) {
        *(void**)(e + 0x54) = arg2;
    }
}

// 0x801FFF24
// Clamps arg farg0 to [lbl_80600354, lbl_80600358], stores it in lbl_805F98D0, and rescales
// the level of the 0x2F pairs of records in lbl_8051E6A8 (type-2 entries at +0x24 -> +0x28).
void fn_801FFF24(u8* arg0, f32 farg0) {
    f32 v;
    s32 prev;
    u8* p;
    s32 i;

    (void)arg0;
    if (farg0 < lbl_80600354) {
        v = lbl_80600354;
    } else if (farg0 > lbl_80600358) {
        v = lbl_80600358;
    } else {
        v = farg0;
    }
    prev = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    lbl_805F98D0 = v;
    p = lbl_8051E6A8 + 0x228;
    for (i = 0; i < 0x2F; i++) {
        u8* a = *(u8**)(p + 0xC);
        u8* b;
        if (a != NULL && *(s32*)(a + 0x14) == 2) {
            *(s32*)(a + 0x28) = (s32)((f32)*(s32*)(a + 0x24) * v);
        }
        b = *(u8**)(p + 0x120);
        if (b != NULL && *(s32*)(b + 0x14) == 2) {
            *(s32*)(b + 0x28) = (s32)((f32)*(s32*)(b + 0x24) * v);
        }
        p += 0x228;
    }
    lbl_805FBAE0 = prev;
}

}  // extern "C"
