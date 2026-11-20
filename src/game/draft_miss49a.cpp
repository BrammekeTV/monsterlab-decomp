// UI slider, UI sound list and UI texture list script callbacks, plus small UI helpers
// (0x800D6B0C, 0x8011AD88, 0x801A65A0, 0x801A665C) from the 0x80188xxx-0x801A6xxx range.
#include <string.h>
#include "types.h"
#include "game/script_callbacks.h"

extern "C" {
void*       ScreenSetsCurrent(void* sets);                       // 0x8018D1E4
void*       ScreenSetFindElement(void* set, u32 hash);           // 0x80198F18
void*       fn_80198F74(void* set, u32 hash);                    // 0x80198F74
void*       fn_80198FCC(void* set, u32 hash);                    // 0x80198FCC
void        fn_8018D370(const char* fmt, ...);                   // 0x8018D370 (debug log)
void        fn_8018D2B8(const char* fmt, ...);                   // 0x8018D2B8 (debug log)
const char* fn_801EB708(u32 hash);                               // 0x801EB708 (hash -> name)
u32         fn_8019552C(u8* a3, s32 a4);                         // 0x8019552C (leaf)
u32         fn_8019AD84(void* obj, s32 index);                   // 0x8019AD84
s32         fn_8019AE94(u8* a3);                                 // 0x8019AE94 (leaf)
void*       fn_8019AC78(void* set, BOOL a1);                     // 0x8019AC78
void*       fn_80109330(const char* name);                       // 0x80109330
f32         fn_801A4018(u8* a3);                                 // 0x801A4018 (leaf)
f32         fn_801A4034(u8* a3);                                 // 0x801A4034 (leaf)
s32         fn_801A403C(u8* a3, f32 f1);                         // 0x801A403C (leaf)
f32         fn_801A4048(u8* a3);                                 // 0x801A4048 (leaf)
s32         fn_801A4050(u8* a3, f32 f1);                         // 0x801A4050 (leaf)
f32         fn_801A405C(u8* a3);                                 // 0x801A405C (leaf)
s32         fn_801A4064(u8* a3, f32 f1);                         // 0x801A4064 (leaf)
f32         fn_801A4070(u8* a3);                                 // 0x801A4070 (leaf)
f32         fn_801A4084(u8* a3);                                 // 0x801A4084 (leaf)
void        fn_801A408C(u8* a3, s32 a4);                         // 0x801A408C
void*       fn_801D40E0(s32 n);                                  // 0x801D40E0
void*       fn_80209814(void* v);                                // 0x80209814
s32         PlaySoundAt(void* sound, int a, const void* pos, int b); // 0x80209ABC

// Screen-set debug strings (lbl_80381868 table: +0x40, +0x238, +0x530, +0x674, +0x708, +0x73c).
extern const char lbl_80381868[];
extern const char lbl_80381D68[];                                // 0x80381D68
extern const char lbl_80381EA0[];                                // 0x80381EA0
extern const char lbl_80381EDC[];                                // 0x80381EDC
extern const char lbl_80381F0C[];                                // 0x80381F0C
extern void* lbl_805FBBAC;                                       // 0x805FBBAC (voice SFX handle)
extern "C++" void* gScreenSets;                                  // 0x805FB654

// Shared slider lookup: the screen set's element for hash, or NULL (logs the failure).
static u8* UISlider_FindElement(u32 hash) {
    void* scr = ScreenSetsCurrent(gScreenSets);
    if (scr == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        return NULL;
    }
    u8* e = (u8*)ScreenSetFindElement(scr, hash);
    if (e == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x238, fn_801EB708(hash));
    }
    return e;
}

// Logs a slider element whose type (+0x60) is not 3.
static void UISlider_LogBadType(u32 hash) {
    fn_8018D370(lbl_80381868 + 0x530, fn_801EB708(hash));
}
}

extern "C" void PartAnimKey(char* out, s32 limb, s32 defId);  // 0x802152FC

