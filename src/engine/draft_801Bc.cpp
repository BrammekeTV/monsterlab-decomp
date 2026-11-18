// Physics body lookups, contact test and rumble channel handling in 0x801Bxxxx.
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"

void  OSCreateAlarm(u8* alarm);                       // 0x802340C4
void  fn_80234840(u8* alarm, u8* obj);                // 0x80234840
void  fn_80142878(u8* cb);                            // 0x80142878 (register per-frame callback)
void  fn_80142898(u8* cb);                            // 0x80142898 (unregister per-frame callback)
void* fn_801D40E0(s32 n);                             // 0x801D40E0

extern u8 gWorldState[];       // 0x803E6D00
extern u8 lbl_80383B98[];      // 0x80383B98 (rumble table base)
extern u8 lbl_80383BAC[];      // 0x80383BAC (rumble channel vtable)
extern u8 lbl_804133D8[];      // 0x804133D8 (rumble channel slots, 4 entries)

void fn_801B29B0(void);
}

typedef void (*VFn0)(u8*);
typedef void (*VFn1F)(u8*, f32);
typedef void (*VFn1I)(u8*, s32);
typedef s32 (*VFn2)(u8*, u8*);

// Returns the entry of list (array at +0xC, count at +0x10) whose id at +0x8 equals id, or NULL.
static u8* ListFindById(u8* list, u32 id) {
    u32 n = *(u32*)(list + 0x10);
    for (u32 i = 0; i < n; i++) {
        u8* arr = *(u8**)(list + 0xC);
        u8* e = *(u8**)(arr + i * 4);
        if (e != NULL && *(u32*)(e + 0x8) == id) {
            return *(u8**)(arr + i * 4);
        }
    }
    return NULL;
}

// Returns the i-th shape of a body, or NULL when out of range.
static u8* BodyShape(u8* body, u32 i) {
    if (i >= *(u32*)(body + 0x28)) return NULL;
    return *(u8**)(*(u8**)(body + 0x24) + i * 4);
}

// Shared tail of the body getters: first shape of the found entry, only if its +0xC is zero.
static void* FirstShapeIfFree(u8* found) {
    u8* shape;
    if (found == NULL) return NULL;
    if (*(u32*)(found + 0x28) == 0) {
        shape = NULL;
    } else {
        shape = **(u8***)(found + 0x24);
    }
    if (shape == NULL) return NULL;
    if (*(s32*)(shape + 0xC) != 0) return NULL;
    return shape;
}

