// Small functions batch miss121 (verified against asm).
#include "types.h"

// Callees and globals (prototypes match the other files that declare them).
extern "C" {
int    SysSprintf(char* buf, const char* fmt, ...);     // 0x80085A1C (sprintf)
void   UISetText(void* el, const char* text, s32 a);    // 0x8019EFC8
BOOL   ZoneIsLoaded(void* world, s32 zone);             // 0x8012F408
f32    __cvt_ull_flt(u64 v);                            // 0x8032FA98 (u64 -> float)
u64    fn_801425DC(u8* timer);                          // 0x801425DC (64-bit timer read)
void   fn_8018D2B8(const char* fmt, ...);               // 0x8018D2B8 (debug log)
u8*    fn_801B3308(void);                               // 0x801B3308
void   fn_801B3310(u8* p, u8* q);                       // 0x801B3310
void   fn_801B3378(u8* p, u8* key);                     // 0x801B3378
void   fn_801C83AC(u8* mgr, u8* item);                  // 0x801C83AC
void   fn_801C849C(u8* mgr, u8* item);                  // 0x801C849C
void*  fn_801D40E0(s32 n);                              // 0x801D40E0
u8* fn_801F61AC(u8* arg0, s32 arg1);                       // 0x801F61AC
char*  fn_80338BC0(char* s, const char* sub);           // 0x80338BC0 (strstr)
void   fn_800980C8(u8* obj, s32 mode);                  // 0x800980C8 (returns callee's r3; leaf decl says void)
s32    RaySceneTest(void* ray);                         // 0x80070458

extern u8   gWorldState[];                              // 0x803E6D00
extern s32  lbl_805FB418;                               // 0x805FB418 (connection flags)
extern s32  lbl_80400FE0;                               // 0x80400FE0 (4-entry table)
extern u8   lbl_8047C760[];                             // 0x8047C760 (effect manager)
extern u8   lbl_803B4DD0[];                             // 0x803B4DD0 (+0x38 locale block)
extern u8   lbl_805FB768[];                             // 0x805FB768 (timer object)
extern u8*  lbl_805FB678;                               // 0x805FB678 (current list head)
extern f32  lbl_803471A0[];                             // 0x803471A0 (f32 table)
extern f32  lbl_805F97F0;                               // 0x805F97F0 (float scale)
extern f32  lbl_805F95E0;                               // 0x805F95E0
extern f32  lbl_805FB918;
extern f32  lbl_805FB91C;
extern f32  lbl_805FB920;
extern f32  lbl_805FB924;
extern s32  lbl_805FB928;                               // timer high word
extern u32  lbl_805FB92C;                               // timer low word
extern u8   lbl_805FB930;                               // timer-averaging flag
extern f32  lbl_80600088;                               // 1000000.0f
extern f32  lbl_80600090;                               // lower clamp
extern f32  lbl_8060008C;                               // upper clamp (0.125f)
extern f32  lbl_805FF95C;
extern f32  lbl_805FF87C;
extern f32  lbl_805FF89C;
extern const char lbl_80381318[];                       // ShowCursor log string
extern const char lbl_80382CFC[];                       // "$" name token string
extern const char lbl_805F941C[];                       // printf format
}

