// Small thunks and globals: fader colour reset, pointer table lookup, global-object registration,
// vtable and SDK tail calls. Verified against the asm (sh tools/dump.sh).
#include <string.h>
#include "types.h"
#include "game/node_ref.h"

extern "C" {
extern f32 lbl_805FFEA8;             // 0x805FFEA8 (float constant written to the fader slots)
extern u8  lbl_8047D248[];           // 0x8047D248 (fader colour: 4 floats)
extern u8  lbl_8048A8A8[];           // 0x8048A8A8 (__register_global_object handle)
extern u8  lbl_8048A8B4[];           // 0x8048A8B4 (disc request queue)
extern u8  lbl_8048A8E0[];           // 0x8048A8E0 (thread block)
extern u8  lbl_80388D70[];           // 0x80388D70 (table of 4-byte pointers)
extern u8  gIdPool[];                // 0x8048A8F0
void       __register_global_object(void* obj, void (*dtor)(void*), void* handle);  // 0x8032EFC8
u8*        fn_801E54A4(u8* p, s32 del);  // 0x801E54A4
u8*        fn_801E73E4(u8* p, s32 del);  // 0x801E73E4
void*      fn_80330F50(u8* a0, u32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7);  // 0x80330F50
}

extern "C" {

// 0x801D6694
// Sets the four floats of the fader colour block (lbl_8047D248) to the constant lbl_805FFEA8.
void fn_801D6694(void) {
    f32 c = lbl_805FFEA8;
    *(f32*)(lbl_8047D248 + 0x0) = c;
    *(f32*)(lbl_8047D248 + 0x4) = c;
    *(f32*)(lbl_8047D248 + 0x8) = c;
    *(f32*)(lbl_8047D248 + 0xC) = c;
}

// 0x801ED34C
// Writes the 4-bit value arg1 into the high nibble of the byte at arg0, unless bit 0x80 of that byte is set.
void fn_801ED34C(u8* arg0, s32 arg1) {
    // The reference word is read and written as a whole so the code does not depend on byte order.
    u8 temp_r5 = NodeRefByte0(NodeRefWord(arg0));
    if (!((temp_r5 >> 4U) & 8)) {
        NodeRefSetByte0(arg0, temp_r5 | ((arg1 * 0x10) & 0xF0));
    }
}

// 0x801E5488
// Registers the global object at lbl_8048A8B4 with its destructor fn_801E54A4 (tail call).
void fn_801E5488(void) {
    __register_global_object(lbl_8048A8B4, (void (*)(void*))fn_801E54A4, lbl_8048A8A8);
}

// 0x801E73C8
// Registers the global object gIdPool with its destructor fn_801E73E4 (tail call).
void fn_801E73C8(void) {
    __register_global_object(gIdPool, (void (*)(void*))fn_801E73E4, lbl_8048A8E0);
}

// 0x8020EA20
// Returns entry arg1 of the pointer table at lbl_80388D70 (4-byte entries).
void* fn_8020EA20(s32 arg1) {
    return *(void**)(lbl_80388D70 + arg1 * 4);
}

// 0x801ACF80
// Vtable thunk: calls slot 0x10 of the object's vtable with all arguments passed on (tail jump).
u8* fn_801ACF80(u8* obj, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7) {
    typedef u8* (*Fn)(u8*, s32, s32, s32, s32, s32, s32, s32);
    return (*(Fn*)(*(u8**)obj + 0x10))(obj, a1, a2, a3, a4, a5, a6, a7);
}

// 0x801EE140
// Tail call to fn_80330F50 with the caller's arguments passed on unchanged.
void* fn_801EE140(u8* a0, u32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7) {
    return fn_80330F50(a0, a1, a2, a3, a4, a5, a6, a7);
}

// 0x80221280
// SDK wrapper: tail call to strncpy.
char* StrNCopy(char* dst, const char* src, size_t n) {
    return strncpy(dst, src, n);
}

}  // extern "C"