extern "C" {

// 0x801B1840
// Finds the entry with id 0x7F20C9C6 (hash not resolved) in the list and returns its first shape.
void* PhysicsBodyGet(void* handle) {
    return FirstShapeIfFree(ListFindById((u8*)handle, 0x7F20C9C6u));
}

// 0x801B18E4
// Same as PhysicsBodyGet for the entry with id 0xF62113D5 ("Base").
void* fn_801B18E4(u8* list) {
    return FirstShapeIfFree(ListFindById(list, 0xF62113D5u));
}

// 0x801B1DD4
// Tests body idA (in listA) against body idB (in listB) shape by shape through the shape vtable
// (slot 0x1C). On a hit fills the contact records outA/outB (when the body has flag 2) and returns 1.
s32 fn_801B1DD4(u8* listA, u32 idA, u8* listB, u32 idB, u8* outA, u8* outB, f32 limit) {
    s32 res = 0;
    u8* a = ListFindById(listA, idA);
    if (a == NULL || *(u8*)(a + 0x1C) == 0) return 0;
    u8* b = ListFindById(listB, idB);
    if (b == NULL || a == b || *(u8*)(b + 0x1C) == 0) return 0;

    u32 na = *(u32*)(a + 0x28);
    u32 nb = *(u32*)(b + 0x28);
    for (u32 i = 0; i < na; i++) {
        u8* x = BodyShape(a, i);
        if (x == NULL || *(u8*)(x + 0x38) == 0) continue;
        for (u32 j = 0; j < nb; j++) {
            u8* y = BodyShape(b, j);
            if (y == NULL || *(u8*)(y + 0x38) == 0) continue;
            if ((*(VFn2*)(*(u8**)x + 0x1C))(x, y) == 0) continue;
            if (*(u32*)(a + 0x20) & 2) {
                *(u32*)(outA + 0x5C) = *(u32*)(*(u8**)(b + 0x4) + 0x8);
                *(u8**)(outA + 0x58) = x;
                *(u8**)(outA + 0x60) = *(u8**)(b + 0x4);
                *(u8**)(outA + 0x64) = b;
                *(u8**)(outA + 0x68) = y;
            }
            if (*(u32*)(b + 0x20) & 2) {
                *(u32*)(outB + 0x5C) = *(u32*)(*(u8**)(a + 0x4) + 0x8);
                *(u8**)(outB + 0x58) = y;
                *(u8**)(outB + 0x60) = *(u8**)(a + 0x4);
                *(u8**)(outB + 0x64) = a;
                *(u8**)(outB + 0x68) = x;
            }
            res = 1;
            if (*(f32*)(b + 0x18) > limit) return 1;
            break;
        }
        if (res != 0) return res;
    }
    return res;
}

// 0x801B25C4
// Sets the rumble duration (+0xC) of a channel request; if it was idle (+0x10 set, +0xC == 0) the
// running rumble is stopped first, then restarted with the saved time (or 10.0 when it was negative).
void fn_801B25C4(u8* req, f32 duration) {
    s32 idle = 0;
    if (*(u8*)(req + 0x10) != 0 && *(f32*)(req + 0xC) == 0.0f) {
        idle = 1;
    }
    f32 saved = *(f32*)(req + 0x8);
    if (idle != 0) {
        s32 off = *(s32*)(req + 0x4) * 4;
        u8* ch = *(u8**)(lbl_804133D8 + off);
        if (ch != NULL) {
            (*(VFn0*)(*(u8**)ch + 0x14))(ch);
            *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x10) = 0;
            *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = -1.0f;
            *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x11) = 0;
        }
    }
    *(f32*)(req + 0xC) = duration;
    if (idle != 0) {
        f32 t = (saved >= 0.0f) ? saved : 10.0f;
        s32 off = *(s32*)(req + 0x4) * 4;
        u8* ch = *(u8**)(lbl_804133D8 + off);
        if (ch != NULL) {
            if (*(f32*)(ch + 0x8) < 0.0f) {
                (*(VFn1F*)(*(u8**)ch + 0xC))(ch, *(f32*)(lbl_80383B98 + 0x10));
                *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = t;
                *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x10) = 1;
                u8* c2 = *(u8**)(lbl_804133D8 + off);
                (*(VFn0*)(*(u8**)c2 + 0x10))(c2);
            } else {
                (*(VFn1F*)(*(u8**)ch + 0xC))(ch, *(f32*)(lbl_80383B98 + 0x10));
                *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = t;
            }
        }
    }
}

// 0x801B27F4
// Creates the four rumble channel objects (0x50 bytes each, with an alarm at +0x18) and registers
// fn_801B29B0 as per-frame callback.
void fn_801B27F4(void) {
    for (s32 i = 0; i < 4; i++) {
        u8* p = (u8*)fn_801D40E0(0x50);
        if (p != NULL) {
            *(s32*)(p + 0x4) = i;
            *(f32*)(p + 0x8) = -1.0f;
            *(f32*)(p + 0xC) = 0.0f;
            *(u8*)(p + 0x10) = 0;
            *(u8*)(p + 0x11) = 0;
            *(u8**)(p + 0x0) = lbl_80383BAC;
            *(u8*)(p + 0x48) = 0;
            OSCreateAlarm(p + 0x18);
            fn_80234840(p + 0x18, p);
        }
        *(u8**)(lbl_804133D8 + i * 4) = p;
    }
    fn_80142878((u8*)(uintptr_t)&fn_801B29B0);
}

// 0x801B28C4
// Unregisters the per-frame callback and stops/deletes the four rumble channel objects.
void fn_801B28C4(void) {
    fn_80142898((u8*)(uintptr_t)&fn_801B29B0);
    for (s32 i = 0; i < 4; i++) {
        u8* ch = *(u8**)(lbl_804133D8 + i * 4);
        if (ch == NULL) continue;
        if (*(u8*)(ch + 0x10) != 0) {
            (*(VFn0*)(*(u8**)ch + 0x14))(ch);
            *(u8*)(*(u8**)(lbl_804133D8 + i * 4) + 0x10) = 0;
            *(f32*)(*(u8**)(lbl_804133D8 + i * 4) + 0x8) = -1.0f;
            *(u8*)(*(u8**)(lbl_804133D8 + i * 4) + 0x11) = 0;
        }
        ch = *(u8**)(lbl_804133D8 + i * 4);
        if (ch != NULL) {
            (*(VFn1I*)(*(u8**)ch + 0x8))(ch, 1);   // deleting destructor
        }
        *(u8**)(lbl_804133D8 + i * 4) = NULL;
    }
}

