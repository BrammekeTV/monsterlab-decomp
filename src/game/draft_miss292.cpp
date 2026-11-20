// Jump-table functions: save-flag id lookup, zone resource slot allocator, zone object
// teardown and the part/lab info panel refresh.
#include "types.h"
#include "game/part.h"

extern u8* gSaveSystem;                     // 0x805FBBE0
extern "C++" s32 gZoneResourceSize;         // 0x805FB94C
struct StringTable;
extern "C++" StringTable* gStringTable;     // 0x805FB910

extern "C" {
extern const char lbl_805F8200[];           // 0x805F8200 (empty string)
extern const char lbl_805F8D98[];           // 0x805F8D98 (format string)
extern f32 lbl_805FEAF8;                    // 0x805FEAF8 (float constant)
extern u8 lbl_8035FE60[];                   // 0x8035FE60 (vtable)
extern u8 lbl_8035FF30[];                   // 0x8035FF30 (vtable)
extern u8 lbl_80360060[];                   // 0x80360060 (vtable)
extern u8 lbl_803600B0[];                   // 0x803600B0 (vtable)
extern u8 lbl_803600E8[];                   // 0x803600E8 (vtable)
extern u8 lbl_80360120[];                   // 0x80360120 (vtable)
extern u8 lbl_80360168[];                   // 0x80360168 (vtable)

void  TextureRelease(void* tex);                                      // 0x80184028
s32   PartPower(s32 defIndex, s32 tier);                              // 0x8021B650
s32   fn_80109A0C(s32 id);                                            // 0x80109A0C
void* fn_8011ED38(s32 id);                                            // 0x8011ED38
void* fn_8011CC5C(s32 index);                                         // 0x8011CC5C
void* fn_80118858(s32* args);                                         // 0x80118858
void* fn_8011AE30(PartDef* def, s32 a);                               // 0x8011AE30
void  UIImage_SetTexture(u32 hash, void* tex);                        // 0x80191B68
void  UIText_SetString_TextID(u32 hash, s32 textId);                  // 0x80190B8C
void  UIText_SetString(u32 hash, const char* text);                   // 0x80190E48
void  UIElement_SetVisibleFlag(u32 hash, s32 on);
void  UIElement_PlayUIAnim(u32 element, u32 anim);
int   SysSprintf(char* buf, const char* fmt, ...);                    // 0x80142EFC

void  fn_800DEC28(u8* p, void* arg);                                  // 0x800DEC28
u8*   fn_800E2FE8(u8* p, void* a);                                     // 0x800E2FE8
void  fn_800E42E8(u8* p);                                             // 0x800E42E8
void  fn_800E5550(u8* p);                                             // 0x800E5550
void  fn_8009E818(u8* p);                                             // 0x8009E818
u8*   fn_800D00B4(u8* p);                                             // 0x800D00B4
void  fn_8012F8B8(u8* p, s32 a1, s32 a2);                             // 0x8012F8B8
void  fn_801DB1AC(u8* p);                                             // 0x801DB1AC
u8*   fn_8006F61C(u8* p);                                             // 0x8006F61C
u8*   fn_8005CA88(u8* p);                                             // 0x8005CA88
u8*   fn_80052530(u8* p, s32 type, void* a, s32 b);                   // 0x80052530
void  fn_8004B604(u8* p);                                             // 0x8004B604
u8*   fn_8004B498(u8* p);                                             // 0x8004B498
u8*   fn_8004E684(u8* p, s32 type, void* a, s32 b);                   // 0x8004E684
u8*   fn_8006CCC4(u8* p, s32 type, void* a, s32 b);                  // 0x8006CCC4
u8*   fn_8006CFA4(u8* p, s32 type, void* a, s32 b);                   // 0x8006CFA4
u8*   fn_8005CC30(u8* p, s32 type, void* a, s32 b);                   // 0x8005CC30
void  fn_800690D4(u8* p, s32 type, void* a, s32 b);                   // 0x800690D4
u8*   fn_800636BC(u8* p);                                             // 0x800636BC
u8*   fn_8006D2D0(u8* p);                                             // 0x8006D2D0
u8*   fn_8006307C(u8* p);                                             // 0x8006307C
u8*   fn_801B4334(u8* p, s32 type, void* a, s32 b);                   // 0x801B4334

// 0x8007532C
// Maps the current save-system mode (+0x108 of the save block) to a progress flag id; -1 if none.
s32 fn_8007532C(u8* unused) {
    s32 mode = *(s32*)(gSaveSystem + 0x108);
    switch (mode) {
    case 13: return 0x2f;
    case 14: return 0x25;
    case 15: return 0x27;
    case 16: return 0x2b;
    case 17: return 0x2c;
    case 18: return 0x2d;
    case 19: return 0x1e;
    case 20: return 0x1d;
    case 21: return 0x1a;
    case 22: return 0x20;
    case 23: return 0x1c;
    case 24: return 0x1f;
    case 26: return 0x23;
    case 27: return 0x1b;
    case 28: return 0x21;
    case 37: return 0x26;
    case 38: return 0x29;
    case 39: return 0x28;
    case 40: return 0x2a;
    case 41: return 0x2e;
    case 42: return 0x24;
    case 43: return 0x22;
    default: return -1;
    }
}

// 0x800E6CD0
// Refreshes the part/lab info panel from the selection in the save block: picks an icon, title,
// description text and optional number for the selected entry kind, then updates the UI elements.
void fn_800E6CD0(u8* p) {
    if (*(s32*)(gSaveSystem + 0x8e8) <= 0) return;

    *(u8*)(p + 0x11) = 1;
    *(f32*)(p + 0x14) = lbl_805FEAF8;

    s32 kind = 0;
    if (*(s32*)(gSaveSystem + 0x8e8) > 0) kind = *(s32*)(gSaveSystem + 0x744);
    s32 idx = 0;
    if (*(s32*)(gSaveSystem + 0x8e8) > 0) idx = *(s32*)(gSaveSystem + 0x748);
    void* icon = NULL;
    s32 descId = 0;
    s32 titleId = 0;
    s32 number = -1;
    s32 sel = 0;
    if (*(s32*)(gSaveSystem + 0x8e8) > 0) sel = *(s32*)(gSaveSystem + 0x75c);

    u8* old = *(u8**)p;
    *(s32*)(p + 0x24) = sel;
    if (old != NULL) {
        s32 refs = *(s32*)(old + 0x24) - 1;
        *(s32*)(old + 0x24) = refs;
        if (refs == 0) TextureRelease(old);
        *(void**)p = NULL;
    }

    if ((u32)kind <= 11) {
        u8* data = *(u8**)((u8*)gGameDb + 0xc);
        u8* icons = *(u8**)(data + 0x8930);   // 8-byte icon records, byte at +4 is the icon id
        switch (kind) {
        case 0:
            break;
        case 1: {
            u8* row = *(u8**)(data + 0x63c8) + idx * 16;
            titleId = 0xb70;
            icon = fn_8011ED38(*(s8*)(icons + 0x8c));
            descId = TextIdFromHash(*(u32*)(row + 4));
            break;
        }
        case 11: {
            titleId = 0xb79;
            s32 k = 0;
            if (*(s32*)(gSaveSystem + 0x8e8) > 0) k = *(s32*)(gSaveSystem + 0x754);
            if (k == 0) k = 0x10;
            u32 h = 0;
            if (*(s32*)(gSaveSystem + 0x8e8) > 0) h = *(u32*)(gSaveSystem + 0x750);
            descId = TextIdFromHash(h);
            icon = fn_8011ED38(*(s8*)(icons + k * 8 + 4));
            break;
        }
        case 2: {
            u32 h = 0;
            if (*(s32*)(gSaveSystem + 0x8e8) > 0) h = *(u32*)(gSaveSystem + 0x750);
            titleId = TextIdFromHash(h);
            s32 k = 0;
            if (*(s32*)(gSaveSystem + 0x8e8) > 0) k = *(s32*)(gSaveSystem + 0x754);
            if (k == 0) k = 0x14;
            icon = fn_8011ED38(*(s8*)(icons + k * 8 + 4));
            break;
        }
        case 4: {
            u8* row = *(u8**)(data + 0x10a8) + idx * 0x18;
            icon = fn_8011CC5C(idx);
            titleId = TextIdFromHash(*(u32*)(row + 4));
            number = (s32) * (f32*)(row + 0x10);
            break;
        }
        case 3: {
            titleId = 0xb73;
            icon = fn_8011ED38(*(s8*)(icons + 0xac));
            number = 0;
            if (*(s32*)(gSaveSystem + 0x8e8) > 0) number = *(s32*)(gSaveSystem + 0x74c);
            break;
        }
        case 5: {
            u8* row = *(u8**)(data + 0x4f00) + idx * 16;
            titleId = 0xb6f;
            s32 args[3] = {0, idx, 0};
            icon = fn_80118858(args);
            descId = TextIdFromHash(*(u32*)(row + 4));
            break;
        }
        case 6: {
            u8* row = *(u8**)(data + 0x5328) + idx * 0x18;
            titleId = 0xb77;
            s32 args[3] = {1, idx, 0};
            icon = fn_80118858(args);
            descId = TextIdFromHash(*(u32*)(row + 4));
            break;
        }
        case 7:
        case 8: {
            u8* row = *(u8**)(data + 0x6c18) + idx * 0x24;
            s32 defIdx = *(s32*)(row + 0x20);
            PartDef def;
            PartDbGetDef(&def, gGameDb, defIdx);
            s32 tier = 0;
            if (*(s32*)(gSaveSystem + 0x8e8) > 0) tier = *(s32*)(gSaveSystem + 0x74c);
            s32 power = PartPower(defIdx, tier);
            icon = fn_8011ED38(*(s8*)(icons + 0xf4));
            switch (tier) {
            case 1: titleId = TextIdFromHash(def.tierName[0]); break;
            case 2: titleId = TextIdFromHash(def.tierName[1]); break;
            case 3: titleId = TextIdFromHash(def.tierName[2]); break;
            case 4: titleId = TextIdFromHash(def.tierName[3]); break;
            default: break;
            }
            u8* scale = *(u8**)(data + 0x99d0);
            number = power * (s32) * (f32*)(scale + 0x674);
            break;
        }
        case 9: {
            titleId = 0xb72;
            PartDef def;
            PartDbGetDef(&def, gGameDb, idx);
            icon = fn_8011AE30(&def, 0);
            s32 tier = 0;
            if (*(s32*)(gSaveSystem + 0x8e8) > 0) tier = *(s32*)(gSaveSystem + 0x74c);
            switch (tier) {
            case 1: descId = TextIdFromHash(def.tierName[0]); break;
            case 2: descId = TextIdFromHash(def.tierName[1]); break;
            case 3: descId = TextIdFromHash(def.tierName[2]); break;
            case 4: descId = TextIdFromHash(def.tierName[3]); break;
            default: break;
            }
            break;
        }
        case 10: {
            u8* row = *(u8**)(data + 0x67f0) + idx * 16;
            s32 k = *(s32*)(row + 0xc);
            titleId = 0xb74;
            icon = fn_8011ED38(*(s8*)(icons + k * 8 + 4));
            descId = TextIdFromHash(*(u32*)(row + 4));
            break;
        }
        default:
            break;
        }
    }

    if (icon != NULL) {
        if (fn_80109A0C((s32)(intptr_t)icon) != 0) {
            UIImage_SetTexture(0x5881687A, icon);
            *(void**)p = icon;
        } else {
            *(void**)(p + 4) = icon;
        }
    }
    if (titleId != 0) UIText_SetString_TextID(0x1B191FC3, titleId);
    if (descId != 0) {
        UIText_SetString_TextID(0x5193DD13, descId);
    } else if (number >= 0) {
        char buf[64];
        const char* label = lbl_805F8200;
        u8* st = (u8*)gStringTable;
        u8* tbl = *(u8**)st;
        if (tbl != NULL && *(u32*)(st + 0xc) > 0x5fc) label = *(const char**)(tbl + 0x17f0);
        SysSprintf(buf, lbl_805F8D98, label, number);
        UIText_SetString(0x5193DD13, buf);
    } else {
        UIText_SetString_TextID(0x5193DD13, 0x557);
    }
    UIText_SetString_TextID(0xE7631CF7, 0x557);
    UIElement_SetVisibleFlag(0xE7631CF7, 0);
    UIElement_SetVisibleFlag(0x5881687A, 1);
    if (fn_80109A0C((s32)(intptr_t)icon) != 0) UIElement_PlayUIAnim(0x24542991, 0x5C877396);
}

// 0x8012E420
// Registers a zone object in the zone's slot table for its type (types 2/6/8/10/12 use the table
// at +0x12c/+0x98, others +0x120/+0x94), points obj+4 at its 0x40-byte record, runs the per-type
// constructor and flags the object (+0xb8 |= 4). Returns 1, or 0 when no slot is free.
s32 fn_8012E420(u8* ctx, u8* obj, s32 type, s32 zone) {
    if (zone < 0) return 0;
    u8* z = *(u8**)(*(u8**)ctx + 0x1a4) + zone * gZoneResourceSize;

    s32 countOff, slotsOff, capOff, recOff;
    switch (type) {
    case 2: case 6: case 8: case 10: case 12:
        countOff = 0x98; slotsOff = 0x12c; capOff = 0x130; recOff = 0x90;
        break;
    default:
        countOff = 0x94; slotsOff = 0x120; capOff = 0x124; recOff = 0x8c;
        break;
    }

    s32 n = *(s32*)(z + countOff);
    s32 slot = -1;
    u8** slots = *(u8***)(z + slotsOff);
    for (s32 i = 0; i < n; i++) {
        if (slots[i] == NULL) {
            slot = i;
            break;
        }
    }
    if (slot == -1 && n < *(s32*)(z + capOff) - 1) {
        slot = n;
        *(s32*)(z + countOff) = *(s32*)(z + countOff) + 1;
    }
    if (slot == -1) return 0;
    (*(u8***)(z + slotsOff))[slot] = obj;
    *(u8**)(obj + 4) = *(u8**)(z + recOff) + slot * 0x40;

    if (obj != NULL) {
        void* rec = *(void**)(obj + 4);
        switch (type) {
        case 2:  fn_8006F61C(obj); break;
        case 3:  fn_8005CA88(obj); break;
        case 4:  fn_80052530(obj, type, rec, 1); break;
        case 13: fn_8004B604(obj); break;
        case 14: fn_8004B498(obj); break;
        case 12: fn_8004E684(obj, type, rec, 1); break;
        case 15: fn_8006CCC4(obj, type, rec, 1); break;
        case 16: fn_8006CFA4(obj, type, rec, 1); break;
        case 5:  fn_8005CC30(obj, type, rec, 1); break;
        case 6:  fn_800690D4(obj, type, rec, 1); break;
        case 7:  fn_800636BC(obj); break;
        case 8:  fn_8006D2D0(obj); break;
        case 10: fn_8006307C(obj); break;
        default: fn_801B4334(obj, type, rec, 1); break;
        }
    }
    *(u32*)(obj + 0xb8) |= 4;
    return 1;
}

// 0x8012FD0C
// Tears down a zone's objects: runs fn_8012F8B8 on both slot tables and the +0x50 object, then
// resets each registered object by type (reinstalling its base vtable or running its cleanup).
void fn_8012FD0C(u8* unused, u8* z) {
    for (s32 i = 0; i < *(s32*)(z + 0x94); i++)
        fn_8012F8B8(*(u8**)(*(u8**)(z + 0x120) + i * 4), 1, 0);
    for (s32 i = 0; i < *(s32*)(z + 0x98); i++)
        fn_8012F8B8(*(u8**)(*(u8**)(z + 0x12c) + i * 4), 1, 0);
    fn_8012F8B8(*(u8**)(z + 0x50), 1, 0);
    fn_801DB1AC(z + 0xe4);

    u32 count = *(u8*)(z + 0xac);
    for (u32 i = 0; i < count; i++) {
        u8* obj = *(u8**)(*(u8**)(z + 0xa8) + i * 4);
        if (obj == NULL) continue;
        switch (*(u32*)(obj + 8)) {
        case 0:  fn_800DEC28(obj, NULL); *(u8**)obj = lbl_8035FE60; *(s32*)(obj + 8) = 0; break;
        case 1:  fn_800E2FE8(obj, (void*)1); break;
        case 2:  fn_800DEC28(obj, NULL); *(u8**)obj = lbl_8035FF30; *(s32*)(obj + 8) = 2; break;
        case 3:  fn_800DEC28(obj, NULL); *(u8**)obj = lbl_80360060; *(s32*)(obj + 8) = 3; break;
        case 8:  fn_800DEC28(obj, NULL); *(u8**)obj = lbl_80360120; *(s32*)(obj + 8) = 8; break;
        case 9:  fn_800DEC28(obj, NULL); *(u8**)obj = lbl_80360168; *(s32*)(obj + 8) = 9; break;
        case 4:  fn_800DEC28(obj, NULL); *(u8**)obj = lbl_803600E8; *(s32*)(obj + 8) = 4; break;
        case 5:  fn_800DEC28(obj, NULL); *(u8**)obj = lbl_803600B0; *(s32*)(obj + 8) = 5; break;
        case 15: fn_800E42E8(obj); break;
        case 16: fn_800E5550(obj); break;
        case 17: fn_8009E818(obj); break;
        case 18: fn_800D00B4(obj); break;
        default: break;
        }
    }
}

}  // extern "C"