extern "C" {

// 0x801A65A0
// Counts the rows (outer index over +0x140) in which some entry of the list at +0x1BC has flag
// bit 0 set at +0x64; stops at the first such entry per row.
u32 fn_801A65A0(void* obj) {
    u8* o = (u8*)obj;
    u32 count = 0;
    for (s32 i = 0; i < *(s32*)(o + 0x140); i++) {
        for (s32 j = 0; j < *(s32*)(o + 0x13C); j++) {
            u8** slot = (u8**)(uintptr_t)fn_8019552C(*(u8**)(o + 0x1BC), i + j * *(s32*)(o + 0x140));
            u8* e = (slot != NULL) ? *slot : NULL;
            if (e != NULL && (*(s32*)(e + 0x64) & 1)) {
                count += 1;
                break;
            }
        }
    }
    return count;
}

// 0x801A665C
// Same count as fn_801A65A0 with the loops swapped (outer over +0x13C, inner over +0x140).
u32 fn_801A665C(void* obj) {
    u8* o = (u8*)obj;
    u32 count = 0;
    for (s32 i = 0; i < *(s32*)(o + 0x13C); i++) {
        for (s32 j = 0; j < *(s32*)(o + 0x140); j++) {
            u8** slot = (u8**)(uintptr_t)fn_8019552C(*(u8**)(o + 0x1BC), j + i * *(s32*)(o + 0x140));
            u8* e = (slot != NULL) ? *slot : NULL;
            if (e != NULL && (*(s32*)(e + 0x64) & 1)) {
                count += 1;
                break;
            }
        }
    }
    return count;
}

// 0x800D6B0C
// Resets a 0x28-byte record: three zero words, three 0.0f floats, two -1 words, a zero byte at
// +0x20 and a zero word at +0x24.
void fn_800D6B0C(u8* p) {
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x4) = 0;
    *(s32*)(p + 0x8) = 0;
    *(f32*)(p + 0xC) = 0.0f;
    *(f32*)(p + 0x10) = 0.0f;
    *(f32*)(p + 0x14) = 0.0f;
    *(s32*)(p + 0x18) = -1;
    *(s32*)(p + 0x1C) = -1;
    *(u8*)(p + 0x20) = 0;
    *(s32*)(p + 0x24) = 0;
}

// 0x8011AD88
// UI texture for the part in obj: "ui/msa_parts/<part key>" (+"_h" when flag is set), falling back
// to "ui/msa_parts/placeholder" when that texture is not loaded.
void* fn_8011AD88(u8* obj, s32 flag) {
    char name[0x100];
    char key[0x100];
    PartAnimKey(key, *(s32*)(obj + 0x18), *(s32*)(obj + 0x0));
    strcpy(name, "ui/msa_parts/");
    strcat(name, key);
    if (flag != 0) {
        strcat(name, "_h");
    }
    void* tex = fn_80109330(name);
    if (tex == NULL) {
        strcpy(name, "ui/msa_parts/");
        strcat(name, "placeholder");
        tex = fn_80109330(name);
    }
    return tex;
}

// 0x80192508
f32 UISlider_GetValue(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0.0f;
    if (*(s32*)(e + 0x60) == 3) return fn_801A4018(e);
    UISlider_LogBadType(a0);
    return 0.0f;
}

// 0x801925D0
s32 UISlider_GetValue_Int(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0;
    if (*(s32*)(e + 0x60) == 3) return (s32)fn_801A4018(e);
    UISlider_LogBadType(a0);
    return 0;
}

// 0x801926A4
void UISlider_IncValue(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return;
    if (*(s32*)(e + 0x60) == 3) {
        fn_801A408C(e, 1);
    } else {
        UISlider_LogBadType(a0);
    }
}

// 0x8019304C
void UISlider_SetIncValue(u32 a0, f32 a1) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return;
    if (*(s32*)(e + 0x60) == 3) {
        fn_801A4064(e, a1);
    } else {
        UISlider_LogBadType(a0);
    }
}

// 0x80193118
void UISlider_SetIncValue_Int(u32 a0, s32 a1) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return;
    if (*(s32*)(e + 0x60) == 3) {
        fn_801A4064(e, (f32)a1);
    } else {
        UISlider_LogBadType(a0);
    }
}

// 0x80192D00
void UISlider_SetMaxValue(u32 a0, f32 a1) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return;
    if (*(s32*)(e + 0x60) == 3) {
        fn_801A4050(e, a1);
    } else {
        UISlider_LogBadType(a0);
    }
}

// 0x80192DCC
void UISlider_SetMaxValue_Int(u32 a0, s32 a1) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return;
    if (*(s32*)(e + 0x60) == 3) {
        fn_801A4050(e, (f32)a1);
    } else {
        UISlider_LogBadType(a0);
    }
}

// 0x80192EB0
f32 UISlider_GetMaxValue(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0.0f;
    if (*(s32*)(e + 0x60) == 3) return fn_801A405C(e);
    UISlider_LogBadType(a0);
    return 0.0f;
}

// 0x80192F78
s32 UISlider_GetMaxValue_Int(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0;
    if (*(s32*)(e + 0x60) == 3) return (s32)fn_801A405C(e);
    UISlider_LogBadType(a0);
    return 0;
}

