// Small tail-call wrappers that the draft pipeline skipped (jump-through / forwarding stubs).
#include <stdarg.h>
#include <stdio.h>
#include "types.h"
#include "revolution.h"

extern u8  lbl_803C4298[];   // 0x803C4298 (level info block; +4 = camera controller)
extern u8* lbl_805FB608;     // 0x805FB608 (global object pointer)
extern void* lbl_805FB704;   // 0x805FB704 (table of 0x10-byte records; +8 = array of string pointers)
extern u8  lbl_80412BC0[];   // 0x80412BC0 (static table)
extern s32 lbl_805FB628;     // 0x805FB628
extern s32 lbl_805FB62C;     // 0x805FB62C
extern s32 lbl_805FB630;     // 0x805FB630

extern "C" {
void fn_80187798(u8* table, const char* text, s32 len, s32 a, s32 b, s32 c); // 0x80187798
void fn_800558BC(u8* p);                                                            // 0x800558BC
void fn_80061A98(u8* p, s32* a, s32 v);                                             // 0x80061A98
s32  fn_8008CE44(u8* arg0, const f32* pos, s32 kind, f32 a, f32 b);                 // 0x8008CE44
void fn_8016ACB8(u8* dst, s32 dstStart, u8* srcA, s32 aStart, u8* srcB, s32 bStart, s32 maxCount, f32 t); // 0x8016ACB8
}

extern "C" {

// 0x8005E11C
// Forwards to fn_800558BC (tail call).
void fn_8005E11C(u8* p) {
    fn_800558BC(p);
}

// 0x8008E8A0
// Starts a brazier symbol rising: tail-calls fn_8008CE44 with the effect kind (+0x268), the
// position (+0x2B8) and the two floats at +0x29C / +0x2A8. The asm also loads +0x26C into r6,
// which the callee does not take.
s32 BrazierSymbolStartRising(u8* p) {
    return fn_8008CE44(p, (const f32*)(p + 0x2B8), *(s32*)(p + 0x268), *(f32*)(p + 0x29C), *(f32*)(p + 0x2A8));
}

// 0x800E7868
// Tail-calls slot 0x94 of the camera controller (level info block +4) with the controller as argument.
void WarpCamera(void) {
    u8* cam = *(u8**)(lbl_803C4298 + 0x4);
    (*(void (**)(u8*))(*(u8**)cam + 0x94))(cam);
}

// 0x801036E4
// Copies the first word of a1 to a local and passes it to fn_80061A98. The asm leaves the third
// argument (r5) as whatever the caller had in it; 0 here.
void JumpTo(u8* a0, s32* a1) {
    s32 copy = *a1;
    fn_80061A98(a0, &copy, 0);
}

// 0x8016AFE8
// Tail-calls fn_8016ACB8 (copy/blend of a pose range): dst = a, both sources start at 0, count is
// the smallest of a's count (+0xC) and the two sources' counts (+0x10).
s32 fn_8016AFE8(u8* a, u8* b, u8* c, f32 t) {
    s32 n = *(s32*)(a + 0xC);
    if (*(s32*)(b + 0x10) < n) n = *(s32*)(b + 0x10);
    if (*(s32*)(c + 0x10) < n) n = *(s32*)(c + 0x10);
    fn_8016ACB8(a, 0, b, 0, c, 0, n, t);
    return 0;
}

// 0x8017385C
// Clears +0x60 of the record, then tail-calls the free function at +4 of the heap object (lbl_805FB608).
void fn_8017385C(void* p) {
    *(s32*)((u8*)p + 0x60) = 0;
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(p, lbl_805FB608);
}

// 0x80184268
// Tail-calls the free function at +4 of the heap object (lbl_805FB608) with the record and the object.
void fn_80184268(void* p) {
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(p, lbl_805FB608);
}

// 0x801C6618
// String lookup: entry b of the string array of record a in the table at lbl_805FB704.
const char* fn_801C6618(s32 a, s32 b) {
    u8* rec = (u8*)lbl_805FB704 + a * 0x10;
    return ((const char**)*(void**)(rec + 0x8))[b];
}

// 0x80188F00
// printf-style debug text: formats into a 0x100-byte buffer with vsprintf and hands the text and
// its length plus three state words to fn_80187798.
void fn_80188F00(const char* fmt, ...) {
    char buf[0x100];
    va_list va;
    va_start(va, fmt);
    s32 n = vsprintf(buf, fmt, va);
    va_end(va);
    fn_80187798(lbl_80412BC0, buf, n, lbl_805FB628, lbl_805FB62C, lbl_805FB630);
}

}
