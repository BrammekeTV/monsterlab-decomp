// Misc 0x8018xxxx helpers: texture record loader, screen-set teardown, UI viewport, cursor removal,
// text measurement.
#include <string.h>
#include <stdint.h>
#include "types.h"
#include "engine/rend.h"

extern "C++" u8* lbl_805FB608;       // 0x805FB608: heap-like object; +0 alloc fn, +4 free fn
extern "C++" void* gScreenSets;      // 0x805FB654

typedef u8* (*AllocFn)(s32 size, s32 a, s32 b, const char* name, s32 c);
typedef void (*FreeFn)(void* p, void* heap);
typedef void (*UIColorFn)(void*, s32, f32, f32, f32, f32);   // element vtable (+0x138) +0x2C
typedef void (*UIVisFn)(void*, s32);                         // element vtable (+0x138) +0x30

extern "C" {
void  fn_801499F8(u8* target, s32 w, s32 h, s32 fmt, s32 a, s32 b, const char* name);  // 0x801499F8
void fn_8015A144(u8* arg0, u8** arg1);                                           // 0x8015A144
void  fn_8015DE58(void* video, f32* a, f32* b, f32* c, f32* d);                       // 0x8015DE58
u8* fn_80169770(u8* arg0, const char* name);                                           // 0x80169770
void fn_8016D818(u8* arg0);                                                              // 0x8016D818
void  fn_80172708(void* p);                                                           // 0x80172708
void  fn_80173164(void* p);                                                           // 0x80173164
void  fn_801731C8(void* p);                                                           // 0x801731C8
void  fn_8018D2B8(const char* fmt, ...);                                              // 0x8018D2B8
s32 fn_80198C78(u8* set);                                                         // 0x80198C78
s32   fn_8019BC80(u8* arg0, u32 arg1);                                                // 0x8019BC80
u8*   fn_8019BF64(u8* widget, u32 hash);                                              // 0x8019BF64
u8*   fn_801B3308(void);                                                              // 0x801B3308
void  fn_801B3378(u8* p, u32 key);                                                    // 0x801B3378
extern const char lbl_80381318[];   // 0x80381318 ("UIEngine::ShowCursor : cursor not defined ...")
}