// 0x801929B4
void UISlider_SetMinValue(u32 a0, f32 a1) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return;
    if (*(s32*)(e + 0x60) == 3) {
        fn_801A403C(e, a1);
    } else {
        UISlider_LogBadType(a0);
    }
}

// 0x80192A80
void UISlider_SetMinValue_Int(u32 a0, s32 a1) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return;
    if (*(s32*)(e + 0x60) == 3) {
        fn_801A403C(e, (f32)a1);
    } else {
        UISlider_LogBadType(a0);
    }
}

// 0x80192B64
f32 UISlider_GetMinValue(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0.0f;
    if (*(s32*)(e + 0x60) == 3) return fn_801A4048(e);
    UISlider_LogBadType(a0);
    return 0.0f;
}

// 0x80192C2C
s32 UISlider_GetMinValue_Int(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0;
    if (*(s32*)(e + 0x60) == 3) return (s32)fn_801A4048(e);
    UISlider_LogBadType(a0);
    return 0;
}

// 0x801928EC
f32 UISlider_GetNormalizedCurrentValue(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0.0f;
    if (*(s32*)(e + 0x60) == 3) return fn_801A4084(e);
    UISlider_LogBadType(a0);
    return 0.0f;
}

// 0x80192824
f32 UISlider_GetNormalizedValue(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0.0f;
    if (*(s32*)(e + 0x60) == 3) return fn_801A4034(e);
    UISlider_LogBadType(a0);
    return 0.0f;
}

// 0x801932C4
s32 UISlider_GetIncValue_Int(u32 a0) {
    u8* e = UISlider_FindElement(a0);
    if (e == NULL) return 0;
    if (*(s32*)(e + 0x60) == 3) return (s32)fn_801A4070(e);
    UISlider_LogBadType(a0);
    return 0;
}

// 0x80194A64
// Sound handle for index a1 in the sound list of element a0, or 0 (logs) when it is missing.
ScriptValue UISoundList_GetSound(u32 a0, s32 a1) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* elem = fn_80198FCC(set, a0);
    if (elem == NULL) {
        fn_8018D370(lbl_80381EDC, a0);
        return 0;
    }
    u32 snd = fn_8019AD84(elem, a1);
    if (snd != 0) return (ScriptValue)snd;
    fn_8018D370(lbl_80381F0C, a1);
    return 0;
}

// 0x80194C04
// Plays a random sound effect from the sound list of element a0 (non-voice slot, flags 0).
void UISoundList_PlayRandomSfx(u32 a0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* elem = fn_80198FCC(set, a0);
    if (elem == NULL) {
        fn_8018D370(lbl_80381868 + 0x674, a0);
        return;
    }
    s32 count = fn_8019AE94((u8*)elem);
    if (count == 0) {
        fn_8018D370(lbl_80381868 + 0x73c);
        return;
    }
    s32 idx = IntervalRandomInt(0, count - 1);
    void* snd = (void*)(uintptr_t)fn_8019AD84(elem, idx);
    if (snd == NULL) {
        fn_8018D370(lbl_80381868 + 0x708, idx);
        return;
    }
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
    PlaySoundAt(snd, 0, NULL, 0);
}

// 0x80194E14
// Same as UISoundList_PlayRandomSfx but plays through the voice handle with flags 2.
void UISoundList_PlayRandomVoiceSfx(u32 a0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* elem = fn_80198FCC(set, a0);
    if (elem == NULL) {
        fn_8018D370(lbl_80381868 + 0x674, a0);
        return;
    }
    s32 count = fn_8019AE94((u8*)elem);
    if (count == 0) {
        fn_8018D370(lbl_80381868 + 0x73c);
        return;
    }
    s32 idx = IntervalRandomInt(0, count - 1);
    void* snd = (void*)(uintptr_t)fn_8019AD84(elem, idx);
    if (snd == NULL) {
        fn_8018D370(lbl_80381868 + 0x708, idx);
        return;
    }
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
    PlaySoundAt(snd, 0, NULL, 2);
}

// 0x801949D4
// Result of fn_8019AC78 for texture list a0 at a1, or 0 (logs) when a list or entry is missing.
ScriptValue UITextureList_GetNextTexture(u32 a0, BOOL a1) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* elem = fn_80198F74(set, a0);
    if (elem == NULL) {
        fn_8018D370(lbl_80381D68, a0);
        return 0;
    }
    ScriptValue r = (ScriptValue)(intptr_t)fn_8019AC78(elem, a1);
    if (r != 0) return r;
    fn_8018D370(lbl_80381EA0, a1);
    return 0;
}

}
