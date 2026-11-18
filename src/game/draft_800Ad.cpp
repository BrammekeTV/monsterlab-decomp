// Functions in 0x800A34xx-0x800A42xx (saw-blade prop family: init, anim states, spawn-slot setup).
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

typedef u8*  (*VFnRetPtr)(u8* self);            // virtual slot returning a pointer
typedef void (*VFnSelf)(u8* self);              // virtual slot taking only the object
typedef void (*VFnDelta)(u8* self, f32 dt);     // virtual slot taking a time delta

static const u32 kHashCastleSaw = 0x5AADD230u;       // "castle_saw"
static const u32 kHashSawObstacle01A = 0xAB868DB8u;  // "saw_obstacle_01A"
static const u32 kHashSawSlow01A = 0x8D6957B0u;      // "saw_slow_01A"
static const u32 kHashSawGate01A = 0x677E3F1Bu;      // "saw_gate_01A"

extern "C" {
s32   AnimSetFindClip(void* set, s32 anim);                                   // 0x801AC2FC
void  EntitySyncTransform(void* entity, int a);                               // 0x801B4F4C
void  PropPlayAnim(void* prop, int a, int b, BOOL once);                      // 0x80069350
void  fn_8006AADC(u8* p, f32 delta);                                          // 0x8006AADC
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8);            // 0x8006FA44
u8*   fn_8006FBDC(void);                                                      // 0x8006FBDC
void  fn_800A1850(u8* p, u8* target);                                         // 0x800A1850
void  fn_800A39A4(u8* p);                                                     // 0x800A39A4
void  fn_800A424C(u8* p);                                                     // 0x800A424C
u8*   fn_800A4EE4(u8* tree, u32 key);                                         // 0x800A4EE4
u8*   fn_800A5B4C(u8* p, s32* keyp, void** outNode, u8* outLeft, u8* outAll); // 0x800A5B4C
void  fn_800A59BC(u8* tree, u8* hint, u8 left, u8 all, void* val);            // 0x800A59BC
void  fn_800C1B78(u8* p);                                                     // 0x800C1B78
void  fn_800C1D44(u8* p);                                                     // 0x800C1D44
void  fn_800C2174(u8* a3, void* a4, s32 a5);                                  // 0x800C2174
void  fn_800C4118(u8* p);                                                     // 0x800C4118
BOOL  fn_801B50B0(void* unused, u8* obj);                                     // 0x801B50B0
s32   fn_801B5338(u8* obj, u8* target, u32 hash, f32* vec, u8 flag);          // 0x801B5338
void  fn_801EC080(u8* a, u8* b);                                              // 0x801EC080

void  fn_800A3638(u8** out, u8* tree, u32* key);                              // 0x800A3638
void  fn_800A3E08(u8* arg0, u8* arg1);                                        // 0x800A3E08
void  fn_800A3F2C(u8* arg0, u8* arg1);                                        // 0x800A3F2C

extern u8   lbl_803C6BF0[];   // 0x803C6BF0 (block passed to fn_8006FA44)
extern u8   lbl_803C6C08[];   // 0x803C6C08 (three 8-byte pairs)
extern f32  lbl_803C6C2C[];   // 0x803C6C2C (3-float table)
extern u8   lbl_805F8B08[];   // 0x805F8B08 (symbol address used as value)
extern u8   lbl_805F8B0C[];   // 0x805F8B0C
extern u8   lbl_805F8B10[];   // 0x805F8B10
extern u32  lbl_805F8B38;     // 0x805F8B38 (lookup key)
extern u8*  lbl_805FB160;     // 0x805FB160 (tree)
extern u8*  lbl_805FB164;     // 0x805FB164 (tree)
extern f32  lbl_805FDE40;     // 0x805FDE40
extern f32  lbl_805FDE50;     // 0x805FDE50
extern f32  lbl_805FDE90;     // 0x805FDE90

// 0x800A3418
// Sets the 3-float table to {a, b, a}, registers the saw hashes, and fills three pointer pairs.
void fn_800A3418(void) {
    f32 a = lbl_805FDE40;
    lbl_803C6C2C[0] = a;
    lbl_803C6C2C[1] = lbl_805FDE50;
    lbl_803C6C2C[2] = a;
    fn_8006FA44(lbl_803C6BF0, 0x27A7F2C6, 3, (s32)kHashCastleSaw, (s32)0x85AB5092u, (s32)0x8D4B9D04u);
    fn_800C2174(lbl_803C6C08, lbl_805F8B08, 1);
    fn_800C2174(lbl_803C6C08 + 0x8, lbl_805F8B0C, 1);
    fn_800C2174(lbl_803C6C08 + 0x10, lbl_805F8B10, 1);
}

