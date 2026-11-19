#include "types.h"

// Globals defined elsewhere (or only referenced here), typed as their defining file uses them.
extern "C++" u8* lbl_805FB608;      // 0x805FB608: object; slot +4 is a function pointer called as (p, obj)
extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" void* lbl_805FAFB8;    // 0x805FAFB8: global object (+0x6C / +0x78 floats, +0xB4 cursor element)
extern "C++" void* lbl_805FBBAC;    // 0x805FBBAC: voice SFX handle
extern "C++" u32 lbl_805FAFA4;      // 0x805FAFA4: current cursor element hash
extern "C++" f32 lbl_805FC990;      // 0x805FC990: float 0.0f
extern "C++" f32 lbl_805FC99C;      // 0x805FC99C: float (cursor height)
extern "C++" u8* lbl_805FAFD8;      // 0x805FAFD8: global digging/dirt-pile object pointer
extern "C++" u8* lbl_805FAFB0;      // 0x805FAFB0: global template object pointer
extern "C++" u8 lbl_803860E8[];     // 0x803860E8: vtable-like object address
extern "C++" f32 lbl_805FCBC0;      // 0x805FCBC0: float threshold (full)
extern "C++" f32 lbl_805FCBB0;      // 0x805FCBB0: float threshold (spinning)
extern "C++" const char lbl_80355A08[]; // 0x80355A08: UI name string
extern "C++" const char lbl_80355A18[]; // 0x80355A18: UI name string
extern "C++" const char lbl_80355A60[]; // 0x80355A60: UI name string

extern "C" {
void*  ScreenSetsCurrent(void* sets);
void*  ScreenSetFindElement(void* set, u32 hash);
u32    StringHash(const char* s);
void   UIElement_PlayUIAnim(u32 element, u32 anim);
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);
u8*    fn_80021750(void);            // 0x80021750: digging state object
void   fn_80024D98(u8* p);           // 0x80024D98
void   fn_80024E90(u8* p);           // 0x80024E90
void   fn_80024EDC(u8* p);           // 0x80024EDC
void   fn_80169854(void* a, void* b); // 0x80169854
void   fn_801B1778(void* p, s32 a);  // 0x801B1778
u8*    fn_801B35C8(u8* p, s32 a);    // 0x801B35C8
void*  fn_801D40E0(s32 n);           // 0x801D40E0
void*  fn_80209814(void* v);         // 0x80209814
s32    fn_8020A9A8(s32 id);          // 0x8020A9A8
}

// UI screen set "UI_MSA_NewDigging" and element "shovel" (resolved with tools/hashes.py).
static const u32 kSetNewDigging = 0x9A5E553Bu;
static const u32 kShovelElem = 0x0A7964E0u;

