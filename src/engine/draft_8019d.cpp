// UIImage / UISlideshow / UISlider script callbacks in 0x80191B68-0x80192424.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void*       ScreenSetsCurrent(void* sets);                 // 0x8018D1E4
void*       ScreenSetFindElement(void* set, u32 hash);     // 0x80198F18
void*       fn_80198F74(void* set, u32 hash);              // 0x80198F74
void        fn_8018D2B8(const char* fmt, ...);             // 0x8018D2B8 (debug log)
void        fn_8018D370(const char* fmt, ...);             // 0x8018D370 (debug log)
const char* fn_801EB708(u32 hash);                         // 0x801EB708 (hash -> name for logs)
s32         UISetTexture(u8* el, s32 tex);                 // 0x801A11FC
void        UISetProgress(void* obj, f32 t);               // 0x801A....
ScriptValue fn_801A1208(u8* el);                           // 0x801A1208
void        fn_801A1ABC(u8* el, u32 v);                    // 0x801A1ABC
void        fn_801A1DC8(u8* el, void* list);               // 0x801A1DC8
void        fn_801A23E4(u8* el);                           // 0x801A23E4
void        fn_801A23F8(u8* el);                           // 0x801A23F8
void        fn_801A2510(u8* el);                           // 0x801A2510
void        fn_801A2524(u8* el);                           // 0x801A2524

extern "C++" void* gScreenSets;                  // 0x805FB654
extern const char lbl_80381868[];          // 0x80381868 (string block)
}

typedef void (*ElemFn)(u8*, s32);

// Element lookup using the log strings at lbl_80381868 +0x40 / +0x238.
static u8* FindElementD(u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        return NULL;
    }
    u8* el = (u8*)ScreenSetFindElement(set, hash);
    if (el == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x238, fn_801EB708(hash));
        return NULL;
    }
    return el;
}

// Returns the element if it is an image (type 2, subtype 0), else logs and returns NULL.
static u8* FindImage(u32 hash) {
    u8* el = FindElementD(hash);
    if (el == NULL) return NULL;
    if (*(s32*)(el + 0x60) != 2) {
        fn_8018D370(lbl_80381868 + 0x3d0, fn_801EB708(hash));
        return NULL;
    }
    if (*(s32*)(el + 0x13c) != 0) {
        fn_8018D370(lbl_80381868 + 0x498, fn_801EB708(hash));
        return NULL;
    }
    return el;
}

// Same for slideshow (type 2, subtype 4).
static u8* FindSlideshow(u32 hash) {
    u8* el = FindElementD(hash);
    if (el == NULL) return NULL;
    if (*(s32*)(el + 0x60) != 2) {
        fn_8018D370(lbl_80381868 + 0x3d0, fn_801EB708(hash));
        return NULL;
    }
    if (*(s32*)(el + 0x13c) != 4) {
        fn_8018D370(lbl_80381868 + 0x4c8, fn_801EB708(hash));
        return NULL;
    }
    return el;
}

// Slider (type 3).
static u8* FindSlider(u32 hash) {
    u8* el = FindElementD(hash);
    if (el == NULL) return NULL;
    if (*(s32*)(el + 0x60) != 3) {
        fn_8018D370(lbl_80381868 + 0x530, fn_801EB708(hash));
        return NULL;
    }
    return el;
}

extern "C" {

// 0x80191B68
// Sets the image texture (virtual slot 0x2C) or clears it (slot 0x30) when tex is 0.
void UIImage_SetTexture(u32 a0, BOOL a1) {
    u8* el = FindImage(a0);
    if (el == NULL) return;
    if (a1 != 0) {
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 2);
        UISetTexture(el, a1);
        return;
    }
    (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
}

// 0x80191CAC
ScriptValue UIImage_GetTexture(u32 a0) {
    u8* el = FindImage(a0);
    if (el == NULL) return (ScriptValue)0;
    return fn_801A1208(el);
}

// 0x80191DA0
void UIImage_SetIgnoreAlphaPixels(u32 a0, u32 /*0x06*/ a1) {
    u8* el = FindImage(a0);
    if (el != NULL) fn_801A1ABC(el, a1);
}

// 0x80191E94
void UISlideshow_Start(u32 a0) {
    u8* el = FindSlideshow(a0);
    if (el != NULL) fn_801A23E4(el);
}

// 0x80191F78
void UISlideshow_Stop(u32 a0) {
    u8* el = FindSlideshow(a0);
    if (el != NULL) fn_801A23F8(el);
}

// 0x8019205C
void UISlideshow_Pause(u32 a0) {
    u8* el = FindSlideshow(a0);
    if (el != NULL) fn_801A2510(el);
}

// 0x80192140
void UISlideshow_Resume(u32 a0) {
    u8* el = FindSlideshow(a0);
    if (el != NULL) fn_801A2524(el);
}

// 0x80192224
// Looks up the named texture list in the current screen set and gives it to the slideshow.
void UISlideshow_SetUITextureList(u32 a0, u32 a1) {
    u8* el = FindSlideshow(a0);
    if (el == NULL) return;
    void* list = fn_80198F74(ScreenSetsCurrent(gScreenSets), a1);
    if (list == NULL) {
        fn_8018D370(lbl_80381868 + 0x500, a1);
        return;
    }
    fn_801A1DC8(el, list);
}

// 0x80192358
void UISlider_SetValue(u32 a0, f32 a1) {
    u8* el = FindSlider(a0);
    if (el != NULL) UISetProgress(el, a1);
}

// 0x80192424
void UISlider_SetValue_Int(u32 a0, s32 a1) {
    u8* el = FindSlider(a0);
    if (el != NULL) UISetProgress(el, (f32)a1);
}

}  // extern "C"
