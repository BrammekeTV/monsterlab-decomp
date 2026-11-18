// Small functions in 0x8010xxxx (entity/level script callbacks, debug prints).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8 gGame[];                 // 0x803B53A0
extern u8 gWorldState[];           // 0x803E6D00
extern u8 lbl_803C2A20[];          // global block; tuning table pointer at +0x860 (used by fn_801070B0)
extern u8 lbl_8047C760[];          // effect manager
extern f32 lbl_805FEBB8;           // float constant, fourth element of the effect position
extern const char lbl_805F8E90[];  // table name, printed as %s by the PrintBit/PrintInt/PrintFloat helpers
extern const char lbl_803698C8[];  // format string: "f32 %s[%d] = %5.2f"
extern const char lbl_803698DC[];  // format string: int print
extern const char lbl_803698F0[];  // format string: "bit %s[%d] = %d"

extern "C" {
void     GameLog(int level, int category, const char* fmt, ...);  // 0x8000B2F0
void     operator_delete(void* p);                                 // 0x801D426C
void     SpawnEffectAt(void* fxMgr, void* effect, const f32 posAndScale[4], int a); // 0x801C80E0
void*    CreateEntityByType(void* mem, u32 typeHash, void* params); // entity_factory.cpp
void     fn_8005F204(u8* obj);                                     // 0x8005F204
void     fn_80061A64(void* a0, void* a1, void* a2);                // 0x80061A64 (signature not confirmed)
s32      fn_801DFF00(u8* base, s32 idx);                           // 0x801DFF00 (get bit)
s32      fn_801DFF54(u8* base, s32 idx);                           // 0x801DFF54 (get int)
f32      fn_801DFF9C(u8* base, s32 idx);                           // 0x801DFF9C (get float)
void     fn_801DFE94(u8* base, u8* table);                         // 0x801DFE94
void     fn_801DC574(s32 fn);                                      // 0x801DC574 (callback slot setter)
void     fn_801DC57C(s32 fn);                                      // 0x801DC57C (callback slot setter)
void     fn_801DC584(s32 fn);                                      // 0x801DC584 (callback slot setter)
void     fn_801DC58C(s32 fn);                                      // 0x801DC58C (callback slot setter)
void     fn_801DC594(s32 fn);                                      // 0x801DC594 (callback slot setter)
s32      fn_80106900(s32 a0);                                      // 0x80106900
u32      fn_80106988(s32 a0);                                      // 0x80106988
void     fn_80106920(void);                                        // 0x80106920 (only its address is taken)
void     fn_801069B0(void);                                        // 0x801069B0 (only its address is taken)
s32      fn_80106A2C(s32 code);                                    // 0x80106A2C
}

