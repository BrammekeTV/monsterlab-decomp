// UIElement script callbacks in 0x8019xxxx (colour, rotation, UI animation control).
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void*       ScreenSetsCurrent(void* sets);                 // 0x8018D1E4
void*       ScreenSetFindElement(void* set, u32 hash);     // 0x80198F18
void        fn_8018D2B8(const char* fmt, ...);             // 0x8018D2B8 (debug log)
void        fn_8018D370(const char* fmt, ...);             // 0x8018D370 (debug log)
const char* FormatHashString(u32 hash);                         // 0x801EB708 (hash -> name for logs)
void        UIWidgetPlayAnimLoop(void* w, u32 anim, s32 n);  // 0x8019BB1C
void        UIElement_PlayUIAnimFromSamePositionImpl(void* el, u32 anim, s32 n);        // 0x8019B888
void        UIElement_PlayUIAnimFromReversePositionImpl(void* el, u32 anim, s32 n);        // 0x8019B94C
s32 fn_8019BC80(u8* arg0, u32 arg1);               // 0x8019BC80
s32         fn_801AB80C(u8* a3, f32 rate);                         // 0x801AB80C
s32 fn_801AB748(u8* p, f32 v);                          // 0x801AB748
f32         fn_801AB7F0(void* h);                          // 0x801AB7F0

extern "C++" void* gScreenSets;                  // 0x805FB654
extern const char lbl_80381868[];          // 0x80381868 (string block)
extern const char lbl_803818A8[];          // 0x803818A8 (string)
extern const char lbl_80381AA0[];          // 0x80381AA0 (string)
extern f32 lbl_805FF8A0;                   // 0x805FF8A0
extern f32 lbl_805FF8A8;                   // 0x805FF8A8 (255.0f)

// Looks up a UI element by hash in the current screen set, logging if missing.
static u8* FindUIElement(u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return NULL;
    }
    u8* el = (u8*)ScreenSetFindElement(set, hash);
    if (el == NULL) {
        fn_8018D2B8(lbl_80381AA0, FormatHashString(hash));
        return NULL;
    }
    return el;
}

// 0x80190074
// Sets the element's RGBA (0..255 ints -> 0..1 floats at +0x80) and notifies its handler (slot 0x2C, id 0x20).
void UIElement_SetColor_Ints(u32 a0, BOOL a1, BOOL a2, BOOL a3, BOOL a4) {
    f32 r = (f32)(u32)a1 / lbl_805FF8A8;
    f32 g = (f32)(u32)a2 / lbl_805FF8A8;
    f32 b = (f32)(u32)a3 / lbl_805FF8A8;
    f32 a = (f32)(u32)a4 / lbl_805FF8A8;
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        *(f32*)(el + 0x80) = r;
        *(f32*)(el + 0x84) = g;
        *(f32*)(el + 0x88) = b;
        *(f32*)(el + 0x8c) = a;
        (*(void (**)(void*, s32, f32, f32, f32))(*(u8**)(el + 0x138) + 0x2c))(el, 0x20, b, g, r);
    }
}

// 0x801901D8
// Sets the element's rotation from a vec3 (+0x90) and notifies its handler (slot 0x2C, id 0x18).
void UIElement_SetRot(u32 a0, void* a1) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        if (a1 == NULL) {
            fn_8018D370(lbl_80381868 + 0x2e8);
            return;
        }
        f32 x = *(f32*)((u8*)a1 + 0x0);
        f32 y = *(f32*)((u8*)a1 + 0x4);
        f32 z = *(f32*)((u8*)a1 + 0x8);
        *(f32*)(el + 0x90) = x;
        *(f32*)(el + 0x94) = y;
        *(f32*)(el + 0x98) = z;
        (*(void (**)(void*, s32, f32, f32))(*(u8**)(el + 0x138) + 0x2c))(el, 0x18, y, x);
    }
}

// 0x801902C4
// Same as UIElement_SetRot but with the three components passed as floats.
void UIElement_SetRot_Floats(u32 a0, f32 a1, f32 a2, f32 a3) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        *(f32*)(el + 0x90) = a1;
        *(f32*)(el + 0x94) = a2;
        *(f32*)(el + 0x98) = a3;
        (*(void (**)(void*, s32, f32, f32))(*(u8**)(el + 0x138) + 0x2c))(el, 0x18, a2, a1);
    }
}

// 0x801903CC
// Plays a UI animation on the element with priority 10.
void UIElement_PlayUIAnim(u32 a0, u32 a1) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        UIWidgetPlayAnimLoop(el, a1, 0xA);
    }
}

// 0x8019046C
// Plays a UI animation starting from the same position, priority 10.
void UIElement_PlayUIAnimFromSamePosition(u32 a0, u32 a1) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        UIElement_PlayUIAnimFromSamePositionImpl(el, a1, 0xA);
    }
}

// 0x8019050C
// Plays a UI animation starting from the reverse position, priority 10.
void UIElement_PlayUIAnimFromReversePosition(u32 a0, u32 a1) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        UIElement_PlayUIAnimFromReversePositionImpl(el, a1, 0xA);
    }
}

// 0x801905AC
// Plays a UI animation with a caller-supplied (signed byte) priority.
void UIElement_PlayUIAnimWithPriority(u32 a0, u32 a1, s32 a2) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        UIWidgetPlayAnimLoop(el, a1, (s32)(s8)a2);
    }
}

// 0x80190658
// Plays a UI animation then sets its playback rate; rate below the threshold also runs fn_801AB7F0.
void UIElement_PlayUIAnimAtRate(u32 a0, u32 a1, f32 a2) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        UIWidgetPlayAnimLoop(el, a1, 0xA);
        void* h = *(void**)(el + 0x10);
        if (h == NULL) {
            fn_8018D370(lbl_80381868 + 0x310);
            return;
        }
        fn_801AB80C((u8*)h, a2);
        if (a2 >= lbl_805FF8A0) {
            fn_801AB748(*(u8**)(el + 0x10), a2);  // f1 still holds a2
            return;
        }
        f32 rate = fn_801AB7F0(*(u8**)(el + 0x10));
        fn_801AB748(*(u8**)(el + 0x10), rate);
    }
}

// 0x80190770
// Stops a UI animation on the element.
void UIElement_StopUIAnim(u32 a0, u32 /*0x06*/ a1) {
    u8* el = FindUIElement(a0);
    if (el != NULL) {
        fn_8019BC80(el, a1);
    }
}

// 0x8019080C
// Returns nonzero if the element has an active animation handle at +0x10.
u32 /*0x06*/ UIElement_IsUIAnimPlaying(u32 a0) {
    u8* el = FindUIElement(a0);
    if (el == NULL) {
        return 0;
    }
    return *(void**)(el + 0x10) != NULL;
}

}  // extern "C"
