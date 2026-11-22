// Small functions in 0x8018xxxx (record/table helpers, texture release, anim frame queries).
#include <string.h>
#include "types.h"

extern "C" {
void  fn_80169704(void* a, void* b);                            // 0x80169704
void  fn_8015AAE8(void* p);                                     // 0x8015AAE8
u32   fn_8015A58C(u8* p);                                       // 0x8015A58C
void  fn_801731C8(void* p);                                     // 0x801731C8
void  fn_80330E20(void* p);                                     // 0x80330E20
void  fn_8018A1FC(void* p);                                     // 0x8018A1FC
void  fn_80187DB8(void* a, void* b, void* c);                   // 0x80187DB8
void  fn_80188C2C(void);                                        // 0x80188C2C
void  fn_8018A5F4(void* p);                                     // 0x8018A5F4
}

extern f32 lbl_805FF780;            // 0x805FF780
extern f32 lbl_805FF784;            // 0x805FF784
extern f32 lbl_805FF830;            // 0x805FF830 (default result)
extern f32 lbl_805FF838;            // 0x805FF838 (default result)
extern u8  lbl_80412B20[];          // 0x80412B20 (static record)
extern u8  lbl_80412B80[];          // 0x80412B80 (static record)
extern u8  lbl_80412BC0[];          // 0x80412BC0 (static table)
extern u8  lbl_80412BE0[];          // 0x80412BE0 (static record)
extern u8* lbl_805FB608;            // 0x805FB608 (global object pointer)
extern s32 lbl_805FB628;            // 0x805FB628
extern s32 lbl_805FB62C;            // 0x805FB62C
extern s32 lbl_805FB630;            // 0x805FB630

