// UISlider / UIGrid accessors and UIEvent senders (0x8019xxxx).
// Shared pattern: look the widget up by hash in the current screen set, then check its type at +0x60
// (3 = slider, 4 = grid). Log strings live in the lbl_80381868 block: +0x40 (no screen set),
// +0x238 (element not found, with the hash name), +0x530 (slider type mismatch), +0x558 (grid type mismatch).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void*   ScreenSetsCurrent(void* sets);                          // 0x8018D1E4
void*   ScreenSetFindElement(void* set, u32 hash);              // 0x80198F18
void    fn_8018D2B8(const char* fmt, ...);                      // 0x8018D2B8 (debug log)
void    fn_8018D370(const char* fmt, ...);                      // 0x8018D370 (debug log)
const char* fn_801EB708(u32 hash);                              // 0x801EB708 (hash -> name for logs)
int     SysSprintf(char* buf, const char* fmt, ...);            // 0x80142EF8
void    UISetText(void* el, const char* text, s32 a);           // 0x8019EFC8
void    UIPlayAnim(void* el, u32 anim);                         // 0x8019BE40
void    UIWidgetPlayAnim(void* w, u32 anim);                    // 0x80199B38
void    fn_801A408C(u8* el, s32 arg);                           // 0x801A408C
f32     fn_801A4070(u8* el);                                    // 0x801A4070
u32     fn_801A65A0(void* obj);                                 // 0x801A65A0
u32     fn_801A665C(void* obj);                                 // 0x801A665C
BOOL    fn_801A6518(void* obj);                                 // 0x801A6518
BOOL    fn_801A655C(void* obj);                                 // 0x801A655C
s32     fn_801A6714(u8* el);                                    // 0x801A6714
void    fn_801A67C8(u8* el, u32 a1);                            // 0x801A67C8
s32     fn_801A75E8(u8* el, BOOL a1, s32 count);                // 0x801A75E8
s32     fn_801A7490(u8* el, BOOL a1, s32 count);                // 0x801A7490
s32     fn_801A7780(u8* el, u32 a1, BOOL a2, s32 count);        // 0x801A7780
s32     fn_801A7838(u8* el, BOOL a1, s32 count);                // 0x801A7838
s32     fn_801A79AC(u8* el, BOOL a2, s32 count, u32 a1);        // 0x801A79AC
s32     fn_801A7B14(u8* el, s32 a1, s32 a2, u32 a3);            // 0x801A7B14
void    fn_801A7B8C(u8* el, u32 a1);                            // 0x801A7B8C
void    fn_801A7C38(u8* el, s32 a1, u32 a2);                    // 0x801A7C38
s32     fn_801A7DC0(u8* el, u32 a1);                            // 0x801A7DC0
ScriptValue fn_801A7CC4(u8* el, s32 a1);                        // 0x801A7CC4
}

extern "C++" void* gScreenSets;                                 // 0x805FB654
extern f32 lbl_805FF8A0;                                        // 0x805FF8A0 (float constant read on the miss path)
extern const char lbl_80381868[];                               // 0x80381868 (string block)
extern const char lbl_803818A8[];                               // 0x803818A8 (string)
extern const char lbl_80381AA0[];                               // 0x80381AA0 (string)
extern const char lbl_80381FC8[];                               // 0x80381FC8 (string)

// Looks up the widget with this hash in the current screen set and checks its type at +0x60.
// Returns the widget, or NULL after logging: no screen set (+0x40), not found (+0x238 with the name),
// or type mismatch (typeMsg with the name).
static u8* UIFindTypedElement(u32 hash, s32 type, const char* typeMsg) {
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
    if (*(s32*)(el + 0x60) != type) {
        fn_8018D370(typeMsg, fn_801EB708(hash));
        return NULL;
    }
    return el;
}

// Cell count of a grid: width at +0x13C times height at +0x140.
static s32 UIGridCellCount(u8* el) {
    s32 w = *(s32*)(el + 0x13C);
    s32 h = *(s32*)(el + 0x140);
    return (s32)((u32)w * (u32)h);
}

