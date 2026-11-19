// Small functions in 0x8006xxxx-0x8006Cxxx: random float helper, projectile script callbacks,
// entity event/animation helpers and the item-slot release helpers.
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern f32 lbl_805FD4C8;            // 0x805FD4C8 (float scale for the random value)
extern f32 lbl_805FD4D8;            // 0x805FD4D8 (threshold for fn_80067BB0)
extern f32 lbl_805FD4E8;            // 0x805FD4E8 (threshold for fn_80069028)
extern f32 lbl_805FD4EC;            // 0x805FD4EC (float argument for fn_80069090)
extern f32 lbl_805FD568;            // 0x805FD568 (float constant stored into prop fields)
extern u8  lbl_80383A88[];          // 0x80383A88 (vtable)
extern u8  lbl_8035A170[];          // 0x8035A170 (vtable)
extern u8  lbl_8047C760[];          // 0x8047C760 (effect manager, see debris.cpp)

// ScopedLock is a 4-byte RAII guard (see props.cpp / small_800Fc.cpp).
struct ScopedLock { u8 pad[4]; };

// Virtual slot signatures used by the calls below (self pointer first, float last).
typedef void*  (*VFnRetPtr0)(u8* self);
typedef void   (*VFnVoid0)(u8* self);
typedef void   (*VFnDel)(u8* self, s32 flag);
typedef void*  (*VFnSlot50)(u8* self, u8* obj, s32 a, s32 b, s32 c, f32 f);
typedef void   (*VFnSlot50Void)(u8* self, u8* obj, s32 kind, s32 b, s32 c, f32 f);

extern "C" {
u32   RandomU32(void);                                       // 0x80143460
void  ScopedLockCtor(ScopedLock* l);                         // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);              // 0x801F53B0
void  fn_80062F58(u8* p);                                    // 0x80062F58 (small_8006.cpp)
void  fn_80068B50(u8* p, s32 a1);                            // 0x80068B50
void  fn_801B4520(u8* p);                                    // 0x801B4520
void  fn_8012F8B8(u8* p, s32 a1, s32 a2);                    // 0x8012F8B8
s32   AnimSetFindClip(void* set, s32 anim);                  // 0x801AC2FC
void  PropPlayAnim(void* prop, int a, int b, BOOL once);     // 0x80069350
void  fn_8018BEA4(void* p, f32 t);                           // 0x8018BEA4
void  fn_8018C3D0(void* p);                                  // 0x8018C3D0
void  fn_801B5D94(u8* obj, u8* ev);                          // 0x801B5D94
void  fn_8006C3FC(u8* p);                                    // 0x8006C3FC
void  fn_8006C6C0(u8* p);                                    // 0x8006C6C0
void  fn_801C83AC(u8* mgr, u8* item);                        // 0x801C83AC (small_800D.cpp)
void  fn_801C849C(u8* mgr, u8* item);                        // 0x801C849C
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);        // 0x800690D4 (base ctor)
void  fn_80069180(u8* p, s32 a1, s32 a2);                                    // 0x80069180
void  fn_80069638(u8* p, s32 a1);                            // 0x80069638
void  fn_801BB478(u8* p, s32 a1);                            // 0x801BB478
void  fn_800696C4(u8* p, s32 a1);                            // 0x800696C4

// 0x800677D0
// Returns a random float between lo and hi: lo + (hi - lo) * scaled RandomU32().
f32 fn_800677D0(f32 lo, f32 hi) {
    u32 u = RandomU32();
    f32 span = hi - lo;
    f32 scale = lbl_805FD4C8 * (f32)u;
    return lo + span * scale;
}

// 0x80067BB0
// Does nothing unless f1 is above lbl_805FD4D8. Otherwise reads the kind from obj's record
// (+0x228; kind 0xC when there is no record), copies the record value (+8) into the item's
// slot +0x60C or +0x1A8 (by the item's flags) and calls the item's virtual slot 0x50.
void fn_80067BB0(u8* obj, u8* item, s32 a2, s32 a3, f32 t) {
    if (t <= lbl_805FD4D8) return;
    u8* rec = *(u8**)(obj + 0x228);
    s32 kind;
    if (rec == 0) {
        kind = 0xc;
    } else {
        u8* sub = *(u8**)(rec + 0x14c);
        kind = *(s32*)(sub + 0x4c);
    }
    // The original reads rec+8 unconditionally, so rec is assumed non-null here.
    s32 flags = *(s32*)(*(u8**)(item + 4) + 0x1c);
    if (flags & 0x10) {
        *(u32*)(item + 0x60c) = *(u32*)(rec + 8);
    } else if (flags & 0x40) {
        *(u32*)(item + 0x1a8) = *(u32*)(rec + 8);
    }
    (*(VFnSlot50Void*)(*(u8**)item + 0x50))(item, obj, kind, (s32)(s8)a2, a3, t);
}

