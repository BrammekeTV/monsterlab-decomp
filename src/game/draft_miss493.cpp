// Small functions from draft miss493 (0x8004xxxx-0x8020xxxx). Hand-written from the asm, unverified against a build.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" f32 GetOne(void);  // 0x80014208 (session.cpp defines it with C++ linkage)

extern "C" {
void  BattleHudSetTurn(s32 side, BOOL on);                              // 0x80048BF4
void* ChoiceDestroy(void* c, s32 del);                                  // 0x8020F394
void  MemFreeSmall(void* p);                                            // 0x801D4340
void  OSSetAlarm(void* alarm, u64 tick, void (*handler)(void));         // SDK alarm setter
u32   RandomU32(void);                                                  // 0x80143460
void  RendClear(int flags, int a, int b, f32 depth);                    // 0x801534F0
void  WPADControlMotor(s32 chan, u32 command);                          // 0x80272E3C
void  fn_80058CEC(s32 a, s32 b, s32 c, s32 d, s32 e, f32 scale);        // 0x80058CEC
void  fn_800C2174(u8* a3, s32 a4, s32 a5);                              // 0x800C2174
s32   fn_80109600(u8* p);                                               // 0x80109600
s32   fn_80109A0C(s32 id);                                              // 0x80109A0C
void  fn_801719DC(void* p);                                             // 0x801719DC
void  fn_8018D4CC(void* p);                                             // 0x8018D4CC
u8*   fn_801B3308(void);                                                // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                      // 0x801B3378
BOOL  fn_801B50B0(void* unused, u8* obj);                               // 0x801B50B0
void  fn_801B2E10(void);                                                // 0x801B2E10 (alarm handler)
s32   fn_801DA780(u8* a3);                                              // 0x801DA780
u8*   fn_801DA788(u8* a3, s32 i);                                       // 0x801DA788
u8*   fn_801F6238(u8* p, s32 del);                                      // 0x801F6238
u8*   fn_801F94EC(u8* p, s32 del);                                      // 0x801F94EC
void  fn_8020A398(s32 handle);                                          // 0x8020A398
void  fn_8020A958(u8* p);                                               // 0x8020A958
void  fn_8020F46C(u8* p, u8* choice);                                   // 0x8020F46C
void  fn_80052638(void);                                              // 0x80052638
s32   fn_802E9EE0(void);                                                // 0x802E9EE0
u8    fn_802E9F3C(void);                                                // 0x802E9F3C
void  fn_802F78A0(u8 slot, s32 a, s32 b);                               // 0x802F78A0
void  fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c);     // 0x8032F1E8
u64   fn_8032FB38(u32 clk, f32 secs);                                   // 0x8032FB38
void  operator_delete(void* p);                                         // 0x801D426C

extern u8*  gBattle;                    // 0x805FBBB8
extern u8   gFlag805FB948;              // 0x805FB948
extern u32  lbl_805FB9A4;               // 0x805FB9A4
extern s32  lbl_805FB9E4;               // 0x805FB9E4 (counter)
extern u8   lbl_8035CF68[];             // 0x8035CF68 (vtable)
extern u32  lbl_8035F640[];             // 0x8035F640 (tag table)
extern u8   lbl_803C6ED8[];             // 0x803C6ED8
extern u8   lbl_803C6F00[];             // 0x803C6F00 (three floats)
extern u32  lbl_805F8C70;               // 0x805F8C70 (tag)
extern u32  lbl_805F8C78[];             // 0x805F8C78 (tag table)
extern u32  lbl_805F8C80[];             // 0x805F8C80 (tag table)
extern u8   lbl_805F8C88[];             // 0x805F8C88
extern u8   lbl_805FB2A8[];             // 0x805FB2A8 (flag byte)
extern u8   lbl_80559CF8[];             // 0x80559CF8 (pool block)
extern u8   lbl_8055CAB0[];             // 0x8055CAB0 (choice table)
extern f32  lbl_805FD228;               // 0x805FD228
extern f32  lbl_805FFAF4;               // 0x805FFAF4
extern f32  lbl_805FFB2C;               // 0x805FFB2C
extern f32  lbl_805FE038;               // 0x805FE038
extern f32  lbl_805FE03C;               // 0x805FE03C
extern f32  lbl_805FE540;               // 0x805FE540
extern f32  lbl_805FE550;               // 0x805FE550
}

