// Troublescope part-picker state (lbl_805FAF80), its part-image load/unload, the UI screen and
// element helpers (0x8001xxxx-0x8019xxxx), a 20-slot table search (0x800D3C60) and a CRC-32 helper
// (0x80143974). Verified against the asm.
//
// Note: the state struct and the save-block table hold 32-bit guest values, so 4-byte slots are
// read as u32 and turned into pointers with TsHandle (PC port needs a handle mapping there).
#include "types.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32 MonsterGetName(u8* m, char* out, s32 size);                  // 0x8021C968
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);       // 0x80209E70
int SysSprintf(char* buf, const char* fmt, ...);                  // 0x80142EF8
void operator_delete(void* p);                                    // 0x801D426C
void* ScreenSetsCurrent(void* sets);                              // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);                   // 0x80198E34
void* ScreenSetFindElement(void* set, u32 hash);                  // 0x80198F18
void TextureRelease(void* tex);                                   // 0x80184028
void fn_8003DCE0(u8* p);                                          // 0x8003DCE0
void fn_8004C718(u8* p, s32 idx, void* elem);                     // 0x8004C718
void* fn_8011AD88(void* def, s32 flag);                           // 0x8011AD88
void GameLogDebug(const char* fmt, ...);                           // 0x8018D1FC
void fn_8018D2B8(const char* fmt, ...);                           // 0x8018D2B8
void fn_8018D370(const char* fmt, ...);                           // 0x8018D370
s32 fn_8019892C(u8* set);                                      // 0x8019892C
s32 fn_80198C78(u8* set);                                      // 0x80198C78
void* fn_801D40E0(s32 n);                                         // 0x801D40E0
void* fn_80209814(void* v);                                       // 0x80209814
const char* FormatHashString(u32 hash);                                // 0x801EB708
}

extern "C++" void* gScreenSets;                  // 0x805FB654
extern "C++" u8* lbl_805FAF80;                   // Troublescope state (struct pointer, NULL when closed)
extern "C++" s32 lbl_805FAF84;                   // selected slot index in the save-block table
extern "C++" u8* lbl_805FAF88;                   // loaded part image object (refcount at +0x24)
extern "C++" s32 lbl_805F8288;                   // current part def index, -1 = none
extern "C++" u8 lbl_805FBBE0[];                  // save block pointer (*(u8**)lbl_805FBBE0)
extern "C++" void* lbl_805FBBAC;                 // voice SFX handle
extern "C++" u32 lbl_80347E68[];                 // CRC-32 table, 256 entries
extern "C++" const char lbl_803818A8[];
extern "C++" const char lbl_803818D0[];
extern "C++" const char lbl_80381994[];
extern "C++" const char lbl_80381AA0[];
extern "C++" const char lbl_8038191C[];
extern "C++" const char lbl_8038195C[];

static inline u8* TsHandle(u32 h) { return (u8*)(uintptr_t)h; }

extern "C" {

// 0x8000CD8C
void UIEngine_UnloadUISlot(s32 a0) {
    // Empty in the original (blr): nothing to unload.
    (void)a0;
}

// 0x800EFFB4
// Plays a global sound group entry; creates the voice SFX handle lbl_805FBBAC on first use.
void UIPlayGlobalGroupSound(u32 a0) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(a0, 0, 0, 0, 0, 0);
}

// 0x800D3C60
// Searches 20 in-use slots (flag byte at +0x5748, slot k at +(k+1)*0x428) in ascending order for
// the one whose field at +0x24 equals path. On a match clears its flag and returns db + k,
// otherwise returns db. f1/f2 (a, b) are not read by the asm.
s32 fn_800D3C60(void* db, void* path, f32 a, f32 b) {
    u8* arg0 = (u8*)db;
    u8* arg1 = (u8*)path;
    (void)a;
    (void)b;
    for (s32 k = 0; k < 0x14; k++) {
        if (*(u8*)(arg0 + 0x5748 + k) == 0) continue;
        if (arg1 == arg0 + (k + 1) * 0x428 + 0x24) {
            *(u8*)(arg0 + 0x5748 + k) = 0;
            return (s32)(uintptr_t)(arg0 + k);  // lint:ok (guest address value, as the caller uses it)
        }
    }
    return (s32)(uintptr_t)arg0;  // lint:ok (guest address value)
}