extern "C" {

// 0x80103624
// Asks the object whether it is dead (vtable slot 0x4C); if so, calls fn_8005F204 on it.
void ExplodeIfDead(u32 a0) {
    u8* obj = (u8*)(uintptr_t)a0;
    s32 dead = (*(s32 (**)(u8*))(*(u8**)obj + 0x4C))(obj);
    if (dead) fn_8005F204(obj);
}

// 0x80103710
// Copies the word at a1 and the word at a2, then passes the object a0 and both copies to fn_80061A64.
void JumpToPlatform(u32 a0, u32 a1, u32 a2) {
    u32 fromWord = *(u32*)(uintptr_t)a1;
    u32 toWord = *(u32*)(uintptr_t)a2;
    fn_80061A64((void*)(uintptr_t)a0, &fromWord, &toWord);
}

// 0x80103840
// Spawns the effect a1 at the object's position (+0x18/+0x1C/+0x20). Does nothing for a null object.
void PlayEffectTransient(void* a0, u32 a1) {
    if (a0 == 0) return;
    u8* obj = (u8*)a0;
    f32 pos[4];
    pos[0] = *(f32*)(obj + 0x18);
    pos[1] = *(f32*)(obj + 0x1C);
    pos[2] = *(f32*)(obj + 0x20);
    pos[3] = lbl_805FEBB8;
    SpawnEffectAt(lbl_8047C760, (void*)(uintptr_t)a1, pos, 0);
}

// 0x801068A8
// Registers five callbacks (fn_80106900, fn_80106920, fn_80106988, fn_801069B0, fn_80106A2C)
// with the 0x801DC57x/58x setters.
void fn_801068A8(void) {
    fn_801DC574((s32)(uintptr_t)fn_80106900);
    fn_801DC58C((s32)(uintptr_t)fn_80106920);
    fn_801DC57C((s32)(uintptr_t)fn_80106988);
    fn_801DC584((s32)(uintptr_t)fn_801069B0);
    fn_801DC594((s32)(uintptr_t)fn_80106A2C);
}

// 0x80106A2C
// Returns 1 for codes 0x2B, 0x33, 0x35-0x37, 0x3F-0x54 and 0x5B, otherwise 0.
s32 fn_80106A2C(s32 code) {
    if (code == 0x2B || code == 0x33) return 1;
    if (code >= 0x35 && code <= 0x37) return 1;
    if (code >= 0x3F && code <= 0x54) return 1;
    if (code == 0x5B) return 1;
    return 0;
}

// 0x80106B54
// Debug log of one global bit: "bit NAME[idx] = value".
void PrintBit(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF00(gGame + 0x5C8, idx);
    GameLog(1, 3, lbl_803698F0, lbl_805F8E90, idx, value);
}

// 0x80106BB4
// Debug log of one global int: idx and value.
void PrintInt(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF54(gGame + 0x5C8, idx);
    GameLog(1, 3, lbl_803698DC, lbl_805F8E90, idx, value);
}

// 0x80106C14
// Debug log of one global float: idx, then the value (returned in f1 and passed to the log).
void PrintFloat(u32 a0) {
    s32 idx = (s32)a0;
    f32 value = fn_801DFF9C(gGame + 0x5C8, idx);
    GameLog(1, 3, lbl_803698C8, lbl_805F8E90, idx, value);
}

// 0x80106D1C
// Same as PrintBit, for the level table (gGame + 0x6C8).
void Level_PrintBit(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF00(gGame + 0x6C8, idx);
    GameLog(1, 3, lbl_803698F0, lbl_805F8E90, idx, value);
}

// 0x80106D7C
// Same as PrintInt, for the level table (gGame + 0x6C8).
void Level_PrintInt(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF54(gGame + 0x6C8, idx);
    GameLog(1, 3, lbl_803698DC, lbl_805F8E90, idx, value);
}

// 0x80106DDC
// Same as PrintFloat, for the level table (gGame + 0x6C8).
void Level_PrintFloat(u32 a0) {
    s32 idx = (s32)a0;
    f32 value = fn_801DFF9C(gGame + 0x6C8, idx);
    GameLog(1, 3, lbl_803698C8, lbl_805F8E90, idx, value);
}

// 0x801070B0
// Code 0x1FF: fn_801DFE94 on the global table (gGame + 0x5C8) with the block at lbl_803C2A20 + 0x860.
// Code 0: fn_801DFE94 on the level table (gGame + 0x6C8) with gWorldState's pointer + 0xA4.
// Any other code does nothing. (Return value of fn_801DFE94 is not used.)
void fn_801070B0(s32 a0) {
    if (a0 == 0x1FF) {
        fn_801DFE94(gGame + 0x5C8, lbl_803C2A20 + 0x860);
        return;
    }
    if (a0 != 0) return;
    u8* ws = *(u8**)gWorldState;
    fn_801DFE94(gGame + 0x6C8, ws + 0xA4);
}

// 0x801088C0
// Creates an entity from the given arguments; returns the result of CreateEntityByType.
void* fn_801088C0(void* a0, u32 a1, void* a2) {
    return CreateEntityByType(a0, a1, a2);
}

// 0x80108FB4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80108FB4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80108FF4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80108FF4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80109034
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80109034(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80109074
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80109074(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x801090B4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_801090B4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x801090F4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_801090F4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80109134
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80109134(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}
}
