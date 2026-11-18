// Small UI/screen-set functions in 0x8019xxxx.
#include "types.h"

// The header declares the script callbacks with C linkage so the definition below matches.
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFn)(void*);

extern "C" {
void*   ScreenSetsCurrent(void* sets);                          // 0x8018D1E4
void*   ScreenSetFindWidget(void* set, u32 hash);               // 0x80198E34
void*   ScreenSetFindElement(void* set, u32 hash);              // 0x80198F18 (tail-calls fn_80198E68)
void    fn_8018D2B8(const char* fmt, ...);                      // 0x8018D2B8 (debug log)
void    fn_8018D370(const char* fmt, ...);                      // 0x8018D370 (debug log)
void    fn_8018D4CC(void* p);                                   // 0x8018D4CC (free)
void*   fn_8018E8D8(void* table, u32 hash);                     // 0x8018E8D8 (table lookup by hash)
void    fn_8018E2F4(void* table, void* cb);                     // 0x8018E2F4 (walk table with callback)
const char* fn_801EB708(u32 hash);                              // 0x801EB708 (hash -> name for logs)
u32     StringHash(const char* s);                              // 0x801EB704
void*   TextureLoad(const char* name);                          // 0x801D8E80
void    fn_8019BD18(void* widget, u32* ev);                     // 0x8019BD18 (routes event to widget)
void    UIWidgetPlayAnim(void* w, u32 anim);                    // 0x80199B38
void    fn_801BA8C8(void* p);                                   // 0x801BA8C8
void*   fn_801BA930(void* a3, void* a4, u32 a5, u32 a6, u32 a7, u32 a8, void* a9); // 0x801BA930
s32     fn_801994E0(void* a, void* obj);                        // 0x801994E0 (object setup callback)
}

extern void* gScreenSets;                       // 0x805FB654
extern f32   lbl_805FF8E8;                      // 0x805FF8E8 (float constant)
extern const char lbl_803818A8[];               // 0x803818A8 (string)
extern const char lbl_803818D0[];               // 0x803818D0 (string)
extern const char lbl_80381AA0[];               // 0x80381AA0 (string)
extern const char lbl_80382428[];               // 0x80382428 (string)
extern const char lbl_80382448[];               // 0x80382448 (string)
extern const char lbl_80382478[];               // 0x80382478 (string)
extern u8    lbl_80382380[];                    // 0x80382380 (vtable)

// 0x801951AC
// Sends an event to the top screen of the current screen set (the last entry of its stack at +0x40, count at +0x60).
void UIEvent_SendToTopScreen(u32 a0) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return;
    }
    s32 count = *(s32*)(set + 0x60);
    if (count > 0) {
        void* top = *(void**)(set + 0x3C + count * 4);
        u32 ev = a0;
        fn_8019BD18(top, &ev);
        UIWidgetPlayAnim(top, ev);
    }
}

// 0x801953E4
// Finds a widget by hash in the current screen set. Logs and returns NULL when missing.
void* fn_801953E4(u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_803818A8);
        return NULL;
    }
    void* w = ScreenSetFindWidget(set, hash);
    if (w != NULL) {
        return w;
    }
    fn_8018D370(lbl_803818D0, fn_801EB708(hash));
    return NULL;
}

// 0x801954E4
// Frees the child at +0x10 (if any), then the object itself. Returns 0.
s32 fn_801954E4(void* p) {
    void* child = *(void**)((u8*)p + 0x10);
    if (child != NULL) {
        fn_8018D4CC(child);
    }
    fn_8018D4CC(p);
    return 0;
}

// 0x80195464
// Finds an element by hash in the current screen set. Logs and returns NULL when missing.
void* UIFindElement(u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        return NULL;
    }
    void* el = ScreenSetFindElement(set, hash);
    if (el != NULL) {
        return el;
    }
    fn_8018D2B8(lbl_80381AA0, fn_801EB708(hash));
    return NULL;
}

// 0x80196CCC
// Loads a texture by name (the hash computed from the name is discarded).
void fn_80196CCC(void* unused, const char* name) {
    (void)unused;
    StringHash(name);
    TextureLoad(name);
}

// 0x80196FCC
// Scales a 4-float vector at vec by (lbl_805FF8E8 / scale) when vec is non-NULL, then tail-calls fn_801BA930
// with obj->+8 as the first argument. Its float registers f1-f3 also carry the scaled components.
void* fn_80196FCC(void* obj, void* a4, f32* vec, u32 a6, u32 a7, u32 a8, u32 a9, f32 scale) {
    if (vec != NULL) {
        f32 k = lbl_805FF8E8 / scale;
        vec[0] *= k;
        vec[1] *= k;
        vec[2] *= k;
        vec[3] *= k;
    }
    return fn_801BA930(*(void**)((u8*)obj + 0x8), a4, a6, a7, a8, a9, vec);
}

// 0x8019702C
// Calls fn_801BA8C8 on obj->+8, then multiplies the floats at a7 and a8 (when non-NULL) by scale.
void fn_8019702C(void* obj, u32 a4, u32 a5, u32 a6, f32* a7, f32* a8, f32 scale) {
    (void)a4;
    (void)a5;
    (void)a6;
    fn_801BA8C8(*(void**)((u8*)obj + 0x8));
    if (a7 != NULL) {
        *a7 *= scale;
    }
    if (a8 != NULL) {
        *a8 *= scale;
    }
}

