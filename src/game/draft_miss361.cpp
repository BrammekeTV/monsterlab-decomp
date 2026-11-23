// Drafts miss361: vector helpers (0x801C9000, 0x801CEC30, 0x801D0084, 0x801D0114), a color
// alpha scale (0x801E3CE0), audio voice-state update (0x80201AD8), the audio manager
// constructor (0x802080B8) and a record release/teardown (0x80185A64).
#include "types.h"

extern "C" {
u32  RandomU32(void);
void fn_80182ABC(u8* p);
void fn_80183CFC(u8* p);
void fn_80183E88(u8* p);
void fn_80184C18(u8* p, s32 b);
void fn_80185988(u8* p);
void fn_80171228(s32 a, s32 b, s32 c, f32 scale);
s32  AXSetVoiceAdpcmLoop(u8* voice, u16* p);
s32  fn_801D54B0(u8* p, s32 flags);

extern u8  lbl_80412B20[];   // static record
extern u8  lbl_80556AE0[];
extern u8  lbl_803874F4[];   // vtable of the audio manager
extern u8* lbl_805FB608;     // global object pointer
extern s32 lbl_805FBAE0;     // audio busy flag
extern s32 lbl_805FBB30;
extern u8  lbl_805FBB60;
extern f32 lbl_805FFD54;     // 1.0f
extern f32 lbl_805FFE38;     // lower bound for the zero test
extern f32 lbl_805FFE40;     // RandomU32 scale
extern f32 lbl_805FFE50;     // upper bound for the zero test
}

