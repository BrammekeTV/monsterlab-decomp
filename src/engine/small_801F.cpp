// Small engine functions in 0x801Fxxxx.
#include "types.h"

typedef void (*VFn)(void*);
typedef void (*VFnP)(void*, void*);
#define VT(obj, off) (*(void**)(*(u8**)(obj) + (off)))

extern "C" {
void    operator_delete(void* p);                                    // 0x801D426C
u8*     fn_801B3604(u8* p, s32 del);                                 // 0x801B3604 (deleting dtor)
void    fn_80146F50(u8* p, s32 a);                                   // 0x80146F50 (base dtor)
u8*     fn_80146FD8(u8* p, s32 a);                                   // 0x80146FD8 (base dtor)
u8*     fn_80147150(u8* p, s32 a);                                   // 0x80147150 (base dtor)
u8*     fn_801FA82C(u8* p, s32 del);                                 // 0x801FA82C (deleting dtor)
s32     fn_801F5294(s32 idx);                                        // 0x801F5294
void    fn_801F5E3C(void* a, const void* b, s32 c);                  // 0x801F5E3C (leaf in src/leaf, takes args here)
void*   EventPost(s32 id, void* sender);                             // 0x801F3918
void    BufferFree(void* data);                                      // 0x801D4980
void*   fn_801F4B6C(s32 id);                                        // 0x801F4B6C
void*   fn_801B7198(void* words, s32 count);                        // 0x801B7198
}

extern const char lbl_80385FD8[];   // 0x80385FD8 (string)
extern u8  lbl_804DD168[];          // 0x804DD168 (bss object, 0x18 bytes)
extern u8  lbl_804DD140[];          // 0x804DD140
extern u8  lbl_804FB5A8[];          // 0x804FB5A8
extern u8  gWorldState[];           // 0x803E6D00
extern void* lbl_805FB964;          // 0x805FB964 movie player
extern s32 lbl_805FB950;            // 0x805FB950
extern s32 lbl_805FB954;            // 0x805FB954
extern s32 lbl_805FB958;            // 0x805FB958
extern u32 lbl_805FB9F0;            // 0x805FB9F0
extern s32 lbl_805FB970;            // 0x805FB970 (movie player active flag)
extern s32 lbl_805FB97C;            // 0x805FB97C
extern f32 lbl_805F9820;            // 0x805F9820 (countdown float)
extern f32 lbl_806000C0;            // 0x806000C0 (float constant)

