// Small functions from the miss471 draft batch (0x8002xxxx to 0x80207xxx).
#include "types.h"

extern "C" {
void  fn_8002238C(void);                                                       // 0x8002238C
void  CatchMinigame_UpdateObjects(void* g, f32 dt);                            // 0x8002FFBC
void  CatchMinigame_UpdateLane(void* g, void* lane, BOOL active, f32 dt);      // 0x8002EF4C
void  OSReport(const char* fmt, ...);
u32   OSGetBusClock(void);                                                     // reads 0x800000F8
void  OSSetAlarm(void* alarm, s64 tick, void (*handler)(void));
void  fn_801B2E10(void);                                                       // alarm handler (0x801B2E10)
u64   fn_8032FB38(u32 clk, f32 secs);                                          // 0x8032FB38
u32   RandomU32(void);
s32   TriggerDoAction(void* action, void* a1, void* a2);
void  WPADControlMotor(s32 chan, u32 command);                                 // 0x80272E3C
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count); // 0x8032F2E0
void  operator_delete(void* p);                                                // 0x801D426C
void* fn_8010B184(void* p, s32 del);                                          // 0x8010B184
void  fn_8005382C(u8* p, s32 mode);
void  fn_8006FAF4(u8* p, s32 a);
u8*   fn_80076FCC(s32 a, s32 b);
void  fn_800C6EFC(u8* p);
void  fn_800C7EEC(u8* p, f32 v);
void  fn_800C831C(u8* p);
void  fn_800CCAF4(u8* p, f32 v);
void  fn_800CD018(u8* p, f32 v);
void  fn_800CDCC4(u8* p, u8* a, u8* b);
s32   fn_80109600(u8* p);
s32   fn_80109A0C(s32 id);
void  fn_80121C90(s32 action, void* obj, void* other, const f32* pos);
void  fn_801ACD08(u8* a, u8* b, u8* out, f32 x, f32 y);
u8*   fn_801B3308(void);
void  fn_801B3378(u8* p, u8* key);
s32   fn_801C3E88(u8* tbl, u8* handle);
void* fn_801EF6E8(u8* obj, s32 a, s32 v);
u8*   fn_801FAB3C(u8* p, s32 del);
void  fn_80234418(void* p);
void  OSLockMutex(u8* p);
void  OSUnlockMutex(u8* p);

extern u8 lbl_8035A950[];
extern const char lbl_80387380[];
extern u8 lbl_8047C7AC[];
extern u8 lbl_80556A98[];
extern u8 lbl_80556AE0[];
extern u8 lbl_805FAFC8[];   // s32 word
extern u8 lbl_805FB118[];   // pointer slot
extern u8 lbl_805FB138[];   // pointer slot
extern u8 lbl_805FB260[];   // byte flag
extern f32 lbl_805FFAC0;    // float constant, the 1/3 factor of the average at +0x3C
extern f32 lbl_805FFAF4;    // float constant compared against the alarm period
}