extern "C" {

// 0x80181D84
// Maps the mode at +0xCF8 to a code (2 for mode 2, 3 for mode 3, otherwise 1), stores it at +0xD
// of the linked object and sets the ready flag at +0xE of that object to 1.
void fn_80181D84(u8* p) {
    s32 mode = *(s32*)(p + 0xcf8);
    s32 code;
    if (mode == 2) code = 2;
    else if (mode == 3) code = 3;
    else code = 1;
    u8* obj = *(u8**)p;
    *(u8*)(obj + 0xd) = (u8)code;
    obj = *(u8**)p;
    *(u8*)(obj + 0xe) = 1;
}

// 0x80183F20
// Resets the static record at lbl_80412B20: six counters, the first word and two float defaults.
void fn_80183F20(void) {
    u8* g = lbl_80412B20;
    *(s32*)(g + 0x30) = 0;
    *(s32*)(g + 0x18) = 0;
    *(s32*)(g + 0x1c) = 0;
    *(s32*)(g + 0x20) = 0;
    *(s32*)(g + 0x24) = 0;
    *(s32*)(g + 0x4) = 0;
    *(s32*)(g + 0x28) = 0;
    *(s32*)(g + 0x2c) = 0;
    *(f32*)(g + 0x54) = lbl_805FF780;
    *(f32*)(g + 0x58) = lbl_805FF784;
}

// 0x80183F64
// Hands out the next 12-byte entry of the table at +0x28 (up to 0x1000 entries); NULL when full
// or when the table has not been allocated.
void* fn_80183F64(u8* p) {
    s32 n = *(s32*)(p + 0x34);
    if (n >= 0x1000) return NULL;
    u8* table = *(u8**)(p + 0x28);
    if (table == NULL) return NULL;
    *(s32*)(p + 0x34) = n + 1;
    return table + n * 0xc;
}

// 0x80183FCC
// Appends a 0x24-byte element to the pool at +0 (up to 0x4000 elements) and records its address
// in the list for slot idx, which holds up to 0x2000 entries. Returns the new element, or NULL
// when either limit is reached.
// The list pointer (+8 + idx*4) and the count (+0x18 + idx*4) ranges overlap for idx >= 4 in the
// asm; kept as is.
void* fn_80183FCC(u8* p, s32 idx) {
    u8* slot = p + idx * 4;
    s32 count = *(s32*)(slot + 0x18);
    if (count >= 0x2000) return NULL;
    s32 used = *(s32*)(p + 0x4);
    if (used >= 0x4000) return NULL;
    *(s32*)(p + 0x4) = used + 1;
    u8* elem = *(u8**)p + used * 0x24;
    u8* list = *(u8**)(slot + 0x8);
    *(void**)(list + count * 4) = elem;
    *(s32*)(slot + 0x18) = count + 1;
    return elem;
}

// 0x80184028
// Releases a texture object: frees its backing block (header 4 bytes before the pointer at +0x30)
// through the global object's vtable slot 1, then clears the handle at +0.
void TextureRelease(void* tex) {
    u8* p = (u8*)tex;
    u8* data = *(u8**)(p + 0x30);
    u8* block = data ? data - 4 : NULL;
    if (block) fn_80169704(block, p);
    fn_8015AAE8(p + 4);
    *(s32*)p = 0;
    // The free function at +4 of the heap object, called as (p, heap).
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(p, lbl_805FB608);
}

// 0x8018409C
// Byte size of the record at p: the fixed 0x38-byte header plus the length of the name at +0x35
// plus the size reported by fn_8015A58C for the sub-record at +4.
u32 fn_8018409C(u8* p) {
    u32 n = (u32)strlen((const char*)(p + 0x35)) + 0x38;
    return n + fn_8015A58C(p + 4);
}

// 0x80184278
// Size of the record: base 0x30 plus 24 bytes per +0xC count, plus
// 4 bytes per +0x14 count, 6 bytes per +0x1C count and 8 bytes per +0x24 count.
u32 fn_80184278(u8* p) {
    s32 a = *(s32*)(p + 0xc);
    s32 b = *(s32*)(p + 0x14);
    s32 c = *(s32*)(p + 0x1c);
    s32 d = *(s32*)(p + 0x24);
    return 0x30 + a * 24 + b * 4 + c * 6 + d * 8;
}

// 0x80184414
// If the owner's stored head (+0xC0 of node, minus its header) is this owner, releases node's
// sub-object at +0xB8 and, when flagged in node+0xC, decrements the owner's counter at +0x38.
void fn_80184414(u8* owner, u8* node) {
    u8* arr = *(u8**)(node + 0xc0);
    u8* head = arr ? arr - 4 : NULL;
    if (head == owner) {
        fn_801731C8(node + 0xb8);
        if (*(s32*)(node + 0xc) & 4) {
            s32 n = *(s32*)(owner + 0x38);
            *(s32*)(owner + 0x38) = n - 1;
        }
    }
}

// 0x80184590
// Same pattern as fn_80184414 with the 0x10-byte header, sub-object at +4 and a halfword flag at
// +0x12 that decrements the owner's counter at +0x3C.
void fn_80184590(u8* owner, u8* node) {
    u8* arr = *(u8**)(node + 0xc);
    u8* head = arr ? arr - 0x10 : NULL;
    if (head == owner) {
        fn_801731C8(node + 0x4);
        if (*(s16*)(node + 0x12) & 4) {
            s32 n = *(s32*)(owner + 0x3c);
            *(s32*)(owner + 0x3c) = n - 1;
        }
    }
}

// 0x80187D18
// Frees the two sub-objects at +4 and +8 of p.
void fn_80187D18(u8* p) {
    fn_80330E20(*(void**)(p + 0x8));
    fn_80330E20(*(void**)(p + 0x4));
}

// 0x80187D50
// Walks the 12-byte entry table at +4 (count at +0): every entry with a non-null word at +4 is
// moved onto the list headed at +0xC, and its +4 and +8 words are cleared.
void fn_80187D50(u8* p) {
    for (s32 i = 0, off = 0; i < *(s32*)p; i++, off += 0xc) {
        u8* e = *(u8**)(p + 0x4) + off;
        if (*(s32*)(e + 0x4) == 0) continue;
        *(void**)(*(u8**)(e + 0x8)) = *(void**)(p + 0xc);
        *(void**)(p + 0xc) = *(void**)(*(u8**)(p + 0x4) + off + 0x4);
        *(void**)(*(u8**)(p + 0x4) + off + 0x8) = NULL;
        *(void**)(*(u8**)(p + 0x4) + off + 0x4) = NULL;
    }
}

// 0x80188E14
// Tears down the two static records: the table at lbl_80412BC0 and two sub-objects of lbl_80412B80.
void fn_80188E14(void) {
    fn_80187D18(lbl_80412BC0);
    fn_8015AAE8(lbl_80412B80);
    fn_8015AAE8(lbl_80412B80 + 0x20);
}

// 0x80188E5C
// Resets the static state: initialises lbl_80412BE0, runs the lbl_80412BC0 setup with p, clears
// its entries, zeroes the two counters at lbl_805FB628/2C, sets lbl_805FB630 to -1, then calls
// fn_80188C2C.
void fn_80188E5C(u8* p) {
    fn_8018A1FC(lbl_80412BE0);
    fn_80187DB8(lbl_80412BC0, lbl_80412BE0, p);
    fn_80187D50(lbl_80412BC0);
    lbl_805FB628 = 0;
    lbl_805FB62C = 0;
    lbl_805FB630 = -1;
    fn_80188C2C();
}

// 0x801894B8
// Zeroes the word array at +0x8C, one entry per count in the byte at +3.
void fn_801894B8(u8* p) {
    for (s32 i = 0; i < *(u8*)(p + 0x3); i++) {
        *(s32*)(p + 0x8c + i * 4) = 0;
    }
}

// 0x8018A3EC
// Runs fn_8018A5F4 on p and returns p.
u8* fn_8018A3EC(u8* p) {
    fn_8018A5F4(p);
    return p;
}

// 0x8018BDE0
// Index of the first entry in p's table (8-byte entries, count at +8) whose first word equals key,
// or -1 when there is no table or no match.
s32 fn_8018BDE0(u8* p, s32 key) {
    u8* table = *(u8**)p;
    if (table == NULL) return -1;
    s32 count = *(s32*)(table + 0x8);
    u8* arr = *(u8**)(table + 0xc);
    for (s32 i = 0; i < count; i++) {
        if (key == *(s32*)(arr + i * 8)) return i;
    }
    return -1;
}

// 0x8018BE30
// Ratio used by the attack timing: the sub-object at +0x24 of the second link divided by the
// value at +0x20 of the third; returns lbl_805FF838 when that value is <= t, and lbl_805FF830
// when a link is missing.
f32 fn_8018BE30(u8* p, f32 t) {
    u8* obj = *(u8**)p;
    if (obj == NULL) return lbl_805FF830;
    u8* a = *(u8**)(obj + 0x14);
    u8* b = *(u8**)(a + 0x14);
    if (b == NULL) return lbl_805FF830;
    f32 d = *(f32*)(b + 0x20);
    f32 n = *(f32*)(a + 0x24);
    if (d <= t) return lbl_805FF838;
    return n / d;
}

// 0x8018C014
// Anim frame at time fps: the anim's +0x24 value scaled by fps; lbl_805FF830 when there is no
// anim data.
f32 AnimGetFrame(void* anim, f32 fps) {
    u8* obj = *(u8**)anim;
    if (obj == NULL) return lbl_805FF830;
    u8* data = *(u8**)(obj + 0x14);
    if (*(void**)(data + 0x14) == NULL) return lbl_805FF830;
    f32 rate = *(f32*)(data + 0x24);
    return rate * fps;
}

// 0x8018C044
// Scaled anim value: the object's +0xB8 value times the anim's +0x20 value times x; lbl_805FF830
// when there is no anim data.
f32 fn_8018C044(u8* p, f32 x) {
    u8* obj = *(u8**)p;
    if (obj == NULL) return lbl_805FF830;
    u8* a = *(u8**)(obj + 0x14);
    u8* data = *(u8**)(a + 0x14);
    if (data == NULL) return lbl_805FF830;
    f32 scale = *(f32*)(data + 0x20);
    f32 s = *(f32*)(p + 0xb8) * scale;
    return s * x;
}

// 0x8018C39C
// Address of entry idx (0x30 bytes each) in the table at +4 of the linked object; NULL when there
// is no object or no anim data.
u8* fn_8018C39C(u8* p, s32 idx) {
    u8* obj = *(u8**)p;
    if (obj == NULL) return NULL;
    u8* a = *(u8**)(obj + 0x14);
    if (*(void**)(a + 0x14) == NULL) return NULL;
    return *(u8**)(obj + 0x4) + idx * 0x30;
}

}  // extern "C"