// Virtual slot calls below use the repo form (*(void (**)(u8*))(*(u8**)obj + OFF))(obj).
extern "C" {

// 0x8004B3E0
// Advances the timer at +0xC by farg0 while it is below the limit at +0x8; returns 0 while running.
// Once the timer is done, clears the flag bytes at +4/+5 and returns 1. The turn flag (+0x14) ends the turn.
s32 fn_8004B3E0(u8* arg0, f32 farg0) {
    f32 cur = *(f32*)(arg0 + 0xC);
    if (*(f32*)(arg0 + 0x8) > cur) {
        f32 next = cur + farg0;
        *(f32*)(arg0 + 0xC) = next;
        if (next >= lbl_805FD228 && *(u8*)(arg0 + 0x14) != 0) {
            BattleHudSetTurn(*(s32*)(arg0 + 0x10), 0);
        }
        return 0;
    }
    if (*(u8*)(arg0 + 0x14) != 0) {
        BattleHudSetTurn(*(s32*)(arg0 + 0x10), 0);
    }
    *(u8*)(arg0 + 0x4) = 0;
    *(u8*)(arg0 + 0x5) = 0;
    return 1;
}

// 0x8005F460
// Scales farg0 by GetOne() unless the global flag is set and lbl_805FB9A4 is nonzero, then calls fn_80058CEC.
// The asm passes (arg1, arg0) to GetOne; session.cpp defines GetOne(void), so the args are not passed.
void fn_8005F460(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, f32 farg0) {
    f32 scale = farg0;
    if (gFlag805FB948 == 0 || lbl_805FB9A4 == 0) {
        scale *= GetOne();
    }
    fn_80058CEC(arg0, arg1, arg2, arg3, arg4, scale);
}

// 0x8009F5B0
// Deleting destructor: resets the vtable at +0 to lbl_8035CF68, frees the buffers at +0x4 and +0x8,
// clears +0xC, and frees the object when del > 0. Returns the object.
void* fn_8009F5B0(u8* arg0, s32 del) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x0) = lbl_8035CF68;
        u8* buf = *(u8**)(arg0 + 0x4);
        if (buf != NULL) {
            MemFreeSmall(buf - 0x10);
            *(void**)(arg0 + 0x4) = NULL;
        }
        u8* extra = *(u8**)(arg0 + 0x8);
        if (extra != NULL) {
            MemFreeSmall(extra);
            *(void**)(arg0 + 0x8) = NULL;
        }
        *(s32*)(arg0 + 0xC) = 0;
        if (del > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800ADBE0
// Resets the per-object state: clears the handles at +0x264 and +0x330, releases the object at +0x26C
// (sets bit 1 in the flags of the record it links to), then sets the scale floats and clears the timers.
void fn_800ADBE0(u8* arg0) {
    *(s32*)(arg0 + 0x264) = -1;
    *(s32*)(arg0 + 0x330) = 0;
    if (*(u8**)(arg0 + 0x26C) != NULL) {
        fn_801B50B0(arg0, *(u8**)(arg0 + 0x26C));
        u8* rec = *(u8**)(*(u8**)(arg0 + 0x26C) + 0x4);
        *(u32*)(rec + 0x20) = *(u32*)(rec + 0x20) | 1;
        *(void**)(arg0 + 0x26C) = NULL;
    }
    *(f32*)(arg0 + 0x270) = lbl_805FE038;
    *(f32*)(arg0 + 0x274) = lbl_805FE038;
    *(f32*)(arg0 + 0x278) = lbl_805FE038;
    *(f32*)(arg0 + 0x360) = lbl_805FE03C;
    *(u8*)(arg0 + 0x364) = 0;
    *(f32*)(arg0 + 0x368) = lbl_805FE03C;
    *(f32*)(arg0 + 0x36C) = lbl_805FE03C;
}

// 0x800CE3E4
// Sets the three floats at lbl_803C6F00 and registers five tag tables with fn_800C2174.
// fn_800C2174 takes its pointer arguments as s32 (see its definition in auto_03_800C13D8_text.cpp).
void fn_800CE3E4(void) {
    *(f32*)(lbl_803C6F00 + 0x0) = lbl_805FE540;
    *(f32*)(lbl_803C6F00 + 0x4) = lbl_805FE550;
    *(f32*)(lbl_803C6F00 + 0x8) = lbl_805FE540;
    fn_800C2174(lbl_803C6ED8,        (s32)(uintptr_t)&lbl_805F8C70, 1);
    fn_800C2174(lbl_803C6ED8 + 0x8,  (s32)(uintptr_t)lbl_8035F640, 0xE);
    fn_800C2174(lbl_803C6ED8 + 0x10, (s32)(uintptr_t)lbl_805F8C78, 2);
    fn_800C2174(lbl_803C6ED8 + 0x18, (s32)(uintptr_t)lbl_805F8C80, 2);
    fn_800C2174(lbl_803C6ED8 + 0x20, (s32)(uintptr_t)lbl_805F8C88, 1);
}

// 0x8011CBBC
// Walks 0x1C-byte slots (count at +0x12F0, id at +0x34 of each slot): a slot whose id fails fn_80109A0C and
// passes fn_80109600 moves its id to +0x30 and clears +0x34. Sets lbl_805FB2A8 when a slot moved.
void fn_8011CBBC(u8* arg0) {
    u8* slot = arg0;
    u8* key = arg0 + 0x34;
    s32 i;

    lbl_805FB2A8[0] = 0;
    for (i = 0; i < *(s32*)(arg0 + 0x12F0); i++) {
        s32 id = *(s32*)(slot + 0x34);
        if (id != 0 && fn_80109A0C(id) == 0 && fn_80109600(key) != 0) {
            *(s32*)(slot + 0x30) = *(s32*)(slot + 0x34);
            *(s32*)(slot + 0x34) = 0;
            lbl_805FB2A8[0] = 1;
        }
        slot += 0x1C;
        key += 0x1C;
    }
}

// 0x8019957C
// Shuts the object down: calls virtual slot 0x14 (through the sub-object at +0x138) on each child in the
// list at +0x140 (count at +0x13C), releases the handle at +0x14C, then calls fn_8018D4CC on the object.
s32 fn_8019957C(u8* arg0) {
    s32 i;

    for (i = 0; i < *(s32*)(arg0 + 0x13C); i++) {
        u8* child = *(u8**)(*(u8**)(arg0 + 0x140) + i * 4);
        if (child != NULL) {
            (*(void (**)(u8*))(*(u8**)(child + 0x138) + 0x14))(child);
        }
    }
    if (*(s32*)(arg0 + 0x14C) != -1) {
        fn_8020A398(*(s32*)(arg0 + 0x14C));
        *(s32*)(arg0 + 0x14C) = -1;
    }
    fn_8018D4CC(arg0);
    return 0;
}

// 0x801B2F10
// Rumble: when rumble is enabled, arms an alarm from the time at +0xC (seconds, scaled by the clock at 0x800000F8),
// sets the rumble-active byte at +0x48 and turns the motor on for the channel at +0x4.
void fn_801B2F10(u8* arg0) {
    if (GetOptionsRumble() != 0) {
        f32 secs = *(f32*)(arg0 + 0xC);
        if (secs > lbl_805FFAF4) {
            u32 clk = *(u32*)0x800000F8;
            u64 tick = fn_8032FB38(clk, secs * (f32)(clk >> 2));
            OSSetAlarm(arg0 + 0x18, tick, fn_801B2E10);
        }
        *(u8*)(arg0 + 0x48) = 1;
        WPADControlMotor(*(s32*)(arg0 + 0x4), 1);
    }
}

// 0x801B704C
// Clears the render target, then calls fn_801719DC on each entry of the list returned by virtual slot 0xC
// (fn_801DA780 gives the entry count, fn_801DA788 gives entry i).
void fn_801B704C(u8* arg0) {
    s32 count;
    s32 i;

    RendClear(4, 0, 0, lbl_805FFB2C);
    count = fn_801DA780((*(u8* (**)(u8*))(*(u8**)arg0 + 0xC))(arg0));
    for (i = 0; i < count; i++) {
        fn_801719DC(fn_801DA788((*(u8* (**)(u8*))(*(u8**)arg0 + 0xC))(arg0), i));
    }
}

// 0x80208180
// Calls fn_802F78A0 for every slot below fn_802E9EE0 except the slot from fn_802E9F3C, counting each call in
// lbl_805FB9E4. Skipped entirely when virtual slot 0x5C returns nonzero. Always returns 0.
s32 fn_80208180(u8* arg0, s32 arg1, s32 arg2) {
    s32 i;

    if ((*(s32 (**)(u8*))(*(u8**)arg0 + 0x5C))(arg0) == 0) {
        for (i = 0; i < fn_802E9EE0(); i++) {
            if (i != (s32)fn_802E9F3C()) {
                fn_802F78A0((u8)i, arg1, arg2);
                lbl_805FB9E4 += 1;
            }
        }
    }
    return 0;
}

// 0x8020BC50
// Sets up the four object pools inside lbl_80559CF8 (element size and count per pool).
void fn_8020BC50(void) {
    fn_8032F1E8(lbl_80559CF8 + 0x80, (void (*)(void))fn_8020A958, 0, 0x28, 0x100);
    fn_8032F1E8(lbl_80559CF8 + 0x2C80, (void (*)(void))fn_80052638, 0, 0xC, 4);
    fn_8032F1E8(lbl_80559CF8 + 0x2CB0, (void (*)(void))fn_80052638, 0, 0xC, 4);
    fn_8032F1E8(lbl_80559CF8 + 0x2CE0, (void (*)(void))fn_80052638, 0, 0xC, 4);
}

// 0x8020F608
// Builds a choice record from the table at lbl_8055CAB0 (row from the object's type at +0x44, column from the
// battle turn at gBattle+0xA10 mod 7), commits it through fn_8020F46C, then destroys the temporary.
void fn_8020F608(u8* arg0) {
    u8 choice[12];
    s32 row = *(s32*)(*(u8**)(arg0 + 0x8) + 0x44) * 0x54;
    s32 col = ((*(s32*)(gBattle + 0xA10) - 1) % 7) * 0xC;
    u8* entry = lbl_8055CAB0 + row + col;

    *(s32*)(choice + 0x0) = *(s32*)entry;
    choice[0x4] = *(u8*)(entry + 0x4);
    *(s32*)(choice + 0x8) = *(s32*)(entry + 0x8);
    fn_8020F46C(arg0, choice);
    ChoiceDestroy(choice, -1);
}

// 0x800302B0
// Initializes a record from the source object at arg1: copies the vector at +0x68..+0x70 to +0x2C..+0x34,
// clears the state words, and sets the timer at +0x18 to a random value in 8..12.
void fn_800302B0(u8* arg0, u8* arg1) {
    *(u8**)(arg0 + 0x4) = arg1;
    *(s32*)(arg0 + 0x24) = 0;
    *(u8*)(arg0 + 0x20) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(f32*)(arg0 + 0x2C) = *(f32*)(arg1 + 0x68);
    *(f32*)(arg0 + 0x30) = *(f32*)(arg1 + 0x6C);
    *(f32*)(arg0 + 0x34) = *(f32*)(arg1 + 0x70);
    *(s32*)(arg0 + 0x14) = 0;
    *(f32*)(arg0 + 0x18) = (f32)((RandomU32() % 5) + 8);
}

// 0x800324DC
// Deleting destructor: releases the two sub-objects at +0x4C and +0x108 through fn_801B3378 and
// fn_801F6238/fn_801F94EC, clears the state words, and frees the object when del > 0. Returns the object.
void* fn_800324DC(u8* arg0, s32 del) {
    if (arg0 != NULL) {
        fn_801B3378(fn_801B3308(), arg0 + 0x4C);
        fn_801B3378(fn_801B3308(), arg0 + 0x108);
        *(s32*)(arg0 + 0x18) = 0;
        *(s32*)(arg0 + 0x0) = 0;
        *(s32*)(arg0 + 0xC) = 0;
        *(s32*)(arg0 + 0x10) = 0;
        *(s32*)(arg0 + 0x14) = 0;
        fn_801F94EC(arg0 + 0x108, -1);
        fn_801F6238(arg0 + 0x4C, -1);
        if (del > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

}  // extern "C"
