// Missing-function batch 153: UI engine helpers (0x8017xxxx - 0x8019Bxxx).
#include <string.h>
#include "types.h"
#include "revolution.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFnVoid)(void*);
typedef f32 (*VFnFRet)(void*, f32);

extern "C" {
void RendClear(int flags, int a, int b, f32 depth);                         // 0x801534F0
u32 StringHash(const char* s);
void* UIWidgetFindChild(void* widget, u32 hash);
void fn_8014DE68(void);                                                     // 0x8014DE68
void fn_801514F0(void);                                                     // 0x801514F0
void fn_80154294(void);                                                     // 0x80154294
void fn_80154318(void);                                                     // 0x80154318
s32 fn_8015BA30(void* p);                                                   // 0x8015BA30
void fn_8018D370(const char* fmt, ...);                                     // 0x8018D370 (debug log)
void* fn_8018D428(s32 size, const char* file, void* tag, s32 line);         // 0x8018D428 (alloc)
void fn_8018D558(u8* sets, f32 x, f32 y, f32 z);                            // 0x8018D558
void fn_8018DEF4(void* sets);                                               // 0x8018DEF4
s32 fn_8018E208(void* table, u32 hash, void* entry);                        // 0x8018E208
void fn_80196678(void* gx, void* p, f32 t);                                // 0x80196678
s32 fn_80196B10(u8* a3);                                                    // 0x80196B10 (leaf)
void fn_801981FC(void* obj, f32 dt);                                        // 0x801981FC
s32 fn_80198290(u8* a3, s32 a4);                                            // 0x80198290
s32 fn_8019829C(u8* a3, f32 f1);                                            // 0x8019829C
u32 fn_80198320(void* obj);                                                 // 0x80198320
s32 fn_80198438(void* obj, f32 dt);                                         // 0x80198438
s32 fn_8018CA04(void* obj, f32* dt);                                        // 0x8018CA04
void fn_801999D4(void* widget, s32 pass, void* ctx);                        // 0x801999D4
s32 fn_80199094(void* obj);                                                 // 0x80199094
s32 fn_80199100(void* obj);                                                 // 0x80199100
s32 fn_80199540(u8** a, u8** b);                                            // 0x80199540 (sort compare)
void fn_80199C30(void* obj);                                                // 0x80199C30
void fn_8019BFF8(void* obj, void* cb, void* arg);                           // 0x8019BFF8
void* fn_8019B1C8(void* obj, s32 kind, char* name, char* arg);              // 0x8019B1C8
s32 fn_801AB56C(void* h, f32 t);                                            // 0x801AB56C
s32 fn_801AB7D4(void* h);                                                   // 0x801AB7D4
f32 fn_801AB7F0(void* h);                                                   // 0x801AB7F0
s32 fn_801AB7F8(u8* a3, f32 f1);                                            // 0x801AB7F8
void* fn_801ABAF4(void* anim);                                              // 0x801ABAF4
void fn_801ABCEC(void* h, void* owner);                                     // 0x801ABCEC
void fn_801D65B4(void);                                                     // 0x801D65B4
void fn_801D65C0(void);                                                     // 0x801D65C0
char* fn_801D8ADC(char* s);                                                 // 0x801D8ADC
void* fn_80330D80(s32 size);                                                // 0x80330D80
void fn_80336FE0(void* base, s32 n, s32 size, void* cmp);                   // 0x80336FE0 (sort)
extern const char lbl_803812F8[];   // 0x803812F8 ("UIEngine : not initialized ...\n")
extern const char lbl_80382010[];   // 0x80382010 ("UIList : reallocation attempt with streaming ...\n")
extern const char lbl_803828C8[];   // 0x803828C8 (source file name for the allocator)
extern const char lbl_80382970[];   // 0x80382970 (bad entry kind message)
extern const char lbl_80382AF0[];   // 0x80382AF0 (animation not found message)
extern char lbl_805F93F0[];         // 0x805F93F0 (empty string / allocation tag)
}

extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" s32 gScreenWidth;      // 0x805F9388
extern "C++" s32 gScreenHeight;     // 0x805F938C
extern "C++" s32 gViewportIndex;    // 0x805FB4BC
extern "C++" s32 gFrameBudget;      // 0x805FB4C8
extern "C++" u8 sRendDLs[];         // 0x80401108 (display-list records, 0x94 bytes each)
extern u8 lbl_8047C7D8[];           // 0x8047C7D8
extern u8 lbl_805FB50C;             // 0x805FB50C (GX shadow flags byte)
extern s32 lbl_805F95F8;            // 0x805F95F8 (current allocation pool index)
extern void* lbl_805FB658;          // 0x805FB658 (global render object)
extern void* lbl_805FB670;          // 0x805FB670

// Widget type hashes in a screen set's widget stack.
static const u32 kHashLoading = 0x6825C6AEu;      // "Loading"
static const u32 kHashTypeBlockable = 0x1B6732BBu;
static const u32 kHashBlockFlagChild = 0x8DC6BEC1u;

// True when the widget is a blocking one: type "Loading", or 0x1B6732BB whose child 0x8DC6BEC1
// has flag 2 set.
static inline s32 WidgetBlocks(u8* w) {
    if (w == NULL) return 0;
    u32 type = *(u32*)w;
    if (type == kHashLoading) return 1;
    if (type == kHashTypeBlockable) {
        u8* child = (u8*)UIWidgetFindChild(w, kHashBlockFlagChild);
        if (*(u32*)(child + 0x64) & 2) return 1;
    }
    return 0;
}

static inline u8 ClampByte(f32 v) {
    s32 i = (s32)v;
    if (i < 0) return 0;
    if (i > 0xFF) return 0xFF;
    return (u8)i;
}

// Slot i of a list (count +0, capacity +4, element size +8, data +0x10); NULL when out of range.
static inline u8* ListSlot(u8* list, s32 i) {
    if (i < 0 || i >= *(s32*)(list + 0x4)) return NULL;
    return *(u8**)(list + 0x10) + i * *(s32*)(list + 0x8);
}

