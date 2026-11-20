// Small functions in 0x80053BF0-0x8005BFE0: vector and matrix helpers, a 16-entry
// ring buffer on the character, animation clip hand-off helpers and effect-manager
// item hand-offs. Original range 0x80053BF0-0x8005BFE0.
#include "types.h"
#include "engine/math.h"

extern "C" {
// Globals
extern f32 lbl_805FD300;          // float constant (used as the 'zero' threshold)
extern f32 lbl_805FD304;          // float constant (used as the 'one' value in fn_800557B4)
extern u8  lbl_8047C760[];        // effect manager

// Callees (declared where they are defined elsewhere, or first referenced here)
u8*    AnimSetGetClip(void* set, s32 index);                   // 0x801AC164
s32    AnimSetFindClip(void* set, s32 anim);                   // 0x801AC2FC
void   operator_delete(void* p);                               // 0x801D426C
s32 TriggerDoAction(void* action, void* a1, void* a2);      // 0x801E0054 (three arguments in the asm)
u32    fn_801EC880(u8* p, u32 hash);                           // 0x801EC880
void   fn_801ECFA0(u8* p, s32 idx);                            // 0x801ECFA0
void   fn_801C83AC(u8* mgr, u8* item);                         // 0x801C83AC
void   fn_801C849C(u8* mgr, u8* item);                         // 0x801C849C
void* fn_801ACCA0(u8* arg0, s32 arg1);                         // 0x801ACCA0
void   fn_80055E38(u8* obj, u8* other, s32 flag);              // 0x80055E38
s32 fn_801AC51C(u8* p, u8* obj, u8* target);                  // 0x801AC51C
s32 fn_801AC944(u8* clip, s32 a, u8* b, s32 c, u8* obj);    // 0x801AC944
void   fn_801B6AD4(u8* obj);                                   // 0x801B6AD4
}

// 0x80053BF0
// Returns the pointer stored at +4 of entry idx (idx = obj+4, stride 0x30) of the table
// at **obj; 0 when idx is negative or the entry is missing.
extern "C" void* fn_80053BF0(u8* p) {
    s32 idx = *(s32*)(p + 0x4);
    u8* elem = 0;
    if (idx >= 0) {
        u8* table = *(u8**)(*(u8**)p);
        elem = table + idx * 0x30;
    }
    if (elem == 0) return 0;
    return *(void**)(elem + 0x4);
}

// 0x80054484
// out = a + b, componentwise on three floats.
extern "C" void fn_80054484(Vec3* out, const Vec3* a, const Vec3* b) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

// 0x800549FC
// a *= b, componentwise on three floats.
extern "C" void fn_800549FC(Vec3* a, const Vec3* b) {
    a->x = a->x * b->x;
    a->y = a->y * b->y;
    a->z = a->z * b->z;
}

// 0x80054AC0
// Writes the vector (x, y, z, lbl_805FD300) into the 4-float slot at +0x10 of each
// entry of the list at obj+0x14 (count at obj+0xC; NULL entries are skipped).
extern "C" void fn_80054AC0(u8* obj, const Vec3* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    f32 w = lbl_805FD300;
    s32 count = *(s32*)(obj + 0xC);
    for (s32 i = 0; i < count; i++) {
        u8* e = *(u8**)(obj + 0x14 + i * 4);
        if (e != 0) {
            *(f32*)(e + 0x10) = x;
            *(f32*)(e + 0x14) = y;
            *(f32*)(e + 0x18) = z;
            *(f32*)(e + 0x1C) = w;
        }
    }
}

// 0x8005535C
// Copies a 0x30-byte block (12 floats, a 4x3 matrix) from src to dst.
extern "C" void fn_8005535C(u8* dst, const u8* src) {
    for (s32 i = 0; i < 12; i++) {
        ((f32*)dst)[i] = ((const f32*)src)[i];
    }
}

// 0x800571A0
// out = the character's total velocity: the two velocity terms at +0x844/+0x850,
// +0x848/+0x854 and +0x84C/+0x858 summed per axis.
extern "C" void CharacterGetTotalVelocity(Vec3* out, u8* c) {
    out->x = *(f32*)(c + 0x850) + *(f32*)(c + 0x844);
    out->y = *(f32*)(c + 0x854) + *(f32*)(c + 0x848);
    out->z = *(f32*)(c + 0x858) + *(f32*)(c + 0x84C);
}

