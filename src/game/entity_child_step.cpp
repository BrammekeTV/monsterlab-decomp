// Per-frame step of an entity's children, and the prop's step that runs it (slot 0x74 of the prop
// vtable lbl_80359180, called for every entity of a zone from WorldTick).
// Original addresses: 0x801B512C (entity base), 0x8006B9F4 (prop).
#include "types.h"
#include "engine/math.h"

extern "C" {
void QuatFromMtx(Quat* out, const f32 m[12]);   // 0x801456E4
void fn_801B4EA4(u8* arg0, u8* arg1);           // 0x801B4EA4
void fn_801B75AC(void* p);                      // 0x801B75AC

void fn_801B512C(u8* ent, f32 dt);

// 0x801B512C
// Steps the children of an entity (list at +0xB0: nodes {+0x04 entity, +0x08 next}). A child whose
// record flags (+0x20) have bit 0 or bit 1 set gets the frame time at +0x24 and its clock at +0x28
// advanced, and its node is refreshed (fn_801B75AC); with bit 0 set its own step (vtable +0x20)
// runs as well.
void fn_801B512C(u8* ent, f32 dt) {
    typedef void (*ChildStepFn)(u8* self, s32 arg, f32 dt);
    for (u8* node = *(u8**)(ent + 0xB0); node != NULL; node = *(u8**)(node + 0x8)) {
        u8* child = *(u8**)(node + 0x4);
        if (child == NULL) continue;
        if ((*(u32*)(*(u8**)(child + 0x4) + 0x20) & 3) == 0) continue;
        *(f32*)(child + 0x24) = dt;
        *(f32*)(child + 0x28) = *(f32*)(child + 0x28) + dt;
        fn_801B75AC(node);
        if (*(u32*)(*(u8**)(child + 0x4) + 0x20) & 1) {
            (*(ChildStepFn*)(*(u8**)child + 0x20))(child, 0, dt);
        }
    }
}

// 0x8006B9F4
// The prop's step: steps its children, then moves the entity the prop carries (+0x160) to the
// prop's entry number +0x164 of the 0xC8-byte entries at +0x98: the entry's position (+0x5C) goes
// into that entity's record, the rotation of the entry's matrix (+0x38) into the record's
// quaternion (+0x0C), and the entity is told that it moved (fn_801B4EA4).
void fn_8006B9F4(u8* prop, f32 dt) {
    fn_801B512C(prop, dt);
    u8* carried = *(u8**)(prop + 0x160);
    if (carried == NULL) return;
    if (*(s32*)(prop + 0x164) < 0) return;

    u8* entry = *(u8**)(prop + 0x98) + *(s32*)(prop + 0x164) * 0xC8;
    u8* record = *(u8**)(carried + 0x4);
    *(f32*)(record + 0x0) = *(f32*)(entry + 0x5C);
    *(f32*)(record + 0x4) = *(f32*)(entry + 0x60);
    *(f32*)(record + 0x8) = *(f32*)(entry + 0x64);

    entry = *(u8**)(prop + 0x98) + *(s32*)(prop + 0x164) * 0xC8;
    record = *(u8**)(*(u8**)(prop + 0x160) + 0x4);
    QuatFromMtx((Quat*)(record + 0xC), (const f32*)(entry + 0x38));
    fn_801B4EA4(*(u8**)(prop + 0x160), NULL);
}
}