extern "C" {

// 0x8002A814
// Advances the cycle counter at +0x10C and wraps it, then refreshes the current slot at +0x110.
void fn_8002A814(u8* p) {
    u8* t = *(u8**)(p + *(s32*)(p + 0x110) * 4 + 0x104);
    (*(void (**)(u8*, s32))(*(u8**)(t + 0x138) + 0x30))(t, 2);

    s32 cnt = *(s32*)(p + 0x10C);
    *(s32*)(p + 0x10C) = cnt + 1;
    s32 n = (*(s32 (**)(u8*, s32))(*(u8**)p + 0x14))(p, cnt);
    if (*(s32*)(p + 0x10C) >= *(s32*)lbl_805FAFC8 * n) {
        *(s32*)(p + 0x10C) = 0;
    }

    s32 idx = *(s32*)(p + 0x10C) / *(s32*)lbl_805FAFC8;
    *(s32*)(p + 0x110) = idx;
    u8* u = *(u8**)(p + idx * 4 + 0x104);
    (*(void (**)(u8*, s32))(*(u8**)(u + 0x138) + 0x2C))(u, 2);
}

// 0x8002ABA4
// Sets the state flag at +0x18, re-rolls the index at +8 with RandomU32, then runs the
// per-entry virtual at slot 0x2C on each entry of the list at +0x104 (returns 1).
s32 fn_8002ABA4(u8* p) {
    (*(void (**)(u8*))(*(u8**)p + 0x18))(p);
    *(s32*)(p + 0x18) = 1;

    u8* o = *(u8**)(p + 0x114);
    s32 n = (RandomU32() & 3) + *(s32*)(p + 0x8);
    *(s32*)(p + 0x8) = n - 2;
    (*(void (**)(u8*, s32))(*(u8**)(o + 0x138) + 0x30))(o, 2);

    s32 i = 0;
    while (i < (*(s32 (**)(u8*))(*(u8**)p + 0x14))(p)) {
        u8* e = *(u8**)(p + 0x104 + i * 4);
        (*(void (**)(u8*, s32))(*(u8**)(e + 0x138) + 0x2C))(e, 0x18);
        i++;
    }
    return 1;
}

// 0x8002F720
// Per-frame update of the catch mini-game: objects, both lanes, then copies two floats
// from the source record at +0xF0C into the object at +0x294 and runs its slot 0x2C.
void fn_8002F720(u8* p, f32 dt) {
    fn_8002238C();
    CatchMinigame_UpdateObjects(p, dt);
    CatchMinigame_UpdateLane(p, p + 0x90, *(u8*)(p + 0xF74), dt);
    CatchMinigame_UpdateLane(p, p + 0xC8, *(u8*)(p + 0xF74) == 0, dt);

    u8* a = *(u8**)(p + 0x294);
    if (a != NULL) {
        u8* b = *(u8**)(p + 0xF0C);
        if (b != NULL) {
            u8* src = *(u8**)(b + 0x4);
            *(f32*)(a + 0x68) = *(f32*)(src + 0x68);
            *(f32*)(a + 0x6C) = *(f32*)(src + 0x6C);
            (*(void (**)(u8*, s32))(*(u8**)(a + 0x138) + 0x2C))(a, 0x1A);
        }
    }
}

// 0x8005DB68
// When arg1 is set, clears the two slots at +0xC2C/+0xC34 (releasing the handle at +0xC34
// through fn_801C3E88) and clears the global current pointer if it is p. Returns the last result.
s32 fn_8005DB68(u8* p, s32 arg1) {
    fn_8005382C(p, arg1);
    s32 r = 0; // fn_8005382C returns nothing; leftover r3 is not modelled
    if (arg1 != 0) {
        if (*(u8**)lbl_805FB118 == p) {
            *(u8**)lbl_805FB118 = NULL;
        }
        u8* slot = p;
        for (s32 i = 0; i < 2; i++) {
            *(s32*)(slot + 0xC2C) = 0;
            u8* h = *(u8**)(slot + 0xC34);
            if (h != NULL) {
                r = fn_801C3E88(lbl_8047C7AC, h);
                *(u8**)(slot + 0xC34) = NULL;
            }
            slot += 4;
        }
    }
    return r;
}

// 0x8006A200
// Copies the field at +0x60 of arg1's record (at +0x9C) into +0x14C, fires the action at +0x88
// with a copy of the vector arg2, then triggers the two action slots on arg1 when their bits are set.
void fn_8006A200(u8* p, u8* arg1, u8* arg2) {
    if (arg1 != NULL) {
        u8* t = *(u8**)(arg1 + 0x9C);
        if (t != NULL) {
            *(s32*)(p + 0x14C) = *(s32*)(t + 0x60);
        }
    }
    if (*(s32*)(p + 0x88) != 0) {
        f32 pos[3];
        pos[0] = *(f32*)(arg2 + 0x0);
        pos[1] = *(f32*)(arg2 + 0x4);
        pos[2] = *(f32*)(arg2 + 0x8);
        fn_80121C90(*(s32*)(p + 0x88), p, arg1, pos);
    }
    if (arg1 != NULL) {
        u8* t = *(u8**)(arg1 + 0x9C);
        if (t != NULL && ((((u32)*(s32*)(t + 0x74)) >> 0x1B) & 0xF)) {
            TriggerDoAction(t + 0x74, arg1, p);
        }
        if ((((u32)*(s32*)(arg1 + 0x44)) >> 0x1B) & 0xF) {
            TriggerDoAction(arg1 + 0x44, arg1, p);
        }
    }
}

// 0x80075894
// Deleting destructor: releases the handle at +0x6CC, tears down the lists and the array at
// +0x20C, and frees the object when del > 0. Returns p.
u8* fn_80075894(u8* p, s32 del) {
    if (p != NULL) {
        s32 h = *(s32*)(p + 0x6CC);
        *(u8**)(p + 0x4) = lbl_8035A950;
        if (h != 0) {
            fn_80076FCC(h, 1);
        }
        fn_801B3378(fn_801B3308(), p + 0x8C);
        fn_801B3378(fn_801B3308(), p + 0x14C);
        *(u8**)lbl_805FB138 = NULL;
        __destroy_arr(p + 0x20C, (void (*)(void*, s32))fn_8010B184, 0xC, 4);
        fn_801FAB3C(p + 0x14C, -1);
        fn_801FAB3C(p + 0x8C, -1);
        fn_8006FAF4(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800C7620
// Runs fn_800C6EFC over each of the three entry arrays (0x14-byte entries at +0x14, +0x7E4, +0xFB4,
// counts at +0x1784, +0x1788, +0x178C), zeroing each count, then resets +0x2F04 and calls fn_800C831C.
void fn_800C7620(u8* p) {
    s32 i;
    for (i = 0; i < *(s32*)(p + 0x1784); i++) {
        fn_800C6EFC(p + 0x14 + i * 0x14);
    }
    *(s32*)(p + 0x1784) = 0;
    for (i = 0; i < *(s32*)(p + 0x1788); i++) {
        fn_800C6EFC(p + 0x7E4 + i * 0x14);
    }
    *(s32*)(p + 0x1788) = 0;
    for (i = 0; i < *(s32*)(p + 0x178C); i++) {
        fn_800C6EFC(p + 0xFB4 + i * 0x14);
    }
    *(s32*)(p + 0x178C) = 0;
    *(s32*)(p + 0x2F04) = 0;
    fn_800C831C(p);
}

// 0x800C8EB4
// Clears the 50 words from +0x39C to +0x460.
void fn_800C8EB4(u8* p) {
    for (s32 off = 0x39C; off <= 0x460; off += 4) {
        *(s32*)(p + off) = 0;
    }
}

// 0x800CFCFC
// Updates the sub-objects at +0xA4, +0xF4 and +0xA8 from the float arguments; reads two
// result words through the function table at +0x30F4 for the +0x3020 object.
void fn_800CFCFC(u8* p, f32 a, f32 b) {
    s32 sp14;
    s32 sp8;

    fn_800CD018(*(u8**)(p + 0xA4), b);
    (*(void (**)(u8*, f32))(*(u8**)(p + 0x30F4) + 0x14))(p + 0x3020, a);
    (*(void (**)(u8*))(*(u8**)(p + 0x30F4) + 0x20))(p + 0x3020);
    (*(void (**)(u8*, u8*))(*(u8**)(p + 0x30F4) + 0x24))((u8*)&sp14, p + 0x3020);
    (*(void (**)(u8*, u8*))(*(u8**)(p + 0x30F4) + 0x28))((u8*)&sp8, p + 0x3020);
    fn_800CDCC4(p + 0xA8, (u8*)&sp14, (u8*)&sp8);
    fn_800C7EEC(p + 0xF4, a);
    fn_800CCAF4(*(u8**)(p + 0xA4), a);
}

// 0x80118068
// Walks the 0x20-byte records at +0x48 (count at +0x34): releases the ones fn_80109A0C reports
// done, moves the first one that is live into +0x44, and tracks the last live index at +0x14DC.
void fn_80118068(u8* p) {
    u32 i;
    s32 done = 0;

    lbl_805FB260[0] = 0;
    *(s32*)(p + 0x14DC) = 0;
    for (i = 0; i < *(u32*)(p + 0x34); i++) {
        u8* e = p + i * 0x20;
        u8* slot = p + 0x48 + i * 0x20;
        s32 h = *(s32*)(e + 0x48);
        if (h != 0) {
            if (fn_80109A0C(h) == 0) {
                fn_80109600(slot);
            } else if (i <= *(u32*)(p + 0x14DC)) {
                *(s32*)(e + 0x44) = h;
                *(s32*)(e + 0x48) = 0;
                lbl_805FB260[0] = 1;
            }
        }
        if (done == 0) {
            if (*(s32*)(e + 0x44) != 0) {
                *(s32*)(p + 0x14DC) = (s32)(i + 1);
            } else {
                done = 1;
            }
        }
    }
}

// 0x801AFA0C
// Saves the current point at +0x14/+0x18/+0x1C into +0x20..+0x28, computes a new point from
// the record at +8 through fn_801ACD08, and updates +0x3C..+0x48. When del != 0 the new point is
// also written back to +0x20..+0x28.
void fn_801AFA0C(u8* p, s32 del) {
    u8 out[0x30];  // fn_801ACD08 writes its point at out+0x24..+0x2C

    *(f32*)(p + 0x20) = *(f32*)(p + 0x14);
    *(f32*)(p + 0x24) = *(f32*)(p + 0x18);
    *(f32*)(p + 0x28) = *(f32*)(p + 0x1C);
    fn_801ACD08(p, *(u8**)(p + 0x8) + 0xC, out, *(f32*)(p + 0x18), *(f32*)(p + 0x14));

    u8* q = *(u8**)(p + 0x8);
    u8* t = *(u8**)(p + 0x10);
    *(f32*)(p + 0x14) = *(f32*)(out + 0x24);
    *(f32*)(p + 0x18) = *(f32*)(out + 0x28);
    *(f32*)(p + 0x1C) = *(f32*)(out + 0x2C);
    *(f32*)(p + 0x40) = *(f32*)(q + 0x40);
    *(f32*)(p + 0x44) = *(f32*)(q + 0x44);
    *(f32*)(p + 0x48) = *(f32*)(q + 0x48);
    *(f32*)(p + 0x3C) = *(f32*)(q + 0x24) * (lbl_805FFAC0 * (*(f32*)(t + 0x10) + (*(f32*)(t + 0x8) + *(f32*)(t + 0xC))));
    if (del != 0) {
        *(f32*)(p + 0x20) = *(f32*)(out + 0x24);
        *(f32*)(p + 0x24) = *(f32*)(out + 0x28);
        *(f32*)(p + 0x28) = *(f32*)(out + 0x2C);
    }
}

// 0x801B2FF0
// Sets the rumble/motor level at +0xC to a. When it changes on an active device (+0x10), starts
// the motor with a timed alarm at +0x18 (level 0) or stops it (level 1).
void fn_801B2FF0(u8* p, f32 a) {
    f32 cur = *(f32*)(p + 0xC);
    if (cur != a) {
        if (*(u8*)(p + 0x10) != 0) {
            if (cur == lbl_805FFAF4) {
                WPADControlMotor(*(s32*)(p + 0x4), 0);
                u32 clk = OSGetBusClock();
                u64 tick = fn_8032FB38(clk, a * (f32)(clk >> 2));
                OSSetAlarm(p + 0x18, tick, fn_801B2E10);
            } else if (a == lbl_805FFAF4) {
                fn_80234418(p + 0x18);
                WPADControlMotor(*(s32*)(p + 0x4), 1);
            }
        }
        *(f32*)(p + 0xC) = a;
    }
}

// 0x80207734
// Runs fn_801EF6E8 on lbl_80556AE0 under the lock at lbl_80556A98 and reports a debug message on
// failure. Returns the result of fn_801EF6E8.
void* fn_80207734(s32 arg0, s32 arg1) {
    OSLockMutex(lbl_80556A98);
    void* r = fn_801EF6E8(lbl_80556AE0, arg1, arg0);
    OSUnlockMutex(lbl_80556A98);
    if (r == NULL) {
        OSReport(lbl_80387380);
    }
    return r;
}

}  // extern "C"
