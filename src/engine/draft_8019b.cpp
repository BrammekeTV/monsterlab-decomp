// UIElement / UIGroup / UIText script callbacks in 0x80190xxx-0x80191xxx (anim rate, hit test, text).
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void*       ScreenSetsCurrent(void* sets);                 // 0x8018D1E4
void*       ScreenSetFindElement(void* set, u32 hash);     // 0x80198F18
void        fn_8018D2B8(const char* fmt, ...);             // 0x8018D2B8 (debug log)
void        fn_8018D370(const char* fmt, ...);             // 0x8018D370 (debug log)
const char* fn_801EB708(u32 hash);                         // 0x801EB708 (hash -> name for logs)
void        fn_801AB748(void* h);                          // 0x801AB748
f32         fn_801AB804(void* h);                          // 0x801AB804 (current anim time)
void        fn_801AB80C(f32 rate);                         // 0x801AB80C
void        fn_8019CC7C(void* el, u32 flag);               // 0x8019CC7C
s32         fn_8019CD14(void* el);                         // 0x8019CD14
void        fn_8019DC58(void* el);                         // 0x8019DC58
void        fn_8019EA5C(void* el, s32 v);                  // 0x8019EA5C
void        UISetText(void* el, const char* text, s32 a);  // 0x8019EFC8
const char* UITextGetString(void* text);                   // 0x8019F3AC
int         strcmp(const char* a, const char* b);

void fn_80190D28(u32 hash, const char* text, s32 v);       // 0x80190D28 (defined below)

extern void* gScreenSets;                  // 0x805FB654
struct StringTable;
extern "C++" StringTable* gStringTable;                 // 0x805FB910
extern void* gStringTable2;                // 0x805FB90C
extern const char lbl_80381868[];          // 0x80381868 (string block)
extern const char lbl_803818A8[];          // 0x803818A8 (string)
extern const char lbl_80381AA0[];          // 0x80381AA0 (string)
extern const char lbl_80354800[];          // 0x80354800 (string)
extern const char lbl_8035480C[];          // 0x8035480C (string)
extern const char lbl_805F8200[];          // 0x805F8200 (string)
extern const char lbl_805F9378[];          // 0x805F9378 (string)
extern const char lbl_805F9380[];          // 0x805F9380 (string)
extern f32 lbl_805FF8A0;                   // 0x805FF8A0 (0.0f)
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
        fn_8018D2B8(lbl_80381868 + 0x238, fn_801EB708(hash));
        return NULL;
    }
    return el;
}

// Element lookup using the log strings lbl_803818A8 / lbl_80381AA0.
static u8* FindElementB(u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return NULL;
    }
    u8* el = (u8*)ScreenSetFindElement(set, hash);
    if (el == NULL) {
        fn_8018D2B8(lbl_80381AA0, fn_801EB708(hash));
        return NULL;
    }
    return el;
}

// Reads a string-table slot: table +0/+4 hold arrays, +0xC the count. Returns 0 if out of range.
static inline s32 TableEntry(u8* tbl, s32 off, u32 idx) {
    s32* arr = *(s32**)(tbl + off);
    if (arr != NULL && idx < *(u32*)(tbl + 0xc)) return arr[idx];
    return 0;
}

extern "C" {

// 0x801908AC
// Sets the playback rate of the element's UI animation handle (+0x10) and restarts it.
void UIElement_SetUIAnimPlaybackRate(u32 a0, f32 a1) {
    u8* el = FindElementA(a0);
    if (el != NULL) {
        if (*(void**)(el + 0x10) == NULL) {
            fn_8018D2B8(lbl_80381868 + 0x340, fn_801EB708(a0));
            return;
        }
        fn_801AB80C(a1);
        fn_801AB804(*(void**)(el + 0x10));
        fn_801AB748(*(void**)(el + 0x10));
    }
}

// 0x80190998
// Returns the current time of the element's UI animation (0.0 if none).
f32 UIElement_GetUIAnimCurrentTime(u32 a0) {
    u8* el = FindElementA(a0);
    if (el == NULL) return lbl_805FF8A0;
    void* h = *(void**)(el + 0x10);
    if (h == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x340, fn_801EB708(a0));
        return lbl_805FF8A0;
    }
    return fn_801AB804(h);
}

// 0x80190A60
// Sets the element's do-hit-test flag.
void UIElement_SetDoHitTest(u32 a0, u32 /*0x06*/ a1) {
    u8* el = FindElementB(a0);
    if (el != NULL) {
        fn_8019CC7C(el, a1);
    }
}

