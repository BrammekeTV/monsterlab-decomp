// Small engine functions in 0x801Exxxx.
#include <string.h>

#include "types.h"

extern "C" {
BOOL    ScriptDispatchEngine(s32 func, u32 sig, u32* args, u32* result); // 0x801E03AC
s32     fn_801E4B00(void* p);                                            // 0x801E4B00
void    operator_delete(void* p);                                        // 0x801D426C
void    BufferFree(void* p);                                             // 0x801D4980
s32     RegisterScriptFunc(const char* name, s32 type, void* fn);        // 0x801DC91C
void    InterpolateToNextWaypointTask_DefaultReachedWaypointCallback(u8* a3); // 0x801EB310
void    InterpolateToNextWaypointTask_SetVector3Callback(f32* v, u8* state, u8* task); // 0x801EB330
s32     inflateInit_(void* strm, const char* version, s32 size);         // 0x80230854
s32     inflate(void* strm, s32 flush);                                  // 0x802309E0
s32     inflateEnd(void* strm);                                          // 0x80232120
}

extern "C" int SysSprintf(char* buf, const char* fmt, ...);                         // 0x80142EF8

extern u8  lbl_805FB894[];      // 0x805FB894 (fallback handler pointer for ScriptDispatchEngine)
extern u8  lbl_805FB8C8[];      // 0x805FB8C8 (zlib stream-active flag byte)
extern s32 lbl_805FB8E8;        // 0x805FB8E8 (script func id)
extern s32 lbl_805FB8EC;        // 0x805FB8EC (script func id)
extern u32 lbl_805F9690;        // 0x805F9690 (invalid-handle sentinel)
extern u32 lbl_805F969C;        // 0x805F969C (invalid-handle sentinel)
extern const char lbl_805F96A0[]; // 0x805F96A0 (zlib version string)
extern const char lbl_805F96A8[]; // 0x805F96A8 (format string)
extern const char lbl_80385DF0[]; // 0x80385DF0 (script name string)
extern const char lbl_80385E30[]; // 0x80385E30 (script name string)
extern u8  lbl_8048A8B4[];      // 0x8048A8B4 (object checked by fn_801E5E24)
extern char lbl_804DD0E0[];     // 0x804DD0E0 (16-byte format buffer)
extern f32 lbl_80600028;        // 0x80600028
extern f32 lbl_8060002C;        // 0x8060002C
extern f32 lbl_80600030;        // 0x80600030
extern f32 lbl_80600034;        // 0x80600034

