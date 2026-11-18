// More small functions in 0x8002xxxx (digging challenge).
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern void* gEventMgr;              // 0x805FB6C8
extern void* lbl_805FBBAC;           // 0x805FBBAC (voice SFX handle)
extern void* lbl_805FAFB8;           // 0x805FAFB8 (global object, +0x6C / +0x78 floats)
extern s32   lbl_805FAFBC;           // 0x805FAFBC (toggle counter, -1 or 0)
extern u8    lbl_805F8310;           // 0x805F8310 (flag byte)
extern u8    lbl_803573E4[];         // 0x803573E4 (event name string)
extern f32   lbl_805FC8E0;           // 0x805FC8E0 (float constant)
extern f32   lbl_805FC8F0;           // 0x805FC8F0 (float constant)
extern f32   lbl_805FC8F4;           // 0x805FC8F4 (float constant)
extern f32   lbl_805FC8F8;           // 0x805FC8F8 (float constant)
extern f32   lbl_805FC940;           // 0x805FC940 (float constant)
extern f32   lbl_805FC990;           // 0x805FC990 (float constant, used as 0.0f)
extern f32   lbl_805FC99C;           // 0x805FC99C (float constant)
extern f32   lbl_805FCA24;           // 0x805FCA24 (float constant)

extern "C" {
void  fn_80020334(u8* p);                                   // 0x80020334
u8*   fn_80021750(void);                                    // 0x80021750 (defined in leaf)
void  fn_80021F2C(u8* p, f32 v);                            // 0x80021F2C (f1 forwarded unchanged)
void  fn_80022394(void);                                    // 0x80022394 (defined in small_8002)
void  fn_8002238C(void);                                    // 0x8002238C
void  fn_800223D4(void);                                    // 0x800223D4
void  fn_800223D8(void);                                    // 0x800223D8
void  fn_800223DC(u8* p);                                   // 0x800223DC
void  fn_800223EC(void);                                    // 0x800223EC
void  fn_800223F0(u8* p, f32 v);                            // 0x800223F0
void  fn_80022E0C(void);                                    // 0x80022E0C
void  fn_80022F28(void);                                    // 0x80022F28
void  fn_80023160(void);                                    // 0x80023160
void  fn_80026C88(u8* p);                                   // 0x80026C88
void  fn_80027434(u8* p);                                   // 0x80027434
u8*   fn_801B3308(void);                                    // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                            // 0x801B3310
void* fn_801D40E0(s32 n);                                   // 0x801D40E0
void* fn_80209814(void* v);                                 // 0x80209814
void  fn_80028A84(u8* p, f32 v);                            // 0x80028A84
s32  PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e); // 0x80209E70
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
u32   RandomU32(void);                                      // 0x80143460
u32   StringHash(const char* s);                            // 0x801EB704
}

