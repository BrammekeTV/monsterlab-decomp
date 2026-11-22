// Draft-fixed functions from build/drafts/draft_miss611.cpp (verified against asm).
#include "types.h"

// Globals defined in C++ files: C++ linkage (see small_8002b.cpp).
extern "C++" void* lbl_805FBBAC;           // 0x805FBBAC (voice SFX handle)
extern "C++" u8    lbl_803C4298[];         // 0x803C4298 (+0x38: pad channel)

extern "C" {
u32   RandomU32(void);                                              // small_801Db.cpp
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);       // 0x80209E70
void  TimedRumble(s32 a0, f32 a1, u32 a2);                          // script_callbacks.h
void* fn_801D40E0(s32 n);                                           // 0x801D40E0
void* fn_80209814(void* v);                                         // 0x80209814
void  fn_8020A398(s32 handle);                                      // 0x8020A398
void  fn_80033870(u8* p);                                           // 0x80033870 (return value unused)
void  fn_80036EE0(u8* self, u8* p);                                 // 0x80036EE0 (signature guessed from call site)
void  fn_80077210(u8* a3, u8* a4);                                  // 0x80077210 (matches leaf/auto_03_80076D38_text.cpp)
void  UIElement_SetVisibleFlag(void* el, u32 on);                   // conflicts with ai_states.cpp decl (u32 hash)

extern f32 lbl_805FCD08;   // 0x805FCD08 (float constant)
extern f32 lbl_805FCD20;   // 0x805FCD20 (float constant, RNG scale)
extern f32 lbl_805FCD24;   // 0x805FCD24 (float constant, 1.0f)
extern f32 lbl_805FCEBC;   // 0x805FCEBC (float constant, stored as 0)
extern f32 lbl_805FCEC4;   // 0x805FCEC4 (float constant, rumble strength)
extern f32 lbl_805FD074;   // 0x805FD074 (float constant, 1.0f)
}

// Reads the first word of the object pointed to by the pointer at p+off (lwz, lwz).
static inline void* FirstWordOf(u8* p, s32 off) {
    return *(void**)(*(u8**)(p + off));
}

// Creates the voice SFX handle lbl_805FBBAC once (fn_801D40E0, then fn_80209814 if non-null).
static void EnsureVoiceSfx(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

extern "C" {

// 0x80031AA4
// Scatters entries of the table at +0xC0 (types 6/7 preferred): picks a random count and
// random start indices, then sets the timers at +0xD4/+0xD8.
void fn_80031AA4(u8* arg0) {
    s32 var_r29 = 0;
    s32 count = (s32)(lbl_805FCD24 + (lbl_805FCD08 * (lbl_805FCD20 * (f32)RandomU32())));
    while (var_r29 < count) {
        s32 n = *(s32*)(arg0 + 0xA8);
        f32 f27 = (f32)(n - 1);
        u8* base = *(u8**)(arg0 + 0xC0);
        s32 var_r7 = 0;
        s32 idx = (s32)(lbl_805FCD08 + ((f27 - lbl_805FCD08) * (lbl_805FCD20 * (f32)RandomU32())));
        for (;;) {
            s32 t = *(s32*)(base + idx * 0x60 + 0x8);
            if (t == 7 || t == 6 || var_r7 >= 0x1B) break;
            var_r7 += 1;
            idx = (idx + 1) % *(s32*)(arg0 + 0xA8);
        }
        if (var_r7 < 0x1B) {
            fn_80033870(base + idx * 0x60);
        }
        var_r29 += 1;
    }
    *(f32*)(arg0 + 0xD4) = lbl_805FCD08;
    *(f32*)(arg0 + 0xD8) = lbl_805FCD24 + (lbl_805FCD08 * (lbl_805FCD20 * (f32)RandomU32()));
}

// 0x8003CBB0
// Timed UI step: advances the timer at +4 by farg0 and, when it passes the phase limit,
// toggles the visible widget pair and advances the phase at +0x1C.
void fn_8003CBB0(u8* arg0, f32 farg0) {
    *(f32*)(arg0 + 0x4) = *(f32*)(arg0 + 0x4) + farg0;
    s32 phase = *(s32*)(arg0 + 0x1C);
    f32 limit;
    if (phase & 3) {
        limit = *(f32*)(arg0 + 0x20);
    } else if (phase == 4) {
        limit = *(f32*)(arg0 + 0x24);
    } else {
        limit = *(f32*)(arg0 + 0x28);
    }
    if (*(u8*)(arg0 + 0x2C) != 0) {
        f32 t = *(f32*)(arg0 + 0x4);
        f32 end = *(f32*)(arg0 + 0x30);
        if (t >= end) {
            u8* obj = *(u8**)(arg0 + 0x8);
            u8* vt = *(u8**)(obj + 0x138);
            ((s32 (*)(void*, s32))(*(void**)(vt + 0x30)))(obj, 2);
            *(u8*)(arg0 + 0x2E) = 1;
            return;
        }
        f32 frac = t / end;
        u8* obj = *(u8**)(arg0 + 0x8);
        *(f32*)(obj + 0x8C) = lbl_805FD074 - frac;
        u8* vt = *(u8**)(obj + 0x138);
        ((s32 (*)(void*, s32, f32))(*(void**)(vt + 0x2C)))(obj, 0x20, frac);
        return;
    }
    if (*(f32*)(arg0 + 0x4) > limit) {
        *(f32*)(arg0 + 0x4) = *(f32*)(arg0 + 0x4) - limit;
        switch (*(s32*)(arg0 + 0x1C)) {
        case 1:
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0x8), 0);
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0xC), 1);
            *(s32*)(arg0 + 0x1C) = 2;
            return;
        case 2:
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0xC), 0);
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0x8), 1);
            *(s32*)(arg0 + 0x1C) = 1;
            return;
        case 4:
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0x10), 0);
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0x14), 1);
            *(s32*)(arg0 + 0x1C) = 8;
            *(u8*)(arg0 + 0x0) = 1;
            return;
        case 8:
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0x14), 0);
            UIElement_SetVisibleFlag(FirstWordOf(arg0, 0x8), 1);
            *(s32*)(arg0 + 0x1C) = 1;
            return;
        default:
            return;
        }
    }
}