// 0x801970D4
// Sets the vtable on p, calls its virtual slot 0x14 (init), returns p.
void* fn_801970D4(void* p) {
    *(void**)p = lbl_80382380;
    (*(VFn*)(*(u8**)p + 0x14))(p);
    return p;
}

// 0x80197FE0
// Returns the signed byte at +2 of obj->+8, converted to float.
f32 fn_80197FE0(void* obj) {
    u8* sub = *(u8**)((u8*)obj + 0x8);
    return (f32)(s8)sub[0x2];
}

// 0x801982A8
// Returns 1 when flags+8 has bit 1 set, flags+0xC has any bit of mask, and flags+0x10 has none of mask.
BOOL fn_801982A8(void* obj, u32 mask) {
    u8* o = (u8*)obj;
    if (!(*(u32*)(o + 0x8) & 2)) {
        return 0;
    }
    if (!(*(u32*)(o + 0xC) & mask)) {
        return 0;
    }
    if (*(u32*)(o + 0x10) & mask) {
        return 0;
    }
    return 1;
}

// 0x801982E4
// Same as fn_801982A8 but the exclusion word is at +0x14.
BOOL fn_801982E4(void* obj, u32 mask) {
    u8* o = (u8*)obj;
    if (!(*(u32*)(o + 0x8) & 2)) {
        return 0;
    }
    if (!(*(u32*)(o + 0xC) & mask)) {
        return 0;
    }
    if (*(u32*)(o + 0x14) & mask) {
        return 0;
    }
    return 1;
}

// 0x80198320
// If flags+8 bit 1 is set, returns flags+0xC with the bits of flags+0x10 cleared; otherwise 0.
u32 fn_80198320(void* obj) {
    u8* o = (u8*)obj;
    u32 r = 0;
    if (*(u32*)(o + 0x8) & 2) {
        u32 a = *(u32*)(o + 0xC);
        u32 b = *(u32*)(o + 0x10);
        r = a & (b ^ a);
    }
    return r;
}

// 0x80198C34
// Returns 1 when widget w is in the screen-set stack (entries at +0x40, count at +0x60), else 0.
BOOL fn_80198C34(void* set, void* w) {
    u8* s = (u8*)set;
    s32 count = *(s32*)(s + 0x60);
    s32 j = 0;
    while (j < count) {
        if (*(void**)(s + 0x40 + j * 4) == w) {
            break;
        }
        j++;
    }
    return j < count;
}

// 0x80198DE4
// Returns 1 when the first word of p is one of two known type tags (0x21D18DCB, 0x1B6732BB), else 0.
BOOL fn_80198DE4(void* p) {
    if (p == NULL) {
        return 0;
    }
    u32 v = *(u32*)p;
    if (v == 0x21D18DCB) {
        return 1;
    }
    if (v == 0x1B6732BB) {
        return 1;
    }
    return 0;
}

// 0x80198E34
// Looks up a widget by hash in the table at set+0x28. Returns NULL when missing.
void* ScreenSetFindWidget(void* set, u32 hash) {
    void* table = *(void**)((u8*)set + 0x28);
    return fn_8018E8D8(table, hash);
}

// 0x80198F1C
// Looks up an entry by hash in the table at set+0x20. Logs and returns NULL when missing.
void* fn_80198F1C(void* set, u32 hash) {
    void* table = *(void**)((u8*)set + 0x20);
    void* r = fn_8018E8D8(table, hash);
    if (r == NULL) {
        fn_8018D2B8(lbl_80382428, fn_801EB708(hash));
    }
    return r;
}

// 0x80198F74
// Looks up an entry by hash in the table at set+0x34. Logs and returns NULL when missing.
void* fn_80198F74(void* set, u32 hash) {
    void* table = *(void**)((u8*)set + 0x34);
    void* r = fn_8018E8D8(table, hash);
    if (r == NULL) {
        fn_8018D2B8(lbl_80382448, fn_801EB708(hash));
    }
    return r;
}

// 0x80198FCC
// Looks up an entry by hash in the table at set+0x3C. Logs and returns NULL when missing.
void* fn_80198FCC(void* set, u32 hash) {
    void* table = *(void**)((u8*)set + 0x3C);
    void* r = fn_8018E8D8(table, hash);
    if (r == NULL) {
        fn_8018D2B8(lbl_80382478, fn_801EB708(hash));
    }
    return r;
}

// 0x80199024
// Runs fn_8018E2F4 over the table at obj+0x28 with the object setup callback fn_801994E0.
void* fn_80199024(void* obj) {
    void* table = *(void**)((u8*)obj + 0x28);
    fn_8018E2F4(table, reinterpret_cast<void*>(fn_801994E0));
    return NULL;
}

// 0x80199054
// Calls the slot 0xC virtual of the second vtable (+0x138) on obj. Returns -2 for NULL, else 0.
s32 fn_80199054(void* obj) {
    if (obj == NULL) {
        return -2;
    }
    u8* o = (u8*)obj;
    (*(VFn*)(*(u8**)(o + 0x138) + 0xC))(obj);
    return 0;
}
