// Functions from build/drafts/draft_miss722.cpp that were verified against the asm.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

// Vtable slot call on a UI element: (obj, s32).
typedef void (*VFnI)(void*, s32);

extern "C" {
void*  ScreenSetsCurrent(void* sets);                 // 0x8018D1E4
void*  ScreenSetFindElement(void* set, u32 hash);     // 0x80198F18
void   UISetText(void* el, const char* text, s32 a);  // 0x8019EFC8
void   UIPlayAnim(void* el, u32 anim);                // 0x8019BE40
s32    SysSprintf(char* buf, const char* fmt, ...);   // 0x80142EFC
u32    SaveCheckProgressFlag(u8* s, u32 flag);        // 0x80225C90
s32    fn_8007532C(u8* p);                            // 0x8007532C
extern "C++" void* gScreenSets;                             // 0x805FB654
extern void* lbl_805FBBE0;                            // save block pointer (value, loaded by the asm)
extern const char lbl_805F88D8[];                     // sprintf format string for the "n/m" label
}

extern "C" {

// 0x80074DFC
// Per-frame update of the progress counter screen: steps the current slot index (capped at 9,
// backing off past an empty slot), refreshes the "n/m" label and the text element, and plays the
// show/hide anim on the next-button element depending on whether a further step is available.
void fn_80074DFC(u8* p) {
    void* set;
    u8* el;
    s32 id;
    s32 cur;
    s32 j;
    s32 var;
    char buf[16];

    set = ScreenSetsCurrent(gScreenSets);
    if (set) {
        id = *(s32*)(p + *(s32*)(p + 0x5C) * 4 + 0x30);
        if (id != 0) {
            el = (u8*)ScreenSetFindElement(set, (u32)id);
            if (el) {
                (*(VFnI*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
            }
        }
    }

    cur = *(s32*)(p + 0x5C) + 1;
    *(s32*)(p + 0x5C) = cur;
    if (cur >= 10) {
        *(s32*)(p + 0x5C) = 9;
    }
    id = *(s32*)(p + 0x5C);
    if (*(u32*)(p + id * 4 + 0x8) == 0x10B2u) {
        *(s32*)(p + 0x5C) = id - 1;
    }

    set = ScreenSetsCurrent(gScreenSets);
    if (set) {
        el = (u8*)ScreenSetFindElement(set, 0x0281E9F9u);
        if (el) {
            id = *(s32*)(p + *(s32*)(p + 0x5C) * 4 + 0x8);
            if ((u32)id != 0x10B2u) {
                UIText_SetString_TextID(*(u32*)el, (u32)id);
            }
        }
    }

    set = ScreenSetsCurrent(gScreenSets);
    if (set) {
        el = (u8*)ScreenSetFindElement(set, 0x121100C6u);
        if (el) {
            *(s32*)(p + 0x60) = *(s32*)(p + 0x5C);
            for (;;) {
                j = *(s32*)(p + 0x60);
                if (j >= 10 || *(u32*)(p + j * 4 + 0x8) == 0x10B2u) {
                    break;
                }
                *(s32*)(p + 0x60) = j + 1;
            }
            cur = *(s32*)(p + 0x5C);
            SysSprintf(buf, lbl_805F88D8, cur + 1, j);
            UISetText(el, buf, 1);
        }
    }

    set = ScreenSetsCurrent(gScreenSets);
    if (set) {
        id = *(s32*)(p + *(s32*)(p + 0x5C) * 4 + 0x30);
        if (id != 0) {
            el = (u8*)ScreenSetFindElement(set, (u32)id);
            if (el) {
                (*(VFnI*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
            }
        }
    }

    if (*(s32*)(p + 0x5C) == *(s32*)(p + 0x60) - 1) {
        *(s32*)(p + 0x64) = 1;
    }

    set = ScreenSetsCurrent(gScreenSets);
    if (set) {
        el = (u8*)ScreenSetFindElement(set, 0x24EFD141u);
        if (el) {
            if (*(s32*)(p + 0x64) != 0) {
                var = 1;
            } else {
                id = fn_8007532C(p);
                if (id != -1 && SaveCheckProgressFlag((u8*)lbl_805FBBE0, (u32)id) != 0) {
                    var = 1;
                } else {
                    var = 0;
                }
            }
            if (var) {
                if (!(*(u32*)(el + 0x64) & 0x200u)) {
                    (*(VFnI*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x200);
                    UIPlayAnim(el, 0xA8A7883Eu);
                }
            } else if (*(u32*)(el + 0x64) & 0x200u) {
                (*(VFnI*)(*(u8**)(el + 0x138) + 0x30))(el, 0x200);
                UIPlayAnim(el, 0xCBC1BD99u);
            }
        }
    }
}

}
