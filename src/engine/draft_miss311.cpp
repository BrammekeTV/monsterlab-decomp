// Drafts miss311: script-function thunks, tween step, effect-list helpers.
#include "types.h"

struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes
extern "C++" ScriptFunc gScriptFuncs[];                // 0x80484230

extern "C" {
void* MemAllocDefault(u32 size);                               // 0x801D4650
s32   fn_80031114(void* list, f32* v);                         // 0x80031114
void  fn_8025C650(void* a, u8* b);                             // 0x8025C650
void  fn_8025B780(void* a, u8* b);                             // 0x8025B780

extern u8   lbl_80348F38[];     // 0x80348F38 (dispatch table)
extern u8   lbl_803C54F0[];     // 0x803C54F0 (16 records of 0x18 bytes)
extern u8   lbl_805FBA9C;       // 0x805FBA9C (platform/mode, 2 = alternate)
extern f32  lbl_805FD3F0;       // 0x805FD3F0 (0.0f)
extern f32  lbl_805FD3F4;       // 0x805FD3F4 (100.0f)
extern f32  lbl_805FFCB0;       // 0x805FFCB0 (0.0f)
extern f32  lbl_805FFCB4;       // 0x805FFCB4 (1.0f)
extern f32  lbl_80600098;       // 0x80600098 (0.0f)

typedef s32  (*QueryFn)(void* obj, s32 what);
typedef void (*StoreFn)(void* obj, s32 what, s32 bits);
typedef s32  (*VCall18Fn)(void* obj, s32 a, void* b, f32 f1, f32 f2, f32 f3, f32 f4, f32 f5, f32 f6, f32 f7);
typedef s32  (*VCall18bFn)(void* obj, s32 a, f32 f1, f32 f2);
typedef s32  (*NodeFn)(u8* node, u8* entry);

// 0x801C5AAC
// Initializes an animation channel record from its source definition.
void fn_801C5AAC(u8* arg0, u8* arg1) {
    f32 zero = lbl_805FFCB0;
    f32 one = lbl_805FFCB4;
    *(s32*)(arg0 + 0x4) = 0;
    *(u8**)(arg0 + 0x0) = arg1;
    *(f32*)(arg0 + 0x8) = zero;
    *(f32*)(arg0 + 0xC) = zero;
    *(u32*)(arg0 + 0x10) = *(u32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x20) = zero;
    *(f32*)(arg0 + 0x24) = one;
    *(u32*)(arg0 + 0x28) = *(u32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x38) = one;
    *(f32*)(arg0 + 0x3C) = zero;
    u32 w = *(u32*)(arg1 + 0x4) & 0xFFFFFF;
    *(f32*)(arg0 + 0x50) = one;
    *(u32*)(arg0 + 0x40) = w;
    *(f32*)(arg0 + 0x54) = one;
    *(u32*)(arg0 + 0x58) = *(u32*)(arg1 + 0x4) & 0xFFFFFF;
}

// 0x801E2400
// Tail call: gScriptFuncs[idx].fn(a).
void fn_801E2400(u32 idx, void* a) {
    ((void (*)(void*))gScriptFuncs[idx].fn)(a);
}

// 0x801E2724
// Tail call: gScriptFuncs[idx].fn(a, b).
void fn_801E2724(u32 idx, u32 a, void* b) {
    ((void (*)(u32, void*))gScriptFuncs[idx].fn)(a, b);
}

// 0x801E2748
// Tail call: gScriptFuncs[idx].fn(a, b).
void fn_801E2748(u32 idx, u32 a, void* b) {
    ((void (*)(u32, void*))gScriptFuncs[idx].fn)(a, b);
}

// 0x801E276C
// Tail call: gScriptFuncs[idx].fn(a, b, c).
void fn_801E276C(u32 idx, u32 a, u32 b, void* c) {
    ((void (*)(u32, u32, void*))gScriptFuncs[idx].fn)(a, b, c);
}

// 0x801E2794
// Tail call: gScriptFuncs[idx].fn(a, b, c).
void fn_801E2794(u32 idx, u32 a, u32 b, void* c) {
    ((void (*)(u32, u32, void*))gScriptFuncs[idx].fn)(a, b, c);
}

// 0x801FCDB4
// Tail call into the platform-specific variant (mode 2 = alternate).
void fn_801FCDB4(void* a, u8* b) {
    if (lbl_805FBA9C == 2) {
        fn_8025C650(a, b);
    } else {
        fn_8025B780(a, b);
    }
}

// 0x801EBC74
// Walks the linked list hanging off +0x10 (of the object at +0x18, or the object itself),
// calling fn(node, entry) until it returns 0. Each link is a pointer stored 4 bytes before the node.
void fn_801EBC74(u8* arg0, void* fn, u8* entry) {
    u8* owner = *(u8**)(arg0 + 0x18);
    if (owner == NULL) {
        owner = arg0;
    }
    u8* first = *(u8**)(owner + 0x10);
    u8* node = first != NULL ? first + 8 : NULL;
    while (node != NULL) {
        u8* nextRaw = *(u8**)(node - 4);
        u8* next = nextRaw != NULL ? nextRaw + 8 : NULL;
        if (((NodeFn)fn)(node, entry) == 0) {
            break;
        }
        node = next;
    }
}

// 0x801F2118
// Tween step: moves the value (at +0x9C of the target, or read through the dispatch table) toward
// the goal at +0x8 by dt * speed * direction. Returns 3 = done / nothing to do, 2 = running.
s32 fn_801F2118(u8* arg0, f32 dt) {
    u8* target = *(u8**)(arg0 + 0x4);
    f32 base;
    if (target == NULL && *(u8**)(arg0 + 0x0) == NULL) {
        return 3;
    }
    if (target != NULL) {
        base = *(f32*)(target + 0x9C);
    } else {
        u8* obj = *(u8**)(arg0 + 0x0);
        u8* tbl = lbl_80348F38 + *(u8*)(obj + 0xC) * 4;
        s32 bits = (*(QueryFn*)(tbl + 0x30))(*(void**)(obj + 0x8), 0x10);
        base = *(f32*)&bits;
    }
    s32 done = 0;
    f32 value;
    if (!(*(f32*)(arg0 + 0x70) < *(f32*)(arg0 + 0xC))) {
        value = *(f32*)(arg0 + 0x8);
        done = 1;
    } else {
        f32 dir = *(f32*)(arg0 + 0x14);
        f32 speed = *(f32*)(arg0 + 0x10);
        value = base + dt * (dir * speed);
        if ((dir > lbl_80600098 && value > *(f32*)(arg0 + 0x8)) ||
            (*(f32*)(arg0 + 0x14) < lbl_80600098 && value < *(f32*)(arg0 + 0x8))) {
            value = *(f32*)(arg0 + 0x8);
        }
    }
    u8* obj = *(u8**)(arg0 + 0x0);
    if (obj != NULL) {
        u8* tbl = lbl_80348F38 + *(u8*)(obj + 0xC) * 4;
        (*(StoreFn*)(tbl + 0x24))(*(void**)(obj + 0x8), 0x10, *(s32*)&value);
    } else {
        *(f32*)(*(u8**)(arg0 + 0x4) + 0x9C) = value;
    }
    if (done != 0) {
        return 3;
    }
    return 2;
}

// 0x801E47D8
// Allocates the two pool buffers (0x1400 and 0xB200 bytes) and clears the first byte of every
// 0x164-byte slot in the 8 records (0xB20 bytes each) of the second buffer.
void fn_801E47D8(void* pool) {
    u8* arg0 = (u8*)pool;
    *(void**)(arg0 + 0x4) = MemAllocDefault(0x1400);
    *(void**)(arg0 + 0x0) = MemAllocDefault(0xB200);
    *(void**)(arg0 + 0x1C) = *(void**)(arg0 + 0x4);
    *(s32*)(arg0 + 0x18) = 0x80;
    *(s32*)(arg0 + 0x14) = -1;
    *(s32*)(arg0 + 0x10) = -1;
    *(s32*)(arg0 + 0xC) = 0;
    s32 off = 0;
    for (s32 i = 0; i < 8; i++) {
        u8* rec = *(u8**)(arg0 + 0x0) + off;
        rec[0x0] = 0;
        rec[0x164] = 0;
        rec[0x2C8] = 0;
        rec[0x42C] = 0;
        rec[0x590] = 0;
        rec[0x6F4] = 0;
        rec[0x858] = 0;
        rec[0x9BC] = 0;
        off += 0xB20;
    }
    *(s32*)(arg0 + 0x20) = 0;
}

// 0x8002BE88
// Updates the screen-space marker: copies v[0..1] into +0x40/+0x44 when the list at +0x2C accepts
// it, pushes the position into the current element and the objects at +0x0 / +0x48 via their
// virtual slot 0x2C, then rebuilds the bounds rect (+0x30..+0x3C) from the radius at +0x18.
void fn_8002BE88(u8* arg0, f32* arg1, u8* el) {
    (void)el;
    if (fn_80031114(*(void**)(arg0 + 0x2C), arg1) != 0) {
        *(f32*)(arg0 + 0x40) = arg1[0];
        *(f32*)(arg0 + 0x44) = arg1[1];
    }
    f32 f6 = *(f32*)(arg0 + 0x40);
    f32 f7 = *(f32*)(arg0 + 0x44);
    u8* cur = *(u8**)(arg0 + *(s32*)(arg0 + 0x8) * 4);
    u8* obj = *(u8**)(arg0 + 0x0);
    f32 f3 = *(f32*)(cur + 0x70);
    f32 f5 = *(f32*)(cur + 0x68);
    f32 f4 = *(f32*)(cur + 0x6C);
    *(f32*)(obj + 0x68) = f6;
    *(f32*)(obj + 0x6C) = f7;
    *(f32*)(obj + 0x70) = f3;
    obj = *(u8**)(arg0 + 0x0);
    (*(VCall18Fn*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x18, obj, f7, f6, f3, f4, f5, f6, f7);

    u8* o2 = *(u8**)(arg0 + 0x48);
    f32 z = *(f32*)(o2 + 0x70);
    *(f32*)(o2 + 0x68) = f6;
    *(f32*)(o2 + 0x6C) = f7;
    o2 = *(u8**)(arg0 + 0x48);
    (*(VCall18bFn*)(*(u8**)(o2 + 0x138) + 0x2C))(o2, 0x18, f6, z);

    f32 x = *(f32*)(arg0 + 0x40);
    f32 r = *(f32*)(arg0 + 0x18);
    f32 y = *(f32*)(arg0 + 0x44);
    *(f32*)(arg0 + 0x30) = x - r;
    *(f32*)(arg0 + 0x34) = y - r;
    *(f32*)(arg0 + 0x38) = x + r;
    *(f32*)(arg0 + 0x3C) = y + r;
}

// 0x8005CBC4
// Picks a slot in the 16-entry table (0x18-byte records, float at +0x10): the first free one
// (value 0), else the last one whose value is below 100.
u8* fn_8005CBC4(void) {
    f32 zero = lbl_805FD3F0;
    f32 limit = lbl_805FD3F4;
    u8* p = lbl_803C54F0;
    s32 best = 0;
    for (s32 i = 0; i < 16; i++) {
        f32 v = *(f32*)(p + 0x10);
        if (zero == v) {
            return lbl_803C54F0 + i * 0x18;
        }
        if (v < limit) {
            best = i;
        }
        p += 0x18;
    }
    return lbl_803C54F0 + best * 0x18;
}

}  // extern "C"