extern "C" {

// 0x801840E4
// Reads a texture record from a data stream (cursor *cur): skips the entry (size + name), returns
// NULL if fn_80169770 already knows the name, else allocates and fills a 0x38+ record.
u8* fn_801840E4(u8* arg0, u8** cur, const char* nameOverride) {
    u8* start = *cur;
    s32 size = *(s32*)start;
    const char* name = (const char*)(start + 4);
    *cur = (u8*)name;
    *cur = *cur + strlen(name) + 1;
    if (nameOverride != NULL) name = nameOverride;
    if (arg0 != NULL && fn_80169770(arg0, name) != 0) {
        *cur = start + size;
        return NULL;
    }
    u8* p = *cur;
    u8 fmt = p[0];
    u8 a = p[1];
    u8 b = p[2];
    u8 flag = p[3];
    u8* q = (u8*)(((uintptr_t)(p + 7)) & ~(uintptr_t)3);
    u16 w = *(u16*)q;
    u16 h = *(u16*)(q + 2);
    *cur = q + 4;
    u8* rec = (*(AllocFn*)lbl_805FB608)((s32)strlen(name) + 0x38, 0x10, 0x14, name, 0);
    strcpy((char*)(rec + 0x35), name);
    *(s32*)(rec + 0x0) = 0x54;
    *(s32*)(rec + 0x24) = 1;
    *(s32*)(rec + 0x28) = 0;
    *(s32*)(rec + 0x2C) = 0;
    *(s32*)(rec + 0x30) = 0;
    *(u8*)(rec + 0x34) = 0;
    fn_801499F8(rec + 4, w, h, fmt, a, b, name);
    *(u8*)(rec + 0x34) = flag;
    fn_8015A144(rec + 4, cur);
    return rec;
}

// 0x80185B4C
// Tears down a screen-set object: releases all nodes of the lists at +4 and +0x10 (decrementing the
// counters at +0x38/+0x3C), the object at +0x1C, then frees the object itself.
void fn_80185B4C(u8* arg0) {
    u8* n;
    while ((n = *(u8**)(arg0 + 0x4)) != NULL) {
        u8* node = n - 0xB8;
        if (*(s32*)(node + 0xC) & 4) {
            fn_80172708(node);
        } else {
            u8* link = *(u8**)(node + 0xC0);
            u8* owner = (link != NULL) ? link - 4 : NULL;
            if (owner == arg0) {
                fn_801731C8(node + 0xB8);
                if (*(s32*)(node + 0xC) & 4) {
                    *(s32*)(arg0 + 0x38) = *(s32*)(arg0 + 0x38) - 1;
                }
            }
        }
    }
    *(s32*)(arg0 + 0x38) = 0;
    while ((n = *(u8**)(arg0 + 0x10)) != NULL) {
        u8* node = n - 4;
        if (*(s16*)(n + 0xE) & 4) {
            fn_80173164(node);
        } else {
            u8* link = *(u8**)(node + 0xC);
            u8* owner = (link != NULL) ? link - 0x10 : NULL;
            if (owner == arg0) {
                fn_801731C8(node + 4);
                if (*(s16*)(node + 0x12) & 4) {
                    *(s32*)(arg0 + 0x3C) = *(s32*)(arg0 + 0x3C) - 1;
                }
            }
        }
    }
    *(s32*)(arg0 + 0x3C) = 0;
    if (*(s32*)(arg0 + 0x1C) != 0) {
        fn_8016D818(arg0);
        *(s32*)(arg0 + 0x1C) = 0;
    }
    *(s32*)(arg0 + 0x0) = 0;
    (*(FreeFn*)(lbl_805FB608 + 4))(arg0, lbl_805FB608);
}

// 0x8018A1FC
// Computes the UI viewport transform (pixel rect and normalized scale/offset) from the video size.
void fn_8018A1FC(void* arg0) {
    u8* p = (u8*)arg0;
    f32 sp14 = -1.0f;
    f32 sp10 = 1.0f;
    f32 spC = 1.0f;
    f32 sp8 = -1.0f;
    fn_8015DE58(&gVideo, &sp14, &sp10, &spC, &sp8);
    s32 w = VideoGetWidth(&gVideo) >> 1;
    s32 h = VideoGetHeight(&gVideo) >> 1;
    *(f32*)(p + 0x8) = 1.0f / (f32)w;
    *(f32*)(p + 0xC) = -1.0f / (f32)h;
    s32 r4 = (s32)(sp10 * (f32)h);
    s32 r5 = -(s32)(-sp14 * (f32)w);
    *(f32*)(p + 0x10) = (f32)r5 / (f32)w;
    *(s32*)(p + 0x0) = (s32)(spC * (f32)w) - r5;
    *(s32*)(p + 0x4) = r4 - -(s32)(-sp8 * (f32)h);
    *(f32*)(p + 0x14) = (f32)r4 / (f32)h;
}

// 0x8018CF1C
// Removes focus entry arg1 from the focus slots at +0x84/+0x88/+0x8C/+0x90 of arg0; if it was the
// focused one (+0x84) the cursor screen of gScreenSets is reset and hidden first. Returns 0.
s32 fn_8018CF1C(u8* arg0, u8* arg1) {
    if (*(u8**)(arg0 + 0x84) == arg1) {
        u8* s = (u8*)gScreenSets;
        if (*(u8**)(s + 0x7C) != NULL) {
            u8* w = fn_8019BF64(*(u8**)(s + 0x7C), 0x0517EA02u);
            if (w != NULL) {
                fn_8019BC80(w, 0);
                *(f32*)(w + 0x80) = 0.0f;
                *(f32*)(w + 0x84) = 0.0f;
                *(f32*)(w + 0x88) = 0.0f;
                *(f32*)(w + 0x8C) = 0.0f;
                (*(UIColorFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 0x20, 0.0f, 0.0f, 0.0f, 0.003921569f);
            }
            w = fn_8019BF64(*(u8**)(s + 0x7C), 0x9C1EBBB8u);
            if (w != NULL) {
                fn_8019BC80(w, 0);
                *(f32*)(w + 0x80) = 0.0f;
                *(f32*)(w + 0x84) = 0.0f;
                *(f32*)(w + 0x88) = 0.0f;
                *(f32*)(w + 0x8C) = 0.0f;
                (*(UIColorFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 0x20, 0.0f, 0.0f, 0.0f, 0.003921569f);
            }
            w = fn_8019BF64(*(u8**)(s + 0x7C), 0x382C6918u);
            if (w != NULL) (*(UIVisFn*)(*(u8**)(w + 0x138) + 0x30))(w, 2);
            if (fn_801B3308() != NULL) fn_801B3378(fn_801B3308(), *(u32*)(s + 0x78));
            u8* cursor = *(u8**)(s + 0x7C);
            if (cursor != NULL) {
                (*(UIVisFn*)(*(u8**)(cursor + 0x138) + 0x30))(cursor, 2);
                if (fn_801B3308() != NULL) fn_801B3378(fn_801B3308(), *(u32*)(s + 0x78));
            } else {
                fn_8018D2B8(lbl_80381318);
            }
            *(u8**)(s + 0x7C) = NULL;
        }
    }
    u8* f88 = *(u8**)(arg0 + 0x88);
    if (f88 == arg1) {
        u8* f8c = *(u8**)(arg0 + 0x8C);
        *(u8**)(arg0 + 0x88) = NULL;
        *(u8**)(arg0 + 0x84) = f8c;
    } else if (*(u8**)(arg0 + 0x8C) == arg1) {
        *(u8**)(arg0 + 0x84) = f88;
        *(u8**)(arg0 + 0x8C) = NULL;
    } else if (*(u8**)(arg0 + 0x90) == arg1) {
        *(u8**)(arg0 + 0x90) = NULL;
        if (*(u8**)(arg0 + 0x84) == arg1) *(u8**)(arg0 + 0x84) = f88;
    }
    fn_80198C78(arg1);
    return 0;
}

}  // extern "C"

// Glyph lookup in the font's hashed table: entries are 0x10 bytes, chained while the low 6 bits match.
static u8* FontFindGlyph(u8* font, u8 c) {
    u8* e = font + 0x8C + font[3] * 4 + *(u16*)(font + 0xC + (c & 0x3F) * 2) * 16;
    for (;;) {
        u32 k = *(u16*)e;
        if (k == 0) return NULL;
        if (k == c) return e;
        if ((k & 0x3F) != (u32)(c & 0x3F)) return NULL;
        e += 0x10;
    }
}

extern "C" {

// 0x8018979C
// Measures a string with the font: *w = width (advance + kerning), *h = max glyph height.
// Unknown glyphs are skipped.
void fn_8018979C(u8* font, const s8* str, s32* w, s32* h) {
    if (*str == 0) {
        *h = 0;
        *w = 0;
        return;
    }
    u8 c = (u8)*str++;
    u8* g = FontFindGlyph(font, c);
    if (g == NULL) return fn_8018979C(font, str, w, h);
    *w = (s8)g[8];
    *h = (s8)g[9];
    while (*str != 0) {
        c = (u8)*str++;
        u8* k = FontFindGlyph(font, c);
        if (k == NULL) continue;
        s32 kern;
        if (!(g[0xF] & 2) || !(k[0xF] & 1)) {
            kern = 0;
        } else {
            u8* t = font + 0x8C + font[3] * 4 + *(u16*)(font + 8) * 16 + *(u16*)(g + 6) * 6;
            u32 gk = *(u16*)g;
            u32 kk = *(u16*)k;
            for (;;) {
                if (*(u16*)t != gk) { kern = 0; break; }
                u32 nk = *(u16*)(t + 2);
                if (nk > kk) { kern = 0; break; }
                if (nk == kk) { kern = *(s16*)(t + 4); break; }
                t += 6;
            }
        }
        s32 nw = *w + kern;
        *w = nw;
        *w = nw + (s8)k[8];
        s32 gh = (s8)k[9];
        if (*h < gh) *h = gh;
        g = k;
    }
}

}  // extern "C"
