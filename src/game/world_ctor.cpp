// World object (gWorldState): constructor.
// Original: 0x801268C8
#include "types.h"

extern "C" {
void ZoneSlotInit(void* slot);     // 0x801F2B40
void fn_80132094(void);            // 0x80132094 (empty in this build; the original passes the world)
void fn_80132BC4(u8* world);       // 0x80132BC4
}
extern "C++" u8 lbl_805FB210;      // 0x805FB210 (flag byte)

// 0x801268C8
// Run from the static initialiser fn_8012F64C. Clears the three zone slots (0x30 bytes each from
// +0x20), the id list header at +0x1B0 (empty: 0, -1, -1, room for 0x10 entries stored at +0x1C4),
// the first word of each 0xC-byte record from +0x2A0 to +0x3294, and the flags and timers at
// +0x4..+0xC. The original takes the world only and returns it; the prototype used by its caller
// (draft_miss521.cpp) has two more parameters, which are not read.
extern "C" void fn_801268C8(u8* world, u8* data, f32 v) {
    (void)data;
    (void)v;
    for (u8* slot = world + 0x20; slot < world + 0xB0; slot += 0x30) {
        *(s32*)slot = 0;
        ZoneSlotInit(slot);
    }
    *(u8**)(world + 0x1C0) = world + 0x1C4;
    *(s32*)(world + 0x1BC) = 0x10;
    *(s32*)(world + 0x1B8) = -1;
    *(s32*)(world + 0x1B4) = -1;
    *(s32*)(world + 0x1B0) = 0;
    *(s32*)(world + 0x294) = 0;
    for (u8* rec = world + 0x2A0; rec < world + 0x3294; rec += 0xC) {
        *(s32*)rec = 0;
    }
    world[4] = 0;
    world[5] = 0;
    *(f32*)(world + 0x8) = 0.0f;
    *(f32*)(world + 0xC) = 0.0f;
    lbl_805FB210 = 0;
    *(s32*)(world + 0x114) = 0;
    fn_80132094();
    fn_80132BC4(world);
}
