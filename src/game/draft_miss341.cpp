#include <stdint.h>

#include "types.h"

extern "C" {
const char* AnimSetName(s32 id);
u8* fn_801BC910(void);
s32 fn_8004E594(u8* p);
char* fn_8004E8D4(u8* a3, u8* a4, s32 a5);
int SysSprintf(char* buf, const char* fmt, ...);
void* fn_801D40E0(s32 n);
u8* fn_8020E7EC(u8* p, u8* a, u8* b);
void fn_80025524(u8* obj, f32* a, f32* b);
void fn_801C83AC(u8* mgr, u8* item);
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);
void TextureRelease(void* tex);
ScriptValue UITextureList_GetTexture(u32 hash, s32 index);
void UIImage_SetTexture(void* img, void* tex);
void UIElement_SetColor(void* el, u32 color);
void UISetText(void* el, const char* text, s32 a);
void UIScreen_Push(u32 hash);
void UIElement_PlayUIAnim(u32 element, u32 anim);
void* ScreenSetsCurrent(void* sets);
void* ScreenSetFindWidget(void* set, u32 hash);
void* UIWidgetFindChild(void* widget, u32 hash);
void UIText_SetString_TextID(u32 hash, s32 textId);
s32 WorldGetPhase(void* world);
void PauseAllExplorationCharacters(void);
f32 sinf(f32 x);

extern f32 lbl_805FC928;
extern f32 lbl_805FC93C;
extern f32 lbl_805FC95C;
extern f32 lbl_805FC960;
extern f32 lbl_805FC964;
extern f32 lbl_805FE380;
extern f32 lbl_805FDB20;
extern const char* const lbl_80389058[];
extern const char lbl_80358FD8[];
extern u8 lbl_805FB0E3[];
extern u8* lbl_805FB0FC;
extern void* lbl_805FB208[];
extern void* lbl_805FAFE0;
extern u8 lbl_8047C760[];
extern f32 lbl_805FD300;
}

extern u8 gGame[];
extern void* gScreenSets;
extern f32 lbl_805F8738;
extern f32 lbl_805FD2B8;