extern "C" {

// 0x801F2BCC
// Starts the setup call with the 0x18-byte object and the string, then clears three counters.
void fn_801F2BCC(void) {
    fn_801F5E3C(lbl_804DD168, lbl_80385FD8, 1);
    lbl_805FB950 = 0;
    lbl_805FB954 = 0;
    lbl_805FB958 = 0;
}

// 0x801F3388
// Deleting destructor with an empty base: frees the object if del > 0.
u8* fn_801F3388(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801F51DC
// Movie player step: vtable slot 0x20 when flag is set, slot 0x28 otherwise. Does nothing if no movie is active.
void fn_801F51DC(s32 flag) {
    if (lbl_805FB970 == 0) return;
    if (flag != 0) ((VFn)VT(lbl_805FB964, 0x20))(lbl_805FB964);
    else           ((VFn)VT(lbl_805FB964, 0x28))(lbl_805FB964);
}

// 0x801F5734
// Forwards arg to movie player vtable slot 0x30; clears lbl_805FB97C when arg is null.
void fn_801F5734(void* arg) {
    if (lbl_805FB964 != 0) ((VFnP)VT(lbl_805FB964, 0x30))(lbl_805FB964, arg);
    if (arg == 0) lbl_805FB97C = 0;
}

// 0x801F57E0
// Countdown: returns 1 while the timer is below the constant. Otherwise subtracts dt and
// returns 1 if the timer is still above the constant, 0 once it has reached it.
s32 fn_801F57E0(f32 dt) {
    if (!(lbl_805F9820 >= lbl_806000C0)) return 1;
    lbl_805F9820 = lbl_805F9820 - dt;
    return !(lbl_805F9820 <= lbl_806000C0);
}

// 0x801F59B0
// Deleting destructor with an empty base: frees the object if del > 0.
u8* fn_801F59B0(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801F5B44
// Computes a 0/1 flag from fn_801F5294(x) plus the word at +0x10 (sign bit of a bit-trick difference).
s32 fn_801F5B44(u8* p, s32 x) {
    u32 a = (u32)fn_801F5294(x) + (u32)*(s32*)(p + 0x10);
    u32 t = (0x400u - a) >> 1;
    return (s32)(((0x400u | ~a) - t) >> 31);
}

// 0x801F5D34
// Deleting destructor: base dtor fn_80146F50 with 0, then frees if del > 0.
u8* fn_801F5D34(u8* p, s32 del) {
    if (p) {
        fn_80146F50(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F5D8C
// Deleting destructor: base dtor fn_80146FD8 with 0, then frees if del > 0.
u8* fn_801F5D8C(u8* p, s32 del) {
    if (p) {
        fn_80146FD8(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F5DE4
// Deleting destructor: base dtor fn_80147150 with 0, then frees if del > 0.
u8* fn_801F5DE4(u8* p, s32 del) {
    if (p) {
        fn_80147150(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F5E50
// Copies the position from the pointer chain at gWorldState into out, adding dz to the third component.
void fn_801F5E50(u8* out, f32 dz) {
    u8* a = *(u8**)gWorldState;
    u8* b = *(u8**)(a + 0x8);
    u8* c = *(u8**)(b + 0x4);
    *(f32*)(out + 0x0) = *(f32*)(c + 0x0);
    *(f32*)(out + 0x4) = *(f32*)(c + 0x4);
    *(f32*)(out + 0x8) = dz + *(f32*)(c + 0x8);
}

// 0x801F6238
// Deleting destructor: base dtor fn_801B3604 with 0, then frees if del > 0.
u8* fn_801F6238(u8* p, s32 del) {
    if (p) {
        fn_801B3604(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F71E8
// Deleting destructor: base dtor fn_801B3604 with 0, then frees if del > 0.
u8* fn_801F71E8(u8* p, s32 del) {
    if (p) {
        fn_801B3604(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F69B8
// Deleting destructor: base dtor fn_801F71E8 with 0, then frees if del > 0.
u8* fn_801F69B8(u8* p, s32 del) {
    if (p) {
        fn_801F71E8(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F858C
// Deleting destructor: base dtor fn_801FA82C with 0, then frees if del > 0.
u8* fn_801F858C(u8* p, s32 del) {
    if (p) {
        fn_801FA82C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F81E0
// Deleting destructor: base dtor fn_801F858C with 0, then frees if del > 0.
u8* fn_801F81E0(u8* p, s32 del) {
    if (p) {
        fn_801F858C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F85E4
// Stores the count at +0x1E0, then copies count 3-float records from src into the table at +0xA8 (stride 0xC).
// The loop re-reads the count each pass, as the original does (the table can overlap the count).
void fn_801F85E4(u8* p, const u8* src, s32 count) {
    *(s32*)(p + 0x1e0) = count;
    u8* dst = p;
    s32 i = 0;
    while (i < *(s32*)(p + 0x1e0)) {
        f32 x = *(const f32*)(src + 0x0);
        f32 y = *(const f32*)(src + 0x4);
        i++;
        f32 z = *(const f32*)(src + 0x8);
        *(f32*)(dst + 0xa8) = x;
        *(f32*)(dst + 0xac) = y;
        *(f32*)(dst + 0xb0) = z;
        src += 0xc;
        dst += 0xc;
    }
}

// 0x801F900C
// Copies count floats (count at +0x1E0) from src into the float table at +0x2B4.
void fn_801F900C(u8* p, const u8* src) {
    u8* dst = p + 0x2b4;
    s32 i = 0;
    while (i < *(s32*)(p + 0x1e0)) {
        f32 v = *(const f32*)src;
        src += 4;
        i++;
        *(f32*)dst = v;
        dst += 4;
    }
}

// 0x801F94EC
// Deleting destructor: base dtor fn_801F858C with 0, then frees if del > 0.
u8* fn_801F94EC(u8* p, s32 del) {
    if (p) {
        fn_801F858C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// Helpers and globals used by the functions below.
struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)
struct ScopedLock { u8 pad[4]; };                       // as in props.cpp

extern ScriptFunc gScriptFuncs[];                       // 0x80484230
extern s32 lbl_805F9800;                                // script function index for the fallback call
extern s32 lbl_80386064[];                              // 0x80386064 table (read at idx - 0x5F)
extern u8  lbl_80386338[];                              // 0x80386338 (vtable)
extern u8  lbl_80386390[];                              // 0x80386390 (vtable)
extern u8  lbl_803863D8[];                              // 0x803863D8 (vtable)
extern u8  lbl_80386478[];                              // 0x80386478 (vtable)
extern u8  lbl_80386490[];                              // 0x80386490 (vtable)
extern u8  lbl_804FB5E0[];                              // 0x804FB5E0 (object passed to fn_801F5E4C / fn_801F5E48)
extern const char lbl_805F9714[];                       // 0x805F9714 (scan format for fn_8033861C)
extern u32 lbl_805FB9A4;                                // 0x805FB9A4 (lock flag, see world_update.cpp)
extern s32 lbl_805FB9D8;                                // 0x805FB9D8
extern void* lbl_805FB968;                              // 0x805FB968 (current target pointer)
extern f32 lbl_80600080;                                // 0x80600080 (default float)
extern f32 lbl_80600220;                                // 0x80600220
extern f32 lbl_80600224;                                // 0x80600224
extern f32 lbl_80600228;                                // 0x80600228
extern f32 lbl_80600248;                                // 0x80600248 (1.0 numerator in fn_801F9CD0)
extern f32 lbl_8060024C;                                // 0x8060024C
extern f32 lbl_80600250;                                // 0x80600250
extern f32 lbl_80600278;                                // 0x80600278

// Callees not defined in this file.
s32     fn_8033861C(const char* s, const char* fmt, ...);            // 0x8033861C (sscanf)
void    fn_801F5E4C(void* a, s32 b);                                 // 0x801F5E4C (leaf in src/leaf, takes args here)
void    fn_801F5E48(void* a, s32 b);                                 // 0x801F5E48 (leaf in src/leaf, takes args here)
void    fn_801F8238(u8* p, s32 a4);                                  // 0x801F8238
void    fn_801F95F0(u8* p, s32 a4, f32 dt);                          // 0x801F95F0
u8*     fn_801B365C(u8* p, s32 a4);                                  // 0x801B365C (small_801B.cpp)
u8*     fn_801B36A8(u8* p, s32 del);                                 // 0x801B36A8 (small_801B.cpp)
u8*     fn_801FA7CC(u8* p, s32 a4);                                  // 0x801FA7CC (defined below)
u8*     fn_801F9544(u8* p, s32 a4);                                  // 0x801F9544 (defined below)
u8*     fn_801FA82C(u8* p, s32 del);                                 // 0x801FA82C (defined below)

// 0x801F005C
// Reads a float from the text at s (skipping leading spaces and '=') into the record at +0x8.
// Keeps the default value when the scan fails.
void fn_801F005C(u8* p, const char* s) {
    while (*s == ' ' || *s == '=') s++;
    f32 v = lbl_80600080;
    fn_8033861C(s, lbl_805F9714, &v);
    *(f32*)(p + 0x8) = v;
}

// 0x801F5294
// Resolves a 7-bit id: 0x5F-0x7F read the static table at lbl_80386064; anything else calls the
// registered script function at index lbl_805F9800 with idx.
s32 fn_801F5294(s32 idx) {
    u32 u = (u32)idx;
    if (u >= 0x5f && u <= 0x7f) return lbl_80386064[u - 0x5f];
    ScriptFunc* f = &gScriptFuncs[lbl_805F9800];
    return ((s32 (*)(s32))f->fn)(idx);
}

// 0x801F5308
// Reads the 7-bit id in bits 5-11 of the word at p and resolves it with fn_801F5294.
s32 fn_801F5308(u8* p) {
    u32 w = *(u32*)p;
    return fn_801F5294((s32)((w >> 20) & 0x7f));
}

// 0x801F53B0
// Deleting destructor for a scoped lock: clears the lock flag when the lock's first word is 0,
// then frees the object if flags > 0.
void ScopedLockDtor(ScopedLock* l, s32 flags) {
    if (l) {
        if (*(s32*)l == 0) lbl_805FB9A4 = 0;
        if (flags > 0) operator_delete(l);
    }
}

// 0x801F5414
// While the movie player flag lbl_805FB970 is set, records p as the current target and runs the
// lbl_804FB5E0 setup calls when no slot is set (lbl_805FB9D8 is re-read after the first call).
void fn_801F5414(u8* p) {
    if (lbl_805FB970 == 0) return;
    if (lbl_805FB9D8 == 0) fn_801F5E4C(lbl_804FB5E0, 6);
    lbl_805FB968 = p;
    if (lbl_805FB9D8 == 0) fn_801F5E48(lbl_804FB5E0, 6);
}

// 0x801F5A14
// Returns the end of the entry at q (q + fn_801F5308(q)) when it is a valid entry, else 0.
// An id of 0x6F ends the list; an entry whose end lies at or past p + the limit at +0x10 is rejected.
u8* fn_801F5A14(u8* p, u8* q) {
    if (q == 0) return 0;
    s32 size = fn_801F5308(q);
    u32 w = *(u32*)q;
    u8* next = q + size;
    if (((w >> 20) & 0x7f) == 0x6f) return 0;
    if ((u32)(next - p) < *(u32*)(p + 0x10)) return next;
    return 0;
}

// 0x801F2EB8
// Marks the event queue for processing when it has entries and its saved head differs from the
// current head word.
void fn_801F2EB8(void) {
    if (lbl_805FB950 != 0 && lbl_805FB9F0 != *(u32*)lbl_804DD140) {
        lbl_805FB954 = 1;
    }
}

// 0x801F4DFC
// Dispatches an entry through its referenced world object when it has one; entries without an
// object are sent to the fallback handler as a one-word command.
void fn_801F4DFC(u8* entry) {
    u32 kind = (*(u32*)entry >> 20) & 0x7F;
    s32 id = -1;
    if (kind == 0x75) id = *(s32*)(entry + 0x20);
    if (kind == 0x73) id = *(s32*)(entry + 0x04);
    if (kind == 0x72) id = *(s32*)(entry + 0x04);
    if (kind == 0x71) id = *(s32*)(entry + 0x04);
    if (kind == 0x63) id = *(s32*)(entry + 0x08);

    if (id != -1) {
        fn_801F4B6C(id);
    } else {
        u32 word = *(u32*)entry;
        fn_801B7198(&word, 1);
    }
}

// 0x801F5484
// Posts the pending stream-complete event, then releases its buffered payload. Returns false
// only when an active request could not be posted.
s32 fn_801F5484(void) {
    s32 result = 1;
    if (*(s32*)(lbl_804FB5A8 + 0x00) != 0 && *(s32*)(lbl_804FB5A8 + 0x04) != 0) {
        u8* event = (u8*)EventPost(0x6D, NULL);
        if (event != NULL) {
            *(s32*)(event + 0x04) = 1;
            u8* buffer = *(u8**)(lbl_804FB5A8 + 0x0C);
            BufferFree(*(void**)(buffer + 0x08));
            BufferFree(buffer);
            *(void**)(lbl_804FB5A8 + 0x0C) = NULL;
            *(s32*)(lbl_804FB5A8 + 0x00) = 0;
        } else {
            result = 0;
        }
    }
    return result;
}

// 0x801F57C4
// Tests whether the countdown has reached the threshold.
s32 fn_801F57C4(void) {
    return lbl_805F9820 >= lbl_806000C0;
}

// 0x801F5A90
// Walks the command records after the 0x18-byte header until the end marker or buffer limit.
s32 fn_801F5A90(u8* p) {
    u8* q = p + 0x18;
    if ((u32)(q - p) >= *(u32*)(p + 0x10)) q = NULL;

    while (q != NULL) {
        u32 word = *(u32*)q;
        q += fn_801F5308(q);
        if (((word >> 20) & 0x7F) == 0x6F || (u32)(q - p) >= *(u32*)(p + 0x10)) {
            q = NULL;
        }
    }
    return 1;
}

// 0x801F5B94
// Returns the unused portion of the 0x400-byte command buffer.
u32 fn_801F5B94(u8* p) {
    return 0x400 - *(u32*)(p + 0x10);
}

// 0x801F5BA0
// Reserves the remaining command-buffer space and returns its first byte.
u8* fn_801F5BA0(u8* p) {
    u32 used = *(u32*)(p + 0x10);
    *(u32*)(p + 0x10) = 0x400;
    return p + used;
}

// 0x801F5BB8
// Appends an encoded command when the resolved command size fits in the 0x400-byte buffer.
u8* fn_801F5BB8(u8* p, s32 id, s32 a, s32 b, s32 c, s32 d) {
    u32 size = (u32)fn_801F5294(id);
    u32 used = *(u32*)(p + 0x10);
    if (used + size > 0x400) return NULL;

    u32* command = (u32*)(p + used);
    u32 word = *command;
    word = (word & 0xFFF7FFFF) | ((u32)a << 19);
    word = (word & 0xF80FFFFF) | ((u32)id << 20);
    word = (word & 0xFFFBFFFF) | ((u32)b << 18);
    if ((u32)c >= 3) {
        word = (word & 0xFFFCFFFF) | 0x00030000;
        word = (word & 0xFFFF007F) | ((u32)c << 7);
        word = (word & 0xFFFFFF83) | ((u32)d << 2);
    } else {
        word = (word & 0xFFFCFFFF) | ((u32)c << 16);
        word = (word & 0xFFFF0003) | ((u32)d << 2);
    }
    *command = word;
    *(u32*)(p + 0x10) = used + size;
    return (u8*)command;
}

// 0x801F8F34
// Sets the 26 floats from +0x1E4 to +0x248 to v.
void fn_801F8F34(u8* p, f32 v) {
    for (s32 off = 0x1e4; off <= 0x248; off += 4) *(f32*)(p + off) = v;
}

// 0x801F8FA0
// Sets the 26 floats from +0x24C to +0x2B0 to v.
void fn_801F8FA0(u8* p, f32 v) {
    for (s32 off = 0x24c; off <= 0x2b0; off += 4) *(f32*)(p + off) = v;
}

// 0x801F94B0
// Base init: runs fn_801F8238 on p (the second argument passes through), then sets the vtable.
void fn_801F94B0(u8* p, s32 a4) {
    fn_801F8238(p, a4);
    *(void**)p = lbl_80386338;
}

// 0x801F9544
// Base init: runs fn_801FA7CC, then sets the vtable and three floats at +0xA8..+0xB0.
u8* fn_801F9544(u8* p, s32 a4) {
    fn_801FA7CC(p, a4);
    *(void**)p = lbl_80386390;
    *(f32*)(p + 0xa8) = lbl_80600220;
    *(f32*)(p + 0xac) = lbl_80600224;
    *(f32*)(p + 0xb0) = lbl_80600228;
    return p;
}

// 0x801F9598
// Deleting destructor: base dtor fn_801FA82C with 0, then frees if del > 0.
u8* fn_801F9598(u8* p, s32 del) {
    if (p) {
        fn_801FA82C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F9C2C
// Init: runs fn_801F9544, then sets the vtable and two floats at +0xB4 and +0xB8.
u8* fn_801F9C2C(u8* p, s32 a4) {
    fn_801F9544(p, a4);
    *(void**)p = lbl_803863D8;
    *(f32*)(p + 0xb4) = lbl_80600248;
    *(f32*)(p + 0xb8) = lbl_8060024C;
    return p;
}

// 0x801F9C78
// Deleting destructor: base dtor fn_801F9598 with 0, then frees if del > 0.
u8* fn_801F9C78(u8* p, s32 del) {
    if (p) {
        fn_801F9598(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801F9CD0
// Advances the timer at +0xB8 by dt, then calls fn_801F95F0. The timer is held at lbl_80600250
// while the flag at +0x90 is set. The flag is set once the timer drops below 1 / (+0xB4).
void fn_801F9CD0(u8* p, s32 a4, f32 dt) {
    *(f32*)(p + 0xb8) = *(f32*)(p + 0xb8) + dt;
    fn_801F95F0(p, a4, dt);
    if (*(u8*)(p + 0x90) != 0) *(f32*)(p + 0xb8) = lbl_80600250;
    if (*(f32*)(p + 0xb8) < lbl_80600248 / *(f32*)(p + 0xb4)) *(u8*)(p + 0x90) = 1;
}

// 0x801FA048
// Deleting destructor: base dtor fn_801FA82C with 0, then frees if del > 0.
u8* fn_801FA048(u8* p, s32 del) {
    if (p) {
        fn_801FA82C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801FA7CC
// Base init: runs fn_801B365C, sets the vtable, clears the int at +0x98 and three floats at +0x9C..+0xA4.
u8* fn_801FA7CC(u8* p, s32 a4) {
    fn_801B365C(p, a4);
    *(void**)p = lbl_80386478;
    *(s32*)(p + 0x98) = 0;
    *(f32*)(p + 0x9c) = lbl_80600278;
    *(f32*)(p + 0xa0) = lbl_80600278;
    *(f32*)(p + 0xa4) = lbl_80600278;
    return p;
}

// 0x801FA82C
// Deleting destructor: base dtor fn_801B36A8 with 0, then frees if del > 0.
u8* fn_801FA82C(u8* p, s32 del) {
    if (p) {
        fn_801B36A8(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801FA884
// Init: runs fn_801FA7CC, clears the int at +0xA8 and sets the vtable.
u8* fn_801FA884(u8* p, s32 a4) {
    fn_801FA7CC(p, a4);
    *(s32*)(p + 0xa8) = 0;
    *(void**)p = lbl_80386490;
    return p;
}

// 0x801FA8C8
// Deleting destructor: base dtor fn_801FA82C with 0, then frees if del > 0.
u8* fn_801FA8C8(u8* p, s32 del) {
    if (p) {
        fn_801FA82C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

}  // extern "C"