// 0x80143974
// CRC-32 (reflected, poly 0xEDB88320) of len bytes at data, continuing from seed. NULL data gives 0.
// Length is compared as signed (ble in the asm): len <= 0 returns the seed unchanged.
u32 fn_80143974(s32 seed, void* data, u32 len) {
    const u8* buf = (const u8*)data;
    if (buf == NULL) return 0;
    u32 c = (u32)seed ^ 0xFFFFFFFFu;
    if ((s32)len > 0) {
        for (u32 i = 0; i < len; i++) {
            c = (c >> 8) ^ lbl_80347E68[(c ^ buf[i]) & 0xFF];
        }
    }
    return c ^ 0xFFFFFFFFu;
}

// 0x8001EF50
void Troublescope_Deinit(void) {
    u8* st = lbl_805FAF80;
    if (st == NULL) return;
    if (*(u32*)(st + 0xC) != 0) {
        for (s32 i = 0; i < 5; i++) {
            u8* p = TsHandle(*(u32*)(st + 0xC));
            u8* obj = TsHandle(*(u32*)(p + 0xC50 + i * 4));
            fn_8004C718(p, i, NULL);
            fn_8003DCE0(obj);
        }
    }
    *(s32*)(st + 0x4) = 0;
    *(u32*)(st + 0xC) = 0;
    operator_delete(st);
    lbl_805FAF80 = NULL;
}

// 0x8001F7BC
ScriptValue Troublescope_GetMonsterName(void) {
    u8* st = lbl_805FAF80;
    if (st != NULL && *(u32*)(st + 0x4) != 0) {
        MonsterGetName(TsHandle(*(u32*)(st + 0x4)), (char*)(st + 0x24), 0x10);
        return (ScriptValue)(st + 0x24);
    }
    return 0;
}

// 0x8001F4FC
// Moves the selection by a0 (wrapping over the part count at +0x18) and refreshes the "n/m" label.
void Troublescope_IncCreature(s32 a0) {
    u8* st = lbl_805FAF80;
    if (st == NULL) return;
    *(s32*)(st + 0x14) = *(s32*)(st + 0x10);
    s32 idx = *(s32*)(st + 0x10) + a0;
    *(s32*)(st + 0x10) = idx;
    s32 count = *(s32*)(st + 0x18);
    if (idx >= count) {
        idx = idx % count;
        *(s32*)(st + 0x10) = idx;
    }
    while (idx < 0) {
        idx += *(s32*)(st + 0x18);
        *(s32*)(st + 0x10) = idx;
    }
    u8* save = *(u8**)lbl_805FBBE0;
    *(u32*)(st + 0x4) = *(u32*)(save + 0xA30 + idx * 4);
    SysSprintf((char*)(st + 0x34), "%d/%d", *(s32*)(st + 0x10) + 1, *(s32*)(st + 0x18));
    *(s32*)(st + 0x1C) = 0;
    *(s32*)(st + 0x20) = 0;
}

// 0x8001FB88
// Releases the previous part image (refcount at +0x24, freed at zero), then loads the image for
// the current part def into lbl_805FAF88.
void Troublescope_LoadPartImage(void) {
    if (lbl_805FAF88 != NULL) {
        s32 refs = *(s32*)(lbl_805FAF88 + 0x24) - 1;
        *(s32*)(lbl_805FAF88 + 0x24) = refs;
        if (refs == 0) TextureRelease(lbl_805FAF88);
        lbl_805FAF88 = NULL;
    }
    if (lbl_805F8288 != -1) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, lbl_805F8288);
        PartDef copy = def;
        lbl_805FAF88 = (u8*)fn_8011AD88(&copy, 1);
    }
}

// Shared body of the Troublescope_Set* selectors: st+0x4 names a record whose field at `off` is a
// part handle. lbl_805F8288 gets that handle's def index (-1 when missing) and lbl_805FAF84 the
// position of the handle in the save-block table (+0xA58, count at +0x10EC).
static void TroublescopeSetSlot(s32 off) {
    u8* st = lbl_805FAF80;
    if (st == NULL) return;
    u32 p = *(u32*)(st + 0x4);
    if (p == 0) {
        lbl_805F8288 = -1;
        lbl_805FAF84 = 0;
        return;
    }
    u32 q = *(u32*)(TsHandle(p) + off);
    if (q == 0) {
        lbl_805F8288 = -1;
        lbl_805FAF84 = 0;
        return;
    }
    s32 idx = *(s32*)TsHandle(q);
    lbl_805F8288 = idx;
    lbl_805FAF84 = 0;
    if (idx == -1) return;
    u8* save = *(u8**)lbl_805FBBE0;
    s32 count = *(s32*)(save + 0x10EC);
    for (s32 i = 0; i < count; i++) {
        if (*(u32*)(save + 0xA58 + i * 4) == q) lbl_805FAF84 = i;
    }
}