extern "C" {

// 0x801C9000
// Builds a normalized direction from two points (arg0 = bounds, farg0 = scale): arg1 receives
// the difference vector, then the same vector divided by farg0 * arg0[0].
void fn_801C9000(u8* arg0, u8* arg1, f32 farg0) {
    f32 d0 = *(f32*)(arg0 + 0x20) - *(f32*)(arg0 + 0x10);
    f32 d1 = *(f32*)(arg0 + 0x24) - *(f32*)(arg0 + 0x14);
    f32 d2 = *(f32*)(arg0 + 0x28) - *(f32*)(arg0 + 0x18);
    f32 d3 = *(f32*)(arg0 + 0x2C) - *(f32*)(arg0 + 0x1C);
    f32 s;

    *(f32*)(arg1 + 0x0) = d0;
    *(f32*)(arg1 + 0x4) = d1;
    *(f32*)(arg1 + 0x8) = d2;
    *(f32*)(arg1 + 0xC) = d3;
    s = lbl_805FFD54 / (farg0 * *(f32*)(arg0 + 0x0));
    *(f32*)(arg1 + 0x0) = d0 * s;
    *(f32*)(arg1 + 0x4) = d1 * s;
    *(f32*)(arg1 + 0x8) = d2 * s;
    *(f32*)(arg1 + 0xC) = d3 * s;
}

// 0x801CEC30
// Scales the four floats of v by the float *s.
void fn_801CEC30(u8* v, u8* s) {
    f32 k = *(f32*)s;

    *(f32*)(v + 0x0) = *(f32*)(v + 0x0) * k;
    *(f32*)(v + 0x4) = *(f32*)(v + 0x4) * k;
    *(f32*)(v + 0x8) = *(f32*)(v + 0x8) * k;
    *(f32*)(v + 0xC) = *(f32*)(v + 0xC) * k;
}

// 0x801D0084
// Returns 1 if all four floats of v are strictly inside (lbl_805FFE38, lbl_805FFE50), i.e. near zero.
s32 fn_801D0084(u8* v) {
    s32 i;

    for (i = 0; i < 4; i++) {
        f32 x = *(f32*)(v + i * 4);
        if (x <= lbl_805FFE38 || x >= lbl_805FFE50) {
            return 0;
        }
    }
    return 1;
}

// 0x801D0114
// Fills the three floats of v with random values in [lo, hi).
void fn_801D0114(u8* v, f32 lo, f32 hi) {
    *(f32*)(v + 0x0) = lo + ((hi - lo) * (lbl_805FFE40 * (f32)RandomU32()));
    *(f32*)(v + 0x4) = lo + ((hi - lo) * (lbl_805FFE40 * (f32)RandomU32()));
    *(f32*)(v + 0x8) = lo + ((hi - lo) * (lbl_805FFE40 * (f32)RandomU32()));
}

// 0x801E3CE0
// Scales the alpha byte of color by (a + 1) / 256; the RGB bytes are kept.
u32 fn_801E3CE0(u32 color, s32 a) {
    u32 alpha = (color >> 24) * (u32)(a + 1);

    return ((alpha << 16) & 0xFF000000u) | (color & 0xFFFFFFu);
}

// 0x80201AD8
// Audio voice update: while the busy flag is set, pushes the current sample byte of the
// state's source into the voice (when the state is in mode 2 and not looping), then clears
// the pending flags of the state.
void fn_80201AD8(u8* audio, u8* state) {
    u16 params[3];
    s32 saved;
    u8* src;

    saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    src = *(u8**)(state + 0x98);
    if (src == 0 && *(s32*)(state + 0xA0) == 2) {
        *(s32*)(state + 0xB8) = 0;
        lbl_805FBAE0 = saved;
        return;
    }
    if (*(s32*)(state + 0xA0) == 2 && *(s32*)(state + 0xA4) == 0 && *(u8**)(state + 0x90) != 0) {
        params[0] = *src;
        params[1] = 0;
        params[2] = 0;
        AXSetVoiceAdpcmLoop(*(u8**)(state + 0x90), params);
    }
    *(s32*)(state + 0xBC) = 1;
    *(s32*)(state + 0xB8) = 0;
    lbl_805FBAE0 = saved;
}

// 0x802080B8
// Audio manager constructor: installs the vtable, clears the state, runs the base
// initializer (vtable slot 0x60) and sets up the global audio flags. Returns this.
void* fn_802080B8(void* self) {
    u8* p = (u8*)self;

    *(u8*)(p + 0xA0) = 0;
    *(void**)p = lbl_803874F4;
    *(s32*)(p + 0xA4) = 0;
    (*(void (**)(void*))(lbl_803874F4 + 0x60))(self);
    lbl_805FBB30 = 0;
    if (lbl_805FBB60 == 0 && fn_801D54B0(lbl_80556AE0, 0x80000) != 0) {
        lbl_805FBB60 = 1;
    }
    return self;
}

// 0x80185A64
// Tears down a record: releases each of its 0x20-byte entries, calls the global object's
// hook with the record's pointer, and clears the record slots.
void fn_80185A64(u8* arg0, s32 arg1) {
    s32 temp_r31;
    s32 i;
    u8* item;

    temp_r31 = *(s32*)(lbl_80412B20 + 0x30);
    if (temp_r31 == 0) {
        fn_80182ABC(lbl_80412B20);
    }
    fn_80184C18(arg0, arg1);
    fn_80185988(arg0);
    item = *(u8**)(arg0 + 0x5C);
    for (i = 0; i < *(s32*)(arg0 + 0x68); i++) {
        fn_80171228(*(s32*)(item + 0x10), *(s32*)(item + 0x18), *(s32*)(arg0 + 0x58), *(f32*)(item + 0x14));
        item += 0x20;
    }
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x10))(*(u8**)(arg0 + 0x5C), lbl_805FB608);
    *(s32*)(arg0 + 0x5C) = 0;
    *(s32*)(arg0 + 0x60) = 0;
    *(s32*)(arg0 + 0x64) = 0;
    if (temp_r31 == 0) {
        fn_80183CFC(lbl_80412B20);
        fn_80183E88(lbl_80412B20);
    }
}

}  // extern "C"