// 0x80068AE0
// Runs under a scoped lock: a nonzero flag calls fn_80068B50(obj, 1), a zero flag calls fn_80062F58(obj).
void fn_80068AE0(u8* obj, s32 flag) {
    ScopedLock lock;
    ScopedLockCtor(&lock);
    if (flag != 0) fn_80068B50(obj, 1);
    if (flag == 0) fn_80062F58(obj);
    ScopedLockDtor(&lock, -1);
}

// 0x80068DF0
// Calls fn_80068B50(obj, 1), then fn_801B4520(obj).
void fn_80068DF0(u8* obj) {
    fn_80068B50(obj, 1);
    fn_801B4520(obj);
}

// 0x80068F78
// Script callback (type 0x7): calls fn_8012F8B8(obj, 0, 7); if the record at obj+0x22C exists,
// clears its +4 word and sets its vtable to lbl_80383A88. Always returns true.
u32 FixUpProjectileEntityCallback(u32 a0, u32 a1) {
    u8* obj = (u8*)a0;
    fn_8012F8B8(obj, 0, 7);
    u8* rec = *(u8**)(obj + 0x22c);
    if (rec != 0) {
        *(u32*)(rec + 4) = 0;
        *(void**)rec = lbl_80383A88;
    }
    return 1;
}

// 0x80068FD4
// Script callback (type 0x7): calls the object's virtual slot 0x8 with flag -1. Always returns true.
u32 DemolishProjectileEntityCallback(u32 a0, u32 a1) {
    u8* obj = (u8*)a0;
    (*(VFnDel*)(*(u8**)obj + 0x8))(obj, -1);
    return 1;
}

// 0x80069028
// Returns 1 when the float at sub+0x24 (sub = obj+4) is at most lbl_805FD4E8 and the byte at
// sub+0x35 is at least obj+0x1A4 minus one; otherwise 1 only when sub+0x35 is nonzero and
// bit 0x20 of sub+0x20 is set.
s32 fn_80069028(u8* obj) {
    u8* sub = *(u8**)(obj + 4);
    f32 f = *(f32*)(sub + 0x24);
    bool c1 = (f <= lbl_805FD4E8);
    s32 idx = *(s32*)(obj + 0x1a4) - 1;
    s32 b = *(u8*)(sub + 0x35);
    if (b >= idx && c1) return 1;
    if (b == 0) return 0;
    if ((*(u32*)(sub + 0x20) & 0x20) == 0) return 0;
    return 1;
}

// 0x80069090
// Proceeds only when both low bits of sub+0x20 (sub = obj+4) are set. Then clears obj+0x1A8 and
// calls the object's virtual slot 0x50 with (obj, 7, 0, 0, lbl_805FD4EC). Otherwise returns obj.
void* fn_80069090(u8* obj) {
    u8* sub = *(u8**)(obj + 4);
    if (((~*(u32*)(sub + 0x20)) & 3) != 0) return obj;
    *(u32*)(obj + 0x1a8) = 0;
    return (*(VFnSlot50*)(*(u8**)obj + 0x50))(obj, obj, 7, 0, 0, lbl_805FD4EC);
}

// 0x80069538
// If obj+0x9C has a set (+0x48) and the set has a clip for anim, plays that clip on the prop
// (no loop, not once). Otherwise does nothing.
void fn_80069538(u8* obj, s32 anim) {
    u8* c = *(u8**)(obj + 0x9c);
    if (c == 0) return;
    void* set = *(void**)(c + 0x48);
    if (set == 0) return;
    s32 clip = AnimSetFindClip(set, anim);
    if (clip < 0) return;
    PropPlayAnim(obj, clip, 0, 0);
}

// 0x80069598
// Same lookup as fn_80069538 (obj+0x9C -> set +0x48), then plays the clip on the prop with
// both flags set, without the negative check.
void fn_80069598(u8* obj, s32 anim) {
    u8* c = *(u8**)(obj + 0x9c);
    if (c == 0) return;
    void* set = *(void**)(c + 0x48);
    if (set == 0) return;
    s32 clip = AnimSetFindClip(set, anim);
    PropPlayAnim(obj, clip, 1, 1);
}

// 0x800695F0
// Calls the object's virtual slot 0x10; if it returns a non-null object, calls fn_8018BEA4 on it with t.
void fn_800695F0(u8* obj, f32 t) {
    void* r = (*(VFnRetPtr0*)(*(u8**)obj + 0x10))(obj);
    if (r != 0) fn_8018BEA4(r, t);
}

