// Small engine functions in 0x801Bxxxx (ray queries, rumble, overlay alpha).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  fn_801B3310(u8* p, u8* q);                                 // 0x801B3310
void  fn_801B1778(void* p, s32 a);                               // 0x801B1778
void  fn_801B737C(void* a, void* b, void* c, s32 d);             // 0x801B737C
void* fn_801DA798(void* p);                                      // 0x801DA798
u8* fn_801E7BC4(u8* p, s32 del);                               // 0x801E7BC4
s32 fn_801E9678(void* list, void* handle);                          // 0x801E9678
void* fn_801E9D18(void* p, u32 hash, s32 size);                  // 0x801E9D18
u8*  fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);      // 0x801EB7A4
void* fn_801EB94C(void* p);                                      // 0x801EB94C
void  fn_801EBA20(void* list, void* item);                                     // 0x801EBA20
void* MemAllocDefault(u32 size);                                 // 0x801D4650
void* ObjAlloc(u32 size);                                        // 0x801D4414
void  operator_delete(void* p);                                  // 0x801D426C
u32   StringHash(const char* s);                                 // 0x801EB704

extern u8  lbl_803839A8[];      // 0x803839A8 (vtable)
extern u8  lbl_80383D68[];      // 0x80383D68 (vtable)
extern u8  lbl_80383B98[];      // 0x80383B98 (string table base)
extern const char lbl_80383E20[]; // 0x80383E20 (string)
extern u8  lbl_804133D8[];      // 0x804133D8 (rumble channel slots, 4 entries)
extern u32 lbl_804133E8[];      // 0x804133E8 (StringHash results, 5 entries)
extern u8  lbl_805FB6B0;        // 0x805FB6B0 (hash-resolved flag)
extern u32 lbl_806010F8;        // 0x806010F8 (cached hash)
extern f32 lbl_805FFAB8;        // 0x805FFAB8
extern f32 lbl_805FFAF0;        // 0x805FFAF0
}

// Rumble channel slot for a controller channel (pointer table of 4 entries).
static u8* RumbleSlot(s32 chan) {
    return *(u8**)(lbl_804133D8 + chan * 4);
}

// 0x801B11DC
// Initializes a ray query: vtable, zeroed results, default hit state.
extern "C" void RayQueryInit(u8* q) {
    *(u8**)(q + 0x0) = lbl_803839A8;
    *(s32*)(q + 0x4) = 0;
    *(s32*)(q + 0x8) = 0;
    *(s32*)(q + 0xc) = 6;
    *(u32*)(q + 0x2c) = 0x6BACB4D8;
    *(s32*)(q + 0x30) = 0;
    *(s32*)(q + 0x34) = -1;
    *(u8*)(q + 0x38) = 1;
    *(f32*)(q + 0x48) = lbl_805FFAB8;
    *(f32*)(q + 0x4c) = lbl_805FFAB8;
    *(f32*)(q + 0x50) = lbl_805FFAB8;
    *(f32*)(q + 0x3c) = lbl_805FFAB8;
    *(f32*)(q + 0x40) = lbl_805FFAB8;
    *(f32*)(q + 0x44) = lbl_805FFAB8;
}

// 0x801B2B94
// Stops the rumble on a channel: calls the slot's virtual stop (+0x14), then clears its flags and intensity.
extern "C" void StopRumble(s32 chan) {
    u8* p = RumbleSlot(chan);
    if (p) {
        (*(void (**)(u8*))(*(u8**)p + 0x14))(p);
        p = RumbleSlot(chan);
        *(u8*)(p + 0x10) = 0;
        p = RumbleSlot(chan);
        *(f32*)(p + 0x8) = lbl_805FFAF0;
        p = RumbleSlot(chan);
        *(u8*)(p + 0x11) = 0;
    }
}

// 0x801B2D98
// Clears the paused flag of a channel's rumble; if it was active, resumes it via the slot's virtual (+0x10).
void UnpauseRumble(s32 chan) {
    u8* p = RumbleSlot(chan);
    if (!p) return;
    *(u8*)(p + 0x11) = 0;
    p = RumbleSlot(chan);
    if (*(u8*)(p + 0x10) == 0) return;
    (*(void (**)(u8*))(*(u8**)p + 0x10))(p);
}

