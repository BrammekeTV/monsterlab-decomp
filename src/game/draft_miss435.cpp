// Tail-call stubs in the audio/stream code (0x80201A94-0x8020A68C), skipped by the draft pipeline.
#include "types.h"
#include "revolution.h"

extern u8 gAudio[];          // audio manager
extern u8 lbl_8051DAD0[];    // 0x8051DAD0 (two music stream blocks, 0xC0 each; DVD info at +0x3C / +0xFC)

extern "C" {
void fn_801FF590(u8* audio, s32 zone);                      // 0x801FF590
void fn_80201AD8(u8* audio, u8* state);                     // 0x80201AD8
void fn_80201BD4(u8* audio, u8* state);                     // 0x80201BD4
s32  fn_802095B0(u8* arg0, void* arg1, void* arg2, void* arg3); // 0x802095B0
void fn_80235110(void* a, s32 b);                           // 0x80235110 (library)

// 0x80201A94
// DVD read callback: which of the two music stream blocks was the file info embedded in?
// Calls fn_80201AD8 for that block; ignores any other file.
void fn_80201A94(s32 result, u8* fileInfo) {
    u8* base = lbl_8051DAD0;
    if (fileInfo == base + 0x3C) {
        fn_80201AD8(gAudio, base);
    } else if (fileInfo == base + 0xFC) {
        fn_80201AD8(gAudio, base + 0xC0);
    }
}

// 0x80201B94
// DVD read callback (second variant): the file info is the stream block itself.
void fn_80201B94(s32 result, u8* fileInfo) {
    u8* base = lbl_8051DAD0;
    if (fileInfo == base) {
        fn_80201BD4(gAudio, base);
    } else if (fileInfo == base + 0xC0) {
        fn_80201BD4(gAudio, base + 0xC0);
    }
}

// 0x802062F0
// Forwards the current 0x30-byte record's pointer (+0x11C) and the word at +0x17C to fn_80235110.
void fn_802062F0(u8* p) {
    s32 idx = *(s32*)(p + 0x114);
    fn_80235110(*(void**)(p + idx * 0x30 + 0x11C), *(s32*)(p + 0x17C));
}

// 0x802097CC
// Calls the object's override function pointer (+0x54) if set, else the default fn_802095B0.
s32 fn_802097CC(u8* obj, f32* a, f32* b, f32* c, f32 t) {
    typedef s32 (*Override)(u8*, f32*, f32*, f32*, f32);
    Override fn = *(Override*)(obj + 0x54);
    if (fn) return fn(obj, a, b, c, t);
    return fn_802095B0(obj, a, b, c);
}

// 0x8020A68C
void fn_8020A68C(s32 zone) {
    fn_801FF590(gAudio, zone);
}
}
