// Follow-camera state: reset.
// Original: 0x800E02A8 (auto_03_800E02A8_text.s)
#include "types.h"

extern "C" {
void fn_800DE28C(u8* out, u8* obj, f32 up);    // 0x800DE28C (out = obj's 0x50 vector, z + up)
f32  fn_8033F1FC(f32 y, f32 x);                // 0x8033F1FC (MSL atan2)
extern u8 lbl_803C4298[];                      // 0x803C4298 (+8: the camera object)
}

// 0x800E02A8
// Slot 0x24 of the state object at camera+0x68, called from the camera reset (fn_800DBEF0) with
// the eye position. The look-at point is the camera's target position raised by 1.3; it goes to
// 0x24 and 0x30, the eye to 0x18, and 0x4C is the heading from the eye to the look-at point
// (atan2(dy, dx)). The rest of the state is set to its defaults (0x68 = 0.5235988, 30 degrees;
// 0x6C and 0x70 = 12.5; 0x48 = 1.3; 0xC0 = 1.0).
extern "C" void fn_800E02A8(u8* st, const f32* eye) {
    f32 at[3];
    fn_800DE28C((u8*)at, *(u8**)(lbl_803C4298 + 0x8), 1.3f);
    *(f32*)(st + 0x24) = at[0];
    *(f32*)(st + 0x28) = at[1];
    *(f32*)(st + 0x2C) = at[2];
    *(f32*)(st + 0x30) = at[0];
    *(f32*)(st + 0x34) = at[1];
    *(f32*)(st + 0x38) = at[2];
    f32 heading = fn_8033F1FC(at[1] - eye[1], at[0] - eye[0]);
    *(f32*)(st + 0x4C) = heading;
    *(f32*)(st + 0x68) = 0.5235988f;
    *(f32*)(st + 0x18) = eye[0];
    *(f32*)(st + 0x1C) = eye[1];
    *(f32*)(st + 0x20) = eye[2];
    *(s32*)(st + 0xB0) = 0;
    *(f32*)(st + 0xB4) = 0.0f;
    *(u8*)(st + 0x78)  = 0;
    *(u8*)(st + 0x94)  = 0;
    *(u8*)(st + 0x95)  = 0;
    *(f32*)(st + 0x3C) = 0.0f;
    *(f32*)(st + 0x40) = 0.0f;
    *(f32*)(st + 0x44) = 0.0f;
    *(f32*)(st + 0x6C) = 12.5f;
    *(f32*)(st + 0x70) = 12.5f;
    *(f32*)(st + 0x74) = 0.0f;
    *(f32*)(st + 0x48) = 1.3f;
    *(s32*)(st + 0xD4) = 0;
    *(u8*)(st + 0x60)  = 0;
    *(f32*)(st + 0x50) = 0.0f;
    *(s32*)(st + 0x54) = 0;
    *(f32*)(st + 0xB8) = 0.0f;
    *(f32*)(st + 0xBC) = 0.0f;
    *(f32*)(st + 0xC0) = 1.0f;
    *(f32*)(st + 0xC4) = 0.0f;
    *(f32*)(st + 0xC8) = 0.0f;
    *(f32*)(st + 0xCC) = 0.0f;
    *(f32*)(st + 0xD0) = 0.0f;
}
