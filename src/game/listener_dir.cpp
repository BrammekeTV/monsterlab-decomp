// The direction the current camera object faces, for the audio listener.
// Original: 0x800DB888, 0x800DB8D4 (auto_03_800D9AA4_text.s)
#include "types.h"
#include "engine/math.h"

struct Quat;
extern "C" void QuatGetAxisXY(Quat* q, Vec3* out);   // 0x801453BC
extern u8 lbl_803C4298[];               // 0x803C4298 (+4: the object whose +4 points at the transform)

// 0x800DB888
// Writes the axis from the orientation at +0xC of the object's transform; returns 0 (and writes
// nothing) while there is no object.
extern "C" s32 fn_800DB888(Vec3* out) {
    u8* obj = *(u8**)(lbl_803C4298 + 4);
    if (obj == NULL) {
        return 0;
    }
    QuatGetAxisXY((Quat*)(*(u8**)(obj + 4) + 0xC), out);
    return 1;
}

extern "C" void fn_801455E0(f32* out, u8* quat);     // 0x801455E0: an axis of a quaternion's rotation

// 0x800DB8D4
// The second axis of the same orientation (the listener's "up"); returns 0 while there is no object.
extern "C" s32 fn_800DB8D4(Vec3* out) {
    u8* obj = *(u8**)(lbl_803C4298 + 4);
    if (obj == NULL) {
        return 0;
    }
    f32 axis[3];
    fn_801455E0(axis, *(u8**)(obj + 4) + 0xC);
    out->x = axis[0];
    out->y = axis[1];
    out->z = axis[2];
    return 1;
}