// 0x801B29B0
// Per-frame rumble update: ticks the remaining time down by the frame delta (gWorldState+0xC)
// and stops the channel when it runs out.
void fn_801B29B0(void) {
    for (s32 i = 0; i < 4; i++) {
        u8* ch = *(u8**)(lbl_804133D8 + i * 4);
        if (ch == NULL || *(u8*)(ch + 0x11) != 0) continue;
        (*(VFn0*)(*(u8**)ch + 0x18))(ch);
        ch = *(u8**)(lbl_804133D8 + i * 4);
        f32 t = *(f32*)(ch + 0x8);
        if (t >= 0.0f) {
            *(f32*)(ch + 0x8) = t - *(f32*)(gWorldState + 0xC);
            ch = *(u8**)(lbl_804133D8 + i * 4);
            if (*(f32*)(ch + 0x8) < 0.0f && ch != NULL) {
                (*(VFn0*)(*(u8**)ch + 0x14))(ch);
                *(u8*)(*(u8**)(lbl_804133D8 + i * 4) + 0x10) = 0;
                *(f32*)(*(u8**)(lbl_804133D8 + i * 4) + 0x8) = -1.0f;
                *(u8*)(*(u8**)(lbl_804133D8 + i * 4) + 0x11) = 0;
            }
        }
    }
}

// 0x801B2AC4
// Starts rumble pattern a1 on channel a0 for the default 10 seconds.
void StartRumble(s32 a0, u32 /*0x2A*/ a1) {
    s32 off = a0 * 4;
    u8* ch = *(u8**)(lbl_804133D8 + off);
    if (ch == NULL) return;
    if (*(f32*)(ch + 0x8) < 0.0f) {
        (*(VFn1F*)(*(u8**)ch + 0xC))(ch, *(f32*)(lbl_80383B98 + a1 * 4));
        *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = 10.0f;
        *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x10) = 1;
        u8* c2 = *(u8**)(lbl_804133D8 + off);
        (*(VFn0*)(*(u8**)c2 + 0x10))(c2);
    } else {
        (*(VFn1F*)(*(u8**)ch + 0xC))(ch, *(f32*)(lbl_80383B98 + a1 * 4));
        *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = 10.0f;
    }
}

// 0x801B2C08
// Starts rumble pattern a2 on channel a0 for a1 seconds.
void TimedRumble(s32 a0, f32 a1, u32 /*0x2A*/ a2) {
    s32 off = a0 * 4;
    u8* ch = *(u8**)(lbl_804133D8 + off);
    if (ch == NULL) return;
    if (*(f32*)(ch + 0x8) < 0.0f) {
        (*(VFn1F*)(*(u8**)ch + 0xC))(ch, *(f32*)(lbl_80383B98 + a2 * 4));
        *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = a1;
        *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x10) = 1;
        u8* c2 = *(u8**)(lbl_804133D8 + off);
        (*(VFn0*)(*(u8**)c2 + 0x10))(c2);
    } else {
        (*(VFn1F*)(*(u8**)ch + 0xC))(ch, *(f32*)(lbl_80383B98 + a2 * 4));
        *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = a1;
    }
}

// 0x801B2CE4
// Pauses an active rumble on channel a0 (timed ones keep running state, untimed ones are cleared).
void PauseRumble(s32 a0) {
    s32 off = a0 * 4;
    u8* ch = *(u8**)(lbl_804133D8 + off);
    if (ch == NULL || *(u8*)(ch + 0x10) == 0) return;
    if (*(f32*)(ch + 0x8) < 0.0f) {
        (*(VFn0*)(*(u8**)ch + 0x14))(ch);
        *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x11) = 1;
    } else {
        (*(VFn0*)(*(u8**)ch + 0x14))(ch);
        *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x10) = 0;
        *(f32*)(*(u8**)(lbl_804133D8 + off) + 0x8) = -1.0f;
        *(u8*)(*(u8**)(lbl_804133D8 + off) + 0x11) = 0;
    }
}

}  // extern "C"