// 0x80036998
// Per-frame update of the target probe at +0x810 (called with the owner in arg1): accumulates
// the timers at +0x824/+0x82C, and on a hit releases the lock and plays the select sound;
// otherwise plays the lock sounds and the pad rumble.
void fn_80036998(u8* arg0, u8* arg1, f32 farg0) {
    if (*(s32*)(arg0 + 0x830) != 0) {
        *(f32*)(arg0 + 0x82C) = farg0 + *(f32*)(arg0 + 0x82C);
    }
    if (*(s32*)(arg0 + 0x828) != 0) {
        *(f32*)(arg0 + 0x824) = farg0 + *(f32*)(arg0 + 0x824);
    }
    u8* target = *(u8**)(arg0 + 0x810);
    if (target == NULL) {
        return;
    }
    u8* p158 = *(u8**)(arg1 + 0x158);
    u8* vt = *(u8**)(target + 0x138);
    s32 hit = ((s32 (*)(void*, s32, f32, f32))(*(void**)(vt + 0x38)))(
        target, 0, *(f32*)(p158 + 0x68), *(f32*)(p158 + 0x6C));
    if (hit != 0) {
        *(s32*)(arg0 + 0x830) = 0;
        *(s32*)(arg0 + 0x828) = 0;
        *(f32*)(arg0 + 0x824) = lbl_805FCEBC;
        if (*(s32*)(arg1 + 0xA60) != -1) {
            fn_8020A398(*(s32*)(arg1 + 0xA60));
            *(s32*)(arg1 + 0xA60) = -1;
        }
        if (*(s32*)(arg1 + 0xA5C) == -1) {
            EnsureVoiceSfx();
            *(s32*)(arg1 + 0xA5C) = PlaySound(0xCE6B0ACFu, 0, 0, 0, 1, 0);
        }
        fn_80036EE0(arg0, *(u8**)(arg1 + 0x158) + 0x68);
        return;
    }
    *(s32*)(arg0 + 0x830) = 1;
    *(s32*)(arg0 + 0x828) = 1;
    if (*(s32*)(arg1 + 0xA5C) == -1) {
        EnsureVoiceSfx();
        *(s32*)(arg1 + 0xA5C) = PlaySound(0xCE6B0ACFu, 0, 0, 0, 1, 0);
    }
    if (*(s32*)(arg1 + 0xA60) == -1) {
        EnsureVoiceSfx();
        *(s32*)(arg1 + 0xA60) = PlaySound(0x04ED2607u, 0, 0, 0, 1, 0);
    }
    TimedRumble(*(s32*)(lbl_803C4298 + 0x38), lbl_805FCEC4, 4);
}

// 0x80076928
// Claims the next free entry of the 4-slot ring at +0 (slots are 8 bytes: busy byte at +0xC,
// value pointer at +0x10). Falls back to the pointer at +0x8 and returns 0 when none is free.
s32 fn_80076928(u8* arg0, u8* arg1) {
    s32 var_r31 = 1;
    u8* r8 = NULL;
    u32 r6 = *(u32*)(arg0 + 0x2C);
    u32 r5 = *(u32*)(arg0 + 0x30);
    for (u32 k = 0; k < 4; k++) {
        u32 idx = (r6 + k) & 3;
        if (idx == r5) {
            break;
        }
        u8* slot = arg0 + idx * 8;
        if (*(u8*)(slot + 0xC) == 0) {
            *(u8*)(slot + 0xC) = 1;
            r8 = *(u8**)(slot + 0x10);
            break;
        }
    }
    if (r8 == NULL) {
        r8 = *(u8**)(arg0 + 0x8);
        var_r31 = 0;
    }
    fn_80077210(*(u8**)(arg1 + 0x6CC), r8);
    return var_r31;
}

}  // extern "C"