// 0x800A355C
// Init: base init, picks a linked object from the tree, binds it, resets the anim-state flag.
void fn_800A355C(u8* arg0) {
    f32 vec[3];
    u8* out[2];

    fn_800C1B78(arg0);
    u8* link = *(u8**)(arg0 + 0x9C);
    u32 key = link ? *(u32*)(link + 0x8) : 0;
    u8* obj = fn_800A4EE4(lbl_805FB160, key);
    *(u8**)(arg0 + 0x2A4) = obj;
    if (obj != NULL) {
        vec[2] = lbl_805FDE90;
        vec[1] = lbl_805FDE90;
        vec[0] = lbl_805FDE90;
        fn_801B5338(arg0, *(u8**)(arg0 + 0x2A4), 0x206114EFu, vec, 0);
        fn_800A3E08(*(u8**)(arg0 + 0x2A4), arg0);
    }
    fn_800A3638(out, lbl_805FB164, &lbl_805F8B38);
    u8* r = (*(VFnRetPtr*)(*(u8**)arg0 + 0x10))(arg0);
    *(s32*)(*(u8**)(*(u8**)r + 0x14) + 0x20) = 0;
    if (fn_8006FBDC() != NULL) {
        fn_800A1850(fn_8006FBDC(), arg0);
    }
}

// 0x800A3638
// Equal-range lookup in a tree: out[0] = first node with key, out[1] = first node past it (or end).
void fn_800A3638(u8** out, u8* tree, u32* key) {
    u8* node = *(u8**)(tree + 4);
    u8* upper = tree + 4;
    u32 k = 0;
    while (node != NULL) {
        u32 nk = *(u32*)(node + 0xC);
        k = *key;
        if (k < nk) {
            upper = node;
            node = *(u8**)(node + 0x0);
        } else if (nk < k) {
            node = *(u8**)(node + 0x4);
        } else {
            u8* left = *(u8**)(node + 0x0);
            u8* right = *(u8**)(node + 0x4);
            while (left != NULL) {
                if (*(u32*)(left + 0xC) >= k) {
                    node = left;
                    left = *(u8**)(left + 0x0);
                } else {
                    left = *(u8**)(left + 0x4);
                }
            }
            while (right != NULL) {
                if (k < *(u32*)(right + 0xC)) {
                    upper = right;
                    right = *(u8**)(right + 0x0);
                } else {
                    right = *(u8**)(right + 0x4);
                }
            }
            out[0] = node;
            out[1] = upper;
            return;
        }
    }
    out[0] = upper;
    out[1] = upper;
}

// 0x800A36E8
// Teardown: plays the clip from the anim set, flags the component, releases the linked object.
void fn_800A36E8(u8* arg0) {
    u8* link = *(u8**)(arg0 + 0x9C);
    if (link != NULL) {
        u8* set = *(u8**)(link + 0x48);
        if (set != NULL) {
            PropPlayAnim(arg0, AnimSetFindClip(set, (s32)0xECFDEE97u), 0, 1);
            u8* r = (*(VFnRetPtr*)(*(u8**)arg0 + 0x10))(arg0);
            *(s32*)(*(u8**)(*(u8**)r + 0x14) + 0x20) = 1;
        }
    }
    u8* obj = *(u8**)(arg0 + 0x2A4);
    if (obj != NULL) {
        fn_801B50B0(arg0, obj);
        fn_800A424C(*(u8**)(arg0 + 0x2A4));
    }
    fn_800C1D44(arg0);
}

// 0x800A3790
// Base state init, then plays the clip from the anim set and flags the component.
void fn_800A3790(u8* arg0) {
    fn_800C4118(arg0);
    u8* link = *(u8**)(arg0 + 0x9C);
    if (link != NULL) {
        u8* set = *(u8**)(link + 0x48);
        if (set != NULL) {
            PropPlayAnim(arg0, AnimSetFindClip(set, (s32)0xECFDEE97u), 0, 1);
            u8* r = (*(VFnRetPtr*)(*(u8**)arg0 + 0x10))(arg0);
            *(s32*)(*(u8**)(*(u8**)r + 0x14) + 0x20) = 1;
        }
    }
}

