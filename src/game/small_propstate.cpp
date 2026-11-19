// Prop state step helpers (0x8006BDCC-0x8006BDE0).
#include "types.h"

extern "C" {
ScriptValue SetPropState(u8* prop, u32 state, s32 a2);  // 0x8006BB3C (draft_miss2.cpp)

// 0x8006BDCC
// Moves a prop to its next state (current state byte + 1).
ScriptValue IncrementPropState(u8* prop, s32 a2) {
    u8 cur = *(u8*)(*(u8**)(prop + 4) + 53);
    return SetPropState(prop, (u32)cur + 1u, a2);
}

// 0x8006BDE0
// Moves a prop to its previous state (current state byte - 1).
ScriptValue DecrementPropState(u8* prop, s32 a2) {
    u8 cur = *(u8*)(*(u8**)(prop + 4) + 53);
    return SetPropState(prop, (u32)cur - 1u, a2);
}
}

extern "C" {
void fn_801C4B90(u8* obj, f32* a, f32* b, s32 c, s32 d, u8* e);  // 0x801C4B90
// 0x80054AB4
void fn_80054AB4(u8* obj, f32* a, f32* b, s32 c, s32 d) {
    fn_801C4B90(obj, a, b, c, d, *(u8**)(*(u8**)obj + 4));
}
}

extern "C" {
void fn_8010C354(f32* out, u8* self, f32 t);  // 0x8010C354
// 0x800C4490
void fn_800C4490(f32* out, u8* a4, f32 t) {
    fn_8010C354(out, a4 + 0xD8, t);
}
}