extern "C" {

// 0x80192764
// Steps a slider down (fn_801A408C with 0). Slider type 3 only.
void UISlider_DecValue(u32 a0) {
    u8* el = UIFindTypedElement(a0, 3, lbl_80381868 + 0x530);
    if (el != NULL) {
        fn_801A408C(el, 0);
    }
}

// 0x801931FC
// Returns the slider's increment step (fn_801A4070), or the constant lbl_805FF8A0 on a miss.
f32 UISlider_GetIncValue(u32 a0) {
    u8* el = UIFindTypedElement(a0, 3, lbl_80381868 + 0x530);
    if (el == NULL) {
        return lbl_805FF8A0;
    }
    return fn_801A4070(el);
}

// 0x801939D0
// Grid cell count (width * height). Grid type 4 only, 0 on a miss.
s32 UIGrid_GetSize(u32 a0) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el == NULL) {
        return 0;
    }
    return UIGridCellCount(el);
}

// 0x80193AA0
u32 UIGrid_IsHScrolling(u32 a0) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el == NULL) {
        return 0;
    }
    return (u32)fn_801A6518(el);
}

// 0x80193B68
u32 UIGrid_IsVScrolling(u32 a0) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el == NULL) {
        return 0;
    }
    return (u32)fn_801A655C(el);
}

// 0x80193C30
s32 UIGrid_GetNumValidCols(u32 a0) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el == NULL) {
        return 0;
    }
    return (s32)fn_801A65A0(el);
}

// 0x80193CF8
s32 UIGrid_GetNumValidRows(u32 a0) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el == NULL) {
        return 0;
    }
    return (s32)fn_801A665C(el);
}

// 0x80193DC0
s32 UIGrid_GetNumValidItems(u32 a0) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el == NULL) {
        return 0;
    }
    return fn_801A6714(el);
}

// 0x80193E88
void UIGrid_SetTextList(u32 a0, BOOL a1) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A7490(el, a1, UIGridCellCount(el));
    }
}

// 0x80193F60
void UIGrid_SetImageList(u32 a0, BOOL a1) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A75E8(el, a1, UIGridCellCount(el));
    }
}

// 0x80194038
void UIGrid_SetEventList(u32 a0, u32 a1, BOOL a2) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A7780(el, a1, a2, UIGridCellCount(el));
    }
}

// 0x80194120
void UIGrid_SetTextColorList(u32 a0, BOOL a1) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A7838(el, a1, UIGridCellCount(el));
    }
}

// 0x801941F8
void UIGrid_SetActiveList(u32 a0, u32 a1, BOOL a2) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A79AC(el, a2, UIGridCellCount(el), a1);
    }
}

// 0x801942E0
void UIGrid_PlayUIAnim(u32 a0, s32 a1, s32 a2, u32 a3) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A7B14(el, a1, a2, a3);
    }
}

// 0x801943B4
void UIGrid_PlayUIAnimOnType(u32 a0, s32 a1, u32 a2) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A7C38(el, a1, a2);
    }
}

// 0x80194490
void UIGrid_PlayUIAnimAll(u32 a0, u32 a1) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A7B8C(el, a1);
    }
}

// 0x8019455C
void UIGrid_SetHighlightVisibleFlag(u32 a0, u32 a1) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A7DC0(el, a1);
    }
}

// 0x801947A0
ScriptValue UIGrid_GetElementPos(u32 a0, s32 a1) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el == NULL) {
        return 0;
    }
    return fn_801A7CC4(el, a1);
}

// 0x80194878
void UIGrid_SetIgnoreAlphaPixels(u32 a0, u32 a1) {
    u8* el = UIFindTypedElement(a0, 4, lbl_80381868 + 0x558);
    if (el != NULL) {
        fn_801A67C8(el, a1);
    }
}

// 0x80195224
// Sends an event to the top screen of the current screen set and its children (the widget at count*4+0x3C).
void UIEvent_SendToTopScreenAndChildren(u32 a0) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return;
    }
    s32 count = *(s32*)(set + 0x60);
    if (count > 0) {
        void* top = *(void**)(set + 0x3C + count * 4);
        if (top == NULL) {
            fn_8018D370(lbl_80381FC8);
        } else {
            UIWidgetPlayAnim(top, a0);
        }
    }
}

// 0x80195348
// Plays the animation a0 on the element with hash a1 in the current screen set.
void UIEvent_SendToElement(u32 a0, u32 a1) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return;
    }
    u8* el = (u8*)ScreenSetFindElement(set, a1);
    if (el == NULL) {
        fn_8018D2B8(lbl_80381AA0, fn_801EB708(a1));
        return;
    }
    UIPlayAnim(el, a0);
}

// 0x8019F2F4
// Shows n on the element as text, with an explicit sign ("%+d").
void UISetNegativeNumber(void* el, s32 n) {
    char buf[16];
    SysSprintf(buf, "%+d", n);
    UISetText(el, buf, 1);
}

}  // extern "C"