extern "C" {

// 0x801785A8
// Resets a render-state record: unit colour/scale floats, four 0x30-byte texture-stage entries
// with default wrap/filter/mapping bytes, then the blend and flag fields at +0x118.
void fn_801785A8(u8* p) {
    *(s32*)(p + 0x54) = 0;
    for (s32 i = 0; i < 0x50; i += 4) {
        f32 v = 1.0f;
        if (i >= 0x20 && i < 0x30) v = 0.0f;
        *(f32*)(p + i) = v;
    }
    u8* e = p;
    for (s32 i = 0; i < 4; i++) {
        *(u16*)(e + 0x58) = 0;
        *(u16*)(e + 0x5A) = 0xC00;
        *(s32*)(e + 0x5C) = 0;
        *(u16*)(e + 0x60) = 0;
        *(u8*)(e + 0x62) = 1;
        *(u8*)(e + 0x63) = 1;
        *(u8*)(e + 0x64) = 3;
        *(u8*)(e + 0x65) = 0;
        *(u8*)(e + 0x66) = 0;
        *(u8*)(e + 0x67) = 0;
        *(u8*)(e + 0x68) = 2;
        *(u8*)(e + 0x69) = 2;
        *(u8*)(e + 0x6A) = 2;
        *(u8*)(e + 0x6B) = 1;
        *(u8*)(e + 0x6C) = 7;
        *(u8*)(e + 0x6D) = 0;
        *(u16*)(e + 0x70) = 0;
        *(u16*)(e + 0x72) = 0xC00;
        *(s32*)(e + 0x74) = 0;
        *(u16*)(e + 0x78) = 0;
        *(u8*)(e + 0x7A) = 1;
        *(u8*)(e + 0x7B) = 1;
        *(u8*)(e + 0x7C) = 3;
        *(u8*)(e + 0x7D) = 0;
        *(u8*)(e + 0x7E) = 0;
        *(u8*)(e + 0x7F) = 0;
        *(u8*)(e + 0x80) = 2;
        *(u8*)(e + 0x81) = 2;
        *(u8*)(e + 0x82) = 2;
        *(u8*)(e + 0x83) = 1;
        *(u8*)(e + 0x84) = 7;
        *(u8*)(e + 0x85) = 0;
        e += 0x30;
    }
    *(f32*)(p + 0x118) = 1.0f;
    *(u16*)(p + 0x11C) = 0x8006;
    *(u16*)(p + 0x11E) = 0x302;
    *(u16*)(p + 0x120) = 0x303;
    *(u16*)(p + 0x122) = 0x8006;
    *(u16*)(p + 0x124) = 1;
    *(u16*)(p + 0x126) = 0;
    *(s32*)(p + 0x128) = 0;
    *(s32*)(p + 0x12C) = 0x01010101;
    *(u16*)(p + 0x130) = 0x405;
    *(u16*)(p + 0x132) = 0x203;
    *(u16*)(p + 0x134) = 0;
    *(u8*)(p + 0x136) = 0;
    *(u8*)(p + 0x137) = 1;
    *(u8*)(p + 0x138) = 0;
    *(u8*)(p + 0x139) = 1;
}

// 0x8017ABB8
// Fills the render-state block with defaults, then sets the byte flags at +0xCFC/+0xD10 from the
// mode index (0..3).
void fn_8017ABB8(u8* p, s32 mode) {
    *(s32*)(p + 0x4) = 1;
    *(u8*)(p + 0x14) = 0;
    *(u8*)(p + 0x1C) = 0;
    *(u8*)(p + 0x24) = 1;
    *(u8*)(p + 0x25) = 0xF;
    *(s32*)(p + 0x28) = 1;
    *(s32*)(p + 0x2C) = 0;
    *(s32*)(p + 0x30) = 2;
    *(s32*)(p + 0x34) = 3;
    *(s32*)(p + 0x8) = 1;
    *(s32*)(p + 0x1F4) = 0;
    *(s32*)(p + 0xC) = 2;
    *(u8*)(p + 0x430) = 0;
    *(s32*)(p + 0x434) = 0;
    *(s32*)(p + 0x438) = 0;
    *(s32*)(p + 0x43C) = 0;
    *(s32*)(p + 0x458) = 0xF;
    *(s32*)(p + 0x45C) = 2;
    *(s32*)(p + 0x460) = 0xA;
    *(s32*)(p + 0x464) = 4;
    *(s32*)(p + 0x47C) = 7;
    *(s32*)(p + 0x480) = 1;
    *(s32*)(p + 0x484) = 5;
    *(s32*)(p + 0x488) = 7;
    *(u8*)(p + 0x498) = 1;
    *(s32*)(p + 0x49C) = 0;
    *(s32*)(p + 0x4A0) = 0;
    *(s32*)(p + 0x4A4) = 7;
    *(s32*)(p + 0x4C0) = 0xF;
    *(s32*)(p + 0x4C4) = 8;
    *(s32*)(p + 0x4C8) = 0;
    *(s32*)(p + 0x4CC) = 0xF;
    *(s32*)(p + 0x4E4) = 7;
    *(s32*)(p + 0x4E8) = 4;
    *(s32*)(p + 0x4EC) = 0;
    *(s32*)(p + 0x4F0) = 7;
    switch (mode) {
    case 0:
        *(u8*)(p + 0xCFC) = 0;
        *(u8*)(p + 0xD10) = 0;
        break;
    case 1:
    case 2:
        *(u8*)(p + 0xCFC) = 0;
        break;
    case 3:
        *(u8*)(p + 0xCFC) = 1;
        break;
    }
}

// 0x80187B68
// Pool init: count = n + 1, an entry pool of size nodes (0x14 bytes each) chained as a free list
// (head at +0xC), a zeroed table of count * 0xC bytes at +4, and the last node's link cleared.
void fn_80187B68(u8* p, s32 n, s32 size) {
    *(s32*)(p + 0x0) = n + 1;
    *(void**)(p + 0x8) = fn_80330D80(size * 0x14);
    void* tbl = fn_80330D80(*(s32*)(p + 0x0) * 0xC);
    *(void**)(p + 0x4) = tbl;
    memset(tbl, 0, *(s32*)(p + 0x0) * 0xC);
    *(void**)(p + 0xC) = *(void**)(p + 0x8);
    for (s32 i = 0; i < size - 1; i++) {
        u8* node = *(u8**)(p + 0x8) + i * 0x14;
        *(u8**)node = node + 0x14;
    }
    *(void**)(*(u8**)(p + 0x8) + (size - 1) * 0x14) = NULL;
    *(s32*)(p + 0x10) = 0;
    *(s16*)(p + 0x16) = -1;
    *(s16*)(p + 0x14) = -1;
    *(s16*)(p + 0x1A) = 0;
    *(s16*)(p + 0x18) = 0;
}

// 0x8018CA38
// UI engine per-frame update: polls the pad state, sends the matching UI events to the top
// screen, moves the cursor widget with the pointer, then updates the current screen set.
s32 fn_8018CA38(u8* sets, f32 dt) {
    f32 t = dt;
    if (gScreenSets == NULL) {
        fn_8018D370(lbl_803812F8, 0);  // r4 is never set in the original; zero keeps the variadic call clean
        return -1;
    }
    if (*(s32*)(sets + 0x84) == 0) {
        return 0;
    }
    if (lbl_8047C7D8[0] == 0) {
        fn_801981FC(lbl_805FB670, t);
        u32 pad = fn_80198320(lbl_805FB670);
        if (pad & 0x1000) UIEvent_SendToTopScreen(0x23369815u);
        if (pad & 0x2000) UIEvent_SendToTopScreen(0xF777A7CAu);
        if (pad & 0x10) UIEvent_SendToTopScreen(0xBA3FC9AFu);
        if (pad & 0x20) UIEvent_SendToTopScreen(0x6E7EF670u);
        if (pad & 0x200) UIEvent_SendToTopScreen(0x24A15331u);
        if (pad & 0x80) UIEvent_SendToTopScreen(0xBAC5C692u);
        if (pad & 0x40) UIEvent_SendToTopScreen(0x4E1173C3u);
        if (pad & 0x100) UIEvent_SendToTopScreen(0x40CAFBF1u);
        if (pad & 0x800) UIEvent_SendToTopScreen(0x374BDCEAu);
        if (pad & 0x400) UIEvent_SendToTopScreen(0xFAA0C861u);
        if (pad & 0x4001) UIEvent_SendToTopScreen(0xF0E06CEEu);
        if (pad & 0x20008) UIEvent_SendToTopScreen(0x6E84F94Du);
        if (pad & 0x8002) UIEvent_SendToTopScreen(0x9A504C1Cu);
        if (pad & 0x10004) UIEvent_SendToTopScreen(0x948BC42Eu);
        u8* cursor = *(u8**)(sets + 0x7C);
        if (cursor != NULL) {
            u8* ptr = *(u8**)(sets + 0x78);
            f32 hw = 0.5f * (f32)gScreenWidth;
            f32 x = hw + hw * (*(f32*)(ptr + 0xB0) * *(f32*)(ptr + 0x90));
            f32 hh = 0.5f * (f32)gScreenHeight;
            f32 y = hh + hh * (*(f32*)(ptr + 0xB4) * *(f32*)(ptr + 0x94));
            if (x != *(f32*)(cursor + 0x68) || y != *(f32*)(cursor + 0x6C)) {
                fn_8018D558(sets, x, y, *(f32*)(cursor + 0x70));
            }
            if (*(u8*)(sets + 0x80) != 0) {
                u8* c = *(u8**)(sets + 0x7C);
                f32 r = (*(VFnFRet*)(*(u8**)(c + 0x138) + 0x18))(c, t);
                fn_80196678(lbl_805FB658, *(u8**)(sets + 0x7C) + 0xF0, r);
                fn_8019BFF8(*(void**)(sets + 0x7C), (void*)fn_8018CA04, &t);
                fn_80196B10((u8*)lbl_805FB658);
            }
        }
    }
    return fn_80198438(*(void**)((u8*)gScreenSets + 0x84), t);
}

// 0x80195608
// Inserts a copy of elem at index idx + 1 of the list (count +0, capacity +4, element size +8,
// data +0x10), shifting later elements up. Out-of-range slots resolve to NULL. Returns 0.
s32 fn_80195608(u8* list, void* elem, s32 idx) {
    if (*(s32*)(list + 0x0) + 1 > *(s32*)(list + 0x4)) {
        fn_8018D370(lbl_80382010);
    }
    s32 n = *(s32*)(list + 0x0);
    if (idx == n - 1) {
        memcpy(ListSlot(list, n), elem, *(s32*)(list + 0x8));
        *(s32*)(list + 0x0) = *(s32*)(list + 0x0) + 1;
        return 0;
    }
    s32 size = *(s32*)(list + 0x8);
    u8* dst = ListSlot(list, n);
    idx += 1;
    while (n > idx && n > 0) {
        u8* to = ListSlot(list, n);
        dst = ListSlot(list, n - 1);
        memcpy(to, dst, size);
        n -= 1;
    }
    memcpy(dst, elem, *(s32*)(list + 0x8));
    *(s32*)(list + 0x0) = *(s32*)(list + 0x0) + 1;
    return 0;
}

// 0x801960E4
// Ends a GX pass: flushes the pending z/alpha-compare BP state when flag bit 0 is set, clears
// the framebuffer to white (1.0 per channel, clamped to bytes), then resets the pipeline state.
// Returns 0.
s32 fn_801960E4(void* gx) {
    f32 col[4];
    col[0] = 1.0f;
    col[1] = 1.0f;
    col[2] = 1.0f;
    col[3] = 1.0f;
    if (fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x1107 > gFrameBudget) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + gViewportIndex * 0x94);
    }
    u8 flags = lbl_805FB50C;
    if (flags & 1) {
        lbl_805FB50C = flags & 0xFE;
        if (flags & 2) {
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 1;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 1;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x100E;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0x28380040u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC008F8AFu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC108F2F0u;
        } else {
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x100E;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0x28380000u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC008FFFAu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC108FFD0u;
        }
    }
    u32 r = ClampByte(255.0f * col[0]);
    u32 g = ClampByte(255.0f * col[1]);
    u32 b = ClampByte(255.0f * col[2]);
    u32 a = ClampByte(255.0f * col[3]);
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x100C;
    GXWGFifo._u32 = a | (b << 8) | (r << 24) | (g << 16);
    fn_801514F0();
    lbl_805FB50C &= 0xF7;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0;
    fn_80154294();
    fn_80154318();
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE000FE0u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x410004A0u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00000Fu;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x80000000u;
    return 0;
}

