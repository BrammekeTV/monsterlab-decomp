// Small render-queue, UI stick, action and list helpers (draft_miss171).
// Functional rewrites checked against the asm; not byte-matching.
#include "types.h"
#include "revolution.h"

typedef void* (*HeapAllocFn)(s32 size, s32 align, s32 flags, s32 a4, s32 a5);
typedef void (*VFnObjList)(void* self, void* list);

extern "C" {
void* memcpy(void* d, const void* s, size_t n);
void StickRead(s32 channel, f32* x, f32* y);                       // 0x80147704
void fn_8019C348(void* obj);                                       // 0x8019C348
void fn_8018D370(const char* fmt, ...);                            // 0x8018D370
void fn_801BBB68(void* mgr, s32 id, s32 a, s32 b);                 // 0x801BBB68
s32 TriggerDoAction(void* action, void* a1, void* a2);           // 0x801E0054 (returns r3)
s32  fn_801BC4C4(u8* a, s32 b);                                    // 0x801BC4C4
void fn_801BF714(u8* node);                               // 0x801BF714

extern u8    sRendThread[];                                        // 0x8040AEA8
extern s32   lbl_805FB560;                                         // 0x805FB560
extern u8*   lbl_805FB608;                                         // 0x805FB608 (allocator object)
extern void* lbl_805FB670;                                         // 0x805FB670 (pad state)
extern void* gStringTable2;                                        // 0x805FB90C
extern f32   lbl_805FF5AC;                                         // 0x805FF5AC (0.0f)
extern f32   lbl_805FF5B0;                                         // 0x805FF5B0
extern f32   lbl_805FF91C;                                         // 0x805FF91C (stick low dead-zone edge)
extern f32   lbl_805FF920;                                         // 0x805FF920 (stick high dead-zone edge)
extern f32   lbl_805FF930;                                         // 0x805FF930
extern u8    lbl_80382748[];                                       // 0x80382748 (overflow log string)
extern u8    lbl_80383EE8[];                                       // 0x80383EE8
extern u8    lbl_80383F20[];                                       // 0x80383F20
extern u8    lbl_80383F58[];                                       // 0x80383F58
extern u8    lbl_80383F90[];                                       // 0x80383F90
extern u8    lbl_80383FC4[];                                       // 0x80383FC4
}

// Resolves a packed action word: type 3 indexes the context's table (ctx+0x54), others pass through.
static u32 ResolveActionWord(u32 word, u8* ctx) {
    if (((word >> 27) & 0xF) != 3) {
        return word;
    }
    u32* table = *(u32**)(ctx + 0x54);
    u32 idx = word & 0x3FFFF;
    if (table != NULL && idx < table[0]) {
        return table[idx + 1];
    }
    return 0x3FFFF;
}

// Points gStringTable2 at the context's first sub-object, then fires the resolved action.
static s32 FireActionWord(u32 word, u8* ctx) {
    gStringTable2 = *(void**)(*(u8**)ctx + 0xC);
    u32 action = ResolveActionWord(word, ctx);
    return TriggerDoAction(&action, *(void**)(ctx + 0x10), NULL);
}

