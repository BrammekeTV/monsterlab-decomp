// Small functions in 0x8002xxxx (incl. digging challenge callbacks).
#include <stdint.h>

#include "types.h"

extern void* gLab;                   // 0x805FBBC8
extern void* gScreenSets;            // 0x805FB654
extern u8  lbl_803558F8[];           // 0x803558F8 vtable-like table (0x18)
extern u8  lbl_80355870[];           // 0x80355870
extern u8  lbl_803558B0[];           // 0x803558B0
extern u8  lbl_80355828[];           // 0x80355828
extern u8  lbl_803557E8[];           // 0x803557E8
extern u8  lbl_803557A8[];           // 0x803557A8
extern u8  lbl_80355760[];           // 0x80355760
extern u8  lbl_80355718[];           // 0x80355718
extern u8  lbl_803556D0[];           // 0x803556D0
extern u8  lbl_80355690[];           // 0x80355690
extern s32 lbl_805FAF94;             // 0x805FAF94
extern u8  lbl_805FAF99;             // 0x805FAF99
extern u8  lbl_805FAF9B;             // 0x805FAF9B
extern u32 lbl_805FAFA4;             // 0x805FAFA4 (screen element hash)

extern "C" {
void  fn_800202FC(u8* a3);                                  // 0x800202FC (defined in leaf)
u8*   fn_80021750(void);                                    // 0x80021750 (defined in leaf)
void  LabShowBattleResult(void* lab, const void* result);   // 0x802186D8
void  operator_delete(void* p);                             // 0x801D4980
void* ScreenSetsCurrent(void* sets);                        // 0x8018D1E4
void  ScreenSetsActivate(void* sets, u32 hash);             // 0x8018D70C
void  ScreenSetsFlush(void* sets);                          // 0x8018DCA4
void* ScreenSetFindElement(void* set, u32 hash);            // 0x80198F18
}

typedef void (*ScreenElemFn)(void* el, s32 arg);

extern "C" {

// 0x80020480
// Calls fn_800202FC(obj), then stores table lbl_803558F8 at obj+0x4C. Returns obj.
void* fn_80020480(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_803558F8;
    return a0;
}

// 0x80020590
// Same as fn_80020480 with table lbl_80355870.
void* fn_80020590(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_80355870;
    return a0;
}

// 0x80020758
// Same as fn_80020480 with table lbl_803558B0.
void* fn_80020758(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_803558B0;
    return a0;
}

// 0x80020864
// Same as fn_80020480 with table lbl_80355828.
void* fn_80020864(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_80355828;
    return a0;
}

// 0x80020A04
// Same as fn_80020480 with table lbl_803557E8.
void* fn_80020A04(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_803557E8;
    return a0;
}

// 0x80020D28
// Same as fn_80020480 with table lbl_803557A8.
void* fn_80020D28(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_803557A8;
    return a0;
}

// 0x8002104C
// Same as fn_80020480 with table lbl_80355760.
void* fn_8002104C(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_80355760;
    return a0;
}

// 0x8002112C
// Same as fn_80020480 with table lbl_80355718.
void* fn_8002112C(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_80355718;
    return a0;
}

// 0x80021240
// Same as fn_80020480 with table lbl_803556D0.
void* fn_80021240(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_803556D0;
    return a0;
}

// 0x80021354
// Same as fn_80020480 with table lbl_80355690.
void* fn_80021354(void* a0) {
    u8* p = (u8*)a0;
    fn_800202FC(p);
    *(u32*)(p + 0x4C) = (u32)(uintptr_t)lbl_80355690;
    return a0;
}

// 0x80021678
// Deleting-destructor tail: frees p when del > 0. Returns p.
void* fn_80021678(void* a0, s32 a1) {
    if (a0 != 0 && a1 > 0) operator_delete(a0);
    return a0;
}

// 0x80022394
// Zeroes a battle-result block and clears lbl_805FAF9B, then shows the battle result on gLab.
void fn_80022394(void) {
    u8 r[0x14];
    lbl_805FAF9B = 0;
    r[0] = 0;
    *(u32*)(r + 0x4) = 0;
    *(u32*)(r + 0xC) = 0;
    *(u32*)(r + 0x10) = 0;
    LabShowBattleResult(gLab, r);
}

// 0x800225C8
// Once per object (flag at +0x82): latches flag, stores (s32)obj->f32[+0x6C] in lbl_805FAF94
// and sets lbl_805FAF99 when that value is >= 5.
void fn_800225C8(void* a0) {
    u8* p = (u8*)a0;
    if (p[0x82] != 0) return;
    f32 f = *(f32*)(p + 0x6C);
    p[0x82] = 1;
    s32 v = (s32)f;
    lbl_805FAF94 = v;
    if (v >= 5) lbl_805FAF99 = 1;
}

// 0x80022D34
// If the hash lbl_805FAFA4 is set: activate it in gScreenSets, then call vslot 0x2C (arg 2)
// on the element found in the current set.
void fn_80022D34(void) {
    u32 hash = lbl_805FAFA4;
    if (hash == 0) return;
    ScreenSetsActivate(gScreenSets, hash);
    void* el = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), hash);
    u8* vt = *(u8**)((u8*)el + 0x138);
    ScreenElemFn fn = *(ScreenElemFn*)(vt + 0x2C);
    fn(el, 2);
}

// 0x80022D88
// If the hash lbl_805FAFA4 is set: flush gScreenSets, then call vslot 0x30 (arg 2)
// on the element found in the current set.
void fn_80022D88(void) {
    u32 hash = lbl_805FAFA4;
    if (hash == 0) return;
    ScreenSetsFlush(gScreenSets);
    void* el = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), hash);
    u8* vt = *(u8**)((u8*)el + 0x138);
    ScreenElemFn fn = *(ScreenElemFn*)(vt + 0x30);
    fn(el, 2);
}

// 0x800232D0
// Returns 1 when the digging record's byte at +0xBC equals 1.
s32 DiggingRockPresent(void) {
    u8* p = fn_80021750();
    return p[0xBC] == 1;
}

// 0x80023300
// Returns 1 when the byte at +0xBC equals 1 and the s32 at +0xC0 is > 0.
s32 DiggingRockIntact(void) {
    u8* p = fn_80021750();
    if (p[0xBC] == 1 && *(s32*)(p + 0xC0) > 0) return 1;
    return 0;
}

// 0x80023370
// Returns 1 when the byte at +0xFC equals 1.
s32 DiggingCheckChallengeFinished(void) {
    u8* p = fn_80021750();
    return p[0xFC] == 1;
}

}
