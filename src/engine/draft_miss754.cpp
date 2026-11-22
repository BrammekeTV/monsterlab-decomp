// Small functions in 0x80176xxx-0x80178xxx (character state flags, GX matrix load, record create, id maps).
#include "types.h"
#include "revolution.h"

extern "C" {
void fn_801785A8(void* p);               // 0x801785A8
void fn_801787E0(u8* p, u8** stream);        // 0x801787E0
}

extern u8* lbl_805FB608;                 // 0x805FB608 (heap object: slot +0 = alloc, called as (..., heap))
extern f32 lbl_805FF6F8;                 // 0x805FF6F8 (threshold, 1.0f)
extern f32 lbl_805FF6FC;                 // 0x805FF6FC (0.0f)

extern "C" {

// 0x80176D18
// Picks the animation class byte at +0xD of the owner record from the state at +0x54 and the
// owner's byte +0x34, and sets the flag byte at +0xE (moving and byte +5 set).
void fn_80176D18(u8* arg0) {
    u8* owner = *(u8**)(arg0 + 0x0);
    u8* sub = *(u8**)(owner + 0x1C);
    s32 v = 0;
    if (sub != NULL) v = *(u8*)(sub + 0x34);
    s32 state = *(s32*)(arg0 + 0x54);
    s8 cls;
    if (state == 7) {
        cls = (v != 0) ? 2 : 1;
    } else if (state == 0) {
        if (*(f32*)(arg0 + 0x4C) < lbl_805FF6F8) {
            cls = 3;
        } else if (v == 1) {
            cls = 2;
        } else if (v == 2) {
            cls = 3;
        } else {
            cls = 1;
        }
    } else {
        cls = 3;
    }
    *(s8*)(owner + 0xD) = cls;
    f32 x = *(f32*)(arg0 + 0x38);
    f32 y = *(f32*)(arg0 + 0x3C);
    f32 z = *(f32*)(arg0 + 0x40);
    s32 zero = 0;
    if (lbl_805FF6FC == x && lbl_805FF6FC == y && lbl_805FF6FC == z) zero = 1;
    u8 flag = *(u8*)(arg0 + 0x5);
    *(u8*)(*(u8**)(arg0 + 0x0) + 0xE) = (u8)((zero == 0) && (flag != 0));
}

// 0x801776C8
// Loads the 3x4 matrix at arg0 (row-major, 12 floats) into XF memory at 0x500 (transposed order).
void fn_801776C8(f32* arg0) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0xB;
    GXWGFifo._u16 = 0x500;
    GXWGFifo._f32 = arg0[0];
    GXWGFifo._f32 = arg0[3];
    GXWGFifo._f32 = arg0[6];
    GXWGFifo._f32 = arg0[9];
    GXWGFifo._f32 = arg0[1];
    GXWGFifo._f32 = arg0[4];
    GXWGFifo._f32 = arg0[7];
    GXWGFifo._f32 = arg0[10];
    GXWGFifo._f32 = arg0[2];
    GXWGFifo._f32 = arg0[5];
    GXWGFifo._f32 = arg0[8];
    GXWGFifo._f32 = arg0[11];
}

// 0x80178778
// Allocates a 0x13C-byte record from the heap object (parent link = arg0 + 0x80 when set),
// stores the owner at +0x50 and initialises it.
u8* fn_80178778(u8* arg0) {
    u8* link = (arg0 != NULL) ? arg0 + 0x80 : NULL;
    u8* rec = (*(u8* (**)(u32, u32, u32, u8*, u32, u8*))lbl_805FB608)(0x13C, 0x10, 0x12, link, 0, lbl_805FB608);
    *(u8**)(rec + 0x50) = arg0;
    fn_801785A8(rec);
    return rec;
}

// 0x80178C44
// Same as fn_80178778, then applies arg1 through fn_801787E0.
u8* fn_80178C44(u8* arg0, u8** arg1) {
    u8* link = (arg0 != NULL) ? arg0 + 0x80 : NULL;
    u8* rec = (*(u8* (**)(u32, u32, u32, u8*, u32, u8*))lbl_805FB608)(0x13C, 0x10, 0x12, link, 0, lbl_805FB608);
    *(u8**)(rec + 0x50) = arg0;
    fn_801785A8(rec);
    fn_801787E0(rec, arg1);
    return rec;
}

// 0x80178CC4
// Maps an index 0..7 to a 16-bit id (default 0x8006); the inverse of fn_80178D2C.
u16 fn_80178CC4(u32 arg0) {
    u32 r = 0x8006;
    switch (arg0) {
    case 0: r = 0x8006; break;
    case 1: r = 0x8007; break;
    case 2: r = 0x8008; break;
    case 3: r = 0x800A; break;
    case 4: r = 0x800B; break;
    case 5: r = 0xF005; break;
    case 6: r = 0xF006; break;
    case 7: r = 0xF007; break;
    default: break;
    }
    return (u16)r;
}

// 0x80178D2C
// Maps a 16-bit id to an index 0..7 (default 0).
s32 fn_80178D2C(s32 arg0) {
    s32 r = 0;
    switch (arg0) {
    case 0x8006: r = 0; break;
    case 0x8007: r = 1; break;
    case 0x8008: r = 2; break;
    case 0x800A: r = 3; break;
    case 0x800B: r = 4; break;
    case 0xF005: r = 5; break;
    case 0xF006: r = 6; break;
    case 0xF007: r = 7; break;
    default: break;
    }
    return r;
}

// 0x80178DEC
// Maps an index 0..14 to a 16-bit id (default 0).
u16 fn_80178DEC(u32 arg0) {
    u32 r = 0;
    switch (arg0) {
    case 0: r = 0; break;
    case 1: r = 1; break;
    case 2: r = 0x300; break;
    case 3: r = 0x301; break;
    case 4: r = 0x302; break;
    case 5: r = 0x303; break;
    case 6: r = 0x304; break;
    case 7: r = 0x305; break;
    case 8: r = 0x306; break;
    case 9: r = 0x307; break;
    case 10: r = 0x308; break;
    case 11: r = 0x8001; break;
    case 12: r = 0x8002; break;
    case 13: r = 0x8003; break;
    case 14: r = 0x8004; break;
    default: break;
    }
    return (u16)r;
}

}  // extern "C"