// 0x801985BC
// Draws one screen set: for each widget on the stack (count +0x60, array +0x40) clears the
// depth buffer, collects the draw list (+0xC count, +0x10 array) through fn_801999D4, sorts it,
// and draws every non-hidden entry through its slot 0x1C (with the optional second object at
// +0x130 first), switching the BP mode between 3 and 7 by flag 0x80. Returns 0.
s32 fn_801985BC(u8* set, s32 pass) {
    for (s32 w = 0; w < *(s32*)(set + 0x60); w++) {
        u8* widget = *(u8**)(set + 0x40 + w * 4);
        RendClear(4, 0, 0, 1.0f);
        *(s32*)(set + 0xC) = 0;
        fn_801999D4(widget, pass, set + 0x8);
        if (WidgetBlocks(*(u8**)(set + 0x40 + w * 4)) == 1) {
            fn_801D65B4();
        }
        s32 mode = 3;
        if (*(s32*)(set + 0xC) != 0) {
            fn_80336FE0(*(void**)(set + 0x10), *(s32*)(set + 0xC), 4, (void*)fn_80199540);
            for (s32 i = 0; i < *(s32*)(set + 0xC); i++) {
                u8* obj = *(u8**)(*(u8**)(set + 0x10) + i * 4);
                u32 flags = *(u32*)(obj + 0x64);
                if (!(flags & 0x40)) {
                    s32 newMode = (flags & 0x80) ? 7 : 3;
                    if (newMode != mode) {
                        GXWGFifo._u8 = 0x61;
                        mode = newMode;
                        GXWGFifo._u32 = 0xFE00000Eu;
                        GXWGFifo._u8 = 0x61;
                        GXWGFifo._u32 = (u32)(newMode * 2) | 0x40000000u;
                    }
                    if (*(u8**)(obj + 0x130) != NULL) {
                        GXWGFifo._u8 = 0x61;
                        GXWGFifo._u32 = 0xFE000018u;
                        GXWGFifo._u8 = 0x61;
                        GXWGFifo._u32 = 0x41000000u;
                        u8* sub = *(u8**)(obj + 0x130);
                        (*(VFnVoid*)(*(u8**)(sub + 0x138) + 0x1C))(sub);
                        GXWGFifo._u8 = 0x61;
                        GXWGFifo._u32 = 0xFE000018u;
                        GXWGFifo._u8 = 0x61;
                        GXWGFifo._u32 = 0x41000018u;
                    }
                    (*(VFnVoid*)(*(u8**)(obj + 0x138) + 0x1C))(obj);
                }
            }
        }
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0xFE00000Eu;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0x40000006u;
    }
    return 0;
}