extern "C" {

// 0x801B6F34
void fn_801B6F34(u8* p) {
    if (p == NULL) {
        return;
    }
    if (*(u32*)(*(u8**)(p + 0x4) + 0x20) & 0x8000u) {
        u8* t = (*(u8* (**)(void*))(*(u8**)p + 0xC))(p);
        u8* n = *(u8**)(p + 0x4);
        *(u32*)(n + 0x20) &= 0xFFFF7FFFu;
        if (t != NULL) {
            (*(void (**)(void*, void*, void*, void*))(*(u8**)t + 0x14))(t, n, p + 0x8, n + 0xC);
        }
    }
    u8* w = *(u8**)(p + 0x8C);
    if (w != NULL && (*(u32*)(*(u8**)(p + 0x4) + 0x20) & 0x4000u)) {
        fn_801B1778(w, 0);
    }
    for (u8* e = *(u8**)(p + 0xB0); e != NULL; e = *(u8**)(e + 0x8)) {
        u8* c = *(u8**)(e + 0x4);
        if (c != NULL) {
            u8* w2 = *(u8**)(c + 0x8C);
            if (w2 != NULL && (*(u32*)(*(u8**)(c + 0x4) + 0x20) & 0x4000u)) {
                fn_801B1778(w2, 0);
            }
        }
    }
}

// 0x8016A2B8
void fn_8016A2B8(u8* p) {
    u8* c = *(u8**)(p + 0xC);
    u8* t = (c != NULL) ? c - 0x1C : NULL;
    if (t != NULL) {
        fn_80169854(t, p);
    }
    *(void**)p = NULL;
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(p, lbl_805FB608);
}

// 0x8017493C
void fn_8017493C(u8* p) {
    u8* a = *(u8**)(p + 0x18);
    if (a != NULL) {
        (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(a, lbl_805FB608);
        *(void**)(p + 0x18) = NULL;
    }
    u8* r30 = *(u8**)(p + 0x14);
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x4) = 0;
    *(s32*)(p + 0xC) = 0;
    *(s32*)(p + 0x10) = 0;
    u8* sub = *(u8**)(r30 + 0x14);
    if (sub != NULL) {
        s32 n = *(s32*)(sub + 0x10) - 1;
        *(s32*)(sub + 0x10) = n;
        if (n == 0) {
            fn_8016A2B8(sub);
        }
        *(void**)(r30 + 0x14) = NULL;
    }
    *(s32*)(r30 + 0x0) = 0;
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(r30, lbl_805FB608);
    *(void**)(p + 0x14) = NULL;
    *(s32*)(p + 0x1C) = 0;
    *(s32*)(p + 0x20) = 0;
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(p, lbl_805FB608);
}

// 0x801F61AC
void fn_801F61AC(u8* arg0, s32 arg1) {
    fn_801B35C8(arg0, arg1);
    *(void**)arg0 = lbl_803860E8;
    *(f32*)(arg0 + 0x90) = 0.0f;
    *(f32*)(arg0 + 0x94) = 0.0f;
    *(f32*)(arg0 + 0x98) = 0.0f;
    *(f32*)(arg0 + 0x9C) = 0.0f;
    *(f32*)(arg0 + 0xA0) = 0.0f;
    *(f32*)(arg0 + 0xA4) = 0.0f;
    *(f32*)(arg0 + 0xA8) = 0.0f;
    *(f32*)(arg0 + 0xAC) = 0.0f;
    *(f32*)(arg0 + 0xB0) = 1.0f;
    *(f32*)(arg0 + 0xB4) = 1.0f;
    *(u8*)(arg0 + 0xB8) = 0;
}

// 0x8021F350
u8* fn_8021F350(u8* p, s32 idx) {
    return *(u8**)(p + idx * 4);
}

// 0x80023414
void DiggingClearRockGraphic(void) {
    u8* p = fn_80021750();
    void* sets = ScreenSetsCurrent(gScreenSets);
    if (*(u32*)sets != kSetNewDigging) {
        return;
    }
    void* elemA = ScreenSetFindElement(sets, 0xDE8244C0u);
    void* elemB = ScreenSetFindElement(sets, 0x478B157Au);
    s32 state = *(s32*)(p + 0xE8);
    if (state == 1 || state == 3) {
        (*(void (**)(void*, s32))(*(u8**)((u8*)elemA + 0x138) + 0x30))(elemA, 2);
    } else if (state == 2) {
        (*(void (**)(void*, s32))(*(u8**)((u8*)elemB + 0x138) + 0x30))(elemB, 2);
    }
}

// 0x800236CC
void DiggingPlayClangAnim(void) {
    u8* p = fn_80021750();
    s32 state = *(s32*)(p + 0xE8);
    if (state == 1) {
        UIElement_PlayUIAnim(kShovelElem, 0x7272C9A6u);
    } else if (state == 2 || state == 3) {
        UIElement_PlayUIAnim(kShovelElem, 0x7AC453D4u);
    }
}

// 0x80023754
void DiggingPlayDirtAnim(void) {
    u8* p = fn_80021750();
    void* sets = ScreenSetsCurrent(gScreenSets);
    u32 h1 = StringHash(lbl_80355A08);
    void* el = ScreenSetFindElement(sets, h1);
    (*(void (**)(void*, s32))(*(u8**)((u8*)el + 0x138) + 0x2C))(el, 2);
    fn_80024EDC(p);
    u32 anim = StringHash(lbl_80355A18);
    UIElement_PlayUIAnim(h1, anim);
}

// 0x80023644
void DiggingPlayHitAnim(void) {
    u8* p = fn_80021750();
    s32 state = *(s32*)(p + 0xE8);
    if (state == 1) {
        UIElement_PlayUIAnim(kShovelElem, 0x9FB619A1u);
    } else if (state == 2 || state == 3) {
        UIElement_PlayUIAnim(kShovelElem, 0x06977454u);
    }
}

// 0x800235A0
void DiggingPlayLiftAnim(void) {
    u8* p = fn_80021750();
    s32 state = *(s32*)(p + 0xE8);
    if (state == 1) {
        UIElement_PlayUIAnim(kShovelElem, 0xB324F8EFu);
    } else if (state >= 2 && state <= 4) {
        UIElement_PlayUIAnim(kShovelElem, 0x4DE8B1EAu);
    }
}

// 0x800234FC
void DiggingPlayThrustAnim(void) {
    u8* p = fn_80021750();
    s32 state = *(s32*)(p + 0xE8);
    if (state == 1) {
        UIElement_PlayUIAnim(kShovelElem, 0x2B13D2BDu);
    } else if (state >= 2 && state <= 4) {
        UIElement_PlayUIAnim(kShovelElem, 0xA8E25FA8u);
    }
}

// 0x800232A4
s32 DiggingRockAbsent(void) {
    return *(u8*)(fn_80021750() + 0xBC) == 0;
}

// 0x80023344
s32 DiggingRockCrushed(void) {
    // Same test as DiggingRockAbsent in the original (identical asm).
    return *(u8*)(fn_80021750() + 0xBC) == 0;
}

// 0x80024DA4
void DiggingSetDustCloud(void) {
    u32 h = StringHash(lbl_80355A60);
    void* sets = ScreenSetsCurrent(gScreenSets);
    u8* e = (u8*)ScreenSetFindElement(sets, h);
    u8* g = lbl_805FAFB0;
    u8* src = *(u8**)(g + 0x90);
    *(f32*)(e + 0x68) = *(f32*)(src + 0x68);
    *(f32*)(e + 0x6C) = *(f32*)(src + 0x6C);
    *(f32*)(e + 0x80) = *(f32*)(lbl_805FAFB0 + 0xC8);
    *(f32*)(e + 0x84) = *(f32*)(lbl_805FAFB0 + 0xCC);
    *(f32*)(e + 0x88) = *(f32*)(lbl_805FAFB0 + 0xD0);
    *(f32*)(e + 0x8C) = *(f32*)(lbl_805FAFB0 + 0xD4);
    *(f32*)(e + 0x74) = *(f32*)(lbl_805FAFB0 + 0xD8);
    *(f32*)(e + 0x78) = *(f32*)(lbl_805FAFB0 + 0xDC);
    *(f32*)(e + 0x7C) = *(f32*)(lbl_805FAFB0 + 0xE0);
}

// 0x8002D54C
s32 IsFull(void) {
    u8* g = lbl_805FAFD8;
    if (g == NULL) {
        return 0;
    }
    u8* e = g + *(s32*)(g + 0xA8) * 0x28;
    return *(f32*)(e + 0x108) >= lbl_805FCBC0;
}

// 0x8002D588
s32 IsNewlyFull(void) {
    u8* g = lbl_805FAFD8;
    if (g == NULL) {
        return 0;
    }
    u8* e = g + *(s32*)(g + 0xA8) * 0x28;
    if (!(*(f32*)(e + 0x108) >= lbl_805FCBC0)) {
        return 0;
    }
    g = lbl_805FAFD8;
    e = g + *(s32*)(g + 0xA8) * 0x28;
    if (*(u8*)(e + 0x110) != 0) {
        return 0;
    }
    *(u8*)(e + 0x110) = 1;
    return 1;
}

// 0x8002D610
s32 IsSpinning(void) {
    u8* g = lbl_805FAFD8;
    if (g == NULL) {
        return 0;
    }
    f32 v = *(f32*)(g + 0xAC);
    if (v < 0.0f) {
        v = -v;
    }
    return v > lbl_805FCBB0;
}

// 0x8002D644
s32 NeedWheelSound(void) {
    u8* g = lbl_805FAFD8;
    if (g == NULL) {
        return 0;
    }
    s32 a = (s32)*(f32*)(g + 0x74);
    s32 b = (s32)*(f32*)(g + 0x6C);
    if (a == b) {
        return 0;
    }
    f32 v = *(f32*)(g + 0xAC);
    if (v < 0.0f) {
        v = -v;
    }
    return v > lbl_805FCBB0;
}

// 0x8002DD14
void PlayHealthRecoverySound(void) {
    if (lbl_805FAFD8 == NULL) {
        return;
    }
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) {
            v = fn_80209814(v);
        }
        lbl_805FBBAC = v;
    }
    if (fn_8020A9A8(*(s32*)(lbl_805FAFD8 + 0x94)) != 0) {
        return;
    }
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) {
            v = fn_80209814(v);
        }
        lbl_805FBBAC = v;
    }
    s32 handle = PlaySound(0xC374641Eu, 0, 0, 0, 0, 0);
    *(s32*)(lbl_805FAFD8 + 0x94) = handle;
}

