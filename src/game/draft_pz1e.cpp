// Assorted mid-sized functions: UI hash tree, move task init, list helpers, effect pool, UI text, ...
#include "types.h"

struct StringTable;
extern "C++" void*        gScreenSets;     // 0x805FB654
extern "C++" StringTable* gStringTable;    // 0x805FB910

extern "C" {
u32   StringHash(const char* s);                              // 0x801EB704
void* MemAllocDefault(u32 size);                              // 0x801D4650
void* ObjAlloc(u32 size);                                     // 0x801D4414
BOOL  ZoneIsLoaded(void* world, s32 zone);                    // 0x8012F408
void  UIPlayAnim(void* el, u32 anim);                         // 0x8019BE40
void  UISetText(void* el, const char* text, s32 a);           // 0x8019EFC8
void  fn_80144C60(f32* m);                                    // 0x80144C60
void  fn_8016E304(void* cam);                                 // 0x8016E304
u8*   fn_801725C8(u8* p);                                     // 0x801725C8
u32 fn_80184278(u8* p);                                     // 0x80184278 (returns a size, see report)
void  fn_8018D370(const char* fmt, ...);                      // 0x8018D370 (debug log)
void  fn_80199738(u8* screen);                                // 0x80199738
s32   fn_801B9F14(u8* sender, u32 id);                        // 0x801B9F14
s32 fn_801B8184(u8* p, u8* a4);                                      // 0x801B8184 (stored as callback)
void  fn_801C5444(u8* node, f32 dt);                          // 0x801C5444
void  fn_801C7E9C(u8* item, u8* mgr);                         // 0x801C7E9C
void  fn_801C9B60(u8* p, f32 dt);                             // 0x801C9B60
void  fn_801CD790(u8* p, f32 dt);                             // 0x801CD790
void  fn_801D6A4C(void* cam, const f32* mtx);                 // 0x801D6A4C
void  fn_801DA34C(void);                                      // 0x801DA34C (callback stored below)
BOOL  fn_801E7460(u8* base, u16* id);                         // 0x801E7460
void  fn_801EA3C8(u8* entry, f32 v);                          // 0x801EA3C8
void  fn_801EBA20(void* list, void* item);                    // 0x801EBA20
u32   strlen(const char* s);
void* memset(void* p, int c, unsigned long n);

extern void* gStringTable2;                                   // 0x805FB90C
extern u8    gWorldState[];                                   // 0x803E6D00
extern const char lbl_80354800[];                             // 0x80354800 (string)
extern const char lbl_80381530[];                             // 0x80381530 (string)
extern const char lbl_80381560[];                             // 0x80381560 (string)
extern const char lbl_805F8200[];                             // 0x805F8200 (empty string)
extern u32 lbl_805F9614;                                      // 0x805F9614
extern u8  lbl_805FB6C0;                                      // 0x805FB6C0 (MoveEntityTask hash cached flag)
extern s32 lbl_805FB750;                                      // 0x805FB750
extern u8* lbl_805FB8D8;                                      // 0x805FB8D8 (effect pool: entries of 0x80 bytes)
extern u32 lbl_80601100;                                      // 0x80601100 (cached MoveEntityTask hash)
extern f32 lbl_805FFB68;                                      // 0x805FFB68
extern f32 lbl_805FFB70;                                      // 0x805FFB70
extern f32 lbl_805FFE18;                                      // 0x805FFE18
extern f32 lbl_80600028;                                      // 0x80600028
extern f32 lbl_8060002C;                                      // 0x8060002C
extern f32 lbl_80600030;                                      // 0x80600030
extern f32 lbl_80600034;                                      // 0x80600034

// 0x8018E0CC
// UIHashTree insert: node (+0 key, +8 parent, +0xC left, +0x10 right); tree has root/min/max at +0/+4/+8.
// Returns 0 ok, -1 duplicate key, -2 null tree.
s32 fn_8018E0CC(u8* tree, u8* node) {
    s32 wentLeft = 0;
    s32 wentRight = 0;
    s32 placed = 0;
    if (tree == NULL) {
        fn_8018D370(lbl_80381530, node);  // r4 left over (node)
        return -2;
    }
    u8* cur = *(u8**)(tree + 0x0);
    if (cur == NULL) {
        *(u8**)(tree + 0x0) = node;
        *(u8**)(tree + 0x4) = node;
        *(u8**)(tree + 0x8) = node;
        *(u8**)(node + 0x8) = NULL;
        *(u8**)(node + 0xc) = NULL;
        *(u8**)(node + 0x10) = NULL;
        return 0;
    }
    u32 key = *(u32*)(node + 0x0);
    while (!placed) {
        u32 k = *(u32*)(cur + 0x0);
        if (key < k) {
            u8* l = *(u8**)(cur + 0xc);
            wentLeft = 1;
            if (l != NULL) {
                cur = l;
            } else {
                *(u8**)(cur + 0xc) = node;
                placed = 1;
                *(u8**)(node + 0x8) = cur;
            }
        } else if (key > k) {
            u8* r = *(u8**)(cur + 0x10);
            wentRight = 1;
            if (r != NULL) {
                cur = r;
            } else {
                *(u8**)(cur + 0x10) = node;
                placed = 1;
                *(u8**)(node + 0x8) = cur;
            }
        } else {
            fn_8018D370(lbl_80381560, node);  // r4 left over (node)
            return -1;
        }
    }
    *(u8**)(node + 0xc) = NULL;
    *(u8**)(node + 0x10) = NULL;
    if (wentLeft != 0 && wentRight == 0) {
        *(u8**)(tree + 0x4) = node;   // new minimum
    } else if (wentRight != 0 && wentLeft == 0) {
        *(u8**)(tree + 0x8) = node;   // new maximum
    }
    return 0;
}

// 0x801C79E4
// Releases the items of slot `slot` from the owner's list, clears the slot at +0x28+slot*4 and, if it
// was the current slot (+0x24), picks the nearest used slot (searching down first, then up), else slot 0.
void fn_801C79E4(u8* p, s32 slot) {
    u8* list = *(u8**)(p + 0x18);
    if (list != NULL) {
        u8* link = *(u8**)(list + 0x14);
        u8* node = link ? link + 8 : NULL;
        while (node != NULL) {
            u8* l = *(u8**)(node - 4);
            u8* next = l ? l + 8 : NULL;
            if (slot == *(s32*)(node + 0x7c)) fn_801C7E9C(node, p);
            node = next;
        }
    }
    *(s32*)(p + 0x28 + slot * 4) = -1;
    s32 cur = *(s32*)(p + 0x24);
    if (cur == slot) {
        for (s32 i = cur - 1; i >= 0; i--) {
            if (*(s32*)(p + 0x28 + i * 4) != -1) {
                *(s32*)(p + 0x24) = i;
                return;
            }
        }
        for (s32 i = cur + 1; i < 2; i++) {
            if (*(s32*)(p + 0x28 + i * 4) != -1) {
                *(s32*)(p + 0x24) = i;
                return;
            }
        }
        *(s32*)(p + 0x24) = 0;
        *(s32*)(p + 0x28) = 1;
    }
}

// 0x801B8040
// Initializes a MoveEntityTask: from point (a), to point (b), duration dur (+0x28 = 1/dur), mode flag, owner.
u8* fn_801B8040(u8* p, f32* a, f32* b, u32 owner, s32 flag, f32 dur) {
    *(u16*)(p + 0x78) = 0xFFFF;
    *(u16*)(p + 0x7a) = 0xFFFF;
    *(u16*)(p + 0x7c) = 0xFFFF;
    memset(p, 0, 0x64);
    f32 inv = lbl_805FFB68;
    *(void**)(p + 0x6c) = NULL;
    *(s32*)(p + 0x64) = 0;
    *(s32*)(p + 0x68) = 0;
    *(f32*)(p + 0x70) = inv;
    *(u16*)(p + 0x78) = 0xFFFF;
    *(u16*)(p + 0x7a) = 0xFFFF;
    *(u16*)(p + 0x7c) = 0xFFFF;
    *(s32*)(p + 0x74) = 0;
    *(u8*)(p + 0x7e) = 0;
    *(u8*)(p + 0x7f) = 0;
    *(void**)(p + 0x6c) = (void*)fn_801B8184;
    *(f32*)(p + 0x0) = a[0];
    *(f32*)(p + 0x4) = a[1];
    *(f32*)(p + 0x8) = a[2];
    *(f32*)(p + 0xc) = b[0];
    *(f32*)(p + 0x10) = b[1];
    *(f32*)(p + 0x14) = b[2];
    *(f32*)(p + 0x24) = dur;
    if (dur > inv) inv = lbl_805FFB70 / dur;
    *(f32*)(p + 0x28) = inv;
    *(s32*)(p + 0x2c) = flag ? 1 : 2;
    *(u32*)(p + 0x30) = owner;
    if ((s8)lbl_805FB6C0 == 0) {
        lbl_80601100 = StringHash("MoveEntityTask");
        lbl_805FB6C0 = 1;
    }
    *(u32*)(p + 0x64) = lbl_80601100;
    return p;
}

// 0x801CE7FC
// Per-frame update when the manager's +0x1C record has byte +0x20 == 0: walks the list at +0x18,
// updates nodes of the current group (+0x24) every frame or, for flagged nodes, once every 4 frames
// (at 4x dt), then runs fn_801C9B60 and advances the global phase counter 0..3.
void fn_801CE7FC(u8* p, f32 dt) {
    if (*(u8*)(*(u8**)(p + 0x1c) + 0x20) != 0) return;
    *(s32*)(p + 0x8) = 0;
    *(s32*)(p + 0x4) = 0;
    u8* link = *(u8**)(*(u8**)(p + 0x18) + 0x14);
    u8* node = link ? link + 8 : NULL;
    f32 four = lbl_805FFE18;
    while (node != NULL) {
        u8* l = *(u8**)(node - 4);
        u8* next = l ? l + 8 : NULL;
        if (*(s32*)(p + 0x24) == *(s32*)(node + 0x7c)) {
            if (!((*(u32*)(node + 0x0) >> 5) & 1)) {
                fn_801CD790(p, dt);
            } else if (lbl_805FB750 == *(s32*)(p + 0x4) % 4) {
                fn_801CD790(p, four * dt);
            }
        }
        node = next;
        *(s32*)(p + 0x4) = *(s32*)(p + 0x4) + 1;
    }
    fn_801C9B60(p, dt);
    lbl_805FB750 = lbl_805FB750 + 1;
    if (lbl_805FB750 == 4) lbl_805FB750 = 0;
}

// 0x801DA5F4
// Copies the object array of `src` (+0x54 count, +0x58 objects, +0x5C their +4 field) into `dst`:
// allocates both arrays (mode 1: MemAllocDefault, mode 2: ObjAlloc), clones each object and hooks
// fn_801DA34C / dst into it. Calls virtual slot +0x10 of dst first.
void fn_801DA5F4(u8* dst, u8* src, s32 mode) {
    (*(void (**)(u8*))(*(u8**)dst + 0x10))(dst);
    *(u32*)(dst + 0x8) |= 8;
    s32 n = *(s32*)(src + 0x54);
    if (n != 0) {
        *(s32*)(dst + 0x54) = n;
        if (mode == 1) {
            *(void**)(dst + 0x58) = MemAllocDefault(n * 4);
            *(void**)(dst + 0x5c) = MemAllocDefault(*(s32*)(dst + 0x54) * 4);
        } else if (mode == 2) {
            *(void**)(dst + 0x58) = ObjAlloc(n * 4);
            *(void**)(dst + 0x5c) = ObjAlloc(*(s32*)(dst + 0x54) * 4);
        }
        for (s32 i = 0, off = 0; i < *(s32*)(dst + 0x54); i++, off += 4) {
            *(u8**)(*(u8**)(dst + 0x58) + off) = NULL;
            *(u8**)(*(u8**)(dst + 0x5c) + off) = NULL;
        }
        for (s32 i = 0, off = 0; i < *(s32*)(dst + 0x54); i++, off += 4) {
            u8* obj = fn_801725C8(*(u8**)(*(u8**)(src + 0x58) + off));
            *(u8**)(*(u8**)(dst + 0x58) + off) = obj;
            *(u8**)(*(u8**)(dst + 0x5c) + off) = *(u8**)(*(u8**)(*(u8**)(dst + 0x58) + off) + 0x4);
            u8* o = *(u8**)(*(u8**)(dst + 0x58) + off);
            *(void**)(o + 0x30) = (void*)fn_801DA34C;
            *(u8**)(o + 0x34) = dst;
        }
        if (*(s32*)(dst + 0x54) > 0) {
            *(s32*)(dst + 0x4c) = *(s32*)(*(u8**)(*(u8**)(dst + 0x58)) + 0xb0);
        }
    }
}

// 0x801B9FA4
// Drains two 0x40-entry ring buffers (head at +4 / +0x410, tail at +0xC / +0x418, 16-byte entries)
// of the queue p, invoking cb(target, e[0x10], e[0x14], e[0x18], extra) for matching entries.
// The first ring matches entries whose target (+0xC) is `sender`; the second (only with a real sender)
// matches the sender's two ids at +0xF0/+0xF4 when fn_801B9F14(sender, e[0x10]) is 0.
void fn_801B9FA4(u8* p, u8* sender, void (*cb)(u8*, s32, s32, s32, u8*), u8* extra) {
    u8* target = sender;
    u8* owner;
    if (sender == NULL) {
        target = extra;
        owner = extra;
    } else {
        owner = **(u8***)(sender + 0xa4);
    }
    if (cb == NULL) return;
    u32 head = *(u32*)(p + 0x4);
    u8* ring = p + 4;
    while (head != *(u32*)(ring + 0x8) && !(*(s32*)(owner + 0x28) & 0x200)) {
        u8* e = ring + head * 16;
        head++;
        if (head == 0x40) head = 0;
        if (*(u8**)(e + 0xc) == target) {
            cb(target, *(s32*)(e + 0x10), *(s32*)(e + 0x14), *(s32*)(e + 0x18), extra);
        }
    }
    if (sender == NULL) return;
    u32 id0 = *(u32*)(target + 0xf0);
    u32 id1 = *(u32*)(target + 0xf4);
    if (id0 == 0 && id1 == 0) return;
    u32 head2 = *(u32*)(p + 0x410);
    u8* ring2 = p + 0x410;
    while (head2 != *(u32*)(ring2 + 0x8)) {
        u8* e = ring2 + head2 * 16;
        head2++;
        if (head2 == 0x40) head2 = 0;
        u32 v = *(u32*)(e + 0xc);
        if (v == id0 || v == id1) {
            if (fn_801B9F14(target, *(u32*)(e + 0x10)) == 0) {
                cb(target, *(s32*)(e + 0x10), *(s32*)(e + 0x14), *(s32*)(e + 0x18), extra);
            }
        }
    }
}

// 0x80173568
// Serialized size of the record: name length + 0x78 + 4 per entry, plus the first word of each
// entry's block at +0x74+i*8 (count at +0x6C), the word at *(+0x64) and the size of the block at +0x68.
s32 fn_80173568(u8* p) {
    s32 n = *(s32*)(p + 0x6c);
    s32 size = (s32)strlen(*(const char**)(p + 0x5c)) + ((n - 1) * 4 + 0x78) + 1;
    for (s32 i = 0; i < n; i++) {
        size += **(s32**)(p + 0x74 + i * 8);
    }
    s32* extra = *(s32**)(p + 0x64);
    if (extra != NULL) size += *extra;
    u8* blk = *(u8**)(p + 0x68);
    if (blk != NULL) size += (s32)fn_80184278(blk);
    return size;
}

// 0x80198438
// Per-frame update of the UI screen stack (count at +0x60, screens at +0x40+i*4): updates the top
// screen via vslot 0x28, then every screen via vslot 0x18 + fn_80199738 from top to bottom, and
// swaps the active screen (gScreenSets +0x70 new, +0x74 previous), playing the hide anim on the old one.
s32 fn_80198438(u8* p, f32 dt) {
    if (ZoneIsLoaded(gWorldState, *(s32*)(p + 0x4)) != 0) {
        s32 n = *(s32*)(p + 0x60);
        if (n >= 1) {
            u8* top = *(u8**)(p + n * 4 + 0x3c);
            (*(void (**)(u8*, f32))(*(u8**)(top + 0x138) + 0x28))(top, dt);
        }
        u8* gs = (u8*)gScreenSets;
        *(u8**)(gs + 0x70) = NULL;
        for (s32 i = *(s32*)(p + 0x60) - 1; i >= 0; i--) {
            u8* scr = *(u8**)(p + 0x40 + i * 4);
            (*(void (**)(u8*, f32))(*(u8**)(scr + 0x138) + 0x18))(scr, dt);
            fn_80199738(*(u8**)(p + 0x40 + i * 4));
        }
        gs = (u8*)gScreenSets;
        u8* prev = *(u8**)(gs + 0x74);
        if (prev != NULL && *(s32*)(prev + 0xbc) == 0 && prev != *(u8**)(gs + 0x70)) {
            if (*(s32*)(prev + 0x60) == 4) {
                if (*(u8*)(prev + 0x16c) != 0) {
                    *(u8*)(prev + 0x16c) = 0;
                    UIPlayAnim(*(u8**)((u8*)gScreenSets + 0x74), 0xF5A9CE6Cu);
                }
            } else {
                UIPlayAnim(prev, 0xF5A9CE6Cu);
            }
        }
        gs = (u8*)gScreenSets;
        u8* cur = *(u8**)(gs + 0x70);
        if (cur != NULL && (*(u8**)(gs + 0x74) != cur || *(s32*)(cur + 0x60) == 4)) {
            (*(void (**)(u8*))(*(u8**)(cur + 0x138) + 0x3c))(cur);
        }
        gs = (u8*)gScreenSets;
        *(u8**)(gs + 0x74) = *(u8**)(gs + 0x70);
    }
    return 0;
}

// 0x801D6E4C
// Builds a camera matrix (12 floats) from the 3x4 source at m: reorders the rows, runs fn_80144C60 on
// them, negates rows 0 and 2, and hands the result to fn_8016E304. With the flag at lbl_805F9614 set,
// defers to fn_801D6A4C instead.
void fn_801D6E4C(u8* unused, f32* m, f32* cam) {
    f32 t[12];
    if (lbl_805F9614 != 0) {
        fn_801D6A4C(cam, m);
        return;
    }
    t[3] = m[6];
    t[4] = m[7];
    t[5] = m[8];
    t[0] = m[3];
    t[1] = m[4];
    t[2] = m[5];
    t[6] = m[0];
    t[7] = m[1];
    t[8] = m[2];
    t[9] = m[9];
    t[10] = m[10];
    t[11] = m[11];
    fn_80144C60(t);
    cam[0] = -t[0];
    cam[1] = -t[1];
    cam[2] = -t[2];
    cam[3] = t[3];
    cam[4] = t[4];
    cam[5] = t[5];
    cam[6] = -t[6];
    cam[7] = -t[7];
    cam[8] = -t[8];
    cam[9] = t[9];
    cam[10] = t[10];
    cam[11] = t[11];
    fn_8016E304(cam);
}

// 0x801EAE20
// Effect pool update: runs fn_801EA3C8 on the entry of id (+0x24); the entry of id2 (+0x34) gets
// +0x40 = 1 / max(1/x, eps) where x is the first float of the first entry (0 when ~0). Stores dt at
// +0x14 and sets bit 2 of +0x38.
void fn_801EAE20(u8* p, f32 dt) {
    u16 id = *(u16*)(p + 0x24);
    u8* pool = lbl_805FB8D8;
    u16 idTmp = id;
    u8* e1 = fn_801E7460(pool, &idTmp) ? pool + ((id & 0xFF) << 7) : NULL;
    if (e1 != NULL) {
        fn_801EA3C8(e1, dt);
        u16 id2 = *(u16*)(p + 0x34);
        pool = lbl_805FB8D8;
        u16 id2Tmp = id2;
        u8* e2 = fn_801E7460(pool, &id2Tmp) ? pool + ((id2 & 0xFF) << 7) : NULL;
        if (e2 != NULL) {
            f32 v = *(f32*)(e1 + 0x0);
            f32 r;
            if (v > lbl_80600030 && v < lbl_8060002C) {
                r = lbl_80600028;
            } else {
                r = lbl_80600034 / v;
            }
            if (r > lbl_80600030 && r < lbl_8060002C) r = lbl_8060002C;
            *(f32*)(e2 + 0x40) = lbl_80600034 / r;
        }
    }
    *(f32*)(p + 0x14) = dt;
    *(u32*)(p + 0x38) |= 2;
}

// 0x8019EE40
// Sets a UI text element to string id `id` (high bit: second table; 0xFFFFFFFF: "Not found").
// Resets the cached string pointer state at +0x140..+0x148 when it changes, remembers id at +0x16C.
s32 fn_8019EE40(u8* el, u32 id, s32 mode) {
    u32 cached;
    if (id == 0xFFFFFFFFu) {
        cached = 0;
    } else if ((id & 0x80000000u) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        u8* arr = *(u8**)(t + 0x4);
        u32 idx = id & 0x7FFFFFFFu;
        cached = (arr != NULL && idx < *(u32*)(t + 0xc)) ? *(u32*)(arr + idx * 4) : 0;
    } else {
        u8* t = (u8*)gStringTable;
        u8* arr = *(u8**)(t + 0x4);
        cached = (arr != NULL && id < *(u32*)(t + 0xc)) ? *(u32*)(arr + id * 4) : 0;
    }
    if (*(u32*)(el + 0x140) != cached) {
        *(u32*)(el + 0x140) = cached;
        *(s32*)(el + 0x144) = 0;
        *(u8*)(el + 0x148) = 0;
    }
    const char* text;
    if (id == 0xFFFFFFFFu) {
        text = lbl_80354800;
    } else if ((id & 0x80000000u) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        u8* arr = *(u8**)(t + 0x0);
        u32 idx = id & 0x7FFFFFFFu;
        text = (arr != NULL && idx < *(u32*)(t + 0xc)) ? *(const char**)(arr + idx * 4) : lbl_805F8200;
    } else {
        u8* t = (u8*)gStringTable;
        u8* arr = *(u8**)(t + 0x0);
        // (the original has a further id == 0xFFFFFFFF -> "NOT FOUND" fallback here; unreachable)
        text = (arr != NULL && id < *(u32*)(t + 0xc)) ? *(const char**)(arr + id * 4) : lbl_805F8200;
    }
    UISetText(el, text, mode);
    *(u32*)(el + 0x16c) = id;
    *(u8*)(el + 0x1f0) = 0;
    return 0;
}

// 0x801C3C2C
// Per-frame update of all groups (count +0x18, 0x80-byte records at +0x1C): resets the counter at +0x24,
// then for each group either counts its entries (when +0x4C != 0) or updates every node of the list at
// +0x78 with fn_801C5444, retiring flagged nodes (clears bits 0/1/6/7 and removes via fn_801EBA20).
void fn_801C3C2C(u8* p, f32 dt) {
    *(s32*)(p + 0x24) = 0;
    u8* grp = *(u8**)(p + 0x1c);
    for (s32 i = 0; i < *(s32*)(p + 0x18); i++, grp += 0x80) {
        if (*(s32*)(grp + 0x4c) != 0) {
            u8* link = *(u8**)(*(u8**)(grp + 0x7c) + 0x14);
            u8* node = link ? link + 8 : NULL;
            while (node != NULL) {
                u8* l = *(u8**)(node - 4);
                node = l ? l + 8 : NULL;
                *(s32*)(p + 0x24) = *(s32*)(p + 0x24) + 1;
            }
        } else {
            u8* link = *(u8**)(*(u8**)(grp + 0x78) + 0x14);
            u8* node = link ? link + 8 : NULL;
            while (node != NULL) {
                u8* l = *(u8**)(node - 4);
                u8* next = l ? l + 8 : NULL;
                fn_801C5444(node, dt);
                if (*(u32*)(node + 0x8) < 2) {
                    s32 flags = *(s32*)(node + 0x4);
                    if (flags & 2) {
                        *(s32*)(node + 0x4) = flags & ~1 & 0xFFFFFF3F & ~2;
                        if (flags & 8) fn_801EBA20(*(u8**)(grp + 0x78), node);
                    }
                }
                s32 f2 = *(s32*)(node + 0x4);
                if ((f2 & 8) && !(f2 & 2)) {
                    node = next;
                    if (node == NULL) break;
                }
                l = *(u8**)(node - 4);
                node = l ? l + 8 : NULL;
                *(s32*)(p + 0x24) = *(s32*)(p + 0x24) + 1;
            }
        }
    }
}

}  // extern "C"