// 0x8019892C
// Pops the top widget of the screen set's stack: runs the UI-engine notification, calls
// fn_801D65C0 when no remaining widget is a blocking one, clears the popped widget's callbacks,
// and hands the new top widget's value at +0x148 to lbl_805FB670. Returns 0.
s32 fn_8019892C(u8* set) {
    if (*(s32*)(set + 0x60) > 0) {
        fn_8018DEF4(gScreenSets);
        s32 top = *(s32*)(set + 0x60) - 1;
        *(s32*)(set + 0x60) = top;
        u8* closing = *(u8**)(set + 0x40 + top * 4);
        s32 blocking = 0;
        for (s32 i = 0; i < *(s32*)(set + 0x60); i++) {
            if (WidgetBlocks(*(u8**)(set + 0x40 + i * 4))) {
                blocking = 1;
                break;
            }
        }
        if (blocking == 0) {
            fn_801D65C0();
        }
        fn_8019BFF8(closing, (void*)fn_80199094, NULL);
        fn_8019BFF8(closing, (void*)fn_80199100, NULL);
        fn_80199C30(closing);
        s32 left = *(s32*)(set + 0x60);
        if (left > 0) {
            u8* next = *(u8**)(set + 0x3C + left * 4);
            fn_80198290((u8*)lbl_805FB670, 2);
            fn_8019829C((u8*)lbl_805FB670, *(f32*)(next + 0x148));
        }
    }
    *(s32*)((u8*)gScreenSets + 0x74) = 0;
    return 0;
}

