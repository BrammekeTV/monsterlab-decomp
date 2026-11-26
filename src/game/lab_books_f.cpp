// Lab books UI: page object helpers (0x8011A000-0x8011D7FF).
#include "types.h"

// Virtual slot helpers: each UI page object has a vtable; these are the slots used here.
typedef s32  (*SelIndexFn)(u8* obj, u32 hash);   // slot 0x1C: the index of the section named by the hash (-1 = none)
typedef void (*VoidFn)(u8* obj);       // slot 0x10: deactivate

// SysSprintf is defined in engine.cpp with C++ linkage.
extern "C" int SysSprintf(char* buf, const char* fmt, ...);

extern "C" {
struct StringTable;
extern "C++" StringTable* gStringTable;             // 0x805FB910 (string table pointer)
extern u8    lbl_805FBBE0[];           // 0x805FBBE0 (save block pointer)
extern const char lbl_805F8200[];      // 0x805F8200 (default format string)
extern const char lbl_805F8F54[];      // 0x805F8F54 (format string)
extern u8    lbl_8036D29C[];           // 0x8036D29C (UI page vtable)

u8*  fn_80116D00(u8* p, s32 del);      // 0x80116D00
u8*  fn_80116C04(u8* p, s32 del);      // 0x80116C04
void fn_80116BE8(u8* p);               // 0x80116BE8 (base ctor, sets vtable)
void fn_80116CA8(u8* p);               // 0x80116CA8
void operator_delete(void* p);         // 0x801D426C
u8   fn_8021EE94(u8* p);               // 0x8021EE94
s32  fn_8022EC08(u8* a, u8* b);        // 0x8022EC08 (return type assumed, tested for nonzero)
s32  fn_8020E4E8(s32 a);               // 0x8020E4E8 (return type assumed)
}