// 0x800557B4
// Resets a 0x30-byte block to three rows of (lbl_805FD304, lbl_805FD300, lbl_805FD300,
// lbl_805FD300).
extern "C" void fn_800557B4(u8* out) {
    for (s32 row = 0; row < 3; row++) {
        f32* r = (f32*)(out + row * 0x10);
        r[0] = lbl_805FD304;
        r[1] = lbl_805FD300;
        r[2] = lbl_805FD300;
        r[3] = lbl_805FD300;
    }
}

// 0x80055C80
// Clears the query object q (fn_801ACCA0 with mode 0), frees it when del > 0,
// and returns q.
extern "C" u8* RayQueryDestroy(u8* q, s32 del) {
    if (q != 0) {
        fn_801ACCA0(q, 0);
        if (del > 0) operator_delete(q);
    }
    return q;
}

// 0x80056210
// Runs fn_80055E38 on both objects a and b, when their flags at (ptr+4)->+0x20 have bit
// 0x10 set and bit 0x04000000 clear: a first with flag 0, then b with a as the other object.
extern "C" void fn_80056210(u8* a, u8* b) {
    u32 fa = *(u32*)(*(u8**)(a + 0x4) + 0x20);
    if ((fa & 0x10) != 0 && (fa & 0x04000000) == 0) {
        fn_80055E38(a, b, 0);
    }
    u32 fb = *(u32*)(*(u8**)(b + 0x4) + 0x20);
    if ((fb & 0x10) != 0 && (fb & 0x04000000) == 0) {
        fn_80055E38(b, a, 1);
    }
}