// 0x800A381C
// Update: sets flag 1 on each list item's transform, runs base + virtual update, clears the flag.
void fn_800A381C(u8* arg0, f32 dt) {
    for (u8* n = *(u8**)(arg0 + 0xB0); n != NULL; n = *(u8**)(n + 0x8)) {
        u8* item = *(u8**)(n + 0x4);
        if (item != NULL) {
            u8* t = *(u8**)(item + 0x4);
            *(u32*)(t + 0x20) |= 1;
        }
    }
    fn_8006AADC(arg0, dt);
    (*(VFnDelta*)(*(u8**)arg0 + 0x74))(arg0, dt);
    for (u8* n = *(u8**)(arg0 + 0xB0); n != NULL; n = *(u8**)(n + 0x8)) {
        u8* item = *(u8**)(n + 0x4);
        if (item != NULL) {
            u8* t = *(u8**)(item + 0x4);
            *(u32*)(t + 0x20) &= ~1u;
        }
    }
}

// 0x800A3CAC
// Looks up each 0xC8-byte record's key in the tree; on the first hit inserts a value there.
// Then finishes with fn_800A39A4 and clears flags on the prop.
void fn_800A3CAC(u8* arg0, u8* arg1, s32 arg2) {
    struct { s32 key; u8* obj; } sp10[3];
    void* spC;
    u8 sp8, sp9;

    u8* end = arg1 + 4;
    s32 count = *(s32*)(arg0 + 0x94);
    for (s32 i = 0, off = 0; i < count; i++, off += 0xC8) {
        u8* tbl = *(u8**)(arg0 + 0x98);
        u8* best = end;
        u8* cur = *(u8**)(arg1 + 4);
        while (cur != NULL) {
            if (*(u32*)(cur + 0xC) >= *(u32*)(tbl + off)) {
                best = cur;
                cur = *(u8**)(cur + 0x0);
            } else {
                cur = *(u8**)(cur + 0x4);
            }
        }
        if (best == end || *(u32*)(tbl + off) < *(u32*)(best + 0xC)) best = end;
        if (best != end) {
            sp10[0].key = arg2;
            sp10[0].obj = arg0;
            sp10[1].key = arg2;
            sp10[1].obj = arg0;
            sp10[2].key = arg2;
            sp10[2].obj = arg0;
            u8* node = best + 0x10;
            u8* hint = fn_800A5B4C(node, &sp10[0].key, &spC, &sp8, &sp9);
            fn_800A59BC(node, hint, sp8, sp9, sp10);
            break;
        }
    }
    fn_800A39A4(arg0);
    *(u32*)(arg0 + 0x15C) |= 0x400000;
    if ((*(VFnRetPtr*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* r = (*(VFnRetPtr*)(*(u8**)arg0 + 0xC))(arg0);
        *(u32*)(r + 0x8) &= ~2u;
    }
    u8* t = *(u8**)(arg0 + 0x4);
    *(u32*)(t + 0x20) &= ~3u;
}

// 0x800A3E08
// Copies position (3 floats) and orientation (4 floats) from arg1's transform into arg0's,
// marks it dirty, syncs the entity, then runs fn_801EC080 and fn_800A3F2C.
void fn_800A3E08(u8* arg0, u8* arg1) {
    u8* src = *(u8**)(arg1 + 0x4);
    u8* dst = *(u8**)(arg0 + 0x4);
    *(f32*)(dst + 0x0) = *(f32*)(src + 0x0);
    *(f32*)(dst + 0x4) = *(f32*)(src + 0x4);
    *(f32*)(dst + 0x8) = *(f32*)(src + 0x8);
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    src = *(u8**)(arg1 + 0x4);
    dst = *(u8**)(arg0 + 0x4);
    *(f32*)(dst + 0xC) = *(f32*)(src + 0xC);
    *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
    *(f32*)(dst + 0x14) = *(f32*)(src + 0x14);
    *(f32*)(dst + 0x18) = *(f32*)(src + 0x18);
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u8*)(arg0 + 0x364) = 1;
    EntitySyncTransform(arg0, 0);
    fn_801EC080(arg0 + 0x94, arg0);
    fn_800A3F2C(arg0, arg1);
}

// 0x800A3F2C
// For each of three groups, looks up the saw-type hash in the tree and assigns the first matching
// unclaimed entry to every slot (flag byte at +0x264 clear), then sorts the +0x338 entries by x.
void fn_800A3F2C(u8* arg0, u8* arg1) {
    (void)arg1;
    u8* tree = lbl_805FB164;
    u8* slotBase = arg0;
    u8* countBase = arg0;
    for (s32 g = 0; g < 3; g++, countBase += 4, slotBase += 0x28) {
        u32 key = 0;
        switch (g) {
        case 0: key = kHashSawObstacle01A; break;
        case 1: key = kHashSawSlow01A; break;
        case 2: key = kHashSawGate01A; break;
        }
        u8* slot = slotBase;
        for (s32 j = 0; j < *(s32*)(countBase + 0x264); j++, slot += 4) {
            u8* range[2];
            fn_800A3638(range, tree, &key);
            u8* node = range[0];
            u8* endNode = range[1];
            while (node != endNode) {
                u8* obj = *(u8**)(node + 0x10);
                if (*(u8*)(obj + 0x264) == 0) {
                    *(u8**)(slot + 0x2E8) = obj;
                    (*(VFnSelf*)(*(u8**)obj + 0xBC))(obj);
                    break;
                }
                // advance to the next node
                u8* right = *(u8**)(node + 0x4);
                if (right != NULL) {
                    u8* n = right;
                    while (*(u8**)n != NULL) n = *(u8**)n;
                    node = n;
                } else {
                    u8* parent = (u8*)((uintptr_t)*(u8**)(node + 0x8) & ~(uintptr_t)1);
                    while (node != *(u8**)parent) {
                        node = parent;
                        parent = (u8*)((uintptr_t)*(u8**)(node + 0x8) & ~(uintptr_t)1);
                    }
                    node = parent;
                }
            }
        }
    }

    // sort the +0x338 entries (and their +0x2C0 indices) ascending by x of entry->+4
    u8* pi = arg0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x26C); i++, pi += 4) {
        f32* v = *(f32**)(*(u8**)(pi + 0x338) + 0x4);
        f32 mx = v[0], my = v[1], mz = v[2];
        u8* pj = arg0 + (i + 1) * 4;
        for (s32 j = i + 1; j < *(s32*)(arg0 + 0x26C); j++, pj += 4) {
            u8* ent = *(u8**)(pj + 0x338);
            f32* w = *(f32**)(ent + 0x4);
            f32 x = w[0], y = w[1], z = w[2];
            if (x < mx) {
                u8* tmp = *(u8**)(pi + 0x338);
                mx = x;
                *(u8**)(pi + 0x338) = ent;
                *(u8**)(pj + 0x338) = tmp;
                s32 t = *(s32*)(pi + 0x2C0);
                my = y;
                *(s32*)(pi + 0x2C0) = *(s32*)(pj + 0x2C0);
                mz = z;
                *(s32*)(pj + 0x2C0) = t;
            }
        }
        (void)my;
        (void)mz;
    }
    *(s32*)(arg0 + 0x360) = 0;
}

// 0x800A415C
// Update: base update, then for each slot places its object at the record's position, marks the
// transform dirty and syncs it.
void fn_800A415C(u8* arg0, f32 dt) {
    fn_8006AADC(arg0, dt);
    u8* slotBase = arg0;
    u8* countBase = arg0;
    for (s32 g = 0; g < 3; g++, countBase += 4, slotBase += 0x28) {
        u8* slot = slotBase;
        for (s32 j = 0; j < *(s32*)(countBase + 0x264); j++, slot += 4) {
            u8* obj = *(u8**)(slot + 0x2E8);
            u8* t = *(u8**)(obj + 0x4);
            u8* rec = *(u8**)(arg0 + 0x98) + *(s32*)(slot + 0x270) * 0xC8;
            *(f32*)(t + 0x0) = *(f32*)(rec + 0x5C);
            *(f32*)(t + 0x4) = *(f32*)(rec + 0x60);
            *(f32*)(t + 0x8) = *(f32*)(rec + 0x64);
            *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x8000;
            *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
            *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x4000;
            *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
            EntitySyncTransform(*(u8**)(slot + 0x2E8), 0);
        }
    }
}

}  // extern "C"
