// Small functions in 0x8011xxxx.
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void     operator_delete(void* p);                               // 0x801D426C
void     SysSprintf(char* buf, const char* fmt, ...);            // 0x80142EFC
void     MonsterGetName(u8* m, char* out, s32 size);             // 0x8021C968
BOOL     EntryAllowedAtRank(u8* e);                              // 0x80219030
BOOL     LabTakeMaterial(u8* inv, s32 index, s32 kind, u32 amount); // 0x80219ED4
void     fn_80218588(u8* lab, s32 id);                           // 0x80218588
void     fn_802186D0(u8* lab, s32 id);                           // 0x802186D0
void     fn_8011A04C(void);                                      // 0x8011A04C
void     fn_80116BE8(u8* p);                                     // 0x80116BE8 (base ctor, sets vtable)
void     fn_8011813C(u8* p);                                     // 0x8011813C
}

extern "C" {
extern const char lbl_805F8EF0[];   // 0x805F8EF0 (format string)
extern u8  lbl_805FB248[];          // 0x805FB248 (section table pointer)
extern u8  lbl_805FBBE0[];          // 0x805FBBE0 (save block pointer)
extern f32 lbl_805FECD0;            // 0x805FECD0 (float constant)
extern u8  lbl_805FB260[];          // 0x805FB260 (byte flag)
extern u8  lbl_8036D028[];          // 0x8036D028 (vtable)
extern u8  lbl_8036D118[];          // 0x8036D118 (vtable)
extern u8  lbl_803E6B28[];          // 0x803E6B28 (list entry record)
extern u8  lbl_803E6B34[];          // 0x803E6B34 (list entry record)
extern void* gLab;                  // 0x805FBBC8
}