// 0x80190AFC
// Group element only (type +0x60 == 0): calls handler slot 0x2C with id 0x1000.
void UIGroup_DisableHitTest(u32 a0) {
    u8* el = FindElementA(a0);
    if (el != NULL) {
        if (*(s32*)(el + 0x60) != 0) {
            fn_8018D2B8(lbl_80381868 + 0x378, fn_801EB708(a0));
            return;
        }
        (*(void (**)(void*, s32))(*(u8**)(el + 0x138) + 0x2c))(el, 0x1000);
    }
}

// 0x80190BC8
// Group element only (type +0x60 == 0): calls handler slot 0x30 with id 0x1000.
void UIGroup_EnableHitTest(u32 a0) {
    u8* el = FindElementA(a0);
    if (el != NULL) {
        if (*(s32*)(el + 0x60) != 0) {
            fn_8018D2B8(lbl_80381868 + 0x378, fn_801EB708(a0));
            return;
        }
        (*(void (**)(void*, s32))(*(u8**)(el + 0x138) + 0x30))(el, 0x1000);
    }
}

// 0x80190C94
// Returns whether the cursor is over the element (0 if not found).
s32 UIElement_IsCursorOverItem(u32 a0) {
    u8* el = FindElementB(a0);
    if (el != NULL) {
        return fn_8019CD14(el);
    }
    return 0;
}

// 0x80190D28
// Text element only (type +0x60 == 1): sets the text (and font value) or clears it if NULL / "NULL".
void fn_80190D28(u32 a0, const char* a1, s32 a2) {
    u8* el = FindElementA(a0);
    if (el != NULL) {
        if (*(s32*)(el + 0x60) != 1) {
            fn_8018D370(lbl_80381868 + 0x3a8, fn_801EB708(a0));
            return;
        }
        if (a1 != NULL && strcmp(a1, lbl_805F9378) != 0) {
            fn_8019EA5C(el, a2);
            UISetText(el, a1, 1);
            return;
        }
        fn_8019EA5C(el, 0);
        UISetText(el, lbl_805F9380, 0);
    }
}

// 0x80190E50
// Sets a text element from a string-table id (high bit selects the second table; -1 = not found).
void UIText_SetString_TextID(u32 a0, u32 /*0x11*/ a1) {
    u8* el = FindElementA(a0);
    if (el != NULL) {
        if (*(s32*)(el + 0x60) != 1) {
            fn_8018D370(lbl_80381868 + 0x3a8, fn_801EB708(a0));
            return;
        }
        *(void**)(el + 0x13c) = gStringTable2;
        const char* str;
        if (a1 == 0xFFFFFFFFu) {
            str = lbl_80354800;
        } else if ((a1 & 0x80000000u) && gStringTable2 != NULL) {
            u8* t = (u8*)gStringTable2;
            const char** arr = *(const char***)(t + 0x0);
            u32 idx = a1 & 0x7FFFFFFFu;
            if (arr != NULL && idx < *(u32*)(t + 0xc)) str = arr[idx];
            else str = lbl_805F8200;
        } else {
            u8* t = (u8*)gStringTable;
            const char** arr = *(const char***)(t + 0x0);
            if (arr != NULL && a1 < *(u32*)(t + 0xc)) {
                str = arr[a1];
            } else {
                str = lbl_805F8200;
                if (a1 == 0xFFFFFFFFu) str = lbl_8035480C;
            }
        }
        s32 v;
        if (a1 == 0xFFFFFFFFu) {
            v = 0;
        } else if ((a1 & 0x80000000u) && gStringTable2 != NULL) {
            v = TableEntry((u8*)gStringTable2, 0x4, a1 & 0x7FFFFFFFu);
        } else {
            v = TableEntry((u8*)gStringTable, 0x4, a1);
        }
        fn_80190D28(a0, str, v);
    }
}

// 0x80191044
// Returns the current string of a text element (NULL if not found / not a text element).
ScriptValue UIText_GetString(u32 a0) {
    u8* el = FindElementA(a0);
    if (el == NULL) return 0;
    if (*(s32*)(el + 0x60) != 1) {
        fn_8018D370(lbl_80381868 + 0x3a8, fn_801EB708(a0));
        return 0;
    }
    return (ScriptValue)UITextGetString(el);
}

// 0x8019110C
// Re-renders a text element.
void UIText_RefreshText(u32 a0) {
    u8* el = FindElementA(a0);
    if (el != NULL) {
        if (*(s32*)(el + 0x60) != 1) {
            fn_8018D370(lbl_80381868 + 0x3a8, fn_801EB708(a0));
            return;
        }
        fn_8019DC58(el);
    }
}

}  // extern "C"