extern "C" {

// 0x80007C20
s32 fn_80007C20(u8* arg0) {
    *(u8**)(arg0 + 0x48) = fn_801BC910();
    return 1;
}

// 0x80024EDC
void fn_80024EDC(u8* arg0) {
    // Two vectors handed to fn_80025524 (a38 = scaled offsets, a48 = base positions).
    f32 a38[4];
    f32 a48[4];
    u8* r6 = *(u8**)(arg0 + 0x90);
    f32 f104 = *(f32*)(arg0 + 0x104);
    f32 f10 = lbl_805FC95C * f104;

    a38[0] = f104 * lbl_805FC960;
    a38[1] = *(f32*)(arg0 + 0x108) * lbl_805FC964;
    a38[2] = *(f32*)(arg0 + 0x10C) * lbl_805FC928;

    a48[0] = *(f32*)(r6 + 0x68) + f10;
    a48[1] = *(f32*)(r6 + 0x6C) + lbl_805FC928;
    a48[2] = *(f32*)(r6 + 0x70) + lbl_805FC928;
    a48[3] = lbl_805FC93C + lbl_805FC928;

    fn_80025524(arg0, a48, a38);
}

// 0x8002FD5C
void fn_8002FD5C(void) {
    u8* obj = (u8*)lbl_805FAFE0;
    s32 state = *(s32*)(obj + 0x2A4);

    if (state == 1) {
        void* tex = (void*)UITextureList_GetTexture(0xEC1A6D6EU, *(s32*)(obj + 0x2A8));
        u8* o = (u8*)lbl_805FAFE0;
        UIImage_SetTexture(*(void**)(*(u8**)(o + 0x27C)), tex);
    } else if (state == 0 || state == 2) {
        void* tex = (void*)UITextureList_GetTexture(0x7AD719E2U, *(s32*)(obj + 0x2A8));
        u8* o = (u8*)lbl_805FAFE0;
        UIImage_SetTexture(*(void**)(*(u8**)(o + 0x27C)), tex);
    }

    // Advance the frame index and wrap it with the same bit sequence as the original.
    u8* o = (u8*)lbl_805FAFE0;
    *(s32*)(o + 0x2A8) = *(s32*)(o + 0x2A8) + 1;
    u32 t = (u32)*(s32*)(o + 0x2A8);
    u32 sg = t >> 31;
    u32 v = (t << 30) - sg;
    v = (v << 2) | (v >> 30);
    *(s32*)(o + 0x2A8) = (s32)(v + sg);
}

// 0x80031114
s32 fn_80031114(u8* arg0, f32* arg1) {
    // Even-odd point-in-polygon test: arg0 is {verts, count}, verts are (x, y) pairs.
    s32 count = *(s32*)(arg0 + 0x4);
    u8* verts = *(u8**)arg0;
    s32 inside = 0;
    s32 prev = count - 1;

    // The point is read inside the loop, as the original does (an empty polygon never touches it).
    for (s32 j = 0; j < count; j++) {
        f32 px = arg1[0];
        f32 py = arg1[1];
        f32* cur = (f32*)(verts + j * 8);
        f32* pv = (f32*)(verts + prev * 8);
        f32 xi = cur[0];
        f32 yi = cur[1];
        f32 xp = pv[0];
        f32 yp = pv[1];
        if ((yi <= py) != (yp <= py)) {
            f32 xint = xi + ((xp - xi) * (py - yi)) / (yp - yi);
            if (px < xint) {
                inside = (inside == 0);
            }
        }
        prev = j;
    }
    return inside;
}

// 0x8004D2C8
void* fn_8004D2C8(u8* arg0, const char* arg1, s32 arg2, s32 arg3) {
    u8* tbl = *(u8**)(arg0 + 0xC4C);
    void* r = arg0;
    if (tbl == NULL) {
        return r;
    }
    u8* ent = ((u8**)tbl)[arg2];
    r = ent;
    if (ent == NULL) {
        return r;
    }
    u8* base = *(u8**)(ent + 0x20);
    if (base != NULL) {
        base -= 0x264;
    }
    const char* name = AnimSetName(fn_8004E594(base + 0x108));
    char buf[64];
    SysSprintf(buf, lbl_80358FD8, lbl_80389058[arg2], name, arg1);

    u8* slot = arg0;
    s32 k = 0;
    do {
        r = *(void**)(slot + 0xC50);
        if (r != NULL) {
            r = fn_8004E8D4((u8*)r, (u8*)buf, arg3);
        }
        k++;
        slot += 4;
    } while (k < 5);
    return r;
}

// 0x8004FFB4
void* fn_8004FFB4(u8* arg0, u8* arg1) {
    void* r = fn_801D40E0(0x40);
    if (r != NULL) {
        u8* b = arg0;
        if (arg0 != NULL) {
            b = arg0 + 0x264;
        }
        r = fn_8020E7EC((u8*)r, arg1, b);
    }
    *(void**)(arg0 + 0x268) = r;
    return r;
}

// 0x80051834
void ShowConversationPopup(s32 textId) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        PauseAllExplorationCharacters();
    }
    lbl_805FB0E3[0] = 1;
    UIScreen_Push(0xBEF77BAEU);
    UIElement_PlayUIAnim(0x3BD6A2A1U, 0xA0DD766CU);
    void* widget = ScreenSetFindWidget(ScreenSetsCurrent(gScreenSets), 0xBEF77BAEU);
    if (widget != NULL) {
        lbl_805FB0FC = (u8*)UIWidgetFindChild(widget, 0xF8F01FECU);
        void* other = UIWidgetFindChild(widget, 0x9D8A77U);
        UISetText(lbl_805FB0FC, NULL, 1);
        UISetText(other, NULL, 1);
    }
    UIText_SetString_TextID(0xF8F01FECU, textId);
}

// 0x80051924
BOOL IsEventPopupVisible(void) {
    return lbl_805F8738 >= lbl_805FD2B8;
}