// 0x801B30C4
// Resolves five fixed name strings into StringHash values stored in lbl_804133E8.
void fn_801B30C4(void) {
    lbl_804133E8[0] = StringHash((const char*)(lbl_80383B98 + 0x80));
    lbl_804133E8[1] = StringHash((const char*)(lbl_80383B98 + 0x90));
    lbl_804133E8[2] = StringHash((const char*)(lbl_80383B98 + 0x9c));
    lbl_804133E8[3] = StringHash((const char*)(lbl_80383B98 + 0xac));
    lbl_804133E8[4] = StringHash((const char*)(lbl_80383B98 + 0xc0));
}

// 0x801B3310
// Appends a pointer to the pointer list (count at +0, array at +4, capacity 0x20) unless it is already there.
extern "C" void fn_801B3310(u8* p, u8* q) {
    s32 count = *(s32*)(p + 0x0);
    if (count >= 0x20) return;
    u8* arr = *(u8**)(p + 0x4);
    for (s32 i = 0; i < count; i++) {
        if (*(u8**)(arr + i * 4) == q) return;
    }
    *(u8**)(arr + count * 4) = q;
    *(s32*)(p + 0x0) = count + 1;
}

// 0x801B3378
// Removes the first occurrence of a pointer from the pointer list, shifting later entries down.
extern "C" void fn_801B3378(u8* p, u8* key) {
    s32 count = *(s32*)(p + 0x0);
    u8* arr = *(u8**)(p + 0x4);
    s32 found = 0;
    for (s32 i = 0; i < count; i++) {
        if (*(u8**)(arr + i * 4) == key) found = 1;
        if (found && i < count - 1) *(u8**)(arr + i * 4) = *(u8**)(arr + (i + 1) * 4);
    }
    if (!found) return;
    *(s32*)(p + 0x0) = count - 1;
    *(u8**)(arr + (count - 1) * 4) = NULL;
}

// 0x801B38BC
// Walks the linked list hanging off the object at +0x60 and calls fn_801EBA20 on that object for each node.
extern "C" void fn_801B38BC(u8* p) {
    u8* obj = *(u8**)(p + 0x60);
    u8* list = *(u8**)(obj + 0x14);
    u8* cur = list ? list + 8 : NULL;
    while (cur) {
        u8* next = *(u8**)(cur - 4);
        next = next ? next + 8 : NULL;
        fn_801EBA20(*(u8**)(p + 0x60), 0);  // r4 left over in the original
        cur = next;
    }
}

// 0x801B402C
// Initializes a lab object: clears the flag at +4, allocates a 0x2C-byte buffer into +8, and sets up its
// helper; the flag byte at +0 is cleared.
extern "C" void fn_801B402C(void* obj) {
    u8* p = (u8*)obj;
    *(s32*)(p + 0x4) = 0;
    u8* buf = (u8*)MemAllocDefault(0x2c);
    *(u8**)(p + 0x8) = buf;
    if (buf) fn_801EB7A4(buf, 4, 1, 0x10, 1);
    *(u8*)(p + 0x0) = 0;
}

// 0x801B4090
// Allocates a node through fn_801EB94C; on success stores the value in it and marks the object active.
void fn_801B4090(u8* p, u8* val) {
    u8* node = (u8*)fn_801EB94C(*(u8**)(p + 0x8));
    if (node) {
        *(u8**)node = val;
        *(u8**)(p + 0x4) = node;
        *(u8*)(p + 0x0) = 1;
    }
}

// 0x801B40E4
// Advances the node cursor at +4 along its link (node - 4), releasing the object at +8; clears the active flag at the end.
extern "C" void fn_801B40E4(u8* p) {
    u8* node = *(u8**)(p + 0x4);
    if (!node) {
        *(u8*)(p + 0x0) = 0;
        return;
    }
    u8* next = *(u8**)(node - 0x4);
    next = next ? next + 8 : NULL;
    fn_801EBA20(*(u8**)(p + 0x8), 0);  // r4 left over in the original
    *(u8**)(p + 0x4) = next;
    if (!next) *(u8*)(p + 0x0) = 0;
}

