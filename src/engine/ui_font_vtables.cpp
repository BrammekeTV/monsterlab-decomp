// UiInit helper: gives the sub-object at +0xCC of the three objects at +0x14, +0x18 and +0x1C its
// vtable, chosen by the kind stored at its +4 (0 or 1).
// Original: 0x8018C8F4 (auto_03_8018A3EC_text.s)
#include "types.h"

extern "C" {
extern u8 lbl_803821D0[];               // 0x803821D0: vtable for kind 0
extern u8 lbl_80382380[];               // 0x80382380: vtable for kind 1
}

static inline void SetVtable(u8* obj) {
    if (obj == NULL) return;
    u8* sub = *(u8**)(obj + 0xCC);
    if (sub == NULL) return;
    s32 kind = *(s32*)(sub + 4);
    if (kind == 1) {
        *(u8**)sub = lbl_80382380;
    } else if (kind == 0) {
        *(u8**)sub = lbl_803821D0;
    }
}

// 0x8018C8F4
extern "C" void fn_8018C8F4(u8* ui) {
    SetVtable(*(u8**)(ui + 0x14));
    SetVtable(*(u8**)(ui + 0x18));
    SetVtable(*(u8**)(ui + 0x1C));
}