// 0x8001F9A4
void Troublescope_SetHead(void) {
    TroublescopeSetSlot(0x4);
}

// 0x8001FA40
void Troublescope_SetLeftArm(void) {
    TroublescopeSetSlot(0xC);
}

// 0x8001F86C
void Troublescope_SetLegs(void) {
    TroublescopeSetSlot(0x8);
}

// 0x8001F908
void Troublescope_SetTorso(void) {
    TroublescopeSetSlot(0x14);
}

// 0x8001FADC
void Troublescope_SetRightArm(void) {
    TroublescopeSetSlot(0x10);
}

// 0x8001FC20
void Troublescope_UnLoadPartImage(void) {
    u8* img = lbl_805FAF88;
    if (img == NULL) return;
    s32 refs = *(s32*)(img + 0x24) - 1;
    *(s32*)(img + 0x24) = refs;
    if (refs == 0) TextureRelease(img);
    lbl_805FAF88 = NULL;
}

// 0x8018EF9C
void UIScreen_Pop(void) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_8038191C);
        return;
    }
    GameLogDebug(lbl_8038195C);
    fn_8019892C((u8*)set);
}

// 0x8018F088
void UIScreen_ClearStack(void) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return;
    }
    GameLogDebug(lbl_80381994);
    fn_80198C78((u8*)set);
}

// 0x8018F0EC
// Looks up widget a0 in the current screen set and calls its vtable command 2 (slot +0x2C when a1
// is set, +0x30 when clear). Logs and does nothing when the set or the widget is missing.
void UIScreen_SetVisibleFlag(u32 a0, u32 a1) {
    void* w = NULL;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_803818A8);
    } else {
        w = ScreenSetFindWidget(set, a0);
        if (w == NULL) {
            fn_8018D370(lbl_803818D0, FormatHashString(a0));  // logged twice, as in the original
        }
    }
    if (w == NULL) {
        fn_8018D370(lbl_803818D0, FormatHashString(a0));
        return;
    }
    u8* vt = *(u8**)((u8*)w + 0x138);
    if (a1 != 0) {
        void (*fn)(void*, s32) = (void (*)(void*, s32))(*(void**)(vt + 0x2C));
        fn(w, 2);
    } else {
        void (*fn)(void*, s32) = (void (*)(void*, s32))(*(void**)(vt + 0x30));
        fn(w, 2);
    }
}

// 0x8018F1D4
// Same as UIScreen_SetVisibleFlag, with command 1 instead of 2.
void UIScreen_SetEnableFlag(u32 a0, u32 a1) {
    void* w = NULL;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_803818A8);
    } else {
        w = ScreenSetFindWidget(set, a0);
        if (w == NULL) {
            fn_8018D370(lbl_803818D0, FormatHashString(a0));  // logged twice, as in the original
        }
    }
    if (w == NULL) {
        fn_8018D370(lbl_803818D0, FormatHashString(a0));
        return;
    }
    u8* vt = *(u8**)((u8*)w + 0x138);
    if (a1 != 0) {
        void (*fn)(void*, s32) = (void (*)(void*, s32))(*(void**)(vt + 0x2C));
        fn(w, 1);
    } else {
        void (*fn)(void*, s32) = (void (*)(void*, s32))(*(void**)(vt + 0x30));
        fn(w, 1);
    }
}

// 0x8018FAD8
// Sets the X position (f32 at +0x68) of element a0 in the current screen set and calls its
// vtable command 0x18 (slot +0x2C).
void UIElement_SetX(u32 a0, f32 a1) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return;
    }
    void* el = ScreenSetFindElement(set, a0);
    if (el == NULL) {
        fn_8018D2B8(lbl_80381AA0, FormatHashString(a0));
        return;
    }
    *(f32*)((u8*)el + 0x68) = a1;
    u8* vt = *(u8**)((u8*)el + 0x138);
    void (*fn)(void*, s32) = (void (*)(void*, s32))(*(void**)(vt + 0x2C));
    fn(el, 0x18);
}

}  // extern "C"