extern "C" {

// 0x8000B98C
// Updates the status flags at +0x14C/+0x148 from the state word at +0x150.
void fn_8000B98C(u8* p) {
    if ((u32)*(s32*)(p + 0x150) - 0x20CE0000u == 0x505Du) {
        s32 flags = *(s32*)(p + 0x14C);
        if ((flags & 4) && (flags & 0x78)) {
            *(s32*)(p + 0x14C) = flags & ~4;
        }
    }
    *(s32*)(p + 0x148) = *(s32*)(p + 0x148) | *(s32*)(p + 0x14C);
}

// 0x800974AC
// Swaps the object stored at +0x1D8 (releasing the old one into +0x1DC) and
// caches a float lookup for the new object at +0x374.
void fn_800974AC(u8* p, u8* obj) {
    if (*(u8**)(p + 0x1D8) != NULL) {
        fn_800980C8(*(u8**)(p + 0x1D8), 0);
        *(u8**)(p + 0x1DC) = *(u8**)(p + 0x1D8);
    }
    *(u8**)(p + 0x1D8) = obj;
    if (obj != NULL) {
        s32 idx = (fn_800980C8(obj, 1), RaySceneTest(NULL));
        *(f32*)(p + 0x374) = lbl_803471A0[idx];
    }
}

// 0x80140B8C
// Clears the given bits in the connection flags lbl_805FB418.
void fn_80140B8C(s32 mask) {
    lbl_805FB418 &= ~mask;
}

// 0x801481FC
// Returns entry idx (0..3) of the 4-word table at lbl_80400FE0, or 0 if out of range.
s32 fn_801481FC(s32 idx) {
    if (idx < 0 || idx >= 4) {
        return 0;
    }
    return (&lbl_80400FE0)[idx];
}

// 0x8018DBB8
// Shows or hides the UI cursor; when shown, makes sure the cursor object exists
// (at +0x78) and positions it.
void UIEngine_ShowCursor(u8* p, s32 show) {
    u8* cursor = *(u8**)(p + 0x7C);
    if (cursor == NULL) {
        fn_8018D2B8(lbl_80381318);
        return;
    }
    u8* vt = *(u8**)(cursor + 0x138);
    if (show != 0) {
        ((void (*)(u8*, s32)) * (void**)(vt + 0x2C))(cursor, 2);
        if (fn_801B3308() != NULL) {
            if (*(u8**)(p + 0x78) == NULL) {
                u8* obj = (u8*)fn_801D40E0(0xBC);
                if (obj != NULL) {
                    fn_801F61AC(obj, 0);
                }
                *(u8**)(p + 0x78) = obj;
                *(f32*)(obj + 0xB0) = lbl_805FF87C;
                *(f32*)(obj + 0xB4) = lbl_805FF89C;
            }
            fn_801B3310(fn_801B3308(), *(u8**)(p + 0x78));
        }
    } else {
        ((void (*)(u8*, s32)) * (void**)(vt + 0x30))(cursor, 2);
        if (fn_801B3308() != NULL) {
            fn_801B3378(fn_801B3308(), *(u8**)(p + 0x78));
        }
    }
}

// 0x8019C9F4
// Walks the linked list at +0x4 and writes *value into the node whose key (+0x4)
// equals key.
void fn_8019C9F4(u8* p, u32 key, s32* value) {
    u8* node = *(u8**)(p + 0x4);
    while (node != NULL) {
        if (*(u32*)(node + 0x4) == key) {
            *(s32*)(node + 0x8) = *value;
            return;
        }
        node = *(u8**)node;
    }
}

// 0x8019CA2C
// Searches the three lists at +0x4, +0x8, +0xC for key; sets the first match's
// value to 0x3FFFF (list 1) or 0 (lists 2 and 3).
void fn_8019CA2C(u8* p, u32 key) {
    u8* node;
    for (node = *(u8**)(p + 0x4); node != NULL; node = *(u8**)node) {
        if (*(u32*)(node + 0x4) == key) {
            *(s32*)(node + 0x8) = 0x3FFFF;
            return;
        }
    }
    for (node = *(u8**)(p + 0x8); node != NULL; node = *(u8**)node) {
        if (*(u32*)(node + 0x4) == key) {
            *(s32*)(node + 0x8) = 0;
            return;
        }
    }
    for (node = *(u8**)(p + 0xC); node != NULL; node = *(u8**)node) {
        if (*(u32*)(node + 0x4) == key) {
            *(s32*)(node + 0x8) = 0;
            return;
        }
    }
}

// 0x8019DC10
// Returns 1 when the counter at +0x1E8 has reached (+0x1EC - 1) and the float at
// +0x14C is >= lbl_805FF95C, else 0.
u32 fn_8019DC10(u8* p) {
    u32 result = 0;
    if ((u32)*(s32*)(p + 0x1E8) >= (u32)(*(s32*)(p + 0x1EC) - 1)) {
        if (*(f32*)(p + 0x14C) >= lbl_805FF95C) {
            result = 1;
        }
    }
    return result;
}

// 0x8019EA7C
// Stores p as the current list head and marks every entry whose name string is
// non-empty (entries are 0x14 bytes, name pointer at +0) with -1 at +0xC.
void fn_8019EA7C(u8* p) {
    u8* entry = p;
    lbl_805FB678 = entry;
    while (**(s8**)entry != 0) {
        *(s32*)(entry + 0xC) = -1;
        entry += 0x14;
    }
}

// 0x8019EAA4
// Counts the output width of a format-like string: plain chars count 1, each '$'
// token counts 0x30 (0x80 when it is "userString") plus the length of the name
// that follows, using the character class table at lbl_803B4DD0 +0x38.
s32 fn_8019EAA4(char* text) {
    s32 count = 0;
    u8* table = *(u8**)(lbl_803B4DD0 + 0x38);
    char* p = text;
    while (*p != 0) {
        if (*p != '$') {
            count += 1;
            p += 1;
            continue;
        }
        count = count + ((fn_80338BC0(p, lbl_80382CFC) != NULL) ? 0x80 : 0x30);
        p += 1;
        for (;;) {
            s32 c = (s8)*p;
            if (c < 0) {
                break;
            }
            u8* ctype = *(u8**)(table + 0x8);
            if ((*(u16*)(ctype + c * 2) & 1) == 0) {
                break;
            }
            p += 1;
        }
    }
    return count;
}

// 0x8019F340
// Formats value with digits decimal places and sets it as the text of element p.
void fn_8019F340(u8* p, s32 digits, f32 value) {
    char fmtBuf[0x20];
    char textBuf[0x20];

    SysSprintf(fmtBuf, lbl_805F941C, digits);
    SysSprintf(textBuf, fmtBuf, value);
    UISetText(p, textBuf, 0);
}

// 0x801BC1C0
// Looks up id in the table at +0x20 (count at +0x2C, 0xC-byte entries, key at +0)
// and returns that entry's value at +8, or NULL if not found.
u8* fn_801BC1C0(u8* p, s32 id) {
    s32 count = *(s32*)(p + 0x2C);
    s32 i;
    for (i = 0; i < count; i++) {
        u8* arr = *(u8**)(p + 0x20);
        if ((u32)id == *(u32*)(arr + i * 0xC)) {
            return *(u8**)(arr + i * 0xC + 0x8);
        }
    }
    return NULL;
}

// 0x801C6634
// Returns 1 when the zone byte at +1 is loaded in gWorldState and the byte at +0
// is not 0xFF, else 0.
s32 fn_801C6634(u8* p) {
    s32 result = 0;
    if (*(u8*)p != 0xFF && ZoneIsLoaded(gWorldState, (s32)*(u8*)(p + 1)) != 0) {
        result = 1;
    }
    return result;
}

// 0x801ED050
// For each entry of the list at +0x4 (count at +0, 0xC8-byte stride, 8 slots of
// 4 bytes), releases the item at +0x88 into the effect manager (or detaches it
// when flag is 0) and clears the item's +0x88, +0x68 and +0xA8 fields.
void fn_801ED050(u8* p, s32 flag) {
    s32 count = *(s32*)p;
    s32 base = 0;
    for (s32 i = 0; i < count; i++, base += 0xC8) {
        if (i < 0) {
            continue;
        }
        for (s32 k = 0; k < 8; k++) {
            u8* item = *(u8**)(*(u8**)(p + 0x4) + base + k * 4 + 0x88);
            if (item != NULL) {
                if (flag != 0) {
                    fn_801C83AC(lbl_8047C760, item);
                } else {
                    fn_801C849C(lbl_8047C760, item);
                }
                *(s32*)(*(u8**)(p + 0x4) + base + k * 4 + 0x88) = 0;
                *(s32*)(*(u8**)(p + 0x4) + base + k * 4 + 0x68) = 0;
                *(s32*)(*(u8**)(p + 0x4) + base + k * 4 + 0xA8) = 0;
            }
        }
    }
}

// 0x801F0564
// Per-frame timer update: measures the 64-bit timer delta since the previous call,
// converts it to a clamped float interval (lbl_805FB920, max 0.125) and refreshes the
// smoothed values lbl_805FB91C/918 (when lbl_805FB930 is 0) and lbl_805F95E0.
void fn_801F0564(void) {
    u64 now = fn_801425DC(lbl_805FB768);
    u64 old = ((u64)(u32)lbl_805FB928 << 32) | (u64)lbl_805FB92C;
    u64 delta = now - old;
    u64 stamp = fn_801425DC(lbl_805FB768);
    lbl_805FB928 = (s32)(stamp >> 32);
    lbl_805FB92C = (u32)stamp;
    lbl_805FB920 = (__cvt_ull_flt(delta) * lbl_805F97F0) / lbl_80600088;
    f32 rate = (lbl_805F97F0 * __cvt_ull_flt(fn_801425DC(lbl_805FB768))) / lbl_80600088;
    lbl_805FB924 = rate;
    if (lbl_805FB920 < lbl_80600090) {
        lbl_805FB920 = lbl_8060008C;
    }
    if (lbl_805FB920 > lbl_8060008C) {
        lbl_805FB920 = lbl_8060008C;
    }
    if (lbl_805FB930 == 0) {
        lbl_805FB91C = rate * lbl_805F97F0;
        lbl_805FB918 = lbl_805FB920 * lbl_805F97F0;
    }
    lbl_805F95E0 = lbl_805FB918;
}

}  // extern "C"