// 0x8006B974
// Calls the virtual slot 0x58 of the held child at +0x160 (if any), then clears +0x160 and sets +0x164 to -1.
void fn_8006B974(u8* obj) {
    u8* h = *(u8**)(obj + 0x160);
    if (h != 0) {
        (*(VFnVoid0*)(*(u8**)h + 0x58))(h);
    }
    *(void**)(obj + 0x160) = 0;
    *(s32*)(obj + 0x164) = -1;
}

// 0x8006BFF8
// Event handler. When the event word's 7-bit field at bits 5..11 is 0x5F and obj+0xA0 is set,
// calls fn_8018C3D0 on that pointer. Then calls fn_801B5D94(obj, ev).
void fn_8006BFF8(u8* obj, u8* ev) {
    u32 w = *(u32*)ev;
    u32 kind = (w >> 20) & 0x7f;
    if (kind == 0x5f) {
        u8* p = *(u8**)(obj + 0xa0);
        if (p != 0) fn_8018C3D0(p);
    }
    fn_801B5D94(obj, ev);
}

// 0x8006C66C
// Sets the byte at +0x220 to 1, stores a1 at +0x224, then calls fn_8006C6C0(obj) and fn_8006C3FC(obj).
void fn_8006C66C(u8* obj, void* a1) {
    *(void**)(obj + 0x224) = a1;
    *(u8*)(obj + 0x220) = 1;
    fn_8006C6C0(obj);
    fn_8006C3FC(obj);
}

// 0x8006CC14
// If obj+0xA8 holds an item, hands it to the manager lbl_8047C760 (fn_801C83AC when a1 != 0,
// otherwise fn_801C849C) and clears +0xA8.
void fn_8006CC14(u8* obj, s32 a1) {
    u8* item = *(u8**)(obj + 0xa8);
    if (item == 0) return;
    if (a1 != 0) {
        fn_801C83AC(lbl_8047C760, item);
    } else {
        fn_801C849C(lbl_8047C760, item);
    }
    *(void**)(obj + 0xa8) = 0;
}

// 0x8006CCC4
// Init: runs the base init fn_800690D4(obj, a1, a2, a3), sets the vtable to lbl_8035A170, and if a
// sub-object is attached at +4, sets flag 0x8000 in its +0x1C and byte 0x0F at its +0x34. Returns obj.
u8* fn_8006CCC4(u8* obj, s32 a1, void* a2, s32 a3) {
    fn_800690D4(obj, a1, a2, a3);
    *(void**)obj = lbl_8035A170;
    u8* sub = *(u8**)(obj + 4);
    if (sub != 0) {
        *(u32*)(sub + 0x1c) |= 0x8000;
        *(u8*)(sub + 0x34) = 0x0f;
    }
    return obj;
}

// 0x8006CD24
// Calls fn_80069180(obj), sets the float at +0x264 to lbl_805FD568, sets bit 0x02000000 in
// sub+0x20 (sub = obj+4) and returns 1.
s32 fn_8006CD24(u8* obj, s32 a1, s32 a2) {
    fn_80069180(obj, a1, a2);
    *(f32*)(obj + 0x264) = lbl_805FD568;
    u8* sub = *(u8**)(obj + 4);
    *(u32*)(sub + 0x20) |= 0x02000000;
    return 1;
}

// 0x8006CE08
// Releases the item at obj+0xA8 through fn_801C849C (manager lbl_8047C760) and clears +0xA8.
void fn_8006CE08(u8* obj) {
    u8* item = *(u8**)(obj + 0xa8);
    if (item != 0) {
        fn_801C849C(lbl_8047C760, item);
        *(void**)(obj + 0xa8) = 0;
    }
}

// 0x8006CE50
// If obj+0xA4 points to a block whose first pointer is set, calls fn_801BB478(that, 1). Then clears
// the float at +0x264 to lbl_805FD568 and the word at +0x268, and calls fn_80069638(obj, a1).
void fn_8006CE50(u8* obj, s32 a1) {
    u8* p = *(u8**)(obj + 0xa4);
    if (p != 0 && *(u8**)p != 0) {
        fn_801BB478(*(u8**)p, 1);
    }
    *(f32*)(obj + 0x264) = lbl_805FD568;
    *(s32*)(obj + 0x268) = 0;
    fn_80069638(obj, a1);
}

// 0x8006CEC0
// Releases the item at obj+0xA8 through fn_801C83AC (manager lbl_8047C760) if set and clears +0xA8,
// then calls fn_800696C4(obj, a1).
void fn_8006CEC0(u8* obj, s32 a1) {
    u8* item = *(u8**)(obj + 0xa8);
    if (item != 0) {
        fn_801C83AC(lbl_8047C760, item);
        *(void**)(obj + 0xa8) = 0;
    }
    fn_800696C4(obj, a1);
}
}