// 0x800567A4
// Same as RayQueryDestroy: clears the object with fn_801ACCA0 (mode 0), frees it when
// del > 0, and returns it.
extern "C" u8* fn_800567A4(u8* p, s32 del) {
    if (p != 0) {
        fn_801ACCA0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80056834
// Returns 1 when flag 0x200 at obj+0x1B0 is set and the value at obj+0x1C0 is greater
// than lbl_805FD300 (NaN counts as greater), else 0.
extern "C" BOOL fn_80056834(u8* obj) {
    s32 result = 0;
    u32 flags = *(u32*)(obj + 0x1B0);
    if ((flags & 0x200) != 0) {
        f32 v = *(f32*)(obj + 0x1C0);
        if (!(v <= lbl_805FD300)) result = 1;
    }
    return result;
}

// 0x80058B5C
// Fires the trigger actions of entity b: the action at b->0x9C->0x74 and the one at
// b+0x44, each when its flag bits 1..4 (mask 0x78000000) are set. Passes (b, a) along.
extern "C" void fn_80058B5C(u8* a, u8* b) {
    if (b == 0) return;
    u8* c = *(u8**)(b + 0x9C);
    if (c != 0) {
        if ((*(u32*)(c + 0x74) & 0x78000000) != 0) {
            TriggerDoAction(c + 0x74, b, a);
        }
    }
    if ((*(u32*)(b + 0x44) & 0x78000000) != 0) {
        TriggerDoAction(b + 0x44, b, a);
    }
}

// 0x8005979C
// Looks up hash 0xCC70BCDE in the table at self+0x94; if found, removes the record with
// fn_801ECFA0. When clear is nonzero, zeroes the counter at self+0x9FC.
extern "C" void fn_8005979C(u8* self, s32 clear) {
    u8* table = self + 0x94;
    s32 idx = (s32)fn_801EC880(table, 0xCC70BCDEu);
    if (idx >= 0) {
        fn_801ECFA0(table, idx);
    }
    if (clear != 0) {
        *(s32*)(self + 0x9FC) = 0;
    }
}

// 0x800598E8
// If self+0x15C holds an item, hands it to the effect manager lbl_8047C760 (fn_801C83AC
// when flag != 0, else fn_801C849C) and clears the slot.
extern "C" void fn_800598E8(u8* self, s32 flag) {
    u8* item = *(u8**)(self + 0x15C);
    if (item != 0) {
        if (flag != 0) {
            fn_801C83AC(lbl_8047C760, item);
        } else {
            fn_801C849C(lbl_8047C760, item);
        }
        *(u8**)(self + 0x15C) = 0;
    }
}

// 0x8005A200
// Returns 1 when the float at (obj+4)->+0x24 is greater than lbl_805FD300 and either
// flag 0x20 at (obj+4)->+0x1C is clear and the flag 0x80 at obj+0x838 is set, or flag
// 0x20 at (obj+4)->+0x1C is set. Otherwise 0.
extern "C" BOOL fn_8005A200(u8* obj) {
    s32 result = 0;
    u8* p = *(u8**)(obj + 0x4);
    f32 t = *(f32*)(p + 0x24);
    if (!(t <= lbl_805FD300)) {
        u32 flags = *(u32*)(p + 0x1C);
        if ((flags & 0x20) != 0) {
            result = 1;
        } else if ((*(u32*)(obj + 0x838) & 0x80) != 0) {
            result = 1;
        }
    }
    return result;
}

// 0x8005BE70
// Stops the current animation: when the set at (self+0x9C)+0x48 has a clip for a1,
// calls fn_801AC51C with it; then clears self+0x5E8. a1 is the caller's r4, passed through.
extern "C" void fn_8005BE70(u8* self, s32 a1) {
    u8* c = *(u8**)(self + 0x9C);
    void* set = *(void**)(c + 0x48);
    if (set != 0) {
        u8* clip = AnimSetGetClip(set, a1);
        if (clip != 0) {
            fn_801AC51C(clip, *(u8**)(self + 0xA0), self);
        }
    }
    *(s32*)(self + 0x5E8) = 0;
}

typedef void (*VirtSlot80)(u8* obj, s32 arg);

// 0x8005BEC8
// Finds the clip for a1 in the set at (self+0x9C)+0x48; if found, calls the virtual slot
// 0x80 of self with the clip index. Then clears self+0x5E8. a1 is the caller's r4, passed through.
extern "C" void fn_8005BEC8(u8* self, s32 a1) {
    u8* c = *(u8**)(self + 0x9C);
    if (c != 0) {
        void* set = *(void**)(c + 0x48);
        if (set != 0) {
            s32 idx = AnimSetFindClip(set, a1);
            if (idx >= 0) {
                (*(VirtSlot80*)(*(u8**)self + 0x80))(self, idx);
            }
        }
    }
    *(s32*)(self + 0x5E8) = 0;
}

// 0x8005BF34
// Pushes value into a 16-entry ring at self+0x5A8. The write index is at +0x5EC (wraps at
// 16) and the count at +0x5E8. Does nothing when the ring is full.
extern "C" void fn_8005BF34(u8* self, s32 value) {
    s32 count = *(s32*)(self + 0x5E8);
    if (count >= 0x10) return;
    s32 head = *(s32*)(self + 0x5EC);
    *(s32*)(self + 0x5A8 + head * 4) = value;
    s32 next = head + 1;
    *(s32*)(self + 0x5EC) = next;
    if (next >= 0x10) {
        *(s32*)(self + 0x5EC) = 0;
    }
    *(s32*)(self + 0x5E8) = count + 1;
}

// 0x8005BF7C
// Starts the clip a4 in the set at (self+0x9C)+0x48 with a5 and the object's
// (self+0xA0) value, via fn_801AC944 with mode -1. a4 is passed through to AnimSetGetClip.
extern "C" void fn_8005BF7C(u8* self, s32 a4, s32 a5) {
    u8* c = *(u8**)(self + 0x9C);
    void* set = *(void**)(c + 0x48);
    if (set != 0) {
        u8* clip = AnimSetGetClip(set, a4);
        if (clip != 0) {
            fn_801AC944(clip, a5, *(u8**)(self + 0xA0), -1, self);
        }
    }
}

// 0x8005BFE0
// Calls fn_801B6AD4 on self, then copies the word at +0x5C of the object at self+0x88
// (when present) into self+0x82C.
extern "C" void fn_8005BFE0(u8* self) {
    fn_801B6AD4(self);
    u8* obj = *(u8**)(self + 0x88);
    if (obj != 0) {
        *(u32*)(self + 0x82C) = *(u32*)(obj + 0x5C);
    }
}