extern "C" {

// 0x8015E6D0
// Pushes a type-2 record (frame, value) into the 0x40-entry render request ring.
// Only r4 (frame) and r7 (value) are inputs; r3, r5, r6 are overwritten before use.
void fn_8015E6D0(void* unusedR3, s32 frame, s32 unusedR5, s32 unusedR6, s32 value) {
    OSWaitSemaphore((OSSemaphore*)(sRendThread + 0x2618));
    u8* ring = sRendThread + 0x2318;
    s32 idx = lbl_805FB560;
    s32 next = idx + 1;
    lbl_805FB560 = next;
    u8* rec = ring + idx * 0xC;
    *(s32*)rec = 2;
    *(s32*)(rec + 0x4) = value;
    *(s32*)(rec + 0x8) = frame;
    if (next >= 0x40) {
        lbl_805FB560 = 0;
    }
    OSSignalSemaphore((OSSemaphore*)(sRendThread + 0x2624));
}

// 0x8015E74C
// Same as fn_8015E6D0 with record type 3.
void fn_8015E74C(void* unusedR3, s32 frame, s32 unusedR5, s32 unusedR6, s32 value) {
    OSWaitSemaphore((OSSemaphore*)(sRendThread + 0x2618));
    u8* ring = sRendThread + 0x2318;
    s32 idx = lbl_805FB560;
    s32 next = idx + 1;
    lbl_805FB560 = next;
    u8* rec = ring + idx * 0xC;
    *(s32*)rec = 3;
    *(s32*)(rec + 0x4) = value;
    *(s32*)(rec + 0x8) = frame;
    if (next >= 0x40) {
        lbl_805FB560 = 0;
    }
    OSSignalSemaphore((OSSemaphore*)(sRendThread + 0x2624));
}

// 0x8016A570
// Resets (kind 0) or copies (kind 1) a block of 0x1C/0xC-byte records; other kinds do nothing.
void fn_8016A570(u8* arg0) {
    s32 kind = *(s32*)(arg0 + 0x4);
    if (kind == 0) {
        u8* rec = arg0 + *(s32*)(arg0 + 0x8);
        for (s32 i = 0; i < *(s32*)(arg0 + 0x10); i++) {
            *(f32*)(rec + 0x0) = lbl_805FF5AC;
            *(f32*)(rec + 0x4) = lbl_805FF5AC;
            *(f32*)(rec + 0x8) = lbl_805FF5AC;
            *(f32*)(rec + 0xC) = lbl_805FF5AC;
            *(f32*)(rec + 0x10) = lbl_805FF5AC;
            *(f32*)(rec + 0x14) = lbl_805FF5AC;
            *(f32*)(rec + 0x18) = lbl_805FF5B0;
            rec += 0x1C;
        }
    } else if (kind == 1) {
        u8* srcBase = *(u8**)(arg0 + 0x14);
        u8* src = srcBase + *(s32*)(srcBase + 0x30);
        u8* dst = arg0 + *(s32*)(arg0 + 0x8);
        for (s32 i = 0; i < *(s32*)(arg0 + 0x10); i++) {
            *(s32*)(dst + 0x0) = *(s32*)(src + 0x0);
            *(s32*)(dst + 0x4) = *(s32*)(src + 0x4);
            *(s32*)(dst + 0x8) = 0;
            src += 8;
            dst += 0xC;
        }
    }
}

// 0x8016B478
// Copies the header fields of arg1 into arg0, clamps the element count to arg0's capacity,
// then copies the two data blocks.
void fn_8016B478(u8* arg0, u8* arg1) {
    s32 count = *(s32*)(arg1 + 0x10);
    s32 cap = *(s32*)(arg0 + 0xC);
    if (cap < count) {
        count = cap;
    }
    *(s32*)(arg0 + 0x10) = count;
    *(void**)(arg0 + 0x14) = *(void**)(arg1 + 0x14);
    *(void**)(arg0 + 0x1C) = *(void**)(arg1 + 0x1C);
    *(f32*)(arg0 + 0x24) = *(f32*)(arg1 + 0x24);
    memcpy(arg0 + 0x28, arg1 + 0x28, count * 4);
    memcpy(arg0 + *(s32*)(arg0 + 0x8), arg1 + *(s32*)(arg1 + 0x8),
           *(s32*)(arg0 + 0x18) * count);
}

// 0x8016B50C
// Allocates a 0x50-byte record plus kind-dependent header space and count*elemSize data
// from the allocator object (slot 0 is alloc(size, 0x10, 2, 0, 0)).
// kind 0 uses 0x1C-byte elements, kind 1 uses 0xC, others 0.
u8* fn_8016B50C(s32 kind, s32 count) {
    s32 elemSize = 0;
    if (kind == 0) {
        elemSize = 0x1C;
    } else if (kind == 1) {
        elemSize = 0xC;
    }
    s32 dataSize = (count * 4 + 0x37) & ~0xF;
    HeapAllocFn alloc = *(HeapAllocFn*)lbl_805FB608;
    u8* rec = (u8*)alloc(dataSize + elemSize * count, 0x10, 2, 0, 0);
    *(s32*)(rec + 0x0) = 0x50;
    *(s32*)(rec + 0x4) = kind;
    *(s32*)(rec + 0x8) = dataSize;
    *(s32*)(rec + 0xC) = count;
    *(s32*)(rec + 0x10) = 0;
    *(s32*)(rec + 0x14) = 0;
    *(s32*)(rec + 0x18) = elemSize;
    *(s32*)(rec + 0x1C) = 0;
    *(s32*)(rec + 0x20) = 0;
    *(f32*)(rec + 0x24) = lbl_805FF5AC;
    return rec;
}

// 0x8017387C
// Loads a 3-float vector from arg1's data block (offset at +8); if arg1 has a transform
// matrix (+0x1C), writes the transformed vector (3x3 rows plus translation) instead.
void fn_8017387C(f32* out, u8* arg1) {
    u8* src = *(u8**)(arg1 + 0x14);
    u8* xf = *(u8**)(arg1 + 0x1C);
    s32 off = *(s32*)(src + 0x8);
    f32 x = *(f32*)(src + off);
    f32 y = *(f32*)(src + off + 0x4);
    f32 z = *(f32*)(src + off + 0x8);
    out[0] = x;
    out[1] = y;
    out[2] = z;
    if (xf != NULL) {
        f32 nx = *(f32*)(xf + 0x24) + ((*(f32*)(xf + 0x0) * x + *(f32*)(xf + 0xC) * y) + *(f32*)(xf + 0x18) * z);
        f32 ny = *(f32*)(xf + 0x28) + ((*(f32*)(xf + 0x4) * x + *(f32*)(xf + 0x10) * y) + *(f32*)(xf + 0x1C) * z);
        f32 nz = *(f32*)(xf + 0x2C) + ((*(f32*)(xf + 0x8) * x + *(f32*)(xf + 0x14) * y) + *(f32*)(xf + 0x20) * z);
        out[0] = nx;
        out[1] = ny;
        out[2] = nz;
    }
}

// 0x8018D534
// Callback: forwards its first argument to fn_8019C348 and returns 0.
s32 fn_8018D534(void* obj) {
    fn_8019C348(obj);
    return 0;
}

// 0x80198350
// Stick-to-direction flags: writes the held-buttons word to out, then, if the pad object
// (lbl_805FB670) has flag 2, ORs in a direction bit (0x10000/0x20000 for X, 0x8000/0x4000 for Y)
// for the dominant stick axis when it is past the dead zone.
void fn_80198350(s32 pad, u32 held, u8* out) {
    if (out == NULL) {
        return;
    }
    *(u32*)out = held;
    if ((*(s32*)((u8*)lbl_805FB670 + 0x4) & 2) == 0) {
        return;
    }
    f32 x;
    f32 y;
    StickRead(pad * 0x10 + 0xE, &x, &y);
    f32 ax = x < 0.0f ? -x : x;
    f32 ay = y < 0.0f ? -y : y;
    u32* flags = (u32*)out;
    if (ax >= ay) {
        if (x < lbl_805FF91C) {
            *flags |= 0x10000u;
            return;
        }
        if (x > lbl_805FF920) {
            *flags |= 0x20000u;
        }
    } else {
        if (y < lbl_805FF91C) {
            *flags |= 0x8000u;
            return;
        }
        if (y > lbl_805FF920) {
            *flags |= 0x4000u;
        }
    }
}

// 0x80199CE8
// Queues an object's render entry into a list if there is room and its state allows it.
// Returns -2 for bad args, -1 when the list is full, -7 when not eligible, 0 otherwise.
s32 UIScreen_AddDeferredRender(u8* obj, u8* list) {
    if (obj == NULL || list == NULL) {
        return -2;
    }
    if (*(s32*)(list + 0x4) + 1 > *(s32*)list) {
        fn_8018D370((const char*)lbl_80382748);
        return -1;
    }
    s32 flags = *(s32*)(obj + 0x64);
    if ((flags & 2) == 0) {
        return -7;
    }
    if (!(*(f32*)(obj + 0x8C) > lbl_805FF930)) {
        return -7;
    }
    u8* sub = *(u8**)(obj + 0x128);
    if (sub != NULL && (*(s32*)(sub + 0x64) & 2) == 0) {
        return -7;
    }
    if ((flags & 0x40) == 0) {
        VFnObjList fn = *(VFnObjList*)(*(u8**)(obj + 0x138) + 0x20);
        fn(obj, list);
    }
    return 0;
}

// 0x801BB434
// Event callback: if obj has flag 2 at +0x28 it forwards to fn_801BBB68. Returns 1 for a
// NULL object or after forwarding, 0 otherwise.
s32 fn_801BB434(void* mgr, s32 id, s32 a, s32 b) {
    if (mgr == NULL) {
        return 1;
    }
    if ((*(s32*)((u8*)mgr + 0x28) & 2) != 0) {
        fn_801BBB68(mgr, id, a, b);
        return 1;
    }
    return 0;
}

// 0x801BB530
// Writes a byte index into out: 0 when the entry is empty, else the entry's index plus 1
// (taken from +0xC when that is set, else +0x4).
void fn_801BB530(u8* arg0, u8* arg1) {
    u8* obj = *(u8**)arg0;
    if (*(u8*)(obj + 0x44) == 0 || arg1 == NULL) {
        return;
    }
    s32 idx = *(s32*)(arg0 + 0x4);
    if (idx == -1) {
        *arg1 = 0;
        return;
    }
    s32 alt = *(s32*)(arg0 + 0xC);
    if (alt != -1) {
        idx = alt;
    }
    *arg1 = (u8)(idx + 1);
}

// 0x801BC098
// Fires the action packed in state's word +0x10 when its type field is non-zero.
void fn_801BC098(u8* state, u8* hud) {
    u32 word = *(u32*)(state + 0x10);
    if (((word >> 27) & 0xF) != 0) {
        FireActionWord(word, hud);
    }
}

// 0x801BC350
// If state+0x14 is set, stores a pointer into the list entry selected by hud+8 (NULL if
// none); otherwise fires the action packed in state+0xC when its type field is non-zero.
void fn_801BC350(u8* arg0, u8* arg1) {
    if (*(s32*)(arg0 + 0x14) != 0) {
        s32 idx = *(s32*)(arg1 + 0x8);
        u8* result = NULL;
        if (idx > -1) {
            result = *(u8**)(*(u8**)arg1) + idx * 0x30;
        }
        *(u8**)(arg0 + 0x4) = result;
        return;
    }
    u32 word = *(u32*)(arg0 + 0xC);
    if (((word >> 27) & 0xF) != 0) {
        FireActionWord(word, arg1);
    }
}

// 0x801BC41C
// Returns 1 when the action word at +8 has no type; otherwise fires it and returns the
// result of TriggerDoAction.
BOOL fn_801BC41C(u8* arg0, u8* arg1) {
    u32 word = *(u32*)(arg0 + 0x8);
    if (((word >> 27) & 0xF) == 0) {
        return 1;
    }
    return FireActionWord(word, arg1);
}

// 0x801BC5F0
// Initialises the object's pointer slots: obj+0x34 gets a static block, each entry of the
// array at obj+0 gets a static pointer, and each entry of the pointer array at obj+0x10
// gets a static block chosen by its type (0..4).
void fn_801BC5F0(void* obj) {
    u8* o = (u8*)obj;
    *(void**)(o + 0x34) = lbl_80383F58;
    u8* base = *(u8**)o;
    s32 count = *(s32*)(o + 0x4);
    for (s32 i = 0, off = 0; i < count; i++, off += 0x30) {
        u8* elt = base + off;
        if (elt != NULL) {
            *(void**)elt = lbl_80383F90;
        }
    }
    s32 count2 = *(s32*)(o + 0x1C);
    for (s32 i = 0, off = 0; i < count2; i++, off += 4) {
        u8* node = *(u8**)(*(u8**)(o + 0x10) + off);
        s32 type = *(s32*)(node + 0x8);
        if (type == 0) {
            if (node != NULL) {
                *(void**)node = lbl_80383FC4;
            }
        } else if (type == 2) {
            if (node != NULL) {
                *(void**)node = lbl_80383F58;
            }
        } else if (type == 3) {
            if (node != NULL) {
                *(void**)node = lbl_80383F20;
            }
        } else if (type == 4) {
            if (node != NULL) {
                *(void**)node = lbl_80383EE8;
            }
        }
    }
}

// 0x801BC6F0
// Looks up the entry with key id in the table at base+0x20 (count at +0x2C) and returns its
// value if fn_801BC4C4(value, sub) is 0; NULL otherwise.
u8* fn_801BC6F0(u8* base, s32 sub, s32 id) {
    s32 count = *(s32*)(base + 0x2C);
    for (s32 i = 0, off = 0; i < count; i++, off += 0xC) {
        u8* arr = *(u8**)(base + 0x20);
        if (*(s32*)(arr + off) == id && fn_801BC4C4(*(u8**)(arr + off + 0x4), sub) == 0) {
            return *(u8**)(arr + off + 0x4);
        }
    }
    return NULL;
}

// 0x801BE3A4
// For each of the object's entries (0x70 bytes, count at +0xC, array at +0x10), walks the
// intrusive list at entry+0x68 -> +0x14 (nodes are stored 8 bytes past their link word)
// and calls fn_801BF714 on every node.
void fn_801BE3A4(u8* arg0) {
    s32 count = *(s32*)(arg0 + 0xC);
    u8* ent = *(u8**)(arg0 + 0x10);
    for (s32 i = 0; i < count; i++, ent += 0x70) {
        u8* head = *(u8**)(*(u8**)(ent + 0x68) + 0x14);
        u8* node = head != NULL ? head + 0x8 : NULL;
        while (node != NULL) {
            u8* raw = *(u8**)(node - 0x4);
            u8* next = raw != NULL ? raw + 0x8 : NULL;
            fn_801BF714(node);
            node = next;
        }
    }
}

}  // extern "C"