// 0x8002671C
void PuzzleSetCursor(s32 unused, u32 hash) {
    // The first argument is overwritten before it is read in the original, so it is unused.
    u8* saved = NULL;
    u8* cur = *(u8**)((u8*)lbl_805FAFB8 + 0xB4);
    if (cur != NULL) {
        (*(void (**)(void*, s32))(*(u8**)(cur + 0x138) + 0x30))(cur, 2);
        saved = *(u8**)((u8*)lbl_805FAFB8 + 0xB4) + 0x68;
    }
    lbl_805FAFA4 = hash;
    void* el = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), hash);
    *(void**)((u8*)lbl_805FAFB8 + 0xB4) = el;
    if (saved != NULL) {
        u8* dst = *(u8**)((u8*)lbl_805FAFB8 + 0xB4);
        *(f32*)(dst + 0x68) = *(f32*)(saved + 0x0);
        *(f32*)(dst + 0x6C) = *(f32*)(saved + 0x4);
        *(f32*)(dst + 0x70) = *(f32*)(saved + 0x8);
    }
    u8* nw = *(u8**)((u8*)lbl_805FAFB8 + 0xB4);
    *(f32*)(nw + 0x70) = lbl_805FC99C;
    (*(void (**)(void*, s32))(*(u8**)(nw + 0x138) + 0x2C))(nw, 0x18);
}

// 0x80025D98
s32 PuzzleTimeout(void) {
    return *(f32*)((u8*)lbl_805FAFB8 + 0x6C) < lbl_805FC990;
}

// 0x8002381C
void ResetDirtPile(void) {
    fn_80024E90(fn_80021750());
}

// 0x800237F8
void SetDirtPile(void) {
    fn_80024D98(fn_80021750());
}

}  // extern "C"