extern "C" {

// 0x800214D4
// Calls fn_80020334 on each entry of the list at +0x240 (count s32 at +0x1EE8), then on the
// first eight 0x48-byte records at the start of the object.
void fn_800214D4(u8* p) {
    s32 n = *(s32*)(p + 0x1EE8);
    u8* e = p + 0x240;
    for (s32 i = 0; i < n; i++, e += 0x48) fn_80020334(e);
    e = p;
    for (s32 i = 0; i < 8; i++, e += 0x48) fn_80020334(e);
}

// 0x80021DD0
// Rolls RandomU32 as a float: v = lbl_805FC8E0 + lbl_805FC8F4 * (lbl_805FC8F0 * roll).
// Returns 1 when v is below lbl_805FC8F8, else 0. The result is the cr0 LT bit
// (mfcr + srwi 31 after fcmpo cr0, f1, f0 gives "f1 < f0").
s32 fn_80021DD0(void) {
    f32 t = (f32)RandomU32();
    f32 scaled = lbl_805FC8F0 * t;
    f32 v = lbl_805FC8E0 + lbl_805FC8F4 * scaled;
    return v < lbl_805FC8F8 ? 1 : 0;
}

// 0x80021ECC
// Creates the voice SFX handle lbl_805FBBAC once (via fn_801D40E0, then fn_80209814 if it
// returned something), then plays sound 0x8E77F0E1.
void fn_80021ECC(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(0x8E77F0E1, 0, 0, 0, 0, 0);
}

// 0x80023D38
// Ensures the voice handle exists (fn_80021ECC), then fn_800223F0 with the constant lbl_805FC940.
void fn_80023D38(u8* p) {
    fn_80021ECC();
    fn_800223F0(p, lbl_805FC940);
}

// 0x800233A0
// Digging record: decrements the counters at +0xEC and +0xF4 by one.
void DiggingDecrementScoop(void) {
    u8* p = fn_80021750();
    *(s32*)(p + 0xEC) -= 1;
    *(s32*)(p + 0xF4) -= 1;
}

// 0x800233D8
// Digging record: decrements the s32 at +0xC0 by one, clamped at zero (negative or zero stays 0).
void DiggingDecrementRock(void) {
    u8* p = fn_80021750();
    s32 v = *(s32*)(p + 0xC0);
    *(s32*)(p + 0xC0) = v > 0 ? v - 1 : 0;
}

// 0x80024B7C
// If either flag byte at +0x80 or +0x81 is set, sends event 0xC608C842 to the target at +0xB8.
void fn_80024B7C(u8* p) {
    if (p[0x80] == 0 && p[0x81] == 0) return;
    EventSend(gEventMgr, *(void**)(p + 0xB8), NULL, 0xC608C842, 0, 0, 1, 0);
}

// 0x800259D8
// Resets a record: clears the s32 at +0x0 and the floats at +0x4, +0x8, +0xC, +0x1C, +0x20,
// +0x24, +0x34 to lbl_805FC990, and the byte at +0x44.
void fn_800259D8(u8* p) {
    *(s32*)(p + 0x0) = 0;
    *(f32*)(p + 0x4) = lbl_805FC990;
    *(f32*)(p + 0x8) = lbl_805FC990;
    *(f32*)(p + 0xC) = lbl_805FC990;
    *(f32*)(p + 0x1C) = lbl_805FC990;
    *(f32*)(p + 0x20) = lbl_805FC990;
    *(f32*)(p + 0x24) = lbl_805FC990;
    *(f32*)(p + 0x34) = lbl_805FC990;
    p[0x44] = 0;
}

// 0x80027958
// Calls fn_8002238C, then sends event 0x88C6E941 to the target at +0x94.
void fn_80027958(u8* p) {
    fn_8002238C();
    EventSend(gEventMgr, *(void**)(p + 0x94), NULL, 0x88C6E941, 0, 0, 1, 0);
}

// 0x800279AC
// Calls fn_800223D4, then sends event 0x88C6E941 to the target at +0x94.
void fn_800279AC(u8* p) {
    fn_800223D4();
    EventSend(gEventMgr, *(void**)(p + 0x94), NULL, 0x88C6E941, 0, 0, 1, 0);
}

// 0x80027A00
// Ensures the voice handle exists, then on the first call (flag at +0x90) runs fn_80026C88,
// fn_80027434, and fn_801B3310 on the list from fn_801B3308 and +0xCC, and sets the flag.
// Finishes with fn_800223F0 using the constant lbl_805FCA24.
void fn_80027A00(u8* p) {
    fn_80021ECC();
    if (p[0x90] == 0) {
        fn_80026C88(p);
        fn_80027434(p);
        fn_801B3310(fn_801B3308(), p + 0xCC);
        p[0x90] = 1;
    }
    fn_800223F0(p, lbl_805FCA24);
}

// 0x80027BAC
// Calls fn_800223DC, then sends event 0xF15918A6 to the target at +0x94.
void fn_80027BAC(u8* p) {
    fn_800223DC(p);
    EventSend(gEventMgr, *(void**)(p + 0x94), NULL, 0xF15918A6, 0, 0, 1, 0);
}

// 0x80027C00
// Calls fn_800223EC; if either flag byte at +0x80 or +0x81 is set, sends event 0x117741C3.
void fn_80027C00(u8* p) {
    fn_800223EC();
    if (p[0x80] == 0 && p[0x81] == 0) return;
    EventSend(gEventMgr, *(void**)(p + 0x94), NULL, 0x117741C3, 0, 0, 1, 0);
}

// 0x80027C70
// Clears the battle result (fn_80022394), then hides the eight UI elements in the table at
// +0xB0 (stride 0x48). Each entry points to a record whose first word is the element hash.
void fn_80027C70(u8* p) {
    fn_80022394();
    u8* table = *(u8**)(p + 0xB0);
    for (s32 i = 0; i < 8; i++) {
        void* rec = *(void**)(table + i * 0x48);
        UIElement_SetVisibleFlag(*(u32*)rec, 0);
    }
}

// 0x80027CDC
// Calls fn_800223D8, then sends the event named by lbl_803573E4 (hashed) to the target at +0x94.
void fn_80027CDC(u8* p) {
    fn_800223D8();
    u32 h = StringHash((const char*)lbl_803573E4);
    EventSend(gEventMgr, *(void**)(p + 0x94), NULL, h, 0, 0, 1, 0);
}

// 0x80027D60
// Toggle: if lbl_805FAFBC is 0 or more it becomes -1; otherwise fn_80022F28 runs and it becomes 0.
void fn_80027D60(void) {
    if (lbl_805FAFBC >= 0) {
        lbl_805FAFBC = -1;
    } else {
        fn_80022F28();
        lbl_805FAFBC = 0;
    }
}

// 0x80027DA0
// Using the global object at lbl_805FAFB8: when its float at +0x6C is above lbl_805FC990 and
// below its float at +0x78 times lbl_805FC99C, and the flag lbl_805F8310 is set, runs fn_80023160
// and clears the flag.
void fn_80027DA0(void) {
    u8* g = (u8*)lbl_805FAFB8;
    f32 limit = *(f32*)(g + 0x78) * lbl_805FC99C;
    f32 v = *(f32*)(g + 0x6C);
    if (v > lbl_805FC990 && v < limit && lbl_805F8310 != 0) {
        fn_80023160();
        lbl_805F8310 = 0;
    }
}

// 0x80027DFC
// Calls fn_80022E0C, then sends event 0x88C6E941 to the target at +0x94.
void fn_80027DFC(u8* p) {
    fn_80022E0C();
    EventSend(gEventMgr, *(void**)(p + 0x94), NULL, 0x88C6E941, 0, 0, 1, 0);
}

// 0x80028928
// Calls fn_80021F2C (f1 forwarded unchanged), then fn_80028A84 with the same float.
void fn_80028928(u8* p, f32 v) {
    fn_80021F2C(p, v);
    fn_80028A84(p, v);
}

}