extern "C" {

// 0x801E0004
// Adds delta to float slot idx of the 32-entry float table at +4. Indices >= 0x20 are ignored.
void fn_801E0004(u8* p, u32 idx, f32 delta) {
    if (idx < 0x20) {
        f32* slot = (f32*)(p + 4 + idx * 4);
        *slot = delta + *slot;
    }
}

// 0x801E0330
// Dispatches a script call to the engine dispatcher; if that does not handle the signature,
// forwards it to the fallback handler stored in lbl_805FB894 (when one is set).
void fn_801E0330(s32 func, u32 sig, u32* args, u32* result) {
    if (!ScriptDispatchEngine(func, sig, args, result)) {
        void* fallback = *(void**)lbl_805FB894;
        if (fallback) {
            ((void (*)(s32, u32, u32*, u32*))fallback)(func, sig, args, result);
        }
    }
}

// Shared handle lookup: 0xFFFFFFFF is the null handle. Otherwise the low 7 bits pick a slot
// in the table at +4 (stride `stride`) and the rest must match the slot's generation word at `genOff`.
static u8* FindHandleSlot(u8* obj, u32 h, u32 stride, u32 genOff) {
    if (h == 0xFFFFFFFF) return NULL;
    u8* slot = *(u8**)(obj + 4) + (h & 0x7F) * stride;
    if (*(u32*)(slot + genOff) != (h >> 7)) return NULL;
    return slot;
}

// 0x801E4B2C
// Looks up the handle in *handle. Returns FALSE if the slot is in state 2; otherwise marks the slot
// as state 1 (when it exists) and returns TRUE.
BOOL fn_801E4B2C(u8* obj, u32* handle) {
    u8* slot = FindHandleSlot(obj, *handle, 0x28, 0x24);
    if (slot) {
        if (*(s32*)(slot + 4) == 2) return 0;
        *(s32*)(slot + 0x20) = 1;
    }
    return 1;
}

// 0x801E4B98
// Finds the first free 0x164-byte record (flag byte at +0 is zero) in the pool at *p, out of 64.
// Marks it used and returns it, or NULL when the pool is full.
u8* fn_801E4B98(u8* p) {
    u8* base = *(u8**)p;
    for (u32 i = 0; i < 0x40; i++) {
        u8* rec = base + i * 0x164;
        if (*rec == 0) {
            *rec = 1;
            return rec;
        }
    }
    return NULL;
}

// 0x801E51AC
// Returns FALSE for the sentinel handle. Otherwise returns TRUE unless the handle's slot exists
// and its state word at +4 is nonzero.
BOOL fn_801E51AC(u8* obj, u32* handle) {
    if (*handle == lbl_805F9690) return 0;
    u8* slot = FindHandleSlot(obj, *handle, 0x28, 0x24);
    if (!slot) return 1;
    return *(u32*)(slot + 4) == 0;
}

// 0x801E54A4
// Deleting destructor (no base dtor): frees the object when del > 0. Returns the object.
u8* fn_801E54A4(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801E73E4
// Same deleting-destructor shape as fn_801E54A4: frees the object when del > 0. Returns the object.
u8* fn_801E73E4(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x801E5E24
// Returns TRUE when the two counters at +0x10 and +0x14 are both non-negative. Otherwise
// returns the result of fn_801E4B00 on the global object at lbl_8048A8B4.
BOOL fn_801E5E24(u8* p) {
    s32 pending = (*(s32*)(p + 0x10) < 0) || (*(s32*)(p + 0x14) < 0);
    if (!pending) return 1;
    return fn_801E4B00(lbl_8048A8B4) != 0;
}

// 0x801E6BD0
// Same handle lookup as fn_801E51AC, with 0x88-byte slots and the generation word at +0x74.
BOOL fn_801E6BD0(u8* obj, u32* handle) {
    if (*handle == lbl_805F969C) return 0;
    u8* slot = FindHandleSlot(obj, *handle, 0x88, 0x74);
    if (!slot) return 1;
    return *(u32*)(slot + 4) == 0;
}

// 0x801E6EB8
// Inflates the zlib stream at +0xC of p in one call and closes it again.
s32 fn_801E6EB8(u8* p) {
    inflateInit_(p + 0xc, lbl_805F96A0, 0x38);
    inflate(p + 0xc, 0);
    inflateEnd(p + 0xc);
    return 0;
}

// 0x801E6F08
// One step of the streamed inflate on the zlib stream at +0xC. When the stream ends, closes it,
// clears the global stream-active flag and zeroes the word at +0x78.
s32 fn_801E6F08(u8* p) {
    if (*(s32*)(p + 0x14) == 0) {
        inflateInit_(p + 0xc, lbl_805F96A0, 0x38);
    }
    if (inflate(p + 0xc, 0) == 1) {
        inflateEnd(p + 0xc);
        lbl_805FB8C8[0] = 0;
        *(s32*)(p + 0x78) = 0;
    }
    return 0;
}

// 0x801E6F78
// Copies the block of +0x60 bytes from +0xC into the buffer at +0x5C.
s32 fn_801E6F78(u8* p) {
    memcpy(*(void**)(p + 0x5c), *(void**)(p + 0xc), *(u32*)(p + 0x60));
    return 0;
}

// 0x801E7424
// Maps a 16-bit id to a 0x80-byte record in the table at base. The low byte is the index (must be
// below 0xFF) and the high byte must be nonzero. Returns NULL for an invalid id.
u8* fn_801E7424(u8* base, u16* id) {
    u16 h = *id;
    u32 idx = h & 0xFF;
    if (idx < 0xFF && ((h >> 8) & 0xFF) != 0) {
        return base + idx * 0x80;
    }
    return NULL;
}

// 0x801E7460
// Checks that the 16-bit id is valid (see fn_801E7424) and that the record stores the same id at +0x7C.
BOOL fn_801E7460(u8* base, u16* id) {
    u16 h = *id;
    u32 idx = h & 0xFF;
    if (idx < 0xFF && ((h >> 8) & 0xFF) != 0) {
        u8* rec = base + idx * 0x80;
        return *(u16*)(rec + 0x7c) == h;
    }
    return 0;
}

// 0x801EA350
// Returns 1.0 / x when x lies outside (lbl_80600030, lbl_8060002C), or lbl_80600028 inside it.
f32 fn_801EA350(const u8* p) {
    f32 x = *(const f32*)p;
    if (x > lbl_80600030 && x < lbl_8060002C) {
        return lbl_80600028;
    }
    return lbl_80600034 / x;
}

// 0x801EAA08
// Registers the two InterpolateToNextWaypointTask script callbacks and stores their ids.
void fn_801EAA08(void) {
    lbl_805FB8E8 = RegisterScriptFunc(lbl_80385DF0, 0x66,
                                      (void*)InterpolateToNextWaypointTask_DefaultReachedWaypointCallback);
    lbl_805FB8EC = RegisterScriptFunc(lbl_80385E30, 0x90,
                                      (void*)InterpolateToNextWaypointTask_SetVector3Callback);
}

// 0x801EB568
// Fills a 0x28-byte record: three floats from v3 at +0, four from v4 at +0xC, the scalar f at +0x1C,
// then the ints a at +0x20 and b at +0x24.
void fn_801EB568(u8* p, s32 a, const f32* v3, s32 b, const f32* v4, f32 f) {
    *(f32*)(p + 0x0)  = v3[0];
    *(f32*)(p + 0x4)  = v3[1];
    *(f32*)(p + 0x8)  = v3[2];
    *(f32*)(p + 0xc)  = v4[0];
    *(f32*)(p + 0x10) = v4[1];
    *(f32*)(p + 0x14) = v4[2];
    *(f32*)(p + 0x18) = v4[3];
    *(f32*)(p + 0x1c) = f;
    *(s32*)(p + 0x20) = a;
    *(s32*)(p + 0x24) = b;
}

// 0x801EB708
// Formats the hash into a shared 16-byte buffer and returns it (used for log messages).
const char* fn_801EB708(u32 hash) {
    SysSprintf(lbl_804DD0E0, lbl_805F96A8, hash);
    return lbl_804DD0E0;
}

// 0x801EB748
// Frees a linked list of buffers starting at +0x1C, but only when the record at +0x24 is zero and
// the word at +0x18 is zero.
void fn_801EB748(void* p) {
    u8* rec = (u8*)p;
    if (*(s32*)(rec + 0x24) != 0) return;
    if (*(s32*)(rec + 0x18) != 0) return;
    void* cur = *(void**)(rec + 0x1c);
    while (cur) {
        void* next = *(void**)cur;
        BufferFree(cur);
        cur = next;
    }
}

// 0x801EB908
// Copies the header of record src into p (words at +0, +4, +8, byte +0x20, word +0x24), links p to
// src at +0x18 and clears the remaining fields (+0xC, +0x10, +0x14, +0x1C).
void fn_801EB908(u8* p, u8* src) {
    *(u32*)(p + 0x4) = *(u32*)(src + 0x4);
    *(u32*)(p + 0x8) = *(u32*)(src + 0x8);
    *(u32*)(p + 0x0) = *(u32*)(src + 0x0);
    *(u32*)(p + 0x1c) = 0;
    *(void**)(p + 0x18) = (void*)src;
    *(u32*)(p + 0x10) = 0;
    *(u32*)(p + 0x14) = 0;
    *(u32*)(p + 0xc) = 0;
    *(u8*)(p + 0x20) = *(u8*)(src + 0x20);
    *(u32*)(p + 0x24) = *(u32*)(src + 0x24);
}

}  // extern "C"
