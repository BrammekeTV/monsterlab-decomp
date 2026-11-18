// Functions in 0x8002xxxx: input-source set update, experiment timer object ctor/dtor/timer UI.
#include "types.h"

extern "C" {

void   AudioSetPaused(void* audio, int paused);                  // 0x801FF708 (provisional)
u32    RandomU32(void);
void   SaveRecordBestTime(u8* s, s32 i, u32 t);                  // 0x80225C1C
void*  ScreenSetFindElement(void* set, u32 hash);
void*  ScreenSetsCurrent(void* sets);
u32    StringHash(const char* s);
int    SysSprintf(char* buf, const char* fmt, ...);
void   UISetText(void* el, const char* text, s32 a);
s32    WorldGetPhase(void* world);                               // 0x800D83A8
void   fn_8001FCA8(u8* p);                                       // 0x8001FCA8
void   fn_800D83B8(u8* a3, s32 a4);                              // 0x800D83B8
u8*    fn_80074CC8(u8* p, s32 a);                                // 0x80074CC8 (base ctor)
u8*    fn_800E4D7C(u32 a0);                                      // 0x800E4D7C
void   fn_800E4C30(u8* p);                                      // 0x800E4C30
void   fn_800E4C70(u8* p);                                      // 0x800E4C70
s32    fn_801C79A4(u8* p);                                       // 0x801C79A4
void   fn_801C79E4(u8* p, s32 a, u8* b);                         // 0x801C79E4
void*  fn_80198F1C(void* set, u32 hash);                         // 0x80198F1C
void fn_8019BBA4(void* el, void* anim, s32 level);                             // 0x8019BBA4
void   operator_delete(void* p);                                 // 0x801D426C

extern u8   gAudio[];
extern u8   gGame[];
extern "C++" void* gScreenSets;
extern u8*  gWorldState;                // first word is a pointer to the world block
extern u8   lbl_80355940[];             // 0x80355940 (4 ints, reset to -1)
extern u8   lbl_80355970[];             // 0x80355970
extern u8   lbl_8047C760[];             // 0x8047C760 (effect manager)
extern f32  lbl_805FC8C8;               // 0x805FC8C8 (0.0f)
extern f32  lbl_805FC8CC;               // 0x805FC8CC (1.0f)
extern f32  lbl_805FC8E0;               // 0x805FC8E0 (float constant)
extern f32  lbl_805FC8F0;               // 0x805FC8F0
extern f32  lbl_805FC8F4;               // 0x805FC8F4
extern f32  lbl_805FC8F8;               // 0x805FC8F8
extern s32  lbl_805FAF90;               // 0x805FAF90
extern s32  lbl_805FAF94;               // 0x805FAF94 (countdown)
extern u8   lbl_805FAF98;               // 0x805FAF98
extern u8   lbl_805FAF99;               // 0x805FAF99
extern u8   lbl_805FAF9A;               // 0x805FAF9A
extern u8   lbl_805FAF9B;               // 0x805FAF9B
extern s32  lbl_805FAF9C;               // 0x805FAF9C
extern s32  lbl_805FAFA0;               // 0x805FAFA0
extern s32  lbl_805FAFA4;               // 0x805FAFA4
extern s32  lbl_805FAFA8;               // 0x805FAFA8
extern u8*  lbl_805FB134;               // 0x805FB134 (experiment object)
extern u8*  lbl_805FBBE0;               // 0x805FBBE0 (save block pointer)

typedef s32 (*VFn2)(void* obj, s32 a);

// 0x8002129C
// Updates every input source in the set; flag at +4 becomes 1 when one went from 0 to positive.
void fn_8002129C(u8* p) {
    f32 zero = lbl_805FC8C8;
    f32 one = lbl_805FC8CC;
    *(f32*)(p + 0x4) = zero;
    for (s32 i = 0; i < *(s32*)(p + 0x8); i++) {
        fn_8001FCA8(p + 0xC + i * 0xC);
        if (zero == *(f32*)(p + 0x50 + i * 4) && *(f32*)(p + 0x10 + i * 0xC) > zero) {
            *(f32*)(p + 0x4) = one;
        }
        *(f32*)(p + 0x50 + i * 4) = *(f32*)(p + 0x10 + i * 0xC);
    }
}

// 0x800213A4
// Updates every input source; +0 is the product of their values, +4 toggles on a rising edge.
void fn_800213A4(u8* p) {
    *(f32*)(p + 0x0) = lbl_805FC8CC;
    for (s32 i = 0; i < *(s32*)(p + 0x8); i++) {
        fn_8001FCA8(p + 0xC + i * 0xC);
        *(f32*)(p + 0x0) = *(f32*)(p + 0x0) * *(f32*)(p + 0x10 + i * 0xC);
    }
    f32 zero = lbl_805FC8C8;
    if (zero != *(f32*)(p + 0x0) && zero == *(f32*)(p + 0x50)) {
        if (zero == *(f32*)(p + 0x4)) {
            *(f32*)(p + 0x4) = lbl_805FC8CC;
        } else {
            *(f32*)(p + 0x4) = zero;
        }
    }
    *(f32*)(p + 0x50) = *(f32*)(p + 0x0);
}

// 0x80021594
// Experiment timer constructor: base ctor, clear fields and globals, init effect manager link.
u8* fn_80021594(u8* p, s32 a1, s32 a2) {
    fn_80074CC8(p, 0);
    *(s32*)(p + 0x7C) = a1;
    *(u8**)(p + 0x4) = lbl_80355970;
    *(s32*)(p + 0x68) = 0;
    f32 c = lbl_805FC8E0;
    *(f32*)(p + 0x6C) = c;
    *(f32*)(p + 0x70) = c;
    *(u8*)(p + 0x80) = 0;
    *(u8*)(p + 0x81) = 0;
    *(u8*)(p + 0x82) = 0;
    *(s32*)(p + 0x84) = 0;
    lbl_805FAF90 = 0;
    lbl_805FAF94 = 0;
    lbl_805FAF98 = 0;
    lbl_805FAF99 = 0;
    lbl_805FAF9A = 0;
    lbl_805FAF9C = a2;
    lbl_805FAFA0 = 0;
    lbl_805FAFA4 = 0;
    // asm passes the leftover r3 (address of lbl_80355970) as the id; not representable on x64
    u8* prop = fn_800E4D7C(0);
    if (prop != 0) {
        fn_800E4C30(prop);
    }
    *(s32*)(p + 0x8C) = *(s32*)(lbl_8047C760 + 0x24);
    *(s32*)(p + 0x88) = fn_801C79A4(lbl_8047C760);
    *(s32*)(lbl_80355940 + 0x0) = -1;
    *(s32*)(lbl_80355940 + 0x4) = -1;
    *(s32*)(lbl_80355940 + 0x8) = -1;
    *(s32*)(lbl_80355940 + 0xC) = -1;
    return p;
}

// 0x800216B8
// Experiment timer deleting destructor: restores the effect manager state, frees if del > 0.
u8* fn_800216B8(u8* p, s32 del) {
    if (p != 0) {
        *(s32*)(p + 0x68) = 0;
        *(u8**)(p + 0x4) = lbl_80355970;
        fn_801C79E4(lbl_8047C760, *(s32*)(p + 0x88), lbl_80355970);
        *(s32*)(lbl_8047C760 + 0x24) = *(s32*)(p + 0x8C);
        // asm passes the leftover r3 (address of lbl_8047C760) as the id; not representable on x64
        u8* prop = fn_800E4D7C(0);
        if (prop != 0) {
            fn_800E4C70(prop);
        }
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80021BE4
// Ends the experiment: unpauses audio, destroys the experiment object, and in phase 4 swaps
// the two save-block slots at +0x108/+0x10C.
void fn_80021BE4(void) {
    if (lbl_805FB134 != 0) {
        AudioSetPaused(gAudio, 0);
        if (lbl_805FB134 != 0) {
            u8* obj = lbl_805FB134;
            (*(VFn2*)(*(u8**)(obj + 0x4) + 0x8))(obj, 1);
        }
        lbl_805FB134 = 0;
        if (WorldGetPhase(gGame + 0xD5F0) == 4) {
            fn_800D83B8(gGame + 0xD5F0, *(s32*)(gWorldState + 0x1E0));
            u8* sb = lbl_805FBBE0;
            s32 a = *(s32*)(sb + 0x108);
            s32 b = *(s32*)(sb + 0x10C);
            *(s32*)(sb + 0x10C) = a;
            *(s32*)(sb + 0x108) = b;
            *(s32*)(sb + 0x104) = -1;
            if (a == -1) {
                *(s32*)(sb + 0x10C) = *(s32*)(sb + 0x108);
            }
        }
    }
}

// 0x80021CAC
// Per-tick countdown: decrement when above farg0; at <= 5 pulses the timer group once.
void fn_80021CAC(f32 farg0) {
    s32 t = lbl_805FAF94;
    if ((f32)t > farg0) {
        t -= 1;
        lbl_805FAF94 = t;
        if (t < 5 && t >= 0) {
            lbl_805FAF98 = 1;
        }
    }
    if (t <= 5 && lbl_805FAF9A == 0) {
        lbl_805FAF9A = 1;
        void* set = ScreenSetsCurrent(gScreenSets);
        void* grp = 0;
        void* anim = 0;
        if (set != 0) {
            grp = ScreenSetFindElement(set, StringHash("timer_group"));
            anim = fn_80198F1C(set, StringHash("timer_pulse_anim"));
        }
        if (grp != 0 && anim != 0) {
            fn_8019BBA4(grp, anim, 10);
        }
    }
}

// 0x80021E30
// Returns 1 with ~67% chance when the countdown just hit 5 (or the flag is set), else 0.
s32 fn_80021E30(void) {
    s32 t = lbl_805FAF94;
    if ((t == 5 && lbl_805FAFA8 != 5) || lbl_805FAF99 != 0) {
        lbl_805FAFA8 = t;
        lbl_805FAF99 = 0;
        f32 v = lbl_805FC8E0 + lbl_805FC8F4 * (lbl_805FC8F0 * (f32)RandomU32());
        return v < lbl_805FC8F8;
    }
    lbl_805FAFA8 = t;
    return 0;
}

// 0x800222F4
// Stops the timer: stores the elapsed time and records the best time for the experiment.
void fn_800222F4(u8* p) {
    lbl_805FAF9B = 1;
    u8* obj = lbl_805FB134;
    lbl_805FAF90 = (s32)*(f32*)(p + 0x6C);
    if (obj != 0 && *(s32*)obj != 0) {
        obj = 0;
    }
    u32 t = (u32)(*(f32*)(p + 0x78) - *(f32*)(p + 0x6C));
    if (t != 0) {
        SaveRecordBestTime(lbl_805FBBE0, *(s32*)(obj + 0x7C), t);
    }
    *(f32*)(p + 0x6C) = lbl_805FC8E0;
}

// 0x800223F0
// Starts the timer at farg0: caches it, binds the "timer" UI element and shows the value.
void fn_800223F0(u8* p, f32 farg0) {
    char buf[32];
    *(f32*)(p + 0x78) = farg0;
    *(f32*)(p + 0x6C) = farg0;
    *(u8*)(p + 0x82) = 0;
    void* set = ScreenSetsCurrent(gScreenSets);
    void* el = ScreenSetFindElement(set, 0x9D841CAAu);  // "timer"
    *(void**)(p + 0x68) = el;
    SysSprintf(buf, "%d", (s32)*(f32*)(p + 0x6C));
    UISetText(*(void**)(p + 0x68), buf, 1);
    u8* e = *(u8**)(p + 0x68);
    (*(VFn2*)(*(u8**)(e + 0x138) + 0x2C))(e, 2);
    u8* e2 = (u8*)ScreenSetFindElement(set, 0x0CAF9C6Fu);
    if (e2 != 0) {
        (*(VFn2*)(*(u8**)(e2 + 0x138) + 0x2C))(e2, 2);
    }
}

}  // extern "C"