// 0x8019B088
// Creates a named entry (0xD0 bytes) of the given kind (0..4) in the table at obj + 0x18 + kind*4:
// name at +4, optional second string at +0x68, handle from fn_8019B1C8 at +0xCC. Allocates from
// the pool index stored at obj+0. Returns the entry.
void* fn_8019B088(u8* obj, s32 kind, char* name, char* arg) {
    s32 savedPool = lbl_805F95F8;
    lbl_805F95F8 = *(s32*)obj;
    void* handle = fn_8019B1C8(obj, kind, name, arg);
    u8* e = (u8*)fn_8018D428(0xD0, lbl_803828C8, lbl_805F93F0, 0x19E);
    *(s32*)e = kind;
    if (kind >= 0 && kind < 5) {
        *(void**)(e + 0xCC) = handle;
    } else {
        fn_8018D370(lbl_80382970, kind);
    }
    strncpy((char*)(e + 4), name, 0x63);
    *(u8*)(e + 0x67) = 0;
    if (arg != NULL) {
        strncpy((char*)(e + 0x68), arg, 0x63);
        *(u8*)(e + 0xCB) = 0;
    } else {
        strcpy((char*)(e + 0x68), lbl_805F93F0);
    }
    fn_8018E208(*(void**)(obj + 0x18 + kind * 4), StringHash(fn_801D8ADC(name)), e);
    lbl_805F95F8 = savedPool;
    return e;
}

// 0x8019BA18
// Starts the animation desc on the object (current handle at +0x10, vtable slot 0x10 stops it)
// unless the running one has a higher priority (byte +0x30 greater than prio). Returns 0 when
// blocked, -2 when the animation is missing, else the result of fn_801AB7D4.
s32 fn_8019BA18(u8* obj, void* desc, s8 prio, f32 speed) {
    u8* cur = *(u8**)(obj + 0x10);
    if (cur != NULL) {
        if ((s8)*(u8*)(cur + 0x30) < prio) {
            return 0;
        }
        (*(VFnVoid*)(*(u8**)(obj + 0x138) + 0x10))(obj);
    }
    u8* h = (u8*)fn_801ABAF4(desc);
    if (h == NULL) {
        fn_8018D370(lbl_80382AF0);
        return -2;
    }
    fn_801ABCEC(h, obj);
    *(u8**)(obj + 0x10) = h;
    *(s8*)(h + 0x30) = prio;
    s32 r = fn_801AB7D4(*(u8**)(obj + 0x10));
    if (r == 0) {
        fn_801AB7F8(*(u8**)(obj + 0x10), speed * fn_801AB7F0(*(u8**)(obj + 0x10)));
        fn_801AB56C(*(u8**)(obj + 0x10), 0.0f);
    }
    return r;
}

}  // extern "C"