// 0x80059810
void fn_80059810(u8* arg0, u32 arg1, f32* arg2) {
    *(f32*)(arg0 + 0x160) = arg2[0];
    *(f32*)(arg0 + 0x164) = arg2[1];
    *(f32*)(arg0 + 0x168) = arg2[2];
    u8* item = *(u8**)(arg0 + 0x15C);
    if (item != NULL) {
        fn_801C83AC(lbl_8047C760, item);
        *(u8**)(arg0 + 0x15C) = NULL;
    }
    if (arg1 != 0) {
        u8* base = *(u8**)(arg0 + 0x4);
        f32 pos[4];
        pos[0] = *(f32*)(base + 0x0) + *(f32*)(arg0 + 0x160);
        pos[1] = *(f32*)(base + 0x4) + *(f32*)(arg0 + 0x164);
        pos[2] = *(f32*)(base + 0x8) + *(f32*)(arg0 + 0x168);
        pos[3] = lbl_805FD300;
        SpawnEffectAt(lbl_8047C760, arg1, pos, (u8**)(arg0 + 0x15C));
    }
}

// 0x8007FD98
void fn_8007FD98(u8* arg0, u8* arg1) {
    *(f32*)(arg0 + 0x24) = *(f32*)(arg0 + 0x24) + *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x28) + *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x2C) = *(f32*)(arg0 + 0x2C) + *(f32*)(arg1 + 0x8);
}

// 0x800925C0
void fn_800925C0(u8* arg0, u8* arg1, u32 color, s32 flags) {
    if (*(s32*)(arg0 + 0x8) == 2) {
        return;
    }
    u8* old = *(u8**)(arg0 + 0xC);
    if (old != NULL) {
        s32 refs = *(s32*)(old + 0x24) - 1;
        *(s32*)(old + 0x24) = refs;
        if (refs == 0) {
            TextureRelease(old);
        }
        *(u8**)(arg0 + 0xC) = NULL;
    }
    *(u8**)(arg0 + 0xC) = arg1;
    *(s32*)(arg1 + 0x24) = *(s32*)(arg1 + 0x24) + 1;
    *(u8*)(arg0 + 0x14) = (u8)flags;
    *(u8*)(arg0 + 0x34) = 1;
    UIImage_SetTexture(*(void**)(arg0 + 0x10), *(void**)(arg0 + 0xC));
    UIElement_SetColor(*(void**)(arg0 + 0x10), color);
}

// 0x80093C20
void fn_80093C20(u8* arg0, s32 arg1) {
    if (*(u8*)(arg0 + 0x24) == (u32)arg1) {
        return;
    }
    if (arg1 != 0) {
        u8* src = *(u8**)(arg0 + 0x10);
        f32 pos[4];
        pos[0] = *(f32*)(src + 0x8);
        pos[1] = *(f32*)(src + 0xC);
        pos[2] = *(f32*)(src + 0x10);
        pos[3] = lbl_805FDB20;
        SpawnEffectAt(lbl_8047C760, 0x040EE80CU, pos, (u8**)(arg0 + 0x14));
    } else {
        u8* a = *(u8**)(arg0 + 0x14);
        if (a != NULL) {
            fn_801C83AC(lbl_8047C760, a);
            *(u8**)(arg0 + 0x14) = NULL;
        }
        u8* b = *(u8**)(arg0 + 0x18);
        if (b != NULL) {
            fn_801C83AC(lbl_8047C760, b);
            *(u8**)(arg0 + 0x18) = NULL;
        }
    }
    *(u8*)(arg0 + 0x24) = (u8)arg1;
}

// 0x800C11F0
f32 fn_800C11F0(f32 farg0) {
    return sinf(lbl_805FE380 * farg0);
}

// 0x800FA924
void ShowSplashScreen(BOOL a0) {
    if ((u32)a0 >= 2) {
        return;
    }
    void* tex = lbl_805FB208[a0];
    if (tex == NULL) {
        return;
    }
    // 0xEB23F438 is the element hash passed as the first argument (tail call).
    return UIImage_SetTexture((void*)(uintptr_t)0xEB23F438U, tex);
}

}  // extern "C"