extern "C" {

// 0x80116C04
// Deleting destructor for the empty record: frees it when del > 0.
u8* fn_80116C04(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80116C44
// Formats two ints from the record (+4, +8) into the text buffer at +0xC and returns that buffer.
char* fn_80116C44(u8* p) {
    SysSprintf((char*)(p + 0xc), lbl_805F8EF0, *(s32*)(p + 0x4), *(s32*)(p + 0x8));
    return (char*)(p + 0xc);
}

// 0x80116CA8
// Resets the record: six floats at +0 and two at +0x24/+0x28 to the default constant,
// integers at +0x18, +0x1C and +0x20 cleared.
void fn_80116CA8(u8* p) {
    f32 v = lbl_805FECD0;
    *(f32*)(p + 0x0)  = v;
    *(f32*)(p + 0x4)  = v;
    *(f32*)(p + 0x8)  = v;
    *(f32*)(p + 0xc)  = v;
    *(f32*)(p + 0x10) = v;
    *(f32*)(p + 0x14) = v;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1c) = 0;
    *(s32*)(p + 0x20) = 0;
    *(f32*)(p + 0x24) = v;
    *(f32*)(p + 0x28) = v;
}

// 0x80116D00
// Deleting destructor for the empty record: frees it when del > 0.
u8* fn_80116D00(u8* p, s32 del) {
    if (p) {
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80117090
// Constructor: base init, clears +0x20 and installs the derived vtable.
u8* fn_80117090(u8* p) {
    fn_80116BE8(p);
    *(s32*)(p + 0x20) = 0;
    *(u8**)p = lbl_8036D028;
    return p;
}

// 0x801170D4
// Deleting destructor: base dtor (no free), then frees the record when del > 0.
u8* fn_801170D4(u8* p, s32 del) {
    if (p) {
        fn_80116C04(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80117970
// Returns the monster name stored in the record at +0x40 (filled from the monster at +0x20),
// or NULL when no monster is attached.
char* fn_80117970(u8* p) {
    u8* m = *(u8**)(p + 0x20);
    if (m) {
        MonsterGetName(m, (char*)(p + 0x40), 0x10);
        return (char*)(p + 0x40);
    }
    return NULL;
}

// 0x80117F34
// Resets the list state: starts at entry 0 when none is set, clears the selection and
// flags, then hands off to fn_8011813C.
void fn_80117F34(u8* p) {
    if (*(s32*)(p + 0x1c) == -1) {
        *(s32*)(p + 0x28) = 0;
        *(s32*)(p + 0x20) = 3;
    }
    s32 cur = *(s32*)(p + 0x28);
    *(s32*)(p + 0x2c) = -1;
    *(s32*)(p + 0xd58) = 0;
    *(u8*)(p + 0x14d8) = 1;
    *(s32*)(p + 0x34) = 0;
    *(s32*)(p + 0x14dc) = 0;
    *(s32*)(p + 0x8) = 2;
    *(s32*)(p + 0x4) = cur + 1;
    *(s32*)(p + 0x30) = 0;
    lbl_805FB260[0] = 0;
    fn_8011813C(p);
}

// 0x80118BDC
// Takes one unit of the selected list entry's material out of the lab inventory
// (entry kind at +4, item at +8). Does nothing when the selection is out of range or empty.
void fn_80118BDC(u8* s) {
    s32 idx = *(s32*)(s + 0x2c);
    if (idx < 0) return;
    if (idx >= *(s32*)(s + 0x34)) return;
    u8* e = *(u8**)(s + 0xd5c + idx * 4);
    if (!e) return;
    u8* inv = *(u8**)lbl_805FBBE0 + 0xa30;
    LabTakeMaterial(inv, *(s32*)(e + 0x8), *(s32*)(e + 0x4), 1);
}

// 0x80118E88
// Sets the selected key-ingredient index and its count, then loads the matching
// 12-byte row of the game database table into +0x20.
void fn_80118E88(u8* p, s32 a, s32 b) {
    *(s32*)(p + 0x1c) = a;
    *(s32*)(p + 0x28) = b;
    *(s32*)(p + 0x4)  = b + 1;
    u8* data = *(u8**)((u8*)gGameDb + 0xc);
    u8* tbl  = *(u8**)(data + 0x3e60);
    u8* row  = tbl + a * 12;
    *(s32*)(p + 0x20) = *(s32*)(row + 0x8);
}

// 0x801194E0
// Moves the list selection: stores the new position (raw sum a + 4*top) at +0x2C and scrolls the
// three-row window at +0x30 so the selected row stays visible.
void fn_801194E0(u8* p, s32 a) {
    s32 top = *(s32*)(p + 0x30);
    s32 v = a + top * 4;
    *(s32*)(p + 0x2c) = v;
    s32 row = v / 4;
    if (row < top) {
        *(s32*)(p + 0x30) = row;
        return;
    }
    if (row < top + 3) return;
    while (row >= *(s32*)(p + 0x30) + 3) {
        *(s32*)(p + 0x30) = *(s32*)(p + 0x30) + 1;
    }
}

// 0x8011966C
// Returns the value at +0x18 of the list entry at the current selection, or NULL when none.
u8* fn_8011966C(u8* p) {
    s32 idx = *(s32*)(p + 0x2c);
    u8* e = NULL;
    if (idx >= 0 && idx < 0x75) e = *(u8**)(p + 0xd5c + idx * 4);
    if (e) return *(u8**)(e + 0x18);
    return NULL;
}

// 0x801196F4
// Constructor: base init, then installs the derived vtable.
u8* fn_801196F4(u8* p) {
    fn_80116BE8(p);
    *(u8**)p = lbl_8036D118;
    return p;
}

// 0x80119730
// Deleting destructor: base dtor (no free), then frees the record when del > 0.
u8* fn_80119730(u8* p, s32 del) {
    if (p) {
        fn_80116C04(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80119810
// Stores the entry's id at +8 into the first list record (lbl_803E6B28), notifies the lab
// object when it exists, then runs fn_8011A04C.
void fn_80119810(u8* s) {
    *(s32*)(lbl_803E6B28 + 0x4) = *(s32*)(s + 0x8);
    if (gLab) fn_80218588((u8*)gLab, *(s32*)(s + 0x8));
    fn_8011A04C();
}

// 0x80119858
// Same as fn_80119810, but for the second list record (lbl_803E6B34) and fn_802186D0.
void fn_80119858(u8* s) {
    *(s32*)(lbl_803E6B34 + 0x4) = *(s32*)(s + 0x8);
    if (gLab) fn_802186D0((u8*)gLab, *(s32*)(s + 0x8));
    fn_8011A04C();
}

// 0x80119B00
// Returns 1 when both list records hold a valid entry (id at +4 is not -1), else 0.
BOOL fn_80119B00(void) {
    if (*(s32*)(lbl_803E6B28 + 0x4) == -1) return 0;
    if (*(s32*)(lbl_803E6B34 + 0x4) == -1) return 0;
    return 1;
}

// 0x80119B38
// Returns 0 when the lab object exists and EntryAllowedAtRank accepts it, else 1.
BOOL fn_80119B38(void) {
    if (gLab && EntryAllowedAtRank((u8*)gLab)) return 0;
    return 1;
}

// 0x801102A0
// Script callback: formats the current section's page record (fn_80116C44) and returns the
// buffer pointer as a BOOL, or 0 when there is no section or no page.
BOOL GetPageString(void) {
    u8* sec = *(u8**)lbl_805FB248;
    u8* p = NULL;
    if (sec) {
        s32 idx = *(s32*)sec;
        u8* obj = *(u8**)(sec + idx * 4 + 4);
        p = *(u8**)(obj + 0x4);
    }
    if (p) return (BOOL)(uintptr_t)fn_80116C44(p);
    return 0;
}

// 0x80110048
// Script callback: forwards the section id to the current section object's virtual slot 0x20.
// Does nothing when no section table exists or the current index is -1.
void SetSection(u32 a0) {
    u8* sec = *(u8**)lbl_805FB248;
    if (!sec) return;
    s32 idx = *(s32*)sec;
    if (idx == -1) return;
    u8* obj = *(u8**)(sec + idx * 4 + 4);
    u8* vt = *(u8**)obj;
    (*(void (**)(u8*, u32))(vt + 0x20))(obj, a0);
}

}  // extern "C"