// 0x801B44AC
// Deleting destructor: sets the vtable, calls its shutdown slot (+0x40), tears down the member at +0xFC, frees if del > 0.
extern "C" u8* fn_801B44AC(u8* p, s32 del) {
    if (p) {
        *(u8**)p = lbl_80383D68;
        (*(void (**)(u8*))(lbl_80383D68 + 0x40))(p);
        fn_801E7BC4(p + 0xfc, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B503C
// If the flag bit 0x2 is set on the object at +4, calls the virtual (+0x24) on each child entry whose owner has that flag.
void fn_801B503C(u8* p) {
    u8* a = *(u8**)(p + 0x4);
    if (*(u32*)(a + 0x20) & 0x2) {
        u8* item = *(u8**)(p + 0xb0);
        while (item) {
            u8* obj = *(u8**)(item + 0x4);
            if (obj) {
                u8* sub = *(u8**)(obj + 0x4);
                if (*(u32*)(sub + 0x20) & 0x2) {
                    (*(void (**)(u8*))(*(u8**)obj + 0x24))(obj);
                }
            }
            item = *(u8**)(item + 0x8);
        }
    }
}

// 0x801B50FC
// Calls the virtual (+0xC) on the object and passes its result to fn_801DA798.
extern "C" void* fn_801B50FC(u8* p) {
    void* r = (*(void* (**)(u8*))(*(u8**)p + 0xc))(p);
    return fn_801DA798(r);
}

// 0x801B51E0
// For each child entry with a sub-object whose flag bit 0x4000 is set, calls fn_801B1778 with a zero argument.
void fn_801B51E0(u8* p) {
    u8* item = *(u8**)(p + 0xb0);
    while (item) {
        u8* x = *(u8**)(item + 0x4);
        if (x) {
            u8* obj = *(u8**)(x + 0x8c);
            if (obj) {
                u8* y = *(u8**)(x + 0x4);
                if (*(u32*)(y + 0x20) & 0x4000) fn_801B1778(obj, 0);
            }
        }
        item = *(u8**)(item + 0x8);
    }
}

// 0x801B5248
// Calls the virtual (+0x28) with the given argument on every child entry of the list at +0xB0.
void fn_801B5248(u8* p, u8* arg) {
    u8* item = *(u8**)(p + 0xb0);
    while (item) {
        u8* obj = *(u8**)(item + 0x4);
        if (obj) {
            (*(void (**)(u8*, u8*))(*(u8**)obj + 0x28))(obj, arg);
        }
        item = *(u8**)(item + 0x8);
    }
}

// 0x801B52AC
// Returns the index of the highest set bit of (flags at +0x1C of the object at +4) & 0x1D0, or 0 if none.
s32 fn_801B52AC(u8* p) {
    u8* x = *(u8**)(p + 0x4);
    s32 v = *(s32*)(x + 0x1c) & 0x1d0;
    if (v == 0) return 0;
    s32 r = -1;
    for (; v > 0; v >>= 1) r++;
    return r;
}

// 0x801B52E0
// Allocates an object and initializes it with fn_801B737C from the two given arguments.
void fn_801B52E0(void* a, void* b) {
    void* obj = ObjAlloc(0x2c);
    if (obj) {
        fn_801B737C(obj, b, a, 1);
    }
}

// 0x801B55A8
// Calls the virtual (+0x10) on the object, then follows its first pointer and returns the value at +8 of that node.
u8* fn_801B55A8(u8* p) {
    u8* ret = (*(u8* (**)(u8*))(*(u8**)p + 0x10))(p);
    if (!ret) return NULL;
    u8* q = *(u8**)ret;
    if (!q) return NULL;
    return *(u8**)(q + 0x8);
}

// 0x801B6C48
// Stops the bouncing overlay alpha on an object: resolves the effect name hash once, then reads the
// effect's 16-bit value at +0x7C and passes it to fn_801E9678.
void StopBouncingOverlayAlpha(void* a0) {
    u8* p = (u8*)a0;
    if (!p) return;
    if (!lbl_805FB6B0) {
        lbl_806010F8 = StringHash(lbl_80383E20);
        lbl_805FB6B0 = 1;
    }
    u8* item = (u8*)fn_801E9D18(p + 0xfc, lbl_806010F8, 0xf8);
    if (item) {
        u16 v = *(u16*)(item + 0x7c);
        fn_801E9678(p + 0xfc, &v);
    }
}