extern "C" {

// 0x8011A6BC
// True when the section named by `hash` (slot 0x1C maps the name to an index) is the open one (+0x18).
// The hash stays in r4 from the caller through to slot 0x1C.
s32 fn_8011A6BC(u8* p, u32 hash) {
    s32 cur = (*(SelIndexFn*)(*(u8**)p + 0x1C))(p, hash);
    return *(s32*)(p + 0x18) == cur;
}

// 0x8011A704
// Returns the book's section named by `hash` (slot 0x1C maps the name to an index), or NULL for an
// unknown name. The hash stays in r4 from the caller through to slot 0x1C.
u8* fn_8011A704(u8* p, u32 hash) {
    s32 idx = (*(SelIndexFn*)(*(u8**)p + 0x1C))(p, hash);
    if (idx < 0) return NULL;
    return *(u8**)(p + idx * 4 + 8);
}

// 0x8011A978
// Deleting destructor for the sub-object at +0x2CB4; frees p when del > 0. Returns p.
u8* fn_8011A978(u8* p, s32 del) {
    if (p != NULL) {
        fn_80116D00(p + 0x2CB4, -1);
        fn_80116C04(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8011AFEC
// Formats the page's delete/prompt text into the buffer at +0x2CE0 from the string table
// (entry 0x594 when present, else the default string) and the current item's value.
// Returns the buffer. (The asm returns a pointer; lab_books_c.cpp declares it as s32.)
u8* fn_8011AFEC(u8* p) {
    const char* fmt = lbl_805F8200;
    u8* strs = *(u8**)gStringTable;
    if (strs != NULL && (u32)*(u32*)((u8*)gStringTable + 0xC) > 0x594) {
        fmt = *(const char**)(strs + 0x1650);
    }
    s32 idx = *(s32*)(p + 0x259C);
    u8* entry = *(u8**)(p + idx * 4 + 0x25A8);
    s32 val = *(s32*)(entry + 0x2C);
    SysSprintf((char*)(p + 0x2CE0), fmt, val);
    return p + 0x2CE0;
}

// 0x8011BAB8
// Sets the page mode from a (via fn_8020E4E8); invalid values become 3. Changing the mode
// clears the value at +0x25A0.
void fn_8011BAB8(u8* p, s32 a) {
    s32 v = fn_8020E4E8(a);
    s32 old = *(s32*)(p + 0x2CAC);
    s32 mode = (v < 0 || v >= 3) ? 3 : v;
    *(s32*)(p + 0x2CAC) = mode;
    if (old != mode) *(s32*)(p + 0x25A0) = 0;
}

// 0x8011BE4C
// True when the current item (index +0x259C) exists and its flag at +0x5C is set.
s32 fn_8011BE4C(u8* p) {
    s32 i = *(s32*)(p + 0x259C);
    if (i < *(s32*)(p + 0x2CA8)) {
        u8* entry = *(u8**)(p + i * 4 + 0x25A8);
        if (*(s32*)(entry + 0x5C) != 0) return 1;
    }
    return 0;
}

// 0x8011C130
// True when the current item exists and its field at +0x58 is not -1.
s32 fn_8011C130(u8* p) {
    s32 i = *(s32*)(p + 0x259C);
    if (i < *(s32*)(p + 0x2CA8)) {
        u8* entry = *(u8**)(p + i * 4 + 0x25A8);
        if (*(s32*)(entry + 0x58) != -1) return 1;
    }
    return 0;
}

// 0x8011C168
// True when the current item exists and the save-block check (fn_8022EC08) passes for it.
s32 fn_8011C168(u8* p) {
    s32 i = *(s32*)(p + 0x259C);
    if (i < *(s32*)(p + 0x2CA8)) {
        u8* base = *(u8**)lbl_805FBBE0;
        u8* entry = *(u8**)(p + i * 4 + 0x25A8);
        s32 idx = *(s32*)(entry + 0x54);
        u8* elem = *(u8**)(base + idx * 4 + 0xA58);
        if (fn_8022EC08(base, elem) != 0) return 1;
    }
    return 0;
}

// 0x8011C2B4
// True when the current item exists and its save-block element passes fn_8021EE94.
s32 fn_8011C2B4(u8* p) {
    s32 i = *(s32*)(p + 0x259C);
    if (i < *(s32*)(p + 0x2CA8)) {
        u8* base = *(u8**)lbl_805FBBE0;
        u8* entry = *(u8**)(p + i * 4 + 0x25A8);
        s32 idx = *(s32*)(entry + 0x54);
        u8* elem = *(u8**)(base + idx * 4 + 0xA58);
        if (fn_8021EE94(elem) != 0) return 1;
    }
    return 0;
}

// 0x8011C738
// Deleting destructor for the sub-object at +0x1318; frees p when del > 0. Returns p.
u8* fn_8011C738(u8* p, s32 del) {
    if (p != NULL) {
        fn_80116D00(p + 0x1318, -1);
        fn_80116C04(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8011CF8C
// Returns word +0x14 of entry (cursor + a) in the table at +0x1088, or 0 when out of range.
s32 fn_8011CF8C(u8* p, s32 a) {
    s32 idx = *(s32*)(p + 0x12EC) + a;
    if (idx >= 0 && idx < *(s32*)(p + 0x1084)) {
        u8* e = *(u8**)(p + 0x1088 + idx * 4);
        return *(s32*)(e + 0x14);
    }
    return 0;
}

// 0x8011CFC0
// Returns word +0x04 of entry (cursor + a) in the table at +0x1088, or 0 when out of range.
s32 fn_8011CFC0(u8* p, s32 a) {
    s32 idx = *(s32*)(p + 0x12EC) + a;
    if (idx >= 0 && idx < *(s32*)(p + 0x1084)) {
        u8* e = *(u8**)(p + 0x1088 + idx * 4);
        return *(s32*)(e + 0x4);
    }
    return 0;
}

// 0x8011CFF4
// Returns word +0x0C of entry (cursor + a) in the table at +0x1088, or 0 when out of range.
s32 fn_8011CFF4(u8* p, s32 a) {
    s32 idx = *(s32*)(p + 0x12EC) + a;
    if (idx >= 0 && idx < *(s32*)(p + 0x1084)) {
        u8* e = *(u8**)(p + 0x1088 + idx * 4);
        return *(s32*)(e + 0xC);
    }
    return 0;
}

// 0x8011D150
// Formats the two values of the current pair (at +0x12F8/+0x12FC, indexed by +0x12F4)
// into the text buffer at +0x1344 and returns the buffer.
char* fn_8011D150(u8* p) {
    s32 k = *(s32*)(p + 0x12F4);
    u8* e = p + k * 8;
    SysSprintf((char*)(p + 0x1344), lbl_805F8F54, *(s32*)(e + 0x12FC), *(s32*)(e + 0x12F8));
    return (char*)(p + 0x1344);
}

// 0x8011D338
// Deactivates the current selection (slot 0x10 on the child), clears the pending flag at
// +0x1C and sets the selection index at +0x18 to -1.
void fn_8011D338(u8* p) {
    s32 idx = *(s32*)(p + 0x18);
    *(u8*)(p + 0x1C) = 0;
    if (idx != -1) {
        u8* child = *(u8**)(p + idx * 4 + 8);
        (*(VoidFn*)(*(u8**)child + 0x10))(child);
    }
    *(s32*)(p + 0x18) = -1;
}

// 0x8011D490
// Maps a page-name StringHash to an index: Statistics 1, QuestLog 2, Recipes 3, badges 0;
// any other hash gives -1. (r3 is not read; the hash is the second argument.)
s32 fn_8011D490(u8* p, s32 key) {
    (void)p;
    if (key == (s32)0xE94C961Du) return 1;   // "Statistics"
    if (key >= (s32)0xE94C961Du) {
        return (key == (s32)0x0FD3AF4Du) ? 2 : -1;  // "QuestLog"
    }
    if (key == (s32)0x9C2C1CAFu) return 3;   // "Recipes"
    if (key > (s32)0x9C2C1CAFu) return -1;
    if (key == (s32)0x880E941Cu) return 0;   // "badges"
    return -1;
}

// 0x8011D508
// Returns the book's section named by `hash` (slot 0x1C maps the name to an index), or NULL for an
// unknown name. The hash stays in r4 from the caller through to slot 0x1C.
u8* fn_8011D508(u8* p, u32 hash) {
    s32 idx = (*(SelIndexFn*)(*(u8**)p + 0x1C))(p, hash);
    if (idx < 0) return NULL;
    return *(u8**)(p + idx * 4 + 8);
}

// 0x8011D55C
// True when the section named by `hash` (slot 0x1C maps the name to an index) is the open one (+0x18).
// The hash stays in r4 from the caller through to slot 0x1C.
s32 fn_8011D55C(u8* p, u32 hash) {
    s32 cur = (*(SelIndexFn*)(*(u8**)p + 0x1C))(p, hash);
    return *(s32*)(p + 0x18) == cur;
}

// 0x8011D5C4
// Constructor: base init, installs the page vtable (lbl_8036D29C) and initialises the
// sub-object at +0x24. Returns p.
u8* fn_8011D5C4(u8* p) {
    fn_80116BE8(p);
    *(void**)p = (void*)lbl_8036D29C;
    fn_80116CA8(p + 0x24);
    return p;
}

// 0x8011D608
// Deleting destructor for the sub-object at +0x24; frees p when del > 0. Returns p.
u8* fn_8011D608(u8* p, s32 del) {
    if (p != NULL) {
        fn_80116D00(p + 0x24, -1);
        fn_80116C04(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

}  // extern "C"
