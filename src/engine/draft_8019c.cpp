// UIText / UIEntity / UIParticleEffect script callbacks in 0x801911C8-0x80191B68.
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
void        UISetNumber(void* el, s32 n);                  // 0x8019EF..
u32         fn_8019DC10(void* el);                         // 0x8019DC10
void        fn_8019DC58(void* el);                         // 0x8019DC58
void        fn_8019F340(void* el, s32 a, f32 v);           // 0x8019F340
s32 fn_8019F3B4(u8* arg0, s32 arg1);                  // 0x8019F3B4
s32         fn_801A3010(void* obj, void* data);            // 0x801A3010
s32         fn_801A30C0(void* el, u32 bone);               // 0x801A30C0
void        fn_801A8654(u8* obj, s32 a1);                  // 0x801A8654

extern "C++" void* gScreenSets;                  // 0x805FB654
extern const char lbl_80381868[];          // 0x80381868 (string block)
extern f32 lbl_805F9408;                   // 0x805F9408
extern f32 lbl_805FF8A0;                   // 0x805FF8A0 (0.0f)
extern f32 lbl_805FF8A4;                   // 0x805FF8A4 (1.0f)
}

// Element lookup using the log strings at lbl_80381868 +0x40 / +0x238.
static u8* FindElementA(u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        return NULL;
    }
    u8* el = (u8*)ScreenSetFindElement(set, hash);
    if (el == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x238, FormatHashString(hash));
        return NULL;
    }
    return el;
}

extern "C" {

// 0x801911C8
// Sets a UIText element's number (element type 1 only).
void UIText_SetString_Int(u32 a0, s32 a1) {
    u8* el = FindElementA(a0);
    if (el == NULL) return;
    if (*(s32*)(el + 0x60) != 1) {
        fn_8018D370(lbl_80381868 + 0x3A8, FormatHashString(a0));
        return;
    }
    UISetNumber(el, a1);
}

// 0x80191294
// Sets a UIText element's text from a float (element type 1 only).
void UIText_SetString_Float(u32 a0, f32 a1, BOOL a2) {
    u8* el = FindElementA(a0);
    if (el == NULL) return;
    if (*(s32*)(el + 0x60) != 1) {
        fn_8018D370(lbl_80381868 + 0x3A8, FormatHashString(a0));
        return;
    }
    fn_8019F340(el, a2, a1);
}

// 0x80191370
// Finishes the typeout: progress (+0x14C) = 1.0 + length (+0x150) / scale.
void UIText_ForceTypeout(u32 a0) {
    u8* el = FindElementA(a0);
    if (el == NULL || *(s32*)(el + 0x60) != 1) {
        fn_8018D2B8(lbl_80381868 + 0x3A8, FormatHashString(a0));
        return;
    }
    *(f32*)(el + 0x14C) = lbl_805FF8A4 + (f32)*(s32*)(el + 0x150) / lbl_805F9408;
}

// 0x8019145C
// True if the typeout has reached the end (or the element is missing / wrong type).
u32 /*0x06*/ UIText_IsTypeoutDone(u32 a0) {
    u8* el = FindElementA(a0);
    if (el == NULL) return 1;
    if (*(s32*)(el + 0x60) != 1) {
        fn_8018D370(lbl_80381868 + 0x3A8, FormatHashString(a0));
        return 1;
    }
    return (*(f32*)(el + 0x14C) * lbl_805F9408 - (f32)*(s32*)(el + 0x150)) >= lbl_805FF8A4;
}

// 0x80191564
// True if the text is on its last page (or the element is missing / wrong type).
u32 /*0x06*/ UIText_IsOnLastPage(u32 a0) {
    u8* el = FindElementA(a0);
    if (el == NULL) return 1;
    if (*(s32*)(el + 0x60) != 1) {
        fn_8018D370(lbl_80381868 + 0x3A8, FormatHashString(a0));
        return 1;
    }
    return fn_8019DC10(el);
}

// 0x8019162C
// Sets the wrap length of a UIText element.
void UIText_SetWrapLength(u32 a0, s32 a1) {
    u8* el = FindElementA(a0);
    if (el == NULL) return;
    if (*(s32*)(el + 0x60) != 1) {
        fn_8018D370(lbl_80381868 + 0x3A8, FormatHashString(a0));
        return;
    }
    fn_8019F3B4(el, a1);
}

// 0x801916F8
// X of the text's right edge: transforms (+0x1D4,+0x1D8,+0x1DC) by the matrix at +0x17C
// (translation +0x1A0); falls back to the element's +0x68 if the result is below 1.0.
f32 UIText_GetRightXPos(u32 a0) {
    u8* el = FindElementA(a0);
    if (el == NULL) return lbl_805FF8A0;
    if (*(s32*)(el + 0x60) != 1) {
        fn_8018D370(lbl_80381868 + 0x3A8, FormatHashString(a0));
        return lbl_805FF8A0;
    }
    fn_8019DC58(el);
    f32 x = *(f32*)(el + 0x1D4);
    f32 y = *(f32*)(el + 0x1D8);
    f32 z = *(f32*)(el + 0x1DC);
    f32 r = *(f32*)(el + 0x1A0) + (*(f32*)(el + 0x194) * z + (*(f32*)(el + 0x17C) * x + *(f32*)(el + 0x188) * y));
    if (r < lbl_805FF8A4) r = *(f32*)(el + 0x68);
    return r;
}

// 0x80191874
// Sets the entity shown by a UIEntity element (type 2, subtype 1).
void UIEntity_SetEntity(u32 a0, BOOL a1) {
    u8* el = FindElementA(a0);
    if (el == NULL) return;
    if (*(s32*)(el + 0x60) != 2) {
        fn_8018D370(lbl_80381868 + 0x3D0, FormatHashString(a0));
        return;
    }
    if (*(s32*)(el + 0x13C) != 1) {
        fn_8018D370(lbl_80381868 + 0x400, FormatHashString(a0));
        return;
    }
    fn_801A3010(el, (void*)(uintptr_t)a1);
}

// 0x80191968
// Sets the offset bone of a UIEntity element (type 2, subtype 1).
void UIEntity_SetOffsetBone(u32 a0, u32 a1) {
    u8* el = FindElementA(a0);
    if (el == NULL) return;
    if (*(s32*)(el + 0x60) != 2) {
        fn_8018D370(lbl_80381868 + 0x3D0, FormatHashString(a0));
        return;
    }
    if (*(s32*)(el + 0x13C) != 1) {
        fn_8018D370(lbl_80381868 + 0x400, FormatHashString(a0));
        return;
    }
    fn_801A30C0(el, a1);
}

// 0x80191A6C
// Activates/deactivates a UIParticleEffect element (type 2, subtype 3); active if a1 > 0.
void UIParticleEffect_SetActive(u32 a0, u32 /*0x06*/ a1) {
    u8* el = FindElementA(a0);
    if (el == NULL) return;
    if (*(s32*)(el + 0x60) != 2) {
        fn_8018D370(lbl_80381868 + 0x3D0, FormatHashString(a0));
        return;
    }
    if (*(s32*)(el + 0x13C) != 3) {
        fn_8018D370(lbl_80381868 + 0x460, FormatHashString(a0));
        return;
    }
    fn_801A8654(el, (s32)a1 > 0);
}

}  // extern "C"
